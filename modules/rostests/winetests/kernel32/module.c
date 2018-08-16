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
#include <stdio.h>
#include <psapi.h>

static DWORD (WINAPI *pGetDllDirectoryA)(DWORD,LPSTR);
static DWORD (WINAPI *pGetDllDirectoryW)(DWORD,LPWSTR);
static BOOL (WINAPI *pSetDllDirectoryA)(LPCSTR);
static DLL_DIRECTORY_COOKIE (WINAPI *pAddDllDirectory)(const WCHAR*);
static BOOL (WINAPI *pRemoveDllDirectory)(DLL_DIRECTORY_COOKIE);
static BOOL (WINAPI *pSetDefaultDllDirectories)(DWORD);
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

static const struct
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_SECTION_HEADER section;
} dll_image =
{
    { IMAGE_DOS_SIGNATURE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, 0, 0, { 0 },
      sizeof(IMAGE_DOS_HEADER) },
    {
        IMAGE_NT_SIGNATURE, /* Signature */
        {
#if defined __i386__
            IMAGE_FILE_MACHINE_I386, /* Machine */
#elif defined __x86_64__
            IMAGE_FILE_MACHINE_AMD64, /* Machine */
#elif defined __powerpc__
            IMAGE_FILE_MACHINE_POWERPC, /* Machine */
#elif defined __arm__
            IMAGE_FILE_MACHINE_ARMNT, /* Machine */
#elif defined __aarch64__
            IMAGE_FILE_MACHINE_ARM64, /* Machine */
#else
# error You must specify the machine type
#endif
            1, /* NumberOfSections */
            0, /* TimeDateStamp */
            0, /* PointerToSymbolTable */
            0, /* NumberOfSymbols */
            sizeof(IMAGE_OPTIONAL_HEADER), /* SizeOfOptionalHeader */
            IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL /* Characteristics */
        },
        { IMAGE_NT_OPTIONAL_HDR_MAGIC, /* Magic */
          1, /* MajorLinkerVersion */
          0, /* MinorLinkerVersion */
          0, /* SizeOfCode */
          0, /* SizeOfInitializedData */
          0, /* SizeOfUninitializedData */
          0, /* AddressOfEntryPoint */
          0x1000, /* BaseOfCode */
#ifndef _WIN64
          0, /* BaseOfData */
#endif
          0x10000000, /* ImageBase */
          0x1000, /* SectionAlignment */
          0x1000, /* FileAlignment */
          4, /* MajorOperatingSystemVersion */
          0, /* MinorOperatingSystemVersion */
          1, /* MajorImageVersion */
          0, /* MinorImageVersion */
          4, /* MajorSubsystemVersion */
          0, /* MinorSubsystemVersion */
          0, /* Win32VersionValue */
          0x2000, /* SizeOfImage */
          sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS), /* SizeOfHeaders */
          0, /* CheckSum */
          IMAGE_SUBSYSTEM_WINDOWS_CUI, /* Subsystem */
          0, /* DllCharacteristics */
          0, /* SizeOfStackReserve */
          0, /* SizeOfStackCommit */
          0, /* SizeOfHeapReserve */
          0, /* SizeOfHeapCommit */
          0, /* LoaderFlags */
          0, /* NumberOfRvaAndSizes */
          { { 0 } } /* DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] */
        }
    },
    { ".rodata", { 0 }, 0x1000, 0x1000, 0, 0, 0, 0, 0,
      IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ }
};

static void create_test_dll( const char *name )
{
    DWORD dummy;
    HANDLE handle = CreateFileA( name, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0 );

    ok( handle != INVALID_HANDLE_VALUE, "failed to create file err %u\n", GetLastError() );
    WriteFile( handle, &dll_image, sizeof(dll_image), &dummy, NULL );
    SetFilePointer( handle, dll_image.nt.OptionalHeader.SizeOfImage, NULL, FILE_BEGIN );
    SetEndOfFile( handle );
    CloseHandle( handle );
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

    ok(len1A / 2 == len2A,
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
    ok(bufA[0] == '*', "When failing, buffer shouldn't be written to\n");
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
    ok( hModule1 != NULL, "\"kernel32   \" should be loadable\n" );
    ok( GetLastError() == 0xdeadbeef, "GetLastError should be 0xdeadbeef but is %d\n", GetLastError() );
    ok( hModule == hModule1, "Loaded wrong module\n" );
    FreeLibrary(hModule1);
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
        /* We must be on Windows, so we cannot test */
        return;
    }

    GetWindowsDirectoryA(path2, sizeof(path2));
    strcat(path2, "\\system32\\");
    strcat(path2, dllname);
    hModule2 = LoadLibraryA(path2);
    ok(hModule2 != NULL, "LoadLibrary(%s) failed\n", path2);

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
    ok( GetLastError() == ERROR_MOD_NOT_FOUND, "Expected ERROR_MOD_NOT_FOUND, got %d\n", GetLastError() );

    /* Just in case */
    FreeLibrary(hModule);
}

static void testGetProcAddress_Wrong(void)
{
    FARPROC fp;

    SetLastError(0xdeadbeef);
    fp = GetProcAddress(NULL, "non_ex_call");
    ok( !fp, "non_ex_call should not be found\n");
    ok( GetLastError() == ERROR_PROC_NOT_FOUND, "Expected ERROR_PROC_NOT_FOUND, got %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    fp = GetProcAddress((HMODULE)0xdeadbeef, "non_ex_call");
    ok( !fp, "non_ex_call should not be found\n");
    ok( GetLastError() == ERROR_MOD_NOT_FOUND, "Expected ERROR_MOD_NOT_FOUND, got %d\n", GetLastError() );
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
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA(NULL, NULL, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    ok(GetLastError() == ERROR_MOD_NOT_FOUND ||
       GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_MOD_NOT_FOUND or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* empty lpFileName */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("", NULL, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    ok(GetLastError() == ERROR_MOD_NOT_FOUND ||
       GetLastError() == ERROR_INVALID_PARAMETER /* win8 */,
       "Expected ERROR_MOD_NOT_FOUND or ERROR_DLL_NOT_FOUND, got %d\n", GetLastError());

    /* hFile is non-NULL */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("testfile.dll", hfile, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    todo_wine
    {
        ok(GetLastError() == ERROR_SHARING_VIOLATION ||
           GetLastError() == ERROR_INVALID_PARAMETER, /* win2k3 */
           "Unexpected last error, got %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("testfile.dll", (HANDLE)0xdeadbeef, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    todo_wine
    {
        ok(GetLastError() == ERROR_SHARING_VIOLATION ||
           GetLastError() == ERROR_INVALID_PARAMETER, /* win2k3 */
           "Unexpected last error, got %d\n", GetLastError());
    }

    /* try to open a file that is locked */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("testfile.dll", NULL, 0);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    todo_wine
    {
        ok(GetLastError() == ERROR_SHARING_VIOLATION,
           "Expected ERROR_SHARING_VIOLATION, got %d\n", GetLastError());
    }

    /* lpFileName does not matter */
    if (is_unicode_enabled)
    {
        SetLastError(0xdeadbeef);
        hmodule = LoadLibraryExA(NULL, hfile, 0);
        ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
        ok(GetLastError() == ERROR_MOD_NOT_FOUND ||
           GetLastError() == ERROR_INVALID_PARAMETER, /* win2k3 */
           "Expected ERROR_MOD_NOT_FOUND or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    CloseHandle(hfile);

    /* load empty file */
    SetLastError(0xdeadbeef);
    hmodule = LoadLibraryExA("testfile.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    ok(hmodule == 0, "Expected 0, got %p\n", hmodule);
    todo_wine
    {
        ok(GetLastError() == ERROR_FILE_INVALID,
           "Expected ERROR_FILE_INVALID, got %d\n", GetLastError());
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
       GetLastError() == ERROR_SUCCESS,
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
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

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
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
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

static void test_LoadLibraryEx_search_flags(void)
{
    static const struct
    {
        int add_dirs[4];
        int dll_dir;
        int expect;
    } tests[] =
    {
        { { 1, 2, 3 }, 4, 3 }, /* 0 */
        { { 1, 3, 2 }, 4, 2 },
        { { 3, 1 },    4, 1 },
        { { 5, 6 },    4, 4 },
        { { 5, 2 },    4, 2 },
        { { 0 },       4, 4 }, /* 5 */
        { { 0 },       0, 0 },
        { { 6, 5 },    5, 0 },
        { { 1, 1, 2 }, 0, 2 },
    };
    char *p, path[MAX_PATH], buf[MAX_PATH];
    WCHAR bufW[MAX_PATH];
    DLL_DIRECTORY_COOKIE cookies[4];
    unsigned int i, j, k;
    BOOL ret;
    HMODULE mod;

    if (!pAddDllDirectory || !pSetDllDirectoryA) return;

    GetTempPathA( sizeof(path), path );
    GetTempFileNameA( path, "tmp", 0, buf );
    DeleteFileA( buf );
    ret = CreateDirectoryA( buf, NULL );
    ok( ret, "CreateDirectory failed err %u\n", GetLastError() );
    p = buf + strlen( buf );
    for (i = 1; i <= 6; i++)
    {
        sprintf( p, "\\%u", i );
        ret = CreateDirectoryA( buf, NULL );
        ok( ret, "CreateDirectory failed err %u\n", GetLastError() );
        if (i >= 5) continue;  /* dirs 5 and 6 are left empty */
        sprintf( p, "\\%u\\winetestdll.dll", i );
        create_test_dll( buf );
    }
    SetLastError( 0xdeadbeef );
    mod = LoadLibraryExA( "winetestdll.dll", 0, LOAD_LIBRARY_SEARCH_APPLICATION_DIR );
    ok( !mod, "LoadLibrary succeeded\n" );
    ok( GetLastError() == ERROR_MOD_NOT_FOUND, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    mod = LoadLibraryExA( "winetestdll.dll", 0, LOAD_LIBRARY_SEARCH_USER_DIRS );
    ok( !mod, "LoadLibrary succeeded\n" );
    ok( GetLastError() == ERROR_MOD_NOT_FOUND || broken(GetLastError() == ERROR_NOT_ENOUGH_MEMORY),
        "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    mod = LoadLibraryExA( "winetestdll.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32 );
    ok( !mod, "LoadLibrary succeeded\n" );
    ok( GetLastError() == ERROR_MOD_NOT_FOUND, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    mod = LoadLibraryExA( "winetestdll.dll", 0, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR );
    ok( !mod, "LoadLibrary succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    mod = LoadLibraryExA( "winetestdll.dll", 0, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32 );
    ok( !mod, "LoadLibrary succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    mod = LoadLibraryExA( "foo\\winetestdll.dll", 0, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR );
    ok( !mod, "LoadLibrary succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    mod = LoadLibraryExA( "\\windows\\winetestdll.dll", 0, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR );
    ok( !mod, "LoadLibrary succeeded\n" );
    ok( GetLastError() == ERROR_MOD_NOT_FOUND, "wrong error %u\n", GetLastError() );

    for (j = 0; j < sizeof(tests) / sizeof(tests[0]); j++)
    {
        for (k = 0; tests[j].add_dirs[k]; k++)
        {
            sprintf( p, "\\%u", tests[j].add_dirs[k] );
            MultiByteToWideChar( CP_ACP, 0, buf, -1, bufW, MAX_PATH );
            cookies[k] = pAddDllDirectory( bufW );
            ok( cookies[k] != NULL, "failed to add %s\n", buf );
        }
        if (tests[j].dll_dir)
        {
            sprintf( p, "\\%u", tests[j].dll_dir );
            pSetDllDirectoryA( buf );
        }
        else pSetDllDirectoryA( NULL );

        SetLastError( 0xdeadbeef );
        mod = LoadLibraryExA( "winetestdll.dll", 0, LOAD_LIBRARY_SEARCH_USER_DIRS );
        if (tests[j].expect)
        {
            ok( mod != NULL, "%u: LoadLibrary failed err %u\n", j, GetLastError() );
            GetModuleFileNameA( mod, path, MAX_PATH );
            sprintf( p, "\\%u\\winetestdll.dll", tests[j].expect );
            ok( !lstrcmpiA( path, buf ), "%u: wrong module %s expected %s\n", j, path, buf );
        }
        else
        {
            ok( !mod, "%u: LoadLibrary succeeded\n", j );
            ok( GetLastError() == ERROR_MOD_NOT_FOUND || broken(GetLastError() == ERROR_NOT_ENOUGH_MEMORY),
                "%u: wrong error %u\n", j, GetLastError() );
        }
        FreeLibrary( mod );

        for (k = 0; tests[j].add_dirs[k]; k++) pRemoveDllDirectory( cookies[k] );
    }

    for (i = 1; i <= 6; i++)
    {
        sprintf( p, "\\%u\\winetestdll.dll", i );
        DeleteFileA( buf );
        sprintf( p, "\\%u", i );
        RemoveDirectoryA( buf );
    }
    *p = 0;
    RemoveDirectoryA( buf );
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
    MAKEFUNC(AddDllDirectory);
    MAKEFUNC(RemoveDllDirectory);
    MAKEFUNC(SetDefaultDllDirectories);
    MAKEFUNC(K32GetModuleInformation);
#undef MAKEFUNC

    /* before Windows 7 this was not exported in kernel32 */
    if (!pK32GetModuleInformation)
    {
        HMODULE hPsapi = LoadLibraryA("psapi.dll");
        pK32GetModuleInformation = (void *)GetProcAddress(hPsapi, "GetModuleInformation");
    }
}

static void testGetModuleHandleEx(void)
{
    static const WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2',0};
    static const WCHAR nosuchmodW[] = {'n','o','s','u','c','h','m','o','d',0};
    BOOL ret;
    DWORD error;
    HMODULE mod, mod_kernel32;

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExA( 0, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExA( 0, "kernel32", NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExA( 0, "kernel32", &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod != (HMODULE)0xdeadbeef, "got %p\n", mod );
    FreeLibrary( mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExA( 0, "nosuchmod", &mod );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_MOD_NOT_FOUND, "got %u\n", error );
    ok( mod == NULL, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExW( 0, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExW( 0, kernel32W, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExW( 0, kernel32W, &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod != (HMODULE)0xdeadbeef, "got %p\n", mod );
    FreeLibrary( mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExW( 0, nosuchmodW, &mod );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_MOD_NOT_FOUND, "got %u\n", error );
    ok( mod == NULL, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, "kernel32", NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, "kernel32", &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod != (HMODULE)0xdeadbeef, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, "nosuchmod", &mod );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_MOD_NOT_FOUND, "got %u\n", error );
    ok( mod == NULL, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, kernel32W, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, kernel32W, &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod != (HMODULE)0xdeadbeef, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, nosuchmodW, &mod );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_MOD_NOT_FOUND, "got %u\n", error );
    ok( mod == NULL, "got %p\n", mod );

    mod_kernel32 = LoadLibraryA( "kernel32" );

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)mod_kernel32, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)mod_kernel32, &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod == mod_kernel32, "got %p\n", mod );
    FreeLibrary( mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)0xbeefdead, &mod );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_MOD_NOT_FOUND, "got %u\n", error );
    ok( mod == NULL, "got %p\n", mod );

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, NULL, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    ret = GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)mod_kernel32, NULL );
    error = GetLastError();
    ok( !ret, "unexpected success\n" );
    ok( error == ERROR_INVALID_PARAMETER, "got %u\n", error );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)mod_kernel32, &mod );
    ok( ret, "unexpected failure %u\n", GetLastError() );
    ok( mod == mod_kernel32, "got %p\n", mod );
    FreeLibrary( mod );

    SetLastError( 0xdeadbeef );
    mod = (HMODULE)0xdeadbeef;
    ret = GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)0xbeefdead, &mod );
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

static void test_AddDllDirectory(void)
{
    static const WCHAR tmpW[] = {'t','m','p',0};
    static const WCHAR dotW[] = {'.','\\','.',0};
    static const WCHAR rootW[] = {'\\',0};
    WCHAR path[MAX_PATH], buf[MAX_PATH];
    DLL_DIRECTORY_COOKIE cookie;
    BOOL ret;

    if (!pAddDllDirectory || !pRemoveDllDirectory)
    {
        win_skip( "AddDllDirectory not available\n" );
        return;
    }

    buf[0] = '\0';
    GetTempPathW( sizeof(path)/sizeof(path[0]), path );
    ret = GetTempFileNameW( path, tmpW, 0, buf );
    ok( ret, "GetTempFileName failed err %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    cookie = pAddDllDirectory( buf );
    ok( cookie != NULL, "AddDllDirectory failed err %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pRemoveDllDirectory( cookie );
    ok( ret, "RemoveDllDirectory failed err %u\n", GetLastError() );

    DeleteFileW( buf );
    SetLastError( 0xdeadbeef );
    cookie = pAddDllDirectory( buf );
    ok( !cookie, "AddDllDirectory succeeded\n" );
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %u\n", GetLastError() );
    cookie = pAddDllDirectory( dotW );
    ok( !cookie, "AddDllDirectory succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    cookie = pAddDllDirectory( rootW );
    ok( cookie != NULL, "AddDllDirectory failed err %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pRemoveDllDirectory( cookie );
    ok( ret, "RemoveDllDirectory failed err %u\n", GetLastError() );
    GetWindowsDirectoryW( buf, MAX_PATH );
    lstrcpyW( buf + 2, tmpW );
    cookie = pAddDllDirectory( buf );
    ok( !cookie, "AddDllDirectory succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
}

static void test_SetDefaultDllDirectories(void)
{
    HMODULE mod;
    BOOL ret;

    if (!pSetDefaultDllDirectories)
    {
        win_skip( "SetDefaultDllDirectories not available\n" );
        return;
    }

    mod = LoadLibraryA( "authz.dll" );
    ok( mod != NULL, "loading authz failed\n" );
    FreeLibrary( mod );
    ret = pSetDefaultDllDirectories( LOAD_LIBRARY_SEARCH_USER_DIRS );
    ok( ret, "SetDefaultDllDirectories failed err %u\n", GetLastError() );
    mod = LoadLibraryA( "authz.dll" );
    todo_wine ok( !mod, "loading authz succeeded\n" );
    FreeLibrary( mod );
    ret = pSetDefaultDllDirectories( LOAD_LIBRARY_SEARCH_SYSTEM32 );
    ok( ret, "SetDefaultDllDirectories failed err %u\n", GetLastError() );
    mod = LoadLibraryA( "authz.dll" );
    ok( mod != NULL, "loading authz failed\n" );
    FreeLibrary( mod );
    mod = LoadLibraryExA( "authz.dll", 0, LOAD_LIBRARY_SEARCH_APPLICATION_DIR );
    todo_wine ok( !mod, "loading authz succeeded\n" );
    FreeLibrary( mod );
    ret = pSetDefaultDllDirectories( LOAD_LIBRARY_SEARCH_APPLICATION_DIR );
    ok( ret, "SetDefaultDllDirectories failed err %u\n", GetLastError() );
    mod = LoadLibraryA( "authz.dll" );
    todo_wine ok( !mod, "loading authz succeeded\n" );
    FreeLibrary( mod );
    ret = pSetDefaultDllDirectories( LOAD_LIBRARY_SEARCH_DEFAULT_DIRS );
    ok( ret, "SetDefaultDllDirectories failed err %u\n", GetLastError() );
    mod = LoadLibraryA( "authz.dll" );
    ok( mod != NULL, "loading authz failed\n" );
    FreeLibrary( mod );

    SetLastError( 0xdeadbeef );
    ret = pSetDefaultDllDirectories( 0 );
    ok( !ret, "SetDefaultDllDirectories succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pSetDefaultDllDirectories( 3 );
    ok( !ret, "SetDefaultDllDirectories succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pSetDefaultDllDirectories( LOAD_LIBRARY_SEARCH_APPLICATION_DIR | 0x8000 );
    ok( !ret, "SetDefaultDllDirectories succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pSetDefaultDllDirectories( LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR );
    ok( !ret || broken(ret) /* win7 */, "SetDefaultDllDirectories succeeded\n" );
    if (!ret) ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pSetDefaultDllDirectories( LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_USER_DIRS );
    ok( !ret || broken(ret) /* win7 */, "SetDefaultDllDirectories succeeded\n" );
    if (!ret) ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    /* restore some sane defaults */
    pSetDefaultDllDirectories( LOAD_LIBRARY_SEARCH_DEFAULT_DIRS );
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
    test_LoadLibraryEx_search_flags();
    testGetModuleHandleEx();
    testK32GetModuleInformation();
    test_AddDllDirectory();
    test_SetDefaultDllDirectories();
}
