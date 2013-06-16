/* multiboot.h - the header for Multiboot */
/* Copyright (C) 1999  Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __ASM__
#pragma once
#endif

/* Macros. */

/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC          HEX(1BADB002)

/* The flags for the Multiboot header. */
#define MULTIBOOT_HEADER_FLAGS          HEX(00010003)

/* The magic number passed by a Multiboot-compliant boot loader. */
#define MULTIBOOT_BOOTLOADER_MAGIC      HEX(2BADB002)

/* The size of our stack (16KB). */
#define STACK_SIZE                      0x4000

/* C symbol format. HAVE_ASM_USCORE is defined by configure. */
#ifdef HAVE_ASM_USCORE
# define EXT_C(sym)                     _ ## sym
#else
# define EXT_C(sym)                     sym
#endif

#define MB_INFO_FLAG_MEM_SIZE            HEX(00000001)
#define MB_INFO_FLAG_BOOT_DEVICE        HEX(00000002)
#define MB_INFO_FLAG_COMMAND_LINE        HEX(00000004)
#define MB_INFO_FLAG_MODULES            HEX(00000008)
#define MB_INFO_FLAG_AOUT_SYMS            HEX(00000010)
#define MB_INFO_FLAG_ELF_SYMS            HEX(00000020)
#define MB_INFO_FLAG_MEMORY_MAP            HEX(00000040)
#define MB_INFO_FLAG_DRIVES                HEX(00000080)
#define MB_INFO_FLAG_CONFIG_TABLE        HEX(00000100)
#define MB_INFO_FLAG_BOOT_LOADER_NAME    HEX(00000200)
#define MB_INFO_FLAG_APM_TABLE            HEX(00000400)
#define MB_INFO_FLAG_GRAPHICS_TABLE        HEX(00000800)

#ifndef __ASM__
/* Do not include here in boot.S. */

/* Types. */

/* The Multiboot header. */
typedef struct multiboot_header
{
  unsigned long magic;
  unsigned long flags;
  unsigned long checksum;
  unsigned long header_addr;
  unsigned long load_addr;
  unsigned long load_end_addr;
  unsigned long bss_end_addr;
  unsigned long entry_addr;
} multiboot_header_t;

/* The symbol table for a.out. */
typedef struct aout_symbol_table
{
  unsigned long tabsize;
  unsigned long strsize;
  unsigned long addr;
  unsigned long reserved;
} aout_symbol_table_t;

/* The section header table for ELF. */
typedef struct elf_section_header_table
{
  unsigned long num;
  unsigned long size;
  unsigned long addr;
  unsigned long shndx;
} elf_section_header_table_t;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size. */
typedef struct memory_map
{
  //unsigned long size;
  unsigned long base_addr_low;
  unsigned long base_addr_high;
  unsigned long length_low;
  unsigned long length_high;
  unsigned long type;
  unsigned long reserved;
} memory_map_t;

#endif /* ! __ASM__ */
