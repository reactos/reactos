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
#include "fci.h"
#include "fdi.h"
#include "wine/test.h"

/* make the max size large so there is only one cab file */
#define MEDIA_SIZE          999999999
#define FOLDER_THRESHOLD    900000

/* The following definitions were copied from dlls/cabinet/cabinet.h
 * because they are undocumented in windows.
 */

/* SESSION Operation */
#define EXTRACT_FILLFILELIST  0x00000001
#define EXTRACT_EXTRACTFILES  0x00000002

struct FILELIST{
    LPSTR FileName;
    struct FILELIST *next;
    BOOL DoExtract;
};

typedef struct {
    INT FileSize;
    ERF Error;
    struct FILELIST *FileList;
    INT FileCount;
    INT Operation;
    CHAR Destination[MAX_PATH];
    CHAR CurrentFile[MAX_PATH];
    CHAR Reserved[MAX_PATH];
    struct FILELIST *FilterList;
} SESSION;

/* function pointers */
static HMODULE hCabinet;
static HRESULT (WINAPI *pExtract)(SESSION*, LPCSTR);

static CHAR CURR_DIR[MAX_PATH];

static void init_function_pointers(void)
{
    hCabinet = GetModuleHandleA("cabinet.dll");

    pExtract = (void *)GetProcAddress(hCabinet, "Extract");
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

static int getFileSize(const CHAR *name)
{
    HANDLE file;
    int size;
    file = CreateFileA(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return -1;
    size = GetFileSize(file, NULL);
    CloseHandle(file);
    return size;
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
    static CHAR a_txt[]         = "a.txt",
                b_txt[]         = "b.txt",
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

static BOOL check_list(struct FILELIST **node, const char *filename, BOOL do_extract)
{
    if (!*node)
        return FALSE;

    if (lstrcmpA((*node)->FileName, filename))
        return FALSE;

    if ((*node)->DoExtract != do_extract)
        return FALSE;

    *node = (*node)->next;
    return TRUE;
}

static void free_file_node(struct FILELIST *node)
{
    HeapFree(GetProcessHeap(), 0, node->FileName);
    HeapFree(GetProcessHeap(), 0, node);
}

static void free_file_list(SESSION* session)
{
    struct FILELIST *next, *curr = session->FileList;

    while (curr)
    {
        next = curr->next;
        free_file_node(curr);
        curr = next;
    }

    session->FileList = NULL;
}

static void test_Extract(void)
{
    SESSION session;
    HRESULT res;
    struct FILELIST *node;

    /* native windows crashes if
    *   - invalid parameters are sent in
    *   - you call EXTRACT_EXTRACTFILES without calling
    *     EXTRACT_FILLFILELIST first or at the same time
    */

    /* try to extract all files */
    ZeroMemory(&session, sizeof(SESSION));
    lstrcpyA(session.Destination, "dest");
    session.Operation = EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES;
    res = pExtract(&session, "extract.cab");
    node = session.FileList;
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(session.FileSize == 40, "Expected 40, got %d\n", session.FileSize);
    ok(session.Error.erfOper == FDIERROR_NONE,
       "Expected FDIERROR_NONE, got %d\n", session.Error.erfOper);
    ok(session.Error.erfType == 0, "Expected 0, got %d\n", session.Error.erfType);
    ok(session.Error.fError == FALSE, "Expected FALSE, got %d\n", session.Error.fError);
    ok(session.FileCount == 4, "Expected 4, got %d\n", session.FileCount);
    ok(session.Operation == (EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES),
       "Expected EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES, got %d\n", session.Operation);
    ok(!lstrcmpA(session.Destination, "dest"), "Expected dest, got %s\n", session.Destination);
    ok(!lstrcmpA(session.CurrentFile, "dest\\testdir\\d.txt"),
       "Expected dest\\testdir\\d.txt, got %s\n", session.CurrentFile);
    ok(!*session.Reserved, "Expected empty string, got %s\n", session.Reserved);
    ok(!session.FilterList, "Expected empty filter list\n");
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to exist\n");
    ok(RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to exist\n");
    ok(check_list(&node, "testdir\\d.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "testdir\\c.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "b.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "a.txt", FALSE), "list entry wrong\n");
    free_file_list(&session);

    /* try fill file list operation */
    ZeroMemory(&session, sizeof(SESSION));
    lstrcpyA(session.Destination, "dest");
    session.Operation = EXTRACT_FILLFILELIST;
    res = pExtract(&session, "extract.cab");
    node = session.FileList;
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(session.FileSize == 40, "Expected 40, got %d\n", session.FileSize);
    ok(session.Error.erfOper == FDIERROR_NONE,
       "Expected FDIERROR_NONE, got %d\n", session.Error.erfOper);
    ok(session.Error.erfType == 0, "Expected 0, got %d\n", session.Error.erfType);
    ok(session.Error.fError == FALSE, "Expected FALSE, got %d\n", session.Error.fError);
    ok(session.FileCount == 4, "Expected 4, got %d\n", session.FileCount);
    ok(session.Operation == EXTRACT_FILLFILELIST,
       "Expected EXTRACT_FILLFILELIST, got %d\n", session.Operation);
    ok(!lstrcmpA(session.Destination, "dest"), "Expected dest, got %s\n", session.Destination);
    ok(!lstrcmpA(session.CurrentFile, "dest\\testdir\\d.txt"),
       "Expected dest\\testdir\\d.txt, got %s\n", session.CurrentFile);
    ok(!*session.Reserved, "Expected empty string, got %s\n", session.Reserved);
    ok(!session.FilterList, "Expected empty filter list\n");
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to not exist\n");
    ok(check_list(&node, "testdir\\d.txt", TRUE), "list entry wrong\n");
    ok(check_list(&node, "testdir\\c.txt", TRUE), "list entry wrong\n");
    ok(check_list(&node, "b.txt", TRUE), "list entry wrong\n");
    ok(check_list(&node, "a.txt", TRUE), "list entry wrong\n");

    /* try extract files operation once file list is filled */
    session.Operation = EXTRACT_EXTRACTFILES;
    res = pExtract(&session, "extract.cab");
    node = session.FileList;
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(session.FileSize == 40, "Expected 40, got %d\n", session.FileSize);
    ok(session.Error.erfOper == FDIERROR_NONE,
       "Expected FDIERROR_NONE, got %d\n", session.Error.erfOper);
    ok(session.Error.erfType == 0, "Expected 0, got %d\n", session.Error.erfType);
    ok(session.Error.fError == FALSE, "Expected FALSE, got %d\n", session.Error.fError);
    ok(session.FileCount == 4, "Expected 4, got %d\n", session.FileCount);
    ok(session.Operation == EXTRACT_EXTRACTFILES,
       "Expected EXTRACT_EXTRACTFILES, got %d\n", session.Operation);
    ok(!lstrcmpA(session.Destination, "dest"), "Expected dest, got %s\n", session.Destination);
    ok(!lstrcmpA(session.CurrentFile, "dest\\testdir\\d.txt"),
       "Expected dest\\testdir\\d.txt, got %s\n", session.CurrentFile);
    ok(!*session.Reserved, "Expected empty string, got %s\n", session.Reserved);
    ok(!session.FilterList, "Expected empty filter list\n");
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to exist\n");
    ok(RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to exist\n");
    ok(RemoveDirectoryA("dest"), "Expected dest to exist\n");
    ok(check_list(&node, "testdir\\d.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "testdir\\c.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "b.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "a.txt", FALSE), "list entry wrong\n");

    /* Extract does not extract files if the dest dir does not exist */
    res = pExtract(&session, "extract.cab");
    node = session.FileList;
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(session.FileSize == 40, "Expected 40, got %d\n", session.FileSize);
    ok(session.Error.erfOper == FDIERROR_NONE,
       "Expected FDIERROR_NONE, got %d\n", session.Error.erfOper);
    ok(session.Error.erfType == 0, "Expected 0, got %d\n", session.Error.erfType);
    ok(session.Error.fError == FALSE, "Expected FALSE, got %d\n", session.Error.fError);
    ok(session.FileCount == 4, "Expected 4, got %d\n", session.FileCount);
    ok(session.Operation == EXTRACT_EXTRACTFILES,
       "Expected EXTRACT_EXTRACTFILES, got %d\n", session.Operation);
    ok(!lstrcmpA(session.Destination, "dest"), "Expected dest, got %s\n", session.Destination);
    ok(!lstrcmpA(session.CurrentFile, "dest\\testdir\\d.txt"),
       "Expected dest\\testdir\\d.txt, got %s\n", session.CurrentFile);
    ok(!*session.Reserved, "Expected empty string, got %s\n", session.Reserved);
    ok(!session.FilterList, "Expected empty filter list\n");
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to not exist\n");
    ok(!DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to not exist\n");
    ok(check_list(&node, "testdir\\d.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "testdir\\c.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "b.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "a.txt", FALSE), "list entry wrong\n");

    /* remove two of the files in the list */
    node = session.FileList->next;
    session.FileList->next = session.FileList->next->next;
    free_file_node(node);
    free_file_node(session.FileList->next->next);
    session.FileList->next->next = NULL;
    session.FilterList = NULL;
    CreateDirectoryA("dest", NULL);
    res = pExtract(&session, "extract.cab");
    node = session.FileList;
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(session.FileSize == 40, "Expected 40, got %d\n", session.FileSize);
    ok(session.Error.erfOper == FDIERROR_NONE,
       "Expected FDIERROR_NONE, got %d\n", session.Error.erfOper);
    ok(session.Error.erfType == 0, "Expected 0, got %d\n", session.Error.erfType);
    ok(session.Error.fError == FALSE, "Expected FALSE, got %d\n", session.Error.fError);
    ok(session.FileCount == 4, "Expected 4, got %d\n", session.FileCount);
    ok(session.Operation == EXTRACT_EXTRACTFILES,
       "Expected EXTRACT_EXTRACTFILES, got %d\n", session.Operation);
    ok(!lstrcmpA(session.Destination, "dest"), "Expected dest, got %s\n", session.Destination);
    ok(!lstrcmpA(session.CurrentFile, "dest\\testdir\\d.txt"),
       "Expected dest\\testdir\\d.txt, got %s\n", session.CurrentFile);
    ok(!*session.Reserved, "Expected empty string, got %s\n", session.Reserved);
    ok(!session.FilterList, "Expected empty filter list\n");
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(!DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to not exist\n");
    ok(check_list(&node, "testdir\\d.txt", FALSE), "list entry wrong\n");
    ok(!check_list(&node, "testdir\\c.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "b.txt", FALSE), "list entry wrong\n");
    ok(!check_list(&node, "a.txt", FALSE), "list entry wrong\n");
    free_file_list(&session);

    session.Operation = EXTRACT_FILLFILELIST;
    session.FileList = NULL;
    res = pExtract(&session, "extract.cab");
    node = session.FileList;
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(session.FileSize == 40, "Expected 40, got %d\n", session.FileSize);
    ok(session.Error.erfOper == FDIERROR_NONE,
       "Expected FDIERROR_NONE, got %d\n", session.Error.erfOper);
    ok(session.Error.erfType == 0, "Expected 0, got %d\n", session.Error.erfType);
    ok(session.Error.fError == FALSE, "Expected FALSE, got %d\n", session.Error.fError);
    ok(session.FileCount == 8, "Expected 8, got %d\n", session.FileCount);
    ok(session.Operation == EXTRACT_FILLFILELIST,
       "Expected EXTRACT_FILLFILELIST, got %d\n", session.Operation);
    ok(!lstrcmpA(session.Destination, "dest"), "Expected dest, got %s\n", session.Destination);
    ok(!lstrcmpA(session.CurrentFile, "dest\\testdir\\d.txt"),
       "Expected dest\\testdir\\d.txt, got %s\n", session.CurrentFile);
    ok(!*session.Reserved, "Expected empty string, got %s\n", session.Reserved);
    ok(!session.FilterList, "Expected empty filter list\n");
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to not exist\n");
    ok(!DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to not exist\n");
    ok(check_list(&node, "testdir\\d.txt", TRUE), "list entry wrong\n");
    ok(!check_list(&node, "testdir\\c.txt", FALSE), "list entry wrong\n");
    ok(!check_list(&node, "b.txt", FALSE), "list entry wrong\n");
    ok(!check_list(&node, "a.txt", FALSE), "list entry wrong\n");

    session.Operation = 0;
    res = pExtract(&session, "extract.cab");
    node = session.FileList;
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(session.FileSize == 40, "Expected 40, got %d\n", session.FileSize);
    ok(session.Error.erfOper == FDIERROR_NONE,
       "Expected FDIERROR_NONE, got %d\n", session.Error.erfOper);
    ok(session.Error.erfType == 0, "Expected 0, got %d\n", session.Error.erfType);
    ok(session.Error.fError == FALSE, "Expected FALSE, got %d\n", session.Error.fError);
    ok(session.FileCount == 8, "Expected 8, got %d\n", session.FileCount);
    ok(session.Operation == 0, "Expected 0, got %d\n", session.Operation);
    ok(!lstrcmpA(session.Destination, "dest"), "Expected dest, got %s\n", session.Destination);
    ok(!lstrcmpA(session.CurrentFile, "dest\\testdir\\d.txt"),
       "Expected dest\\testdir\\d.txt, got %s\n", session.CurrentFile);
    ok(!*session.Reserved, "Expected empty string, got %s\n", session.Reserved);
    ok(!session.FilterList, "Expected empty filter list\n");
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(!DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to exist\n");
    ok(!DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to exist\n");
    ok(check_list(&node, "testdir\\d.txt", TRUE), "list entry wrong\n");
    ok(check_list(&node, "testdir\\c.txt", TRUE), "list entry wrong\n");
    ok(check_list(&node, "b.txt", TRUE), "list entry wrong\n");
    ok(check_list(&node, "a.txt", TRUE), "list entry wrong\n");

    session.Operation = 0;
    session.FilterList = session.FileList;
    res = pExtract(&session, "extract.cab");
    node = session.FileList;
    ok(res == S_OK, "Expected S_OK, got %ld\n", res);
    ok(session.FileSize == 40, "Expected 40, got %d\n", session.FileSize);
    ok(session.Error.erfOper == FDIERROR_NONE,
       "Expected FDIERROR_NONE, got %d\n", session.Error.erfOper);
    ok(session.Error.erfType == 0, "Expected 0, got %d\n", session.Error.erfType);
    ok(session.Error.fError == FALSE, "Expected FALSE, got %d\n", session.Error.fError);
    ok(session.FileCount == 8, "Expected 8, got %d\n", session.FileCount);
    ok(session.Operation == 0, "Expected 0, got %d\n", session.Operation);
    ok(!lstrcmpA(session.Destination, "dest"), "Expected dest, got %s\n", session.Destination);
    ok(!lstrcmpA(session.CurrentFile, "dest\\testdir\\d.txt"),
       "Expected dest\\testdir\\d.txt, got %s\n", session.CurrentFile);
    ok(!*session.Reserved, "Expected empty string, got %s\n", session.Reserved);
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to exist\n");
    ok(DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to exist\n");
    ok(DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to exist\n");
    ok(check_list(&node, "testdir\\d.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "testdir\\c.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "b.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "a.txt", FALSE), "list entry wrong\n");
    node = session.FilterList;
    ok(check_list(&node, "testdir\\d.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "testdir\\c.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "b.txt", FALSE), "list entry wrong\n");
    ok(check_list(&node, "a.txt", FALSE), "list entry wrong\n");
    free_file_list(&session);

    /* cabinet does not exist */
    ZeroMemory(&session, sizeof(SESSION));
    lstrcpyA(session.Destination, "dest");
    session.Operation = EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES;
    res = pExtract(&session, "nonexistent.cab");
    node = session.FileList;
    ok(res == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %08lx\n", res);
    ok(session.Error.erfOper == FDIERROR_CABINET_NOT_FOUND,
       "Expected FDIERROR_CABINET_NOT_FOUND, got %d\n", session.Error.erfOper);
    ok(session.FileSize == 0, "Expected 0, got %d\n", session.FileSize);
    ok(session.Error.erfType == 0, "Expected 0, got %d\n", session.Error.erfType);
    ok(session.Error.fError == TRUE, "Expected TRUE, got %d\n", session.Error.fError);
    ok(session.FileCount == 0, "Expected 0, got %d\n", session.FileCount);
    ok(session.Operation == (EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES),
       "Expected EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES, got %d\n", session.Operation);
    ok(!lstrcmpA(session.Destination, "dest"), "Expected dest, got %s\n", session.Destination);
    ok(!*session.CurrentFile, "Expected empty string, got %s\n", session.CurrentFile);
    ok(!*session.Reserved, "Expected empty string, got %s\n", session.Reserved);
    ok(!session.FilterList, "Expected empty filter list\n");
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to not exist\n");
    ok(!DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to not exist\n");
    ok(!check_list(&node, "testdir\\d.txt", FALSE), "list entry should not exist\n");
    ok(!check_list(&node, "testdir\\c.txt", FALSE), "list entry should not exist\n");
    ok(!check_list(&node, "b.txt", FALSE), "list entry should not exist\n");
    ok(!check_list(&node, "a.txt", FALSE), "list entry should not exist\n");
    free_file_list(&session);

    /* first file exists but is read-only */
    createTestFile("dest\\a.txt");
    SetFileAttributesA("dest\\a.txt", FILE_ATTRIBUTE_READONLY);
    ok(getFileSize("dest\\a.txt") == 11, "Expected dest\\a.txt to be 11 bytes\n");
    ZeroMemory(&session, sizeof(SESSION));
    lstrcpyA(session.Destination, "dest");
    session.Operation = EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES;
    res = pExtract(&session, "extract.cab");
    node = session.FileList;
    ok(res == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) || res == E_FAIL,
       "Expected HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) or E_FAIL, got %08lx\n", res);
    ok(session.FileSize == 6, "Expected 6, got %d\n", session.FileSize);
    ok(session.Error.erfOper == FDIERROR_USER_ABORT,
       "Expected FDIERROR_USER_ABORT, got %d\n", session.Error.erfOper);
    ok(session.Error.fError == TRUE, "Expected TRUE, got %d\n", session.Error.fError);
    ok(session.FileCount == 1, "Expected 1, got %d\n", session.FileCount);
    ok(!lstrcmpA(session.CurrentFile, "dest\\a.txt"),
       "Expected dest\\a.txt, got %s\n", session.CurrentFile);
    ok(session.Error.erfType == 0, "Expected 0, got %d\n", session.Error.erfType);
    ok(session.Operation == (EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES),
       "Expected EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES, got %d\n", session.Operation);
    ok(!lstrcmpA(session.Destination, "dest"), "Expected dest, got %s\n", session.Destination);
    ok(!*session.Reserved, "Expected empty string, got %s\n", session.Reserved);
    ok(!session.FilterList, "Expected empty filter list\n");
    ok(getFileSize("dest\\a.txt") == 11, "Expected dest\\a.txt to be 11 bytes\n");
    ok(!DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to be read-only\n");
    ok(!DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to not exist\n");
    ok(!DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to not exist\n");
    ok(!check_list(&node, "testdir\\d.txt", FALSE), "list entry should not exist\n");
    ok(!check_list(&node, "testdir\\c.txt", FALSE), "list entry should not exist\n");
    ok(!check_list(&node, "b.txt", FALSE), "list entry should not exist\n");
    ok(!check_list(&node, "a.txt", FALSE), "list entry should not exist\n");
    free_file_list(&session);

    SetFileAttributesA("dest\\a.txt", FILE_ATTRIBUTE_NORMAL);
    DeleteFileA("dest\\a.txt");

    /* first file exists and is writable, third file exists but is read-only */
    createTestFile("dest\\a.txt");
    createTestFile("dest\\testdir\\c.txt");
    SetFileAttributesA("dest\\testdir\\c.txt", FILE_ATTRIBUTE_READONLY);
    ZeroMemory(&session, sizeof(SESSION));
    lstrcpyA(session.Destination, "dest");
    session.Operation = EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES;
    res = pExtract(&session, "extract.cab");
    node = session.FileList;
    ok(res == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) || res == E_FAIL,
       "Expected HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) or E_FAIL, got %08lx\n", res);
    ok(session.FileSize == 26, "Expected 26, got %d\n", session.FileSize);
    ok(session.Error.erfOper == FDIERROR_USER_ABORT,
       "Expected FDIERROR_USER_ABORT, got %d\n", session.Error.erfOper);
    ok(session.Error.fError == TRUE, "Expected TRUE, got %d\n", session.Error.fError);
    ok(session.FileCount == 3, "Expected 3, got %d\n", session.FileCount);
    ok(!lstrcmpA(session.CurrentFile, "dest\\testdir\\c.txt"),
       "Expected dest\\c.txt, got %s\n", session.CurrentFile);
    ok(session.Error.erfType == 0, "Expected 0, got %d\n", session.Error.erfType);
    ok(session.Operation == (EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES),
       "Expected EXTRACT_FILLFILELIST | EXTRACT_EXTRACTFILES, got %d\n", session.Operation);
    ok(!lstrcmpA(session.Destination, "dest"), "Expected dest, got %s\n", session.Destination);
    ok(!*session.Reserved, "Expected empty string, got %s\n", session.Reserved);
    ok(!session.FilterList, "Expected empty filter list\n");
    ok(getFileSize("dest\\a.txt") == 6, "Expected dest\\a.txt to be 6 bytes\n");
    ok(DeleteFileA("dest\\a.txt"), "Expected dest\\a.txt to exist\n");
    ok(DeleteFileA("dest\\b.txt"), "Expected dest\\b.txt to exist\n");
    ok(!DeleteFileA("dest\\testdir\\c.txt"), "Expected dest\\testdir\\c.txt to be read-only\n");
    ok(!DeleteFileA("dest\\testdir\\d.txt"), "Expected dest\\testdir\\d.txt to not exist\n");
    ok(!check_list(&node, "testdir\\d.txt", FALSE), "list entry should not exist\n");
    ok(!check_list(&node, "testdir\\c.txt", FALSE), "list entry wrong\n");
    ok(!check_list(&node, "b.txt", FALSE), "list entry wrong\n");
    ok(!check_list(&node, "a.txt", TRUE), "list entry wrong\n");
    free_file_list(&session);

    SetFileAttributesA("dest\\testdir\\c.txt", FILE_ATTRIBUTE_NORMAL);
    DeleteFileA("dest\\testdir\\c.txt");

    ok(RemoveDirectoryA("dest\\testdir"), "Expected dest\\testdir to exist\n");
    ok(RemoveDirectoryA("dest"), "Expected dest to exist\n");
}

START_TEST(extract)
{
    init_function_pointers();
    create_test_files();
    create_cab_file();

    test_Extract();

    delete_test_files();
}
