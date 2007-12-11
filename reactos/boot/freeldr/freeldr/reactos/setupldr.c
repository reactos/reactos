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

#define _NTSYSTEM_
#include <freeldr.h>
#include <debug.h>

extern ULONG PageDirectoryStart;
extern ULONG PageDirectoryEnd;

ROS_LOADER_PARAMETER_BLOCK LoaderBlock;
char					reactos_kernel_cmdline[255];	// Command line passed to kernel
LOADER_MODULE			reactos_modules[64];		// Array to hold boot module info loaded for the kernel
char					reactos_module_strings[64][256];	// Array to hold module names
reactos_mem_data_t reactos_mem_data;
extern char reactos_arc_hardware_data[HW_MAX_ARC_HEAP_SIZE];
char szBootPath[256];
char szHalName[256];
CHAR SystemRoot[255];
extern ULONG_PTR KernelBase, KernelEntryPoint;

extern BOOLEAN FrLdrLoadDriver(PCHAR szFileName, INT nPos);

#define USE_UI

BOOLEAN
NTAPI
static FrLdrLoadKernel(IN PCHAR szFileName,
                       IN INT nPos)
{
    PFILE FilePointer;
    PCHAR szShortName;
    CHAR szBuffer[256];
    PVOID LoadBase;
    PIMAGE_NT_HEADERS NtHeader;

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
    LoadBase = FrLdrMapImage(FilePointer, szShortName, 1);

    /* Get the NT header, kernel base and kernel entry */
    NtHeader = RtlImageNtHeader(LoadBase);
    KernelBase = NtHeader->OptionalHeader.ImageBase;
    KernelEntryPoint = KernelBase + NtHeader->OptionalHeader.AddressOfEntryPoint;
    LoaderBlock.KernelBase = KernelBase;

    /* Update Processbar and return success */
    return TRUE;
}

static BOOLEAN
LoadDriver(PCSTR szSourcePath, PCSTR szFileName)
{
    return FrLdrLoadDriver((PCHAR)szFileName, 0);
}


static BOOLEAN
LoadNlsFile(PCSTR szSourcePath, PCSTR szFileName, PCSTR szModuleName)
{
  CHAR szFullName[256];
  CHAR szBuffer[80];
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
  sprintf(szBuffer, "Setup is loading files (%s)", szShortName);
  UiDrawStatusText(szBuffer);

  /* Load the driver */
  FrLdrLoadModule(FilePointer, szModuleName, NULL);

  return(TRUE);
}

VOID RunLoader(VOID)
{
  ULONG i;
  const char *SourcePath;
  const char *LoadOptions = "", *DbgLoadOptions = "";
  const char *sourcePaths[] = {
      "", /* Only for floppy boot */
#if defined(_M_IX86)
      "\\I386",
#elif defined(_M_MPPC)
      "\\PPC",
#elif defined(_M_MRX000)
      "\\MIPS",
#endif
      "\\reactos",
      NULL };
  char szKernelName[256];

  HINF InfHandle;
  ULONG ErrorLine;
  INFCONTEXT InfContext;

  /* Setup multiboot information structure */
  LoaderBlock.CommandLine = reactos_kernel_cmdline;
  LoaderBlock.PageDirectoryStart = (ULONG)&PageDirectoryStart;
  LoaderBlock.PageDirectoryEnd = (ULONG)&PageDirectoryEnd;
  LoaderBlock.ModsCount = 0;
  LoaderBlock.ModsAddr = reactos_modules;
  LoaderBlock.ArchExtra = (ULONG)reactos_arc_hardware_data;
  LoaderBlock.MmapLength = (unsigned long)MachGetMemoryMap((PBIOS_MEMORY_MAP)reactos_memory_map, 32) * sizeof(memory_map_t);
  if (LoaderBlock.MmapLength)
  {
      ULONG i;

      LoaderBlock.MmapAddr = (unsigned long)&reactos_memory_map;
      reactos_memory_map_descriptor_size = sizeof(memory_map_t); // GetBiosMemoryMap uses a fixed value of 24
      for (i=0; i<(LoaderBlock.MmapLength/sizeof(memory_map_t)); i++)
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
      }
  }

#ifdef USE_UI
  SetupUiInitialize();
#endif
  UiDrawStatusText("");

    extern BOOLEAN FrLdrBootType;
    FrLdrBootType = TRUE;

  /* Initialize registry */
  RegInitializeRegistry();

  /* Detect hardware */
  UiDrawStatusText("Detecting hardware...");
  MachHwDetect();
  UiDrawStatusText("");

  /* set boot device */
  MachDiskGetBootDevice(&LoaderBlock.BootDevice);

  /* Open boot drive */
  if (!FsOpenBootVolume())
    {
      UiMessageBox("Failed to open boot drive.");
      return;
    }

  /* Open 'txtsetup.sif' */
  for (i = MachDiskBootingFromFloppy() ? 0 : 1; ; i++)
  {
    SourcePath = sourcePaths[i];
    if (!SourcePath)
    {
      printf("Failed to open 'txtsetup.sif'\n");
      return;
    }
    strcpy(szKernelName, SourcePath);
    strcat(szKernelName, "\\txtsetup.sif");
    if (InfOpenFile (&InfHandle, szKernelName, &ErrorLine))
      break;
  }
  if (!*SourcePath)
    SourcePath = "\\";

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

  /* Set kernel command line */
  MachDiskGetBootPath(reactos_kernel_cmdline, sizeof(reactos_kernel_cmdline));
  strcat(strcat(strcat(strcat(reactos_kernel_cmdline, SourcePath), " "),
		LoadOptions), DbgLoadOptions);

  strcpy(SystemRoot, SourcePath);
  strcat(SystemRoot, "\\");

    /* Setup the boot path and kernel path */
    strcpy(szBootPath, SourcePath);
    strcpy(szKernelName, szBootPath);
    strcat(szKernelName, "\\ntoskrnl.exe");

    /* Setup the HAL path */
    strcpy(szHalName, szBootPath);
    strcat(szHalName, "\\hal.dll");

    /* Load the kernel */
    if (!FrLdrLoadKernel(szKernelName, 5)) return;

  /* Insert boot disk 2 */
  if (MachDiskBootingFromFloppy())
    {
      UiMessageBox("Please insert \"ReactOS Boot Disk 2\" and press ENTER");

      /* Open boot drive */
      if (!FsOpenBootVolume())
	{
	  UiMessageBox("Failed to open boot drive.");
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
      UiMessageBox("Failed to load the ANSI codepage file.");
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
      UiMessageBox("Failed to load the OEM codepage file.");
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
      UiMessageBox("Failed to load the Unicode casemap file.");
      return;
    }

  /* Load vfatfs.sys (could be loaded by the setup prog!) */
  if (!LoadDriver(SourcePath, "vfatfs.sys"))
    return;

    /* Load additional files specified in txtsetup.inf */
    if (InfFindFirstLine(InfHandle,
                         "SourceDisksFiles",
                         NULL,
                         &InfContext))
    {
        do
        {
            LPCSTR Media, DriverName;
            if (InfGetDataField(&InfContext, 7, &Media) &&
                InfGetDataField(&InfContext, 0, &DriverName))
            {
                if (strcmp(Media, "x") == 0)
                {
                    if (!LoadDriver(SourcePath, DriverName))
                        return;
                }
            }
        } while (InfFindNextLine(&InfContext, &InfContext));
    }

  UiUnInitialize("Booting ReactOS...");

  /* Now boot the kernel */
  DiskStopFloppyMotor();
  MachVideoPrepareForReactOS(TRUE);
  FrLdrStartup(0x2badb002);
}

/* EOF */
