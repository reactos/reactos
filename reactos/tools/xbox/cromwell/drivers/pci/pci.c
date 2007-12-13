#include "boot.h"
// by ozpaulb@hotmail.com 2002-07-14

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

BYTE PciReadByte(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

		IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfc)));
		return IoInputByte(0xcfc + (reg_off & 3));
}

void PciWriteByte (unsigned int bus, unsigned int dev, unsigned int func,
		unsigned int reg_off, unsigned char byteval)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

	IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfc)));
	IoOutputByte(0xcfc + (reg_off & 3), byteval);
}


WORD PciReadWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

		IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfe)));
		return IoInputWord(0xcfc + (reg_off & 1));
}


void PciWriteWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, WORD w)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

	IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfc)));
	IoOutputWord(0xcfc + (reg_off & 1), w);
}


DWORD PciReadDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #
        base_addr |= ((func & 0x07) << 8);
        base_addr |= ((reg_off & 0xff));
        
	IoOutputDword(0xcf8, base_addr);
	return IoInputDword(0xcfc);
}


DWORD PciWriteDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, unsigned int dw) 
{
		
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #
	base_addr |= ((reg_off & 0xff));

	IoOutputDword(0xcf8, base_addr );	
	IoOutputDword(0xcfc ,dw);

	return 0;    
}
#define RTC_REG_A		10
#define RTC_REG_B		11
#define RTC_REG_C		12
#define RTC_REG_D		13
#define RTC_FREQ_SELECT		RTC_REG_A
#define RTC_CONTROL		RTC_REG_B
#define RTC_INTR_FLAGS		RTC_REG_C

/* On PCs, the checksum is built only over bytes 16..45 */
#define PC_CKS_RANGE_START	16
#define PC_CKS_RANGE_END	45
#define PC_CKS_LOC		46

#define RTC_RATE_1024HZ		0x06
#define RTC_REF_CLCK_32KHZ	0x20
#define RTC_FREQ_SELECT_DEFAULT (RTC_REF_CLCK_32KHZ | RTC_RATE_1024HZ)
#define RTC_24H 		0x02
#define RTC_CONTROL_DEFAULT (RTC_24H)


// access to RTC CMOS memory
BYTE CMOS_READ(BYTE addr) { 
	IoOutputByte(0x70,addr); 
	return IoInputByte(0x71); 
}

void CMOS_WRITE(BYTE val, BYTE addr) { 
	IoOutputByte(0x70,addr);
	IoOutputByte(0x71,val); 
}

void BiosCmosWrite(BYTE bAds, BYTE bData) {
	IoOutputByte(0x70, bAds);
	IoOutputByte(0x71, bData);

	IoOutputByte(0x72, bAds);
	IoOutputByte(0x73, bData);
}

BYTE BiosCmosRead(BYTE bAds)
{
	IoOutputByte(0x72, bAds);
	return IoInputByte(0x73);
}

int rtc_checksum_valid(int range_start, int range_end, int cks_loc)
{
	int i;
	unsigned sum, old_sum;
	sum = 0;
	for(i = range_start; i <= range_end; i++) {
		sum += CMOS_READ(i);
	}
	sum = (~sum)&0x0ffff;
	old_sum = ((CMOS_READ(cks_loc)<<8) | CMOS_READ(cks_loc+1))&0x0ffff;
	return sum == old_sum;
}

void rtc_set_checksum(int range_start, int range_end, int cks_loc)
{
	int i;
	unsigned sum;
	sum = 0;
	for(i = range_start; i <= range_end; i++) {
		sum += CMOS_READ(i);
	}
	sum = ~(sum & 0x0ffff);
	CMOS_WRITE(((sum >> 8) & 0x0ff), cks_loc);
	CMOS_WRITE(((sum >> 0) & 0x0ff), cks_loc+1);
}

void BootAGPBUSInitialization(void)
{
	DWORD temp;
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x54,   PciReadDword(BUS_0, DEV_1, FUNC_0, 0x54) | 0x88000000 );
	
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x64,   (PciReadDword(BUS_0, DEV_0, FUNC_0, 0x64))| 0x88000000 );
	
	temp =  PciReadDword(BUS_0, DEV_0, FUNC_0, 0x6C);
	IoOutputDword(0xcfc , temp & 0xFFFFFFFE);
	IoOutputDword(0xcfc , temp );
	
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x80, 0x00000100);
    
}

void BootDetectMemorySize(void)
{
	int i;
	int result;
	unsigned char *fillstring;
	void *membasetop = (void*)((64*1024*1024));
	void *membaselow = (void*)((0));
	
	(*(unsigned int*)(0xFD000000 + 0x100200)) = 0x03070103 ;
	(*(unsigned int*)(0xFD000000 + 0x100204)) = 0x11448000 ;
        
        PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x84, 0x7FFFFFF);  // 128 MB
        
	xbox_ram = 64;	
	fillstring = malloc(0x200);
	memset(fillstring,0xAA,0x200);
	memset(membasetop,0xAA,0x200);
	asm volatile ("wbinvd\n");
	
	if (!memcmp(membasetop,fillstring,0x200)) {
		// Looks like there is memory .. maybe a 128MB box 
		memset(fillstring,0x55,0x200);
		memset(membasetop,0x55,0x200);
		asm volatile ("wbinvd\n");
		if (!memcmp(membasetop,fillstring,0x200)) {
			// Looks like there is memory 
			// now we are sure, we set memory
                        if (memcmp(membaselow,fillstring,0x200) == 0) {
                             	// Hell, we find the Test-string at 0x0 too !
                             	xbox_ram = 64;
                        } else {
                        	xbox_ram = 128;
                        }
		}		
		
	}
	if (xbox_ram == 64) {
		PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x84, 0x3FFFFFF);  // 64 MB
	}
	else if (xbox_ram == 128) {
		PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x84, 0x7FFFFFF);  // 128 MB
	}
        free(fillstring);
}

void BootPciPeripheralInitialization()
{

	__asm__ __volatile__ ( "cli" );

	PciWriteDword(BUS_0, DEV_1, 0, 0x80, 2);  // v1.1 2BL kill ROM area
	if(PciReadByte(BUS_0, DEV_1, 0, 0x8)>=0xd1) { // check revision
		PciWriteDword(BUS_0, DEV_1, 0, 0xc8, 0x8f00);  // v1.1 2BL <-- death
	}

	// Bus 0, Device 0, Function 0 = PCI Bridge Device - Host Bridge
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x48, 0x00000114);
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x44, 0x80000000); // new 2003-01-23 ag  trying to get single write actions on TSOP
	PciWriteByte(BUS_0, DEV_0, FUNC_0, 0x87, 3); // kern 8001FC21
	PciWriteByte(BUS_0, DEV_0, 8, 0, 0x42);       // Xbeboot-compare
	
	IoOutputByte(0x2e, 0x55);
	IoOutputByte(0x2e, 0x26);
	IoOutputByte(0x61, 0xff);
	IoOutputByte(0x92, 0x01);
	IoOutputByte(0xcf9, 0x0);	// Reset Port
        IoOutputByte(0x43, 0x36);         	// Timer 0 (system time): mode 3
        IoOutputByte(0x40, 0xFF);              // 18.2Hz (1.19318MHz/65535)
        IoOutputByte(0x40, 0xFF);
        IoOutputByte(0x43, 0x54);         	// Timer 1 (ISA refresh): mode 2
        IoOutputByte(0x41, 18);                // 64KHz (1.19318MHz/18)
        IoOutputByte(0x00, 0);                 // clear base address 0
        IoOutputByte(0x00, 0);
        IoOutputByte(0x01, 0);                 // clear count 0
        IoOutputByte(0x01, 0);
        IoOutputByte(0x02, 0);                 // clear base address 1
        IoOutputByte(0x02, 0 );
        IoOutputByte(0x03, 0);                 // clear count 1
        IoOutputByte(0x03, 0);
        IoOutputByte(0x04, 0);                 // clear base address 2
        IoOutputByte(0x04, 0);
        IoOutputByte(0x05, 0);                 // clear count 2
        IoOutputByte(0x05, 0);
        IoOutputByte(0x06, 0);                 // clear base address 3
        IoOutputByte(0x06, 0);
        IoOutputByte(0x07, 0);                 // clear count 3
        IoOutputByte(0x07, 0);
        IoOutputByte(0x0B, 0x40);         	// set channel 0 to single mode, verify transfer
        IoOutputByte(0x0B, 0x41);         	// set channel 1 to single mode, verify transfer
        IoOutputByte(0x0B, 0x42);         	// set channel 2 to single mode, verify transfer
        IoOutputByte(0x0B, 0x43);         	// set channel 3 to single mode, verify transfer
        IoOutputByte(0x08, 0);                 // enable controller
        IoOutputByte(0xC0, 0);                 // clear base address 0
        IoOutputByte(0xC0, 0);
        IoOutputByte(0xC2, 0);                 // clear count 0
        IoOutputByte(0xC2, 0);
        IoOutputByte(0xC4, 0);                // clear base address 1
        IoOutputByte(0xC4, 0);
        IoOutputByte(0xC6, 0);                 // clear count 1
        IoOutputByte(0xC6, 0);
        IoOutputByte(0xC8, 0);                 // clear base address 2
        IoOutputByte(0xC8, 0);
        IoOutputByte(0xCA, 0);                 // clear count 2
        IoOutputByte(0xCA, 0);
        IoOutputByte(0xCC, 0);                 // clear base address 3
        IoOutputByte(0xCC, 0);
        IoOutputByte(0xCE, 0);                 // clear count 3
        IoOutputByte(0xCE, 0);
        IoOutputByte(0xD6, 0xC0);         // set channel 0 to cascade mode
        IoOutputByte(0xD6, 0xC1);         // set channel 1 to single mode, verify transfer
        IoOutputByte(0xD6, 0xC2);         // set channel 2 to single mode, verify transfer
        IoOutputByte(0xD6, 0xC3);         // set channel 3 to single mode, verify transfer
        IoOutputByte(0xD0, 0);                 // enable controller
        IoOutputByte(0x0E, 0);                 // enable DMA0 channels
        IoOutputByte(0xD4, 0);                 // clear chain 4 mask

	/* Setup the real time clock */
	CMOS_WRITE(RTC_CONTROL_DEFAULT, RTC_CONTROL);
	
	/* Setup the frequency it operates at */
	CMOS_WRITE(RTC_FREQ_SELECT_DEFAULT, RTC_FREQ_SELECT);
	
	/* Make certain we have a valid checksum */
	//rtc_set_checksum(PC_CKS_RANGE_START,
  	//PC_CKS_RANGE_END,PC_CKS_LOC);
	/* Clear any pending interrupts */
	(void) CMOS_READ(RTC_INTR_FLAGS);

	// configure ACPI hardware to generate interrupt on PIC-chip pin6 action (via EXTSMI#)
	IoOutputByte(0x80ce, 0x08);  // from 2bl RI#
	IoOutputByte(0x80c0, 0x08);  // from 2bl SMBUSC
	IoOutputByte(0x8004, IoInputByte(0x8004)|1);  // KERN: SCI enable == SCI interrupt generated
	IoOutputWord(0x8022, IoInputByte(0x8022)|2);  // KERN: Interrupt enable register, b1 RESERVED in AMD docs
	IoOutputWord(0x8023, IoInputByte(0x8023)|2);  // KERN: Interrupt enable register, b1 RESERVED in AMD docs
	IoOutputByte(0x8002, IoInputByte(0x8002)|1);  // KERN: Enable SCI interrupt when timer status goes high
	IoOutputWord(0x8028, IoInputByte(0x8028)|1);  // KERN: setting readonly trap event???
 
	I2CTransmitWord(0x10, 0x0b00); // Allow audio
	//I2CTransmitWord(0x10, 0x0b01); // GAH!!!  Audio Mute!
	
	// Bus 0, Device 1, Function 0 = nForce HUB Interface - ISA Bridge
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x6c, 0x0e065491);
	PciWriteByte(BUS_0, DEV_1, FUNC_0, 0x6a, 0x0003); // kern ??? gets us an int3?  vsync req
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x64, 0x00000b0c);
	PciWriteByte(BUS_0, DEV_1, FUNC_0, 0x81, PciReadByte(BUS_0, DEV_1, FUNC_0, 0x81)|8);
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x4c, 0x000f0000); // RTC clocks enable?  2Hz INT8?


	// Bus 0, Device 9, Function 0 = nForce ATA Controller
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x20, 0x0000ff61);	// (BMIBA) Set Busmaster regs I/O base address 0xff60
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 4, PciReadDword(BUS_0, DEV_9, FUNC_0, 4)|5); // 0x00b00005 );
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 8, PciReadDword(BUS_0, DEV_9, FUNC_0, 8)&0xfffffeff); // 0x01018ab1 ); // was fffffaff
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x58, 0x20202020); // kern1.1
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x60, 0x00000000); // kern1.1
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x50, 0x00000002);  // without this there is no register footprint at IO 1F0
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x2c, 0x00000000); // frankenregister from xbe boot
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x40, 0x00000000); // frankenregister from xbe boot
	
	// below reinstated by frankenregister compare with xbe boot
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x60, 0xC0C0C0C0); // kern1.1 <--- this was in kern1.1 but is FATAL for good HDD access

	// Bus 0, Device 4, Function 0 = nForce MCP Networking Adapter - all verified with kern1.1
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 4, PciReadDword(BUS_0, DEV_4, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 0x10, 0xfef00000); // memory base address 0xfef00000
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 0x14, 0x0000e001); // I/O base address 0xe000
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 0x118-0xdc, (PciReadDword(BUS_0, DEV_4, FUNC_0, 0x3c) &0xffff0000) | 0x0004 );


	// Bus 0, Device 2, Function 0 = nForce OHCI USB Controller - all verified with kern 1.1
	PciWriteDword(BUS_0, DEV_2, FUNC_0, 4, PciReadDword(BUS_0, DEV_2, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x10, 0xfed00000);	// memory base address 0xfed00000
	PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x3c, (PciReadDword(BUS_0, DEV_2, FUNC_0, 0x3c) &0xffff0000) | 0x0001 );
	PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x50, 0x0000000f);

	// Bus 0, Device 3, Function 0 = nForce OHCI USB Controller - verified with kern1.1
	PciWriteDword(BUS_0, DEV_3, FUNC_0, 4, PciReadDword(BUS_0, DEV_3, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_3, FUNC_0, 0x10, 0xfed08000);	// memory base address 0xfed08000
	PciWriteDword(BUS_0, DEV_3, FUNC_0, 0x3c, (PciReadDword(BUS_0, DEV_3, FUNC_0, 0x3c) &0xffff0000) | 0x0009 );
	PciWriteDword(BUS_0, DEV_3, FUNC_0, 0x50, 0x00000030);  // actually BYTE?

	// Bus 0, Device 6, Function 0 = nForce Audio Codec Interface - verified with kern1.1
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 4, PciReadDword(BUS_0, DEV_6, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x10, (PciReadDword(BUS_0, DEV_6, FUNC_0, 0x10) &0xffff0000) | 0xd001 );  // MIXER at IO 0xd000
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x14, (PciReadDword(BUS_0, DEV_6, FUNC_0, 0x14) &0xffff0000) | 0xd201 );  // BusMaster at IO 0xD200
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x18, 0xfec00000);	// memory base address 0xfec00000

	// frankenregister from working Linux driver
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 8, 0x40100b1 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0xc, 0x800000 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x3c, 0x05020106 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x44, 0x20001 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x4c, 0x107 );

	// Bus 0, Device 5, Function 0 = nForce MCP APU
	PciWriteDword(BUS_0, DEV_5, FUNC_0, 4, PciReadDword(BUS_0, DEV_5, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_5, FUNC_0, 0x3c, (PciReadDword(BUS_0, DEV_5, FUNC_0, 0x3c) &0xffff0000) | 0x0005 );
	PciWriteDword(BUS_0, DEV_5, FUNC_0, 0x10, 0xfe800000);	// memory base address 0xfe800000

	// Bus 0, Device 1, Function 0 = nForce HUB Interface - ISA Bridge
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x8c, (PciReadDword(BUS_0, DEV_1, FUNC_0, 0x8c) &0xfbffffff) | 0x08000000 );

	// ACPI pin init
	IoOutputDword(0x80b4, 0xffff);  // any interrupt resets ACPI system inactivity timer
	IoOutputByte(0x80cc, 0x08); // Set EXTSMI# pin to be pin function
	IoOutputByte(0x80cd, 0x08); // Set PRDY pin on ACPI to be PRDY function
	IoOutputByte(0x80cf, 0x08); // Set C32KHZ pin to be pin function
	IoOutputWord(0x8020, IoInputWord(0x8020)|0x200); // ack any preceding ACPI int
	
	// Bus 0, Device 1e, Function 0 = nForce AGP Host to PCI Bridge
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 4, PciReadDword(BUS_0, DEV_1e, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x18, (PciReadDword(BUS_0, DEV_1e, FUNC_0, 0x18) &0xffffff00));
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x3c, 7);  // trying to get video irq

	// frankenregister xbe load correction to match cromwell load
	// controls requests for memory regions
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x0c, 0xff019ee7);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x10, 0xbcfaf7e7);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x14, 0x0101fafa);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x1c, 0x02a000f0);    // Coud this be the BASE Video Address ?
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x20, 0xfdf0fd00);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x24, 0xf7f0f000);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x28, 0x8e7ffcff);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x2c, 0xf8bfef87);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x30, 0xdf758fa3);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x38, 0xb785fccc);

	// Bus 1, Device 0, Function 0 = NV2A GeForce3 Integrated GPU
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 4, PciReadDword(BUS_1, DEV_0, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 0x3c, (PciReadDword(BUS_1, DEV_0, FUNC_0, 0x3c) &0xffff0000) | 0x0103 );  // should get vid irq!!
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 0x4c, 0x00000114);

	// frankenregisters so Xromwell matches Cromwell
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 0x0c, 0x0);
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 0x18, 0x08);
}
