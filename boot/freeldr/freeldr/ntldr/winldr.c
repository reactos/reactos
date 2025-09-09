/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Windows-compatible NT OS Loader.
 * COPYRIGHT:   Copyright 2006-2019 Aleksey Bragin <aleksey@reactos.org>
 */

#include <freeldr.h>
#include <ndk/ldrtypes.h>
#include "winldr.h"
#include "ntldropts.h"
#include "registry.h"
#include <internal/cmboot.h>

// AGENT-MODIFIED: Include UEFI ARC functions header for UEFI boot support
#ifdef UEFIBOOT
#include <uefildr.h>
#include <uefi/uefiarcname.h>
extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
#endif

#include <debug.h>
DBG_DEFAULT_CHANNEL(WINDOWS);

ULONG ArcGetDiskCount(VOID);
PARC_DISK_SIGNATURE_EX ArcGetDiskInfo(ULONG Index);

BOOLEAN IsAcpiPresent(VOID);

extern HEADLESS_LOADER_BLOCK LoaderRedirectionInformation;
extern BOOLEAN WinLdrTerminalConnected;
extern VOID WinLdrSetupEms(IN PCSTR BootOptions);

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
    
    /* Set boot type flags */
#ifdef UEFIBOOT
    /* Mark this as a UEFI boot and pass framebuffer info */
    Extension->BootViaEFI = TRUE;
    Extension->BootViaWinload = FALSE;
    
    /* Pass UEFI GOP framebuffer info if available */
    {
        /* The framebuffer struct is defined in uefildr.h for UEFI builds */
        typedef struct {
            ULONG_PTR BaseAddress;
            ULONG BufferSize;
            UINT32 ScreenWidth;
            UINT32 ScreenHeight;
            UINT32 PixelsPerScanLine;
            UINT32 PixelFormat;
        } REACTOS_INTERNAL_BGCONTEXT;
        
        /* These externs are only available in UEFI builds */
        extern REACTOS_INTERNAL_BGCONTEXT framebufferData;
        extern BOOLEAN UefiVideoInitialized;
        
        if (UefiVideoInitialized)
        {
            /* Copy framebuffer information to loader extension */
            Extension->UefiFramebuffer.FrameBufferBase.QuadPart = framebufferData.BaseAddress;
            Extension->UefiFramebuffer.FrameBufferSize = framebufferData.BufferSize;
            Extension->UefiFramebuffer.ScreenWidth = framebufferData.ScreenWidth;
            Extension->UefiFramebuffer.ScreenHeight = framebufferData.ScreenHeight;
            Extension->UefiFramebuffer.PixelsPerScanLine = framebufferData.PixelsPerScanLine;
            Extension->UefiFramebuffer.PixelFormat = framebufferData.PixelFormat;
            
            /* Log framebuffer info for debugging */
            TRACE("UEFI GOP initialized: %ux%u @ 0x%llx (size=%u)\n",
                (unsigned int)framebufferData.ScreenWidth, 
                (unsigned int)framebufferData.ScreenHeight,
                (unsigned long long)framebufferData.BaseAddress, 
                (unsigned int)framebufferData.BufferSize);
        }
        else
        {
            TRACE("UEFI video not initialized\n");
        }
    }
#else
    /* Legacy BIOS boot - no framebuffer info */
    Extension->BootViaEFI = FALSE;
    Extension->BootViaWinload = FALSE;
#endif

    /* Init three critical lists, used right away */
    InitializeListHead(&LoaderBlock->LoadOrderListHead);
    InitializeListHead(&LoaderBlock->MemoryDescriptorListHead);
    InitializeListHead(&LoaderBlock->BootDriverListHead);

    *OutLoaderBlock = LoaderBlock;
}

// Init "phase 1"
VOID
WinLdrInitializePhase1(PLOADER_PARAMETER_BLOCK LoaderBlock,
                       PCSTR Options,
                       PCSTR SystemRoot,
                       PCSTR BootPath,
                       USHORT VersionToBoot)
{
    /*
     * Examples of correct options and paths:
     * CHAR Options[] = "/DEBUGPORT=COM1 /BAUDRATE=115200";
     * CHAR Options[] = "/NODEBUG";
     * CHAR SystemRoot[] = "\\WINNT\\";
     * CHAR ArcBoot[] = "multi(0)disk(0)rdisk(0)partition(1)";
     */

    PSTR  LoadOptions, NewLoadOptions;
    CHAR  HalPath[] = "\\";
    CHAR  ArcBoot[MAX_PATH+1];
    CHAR  MiscFiles[MAX_PATH+1];
    ULONG i;
    ULONG_PTR PathSeparator;
    PLOADER_PARAMETER_EXTENSION Extension;

    // AGENT-MODIFIED: Use UEFI-specific boot partition detection if running under UEFI
#ifdef UEFIBOOT
    if (GlobalSystemTable != NULL)
    {
        ULONG RDiskNumber = 0;
        ULONG PartitionNumber = 1;
        
        /* Get boot partition info from UEFI */
        if (UefiGetBootPartitionInfo(&RDiskNumber, &PartitionNumber, ArcBoot, sizeof(ArcBoot)))
        {
            TRACE("UEFI Boot Device: %s\n", ArcBoot);
        }
        else
        {
            /* Fallback to parsing the BootPath */
            PathSeparator = strstr(BootPath, "\\") - BootPath;
            RtlStringCbCopyNA(ArcBoot, sizeof(ArcBoot), BootPath, PathSeparator);
            TRACE("Using fallback ArcBoot: '%s'\n", ArcBoot);
        }
    }
    else
#endif
    {
        /* Construct SystemRoot and ArcBoot from SystemPath */
        PathSeparator = strstr(BootPath, "\\") - BootPath;
        RtlStringCbCopyNA(ArcBoot, sizeof(ArcBoot), BootPath, PathSeparator);
    }

    TRACE("ArcBoot: '%s'\n", ArcBoot);
    TRACE("SystemRoot: '%s'\n", SystemRoot);
    TRACE("Options: '%s'\n", Options);

    /* Fill ARC BootDevice */
    LoaderBlock->ArcBootDeviceName = WinLdrSystemBlock->ArcBootDeviceName;
    RtlStringCbCopyA(LoaderBlock->ArcBootDeviceName, sizeof(WinLdrSystemBlock->ArcBootDeviceName), ArcBoot);
    LoaderBlock->ArcBootDeviceName = PaToVa(LoaderBlock->ArcBootDeviceName);

//
// IMPROVE!!
// SetupBlock->ArcSetupDeviceName must be the path to the setup **SOURCE**,
// and not the setup boot path. Indeed they may differ!!
//
    if (LoaderBlock->SetupLdrBlock)
    {
        PSETUP_LOADER_BLOCK SetupBlock = LoaderBlock->SetupLdrBlock;

        /* Adjust the ARC path in the setup block - Matches ArcBoot path */
        SetupBlock->ArcSetupDeviceName = WinLdrSystemBlock->ArcBootDeviceName;
        SetupBlock->ArcSetupDeviceName = PaToVa(SetupBlock->ArcSetupDeviceName);

        /* Convert the setup block pointer */
        LoaderBlock->SetupLdrBlock = PaToVa(LoaderBlock->SetupLdrBlock);
    }

    /* Fill ARC HalDevice, it matches ArcBoot path */
    LoaderBlock->ArcHalDeviceName = WinLdrSystemBlock->ArcBootDeviceName;
    LoaderBlock->ArcHalDeviceName = PaToVa(LoaderBlock->ArcHalDeviceName);

    /* Fill SystemRoot */
    LoaderBlock->NtBootPathName = WinLdrSystemBlock->NtBootPathName;
    RtlStringCbCopyA(LoaderBlock->NtBootPathName, sizeof(WinLdrSystemBlock->NtBootPathName), SystemRoot);
    LoaderBlock->NtBootPathName = PaToVa(LoaderBlock->NtBootPathName);

    /* Fill NtHalPathName */
    LoaderBlock->NtHalPathName = WinLdrSystemBlock->NtHalPathName;
    RtlStringCbCopyA(LoaderBlock->NtHalPathName, sizeof(WinLdrSystemBlock->NtHalPathName), HalPath);
    LoaderBlock->NtHalPathName = PaToVa(LoaderBlock->NtHalPathName);

    /* Fill LoadOptions and strip the '/' switch symbol in front of each option */
    NewLoadOptions = LoadOptions = LoaderBlock->LoadOptions = WinLdrSystemBlock->LoadOptions;
    RtlStringCbCopyA(LoaderBlock->LoadOptions, sizeof(WinLdrSystemBlock->LoadOptions), Options);

    do
    {
        while (*LoadOptions == '/')
            ++LoadOptions;

        *NewLoadOptions++ = *LoadOptions;
    } while (*LoadOptions++);

    LoaderBlock->LoadOptions = PaToVa(LoaderBlock->LoadOptions);

    /* ARC devices */
    LoaderBlock->ArcDiskInformation = &WinLdrSystemBlock->ArcDiskInformation;
    InitializeListHead(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead);

    // AGENT-MODIFIED: Use UEFI-specific ARC disk initialization if running under UEFI
#ifdef UEFIBOOT
    if (GlobalSystemTable != NULL)
    {
        /* Use UEFI-specific ARC disk initialization */
        if (!UefiInitializeArcDisks(LoaderBlock))
        {
            ERR("UefiInitializeArcDisks() failed\n");
        }
    }
    else
#endif
    {
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

    if (VersionToBoot >= _WIN32_WINNT_VISTA)
    {
        Extension->BootViaWinload = 1;
        Extension->LoaderPerformanceData = PaToVa(&WinLdrSystemBlock->LoaderPerformanceData);

        InitializeListHead(&Extension->BootApplicationPersistentData);
        List_PaToVa(&Extension->BootApplicationPersistentData);
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
    RtlStringCbCopyA(MiscFiles, sizeof(MiscFiles), BootPath);
    RtlStringCbCatA(MiscFiles, sizeof(MiscFiles), "AppPatch\\drvmain.sdb");
    Extension->DrvDBImage = PaToVa(WinLdrLoadModule(MiscFiles,
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

    /* With increased heap size (128MB), we can load all drivers
     * Only skip truly problematic or unnecessary drivers */
    {
        /* Skip only known problematic drivers that might cause issues */
        if (_stricmp(DllName, "nmidebug.sys") == 0 ||  /* NMI Debug driver - can cause issues */
            _stricmp(DllName, "sacdrv.sys") == 0)       /* Special Admin Console - not needed */
        {
            TRACE("Skipping problematic driver %s\n", DllName);
            *DriverDTE = NULL;
            return TRUE;
        }
    }

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
    IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PCCH Path,
    IN PCCH File,
    IN PCCH ImportName, // BaseDllName
    IN TYPE_OF_MEMORY MemoryType,
    OUT PLDR_DATA_TABLE_ENTRY *Dte,
    IN ULONG Percentage)
{
    BOOLEAN Success;
    CHAR FullFileName[MAX_PATH];
    CHAR ProgressString[256];
    PVOID BaseAddress;

    TRACE("LoadModule: Path='%s', File='%s', ImportName='%s'\n", Path, File, ImportName);
    
    RtlStringCbPrintfA(ProgressString, sizeof(ProgressString), "Loading %s...", File);
    UiUpdateProgressBar(Percentage, ProgressString);

    RtlStringCbCopyA(FullFileName, sizeof(FullFileName), Path);
    RtlStringCbCatA(FullFileName, sizeof(FullFileName), File);

    TRACE("LoadModule: FullFileName='%s'\n", FullFileName);
    NtLdrOutputLoadMsg(FullFileName, NULL);
    
    TRACE("LoadModule: About to call PeLdrLoadImage('%s')\n", FullFileName);
    Success = PeLdrLoadImage(FullFileName, MemoryType, &BaseAddress);
    if (!Success)
    {
        ERR("PeLdrLoadImage('%s') failed\n", File);
        return NULL;
    }
    TRACE("%s loaded successfully at %p\n", File, BaseAddress);

    TRACE("LoadModule: About to call PeLdrAllocateDataTableEntry for '%s'\n", ImportName);
    Success = PeLdrAllocateDataTableEntry(&LoaderBlock->LoadOrderListHead,
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
    TRACE("LoadModule: DataTableEntry allocated, Dte=%p\n", *Dte);
    
    if (*Dte)
    {
        TRACE("LoadModule: Dte->DllBase=%p, Dte->EntryPoint=%p\n", 
              (*Dte)->DllBase, (*Dte)->EntryPoint);
    }

    /* Init security cookie */
    TRACE("LoadModule: About to init security cookie\n");
    PeLdrInitSecurityCookie(*Dte);
    TRACE("LoadModule: Security cookie initialized\n");

    TRACE("LoadModule: Returning BaseAddress=%p for '%s'\n", BaseAddress, File);
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
    PVOID KernelBase, HalBase, KdDllBase = NULL;
    PLDR_DATA_TABLE_ENTRY HalDTE, KdDllDTE = NULL;
    CHAR DirPath[MAX_PATH];
    CHAR HalFileName[MAX_PATH];
    CHAR KernelFileName[MAX_PATH];
    CHAR KdDllName[MAX_PATH];

    if (!KernelDTE) return FALSE;

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
     * Check for any "/HAL=" or "/KERNEL=" override option.
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
    TRACE("*** LOADING KERNEL: KernelFileName='%s', DirPath='%s' ***\n", KernelFileName, DirPath);
    KernelBase = LoadModule(LoaderBlock, DirPath, KernelFileName,
                            "ntoskrnl.exe", LoaderSystemCode, KernelDTE, 30);
    if (!KernelBase)
    {
        ERR("LoadModule('%s') failed\n", KernelFileName);
        UiMessageBox("Could not load %s", KernelFileName);
        return FALSE;
    }
    TRACE("*** KERNEL LOADED: Base=%p, KernelDTE=%p ***\n", KernelBase, *KernelDTE);
    if (*KernelDTE)
    {
        TRACE("*** KERNEL DTE: DllBase=%p, EntryPoint=%p ***\n", 
              (*KernelDTE)->DllBase, (*KernelDTE)->EntryPoint);
        
        /* Log the physical address of the kernel for debugging */
        {
            PVOID KernelPhysical = VaToPa((*KernelDTE)->DllBase);
            TRACE("*** KERNEL Physical Base: %p (from VA %p) ***\n", 
                  KernelPhysical, (*KernelDTE)->DllBase);
        }
    }

    /* Load the HAL */
    HalBase = LoadModule(LoaderBlock, DirPath, HalFileName,
                         "hal.dll", LoaderHalCode, &HalDTE, 35);
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
        KdDllBase = LoadModule(LoaderBlock, DirPath, KdDllName,
                               "kdcom.dll", LoaderSystemCode, &KdDllDTE, 40);
        if (!KdDllBase)
        {
            /* If we failed to load a custom KD DLL, fall back to the standard one */
            if (IsCustomKdDll)
            {
                /* The custom KD DLL being optional, just ignore the failure */
                WARN("LoadModule('%s') failed\n", KdDllName);

                IsCustomKdDll = FALSE;
                RtlStringCbCopyA(KdDllName, sizeof(KdDllName), "kdcom.dll");

                KdDllBase = LoadModule(LoaderBlock, DirPath, KdDllName,
                                       "kdcom.dll", LoaderSystemCode, &KdDllDTE, 40);
            }

            if (!KdDllBase)
            {
                /* Ignore the failure; we will fail later when scanning the
                 * kernel import tables, if it really needs the KD DLL. */
                ERR("LoadModule('%s') failed\n", KdDllName);
            }
        }
    }

    /* Load all referenced DLLs for Kernel, HAL and Kernel Debugger Transport DLL */
    Success = PeLdrScanImportDescriptorTable(&LoaderBlock->LoadOrderListHead, DirPath, *KernelDTE);
    if (!Success)
    {
        UiMessageBox("Could not load %s", KernelFileName);
        goto Quit;
    }
    Success = PeLdrScanImportDescriptorTable(&LoaderBlock->LoadOrderListHead, DirPath, HalDTE);
    if (!Success)
    {
        UiMessageBox("Could not load %s", HalFileName);
        goto Quit;
    }
    if (KdDllDTE)
    {
        Success = PeLdrScanImportDescriptorTable(&LoaderBlock->LoadOrderListHead, DirPath, KdDllDTE);
        if (!Success)
        {
            UiMessageBox("Could not load %s", KdDllName);
            goto Quit;
        }
    }

Quit:
    if (!Success)
    {
        /* Cleanup and bail out */
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
    CHAR BootOptions[256];

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

    /* Append boot-time options */
    AppendBootTimeOptions(BootOptions);

    /*
     * Set the "/HAL=" and "/KERNEL=" options if needed.
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

    TRACE("BootOptions: '%s'\n", BootOptions);

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
                                    BootPath);
}

ARC_STATUS
LoadAndBootWindowsCommon(
    IN USHORT OperatingSystemVersion,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PCSTR BootOptions,
    IN PCSTR BootPath)
{
    PLOADER_PARAMETER_BLOCK LoaderBlockVA;
    BOOLEAN Success;
    PLDR_DATA_TABLE_ENTRY KernelDTE;
    KERNEL_ENTRY_POINT KiSystemStartup;
    PCSTR SystemRoot;

    TRACE("LoadAndBootWindowsCommon()\n");

    ASSERT(OperatingSystemVersion != 0);

#ifdef _M_IX86
    /* Setup redirection support */
    WinLdrSetupEms(BootOptions);
#endif

    /* Convert BootPath to SystemRoot */
    SystemRoot = strstr(BootPath, "\\");

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
    TRACE("About to call WinLdrInitializePhase1()\n");
    WinLdrInitializePhase1(LoaderBlock,
                           BootOptions,
                           SystemRoot,
                           BootPath,
                           OperatingSystemVersion);
    TRACE("Returned from WinLdrInitializePhase1()\n");

    TRACE("About to call UiUpdateProgressBar(100)\n");
    UiUpdateProgressBar(100, NULL);
    TRACE("Returned from UiUpdateProgressBar()\n");

    /* NOTE: We cannot access KernelDTE->EntryPoint yet because it contains virtual addresses
     * that aren't mapped until WinLdrSetupMemoryLayout() sets up the page tables.
     * We'll get the entry point after page tables are set up. */
    TRACE("KernelDTE saved at %p (will get entry point after paging setup)\n", KernelDTE);
    
    /* For now, just save the LoaderBlock virtual address */
    LoaderBlockVA = PaToVa(LoaderBlock);
    TRACE("LoaderBlockVA set to %p (from %p)\n", LoaderBlockVA, LoaderBlock);

    /* Show the "debug mode" notice if needed BEFORE exiting boot services */
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

    /* "Stop all motors", change videomode - This exits UEFI boot services! */
    TRACE("About to call MachPrepareForReactOS() to exit boot services\n");
    MachPrepareForReactOS();
    TRACE("Returned from MachPrepareForReactOS() - Boot services should be exited now\n");
    /* Boot services are now exited - no more UEFI calls allowed! */

    /* Debugging... */
    //DumpMemoryAllocMap();

    /* Do the machine specific initialization */
    TRACE("About to call WinLdrSetupMachineDependent()\n");
    WinLdrSetupMachineDependent(LoaderBlock);
    TRACE("Returned from WinLdrSetupMachineDependent()\n");

    /* Map pages and create memory descriptors */
    TRACE("About to call WinLdrSetupMemoryLayout()\n");
    WinLdrSetupMemoryLayout(LoaderBlock);
    TRACE("Returned from WinLdrSetupMemoryLayout()\n");

    /* Set processor context */
    TRACE("About to call WinLdrSetProcessorContext()\n");
    WinLdrSetProcessorContext(OperatingSystemVersion);
    TRACE("Returned from WinLdrSetProcessorContext()\n");

    /* NOW we can safely access the kernel entry point after page tables are set up */
    TRACE("Page tables set up, now getting kernel entry point from KernelDTE\n");
    KiSystemStartup = (KERNEL_ENTRY_POINT)KernelDTE->EntryPoint;
    TRACE("KiSystemStartup = %p\n", KiSystemStartup);
    
    /* Verify kernel mapping */
    {
        PVOID KernelPhysical = VaToPa(KernelDTE->DllBase);
        PVOID EntryPhysical = VaToPa((PVOID)KiSystemStartup);
        PIMAGE_NT_HEADERS NtHeaders;
        
        TRACE("*** KERNEL MAPPING CHECK ***\n");
        TRACE("    Kernel VA Base: %p -> PA: %p\n", KernelDTE->DllBase, KernelPhysical);
        TRACE("    Entry Point VA: %p -> PA: %p\n", KiSystemStartup, EntryPhysical);
        TRACE("    Offset from base: 0x%lx\n", 
              (ULONG_PTR)KiSystemStartup - (ULONG_PTR)KernelDTE->DllBase);
        
        /* Check the PE header to verify entry point */
        NtHeaders = RtlImageNtHeader(KernelPhysical);
        if (NtHeaders)
        {
            TRACE("    PE Entry Point RVA: 0x%lx\n", NtHeaders->OptionalHeader.AddressOfEntryPoint);
            TRACE("    PE Image Base: 0x%llx\n", (ULONG64)NtHeaders->OptionalHeader.ImageBase);
            TRACE("    Calculated Entry: %p\n", 
                  (PVOID)((ULONG_PTR)KernelDTE->DllBase + NtHeaders->OptionalHeader.AddressOfEntryPoint));
        }
    }
    
    /* Validate kernel entry point */
    if (KiSystemStartup == NULL)
    {
        ERR("FATAL: Kernel entry point is NULL!\n");
        UiMessageBox("Failed to get kernel entry point");
        return ENOEXEC;
    }

    /* Save final value of LoaderPagesSpanned */
    TRACE("About to save LoaderPagesSpanned\n");
    LoaderBlock->Extension->LoaderPagesSpanned = MmGetLoaderPagesSpanned();
    TRACE("LoaderPagesSpanned saved as %lu\n", LoaderBlock->Extension->LoaderPagesSpanned);

    TRACE("Hello from paged mode, KiSystemStartup %p, LoaderBlockVA %p!\n",
          KiSystemStartup, LoaderBlockVA);

    /* Zero KI_USER_SHARED_DATA page */
    TRACE("About to zero KI_USER_SHARED_DATA page at %p\n", (PVOID)KI_USER_SHARED_DATA);
    RtlZeroMemory((PVOID)KI_USER_SHARED_DATA, MM_PAGE_SIZE);
    TRACE("KI_USER_SHARED_DATA page zeroed\n");

    TRACE("About to call WinLdrpDumpMemoryDescriptors()\n");
    WinLdrpDumpMemoryDescriptors(LoaderBlockVA);
    TRACE("Returned from WinLdrpDumpMemoryDescriptors()\n");
    
    TRACE("About to call WinLdrpDumpBootDriver()\n");
    WinLdrpDumpBootDriver(LoaderBlockVA);
    TRACE("Returned from WinLdrpDumpBootDriver()\n");
    
#ifndef _M_AMD64
    TRACE("About to call WinLdrpDumpArcDisks()\n");
    WinLdrpDumpArcDisks(LoaderBlockVA);
    TRACE("Returned from WinLdrpDumpArcDisks()\n");
#endif

    /* Pass control */
    TRACE("*** ABOUT TO JUMP TO KERNEL ***\n");
    TRACE("*** KiSystemStartup = %p ***\n", KiSystemStartup);
    TRACE("*** LoaderBlockVA = %p ***\n", LoaderBlockVA);
    
    /* Final sanity check */
    if ((ULONG_PTR)KiSystemStartup < KSEG0_BASE)
    {
        ERR("FATAL: KiSystemStartup address %p is not in kernel space!\n", KiSystemStartup);
        TRACE("Expected kernel address >= %p\n", (PVOID)KSEG0_BASE);
        for(;;); /* Hang */
    }
    
#if defined(_M_AMD64) || defined(_M_X64) || defined(__x86_64__)
    /* For AMD64, ensure proper state before kernel handoff */
    {
#ifdef UEFIBOOT
        extern BOOLEAN UefiBootServicesExited;
        
        if (!UefiBootServicesExited)
        {
            TRACE("WARNING: Boot services not properly exited before kernel handoff!\n");
            /* Try to exit them now */
            MachPrepareForReactOS();
        }
#endif
        
        /* Ensure the kernel entry point is accessible */
        {
            /* Get the physical address where kernel should be */
            ULONG64 KernelEntryVA = (ULONG64)KiSystemStartup;
            ULONG64 KernelEntryPA = KernelEntryVA - KSEG0_BASE;
            ULONG64 KernelPagePA = KernelEntryPA & ~(PAGE_SIZE - 1);
            ULONG64 KernelPageVA = KernelEntryVA & ~(PAGE_SIZE - 1);
            
            TRACE("*** KERNEL ENTRY POINT MAPPING ***\n");
            TRACE("    Entry VA: 0x%llx\n", KernelEntryVA);
            TRACE("    Entry PA: 0x%llx\n", KernelEntryPA);
            TRACE("    Page VA: 0x%llx\n", KernelPageVA);
            TRACE("    Page PA: 0x%llx\n", KernelPagePA);
            
            /* Try to read bytes from the entry point to verify it's mapped and contains code */
            {
                volatile UCHAR TestBytes[16];
                int i;
                TRACE("    Testing read from kernel entry point...\n");
                for (i = 0; i < 16; i++)
                {
                    TestBytes[i] = *((volatile UCHAR*)KiSystemStartup + i);
                }
                TRACE("    Read successful, first 16 bytes at entry:");
                for (i = 0; i < 16; i++)
                {
                    TRACE(" %02x", TestBytes[i]);
                }
                TRACE("\n");
                
                /* Check if this looks like valid x64 code */
                if (TestBytes[0] == 0x41 && TestBytes[1] == 0x50)
                {
                    TRACE("    WARNING: Bytes look like ASCII 'AP' not x64 code!\n");
                }
                else if (TestBytes[0] == 0x48 || TestBytes[0] == 0x4C || 
                         TestBytes[0] == 0x55 || TestBytes[0] == 0x53)
                {
                    TRACE("    Bytes look like valid x64 code (REX prefix or PUSH)\n");
                }
            }
        }
        
        /* Ensure interrupts are disabled */
        _disable();
        
        /* Memory barrier to ensure all writes are complete */
        MemoryBarrier();
        
        /* Verify critical control registers */
        {
            ULONG64 cr0, cr3, cr4, efer;
            __asm__ __volatile__("movq %%cr0, %0" : "=r"(cr0));
            __asm__ __volatile__("movq %%cr3, %0" : "=r"(cr3));
            __asm__ __volatile__("movq %%cr4, %0" : "=r"(cr4));
            
            /* Read EFER MSR */
            efer = __readmsr(0xC0000080);
            
            TRACE("*** Control registers before jump: CR0=%llx, CR3=%llx, CR4=%llx ***\n", cr0, cr3, cr4);
            TRACE("*** EFER MSR: %llx (LME=%d, LMA=%d, NXE=%d) ***\n", 
                  efer, !!(efer & 0x100), !!(efer & 0x400), !!(efer & 0x800));
            
            /* Ensure paging is enabled */
            if (!(cr0 & 0x80000000))
            {
                ERR("FATAL: Paging not enabled! CR0=%llx\n", cr0);
                for(;;);
            }
            
            /* Ensure Long Mode is active */
            if (!(efer & 0x400))
            {
                ERR("FATAL: Long Mode not active! EFER=%llx\n", efer);
                for(;;);
            }
        }
        
        /* Flush TLB to ensure all mappings are visible */
        {
            ULONG64 cr3;
            __asm__ __volatile__("movq %%cr3, %0" : "=r"(cr3));
            __writecr3(cr3);
        }
        
        /* Flush instruction cache */
        __wbinvd();
        
        /* Ensure stack is 16-byte aligned for AMD64 calling convention */
        {
            ULONG64 rsp;
            __asm__ __volatile__("movq %%rsp, %0" : "=r"(rsp));
            TRACE("Current RSP before alignment: %llx\n", rsp);
            
            /* Align RSP to 16-byte boundary and leave space for red zone */
            rsp = (rsp & ~0xF) - 0x8;  /* Align and adjust for call instruction */
            TRACE("Aligned RSP: %llx\n", rsp);
            
            /* Set the aligned stack */
            __asm__ __volatile__("movq %0, %%rsp" : : "r"(rsp));
        }
    }
#endif
    
    TRACE("*** Calling (*KiSystemStartup)(LoaderBlockVA) NOW! ***\n");
    TRACE("*** THIS IS THE LAST MESSAGE FROM FREELDR ***\n");
    
    /* Add immediate debug output to verify we reach this point */
    TRACE("*** DEBUG: Line 1570 reached, about to enter architecture-specific code ***\n");
    
#if defined(_M_AMD64) || defined(_M_X64) || defined(__x86_64__)
    TRACE("*** DEBUG: AMD64 path selected (entering block at line 1573) ***\n");
    /* For AMD64, perform direct call with proper ABI */
    {
        TRACE("*** DEBUG: Inside AMD64 block (line 1575) ***\n");
        
        /* Skip segment setup in UEFI mode - segments are already correct */
        /* In UEFI boot, we're already in long mode with proper segments */
        TRACE("*** DEBUG: Skipping segment setup in UEFI mode (segments already correct) ***\n");
        
        /* Verify we're in a valid code segment */
        {
            USHORT CurrentCS;
            __asm__ __volatile__("movw %%cs, %0" : "=r"(CurrentCS));
            TRACE("Current CS before jump: 0x%x (0x38 is valid for UEFI mode)\n", CurrentCS);
            
            /* In UEFI mode, CS=0x38 is perfectly valid - it's the UEFI long mode code segment
             * The kernel will set up its own segments after taking control */
        }
        
        /* Clear direction flag as required by ABI */
        __asm__ __volatile__("cld");
        
        /* Verify LoaderBlock is valid before jumping */
        if (LoaderBlockVA == NULL)
        {
            ERR("FATAL: LoaderBlockVA is NULL!\n");
            for(;;);
        }
        
        /* Verify LoaderBlock contents */
        {
            PLOADER_PARAMETER_BLOCK TestBlock = (PLOADER_PARAMETER_BLOCK)LoaderBlockVA;
            TRACE("LoaderBlock verification:\n");
            TRACE("    LoaderBlock at VA = %p\n", TestBlock);
            TRACE("    LoaderBlock->LoadOrderListHead = %p\n", &TestBlock->LoadOrderListHead);
            TRACE("    LoaderBlock->MemoryDescriptorListHead = %p\n", &TestBlock->MemoryDescriptorListHead);
            TRACE("    LoaderBlock->KernelStack = %p\n", TestBlock->KernelStack);
            TRACE("    LoaderBlock->Process = %p\n", TestBlock->Process);
            TRACE("    LoaderBlock->Thread = %p\n", TestBlock->Thread);
        }
        
        /* Jump to kernel with Windows x64 ABI - LoaderBlock in RCX */
        TRACE("*** About to execute final jump to kernel entry point ***\n");
        TRACE("    RCX (LoaderBlock) = %p\n", LoaderBlockVA);
        TRACE("    Target address = %p\n", KiSystemStartup);
        
        /* Simple direct jump - we're already in long mode with correct segments */
        TRACE("*** Performing simple direct jump to kernel ***\n");
        
        /* One more verification that the kernel is mapped */
        {
            volatile UCHAR TestByte;
            TRACE("*** Final kernel mapping check before jump ***\n");
            TestByte = *(volatile UCHAR *)KiSystemStartup;
            TRACE("    First byte at entry point: 0x%02x\n", TestByte);
        }
        
        /* Check IDT is set */
        {
            KDESCRIPTOR IdtDesc;
            __sidt(&IdtDesc.Limit);
            TRACE("*** IDT check: Base=%p, Limit=0x%x ***\n", IdtDesc.Base, IdtDesc.Limit);
        }
        
        /* Check stack pointer */
        {
            ULONG64 CurrentRSP;
            __asm__ __volatile__("movq %%rsp, %0" : "=r"(CurrentRSP));
            TRACE("*** Current RSP before call: 0x%llx ***\n", CurrentRSP);
        }
        
        /* Set IF flag to disable interrupts */
        _disable();
        
        /* Set up MSRs for kernel */
        {
            ULONG64 efer = __readmsr(0xC0000080);
            
            /* Enable SYSCALL/SYSRET if not already set */
            if (!(efer & 1))  /* Check SCE bit */
            {
                TRACE("*** Enabling SYSCALL/SYSRET (SCE bit) in EFER ***\n");
                efer |= 1;  /* Set SCE bit */
                __writemsr(0xC0000080, efer);
            }
            
            /* Set up SYSCALL MSRs with kernel segments */
            /* STAR MSR (0xC0000081) - SYSCALL CS/SS */
            /* Upper 32 bits: SYSRET CS/SS (+16/+8), Lower 32 bits: SYSCALL CS/SS */
            __writemsr(0xC0000081, 0x0023001000000000ULL);  /* Kernel CS=0x10, User CS=0x23 */
            
            /* LSTAR MSR (0xC0000082) - SYSCALL RIP target */
            /* Will be set by kernel to its system call handler */
            __writemsr(0xC0000082, 0);
            
            /* SFMASK MSR (0xC0000084) - SYSCALL RFLAGS mask */
            __writemsr(0xC0000084, 0x47700);  /* Standard NT flags mask */
            
            TRACE("*** MSRs configured for kernel ***\n");
        }
        
        /* Note: We can't switch to kernel stack here because it's in kernel space
         * and we're still running in identity-mapped space. The kernel will
         * switch to its own stack after it starts executing. */
        TRACE("*** Kernel stack prepared at %p for kernel to use ***\n", 
              LoaderBlockVA ? ((PLOADER_PARAMETER_BLOCK)LoaderBlockVA)->KernelStack : 0);
        
        /* In UEFI mode, we're using UEFI's segments until we jump to kernel
         * The kernel will set up its own segments after the jump */
        TRACE("*** Using UEFI segment configuration (CS=0x38) ***\n");
        
        /* Final state check before kernel jump */
        {
            ULONG64 cr0, cr3, cr4, efer, rsp;
            __asm__ __volatile__("movq %%cr0, %0" : "=r"(cr0));
            __asm__ __volatile__("movq %%cr3, %0" : "=r"(cr3));
            __asm__ __volatile__("movq %%cr4, %0" : "=r"(cr4));
            __asm__ __volatile__("movq %%rsp, %0" : "=r"(rsp));
            efer = __readmsr(0xC0000080);
            
            TRACE("*** FINAL STATE BEFORE KERNEL JUMP ***\n");
            TRACE("    CR0=0x%llx (PG=%d, PE=%d, WP=%d)\n", 
                  cr0, !!(cr0 & 0x80000000), !!(cr0 & 1), !!(cr0 & 0x10000));
            TRACE("    CR3=0x%llx\n", cr3);
            TRACE("    CR4=0x%llx (PAE=%d, PSE=%d, PGE=%d)\n",
                  cr4, !!(cr4 & 0x20), !!(cr4 & 0x10), !!(cr4 & 0x80));
            TRACE("    EFER=0x%llx (LME=%d, LMA=%d, NXE=%d, SCE=%d)\n",
                  efer, !!(efer & 0x100), !!(efer & 0x400), !!(efer & 0x800), !!(efer & 1));
            TRACE("    RSP=0x%llx\n", rsp);
        }
        
        /* Verify kernel entry is accessible one more time */
        {
            volatile ULONG64 *TestPtr = (volatile ULONG64 *)KiSystemStartup;
            volatile ULONG64 TestVal;
            TRACE("*** Testing kernel entry point accessibility ***\n");
            TestVal = *TestPtr;
            TRACE("*** First 8 bytes at entry: 0x%016llx ***\n", TestVal);
        }
        
        /* Jump to kernel entry point */
        TRACE("*** JUMPING TO KERNEL NOW AT %p WITH RCX=%p ***\n", KiSystemStartup, LoaderBlockVA);
        
        /* Simple direct jump - avoid far jump complications */
        __asm__ __volatile__(
            /* Set up LoaderBlock parameter in RCX per Windows x64 ABI */
            "movq %0, %%rcx\n\t"
            
            /* Clear other parameter registers */
            "xorq %%rdx, %%rdx\n\t"
            "xorq %%r8, %%r8\n\t"
            "xorq %%r9, %%r9\n\t"
            
            /* Direct jump to kernel entry point */
            "jmpq *%1\n\t"
            : 
            : "r"(LoaderBlockVA), "r"(KiSystemStartup)
            : "rcx", "rdx", "r8", "r9", "memory"
        );
    }
#else
    (*KiSystemStartup)(LoaderBlockVA);
#endif

    TRACE("*** ERROR: Returned from KiSystemStartup - this should never happen! ***\n");
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
