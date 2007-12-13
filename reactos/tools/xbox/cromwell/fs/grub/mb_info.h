/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2003  Free Software Foundation, Inc.
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *  The structure type "mod_list" is used by the "multiboot_info" structure.
 */

struct mod_list
{
  /* the memory used goes from bytes 'mod_start' to 'mod_end-1' inclusive */
  unsigned long mod_start;
  unsigned long mod_end;
  
  /* Module command line */
  unsigned long cmdline;
  
  /* padding to take it to 16 bytes (must be zero) */
  unsigned long pad;
};


/*
 *  INT-15, AX=E820 style "AddressRangeDescriptor"
 *  ...with a "size" parameter on the front which is the structure size - 4,
 *  pointing to the next one, up until the full buffer length of the memory
 *  map has been reached.
 */

struct AddrRangeDesc
{
  unsigned long size;
  unsigned long long BaseAddr;
  unsigned long long Length;
  unsigned long Type;
  
  /* unspecified optional padding... */
} __attribute__ ((packed));

/* usable memory "Type", all others are reserved.  */
#define MB_ARD_MEMORY		1


/* Drive Info structure.  */
struct drive_info
{
  /* The size of this structure.  */
  unsigned long size;

  /* The BIOS drive number.  */
  unsigned char drive_number;

  /* The access mode (see below).  */
  unsigned char drive_mode;

  /* The BIOS geometry.  */
  unsigned short drive_cylinders;
  unsigned char drive_heads;
  unsigned char drive_sectors;

  /* The array of I/O ports used for the drive.  */
  unsigned short drive_ports[0];
};

/* Drive Mode.  */
#define MB_DI_CHS_MODE		0
#define MB_DI_LBA_MODE		1


/* APM BIOS info.  */
struct apm_info
{
  unsigned short version;
  unsigned short cseg;
  unsigned long offset;
  unsigned short cseg_16;
  unsigned short dseg_16;
  unsigned short cseg_len;
  unsigned short cseg_16_len;
  unsigned short dseg_16_len;
};


/*
 *  MultiBoot Info description
 *
 *  This is the struct passed to the boot image.  This is done by placing
 *  its address in the EAX register.
 */

struct multiboot_info
{
  /* MultiBoot info version number */
  unsigned long flags;
  
  /* Available memory from BIOS */
  unsigned long mem_lower;
  unsigned long mem_upper;
  
  /* "root" partition */
  unsigned long boot_device;
  
  /* Kernel command line */
  unsigned long cmdline;
  
  /* Boot-Module list */
  unsigned long mods_count;
  unsigned long mods_addr;
  
  union
  {
    struct
    {
      /* (a.out) Kernel symbol table info */
      unsigned long tabsize;
      unsigned long strsize;
      unsigned long addr;
      unsigned long pad;
    }
    a;
    
    struct
    {
      /* (ELF) Kernel section header table */
      unsigned long num;
      unsigned long size;
      unsigned long addr;
      unsigned long shndx;
    }
    e;
  }
  syms;
  
  /* Memory Mapping buffer */
  unsigned long mmap_length;
  unsigned long mmap_addr;
  
  /* Drive Info buffer */
  unsigned long drives_length;
  unsigned long drives_addr;
  
  /* ROM configuration table */
  unsigned long config_table;
  
  /* Boot Loader Name */
  unsigned long boot_loader_name;

  /* APM table */
  unsigned long apm_table;

  /* Video */
  unsigned long vbe_control_info;
  unsigned long vbe_mode_info;
  unsigned short vbe_mode;
  unsigned short vbe_interface_seg;
  unsigned short vbe_interface_off;
  unsigned short vbe_interface_len;
};

/*
 *  Flags to be set in the 'flags' parameter above
 */

/* is there basic lower/upper memory information? */
#define MB_INFO_MEMORY			0x00000001
/* is there a boot device set? */
#define MB_INFO_BOOTDEV			0x00000002
/* is the command-line defined? */
#define MB_INFO_CMDLINE			0x00000004
/* are there modules to do something with? */
#define MB_INFO_MODS			0x00000008

/* These next two are mutually exclusive */

/* is there a symbol table loaded? */
#define MB_INFO_AOUT_SYMS		0x00000010
/* is there an ELF section header table? */
#define MB_INFO_ELF_SHDR		0x00000020

/* is there a full memory map? */
#define MB_INFO_MEM_MAP			0x00000040

/* Is there drive info?  */
#define MB_INFO_DRIVE_INFO		0x00000080

/* Is there a config table?  */
#define MB_INFO_CONFIG_TABLE		0x00000100

/* Is there a boot loader name?  */
#define MB_INFO_BOOT_LOADER_NAME	0x00000200

/* Is there a APM table?  */
#define MB_INFO_APM_TABLE		0x00000400

/* Is there video information?  */
#define MB_INFO_VIDEO_INFO		0x00000800

/*
 *  The following value must be present in the EAX register.
 */

#define MULTIBOOT_VALID			0x2BADB002
