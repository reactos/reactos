/*
 * Unit tests for advpack.dll file functions
 *
 * Copyright (C) 2006 James Hawkins
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

#include <stdio.h>
#include <windows.h>
#include <advpub.h>
#include <fci.h>
#include "wine/test.h"

/* make the max size large so there is only one cab file */
#define MEDIA_SIZE          999999999
#define FOLDER_THRESHOLD    900000

/* function pointers */
static HMODULE hAdvPack;
static HRESULT (WINAPI *pAddDelBackupEntry)(LPCSTR, LPCSTR, LPCSTR, DWORD);
static HRESULT (WINAPI *pExtractFiles)(LPCSTR, LPCSTR, DWORD, LPCSTR, LPVOID, DWORD);
static HRESULT (WINAPI *pExtractFilesW)(const WCHAR*,const WCHAR*,DWORD,const WCHAR*,void*,DWORD);
static HRESULT (WINAPI *pAdvInstallFile)(HWND,LPCSTR,LPCSTR,LPCSTR,LPCSTR,DWORD,DWORD);

static CHAR CURR_DIR[MAX_PATH];

static void init_function_pointers(void)
{
    hAdvPack = LoadLibraryA("advpack.dll");

    if (hAdvPack)
    {
        pAddDelBackupEntry = (void *)GetProcAddress(hAdvPack, "AddDelBackupEntry");
        pExtractFiles = (void *)GetProcAddress(hAdvPack, "ExtractFiles");
        pExtractFilesW = (void *)GetProcAddress(hAdvPack, "ExtractFilesW");
        pAdvInstallFile = (void*)GetProcAddress(hAdvPack, "AdvInstallFile");
    }
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

static void create_test_files(void)
{
    createTestFile("a.txt");
    createTestFile("b.txt");
    CreateDirectoryA("testdir", NULL);
    createTestFile("testdir\\c.txt");
    createTestFile("testdir\\d.txt");
    CreateDirectoryA("dest", NULL);
}

static void delete_test_files(void)
{
    DeleteFileA("a.txt");
    DeleteFileA("b.txt");
    DeleteFileA("testdir\\c.txt");
    DeleteFileA("testdir\\d.txt");
    RemoveDirectoryA("testdir");
    RemoveDirectoryA("dest");

    DeleteFileA("extract.cab");
}

static BOOL check_ini_file_attr(LPSTR filename)
{
    BOOL ret;
    DWORD expected = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY;
    DWORD attr = GetFileAttributesA(filename);

    ret = (attr & expected) && (attr != INVALID_FILE_ATTRIBUTES);
    SetFileAttributesA(filename, FILE_ATTRIBUTE_NORMAL);

    return ret;
}

static void test_AddDelBackupEntry(void)
{
    BOOL ret;
    HRESULT res;
    CHAR path[MAX_PATH + 13];
    CHAR windir[MAX_PATH];

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\backup\\basename.INI");

    /* native AddDelBackupEntry crashes if lpcszBaseName is NULL */

    /* try a NULL file list */
    res = pAddDelBackupEntry(NULL, "backup", "basename", AADBE_ADD_ENTRY);
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(!DeleteFileA(path), "Expected path to not exist\n");

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\backup\\.INI");

    /* try an empty base name */
    res = pAddDelBackupEntry("one\0two\0three\0", "backup", "", AADBE_ADD_ENTRY);
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(!DeleteFileA(path), "Expected path to not exist\n");

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\basename.INI");

    /* try an invalid flag */
    res = pAddDelBackupEntry("one\0two\0three\0", NULL, "basename", 0);
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(!DeleteFileA(path), "Expected path to not exist\n");

    lstrcpyA(path, "c:\\basename.INI");

    /* create the INF file */
    res = pAddDelBackupEntry("one\0two\0three\0", "c:\\", "basename", AADBE_ADD_ENTRY);
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES)
    {
        ok(check_ini_file_attr(path), "Expected ini file to be hidden\n");
        ok(DeleteFileA(path), "Expected path to exist\n");
    }
    else
        win_skip("Test file could not be created\n");

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\backup\\basename.INI");

    /* try to create the INI file in a nonexistent directory */
    RemoveDirectoryA("backup");
    res = pAddDelBackupEntry("one\0two\0three\0", "backup", "basename", AADBE_ADD_ENTRY);
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(!check_ini_file_attr(path), "Expected ini file to not be hidden\n");
    ok(!DeleteFileA(path), "Expected path to not exist\n");

    /* try an existent, relative backup directory */
    CreateDirectoryA("backup", NULL);
    res = pAddDelBackupEntry("one\0two\0three\0", "backup", "basename", AADBE_ADD_ENTRY);
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(check_ini_file_attr(path), "Expected ini file to be hidden\n");
    ok(DeleteFileA(path), "Expected path to exist\n");
    RemoveDirectoryA("backup");

    GetWindowsDirectoryA(windir, sizeof(windir));
    sprintf(path, "%s\\basename.INI", windir);

    /* try a NULL backup dir, INI is created in the windows directory */
    res = pAddDelBackupEntry("one\0two\0three\0", NULL, "basename", AADBE_ADD_ENTRY);
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);

    /* remove the entries with AADBE_DEL_ENTRY */
    SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
    res = pAddDelBackupEntry("one\0three\0", NULL, "basename", AADBE_DEL_ENTRY);
    SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ret = DeleteFileA(path);
    ok(ret == TRUE ||
       broken(ret == FALSE), /* win98 */
       "Expected path to exist\n");
}

/* the FCI callbacks */

static void * CDECL mem_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void CDECL mem_free(void *memory)
{
    HeapFree(GetProcessHeap(), 0, memory);
}

static BOOL CDECL get_next_cabinet(PCCAB pccab, ULONG  cbPrevCab, void *pv)
{
    return TRUE;
}

static LONG CDECL progress(UINT typeStatus, ULONG cb1, ULONG cb2, void *pv)
{
    return 0;
}

static int CDECL file_placed(PCCAB pccab, char *pszFile, LONG cbFile,
                             BOOL fContinuation, void *pv)
{
    return 0;
}

static INT_PTR CDECL fci_open(char *pszFile, int oflag, int pmode, int *err, void *pv)
{
    HANDLE handle;
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;
    
    dwAccess = GENERIC_READ | GENERIC_WRITE;
    /* FILE_SHARE_DELETE is not supported by Windows Me/98/95 */
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    if (GetFileAttributesA(pszFile) != INVALID_FILE_ATTRIBUTES)
        dwCreateDisposition = OPEN_EXISTING;
    else
        dwCreateDisposition = CREATE_NEW;

    handle = CreateFileA(pszFile, dwAccess, dwShareMode, NULL,
                         dwCreateDisposition, 0, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszFile);

    return (INT_PTR)handle;
}

static UINT CDECL fci_read(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwRead;
    BOOL res;
    
    res = ReadFile(handle, memory, cb, &dwRead, NULL);
    ok(res, "Failed to ReadFile\n");

    return dwRead;
}

static UINT CDECL fci_write(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwWritten;
    BOOL res;

    res = WriteFile(handle, memory, cb, &dwWritten, NULL);
    ok(res, "Failed to WriteFile\n");

    return dwWritten;
}

static int CDECL fci_close(INT_PTR hf, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    ok(CloseHandle(handle), "Failed to CloseHandle\n");

    return 0;
}

static LONG CDECL fci_seek(INT_PTR hf, LONG dist, int seektype, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD ret;
    
    ret = SetFilePointer(handle, dist, NULL, seektype);
    ok(ret != INVALID_SET_FILE_POINTER, "Failed to SetFilePointer\n");

    return ret;
}

static int CDECL fci_delete(char *pszFile, int *err, void *pv)
{
    BOOL ret = DeleteFileA(pszFile);
    ok(ret, "Failed to DeleteFile %s\n", pszFile);

    return 0;
}

static BOOL CDECL get_temp_file(char *pszTempName, int cbTempName, void *pv)
{
    LPSTR tempname;

    tempname = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    GetTempFileNameA(".", "xx", 0, tempname);

    if (tempname && (strlen(tempname) < (unsigned)cbTempName))
    {
        lstrcpyA(pszTempName, tempname);
        HeapFree(GetProcessHeap(), 0, tempname);
        return TRUE;
    }

    HeapFree(GetProcessHeap(), 0, tempname);

    return FALSE;
}

static INT_PTR CDECL get_open_info(char *pszName, USHORT *pdate, USHORT *ptime,
                                   USHORT *pattribs, int *err, void *pv)
{
    BY_HANDLE_FILE_INFORMATION finfo;
    FILETIME filetime;
    HANDLE handle;
    DWORD attrs;
    BOOL res;

    handle = CreateFileA(pszName, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszName);

    res = GetFileInformationByHandle(handle, &finfo);
    ok(res, "Expected GetFileInformationByHandle to succeed\n");
   
    FileTimeToLocalFileTime(&finfo.ftLastWriteTime, &filetime);
    FileTimeToDosDateTime(&filetime, pdate, ptime);

    attrs = GetFileAttributesA(pszName);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Failed to GetFileAttributes\n");

    return (INT_PTR)handle;
}

static void add_file(HFCI hfci, char *file)
{
    char path[MAX_PATH];
    BOOL res;

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, file);

    res = FCIAddFile(hfci, path, file, FALSE, get_next_cabinet, progress,
                     get_open_info, tcompTYPE_MSZIP);
    ok(res, "Expected FCIAddFile to succeed\n");
}

static void set_cab_parameters(PCCAB pCabParams)
{
    ZeroMemory(pCabParams, sizeof(CCAB));

    pCabParams->cb = MEDIA_SIZE;
    pCabParams->cbFolderThresh = FOLDER_THRESHOLD;
    pCabParams->setID = 0xbeef;
    lstrcpyA(pCabParams->szCabPath, CURR_DIR);
    lstrcatA(pCabParams->szCabPath, "\\");
    lstrcpyA(pCabParams->szCab, "extract.cab");
}

static void create_cab_file(void)
{
    CCAB cabParams;
    HFCI hfci;
    ERF erf;
    static CHAR a_txt[] = "a.txt",
                b_txt[] = "b.txt",
                testdir_c_txt[] = "testdir\\c.txt",
                testdir_d_txt[] = "testdir\\d.txt";
    BOOL res;

    set_cab_parameters(&cabParams);

    hfci = FCICreate(&erf, file_placed, mem_alloc, mem_free, fci_open,
                      fci_read, fci_write, fci_close, fci_seek, fci_delete,
                      get_temp_file, &cabParams, NULL);

    ok(hfci != NULL, "Failed to create an FCI context\n");

    add_file(hfci, a_txt);
    add_file(hfci, b_txt);
    add_file(hfci, testdir_c_txt);
    add_file(hfci, testdir_d_txt);

    res = FCIFlushCabinet(hfci, FALSE, get_next_cabinet, progress);
    ok(res, "Failed to flush the cabinet\n");

    res = FCIDestroy(hfci);
    ok(res, "Failed to destroy the cabinet\n");
}

static void test_ExtractFiles(void)
{
    HRESULT hr;
    char destFolder[MAX_PATH];

    lstrcpyA(destFolder, CURR_DIR);
    lstrcatA(destFolder, "\\");
    lstrcatA(destFolder, "dest");

    /* try NULL cab file */
    hr = pExtractFiles(NULL, destFolder, 0, NULL, NULL, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);
    ok(RemoveDirectoryA("dest"), "Expected dest to exist\n");
    
    /* try NULL destination */
    hr = pExtractFiles("extract.cab", NULL, 0, NULL, NULL, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);
    ok(!RemoveDirectoryA("dest"), "Expected dest to not exist\n");

    /* extract all files in the cab to nonexistent destination directory */
    hr = pExtractFiles("extract.cab", destFolder, 0, NULL, NULL, 0);
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) ||
       hr == E_FAIL, /* win95 */
       "Expected %08lx or %08lx, got %08lx\n", E_FAIL,
       HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), hr);
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to not exist\n");
    ok(!RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to not exist\n");
    ok(!RemoveDirectoryA("dest"), "Expected dest to not exist\n");

    /* extract all files in the cab to the destination directory */
    CreateDirectoryA("dest", NULL);
    hr = pExtractFiles("extract.cab", destFolder, 0, NULL, NULL, 0);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to exist\n");
    ok(RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to exist\n");

    /* extract all files to a relative destination directory */
    hr = pExtractFiles("extract.cab", "dest", 0, NULL, NULL, 0);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to exist\n");
    ok(RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to exist\n");

    /* only extract two of the files from the cab */
    hr = pExtractFiles("extract.cab", "dest", 0, "a.txt:testdir\\c.txt", NULL, 0);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to exist\n");
    ok(!DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to not exist\n");

    /* use valid chars before and after file list */
    hr = pExtractFiles("extract.cab", "dest", 0, " :\t: a.txt:testdir\\c.txt  \t:", NULL, 0);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to exist\n");
    ok(!DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to not exist\n");

    /* use invalid chars before and after file list */
    hr = pExtractFiles("extract.cab", "dest", 0, " +-\\ a.txt:testdir\\c.txt  a_:", NULL, 0);
    ok(hr == E_FAIL, "Expected E_FAIL, got %ld\n", hr);
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to not exist\n");
    ok(!RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to not exist\n");

    /* try an empty file list */
    hr = pExtractFiles("extract.cab", "dest", 0, "", NULL, 0);
    ok(hr == E_FAIL, "Expected E_FAIL, got %ld\n", hr);
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to not exist\n");
    ok(!RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to not exist\n");

    /* try a nonexistent file in the file list */
    hr = pExtractFiles("extract.cab", "dest", 0, "a.txt:idontexist:testdir\\c.txt", NULL, 0);
    ok(hr == E_FAIL, "Expected E_FAIL, got %ld\n", hr);
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to not exist\n");
    ok(!RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to not exist\n");

    if(pExtractFilesW) {
        hr = pExtractFilesW(L"extract.cab", L"dest", 0, L"a.txt:testdir\\c.txt", NULL, 0);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
        ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
        ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
        ok(RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to exist\n");
        ok(!DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to not exist\n");
        ok(!DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to not exist\n");
    }else {
        win_skip("ExtractFilesW not available\n");
    }
}

static void test_AdvInstallFile(void)
{
    HRESULT hr;
    HMODULE hmod;
    char destFolder[MAX_PATH];

    hmod = LoadLibraryA("setupapi.dll");
    if (!hmod)
    {
        skip("setupapi.dll not present\n");
        return;
    }

    FreeLibrary(hmod);

    lstrcpyA(destFolder, CURR_DIR);
    lstrcatA(destFolder, "\\");
    lstrcatA(destFolder, "dest");

    createTestFile("source.txt");

    /* try invalid source directory */
    hr = pAdvInstallFile(NULL, NULL, "source.txt", destFolder, "destination.txt", 0, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);
    ok(!DeleteFileA("dest\\destination.txt"), "Expected dest\\destination.txt to not exist\n");

    /* try invalid source file */
    hr = pAdvInstallFile(NULL, CURR_DIR, NULL, destFolder, "destination.txt", 0, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);
    ok(!DeleteFileA("dest\\destination.txt"), "Expected dest\\destination.txt to not exist\n");

    /* try invalid destination directory */
    hr = pAdvInstallFile(NULL, CURR_DIR, "source.txt", NULL, "destination.txt", 0, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);
    ok(!DeleteFileA("dest\\destination.txt"), "Expected dest\\destination.txt to not exist\n");

    /* try copying to nonexistent destination directory */
    hr = pAdvInstallFile(NULL, CURR_DIR, "source.txt", destFolder, "destination.txt", 0, 0);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);
    ok(DeleteFileA("dest\\destination.txt"), "Expected dest\\destination.txt to exist\n");

    /* native windows screws up if the source file doesn't exist */

    /* test AIF_NOOVERWRITE behavior, asks the user to overwrite if AIF_QUIET is not specified */
    createTestFile("dest\\destination.txt");
    hr = pAdvInstallFile(NULL, CURR_DIR, "source.txt", destFolder,
                         "destination.txt", AIF_NOOVERWRITE | AIF_QUIET, 0);
    ok(hr == S_OK, "Expected S_OK, got %ld\n", hr);
    ok(DeleteFileA("dest\\destination.txt"), "Expected dest\\destination.txt to exist\n");
    ok(RemoveDirectoryA("dest"), "Expected dest to exist\n");

    DeleteFileA("source.txt");
}

START_TEST(files)
{
    DWORD len;
    char temp_path[MAX_PATH], prev_path[MAX_PATH];

    init_function_pointers();

    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPathA(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    lstrcpyA(CURR_DIR, temp_path);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    create_test_files();
    create_cab_file();

    test_AddDelBackupEntry();
    test_ExtractFiles();
    test_AdvInstallFile();

    delete_test_files();

    FreeLibrary(hAdvPack);
    SetCurrentDirectoryA(prev_path);
}
