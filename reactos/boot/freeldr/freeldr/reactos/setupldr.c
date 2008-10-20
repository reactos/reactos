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
extern ULONG_PTR KernelBase;
extern ROS_KERNEL_ENTRY_POINT KernelEntryPoint;

extern BOOLEAN FrLdrLoadDriver(PCHAR szFileName, INT nPos);
extern BOOLEAN FrLdrLoadNlsFile(PCSTR szFileName, PCSTR szModuleName);

#define USE_UI

VOID RunLoader(VOID)
{
    ULONG i;
    LPCSTR SourcePath;
    LPCSTR LoadOptions, DbgLoadOptions = "";
    LPCSTR sourcePaths[] = {
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
    CHAR FileName[256];

  HINF InfHandle;
  ULONG ErrorLine;
  INFCONTEXT InfContext;
    PIMAGE_NT_HEADERS NtHeader;
    PVOID LoadBase;

  /* Setup multiboot information structure */
  LoaderBlock.CommandLine = reactos_kernel_cmdline;
  LoaderBlock.PageDirectoryStart = (ULONG_PTR)&PageDirectoryStart;
  LoaderBlock.PageDirectoryEnd = (ULONG_PTR)&PageDirectoryEnd;
  LoaderBlock.ModsCount = 0;
  LoaderBlock.ModsAddr = reactos_modules;
  LoaderBlock.MmapLength = (unsigned long)MachGetMemoryMap((PBIOS_MEMORY_MAP)reactos_memory_map, 32) * sizeof(memory_map_t);
  if (LoaderBlock.MmapLength)
  {
#if defined (_M_IX86) || defined (_M_AMD64)
      ULONG i;
#endif
      LoaderBlock.Flags |= MB_FLAGS_MEM_INFO | MB_FLAGS_MMAP_INFO;
      LoaderBlock.MmapAddr = (ULONG_PTR)&reactos_memory_map;
      reactos_memory_map_descriptor_size = sizeof(memory_map_t); // GetBiosMemoryMap uses a fixed value of 24
#if defined (_M_IX86) || defined (_M_AMD64)
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
#endif
  }

#ifdef USE_UI
  SetupUiInitialize();
#endif
  UiDrawStatusText("");

    extern BOOLEAN FrLdrBootType;
    FrLdrBootType = TRUE;

  /* Detect hardware */
  UiDrawStatusText("Detecting hardware...");
  LoaderBlock.ArchExtra = (ULONG_PTR)MachHwDetect();
  UiDrawStatusText("");

  /* set boot device */
  MachDiskGetBootDevice(&LoaderBlock.BootDevice);

  /* Open boot drive */
  if (!FsOpenBootVolume())
    {
      UiMessageBox("Failed to open boot drive.");
      return;
    }

  UiDrawStatusText("Loading txtsetup.sif...");
  /* Open 'txtsetup.sif' */
  for (i = MachDiskBootingFromFloppy() ? 0 : 1; ; i++)
  {
    SourcePath = sourcePaths[i];
    if (!SourcePath)
    {
      printf("Failed to open 'txtsetup.sif'\n");
      return;
    }
    sprintf(FileName,"%s\\txtsetup.sif", SourcePath);
    if (InfOpenFile (&InfHandle, FileName, &ErrorLine))
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

    /* Setup the boot path and kernel path */
    strcpy(szBootPath, SourcePath);

    sprintf(SystemRoot,"%s\\", SourcePath);
    sprintf(FileName,"%s\\ntoskrnl.exe", SourcePath);
    sprintf(szHalName,"%s\\hal.dll", SourcePath);

    /* Load the kernel */
    LoadBase = FrLdrLoadImage(FileName, 5, 1);
    if (!LoadBase) return;

    /* Get the NT header, kernel base and kernel entry */
    NtHeader = RtlImageNtHeader(LoadBase);
    KernelBase = SWAPD(NtHeader->OptionalHeader.ImageBase);
    KernelEntryPoint = (ROS_KERNEL_ENTRY_POINT)(KernelBase + SWAPD(NtHeader->OptionalHeader.AddressOfEntryPoint));
    LoaderBlock.KernelBase = KernelBase;

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

  sprintf(FileName,"%s\\%s", SourcePath,LoadOptions);
  /* Load ANSI codepage file */
  if (!FrLdrLoadNlsFile(FileName, "ansi.nls"))
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

    sprintf(FileName,"%s\\%s", SourcePath,LoadOptions);
  /* Load OEM codepage file */
  if (!FrLdrLoadNlsFile(FileName, "oem.nls"))
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

    sprintf(FileName,"%s\\%s", SourcePath,LoadOptions);
  /* Load Unicode casemap file */
  if (!FrLdrLoadNlsFile(FileName, "casemap.nls"))
    {
      UiMessageBox("Failed to load the Unicode casemap file.");
      return;
    }

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
                    if (!FrLdrLoadDriver((PCHAR)DriverName,0))
                    {
                        DbgPrint((DPRINT_WARNING, "could not load %s, %s\n", SourcePath, DriverName));
                        return;
                    }
                }
            }
        } while (InfFindNextLine(&InfContext, &InfContext));
    }

  UiUnInitialize("Booting ReactOS...");

    //
    // Perform architecture-specific pre-boot configuration
    //
    MachPrepareForReactOS(TRUE);
    
    //
    // Setup paging and jump to kernel
    //
	FrLdrStartup(0x2badb002);
}

/* EOF */
