/* Unit test suite for wintrust crypt functions
 *
 * Copyright 2007 Paul Vriens
 * Copyright 2008 Hans Leidekker for CodeWeavers
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
 *
 */

#include <stdarg.h>
#include <stdio.h>

#include "windows.h"
#include "mscat.h"

#include "wine/test.h"

static char selfname[MAX_PATH];
static CHAR CURR_DIR[MAX_PATH];
static CHAR catroot[MAX_PATH];
static CHAR catroot2[MAX_PATH];

/*
 * Minimalistic catalog file. To reconstruct, save text below as winetest.cdf,
 * convert to DOS line endings and run 'makecat /cat winetest.cdf'
 */

/*
[CatalogHeader]
Name=winetest.cat
ResultDir=.\
PublicVersion=0x00000001
EncodingType=
CATATTR1=0x10010001:attr1:value1
CATATTR2=0x10010001:attr2:value2

[CatalogFiles]
hashme=.\winetest.cdf
*/

static const BYTE test_catalog[] = {
    0x30, 0x82, 0x01, 0xbc, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x02, 0xa0,
    0x82, 0x01, 0xad, 0x30, 0x82, 0x01, 0xa9, 0x02, 0x01, 0x01, 0x31, 0x00, 0x30, 0x82, 0x01, 0x9e,
    0x06, 0x09, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x0a, 0x01, 0xa0, 0x82, 0x01, 0x8f, 0x30,
    0x82, 0x01, 0x8b, 0x30, 0x0c, 0x06, 0x0a, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x0c, 0x01,
    0x01, 0x04, 0x10, 0xfa, 0x55, 0x2c, 0xc2, 0xf6, 0xcc, 0xdd, 0x11, 0x2a, 0x9c, 0x00, 0x14, 0x22,
    0xec, 0x8f, 0x3b, 0x17, 0x0d, 0x30, 0x38, 0x31, 0x32, 0x31, 0x38, 0x31, 0x31, 0x32, 0x36, 0x34,
    0x38, 0x5a, 0x30, 0x0e, 0x06, 0x0a, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x0c, 0x01, 0x02,
    0x05, 0x00, 0x30, 0x81, 0xdd, 0x30, 0x81, 0xda, 0x04, 0x0e, 0x68, 0x00, 0x61, 0x00, 0x73, 0x00,
    0x68, 0x00, 0x6d, 0x00, 0x65, 0x00, 0x00, 0x00, 0x31, 0x81, 0xc7, 0x30, 0x61, 0x06, 0x0a, 0x2b,
    0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x01, 0x04, 0x31, 0x53, 0x30, 0x51, 0x30, 0x2c, 0x06,
    0x0a, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x01, 0x19, 0xa2, 0x1e, 0x80, 0x1c, 0x00,
    0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x4f, 0x00, 0x62, 0x00, 0x73, 0x00, 0x6f, 0x00, 0x6c, 0x00,
    0x65, 0x00, 0x74, 0x00, 0x65, 0x00, 0x3e, 0x00, 0x3e, 0x00, 0x3e, 0x30, 0x21, 0x30, 0x09, 0x06,
    0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14, 0xed, 0xd6, 0x9c, 0x9c, 0xb2, 0xfc,
    0xaa, 0x03, 0xe8, 0xd3, 0x20, 0xf6, 0xab, 0x28, 0xc3, 0xff, 0xbd, 0x07, 0x36, 0xf5, 0x30, 0x62,
    0x06, 0x0a, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x0c, 0x02, 0x02, 0x31, 0x54, 0x30, 0x52,
    0x1e, 0x4c, 0x00, 0x7b, 0x00, 0x44, 0x00, 0x45, 0x00, 0x33, 0x00, 0x35, 0x00, 0x31, 0x00, 0x41,
    0x00, 0x34, 0x00, 0x32, 0x00, 0x2d, 0x00, 0x38, 0x00, 0x45, 0x00, 0x35, 0x00, 0x39, 0x00, 0x2d,
    0x00, 0x31, 0x00, 0x31, 0x00, 0x44, 0x00, 0x30, 0x00, 0x2d, 0x00, 0x38, 0x00, 0x43, 0x00, 0x34,
    0x00, 0x37, 0x00, 0x2d, 0x00, 0x30, 0x00, 0x30, 0x00, 0x43, 0x00, 0x30, 0x00, 0x34, 0x00, 0x46,
    0x00, 0x43, 0x00, 0x32, 0x00, 0x39, 0x00, 0x35, 0x00, 0x45, 0x00, 0x45, 0x00, 0x7d, 0x02, 0x02,
    0x02, 0x00, 0xa0, 0x6a, 0x30, 0x68, 0x30, 0x32, 0x06, 0x0a, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82,
    0x37, 0x0c, 0x02, 0x01, 0x04, 0x24, 0x30, 0x22, 0x1e, 0x0a, 0x00, 0x61, 0x00, 0x74, 0x00, 0x74,
    0x00, 0x72, 0x00, 0x32, 0x02, 0x04, 0x10, 0x01, 0x00, 0x01, 0x04, 0x0e, 0x76, 0x00, 0x61, 0x00,
    0x6c, 0x00, 0x75, 0x00, 0x65, 0x00, 0x32, 0x00, 0x00, 0x00, 0x30, 0x32, 0x06, 0x0a, 0x2b, 0x06,
    0x01, 0x04, 0x01, 0x82, 0x37, 0x0c, 0x02, 0x01, 0x04, 0x24, 0x30, 0x22, 0x1e, 0x0a, 0x00, 0x61,
    0x00, 0x74, 0x00, 0x74, 0x00, 0x72, 0x00, 0x31, 0x02, 0x04, 0x10, 0x01, 0x00, 0x01, 0x04, 0x0e,
    0x76, 0x00, 0x61, 0x00, 0x6c, 0x00, 0x75, 0x00, 0x65, 0x00, 0x31, 0x00, 0x00, 0x00, 0x31, 0x00,
};

static BOOL (WINAPI * pCryptCATAdminAcquireContext)(HCATADMIN*, const GUID*, DWORD);
static BOOL (WINAPI * pCryptCATAdminReleaseContext)(HCATADMIN, DWORD);
static BOOL (WINAPI * pCryptCATAdminCalcHashFromFileHandle)(HANDLE hFile, DWORD*, BYTE*, DWORD);
static HCATINFO (WINAPI * pCryptCATAdminAddCatalog)(HCATADMIN, PWSTR, PWSTR, DWORD);
static BOOL (WINAPI * pCryptCATAdminRemoveCatalog)(HCATADMIN, LPCWSTR, DWORD);
static BOOL (WINAPI * pCryptCATAdminReleaseCatalogContext)(HCATADMIN, HCATINFO, DWORD);
static HANDLE (WINAPI * pCryptCATOpen)(LPWSTR, DWORD, HCRYPTPROV, DWORD, DWORD);
static BOOL (WINAPI * pCryptCATCatalogInfoFromContext)(HCATINFO, CATALOG_INFO *, DWORD);
static CRYPTCATMEMBER * (WINAPI * pCryptCATEnumerateMember)(HANDLE, CRYPTCATMEMBER *);
static CRYPTCATATTRIBUTE * (WINAPI * pCryptCATEnumerateAttr)(HANDLE, CRYPTCATMEMBER *, CRYPTCATATTRIBUTE *);
static BOOL (WINAPI * pCryptCATClose)(HANDLE);

static void InitFunctionPtrs(void)
{
    HMODULE hWintrust = GetModuleHandleA("wintrust.dll");

#define WINTRUST_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hWintrust, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
    }

    WINTRUST_GET_PROC(CryptCATAdminAcquireContext)
    WINTRUST_GET_PROC(CryptCATAdminReleaseContext)
    WINTRUST_GET_PROC(CryptCATAdminCalcHashFromFileHandle)
    WINTRUST_GET_PROC(CryptCATAdminAddCatalog)
    WINTRUST_GET_PROC(CryptCATAdminRemoveCatalog)
    WINTRUST_GET_PROC(CryptCATAdminReleaseCatalogContext)
    WINTRUST_GET_PROC(CryptCATOpen)
    WINTRUST_GET_PROC(CryptCATCatalogInfoFromContext)
    WINTRUST_GET_PROC(CryptCATEnumerateMember)
    WINTRUST_GET_PROC(CryptCATEnumerateAttr)
    WINTRUST_GET_PROC(CryptCATClose)

#undef WINTRUST_GET_PROC
}

static GUID dummy = {0xdeadbeef,0xdead,0xbeef,{0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef}};

static void test_context(void)
{
    BOOL ret;
    HCATADMIN hca;
    static GUID unknown = { 0xC689AABA, 0x8E78, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }}; /* WINTRUST.DLL */
    CHAR dummydir[MAX_PATH];
    DWORD attrs;

    /* When CryptCATAdminAcquireContext is successful it will create
     * several directories if they don't exist:
     *
     * ...\system32\CatRoot\{GUID}, this directory holds the .cat files
     * ...\system32\CatRoot2\{GUID}  (WinXP and up), here we find the catalog database for that GUID
     *
     * Windows Vista uses lowercase catroot and catroot2.
     *
     * When passed a NULL GUID it will create the following directories although on
     * WinXP and up these directories are already present when Windows is installed:
     *
     * ...\system32\CatRoot\{127D0A1D-4EF2-11D1-8608-00C04FC295EE}
     * ...\system32\CatRoot2\{127D0A1D-4EF2-11D1-8608-00C04FC295EE} (WinXP up)
     *
     * TODO: Find out what this GUID is/does.
     *
     * On WinXP and up there is also a TimeStamp file in some of directories that
     * seem to indicate the last change to the catalog database for that GUID.
     *
     * On Windows 2000 some files are created/updated:
     *
     * ...\system32\CatRoot\SYSMAST.cbk
     * ...\system32\CatRoot\SYSMAST.cbd
     * ...\system32\CatRoot\{GUID}\CATMAST.cbk
     * ...\system32\CatRoot\{GUID}\CATMAST.cbd
     *
     */

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(NULL, NULL, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* NULL GUID */
    ret = pCryptCATAdminAcquireContext(&hca, NULL, 0);
    ok(ret, "Expected success\n");
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(NULL, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Proper release */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected no change in last error, got %d\n", GetLastError());

    /* Try to release a second time */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* NULL context handle and dummy GUID */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(NULL, &dummy, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Correct context handle and dummy GUID
     *
     * The tests run in the past unfortunately made sure that some directories were created.
     *
     * FIXME:
     * We don't want to mess too much with these for now so we should delete only the ones
     * that shouldn't be there like the deadbeef ones. We first have to figure out if it's
     * save to remove files and directories from CatRoot/CatRoot2.
     */

    ret = pCryptCATAdminAcquireContext(&hca, &dummy, 0);
    ok(ret, "Expected success\n");
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    attrs = GetFileAttributes(catroot);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Expected the CatRoot directory to exist\n");

    /* Windows creates the GUID directory in capitals */
    lstrcpyA(dummydir, catroot);
    lstrcatA(dummydir, "\\{DEADBEEF-DEAD-BEEF-DEAD-BEEFDEADBEEF}");
    attrs = GetFileAttributes(dummydir);
    ok(attrs != INVALID_FILE_ATTRIBUTES,
       "Expected CatRoot\\{DEADBEEF-DEAD-BEEF-DEAD-BEEFDEADBEEF} directory to exist\n");

    /* Only present on XP or higher. */
    attrs = GetFileAttributes(catroot2);
    if (attrs != INVALID_FILE_ATTRIBUTES)
    {
        lstrcpyA(dummydir, catroot2);
        lstrcatA(dummydir, "\\{DEADBEEF-DEAD-BEEF-DEAD-BEEFDEADBEEF}");
        attrs = GetFileAttributes(dummydir);
        ok(attrs != INVALID_FILE_ATTRIBUTES,
            "Expected CatRoot2\\{DEADBEEF-DEAD-BEEF-DEAD-BEEFDEADBEEF} directory to exist\n");
    }

    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");

    /* Correct context handle and GUID */
    ret = pCryptCATAdminAcquireContext(&hca, &unknown, 0);
    ok(ret, "Expected success\n");
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");

    /* Flags not equal to 0 */
    ret = pCryptCATAdminAcquireContext(&hca, &unknown, 1);
    ok(ret, "Expected success\n");
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");
}

/* TODO: Check whether SHA-1 is the algorithm that's always used */
static void test_calchash(void)
{
    BOOL ret;
    HANDLE file;
    DWORD hashsize = 0;
    BYTE* hash;
    BYTE expectedhash[20] = {0x3a,0xa1,0x19,0x08,0xec,0xa6,0x0d,0x2e,0x7e,0xcc,0x7a,0xca,0xf5,0xb8,0x2e,0x62,0x6a,0xda,0xf0,0x19};
    CHAR temp[MAX_PATH];
    DWORD written;

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(NULL, NULL, NULL, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* NULL filehandle, rest is legal */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(NULL, &hashsize, NULL, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Correct filehandle, rest is NULL */
    file = CreateFileA(selfname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, NULL, NULL, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    CloseHandle(file);

    /* All OK, but dwFlags set to 1 */
    file = CreateFileA(selfname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, NULL, 1);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    CloseHandle(file);

    /* All OK, requesting the size of the hash */
    file = CreateFileA(selfname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed %u\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, NULL, 0);
    ok(ret, "Expected success %u\n", GetLastError());
    ok(hashsize == 20," Expected a hash size of 20, got %d\n", hashsize);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    CloseHandle(file);

    /* All OK, retrieve the hash
     * Double the hash buffer to see what happens to the size parameter
     */
    file = CreateFileA(selfname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    hashsize *= 2;
    hash = HeapAlloc(GetProcessHeap(), 0, hashsize);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, hash, 0);
    ok(ret, "Expected success %u\n", GetLastError());
    ok(hashsize == 20," Expected a hash size of 20, got %d\n", hashsize);
    ok(GetLastError() == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    CloseHandle(file);
    HeapFree(GetProcessHeap(), 0, hash);

    /* Do the same test with a file created and filled by ourselves (and we thus
     * have a known hash for).
     */
    GetTempFileNameA(CURR_DIR, "hsh", 0, temp); 
    file = CreateFileA(temp, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    WriteFile(file, "Text in this file is needed to create a know hash", 49, &written, NULL);
    CloseHandle(file);

    /* All OK, first request the size and then retrieve the hash */
    file = CreateFileA(temp, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    hashsize = 0;
    pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, NULL, 0);
    hash = HeapAlloc(GetProcessHeap(), 0, hashsize);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, hash, 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok(hashsize == sizeof(expectedhash) &&
       !memcmp(hash, expectedhash, sizeof(expectedhash)),
       "Hashes didn't match\n");
    CloseHandle(file);

    HeapFree(GetProcessHeap(), 0, hash);
    DeleteFileA(temp);
}

static void test_CryptCATAdminAddRemoveCatalog(void)
{
    static WCHAR basenameW[] = {'w','i','n','e','t','e','s','t','.','c','a','t',0};
    static CHAR basename[] = "winetest.cat";
    HCATADMIN hcatadmin;
    HCATINFO hcatinfo;
    CATALOG_INFO info;
    WCHAR tmpfileW[MAX_PATH];
    char tmpfile[MAX_PATH];
    char catfile[MAX_PATH], catfilepath[MAX_PATH], *p;
    WCHAR catfileW[MAX_PATH];
    HANDLE file;
    DWORD error, written;
    BOOL ret;
    DWORD attrs;

    if (!pCryptCATAdminRemoveCatalog)
    {
        /* NT4 and W2K do have CryptCATAdminAddCatalog !! */
        win_skip("CryptCATAdminRemoveCatalog is not available\n");
        return;
    }

    if (!GetTempFileNameA(CURR_DIR, "cat", 0, tmpfile)) return;
    DeleteFileA(tmpfile);
    file = CreateFileA(tmpfile, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed %u\n", GetLastError());
    CloseHandle(file);

    ret = pCryptCATAdminAcquireContext(&hcatadmin, &dummy, 0);
    ok(ret, "CryptCATAdminAcquireContext failed %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    hcatinfo = pCryptCATAdminAddCatalog(NULL, NULL, NULL, 0);
    error = GetLastError();
    ok(hcatinfo == NULL, "CryptCATAdminAddCatalog succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u expected ERROR_INVALID_PARAMTER\n", GetLastError());

    SetLastError(0xdeadbeef);
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, NULL, NULL, 0);
    error = GetLastError();
    ok(hcatinfo == NULL, "CryptCATAdminAddCatalog succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u expected INVALID_PARAMTER\n", GetLastError());

    MultiByteToWideChar(0, 0, tmpfile, -1, tmpfileW, MAX_PATH);

    SetLastError(0xdeadbeef);
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, tmpfileW, basenameW, 0);
    error = GetLastError();
    todo_wine {
    ok(hcatinfo == NULL, "CryptCATAdminAddCatalog succeeded\n");
    ok(error == ERROR_BAD_FORMAT, "got %u expected ERROR_BAD_FORMAT\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, tmpfileW, basenameW, 1);
    error = GetLastError();
    ok(hcatinfo == NULL, "CryptCATAdminAddCatalog succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u expected ERROR_INVALID_PARAMTER\n", GetLastError());

    SetLastError(0xdeadbeef);
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, tmpfileW, NULL, 0);
    error = GetLastError();
    ok(hcatinfo == NULL, "CryptCATAdminAddCatalog succeeded\n");
    todo_wine ok(error == ERROR_BAD_FORMAT, "got %u expected ERROR_BAD_FORMAT\n", GetLastError());

    DeleteFileA(tmpfile);
    file = CreateFileA(tmpfile, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed %u\n", GetLastError());
    WriteFile(file, test_catalog, sizeof(test_catalog), &written, NULL);
    CloseHandle(file);

    /* Unique name will be created */
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, tmpfileW, NULL, 0);
    todo_wine ok(hcatinfo != NULL, "CryptCATAdminAddCatalog failed %u\n", GetLastError());

    info.cbStruct = sizeof(info);
    info.wszCatalogFile[0] = 0;
    ret = pCryptCATCatalogInfoFromContext(hcatinfo, &info, 0);
    todo_wine
    {
    ok(ret, "CryptCATCatalogInfoFromContext failed %u\n", GetLastError());
    ok(info.wszCatalogFile[0] != 0, "Expected a filename\n");
    }
    WideCharToMultiByte(CP_ACP, 0, info.wszCatalogFile, -1, catfile, MAX_PATH, 0, 0);
    if ((p = strrchr(catfile, '\\'))) p++;
    memset(catfileW, 0, sizeof(catfileW));
    MultiByteToWideChar(0, 0, p, -1, catfileW, MAX_PATH);

    /* winetest.cat will be created */
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, tmpfileW, basenameW, 0);
    ok(hcatinfo != NULL, "CryptCATAdminAddCatalog failed %u\n", GetLastError());

    lstrcpyA(catfilepath, catroot);
    lstrcatA(catfilepath, "\\{DEADBEEF-DEAD-BEEF-DEAD-BEEFDEADBEEF}\\winetest.cat");
    attrs = GetFileAttributes(catfilepath);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Expected %s to exist\n", catfilepath);

    info.cbStruct = sizeof(info);
    info.wszCatalogFile[0] = 0;
    ret = pCryptCATCatalogInfoFromContext(hcatinfo, &info, 0);
    ok(ret, "CryptCATCatalogInfoFromContext failed %u\n", GetLastError());
    ok(info.wszCatalogFile[0] != 0, "Expected a filename\n");
    WideCharToMultiByte(CP_ACP, 0, info.wszCatalogFile, -1, catfile, MAX_PATH, 0, 0);
    if ((p = strrchr(catfile, '\\'))) p++;
    ok(!lstrcmpA(basename, p), "Expected %s, got %s\n", basename, p);

    ret = pCryptCATAdminReleaseCatalogContext(hcatadmin, hcatinfo, 0);
    ok(ret, "CryptCATAdminReleaseCatalogContext failed %u\n", GetLastError());

    /* Remove the catalog file with the unique name */
    ret = pCryptCATAdminRemoveCatalog(hcatadmin, catfileW, 0);
    ok(ret, "CryptCATAdminRemoveCatalog failed %u\n", GetLastError());

    /* Remove the winetest.cat catalog file, first with the full path. This should not succeed
     * according to MSDN */
    ret = pCryptCATAdminRemoveCatalog(hcatadmin, info.wszCatalogFile, 0);
    ok(ret, "CryptCATAdminRemoveCatalog failed %u\n", GetLastError());
    /* The call succeeded with the full path but the file is not removed */
    attrs = GetFileAttributes(catfilepath);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Expected %s to exist\n", catfilepath);
    /* Given only the filename the file is removed */
    ret = pCryptCATAdminRemoveCatalog(hcatadmin, basenameW, 0);
    ok(ret, "CryptCATAdminRemoveCatalog failed %u\n", GetLastError());
    attrs = GetFileAttributes(catfilepath);
    ok(attrs == INVALID_FILE_ATTRIBUTES, "Expected %s to be removed\n", catfilepath);

    ret = pCryptCATAdminReleaseContext(hcatadmin, 0);
    ok(ret, "CryptCATAdminReleaseContext failed %u\n", GetLastError());

    DeleteFileA(tmpfile);
}

static void test_catalog_properties(void)
{
    static const WCHAR hashmeW[] = {'h','a','s','h','m','e',0};
    static const GUID subject = {0xde351a42,0x8e59,0x11d0,{0x8c,0x47,0x00,0xc0,0x4f,0xc2,0x95,0xee}};

    HANDLE hcat;
    CRYPTCATMEMBER *m;
    CRYPTCATATTRIBUTE *attr;
    char catalog[MAX_PATH];
    WCHAR catalogW[MAX_PATH];
    DWORD written;
    HANDLE file;
    BOOL ret;

    if (!GetTempFileNameA(CURR_DIR, "cat", 0, catalog)) return;
    file = CreateFileA(catalog, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed %u\n", GetLastError());
    WriteFile(file, test_catalog, sizeof(test_catalog), &written, NULL);
    CloseHandle(file);

    hcat = pCryptCATOpen(NULL, 0, 0, 0, 0);
    ok(hcat == INVALID_HANDLE_VALUE, "CryptCATOpen succeeded\n");

    MultiByteToWideChar(CP_ACP, 0, catalog, -1, catalogW, MAX_PATH);

    hcat = pCryptCATOpen(catalogW, 0, 0, 0, 0);
    ok(hcat != INVALID_HANDLE_VALUE, "CryptCATOpen failed %u\n", GetLastError());

    m = pCryptCATEnumerateMember(NULL, NULL);
    ok(m == NULL, "CryptCATEnumerateMember succeeded\n");

    m = pCryptCATEnumerateMember(hcat, NULL);
    ok(m != NULL, "CryptCATEnumerateMember failed %u\n", GetLastError());

    ok(m->cbStruct == sizeof(CRYPTCATMEMBER), "unexpected size %u\n", m->cbStruct);
    todo_wine ok(!lstrcmpW(m->pwszReferenceTag, hashmeW), "unexpected tag\n");
    ok(!memcmp(&m->gSubjectType, &subject, sizeof(subject)), "guid differs\n");
    ok(!m->fdwMemberFlags, "got %x expected 0\n", m->fdwMemberFlags);
    ok(m->dwCertVersion == 0x200, "got %x expected 0x200\n", m->dwCertVersion);
    ok(!m->dwReserved, "got %x expected 0\n", m->dwReserved);
    ok(m->hReserved == NULL, "got %p expected NULL\n", m->hReserved);

    attr = pCryptCATEnumerateAttr(NULL, NULL, NULL);
    ok(attr == NULL, "CryptCATEnumerateAttr succeeded\n");

    attr = pCryptCATEnumerateAttr(hcat, NULL, NULL);
    ok(attr == NULL, "CryptCATEnumerateAttr succeeded\n");

    attr = pCryptCATEnumerateAttr(hcat, m, NULL);
    ok(attr == NULL, "CryptCATEnumerateAttr succeeded\n");

    m = pCryptCATEnumerateMember(hcat, m);
    ok(m == NULL, "CryptCATEnumerateMember succeeded\n");

    ret = pCryptCATClose(hcat);
    ok(ret, "CryptCATClose failed\n");

    DeleteFileA(catalog);
}

START_TEST(crypt)
{
    int myARGC;
    char** myARGV;
    char windir[MAX_PATH];

    InitFunctionPtrs();

    if (!pCryptCATAdminAcquireContext)
    {
        win_skip("CryptCATAdmin functions are not available\n");
        return;
    }

    GetWindowsDirectoryA(windir, MAX_PATH);
    lstrcpyA(catroot, windir);
    lstrcatA(catroot, "\\system32\\CatRoot");
    lstrcpyA(catroot2, windir);
    lstrcatA(catroot2, "\\system32\\CatRoot2");

    myARGC = winetest_get_mainargs(&myARGV);
    strcpy(selfname, myARGV[0]);

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);
   
    test_context();
    test_calchash();
    test_CryptCATAdminAddRemoveCatalog();
    test_catalog_properties();
}
