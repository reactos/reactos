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

static CHAR CURR_DIR[MAX_PATH];
static const WCHAR UNICODE_PATH[] = {'c',':','\\',0x00c4,'\0','\0'};
    /* "c:\Ä", or "c:\A" with diaeresis */
    /* Double-null termination needed for pFrom field of SHFILEOPSTRUCT */

static HMODULE hshell32;
static int (WINAPI *pSHCreateDirectoryExA)(HWND, LPCSTR, LPSECURITY_ATTRIBUTES);
static int (WINAPI *pSHCreateDirectoryExW)(HWND, LPCWSTR, LPSECURITY_ATTRIBUTES);

static void InitFunctionPointers(void)
{
    hshell32 = GetModuleHandleA("shell32.dll");
    pSHCreateDirectoryExA = (void*)GetProcAddress(hshell32, "SHCreateDirectoryExA");
    pSHCreateDirectoryExW = (void*)GetProcAddress(hshell32, "SHCreateDirectoryExW");
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
    SHFILEINFO shfi, shfi2;
    char notepad[MAX_PATH];

    /* Test some flag combinations that MSDN claims are not allowed,
     * but which work anyway
     */
    shfi.dwAttributes=0xdeadbeef;
    rc=SHGetFileInfoA("c:\\nonexistent", FILE_ATTRIBUTE_DIRECTORY,
                      &shfi, sizeof(shfi),
                      SHGFI_ATTRIBUTES | SHGFI_USEFILEATTRIBUTES);
    todo_wine ok(rc, "SHGetFileInfoA(c:\\nonexistent | SHGFI_ATTRIBUTES) failed\n");
    if (rc)
        ok(shfi.dwAttributes != 0xdeadbeef, "dwFileAttributes is not set\n");

    rc=SHGetFileInfoA("c:\\nonexistent", FILE_ATTRIBUTE_DIRECTORY,
                      &shfi, sizeof(shfi),
                      SHGFI_EXETYPE | SHGFI_USEFILEATTRIBUTES);
    todo_wine ok(rc == 1, "SHGetFileInfoA(c:\\nonexistent | SHGFI_EXETYPE) returned %d\n", rc);

    /* Test SHGFI_USEFILEATTRIBUTES support */
    strcpy(shfi.szDisplayName, "dummy");
    shfi.iIcon=0xdeadbeef;
    rc=SHGetFileInfoA("c:\\nonexistent", FILE_ATTRIBUTE_DIRECTORY,
                      &shfi, sizeof(shfi),
                      SHGFI_ICONLOCATION | SHGFI_USEFILEATTRIBUTES);
    ok(rc, "SHGetFileInfoA(c:\\nonexistent) failed\n");
    if (rc)
    {
        ok(strcpy(shfi.szDisplayName, "dummy") != 0, "SHGetFileInfoA(c:\\nonexistent) displayname is not set\n");
        ok(shfi.iIcon != 0xdeadbeef, "SHGetFileInfoA(c:\\nonexistent) iIcon is not set\n");
    }

    /* Wine does not have a default icon for text files, and Windows 98 fails
     * if we give it an empty executable. So use notepad.exe as the test
     */
    if (SearchPath(NULL, "notepad.exe", NULL, sizeof(notepad), notepad, NULL))
    {
        strcpy(shfi.szDisplayName, "dummy");
        shfi.iIcon=0xdeadbeef;
        rc=SHGetFileInfoA(notepad, GetFileAttributes(notepad),
                          &shfi, sizeof(shfi),
                          SHGFI_ICONLOCATION | SHGFI_USEFILEATTRIBUTES);
        ok(rc, "SHGetFileInfoA(%s, SHGFI_USEFILEATTRIBUTES) failed\n", notepad);
        strcpy(shfi2.szDisplayName, "dummy");
        shfi2.iIcon=0xdeadbeef;
        rc2=SHGetFileInfoA(notepad, 0,
                           &shfi2, sizeof(shfi2),
                           SHGFI_ICONLOCATION);
        ok(rc2, "SHGetFileInfoA(%s) failed\n", notepad);
        if (rc && rc2)
        {
            ok(lstrcmpi(shfi2.szDisplayName, shfi.szDisplayName) == 0, "wrong display name %s != %s\n", shfi.szDisplayName, shfi2.szDisplayName);
            ok(shfi2.iIcon == shfi.iIcon, "wrong icon index %d != %d\n", shfi.iIcon, shfi2.iIcon);
        }
    }

    /* with a directory now */
    strcpy(shfi.szDisplayName, "dummy");
    shfi.iIcon=0xdeadbeef;
    rc=SHGetFileInfoA("test4.txt", GetFileAttributes("test4.txt"),
                      &shfi, sizeof(shfi),
                      SHGFI_ICONLOCATION | SHGFI_USEFILEATTRIBUTES);
    ok(rc, "SHGetFileInfoA(test4.txt/, SHGFI_USEFILEATTRIBUTES) failed\n");
    strcpy(shfi2.szDisplayName, "dummy");
    shfi2.iIcon=0xdeadbeef;
    rc2=SHGetFileInfoA("test4.txt", 0,
                      &shfi2, sizeof(shfi2),
                      SHGFI_ICONLOCATION);
    ok(rc2, "SHGetFileInfoA(test4.txt/) failed\n");
    if (rc && rc2)
    {
        ok(lstrcmpi(shfi2.szDisplayName, shfi.szDisplayName) == 0, "wrong display name %s != %s\n", shfi.szDisplayName, shfi2.szDisplayName);
        ok(shfi2.iIcon == shfi.iIcon, "wrong icon index %d != %d\n", shfi.iIcon, shfi2.iIcon);
    }
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
    shfo.pTo = "\0";
    shfo.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_SILENT;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    ok(!SHFileOperationA(&shfo), "Deletion was not successful\n");
    ok(file_exists("test4.txt"), "Directory should not have been removed\n");
    ok(!file_exists("test1.txt"), "File should have been removed\n");

    ret = SHFileOperationA(&shfo);
    ok(!ret, "Directory exists, but is not removed, ret=%d\n", ret);
    ok(file_exists("test4.txt"), "Directory should not have been removed\n");

    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;

    ok(!SHFileOperationA(&shfo), "Directory is not removed\n");
    ok(!file_exists("test4.txt"), "Directory should have been removed\n");

    ret = SHFileOperationA(&shfo);
    ok(!ret, "The requested file does not exist, ret=%d\n", ret);

    init_shfo_tests();
    sprintf(buf, "%s\\%s", CURR_DIR, "test4.txt");
    buf[strlen(buf) + 1] = '\0';
    ok(MoveFileA("test1.txt", "test4.txt\\test1.txt"), "Filling the subdirectory failed\n");
    ok(!SHFileOperationA(&shfo), "Directory is not removed\n");
    ok(!file_exists("test4.txt"), "Directory is not removed\n");

    init_shfo_tests();
    shfo.pFrom = "test1.txt\0test4.txt\0";
    ok(!SHFileOperationA(&shfo), "Directory and a file are not removed\n");
    ok(!file_exists("test1.txt"), "The file should have been removed\n");
    ok(!file_exists("test4.txt"), "Directory should have been removed\n");
    ok(file_exists("test2.txt"), "This file should not have been removed\n");

    /* FOF_FILESONLY does not delete a dir matching a wildcard */
    init_shfo_tests();
    shfo.fFlags |= FOF_FILESONLY;
    shfo.pFrom = "*.txt\0";
    ok(!SHFileOperation(&shfo), "Failed to delete files\n");
    ok(!file_exists("test1.txt"), "test1.txt should have been removed\n");
    ok(!file_exists("test_5.txt"), "test_5.txt should have been removed\n");
    ok(file_exists("test4.txt"), "test4.txt should not have been removed\n");

    /* FOF_FILESONLY only deletes a dir if explicitly specified */
    init_shfo_tests();
    shfo.pFrom = "test_?.txt\0test4.txt\0";
    ok(!SHFileOperation(&shfo), "Failed to delete files and directory\n");
    ok(!file_exists("test4.txt"), "test4.txt should have been removed\n");
    ok(!file_exists("test_5.txt"), "test_5.txt should have been removed\n");
    ok(file_exists("test1.txt"), "test1.txt should not have been removed\n");

    /* try to delete an invalid filename */
    init_shfo_tests();
    shfo.pFrom = "\0";
    shfo.fFlags &= ~FOF_FILESONLY;
    shfo.fAnyOperationsAborted = FALSE;
    ret = SHFileOperation(&shfo);
    ok(ret == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %d\n", ret);
    ok(!shfo.fAnyOperationsAborted, "Expected no aborted operations\n");
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");

    /* try an invalid function */
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0";
    shfo.wFunc = 0;
    ret = SHFileOperation(&shfo);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", ret);
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");

    /* try an invalid list, only one null terminator */
    init_shfo_tests();
    shfo.pFrom = "";
    shfo.wFunc = FO_DELETE;
    ret = SHFileOperation(&shfo);
    ok(ret == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %d\n", ret);
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");

    /* delete a dir, and then a file inside the dir, same as
    * deleting a nonexistent file
    */
    init_shfo_tests();
    shfo.pFrom = "testdir2\0testdir2\\one.txt\0";
    ret = SHFileOperation(&shfo);
    ok(ret == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %d\n", ret);
    ok(!file_exists("testdir2"), "Expected testdir2 to not exist\n");
    ok(!file_exists("testdir2\\one.txt"), "Expected testdir2\\one.txt to not exist\n");

    /* try the FOF_NORECURSION flag, continues deleting subdirs */
    init_shfo_tests();
    shfo.pFrom = "testdir2\0";
    shfo.fFlags |= FOF_NORECURSION;
    ret = SHFileOperation(&shfo);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ok(!file_exists("testdir2\\one.txt"), "Expected testdir2\\one.txt to not exist\n");
    ok(!file_exists("testdir2\\nested"), "Expected testdir2\\nested to exist\n");
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
    ok(SHFileOperationA(&shfo), "File is not renamed moving to other directory "
       "when specifying directory name only\n");
    ok(file_exists("test1.txt"), "The file is removed\n");

    set_curr_dir_path(from, "test3.txt\0");
    set_curr_dir_path(to, "test4.txt\\test1.txt\0");
    ok(!SHFileOperationA(&shfo), "File is renamed moving to other directory\n");
    ok(file_exists("test4.txt\\test1.txt"), "The file is not renamed\n");

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    retval = SHFileOperationA(&shfo); /* W98 returns 0, W2K and newer returns ERROR_GEN_FAILURE, both do nothing */
    ok(!retval || retval == ERROR_GEN_FAILURE || retval == ERROR_INVALID_TARGET_HANDLE,
       "Can't rename many files, retval = %d\n", retval);
    ok(file_exists("test1.txt"), "The file is renamed - many files are specified\n");

    memcpy(&shfo2, &shfo, sizeof(SHFILEOPSTRUCTA));
    shfo2.fFlags |= FOF_MULTIDESTFILES;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    retval = SHFileOperationA(&shfo2); /* W98 returns 0, W2K and newer returns ERROR_GEN_FAILURE, both do nothing */
    ok(!retval || retval == ERROR_GEN_FAILURE || retval == ERROR_INVALID_TARGET_HANDLE,
       "Can't rename many files, retval = %d\n", retval);
    ok(file_exists("test1.txt"), "The file is not renamed - many files are specified\n");

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(!retval, "Rename file failed, retval = %d\n", retval);
    ok(!file_exists("test1.txt"), "The file is not renamed\n");
    ok(file_exists("test6.txt"), "The file is not renamed\n");

    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test1.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(!retval, "Rename file back failed, retval = %d\n", retval);

    set_curr_dir_path(from, "test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(!retval, "Rename dir failed, retval = %d\n", retval);
    ok(!file_exists("test4.txt"), "The dir is not renamed\n");
    ok(file_exists("test6.txt"), "The dir is not renamed\n");

    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    retval = SHFileOperationA(&shfo);
    ok(!retval, "Rename dir back failed, retval = %d\n", retval);

    /* try to rename more than one file to a single file */
    shfo.pFrom = "test1.txt\0test2.txt\0";
    shfo.pTo = "a.txt\0";
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_GEN_FAILURE, "Expected ERROR_GEN_FAILURE, got %d\n", retval);
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");
    ok(file_exists("test2.txt"), "Expected test2.txt to exist\n");

    /* pFrom doesn't exist */
    shfo.pFrom = "idontexist\0";
    shfo.pTo = "newfile\0";
    retval = SHFileOperationA(&shfo);
    ok(retval == 1026, "Expected 1026, got %d\n", retval);
    ok(!file_exists("newfile"), "Expected newfile to not exist\n");

    /* pTo already exist */
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "test2.txt\0";
    retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_ALREADY_EXISTS, "Expected ERROR_ALREADY_EXISTS, got %d\n", retval);

    /* pFrom is valid, but pTo is empty */
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "\0";
    retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(file_exists("test1.txt"), "Expected test1.txt to exist\n");

    /* pFrom is empty */
    shfo.pFrom = "\0";
    retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %d\n", retval);

    /* pFrom is NULL, commented out because it crashes on nt 4.0 */
#if 0
    shfo.pFrom = NULL;
    retval = SHFileOperationA(&shfo);
    ok(retval == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", retval);
#endif
}

/* tests the FO_COPY action */
static void test_copy(void)
{
    SHFILEOPSTRUCTA shfo, shfo2;
    CHAR from[5*MAX_PATH];
    CHAR to[5*MAX_PATH];
    FILEOP_FLAGS tmp_flags;
    DWORD retval;

    shfo.hwnd = NULL;
    shfo.wFunc = FO_COPY;
    shfo.pFrom = from;
    shfo.pTo = to;
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    ok(SHFileOperationA(&shfo), "Can't copy many files\n");
    ok(!file_exists("test6.txt"), "The file is not copied - many files are "
       "specified as a target\n");

    memcpy(&shfo2, &shfo, sizeof(SHFILEOPSTRUCTA));
    shfo2.fFlags |= FOF_MULTIDESTFILES;

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    ok(!SHFileOperationA(&shfo2), "Can't copy many files\n");
    ok(file_exists("test6.txt"), "The file is copied - many files are "
       "specified as a target\n");
    DeleteFileA("test6.txt");
    DeleteFileA("test7.txt");
    RemoveDirectoryA("test8.txt");

    /* number of sources do not correspond to number of targets */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    ok(SHFileOperationA(&shfo2), "Can't copy many files\n");
    ok(!file_exists("test6.txt"), "The file is not copied - many files are "
       "specified as a target\n");

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
    if (!retval)
        /* Win 95/NT returns success but copies only the files up to the nonexistent source */
        ok(file_exists("testdir2\\test1.txt"), "The file is not copied\n");
    else
    {
        /* Win 98/ME/2K/XP fail the entire operation with return code 1026 if one source file does not exist */
        ok(retval == 1026, "Files are copied to other directory\n");
        ok(!file_exists("testdir2\\test1.txt"), "The file is copied\n");
    }
    ok(!file_exists("testdir2\\test2.txt"), "The file is copied\n");
    shfo.fFlags = tmp_flags;

    /* copy into a nonexistent directory */
    init_shfo_tests();
    shfo.fFlags = FOF_NOCONFIRMMKDIR;
    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "nonexistent\\notreal\\test2.txt\0");
    retval= SHFileOperation(&shfo);
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
    retval = SHFileOperation(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(file_exists("testdir2\\test1.txt"), "Expected testdir2\\test1 to exist\n");

    /* try to copy files to a file */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.pFrom = from;
    shfo.pTo = to;
    set_curr_dir_path(from, "test1.txt\0test2.txt\0");
    set_curr_dir_path(to, "test3.txt\0");
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(shfo.fAnyOperationsAborted, "Expected aborted operations\n");
    ok(!file_exists("test3.txt\\test2.txt"), "Expected test3.txt\\test2.txt to not exist\n");

    /* try to copy many files to nonexistent directory */
    DeleteFile(to);
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFile("test3.txt\\test1.txt"), "Expected test3.txt\\test1.txt to exist\n");
        ok(DeleteFile("test3.txt\\test2.txt"), "Expected test3.txt\\test1.txt to exist\n");
        ok(RemoveDirectory(to), "Expected test3.txt to exist\n");

    /* send in FOF_MULTIDESTFILES with too many destination files */
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0test2.txt\0test3.txt\0";
    shfo.pTo = "testdir2\\a.txt\0testdir2\\b.txt\0testdir2\\c.txt\0testdir2\\d.txt\0";
    shfo.fFlags |= FOF_NOERRORUI | FOF_MULTIDESTFILES;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(shfo.fAnyOperationsAborted, "Expected aborted operations\n");
    ok(!file_exists("testdir2\\a.txt"), "Expected testdir2\\a.txt to not exist\n");

    /* send in FOF_MULTIDESTFILES with too many destination files */
    shfo.pFrom = "test1.txt\0test2.txt\0test3.txt\0";
    shfo.pTo = "e.txt\0f.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(shfo.fAnyOperationsAborted, "Expected aborted operations\n");
    ok(!file_exists("e.txt"), "Expected e.txt to not exist\n");

    /* use FOF_MULTIDESTFILES with files and a source directory */
    shfo.pFrom = "test1.txt\0test2.txt\0test4.txt\0";
    shfo.pTo = "testdir2\\a.txt\0testdir2\\b.txt\0testdir2\\c.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperation(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(DeleteFile("testdir2\\a.txt"), "Expected testdir2\\a.txt to exist\n");
    ok(DeleteFile("testdir2\\b.txt"), "Expected testdir2\\b.txt to exist\n");
    ok(RemoveDirectory("testdir2\\c.txt"), "Expected testdir2\\c.txt to exist\n");

    /* try many dest files without FOF_MULTIDESTFILES flag */
    shfo.pFrom = "test1.txt\0test2.txt\0test3.txt\0";
    shfo.pTo = "a.txt\0b.txt\0c.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    shfo.fFlags &= ~FOF_MULTIDESTFILES;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(!file_exists("a.txt"), "Expected a.txt to not exist\n");

    /* try a glob */
    shfo.pFrom = "test?.txt\0";
    shfo.pTo = "testdir2\0";
    shfo.fFlags &= ~FOF_MULTIDESTFILES;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(file_exists("testdir2\\test1.txt"), "Expected testdir2\\test1.txt to exist\n");

    /* try a glob with FOF_FILESONLY */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.pFrom = "test?.txt\0";
    shfo.fFlags |= FOF_FILESONLY;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(file_exists("testdir2\\test1.txt"), "Expected testdir2\\test1.txt to exist\n");
    ok(!file_exists("testdir2\\test4.txt"), "Expected testdir2\\test4.txt to not exist\n");

    /* try a glob with FOF_MULTIDESTFILES and the same number
    * of dest files that we would expect
    */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.pTo = "testdir2\\a.txt\0testdir2\\b.txt\0testdir2\\c.txt\0testdir2\\d.txt\0";
    shfo.fFlags &= ~FOF_FILESONLY;
    shfo.fFlags |= FOF_MULTIDESTFILES;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(shfo.fAnyOperationsAborted, "Expected aborted operations\n");
    ok(!file_exists("testdir2\\a.txt"), "Expected testdir2\\test1.txt to not exist\n");
    ok(!RemoveDirectory("b.txt"), "b.txt should not exist\n");

    /* copy one file to two others, second is ignored */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "b.txt\0c.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFile("b.txt"), "Expected b.txt to exist\n");
    ok(!DeleteFile("c.txt"), "Expected c.txt to not exist\n");

    /* copy two file to three others, all fail */
    shfo.pFrom = "test1.txt\0test2.txt\0";
    shfo.pTo = "b.txt\0c.txt\0d.txt\0";
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(shfo.fAnyOperationsAborted, "Expected operations to be aborted\n");
    ok(!DeleteFile("b.txt"), "Expected b.txt to not exist\n");

    /* copy one file and one directory to three others */
    shfo.pFrom = "test1.txt\0test4.txt\0";
    shfo.pTo = "b.txt\0c.txt\0d.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(shfo.fAnyOperationsAborted, "Expected operations to be aborted\n");
    ok(!DeleteFile("b.txt"), "Expected b.txt to not exist\n");
    ok(!DeleteFile("c.txt"), "Expected c.txt to not exist\n");

    /* copy a directory with a file beneath it, plus some files */
    createTestFile("test4.txt\\a.txt");
    shfo.pFrom = "test4.txt\0test1.txt\0";
    shfo.pTo = "testdir2\0";
    shfo.fFlags &= ~FOF_MULTIDESTFILES;
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperation(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(DeleteFile("testdir2\\test1.txt"), "Expected newdir\\test1.txt to exist\n");
    ok(DeleteFile("testdir2\\test4.txt\\a.txt"), "Expected a.txt to exist\n");
    ok(RemoveDirectory("testdir2\\test4.txt"), "Expected testdir2\\test4.txt to exist\n");

    /* copy one directory and a file in that dir to another dir */
    shfo.pFrom = "test4.txt\0test4.txt\\a.txt\0";
    shfo.pTo = "testdir2\0";
    retval = SHFileOperation(&shfo);
    ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(DeleteFile("testdir2\\test4.txt\\a.txt"), "Expected a.txt to exist\n");
    ok(DeleteFile("testdir2\\a.txt"), "Expected testdir2\\a.txt to exist\n");

    /* copy a file in a directory first, and then the directory to a nonexistent dir */
    shfo.pFrom = "test4.txt\\a.txt\0test4.txt\0";
    shfo.pTo = "nonexistent\0";
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(shfo.fAnyOperationsAborted, "Expected operations to be aborted\n");
    ok(!file_exists("nonexistent\\test4.txt"), "Expected nonexistent\\test4.txt to not exist\n");
    DeleteFile("test4.txt\\a.txt");

    /* destination is same as source file */
    shfo.pFrom = "test1.txt\0test2.txt\0test3.txt\0";
    shfo.pTo = "b.txt\0test2.txt\0c.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    shfo.fFlags = FOF_NOERRORUI | FOF_MULTIDESTFILES;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_NO_MORE_SEARCH_HANDLES,
           "Expected ERROR_NO_MORE_SEARCH_HANDLES, got %d\n", retval);
        ok(!shfo.fAnyOperationsAborted, "Expected no operations to be aborted\n");
        ok(DeleteFile("b.txt"), "Expected b.txt to exist\n");
    ok(!file_exists("c.txt"), "Expected c.txt to not exist\n");

    /* destination is same as source directory */
    shfo.pFrom = "test1.txt\0test4.txt\0test3.txt\0";
    shfo.pTo = "b.txt\0test4.txt\0c.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(DeleteFile("b.txt"), "Expected b.txt to exist\n");
    ok(!file_exists("c.txt"), "Expected c.txt to not exist\n");

    /* copy a directory into itself, error displayed in UI */
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "test4.txt\\newdir\0";
    shfo.fFlags &= ~FOF_MULTIDESTFILES;
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(!RemoveDirectory("test4.txt\\newdir"), "Expected test4.txt\\newdir to not exist\n");

    /* copy a directory to itself, error displayed in UI */
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "test4.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);

    /* copy a file into a directory, and the directory into itself */
    shfo.pFrom = "test1.txt\0test4.txt\0";
    shfo.pTo = "test4.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    shfo.fFlags |= FOF_NOCONFIRMATION;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(DeleteFile("test4.txt\\test1.txt"), "Expected test4.txt\\test1.txt to exist\n");

    /* copy a file to a file, and the directory into itself */
    shfo.pFrom = "test1.txt\0test4.txt\0";
    shfo.pTo = "test4.txt\\a.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperation(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(!file_exists("test4.txt\\a.txt"), "Expected test4.txt\\a.txt to not exist\n");

    /* copy a nonexistent file to a nonexistent directory */
    shfo.pFrom = "e.txt\0";
    shfo.pTo = "nonexistent\0";
    shfo.fAnyOperationsAborted = FALSE;
    retval = SHFileOperation(&shfo);
    ok(retval == 1026, "Expected 1026, got %d\n", retval);
    ok(!file_exists("nonexistent\\e.txt"), "Expected nonexistent\\e.txt to not exist\n");
    ok(!file_exists("nonexistent"), "Expected nonexistent to not exist\n");

    /* Overwrite tests */
    clean_after_shfo_tests();
    init_shfo_tests();
    shfo.fFlags = FOF_NOCONFIRMATION;
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "test2.txt\0";
    shfo.fAnyOperationsAborted = FALSE;
    /* without FOF_NOCOFIRMATION the confirmation is Yes/No */
    retval = SHFileOperation(&shfo);
    ok(retval == 0, "Expected 0, got %d\n", retval);
    ok(file_has_content("test2.txt", "test1.txt\n"), "The file was not copied\n");

    shfo.pFrom = "test3.txt\0test1.txt\0";
    shfo.pTo = "test2.txt\0one.txt\0";
    shfo.fFlags = FOF_NOCONFIRMATION | FOF_MULTIDESTFILES;
    /* without FOF_NOCOFIRMATION the confirmation is Yes/Yes to All/No/Cancel */
    retval = SHFileOperation(&shfo);
    ok(retval == 0, "Expected 0, got %d\n", retval);
    ok(file_has_content("test2.txt", "test3.txt\n"), "The file was not copied\n");

    shfo.pFrom = "one.txt\0";
    shfo.pTo = "testdir2\0";
    shfo.fFlags = FOF_NOCONFIRMATION;
    /* without FOF_NOCOFIRMATION the confirmation is Yes/No */
    retval = SHFileOperation(&shfo);
    ok(retval == 0, "Expected 0, got %d\n", retval);
    ok(file_has_content("testdir2\\one.txt", "test1.txt\n"), "The file was not copied\n");

    createTestFile("test4.txt\\test1.txt");
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "testdir2\0";
    shfo.fFlags = FOF_NOCONFIRMATION;
    ok(!SHFileOperation(&shfo), "First SHFileOperation failed\n");
    createTestFile("test4.txt\\.\\test1.txt"); /* modify the content of the file */
    /* without FOF_NOCOFIRMATION the confirmation is "This folder already contains a folder named ..." */
    retval = SHFileOperation(&shfo);
    ok(retval == 0, "Expected 0, got %d\n", retval);
    ok(file_has_content("testdir2\\test4.txt\\test1.txt", "test4.txt\\.\\test1.txt\n"), "The file was not copied\n");
}

/* tests the FO_MOVE action */
static void test_move(void)
{
    SHFILEOPSTRUCTA shfo, shfo2;
    CHAR from[5*MAX_PATH];
    CHAR to[5*MAX_PATH];
    DWORD retval;

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
    ok(!SHFileOperationA(&shfo2), "Move many files\n");
    ok(file_exists("test6.txt"), "The file is moved - many files are "
       "specified as a target\n");
    DeleteFileA("test6.txt");
    DeleteFileA("test7.txt");
    RemoveDirectoryA("test8.txt");

    init_shfo_tests();

    /* number of sources do not correspond to number of targets */
    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0");
    ok(SHFileOperationA(&shfo2), "Can't move many files\n");
    ok(!file_exists("test6.txt"), "The file is not moved - many files are "
       "specified as a target\n");

    init_shfo_tests();

    set_curr_dir_path(from, "test3.txt\0");
    set_curr_dir_path(to, "test4.txt\\test1.txt\0");
    ok(!SHFileOperationA(&shfo), "File is moved moving to other directory\n");
    ok(file_exists("test4.txt\\test1.txt"), "The file is moved\n");

    set_curr_dir_path(from, "test1.txt\0test2.txt\0test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0test7.txt\0test8.txt\0");
    ok(SHFileOperationA(&shfo), "Cannot move many files\n");
    ok(file_exists("test1.txt"), "The file is not moved. Many files are specified\n");
    ok(file_exists("test4.txt"), "The directory is not moved. Many files are specified\n");

    set_curr_dir_path(from, "test1.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    ok(!SHFileOperationA(&shfo), "Move file\n");
    ok(!file_exists("test1.txt"), "The file is moved\n");
    ok(file_exists("test6.txt"), "The file is moved\n");
    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test1.txt\0");
    ok(!SHFileOperationA(&shfo), "Move file back\n");

    set_curr_dir_path(from, "test4.txt\0");
    set_curr_dir_path(to, "test6.txt\0");
    ok(!SHFileOperationA(&shfo), "Move dir\n");
    ok(!file_exists("test4.txt"), "The dir is moved\n");
    ok(file_exists("test6.txt"), "The dir is moved\n");
    set_curr_dir_path(from, "test6.txt\0");
    set_curr_dir_path(to, "test4.txt\0");
    ok(!SHFileOperationA(&shfo), "Move dir back\n");

    /* move one file to two others */
    init_shfo_tests();
    shfo.pFrom = "test1.txt\0";
    shfo.pTo = "a.txt\0b.txt\0";
    retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(!file_exists("test1.txt"), "Expected test1.txt to not exist\n");
        ok(DeleteFile("a.txt"), "Expected a.txt to exist\n");
    ok(!file_exists("b.txt"), "Expected b.txt to not exist\n");

    /* move two files to one other */
    shfo.pFrom = "test2.txt\0test3.txt\0";
    shfo.pTo = "test1.txt\0";
    retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
        ok(!file_exists("test1.txt"), "Expected test1.txt to not exist\n");
    ok(file_exists("test2.txt"), "Expected test2.txt to exist\n");
    ok(file_exists("test3.txt"), "Expected test3.txt to exist\n");

    /* move a directory into itself */
    shfo.pFrom = "test4.txt\0";
    shfo.pTo = "test4.txt\\b.txt\0";
    retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
    ok(!RemoveDirectory("test4.txt\\b.txt"), "Expected test4.txt\\b.txt to not exist\n");
    ok(file_exists("test4.txt"), "Expected test4.txt to exist\n");

    /* move many files without FOF_MULTIDESTFILES */
    shfo.pFrom = "test2.txt\0test3.txt\0";
    shfo.pTo = "d.txt\0e.txt\0";
    retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(!DeleteFile("d.txt"), "Expected d.txt to not exist\n");
    ok(!DeleteFile("e.txt"), "Expected e.txt to not exist\n");

    /* number of sources != number of targets */
    shfo.pTo = "d.txt\0";
    shfo.fFlags |= FOF_MULTIDESTFILES;
    retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(!DeleteFile("d.txt"), "Expected d.txt to not exist\n");

    /* FO_MOVE does not create dest directories */
    shfo.pFrom = "test2.txt\0";
    shfo.pTo = "dir1\\dir2\\test2.txt\0";
    retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_CANCELLED, "Expected ERROR_CANCELLED, got %d\n", retval);
    ok(!file_exists("dir1"), "Expected dir1 to not exist\n");

    /* try to overwrite an existing file */
    shfo.pTo = "test3.txt\0";
    retval = SHFileOperationA(&shfo);
        ok(retval == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", retval);
        ok(!file_exists("test2.txt"), "Expected test2.txt to not exist\n");
    ok(file_exists("test3.txt"), "Expected test3.txt to exist\n");
}

static void test_sh_create_dir(void)
{
    CHAR path[MAX_PATH];
    int ret;

    if(!pSHCreateDirectoryExA)
    {
	trace("skipping SHCreateDirectoryExA tests\n");
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
    ok(file_exists("c:\\testdir3"), "The directory is not created\n");
}

static void test_unicode(void)
{
    SHFILEOPSTRUCTW shfoW;
    int ret;
    HANDLE file;

    shfoW.hwnd = NULL;
    shfoW.wFunc = FO_DELETE;
    shfoW.pFrom = UNICODE_PATH;
    shfoW.pTo = '\0';
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
    ret = SHFileOperationW(&shfoW);
    ok(!ret, "File is not removed, ErrorCode: %d\n", ret);
    ok(!file_existsW(UNICODE_PATH), "The file should have been removed\n");

    /* Try to trash a file with unicode filename */
    createTestFileW(UNICODE_PATH);
    shfoW.fFlags |= FOF_ALLOWUNDO;
    ok(file_existsW(UNICODE_PATH), "The file does not exist\n");
    ret = SHFileOperationW(&shfoW);
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
    ret = SHFileOperationW(&shfoW);
    ok(!ret, "Directory is not removed, ErrorCode: %d\n", ret);
    ok(!file_existsW(UNICODE_PATH), "The directory should have been removed\n");

    /* Try to trash a directory with unicode filename */
    ret = pSHCreateDirectoryExW(NULL, UNICODE_PATH, NULL);
    ok(!ret, "SHCreateDirectoryExW returned %d\n", ret);
    ok(file_existsW(UNICODE_PATH), "The directory was not created\n");
    shfoW.fFlags |= FOF_ALLOWUNDO;
    ret = SHFileOperationW(&shfoW);
    ok(!ret, "Directory is not removed, ErrorCode: %d\n", ret);
    ok(!file_existsW(UNICODE_PATH), "The directory should have been removed\n");
}

START_TEST(shlfileop)
{
    InitFunctionPointers();

    clean_after_shfo_tests();

    init_shfo_tests();
    test_get_file_info();
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

    test_unicode();
}
