/*
 *  FreeLoader
 *
 *  Copyright (C) 1999, 2000, 2001  Brian Palmer  <brianp@sginet.com>
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

#include "../freeldr.h"
#include "../asmcode.h"
#include "reactos.h"
#include "../stdlib.h"
#include "../fs.h"
#include "../tui.h"
#include "../multiboot.h"
#include "../arcname.h"
#include "../memory.h"
#include "../parseini.h"

#include "registry.h"
#include "hwdetect.h"


#define NDEBUG


static BOOL
LoadKernel(PCHAR szFileName, int nPos)
{
  PFILE FilePointer;
  PCHAR szShortName;
  char szBuffer[256];

  szShortName = strrchr(szFileName, '\\');
  if (szShortName == NULL)
    szShortName = szFileName;
  else
    szShortName = szShortName + 1;

  FilePointer = OpenFile(szFileName);
  if (FilePointer == NULL)
    {
      strcat(szBuffer, szShortName);
      strcat(szBuffer, " not found.");
      MessageBox(szBuffer);
      return(FALSE);
    }

  /*
   * Update the status bar with the current file
   */
  strcpy(szBuffer, " Reading ");
  strcat(szBuffer, szShortName);
  DrawStatusText(szBuffer);

  /*
   * Load the kernel
   */
  MultiBootLoadKernel(FilePointer);

  DrawProgressBar(nPos);

  return(TRUE);
}


static BOOL
LoadDriver(PCHAR szFileName, int nPos)
{
  PFILE FilePointer;
  char value[256];
  char *p;

  FilePointer = OpenFile(szFileName);
  if (FilePointer == NULL)
    {
      strcat(value, szFileName);
      strcat(value, " not found.");
      MessageBox(value);
      return(FALSE);
    }

  /*
   * Update the status bar with the current file
   */
  strcpy(value, " Reading ");
  p = strrchr(szFileName, '\\');
  if (p == NULL)
    strcat(value, szFileName);
  else
    strcat(value, p + 1);
  while (strlen(value) < 80)
    strcat(value, " ");
  DrawStatusText(value);

  /*
   * Load the driver
   */
  MultiBootLoadModule(FilePointer, szFileName, NULL);

  DrawProgressBar(nPos);

  return(TRUE);
}


static VOID
LoadBootDrivers(PCHAR szSystemRoot, int nPos)
{
  LONG rc = 0;
  HKEY hGroupKey, hServiceKey, hDriverKey;
  char ValueBuffer[256];
  char ServiceName[256];
  ULONG BufferSize;
  ULONG Index;
  char *s, *p;
  char GroupName[256];
  ULONG len;
  BOOL done;

  ULONG ValueSize;
  ULONG ValueType;
  ULONG StartValue;
  UCHAR DriverGroup[256];
  ULONG DriverGroupSize;

  UCHAR ImagePath[256];

  /* get 'service group order' key */
  rc = RegOpenKey(NULL,
		  "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\ServiceGroupOrder",
		  &hGroupKey);
//  printf("RegOpenKey(): rc %d\n", (int)rc);
  if (rc != ERROR_SUCCESS)
    return;

  /* enumerate drivers */
  rc = RegOpenKey(NULL,
		  "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services",
		  &hServiceKey);
//  printf("RegOpenKey(): rc %d\n", (int)rc);
  if (rc != ERROR_SUCCESS)
    return;

//  printf("hKey: %x\n", (int)hKey);

  BufferSize = 256;
  rc = RegQueryValue(hGroupKey, "List", NULL, (PUCHAR)ValueBuffer, &BufferSize);
//  printf("RegQueryValue(): rc %d\n", (int)rc);
  if (rc != ERROR_SUCCESS)
    return;


//  printf("BufferSize: %d \n", (int)BufferSize);

//  printf("ValueBuffer: '%s' \n", ValueBuffer);

  done = FALSE;
  s = ValueBuffer;
  do
    {
      p = strchr(s, ';');
      if (p != NULL)
	{
	  len = p - s;
	  memcpy(GroupName, s, len);
	  GroupName[len] = 0;
	  s = p + 1;
	}
      else
	{
	  strcpy(GroupName, s);
	  done = TRUE;
	}
//      printf("Driver group: '%s'\n", GroupName);

      /* enumerate all drivers */
      Index = 0;
      while (TRUE)
	{
	  ValueSize = 256;
	  rc = RegEnumKey(hServiceKey, Index, ServiceName, &ValueSize);
//	  printf("RegEnumKey(): rc %d\n", (int)rc);
	  if (rc == ERROR_NO_MORE_ITEMS)
	    break;
	  if (rc != ERROR_SUCCESS)
	    return;
//	  printf("Service %d: '%s'\n", (int)Index, ServiceName);

	  /* open driver Key */
	  rc = RegOpenKey(hServiceKey, ServiceName, &hDriverKey);

	  ValueSize = sizeof(ULONG);
	  rc = RegQueryValue(hDriverKey, "Start", &ValueType, (PUCHAR)&StartValue, &ValueSize);
//	  printf("  Start: %x  \n", (int)StartValue);

	  DriverGroupSize = 256;
	  rc = RegQueryValue(hDriverKey, "Group", NULL, (PUCHAR)DriverGroup, &DriverGroupSize);
//	  printf("  Group: %s  \n", DriverGroup);

	  if ((StartValue == 0) && (stricmp(DriverGroup, GroupName) == 0))
	    {
	      ValueSize = 256;
	      rc = RegQueryValue(hDriverKey,
				 "ImagePathName",
				 NULL,
				 (PUCHAR)ImagePath,
				 &ValueSize);
	      if (rc != ERROR_SUCCESS)
		{
//		  printf("  ImagePath: not found\n");
		  strcpy(ImagePath, szSystemRoot);
		  strcat(ImagePath, "system32\\drivers\\");
		  strcat(ImagePath, ServiceName);
		  strcat(ImagePath, ".sys");
		}
	      else
		{
//		  printf("  ImagePath: '%s'\n", ImagePath);
		}
//	      printf("  Loading driver: '%s'\n", ImagePath);

	      LoadDriver(ImagePath, nPos);

	      if (nPos < 100)
		nPos += 5;
	    }
	  Index++;
	}
    }
  while(done == FALSE);

}

void LoadAndBootReactOS(PUCHAR OperatingSystemName)
{
	PFILE		FilePointer;
	char		name[1024];
	char		value[1024];
	char		szFileName[1024];
	char		szBootPath[256];
//	int		i;
//	int		nNumDriverFiles=0;
//	int		nNumFilesLoaded=0;
	char		MsgBuffer[256];
	ULONG		SectionId;

	char* Base;
	ULONG Size;

	//
	// Open the operating system section
	// specified in the .ini file
	//
	if (!OpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(MsgBuffer,"Operating System section '%s' not found in freeldr.ini", OperatingSystemName);
		MessageBox(MsgBuffer);
		return;
	}

	/*
	 * Setup multiboot information structure
	 */
	mb_info.flags = MB_INFO_FLAG_MEM_SIZE | MB_INFO_FLAG_BOOT_DEVICE | MB_INFO_FLAG_COMMAND_LINE | MB_INFO_FLAG_MODULES;
	mb_info.mem_lower = GetConventionalMemorySize();
	mb_info.mem_upper = GetExtendedMemorySize();
	mb_info.boot_device = 0xffffffff;
	mb_info.cmdline = (unsigned long)multiboot_kernel_cmdline;
	mb_info.mods_count = 0;
	mb_info.mods_addr = (unsigned long)multiboot_modules;
	mb_info.mmap_length = GetBiosMemoryMap(&multiboot_memory_map);
	if (mb_info.mmap_length)
	{
		mb_info.mmap_addr = (unsigned long)&multiboot_memory_map;
		mb_info.flags |= MB_INFO_FLAG_MEMORY_MAP;
		//printf("memory map length: %d\n", mb_info.mmap_length);
		//printf("dumping memory map:\n");
		//for (i=0; i<(mb_info.mmap_length / 4); i++)
		//{
		//	printf("0x%x\n", ((unsigned long *)&multiboot_memory_map)[i]);
		//}
		//getch();
	}
	//printf("low_mem = %d\n", mb_info.mem_lower);
	//printf("high_mem = %d\n", mb_info.mem_upper);
	//getch();

	/*
	 * Make sure the system path is set in the .ini file
	 */
	if (!ReadSectionSettingByName(SectionId, "SystemPath", value, 1024))
	{
		MessageBox("System path not specified for selected operating system.");
		return;
	}

	/*
	 * Verify system path
	 */
	if (!DissectArcPath(value, szBootPath, &BootDrive, &BootPartition))
	{
		sprintf(MsgBuffer,"Invalid system path: '%s'", value);
		MessageBox(MsgBuffer);
		return;
	}

	/* set boot drive and partition */
	((char *)(&mb_info.boot_device))[0] = (char)BootDrive;
	((char *)(&mb_info.boot_device))[1] = (char)BootPartition;

	/* copy ARC path into kernel command line */
	strcpy(multiboot_kernel_cmdline, value);

	/*
	 * Read the optional kernel parameters (if any)
	 */
	if (ReadSectionSettingByName(SectionId, "Options", value, 1024))
	{
		strcat(multiboot_kernel_cmdline, " ");
		strcat(multiboot_kernel_cmdline, value);
	}

	/* append a backslash */
	if ((strlen(szBootPath)==0) ||
	    szBootPath[strlen(szBootPath)] != '\\')
		strcat(szBootPath, "\\");

#ifndef NDEBUG
	sprintf(MsgBuffer,"SystemRoot: '%s'", szBootPath);
	MessageBox(MsgBuffer);
#endif

	DrawBackdrop();

	DrawStatusText(" Loading...");
	DrawProgressBar(0);

	/*
	 * Try to open boot drive
	 */
	if (!OpenDiskDrive(BootDrive, BootPartition))
	{
		MessageBox("Failed to open boot drive.");
		return;
	}

	/*
	 * Find the kernel image name
	 * and try to load the kernel off the disk
	 */
	if(ReadSectionSettingByName(SectionId, "Kernel", value, 1024))
	{
		/*
		 * Set the name and
		 */
		if (value[0] == '\\')
		{
			strcpy(szFileName, value);
		}
		else
		{
			strcpy(szFileName, szBootPath);
			strcat(szFileName, "SYSTEM32\\");
			strcat(szFileName, value);
		}
	}
	else
	{
		strcpy(value, "NTOSKRNL.EXE");
		strcpy(szFileName, szBootPath);
		strcat(szFileName, "SYSTEM32\\");
		strcat(szFileName, value);
	}

	if (!LoadKernel(szFileName, 5))
		return;

	/*
	 * Find the HAL image name
	 * and try to load the kernel off the disk
	 */
	if(ReadSectionSettingByName(SectionId, "Hal", value, 1024))
	{
		/*
		 * Set the name and
		 */
		if (value[0] == '\\')
		{
			strcpy(szFileName, value);
		}
		else
		{
			strcpy(szFileName, szBootPath);
			strcat(szFileName, "SYSTEM32\\");
			strcat(szFileName, value);
		}
	}
	else
	{
		strcpy(value, "HAL.DLL");
		strcpy(szFileName, szBootPath);
		strcat(szFileName, "SYSTEM32\\");
		strcat(szFileName, value);
	}

	if (!LoadDriver(szFileName, 10))
		return;

	/*
	 * Find the System hive image name
	 * and try to load it off the disk
	 */
	if(ReadSectionSettingByName(SectionId, "SystemHive", value, 1024))
	{
		/*
		 * Set the name and
		 */
		if (value[0] == '\\')
		{
			strcpy(szFileName, value);
		}
		else
		{
			strcpy(szFileName, szBootPath);
			strcat(szFileName, "SYSTEM32\\CONFIG\\");
			strcat(szFileName, value);
		}
	}
	else
	{
		strcpy(value, "SYSTEM.HIV");
		strcpy(szFileName, szBootPath);
		strcat(szFileName, "SYSTEM32\\CONFIG\\");
		strcat(szFileName, value);
	}

#ifndef NDEBUG
	sprintf(MsgBuffer,"SystemHive: '%s'", szFileName);
	MessageBox(MsgBuffer);
#endif

	FilePointer = OpenFile(szFileName);
	if (FilePointer == NULL)
	{
		strcat(value, " not found.");
		MessageBox(value);
		return;
	}

	/*
	 * Update the status bar with the current file
	 */
	strcpy(name, " Reading ");
	strcat(name, value);
	while (strlen(name) < 80)
		strcat(name, " ");
	DrawStatusText(name);

	/*
	 * Load the system hive
	 */
	Base = MultiBootLoadModule(FilePointer, szFileName, &Size);
	RegInitializeRegistry();
	RegImportHive(Base, Size);

	DrawProgressBar(15);

#ifndef NDEBUG
	sprintf(MsgBuffer,"SystemHive loaded at 0x%x size %u", (unsigned)Base, (unsigned)Size);
	MessageBox(MsgBuffer);
#endif

	/*
	 * retrieve hardware information and create the hardware hive
	 */
	DetectHardware();
//	Base = MultiBootCreateModule(HARDWARE.HIV);
//	RegExportHardwareHive(Base, &Size);
//	MultiBootCloseModule(Base, Size);

	DrawProgressBar(20);

	/*
	 * load NLS files
	 */
//	LoadNlsFiles(szBootPath);

	DrawProgressBar(25);

	/*
	 * load boot drivers
	 */
	LoadBootDrivers(szBootPath, 30);


	/*
	 * Clear the screen and redraw the backdrop and status bar
	 */
	DrawBackdrop();
	DrawStatusText(" Press any key to boot");

	/*
	 * Wait for user
	 */
	strcpy(name, "Kernel and Drivers loaded.\nPress any key to boot ");
	strcat(name, OperatingSystemName);
	strcat(name, ".");
	//MessageBox(name);

	RestoreScreen(ScreenBuffer);

	/*
	 * Now boot the kernel
	 */
	stop_floppy();
	boot_reactos();
}

