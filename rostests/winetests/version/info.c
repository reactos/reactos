/*
 * Copyright (C) 2004 Stefan Leichter
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
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winver.h"
#include "wine/test.h"

#define MY_LAST_ERROR ((DWORD)-1)
#define EXPECT_BAD_PATH__NOT_FOUND \
    ok( (ERROR_PATH_NOT_FOUND == GetLastError()) || \
	(ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError()) || \
	(ERROR_FILE_NOT_FOUND == GetLastError()) || \
	(ERROR_BAD_PATHNAME == GetLastError()) || \
        (ERROR_SUCCESS == GetLastError()), \
	"Last error wrong! ERROR_RESOURCE_DATA_NOT_FOUND/ERROR_BAD_PATHNAME (98)/" \
	"ERROR_PATH_NOT_FOUND (NT4)/ERROR_FILE_NOT_FOUND (2k3) " \
        "ERROR_SUCCESS (2k) expected, got %u\n", GetLastError());
#define EXPECT_INVALID__NOT_FOUND \
    ok( (ERROR_PATH_NOT_FOUND == GetLastError()) || \
	(ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError()) || \
	(ERROR_FILE_NOT_FOUND == GetLastError()) || \
	(ERROR_INVALID_PARAMETER == GetLastError()) || \
        (ERROR_SUCCESS == GetLastError()), \
	"Last error wrong! ERROR_RESOURCE_DATA_NOT_FOUND/ERROR_INVALID_PARAMETER (98)/" \
	"ERROR_PATH_NOT_FOUND (NT4)/ERROR_FILE_NOT_FOUND (2k3) " \
	"ERROR_SUCCESS (2k) expected, got %u\n", GetLastError());

static void create_file(const CHAR *name)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, name, strlen(name), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);
    CloseHandle(file);
}

static void test_info_size(void)
{   DWORD hdl, retval;
    char mypath[MAX_PATH] = "";

    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( NULL, NULL);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08x\n",
	retval);
    EXPECT_INVALID__NOT_FOUND;

    hdl = 0x55555555;
    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( NULL, &hdl);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08x\n",
	retval);
    EXPECT_INVALID__NOT_FOUND;
    ok( hdl == 0L,
	"Handle wrong! 0L expected, got 0x%08x\n", hdl);

    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "", NULL);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08x\n",
	retval);
    EXPECT_BAD_PATH__NOT_FOUND;

    hdl = 0x55555555;
    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "", &hdl);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08x\n",
	retval);
    EXPECT_BAD_PATH__NOT_FOUND;
    ok( hdl == 0L,
	"Handle wrong! 0L expected, got 0x%08x\n", hdl);

    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "kernel32.dll", NULL);
    ok( retval,
	"GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08x\n",
	retval);
    ok((NO_ERROR == GetLastError()) || (MY_LAST_ERROR == GetLastError()),
	"Last error wrong! NO_ERROR/0x%08x (NT4)  expected, got %u\n",
	MY_LAST_ERROR, GetLastError());

    hdl = 0x55555555;
    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "kernel32.dll", &hdl);
    ok( retval,
	"GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08x\n",
	retval);
    ok((NO_ERROR == GetLastError()) || (MY_LAST_ERROR == GetLastError()),
	"Last error wrong! NO_ERROR/0x%08x (NT4)  expected, got %u\n",
	MY_LAST_ERROR, GetLastError());
    ok( hdl == 0L,
	"Handle wrong! 0L expected, got 0x%08x\n", hdl);

    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "notexist.dll", NULL);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08x\n",
	retval);
    ok( (ERROR_FILE_NOT_FOUND == GetLastError()) ||
	(ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError()) ||
	(MY_LAST_ERROR == GetLastError()) ||
	(ERROR_SUCCESS == GetLastError()), /* win2k */
	"Last error wrong! ERROR_FILE_NOT_FOUND/ERROR_RESOURCE_DATA_NOT_FOUND "
	"(XP)/0x%08x (NT4) expected, got %u\n", MY_LAST_ERROR, GetLastError());

    /* test a currently loaded executable */
    if(GetModuleFileNameA(NULL, mypath, MAX_PATH)) {
	hdl = 0x55555555;
	SetLastError(MY_LAST_ERROR);
	retval = GetFileVersionInfoSizeA( mypath, &hdl);
	ok( retval,
            "GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08x\n",
	    retval);
	ok((NO_ERROR == GetLastError()) || (MY_LAST_ERROR == GetLastError()),
            "Last error wrong! NO_ERROR/0x%08x (NT4)  expected, got %u\n",
	    MY_LAST_ERROR, GetLastError());
	ok( hdl == 0L,
            "Handle wrong! 0L expected, got 0x%08x\n", hdl);
    }
    else
	trace("skipping GetModuleFileNameA(NULL,..) failed\n");

    /* test a not loaded executable */
    if(GetSystemDirectoryA(mypath, MAX_PATH)) {
	lstrcatA(mypath, "\\regsvr32.exe");

	if(INVALID_FILE_ATTRIBUTES == GetFileAttributesA(mypath))
	    trace("GetFileAttributesA(%s) failed\n", mypath);
	else {
	    hdl = 0x55555555;
	    SetLastError(MY_LAST_ERROR);
	    retval = GetFileVersionInfoSizeA( mypath, &hdl);
	    ok( retval,
		"GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08x\n",
		retval);
	    ok((NO_ERROR == GetLastError()) || (MY_LAST_ERROR == GetLastError()),
		"Last error wrong! NO_ERROR/0x%08x (NT4)  expected, got %u\n",
		MY_LAST_ERROR, GetLastError());
	    ok( hdl == 0L,
		"Handle wrong! 0L expected, got 0x%08x\n", hdl);
	}
    }
    else
	trace("skipping GetSystemDirectoryA(mypath,..) failed\n");

    create_file("test.txt");

    /* no version info */
    SetLastError(0xdeadbeef);
    hdl = 0xcafe;
    retval = GetFileVersionInfoSizeA("test.txt", &hdl);
    ok(retval == 0, "Expected 0, got %d\n", retval);
    ok(hdl == 0, "Expected 0, got %d\n", hdl);
    ok(GetLastError() == ERROR_RESOURCE_DATA_NOT_FOUND ||
       GetLastError() == ERROR_SUCCESS, /* win2k */
       "Expected ERROR_RESOURCE_DATA_NOT_FOUND, got %d\n", GetLastError());

    DeleteFileA("test.txt");
}

static void VersionDwordLong2String(DWORDLONG Version, LPSTR lpszVerString)
{
    WORD a, b, c, d;

    a = (WORD)(Version >> 48);
    b = (WORD)((Version >> 32) & 0xffff);
    c = (WORD)((Version >> 16) & 0xffff);
    d = (WORD)(Version & 0xffff);

    sprintf(lpszVerString, "%d.%d.%d.%d", a, b, c, d);
}

static void test_info(void)
{
    DWORD hdl, retval;
    PVOID pVersionInfo = NULL;
    BOOL boolret;
    VS_FIXEDFILEINFO *pFixedVersionInfo;
    UINT uiLength;
    char VersionString[MAX_PATH];
    static const char backslash[] = "\\";
    DWORDLONG dwlVersion;

    hdl = 0x55555555;
    SetLastError(MY_LAST_ERROR);
    retval = GetFileVersionInfoSizeA( "kernel32.dll", &hdl);
    ok( retval,
	"GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08x\n",
	retval);
    ok((NO_ERROR == GetLastError()) || (MY_LAST_ERROR == GetLastError()),
	"Last error wrong! NO_ERROR/0x%08x (NT4)  expected, got %u\n",
	MY_LAST_ERROR, GetLastError());
    ok( hdl == 0L,
	"Handle wrong! 0L expected, got 0x%08x\n", hdl);

    if ( retval == 0 || hdl != 0)
        return;

    pVersionInfo = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, retval );
    ok(pVersionInfo != 0, "HeapAlloc failed\n" );
    if (pVersionInfo == 0)
        return;

    if (0)
    {
    /* this test crashes on WinNT4
     */
    boolret = GetFileVersionInfoA( "kernel32.dll", 0, retval, 0);
    ok (!boolret, "GetFileVersionInfoA should have failed: GetLastError = %u\n", GetLastError());
    ok ((GetLastError() == ERROR_INVALID_DATA) || (GetLastError() == ERROR_BAD_PATHNAME) ||
	(GetLastError() == NO_ERROR),
        "Last error wrong! ERROR_INVALID_DATA/ERROR_BAD_PATHNAME (ME)/"
	"NO_ERROR (95) expected, got %u\n",
        GetLastError());
    }

    boolret = GetFileVersionInfoA( "kernel32.dll", 0, retval, pVersionInfo );
    ok (boolret, "GetFileVersionInfoA failed: GetLastError = %u\n", GetLastError());
    if (!boolret)
        goto cleanup;

    boolret = VerQueryValueA( pVersionInfo, NULL, (LPVOID *)&pFixedVersionInfo, &uiLength );
    ok (boolret || GetLastError() == NO_ERROR /* Win98 */,
       "VerQueryValueA failed: GetLastError = %u\n", GetLastError());

    boolret = VerQueryValueA( pVersionInfo, "", (LPVOID *)&pFixedVersionInfo, &uiLength );
    ok (boolret, "VerQueryValueA failed: GetLastError = %u\n", GetLastError());

    boolret = VerQueryValueA( pVersionInfo, backslash, (LPVOID *)&pFixedVersionInfo, &uiLength );
    ok (boolret, "VerQueryValueA failed: GetLastError = %u\n", GetLastError());
    if (!boolret)
        goto cleanup;

    dwlVersion = (((DWORDLONG)pFixedVersionInfo->dwFileVersionMS) << 32) +
        pFixedVersionInfo->dwFileVersionLS;

    VersionDwordLong2String(dwlVersion, VersionString);

    trace("kernel32.dll version: %s\n", VersionString);

    if (0)
    {
    /* this test crashes on WinNT4
     */
    boolret = VerQueryValueA( pVersionInfo, backslash, (LPVOID *)&pFixedVersionInfo, 0);
    ok (boolret, "VerQueryValue failed: GetLastError = %u\n", GetLastError());
    }

cleanup:
    HeapFree( GetProcessHeap(), 0, pVersionInfo);
}

static void test_32bit_win(void)
{
    DWORD hdlA, retvalA;
    DWORD hdlW, retvalW = 0;
    BOOL retA,retW;
    PVOID pVersionInfoA = NULL;
    PVOID pVersionInfoW = NULL;
    char *pBufA;
    WCHAR *pBufW;
    UINT uiLengthA, uiLengthW;
    char mypathA[MAX_PATH];
    WCHAR mypathW[MAX_PATH];
    char rootA[] = "\\";
    WCHAR rootW[] = { '\\', 0 };
    WCHAR emptyW[] = { 0 };
    char varfileinfoA[] = "\\VarFileInfo\\Translation";
    WCHAR varfileinfoW[]    = { '\\','V','a','r','F','i','l','e','I','n','f','o',
                                '\\','T','r','a','n','s','l','a','t','i','o','n', 0 };
    char WineVarFileInfoA[] = { 0x09, 0x04, 0xE4, 0x04 };
    char FileDescriptionA[] = "\\StringFileInfo\\040904E4\\FileDescription";
    WCHAR FileDescriptionW[] = { '\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o',
                                '\\','0','4','0','9','0','4','E','4',
                                '\\','F','i','l','e','D','e','s','c','r','i','p','t','i','o','n', 0 };
    char WineFileDescriptionA[] = "FileDescription";
    WCHAR WineFileDescriptionW[] = { 'F','i','l','e','D','e','s','c','r','i','p','t','i','o','n', 0 };
    BOOL is_unicode_enabled = TRUE;

    /* A copy from dlls/version/info.c */
    typedef struct
    {
        WORD  wLength;
        WORD  wValueLength;
        WORD  wType;
        WCHAR szKey[1];
#if 0   /* variable length structure */
        /* DWORD aligned */
        BYTE  Value[];
        /* DWORD aligned */
        VS_VERSION_INFO_STRUCT32 Children[];
#endif
    } VS_VERSION_INFO_STRUCT32;

    /* If we call GetFileVersionInfoA on a system that supports Unicode, NT/W2K/XP/W2K3 (by default) and Wine,
     * the versioninfo will contain Unicode strings.
     * Part of the test is to call both the A and W versions, which should have the same Version Information
     * for some requests, on systems that support both calls.
     */

    /* First get the versioninfo via the W versions */
    SetLastError(0xdeadbeef);
    GetModuleFileNameW(NULL, mypathW, MAX_PATH);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetModuleFileNameW not existing on this platform, skipping comparison between A- and W-calls\n");
        is_unicode_enabled = FALSE;
    }

    if (is_unicode_enabled)
    { 
        retvalW = GetFileVersionInfoSizeW( mypathW, &hdlW);
        pVersionInfoW = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, retvalW );
        retW = GetFileVersionInfoW( mypathW, 0, retvalW, pVersionInfoW );
        ok(retW, "GetFileVersionInfo failed: GetLastError = %u\n", GetLastError());
    }

    GetModuleFileNameA(NULL, mypathA, MAX_PATH);
    retvalA = GetFileVersionInfoSizeA( mypathA, &hdlA);
    pVersionInfoA = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, retvalA );
    retA = GetFileVersionInfoA( mypathA, 0, retvalA, pVersionInfoA );
    ok(retA, "GetFileVersionInfo failed: GetLastError = %u\n", GetLastError());

    if (is_unicode_enabled)
    { 
        ok( retvalA == retvalW, "The size of the struct should be the same for both A/W calls, it is (%d) vs. (%d)\n",
                                retvalA, retvalW);
        ok( !memcmp(pVersionInfoA, pVersionInfoW, retvalA), "Both structs should be the same, they aren't\n");
    }

    /* The structs on Windows are bigger than just the struct for the basic information. The total struct
     * contains also an empty part, which is used for converted strings. The converted strings are a result
     * of calling VerQueryValueA on a 32bit resource and calling VerQueryValueW on a 16bit resource.
     * The first WORD of the structure (wLength) shows the size of the base struct. The total struct size depends
     * on the Windows version:
     *
     * 16bits resource (numbers are from a sample app):
     *
     * Windows Version    Retrieved with A/W    wLength        StructSize
     * ====================================================================================
     * Win98              A                     0x01B4 (436)   436
     * NT4                A/W                   0x01B4 (436)   2048 ???
     * W2K/XP/W2K3        A/W                   0x01B4 (436)   1536 which is (436 - sizeof(VS_FIXEDFILEINFO)) * 4
     *
     * 32bits resource (numbers are from this test executable version_crosstest.exe):
     * Windows Version    Retrieved with A/W    wLength        StructSize
     * =============================================================
     * Win98              A                     0x01E0 (480)   848 (structure data doesn't seem correct)
     * NT4                A/W                   0x0350 (848)   1272 (848 * 1.5)
     * W2K/XP/W2K3        A/W                   0x0350 (848)   1700 which is (848 * 2) + 4 
     *
     * Wine will follow the implementation (eventually) of W2K/XP/W2K3
     */

    /* Now some tests for the above (only if we are unicode enabled) */

    if (is_unicode_enabled)
    { 
        VS_VERSION_INFO_STRUCT32 *vvis = pVersionInfoW;
        ok ( retvalW == ((vvis->wLength * 2) + 4) || retvalW == (vvis->wLength * 1.5),
             "Structure is not of the correct size\n");
    }

    /* Although the 32bit resource structures contain Unicode strings, VerQueryValueA will always return normal strings,
     * VerQueryValueW will always return Unicode ones. (That means everything returned for StringFileInfo requests).
     */

    /* Get the VS_FIXEDFILEINFO information, this must be the same for both A- and W-Calls */ 

    retA = VerQueryValueA( pVersionInfoA, rootA, (LPVOID *)&pBufA, &uiLengthA );
    ok (retA, "VerQueryValueA failed: GetLastError = %u\n", GetLastError());
    ok ( uiLengthA == sizeof(VS_FIXEDFILEINFO), "Size (%d) doesn't match the size of the VS_FIXEDFILEINFO struct\n", uiLengthA);

    if (is_unicode_enabled)
    { 
        if(0)
        {   /* This causes Vista and w2k8 to crash */
            retW = VerQueryValueW( pVersionInfoW, NULL, (LPVOID *)&pBufW, &uiLengthW );
            ok (retW, "VerQueryValueW failed: GetLastError = %u\n", GetLastError());
        }

        retW = VerQueryValueW( pVersionInfoW, emptyW, (LPVOID *)&pBufW, &uiLengthW );
        ok (retW, "VerQueryValueW failed: GetLastError = %u\n", GetLastError());

        retW = VerQueryValueW( pVersionInfoW, rootW, (LPVOID *)&pBufW, &uiLengthW );
        ok (retW, "VerQueryValueW failed: GetLastError = %u\n", GetLastError());
        ok ( uiLengthW == sizeof(VS_FIXEDFILEINFO), "Size (%d) doesn't match the size of the VS_FIXEDFILEINFO struct\n", uiLengthW );

        ok( uiLengthA == uiLengthW, "The size of VS_FIXEDFILEINFO should be the same for both A/W calls, it is (%d) vs. (%d)\n",
                                    uiLengthA, uiLengthW);
        ok( !memcmp(pBufA, pBufW, uiLengthA), "Both values should be the same, they aren't\n");
    }

    /* Get some VarFileInfo information, this must be the same for both A- and W-Calls */

    retA = VerQueryValueA( pVersionInfoA, varfileinfoA, (LPVOID *)&pBufA, &uiLengthA );
    ok (retA, "VerQueryValueA failed: GetLastError = %u\n", GetLastError());
    ok( !memcmp(pBufA, WineVarFileInfoA, uiLengthA), "The VarFileInfo should have matched 0904e404 (non case sensitive)\n");

    if (is_unicode_enabled)
    { 
        retW = VerQueryValueW( pVersionInfoW, varfileinfoW, (LPVOID *)&pBufW, &uiLengthW );
        ok (retW, "VerQueryValueW failed: GetLastError = %u\n", GetLastError());
        ok( uiLengthA == uiLengthW, "The size of the VarFileInfo information should be the same for both A/W calls, it is (%d) vs. (%d)\n",
                                    uiLengthA, uiLengthW);
        ok( !memcmp(pBufA, pBufW, uiLengthA), "Both values should be the same, they aren't\n");
    }

    /* Get some StringFileInfo information, this will be ANSI for A-Calls and Unicode for W-Calls */

    retA = VerQueryValueA( pVersionInfoA, FileDescriptionA, (LPVOID *)&pBufA, &uiLengthA );
    ok (retA, "VerQueryValueA failed: GetLastError = %u\n", GetLastError());
    ok( !lstrcmpA(WineFileDescriptionA, pBufA), "expected '%s' got '%s'\n",
        WineFileDescriptionA, pBufA);

    /* Test a second time */
    retA = VerQueryValueA( pVersionInfoA, FileDescriptionA, (LPVOID *)&pBufA, &uiLengthA );
    ok (retA, "VerQueryValueA failed: GetLastError = %u\n", GetLastError());
    ok( !lstrcmpA(WineFileDescriptionA, pBufA), "expected '%s' got '%s'\n",
        WineFileDescriptionA, pBufA);

    if (is_unicode_enabled)
    { 
        retW = VerQueryValueW( pVersionInfoW, FileDescriptionW, (LPVOID *)&pBufW, &uiLengthW );
        ok (retW, "VerQueryValueW failed: GetLastError = %u\n", GetLastError());
        ok( !lstrcmpW(WineFileDescriptionW, pBufW), "FileDescription should have been '%s'\n", WineFileDescriptionA);
    }

    HeapFree( GetProcessHeap(), 0, pVersionInfoA);
    if (is_unicode_enabled)
        HeapFree( GetProcessHeap(), 0, pVersionInfoW);
}

static void test_VerQueryValueA(void)
{
    static const char * const value_name[] = {
        "Product", "CompanyName", "FileDescription", "Internal",
        "ProductVersion", "InternalName", "File", "LegalCopyright",
        "FileVersion", "Legal", "OriginalFilename", "ProductName",
        "Company", "Original" };
    char *ver, *p;
    UINT len, ret, translation, i;
    char buf[MAX_PATH];

    ret = GetModuleFileNameA(NULL, buf, sizeof(buf));
    assert(ret);

    SetLastError(0xdeadbeef);
    len = GetFileVersionInfoSizeA(buf, NULL);
    ok(len, "GetFileVersionInfoSizeA(%s) error %u\n", buf, GetLastError());

    ver = HeapAlloc(GetProcessHeap(), 0, len);
    assert(ver);

    SetLastError(0xdeadbeef);
    ret = GetFileVersionInfoA(buf, 0, len, ver);
    ok(ret, "GetFileVersionInfoA error %u\n", GetLastError());

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueA(ver, "\\VarFileInfo\\Translation", (LPVOID*)&p, &len);
    ok(ret, "VerQueryValue error %u\n", GetLastError());
    ok(len == 4, "VerQueryValue returned %u, expected 4\n", len);

    translation = *(UINT *)p;
    translation = MAKELONG(HIWORD(translation), LOWORD(translation));

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueA(ver, "String", (LPVOID*)&p, &len);
    ok(!ret, "VerQueryValue should fail\n");
    ok(GetLastError() == ERROR_RESOURCE_TYPE_NOT_FOUND ||
       GetLastError() == 0xdeadbeef /* NT4, W2K */,
       "VerQueryValue returned %u\n", GetLastError());
    ok(p == (char *)0xdeadbeef, "expected 0xdeadbeef got %p\n", p);
    ok(len == 0, "expected 0 got %x\n", len);

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueA(ver, "StringFileInfo", (LPVOID*)&p, &len);
    ok(ret, "VerQueryValue error %u\n", GetLastError());
    ok(len == 0, "VerQueryValue returned %u, expected 0\n", len);
    ok(p != (char *)0xdeadbeef, "not expected 0xdeadbeef\n");

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueA(ver, "\\StringFileInfo", (LPVOID*)&p, &len);
    ok(ret, "VerQueryValue error %u\n", GetLastError());
    ok(len == 0, "VerQueryValue returned %u, expected 0\n", len);
    ok(p != (char *)0xdeadbeef, "not expected 0xdeadbeef\n");

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueA(ver, "\\\\StringFileInfo", (LPVOID*)&p, &len);
    ok(ret, "VerQueryValue error %u\n", GetLastError());
    ok(len == 0, "VerQueryValue returned %u, expected 0\n", len);
    ok(p != (char *)0xdeadbeef, "not expected 0xdeadbeef\n");

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueA(ver, "\\StringFileInfo\\\\", (LPVOID*)&p, &len);
    ok(ret, "VerQueryValue error %u\n", GetLastError());
    ok(len == 0, "VerQueryValue returned %u, expected 0\n", len);
    ok(p != (char *)0xdeadbeef, "not expected 0xdeadbeef\n");

    sprintf(buf, "\\StringFileInfo\\%08x", translation);
    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueA(ver, buf, (LPVOID*)&p, &len);
    ok(ret, "VerQueryValue error %u\n", GetLastError());
    ok(len == 0, "VerQueryValue returned %u, expected 0\n", len);
    ok(p != (char *)0xdeadbeef, "not expected 0xdeadbeef\n");

    for (i = 0; i < sizeof(value_name)/sizeof(value_name[0]); i++)
    {
	sprintf(buf, "\\StringFileInfo\\%08x\\%s", translation, value_name[i]);
        p = (char *)0xdeadbeef;
        len = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VerQueryValueA(ver, buf, (LPVOID*)&p, &len);
        ok(ret, "VerQueryValueA(%s) error %u\n", buf, GetLastError());
        ok(len == strlen(value_name[i]) + 1, "VerQueryValue returned %u\n", len);
        ok(!strcmp(value_name[i], p), "expected \"%s\", got \"%s\"\n",
           value_name[i], p);

        /* test partial value names */
        len = lstrlenA(buf);
        buf[len - 2] = 0;
        p = (char *)0xdeadbeef;
        len = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VerQueryValueA(ver, buf, (LPVOID*)&p, &len);
        ok(!ret, "VerQueryValueA(%s) succeeded\n", buf);
        ok(GetLastError() == ERROR_RESOURCE_TYPE_NOT_FOUND ||
           GetLastError() == 0xdeadbeef /* NT4, W2K */,
           "VerQueryValue returned %u\n", GetLastError());
        ok(p == (char *)0xdeadbeef, "expected 0xdeadbeef got %p\n", p);
        ok(len == 0, "expected 0 or 0xbeef, got %x\n", len);
    }

    HeapFree(GetProcessHeap(), 0, ver);
}

static void test_VerQueryValue_InvalidLength(void)
{
    /* this buffer is created with the reactos resource compiler from this resource:
#include "winver.h"

VS_VERSION_INFO VERSIONINFO
FILEVERSION    1,0,0,0
PRODUCTVERSION 1,0,0,0
FILEFLAGSMASK  63
FILEFLAGS      0
FILEOS         VOS_UNKNOWN
FILETYPE       VFT_APP
FILESUBTYPE    VFT2_UNKNOWN
{
    BLOCK "StringFileInfo"
    {
    }
}
*/
    char preparedbuffer[] = {
        /* VS_VERSION_INFO_STRUCT32 */
        0x80, 0x00,     /* wLength */
        0x34, 0x00,     /* wValueLength */
        0x00, 0x00,     /* wType */
        /* L"VS_VERSION_INFO" + DWORD alignment */
        0x56, 0x00, 0x53, 0x00, 0x5f, 0x00, 0x56, 0x00, 0x45, 0x00, 0x52, 0x00, 0x53, 0x00, 0x49, 0x00, 0x4f,
        0x00, 0x4e, 0x00, 0x5f, 0x00, 0x49, 0x00, 0x4e, 0x00, 0x46, 0x00, 0x4f, 0x00, 0x00, 0x00, 0x00, 0x00,

        /* VS_FIXEDFILEINFO */
        0xbd, 0x04, 0xef, 0xfe,     /* dwSignature */
        0x00, 0x00, 0x01, 0x00,     /* dwStrucVersion */
        0x00, 0x00, 0x01, 0x00,     /* dwFileVersionMS */
        0x00, 0x00, 0x00, 0x00,     /* dwFileVersionLS */
        0x00, 0x00, 0x01, 0x00,     /* dwProductVersionMS */
        0x00, 0x00, 0x00, 0x00,     /* dwProductVersionLS */
        0x3f, 0x00, 0x00, 0x00,     /* dwFileFlagsMask */
        0x00, 0x00, 0x00, 0x00,     /* dwFileFlags */
        0x00, 0x00, 0x00, 0x00,     /* dwFileOS */
        0x01, 0x00, 0x00, 0x00,     /* dwFileType */
        0x00, 0x00, 0x00, 0x00,     /* dwFileSubtype */
        0x00, 0x00, 0x00, 0x00,     /* dwFileDateMS */
        0x00, 0x00, 0x00, 0x00,     /* dwFileDateLS */

        /* first child: */
            0x24, 0x00,     /* wLength */
            0x00, 0x00,     /* wValueLength */
            0x01, 0x00,     /* wType */
            /* L"StringFileInfo" + DWORD alignment */
            0x53, 0x00, 0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x67, 0x00, 0x46, 0x00, 0x69, 0x00,
            0x6c, 0x00, 0x65, 0x00, 0x49, 0x00, 0x6e, 0x00, 0x66, 0x00, 0x6f, 0x00, 0x00, 0x00,
            /* "FE2X" */
            0x46, 0x45, 0x32, 0x58,

            /* Extra bytes allocated for W->A conversions. */
            0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba,
            0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba,
            0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba,
            0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba,
            0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba,
            0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba,
            0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba,
            0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba, 0x0d, 0xf0, 0xad, 0xba,
    };
    char *p;
    UINT len, ret;
    WCHAR FileDescriptionW[] = { '\\', '\\', 'S', 't', 'r', 'i', 'n', 'g', 'F', 'i', 'l', 'e', 'I', 'n', 'f', 'o', 0 };

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueA(preparedbuffer, "StringFileInfo", (LPVOID *)&p, &len);
    ok(ret, "VerQueryValueA error %u\n", GetLastError());
    todo_wine
    ok(len == 0, "VerQueryValueA returned %u, expected 0\n", len);
    todo_wine
    ok(p == preparedbuffer + 0x7e, "p was %p, expected %p\n", p, preparedbuffer + 0x7e);

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueA(preparedbuffer, "\\StringFileInfo", (LPVOID *)&p, &len);
    ok(ret, "VerQueryValueA error %u\n", GetLastError());
    todo_wine
    ok(len == 0, "VerQueryValueA returned %u, expected 0\n", len);
    todo_wine
    ok(p == preparedbuffer + 0x7e, "p was %p, expected %p\n", p, preparedbuffer + 0x7e);

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueA(preparedbuffer, "\\\\StringFileInfo", (LPVOID *)&p, &len);
    ok(ret, "VerQueryValueA error %u\n", GetLastError());
    todo_wine
    ok(len == 0, "VerQueryValueA returned %u, expected 0\n", len);
    todo_wine
    ok(p == preparedbuffer + 0x7e, "p was %p, expected %p\n", p, preparedbuffer + 0x7e);

    /* also test the W versions. */
    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueW(preparedbuffer, FileDescriptionW + 2, (LPVOID *)&p, &len);
    ok(ret, "VerQueryValueW error %u\n", GetLastError());
    ok(len == 0, "VerQueryValueW returned %u, expected 0\n", len);
    todo_wine
    ok(p == preparedbuffer + 0x7e, "p was %p, expected %p\n", p, preparedbuffer + 0x7e);

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueW(preparedbuffer, FileDescriptionW + 1, (LPVOID *)&p, &len);
    ok(ret, "VerQueryValueW error %u\n", GetLastError());
    ok(len == 0, "VerQueryValueW returned %u, expected 0\n", len);
    todo_wine
    ok(p == preparedbuffer + 0x7e, "p was %p, expected %p\n", p, preparedbuffer + 0x7e);

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VerQueryValueW(preparedbuffer, FileDescriptionW, (LPVOID *)&p, &len);
    ok(ret, "VerQueryValueW error %u\n", GetLastError());
    ok(len == 0, "VerQueryValueW returned %u, expected 0\n", len);
    todo_wine
    ok(p == preparedbuffer + 0x7e, "p was %p, expected %p\n", p, preparedbuffer + 0x7e);
}

static void test_extra_block(void)
{
    WORD extra_block[] = {
        72, 0, 0, 'W', 'i', 'n', 'e', 'T', 'e', 's', 't', '\0',
        24, 4, 0, 'B', 'i', 'n', 'a', 'r', 'y', '\0', 0xbeef, 0xdead,
        24, 4, 1, 'T', 'e', 'x', 't', '\0', 'B', '-', ')', '\0',
    };
    char buf[MAX_PATH];
    UINT len, ret;
    ULONG w;
    char *ver, *p;
    WORD *length;

    ret = GetModuleFileNameA(NULL, buf, sizeof(buf));
    ok(ret, "GetModuleFileNameA failed\n");

    len = GetFileVersionInfoSizeA(buf, NULL);
    ok(len, "GetFileVersionInfoSizeA(%s) error %u\n", buf, GetLastError());

    ver = HeapAlloc(GetProcessHeap(), 0, len + sizeof(extra_block) * 2);
    ok(ver != NULL, "Can't allocate memory\n");

    ret = GetFileVersionInfoA(buf, 0, len, ver);
    ok(ret, "GetFileVersionInfoA error %u\n", GetLastError());

    /* forge the string table, as windres dislike an extra block */
    length = (WORD *)ver; /* see VS_VERSION_INFO_STRUCT32 for details */
    memcpy(ver + *length, extra_block, sizeof(extra_block));
    *length += sizeof(extra_block);

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    w = 0xdeadbeef;
    ret = VerQueryValueA(ver, "WineTest\\Binary", (LPVOID*)&p, &len);
    ok(ret, "VerQueryValue error %u\n", GetLastError());
    ok(len == 4, "VerQueryValue returned %u, expected 4\n", len);
    ok(p != (char *)0xdeadbeef, "not expected 0xdeadbeef\n");
    ok(memcmp(p, &w, sizeof(w)) == 0, "got 0x%08x, expected 0x%08x\n", *(PULONG)p, w);

    p = (char *)0xdeadbeef;
    len = 0xdeadbeef;
    ret = VerQueryValueA(ver, "WineTest\\Text", (LPVOID*)&p, &len);
    ok(ret, "VerQueryValue error %u\n", GetLastError());
    ok(len == 4, "VerQueryValue returned %u, expected 4\n", len);
    ok(p != (char *)0xdeadbeef, "not expected 0xdeadbeef\n");
    ok(strcmp(p, "B-)") == 0, "got '%s', expected '%s'\n", p, "B-)");

    HeapFree(GetProcessHeap(), 0, ver);
}

START_TEST(info)
{
    test_info_size();
    test_info();
    test_32bit_win();
    test_VerQueryValueA();
    test_VerQueryValue_InvalidLength();
    test_extra_block();
}
