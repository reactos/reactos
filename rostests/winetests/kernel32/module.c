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
#include <psapi.h>

static DWORD (WINAPI *pGetDllDirectoryA)(DWORD,LPSTR);
static DWORD (WINAPI *pGetDllDirectoryW)(DWORD,LPWSTR);
static BOOL (WINAPI *pSetDllDirectoryA)(LPCSTR);
static BOOL (WINAPI *pGetModuleHandleExA)(DWORD,LPCSTR,HMODULE*);
static BOOL (WINAPI *pGetModuleHandleExW)(DWORD,LPCWSTR,HMODULE*);
static BOOL (WINAPI *pK32GetModuleInformation)(HANDLE process, HMODULE module,
                                               MODULEINFO *modinfo, DWORD cb);

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

    hMod = (name) ? GetModuleHandleA(name) : NULL;

    /* first test, with enough space in buffer */
    memset(bufA, '-', sizeof(bufA));
    SetLastError(0xdeadbeef);
    len1A = GetModuleFileNameA(hMod, bufA, sizeof(bufA));
    ok(GetLastError() == ERROR_SUCCESS ||
       broken(GetLastError() == 0xdeadbeef), /* <= XP SP3 */
       "LastError was not reset: %u\n", GetLastError());
    ok(len1A > 0, "Getting module filename for handle %p\n", hMod);

    if (is_unicode_enabled)
    {
        memset(bufW, '-', sizeof(bufW));
        SetLastError(0xdeadbeef);
        len1W = GetModuleFileNameW(hMod, bufW, sizeof(bufW) / sizeof(WCHAR));
        ok(GetLastError() == ERROR_SUCCESS ||
           broken(GetLastError() == 0xdeadbeef), /* <= XP SP3 */
           "LastError was not reset: %u\n", GetLastError());
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
    GetWindowsDirectoryA(path1, sizeof(path1));
    strcat(path1, "\\system\\");
    strcat(path1, dllname);
    hModule1 = LoadLibraryA(path1);
    if (!hModule1)
    {
        /* We must be on Windows NT, so we cannot test */
        return;
    }

    GetWindowsDirectoryA(path2, sizeof(path2));
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
    ok(GetModuleHandleA(dllname) == NULL, "%s was not fully unloaded\n", dllname);

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
    BOOL ret;

    hfile = CreateFileA("testfile.dll", GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "Expected a valid file handle\n");

    /* NULL lpFileName */
    if (is_unicode_enabled)
    {
        SetLastError(0xdeadbeef);
        hmodule = LoadLibraryExA(NULL, NULL, 0);
        ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
        ok(GetLastError() == ERROR_MOD_NOT_FOUND ||
           GetLastError() == ERROR_INVALID_PARAMETER, /* win9x */
           "Expected ERROR_MOD_NOT_FOUND or ERROR_INVALID_PARAMETER, got %d\n",
           GetLastError());
    }
    else
        win_skip("NULL filename crashes on WinMe\n");

    /* empty lpFileName */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("", NULL, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    ok(GetLastError() == ERROR_MOD_NOT_FOUND ||
       GetLastError() == ERROR_DLL_NOT_FOUND /* win9x */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* win8 */,
       "Expected ERROR_MOD_NOT_FOUND or ERROR_DLL_NOT_FOUND, got %d\n",
       GetLastError());

    /* hFile is non-NULL */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("testfile.dll", hfile, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    todo_wine
    {
        ok(GetLastError() == ERROR_SHARING_VIOLATION ||
           GetLastError() == ERROR_INVALID_PARAMETER || /* win2k3 */
           GetLastError() == ERROR_FILE_NOT_FOUND, /* win9x */
           "Unexpected last error, got %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("testfile.dll", (HANDLE)0xdeadbeef, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    todo_wine
    {
        ok(GetLastError() == ERROR_SHARING_VIOLATION ||
           GetLastError() == ERROR_INVALID_PARAMETER || /* win2k3 */
           GetLastError() == ERROR_FILE_NOT_FOUND, /* win9x */
           "Unexpected last error, got %d\n", GetLastError());
    }

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
    if (is_unicode_enabled)
    {
        SetLastError(0xdeadbeef);
        hmodule = LoadLibraryExA(NULL, hfile, 0);
        ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
        ok(GetLastError() == ERROR_MOD_NOT_FOUND ||
           GetLastError() == ERROR_INVALID_PARAMETER, /* win2k3 */
           "Expected ERROR_MOD_NOT_FOUND or ERROR_INVALID_PARAMETER, got %d\n",
           GetLastError());
    }

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

    /* try invalid file handle */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA(path, (HANDLE)0xdeadbeef, 0);
    if (!hmodule)  /* succeeds on xp and older */
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError());

    FreeLibrary(hmodule);

    /* load kernel32.dll with no path */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("kernel32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    ok(hmodule != 0, "Expected valid module handle\n");
    ok(GetLastError() == 0xdeadbeef ||
       GetLastError() == ERROR_SUCCESS, /* win9x */
       "Expected 0xdeadbeef or ERROR_SUCCESS, got %d\n", GetLastError());

    FreeLibrary(hmodule);

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

    /* Free the loaded dll when it's the first time this dll is loaded
       in process - First time should pass, second fail */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("comctl32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    ok(hmodule != 0, "Expected valid module handle\n");

    SetLastError(0xdeadbeef);
    ret = FreeLibrary(hmodule);
    ok(ret, "Expected to be able to free the module, failed with %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = FreeLibrary(hmodule);
    ok(!ret, "Unexpected ability to free the module, failed with %d\n", GetLastError());

    /* load with full path, name without extension */
    GetSystemDirectoryA(path, MAX_PATH);
    if (path[lstrlenA(path) - 1] != '\\')
        lstrcatA(path, "\\");
    lstrcatA(path, "kernel32");
    hmodule = LoadLibraryExA(path, NULL, 0);
    ok(hmodule != NULL, "got %p\n", hmodule);
    FreeLibrary(hmodule);

    /* same with alterate search path */
    hmodule = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    ok(hmodule != NULL, "got %p\n", hmodule);
    FreeLibrary(hmodule);
}

static void testGetDllDirectory(void)
{
    CHAR bufferA[MAX_PATH];
    WCHAR bufferW[MAX_PATH];
    DWORD length, ret;
    int i;
    static const char *dll_directories[] =
    {
        "",
        "C:\\Some\\Path",
        "C:\\Some\\Path\\",
        "Q:\\A\\Long\\Path with spaces that\\probably\\doesn't exist!",
    };
    const int test_count = sizeof(dll_directories) / sizeof(dll_directories[0]);

    if (!pGetDllDirectoryA || !pGetDllDirectoryW)
    {
        win_skip("GetDllDirectory not available\n");
        return;
    }
    if (!pSetDllDirectoryA)
    {
        win_skip("SetDllDirectoryA not available\n");
        return;
    }

    for (i = 0; i < test_count; i++)
    {
        length = strlen(dll_directories[i]);
        if (!pSetDllDirectoryA(dll_directories[i]))
        {
            skip("i=%d, SetDllDirectoryA failed\n", i);
            continue;
        }

        /* no buffer, determine length */
        ret = pGetDllDirectoryA(0, NULL);
        ok(ret == length + 1, "Expected %u, got %u\n", length + 1, ret);

        ret = pGetDllDirectoryW(0, NULL);
        ok(ret == length + 1, "Expected %u, got %u\n", length + 1, ret);

        /* buffer of exactly the right size */
        bufferA[length] = 'A';
        bufferA[length + 1] = 'A';
        ret = pGetDllDirectoryA(length + 1, bufferA);
        ok(ret == length || broken(ret + 1 == length) /* win8 */,
           "i=%d, Expected %u(+1), got %u\n", i, length, ret);
        ok(bufferA[length + 1] == 'A', "i=%d, Buffer overflow\n", i);
        ok(strcmp(bufferA, dll_directories[i]) == 0, "i=%d, Wrong path returned: '%s'\n", i, bufferA);

        bufferW[length] = 'A';
        bufferW[length + 1] = 'A';
        ret = pGetDllDirectoryW(length + 1, bufferW);
        ok(ret == length, "i=%d, Expected %u, got %u\n", i, length, ret);
        ok(bufferW[length + 1] == 'A', "i=%d, Buffer overflow\n", i);
        ok(cmpStrAW(dll_directories[i], bufferW, length, length),
           "i=%d, Wrong path returned: %s\n", i, wine_dbgstr_w(bufferW));

        /* Zero size buffer. The buffer may or may not be terminated depending
         * on the Windows version and whether the A or W API is called. */
        bufferA[0] = 'A';
        ret = pGetDllDirectoryA(0, bufferA);
        ok(ret == length + 1, "i=%d, Expected %u, got %u\n", i, length + 1, ret);

        bufferW[0] = 'A';
        ret = pGetDllDirectoryW(0, bufferW);
        ok(ret == length + 1, "i=%d, Expected %u, got %u\n", i, length + 1, ret);
        ok(bufferW[0] == 0 || /* XP, 2003 */
           broken(bufferW[0] == 'A'), "i=%d, Buffer overflow\n", i);

        /* buffer just one too short */
        bufferA[0] = 'A';
        ret = pGetDllDirectoryA(length, bufferA);
        ok(ret == length + 1, "i=%d, Expected %u, got %u\n", i, length + 1, ret);
        if (length != 0)
            ok(bufferA[0] == 0, "i=%d, Buffer not null terminated\n", i);

        bufferW[0] = 'A';
        ret = pGetDllDirectoryW(length, bufferW);
        ok(ret == length + 1, "i=%d, Expected %u, got %u\n", i, length + 1, ret);
        ok(bufferW[0] == 0 || /* XP, 2003 */
           broken(bufferW[0] == 'A'), "i=%d, Buffer overflow\n", i);

        if (0)
        {
            /* crashes on win8 */
            /* no buffer, but too short length */
            ret = pGetDllDirectoryA(length, NULL);
            ok(ret == length + 1, "i=%d, Expected %u, got %u\n", i, length + 1, ret);

            ret = pGetDllDirectoryW(length, NULL);
            ok(ret == length + 1, "i=%d, Expected %u, got %u\n", i, length + 1, ret);
        }
    }

    /* unset whatever we did so following tests won't be affected */
    pSetDllDirectoryA(NULL);
}

static void init_pointers(void)
{
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");

#define MAKEFUNC(f) (p##f = (void*)GetProcAddress(hKernel32, #f))
    MAKEFUNC(GetDllDirectoryA);
    MAKEFUNC(GetDllDirectoryW);
    MAKEFUNC(SetDllDirectoryA);
    MAKEFUNC(GetModuleHandleExA);
    MAKEFUNC(GetModuleHandleExW);
    MAKEFUNC(K32GetModuleInformation);
#undef MAKEFUNC

    /* not all Windows versions export this in kernel32 */
    if (!pK32GetModuleInformation)
    {
        HMODULE hPsapi = LoadLibraryA("psapi.dll");
        if (hPsapi)
        {
            pK32GetModuleInformation = (void *)GetProcAddress(hPsapi, "GetModuleInformation");
            if (!pK32GetModuleInformation) FreeLibrary(hPsapi);
        }
    }

}

static void testGetModuleHandleEx(void)
{
    static const WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2',0};
    static const WCHAR nosuchmodW[] = {'n','o','s','u','c','h','m','o','d',0};
    BOOL ret;
    DWORD error;
    HMODULE mod, mod_kernel32;

    if (!pGetModuleHandleExA || !pGetModuleHandleExW)
    {
        win_skip( "GetModuleHandleEx not available\n" );
        return;
    }

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExA( 0, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExA( 0, "kernel32", NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExA( 0, "kernel32", &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod != (HMODULE)0xdeadbeef, "got %p\n", mod );
    FreeLibrary( mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExA( 0, "nosuchmod", &mod );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_MOD_NOT_FOUND, "got %u\n", error );
    ok( mod == NULL, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExW( 0, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExW( 0, kernel32W, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExW( 0, kernel32W, &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod != (HMODULE)0xdeadbeef, "got %p\n", mod );
    FreeLibrary( mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExW( 0, nosuchmodW, &mod );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_MOD_NOT_FOUND, "got %u\n", error );
    ok( mod == NULL, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, "kernel32", NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, "kernel32", &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod != (HMODULE)0xdeadbeef, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, "nosuchmod", &mod );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_MOD_NOT_FOUND, "got %u\n", error );
    ok( mod == NULL, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, kernel32W, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, kernel32W, &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod != (HMODULE)0xdeadbeef, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, nosuchmodW, &mod );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_MOD_NOT_FOUND, "got %u\n", error );
    ok( mod == NULL, "got %p\n", mod );

    mod_kernel32 = LoadLibraryA( "kernel32" );

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)mod_kernel32, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)mod_kernel32, &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod == mod_kernel32, "got %p\n", mod );
    FreeLibrary( mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)0xbeefdead, &mod );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_MOD_NOT_FOUND, "got %u\n", error );
    ok( mod == NULL, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = pGetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)mod_kernel32, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)mod_kernel32, &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod == mod_kernel32, "got %p\n", mod );
    FreeLibrary( mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = pGetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)0xbeefdead, &mod );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_MOD_NOT_FOUND, "got %u\n", error );
    ok( mod == NULL, "got %p\n", mod );

    FreeLibrary( mod_kernel32 );
}

static void testK32GetModuleInformation(void)
{
    MODULEINFO info;
    HMODULE mod;
    BOOL ret;

    if (!pK32GetModuleInformation)
    {
        win_skip("K32GetModuleInformation not available\n");
        return;
    }

    mod = GetModuleHandleA(NULL);
    memset(&info, 0xAA, sizeof(info));
    ret = pK32GetModuleInformation(GetCurrentProcess(), mod, &info, sizeof(info));
    ok(ret, "K32GetModuleInformation failed for main module\n");
    ok(info.lpBaseOfDll == mod, "Wrong info.lpBaseOfDll = %p, expected %p\n", info.lpBaseOfDll, mod);
    ok(info.EntryPoint != NULL, "Expected nonzero entrypoint\n");

    mod = GetModuleHandleA("kernel32.dll");
    memset(&info, 0xAA, sizeof(info));
    ret = pK32GetModuleInformation(GetCurrentProcess(), mod, &info, sizeof(info));
    ok(ret, "K32GetModuleInformation failed for kernel32 module\n");
    ok(info.lpBaseOfDll == mod, "Wrong info.lpBaseOfDll = %p, expected %p\n", info.lpBaseOfDll, mod);
    ok(info.EntryPoint != NULL, "Expected nonzero entrypoint\n");
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

    init_pointers();

    testGetModuleFileName(NULL);
    testGetModuleFileName("kernel32.dll");
    testGetModuleFileName_Wrong();

    testGetDllDirectory();

    testLoadLibraryA();
    testNestedLoadLibraryA();
    testLoadLibraryA_Wrong();
    testGetProcAddress_Wrong();
    testLoadLibraryEx();
    testGetModuleHandleEx();
    testK32GetModuleInformation();
}
