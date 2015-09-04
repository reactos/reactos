/*
 *  FreeLoader
 *
 *  Copyright (C) 1998-2003  Brian Palmer    <brianp@sginet.com>
 *  Copyright (C) 2006       Aleksey Bragin  <aleksey@reactos.org>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>
#include "registry.h"

#include <ndk/ldrtypes.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WINDOWS);

//FIXME: Find a better way to retrieve ARC disk information
extern ULONG reactos_disk_count;
extern ARC_DISK_SIGNATURE reactos_arc_disk_info[];

extern ULONG LoaderPagesSpanned;
extern BOOLEAN AcpiPresent;

extern HEADLESS_LOADER_BLOCK LoaderRedirectionInformation;
extern BOOLEAN WinLdrTerminalConnected;
extern void WinLdrSetupEms(IN PCHAR BootOptions);

PLOADER_SYSTEM_BLOCK WinLdrSystemBlock;

// debug stuff
VOID DumpMemoryAllocMap(VOID);

// Init "phase 0"
VOID
AllocateAndInitLPB(PLOADER_PARAMETER_BLOCK *OutLoaderBlock)
{
    PLOADER_PARAMETER_BLOCK LoaderBlock;

    /* Allocate and zero-init the LPB */
    WinLdrSystemBlock = MmAllocateMemoryWithType(sizeof(LOADER_SYSTEM_BLOCK),
                                                 LoaderSystemBlock);
    if (WinLdrSystemBlock == NULL)
    {
        UiMessageBox("Failed to allocate memory for system block!");
        return;
    }

    RtlZeroMemory(WinLdrSystemBlock, sizeof(LOADER_SYSTEM_BLOCK));

    LoaderBlock = &WinLdrSystemBlock->LoaderBlock;
    LoaderBlock->NlsData = &WinLdrSystemBlock->NlsDataBlock;

    /* Init three critical lists, used right away */
    InitializeListHead(&LoaderBlock->LoadOrderListHead);
    InitializeListHead(&LoaderBlock->MemoryDescriptorListHead);
    InitializeListHead(&LoaderBlock->BootDriverListHead);

    *OutLoaderBlock = LoaderBlock;
}

// Init "phase 1"
VOID
WinLdrInitializePhase1(PLOADER_PARAMETER_BLOCK LoaderBlock,
                       LPCSTR Options,
                       LPCSTR SystemRoot,
                       LPCSTR BootPath,
                       USHORT VersionToBoot)
{
    /* Examples of correct options and paths */
    //CHAR    Options[] = "/DEBUGPORT=COM1 /BAUDRATE=115200";
    //CHAR    Options[] = "/NODEBUG";
    //CHAR    SystemRoot[] = "\\WINNT\\";
    //CHAR    ArcBoot[] = "multi(0)disk(0)rdisk(0)partition(1)";

    LPSTR LoadOptions, NewLoadOptions;
    CHAR  HalPath[] = "\\";
    CHAR  ArcBoot[256];
    CHAR  MiscFiles[256];
    ULONG i;
    ULONG_PTR PathSeparator;
    PLOADER_PARAMETER_EXTENSION Extension;

    /* Construct SystemRoot and ArcBoot from SystemPath */
    PathSeparator = strstr(BootPath, "\\") - BootPath;
    strncpy(ArcBoot, BootPath, PathSeparator);
    ArcBoot[PathSeparator] = 0;

    TRACE("ArcBoot: %s\n", ArcBoot);
    TRACE("SystemRoot: %s\n", SystemRoot);
    TRACE("Options: %s\n", Options);

    /* Fill Arc BootDevice */
    LoaderBlock->ArcBootDeviceName = WinLdrSystemBlock->ArcBootDeviceName;
    strncpy(LoaderBlock->ArcBootDeviceName, ArcBoot, MAX_PATH);
    LoaderBlock->ArcBootDeviceName = PaToVa(LoaderBlock->ArcBootDeviceName);

    /* Fill Arc HalDevice, it matches ArcBoot path */
    LoaderBlock->ArcHalDeviceName = WinLdrSystemBlock->ArcBootDeviceName;
    LoaderBlock->ArcHalDeviceName = PaToVa(LoaderBlock->ArcHalDeviceName);

    /* Fill SystemRoot */
    LoaderBlock->NtBootPathName = WinLdrSystemBlock->NtBootPathName;
    strncpy(LoaderBlock->NtBootPathName, SystemRoot, MAX_PATH);
    LoaderBlock->NtBootPathName = PaToVa(LoaderBlock->NtBootPathName);

    /* Fill NtHalPathName */
    LoaderBlock->NtHalPathName = WinLdrSystemBlock->NtHalPathName;
    strncpy(LoaderBlock->NtHalPathName, HalPath, MAX_PATH);
    LoaderBlock->NtHalPathName = PaToVa(LoaderBlock->NtHalPathName);

    /* Fill LoadOptions and strip the '/' commutator symbol in front of each option */
    NewLoadOptions = LoadOptions = LoaderBlock->LoadOptions = WinLdrSystemBlock->LoadOptions;
    strncpy(LoaderBlock->LoadOptions, Options, MAX_OPTIONS_LENGTH);

    do
    {
        while (*LoadOptions == '/')
            ++LoadOptions;

        *NewLoadOptions++ = *LoadOptions;
    } while (*LoadOptions++);

    LoaderBlock->LoadOptions = PaToVa(LoaderBlock->LoadOptions);

    /* Arc devices */
    LoaderBlock->ArcDiskInformation = &WinLdrSystemBlock->ArcDiskInformation;
    InitializeListHead(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead);

    /* Convert ARC disk information from freeldr to a correct format */
    for (i = 0; i < reactos_disk_count; i++)
    {
        PARC_DISK_SIGNATURE_EX ArcDiskSig;

        /* Allocate the ARC structure */
        ArcDiskSig = FrLdrHeapAlloc(sizeof(ARC_DISK_SIGNATURE_EX), 'giSD');

        /* Copy the data over */
        ArcDiskSig->DiskSignature.Signature = reactos_arc_disk_info[i].Signature;
        ArcDiskSig->DiskSignature.CheckSum = reactos_arc_disk_info[i].CheckSum;

        /* Copy the ARC Name */
        strncpy(ArcDiskSig->ArcName, reactos_arc_disk_info[i].ArcName, MAX_PATH);
        ArcDiskSig->DiskSignature.ArcName = PaToVa(ArcDiskSig->ArcName);

        /* Mark partition table as valid */
        ArcDiskSig->DiskSignature.ValidPartitionTable = TRUE;

        /* Insert into the list */
        InsertTailList(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead,
                       &ArcDiskSig->DiskSignature.ListEntry);
    }

    /* Convert all list's to Virtual address */

    /* Convert the ArcDisks list to virtual address */
    List_PaToVa(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead);
    LoaderBlock->ArcDiskInformation = PaToVa(LoaderBlock->ArcDiskInformation);

    /* Convert configuration entries to VA */
    ConvertConfigToVA(LoaderBlock->ConfigurationRoot);
    LoaderBlock->ConfigurationRoot = PaToVa(LoaderBlock->ConfigurationRoot);

    /* Convert all DTE into virtual addresses */
    List_PaToVa(&LoaderBlock->LoadOrderListHead);

    /* this one will be converted right before switching to
       virtual paging mode */
    //List_PaToVa(&LoaderBlock->MemoryDescriptorListHead);

    /* Convert list of boot drivers */
    List_PaToVa(&LoaderBlock->BootDriverListHead);

    /* Initialize Extension now */
    Extension = &WinLdrSystemBlock->Extension;
    Extension->Size = sizeof(LOADER_PARAMETER_EXTENSION);
    Extension->MajorVersion = (VersionToBoot & 0xFF00) >> 8;
    Extension->MinorVersion = VersionToBoot & 0xFF;
    Extension->Profile.Status = 2;

    /* Check if ACPI is present */
    if (AcpiPresent)
    {
        /* See KiRosFrldrLpbToNtLpb for details */
        Extension->AcpiTable = (PVOID)1;
    }

#ifdef _M_IX86
    /* Set headless block pointer */
    if (WinLdrTerminalConnected)
    {
        Extension->HeadlessLoaderBlock = &WinLdrSystemBlock->HeadlessLoaderBlock;
        RtlCopyMemory(Extension->HeadlessLoaderBlock,
                      &LoaderRedirectionInformation,
                      sizeof(HEADLESS_LOADER_BLOCK));
        Extension->HeadlessLoaderBlock = PaToVa(Extension->HeadlessLoaderBlock);
    }
#endif
    /* Load drivers database */
    strcpy(MiscFiles, BootPath);
    strcat(MiscFiles, "AppPatch\\drvmain.sdb");
    Extension->DrvDBImage = PaToVa(WinLdrLoadModule(MiscFiles,
                                                    &Extension->DrvDBSize,
                                                    LoaderRegistryData));

    /* Convert extension and setup block pointers */
    LoaderBlock->Extension = PaToVa(Extension);

    if (LoaderBlock->SetupLdrBlock)
        LoaderBlock->SetupLdrBlock = PaToVa(LoaderBlock->SetupLdrBlock);

    TRACE("WinLdrInitializePhase1() completed\n");
}

static BOOLEAN
WinLdrLoadDeviceDriver(PLIST_ENTRY LoadOrderListHead,
                       LPCSTR BootPath,
                       PUNICODE_STRING FilePath,
                       ULONG Flags,
                       PLDR_DATA_TABLE_ENTRY *DriverDTE)
{
    CHAR FullPath[1024];
    CHAR DriverPath[1024];
    CHAR DllName[1024];
    PCHAR DriverNamePos;
    BOOLEAN Success;
    PVOID DriverBase = NULL;

    // Separate the path to file name and directory path
    _snprintf(DriverPath, sizeof(DriverPath), "%wZ", FilePath);
    DriverNamePos = strrchr(DriverPath, '\\');
    if (DriverNamePos != NULL)
    {
        // Copy the name
        strcpy(DllName, DriverNamePos+1);

        // Cut out the name from the path
        *(DriverNamePos+1) = 0;
    }
    else
    {
        // There is no directory in the path
        strcpy(DllName, DriverPath);
        DriverPath[0] = 0;
    }

    TRACE("DriverPath: %s, DllName: %s, LPB\n", DriverPath, DllName);

    // Check if driver is already loaded
    Success = WinLdrCheckForLoadedDll(LoadOrderListHead, DllName, DriverDTE);
    if (Success)
    {
        // We've got the pointer to its DTE, just return success
        return TRUE;
    }

    // It's not loaded, we have to load it
    _snprintf(FullPath, sizeof(FullPath), "%s%wZ", BootPath, FilePath);
    Success = WinLdrLoadImage(FullPath, LoaderBootDriver, &DriverBase);
    if (!Success)
        return FALSE;

    // Allocate a DTE for it
    Success = WinLdrAllocateDataTableEntry(LoadOrderListHead, DllName, DllName, DriverBase, DriverDTE);
    if (!Success)
    {
        ERR("WinLdrAllocateDataTableEntry() failed\n");
        return FALSE;
    }

    // Modify any flags, if needed
    (*DriverDTE)->Flags |= Flags;

    // Look for any dependencies it may have, and load them too
    sprintf(FullPath,"%s%s", BootPath, DriverPath);
    Success = WinLdrScanImportDescriptorTable(LoadOrderListHead, FullPath, *DriverDTE);
    if (!Success)
    {
        ERR("WinLdrScanImportDescriptorTable() failed for %s\n", FullPath);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
WinLdrLoadBootDrivers(PLOADER_PARAMETER_BLOCK LoaderBlock,
                      LPCSTR BootPath)
{
    PLIST_ENTRY NextBd;
    PBOOT_DRIVER_LIST_ENTRY BootDriver;
    BOOLEAN Success;
    BOOLEAN ret = TRUE;

    // Walk through the boot drivers list
    NextBd = LoaderBlock->BootDriverListHead.Flink;

    while (NextBd != &LoaderBlock->BootDriverListHead)
    {
        BootDriver = CONTAINING_RECORD(NextBd, BOOT_DRIVER_LIST_ENTRY, Link);

        TRACE("BootDriver %wZ DTE %08X RegPath: %wZ\n", &BootDriver->FilePath,
            BootDriver->LdrEntry, &BootDriver->RegistryPath);

        // Paths are relative (FIXME: Are they always relative?)

        // Load it
        Success = WinLdrLoadDeviceDriver(&LoaderBlock->LoadOrderListHead,
                                         BootPath,
                                         &BootDriver->FilePath,
                                         0,
                                         &BootDriver->LdrEntry);

        if (Success)
        {
            // Convert the RegistryPath and DTE addresses to VA since we are not going to use it anymore
            BootDriver->RegistryPath.Buffer = PaToVa(BootDriver->RegistryPath.Buffer);
            BootDriver->FilePath.Buffer = PaToVa(BootDriver->FilePath.Buffer);
            BootDriver->LdrEntry = PaToVa(BootDriver->LdrEntry);
        }
        else
        {
            // Loading failed - cry loudly
            ERR("Can't load boot driver '%wZ'!\n", &BootDriver->FilePath);
            UiMessageBox("Can't load boot driver '%wZ'!", &BootDriver->FilePath);
            ret = FALSE;

            // Remove it from the list and try to continue
            RemoveEntryList(NextBd);
        }

        NextBd = BootDriver->Link.Flink;
    }

    return ret;
}

PVOID
WinLdrLoadModule(PCSTR ModuleName,
                 ULONG *Size,
                 TYPE_OF_MEMORY MemoryType)
{
    ULONG FileId;
    PVOID PhysicalBase;
    FILEINFORMATION FileInfo;
    ULONG FileSize;
    ARC_STATUS Status;
    ULONG BytesRead;

    //CHAR ProgressString[256];

    /* Inform user we are loading files */
    //sprintf(ProgressString, "Loading %s...", FileName);
    //UiDrawProgressBarCenter(1, 100, ProgressString);

    TRACE("Loading module %s\n", ModuleName);
    *Size = 0;

    /* Open the image file */
    Status = ArcOpen((PCHAR)ModuleName, OpenReadOnly, &FileId);
    if (Status != ESUCCESS)
    {
        /* In case of errors, we just return, without complaining to the user */
        return NULL;
    }

    /* Get this file's size */
    Status = ArcGetFileInformation(FileId, &FileInfo);
    if (Status != ESUCCESS)
    {
        ArcClose(FileId);
        return NULL;
    }
    FileSize = FileInfo.EndingAddress.LowPart;
    *Size = FileSize;

    /* Allocate memory */
    PhysicalBase = MmAllocateMemoryWithType(FileSize, MemoryType);
    if (PhysicalBase == NULL)
    {
        ArcClose(FileId);
        return NULL;
    }

    /* Load whole file */
    Status = ArcRead(FileId, PhysicalBase, FileSize, &BytesRead);
    ArcClose(FileId);
    if (Status != ESUCCESS)
    {
        return NULL;
    }

    TRACE("Loaded %s at 0x%x with size 0x%x\n", ModuleName, PhysicalBase, FileSize);

    return PhysicalBase;
}

USHORT
WinLdrDetectVersion(VOID)
{
    LONG rc;
    FRLDRHKEY hKey;

    rc = RegOpenKey(
        NULL,
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Terminal Server",
        &hKey);
    if (rc != ERROR_SUCCESS)
    {
        // Key doesn't exist; assume NT 4.0
        return _WIN32_WINNT_NT4;
    }

    // We may here want to read the value of ProductVersion
    return _WIN32_WINNT_WS03;
}

static
BOOLEAN
LoadModule(
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    PCCH Path,
    PCCH File,
    TYPE_OF_MEMORY MemoryType,
    PLDR_DATA_TABLE_ENTRY *Dte,
    BOOLEAN IsKdTransportDll,
    ULONG Percentage)
{
    BOOLEAN Success;
    CHAR FullFileName[MAX_PATH];
    CHAR ProgressString[256];
    PVOID BaseAdress = NULL;

    UiDrawBackdrop();
    sprintf(ProgressString, "Loading %s...", File);
    UiDrawProgressBarCenter(Percentage, 100, ProgressString);

    strcpy(FullFileName, Path);
    strcat(FullFileName, "SYSTEM32\\");
    strcat(FullFileName, File);

    Success = WinLdrLoadImage(FullFileName, MemoryType, &BaseAdress);
    if (!Success)
    {
        TRACE("Loading %s failed\n", File);
        return FALSE;
    }
    TRACE("%s loaded successfully at %p\n", File, BaseAdress);

    strcpy(FullFileName, "WINDOWS\\SYSTEM32\\");
    strcat(FullFileName, File);
    /*
     * Cheat about the base DLL name if we are loading
     * the Kernel Debugger Transport DLL, to make the
     * PE loader happy.
     */
    Success = WinLdrAllocateDataTableEntry(&LoaderBlock->LoadOrderListHead,
                                           (IsKdTransportDll ? "KDCOM.DLL" : File),
                                           FullFileName,
                                           BaseAdress,
                                           Dte);

    return Success;
}

static
BOOLEAN
LoadWindowsCore(IN USHORT OperatingSystemVersion,
                IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                IN LPCSTR BootOptions,
                IN LPCSTR BootPath,
                IN OUT PLDR_DATA_TABLE_ENTRY* KernelDTE)
{
    BOOLEAN Success;
    CHAR DirPath[MAX_PATH];
    CHAR KdTransportDllName[MAX_PATH];
    PLDR_DATA_TABLE_ENTRY HalDTE, KdComDTE = NULL;

    if (!KernelDTE) return FALSE;

    /* Load the Kernel */
    LoadModule(LoaderBlock, BootPath, "NTOSKRNL.EXE", LoaderSystemCode, KernelDTE, FALSE, 30);

    /* Load the HAL */
    LoadModule(LoaderBlock, BootPath, "HAL.DLL", LoaderHalCode, &HalDTE, FALSE, 45);

    /* Load the Kernel Debugger Transport DLL */
    if (OperatingSystemVersion > _WIN32_WINNT_WIN2K)
    {
        /*
         * According to http://www.nynaeve.net/?p=173 :
         * "[...] Another enhancement that could be done Microsoft-side would be
         * a better interface for replacing KD transport modules. Right now, due
         * to the fact that ntoskrnl is static linked to KDCOM.DLL, the OS loader
         * has a hardcoded hack that interprets the KD type in the OS loader options,
         * loads one of the (hardcoded filenames) "kdcom.dll", "kd1394.dll", or
         * "kdusb2.dll" modules, and inserts them into the loaded module list under
         * the name "kdcom.dll". [...]"
         */

        /*
         * This loop replaces a dumb call to strstr(..., "DEBUGPORT=").
         * Indeed I want it to be case-insensitive to allow "debugport="
         * or "DeBuGpOrT=" or... , and I don't want it to match malformed
         * command-line options, such as:
         *
         * "...foo DEBUGPORT=xxx bar..."
         * "...foo/DEBUGPORT=xxx bar..."
         * "...foo/DEBUGPORT=bar..."
         *
         * i.e. the "DEBUGPORT=" switch must start with a slash and be separated
         * from the rest by whitespace, unless it begins the command-line, e.g.:
         *
         * "/DEBUGPORT=COM1 foo...bar..."
         * "...foo /DEBUGPORT=USB bar..."
         * or:
         * "...foo /DEBUGPORT= bar..."
         * (in that case, we default the port to COM).
         */
        while (BootOptions)
        {
            /* Skip possible initial whitespace */
            BootOptions += strspn(BootOptions, " \t");

            /* Check whether a new commutator starts and it is the DEBUGPORT one */
            if (*BootOptions != '/' || _strnicmp(++BootOptions, "DEBUGPORT=", 10) != 0)
            {
                /* Search for another whitespace */
                BootOptions = strpbrk(BootOptions, " \t");
                continue;
            }
            else
            {
                /* We found the DEBUGPORT commutator. Move to the port name. */
                BootOptions += 10;
                break;
            }
        }

        if (BootOptions)
        {
            /*
             * We have found the DEBUGPORT commutator. Parse the port name.
             * Format: /DEBUGPORT=COM1 or /DEBUGPORT=FILE:\Device\HarddiskX\PartitionY\debug.log or /DEBUGPORT=FOO
             * If we only have /DEBUGPORT= (i.e. without any port name), defaults it to "COM".
             */
            strcpy(KdTransportDllName, "KD");
            if (_strnicmp(BootOptions, "COM", 3) == 0 && '0' <= BootOptions[3] && BootOptions[3] <= '9')
            {
                strncat(KdTransportDllName, BootOptions, 3);
            }
            else
            {
                size_t i = strcspn(BootOptions, " \t:"); /* Skip valid separators: whitespace or colon */
                if (i == 0)
                    strcat(KdTransportDllName, "COM");
                else
                    strncat(KdTransportDllName, BootOptions, i);
            }
            strcat(KdTransportDllName, ".DLL");
            _strupr(KdTransportDllName);

            /*
             * Load the transport DLL. Specify it to LoadModule so that it can
             * change the base DLL name of the loaded transport DLL to the default
             * "KDCOM.DLL" name, to make the PE loader happy.
             */
            LoadModule(LoaderBlock, BootPath, KdTransportDllName, LoaderSystemCode, &KdComDTE, TRUE, 60);
        }
    }

    /* Load all referenced DLLs for Kernel, HAL and Kernel Debugger Transport DLL */
    strcpy(DirPath, BootPath);
    strcat(DirPath, "system32\\");
    Success  = WinLdrScanImportDescriptorTable(&LoaderBlock->LoadOrderListHead, DirPath, *KernelDTE);
    Success &= WinLdrScanImportDescriptorTable(&LoaderBlock->LoadOrderListHead, DirPath, HalDTE);
    if (KdComDTE)
    {
        Success &= WinLdrScanImportDescriptorTable(&LoaderBlock->LoadOrderListHead, DirPath, KdComDTE);
    }

    return Success;
}

VOID
LoadAndBootWindows(IN OperatingSystemItem* OperatingSystem,
                   IN USHORT OperatingSystemVersion)
{
    ULONG_PTR SectionId;
    PCSTR SectionName = OperatingSystem->SystemPartition;
    CHAR  SettingsValue[80];
    BOOLEAN HasSection;
    CHAR  BootPath[MAX_PATH];
    CHAR  FileName[MAX_PATH];
    CHAR  BootOptions[256];
    PCHAR File;
    BOOLEAN Success;
    PLOADER_PARAMETER_BLOCK LoaderBlock;

    /* Get OS setting value */
    SettingsValue[0] = ANSI_NULL;
    IniOpenSection("Operating Systems", &SectionId);
    IniReadSettingByName(SectionId, SectionName, SettingsValue, sizeof(SettingsValue));

    /* Open the operating system section specified in the .ini file */
    HasSection = IniOpenSection(SectionName, &SectionId);

    UiDrawBackdrop();
    UiDrawProgressBarCenter(1, 100, "Loading NT...");

    /* Read the system path is set in the .ini file */
    if (!HasSection ||
        !IniReadSettingByName(SectionId, "SystemPath", BootPath, sizeof(BootPath)))
    {
        strcpy(BootPath, SectionName);
    }

    /*
     * Check whether BootPath is a full path
     * and if not, create a full boot path.
     *
     * See FsOpenFile for the technique used.
     */
    if (strrchr(BootPath, ')') == NULL)
    {
        /* Temporarily save the boot path */
        strcpy(FileName, BootPath);

        /* This is not a full path. Use the current (i.e. boot) device. */
        MachDiskGetBootPath(BootPath, sizeof(BootPath));

        /* Append a path separator if needed */
        if (FileName[0] != '\\' && FileName[0] != '/')
            strcat(BootPath, "\\");

        /* Append the remaining path */
        strcat(BootPath, FileName);
    }

    /* Append a backslash if needed */
    if ((strlen(BootPath) == 0) || BootPath[strlen(BootPath) - 1] != '\\')
        strcat(BootPath, "\\");

    /* Read booting options */
    if (!HasSection || !IniReadSettingByName(SectionId, "Options", BootOptions, sizeof(BootOptions)))
    {
        /* Get options after the title */
        PCSTR p = SettingsValue;
        while (*p == ' ' || *p == '"')
            p++;
        while (*p != '\0' && *p != '"')
            p++;
        strcpy(BootOptions, p);
        TRACE("BootOptions: '%s'\n", BootOptions);
    }

    /* Append boot-time options */
    AppendBootTimeOptions(BootOptions);

    /* Check if a ramdisk file was given */
    File = strstr(BootOptions, "/RDPATH=");
    if (File)
    {
        /* Copy the file name and everything else after it */
        strcpy(FileName, File + 8);

        /* Null-terminate */
        *strstr(FileName, " ") = ANSI_NULL;

        /* Load the ramdisk */
        if (!RamDiskLoadVirtualFile(FileName))
        {
            UiMessageBox("Failed to load RAM disk file %s", FileName);
            return;
        }
    }

    /* Let user know we started loading */
    //UiDrawStatusText("Loading...");

    TRACE("BootPath: '%s'\n", BootPath);

    /* Allocate and minimalistic-initialize LPB */
    AllocateAndInitLPB(&LoaderBlock);

    /* Load Hive */
    UiDrawBackdrop();
    UiDrawProgressBarCenter(15, 100, "Loading system hive...");
    Success = WinLdrInitSystemHive(LoaderBlock, BootPath);
    TRACE("SYSTEM hive %s\n", (Success ? "loaded" : "not loaded"));

    /* Load NLS data, OEM font, and prepare boot drivers list */
    Success = WinLdrScanSystemHive(LoaderBlock, BootPath);
    TRACE("SYSTEM hive %s\n", (Success ? "scanned" : "not scanned"));

    /* Finish loading */
    LoadAndBootWindowsCommon(OperatingSystemVersion,
                             LoaderBlock,
                             BootOptions,
                             BootPath,
                             FALSE);
}

VOID
LoadAndBootWindowsCommon(
    USHORT OperatingSystemVersion,
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    LPCSTR BootOptions,
    LPCSTR BootPath,
    BOOLEAN Setup)
{
    PLOADER_PARAMETER_BLOCK LoaderBlockVA;
    BOOLEAN Success;
    PLDR_DATA_TABLE_ENTRY KernelDTE;
    KERNEL_ENTRY_POINT KiSystemStartup;
    LPCSTR SystemRoot;
    TRACE("LoadAndBootWindowsCommon()\n");

#ifdef _M_IX86
    /* Setup redirection support */
    WinLdrSetupEms((PCHAR)BootOptions);
#endif

    /* Convert BootPath to SystemRoot */
    SystemRoot = strstr(BootPath, "\\");

    /* Detect hardware */
    LoaderBlock->ConfigurationRoot = MachHwDetect();

    if (OperatingSystemVersion == 0)
        OperatingSystemVersion = WinLdrDetectVersion();

    /* Load the operating system core: the Kernel, the HAL and the Kernel Debugger Transport DLL */
    Success = LoadWindowsCore(OperatingSystemVersion,
                              LoaderBlock,
                              BootOptions,
                              BootPath,
                              &KernelDTE);
    if (!Success)
    {
        UiMessageBox("Error loading NTOS core.");
        return;
    }

    /* Load boot drivers */
    UiDrawBackdrop();
    UiDrawProgressBarCenter(100, 100, "Loading boot drivers...");
    Success = WinLdrLoadBootDrivers(LoaderBlock, BootPath);
    TRACE("Boot drivers loading %s\n", Success ? "successful" : "failed");

    /* Initialize Phase 1 - no drivers loading anymore */
    WinLdrInitializePhase1(LoaderBlock,
                           BootOptions,
                           SystemRoot,
                           BootPath,
                           OperatingSystemVersion);

    /* Save entry-point pointer and Loader block VAs */
    KiSystemStartup = (KERNEL_ENTRY_POINT)KernelDTE->EntryPoint;
    LoaderBlockVA = PaToVa(LoaderBlock);

    /* "Stop all motors", change videomode */
    MachPrepareForReactOS(Setup);

    /* Cleanup ini file */
    IniCleanup();

    /* Debugging... */
    //DumpMemoryAllocMap();

    /* Do the machine specific initialization */
    WinLdrSetupMachineDependent(LoaderBlock);

    /* Map pages and create memory descriptors */
    WinLdrSetupMemoryLayout(LoaderBlock);

    /* Set processor context */
    WinLdrSetProcessorContext();

    /* Save final value of LoaderPagesSpanned */
    LoaderBlock->Extension->LoaderPagesSpanned = LoaderPagesSpanned;

    TRACE("Hello from paged mode, KiSystemStartup %p, LoaderBlockVA %p!\n",
          KiSystemStartup, LoaderBlockVA);

    // Zero KI_USER_SHARED_DATA page
    memset((PVOID)KI_USER_SHARED_DATA, 0, MM_PAGE_SIZE);

    WinLdrpDumpMemoryDescriptors(LoaderBlockVA);
    WinLdrpDumpBootDriver(LoaderBlockVA);
#ifndef _M_AMD64
    WinLdrpDumpArcDisks(LoaderBlockVA);
#endif

    /* Pass control */
    (*KiSystemStartup)(LoaderBlockVA);
}

VOID
WinLdrpDumpMemoryDescriptors(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead)
    {
        MemoryDescriptor = CONTAINING_RECORD(NextMd, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);

        TRACE("BP %08X PC %04X MT %d\n", MemoryDescriptor->BasePage,
            MemoryDescriptor->PageCount, MemoryDescriptor->MemoryType);

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }
}

VOID
WinLdrpDumpBootDriver(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextBd;
    PBOOT_DRIVER_LIST_ENTRY BootDriver;

    NextBd = LoaderBlock->BootDriverListHead.Flink;

    while (NextBd != &LoaderBlock->BootDriverListHead)
    {
        BootDriver = CONTAINING_RECORD(NextBd, BOOT_DRIVER_LIST_ENTRY, Link);

        TRACE("BootDriver %wZ DTE %08X RegPath: %wZ\n", &BootDriver->FilePath,
            BootDriver->LdrEntry, &BootDriver->RegistryPath);

        NextBd = BootDriver->Link.Flink;
    }
}

VOID
WinLdrpDumpArcDisks(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextBd;
    PARC_DISK_SIGNATURE ArcDisk;

    NextBd = LoaderBlock->ArcDiskInformation->DiskSignatureListHead.Flink;

    while (NextBd != &LoaderBlock->ArcDiskInformation->DiskSignatureListHead)
    {
        ArcDisk = CONTAINING_RECORD(NextBd, ARC_DISK_SIGNATURE, ListEntry);

        TRACE("ArcDisk %s checksum: 0x%X, signature: 0x%X\n",
            ArcDisk->ArcName, ArcDisk->CheckSum, ArcDisk->Signature);

        NextBd = ArcDisk->ListEntry.Flink;
    }
}
