/*
 * Unit tests for module/DLL/library API
 *
 * Copyright (c) 2004 Eric Pouech
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/test.h"
#include <windows.h>

static BOOL is_unicode_enabled = TRUE;

static BOOL cmpStrAW(const char* a, const WCHAR* b, DWORD lenA, DWORD lenB)
{
    WCHAR       aw[1024];

    DWORD len = MultiByteToWideChar( AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0,
                                     a, lenA, aw, sizeof(aw) / sizeof(aw[0]) );
    if (len != lenB) return FALSE;
    return memcmp(aw, b, len * sizeof(WCHAR)) == 0;
}

static void testGetModuleFileName(const char* name)
{
    HMODULE     hMod;
    char        bufA[MAX_PATH];
    WCHAR       bufW[MAX_PATH];
    DWORD       len1A, len1W = 0, len2A, len2W = 0;

    hMod = (name) ? GetModuleHandle(name) : NULL;

    /* first test, with enough space in buffer */
    memset(bufA, '-', sizeof(bufA));
    len1A = GetModuleFileNameA(hMod, bufA, sizeof(bufA));
    ok(len1A > 0, "Getting module filename for handle %p\n", hMod);

    if (is_unicode_enabled)
    {
        memset(bufW, '-', sizeof(bufW));
        len1W = GetModuleFileNameW(hMod, bufW, sizeof(bufW) / sizeof(WCHAR));
        ok(len1W > 0, "Getting module filename for handle %p\n", hMod);
    }

    ok(len1A == strlen(bufA), "Unexpected length of GetModuleFilenameA (%ld/%d)\n", len1A, strlen(bufA));

    if (is_unicode_enabled)
    {
        ok(len1W == lstrlenW(bufW), "Unexpected length of GetModuleFilenameW (%ld/%d)\n", len1W, lstrlenW(bufW));
        ok(cmpStrAW(bufA, bufW, len1A, len1W), "Comparing GetModuleFilenameAW results\n");
    }

    /* second test with a buffer too small */
    memset(bufA, '-', sizeof(bufA));
    len2A = GetModuleFileNameA(hMod, bufA, len1A / 2);
    ok(len2A > 0, "Getting module filename for handle %p\n", hMod);

    if (is_unicode_enabled)
    {
        memset(bufW, '-', sizeof(bufW));
        len2W = GetModuleFileNameW(hMod, bufW, len1W / 2);
        ok(len2W > 0, "Getting module filename for handle %p\n", hMod);
        ok(cmpStrAW(bufA, bufW, len2A, len2W), "Comparing GetModuleFilenameAW results with buffer too small\n" );
        ok(len1W / 2 == len2W, "Correct length in GetModuleFilenameW with buffer too small (%ld/%ld)\n", len1W / 2, len2W);
    }

    ok(len1A / 2 == len2A ||
       len1A / 2 == len2A + 1, /* Win9x */
       "Correct length in GetModuleFilenameA with buffer too small (%ld/%ld)\n", len1A / 2, len2A);
}

static void testGetModuleFileName_Wrong(void)
{
    char        bufA[MAX_PATH];
    WCHAR       bufW[MAX_PATH];

    /* test wrong handle */
    if (is_unicode_enabled)
    {
        bufW[0] = '*';
        ok(GetModuleFileNameW((void*)0xffffffff, bufW, sizeof(bufW) / sizeof(WCHAR)) == 0, "Unexpected success in module handle\n");
        ok(bufW[0] == '*', "When failing, buffer shouldn't be written to\n");
    }

    bufA[0] = '*';
    ok(GetModuleFileNameA((void*)0xffffffff, bufA, sizeof(bufA)) == 0, "Unexpected success in module handle\n");
    ok(bufA[0] == '*' ||
       bufA[0] == 0 /* Win9x */,
       "When failing, buffer shouldn't be written to\n");
}

static void testLoadLibraryA(void)
{
    HMODULE hModule, hModule1;
    FARPROC fp;

    SetLastError(0xdeadbeef);
    hModule = LoadLibraryA("kernel32.dll");
    ok( hModule != NULL, "kernel32.dll should be loadable\n");
    ok( GetLastError() == 0xdeadbeef, "GetLastError should be 0xdeadbeef but is %08lx\n", GetLastError());

    fp = GetProcAddress(hModule, "CreateFileA");
    ok( fp != NULL, "CreateFileA should be there\n");
    ok( GetLastError() == 0xdeadbeef, "GetLastError should be 0xdeadbeef but is %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    hModule1 = LoadLibraryA("kernel32   ");
    /* Only winNT does this */
    if (GetLastError() != ERROR_DLL_NOT_FOUND)
    {
        ok( hModule1 != NULL, "\"kernel32   \" should be loadable\n");
        ok( GetLastError() == 0xdeadbeef, "GetLastError should be 0xdeadbeef but is %08lx\n", GetLastError());
        ok( hModule == hModule1, "Loaded wrong module\n");
        FreeLibrary(hModule1);
    }
    FreeLibrary(hModule);
}

static void testLoadLibraryA_Wrong(void)
{
    HMODULE hModule;

    /* Try to load a nonexistent dll */
    SetLastError(0xdeadbeef);
    hModule = LoadLibraryA("non_ex_pv.dll");
    ok( !hModule, "non_ex_pv.dll should be not loadable\n");
    ok( GetLastError() == ERROR_MOD_NOT_FOUND || GetLastError() == ERROR_DLL_NOT_FOUND,
        "Expected ERROR_MOD_NOT_FOUND or ERROR_DLL_NOT_FOUND (win9x), got %08lx\n", GetLastError());

    /* Just in case */
    FreeLibrary(hModule);
}

static void testGetProcAddress_Wrong(void)
{
    FARPROC fp;

    SetLastError(0xdeadbeef);
    fp = GetProcAddress(NULL, "non_ex_call");
    ok( !fp, "non_ex_call should not be found\n");
    ok( GetLastError() == ERROR_PROC_NOT_FOUND || GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_PROC_NOT_FOUND or ERROR_INVALID_HANDLE(win9x), got %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    fp = GetProcAddress((HMODULE)0xdeadbeef, "non_ex_call");
    ok( !fp, "non_ex_call should not be found\n");
    ok( GetLastError() == ERROR_MOD_NOT_FOUND || GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_MOD_NOT_FOUND or ERROR_INVALID_HANDLE(win9x), got %08lx\n", GetLastError());
}

START_TEST(module)
{
    WCHAR filenameW[MAX_PATH];

    /* Test if we can use GetModuleFileNameW */

    SetLastError(0xdeadbeef);
    GetModuleFileNameW(NULL, filenameW, MAX_PATH);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        trace("GetModuleFileNameW not existing on this platform, skipping W-calls\n");
        is_unicode_enabled = FALSE;
    }

    testGetModuleFileName(NULL);
    testGetModuleFileName("kernel32.dll");
    testGetModuleFileName_Wrong();

    testLoadLibraryA();
    testLoadLibraryA_Wrong();
    testGetProcAddress_Wrong();
}
