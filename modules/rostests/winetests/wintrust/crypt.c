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
#include "wincrypt.h"
#include "mscat.h"

#include "wine/test.h"

static char selfname[MAX_PATH];
static CHAR CURR_DIR[MAX_PATH];
static CHAR catroot[MAX_PATH];
static CHAR catroot2[MAX_PATH];

static const WCHAR hashmeW[] = {'h','a','s','h','m','e',0};
static const WCHAR attr1W[] = {'a','t','t','r','1',0};
static const WCHAR attr2W[] = {'a','t','t','r','2',0};

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

static const CHAR test_cdf[] =
    "[CatalogHeader]\r\n"
    "Name=winetest.cat\r\n"
    "ResultDir=.\\\r\n"
    "PublicVersion=0x00000001\r\n"
    "EncodingType=\r\n"
    "CATATTR1=0x10010001:attr1:value1\r\n"
    "CATATTR2=0x10010001:attr2:value2\r\n"
    "\r\n"
    "[CatalogFiles]\r\n"
    "hashme=.\\winetest.cdf\r\n";

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
static BOOL (WINAPI * pCryptCATCDFClose)(CRYPTCATCDF *);
static CRYPTCATATTRIBUTE * (WINAPI * pCryptCATCDFEnumCatAttributes)(CRYPTCATCDF *, CRYPTCATATTRIBUTE *,
                                                                    PFN_CDF_PARSE_ERROR_CALLBACK);
static LPWSTR (WINAPI * pCryptCATCDFEnumMembersByCDFTagEx)(CRYPTCATCDF *, LPWSTR, PFN_CDF_PARSE_ERROR_CALLBACK,
                                                           CRYPTCATMEMBER **, BOOL, LPVOID);
static CRYPTCATCDF * (WINAPI * pCryptCATCDFOpen)(LPWSTR, PFN_CDF_PARSE_ERROR_CALLBACK);
static CRYPTCATATTRIBUTE * (WINAPI * pCryptCATEnumerateCatAttr)(HANDLE, CRYPTCATATTRIBUTE *);
static CRYPTCATMEMBER * (WINAPI * pCryptCATEnumerateMember)(HANDLE, CRYPTCATMEMBER *);
static CRYPTCATATTRIBUTE * (WINAPI * pCryptCATEnumerateAttr)(HANDLE, CRYPTCATMEMBER *, CRYPTCATATTRIBUTE *);
static BOOL (WINAPI * pCryptCATClose)(HANDLE);
static pCryptSIPGetSignedDataMsg pGetSignedDataMsg;
static pCryptSIPPutSignedDataMsg pPutSignedDataMsg;

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
    WINTRUST_GET_PROC(CryptCATCDFClose)
    WINTRUST_GET_PROC(CryptCATCDFEnumCatAttributes)
    WINTRUST_GET_PROC(CryptCATCDFEnumMembersByCDFTagEx)
    WINTRUST_GET_PROC(CryptCATCDFOpen)
    WINTRUST_GET_PROC(CryptCATEnumerateCatAttr)
    WINTRUST_GET_PROC(CryptCATEnumerateMember)
    WINTRUST_GET_PROC(CryptCATEnumerateAttr)
    WINTRUST_GET_PROC(CryptCATClose)

#undef WINTRUST_GET_PROC

    pGetSignedDataMsg = (void*)GetProcAddress(hWintrust, "CryptSIPGetSignedDataMsg");
    if(!pGetSignedDataMsg)
        trace("GetProcAddress(CryptSIPGetSignedDataMsg) failed\n");

    pPutSignedDataMsg = (void*)GetProcAddress(hWintrust, "CryptSIPPutSignedDataMsg");
    if(!pPutSignedDataMsg)
        trace("GetProcAddress(CryptSIPPutSignedDataMsg) failed\n");
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
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* NULL GUID */
    if (0) { /* crashes on 64-bit win10 */
    ret = pCryptCATAdminAcquireContext(&hca, NULL, 0);
    ok(ret, "Expected success, got FALSE with %ld\n", GetLastError());
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    /* Proper release */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success, got FALSE with %ld\n", GetLastError());

    /* Try to release a second time */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    }

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(NULL, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* NULL context handle and dummy GUID */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(NULL, &dummy, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* Correct context handle and dummy GUID
     *
     * The tests run in the past unfortunately made sure that some directories were created.
     *
     * FIXME:
     * We don't want to mess too much with these for now so we should delete only the ones
     * that shouldn't be there like the deadbeef ones. We first have to figure out if it's
     * safe to remove files and directories from CatRoot/CatRoot2.
     */

    ret = pCryptCATAdminAcquireContext(&hca, &dummy, 0);
    ok(ret || GetLastError() == ERROR_ACCESS_DENIED, "CryptCATAdminAcquireContext failed %lu\n", GetLastError());
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        win_skip("Not running as administrator\n");
        return;
    }
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    attrs = GetFileAttributesA(catroot);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Expected the CatRoot directory to exist\n");

    /* Windows creates the GUID directory in capitals */
    lstrcpyA(dummydir, catroot);
    lstrcatA(dummydir, "\\{DEADBEEF-DEAD-BEEF-DEAD-BEEFDEADBEEF}");
    attrs = GetFileAttributesA(dummydir);
    ok(attrs != INVALID_FILE_ATTRIBUTES,
       "Expected CatRoot\\{DEADBEEF-DEAD-BEEF-DEAD-BEEFDEADBEEF} directory to exist\n");

    /* Only present on XP or higher. */
    attrs = GetFileAttributesA(catroot2);
    if (attrs != INVALID_FILE_ATTRIBUTES)
    {
        lstrcpyA(dummydir, catroot2);
        lstrcatA(dummydir, "\\{DEADBEEF-DEAD-BEEF-DEAD-BEEFDEADBEEF}");
        attrs = GetFileAttributesA(dummydir);
        ok(attrs != INVALID_FILE_ATTRIBUTES,
            "Expected CatRoot2\\{DEADBEEF-DEAD-BEEF-DEAD-BEEFDEADBEEF} directory to exist\n");
    }

    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success, got FALSE with %ld\n", GetLastError());

    /* Correct context handle and GUID */
    ret = pCryptCATAdminAcquireContext(&hca, &unknown, 0);
    ok(ret, "Expected success, got FALSE with %ld\n", GetLastError());
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success, got FALSE with %ld\n", GetLastError());

    hca = (void *) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    /* Flags is documented as unused, but the parameter is checked since win8 */
    ret = pCryptCATAdminAcquireContext(&hca, &unknown, 1);
    ok((!ret && (GetLastError() == ERROR_INVALID_PARAMETER) && (hca == (void *) 0xdeadbeef)) ||
        broken(ret && hca != NULL && hca != (void *) 0xdeadbeef),
        "Expected FALSE and ERROR_INVALID_PARAMETER with untouched handle, got %d and %lu with %p\n",
        ret, GetLastError(), hca);

    if (ret && hca)
    {
        SetLastError(0xdeadbeef);
        ret = pCryptCATAdminReleaseContext(hca, 0);
        ok(ret, "Expected success, got FALSE with %ld\n", GetLastError());
    }
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
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* NULL filehandle, rest is legal */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(NULL, &hashsize, NULL, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* Correct filehandle, rest is NULL */
    file = CreateFileA(selfname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, NULL, NULL, 0);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    CloseHandle(file);

    /* All OK, but dwFlags set to 1 */
    file = CreateFileA(selfname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, NULL, 1);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    CloseHandle(file);

    /* All OK, requesting the size of the hash */
    file = CreateFileA(selfname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed %lu\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, NULL, 0);
    ok(ret, "Expected success, got FALSE with %ld\n", GetLastError());
    ok(hashsize == 20," Expected a hash size of 20, got %ld\n", hashsize);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    CloseHandle(file);

    /* All OK, retrieve the hash
     * Double the hash buffer to see what happens to the size parameter
     */
    file = CreateFileA(selfname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    hashsize *= 2;
    hash = HeapAlloc(GetProcessHeap(), 0, hashsize);
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminCalcHashFromFileHandle(file, &hashsize, hash, 0);
    ok(ret, "Expected success, got FALSE with %ld\n", GetLastError());
    ok(hashsize == 20," Expected a hash size of 20, got %ld\n", hashsize);
    ok(GetLastError() == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
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
    ok(ret, "Expected success, got FALSE with %ld\n", GetLastError());
    ok(GetLastError() == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    ok(hashsize == sizeof(expectedhash) &&
       !memcmp(hash, expectedhash, sizeof(expectedhash)),
       "Hashes didn't match\n");
    CloseHandle(file);

    HeapFree(GetProcessHeap(), 0, hash);
    DeleteFileA(temp);
}

static void test_CryptCATOpen(void)
{
    WCHAR filename[MAX_PATH], temp_path[MAX_PATH];
    HANDLE cat;
    DWORD flags;
    BOOL ret;
    FILE *file;
    char buffer[10];

    GetTempPathW(ARRAY_SIZE(temp_path), temp_path);
    GetTempFileNameW(temp_path, L"cat", 0, filename);

    SetLastError(0xdeadbeef);
    cat = pCryptCATOpen(NULL, 0, 0, 0, 0);
    ok(cat == INVALID_HANDLE_VALUE, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());

    for (flags = 0; flags < 8; ++flags)
    {
        SetLastError(0xdeadbeef);
        cat = pCryptCATOpen(filename, flags, 0, 0, 0);
        if (flags == CRYPTCAT_OPEN_EXISTING)
        {
            ok(cat == INVALID_HANDLE_VALUE, "flags %#lx: expected failure\n", flags);
            ok(GetLastError() == ERROR_FILE_NOT_FOUND, "flags %#lx: got error %lu\n", flags, GetLastError());
            ret = DeleteFileW(filename);
            ok(!ret, "flags %#lx: expected failure\n", flags);
        }
        else
        {
            ok(cat != INVALID_HANDLE_VALUE, "flags %#lx: expected success\n", flags);
            ok(!GetLastError(), "flags %#lx: got error %lu\n", flags, GetLastError());
            ret = pCryptCATClose(cat);
            ok(ret, "flags %#lx: failed to close file\n", flags);
            ret = DeleteFileW(filename);
            ok(ret, "flags %#lx: failed to delete file, error %lu\n", flags, GetLastError());
        }

        file = _wfopen(filename, L"w");
        fputs("test text", file);
        fclose(file);

        SetLastError(0xdeadbeef);
        cat = pCryptCATOpen(filename, flags, 0, 0, 0);
        ok(cat != INVALID_HANDLE_VALUE, "flags %#lx: expected success\n", flags);
        ok(!GetLastError(), "flags %#lx: got error %lu\n", flags, GetLastError());
        ret = pCryptCATClose(cat);
        ok(ret, "flags %#lx: failed to close file\n", flags);

        file = _wfopen(filename, L"r");
        ret = fread(buffer, 1, sizeof(buffer), file);
        if (flags & CRYPTCAT_OPEN_CREATENEW)
            ok(!ret, "flags %#lx: got %s\n", flags, debugstr_an(buffer, ret));
        else
            ok(ret == 9 && !strncmp(buffer, "test text", ret), "flags %#lx: got %s\n", flags, debugstr_an(buffer, ret));
        fclose(file);

        ret = DeleteFileW(filename);
        ok(ret, "flags %#lx: failed to delete file, error %lu\n", flags, GetLastError());
    }
}

static DWORD error_area;
static DWORD local_error;

static void WINAPI cdf_callback(DWORD area, DWORD error, WCHAR* line)
{
    ok(error_area != -2, "Didn't expect cdf_callback() to be called (%08lx, %08lx)\n",
       area, error);

    error_area = area;
    local_error = error;
}

static void test_CryptCATCDF_params(void)
{
    static WCHAR nonexistent[] = {'d','e','a','d','b','e','e','f','.','c','d','f',0};
    CRYPTCATCDF *catcdf;
    BOOL ret;

    if (!pCryptCATCDFOpen)
    {
        win_skip("CryptCATCDFOpen is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(NULL, NULL);
    ok(catcdf == NULL, "CryptCATCDFOpen succeeded\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(NULL, cdf_callback);
    ok(catcdf == NULL, "CryptCATCDFOpen succeeded\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* File doesn't exist */
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(nonexistent, cdf_callback);
    ok(catcdf == NULL, "CryptCATCDFOpen succeeded\n");
    todo_wine
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pCryptCATCDFClose(NULL);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    catcdf = NULL;
    SetLastError(0xdeadbeef);
    ret = pCryptCATCDFClose(catcdf);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
}

/* FIXME: Once Wine can create catalog files we should use the created catalog file in this test */
static void test_CryptCATAdminAddRemoveCatalog(void)
{
    static WCHAR basenameW[] = {'w','i','n','e','t','e','s','t','.','c','a','t',0};
    static const char basename[] = "winetest.cat";
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
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed %lu\n", GetLastError());
    CloseHandle(file);

    ret = pCryptCATAdminAcquireContext(&hcatadmin, &dummy, 0);
    ok(ret || GetLastError() == ERROR_ACCESS_DENIED, "CryptCATAdminAcquireContext failed %lu\n", GetLastError());
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        win_skip("Not running as administrator\n");
        return;
    }

    SetLastError(0xdeadbeef);
    hcatinfo = pCryptCATAdminAddCatalog(NULL, NULL, NULL, 0);
    error = GetLastError();
    ok(hcatinfo == NULL, "CryptCATAdminAddCatalog succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %lu expected ERROR_INVALID_PARAMETER\n", GetLastError());

    SetLastError(0xdeadbeef);
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, NULL, NULL, 0);
    error = GetLastError();
    ok(hcatinfo == NULL, "CryptCATAdminAddCatalog succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %lu expected ERROR_INVALID_PARAMETER\n", GetLastError());

    MultiByteToWideChar(CP_ACP, 0, tmpfile, -1, tmpfileW, MAX_PATH);

    SetLastError(0xdeadbeef);
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, tmpfileW, basenameW, 0);
    error = GetLastError();
    todo_wine {
    ok(hcatinfo == NULL, "CryptCATAdminAddCatalog succeeded\n");
    ok(error == ERROR_BAD_FORMAT, "got %lu expected ERROR_BAD_FORMAT\n", GetLastError());
    }
    if (hcatinfo != NULL)
        pCryptCATAdminReleaseCatalogContext(hcatadmin, hcatinfo, 0);

    SetLastError(0xdeadbeef);
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, tmpfileW, basenameW, 1);
    error = GetLastError();
    ok(hcatinfo == NULL, "CryptCATAdminAddCatalog succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER ||
       error == ERROR_BAD_FORMAT, /* win 8 */
       "got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, tmpfileW, NULL, 0);
    error = GetLastError();
    ok(hcatinfo == NULL, "CryptCATAdminAddCatalog succeeded\n");
    todo_wine ok(error == ERROR_BAD_FORMAT, "got %lu expected ERROR_BAD_FORMAT\n", GetLastError());

    DeleteFileA(tmpfile);
    file = CreateFileA(tmpfile, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed %lu\n", GetLastError());
    WriteFile(file, test_catalog, sizeof(test_catalog), &written, NULL);
    CloseHandle(file);

    /* Unique name will be created */
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, tmpfileW, NULL, 0);
    if (!hcatinfo && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        win_skip("Not enough rights\n");
        goto cleanup;
    }
    todo_wine ok(hcatinfo != NULL, "CryptCATAdminAddCatalog failed %lu\n", GetLastError());

    info.cbStruct = sizeof(info);
    info.wszCatalogFile[0] = 0;
    ret = pCryptCATCatalogInfoFromContext(hcatinfo, &info, 0);
    todo_wine
    {
    ok(ret, "CryptCATCatalogInfoFromContext failed %lu\n", GetLastError());
    ok(info.wszCatalogFile[0] != 0, "Expected a filename\n");
    }
    WideCharToMultiByte(CP_ACP, 0, info.wszCatalogFile, -1, catfile, MAX_PATH, NULL, NULL);
    if ((p = strrchr(catfile, '\\'))) p++;
    memset(catfileW, 0, sizeof(catfileW));
    MultiByteToWideChar(CP_ACP, 0, p, -1, catfileW, MAX_PATH);

    /* Set the file attributes so we can check what happens with them during the 'copy' */
    attrs = FILE_ATTRIBUTE_READONLY;
    ret = SetFileAttributesA(tmpfile, attrs);
    ok(ret, "SetFileAttributesA failed : %lu\n", GetLastError());

    /* winetest.cat will be created */
    hcatinfo = pCryptCATAdminAddCatalog(hcatadmin, tmpfileW, basenameW, 0);
    ok(hcatinfo != NULL, "CryptCATAdminAddCatalog failed %lu\n", GetLastError());

    lstrcpyA(catfilepath, catroot);
    lstrcatA(catfilepath, "\\{DEADBEEF-DEAD-BEEF-DEAD-BEEFDEADBEEF}\\winetest.cat");
    attrs = GetFileAttributesA(catfilepath);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Expected %s to exist\n", catfilepath);
    todo_wine
    ok(attrs == FILE_ATTRIBUTE_SYSTEM ||
       attrs == (FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_SYSTEM), /* Vista */
       "File has wrong attributes : %08lx\n", attrs);

    info.cbStruct = sizeof(info);
    info.wszCatalogFile[0] = 0;
    ret = pCryptCATCatalogInfoFromContext(hcatinfo, &info, 0);
    ok(ret, "CryptCATCatalogInfoFromContext failed %lu\n", GetLastError());
    ok(info.wszCatalogFile[0] != 0, "Expected a filename\n");
    WideCharToMultiByte(CP_ACP, 0, info.wszCatalogFile, -1, catfile, MAX_PATH, NULL, NULL);
    if ((p = strrchr(catfile, '\\'))) p++;
    ok(!lstrcmpA(basename, p), "Expected %s, got %s\n", basename, p);

    ret = pCryptCATAdminReleaseCatalogContext(hcatadmin, hcatinfo, 0);
    ok(ret, "CryptCATAdminReleaseCatalogContext failed %lu\n", GetLastError());

    /* Remove the catalog file with the unique name */
    ret = pCryptCATAdminRemoveCatalog(hcatadmin, catfileW, 0);
    ok(ret, "CryptCATAdminRemoveCatalog failed %lu\n", GetLastError());

    /* Remove the winetest.cat catalog file, first with the full path. This should not succeed
     * according to MSDN */
    ret = pCryptCATAdminRemoveCatalog(hcatadmin, info.wszCatalogFile, 0);
    ok(ret, "CryptCATAdminRemoveCatalog failed %lu\n", GetLastError());
    /* The call succeeded with the full path but the file is not removed */
    attrs = GetFileAttributesA(catfilepath);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Expected %s to exist\n", catfilepath);
    /* Given only the filename the file is removed */
    ret = pCryptCATAdminRemoveCatalog(hcatadmin, basenameW, 0);
    ok(ret, "CryptCATAdminRemoveCatalog failed %lu\n", GetLastError());
    attrs = GetFileAttributesA(catfilepath);
    ok(attrs == INVALID_FILE_ATTRIBUTES, "Expected %s to be removed\n", catfilepath);

cleanup:
    ret = pCryptCATAdminReleaseContext(hcatadmin, 0);
    ok(ret, "CryptCATAdminReleaseContext failed %lu\n", GetLastError());

    /* Set the attributes so we can delete the file */
    attrs = FILE_ATTRIBUTE_NORMAL;
    ret = SetFileAttributesA(tmpfile, attrs);
    ok(ret, "SetFileAttributesA failed %lu\n", GetLastError());
    DeleteFileA(tmpfile);
}

static void test_catalog_properties(const char *catfile, int attributes, int members)
{
    static const GUID subject = {0xde351a42,0x8e59,0x11d0,{0x8c,0x47,0x00,0xc0,0x4f,0xc2,0x95,0xee}};

    HANDLE hcat;
    CRYPTCATMEMBER *m;
    CRYPTCATATTRIBUTE *attr;
    char catalog[MAX_PATH];
    WCHAR catalogW[MAX_PATH];
    DWORD attrs;
    BOOL ret;
    int attrcount = 0, membercount = 0;

    /* FIXME: Wine can't create catalog files out of catalog definition files yet. Remove this piece
     * once wine is fixed
     */
    attrs = GetFileAttributesA(catfile);
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        HANDLE file;
        DWORD written;

        trace("Creating the catalog file\n");
        if (!GetTempFileNameA(CURR_DIR, "cat", 0, catalog)) return;
        file = CreateFileA(catalog, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed %lu\n", GetLastError());
        WriteFile(file, test_catalog, sizeof(test_catalog), &written, NULL);
        CloseHandle(file);

        attributes = 2;
        members = 1;
        MultiByteToWideChar(CP_ACP, 0, catalog, -1, catalogW, MAX_PATH);
    }
    else
    {
        MultiByteToWideChar(CP_ACP, 0, catfile, -1, catalogW, MAX_PATH);
        catalog[0] = 0;
    }

    hcat = pCryptCATOpen(catalogW, 0, 0, 0, 0);
    if (hcat == INVALID_HANDLE_VALUE && members == 0)
    {
        win_skip("CryptCATOpen on W2K can't handle catalog files with no members\n");
        return;
    }
    ok(hcat != INVALID_HANDLE_VALUE, "CryptCATOpen failed %lu\n", GetLastError());

    m = pCryptCATEnumerateMember(NULL, NULL);
    ok(m == NULL, "CryptCATEnumerateMember succeeded\n");

    m = NULL;
    while ((m = pCryptCATEnumerateMember(hcat, m)))
    {
        ok(m->cbStruct == sizeof(CRYPTCATMEMBER), "unexpected size %lu\n", m->cbStruct);
        todo_wine ok(!lstrcmpW(m->pwszReferenceTag, hashmeW), "unexpected tag\n");
        ok(!memcmp(&m->gSubjectType, &subject, sizeof(subject)), "guid differs\n");
        ok(!m->fdwMemberFlags, "got %lx expected 0\n", m->fdwMemberFlags);
        ok(m->dwCertVersion == 0x200, "got %lx expected 0x200\n", m->dwCertVersion);
        ok(!m->dwReserved, "got %lx expected 0\n", m->dwReserved);
        ok(m->hReserved == NULL, "got %p expected NULL\n", m->hReserved);

        attr = pCryptCATEnumerateAttr(hcat, m, NULL);
        ok(attr == NULL, "CryptCATEnumerateAttr succeeded\n");

        membercount++;
    }
    ok(membercount == members, "Expected %d members, got %d\n", members, membercount);

    attr = pCryptCATEnumerateAttr(NULL, NULL, NULL);
    ok(attr == NULL, "CryptCATEnumerateAttr succeeded\n");

    attr = pCryptCATEnumerateAttr(hcat, NULL, NULL);
    ok(attr == NULL, "CryptCATEnumerateAttr succeeded\n");

    attr = NULL;
    while ((attr = pCryptCATEnumerateCatAttr(hcat, attr)))
    {
        ok(!lstrcmpW(attr->pwszReferenceTag, attr1W) ||
           !lstrcmpW(attr->pwszReferenceTag, attr2W),
           "Expected 'attr1' or 'attr2'\n");

        attrcount++;
    }
    todo_wine
    ok(attrcount == attributes, "Expected %d catalog attributes, got %d\n", attributes, attrcount);

    ret = pCryptCATClose(hcat);
    ok(ret, "CryptCATClose failed\n");
    if (catalog[0]) DeleteFileA( catalog );
}

static void test_create_catalog_file(void)
{
    static const char catfileA[] = "winetest.cat";
    static const char cdffileA[] = "winetest.cdf";
    static WCHAR cdffileW[] = {'w','i','n','e','t','e','s','t','.','c','d','f',0};
    CRYPTCATCDF *catcdf;
    CRYPTCATATTRIBUTE *catattr;
    CRYPTCATMEMBER *catmember;
    WCHAR  *catmembertag;
    DWORD written, attrs;
    HANDLE file;
    BOOL ret;
    int attrcount, membercount;

    if (!pCryptCATCDFOpen)
    {
        win_skip("CryptCATCDFOpen is not available\n");
        return;
    }

    /* Create the cdf file */
    file = CreateFileA(cdffileA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed %lu\n", GetLastError());
    WriteFile(file, test_cdf, sizeof(test_cdf) - 1, &written, NULL);
    CloseHandle(file);

    /* Don't enumerate attributes and members */
    trace("No attribs and members\n");
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, NULL);
    todo_wine
    {
    ok(catcdf != NULL, "CryptCATCDFOpen failed\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    }

    ret = pCryptCATCDFClose(catcdf);
    todo_wine
    {
    ok(ret, "Expected success, got FALSE with %ld\n", GetLastError());
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    }

    attrs = GetFileAttributesA(catfileA);
    todo_wine
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Expected the catalog file to exist\n");

    test_catalog_properties(catfileA, 0, 0);
    DeleteFileA(catfileA);

    /* Only enumerate the attributes */
    trace("Only attributes\n");
    attrcount = 0;
    catcdf = pCryptCATCDFOpen(cdffileW, NULL);

    catattr = NULL;
    while ((catattr = pCryptCATCDFEnumCatAttributes(catcdf, catattr, NULL)))
    {
        ok(!lstrcmpW(catattr->pwszReferenceTag, attr1W) ||
           !lstrcmpW(catattr->pwszReferenceTag, attr2W),
           "Expected 'attr1' or 'attr2'\n");

        attrcount++;
    }
    todo_wine
    ok(attrcount == 2, "Expected 2 attributes, got %d\n", attrcount);

    pCryptCATCDFClose(catcdf);
    /* Even though the resulting catalog file shows the attributes, they will not be enumerated */
    test_catalog_properties(catfileA, 0, 0);
    DeleteFileA(catfileA);

    /* Only enumerate the members */
    trace("Only members\n");
    membercount = 0;
    catcdf = pCryptCATCDFOpen(cdffileW, NULL);

    catmember = NULL;
    catmembertag = NULL;
    while ((catmembertag = pCryptCATCDFEnumMembersByCDFTagEx(catcdf, catmembertag, NULL, &catmember, FALSE, NULL)))
    {
        ok(!lstrcmpW(catmembertag, hashmeW), "Expected 'hashme'\n");
        membercount++;
    }
    todo_wine
    ok(membercount == 1, "Expected 1 member, got %d\n", membercount);

    pCryptCATCDFClose(catcdf);
    test_catalog_properties(catfileA, 0, 1);
    DeleteFileA(catfileA);

    /* Enumerate members and attributes */
    trace("Attributes and members\n");
    attrcount = membercount = 0;
    catcdf = pCryptCATCDFOpen(cdffileW, NULL);

    catattr = NULL;
    while ((catattr = pCryptCATCDFEnumCatAttributes(catcdf, catattr, NULL)))
        attrcount++;
    todo_wine
    ok(attrcount == 2, "Expected 2 attributes, got %d\n", attrcount);

    catmember = NULL;
    catmembertag = NULL;
    while ((catmembertag = pCryptCATCDFEnumMembersByCDFTagEx(catcdf, catmembertag, NULL, &catmember, FALSE, NULL)))
        membercount++;
    todo_wine
    ok(membercount == 1, "Expected 1 member, got %d\n", membercount);

    pCryptCATCDFClose(catcdf);
    test_catalog_properties(catfileA, 2, 1);
    DeleteFileA(catfileA);

    DeleteFileA(cdffileA);
}

static void create_cdf_file(const CHAR *filename, const CHAR *contents)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed %lu\n", GetLastError());
    WriteFile(file, contents, lstrlenA(contents), &written, NULL);
    CloseHandle(file);
}

#define CHECK_EXPECT(a, b) \
    do { \
        ok(a == error_area, "Expected %08x, got %08lx\n", a, error_area); \
        ok(b == local_error, "Expected %08x, got %08lx\n", b, local_error); \
    } while (0)

/* Clear the variables (can't use 0) */
#define CLEAR_EXPECT \
    error_area = local_error = -1

/* Set both variables so the callback routine can check if a call to it was unexpected */
#define SET_UNEXPECTED \
    error_area = local_error = -2

static void test_cdf_parsing(void)
{
    static const char catfileA[] = "tempfile.cat";
    static const char cdffileA[] = "tempfile.cdf";
    static WCHAR cdffileW[] = {'t','e','m','p','f','i','l','e','.','c','d','f',0};
    CHAR cdf_contents[4096];
    CRYPTCATCDF *catcdf;
    CRYPTCATATTRIBUTE *catattr;
    CRYPTCATMEMBER *catmember;
    WCHAR  *catmembertag;

    if (!pCryptCATCDFOpen)
    {
        win_skip("CryptCATCDFOpen is not available\n");
        return;
    }

    /* Empty file */
    DeleteFileA(cdffileA);
    create_cdf_file(cdffileA, "");

    CLEAR_EXPECT;
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, cdf_callback);
    todo_wine
    CHECK_EXPECT(CRYPTCAT_E_AREA_HEADER, CRYPTCAT_E_CDF_TAGNOTFOUND);
    ok(catcdf == NULL, "CryptCATCDFOpen succeeded\n");
    todo_wine
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    DeleteFileA(cdffileA);
    ok(!DeleteFileA(catfileA), "Didn't expect a catalog file to be created\n");

    /* Just the header */
    lstrcpyA(cdf_contents, "[CatalogHeader]\r\n");
    create_cdf_file(cdffileA, cdf_contents);

    SET_UNEXPECTED;
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, cdf_callback);
    ok(catcdf == NULL, "CryptCATCDFOpen succeeded\n");
    todo_wine
    ok(GetLastError() == ERROR_SHARING_VIOLATION,
        "Expected ERROR_SHARING_VIOLATION, got %ld\n", GetLastError());
    DeleteFileA(cdffileA);

    /* Header and member only */
    lstrcpyA(cdf_contents, "[CatalogHeader]\r\n");
    lstrcatA(cdf_contents, "[CatalogFiles]\r\n");
    lstrcatA(cdf_contents, "hashme=.\\tempfile.cdf\r\n");
    create_cdf_file(cdffileA, cdf_contents);

    SET_UNEXPECTED;
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, cdf_callback);
    ok(catcdf == NULL, "CryptCATCDFOpen succeeded\n");
    todo_wine
    ok(GetLastError() == ERROR_SHARING_VIOLATION,
        "Expected ERROR_SHARING_VIOLATION, got %ld\n", GetLastError());
    DeleteFileA(cdffileA);
    ok(!DeleteFileA(catfileA), "Didn't expect a catalog file to be created\n");

    /* Header and Name (no value) */
    lstrcpyA(cdf_contents, "[CatalogHeader]\r\n");
    lstrcatA(cdf_contents, "Name=\r\n");
    create_cdf_file(cdffileA, cdf_contents);

    SET_UNEXPECTED;
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, cdf_callback);
    ok(catcdf == NULL, "CryptCATCDFOpen succeeded\n");
    todo_wine
    ok(GetLastError() == ERROR_SHARING_VIOLATION,
        "Expected ERROR_SHARING_VIOLATION, got %ld\n", GetLastError());
    DeleteFileA(cdffileA);
    ok(!DeleteFileA(catfileA), "Didn't expect a catalog file to be created\n");

    /* Header and Name */
    lstrcpyA(cdf_contents, "[CatalogHeader]\r\n");
    lstrcatA(cdf_contents, "Name=tempfile.cat\r\n");
    create_cdf_file(cdffileA, cdf_contents);

    SET_UNEXPECTED;
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, cdf_callback);
    todo_wine
    {
    ok(catcdf != NULL, "CryptCATCDFOpen failed\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    }
    pCryptCATCDFClose(catcdf);
    DeleteFileA(cdffileA);
    todo_wine
    ok(DeleteFileA(catfileA), "Expected a catalog file to be created\n");

    /* Header and nonexistent member */
    lstrcpyA(cdf_contents, "[CatalogHeader]\r\n");
    lstrcatA(cdf_contents, "Name=tempfile.cat\r\n");
    lstrcatA(cdf_contents, "[CatalogFiles]\r\n");
    lstrcatA(cdf_contents, "hashme=.\\deadbeef.cdf\r\n");
    create_cdf_file(cdffileA, cdf_contents);

    SET_UNEXPECTED;
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, cdf_callback);
    todo_wine
    {
    ok(catcdf != NULL, "CryptCATCDFOpen failed\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    }
    /* Loop through the members */
    CLEAR_EXPECT;
    catmember = NULL;
    catmembertag = NULL;
    while ((catmembertag = pCryptCATCDFEnumMembersByCDFTagEx(catcdf, catmembertag, cdf_callback, &catmember, FALSE, NULL))) ;
    todo_wine
    CHECK_EXPECT(CRYPTCAT_E_AREA_MEMBER, CRYPTCAT_E_CDF_MEMBER_FILENOTFOUND);
    pCryptCATCDFClose(catcdf);
    DeleteFileA(cdffileA);
    todo_wine
    ok(DeleteFileA(catfileA), "Expected a catalog file to be created\n");

    /* Header, correct member but no explicit newline */
    lstrcpyA(cdf_contents, "[CatalogHeader]\r\n");
    lstrcatA(cdf_contents, "Name=tempfile.cat\r\n");
    lstrcatA(cdf_contents, "[CatalogFiles]\r\n");
    lstrcatA(cdf_contents, "hashme=.\\tempfile.cdf\r");
    create_cdf_file(cdffileA, cdf_contents);

    SET_UNEXPECTED;
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, cdf_callback);
    todo_wine
    {
    ok(catcdf != NULL, "CryptCATCDFOpen failed\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    }
    /* Loop through the members */
    CLEAR_EXPECT;
    catmember = NULL;
    catmembertag = NULL;
    while ((catmembertag = pCryptCATCDFEnumMembersByCDFTagEx(catcdf, catmembertag, cdf_callback, &catmember, FALSE, NULL))) ;
    ok(error_area == 0xffffffff || broken(error_area == CRYPTCAT_E_AREA_MEMBER) /* < win81 */,
       "Expected area 0xffffffff, got %08lx\n", error_area);
    ok(local_error == 0xffffffff || broken(local_error == CRYPTCAT_E_CDF_MEMBER_FILE_PATH) /* < win81 */,
       "Expected error 0xffffffff, got %08lx\n", local_error);

    pCryptCATCDFClose(catcdf);
    DeleteFileA(cdffileA);
    todo_wine
    ok(DeleteFileA(catfileA), "Expected a catalog file to be created\n");

    /* Header and 2 duplicate members */
    lstrcpyA(cdf_contents, "[CatalogHeader]\r\n");
    lstrcatA(cdf_contents, "Name=tempfile.cat\r\n");
    lstrcatA(cdf_contents, "[CatalogFiles]\r\n");
    lstrcatA(cdf_contents, "hashme=.\\tempfile.cdf\r\n");
    lstrcatA(cdf_contents, "hashme=.\\tempfile.cdf\r\n");
    create_cdf_file(cdffileA, cdf_contents);

    SET_UNEXPECTED;
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, cdf_callback);
    todo_wine
    {
    ok(catcdf != NULL, "CryptCATCDFOpen failed\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    }
    /* Loop through the members */
    SET_UNEXPECTED;
    catmember = NULL;
    catmembertag = NULL;
    while ((catmembertag = pCryptCATCDFEnumMembersByCDFTagEx(catcdf, catmembertag, cdf_callback, &catmember, FALSE, NULL))) ;
    pCryptCATCDFClose(catcdf);
    test_catalog_properties(catfileA, 0, 1);
    DeleteFileA(cdffileA);
    todo_wine
    ok(DeleteFileA(catfileA), "Expected a catalog file to be created\n");

    /* Wrong attribute */
    lstrcpyA(cdf_contents, "[CatalogHeader]\r\n");
    lstrcatA(cdf_contents, "Name=tempfile.cat\r\n");
    lstrcatA(cdf_contents, "CATATTR1=0x10010001:attr1\r\n");
    create_cdf_file(cdffileA, cdf_contents);

    SET_UNEXPECTED;
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, cdf_callback);
    todo_wine
    {
    ok(catcdf != NULL, "CryptCATCDFOpen failed\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    }
    /* Loop through the attributes */
    CLEAR_EXPECT;
    catattr = NULL;
    while ((catattr = pCryptCATCDFEnumCatAttributes(catcdf, catattr, cdf_callback))) ;
    todo_wine
    CHECK_EXPECT(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_ATTR_TOOFEWVALUES);
    pCryptCATCDFClose(catcdf);
    DeleteFileA(cdffileA);
    todo_wine
    ok(DeleteFileA(catfileA), "Expected a catalog file to be created\n");

    /* Two identical attributes */
    lstrcpyA(cdf_contents, "[CatalogHeader]\r\n");
    lstrcatA(cdf_contents, "Name=tempfile.cat\r\n");
    lstrcatA(cdf_contents, "CATATTR1=0x10010001:attr1:value1\r\n");
    lstrcatA(cdf_contents, "CATATTR1=0x10010001:attr1:value1\r\n");
    lstrcatA(cdf_contents, "[CatalogFiles]\r\n");
    lstrcatA(cdf_contents, "hashme=.\\tempfile.cdf\r\n");
    create_cdf_file(cdffileA, cdf_contents);

    SET_UNEXPECTED;
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, cdf_callback);
    todo_wine
    {
    ok(catcdf != NULL, "CryptCATCDFOpen failed\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    }
    /* Loop through the members */
    SET_UNEXPECTED;
    catmember = NULL;
    catmembertag = NULL;
    while ((catmembertag = pCryptCATCDFEnumMembersByCDFTagEx(catcdf, catmembertag, cdf_callback, &catmember, FALSE, NULL))) ;
    /* Loop through the attributes */
    SET_UNEXPECTED;
    catattr = NULL;
    while ((catattr = pCryptCATCDFEnumCatAttributes(catcdf, catattr, cdf_callback))) ;
    pCryptCATCDFClose(catcdf);
    test_catalog_properties(catfileA, 1, 1);
    DeleteFileA(cdffileA);
    todo_wine
    ok(DeleteFileA(catfileA), "Expected a catalog file to be created\n");

    /* Two different attribute values with the same tag */
    lstrcpyA(cdf_contents, "[CatalogHeader]\r\n");
    lstrcatA(cdf_contents, "Name=tempfile.cat\r\n");
    lstrcatA(cdf_contents, "CATATTR1=0x10010001:attr1:value1\r\n");
    lstrcatA(cdf_contents, "CATATTR1=0x10010001:attr2:value2\r\n");
    lstrcatA(cdf_contents, "[CatalogFiles]\r\n");
    lstrcatA(cdf_contents, "hashme=.\\tempfile.cdf\r\n");
    create_cdf_file(cdffileA, cdf_contents);

    SET_UNEXPECTED;
    SetLastError(0xdeadbeef);
    catcdf = pCryptCATCDFOpen(cdffileW, cdf_callback);
    todo_wine
    {
    ok(catcdf != NULL, "CryptCATCDFOpen failed\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", GetLastError());
    }
    /* Loop through the members */
    SET_UNEXPECTED;
    catmember = NULL;
    catmembertag = NULL;
    while ((catmembertag = pCryptCATCDFEnumMembersByCDFTagEx(catcdf, catmembertag, cdf_callback, &catmember, FALSE, NULL))) ;
    /* Loop through the attributes */
    SET_UNEXPECTED;
    catattr = NULL;
    while ((catattr = pCryptCATCDFEnumCatAttributes(catcdf, catattr, cdf_callback))) ;
    pCryptCATCDFClose(catcdf);
    test_catalog_properties(catfileA, 1, 1);
    DeleteFileA(cdffileA);
    todo_wine
    ok(DeleteFileA(catfileA), "Expected a catalog file to be created\n");
}

static const struct
{
    WORD e_magic;      /* 00: MZ Header signature */
    WORD unused[29];
    DWORD e_lfanew;    /* 3c: Offset to extended header */
} dos_header =
{
    IMAGE_DOS_SIGNATURE, { 0 }, sizeof(dos_header)
};

static IMAGE_NT_HEADERS nt_header =
{
    IMAGE_NT_SIGNATURE, /* Signature */
    {
        IMAGE_FILE_MACHINE_I386, /* Machine */
        1, /* NumberOfSections */
        0, /* TimeDateStamp */
        0, /* PointerToSymbolTable */
        0, /* NumberOfSymbols */
        sizeof(IMAGE_OPTIONAL_HEADER), /* SizeOfOptionalHeader */
        IMAGE_FILE_EXECUTABLE_IMAGE /* Characteristics */
    },
    {
        IMAGE_NT_OPTIONAL_HDR_MAGIC, /* Magic */
        2, /* MajorLinkerVersion */
        15, /* MinorLinkerVersion */
        0, /* SizeOfCode */
        0, /* SizeOfInitializedData */
        0, /* SizeOfUninitializedData */
        0, /* AddressOfEntryPoint */
        0x10, /* BaseOfCode, also serves as e_lfanew in the truncated MZ header */
#ifndef _WIN64
        0, /* BaseOfData */
#endif
        0x10000000, /* ImageBase */
        0, /* SectionAlignment */
        0, /* FileAlignment */
        4, /* MajorOperatingSystemVersion */
        0, /* MinorOperatingSystemVersion */
        1, /* MajorImageVersion */
        0, /* MinorImageVersion */
        4, /* MajorSubsystemVersion */
        0, /* MinorSubsystemVersion */
        0, /* Win32VersionValue */
        0x200, /* SizeOfImage */
        sizeof(dos_header) + sizeof(nt_header), /* SizeOfHeaders */
        0, /* CheckSum */
        IMAGE_SUBSYSTEM_WINDOWS_CUI, /* Subsystem */
        0, /* DllCharacteristics */
        0, /* SizeOfStackReserve */
        0, /* SizeOfStackCommit */
        0, /* SizeOfHeapReserve */
        3, /* SizeOfHeapCommit */
        2, /* LoaderFlags */
        1, /* NumberOfRvaAndSizes */
        { { 0 } } /* DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] */
    }
};

static void test_sip(void)
{
    static const WCHAR nameW[] = {'t','e','s','t','.','e','x','e',0};
    SIP_SUBJECTINFO info;
    DWORD index, encoding, size;
    HANDLE file;
    GUID guid;
    BOOL ret;
    char buf[1024];

    file = CreateFileW(nameW, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "can't create file\n");
    if(file == INVALID_HANDLE_VALUE)
        return;
    WriteFile(file, &dos_header, sizeof(dos_header), &size, NULL);
    WriteFile(file, &nt_header, sizeof(nt_header), &size, NULL);
    memset(buf, 0, sizeof(buf));
    WriteFile(file, buf, 0x200 - sizeof(dos_header) - sizeof(nt_header), &size, NULL);
    CloseHandle(file);

    file= CreateFileW(nameW, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "can't open file\n");

    memset(&info, 0, sizeof(SIP_SUBJECTINFO));
    info.cbSize = sizeof(SIP_SUBJECTINFO);
    info.pgSubjectType = &guid;
    ret = CryptSIPRetrieveSubjectGuid(NULL, file, info.pgSubjectType);
    ok(ret, "CryptSIPRetrieveSubjectGuid failed (%lx)\n", GetLastError());

    ret = pPutSignedDataMsg(&info, X509_ASN_ENCODING, &index, 4, (BYTE*)"test");
    ok(!ret, "CryptSIPPutSignedDataMsg succeeded\n");
    index = GetLastError();
    ok(index == ERROR_PATH_NOT_FOUND, "GetLastError returned %lx\n", index);

    info.hFile = file;
    info.pwsFileName = nameW;
    ret = pPutSignedDataMsg(&info, X509_ASN_ENCODING, &index, 4, (BYTE*)"test");
    ok(!ret, "CryptSIPPutSignedDataMsg succeeded\n");
    index = GetLastError();
    todo_wine ok(index == ERROR_INVALID_PARAMETER, "GetLastError returned %lx\n", index);

    info.hFile = INVALID_HANDLE_VALUE;
    info.pwsFileName = nameW;
    ret = pPutSignedDataMsg(&info, X509_ASN_ENCODING, &index, 4, (BYTE*)"test");
    ok(!ret, "CryptSIPPutSignedDataMsg succeeded\n");
    index = GetLastError();
    ok(index == ERROR_SHARING_VIOLATION, "GetLastError returned %lx\n", index);

    CloseHandle(file);
    file= CreateFileW(nameW, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    info.hFile = file;
    info.pwsFileName = (void*)0xdeadbeef;
    ret = pPutSignedDataMsg(&info, X509_ASN_ENCODING, &index, 4, (BYTE*)"test");
    ok(ret, "CryptSIPPutSignedDataMsg failed (%lx)\n", GetLastError());
    ok(index == 0, "index = %lx\n", index);

    CloseHandle(file);
    file= CreateFileW(nameW, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    info.hFile = INVALID_HANDLE_VALUE;
    info.pwsFileName = nameW;
    ret = pPutSignedDataMsg(&info, X509_ASN_ENCODING, &index, 14, (BYTE*)"longer message");
    ok(ret, "CryptSIPPutSignedDataMsg failed (%lx)\n", GetLastError());
    ok(index == 1, "index = %lx\n", index);

    size = 0;
    encoding = 0xdeadbeef;
    ret = pGetSignedDataMsg(&info, &encoding, 0, &size, NULL);
    ok(ret, "CryptSIPGetSignedDataMsg failed (%lx)\n", GetLastError());
    ok(encoding == 0xdeadbeef, "encoding = %lx\n", encoding);
    ok(size == 16, "size = %ld\n", size);

    ret = pGetSignedDataMsg(&info, &encoding, 0, &size, (BYTE*)buf);
    ok(ret, "CryptSIPGetSignedDataMsg failed (%lx)\n", GetLastError());
    ok(encoding == (X509_ASN_ENCODING|PKCS_7_ASN_ENCODING), "encoding = %lx\n", encoding);
    ok(size == 8, "size = %ld\n", size);
    ok(!memcmp(buf, "test\0\0\0\0", 8), "buf = %s\n", buf);

    size = 0;
    encoding = 0xdeadbeef;
    ret = pGetSignedDataMsg(&info, &encoding, 1, &size, NULL);
    ok(ret, "CryptSIPGetSignedDataMsg failed (%lx)\n", GetLastError());
    ok(encoding == 0xdeadbeef, "encoding = %lx\n", encoding);
    ok(size == 24, "size = %ld\n", size);

    ret = pGetSignedDataMsg(&info, &encoding, 1, &size, (BYTE*)buf);
    ok(ret, "CryptSIPGetSignedDataMsg failed (%lx)\n", GetLastError());
    ok(encoding == (X509_ASN_ENCODING|PKCS_7_ASN_ENCODING), "encoding = %lx\n", encoding);
    ok(size == 16, "size = %ld\n", size);
    ok(!strcmp(buf, "longer message"), "buf = %s\n", buf);

    CryptReleaseContext(info.hProv, 0);
    CloseHandle(file);
    DeleteFileW(nameW);
}

START_TEST(crypt)
{
    char** myARGV;
    char sysdir[MAX_PATH];

    InitFunctionPtrs();

    if (!pCryptCATAdminAcquireContext)
    {
        win_skip("CryptCATAdmin functions are not available\n");
        return;
    }

    GetSystemDirectoryA(sysdir, MAX_PATH);
    lstrcpyA(catroot, sysdir);
    lstrcatA(catroot, "\\CatRoot");
    lstrcpyA(catroot2, sysdir);
    lstrcatA(catroot2, "\\CatRoot2");

    winetest_get_mainargs(&myARGV);
    strcpy(selfname, myARGV[0]);

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);
   
    test_context();
    test_calchash();
    test_CryptCATOpen();
    /* Parameter checking only */
    test_CryptCATCDF_params();
    /* Test the parsing of a cdf file */
    test_cdf_parsing();
    /* Create a catalog file out of our own catalog definition file */
    test_create_catalog_file();
    test_CryptCATAdminAddRemoveCatalog();
    test_sip();
}
