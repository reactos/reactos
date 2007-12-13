/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "boot.h"
#include "BootFATX.h"
#include "LoadReactOS.h"
#include "memory_layout.h"

static void startReactOS(void);

int ExittoReactOS(CONFIGENTRY *config) {
	char *sz="\2Starting ReactOS\2";
	VIDEO_CURSOR_POSX=((vmode.width-BootVideoGetStringTotalWidth(sz))/2)*4;
	VIDEO_CURSOR_POSY=vmode.height-64;

	VIDEO_ATTR=0xff9f9fbf;
       	printk(sz);
        I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );
        startReactOS();
}

static void startReactOS(void)
{
	__asm __volatile__ (
//	"ljmp   $0x10, $0x90000\n"
	"mov	0x8202, %eax\n"
	"jmp	%eax\n"
// "jmp 0x831f\n"
	);

        // We are not longer here, we are already in freeldr, we never come back here

        // See you again in ReactOS then
        while(1);

}

/* Length of area to be searched for multiboot header */
#define MULTIBOOT_SEARCHAREA_LEN	8192

/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC		0x1BADB002

/* The Multiboot header. */
typedef struct tagMULTIBOOTHEADER {
	u_int32_t magic;
	u_int32_t flags;
	u_int32_t checksum;
	u_int32_t header_addr;
	u_int32_t load_addr;
	u_int32_t load_end_addr;
	u_int32_t bss_end_addr;
	u_int32_t entry_addr;
} MULTIBOOTHEADER, *PMULTIBOOTHEADER;

#define MB_INFO_FLAG_MEM_SIZE		0x00000001
#define MB_INFO_FLAG_BOOT_DEVICE	0x00000002
#define MB_INFO_FLAG_COMMAND_LINE	0x00000004
#define MB_INFO_FLAG_MODULES		0x00000008
#define MB_INFO_FLAG_AOUT_SYMS		0x00000010
#define MB_INFO_FLAG_ELF_SYMS		0x00000020
#define MB_INFO_FLAG_MEMORY_MAP		0x00000040
#define MB_INFO_FLAG_DRIVES		0x00000080
#define MB_INFO_FLAG_CONFIG_TABLE	0x00000100
#define MB_INFO_FLAG_BOOT_LOADER_NAME	0x00000200
#define MB_INFO_FLAG_APM_TABLE		0x00000400
#define MB_INFO_FLAG_GRAPHICS_TABLE	0x00000800

/* The symbol table for a.out. */
typedef struct tagAOUTSYMBOLTABLE {
	u_int32_t tabsize;
	u_int32_t strsize;
	u_int32_t addr;
	u_int32_t reserved;
} AOUTSYMBOLTABLE, *PAOUTSYMBOLTABLE;

/* The section header table for ELF. */
typedef struct tagELFSECTIONHEADERTABLE {
	u_int32_t num;
	u_int32_t size;
	u_int32_t addr;
	u_int32_t shndx;
} ELFSECTIONHEADERTABLE, *PELFSECTIONHEADERTABLE;

/* The Multiboot information. */
typedef struct tagMULTIBOOTINFO
{
	u_int32_t flags;
	u_int32_t mem_lower;
	u_int32_t mem_upper;
	u_int32_t boot_device;
	u_int32_t cmdline;
	u_int32_t mods_count;
	u_int32_t mods_addr;
	union {
		AOUTSYMBOLTABLE aout_sym;
		ELFSECTIONHEADERTABLE elf_sec;
	} u;
	u_int32_t mmap_length;
	u_int32_t mmap_addr;
	u_int32_t drives_length;
	u_int32_t drives_addr;
	u_int32_t config_table;
	u_int32_t boot_loader_name;
	u_int32_t apm_table;
	u_int32_t vbe_control_info;
	u_int32_t vbe_mode_info;
	u_int32_t vbe_mode;
	u_int32_t vbe_interface_seg;
	u_int32_t vbe_interface_off;
	u_int32_t vbe_interface_len;
} MULTIBOOTINFO, *PMULTIBOOTINFO;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size. */
typedef struct tagMEMORYMAP {
	u_int32_t size;
	u_int32_t base_addr_low;
	u_int32_t base_addr_high;
	u_int32_t length_low;
	u_int32_t length_high;
	u_int32_t type;
} MEMORYMAP, *PMEMORYMAP;


static void BootReactOS(BYTE *buffer, u_int32_t bootDevice)
{
	PMULTIBOOTHEADER mbHeader;
	PMULTIBOOTINFO mbInfo;
	PMEMORYMAP mmap;
	int nAta;
	char *sz="\2Starting ReactOS\2";

	VIDEO_CURSOR_POSX=((vmode.width-BootVideoGetStringTotalWidth(sz))/2)*4;
	VIDEO_CURSOR_POSY=vmode.height-64;

	VIDEO_ATTR=0xff9f9fbf;
       	printk(sz);

	for (mbHeader = (PMULTIBOOTHEADER) buffer;
	     (BYTE *) mbHeader - buffer < MULTIBOOT_SEARCHAREA_LEN;
	     mbHeader = (PMULTIBOOTHEADER) ((BYTE *) mbHeader + 4)) {
		if (MULTIBOOT_HEADER_MAGIC == mbHeader->magic &&
		    0 == mbHeader->magic + mbHeader->flags + mbHeader->checksum) {
			break;
		}
	}

	if (MULTIBOOT_SEARCHAREA_LEN <= (BYTE *) mbHeader - buffer) {
		printk("No multiboot header found\n");
		return;
	}

	/* Prepare to move freeldr to its final destination. Since that might
	 * involve overwriting cromwell structures we shutdown as much as
	 * possible. After this point, we can't return anymore */
	BootStopUSB();
	if(80 == tsaHarddiskInfo[0].m_bCableConductors) {
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&2) nAta=1;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&4) nAta=2;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&8) nAta=3;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&16) nAta=4;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&32) nAta=5;
	} else {
                /* force the HDD into a good mode 0x40 ==UDMA | 2 == UDMA2 */
		nAta=2; /* best transfer mode without 80-pin cable */
	}
	BootIdeSetTransferMode(0, 0x40 | nAta);
	BootIdeSetTransferMode(1, 0x40 | nAta);

	/* orange, people seem to like that colour */
	I2cSetFrontpanelLed(
		I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3 |
		I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3
	);

	/* disable interrupts */
	__asm__ __volatile__("cli\n");

	/* clear idt area */
	memset((void*)IDT_LOC,0x0,1024*8);

	__asm__ __volatile__ (
	"wbinvd\n"

	/* Flush the TLB */
	"xor %eax, %eax \n"
	"mov %eax, %cr3 \n"

	/* Load IDT table (0xB0000 = IDT_LOC) */
	"lidt   0xB0000\n"

	/* DR6/DR7: Clear the debug registers */
	"xor %eax, %eax \n"
	"mov %eax, %dr6 \n"
	"mov %eax, %dr7 \n"
	"mov %eax, %dr0 \n"
	"mov %eax, %dr1 \n"
	"mov %eax, %dr2 \n"
	"mov %eax, %dr3 \n"

	/* Kill the LDT, if any */
	"xor    %eax, %eax \n"
	"lldt %ax \n"

	/* Reload CS as 0010 from the new GDT using a far jump */
	"ljmp	$0x0010,$reload_cs_exit \n"

        ".align 16  \n"
        "reload_cs_exit: \n"

	/* CS is now a valid entry in the GDT.  Set the other segment registers
	 * to valid descriptors (4Gb flat mode) as required by the multiboot
	 * spec */

	"movw $0x0018, %ax \n"
	"mov %eax, %ss \n"
	"mov %eax, %ds \n"
	"mov %eax, %es \n"
	"mov %eax, %fs \n"
	"mov %eax, %gs \n"

	/* Set the stack pointer to give us a valid stack */
	"movl $0x03BFFFFC, %esp \n"
	);

	/* Ok, preparations are complete. Now move the image */
	memcpy((BYTE *) mbHeader->load_addr,
	       (BYTE *) mbHeader - (mbHeader->header_addr - mbHeader->load_addr),
		mbHeader->load_end_addr - mbHeader->load_addr);
	memset((BYTE *) mbHeader->load_end_addr, 0x00, mbHeader->bss_end_addr - mbHeader->load_end_addr);

	/* Set up the multiboot info structure */
	mbInfo = (PMULTIBOOTINFO) ((mbHeader->bss_end_addr + 3) & ~ 0x3);
	memset(mbInfo, 0x00, sizeof(MULTIBOOTINFO));
	mbInfo->flags = MB_INFO_FLAG_MEM_SIZE | MB_INFO_FLAG_BOOT_DEVICE |
	                MB_INFO_FLAG_MEMORY_MAP;
	/* Multiboot spec says mem_lower can't be larger than 640, which is
	 * true for a PC. ince we're bending the multiboot rules anyway
	 * let's pass the full megabyte */
	mbInfo->mem_lower = 1024;
	mbInfo->mem_upper = (xbox_ram - 1) * 1024;
	mbInfo->boot_device = bootDevice;
	mbInfo->mmap_length = 2 * sizeof(MEMORYMAP);
	mmap = (PMEMORYMAP)(mbInfo + 1);
	mbInfo->mmap_addr = (u_int32_t) mmap + sizeof(u_int32_t);
	/* Normal RAM */
	mmap->size = sizeof(MEMORYMAP);
	mmap->base_addr_low = 0;
	mmap->base_addr_high = 0;
	mmap->length_low = (xbox_ram - 4) * 1024 * 1024;
	mmap->length_high = 0;
	mmap->type = 1;
	/* Video RAM */
	mmap++;
	mmap->size = sizeof(MEMORYMAP);
	mmap->base_addr_low = (xbox_ram - 4) * 1024 * 1024;
	mmap->base_addr_high = 0;
	mmap->length_low = 4 * 1024 * 1024;
	mmap->length_high = 0;
	mmap->type = 0;

	/* Now setup the registers and jump to the multiboot entry point */
	__asm__ __volatile__ (
	"xorl %%ecx,%%ecx \n"
	"xorl %%edx,%%edx \n"
	"xorl %%esi,%%esi \n"
	"xorl %%edi,%%edi \n"
	"pushl %%eax \n"
	"movl $0x2badb002,%%eax \n"
	"ret \n"
	: : "b" (mbInfo), "a" (mbHeader->entry_addr));
	
	/* We should never return here, but if we do there's not much we can
	 * do besides sitting around idly */
	while (1);
}

#define LOAD_AREA ((BYTE *) 0x00001000)
#define FATX_BOOTDEVICE 0x8000ffff	/* drive 0x80, partition 0 */
#define CD_BOOTDEVICE 0x9f00ffff	/* drive 0x9f is used by most BIOSes
					   for CDROM */

bool ReactOSPresentOnCD(int cdromId) {
	return BootIso9660GetFile(cdromId,"/loader/setupldr.sys", LOAD_AREA, 0x80000) >=0;
}

void BootReactOSFromCD(int cdromId)
{
	if (BootIso9660GetFile(cdromId,"/loader/setupldr.sys", LOAD_AREA, 0x80000) < 0) {
		printk("Unable to load setupldr\n");
		return;
	}

	BootReactOS(LOAD_AREA, CD_BOOTDEVICE);
}

bool ReactOSPresentOnFATX(FATXPartition *partition) {
	static int present = -1;
	FATXFILEINFO fileinfo;
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
	if(! FATXFindFile(partition,"/freeldr.sys",FATX_ROOT_FAT_CLUSTER,&fileinfo)) {
		present = 0;
	} else {
		present = 1;
	}
	if (partitionOpened) {
		CloseFATXPartition(partition);
	}

	return 0 != present;
}

void BootReactOSFromFATX(void) {
	FATXPartition *partition;
	FATXFILEINFO fileinfo;

	partition = OpenFATXPartition(0,SECTOR_STORE,STORE_SIZE);
	if (NULL == partition) {
		return;
	}

	memset(&fileinfo,0x00,sizeof(fileinfo));
	if(! LoadFATXFilefixed(partition,"/freeldr.sys",&fileinfo, LOAD_AREA) ) {
		CloseFATXPartition(partition);
		return;
	}

	BootReactOS(fileinfo.buffer, FATX_BOOTDEVICE);

	free(fileinfo.buffer);
}

/* ReactOS on native partitions not supported yet */
bool ReactOSPresentOnNative(int partition) {
	return false;
}

void BootReactOSFromNative(int partition) {
	printk("Can't boot ReactOS from native HDD yet\n");
}

#ifdef ETHERBOOT
/* ReactOS on Etherboot not supported yet */
bool ReactOSPresentOnEtherboot(void) {
	return false;
}

void BootReactOSFromEtherboot(void) {
	printk("Can't boot ReactOS from network yet\n");
}
#endif
