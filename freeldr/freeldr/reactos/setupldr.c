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
#include <ui.h>
#include <inffile.h>

#include "registry.h"


//#define USE_UI


static BOOL
LoadKernel(PCHAR szSourcePath, PCHAR szFileName)
{
  CHAR szFullName[256];
#ifdef USE_UI
  CHAR szBuffer[80];
#endif
  PFILE FilePointer;
  PCHAR szShortName;

  if (szSourcePath[0] != '\\')
    {
      strcpy(szFullName, "\\");
      strcat(szFullName, szSourcePath);
    }
  else
    {
      strcpy(szFullName, szSourcePath);
    }

  if (szFullName[strlen(szFullName)] != '\\')
    {
      strcat(szFullName, "\\");
    }

  if (szFileName[0] != '\\')
    {
      strcat(szFullName, szFileName);
    }
  else
    {
      strcat(szFullName, szFileName + 1);
    }

  szShortName = strrchr(szFileName, '\\');
  if (szShortName == NULL)
    szShortName = szFileName;
  else
    szShortName = szShortName + 1;

  FilePointer = FsOpenFile(szFullName);
  if (FilePointer == NULL)
    {
      printf("Could not find %s\n", szShortName);
      return(FALSE);
    }

  /*
   * Update the status bar with the current file
   */
#ifdef USE_UI
  sprintf(szBuffer, "Reading %s", szShortName);
  UiDrawStatusText(szBuffer);
#else
  printf("Reading %s\n", szShortName);
#endif

  /*
   * Load the kernel
   */
  MultiBootLoadKernel(FilePointer);

  return(TRUE);
}


static BOOL
LoadDriver(PCHAR szSourcePath, PCHAR szFileName)
{
  CHAR szFullName[256];
#ifdef USE_UI
  CHAR szBuffer[80];
#endif
  PFILE FilePointer;
  PCHAR szShortName;

  if (szSourcePath[0] != '\\')
    {
      strcpy(szFullName, "\\");
      strcat(szFullName, szSourcePath);
    }
  else
    {
      strcpy(szFullName, szSourcePath);
    }

  if (szFullName[strlen(szFullName)] != '\\')
    {
      strcat(szFullName, "\\");
    }

  if (szFileName[0] != '\\')
    {
      strcat(szFullName, szFileName);
    }
  else
    {
      strcat(szFullName, szFileName + 1);
    }

  szShortName = strrchr(szFileName, '\\');
  if (szShortName == NULL)
    szShortName = szFileName;
  else
    szShortName = szShortName + 1;


  FilePointer = FsOpenFile(szFullName);
  if (FilePointer == NULL)
    {
      printf("Could not find %s\n", szFileName);
      return(FALSE);
    }

  /*
   * Update the status bar with the current file
   */
#ifdef USE_UI
  sprintf(szBuffer, "Reading %s", szShortName);
  UiDrawStatusText(szBuffer);
#else
  printf("Reading %s\n", szShortName);
#endif

  /* Load the driver */
  MultiBootLoadModule(FilePointer, szFileName, NULL);

  return(TRUE);
}


static BOOL
LoadNlsFile(PCHAR szSourcePath, PCHAR szFileName, PCHAR szModuleName)
{
  CHAR szFullName[256];
#ifdef USE_UI
  CHAR szBuffer[80];
#endif
  PFILE FilePointer;
  PCHAR szShortName;

  if (szSourcePath[0] != '\\')
    {
      strcpy(szFullName, "\\");
      strcat(szFullName, szSourcePath);
    }
  else
    {
      strcpy(szFullName, szSourcePath);
    }

  if (szFullName[strlen(szFullName)] != '\\')
    {
      strcat(szFullName, "\\");
    }

  if (szFileName[0] != '\\')
    {
      strcat(szFullName, szFileName);
    }
  else
    {
      strcat(szFullName, szFileName + 1);
    }

  szShortName = strrchr(szFileName, '\\');
  if (szShortName == NULL)
    szShortName = szFileName;
  else
    szShortName = szShortName + 1;


  FilePointer = FsOpenFile(szFullName);
  if (FilePointer == NULL)
    {
      printf("Could not find %s\n", szFileName);
      return(FALSE);
    }

  /*
   * Update the status bar with the current file
   */
#ifdef USE_UI
  sprintf(szBuffer, "Reading %s", szShortName);
  UiDrawStatusText(szBuffer);
#else
  printf("Reading %s\n", szShortName);
#endif

  /* Load the driver */
  MultiBootLoadModule(FilePointer, szModuleName, NULL);

  return(TRUE);
}


VOID RunLoader(VOID)
{
  PVOID Base;
  U32 Size;
  char *SourcePath;
  char *LoadOptions;

  HINF InfHandle;
  U32 ErrorLine;
  INFCONTEXT InfContext;

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

#ifdef USE_UI
  UiInitialize();
  UiDrawStatusText("");
#endif

  /* Initialize registry */
  RegInitializeRegistry();

  /* Detect hardware */
#ifdef USE_UI
  UiDrawStatusText("Detecting hardware...");
#else
  printf("Detecting hardware...\n\n");
#endif
  DetectHardware();
#ifdef USE_UI
  UiDrawStatusText("");
#endif

  /* set boot drive and partition */
  ((char *)(&mb_info.boot_device))[0] = (char)BootDrive;
  ((char *)(&mb_info.boot_device))[1] = (char)BootPartition;


  /* Open boot drive */
  if (!FsOpenVolume(BootDrive, BootPartition))
    {
#ifdef USE_UI
      UiMessageBox("Failed to open boot drive.");
#else
      printf("Failed to open boot drive.");
#endif
      return;
    }

  /* Open 'txtsetup.sif' */
  if (!InfOpenFile (&InfHandle,
		    (BootDrive < 0x80) ? "\\txtsetup.sif" : "\\reactos\\txtsetup.sif",
		    &ErrorLine))
    {
      printf("Failed to open 'txtsetup.sif'\n");
      return;
    }

  /* Get load options */
  if (!InfFindFirstLine (InfHandle,
			 "SetupData",
			 "OsLoadOptions",
			 &InfContext))
    {
      printf("Failed to find 'SetupData/OsLoadOptions'\n");
      return;
    }

  if (!InfGetDataField (&InfContext,
			1,
			&LoadOptions))
    {
      printf("Failed to get load options\n");
      return;
    }
#if 0
  printf("LoadOptions: '%s'\n", LoadOptions);
#endif

  if (BootDrive < 0x80)
    {
      /* Boot from floppy disk */
      SourcePath = "\\";
    }
  else
    {
      /* Boot from cdrom */
      SourcePath = "\\reactos";
    }

  /* Set kernel command line */
  sprintf(multiboot_kernel_cmdline,
	  "multi(0)disk(0)%s(%u)%s  %s",
	  (BootDrive < 0x80) ? "fdisk" : "cdrom",
	  (unsigned int)BootDrive,
	  SourcePath,
	  LoadOptions);

  /* Load ntoskrnl.exe */
  if (!LoadKernel(SourcePath, "ntoskrnl.exe"))
    return;


  /* Load hal.dll */
  if (!LoadDriver(SourcePath, "hal.dll"))
    return;


  /* Export the hardware hive */
  Base = MultiBootCreateModule ("HARDWARE");
  RegExportBinaryHive ("\\Registry\\Machine\\HARDWARE", Base, &Size);
  MultiBootCloseModule (Base, Size);

#if 0
  printf("Base: %x\n", Base);
  printf("Size: %u\n", Size);
  printf("*** System stopped ***\n");
for(;;);
#endif

  /* Insert boot disk 2 */
  if (BootDrive < 0x80)
    {
#ifdef USE_UI
      UiMessageBox("Please insert \"ReactOS Boot Disk 2\" and press ENTER");
#else
      printf("\n\n Please insert \"ReactOS Boot Disk 2\" and press ENTER\n");
      getch();
#endif

      /* Open boot drive */
      if (!FsOpenVolume(BootDrive, BootPartition))
	{
#ifdef USE_UI
	  UiMessageBox("Failed to open boot drive.");
#else
	  printf("Failed to open boot drive.");
#endif
	  return;
	}

      /* FIXME: check volume label or disk marker file */
    }


  /* Load ANSI codepage file */
  if (!LoadNlsFile(SourcePath, "c_1252.nls", "ansi.nls"))
    {
#ifdef USE_UI
      UiMessageBox("Failed to load the ANSI codepage file.");
#else
      printf("Failed to load the ANSI codepage file.");
#endif
      return;
    }

  /* Load OEM codepage file */
  if (!LoadNlsFile(SourcePath, "c_437.nls", "oem.nls"))
    {
#ifdef USE_UI
      UiMessageBox("Failed to load the OEM codepage file.");
#else
      printf("Failed to load the OEM codepage file.");
#endif
      return;
    }

  /* Load Unicode casemap file */
  if (!LoadNlsFile(SourcePath, "l_intl.nls", "casemap.nls"))
    {
#ifdef USE_UI
      UiMessageBox("Failed to load the Unicode casemap file.");
#else
      printf("Failed to load the Unicode casemap file.");
#endif
      return;
    }


  /* Load drivers */
  if (BootDrive < 0x80)
    {
      /*
       * Load floppy.sys
       */
      if (!LoadDriver(SourcePath, "floppy.sys"))
	return;

      /*
       * Load vfatfs.sys (could be loaded by the setup prog!)
       */
      if (!LoadDriver(SourcePath, "vfatfs.sys"))
	return;
    }
  else
    {
	/*
	 * Load scsiport.sys
	 */
	if (!LoadDriver(SourcePath, "scsiport.sys"))
		return;

	/*
	 * Load atapi.sys (depends on hardware detection)
	 */
	if (!LoadDriver(SourcePath, "atapi.sys"))
		return;

	/*
	 * Load class2.sys
	 */
	if (!LoadDriver(SourcePath, "class2.sys"))
		return;

	/*
	 * Load cdrom.sys
	 */
	if (!LoadDriver(SourcePath, "cdrom.sys"))
		return;

	/*
	 * Load cdfs.sys
	 */
	if (!LoadDriver(SourcePath, "cdfs.sys"))
		return;

	/*
	 * Load disk.sys
	 */
	if (!LoadDriver(SourcePath, "disk.sys"))
		return;

	/*
	 * Load vfatfs.sys (could be loaded by the setup prog!)
	 */
	if (!LoadDriver(SourcePath, "vfatfs.sys"))
		return;
    }


  /*
   * Load keyboard driver
   */
  if (!LoadDriver(SourcePath, "keyboard.sys"))
    return;

  /*
   * Load screen driver
   */
  if (!LoadDriver(SourcePath, "blue.sys"))
    return;

#ifdef USE_UI
  UiUnInitialize("Booting ReactOS...");
#endif

  /*
   * Now boot the kernel
   */
  DiskStopFloppyMotor();
  boot_reactos();
}

/* EOF */
