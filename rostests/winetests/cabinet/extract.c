/*
 * Unit tests for cabinet.dll extract functions
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
#include <fci.h>
#include "wine/test.h"

/* make the max size large so there is only one cab file */
#define MEDIA_SIZE          999999999
#define FOLDER_THRESHOLD    900000

/* The following defintions were copied from dlls/cabinet/cabinet.h
 * because they are undocumented in windows.
 */

/* EXTRACTdest flags */
#define EXTRACT_FILLFILELIST  0x00000001
#define EXTRACT_EXTRACTFILES  0x00000002

struct ExtractFileList {
    LPSTR  filename;
    struct ExtractFileList *next;
    BOOL   unknown;  /* always 1L */
};

/* the first parameter of the function extract */
typedef struct {
    long   result1;          /* 0x000 */
    long   unknown1[3];      /* 0x004 */
    struct ExtractFileList *filelist; /* 0x010 */
    long   filecount;        /* 0x014 */
    long   flags;            /* 0x018 */
    char   directory[0x104]; /* 0x01c */
    char   lastfile[0x20c];  /* 0x120 */
} EXTRACTDEST;

/* function pointers */
HMODULE hCabinet;
static HRESULT (WINAPI *pExtract)(EXTRACTDEST*, LPCSTR);

CHAR CURR_DIR[MAX_PATH];

static void init_function_pointers(void)
{
    hCabinet = LoadLibraryA("cabinet.dll");

    if (hCabinet)
    {
        pExtract = (void *)GetProcAddress(hCabinet, "Extract");
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
    int len;

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len-1] == '\\'))
        CURR_DIR[len-1] = 0;

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

    DeleteFileA("extract.cab");
}

/* the FCI callbacks */

static void *mem_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void mem_free(void *memory)
{
    HeapFree(GetProcessHeap(), 0, memory);
}

static BOOL get_next_cabinet(PCCAB pccab, ULONG  cbPrevCab, void *pv)
{
    return TRUE;
}

static long progress(UINT typeStatus, ULONG cb1, ULONG cb2, void *pv)
{
    return 0;
}

static int file_placed(PCCAB pccab, char *pszFile, long cbFile,
                       BOOL fContinuation, void *pv)
{
    return 0;
}

static INT_PTR fci_open(char *pszFile, int oflag, int pmode, int *err, void *pv)
{
    HANDLE handle;
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;

    dwAccess = GENERIC_READ | GENERIC_WRITE;
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    if (GetFileAttributesA(pszFile) != INVALID_FILE_ATTRIBUTES)
        dwCreateDisposition = OPEN_EXISTING;
    else
        dwCreateDisposition = CREATE_NEW;

    handle = CreateFileA(pszFile, dwAccess, dwShareMode, NULL,
                         dwCreateDisposition, 0, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszFile);

    return (INT_PTR)handle;
}

static UINT fci_read(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwRead;
    BOOL res;

    res = ReadFile(handle, memory, cb, &dwRead, NULL);
    ok(res, "Failed to ReadFile\n");

    return dwRead;
}

static UINT fci_write(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwWritten;
    BOOL res;

    res = WriteFile(handle, memory, cb, &dwWritten, NULL);
    ok(res, "Failed to WriteFile\n");

    return dwWritten;
}

static int fci_close(INT_PTR hf, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    ok(CloseHandle(handle), "Failed to CloseHandle\n");

    return 0;
}

static long fci_seek(INT_PTR hf, long dist, int seektype, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD ret;

    ret = SetFilePointer(handle, dist, NULL, seektype);
    ok(ret != INVALID_SET_FILE_POINTER, "Failed to SetFilePointer\n");

    return ret;
}

static int fci_delete(char *pszFile, int *err, void *pv)
{
    BOOL ret = DeleteFileA(pszFile);
    ok(ret, "Failed to DeleteFile %s\n", pszFile);

    return 0;
}

static BOOL get_temp_file(char *pszTempName, int cbTempName, void *pv)
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

static INT_PTR get_open_info(char *pszName, USHORT *pdate, USHORT *ptime,
                             USHORT *pattribs, int *err, void *pv)
{
    BY_HANDLE_FILE_INFORMATION finfo;
    FILETIME filetime;
    HANDLE handle;
    DWORD attrs;
    BOOL res;

    handle = CreateFile(pszName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszName);

    res = GetFileInformationByHandle(handle, &finfo);
    ok(res, "Expected GetFileInformationByHandle to succeed\n");

    FileTimeToLocalFileTime(&finfo.ftLastWriteTime, &filetime);
    FileTimeToDosDateTime(&filetime, pdate, ptime);

    attrs = GetFileAttributes(pszName);
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
    BOOL res;

    set_cab_parameters(&cabParams);

    hfci = FCICreate(&erf, file_placed, mem_alloc, mem_free, fci_open,
                      fci_read, fci_write, fci_close, fci_seek, fci_delete,
                      get_temp_file, &cabParams, NULL);

    ok(hfci != NULL, "Failed to create an FCI context\n");

    add_file(hfci, "a.txt");
    add_file(hfci, "b.txt");
    add_file(hfci, "testdir\\c.txt");
    add_file(hfci, "testdir\\d.txt");

    res = FCIFlushCabinet(hfci, FALSE, get_next_cabinet, progress);
    ok(res, "Failed to flush the cabinet\n");

    res = FCIDestroy(hfci);
    ok(res, "Failed to destroy the cabinet\n");
}

static void test_Extract(void)
{
    EXTRACTDEST extractDest;
    HRESULT res;

    /* native windows crashes if
    *   - invalid parameters are sent in
    *   - you call EXTRACT_EXTRACTFILES without calling
    *     EXTRACT_FILLFILELIST first or at the same time
    */

    /* try to extract all files */
    ZeroMemory(&extractDest, sizeof(EXTRACTDEST));
    lstrcpyA(extractDest.directory, "dest");
    extractDest.flags = EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES;
    res = pExtract(&extractDest, "extract.cab");
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to exist\n");
    ok(RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to exist\n");

    /* try fill file list operation */
    ZeroMemory(&extractDest, sizeof(EXTRACTDEST));
    lstrcpyA(extractDest.directory, "dest");
    extractDest.flags = EXTRACT_FILLFILELIST;
    res = pExtract(&extractDest, "extract.cab");
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to not exist\n");
    ok(extractDest.filecount == 4, "Expected 4 files, got %ld\n", extractDest.filecount);
    ok(!lstrcmpA(extractDest.lastfile, "dest\\testdir\\d.txt"),
        "Expected last file to be dest\\testdir\\d.txt, got %s\n", extractDest.lastfile);

    /* try extract files operation once file list is filled */
    extractDest.flags = EXTRACT_EXTRACTFILES;
    res = pExtract(&extractDest, "extract.cab");
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to exist\n");
    ok(RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to exist\n");
    ok(RemoveDirectoryA("dest"), "Expected dest to exist\n");

    /* Extract does not extract files if the dest dir does not exist */
    res = pExtract(&extractDest, "extract.cab");
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to not exist\n");

    /* remove two of the files in the list */
    extractDest.filelist->next = extractDest.filelist->next->next;
    extractDest.filelist->next->next = NULL;
    CreateDirectoryA("dest", NULL);
    res = pExtract(&extractDest, "extract.cab");
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(!DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to not exist\n");
    ok(RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to exist\n");
    ok(RemoveDirectoryA("dest"), "Expected dest\\testdir to exist\n");
}

START_TEST(extract)
{
    init_function_pointers();
    create_test_files();
    create_cab_file();

    test_Extract();

    delete_test_files();
}
