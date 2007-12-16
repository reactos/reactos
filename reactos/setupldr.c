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

ROS_LOADER_PARAMETER_BLOCK LoaderBlock;
char					reactos_kernel_cmdline[255];	// Command line passed to kernel
LOADER_MODULE			reactos_modules[64];		// Array to hold boot module info loaded for the kernel
char					reactos_module_strings[64][256];	// Array to hold module names
unsigned long			reactos_memory_map_descriptor_size;
memory_map_t			reactos_memory_map[32];		// Memory map
char szBootPath[256];
char szHalName[256];

#define USE_UI

static BOOLEAN
FreeldrReadFile(PVOID FileContext, PVOID Buffer, ULONG Size)
{
  ULONG BytesRead;

  return FsReadFile((PFILE) FileContext, (ULONG) Size, &BytesRead, Buffer)
         && Size == BytesRead;
}

static BOOLEAN
FreeldrSeekFile(PVOID FileContext, ULONG_PTR Position)
{
  FsSetFilePointer((PFILE) FileContext, (ULONG) Position);
    return TRUE;
}

BOOLEAN
NTAPI
static FrLdrLoadKernel(IN PCHAR szFileName,
                       IN INT nPos)
{
    PFILE FilePointer;
    PCHAR szShortName;
    CHAR szBuffer[256];

    /* Extract Kernel filename without path */
    szShortName = strrchr(szFileName, '\\');
    if (!szShortName)
    {
        /* No path, leave it alone */
        szShortName = szFileName;
    }
    else
    {
        /* Skip the path */
        szShortName = szShortName + 1;
    }

    /* Open the Kernel */
    FilePointer = FsOpenFile(szFileName);
    if (!FilePointer)
    {
        /* Return failure on the short name */
        strcpy(szBuffer, szShortName);
        strcat(szBuffer, " not found.");
        UiMessageBox(szBuffer);
        return FALSE;
    }

    /* Update the status bar with the current file */
    strcpy(szBuffer, "Reading ");
    strcat(szBuffer, szShortName);
    UiDrawStatusText(szBuffer);

    /* Do the actual loading */
    FrLdrMapKernel(FilePointer);

    /* Update Processbar and return success */
    return TRUE;
}

static BOOLEAN
LoadKernelSymbols(PCSTR szSourcePath, PCSTR szFileName)
{
  static ROSSYM_CALLBACKS FreeldrCallbacks =
    {
      MmAllocateMemory,
      MmFreeMemory,
      FreeldrReadFile,
      FreeldrSeekFile
    };
  CHAR szFullName[256];
  PFILE FilePointer;
  PROSSYM_INFO RosSymInfo;
  ULONG Size;
  ULONG_PTR Base;

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

  RosSymInit(&FreeldrCallbacks);

  FilePointer = FsOpenFile(szFullName);
  if (FilePointer  && RosSymCreateFromFile(FilePointer, &RosSymInfo))
    {
      Base = FrLdrCreateModule("NTOSKRNL.SYM");
      Size = RosSymGetRawDataLength(RosSymInfo);
      RosSymGetRawData(RosSymInfo, (PVOID)Base);
      FrLdrCloseModule(Base, Size);
      RosSymDelete(RosSymInfo);
      return TRUE;
    }
  return FALSE;
}

static BOOLEAN
LoadDriver(PCSTR szSourcePath, PCSTR szFileName)
{
  CHAR szFullName[256];
#ifdef USE_UI
  CHAR szBuffer[80];
#endif
  PFILE FilePointer;
  PCSTR szShortName;

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
  sprintf(szBuffer, "Setup is loading files (%s)", szShortName);
  UiDrawStatusText(szBuffer);
#else
  printf("Reading %s\n", szShortName);
#endif

  /* Load the driver */
  FrLdrLoadModule(FilePointer, szFileName, NULL);

  return(TRUE);
}


static BOOLEAN
LoadNlsFile(PCSTR szSourcePath, PCSTR szFileName, PCSTR szModuleName)
{
  CHAR szFullName[256];
#ifdef USE_UI
  CHAR szBuffer[80];
#endif
  PFILE FilePointer;
  PCSTR szShortName;

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
  sprintf(szBuffer, "Setup is loading files (%s)", szShortName);
  UiDrawStatusText(szBuffer);
#else
  printf("Reading %s\n", szShortName);
#endif

  /* Load the driver */
  FrLdrLoadModule(FilePointer, szModuleName, NULL);

  return(TRUE);
}

VOID RunLoader(VOID)
{
  //ULONG_PTR Base;
  //ULONG Size;
  const char *SourcePath;
  const char *LoadOptions = "", *DbgLoadOptions = "";
  UINT i;
  char szKernelName[256];
  PCONFIGURATION_COMPONENT_DATA ConfigRoot;

  HINF InfHandle;
  ULONG ErrorLine;
  INFCONTEXT InfContext;

  extern ULONG PageDirectoryStart;
  extern ULONG PageDirectoryEnd;

  /* Setup multiboot information structure */
  LoaderBlock.Flags = MB_FLAGS_BOOT_DEVICE | MB_FLAGS_COMMAND_LINE | MB_FLAGS_MODULE_INFO;
  LoaderBlock.PageDirectoryStart = (ULONG)&PageDirectoryStart;
  LoaderBlock.PageDirectoryEnd = (ULONG)&PageDirectoryEnd;
  LoaderBlock.BootDevice = 0xffffffff;
  LoaderBlock.CommandLine = reactos_kernel_cmdline;
  LoaderBlock.ModsCount = 0;
  LoaderBlock.ModsAddr = reactos_modules;
  LoaderBlock.MmapLength = (unsigned long)MachGetMemoryMap((PBIOS_MEMORY_MAP)(PVOID)&reactos_memory_map, 32) * sizeof(memory_map_t);
  if (LoaderBlock.MmapLength)
    {
      LoaderBlock.MmapAddr = (unsigned long)&reactos_memory_map;
      LoaderBlock.Flags |= MB_FLAGS_MEM_INFO | MB_FLAGS_MMAP_INFO;
      reactos_memory_map_descriptor_size = sizeof(memory_map_t); // GetBiosMemoryMap uses a fixed value of 24
      for (i = 0; i < (LoaderBlock.MmapLength / sizeof(memory_map_t)); i++)
        {
          if (BiosMemoryUsable == reactos_memory_map[i].type &&
              0 == reactos_memory_map[i].base_addr_low)
            {
              LoaderBlock.MemLower = (reactos_memory_map[i].base_addr_low + reactos_memory_map[i].length_low) / 1024;
              if (640 < LoaderBlock.MemLower)
                {
                  LoaderBlock.MemLower = 640;
                }
            }
          if (BiosMemoryUsable == reactos_memory_map[i].type &&
              reactos_memory_map[i].base_addr_low <= 1024 * 1024 &&
              1024 * 1024 <= reactos_memory_map[i].base_addr_low + reactos_memory_map[i].length_low)
            {
              LoaderBlock.MemHigher = (reactos_memory_map[i].base_addr_low + reactos_memory_map[i].length_low) / 1024 - 1024;
            }
#if 0
	    printf("start: %x\t size: %x\t type %d\n",
		   reactos_memory_map[i].base_addr_low,
		   reactos_memory_map[i].length_low,
		   reactos_memory_map[i].type);
#endif
        }
    }
#if 0
  printf("low_mem = %d\n", LoaderBlock.MemLower);
  printf("high_mem = %d\n", LoaderBlock.MemHigher);
  MachConsGetCh();
#endif

#ifdef USE_UI
  SetupUiInitialize();
  UiDrawStatusText("");
#endif

    extern BOOLEAN FrLdrBootType;
    FrLdrBootType = TRUE;

  /* Initialize registry */
  RegInitializeRegistry();

  /* Detect hardware */
#ifdef USE_UI
  UiDrawStatusText("Detecting hardware...");
#else
  printf("Detecting hardware...\n\n");
#endif
  MachHwDetect(&ConfigRoot);
#ifdef USE_UI
  UiDrawStatusText("");
#endif

  /* set boot device */
  MachDiskGetBootDevice(&LoaderBlock.BootDevice);

  /* Open boot drive */
  if (!FsOpenBootVolume())
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
		    MachDiskBootingFromFloppy() ? "\\txtsetup.sif" : "\\reactos\\txtsetup.sif",
		    &ErrorLine))
    {
      printf("Failed to open 'txtsetup.sif'\n");
      return;
    }

#ifdef DBG
  /* Get load options */  
  if (InfFindFirstLine (InfHandle,
			"SetupData",
			"DbgOsLoadOptions",
			&InfContext))
    {
	if (!InfGetDataField (&InfContext, 1, &DbgLoadOptions))
	    DbgLoadOptions = "";
    }
#endif
  if (!strlen(DbgLoadOptions) && !InfFindFirstLine (InfHandle,
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

  if (MachDiskBootingFromFloppy())
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
  MachDiskGetBootPath(reactos_kernel_cmdline, sizeof(reactos_kernel_cmdline));
  strcat(strcat(strcat(strcat(reactos_kernel_cmdline, SourcePath), " "),
		LoadOptions), DbgLoadOptions);

    /* Setup the boot path and kernel path */
    strcpy(szBootPath, SourcePath);
    strcpy(szKernelName, szBootPath);
    strcat(szKernelName, "\\ntoskrnl.exe");

    /* Setup the HAL path */
    strcpy(szHalName, szBootPath);
    strcat(szHalName, "\\hal.dll");

    /* Load the kernel */
    if (!FrLdrLoadKernel(szKernelName, 5)) return;

  /* Create ntoskrnl.sym */
  LoadKernelSymbols(SourcePath, "ntoskrnl.exe");

  /* Export the hardware hive */
  //Base = FrLdrCreateModule ("HARDWARE");
  //RegExportBinaryHive (L"\\Registry\\Machine\\HARDWARE", (PVOID)Base, &Size);
  //FrLdrCloseModule (Base, Size);

#if 0
  printf("Base: %x\n", Base);
  printf("Size: %u\n", Size);
  printf("*** System stopped ***\n");
for(;;);
#endif

  /* Insert boot disk 2 */
  if (MachDiskBootingFromFloppy())
    {
#ifdef USE_UI
      UiMessageBox("Please insert \"ReactOS Boot Disk 2\" and press ENTER");
#else
      printf("\n\n Please insert \"ReactOS Boot Disk 2\" and press ENTER\n");
      MachConsGetCh();
#endif

      /* Open boot drive */
      if (!FsOpenBootVolume())
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


  /* Get ANSI codepage file */
  if (!InfFindFirstLine (InfHandle,
			 "NLS",
			 "AnsiCodepage",
			 &InfContext))
    {
      printf("Failed to find 'NLS/AnsiCodepage'\n");
      return;
    }

  if (!InfGetDataField (&InfContext,
			1,
			&LoadOptions))
    {
      printf("Failed to get load options\n");
      return;
    }

  /* Load ANSI codepage file */
  if (!LoadNlsFile(SourcePath, LoadOptions, "ansi.nls"))
    {
#ifdef USE_UI
      UiMessageBox("Failed to load the ANSI codepage file.");
#else
      printf("Failed to load the ANSI codepage file.");
#endif
      return;
    }

  /* Get OEM codepage file */
  if (!InfFindFirstLine (InfHandle,
			 "NLS",
			 "OemCodepage",
			 &InfContext))
    {
      printf("Failed to find 'NLS/AnsiCodepage'\n");
      return;
    }

  if (!InfGetDataField (&InfContext,
			1,
			&LoadOptions))
    {
      printf("Failed to get load options\n");
      return;
    }

  /* Load OEM codepage file */
  if (!LoadNlsFile(SourcePath, LoadOptions, "oem.nls"))
    {
#ifdef USE_UI
      UiMessageBox("Failed to load the OEM codepage file.");
#else
      printf("Failed to load the OEM codepage file.");
#endif
      return;
    }

  /* Get Unicode Casemap file */
  if (!InfFindFirstLine (InfHandle,
			 "NLS",
			 "UnicodeCasetable",
			 &InfContext))
    {
      printf("Failed to find 'NLS/AnsiCodepage'\n");
      return;
    }

  if (!InfGetDataField (&InfContext,
			1,
			&LoadOptions))
    {
      printf("Failed to get load options\n");
      return;
    }

  /* Load Unicode casemap file */
  if (!LoadNlsFile(SourcePath, LoadOptions, "casemap.nls"))
    {
#ifdef USE_UI
      UiMessageBox("Failed to load the Unicode casemap file.");
#else
      printf("Failed to load the Unicode casemap file.");
#endif
      return;
    }

#if 0
  /* Load acpi.sys */
  if (!LoadDriver(SourcePath, "acpi.sys"))
    return;
#endif

#if 0
  /* Load isapnp.sys */
  if (!LoadDriver(SourcePath, "isapnp.sys"))
    return;
#endif

#if 0
  /* Load pci.sys */
  if (!LoadDriver(SourcePath, "pci.sys"))
    return;
#endif

  /* Load scsiport.sys */
  if (!LoadDriver(SourcePath, "scsiport.sys"))
    return;

  /* Load atapi.sys (depends on hardware detection) */
  if (!LoadDriver(SourcePath, "atapi.sys"))
    return;

  /* Load buslogic.sys (depends on hardware detection) */
  if (!LoadDriver(SourcePath, "buslogic.sys"))
    return;

  /* Load class2.sys */
  if (!LoadDriver(SourcePath, "class2.sys"))
    return;

  /* Load cdrom.sys */
  if (!LoadDriver(SourcePath, "cdrom.sys"))
    return;

  /* Load cdfs.sys */
  if (!LoadDriver(SourcePath, "cdfs.sys"))
    return;

  /* Load disk.sys */
  if (!LoadDriver(SourcePath, "disk.sys"))
    return;

  /* Load floppy.sys */
  if (!LoadDriver(SourcePath, "floppy.sys"))
    return;

  /* Load vfatfs.sys (could be loaded by the setup prog!) */
  if (!LoadDriver(SourcePath, "vfatfs.sys"))
    return;


  /* Load keyboard driver */
#if 0
  if (!LoadDriver(SourcePath, "keyboard.sys"))
    return;
#endif
  if (!LoadDriver(SourcePath, "i8042prt.sys"))
    return;
  if (!LoadDriver(SourcePath, "kbdclass.sys"))
    return;

  /* Load screen driver */
  if (!LoadDriver(SourcePath, "blue.sys"))
    return;

#ifdef USE_UI
  UiUnInitialize("Booting ReactOS...");
#endif

  /* Now boot the kernel */
  DiskStopFloppyMotor();
  MachVideoPrepareForReactOS();
  FrLdrStartup(0x2badb002);
}

/* EOF */
