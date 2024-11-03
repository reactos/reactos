/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Sdb low level glue layer
 * COPYRIGHT:   Copyright 2011 André Hentschel
 *              Copyright 2013 Mislav Blaževic
 *              Copyright 2015-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include "ntndk.h"
#include "strsafe.h"
#include "apphelp.h"
#include "sdbstringtable.h"


static const GUID GUID_DATABASE_MSI = {0xd8ff6d16,0x6a3a,0x468a, {0x8b,0x44,0x01,0x71,0x4d,0xdc,0x49,0xea}};
static const GUID GUID_DATABASE_SHIM = {0x11111111,0x1111,0x1111, {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11}};
static const GUID GUID_DATABASE_DRIVERS = {0xf9ab2228,0x3312,0x4a73, {0xb6,0xf9,0x93,0x6d,0x70,0xe1,0x12,0xef}};

static HANDLE SdbpHeap(void);

#if SDBAPI_DEBUG_ALLOC

/* dbgheap.c */
void SdbpInsertAllocation(PVOID address, SIZE_T size, int line, const char* file);
void SdbpUpdateAllocation(PVOID address, PVOID newaddress, SIZE_T size, int line, const char* file);
void SdbpRemoveAllocation(PVOID address, int line, const char* file);
void SdbpDebugHeapInit(HANDLE privateHeapPtr);
void SdbpDebugHeapDeinit(void);

#endif

static HANDLE g_Heap;
void SdbpHeapInit(void)
{
    g_Heap = RtlCreateHeap(HEAP_GROWABLE, NULL, 0, 0x10000, NULL, NULL);
#if SDBAPI_DEBUG_ALLOC
    SdbpDebugHeapInit(g_Heap);
#endif
}

void SdbpHeapDeinit(void)
{
#if SDBAPI_DEBUG_ALLOC
    SdbpDebugHeapDeinit();
#endif
    RtlDestroyHeap(g_Heap);
}

static HANDLE SdbpHeap(void)
{
    return g_Heap;
}

LPVOID SdbpAlloc(SIZE_T size
#if SDBAPI_DEBUG_ALLOC
    , int line, const char* file
#endif
    )
{
    LPVOID mem = RtlAllocateHeap(SdbpHeap(), HEAP_ZERO_MEMORY, size);
#if SDBAPI_DEBUG_ALLOC
    SdbpInsertAllocation(mem, size, line, file);
#endif
    return mem;
}

LPVOID SdbpReAlloc(LPVOID mem, SIZE_T size, SIZE_T oldSize
#if SDBAPI_DEBUG_ALLOC
    , int line, const char* file
#endif
    )
{
    LPVOID newmem = RtlReAllocateHeap(SdbpHeap(), HEAP_ZERO_MEMORY, mem, size);
#if SDBAPI_DEBUG_ALLOC
    SdbpUpdateAllocation(mem, newmem, size, line, file);
#endif
    return newmem;
}

void SdbpFree(LPVOID mem
#if SDBAPI_DEBUG_ALLOC
    , int line, const char* file
#endif
    )
{
#if SDBAPI_DEBUG_ALLOC
    SdbpRemoveAllocation(mem, line, file);
#endif
    RtlFreeHeap(SdbpHeap(), 0, mem);
}

PDB WINAPI SdbpCreate(LPCWSTR path, PATH_TYPE type, BOOL write)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK io;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    PDB pdb;

    if (type == DOS_PATH)
    {
        if (!RtlDosPathNameToNtPathName_U(path, &str, NULL, NULL))
            return NULL;
    }
    else
    {
        RtlInitUnicodeString(&str, path);
    }

    /* SdbAlloc zeroes the memory. */
    pdb = (PDB)SdbAlloc(sizeof(DB));
    if (!pdb)
    {
        SHIM_ERR("Failed to allocate memory for shim database\n");
        return NULL;
    }

    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtCreateFile(&pdb->file, (write ? FILE_GENERIC_WRITE : FILE_GENERIC_READ )| SYNCHRONIZE,
                          &attr, &io, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
                          write ? FILE_SUPERSEDE : FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

    pdb->for_write = write;

    if (type == DOS_PATH)
        RtlFreeUnicodeString(&str);

    if (!NT_SUCCESS(Status))
    {
        SdbCloseDatabase(pdb);
        SHIM_ERR("Failed to create shim database file: %lx\n", Status);
        return NULL;
    }

    return pdb;
}

void WINAPI SdbpFlush(PDB pdb)
{
    IO_STATUS_BLOCK io;
    NTSTATUS Status;

    ASSERT(pdb->for_write);
    Status = NtWriteFile(pdb->file, NULL, NULL, NULL, &io,
        pdb->data, pdb->write_iter, NULL, NULL);
    if (!NT_SUCCESS(Status))
        SHIM_WARN("failed with 0x%lx\n", Status);
}

DWORD SdbpStrlen(PCWSTR string)
{
    return (DWORD)wcslen(string);
}

DWORD SdbpStrsize(PCWSTR string)
{
    return (SdbpStrlen(string) + 1) * sizeof(WCHAR);
}

PWSTR SdbpStrDup(LPCWSTR string)
{
    PWSTR ret = SdbAlloc(SdbpStrsize(string));
    wcscpy(ret, string);
    return ret;
}


BOOL WINAPI SdbpOpenMemMappedFile(LPCWSTR path, PMEMMAPPED mapping)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileStandard;
    UNICODE_STRING FileName;

    RtlZeroMemory(mapping, sizeof(*mapping));

    RtlInitUnicodeString(&FileName, path);

    InitializeObjectAttributes(&ObjectAttributes, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&mapping->file, GENERIC_READ | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);

    if (Status == STATUS_OBJECT_NAME_INVALID || Status == STATUS_OBJECT_PATH_SYNTAX_BAD)
    {
        if (!RtlDosPathNameToNtPathName_U(path, &FileName, NULL, NULL))
        {
            SHIM_ERR("Failed to convert %S to Nt path: 0x%lx\n", path, Status);
            return FALSE;
        }
        InitializeObjectAttributes(&ObjectAttributes, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
        Status = NtOpenFile(&mapping->file, GENERIC_READ | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
        RtlFreeUnicodeString(&FileName);
    }

    if (!NT_SUCCESS(Status))
    {
        SHIM_ERR("Failed to open file %S: 0x%lx\n", path, Status);
        return FALSE;
    }

    Status = NtCreateSection(&mapping->section, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ, 0, 0, PAGE_READONLY, SEC_COMMIT, mapping->file);
    if (!NT_SUCCESS(Status))
    {
        /* Special case */
        if (Status == STATUS_MAPPED_FILE_SIZE_ZERO)
        {
            NtClose(mapping->file);
            mapping->file = mapping->section = NULL;
            return TRUE;
        }
        SHIM_ERR("Failed to create mapping for file: 0x%lx\n", Status);
        goto err_out;
    }

    Status = NtQueryInformationFile(mapping->file, &IoStatusBlock, &FileStandard, sizeof(FileStandard), FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        SHIM_ERR("Failed to read file info for file: 0x%lx\n", Status);
        goto err_out;
    }

    mapping->mapped_size = mapping->size = FileStandard.EndOfFile.LowPart;
    Status = NtMapViewOfSection(mapping->section, NtCurrentProcess(), (PVOID*)&mapping->view, 0, 0, 0, &mapping->mapped_size, ViewUnmap, 0, PAGE_READONLY);
    if (!NT_SUCCESS(Status))
    {
        SHIM_ERR("Failed to map view of file: 0x%lx\n", Status);
        goto err_out;
    }

    return TRUE;

err_out:
    if (!mapping->view)
    {
        if (mapping->section)
            NtClose(mapping->section);
        NtClose(mapping->file);
    }
    return FALSE;
}

void WINAPI SdbpCloseMemMappedFile(PMEMMAPPED mapping)
{
    /* Prevent a VAD warning */
    if (mapping->view)
        NtUnmapViewOfSection(NtCurrentProcess(), mapping->view);
    NtClose(mapping->section);
    NtClose(mapping->file);
    RtlZeroMemory(mapping, sizeof(*mapping));
}

BOOL WINAPI SdbpCheckTagType(TAG tag, WORD type)
{
    if ((tag & TAG_TYPE_MASK) != type)
        return FALSE;
    return TRUE;
}

BOOL WINAPI SdbpCheckTagIDType(PDB pdb, TAGID tagid, WORD type)
{
    TAG tag = SdbGetTagFromTagID(pdb, tagid);
    if (tag == TAG_NULL)
        return FALSE;
    return SdbpCheckTagType(tag, type);
}

PDB SdbpOpenDatabase(LPCWSTR path, PATH_TYPE type)
{
    IO_STATUS_BLOCK io;
    FILE_STANDARD_INFORMATION fsi;
    PDB pdb;
    NTSTATUS Status;
    BYTE header[12];

    pdb = SdbpCreate(path, type, FALSE);
    if (!pdb)
        return NULL;

    Status = NtQueryInformationFile(pdb->file, &io, &fsi, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        SdbCloseDatabase(pdb);
        SHIM_ERR("Failed to get shim database size: 0x%lx\n", Status);
        return NULL;
    }

    pdb->size = fsi.EndOfFile.u.LowPart;
    pdb->data = SdbAlloc(pdb->size);
    Status = NtReadFile(pdb->file, NULL, NULL, NULL, &io, pdb->data, pdb->size, NULL, NULL);

    if (!NT_SUCCESS(Status))
    {
        SdbCloseDatabase(pdb);
        SHIM_ERR("Failed to open shim database file: 0x%lx\n", Status);
        return NULL;
    }

    if (!SdbpReadData(pdb, &header, 0, 12))
    {
        SdbCloseDatabase(pdb);
        SHIM_ERR("Failed to read shim database header\n");
        return NULL;
    }

    if (memcmp(&header[8], "sdbf", 4) != 0)
    {
        SdbCloseDatabase(pdb);
        SHIM_ERR("Shim database header is invalid\n");
        return NULL;
    }

    pdb->major = *(DWORD*)&header[0];
    pdb->minor = *(DWORD*)&header[4];

    return pdb;
}


/**
 * Opens specified shim database file.
 *
 * @param [in]  path    Path to the shim database.
 * @param [in]  type    Type of path. Either DOS_PATH or NT_PATH.
 *
 * @return  Success: Handle to the shim database, NULL otherwise.
 */
PDB WINAPI SdbOpenDatabase(LPCWSTR path, PATH_TYPE type)
{
    PDB pdb;
    TAGID root, name;

    pdb = SdbpOpenDatabase(path, type);
    if (!pdb)
        return NULL;

    if (pdb->major != 2 && pdb->major != 3)
    {
        SdbCloseDatabase(pdb);
        SHIM_ERR("Invalid shim database version\n");
        return NULL;
    }

    pdb->stringtable = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_STRINGTABLE);
    if (!SdbGetDatabaseID(pdb, &pdb->database_id))
    {
        SHIM_INFO("Failed to get the database id\n");
    }

    root = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    if (root != TAGID_NULL)
    {
        name = SdbFindFirstTag(pdb, root, TAG_NAME);
        if (name != TAGID_NULL)
        {
            pdb->database_name = SdbGetStringTagPtr(pdb, name);
        }
    }
    if (!pdb->database_name)
    {
        SHIM_INFO("Failed to get the database name\n");
    }

    return pdb;
}

/**
 * Closes specified database and frees its memory.
 *
 * @param [in]  pdb  Handle to the shim database.
 */
void WINAPI SdbCloseDatabase(PDB pdb)
{
    if (!pdb)
        return;

    if (pdb->file)
        NtClose(pdb->file);
    if (pdb->string_buffer)
        SdbCloseDatabase(pdb->string_buffer);
    if (pdb->string_lookup)
        SdbpTableDestroy(&pdb->string_lookup);
    SdbFree(pdb->data);
    SdbFree(pdb);
}

/**
 * Parses a string to retrieve a GUID.
 *
 * @param [in]  GuidString  The string to parse.
 * @param [out] Guid        The resulting GUID.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbGUIDFromString(PCWSTR GuidString, GUID *Guid)
{
    UNICODE_STRING GuidString_u;
    RtlInitUnicodeString(&GuidString_u, GuidString);
    return NT_SUCCESS(RtlGUIDFromString(&GuidString_u, Guid));
}

/**
 * Converts a GUID to a string.
 *
 * @param [in]  Guid        The GUID to convert.
 * @param [out] GuidString  The resulting string representation of Guid.
 * @param [in]  Length      The length of GuidString.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbGUIDToString(CONST GUID *Guid, PWSTR GuidString, SIZE_T Length)
{
    UNICODE_STRING GuidString_u;
    if (NT_SUCCESS(RtlStringFromGUID(Guid, &GuidString_u)))
    {
        HRESULT hr = StringCchCopyNW(GuidString, Length, GuidString_u.Buffer, GuidString_u.Length / sizeof(WCHAR));
        RtlFreeUnicodeString(&GuidString_u);
        return SUCCEEDED(hr);
    }
    return FALSE;
}

/**
 * Checks if the specified GUID is a NULL GUID
 *
 * @param [in]  Guid    The GUID to check.
 *
 * @return  TRUE if it is a NULL GUID.
 */
BOOL WINAPI SdbIsNullGUID(CONST GUID *Guid)
{
    static GUID NullGuid = { 0 };
    return !Guid || IsEqualGUID(&NullGuid, Guid);
}

/**
 * Get the GUID from one of the standard databases.
 *
 * @param [in]  Flags   The ID to retrieve the guid from. (See SDB_DATABASE_MAIN_[xxx])
 * @param [out] Guid    The resulting GUID.
 *
 * @return  TRUE if a known database ID.
 */
BOOL WINAPI SdbGetStandardDatabaseGUID(DWORD Flags, GUID* Guid)
{
    const GUID* copy_from = NULL;
    switch(Flags & HID_DATABASE_TYPE_MASK)
    {
    case SDB_DATABASE_MAIN_MSI:
        copy_from = &GUID_DATABASE_MSI;
        break;
    case SDB_DATABASE_MAIN_SHIM:
        copy_from = &GUID_DATABASE_SHIM;
        break;
    case SDB_DATABASE_MAIN_DRIVERS:
        copy_from = &GUID_DATABASE_DRIVERS;
        break;
    default:
        SHIM_ERR("Cannot obtain database guid for databases other than main\n");
        return FALSE;
    }
    if (Guid)
    {
        memcpy(Guid, copy_from, sizeof(GUID));
    }
    return TRUE;
}

/**
 * Read the database version from the specified database.
 *
 * @param [in]  database    The database.
 * @param [out] VersionHi   The first part of the version number.
 * @param [out] VersionLo   The second part of the version number.
 *
 * @return  TRUE if it succeeds or fails, FALSE if ???
 */
BOOL WINAPI SdbGetDatabaseVersion(LPCWSTR database, PDWORD VersionHi, PDWORD VersionLo)
{
    PDB pdb;

    pdb = SdbpOpenDatabase(database, DOS_PATH);
    if (pdb)
    {
        *VersionHi = pdb->major;
        *VersionLo = pdb->minor;
        SdbCloseDatabase(pdb);
    }

    return TRUE;
}

/**
 * @name SdbGetDatabaseInformation
 * Get information about the database
 *
 * @param pdb           The database
 * @param information   The returned information
 * @return TRUE on success
 */
BOOL WINAPI SdbGetDatabaseInformation(PDB pdb, PDB_INFORMATION information)
{
    if (pdb && information)
    {
        information->dwFlags = 0;
        information->dwMajor = pdb->major;
        information->dwMinor = pdb->minor;
        information->Description = pdb->database_name;
        if (!SdbIsNullGUID(&pdb->database_id))
        {
            information->dwFlags |= DB_INFO_FLAGS_VALID_GUID;
            information->Id = pdb->database_id;
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * @name SdbFreeDatabaseInformation
 * Free up resources allocated in SdbGetDatabaseInformation
 *
 * @param information   The information retrieved from SdbGetDatabaseInformation
 */
VOID WINAPI SdbFreeDatabaseInformation(PDB_INFORMATION information)
{
    // No-op
}


/**
 * Find the first named child tag.
 *
 * @param [in]  pdb         The database.
 * @param [in]  root        The tag to start at
 * @param [in]  find        The tag type to find
 * @param [in]  nametag     The child of 'find' that contains the name
 * @param [in]  find_name   The name to find
 *
 * @return  The found tag, or TAGID_NULL on failure
 */
TAGID WINAPI SdbFindFirstNamedTag(PDB pdb, TAGID root, TAGID find, TAGID nametag, LPCWSTR find_name)
{
    TAGID iter;

    iter = SdbFindFirstTag(pdb, root, find);

    while (iter != TAGID_NULL)
    {
        TAGID tmp = SdbFindFirstTag(pdb, iter, nametag);
        if (tmp != TAGID_NULL)
        {
            LPCWSTR name = SdbGetStringTagPtr(pdb, tmp);
            if (name && !_wcsicmp(name, find_name))
                return iter;
        }
        iter = SdbFindNextTag(pdb, root, iter);
    }
    return TAGID_NULL;
}


/**
 * Find a named layer in a multi-db.
 *
 * @param [in]  hsdb        The multi-database.
 * @param [in]  layerName   The named tag to find.
 *
 * @return  The layer, or TAGREF_NULL on failure
 */
TAGREF WINAPI SdbGetLayerTagRef(HSDB hsdb, LPCWSTR layerName)
{
    PDB pdb = hsdb->pdb;

    TAGID database = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    if (database != TAGID_NULL)
    {
        TAGID layer = SdbFindFirstNamedTag(pdb, database, TAG_LAYER, TAG_NAME, layerName);
        if (layer != TAGID_NULL)
        {
            TAGREF tr;
            if (SdbTagIDToTagRef(hsdb, pdb, layer, &tr))
            {
                return tr;
            }
        }
    }
    return TAGREF_NULL;
}


#ifndef REG_SZ
#define REG_SZ 1
#define REG_DWORD 4
#define REG_QWORD 11
#endif


/**
 * Retrieve a Data entry
 *
 * @param [in]  pdb                     The database.
 * @param [in]  tiExe                   The tagID to start at
 * @param [in,opt]  lpszDataName        The name of the Data entry to find, or NULL to return all.
 * @param [out,opt]  lpdwDataType       Any of REG_SZ, REG_QWORD, REG_DWORD, ...
 * @param [out]  lpBuffer               The output buffer
 * @param [in,out,opt]  lpcbBufferSize  The size of lpBuffer in bytes
 * @param [out,opt]  ptiData            The tagID of the data
 *
 * @return  ERROR_SUCCESS
 */
DWORD WINAPI SdbQueryDataExTagID(PDB pdb, TAGID tiExe, LPCWSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpcbBufferSize, TAGID *ptiData)
{
    TAGID tiData, tiValueType, tiValue;
    DWORD dwDataType, dwSizeRequired, dwInputSize;
    LPCWSTR lpStringData;
    /* Not supported yet */
    if (!lpszDataName)
        return ERROR_INVALID_PARAMETER;

    tiData = SdbFindFirstNamedTag(pdb, tiExe, TAG_DATA, TAG_NAME, lpszDataName);
    if (tiData == TAGID_NULL)
    {
        SHIM_INFO("No data tag found\n");
        return ERROR_NOT_FOUND;
    }

    if (ptiData)
        *ptiData = tiData;

    tiValueType = SdbFindFirstTag(pdb, tiData, TAG_DATA_VALUETYPE);
    if (tiValueType == TAGID_NULL)
    {
        SHIM_WARN("Data tag (0x%x) without valuetype\n", tiData);
        return ERROR_INTERNAL_DB_CORRUPTION;
    }

    dwDataType = SdbReadDWORDTag(pdb, tiValueType, 0);
    switch (dwDataType)
    {
    case REG_SZ:
        tiValue = SdbFindFirstTag(pdb, tiData, TAG_DATA_STRING);
        break;
    case REG_DWORD:
        tiValue = SdbFindFirstTag(pdb, tiData, TAG_DATA_DWORD);
        break;
    case REG_QWORD:
        tiValue = SdbFindFirstTag(pdb, tiData, TAG_DATA_QWORD);
        break;
    default:
        /* Not supported (yet) */
        SHIM_WARN("Unsupported dwDataType=0x%x\n", dwDataType);
        return ERROR_INVALID_PARAMETER;
    }

    if (lpdwDataType)
        *lpdwDataType = dwDataType;

    if (tiValue == TAGID_NULL)
    {
        SHIM_WARN("Data tag (0x%x) without data\n", tiData);
        return ERROR_INTERNAL_DB_CORRUPTION;
    }

    if (dwDataType != REG_SZ)
    {
        dwSizeRequired = SdbGetTagDataSize(pdb, tiValue);
    }
    else
    {
        lpStringData = SdbpGetString(pdb, tiValue, &dwSizeRequired);
        if (lpStringData == NULL)
        {
            return ERROR_INTERNAL_DB_CORRUPTION;
        }
    }
    if (!lpcbBufferSize)
        return ERROR_INSUFFICIENT_BUFFER;

    dwInputSize = *lpcbBufferSize;
    *lpcbBufferSize = dwSizeRequired;

    if (dwInputSize < dwSizeRequired || lpBuffer == NULL)
    {
        SHIM_WARN("dwInputSize %u not sufficient to hold %u bytes\n", dwInputSize, dwSizeRequired);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    if (dwDataType != REG_SZ)
    {
        SdbpReadData(pdb, lpBuffer, tiValue + sizeof(TAG), dwSizeRequired);
    }
    else
    {
        StringCbCopyNW(lpBuffer, dwInputSize, lpStringData, dwSizeRequired);
    }

    return ERROR_SUCCESS;
}


/**
 * Converts the specified string to an index key.
 *
 * @param [in]  str The string which will be converted.
 *
 * @return  The resulting index key
 *
 * @todo: Fix this for unicode strings.
 */
LONGLONG WINAPI SdbMakeIndexKeyFromString(LPCWSTR str)
{
    LONGLONG result = 0;
    int shift = 56;

    while (*str && shift >= 0)
    {
        WCHAR c = toupper(*(str++));

        if (c & 0xff)
        {
            result |= (((LONGLONG)(c & 0xff)) << shift);
            shift -= 8;
        }

        if (shift < 0)
            break;

        c >>= 8;

        if (c & 0xff)
        {
            result |= (((LONGLONG)(c & 0xff)) << shift);
            shift -= 8;
        }
    }

    return result;
}


/**
 * Converts specified tag into a string.
 *
 * @param [in]  tag The tag which will be converted to a string.
 *
 * @return  Success: Pointer to the string matching specified tag, or L"InvalidTag" on failure.
 *
 */
LPCWSTR WINAPI SdbTagToString(TAG tag)
{
    switch (tag)
    {
    case TAG_NULL: return L"NULL";

    /* TAG_TYPE_NULL */
    case TAG_INCLUDE: return L"INCLUDE";
    case TAG_GENERAL: return L"GENERAL";
    case TAG_MATCH_LOGIC_NOT: return L"MATCH_LOGIC_NOT";
    case TAG_APPLY_ALL_SHIMS: return L"APPLY_ALL_SHIMS";
    case TAG_USE_SERVICE_PACK_FILES: return L"USE_SERVICE_PACK_FILES";
    case TAG_MITIGATION_OS: return L"MITIGATION_OS";
    case TAG_BLOCK_UPGRADE: return L"BLOCK_UPGRADE";
    case TAG_INCLUDEEXCLUDEDLL: return L"INCLUDEEXCLUDEDLL";
    case TAG_RAC_EVENT_OFF: return L"RAC_EVENT_OFF";
    case TAG_TELEMETRY_OFF: return L"TELEMETRY_OFF";
    case TAG_SHIM_ENGINE_OFF: return L"SHIM_ENGINE_OFF";
    case TAG_LAYER_PROPAGATION_OFF: return L"LAYER_PROPAGATION_OFF";
    case TAG_REINSTALL_UPGRADE: return L"REINSTALL_UPGRADE";

    /* TAG_TYPE_WORD */
    case TAG_MATCH_MODE: return L"MATCH_MODE";
    case TAG_TAG: return L"TAG";
    case TAG_INDEX_TAG: return L"INDEX_TAG";
    case TAG_INDEX_KEY: return L"INDEX_KEY";

    /* TAG_TYPE_DWORD */
    case TAG_SIZE: return L"SIZE";
    case TAG_OFFSET: return L"OFFSET";
    case TAG_CHECKSUM: return L"CHECKSUM";
    case TAG_SHIM_TAGID: return L"SHIM_TAGID";
    case TAG_PATCH_TAGID: return L"PATCH_TAGID";
    case TAG_MODULE_TYPE: return L"MODULE_TYPE";
    case TAG_VERDATEHI: return L"VERDATEHI";
    case TAG_VERDATELO: return L"VERDATELO";
    case TAG_VERFILEOS: return L"VERFILEOS";
    case TAG_VERFILETYPE: return L"VERFILETYPE";
    case TAG_PE_CHECKSUM: return L"PE_CHECKSUM";
    case TAG_PREVOSMAJORVER: return L"PREVOSMAJORVER";
    case TAG_PREVOSMINORVER: return L"PREVOSMINORVER";
    case TAG_PREVOSPLATFORMID: return L"PREVOSPLATFORMID";
    case TAG_PREVOSBUILDNO: return L"PREVOSBUILDNO";
    case TAG_PROBLEMSEVERITY: return L"PROBLEMSEVERITY";
    case TAG_LANGID: return L"LANGID";
    case TAG_VER_LANGUAGE: return L"VER_LANGUAGE";
    case TAG_ENGINE: return L"ENGINE";
    case TAG_HTMLHELPID: return L"HTMLHELPID";
    case TAG_INDEX_FLAGS: return L"INDEX_FLAGS";
    case TAG_FLAGS: return L"FLAGS";
    case TAG_DATA_VALUETYPE: return L"DATA_VALUETYPE";
    case TAG_DATA_DWORD: return L"DATA_DWORD";
    case TAG_LAYER_TAGID: return L"LAYER_TAGID";
    case TAG_MSI_TRANSFORM_TAGID: return L"MSI_TRANSFORM_TAGID";
    case TAG_LINKER_VERSION: return L"LINKER_VERSION";
    case TAG_LINK_DATE: return L"LINK_DATE";
    case TAG_UPTO_LINK_DATE: return L"UPTO_LINK_DATE";
    case TAG_OS_SERVICE_PACK: return L"OS_SERVICE_PACK";
    case TAG_FLAG_TAGID: return L"FLAG_TAGID";
    case TAG_RUNTIME_PLATFORM: return L"RUNTIME_PLATFORM";
    case TAG_OS_SKU: return L"OS_SKU";
    case TAG_OS_PLATFORM: return L"OS_PLATFORM";
    case TAG_APP_NAME_RC_ID: return L"APP_NAME_RC_ID";
    case TAG_VENDOR_NAME_RC_ID: return L"VENDOR_NAME_RC_ID";
    case TAG_SUMMARY_MSG_RC_ID: return L"SUMMARY_MSG_RC_ID";
    case TAG_VISTA_SKU: return L"VISTA_SKU";
    case TAG_DESCRIPTION_RC_ID: return L"DESCRIPTION_RC_ID";
    case TAG_PARAMETER1_RC_ID: return L"PARAMETER1_RC_ID";
    case TAG_CONTEXT_TAGID: return L"CONTEXT_TAGID";
    case TAG_EXE_WRAPPER: return L"EXE_WRAPPER";
    case TAG_URL_ID: return L"URL_ID";
    case TAG_TAGID: return L"TAGID";

    /* TAG_TYPE_QWORD */
    case TAG_TIME: return L"TIME";
    case TAG_BIN_FILE_VERSION: return L"BIN_FILE_VERSION";
    case TAG_BIN_PRODUCT_VERSION: return L"BIN_PRODUCT_VERSION";
    case TAG_MODTIME: return L"MODTIME";
    case TAG_FLAG_MASK_KERNEL: return L"FLAG_MASK_KERNEL";
    case TAG_UPTO_BIN_PRODUCT_VERSION: return L"UPTO_BIN_PRODUCT_VERSION";
    case TAG_DATA_QWORD: return L"DATA_QWORD";
    case TAG_FLAG_MASK_USER: return L"FLAG_MASK_USER";
    case TAG_FLAGS_NTVDM1: return L"FLAGS_NTVDM1";
    case TAG_FLAGS_NTVDM2: return L"FLAGS_NTVDM2";
    case TAG_FLAGS_NTVDM3: return L"FLAGS_NTVDM3";
    case TAG_FLAG_MASK_SHELL: return L"FLAG_MASK_SHELL";
    case TAG_UPTO_BIN_FILE_VERSION: return L"UPTO_BIN_FILE_VERSION";
    case TAG_FLAG_MASK_FUSION: return L"FLAG_MASK_FUSION";
    case TAG_FLAG_PROCESSPARAM: return L"FLAG_PROCESSPARAM";
    case TAG_FLAG_LUA: return L"FLAG_LUA";
    case TAG_FLAG_INSTALL: return L"FLAG_INSTALL";

    /* TAG_TYPE_STRINGREF */
    case TAG_NAME: return L"NAME";
    case TAG_DESCRIPTION: return L"DESCRIPTION";
    case TAG_MODULE: return L"MODULE";
    case TAG_API: return L"API";
    case TAG_VENDOR: return L"VENDOR";
    case TAG_APP_NAME: return L"APP_NAME";
    case TAG_COMMAND_LINE: return L"COMMAND_LINE";
    case TAG_COMPANY_NAME: return L"COMPANY_NAME";
    case TAG_DLLFILE: return L"DLLFILE";
    case TAG_WILDCARD_NAME: return L"WILDCARD_NAME";
    case TAG_PRODUCT_NAME: return L"PRODUCT_NAME";
    case TAG_PRODUCT_VERSION: return L"PRODUCT_VERSION";
    case TAG_FILE_DESCRIPTION: return L"FILE_DESCRIPTION";
    case TAG_FILE_VERSION: return L"FILE_VERSION";
    case TAG_ORIGINAL_FILENAME: return L"ORIGINAL_FILENAME";
    case TAG_INTERNAL_NAME: return L"INTERNAL_NAME";
    case TAG_LEGAL_COPYRIGHT: return L"LEGAL_COPYRIGHT";
    case TAG_16BIT_DESCRIPTION: return L"16BIT_DESCRIPTION";
    case TAG_APPHELP_DETAILS: return L"APPHELP_DETAILS";
    case TAG_LINK_URL: return L"LINK_URL";
    case TAG_LINK_TEXT: return L"LINK_TEXT";
    case TAG_APPHELP_TITLE: return L"APPHELP_TITLE";
    case TAG_APPHELP_CONTACT: return L"APPHELP_CONTACT";
    case TAG_SXS_MANIFEST: return L"SXS_MANIFEST";
    case TAG_DATA_STRING: return L"DATA_STRING";
    case TAG_MSI_TRANSFORM_FILE: return L"MSI_TRANSFORM_FILE";
    case TAG_16BIT_MODULE_NAME: return L"16BIT_MODULE_NAME";
    case TAG_LAYER_DISPLAYNAME: return L"LAYER_DISPLAYNAME";
    case TAG_COMPILER_VERSION: return L"COMPILER_VERSION";
    case TAG_ACTION_TYPE: return L"ACTION_TYPE";
    case TAG_EXPORT_NAME: return L"EXPORT_NAME";
    case TAG_URL: return L"URL";

    /* TAG_TYPE_LIST */
    case TAG_DATABASE: return L"DATABASE";
    case TAG_LIBRARY: return L"LIBRARY";
    case TAG_INEXCLUD: return L"INEXCLUDE";
    case TAG_SHIM: return L"SHIM";
    case TAG_PATCH: return L"PATCH";
    case TAG_APP: return L"APP";
    case TAG_EXE: return L"EXE";
    case TAG_MATCHING_FILE: return L"MATCHING_FILE";
    case TAG_SHIM_REF: return L"SHIM_REF";
    case TAG_PATCH_REF: return L"PATCH_REF";
    case TAG_LAYER: return L"LAYER";
    case TAG_FILE: return L"FILE";
    case TAG_APPHELP: return L"APPHELP";
    case TAG_LINK: return L"LINK";
    case TAG_DATA: return L"DATA";
    case TAG_MSI_TRANSFORM: return L"MSI_TRANSFORM";
    case TAG_MSI_TRANSFORM_REF: return L"MSI_TRANSFORM_REF";
    case TAG_MSI_PACKAGE: return L"MSI_PACKAGE";
    case TAG_FLAG: return L"FLAG";
    case TAG_MSI_CUSTOM_ACTION: return L"MSI_CUSTOM_ACTION";
    case TAG_FLAG_REF: return L"FLAG_REF";
    case TAG_ACTION: return L"ACTION";
    case TAG_LOOKUP: return L"LOOKUP";
    case TAG_CONTEXT: return L"CONTEXT";
    case TAG_CONTEXT_REF: return L"CONTEXT_REF";
    case TAG_SPC: return L"SPC";
    case TAG_STRINGTABLE: return L"STRINGTABLE";
    case TAG_INDEXES: return L"INDEXES";
    case TAG_INDEX: return L"INDEX";

    /* TAG_TYPE_STRING */
    case TAG_STRINGTABLE_ITEM: return L"STRINGTABLE_ITEM";

    /* TAG_TYPE_BINARY */
    case TAG_PATCH_BITS: return L"PATCH_BITS";
    case TAG_FILE_BITS: return L"FILE_BITS";
    case TAG_EXE_ID: return L"EXE_ID";
    case TAG_DATA_BITS: return L"DATA_BITS";
    case TAG_MSI_PACKAGE_ID: return L"MSI_PACKAGE_ID";
    case TAG_DATABASE_ID: return L"DATABASE_ID";
    case TAG_CONTEXT_PLATFORM_ID: return L"CONTEXT_PLATFORM_ID";
    case TAG_CONTEXT_BRANCH_ID: return L"CONTEXT_BRANCH_ID";
    case TAG_FIX_ID: return L"FIX_ID";
    case TAG_APP_ID: return L"APP_ID";
    case TAG_INDEX_BITS: return L"INDEX_BITS";

        break;
    }
    return L"InvalidTag";
}
