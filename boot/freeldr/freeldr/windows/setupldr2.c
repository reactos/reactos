/*
 *  FreeLoader
 *
 *  Copyright (C) 2009       Aleksey Bragin  <aleksey@reactos.org>
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

#include <ndk/ldrtypes.h>
#include <arc/setupblk.h>

#include <debug.h>

// TODO: Move to .h
VOID AllocateAndInitLPB(PLOADER_PARAMETER_BLOCK *OutLoaderBlock);
BOOLEAN WinLdrLoadBootDrivers(PLOADER_PARAMETER_BLOCK LoaderBlock, LPSTR BootPath);
void WinLdrSetupForNt(PLOADER_PARAMETER_BLOCK LoaderBlock,
                      PVOID *GdtIdt,
                      ULONG *PcrBasePage,
                      ULONG *TssBasePage);
VOID
WinLdrInitializePhase1(PLOADER_PARAMETER_BLOCK LoaderBlock,
                       PCHAR Options,
                       PCHAR SystemPath,
                       PCHAR BootPath,
                       USHORT VersionToBoot);
BOOLEAN
WinLdrLoadNLSData(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                  IN LPCSTR DirectoryPath,
                  IN LPCSTR AnsiFileName,
                  IN LPCSTR OemFileName,
                  IN LPCSTR LanguageFileName);
BOOLEAN
WinLdrAddDriverToList(LIST_ENTRY *BootDriverListHead,
                      LPWSTR RegistryPath,
                      LPWSTR ImagePath,
                      LPWSTR ServiceName);


//FIXME: Do a better way to retrieve Arc disk information
extern ULONG reactos_disk_count;
extern ARC_DISK_SIGNATURE reactos_arc_disk_info[];
extern char reactos_arc_strings[32][256];

extern BOOLEAN UseRealHeap;
extern ULONG LoaderPagesSpanned;


VOID
SetupLdrLoadNlsData(PLOADER_PARAMETER_BLOCK LoaderBlock, HINF InfHandle, LPCSTR SearchPath)
{
    INFCONTEXT InfContext;
    BOOLEAN Status;
    LPCSTR AnsiName, OemName, LangName;

    /* Get ANSI codepage file */
    if (!InfFindFirstLine(InfHandle, "NLS", "AnsiCodepage", &InfContext))
    {
        printf("Failed to find 'NLS/AnsiCodepage'\n");
        return;
    }
    if (!InfGetDataField(&InfContext, 1, &AnsiName))
    {
        printf("Failed to get load options\n");
        return;
    }

    /* Get OEM codepage file */
    if (!InfFindFirstLine(InfHandle, "NLS", "OemCodepage", &InfContext))
    {
        printf("Failed to find 'NLS/AnsiCodepage'\n");
        return;
    }
    if (!InfGetDataField(&InfContext, 1, &OemName))
    {
        printf("Failed to get load options\n");
        return;
    }

    if (!InfFindFirstLine(InfHandle, "NLS", "UnicodeCasetable", &InfContext))
    {
        printf("Failed to find 'NLS/AnsiCodepage'\n");
        return;
    }
    if (!InfGetDataField(&InfContext, 1, &LangName))
    {
        printf("Failed to get load options\n");
        return;
    }

    Status = WinLdrLoadNLSData(LoaderBlock, SearchPath, AnsiName, OemName, LangName);
    DPRINTM(DPRINT_WINDOWS, "NLS data loaded with status %d\n", Status);
}

VOID
SetupLdrScanBootDrivers(PLOADER_PARAMETER_BLOCK LoaderBlock, HINF InfHandle, LPCSTR SearchPath)
{
    INFCONTEXT InfContext;
    BOOLEAN Status;
    LPCSTR Media, DriverName;
    WCHAR ServiceName[256];
    WCHAR ImagePath[256];

    /* Open inf section */
    if (!InfFindFirstLine(InfHandle, "SourceDisksFiles", NULL, &InfContext))
        return;

    /* Load all listed boot drivers */
    do
    {
        if (InfGetDataField(&InfContext, 7, &Media) &&
            InfGetDataField(&InfContext, 0, &DriverName))
        {
            if (strcmp(Media, "x") == 0)
            {
                /* Convert name to widechar */
                swprintf(ServiceName, L"%S", DriverName);

                /* Remove .sys extension */
                ServiceName[wcslen(ServiceName) - 4] = 0;

                /* Prepare image path */
                swprintf(ImagePath, L"%S", DriverName);

                /* Add it to the list */
                Status = WinLdrAddDriverToList(&LoaderBlock->BootDriverListHead,
                    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\",
                    ImagePath,
                    ServiceName);

                if (!Status)
                {
                    DPRINTM(DPRINT_WINDOWS, "could not add boot driver %s, %s\n", SearchPath, DriverName);
                    return;
                }
            }
        }
    } while (InfFindNextLine(&InfContext, &InfContext));
}

VOID LoadReactOSSetup2(VOID)
{
    CHAR  SystemPath[512], SearchPath[512];
    CHAR  FileName[512];
    CHAR  BootPath[512];
    CHAR  LoadOptions[512];
    LPCSTR BootOptions;
    PVOID NtosBase = NULL, HalBase = NULL, KdComBase = NULL;
    BOOLEAN Status;
    ULONG BootDevice;
    ULONG i, ErrorLine;
    HINF InfHandle;
    INFCONTEXT InfContext;
    PLOADER_PARAMETER_BLOCK LoaderBlock, LoaderBlockVA;
    PSETUP_LOADER_BLOCK SetupBlock;
    KERNEL_ENTRY_POINT KiSystemStartup;
    PLDR_DATA_TABLE_ENTRY KernelDTE, HalDTE, KdComDTE = NULL;
    // Mm-related things
    PVOID GdtIdt;
    ULONG PcrBasePage=0;
    ULONG TssBasePage=0;
    LPCSTR SourcePath;
    LPCSTR SourcePaths[] =
    {
        "", /* Only for floppy boot */
#if defined(_M_IX86)
        "\\I386",
#elif defined(_M_MPPC)
        "\\PPC",
#elif defined(_M_MRX000)
        "\\MIPS",
#endif
        "\\reactos",
        NULL
    };

    /* Get boot device number */
    MachDiskGetBootDevice(&BootDevice);

    /* Open 'txtsetup.sif' from any of source paths */
    for (i = MachDiskBootingFromFloppy() ? 0 : 1; ; i++)
    {
        SourcePath = SourcePaths[i];
        if (!SourcePath)
        {
            printf("Failed to open 'txtsetup.sif'\n");
            return;
        }
        sprintf(FileName,"%s\\txtsetup.sif", SourcePath);
        if (InfOpenFile (&InfHandle, FileName, &ErrorLine))
            break;
    }

    /* If we didn't find it anywhere, then just use root */
    if (!*SourcePath)
        SourcePath = "\\";

    /* Load options */
    if (!InfFindFirstLine(InfHandle,
                          "SetupData",
                          "OsLoadOptions",
                          &InfContext))
    {
        printf("Failed to find 'SetupData/OsLoadOptions'\n");
        return;
    }

    if (!InfGetDataField (&InfContext, 1, &BootOptions))
    {
        printf("Failed to get load options\n");
        return;
    }

    /* Save source path */
    strcpy(BootPath, SourcePath);

    SetupUiInitialize();
    UiDrawStatusText("");
    UiDrawStatusText("Detecting Hardware...");

    /* Let user know we started loading */
    UiDrawStatusText("Loading...");

    /* Try to open system drive */
    FsOpenBootVolume();

    /* Append a backslash to the bootpath if needed */
    if ((strlen(BootPath)==0) || BootPath[strlen(BootPath)] != '\\')
    {
        strcat(BootPath, "\\");
    }

    /* Construct the system path */
    MachDiskGetBootPath(SystemPath, sizeof(SystemPath));
    strcat(SystemPath, SourcePath);

    DPRINTM(DPRINT_WINDOWS,"SystemRoot: '%s', SystemPath: '%s'\n", BootPath, SystemPath);

    /* Allocate and minimalistic-initialize LPB */
    AllocateAndInitLPB(&LoaderBlock);

    /* Allocate and initialize setup loader block */
    SetupBlock = MmHeapAlloc(sizeof(SETUP_LOADER_BLOCK));
    RtlZeroMemory(SetupBlock, sizeof(SETUP_LOADER_BLOCK));
    LoaderBlock->SetupLdrBlock = SetupBlock;

    /* Set textmode setup flag */
    SetupBlock->Flags = SETUPLDR_TEXT_MODE;

    /* Detect hardware */
    UseRealHeap = TRUE;
    LoaderBlock->ConfigurationRoot = MachHwDetect();

    /* Load kernel */
    strcpy(FileName, BootPath);
    strcat(FileName, "NTOSKRNL.EXE");
    Status = WinLdrLoadImage(FileName, LoaderSystemCode, &NtosBase);
    DPRINTM(DPRINT_WINDOWS, "Ntos loaded with status %d at %p\n", Status, NtosBase);

    /* Load HAL */
    strcpy(FileName, BootPath);
    strcat(FileName, "HAL.DLL");
    Status = WinLdrLoadImage(FileName, LoaderHalCode, &HalBase);
    DPRINTM(DPRINT_WINDOWS, "HAL loaded with status %d at %p\n", Status, HalBase);

    /* Load kernel-debugger support dll */
    strcpy(FileName, BootPath);
    strcat(FileName, "KDCOM.DLL");
    Status = WinLdrLoadImage(FileName, LoaderBootDriver, &KdComBase);
    DPRINTM(DPRINT_WINDOWS, "KdCom loaded with status %d at %p\n", Status, KdComBase);

    /* Allocate data table entries for above-loaded modules */
    WinLdrAllocateDataTableEntry(LoaderBlock, "ntoskrnl.exe",
        "NTOSKRNL.EXE", NtosBase, &KernelDTE);
    WinLdrAllocateDataTableEntry(LoaderBlock, "hal.dll",
        "HAL.DLL", HalBase, &HalDTE);
    WinLdrAllocateDataTableEntry(LoaderBlock, "kdcom.dll",
        "KDCOM.DLL", KdComBase, &KdComDTE);

    /* Load all referenced DLLs for kernel, HAL and kdcom.dll */
    strcpy(SearchPath, BootPath);
    WinLdrScanImportDescriptorTable(LoaderBlock, SearchPath, KernelDTE);
    WinLdrScanImportDescriptorTable(LoaderBlock, SearchPath, HalDTE);
    if (KdComDTE)
        WinLdrScanImportDescriptorTable(LoaderBlock, SearchPath, KdComDTE);

    /* Load NLS data */
    SetupLdrLoadNlsData(LoaderBlock, InfHandle, BootPath);

    /* Get a list of boot drivers */
    SetupLdrScanBootDrivers(LoaderBlock, InfHandle, BootPath);

    /* Load boot drivers */
    Status = WinLdrLoadBootDrivers(LoaderBlock, BootPath);
    DPRINTM(DPRINT_WINDOWS, "Boot drivers loaded with status %d\n", Status);

    /* Alloc PCR, TSS, do magic things with the GDT/IDT */
    WinLdrSetupForNt(LoaderBlock, &GdtIdt, &PcrBasePage, &TssBasePage);

    /* Initialize Phase 1 - no drivers loading anymore */
    LoadOptions[0] = 0;
    WinLdrInitializePhase1(LoaderBlock, LoadOptions, SystemPath, BootPath, _WIN32_WINNT_WS03);

    /* Save entry-point pointer and Loader block VAs */
    KiSystemStartup = (KERNEL_ENTRY_POINT)KernelDTE->EntryPoint;
    LoaderBlockVA = PaToVa(LoaderBlock);

    /* "Stop all motors", change videomode */
    MachPrepareForReactOS(FALSE);

    /* Debugging... */
    //DumpMemoryAllocMap();

    /* Turn on paging mode of CPU*/
    WinLdrTurnOnPaging(LoaderBlock, PcrBasePage, TssBasePage, GdtIdt);

    /* Save final value of LoaderPagesSpanned */
    LoaderBlock->Extension->LoaderPagesSpanned = LoaderPagesSpanned;

    DPRINTM(DPRINT_WINDOWS, "Hello from paged mode, KiSystemStartup %p, LoaderBlockVA %p!\n",
        KiSystemStartup, LoaderBlockVA);

    //WinLdrpDumpMemoryDescriptors(LoaderBlockVA);
    //WinLdrpDumpBootDriver(LoaderBlockVA);
    //WinLdrpDumpArcDisks(LoaderBlockVA);

    /*asm(".intel_syntax noprefix\n");
    asm("test1:\n");
    asm("jmp test1\n");
    asm(".att_syntax\n");*/

    /* Pass control */
    (*KiSystemStartup)(LoaderBlockVA);

    return;
}
