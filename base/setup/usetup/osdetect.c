/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/osdetect.c
 * PURPOSE:         NT 5.x family (MS Windows <= 2003, and ReactOS)
 *                  operating systems detection code.
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "usetup.h"

#include <ndk/ldrtypes.h>
#include <ndk/ldrfuncs.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

extern PPARTLIST PartitionList;


/* VERSION RESOURCE API ******************************************************/

/*
 * NT-oriented version resource management, adapted from dll/win32/version.
 * We only deal with 32-bit PE executables.
 */

NTSTATUS
NtGetVersionResource(
    IN PVOID BaseAddress,
    OUT PVOID* Resource,
    OUT PULONG ResourceSize OPTIONAL)
{
// #define RT_VERSION MAKEINTRESOURCE(16)  // See winuser.h
#define VS_VERSION_INFO         1       // See psdk/verrsrc.h
#define VS_FILE_INFO            RT_VERSION

    NTSTATUS Status;
    LDR_RESOURCE_INFO ResourceInfo;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    PVOID Data = NULL;
    ULONG Size = 0;

    /* Try to find the resource */
    ResourceInfo.Type = 16; // RT_VERSION;
    ResourceInfo.Name = VS_VERSION_INFO; // MAKEINTRESOURCEW(VS_VERSION_INFO);
    ResourceInfo.Language = 0; // Don't care about the language

    Status = LdrFindResource_U(BaseAddress,
                               &ResourceInfo,
                               RESOURCE_DATA_LEVEL,
                               &ResourceDataEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtGetVersionResource: Version resource not found, Status 0x%08lx\n", Status);
        return Status;
    }

    /* Access the resource */
    Status = LdrAccessResource(BaseAddress,
                               ResourceDataEntry,
                               &Data,
                               &Size);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtGetVersionResource: Cannot access Version resource, Status 0x%08lx\n", Status);
        return Status;
    }

    *Resource = Data;
    if (ResourceSize) *ResourceSize = Size;

    return STATUS_SUCCESS;
}

/* NOTE: the xxx_STRUCT16 version differs by storing strings in ANSI, not in UNICODE */
typedef struct _VS_VERSION_INFO_STRUCT32
{
    WORD  wLength;
    WORD  wValueLength;
    WORD  wType; /* 1:Text, 0:Binary */
    WCHAR szKey[1];
#if 0   /* variable length structure */
    /* DWORD aligned */
    BYTE  Value[];
    /* DWORD aligned */
    VS_VERSION_INFO_STRUCT32 Children[];
#endif
}   VS_VERSION_INFO_STRUCT32, *PVS_VERSION_INFO_STRUCT32;
typedef const VS_VERSION_INFO_STRUCT32 *PCVS_VERSION_INFO_STRUCT32;

#define DWORD_ALIGN( base, ptr ) \
    ( (ULONG_PTR)(base) + ((((ULONG_PTR)(ptr) - (ULONG_PTR)(base)) + 3) & ~3) )

#define VersionInfo32_Value( ver )  \
    DWORD_ALIGN( (ver), (ver)->szKey + wcslen((ver)->szKey) + 1 )

#define VersionInfo32_Children( ver )  \
    (PCVS_VERSION_INFO_STRUCT32)( VersionInfo32_Value( ver ) + \
                           ( ( (ver)->wValueLength * \
                               ((ver)->wType? 2 : 1) + 3 ) & ~3 ) )

#define VersionInfo32_Next( ver ) \
    (PVS_VERSION_INFO_STRUCT32)( (ULONG_PTR)ver + (((ver)->wLength + 3) & ~3) )

static PCVS_VERSION_INFO_STRUCT32
VersionInfo32_FindChild(
    IN PCVS_VERSION_INFO_STRUCT32 info,
    IN PCWSTR szKey,
    IN UINT cbKey)
{
    PCVS_VERSION_INFO_STRUCT32 child = VersionInfo32_Children(info);

    while ((ULONG_PTR)child < (ULONG_PTR)info + info->wLength)
    {
        if (!_wcsnicmp(child->szKey, szKey, cbKey) && !child->szKey[cbKey])
            return child;

        if (child->wLength == 0) return NULL;
        child = VersionInfo32_Next(child);
    }

    return NULL;
}

static NTSTATUS
VersionInfo32_QueryValue(
    IN PCVS_VERSION_INFO_STRUCT32 info,
    IN PCWSTR lpSubBlock,
    OUT PVOID* lplpBuffer,
    OUT PUINT puLen OPTIONAL,
    OUT BOOL* pbText OPTIONAL)
{
    PCWSTR lpNextSlash;

    DPRINT("lpSubBlock : (%S)\n", lpSubBlock);

    while (*lpSubBlock)
    {
        /* Find next path component */
        for (lpNextSlash = lpSubBlock; *lpNextSlash; lpNextSlash++)
        {
            if (*lpNextSlash == '\\')
                break;
        }

        /* Skip empty components */
        if (lpNextSlash == lpSubBlock)
        {
            lpSubBlock++;
            continue;
        }

        /* We have a non-empty component: search info for key */
        info = VersionInfo32_FindChild(info, lpSubBlock, lpNextSlash - lpSubBlock);
        if (!info)
        {
            if (puLen) *puLen = 0;
            return STATUS_RESOURCE_TYPE_NOT_FOUND;
        }

        /* Skip path component */
        lpSubBlock = lpNextSlash;
    }

    /* Return value */
    *lplpBuffer = (PVOID)VersionInfo32_Value(info);
    if (puLen)
        *puLen = info->wValueLength;
    if (pbText)
        *pbText = info->wType;

    return STATUS_SUCCESS;
}

NTSTATUS
NtVerQueryValue(
    IN const VOID* pBlock,
    IN PCWSTR lpSubBlock,
    OUT PVOID* lplpBuffer,
    OUT PUINT puLen)
{
    PCVS_VERSION_INFO_STRUCT32 info = pBlock;

    DPRINT("%s (%p, %S, %p, %p)\n", __FUNCTION__, pBlock, lpSubBlock, lplpBuffer, puLen);

    if (!pBlock)
        return FALSE;

    if (!lpSubBlock || !*lpSubBlock)
        lpSubBlock = L"\\";

    return VersionInfo32_QueryValue(info, lpSubBlock, lplpBuffer, puLen, NULL);
}



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

static BOOLEAN
IsRecognizedOS(
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber)
{
    BOOLEAN Success = FALSE;
    NTSTATUS Status;
    UINT i;
    HANDLE PartitionHandle, FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PartitionRootPath;
    WCHAR PathBuffer[MAX_PATH];
    UNICODE_STRING FileName;
    // WCHAR FullName[MAX_PATH];

    /* Version stuff */
    HANDLE SectionHandle;
    SIZE_T ViewSize;
    PVOID ViewBase;
    PVOID VersionBuffer = NULL; // Read-only
    PVOID pvData = NULL;
    UINT BufLen = 0;

    /* Set PartitionRootPath */
    swprintf(PathBuffer,
             L"\\Device\\Harddisk%lu\\Partition%lu\\",
             DiskNumber, PartitionNumber);
    RtlInitUnicodeString(&PartitionRootPath, PathBuffer);
    DPRINT1("IsRecognizedOS: PartitionRootPath: %wZ\n", &PartitionRootPath);

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
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open partition %wZ, Status 0x%08lx\n", &PartitionRootPath, Status);
        return FALSE;
    }

    /* Try to see whether we recognize some NT boot loaders */
    for (i = 0; i < ARRAYSIZE(NtosBootLoaders); ++i)
    {
        /* Check whether the loader executable exists */
        RtlInitUnicodeString(&FileName, NtosBootLoaders[i].LoaderExecutable);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &FileName,
                                   OBJ_CASE_INSENSITIVE,
                                   PartitionHandle,
                                   NULL);
        Status = NtOpenFile(&FileHandle,
                            GENERIC_READ | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            0,
                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
        if (!NT_SUCCESS(Status))
        {
            /* The loader does not exist, continue with another one */
            DPRINT1("Loader executable %S does not exist, continue with another one...\n", NtosBootLoaders[i].LoaderExecutable);
            continue;
        }
        NtClose(FileHandle);

        /* Check whether the loader configuration file exists */
        RtlInitUnicodeString(&FileName, NtosBootLoaders[i].LoaderConfigurationFile);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &FileName,
                                   OBJ_CASE_INSENSITIVE,
                                   PartitionHandle,
                                   NULL);
        Status = NtOpenFile(&FileHandle,
                            GENERIC_READ | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            0,
                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
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

        /* Close the file */
        NtClose(FileHandle);
    }




    /* Find a version string in \SystemRoot\System32\ntoskrnl.exe (standard name) */
    // FIXME: Do NOT hardcode the path!! But retrieve it from boot.ini etc...
    // RtlInitUnicodeString(&FileName, L"\\SystemRoot\\System32\\ntoskrnl.exe");
    RtlInitUnicodeString(&FileName, L"WINDOWS\\system32\\ntoskrnl.exe");
    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               PartitionHandle,
                               NULL);
    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open file %wZ, Status 0x%08lx\n", &FileName, Status);
        goto Quit;
    }

    /* Map the file in memory */

    /* Create the section */
    Status = NtCreateSection(&SectionHandle,
                             SECTION_MAP_READ,
                             NULL,
                             NULL,
                             PAGE_READONLY,
                             SEC_COMMIT /* | SEC_IMAGE (_NO_EXECUTE) */,
                             FileHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create a memory section for file %wZ, Status 0x%08lx\n", &FileName, Status);
        goto SkipFile;
    }

    /* Map the section */
    ViewSize = 0;
    ViewBase = NULL;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &ViewBase,
                                0, 0,
                                NULL,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READONLY);
    /* Close handle to the section */
    NtClose(SectionHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to map a view for file %wZ, Status 0x%08lx\n", &FileName, Status);
        goto SkipFile;
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

            /**/
            Success = TRUE;
            /**/
        }
        else
        {
            DPRINT1("No version vendor found for file %wZ\n", &FileName);
        }
    }

UnmapFile:
    /* Finally, unmap the file */
    NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);

SkipFile:
    /* Close the file */
    NtClose(FileHandle);

Quit:
    /* Close the partition */
    NtClose(PartitionHandle);
    return Success;
}

static BOOLEAN
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

VOID
DetectOperatingSystems(
    IN PPARTLIST List)
{
    PLIST_ENTRY Entry, Entry2;
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry = NULL;

    /* Loop each available disk ... */
    Entry = List->DiskListHead.Flink;
    while (Entry != &List->DiskListHead)
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
            {
                BOOLEAN IsRecognized = IsRecognizedOS(DiskEntry->DiskNumber, PartEntry->PartitionNumber);
                DPRINT1("%s OS in partition #%d, index %d\n",
                        IsRecognized ? "Recognized" : "Unrecognized",
                        PartEntry->PartitionNumber, PartEntry->PartitionIndex);
            }
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
            {
                BOOLEAN IsRecognized = IsRecognizedOS(DiskEntry->DiskNumber, PartEntry->PartitionNumber);
                DPRINT1("%s OS in partition #%d, index %d\n",
                        IsRecognized ? "Recognized" : "Unrecognized",
                        PartEntry->PartitionNumber, PartEntry->PartitionIndex);
            }
        }
    }
}

/* EOF */
