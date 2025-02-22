/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Boot Stores Management functionality, with support for
 *              NT 5.x family (MS Windows <= 2003, and ReactOS) bootloaders.
 * COPYRIGHT:   Copyright 2017-2018 Hermes Belusca-Maito
 */

// TODO: Add support for NT 6.x family! (detection + BCD manipulation).

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "bldrsup.h"
#include "filesup.h"
#include "inicache.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

typedef NTSTATUS
(*POPEN_BOOT_STORE)(
    _Out_ PVOID* Handle,
    _In_ HANDLE PartitionDirectoryHandle, // _In_opt_
    _In_ BOOT_STORE_TYPE Type,
    _In_ BOOT_STORE_OPENMODE OpenMode,
    _In_ BOOT_STORE_ACCESS Access);

typedef NTSTATUS
(*PCLOSE_BOOT_STORE)(
    _In_ PVOID Handle);

typedef NTSTATUS
(*PENUM_BOOT_STORE_ENTRIES)(
    IN PVOID Handle,
//  IN ULONG Flags, // Determine which data to retrieve
    IN PENUM_BOOT_ENTRIES_ROUTINE EnumBootEntriesRoutine,
    IN PVOID Parameter OPTIONAL);

typedef struct _NTOS_BOOT_LOADER_FILES
{
    BOOT_STORE_TYPE Type;
    PCZZWSTR LoaderExecutables;
    PCWSTR LoaderConfigurationFile;
    POPEN_BOOT_STORE OpenBootStore;
    PCLOSE_BOOT_STORE CloseBootStore;
    PENUM_BOOT_STORE_ENTRIES EnumBootStoreEntries;
} NTOS_BOOT_LOADER_FILES, *PNTOS_BOOT_LOADER_FILES;


/*
 * Header for particular store contexts
 */
typedef struct _BOOT_STORE_CONTEXT
{
    BOOT_STORE_TYPE Type;
    BOOLEAN ReadOnly;
//  PNTOS_BOOT_LOADER_FILES ??
/*
    PVOID PrivateData;
*/
} BOOT_STORE_CONTEXT, *PBOOT_STORE_CONTEXT;

typedef struct _BOOT_STORE_INI_CONTEXT
{
    BOOT_STORE_CONTEXT Header;

    /*
     * If all these members are NULL, we know that the store is freshly created
     * and is cached in memory only. At file closure we will therefore need to
     * create the file proper and save its contents.
     */
    HANDLE FileHandle;
    HANDLE SectionHandle;
    // SIZE_T ViewSize;
    ULONG FileSize;
    PVOID ViewBase;

    PINICACHE IniCache;
    PINI_SECTION OptionsIniSection;
    PINI_SECTION OsIniSection;
} BOOT_STORE_INI_CONTEXT, *PBOOT_STORE_INI_CONTEXT;

// TODO!
typedef struct _BOOT_STORE_BCDREG_CONTEXT
{
    BOOT_STORE_CONTEXT Header;
    ULONG PlaceHolder;
} BOOT_STORE_BCDREG_CONTEXT, *PBOOT_STORE_BCDREG_CONTEXT;


static NTSTATUS
OpenIniBootLoaderStore(
    _Out_ PVOID* Handle,
    _In_ HANDLE PartitionDirectoryHandle, // _In_opt_
    _In_ BOOT_STORE_TYPE Type,
    _In_ BOOT_STORE_OPENMODE OpenMode,
    _In_ BOOT_STORE_ACCESS Access);

static NTSTATUS
CloseIniBootLoaderStore(
    _In_ PVOID Handle);

static NTSTATUS
FreeLdrEnumerateBootEntries(
    IN PBOOT_STORE_INI_CONTEXT BootStore,
//  IN ULONG Flags, // Determine which data to retrieve
    IN PENUM_BOOT_ENTRIES_ROUTINE EnumBootEntriesRoutine,
    IN PVOID Parameter OPTIONAL);

static NTSTATUS
NtLdrEnumerateBootEntries(
    IN PBOOT_STORE_INI_CONTEXT BootStore,
//  IN ULONG Flags, // Determine which data to retrieve
    IN PENUM_BOOT_ENTRIES_ROUTINE EnumBootEntriesRoutine,
    IN PVOID Parameter OPTIONAL);


// Question 1: What if config file is optional?
// Question 2: What if many config files are possible?
NTOS_BOOT_LOADER_FILES NtosBootLoaders[] =
{
    {FreeLdr, L"freeldr.sys\0" L"rosload.exe\0", L"freeldr.ini",
        OpenIniBootLoaderStore, CloseIniBootLoaderStore, (PENUM_BOOT_STORE_ENTRIES)FreeLdrEnumerateBootEntries},
    {NtLdr  , L"ntldr\0" L"osloader.exe\0", L"boot.ini",
        OpenIniBootLoaderStore, CloseIniBootLoaderStore, (PENUM_BOOT_STORE_ENTRIES)NtLdrEnumerateBootEntries  },
//  {SetupLdr, L"setupldr\0" L"setupldr.bin\0" L"setupldr.exe\0", L"txtsetup.sif", UNIMPLEMENTED, UNIMPLEMENTED, UNIMPLEMENTED}
//  {BootMgr , L"bootmgr", L"BCD", UNIMPLEMENTED, UNIMPLEMENTED, UNIMPLEMENTED}
};
C_ASSERT(_countof(NtosBootLoaders) == BldrTypeMax);

enum BOOT_OPTION
{
    BO_TimeOut,
    BO_DefaultOS,
};
static const PCWSTR BootOptionNames[][2] =
{
    {L"TimeOut", L"DefaultOS"}, // FreeLdr
    {L"timeout", L"default"  }  // NtLdr
};


/* FUNCTIONS ****************************************************************/

NTSTATUS
FindBootStore( // By handle
    IN HANDLE PartitionDirectoryHandle, // OPTIONAL
    IN BOOT_STORE_TYPE Type,
    OUT PULONG VersionNumber OPTIONAL)
// OUT PHANDLE ConfigFileHande OPTIONAL ????
{
    PCWSTR LoaderExecutable;
    // UINT i;

    if (Type >= BldrTypeMax)
        return STATUS_INVALID_PARAMETER;

    if (VersionNumber)
        *VersionNumber = 0;

    /* Check whether any of the loader executables exist */
    LoaderExecutable = NtosBootLoaders[Type].LoaderExecutables;
    while (*LoaderExecutable)
    {
        if (DoesFileExist(PartitionDirectoryHandle, LoaderExecutable))
        {
            /* A loader was found, stop there */
            DPRINT("Found loader executable '%S'\n", LoaderExecutable);
            break;
        }

        /* The loader does not exist, continue with another one */
        DPRINT("Loader executable '%S' does not exist, continue with another one...\n", LoaderExecutable);
        LoaderExecutable += wcslen(LoaderExecutable) + 1;
    }
    if (!*LoaderExecutable)
    {
        /* No loader was found */
        DPRINT("No loader executable was found\n");
        return STATUS_NOT_FOUND;
    }

    /* Check for loader version if needed */
    if (VersionNumber)
    {
        *VersionNumber = 0;
        // TODO: Check for BLDR version!
    }

    /* Check whether the loader configuration file exists */
#if 0
    Status = OpenAndMapFile(PartitionDirectoryHandle, NtosBootLoaders[Type].LoaderConfigurationFile,
                            &FileHandle, &FileSize, &SectionHandle, &ViewBase, FALSE);
    if (!NT_SUCCESS(Status))
#else
    if (!DoesFileExist(PartitionDirectoryHandle, NtosBootLoaders[Type].LoaderConfigurationFile))
#endif
    {
        /* The loader does not exist, continue with another one */
        // FIXME: Consider it might be optional??
        DPRINT1("Loader configuration file '%S' does not exist\n", NtosBootLoaders[Type].LoaderConfigurationFile);
        return STATUS_NOT_FOUND;
    }

    return STATUS_SUCCESS;
}


//
// TEMPORARY functions to migrate the DEPRECATED BootDrive and BootPartition
// values of BootSector boot entries in FREELDR.INI to the newer BootPath value.
//
// REMOVE THEM once they won't be necessary anymore,
// after the removal of their support in FreeLoader!
//
static VOID
FreeLdrMigrateBootDrivePartWorker(
    _In_ PINI_SECTION OsIniSection)
{
    PCWSTR KeyData;
    PINI_KEYWORD OldKey;

    /*
     * Check whether we have a "BootPath" value (takes precedence
     * over both "BootDrive" and "BootPartition").
     */
    if (IniGetKey(OsIniSection, L"BootPath", &KeyData) && KeyData && *KeyData)
    {
        /* We already have a BootPath value, do nothing more */
        return;
    }

    /* We don't have one: retrieve the BIOS drive and
     * partition and convert them to a valid ARC path */

    /* Retrieve the boot drive */
    OldKey = IniGetKey(OsIniSection, L"BootDrive", &KeyData);
    if (OldKey)
    {
        PCWSTR OldDrive = KeyData;
        ULONG DriveNumber = 0;
        ULONG PartitionNumber = 0;
        UCHAR DriveType = 0;
        WCHAR BufferBootPath[80]; // 80 chars is enough for "multi(0)disk(0)rdisk(x)partition(y)", with (x,y) == MAXULONG

        /* If a number string is given, then just
         * convert it to decimal (BIOS HW only) */
        PCWCH p = KeyData;
        if (p[0] >= L'0' && p[0] <= L'9')
        {
            DriveNumber = wcstoul(p, (PWCHAR*)&p, 0);
            if (DriveNumber >= 0x80)
            {
                /* It's quite probably a hard disk */
                DriveNumber -= 0x80;
                DriveType = L'h';
            }
            else
            {
                /* It's quite probably a floppy */
                DriveType = L'f';
            }
        }
        else if (p[0] && towlower(p[1]) == L'd')
        {
            /* Convert the drive number string into a number: 'hd1' = 1 */
            DriveType = tolower(p[0]);
            DriveNumber = _wtoi(&p[2]);
        }

        /* Retrieve the boot partition (optional, fall back to zero otherwise) */
        if (IniGetKey(OsIniSection, L"BootPartition", &KeyData))
            PartitionNumber = _wtoi(KeyData);

        if (DriveType == L'f')
        {
            /* Floppy disk path: multi(0)disk(0)fdisk(x) */
            RtlStringCchPrintfW(BufferBootPath, _countof(BufferBootPath),
                                L"multi(0)disk(0)fdisk(%lu)", DriveNumber);
        }
        else if (DriveType == L'h')
        {
            /* Hard disk path: multi(0)disk(0)rdisk(x)partition(y) */
            RtlStringCchPrintfW(BufferBootPath, _countof(BufferBootPath),
                                L"multi(0)disk(0)rdisk(%lu)partition(%lu)",
                                DriveNumber, PartitionNumber);
        }
        else if (DriveType == L'c')
        {
            /* CD-ROM disk path: multi(0)disk(0)cdrom(x) */
            RtlStringCchPrintfW(BufferBootPath, _countof(BufferBootPath),
                                L"multi(0)disk(0)cdrom(%lu)", DriveNumber);
        }
        else
        {
            /* This case should rarely happen, if ever */
            DPRINT1("Unrecognized BootDrive type '%C'\n", DriveType ? DriveType : L'?');

            /* Build the boot path in the form: hdX,Y */
            RtlStringCchCopyW(BufferBootPath, _countof(BufferBootPath), OldDrive);
            if (KeyData && *KeyData)
            {
                RtlStringCchCatW(BufferBootPath, _countof(BufferBootPath), L",");
                RtlStringCchCatW(BufferBootPath, _countof(BufferBootPath), KeyData);
            }
        }

        /* Add the new BootPath value */
        IniInsertKey(OsIniSection, OldKey, INSERT_BEFORE, L"BootPath", BufferBootPath);
    }

    /* Delete the deprecated BootDrive and BootPartition values */
    IniRemoveKeyByName(OsIniSection, L"BootDrive");
    IniRemoveKeyByName(OsIniSection, L"BootPartition");
}

static VOID
FreeLdrMigrateBootDrivePart(
    _In_ PBOOT_STORE_INI_CONTEXT BootStore)
{
    PINICACHEITERATOR Iterator;
    PINI_SECTION OsIniSection;
    PCWSTR SectionName, KeyData;

    /* Enumerate all the valid entries in the "Operating Systems" section */
    Iterator = IniFindFirstValue(BootStore->OsIniSection, &SectionName, &KeyData);
    if (!Iterator) return;
    do
    {
        /* Search for an existing boot entry section */
        OsIniSection = IniGetSection(BootStore->IniCache, SectionName);
        if (!OsIniSection)
            continue;

        /* Check for boot type to migrate */
        if (!IniGetKey(OsIniSection, L"BootType", &KeyData) || !KeyData)
        {
            /* Certainly not a ReactOS installation */
            DPRINT1("No BootType value present\n");
            continue;
        }
        if ((_wcsicmp(KeyData, L"Drive")     == 0) ||
            (_wcsicmp(KeyData, L"\"Drive\"") == 0) ||
            (_wcsicmp(KeyData, L"Partition")     == 0) ||
            (_wcsicmp(KeyData, L"\"Partition\"") == 0))
        {
            /* Modify the BootPath value */
            IniAddKey(OsIniSection, L"BootType", L"BootSector");
            goto migrate_drivepart;
        }
        if ((_wcsicmp(KeyData, L"BootSector")     == 0) ||
            (_wcsicmp(KeyData, L"\"BootSector\"") == 0))
        {
migrate_drivepart:
            DPRINT("This is a '%S' boot entry\n", KeyData);
            FreeLdrMigrateBootDrivePartWorker(OsIniSection);
        }
    }
    while (IniFindNextValue(Iterator, &SectionName, &KeyData));

    IniFindClose(Iterator);
}
//////////////


static VOID
CreateCommonFreeLdrSections(
    IN OUT PBOOT_STORE_INI_CONTEXT BootStore)
{
    PINI_SECTION IniSection;

    /*
     * Cache the "FREELOADER" section for our future usage.
     */

    /* Create the "FREELOADER" section */
    IniSection = IniAddSection(BootStore->IniCache, L"FREELOADER");
    if (!IniSection)
        DPRINT1("CreateCommonFreeLdrSections: Failed to create 'FREELOADER' section!\n");

    BootStore->OptionsIniSection = IniSection;

    /* TimeOut */
    IniAddKey(BootStore->OptionsIniSection, L"TimeOut", L"0");

    /* Create "Display" section */
    IniSection = IniAddSection(BootStore->IniCache, L"Display");

    /* TitleText and MinimalUI */
    IniAddKey(IniSection, L"TitleText", L"ReactOS Boot Manager");
    IniAddKey(IniSection, L"MinimalUI", L"Yes");

    /*
     * Cache the "Operating Systems" section for our future usage.
     */

    /* Create the "Operating Systems" section */
    IniSection = IniAddSection(BootStore->IniCache, L"Operating Systems");
    if (!IniSection)
        DPRINT1("CreateCommonFreeLdrSections: Failed to create 'Operating Systems' section!\n");

    BootStore->OsIniSection = IniSection;
}

static NTSTATUS
OpenIniBootLoaderStore(
    _Out_ PVOID* Handle,
    _In_ HANDLE PartitionDirectoryHandle, // _In_opt_
    _In_ BOOT_STORE_TYPE Type,
    _In_ BOOT_STORE_OPENMODE OpenMode,
    _In_ BOOT_STORE_ACCESS Access)
{
    NTSTATUS Status;
    PBOOT_STORE_INI_CONTEXT BootStore;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    ACCESS_MASK DesiredAccess;
    ULONG CreateDisposition;

    //
    // WARNING! We support the INI creation *ONLY* for FreeLdr, and not for NTLDR
    //
    if ((Type == NtLdr) && (OpenMode == BS_CreateNew || OpenMode == BS_CreateAlways || OpenMode == BS_RecreateExisting))
    {
        DPRINT1("OpenIniBootLoaderStore() unsupported for NTLDR\n");
        return STATUS_NOT_SUPPORTED;
    }

    /* Create a boot store structure */
    BootStore = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, sizeof(*BootStore));
    if (!BootStore)
        return STATUS_INSUFFICIENT_RESOURCES;

    BootStore->Header.Type = Type;

    /*
     * So far, we only use the INI cache. The file itself is not created or
     * opened yet, therefore FileHandle, SectionHandle, ViewBase and FileSize
     * are all NULL. We will use this fact to know that the INI file was indeed
     * created, and not just opened as an existing file.
     */
    // BootStore->FileHandle = NULL;
    BootStore->SectionHandle = NULL;
    BootStore->ViewBase = NULL;
    BootStore->FileSize = 0;

    /*
     * Create or open the loader configuration INI file as necessary.
     */
    RtlInitUnicodeString(&Name, NtosBootLoaders[Type].LoaderConfigurationFile);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               PartitionDirectoryHandle,
                               NULL);

    DesiredAccess =
        ((Access & BS_ReadAccess ) ? FILE_GENERIC_READ  : 0) |
        ((Access & BS_WriteAccess) ? FILE_GENERIC_WRITE : 0);

    CreateDisposition = FILE_OPEN;
    switch (OpenMode)
    {
        case BS_CreateNew:
            CreateDisposition = FILE_CREATE;
            break;
        case BS_CheckExisting:
        case BS_OpenExisting:
            CreateDisposition = FILE_OPEN;
            break;
        case BS_OpenAlways:
            CreateDisposition = FILE_OPEN_IF;
            break;
        case BS_RecreateExisting:
            CreateDisposition = FILE_OVERWRITE;
            break;
        case BS_CreateAlways:
            CreateDisposition = FILE_OVERWRITE_IF;
            break;
        default:
            ASSERT(FALSE);
    }

    IoStatusBlock.Information = 0;
    Status = NtCreateFile(&BootStore->FileHandle,
                          DesiredAccess | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ,
                          CreateDisposition,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY | FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0);

    if (OpenMode == BS_CheckExisting)
    {
        /* We just want to check for file existence. If we either succeeded
         * opening the file, or we failed because it exists but we do not
         * currently have access to it, return success in either case. */
        BOOLEAN Success = (NT_SUCCESS(Status) || (Status == STATUS_ACCESS_DENIED));
        if (!Success)
        {
            DPRINT1("Couldn't find Loader configuration file '%S'\n",
                    NtosBootLoaders[Type].LoaderConfigurationFile);
        }
        if (BootStore->FileHandle)
            NtClose(BootStore->FileHandle);
        RtlFreeHeap(ProcessHeap, 0, BootStore);
        return (Success ? STATUS_SUCCESS : Status);
    }

    /*
     * If create/open failed because the file is in read-only mode,
     * change its attributes and re-attempt opening it.
     */
    if (Status == STATUS_ACCESS_DENIED) do
    {
        FILE_BASIC_INFORMATION FileInfo = {0};

        /* Reattempt to open it with limited access */
        Status = NtCreateFile(&BootStore->FileHandle,
                              FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ,
                              FILE_OPEN,
                              FILE_NO_INTERMEDIATE_BUFFERING |
                              FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY | FILE_NON_DIRECTORY_FILE,
                              NULL,
                              0);
        /* Fail for real if we cannot open it that way */
        if (!NT_SUCCESS(Status))
            break;

        /* Reset attributes to normal, no read-only */
        FileInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        /*
         * We basically don't care about whether it succeeds:
         * if it didn't, later open will fail.
         */
        NtSetInformationFile(BootStore->FileHandle, &IoStatusBlock,
                             &FileInfo, sizeof(FileInfo),
                             FileBasicInformation);

        /* Close file */
        NtClose(BootStore->FileHandle);

        /* And re-attempt create/open */
        Status = NtCreateFile(&BootStore->FileHandle,
                              DesiredAccess | SYNCHRONIZE,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ,
                              CreateDisposition,
                              FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY | FILE_NON_DIRECTORY_FILE,
                              NULL,
                              0);
    } while (0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Couldn't open Loader configuration file '%S' (Status 0x%08lx)\n",
                NtosBootLoaders[Type].LoaderConfigurationFile, Status);
        RtlFreeHeap(ProcessHeap, 0, BootStore);
        return Status;
    }

    BootStore->Header.ReadOnly = !(Access & BS_WriteAccess);

    if (IoStatusBlock.Information == FILE_CREATED     || // with: FILE_CREATE, FILE_OVERWRITE_IF, FILE_OPEN_IF, FILE_SUPERSEDE
        IoStatusBlock.Information == FILE_OVERWRITTEN || // with: FILE_OVERWRITE, FILE_OVERWRITE_IF
        IoStatusBlock.Information == FILE_SUPERSEDED)    // with: FILE_SUPERSEDE
    {
        /*
         * The loader configuration INI file is (re)created
         * fresh new, initialize its cache and its contents.
         */
        BootStore->IniCache = IniCacheCreate();
        if (!BootStore->IniCache)
        {
            DPRINT1("IniCacheCreate() failed\n");
            NtClose(BootStore->FileHandle);
            RtlFreeHeap(ProcessHeap, 0, BootStore);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (Type == FreeLdr)
            CreateCommonFreeLdrSections(BootStore);
    }
    else // if (IoStatusBlock.Information == FILE_OPENED) // with: FILE_OPEN, FILE_OPEN_IF
    {
        PINI_SECTION IniSection;

        /*
         * The loader configuration INI file exists and is opened,
         * map its file contents into memory.
         */
#if 0
        // FIXME: &BootStore->FileSize
        Status = MapFile(BootStore->FileHandle,
                         &BootStore->SectionHandle,
                         &BootStore->ViewBase,
                         (Access & BS_WriteAccess));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to map Loader configuration file '%S' (Status 0x%08lx)\n",
                    NtosBootLoaders[Type].LoaderConfigurationFile, Status);
            NtClose(BootStore->FileHandle);
            RtlFreeHeap(ProcessHeap, 0, BootStore);
            return Status;
        }
#else
        BootStore->SectionHandle = UlongToPtr(1); // Workaround for CloseIniBootLoaderStore
#endif

        /* Open an *existing* INI configuration file */
#if 0
        Status = IniCacheLoadFromMemory(&BootStore->IniCache,
                                        BootStore->ViewBase,
                                        BootStore->FileSize,
                                        FALSE);
#else
        Status = IniCacheLoadByHandle(&BootStore->IniCache, BootStore->FileHandle, FALSE);
#endif
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IniCacheLoadFromMemory() failed (Status 0x%08lx)\n", Status);
#if 0
            /* Finally, unmap and close the file */
            UnMapAndCloseFile(BootStore->FileHandle,
                              BootStore->SectionHandle,
                              BootStore->ViewBase);
#else
            NtClose(BootStore->FileHandle);
#endif
            RtlFreeHeap(ProcessHeap, 0, BootStore);
            return Status;
        }

        if (Type == FreeLdr)
        {
            /*
             * Cache the "FREELOADER" section for our future usage.
             */

            /* Get or create the "FREELOADER" section */
            IniSection = IniAddSection(BootStore->IniCache, L"FREELOADER");
            if (!IniSection)
                DPRINT1("OpenIniBootLoaderStore: Failed to retrieve 'FREELOADER' section!\n");

            BootStore->OptionsIniSection = IniSection;

            /*
             * Cache the "Operating Systems" section for our future usage.
             */

            /* Get or create the "Operating Systems" section */
            IniSection = IniAddSection(BootStore->IniCache, L"Operating Systems");
            if (!IniSection)
                DPRINT1("OpenIniBootLoaderStore: Failed to retrieve 'Operating Systems' section!\n");

            BootStore->OsIniSection = IniSection;

            //
            // TEMPORARY: Migrate the DEPRECATED BootDrive and BootPartition
            // values of BootSector boot entries to the newer BootPath value.
            //
            FreeLdrMigrateBootDrivePart(BootStore);
        }
        else
        if (Type == NtLdr)
        {
            /*
             * Cache the "boot loader" section for our future usage.
             */
            /*
             * HISTORICAL NOTE:
             *
             * While the "operating systems" section acquired its definitive
             * name already when Windows NT was at its very early beta stage
             * (NT 3.1 October 1991 Beta, 10-16-1991), this was not the case
             * for its general settings section "boot loader".
             *
             * The following section names were successively introduced:
             *
             * - In NT 3.1 October 1991 Beta, 10-16-1991, using OS Loader V1.5,
             *   the section was named "multiboot".
             *
             * - In the next public beta version NT 3.10.340 Beta, 10-12-1992,
             *   using OS Loader V2.10, a new name was introduced: "flexboot".
             *   This is around this time that the NT OS Loader was also
             *   introduced as the "Windows NT FlexBoot" loader, as shown by
             *   the Windows NT FAQs that circulated around this time:
             *   http://cd.textfiles.com/cica9308/CIS_LIBS/WINNT/1/NTFAQ.TXT
             *   http://cd.textfiles.com/cica/cica9308/UNZIPPED/NT/NTFAQ/FTP/NEWS/NTFAQ1.TXT
             *   I can only hypothesize that the "FlexBoot" name was chosen
             *   as a marketing coup, possibly to emphasise its "flexibility"
             *   as a simple multiboot-aware boot manager.
             *
             * - A bit later, with NT 3.10.404 Beta, 3-7-1993, using an updated
             *   version of OS Loader V2.10, the final section name "boot loader"
             *   was introduced, and was kept since then.
             *
             * Due to the necessity to be able to boot and / or upgrade any
             * Windows NT version at any time, including its NT Loader and the
             * associated boot.ini file, all versions of NTLDR and the NT installer
             * understand and parse these three section names, the default one
             * being "boot loader", and if not present, they successively fall
             * back to "flexboot" and then to "multiboot".
             */

            /* Get the "boot loader" section */
            IniSection = IniGetSection(BootStore->IniCache, L"boot loader");
            if (!IniSection)
            {
                /* Fall back to "flexboot" */
                IniSection = IniGetSection(BootStore->IniCache, L"flexboot");
                if (!IniSection)
                {
                    /* Fall back to "multiboot" */
                    IniSection = IniGetSection(BootStore->IniCache, L"multiboot");
                }
            }
#if 0
            if (!IniSection)
            {
                /* It does not exist yet, so create it */
                IniSection = IniAddSection(BootStore->IniCache, L"boot loader");
            }
#endif
            if (!IniSection)
                DPRINT1("OpenIniBootLoaderStore: Failed to retrieve 'boot loader' section!\n");

            BootStore->OptionsIniSection = IniSection;

            /*
             * Cache the "Operating Systems" section for our future usage.
             */

            /* Get or create the "Operating Systems" section */
            IniSection = IniAddSection(BootStore->IniCache, L"operating systems");
            if (!IniSection)
                DPRINT1("OpenIniBootLoaderStore: Failed to retrieve 'operating systems' section!\n");

            BootStore->OsIniSection = IniSection;
        }
    }

    *Handle = BootStore;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Selectively changes the attributes of a file.
 *
 * @param[in]   FileHandle
 * Handle to an opened file for which to change its attributes.
 *
 * @param[in]   MaskAttributes
 * A mask specifying which attributes to change; any other attributes
 * will be maintained as they are. If this parameter is zero, all of
 * the attributes in *Attributes will be changed.
 *
 * @param[in,out]   Attributes
 * In input, specifies the new attributes to set. Attributes that
 * are not set, but are specified in MaskAttributes, are removed.
 * In output, receives the original attributes of the file.
 *
 * @return
 * STATUS_SUCCESS if the attributes were successfully changed,
 * or a failure code if an error happened.
 **/
static NTSTATUS
ProtectFile(
    _In_ HANDLE FileHandle,
    _In_ ULONG MaskAttributes,
    _Inout_ PULONG Attributes)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileInfo;
    ULONG OldAttributes;

    /* Retrieve the original file attributes */
    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &FileInfo,
                                    sizeof(FileInfo),
                                    FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationFile() failed (Status 0x%08lx)\n", Status);
        return Status;
    }
    OldAttributes = FileInfo.FileAttributes;

    /* Modify the attributes and return the old ones */
    if (MaskAttributes)
        FileInfo.FileAttributes = (OldAttributes & ~MaskAttributes) | (*Attributes & MaskAttributes);
    else
        FileInfo.FileAttributes = *Attributes;

    *Attributes = OldAttributes;

    /* Set the new file attributes */
    Status = NtSetInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileInfo,
                                  sizeof(FileInfo),
                                  FileBasicInformation);
    if (!NT_SUCCESS(Status))
        DPRINT1("NtSetInformationFile() failed (Status 0x%08lx)\n", Status);

    return Status;
}

static NTSTATUS
CloseIniBootLoaderStore(
    _In_ PVOID Handle)
{
    /* Set or remove SYSTEM, HIDDEN and READONLY attributes */
    static const ULONG ProtectAttribs =
        (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);

    PBOOT_STORE_INI_CONTEXT BootStore = (PBOOT_STORE_INI_CONTEXT)Handle;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG FileAttribs;

    ASSERT(BootStore);

    /* If the INI file was opened in read-only mode, skip saving */
    if (BootStore->Header.ReadOnly)
        goto Quit;

    /* If the INI file was already opened because it already existed, unprotect it */
    if (BootStore->SectionHandle)
    {
        FileAttribs = 0;
        Status = ProtectFile(BootStore->FileHandle, ProtectAttribs, &FileAttribs);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Could not unprotect INI boot store (Status 0x%08lx)\n", Status);
            goto Quit;
        }
    }

    IniCacheSaveByHandle(BootStore->IniCache, BootStore->FileHandle);

    /* Re-protect the INI file */
    FileAttribs = ProtectAttribs;
    if (BootStore->Header.Type == FreeLdr)
    {
        // NOTE: CORE-19575: For the time being, don't add READONLY for ease
        // of testing and modifying files, but it won't always stay this way.
	FileAttribs &= ~FILE_ATTRIBUTE_READONLY;
    }
    /*Status =*/ ProtectFile(BootStore->FileHandle, FileAttribs, &FileAttribs);
    Status = STATUS_SUCCESS; // Ignore the status and just succeed.

Quit:
    IniCacheDestroy(BootStore->IniCache);

#if 0
    if (BootStore->SectionHandle)
    {
        /* Finally, unmap and close the file */
        UnMapAndCloseFile(BootStore->FileHandle,
                          BootStore->SectionHandle,
                          BootStore->ViewBase);
    }
    else // if (BootStore->FileHandle)
#endif
    {
        /* Just close the file we have opened for creation */
        NtClose(BootStore->FileHandle);
    }

    /* Finally, free the boot store structure */
    RtlFreeHeap(ProcessHeap, 0, BootStore);
    return Status;
}


NTSTATUS
OpenBootStoreByHandle(
    _Out_ PVOID* Handle,
    _In_ HANDLE PartitionDirectoryHandle, // _In_opt_
    _In_ BOOT_STORE_TYPE Type,
    _In_ BOOT_STORE_OPENMODE OpenMode,
    _In_ BOOT_STORE_ACCESS Access)
{
    /*
     * NOTE: Currently we open & map the loader configuration file without
     * further tests. It's OK as long as we only deal with FreeLdr's freeldr.ini
     * and NTLDR's boot.ini files. But as soon as we'll implement support for
     * BOOTMGR detection, the "configuration file" will be the BCD registry
     * hive and then, we'll have instead to mount the hive & open it.
     */

    if (Type >= BldrTypeMax || NtosBootLoaders[Type].Type >= BldrTypeMax)
    {
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[Type].Type);
        return STATUS_NOT_SUPPORTED;
    }

    /*
     * Verify the access modes to perform the open actions.
     * The operating system may allow e.g. file creation even with
     * read-only access, but we do not allow this because we want
     * to protect any existing boot store file in case the caller
     * specified such an open mode.
     */
    // if ((OpenMode == BS_CheckExisting) && !(Access & BS_ReadAccess))
    //     return STATUS_ACCESS_DENIED;
    if ((OpenMode == BS_CreateNew || OpenMode == BS_CreateAlways || OpenMode == BS_RecreateExisting) && !(Access & BS_WriteAccess))
        return STATUS_ACCESS_DENIED;
    if ((OpenMode == BS_OpenExisting || OpenMode == BS_OpenAlways) && !(Access & BS_ReadWriteAccess))
        return STATUS_ACCESS_DENIED;

    return NtosBootLoaders[Type].OpenBootStore(Handle,
                                               PartitionDirectoryHandle,
                                               Type,
                                               OpenMode,
                                               Access);
}

NTSTATUS
OpenBootStore_UStr(
    _Out_ PVOID* Handle,
    _In_ PUNICODE_STRING SystemPartitionPath,
    _In_ BOOT_STORE_TYPE Type,
    _In_ BOOT_STORE_OPENMODE OpenMode,
    _In_ BOOT_STORE_ACCESS Access)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PartitionDirectoryHandle;

    /*
     * NOTE: Currently we open & map the loader configuration file without
     * further tests. It's OK as long as we only deal with FreeLdr's freeldr.ini
     * and NTLDR's boot.ini files. But as soon as we'll implement support for
     * BOOTMGR detection, the "configuration file" will be the BCD registry
     * hive and then, we'll have instead to mount the hive & open it.
     */

    if (Type >= BldrTypeMax || NtosBootLoaders[Type].Type >= BldrTypeMax)
    {
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[Type].Type);
        return STATUS_NOT_SUPPORTED;
    }

    /* Open SystemPartition */
    InitializeObjectAttributes(&ObjectAttributes,
                               SystemPartitionPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&PartitionDirectoryHandle,
                        FILE_LIST_DIRECTORY | FILE_ADD_FILE /* | FILE_ADD_SUBDIRECTORY | FILE_TRAVERSE*/ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE /* | FILE_OPEN_FOR_BACKUP_INTENT */);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open SystemPartition '%wZ' (Status 0x%08lx)\n",
                SystemPartitionPath, Status);
        return Status;
    }

    Status = OpenBootStoreByHandle(Handle,
                                   PartitionDirectoryHandle,
                                   Type,
                                   OpenMode,
                                   Access);

    /* Done! */
    NtClose(PartitionDirectoryHandle);
    return Status;
}

NTSTATUS
OpenBootStore(
    _Out_ PVOID* Handle,
    _In_ PCWSTR SystemPartition,
    _In_ BOOT_STORE_TYPE Type,
    _In_ BOOT_STORE_OPENMODE OpenMode,
    _In_ BOOT_STORE_ACCESS Access)
{
    UNICODE_STRING SystemPartitionPath;
    RtlInitUnicodeString(&SystemPartitionPath, SystemPartition);
    return OpenBootStore_UStr(Handle,
                              &SystemPartitionPath,
                              Type,
                              OpenMode,
                              Access);
}

NTSTATUS
CloseBootStore(
    _In_ PVOID Handle)
{
    PBOOT_STORE_CONTEXT BootStore = (PBOOT_STORE_CONTEXT)Handle;

    if (!BootStore)
        return STATUS_INVALID_PARAMETER;

    /*
     * NOTE: Currently we open & map the loader configuration file without
     * further tests. It's OK as long as we only deal with FreeLdr's freeldr.ini
     * and NTLDR's boot.ini files. But as soon as we'll implement support for
     * BOOTMGR detection, the "configuration file" will be the BCD registry
     * hive and then, we'll have instead to mount the hive & open it.
     */

    if (BootStore->Type >= BldrTypeMax || NtosBootLoaders[BootStore->Type].Type >= BldrTypeMax)
    {
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[BootStore->Type].Type);
        return STATUS_NOT_SUPPORTED;
    }

    return NtosBootLoaders[BootStore->Type].CloseBootStore(Handle /* BootStore */);
}


static
NTSTATUS
CreateNTOSEntry(
    IN PBOOT_STORE_INI_CONTEXT BootStore,
    IN ULONG_PTR BootEntryKey,
    IN PBOOT_STORE_ENTRY BootEntry)
{
    PINI_SECTION IniSection;
    PCWSTR Section = (PCWSTR)BootEntryKey;

    /* Insert the entry into the "Operating Systems" section */
    IniAddKey(BootStore->OsIniSection, Section, BootEntry->FriendlyName);

    /* Create a new section */
    IniSection = IniAddSection(BootStore->IniCache, Section);

    if (BootEntry->OsOptionsLength >= sizeof(NTOS_OPTIONS) &&
        RtlCompareMemory(&BootEntry->OsOptions /* Signature */,
                         NTOS_OPTIONS_SIGNATURE,
                         RTL_FIELD_SIZE(NTOS_OPTIONS, Signature)) ==
                         RTL_FIELD_SIZE(NTOS_OPTIONS, Signature))
    {
        PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;

        /* BootType, SystemPath and Options */
        IniAddKey(IniSection, L"BootType", L"Windows2003");
        IniAddKey(IniSection, L"SystemPath", Options->OsLoadPath);
        IniAddKey(IniSection, L"Options", Options->OsLoadOptions);
    }
    else
    if (BootEntry->OsOptionsLength >= sizeof(BOOTSECTOR_OPTIONS) &&
        RtlCompareMemory(&BootEntry->OsOptions /* Signature */,
                         BOOTSECTOR_OPTIONS_SIGNATURE,
                         RTL_FIELD_SIZE(BOOTSECTOR_OPTIONS, Signature)) ==
                         RTL_FIELD_SIZE(BOOTSECTOR_OPTIONS, Signature))
    {
        PBOOTSECTOR_OPTIONS Options = (PBOOTSECTOR_OPTIONS)&BootEntry->OsOptions;

        /* BootType, BootPath and BootSector */
        IniAddKey(IniSection, L"BootType", L"BootSector");
        IniAddKey(IniSection, L"BootPath", Options->BootPath);
        IniAddKey(IniSection, L"BootSectorFile", Options->FileName);
    }
    else
    {
        // DPRINT1("Unsupported BootType %lu/'%*.s'\n",
                // BootEntry->OsOptionsLength, 8, &BootEntry->OsOptions);
        DPRINT1("Unsupported BootType %lu\n", BootEntry->OsOptionsLength);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
AddBootStoreEntry(
    IN PVOID Handle,
    IN PBOOT_STORE_ENTRY BootEntry,
    IN ULONG_PTR BootEntryKey)
{
    PBOOT_STORE_CONTEXT BootStore = (PBOOT_STORE_CONTEXT)Handle;

    if (!BootStore || !BootEntry)
        return STATUS_INVALID_PARAMETER;

    /*
     * NOTE: Currently we open & map the loader configuration file without
     * further tests. It's OK as long as we only deal with FreeLdr's freeldr.ini
     * and NTLDR's boot.ini files. But as soon as we'll implement support for
     * BOOTMGR detection, the "configuration file" will be the BCD registry
     * hive and then, we'll have instead to mount the hive & open it.
     */

    //
    // FIXME!!
    //

    // if (BootStore->Type >= BldrTypeMax || NtosBootLoaders[BootStore->Type].Type >= BldrTypeMax)

    if (BootStore->Type == FreeLdr)
    {
        if (BootEntry->Version != FreeLdr)
            return STATUS_INVALID_PARAMETER;

        return CreateNTOSEntry((PBOOT_STORE_INI_CONTEXT)BootStore,
                               BootEntryKey, BootEntry);
    }
    else
    if (BootStore->Type == NtLdr)
    {
        PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;
        PWCHAR Buffer;
        ULONG BufferLength;
        PCWSTR InstallName, OsOptions;
        // ULONG InstallNameLength, OsOptionsLength;
        BOOLEAN IsNameNotQuoted;

        if (BootEntry->Version != NtLdr)
            return STATUS_INVALID_PARAMETER;

        if (BootEntry->OsOptionsLength < sizeof(NTOS_OPTIONS) ||
            RtlCompareMemory(&BootEntry->OsOptions /* Signature */,
                             NTOS_OPTIONS_SIGNATURE,
                             RTL_FIELD_SIZE(NTOS_OPTIONS, Signature)) !=
                             RTL_FIELD_SIZE(NTOS_OPTIONS, Signature))
        {
            // DPRINT1("Unsupported BootType '%S'\n", BootEntry->Version);
            DPRINT1("Unsupported BootType %lu\n", BootEntry->OsOptionsLength);
            return STATUS_SUCCESS; // STATUS_NOT_SUPPORTED;
        }

        InstallName = BootEntry->FriendlyName;
        OsOptions = Options->OsLoadOptions;

        // if (InstallNameLength == 0) InstallName = NULL;
        // if (OsOptionsLength == 0) OsOptions = NULL;

        IsNameNotQuoted = (InstallName[0] != L'\"' || InstallName[wcslen(InstallName)-1] != L'\"');

        BufferLength = (IsNameNotQuoted ? 2 /* Quotes for FriendlyName*/ : 0) + wcslen(InstallName);
        if (OsOptions)
            BufferLength += 1 /* Space between FriendlyName and options */ + wcslen(OsOptions);
        BufferLength++; /* NULL-termination */

        Buffer = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, BufferLength * sizeof(WCHAR));
        if (!Buffer)
            return STATUS_INSUFFICIENT_RESOURCES;

        *Buffer = UNICODE_NULL;
        if (IsNameNotQuoted) RtlStringCchCatW(Buffer, BufferLength, L"\"");
        RtlStringCchCatW(Buffer, BufferLength, InstallName);
        if (IsNameNotQuoted) RtlStringCchCatW(Buffer, BufferLength, L"\"");
        if (OsOptions)
        {
            RtlStringCchCatW(Buffer, BufferLength, L" ");
            RtlStringCchCatW(Buffer, BufferLength, OsOptions);
        }

        /* Insert the entry into the "Operating Systems" section */
        IniAddKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OsIniSection,
                  Options->OsLoadPath, Buffer);

        RtlFreeHeap(ProcessHeap, 0, Buffer);
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT1("Loader type %d is currently unsupported!\n", BootStore->Type);
        return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS
DeleteBootStoreEntry(
    IN PVOID Handle,
    IN ULONG_PTR BootEntryKey)
{
    PBOOT_STORE_CONTEXT BootStore = (PBOOT_STORE_CONTEXT)Handle;

    if (!BootStore)
        return STATUS_INVALID_PARAMETER;

    /*
     * NOTE: Currently we open & map the loader configuration file without
     * further tests. It's OK as long as we only deal with FreeLdr's freeldr.ini
     * and NTLDR's boot.ini files. But as soon as we'll implement support for
     * BOOTMGR detection, the "configuration file" will be the BCD registry
     * hive and then, we'll have instead to mount the hive & open it.
     */

    //
    // FIXME!!
    //

    // if (BootStore->Type >= BldrTypeMax || NtosBootLoaders[BootStore->Type].Type >= BldrTypeMax)
    if (BootStore->Type != FreeLdr)
    {
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[BootStore->Type].Type);
        return STATUS_NOT_SUPPORTED;
    }

    // FIXME! This function needs my INI library rewrite to be implemented!!
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ModifyBootStoreEntry(
    IN PVOID Handle,
    IN PBOOT_STORE_ENTRY BootEntry)
{
    PBOOT_STORE_CONTEXT BootStore = (PBOOT_STORE_CONTEXT)Handle;

    if (!BootStore || !BootEntry)
        return STATUS_INVALID_PARAMETER;

    /*
     * NOTE: Currently we open & map the loader configuration file without
     * further tests. It's OK as long as we only deal with FreeLdr's freeldr.ini
     * and NTLDR's boot.ini files. But as soon as we'll implement support for
     * BOOTMGR detection, the "configuration file" will be the BCD registry
     * hive and then, we'll have instead to mount the hive & open it.
     */

    //
    // FIXME!!
    //

    // if (BootStore->Type >= BldrTypeMax || NtosBootLoaders[BootStore->Type].Type >= BldrTypeMax)
    if (BootStore->Type != FreeLdr)
    {
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[BootStore->Type].Type);
        return STATUS_NOT_SUPPORTED;
    }

    // FIXME! This function needs my INI library rewrite to operate properly!!
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
QueryBootStoreEntry(
    IN PVOID Handle,
    IN ULONG_PTR BootEntryKey,
    OUT PBOOT_STORE_ENTRY BootEntry) // Technically this should be PBOOT_STORE_ENTRY*
{
    PBOOT_STORE_CONTEXT BootStore = (PBOOT_STORE_CONTEXT)Handle;

    if (!BootStore)
        return STATUS_INVALID_PARAMETER;

    /*
     * NOTE: Currently we open & map the loader configuration file without
     * further tests. It's OK as long as we only deal with FreeLdr's freeldr.ini
     * and NTLDR's boot.ini files. But as soon as we'll implement support for
     * BOOTMGR detection, the "configuration file" will be the BCD registry
     * hive and then, we'll have instead to mount the hive & open it.
     */

    //
    // FIXME!!
    //

    // if (BootStore->Type >= BldrTypeMax || NtosBootLoaders[BootStore->Type].Type >= BldrTypeMax)
    if (BootStore->Type != FreeLdr)
    {
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[BootStore->Type].Type);
        return STATUS_NOT_SUPPORTED;
    }

    // FIXME! This function needs my INI library rewrite to be implemented!!
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
QueryBootStoreOptions(
    IN PVOID Handle,
    IN OUT PBOOT_STORE_OPTIONS BootOptions
/* , IN PULONG BootOptionsLength */ )
{
    PBOOT_STORE_CONTEXT BootStore = (PBOOT_STORE_CONTEXT)Handle;
    PCWSTR TimeoutStr;

    if (!BootStore || !BootOptions)
        return STATUS_INVALID_PARAMETER;

    /*
     * NOTE: Currently we open & map the loader configuration file without
     * further tests. It's OK as long as we only deal with FreeLdr's freeldr.ini
     * and NTLDR's boot.ini files. But as soon as we'll implement support for
     * BOOTMGR detection, the "configuration file" will be the BCD registry
     * hive and then, we'll have instead to mount the hive & open it.
     */

    //
    // FIXME!!
    //

    // if (BootStore->Type >= BldrTypeMax || NtosBootLoaders[BootStore->Type].Type >= BldrTypeMax)
    if (BootStore->Type != FreeLdr && BootStore->Type != NtLdr)
    {
        DPRINT1("Loader type %d is currently unsupported!\n", BootStore->Type);
        return STATUS_NOT_SUPPORTED;
    }

    BootOptions->Timeout = 0;
    BootOptions->CurrentBootEntryKey = 0;
    BootOptions->NextBootEntryKey = 0;

    if (IniGetKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OptionsIniSection,
                  BootOptionNames[BootStore->Type][BO_TimeOut],
                  &TimeoutStr) && TimeoutStr)
    {
        BootOptions->Timeout = _wtoi(TimeoutStr);
    }

    IniGetKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OptionsIniSection,
              BootOptionNames[BootStore->Type][BO_DefaultOS],
              (PCWSTR*)&BootOptions->NextBootEntryKey);

    /*
     * NOTE: BootOptions->CurrentBootEntryKey is an informative field only.
     * It indicates which boot entry has been selected for starting the
     * current OS instance. Such information is NOT stored in the INI file,
     * but has to be determined via other means. On UEFI the 'BootCurrent'
     * environment variable does that. Otherwise, one could heuristically
     * determine it by comparing the boot path and options of each entry
     * with those used by the current OS instance.
     * Since we currently do not need this information (and it can be costly
     * to determine), BootOptions->CurrentBootEntryKey is not evaluated.
     */

    return STATUS_SUCCESS;
}

NTSTATUS
SetBootStoreOptions(
    IN PVOID Handle,
    IN PBOOT_STORE_OPTIONS BootOptions,
    IN ULONG FieldsToChange)
{
    PBOOT_STORE_CONTEXT BootStore = (PBOOT_STORE_CONTEXT)Handle;

    if (!BootStore || !BootOptions)
        return STATUS_INVALID_PARAMETER;

    /*
     * NOTE: Currently we open & map the loader configuration file without
     * further tests. It's OK as long as we only deal with FreeLdr's freeldr.ini
     * and NTLDR's boot.ini files. But as soon as we'll implement support for
     * BOOTMGR detection, the "configuration file" will be the BCD registry
     * hive and then, we'll have instead to mount the hive & open it.
     */

    //
    // FIXME!!
    //

    // if (BootStore->Type >= BldrTypeMax || NtosBootLoaders[BootStore->Type].Type >= BldrTypeMax)
    if (BootStore->Type != FreeLdr && BootStore->Type != NtLdr)
    {
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[BootStore->Type].Type);
        return STATUS_NOT_SUPPORTED;
    }

    // if (BootOptions->Length < sizeof(*BootOptions))
    //     return STATUS_INVALID_PARAMETER;

    if (FieldsToChange & BOOT_OPTIONS_TIMEOUT)
    {
        WCHAR TimeoutStr[15];
        RtlStringCchPrintfW(TimeoutStr, ARRAYSIZE(TimeoutStr), L"%d", BootOptions->Timeout);
        IniAddKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OptionsIniSection,
                  BootOptionNames[BootStore->Type][BO_TimeOut],
                  TimeoutStr);
    }
    if (FieldsToChange & BOOT_OPTIONS_NEXT_BOOTENTRY_KEY)
    {
        IniAddKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OptionsIniSection,
                  BootOptionNames[BootStore->Type][BO_DefaultOS],
                  (PCWSTR)BootOptions->NextBootEntryKey);
    }

    return STATUS_SUCCESS;
}


static NTSTATUS
FreeLdrEnumerateBootEntries(
    IN PBOOT_STORE_INI_CONTEXT BootStore,
//  IN ULONG Flags, // Determine which data to retrieve
    IN PENUM_BOOT_ENTRIES_ROUTINE EnumBootEntriesRoutine,
    IN PVOID Parameter OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PINICACHEITERATOR Iterator;
    PINI_SECTION OsIniSection;
    PCWSTR SectionName, KeyData;
    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) +
                      max(sizeof(NTOS_OPTIONS), sizeof(BOOTSECTOR_OPTIONS))];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
    PWCHAR Buffer;

    /* Enumerate all the valid installations listed in the "Operating Systems" section */
    Iterator = IniFindFirstValue(BootStore->OsIniSection, &SectionName, &KeyData);
    if (!Iterator) return STATUS_SUCCESS;
    do
    {
        PCWSTR InstallName;
        ULONG InstallNameLength;

        /* Poor-man quotes removal (improvement over bootsup.c:UpdateFreeLoaderIni) */
        if (*KeyData == L'"')
        {
            /* Quoted name, copy up to the closing quote */
            PWCHAR End = wcschr(KeyData + 1, L'"');

            if (End)
            {
                /* Skip the first quote */
                InstallName = KeyData + 1;
                InstallNameLength = End - InstallName;
            }
            else // if (!End)
            {
                /* No corresponding closing quote, so we include the first one in the InstallName */
                InstallName = KeyData;
                InstallNameLength = wcslen(InstallName);
            }
            if (InstallNameLength == 0) InstallName = NULL;
        }
        else
        {
            /* Non-quoted name, copy everything */
            InstallName = KeyData;
            InstallNameLength = wcslen(InstallName);
            if (InstallNameLength == 0) InstallName = NULL;
        }

        /* Allocate the temporary buffer */
        Buffer = NULL;
        if (InstallNameLength)
            Buffer = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, (InstallNameLength + 1) * sizeof(WCHAR));
        if (Buffer)
        {
            RtlCopyMemory(Buffer, InstallName, InstallNameLength * sizeof(WCHAR));
            Buffer[InstallNameLength] = UNICODE_NULL;
            InstallName = Buffer;
        }

        DPRINT("Boot entry '%S' in OS section '%S'\n", InstallName, SectionName);

        BootEntry->Version = FreeLdr;
        BootEntry->BootEntryKey = MAKESTRKEY(SectionName);
        BootEntry->FriendlyName = InstallName;
        BootEntry->BootFilePath = NULL;
        BootEntry->OsOptionsLength = 0;

        /* Search for an existing boot entry section */
        OsIniSection = IniGetSection(BootStore->IniCache, SectionName);
        if (!OsIniSection)
            goto DoEnum;

        /* Check for supported boot type */
        if (!IniGetKey(OsIniSection, L"BootType", &KeyData) || !KeyData)
        {
            /* Certainly not a ReactOS installation */
            DPRINT1("No BootType value present\n");
            goto DoEnum;
        }

        // TODO: What to do with "Windows" ; "WindowsNT40" ; "ReactOSSetup" ?
        if ((_wcsicmp(KeyData, L"Windows2003")     == 0) ||
            (_wcsicmp(KeyData, L"\"Windows2003\"") == 0))
        {
            /* BootType is Windows2003 */
            PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;

            DPRINT("This is a '%S' boot entry\n", KeyData);

            BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
            RtlCopyMemory(Options->Signature,
                          NTOS_OPTIONS_SIGNATURE,
                          RTL_FIELD_SIZE(NTOS_OPTIONS, Signature));

            // BootEntry->BootFilePath = NULL;

            /* Check its SystemPath */
            Options->OsLoadPath = NULL;
            if (IniGetKey(OsIniSection, L"SystemPath", &KeyData))
                Options->OsLoadPath = KeyData;
            // KeyData == SystemRoot;

            /* Check the optional Options */
            Options->OsLoadOptions = NULL;
            if (IniGetKey(OsIniSection, L"Options", &KeyData))
                Options->OsLoadOptions = KeyData;
        }
        else
        if ((_wcsicmp(KeyData, L"BootSector")     == 0) ||
            (_wcsicmp(KeyData, L"\"BootSector\"") == 0))
        {
            /* BootType is BootSector */
            PBOOTSECTOR_OPTIONS Options = (PBOOTSECTOR_OPTIONS)&BootEntry->OsOptions;

            DPRINT("This is a '%S' boot entry\n", KeyData);

            BootEntry->OsOptionsLength = sizeof(BOOTSECTOR_OPTIONS);
            RtlCopyMemory(Options->Signature,
                          BOOTSECTOR_OPTIONS_SIGNATURE,
                          RTL_FIELD_SIZE(BOOTSECTOR_OPTIONS, Signature));

            // BootEntry->BootFilePath = NULL;

            /* Check its BootPath */
            Options->BootPath = NULL;
            if (IniGetKey(OsIniSection, L"BootPath", &KeyData))
                Options->BootPath = KeyData;

            /* Check its BootSector */
            Options->FileName = NULL;
            if (IniGetKey(OsIniSection, L"BootSectorFile", &KeyData))
                Options->FileName = KeyData;
        }
        else
        {
            DPRINT1("Unrecognized BootType value '%S'\n", KeyData);
            // goto DoEnum;
        }

DoEnum:
        /* Call the user enumeration routine callback */
        Status = EnumBootEntriesRoutine(FreeLdr, BootEntry, Parameter);

        /* Free temporary buffers */
        if (Buffer)
            RtlFreeHeap(ProcessHeap, 0, Buffer);

        /* Stop the enumeration if needed */
        if (!NT_SUCCESS(Status))
            break;
    }
    while (IniFindNextValue(Iterator, &SectionName, &KeyData));

    IniFindClose(Iterator);
    return Status;
}

static NTSTATUS
NtLdrEnumerateBootEntries(
    IN PBOOT_STORE_INI_CONTEXT BootStore,
//  IN ULONG Flags, // Determine which data to retrieve
    IN PENUM_BOOT_ENTRIES_ROUTINE EnumBootEntriesRoutine,
    IN PVOID Parameter OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PINICACHEITERATOR Iterator;
    PCWSTR SectionName, KeyData;
    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(NTOS_OPTIONS)];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
    PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;
    PWCHAR Buffer;
    ULONG BufferLength;

    /* Enumerate all the valid installations */
    Iterator = IniFindFirstValue(BootStore->OsIniSection, &SectionName, &KeyData);
    if (!Iterator) return STATUS_SUCCESS;
    do
    {
        PCWSTR InstallName, OsOptions;
        ULONG InstallNameLength, OsOptionsLength;

        /* Poor-man quotes removal (improvement over bootsup.c:UpdateFreeLoaderIni) */
        if (*KeyData == L'"')
        {
            /* Quoted name, copy up to the closing quote */
            OsOptions = wcschr(KeyData + 1, L'"');

            /* Retrieve the starting point of the installation name and the OS options */
            if (OsOptions)
            {
                /* Skip the first quote */
                InstallName = KeyData + 1;
                InstallNameLength = OsOptions - InstallName;
                if (InstallNameLength == 0) InstallName = NULL;

                /* Skip the ending quote (if found) */
                ++OsOptions;

                /* Skip any whitespace */
                while (iswspace(*OsOptions)) ++OsOptions;
                /* Get its final length */
                OsOptionsLength = wcslen(OsOptions);
                if (OsOptionsLength == 0) OsOptions = NULL;
            }
            else
            {
                /* No corresponding closing quote, so we include the first one in the InstallName */
                InstallName = KeyData;
                InstallNameLength = wcslen(InstallName);
                if (InstallNameLength == 0) InstallName = NULL;

                /* There are no OS options */
                // OsOptions = NULL;
                OsOptionsLength = 0;
            }
        }
        else
        {
            /* Non-quoted name, copy everything */

            /* Retrieve the starting point of the installation name */
            InstallName = KeyData;
            InstallNameLength = wcslen(InstallName);
            if (InstallNameLength == 0) InstallName = NULL;

            /* There are no OS options */
            OsOptions = NULL;
            OsOptionsLength = 0;
        }

        /* Allocate the temporary buffer */
        Buffer = NULL;
        BufferLength = (InstallNameLength + OsOptionsLength) * sizeof(WCHAR);
        if (BufferLength)
            Buffer = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, BufferLength + 2*sizeof(UNICODE_NULL));
        if (Buffer)
        {
            PWCHAR ptr;

            /* Copy the installation name, and make InstallName point into the buffer */
            if (InstallName && InstallNameLength)
            {
                ptr = Buffer;
                RtlCopyMemory(ptr, InstallName, InstallNameLength * sizeof(WCHAR));
                ptr[InstallNameLength] = UNICODE_NULL;
                InstallName = ptr;
            }

            /* Copy the OS options, and make OsOptions point into the buffer */
            if (OsOptions && OsOptionsLength)
            {
                ptr = Buffer + InstallNameLength + 1;
                RtlCopyMemory(ptr, OsOptions, OsOptionsLength * sizeof(WCHAR));
                ptr[OsOptionsLength] = UNICODE_NULL;
                OsOptions = ptr;
            }
        }

        DPRINT1("Boot entry '%S' in OS section (path) '%S'\n", InstallName, SectionName);
        // SectionName == SystemRoot;

        BootEntry->Version = NtLdr;
        BootEntry->BootEntryKey = 0; // FIXME??
        BootEntry->FriendlyName = InstallName;
        BootEntry->BootFilePath = NULL;

        BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
        RtlCopyMemory(Options->Signature,
                      NTOS_OPTIONS_SIGNATURE,
                      RTL_FIELD_SIZE(NTOS_OPTIONS, Signature));

        Options->OsLoadPath    = SectionName;
        Options->OsLoadOptions = OsOptions;

        /* Call the user enumeration routine callback */
        Status = EnumBootEntriesRoutine(NtLdr, BootEntry, Parameter);

        /* Free temporary buffers */
        if (Buffer)
            RtlFreeHeap(ProcessHeap, 0, Buffer);

        /* Stop the enumeration if needed */
        if (!NT_SUCCESS(Status))
            break;
    }
    while (IniFindNextValue(Iterator, &SectionName, &KeyData));

    IniFindClose(Iterator);
    return Status;
}

NTSTATUS
EnumerateBootStoreEntries(
    IN PVOID Handle,
//  IN ULONG Flags, // Determine which data to retrieve
    IN PENUM_BOOT_ENTRIES_ROUTINE EnumBootEntriesRoutine,
    IN PVOID Parameter OPTIONAL)
{
    PBOOT_STORE_CONTEXT BootStore = (PBOOT_STORE_CONTEXT)Handle;

    if (!BootStore)
        return STATUS_INVALID_PARAMETER;

    if (BootStore->Type >= BldrTypeMax || NtosBootLoaders[BootStore->Type].Type >= BldrTypeMax)
    {
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[BootStore->Type].Type);
        /**/return STATUS_SUCCESS;/**/
        // return STATUS_INVALID_PARAMETER;
    }

    return NtosBootLoaders[BootStore->Type].EnumBootStoreEntries(
                (PBOOT_STORE_INI_CONTEXT)BootStore, // Flags,
                EnumBootEntriesRoutine, Parameter);
}

/* EOF */
