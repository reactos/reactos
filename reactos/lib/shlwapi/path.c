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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winnls.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

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
extern HMODULE SHLWAPI_hshell32;

/* Function pointers for GET_FUNC macro; these need to be global because of gcc bug */
typedef BOOL (WINAPI *fnpIsNetDrive)(int);
static  fnpIsNetDrive pIsNetDrive;

HRESULT WINAPI SHGetWebFolderFilePathW(LPCWSTR,LPWSTR,DWORD);

/*************************************************************************
 * PathAppendA    [SHLWAPI.@]
 *
 * Append one path to another.
 *
 * PARAMS
 *  lpszPath   [I/O] Initial part of path, and destination for output
 *  lpszAppend [I]   Path to append
 *
 * RETURNS
 *  Success: TRUE. lpszPath contains the newly created path.
 *  Failure: FALSE, if either path is NULL, or PathCombineA() fails.
 *
 * NOTES
 *  lpszAppend must contain at least one backslash ('\') if not NULL.
 *  Because PathCombineA() is used to join the paths, the resulting
 *  path is also canonicalized.
 */
BOOL WINAPI PathAppendA (LPSTR lpszPath, LPCSTR lpszAppend)
{
  TRACE("(%s,%s)\n",debugstr_a(lpszPath), debugstr_a(lpszAppend));

  if (lpszPath && lpszAppend)
  {
    if (!PathIsUNCA(lpszAppend))
      while (*lpszAppend == '\\')
        lpszAppend++;
    if (PathCombineA(lpszPath, lpszPath, lpszAppend))
      return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 * PathAppendW    [SHLWAPI.@]
 *
 * See PathAppendA.
 */
BOOL WINAPI PathAppendW(LPWSTR lpszPath, LPCWSTR lpszAppend)
{
  TRACE("(%s,%s)\n",debugstr_w(lpszPath), debugstr_w(lpszAppend));

  if (lpszPath && lpszAppend)
  {
    if (!PathIsUNCW(lpszAppend))
      while (*lpszAppend == '\\')
        lpszAppend++;
    if (PathCombineW(lpszPath, lpszPath, lpszAppend))
      return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 * PathCombineA		[SHLWAPI.@]
 *
 * Combine two paths together.
 *
 * PARAMS
 *  lpszDest [O] Destination for combined path
 *  lpszDir  [I] Directory path
 *  lpszFile [I] File path
 *
 * RETURNS
 *  Success: The output path
 *  Failure: NULL, if inputs are invalid.
 *
 * NOTES
 *  lpszDest should be at least MAX_PATH in size, and may point to the same
 *  memory location as lpszDir. The combined path is canonicalised.
 */
LPSTR WINAPI PathCombineA(LPSTR lpszDest, LPCSTR lpszDir, LPCSTR lpszFile)
{
  TRACE("(%p,%s,%s)\n", lpszDest, debugstr_a(lpszDir), debugstr_a(lpszFile));

  if (!lpszDest || (!lpszDir && !lpszFile))
    return NULL; /* Invalid parameters */
  else
  {
    WCHAR szDest[MAX_PATH];
    WCHAR szDir[MAX_PATH];
    WCHAR szFile[MAX_PATH];
    if (lpszDir)
      MultiByteToWideChar(0,0,lpszDir,-1,szDir,MAX_PATH);
    if (lpszFile)
      MultiByteToWideChar(0,0,lpszFile,-1,szFile,MAX_PATH);
    PathCombineW(szDest, lpszDir ? szDir : NULL, lpszFile ? szFile : NULL);
    WideCharToMultiByte(0,0,szDest,-1,lpszDest,MAX_PATH,0,0);
  }
  return lpszDest;
}

/*************************************************************************
 * PathCombineW		 [SHLWAPI.@]
 *
 * See PathCombineA.
 */
LPWSTR WINAPI PathCombineW(LPWSTR lpszDest, LPCWSTR lpszDir, LPCWSTR lpszFile)
{
  WCHAR szTemp[MAX_PATH];
  BOOL bUseBoth = FALSE, bStrip = FALSE;

  TRACE("(%p,%s,%s)\n", lpszDest, debugstr_w(lpszDir), debugstr_w(lpszFile));

  if (!lpszDest || (!lpszDir && !lpszFile))
    return lpszDest; /* Invalid parameters */

  if (!lpszFile || !*lpszFile)
  {
    /* Use dir only */
    strncpyW(szTemp, lpszDir, MAX_PATH);
  }
  else if (!lpszDir || !*lpszDir || !PathIsRelativeW(lpszFile))
  {
    if (!lpszDir || !*lpszDir || *lpszFile != '\\' || PathIsUNCW(lpszFile))
    {
      /* Use file only */
      strncpyW(szTemp, lpszFile, MAX_PATH);
    }
    else
    {
      bUseBoth = TRUE;
      bStrip = TRUE;
    }
  }
  else
    bUseBoth = TRUE;

  if (bUseBoth)
  {
    strncpyW(szTemp, lpszDir, MAX_PATH);
    if (bStrip)
    {
      PathStripToRootW(szTemp);
      lpszFile++; /* Skip '\' */
    }
    if (!PathAddBackslashW(szTemp))
      return NULL;
    if (strlenW(szTemp) + strlenW(lpszFile) >= MAX_PATH)
      return NULL;
    strcatW(szTemp, lpszFile);
  }

  PathCanonicalizeW(lpszDest, szTemp);
  return lpszDest;
}

/*************************************************************************
 * PathAddBackslashA	[SHLWAPI.@]
 *
 * Append a backslash ('\') to a path if one doesn't exist.
 *
 * PARAMS
 *  lpszPath [I/O] The path to append a backslash to.
 *
 * RETURNS
 *  Success: The position of the last backslash in the path.
 *  Failure: NULL, if lpszPath is NULL or the path is too large.
 */
LPSTR WINAPI PathAddBackslashA(LPSTR lpszPath)
{
  size_t iLen;

  TRACE("(%s)\n",debugstr_a(lpszPath));

  if (!lpszPath || (iLen = strlen(lpszPath)) >= MAX_PATH)
    return NULL;

  if (iLen)
  {
    lpszPath += iLen;
    if (lpszPath[-1] != '\\')
    {
     *lpszPath++ = '\\';
     *lpszPath = '\0';
    }
  }
  return lpszPath;
}

/*************************************************************************
 * PathAddBackslashW  [SHLWAPI.@]
 *
 * See PathAddBackslashA.
 */
LPWSTR WINAPI PathAddBackslashW( LPWSTR lpszPath )
{
  size_t iLen;

  TRACE("(%s)\n",debugstr_w(lpszPath));

  if (!lpszPath || (iLen = strlenW(lpszPath)) >= MAX_PATH)
    return NULL;

  if (iLen)
  {
    lpszPath += iLen;
    if (lpszPath[-1] != '\\')
    {
      *lpszPath++ = '\\';
      *lpszPath = '\0';
    }
  }
  return lpszPath;
}

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
 * PathFindFileNameA  [SHLWAPI.@]
 *
 * Locate the start of the file name in a path
 *
 * PARAMS
 *  lpszPath [I] Path to search
 *
 * RETURNS
 *  A pointer to the first character of the file name
 */
LPSTR WINAPI PathFindFileNameA(LPCSTR lpszPath)
{
  LPCSTR lastSlash = lpszPath;

  TRACE("(%s)\n",debugstr_a(lpszPath));

  while (lpszPath && *lpszPath)
  {
    if ((*lpszPath == '\\' || *lpszPath == '/' || *lpszPath == ':') &&
        lpszPath[1] && lpszPath[1] != '\\' && lpszPath[1] != '/')
      lastSlash = lpszPath + 1;
    lpszPath = CharNextA(lpszPath);
  }
  return (LPSTR)lastSlash;
}

/*************************************************************************
 * PathFindFileNameW  [SHLWAPI.@]
 *
 * See PathFindFileNameA.
 */
LPWSTR WINAPI PathFindFileNameW(LPCWSTR lpszPath)
{
  LPCWSTR lastSlash = lpszPath;

  TRACE("(%s)\n",debugstr_w(lpszPath));

  while (lpszPath && *lpszPath)
  {
    if ((*lpszPath == '\\' || *lpszPath == '/' || *lpszPath == ':') &&
        lpszPath[1] && lpszPath[1] != '\\' && lpszPath[1] != '/')
      lastSlash = lpszPath + 1;
    lpszPath = CharNextW(lpszPath);
  }
  return (LPWSTR)lastSlash;
}

/*************************************************************************
 * PathFindExtensionA  [SHLWAPI.@]
 *
 * Locate the start of the file extension in a path
 *
 * PARAMS
 *  lpszPath [I] The path to search
 *
 * RETURNS
 *  A pointer to the first character of the extension, the end of
 *  the string if the path has no extension, or NULL If lpszPath is NULL
 */
LPSTR WINAPI PathFindExtensionA( LPCSTR lpszPath )
{
  LPCSTR lastpoint = NULL;

  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (lpszPath)
  {
    while (*lpszPath)
    {
      if (*lpszPath == '\\' || *lpszPath==' ')
        lastpoint = NULL;
      else if (*lpszPath == '.')
        lastpoint = lpszPath;
      lpszPath = CharNextA(lpszPath);
    }
  }
  return (LPSTR)(lastpoint ? lastpoint : lpszPath);
}

/*************************************************************************
 * PathFindExtensionW  [SHLWAPI.@]
 *
 * See PathFindExtensionA.
 */
LPWSTR WINAPI PathFindExtensionW( LPCWSTR lpszPath )
{
  LPCWSTR lastpoint = NULL;

  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (lpszPath)
  {
    while (*lpszPath)
    {
      if (*lpszPath == '\\' || *lpszPath==' ')
        lastpoint = NULL;
      else if (*lpszPath == '.')
        lastpoint = lpszPath;
      lpszPath = CharNextW(lpszPath);
    }
  }
  return (LPWSTR)(lastpoint ? lastpoint : lpszPath);
}

/*************************************************************************
 * PathGetArgsA    [SHLWAPI.@]
 *
 * Find the next argument in a string delimited by spaces.
 *
 * PARAMS
 *  lpszPath [I] The string to search for arguments in
 *
 * RETURNS
 *  The start of the next argument in lpszPath, or NULL if lpszPath is NULL
 *
 * NOTES
 *  Spaces in quoted strings are ignored as delimiters.
 */
LPSTR WINAPI PathGetArgsA(LPCSTR lpszPath)
{
  BOOL bSeenQuote = FALSE;

  TRACE("(%s)\n",debugstr_a(lpszPath));

  if (lpszPath)
  {
    while (*lpszPath)
    {
      if ((*lpszPath==' ') && !bSeenQuote)
        return (LPSTR)lpszPath + 1;
      if (*lpszPath == '"')
        bSeenQuote = !bSeenQuote;
      lpszPath = CharNextA(lpszPath);
    }
  }
  return (LPSTR)lpszPath;
}

/*************************************************************************
 * PathGetArgsW    [SHLWAPI.@]
 *
 * See PathGetArgsA.
 */
LPWSTR WINAPI PathGetArgsW(LPCWSTR lpszPath)
{
  BOOL bSeenQuote = FALSE;

  TRACE("(%s)\n",debugstr_w(lpszPath));

  if (lpszPath)
  {
    while (*lpszPath)
    {
      if ((*lpszPath==' ') && !bSeenQuote)
        return (LPWSTR)lpszPath + 1;
      if (*lpszPath == '"')
        bSeenQuote = !bSeenQuote;
      lpszPath = CharNextW(lpszPath);
    }
  }
  return (LPWSTR)lpszPath;
}

/*************************************************************************
 * PathGetDriveNumberA	[SHLWAPI.@]
 *
 * Return the drive number from a path
 *
 * PARAMS
 *  lpszPath [I] Path to get the drive number from
 *
 * RETURNS
 *  Success: The drive number corresponding to the drive in the path
 *  Failure: -1, if lpszPath contains no valid drive
 */
int WINAPI PathGetDriveNumberA(LPCSTR lpszPath)
{
  TRACE ("(%s)\n",debugstr_a(lpszPath));

  if (lpszPath && !IsDBCSLeadByte(*lpszPath) && lpszPath[1] == ':' &&
      tolower(*lpszPath) >= 'a' && tolower(*lpszPath) <= 'z')
    return tolower(*lpszPath) - 'a';
  return -1;
}

/*************************************************************************
 * PathGetDriveNumberW	[SHLWAPI.@]
 *
 * See PathGetDriveNumberA.
 */
int WINAPI PathGetDriveNumberW(LPCWSTR lpszPath)
{
  TRACE ("(%s)\n",debugstr_w(lpszPath));

  if (lpszPath && lpszPath[1] == ':' &&
      tolowerW(*lpszPath) >= 'a' && tolowerW(*lpszPath) <= 'z')
    return tolowerW(*lpszPath) - 'a';
  return -1;
}

/*************************************************************************
 * PathRemoveFileSpecA	[SHLWAPI.@]
 *
 * Remove the file specification from a path.
 *
 * PARAMS
 *  lpszPath [I/O] Path to remove the file spec from
 *
 * RETURNS
 *  TRUE  If the path was valid and modified
 *  FALSE Otherwise
 */
BOOL WINAPI PathRemoveFileSpecA(LPSTR lpszPath)
{
  LPSTR lpszFileSpec = lpszPath;
  BOOL bModified = FALSE;

  TRACE("(%s)\n",debugstr_a(lpszPath));

  if(lpszPath)
  {
    /* Skip directory or UNC path */
    if (*lpszPath == '\\')
      lpszFileSpec = ++lpszPath;
    if (*lpszPath == '\\')
      lpszFileSpec = ++lpszPath;

    while (*lpszPath)
    {
      if(*lpszPath == '\\')
        lpszFileSpec = lpszPath; /* Skip dir */
      else if(*lpszPath == ':')
      {
        lpszFileSpec = ++lpszPath; /* Skip drive */
        if (*lpszPath == '\\')
          lpszFileSpec++;
      }
      if (!(lpszPath = CharNextA(lpszPath)))
        break;
    }

    if (*lpszFileSpec)
    {
      *lpszFileSpec = '\0';
      bModified = TRUE;
    }
  }
  return bModified;
}

/*************************************************************************
 * PathRemoveFileSpecW	[SHLWAPI.@]
 *
 * See PathRemoveFileSpecA.
 */
BOOL WINAPI PathRemoveFileSpecW(LPWSTR lpszPath)
{
  LPWSTR lpszFileSpec = lpszPath;
  BOOL bModified = FALSE;

  TRACE("(%s)\n",debugstr_w(lpszPath));

  if(lpszPath)
  {
    /* Skip directory or UNC path */
    if (*lpszPath == '\\')
      lpszFileSpec = ++lpszPath;
    if (*lpszPath == '\\')
      lpszFileSpec = ++lpszPath;

    while (*lpszPath)
    {
      if(*lpszPath == '\\')
        lpszFileSpec = lpszPath; /* Skip dir */
      else if(*lpszPath == ':')
      {
        lpszFileSpec = ++lpszPath; /* Skip drive */
        if (*lpszPath == '\\')
          lpszFileSpec++;
      }
      if (!(lpszPath = CharNextW(lpszPath)))
        break;
    }

    if (*lpszFileSpec)
    {
      *lpszFileSpec = '\0';
      bModified = TRUE;
    }
  }
  return bModified;
}

/*************************************************************************
 * PathStripPathA	[SHLWAPI.@]
 *
 * Remove the initial path from the beginning of a filename
 *
 * PARAMS
 *  lpszPath [I/O] Path to remove the initial path from
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI PathStripPathA(LPSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (lpszPath)
  {
    LPSTR lpszFileName = PathFindFileNameA(lpszPath);
    if(lpszFileName)
      RtlMoveMemory(lpszPath, lpszFileName, strlen(lpszFileName)+1);
  }
}

/*************************************************************************
 * PathStripPathW	[SHLWAPI.@]
 *
 * See PathStripPathA.
 */
void WINAPI PathStripPathW(LPWSTR lpszPath)
{
  LPWSTR lpszFileName;

  TRACE("(%s)\n", debugstr_w(lpszPath));
  lpszFileName = PathFindFileNameW(lpszPath);
  if(lpszFileName)
    RtlMoveMemory(lpszPath, lpszFileName, (strlenW(lpszFileName)+1)*sizeof(WCHAR));
}

/*************************************************************************
 * PathStripToRootA	[SHLWAPI.@]
 *
 * Reduce a path to its root.
 *
 * PARAMS
 *  lpszPath [I/O] the path to reduce
 *
 * RETURNS
 *  Success: TRUE if the stripped path is a root path
 *  Failure: FALSE if the path cannot be stripped or is NULL
 */
BOOL WINAPI PathStripToRootA(LPSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (!lpszPath)
    return FALSE;
  while(!PathIsRootA(lpszPath))
    if (!PathRemoveFileSpecA(lpszPath))
      return FALSE;
  return TRUE;
}

/*************************************************************************
 * PathStripToRootW	[SHLWAPI.@]
 *
 * See PathStripToRootA.
 */
BOOL WINAPI PathStripToRootW(LPWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (!lpszPath)
    return FALSE;
  while(!PathIsRootW(lpszPath))
    if (!PathRemoveFileSpecW(lpszPath))
      return FALSE;
  return TRUE;
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
    if (*lpszArgs)
      lpszArgs[-1] = '\0';
    else
    {
      LPWSTR lpszLastChar = CharPrevW(lpszPath, lpszArgs);
      if(*lpszLastChar == ' ')
        *lpszLastChar = '\0';
    }
  }
}

/*************************************************************************
 * PathRemoveExtensionA		[SHLWAPI.@]
 *
 * Remove the file extension from a path
 *
 * PARAMS
 *  lpszPath [I/O] Path to remove the extension from
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI PathRemoveExtensionA(LPSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (lpszPath)
  {
    lpszPath = PathFindExtensionA(lpszPath);
    *lpszPath = '\0';
  }
}

/*************************************************************************
 * PathRemoveExtensionW		[SHLWAPI.@]
 *
 * See PathRemoveExtensionA.
*/
void WINAPI PathRemoveExtensionW(LPWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (lpszPath)
  {
    lpszPath = PathFindExtensionW(lpszPath);
    *lpszPath = '\0';
  }
}

/*************************************************************************
 * PathRemoveBackslashA	[SHLWAPI.@]
 *
 * Remove a trailing backslash from a path.
 *
 * PARAMS
 *  lpszPath [I/O] Path to remove backslash from
 *
 * RETURNS
 *  Success: A pointer to the end of the path
 *  Failure: NULL, if lpszPath is NULL
 */
LPSTR WINAPI PathRemoveBackslashA( LPSTR lpszPath )
{
  LPSTR szTemp = NULL;

  TRACE("(%s)\n", debugstr_a(lpszPath));

  if(lpszPath)
  {
    szTemp = CharPrevA(lpszPath, lpszPath + strlen(lpszPath));
    if (!PathIsRootA(lpszPath) && *szTemp == '\\')
      *szTemp = '\0';
  }
  return szTemp;
}

/*************************************************************************
 * PathRemoveBackslashW	[SHLWAPI.@]
 *
 * See PathRemoveBackslashA.
 */
LPWSTR WINAPI PathRemoveBackslashW( LPWSTR lpszPath )
{
  LPWSTR szTemp = NULL;

  TRACE("(%s)\n", debugstr_w(lpszPath));

  if(lpszPath)
  {
    szTemp = CharPrevW(lpszPath, lpszPath + strlenW(lpszPath));
    if (!PathIsRootW(lpszPath) && *szTemp == '\\')
      *szTemp = '\0';
  }
  return szTemp;
}

/*************************************************************************
 * PathRemoveBlanksA [SHLWAPI.@]
 *
 * Remove Spaces from the start and end of a path.
 *
 * PARAMS
 *  lpszPath [I/O] Path to strip blanks from
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI PathRemoveBlanksA(LPSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if(lpszPath && *lpszPath)
  {
    LPSTR start = lpszPath;

    while (*lpszPath == ' ')
      lpszPath = CharNextA(lpszPath);

    while(*lpszPath)
      *start++ = *lpszPath++;

    if (start != lpszPath)
      while (start[-1] == ' ')
        start--;
    *start = '\0';
  }
}

/*************************************************************************
 * PathRemoveBlanksW [SHLWAPI.@]
 *
 * See PathRemoveBlanksA.
 */
VOID WINAPI PathRemoveBlanksW(LPWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if(lpszPath && *lpszPath)
  {
    LPWSTR start = lpszPath;

    while (*lpszPath == ' ')
      lpszPath++;

    while(*lpszPath)
      *start++ = *lpszPath++;

    if (start != lpszPath)
      while (start[-1] == ' ')
        start--;
    *start = '\0';
  }
}

/*************************************************************************
 * PathQuoteSpacesA [SHLWAPI.@]
 *
 * Surround a path containg spaces in quotes.
 *
 * PARAMS
 *  lpszPath [I/O] Path to quote
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  The path is not changed if it is invalid or has no spaces.
 */
VOID WINAPI PathQuoteSpacesA(LPSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if(lpszPath && StrChrA(lpszPath,' '))
  {
    size_t iLen = strlen(lpszPath) + 1;

    if (iLen + 2 < MAX_PATH)
    {
      memmove(lpszPath + 1, lpszPath, iLen);
      lpszPath[0] = '"';
      lpszPath[iLen] = '"';
      lpszPath[iLen + 1] = '\0';
    }
  }
}

/*************************************************************************
 * PathQuoteSpacesW [SHLWAPI.@]
 *
 * See PathQuoteSpacesA.
 */
VOID WINAPI PathQuoteSpacesW(LPWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if(lpszPath && StrChrW(lpszPath,' '))
  {
    int iLen = strlenW(lpszPath) + 1;

    if (iLen + 2 < MAX_PATH)
    {
      memmove(lpszPath + 1, lpszPath, iLen * sizeof(WCHAR));
      lpszPath[0] = '"';
      lpszPath[iLen] = '"';
      lpszPath[iLen + 1] = '\0';
    }
  }
}

/*************************************************************************
 * PathUnquoteSpacesA [SHLWAPI.@]
 *
 * Remove quotes ("") from around a path, if present.
 *
 * PARAMS
 *  lpszPath [I/O] Path to strip quotes from
 *
 * RETURNS
 *  Nothing
 *
 * NOTES
 *  If the path contains a single quote only, an empty string will result.
 *  Otherwise quotes are only removed if they appear at the start and end
 *  of the path.
 */
VOID WINAPI PathUnquoteSpacesA(LPSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (lpszPath && *lpszPath == '"')
  {
    DWORD dwLen = strlen(lpszPath) - 1;

    if (lpszPath[dwLen] == '"')
    {
      lpszPath[dwLen] = '\0';
      for (; *lpszPath; lpszPath++)
        *lpszPath = lpszPath[1];
    }
  }
}

/*************************************************************************
 * PathUnquoteSpacesW [SHLWAPI.@]
 *
 * See PathUnquoteSpacesA.
 */
VOID WINAPI PathUnquoteSpacesW(LPWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (lpszPath && *lpszPath == '"')
  {
    DWORD dwLen = strlenW(lpszPath) - 1;

    if (lpszPath[dwLen] == '"')
    {
      lpszPath[dwLen] = '\0';
      for (; *lpszPath; lpszPath++)
        *lpszPath = lpszPath[1];
    }
  }
}

/*************************************************************************
 * PathParseIconLocationA  [SHLWAPI.@]
 *
 * Parse the location of an icon from a path.
 *
 * PARAMS
 *  lpszPath [I/O] The path to parse the icon location from.
 *
 * RETURNS
 *  Success: The number of the icon
 *  Failure: 0 if the path does not contain an icon location or is NULL
 *
 * NOTES
 *  The path has surrounding quotes and spaces removed regardless
 *  of whether the call succeeds or not.
 */
int WINAPI PathParseIconLocationA(LPSTR lpszPath)
{
  int iRet = 0;
  LPSTR lpszComma;

  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (lpszPath)
  {
    if ((lpszComma = strchr(lpszPath, ',')))
    {
      *lpszComma++ = '\0';
      iRet = StrToIntA(lpszComma);
    }
    PathUnquoteSpacesA(lpszPath);
    PathRemoveBlanksA(lpszPath);
  }
  return iRet;
}

/*************************************************************************
 * PathParseIconLocationW  [SHLWAPI.@]
 *
 * See PathParseIconLocationA.
 */
int WINAPI PathParseIconLocationW(LPWSTR lpszPath)
{
  int iRet = 0;
  LPWSTR lpszComma;

  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (lpszPath)
  {
    if ((lpszComma = StrChrW(lpszPath, ',')))
    {
      *lpszComma++ = '\0';
      iRet = StrToIntW(lpszComma);
    }
    PathUnquoteSpacesW(lpszPath);
    PathRemoveBlanksW(lpszPath);
  }
  return iRet;
}

/*************************************************************************
 * @	[SHLWAPI.4]
 *
 * Unicode version of PathFileExistsDefExtA.
 */
BOOL WINAPI PathFileExistsDefExtW(LPWSTR lpszPath,DWORD dwWhich)
{
  static const WCHAR pszExts[7][5] = { { '.', 'p', 'i', 'f', 0},
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
    if (!*szExt || dwWhich & 0x40)
    {
      size_t iChoose = 0;
      int iLen = lstrlenW(lpszPath);
      if (iLen > (MAX_PATH - 5))
        return FALSE;
      while (dwWhich & 0x1 && iChoose < sizeof(pszExts))
      {
        lstrcpyW(lpszPath + iLen, pszExts[iChoose]);
        if (PathFileExistsW(lpszPath))
          return TRUE;
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
 *  If the function returns FALSE, the path is modified to its orginal state.
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
    MultiByteToWideChar(0,0,lpszPath,-1,szPath,MAX_PATH);
    bRet = PathFileExistsDefExtW(szPath, dwWhich);
    if (bRet)
      WideCharToMultiByte(0,0,szPath,-1,lpszPath,MAX_PATH,0,0);
  }
  return bRet;
}

/*************************************************************************
 * SHLWAPI_PathFindInOtherDirs
 *
 * Internal helper for SHLWAPI_PathFindOnPathExA/W.
 */
static BOOL WINAPI SHLWAPI_PathFindInOtherDirs(LPWSTR lpszFile, DWORD dwWhich)
{
  static WCHAR szSystem[] = { 'S','y','s','t','e','m','\0'};
  static WCHAR szPath[] = { 'P','A','T','H','\0'};
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
    strcpyW(lpszFile, buff);
    return TRUE;
  }
  GetWindowsDirectoryW(buff, MAX_PATH);
  if (!PathAppendW(buff, szSystem ) || !PathAppendW(buff, lpszFile))
    return FALSE;
  if (PathFileExistsDefExtW(buff, dwWhich))
  {
    strcpyW(lpszFile, buff);
    return TRUE;
  }
  GetWindowsDirectoryW(buff, MAX_PATH);
  if (!PathAppendW(buff, lpszFile))
    return FALSE;
  if (PathFileExistsDefExtW(buff, dwWhich))
  {
    strcpyW(lpszFile, buff);
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
      strcpyW(lpszFile, buff);
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

  MultiByteToWideChar(0,0,lpszFile,-1,szFile,MAX_PATH);

  /* Search provided directories first */
  if (lppszOtherDirs && *lppszOtherDirs)
  {
    WCHAR szOther[MAX_PATH];
    LPCSTR *lpszOtherPath = lppszOtherDirs;

    while (lpszOtherPath && *lpszOtherPath && (*lpszOtherPath)[0])
    {
      MultiByteToWideChar(0,0,*lpszOtherPath,-1,szOther,MAX_PATH);
      PathCombineW(buff, szOther, szFile);
      if (PathFileExistsDefExtW(buff, dwWhich))
      {
        WideCharToMultiByte(0,0,buff,-1,lpszFile,MAX_PATH,0,0);
        return TRUE;
      }
      lpszOtherPath++;
    }
  }
  /* Not found, try system and path dirs */
  if (SHLWAPI_PathFindInOtherDirs(szFile, dwWhich))
  {
    WideCharToMultiByte(0,0,szFile,-1,lpszFile,MAX_PATH,0,0);
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
        strcpyW(lpszFile, buff);
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

    MultiByteToWideChar(0,0,lpszPath,-1,szPath,MAX_PATH);
    szDest[0] = '\0';
    bRet = PathCompactPathExW(szDest, szPath, cchMax, dwFlags);
    WideCharToMultiByte(0,0,szDest,-1,lpszDest,MAX_PATH,0,0);
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

  dwLen = strlenW(lpszPath) + 1;

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
    strcpyW(lpszDest + cchMax, szEllipses);
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
    strcpyW(lpszDest, szEllipses);
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
    strcpyW(lpszDest + cchMax, szEllipses);
    return TRUE;
  }

  /* Only the root needs to be Compacted */
  dwLen = cchMax - dwFileLen - 3;
  memcpy(lpszDest, lpszPath, dwLen * sizeof(WCHAR));
  strcpyW(lpszDest + dwLen, szEllipses);
  strcpyW(lpszDest + dwLen + 3, lpszFile);
  return TRUE;
}

/*************************************************************************
 * PathIsRelativeA	[SHLWAPI.@]
 *
 * Determine if a path is a relative path.
 *
 * PARAMS
 *  lpszPath [I] Path to check
 *
 * RETURNS
 *  TRUE:  The path is relative, or is invalid.
 *  FALSE: The path is not relative.
 */
BOOL WINAPI PathIsRelativeA (LPCSTR lpszPath)
{
  TRACE("(%s)\n",debugstr_a(lpszPath));

  if (!lpszPath || !*lpszPath || IsDBCSLeadByte(*lpszPath))
    return TRUE;
  if (*lpszPath == '\\' || (*lpszPath && lpszPath[1] == ':'))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 *  PathIsRelativeW	[SHLWAPI.@]
 *
 * See PathIsRelativeA.
 */
BOOL WINAPI PathIsRelativeW (LPCWSTR lpszPath)
{
  TRACE("(%s)\n",debugstr_w(lpszPath));

  if (!lpszPath || !*lpszPath)
    return TRUE;
  if (*lpszPath == '\\' || (*lpszPath && lpszPath[1] == ':'))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 * PathIsRootA		[SHLWAPI.@]
 *
 * Determine if a path is a root path.
 *
 * PARAMS
 *  lpszPath [I] Path to check
 *
 * RETURNS
 *  TRUE  If lpszPath is valid and a root path,
 *  FALSE Otherwise
 */
BOOL WINAPI PathIsRootA(LPCSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (lpszPath && *lpszPath)
  {
    if (*lpszPath == '\\')
    {
      if (!lpszPath[1])
        return TRUE; /* \ */
      else if (lpszPath[1]=='\\')
      {
        BOOL bSeenSlash = FALSE;
        lpszPath += 2;

        /* Check for UNC root path */
        while (*lpszPath)
        {
          if (*lpszPath == '\\')
          {
            if (bSeenSlash)
              return FALSE;
            bSeenSlash = TRUE;
          }
          lpszPath = CharNextA(lpszPath);
        }
        return TRUE;
      }
    }
    else if (lpszPath[1] == ':' && lpszPath[2] == '\\' && lpszPath[3] == '\0')
      return TRUE; /* X:\ */
  }
  return FALSE;
}

/*************************************************************************
 * PathIsRootW		[SHLWAPI.@]
 *
 * See PathIsRootA.
 */
BOOL WINAPI PathIsRootW(LPCWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (lpszPath && *lpszPath)
  {
    if (*lpszPath == '\\')
    {
      if (!lpszPath[1])
        return TRUE; /* \ */
      else if (lpszPath[1]=='\\')
      {
        BOOL bSeenSlash = FALSE;
        lpszPath += 2;

        /* Check for UNC root path */
        while (*lpszPath)
        {
          if (*lpszPath == '\\')
          {
            if (bSeenSlash)
              return FALSE;
            bSeenSlash = TRUE;
          }
          lpszPath = CharNextW(lpszPath);
        }
        return TRUE;
      }
    }
    else if (lpszPath[1] == ':' && lpszPath[2] == '\\' && lpszPath[3] == '\0')
      return TRUE; /* X:\ */
  }
  return FALSE;
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
    FIXME("UNC Server Share not yet supported - FAILING\n");
    return FALSE;
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
    FIXME("UNC Server Share not yet supported - FAILING\n");
    return FALSE;
  }

  if ((dwAttr = GetFileAttributesW(lpszPath)) == INVALID_FILE_ATTRIBUTES)
    return FALSE;
  return dwAttr & FILE_ATTRIBUTE_DIRECTORY;
}

/*************************************************************************
 * PathFileExistsA	[SHLWAPI.@]
 *
 * Determine if a file exists.
 *
 * PARAMS
 *  lpszPath [I] Path to check
 *
 * RETURNS
 *  TRUE  If the file exists and is readable
 *  FALSE Otherwise
 */
BOOL WINAPI PathFileExistsA(LPCSTR lpszPath)
{
  UINT iPrevErrMode;
  DWORD dwAttr;

  TRACE("(%s)\n",debugstr_a(lpszPath));

  if (!lpszPath)
    return FALSE;

  iPrevErrMode = SetErrorMode(1);
  dwAttr = GetFileAttributesA(lpszPath);
  SetErrorMode(iPrevErrMode);
  return dwAttr == INVALID_FILE_ATTRIBUTES ? FALSE : TRUE;
}

/*************************************************************************
 * PathFileExistsW	[SHLWAPI.@]
 *
 * See PathFileExistsA.
 */
BOOL WINAPI PathFileExistsW(LPCWSTR lpszPath)
{
  UINT iPrevErrMode;
  DWORD dwAttr;

  TRACE("(%s)\n",debugstr_w(lpszPath));

  if (!lpszPath)
    return FALSE;

  iPrevErrMode = SetErrorMode(1);
  dwAttr = GetFileAttributesW(lpszPath);
  SetErrorMode(iPrevErrMode);
  return dwAttr == INVALID_FILE_ATTRIBUTES ? FALSE : TRUE;
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
 * PathMatchSingleMaskA	[internal]
 */
static BOOL WINAPI PathMatchSingleMaskA(LPCSTR name, LPCSTR mask)
{
  while (*name && *mask && *mask!=';')
  {
    if (*mask == '*')
    {
      do
      {
        if (PathMatchSingleMaskA(name,mask+1))
          return TRUE;  /* try substrings */
      } while (*name++);
      return FALSE;
    }

    if (toupper(*mask) != toupper(*name) && *mask != '?')
      return FALSE;

    name = CharNextA(name);
    mask = CharNextA(mask);
  }

  if (!*name)
  {
    while (*mask == '*')
      mask++;
    if (!*mask || *mask == ';')
      return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 * PathMatchSingleMaskW	[internal]
 */
static BOOL WINAPI PathMatchSingleMaskW(LPCWSTR name, LPCWSTR mask)
{
  while (*name && *mask && *mask != ';')
  {
    if (*mask == '*')
    {
      do
      {
        if (PathMatchSingleMaskW(name,mask+1))
          return TRUE;  /* try substrings */
      } while (*name++);
      return FALSE;
    }

    if (toupperW(*mask) != toupperW(*name) && *mask != '?')
      return FALSE;

    name = CharNextW(name);
    mask = CharNextW(mask);
  }
  if (!*name)
  {
    while (*mask == '*')
      mask++;
    if (!*mask || *mask == ';')
      return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 * PathMatchSpecA	[SHLWAPI.@]
 *
 * Determine if a path matches one or more search masks.
 *
 * PARAMS
 *  lpszPath [I] Path to check
 *  lpszMask [I] Search mask(s)
 *
 * RETURNS
 *  TRUE  If lpszPath is valid and is matched
 *  FALSE Otherwise
 *
 * NOTES
 *  Multiple search masks may be given if they are separated by ";". The
 *  pattern "*.*" is treated specially in that it matches all paths (for
 *  backwards compatibility with DOS).
 */
BOOL WINAPI PathMatchSpecA(LPCSTR lpszPath, LPCSTR lpszMask)
{
  TRACE("(%s,%s)\n", lpszPath, lpszMask);

  if (!lstrcmpA(lpszMask, "*.*"))
    return TRUE; /* Matches every path */

  while (*lpszMask)
  {
    if (PathMatchSingleMaskA(lpszPath, lpszMask))
      return TRUE; /* Matches the current mask */

    while (*lpszMask && *lpszMask != ';')
      lpszMask = CharNextA(lpszMask);

    if (*lpszMask == ';')
    {
      lpszMask++;
      while (*lpszMask == ' ')
        lpszMask++; /*  masks may be separated by "; " */
    }
  }
  return FALSE;
}

/*************************************************************************
 * PathMatchSpecW	[SHLWAPI.@]
 *
 * See PathMatchSpecA.
 */
BOOL WINAPI PathMatchSpecW(LPCWSTR lpszPath, LPCWSTR lpszMask)
{
  static const WCHAR szStarDotStar[] = { '*', '.', '*', '\0' };

  TRACE("(%s,%s)\n", debugstr_w(lpszPath), debugstr_w(lpszMask));

  if (!lstrcmpW(lpszMask, szStarDotStar))
    return TRUE; /* Matches every path */

  while (*lpszMask)
  {
    if (PathMatchSingleMaskW(lpszPath, lpszMask))
      return TRUE; /* Matches the current path */

    while (*lpszMask && *lpszMask != ';')
      lpszMask++;

    if (*lpszMask == ';')
    {
      lpszMask++;
      while (*lpszMask == ' ')
        lpszMask++; /* Masks may be separated by "; " */
    }
  }
  return FALSE;
}

/*************************************************************************
 * PathIsSameRootA	[SHLWAPI.@]
 *
 * Determine if two paths share the same root.
 *
 * PARAMS
 *  lpszPath1 [I] Source path
 *  lpszPath2 [I] Path to compare with
 *
 * RETURNS
 *  TRUE  If both paths are valid and share the same root.
 *  FALSE If either path is invalid or the paths do not share the same root.
 */
BOOL WINAPI PathIsSameRootA(LPCSTR lpszPath1, LPCSTR lpszPath2)
{
  LPCSTR lpszStart;
  int dwLen;

  TRACE("(%s,%s)\n", debugstr_a(lpszPath1), debugstr_a(lpszPath2));

  if (!lpszPath1 || !lpszPath2 || !(lpszStart = PathSkipRootA(lpszPath1)))
    return FALSE;

  dwLen = PathCommonPrefixA(lpszPath1, lpszPath2, NULL) + 1;
  if (lpszStart - lpszPath1 > dwLen)
    return FALSE; /* Paths not common up to length of the root */
  return TRUE;
}

/*************************************************************************
 * PathIsSameRootW	[SHLWAPI.@]
 *
 * See PathIsSameRootA.
 */
BOOL WINAPI PathIsSameRootW(LPCWSTR lpszPath1, LPCWSTR lpszPath2)
{
  LPCWSTR lpszStart;
  int dwLen;

  TRACE("(%s,%s)\n", debugstr_w(lpszPath1), debugstr_w(lpszPath2));

  if (!lpszPath1 || !lpszPath2 || !(lpszStart = PathSkipRootW(lpszPath1)))
    return FALSE;

  dwLen = PathCommonPrefixW(lpszPath1, lpszPath2, NULL) + 1;
  if (lpszStart - lpszPath1 > dwLen)
    return FALSE; /* Paths not common up to length of the root */
  return TRUE;
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
BOOL WINAPI PathIsContentTypeA(LPCSTR lpszPath, LPCSTR lpszContentType)
{
  LPCSTR szExt;
  DWORD dwDummy;
  char szBuff[MAX_PATH];

  TRACE("(%s,%s)\n", debugstr_a(lpszPath), debugstr_a(lpszContentType));

  if (lpszPath && (szExt = PathFindExtensionA(lpszPath)) && *szExt &&
      !SHGetValueA(HKEY_CLASSES_ROOT, szExt, "Content Type",
                   REG_NONE, szBuff, &dwDummy) &&
      !strcasecmp(lpszContentType, szBuff))
  {
    return TRUE;
  }
  return FALSE;
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
      !strcmpiW(lpszContentType, szBuff))
  {
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 * PathIsFileSpecA   [SHLWAPI.@]
 *
 * Determine if a path is a file specification.
 *
 * PARAMS
 *  lpszPath [I] Path to chack
 *
 * RETURNS
 *  TRUE  If lpszPath is a file specification (i.e. Contains no directories).
 *  FALSE Otherwise.
 */
BOOL WINAPI PathIsFileSpecA(LPCSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (!lpszPath)
    return FALSE;

  while (*lpszPath)
  {
    if (*lpszPath == '\\' || *lpszPath == ':')
      return FALSE;
    lpszPath = CharNextA(lpszPath);
  }
  return TRUE;
}

/*************************************************************************
 * PathIsFileSpecW   [SHLWAPI.@]
 *
 * See PathIsFileSpecA.
 */
BOOL WINAPI PathIsFileSpecW(LPCWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (!lpszPath)
    return FALSE;

  while (*lpszPath)
  {
    if (*lpszPath == '\\' || *lpszPath == ':')
      return FALSE;
    lpszPath = CharNextW(lpszPath);
  }
  return TRUE;
}

/*************************************************************************
 * PathIsPrefixA   [SHLWAPI.@]
 *
 * Determine if a path is a prefix of another.
 *
 * PARAMS
 *  lpszPrefix [I] Prefix
 *  lpszPath   [I] Path to check
 *
 * RETURNS
 *  TRUE  If lpszPath has lpszPrefix as its prefix,
 *  FALSE If either path is NULL or lpszPrefix is not a prefix
 */
BOOL WINAPI PathIsPrefixA (LPCSTR lpszPrefix, LPCSTR lpszPath)
{
  TRACE("(%s,%s)\n", debugstr_a(lpszPrefix), debugstr_a(lpszPath));

  if (lpszPrefix && lpszPath &&
      PathCommonPrefixA(lpszPath, lpszPrefix, NULL) == (int)strlen(lpszPrefix))
    return TRUE;
  return FALSE;
}

/*************************************************************************
 *  PathIsPrefixW   [SHLWAPI.@]
 *
 *  See PathIsPrefixA.
 */
BOOL WINAPI PathIsPrefixW(LPCWSTR lpszPrefix, LPCWSTR lpszPath)
{
  TRACE("(%s,%s)\n", debugstr_w(lpszPrefix), debugstr_w(lpszPath));

  if (lpszPrefix && lpszPath &&
      PathCommonPrefixW(lpszPath, lpszPrefix, NULL) == (int)strlenW(lpszPrefix))
    return TRUE;
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
 * PathIsUNCA		[SHLWAPI.@]
 *
 * Determine if a path is in UNC format.
 *
 * PARAMS
 *  lpszPath [I] Path to check
 *
 * RETURNS
 *  TRUE: The path is UNC.
 *  FALSE: The path is not UNC or is NULL.
 */
BOOL WINAPI PathIsUNCA(LPCSTR lpszPath)
{
  TRACE("(%s)\n",debugstr_a(lpszPath));

  if (lpszPath && (lpszPath[0]=='\\') && (lpszPath[1]=='\\'))
    return TRUE;
  return FALSE;
}

/*************************************************************************
 * PathIsUNCW		[SHLWAPI.@]
 *
 * See PathIsUNCA.
 */
BOOL WINAPI PathIsUNCW(LPCWSTR lpszPath)
{
  TRACE("(%s)\n",debugstr_w(lpszPath));

  if (lpszPath && (lpszPath[0]=='\\') && (lpszPath[1]=='\\'))
    return TRUE;
  return FALSE;
}

/*************************************************************************
 * PathIsUNCServerA   [SHLWAPI.@]
 *
 * Determine if a path is a UNC server name ("\\SHARENAME").
 *
 * PARAMS
 *  lpszPath  [I] Path to check.
 *
 * RETURNS
 *  TRUE   If lpszPath is a valid UNC server name.
 *  FALSE  Otherwise.
 *
 * NOTES
 *  This routine is bug compatible with Win32: Server names with a
 *  trailing backslash (e.g. "\\FOO\"), return FALSE incorrectly.
 *  Fixing this bug may break other shlwapi functions!
 */
BOOL WINAPI PathIsUNCServerA(LPCSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (lpszPath && *lpszPath++ == '\\' && *lpszPath++ == '\\')
  {
    while (*lpszPath)
    {
      if (*lpszPath == '\\')
        return FALSE;
      lpszPath = CharNextA(lpszPath);
    }
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 * PathIsUNCServerW   [SHLWAPI.@]
 *
 * See PathIsUNCServerA.
 */
BOOL WINAPI PathIsUNCServerW(LPCWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (lpszPath && *lpszPath++ == '\\' && *lpszPath++ == '\\')
  {
    while (*lpszPath)
    {
      if (*lpszPath == '\\')
        return FALSE;
      lpszPath = CharNextW(lpszPath);
    }
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 * PathIsUNCServerShareA   [SHLWAPI.@]
 *
 * Determine if a path is a UNC server share ("\\SHARENAME\SHARE").
 *
 * PARAMS
 *  lpszPath  [I] Path to check.
 *
 * RETURNS
 *  TRUE   If lpszPath is a valid UNC server share.
 *  FALSE  Otherwise.
 *
 * NOTES
 *  This routine is bug compatible with Win32: Server shares with a
 *  trailing backslash (e.g. "\\FOO\BAR\"), return FALSE incorrectly.
 *  Fixing this bug may break other shlwapi functions!
 */
BOOL WINAPI PathIsUNCServerShareA(LPCSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (lpszPath && *lpszPath++ == '\\' && *lpszPath++ == '\\')
  {
    BOOL bSeenSlash = FALSE;
    while (*lpszPath)
    {
      if (*lpszPath == '\\')
      {
        if (bSeenSlash)
          return FALSE;
        bSeenSlash = TRUE;
      }
      lpszPath = CharNextA(lpszPath);
    }
    return bSeenSlash;
  }
  return FALSE;
}

/*************************************************************************
 * PathIsUNCServerShareW   [SHLWAPI.@]
 *
 * See PathIsUNCServerShareA.
 */
BOOL WINAPI PathIsUNCServerShareW(LPCWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (lpszPath && *lpszPath++ == '\\' && *lpszPath++ == '\\')
  {
    BOOL bSeenSlash = FALSE;
    while (*lpszPath)
    {
      if (*lpszPath == '\\')
      {
        if (bSeenSlash)
          return FALSE;
        bSeenSlash = TRUE;
      }
      lpszPath = CharNextW(lpszPath);
    }
    return bSeenSlash;
  }
  return FALSE;
}

/*************************************************************************
 * PathCanonicalizeA   [SHLWAPI.@]
 *
 * Convert a path to its canonical form.
 *
 * PARAMS
 *  lpszBuf  [O] Output path
 *  lpszPath [I] Path to cnonicalize
 *
 * RETURNS
 *  Success: TRUE.  lpszBuf contains the output path,
 *  Failure: FALSE, If input path is invalid. lpszBuf is undefined
 */
BOOL WINAPI PathCanonicalizeA(LPSTR lpszBuf, LPCSTR lpszPath)
{
  BOOL bRet = FALSE;

  TRACE("(%p,%s)\n", lpszBuf, debugstr_a(lpszPath));

  if (lpszBuf)
    *lpszBuf = '\0';

  if (!lpszBuf || !lpszPath)
    SetLastError(ERROR_INVALID_PARAMETER);
  else
  {
    WCHAR szPath[MAX_PATH];
    WCHAR szBuff[MAX_PATH];
    MultiByteToWideChar(0,0,lpszPath,-1,szPath,MAX_PATH);
    bRet = PathCanonicalizeW(szBuff, szPath);
    WideCharToMultiByte(0,0,szBuff,-1,lpszBuf,MAX_PATH,0,0);
  }
  return bRet;
}


/*************************************************************************
 * PathCanonicalizeW   [SHLWAPI.@]
 *
 * See PathCanonicalizeA.
 */
BOOL WINAPI PathCanonicalizeW(LPWSTR lpszBuf, LPCWSTR lpszPath)
{
  LPWSTR lpszDst = lpszBuf;
  LPCWSTR lpszSrc = lpszPath;

  TRACE("(%p,%s)\n", lpszBuf, debugstr_w(lpszPath));

  if (lpszBuf)
    *lpszDst = '\0';

  if (!lpszBuf || !lpszPath)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  if (!*lpszPath)
  {
    *lpszBuf++ = '\\';
    *lpszBuf = '\0';
    return TRUE;
  }

  /* Copy path root */
  if (*lpszSrc == '\\')
  {
    *lpszDst++ = *lpszSrc++;
  }
  else if (*lpszSrc && lpszSrc[1] == ':')
  {
    /* X:\ */
    *lpszDst++ = *lpszSrc++;
    *lpszDst++ = *lpszSrc++;
    if (*lpszSrc == '\\')
      *lpszDst++ = *lpszSrc++;
  }

  /* Canonicalize the rest of the path */
  while (*lpszSrc)
  {
    if (*lpszSrc == '.')
    {
      if (lpszSrc[1] == '\\' && (lpszSrc == lpszPath || lpszSrc[-1] == '\\' || lpszSrc[-1] == ':'))
      {
        lpszSrc += 2; /* Skip .\ */
      }
      else if (lpszSrc[1] == '.' && (lpszDst == lpszBuf || lpszDst[-1] == '\\'))
      {
        /* \.. backs up a directory, over the root if it has no \ following X:.
         * .. is ignored if it would remove a UNC server name or inital \\
         */
        if (lpszDst != lpszBuf)
        {
          *lpszDst = '\0'; /* Allow PathIsUNCServerShareA test on lpszBuf */
          if (lpszDst > lpszBuf+1 && lpszDst[-1] == '\\' &&
             (lpszDst[-2] != '\\' || lpszDst > lpszBuf+2))
          {
            if (lpszDst[-2] == ':' && (lpszDst > lpszBuf+3 || lpszDst[-3] == ':'))
            {
              lpszDst -= 2;
              while (lpszDst > lpszBuf && *lpszDst != '\\')
                lpszDst--;
              if (*lpszDst == '\\')
                lpszDst++; /* Reset to last '\' */
              else
                lpszDst = lpszBuf; /* Start path again from new root */
            }
            else if (lpszDst[-2] != ':' && !PathIsUNCServerShareW(lpszBuf))
              lpszDst -= 2;
          }
          while (lpszDst > lpszBuf && *lpszDst != '\\')
            lpszDst--;
          if (lpszDst == lpszBuf)
          {
            *lpszDst++ = '\\';
            lpszSrc++;
          }
        }
        lpszSrc += 2; /* Skip .. in src path */
      }
      else
        *lpszDst++ = *lpszSrc++;
    }
    else
      *lpszDst++ = *lpszSrc++;
  }
  /* Append \ to naked drive specs */
  if (lpszDst - lpszBuf == 2 && lpszDst[-1] == ':')
    *lpszDst++ = '\\';
  *lpszDst++ = '\0';
  return TRUE;
}

/*************************************************************************
 * PathFindNextComponentA   [SHLWAPI.@]
 *
 * Find the next component in a path.
 *
 * PARAMS
 *   lpszPath [I] Path to find next component in
 *
 * RETURNS
 *  Success: A pointer to the next component, or the end of the string.
 *  Failure: NULL, If lpszPath is invalid
 *
 * NOTES
 *  A 'component' is either a backslash character (\) or UNC marker (\\).
 *  Because of this, relative paths (e.g "c:foo") are regarded as having
 *  only one component.
 */
LPSTR WINAPI PathFindNextComponentA(LPCSTR lpszPath)
{
  LPSTR lpszSlash;

  TRACE("(%s)\n", debugstr_a(lpszPath));

  if(!lpszPath || !*lpszPath)
    return NULL;

  if ((lpszSlash = StrChrA(lpszPath, '\\')))
  {
    if (lpszSlash[1] == '\\')
      lpszSlash++;
    return lpszSlash + 1;
  }
  return (LPSTR)lpszPath + strlen(lpszPath);
}

/*************************************************************************
 * PathFindNextComponentW   [SHLWAPI.@]
 *
 * See PathFindNextComponentA.
 */
LPWSTR WINAPI PathFindNextComponentW(LPCWSTR lpszPath)
{
  LPWSTR lpszSlash;

  TRACE("(%s)\n", debugstr_w(lpszPath));

  if(!lpszPath || !*lpszPath)
    return NULL;

  if ((lpszSlash = StrChrW(lpszPath, '\\')))
  {
    if (lpszSlash[1] == '\\')
      lpszSlash++;
    return lpszSlash + 1;
  }
  return (LPWSTR)lpszPath + strlenW(lpszPath);
}

/*************************************************************************
 * PathAddExtensionA   [SHLWAPI.@]
 *
 * Add a file extension to a path
 *
 * PARAMS
 *  lpszPath      [I/O] Path to add extension to
 *  lpszExtension [I]   Extension to add to lpszPath
 *
 * RETURNS
 *  TRUE  If the path was modified,
 *  FALSE If lpszPath or lpszExtension are invalid, lpszPath has an
 *        extension allready, or the new path length is too big.
 *
 * FIXME
 *  What version of shlwapi.dll adds "exe" if lpszExtension is NULL? Win2k
 *  does not do this, so the behaviour was removed.
 */
BOOL WINAPI PathAddExtensionA(LPSTR lpszPath, LPCSTR lpszExtension)
{
  size_t dwLen;

  TRACE("(%s,%s)\n", debugstr_a(lpszPath), debugstr_a(lpszExtension));

  if (!lpszPath || !lpszExtension || *(PathFindExtensionA(lpszPath)))
    return FALSE;

  dwLen = strlen(lpszPath);

  if (dwLen + strlen(lpszExtension) >= MAX_PATH)
    return FALSE;

  strcpy(lpszPath + dwLen, lpszExtension);
  return TRUE;
}

/*************************************************************************
 * PathAddExtensionW   [SHLWAPI.@]
 *
 * See PathAddExtensionA.
 */
BOOL WINAPI PathAddExtensionW(LPWSTR lpszPath, LPCWSTR lpszExtension)
{
  size_t dwLen;

  TRACE("(%s,%s)\n", debugstr_w(lpszPath), debugstr_w(lpszExtension));

  if (!lpszPath || !lpszExtension || *(PathFindExtensionW(lpszPath)))
    return FALSE;

  dwLen = strlenW(lpszPath);

  if (dwLen + strlenW(lpszExtension) >= MAX_PATH)
    return FALSE;

  strcpyW(lpszPath + dwLen, lpszExtension);
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

  if (!pszIter || !*pszIter)
    return FALSE;

  while (*pszIter)
  {
    if (islower(*pszIter) || IsDBCSLeadByte(*pszIter))
      return FALSE; /* Not DOS path */
    pszIter++;
  }
  pszIter = lpszPath + 1;
  while (*pszIter)
  {
    *pszIter = tolower(*pszIter);
    pszIter++;
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

  if (!pszIter || !*pszIter)
    return FALSE;

  while (*pszIter)
  {
    if (islowerW(*pszIter))
      return FALSE; /* Not DOS path */
    pszIter++;
  }
  pszIter = lpszPath + 1;
  while (*pszIter)
  {
    *pszIter = tolowerW(*pszIter);
    pszIter++;
  }
  return TRUE;
}

/*************************************************************************
 * PathCommonPrefixA   [SHLWAPI.@]
 *
 * Determine the length of the common prefix between two paths.
 *
 * PARAMS
 *  lpszFile1 [I] First path for comparison
 *  lpszFile2 [I] Second path for comparison
 *  achPath   [O] Destination for common prefix string
 *
 * RETURNS
 *  The length of the common prefix. This is 0 if there is no common
 *  prefix between the paths or if any parameters are invalid. If the prefix
 *  is non-zero and achPath is not NULL, achPath is filled with the common
 *  part of the prefix and NUL terminated.
 *
 * NOTES
 *  A common prefix of 2 is always returned as 3. It is thus possible for
 *  the length returned to be invalid (i.e. Longer than one or both of the
 *  strings given as parameters). This Win32 behaviour has been implemented
 *  here, and cannot be changed (fixed?) without breaking other SHLWAPI calls.
 *  To work around this when using this function, always check that the byte
 *  at [common_prefix_len-1] is not a NUL. If it is, deduct 1 from the prefix.
 */
int WINAPI PathCommonPrefixA(LPCSTR lpszFile1, LPCSTR lpszFile2, LPSTR achPath)
{
  size_t iLen = 0;
  LPCSTR lpszIter1 = lpszFile1;
  LPCSTR lpszIter2 = lpszFile2;

  TRACE("(%s,%s,%p)\n", debugstr_a(lpszFile1), debugstr_a(lpszFile2), achPath);

  if (achPath)
    *achPath = '\0';

  if (!lpszFile1 || !lpszFile2)
    return 0;

  /* Handle roots first */
  if (PathIsUNCA(lpszFile1))
  {
    if (!PathIsUNCA(lpszFile2))
      return 0;
    lpszIter1 += 2;
    lpszIter2 += 2;
  }
  else if (PathIsUNCA(lpszFile2))
      return 0; /* Know already lpszFile1 is not UNC */

  do
  {
    /* Update len */
    if ((!*lpszIter1 || *lpszIter1 == '\\') &&
        (!*lpszIter2 || *lpszIter2 == '\\'))
      iLen = lpszIter1 - lpszFile1; /* Common to this point */

    if (!*lpszIter1 || (tolower(*lpszIter1) != tolower(*lpszIter2)))
      break; /* Strings differ at this point */

    lpszIter1++;
    lpszIter2++;
  } while (1);

  if (iLen == 2)
    iLen++; /* Feature/Bug compatible with Win32 */

  if (iLen && achPath)
  {
    memcpy(achPath,lpszFile1,iLen);
    achPath[iLen] = '\0';
  }
  return iLen;
}

/*************************************************************************
 * PathCommonPrefixW   [SHLWAPI.@]
 *
 * See PathCommonPrefixA.
 */
int WINAPI PathCommonPrefixW(LPCWSTR lpszFile1, LPCWSTR lpszFile2, LPWSTR achPath)
{
  size_t iLen = 0;
  LPCWSTR lpszIter1 = lpszFile1;
  LPCWSTR lpszIter2 = lpszFile2;

  TRACE("(%s,%s,%p)\n", debugstr_w(lpszFile1), debugstr_w(lpszFile2), achPath);

  if (achPath)
    *achPath = '\0';

  if (!lpszFile1 || !lpszFile2)
    return 0;

  /* Handle roots first */
  if (PathIsUNCW(lpszFile1))
  {
    if (!PathIsUNCW(lpszFile2))
      return 0;
    lpszIter1 += 2;
    lpszIter2 += 2;
  }
  else if (PathIsUNCW(lpszFile2))
      return 0; /* Know already lpszFile1 is not UNC */

  do
  {
    /* Update len */
    if ((!*lpszIter1 || *lpszIter1 == '\\') &&
        (!*lpszIter2 || *lpszIter2 == '\\'))
      iLen = lpszIter1 - lpszFile1; /* Common to this point */

    if (!*lpszIter1 || (tolowerW(*lpszIter1) != tolowerW(*lpszIter2)))
      break; /* Strings differ at this point */

    lpszIter1++;
    lpszIter2++;
  } while (1);

  if (iLen == 2)
    iLen++; /* Feature/Bug compatible with Win32 */

  if (iLen && achPath)
  {
    memcpy(achPath,lpszFile1,iLen * sizeof(WCHAR));
    achPath[iLen] = '\0';
  }
  return iLen;
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
 *  TRUE  If the path was modified.
 *  FALSE Otherwise.
 */
BOOL WINAPI PathCompactPathA(HDC hDC, LPSTR lpszPath, UINT dx)
{
  BOOL bRet = FALSE;

  TRACE("(%p,%s,%d)\n", hDC, debugstr_a(lpszPath), dx);

  if (lpszPath)
  {
    WCHAR szPath[MAX_PATH];
    MultiByteToWideChar(0,0,lpszPath,-1,szPath,MAX_PATH);
    bRet = PathCompactPathW(hDC, szPath, dx);
    WideCharToMultiByte(0,0,szPath,-1,lpszPath,MAX_PATH,0,0);
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
    return bRet;

  if (!hDC)
    hdc = hDC = GetDC(0);

  /* Get the length of the whole path */
  dwLen = strlenW(lpszPath);
  GetTextExtentPointW(hDC, lpszPath, dwLen, &size);

  if ((UINT)size.cx > dx)
  {
    /* Path too big, must reduce it */
    LPWSTR sFile;
    DWORD dwEllipsesLen = 0, dwPathLen = 0;

    sFile = PathFindFileNameW(lpszPath);
    if (sFile != lpszPath)
      sFile = CharPrevW(lpszPath, sFile);

    /* Get the size of ellipses */
    GetTextExtentPointW(hDC, szEllipses, 3, &size);
    dwEllipsesLen = size.cx;
    /* Get the size of the file name */
    GetTextExtentPointW(hDC, sFile, strlenW(sFile), &size);
    dwPathLen = size.cx;

    if (sFile != lpszPath)
    {
      LPWSTR sPath = sFile;
      BOOL bEllipses = FALSE;

      /* The path includes a file name. Include as much of the path prior to
       * the file name as possible, allowing for the ellipses, e.g:
       * c:\some very long path\filename ==> c:\some v...\filename
       */
      strncpyW(buff, sFile, MAX_PATH);

      do
      {
        DWORD dwTotalLen = bEllipses? dwPathLen + dwEllipsesLen : dwPathLen;

        GetTextExtentPointW(hDC, lpszPath, sPath - lpszPath, &size);
        dwTotalLen += size.cx;
        if (dwTotalLen <= dx)
          break;
        sPath = CharPrevW(lpszPath, sPath);
        if (!bEllipses)
        {
          bEllipses = TRUE;
          sPath = CharPrevW(lpszPath, sPath);
          sPath = CharPrevW(lpszPath, sPath);
        }
      } while (sPath > lpszPath);

      if (sPath > lpszPath)
      {
        if (bEllipses)
        {
          strcpyW(sPath, szEllipses);
          strcpyW(sPath+3, buff);
        }
        bRet = TRUE;
        goto end;
      }
      strcpyW(lpszPath, szEllipses);
      strcpyW(lpszPath+3, buff);
      bRet = FALSE;
      goto end;
    }

    /* Trim the path by adding ellipses to the end, e.g:
     * A very long file name.txt ==> A very...
     */
    dwLen = strlenW(lpszPath);

    if (dwLen > MAX_PATH - 3)
      dwLen =  MAX_PATH - 3;
    strncpyW(buff, sFile, dwLen);

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
     strcpyW(buff + dwLen, szEllipses);
     strcpyW(lpszPath, buff);
    }
  }

end:
  if (hdc)
    ReleaseDC(0, hdc);

  return bRet;
}

/*************************************************************************
 * PathGetCharTypeA   [SHLWAPI.@]
 *
 * Categorise a character from a file path.
 *
 * PARAMS
 *  ch [I] Character to get the type of
 *
 * RETURNS
 *  A set of GCT_ bit flags (from "shlwapi.h") indicating the character type.
 */
UINT WINAPI PathGetCharTypeA(UCHAR ch)
{
  return PathGetCharTypeW(ch);
}

/*************************************************************************
 * PathGetCharTypeW   [SHLWAPI.@]
 *
 * See PathGetCharTypeA.
 */
UINT WINAPI PathGetCharTypeW(WCHAR ch)
{
  UINT flags = 0;

  TRACE("(%d)\n", ch);

  if (!ch || ch < ' ' || ch == '<' || ch == '>' ||
      ch == '"' || ch == '|' || ch == '/')
    flags = GCT_INVALID; /* Invalid */
  else if (ch == '*' || ch=='?')
    flags = GCT_WILD; /* Wildchars */
  else if ((ch == '\\') || (ch == ':'))
    return GCT_SEPARATOR; /* Path separators */
  else
  {
     if (ch < 126)
     {
       if ((ch & 0x1 && ch != ';') || !ch || isalnum(ch) || ch == '$' || ch == '&' || ch == '(' ||
            ch == '.' || ch == '@' || ch == '^' ||
            ch == '\'' || ch == 130 || ch == '`')
         flags |= GCT_SHORTCHAR; /* All these are valid for DOS */
     }
     else
       flags |= GCT_SHORTCHAR; /* Bug compatible with win32 */
     flags |= GCT_LFNCHAR; /* Valid for long file names */
  }
  return flags;
}

/*************************************************************************
 * SHLWAPI_UseSystemForSystemFolders
 *
 * Internal helper for PathMakeSystemFolderW.
 */
static BOOL WINAPI SHLWAPI_UseSystemForSystemFolders()
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
BOOL WINAPI PathMakeSystemFolderA(LPSTR lpszPath)
{
  BOOL bRet = FALSE;

  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (lpszPath && *lpszPath)
  {
    WCHAR szPath[MAX_PATH];
    MultiByteToWideChar(0,0,lpszPath,-1,szPath,MAX_PATH);
    bRet = PathMakeSystemFolderW(szPath);
  }
  return bRet;
}

/*************************************************************************
 * PathMakeSystemFolderW   [SHLWAPI.@]
 *
 * See PathMakeSystemFolderA.
 */
BOOL WINAPI PathMakeSystemFolderW(LPWSTR lpszPath)
{
  DWORD dwDefaultAttr = FILE_ATTRIBUTE_READONLY, dwAttr;
  WCHAR buff[MAX_PATH];

  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (!lpszPath || !*lpszPath)
    return FALSE;

  /* If the directory is already a system directory, don't do anything */
  GetSystemDirectoryW(buff, MAX_PATH);
  if (!strcmpW(buff, lpszPath))
    return TRUE;

  GetWindowsDirectoryW(buff, MAX_PATH);
  if (!strcmpW(buff, lpszPath))
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
 * PathRenameExtensionA   [SHLWAPI.@]
 *
 * Swap the file extension in a path with another extension.
 *
 * PARAMS
 *  lpszPath [I/O] Path to swap the extension in
 *  lpszExt  [I]   The new extension
 *
 * RETURNS
 *  TRUE  if lpszPath was modified,
 *  FALSE if lpszPath or lpszExt is NULL, or the new path is too long
 */
BOOL WINAPI PathRenameExtensionA(LPSTR lpszPath, LPCSTR lpszExt)
{
  LPSTR lpszExtension;

  TRACE("(%s,%s)\n", debugstr_a(lpszPath), debugstr_a(lpszExt));

  lpszExtension = PathFindExtensionA(lpszPath);

  if (!lpszExtension || (lpszExtension - lpszPath + strlen(lpszExt) >= MAX_PATH))
    return FALSE;

  strcpy(lpszExtension, lpszExt);
  return TRUE;
}

/*************************************************************************
 * PathRenameExtensionW   [SHLWAPI.@]
 *
 * See PathRenameExtensionA.
 */
BOOL WINAPI PathRenameExtensionW(LPWSTR lpszPath, LPCWSTR lpszExt)
{
  LPWSTR lpszExtension;

  TRACE("(%s,%s)\n", debugstr_w(lpszPath), debugstr_w(lpszExt));

  lpszExtension = PathFindExtensionW(lpszPath);

  if (!lpszExtension || (lpszExtension - lpszPath + strlenW(lpszExt) >= MAX_PATH))
    return FALSE;

  strcpyW(lpszExtension, lpszExt);
  return TRUE;
}

/*************************************************************************
 * PathSearchAndQualifyA   [SHLWAPI.@]
 *
 * Determine if a given path is correct and fully qualified.
 *
 * PARAMS
 *  lpszPath [I] Path to check
 *  lpszBuf  [O] Output for correct path
 *  cchBuf   [I] Size of lpszBuf
 *
 * RETURNS
 *  Unknown.
 */
BOOL WINAPI PathSearchAndQualifyA(LPCSTR lpszPath, LPSTR lpszBuf, UINT cchBuf)
{
  FIXME("(%s,%p,0x%08x)-stub\n", debugstr_a(lpszPath), lpszBuf, cchBuf);
  return FALSE;
}

/*************************************************************************
 * PathSearchAndQualifyW   [SHLWAPI.@]
 *
 * See PathSearchAndQualifyA.
 */
BOOL WINAPI PathSearchAndQualifyW(LPCWSTR lpszPath, LPWSTR lpszBuf, UINT cchBuf)
{
  FIXME("(%s,%p,0x%08x)-stub\n", debugstr_w(lpszPath), lpszBuf, cchBuf);
  return FALSE;
}

/*************************************************************************
 * PathSkipRootA   [SHLWAPI.@]
 *
 * Return the portion of a path following the drive letter or mount point.
 *
 * PARAMS
 *  lpszPath [I] The path to skip on
 *
 * RETURNS
 *  Success: A pointer to the next character after the root.
 *  Failure: NULL, if lpszPath is invalid, has no root or is a multibyte string.
 */
LPSTR WINAPI PathSkipRootA(LPCSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (!lpszPath || !*lpszPath)
    return NULL;

  if (*lpszPath == '\\' && lpszPath[1] == '\\')
  {
    /* Network share: skip share server and mount point */
    lpszPath += 2;
    if ((lpszPath = StrChrA(lpszPath, '\\')) &&
        (lpszPath = StrChrA(lpszPath + 1, '\\')))
      lpszPath++;
    return (LPSTR)lpszPath;
  }

  if (IsDBCSLeadByte(*lpszPath))
    return NULL;

  /* Check x:\ */
  if (lpszPath[0] && lpszPath[1] == ':' && lpszPath[2] == '\\')
    return (LPSTR)lpszPath + 3;
  return NULL;
}

/*************************************************************************
 * PathSkipRootW   [SHLWAPI.@]
 *
 * See PathSkipRootA.
 */
LPWSTR WINAPI PathSkipRootW(LPCWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (!lpszPath || !*lpszPath)
    return NULL;

  if (*lpszPath == '\\' && lpszPath[1] == '\\')
  {
    /* Network share: skip share server and mount point */
    lpszPath += 2;
    if ((lpszPath = StrChrW(lpszPath, '\\')) &&
        (lpszPath = StrChrW(lpszPath + 1, '\\')))
     lpszPath++;
    return (LPWSTR)lpszPath;
  }

  /* Check x:\ */
  if (lpszPath[0] && lpszPath[1] == ':' && lpszPath[2] == '\\')
    return (LPWSTR)lpszPath + 3;
  return NULL;
}

/*************************************************************************
 * PathCreateFromUrlA   [SHLWAPI.@]
 *
 * Create a path from a URL
 *
 * PARAMS
 *  lpszUrl  [I] URL to convert into a path
 *  lpszPath [O] Output buffer for the resulting Path
 *  pcchPath [I] Length of lpszPath
 *  dwFlags  [I] Flags controlling the conversion
 *
 * RETURNS
 *  Success: S_OK. lpszPath contains the URL in path format,
 *  Failure: An HRESULT error code such as E_INVALIDARG.
 */
HRESULT WINAPI PathCreateFromUrlA(LPCSTR lpszUrl, LPSTR lpszPath,
                                  LPDWORD pcchPath, DWORD dwFlags)
{
  LPSTR pszPathPart;
  TRACE("(%s,%p,%p,0x%08lx)\n", debugstr_a(lpszUrl), lpszPath, pcchPath, dwFlags);

  if (!lpszUrl || !lpszPath || !pcchPath || !*pcchPath)
    return E_INVALIDARG;

  pszPathPart = StrChrA(lpszUrl, ':');
  if ((((pszPathPart - lpszUrl) == 1) && isalpha(*lpszUrl)) ||
         !lstrcmpA(lpszUrl, "file:"))
  {
    return UrlUnescapeA(pszPathPart, lpszPath, pcchPath, dwFlags);
  }
    /* extracts thing prior to : in pszURL and checks against:
     *   https
     *   shell
     *   local
     *   about  - if match returns E_INVALIDARG
     */

  return E_INVALIDARG;
}

/*************************************************************************
 * PathCreateFromUrlW   [SHLWAPI.@]
 *
 * See PathCreateFromUrlA.
 */
HRESULT WINAPI PathCreateFromUrlW(LPCWSTR lpszUrl, LPWSTR lpszPath,
                                  LPDWORD pcchPath, DWORD dwFlags)
{
  static const WCHAR stemp[] = { 'f','i','l','e',':','/','/',0 };
  LPWSTR pwszPathPart;
  HRESULT hr;

  TRACE("(%s,%p,%p,0x%08lx)\n", debugstr_w(lpszUrl), lpszPath, pcchPath, dwFlags);

  if (!lpszUrl || !lpszPath || !pcchPath || !*pcchPath)
    return E_INVALIDARG;

  /* Path of the form file://... */
  if (!strncmpW(lpszUrl, stemp, 7))
  {
    lpszUrl += 7;
  }
  /* Path of the form file:... */
  else if (!strncmpW(lpszUrl, stemp, 5))
  {
    lpszUrl += 5;
  }

  /* Ensure that path is of the form c:... or c|... */
  if (lpszUrl[1] != ':' && lpszUrl[1] != '|' && isalphaW(*lpszUrl))
    return E_INVALIDARG;

  hr = UrlUnescapeW((LPWSTR) lpszUrl, lpszPath, pcchPath, dwFlags);
  if (lpszPath[1] == '|')
    lpszPath[1] = ':';

  for (pwszPathPart = lpszPath; *pwszPathPart; pwszPathPart++)
    if (*pwszPathPart == '/')
      *pwszPathPart = '\\';

  TRACE("Returning %s\n",debugstr_w(lpszPath));

  return hr;
}

/*************************************************************************
 * PathRelativePathToA   [SHLWAPI.@]
 *
 * Create a relative path from one path to another.
 *
 * PARAMS
 *  lpszPath   [O] Destination for relative path
 *  lpszFrom   [I] Source path
 *  dwAttrFrom [I] File attribute of source path
 *  lpszTo     [I] Destination path
 *  dwAttrTo   [I] File attributes of destination path
 *
 * RETURNS
 *  TRUE  If a relative path can be formed. lpszPath contains the new path
 *  FALSE If the paths are not relavtive or any parameters are invalid
 *
 * NOTES
 *  lpszTo should be at least MAX_PATH in length.
 *
 *  Calling this function with relative paths for lpszFrom or lpszTo may
 *  give erroneous results.
 *
 *  The Win32 version of this function contains a bug where the lpszTo string
 *  may be referenced 1 byte beyond the end of the string. As a result random
 *  garbage may be written to the output path, depending on what lies beyond
 *  the last byte of the string. This bug occurs because of the behaviour of
 *  PathCommonPrefix() (see notes for that function), and no workaround seems
 *  possible with Win32.
 *
 *  This bug has been fixed here, so for example the relative path from "\\"
 *  to "\\" is correctly determined as "." in this implementation.
 */
BOOL WINAPI PathRelativePathToA(LPSTR lpszPath, LPCSTR lpszFrom, DWORD dwAttrFrom,
                                LPCSTR lpszTo, DWORD dwAttrTo)
{
  BOOL bRet = FALSE;

  TRACE("(%p,%s,0x%08lx,%s,0x%08lx)\n", lpszPath, debugstr_a(lpszFrom),
        dwAttrFrom, debugstr_a(lpszTo), dwAttrTo);

  if(lpszPath && lpszFrom && lpszTo)
  {
    WCHAR szPath[MAX_PATH];
    WCHAR szFrom[MAX_PATH];
    WCHAR szTo[MAX_PATH];
    MultiByteToWideChar(0,0,lpszFrom,-1,szFrom,MAX_PATH);
    MultiByteToWideChar(0,0,lpszTo,-1,szTo,MAX_PATH);
    bRet = PathRelativePathToW(szPath,szFrom,dwAttrFrom,szTo,dwAttrTo);
    WideCharToMultiByte(0,0,szPath,-1,lpszPath,MAX_PATH,0,0);
  }
  return bRet;
}

/*************************************************************************
 * PathRelativePathToW   [SHLWAPI.@]
 *
 * See PathRelativePathToA.
 */
BOOL WINAPI PathRelativePathToW(LPWSTR lpszPath, LPCWSTR lpszFrom, DWORD dwAttrFrom,
                                LPCWSTR lpszTo, DWORD dwAttrTo)
{
  static const WCHAR szPrevDirSlash[] = { '.', '.', '\\', '\0' };
  static const WCHAR szPrevDir[] = { '.', '.', '\0' };
  WCHAR szFrom[MAX_PATH];
  WCHAR szTo[MAX_PATH];
  DWORD dwLen;

  TRACE("(%p,%s,0x%08lx,%s,0x%08lx)\n", lpszPath, debugstr_w(lpszFrom),
        dwAttrFrom, debugstr_w(lpszTo), dwAttrTo);

  if(!lpszPath || !lpszFrom || !lpszTo)
    return FALSE;

  *lpszPath = '\0';
  strncpyW(szFrom, lpszFrom, MAX_PATH);
  strncpyW(szTo, lpszTo, MAX_PATH);

  if(!(dwAttrFrom & FILE_ATTRIBUTE_DIRECTORY))
    PathRemoveFileSpecW(szFrom);
  if(!(dwAttrFrom & FILE_ATTRIBUTE_DIRECTORY))
    PathRemoveFileSpecW(szTo);

  /* Paths can only be relative if they have a common root */
  if(!(dwLen = PathCommonPrefixW(szFrom, szTo, 0)))
    return FALSE;

  /* Strip off lpszFrom components to the root, by adding "..\" */
  lpszFrom = szFrom + dwLen;
  if (!*lpszFrom)
  {
    lpszPath[0] = '.';
    lpszPath[1] = '\0';
  }
  if (*lpszFrom == '\\')
    lpszFrom++;

  while (*lpszFrom)
  {
    lpszFrom = PathFindNextComponentW(lpszFrom);
    strcatW(lpszPath, *lpszFrom ? szPrevDirSlash : szPrevDir);
  }

  /* From the root add the components of lpszTo */
  lpszTo += dwLen;
  /* We check lpszTo[-1] to avoid skipping end of string. See the notes for
   * this function.
   */
  if (*lpszTo && lpszTo[-1])
  {
    if (*lpszTo != '\\')
      lpszTo--;
    dwLen = strlenW(lpszPath);
    if (dwLen + strlenW(lpszTo) >= MAX_PATH)
    {
      *lpszPath = '\0';
      return FALSE;
    }
    strcpyW(lpszPath + dwLen, lpszTo);
  }
  return TRUE;
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
BOOL WINAPI PathUnmakeSystemFolderA(LPSTR lpszPath)
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
BOOL WINAPI PathUnmakeSystemFolderW(LPWSTR lpszPath)
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
    MultiByteToWideChar(0,0,lpszPath,-1,szPath,MAX_PATH);
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
    strncpyW(path, lpszPath, sizeof(path));
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
  GET_FUNC(pIsNetDrive, shell32, (LPCSTR)66, FALSE); /* ord 66 = shell32.IsNetDrive */
  return pIsNetDrive(dwDriveNum);
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
  GET_FUNC(pIsNetDrive, shell32, (LPCSTR)66, FALSE); /* ord 66 = shell32.IsNetDrive */
  return pIsNetDrive(dwDriveNum);
}

/*************************************************************************
 * PathIsLFNFileSpecA [SHLWAPI.@]
 *
 * Determine if the given path is a long file name
 *
 * PARAMS
 *  lpszPath [I] Path to check
 *
 * RETURNS
 *  TRUE  If path is a long file name,
 *  FALSE If path is a valid DOS short file name
 */
BOOL WINAPI PathIsLFNFileSpecA(LPCSTR lpszPath)
{
  DWORD dwNameLen = 0, dwExtLen = 0;

  TRACE("(%s)\n",debugstr_a(lpszPath));

  if (!lpszPath)
    return FALSE;

  while (*lpszPath)
  {
    if (*lpszPath == ' ')
      return TRUE; /* DOS names cannot have spaces */
    if (*lpszPath == '.')
    {
      if (dwExtLen)
        return TRUE; /* DOS names have only one dot */
      dwExtLen = 1;
    }
    else if (dwExtLen)
    {
      dwExtLen++;
      if (dwExtLen > 4)
        return TRUE; /* DOS extensions are <= 3 chars*/
    }
    else
    {
      dwNameLen++;
      if (dwNameLen > 8)
        return TRUE; /* DOS names are <= 8 chars */
    }
    lpszPath += IsDBCSLeadByte(*lpszPath) ? 2 : 1;
  }
  return FALSE; /* Valid DOS path */
}

/*************************************************************************
 * PathIsLFNFileSpecW [SHLWAPI.@]
 *
 * See PathIsLFNFileSpecA.
 */
BOOL WINAPI PathIsLFNFileSpecW(LPCWSTR lpszPath)
{
  DWORD dwNameLen = 0, dwExtLen = 0;

  TRACE("(%s)\n",debugstr_w(lpszPath));

  if (!lpszPath)
    return FALSE;

  while (*lpszPath)
  {
    if (*lpszPath == ' ')
      return TRUE; /* DOS names cannot have spaces */
    if (*lpszPath == '.')
    {
      if (dwExtLen)
        return TRUE; /* DOS names have only one dot */
      dwExtLen = 1;
    }
    else if (dwExtLen)
    {
      dwExtLen++;
      if (dwExtLen > 4)
        return TRUE; /* DOS extensions are <= 3 chars*/
    }
    else
    {
      dwNameLen++;
      if (dwNameLen > 8)
        return TRUE; /* DOS names are <= 8 chars */
    }
    lpszPath++;
  }
  return FALSE; /* Valid DOS path */
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
    MultiByteToWideChar(0,0,lpszPath,-1,szPath,MAX_PATH);
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
  BOOL retVal = FALSE;
  WIN32_FIND_DATAW find_data;

  TRACE("(%s)\n",debugstr_w(lpszPath));

  if (!lpszPath || !PathIsDirectoryW(lpszPath))
      return FALSE;

  strncpyW(szSearch, lpszPath, MAX_PATH);
  PathAddBackslashW(szSearch);
  dwLen = strlenW(szSearch);
  if (dwLen > MAX_PATH - 4)
    return FALSE;

  strcpyW(szSearch + dwLen, szAllFiles);
  hfind = FindFirstFileW(szSearch, &find_data);

  if (hfind != INVALID_HANDLE_VALUE &&
      find_data.cFileName[0] == '.' &&
      find_data.cFileName[1] == '.')
  {
    /* The only directory entry should be the parent */
    if (!FindNextFileW(hfind, &find_data))
      retVal = TRUE;
    FindClose(hfind);
  }
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
    dwLen = strlenW(lpszSuffix);

    while (dwRet < dwCount)
    {
      size_t dwCompareLen = strlenW(*lppszArray);
      if (dwCompareLen < dwLen)
      {
        if (!strcmpW(lpszSuffix + dwLen - dwCompareLen, *lppszArray))
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
VOID WINAPI PathUndecorateA(LPSTR lpszPath)
{
  TRACE("(%s)\n",debugstr_a(lpszPath));

  if (lpszPath)
  {
    LPSTR lpszExt = PathFindExtensionA(lpszPath);
    if (lpszExt > lpszPath && lpszExt[-1] == ']')
    {
      LPSTR lpszSkip = lpszExt - 2;
      if (*lpszSkip == '[')
        lpszSkip++;  /* [] (no number) */
      else
        while (lpszSkip > lpszPath && isdigit(lpszSkip[-1]))
          lpszSkip--;
      if (lpszSkip > lpszPath && lpszSkip[-1] == '[' && lpszSkip[-2] != '\\')
      {
        /* remove the [n] */
        lpszSkip--;
        while (*lpszExt)
          *lpszSkip++ = *lpszExt++;
        *lpszSkip = '\0';
      }
    }
  }
}

/*************************************************************************
 * PathUndecorateW [SHLWAPI.@]
 *
 * See PathUndecorateA.
 */
VOID WINAPI PathUndecorateW(LPWSTR lpszPath)
{
  TRACE("(%s)\n",debugstr_w(lpszPath));

  if (lpszPath)
  {
    LPWSTR lpszExt = PathFindExtensionW(lpszPath);
    if (lpszExt > lpszPath && lpszExt[-1] == ']')
    {
      LPWSTR lpszSkip = lpszExt - 2;
      if (*lpszSkip == '[')
        lpszSkip++; /* [] (no number) */
      else
        while (lpszSkip > lpszPath && isdigitW(lpszSkip[-1]))
          lpszSkip--;
      if (lpszSkip > lpszPath && lpszSkip[-1] == '[' && lpszSkip[-2] != '\\')
      {
        /* remove the [n] */
        lpszSkip--;
        while (*lpszExt)
          *lpszSkip++ = *lpszExt++;
        *lpszSkip = '\0';
      }
    }
  }
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

  MultiByteToWideChar(0, 0, lpszFile, -1, szFile, MAX_PATH);
  szPath[0] = '\0';
  hRet = SHGetWebFolderFilePathW(szFile, szPath, dwPathLen);
  WideCharToMultiByte(0, 0, szPath, -1, lpszPath, dwPathLen, 0, 0);
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
#define szWebLen (sizeof(szWeb)/sizeof(WCHAR))
#define szWebMuiLen ((sizeof(szWebMui)+1)/sizeof(WCHAR))
  DWORD dwLen, dwFileLen;
  LANGID lidSystem, lidUser;

  TRACE("(%s,%p,%ld)\n", debugstr_w(lpszFile), lpszPath, dwPathLen);

  /* Get base directory for web content */
  dwLen = GetSystemWindowsDirectoryW(lpszPath, dwPathLen);
  if (dwLen > 0 && lpszPath[dwLen-1] == '\\')
    dwLen--;

  dwFileLen = strlenW(lpszFile);

  if (dwLen + dwFileLen + szWebLen >= dwPathLen)
    return E_FAIL; /* lpszPath too short */

  strcpyW(lpszPath+dwLen, szWeb);
  dwLen += szWebLen;
  dwPathLen = dwPathLen - dwLen; /* Remaining space */

  lidSystem = GetSystemDefaultUILanguage();
  lidUser = GetUserDefaultUILanguage();

  if (lidSystem != lidUser)
  {
    if (dwFileLen + szWebMuiLen < dwPathLen)
    {
      /* Use localised content in the users UI language if present */
      wsprintfW(lpszPath + dwLen, szWebMui, lidUser);
      strcpyW(lpszPath + dwLen + szWebMuiLen, lpszFile);
      if (PathFileExistsW(lpszPath))
        return S_OK;
    }
  }

  /* Fall back to OS default installed content */
  strcpyW(lpszPath + dwLen, lpszFile);
  if (PathFileExistsW(lpszPath))
    return S_OK;
  return E_FAIL;
}
