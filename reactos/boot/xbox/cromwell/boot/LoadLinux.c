/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "boot.h"
#include "video.h"
#include "memory_layout.h"
#include <shared.h>
#include <filesys.h>
#include "rc4.h"
#include "sha1.h"
#include "BootFATX.h"
#include "xbox.h"
#include "BootFlash.h"
#include "cpu.h"
#include "BootIde.h"

#include "config.h"


static int nRet;
static DWORD dwKernelSize= 0, dwInitrdSize = 0;


int ExittoLinux(CONFIGENTRY *config);
void startLinux(void* initrdStart, unsigned long initrdSize, const char* appendLine);


void BootPrintConfig(CONFIGENTRY *config) {
	int CharsProcessed=0, CharsSinceNewline=0, Length=0;
	char c;
	printk("  Bootconfig : Kernel  %s \n", config->szKernel);
	VIDEO_ATTR=0xffa8a8a8;
	printk("  Bootconfig : Initrd  %s \n", config->szInitrd);
	VIDEO_ATTR=0xffa8a8a8;
	printk("  Bootconfig : Appended arguments :\n");
	Length=strlen(config->szAppend);
	while (CharsProcessed<Length) {
		c = config->szAppend[CharsProcessed];
		CharsProcessed++;
		CharsSinceNewline++;
		if ((CharsSinceNewline>50 && c==' ') || CharsSinceNewline>65) {
			printk("\n");
			if (CharsSinceNewline>25) printk("%c",c);
			CharsSinceNewline = 0;
		} 
		else printk("%c",c);
	}
	printk("\n");
	VIDEO_ATTR=0xffa8a8a8;
}


void memPlaceKernel(const BYTE* kernelOrg, DWORD kernelSize)
{
	unsigned int nSizeHeader=((*(kernelOrg + 0x01f1))+1)*512;
	memcpy((BYTE *)KERNEL_SETUP, kernelOrg, nSizeHeader);
	memcpy((BYTE *)KERNEL_PM_CODE,(kernelOrg+nSizeHeader),kernelSize-nSizeHeader);
}



// if fJustTestingForPossible is true, returns 0 if this kind of boot not possible, 1 if it is worth trying
int BootLoadConfigNative(int nActivePartition, CONFIGENTRY *config, bool fJustTestingForPossible) {
	DWORD dwConfigSize=0;
	char *szGrub;
	BYTE* tempBuf;
        
	szGrub = (char *) malloc(265+4);
        memset(szGrub,0,256+4);
        
	memset((BYTE *)KERNEL_SETUP,0,2048);

	szGrub[0]=0xff;
	szGrub[1]=0xff;
	szGrub[2]=nActivePartition;
	szGrub[3]=0x00;

	errnum=0;
	boot_drive=0;
	saved_drive=0;
	saved_partition=0x0001ffff;
	buf_drive=-1;
	current_partition=0x0001ffff;
	current_drive=0xff;
	buf_drive=-1;
	fsys_type = NUM_FSYS;
	disk_read_hook=NULL;
	disk_read_func=NULL;

	VIDEO_ATTR=0xffa8a8a8;

	//Try for /boot/linuxboot.cfg first
	strcpy(&szGrub[4], "/boot/linuxboot.cfg");
	nRet=grub_open(szGrub);

	if(nRet!=1 || (errnum)) {
		//Not found - try /linuxboot.cfg
		errnum=0;
		strcpy(&szGrub[4], "/linuxboot.cfg");
		nRet=grub_open(szGrub);
	}
	
	dwConfigSize=filemax;
	if (nRet!=1 || (errnum)) {
		if(!fJustTestingForPossible) printk("linuxboot.cfg not found, using defaults\n");
	} else {
		int nLen;
		if(fJustTestingForPossible) return 1; // if there's a linuxboot.cfg it must be worth trying to boot
		nLen=grub_read((void *)KERNEL_SETUP, filemax);
		if(nLen>0) { ((char *)KERNEL_SETUP)[nLen]='\0'; }  // needed to terminate incoming string, reboot in ParseConfig without it
		ParseConfig((char *)KERNEL_SETUP,config,&eeprom, NULL);
		BootPrintConfig(config);
		printf("linuxboot.cfg is %d bytes long.\n", dwConfigSize);
	}
	grub_close();
	
        strncpy(&szGrub[4], config->szKernel,sizeof(config->szKernel));

	nRet=grub_open(szGrub);

	if(nRet!=1) {
		if(fJustTestingForPossible) return 0;
		printk("Unable to load kernel, Grub error %d\n", errnum);
		while(1) ;
	}
	if(fJustTestingForPossible) return 1; // if there's a default kernel it must be worth trying to boot
        
	// Use INITRD_START as temporary location for loading the Kernel 
	tempBuf = (BYTE*)INITRD_START;
	dwKernelSize=grub_read(tempBuf, MAX_KERNEL_SIZE);
	memPlaceKernel(tempBuf, dwKernelSize);
	grub_close();
	printk(" -  %d bytes...\n", dwKernelSize);

	if( (strncmp(config->szInitrd, "/no", strlen("/no")) != 0) && config->szInitrd[0]) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s ", config->szInitrd);
		VIDEO_ATTR=0xffa8a8a8;
 		strncpy(&szGrub[4], config->szInitrd,sizeof(config->szInitrd));
		nRet=grub_open(szGrub);
		if(filemax==0) {
			printf("Empty file\n"); while(1);
		}
		if( (nRet!=1) || (errnum)) {
			printk("Unable to load initrd, Grub error %d\n", errnum);
			while(1) ;
		}
		printk(" - %d bytes\n", filemax);
		dwInitrdSize=grub_read((void*)INITRD_START, MAX_INITRD_SIZE);
		grub_close();
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
	}
	free(szGrub);
	return true;
}

#ifdef FLASH 
int BootLoadFlashCD(void) {
	
	DWORD dwConfigSize=0, dw;
	int n;
	int cdPresent=0;
	int cdromId=0;
	DWORD dwY=VIDEO_CURSOR_POSY;
	DWORD dwX=VIDEO_CURSOR_POSX;
	BYTE* tempBuf;
	struct SHA1Context context;
	unsigned char SHA1_result[20];
	unsigned char checksum[20];
	 
	memset((BYTE *)KERNEL_SETUP,0,4096);

	//See if we already have a CDROM in the drive
	//Try for 4 seconds.
	I2CTransmitWord(0x10, 0x0c01); // close DVD tray
	for (n=0;n<16;++n) {
		if((BootIso9660GetFile(cdromId,"/image.bin", (BYTE *)KERNEL_SETUP, 0x10)) >=0 ) {
			cdPresent=1;
			break;
		}
		wait_ms(250);
	}

	if (!cdPresent) {
		//Needs to be changed for non-xbox drives, which don't have an eject line
		//Need to send ATA eject command.
		I2CTransmitWord(0x10, 0x0c00); // eject DVD tray
		wait_ms(2000); // Wait for DVD to become responsive to inject command
			
		VIDEO_ATTR=0xffeeeeff;
		VIDEO_CURSOR_POSX=dwX;
		VIDEO_CURSOR_POSY=dwY;
		printk("Please insert CD with image.bin file on, and press Button A\n");

		while(1) {
			if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1) {
				I2CTransmitWord(0x10, 0x0c01); // close DVD tray
				wait_ms(500);
				break;
			}
        	        USBGetEvents();
			wait_ms(10);
		}						

		VIDEO_ATTR=0xffffffff;

		// wait until the media is readable
		while(1) {
			if((BootIso9660GetFile(cdromId,"/image.bin", (BYTE *)KERNEL_SETUP, 0x10)) >=0 ) {
				break;
			}
			wait_ms(200);
		}
	}
	printk("CDROM: ");
	printk("Loading bios image from CDROM:/image.bin. \n");
	dwConfigSize=BootIso9660GetFile(cdromId,"/image.bin", (BYTE *)KERNEL_SETUP, 0x800);
	dwConfigSize=BootIso9660GetFile("/IMAGE.BIN", (BYTE *)KERNEL_PM_CODE, 256*1024);
	
	if( dwConfigSize < 0 ) { //It's not there
		printk("image.bin not found on CDROM... Halting\n");
		while(1) ;
	}

	printk("Image size: %i\n", dwConfigSize);
        if (dwConfigSize!=256*1024) {
		printk("Image is not a 256kB image\n");
		while (1) ;
	}
	SHA1Reset(&context);
	SHA1Input(&context,(BYTE *)KERNEL_PM_CODE,dwConfigSize);
	SHA1Result(&context,SHA1_result);
	memcpy(checksum,SHA1_result,20);
	printk("Error code: $i\n", BootReflashAndReset((BYTE*) KERNEL_PM_CODE, (DWORD) 0, (DWORD) dwConfigSize));
	SHA1Reset(&context);
	SHA1Input(&context,(void *)LPCFlashadress,dwConfigSize);
	SHA1Result(&context,SHA1_result);
	if (memcmp(&checksum[0],&SHA1_result[0],20)==0) {
		printk("Checksum in flash matching - Flash successful.\nYou should now reboot.");
		while (1);
	} else {
		printk("Checksum in Flash not matching - MISTAKE -Reflashing!\n");
		printk("Error code: %i\n", BootReflashAndReset((BYTE*) KERNEL_PM_CODE, (DWORD) 0, (DWORD) dwConfigSize));
	}
}
#endif //Flash


int ExittoLinux(CONFIGENTRY *config) {
	VIDEO_ATTR=0xff8888a8;
	printk("     Kernel:  %s\n", (char *)(0x00090200+(*((WORD *)0x9020e)) ));
	printk("\n");

	{
		char *sz="\2Starting Linux\2";
		VIDEO_CURSOR_POSX=((vmode.width-BootVideoGetStringTotalWidth(sz))/2)*4;
		VIDEO_CURSOR_POSY=vmode.height-64;

		VIDEO_ATTR=0xff9f9fbf;
		printk(sz);
	}
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );
	startLinux((void*)INITRD_START, dwInitrdSize, config->szAppend);
}
	

void startLinux(void* initrdStart, unsigned long initrdSize, const char* appendLine)
{
	int nAta=0;
	// turn off USB
	BootStopUSB();
	setup( (void *)KERNEL_SETUP, initrdStart, initrdSize, appendLine);
        
	if(tsaHarddiskInfo[0].m_bCableConductors == 80) {
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&2) nAta=1;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&4) nAta=2;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&8) nAta=3;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&16) nAta=4;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&32) nAta=5;
	} else {
		// force the HDD into a good mode 0x40 ==UDMA | 2 == UDMA2
		nAta=2; // best transfer mode without 80-pin cable
	}
	// nAta=1;
	BootIdeSetTransferMode(0, 0x40 | nAta);
	BootIdeSetTransferMode(1, 0x40 | nAta);

	// orange, people seem to like that colour
	I2cSetFrontpanelLed(
		I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3 |
		I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3
	);
	         
	// Set framebuffer address to final location (for vesafb driver)
	(*(unsigned int*)0xFD600800) = (0xf0000000 | ((xbox_ram*0x100000) - FB_SIZE));
	
	// disable interrupts
	asm volatile ("cli\n");
	
	// clear idt area
	memset((void*)IDT_LOC,0x0,1024*8);
	
	__asm __volatile__ (
	"wbinvd\n"
	
	// Flush the TLB
	"xor %eax, %eax \n"
	"mov %eax, %cr3 \n"
	
	// Load IDT table (0xB0000 = IDT_LOC)
	"lidt 	0xB0000\n"
	
	// DR6/DR7: Clear the debug registers
	"xor %eax, %eax \n"
	"mov %eax, %dr6 \n"
	"mov %eax, %dr7 \n"
	"mov %eax, %dr0 \n"
	"mov %eax, %dr1 \n"
	"mov %eax, %dr2 \n"
	"mov %eax, %dr3 \n"
	
	// Kill the LDT, if any
	"xor	%eax, %eax \n"
	"lldt %ax \n"
        
	// Reload CS as 0010 from the new GDT using a far jump
	".byte 0xEA       \n"   // jmp far 0010:reload_cs
	".long reload_cs_exit  \n"
	".word 0x0010  \n"
	
	".align 16  \n"
	"reload_cs_exit: \n"

	// CS is now a valid entry in the GDT.  Set SS, DS, and ES to valid
	// descriptors, but clear FS and GS as they are not necessary.

	// Set SS, DS, and ES to a data32 segment with maximum limit.
	"movw $0x0018, %ax \n"
	"mov %eax, %ss \n"
	"mov %eax, %ds \n"
	"mov %eax, %es \n"

	// Clear FS and GS
	"xor %eax, %eax \n"
	"mov %eax, %fs \n"
	"mov %eax, %gs \n"

	// Set the stack pointer to give us a valid stack
	"movl $0x03BFFFFC, %esp \n"
	
	"xor 	%ebx, %ebx \n"
	"xor 	%eax, %eax \n"
	"xor 	%ecx, %ecx \n"
	"xor 	%edx, %edx \n"
	"xor 	%edi, %edi \n"
	"movl 	$0x90000, %esi\n"       // kernel setup area
	"ljmp 	$0x10, $0x100000\n"     // Jump to Kernel protected mode entry
	);
	
	// We are not longer here, we are already in the Linux loader, we never come back here
	
	// See you again in Linux then	
	while(1);
}

bool LinuxPresentOnCD(int cdromId) {
	return BootIso9660GetFile(cdromId,"/linuxboo.cfg", (BYTE *)KERNEL_SETUP, 0x800) >=0;
}

void BootLinuxFromCD(int cdromId)
{

	DWORD dwConfigSize=0, dw;
	int n;
	DWORD dwY=VIDEO_CURSOR_POSY;
	DWORD dwX=VIDEO_CURSOR_POSX;
	BYTE* tempBuf;
	CONFIGENTRY config;

	memset((BYTE *)KERNEL_SETUP,0,4096);

	printk("CDROM: ");
	printk("Loading linuxboot.cfg from CDROM... \n");
	dwConfigSize=BootIso9660GetFile(cdromId,"/linuxboo.cfg", (BYTE *)KERNEL_SETUP, 0x800);

	if( dwConfigSize < 0 ) { // has to be there on CDROM
		printk("linuxboot.cfg not found on CDROM... Halting\n");
		while(1) ;
	}
        // LinuxBoot.cfg File Loaded
	((char *)KERNEL_SETUP)[dwConfigSize]=0;
	ParseConfig((char *)KERNEL_SETUP,&config,&eeprom, NULL);
	BootPrintConfig(&config);

	// Use INITRD_START as temporary location for loading the Kernel 
	tempBuf = (BYTE*)INITRD_START;
	dwKernelSize=BootIso9660GetFile(cdromId,config.szKernel, tempBuf, MAX_KERNEL_SIZE);

	if( dwKernelSize < 0 ) {
		printk("Not Found, error %d\nHalting\n", dwKernelSize); 
		while(1);
	} else {
		memPlaceKernel(tempBuf, dwKernelSize);
		printk(" -  %d bytes...\n", dwKernelSize);
	}

	if( (strncmp(config.szInitrd, "/no", strlen("/no")) != 0) && config.szInitrd) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s from CDROM", config.szInitrd);
		VIDEO_ATTR=0xffa8a8a8;
		
		dwInitrdSize=BootIso9660GetFile(cdromId,config.szInitrd, (void*)INITRD_START, MAX_INITRD_SIZE);
		if( dwInitrdSize < 0 ) {
			printk("Not Found, error %d\nHalting\n", dwInitrdSize); 
			while(1);
		}
		
		printk(" - %d bytes\n", dwInitrdSize);
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
		printk("");
	}

	ExittoLinux(&config);
}

bool LinuxPresentOnFATX(FATXPartition *partition) {
	static int present = -1;
	FATXFILEINFO fileinfo;
	FATXFILEINFO infokernel;
	int nConfig = 0;
	BYTE *tempBuf;
	CONFIGENTRY config;
	bool partitionOpened;

	if (0 <= present) {
		return 0 != present;
	}

	if (NULL == partition) {
		partition = OpenFATXPartition(0,SECTOR_STORE,STORE_SIZE);
		if (NULL == partition) {
			present = 0;
			return false;
		}
		partitionOpened = true;
	} else {
		partitionOpened = false;
	}

	if(!LoadFATXFile(partition,"/linuxboot.cfg",&fileinfo)) {
		if(LoadFATXFile(partition,"/debian/linuxboot.cfg",&fileinfo) ) {
			fileinfo.buffer[fileinfo.fileSize]=0;
			ParseConfig(fileinfo.buffer,&config,&eeprom,"/debian");
			free(fileinfo.buffer);
		}
	} else {
		fileinfo.buffer[fileinfo.fileSize]=0;
		ParseConfig(fileinfo.buffer,&config,&eeprom,NULL);
		free(fileinfo.buffer);
	}

	// Use INITRD_START as temporary location for loading the Kernel 
	tempBuf = (BYTE*)INITRD_START;
	if(! LoadFATXFilefixed(partition,config.szKernel,&infokernel,tempBuf)) {
		present = 0;
	} else {
		present = 1; // worth trying, since the filesystem and kernel exists
	}

	if (partitionOpened) {
		CloseFATXPartition(partition);
	}

	return 0 != present;
}

void BootLinuxFromFATX(void)
{
	static FATXPartition *partition = NULL;
	static FATXFILEINFO fileinfo;
	static FATXFILEINFO infokernel;
	static FATXFILEINFO infoinitrd;
	CONFIGENTRY config;
	BYTE* tempBuf;

	memset((BYTE *)KERNEL_SETUP,0,4096);
	memset(&fileinfo,0x00,sizeof(fileinfo));
	memset(&infokernel,0x00,sizeof(infokernel));
	memset(&infoinitrd,0x00,sizeof(infoinitrd));

	I2CTransmitWord(0x10, 0x0c01); // Close DVD tray
	
	partition = OpenFATXPartition(0,
			SECTOR_STORE,
			STORE_SIZE);
	
	if(partition == NULL) return;

	printk("Loading linuxboot.cfg from FATX\n");

	if(LoadFATXFile(partition,"/linuxboot.cfg",&fileinfo) ) {
		wait_ms(50);
		fileinfo.buffer[fileinfo.fileSize]=0;
		ParseConfig(fileinfo.buffer,&config,&eeprom, NULL);
		free(fileinfo.buffer);
	} 
	else if(LoadFATXFile(partition,"/debian/linuxboot.cfg",&fileinfo) ) {
		wait_ms(50);
		fileinfo.buffer[fileinfo.fileSize]=0;
		ParseConfig(fileinfo.buffer,&config,&eeprom, "/debian");
		free(fileinfo.buffer);
	} else {
		wait_ms(50);
		printk("linuxboot.cfg not found, using defaults\n");
	}

	BootPrintConfig(&config);
	
	// Use INITRD_START as temporary location for loading the Kernel 
	tempBuf = (BYTE*)INITRD_START;
	if(! LoadFATXFilefixed(partition,config.szKernel,&infokernel,tempBuf)) {
		printk("Error loading kernel %s\n",config.szKernel);
		while(1);
	} else {
		dwKernelSize = infokernel.fileSize;
		// moving the kernel to its final location
		memPlaceKernel(tempBuf, dwKernelSize);
		
		printk(" -  %d %d bytes...\n", dwKernelSize, infokernel.fileRead);
	}
	
	if( (strncmp(config.szInitrd, "/no", strlen("/no")) != 0) && config.szInitrd[0]) {

		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s from FATX", config.szInitrd);
		wait_ms(50);
		if(! LoadFATXFilefixed(partition,config.szInitrd,&infoinitrd, (void*)INITRD_START)) {
			printk("Error loading initrd %s\n",config.szInitrd);
			while(1);
		}
		
		dwInitrdSize = infoinitrd.fileSize;
		printk(" - %d %d bytes\n", dwInitrdSize,infoinitrd.fileRead);
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
		printk("");
	}

	ExittoLinux(&config);
}

bool LinuxPresentOnNative(int partition) {
	CONFIGENTRY config;

	return BootLoadConfigNative(partition, &config, true);
}

//More grub bits
unsigned long saved_drive;
unsigned long saved_partition;
grub_error_t errnum;
unsigned long boot_drive;

extern unsigned long current_drive;

void BootLinuxFromNative(int partitionId) {
	CONFIGENTRY config;
	//This stuff is needed to keep the grub FS code happy.
	char szGrub[256+4];
	int menu=0,selected=0;
	
	memset(szGrub,0x00,sizeof(szGrub));
	szGrub[0]=0xff;
	szGrub[1]=0xff;
	szGrub[2]=partitionId;
	szGrub[3]=0x00;
	errnum=0;
	boot_drive=0;
	saved_drive=0;
	saved_partition=0x0001ffff;
	buf_drive=-1;
	current_partition=0x0001ffff;
	current_drive=0xff;
	buf_drive=-1;
	fsys_type = NUM_FSYS;
	disk_read_hook=NULL;
	disk_read_func=NULL;
	BootLoadConfigNative(*(int*)partitionId,&config,false);
	ExittoLinux(&config);
}

#ifdef ETHERBOOT
bool LinuxPresentOnEtherboot(void) {
        return true;
}

void BootLinuxFromEtherboot(void) {
        etherboot();
}
#endif
