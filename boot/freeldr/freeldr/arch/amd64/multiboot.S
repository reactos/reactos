/*
 * PROJECT:     FreeLoader - StartUp Module
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Support for the Multiboot v1 specification.
 * COPYRIGHT:   Copyright 1998-2002  Daniel Victor  <ilauncherdeveloper@gmail.com>
 */

#include <asm.inc>
#include <arch/pc/x86common.h>
#include <multiboot.h>

/* Multiboot v1 support
 *
 * Allows freeldr to be loaded as a "multiboot kernel" by
 * other boot loaders like GRUB.
 * This code is not referenced from anywhere. GRUB searches for
 * for the header signature and uses the header to load it.
 */

#define MB_INFO_SIZE                60   /* sizeof(multiboot_info_t) */
#define MB_INFO_FLAGS_OFFSET        0
#define MB_INFO_BOOT_DEVICE_OFFSET  12
#define MB_INFO_COMMAND_LINE_OFFSET 16
#define MB_INFO_MMAP_LEN_OFFSET     44
#define MB_INFO_MMAP_ADDR_OFFSET    48
#define MB_MMAP_SIZE                480  /* 20 * sizeof(memory_map_t) - up to 20 entries */
#define CMDLINE_SIZE                256

/*
 * We want to execute at FREELDR_BASE (to be compatible with
 * bootsector loading), but GRUB only allows loading of
 * multiboot kernels above 1MB. So we let GRUB load us
 * there and then relocate ourself to FREELDR_BASE.
 */
#define INITIAL_BASE HEX(200000)

#ifdef _USE_ML
.MBDATA SEGMENT PUBLIC 'DATA'
#endif

    /* Align to 32 bits boundary */
    .align 4

    /* Multiboot header */
MultibootHeader:
    /* magic */
    .long MULTIBOOT_HEADER_MAGIC
    /* flags */
    .long MULTIBOOT_HEADER_FLAGS
    /* checksum */
    .long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
    /* header_addr */
    .long MultibootHeader + INITIAL_BASE - FREELDR_BASE
    /* load_addr */
    .long INITIAL_BASE
    /* load_end_addr */
    .long 0
    /* bss_end_addr */
    .long 0
    /* entry_addr */
    .long MultibootEntry + INITIAL_BASE - FREELDR_BASE

#ifdef _USE_ML
.MBDATA ENDS
#endif

.code64

MultibootEntry:
    jmp MultibootEntry

/* Force 8-byte alignment */
    .align 8
gdt16:
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 00: NULL descriptor */
    .word HEX(FFFF), HEX(0000), HEX(9B00), HEX(008F) /* 08: 16-bit flat CS (!) */
    .word HEX(FFFF), HEX(0000), HEX(9300), HEX(0000) /* 10: 16-bit real mode DS */

/* GDT table pointer */
gdtptr16:
    .word HEX(17)   /* Limit */
    .long gdt16     /* Base Address */
