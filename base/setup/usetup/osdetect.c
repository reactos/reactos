/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/osdetect.c
 * PURPOSE:         NT 5.x family (MS Windows <= 2003, and ReactOS)
 *                  operating systems detection code.
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "usetup.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

extern PPARTLIST PartitionList;

/* Language-independent Vendor strings */
static const PCWSTR KnownVendors[] = { L"ReactOS", L"Microsoft" };


/* FUNCTIONS ****************************************************************/

#if 0

BOOL IsWindowsOS(VOID)
{
    // TODO:
    // Load the "SystemRoot\System32\Config\SOFTWARE" hive and mount it,
    // then go to (SOFTWARE\\)Microsoft\\Windows NT\\CurrentVersion,
    // check the REG_SZ value "ProductName" and see whether it's "Windows"
    // or "ReactOS". One may also check the REG_SZ "CurrentVersion" value,
    // the REG_SZ "SystemRoot" and "PathName" values (what are the differences??).
    //
    // Optionally, looking at the SYSTEM hive, CurrentControlSet\\Control,
    // REG_SZ values "SystemBootDevice" (and "FirmwareBootDevice" ??)...
    //

    /* ReactOS reports as Windows NT 5.2 */
    HKEY hKey = NULL;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                      0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        LONG ret;
        DWORD dwType = 0, dwBufSize = 0;

        ret = RegQueryValueExW(hKey, L"ProductName", NULL, &dwType, NULL, &dwBufSize);
        if (ret == ERROR_SUCCESS && dwType == REG_SZ)
        {
            LPTSTR lpszProductName = (LPTSTR)MemAlloc(0, dwBufSize);
            RegQueryValueExW(hKey, L"ProductName", NULL, &dwType, (LPBYTE)lpszProductName, &dwBufSize);

            bIsWindowsOS = (FindSubStrI(lpszProductName, _T("Windows")) != NULL);

            MemFree(lpszProductName);
        }

        RegCloseKey(hKey);
    }

    return bIsWindowsOS;
}

#endif

typedef enum _NTOS_BOOT_LOADER_TYPE
{
    FreeLdr,    // ReactOS' FreeLDR
    NtLdr,      // Windows <= 2k3 NTLDR
//  BootMgr,    // Vista+ BCD-oriented BOOTMGR
} NTOS_BOOT_LOADER_TYPE;

typedef struct _NTOS_BOOT_LOADER_FILES
{
    NTOS_BOOT_LOADER_TYPE Type;
    PCWSTR LoaderExecutable;
    PCWSTR LoaderConfigurationFile;
    // EnumerateInstallations;
} NTOS_BOOT_LOADER_FILES, *PNTOS_BOOT_LOADER_FILES;

// Question 1: What if config file is optional?
// Question 2: What if many config files are possible?
NTOS_BOOT_LOADER_FILES NtosBootLoaders[] =
{
    {FreeLdr, L"freeldr.sys", L"freeldr.ini"},
    {NtLdr  , L"ntldr"      , L"boot.ini"},
    {NtLdr  , L"setupldr"   , L"txtsetup.sif"},
//  {BootMgr, L"bootmgr"    , ???}
};

#if 0

static VOID
EnumerateInstallationsFreeLdr()
{
    NTSTATUS Status;
    PINICACHE IniCache;
    PINICACHESECTION IniSection;
    PINICACHESECTION OsIniSection;
    WCHAR SectionName[80];
    WCHAR OsName[80];
    WCHAR SystemPath[200];
    WCHAR SectionName2[200];
    PWCHAR KeyData;
    ULONG i,j;

    /* Open an *existing* FreeLdr.ini configuration file */
    Status = IniCacheLoad(&IniCache, IniPath, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Get "Operating Systems" section */
    IniSection = IniCacheGetSection(IniCache, L"Operating Systems");
    if (IniSection == NULL)
    {
        IniCacheDestroy(IniCache);
        return STATUS_UNSUCCESSFUL;
    }

    /* NOTE that we enumerate all the valid installations, not just the default one */
    while (TRUE)
    {
        Status = IniCacheGetKey(IniSection, SectionName, &KeyData);
        if (!NT_SUCCESS(Status))
            break;

        // TODO some foobaring...

        // TODO 2 : Remind the entry name so that we may display it as available installation...

        /* Search for an existing ReactOS entry */
        OsIniSection = IniCacheGetSection(IniCache, SectionName2);
        if (OsIniSection != NULL)
        {
            BOOLEAN UseExistingEntry = TRUE;

            /* Check for supported boot type "Windows2003" */
            Status = IniCacheGetKey(OsIniSection, L"BootType", &KeyData);
            if (NT_SUCCESS(Status))
            {
                if ((KeyData == NULL) ||
                    ( (_wcsicmp(KeyData, L"Windows2003") != 0) &&
                      (_wcsicmp(KeyData, L"\"Windows2003\"") != 0) ))
                {
                    /* This is not a ReactOS entry */
                    UseExistingEntry = FALSE;
                }
            }
            else
            {
                UseExistingEntry = FALSE;
            }

            if (UseExistingEntry)
            {
                /* BootType is Windows2003. Now check SystemPath. */
                Status = IniCacheGetKey(OsIniSection, L"SystemPath", &KeyData);
                if (NT_SUCCESS(Status))
                {
                    swprintf(SystemPath, L"\"%s\"", ArcPath);
                    if ((KeyData == NULL) ||
                        ( (_wcsicmp(KeyData, ArcPath) != 0) &&
                          (_wcsicmp(KeyData, SystemPath) != 0) ))
                    {
                        /* This entry is a ReactOS entry, but the SystemRoot
                           does not match the one we are looking for. */
                        UseExistingEntry = FALSE;
                    }
                }
                else
                {
                    UseExistingEntry = FALSE;
                }
            }
        }
    }

    IniCacheDestroy(IniCache);
}

#endif


/***
*wchar_t *wcsstr(string1, string2) - search for string2 in string1
*       (wide strings)
*
*Purpose:
*       finds the first occurrence of string2 in string1 (wide strings)
*
*Entry:
*       wchar_t *string1 - string to search in
*       wchar_t *string2 - string to search for
*
*Exit:
*       returns a pointer to the first occurrence of string2 in
*       string1, or NULL if string2 does not occur in string1
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/
PWSTR FindSubStrI(PCWSTR str, PCWSTR strSearch)
{
    PWSTR cp = (PWSTR)str;
    PWSTR s1, s2;

    if (!*strSearch)
        return (PWSTR)str;

    while (*cp)
    {
        s1 = cp;
        s2 = (PWSTR)strSearch;

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
    IN PCWSTR PathName OPTIONAL,
    IN PCWSTR FileName,             // OPTIONAL
    IN PCWSTR VendorName    // Better would be OUT PCWSTR*, and the function returning NTSTATUS ?
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

    Status = OpenAndMapFile(RootDirectory, PathName, FileName,
                            &FileHandle, &SectionHandle, &ViewBase);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open and map file %wZ, Status 0x%08lx\n", &FileName, Status);
        return FALSE; // Status;
    }

    /* Make sure it's a valid PE file */
    if (!RtlImageNtHeader(ViewBase))
    {
        DPRINT1("File %wZ does not seem to be a valid PE, bail out\n", &FileName);
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
        DPRINT1("Failed to get version resource for file %wZ, Status 0x%08lx\n", &FileName, Status);
        goto UnmapFile;
    }

    Status = NtVerQueryValue(VersionBuffer, L"\\VarFileInfo\\Translation", &pvData, &BufLen);
    if (NT_SUCCESS(Status))
    {
        USHORT wCodePage = 0, wLangID = 0;
        WCHAR FileInfo[MAX_PATH];
        UNICODE_STRING Vendor;

        wCodePage = LOWORD(*(ULONG*)pvData);
        wLangID   = HIWORD(*(ULONG*)pvData);

        StringCchPrintfW(FileInfo, ARRAYSIZE(FileInfo),
                         L"StringFileInfo\\%04X%04X\\CompanyName",
                         wCodePage, wLangID);

        Status = NtVerQueryValue(VersionBuffer, FileInfo, &pvData, &BufLen);
        if (NT_SUCCESS(Status) && pvData)
        {
            /* BufLen includes the NULL terminator count */
            RtlInitEmptyUnicodeString(&Vendor, pvData, BufLen * sizeof(WCHAR));
            Vendor.Length = Vendor.MaximumLength - sizeof(UNICODE_NULL);

            DPRINT1("Found version vendor: \"%wZ\" for file %wZ\n", &Vendor, &FileName);

            Success = !!FindSubStrI(pvData, VendorName);
        }
        else
        {
            DPRINT1("No version vendor found for file %wZ\n", &FileName);
        }
    }

UnmapFile:
    /* Finally, unmap and close the file */
    UnMapFile(SectionHandle, ViewBase);
    NtClose(FileHandle);

    return Success;
}

static BOOLEAN
IsValidNTOSInstallation(
    IN HANDLE PartitionHandle,
    IN PCWSTR SystemRoot)
{
    BOOLEAN Success = FALSE;
    USHORT i;
    WCHAR PathBuffer[MAX_PATH];

    // DoesPathExist(PartitionHandle, SystemRoot, L"System32\\"); etc...

    /* Check for the existence of \SystemRoot\System32 */
    StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer), L"%s%s", SystemRoot, L"System32\\");
    if (!DoesPathExist(PartitionHandle, PathBuffer))
    {
        // DPRINT1("Failed to open directory %wZ, Status 0x%08lx\n", &FileName, Status);
        return FALSE;
    }

    /* Check for the existence of \SystemRoot\System32\drivers */
    StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer), L"%s%s", SystemRoot, L"System32\\drivers\\");
    if (!DoesPathExist(PartitionHandle, PathBuffer))
    {
        // DPRINT1("Failed to open directory %wZ, Status 0x%08lx\n", &FileName, Status);
        return FALSE;
    }

    /* Check for the existence of \SystemRoot\System32\config */
    StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer), L"%s%s", SystemRoot, L"System32\\config\\");
    if (!DoesPathExist(PartitionHandle, PathBuffer))
    {
        // DPRINT1("Failed to open directory %wZ, Status 0x%08lx\n", &FileName, Status);
        return FALSE;
    }

#if 0
    /*
     * Check for the existence of SYSTEM and SOFTWARE hives in \SystemRoot\System32\config
     * (but we don't check here whether they are actually valid).
     */
    if (!DoesFileExist(PartitionHandle, SystemRoot, L"System32\\config\\SYSTEM"))
    {
        // DPRINT1("Failed to open file %wZ, Status 0x%08lx\n", &FileName, Status);
        return FALSE;
    }
    if (!DoesFileExist(PartitionHandle, SystemRoot, L"System32\\config\\SOFTWARE"))
    {
        // DPRINT1("Failed to open file %wZ, Status 0x%08lx\n", &FileName, Status);
        return FALSE;
    }
#endif

    for (i = 0; i < ARRAYSIZE(KnownVendors); ++i)
    {
        /* Check for the existence of \SystemRoot\System32\ntoskrnl.exe and verify its version */
        Success = CheckForValidPEAndVendor(PartitionHandle, SystemRoot, L"System32\\ntoskrnl.exe", KnownVendors[i]);

        /* OPTIONAL: Check for the existence of \SystemRoot\System32\ntkrnlpa.exe */

        /* Check for the existence of \SystemRoot\System32\ntdll.dll */
        Success = CheckForValidPEAndVendor(PartitionHandle, SystemRoot, L"System32\\ntdll.dll", KnownVendors[i]);

        /* We have found a correct vendor combination */
        if (Success)
            break;
    }

    return Success;
}

static VOID
ListNTOSInstalls(
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

        DPRINT1("    On disk #%d, partition #%d: Installation \"%S\" in SystemRoot %S\n",
                NtOsInstall->DiskNumber, NtOsInstall->PartitionNumber,
                NtOsInstall->InstallationName, NtOsInstall->SystemRoot);
    }

    DPRINT1("Done.\n");
}

static PNTOS_INSTALLATION
FindExistingNTOSInstall(
    IN PGENERIC_LIST List,
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber,
    IN PCWSTR SystemRoot)
{
    PGENERIC_LIST_ENTRY Entry;
    PNTOS_INSTALLATION NtOsInstall;

    Entry = GetFirstListEntry(List);
    while (Entry)
    {
        NtOsInstall = (PNTOS_INSTALLATION)GetListEntryUserData(Entry);
        Entry = GetNextListEntry(Entry);

        if (NtOsInstall->DiskNumber == DiskNumber &&
            NtOsInstall->PartitionNumber == PartitionNumber &&
            _wcsicmp(NtOsInstall->SystemRoot, SystemRoot) == 0)
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
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber,
    IN PCWSTR SystemRoot,
    IN PCWSTR InstallationName)
{
    PNTOS_INSTALLATION NtOsInstall;
    CHAR InstallNameA[MAX_PATH];

    /* Is there already any installation with these settings? */
    NtOsInstall = FindExistingNTOSInstall(List, DiskNumber, PartitionNumber, SystemRoot);
    if (NtOsInstall)
    {
        DPRINT1("An NTOS installation with name \"%S\" already exists on disk #%d, partition #%d, in SystemRoot %S\n",
                NtOsInstall->InstallationName, NtOsInstall->DiskNumber, NtOsInstall->PartitionNumber, NtOsInstall->SystemRoot);
        return NtOsInstall;
    }

    /* None was found, so add a new one */
    NtOsInstall = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, sizeof(*NtOsInstall));
    if (!NtOsInstall)
        return NULL;

    NtOsInstall->DiskNumber = DiskNumber;
    NtOsInstall->PartitionNumber = PartitionNumber;
    StringCchCopyW(NtOsInstall->SystemRoot, ARRAYSIZE(NtOsInstall->SystemRoot), SystemRoot);
    StringCchCopyW(NtOsInstall->InstallationName, ARRAYSIZE(NtOsInstall->InstallationName), InstallationName);

    // Having the GENERIC_LIST storing the display item string plainly sucks...
    StringCchPrintfA(InstallNameA, ARRAYSIZE(InstallNameA), "%S", InstallationName);
    AppendGenericListEntry(List, InstallNameA, NtOsInstall, FALSE);

    return NtOsInstall;
}

static VOID
FindNTOSInstallations(
    IN PGENERIC_LIST List,
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber)
{
    NTSTATUS Status;
    UINT i;
    HANDLE PartitionHandle, FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PartitionRootPath;
    HANDLE SectionHandle;
    // SIZE_T ViewSize;
    PVOID ViewBase;
    WCHAR PathBuffer[MAX_PATH];
    WCHAR SystemRoot[MAX_PATH];
    WCHAR InstallNameW[MAX_PATH];

    /* Set PartitionRootPath */
    swprintf(PathBuffer,
             L"\\Device\\Harddisk%lu\\Partition%lu\\",
             DiskNumber, PartitionNumber);
    RtlInitUnicodeString(&PartitionRootPath, PathBuffer);
    DPRINT1("FindNTOSInstallations: PartitionRootPath: %wZ\n", &PartitionRootPath);

    /* Open the partition */
    InitializeObjectAttributes(&ObjectAttributes,
                               &PartitionRootPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&PartitionHandle,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open partition %wZ, Status 0x%08lx\n", &PartitionRootPath, Status);
        return;
    }

    /* Try to see whether we recognize some NT boot loaders */
    for (i = 0; i < ARRAYSIZE(NtosBootLoaders); ++i)
    {
        /* Check whether the loader executable exists */
        if (!DoesFileExist(PartitionHandle, NULL, NtosBootLoaders[i].LoaderExecutable))
        {
            /* The loader does not exist, continue with another one */
            DPRINT1("Loader executable %S does not exist, continue with another one...\n", NtosBootLoaders[i].LoaderExecutable);
            continue;
        }

        /* Check whether the loader configuration file exists */
        Status = OpenAndMapFile(PartitionHandle, NULL, NtosBootLoaders[i].LoaderConfigurationFile,
                                &FileHandle, &SectionHandle, &ViewBase);
        if (!NT_SUCCESS(Status))
        {
            /* The loader does not exist, continue with another one */
            // FIXME: Consider it might be optional??
            DPRINT1("Loader configuration file %S does not exist, continue with another one...\n", NtosBootLoaders[i].LoaderConfigurationFile);
            continue;
        }

        /* The loader configuration file exists, interpret it to find valid installations */
        // TODO!!
        DPRINT1("TODO: Analyse the OS installations inside %S !\n", NtosBootLoaders[i].LoaderConfigurationFile);

        // Here we get a SystemRootPath for each installation // FIXME!
        // FIXME: Do NOT hardcode the path!! But retrieve it from boot.ini etc...
        StringCchCopyW(SystemRoot, ARRAYSIZE(SystemRoot), L"WINDOWS\\");
        if (IsValidNTOSInstallation(PartitionHandle, SystemRoot))
        {
            DPRINT1("Found a valid NTOS installation in disk #%d, partition #%d, SystemRoot %S\n",
                    DiskNumber, PartitionNumber, SystemRoot);
            StringCchPrintfW(InstallNameW, ARRAYSIZE(InstallNameW), L"%C: \\Device\\Harddisk%lu\\Partition%lu\\%s    \"%s\"",
                             'X' /* FIXME: Partition letter */, DiskNumber, PartitionNumber, SystemRoot, L"Windows (placeholder)");
            AddNTOSInstallation(List, DiskNumber, PartitionNumber, SystemRoot, InstallNameW);
        }

        // Here we get a SystemRootPath for each installation // FIXME!
        // FIXME: Do NOT hardcode the path!! But retrieve it from boot.ini etc...
        StringCchCopyW(SystemRoot, ARRAYSIZE(SystemRoot), L"ReactOS\\");
        if (IsValidNTOSInstallation(PartitionHandle, SystemRoot))
        {
            DPRINT1("Found a valid NTOS installation in disk #%d, partition #%d, SystemRoot %S\n",
                    DiskNumber, PartitionNumber, SystemRoot);
            StringCchPrintfW(InstallNameW, ARRAYSIZE(InstallNameW), L"%C: \\Device\\Harddisk%lu\\Partition%lu\\%s    \"%s\"",
                             'X' /* FIXME: Partition letter */, DiskNumber, PartitionNumber, SystemRoot, L"ReactOS (placeholder)");
            AddNTOSInstallation(List, DiskNumber, PartitionNumber, SystemRoot, InstallNameW);
        }

        /* Finally, unmap and close the file */
        UnMapFile(SectionHandle, ViewBase);
        NtClose(FileHandle);
    }

    /* Close the partition */
    NtClose(PartitionHandle);
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

            DPRINT1("   Primary Partition #%d, index %d - Type 0x%02x, IsLogical = %s, IsPartitioned = %s, IsNew = %s, AutoCreate = %s, FormatState = %lu -- Should I check it? %s\n",
                    PartEntry->PartitionNumber, PartEntry->PartitionIndex,
                    PartEntry->PartitionType, PartEntry->LogicalPartition ? "TRUE" : "FALSE",
                    PartEntry->IsPartitioned ? "TRUE" : "FALSE",
                    PartEntry->New ? "Yes" : "No",
                    PartEntry->AutoCreate ? "Yes" : "No",
                    PartEntry->FormatState,
                    ShouldICheckThisPartition(PartEntry) ? "YES!" : "NO!");

            if (ShouldICheckThisPartition(PartEntry))
                FindNTOSInstallations(List, DiskEntry->DiskNumber, PartEntry->PartitionNumber);
        }

        /* Then, the logical partitions (present in the extended partition) */
        Entry2 = DiskEntry->LogicalPartListHead.Flink;
        while (Entry2 != &DiskEntry->LogicalPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            Entry2 = Entry2->Flink;

            DPRINT1("   Logical Partition #%d, index %d - Type 0x%02x, IsLogical = %s, IsPartitioned = %s, IsNew = %s, AutoCreate = %s, FormatState = %lu -- Should I check it? %s\n",
                    PartEntry->PartitionNumber, PartEntry->PartitionIndex,
                    PartEntry->PartitionType, PartEntry->LogicalPartition ? "TRUE" : "FALSE",
                    PartEntry->IsPartitioned ? "TRUE" : "FALSE",
                    PartEntry->New ? "Yes" : "No",
                    PartEntry->AutoCreate ? "Yes" : "No",
                    PartEntry->FormatState,
                    ShouldICheckThisPartition(PartEntry) ? "YES!" : "NO!");

            if (ShouldICheckThisPartition(PartEntry))
                FindNTOSInstallations(List, DiskEntry->DiskNumber, PartEntry->PartitionNumber);
        }
    }

    /**** Debugging: List all the collected installations ****/
    ListNTOSInstalls(List);

    return List;
}

/* EOF */
