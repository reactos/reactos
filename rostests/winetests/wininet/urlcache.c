/*
 * URL Cache Tests
 *
 * Copyright 2008 Robert Shearman for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"

#include "wine/test.h"

#define TEST_URL    "http://urlcachetest.winehq.org/index.html"

static BOOL (WINAPI *pDeleteUrlCacheEntryA)(LPCSTR);
static BOOL (WINAPI *pUnlockUrlCacheEntryFileA)(LPCSTR,DWORD);

static char filenameA[MAX_PATH + 1];

static void check_cache_entry_infoA(const char *returnedfrom, LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo)
{
    ok(lpCacheEntryInfo->dwStructSize == sizeof(*lpCacheEntryInfo), "%s: dwStructSize was %d\n", returnedfrom, lpCacheEntryInfo->dwStructSize);
    ok(!strcmp(lpCacheEntryInfo->lpszSourceUrlName, TEST_URL), "%s: lpszSourceUrlName should be %s instead of %s\n", returnedfrom, TEST_URL, lpCacheEntryInfo->lpszSourceUrlName);
    ok(!strcmp(lpCacheEntryInfo->lpszLocalFileName, filenameA), "%s: lpszLocalFileName should be %s instead of %s\n", returnedfrom, filenameA, lpCacheEntryInfo->lpszLocalFileName);
    ok(!strcmp(lpCacheEntryInfo->lpszFileExtension, "html"), "%s: lpszFileExtension should be html instead of %s\n", returnedfrom, lpCacheEntryInfo->lpszFileExtension);
}

static void test_find_url_cache_entriesA(void)
{
    BOOL ret;
    HANDLE hEnumHandle;
    BOOL found = FALSE;
    DWORD cbCacheEntryInfo;
    DWORD cbCacheEntryInfoSaved;
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo;

    cbCacheEntryInfo = 0;
    hEnumHandle = FindFirstUrlCacheEntry(NULL, NULL, &cbCacheEntryInfo);
    ok(!hEnumHandle, "FindFirstUrlCacheEntry should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "FindFirstUrlCacheEntry should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo * sizeof(char));
    cbCacheEntryInfoSaved = cbCacheEntryInfo;
    hEnumHandle = FindFirstUrlCacheEntry(NULL, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(hEnumHandle != NULL, "FindFirstUrlCacheEntry failed with error %d\n", GetLastError());
    while (TRUE)
    {
        if (!strcmp(lpCacheEntryInfo->lpszSourceUrlName, TEST_URL))
        {
            found = TRUE;
            break;
        }
        cbCacheEntryInfo = cbCacheEntryInfoSaved;
        ret = FindNextUrlCacheEntry(hEnumHandle, lpCacheEntryInfo, &cbCacheEntryInfo);
        if (!ret)
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                lpCacheEntryInfo = HeapReAlloc(GetProcessHeap(), 0, lpCacheEntryInfo, cbCacheEntryInfo);
                cbCacheEntryInfoSaved = cbCacheEntryInfo;
                ret = FindNextUrlCacheEntry(hEnumHandle, lpCacheEntryInfo, &cbCacheEntryInfo);
            }
        }
        ok(ret, "FindNextUrlCacheEntry failed with error %d\n", GetLastError());
        if (!ret)
            break;
    }
    ok(found, "committed url cache entry not found during enumeration\n");

    ret = FindCloseUrlCache(hEnumHandle);
    ok(ret, "FindCloseUrlCache failed with error %d\n", GetLastError());
}

static void test_GetUrlCacheEntryInfoExA(void)
{
    BOOL ret;
    DWORD cbCacheEntryInfo;
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo;

    ret = GetUrlCacheEntryInfoEx(NULL, NULL, NULL, NULL, NULL, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx with NULL URL and NULL args should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetUrlCacheEntryInfoEx with NULL URL and NULL args should have set last error to ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());

    ret = GetUrlCacheEntryInfoEx(TEST_URL, NULL, NULL, NULL, NULL, NULL, 0);
    ok(ret, "GetUrlCacheEntryInfoEx with NULL args failed with error %d\n", GetLastError());

    cbCacheEntryInfo = 0;
    ret = GetUrlCacheEntryInfoEx(TEST_URL, NULL, &cbCacheEntryInfo, NULL, NULL, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx with zero-length buffer should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetUrlCacheEntryInfoEx should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());

    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfoEx(TEST_URL, lpCacheEntryInfo, &cbCacheEntryInfo, NULL, NULL, NULL, 0);
    ok(ret, "GetUrlCacheEntryInfoEx failed with error %d\n", GetLastError());

    check_cache_entry_infoA("GetUrlCacheEntryInfoEx", lpCacheEntryInfo);

    cbCacheEntryInfo = 100000;
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoEx(TEST_URL, NULL, &cbCacheEntryInfo, NULL, NULL, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx with zero-length buffer should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetUrlCacheEntryInfoEx should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);
}

static void test_RetrieveUrlCacheEntryA(void)
{
    BOOL ret;
    DWORD cbCacheEntryInfo;

    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = RetrieveUrlCacheEntryFile(NULL, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "RetrieveUrlCacheEntryFile should have set last error to ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());

    if (0)
    {
        /* Crashes on Win9x, NT4 and W2K */
        SetLastError(0xdeadbeef);
        ret = RetrieveUrlCacheEntryFile(TEST_URL, NULL, NULL, 0);
        ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "RetrieveUrlCacheEntryFile should have set last error to ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    cbCacheEntryInfo = 100000;
    ret = RetrieveUrlCacheEntryFile(NULL, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "RetrieveUrlCacheEntryFile should have set last error to ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());
}

static void test_urlcacheA(void)
{
    BOOL ret;
    HANDLE hFile;
    DWORD written;
    BYTE zero_byte = 0;
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo;
    DWORD cbCacheEntryInfo;
    static const FILETIME filetime_zero;

    ret = CreateUrlCacheEntry(TEST_URL, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());

    hFile = CreateFileA(filenameA, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA failed with error %d\n", GetLastError());

    ret = WriteFile(hFile, &zero_byte, sizeof(zero_byte), &written, NULL);
    ok(ret, "WriteFile failed with error %d\n", GetLastError());

    CloseHandle(hFile);

    ret = CommitUrlCacheEntry(TEST_URL, filenameA, filetime_zero, filetime_zero, NORMAL_CACHE_ENTRY, NULL, 0, "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());

    cbCacheEntryInfo = 0;
    ret = RetrieveUrlCacheEntryFile(TEST_URL, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "RetrieveUrlCacheEntryFile should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());

    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = RetrieveUrlCacheEntryFile(TEST_URL, lpCacheEntryInfo, &cbCacheEntryInfo, 0);
    ok(ret, "RetrieveUrlCacheEntryFile failed with error %d\n", GetLastError());

    check_cache_entry_infoA("RetrieveUrlCacheEntryFile", lpCacheEntryInfo);

    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    if (pUnlockUrlCacheEntryFileA)
    {
        ret = pUnlockUrlCacheEntryFileA(TEST_URL, 0);
        ok(ret, "UnlockUrlCacheEntryFileA failed with error %d\n", GetLastError());
    }

    /* test Find*UrlCacheEntry functions */
    test_find_url_cache_entriesA();

    test_GetUrlCacheEntryInfoExA();
    test_RetrieveUrlCacheEntryA();

    if (pDeleteUrlCacheEntryA)
    {
        ret = pDeleteUrlCacheEntryA(TEST_URL);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
    }

    ret = DeleteFile(filenameA);
    todo_wine
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND, "local file should no longer exist\n");
}

static void test_FindCloseUrlCache(void)
{
    BOOL r;
    DWORD err;
    r = FindCloseUrlCache(NULL);
    err = GetLastError();
    ok(0 == r, "expected 0, got %d\n", r);
    ok(ERROR_INVALID_HANDLE == err, "expected %d, got %d\n", ERROR_INVALID_HANDLE, err);
}

START_TEST(urlcache)
{
    HMODULE hdll;
    hdll = GetModuleHandleA("wininet.dll");
    pDeleteUrlCacheEntryA = (void*)GetProcAddress(hdll, "DeleteUrlCacheEntryA");
    pUnlockUrlCacheEntryFileA = (void*)GetProcAddress(hdll, "UnlockUrlCacheEntryFileA");
    test_urlcacheA();
    test_FindCloseUrlCache();
}
