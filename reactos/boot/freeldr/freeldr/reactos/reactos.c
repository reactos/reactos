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
#include <reactos.h>
#include <rtl.h>
#include <disk.h>
#include <fs.h>
#include <ui.h>
#include <multiboot.h>
#include <mm.h>
#include <machine.h>
#include <inifile.h>

#include <reactos/rossym.h>

#include "registry.h"


#define NDEBUG

#define IsRecognizedPartition(P)  \
    ((P) == PARTITION_FAT_12       || \
     (P) == PARTITION_FAT_16       || \
     (P) == PARTITION_HUGE         || \
     (P) == PARTITION_IFS          || \
     (P) == PARTITION_EXT2         || \
     (P) == PARTITION_FAT32        || \
     (P) == PARTITION_FAT32_XINT13 || \
     (P) == PARTITION_XINT13)

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

  FilePointer = FsOpenFile(szFileName);
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

  UiDrawProgressBarCenter(nPos, 100, "Loading ReactOS...");

  return(TRUE);
}

static PVOID
FreeldrAllocMem(ULONG_PTR Size)
{
  return MmAllocateMemory((U32) Size);
}

static VOID
FreeldrFreeMem(PVOID Area)
{
  MmFreeMemory(Area);
}

static BOOLEAN
FreeldrReadFile(PVOID FileContext, PVOID Buffer, ULONG Size)
{
  U32 BytesRead;

  return FsReadFile((PFILE) FileContext, (U32) Size, &BytesRead, Buffer)
         && Size == BytesRead;
}

static BOOLEAN
FreeldrSeekFile(PVOID FileContext, ULONG_PTR Position)
{
  FsSetFilePointer((PFILE) FileContext, (U32) Position);

  return TRUE;
}

static BOOL
LoadKernelSymbols(PCHAR szKernelName, int nPos)
{
  static ROSSYM_CALLBACKS FreeldrCallbacks =
    {
      FreeldrAllocMem,
      FreeldrFreeMem,
      FreeldrReadFile,
      FreeldrSeekFile
    };
  PFILE FilePointer;
  PROSSYM_INFO RosSymInfo;
  U32 Size;
  PVOID Base;

  RosSymInit(&FreeldrCallbacks);

  FilePointer = FsOpenFile(szKernelName);
  if (FilePointer == NULL)
    {
      return FALSE;
    }

  if (! RosSymCreateFromFile(FilePointer, &RosSymInfo))
    {
      return FALSE;
    }

  Base = MultiBootCreateModule("NTOSKRNL.SYM");
  Size = RosSymGetRawDataLength(RosSymInfo);
  RosSymGetRawData(RosSymInfo, Base);
  MultiBootCloseModule(Base, Size);

  RosSymDelete(RosSymInfo);

  return TRUE;
}

static BOOL
LoadDriver(PCHAR szFileName, int nPos)
{
  PFILE FilePointer;
  char value[256];
  char *p;

  FilePointer = FsOpenFile(szFileName);
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

  UiDrawProgressBarCenter(nPos, 100, "Loading ReactOS...");

  return(TRUE);
}


static BOOL
LoadNlsFile(PCHAR szFileName, PCHAR szModuleName)
{
  PFILE FilePointer;
  char value[256];
  char *p;

  FilePointer = FsOpenFile(szFileName);
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


static VOID
LoadBootDrivers(PCHAR szSystemRoot, int nPos)
{
  S32 rc = 0;
  HKEY hGroupKey, hOrderKey, hServiceKey, hDriverKey;
  char GroupNameBuffer[512];
  char ServiceName[256];
  U32 OrderList[128];
  U32 BufferSize;
  U32 Index;
  U32 TagIndex;
  char *GroupName;

  U32 ValueSize;
  U32 ValueType;
  U32 StartValue;
  U32 TagValue;
  UCHAR DriverGroup[256];
  U32 DriverGroupSize;

  UCHAR ImagePath[256];
  UCHAR TempImagePath[256];

  /* get 'service group order' key */
  rc = RegOpenKey(NULL,
		  "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\ServiceGroupOrder",
		  &hGroupKey);
  if (rc != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REACTOS, "Failed to open the 'ServiceGroupOrder' key (rc %d)\n", (int)rc));
      return;
    }
  
  /* get 'group order list' key */
  rc = RegOpenKey(NULL,
                  "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\GroupOrderList",
		  &hOrderKey);
  if (rc != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REACTOS, "Failed to open the 'GroupOrderList' key (rc %d)\n", (int)rc));
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

  BufferSize = sizeof(GroupNameBuffer);
  rc = RegQueryValue(hGroupKey, "List", NULL, (PUCHAR)GroupNameBuffer, &BufferSize);
  DbgPrint((DPRINT_REACTOS, "RegQueryValue(): rc %d\n", (int)rc));
  if (rc != ERROR_SUCCESS)
    return;

  DbgPrint((DPRINT_REACTOS, "BufferSize: %d \n", (int)BufferSize));

  DbgPrint((DPRINT_REACTOS, "GroupNameBuffer: '%s' \n", GroupNameBuffer));

  GroupName = GroupNameBuffer;
  while (*GroupName)
    {
      DbgPrint((DPRINT_REACTOS, "Driver group: '%s'\n", GroupName));

      BufferSize = sizeof(OrderList);
      rc = RegQueryValue(hOrderKey, GroupName, NULL, (PUCHAR)OrderList, &BufferSize);
      if (rc != ERROR_SUCCESS)
        {
	  OrderList[0] = 0;
	}
      
      for (TagIndex = 1; TagIndex <= OrderList[0]; TagIndex++)
        {
	  /* enumerate all drivers */
	  Index = 0;
	  while (TRUE)
	    {
	      ValueSize = sizeof(ServiceName);
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

	      ValueSize = sizeof(U32);
	      rc = RegQueryValue(hDriverKey, "Tag", &ValueType, (PUCHAR)&TagValue, &ValueSize);
	      if (rc != ERROR_SUCCESS)
	        {
		  TagValue = (U32)-1;
		}
	      DbgPrint((DPRINT_REACTOS, "  Tag:   %x  \n", (int)TagValue));
		  

	      DriverGroupSize = 256;
	      rc = RegQueryValue(hDriverKey, "Group", NULL, (PUCHAR)DriverGroup, &DriverGroupSize);
	      DbgPrint((DPRINT_REACTOS, "  Group: '%s'  \n", DriverGroup));

	      if ((StartValue == 0) && (TagValue == OrderList[TagIndex]) &&(stricmp(DriverGroup, GroupName) == 0))
	        {
	          ValueSize = 256;
	          rc = RegQueryValue(hDriverKey,
				     "ImagePath",
				     NULL,
				     (PUCHAR)TempImagePath,
				     &ValueSize);
	          if (rc != ERROR_SUCCESS)
		    {
		      DbgPrint((DPRINT_REACTOS, "  ImagePath: not found\n"));
		      strcpy(ImagePath, szSystemRoot);
		      strcat(ImagePath, "system32\\drivers\\");
		      strcat(ImagePath, ServiceName);
		      strcat(ImagePath, ".sys");
		    }
	          else if (TempImagePath[0] != '\\')
		    {
		      strcpy(ImagePath, szSystemRoot);
		      strcat(ImagePath, TempImagePath);
		    }
	          else
		    {
		      strcpy(ImagePath, TempImagePath);
		      DbgPrint((DPRINT_REACTOS, "  ImagePath: '%s'\n", ImagePath));
		    }
	          DbgPrint((DPRINT_REACTOS, "  Loading driver: '%s'\n", ImagePath));

	          if (nPos < 100)
		    nPos += 5;

	          LoadDriver(ImagePath, nPos);
	        }
	      else
	        {
	          DbgPrint((DPRINT_REACTOS, "  Skipping driver '%s' with Start %d, Tag %d and Group '%s' (Current Tag %d, current group '%s')\n",
	                   ServiceName, StartValue, TagValue, DriverGroup, OrderList[TagIndex], GroupName));
	        }
	      Index++;
	    }
	}  

      Index = 0;
      while (TRUE)
	{
	  ValueSize = sizeof(ServiceName);
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

	  ValueSize = sizeof(U32);
	  rc = RegQueryValue(hDriverKey, "Tag", &ValueType, (PUCHAR)&TagValue, &ValueSize);
	  if (rc != ERROR_SUCCESS)
	    {
	      TagValue = (U32)-1;
	    }
	  DbgPrint((DPRINT_REACTOS, "  Tag:   %x  \n", (int)TagValue));

	  DriverGroupSize = 256;
	  rc = RegQueryValue(hDriverKey, "Group", NULL, (PUCHAR)DriverGroup, &DriverGroupSize);
	  DbgPrint((DPRINT_REACTOS, "  Group: '%s'  \n", DriverGroup));

          for (TagIndex = 1; TagIndex <= OrderList[0]; TagIndex++)
	    {
	      if (TagValue == OrderList[TagIndex])
	        break;
	    }

	  if ((StartValue == 0) && (TagIndex > OrderList[0]) && (stricmp(DriverGroup, GroupName) == 0))
	    {
	      ValueSize = 256;
	      rc = RegQueryValue(hDriverKey,
				 "ImagePath",
				 NULL,
				 (PUCHAR)TempImagePath,
				 &ValueSize);
	      if (rc != ERROR_SUCCESS)
		{
		  DbgPrint((DPRINT_REACTOS, "  ImagePath: not found\n"));
		  strcpy(ImagePath, szSystemRoot);
		  strcat(ImagePath, "system32\\drivers\\");
		  strcat(ImagePath, ServiceName);
		  strcat(ImagePath, ".sys");
		}
	      else if (TempImagePath[0] != '\\')
		{
		  strcpy(ImagePath, szSystemRoot);
		  strcat(ImagePath, TempImagePath);
		}
	      else
		{
		  strcpy(ImagePath, TempImagePath);
		  DbgPrint((DPRINT_REACTOS, "  ImagePath: '%s'\n", ImagePath));
		}
	      DbgPrint((DPRINT_REACTOS, "  Loading driver: '%s'\n", ImagePath));

	      if (nPos < 100)
		nPos += 5;

	      LoadDriver(ImagePath, nPos);
	    }
	  else
	    {
	      DbgPrint((DPRINT_REACTOS, "  Skipping driver '%s' with Start %d, Tag %d and Group '%s' (Current group '%s')\n",
	               ServiceName, StartValue, TagValue, DriverGroup, GroupName));
	    }
	  Index++;
	}

      GroupName = GroupName + strlen(GroupName) + 1;
    }
}


static BOOL
LoadNlsFiles(PCHAR szSystemRoot, PCHAR szErrorOut)
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
  if (rc != ERROR_SUCCESS) {
      strcpy(szErrorOut, "Couldn't open CodePage registry key");
      return(FALSE);
  }


  /* get ANSI codepage */
  BufferSize = 80;
  rc = RegQueryValue(hKey, "ACP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS) {
      strcpy(szErrorOut, "Couldn't get ACP NLS setting");
      return(FALSE);
  }

  BufferSize = 80;
  rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS) {
      strcpy(szErrorOut, "ACP NLS Setting exists, but isn't readable");
      return(FALSE);
  }

  /* load ANSI codepage table */
  strcpy(szFileName, szSystemRoot);
  strcat(szFileName, "system32\\");
  strcat(szFileName, szNameBuffer);
  DbgPrint((DPRINT_REACTOS, "ANSI file: %s\n", szFileName));
  if (!LoadNlsFile(szFileName, "ansi.nls")) {
      strcpy(szErrorOut, "Couldn't load ansi.nls");
      return(FALSE);
  }

  /* get OEM codepage */
  BufferSize = 80;
  rc = RegQueryValue(hKey, "OEMCP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS) {
      strcpy(szErrorOut, "Couldn't get OEMCP NLS setting");
      return(FALSE);
  }

  BufferSize = 80;
  rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS) {
      strcpy(szErrorOut, "OEMCP NLS setting exists, but isn't readable");
      return(FALSE);
  }

  /* load OEM codepage table */
  strcpy(szFileName, szSystemRoot);
  strcat(szFileName, "system32\\");
  strcat(szFileName, szNameBuffer);
  DbgPrint((DPRINT_REACTOS, "Oem file: %s\n", szFileName));
  if (!LoadNlsFile(szFileName, "oem.nls")) {
      strcpy(szErrorOut, "Couldn't load oem.nls");
      return(FALSE);
  }

  /* open the language key */
  rc = RegOpenKey(NULL,
		  "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\Language",
		  &hKey);
  if (rc != ERROR_SUCCESS) {
      strcpy(szErrorOut, "Couldn't open Language registry key");
      return(FALSE);
  }

  /* get the Unicode case table */
  BufferSize = 80;
  rc = RegQueryValue(hKey, "Default", NULL, (PUCHAR)szIdBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS) {
      strcpy(szErrorOut, "Couldn't get Language Default setting");
      return(FALSE);
  }

  BufferSize = 80;
  rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)szNameBuffer, &BufferSize);
  if (rc != ERROR_SUCCESS) {
      strcpy(szErrorOut, 
	     "Language Default setting exists, but isn't readable");
      return(FALSE);
  }

  /* load Unicode case table */
  strcpy(szFileName, szSystemRoot);
  strcat(szFileName, "system32\\");
  strcat(szFileName, szNameBuffer);
  DbgPrint((DPRINT_REACTOS, "Casemap file: %s\n", szFileName));
  if (!LoadNlsFile(szFileName, "casemap.nls")) {
      strcpy(szErrorOut, "casemap.nls");
      return(FALSE);
  }

  return(TRUE);
}


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
	int		i;
	char  MsgBuffer[256];
	U32 SectionId;

	char* Base;
	U32 Size;

	PARTITION_TABLE_ENTRY PartitionTableEntry;
	U32 rosPartition;

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
	mb_info.boot_device = 0xffffffff;
	mb_info.cmdline = (unsigned long)multiboot_kernel_cmdline;
	mb_info.mods_count = 0;
	mb_info.mods_addr = (unsigned long)multiboot_modules;
	mb_info.mmap_length = (unsigned long)MachGetMemoryMap((PBIOS_MEMORY_MAP)(PVOID)&multiboot_memory_map, 32) * sizeof(memory_map_t);
	if (mb_info.mmap_length)
	{
		mb_info.mmap_addr = (unsigned long)&multiboot_memory_map;
		mb_info.flags |= MB_INFO_FLAG_MEM_SIZE | MB_INFO_FLAG_MEMORY_MAP;
		multiboot_memory_map_descriptor_size = sizeof(memory_map_t); // GetBiosMemoryMap uses a fixed value of 24
		DbgPrint((DPRINT_REACTOS, "memory map length: %d\n", mb_info.mmap_length));
		DbgPrint((DPRINT_REACTOS, "dumping memory map:\n"));
		for (i=0; i<(mb_info.mmap_length/sizeof(memory_map_t)); i++)
		{
			if (MEMTYPE_USABLE == multiboot_memory_map[i].type &&
			    0 == multiboot_memory_map[i].base_addr_low)
			{
				mb_info.mem_lower = (multiboot_memory_map[i].base_addr_low + multiboot_memory_map[i].length_low) / 1024;
				if (640 < mb_info.mem_lower)
				{
					mb_info.mem_lower = 640;
				}
			}
			if (MEMTYPE_USABLE == multiboot_memory_map[i].type &&
			    multiboot_memory_map[i].base_addr_low <= 1024 * 1024 &&
			    1024 * 1024 <= multiboot_memory_map[i].base_addr_low + multiboot_memory_map[i].length_low)
			{
				mb_info.mem_upper = (multiboot_memory_map[i].base_addr_low + multiboot_memory_map[i].length_low) / 1024 - 1024;
			}
			DbgPrint((DPRINT_REACTOS, "start: %x\t size: %x\t type %d\n", 
			          multiboot_memory_map[i].base_addr_low, 
				  multiboot_memory_map[i].length_low,
				  multiboot_memory_map[i].type));
		}
	}
	DbgPrint((DPRINT_REACTOS, "low_mem = %d\n", mb_info.mem_lower));
	DbgPrint((DPRINT_REACTOS, "high_mem = %d\n", mb_info.mem_upper));

	/*
	 * Initialize the registry
	 */
	RegInitializeRegistry();

	/*
	 * Make sure the system path is set in the .ini file
	 */
	if (!IniReadSettingByName(SectionId, "SystemPath", value, 1024))
	{
		UiMessageBox("System path not specified for selected operating system.");
		return;
	}

	/*
	 * Special case for Live CD.
	 */
	if (!stricmp(value, "LiveCD"))
	{
		strcpy(szBootPath, "\\reactos");

		/* Set kernel command line */
		sprintf(multiboot_kernel_cmdline,
		        "multi(0)disk(0)cdrom(%u)\\reactos /MININT",
		        (unsigned int)BootDrive);
	}
	else
	{
		/*
		 * Verify system path
		 */
		if (!DissectArcPath(value, szBootPath, &BootDrive, &BootPartition))
		{
			sprintf(MsgBuffer,"Invalid system path: '%s'", value);
			UiMessageBox(MsgBuffer);
			return;
		}

		/* recalculate the boot partition for freeldr */
		i = 0;
		rosPartition = 0;
		while (1)
		{
		   if (!MachDiskGetPartitionEntry(BootDrive, ++i, &PartitionTableEntry))
		   {
		      BootPartition = 0;
		      break;
		   }
		   if (IsRecognizedPartition(PartitionTableEntry.SystemIndicator))
		   {
		      if (++rosPartition == BootPartition)
		      {
		         BootPartition = i;
			 break;
		      }
		   }
		}

		if (BootPartition == 0)
		{
			sprintf(MsgBuffer,"Invalid system path: '%s'", value);
			UiMessageBox(MsgBuffer);
			return;
		}

		/* copy ARC path into kernel command line */
		strcpy(multiboot_kernel_cmdline, value);
	}

	/* Set boot drive and partition */
	((char *)(&mb_info.boot_device))[0] = (char)BootDrive;
	((char *)(&mb_info.boot_device))[1] = (char)BootPartition;

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

	DbgPrint((DPRINT_REACTOS,"SystemRoot: '%s'\n", szBootPath));


	UiDrawBackdrop();
	UiDrawStatusText("Detecting Hardware...");

	/*
	 * Detect hardware
	 */
	MachHwDetect();


	UiDrawStatusText("Loading...");
	UiDrawProgressBarCenter(0, 100, "Loading ReactOS...");

	/*
	 * Try to open boot drive
	 */
	if (!FsOpenVolume(BootDrive, BootPartition))
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
	 * Load the System hive from disk
	 */
	strcpy(szFileName, szBootPath);
	strcat(szFileName, "SYSTEM32\\CONFIG\\SYSTEM");

	DbgPrint((DPRINT_REACTOS, "SystemHive: '%s'", szFileName));

	FilePointer = FsOpenFile(szFileName);
	if (FilePointer == NULL)
	{
		UiMessageBox("Could not find the System hive!");
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
	 * Load the System hive
	 */
	Base = MultiBootLoadModule(FilePointer, szFileName, &Size);
	if (Base == NULL || Size == 0)
	{
		UiMessageBox("Could not load the System hive!\n");
		return;
	}
	DbgPrint((DPRINT_REACTOS, "SystemHive loaded at 0x%x size %u", (unsigned)Base, (unsigned)Size));

	/*
	 * Import the loaded system hive
	 */
	RegImportBinaryHive(Base, Size);

	/*
	 * Initialize the 'CurrentControlSet' link
	 */
	RegInitCurrentControlSet(FALSE);

	UiDrawProgressBarCenter(15, 100, "Loading ReactOS...");

	/*
	 * Export the hardware hive
	 */
	Base = MultiBootCreateModule ("HARDWARE");
	RegExportBinaryHive ("\\Registry\\Machine\\HARDWARE", Base, &Size);
	MultiBootCloseModule (Base, Size);

	UiDrawProgressBarCenter(20, 100, "Loading ReactOS...");

	/*
	 * Load NLS files
	 */
	if (!LoadNlsFiles(szBootPath, MsgBuffer))
	{
	        UiMessageBox(MsgBuffer);
		return;
	}
	UiDrawProgressBarCenter(30, 100, "Loading ReactOS...");

	/*
	 * Load kernel symbols
	 */
	LoadKernelSymbols(szKernelName, 30);
	UiDrawProgressBarCenter(40, 100, "Loading ReactOS...");

	/*
	 * Load boot drivers
	 */
	LoadBootDrivers(szBootPath, 40);

#if 0
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
	MessageBox(name);
#endif

	UiUnInitialize("Booting ReactOS...");

	/*
	 * Now boot the kernel
	 */
	DiskStopFloppyMotor();
	boot_reactos();
}

#undef DbgPrint
ULONG
DbgPrint(char *Fmt, ...)
{
  UiMessageBox(Fmt);
  return 0;
}

/* EOF */
