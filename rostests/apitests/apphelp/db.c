/*
 * Copyright 2012 Detlef Riekenberg
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

#include "wine/test.h"

typedef WORD TAG;
typedef DWORD TAGID;
typedef DWORD TAGREF;
typedef UINT64 QWORD;
typedef VOID* PDB;
typedef INT PATH_TYPE;
#define DOS_PATH 0

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
#define TAG_SIZE (0x1 | TAG_TYPE_DWORD)

#define TAG_MATCH_MODE (0x1 | TAG_TYPE_WORD)

#define TAG_FLAG_LUA (0x10 | TAG_TYPE_QWORD)

#define TAG_STRINGTABLE (0x801 | TAG_TYPE_LIST)

#define TAG_NAME (0x1 | TAG_TYPE_STRINGREF)
#define TAG_STRINGTABLE_ITEM (0x801 | TAG_TYPE_STRING)


#define TAG_GENERAL (0x2 | TAG_TYPE_NULL)

#define TAG_DATA_BITS (0x5 | TAG_TYPE_BINARY)



static HMODULE hdll;
static LPCWSTR (WINAPI *pSdbTagToString)(TAG);
static PDB (WINAPI *pSdbOpenDatabase)(LPCWSTR, PATH_TYPE);
static PDB (WINAPI *pSdbCreateDatabase)(LPCWSTR, PATH_TYPE);
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

static void Write(HANDLE file, LPCVOID buffer, DWORD size)
{
    DWORD dwWritten = 0;
    WriteFile(file, buffer, size, &dwWritten, NULL);
}

static void test_Sdb(void)
{
    static const WCHAR path[] = {'t','e','m','p',0};
    static const WCHAR path2[] = {'t','e','m','p','2',0};
    static const WCHAR tag_size_string[] = {'S','I','Z','E',0};
    static const WCHAR tag_flag_lua_string[] = {'F','L','A','G','_','L','U','A',0};
    static const TAG tags[5] = {
        TAG_SIZE, TAG_FLAG_LUA, TAG_NAME,
        TAG_STRINGTABLE, TAG_STRINGTABLE_ITEM
    };
    WCHAR buffer[6] = {0};
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

    pdb = pSdbCreateDatabase(path, DOS_PATH);
    ok (pdb != NULL, "failed to create database\n");
    if(pdb != NULL)
    {
        ret = pSdbWriteDWORDTag(pdb, tags[0], 0xDEADBEEF);
        ok (ret, "failed to write DWORD tag\n");
        ret = pSdbWriteQWORDTag(pdb, tags[1], 0xDEADBEEFBABE);
        ok (ret, "failed to write QWORD tag\n");
        ret = pSdbWriteStringRefTag(pdb, tags[2], stringref);
        ok (ret, "failed to write stringref tag\n");
        tagid = pSdbBeginWriteListTag(pdb, tags[3]);
        ok (tagid != TAGID_NULL, "unexpected NULL tagid\n");
        ret = pSdbWriteStringTag(pdb, tags[4], path);
        ok (ret, "failed to write string tag\n");
        ret = pSdbWriteNULLTag(pdb, TAG_GENERAL);
        ok (ret, "failed to write NULL tag\n");
        ret = pSdbWriteWORDTag(pdb, TAG_MATCH_MODE, 0xACE);
        ok (ret, "failed to write WORD tag\n");
        ret = pSdbEndWriteListTag(pdb, tagid);
        ok (ret, "failed to update list size\n");
        /* [Err ][SdbCloseDatabase    ] Failed to close the file. */
        pSdbCloseDatabaseWrite(pdb);
    }

    /* [Err ][SdbGetDatabaseID    ] Failed to get root tag */
    pdb = pSdbOpenDatabase(path, DOS_PATH);
    ok(pdb != NULL, "unexpected NULL handle\n");

    if(pdb)
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
        ok (tag == TAG_FLAG_LUA, "unexpected tag 0x%x, expected 0x%x\n", tag, TAG_FLAG_LUA);

        string = pSdbTagToString(tag);
        ok(lstrcmpW(string, tag_flag_lua_string) == 0, "unexpected string %s, expected %s\n",
           wine_dbgstr_w(string), wine_dbgstr_w(tag_flag_lua_string));

        qword = pSdbReadQWORDTag(pdb, tagid, 0);
        ok(qword == 0xDEADBEEFBABE, "unexpected value 0x%I64x, expected 0xDEADBEEFBABE\n", qword);

        tagid = pSdbGetNextChild(pdb, TAGID_ROOT, tagid);
        string = pSdbGetStringTagPtr(pdb, tagid);
        ok (string && (lstrcmpW(string, path) == 0), "unexpected string %s, expected %s\n",
            wine_dbgstr_w(string), wine_dbgstr_w(path));

        ptagid = pSdbGetNextChild(pdb, TAGID_ROOT, tagid);
        tagid = pSdbGetFirstChild(pdb, ptagid);

        string = pSdbGetStringTagPtr(pdb, tagid);
        ok (string && (lstrcmpW(string, path) == 0), "unexpected string %s, expected %s\n",
            wine_dbgstr_w(string), wine_dbgstr_w(path));

        ok (pSdbReadStringTag(pdb, tagid, buffer, 6), "failed to write string to buffer\n");
        /* [Err ][SdbpReadTagData     ] Buffer too small. Avail: 6, Need: 10. */
        ok (!pSdbReadStringTag(pdb, tagid, buffer, 3), "string was written to buffer, but failure was expected");
        ok (pSdbGetTagDataSize(pdb, tagid) == 5 * sizeof(WCHAR), "string has unexpected size\n");

        tagid = pSdbGetNextChild(pdb, ptagid, tagid);
        tag = pSdbGetTagFromTagID(pdb, tagid);
        ok (tag == TAG_GENERAL, "unexpected tag 0x%x, expected 0x%x\n", tag, TAG_GENERAL);
        ok (pSdbGetTagDataSize(pdb, tagid) == 0, "null tag with size > 0\n");

        tagid = pSdbGetNextChild(pdb, ptagid, tagid);
        word = pSdbReadWORDTag(pdb, tagid, 0);
        ok (word == 0xACE, "unexpected value 0x%x, expected 0x%x\n", word, 0xACE);

        pSdbCloseDatabase(pdb);
    }
    DeleteFileW(path);

    file = CreateFileW(path2, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok (file != INVALID_HANDLE_VALUE, "failed to open file\n");
    Write(file, &qword, 8);
    CloseHandle(file);

    pdb = pSdbCreateDatabase(path, DOS_PATH);
    ok(pdb != NULL, "unexpected NULL handle\n");

    if(pdb)
    {
        ret = pSdbWriteBinaryTagFromFile(pdb, TAG_DATA_BITS, path2);
        ok(ret, "failed to write tag from binary file\n");
        pSdbCloseDatabaseWrite(pdb);     /* [Err ][SdbCloseDatabase    ] Failed to close the file. */
        DeleteFileW(path2);

        pdb = pSdbOpenDatabase(path, DOS_PATH);
        ok(pdb != NULL, "unexpected NULL handle\n");
        binary = pSdbGetBinaryTagData(pdb, _TAGID_ROOT);
        ok(memcmp(binary, &qword, 8) == 0, "binary data is corrupt\n");
        ret = pSdbReadBinaryTag(pdb, _TAGID_ROOT, (PBYTE)buffer, 12);
        ok(ret, "failed to read binary tag\n");
        ok(memcmp(buffer, &qword, 8) == 0, "binary data is corrupt\n");
        pSdbCloseDatabase(pdb);
    }
    DeleteFileW(path);
}

START_TEST(db)
{
    //SetEnvironmentVariable("SHIM_DEBUG_LEVEL", "4");
    //SetEnvironmentVariable("DEBUGCHANNEL", "+apphelp");
    hdll = LoadLibraryA("apphelp.dll");
    pSdbTagToString = (void *) GetProcAddress(hdll, "SdbTagToString");
    pSdbOpenDatabase = (void *) GetProcAddress(hdll, "SdbOpenDatabase");
    pSdbCreateDatabase = (void *) GetProcAddress(hdll, "SdbCreateDatabase");
    pSdbCloseDatabase = (void *) GetProcAddress(hdll, "SdbCloseDatabase");
    pSdbCloseDatabaseWrite = (void *) GetProcAddress(hdll, "SdbCloseDatabaseWrite");
    pSdbGetTagFromTagID = (void *) GetProcAddress(hdll, "SdbGetTagFromTagID");
    pSdbWriteNULLTag = (void *) GetProcAddress(hdll, "SdbWriteNULLTag");
    pSdbWriteWORDTag = (void *) GetProcAddress(hdll, "SdbWriteWORDTag");
    pSdbWriteDWORDTag = (void *) GetProcAddress(hdll, "SdbWriteDWORDTag");
    pSdbWriteQWORDTag = (void *) GetProcAddress(hdll, "SdbWriteQWORDTag");
    pSdbWriteBinaryTagFromFile = (void *) GetProcAddress(hdll, "SdbWriteBinaryTagFromFile");
    pSdbWriteStringTag = (void *) GetProcAddress(hdll, "SdbWriteStringTag");
    pSdbWriteStringRefTag = (void *) GetProcAddress(hdll, "SdbWriteStringRefTag");
    pSdbBeginWriteListTag = (void *)GetProcAddress(hdll, "SdbBeginWriteListTag");
    pSdbEndWriteListTag = (void *) GetProcAddress(hdll, "SdbEndWriteListTag");
    pSdbReadWORDTag = (void *) GetProcAddress(hdll, "SdbReadWORDTag");
    pSdbReadDWORDTag = (void *) GetProcAddress(hdll, "SdbReadDWORDTag");
    pSdbReadQWORDTag = (void *) GetProcAddress(hdll, "SdbReadQWORDTag");
    pSdbReadBinaryTag = (void *) GetProcAddress(hdll, "SdbReadBinaryTag");
    pSdbReadStringTag = (void *) GetProcAddress(hdll, "SdbReadStringTag");
    pSdbGetTagDataSize = (void *) GetProcAddress(hdll, "SdbGetTagDataSize");
    pSdbGetBinaryTagData = (void *) GetProcAddress(hdll, "SdbGetBinaryTagData");
    pSdbGetStringTagPtr = (void *) GetProcAddress(hdll, "SdbGetStringTagPtr");
    pSdbGetFirstChild = (void *) GetProcAddress(hdll, "SdbGetFirstChild");
    pSdbGetNextChild = (void *) GetProcAddress(hdll, "SdbGetNextChild");

    test_Sdb();
}
