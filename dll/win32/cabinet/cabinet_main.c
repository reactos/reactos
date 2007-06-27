/*
 * cabinet.dll main
 *
 * Copyright 2002 Patrik Stridvall
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

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#define NO_SHLWAPI_REG
#include "shlwapi.h"
#undef NO_SHLWAPI_REG

#include "cabinet.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

/* the following defintions are copied from msvcrt/fcntl.h */

#define _O_RDONLY      0
#define _O_WRONLY      1
#define _O_RDWR        2
#define _O_ACCMODE     (_O_RDONLY|_O_WRONLY|_O_RDWR)


/***********************************************************************
 * DllGetVersion (CABINET.2)
 *
 * Retrieves version information of the 'CABINET.DLL'
 *
 * PARAMS
 *     pdvi [O] pointer to version information structure.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: E_INVALIDARG
 *
 * NOTES
 *     Supposedly returns version from IE6SP1RP1
 */
HRESULT WINAPI DllGetVersion (DLLVERSIONINFO *pdvi)
{
  WARN("hmmm... not right version number \"5.1.1106.1\"?\n");

  if (pdvi->cbSize != sizeof(DLLVERSIONINFO)) return E_INVALIDARG;

  pdvi->dwMajorVersion = 5;
  pdvi->dwMinorVersion = 1;
  pdvi->dwBuildNumber = 1106;
  pdvi->dwPlatformID = 1;

  return S_OK;
}

/* FDI callback functions */

static void *mem_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void mem_free(void *memory)
{
    HeapFree(GetProcessHeap(), 0, memory);
}

static INT_PTR fdi_open(char *pszFile, int oflag, int pmode)
{
    HANDLE handle;
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;

    switch (oflag & _O_ACCMODE)
    {
        case _O_RDONLY:
            dwAccess = GENERIC_READ;
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
            break;
        case _O_WRONLY:
            dwAccess = GENERIC_WRITE;
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
            break;
        case _O_RDWR:
            dwAccess = GENERIC_READ | GENERIC_WRITE;
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
            break;
    }

    if (GetFileAttributesA(pszFile) != INVALID_FILE_ATTRIBUTES)
        dwCreateDisposition = OPEN_EXISTING;
    else
        dwCreateDisposition = CREATE_NEW;

    handle = CreateFileA(pszFile, dwAccess, dwShareMode, NULL,
                         dwCreateDisposition, 0, NULL);

    return (INT_PTR) handle;
}

static UINT fdi_read(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE) hf;
    DWORD dwRead;
    
    if (ReadFile(handle, pv, cb, &dwRead, NULL))
        return dwRead;

    return 0;
}

static UINT fdi_write(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE) hf;
    DWORD dwWritten;

    if (WriteFile(handle, pv, cb, &dwWritten, NULL))
        return dwWritten;

    return 0;
}

static int fdi_close(INT_PTR hf)
{
    HANDLE handle = (HANDLE) hf;
    return CloseHandle(handle) ? 0 : -1;
}

static long fdi_seek(INT_PTR hf, long dist, int seektype)
{
    HANDLE handle = (HANDLE) hf;
    return SetFilePointer(handle, dist, NULL, seektype);
}

static void fill_file_node(struct ExtractFileList *pNode, LPCSTR szFilename)
{
    pNode->next = NULL;
    pNode->flag = FALSE;

    pNode->filename = HeapAlloc(GetProcessHeap(), 0, strlen(szFilename) + 1);
    lstrcpyA(pNode->filename, szFilename);
}

static BOOL file_in_list(const struct ExtractFileList *pNode, LPCSTR szFilename)
{
    while (pNode)
    {
        if (!lstrcmpiA(pNode->filename, szFilename))
            return TRUE;

        pNode = pNode->next;
    }

    return FALSE;
}

static INT_PTR fdi_notify_extract(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
    switch (fdint)
    {
        case fdintCOPY_FILE:
        {
            struct ExtractFileList **fileList;
            EXTRACTdest *pDestination = pfdin->pv;
            LPSTR szFullPath, szDirectory;
            HANDLE hFile = 0;
            DWORD dwSize;

            dwSize = lstrlenA(pDestination->directory) +
                    lstrlenA("\\") + lstrlenA(pfdin->psz1) + 1;
            szFullPath = HeapAlloc(GetProcessHeap(), 0, dwSize);

            lstrcpyA(szFullPath, pDestination->directory);
            lstrcatA(szFullPath, "\\");
            lstrcatA(szFullPath, pfdin->psz1);

            /* pull out the destination directory string from the full path */
            dwSize = strrchr(szFullPath, '\\') - szFullPath + 1;
            szDirectory = HeapAlloc(GetProcessHeap(), 0, dwSize);
            lstrcpynA(szDirectory, szFullPath, dwSize);

            if (pDestination->flags & EXTRACT_FILLFILELIST)
            {
                fileList = &pDestination->filelist;

                while (*fileList)
                    fileList = &((*fileList)->next);

                *fileList = HeapAlloc(GetProcessHeap(), 0,
                                      sizeof(struct ExtractFileList));

                fill_file_node(*fileList, pfdin->psz1);
                lstrcpyA(pDestination->lastfile, szFullPath);
                pDestination->filecount++;
            }

            if ((pDestination->flags & EXTRACT_EXTRACTFILES) ||
                file_in_list(pDestination->filterlist, pfdin->psz1))
            {
                /* skip this file if it is not in the file list */
                if (!file_in_list(pDestination->filelist, pfdin->psz1))
                    return 0;

                /* create the destination directory if it doesn't exist */
                if (GetFileAttributesA(szDirectory) == INVALID_FILE_ATTRIBUTES)
                    CreateDirectoryA(szDirectory, NULL);

                hFile = CreateFileA(szFullPath, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                    CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

                if (hFile == INVALID_HANDLE_VALUE)
                    hFile = 0;
            }

            HeapFree(GetProcessHeap(), 0, szFullPath);
            HeapFree(GetProcessHeap(), 0, szDirectory);

            return (INT_PTR) hFile;
        }

        case fdintCLOSE_FILE_INFO:
        {
            FILETIME ft;
            FILETIME ftLocal;
            HANDLE handle = (HANDLE) pfdin->hf;

            if (!DosDateTimeToFileTime(pfdin->date, pfdin->time, &ft))
                return FALSE;

            if (!LocalFileTimeToFileTime(&ft, &ftLocal))
                return FALSE;

            if (!SetFileTime(handle, &ftLocal, 0, &ftLocal))
                return FALSE;

            CloseHandle(handle);
            return TRUE;
        }

        default:
            return 0;
    }
}

/***********************************************************************
 * Extract (CABINET.3)
 *
 * Extracts the contents of the cabinet file to the specified
 * destination.
 *
 * PARAMS
 *   dest      [I/O] Controls the operation of Extract.  See NOTES.
 *   szCabName [I] Filename of the cabinet to extract.
 *
 * RETURNS
 *     Success: S_OK.
 *     Failure: E_FAIL.
 *
 * NOTES
 *   The following members of the dest struct control the operation
 *   of Extract:
 *       filelist  [I] A linked list of filenames.  Extract only extracts
 *                     files from the cabinet that are in this list.
 *       filecount [O] Contains the number of files in filelist on
 *                     completion.
 *       flags     [I] See Operation.
 *       directory [I] The destination directory.
 *       lastfile  [O] The last file extracted.
 *
 *   Operation
 *     If flags contains EXTRACT_FILLFILELIST, then filelist will be
 *     filled with all the files in the cabinet.  If flags contains
 *     EXTRACT_EXTRACTFILES, then only the files in the filelist will
 *     be extracted from the cabinet.  EXTRACT_FILLFILELIST can be called
 *     by itself, but EXTRACT_EXTRACTFILES must have a valid filelist
 *     in order to succeed.  If flags contains both EXTRACT_FILLFILELIST
 *     and EXTRACT_EXTRACTFILES, then all the files in the cabinet
 *     will be extracted.
 */
HRESULT WINAPI Extract(EXTRACTdest *dest, LPCSTR szCabName)
{
    HRESULT res = S_OK;
    HFDI hfdi;
    ERF erf;
    char *str, *path, *name;

    TRACE("(%p, %s)\n", dest, szCabName);

    hfdi = FDICreate(mem_alloc,
                     mem_free,
                     fdi_open,
                     fdi_read,
                     fdi_write,
                     fdi_close,
                     fdi_seek,
                     cpuUNKNOWN,
                     &erf);

    if (!hfdi)
        return E_FAIL;

    if (GetFileAttributesA(dest->directory) == INVALID_FILE_ATTRIBUTES)
        return S_OK;

    /* split the cabinet name into path + name */
    str = HeapAlloc(GetProcessHeap(), 0, lstrlenA(szCabName)+1);
    if (!str)
    {
        res = E_OUTOFMEMORY;
        goto end;
    }
    lstrcpyA(str, szCabName);

    path = str;
    name = strrchr(path, '\\');
    if (name)
        *name++ = 0;
    else
    {
        name = path;
        path = NULL;
    }

    if (!FDICopy(hfdi, name, path, 0,
         fdi_notify_extract, NULL, dest))
        res = E_FAIL;

    HeapFree(GetProcessHeap(), 0, str);
end:

    FDIDestroy(hfdi);

    return res;
}
