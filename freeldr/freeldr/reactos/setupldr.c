/*
 *  FreeLoader
 *
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

#include <freeldr.h>
#include <debug.h>
#include <arch.h>
#include <disk.h>
#include <reactos.h>
#include <rtl.h>
#include <fs.h>
#include <multiboot.h>
#include <mm.h>

#include "registry.h"
#include "hwdetect.h"


static BOOL
LoadKernel(PCHAR szFileName)
{
  PFILE FilePointer;
  PCHAR szShortName;

  szShortName = strrchr(szFileName, '\\');
  if (szShortName == NULL)
    szShortName = szFileName;
  else
    szShortName = szShortName + 1;

  FilePointer = FsOpenFile(szFileName);
  if (FilePointer == NULL)
    {
      printf("Could not find %s\n", szShortName);
      return(FALSE);
    }

  /*
   * Update the status bar with the current file
   */
  printf("Reading %s\n", szShortName);

  /*
   * Load the kernel
   */
  MultiBootLoadKernel(FilePointer);

  return(TRUE);
}


static BOOL
LoadDriver(PCHAR szFileName)
{
  PFILE FilePointer;
  PCHAR szShortName;

  szShortName = strrchr(szFileName, '\\');
  if (szShortName == NULL)
    szShortName = szFileName;
  else
    szShortName = szShortName + 1;


  FilePointer = FsOpenFile(szFileName);
  if (FilePointer == NULL)
    {
      printf("Could not find %s\n", szFileName);
      return(FALSE);
    }

  /*
   * Update the status bar with the current file
   */
  printf("Reading %s\n", szShortName);

  /* Load the driver */
  MultiBootLoadModule(FilePointer, szFileName, NULL);

  return(TRUE);
}


VOID RunLoader(VOID)
{

  /* Setup multiboot information structure */
  mb_info.flags = MB_INFO_FLAG_MEM_SIZE | MB_INFO_FLAG_BOOT_DEVICE | MB_INFO_FLAG_COMMAND_LINE | MB_INFO_FLAG_MODULES;
  mb_info.mem_lower = GetConventionalMemorySize();
  mb_info.mem_upper = GetExtendedMemorySize();
  mb_info.boot_device = 0xffffffff;
  mb_info.cmdline = (unsigned long)multiboot_kernel_cmdline;
  mb_info.mods_count = 0;
  mb_info.mods_addr = (unsigned long)multiboot_modules;
  mb_info.mmap_length = (unsigned long)GetBiosMemoryMap((PBIOS_MEMORY_MAP)&multiboot_memory_map, 32) * sizeof(memory_map_t);
  if (mb_info.mmap_length)
    {
      mb_info.mmap_addr = (unsigned long)&multiboot_memory_map;
      mb_info.flags |= MB_INFO_FLAG_MEMORY_MAP;
      multiboot_memory_map_descriptor_size = sizeof(memory_map_t); // GetBiosMemoryMap uses a fixed value of 24
#if 0
      {
	 int i;
         printf("memory map length: %d\n", mb_info.mmap_length);
         printf("dumping memory map:\n");
         for (i=0; i<(mb_info.mmap_length / sizeof(memory_map_t)); i++)
	 {
	    printf("start: %x\t size: %x\t type %d\n", 
		   multiboot_memory_map[i].base_addr_low, 
		   multiboot_memory_map[i].length_low,
		   multiboot_memory_map[i].type);
	 }
         getch();
      }
#endif
    }
#if 0
  printf("low_mem = %d\n", mb_info.mem_lower);
  printf("high_mem = %d\n", mb_info.mem_upper);
  getch();
#endif

  /* Initialize registry */
  RegInitializeRegistry();

  /* Detect hardware */
  printf("Detecting hardware...\n\n");
  DetectHardware();

  /* set boot drive and partition */
  ((char *)(&mb_info.boot_device))[0] = (char)BootDrive;
  ((char *)(&mb_info.boot_device))[1] = (char)BootPartition;

  /* Copy ARC path into kernel command line */
  sprintf(multiboot_kernel_cmdline,
	  "multi(0)disk(0)cdrom(%u)\\reactos  /DEBUGPORT=COM1",
	  (unsigned int)BootDrive);

  /* Open boot drive */
  if (!FsOpenVolume(BootDrive, BootPartition))
    {
      printf("Failed to open boot drive.");
      return;
    }

  /* Load ntoskrnl.exe */
  if (!LoadKernel("\\reactos\\ntoskrnl.exe"))
    return;


  /* Load hal.dll */
  if (!LoadDriver("\\reactos\\hal.dll"))
    return;


  /* Export the system and hardware hives */
//  Base = MultiBootCreateModule(SYSTEM.HIV);
//  RegExportHive("\\Registry\\Machine\\SYSTEM", Base, &Size);
//  MultiBootCloseModule(Base, Size);

//  Base = MultiBootCreateModule(HARDWARE.HIV);
//  RegExportHive("\\Registry\\Machine\\HARDWARE", Base, &Size);
//  MultiBootCloseModule(Base, Size);




  /* Load NLS files */
#if 0
  if (!LoadNlsFiles(szBootPath))
    {
      MessageBox("Failed to load NLS files\n");
      return;
    }
#endif


	/*
	 * Load scsiport.sys
	 */
	if (!LoadDriver("\\reactos\\scsiport.sys"))
		return;

	/*
	 * Load atapi.sys (depends on hardware detection)
	 */
	if (!LoadDriver("\\reactos\\atapi.sys"))
		return;

	/*
	 * Load class2.sys
	 */
	if (!LoadDriver("\\reactos\\class2.sys"))
		return;

	/*
	 * Load cdrom.sys
	 */
	if (!LoadDriver("\\reactos\\cdrom.sys"))
		return;

	/*
	 * Load cdfs.sys
	 */
	if (!LoadDriver("\\reactos\\cdfs.sys"))
		return;

	/*
	 * Load floppy.sys (only in case of a floppy disk setup!)
	 */
//	if (!LoadDriver("\\reactos\\floppy.sys"))
//		return;

	/*
	 * Load disk.sys
	 */
	if (!LoadDriver("\\reactos\\disk.sys"))
		return;

	/*
	 * Load vfatfs.sys (could be loaded by the setup prog!)
	 */
	if (!LoadDriver("\\reactos\\vfatfs.sys"))
		return;

	/*
	 * Load keyboard driver
	 */
	if (!LoadDriver("\\reactos\\keyboard.sys"))
		return;

	/*
	 * Load screen driver
	 */
	if (!LoadDriver("\\reactos\\blue.sys"))
		return;

	/*
	 * Now boot the kernel
	 */
	DiskStopFloppyMotor();
	boot_reactos();


//  printf("*** System stopped ***\n");
//  for(;;);
}
