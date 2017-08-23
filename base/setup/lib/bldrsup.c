/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/bldrsup.c
 * PURPOSE:         Boot Stores Management functionality, with support for
 *                  NT 5.x family (MS Windows <= 2003, and ReactOS) bootloaders.
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
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
    OUT PVOID* Handle,
    IN HANDLE PartitionDirectoryHandle, // OPTIONAL
    IN BOOT_STORE_TYPE Type,
    IN BOOLEAN CreateNew);

typedef NTSTATUS
(*PCLOSE_BOOT_STORE)(
    IN PVOID Handle);

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
    PINICACHESECTION OptionsIniSection;
    PINICACHESECTION OsIniSection;
} BOOT_STORE_INI_CONTEXT, *PBOOT_STORE_INI_CONTEXT;

// TODO!
typedef struct _BOOT_STORE_BCDREG_CONTEXT
{
    BOOT_STORE_CONTEXT Header;
    ULONG PlaceHolder;
} BOOT_STORE_BCDREG_CONTEXT, *PBOOT_STORE_BCDREG_CONTEXT;


static NTSTATUS
OpenIniBootLoaderStore(
    OUT PVOID* Handle,
    IN HANDLE PartitionDirectoryHandle, // OPTIONAL
    IN BOOT_STORE_TYPE Type,
    IN BOOLEAN CreateNew);

static NTSTATUS
CloseIniBootLoaderStore(
    IN PVOID Handle);

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
    {FreeLdr, L"freeldr.sys\0", L"freeldr.ini",
        OpenIniBootLoaderStore, CloseIniBootLoaderStore, (PENUM_BOOT_STORE_ENTRIES)FreeLdrEnumerateBootEntries},
    {NtLdr  , L"ntldr\0" L"osloader.exe\0", L"boot.ini",
        OpenIniBootLoaderStore, CloseIniBootLoaderStore, (PENUM_BOOT_STORE_ENTRIES)NtLdrEnumerateBootEntries  },
//  {SetupLdr, L"setupldr\0" L"setupldr.bin\0" L"setupldr.exe\0", L"txtsetup.sif", UNIMPLEMENTED, UNIMPLEMENTED, UNIMPLEMENTED}
//  {BootMgr , L"bootmgr", L"BCD", UNIMPLEMENTED, UNIMPLEMENTED, UNIMPLEMENTED}
};
C_ASSERT(_countof(NtosBootLoaders) == BldrTypeMax);


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
            DPRINT1("Found loader executable '%S'\n", LoaderExecutable);
            break;
        }

        /* The loader does not exist, continue with another one */
        DPRINT1("Loader executable '%S' does not exist, continue with another one...\n", LoaderExecutable);
        LoaderExecutable += wcslen(LoaderExecutable) + 1;
    }
    if (!*LoaderExecutable)
    {
        /* No loader was found */
        DPRINT1("No loader executable was found\n");
        return STATUS_NOT_FOUND;
    }

    /* Check for loader version if needed */
    if (VersionNumber)
    {
        *VersionNumber = 0;
        // TODO: Check for BLDR version!
    }

    /* Check whether the loader configuration file exists */
    if (!DoesFileExist(PartitionDirectoryHandle, NtosBootLoaders[Type].LoaderConfigurationFile))
    {
        /* The loader does not exist, continue with another one */
        // FIXME: Consider it might be optional??
        DPRINT1("Loader configuration file '%S' does not exist\n", NtosBootLoaders[Type].LoaderConfigurationFile);
        return STATUS_NOT_FOUND;
    }

#if 0
    /* Check whether the loader configuration file exists */
    Status = OpenAndMapFile(PartitionDirectoryHandle, NtosBootLoaders[Type].LoaderConfigurationFile,
                            &FileHandle, &SectionHandle, &ViewBase, &FileSize, FALSE);
    if (!NT_SUCCESS(Status))
    {
        /* The loader does not exist, continue with another one */
        // FIXME: Consider it might be optional??
        DPRINT1("Loader configuration file '%S' does not exist\n", NtosBootLoaders[Type].LoaderConfigurationFile);
        return STATUS_NOT_FOUND;
    }
#endif

    return STATUS_SUCCESS;
}


static VOID
CreateCommonFreeLdrSections(
    IN OUT PBOOT_STORE_INI_CONTEXT BootStore)
{
    PINICACHESECTION IniSection;

    /*
     * Cache the "FREELOADER" section for our future usage.
     */

    /* Get the "FREELOADER" section */
    IniSection = IniCacheGetSection(BootStore->IniCache, L"FREELOADER");
    if (!IniSection)
    {
        /* It does not exist yet, so create it */
        IniSection = IniCacheAppendSection(BootStore->IniCache, L"FREELOADER");
        if (!IniSection)
        {
            DPRINT1("CreateCommonFreeLdrSections: Failed to create 'FREELOADER' section!\n");
        }
    }

    BootStore->OptionsIniSection = IniSection;

    /* Timeout=0 */
    IniCacheInsertKey(BootStore->OptionsIniSection, NULL, INSERT_LAST,
                      L"TimeOut", L"0");

    /* Create "Display" section */
    IniSection = IniCacheAppendSection(BootStore->IniCache, L"Display");

    /* TitleText=ReactOS Boot Manager */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"TitleText", L"ReactOS Boot Manager");

    /* StatusBarColor=Cyan */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"StatusBarColor", L"Cyan");

    /* StatusBarTextColor=Black */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"StatusBarTextColor", L"Black");

    /* BackdropTextColor=White */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"BackdropTextColor", L"White");

    /* BackdropColor=Blue */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"BackdropColor", L"Blue");

    /* BackdropFillStyle=Medium */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"BackdropFillStyle", L"Medium");

    /* TitleBoxTextColor=White */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"TitleBoxTextColor", L"White");

    /* TitleBoxColor=Red */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"TitleBoxColor", L"Red");

    /* MessageBoxTextColor=White */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"MessageBoxTextColor", L"White");

    /* MessageBoxColor=Blue */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"MessageBoxColor", L"Blue");

    /* MenuTextColor=White */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"MenuTextColor", L"Gray");

    /* MenuColor=Blue */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"MenuColor", L"Black");

    /* TextColor=Yellow */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"TextColor", L"Gray");

    /* SelectedTextColor=Black */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"SelectedTextColor", L"Black");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"SelectedColor", L"Gray");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"ShowTime", L"No");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"MenuBox", L"No");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"CenterMenu", L"No");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"MinimalUI", L"Yes");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                      L"TimeText",
                      L"Seconds until highlighted choice will be started automatically:   ");

    /*
     * Cache the "Operating Systems" section for our future usage.
     */

    /* Get the "Operating Systems" section */
    IniSection = IniCacheGetSection(BootStore->IniCache, L"Operating Systems");
    if (!IniSection)
    {
        /* It does not exist yet, so create it */
        IniSection = IniCacheAppendSection(BootStore->IniCache, L"Operating Systems");
        if (!IniSection)
        {
            DPRINT1("CreateCommonFreeLdrSections: Failed to create 'Operating Systems' section!\n");
        }
    }

    BootStore->OsIniSection = IniSection;
}

static NTSTATUS
OpenIniBootLoaderStore(
    OUT PVOID* Handle,
    IN HANDLE PartitionDirectoryHandle, // OPTIONAL
    IN BOOT_STORE_TYPE Type,
    IN BOOLEAN CreateNew)
{
    NTSTATUS Status;
    PBOOT_STORE_INI_CONTEXT BootStore;

    /* Create a boot store structure */
    BootStore = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, sizeof(*BootStore));
    if (!BootStore)
        return STATUS_INSUFFICIENT_RESOURCES;

    BootStore->Header.Type = Type;

    if (CreateNew)
    {
        UNICODE_STRING Name;
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatusBlock;

        //
        // WARNING! We "support" the INI creation *ONLY* for FreeLdr, and not for NTLDR!!
        //
        if (Type == NtLdr)
        {
            DPRINT1("OpenIniBootLoaderStore() unsupported for NTLDR!\n");
            RtlFreeHeap(ProcessHeap, 0, BootStore);
            return STATUS_NOT_SUPPORTED;
        }

        /* Initialize the INI file */
        BootStore->IniCache = IniCacheCreate();
        if (!BootStore->IniCache)
        {
            DPRINT1("IniCacheCreate() failed.\n");
            RtlFreeHeap(ProcessHeap, 0, BootStore);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /*
         * So far, we only use the INI cache. The file itself is not created
         * yet, therefore FileHandle, SectionHandle, ViewBase and FileSize
         * are all NULL. We will use this fact to know that the INI file was
         * indeed created, and not just opened as an existing file.
         */
        // BootStore->FileHandle = NULL;
        BootStore->SectionHandle = NULL;
        BootStore->ViewBase = NULL;
        BootStore->FileSize = 0;

        /*
         * The INI file is fresh new, we need to create it now.
         */

        RtlInitUnicodeString(&Name, NtosBootLoaders[Type].LoaderConfigurationFile);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &Name,
                                   OBJ_CASE_INSENSITIVE,
                                   PartitionDirectoryHandle,
                                   NULL);

        Status = NtCreateFile(&BootStore->FileHandle,
                              FILE_GENERIC_READ | FILE_GENERIC_WRITE, // Contains SYNCHRONIZE
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              0,
                              FILE_SUPERSEDE,
                              FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY | FILE_NON_DIRECTORY_FILE,
                              NULL,
                              0);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtCreateFile() failed (Status 0x%08lx)\n", Status);
            IniCacheDestroy(BootStore->IniCache);
            RtlFreeHeap(ProcessHeap, 0, BootStore);
            return Status;
        }

        /* Initialize the INI file contents */
        if (Type == FreeLdr)
            CreateCommonFreeLdrSections(BootStore);
    }
    else
    {
        PINICACHESECTION IniSection;

        /*
         * Check whether the loader configuration INI file exists,
         * and open it if so.
         * TODO: FIXME: What if it doesn't exist yet???
         */
        Status = OpenAndMapFile(PartitionDirectoryHandle,
                                NtosBootLoaders[Type].LoaderConfigurationFile,
                                &BootStore->FileHandle,
                                &BootStore->SectionHandle,
                                &BootStore->ViewBase,
                                &BootStore->FileSize,
                                TRUE);
        if (!NT_SUCCESS(Status))
        {
            /* The loader configuration file does not exist */
            // FIXME: Consider it might be optional??
            DPRINT1("Loader configuration file '%S' does not exist (Status 0x%08lx)\n",
                    NtosBootLoaders[Type].LoaderConfigurationFile, Status);
            RtlFreeHeap(ProcessHeap, 0, BootStore);
            return Status;
        }

        /* Open an *existing* INI configuration file */
        // Status = IniCacheLoad(&BootStore->IniCache, NtosBootLoaders[Type].LoaderConfigurationFile, FALSE);
        Status = IniCacheLoadFromMemory(&BootStore->IniCache,
                                        BootStore->ViewBase,
                                        BootStore->FileSize,
                                        FALSE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IniCacheLoadFromMemory() failed (Status 0x%08lx)\n", Status);

            /* Finally, unmap and close the file */
            UnMapAndCloseFile(BootStore->FileHandle,
                              BootStore->SectionHandle,
                              BootStore->ViewBase);

            RtlFreeHeap(ProcessHeap, 0, BootStore);
            return Status;
        }

        if (Type == FreeLdr)
        {
            /*
             * Cache the "FREELOADER" section for our future usage.
             */

            /* Get the "FREELOADER" section */
            IniSection = IniCacheGetSection(BootStore->IniCache, L"FREELOADER");
            if (!IniSection)
            {
                /* It does not exist yet, so create it */
                IniSection = IniCacheAppendSection(BootStore->IniCache, L"FREELOADER");
                if (!IniSection)
                {
                    DPRINT1("OpenIniBootLoaderStore: Failed to retrieve 'FREELOADER' section!\n");
                }
            }

            BootStore->OptionsIniSection = IniSection;

            /*
             * Cache the "Operating Systems" section for our future usage.
             */

            /* Get the "Operating Systems" section */
            IniSection = IniCacheGetSection(BootStore->IniCache, L"Operating Systems");
            if (!IniSection)
            {
                /* It does not exist yet, so create it */
                IniSection = IniCacheAppendSection(BootStore->IniCache, L"Operating Systems");
                if (!IniSection)
                {
                    DPRINT1("OpenIniBootLoaderStore: Failed to retrieve 'Operating Systems' section!\n");
                }
            }

            BootStore->OsIniSection = IniSection;
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
            IniSection = IniCacheGetSection(BootStore->IniCache, L"boot loader");
            if (!IniSection)
            {
                /* Fall back to "flexboot" */
                IniSection = IniCacheGetSection(BootStore->IniCache, L"flexboot");
                if (!IniSection)
                {
                    /* Fall back to "multiboot" */
                    IniSection = IniCacheGetSection(BootStore->IniCache, L"multiboot");
                }
            }
#if 0
            if (!IniSection)
            {
                /* It does not exist yet, so create it */
                IniSection = IniCacheAppendSection(BootStore->IniCache, L"boot loader");
                if (!IniSection)
                {
                    DPRINT1("OpenIniBootLoaderStore: Failed to retrieve 'boot loader' section!\n");
                }
            }
#endif

            BootStore->OptionsIniSection = IniSection;

            /*
             * Cache the "Operating Systems" section for our future usage.
             */

            /* Get the "Operating Systems" section */
            IniSection = IniCacheGetSection(BootStore->IniCache, L"operating systems");
            if (!IniSection)
            {
#if 0
                /* It does not exist yet, so create it */
                IniSection = IniCacheAppendSection(BootStore->IniCache, L"operating systems");
                if (!IniSection)
                {
                    DPRINT1("OpenIniBootLoaderStore: Failed to retrieve 'operating systems' section!\n");
                }
#endif
            }

            BootStore->OsIniSection = IniSection;
        }
    }

    *Handle = BootStore;
    return STATUS_SUCCESS;
}

static NTSTATUS
UnprotectBootIni(
    IN HANDLE FileHandle,
    OUT PULONG Attributes)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileInfo;

    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &FileInfo,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationFile() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    *Attributes = FileInfo.FileAttributes;

    /* Delete attributes SYSTEM, HIDDEN and READONLY */
    FileInfo.FileAttributes = FileInfo.FileAttributes &
                              ~(FILE_ATTRIBUTE_SYSTEM |
                                FILE_ATTRIBUTE_HIDDEN |
                                FILE_ATTRIBUTE_READONLY);

    Status = NtSetInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileInfo,
                                  sizeof(FILE_BASIC_INFORMATION),
                                  FileBasicInformation);
    if (!NT_SUCCESS(Status))
        DPRINT1("NtSetInformationFile() failed (Status 0x%08lx)\n", Status);

    return Status;
}

static NTSTATUS
ProtectBootIni(
    IN HANDLE FileHandle,
    IN ULONG Attributes)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileInfo;

    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &FileInfo,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationFile() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    FileInfo.FileAttributes = FileInfo.FileAttributes | Attributes;

    Status = NtSetInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileInfo,
                                  sizeof(FILE_BASIC_INFORMATION),
                                  FileBasicInformation);
    if (!NT_SUCCESS(Status))
        DPRINT1("NtSetInformationFile() failed (Status 0x%08lx)\n", Status);

    return Status;
}

static NTSTATUS
CloseIniBootLoaderStore(
    IN PVOID Handle)
{
    NTSTATUS Status;
    PBOOT_STORE_INI_CONTEXT BootStore = (PBOOT_STORE_INI_CONTEXT)Handle;
    ULONG FileAttribute = 0;

    // if (!BootStore)
        // return STATUS_INVALID_PARAMETER;

    if (BootStore->SectionHandle)
    {
        /*
         * The INI file was already opened because it already existed,
         * thus (in the case of NTLDR's boot.ini), unprotect it.
         */
        if (BootStore->Header.Type == NtLdr)
        {
            Status = UnprotectBootIni(BootStore->FileHandle, &FileAttribute);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Could not unprotect BOOT.INI ! (Status 0x%08lx)\n", Status);
                goto Quit;
            }
        }
    }

    IniCacheSaveByHandle(BootStore->IniCache, BootStore->FileHandle);

    /* In the case of NTLDR's boot.ini, re-protect the INI file */
    if (BootStore->Header.Type == NtLdr)
    {
        FileAttribute |= (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);
        Status = ProtectBootIni(BootStore->FileHandle, FileAttribute);
    }

Quit:
    IniCacheDestroy(BootStore->IniCache);

    if (BootStore->SectionHandle)
    {
        /* Finally, unmap and close the file */
        UnMapAndCloseFile(BootStore->FileHandle,
                          BootStore->SectionHandle,
                          BootStore->ViewBase);
    }
    else // if (BootStore->FileHandle)
    {
        /* Just close the file we have opened for creation */
        NtClose(BootStore->FileHandle);
    }

    /* Finally, free the boot store structure */
    RtlFreeHeap(ProcessHeap, 0, BootStore);

    // TODO: Use a correct Status based on the return values of the previous functions...
    return STATUS_SUCCESS;
}


NTSTATUS
OpenBootStoreByHandle(
    OUT PVOID* Handle,
    IN HANDLE PartitionDirectoryHandle, // OPTIONAL
    IN BOOT_STORE_TYPE Type,
    IN BOOLEAN CreateNew)
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

    return NtosBootLoaders[Type].OpenBootStore(Handle,
                                               PartitionDirectoryHandle,
                                               Type,
                                               CreateNew);
}

NTSTATUS
OpenBootStore_UStr(
    OUT PVOID* Handle,
    IN PUNICODE_STRING SystemPartitionPath,
    IN BOOT_STORE_TYPE Type,
    IN BOOLEAN CreateNew)
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
        DPRINT1("Failed to open SystemPartition '%wZ', Status 0x%08lx\n", SystemPartitionPath, Status);
        return Status;
    }

    Status = OpenBootStoreByHandle(Handle, PartitionDirectoryHandle, Type, CreateNew);

    /* Done! */
    NtClose(PartitionDirectoryHandle);
    return Status;
}

NTSTATUS
OpenBootStore(
    OUT PVOID* Handle,
    IN PCWSTR SystemPartition,
    IN BOOT_STORE_TYPE Type,
    IN BOOLEAN CreateNew)
{
    UNICODE_STRING SystemPartitionPath;
    RtlInitUnicodeString(&SystemPartitionPath, SystemPartition);
    return OpenBootStore_UStr(Handle, &SystemPartitionPath, Type, CreateNew);
}

NTSTATUS
CloseBootStore(
    IN PVOID Handle)
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
    PINICACHESECTION IniSection;
    PWCHAR Section = (PWCHAR)BootEntryKey;

    /* Insert the entry into the "Operating Systems" section */
    IniCacheInsertKey(BootStore->OsIniSection, NULL, INSERT_LAST,
                      Section, (PWSTR)BootEntry->FriendlyName);

    /* Create a new section */
    IniSection = IniCacheAppendSection(BootStore->IniCache, Section);

    if (BootEntry->OsOptionsLength >= sizeof(NTOS_OPTIONS) &&
        RtlCompareMemory(&BootEntry->OsOptions /* Signature */,
                         NTOS_OPTIONS_SIGNATURE,
                         RTL_FIELD_SIZE(NTOS_OPTIONS, Signature)) ==
                         RTL_FIELD_SIZE(NTOS_OPTIONS, Signature))
    {
        PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;

        /* BootType= */
        IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                          L"BootType", L"Windows2003");

        /* SystemPath= */
        IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                          L"SystemPath", (PWSTR)Options->OsLoadPath);

        /* Options= */
        IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                          L"Options", (PWSTR)Options->OsLoadOptions);
    }
    else
    if (BootEntry->OsOptionsLength >= sizeof(BOOT_SECTOR_OPTIONS) &&
        RtlCompareMemory(&BootEntry->OsOptions /* Signature */,
                         BOOT_SECTOR_OPTIONS_SIGNATURE,
                         RTL_FIELD_SIZE(BOOT_SECTOR_OPTIONS, Signature)) ==
                         RTL_FIELD_SIZE(BOOT_SECTOR_OPTIONS, Signature))
    {
        PBOOT_SECTOR_OPTIONS Options = (PBOOT_SECTOR_OPTIONS)&BootEntry->OsOptions;

        /* BootType= */
        IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                          L"BootType", L"BootSector");

        /* BootDrive= */
        IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                          L"BootDrive", (PWSTR)Options->Drive);

        /* BootPartition= */
        IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                          L"BootPartition", (PWSTR)Options->Partition);

        /* BootSector= */
        IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                          L"BootSectorFile", (PWSTR)Options->BootSectorFileName);
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
        if (IsNameNotQuoted) wcscat(Buffer, L"\"");
        wcscat(Buffer, InstallName);
        if (IsNameNotQuoted) wcscat(Buffer, L"\"");
        if (OsOptions)
        {
            wcscat(Buffer, L" ");
            wcscat(Buffer, OsOptions);
        }

        /* Insert the entry into the "Operating Systems" section */
        IniCacheInsertKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OsIniSection, NULL, INSERT_LAST,
                          (PWSTR)Options->OsLoadPath, Buffer);

        RtlFreeHeap(ProcessHeap, 0, Buffer);
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[BootStore->Type].Type);
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
    NTSTATUS Status = STATUS_SUCCESS;
    PBOOT_STORE_CONTEXT BootStore = (PBOOT_STORE_CONTEXT)Handle;
    PWCHAR TimeoutStr;

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
    if (BootStore->Type != FreeLdr || BootStore->Type != NtLdr)
    {
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[BootStore->Type].Type);
        return STATUS_NOT_SUPPORTED;
    }

    if (BootStore->Type == FreeLdr)
    {
        BootOptions->Version = FreeLdr;

        Status = IniCacheGetKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OptionsIniSection,
                                L"DefaultOS", (PWCHAR*)&BootOptions->CurrentBootEntryKey);
        if (!NT_SUCCESS(Status))
            BootOptions->CurrentBootEntryKey = 0;

        Status = IniCacheGetKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OptionsIniSection,
                                L"TimeOut", &TimeoutStr);
        if (NT_SUCCESS(Status) && TimeoutStr)
            BootOptions->Timeout = _wtoi(TimeoutStr);
        else
            BootOptions->Timeout = 0;
    }
    else if (BootStore->Type == NtLdr)
    {
        BootOptions->Version = NtLdr;

        Status = IniCacheGetKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OptionsIniSection,
                                L"default", (PWCHAR*)&BootOptions->CurrentBootEntryKey);
        if (!NT_SUCCESS(Status))
            BootOptions->CurrentBootEntryKey = 0;

        Status = IniCacheGetKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OptionsIniSection,
                                L"timeout", &TimeoutStr);
        if (NT_SUCCESS(Status) && TimeoutStr)
            BootOptions->Timeout = _wtoi(TimeoutStr);
        else
            BootOptions->Timeout = 0;
    }

    return STATUS_SUCCESS; // FIXME: use Status; instead?
}

NTSTATUS
SetBootStoreOptions(
    IN PVOID Handle,
    IN PBOOT_STORE_OPTIONS BootOptions,
    IN ULONG FieldsToChange)
{
    PBOOT_STORE_CONTEXT BootStore = (PBOOT_STORE_CONTEXT)Handle;
    WCHAR TimeoutStr[15];

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
    if (BootStore->Type != FreeLdr)
    {
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[BootStore->Type].Type);
        return STATUS_NOT_SUPPORTED;
    }

    if (BootOptions->Version != FreeLdr)
        return STATUS_INVALID_PARAMETER;

    //
    // TODO: Depending on the flags set in 'FieldsToChange',
    // change either one or both these bootloader options.
    //
    IniCacheInsertKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OptionsIniSection,
                      NULL, INSERT_LAST,
                      L"DefaultOS", (PWCHAR)BootOptions->CurrentBootEntryKey);

    StringCchPrintfW(TimeoutStr, ARRAYSIZE(TimeoutStr), L"%d", BootOptions->Timeout);
    IniCacheInsertKey(((PBOOT_STORE_INI_CONTEXT)BootStore)->OptionsIniSection,
                      NULL, INSERT_LAST,
                      L"TimeOut", TimeoutStr);

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
    PINICACHESECTION OsIniSection;
    PWCHAR SectionName, KeyData;
    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) +
                      max(sizeof(NTOS_OPTIONS), sizeof(BOOT_SECTOR_OPTIONS))];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
    PWCHAR Buffer;

    /* Enumerate all the valid installations listed in the "Operating Systems" section */
    Iterator = IniCacheFindFirstValue(BootStore->OsIniSection, &SectionName, &KeyData);
    if (!Iterator) return STATUS_SUCCESS;
    do
    {
        PWCHAR InstallName;
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

        DPRINT1("Boot entry '%S' in OS section '%S'\n", InstallName, SectionName);

        BootEntry->Version = FreeLdr;
        BootEntry->BootEntryKey = MAKESTRKEY(SectionName);
        BootEntry->FriendlyName = InstallName;
        BootEntry->BootFilePath = NULL;
        BootEntry->OsOptionsLength = 0;

        /* Search for an existing boot entry section */
        OsIniSection = IniCacheGetSection(BootStore->IniCache, SectionName);
        if (!OsIniSection)
            goto DoEnum;

        /* Check for supported boot type "Windows2003" */
        Status = IniCacheGetKey(OsIniSection, L"BootType", &KeyData);
        if (!NT_SUCCESS(Status) || (KeyData == NULL))
        {
            /* Certainly not a ReactOS installation */
            DPRINT1("No BootType value present!\n");
            goto DoEnum;
        }

        // TODO: What to do with "Windows" ; "WindowsNT40" ; "ReactOSSetup" ?
        if ((_wcsicmp(KeyData, L"Windows2003")     == 0) ||
            (_wcsicmp(KeyData, L"\"Windows2003\"") == 0))
        {
            /* BootType is Windows2003 */
            PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;

            DPRINT1("This is a '%S' boot entry\n", KeyData);

            BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
            RtlCopyMemory(Options->Signature,
                          NTOS_OPTIONS_SIGNATURE,
                          RTL_FIELD_SIZE(NTOS_OPTIONS, Signature));

            // BootEntry->BootFilePath = NULL;

            /* Check its SystemPath */
            Status = IniCacheGetKey(OsIniSection, L"SystemPath", &KeyData);
            if (!NT_SUCCESS(Status))
                Options->OsLoadPath = NULL;
            else
                Options->OsLoadPath = KeyData;
            // KeyData == SystemRoot;

            /* Check the optional Options */
            Status = IniCacheGetKey(OsIniSection, L"Options", &KeyData);
            if (!NT_SUCCESS(Status))
                Options->OsLoadOptions = NULL;
            else
                Options->OsLoadOptions = KeyData;
        }
        else
        if ((_wcsicmp(KeyData, L"BootSector")     == 0) ||
            (_wcsicmp(KeyData, L"\"BootSector\"") == 0))
        {
            /* BootType is BootSector */
            PBOOT_SECTOR_OPTIONS Options = (PBOOT_SECTOR_OPTIONS)&BootEntry->OsOptions;

            DPRINT1("This is a '%S' boot entry\n", KeyData);

            BootEntry->OsOptionsLength = sizeof(BOOT_SECTOR_OPTIONS);
            RtlCopyMemory(Options->Signature,
                          BOOT_SECTOR_OPTIONS_SIGNATURE,
                          RTL_FIELD_SIZE(BOOT_SECTOR_OPTIONS, Signature));

            // BootEntry->BootFilePath = NULL;

            /* Check its BootDrive */
            Status = IniCacheGetKey(OsIniSection, L"BootDrive", &KeyData);
            if (!NT_SUCCESS(Status))
                Options->Drive = NULL;
            else
                Options->Drive = KeyData;

            /* Check its BootPartition */
            Status = IniCacheGetKey(OsIniSection, L"BootPartition", &KeyData);
            if (!NT_SUCCESS(Status))
                Options->Partition = NULL;
            else
                Options->Partition = KeyData;

            /* Check its BootSector */
            Status = IniCacheGetKey(OsIniSection, L"BootSectorFile", &KeyData);
            if (!NT_SUCCESS(Status))
                Options->BootSectorFileName = NULL;
            else
                Options->BootSectorFileName = KeyData;
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
    while (IniCacheFindNextValue(Iterator, &SectionName, &KeyData));

    IniCacheFindClose(Iterator);
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
    PWCHAR SectionName, KeyData;
    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(NTOS_OPTIONS)];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
    PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;
    PWCHAR Buffer;
    ULONG BufferLength;

    /* Enumerate all the valid installations */
    Iterator = IniCacheFindFirstValue(BootStore->OsIniSection, &SectionName, &KeyData);
    if (!Iterator) return STATUS_SUCCESS;
    do
    {
        PWCHAR InstallName, OsOptions;
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
    while (IniCacheFindNextValue(Iterator, &SectionName, &KeyData));

    IniCacheFindClose(Iterator);
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
