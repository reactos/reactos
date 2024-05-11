/*
 * PROJECT:     apphelp_apitest
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for shim-database api's
 * COPYRIGHT:   Copyright 2012 Detlef Riekenberg
 *              Copyright 2013 Mislav Blažević
 *              Copyright 2015-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <shlwapi.h>
#include <winnt.h>
#ifdef __REACTOS__
#include <ntndk.h>
#else
#include <winternl.h>
#endif

#include <winerror.h>
#include <stdio.h>
#include <initguid.h>

#include <pseh/pseh2.h>

#include "wine/test.h"

#include "apphelp_apitest.h"


#define DOS_PATH 0
#define HID_DATABASE_FULLPATH 2

#define SDB_DATABASE_MAIN_SHIM 0x80030000


#define TAGID_NULL 0x0
#define TAGID_ROOT 0x0
#define _TAGID_ROOT 12


#define TAG_TYPE_MASK 0xF000

#define TAG_TYPE_NULL 0x1000
#define TAG_TYPE_BYTE 0x2000
#define TAG_TYPE_WORD 0x3000
#define TAG_TYPE_DWORD 0x4000
#define TAG_TYPE_QWORD 0x5000
#define TAG_TYPE_STRINGREF 0x6000
#define TAG_TYPE_LIST 0x7000
#define TAG_TYPE_STRING 0x8000
#define TAG_TYPE_BINARY 0x9000
#define TAG_NULL 0x0

#define TAG_INCLUDE (0x1 | TAG_TYPE_NULL)

#define TAG_MATCH_MODE (0x1 | TAG_TYPE_WORD)

#define TAG_SIZE (0x1 | TAG_TYPE_DWORD)
#define TAG_CHECKSUM (0x3 | TAG_TYPE_DWORD)
#define TAG_MODULE_TYPE (0x6 | TAG_TYPE_DWORD)
#define TAG_VERFILEOS (0x9 | TAG_TYPE_DWORD)
#define TAG_VERFILETYPE (0xA | TAG_TYPE_DWORD)
#define TAG_PE_CHECKSUM (0xB | TAG_TYPE_DWORD)
#define TAG_PROBLEMSEVERITY (0x10 | TAG_TYPE_DWORD)
#define TAG_HTMLHELPID (0x15 | TAG_TYPE_DWORD)
#define TAG_FLAGS (0x17 | TAG_TYPE_DWORD)
#define TAG_LAYER_TAGID (0x1A | TAG_TYPE_DWORD)
#define TAG_LINKER_VERSION (0x1C | TAG_TYPE_DWORD)
#define TAG_LINK_DATE (0x1D | TAG_TYPE_DWORD)
#define TAG_UPTO_LINK_DATE (0x1E | TAG_TYPE_DWORD)
#define TAG_APP_NAME_RC_ID (0x24 | TAG_TYPE_DWORD)
#define TAG_VENDOR_NAME_RC_ID (0x25 | TAG_TYPE_DWORD)
#define TAG_SUMMARY_MSG_RC_ID (0x26 | TAG_TYPE_DWORD)
#define TAG_OS_PLATFORM (0x23 | TAG_TYPE_DWORD)

#define TAG_TIME (0x1 | TAG_TYPE_QWORD)
#define TAG_BIN_FILE_VERSION (0x2 | TAG_TYPE_QWORD)
#define TAG_BIN_PRODUCT_VERSION (0x3 | TAG_TYPE_QWORD)
#define TAG_UPTO_BIN_PRODUCT_VERSION (0x6 | TAG_TYPE_QWORD)
#define TAG_UPTO_BIN_FILE_VERSION (0xD | TAG_TYPE_QWORD)
#define TAG_FLAG_LUA (0x10 | TAG_TYPE_QWORD)

#define TAG_DATABASE (0x1 | TAG_TYPE_LIST)
#define TAG_INEXCLUD (0x3 | TAG_TYPE_LIST)
#define TAG_EXE (0x7 | TAG_TYPE_LIST)
#define TAG_MATCHING_FILE (0x8 | TAG_TYPE_LIST)
#define TAG_SHIM_REF (0x9| TAG_TYPE_LIST)
#define TAG_LAYER (0xB | TAG_TYPE_LIST)
#define TAG_APPHELP (0xD | TAG_TYPE_LIST)
#define TAG_LINK (0xE | TAG_TYPE_LIST)
#define TAG_DATA (0xF | TAG_TYPE_LIST)
#define TAG_STRINGTABLE (0x801 | TAG_TYPE_LIST)

#define TAG_STRINGTABLE_ITEM (0x801 | TAG_TYPE_STRING)

#define TAG_NAME (0x1 | TAG_TYPE_STRINGREF)
#define TAG_MODULE (0x3 | TAG_TYPE_STRINGREF)
#define TAG_VENDOR (0x5 | TAG_TYPE_STRINGREF)
#define TAG_APP_NAME (0x6 | TAG_TYPE_STRINGREF)
#define TAG_COMMAND_LINE (0x8 | TAG_TYPE_STRINGREF)
#define TAG_COMPANY_NAME (0x9 | TAG_TYPE_STRINGREF)
#define TAG_WILDCARD_NAME (0xB | TAG_TYPE_STRINGREF)
#define TAG_PRODUCT_NAME (0x10 | TAG_TYPE_STRINGREF)
#define TAG_PRODUCT_VERSION (0x11 | TAG_TYPE_STRINGREF)
#define TAG_FILE_DESCRIPTION (0x12 | TAG_TYPE_STRINGREF)
#define TAG_FILE_VERSION (0x13 | TAG_TYPE_STRINGREF)
#define TAG_ORIGINAL_FILENAME (0x14 | TAG_TYPE_STRINGREF)
#define TAG_INTERNAL_NAME (0x15 | TAG_TYPE_STRINGREF)
#define TAG_LEGAL_COPYRIGHT (0x16 | TAG_TYPE_STRINGREF)
#define TAG_APPHELP_DETAILS (0x18 | TAG_TYPE_STRINGREF)
#define TAG_LINK_URL (0x19 | TAG_TYPE_STRINGREF)
#define TAG_APPHELP_TITLE (0x1B | TAG_TYPE_STRINGREF)

#define TAG_COMPILER_VERSION (0x22 | TAG_TYPE_STRINGREF)

#define TAG_GENERAL (0x2 | TAG_TYPE_NULL)

#define TAG_EXE_ID (0x4 | TAG_TYPE_BINARY)
#define TAG_DATA_BITS (0x5 | TAG_TYPE_BINARY)
#define TAG_DATABASE_ID (0x7 | TAG_TYPE_BINARY)


typedef struct _DB_INFORMATION
{
    DWORD dwSomething;
    DWORD dwMajor;
    DWORD dwMinor;
    LPCWSTR Description;
    GUID Id;
    /* Win10+ has an extra field here */
} DB_INFORMATION, *PDB_INFORMATION;


static HMODULE hdll;
static LPCWSTR (WINAPI *pSdbTagToString)(TAG);
static PDB (WINAPI *pSdbOpenDatabase)(LPCWSTR, PATH_TYPE);
static PDB (WINAPI *pSdbCreateDatabase)(LPCWSTR, PATH_TYPE);
static BOOL (WINAPI *pSdbGetDatabaseVersion)(LPCWSTR, PDWORD, PDWORD);
static void (WINAPI *pSdbCloseDatabase)(PDB);
static void (WINAPI *pSdbCloseDatabaseWrite)(PDB);
static TAG (WINAPI *pSdbGetTagFromTagID)(PDB, TAGID);
static BOOL (WINAPI *pSdbWriteNULLTag)(PDB, TAG);
static BOOL (WINAPI *pSdbWriteWORDTag)(PDB, TAG, WORD);
static BOOL (WINAPI *pSdbWriteDWORDTag)(PDB, TAG, DWORD);
static BOOL (WINAPI *pSdbWriteQWORDTag)(PDB, TAG, QWORD);
static BOOL (WINAPI *pSdbWriteBinaryTagFromFile)(PDB, TAG, LPCWSTR);
static BOOL (WINAPI *pSdbWriteStringTag)(PDB, TAG, LPCWSTR);
static BOOL (WINAPI *pSdbWriteStringRefTag)(PDB, TAG, TAGID);
static TAGID (WINAPI *pSdbBeginWriteListTag)(PDB, TAG);
static BOOL (WINAPI *pSdbEndWriteListTag)(PDB, TAGID);
static TAGID (WINAPI *pSdbFindFirstTag)(PDB, TAGID, TAG);
static TAGID (WINAPI *pSdbFindNextTag)(PDB, TAGID, TAGID);
static TAGID (WINAPI *pSdbFindFirstNamedTag)(PDB pdb, TAGID root, TAGID find, TAGID nametag, LPCWSTR find_name);
static WORD (WINAPI *pSdbReadWORDTag)(PDB, TAGID, WORD);
static DWORD (WINAPI *pSdbReadDWORDTag)(PDB, TAGID, DWORD);
static QWORD (WINAPI *pSdbReadQWORDTag)(PDB, TAGID, QWORD);
static BOOL (WINAPI *pSdbReadBinaryTag)(PDB, TAGID, PBYTE, DWORD);
static BOOL (WINAPI *pSdbReadStringTag)(PDB, TAGID, LPWSTR, DWORD);
static DWORD (WINAPI *pSdbGetTagDataSize)(PDB, TAGID);
static PVOID (WINAPI *pSdbGetBinaryTagData)(PDB, TAGID);
static LPWSTR (WINAPI *pSdbGetStringTagPtr)(PDB, TAGID);
static TAGID (WINAPI *pSdbGetFirstChild)(PDB, TAGID);
static TAGID (WINAPI *pSdbGetNextChild)(PDB, TAGID, TAGID);
static BOOL (WINAPI *pSdbGetDatabaseID)(PDB, GUID*);
static BOOL (WINAPI *pSdbGUIDToString)(CONST GUID *, PCWSTR, SIZE_T);
static HSDB (WINAPI *pSdbInitDatabase)(DWORD, LPCWSTR);
static void (WINAPI *pSdbReleaseDatabase)(HSDB);
static BOOL (WINAPI *pSdbGetMatchingExe)(HSDB hsdb, LPCWSTR path, LPCWSTR module_name, LPCWSTR env, DWORD flags, PSDBQUERYRESULT_VISTA result);
static BOOL (WINAPI *pSdbTagRefToTagID)(HSDB hSDB, TAGREF trWhich, PDB *ppdb, TAGID *ptiWhich);
static BOOL (WINAPI *pSdbTagIDToTagRef)(HSDB hSDB, PDB pdb, TAGID tiWhich, TAGREF *ptrWhich);
static TAGREF (WINAPI *pSdbGetLayerTagRef)(HSDB hsdb, LPCWSTR layerName);
static LONGLONG (WINAPI* pSdbMakeIndexKeyFromString)(LPCWSTR);
static DWORD (WINAPI* pSdbQueryData)(HSDB hsdb, TAGREF trWhich, LPCWSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpcbBufferSize);
static DWORD (WINAPI* pSdbQueryDataEx)(HSDB hsdb, TAGREF trWhich, LPCWSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpcbBufferSize, TAGREF *ptrData);
static DWORD (WINAPI* pSdbQueryDataExTagID)(PDB pdb, TAGID tiExe, LPCWSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpcbBufferSize, TAGID *ptiData);
static BOOL (WINAPI* pSdbGetDatabaseInformation)(PDB pdb, PDB_INFORMATION information);
static VOID (WINAPI* pSdbFreeDatabaseInformation)(PDB_INFORMATION information);

DEFINE_GUID(GUID_DATABASE_TEST, 0xe39b0eb0, 0x55db, 0x450b, 0x9b, 0xd4, 0xd2, 0x0c, 0x94, 0x84, 0x26, 0x0f);
DEFINE_GUID(GUID_MAIN_DATABASE, 0x11111111, 0x1111, 0x1111, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11);


static void Write(HANDLE file, LPCVOID buffer, DWORD size)
{
    DWORD dwWritten = 0;
    WriteFile(file, buffer, size, &dwWritten, NULL);
}

static void test_GetDatabaseInformationEmpty(PDB pdb)
{
    PDB_INFORMATION pInfo;
    BOOL fResult;

    if (!pSdbGetDatabaseInformation || !pSdbFreeDatabaseInformation)
    {
        skip("GetDatabaseInformation or SdbFreeDatabaseInformation not found\n");
        return;
    }

    pInfo = (PDB_INFORMATION)malloc(sizeof(*pInfo) * 4);
    memset(pInfo, 0xDE, sizeof(*pInfo) * 2);

    fResult = pSdbGetDatabaseInformation(pdb, pInfo);
    ok(fResult, "SdbGetDatabaseInformation failed\n");
    if (fResult)
    {
        ok_int(pInfo->dwSomething, 0);
        ok(IsEqualGUID(GUID_NULL, pInfo->Id), "expected guid to be empty(%s)\n", wine_dbgstr_guid(&pInfo->Id));
        ok(pInfo->Description == NULL, "Expected pInfo->Description to be NULL, was %s\n", wine_dbgstr_w(pInfo->Description));

        /* Struct is slightly bigger on some Win10, and the DB version nr is different on all */
        if (g_WinVersion >= WINVER_WIN10)
        {
            ok(pInfo->dwMajor == 3, "Expected pInfo->dwMajor to be 3, was: %d\n", pInfo->dwMajor);
            ok(pInfo->dwMinor == 0, "Expected pInfo->dwMinor to be 0, was: %d\n", pInfo->dwMinor);

            ok(pInfo[1].dwSomething == 0 || pInfo[1].dwSomething == 0xdededede, "Something amiss: 0x%x\n", pInfo[1].dwSomething);
            ok(pInfo[1].dwMajor == 0xdededede, "Cookie2 corrupt: 0x%x\n", pInfo[1].dwMajor);
        }
        else
        {
            ok(pInfo->dwMajor == 2, "Expected pInfo->dwMajor to be 2, was: %d\n", pInfo->dwMajor);
            if (g_WinVersion >= _WIN32_WINNT_VISTA)
            {
                ok(pInfo->dwMinor == 1, "Expected pInfo->dwMinor to be 1, was: %d\n", pInfo->dwMinor);
            }
            else
            {
                SYSTEMTIME si = {0};
                GetSystemTime(&si);
                DWORD dwExpect = ((DWORD)si.wYear - 2000) * 10000 + si.wMonth * 100 + si.wDay;
                ok(pInfo->dwMinor == dwExpect, "Expected pInfo->dwMinor to be %d, was: %d\n", dwExpect, pInfo->dwMinor);
            }

            ok(pInfo[1].dwSomething == 0xdededede, "Cookie1 corrupt: 0x%x\n", pInfo[1].dwSomething);
            ok(pInfo[1].dwMajor == 0xdededede, "Cookie2 corrupt: 0x%x\n", pInfo[1].dwMajor);
        }

    }
    free(pInfo);
}


static void test_Sdb(void)
{
    static const WCHAR temp[] = L"temp";
    static const WCHAR path1[] = L"temp.sdb";
    static const WCHAR path2[] = L"temp2.bin";
    static const WCHAR tag_size_string[] = L"SIZE";
    static const WCHAR tag_flag_lua_string[] = L"FLAG_LUA";
    static const WCHAR invalid_tag[] = L"InvalidTag";
    static const TAG tags[5] = {
        TAG_SIZE, TAG_FLAG_LUA, TAG_NAME,
        TAG_STRINGTABLE, TAG_STRINGTABLE_ITEM
    };
    WCHAR buffer[6] = { 0 };
    PDB pdb;
    QWORD qword;
    DWORD dword;
    WORD word;
    BOOL ret;
    HANDLE file; /* temp file created for testing purpose */
    TAG tag;
    TAGID tagid, ptagid, stringref = 6;
    LPCWSTR string;
    PBYTE binary;

    pdb = pSdbCreateDatabase(path1, DOS_PATH);
    ok(pdb != NULL, "failed to create database\n");
    if (pdb != NULL)
    {
        ret = pSdbWriteDWORDTag(pdb, tags[0], 0xDEADBEEF);
        ok(ret, "failed to write DWORD tag\n");
        ret = pSdbWriteQWORDTag(pdb, tags[1], 0xDEADBEEFBABE);
        ok(ret, "failed to write QWORD tag\n");
        ret = pSdbWriteStringRefTag(pdb, tags[2], stringref);
        ok(ret, "failed to write stringref tag\n");
        tagid = pSdbBeginWriteListTag(pdb, tags[3]);
        ok(tagid != TAGID_NULL, "unexpected NULL tagid\n");
        ret = pSdbWriteStringTag(pdb, tags[4], temp);
        ok(ret, "failed to write string tag\n");
        ret = pSdbWriteNULLTag(pdb, TAG_GENERAL);
        ok(ret, "failed to write NULL tag\n");
        ret = pSdbWriteWORDTag(pdb, TAG_MATCH_MODE, 0xACE);
        ok(ret, "failed to write WORD tag\n");
        ret = pSdbEndWriteListTag(pdb, tagid);
        ok(ret, "failed to update list size\n");
        /* [Err ][SdbCloseDatabase    ] Failed to close the file. */
        pSdbCloseDatabaseWrite(pdb);
    }

    /* [Err ][SdbGetDatabaseID    ] Failed to get root tag */
    pdb = pSdbOpenDatabase(path1, DOS_PATH);
    ok(pdb != NULL, "unexpected NULL handle\n");

    if (pdb)
    {
        tagid = pSdbGetFirstChild(pdb, TAGID_ROOT);
        ok(tagid == _TAGID_ROOT, "unexpected tagid %u, expected %u\n", tagid, _TAGID_ROOT);

        tag = pSdbGetTagFromTagID(pdb, tagid);
        ok(tag == TAG_SIZE, "unexpected tag 0x%x, expected 0x%x\n", tag, TAG_SIZE);

        string = pSdbTagToString(tag);
        ok(lstrcmpW(string, tag_size_string) == 0, "unexpected string %s, expected %s\n",
            wine_dbgstr_w(string), wine_dbgstr_w(tag_size_string));

        dword = pSdbReadDWORDTag(pdb, tagid, 0);
        ok(dword == 0xDEADBEEF, "unexpected value %u, expected 0xDEADBEEF\n", dword);

        tagid = pSdbGetNextChild(pdb, TAGID_ROOT, tagid);
        ok(tagid == _TAGID_ROOT + sizeof(TAG) + sizeof(DWORD), "unexpected tagid %u, expected %u\n",
            tagid, _TAGID_ROOT + sizeof(TAG) + sizeof(DWORD));

        tag = pSdbGetTagFromTagID(pdb, tagid);
        ok(tag == TAG_FLAG_LUA, "unexpected tag 0x%x, expected 0x%x\n", tag, TAG_FLAG_LUA);

        string = pSdbTagToString(tag);
        if (g_WinVersion >= WINVER_VISTA)
        {
            ok(lstrcmpW(string, tag_flag_lua_string) == 0, "unexpected string %s, expected %s\n",
                wine_dbgstr_w(string), wine_dbgstr_w(tag_flag_lua_string));
        }
        else
        {
            ok(lstrcmpW(string, invalid_tag) == 0, "unexpected string %s, expected %s\n",
                wine_dbgstr_w(string), wine_dbgstr_w(invalid_tag));
        }

        qword = pSdbReadQWORDTag(pdb, tagid, 0);
        ok(qword == 0xDEADBEEFBABE, "unexpected value 0x%I64x, expected 0xDEADBEEFBABE\n", qword);

        tagid = pSdbGetNextChild(pdb, TAGID_ROOT, tagid);
        string = pSdbGetStringTagPtr(pdb, tagid);
        ok(string && (lstrcmpW(string, temp) == 0), "unexpected string %s, expected %s\n",
            wine_dbgstr_w(string), wine_dbgstr_w(temp));

        ptagid = pSdbGetNextChild(pdb, TAGID_ROOT, tagid);
        tagid = pSdbGetFirstChild(pdb, ptagid);

        string = pSdbGetStringTagPtr(pdb, tagid);
        ok(string && (lstrcmpW(string, temp) == 0), "unexpected string %s, expected %s\n",
            wine_dbgstr_w(string), wine_dbgstr_w(temp));

        ok(pSdbReadStringTag(pdb, tagid, buffer, 6), "failed to write string to buffer\n");
        /* [Err ][SdbpReadTagData     ] Buffer too small. Avail: 6, Need: 10. */
        ok(!pSdbReadStringTag(pdb, tagid, buffer, 3), "string was written to buffer, but failure was expected");
        ok(pSdbGetTagDataSize(pdb, tagid) == 5 * sizeof(WCHAR), "string has unexpected size\n");

        tagid = pSdbGetNextChild(pdb, ptagid, tagid);
        tag = pSdbGetTagFromTagID(pdb, tagid);
        ok(tag == TAG_GENERAL, "unexpected tag 0x%x, expected 0x%x\n", tag, TAG_GENERAL);
        ok(pSdbGetTagDataSize(pdb, tagid) == 0, "null tag with size > 0\n");

        tagid = pSdbGetNextChild(pdb, ptagid, tagid);
        word = pSdbReadWORDTag(pdb, tagid, 0);
        ok(word == 0xACE, "unexpected value 0x%x, expected 0x%x\n", word, 0xACE);

        test_GetDatabaseInformationEmpty(pdb);

        pSdbCloseDatabase(pdb);
    }
    DeleteFileW(path1);

    file = CreateFileW(path2, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open file\n");
    Write(file, &qword, 8);
    CloseHandle(file);

    pdb = pSdbCreateDatabase(path1, DOS_PATH);
    ok(pdb != NULL, "unexpected NULL handle\n");

    if (pdb)
    {
        ret = pSdbWriteBinaryTagFromFile(pdb, TAG_DATA_BITS, path2);
        ok(ret, "failed to write tag from binary file\n");
        pSdbCloseDatabaseWrite(pdb);     /* [Err ][SdbCloseDatabase    ] Failed to close the file. */
        DeleteFileW(path2);

        /* FIXME: doesnt work on win10?! */
        pdb = pSdbOpenDatabase(path1, DOS_PATH);
        if (g_WinVersion < WINVER_WIN10)
        {
            /* ERROR,SdbOpenDatabaseEx,845,Failed to open SDB - File size too large or small. */
            ok(pdb != NULL, "unexpected NULL handle\n");
        }
        if (pdb)
        {
            binary = (PBYTE)pSdbGetBinaryTagData(pdb, _TAGID_ROOT);
            ok(memcmp(binary, &qword, 8) == 0, "binary data is corrupt\n");
            ret = pSdbReadBinaryTag(pdb, _TAGID_ROOT, (PBYTE)buffer, 12);
            ok(ret, "failed to read binary tag\n");
            ok(memcmp(buffer, &qword, 8) == 0, "binary data is corrupt\n");
            pSdbCloseDatabase(pdb);
        }
    }
    DeleteFileW(path1);
}

/*
 - Show that a stringtable is automatically generated,
 - Show that entries in the stringtable are re-used,
 - validate multiple lists (for the length)
 */
static void test_write_ex(void)
{
    WCHAR path1[] = {'t','e','s','t','.','s','d','b',0};
    WCHAR test1[] = {'T','E','S','T',0};
    WCHAR test2[] = {'t','e','s','t',0};
    PDB pdb;
    TAGID tagdb, tagstr;
    TAG tag;
    DWORD size;
    BOOL ret;
    LPWSTR ptr;

    /* Write a small database */
    pdb = pSdbCreateDatabase(path1, DOS_PATH);
    ok(pdb != NULL, "Expected a valid database\n");
    if (!pdb)
        return;
    tagdb = pSdbBeginWriteListTag(pdb, TAG_DATABASE);
    ok(tagdb == 12, "Expected tag to be 12, was %u\n", tagdb);
    ret = pSdbWriteStringTag(pdb, TAG_NAME, test1);
    ret = pSdbWriteStringTag(pdb, TAG_NAME, test2);
    ok(ret, "Expected SdbWriteStringTag to succeed\n");
    ret = pSdbEndWriteListTag(pdb, tagdb);
    ok(ret, "Expected SdbEndWriteListTag to succeed\n");

    tagdb = pSdbBeginWriteListTag(pdb, TAG_DATABASE);
    ok(tagdb == 30, "Expected tag to be 24, was %u\n", tagdb);
    ret = pSdbWriteStringTag(pdb, TAG_NAME, test1);
    ret = pSdbWriteStringTag(pdb, TAG_NAME, test2);
    ok(ret, "Expected SdbWriteStringTag to succeed\n");
    ret = pSdbEndWriteListTag(pdb, tagdb);
    ok(ret, "Expected SdbEndWriteListTag to succeed\n");

    pSdbCloseDatabaseWrite(pdb);

    /* Now validate it's contents */
    pdb = pSdbOpenDatabase(path1, DOS_PATH);
    ok(pdb != NULL, "Expected a valid database\n");
    if (!pdb)
        return;

    tagdb = pSdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    ok(tagdb == 12, "Expected tag to be 12, was %u\n", tagdb);
    size = pSdbGetTagDataSize(pdb, tagdb);
    ok(size == 12, "Expected size to be 12, was %u\n", size);

    tagstr = pSdbFindFirstTag(pdb, tagdb, TAG_NAME);
    ok(tagstr == 18, "Expected string tag to be 18, was %u\n", tagstr);
    tag = pSdbGetTagFromTagID(pdb, tagstr);
    ok(tag == TAG_NAME, "Expected tag to be TAG_NAME, was 0x%x\n", (DWORD)tag);
    size = pSdbGetTagDataSize(pdb, tagstr);
    ok(size == 4, "Expected size to be 4, was 0x%x\n", size);

    tagstr = pSdbFindNextTag(pdb, tagdb, tagstr);
    ok(tagstr == 24, "Expected string tag to be 24, was %u\n", tagstr);
    tag = pSdbGetTagFromTagID(pdb, tagstr);
    ok(tag == TAG_NAME, "Expected tag to be TAG_NAME, was 0x%x\n", (DWORD)tag);
    size = pSdbGetTagDataSize(pdb, tagstr);
    ok(size == 4, "Expected size to be 4, was 0x%x\n", size);

    tagdb = pSdbFindNextTag(pdb, TAGID_ROOT, tagdb);
    ok(tagdb == 30, "Expected tag to be 30, was %u\n", tagdb);
    size = pSdbGetTagDataSize(pdb, tagdb);
    ok(size == 12, "Expected size to be 12, was %u\n", size);

    tagstr = pSdbFindFirstTag(pdb, tagdb, TAG_NAME);
    ok(tagstr == 36, "Expected string tag to be 36, was %u\n", tagstr);
    tag = pSdbGetTagFromTagID(pdb, tagstr);
    ok(tag == TAG_NAME, "Expected tag to be TAG_NAME, was 0x%x\n", (DWORD)tag);
    size = pSdbGetTagDataSize(pdb, tagstr);
    ok(size == 4, "Expected size to be 4, was %u\n", size);

    tagstr = pSdbFindNextTag(pdb, tagdb, tagstr);
    ok(tagstr == 42, "Expected string tag to be 42, was %u\n", tagstr);
    tag = pSdbGetTagFromTagID(pdb, tagstr);
    ok(tag == TAG_NAME, "Expected tag to be TAG_NAME, was 0x%x\n", (DWORD)tag);
    size = pSdbGetTagDataSize(pdb, tagstr);
    ok(size == 4, "Expected size to be 4, was 0x%x\n", size);

    tagdb = pSdbFindFirstTag(pdb, TAGID_ROOT, TAG_STRINGTABLE);
    ok(tagdb == 48, "Expected tag to be 48, was %u\n", tagdb);
    size = pSdbGetTagDataSize(pdb, tagdb);
    ok(size == 32, "Expected size to be 32, was %u\n", size);

    tagstr = pSdbGetFirstChild(pdb, tagdb);
    ok(tagstr == 54, "Expected string tag to be 54, was %u\n", tagstr);
    tag = pSdbGetTagFromTagID(pdb, tagstr);
    ok(tag == TAG_STRINGTABLE_ITEM, "Expected tag to be TAG_STRINGTABLE_ITEM, was 0x%x\n", (DWORD)tag);
    size = pSdbGetTagDataSize(pdb, tagstr);
    ok(size == 10, "Expected size to be 10, was %u\n", size);
    ptr = pSdbGetStringTagPtr(pdb, tagstr);
    ok(ptr != NULL, "Expected a valid pointer\n");
    if (ptr)
        ok(!wcscmp(ptr, test1), "Expected ptr to be %s, was %s\n", wine_dbgstr_w(test1), wine_dbgstr_w(ptr));

    tagstr = pSdbGetNextChild(pdb, tagdb, tagstr);
    ok(tagstr == 70, "Expected string tag to be 70, was %u\n", tagstr);
    tag = pSdbGetTagFromTagID(pdb, tagstr);
    ok(tag == TAG_STRINGTABLE_ITEM, "Expected tag to be TAG_STRINGTABLE_ITEM, was 0x%x\n", (DWORD)tag);
    size = pSdbGetTagDataSize(pdb, tagstr);
    ok(size == 10, "Expected size to be 10, was %u\n", size);
    ptr = pSdbGetStringTagPtr(pdb, tagstr);
    ok(ptr != NULL, "Expected a valid pointer\n");
    if (ptr)
        ok(!wcscmp(ptr, test2), "Expected ptr to be %s, was %s\n", wine_dbgstr_w(test2), wine_dbgstr_w(ptr));

    pSdbCloseDatabase(pdb);
}


static void write_db_strings(const WCHAR* name, const WCHAR* data[], size_t count)
{
    PDB pdb;
    size_t n;
    BOOL ret;

    pdb = pSdbCreateDatabase(name, DOS_PATH);
    ok(pdb != NULL, "Failed to create db for case %u\n", count);
    for (n = 0; n < count; ++n)
    {
        ret = pSdbWriteStringTag(pdb, TAG_NAME, data[n]);
        ok(ret, "Failed to write string %u/%u\n", n, count);
    }
    pSdbCloseDatabaseWrite(pdb);
}

static void test_stringtable()
{
    static const WCHAR path1[] = {'t','e','s','t','.','s','d','b',0};
    static const WCHAR test1[] = {'t','e','s','t','1',0};
    static const WCHAR test2[] = {'T','e','s','t','1',0};
    static const WCHAR test3[] = {'T','E','s','t','1',0};
    static const WCHAR test4[] = {'T','E','S','T','1',0};
    static const WCHAR test5[] = {'T','E','S','T','2',0};
    static const WCHAR lipsum[] = {'L','o','r','e','m',' ','i','p','s','u','m',' ','d','o','l','o','r',' ','s','i','t',' ','a','m','e','t',',',' ','c','o','n','s','e','c','t','e','t','u','r',' ','a','d','i','p','i','s','c','i','n','g',' ','e','l','i','t','.',' ','N','u','l','l','a',' ','a','n','t','e',' ','r','i','s','u','s',',',' ','m','a','l','e','s','u','a','d','a',' ','s','e','d',' ','i','a','c','u','l','i','s',' ','l','u','c','t','u','s',',',' ','o','r','n','a','r','e',' ','p','u','l','v','i','n','a','r',' ','v','e','l','i','t','.',' ','L','o','r','e','m',' ','i','p','s','u','m',' ','d','o','l','o','r',' ','s','i','t',' ','a','m','e','t',',',' ','c','o','n','s','e','c','t','e','t','u','r',' ','a','d','i','p','i','s','c','i','n','g',' ','e','l','i','t','.',' ','I','n','t','e','g','e','r',' ','q','u','i','s',' ','f','e','l','i','s',' ','u','t',' ','l','e','o',' ','e','l','e','i','f','e','n','d',' ','u','l','t','r','i','c','e','s',' ','f','i','n','i','b','u','s',' ','e','u',' ','d','o','l','o','r','.',' ','I','n',' ','b','i','b','e','n','d','u','m',',',' ','e','r','o','s',' ','e','u',' ','f','a','u','c','i','b','u','s',' ','c','o','n','s','e','q','u','a','t',',',' ','n','i','s','i',' ','m','a','g','n','a',' ','v','e','n','e','n','a','t','i','s',' ','j','u','s','t','o',',',' ','a','t',' ','t','r','i','s','t','i','q','u','e',' ','m','e','t','u','s',' ','d','o','l','o','r',' ','u','t',' ','r','i','s','u','s','.',' ','N','u','n','c',' ','e','u',' ','o','d','i','o',' ','d','i','g','n','i','s','s','i','m',',',' ','o','r','n','a','r','e',' ','a','n','t','e',' ','g','r','a','v','i','d','a',',',' ','l','o','b','o','r','t','i','s',' ','e','r','o','s','.',' ','C','r','a','s',' ','s','e','m',' ','e','x',',',' ','c','o','n','s','e','c','t','e','t','u','r',' ','p','u','l','v','i','n','a','r',' ','t','i','n','c','i','d','u','n','t',' ','e','u',',',' ','c','o','n','g','u','e',' ','a',' ','e','r','o','s','.',' ','C','u','r','a','b','i','t','u','r',' ','e','r','o','s',' ','e','r','a','t',',',' ','p','e','l','l','e','n','t','e','s','q','u','e',' ','e','t',' ','n','i','b','h',' ','q','u','i','s',',',' ','i','n','t','e','r','d','u','m',' ','t','e','m','p','o','r',' ','o','d','i','o','.',' ','E','t','i','a','m',' ','s','a','p','i','e','n',' ','s','a','p','i','e','n',',',' ','a','l','i','q','u','a','m',' ','u','t',' ','a','l','i','q','u','a','m',' ','a','t',',',' ','s','a','g','i','t','t','i','s',' ','e','u',' ','m','a','g','n','a','.',' ','M','a','e','c','e','n','a','s',' ','m','a','g','n','a',' ','m','a','g','n','a',',',' ','s','u','s','c','i','p','i','t',' ','u','t',' ','l','o','r','e','m',' ','u','t',',',' ','v','a','r','i','u','s',' ','p','r','e','t','i','u','m',' ','f','e','l','i','s','.',' ','I','n','t','e','g','e','r',' ','t','i','n','c','i','d','u','n','t',',',' ','m','e','t','u','s',' ','v','e','l',' ','s','o','l','l','i','c','i','t','u','d','i','n',' ','f','i','n','i','b','u','s',',',' ','f','e','l','i','s',' ','e','r','a','t',' ','m','o','l','e','s','t','i','e',' ','u','r','n','a',',',' ','a',' ','c','o','n','d','i','m','e','n','t','u','m',' ','a','u','g','u','e',' ','a','r','c','u',' ','v','i','t','a','e',' ','r','i','s','u','s','.',' ','E','t','i','a','m',' ','i','d',' ','s','a','g','i','t','t','i','s',' ','q','u','a','m','.',' ','M','o','r','b','i',' ','a',' ','u','l','t','r','i','c','i','e','s',' ','n','u','n','c','.',' ','P','h','a','s','e','l','l','u','s',' ','e','r','o','s',' ','r','i','s','u','s',',',' ','c','u','r','s','u','s',' ','u','l','l','a','m','c','o','r','p','e','r',' ','m','a','s','s','a',' ','s','e','d',',',' ','d','i','g','n','i','s','s','i','m',' ','c','o','n','s','e','q','u','a','t',' ','l','i','g','u','l','a','.',' ','A','l','i','q','u','a','m',' ','t','u','r','p','i','s',' ','a','r','c','u',',',' ','a','c','c','u','m','s','a','n',' ','q','u','i','s',' ','s','a','p','i','e','n',' ','v','i','t','a','e',',',' ','l','a','c','i','n','i','a',' ','e','u','i','s','m','o','d',' ','n','i','s','l','.',' ','M','a','u','r','i','s',' ','i','d',' ','f','e','l','i','s',' ','s','e','m','.',0};
    /* Last char changed from '.' to '!' */
    static const WCHAR lipsum2[] = {'L','o','r','e','m',' ','i','p','s','u','m',' ','d','o','l','o','r',' ','s','i','t',' ','a','m','e','t',',',' ','c','o','n','s','e','c','t','e','t','u','r',' ','a','d','i','p','i','s','c','i','n','g',' ','e','l','i','t','.',' ','N','u','l','l','a',' ','a','n','t','e',' ','r','i','s','u','s',',',' ','m','a','l','e','s','u','a','d','a',' ','s','e','d',' ','i','a','c','u','l','i','s',' ','l','u','c','t','u','s',',',' ','o','r','n','a','r','e',' ','p','u','l','v','i','n','a','r',' ','v','e','l','i','t','.',' ','L','o','r','e','m',' ','i','p','s','u','m',' ','d','o','l','o','r',' ','s','i','t',' ','a','m','e','t',',',' ','c','o','n','s','e','c','t','e','t','u','r',' ','a','d','i','p','i','s','c','i','n','g',' ','e','l','i','t','.',' ','I','n','t','e','g','e','r',' ','q','u','i','s',' ','f','e','l','i','s',' ','u','t',' ','l','e','o',' ','e','l','e','i','f','e','n','d',' ','u','l','t','r','i','c','e','s',' ','f','i','n','i','b','u','s',' ','e','u',' ','d','o','l','o','r','.',' ','I','n',' ','b','i','b','e','n','d','u','m',',',' ','e','r','o','s',' ','e','u',' ','f','a','u','c','i','b','u','s',' ','c','o','n','s','e','q','u','a','t',',',' ','n','i','s','i',' ','m','a','g','n','a',' ','v','e','n','e','n','a','t','i','s',' ','j','u','s','t','o',',',' ','a','t',' ','t','r','i','s','t','i','q','u','e',' ','m','e','t','u','s',' ','d','o','l','o','r',' ','u','t',' ','r','i','s','u','s','.',' ','N','u','n','c',' ','e','u',' ','o','d','i','o',' ','d','i','g','n','i','s','s','i','m',',',' ','o','r','n','a','r','e',' ','a','n','t','e',' ','g','r','a','v','i','d','a',',',' ','l','o','b','o','r','t','i','s',' ','e','r','o','s','.',' ','C','r','a','s',' ','s','e','m',' ','e','x',',',' ','c','o','n','s','e','c','t','e','t','u','r',' ','p','u','l','v','i','n','a','r',' ','t','i','n','c','i','d','u','n','t',' ','e','u',',',' ','c','o','n','g','u','e',' ','a',' ','e','r','o','s','.',' ','C','u','r','a','b','i','t','u','r',' ','e','r','o','s',' ','e','r','a','t',',',' ','p','e','l','l','e','n','t','e','s','q','u','e',' ','e','t',' ','n','i','b','h',' ','q','u','i','s',',',' ','i','n','t','e','r','d','u','m',' ','t','e','m','p','o','r',' ','o','d','i','o','.',' ','E','t','i','a','m',' ','s','a','p','i','e','n',' ','s','a','p','i','e','n',',',' ','a','l','i','q','u','a','m',' ','u','t',' ','a','l','i','q','u','a','m',' ','a','t',',',' ','s','a','g','i','t','t','i','s',' ','e','u',' ','m','a','g','n','a','.',' ','M','a','e','c','e','n','a','s',' ','m','a','g','n','a',' ','m','a','g','n','a',',',' ','s','u','s','c','i','p','i','t',' ','u','t',' ','l','o','r','e','m',' ','u','t',',',' ','v','a','r','i','u','s',' ','p','r','e','t','i','u','m',' ','f','e','l','i','s','.',' ','I','n','t','e','g','e','r',' ','t','i','n','c','i','d','u','n','t',',',' ','m','e','t','u','s',' ','v','e','l',' ','s','o','l','l','i','c','i','t','u','d','i','n',' ','f','i','n','i','b','u','s',',',' ','f','e','l','i','s',' ','e','r','a','t',' ','m','o','l','e','s','t','i','e',' ','u','r','n','a',',',' ','a',' ','c','o','n','d','i','m','e','n','t','u','m',' ','a','u','g','u','e',' ','a','r','c','u',' ','v','i','t','a','e',' ','r','i','s','u','s','.',' ','E','t','i','a','m',' ','i','d',' ','s','a','g','i','t','t','i','s',' ','q','u','a','m','.',' ','M','o','r','b','i',' ','a',' ','u','l','t','r','i','c','i','e','s',' ','n','u','n','c','.',' ','P','h','a','s','e','l','l','u','s',' ','e','r','o','s',' ','r','i','s','u','s',',',' ','c','u','r','s','u','s',' ','u','l','l','a','m','c','o','r','p','e','r',' ','m','a','s','s','a',' ','s','e','d',',',' ','d','i','g','n','i','s','s','i','m',' ','c','o','n','s','e','q','u','a','t',' ','l','i','g','u','l','a','.',' ','A','l','i','q','u','a','m',' ','t','u','r','p','i','s',' ','a','r','c','u',',',' ','a','c','c','u','m','s','a','n',' ','q','u','i','s',' ','s','a','p','i','e','n',' ','v','i','t','a','e',',',' ','l','a','c','i','n','i','a',' ','e','u','i','s','m','o','d',' ','n','i','s','l','.',' ','M','a','u','r','i','s',' ','i','d',' ','f','e','l','i','s',' ','s','e','m','!',0};
    static const WCHAR empty[] = {0};
    static const WCHAR* all[] = { test1, test2, test3, test4, test5, lipsum, lipsum2, empty };
    static const TAGID expected_str[] = { 0xc, 0x12, 0x18, 0x1e, 0x24, 0x2a, 0x30, 0x36 };
    static const TAGID expected_tab[] = { 6, 0x18, 0x2a, 0x3c, 0x4e, 0x60, 0x846, 0x102c };
    DWORD n, j;

    for (n = 0; n < (sizeof(all) / sizeof(all[0])); ++n)
    {
        PDB pdb;
        TAGID tagstr, table, expected_table;

        write_db_strings(path1, all, n+1);

        pdb = pSdbOpenDatabase(path1, DOS_PATH);
        ok(pdb != NULL, "Expected a valid database\n");
        if (!pdb)
        {
            DeleteFileW(path1);
            continue;
        }
        tagstr = pSdbFindFirstTag(pdb, TAGID_ROOT, TAG_NAME);
        for (j = 0; j <= n; ++j)
        {
            ok(tagstr == expected_str[j], "Expected tagstr to be 0x%x, was 0x%x for %u/%u\n", expected_str[j], tagstr, j, n);
            if (tagstr)
            {
                LPWSTR data;
                DWORD size;
                TAG tag = pSdbGetTagFromTagID(pdb, tagstr);
                ok(tag == TAG_NAME, "Expected tag to be TAG_NAME, was 0x%x for %u/%u\n", tag, j, n);
                size = pSdbGetTagDataSize(pdb, tagstr);
                ok(size == 4, "Expected datasize to be 4, was %u for %u/%u\n", size, j, n);
                data = pSdbGetStringTagPtr(pdb, tagstr);
                ok(data && !_wcsicmp(data, all[j]), "Expected data to be %s was %s for %u/%u\n", wine_dbgstr_w(all[j]), wine_dbgstr_w(data), j, n);
            }
            tagstr = pSdbFindNextTag(pdb, TAGID_ROOT, tagstr);
        }
        ok(tagstr == TAGID_NULL, "Expected to be at the end for %u\n", n);


        table = pSdbFindFirstTag(pdb, TAGID_ROOT, TAG_STRINGTABLE);
        expected_table = 0xc + (n+1)*6;
        ok(table == expected_table, "Expected to find a stringtable at 0x%x instead of 0x%x for %u\n", expected_table, table, n);
        if (table)
        {
            tagstr = pSdbFindFirstTag(pdb, table, TAG_STRINGTABLE_ITEM);
            for (j = 0; j <= n; ++j)
            {
                ok(tagstr == (expected_tab[j] + expected_table), "Expected tagstr to be 0x%x, was 0x%x for %u/%u\n", (expected_tab[j] + expected_table), tagstr, j, n);
                if (tagstr)
                {
                    LPWSTR data;
                    DWORD size, expected_size;
                    TAG tag = pSdbGetTagFromTagID(pdb, tagstr);
                    ok(tag == TAG_STRINGTABLE_ITEM, "Expected tag to be TAG_NAME, was 0x%x for %u/%u\n", tag, j, n);
                    size = pSdbGetTagDataSize(pdb, tagstr);
                    expected_size = (lstrlenW(all[j])+1) * 2;
                    ok(size == expected_size, "Expected datasize to be %u, was %u for %u/%u\n", expected_size, size, j, n);
                    data = pSdbGetStringTagPtr(pdb, tagstr);
                    ok(data && !_wcsicmp(data, all[j]), "Expected data to be %s was %s for %u/%u\n", wine_dbgstr_w(all[j]), wine_dbgstr_w(data), j, n);
                }
                tagstr = pSdbFindNextTag(pdb, TAGID_ROOT, tagstr);
            }
            ok(tagstr == TAGID_NULL, "Expected to be at the end for %u\n", n);
        }

        pSdbCloseDatabase(pdb);
        DeleteFileW(path1);
    }
}

static void match_strw_attr_imp(PDB pdb, TAGID parent, TAG find, const WCHAR* compare)
{
    TAGID attr = pSdbFindFirstTag(pdb, parent, find);
    winetest_ok(attr != TAG_NULL, "Could not find: %x\n", find);
    if (attr != TAG_NULL)
    {
        LPWSTR name = pSdbGetStringTagPtr(pdb, attr);
        winetest_ok(name != NULL, "Could not convert attr to str.\n");
        if (name)
        {
            winetest_ok(wcscmp(name, compare) == 0, "Expected tagid %x to be %s, was %s\n", attr, wine_dbgstr_w(compare), wine_dbgstr_w(name));
        }
    }
}

static void match_dw_attr_imp(PDB pdb, TAGID parent, TAG find, DWORD compare)
{
    TAGID attr = pSdbFindFirstTag(pdb, parent, find);
    winetest_ok(attr != TAG_NULL, "Could not find: %x\n", find);
    if (attr != TAG_NULL)
    {
        DWORD val = pSdbReadDWORDTag(pdb, attr, 0x1234567);
        winetest_ok(val == compare, "Expected tagid %x to be 0x%x, was 0x%x\n", attr, compare, val);
    }
}

static void match_qw_attr_imp(PDB pdb, TAGID parent, TAG find, QWORD compare)
{
    TAGID attr = pSdbFindFirstTag(pdb, parent, find);
    winetest_ok(attr != TAG_NULL, "Could not find: %x\n", find);
    if (attr != TAG_NULL)
    {
        QWORD val = pSdbReadQWORDTag(pdb, attr, 0x123456789abcdef);
        winetest_ok(val == compare, "Expected tagid %x to be 0x%I64x, was 0x%I64x\n", attr, compare, val);
    }
}

static void match_guid_attr_imp(PDB pdb, TAGID parent, TAG find, const GUID* compare)
{
    TAGID attr = pSdbFindFirstTag(pdb, parent, find);
    winetest_ok(attr != TAG_NULL, "Could not find: %x\n", find);
    if (attr != TAG_NULL)
    {
        GUID guid = { 0 };
        BOOL result = pSdbReadBinaryTag(pdb, attr, (PBYTE)&guid, sizeof(guid));
        winetest_ok(result, "expected pSdbReadBinaryTag not to fail.\n");
        winetest_ok(IsEqualGUID(guid, *compare), "expected guids to be equal(%s:%s)\n", wine_dbgstr_guid(&guid), wine_dbgstr_guid(compare));
    }
}

#define match_strw_attr  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : match_strw_attr_imp
#define match_dw_attr  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : match_dw_attr_imp
#define match_qw_attr  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : match_qw_attr_imp
#define match_guid_attr  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : match_guid_attr_imp


//The application name cannot contain any of the following characters:
// \ / < > : * ? |  "

static void check_db_properties(PDB pdb, TAGID root)
{
    TAGID iter = pSdbFindFirstTag(pdb, root, TAG_DATABASE_ID);
    ok(iter != TAGID_NULL, "expected a result, got TAGID_NULL\n");
    if (iter != TAGID_NULL)
    {
        GUID guid = { 0 }, guid2 = { 0 };
        BOOL result = pSdbReadBinaryTag(pdb, iter, (PBYTE)&guid, sizeof(guid));
        ok(result, "expected SdbReadBinaryTag not to fail.\n");
        if (result)
        {
            WCHAR guid_wstr[50];
            result = pSdbGUIDToString(&guid, guid_wstr, 50);
            ok(result, "expected SdbGUIDToString not to fail.\n");
            if (result)
            {
                char guid_str[50];
                WideCharToMultiByte(CP_ACP, 0, guid_wstr, -1, guid_str, sizeof(guid_str), NULL, NULL);
                ok_str(guid_str, "{e39b0eb0-55db-450b-9bd4-d20c9484260f}");
            }
            ok(pSdbGetDatabaseID(pdb, &guid2), "expected SdbGetDatabaseID not to fail.\n");
            ok(IsEqualGUID(guid, guid2), "expected guids to be equal(%s:%s)\n", wine_dbgstr_guid(&guid), wine_dbgstr_guid(&guid2));
        }
    }
    match_qw_attr(pdb, root, TAG_TIME, 0x1d1b91a02c0d63e);
    match_strw_attr(pdb, root, TAG_COMPILER_VERSION, L"2.1.0.3");
    match_strw_attr(pdb, root, TAG_NAME, L"apphelp_test1");
    match_dw_attr(pdb, root, TAG_OS_PLATFORM, 1);
}

static void check_db_layer(PDB pdb, TAGID layer)
{
    TAGID shimref, inexclude, is_include;
    ok(layer != TAGID_NULL, "Expected a valid layer, got NULL\n");
    if (!layer)
        return;

    match_strw_attr(pdb, layer, TAG_NAME, L"TestNewMode");
    shimref = pSdbFindFirstTag(pdb, layer, TAG_SHIM_REF);
    ok(shimref != TAGID_NULL, "Expected a valid shim ref, got NULL\n");
    if (!shimref)
        return;

    match_strw_attr(pdb, shimref, TAG_NAME, L"VirtualRegistry");
    match_strw_attr(pdb, shimref, TAG_COMMAND_LINE, L"ThemeActive");
    inexclude = pSdbFindFirstTag(pdb, shimref, TAG_INEXCLUD);
    ok(inexclude != TAGID_NULL, "Expected a valid in/exclude ref, got NULL\n");
    if (!inexclude)
        return;

    is_include = pSdbFindFirstTag(pdb, inexclude, TAG_INCLUDE);
    ok(is_include == TAGID_NULL, "Expected a NULL include ref, but got one anyway.\n");
    match_strw_attr(pdb, inexclude, TAG_MODULE, L"exclude.dll");

    inexclude = pSdbFindNextTag(pdb, shimref, inexclude);
    ok(inexclude != TAGID_NULL, "Expected a valid in/exclude ref, got NULL\n");
    if (!inexclude)
        return;

    is_include = pSdbFindFirstTag(pdb, inexclude, TAG_INCLUDE);
    ok(is_include != TAGID_NULL, "Expected a valid include ref, got NULL\n");
    match_strw_attr(pdb, inexclude, TAG_MODULE, L"include.dll");
}

static void check_matching_file(PDB pdb, TAGID exe, TAGID matching_file, int num)
{
    ok(matching_file != TAGID_NULL, "Expected to find atleast 1 matching file.\n");
    if (matching_file == TAGID_NULL)
        return;

    ok(num < 4, "Too many matches, expected only 4!\n");
    if (num >= 4)
        return;


    match_strw_attr(pdb, matching_file, TAG_NAME, L"*");
    match_strw_attr(pdb, matching_file, TAG_COMPANY_NAME, L"CompanyName");
    match_strw_attr(pdb, matching_file, TAG_PRODUCT_NAME, L"ProductName");
    match_strw_attr(pdb, matching_file, TAG_PRODUCT_VERSION, L"1.0.0.1");
    match_strw_attr(pdb, matching_file, TAG_FILE_VERSION, L"1.0.0.0");

    if (num == 0 || num == 3)
    {
        match_qw_attr(pdb, matching_file, TAG_UPTO_BIN_PRODUCT_VERSION, 0x1000000000001);
        match_qw_attr(pdb, matching_file, TAG_UPTO_BIN_FILE_VERSION, 0x1000000000000);
    }
    if (num == 1 || num == 3)
    {
        match_dw_attr(pdb, matching_file, TAG_PE_CHECKSUM, 0xbaad);
    }
    if (num != 0)
    {
        match_qw_attr(pdb, matching_file, TAG_BIN_PRODUCT_VERSION, 0x1000000000001);
        match_qw_attr(pdb, matching_file, TAG_BIN_FILE_VERSION, 0x1000000000000);
    }
    if (num == 3)
    {
        match_dw_attr(pdb, matching_file, TAG_SIZE, 0x800);
        match_dw_attr(pdb, matching_file, TAG_CHECKSUM, 0x178bd629);
        match_strw_attr(pdb, matching_file, TAG_FILE_DESCRIPTION, L"FileDescription");
        match_dw_attr(pdb, matching_file, TAG_MODULE_TYPE, 3);
        match_dw_attr(pdb, matching_file, TAG_VERFILEOS, 4);
        match_dw_attr(pdb, matching_file, TAG_VERFILETYPE, 1);
        match_dw_attr(pdb, matching_file, TAG_LINKER_VERSION, 0x40002);
        match_strw_attr(pdb, matching_file, TAG_ORIGINAL_FILENAME, L"OriginalFilename");
        match_strw_attr(pdb, matching_file, TAG_INTERNAL_NAME, L"InternalName");
        match_strw_attr(pdb, matching_file, TAG_LEGAL_COPYRIGHT, L"LegalCopyright");
        match_dw_attr(pdb, matching_file, TAG_LINK_DATE, 0x12345);
        match_dw_attr(pdb, matching_file, TAG_UPTO_LINK_DATE, 0x12345);
    }
    if (num > 3)
    {
        ok(0, "unknown case: %d\n", num);
    }
    matching_file = pSdbFindNextTag(pdb, exe, matching_file);
    if (num == 2)
    {
        ok(matching_file != TAGID_NULL, "Did expect a secondary match on %d\n", num);
        match_strw_attr(pdb, matching_file, TAG_NAME, L"test_checkfile.txt");
        match_dw_attr(pdb, matching_file, TAG_SIZE, 0x4);
        match_dw_attr(pdb, matching_file, TAG_CHECKSUM, 0xb0b0b0b0);
    }
    else
    {
        ok(matching_file == TAGID_NULL, "Did not expect a secondary match on %d\n", num);
    }
}

// "C:\WINDOWS\system32\pcaui.exe" /g {bf39e0e6-c61c-4a22-8802-3ea8ad00b655} /x {4e50c93f-b863-4dfa-bae2-d80ef4ce5c89} /a "apphelp_name_allow" /v "apphelp_vendor_allow" /s "Allow it!" /b 1 /f 0 /k 0 /e "C:\Users\Mark\AppData\Local\Temp\apphelp_test\test_allow.exe" /u "http://reactos.org/allow" /c
// "C:\WINDOWS\system32\pcaui.exe" /g {fa150915-1244-4169-a4ba-fc098c442840} /x {156720e1-ef98-4d04-965a-d85de05e6d9f} /a "apphelp_name_disallow" /v "apphelp_vendor_disallow" /s "Not allowed!" /b 2 /f 0 /k 0 /e "C:\Users\Mark\AppData\Local\Temp\apphelp_test\test_disallow.exe" /u "http://reactos.org/disallow" /c

static void check_matching_apphelp(PDB pdb, TAGID apphelp, int num)
{
    if (num == 0)
    {
/*
[Window Title]
Program Compatibility Assistant

[Main Instruction]
This program has known compatibility issues

[Expanded Information]
Allow it!

[^] Hide details  [ ] Don't show this message again  [Check for solutions online] [Run program] [Cancel]
*/
        match_dw_attr(pdb, apphelp, TAG_FLAGS, 1);
        match_dw_attr(pdb, apphelp, TAG_PROBLEMSEVERITY, 1);
        match_dw_attr(pdb, apphelp, TAG_HTMLHELPID, 1);
        match_dw_attr(pdb, apphelp, TAG_APP_NAME_RC_ID, 0x6f0072);
        match_dw_attr(pdb, apphelp, TAG_VENDOR_NAME_RC_ID, 0x720067);
        match_dw_attr(pdb, apphelp, TAG_SUMMARY_MSG_RC_ID, 0);
    }
    else
    {
/*
[Window Title]
Program Compatibility Assistant

[Main Instruction]
This program is blocked due to compatibility issues

[Expanded Information]
Not allowed!

[^] Hide details  [Check for solutions online] [Cancel]
*/
        match_dw_attr(pdb, apphelp, TAG_FLAGS, 1);
        match_dw_attr(pdb, apphelp, TAG_PROBLEMSEVERITY, 2);
        match_dw_attr(pdb, apphelp, TAG_HTMLHELPID, 2);
        match_dw_attr(pdb, apphelp, TAG_APP_NAME_RC_ID, 0x320020);
        match_dw_attr(pdb, apphelp, TAG_VENDOR_NAME_RC_ID, 0x38002e);
        match_dw_attr(pdb, apphelp, TAG_SUMMARY_MSG_RC_ID, 0);
    }
    apphelp = pSdbFindNextTag(pdb, apphelp, apphelp);
    ok(apphelp == TAGID_NULL, "Did not expect a secondary match on %d\n", num);
}

static void check_matching_layer(PDB pdb, TAGID layer, int num)
{
    if (num == 2)
    {
        match_dw_attr(pdb, layer, TAG_LAYER_TAGID, 0x18e);
        match_strw_attr(pdb, layer, TAG_NAME, L"TestNewMode");
    }
    else
    {
        TAGID layer_tagid = pSdbFindFirstTag(pdb, layer, TAG_LAYER_TAGID);
        ok(layer_tagid == TAGID_NULL, "expected not to find a layer tagid, got %x\n", layer_tagid);
        match_strw_attr(pdb, layer, TAG_NAME, L"WinSrv03");
    }
}

static struct
{
    const WCHAR* name;
    const WCHAR* app_name;
    const WCHAR* vendor;
    GUID exe_id;
    const WCHAR* extra_file;
    DWORD dwLayerCount;
    TAGREF atrExes_0;
    DWORD adwExeFlags_0;
    TAGREF atrLayers_0;
    TAGREF trApphelp;
    const char* env_var;
} test_exedata[5] = {
    {
        L"test_allow.exe",
        L"apphelp_name_allow",
        L"apphelp_vendor_allow",
        { 0x4e50c93f, 0xb863, 0x4dfa, { 0xba, 0xe2, 0xd8, 0x0e, 0xf4, 0xce, 0x5c, 0x89 } },
        NULL,
        0,
        0x1c6,
        0x1000,
        0,
        0x1c6,
        NULL,
    },
    {
        L"test_disallow.exe",
        L"apphelp_name_disallow",
        L"apphelp_vendor_disallow",
        { 0x156720e1, 0xef98, 0x4d04, { 0x96, 0x5a, 0xd8, 0x5d, 0xe0, 0x5e, 0x6d, 0x9f } },
        NULL,
        0,
        0x256,
        0x3000,
        0,
        0x256,
        NULL,
    },
    {
        L"test_new.exe",
        L"fixnew_name",
        L"fixnew_vendor",
        { 0xce70ef69, 0xa21d, 0x408b, { 0x84, 0x5b, 0xf9, 0x9e, 0xac, 0x06, 0x09, 0xe7 } },
        L"test_checkfile.txt",
        1,
        0x2ec,
        0,
        0x18e,
        0,
        NULL,
    },
    {
        L"test_w2k3.exe",
        L"fix_name",
        L"fix_vendor",
        { 0xb4ead144, 0xf640, 0x4e4b, { 0x94, 0xc4, 0x0c, 0x7f, 0xa8, 0x66, 0x23, 0xb0 } },
        NULL,
        0,
        0x37c,
        0,
        0,
        0,
        NULL,
    },
    {
        L"test_unknown_file.exe",
        L"apphelp_name_allow",
        L"apphelp_vendor_allow",
        { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
        NULL,
        1,
        0,
        0,
        0x18e,
        0,
        "TestNewMode",
    },
};

static void check_db_exes(PDB pdb, TAGID root)
{
    int num = 0;
    TAGID exe = pSdbFindFirstTag(pdb, root, TAG_EXE);
    TAGID altExe = pSdbFindFirstNamedTag(pdb, root, TAG_EXE, TAG_NAME, L"test_allow.exe");
    ok_hex(altExe, (int)exe);
    while (exe != TAGID_NULL)
    {
        TAGID apphelp, layer;
        ok(num < 4, "Too many matches, expected only 4!\n");
        if (num >= 4)
            break;
        match_strw_attr(pdb, exe, TAG_NAME, test_exedata[num].name);
        match_strw_attr(pdb, exe, TAG_APP_NAME, test_exedata[num].app_name);
        match_strw_attr(pdb, exe, TAG_VENDOR, test_exedata[num].vendor);
        match_guid_attr(pdb, exe, TAG_EXE_ID, &test_exedata[num].exe_id);
        check_matching_file(pdb, exe, pSdbFindFirstTag(pdb, exe, TAG_MATCHING_FILE), num);
        apphelp = pSdbFindFirstTag(pdb, exe, TAG_APPHELP);
        if (num == 0 || num == 1)
        {
            ok(apphelp != TAGID_NULL, "Expected to find a valid apphelp match on %d.\n", num);
            if (apphelp)
                check_matching_apphelp(pdb, apphelp, num);
        }
        else
        {
            ok(apphelp == TAGID_NULL, "Did not expect an apphelp match on %d\n", num);
        }
        layer = pSdbFindFirstTag(pdb, exe, TAG_LAYER);
        if (num == 2 || num == 3)
        {
            ok(layer != TAGID_NULL, "Expected to find a valid layer match on %d.\n", num);
            if (layer)
                check_matching_layer(pdb, layer, num);
        }
        else
        {
            ok(layer == TAGID_NULL, "Did not expect a layer match on %d\n", num);
        }
        ++num;
        exe = pSdbFindNextTag(pdb, root, exe);
    }
    ok(num == 4, "Expected to find 4 exe tags, found: %d\n", num);
}

static struct
{
    DWORD htmlhelpid;
    const WCHAR* link;
    const WCHAR* apphelp_title;
    const WCHAR* apphelp_details;
} test_layerdata[2] = {
    {
        2,
        L"http://reactos.org/disallow",
        L"apphelp_name_disallow",
        L"Not allowed!",
    },
    {
        1,
        L"http://reactos.org/allow",
        L"apphelp_name_allow",
        L"Allow it!",
    },
};

static void check_db_apphelp(PDB pdb, TAGID root)
{
    int num = 0;
    TAGID apphelp = pSdbFindFirstTag(pdb, root, TAG_APPHELP);
    while (apphelp != TAGID_NULL)
    {
        TAGID link;
        ok(num < 2, "Too many matches, expected only 4!\n");
        if (num >= 2)
            break;
        match_dw_attr(pdb, apphelp, TAG_HTMLHELPID, test_layerdata[num].htmlhelpid);
        link = pSdbFindFirstTag(pdb, apphelp, TAG_LINK);
        ok(link != TAGID_NULL, "expected to find a link tag\n");
        if (link != TAGID_NULL)
        {
            match_strw_attr(pdb, link, TAG_LINK_URL, test_layerdata[num].link);
        }
        match_strw_attr(pdb, apphelp, TAG_APPHELP_TITLE, test_layerdata[num].apphelp_title);
        match_strw_attr(pdb, apphelp, TAG_APPHELP_DETAILS, test_layerdata[num].apphelp_details);
        apphelp = pSdbFindNextTag(pdb, root, apphelp);
        num++;
    }
    ok(num == 2, "Expected to find 2 layer tags, found: %d\n", num);
}

static void test_GetDatabaseInformation(PDB pdb)
{
    PDB_INFORMATION pInfo;
    BOOL fResult;

    if (!pSdbGetDatabaseInformation || !pSdbFreeDatabaseInformation)
    {
        skip("GetDatabaseInformation or SdbFreeDatabaseInformation not found\n");
        return;
    }

    _SEH2_TRY
    {
        pSdbFreeDatabaseInformation(NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(0, "SdbFreeDatabaseInformation did not handle a NULL pointer very gracefully.\n");
    }
    _SEH2_END;


    pInfo = (PDB_INFORMATION)malloc(sizeof(*pInfo) * 4);
    memset(pInfo, 0xDE, sizeof(*pInfo) * 2);

    fResult = pSdbGetDatabaseInformation(pdb, pInfo);
    ok(fResult, "SdbGetDatabaseInformation failed\n");
    if (fResult)
    {
        ok_int(pInfo->dwSomething, 1);
        ok(IsEqualGUID(GUID_DATABASE_TEST, pInfo->Id), "expected guids to be equal(%s:%s)\n",
           wine_dbgstr_guid(&GUID_DATABASE_TEST), wine_dbgstr_guid(&pInfo->Id));
        ok(wcscmp(pInfo->Description, L"apphelp_test1") == 0,
           "Expected pInfo->Description to be 'apphelp_test1', was %s\n", wine_dbgstr_w(pInfo->Description));

        /* Struct is slightly bigger on some Win10, and the DB version nr is different on all */
        if (g_WinVersion >= WINVER_WIN10)
        {
            ok(pInfo->dwMajor == 3, "Expected pInfo->dwMajor to be 3, was: %d\n", pInfo->dwMajor);
            ok(pInfo->dwMinor == 0, "Expected pInfo->dwMinor to be 0, was: %d\n", pInfo->dwMinor);

            ok(pInfo[1].dwSomething == 4 || pInfo[1].dwSomething == 0xdededede, "Something amiss: 0x%x\n", pInfo[1].dwSomething);
            ok(pInfo[1].dwMajor == 0xdededede, "Cookie2 corrupt: 0x%x\n", pInfo[1].dwMajor);
        }
        else
        {
            ok(pInfo->dwMajor == 2, "Expected pInfo->dwMajor to be 2, was: %d\n", pInfo->dwMajor);
            ok(pInfo->dwMinor == 1, "Expected pInfo->dwMinor to be 1, was: %d\n", pInfo->dwMinor);

            ok(pInfo[1].dwSomething == 0xdededede, "Cookie1 corrupt: 0x%x\n", pInfo[1].dwSomething);
            ok(pInfo[1].dwMajor == 0xdededede, "Cookie2 corrupt: 0x%x\n", pInfo[1].dwMajor);
        }

    }
    free(pInfo);
}

static void test_CheckDatabaseManually(void)
{
    static const WCHAR path[] = {'t','e','s','t','_','d','b','.','s','d','b',0};
    TAGID root;
    PDB pdb;
    BOOL ret;
    DWORD ver_hi, ver_lo;

    test_create_db(L"test_db.sdb", g_WinVersion >= WINVER_WIN10);

    /* both ver_hi and ver_lo cannot be null, it'll crash. */
    ver_hi = ver_lo = 0x12345678;
    ret = pSdbGetDatabaseVersion(path, &ver_hi, &ver_lo);
    ok(ret, "Expected SdbGetDatabaseVersion to succeed\n");
    if (g_WinVersion >= WINVER_WIN10)
    {
        ok(ver_hi == 3, "Expected ver_hi to be 3, was: %d\n", ver_hi);
        ok(ver_lo == 0, "Expected ver_lo to be 0, was: %d\n", ver_lo);
    }
    else
    {
        ok(ver_hi == 2, "Expected ver_hi to be 2, was: %d\n", ver_hi);
        ok(ver_lo == 1, "Expected ver_lo to be 1, was: %d\n", ver_lo);
    }

    ver_hi = ver_lo = 0x12345678;
    ret = pSdbGetDatabaseVersion(NULL, &ver_hi, &ver_lo);
    if (g_WinVersion >= WINVER_WIN10)
    {
        ok(!ret, "Expected SdbGetDatabaseVersion to fail\n");
        ok(ver_hi == 0, "Expected ver_hi to be 0, was: 0x%x\n", ver_hi);
        ok(ver_lo == 0, "Expected ver_lo to be 0, was: 0x%x\n", ver_lo);
    }
    else
    {
        ok(ret, "Expected SdbGetDatabaseVersion to succeed\n");
        ok(ver_hi == 0x12345678, "Expected ver_hi to be 0x12345678, was: 0x%x\n", ver_hi);
        ok(ver_lo == 0x12345678, "Expected ver_lo to be 0x12345678, was: 0x%x\n", ver_lo);
    }

    ver_hi = ver_lo = 0x12345678;
    ret = pSdbGetDatabaseVersion(path + 1, &ver_hi, &ver_lo);
    if (g_WinVersion >= WINVER_WIN10)
    {
        ok(!ret, "Expected SdbGetDatabaseVersion to fail\n");
        ok(ver_hi == 0, "Expected ver_hi to be 0, was: 0x%x\n", ver_hi);
        ok(ver_lo == 0, "Expected ver_lo to be 0, was: 0x%x\n", ver_lo);
    }
    else
    {
        ok(ret, "Expected SdbGetDatabaseVersion to succeed\n");
        ok(ver_hi == 0x12345678, "Expected ver_hi to be 0x12345678, was: 0x%x\n", ver_hi);
        ok(ver_lo == 0x12345678, "Expected ver_lo to be 0x12345678, was: 0x%x\n", ver_lo);
    }

    pdb = pSdbOpenDatabase(path, DOS_PATH);
    ok(pdb != NULL, "unexpected NULL handle\n");

    root = pSdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    ok(root != TAGID_NULL, "expected to find a root tag\n");
    if (root != TAGID_NULL)
    {
        TAGID tagLayer = pSdbFindFirstTag(pdb, root, TAG_LAYER);
        TAGID tagAlt = pSdbFindFirstNamedTag(pdb, root, TAG_LAYER, TAG_NAME, L"TestNewMode");
        TAGID tagAlt2 = pSdbFindFirstNamedTag(pdb, root, TAG_LAYER, TAG_NAME, L"TESTNEWMODE");
        TAGID tagAlt3 = pSdbFindFirstNamedTag(pdb, root, TAG_LAYER, TAG_NAME, L"testnewmode");
        ok_hex(tagLayer, (int)tagAlt);
        ok_hex(tagLayer, (int)tagAlt2);
        ok_hex(tagLayer, (int)tagAlt3);
        check_db_properties(pdb, root);
        check_db_layer(pdb, tagLayer);
        check_db_exes(pdb, root);
        check_db_apphelp(pdb, root);
    }
    test_GetDatabaseInformation(pdb);

    pSdbCloseDatabase(pdb);
    DeleteFileA("test_db.sdb");
}

static void test_is_testdb(PDB pdb)
{
    if (pdb)
    {
        GUID guid;
        memset(&guid, 0, sizeof(guid));
        ok(pSdbGetDatabaseID(pdb, &guid), "expected SdbGetDatabaseID not to fail.\n");
        ok(IsEqualGUID(guid, GUID_DATABASE_TEST), "Expected SdbGetDatabaseID to return the test db GUID, was: %s\n", wine_dbgstr_guid(&guid));
    }
    else
    {
        skip("Not checking DB GUID, received a null pdb\n");
    }
}


template<typename SDBQUERYRESULT_T>
static void check_adwExeFlags(DWORD adwExeFlags_0, SDBQUERYRESULT_T& query, const char* file, int line, size_t cur)
{
    ok_(file, line)(query.adwExeFlags[0] == adwExeFlags_0, "Expected adwExeFlags[0] to be 0x%x, was: 0x%x for %d\n", adwExeFlags_0, query.adwExeFlags[0], cur);
    for (size_t n = 1; n < _countof(query.atrExes); ++n)
        ok_(file, line)(query.adwExeFlags[n] == 0, "Expected adwExeFlags[%d] to be 0, was: %x for %d\n", n, query.adwExeFlags[0], cur);
}

template<>
void check_adwExeFlags(DWORD, SDBQUERYRESULT_2k3&, const char*, int, size_t)
{
}


template<typename SDBQUERYRESULT_T>
static void test_mode_generic(const WCHAR* workdir, HSDB hsdb, size_t cur)
{
    WCHAR exename[MAX_PATH], testfile[MAX_PATH];
    BOOL ret;
    SDBQUERYRESULT_T query;
    PDB pdb;
    TAGID tagid;
    TAGREF trApphelp;
    DWORD expect_flags = 0, adwExeFlags_0, exe_count;
    UNICODE_STRING exenameNT;

    memset(&query, 0xab, sizeof(query));

    swprintf(exename, L"%s\\%s", workdir, test_exedata[cur].name);
    if (test_exedata[cur].extra_file)
        swprintf(testfile, L"%s\\%s", workdir, test_exedata[cur].extra_file);
    test_create_exe(exename, 0);

    if (test_exedata[cur].extra_file)
    {
        /* First we try without the file at all. */
        DeleteFileW(testfile);
        ret = pSdbGetMatchingExe(hsdb, exename, NULL, NULL, 0, (SDBQUERYRESULT_VISTA*)&query);
        ok(ret == 0, "SdbGetMatchingExe should have failed for %d.\n", cur);
        /* Now re-try with the correct file */
        test_create_file(testfile, "aaaa", 4);
    }

#if 0
    // Results seem to be cached based on filename, until we can invalidate this, do not test the same filename twice!
    DeleteFileW(exename);
    // skip exports
    test_create_exe(exename, 1);
    ret = pSdbGetMatchingExe(hsdb, exenameW, NULL, NULL, 0, &query);
    ok(ret == 0, "SdbGetMatchingExe should have failed for %d.\n", cur);

    DeleteFileW(exename);
    test_create_exe(exename, 0);
#endif

    if (test_exedata[cur].env_var)
    {
        SetEnvironmentVariableA("__COMPAT_LAYER", test_exedata[cur].env_var);
    }

    ret = pSdbGetMatchingExe(hsdb, exename, NULL, NULL, 0, (SDBQUERYRESULT_VISTA*)&query);
    ok(ret, "SdbGetMatchingExe should not fail for %d.\n", cur);

    exe_count = (test_exedata[cur].env_var == NULL) ? 1 : 0;

    ok(query.dwExeCount == exe_count, "Expected dwExeCount to be %d, was %d for %d\n", exe_count, query.dwExeCount, cur);
    ok(query.dwLayerCount == test_exedata[cur].dwLayerCount, "Expected dwLayerCount to be %d, was %d for %d\n", test_exedata[cur].dwLayerCount, query.dwLayerCount, cur);
    ok(query.dwCustomSDBMap == 1, "Expected dwCustomSDBMap to be 1, was %d for %d\n", query.dwCustomSDBMap, cur);
    ok(query.dwLayerFlags == 0, "Expected dwLayerFlags to be 0, was 0x%x for %d\n", query.dwLayerFlags, cur);
    trApphelp = (g_WinVersion < WINVER_WIN10) ? 0 : test_exedata[cur].trApphelp;
    ok(query.trApphelp == trApphelp, "Expected trApphelp to be 0x%x, was 0x%x for %d\n", trApphelp, query.trApphelp, cur);

    if (g_WinVersion < WINVER_WIN7)
        expect_flags = 0;
    else if (g_WinVersion < WINVER_WIN8)
        expect_flags = 1;
    else if (g_WinVersion < WINVER_WIN10)
        expect_flags = 0x101;
    else
    {
        expect_flags = 0x121;   /* for 2 and 3, this becomes 101 when not elevated. */
        if ((cur == 2 || cur == 3) && !IsUserAdmin())
            expect_flags &= ~0x20;
    }

    if (test_exedata[cur].env_var)
        expect_flags &= ~0x100;

    ok(query.dwFlags == expect_flags, "Expected dwFlags to be 0x%x, was 0x%x for %d\n", expect_flags, query.dwFlags, cur);

    ok(query.atrExes[0] == test_exedata[cur].atrExes_0, "Expected atrExes[0] to be 0x%x, was: 0x%x for %d\n", test_exedata[cur].atrExes_0, query.atrExes[0], cur);
    for (size_t n = 1; n < _countof(query.atrExes); ++n)
        ok(query.atrExes[n] == 0, "Expected atrExes[%d] to be 0, was: %x for %d\n", n, query.atrExes[n], cur);

    adwExeFlags_0 = (g_WinVersion < WINVER_WIN10) ? 0 : test_exedata[cur].adwExeFlags_0;
    check_adwExeFlags(adwExeFlags_0, query, __FILE__, __LINE__, cur);

    ok(query.atrLayers[0] == test_exedata[cur].atrLayers_0, "Expected atrLayers[0] to be 0x%x, was: %x for %d\n", test_exedata[cur].atrLayers_0, query.atrLayers[0], cur);
    for (size_t n = 1; n < _countof(query.atrLayers); ++n)
        ok(query.atrLayers[n] == 0, "Expected atrLayers[%d] to be 0, was: %x for %d\n", n, query.atrLayers[0], cur);

    if (g_WinVersion >= WINVER_VISTA)
        ok(IsEqualGUID(query.rgGuidDB[0], GUID_DATABASE_TEST), "Expected rgGuidDB[0] to be the test db GUID, was: %s for %d\n", wine_dbgstr_guid(&query.rgGuidDB[0]), cur);
    else
        ok(IsEqualGUID(query.rgGuidDB[0], GUID_MAIN_DATABASE), "Expected rgGuidDB[0] to be the main db GUID, was: %s for %d\n", wine_dbgstr_guid(&query.rgGuidDB[0]), cur);
    for (size_t n = 1; n < _countof(query.rgGuidDB); ++n)
        ok(IsEqualGUID(query.rgGuidDB[n], GUID_NULL), "Expected rgGuidDB[%d] to be GUID_NULL, was: %s for %d\n", n, wine_dbgstr_guid(&query.rgGuidDB[n]), cur);

    if (query.atrExes[0])
    {
        pdb = (PDB)0x12345678;
        tagid = 0x76543210;
        ret = pSdbTagRefToTagID(hsdb, query.atrExes[0], &pdb, &tagid);
        ok(ret, "SdbTagRefToTagID failed for %d.\n", cur);
        ok(pdb != NULL && pdb != (PDB)0x12345678, "SdbTagRefToTagID failed to return a pdb for %d.\n", cur);
        ok(tagid != 0 && tagid != 0x76543210, "SdbTagRefToTagID failed to return a tagid for %d.\n", cur);

        if (pdb && pdb != (PDB)0x12345678)
        {
            TAGREF tr = 0x12345678;
            TAG tag = pSdbGetTagFromTagID(pdb, tagid);
            test_is_testdb(pdb);
            ok(tag == TAG_EXE, "Expected tag to be TAG_EXE, was 0x%x for %d.\n", tag, cur);
            match_strw_attr(pdb, tagid, TAG_NAME, test_exedata[cur].name);

            /* And back again */
            ret = pSdbTagIDToTagRef(hsdb, pdb, tagid, &tr);
            ok(ret, "SdbTagIDToTagRef failed for %d.\n", cur);
            ok(tr == query.atrExes[0], "Expected tr to be 0x%x, was 0x%x for %d.\n", query.atrExes[0], tr, cur);
        }
        else
        {
            skip("Skipping a bunch of tests because of an invalid pointer\n");
        }
    }

    if (test_exedata[cur].atrLayers_0)
    {
        pdb = (PDB)0x12345678;
        tagid = 0x76543210;
        ret = pSdbTagRefToTagID(hsdb, query.atrLayers[0], &pdb, &tagid);
        ok(ret, "SdbTagRefToTagID failed for %d.\n", cur);
        ok(pdb != NULL && pdb != (PDB)0x12345678, "SdbTagRefToTagID failed to return a pdb for %d.\n", cur);
        ok(tagid != 0 && tagid != 0x76543210, "SdbTagRefToTagID failed to return a tagid for %d.\n", cur);

        if (pdb && pdb != (PDB)0x12345678)
        {
            TAGREF tr = 0x12345678;
            TAG tag = pSdbGetTagFromTagID(pdb, tagid);
            test_is_testdb(pdb);
            ok(tag == TAG_LAYER, "Expected tag to be TAG_LAYER, was 0x%x for %d.\n", tag, cur);
            match_strw_attr(pdb, tagid, TAG_NAME, L"TestNewMode");

            /* And back again */
            ret = pSdbTagIDToTagRef(hsdb, pdb, tagid, &tr);
            ok(ret, "SdbTagIDToTagRef failed for %d.\n", cur);
            ok(tr == test_exedata[cur].atrLayers_0, "Expected tr to be 0x%x, was 0x%x for %d.\n", test_exedata[cur].atrLayers_0, tr, cur);
        }
        else
        {
            skip("Skipping a bunch of tests because of an invalid pointer\n");
        }
    }

    pdb = (PDB)0x12345678;
    tagid = 0x76543210;
    ret = pSdbTagRefToTagID(hsdb, 0, &pdb, &tagid);
    ok(pdb != NULL && pdb != (PDB)0x12345678, "Expected pdb to be set to a valid pdb, was: %p\n", pdb);
    ok(tagid == 0, "Expected tagid to be set to 0, was: 0x%x\n", tagid);



    if (RtlDosPathNameToNtPathName_U(exename, &exenameNT, NULL, NULL))
    {
        /*
        ERROR,AslPathGetLongFileNameLongpath,110,Long path conversion failed 123 [c0000001]
        ERROR,AslPathBuildSignatureLongpath,1086,AslPathGetLongFileNameLongpath failed for \??\C:\Users\MARK~1.DEV\AppData\Local\Temp\apphelp_test\test_allow.exe [c0000001]
        */
        ret = pSdbGetMatchingExe(hsdb, exenameNT.Buffer, NULL, NULL, 0, (SDBQUERYRESULT_VISTA*)&query);
        ok(!ret, "SdbGetMatchingExe should not succeed for %d.\n", cur);

        RtlFreeUnicodeString(&exenameNT);
    }

    if (test_exedata[cur].extra_file)
        DeleteFileW(testfile);
    DeleteFileW(exename);

    if (test_exedata[cur].env_var)
    {
        SetEnvironmentVariableA("__COMPAT_LAYER", NULL);
    }
}

template<typename SDBQUERYRESULT_T>
static void test_MatchApplications(void)
{
    WCHAR workdir[MAX_PATH], dbpath[MAX_PATH];
    BOOL ret;
    HSDB hsdb;

    ret = GetTempPathW(_countof(workdir), workdir);
    ok(ret, "GetTempPathW error: %d\n", GetLastError());
    wcscat(workdir, L"apphelp_test");

    ret = CreateDirectoryW(workdir, NULL);
    ok(ret, "CreateDirectoryW error: %d\n", GetLastError());

    /* SdbInitDatabase needs an nt-path */
    swprintf(dbpath, L"\\??\\%s\\test.sdb", workdir);

    test_create_db(dbpath + 4, g_WinVersion >= WINVER_WIN10);

    hsdb = pSdbInitDatabase(HID_DATABASE_FULLPATH, dbpath);

    ok(hsdb != NULL, "Expected a valid database handle\n");

    if (!hsdb)
    {
        skip("SdbInitDatabase not implemented?\n");
    }
    else
    {
        /* now that our enviroment is setup, let's go ahead and run the actual tests.. */
        size_t n;
        for (n = 0; n < _countof(test_exedata); ++n)
            test_mode_generic<SDBQUERYRESULT_T>(workdir, hsdb, n);
        pSdbReleaseDatabase(hsdb);
    }

    DeleteFileW(dbpath + 4);

    ret = RemoveDirectoryW(workdir);
    ok(ret, "RemoveDirectoryW error: %d\n", GetLastError());
}

static BOOL write_raw_file(const WCHAR* FileName, const void* Data, DWORD Size)
{
    BOOL Success;
    DWORD dwWritten;
    HANDLE Handle = CreateFileW(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (Handle == INVALID_HANDLE_VALUE)
    {
        skip("Failed to create temp file %ls, error %u\n", FileName, GetLastError());
        return FALSE;
    }
    Success = WriteFile(Handle, Data, Size, &dwWritten, NULL);
    ok(Success == TRUE, "WriteFile failed with %u\n", GetLastError());
    ok(dwWritten == Size, "WriteFile wrote %u bytes instead of %u\n", dwWritten, Size);
    CloseHandle(Handle);
    return Success && (dwWritten == Size);
}

static bool extract_resource(const WCHAR* Filename, LPCWSTR ResourceName)
{
    HMODULE hMod = GetModuleHandleW(NULL);
    HRSRC hRsrc = FindResourceW(hMod, ResourceName, MAKEINTRESOURCEW(RT_RCDATA));
    ok(!!hRsrc, "Unable to find %s\n", wine_dbgstr_w(ResourceName));
    if (!hRsrc)
        return false;

    HGLOBAL hGlobal = LoadResource(hMod, hRsrc);
    DWORD Size = SizeofResource(hMod, hRsrc);
    LPVOID pData = LockResource(hGlobal);

    ok(Size && !!pData, "Unable to load %s\n", wine_dbgstr_w(ResourceName));
    if (!Size || !pData)
        return false;

    BOOL Written = write_raw_file(Filename, pData, Size);
    UnlockResource(pData);
    return Written;
}

template<typename SDBQUERYRESULT_T>
static void test_match_ex(const WCHAR* workdir, HSDB hsdb)
{
    WCHAR exename[MAX_PATH];
    PWCHAR Vendor, AppName, TestName;
    SDBQUERYRESULT_T query;
    TAGID dbtag, exetag, tagid;
    BOOL ret, Succeed;
    PDB pdb;

    memset(&query, 0xab, sizeof(query));

    ret = pSdbTagRefToTagID(hsdb, 0, &pdb, &tagid);
    ok(pdb != NULL && pdb != (PDB)0x12345678, "Expected pdb to be set to a valid pdb, was: %p\n", pdb);

    dbtag = pSdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    ok(dbtag != TAGID_NULL, "Expected to get a valid TAG_DATABASE\n");

    for (exetag = pSdbFindFirstTag(pdb, dbtag, TAG_EXE); exetag; exetag = pSdbFindNextTag(pdb, dbtag, exetag))
    {
        tagid = pSdbFindFirstTag(pdb, exetag, TAG_VENDOR);
        Vendor = pSdbGetStringTagPtr(pdb, tagid);
        if (!Vendor)
            continue;
        Succeed = !_wcsicmp(Vendor, L"Succeed");
        if (!Succeed && _wcsicmp(Vendor, L"Fail"))
            continue;
        tagid = pSdbFindFirstTag(pdb, exetag, TAG_APP_NAME);
        AppName = pSdbGetStringTagPtr(pdb, tagid);
        if (!AppName)
            continue;

        tagid = pSdbFindFirstTag(pdb, exetag, TAG_NAME);
        TestName = pSdbGetStringTagPtr(pdb, tagid);
        if (!TestName)
            continue;

        swprintf(exename, L"%s\\%s", workdir, AppName);
        test_create_exe(exename, 0);

        ret = pSdbGetMatchingExe(hsdb, exename, NULL, NULL, 0, (SDBQUERYRESULT_VISTA*)&query);
        DWORD exe_count = Succeed ? 1 : 0;

        if (Succeed && !ret && g_WinVersion == _WIN32_WINNT_WS03)
        {
            skip("As long as we do not have indexes, we will hit a bug in W2k3\n");
#if 0
[Info][SdbGetIndex         ] index 0x7007(0x600b) was not found in the index table
[Info][SdbGetIndex         ] index 0x7007(0x6001) was not found in the index table
[Info][SdbpSearchDB        ] Searching database with no index.
[Err ][SdbpSearchDB        ] No DATABASE tag found.
#endif
        }
        else
        {
            if (Succeed)
                ok(ret, "SdbGetMatchingExe should not fail for %s.\n", wine_dbgstr_w(TestName));
            else
                ok(!ret, "SdbGetMatchingExe should not succeed for %s.\n", wine_dbgstr_w(TestName));

            ok(query.dwExeCount == exe_count, "Expected dwExeCount to be %d, was %d for %s\n", exe_count, query.dwExeCount, wine_dbgstr_w(TestName));
        }
        DeleteFileW(exename);
    }
}


template<typename SDBQUERYRESULT_T>
static void test_MatchApplicationsEx(void)
{
    WCHAR workdir[MAX_PATH], dbpath[MAX_PATH];
    BOOL ret;
    HSDB hsdb;

    ret = GetTempPathW(_countof(workdir), workdir);
    ok(ret, "GetTempPathW error: %d\n", GetLastError());
    lstrcatW(workdir, L"apphelp_test");

    ret = CreateDirectoryW(workdir, NULL);
    ok(ret, "CreateDirectoryW error: %d\n", GetLastError());

    /* SdbInitDatabase needs an nt-path */
    swprintf(dbpath, L"\\??\\%s\\test.sdb", workdir);

    if (extract_resource(dbpath + 4, MAKEINTRESOURCEW(101)))
    {
        hsdb = pSdbInitDatabase(HID_DATABASE_FULLPATH, dbpath);

        ok(hsdb != NULL, "Expected a valid database handle\n");

        if (!hsdb)
        {
            skip("SdbInitDatabase not implemented?\n");
        }
        else
        {
            /* now that our enviroment is setup, let's go ahead and run the actual tests.. */
            test_match_ex<SDBQUERYRESULT_T>(workdir, hsdb);
            pSdbReleaseDatabase(hsdb);
        }
    }
    else
    {
        ok(0, "Unable to extract database\n");
    }

    DeleteFileW(dbpath + 4);

    ret = RemoveDirectoryW(workdir);
    ok(ret, "RemoveDirectoryW error: %d\n", GetLastError());
}


static void test_TagRef(void)
{
    WCHAR tmpdir[MAX_PATH], dbpath[MAX_PATH];
    BOOL ret;
    HSDB hsdb;
    PDB pdb;
    TAGID db;
    DWORD size;
    TAGREF tr;

    ret = GetTempPathW(_countof(tmpdir), tmpdir);
    ok(ret, "GetTempPathA error: %d\n", GetLastError());

    /* SdbInitDatabase needs an nt-path */
    swprintf(dbpath, L"\\??\\%stest.sdb", tmpdir);

    test_create_db(dbpath + 4, g_WinVersion >= WINVER_WIN10);

    hsdb = pSdbInitDatabase(HID_DATABASE_FULLPATH, dbpath);

    /* HSDB is the only arg that can't be null */
    ret = pSdbTagRefToTagID(hsdb, 0, NULL, NULL);
    ok(ret == TRUE, "Expected ret to be TRUE, was: %d\n", ret);

    size = test_get_db_size();

    pdb = (PDB)&db;
    db = 12345;
    ret = pSdbTagRefToTagID(hsdb, size - 1, &pdb, &db);
    ok(ret == TRUE, "Expected ret to be TRUE, was: %d\n", ret);
    ok(pdb != NULL, "Expected a result, got: %p\n", pdb);
    ok(db == (size - 1), "Expected %u, got: %u\n", size - 1, db);

    /* Convert it back. */
    tr = 0x12345678;
    ret = pSdbTagIDToTagRef(hsdb, pdb, db, &tr);
    ok(ret == TRUE, "Expected ret to be TRUE, was: %d\n", ret);
    ok(tr == (size - 1), "Expected %u, got: %u\n", size - 1, tr);

    pdb = (PDB)&db;
    db = 12345;
    ret = pSdbTagRefToTagID(hsdb, size, &pdb, &db);
    ok(ret == TRUE, "Expected ret to be TRUE, was: %d\n", ret);
    ok(pdb != NULL, "Expected a result, got: %p\n", pdb);
    ok(db == size, "Expected %u, got: %u\n", size, db);

    tr = 0x12345678;
    ret = pSdbTagIDToTagRef(hsdb, pdb, db, &tr);
    ok(ret == TRUE, "Expected ret to be TRUE, was: %d\n", ret);
    ok(tr == size, "Expected %u, got: %u\n", size, tr);

    pdb = (PDB)&db;
    db = 12345;
    ret = pSdbTagRefToTagID(hsdb, size + 1, &pdb, &db);
    ok(ret == TRUE, "Expected ret to be TRUE, was: %d\n", ret);
    ok(pdb != NULL, "Expected a result, got: %p\n", pdb);
    ok(db == (size + 1), "Expected %u, got: %u\n", size + 1, db);

    tr = 0x12345678;
    ret = pSdbTagIDToTagRef(hsdb, pdb, db, &tr);
    ok(ret == TRUE, "Expected ret to be TRUE, was: %d\n", ret);
    ok(tr == (size + 1), "Expected %u, got: %u\n", (size + 1), tr);

    pdb = (PDB)&db;
    db = 12345;
    ret = pSdbTagRefToTagID(hsdb, 0x0fffffff, &pdb, &db);
    ok(ret == TRUE, "Expected ret to be TRUE, was: %d\n", ret);
    ok(pdb != NULL, "Expected a result, got: %p\n", pdb);
    ok(db == 0x0fffffff, "Expected %u, got: %u\n", 0x0fffffff, db);

    tr = 0x12345678;
    ret = pSdbTagIDToTagRef(hsdb, pdb, db, &tr);
    ok(ret == TRUE, "Expected ret to be TRUE, was: %d\n", ret);
    ok(tr == 0x0fffffff, "Expected %u, got: %u\n", 0x0fffffff, tr);

    pdb = (PDB)&db;
    db = 12345;
    ret = pSdbTagRefToTagID(hsdb, 0x10000000, &pdb, &db);
    ok(ret == FALSE, "Expected ret to be FALSE, was: %d\n", ret);
    ok(pdb == NULL, "Expected no result, got: %p\n", pdb);
    ok(db == 0, "Expected no result, got: 0x%x\n", db);

    tr = 0x12345678;
    ret = pSdbTagIDToTagRef(hsdb, pdb, 0x10000000, &tr);
    ok(ret == FALSE, "Expected ret to be TRUE, was: %d\n", ret);
    ok(tr == 0, "Expected %u, got: %u\n", 0, tr);

    pdb = NULL;
    db = TAGID_NULL;
    ret = pSdbTagRefToTagID(hsdb, TAGID_ROOT, &pdb, NULL);
    ok(ret != FALSE, "Expected ret to be TRUE, was: %d\n", ret);
    ok(pdb != NULL, "Expected pdb to be valid\n");

    if (pdb == NULL)
    {
        skip("Cannot run tests without pdb\n");
    }
    else
    {
        db = pSdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
        if (db != TAGID_NULL)
        {
            TAGID child;
            child = pSdbGetFirstChild(pdb, db);
            while (child != TAGID_NULL)
            {
                PDB pdb_res;
                TAGID tagid_res;
                /* We are using a TAGID as a TAGREF here. */
                ret = pSdbTagRefToTagID(hsdb, child, &pdb_res, &tagid_res);
                ok(ret, "Expected SdbTagRefToTagID to succeed\n");

                /* For simple cases (primary DB) TAGREF == TAGID */
                tr = 0x12345678;
                ret = pSdbTagIDToTagRef(hsdb, pdb_res, tagid_res, &tr);
                ok(ret, "Expected SdbTagIDToTagRef to succeed\n");
                ok_hex(tr, (int)tagid_res);

                child = pSdbGetNextChild(pdb, db, child);
            }
        }
        else
        {
            skip("Cannot run tests without valid db tag\n");
        }
    }

    /* Get a tagref for our own layer */
    tr = pSdbGetLayerTagRef(hsdb, L"TestNewMode");
    ok_hex(tr, 0x18e);

    /* We cannot find a tagref from the main database. */
    tr = pSdbGetLayerTagRef(hsdb, L"256Color");
    ok_hex(tr, 0);

    pSdbReleaseDatabase(hsdb);

    DeleteFileW(dbpath + 4);
}


static void test_DataTags(HSDB hsdb)
{
    PDB pdb = NULL;
    TAGID db = TAGID_NULL, layer, exe;
    TAGREF trData;
    BYTE Buffer[1024];
    DWORD dwBufferSize, dwDataType, dwRet;
    TAGID tiData;

    BOOL ret = pSdbTagRefToTagID(hsdb, TAGID_ROOT, &pdb, NULL);

    ok(ret != FALSE, "Expected ret to be TRUE, was: %d\n", ret);
    ok(pdb != NULL, "Expected pdb to be valid\n");

    if (pdb == NULL)
    {
        skip("Cannot run tests without pdb\n");
        return;
    }

    db = pSdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    ok(db != NULL, "Expected db to be valid\n");
    if (db == TAGID_NULL)
    {
        skip("Cannot run tests without db\n");
        return;
    }

    layer = pSdbFindFirstNamedTag(pdb, db, TAG_LAYER, TAG_NAME, L"DATA_LAYER");
    ok(layer != NULL, "Expected layer to be valid\n");
    if (layer == TAGID_NULL)
    {
        skip("Cannot run tests without layer\n");
        return;
    }

    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = sizeof(Buffer);
    dwDataType = 0x12345;
    tiData = 0x111111;
    dwRet = pSdbQueryDataExTagID(pdb, layer, L"TESTDATA1", &dwDataType, Buffer, &dwBufferSize, &tiData);
    ok_hex(dwRet, ERROR_SUCCESS);
    ok_hex(dwDataType, REG_DWORD);
    ok_hex(dwBufferSize, sizeof(DWORD));
    ok_hex(*(DWORD*)Buffer, 3333);
    ok(tiData != NULL && tiData != 0x111111, "Expected tiData, got NULL\n");
    ok_hex(pSdbGetTagFromTagID(pdb, tiData), TAG_DATA);

    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = sizeof(Buffer);
    dwRet = pSdbQueryDataExTagID(pdb, layer, L"TESTDATA1", NULL, Buffer, &dwBufferSize, NULL);
    ok_hex(dwRet, ERROR_SUCCESS);
    ok_hex(dwBufferSize, sizeof(DWORD));
    ok_hex(*(DWORD*)Buffer, 3333);

    if (g_WinVersion > _WIN32_WINNT_WS03)
    {
        memset(Buffer, 0xaa, sizeof(Buffer));
        dwBufferSize = sizeof(Buffer);
        dwRet = pSdbQueryDataExTagID(pdb, layer, L"TESTDATA1", NULL, Buffer, NULL, NULL);
        ok_hex(dwRet, ERROR_INSUFFICIENT_BUFFER);
        ok_hex(*(DWORD*)Buffer, (int)0xaaaaaaaa);
    }

    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = 1;
    dwRet = pSdbQueryDataExTagID(pdb, layer, L"TESTDATA1", NULL, Buffer, &dwBufferSize, NULL);
    ok_hex(dwRet, ERROR_INSUFFICIENT_BUFFER);
    ok_hex(dwBufferSize, sizeof(DWORD));
    ok_hex(*(DWORD*)Buffer, (int)0xaaaaaaaa);

    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = sizeof(Buffer);
    dwRet = pSdbQueryDataExTagID(pdb, layer, L"TESTDATA1", NULL, NULL, &dwBufferSize, NULL);
    ok_hex(dwRet, ERROR_INSUFFICIENT_BUFFER);
    ok_hex(dwBufferSize, sizeof(DWORD));
    ok_hex(*(DWORD*)Buffer, (int)0xaaaaaaaa);

    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = sizeof(Buffer);
    dwRet = pSdbQueryDataExTagID(pdb, TAGID_NULL, L"TESTDATA1", NULL, Buffer, &dwBufferSize, NULL);
    ok_hex(dwRet, ERROR_NOT_FOUND);
    ok_hex(dwBufferSize, sizeof(Buffer));
    ok_hex(*(DWORD*)Buffer, (int)0xaaaaaaaa);

    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = sizeof(Buffer);
    dwDataType = 0x12345;
    tiData = 0x111111;
    dwRet = pSdbQueryDataExTagID(pdb, layer, L"TESTDATA2", &dwDataType, Buffer, &dwBufferSize, &tiData);
    ok_hex(dwRet, ERROR_SUCCESS);
    ok_hex(dwDataType, REG_QWORD);
    ok_hex(dwBufferSize, sizeof(QWORD));
    ok(*(QWORD*)Buffer == 4294967295ull, "unexpected value 0x%I64x, expected 4294967295\n", *(QWORD*)Buffer);
    ok(tiData != NULL && tiData != 0x111111, "Expected tiData, got NULL\n");
    ok_hex(pSdbGetTagFromTagID(pdb, tiData), TAG_DATA);

    /* Not case sensitive */
    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = sizeof(Buffer);
    dwDataType = 0x12345;
    tiData = 0x111111;
    dwRet = pSdbQueryDataExTagID(pdb, layer, L"TESTDATA3", &dwDataType, Buffer, &dwBufferSize, &tiData);
    ok_hex(dwRet, ERROR_SUCCESS);
    ok_hex(dwDataType, REG_SZ);
    ok_hex(dwBufferSize, (int)((wcslen(L"Test string")+1) * sizeof(WCHAR)));
    Buffer[_countof(Buffer)-1] = L'\0';
    ok_wstr(((WCHAR*)Buffer), L"Test string");
    ok(tiData != NULL && tiData != 0x111111, "Expected tiData, got NULL\n");
    ok_hex(pSdbGetTagFromTagID(pdb, tiData), TAG_DATA);

    /* Show that SdbQueryDataEx behaves the same */
    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = sizeof(Buffer);
    dwDataType = 0x12345;
    trData = 0x111111;
    dwRet = pSdbQueryDataEx(hsdb, layer, L"TESTDATA1", &dwDataType, Buffer, &dwBufferSize, &trData);
    ok_hex(dwRet, ERROR_SUCCESS);
    ok_hex(dwDataType, REG_DWORD);
    ok_hex(dwBufferSize, sizeof(DWORD));
    ok_hex(*(DWORD*)Buffer, 3333);
    ok(trData != NULL && trData != 0x111111, "Expected trData, got NULL\n");

    /* And SdbQueryData as well */
    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = sizeof(Buffer);
    dwDataType = 0x12345;
    dwRet = pSdbQueryData(hsdb, layer, L"TESTDATA1", &dwDataType, Buffer, &dwBufferSize);
    ok_hex(dwRet, ERROR_SUCCESS);
    ok_hex(dwDataType, REG_DWORD);
    ok_hex(dwBufferSize, sizeof(DWORD));
    ok_hex(*(DWORD*)Buffer, 3333);

    exe = pSdbFindFirstNamedTag(pdb, db, TAG_EXE, TAG_NAME, L"test_match0.exe");
    ok(exe != NULL, "Expected exe to be valid\n");
    if (exe == TAGID_NULL)
    {
        skip("Cannot run tests without exe\n");
        return;
    }

    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = sizeof(Buffer);
    dwDataType = 0x12345;
    tiData = 0x111111;
    dwRet = pSdbQueryDataExTagID(pdb, exe, L"TESTDATA1", &dwDataType, Buffer, &dwBufferSize, &tiData);
    ok_hex(dwRet, ERROR_NOT_FOUND);
    ok_hex(dwDataType, 0x12345);
    ok_hex(dwBufferSize, sizeof(Buffer));
    ok_hex(*(DWORD*)Buffer, (int)0xaaaaaaaa);
    ok(tiData == 0x111111, "Expected 0x111111, got 0x%x\n", tiData);

    /* Show that SdbQueryDataEx behaves the same */
    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = sizeof(Buffer);
    dwDataType = 0x12345;
    trData = 0x111111;
    dwRet = pSdbQueryDataEx(hsdb, exe, L"TESTDATA1", &dwDataType, Buffer, &dwBufferSize, &trData);
    ok_hex(dwRet, ERROR_NOT_FOUND);
    ok_hex(dwDataType, 0x12345);
    ok_hex(dwBufferSize, sizeof(Buffer));
    ok_hex(*(DWORD*)Buffer, (int)0xaaaaaaaa);
    if (g_WinVersion == _WIN32_WINNT_WS03)
        ok(trData == 0, "Expected 0, got 0x%x\n", trData);
    else
        ok(trData == 0x111111, "Expected 0x111111, got 0x%x\n", trData);

    /* And SdbQueryData as well */
    memset(Buffer, 0xaa, sizeof(Buffer));
    dwBufferSize = sizeof(Buffer);
    dwDataType = 0x12345;
    dwRet = pSdbQueryData(hsdb, exe, L"TESTDATA1", &dwDataType, Buffer, &dwBufferSize);
    ok_hex(dwRet, ERROR_NOT_FOUND);
    ok_hex(dwDataType, 0x12345);
    ok_hex(dwBufferSize, sizeof(Buffer));
    ok_hex(*(DWORD*)Buffer, (int)0xaaaaaaaa);
}


static void test_Data(void)
{
    WCHAR workdir[MAX_PATH], dbpath[MAX_PATH];
    BOOL ret;
    HSDB hsdb;

    ret = GetTempPathW(_countof(workdir), workdir);
    ok(ret, "GetTempPathW error: %d\n", GetLastError());
    lstrcatW(workdir, L"apphelp_test");

    ret = CreateDirectoryW(workdir, NULL);
    ok(ret, "CreateDirectoryW error: %d\n", GetLastError());

    /* SdbInitDatabase needs an nt-path */
    swprintf(dbpath, L"\\??\\%s\\test.sdb", workdir);

    if (extract_resource(dbpath + 4, MAKEINTRESOURCEW(101)))
    {
        hsdb = pSdbInitDatabase(HID_DATABASE_FULLPATH, dbpath);

        ok(hsdb != NULL, "Expected a valid database handle\n");

        if (!hsdb)
        {
            skip("SdbInitDatabase not implemented?\n");
        }
        else
        {
            test_DataTags(hsdb);
            pSdbReleaseDatabase(hsdb);
        }
    }
    else
    {
        ok(0, "Unable to extract database\n");
    }

    DeleteFileW(dbpath + 4);

    ret = RemoveDirectoryW(workdir);
    ok(ret, "RemoveDirectoryW error: %d\n", GetLastError());
}


static void expect_indexA_imp(const char* text, LONGLONG expected)
{
    static WCHAR wide_string[100] = { 0 };
    LONGLONG result;
    MultiByteToWideChar(CP_ACP, 0, text, -1, wide_string, 100);

    result = pSdbMakeIndexKeyFromString(wide_string);
    winetest_ok(result == expected, "Expected %s to result in %s, was: %s\n", text, wine_dbgstr_longlong(expected), wine_dbgstr_longlong(result));
}

#define expect_indexA  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_indexA_imp

static void test_IndexKeyFromString(void)
{
#if 0
    static WCHAR tmp[] = { 0xabba, 0xbcde, 0x2020, 0x20, 0x4444, 0 };
    static WCHAR tmp2[] = { 0xabba, 0xbcde, 0x20, 0x4444, 0 };
    static WCHAR tmp3[] = { 0x20, 0xbcde, 0x4041, 0x4444, 0 };
    static WCHAR tmp4[] = { 0x20, 0xbcde, 0x4041, 0x4444, 0x4444, 0 };
    static WCHAR tmp5[] = { 0x2020, 0xbcde, 0x4041, 0x4444, 0x4444, 0 };
    static WCHAR tmp6 [] = { 0x20, 0xbcde, 0x4041, 0x4444, 0x4444, 0x4444, 0};
    static WCHAR tmp7 [] = { 0xbcde, 0x4041, 0x4444, 0x4444, 0x4444, 0x4444, 0x4444, 0x4444, 0x4444, 0};
    static WCHAR tmp8 [] = { 0xbc00, 0x4041, 0x4444, 0x4444, 0x4444, 0x4444, 0x4444, 0x4444, 0x4444, 0};
#endif

#if 0
    /* This crashes. */
    pSdbMakeIndexKeyFromString(NULL);
#endif

    expect_indexA("", 0x0000000000000000);
    expect_indexA("a", 0x4100000000000000);
    expect_indexA("aa", 0x4141000000000000);
    expect_indexA("aaa", 0x4141410000000000);
    expect_indexA("aaaa", 0x4141414100000000);
    expect_indexA("aaaaa", 0x4141414141000000);
    expect_indexA("aaaaaa", 0x4141414141410000);
    expect_indexA("aaaaaaa", 0x4141414141414100);
    expect_indexA("aaaaaaaa", 0x4141414141414141);
    expect_indexA("aaa aaaaa", 0x4141412041414141);
    /* Does not change */
    expect_indexA("aaaaaaaaa", 0x4141414141414141);
    expect_indexA("aaaaaaaab", 0x4141414141414141);
    expect_indexA("aaaaaaaac", 0x4141414141414141);
    expect_indexA("aaaaaaaaF", 0x4141414141414141);
    /* Upcase */
    expect_indexA("AAAAAAAA", 0x4141414141414141);
    expect_indexA("ABABABAB", 0x4142414241424142);
    expect_indexA("ABABABABZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ", 0x4142414241424142);

#if 0
    /* These fail, but is that because the codepoints are too weird, or because the func is not correct? */
    result = pSdbMakeIndexKeyFromString(tmp);
    ok(result == 0xbaabdebc20200000, "Expected %s to result in %s, was: %s\n", wine_dbgstr_w(tmp),
        wine_dbgstr_longlong(0xbaabdebc20200000), wine_dbgstr_longlong(result));

    result = pSdbMakeIndexKeyFromString(tmp2);
    ok(result == 0xbaabdebc00000000, "Expected %s to result in %s, was: %s\n", wine_dbgstr_w(tmp2),
        wine_dbgstr_longlong(0xbaabdebc00000000), wine_dbgstr_longlong(result));

    result = pSdbMakeIndexKeyFromString(tmp3);
    ok(result == 0x20debc4140000000, "Expected %s to result in %s, was: %s\n", wine_dbgstr_w(tmp3),
        wine_dbgstr_longlong(0x20debc4140000000), wine_dbgstr_longlong(result));

    result = pSdbMakeIndexKeyFromString(tmp4);
    ok(result == 0x20debc4140000000, "Expected %s to result in %s, was: %s\n", wine_dbgstr_w(tmp4),
        wine_dbgstr_longlong(0x20debc4140000000), wine_dbgstr_longlong(result));

    result = pSdbMakeIndexKeyFromString(tmp5);
    ok(result == 0x2020debc41400000, "Expected %s to result in %s, was: %s\n", wine_dbgstr_w(tmp5),
        wine_dbgstr_longlong(0x2020debc41400000), wine_dbgstr_longlong(result));

    result = pSdbMakeIndexKeyFromString(tmp6);
    ok(result == 0x20debc4140444400, "Expected %s to result in %s, was: %s\n", wine_dbgstr_w(tmp6),
        wine_dbgstr_longlong(0x20debc4140444400), wine_dbgstr_longlong(result));

    result = pSdbMakeIndexKeyFromString(tmp7);
    ok(result == 0xdebc414044444444, "Expected %s to result in %s, was: %s\n", wine_dbgstr_w(tmp7),
        wine_dbgstr_longlong(0xdebc414044444444), wine_dbgstr_longlong(result));

    result = pSdbMakeIndexKeyFromString(tmp8);
    ok(result == 0xbc414044444444, "Expected %s to result in %s, was: %s\n", wine_dbgstr_w(tmp8),
        wine_dbgstr_longlong(0xbc414044444444), wine_dbgstr_longlong(result));
#endif
}

static int validate_SDBQUERYRESULT_size()
{
    unsigned char buffer[SDBQUERYRESULT_EXPECTED_SIZE_VISTA * 2];
    WCHAR path[MAX_PATH];
    size_t n;

    memset(buffer, 0xab, sizeof(buffer));

    GetModuleFileNameW(NULL, path, MAX_PATH);
    pSdbGetMatchingExe(NULL, path, NULL, NULL, 0, (SDBQUERYRESULT_VISTA*)buffer);
    if (buffer[0] == 0xab)
    {
        trace("SdbGetMatchingExe didnt do anything, cannot determine SDBQUERYRESULT size\n");
        return 0;
    }

    if (buffer[SDBQUERYRESULT_EXPECTED_SIZE_2k3] == 0xab && buffer[SDBQUERYRESULT_EXPECTED_SIZE_2k3-1] != 0xab)
    {
        return 1;
    }

    if (buffer[SDBQUERYRESULT_EXPECTED_SIZE_VISTA] == 0xab && buffer[SDBQUERYRESULT_EXPECTED_SIZE_VISTA-1] != 0xab)
    {
        return 2;
    }

    for (n = 0; n < _countof(buffer); ++n)
    {
        if (buffer[n] != 0xab)
        {
            trace("Unknown size: %i\n", n);
            break;
        }
    }

    return 0;
}


START_TEST(db)
{
    //SetEnvironmentVariableA("SHIM_DEBUG_LEVEL", "4");
    //SetEnvironmentVariableA("SHIMENG_DEBUG_LEVEL", "4");
    //SetEnvironmentVariableA("DEBUGCHANNEL", "+apphelp");

    silence_debug_output();
    hdll = LoadLibraryA("apphelp.dll");

    /* We detect the apphelp version that is loaded, instead of the os we are running on.
       This allows for easier testing multiple versions of the dll */
    g_WinVersion = get_module_version(hdll);
    trace("Apphelp version: 0x%x\n", g_WinVersion);

    *(void**)&pSdbTagToString = (void *)GetProcAddress(hdll, "SdbTagToString");
    *(void**)&pSdbOpenDatabase = (void *)GetProcAddress(hdll, "SdbOpenDatabase");
    *(void**)&pSdbCreateDatabase = (void *)GetProcAddress(hdll, "SdbCreateDatabase");
    *(void**)&pSdbGetDatabaseVersion = (void *)GetProcAddress(hdll, "SdbGetDatabaseVersion");
    *(void**)&pSdbCloseDatabase = (void *)GetProcAddress(hdll, "SdbCloseDatabase");
    *(void**)&pSdbCloseDatabaseWrite = (void *)GetProcAddress(hdll, "SdbCloseDatabaseWrite");
    *(void**)&pSdbGetTagFromTagID = (void *)GetProcAddress(hdll, "SdbGetTagFromTagID");
    *(void**)&pSdbWriteNULLTag = (void *)GetProcAddress(hdll, "SdbWriteNULLTag");
    *(void**)&pSdbWriteWORDTag = (void *)GetProcAddress(hdll, "SdbWriteWORDTag");
    *(void**)&pSdbWriteDWORDTag = (void *)GetProcAddress(hdll, "SdbWriteDWORDTag");
    *(void**)&pSdbWriteQWORDTag = (void *)GetProcAddress(hdll, "SdbWriteQWORDTag");
    *(void**)&pSdbWriteBinaryTagFromFile = (void *)GetProcAddress(hdll, "SdbWriteBinaryTagFromFile");
    *(void**)&pSdbWriteStringTag = (void *)GetProcAddress(hdll, "SdbWriteStringTag");
    *(void**)&pSdbWriteStringRefTag = (void *)GetProcAddress(hdll, "SdbWriteStringRefTag");
    *(void**)&pSdbBeginWriteListTag = (void *)GetProcAddress(hdll, "SdbBeginWriteListTag");
    *(void**)&pSdbEndWriteListTag = (void *)GetProcAddress(hdll, "SdbEndWriteListTag");
    *(void**)&pSdbFindFirstTag = (void *)GetProcAddress(hdll, "SdbFindFirstTag");
    *(void**)&pSdbFindNextTag = (void *)GetProcAddress(hdll, "SdbFindNextTag");
    *(void**)&pSdbFindFirstNamedTag = (void *)GetProcAddress(hdll, "SdbFindFirstNamedTag");
    *(void**)&pSdbReadWORDTag = (void *)GetProcAddress(hdll, "SdbReadWORDTag");
    *(void**)&pSdbReadDWORDTag = (void *)GetProcAddress(hdll, "SdbReadDWORDTag");
    *(void**)&pSdbReadQWORDTag = (void *)GetProcAddress(hdll, "SdbReadQWORDTag");
    *(void**)&pSdbReadBinaryTag = (void *)GetProcAddress(hdll, "SdbReadBinaryTag");
    *(void**)&pSdbReadStringTag = (void *)GetProcAddress(hdll, "SdbReadStringTag");
    *(void**)&pSdbGetTagDataSize = (void *)GetProcAddress(hdll, "SdbGetTagDataSize");
    *(void**)&pSdbGetBinaryTagData = (void *)GetProcAddress(hdll, "SdbGetBinaryTagData");
    *(void**)&pSdbGetStringTagPtr = (void *)GetProcAddress(hdll, "SdbGetStringTagPtr");
    *(void**)&pSdbGetFirstChild = (void *)GetProcAddress(hdll, "SdbGetFirstChild");
    *(void**)&pSdbGetNextChild = (void *)GetProcAddress(hdll, "SdbGetNextChild");
    *(void**)&pSdbGetDatabaseID = (void *)GetProcAddress(hdll, "SdbGetDatabaseID");
    *(void**)&pSdbGUIDToString = (void *)GetProcAddress(hdll, "SdbGUIDToString");
    *(void**)&pSdbInitDatabase = (void *)GetProcAddress(hdll, "SdbInitDatabase");
    *(void**)&pSdbReleaseDatabase = (void *)GetProcAddress(hdll, "SdbReleaseDatabase");
    *(void**)&pSdbGetMatchingExe = (void *)GetProcAddress(hdll, "SdbGetMatchingExe");
    *(void**)&pSdbTagRefToTagID = (void *)GetProcAddress(hdll, "SdbTagRefToTagID");
    *(void**)&pSdbTagIDToTagRef = (void *)GetProcAddress(hdll, "SdbTagIDToTagRef");
    *(void**)&pSdbMakeIndexKeyFromString = (void *)GetProcAddress(hdll, "SdbMakeIndexKeyFromString");
    *(void**)&pSdbQueryData = (void *)GetProcAddress(hdll, "SdbQueryData");
    *(void**)&pSdbQueryDataEx = (void *)GetProcAddress(hdll, "SdbQueryDataEx");
    *(void**)&pSdbQueryDataExTagID = (void *)GetProcAddress(hdll, "SdbQueryDataExTagID");
    *(void**)&pSdbGetLayerTagRef = (void *)GetProcAddress(hdll, "SdbGetLayerTagRef");
    *(void**)&pSdbGetDatabaseInformation = (void *)GetProcAddress(hdll, "SdbGetDatabaseInformation");
    *(void**)&pSdbFreeDatabaseInformation = (void *)GetProcAddress(hdll, "SdbFreeDatabaseInformation");

    test_Sdb();
    test_write_ex();
    test_stringtable();
    test_CheckDatabaseManually();
    switch (validate_SDBQUERYRESULT_size())
    {
    case 1:
        test_MatchApplications<SDBQUERYRESULT_2k3>();
        test_MatchApplicationsEx<SDBQUERYRESULT_2k3>();
        break;
    case 2:
        test_MatchApplications<SDBQUERYRESULT_VISTA>();
        test_MatchApplicationsEx<SDBQUERYRESULT_VISTA>();
        break;
    default:
        skip("Skipping tests with SDBQUERYRESULT due to a wrong size reported\n");
        break;
    }
    test_TagRef();
    test_Data();
    skip("test_SecondaryDB()\n");
    test_IndexKeyFromString();
}
