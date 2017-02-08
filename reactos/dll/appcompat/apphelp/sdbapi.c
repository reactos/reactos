/*
 * Copyright 2011 André Hentschel
 * Copyright 2013 Mislav Blažević
 * Copyright 2015,2016 Mark Jansen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS
#include "windows.h"
#include "ntndk.h"
#include "strsafe.h"
#include "apphelp.h"
#include "sdbstringtable.h"

#include "wine/unicode.h"


static const GUID GUID_DATABASE_MSI = {0xd8ff6d16,0x6a3a,0x468a, {0x8b,0x44,0x01,0x71,0x4d,0xdc,0x49,0xea}};
static const GUID GUID_DATABASE_SHIM = {0x11111111,0x1111,0x1111, {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11}};
static const GUID GUID_DATABASE_DRIVERS = {0xf9ab2228,0x3312,0x4a73, {0xb6,0xf9,0x93,0x6d,0x70,0xe1,0x12,0xef}};

static HANDLE SdbpHeap(void);

#if SDBAPI_DEBUG_ALLOC

typedef struct SHIM_ALLOC_ENTRY
{
    PVOID Address;
    SIZE_T Size;
    int Line;
    const char* File;
    PVOID Next;
    PVOID Prev;
} SHIM_ALLOC_ENTRY, *PSHIM_ALLOC_ENTRY;


static RTL_AVL_TABLE g_SdbpAllocationTable;


static RTL_GENERIC_COMPARE_RESULTS
NTAPI ShimAllocCompareRoutine(_In_ PRTL_AVL_TABLE Table, _In_ PVOID FirstStruct, _In_ PVOID SecondStruct)
{
    PVOID First = ((PSHIM_ALLOC_ENTRY)FirstStruct)->Address;
    PVOID Second = ((PSHIM_ALLOC_ENTRY)SecondStruct)->Address;

    if (First < Second)
        return GenericLessThan;
    else if (First == Second)
        return GenericEqual;
    return GenericGreaterThan;
}

static PVOID NTAPI ShimAllocAllocateRoutine(_In_ PRTL_AVL_TABLE Table, _In_ CLONG ByteSize)
{
    return HeapAlloc(SdbpHeap(), HEAP_ZERO_MEMORY, ByteSize);
}

static VOID NTAPI ShimAllocFreeRoutine(_In_ PRTL_AVL_TABLE Table, _In_ PVOID Buffer)
{
    HeapFree(SdbpHeap(), 0, Buffer);
}

static void SdbpInsertAllocation(PVOID address, SIZE_T size, int line, const char* file)
{
    SHIM_ALLOC_ENTRY Entry = {0};

    Entry.Address = address;
    Entry.Size = size;
    Entry.Line = line;
    Entry.File = file;
    RtlInsertElementGenericTableAvl(&g_SdbpAllocationTable, &Entry, sizeof(Entry), NULL);
}

static void SdbpUpdateAllocation(PVOID address, PVOID newaddress, SIZE_T size, int line, const char* file)
{
    SHIM_ALLOC_ENTRY Lookup = {0};
    PSHIM_ALLOC_ENTRY Entry;
    Lookup.Address = address;
    Entry = RtlLookupElementGenericTableAvl(&g_SdbpAllocationTable, &Lookup);

    if (address == newaddress)
    {
        Entry->Size = size;
    }
    else
    {
        Lookup.Address = newaddress;
        Lookup.Size = size;
        Lookup.Line = line;
        Lookup.File = file;
        Lookup.Prev = address;
        RtlInsertElementGenericTableAvl(&g_SdbpAllocationTable, &Lookup, sizeof(Lookup), NULL);
        Entry->Next = newaddress;
    }
}

static void SdbpRemoveAllocation(PVOID address, int line, const char* file)
{
    char buf[512];
    SHIM_ALLOC_ENTRY Lookup = {0};
    PSHIM_ALLOC_ENTRY Entry;

    sprintf(buf, "\r\n===============\r\n%s(%d): SdbpFree called, tracing alloc:\r\n", file, line);
    OutputDebugStringA(buf);

    Lookup.Address = address;
    while (Lookup.Address)
    {
        Entry = RtlLookupElementGenericTableAvl(&g_SdbpAllocationTable, &Lookup);
        if (Entry)
        {
            Lookup = *Entry;
            RtlDeleteElementGenericTableAvl(&g_SdbpAllocationTable, Entry);

            sprintf(buf, " > %s(%d): %s%sAlloc( %d ) ==> %p\r\n", Lookup.File, Lookup.Line,
                Lookup.Next ? "Invalidated " : "", Lookup.Prev ? "Re" : "", Lookup.Size, Lookup.Address);
            OutputDebugStringA(buf);
            Lookup.Address = Lookup.Prev;
        }
        else
        {
            Lookup.Address = NULL;
        }
    }
    sprintf(buf, "\r\n===============\r\n");
    OutputDebugStringA(buf);
}

#endif

static HANDLE g_Heap;
void SdbpHeapInit(void)
{
#if SDBAPI_DEBUG_ALLOC
    RtlInitializeGenericTableAvl(&g_SdbpAllocationTable, ShimAllocCompareRoutine,
        ShimAllocAllocateRoutine, ShimAllocFreeRoutine, NULL);
#endif
    g_Heap = HeapCreate(0, 0x10000, 0);
}

void SdbpHeapDeinit(void)
{
#if SDBAPI_DEBUG_ALLOC
    if (g_SdbpAllocationTable.NumberGenericTableElements != 0)
        __debugbreak();
#endif
    HeapDestroy(g_Heap);
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
    LPVOID mem = HeapAlloc(SdbpHeap(), HEAP_ZERO_MEMORY, size);
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
    LPVOID newmem = HeapReAlloc(SdbpHeap(), HEAP_ZERO_MEMORY, mem, size);
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
    HeapFree(SdbpHeap(), 0, mem);
}

PDB WINAPI SdbpCreate(LPCWSTR path, PATH_TYPE type, BOOL write)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK io;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    PDB db;

    if (type == DOS_PATH)
    {
        if (!RtlDosPathNameToNtPathName_U(path, &str, NULL, NULL))
            return NULL;
    }
    else
        RtlInitUnicodeString(&str, path);

    /* SdbAlloc zeroes the memory. */
    db = (PDB)SdbAlloc(sizeof(DB));
    if (!db)
    {
        SHIM_ERR("Failed to allocate memory for shim database\n");
        return NULL;
    }

    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtCreateFile(&db->file, (write ? FILE_GENERIC_WRITE : FILE_GENERIC_READ )| SYNCHRONIZE,
                          &attr, &io, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
                          write ? FILE_SUPERSEDE : FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

    if (type == DOS_PATH)
        RtlFreeUnicodeString(&str);

    if (!NT_SUCCESS(Status))
    {
        SdbCloseDatabase(db);
        SHIM_ERR("Failed to create shim database file: %lx\n", Status);
        return NULL;
    }

    return db;
}

void WINAPI SdbpFlush(PDB db)
{
    IO_STATUS_BLOCK io;
    NTSTATUS Status = NtWriteFile(db->file, NULL, NULL, NULL, &io,
        db->data, db->write_iter, NULL, NULL);
    if( !NT_SUCCESS(Status))
        SHIM_WARN("failed with 0x%lx\n", Status);
}

DWORD SdbpStrlen(PCWSTR string)
{
    return lstrlenW(string);
}

DWORD SdbpStrsize(PCWSTR string)
{
    return (SdbpStrlen(string) + 1) * sizeof(WCHAR);
}

PWSTR SdbpStrDup(LPCWSTR string)
{
    PWSTR ret = SdbpAlloc(SdbpStrsize(string));
    lstrcpyW(ret, string);
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

    if(!RtlDosPathNameToNtPathName_U(path, &FileName, NULL, NULL))
    {
        RtlFreeUnicodeString(&FileName);
        return FALSE;
    }

    InitializeObjectAttributes(&ObjectAttributes, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&mapping->file, GENERIC_READ | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
    RtlFreeUnicodeString(&FileName);

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

BOOL WINAPI SdbpCheckTagIDType(PDB db, TAGID tagid, WORD type)
{
    TAG tag = SdbGetTagFromTagID(db, tagid);
    if (tag == TAG_NULL)
        return FALSE;
    return SdbpCheckTagType(tag, type);
}

PDB SdbpOpenDatabase(LPCWSTR path, PATH_TYPE type, PDWORD major, PDWORD minor)
{
    IO_STATUS_BLOCK io;
    PDB db;
    NTSTATUS Status;
    BYTE header[12];

    db = SdbpCreate(path, type, FALSE);
    if (!db)
        return NULL;

    db->size = GetFileSize(db->file, NULL);
    db->data = SdbAlloc(db->size);
    Status = NtReadFile(db->file, NULL, NULL, NULL, &io, db->data, db->size, NULL, NULL);

    if (!NT_SUCCESS(Status))
    {
        SdbCloseDatabase(db);
        SHIM_ERR("Failed to open shim database file: 0x%lx\n", Status);
        return NULL;
    }

    if (!SdbpReadData(db, &header, 0, 12))
    {
        SdbCloseDatabase(db);
        SHIM_ERR("Failed to read shim database header\n");
        return NULL;
    }

    if (memcmp(&header[8], "sdbf", 4) != 0)
    {
        SdbCloseDatabase(db);
        SHIM_ERR("Shim database header is invalid\n");
        return NULL;
    }

    *major = *(DWORD*)&header[0];
    *minor = *(DWORD*)&header[4];

    return db;
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
    PDB db;
    DWORD major, minor;

    db = SdbpOpenDatabase(path, type, &major, &minor);
    if (!db)
        return NULL;

    if (major != 2 && major != 3)
    {
        SdbCloseDatabase(db);
        SHIM_ERR("Invalid shim database version\n");
        return NULL;
    }

    db->stringtable = SdbFindFirstTag(db, TAGID_ROOT, TAG_STRINGTABLE);
    if(!SdbGetDatabaseID(db, &db->database_id))
    {
        SHIM_INFO("Failed to get the database id\n");
    }
    return db;
}

/**
 * Closes specified database and frees its memory.
 *
 * @param [in]  db  Handle to the shim database.
 */
void WINAPI SdbCloseDatabase(PDB db)
{
    if (!db)
        return;

    if (db->file)
        NtClose(db->file);
    if (db->string_buffer)
        SdbCloseDatabase(db->string_buffer);
    if (db->string_lookup)
        SdbpTableDestroy(&db->string_lookup);
    SdbFree(db->data);
    SdbFree(db);
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
    if(NT_SUCCESS(RtlStringFromGUID(Guid, &GuidString_u)))
    {
        HRESULT hr = StringCchCopyNW(GuidString, Length, GuidString_u.Buffer, GuidString_u.Length / 2);
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
    if(Guid)
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
    PDB db;

    db = SdbpOpenDatabase(database, DOS_PATH, VersionHi, VersionLo);
    if (db)
        SdbCloseDatabase(db);

    return TRUE;
}


/**
 * Find the first named child tag.
 *
 * @param [in]  database    The database.
 * @param [in]  root        The tag to start at
 * @param [in]  find        The tag type to find
 * @param [in]  nametag     The child of 'find' that contains the name
 * @param [in]  find_name   The name to find
 *
 * @return  The found tag, or TAGID_NULL on failure
 */
TAGID WINAPI SdbFindFirstNamedTag(PDB db, TAGID root, TAGID find, TAGID nametag, LPCWSTR find_name)
{
    TAGID iter;

    iter = SdbFindFirstTag(db, root, find);

    while (iter != TAGID_NULL)
    {
        TAGID tmp = SdbFindFirstTag(db, iter, nametag);
        if (tmp != TAGID_NULL)
        {
            LPCWSTR name = SdbGetStringTagPtr(db, tmp);
            if (name && !lstrcmpiW(name, find_name))
                return iter;
        }
        iter = SdbFindNextTag(db, root, iter);
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
    PDB db = hsdb->db;

    TAGID database = SdbFindFirstTag(db, TAGID_ROOT, TAG_DATABASE);
    if (database != TAGID_NULL)
    {
        TAGID layer = SdbFindFirstNamedTag(db, database, TAG_LAYER, TAG_NAME, layerName);
        if (layer != TAGID_NULL)
        {
            TAGREF tr;
            if (SdbTagIDToTagRef(hsdb, db, layer, &tr))
            {
                return tr;
            }
        }
    }
    return TAGREF_NULL;
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
