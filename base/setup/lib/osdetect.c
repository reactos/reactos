/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/osdetect.c
 * PURPOSE:         NT 5.x family (MS Windows <= 2003, and ReactOS)
 *                  operating systems detection code.
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "ntverrsrc.h"
// #include "arcname.h"
#include "bldrsup.h"
#include "filesup.h"
#include "genlist.h"
#include "partlist.h"
#include "arcname.h"
#include "osdetect.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

/* Language-independent Vendor strings */
static const PCWSTR KnownVendors[] = { L"ReactOS", L"Microsoft" };


/* FUNCTIONS ****************************************************************/

static BOOLEAN
IsValidNTOSInstallation_UStr(
    IN PUNICODE_STRING SystemRootPath);

/*static*/ BOOLEAN
IsValidNTOSInstallation(
    IN PCWSTR SystemRoot);

static PNTOS_INSTALLATION
FindExistingNTOSInstall(
    IN PGENERIC_LIST List,
    IN PCWSTR SystemRootArcPath OPTIONAL,
    IN PUNICODE_STRING SystemRootNtPath OPTIONAL // or PCWSTR ?
    );

static PNTOS_INSTALLATION
AddNTOSInstallation(
    IN PGENERIC_LIST List,
    IN PCWSTR SystemRootArcPath,
    IN PUNICODE_STRING SystemRootNtPath, // or PCWSTR ?
    IN PCWSTR PathComponent,    // Pointer inside SystemRootNtPath buffer
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber,
    IN PPARTENTRY PartEntry OPTIONAL,
    IN PCWSTR InstallationName);

typedef struct _ENUM_INSTALLS_DATA
{
    IN OUT PGENERIC_LIST List;
    IN PPARTLIST PartList;
    // IN PPARTENTRY PartEntry;
} ENUM_INSTALLS_DATA, *PENUM_INSTALLS_DATA;

// PENUM_BOOT_ENTRIES_ROUTINE
static NTSTATUS
NTAPI
EnumerateInstallations(
    IN NTOS_BOOT_LOADER_TYPE Type,
    IN PNTOS_BOOT_ENTRY BootEntry,
    IN PVOID Parameter OPTIONAL)
{
    PENUM_INSTALLS_DATA Data = (PENUM_INSTALLS_DATA)Parameter;

    PNTOS_INSTALLATION NtOsInstall;
    UNICODE_STRING SystemRootPath;
    WCHAR SystemRoot[MAX_PATH];
    WCHAR InstallNameW[MAX_PATH];

    ULONG DiskNumber = 0, PartitionNumber = 0;
    PCWSTR PathComponent = NULL;
    PDISKENTRY DiskEntry = NULL;
    PPARTENTRY PartEntry = NULL;

    /* We have a boot entry */

    /* Check for supported boot type "Windows2003" */
    // TODO: What to do with "Windows" ; "WindowsNT40" ; "ReactOSSetup" ?
    if ((BootEntry->Version == NULL) ||
        ( (_wcsicmp(BootEntry->Version, L"Windows2003")     != 0) &&
          (_wcsicmp(BootEntry->Version, L"\"Windows2003\"") != 0) ))
    {
        /* This is not a ReactOS entry */
        DPRINT1("    An installation '%S' of unsupported type '%S'\n",
                BootEntry->FriendlyName, BootEntry->Version ? BootEntry->Version : L"n/a");
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    if (!BootEntry->OsLoadPath || !*BootEntry->OsLoadPath)
    {
        /* Certainly not a ReactOS installation */
        DPRINT1("    A Win2k3 install '%S' without an ARC path?!\n", BootEntry->FriendlyName);
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    DPRINT1("    Found a candidate Win2k3 install '%S' with ARC path '%S'\n",
            BootEntry->FriendlyName, BootEntry->OsLoadPath);
    // DPRINT1("    Found a Win2k3 install '%S' with ARC path '%S'\n",
            // BootEntry->FriendlyName, BootEntry->OsLoadPath);

    // TODO: Normalize the ARC path.

    /*
     * Check whether we already have an installation with this ARC path.
     * If this is the case, stop there.
     */
    NtOsInstall = FindExistingNTOSInstall(Data->List, BootEntry->OsLoadPath, NULL);
    if (NtOsInstall)
    {
        DPRINT1("    An NTOS installation with name \"%S\" already exists in SystemRoot '%wZ'\n",
                NtOsInstall->InstallationName, &NtOsInstall->SystemArcPath);
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    /*
     * Convert the ARC path into an NT path, from which we will deduce
     * the real disk drive & partition on which the candidate installation
     * resides, as well verifying whether it is indeed an NTOS installation.
     */
    RtlInitEmptyUnicodeString(&SystemRootPath, SystemRoot, sizeof(SystemRoot));
    if (!ArcPathToNtPath(&SystemRootPath, BootEntry->OsLoadPath, Data->PartList))
    {
        DPRINT1("ArcPathToNtPath(%S) failed, skip the installation.\n", BootEntry->OsLoadPath);
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    DPRINT1("ArcPathToNtPath() succeeded: '%S' --> '%wZ'\n",
            BootEntry->OsLoadPath, &SystemRootPath);

    /*
     * Check whether we already have an installation with this NT path.
     * If this is the case, stop there.
     */
    NtOsInstall = FindExistingNTOSInstall(Data->List, NULL /*BootEntry->OsLoadPath*/, &SystemRootPath);
    if (NtOsInstall)
    {
        DPRINT1("    An NTOS installation with name \"%S\" already exists in SystemRoot '%wZ'\n",
                NtOsInstall->InstallationName, &NtOsInstall->SystemNtPath);
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    DPRINT1("EnumerateInstallations: SystemRootPath: '%wZ'\n", &SystemRootPath);

    /* Check if this is a valid NTOS installation; stop there if it isn't one */
    if (!IsValidNTOSInstallation_UStr(&SystemRootPath))
    {
        /* Continue the enumeration */
        return STATUS_SUCCESS;
    }

    DPRINT1("Found a valid NTOS installation in SystemRoot ARC path '%S', NT path '%wZ'\n",
            BootEntry->OsLoadPath, &SystemRootPath);

    /* From the NT path, compute the disk, partition and path components */
    if (NtPathToDiskPartComponents(SystemRootPath.Buffer, &DiskNumber, &PartitionNumber, &PathComponent))
    {
        DPRINT1("SystemRootPath = '%wZ' points to disk #%d, partition #%d, path '%S'\n",
                &SystemRootPath, DiskNumber, PartitionNumber, PathComponent);

        /* Retrieve the corresponding disk and partition */
        if (!GetDiskOrPartition(Data->PartList, DiskNumber, PartitionNumber, &DiskEntry, &PartEntry))
        {
            DPRINT1("GetDiskOrPartition(disk #%d, partition #%d) failed\n",
                    DiskNumber, PartitionNumber);
        }
    }
    else
    {
        DPRINT1("NtPathToDiskPartComponents(%wZ) failed\n", &SystemRootPath);
    }

    /* Add the discovered NTOS installation into the list */
    if (PartEntry && PartEntry->DriveLetter)
    {
        /* We have retrieved a partition that is mounted */
        StringCchPrintfW(InstallNameW, ARRAYSIZE(InstallNameW), L"%C:%s  \"%s\"",
                         PartEntry->DriveLetter, PathComponent, BootEntry->FriendlyName);
    }
    else
    {
        /* We failed somewhere, just show the NT path */
        StringCchPrintfW(InstallNameW, ARRAYSIZE(InstallNameW), L"%wZ  \"%s\"",
                         &SystemRootPath, BootEntry->FriendlyName);
    }
    AddNTOSInstallation(Data->List, BootEntry->OsLoadPath,
                        &SystemRootPath, PathComponent,
                        DiskNumber, PartitionNumber, PartEntry,
                        InstallNameW);

    /* Continue the enumeration */
    return STATUS_SUCCESS;
}

/*
 * FindSubStrI(PCWSTR str, PCWSTR strSearch) :
 *    Searches for a sub-string 'strSearch' inside 'str', similarly to what
 *    wcsstr(str, strSearch) does, but ignores the case during the comparisons.
 */
PCWSTR FindSubStrI(PCWSTR str, PCWSTR strSearch)
{
    PCWSTR cp = str;
    PCWSTR s1, s2;

    if (!*strSearch)
        return str;

    while (*cp)
    {
        s1 = cp;
        s2 = strSearch;

        while (*s1 && *s2 && (towupper(*s1) == towupper(*s2)))
            ++s1, ++s2;

        if (!*s2)
            return cp;

        ++cp;
    }

    return NULL;
}

static BOOLEAN
CheckForValidPEAndVendor(
    IN HANDLE RootDirectory OPTIONAL,
    IN PCWSTR PathNameToFile,
    OUT PUNICODE_STRING VendorName
    )
{
    BOOLEAN Success = FALSE;
    NTSTATUS Status;
    HANDLE FileHandle, SectionHandle;
    // SIZE_T ViewSize;
    PVOID ViewBase;
    PVOID VersionBuffer = NULL; // Read-only
    PVOID pvData = NULL;
    UINT BufLen = 0;

    if (VendorName->MaximumLength < sizeof(UNICODE_NULL))
        return FALSE;

    *VendorName->Buffer = UNICODE_NULL;
    VendorName->Length = 0;

    Status = OpenAndMapFile(RootDirectory, PathNameToFile,
                            &FileHandle, &SectionHandle, &ViewBase,
                            NULL, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open and map file '%S', Status 0x%08lx\n", PathNameToFile, Status);
        return FALSE; // Status;
    }

    /* Make sure it's a valid PE file */
    if (!RtlImageNtHeader(ViewBase))
    {
        DPRINT1("File '%S' does not seem to be a valid PE, bail out\n", PathNameToFile);
        Status = STATUS_INVALID_IMAGE_FORMAT;
        goto UnmapFile;
    }

    /*
     * Search for a valid executable version and vendor.
     * NOTE: The module is loaded as a data file, it should be marked as such.
     */
    Status = NtGetVersionResource((PVOID)((ULONG_PTR)ViewBase | 1), &VersionBuffer, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get version resource for file '%S', Status 0x%08lx\n", PathNameToFile, Status);
        goto UnmapFile;
    }

    Status = NtVerQueryValue(VersionBuffer, L"\\VarFileInfo\\Translation", &pvData, &BufLen);
    if (NT_SUCCESS(Status))
    {
        USHORT wCodePage = 0, wLangID = 0;
        WCHAR FileInfo[MAX_PATH];

        wCodePage = LOWORD(*(ULONG*)pvData);
        wLangID   = HIWORD(*(ULONG*)pvData);

        StringCchPrintfW(FileInfo, ARRAYSIZE(FileInfo),
                         L"StringFileInfo\\%04X%04X\\CompanyName",
                         wCodePage, wLangID);

        Status = NtVerQueryValue(VersionBuffer, FileInfo, &pvData, &BufLen);

        /* Fixup the Status in case pvData is NULL */
        if (NT_SUCCESS(Status) && !pvData)
            Status = STATUS_NOT_FOUND;

        if (NT_SUCCESS(Status) /*&& pvData*/)
        {
            /* BufLen includes the NULL terminator count */
            DPRINT1("Found version vendor: \"%S\" for file '%S'\n", pvData, PathNameToFile);

            StringCbCopyNW(VendorName->Buffer, VendorName->MaximumLength,
                           pvData, BufLen * sizeof(WCHAR));
            VendorName->Length = wcslen(VendorName->Buffer) * sizeof(WCHAR);

            Success = TRUE;
        }
    }

    if (!NT_SUCCESS(Status))
        DPRINT1("No version vendor found for file '%S'\n", PathNameToFile);

UnmapFile:
    /* Finally, unmap and close the file */
    UnMapFile(SectionHandle, ViewBase);
    NtClose(FileHandle);

    return Success;
}

//
// TODO: Instead of returning TRUE/FALSE, it would be nice to return
// a flag indicating:
// - whether the installation is actually valid;
// - if it's broken or not (aka. needs for repair, or just upgrading).
//
static BOOLEAN
IsValidNTOSInstallationByHandle(
    IN HANDLE SystemRootDirectory)
{
    BOOLEAN Success = FALSE;
    PCWSTR PathName;
    USHORT i;
    UNICODE_STRING VendorName;
    WCHAR VendorNameBuffer[MAX_PATH];

    /* Check for the existence of \SystemRoot\System32 */
    PathName = L"System32\\";
    if (!DoesPathExist(SystemRootDirectory, PathName))
    {
        // DPRINT1("Failed to open directory '%S', Status 0x%08lx\n", PathName, Status);
        return FALSE;
    }

    /* Check for the existence of \SystemRoot\System32\drivers */
    PathName = L"System32\\drivers\\";
    if (!DoesPathExist(SystemRootDirectory, PathName))
    {
        // DPRINT1("Failed to open directory '%S', Status 0x%08lx\n", PathName, Status);
        return FALSE;
    }

    /* Check for the existence of \SystemRoot\System32\config */
    PathName = L"System32\\config\\";
    if (!DoesPathExist(SystemRootDirectory, PathName))
    {
        // DPRINT1("Failed to open directory '%S', Status 0x%08lx\n", PathName, Status);
        return FALSE;
    }

#if 0
    /*
     * Check for the existence of SYSTEM and SOFTWARE hives in \SystemRoot\System32\config
     * (but we don't check here whether they are actually valid).
     */
    PathName = L"System32\\config\\SYSTEM";
    if (!DoesFileExist(SystemRootDirectory, PathName))
    {
        // DPRINT1("Failed to open file '%S', Status 0x%08lx\n", PathName, Status);
        return FALSE;
    }
    PathName = L"System32\\config\\SOFTWARE";
    if (!DoesFileExist(SystemRootDirectory, PathName))
    {
        // DPRINT1("Failed to open file '%S', Status 0x%08lx\n", PathName, Status);
        return FALSE;
    }
#endif

    RtlInitEmptyUnicodeString(&VendorName, VendorNameBuffer, sizeof(VendorNameBuffer));

    /* Check for the existence of \SystemRoot\System32\ntoskrnl.exe and retrieves its vendor name */
    PathName = L"System32\\ntoskrnl.exe";
    Success = CheckForValidPEAndVendor(SystemRootDirectory, PathName, &VendorName);
    if (!Success)
        DPRINT1("Kernel executable '%S' is either not a PE file, or does not have any vendor?\n", PathName);

    /* The kernel gives the OS its flavour */
    if (Success)
    {
        for (i = 0; i < ARRAYSIZE(KnownVendors); ++i)
        {
            Success = !!FindSubStrI(VendorName.Buffer, KnownVendors[i]);
            if (Success)
            {
                /* We have found a correct vendor combination */
                DPRINT1("IsValidNTOSInstallation: We've got an NTOS installation from %S !\n", KnownVendors[i]);
                break;
            }
        }
    }

    /* OPTIONAL: Check for the existence of \SystemRoot\System32\ntkrnlpa.exe */

    /* Check for the existence of \SystemRoot\System32\ntdll.dll and retrieves its vendor name */
    PathName = L"System32\\ntdll.dll";
    Success = CheckForValidPEAndVendor(SystemRootDirectory, PathName, &VendorName);
    if (!Success)
        DPRINT1("User-mode DLL '%S' is either not a PE file, or does not have any vendor?\n", PathName);
    if (Success)
    {
        for (i = 0; i < ARRAYSIZE(KnownVendors); ++i)
        {
            if (!!FindSubStrI(VendorName.Buffer, KnownVendors[i]))
            {
                /* We have found a correct vendor combination */
                DPRINT1("IsValidNTOSInstallation: The user-mode DLL '%S' is from %S\n", PathName, KnownVendors[i]);
                break;
            }
        }
    }

    return Success;
}

static BOOLEAN
IsValidNTOSInstallation_UStr(
    IN PUNICODE_STRING SystemRootPath)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE SystemRootDirectory;
    BOOLEAN Success;

    /* Open SystemRootPath */
    InitializeObjectAttributes(&ObjectAttributes,
                               SystemRootPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&SystemRootDirectory,
                        FILE_LIST_DIRECTORY | FILE_TRAVERSE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open SystemRoot '%wZ', Status 0x%08lx\n", SystemRootPath, Status);
        return FALSE;
    }

    Success = IsValidNTOSInstallationByHandle(SystemRootDirectory);

    /* Done! */
    NtClose(SystemRootDirectory);
    return Success;
}

/*static*/ BOOLEAN
IsValidNTOSInstallation(
    IN PCWSTR SystemRoot)
{
    UNICODE_STRING SystemRootPath;
    RtlInitUnicodeString(&SystemRootPath, SystemRoot);
    return IsValidNTOSInstallationByHandle(&SystemRootPath);
}

static VOID
DumpNTOSInstalls(
    IN PGENERIC_LIST List)
{
    PGENERIC_LIST_ENTRY Entry;
    PNTOS_INSTALLATION NtOsInstall;
    ULONG NtOsInstallsCount = GetNumberOfListEntries(List);

    DPRINT1("There %s %d installation%s detected:\n",
            NtOsInstallsCount >= 2 ? "are" : "is",
            NtOsInstallsCount,
            NtOsInstallsCount >= 2 ? "s" : "");

    Entry = GetFirstListEntry(List);
    while (Entry)
    {
        NtOsInstall = (PNTOS_INSTALLATION)GetListEntryUserData(Entry);
        Entry = GetNextListEntry(Entry);

        DPRINT1("    On disk #%d, partition #%d: Installation \"%S\" in SystemRoot '%wZ'\n",
                NtOsInstall->DiskNumber, NtOsInstall->PartitionNumber,
                NtOsInstall->InstallationName, &NtOsInstall->SystemNtPath);
    }

    DPRINT1("Done.\n");
}

static PNTOS_INSTALLATION
FindExistingNTOSInstall(
    IN PGENERIC_LIST List,
    IN PCWSTR SystemRootArcPath OPTIONAL,
    IN PUNICODE_STRING SystemRootNtPath OPTIONAL // or PCWSTR ?
    )
{
    PGENERIC_LIST_ENTRY Entry;
    PNTOS_INSTALLATION NtOsInstall;
    UNICODE_STRING SystemArcPath;

    /*
     * We search either via ARC path or NT path.
     * If both pointers are NULL then we fail straight away.
     */
    if (!SystemRootArcPath && !SystemRootNtPath)
        return NULL;

    RtlInitUnicodeString(&SystemArcPath, SystemRootArcPath);

    Entry = GetFirstListEntry(List);
    while (Entry)
    {
        NtOsInstall = (PNTOS_INSTALLATION)GetListEntryUserData(Entry);
        Entry = GetNextListEntry(Entry);

        /*
         * Note that if both ARC paths are equal, then the corresponding
         * NT paths must be the same. However, two ARC paths may be different
         * but resolve into the same NT path.
         */
        if ( (SystemRootArcPath &&
              RtlEqualUnicodeString(&NtOsInstall->SystemArcPath,
                                    &SystemArcPath, TRUE)) ||
             (SystemRootNtPath  &&
              RtlEqualUnicodeString(&NtOsInstall->SystemNtPath,
                                    SystemRootNtPath, TRUE)) )
        {
            /* Found it! */
            return NtOsInstall;
        }
    }

    return NULL;
}

static PNTOS_INSTALLATION
AddNTOSInstallation(
    IN PGENERIC_LIST List,
    IN PCWSTR SystemRootArcPath,
    IN PUNICODE_STRING SystemRootNtPath, // or PCWSTR ?
    IN PCWSTR PathComponent,    // Pointer inside SystemRootNtPath buffer
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber,
    IN PPARTENTRY PartEntry OPTIONAL,
    IN PCWSTR InstallationName)
{
    PNTOS_INSTALLATION NtOsInstall;
    SIZE_T ArcPathLength, NtPathLength;
    CHAR InstallNameA[MAX_PATH];

    /* Is there already any installation with these settings? */
    NtOsInstall = FindExistingNTOSInstall(List, SystemRootArcPath, SystemRootNtPath);
    if (NtOsInstall)
    {
        DPRINT1("An NTOS installation with name \"%S\" already exists on disk #%d, partition #%d, in SystemRoot '%wZ'\n",
                NtOsInstall->InstallationName, NtOsInstall->DiskNumber, NtOsInstall->PartitionNumber, &NtOsInstall->SystemNtPath);
        //
        // NOTE: We may use its "IsDefault" attribute, and only keep the entries that have IsDefault == TRUE...
        // Setting IsDefault to TRUE would imply searching for the "Default" entry in the loader configuration file.
        //
        return NtOsInstall;
    }

    ArcPathLength = (wcslen(SystemRootArcPath) + 1) * sizeof(WCHAR);
    // NtPathLength  = ROUND_UP(SystemRootNtPath->Length + sizeof(UNICODE_NULL), sizeof(WCHAR));
    NtPathLength  = SystemRootNtPath->Length + sizeof(UNICODE_NULL);

    /* None was found, so add a new one */
    NtOsInstall = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY,
                                  sizeof(*NtOsInstall) +
                                  ArcPathLength + NtPathLength);
    if (!NtOsInstall)
        return NULL;

    NtOsInstall->DiskNumber = DiskNumber;
    NtOsInstall->PartitionNumber = PartitionNumber;
    NtOsInstall->PartEntry = PartEntry;

    RtlInitEmptyUnicodeString(&NtOsInstall->SystemArcPath,
                              (PWCHAR)(NtOsInstall + 1),
                              ArcPathLength);
    RtlCopyMemory(NtOsInstall->SystemArcPath.Buffer, SystemRootArcPath, ArcPathLength);
    NtOsInstall->SystemArcPath.Length = ArcPathLength - sizeof(UNICODE_NULL);

    RtlInitEmptyUnicodeString(&NtOsInstall->SystemNtPath,
                              (PWCHAR)((ULONG_PTR)(NtOsInstall + 1) + ArcPathLength),
                              NtPathLength);
    RtlCopyUnicodeString(&NtOsInstall->SystemNtPath, SystemRootNtPath);
    NtOsInstall->PathComponent = NtOsInstall->SystemNtPath.Buffer +
                                    (PathComponent - SystemRootNtPath->Buffer);

    StringCchCopyW(NtOsInstall->InstallationName, ARRAYSIZE(NtOsInstall->InstallationName), InstallationName);

    // Having the GENERIC_LIST storing the display item string plainly sucks...
    StringCchPrintfA(InstallNameA, ARRAYSIZE(InstallNameA), "%S", InstallationName);
    AppendGenericListEntry(List, InstallNameA, NtOsInstall, FALSE);

    return NtOsInstall;
}

static VOID
FindNTOSInstallations(
    IN OUT PGENERIC_LIST List,
    IN PPARTLIST PartList,
    IN PPARTENTRY PartEntry)
{
    NTSTATUS Status;
    ULONG DiskNumber = PartEntry->DiskEntry->DiskNumber;
    ULONG PartitionNumber = PartEntry->PartitionNumber;
    HANDLE PartitionDirectoryHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PartitionRootPath;
    NTOS_BOOT_LOADER_TYPE Type;
    PVOID BootStoreHandle;
    ENUM_INSTALLS_DATA Data;
    ULONG Version;
    WCHAR PathBuffer[MAX_PATH];

    /* Set PartitionRootPath */
    StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                     L"\\Device\\Harddisk%lu\\Partition%lu\\",
                     DiskNumber, PartitionNumber);
    RtlInitUnicodeString(&PartitionRootPath, PathBuffer);
    DPRINT1("FindNTOSInstallations: PartitionRootPath: '%wZ'\n", &PartitionRootPath);

    /* Open the partition */
    InitializeObjectAttributes(&ObjectAttributes,
                               &PartitionRootPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&PartitionDirectoryHandle,
                        FILE_LIST_DIRECTORY | FILE_TRAVERSE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open partition '%wZ', Status 0x%08lx\n", &PartitionRootPath, Status);
        return;
    }

    Data.List = List;
    Data.PartList = PartList;

    /* Try to see whether we recognize some NT boot loaders */
    for (Type = FreeLdr; Type < BldrTypeMax; ++Type)
    {
        Status = FindNTOSBootLoader(PartitionDirectoryHandle, Type, &Version);
        if (!NT_SUCCESS(Status))
        {
            /* The loader does not exist, continue with another one */
            DPRINT1("Loader type '%d' does not exist, or an error happened (Status 0x%08lx), continue with another one...\n",
                    Type, Status);
            continue;
        }

        /* The loader exists, try to enumerate its boot entries */
        DPRINT1("Analyse the OS installations for loader type '%d' in disk #%d, partition #%d\n",
                Type, DiskNumber, PartitionNumber);

        Status = OpenNTOSBootLoaderStoreByHandle(&BootStoreHandle, PartitionDirectoryHandle, Type, FALSE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Could not open the NTOS boot store of type '%d' (Status 0x%08lx), continue with another one...\n",
                    Type, Status);
            continue;
        }
        EnumerateNTOSBootEntries(BootStoreHandle, EnumerateInstallations, &Data);
        CloseNTOSBootLoaderStore(BootStoreHandle);
    }

    /* Close the partition */
    NtClose(PartitionDirectoryHandle);
}

// static
FORCEINLINE BOOLEAN
ShouldICheckThisPartition(
    IN PPARTENTRY PartEntry)
{
    if (!PartEntry)
        return FALSE;

    return PartEntry->IsPartitioned &&
           !IsContainerPartition(PartEntry->PartitionType) /* alternatively: PartEntry->PartitionNumber != 0 */ &&
           !PartEntry->New &&
           (PartEntry->FormatState == Preformatted /* || PartEntry->FormatState == Formatted */);
}

// EnumerateNTOSInstallations
PGENERIC_LIST
CreateNTOSInstallationsList(
    IN PPARTLIST PartList)
{
    PGENERIC_LIST List;
    PLIST_ENTRY Entry, Entry2;
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

    List = CreateGenericList();
    if (List == NULL)
        return NULL;

    /* Loop each available disk ... */
    Entry = PartList->DiskListHead.Flink;
    while (Entry != &PartList->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);
        Entry = Entry->Flink;

        DPRINT1("Disk #%d\n", DiskEntry->DiskNumber);

        /* ... and for each disk, loop each available partition */

        /* First, the primary partitions */
        Entry2 = DiskEntry->PrimaryPartListHead.Flink;
        while (Entry2 != &DiskEntry->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            Entry2 = Entry2->Flink;

            ASSERT(PartEntry->DiskEntry == DiskEntry);

            DPRINT1("   Primary Partition #%d, index %d - Type 0x%02x, IsLogical = %s, IsPartitioned = %s, IsNew = %s, AutoCreate = %s, FormatState = %lu -- Should I check it? %s\n",
                    PartEntry->PartitionNumber, PartEntry->PartitionIndex,
                    PartEntry->PartitionType, PartEntry->LogicalPartition ? "TRUE" : "FALSE",
                    PartEntry->IsPartitioned ? "TRUE" : "FALSE",
                    PartEntry->New ? "Yes" : "No",
                    PartEntry->AutoCreate ? "Yes" : "No",
                    PartEntry->FormatState,
                    ShouldICheckThisPartition(PartEntry) ? "YES!" : "NO!");

            if (ShouldICheckThisPartition(PartEntry))
                FindNTOSInstallations(List, PartList, PartEntry);
        }

        /* Then, the logical partitions (present in the extended partition) */
        Entry2 = DiskEntry->LogicalPartListHead.Flink;
        while (Entry2 != &DiskEntry->LogicalPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            Entry2 = Entry2->Flink;

            ASSERT(PartEntry->DiskEntry == DiskEntry);

            DPRINT1("   Logical Partition #%d, index %d - Type 0x%02x, IsLogical = %s, IsPartitioned = %s, IsNew = %s, AutoCreate = %s, FormatState = %lu -- Should I check it? %s\n",
                    PartEntry->PartitionNumber, PartEntry->PartitionIndex,
                    PartEntry->PartitionType, PartEntry->LogicalPartition ? "TRUE" : "FALSE",
                    PartEntry->IsPartitioned ? "TRUE" : "FALSE",
                    PartEntry->New ? "Yes" : "No",
                    PartEntry->AutoCreate ? "Yes" : "No",
                    PartEntry->FormatState,
                    ShouldICheckThisPartition(PartEntry) ? "YES!" : "NO!");

            if (ShouldICheckThisPartition(PartEntry))
                FindNTOSInstallations(List, PartList, PartEntry);
        }
    }

    /**** Debugging: List all the collected installations ****/
    DumpNTOSInstalls(List);

    return List;
}

/* EOF */
