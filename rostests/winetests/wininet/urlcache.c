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
#include "winineti.h"

#include "wine/test.h"

#define TEST_URL    "http://urlcachetest.winehq.org/index.html"
#define TEST_URL1   "Visited: user@http://urlcachetest.winehq.org/index.html"

static BOOL (WINAPI *pDeleteUrlCacheEntryA)(LPCSTR);
static BOOL (WINAPI *pUnlockUrlCacheEntryFileA)(LPCSTR,DWORD);

static char filenameA[MAX_PATH + 1];
static char filenameA1[MAX_PATH + 1];

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
    SetLastError(0xdeadbeef);
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
        SetLastError(0xdeadbeef);
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
    DWORD cbCacheEntryInfo, cbRedirectUrl;
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo;

    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoEx(NULL, NULL, NULL, NULL, NULL, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx with NULL URL and NULL args should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "GetUrlCacheEntryInfoEx with NULL URL and NULL args should have set last error to ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());

    cbCacheEntryInfo = sizeof(INTERNET_CACHE_ENTRY_INFO);
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoEx("", NULL, &cbCacheEntryInfo, NULL, NULL, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx with zero-length buffer should fail\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "GetUrlCacheEntryInfoEx should have set last error to ERROR_FILE_NOT_FOUND instead of %d\n", GetLastError());

    ret = GetUrlCacheEntryInfoEx(TEST_URL, NULL, NULL, NULL, NULL, NULL, 0);
    ok(ret, "GetUrlCacheEntryInfoEx with NULL args failed with error %d\n", GetLastError());

    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoEx(TEST_URL, NULL, &cbCacheEntryInfo, NULL, NULL, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx with zero-length buffer should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "GetUrlCacheEntryInfoEx should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());

    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);

    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoEx(TEST_URL, lpCacheEntryInfo, &cbCacheEntryInfo, NULL, NULL, NULL, 0x200);
    ok(!ret, "GetUrlCacheEntryInfoEx succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "GetUrlCacheEntryInfoEx should have set last error to ERROR_FILE_NOT_FOUND instead of %d\n", GetLastError());

    ret = GetUrlCacheEntryInfoEx(TEST_URL, lpCacheEntryInfo, &cbCacheEntryInfo, NULL, NULL, NULL, 0);
    ok(ret, "GetUrlCacheEntryInfoEx failed with error %d\n", GetLastError());

    check_cache_entry_infoA("GetUrlCacheEntryInfoEx", lpCacheEntryInfo);

    cbCacheEntryInfo = 100000;
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoEx(TEST_URL, NULL, &cbCacheEntryInfo, NULL, NULL, NULL, 0);
    ok(!ret, "GetUrlCacheEntryInfoEx with zero-length buffer should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetUrlCacheEntryInfoEx should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    /* Querying the redirect URL fails with ERROR_INVALID_PARAMETER */
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoEx(TEST_URL, NULL, NULL, NULL, &cbRedirectUrl, NULL, 0);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfoEx(TEST_URL, NULL, &cbCacheEntryInfo, NULL, &cbRedirectUrl, NULL, 0);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
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

static void test_IsUrlCacheEntryExpiredA(void)
{
    static const char uncached_url[] =
        "What's the airspeed velocity of an unladen swallow?";
    BOOL ret;
    FILETIME ft;
    DWORD size;
    LPINTERNET_CACHE_ENTRY_INFO info;
    ULARGE_INTEGER exp_time;

    /* The function returns TRUE when the output time is NULL or the tested URL
     * is NULL.
     */
    ret = IsUrlCacheEntryExpiredA(NULL, 0, NULL);
    ok(ret, "expected TRUE\n");
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    ret = IsUrlCacheEntryExpiredA(NULL, 0, &ft);
    ok(ret, "expected TRUE\n");
    ok(ft.dwLowDateTime == 0xdeadbeef && ft.dwHighDateTime == 0xbaadf00d,
       "expected time to be unchanged, got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);
    ret = IsUrlCacheEntryExpiredA(TEST_URL, 0, NULL);
    ok(ret, "expected TRUE\n");

    /* The return value should indicate whether the URL is expired,
     * and the filetime indicates the last modified time, but a cache entry
     * with a zero expire time is "not expired".
     */
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    ret = IsUrlCacheEntryExpiredA(TEST_URL, 0, &ft);
    ok(!ret, "expected FALSE\n");
    ok(!ft.dwLowDateTime && !ft.dwHighDateTime,
       "expected time (0,0), got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);

    /* Same behavior with bogus flags. */
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    ret = IsUrlCacheEntryExpiredA(TEST_URL, 0xffffffff, &ft);
    ok(!ret, "expected FALSE\n");
    ok(!ft.dwLowDateTime && !ft.dwHighDateTime,
       "expected time (0,0), got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);

    /* Set the expire time to a point in the past.. */
    ret = GetUrlCacheEntryInfo(TEST_URL, NULL, &size);
    ok(!ret, "GetUrlCacheEntryInfo should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    info = HeapAlloc(GetProcessHeap(), 0, size);
    ret = GetUrlCacheEntryInfo(TEST_URL, info, &size);
    GetSystemTimeAsFileTime(&info->ExpireTime);
    exp_time.u.LowPart = info->ExpireTime.dwLowDateTime;
    exp_time.u.HighPart = info->ExpireTime.dwHighDateTime;
    exp_time.QuadPart -= 10 * 60 * (ULONGLONG)10000000;
    info->ExpireTime.dwLowDateTime = exp_time.u.LowPart;
    info->ExpireTime.dwHighDateTime = exp_time.u.HighPart;
    ret = SetUrlCacheEntryInfo(TEST_URL, info, CACHE_ENTRY_EXPTIME_FC);
    ok(ret, "SetUrlCacheEntryInfo failed: %d\n", GetLastError());
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    /* and the entry should be expired. */
    ret = IsUrlCacheEntryExpiredA(TEST_URL, 0, &ft);
    ok(ret, "expected TRUE\n");
    /* The modified time returned is 0. */
    ok(!ft.dwLowDateTime && !ft.dwHighDateTime,
       "expected time (0,0), got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);
    /* Set the expire time to a point in the future.. */
    exp_time.QuadPart += 20 * 60 * (ULONGLONG)10000000;
    info->ExpireTime.dwLowDateTime = exp_time.u.LowPart;
    info->ExpireTime.dwHighDateTime = exp_time.u.HighPart;
    ret = SetUrlCacheEntryInfo(TEST_URL, info, CACHE_ENTRY_EXPTIME_FC);
    ok(ret, "SetUrlCacheEntryInfo failed: %d\n", GetLastError());
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    /* and the entry should no longer be expired. */
    ret = IsUrlCacheEntryExpiredA(TEST_URL, 0, &ft);
    ok(!ret, "expected FALSE\n");
    /* The modified time returned is still 0. */
    ok(!ft.dwLowDateTime && !ft.dwHighDateTime,
       "expected time (0,0), got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);
    /* Set the modified time... */
    GetSystemTimeAsFileTime(&info->LastModifiedTime);
    ret = SetUrlCacheEntryInfo(TEST_URL, info, CACHE_ENTRY_MODTIME_FC);
    ok(ret, "SetUrlCacheEntryInfo failed: %d\n", GetLastError());
    /* and the entry should still be unexpired.. */
    ret = IsUrlCacheEntryExpiredA(TEST_URL, 0, &ft);
    ok(!ret, "expected FALSE\n");
    /* but the modified time returned is the last modified time just set. */
    ok(ft.dwLowDateTime == info->LastModifiedTime.dwLowDateTime &&
       ft.dwHighDateTime == info->LastModifiedTime.dwHighDateTime,
       "expected time (%u,%u), got (%u,%u)\n",
       info->LastModifiedTime.dwLowDateTime,
       info->LastModifiedTime.dwHighDateTime,
       ft.dwLowDateTime, ft.dwHighDateTime);
    HeapFree(GetProcessHeap(), 0, info);

    /* An uncached URL is implicitly expired, but with unknown time. */
    ft.dwLowDateTime = 0xdeadbeef;
    ft.dwHighDateTime = 0xbaadf00d;
    ret = IsUrlCacheEntryExpiredA(uncached_url, 0, &ft);
    ok(ret, "expected TRUE\n");
    ok(!ft.dwLowDateTime && !ft.dwHighDateTime,
       "expected time (0,0), got (%u,%u)\n",
       ft.dwLowDateTime, ft.dwHighDateTime);
}

static void _check_file_exists(LONG l, LPCSTR filename)
{
    HANDLE file;

    file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok_(__FILE__,l)(file != INVALID_HANDLE_VALUE,
                    "expected file to exist, CreateFile failed with error %d\n",
                    GetLastError());
    CloseHandle(file);
}

#define check_file_exists(f) _check_file_exists(__LINE__, f)

static void _check_file_not_exists(LONG l, LPCSTR filename)
{
    HANDLE file;

    file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok_(__FILE__,l)(file == INVALID_HANDLE_VALUE,
                    "expected file not to exist\n");
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);
}

#define check_file_not_exists(f) _check_file_not_exists(__LINE__, f)

static void create_and_write_file(LPCSTR filename, void *data, DWORD len)
{
    HANDLE file;
    DWORD written;
    BOOL ret;

    file = CreateFileA(filename, GENERIC_WRITE,
                       FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed with error %d\n", GetLastError());

    ret = WriteFile(file, data, len, &written, NULL);
    ok(ret, "WriteFile failed with error %d\n", GetLastError());

    CloseHandle(file);
}

static void test_urlcacheA(void)
{
    static char ok_header[] = "HTTP/1.0 200 OK\r\n\r\n";
    BOOL ret;
    HANDLE hFile;
    BYTE zero_byte = 0;
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo;
    DWORD cbCacheEntryInfo;
    static const FILETIME filetime_zero;
    FILETIME now;

    ret = CreateUrlCacheEntry(TEST_URL, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());

    ret = CreateUrlCacheEntry(TEST_URL, 0, "html", filenameA1, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());

    ok(lstrcmpiA(filenameA, filenameA1), "expected a different file name\n");

    create_and_write_file(filenameA, &zero_byte, sizeof(zero_byte));

    ret = CommitUrlCacheEntry(TEST_URL1, NULL, filetime_zero, filetime_zero, NORMAL_CACHE_ENTRY|URLHISTORY_CACHE_ENTRY, NULL, 0, NULL, NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    cbCacheEntryInfo = 0;
    ret = GetUrlCacheEntryInfo(TEST_URL1, NULL, &cbCacheEntryInfo);
    ok(!ret, "GetUrlCacheEntryInfo should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "GetUrlCacheEntryInfo should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfo(TEST_URL1, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(!memcmp(&lpCacheEntryInfo->ExpireTime, &filetime_zero, sizeof(FILETIME)),
       "expected zero ExpireTime\n");
    ok(!memcmp(&lpCacheEntryInfo->LastModifiedTime, &filetime_zero, sizeof(FILETIME)),
       "expected zero LastModifiedTime\n");
    ok(lpCacheEntryInfo->CacheEntryType == (NORMAL_CACHE_ENTRY|URLHISTORY_CACHE_ENTRY) ||
       broken(lpCacheEntryInfo->CacheEntryType == NORMAL_CACHE_ENTRY /* NT4/W2k */),
       "expected type NORMAL_CACHE_ENTRY|URLHISTORY_CACHE_ENTRY, got %08x\n",
       lpCacheEntryInfo->CacheEntryType);
    ok(!U(*lpCacheEntryInfo).dwExemptDelta, "expected dwExemptDelta 0, got %d\n",
       U(*lpCacheEntryInfo).dwExemptDelta);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    /* A subsequent commit with a different time/type doesn't change the type */
    GetSystemTimeAsFileTime(&now);
    ret = CommitUrlCacheEntry(TEST_URL1, NULL, now, now, NORMAL_CACHE_ENTRY,
            (LPBYTE)ok_header, strlen(ok_header), NULL, NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    cbCacheEntryInfo = 0;
    ret = GetUrlCacheEntryInfo(TEST_URL1, NULL, &cbCacheEntryInfo);
    ok(!ret, "GetUrlCacheEntryInfo should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfo(TEST_URL1, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    /* but it does change the time.. */
    todo_wine
    ok(memcmp(&lpCacheEntryInfo->ExpireTime, &filetime_zero, sizeof(FILETIME)),
       "expected positive ExpireTime\n");
    todo_wine
    ok(memcmp(&lpCacheEntryInfo->LastModifiedTime, &filetime_zero, sizeof(FILETIME)),
       "expected positive LastModifiedTime\n");
    ok(lpCacheEntryInfo->CacheEntryType == (NORMAL_CACHE_ENTRY|URLHISTORY_CACHE_ENTRY) ||
       broken(lpCacheEntryInfo->CacheEntryType == NORMAL_CACHE_ENTRY /* NT4/W2k */),
       "expected type NORMAL_CACHE_ENTRY|URLHISTORY_CACHE_ENTRY, got %08x\n",
       lpCacheEntryInfo->CacheEntryType);
    /* and set the headers. */
    todo_wine
    ok(lpCacheEntryInfo->dwHeaderInfoSize == 19,
       "expected headers size 19, got %d\n",
       lpCacheEntryInfo->dwHeaderInfoSize);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    ret = CommitUrlCacheEntry(TEST_URL, filenameA, filetime_zero, filetime_zero, NORMAL_CACHE_ENTRY, NULL, 0, "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());

    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = RetrieveUrlCacheEntryFile(TEST_URL, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "RetrieveUrlCacheEntryFile should have set last error to ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());

    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = RetrieveUrlCacheEntryFile(TEST_URL, lpCacheEntryInfo, &cbCacheEntryInfo, 0);
    ok(ret, "RetrieveUrlCacheEntryFile failed with error %d\n", GetLastError());

    check_cache_entry_infoA("RetrieveUrlCacheEntryFile", lpCacheEntryInfo);

    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = RetrieveUrlCacheEntryFile(TEST_URL1, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INVALID_DATA,
       "RetrieveUrlCacheEntryFile should have set last error to ERROR_INVALID_DATA instead of %d\n", GetLastError());

    if (pUnlockUrlCacheEntryFileA)
    {
        ret = pUnlockUrlCacheEntryFileA(TEST_URL, 0);
        ok(ret, "UnlockUrlCacheEntryFileA failed with error %d\n", GetLastError());
    }

    /* test Find*UrlCacheEntry functions */
    test_find_url_cache_entriesA();

    test_GetUrlCacheEntryInfoExA();
    test_RetrieveUrlCacheEntryA();
    test_IsUrlCacheEntryExpiredA();

    if (pDeleteUrlCacheEntryA)
    {
        ret = pDeleteUrlCacheEntryA(TEST_URL);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
        ret = pDeleteUrlCacheEntryA(TEST_URL1);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = DeleteFile(filenameA);
    todo_wine
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND, "local file should no longer exist\n");

    /* Creating two entries with the same URL */
    ret = CreateUrlCacheEntry(TEST_URL, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());

    ret = CreateUrlCacheEntry(TEST_URL, 0, "html", filenameA1, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());

    ok(lstrcmpiA(filenameA, filenameA1), "expected a different file name\n");

    create_and_write_file(filenameA, &zero_byte, sizeof(zero_byte));
    create_and_write_file(filenameA1, &zero_byte, sizeof(zero_byte));
    check_file_exists(filenameA);
    check_file_exists(filenameA1);

    ret = CommitUrlCacheEntry(TEST_URL, filenameA, filetime_zero,
            filetime_zero, NORMAL_CACHE_ENTRY, (LPBYTE)ok_header,
            strlen(ok_header), "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    check_file_exists(filenameA);
    check_file_exists(filenameA1);
    ret = CommitUrlCacheEntry(TEST_URL, filenameA1, filetime_zero,
            filetime_zero, COOKIE_CACHE_ENTRY, NULL, 0, "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    /* By committing the same URL a second time, the prior entry is
     * overwritten...
     */
    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfo(TEST_URL, NULL, &cbCacheEntryInfo);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfo(TEST_URL, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    /* with the previous entry type retained.. */
    ok(lpCacheEntryInfo->CacheEntryType & NORMAL_CACHE_ENTRY,
       "expected cache entry type NORMAL_CACHE_ENTRY, got %d (0x%08x)\n",
       lpCacheEntryInfo->CacheEntryType, lpCacheEntryInfo->CacheEntryType);
    /* and the headers overwritten.. */
    todo_wine
    ok(!lpCacheEntryInfo->dwHeaderInfoSize, "expected headers size 0, got %d\n",
       lpCacheEntryInfo->dwHeaderInfoSize);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);
    /* and the previous filename shouldn't exist. */
    todo_wine
    check_file_not_exists(filenameA);
    check_file_exists(filenameA1);

    if (pDeleteUrlCacheEntryA)
    {
        ret = pDeleteUrlCacheEntryA(TEST_URL);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
        todo_wine
        check_file_not_exists(filenameA);
        todo_wine
        check_file_not_exists(filenameA1);
        /* Just in case, clean up files */
        DeleteFileA(filenameA1);
        DeleteFileA(filenameA);
    }

    /* Check whether a retrieved cache entry can be deleted before it's
     * unlocked:
     */
    ret = CreateUrlCacheEntry(TEST_URL, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());
    ret = CommitUrlCacheEntry(TEST_URL, filenameA, filetime_zero, filetime_zero,
            NORMAL_CACHE_ENTRY, NULL, 0, "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());

    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = RetrieveUrlCacheEntryFile(TEST_URL, NULL, &cbCacheEntryInfo, 0);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = RetrieveUrlCacheEntryFile(TEST_URL, lpCacheEntryInfo,
            &cbCacheEntryInfo, 0);
    ok(ret, "RetrieveUrlCacheEntryFile failed with error %d\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);

    if (pDeleteUrlCacheEntryA)
    {
        ret = pDeleteUrlCacheEntryA(TEST_URL);
        todo_wine
        ok(!ret, "Expected failure\n");
        todo_wine
        ok(GetLastError() == ERROR_SHARING_VIOLATION,
           "Expected ERROR_SHARING_VIOLATION, got %d\n", GetLastError());
        check_file_exists(filenameA);
    }
    if (pUnlockUrlCacheEntryFileA)
    {
        check_file_exists(filenameA);
        ret = pUnlockUrlCacheEntryFileA(TEST_URL, 0);
        todo_wine
        ok(ret, "UnlockUrlCacheEntryFileA failed: %d\n", GetLastError());
        /* By unlocking the already-deleted cache entry, the file associated
         * with it is deleted..
         */
        todo_wine
        check_file_not_exists(filenameA);
        /* (just in case, delete file) */
        DeleteFileA(filenameA);
    }
    if (pDeleteUrlCacheEntryA)
    {
        /* and a subsequent deletion should fail. */
        ret = pDeleteUrlCacheEntryA(TEST_URL);
        ok(!ret, "Expected failure\n");
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }

    /* Test whether preventing a file from being deleted causes
     * DeleteUrlCacheEntryA to fail.
     */
    ret = CreateUrlCacheEntry(TEST_URL, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());

    create_and_write_file(filenameA, &zero_byte, sizeof(zero_byte));
    check_file_exists(filenameA);

    ret = CommitUrlCacheEntry(TEST_URL, filenameA, filetime_zero,
            filetime_zero, NORMAL_CACHE_ENTRY, (LPBYTE)ok_header,
            strlen(ok_header), "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    check_file_exists(filenameA);
    hFile = CreateFileA(filenameA, GENERIC_READ, 0, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA failed: %d\n",
       GetLastError());
    if (pDeleteUrlCacheEntryA)
    {
        /* DeleteUrlCacheEntryA should succeed.. */
        ret = pDeleteUrlCacheEntryA(TEST_URL);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
    }
    CloseHandle(hFile);
    if (pDeleteUrlCacheEntryA)
    {
        /* and a subsequent deletion should fail.. */
        ret = pDeleteUrlCacheEntryA(TEST_URL);
        ok(!ret, "Expected failure\n");
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }
    /* and the file should be untouched. */
    check_file_exists(filenameA);
    DeleteFileA(filenameA);

    /* Try creating a sticky entry.  Unlike non-sticky entries, the filename
     * must have been set already.
     */
    SetLastError(0xdeadbeef);
    ret = CommitUrlCacheEntry(TEST_URL, NULL, filetime_zero, filetime_zero,
            STICKY_CACHE_ENTRY, (LPBYTE)ok_header, strlen(ok_header), "html",
            NULL);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = CommitUrlCacheEntry(TEST_URL, NULL, filetime_zero, filetime_zero,
            NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY,
            (LPBYTE)ok_header, strlen(ok_header), "html", NULL);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    ret = CreateUrlCacheEntry(TEST_URL, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());
    create_and_write_file(filenameA, &zero_byte, sizeof(zero_byte));
    ret = CommitUrlCacheEntry(TEST_URL, filenameA, filetime_zero, filetime_zero,
            NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY,
            (LPBYTE)ok_header, strlen(ok_header), "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfo(TEST_URL, NULL, &cbCacheEntryInfo);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfo(TEST_URL, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(lpCacheEntryInfo->CacheEntryType & (NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY),
       "expected cache entry type NORMAL_CACHE_ENTRY | STICKY_CACHE_ENTRY, got %d (0x%08x)\n",
       lpCacheEntryInfo->CacheEntryType, lpCacheEntryInfo->CacheEntryType);
    ok(U(*lpCacheEntryInfo).dwExemptDelta == 86400,
       "expected dwExemptDelta 864000, got %d\n",
       U(*lpCacheEntryInfo).dwExemptDelta);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);
    if (pDeleteUrlCacheEntryA)
    {
        ret = pDeleteUrlCacheEntryA(TEST_URL);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
        /* When explicitly deleting the cache entry, the file is also deleted */
        todo_wine
        check_file_not_exists(filenameA);
    }
    /* Test once again, setting the exempt delta via SetUrlCacheEntryInfo */
    ret = CreateUrlCacheEntry(TEST_URL, 0, "html", filenameA, 0);
    ok(ret, "CreateUrlCacheEntry failed with error %d\n", GetLastError());
    create_and_write_file(filenameA, &zero_byte, sizeof(zero_byte));
    ret = CommitUrlCacheEntry(TEST_URL, filenameA, filetime_zero, filetime_zero,
            NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY,
            (LPBYTE)ok_header, strlen(ok_header), "html", NULL);
    ok(ret, "CommitUrlCacheEntry failed with error %d\n", GetLastError());
    cbCacheEntryInfo = 0;
    SetLastError(0xdeadbeef);
    ret = GetUrlCacheEntryInfo(TEST_URL, NULL, &cbCacheEntryInfo);
    ok(!ret, "RetrieveUrlCacheEntryFile should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    lpCacheEntryInfo = HeapAlloc(GetProcessHeap(), 0, cbCacheEntryInfo);
    ret = GetUrlCacheEntryInfo(TEST_URL, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(lpCacheEntryInfo->CacheEntryType & (NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY),
       "expected cache entry type NORMAL_CACHE_ENTRY | STICKY_CACHE_ENTRY, got %d (0x%08x)\n",
       lpCacheEntryInfo->CacheEntryType, lpCacheEntryInfo->CacheEntryType);
    ok(U(*lpCacheEntryInfo).dwExemptDelta == 86400,
       "expected dwExemptDelta 864000, got %d\n",
       U(*lpCacheEntryInfo).dwExemptDelta);
    U(*lpCacheEntryInfo).dwExemptDelta = 0;
    ret = SetUrlCacheEntryInfoA(TEST_URL, lpCacheEntryInfo,
            CACHE_ENTRY_EXEMPT_DELTA_FC);
    ok(ret, "SetUrlCacheEntryInfo failed: %d\n", GetLastError());
    ret = GetUrlCacheEntryInfo(TEST_URL, lpCacheEntryInfo, &cbCacheEntryInfo);
    ok(ret, "GetUrlCacheEntryInfo failed with error %d\n", GetLastError());
    ok(!U(*lpCacheEntryInfo).dwExemptDelta, "expected dwExemptDelta 0, got %d\n",
       U(*lpCacheEntryInfo).dwExemptDelta);
    /* See whether a sticky cache entry has the flag cleared once the exempt
     * delta is meaningless.
     */
    ok(lpCacheEntryInfo->CacheEntryType & (NORMAL_CACHE_ENTRY|STICKY_CACHE_ENTRY),
       "expected cache entry type NORMAL_CACHE_ENTRY | STICKY_CACHE_ENTRY, got %d (0x%08x)\n",
       lpCacheEntryInfo->CacheEntryType, lpCacheEntryInfo->CacheEntryType);
    HeapFree(GetProcessHeap(), 0, lpCacheEntryInfo);
    if (pDeleteUrlCacheEntryA)
    {
        ret = pDeleteUrlCacheEntryA(TEST_URL);
        ok(ret, "DeleteUrlCacheEntryA failed with error %d\n", GetLastError());
        todo_wine
        check_file_not_exists(filenameA);
    }
}

static void test_FindCloseUrlCache(void)
{
    BOOL r;
    DWORD err;

    SetLastError(0xdeadbeef);
    r = FindCloseUrlCache(NULL);
    err = GetLastError();
    ok(0 == r, "expected 0, got %d\n", r);
    ok(ERROR_INVALID_HANDLE == err, "expected %d, got %d\n", ERROR_INVALID_HANDLE, err);
}

static void test_GetDiskInfoA(void)
{
    BOOL ret;
    DWORD error, cluster_size;
    DWORDLONG free, total;
    char path[MAX_PATH], *p;

    GetSystemDirectoryA(path, MAX_PATH);
    if ((p = strchr(path, '\\'))) *++p = 0;

    ret = GetDiskInfoA(path, &cluster_size, &free, &total);
    ok(ret, "GetDiskInfoA failed %u\n", GetLastError());

    ret = GetDiskInfoA(path, &cluster_size, &free, NULL);
    ok(ret, "GetDiskInfoA failed %u\n", GetLastError());

    ret = GetDiskInfoA(path, &cluster_size, NULL, NULL);
    ok(ret, "GetDiskInfoA failed %u\n", GetLastError());

    ret = GetDiskInfoA(path, NULL, NULL, NULL);
    ok(ret, "GetDiskInfoA failed %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    strcpy(p, "\\non\\existing\\path");
    ret = GetDiskInfoA(path, NULL, NULL, NULL);
    error = GetLastError();
    ok(!ret ||
       broken(ret), /* < IE7 */
       "GetDiskInfoA succeeded\n");
    ok(error == ERROR_PATH_NOT_FOUND ||
       broken(error == 0xdeadbeef), /* < IE7 */
       "got %u expected ERROR_PATH_NOT_FOUND\n", error);

    SetLastError(0xdeadbeef);
    ret = GetDiskInfoA(NULL, NULL, NULL, NULL);
    error = GetLastError();
    ok(!ret, "GetDiskInfoA succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u expected ERROR_INVALID_PARAMETER\n", error);
}

START_TEST(urlcache)
{
    HMODULE hdll;
    hdll = GetModuleHandleA("wininet.dll");

    if(!GetProcAddress(hdll, "InternetGetCookieExW")) {
        win_skip("Too old IE (older than 6.0)\n");
        return;
    }

    pDeleteUrlCacheEntryA = (void*)GetProcAddress(hdll, "DeleteUrlCacheEntryA");
    pUnlockUrlCacheEntryFileA = (void*)GetProcAddress(hdll, "UnlockUrlCacheEntryFileA");
    test_urlcacheA();
    test_FindCloseUrlCache();
    test_GetDiskInfoA();
}
