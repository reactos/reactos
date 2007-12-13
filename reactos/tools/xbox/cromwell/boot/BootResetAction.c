/*
 * Sequences the necessary post-reset actions from as soon as we are able to run C
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
 */

#include "boot.h"
#include "BootEEPROM.h"
#include "BootFlash.h"
#include "BootFATX.h"
#include "xbox.h"
#include "cpu.h"
#include "config.h"
#include "video.h"
#include "memory_layout.h"

JPEG jpegBackdrop;

int nTempCursorMbrX, nTempCursorMbrY;

extern volatile int nInteruptable;

volatile CURRENT_VIDEO_MODE_DETAILS vmode;
extern KNOWN_FLASH_TYPE aknownflashtypesDefault[];

//////////////////////////////////////////////////////////////////////
//
//  BootResetAction()

extern void BootResetAction ( void ) {
	bool fMbrPresent=false;
	bool fSeenActive=false;
	int nFATXPresent=false;
	int nTempCursorX, nTempCursorY;
	int n, nx;
	
        memcpy(&cromwell_config,(void*)(0x03A00000+0x20),4);
        memcpy(&cromwell_retryload,(void*)(0x03A00000+0x24),4);
	memcpy(&cromwell_loadbank,(void*)(0x03A00000+0x28),4);
        memcpy(&cromwell_Biostype,(void*)(0x03A00000+0x2C),4);
 	
	VIDEO_CURSOR_POSX=40;
	VIDEO_CURSOR_POSY=140; 	
        
	VIDEO_AV_MODE = 0xff;
	nInteruptable = 0;	

	// prep our BIOS console print state
	VIDEO_ATTR=0xffffffff;

	// init malloc() and free() structures
	MemoryManagementInitialization((void *)MEMORYMANAGERSTART, MEMORYMANAGERSIZE);
	
	BootInterruptsWriteIdt();	

	// initialize the PCI devices
	//bprintf("BOOT: starting PCI init\n\r");
	BootPciPeripheralInitialization();
	// Reset the AGP bus and start with good condition
	BootAGPBUSInitialization();
	
	// We disable The CPU Cache
       	cache_disable();
	// We Update the Microcode of the CPU
	display_cpuid_update_microcode();
       	// We Enable The CPU Cache
       	cache_enable();
       	//setup_ioapic();
	// We look how much memory we have ..
	BootDetectMemorySize();     
	
	BootEepromReadEntireEEPROM();
	bprintf("BOOT: Read EEPROM\n\r");
        
        // Load and Init the Background image
        // clear the Video Ram
	memset((void *)FB_START,0x00,0x400000);
	
	BootVgaInitializationKernelNG((CURRENT_VIDEO_MODE_DETAILS *)&vmode);

	{ // decode and malloc backdrop bitmap
		extern int _start_backdrop;
		BootVideoJpegUnpackAsRgb(
			(BYTE *)&_start_backdrop,
			&jpegBackdrop
		);
	}
	// display solid red frontpanel LED while we start up
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );
	// paint the backdrop
#ifndef DEBUG_MODE
	BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
#endif

	I2CTransmitWord(0x10, 0x1a01); // unknown, done immediately after reading out eeprom data
	I2CTransmitWord(0x10, 0x1b04); // unknown
	
	/* Here, the interrupts are Switched on now */
	BootPciInterruptEnable();
        /* We allow interrupts */
	nInteruptable = 1;	

	I2CTransmitWord(0x10, 0x1901); // no reset on eject
         
	VIDEO_CURSOR_POSY=vmode.ymargin;
	VIDEO_CURSOR_POSX=(vmode.xmargin/*+64*/)*4;
	
	if (cromwell_config==XROMWELL) 	printk("\2Xbox Linux Xromwell  " VERSION "\2\n" );
	if (cromwell_config==CROMWELL)	printk("\2Xbox Linux Cromwell BIOS  " VERSION "\2\n" );
	VIDEO_CURSOR_POSY=vmode.ymargin+32;
	VIDEO_CURSOR_POSX=(vmode.xmargin/*+64*/)*4;
	printk( __DATE__ " -  http://xbox-linux.org\n");
	VIDEO_CURSOR_POSX=(vmode.xmargin/*+64*/)*4;
	printk("(C)2002-2004 Xbox Linux Team   RAM : %d MB  ",xbox_ram);
        printk("\n");
    
	// capture title area
	VIDEO_ATTR=0xffc8c8c8;
	printk("Encoder: ");
	VIDEO_ATTR=0xffc8c800;
	printk("%s  ", VideoEncoderName());
	VIDEO_ATTR=0xffc8c8c8;
	printk("Cable: ");
	VIDEO_ATTR=0xffc8c800;
	printk("%s  ", AvCableName());
        
	if (I2CGetTemperature(&n, &nx)) {
		VIDEO_ATTR=0xffc8c8c8;
		printk("CPU Temp: ");
		VIDEO_ATTR=0xffc8c800;
		printk("%doC  ", n);
		VIDEO_ATTR=0xffc8c8c8;
		printk("M/b Temp: ");
		VIDEO_ATTR=0xffc8c800;
		printk("%doC  ", nx);
	}
	
	printk("\n");
	nTempCursorX=VIDEO_CURSOR_POSX;
	nTempCursorY=VIDEO_CURSOR_POSY;
		
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );

	VIDEO_ATTR=0xffffffff;

	// gggb while waiting for Ethernet & Hdd

	I2cSetFrontpanelLed(I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2);

	// set Ethernet MAC address from EEPROM
        {
	volatile BYTE * pb=(BYTE *)0xfef000a8;  // Ethernet MMIO base + MAC register offset (<--thanks to Anders Gustafsson)
	int n;
	for(n=5;n>=0;n--) { *pb++=	eeprom.MACAddress[n]; } // send it in backwards, its reversed by the driver
        }

	BootEepromPrintInfo();
#ifdef FLASH
	{
		OBJECT_FLASH of;
		memset(&of,0x00,sizeof(of));
		of.m_pbMemoryMappedStartAddress=(BYTE *)LPCFlashadress;
		BootFlashGetDescriptor(&of, (KNOWN_FLASH_TYPE *)&aknownflashtypesDefault[0]);
		VIDEO_ATTR=0xffc8c8c8;
		printk("Flash type: ");
		VIDEO_ATTR=0xffc8c800;
		printk("%s\n", of.m_szFlashDescription);
	}
#endif
	printk("BOOT: start USB init\n");
	BootStartUSB();

	// init the IDE devices
	VIDEO_ATTR=0xffc8c8c8;
	printk("Initializing IDE Controller\n");
	BootIdeWaitNotBusy(0x1f0);
       	wait_ms(200);
	
	printk("Ready\n");
	
	// reuse BIOS status area

#ifndef DEBUG_MODE
	BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);  // blank out volatile data area
#endif
	VIDEO_CURSOR_POSX=nTempCursorX;
	VIDEO_CURSOR_POSY=nTempCursorY;

	bprintf("Entering BootIdeInit()\n");
	BootIdeInit();
	printk("\n");

	nTempCursorMbrX=VIDEO_CURSOR_POSX;
	nTempCursorMbrY=VIDEO_CURSOR_POSY;

	// if we made it this far, lets have a solid green LED to celebrate
	I2cSetFrontpanelLed(
		I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3
	);

//	printk("i2C=%d SMC=%d, IDE=%d, tick=%d una=%d unb=%d\n", nCountI2cinterrupts, nCountInterruptsSmc, nCountInterruptsIde, BIOS_TICK_COUNT, nCountUnusedInterrupts, nCountUnusedInterruptsPic2);

	MainMenu();
	//Should never come back here.
	while(1);  
}
