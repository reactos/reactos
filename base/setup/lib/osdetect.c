/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/lib/osdetect.c
 * PURPOSE:         NT 5.x family (MS Windows <= 2003, and ReactOS)
 *                  operating systems detection code.
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "ntverrsrc.h"
// #include "arcname.h"
#include "filesup.h"
#include "genlist.h"
#include "inicache.h"
#include "partlist.h"
#include "arcname.h"
#include "osdetect.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

/* Language-independent Vendor strings */
static const PCWSTR KnownVendors[] = { L"ReactOS", L"Microsoft" };


/* FUNCTIONS ****************************************************************/

#if 0

BOOL IsWindowsOS(VOID)
{
    // TODO? :
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


static BOOLEAN
IsValidNTOSInstallation(
    IN HANDLE SystemRootDirectory OPTIONAL,
    IN PCWSTR SystemRoot OPTIONAL);

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

static NTSTATUS
FreeLdrEnumerateInstallations(
    IN OUT PGENERIC_LIST List,
    IN PPARTLIST PartList,
    // IN PPARTENTRY PartEntry,
    IN PCHAR FileBuffer,
    IN ULONG FileLength)
{
    NTSTATUS Status;
    PINICACHE IniCache;
    PINICACHEITERATOR Iterator;
    PINICACHESECTION IniSection, OsIniSection;
    PWCHAR SectionName, KeyData;
    UNICODE_STRING InstallName;

    HANDLE SystemRootDirectory;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PNTOS_INSTALLATION NtOsInstall;
    UNICODE_STRING SystemRootPath;
    WCHAR SystemRoot[MAX_PATH];
    WCHAR InstallNameW[MAX_PATH];

    /* Open an *existing* FreeLdr.ini configuration file */
    Status = IniCacheLoadFromMemory(&IniCache, FileBuffer, FileLength, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Get the "Operating Systems" section */
    IniSection = IniCacheGetSection(IniCache, L"Operating Systems");
    if (IniSection == NULL)
    {
        IniCacheDestroy(IniCache);
        return STATUS_UNSUCCESSFUL;
    }

    /* Enumerate all the valid installations */
    Iterator = IniCacheFindFirstValue(IniSection, &SectionName, &KeyData);
    if (!Iterator) goto Quit;
    do
    {
        // FIXME: Poor-man quotes removal (improvement over bootsup.c:UpdateFreeLoaderIni).
        if (KeyData[0] == L'"')
        {
            /* Quoted name, copy up to the closing quote */
            PWCHAR Begin = &KeyData[1];
            PWCHAR End   = wcschr(Begin, L'"');
            if (!End)
                End = Begin + wcslen(Begin);
            RtlInitEmptyUnicodeString(&InstallName, Begin, (ULONG_PTR)End - (ULONG_PTR)Begin);
            InstallName.Length = InstallName.MaximumLength;
        }
        else
        {
            /* Non-quoted name, copy everything */
            RtlInitUnicodeString(&InstallName, KeyData);
        }

        DPRINT1("Possible installation '%wZ' in OS section '%S'\n", &InstallName, SectionName);

        /* Search for an existing ReactOS entry */
        OsIniSection = IniCacheGetSection(IniCache, SectionName);
        if (!OsIniSection)
            continue;

        /* Check for supported boot type "Windows2003" */
        Status = IniCacheGetKey(OsIniSection, L"BootType", &KeyData);
        if (NT_SUCCESS(Status))
        {
            // TODO: What to do with "Windows" ; "WindowsNT40" ; "ReactOSSetup" ?
            if ((KeyData == NULL) ||
                ( (_wcsicmp(KeyData, L"Windows2003") != 0) &&
                  (_wcsicmp(KeyData, L"\"Windows2003\"") != 0) ))
            {
                /* This is not a ReactOS entry */
                continue;
            }
        }
        else
        {
            /* Certainly not a ReactOS installation */
            continue;
        }

        /* BootType is Windows2003. Now check SystemPath. */
        Status = IniCacheGetKey(OsIniSection, L"SystemPath", &KeyData);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("    A Win2k3 install '%wZ' without an ARC path?!\n", &InstallName);
            continue;
        }

        DPRINT1("    Found a candidate Win2k3 install '%wZ' with ARC path '%S'\n", &InstallName, KeyData);

        // TODO: Normalize the ARC path.

        /*
         * Check whether we already have an installation with this ARC path.
         * If this is the case, stop there.
         */
        NtOsInstall = FindExistingNTOSInstall(List, KeyData, NULL);
        if (NtOsInstall)
        {
            DPRINT1("    An NTOS installation with name \"%S\" already exists in SystemRoot '%wZ'\n",
                    NtOsInstall->InstallationName, &NtOsInstall->SystemArcPath);
            continue;
        }

        /*
         * Convert the ARC path into an NT path, from which we will deduce
         * the real disk drive & partition on which the candidate installation
         * resides, as well verifying whether it is indeed an NTOS installation.
         */
        RtlInitEmptyUnicodeString(&SystemRootPath, SystemRoot, sizeof(SystemRoot));
        if (!ArcPathToNtPath(&SystemRootPath, KeyData, PartList))
        {
            DPRINT1("ArcPathToNtPath(%S) failed, skip the installation.\n", KeyData);
            continue;
        }

        DPRINT1("ArcPathToNtPath() succeeded: '%S' --> '%wZ'\n", KeyData, &SystemRootPath);

        /*
         * Check whether we already have an installation with this NT path.
         * If this is the case, stop there.
         */
        NtOsInstall = FindExistingNTOSInstall(List, NULL /*KeyData*/, &SystemRootPath);
        if (NtOsInstall)
        {
            DPRINT1("    An NTOS installation with name \"%S\" already exists in SystemRoot '%wZ'\n",
                    NtOsInstall->InstallationName, &NtOsInstall->SystemNtPath);
            continue;
        }

        /* Set SystemRootPath */
        DPRINT1("FreeLdrEnumerateInstallations: SystemRootPath: '%wZ'\n", &SystemRootPath);

        /* Open SystemRootPath */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &SystemRootPath,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtOpenFile(&SystemRootDirectory,
                            FILE_LIST_DIRECTORY | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to open SystemRoot '%wZ', Status 0x%08lx\n", &SystemRootPath, Status);
            continue;
        }

        if (IsValidNTOSInstallation(SystemRootDirectory, NULL))
        {
            ULONG DiskNumber = 0, PartitionNumber = 0;
            PCWSTR PathComponent = NULL;
            PDISKENTRY DiskEntry = NULL;
            PPARTENTRY PartEntry = NULL;

            DPRINT1("Found a valid NTOS installation in SystemRoot ARC path '%S', NT path '%wZ'\n", KeyData, &SystemRootPath);

            /* From the NT path, compute the disk, partition and path components */
            if (NtPathToDiskPartComponents(SystemRootPath.Buffer, &DiskNumber, &PartitionNumber, &PathComponent))
            {
                DPRINT1("SystemRootPath = '%wZ' points to disk #%d, partition #%d, path '%S'\n",
                        &SystemRootPath, DiskNumber, PartitionNumber, PathComponent);

                /* Retrieve the corresponding disk and partition */
                if (!GetDiskOrPartition(PartList, DiskNumber, PartitionNumber, &DiskEntry, &PartEntry))
                    DPRINT1("GetDiskOrPartition(disk #%d, partition #%d) failed\n", DiskNumber, PartitionNumber);
            }
            else
            {
                DPRINT1("NtPathToDiskPartComponents(%wZ) failed\n", &SystemRootPath);
            }

            if (PartEntry && PartEntry->DriveLetter)
            {
                /* We have retrieved a partition that is mounted */
                StringCchPrintfW(InstallNameW, ARRAYSIZE(InstallNameW), L"%C:%s  \"%wZ\"",
                                 PartEntry->DriveLetter, PathComponent, &InstallName);
            }
            else
            {
                /* We failed somewhere, just show the NT path */
                StringCchPrintfW(InstallNameW, ARRAYSIZE(InstallNameW), L"%wZ  \"%wZ\"",
                                 &SystemRootPath, &InstallName);
            }
            AddNTOSInstallation(List, KeyData, &SystemRootPath, PathComponent,
                                DiskNumber, PartitionNumber, PartEntry,
                                InstallNameW);
        }

        NtClose(SystemRootDirectory);
    }
    while (IniCacheFindNextValue(Iterator, &SectionName, &KeyData));

    IniCacheFindClose(Iterator);

Quit:
    IniCacheDestroy(IniCache);
    return STATUS_SUCCESS;
}

static NTSTATUS
NtLdrEnumerateInstallations(
    IN OUT PGENERIC_LIST List,
    IN PPARTLIST PartList,
    // IN PPARTENTRY PartEntry,
    IN PCHAR FileBuffer,
    IN ULONG FileLength)
{
    NTSTATUS Status;
    PINICACHE IniCache;
    PINICACHEITERATOR Iterator;
    PINICACHESECTION IniSection;
    PWCHAR SectionName, KeyData;
    UNICODE_STRING InstallName;

    HANDLE SystemRootDirectory;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PNTOS_INSTALLATION NtOsInstall;
    UNICODE_STRING SystemRootPath;
    WCHAR SystemRoot[MAX_PATH];
    WCHAR InstallNameW[MAX_PATH];

    /* Open an *existing* FreeLdr.ini configuration file */
    Status = IniCacheLoadFromMemory(&IniCache, FileBuffer, FileLength, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Get the "Operating Systems" section */
    IniSection = IniCacheGetSection(IniCache, L"operating systems");
    if (IniSection == NULL)
    {
        IniCacheDestroy(IniCache);
        return STATUS_UNSUCCESSFUL;
    }

    /* Enumerate all the valid installations */
    Iterator = IniCacheFindFirstValue(IniSection, &SectionName, &KeyData);
    if (!Iterator) goto Quit;
    do
    {
        // FIXME: Poor-man quotes removal (improvement over bootsup.c:UpdateFreeLoaderIni).
        if (KeyData[0] == L'"')
        {
            /* Quoted name, copy up to the closing quote */
            PWCHAR Begin = &KeyData[1];
            PWCHAR End   = wcschr(Begin, L'"');
            if (!End)
                End = Begin + wcslen(Begin);
            RtlInitEmptyUnicodeString(&InstallName, Begin, (ULONG_PTR)End - (ULONG_PTR)Begin);
            InstallName.Length = InstallName.MaximumLength;
        }
        else
        {
            /* Non-quoted name, copy everything */
            RtlInitUnicodeString(&InstallName, KeyData);
        }

        DPRINT1("Possible installation '%wZ' with ARC path '%S'\n", &InstallName, SectionName);

        DPRINT1("    Found a Win2k3 install '%wZ' with ARC path '%S'\n", &InstallName, SectionName);

        // TODO: Normalize the ARC path.

        /*
         * Check whether we already have an installation with this ARC path.
         * If this is the case, stop there.
         */
        NtOsInstall = FindExistingNTOSInstall(List, SectionName, NULL);
        if (NtOsInstall)
        {
            DPRINT1("    An NTOS installation with name \"%S\" already exists in SystemRoot '%wZ'\n",
                    NtOsInstall->InstallationName, &NtOsInstall->SystemArcPath);
            continue;
        }

        /*
         * Convert the ARC path into an NT path, from which we will deduce
         * the real disk drive & partition on which the candidate installation
         * resides, as well verifying whether it is indeed an NTOS installation.
         */
        RtlInitEmptyUnicodeString(&SystemRootPath, SystemRoot, sizeof(SystemRoot));
        if (!ArcPathToNtPath(&SystemRootPath, SectionName, PartList))
        {
            DPRINT1("ArcPathToNtPath(%S) failed, skip the installation.\n", SectionName);
            continue;
        }

        DPRINT1("ArcPathToNtPath() succeeded: '%S' --> '%wZ'\n", SectionName, &SystemRootPath);

        /*
         * Check whether we already have an installation with this NT path.
         * If this is the case, stop there.
         */
        NtOsInstall = FindExistingNTOSInstall(List, NULL /*SectionName*/, &SystemRootPath);
        if (NtOsInstall)
        {
            DPRINT1("    An NTOS installation with name \"%S\" already exists in SystemRoot '%wZ'\n",
                    NtOsInstall->InstallationName, &NtOsInstall->SystemNtPath);
            continue;
        }

        /* Set SystemRootPath */
        DPRINT1("NtLdrEnumerateInstallations: SystemRootPath: '%wZ'\n", &SystemRootPath);

        /* Open SystemRootPath */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &SystemRootPath,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtOpenFile(&SystemRootDirectory,
                            FILE_LIST_DIRECTORY | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to open SystemRoot '%wZ', Status 0x%08lx\n", &SystemRootPath, Status);
            continue;
        }

        if (IsValidNTOSInstallation(SystemRootDirectory, NULL))
        {
            ULONG DiskNumber = 0, PartitionNumber = 0;
            PCWSTR PathComponent = NULL;
            PDISKENTRY DiskEntry = NULL;
            PPARTENTRY PartEntry = NULL;

            DPRINT1("Found a valid NTOS installation in SystemRoot ARC path '%S', NT path '%wZ'\n", SectionName, &SystemRootPath);

            /* From the NT path, compute the disk, partition and path components */
            if (NtPathToDiskPartComponents(SystemRootPath.Buffer, &DiskNumber, &PartitionNumber, &PathComponent))
            {
                DPRINT1("SystemRootPath = '%wZ' points to disk #%d, partition #%d, path '%S'\n",
                        &SystemRootPath, DiskNumber, PartitionNumber, PathComponent);

                /* Retrieve the corresponding disk and partition */
                if (!GetDiskOrPartition(PartList, DiskNumber, PartitionNumber, &DiskEntry, &PartEntry))
                    DPRINT1("GetDiskOrPartition(disk #%d, partition #%d) failed\n", DiskNumber, PartitionNumber);
            }
            else
            {
                DPRINT1("NtPathToDiskPartComponents(%wZ) failed\n", &SystemRootPath);
            }

            if (PartEntry && PartEntry->DriveLetter)
            {
                /* We have retrieved a partition that is mounted */
                StringCchPrintfW(InstallNameW, ARRAYSIZE(InstallNameW), L"%C:%s  \"%wZ\"",
                                 PartEntry->DriveLetter, PathComponent, &InstallName);
            }
            else
            {
                /* We failed somewhere, just show the NT path */
                StringCchPrintfW(InstallNameW, ARRAYSIZE(InstallNameW), L"%wZ  \"%wZ\"",
                                 &SystemRootPath, &InstallName);
            }
            AddNTOSInstallation(List, SectionName, &SystemRootPath, PathComponent,
                                DiskNumber, PartitionNumber, PartEntry,
                                InstallNameW);
        }

        NtClose(SystemRootDirectory);
    }
    while (IniCacheFindNextValue(Iterator, &SectionName, &KeyData));

    IniCacheFindClose(Iterator);

Quit:
    IniCacheDestroy(IniCache);
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
    IN PCWSTR PathName OPTIONAL,
    IN PCWSTR FileName,     // OPTIONAL
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

    Status = OpenAndMapFile(RootDirectory, PathName, FileName,
                            &FileHandle, &SectionHandle, &ViewBase, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open and map file '%S', Status 0x%08lx\n", FileName, Status);
        return FALSE; // Status;
    }

    /* Make sure it's a valid PE file */
    if (!RtlImageNtHeader(ViewBase))
    {
        DPRINT1("File '%S' does not seem to be a valid PE, bail out\n", FileName);
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
        DPRINT1("Failed to get version resource for file '%S', Status 0x%08lx\n", FileName, Status);
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
            DPRINT1("Found version vendor: \"%S\" for file '%S'\n", pvData, FileName);

            StringCbCopyNW(VendorName->Buffer, VendorName->MaximumLength,
                           pvData, BufLen * sizeof(WCHAR));
            VendorName->Length = wcslen(VendorName->Buffer) * sizeof(WCHAR);

            Success = TRUE;
        }
    }

    if (!NT_SUCCESS(Status))
        DPRINT1("No version vendor found for file '%S'\n", FileName);

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
IsValidNTOSInstallation(
    IN HANDLE SystemRootDirectory OPTIONAL,
    IN PCWSTR SystemRoot OPTIONAL)
{
    BOOLEAN Success = FALSE;
    USHORT i;
    UNICODE_STRING VendorName;
    WCHAR PathBuffer[MAX_PATH];

    /*
     * Use either the 'SystemRootDirectory' handle or the 'SystemRoot' string,
     * depending on what the user gave to us in entry.
     */
    if (SystemRootDirectory)
        SystemRoot = NULL;
    // else SystemRootDirectory == NULL and SystemRoot is what it is.

    /* If both the parameters are NULL we cannot do anything else more */
    if (!SystemRootDirectory && !SystemRoot)
        return FALSE;

    // DoesPathExist(SystemRootDirectory, SystemRoot, L"System32\\"); etc...

    /* Check for the existence of \SystemRoot\System32 */
    StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer), L"%s%s", SystemRoot ? SystemRoot : L"", L"System32\\");
    if (!DoesPathExist(SystemRootDirectory, PathBuffer))
    {
        // DPRINT1("Failed to open directory '%wZ', Status 0x%08lx\n", &FileName, Status);
        return FALSE;
    }

    /* Check for the existence of \SystemRoot\System32\drivers */
    StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer), L"%s%s", SystemRoot ? SystemRoot : L"", L"System32\\drivers\\");
    if (!DoesPathExist(SystemRootDirectory, PathBuffer))
    {
        // DPRINT1("Failed to open directory '%wZ', Status 0x%08lx\n", &FileName, Status);
        return FALSE;
    }

    /* Check for the existence of \SystemRoot\System32\config */
    StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer), L"%s%s", SystemRoot ? SystemRoot : L"", L"System32\\config\\");
    if (!DoesPathExist(SystemRootDirectory, PathBuffer))
    {
        // DPRINT1("Failed to open directory '%wZ', Status 0x%08lx\n", &FileName, Status);
        return FALSE;
    }

#if 0
    /*
     * Check for the existence of SYSTEM and SOFTWARE hives in \SystemRoot\System32\config
     * (but we don't check here whether they are actually valid).
     */
    if (!DoesFileExist(SystemRootDirectory, SystemRoot, L"System32\\config\\SYSTEM"))
    {
        // DPRINT1("Failed to open file '%wZ', Status 0x%08lx\n", &FileName, Status);
        return FALSE;
    }
    if (!DoesFileExist(SystemRootDirectory, SystemRoot, L"System32\\config\\SOFTWARE"))
    {
        // DPRINT1("Failed to open file '%wZ', Status 0x%08lx\n", &FileName, Status);
        return FALSE;
    }
#endif

    RtlInitEmptyUnicodeString(&VendorName, PathBuffer, sizeof(PathBuffer));

    /* Check for the existence of \SystemRoot\System32\ntoskrnl.exe and retrieves its vendor name */
    Success = CheckForValidPEAndVendor(SystemRootDirectory, SystemRoot, L"System32\\ntoskrnl.exe", &VendorName);
    if (!Success)
        DPRINT1("Kernel file ntoskrnl.exe is either not a PE file, or does not have any vendor?\n");

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
    Success = CheckForValidPEAndVendor(SystemRootDirectory, SystemRoot, L"System32\\ntdll.dll", &VendorName);
    if (!Success)
        DPRINT1("User-mode file ntdll.dll is either not a PE file, or does not have any vendor?\n");
    if (Success)
    {
        for (i = 0; i < ARRAYSIZE(KnownVendors); ++i)
        {
            if (!!FindSubStrI(VendorName.Buffer, KnownVendors[i]))
            {
                /* We have found a correct vendor combination */
                DPRINT1("IsValidNTOSInstallation: The user-mode file ntdll.dll is from %S\n", KnownVendors[i]);
                break;
            }
        }
    }

    return Success;
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
    HANDLE PartitionHandle, FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PartitionRootPath;
    UINT i;
    HANDLE SectionHandle;
    // SIZE_T ViewSize;
    ULONG FileSize;
    PVOID ViewBase;
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
    Status = NtOpenFile(&PartitionHandle,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open partition '%wZ', Status 0x%08lx\n", &PartitionRootPath, Status);
        return;
    }

    /* Try to see whether we recognize some NT boot loaders */
    for (i = 0; i < ARRAYSIZE(NtosBootLoaders); ++i)
    {
        /* Check whether the loader executable exists */
        if (!DoesFileExist(PartitionHandle, NULL, NtosBootLoaders[i].LoaderExecutable))
        {
            /* The loader does not exist, continue with another one */
            DPRINT1("Loader executable '%S' does not exist, continue with another one...\n", NtosBootLoaders[i].LoaderExecutable);
            continue;
        }

        /* Check whether the loader configuration file exists */
        Status = OpenAndMapFile(PartitionHandle, NULL, NtosBootLoaders[i].LoaderConfigurationFile,
                                &FileHandle, &SectionHandle, &ViewBase, &FileSize);
        if (!NT_SUCCESS(Status))
        {
            /* The loader does not exist, continue with another one */
            // FIXME: Consider it might be optional??
            DPRINT1("Loader configuration file '%S' does not exist, continue with another one...\n", NtosBootLoaders[i].LoaderConfigurationFile);
            continue;
        }

        /* The loader configuration file exists, interpret it to find valid installations */
        DPRINT1("Analyse the OS installations inside '%S' in disk #%d, partition #%d\n",
                NtosBootLoaders[i].LoaderConfigurationFile, DiskNumber, PartitionNumber);
        switch (NtosBootLoaders[i].Type)
        {
        case FreeLdr:
            Status = FreeLdrEnumerateInstallations(List, PartList, ViewBase, FileSize);
            break;

        case NtLdr:
            Status = NtLdrEnumerateInstallations(List, PartList, ViewBase, FileSize);
            break;

        default:
            DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[i].Type);
            Status = STATUS_SUCCESS;
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
