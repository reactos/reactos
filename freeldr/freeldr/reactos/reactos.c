/*
 *  FreeLoader
 *
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include <debug.h>
#include <arch.h>
#include <reactos.h>
#include <rtl.h>
#include <fs.h>
#include <ui.h>
#include <multiboot.h>
#include <mm.h>
#include <inifile.h>

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
      strcpy(szBuffer, szShortName);
      strcat(szBuffer, " not found.");
      UiMessageBox(szBuffer);
      return(FALSE);
    }

  /*
   * Update the status bar with the current file
   */
  strcpy(szBuffer, "Reading ");
  strcat(szBuffer, szShortName);
  UiDrawStatusText(szBuffer);

  /*
   * Load the kernel
   */
  MultiBootLoadKernel(FilePointer);

  UiDrawProgressBarCenter(nPos, 100);

  return(TRUE);
}

static BOOL
LoadSymbolFile(PCHAR szSystemRoot,
  PCHAR ModuleName,
  int nPos)
{
  CHAR SymbolFileName[1024];
  PFILE FilePointer;
  U32 Length;
  PCHAR Start;
  PCHAR Ext;
  char value[256];
  char *p;

  /* Get the path to the symbol store */
  strcpy(SymbolFileName, szSystemRoot);
  strcat(SymbolFileName, "symbols\\");

  /* Get the symbol filename from the module name */
  Start = strrchr(ModuleName, '\\');
  if (Start == NULL)
    Start = ModuleName;
  else
    Start++;

  Ext = strrchr(ModuleName, '.');
  if (Ext != NULL)
    Length = Ext - Start;
  else
    Length = strlen(Start);

  strncat(SymbolFileName, Start, Length);
  strcat(SymbolFileName, ".sym");

  FilePointer = OpenFile((PCHAR)&SymbolFileName[0]);
  if (FilePointer == NULL)
    {
      DbgPrint((DPRINT_REACTOS, "Symbol file %s not loaded.\n", SymbolFileName));
      /* This is not critical */
      return FALSE;
    }

  DbgPrint((DPRINT_REACTOS, "Symbol file %s is loaded.\n", SymbolFileName));

  /*
   * Update the status bar with the current file
   */
  strcpy(value, "Reading ");
  p = strrchr(SymbolFileName, '\\');
  if (p == NULL)
    strcat(value, SymbolFileName);
  else
    strcat(value, p + 1);
  UiDrawStatusText(value);

  /*
   * Load the symbol file
   */
  MultiBootLoadModule(FilePointer, SymbolFileName, NULL);

  UiDrawProgressBarCenter(nPos, 100);

  return (TRUE);
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
      strcpy(value, szFileName);
      strcat(value, " not found.");
      UiMessageBox(value);
      return(FALSE);
    }

  /*
   * Update the status bar with the current file
   */
  strcpy(value, "Reading ");
  p = strrchr(szFileName, '\\');
  if (p == NULL)
    strcat(value, szFileName);
  else
    strcat(value, p + 1);
  UiDrawStatusText(value);

  /*
   * Load the driver
   */
  MultiBootLoadModule(FilePointer, szFileName, NULL);

  UiDrawProgressBarCenter(nPos, 100);

  return(TRUE);
}


#if 0
static BOOL
LoadNlsFile(PCHAR szFileName, PCHAR szModuleName)
{
  PFILE FilePointer;
  char value[256];
  char *p;

  FilePointer = OpenFile(szFileName);
  if (FilePointer == NULL)
    {
      strcpy(value, szFileName);
      strcat(value, " not found.");
      UiMessageBox(value);
      return(FALSE);
    }

  /*
   * Update the status bar with the current file
   */
  strcpy(value, "Reading ");
  p = strrchr(szFileName, '\\');
  if (p == NULL)
    strcat(value, szFileName);
  else
    strcat(value, p + 1);
  UiDrawStatusText(value);

  /*
   * Load the driver
   */
  MultiBootLoadModule(FilePointer, szModuleName, NULL);

  return(TRUE);
}
#endif


static VOID
LoadBootDrivers(PCHAR szSystemRoot, int nPos)
{
  S32 rc = 0;
  HKEY hGroupKey, hServiceKey, hDriverKey;
  char ValueBuffer[512];
  char ServiceName[256];
  U32 BufferSize;
  U32 Index;
  char *GroupName;

  U32 ValueSize;
  U32 ValueType;
  U32 StartValue;
  UCHAR DriverGroup[256];
  U32 DriverGroupSize;

  UCHAR ImagePath[256];

  /* get 'service group order' key */
  rc = RegOpenKey(NULL,
		  "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\ServiceGroupOrder",
		  &hGroupKey);
  if (rc != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REACTOS, "Failed to open the 'ServiceGroupOrder key (rc %d)\n", (int)rc));
      return;
    }

  /* enumerate drivers */
  rc = RegOpenKey(NULL,
		  "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services",
		  &hServiceKey);
  if (rc != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REACTOS, "Failed to open the 'Services' key (rc %d)\n", (int)rc));
      return;
    }

  BufferSize = sizeof(ValueBuffer);
  rc = RegQueryValue(hGroupKey, "List", NULL, (PUCHAR)ValueBuffer, &BufferSize);
  DbgPrint((DPRINT_REACTOS, "RegQueryValue(): rc %d\n", (int)rc));
  if (rc != ERROR_SUCCESS)
    return;

  DbgPrint((DPRINT_REACTOS, "BufferSize: %d \n", (int)BufferSize));

  DbgPrint((DPRINT_REACTOS, "ValueBuffer: '%s' \n", ValueBuffer));

  GroupName = ValueBuffer;
  while (*GroupName)
    {
      DbgPrint((DPRINT_REACTOS, "Driver group: '%s'\n", GroupName));

      /* enumerate all drivers */
      Index = 0;
      while (TRUE)
	{
	  ValueSize = sizeof(ValueBuffer);
	  rc = RegEnumKey(hServiceKey, Index, ServiceName, &ValueSize);
	  DbgPrint((DPRINT_REACTOS, "RegEnumKey(): rc %d\n", (int)rc));
	  if (rc == ERROR_NO_MORE_ITEMS)
	    break;
	  if (rc != ERROR_SUCCESS)
	    return;
	  DbgPrint((DPRINT_REACTOS, "Service %d: '%s'\n", (int)Index, ServiceName));

	  /* open driver Key */
	  rc = RegOpenKey(hServiceKey, ServiceName, &hDriverKey);

	  ValueSize = sizeof(U32);
	  rc = RegQueryValue(hDriverKey, "Start", &ValueType, (PUCHAR)&StartValue, &ValueSize);
	  DbgPrint((DPRINT_REACTOS, "  Start: %x  \n", (int)StartValue));

	  DriverGroupSize = 256;
	  rc = RegQueryValue(hDriverKey, "Group", NULL, (PUCHAR)DriverGroup, &DriverGroupSize);
	  DbgPrint((DPRINT_REACTOS, "  Group: '%s'  \n", DriverGroup));

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
		  DbgPrint((DPRINT_REACTOS, "  ImagePath: not found\n"));
		  strcpy(ImagePath, szSystemRoot);
		  strcat(ImagePath, "system32\\drivers\\");
		  strcat(ImagePath, ServiceName);
		  strcat(ImagePath, ".sys");
		}
	      else
		{
		  DbgPrint((DPRINT_REACTOS, "  ImagePath: '%s'\n", ImagePath));
		}
	      DbgPrint((DPRINT_REACTOS, "  Loading driver: '%s'\n", ImagePath));

	      if (nPos < 100)
		nPos += 5;

	      LoadDriver(ImagePath, nPos);
        LoadSymbolFile(szSystemRoot, ImagePath, nPos);
	    }
	  else
	    {
	      DbgPrint((DPRINT_REACTOS, "  Skipping driver '%s' with Start %d and Group '%s' (Current group '%s')\n",
	          ImagePath, StartValue, DriverGroup, GroupName));
	    }
	  Index++;
	}

      GroupName = GroupName + strlen(GroupName) + 1;
    }
}

#if 0
static BOOL
LoadNlsFiles(PCHAR szSystemRoot)
{
  S32 rc = ERROR_SUCCESS;
  HKEY hKey;
  char szIdBuffer[80];
  char szNameBuffer[80];
  char szFileName[256];
  U32 BufferSize;

  /* open the codepage key */
  rc = RegOpenKey(NULL,
		  "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\CodePage",
		  &hKey);
  if (rc != ERROR_SUCCESS)
    return(FALSE);


  /* get ANSI codepage */
  BufferSize = 80;
  rc = RegQueryValue(hKey, "ACP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS)
    return(FALSE);

  BufferSize = 80;
  rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS)
    return(FALSE);


  /* load ANSI codepage table */
  strcpy(szFileName, szSystemRoot);
  strcat(szFileName, "system32\\");
  strcat(szFileName, szNameBuffer);
  if (!LoadNlsFile(szFileName, "ANSI.NLS"))
    return(FALSE);


  /* get OEM codepage */
  BufferSize = 80;
  rc = RegQueryValue(hKey, "OEMCP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS)
    return(FALSE);

  BufferSize = 80;
  rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS)
    return(FALSE);

  /* load OEM codepage table */
  strcpy(szFileName, szSystemRoot);
  strcat(szFileName, "system32\\");
  strcat(szFileName, szNameBuffer);
  if (!LoadNlsFile(szFileName, "OEM.NLS"))
    return(FALSE);


  /* open the language key */
  rc = RegOpenKey(NULL,
		  "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\Language",
		  &hKey);
  if (rc != ERROR_SUCCESS)
    return(FALSE);


  /* get the Unicode case table */
  BufferSize = 80;
  rc = RegQueryValue(hKey, "Default", NULL, (PUCHAR)szIdBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS)
    return(FALSE);

  BufferSize = 80;
  rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS)
    return(FALSE);

  /* load Unicode case table */
  strcpy(szFileName, szSystemRoot);
  strcat(szFileName, "system32\\");
  strcat(szFileName, szNameBuffer);
  if (!LoadNlsFile(szFileName, "UNICASE.NLS"))
    return(FALSE);

  return(TRUE);
}
#endif


void
LoadAndBootReactOS(PUCHAR OperatingSystemName)
{
	PFILE FilePointer;
	char  name[1024];
	char  value[1024];
	char  szKernelName[1024];
	char  szHalName[1024];
	char  szFileName[1024];
	char  szBootPath[256];
//	int		i;
//	int		nNumDriverFiles=0;
//	int		nNumFilesLoaded=0;
	char  MsgBuffer[256];
	U32 SectionId;

	char* Base;
	U32 Size;


	//
	// Open the operating system section
	// specified in the .ini file
	//
	if (!IniOpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(MsgBuffer,"Operating System section '%s' not found in freeldr.ini", OperatingSystemName);
		UiMessageBox(MsgBuffer);
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
	mb_info.mmap_length = (unsigned long)GetBiosMemoryMap((PBIOS_MEMORY_MAP_ARRAY)&multiboot_memory_map);
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
	if (!IniReadSettingByName(SectionId, "SystemPath", value, 1024))
	{
		UiMessageBox("System path not specified for selected operating system.");
		return;
	}

	/*
	 * Verify system path
	 */
	if (!DissectArcPath(value, szBootPath, &BootDrive, &BootPartition))
	{
		sprintf(MsgBuffer,"Invalid system path: '%s'", value);
		UiMessageBox(MsgBuffer);
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
	if (IniReadSettingByName(SectionId, "Options", value, 1024))
	{
		strcat(multiboot_kernel_cmdline, " ");
		strcat(multiboot_kernel_cmdline, value);
	}

	/* append a backslash */
	if ((strlen(szBootPath)==0) ||
	    szBootPath[strlen(szBootPath)] != '\\')
		strcat(szBootPath, "\\");

	DebugPrint(DPRINT_REACTOS,"SystemRoot: '%s'\n", szBootPath);

	UiDrawBackdrop();
	UiDrawStatusText("Loading...");
	UiDrawProgressBarCenter(0, 100);

	/*
	 * Try to open boot drive
	 */
	if (!OpenDiskDrive(BootDrive, BootPartition))
	{
		UiMessageBox("Failed to open boot drive.");
		return;
	}

	/*
	 * Find the kernel image name
	 * and try to load the kernel off the disk
	 */
	if(IniReadSettingByName(SectionId, "Kernel", value, 1024))
	{
		/*
		 * Set the name and
		 */
		if (value[0] == '\\')
		{
			strcpy(szKernelName, value);
		}
		else
		{
			strcpy(szKernelName, szBootPath);
			strcat(szKernelName, "SYSTEM32\\");
			strcat(szKernelName, value);
		}
	}
	else
	{
		strcpy(value, "NTOSKRNL.EXE");
		strcpy(szKernelName, szBootPath);
		strcat(szKernelName, "SYSTEM32\\");
		strcat(szKernelName, value);
	}

	if (!LoadKernel(szKernelName, 5))
		return;

	/*
	 * Find the HAL image name
	 * and try to load the kernel off the disk
	 */
	if(IniReadSettingByName(SectionId, "Hal", value, 1024))
	{
		/*
		 * Set the name and
		 */
		if (value[0] == '\\')
		{
			strcpy(szHalName, value);
		}
		else
		{
			strcpy(szHalName, szBootPath);
			strcat(szHalName, "SYSTEM32\\");
			strcat(szHalName, value);
		}
	}
	else
	{
		strcpy(value, "HAL.DLL");
		strcpy(szHalName, szBootPath);
		strcat(szHalName, "SYSTEM32\\");
		strcat(szHalName, value);
	}

	if (!LoadDriver(szHalName, 10))
		return;

	/*
	 * Find the System hive image name
	 * and try to load it off the disk
	 */
	if(IniReadSettingByName(SectionId, "SystemHive", value, 1024))
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

	DebugPrint(DPRINT_REACTOS, "SystemHive: '%s'", szFileName);

	FilePointer = OpenFile(szFileName);
	if (FilePointer == NULL)
	{
		strcat(value, " not found.");
		UiMessageBox(value);
		return;
	}

	/*
	 * Update the status bar with the current file
	 */
	strcpy(name, "Reading ");
	strcat(name, value);
	while (strlen(name) < 80)
		strcat(name, " ");
	UiDrawStatusText(name);

	/*
	 * Load the system hive
	 */
	Base = MultiBootLoadModule(FilePointer, szFileName, &Size);
	RegInitializeRegistry();
	RegImportHive(Base, Size);

	UiDrawProgressBarCenter(15, 100);
	DebugPrint(DPRINT_REACTOS, "SystemHive loaded at 0x%x size %u", (unsigned)Base, (unsigned)Size);

	/*
	 * Retrieve hardware information and create the hardware hive
	 */
	DetectHardware();
#if 0
	Base = MultiBootCreateModule(HARDWARE.HIV);
	RegExportHive("\\Registry\\Machine\\HARDWARE", Base, &Size);
	MultiBootCloseModule(Base, Size);
#endif
	UiDrawProgressBarCenter(20, 100);

	/*
	 * Initialize the 'currentControlSet' link
	 */
	RegInitCurrentControlSet(FALSE);

	/*
	 * Load NLS files
	 */
#if 0
	if (!LoadNlsFiles(szBootPath))
	{
		MessageBox("Failed to load NLS files\n");
		return;
	}
#endif

  LoadSymbolFile(szBootPath, szKernelName, 25);
  LoadSymbolFile(szBootPath, szHalName, 25);

	UiDrawProgressBarCenter(25, 100);

	/*
	 * Load boot drivers
	 */
	LoadBootDrivers(szBootPath, 25);


	/*
	 * Clear the screen and redraw the backdrop and status bar
	 */
	UiDrawBackdrop();
	UiDrawStatusText("Press any key to boot");

	/*
	 * Wait for user
	 */
	strcpy(name, "Kernel and Drivers loaded.\nPress any key to boot ");
	strcat(name, OperatingSystemName);
	strcat(name, ".");
	//MessageBox(name);

	/*
	 * Now boot the kernel
	 */
	StopFloppyMotor();
	boot_reactos();
}

/* EOF */
