/*
 * Advpack file functions
 *
 * Copyright 2006 James Hawkins
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
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winnls.h"
#include "winver.h"
#include "winternl.h"
#include "setupapi.h"
#include "advpub.h"
#include "fdi.h"
#include "wine/debug.h"
#include "advpack_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(advpack);

/* converts an ansi double null-terminated list to a unicode list */
static LPWSTR ansi_to_unicode_list(LPCSTR ansi_list)
{
    DWORD len, wlen = 0;
    LPWSTR list;
    LPCSTR ptr = ansi_list;

    while (*ptr) ptr += lstrlenA(ptr) + 1;
    len = ptr + 1 - ansi_list;
    wlen = MultiByteToWideChar(CP_ACP, 0, ansi_list, len, NULL, 0);
    list = HeapAlloc(GetProcessHeap(), 0, wlen * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ansi_list, len, list, wlen);
    return list;
}

/***********************************************************************
 *      AddDelBackupEntryA (ADVPACK.@)
 *
 * See AddDelBackupEntryW.
 */
HRESULT WINAPI AddDelBackupEntryA(LPCSTR lpcszFileList, LPCSTR lpcszBackupDir,
                                  LPCSTR lpcszBaseName, DWORD dwFlags)
{
    UNICODE_STRING backupdir, basename;
    LPWSTR filelist;
    LPCWSTR backup;
    HRESULT res;

    TRACE("(%s, %s, %s, %d)\n", debugstr_a(lpcszFileList),
          debugstr_a(lpcszBackupDir), debugstr_a(lpcszBaseName), dwFlags);

    if (lpcszFileList)
        filelist = ansi_to_unicode_list(lpcszFileList);
    else
        filelist = NULL;

    RtlCreateUnicodeStringFromAsciiz(&backupdir, lpcszBackupDir);
    RtlCreateUnicodeStringFromAsciiz(&basename, lpcszBaseName);

    if (lpcszBackupDir)
        backup = backupdir.Buffer;
    else
        backup = NULL;

    res = AddDelBackupEntryW(filelist, backup, basename.Buffer, dwFlags);

    HeapFree(GetProcessHeap(), 0, filelist);

    RtlFreeUnicodeString(&backupdir);
    RtlFreeUnicodeString(&basename);

    return res;
}

/***********************************************************************
 *      AddDelBackupEntryW (ADVPACK.@)
 *
 * Either appends the files in the file list to the backup section of
 * the specified INI, or deletes the entries from the INI file.
 *
 * PARAMS
 *   lpcszFileList  [I] NULL-separated list of filenames.
 *   lpcszBackupDir [I] Path of the backup directory.
 *   lpcszBaseName  [I] Basename of the INI file.
 *   dwFlags        [I] AADBE_ADD_ENTRY adds the entries in the file list
 *                      to the INI file, while AADBE_DEL_ENTRY removes
 *                      the entries from the INI file.
 *
 * RETURNS
 *   S_OK in all cases.
 *
 * NOTES
 *   If the INI file does not exist before adding entries to it, the file
 *   will be created.
 * 
 *   If lpcszBackupDir is NULL, the INI file is assumed to exist in
 *   c:\windows or created there if it does not exist.
 */
HRESULT WINAPI AddDelBackupEntryW(LPCWSTR lpcszFileList, LPCWSTR lpcszBackupDir,
                                  LPCWSTR lpcszBaseName, DWORD dwFlags)
{
    WCHAR szIniPath[MAX_PATH];
    LPCWSTR szString = NULL;

    static const WCHAR szBackupEntry[] = {
        '-','1',',','0',',','0',',','0',',','0',',','0',',','-','1',0
    };
    
    static const WCHAR backslash[] = {'\\',0};
    static const WCHAR ini[] = {'.','i','n','i',0};
    static const WCHAR backup[] = {'b','a','c','k','u','p',0};

    TRACE("(%s, %s, %s, %d)\n", debugstr_w(lpcszFileList),
          debugstr_w(lpcszBackupDir), debugstr_w(lpcszBaseName), dwFlags);

    if (!lpcszFileList || !*lpcszFileList)
        return S_OK;

    if (lpcszBackupDir)
        lstrcpyW(szIniPath, lpcszBackupDir);
    else
        GetWindowsDirectoryW(szIniPath, MAX_PATH);

    lstrcatW(szIniPath, backslash);
    lstrcatW(szIniPath, lpcszBaseName);
    lstrcatW(szIniPath, ini);

    SetFileAttributesW(szIniPath, FILE_ATTRIBUTE_NORMAL);

    if (dwFlags & AADBE_ADD_ENTRY)
        szString = szBackupEntry;
    else if (dwFlags & AADBE_DEL_ENTRY)
        szString = NULL;

    /* add or delete the INI entries */
    while (*lpcszFileList)
    {
        WritePrivateProfileStringW(backup, lpcszFileList, szString, szIniPath);
        lpcszFileList += lstrlenW(lpcszFileList) + 1;
    }

    /* hide the INI file */
    SetFileAttributesW(szIniPath, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN);

    return S_OK;
}

/* FIXME: this is only for the local case, X:\ */
#define ROOT_LENGTH 3

static UINT CALLBACK pQuietQueueCallback(PVOID Context, UINT Notification,
                                         UINT_PTR Param1, UINT_PTR Param2)
{
    return 1;
}

static UINT CALLBACK pQueueCallback(PVOID Context, UINT Notification,
                                    UINT_PTR Param1, UINT_PTR Param2)
{
    /* only be verbose for error notifications */
    if (!Notification ||
        Notification == SPFILENOTIFY_RENAMEERROR ||
        Notification == SPFILENOTIFY_DELETEERROR ||
        Notification == SPFILENOTIFY_COPYERROR)
    {
        return SetupDefaultQueueCallbackW(Context, Notification,
                                          Param1, Param2);
    }

    return 1;
}

/***********************************************************************
 *      AdvInstallFileA (ADVPACK.@)
 *
 * See AdvInstallFileW.
 */
HRESULT WINAPI AdvInstallFileA(HWND hwnd, LPCSTR lpszSourceDir, LPCSTR lpszSourceFile,
                               LPCSTR lpszDestDir, LPCSTR lpszDestFile,
                               DWORD dwFlags, DWORD dwReserved)
{
    UNICODE_STRING sourcedir, sourcefile;
    UNICODE_STRING destdir, destfile;
    HRESULT res;

    TRACE("(%p, %s, %s, %s, %s, %d, %d)\n", hwnd, debugstr_a(lpszSourceDir),
          debugstr_a(lpszSourceFile), debugstr_a(lpszDestDir),
          debugstr_a(lpszDestFile), dwFlags, dwReserved);

    if (!lpszSourceDir || !lpszSourceFile || !lpszDestDir)
        return E_INVALIDARG;

    RtlCreateUnicodeStringFromAsciiz(&sourcedir, lpszSourceDir);
    RtlCreateUnicodeStringFromAsciiz(&sourcefile, lpszSourceFile);
    RtlCreateUnicodeStringFromAsciiz(&destdir, lpszDestDir);
    RtlCreateUnicodeStringFromAsciiz(&destfile, lpszDestFile);

    res = AdvInstallFileW(hwnd, sourcedir.Buffer, sourcefile.Buffer,
                          destdir.Buffer, destfile.Buffer, dwFlags, dwReserved);

    RtlFreeUnicodeString(&sourcedir);
    RtlFreeUnicodeString(&sourcefile);
    RtlFreeUnicodeString(&destdir);
    RtlFreeUnicodeString(&destfile);

    return res;
}

/***********************************************************************
 *      AdvInstallFileW (ADVPACK.@)
 *
 * Copies a file from the source to a destination.
 *
 * PARAMS
 *   hwnd           [I] Handle to the window used for messages.
 *   lpszSourceDir  [I] Source directory.
 *   lpszSourceFile [I] Source filename.
 *   lpszDestDir    [I] Destination directory.
 *   lpszDestFile   [I] Optional destination filename.
 *   dwFlags        [I] See advpub.h.
 *   dwReserved     [I] Reserved.  Must be 0.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * NOTES
 *   If lpszDestFile is NULL, the destination filename is the same as
 *   lpszSourceFIle.
 */
HRESULT WINAPI AdvInstallFileW(HWND hwnd, LPCWSTR lpszSourceDir, LPCWSTR lpszSourceFile,
                               LPCWSTR lpszDestDir, LPCWSTR lpszDestFile,
                               DWORD dwFlags, DWORD dwReserved)
{
    PSP_FILE_CALLBACK_W pFileCallback;
    LPWSTR szDestFilename;
    LPCWSTR szPath;
    WCHAR szRootPath[ROOT_LENGTH];
    DWORD dwLen, dwLastError;
    HSPFILEQ fileQueue;
    PVOID pContext;

    TRACE("(%p, %s, %s, %s, %s, %d, %d)\n", hwnd, debugstr_w(lpszSourceDir),
          debugstr_w(lpszSourceFile), debugstr_w(lpszDestDir),
          debugstr_w(lpszDestFile), dwFlags, dwReserved);

    if (!lpszSourceDir || !lpszSourceFile || !lpszDestDir)
        return E_INVALIDARG;
        
    fileQueue = SetupOpenFileQueue();
    if (fileQueue == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    pContext = NULL;
    dwLastError = ERROR_SUCCESS;

    lstrcpynW(szRootPath, lpszSourceDir, ROOT_LENGTH);
    szPath = lpszSourceDir + ROOT_LENGTH;

    /* use lpszSourceFile as destination filename if lpszDestFile is NULL */
    if (lpszDestFile)
    {
        dwLen = lstrlenW(lpszDestFile);
        szDestFilename = HeapAlloc(GetProcessHeap(), 0, (dwLen+1) * sizeof(WCHAR));
        lstrcpyW(szDestFilename, lpszDestFile);
    }
    else
    {
        dwLen = lstrlenW(lpszSourceFile);
        szDestFilename = HeapAlloc(GetProcessHeap(), 0, (dwLen+1) * sizeof(WCHAR));
        lstrcpyW(szDestFilename, lpszSourceFile);
    }

    /* add the file copy operation to the setup queue */
    if (!SetupQueueCopyW(fileQueue, szRootPath, szPath, lpszSourceFile, NULL,
                         NULL, lpszDestDir, szDestFilename, dwFlags))
    {
        dwLastError = GetLastError();
        goto done;
    }

    pContext = SetupInitDefaultQueueCallbackEx(hwnd, INVALID_HANDLE_VALUE,
                                               0, 0, NULL);
    if (!pContext)
    {
        dwLastError = GetLastError();
        goto done;
    }

    /* don't output anything for AIF_QUIET */
    if (dwFlags & AIF_QUIET)
        pFileCallback = pQuietQueueCallback;
    else
        pFileCallback = pQueueCallback;

    /* perform the file copy */
    if (!SetupCommitFileQueueW(hwnd, fileQueue, pFileCallback, pContext))
    {
        dwLastError = GetLastError();
        goto done;
    }

done:
    SetupTermDefaultQueueCallback(pContext);
    SetupCloseFileQueue(fileQueue);
    
    HeapFree(GetProcessHeap(), 0, szDestFilename);
    
    return HRESULT_FROM_WIN32(dwLastError);
}

static HRESULT DELNODE_recurse_dirtree(LPWSTR fname, DWORD flags)
{
    DWORD fattrs = GetFileAttributesW(fname);
    HRESULT ret = E_FAIL;

    static const WCHAR asterisk[] = {'*',0};
    static const WCHAR dot[] = {'.',0};
    static const WCHAR dotdot[] = {'.','.',0};

    if (fattrs & FILE_ATTRIBUTE_DIRECTORY)
    {
        HANDLE hFindFile;
        WIN32_FIND_DATAW w32fd;
        BOOL done = TRUE;
        int fname_len = lstrlenW(fname);
#ifdef __REACTOS__
        if (flags & ADN_DEL_IF_EMPTY)
            goto deleteinitialdirectory;
#endif

        /* Generate a path with wildcard suitable for iterating */
        if (fname_len && fname[fname_len-1] != '\\') fname[fname_len++] = '\\';
        lstrcpyW(fname + fname_len, asterisk);

        if ((hFindFile = FindFirstFileW(fname, &w32fd)) != INVALID_HANDLE_VALUE)
        {
            /* Iterate through the files in the directory */
            for (done = FALSE; !done; done = !FindNextFileW(hFindFile, &w32fd))
            {
                TRACE("%s\n", debugstr_w(w32fd.cFileName));
                if (lstrcmpW(dot, w32fd.cFileName) != 0 &&
                    lstrcmpW(dotdot, w32fd.cFileName) != 0)
                {
                    lstrcpyW(fname + fname_len, w32fd.cFileName);
                    if (DELNODE_recurse_dirtree(fname, flags) != S_OK)
                    {
                        break; /* Failure */
                    }
                }
            }
            FindClose(hFindFile);
        }

        /* We're done with this directory, so restore the old path without wildcard */
        *(fname + fname_len) = '\0';

        if (done)
        {
#ifdef __REACTOS__
deleteinitialdirectory:
#endif
            TRACE("%s: directory\n", debugstr_w(fname));
#ifdef __REACTOS__
            SetFileAttributesW(fname, FILE_ATTRIBUTE_NORMAL);
            if (RemoveDirectoryW(fname))
#else
            if (SetFileAttributesW(fname, FILE_ATTRIBUTE_NORMAL) && RemoveDirectoryW(fname))
#endif
            {
                ret = S_OK;
            }
        }
    }
    else
    {
        TRACE("%s: file\n", debugstr_w(fname));
#ifdef __REACTOS__
        SetFileAttributesW(fname, FILE_ATTRIBUTE_NORMAL);
        if (DeleteFileW(fname))
#else
        if (SetFileAttributesW(fname, FILE_ATTRIBUTE_NORMAL) && DeleteFileW(fname))
#endif
        {
            ret = S_OK;
        }
    }
    
    return ret;
}

/***********************************************************************
 *              DelNodeA   (ADVPACK.@)
 *
 * See DelNodeW.
 */
HRESULT WINAPI DelNodeA(LPCSTR pszFileOrDirName, DWORD dwFlags)
{
    UNICODE_STRING fileordirname;
    HRESULT res;

    TRACE("(%s, %d)\n", debugstr_a(pszFileOrDirName), dwFlags);

    RtlCreateUnicodeStringFromAsciiz(&fileordirname, pszFileOrDirName);

    res = DelNodeW(fileordirname.Buffer, dwFlags);

    RtlFreeUnicodeString(&fileordirname);

    return res;
}

/***********************************************************************
 *              DelNodeW   (ADVPACK.@)
 *
 * Deletes a file or directory
 *
 * PARAMS
 *   pszFileOrDirName   [I] Name of file or directory to delete
 *   dwFlags            [I] Flags; see include/advpub.h
 *
 * RETURNS 
 *   Success: S_OK
 *   Failure: E_FAIL
 *
 * BUGS
 *   - Ignores flags
 *   - Native version apparently does a lot of checking to make sure
 *     we're not trying to delete a system directory etc.
 */
HRESULT WINAPI DelNodeW(LPCWSTR pszFileOrDirName, DWORD dwFlags)
{
    WCHAR fname[MAX_PATH];
    HRESULT ret = E_FAIL;
    
    TRACE("(%s, %d)\n", debugstr_w(pszFileOrDirName), dwFlags);
    
#ifdef __REACTOS__
    if (dwFlags & ~ADN_DEL_IF_EMPTY)
        FIXME("Flags %#x ignored!\n", dwFlags & ~ADN_DEL_IF_EMPTY);
#else
    if (dwFlags)
        FIXME("Flags ignored!\n");
#endif

    if (pszFileOrDirName && *pszFileOrDirName)
    {
        lstrcpyW(fname, pszFileOrDirName);

        /* TODO: Should check for system directory deletion etc. here */

        ret = DELNODE_recurse_dirtree(fname, dwFlags);
    }

    return ret;
}

/***********************************************************************
 *             DelNodeRunDLL32A   (ADVPACK.@)
 *
 * See DelNodeRunDLL32W.
 */
HRESULT WINAPI DelNodeRunDLL32A(HWND hWnd, HINSTANCE hInst, LPSTR cmdline, INT show)
{
    UNICODE_STRING params;
    HRESULT hr;

    TRACE("(%p, %p, %s, %i)\n", hWnd, hInst, debugstr_a(cmdline), show);

    RtlCreateUnicodeStringFromAsciiz(&params, cmdline);

    hr = DelNodeRunDLL32W(hWnd, hInst, params.Buffer, show);

    RtlFreeUnicodeString(&params);

    return hr;
}

/***********************************************************************
 *             DelNodeRunDLL32W   (ADVPACK.@)
 *
 * Deletes a file or directory, WinMain style.
 *
 * PARAMS
 *   hWnd    [I] Handle to the window used for the display.
 *   hInst   [I] Instance of the process.
 *   cmdline [I] Contains parameters in the order FileOrDirName,Flags.
 *   show    [I] How the window should be shown.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 */
HRESULT WINAPI DelNodeRunDLL32W(HWND hWnd, HINSTANCE hInst, LPWSTR cmdline, INT show)
{
    LPWSTR szFilename, szFlags;
    LPWSTR cmdline_copy, cmdline_ptr;
    DWORD dwFlags = 0;
    HRESULT res;

    TRACE("(%p, %p, %s, %i)\n", hWnd, hInst, debugstr_w(cmdline), show);

    cmdline_copy = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(cmdline) + 1) * sizeof(WCHAR));
    cmdline_ptr = cmdline_copy;
    lstrcpyW(cmdline_copy, cmdline);

    /* get the parameters at indexes 0 and 1 respectively */
    szFilename = get_parameter(&cmdline_ptr, ',', TRUE);
    szFlags = get_parameter(&cmdline_ptr, ',', TRUE);

    if (szFlags)
        dwFlags = wcstol(szFlags, NULL, 10);

    res = DelNodeW(szFilename, dwFlags);

    HeapFree(GetProcessHeap(), 0, cmdline_copy);

    return res;
}

/* The following definitions were copied from dlls/cabinet/cabinet.h */

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

static HRESULT (WINAPI *pExtract)(SESSION*, LPCSTR);

/* removes legal characters before and after file list, and
 * converts the file list to a NULL-separated list
 */
static LPSTR convert_file_list(LPCSTR FileList, DWORD *dwNumFiles)
{
    DWORD dwLen;
    const char *first = FileList;
    const char *last = FileList + strlen(FileList) - 1;
    LPSTR szConvertedList, temp;
    
    /* any number of these chars before the list is OK */
    while (first < last && (*first == ' ' || *first == '\t' || *first == ':'))
        first++;

    /* any number of these chars after the list is OK */
    while (last > first && (*last == ' ' || *last == '\t' || *last == ':'))
        last--;

    if (first == last)
        return NULL;

    dwLen = last - first + 3; /* room for double-null termination */
    szConvertedList = HeapAlloc(GetProcessHeap(), 0, dwLen);
    lstrcpynA(szConvertedList, first, dwLen - 1);
    szConvertedList[dwLen - 1] = '\0';

    /* empty list */
    if (!szConvertedList[0])
    {
        HeapFree(GetProcessHeap(), 0, szConvertedList);
        return NULL;
    }
        
    *dwNumFiles = 1;

    /* convert the colons to double-null termination */
    temp = szConvertedList;
    while (*temp)
    {
        if (*temp == ':')
        {
            *temp = '\0';
            (*dwNumFiles)++;
        }

        temp++;
    }

    return szConvertedList;
}

static void free_file_node(struct FILELIST *pNode)
{
    HeapFree(GetProcessHeap(), 0, pNode->FileName);
    HeapFree(GetProcessHeap(), 0, pNode);
}

/* determines whether szFile is in the NULL-separated szFileList */
static BOOL file_in_list(LPCSTR szFile, LPCSTR szFileList)
{
    DWORD dwLen = lstrlenA(szFile);
    DWORD dwTestLen;

    while (*szFileList)
    {
        dwTestLen = lstrlenA(szFileList);

        if (dwTestLen == dwLen)
        {
            if (!lstrcmpiA(szFile, szFileList))
                return TRUE;
        }

        szFileList += dwTestLen + 1;
    }

    return FALSE;
}


/* returns the number of files that are in both the linked list and szFileList */
static DWORD fill_file_list(SESSION *session, LPCSTR szCabName, LPCSTR szFileList)
{
    DWORD dwNumFound = 0;
    struct FILELIST *pNode;

    session->Operation |= EXTRACT_FILLFILELIST;
    if (pExtract(session, szCabName) != S_OK)
    {
        session->Operation &= ~EXTRACT_FILLFILELIST;
        return -1;
    }

    pNode = session->FileList;
    while (pNode)
    {
        if (!file_in_list(pNode->FileName, szFileList))
            pNode->DoExtract = FALSE;
        else
            dwNumFound++;

        pNode = pNode->next;
    }

    session->Operation &= ~EXTRACT_FILLFILELIST;
    return dwNumFound;
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
}

/***********************************************************************
 *             ExtractFilesA    (ADVPACK.@)
 *
 * Extracts the specified files from a cab archive into
 * a destination directory.
 *
 * PARAMS
 *   CabName   [I] Filename of the cab archive.
 *   ExpandDir [I] Destination directory for the extracted files.
 *   Flags     [I] Reserved.
 *   FileList  [I] Optional list of files to extract.  See NOTES.
 *   LReserved [I] Reserved.  Must be NULL.
 *   Reserved  [I] Reserved.  Must be 0.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * NOTES
 *   FileList is a colon-separated list of filenames.  If FileList is
 *   non-NULL, only the files in the list will be extracted from the
 *   cab file, otherwise all files will be extracted.  Any number of
 *   spaces, tabs, or colons can be before or after the list, but
 *   the list itself must only be separated by colons.
 */
HRESULT WINAPI ExtractFilesA(LPCSTR CabName, LPCSTR ExpandDir, DWORD Flags,
                             LPCSTR FileList, LPVOID LReserved, DWORD Reserved)
{   
    SESSION session;
    HMODULE hCabinet;
    HRESULT res = S_OK;
    DWORD dwFileCount = 0;
    DWORD dwFilesFound = 0;
    LPSTR szConvertedList = NULL;

    TRACE("(%s, %s, %d, %s, %p, %d)\n", debugstr_a(CabName), debugstr_a(ExpandDir),
          Flags, debugstr_a(FileList), LReserved, Reserved);

    if (!CabName || !ExpandDir)
        return E_INVALIDARG;

    if (GetFileAttributesA(ExpandDir) == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

    hCabinet = LoadLibraryA("cabinet.dll");
    if (!hCabinet)
        return E_FAIL;

    ZeroMemory(&session, sizeof(SESSION));

    pExtract = (void *)GetProcAddress(hCabinet, "Extract");
    if (!pExtract)
    {
        res = E_FAIL;
        goto done;
    }

    lstrcpyA(session.Destination, ExpandDir);

    if (FileList)
    {
        szConvertedList = convert_file_list(FileList, &dwFileCount);
        if (!szConvertedList)
        {
            res = E_FAIL;
            goto done;
        }

        dwFilesFound = fill_file_list(&session, CabName, szConvertedList);
        if (dwFilesFound != dwFileCount)
        {
            res = E_FAIL;
            goto done;
        }
    }
    else
        session.Operation |= EXTRACT_FILLFILELIST;

    session.Operation |= EXTRACT_EXTRACTFILES;
    res = pExtract(&session, CabName);

done:
    free_file_list(&session);
    FreeLibrary(hCabinet);
    HeapFree(GetProcessHeap(), 0, szConvertedList);

    return res;
}

/***********************************************************************
 *             ExtractFilesW    (ADVPACK.@)
 *
 * Extracts the specified files from a cab archive into
 * a destination directory.
 *
 * PARAMS
 *   CabName   [I] Filename of the cab archive.
 *   ExpandDir [I] Destination directory for the extracted files.
 *   Flags     [I] Reserved.
 *   FileList  [I] Optional list of files to extract.  See NOTES.
 *   LReserved [I] Reserved.  Must be NULL.
 *   Reserved  [I] Reserved.  Must be 0.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * NOTES
 *   FileList is a colon-separated list of filenames.  If FileList is
 *   non-NULL, only the files in the list will be extracted from the
 *   cab file, otherwise all files will be extracted.  Any number of
 *   spaces, tabs, or colons can be before or after the list, but
 *   the list itself must only be separated by colons.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI ExtractFilesW(LPCWSTR CabName, LPCWSTR ExpandDir, DWORD Flags,
                             LPCWSTR FileList, LPVOID LReserved, DWORD Reserved)
{
    char *cab_name = NULL, *expand_dir = NULL, *file_list = NULL;
    HRESULT hres = S_OK;

    TRACE("(%s, %s, %d, %s, %p, %d)\n", debugstr_w(CabName), debugstr_w(ExpandDir),
          Flags, debugstr_w(FileList), LReserved, Reserved);

    if(CabName) {
        cab_name = heap_strdupWtoA(CabName);
        if(!cab_name)
            return E_OUTOFMEMORY;
    }

    if(ExpandDir) {
        expand_dir = heap_strdupWtoA(ExpandDir);
        if(!expand_dir)
            hres = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hres) && FileList) {
        file_list = heap_strdupWtoA(FileList);
        if(!file_list)
            hres = E_OUTOFMEMORY;
    }

    /* cabinet.dll, which does the real job of extracting files, doesn't have UNICODE API,
       so we need W->A conversion at some point anyway. */
    if(SUCCEEDED(hres))
        hres = ExtractFilesA(cab_name, expand_dir, Flags, file_list, LReserved, Reserved);

    heap_free(cab_name);
    heap_free(expand_dir);
    heap_free(file_list);
    return hres;
}

/***********************************************************************
 *      FileSaveMarkNotExistA (ADVPACK.@)
 *
 * See FileSaveMarkNotExistW.
 */
HRESULT WINAPI FileSaveMarkNotExistA(LPSTR pszFileList, LPSTR pszDir, LPSTR pszBaseName)
{
    TRACE("(%s, %s, %s)\n", debugstr_a(pszFileList),
          debugstr_a(pszDir), debugstr_a(pszBaseName));

    return AddDelBackupEntryA(pszFileList, pszDir, pszBaseName, AADBE_DEL_ENTRY);
}

/***********************************************************************
 *      FileSaveMarkNotExistW (ADVPACK.@)
 *
 * Marks the files in the file list as not existing so they won't be
 * backed up during a save.
 *
 * PARAMS
 *   pszFileList [I] NULL-separated list of filenames.
 *   pszDir      [I] Path of the backup directory.
 *   pszBaseName [I] Basename of the INI file.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 */
HRESULT WINAPI FileSaveMarkNotExistW(LPWSTR pszFileList, LPWSTR pszDir, LPWSTR pszBaseName)
{
    TRACE("(%s, %s, %s)\n", debugstr_w(pszFileList),
          debugstr_w(pszDir), debugstr_w(pszBaseName));

    return AddDelBackupEntryW(pszFileList, pszDir, pszBaseName, AADBE_DEL_ENTRY);
}

/***********************************************************************
 *      FileSaveRestoreA (ADVPACK.@)
 *
 * See FileSaveRestoreW.
 */
HRESULT WINAPI FileSaveRestoreA(HWND hDlg, LPSTR pszFileList, LPSTR pszDir,
                                LPSTR pszBaseName, DWORD dwFlags)
{
    UNICODE_STRING filelist, dir, basename;
    HRESULT hr;

    TRACE("(%p, %s, %s, %s, %d)\n", hDlg, debugstr_a(pszFileList),
          debugstr_a(pszDir), debugstr_a(pszBaseName), dwFlags);

    RtlCreateUnicodeStringFromAsciiz(&filelist, pszFileList);
    RtlCreateUnicodeStringFromAsciiz(&dir, pszDir);
    RtlCreateUnicodeStringFromAsciiz(&basename, pszBaseName);

    hr = FileSaveRestoreW(hDlg, filelist.Buffer, dir.Buffer,
                          basename.Buffer, dwFlags);

    RtlFreeUnicodeString(&filelist);
    RtlFreeUnicodeString(&dir);
    RtlFreeUnicodeString(&basename);

    return hr;
}                         

/***********************************************************************
 *      FileSaveRestoreW (ADVPACK.@)
 *
 * Saves or restores the files in the specified file list.
 *
 * PARAMS
 *   hDlg        [I] Handle to the dialog used for the display.
 *   pszFileList [I] NULL-separated list of filenames.
 *   pszDir      [I] Path of the backup directory.
 *   pszBaseName [I] Basename of the backup files.
 *   dwFlags     [I] See advpub.h.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * NOTES
 *   If pszFileList is NULL on restore, all files will be restored.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI FileSaveRestoreW(HWND hDlg, LPWSTR pszFileList, LPWSTR pszDir,
                                LPWSTR pszBaseName, DWORD dwFlags)
{
    FIXME("(%p, %s, %s, %s, %d) stub\n", hDlg, debugstr_w(pszFileList),
          debugstr_w(pszDir), debugstr_w(pszBaseName), dwFlags);

    return E_FAIL;
}

/***********************************************************************
 *      FileSaveRestoreOnINFA (ADVPACK.@)
 *
 * See FileSaveRestoreOnINFW.
 */
HRESULT WINAPI FileSaveRestoreOnINFA(HWND hWnd, LPCSTR pszTitle, LPCSTR pszINF,
                                    LPCSTR pszSection, LPCSTR pszBackupDir,
                                    LPCSTR pszBaseBackupFile, DWORD dwFlags)
{
    UNICODE_STRING title, inf, section;
    UNICODE_STRING backupdir, backupfile;
    HRESULT hr;

    TRACE("(%p, %s, %s, %s, %s, %s, %d)\n", hWnd, debugstr_a(pszTitle),
          debugstr_a(pszINF), debugstr_a(pszSection), debugstr_a(pszBackupDir),
          debugstr_a(pszBaseBackupFile), dwFlags);

    RtlCreateUnicodeStringFromAsciiz(&title, pszTitle);
    RtlCreateUnicodeStringFromAsciiz(&inf, pszINF);
    RtlCreateUnicodeStringFromAsciiz(&section, pszSection);
    RtlCreateUnicodeStringFromAsciiz(&backupdir, pszBackupDir);
    RtlCreateUnicodeStringFromAsciiz(&backupfile, pszBaseBackupFile);

    hr = FileSaveRestoreOnINFW(hWnd, title.Buffer, inf.Buffer, section.Buffer,
                               backupdir.Buffer, backupfile.Buffer, dwFlags);

    RtlFreeUnicodeString(&title);
    RtlFreeUnicodeString(&inf);
    RtlFreeUnicodeString(&section);
    RtlFreeUnicodeString(&backupdir);
    RtlFreeUnicodeString(&backupfile);

    return hr;
}

/***********************************************************************
 *      FileSaveRestoreOnINFW (ADVPACK.@)
 *
 *
 * PARAMS
 *   hWnd              [I] Handle to the window used for the display.
 *   pszTitle          [I] Title of the window.
 *   pszINF            [I] Fully-qualified INF filename.
 *   pszSection        [I] GenInstall INF section name.
 *   pszBackupDir      [I] Directory to store the backup file.
 *   pszBaseBackupFile [I] Basename of the backup files.
 *   dwFlags           [I] See advpub.h
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * NOTES
 *   If pszSection is NULL, the default section will be used.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI FileSaveRestoreOnINFW(HWND hWnd, LPCWSTR pszTitle, LPCWSTR pszINF,
                                     LPCWSTR pszSection, LPCWSTR pszBackupDir,
                                     LPCWSTR pszBaseBackupFile, DWORD dwFlags)
{
    FIXME("(%p, %s, %s, %s, %s, %s, %d): stub\n", hWnd, debugstr_w(pszTitle),
          debugstr_w(pszINF), debugstr_w(pszSection), debugstr_w(pszBackupDir),
          debugstr_w(pszBaseBackupFile), dwFlags);

    return E_FAIL;
}

/***********************************************************************
 *             GetVersionFromFileA     (ADVPACK.@)
 *
 * See GetVersionFromFileExW.
 */
HRESULT WINAPI GetVersionFromFileA(LPCSTR Filename, LPDWORD MajorVer,
                                   LPDWORD MinorVer, BOOL Version )
{
    TRACE("(%s, %p, %p, %d)\n", debugstr_a(Filename), MajorVer, MinorVer, Version);
    return GetVersionFromFileExA(Filename, MajorVer, MinorVer, Version);
}

/***********************************************************************
 *             GetVersionFromFileW     (ADVPACK.@)
 *
 * See GetVersionFromFileExW.
 */
HRESULT WINAPI GetVersionFromFileW(LPCWSTR Filename, LPDWORD MajorVer,
                                   LPDWORD MinorVer, BOOL Version )
{
    TRACE("(%s, %p, %p, %d)\n", debugstr_w(Filename), MajorVer, MinorVer, Version);
    return GetVersionFromFileExW(Filename, MajorVer, MinorVer, Version);
}

/* data for GetVersionFromFileEx */
typedef struct tagLANGANDCODEPAGE
{
    WORD wLanguage;
    WORD wCodePage;
} LANGANDCODEPAGE;

/***********************************************************************
 *             GetVersionFromFileExA   (ADVPACK.@)
 *
 * See GetVersionFromFileExW.
 */
HRESULT WINAPI GetVersionFromFileExA(LPCSTR lpszFilename, LPDWORD pdwMSVer,
                                     LPDWORD pdwLSVer, BOOL bVersion )
{
    UNICODE_STRING filename;
    HRESULT res;

    TRACE("(%s, %p, %p, %d)\n", debugstr_a(lpszFilename),
          pdwMSVer, pdwLSVer, bVersion);

    RtlCreateUnicodeStringFromAsciiz(&filename, lpszFilename);

    res = GetVersionFromFileExW(filename.Buffer, pdwMSVer, pdwLSVer, bVersion);

    RtlFreeUnicodeString(&filename);

    return res;
}

/***********************************************************************
 *             GetVersionFromFileExW   (ADVPACK.@)
 *
 * Gets the files version or language information.
 *
 * PARAMS
 *   lpszFilename [I] The file to get the info from.
 *   pdwMSVer     [O] Major version.
 *   pdwLSVer     [O] Minor version.
 *   bVersion     [I] Whether to retrieve version or language info.
 *
 * RETURNS
 *   Always returns S_OK.
 *
 * NOTES
 *   If bVersion is TRUE, version information is retrieved, else
 *   pdwMSVer gets the language ID and pdwLSVer gets the codepage ID.
 */
HRESULT WINAPI GetVersionFromFileExW(LPCWSTR lpszFilename, LPDWORD pdwMSVer,
                                     LPDWORD pdwLSVer, BOOL bVersion )
{
    VS_FIXEDFILEINFO *pFixedVersionInfo;
    LANGANDCODEPAGE *pLangAndCodePage;
    DWORD dwHandle, dwInfoSize;
    WCHAR szWinDir[MAX_PATH];
    WCHAR szFile[MAX_PATH];
    LPVOID pVersionInfo = NULL;
    BOOL bFileCopied = FALSE;
    UINT uValueLen;

    static const WCHAR backslash[] = {'\\',0};
    static const WCHAR translation[] = {
        '\\','V','a','r','F','i','l','e','I','n','f','o',
        '\\','T','r','a','n','s','l','a','t','i','o','n',0
    };

    TRACE("(%s, %p, %p, %d)\n", debugstr_w(lpszFilename),
          pdwMSVer, pdwLSVer, bVersion);

    *pdwLSVer = 0;
    *pdwMSVer = 0;

    lstrcpynW(szFile, lpszFilename, MAX_PATH);

    dwInfoSize = GetFileVersionInfoSizeW(szFile, &dwHandle);
    if (!dwInfoSize)
    {
        /* check that the file exists */
        if (GetFileAttributesW(szFile) == INVALID_FILE_ATTRIBUTES)
            return S_OK;

        /* file exists, but won't be found by GetFileVersionInfoSize,
        * so copy it to the temp dir where it will be found.
        */
        GetWindowsDirectoryW(szWinDir, MAX_PATH);
        GetTempFileNameW(szWinDir, NULL, 0, szFile);
        CopyFileW(lpszFilename, szFile, FALSE);
        bFileCopied = TRUE;

        dwInfoSize = GetFileVersionInfoSizeW(szFile, &dwHandle);
        if (!dwInfoSize)
            goto done;
    }

    pVersionInfo = HeapAlloc(GetProcessHeap(), 0, dwInfoSize);
    if (!pVersionInfo)
        goto done;

    if (!GetFileVersionInfoW(szFile, dwHandle, dwInfoSize, pVersionInfo))
        goto done;

    if (bVersion)
    {
        if (!VerQueryValueW(pVersionInfo, backslash,
            (LPVOID *)&pFixedVersionInfo, &uValueLen))
            goto done;

        if (!uValueLen)
            goto done;

        *pdwMSVer = pFixedVersionInfo->dwFileVersionMS;
        *pdwLSVer = pFixedVersionInfo->dwFileVersionLS;
    }
    else
    {
        if (!VerQueryValueW(pVersionInfo, translation,
             (LPVOID *)&pLangAndCodePage, &uValueLen))
            goto done;

        if (!uValueLen)
            goto done;

        *pdwMSVer = pLangAndCodePage->wLanguage;
        *pdwLSVer = pLangAndCodePage->wCodePage;
    }

done:
    HeapFree(GetProcessHeap(), 0, pVersionInfo);

    if (bFileCopied)
        DeleteFileW(szFile);

    return S_OK;
}
