/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Windows-compatible NT OS Loader.
 * COPYRIGHT:   Copyright 2006-2019 Aleksey Bragin <aleksey@reactos.org>
 */

#include <freeldr.h>
#include <ndk/kefuncs.h> // For KeFindConfiguration*Entry()
#include <ndk/ldrtypes.h>
#include "winldr.h"
#include "ntldropts.h"
#include "registry.h"
#include <internal/cmboot.h>

#include <drivers/bootvid/framebuf.h> // For CM_FRAMEBUF_DEVICE_DATA

#include <debug.h>
DBG_DEFAULT_CHANNEL(WINDOWS);

ULONG ArcGetDiskCount(VOID);
PARC_DISK_SIGNATURE_EX ArcGetDiskInfo(ULONG Index);

BOOLEAN IsAcpiPresent(VOID);

extern HEADLESS_LOADER_BLOCK LoaderRedirectionInformation;
extern BOOLEAN WinLdrTerminalConnected;
extern VOID WinLdrSetupEms(_In_ PCSTR BootOptions);

PLOADER_SYSTEM_BLOCK WinLdrSystemBlock;
/**/PCWSTR BootFileSystem = NULL;/**/

BOOLEAN VirtualBias = FALSE;
BOOLEAN SosEnabled = FALSE;
BOOLEAN SafeBoot = FALSE;
BOOLEAN BootLogo = FALSE;
#ifdef _M_IX86
BOOLEAN PaeModeOn = FALSE;
#endif
BOOLEAN NoExecuteEnabled = FALSE;

// debug stuff
VOID DumpMemoryAllocMap(VOID);

/* PE loader import-DLL loading callback */
static VOID
NTAPI
NtLdrImportDllLoadCallback(
    _In_ PCSTR FileName)
{
    NtLdrOutputLoadMsg(FileName, NULL);
}

VOID
NtLdrOutputLoadMsg(
    _In_ PCSTR FileName,
    _In_opt_ PCSTR Description)
{
    if (SosEnabled)
    {
        printf("  %s\n", FileName);
        TRACE("Loading: %s\n", FileName);
    }
    else
    {
        /* Inform the user we load a file */
        CHAR ProgressString[256];

        RtlStringCbPrintfA(ProgressString, sizeof(ProgressString),
                           "Loading %s...",
                           (Description ? Description : FileName));
        // UiSetProgressBarText(ProgressString);
        // UiIndicateProgress();
        UiDrawStatusText(ProgressString);
    }
}

// Init "phase 0"
VOID
AllocateAndInitLPB(
    IN USHORT VersionToBoot,
    OUT PLOADER_PARAMETER_BLOCK* OutLoaderBlock)
{
    PLOADER_PARAMETER_BLOCK LoaderBlock;
    PLOADER_PARAMETER_EXTENSION Extension;

    /* Allocate and zero-init the Loader Parameter Block */
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

    /* Initialize the Loader Block Extension */
    Extension = &WinLdrSystemBlock->Extension;
    LoaderBlock->Extension = Extension;
    Extension->Size = sizeof(LOADER_PARAMETER_EXTENSION);
    Extension->MajorVersion = (VersionToBoot & 0xFF00) >> 8;
    Extension->MinorVersion = (VersionToBoot & 0xFF);

#ifdef UEFIBOOT
    Extension->BootViaEFI = 1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    LoaderBlock->FirmwareInformation.FirmwareTypeEfi = 1;
#endif
#endif

    /* Init three critical lists, used right away */
    InitializeListHead(&LoaderBlock->LoadOrderListHead);
    InitializeListHead(&LoaderBlock->MemoryDescriptorListHead);
    InitializeListHead(&LoaderBlock->BootDriverListHead);

    *OutLoaderBlock = LoaderBlock;
}

// Init "phase 1"
static VOID
WinLdrInitializePhase1(
    _In_ USHORT OperatingSystemVersion,
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ PCSTR BootOptions,
    _In_ PCSTR SystemPartition,
    _In_ PCSTR BootPath)
{
    /*
     * Examples of correct options and paths:
     * CHAR BootOptions[] = "DEBUGPORT=COM1 BAUDRATE=115200";
     * CHAR SystemPartition[] = "multi(0)disk(0)rdisk(0)partition(1)";
     * CHAR BootPath[] = "multi(0)disk(0)rdisk(0)partition(2)\\ReactOS\\";
     * --> ArcBootDevice = "multi(0)disk(0)rdisk(0)partition(2)"
     *     SystemRoot = "\\ReactOS\\"
     */
    CHAR  HalPath[] = "\\";
    CHAR  FilePath[MAX_PATH+1];
    ULONG i;
    PLOADER_PARAMETER_EXTENSION Extension;

    /* Convert BootPath to SystemRoot. Attempt to handle paths like:
     * "multi(0)disk(0)rdisk(0)partition(2)ReactOS\\weird)name"
     * where SystemRoot would be: "ReactOS\\weird)name" */
    PCSTR SystemRoot = BootPath + strcspn(BootPath, "\\");
    PCSTR LastParen;

    CHAR Sep = *SystemRoot;
    *(PSTR)SystemRoot = ANSI_NULL;
    LastParen = strrchr(BootPath, ')');
    *(PSTR)SystemRoot = Sep;
    if (LastParen && (LastParen < SystemRoot))
        SystemRoot = (LastParen + 1);

    TRACE("SystemPartition: '%s'\n", SystemPartition);
    TRACE("ArcBootDevice:   '%.*s'\n", (SystemRoot - BootPath), BootPath);
    TRACE("SystemRoot:      '%s'\n", SystemRoot);
    TRACE("BootOptions:     '%s'\n", BootOptions);

    /* Fill the OS boot partition ARC device path */
    LoaderBlock->ArcBootDeviceName = WinLdrSystemBlock->ArcBootDeviceName;
    RtlStringCbCopyNA(LoaderBlock->ArcBootDeviceName, sizeof(WinLdrSystemBlock->ArcBootDeviceName),
                      BootPath, (SystemRoot - BootPath) * sizeof(CHAR));
    LoaderBlock->ArcBootDeviceName = PaToVa(LoaderBlock->ArcBootDeviceName);

//
// IMPROVE!!
// SetupBlock->ArcSetupDeviceName must be the path to the setup **SOURCE**,
// and not the setup boot path. Indeed they may differ!!
//
    if (LoaderBlock->SetupLdrBlock)
    {
        PSETUP_LOADER_BLOCK SetupBlock = LoaderBlock->SetupLdrBlock;

        /* Adjust the ARC path in the setup block - Matches ArcBootDeviceName */
        SetupBlock->ArcSetupDeviceName = WinLdrSystemBlock->ArcBootDeviceName;
        SetupBlock->ArcSetupDeviceName = PaToVa(SetupBlock->ArcSetupDeviceName);

        /* Convert the setup block pointer */
        LoaderBlock->SetupLdrBlock = PaToVa(LoaderBlock->SetupLdrBlock);
    }

    /* Fill the firmware system loader / "HAL" partition ARC device path */
    LoaderBlock->ArcHalDeviceName = WinLdrSystemBlock->ArcHalDeviceName;
    RtlStringCbCopyA(LoaderBlock->ArcHalDeviceName, sizeof(WinLdrSystemBlock->ArcHalDeviceName), SystemPartition);
    LoaderBlock->ArcHalDeviceName = PaToVa(LoaderBlock->ArcHalDeviceName);

    /* Fill SystemRoot */
    LoaderBlock->NtBootPathName = WinLdrSystemBlock->NtBootPathName;
    RtlStringCbCopyA(LoaderBlock->NtBootPathName, sizeof(WinLdrSystemBlock->NtBootPathName), SystemRoot);
    LoaderBlock->NtBootPathName = PaToVa(LoaderBlock->NtBootPathName);

    /* Fill NtHalPathName */
    LoaderBlock->NtHalPathName = WinLdrSystemBlock->NtHalPathName;
    RtlStringCbCopyA(LoaderBlock->NtHalPathName, sizeof(WinLdrSystemBlock->NtHalPathName), HalPath);
    LoaderBlock->NtHalPathName = PaToVa(LoaderBlock->NtHalPathName);

    /* Fill LoadOptions */
    LoaderBlock->LoadOptions = WinLdrSystemBlock->LoadOptions;
    RtlStringCbCopyA(LoaderBlock->LoadOptions, sizeof(WinLdrSystemBlock->LoadOptions), BootOptions);
    LoaderBlock->LoadOptions = PaToVa(LoaderBlock->LoadOptions);

    /* ARC devices */
    LoaderBlock->ArcDiskInformation = &WinLdrSystemBlock->ArcDiskInformation;
    InitializeListHead(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead);

    /* Convert ARC disk information from freeldr to a correct format */
    ULONG DiscCount = ArcGetDiskCount();
    for (i = 0; i < DiscCount; i++)
    {
        PARC_DISK_SIGNATURE_EX ArcDiskSig;

        /* Allocate the ARC structure */
        ArcDiskSig = FrLdrHeapAlloc(sizeof(ARC_DISK_SIGNATURE_EX), 'giSD');
        if (!ArcDiskSig)
        {
            ERR("Failed to allocate ARC structure! Ignoring remaining ARC disks. (i = %lu, DiskCount = %lu)\n",
                i, DiscCount);
            break;
        }

        /* Copy the data over */
        RtlCopyMemory(ArcDiskSig, ArcGetDiskInfo(i), sizeof(ARC_DISK_SIGNATURE_EX));

        /* Set the ARC Name pointer */
        ArcDiskSig->DiskSignature.ArcName = PaToVa(ArcDiskSig->ArcName);

        /* Insert into the list */
        InsertTailList(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead,
                       &ArcDiskSig->DiskSignature.ListEntry);
    }

    /* Convert all lists to Virtual address */

    /* Convert the ArcDisks list to virtual address */
    List_PaToVa(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead);
    LoaderBlock->ArcDiskInformation = PaToVa(LoaderBlock->ArcDiskInformation);

    /* Convert configuration entries to VA */
    ConvertConfigToVA(LoaderBlock->ConfigurationRoot);
    LoaderBlock->ConfigurationRoot = PaToVa(LoaderBlock->ConfigurationRoot);

    /* Convert all DTE into virtual addresses */
    List_PaToVa(&LoaderBlock->LoadOrderListHead);

    /* This one will be converted right before switching to virtual paging mode */
    //List_PaToVa(&LoaderBlock->MemoryDescriptorListHead);

    /* Convert list of boot drivers */
    List_PaToVa(&LoaderBlock->BootDriverListHead);

    Extension = LoaderBlock->Extension;

    /* FIXME! HACK value for docking profile */
    Extension->Profile.Status = 2;

    /* Check if FreeLdr detected a ACPI table */
    if (IsAcpiPresent())
    {
        /* Set the pointer to something for compatibility */
        Extension->AcpiTable = (PVOID)1;
        // FIXME: Extension->AcpiTableSize;
    }

    if (OperatingSystemVersion >= _WIN32_WINNT_VISTA)
    {
        Extension->BootViaWinload = 1;
        Extension->LoaderPerformanceData = PaToVa(&WinLdrSystemBlock->LoaderPerformanceData);

        InitializeListHead(&Extension->BootApplicationPersistentData);
        List_PaToVa(&Extension->BootApplicationPersistentData);
    }

    /* Set the headless block pointer */
    if (WinLdrTerminalConnected)
    {
        Extension->HeadlessLoaderBlock = &WinLdrSystemBlock->HeadlessLoaderBlock;
        RtlCopyMemory(Extension->HeadlessLoaderBlock,
                      &LoaderRedirectionInformation,
                      sizeof(LoaderRedirectionInformation));
        Extension->HeadlessLoaderBlock = PaToVa(Extension->HeadlessLoaderBlock);
    }

    /* Load drivers database */
    RtlStringCbCopyA(FilePath, sizeof(FilePath), BootPath);
    RtlStringCbCatA(FilePath, sizeof(FilePath), "AppPatch\\drvmain.sdb");
    Extension->DrvDBImage = PaToVa(WinLdrLoadModule(FilePath,
                                                    &Extension->DrvDBSize,
                                                    LoaderRegistryData));

    /* Convert the extension block pointer */
    LoaderBlock->Extension = PaToVa(LoaderBlock->Extension);

    TRACE("WinLdrInitializePhase1() completed\n");
}

static BOOLEAN
WinLdrLoadDeviceDriver(PLIST_ENTRY LoadOrderListHead,
                       PCSTR BootPath,
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
    RtlStringCbPrintfA(DriverPath, sizeof(DriverPath), "%wZ", FilePath);
    DriverNamePos = strrchr(DriverPath, '\\');
    if (DriverNamePos != NULL)
    {
        // Copy the name
        RtlStringCbCopyA(DllName, sizeof(DllName), DriverNamePos+1);

        // Cut out the name from the path
        *(DriverNamePos+1) = ANSI_NULL;
    }
    else
    {
        // There is no directory in the path
        RtlStringCbCopyA(DllName, sizeof(DllName), DriverPath);
        *DriverPath = ANSI_NULL;
    }

    TRACE("DriverPath: '%s', DllName: '%s', LPB\n", DriverPath, DllName);

    // Check if driver is already loaded
    Success = PeLdrCheckForLoadedDll(LoadOrderListHead, DllName, DriverDTE);
    if (Success)
    {
        // We've got the pointer to its DTE, just return success
        return TRUE;
    }

    // It's not loaded, we have to load it
    RtlStringCbPrintfA(FullPath, sizeof(FullPath), "%s%wZ", BootPath, FilePath);

    NtLdrOutputLoadMsg(FullPath, NULL);
    Success = PeLdrLoadImage(FullPath, LoaderBootDriver, &DriverBase);
    if (!Success)
    {
        ERR("PeLdrLoadImage('%s') failed\n", DllName);
        return FALSE;
    }

    // Allocate a DTE for it
    Success = PeLdrAllocateDataTableEntry(LoadOrderListHead,
                                          DllName,
                                          DllName,
                                          PaToVa(DriverBase),
                                          DriverDTE);
    if (!Success)
    {
        /* Cleanup and bail out */
        ERR("PeLdrAllocateDataTableEntry('%s') failed\n", DllName);
        MmFreeMemory(DriverBase);
        return FALSE;
    }

    /* Init security cookie */
    PeLdrInitSecurityCookie(*DriverDTE);

    // Modify any flags, if needed
    (*DriverDTE)->Flags |= Flags;

    // Look for any dependencies it may have, and load them too
    RtlStringCbPrintfA(FullPath, sizeof(FullPath), "%s%s", BootPath, DriverPath);
    Success = PeLdrScanImportDescriptorTable(LoadOrderListHead, FullPath, *DriverDTE);
    if (!Success)
    {
        /* Cleanup and bail out */
        ERR("PeLdrScanImportDescriptorTable('%s') failed\n", FullPath);
        PeLdrFreeDataTableEntry(*DriverDTE);
        MmFreeMemory(DriverBase);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
WinLdrLoadBootDrivers(PLOADER_PARAMETER_BLOCK LoaderBlock,
                      PCSTR BootPath)
{
    PLIST_ENTRY NextBd;
    PBOOT_DRIVER_NODE DriverNode;
    PBOOT_DRIVER_LIST_ENTRY BootDriver;
    BOOLEAN Success;
    BOOLEAN ret = TRUE;

    /* Walk through the boot drivers list */
    NextBd = LoaderBlock->BootDriverListHead.Flink;
    while (NextBd != &LoaderBlock->BootDriverListHead)
    {
        DriverNode = CONTAINING_RECORD(NextBd,
                                       BOOT_DRIVER_NODE,
                                       ListEntry.Link);
        BootDriver = &DriverNode->ListEntry;

        /* Get the next list entry as we may remove the current one on failure */
        NextBd = BootDriver->Link.Flink;

        TRACE("BootDriver %wZ DTE %08X RegPath: %wZ\n",
              &BootDriver->FilePath, BootDriver->LdrEntry,
              &BootDriver->RegistryPath);

        // Paths are relative (FIXME: Are they always relative?)

        /* Load it */
        UiIndicateProgress();
        Success = WinLdrLoadDeviceDriver(&LoaderBlock->LoadOrderListHead,
                                         BootPath,
                                         &BootDriver->FilePath,
                                         0,
                                         &BootDriver->LdrEntry);
        if (Success)
        {
            /* Convert the addresses to VA since we are not going to use them anymore */
            BootDriver->RegistryPath.Buffer = PaToVa(BootDriver->RegistryPath.Buffer);
            BootDriver->FilePath.Buffer = PaToVa(BootDriver->FilePath.Buffer);
            BootDriver->LdrEntry = PaToVa(BootDriver->LdrEntry);

            if (DriverNode->Group.Buffer)
                DriverNode->Group.Buffer = PaToVa(DriverNode->Group.Buffer);
            DriverNode->Name.Buffer = PaToVa(DriverNode->Name.Buffer);
        }
        else
        {
            /* Loading failed: cry loudly */
            ERR("Cannot load boot driver '%wZ'!\n", &BootDriver->FilePath);
            UiMessageBox("Cannot load boot driver '%wZ'!", &BootDriver->FilePath);
            ret = FALSE;

            /* Remove it from the list and try to continue */
            RemoveEntryList(&BootDriver->Link);
        }
    }

    return ret;
}

PVOID
WinLdrLoadModule(PCSTR ModuleName,
                 PULONG Size,
                 TYPE_OF_MEMORY MemoryType)
{
    ULONG FileId;
    PVOID PhysicalBase;
    FILEINFORMATION FileInfo;
    ULONG FileSize;
    ARC_STATUS Status;
    ULONG BytesRead;

    *Size = 0;

    /* Open the image file */
    NtLdrOutputLoadMsg(ModuleName, NULL);
    Status = ArcOpen((PSTR)ModuleName, OpenReadOnly, &FileId);
    if (Status != ESUCCESS)
    {
        /* In case of errors, we just return, without complaining to the user */
        WARN("Error while opening '%s', Status: %u\n", ModuleName, Status);
        return NULL;
    }

    /* Retrieve its size */
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
        ERR("Could not allocate memory for '%s'\n", ModuleName);
        ArcClose(FileId);
        return NULL;
    }

    /* Load the whole file */
    Status = ArcRead(FileId, PhysicalBase, FileSize, &BytesRead);
    ArcClose(FileId);
    if (Status != ESUCCESS)
    {
        WARN("Error while reading '%s', Status: %u\n", ModuleName, Status);
        return NULL;
    }

    TRACE("Loaded %s at 0x%x with size 0x%x\n", ModuleName, PhysicalBase, FileSize);

    return PhysicalBase;
}

USHORT
WinLdrDetectVersion(VOID)
{
    LONG rc;
    HKEY hKey;

    rc = RegOpenKey(CurrentControlSetKey, L"Control\\Terminal Server", &hKey);
    if (rc != ERROR_SUCCESS)
    {
        /* Key doesn't exist; assume NT 4.0 */
        return _WIN32_WINNT_NT4;
    }
    RegCloseKey(hKey);

    /* We may here want to read the value of ProductVersion */
    return _WIN32_WINNT_WS03;
}

static
PVOID
LoadModule(
    _Inout_ PLIST_ENTRY LoadOrderListHead,
    _In_ PCSTR Path,
    _In_ PCSTR File,
    _In_ PCSTR ImportName, // BaseDllName
    _In_ TYPE_OF_MEMORY MemoryType,
    _Out_ PLDR_DATA_TABLE_ENTRY* Dte,
    _In_ ULONG Percentage)
{
    BOOLEAN Success;
    CHAR FullFileName[MAX_PATH];
    CHAR ProgressString[256];
    PVOID BaseAddress;

    RtlStringCbPrintfA(ProgressString, sizeof(ProgressString), "Loading %s...", File);
    UiUpdateProgressBar(Percentage, ProgressString);

    RtlStringCbCopyA(FullFileName, sizeof(FullFileName), Path);
    RtlStringCbCatA(FullFileName, sizeof(FullFileName), File);

    NtLdrOutputLoadMsg(FullFileName, NULL);
    Success = PeLdrLoadImage(FullFileName, MemoryType, &BaseAddress);
    if (!Success)
    {
        ERR("PeLdrLoadImage('%s') failed\n", File);
        return NULL;
    }
    TRACE("%s loaded successfully at %p\n", File, BaseAddress);

    Success = PeLdrAllocateDataTableEntry(LoadOrderListHead,
                                          ImportName,
                                          FullFileName,
                                          PaToVa(BaseAddress),
                                          Dte);
    if (!Success)
    {
        /* Cleanup and bail out */
        ERR("PeLdrAllocateDataTableEntry('%s') failed\n", FullFileName);
        MmFreeMemory(BaseAddress);
        return NULL;
    }

    /* Init security cookie */
    PeLdrInitSecurityCookie(*Dte);

    return BaseAddress;
}

#ifdef _M_IX86
static
BOOLEAN
WinLdrIsPaeSupported(
    _In_ USHORT OperatingSystemVersion,
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ PCSTR BootOptions,
    _In_ PCSTR HalFileName,
    _Inout_updates_bytes_(KernelFileNameSize) _Always_(_Post_z_)
         PSTR KernelFileName,
    _In_ SIZE_T KernelFileNameSize)
{
    BOOLEAN PaeEnabled = FALSE;
    BOOLEAN PaeDisabled = FALSE;
    BOOLEAN Result;

    if ((OperatingSystemVersion > _WIN32_WINNT_NT4) &&
        NtLdrGetOption(BootOptions, "PAE"))
    {
        /* We found the PAE option */
        PaeEnabled = TRUE;
    }

    Result = PaeEnabled;

    if ((OperatingSystemVersion > _WIN32_WINNT_WIN2K) &&
        NtLdrGetOption(BootOptions, "NOPAE"))
    {
        PaeDisabled = TRUE;
    }

    if (SafeBoot)
        PaeDisabled = TRUE;

    TRACE("PaeEnabled %X, PaeDisabled %X\n", PaeEnabled, PaeDisabled);

    if (PaeDisabled)
        Result = FALSE;

    /* Enable PAE if DEP is enabled */
    if (NoExecuteEnabled)
        Result = TRUE;

    // TODO: checks for CPU support, hotplug memory support ... other tests
    // TODO: select kernel name ("ntkrnlpa.exe" or "ntoskrnl.exe"), or,
    // if KernelFileName is a user-specified kernel file, check whether it
    // has, if PAE needs to be enabled, the IMAGE_FILE_LARGE_ADDRESS_AWARE
    // Characteristics bit set, and that the HAL image has a similar support.

    if (Result) UNIMPLEMENTED;

    return Result;
}
#endif /* _M_IX86 */

static VOID
NtLdrDetectBootVid(
    _In_ PCONFIGURATION_COMPONENT_DATA ConfigTree,
    _Out_writes_bytes_(BootVidNameSize) _Always_(_Post_z_)
         PSTR BootVidName,
    _In_ SIZE_T BootVidNameSize)
{
    /* NOTE: The following array should instead be read from txtsetup.sif */
    static const struct
    {
        PCSTR SystemSubId;
        PCSTR ModuleName;
    } BootVidNames[] =
    {
#if defined(_M_IX86) && !defined(UEFIBOOT)
        {"PC-98", "pc98bvid.dll"},
        {"Xbox", "xboxbvid.dll"},
#endif
        {"EFI", "lfbbvid.dll"},
    };

    /* Default BootVid file name */
    RtlStringCbCopyA(BootVidName, BootVidNameSize, "bootvid.dll");

    /* Attempt auto-detection by System ID */
    PCONFIGURATION_COMPONENT_DATA Entry;
    Entry = KeFindConfigurationEntry(ConfigTree,
                                     SystemClass,
                                     ArcSystem,
                                     NULL);
    if (!Entry)
    {
        /* No SystemClass ArcSystem entry, retry with hack */
        Entry = KeFindConfigurationEntry(ConfigTree,
                                         SystemClass,
                                         MaximumType, // HACK for non-ARC machines
                                         NULL);
    }
    if (Entry)
    {
        // Entry->ComponentEntry.IdentifierLength;
        TRACE("ARC System ID: '%s'\n", Entry->ComponentEntry.Identifier);

        /* Look up the name */
        for (ULONG i = 0; i < RTL_NUMBER_OF(BootVidNames); ++i)
        {
            if (strstr(Entry->ComponentEntry.Identifier, BootVidNames[i].SystemSubId))
            {
                RtlStringCbCopyA(BootVidName, BootVidNameSize, BootVidNames[i].ModuleName);
                break;
            }
        }
    }
    /* Special-case detection if the display controller contains
     * a DeviceSpecific CM_FRAMEBUF_DEVICE_DATA resource */
    Entry = KeFindConfigurationEntry(ConfigTree,
                                     ControllerClass,
                                     DisplayController,
                                     NULL);
    if (Entry)
    {
        /* Cast to PCM_PARTIAL_RESOURCE_LIST to access
         * the common Version and Revision fields */
        PCM_PARTIAL_RESOURCE_LIST ResourceList =
            (PCM_PARTIAL_RESOURCE_LIST)Entry->ConfigurationData;
        ULONG ConfigurationDataLength =
            Entry->ComponentEntry.ConfigurationDataLength;

        // Entry->ComponentEntry.IdentifierLength;
        TRACE("Display: '%s'\n", Entry->ComponentEntry.Identifier);

        /* The configuration data length must be valid.
         * The Version/Revision must be greater than or equal to 1.2
         * in order to describe a valid new configuration data. */
        if (Entry->ConfigurationData &&
            (ConfigurationDataLength >= sizeof(CM_PARTIAL_RESOURCE_LIST)) &&
            ( (ResourceList->Version >  1) ||
             ((ResourceList->Version == 1) && (ResourceList->Revision > 1)) ))
        {
            /* Try to find the DeviceSpecific resource */
            PCM_PARTIAL_RESOURCE_DESCRIPTOR DeviceSpecific = NULL;
            for (ULONG i = 0; i < ResourceList->Count; ++i)
            {
                PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor =
                    &ResourceList->PartialDescriptors[i];
                if (Descriptor->Type == CmResourceTypeDeviceSpecific)
                {
                    /* NOTE: This descriptor *MUST* be the last one.
                     * The actual device data follows the descriptor. */
                    ASSERT(i == ResourceList->Count - 1);
                    DeviceSpecific = Descriptor;
                    break;
                }
            }

            /* Check whether the resource describes framebuffer information */
            if (DeviceSpecific &&
                (DeviceSpecific->u.DeviceSpecificData.DataSize >= sizeof(CM_FRAMEBUF_DEVICE_DATA)))
            {
                /* It does, use the Linear FrameBuffer Boot Video Driver instead */
                RtlStringCbCopyA(BootVidName, BootVidNameSize, "lfbbvid.dll");
            }
        }
    }
}

static
BOOLEAN
LoadWindowsCore(IN USHORT OperatingSystemVersion,
                IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                IN PCSTR BootOptions,
                IN PCSTR BootPath,
                IN OUT PLDR_DATA_TABLE_ENTRY* KernelDTE)
{
    BOOLEAN Success;
    PCSTR Option;
    ULONG OptionLength;
    PVOID KernelBase, HalBase, KdDllBase = NULL, BootVidBase = NULL;
    PLDR_DATA_TABLE_ENTRY HalDTE, KdDllDTE = NULL, BootVidDTE = NULL;
    PCSTR ModuleName;
    CHAR DirPath[MAX_PATH];
    CHAR HalFileName[MAX_PATH];
    CHAR KernelFileName[MAX_PATH];
    CHAR KdDllName[MAX_PATH];
    CHAR BootVidName[MAX_PATH];

    /* Initialize SystemRoot\System32 path */
    RtlStringCbCopyA(DirPath, sizeof(DirPath), BootPath);
    RtlStringCbCatA(DirPath, sizeof(DirPath), "system32\\");

    /* Parse the boot options */
    TRACE("LoadWindowsCore: BootOptions '%s'\n", BootOptions);

#ifdef _M_IX86
    if (NtLdrGetOption(BootOptions, "3GB"))
    {
        /* We found the 3GB option. */
        FIXME("LoadWindowsCore: 3GB - TRUE (not implemented)\n");
        VirtualBias = TRUE;
    }
    // TODO: "USERVA=" for XP/2k3
#endif

    if ((OperatingSystemVersion > _WIN32_WINNT_NT4) &&
        (NtLdrGetOption(BootOptions, "SAFEBOOT") ||
         NtLdrGetOption(BootOptions, "SAFEBOOT:")))
    {
        /* We found the SAFEBOOT option. */
        FIXME("LoadWindowsCore: SAFEBOOT - TRUE (not implemented)\n");
        SafeBoot = TRUE;
    }

    if ((OperatingSystemVersion > _WIN32_WINNT_WIN2K) &&
        NtLdrGetOption(BootOptions, "BOOTLOGO"))
    {
        /* We found the BOOTLOGO option. */
        FIXME("LoadWindowsCore: BOOTLOGO - TRUE (not implemented)\n");
        BootLogo = TRUE;
    }

    /* Check the (NO)EXECUTE options */
    if ((OperatingSystemVersion > _WIN32_WINNT_WIN2K) &&
        !LoaderBlock->SetupLdrBlock)
    {
        /* Disable NX by default on x86, otherwise enable it */
#ifdef _M_IX86
        NoExecuteEnabled = FALSE;
#else
        NoExecuteEnabled = TRUE;
#endif

#ifdef _M_IX86
        /* Check the options in decreasing order of precedence */
        if (NtLdrGetOption(BootOptions, "NOEXECUTE=OPTIN")  ||
            NtLdrGetOption(BootOptions, "NOEXECUTE=OPTOUT") ||
            NtLdrGetOption(BootOptions, "NOEXECUTE=ALWAYSON"))
        {
            NoExecuteEnabled = TRUE;
        }
        else if (NtLdrGetOption(BootOptions, "NOEXECUTE=ALWAYSOFF"))
            NoExecuteEnabled = FALSE;
        else
#else
        /* Only the following two options really apply for x64 and other platforms */
#endif
        if (NtLdrGetOption(BootOptions, "NOEXECUTE"))
            NoExecuteEnabled = TRUE;
        else if (NtLdrGetOption(BootOptions, "EXECUTE"))
            NoExecuteEnabled = FALSE;

#ifdef _M_IX86
        /* Disable DEP in SafeBoot mode for x86 only */
        if (SafeBoot)
            NoExecuteEnabled = FALSE;
#endif
    }
    TRACE("NoExecuteEnabled %X\n", NoExecuteEnabled);

    /*
     * Select the HAL and KERNEL file names.
     * Check for any "HAL=" or "KERNEL=" override option.
     *
     * See the following links to know how the file names are actually chosen:
     * https://www.geoffchappell.com/notes/windows/boot/bcd/osloader/detecthal.htm
     * https://www.geoffchappell.com/notes/windows/boot/bcd/osloader/hal.htm
     * https://www.geoffchappell.com/notes/windows/boot/bcd/osloader/kernel.htm
     */
    /* Default HAL and KERNEL file names */
    RtlStringCbCopyA(HalFileName   , sizeof(HalFileName)   , "hal.dll");
    RtlStringCbCopyA(KernelFileName, sizeof(KernelFileName), "ntoskrnl.exe");

    Option = NtLdrGetOptionEx(BootOptions, "HAL=", &OptionLength);
    if (Option && (OptionLength > 4))
    {
        /* Retrieve the HAL file name */
        Option += 4; OptionLength -= 4;
        RtlStringCbCopyNA(HalFileName, sizeof(HalFileName), Option, OptionLength);
        _strlwr(HalFileName);
    }

    Option = NtLdrGetOptionEx(BootOptions, "KERNEL=", &OptionLength);
    if (Option && (OptionLength > 7))
    {
        /* Retrieve the KERNEL file name */
        Option += 7; OptionLength -= 7;
        RtlStringCbCopyNA(KernelFileName, sizeof(KernelFileName), Option, OptionLength);
        _strlwr(KernelFileName);
    }

#ifdef _M_IX86
    /* Check for PAE support and select the adequate kernel image */
    PaeModeOn = WinLdrIsPaeSupported(OperatingSystemVersion,
                                     LoaderBlock,
                                     BootOptions,
                                     HalFileName,
                                     KernelFileName,
                                     sizeof(KernelFileName));
    if (PaeModeOn) FIXME("WinLdrIsPaeSupported: PaeModeOn\n");
#endif

    TRACE("HAL file = '%s' ; Kernel file = '%s'\n", HalFileName, KernelFileName);

    /*
     * Load the core NT files: Kernel, HAL and KD transport DLL.
     * Cheat about their base DLL name so as to satisfy the imports/exports,
     * even if the corresponding underlying files do not have the same names
     * -- this happens e.g. with UP vs. MP kernel, standard vs. ACPI hal, or
     * different KD transport DLLs.
     */

    /* Load the Kernel */
    KernelBase = LoadModule(&LoaderBlock->LoadOrderListHead,
                            DirPath, KernelFileName,
                            "ntoskrnl.exe", LoaderSystemCode,
                            KernelDTE, 30);
    if (!KernelBase)
    {
        ERR("LoadModule('%s') failed\n", KernelFileName);
        UiMessageBox("Could not load %s", KernelFileName);
        return FALSE;
    }

    /* Load the HAL */
    HalBase = LoadModule(&LoaderBlock->LoadOrderListHead,
                         DirPath, HalFileName,
                         "hal.dll", LoaderHalCode,
                         &HalDTE, 35);
    if (!HalBase)
    {
        ERR("LoadModule('%s') failed\n", HalFileName);
        UiMessageBox("Could not load %s", HalFileName);
        PeLdrFreeDataTableEntry(*KernelDTE);
        MmFreeMemory(KernelBase);
        return FALSE;
    }

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
         * A Kernel Debugger Transport DLL is always loaded for Windows XP+ :
         * either the standard KDCOM.DLL (by default): IsCustomKdDll == FALSE
         * or an alternative user-provided one via the /DEBUGPORT= option:
         * IsCustomKdDll == TRUE if it does not specify the default KDCOM.
         */
        BOOLEAN IsCustomKdDll = FALSE;

        /* Check whether there is a DEBUGPORT option */
        Option = NtLdrGetOptionEx(BootOptions, "DEBUGPORT=", &OptionLength);
        if (Option && (OptionLength > 10))
        {
            /* Move to the debug port name */
            Option += 10; OptionLength -= 10;

            /*
             * Parse the port name.
             * Format: /DEBUGPORT=COM[0-9]
             * or: /DEBUGPORT=FILE:\Device\HarddiskX\PartitionY\debug.log
             * or: /DEBUGPORT=FOO
             * If we only have /DEBUGPORT= (i.e. without any port name),
             * default to "COM".
             */

            /* Get the actual length of the debug port
             * until the next whitespace or colon. */
            OptionLength = (ULONG)strcspn(Option, " \t:");

            if ((OptionLength == 0) ||
                ( (OptionLength >= 3) && (_strnicmp(Option, "COM", 3) == 0) &&
                 ((OptionLength == 3) || ('0' <= Option[3] && Option[3] <= '9')) ))
            {
                /* The standard KDCOM.DLL is used */
            }
            else
            {
                /* A custom KD DLL is used */
                IsCustomKdDll = TRUE;
            }
        }
        if (!IsCustomKdDll)
        {
            Option = "COM"; OptionLength = 3;
        }

        RtlStringCbPrintfA(KdDllName, sizeof(KdDllName), "kd%.*s.dll",
                           OptionLength, Option);
        _strlwr(KdDllName);

        /* Load the KD DLL. Override its base DLL name to the default "KDCOM.DLL". */
        KdDllBase = LoadModule(&LoaderBlock->LoadOrderListHead,
                               DirPath, KdDllName,
                               "kdcom.dll", LoaderSystemCode,
                               &KdDllDTE, 40);
        if (!KdDllBase)
        {
            /* If we failed to load a custom KD DLL, fall back to the standard one */
            if (IsCustomKdDll)
            {
                /* The custom KD DLL being optional, just ignore the failure */
                WARN("LoadModule('%s') failed\n", KdDllName);

                IsCustomKdDll = FALSE;
                RtlStringCbCopyA(KdDllName, sizeof(KdDllName), "kdcom.dll");

                KdDllBase = LoadModule(&LoaderBlock->LoadOrderListHead,
                                       DirPath, KdDllName,
                                       "kdcom.dll", LoaderSystemCode,
                                       &KdDllDTE, 40);
            }

            if (!KdDllBase)
            {
                /* Ignore the failure; we will fail later when scanning the
                 * kernel import tables, if it really needs the KD DLL. */
                ERR("LoadModule('%s') failed\n", KdDllName);
            }
        }
    }

    /* Load the correct Boot Video Driver */
    if (OperatingSystemVersion >= _WIN32_WINNT_WIN2K)
    {
        /* Check whether there is a BOOTVID option */
        Option = NtLdrGetOptionEx(BootOptions, "BOOTVID=", &OptionLength);
        if (Option && (OptionLength > 8))
        {
            /* Retrieve the BootVid file name */
            Option += 8; OptionLength -= 8;
            RtlStringCbCopyNA(BootVidName, sizeof(BootVidName), Option, OptionLength);
            _strlwr(BootVidName);
            TRACE("User-specified BootVid file: '%s'\n", BootVidName);
        }
        else
        {
            /* No option given, attempt auto-detection */
            NtLdrDetectBootVid(LoaderBlock->ConfigurationRoot,
                               BootVidName, sizeof(BootVidName));
            TRACE("Auto-detected BootVid file: '%s'\n", BootVidName);
        }

        /* Load the BootVid */
        BootVidBase = LoadModule(&LoaderBlock->LoadOrderListHead,
                                 DirPath, BootVidName,
                                 "bootvid.dll", LoaderSystemCode,
                                 &BootVidDTE, 45);
        if (!BootVidBase)
        {
            /* Ignore the failure; we will fail later when scanning the
             * kernel import tables, if it really needs the BootVid. */
            ERR("LoadModule('%s') failed\n", BootVidName);
        }
    }

    /* Load all referenced DLLs for Kernel, HAL, Kernel Debugger Transport and BootVid */
    ModuleName = KernelFileName;
    Success = PeLdrScanImportDescriptorTable(&LoaderBlock->LoadOrderListHead, DirPath, *KernelDTE);
    if (!Success)
        goto Quit;
    ModuleName = HalFileName;
    Success = PeLdrScanImportDescriptorTable(&LoaderBlock->LoadOrderListHead, DirPath, HalDTE);
    if (!Success)
        goto Quit;
    if (KdDllDTE)
    {
        ModuleName = KdDllName;
        Success = PeLdrScanImportDescriptorTable(&LoaderBlock->LoadOrderListHead, DirPath, KdDllDTE);
        if (!Success)
            goto Quit;
    }
    if (BootVidDTE)
    {
        ModuleName = BootVidName;
        Success = PeLdrScanImportDescriptorTable(&LoaderBlock->LoadOrderListHead, DirPath, BootVidDTE);
        if (!Success)
            goto Quit;
    }

Quit:
    if (!Success)
    {
        ERR("PeLdrScanImportDescriptorTable('%s') failed\n", ModuleName);
        UiMessageBox("Could not load %s", ModuleName);

        /* Cleanup and bail out */
        if (BootVidDTE)
            PeLdrFreeDataTableEntry(BootVidDTE);
        if (BootVidBase) // Optional
            MmFreeMemory(BootVidBase);

        if (KdDllDTE)
            PeLdrFreeDataTableEntry(KdDllDTE);
        if (KdDllBase) // Optional
            MmFreeMemory(KdDllBase);

        PeLdrFreeDataTableEntry(HalDTE);
        MmFreeMemory(HalBase);

        PeLdrFreeDataTableEntry(*KernelDTE);
        MmFreeMemory(KernelBase);
    }

    return Success;
}

static
BOOLEAN
WinLdrInitErrataInf(
    IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN USHORT OperatingSystemVersion,
    IN PCSTR SystemRoot)
{
    LONG rc;
    HKEY hKey;
    ULONG BufferSize;
    ULONG FileSize;
    PVOID PhysicalBase;
    WCHAR szFileName[80];
    CHAR ErrataFilePath[MAX_PATH];

    /* Open either the 'BiosInfo' (Windows <= 2003) or the 'Errata' (Vista+) key */
    if (OperatingSystemVersion >= _WIN32_WINNT_VISTA)
    {
        rc = RegOpenKey(CurrentControlSetKey, L"Control\\Errata", &hKey);
    }
    else // (OperatingSystemVersion <= _WIN32_WINNT_WS03)
    {
        rc = RegOpenKey(CurrentControlSetKey, L"Control\\BiosInfo", &hKey);
    }
    if (rc != ERROR_SUCCESS)
    {
        WARN("Could not open the BiosInfo/Errata registry key (Error %u)\n", (int)rc);
        return FALSE;
    }

    /* Retrieve the INF file name value */
    BufferSize = sizeof(szFileName);
    rc = RegQueryValue(hKey, L"InfName", NULL, (PUCHAR)szFileName, &BufferSize);
    if (rc != ERROR_SUCCESS)
    {
        WARN("Could not retrieve the InfName value (Error %u)\n", (int)rc);
        RegCloseKey(hKey);
        return FALSE;
    }

    // TODO: "SystemBiosDate"

    RegCloseKey(hKey);

    RtlStringCbPrintfA(ErrataFilePath, sizeof(ErrataFilePath), "%s%s%S",
                       SystemRoot, "inf\\", szFileName);

    /* Load the INF file */
    PhysicalBase = WinLdrLoadModule(ErrataFilePath, &FileSize, LoaderRegistryData);
    if (!PhysicalBase)
    {
        WARN("Could not load '%s'\n", ErrataFilePath);
        return FALSE;
    }

    LoaderBlock->Extension->EmInfFileImage = PaToVa(PhysicalBase);
    LoaderBlock->Extension->EmInfFileSize  = FileSize;

    return TRUE;
}

/**
 * @brief
 * Normalize in-place the NT boot options by removing any leading '/',
 * normalizing TABs to spaces, etc.
 **/
VOID
NtLdrNormalizeOptions(
    _Inout_ PSTR LoadOptions)
{
    PCHAR NewOptions = LoadOptions;
    PCSTR Options, Option;
    ULONG OptionLength;

    /* Normalize the boot options by successively enumerating each and
     * copying them back, stripping and replacing any extra separator
     * by one single space. */
    Options = LoadOptions;
    while ((Option = NtLdrGetNextOption(&Options, &OptionLength)))
    {
        if (NewOptions > LoadOptions)
            *NewOptions++ = ' ';

        /* If necessary, move the current option back */
        ASSERT(NewOptions <= Option);
        if (NewOptions < Option)
            RtlMoveMemory(NewOptions, Option, OptionLength * sizeof(CHAR));
        NewOptions += OptionLength;
    }

    /* NUL-terminate */
    *NewOptions = ANSI_NULL;
}

ARC_STATUS
LoadAndBootWindows(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[])
{
    ARC_STATUS Status;
    PCSTR ArgValue;
    PCSTR SystemPartition;
    PCSTR FileName;
    ULONG FileNameLength;
    BOOLEAN Success;
    USHORT OperatingSystemVersion;
    PLOADER_PARAMETER_BLOCK LoaderBlock;
    CHAR BootPath[MAX_PATH];
    CHAR FilePath[MAX_PATH];
    CHAR BootOptions[MAX_OPTIONS_LENGTH+1];

    /* Retrieve the (mandatory) boot type */
    ArgValue = GetArgumentValue(Argc, Argv, "BootType");
    if (!ArgValue || !*ArgValue)
    {
        ERR("No 'BootType' value, aborting!\n");
        return EINVAL;
    }

    /* Convert it to an OS version */
    if (_stricmp(ArgValue, "Windows") == 0 ||
        _stricmp(ArgValue, "Windows2003") == 0)
    {
        OperatingSystemVersion = _WIN32_WINNT_WS03;
    }
    else if (_stricmp(ArgValue, "WindowsNT40") == 0)
    {
        OperatingSystemVersion = _WIN32_WINNT_NT4;
    }
    else if (_stricmp(ArgValue, "WindowsVista") == 0)
    {
        OperatingSystemVersion = _WIN32_WINNT_VISTA;
    }
    else
    {
        ERR("Unknown 'BootType' value '%s', aborting!\n", ArgValue);
        return EINVAL;
    }

    /* Retrieve the (mandatory) system partition */
    SystemPartition = GetArgumentValue(Argc, Argv, "SystemPartition");
    if (!SystemPartition || !*SystemPartition)
    {
        ERR("No 'SystemPartition' specified, aborting!\n");
        return EINVAL;
    }

    /* Let the user know we started loading */
    UiDrawBackdrop(UiGetScreenHeight());
    UiDrawStatusText("Loading...");
    UiDrawProgressBarCenter("Loading NT...");

    /* Retrieve the system path */
    *BootPath = ANSI_NULL;
    ArgValue = GetArgumentValue(Argc, Argv, "SystemPath");
    if (ArgValue)
        RtlStringCbCopyA(BootPath, sizeof(BootPath), ArgValue);

    /*
     * Check whether BootPath is a full path
     * and if not, create a full boot path.
     *
     * See FsOpenFile for the technique used.
     */
    if (strrchr(BootPath, ')') == NULL)
    {
        /* Temporarily save the boot path */
        RtlStringCbCopyA(FilePath, sizeof(FilePath), BootPath);

        /* This is not a full path: prepend the SystemPartition */
        RtlStringCbCopyA(BootPath, sizeof(BootPath), SystemPartition);

        /* Append a path separator if needed */
        if (*FilePath != '\\' && *FilePath != '/')
            RtlStringCbCatA(BootPath, sizeof(BootPath), "\\");

        /* Append the remaining path */
        RtlStringCbCatA(BootPath, sizeof(BootPath), FilePath);
    }

    /* Append a path separator if needed */
    if (!*BootPath || BootPath[strlen(BootPath) - 1] != '\\')
        RtlStringCbCatA(BootPath, sizeof(BootPath), "\\");

    TRACE("BootPath: '%s'\n", BootPath);

    /* Retrieve the boot options */
    *BootOptions = ANSI_NULL;
    ArgValue = GetArgumentValue(Argc, Argv, "Options");
    if (ArgValue && *ArgValue)
        RtlStringCbCopyA(BootOptions, sizeof(BootOptions), ArgValue);
    TRACE("BootOptions(1): '%s'\n", BootOptions);

    /*
     * Set the "HAL=" and "KERNEL=" options if needed.
     * If already present on the standard "Options=" option line, they take
     * precedence over those passed via the separate "Hal=" and "Kernel="
     * options.
     */
    if (!NtLdrGetOption(BootOptions, "HAL="))
    {
        /*
         * Not found in the options, try to retrieve the
         * separate value and append it to the options.
         */
        ArgValue = GetArgumentValue(Argc, Argv, "Hal");
        if (ArgValue && *ArgValue)
        {
            RtlStringCbCatA(BootOptions, sizeof(BootOptions), " /HAL=");
            RtlStringCbCatA(BootOptions, sizeof(BootOptions), ArgValue);
        }
    }
    if (!NtLdrGetOption(BootOptions, "KERNEL="))
    {
        /*
         * Not found in the options, try to retrieve the
         * separate value and append it to the options.
         */
        ArgValue = GetArgumentValue(Argc, Argv, "Kernel");
        if (ArgValue && *ArgValue)
        {
            RtlStringCbCatA(BootOptions, sizeof(BootOptions), " /KERNEL=");
            RtlStringCbCatA(BootOptions, sizeof(BootOptions), ArgValue);
        }
    }

    /* Append boot-time options */
    AppendBootTimeOptions(BootOptions, sizeof(BootOptions));

    /* Post-process the boot options */
    NtLdrNormalizeOptions(BootOptions);
    TRACE("BootOptions(2): '%s'\n", BootOptions);

    /* Check if a RAM disk file was given */
    FileName = NtLdrGetOptionEx(BootOptions, "RDPATH=", &FileNameLength);
    if (FileName && (FileNameLength >= 7))
    {
        /* Load the RAM disk */
        Status = RamDiskInitialize(FALSE, BootOptions, SystemPartition);
        if (Status != ESUCCESS)
        {
            FileName += 7; FileNameLength -= 7;
            UiMessageBox("Failed to load RAM disk file '%.*s'",
                         FileNameLength, FileName);
            return Status;
        }
    }

    /* Handle the SOS option */
    SosEnabled = !!NtLdrGetOption(BootOptions, "SOS");
    if (SosEnabled)
        UiResetForSOS();

    /* Allocate and minimally-initialize the Loader Parameter Block */
    AllocateAndInitLPB(OperatingSystemVersion, &LoaderBlock);

    /* Load the system hive */
    UiUpdateProgressBar(15, "Loading system hive...");
    Success = WinLdrInitSystemHive(LoaderBlock, BootPath, FALSE);
    TRACE("SYSTEM hive %s\n", (Success ? "loaded" : "not loaded"));
    /* Bail out if failure */
    if (!Success)
        return ENOEXEC;

    /* Fixup the version number using data from the registry */
    if (OperatingSystemVersion == 0)
        OperatingSystemVersion = WinLdrDetectVersion();
    LoaderBlock->Extension->MajorVersion = (OperatingSystemVersion & 0xFF00) >> 8;
    LoaderBlock->Extension->MinorVersion = (OperatingSystemVersion & 0xFF);

    /* Load NLS data, OEM font, and prepare boot drivers list */
    Success = WinLdrScanSystemHive(LoaderBlock, BootPath);
    TRACE("SYSTEM hive %s\n", (Success ? "scanned" : "not scanned"));
    /* Bail out if failure */
    if (!Success)
        return ENOEXEC;

    /* Load the Firmware Errata file */
    Success = WinLdrInitErrataInf(LoaderBlock, OperatingSystemVersion, BootPath);
    TRACE("Firmware Errata file %s\n", (Success ? "loaded" : "not loaded"));
    /* Not necessarily fatal if not found - carry on going */

    /* Finish loading */
    return LoadAndBootWindowsCommon(OperatingSystemVersion,
                                    LoaderBlock,
                                    BootOptions,
                                    SystemPartition,
                                    BootPath);
}

ARC_STATUS
LoadAndBootWindowsCommon(
    _In_ USHORT OperatingSystemVersion,
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ PCSTR BootOptions,
    _In_ PCSTR SystemPartition,
    _In_ PCSTR BootPath)
{
    PLOADER_PARAMETER_BLOCK LoaderBlockVA;
    BOOLEAN Success;
    PLDR_DATA_TABLE_ENTRY KernelDTE;
    KERNEL_ENTRY_POINT KiSystemStartup;

    TRACE("LoadAndBootWindowsCommon()\n");

    ASSERT(OperatingSystemVersion != 0);

    /* Setup redirection support */
    WinLdrSetupEms(BootOptions);

    /* Detect hardware */
    UiUpdateProgressBar(20, "Detecting hardware...");
    LoaderBlock->ConfigurationRoot = MachHwDetect(BootOptions);

    /* Initialize the PE loader import-DLL callback, so that we can obtain
     * feedback (for example during SOS) on the PE images that get loaded. */
    PeLdrImportDllLoadCallback = NtLdrImportDllLoadCallback;

    /* Load the operating system core: the Kernel, the HAL and the Kernel Debugger Transport DLL */
    Success = LoadWindowsCore(OperatingSystemVersion,
                              LoaderBlock,
                              BootOptions,
                              BootPath,
                              &KernelDTE);
    if (!Success)
    {
        /* Reset the PE loader import-DLL callback */
        PeLdrImportDllLoadCallback = NULL;

        UiMessageBox("Error loading NTOS core.");
        return ENOEXEC;
    }

    /* Cleanup INI file */
    IniCleanup();

/****
 **** WE HAVE NOW REACHED THE POINT OF NO RETURN !!
 ****/

    UiSetProgressBarSubset(40, 90); // NTOS goes from 25 to 75%

    /* Load boot drivers */
    UiSetProgressBarText("Loading boot drivers...");
    Success = WinLdrLoadBootDrivers(LoaderBlock, BootPath);
    TRACE("Boot drivers loading %s\n", Success ? "successful" : "failed");

    UiSetProgressBarSubset(0, 100);

    /* Reset the PE loader import-DLL callback */
    PeLdrImportDllLoadCallback = NULL;

    /* Initialize Phase 1 - no drivers loading anymore */
    WinLdrInitializePhase1(OperatingSystemVersion,
                           LoaderBlock,
                           BootOptions,
                           SystemPartition,
                           BootPath);

    UiUpdateProgressBar(100, NULL);

    /* Save entry-point pointer and Loader block VAs */
    KiSystemStartup = (KERNEL_ENTRY_POINT)KernelDTE->EntryPoint;
    LoaderBlockVA = PaToVa(LoaderBlock);

    /* "Stop all motors", change videomode */
    MachPrepareForReactOS();

    /* Show the "debug mode" notice if needed */
    /* Match KdInitSystem() conditions */
    if (!NtLdrGetOption(BootOptions, "CRASHDEBUG") &&
        !NtLdrGetOption(BootOptions, "NODEBUG") &&
        !!NtLdrGetOption(BootOptions, "DEBUG"))
    {
        /* Check whether there is a DEBUGPORT option */
        PCSTR DebugPort;
        ULONG DebugPortLength = 0;
        DebugPort = NtLdrGetOptionEx(BootOptions, "DEBUGPORT=", &DebugPortLength);
        if (DebugPort != NULL && DebugPortLength > 10)
        {
            /* Move to the debug port name */
            DebugPort += 10; DebugPortLength -= 10;
        }
        else
        {
            /* Default to COM */
            DebugPort = "COM"; DebugPortLength = 3;
        }

        /* It is booting in debug mode, show the banner */
        TuiPrintf("You need to connect a debugger on port %.*s\n"
                  "For more information, visit https://reactos.org/wiki/Debugging.\n",
                  DebugPortLength, DebugPort);
    }

    /* Debugging... */
    //DumpMemoryAllocMap();

    /* Do the machine specific initialization */
    WinLdrSetupMachineDependent(LoaderBlock);

    /* Map pages and create memory descriptors */
    WinLdrSetupMemoryLayout(LoaderBlock);

    /* Set processor context */
    WinLdrSetProcessorContext(OperatingSystemVersion);

    /* Save final value of LoaderPagesSpanned */
    LoaderBlock->Extension->LoaderPagesSpanned = MmGetLoaderPagesSpanned();

    TRACE("Hello from paged mode, KiSystemStartup %p, LoaderBlockVA %p!\n",
          KiSystemStartup, LoaderBlockVA);

    /* Zero KI_USER_SHARED_DATA page */
    RtlZeroMemory((PVOID)KI_USER_SHARED_DATA, MM_PAGE_SIZE);

    WinLdrpDumpMemoryDescriptors(LoaderBlockVA);
    WinLdrpDumpBootDriver(LoaderBlockVA);
#ifndef _M_AMD64
    WinLdrpDumpArcDisks(LoaderBlockVA);
#endif

    /* Pass control */
    (*KiSystemStartup)(LoaderBlockVA);

    UNREACHABLE; // return ESUCCESS;
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
