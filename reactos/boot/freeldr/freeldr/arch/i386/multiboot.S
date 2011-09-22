/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <asm.inc>
#include <arch/pc/x86common.h>
#include <multiboot.h>

/* Multiboot support
 *
 * Allows freeldr to be loaded as a "multiboot kernel" by
 * other boot loaders like GRUB.
 * This code is not referenced from anywhere. GRUB searches for
 * the header signature and uses the header to load it.
 */

#define MB_INFO_FLAGS_OFFSET        0
#define MB_INFO_BOOT_DEVICE_OFFSET  12
#define MB_INFO_COMMAND_LINE_OFFSET 16
#define CMDLINE_SIZE                256

/*
 * We want to execute at FREELDR_BASE (to be compatible with
 * bootsector loading), but GRUB only allows loading of
 * multiboot kernels above 1MB. So we let GRUB load us
 * there and then relocate ourself to FREELDR_BASE.
 */
#define INITIAL_BASE HEX(200000)

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
    .long INITIAL_BASE + MultibootHeader - FREELDR_BASE
    /* load_addr */
    .long INITIAL_BASE
    /* load_end_addr */
    .long 0
    /* bss_end_addr */
    .long 0
    /* entry_addr */
    .long INITIAL_BASE + MultibootEntry - FREELDR_BASE

MultibootEntry:
    cld

    /* Save command line */
    test dword ptr [ebx + MB_INFO_FLAGS_OFFSET], MB_INFO_FLAG_COMMAND_LINE
    jz mb2
    mov esi, [ebx + MB_INFO_COMMAND_LINE_OFFSET]
    mov edi, offset cmdline + INITIAL_BASE - FREELDR_BASE
    mov ecx, CMDLINE_SIZE - 1
mb1:
    lodsb
    stosb
    test al, al
    jz mb2
    dec ecx
    jnz mb1

mb2:
    /* See if the boot device was passed in */
    test dword ptr [ebx + MB_INFO_FLAGS_OFFSET], MB_INFO_FLAG_BOOT_DEVICE

    /* If no boot device known, assume first partition of first harddisk */
    mov dx, HEX(0180)
    jz mb3

    /* Load boot drive into DL, boot partition into DH */
    mov edx, [ebx + MB_INFO_BOOT_DEVICE_OFFSET]
    bswap edx
    inc dh

mb3:
    /* Relocate itself to lower address */
    mov esi, INITIAL_BASE
    mov edi, FREELDR_BASE
    mov ecx, offset __bss_start__ - FREELDR_BASE
    shr ecx, 2
    rep movsd

    /* Load segment registers for real-address mode */
    lgdt gdtptr
    mov ax, HEX(10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    /* Jump to relocated code */
    ljmp HEX(08), mb4

.code16
mb4:
    /* Disable protected mode */
    mov eax, cr0
    and eax, CR0_PE_CLR
    mov cr0, eax

    /* Jump to real entry point */
    ljmp16 0, FREELDR_BASE

    /* Force 8-byte alignment */
    .align 8
gdt:
    /* 16-bit flat CS (!) */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9B00)
    .word HEX(008F)

    /* 16-bit real mode DS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9300)
    .word HEX(0000)

    /* GDT pointer */
gdtptr:
    .word HEX(17)       /* Limit */
    .long gdt - 8       /* Base Address */

PUBLIC cmdline
cmdline:
    .space CMDLINE_SIZE

END
