/*
 * Path Functions
 *
 * Copyright 1999, 2000 Juergen Schmied
 * Copyright 2001, 2002 Jon Griffiths
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
#include <string.h>
#include <stdlib.h>

#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winternl.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#ifdef __REACTOS__
#include "winnetwk.h"
#endif
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#ifdef __REACTOS__

#include <shlobj.h>
#include <shlwapi_undoc.h>

int WINAPI IsNetDrive(int drive);

#else

/* Get a function pointer from a DLL handle */
#define GET_FUNC(func, module, name, fail) \
  do { \
    if (!func) { \
      if (!SHLWAPI_h##module && !(SHLWAPI_h##module = LoadLibraryA(#module ".dll"))) return fail; \
      func = (fn##func)GetProcAddress(SHLWAPI_h##module, name); \
      if (!func) return fail; \
    } \
  } while (0)

/* DLL handles for late bound calls */
static HMODULE SHLWAPI_hshell32;

/* Function pointers for GET_FUNC macro; these need to be global because of gcc bug */
typedef BOOL (WINAPI *fnpIsNetDrive)(int);
static  fnpIsNetDrive pIsNetDrive;

#endif /* __REACTOS__ */

HRESULT WINAPI SHGetWebFolderFilePathW(LPCWSTR,LPWSTR,DWORD);

/*************************************************************************
 * PathBuildRootA    [SHLWAPI.@]
 *
 * Create a root drive string (e.g. "A:\") from a drive number.
 *
 * PARAMS
 *  lpszPath [O] Destination for the drive string
 *
 * RETURNS
 *  lpszPath
 *
 * NOTES
 *  If lpszPath is NULL or drive is invalid, nothing is written to lpszPath.
 */
LPSTR WINAPI PathBuildRootA(LPSTR lpszPath, int drive)
{
  TRACE("(%p,%d)\n", lpszPath, drive);

  if (lpszPath && drive >= 0 && drive < 26)
  {
    lpszPath[0] = 'A' + drive;
    lpszPath[1] = ':';
    lpszPath[2] = '\\';
    lpszPath[3] = '\0';
  }
  return lpszPath;
}

/*************************************************************************
 * PathBuildRootW    [SHLWAPI.@]
 *
 * See PathBuildRootA.
 */
LPWSTR WINAPI PathBuildRootW(LPWSTR lpszPath, int drive)
{
  TRACE("(%p,%d)\n", lpszPath, drive);

  if (lpszPath && drive >= 0 && drive < 26)
  {
    lpszPath[0] = 'A' + drive;
    lpszPath[1] = ':';
    lpszPath[2] = '\\';
    lpszPath[3] = '\0';
  }
  return lpszPath;
}

/*************************************************************************
 * PathRemoveArgsA	[SHLWAPI.@]
 *
 * Strip space separated arguments from a path.
 *
 * PARAMS
 *  lpszPath [I/O] Path to remove arguments from
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI PathRemoveArgsA(LPSTR lpszPath)
{
  TRACE("(%s)\n",debugstr_a(lpszPath));

  if(lpszPath)
  {
    LPSTR lpszArgs = PathGetArgsA(lpszPath);
    if (*lpszArgs)
      lpszArgs[-1] = '\0';
    else
    {
      LPSTR lpszLastChar = CharPrevA(lpszPath, lpszArgs);
      if(*lpszLastChar == ' ')
        *lpszLastChar = '\0';
    }
  }
}

/*************************************************************************
 * PathRemoveArgsW	[SHLWAPI.@]
 *
 * See PathRemoveArgsA.
 */
void WINAPI PathRemoveArgsW(LPWSTR lpszPath)
{
  TRACE("(%s)\n",debugstr_w(lpszPath));

  if(lpszPath)
  {
    LPWSTR lpszArgs = PathGetArgsW(lpszPath);
    if (*lpszArgs || (lpszArgs > lpszPath && lpszArgs[-1] == ' '))
      lpszArgs[-1] = '\0';
  }
}

/*************************************************************************
 * @	[SHLWAPI.4]
 *
 * Unicode version of PathFileExistsDefExtA.
 */
BOOL WINAPI PathFileExistsDefExtW(LPWSTR lpszPath,DWORD dwWhich)
{
  static const WCHAR pszExts[][5]  = { { '.', 'p', 'i', 'f', 0},
                                       { '.', 'c', 'o', 'm', 0},
                                       { '.', 'e', 'x', 'e', 0},
                                       { '.', 'b', 'a', 't', 0},
                                       { '.', 'l', 'n', 'k', 0},
                                       { '.', 'c', 'm', 'd', 0},
                                       { 0, 0, 0, 0, 0} };

  TRACE("(%s,%ld)\n", debugstr_w(lpszPath), dwWhich);

  if (!lpszPath || PathIsUNCServerW(lpszPath) || PathIsUNCServerShareW(lpszPath))
    return FALSE;

  if (dwWhich)
  {
    LPCWSTR szExt = PathFindExtensionW(lpszPath);
#ifndef __REACTOS__
    if (!*szExt || dwWhich & 0x40)
#else
    if (!*szExt || dwWhich & WHICH_OPTIONAL)
#endif
    {
      size_t iChoose = 0;
      int iLen = lstrlenW(lpszPath);
      if (iLen > (MAX_PATH - 5))
        return FALSE;
#ifndef __REACTOS__
      while ( (dwWhich & 0x1) && pszExts[iChoose][0] )
#else
      while (pszExts[iChoose][0])
#endif
      {
#ifdef __REACTOS__
        if (dwWhich & 0x1)
        {
        if (GetFileAttributes(lpszPath) != FILE_ATTRIBUTE_DIRECTORY)
#endif
        lstrcpyW(lpszPath + iLen, pszExts[iChoose]);
        if (PathFileExistsW(lpszPath))
          return TRUE;
#ifdef __REACTOS__
        }
#endif
        iChoose++;
        dwWhich >>= 1;
      }
      *(lpszPath + iLen) = (WCHAR)'\0';
      return FALSE;
    }
  }
  return PathFileExistsW(lpszPath);
}

/*************************************************************************
 * @	[SHLWAPI.3]
 *
 * Determine if a file exists locally and is of an executable type.
 *
 * PARAMS
 *  lpszPath       [I/O] File to search for
 *  dwWhich        [I]   Type of executable to search for
 *
 * RETURNS
 *  TRUE  If the file was found. lpszPath contains the file name.
 *  FALSE Otherwise.
 *
 * NOTES
 *  lpszPath is modified in place and must be at least MAX_PATH in length.
 *  If the function returns FALSE, the path is modified to its original state.
 *  If the given path contains an extension or dwWhich is 0, executable
 *  extensions are not checked.
 *
 *  Ordinals 3-6 are a classic case of MS exposing limited functionality to
 *  users (here through PathFindOnPathA()) and keeping advanced functionality for
 *  their own developers exclusive use. Monopoly, anyone?
 */
BOOL WINAPI PathFileExistsDefExtA(LPSTR lpszPath,DWORD dwWhich)
{
  BOOL bRet = FALSE;

  TRACE("(%s,%ld)\n", debugstr_a(lpszPath), dwWhich);

  if (lpszPath)
  {
    WCHAR szPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP,0,lpszPath,-1,szPath,MAX_PATH);
    bRet = PathFileExistsDefExtW(szPath, dwWhich);
    if (bRet)
      WideCharToMultiByte(CP_ACP,0,szPath,-1,lpszPath,MAX_PATH,0,0);
  }
  return bRet;
}

/*************************************************************************
 * SHLWAPI_PathFindInOtherDirs
 *
 * Internal helper for SHLWAPI_PathFindOnPathExA/W.
 */
static BOOL SHLWAPI_PathFindInOtherDirs(LPWSTR lpszFile, DWORD dwWhich)
{
  static const WCHAR szSystem[] = { 'S','y','s','t','e','m','\0'};
  static const WCHAR szPath[] = { 'P','A','T','H','\0'};
  DWORD dwLenPATH;
  LPCWSTR lpszCurr;
  WCHAR *lpszPATH;
  WCHAR buff[MAX_PATH];

  TRACE("(%s,%08lx)\n", debugstr_w(lpszFile), dwWhich);

  /* Try system directories */
  GetSystemDirectoryW(buff, MAX_PATH);
  if (!PathAppendW(buff, lpszFile))
     return FALSE;
  if (PathFileExistsDefExtW(buff, dwWhich))
  {
    lstrcpyW(lpszFile, buff);
    return TRUE;
  }
  GetWindowsDirectoryW(buff, MAX_PATH);
  if (!PathAppendW(buff, szSystem ) || !PathAppendW(buff, lpszFile))
    return FALSE;
  if (PathFileExistsDefExtW(buff, dwWhich))
  {
    lstrcpyW(lpszFile, buff);
    return TRUE;
  }
  GetWindowsDirectoryW(buff, MAX_PATH);
  if (!PathAppendW(buff, lpszFile))
    return FALSE;
  if (PathFileExistsDefExtW(buff, dwWhich))
  {
    lstrcpyW(lpszFile, buff);
    return TRUE;
  }
  /* Try dirs listed in %PATH% */
  dwLenPATH = GetEnvironmentVariableW(szPath, buff, MAX_PATH);

  if (!dwLenPATH || !(lpszPATH = malloc((dwLenPATH + 1) * sizeof (WCHAR))))
    return FALSE;

  GetEnvironmentVariableW(szPath, lpszPATH, dwLenPATH + 1);
  lpszCurr = lpszPATH;
  while (lpszCurr)
  {
    LPCWSTR lpszEnd = lpszCurr;
    LPWSTR pBuff = buff;

    while (*lpszEnd == ' ')
      lpszEnd++;
    while (*lpszEnd && *lpszEnd != ';')
      *pBuff++ = *lpszEnd++;
    *pBuff = '\0';

    if (*lpszEnd)
      lpszCurr = lpszEnd + 1;
    else
      lpszCurr = NULL; /* Last Path, terminate after this */

    if (!PathAppendW(buff, lpszFile))
    {
      free(lpszPATH);
      return FALSE;
    }
    if (PathFileExistsDefExtW(buff, dwWhich))
    {
      lstrcpyW(lpszFile, buff);
      free(lpszPATH);
      return TRUE;
    }
  }
  free(lpszPATH);
  return FALSE;
}

/*************************************************************************
 * @	[SHLWAPI.5]
 *
 * Search a range of paths for a specific type of executable.
 *
 * PARAMS
 *  lpszFile       [I/O] File to search for
 *  lppszOtherDirs [I]   Other directories to look in
 *  dwWhich        [I]   Type of executable to search for
 *
 * RETURNS
 *  Success: TRUE. The path to the executable is stored in lpszFile.
 *  Failure: FALSE. The path to the executable is unchanged.
 */
BOOL WINAPI PathFindOnPathExA(LPSTR lpszFile,LPCSTR *lppszOtherDirs,DWORD dwWhich)
{
  WCHAR szFile[MAX_PATH];
  WCHAR buff[MAX_PATH];

  TRACE("(%s,%p,%08lx)\n", debugstr_a(lpszFile), lppszOtherDirs, dwWhich);

  if (!lpszFile || !PathIsFileSpecA(lpszFile))
    return FALSE;

  MultiByteToWideChar(CP_ACP,0,lpszFile,-1,szFile,MAX_PATH);

  /* Search provided directories first */
  if (lppszOtherDirs && *lppszOtherDirs)
  {
    WCHAR szOther[MAX_PATH];
    LPCSTR *lpszOtherPath = lppszOtherDirs;

    while (lpszOtherPath && *lpszOtherPath && (*lpszOtherPath)[0])
    {
      MultiByteToWideChar(CP_ACP,0,*lpszOtherPath,-1,szOther,MAX_PATH);
      PathCombineW(buff, szOther, szFile);
      if (PathFileExistsDefExtW(buff, dwWhich))
      {
        WideCharToMultiByte(CP_ACP,0,buff,-1,lpszFile,MAX_PATH,0,0);
        return TRUE;
      }
      lpszOtherPath++;
    }
  }
  /* Not found, try system and path dirs */
  if (SHLWAPI_PathFindInOtherDirs(szFile, dwWhich))
  {
    WideCharToMultiByte(CP_ACP,0,szFile,-1,lpszFile,MAX_PATH,0,0);
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 * @	[SHLWAPI.6]
 *
 * Unicode version of PathFindOnPathExA.
 */
BOOL WINAPI PathFindOnPathExW(LPWSTR lpszFile,LPCWSTR *lppszOtherDirs,DWORD dwWhich)
{
  WCHAR buff[MAX_PATH];

  TRACE("(%s,%p,%08lx)\n", debugstr_w(lpszFile), lppszOtherDirs, dwWhich);

  if (!lpszFile || !PathIsFileSpecW(lpszFile))
    return FALSE;

  /* Search provided directories first */
  if (lppszOtherDirs && *lppszOtherDirs)
  {
    LPCWSTR *lpszOtherPath = lppszOtherDirs;
    while (lpszOtherPath && *lpszOtherPath && (*lpszOtherPath)[0])
    {
      PathCombineW(buff, *lpszOtherPath, lpszFile);
      if (PathFileExistsDefExtW(buff, dwWhich))
      {
        lstrcpyW(lpszFile, buff);
        return TRUE;
      }
      lpszOtherPath++;
    }
  }
  /* Not found, try system and path dirs */
  return SHLWAPI_PathFindInOtherDirs(lpszFile, dwWhich);
}

/*************************************************************************
 * PathFindOnPathA	[SHLWAPI.@]
 *
 * Search a range of paths for an executable.
 *
 * PARAMS
 *  lpszFile       [I/O] File to search for
 *  lppszOtherDirs [I]   Other directories to look in
 *
 * RETURNS
 *  Success: TRUE. The path to the executable is stored in lpszFile.
 *  Failure: FALSE. The path to the executable is unchanged.
 */
BOOL WINAPI PathFindOnPathA(LPSTR lpszFile, LPCSTR *lppszOtherDirs)
{
  TRACE("(%s,%p)\n", debugstr_a(lpszFile), lppszOtherDirs);
  return PathFindOnPathExA(lpszFile, lppszOtherDirs, 0);
 }

/*************************************************************************
 * PathFindOnPathW      [SHLWAPI.@]
 *
 * See PathFindOnPathA.
 */
BOOL WINAPI PathFindOnPathW(LPWSTR lpszFile, LPCWSTR *lppszOtherDirs)
{
  TRACE("(%s,%p)\n", debugstr_w(lpszFile), lppszOtherDirs);
  return PathFindOnPathExW(lpszFile,lppszOtherDirs, 0);
}

/*************************************************************************
 * PathCompactPathExA   [SHLWAPI.@]
 *
 * Compact a path into a given number of characters.
 *
 * PARAMS
 *  lpszDest [O] Destination for compacted path
 *  lpszPath [I] Source path
 *  cchMax   [I] Maximum size of compacted path
 *  dwFlags  [I] Reserved
 *
 * RETURNS
 *  Success: TRUE. The compacted path is written to lpszDest.
 *  Failure: FALSE. lpszPath is undefined.
 *
 * NOTES
 *  If cchMax is given as 0, lpszDest will still be NUL terminated.
 *
 *  The Win32 version of this function contains a bug: When cchMax == 7,
 *  8 bytes will be written to lpszDest. This bug is fixed in the Wine
 *  implementation.
 *
 *  Some relative paths will be different when cchMax == 5 or 6. This occurs
 *  because Win32 will insert a "\" in lpszDest, even if one is
 *  not present in the original path.
 */
BOOL WINAPI PathCompactPathExA(LPSTR lpszDest, LPCSTR lpszPath,
                               UINT cchMax, DWORD dwFlags)
{
  BOOL bRet = FALSE;

  TRACE("(%p,%s,%d,0x%08lx)\n", lpszDest, debugstr_a(lpszPath), cchMax, dwFlags);

  if (lpszPath && lpszDest)
  {
    WCHAR szPath[MAX_PATH];
    WCHAR szDest[MAX_PATH];

    MultiByteToWideChar(CP_ACP,0,lpszPath,-1,szPath,MAX_PATH);
    szDest[0] = '\0';
    bRet = PathCompactPathExW(szDest, szPath, cchMax, dwFlags);
    WideCharToMultiByte(CP_ACP,0,szDest,-1,lpszDest,MAX_PATH,0,0);
  }
  return bRet;
}

/*************************************************************************
 * PathCompactPathExW   [SHLWAPI.@]
 *
 * See PathCompactPathExA.
 */
BOOL WINAPI PathCompactPathExW(LPWSTR lpszDest, LPCWSTR lpszPath,
                               UINT cchMax, DWORD dwFlags)
{
  static const WCHAR szEllipses[] = { '.', '.', '.', '\0' };
  LPCWSTR lpszFile;
  DWORD dwLen, dwFileLen = 0;

  TRACE("(%p,%s,%d,0x%08lx)\n", lpszDest, debugstr_w(lpszPath), cchMax, dwFlags);

  if (!lpszPath)
    return FALSE;

  if (!lpszDest)
  {
    WARN("Invalid lpszDest would crash under Win32!\n");
    return FALSE;
  }

  *lpszDest = '\0';

  if (cchMax < 2)
    return TRUE;

  dwLen = lstrlenW(lpszPath) + 1;

  if (dwLen < cchMax)
  {
    /* Don't need to compact */
    memcpy(lpszDest, lpszPath, dwLen * sizeof(WCHAR));
    return TRUE;
  }

  /* Path must be compacted to fit into lpszDest */
  lpszFile = PathFindFileNameW(lpszPath);
  dwFileLen = lpszPath + dwLen - lpszFile;

  if (dwFileLen == dwLen)
  {
    /* No root in psth */
    if (cchMax <= 4)
    {
      while (--cchMax > 0) /* No room left for anything but ellipses */
        *lpszDest++ = '.';
      *lpszDest = '\0';
      return TRUE;
    }
    /* Compact the file name with ellipses at the end */
    cchMax -= 4;
    memcpy(lpszDest, lpszFile, cchMax * sizeof(WCHAR));
    lstrcpyW(lpszDest + cchMax, szEllipses);
    return TRUE;
  }
  /* We have a root in the path */
  lpszFile--; /* Start compacted filename with the path separator */
  dwFileLen++;

  if (dwFileLen + 3 > cchMax)
  {
    /* Compact the file name */
    if (cchMax <= 4)
    {
      while (--cchMax > 0) /* No room left for anything but ellipses */
        *lpszDest++ = '.';
      *lpszDest = '\0';
      return TRUE;
    }
    lstrcpyW(lpszDest, szEllipses);
    lpszDest += 3;
    cchMax -= 4;
    *lpszDest++ = *lpszFile++;
    if (cchMax <= 4)
    {
      while (--cchMax > 0) /* No room left for anything but ellipses */
        *lpszDest++ = '.';
      *lpszDest = '\0';
      return TRUE;
    }
    cchMax -= 4;
    memcpy(lpszDest, lpszFile, cchMax * sizeof(WCHAR));
    lstrcpyW(lpszDest + cchMax, szEllipses);
    return TRUE;
  }

  /* Only the root needs to be Compacted */
  dwLen = cchMax - dwFileLen - 3;
  memcpy(lpszDest, lpszPath, dwLen * sizeof(WCHAR));
  lstrcpyW(lpszDest + dwLen, szEllipses);
  lstrcpyW(lpszDest + dwLen + 3, lpszFile);
  return TRUE;
}

/*************************************************************************
 * PathIsDirectoryA	[SHLWAPI.@]
 *
 * Determine if a path is a valid directory
 *
 * PARAMS
 *  lpszPath [I] Path to check.
 *
 * RETURNS
 *  FILE_ATTRIBUTE_DIRECTORY if lpszPath exists and can be read (See Notes)
 *  FALSE if lpszPath is invalid or not a directory.
 *
 * NOTES
 *  Although this function is prototyped as returning a BOOL, it returns
 *  FILE_ATTRIBUTE_DIRECTORY for success. This means that code such as:
 *
 *|  if (PathIsDirectoryA("c:\\windows\\") == TRUE)
 *|    ...
 *
 *  will always fail.
 */
BOOL WINAPI PathIsDirectoryA(LPCSTR lpszPath)
{
  DWORD dwAttr;

  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (!lpszPath || PathIsUNCServerA(lpszPath))
    return FALSE;

  if (PathIsUNCServerShareA(lpszPath))
  {
#ifdef __REACTOS__
    LPSTR lpSystem = NULL;
    BYTE buffer[512] = {0};
    DWORD cbBuffer = sizeof(buffer);
    LPNETRESOURCEA pNetRes = (LPNETRESOURCEA)buffer;
    DWORD dwError;

    pNetRes->dwScope      = RESOURCE_GLOBALNET;
    pNetRes->dwType       = RESOURCETYPE_ANY;
    pNetRes->lpRemoteName = (LPSTR)lpszPath;

    dwError = WNetGetResourceInformationA(pNetRes, pNetRes, &cbBuffer, &lpSystem);
    if (dwError == NO_ERROR && pNetRes->dwDisplayType != RESOURCEDISPLAYTYPE_GENERIC)
    {
      return (pNetRes->dwDisplayType == RESOURCEDISPLAYTYPE_SHARE) &&
             (pNetRes->dwType == RESOURCETYPE_ANY || pNetRes->dwType == RESOURCETYPE_DISK);
    }
#else
    FIXME("UNC Server Share not yet supported - FAILING\n");
    return FALSE;
#endif
  }

  if ((dwAttr = GetFileAttributesA(lpszPath)) == INVALID_FILE_ATTRIBUTES)
    return FALSE;
  return dwAttr & FILE_ATTRIBUTE_DIRECTORY;
}

/*************************************************************************
 * PathIsDirectoryW	[SHLWAPI.@]
 *
 * See PathIsDirectoryA.
 */
BOOL WINAPI PathIsDirectoryW(LPCWSTR lpszPath)
{
  DWORD dwAttr;

  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (!lpszPath || PathIsUNCServerW(lpszPath))
    return FALSE;

  if (PathIsUNCServerShareW(lpszPath))
  {
#ifdef __REACTOS__
    LPWSTR lpSystem = NULL;
    BYTE buffer[1024] = {0};
    DWORD cbBuffer = sizeof(buffer);
    LPNETRESOURCEW pNetRes = (LPNETRESOURCEW)buffer;
    DWORD dwError;

    pNetRes->dwScope      = RESOURCE_GLOBALNET;
    pNetRes->dwType       = RESOURCETYPE_ANY;
    pNetRes->lpRemoteName = (LPWSTR)lpszPath;

    dwError = WNetGetResourceInformationW(pNetRes, pNetRes, &cbBuffer, &lpSystem);
    if (dwError == NO_ERROR && pNetRes->dwDisplayType != RESOURCEDISPLAYTYPE_GENERIC)
    {
      return (pNetRes->dwDisplayType == RESOURCEDISPLAYTYPE_SHARE) &&
             (pNetRes->dwType == RESOURCETYPE_ANY || pNetRes->dwType == RESOURCETYPE_DISK);
    }
#else
    FIXME("UNC Server Share not yet supported - FAILING\n");
    return FALSE;
#endif
  }

  if ((dwAttr = GetFileAttributesW(lpszPath)) == INVALID_FILE_ATTRIBUTES)
    return FALSE;
  return dwAttr & FILE_ATTRIBUTE_DIRECTORY;
}

/*************************************************************************
 * PathFileExistsAndAttributesA	[SHLWAPI.445]
 *
 * Determine if a file exists.
 *
 * PARAMS
 *  lpszPath [I] Path to check
 *  dwAttr   [O] attributes of file
 *
 * RETURNS
 *  TRUE  If the file exists and is readable
 *  FALSE Otherwise
 */
BOOL WINAPI PathFileExistsAndAttributesA(LPCSTR lpszPath, DWORD *dwAttr)
{
  UINT iPrevErrMode;
  DWORD dwVal = 0;

  TRACE("(%s %p)\n", debugstr_a(lpszPath), dwAttr);

  if (dwAttr)
    *dwAttr = INVALID_FILE_ATTRIBUTES;

  if (!lpszPath)
    return FALSE;

  iPrevErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);
  dwVal = GetFileAttributesA(lpszPath);
  SetErrorMode(iPrevErrMode);
  if (dwAttr)
    *dwAttr = dwVal;
  return (dwVal != INVALID_FILE_ATTRIBUTES);
}

/*************************************************************************
 * PathFileExistsAndAttributesW	[SHLWAPI.446]
 *
 * See PathFileExistsA.
 */
BOOL WINAPI PathFileExistsAndAttributesW(LPCWSTR lpszPath, DWORD *dwAttr)
{
  UINT iPrevErrMode;
  DWORD dwVal;

  TRACE("(%s %p)\n", debugstr_w(lpszPath), dwAttr);

  if (!lpszPath)
    return FALSE;

  iPrevErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);
  dwVal = GetFileAttributesW(lpszPath);
  SetErrorMode(iPrevErrMode);
  if (dwAttr)
    *dwAttr = dwVal;
  return (dwVal != INVALID_FILE_ATTRIBUTES);
}

/*************************************************************************
 * PathIsContentTypeA   [SHLWAPI.@]
 *
 * Determine if a file is of a given registered content type.
 *
 * PARAMS
 *  lpszPath        [I] File to check
 *  lpszContentType [I] Content type to check for
 *
 * RETURNS
 *  TRUE  If lpszPath is a given registered content type,
 *  FALSE Otherwise.
 *
 * NOTES
 *  This function looks up the registered content type for lpszPath. If
 *  a content type is registered, it is compared (case insensitively) to
 *  lpszContentType. Only if this matches does the function succeed.
 */
BOOL WINAPI PathIsContentTypeA(LPCSTR path, LPCSTR content_type)
{
    char buf[MAX_PATH];
    DWORD size = sizeof(buf);
    LPCSTR ext;

    TRACE("(%s,%s)\n", debugstr_a(path), debugstr_a(content_type));

    if(!path) return FALSE;
    if(!(ext = PathFindExtensionA(path)) || !*ext) return FALSE;
    if(SHGetValueA(HKEY_CLASSES_ROOT, ext, "Content Type", NULL, buf, &size)) return FALSE;
    return !lstrcmpiA(content_type, buf);
}

/*************************************************************************
 * PathIsContentTypeW   [SHLWAPI.@]
 *
 * See PathIsContentTypeA.
 */
BOOL WINAPI PathIsContentTypeW(LPCWSTR lpszPath, LPCWSTR lpszContentType)
{
  static const WCHAR szContentType[] = { 'C','o','n','t','e','n','t',' ','T','y','p','e','\0' };
  LPCWSTR szExt;
  DWORD dwDummy;
  WCHAR szBuff[MAX_PATH];

  TRACE("(%s,%s)\n", debugstr_w(lpszPath), debugstr_w(lpszContentType));

  if (lpszPath && (szExt = PathFindExtensionW(lpszPath)) && *szExt &&
      !SHGetValueW(HKEY_CLASSES_ROOT, szExt, szContentType,
                   REG_NONE, szBuff, &dwDummy) &&
      !wcsicmp(lpszContentType, szBuff))
  {
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 * PathIsSystemFolderA   [SHLWAPI.@]
 *
 * Determine if a path or file attributes are a system folder.
 *
 * PARAMS
 *  lpszPath  [I] Path to check.
 *  dwAttrib  [I] Attributes to check, if lpszPath is NULL.
 *
 * RETURNS
 *  TRUE   If lpszPath or dwAttrib are a system folder.
 *  FALSE  If GetFileAttributesA() fails or neither parameter is a system folder.
 */
BOOL WINAPI PathIsSystemFolderA(LPCSTR lpszPath, DWORD dwAttrib)
{
  TRACE("(%s,0x%08lx)\n", debugstr_a(lpszPath), dwAttrib);

  if (lpszPath && *lpszPath)
    dwAttrib = GetFileAttributesA(lpszPath);

  if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) ||
      !(dwAttrib & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 * PathIsSystemFolderW   [SHLWAPI.@]
 *
 * See PathIsSystemFolderA.
 */
BOOL WINAPI PathIsSystemFolderW(LPCWSTR lpszPath, DWORD dwAttrib)
{
  TRACE("(%s,0x%08lx)\n", debugstr_w(lpszPath), dwAttrib);

  if (lpszPath && *lpszPath)
    dwAttrib = GetFileAttributesW(lpszPath);

  if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) ||
      !(dwAttrib & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 * PathMakePrettyA   [SHLWAPI.@]
 *
 * Convert an uppercase DOS filename into lowercase.
 *
 * PARAMS
 *  lpszPath [I/O] Path to convert.
 *
 * RETURNS
 *  TRUE  If the path was an uppercase DOS path and was converted,
 *  FALSE Otherwise.
 */
BOOL WINAPI PathMakePrettyA(LPSTR lpszPath)
{
  LPSTR pszIter = lpszPath;

  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (!pszIter)
    return FALSE;

  if (*pszIter)
  {
    do
    {
      if (islower(*pszIter) || IsDBCSLeadByte(*pszIter))
        return FALSE; /* Not DOS path */
      pszIter++;
    } while (*pszIter);
    pszIter = lpszPath + 1;
    while (*pszIter)
    {
      *pszIter = tolower(*pszIter);
      pszIter++;
    }
  }
  return TRUE;
}

/*************************************************************************
 * PathMakePrettyW   [SHLWAPI.@]
 *
 * See PathMakePrettyA.
 */
BOOL WINAPI PathMakePrettyW(LPWSTR lpszPath)
{
  LPWSTR pszIter = lpszPath;

  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (!pszIter)
    return FALSE;

  if (*pszIter)
  {
    do
    {
      if (iswlower(*pszIter))
        return FALSE; /* Not DOS path */
      pszIter++;
    } while (*pszIter);
    pszIter = lpszPath + 1;
    while (*pszIter)
    {
      *pszIter = towlower(*pszIter);
      pszIter++;
    }
  }
  return TRUE;
}

/*************************************************************************
 * PathCompactPathA   [SHLWAPI.@]
 *
 * Make a path fit into a given width when printed to a DC.
 *
 * PARAMS
 *  hDc      [I]   Destination DC
 *  lpszPath [I/O] Path to be printed to hDc
 *  dx       [I]   Desired width
 *
 * RETURNS
 *  TRUE  If the path was modified/went well.
 *  FALSE Otherwise.
 */
BOOL WINAPI PathCompactPathA(HDC hDC, LPSTR lpszPath, UINT dx)
{
  BOOL bRet = FALSE;

  TRACE("(%p,%s,%d)\n", hDC, debugstr_a(lpszPath), dx);

  if (lpszPath)
  {
    WCHAR szPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP,0,lpszPath,-1,szPath,MAX_PATH);
    bRet = PathCompactPathW(hDC, szPath, dx);
    WideCharToMultiByte(CP_ACP,0,szPath,-1,lpszPath,MAX_PATH,0,0);
  }
  return bRet;
}

/*************************************************************************
 * PathCompactPathW   [SHLWAPI.@]
 *
 * See PathCompactPathA.
 */
BOOL WINAPI PathCompactPathW(HDC hDC, LPWSTR lpszPath, UINT dx)
{
  static const WCHAR szEllipses[] = { '.', '.', '.', '\0' };
  BOOL bRet = TRUE;
  HDC hdc = 0;
  WCHAR buff[MAX_PATH];
  SIZE size;
  DWORD dwLen;

  TRACE("(%p,%s,%d)\n", hDC, debugstr_w(lpszPath), dx);

  if (!lpszPath)
    return FALSE;

  if (!hDC)
    hdc = hDC = GetDC(0);

  /* Get the length of the whole path */
  dwLen = lstrlenW(lpszPath);
  GetTextExtentPointW(hDC, lpszPath, dwLen, &size);

  if ((UINT)size.cx > dx)
  {
    /* Path too big, must reduce it */
    LPWSTR sFile;
    DWORD dwEllipsesLen = 0, dwPathLen = 0;

    sFile = PathFindFileNameW(lpszPath);
    if (sFile != lpszPath) sFile--;

    /* Get the size of ellipses */
    GetTextExtentPointW(hDC, szEllipses, 3, &size);
    dwEllipsesLen = size.cx;
    /* Get the size of the file name */
    GetTextExtentPointW(hDC, sFile, lstrlenW(sFile), &size);
    dwPathLen = size.cx;

    if (sFile != lpszPath)
    {
      LPWSTR sPath = sFile;
      BOOL bEllipses = FALSE;

      /* The path includes a file name. Include as much of the path prior to
       * the file name as possible, allowing for the ellipses, e.g:
       * c:\some very long path\filename ==> c:\some v...\filename
       */
      lstrcpynW(buff, sFile, MAX_PATH);

      do
      {
        DWORD dwTotalLen = bEllipses? dwPathLen + dwEllipsesLen : dwPathLen;

        GetTextExtentPointW(hDC, lpszPath, sPath - lpszPath, &size);
        dwTotalLen += size.cx;
        if (dwTotalLen <= dx)
          break;
        sPath--;
        if (!bEllipses)
        {
          bEllipses = TRUE;
          sPath -= 2;
        }
      } while (sPath > lpszPath);

      if (sPath > lpszPath)
      {
        if (bEllipses)
        {
          lstrcpyW(sPath, szEllipses);
          lstrcpyW(sPath+3, buff);
        }
        bRet = TRUE;
        goto end;
      }
      lstrcpyW(lpszPath, szEllipses);
      lstrcpyW(lpszPath+3, buff);
      bRet = FALSE;
      goto end;
    }

    /* Trim the path by adding ellipses to the end, e.g:
     * A very long file name.txt ==> A very...
     */
    dwLen = lstrlenW(lpszPath);

    if (dwLen > MAX_PATH - 3)
      dwLen =  MAX_PATH - 3;
    lstrcpynW(buff, sFile, dwLen);

    do {
      dwLen--;
      GetTextExtentPointW(hDC, buff, dwLen, &size);
    } while (dwLen && size.cx + dwEllipsesLen > dx);

   if (!dwLen)
   {
     DWORD dwWritten = 0;

     dwEllipsesLen /= 3; /* Size of a single '.' */

     /* Write as much of the Ellipses string as possible */
     while (dwWritten + dwEllipsesLen < dx && dwLen < 3)
     {
       *lpszPath++ = '.';
       dwWritten += dwEllipsesLen;
       dwLen++;
     }
     *lpszPath = '\0';
     bRet = FALSE;
   }
   else
   {
     lstrcpyW(buff + dwLen, szEllipses);
     lstrcpyW(lpszPath, buff);
    }
  }

end:
  if (hdc)
    ReleaseDC(0, hdc);

  return bRet;
}

/*************************************************************************
 * SHLWAPI_UseSystemForSystemFolders
 *
 * Internal helper for PathMakeSystemFolderW.
 */
static BOOL SHLWAPI_UseSystemForSystemFolders(void)
{
  static BOOL bCheckedReg = FALSE;
  static BOOL bUseSystemForSystemFolders = FALSE;

  if (!bCheckedReg)
  {
    bCheckedReg = TRUE;

    /* Key tells Win what file attributes to use on system folders */
    if (SHGetValueA(HKEY_LOCAL_MACHINE,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
        "UseSystemForSystemFolders", 0, 0, 0))
      bUseSystemForSystemFolders = TRUE;
  }
  return bUseSystemForSystemFolders;
}

/*************************************************************************
 * PathMakeSystemFolderA   [SHLWAPI.@]
 *
 * Set system folder attribute for a path.
 *
 * PARAMS
 *  lpszPath [I] The path to turn into a system folder
 *
 * RETURNS
 *  TRUE  If the path was changed to/already was a system folder
 *  FALSE If the path is invalid or SetFileAttributesA() fails
 */
BOOL WINAPI PathMakeSystemFolderA(LPCSTR lpszPath)
{
  BOOL bRet = FALSE;

  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (lpszPath && *lpszPath)
  {
    WCHAR szPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP,0,lpszPath,-1,szPath,MAX_PATH);
    bRet = PathMakeSystemFolderW(szPath);
  }
  return bRet;
}

/*************************************************************************
 * PathMakeSystemFolderW   [SHLWAPI.@]
 *
 * See PathMakeSystemFolderA.
 */
BOOL WINAPI PathMakeSystemFolderW(LPCWSTR lpszPath)
{
  DWORD dwDefaultAttr = FILE_ATTRIBUTE_READONLY, dwAttr;
  WCHAR buff[MAX_PATH];

  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (!lpszPath || !*lpszPath)
    return FALSE;

  /* If the directory is already a system directory, don't do anything */
  GetSystemDirectoryW(buff, MAX_PATH);
  if (!wcscmp(buff, lpszPath))
    return TRUE;

  GetWindowsDirectoryW(buff, MAX_PATH);
  if (!wcscmp(buff, lpszPath))
    return TRUE;

  /* "UseSystemForSystemFolders" Tells Win what attributes to use */
  if (SHLWAPI_UseSystemForSystemFolders())
    dwDefaultAttr = FILE_ATTRIBUTE_SYSTEM;

  if ((dwAttr = GetFileAttributesW(lpszPath)) == INVALID_FILE_ATTRIBUTES)
    return FALSE;

  /* Change file attributes to system attributes */
  dwAttr &= ~(FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY);
  return SetFileAttributesW(lpszPath, dwAttr | dwDefaultAttr);
}

/*************************************************************************
 * PathUnmakeSystemFolderA   [SHLWAPI.@]
 *
 * Remove the system folder attributes from a path.
 *
 * PARAMS
 *  lpszPath [I] The path to remove attributes from
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE, if lpszPath is NULL, empty, not a directory, or calling
 *           SetFileAttributesA() fails.
 */
BOOL WINAPI PathUnmakeSystemFolderA(LPCSTR lpszPath)
{
  DWORD dwAttr;

  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (!lpszPath || !*lpszPath || (dwAttr = GetFileAttributesA(lpszPath)) == INVALID_FILE_ATTRIBUTES ||
      !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
    return FALSE;

  dwAttr &= ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
  return SetFileAttributesA(lpszPath, dwAttr);
}

/*************************************************************************
 * PathUnmakeSystemFolderW   [SHLWAPI.@]
 *
 * See PathUnmakeSystemFolderA.
 */
BOOL WINAPI PathUnmakeSystemFolderW(LPCWSTR lpszPath)
{
  DWORD dwAttr;

  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (!lpszPath || !*lpszPath || (dwAttr = GetFileAttributesW(lpszPath)) == INVALID_FILE_ATTRIBUTES ||
    !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
    return FALSE;

  dwAttr &= ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
  return SetFileAttributesW(lpszPath, dwAttr);
}


/*************************************************************************
 * PathSetDlgItemPathA   [SHLWAPI.@]
 *
 * Set the text of a dialog item to a path, shrinking the path to fit
 * if it is too big for the item.
 *
 * PARAMS
 *  hDlg     [I] Dialog handle
 *  id       [I] ID of item in the dialog
 *  lpszPath [I] Path to set as the items text
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  If lpszPath is NULL, a blank string ("") is set (i.e. The previous
 *  window text is erased).
 */
VOID WINAPI PathSetDlgItemPathA(HWND hDlg, int id, LPCSTR lpszPath)
{
  WCHAR szPath[MAX_PATH];

  TRACE("(%p,%8x,%s)\n",hDlg, id, debugstr_a(lpszPath));

  if (lpszPath)
    MultiByteToWideChar(CP_ACP,0,lpszPath,-1,szPath,MAX_PATH);
  else
    szPath[0] = '\0';
  PathSetDlgItemPathW(hDlg, id, szPath);
}

/*************************************************************************
 * PathSetDlgItemPathW   [SHLWAPI.@]
 *
 * See PathSetDlgItemPathA.
 */
VOID WINAPI PathSetDlgItemPathW(HWND hDlg, int id, LPCWSTR lpszPath)
{
  WCHAR path[MAX_PATH + 1];
  HWND hwItem;
  RECT rect;
  HDC hdc;
  HGDIOBJ hPrevObj;

  TRACE("(%p,%8x,%s)\n",hDlg, id, debugstr_w(lpszPath));

  if (!(hwItem = GetDlgItem(hDlg, id)))
    return;

  if (lpszPath)
    lstrcpynW(path, lpszPath, ARRAY_SIZE(path));
  else
    path[0] = '\0';

  GetClientRect(hwItem, &rect);
  hdc = GetDC(hDlg);
  hPrevObj = SelectObject(hdc, (HGDIOBJ)SendMessageW(hwItem,WM_GETFONT,0,0));

  if (hPrevObj)
  {
    PathCompactPathW(hdc, path, rect.right);
    SelectObject(hdc, hPrevObj);
  }

  ReleaseDC(hDlg, hdc);
  SetWindowTextW(hwItem, path);
}

/*************************************************************************
 * PathIsNetworkPathA [SHLWAPI.@]
 *
 * Determine if the given path is a network path.
 *
 * PARAMS
 *  lpszPath [I] Path to check
 *
 * RETURNS
 *  TRUE  If lpszPath is a UNC share or mapped network drive, or
 *  FALSE If lpszPath is a local drive or cannot be determined
 */
BOOL WINAPI PathIsNetworkPathA(LPCSTR lpszPath)
{
  int dwDriveNum;

  TRACE("(%s)\n",debugstr_a(lpszPath));

  if (!lpszPath)
    return FALSE;
  if (*lpszPath == '\\' && lpszPath[1] == '\\')
    return TRUE;
  dwDriveNum = PathGetDriveNumberA(lpszPath);
  if (dwDriveNum == -1)
    return FALSE;
#ifdef __REACTOS__
  return IsNetDrive(dwDriveNum);
#else
  GET_FUNC(pIsNetDrive, shell32, (LPCSTR)66, FALSE); /* ord 66 = shell32.IsNetDrive */
  return pIsNetDrive(dwDriveNum);
#endif
}

/*************************************************************************
 * PathIsNetworkPathW [SHLWAPI.@]
 *
 * See PathIsNetworkPathA.
 */
BOOL WINAPI PathIsNetworkPathW(LPCWSTR lpszPath)
{
  int dwDriveNum;

  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (!lpszPath)
    return FALSE;
  if (*lpszPath == '\\' && lpszPath[1] == '\\')
    return TRUE;
  dwDriveNum = PathGetDriveNumberW(lpszPath);
  if (dwDriveNum == -1)
    return FALSE;
#ifdef __REACTOS__
  return IsNetDrive(dwDriveNum);
#else
  GET_FUNC(pIsNetDrive, shell32, (LPCSTR)66, FALSE); /* ord 66 = shell32.IsNetDrive */
  return pIsNetDrive(dwDriveNum);
#endif
}

/*************************************************************************
 * PathIsDirectoryEmptyA [SHLWAPI.@]
 *
 * Determine if a given directory is empty.
 *
 * PARAMS
 *  lpszPath [I] Directory to check
 *
 * RETURNS
 *  TRUE  If the directory exists and contains no files,
 *  FALSE Otherwise
 */
BOOL WINAPI PathIsDirectoryEmptyA(LPCSTR lpszPath)
{
  BOOL bRet = FALSE;

  TRACE("(%s)\n",debugstr_a(lpszPath));

  if (lpszPath)
  {
    WCHAR szPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP,0,lpszPath,-1,szPath,MAX_PATH);
    bRet = PathIsDirectoryEmptyW(szPath);
  }
  return bRet;
}

/*************************************************************************
 * PathIsDirectoryEmptyW [SHLWAPI.@]
 *
 * See PathIsDirectoryEmptyA.
 */
BOOL WINAPI PathIsDirectoryEmptyW(LPCWSTR lpszPath)
{
  static const WCHAR szAllFiles[] = { '*', '.', '*', '\0' };
  WCHAR szSearch[MAX_PATH];
  DWORD dwLen;
  HANDLE hfind;
  BOOL retVal = TRUE;
  WIN32_FIND_DATAW find_data;

  TRACE("(%s)\n",debugstr_w(lpszPath));

  if (!lpszPath || !PathIsDirectoryW(lpszPath))
    return FALSE;

  lstrcpynW(szSearch, lpszPath, MAX_PATH);
  PathAddBackslashW(szSearch);
  dwLen = lstrlenW(szSearch);
  if (dwLen > MAX_PATH - 4)
    return FALSE;

  lstrcpyW(szSearch + dwLen, szAllFiles);
  hfind = FindFirstFileW(szSearch, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
    return FALSE;

  do
  {
    if (find_data.cFileName[0] == '.')
    {
      if (find_data.cFileName[1] == '\0') continue;
      if (find_data.cFileName[1] == '.' && find_data.cFileName[2] == '\0') continue;
    }

    retVal = FALSE;
    break;
  }
  while (FindNextFileW(hfind, &find_data));

  FindClose(hfind);
  return retVal;
}


/*************************************************************************
 * PathFindSuffixArrayA [SHLWAPI.@]
 *
 * Find a suffix string in an array of suffix strings
 *
 * PARAMS
 *  lpszSuffix [I] Suffix string to search for
 *  lppszArray [I] Array of suffix strings to search
 *  dwCount    [I] Number of elements in lppszArray
 *
 * RETURNS
 *  Success: The index of the position of lpszSuffix in lppszArray
 *  Failure: 0, if any parameters are invalid or lpszSuffix is not found
 *
 * NOTES
 *  The search is case sensitive.
 *  The match is made against the end of the suffix string, so for example:
 *  lpszSuffix="fooBAR" matches "BAR", but lpszSuffix="fooBARfoo" does not.
 */
LPCSTR WINAPI PathFindSuffixArrayA(LPCSTR lpszSuffix, LPCSTR *lppszArray, int dwCount)
{
  size_t dwLen;
  int dwRet = 0;

  TRACE("(%s,%p,%d)\n",debugstr_a(lpszSuffix), lppszArray, dwCount);

  if (lpszSuffix && lppszArray && dwCount > 0)
  {
    dwLen = strlen(lpszSuffix);

    while (dwRet < dwCount)
    {
      size_t dwCompareLen = strlen(*lppszArray);
      if (dwCompareLen < dwLen)
      {
        if (!strcmp(lpszSuffix + dwLen - dwCompareLen, *lppszArray))
          return *lppszArray; /* Found */
      }
      dwRet++;
      lppszArray++;
    }
  }
  return NULL;
}

/*************************************************************************
 * PathFindSuffixArrayW [SHLWAPI.@]
 *
 * See PathFindSuffixArrayA.
 */
LPCWSTR WINAPI PathFindSuffixArrayW(LPCWSTR lpszSuffix, LPCWSTR *lppszArray, int dwCount)
{
  size_t dwLen;
  int dwRet = 0;

  TRACE("(%s,%p,%d)\n",debugstr_w(lpszSuffix), lppszArray, dwCount);

  if (lpszSuffix && lppszArray && dwCount > 0)
  {
    dwLen = lstrlenW(lpszSuffix);

    while (dwRet < dwCount)
    {
      size_t dwCompareLen = lstrlenW(*lppszArray);
      if (dwCompareLen < dwLen)
      {
        if (!wcscmp(lpszSuffix + dwLen - dwCompareLen, *lppszArray))
          return *lppszArray; /* Found */
      }
      dwRet++;
      lppszArray++;
    }
  }
  return NULL;
}

/*************************************************************************
 * PathUndecorateA [SHLWAPI.@]
 *
 * Undecorate a file path
 *
 * PARAMS
 *  lpszPath [I/O] Path to remove any decoration from
 *
 * RETURNS
 *  Nothing
 *
 * NOTES
 *  A decorations form is "path[n].ext" where "n" is an optional decimal number.
 */
void WINAPI PathUndecorateA(char *path)
{
  char *ext, *skip;

  TRACE("(%s)\n", debugstr_a(path));

  if (!path) return;

  ext = PathFindExtensionA(path);
  if (ext == path || ext[-1] != ']') return;

  skip = ext - 2;
  while (skip > path && '0' <= *skip && *skip <= '9')
      skip--;

  if (skip > path && *skip == '[' && skip[-1] != '\\')
      memmove(skip, ext, strlen(ext) + 1);
}

/*************************************************************************
 * PathUndecorateW [SHLWAPI.@]
 *
 * See PathUndecorateA.
 */
void WINAPI PathUndecorateW(WCHAR *path)
{
  WCHAR *ext, *skip;

  TRACE("(%s)\n", debugstr_w(path));

  if (!path) return;

  ext = PathFindExtensionW(path);
  if (ext == path || ext[-1] != ']') return;

  skip = ext - 2;
  while (skip > path && '0' <= *skip && *skip <= '9')
      skip--;

  if (skip > path && *skip == '[' && skip[-1] != '\\')
      memmove(skip, ext, (wcslen(ext) + 1) * sizeof(WCHAR));
}

/*************************************************************************
 * @     [SHLWAPI.440]
 *
 * Find localised or default web content in "%WINDOWS%\web\".
 *
 * PARAMS
 *  lpszFile  [I] File name containing content to look for
 *  lpszPath  [O] Buffer to contain the full path to the file
 *  dwPathLen [I] Length of lpszPath
 *
 * RETURNS
 *  Success: S_OK. lpszPath contains the full path to the content.
 *  Failure: E_FAIL. The content does not exist or lpszPath is too short.
 */
HRESULT WINAPI SHGetWebFolderFilePathA(LPCSTR lpszFile, LPSTR lpszPath, DWORD dwPathLen)
{
  WCHAR szFile[MAX_PATH], szPath[MAX_PATH];
  HRESULT hRet;

  TRACE("(%s,%p,%ld)\n", lpszFile, lpszPath, dwPathLen);

  MultiByteToWideChar(CP_ACP, 0, lpszFile, -1, szFile, MAX_PATH);
  szPath[0] = '\0';
  hRet = SHGetWebFolderFilePathW(szFile, szPath, dwPathLen);
  WideCharToMultiByte(CP_ACP, 0, szPath, -1, lpszPath, dwPathLen, 0, 0);
  return hRet;
}

/*************************************************************************
 * @     [SHLWAPI.441]
 *
 * Unicode version of SHGetWebFolderFilePathA.
 */
HRESULT WINAPI SHGetWebFolderFilePathW(LPCWSTR lpszFile, LPWSTR lpszPath, DWORD dwPathLen)
{
  static const WCHAR szWeb[] = {'\\','W','e','b','\\','\0'};
  static const WCHAR szWebMui[] = {'m','u','i','\\','%','0','4','x','\\','\0'};
  DWORD dwLen, dwFileLen;
  LANGID lidSystem, lidUser;

  TRACE("(%s,%p,%ld)\n", debugstr_w(lpszFile), lpszPath, dwPathLen);

  /* Get base directory for web content */
  dwLen = GetSystemWindowsDirectoryW(lpszPath, dwPathLen);
  if (dwLen > 0 && lpszPath[dwLen-1] == '\\')
    dwLen--;

  dwFileLen = lstrlenW(lpszFile);

  if (dwLen + dwFileLen + ARRAY_SIZE(szWeb) >= dwPathLen)
    return E_FAIL; /* lpszPath too short */

  lstrcpyW(lpszPath+dwLen, szWeb);
  dwLen += ARRAY_SIZE(szWeb);
  dwPathLen = dwPathLen - dwLen; /* Remaining space */

  lidSystem = GetSystemDefaultUILanguage();
  lidUser = GetUserDefaultUILanguage();

  if (lidSystem != lidUser)
  {
    if (dwFileLen + ARRAY_SIZE(szWebMui) < dwPathLen)
    {
      /* Use localised content in the users UI language if present */
      wsprintfW(lpszPath + dwLen, szWebMui, lidUser);
      lstrcpyW(lpszPath + dwLen + ARRAY_SIZE(szWebMui), lpszFile);
      if (PathFileExistsW(lpszPath))
        return S_OK;
    }
  }

  /* Fall back to OS default installed content */
  lstrcpyW(lpszPath + dwLen, lpszFile);
  if (PathFileExistsW(lpszPath))
    return S_OK;
  return E_FAIL;
}
