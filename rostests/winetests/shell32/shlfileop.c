/*
 * Unit test of the SHFileOperation function.
 *
 * Copyright 2002 Andriy Palamarchuk
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

#define WINE_NOWINSOCK
#include <windows.h>
#include "shellapi.h"
#include "shlobj.h"

#include "wine/test.h"

#ifndef FOF_NORECURSION
#define FOF_NORECURSION 0x1000
#endif

/* Error codes could be pre-Win32 */
#define DE_SAMEFILE      0x71
#define DE_MANYSRC1DEST  0x72
#define DE_DIFFDIR       0x73
#define DE_OPCANCELLED   0x75
#define DE_DESTSUBTREE   0x76
#define DE_INVALIDFILES  0x7C
#define DE_DESTSAMETREE  0x7D
#define DE_FLDDESTISFILE 0x7E
#define DE_FILEDESTISFLD 0x80
#define expect_retval(ret, ret_prewin32)\
    ok(retval == ret ||\
       broken(retval == ret_prewin32),\
       "Expected %d, got %d\n", ret, retval)

static BOOL old_shell32 = FALSE;

static CHAR CURR_DIR[MAX_PATH];
static const WCHAR UNICODE_PATH[] = {'c',':','\\',0x00ae,'\0','\0'};
    /* "c:\®" can be used in all codepages */
    /* Double-null termination needed for pFrom field of SHFILEOPSTRUCT */

static HMODULE hshell32;
static int (WINAPI *pSHCreateDirectoryExA)(HWND, LPCSTR, LPSECURITY_ATTRIBUTES);
static int (WINAPI *pSHCreateDirectoryExW)(HWND, LPCWSTR, LPSECURITY_ATTRIBUTES);
static int (WINAPI *pSHFileOperationW)(LPSHFILEOPSTRUCTW);
static DWORD_PTR (WINAPI *pSHGetFileInfoW)(LPCWSTR, DWORD , SHFILEINFOW*, UINT, UINT);
static int (WINAPI *pSHPathPrepareForWriteA)(HWND, IUnknown*, LPCSTR, DWORD);
static int (WINAPI *pSHPathPrepareForWriteW)(HWND, IUnknown*, LPCWSTR, DWORD);

static void InitFunctionPointers(void)
{
    hshell32 = GetModuleHandleA("shell32.dll");
    pSHCreateDirectoryExA = (void*)GetProcAddress(hshell32, "SHCreateDirectoryExA");
    pSHCreateDirectoryExW = (void*)GetProcAddress(hshell32, "SHCreateDirectoryExW");
    pSHFileOperationW = (void*)GetProcAddress(hshell32, "SHFileOperationW");
    pSHGetFileInfoW = (void*)GetProcAddress(hshell32, "SHGetFileInfoW");
    pSHPathPrepareForWriteA = (void*)GetProcAddress(hshell32, "SHPathPrepareForWriteA");
    pSHPathPrepareForWriteW = (void*)GetProcAddress(hshell32, "SHPathPrepareForWriteW");
}

/* creates a file with the specified name for tests */
static void createTestFile(const CHAR *name)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, name, strlen(name), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);
    CloseHandle(file);
}

static void createTestFileW(const WCHAR *name)
{
    HANDLE file;

    file = CreateFileW(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file\n");
    CloseHandle(file);
}

static BOOL file_exists(const CHAR *name)
{
    return GetFileAttributesA(name) != INVALID_FILE_ATTRIBUTES;
}

static BOOL dir_exists(const CHAR *name)
{
    DWORD attr;
    BOOL dir;

    attr = GetFileAttributesA(name);
    dir = ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);

    return ((attr != INVALID_FILE_ATTRIBUTES) && dir);
}

static BOOL file_existsW(LPCWSTR name)
{
  return GetFileAttributesW(name) != INVALID_FILE_ATTRIBUTES;
}

static BOOL file_has_content(const CHAR *name, const CHAR *content)
{
    CHAR buf[MAX_PATH];
    HANDLE file;
    DWORD read;

    file = CreateFileA(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    ReadFile(file, buf, MAX_PATH - 1, &read, NULL);
    buf[read] = 0;
    CloseHandle(file);
    return strcmp(buf, content)==0;
}

/* initializes the tests */
static void init_shfo_tests(void)
{
    int len;

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len-1] == '\\'))
        CURR_DIR[len-1] = 0;

    createTestFile("test1.txt");
    createTestFile("test2.txt");
    createTestFile("test3.txt");
    createTestFile("test_5.txt");
    CreateDirectoryA("test4.txt", NULL);
    CreateDirectoryA("testdir2", NULL);
    CreateDirectoryA("testdir2\\nested", NULL);
    createTestFile("testdir2\\one.txt");
    createTestFile("testdir2\\nested\\two.txt");
}

/* cleans after tests */
static void clean_after_shfo_tests(void)
{
    DeleteFileA("test1.txt");
    DeleteFileA("test2.txt");
    DeleteFileA("test3.txt");
    DeleteFileA("test_5.txt");
    DeleteFileA("one.txt");
    DeleteFileA("test4.txt\\test1.txt");
    DeleteFileA("test4.txt\\test2.txt");
    DeleteFileA("test4.txt\\test3.txt");
    DeleteFileA("test4.txt\\one.txt");
    DeleteFileA("test4.txt\\nested\\two.txt");
    RemoveDirectoryA("test4.txt\\nested");
    RemoveDirectoryA("test4.txt");
    DeleteFileA("testdir2\\one.txt");
    DeleteFileA("testdir2\\test1.txt");
    DeleteFileA("testdir2\\test2.txt");
    DeleteFileA("testdir2\\test3.txt");
    DeleteFileA("testdir2\\test4.txt\\test1.txt");
    DeleteFileA("testdir2\\nested\\two.txt");
    RemoveDirectoryA("testdir2\\test4.txt");
    RemoveDirectoryA("testdir2\\nested");
    RemoveDirectoryA("testdir2");
    RemoveDirectoryA("c:\\testdir3");
    DeleteFileA("nonexistent\\notreal\\test2.txt");
    RemoveDirectoryA("nonexistent\\notreal");
    RemoveDirectoryA("nonexistent");
}


static void test_get_file_info(void)
{
    DWORD rc, rc2;
    SHFILEINFOA shfi, shfi2;
    SHFILEINFOW shfiw;
    char notepad[MAX_PATH];

    /* Test whether fields of SHFILEINFOA are always cleared */
    memset(&shfi, 0xcf, sizeof(shfi));
    rc=SHGetFileInfoA("", 0, &shfi, sizeof(shfi), 0);
    ok(rc == 1, "SHGetFileInfoA('' | 0) should return 1, got 0x%x\n", rc);
    todo_wine ok(shfi.hIcon == 0, "SHGetFileInfoA('' | 0) did not clear hIcon\n");
    todo_wine ok(shfi.szDisplayName[0] == 0, "SHGetFileInfoA('' | 0) did not clear szDisplayName[0]\n");
    todo_wine ok(shfi.szTypeName[0] == 0, "SHGetFileInfoA('' | 0) did not clear szTypeName[0]\n");
    ok(shfi.iIcon == 0xcfcfcfcf ||
       broken(shfi.iIcon != 0xcfcfcfcf), /* NT4 doesn't clear but sets this field */
       "SHGetFileInfoA('' | 0) should not clear iIcon\n");
    ok(shfi.dwAttributes == 0xcfcfcfcf ||
       broken(shfi.dwAttributes != 0xcfcfcfcf), /* NT4 doesn't clear but sets this field */
       "SHGetFileInfoA('' | 0) should not clear dwAttributes\n");

    if (pSHGetFileInfoW)
    {
        HANDLE unset_icon;
        /* Test whether fields of SHFILEINFOW are always cleared */
        memset(&shfiw, 0xcf, sizeof(shfiw));
        memset(&unset_icon, 0xcf, sizeof(unset_icon));
        rc=pSHGetFileInfoW(NULL, 0, &shfiw, sizeof(shfiw), 0);
        ok(!rc, "SHGetFileInfoW(NULL | 0) should fail\n");
        ok(shfiw.hIcon == unset_icon, "SHGetFileInfoW(NULL | 0) should not clear hIcon\n");
        ok(shfiw.szDisplayName[0] == 0xcfcf, "SHGetFileInfoW(NULL | 0) should not clear szDisplayName[0]\n");
        ok(shfiw.szTypeName[0] == 0xcfcf, "SHGetFileInfoW(NULL | 0) should not clear szTypeName[0]\n");
        ok(shfiw.iIcon == 0xcfcfcfcf, "SHGetFileInfoW(NULL | 0) should not clear iIcon\n");
        ok(shfiw.dwAttributes == 0xcfcfcfcf, "SHGetFileInfoW(NULL | 0) should not clear dwAttributes\n");
    }
    else
        win_skip("SHGetFileInfoW is not available\n");


    /* Test some flag combinations that MSDN claims are not allowed,
     * but which work anyway
     */
    memset(&shfi, 0xcf, sizeof(shfi));
    rc=SHGetFileInfoA("c:\\nonexistent", FILE_ATTRIBUTE_DIRECTORY,
                      &shfi, sizeof(shfi),
                      SHGFI_ATTRIBUTES | SHGFI_USEFILEATTRIBUTES);
    ok(rc == 1, "SHGetFileInfoA(c:\\nonexistent | SHGFI_ATTRIBUTES) should return 1, got 0x%x\n", rc);
    if (rc)
        ok(shfi.dwAttributes != 0xcfcfcfcf, "dwFileAttributes is not set\n");
    todo_wine ok(shfi.hIcon == 0, "SHGetFileInfoA(c:\\nonexistent | SHGFI_ATTRIBUTES) did not clear hIcon\n");
    todo_wine ok(shfi.szDisplayName[0] == 0, "SHGetFileInfoA(c:\\nonexistent | SHGFI_ATTRIBUTES) did not clear szDisplayName[0]\n");
    todo_wine ok(shfi.szTypeName[0] == 0, "SHGetFileInfoA(c:\\nonexistent | SHGFI_ATTRIBUTES) did not clear szTypeName[0]\n");
    ok(shfi.iIcon == 0xcfcfcfcf ||
       broken(shfi.iIcon != 0xcfcfcfcf), /* NT4 doesn't clear but sets this field */
       "SHGetFileInfoA(c:\\nonexistent | SHGFI_ATTRIBUTES) should not clear iIcon\n");

    rc=SHGetFileInfoA("c:\\nonexistent", FILE_ATTRIBUTE_DIRECTORY,
                      &shfi, sizeof(shfi),
                      SHGFI_EXETYPE | SHGFI_USEFILEATTRIBUTES);
    todo_wine ok(rc == 1, "SHGetFileInfoA(c:\\nonexistent | SHGFI_EXETYPE) should return 1, got 0x%x\n", rc);

    /* Test SHGFI_USEFILEATTRIBUTES support */
    strcpy(shfi.szDisplayName, "dummy");
    shfi.iIcon=0xdeadbeef;
    rc=SHGetFileInfoA("c:\\nonexistent", FILE_ATTRIBUTE_DIRECTORY,
                      &shfi, sizeof(shfi),
                      SHGFI_ICONLOCATION | SHGFI_USEFILEATTRIBUTES);
    ok(rc == 1, "SHGetFileInfoA(c:\\nonexistent) should return 1, got 0x%x\n", rc);
    if (rc)
    {
        ok(strcmp(shfi.szDisplayName, "dummy"), "SHGetFileInfoA(c:\\nonexistent) displayname is not set\n");
        ok(shfi.iIcon != 0xdeadbeef, "SHGetFileInfoA(c:\\nonexistent) iIcon is not set\n");
    }

    /* Wine does not have a default icon for text files, and Windows 98 fails
     * if we give it an empty executable. So use notepad.exe as the test
     */
    if (SearchPathA(NULL, "notepad.exe", NULL, sizeof(notepad), notepad, NULL))
    {
        strcpy(shfi.szDisplayName, "dummy");
        shfi.iIcon=0xdeadbeef;
        rc=SHGetFileInfoA(notepad, GetFileAttributesA(notepad),
                          &shfi, sizeof(shfi),
                          SHGFI_ICONLOCATION | SHGFI_USEFILEATTRIBUTES);
        ok(rc == 1, "SHGetFileInfoA(%s, SHGFI_USEFILEATTRIBUTES) should return 1, got 0x%x\n", notepad, rc);
        strcpy(shfi2.szDisplayName, "dummy");
        shfi2.iIcon=0xdeadbeef;
        rc2=SHGetFileInfoA(notepad, 0,
                           &shfi2, sizeof(shfi2),
                           SHGFI_ICONLOCATION);
        ok(rc2 == 1, "SHGetFileInfoA(%s) failed %x\n", notepad, rc2);
        if (rc && rc2)
        {
            ok(lstrcmpiA(shfi2.szDisplayName, shfi.szDisplayName) == 0, "wrong display name %s != %s\n", shfi.szDisplayName, shfi2.szDisplayName);
            ok(shfi2.iIcon == shfi.iIcon, "wrong icon index %d != %d\n", shfi.iIcon, shfi2.iIcon);
        }
    }

    /* with a directory now */
    strcpy(shfi.szDisplayName, "dummy");
    shfi.iIcon=0xdeadbeef;
    rc=SHGetFileInfoA("test4.txt", GetFileAttributesA("test4.txt"),
                      &shfi, sizeof(shfi),
                      SHGFI_ICONLOCATION | SHGFI_USEFILEATTRIBUTES);
    ok(rc == 1, "SHGetFileInfoA(test4.txt/, SHGFI_USEFILEATTRIBUTES) should return 1, got 0x%x\n", rc);
    strcpy(shfi2.szDisplayName, "dummy");
    shfi2.iIcon=0xdeadbeef;
    rc2=SHGetFileInfoA("test4.txt", 0,
                      &shfi2, sizeof(shfi2),
                      SHGFI_ICONLOCATION);
    ok(rc2 == 1, "SHGetFileInfoA(test4.txt/) should return 1, got 0x%x\n", rc2);
    if (rc && rc2)
    {
        ok(lstrcmpiA(shfi2.szDisplayName, shfi.szDisplayName) == 0, "wrong display name %s != %s\n", shfi.szDisplayName, shfi2.szDisplayName);
        ok(shfi2.iIcon == shfi.iIcon, "wrong icon index %d != %d\n", shfi.iIcon, shfi2.iIcon);
    }
    /* with drive root directory */
    strcpy(shfi.szDisplayName, "dummy");
    strcpy(shfi.szTypeName, "dummy");
    shfi.hIcon=(HICON) 0xdeadbeef;
    shfi.iIcon=0xdeadbeef;
    shfi.dwAttributes=0xdeadbeef;
    rc=SHGetFileInfoA("c:\\", 0, &shfi, sizeof(shfi),
                      SHGFI_TYPENAME | SHGFI_DISPLAYNAME | SHGFI_ICON | SHGFI_SMALLICON);
    ok(rc == 1, "SHGetFileInfoA(c:\\) should return 1, got 0x%x\n", rc);
    ok(strcmp(shfi.szDisplayName, "dummy") != 0, "display name was expected to change\n");
    ok(strcmp(shfi.szTypeName, "dummy") != 0, "type name was expected to change\n");
    ok(shfi.hIcon != (HICON) 0xdeadbeef, "hIcon was expected to change\n");
    ok(shfi.iIcon != 0xdeadbeef, "iIcon was expected to change\n");
}

static void test_get_file_info_iconlist(void)
{
    /* Test retrieving a handle to the system image list, and
     * what that returns for hIcon
     */
    HRESULT hr;
    HIMAGELIST hSysImageList;
    LPITEMIDLIST pidList;
    SHFILEINFOA shInfoa;
    SHFILEINFOW shInfow;

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidList);
    if (FAILED(hr)) {
         skip("can't get desktop pidl\n");
         return;
    }

    memset(&shInfoa, 0xcf, sizeof(shInfoa));
    hSysImageList = (HIMAGELIST) SHGetFileInfoA((const char *)pidList, 0,
            &shInfoa, sizeof(shInfoa),
	    SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_PIDL);
    ok((hSysImageList != INVALID_HANDLE_VALUE) && (hSysImageList > (HIMAGELIST) 0xffff), "Can't get handle for CSIDL_DESKTOP imagelist\n");
    todo_wine ok(shInfoa.hIcon == 0, "SHGetFileInfoA(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) did not clear hIcon\n");
    todo_wine ok(shInfoa.szTypeName[0] == 0, "SHGetFileInfoA(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) did not clear szTypeName[0]\n");
    ok(shInfoa.iIcon != 0xcfcfcfcf, "SHGetFileInfoA(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) should set iIcon\n");
    ok(shInfoa.dwAttributes == 0xcfcfcfcf ||
       shInfoa.dwAttributes ==  0 || /* Vista */
       broken(shInfoa.dwAttributes != 0xcfcfcfcf), /* NT4 doesn't clear but sets this field */
       "SHGetFileInfoA(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL), unexpected dwAttributes\n");
    CloseHandle(hSysImageList);

    if (!pSHGetFileInfoW)
    {
        win_skip("SHGetFileInfoW is not available\n");
        ILFree(pidList);
        return;
    }

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hSysImageList = (HIMAGELIST) pSHGetFileInfoW((const WCHAR *)pidList, 0,
            &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_PIDL);
    if (!hSysImageList)
    {
        win_skip("SHGetFileInfoW is not implemented\n");
        return;
    }
    ok((hSysImageList != INVALID_HANDLE_VALUE) && (hSysImageList > (HIMAGELIST) 0xffff), "Can't get handle for CSIDL_DESKTOP imagelist\n");
    todo_wine ok(shInfow.hIcon == 0, "SHGetFileInfoW(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) did not clear hIcon\n");
    ok(shInfow.szTypeName[0] == 0, "SHGetFileInfoW(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) did not clear szTypeName[0]\n");
    ok(shInfow.iIcon != 0xcfcfcfcf, "SHGetFileInfoW(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) should set iIcon\n");
    ok(shInfow.dwAttributes == 0xcfcfcfcf ||
       shInfoa.dwAttributes ==  0, /* Vista */
       "SHGetFileInfoW(CSIDL_DESKTOP, SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_PIDL) unexpected dwAttributes\n");
    CloseHandle(hSysImageList);

    /* Various suposidly invalid flag testing */
    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr =  pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON);
    ok(hr != 0, "SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON Failed\n");
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf ||
       shInfoa.dwAttributes==0, /* Vista */
       "unexpected dwAttributes\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_ICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON);
    ok(hr != 0, " SHGFI_ICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON Failed\n");
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.hIcon!=(HICON)0xcfcfcfcf && shInfow.hIcon!=0,"hIcon invalid\n");
    if (shInfow.hIcon!=(HICON)0xcfcfcfcf) DestroyIcon(shInfow.hIcon);
    todo_wine ok(shInfow.dwAttributes==0,"dwAttributes not set\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_ICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_LARGEICON);
    ok(hr != 0, "SHGFI_ICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_LARGEICON Failed\n");
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.hIcon!=(HICON)0xcfcfcfcf && shInfow.hIcon!=0,"hIcon invalid\n");
    if (shInfow.hIcon != (HICON)0xcfcfcfcf) DestroyIcon(shInfow.hIcon);
    todo_wine ok(shInfow.dwAttributes==0,"dwAttributes not set\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_LARGEICON);
    ok(hr != 0, "SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_LARGEICON Failed\n");
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf ||
       shInfoa.dwAttributes==0, /* Vista */
       "unexpected dwAttributes\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_OPENICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON);
    ok(hr != 0, "SHGFI_OPENICON|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf,"dwAttributes modified\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SHELLICONSIZE|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON);
    ok(hr != 0, "SHGFI_SHELLICONSIZE|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf,"dwAttributes modified\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SHELLICONSIZE|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON);
    ok(hr != 0, "SHGFI_SHELLICONSIZE|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf,"dwAttributes modified\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|
        SHGFI_ATTRIBUTES);
    ok(hr != 0, "SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|SHGFI_ATTRIBUTES Failed\n");
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes!=0xcfcfcfcf,"dwAttributes not set\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|
        SHGFI_EXETYPE);
    todo_wine ok(hr != 0, "SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|SHGFI_EXETYPE Failed\n");
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf ||
       shInfoa.dwAttributes==0, /* Vista */
       "unexpected dwAttributes\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
        SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|SHGFI_EXETYPE);
    todo_wine ok(hr != 0, "SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|SHGFI_EXETYPE Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf,"dwAttributes modified\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
        SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|SHGFI_ATTRIBUTES);
    ok(hr != 0, "SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_SMALLICON|SHGFI_ATTRIBUTES Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes!=0xcfcfcfcf,"dwAttributes not set\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
	    SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|
        SHGFI_ATTRIBUTES);
    ok(hr != 0, "SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_ATTRIBUTES Failed\n");
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes!=0xcfcfcfcf,"dwAttributes not set\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
        SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_EXETYPE);
    todo_wine ok(hr != 0, "SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_EXETYPE Failed\n");
    ok(shInfow.iIcon!=0xcfcfcfcf, "Icon Index Missing\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf ||
       shInfoa.dwAttributes==0, /* Vista */
       "unexpected dwAttributes\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
        SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_EXETYPE);
    todo_wine ok(hr != 0, "SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_EXETYPE Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes==0xcfcfcfcf,"dwAttributes modified\n");

    memset(&shInfow, 0xcf, sizeof(shInfow));
    hr = pSHGetFileInfoW((const WCHAR *)pidList, 0, &shInfow, sizeof(shInfow),
        SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_ATTRIBUTES);
    ok(hr != 0, "SHGFI_USEFILEATTRIBUTES|SHGFI_PIDL|SHGFI_ATTRIBUTES Failed\n");
    todo_wine ok(shInfow.iIcon==0xcfcfcfcf, "Icon Index Modified\n");
    ok(shInfow.dwAttributes!=0xcfcfcfcf,"dwAttributes not set\n");

    ILFree(pidList);
}


/*
 puts into the specified buffer file names with current directory.
 files - string with file names, separated by null characters. Ends on a double
 null characters
*/
static void set_curr_dir_path(CHAR *buf, const CHAR* files)
{
    buf[0] = 0;
    while (files[0])
    {
        strcpy(buf, CURR_DIR);
        buf += strlen(buf);
        buf[0] = '\\';
        buf++;
        strcpy(buf, files);
        buf += strlen(buf) + 1;
        files += strlen(files) + 1;
    }
    buf[0] = 0;
}


/* tests the FO_DELETE action */
static void test_delete(void)
{
    SHFILEOPSTRUCTA shfo;
    DWORD ret;
    CHAR buf[sizeof(CURR_DIR)+sizeof("/test?.txt")+1];

    sprintf(buf, "%s\\%s", CURR_DIR, "test?.txt");
    buf[strlen(buf) + 1] = '\0';

    shfo.hwnd = NULL;
    shfo.wFunc = FO_DELETE;
    shfo.pFrom = buf;
    shfo.pTo = NULL;
    shfo.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_SILENT;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    ok(!SHFileOperationA(&shfo), "Deletion was not successful\n");
    ok(dir_exists("test4.txt"), "Directory should not have been removed\n");
    ok(!file_exists("test1.txt"), "File should have been removed\n");
    ok(!file_exists("test2.txt"), "File should have been removed\n");
    ok(!file_exists("test3.txt"), "File should have been removed\n");

    ret = SHFileOperationA(&shfo);
    ok(ret == ERROR_SUCCESS, "Directory exists, but is not removed, ret=%d\n", ret);
    ok(dir_exists("test4.txt"), "Directory should not have been removed\n");

    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;

    ok(!SHFileOperationA(&shfo), "Directory is not removed\n");
    ok(!dir_exists("test4.txt"), "Directory should have been removed\n");

    ret = SHFileOperationA(&shfo);
    ok(!ret, "The requested file does not exist, ret=%d\n", ret);

    init_shfo_tests();
    sprintf(buf, "%s\\%s", CURR_DIR, "test4.txt");
    buf[strlen(buf) + 1] = '\0';
    ok(MoveFileA("test1.txt", "test4.txt\\test1.txt"), "Filling the subdirectory failed\n");
    ok(!SHFileOperationA(&shfo), "Directory is not removed\n");
    ok(!dir_exists("test4.txt"), "Directory is not removed\n");

    init_shfo_tests();
    shfo.pFrom = "test1.txt\0test4.txt\0";
    ok(!SHFileOperationA(&shfo), "Directory and a file are not removed\n");
    ok(!file_exists("test1.txt"), "The file should have been removed\n");
    ok(!dir_exists("test4.txt"), "Directory should have been removed\n");
    ok(file_exists("test2.txt"), "This file should not have been removed\n");

    /* FOF_FILESONLY does not delete a dir matching a wildcard */
    init_shfo_tests();
    shfo.fFlags |= FOF_FILESONLY;
    shfo.pFrom = "*.txt\0";
    ok(!SHFileOperationA(&shfo), "Failed to delete files\n");
    ok(!file_exists("test1.txt"), "test1.txt should have been removed\n");
    ok(!file_exists("test_5.txt"), "test_5.txt should have been removed\n");
    ok(dir_exists("test4.txt"), "test4.txt should not have been removed\n");

    /* FOF_FILESONLY only deletes a dir if explicitly specified */
    init_shfo_tests();
    shfo.pFrom = "test_?.txt\0test4.txt\0";
    ok(!SHFileOperationA(&shfo), "Failed to delete files and directory\n");
    ok(!dir_exists("test4.txt") ||
       broken(dir_exists("test4.txt")), /* NT4 */
      "test4.txt should have been removed\n");
    ok(!file_exists("test_5.txt"), "test_5.txt should have been removed\n");
    ok(file_exists("test1.txt"), "test1.txt should not have been removed\n");

    /* try to delete an invalid filename */
    if (0) {
        /* this crashes on win9x */
        init_shfo_tests();
        shfo.pFrom = "\0";
        shfo.fFlags &= ~FOF_FILESONLY;
        shfo.fAnyOperationsAborted = FALSE;
        ret = SHFileOperationA(&shfo);
        ok(ret == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %d\n", ret);
        ok(!shfo.fAnyOperationsAborted, "Expected no aborted operations\n");
        ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");
    }

    /* try an invalid function */
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0";
    shfo.wFunc = 0;
    ret = SHFileOperationA(&shfo);
    ok(ret == ERROR_INVALID_PARAMETER ||
       broken(ret == ERROR_SUCCESS), /* Win9x, NT4 */
       "Expected ERROR_INVALID_PARAMETER, got %d\n", ret);
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");

    /* try an invalid list, only one null terminator */
    if (0) {
        /* this crashes on win9x */
        init_shfo_tests();
        shfo.pFrom = "";
        shfo.wFunc = FO_DELETE;
        ret = SHFileOperationA(&shfo);
        ok(ret == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %d\n", ret);
        ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");
    }

    /* delete a nonexistent file */
    shfo.pFrom = "nonexistent.txt\0";
    shfo.wFunc = FO_DELETE;
    ret = SHFileOperationA(&shfo);
    ok(ret == 1026 ||
       ret == ERROR_FILE_NOT_FOUND || /* Vista */
       broken(ret == ERROR_SUCCESS), /* NT4 */
       "Expected 1026 or ERROR_FILE_NOT_FOUND, got %d\n", ret);

    /* delete a dir, and then a file inside the dir, same as
    * deleting a nonexistent file
    */
    if (ret != ERROR_FILE_NOT_FOUND)
    {
        /* Vista would throw up a dialog box that we can't suppress */
        init_shfo_tests();
        shfo.pFrom = "testdir2\0testdir2\\one.txt\0";
        ret = SHFileOperationA(&shfo);
        ok(ret == ERROR_PATH_NOT_FOUND ||
           broken(ret == ERROR_SUCCESS), /* NT4 */
           "Expected ERROR_PATH_NOT_FOUND, got %d\n", ret);
        ok(!dir_exists("testdir2"), "Expected testdir2 to not exist\n");
        ok(!file_exists("testdir2\\one.txt"), "Expected testdir2\\one.txt to not exist\n");
    }
    else
        skip("Test would show a dialog box\n");

    /* delete an existent file and a nonexistent file */
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0nonexistent.txt\0test2.txt\0";
    shfo.wFunc = FO_DELETE;
    ret = SHFileOperationA(&shfo);
    ok(ret == 1026 ||
       ret == ERROR_FILE_NOT_FOUND || /* Vista */
       broken(ret == ERROR_SUCCESS), /* NT4 */
       "Expected 1026 or ERROR_FILE_NOT_FOUND, got %d\n", ret);
    todo_wine
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");
    ok(file_exists("test2.txt"), "Expected test2.txt to exist\n");

    /* delete a nonexistent file in an existent dir or a nonexistent dir */
    init_shfo_tests();
    shfo.pFrom = "testdir2\\nonexistent.txt\0";
    ret = SHFileOperationA(&shfo);
    ok(ret == ERROR_FILE_NOT_FOUND || /* Vista */
       broken(ret == 0x402) || /* XP */
       broken(ret == ERROR_SUCCESS), /* NT4 */
       "Expected 0x402 or ERROR_FILE_NOT_FOUND, got %x\n", ret);
    shfo.pFrom = "nonexistent\\one.txt\0";
    ret = SHFileOperationA(&shfo);
    ok(ret == DE_INVALIDFILES || /* Vista or later */
       broken(ret == 0x402), /* XP */
       "Expected 0x402 or DE_INVALIDFILES, got %x\n", ret);

    /* try the FOF_NORECURSION flag, continues deleting subdirs */
    init_shfo_tests();
    shfo.pFrom = "testdir2\0";
    shfo.fFlags |= FOF_NORECURSION;
    ret = SHFileOperationA(&shfo);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ok(!file_exists("testdir2\\one.txt"), "Expected testdir2\\one.txt to not exist\n");
    ok(!dir_exists("testdir2\\nested"), "Expected testdir2\\nested to not exist\n");
}

/* tests the FO_RENAME action */
static void test_rename(void)
{
    SHFILEOPSTRUCTA shfo, shfo2;
    CHAR from[5*MAX_PATH];
    CHAR to[5*MAX_PATH];
    DWORD retval;

    shfo.hwnd = NULL;
    shfo.wFunc = FO_RENAME;
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_ALREADY_EXISTS ||
       retval == DE_FILEDESTISFLD || /* Vista */
       broken(retval == ERROR_INVALID_NAME), /* Win9x, NT4 */
       "Expected ERROR_ALREADY_EXISTS or DE_FILEDESTISFLD, got %d\n", retval);
    ok(file_exists("test1.txt"), "The file is renamed\n");

    set_curr_dir_path(from, "test3.txt\0");
    set_curr_dir_path(to, "test4.txt\\test1.txt\0");
    retval = SHFileOperationA(&shfo);
    if (retval == DE_DIFFDIR)
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(!file_exists("test4.txt\\test1.txt"), "The file is renamed\n");
    }
    else
    {
        ok(retval == ERROR_SUCCESS, "File is renamed moving to other directory\n");
        ok(file_exists("test4.txt\\test1.txt"), "The file is not renamed\n");
    }

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_GEN_FAILURE ||
       retval == DE_MANYSRC1DEST || /* Vista */
       broken(retval == ERROR_SUCCESS), /* Win9x */
       "Expected ERROR_GEN_FAILURE or DE_MANYSRC1DEST , got %d\n", retval);
    ok(file_exists("test1.txt"), "The file is renamed - many files are specified\n");

    memcpy(&shfo2, &shfo, sizeof(SHFILEOPSTRUCTA));
    shfo2.fFlags |= FOF_MULTIDESTFILES;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    retval = SHFileOperationA(&shfo2);
    ok(retval == ERROR_GEN_FAILURE ||
       retval == DE_MANYSRC1DEST || /* Vista */
       broken(retval == ERROR_SUCCESS), /* Win9x */
       "Expected ERROR_GEN_FAILURE or DE_MANYSRC1DEST files, got %d\n", retval);
    ok(file_exists("test1.txt"), "The file is not renamed - many files are specified\n");

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Rename file failed, retval = %d\n", retval);
    ok(!file_exists("test1.txt"), "The file is not renamed\n");
    ok(file_exists("test6.txt"), "The file is not renamed\n");

    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test1.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Rename file back failed, retval = %d\n", retval);

    set_curr_dir_path(from, "test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Rename dir failed, retval = %d\n", retval);
    ok(!dir_exists("test4.txt"), "The dir is not renamed\n");
    ok(dir_exists("test6.txt"), "The dir is not renamed\n");

    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Rename dir back failed, retval = %d\n", retval);
    ok(dir_exists("test4.txt"), "The dir is not renamed\n");

    /* try to rename more than one file to a single file */
    shfo.pFrom = "test1.txt\0test2.txt\0";
    shfo.pTo = "a.txt\0";
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_GEN_FAILURE ||
       retval == DE_MANYSRC1DEST || /* Vista */
       broken(retval == ERROR_SUCCESS), /* Win9x */
       "Expected ERROR_GEN_FAILURE or DE_MANYSRC1DEST, got %d\n", retval);
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");
    ok(file_exists("test2.txt"), "Expected test2.txt to exist\n");
    ok(!file_exists("a.txt"), "Expected a.txt to not exist\n");

    /* pFrom doesn't exist */
    shfo.pFrom = "idontexist\0";
    shfo.pTo = "newfile\0";
    retval = SHFileOperationA(&shfo);
    ok(retval == 1026 ||
       retval == ERROR_FILE_NOT_FOUND || /* Vista */
       broken(retval == ERROR_SUCCESS), /* NT4 */
       "Expected 1026 or ERROR_FILE_NOT_FOUND, got %d\n", retval);
    ok(!file_exists("newfile"), "Expected newfile to not exist\n");

    /* pTo already exist */
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "test2.txt\0";
    if (old_shell32)
        shfo.fFlags |= FOF_NOCONFIRMMKDIR;
    retval = SHFileOperationA(&shfo);
    if (retval == ERROR_SUCCESS)
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        createTestFile("test1.txt");
    }
    else
    {
        ok(retval == ERROR_ALREADY_EXISTS ||
           broken(retval == DE_OPCANCELLED) || /* NT4 */
           broken(retval == ERROR_INVALID_NAME), /* Win9x */
           "Expected ERROR_ALREADY_EXISTS, got %d\n", retval);
    }

    /* pFrom is valid, but pTo is empty */
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "\0";
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_CANCELLED ||
       retval == DE_DIFFDIR || /* Vista */
       broken(retval == DE_OPCANCELLED) || /* Win9x */
       broken(retval == 65652), /* NT4 */
       "Expected ERROR_CANCELLED or DE_DIFFDIR\n");
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");

    /* pFrom is empty */
    shfo.pFrom = "\0";
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_ACCESS_DENIED ||
       retval == DE_MANYSRC1DEST || /* Vista */
       broken(retval == ERROR_SUCCESS), /* Win9x */
       "Expected ERROR_ACCESS_DENIED or DE_MANYSRC1DEST, got %d\n", retval);

    /* pFrom is NULL, commented out because it crashes on nt 4.0 */
    if (0)
    {
        shfo.pFrom = NULL;
        retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", retval);
    }
}

/* tests the FO_COPY action */
static void test_copy(void)
{
    SHFILEOPSTRUCTA shfo, shfo2;
    CHAR from[5*MAX_PATH];
    CHAR to[5*MAX_PATH];
    FILEOP_FLAGS tmp_flags;
    DWORD retval;
    LPSTR ptr;
    BOOL on_nt4 = FALSE;
    BOOL ret;

    if (old_shell32)
    {
        win_skip("Too many differences for old shell32\n");
        return;
    }

    shfo.hwnd = NULL;
    shfo.wFunc = FO_COPY;
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    retval = SHFileOperationA(&shfo);
    if (dir_exists("test6.txt"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("test6.txt\\test1.txt"), "The file is not copied - many files "
           "are specified as a target\n");
        DeleteFileA("test6.txt\\test2.txt");
        RemoveDirectoryA("test6.txt\\test4.txt");
        RemoveDirectoryA("test6.txt");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(!file_exists("test6.txt"), "The file is copied - many files are "
           "specified as a target\n");
    }

    memcpy(&shfo2, &shfo, sizeof(SHFILEOPSTRUCTA));
    shfo2.fFlags |= FOF_MULTIDESTFILES;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    ok(!SHFileOperationA(&shfo2), "Can't copy many files\n");
    ok(file_exists("test6.txt"), "The file is not copied - many files are "
       "specified as a target\n");
    DeleteFileA("test6.txt");
    DeleteFileA("test7.txt");
    RemoveDirectoryA("test8.txt");

    /* number of sources does not correspond to number of targets */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    retval = SHFileOperationA(&shfo2);
    if (dir_exists("test6.txt"))
    {
        /* Vista and W2K8 (broken or new behavior?) */
        ok(retval == DE_DESTSAMETREE, "Expected DE_DESTSAMETREE, got %d\n", retval);
        ok(DeleteFileA("test6.txt\\test1.txt"), "The file is not copied - many files "
           "are specified as a target\n");
        RemoveDirectoryA("test6.txt");
        ok(DeleteFileA("test7.txt\\test2.txt"), "The file is not copied - many files "
           "are specified as a target\n");
        RemoveDirectoryA("test7.txt");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(!file_exists("test6.txt"), "The file is copied - many files are "
           "specified as a target\n");
    }

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    ok(!SHFileOperationA(&shfo), "Prepare test to check how directories are copied recursively\n");
    ok(file_exists("test4.txt\\test1.txt"), "The file is copied\n");

    set_curr_dir_path(from, "test?.txt\0");
    set_curr_dir_path(to, "testdir2\0");
    ok(!file_exists("testdir2\\test1.txt"), "The file is not copied yet\n");
    ok(!file_exists("testdir2\\test4.txt"), "The directory is not copied yet\n");
    ok(!SHFileOperationA(&shfo), "Files and directories are copied to directory\n");
    ok(file_exists("testdir2\\test1.txt"), "The file is copied\n");
    ok(file_exists("testdir2\\test4.txt"), "The directory is copied\n");
    ok(file_exists("testdir2\\test4.txt\\test1.txt"), "The file in subdirectory is copied\n");
    clean_after_shfo_tests();

    init_shfo_tests();
    shfo.fFlags |= FOF_FILESONLY;
    ok(!file_exists("testdir2\\test1.txt"), "The file is not copied yet\n");
    ok(!file_exists("testdir2\\test4.txt"), "The directory is not copied yet\n");
    ok(!SHFileOperationA(&shfo), "Files are copied to other directory\n");
    ok(file_exists("testdir2\\test1.txt"), "The file is copied\n");
    ok(!file_exists("testdir2\\test4.txt"), "The directory is copied\n");
    clean_after_shfo_tests();

    init_shfo_tests();
    set_curr_dir_path(from, "test1.txt\0test2.txt\0");
    ok(!file_exists("testdir2\\test1.txt"), "The file is not copied yet\n");
    ok(!file_exists("testdir2\\test2.txt"), "The file is not copied yet\n");
    ok(!SHFileOperationA(&shfo), "Files are copied to other directory\n");
    ok(file_exists("testdir2\\test1.txt"), "The file is copied\n");
    ok(file_exists("testdir2\\test2.txt"), "The file is copied\n");
    clean_after_shfo_tests();

    /* Copying multiple files with one not existing as source, fails the
       entire operation in Win98/ME/2K/XP, but not in 95/NT */
    init_shfo_tests();
    tmp_flags = shfo.fFlags;
    set_curr_dir_path(from, "test1.txt\0test10.txt\0test2.txt\0");
    ok(!file_exists("testdir2\\test1.txt"), "The file is not copied yet\n");
    ok(!file_exists("testdir2\\test2.txt"), "The file is not copied yet\n");
    retval = SHFileOperationA(&shfo);
    if (retval == ERROR_SUCCESS)
        /* Win 95/NT returns success but copies only the files up to the nonexistent source */
        ok(file_exists("testdir2\\test1.txt"), "The file is not copied\n");
    else
    {
        /* Failure if one source file does not exist */
        ok(retval == 1026 || /* Win 98/ME/2K/XP */
           retval == ERROR_FILE_NOT_FOUND, /* Vista and W2K8 */
           "Files are copied to other directory\n");
        ok(!file_exists("testdir2\\test1.txt"), "The file is copied\n");
    }
    ok(!file_exists("testdir2\\test2.txt"), "The file is copied\n");
    shfo.fFlags = tmp_flags;

    /* copy into a nonexistent directory */
    init_shfo_tests();
    shfo.fFlags = FOF_NOCONFIRMMKDIR;
    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "nonexistent\\notreal\\test2.txt\0");
    retval= SHFileOperationA(&shfo);
    ok(!retval, "Error copying into nonexistent directory\n");
    ok(file_exists("nonexistent"), "nonexistent not created\n");
    ok(file_exists("nonexistent\\notreal"), "nonexistent\\notreal not created\n");
    ok(file_exists("nonexistent\\notreal\\test2.txt"), "Directory not created\n");
    ok(!file_exists("nonexistent\\notreal\\test1.txt"), "test1.txt should not exist\n");

    /* a relative dest directory is OK */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0test2.txt\0test3.txt\0";
    shfo.pTo = "testdir2\0";
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(file_exists("testdir2\\test1.txt"), "Expected testdir2\\test1 to exist\n");

    /* try to overwrite an existing write protected file */
    clean_after_shfo_tests();
    init_shfo_tests();
    tmp_flags = shfo.fFlags;
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "test2.txt\0";
    /* suppress the error-dialog in win9x here */
    shfo.fFlags = FOF_NOERRORUI | FOF_NOCONFIRMATION | FOF_SILENT;
    ret = SetFileAttributesA(shfo.pTo, FILE_ATTRIBUTE_READONLY);
    ok(ret, "Failure to set file attributes (error %x)\n", GetLastError());
    retval = CopyFileA(shfo.pFrom, shfo.pTo, FALSE);
    ok(!retval && GetLastError() == ERROR_ACCESS_DENIED, "CopyFileA should have fail with ERROR_ACCESS_DENIED\n");
    retval = SHFileOperationA(&shfo);
    /* Does not work on Win95, Win95B, NT4WS and NT4SRV */
    ok(!retval || broken(retval == DE_OPCANCELLED), "SHFileOperationA failed to copy (error %x)\n", retval);
    /* Set back normal attributes to make the file deletion succeed */
    ret = SetFileAttributesA(shfo.pTo, FILE_ATTRIBUTE_NORMAL);
    ok(ret, "Failure to set file attributes (error %x)\n", GetLastError());
    shfo.fFlags = tmp_flags;

    /* try to copy files to a file */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.pFrom = from;
    shfo.pTo = to;
    /* suppress the error-dialog in win9x here */
    shfo.fFlags |= FOF_NOERRORUI;
    set_curr_dir_path(from, "test1.txt\0test2.txt\0");
    set_curr_dir_path(to, "test3.txt\0");
    shfo.fAnyOperationsAborted = 0xdeadbeef;
    retval = SHFileOperationA(&shfo);
    ok(shfo.fAnyOperationsAborted != 0xdeadbeef ||
       broken(shfo.fAnyOperationsAborted == 0xdeadbeef), /* NT4 */
       "Expected TRUE/FALSE fAnyOperationsAborted not 0xdeadbeef\n");
    if (retval == DE_FLDDESTISFILE || /* Vista and W2K8 */
        retval == DE_INVALIDFILES)    /* Win7 */
    {
        /* Most likely new behavior */
        ok(!shfo.fAnyOperationsAborted, "Didn't expect aborted operations\n");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(shfo.fAnyOperationsAborted, "Expected aborted operations\n");
    }
    ok(!file_exists("test3.txt\\test2.txt"), "Expected test3.txt\\test2.txt to not exist\n");

    /* try to copy many files to nonexistent directory */
    DeleteFileA(to);
    shfo.fFlags &= ~FOF_NOERRORUI;
    shfo.fAnyOperationsAborted = 0xdeadbeef;
    retval = SHFileOperationA(&shfo);
    ok(!shfo.fAnyOperationsAborted ||
       broken(shfo.fAnyOperationsAborted == 0xdeadbeef), /* NT4 */
       "Didn't expect aborted operations\n");
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(DeleteFileA("test3.txt\\test1.txt"), "Expected test3.txt\\test1.txt to exist\n");
    ok(DeleteFileA("test3.txt\\test2.txt"), "Expected test3.txt\\test1.txt to exist\n");
    ok(RemoveDirectoryA(to), "Expected test3.txt to exist\n");

    /* send in FOF_MULTIDESTFILES with too many destination files */
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0test2.txt\0test3.txt\0";
    shfo.pTo = "testdir2\\a.txt\0testdir2\\b.txt\0testdir2\\c.txt\0testdir2\\d.txt\0";
    shfo.fFlags |= FOF_NOERRORUI | FOF_MULTIDESTFILES;
    shfo.fAnyOperationsAborted = 0xdeadbeef;
    retval = SHFileOperationA(&shfo);
    ok(shfo.fAnyOperationsAborted != 0xdeadbeef ||
       broken(shfo.fAnyOperationsAborted == 0xdeadbeef), /* NT4 */
       "Expected TRUE/FALSE fAnyOperationsAborted not 0xdeadbeef\n");
    if (dir_exists("testdir2\\a.txt"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(!shfo.fAnyOperationsAborted, "Didn't expect aborted operations\n");
        ok(DeleteFileA("testdir2\\a.txt\\test1.txt"), "Expected testdir2\\a.txt\\test1.txt to exist\n");
        RemoveDirectoryA("testdir2\\a.txt");
        ok(DeleteFileA("testdir2\\b.txt\\test2.txt"), "Expected testdir2\\b.txt\\test2.txt to exist\n");
        RemoveDirectoryA("testdir2\\b.txt");
        ok(DeleteFileA("testdir2\\c.txt\\test3.txt"), "Expected testdir2\\c.txt\\test3.txt to exist\n");
        RemoveDirectoryA("testdir2\\c.txt");
        ok(!file_exists("testdir2\\d.txt"), "Expected testdir2\\d.txt to not exist\n");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(shfo.fAnyOperationsAborted ||
           broken(!shfo.fAnyOperationsAborted), /* NT4 */
           "Expected aborted operations\n");
        ok(!file_exists("testdir2\\a.txt"), "Expected testdir2\\a.txt to not exist\n");
    }

    /* send in FOF_MULTIDESTFILES with too many destination files */
    shfo.pFrom = "test1.txt\0test2.txt\0test3.txt\0";
    shfo.pTo = "e.txt\0f.txt\0";
    shfo.fAnyOperationsAborted = 0xdeadbeef;
    retval = SHFileOperationA(&shfo);
    ok(shfo.fAnyOperationsAborted != 0xdeadbeef ||
       broken(shfo.fAnyOperationsAborted == 0xdeadbeef), /* NT4 */
       "Expected TRUE/FALSE fAnyOperationsAborted not 0xdeadbeef\n");
    if (dir_exists("e.txt"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(!shfo.fAnyOperationsAborted, "Didn't expect aborted operations\n");
        ok(retval == DE_SAMEFILE, "Expected DE_SAMEFILE, got %d\n", retval);
        ok(DeleteFileA("e.txt\\test1.txt"), "Expected e.txt\\test1.txt to exist\n");
        RemoveDirectoryA("e.txt");
        ok(DeleteFileA("f.txt\\test2.txt"), "Expected f.txt\\test2.txt to exist\n");
        RemoveDirectoryA("f.txt");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(shfo.fAnyOperationsAborted ||
           broken(!shfo.fAnyOperationsAborted), /* NT4 */
           "Expected aborted operations\n");
        ok(!file_exists("e.txt"), "Expected e.txt to not exist\n");
    }

    /* use FOF_MULTIDESTFILES with files and a source directory */
    shfo.pFrom = "test1.txt\0test2.txt\0test4.txt\0";
    shfo.pTo = "testdir2\\a.txt\0testdir2\\b.txt\0testdir2\\c.txt\0";
    shfo.fAnyOperationsAborted = 0xdeadbeef;
    retval = SHFileOperationA(&shfo);
    ok(!shfo.fAnyOperationsAborted ||
       broken(shfo.fAnyOperationsAborted == 0xdeadbeef), /* NT4 */
       "Didn't expect aborted operations\n");
    ok(retval == ERROR_SUCCESS ||
       broken(retval == 0x100a1), /* WinMe */
       "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(DeleteFileA("testdir2\\a.txt"), "Expected testdir2\\a.txt to exist\n");
    ok(DeleteFileA("testdir2\\b.txt"), "Expected testdir2\\b.txt to exist\n");
    if (retval == ERROR_SUCCESS)
        ok(RemoveDirectoryA("testdir2\\c.txt"), "Expected testdir2\\c.txt to exist\n");

    /* try many dest files without FOF_MULTIDESTFILES flag */
    shfo.pFrom = "test1.txt\0test2.txt\0test3.txt\0";
    shfo.pTo = "a.txt\0b.txt\0c.txt\0";
    shfo.fAnyOperationsAborted = 0xdeadbeef;
    shfo.fFlags &= ~FOF_MULTIDESTFILES;
    retval = SHFileOperationA(&shfo);
    ok(shfo.fAnyOperationsAborted != 0xdeadbeef ||
       broken(shfo.fAnyOperationsAborted == 0xdeadbeef), /* NT4 */
       "Expected TRUE/FALSE fAnyOperationsAborted not 0xdeadbeef\n");
    if (dir_exists("a.txt"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(!shfo.fAnyOperationsAborted, "Didn't expect aborted operations\n");
        ok(DeleteFileA("a.txt\\test1.txt"), "Expected a.txt\\test1.txt to exist\n");
        ok(DeleteFileA("a.txt\\test2.txt"), "Expected a.txt\\test2.txt to exist\n");
        ok(DeleteFileA("a.txt\\test3.txt"), "Expected a.txt\\test3.txt to exist\n");
        RemoveDirectoryA("a.txt");
    }
    else
    {

        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(shfo.fAnyOperationsAborted ||
           broken(!shfo.fAnyOperationsAborted), /* NT4 */
           "Expected aborted operations\n");
        ok(!file_exists("a.txt"), "Expected a.txt to not exist\n");
    }

    /* try a glob */
    shfo.pFrom = "test?.txt\0";
    shfo.pTo = "testdir2\0";
    shfo.fFlags &= ~FOF_MULTIDESTFILES;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS ||
       broken(retval == 0x100a1), /* WinMe */
       "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(file_exists("testdir2\\test1.txt"), "Expected testdir2\\test1.txt to exist\n");

    /* try a glob with FOF_FILESONLY */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.pFrom = "test?.txt\0";
    shfo.fFlags |= FOF_FILESONLY;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(file_exists("testdir2\\test1.txt"), "Expected testdir2\\test1.txt to exist\n");
    ok(!dir_exists("testdir2\\test4.txt"), "Expected testdir2\\test4.txt to not exist\n");

    /* try a glob with FOF_MULTIDESTFILES and the same number
    * of dest files that we would expect
    */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.pTo = "testdir2\\a.txt\0testdir2\\b.txt\0testdir2\\c.txt\0testdir2\\d.txt\0";
    shfo.fFlags &= ~FOF_FILESONLY;
    shfo.fFlags |= FOF_MULTIDESTFILES;
    retval = SHFileOperationA(&shfo);
    if (dir_exists("testdir2\\a.txt"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("testdir2\\a.txt\\test1.txt"), "Expected testdir2\\a.txt\\test1.txt to exist\n");
        ok(DeleteFileA("testdir2\\a.txt\\test2.txt"), "Expected testdir2\\a.txt\\test2.txt to exist\n");
        ok(DeleteFileA("testdir2\\a.txt\\test3.txt"), "Expected testdir2\\a.txt\\test3.txt to exist\n");
        ok(RemoveDirectoryA("testdir2\\a.txt\\test4.txt"), "Expected testdir2\\a.txt\\test4.txt to exist\n");
        RemoveDirectoryA("testdir2\\a.txt");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(shfo.fAnyOperationsAborted ||
           broken(!shfo.fAnyOperationsAborted), /* NT4 */
           "Expected aborted operations\n");
        ok(!file_exists("testdir2\\a.txt"), "Expected testdir2\\test1.txt to not exist\n");
    }
    ok(!RemoveDirectoryA("b.txt"), "b.txt should not exist\n");

    /* copy one file to two others, second is ignored */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "b.txt\0c.txt\0";
    shfo.fAnyOperationsAborted = 0xdeadbeef;
    retval = SHFileOperationA(&shfo);
    ok(!shfo.fAnyOperationsAborted ||
       broken(shfo.fAnyOperationsAborted == 0xdeadbeef), /* NT4 */
       "Didn't expect aborted operations\n");
    if (retval == DE_OPCANCELLED)
    {
        /* NT4 fails and doesn't copy any files */
        ok(!file_exists("b.txt"), "Expected b.txt to not exist\n");
        /* Needed to skip some tests */
        win_skip("Skipping some tests on NT4\n");
        on_nt4 = TRUE;
    }
    else
    {
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("b.txt"), "Expected b.txt to exist\n");
    }
    ok(!DeleteFileA("c.txt"), "Expected c.txt to not exist\n");

    /* copy two file to three others, all fail */
    shfo.pFrom = "test1.txt\0test2.txt\0";
    shfo.pTo = "b.txt\0c.txt\0d.txt\0";
    shfo.fAnyOperationsAborted = 0xdeadbeef;
    retval = SHFileOperationA(&shfo);
    if (dir_exists("b.txt"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(!shfo.fAnyOperationsAborted, "Didn't expect aborted operations\n");
        ok(DeleteFileA("b.txt\\test1.txt"), "Expected b.txt\\test1.txt to exist\n");
        RemoveDirectoryA("b.txt");
        ok(DeleteFileA("c.txt\\test2.txt"), "Expected c.txt\\test2.txt to exist\n");
        RemoveDirectoryA("c.txt");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(shfo.fAnyOperationsAborted ||
           broken(shfo.fAnyOperationsAborted == 0xdeadbeef), /* NT4 */
           "Expected aborted operations\n");
        ok(!DeleteFileA("b.txt"), "Expected b.txt to not exist\n");
    }

    /* copy one file and one directory to three others */
    shfo.pFrom = "test1.txt\0test4.txt\0";
    shfo.pTo = "b.txt\0c.txt\0d.txt\0";
    shfo.fAnyOperationsAborted = 0xdeadbeef;
    retval = SHFileOperationA(&shfo);
    if (dir_exists("b.txt"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(!shfo.fAnyOperationsAborted, "Didn't expect aborted operations\n");
        ok(DeleteFileA("b.txt\\test1.txt"), "Expected b.txt\\test1.txt to exist\n");
        RemoveDirectoryA("b.txt");
        ok(RemoveDirectoryA("c.txt\\test4.txt"), "Expected c.txt\\test4.txt to exist\n");
        RemoveDirectoryA("c.txt");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(shfo.fAnyOperationsAborted ||
           broken(shfo.fAnyOperationsAborted == 0xdeadbeef), /* NT4 */
           "Expected aborted operations\n");
        ok(!DeleteFileA("b.txt"), "Expected b.txt to not exist\n");
        ok(!DeleteFileA("c.txt"), "Expected c.txt to not exist\n");
    }

    /* copy a directory with a file beneath it, plus some files */
    createTestFile("test4.txt\\a.txt");
    shfo.pFrom = "test4.txt\0test1.txt\0";
    shfo.pTo = "testdir2\0";
    shfo.fFlags &= ~FOF_MULTIDESTFILES;
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS ||
       broken(retval == 0x100a1), /* WinMe */
       "Expected ERROR_SUCCESS, got %d\n", retval);
    if (retval == ERROR_SUCCESS)
    {
        ok(DeleteFileA("testdir2\\test1.txt"), "Expected testdir2\\test1.txt to exist\n");
        ok(DeleteFileA("testdir2\\test4.txt\\a.txt"), "Expected a.txt to exist\n");
        ok(RemoveDirectoryA("testdir2\\test4.txt"), "Expected testdir2\\test4.txt to exist\n");
    }

    /* copy one directory and a file in that dir to another dir */
    shfo.pFrom = "test4.txt\0test4.txt\\a.txt\0";
    shfo.pTo = "testdir2\0";
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS ||
       broken(retval == 0x100a1), /* WinMe */
       "Expected ERROR_SUCCESS, got %d\n", retval);
    if (retval == ERROR_SUCCESS)
    {
        ok(DeleteFileA("testdir2\\test4.txt\\a.txt"), "Expected a.txt to exist\n");
        ok(DeleteFileA("testdir2\\a.txt"), "Expected testdir2\\a.txt to exist\n");
    }

    /* copy a file in a directory first, and then the directory to a nonexistent dir */
    shfo.pFrom = "test4.txt\\a.txt\0test4.txt\0";
    shfo.pTo = "nonexistent\0";
    retval = SHFileOperationA(&shfo);
    if (dir_exists("nonexistent"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("nonexistent\\test4.txt\\a.txt"), "Expected nonexistent\\test4.txt\\a.txt to exist\n");
        RemoveDirectoryA("nonexistent\\test4.txt");
        ok(DeleteFileA("nonexistent\\a.txt"), "Expected nonexistent\\a.txt to exist\n");
        RemoveDirectoryA("nonexistent");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(shfo.fAnyOperationsAborted ||
           broken(!shfo.fAnyOperationsAborted), /* NT4 */
           "Expected aborted operations\n");
        ok(!file_exists("nonexistent\\test4.txt"), "Expected nonexistent\\test4.txt to not exist\n");
    }
    DeleteFileA("test4.txt\\a.txt");

    /* destination is same as source file */
    shfo.pFrom = "test1.txt\0test2.txt\0test3.txt\0";
    shfo.pTo = "b.txt\0test2.txt\0c.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    shfo.fFlags = FOF_NOERRORUI | FOF_MULTIDESTFILES;
    retval = SHFileOperationA(&shfo);
    if (retval == DE_OPCANCELLED)
    {
        /* NT4 fails and doesn't copy any files */
        ok(!file_exists("b.txt"), "Expected b.txt to not exist\n");
    }
    else
    {
        ok(retval == DE_SAMEFILE, "Expected DE_SAMEFILE, got %d\n", retval);
        ok(DeleteFileA("b.txt"), "Expected b.txt to exist\n");
    }
    ok(!shfo.fAnyOperationsAborted, "Expected no operations to be aborted\n");
    ok(!file_exists("c.txt"), "Expected c.txt to not exist\n");

    /* destination is same as source directory */
    shfo.pFrom = "test1.txt\0test4.txt\0test3.txt\0";
    shfo.pTo = "b.txt\0test4.txt\0c.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    if (retval == DE_OPCANCELLED)
    {
        /* NT4 fails and doesn't copy any files */
        ok(!file_exists("b.txt"), "Expected b.txt to not exist\n");
    }
    else
    {
        ok(retval == ERROR_SUCCESS ||
           retval == DE_DESTSAMETREE, /* Vista */
           "Expected ERROR_SUCCESS or DE_DESTSAMETREE, got %d\n", retval);
        ok(DeleteFileA("b.txt"), "Expected b.txt to exist\n");
    }
    ok(!file_exists("c.txt"), "Expected c.txt to not exist\n");

    /* copy a directory into itself, error displayed in UI */
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "test4.txt\\newdir\0";
    shfo.fFlags &= ~FOF_MULTIDESTFILES;
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS ||
       retval == DE_DESTSUBTREE, /* Vista */
       "Expected ERROR_SUCCESS or DE_DESTSUBTREE, got %d\n", retval);
    ok(!RemoveDirectoryA("test4.txt\\newdir"), "Expected test4.txt\\newdir to not exist\n");

    /* copy a directory to itself, error displayed in UI */
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "test4.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS ||
       retval == DE_DESTSUBTREE, /* Vista */
       "Expected ERROR_SUCCESS or DE_DESTSUBTREE, got %d\n", retval);

    /* copy a file into a directory, and the directory into itself */
    shfo.pFrom = "test1.txt\0test4.txt\0";
    shfo.pTo = "test4.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    shfo.fFlags |= FOF_NOCONFIRMATION;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS ||
       retval == DE_DESTSUBTREE, /* Vista */
       "Expected ERROR_SUCCESS or DE_DESTSUBTREE, got %d\n", retval);
    ok(DeleteFileA("test4.txt\\test1.txt"), "Expected test4.txt\\test1.txt to exist\n");

    /* copy a file to a file, and the directory into itself */
    shfo.pFrom = "test1.txt\0test4.txt\0";
    shfo.pTo = "test4.txt\\a.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    if (dir_exists("test4.txt\\a.txt"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == DE_DESTSUBTREE, "Expected DE_DESTSUBTREE, got %d\n", retval);
        ok(DeleteFileA("test4.txt\\a.txt\\test1.txt"), "Expected test4.txt\\a.txt\\test1.txt to exist\n");
        RemoveDirectoryA("test4.txt\\a.txt");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(!file_exists("test4.txt\\a.txt"), "Expected test4.txt\\a.txt to not exist\n");
    }

    /* copy a nonexistent file to a nonexistent directory */
    shfo.pFrom = "e.txt\0";
    shfo.pTo = "nonexistent\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperationA(&shfo);
    ok(retval == 1026 ||
       retval == ERROR_FILE_NOT_FOUND || /* Vista */
       broken(retval == ERROR_SUCCESS), /* NT4 */
       "Expected 1026 or ERROR_FILE_NOT_FOUND, got %d\n", retval);
    ok(!file_exists("nonexistent\\e.txt"), "Expected nonexistent\\e.txt to not exist\n");
    ok(!file_exists("nonexistent"), "Expected nonexistent to not exist\n");

    /* Overwrite tests */
    clean_after_shfo_tests();
    init_shfo_tests();
    if (!on_nt4)
    {
        /* NT4 would throw up some dialog boxes and doesn't copy files that are needed
         * in subsequent tests.
         */
        shfo.fFlags = FOF_NOCONFIRMATION;
        shfo.pFrom = "test1.txt\0";
        shfo.pTo = "test2.txt\0";
        shfo.fAnyOperationsAborted = 0xdeadbeef;
        /* without FOF_NOCONFIRMATION the confirmation is Yes/No */
        retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(!shfo.fAnyOperationsAborted, "Didn't expect aborted operations\n");
        ok(file_has_content("test2.txt", "test1.txt\n"), "The file was not copied\n");

        shfo.pFrom = "test3.txt\0test1.txt\0";
        shfo.pTo = "test2.txt\0one.txt\0";
        shfo.fFlags = FOF_NOCONFIRMATION | FOF_MULTIDESTFILES;
        /* without FOF_NOCONFIRMATION the confirmation is Yes/Yes to All/No/Cancel */
        retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(file_has_content("test2.txt", "test3.txt\n"), "The file was not copied\n");

        shfo.pFrom = "one.txt\0";
        shfo.pTo = "testdir2\0";
        shfo.fFlags = FOF_NOCONFIRMATION;
        /* without FOF_NOCONFIRMATION the confirmation is Yes/No */
        retval = SHFileOperationA(&shfo);
        ok(retval == 0, "Expected 0, got %d\n", retval);
        ok(file_has_content("testdir2\\one.txt", "test1.txt\n"), "The file was not copied\n");
    }

    createTestFile("test4.txt\\test1.txt");
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "testdir2\0";
    /* WinMe needs FOF_NOERRORUI */
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS ||
       broken(retval == 0x100a1), /* WinMe */
       "Expected ERROR_SUCCESS, got %d\n", retval);
    shfo.fFlags = FOF_NOCONFIRMATION;
    if (ERROR_SUCCESS)
    {
        createTestFile("test4.txt\\.\\test1.txt"); /* modify the content of the file */
        /* without FOF_NOCONFIRMATION the confirmation is "This folder already contains a folder named ..." */
        retval = SHFileOperationA(&shfo);
        ok(retval == 0, "Expected 0, got %d\n", retval);
        ok(file_has_content("testdir2\\test4.txt\\test1.txt", "test4.txt\\.\\test1.txt\n"), "The file was not copied\n");
    }

    createTestFile("one.txt");

    /* pFrom contains bogus 2nd name longer than MAX_PATH */
    memset(from, 'a', MAX_PATH*2);
    memset(from+MAX_PATH*2, 0, 2);
    lstrcpyA(from, "one.txt");
    shfo.pFrom = from;
    shfo.pTo = "two.txt\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == 1148 || retval == 1026 ||
       retval == ERROR_ACCESS_DENIED || /* win2k */
       retval == DE_INVALIDFILES, /* Vista */
       "Unexpected return value, got %d\n", retval);
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    if (dir_exists("two.txt"))
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(RemoveDirectoryA("two.txt"), "Expected two.txt to exist\n");
    else
        ok(!DeleteFileA("two.txt"), "Expected file to not exist\n");

    createTestFile("one.txt");

    /* pTo contains bogus 2nd name longer than MAX_PATH */
    memset(to, 'a', MAX_PATH*2);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(to, "two.txt");
    shfo.pFrom = "one.txt\0";
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    if (retval == DE_OPCANCELLED)
    {
        /* NT4 fails and doesn't copy any files */
        ok(!file_exists("two.txt"), "Expected two.txt to not exist\n");
    }
    else
    {
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    }
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");

    createTestFile("one.txt");

    /* no FOF_MULTIDESTFILES, two files in pTo */
    shfo.pFrom = "one.txt\0";
    shfo.pTo = "two.txt\0three.txt\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    if (retval == DE_OPCANCELLED)
    {
        /* NT4 fails and doesn't copy any files */
        ok(!file_exists("two.txt"), "Expected two.txt to not exist\n");
    }
    else
    {
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    }
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");

    createTestFile("one.txt");

    /* both pFrom and pTo contain bogus 2nd names longer than MAX_PATH */
    memset(from, 'a', MAX_PATH*2);
    memset(from+MAX_PATH*2, 0, 2);
    memset(to, 'a', MAX_PATH*2);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(from, "one.txt");
    lstrcpyA(to, "two.txt");
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == 1148 || retval == 1026 ||
       retval == ERROR_ACCESS_DENIED ||  /* win2k */
       retval == DE_INVALIDFILES, /* Vista */
       "Unexpected return value, got %d\n", retval);
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    if (dir_exists("two.txt"))
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(RemoveDirectoryA("two.txt"), "Expected two.txt to exist\n");
    else
        ok(!DeleteFileA("two.txt"), "Expected file to not exist\n");

    createTestFile("one.txt");

    /* pTo contains bogus 2nd name longer than MAX_PATH, FOF_MULTIDESTFILES */
    memset(to, 'a', MAX_PATH*2);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(to, "two.txt");
    shfo.pFrom = "one.txt\0";
    shfo.pTo = to;
    shfo.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION |
                  FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    if (retval == DE_OPCANCELLED)
    {
        /* NT4 fails and doesn't copy any files */
        ok(!file_exists("two.txt"), "Expected two.txt to not exist\n");
    }
    else
    {
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    }
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");

    createTestFile("one.txt");
    createTestFile("two.txt");

    /* pTo contains bogus 2nd name longer than MAX_PATH,
     * multiple source files,
     * dest directory does not exist
     */
    memset(to, 'a', 2 * MAX_PATH);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(to, "threedir");
    shfo.pFrom = "one.txt\0two.txt\0";
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    if (dir_exists("threedir"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("threedir\\one.txt"), "Expected file to exist\n");
        ok(DeleteFileA("threedir\\two.txt"), "Expected file to exist\n");
        ok(RemoveDirectoryA("threedir"), "Expected dir to exist\n");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(!DeleteFileA("threedir\\one.txt"), "Expected file to not exist\n");
        ok(!DeleteFileA("threedir\\two.txt"), "Expected file to not exist\n");
        ok(!DeleteFileA("threedir"), "Expected file to not exist\n");
        ok(!RemoveDirectoryA("threedir"), "Expected dir to not exist\n");
    }
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");

    createTestFile("one.txt");
    createTestFile("two.txt");
    CreateDirectoryA("threedir", NULL);

    /* pTo contains bogus 2nd name longer than MAX_PATH,
     * multiple source files,
     * dest directory does exist
     */
    memset(to, 'a', 2 * MAX_PATH);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(to, "threedir");
    shfo.pFrom = "one.txt\0two.txt\0";
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(DeleteFileA("threedir\\one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("threedir\\two.txt"), "Expected file to exist\n");
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    ok(RemoveDirectoryA("threedir"), "Expected dir to exist\n");

    if (0) {
        /* this crashes on win9x */
        createTestFile("one.txt");
        createTestFile("two.txt");

        /* pTo contains bogus 2nd name longer than MAX_PATH,
         * multiple source files, FOF_MULTIDESTFILES
         * dest dir does not exist
         */

        memset(to, 'a', 2 * MAX_PATH);
        memset(to+MAX_PATH*2, 0, 2);
        lstrcpyA(to, "threedir");
        shfo.pFrom = "one.txt\0two.txt\0";
        shfo.pTo = to;
        shfo.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION |
                      FOF_SILENT | FOF_NOERRORUI;
        retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_CANCELLED ||
           retval == ERROR_SUCCESS, /* win2k3 */
           "Expected ERROR_CANCELLED or ERROR_SUCCESS, got %d\n", retval);
        ok(!DeleteFileA("threedir\\one.txt"), "Expected file to not exist\n");
        ok(!DeleteFileA("threedir\\two.txt"), "Expected file to not exist\n");
        ok(DeleteFileA("one.txt"), "Expected file to exist\n");
        ok(DeleteFileA("two.txt"), "Expected file to exist\n");
        ok(!RemoveDirectoryA("threedir"), "Expected dir to not exist\n");

        /* file exists in win2k */
        DeleteFileA("threedir");
    }


    createTestFile("one.txt");
    createTestFile("two.txt");
    CreateDirectoryA("threedir", NULL);

    /* pTo contains bogus 2nd name longer than MAX_PATH,
     * multiple source files, FOF_MULTIDESTFILES
     * dest dir does exist
     */
    memset(to, 'a', 2 * MAX_PATH);
    memset(to+MAX_PATH*2, 0, 2);
    lstrcpyA(to, "threedir");
    ptr = to + lstrlenA(to) + 1;
    lstrcpyA(ptr, "fourdir");
    shfo.pFrom = "one.txt\0two.txt\0";
    shfo.pTo = to;
    shfo.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION |
                  FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    ok(DeleteFileA("threedir\\one.txt"), "Expected file to exist\n");
    if (dir_exists("fourdir"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(!DeleteFileA("threedir\\two.txt"), "Expected file to not exist\n");
        ok(DeleteFileA("fourdir\\two.txt"), "Expected file to exist\n");
        RemoveDirectoryA("fourdir");
    }
    else
    {
        ok(DeleteFileA("threedir\\two.txt"), "Expected file to exist\n");
        ok(!DeleteFileA("fourdir"), "Expected file to not exist\n");
        ok(!RemoveDirectoryA("fourdir"), "Expected dir to not exist\n");
    }
    ok(RemoveDirectoryA("threedir"), "Expected dir to exist\n");

    createTestFile("one.txt");
    createTestFile("two.txt");
    CreateDirectoryA("threedir", NULL);

    /* multiple source files, FOF_MULTIDESTFILES
     * multiple dest files, but first dest dir exists
     * num files in lists is equal
     */
    shfo.pFrom = "one.txt\0two.txt\0";
    shfo.pTo = "threedir\0fourdir\0";
    shfo.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION |
                  FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_CANCELLED ||
       retval == DE_FILEDESTISFLD || /* Vista */
       broken(retval == DE_OPCANCELLED), /* Win9x, NT4 */
       "Expected ERROR_CANCELLED or DE_FILEDESTISFLD. got %d\n", retval);
    if (file_exists("threedir\\threedir"))
    {
        /* NT4 */
        ok(DeleteFileA("threedir\\threedir"), "Expected file to exist\n");
    }
    ok(!DeleteFileA("threedir\\one.txt"), "Expected file to not exist\n");
    ok(!DeleteFileA("threedir\\two.txt"), "Expected file to not exist\n");
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    ok(RemoveDirectoryA("threedir"), "Expected dir to exist\n");
    ok(!DeleteFileA("fourdir"), "Expected file to not exist\n");
    ok(!RemoveDirectoryA("fourdir"), "Expected dir to not exist\n");

    createTestFile("one.txt");
    createTestFile("two.txt");
    CreateDirectoryA("threedir", NULL);

    /* multiple source files, FOF_MULTIDESTFILES
     * multiple dest files, but first dest dir exists
     * num files in lists is not equal
     */
    shfo.pFrom = "one.txt\0two.txt\0";
    shfo.pTo = "threedir\0fourdir\0five\0";
    shfo.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION |
                  FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(DeleteFileA("one.txt"), "Expected file to exist\n");
    ok(DeleteFileA("two.txt"), "Expected file to exist\n");
    ok(DeleteFileA("threedir\\one.txt"), "Expected file to exist\n");
    if (dir_exists("fourdir"))
    {
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(!DeleteFileA("threedir\\two.txt"), "Expected file to not exist\n");
        ok(DeleteFileA("fourdir\\two.txt"), "Expected file to exist\n");
        RemoveDirectoryA("fourdir");
    }
    else
    {
        ok(DeleteFileA("threedir\\two.txt"), "Expected file to exist\n");
        ok(!DeleteFileA("fourdir"), "Expected file to not exist\n");
        ok(!RemoveDirectoryA("fourdir"), "Expected dir to not exist\n");
    }
    ok(RemoveDirectoryA("threedir"), "Expected dir to exist\n");
    ok(!DeleteFileA("five"), "Expected file to not exist\n");
    ok(!RemoveDirectoryA("five"), "Expected dir to not exist\n");

    createTestFile("aa.txt");
    createTestFile("ab.txt");
    CreateDirectoryA("one", NULL);
    CreateDirectoryA("two", NULL);

    /* pFrom has a glob, pTo has more than one dest */
    shfo.pFrom = "a*.txt\0";
    shfo.pTo = "one\0two\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(DeleteFileA("one\\aa.txt"), "Expected file to exist\n");
    ok(DeleteFileA("one\\ab.txt"), "Expected file to exist\n");
    ok(!DeleteFileA("two\\aa.txt"), "Expected file to not exist\n");
    ok(!DeleteFileA("two\\ab.txt"), "Expected file to not exist\n");
    ok(DeleteFileA("aa.txt"), "Expected file to exist\n");
    ok(DeleteFileA("ab.txt"), "Expected file to exist\n");
    ok(RemoveDirectoryA("one"), "Expected dir to exist\n");
    ok(RemoveDirectoryA("two"), "Expected dir to exist\n");

    /* pTo is an empty string  */
    CreateDirectoryA("dir", NULL);
    createTestFile("dir\\abcdefgh.abc");
    shfo.pFrom = "dir\\abcdefgh.abc\0";
    shfo.pTo = "\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS ||
       broken(retval == DE_OPCANCELLED), /* NT4 */
       "Expected ERROR_SUCCESS, got %d\n", retval);
    if (retval == ERROR_SUCCESS)
        ok(DeleteFileA("abcdefgh.abc"), "Expected file to exist\n");
    ok(DeleteFileA("dir\\abcdefgh.abc"), "Expected file to exist\n");
    ok(RemoveDirectoryA("dir"), "Expected dir to exist\n");
}

/* tests the FO_MOVE action */
static void test_move(void)
{
    SHFILEOPSTRUCTA shfo, shfo2;
    CHAR from[5*MAX_PATH];
    CHAR to[5*MAX_PATH];
    DWORD retval;

    clean_after_shfo_tests();
    init_shfo_tests();

    shfo.hwnd = NULL;
    shfo.wFunc = FO_MOVE;
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;
    shfo.fAnyOperationsAborted = FALSE;

    set_curr_dir_path(from, "testdir2\\*.*\0");
    set_curr_dir_path(to, "test4.txt\\*.*\0");
    retval = SHFileOperationA(&shfo);
    ok(retval != 0, "SHFileOperation should fail\n");
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(file_exists("testdir2"), "dir should not be moved\n");
    ok(file_exists("testdir2\\one.txt"), "file should not be moved\n");
    ok(file_exists("testdir2\\nested"), "dir should not be moved\n");
    ok(file_exists("testdir2\\nested\\two.txt"), "file should not be moved\n");

    set_curr_dir_path(from, "testdir2\\*.*\0");
    set_curr_dir_path(to, "test4.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(!retval, "SHFileOperation error %#x\n", retval);
    ok(!shfo.fAnyOperationsAborted, "fAnyOperationsAborted %d\n", shfo.fAnyOperationsAborted);

    ok(file_exists("testdir2"), "dir should not be moved\n");
    ok(!file_exists("testdir2\\one.txt"), "file should be moved\n");
    ok(!file_exists("testdir2\\nested"), "dir should be moved\n");
    ok(!file_exists("testdir2\\nested\\two.txt"), "file should be moved\n");

    ok(file_exists("test4.txt"), "dir should exist\n");
    ok(file_exists("test4.txt\\one.txt"), "file should exist\n");
    ok(file_exists("test4.txt\\nested"), "dir should exist\n");
    ok(file_exists("test4.txt\\nested\\two.txt"), "file should exist\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    shfo.hwnd = NULL;
    shfo.wFunc = FO_MOVE;
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    ok(!SHFileOperationA(&shfo), "Prepare test to check how directories are moved recursively\n");
    ok(!file_exists("test1.txt"), "test1.txt should not exist\n");
    ok(file_exists("test4.txt\\test1.txt"), "The file is not moved\n");

    set_curr_dir_path(from, "test?.txt\0");
    set_curr_dir_path(to, "testdir2\0");
    ok(!file_exists("testdir2\\test2.txt"), "The file is not moved yet\n");
    ok(!file_exists("testdir2\\test4.txt"), "The directory is not moved yet\n");
    ok(!SHFileOperationA(&shfo), "Files and directories are moved to directory\n");
    ok(file_exists("testdir2\\test2.txt"), "The file is moved\n");
    ok(file_exists("testdir2\\test4.txt"), "The directory is moved\n");
    ok(file_exists("testdir2\\test4.txt\\test1.txt"), "The file in subdirectory is moved\n");

    clean_after_shfo_tests();
    init_shfo_tests();

    memcpy(&shfo2, &shfo, sizeof(SHFILEOPSTRUCTA));
    shfo2.fFlags |= FOF_MULTIDESTFILES;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    if (old_shell32)
        shfo2.fFlags |= FOF_NOCONFIRMMKDIR;
    ok(!SHFileOperationA(&shfo2), "Move many files\n");
    ok(DeleteFileA("test6.txt"), "The file is not moved - many files are "
       "specified as a target\n");
    ok(DeleteFileA("test7.txt"), "The file is not moved\n");
    ok(RemoveDirectoryA("test8.txt"), "The directory is not moved\n");

    init_shfo_tests();

    /* number of sources does not correspond to number of targets,
       include directories */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    retval = SHFileOperationA(&shfo2);
    if (dir_exists("test6.txt"))
    {
        if (retval == ERROR_SUCCESS)
        {
            /* Old shell32 */
            DeleteFileA("test6.txt\\test1.txt");
            DeleteFileA("test6.txt\\test2.txt");
            RemoveDirectoryA("test6.txt\\test4.txt");
            RemoveDirectoryA("test6.txt");
        }
        else
        {
            /* Vista and W2K8 (broken or new behavior ?) */
            ok(retval == DE_DESTSAMETREE, "Expected DE_DESTSAMETREE, got %d\n", retval);
            ok(DeleteFileA("test6.txt\\test1.txt"), "The file is not moved\n");
            RemoveDirectoryA("test6.txt");
            ok(DeleteFileA("test7.txt\\test2.txt"), "The file is not moved\n");
            RemoveDirectoryA("test7.txt");
        }
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(!file_exists("test6.txt"), "The file is not moved - many files are "
           "specified as a target\n");
    }

    init_shfo_tests();
    /* number of sources does not correspond to number of targets,
       files only,
       from exceeds to */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test3.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    retval = SHFileOperationA(&shfo2);
    if (dir_exists("test6.txt"))
    {
        if (retval == ERROR_SUCCESS)
        {
            /* Old shell32 */
            DeleteFileA("test6.txt\\test1.txt");
            DeleteFileA("test6.txt\\test2.txt");
            RemoveDirectoryA("test6.txt\\test4.txt");
            RemoveDirectoryA("test6.txt");
        }
        else
        {
            /* Vista and W2K8 (broken or new behavior ?) */
            ok(retval == DE_SAMEFILE, "Expected DE_SAMEFILE, got %d\n", retval);
            ok(DeleteFileA("test6.txt\\test1.txt"), "The file is not moved\n");
            RemoveDirectoryA("test6.txt");
            ok(DeleteFileA("test7.txt\\test2.txt"), "The file is not moved\n");
            RemoveDirectoryA("test7.txt");
            ok(file_exists("test3.txt"), "File should not be moved\n");
        }
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(!file_exists("test6.txt"), "The file is not moved - many files are "
           "specified as a target\n");
    }

    init_shfo_tests();
    /* number of sources does not correspond to number of targets,
       files only,
       too exceeds from */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    retval = SHFileOperationA(&shfo2);
    if (dir_exists("test6.txt"))
    {
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("test6.txt\\test1.txt"),"The file is not moved\n");
        ok(DeleteFileA("test7.txt\\test2.txt"),"The file is not moved\n");
        ok(!dir_exists("test8.txt") && !file_exists("test8.txt"),
            "Directory should not be created\n");
        RemoveDirectoryA("test6.txt");
        RemoveDirectoryA("test7.txt");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* WinXp, Win2k */);
        ok(!file_exists("test6.txt"), "The file is not moved - many files are "
           "specified as a target\n");
    }

    init_shfo_tests();
    /* number of sources does not correspond to number of targets,
       target directories */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test3.txt\0");
    set_curr_dir_path(to, "test4.txt\0test5.txt\0");
    retval = SHFileOperationA(&shfo2);
    if (dir_exists("test5.txt"))
    {
        ok(retval == DE_SAMEFILE, "Expected DE_SAMEFILE, got %d\n", retval);
        ok(DeleteFileA("test4.txt\\test1.txt"),"The file is not moved\n");
        ok(DeleteFileA("test5.txt\\test2.txt"),"The file is not moved\n");
        ok(file_exists("test3.txt"), "The file is not moved\n");
        RemoveDirectoryA("test4.txt");
        RemoveDirectoryA("test5.txt");
    }
    else
    {
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("test4.txt\\test1.txt"),"The file is not moved\n");
        ok(DeleteFileA("test4.txt\\test2.txt"),"The file is not moved\n");
        ok(DeleteFileA("test4.txt\\test3.txt"),"The file is not moved\n");
    }


    init_shfo_tests();
    /*  0 incoming files */
    set_curr_dir_path(from, "\0\0");
    set_curr_dir_path(to, "test6.txt\0\0");
    retval = SHFileOperationA(&shfo2);
    ok(retval == ERROR_SUCCESS || retval == ERROR_ACCESS_DENIED
        , "Expected ERROR_SUCCESS || ERROR_ACCESS_DENIED, got %d\n", retval);
    ok(!file_exists("test6.txt"), "The file should not exist\n");

    init_shfo_tests();
    /*  0 outgoing files */
    set_curr_dir_path(from, "test1\0\0");
    set_curr_dir_path(to, "\0\0");
    retval = SHFileOperationA(&shfo2);
    ok(retval == ERROR_FILE_NOT_FOUND ||
        broken(retval == 1026)
        , "Expected ERROR_FILE_NOT_FOUND, got %d\n", retval);
    ok(!file_exists("test6.txt"), "The file should not exist\n");

    init_shfo_tests();

    set_curr_dir_path(from, "test3.txt\0");
    set_curr_dir_path(to, "test4.txt\\test1.txt\0");
    ok(!SHFileOperationA(&shfo), "Can't move file to other directory\n");
    ok(file_exists("test4.txt\\test1.txt"), "The file is not moved\n");

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    if (old_shell32)
        shfo.fFlags |= FOF_NOCONFIRMMKDIR;
    retval = SHFileOperationA(&shfo);
    if (dir_exists("test6.txt"))
    {
        /* Old shell32 */
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("test6.txt\\test1.txt"), "The file is not moved. Many files are specified\n");
        ok(DeleteFileA("test6.txt\\test2.txt"), "The file is not moved. Many files are specified\n");
        ok(DeleteFileA("test6.txt\\test4.txt\\test1.txt"), "The file is not moved. Many files are specified\n");
        ok(RemoveDirectoryA("test6.txt\\test4.txt"), "The directory is not moved. Many files are specified\n");
        RemoveDirectoryA("test6.txt");
        init_shfo_tests();
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(file_exists("test1.txt"), "The file is moved. Many files are specified\n");
        ok(dir_exists("test4.txt"), "The directory is moved. Many files are specified\n");
    }

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    ok(!SHFileOperationA(&shfo), "Move file failed\n");
    ok(!file_exists("test1.txt"), "The file is not moved\n");
    ok(file_exists("test6.txt"), "The file is not moved\n");
    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test1.txt\0");
    ok(!SHFileOperationA(&shfo), "Move file back failed\n");

    set_curr_dir_path(from, "test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    ok(!SHFileOperationA(&shfo), "Move dir failed\n");
    ok(!dir_exists("test4.txt"), "The dir is not moved\n");
    ok(dir_exists("test6.txt"), "The dir is moved\n");
    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    ok(!SHFileOperationA(&shfo), "Move dir back failed\n");

    /* move one file to two others */
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "a.txt\0b.txt\0";
    retval = SHFileOperationA(&shfo);
    if (retval == DE_OPCANCELLED)
    {
        /* NT4 fails and doesn't move any files */
        ok(!file_exists("a.txt"), "Expected a.txt to not exist\n");
        DeleteFileA("test1.txt");
    }
    else
    {
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        if (old_shell32)
        {
            DeleteFileA("a.txt\\a.txt");
            RemoveDirectoryA("a.txt");
        }
        else
            ok(DeleteFileA("a.txt"), "Expected a.txt to exist\n");
        ok(!file_exists("test1.txt"), "Expected test1.txt to not exist\n");
    }
    ok(!file_exists("b.txt"), "Expected b.txt to not exist\n");

    /* move two files to one other */
    shfo.pFrom = "test2.txt\0test3.txt\0";
    shfo.pTo = "test1.txt\0";
    retval = SHFileOperationA(&shfo);
    if (dir_exists("test1.txt"))
    {
        /* Old shell32 */
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("test1.txt\\test2.txt"), "Expected test1.txt\\test2.txt to exist\n");
        ok(DeleteFileA("test1.txt\\test3.txt"), "Expected test1.txt\\test3.txt to exist\n");
        RemoveDirectoryA("test1.txt");
        createTestFile("test2.txt");
        createTestFile("test3.txt");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(!file_exists("test1.txt"), "Expected test1.txt to not exist\n");
        ok(file_exists("test2.txt"), "Expected test2.txt to exist\n");
        ok(file_exists("test3.txt"), "Expected test3.txt to exist\n");
    }

    /* move a directory into itself */
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "test4.txt\\b.txt\0";
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_SUCCESS ||
       retval == DE_DESTSUBTREE, /* Vista */
       "Expected ERROR_SUCCESS or DE_DESTSUBTREE, got %d\n", retval);
    ok(!RemoveDirectoryA("test4.txt\\b.txt"), "Expected test4.txt\\b.txt to not exist\n");
    ok(dir_exists("test4.txt"), "Expected test4.txt to exist\n");

    /* move many files without FOF_MULTIDESTFILES */
    shfo.pFrom = "test2.txt\0test3.txt\0";
    shfo.pTo = "d.txt\0e.txt\0";
    retval = SHFileOperationA(&shfo);
    if (dir_exists("d.txt"))
    {
        /* Old shell32 */
        /* Vista and W2K8 (broken or new behavior ?) */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("d.txt\\test2.txt"), "Expected d.txt\\test2.txt to exist\n");
        ok(DeleteFileA("d.txt\\test3.txt"), "Expected d.txt\\test3.txt to exist\n");
        RemoveDirectoryA("d.txt");
        createTestFile("test2.txt");
        createTestFile("test3.txt");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(!DeleteFileA("d.txt"), "Expected d.txt to not exist\n");
        ok(!DeleteFileA("e.txt"), "Expected e.txt to not exist\n");
    }

    /* number of sources != number of targets */
    shfo.pTo = "d.txt\0";
    shfo.fFlags |= FOF_MULTIDESTFILES;
    retval = SHFileOperationA(&shfo);
    if (dir_exists("d.txt"))
    {
        if (old_shell32)
        {
            DeleteFileA("d.txt\\test2.txt");
            DeleteFileA("d.txt\\test3.txt");
            RemoveDirectoryA("d.txt");
            createTestFile("test2.txt");
        }
        else
        {
            /* Vista and W2K8 (broken or new behavior ?) */
            ok(retval == DE_SAMEFILE,
               "Expected DE_SAMEFILE, got %d\n", retval);
            ok(DeleteFileA("d.txt\\test2.txt"), "Expected d.txt\\test2.txt to exist\n");
            ok(!file_exists("d.txt\\test3.txt"), "Expected d.txt\\test3.txt to not exist\n");
            RemoveDirectoryA("d.txt");
            createTestFile("test2.txt");
        }
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
        ok(!DeleteFileA("d.txt"), "Expected d.txt to not exist\n");
    }

    /* FO_MOVE should create dest directories */
    shfo.pFrom = "test2.txt\0";
    shfo.pTo = "dir1\\dir2\\test2.txt\0";
    retval = SHFileOperationA(&shfo);
    if (dir_exists("dir1"))
    {
        /* New behavior on Vista or later */
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFileA("dir1\\dir2\\test2.txt"), "Expected dir1\\dir2\\test2.txt to exist\n");
        RemoveDirectoryA("dir1\\dir2");
        RemoveDirectoryA("dir1");
        createTestFile("test2.txt");
    }
    else
    {
        expect_retval(ERROR_CANCELLED, DE_OPCANCELLED /* Win9x, NT4 */);
    }

    /* try to overwrite an existing file */
    shfo.pTo = "test3.txt\0";
    retval = SHFileOperationA(&shfo);
    if (retval == DE_OPCANCELLED)
    {
        /* NT4 fails and doesn't move any files */
        ok(file_exists("test2.txt"), "Expected test2.txt to exist\n");
    }
    else
    {
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(!file_exists("test2.txt"), "Expected test2.txt to not exist\n");
        if (old_shell32)
        {
            DeleteFileA("test3.txt\\test3.txt");
            RemoveDirectoryA("test3.txt");
        }
        else
            ok(file_exists("test3.txt"), "Expected test3.txt to exist\n");
    }
}

static void test_sh_create_dir(void)
{
    CHAR path[MAX_PATH];
    int ret;

    if(!pSHCreateDirectoryExA)
    {
        win_skip("skipping SHCreateDirectoryExA tests\n");
        return;
    }

    set_curr_dir_path(path, "testdir2\\test4.txt\0");
    ret = pSHCreateDirectoryExA(NULL, path, NULL);
    ok(ERROR_SUCCESS == ret, "SHCreateDirectoryEx failed to create directory recursively, ret = %d\n", ret);
    ok(file_exists("testdir2"), "The first directory is not created\n");
    ok(file_exists("testdir2\\test4.txt"), "The second directory is not created\n");

    ret = pSHCreateDirectoryExA(NULL, path, NULL);
    ok(ERROR_ALREADY_EXISTS == ret, "SHCreateDirectoryEx should fail to create existing directory, ret = %d\n", ret);

    ret = pSHCreateDirectoryExA(NULL, "c:\\testdir3", NULL);
    ok(ERROR_SUCCESS == ret, "SHCreateDirectoryEx failed to create directory, ret = %d\n", ret);
    ok(file_exists("c:\\testdir3"), "The directory is not created\n");
}

static void test_sh_path_prepare(void)
{
    HRESULT res;
    CHAR path[MAX_PATH];
    CHAR UNICODE_PATH_A[MAX_PATH];
    BOOL UsedDefaultChar;

    if(!pSHPathPrepareForWriteA)
    {
	win_skip("skipping SHPathPrepareForWriteA tests\n");
	return;
    }

    /* directory exists, SHPPFW_NONE */
    set_curr_dir_path(path, "testdir2\0");
    res = pSHPathPrepareForWriteA(0, 0, path, SHPPFW_NONE);
    ok(res == S_OK, "res == 0x%08x, expected S_OK\n", res);

    /* directory exists, SHPPFW_IGNOREFILENAME */
    set_curr_dir_path(path, "testdir2\\test4.txt\0");
    res = pSHPathPrepareForWriteA(0, 0, path, SHPPFW_IGNOREFILENAME);
    ok(res == S_OK, "res == 0x%08x, expected S_OK\n", res);

    /* directory exists, SHPPFW_DIRCREATE */
    set_curr_dir_path(path, "testdir2\0");
    res = pSHPathPrepareForWriteA(0, 0, path, SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == 0x%08x, expected S_OK\n", res);

    /* directory exists, SHPPFW_IGNOREFILENAME|SHPPFW_DIRCREATE */
    set_curr_dir_path(path, "testdir2\\test4.txt\0");
    res = pSHPathPrepareForWriteA(0, 0, path, SHPPFW_IGNOREFILENAME|SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == 0x%08x, expected S_OK\n", res);
    ok(!file_exists("nonexistent\\"), "nonexistent\\ exists but shouldn't\n");

    /* file exists, SHPPFW_NONE */
    set_curr_dir_path(path, "test1.txt\0");
    res = pSHPathPrepareForWriteA(0, 0, path, SHPPFW_NONE);
    ok(res == HRESULT_FROM_WIN32(ERROR_DIRECTORY) ||
       res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || /* WinMe */
       res == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), /* Vista */
       "Unexpected result : 0x%08x\n", res);

    /* file exists, SHPPFW_DIRCREATE */
    res = pSHPathPrepareForWriteA(0, 0, path, SHPPFW_DIRCREATE);
    ok(res == HRESULT_FROM_WIN32(ERROR_DIRECTORY) ||
       res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || /* WinMe */
       res == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), /* Vista */
       "Unexpected result : 0x%08x\n", res);

    /* file exists, SHPPFW_NONE, trailing \ */
    set_curr_dir_path(path, "test1.txt\\\0");
    res = pSHPathPrepareForWriteA(0, 0, path, SHPPFW_NONE);
    ok(res == HRESULT_FROM_WIN32(ERROR_DIRECTORY) ||
       res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || /* WinMe */
       res == HRESULT_FROM_WIN32(ERROR_INVALID_NAME), /* Vista */
       "Unexpected result : 0x%08x\n", res);

    /* relative path exists, SHPPFW_DIRCREATE */
    res = pSHPathPrepareForWriteA(0, 0, ".\\testdir2", SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == 0x%08x, expected S_OK\n", res);

    /* relative path doesn't exist, SHPPFW_DIRCREATE -- Windows does not create the directory in this case */
    res = pSHPathPrepareForWriteA(0, 0, ".\\testdir2\\test4.txt", SHPPFW_DIRCREATE);
    ok(res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "res == 0x%08x, expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)\n", res);
    ok(!file_exists(".\\testdir2\\test4.txt\\"), ".\\testdir2\\test4.txt\\ exists but shouldn't\n");

    /* directory doesn't exist, SHPPFW_NONE */
    set_curr_dir_path(path, "nonexistent\0");
    res = pSHPathPrepareForWriteA(0, 0, path, SHPPFW_NONE);
    ok(res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "res == 0x%08x, expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)\n", res);

    /* directory doesn't exist, SHPPFW_IGNOREFILENAME */
    set_curr_dir_path(path, "nonexistent\\notreal\0");
    res = pSHPathPrepareForWriteA(0, 0, path, SHPPFW_IGNOREFILENAME);
    ok(res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "res == 0x%08x, expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)\n", res);
    ok(!file_exists("nonexistent\\notreal"), "nonexistent\\notreal exists but shouldn't\n");
    ok(!file_exists("nonexistent\\"), "nonexistent\\ exists but shouldn't\n");

    /* directory doesn't exist, SHPPFW_IGNOREFILENAME|SHPPFW_DIRCREATE */
    set_curr_dir_path(path, "testdir2\\test4.txt\\\0");
    res = pSHPathPrepareForWriteA(0, 0, path, SHPPFW_IGNOREFILENAME|SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == 0x%08x, expected S_OK\n", res);
    ok(file_exists("testdir2\\test4.txt\\"), "testdir2\\test4.txt doesn't exist but should\n");

    /* nested directory doesn't exist, SHPPFW_DIRCREATE */
    set_curr_dir_path(path, "nonexistent\\notreal\0");
    res = pSHPathPrepareForWriteA(0, 0, path, SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == 0x%08x, expected S_OK\n", res);
    ok(file_exists("nonexistent\\notreal"), "nonexistent\\notreal doesn't exist but should\n");

    /* SHPPFW_ASKDIRCREATE, SHPPFW_NOWRITECHECK, and SHPPFW_MEDIACHECKONLY are untested */

    if(!pSHPathPrepareForWriteW)
    {
        win_skip("Skipping SHPathPrepareForWriteW tests\n");
        return;
    }

    SetLastError(0xdeadbeef);
    UsedDefaultChar = FALSE;
    if (WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, UNICODE_PATH, -1, UNICODE_PATH_A, sizeof(UNICODE_PATH_A), NULL, &UsedDefaultChar) == 0)
    {
        win_skip("Could not convert Unicode path name to multibyte (%d)\n", GetLastError());
        return;
    }
    if (UsedDefaultChar)
    {
        win_skip("Could not find unique multibyte representation for directory name using default codepage\n");
        return;
    }

    /* unicode directory doesn't exist, SHPPFW_NONE */
    RemoveDirectoryA(UNICODE_PATH_A);
    res = pSHPathPrepareForWriteW(0, 0, UNICODE_PATH, SHPPFW_NONE);
    ok(res == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "res == %08x, expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)\n", res);
    ok(!file_exists(UNICODE_PATH_A), "unicode path was created but shouldn't be\n");
    RemoveDirectoryA(UNICODE_PATH_A);

    /* unicode directory doesn't exist, SHPPFW_DIRCREATE */
    res = pSHPathPrepareForWriteW(0, 0, UNICODE_PATH, SHPPFW_DIRCREATE);
    ok(res == S_OK, "res == %08x, expected S_OK\n", res);
    ok(file_exists(UNICODE_PATH_A), "unicode path should've been created\n");

    /* unicode directory exists, SHPPFW_NONE */
    res = pSHPathPrepareForWriteW(0, 0, UNICODE_PATH, SHPPFW_NONE);
    ok(res == S_OK, "ret == %08x, expected S_OK\n", res);

    /* unicode directory exists, SHPPFW_DIRCREATE */
    res = pSHPathPrepareForWriteW(0, 0, UNICODE_PATH, SHPPFW_DIRCREATE);
    ok(res == S_OK, "ret == %08x, expected S_OK\n", res);
    RemoveDirectoryA(UNICODE_PATH_A);
}

static void test_sh_new_link_info(void)
{
    BOOL ret, mustcopy=TRUE;
    CHAR linkto[MAX_PATH];
    CHAR destdir[MAX_PATH];
    CHAR result[MAX_PATH];
    CHAR result2[MAX_PATH];

    /* source file does not exist */
    set_curr_dir_path(linkto, "nosuchfile.txt\0");
    set_curr_dir_path(destdir, "testdir2\0");
    ret = SHGetNewLinkInfoA(linkto, destdir, result, &mustcopy, 0);
    ok(ret == FALSE ||
       broken(ret == lstrlenA(result) + 1), /* NT4 */
       "SHGetNewLinkInfoA succeeded\n");
    ok(mustcopy == FALSE, "mustcopy should be FALSE\n");

    /* dest dir does not exist */
    set_curr_dir_path(linkto, "test1.txt\0");
    set_curr_dir_path(destdir, "nosuchdir\0");
    ret = SHGetNewLinkInfoA(linkto, destdir, result, &mustcopy, 0);
    ok(ret == TRUE ||
       broken(ret == lstrlenA(result) + 1), /* NT4 */
       "SHGetNewLinkInfoA failed, err=%i\n", GetLastError());
    ok(mustcopy == FALSE, "mustcopy should be FALSE\n");

    /* source file exists */
    set_curr_dir_path(linkto, "test1.txt\0");
    set_curr_dir_path(destdir, "testdir2\0");
    ret = SHGetNewLinkInfoA(linkto, destdir, result, &mustcopy, 0);
    ok(ret == TRUE ||
       broken(ret == lstrlenA(result) + 1), /* NT4 */
       "SHGetNewLinkInfoA failed, err=%i\n", GetLastError());
    ok(mustcopy == FALSE, "mustcopy should be FALSE\n");
    ok(CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, destdir,
                      lstrlenA(destdir), result, lstrlenA(destdir)) == CSTR_EQUAL,
       "%s does not start with %s\n", result, destdir);
    ok(lstrlenA(result) > 4 && lstrcmpiA(result+lstrlenA(result)-4, ".lnk") == 0,
       "%s does not end with .lnk\n", result);

    /* preferred target name already exists */
    createTestFile(result);
    ret = SHGetNewLinkInfoA(linkto, destdir, result2, &mustcopy, 0);
    ok(ret == TRUE ||
       broken(ret == lstrlenA(result2) + 1), /* NT4 */
       "SHGetNewLinkInfoA failed, err=%i\n", GetLastError());
    ok(mustcopy == FALSE, "mustcopy should be FALSE\n");
    ok(CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, destdir,
                      lstrlenA(destdir), result2, lstrlenA(destdir)) == CSTR_EQUAL,
       "%s does not start with %s\n", result2, destdir);
    ok(lstrlenA(result2) > 4 && lstrcmpiA(result2+lstrlenA(result2)-4, ".lnk") == 0,
       "%s does not end with .lnk\n", result2);
    ok(lstrcmpiA(result, result2) != 0, "%s and %s are the same\n", result, result2);
    DeleteFileA(result);
}

static void test_unicode(void)
{
    SHFILEOPSTRUCTW shfoW;
    int ret;
    HANDLE file;

    if (!pSHFileOperationW)
    {
        skip("SHFileOperationW() is missing\n");
        return;
    }

    shfoW.hwnd = NULL;
    shfoW.wFunc = FO_DELETE;
    shfoW.pFrom = UNICODE_PATH;
    shfoW.pTo = NULL;
    shfoW.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    shfoW.hNameMappings = NULL;
    shfoW.lpszProgressTitle = NULL;

    /* Clean up before start test */
    DeleteFileW(UNICODE_PATH);
    RemoveDirectoryW(UNICODE_PATH);

    /* Make sure we are on a system that supports unicode */
    SetLastError(0xdeadbeef);
    file = CreateFileW(UNICODE_PATH, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("Unicode tests skipped on non-unicode system\n");
        return;
    }
    CloseHandle(file);

    /* Try to delete a file with unicode filename */
    ok(file_existsW(UNICODE_PATH), "The file does not exist\n");
    ret = pSHFileOperationW(&shfoW);
    ok(!ret, "File is not removed, ErrorCode: %d\n", ret);
    ok(!file_existsW(UNICODE_PATH), "The file should have been removed\n");

    /* Try to trash a file with unicode filename */
    createTestFileW(UNICODE_PATH);
    shfoW.fFlags |= FOF_ALLOWUNDO;
    ok(file_existsW(UNICODE_PATH), "The file does not exist\n");
    ret = pSHFileOperationW(&shfoW);
    ok(!ret, "File is not removed, ErrorCode: %d\n", ret);
    ok(!file_existsW(UNICODE_PATH), "The file should have been removed\n");

    if(!pSHCreateDirectoryExW)
    {
        skip("Skipping SHCreateDirectoryExW tests\n");
        return;
    }

    /* Try to delete a directory with unicode filename */
    ret = pSHCreateDirectoryExW(NULL, UNICODE_PATH, NULL);
    ok(!ret, "SHCreateDirectoryExW returned %d\n", ret);
    ok(file_existsW(UNICODE_PATH), "The directory is not created\n");
    shfoW.fFlags &= ~FOF_ALLOWUNDO;
    ret = pSHFileOperationW(&shfoW);
    ok(!ret, "Directory is not removed, ErrorCode: %d\n", ret);
    ok(!file_existsW(UNICODE_PATH), "The directory should have been removed\n");

    /* Try to trash a directory with unicode filename */
    ret = pSHCreateDirectoryExW(NULL, UNICODE_PATH, NULL);
    ok(!ret, "SHCreateDirectoryExW returned %d\n", ret);
    ok(file_existsW(UNICODE_PATH), "The directory was not created\n");
    shfoW.fFlags |= FOF_ALLOWUNDO;
    ret = pSHFileOperationW(&shfoW);
    ok(!ret, "Directory is not removed, ErrorCode: %d\n", ret);
    ok(!file_existsW(UNICODE_PATH), "The directory should have been removed\n");
}

static void
test_shlmenu(void) {
	HRESULT hres;
	hres = Shell_MergeMenus (0, 0, 0x42, 0x4242, 0x424242, 0);
	ok (hres == 0x4242, "expected 0x4242 but got %x\n", hres);
	hres = Shell_MergeMenus ((HMENU)42, 0, 0x42, 0x4242, 0x424242, 0);
	ok (hres == 0x4242, "expected 0x4242 but got %x\n", hres);
}

/* Check for old shell32 (4.0.x) */
static BOOL is_old_shell32(void)
{
    SHFILEOPSTRUCTA shfo;
    CHAR from[5*MAX_PATH];
    CHAR to[5*MAX_PATH];
    DWORD retval;

    shfo.hwnd = NULL;
    shfo.wFunc = FO_COPY;
    shfo.pFrom = from;
    shfo.pTo = to;
    /* FOF_NOCONFIRMMKDIR is needed for old shell32 */
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI | FOF_MULTIDESTFILES | FOF_NOCONFIRMMKDIR;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test3.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    retval = SHFileOperationA(&shfo);

    /* Delete extra files on old shell32 and Vista+*/
    DeleteFileA("test6.txt\\test1.txt");
    /* Delete extra files on old shell32 */
    DeleteFileA("test6.txt\\test2.txt");
    DeleteFileA("test6.txt\\test3.txt");
    /* Delete extra directory on old shell32 and Vista+ */
    RemoveDirectoryA("test6.txt");
    /* Delete extra files/directories on Vista+*/
    DeleteFileA("test7.txt\\test2.txt");
    RemoveDirectoryA("test7.txt");

    if (retval == ERROR_SUCCESS)
        return TRUE;

    return FALSE;
}

START_TEST(shlfileop)
{
    InitFunctionPointers();

    clean_after_shfo_tests();

    init_shfo_tests();
    old_shell32 = is_old_shell32();
    if (old_shell32)
        win_skip("Need to cater for old shell32 (4.0.x) on Win95\n");
    clean_after_shfo_tests();

    init_shfo_tests();
    test_get_file_info();
    test_get_file_info_iconlist();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_delete();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_rename();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_copy();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_move();
    clean_after_shfo_tests();

    test_sh_create_dir();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_sh_path_prepare();
    clean_after_shfo_tests();

    init_shfo_tests();
    test_sh_new_link_info();
    clean_after_shfo_tests();

    test_unicode();

    test_shlmenu();
}
