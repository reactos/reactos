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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

    ok(len1A == strlen(bufA), "Unexpected length of GetModuleFilenameA (%d/%d)\n", len1A, lstrlenA(bufA));

    if (is_unicode_enabled)
    {
        ok(len1W == lstrlenW(bufW), "Unexpected length of GetModuleFilenameW (%d/%d)\n", len1W, lstrlenW(bufW));
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
        ok(len1W / 2 == len2W, "Correct length in GetModuleFilenameW with buffer too small (%d/%d)\n", len1W / 2, len2W);
    }

    ok(len1A / 2 == len2A || 
       len1A / 2 == len2A + 1, /* Win9x */
       "Correct length in GetModuleFilenameA with buffer too small (%d/%d)\n", len1A / 2, len2A);
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
    ok( GetLastError() == 0xdeadbeef, "GetLastError should be 0xdeadbeef but is %d\n", GetLastError());

    fp = GetProcAddress(hModule, "CreateFileA");
    ok( fp != NULL, "CreateFileA should be there\n");
    ok( GetLastError() == 0xdeadbeef, "GetLastError should be 0xdeadbeef but is %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hModule1 = LoadLibraryA("kernel32   ");
    /* Only winNT does this */
    if (GetLastError() != ERROR_DLL_NOT_FOUND)
    {
        ok( hModule1 != NULL, "\"kernel32   \" should be loadable\n");
        ok( GetLastError() == 0xdeadbeef, "GetLastError should be 0xdeadbeef but is %d\n", GetLastError());
        ok( hModule == hModule1, "Loaded wrong module\n");
        FreeLibrary(hModule1);
    }
    FreeLibrary(hModule);
}

static void testNestedLoadLibraryA(void)
{
    static const char dllname[] = "shell32.dll";
    char path1[MAX_PATH], path2[MAX_PATH];
    HMODULE hModule1, hModule2, hModule3;

    /* This is not really a Windows conformance test, but more a Wine
     * regression test. Wine's builtin dlls can be loaded from multiple paths,
     * and this test tries to make sure that Wine does not get confused and
     * really unloads the Unix .so file at the right time. Failure to do so
     * will result in the dll being unloadable.
     * This test must be done with a dll that can be unloaded, which means:
     * - it must not already be loaded
     * - it must not have a 16-bit counterpart
     */
    GetWindowsDirectory(path1, sizeof(path1));
    strcat(path1, "\\system\\");
    strcat(path1, dllname);
    hModule1 = LoadLibraryA(path1);
    if (!hModule1)
    {
        /* We must be on Windows NT, so we cannot test */
        return;
    }

    GetWindowsDirectory(path2, sizeof(path2));
    strcat(path2, "\\system32\\");
    strcat(path2, dllname);
    hModule2 = LoadLibraryA(path2);
    if (!hModule2)
    {
        /* We must be on Windows 9x, so we cannot test */
        ok(FreeLibrary(hModule1), "FreeLibrary() failed\n");
        return;
    }

    /* The first LoadLibrary() call may have registered the dll under the
     * system32 path. So load it, again, under the '...\system\...' path so
     * Wine does not immediately notice that it is already loaded.
     */
    hModule3 = LoadLibraryA(path1);
    ok(hModule3 != NULL, "LoadLibrary(%s) failed\n", path1);

    /* Now fully unload the dll */
    ok(FreeLibrary(hModule3), "FreeLibrary() failed\n");
    ok(FreeLibrary(hModule2), "FreeLibrary() failed\n");
    ok(FreeLibrary(hModule1), "FreeLibrary() failed\n");
    ok(GetModuleHandle(dllname) == NULL, "%s was not fully unloaded\n", dllname);

    /* Try to load the dll again, if refcounting is ok, this should work */
    hModule1 = LoadLibraryA(path1);
    ok(hModule1 != NULL, "LoadLibrary(%s) failed\n", path1);
    if (hModule1 != NULL)
        ok(FreeLibrary(hModule1), "FreeLibrary() failed\n");
}

static void testLoadLibraryA_Wrong(void)
{
    HMODULE hModule;

    /* Try to load a nonexistent dll */
    SetLastError(0xdeadbeef);
    hModule = LoadLibraryA("non_ex_pv.dll");
    ok( !hModule, "non_ex_pv.dll should be not loadable\n");
    ok( GetLastError() == ERROR_MOD_NOT_FOUND || GetLastError() == ERROR_DLL_NOT_FOUND, 
        "Expected ERROR_MOD_NOT_FOUND or ERROR_DLL_NOT_FOUND (win9x), got %d\n", GetLastError());

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
        "Expected ERROR_PROC_NOT_FOUND or ERROR_INVALID_HANDLE(win9x), got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    fp = GetProcAddress((HMODULE)0xdeadbeef, "non_ex_call");
    ok( !fp, "non_ex_call should not be found\n");
    ok( GetLastError() == ERROR_MOD_NOT_FOUND || GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_MOD_NOT_FOUND or ERROR_INVALID_HANDLE(win9x), got %d\n", GetLastError());
}

static void testLoadLibraryEx(void)
{
    CHAR path[MAX_PATH];
    HMODULE hmodule;
    HANDLE hfile;

    hfile = CreateFileA("testfile.dll", GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "Expected a valid file handle\n");

    /* NULL lpFileName */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA(NULL, NULL, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    ok(GetLastError() == ERROR_MOD_NOT_FOUND ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* win9x */
       "Expected ERROR_MOD_NOT_FOUND or ERROR_INVALID_PARAMETER, got %d\n",
       GetLastError());

    /* empty lpFileName */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("", NULL, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    ok(GetLastError() == ERROR_MOD_NOT_FOUND ||
       GetLastError() == ERROR_DLL_NOT_FOUND, /* win9x */
       "Expected ERROR_MOD_NOT_FOUND or ERROR_DLL_NOT_FOUND, got %d\n",
       GetLastError());

    /* hFile is non-NULL */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("testfile.dll", hfile, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    ok(GetLastError() == ERROR_SHARING_VIOLATION ||
       GetLastError() == ERROR_INVALID_PARAMETER || /* win2k3 */
       GetLastError() == ERROR_FILE_NOT_FOUND, /* win9x */
       "Unexpected last error, got %d\n", GetLastError());

    /* try to open a file that is locked */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("testfile.dll", NULL, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    todo_wine
    {
        ok(GetLastError() == ERROR_SHARING_VIOLATION ||
           GetLastError() == ERROR_FILE_NOT_FOUND, /* win9x */
           "Expected ERROR_SHARING_VIOLATION or ERROR_FILE_NOT_FOUND, got %d\n",
           GetLastError());
    }

    /* lpFileName does not matter */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA(NULL, hfile, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    ok(GetLastError() == ERROR_MOD_NOT_FOUND ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* win2k3 */
       "Expected ERROR_MOD_NOT_FOUND or ERROR_INVALID_PARAMETER, got %d\n",
       GetLastError());

    CloseHandle(hfile);

    /* load empty file */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("testfile.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    todo_wine
    {
        ok(GetLastError() == ERROR_FILE_INVALID ||
           GetLastError() == ERROR_BAD_FORMAT, /* win9x */
           "Expected ERROR_FILE_INVALID or ERROR_BAD_FORMAT, got %d\n",
           GetLastError());
    }

    DeleteFileA("testfile.dll");

    GetSystemDirectoryA(path, MAX_PATH);
    if (path[lstrlenA(path) - 1] != '\\')
        lstrcatA(path, "\\");
    lstrcatA(path, "kernel32.dll");

    /* load kernel32.dll with an absolute path */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA(path, NULL, LOAD_LIBRARY_AS_DATAFILE);
    ok(hmodule != 0, "Expected valid module handle\n");
    ok(GetLastError() == 0xdeadbeef ||
       GetLastError() == ERROR_SUCCESS, /* win9x */
       "Expected 0xdeadbeef or ERROR_SUCCESS, got %d\n", GetLastError());

    CloseHandle(hmodule);

    /* load kernel32.dll with no path */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("kernel32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    ok(hmodule != 0, "Expected valid module handle\n");
    ok(GetLastError() == 0xdeadbeef ||
       GetLastError() == ERROR_SUCCESS, /* win9x */
       "Expected 0xdeadbeef or ERROR_SUCCESS, got %d\n", GetLastError());

    CloseHandle(hmodule);

    GetCurrentDirectoryA(MAX_PATH, path);
    if (path[lstrlenA(path) - 1] != '\\')
        lstrcatA(path, "\\");
    lstrcatA(path, "kernel32.dll");

    /* load kernel32.dll with an absolute path that does not exist */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA(path, NULL, LOAD_LIBRARY_AS_DATAFILE);
    todo_wine
    {
        ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    }
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       broken(GetLastError() == ERROR_INVALID_HANDLE),  /* nt4 */
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
}

START_TEST(module)
{
    WCHAR filenameW[MAX_PATH];

    /* Test if we can use GetModuleFileNameW */

    SetLastError(0xdeadbeef);
    GetModuleFileNameW(NULL, filenameW, MAX_PATH);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetModuleFileNameW not existing on this platform, skipping W-calls\n");
        is_unicode_enabled = FALSE;
    }

    testGetModuleFileName(NULL);
    testGetModuleFileName("kernel32.dll");
    testGetModuleFileName_Wrong();

    testLoadLibraryA();
    testNestedLoadLibraryA();
    testLoadLibraryA_Wrong();
    testGetProcAddress_Wrong();
    testLoadLibraryEx();
}
