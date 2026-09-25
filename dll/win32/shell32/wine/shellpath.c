/*
 * Path Functions
 *
 * Copyright 1998, 1999, 2000 Juergen Schmied
 * Copyright 2004 Juan Lang
 * Copyright 2018-2022 Katayama Hirofumi MZ
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
 *
 * NOTES:
 *
 * Many of these functions are in SHLWAPI.DLL also
 *
 */

#ifdef __REACTOS__
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COBJMACROS

#include <wine/config.h>

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <undocshell.h>
#include <shlwapi.h>
#include <sddl.h>
#include <strsafe.h>
#include <wine/debug.h>
#include <wine/unicode.h>
#include <assert.h>

#include <shlwapi_undoc.h>
#include <shellutils.h>

#include <userenv.h>

#include "pidl.h"
#include "shell32_main.h"
#include "shresdef.h"

#else
#define COBJMACROS

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "winioctl.h"
#define WINE_MOUNTMGR_EXTENSIONS
#include "ddk/mountmgr.h"

#include "shlobj.h"
#include "shtypes.h"
#include "shresdef.h"
#include "shell32_main.h"
#include "pidl.h"
#include "shlwapi.h"
#include "sddl.h"
#include "knownfolders.h"
#include "initguid.h"
#include "shobjidl.h"
#include "wine/debug.h"
#endif

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static const BOOL is_win64 = sizeof(void *) > sizeof(int);

#ifdef __REACTOS__
BOOL APIENTRY IsRemovableDrive(DWORD iDrive)
{
    WCHAR szRoot[] = L"C:\\";
    assert(L'A' + iDrive <= L'Z');
    szRoot[0] = (WCHAR)(L'A' + iDrive);
    return GetDriveTypeW(szRoot) == DRIVE_REMOVABLE;
}

/*
	########## Combining and Constructing paths ##########
*/

static BOOL WINAPI
PathSearchOnExtensionsW(
    _Inout_ LPWSTR pszPath,
    _In_opt_ LPCWSTR *ppszDirs,
    _In_ BOOL bDoSearch,
    _In_ DWORD dwWhich)
{
    if (*PathFindExtensionW(pszPath) != 0)
        return FALSE;

    if (bDoSearch)
        return PathFindOnPathExW(pszPath, ppszDirs, dwWhich);
    else
        return PathFileExistsDefExtW(pszPath, dwWhich);
}

#if (_WIN32_WINNT >= _WIN32_WINNT_WS03)
static BOOL WINAPI PathIsAbsoluteW(_In_ LPCWSTR path)
{
    return PathIsUNCW(path) || (PathGetDriveNumberW(path) != -1 && path[2] == L'\\');
}

static BOOL WINAPI PathMakeAbsoluteW(_Inout_ LPWSTR path)
{
    WCHAR path1[MAX_PATH];
    DWORD cch;

    if (path == NULL)
        return FALSE;
    cch = GetCurrentDirectoryW(_countof(path1), path1);
    if (!cch || cch > _countof(path1))
        return FALSE;
    return (PathCombineW(path, path1, path) != NULL);
}
#endif

BOOL WINAPI IsLFNDriveW(LPCWSTR lpszPath);

static VOID WINAPI
PathQualifyExW(_Inout_ LPWSTR pszPath, _Inout_opt_ LPCWSTR pszDir, _In_ DWORD dwFlags)
{
    INT iDrive, cchPathLeft;
    WCHAR szTemp[MAX_PATH], szRoot[MAX_PATH];
    PWCHAR pchTemp = szTemp, pchPath;
    BOOL bLFN;

    TRACE("(%s,%s,0x%08x)\n", debugstr_w(pszPath), debugstr_w(pszDir), dwFlags);

    /* Save pszPath path into szTemp for rebuilding the path later */
    if (FAILED(StringCchCopyW(szTemp, _countof(szTemp), pszPath)))
        return;

    /* Replace every '/' by '\' */
    FixSlashesAndColonW(szTemp);

    cchPathLeft = MAX_PATH;

    /* Build the root-like path on pszPath, and set pchTemp */
    if (!PathIsUNCW(szTemp))
    {
        /*
         * Non-UNC path.
         * Determine and normalize the root drive.
         */
        iDrive = PathGetDriveNumberW(szTemp);
        if (iDrive == -1)
        {
            /*
             * No drive was specified in the path. Try to find one from the
             * optional directory (if this fails, fall back to the one of the
             * Windows directory).
             */
            if (!pszDir || FAILED(StringCchCopyW(szRoot, _countof(szRoot), pszDir)))
            {
                /* pszDir was invalid or NULL. Fall back to the
                 * Windows directory and find its root. */
                szRoot[0] = UNICODE_NULL;
                GetWindowsDirectoryW(szRoot, _countof(szRoot));
                iDrive = PathGetDriveNumberW(szRoot);
                if (iDrive != -1)
                    PathBuildRootW(szRoot, iDrive);
            }

            if (szTemp[0] == L'\\')
            {
                PathStripToRootW(szRoot);
            }
        }
        else
        {
            /*
             * A drive is specified in the path, that can be either of the
             * form 'C:\xxx' or of the form 'C:xxx' (relative path). Isolate
             * the root part 'C:' and the rest of the path 'xxx' in pchTemp.
             */
            PathBuildRootW(szRoot, iDrive);
            pchTemp += 2;
            if (*pchTemp == L'\\')
                ++pchTemp;
        }

        bLFN = IsLFNDriveW(szRoot);
        if (!bLFN)
        {
            PWCHAR pch;
            for (pch = pchTemp; *pch != UNICODE_NULL; ++pch)
            {
#define VALID_SHORT_PATH_CHAR_CLASSES ( \
    PATH_CHAR_CLASS_DOT | \
    PATH_CHAR_CLASS_BACKSLASH | \
    PATH_CHAR_CLASS_COLON | \
    PATH_CHAR_CLASS_OTHER_VALID \
)
                if (!PathIsValidCharW(*pch, VALID_SHORT_PATH_CHAR_CLASSES))
                {
                    *pch = L'_';
                }
            }
        }

        StringCchCopyW(pszPath, MAX_PATH, szRoot);
        cchPathLeft -= lstrlenW(pszPath) + 1;
    }
    else /* UNC path: Begins with double backslash */
    {
        bLFN = IsLFNDriveW(pchTemp);
        if (bLFN)
        {
            pszPath[2] = UNICODE_NULL; /* Cut off */
            cchPathLeft -= (2 + 1);
            pchTemp += 2;
        }
        else
        {
            PWCHAR pchSlash = StrChrW(pszPath + 2, L'\\');
            if (pchSlash)
                pchSlash = StrChrW(pchSlash + 1, L'\\');

            if (pchSlash)
            {
                *(pchSlash + 1) = UNICODE_NULL; /* Cut off */
                pchTemp += pchSlash - pszPath;
                cchPathLeft -= (INT)(SIZE_T)(pchSlash - pszPath) + 1;
            }
            else
            {
                bLFN = TRUE;
                pszPath[2] = UNICODE_NULL; /* Cut off */
                cchPathLeft -= 2;
                pchTemp += 2;
            }
        }
    }
    /* Now pszPath is a root-like path or an empty string. */

    /* Start appending the path components of szTemp to pszPath. */
    while (*pchTemp && cchPathLeft > 0)
    {
        /* Collapse any .\ and ..\ parts in the path */
        if (*pchTemp == L'.')
        {
            BOOL bDots = FALSE; /* '.' or '..' ? */

            if (pchTemp[1] == UNICODE_NULL || pchTemp[1] == L'\\')
            {
                /* Component '.' */
                bDots = TRUE;
            }
            else if (pchTemp[1] == L'.' && (pchTemp[2] == UNICODE_NULL || pchTemp[2] == L'\\'))
            {
                /* Component '..' */
                PathRemoveFileSpecW(pszPath); /* Remove the last component from pszPath */
                bDots = TRUE;
            }

            /* If a '.' or '..' was encountered, skip to the next component */
            if (bDots)
            {
                while (*pchTemp && *pchTemp != L'\\')
                {
                    ++pchTemp;
                }

                while (*pchTemp == L'\\')
                {
                    ++pchTemp;
                }

                continue;
            }
        }

        /* Otherwise, copy the other path component */

        if (!PathAddBackslashW(pszPath)) /* Append a backslash at the end */
            break;

        --cchPathLeft;

        pchPath = &pszPath[lstrlenW(pszPath)];

        if (!bLFN) /* Not LFN? */
        {
            /* Copy MS-DOS 8.3 filename */
            PWCHAR pchDot = NULL;
            INT ich;
            WCHAR szTitle[8 + 1] = L"";
            INT cchTitle = 0;
            WCHAR szDotExtension[1 + 3 + 1] = L"";
            INT cchDotExtension = 0;

            /* Copy the component to szTitle and szDotExtension... */
            while (*pchTemp && *pchTemp != L'\\')
            {
                if (*pchTemp == L'.')
                {
                    pchDot = pchTemp; /* Remember the last position */

                    /* Clear szDotExtension */
                    cchDotExtension = 0;
                    ZeroMemory(szDotExtension, sizeof(szDotExtension));
                }

                if (pchDot)
                {
                    if (cchDotExtension < 1 + 3)
                        szDotExtension[cchDotExtension++] = *pchTemp;
                }
                else
                {
                    if (cchTitle < 8)
                        szTitle[cchTitle++] = *pchTemp;
                }

                ++pchTemp;
            }

            /* Add file title 'szTitle' to pchPath */
            for (ich = 0; szTitle[ich] && cchPathLeft > 0; ++ich)
            {
                *pchPath++ = szTitle[ich];
                --cchPathLeft;
            }

            /* Add file extension 'szDotExtension' to pchPath */
            if (pchDot)
            {
                for (ich = 0; szDotExtension[ich] && cchPathLeft > 0; ++ich)
                {
                    *pchPath++ = szDotExtension[ich];
                    --cchPathLeft;
                }
            }
        }
        else /* LFN */
        {
            /* Copy the component up to the next separator */
            while (*pchTemp != UNICODE_NULL && *pchTemp != L'\\' && cchPathLeft > 0)
            {
                *pchPath++ = *pchTemp++;
                --cchPathLeft;
            }
        }

        /* Skip the backslashes */
        while (*pchTemp == L'\\')
        {
            ++pchTemp;
        }

        /* Keep null-terminated */
        *pchPath = UNICODE_NULL;
    }

    /* Remove any trailing backslash */
    PathRemoveBackslashW(pszPath);

    if (!(dwFlags & 1)) /* Remove the trailing dot? */
    {
        pchPath = CharPrevW(pszPath, pszPath + lstrlenW(pszPath));
        if (*pchPath == L'.')
            *pchPath = UNICODE_NULL;
    }
}

BOOL SHELL_GetUserProfileDirectoryW(HANDLE hToken, LPWSTR szPath, LPDWORD lpcchPath)
{
    BOOL result;
    if (!hToken)
    {
        OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
        result = GetUserProfileDirectoryW(hToken, szPath, lpcchPath);
        CloseHandle(hToken);
    }
    else if ((INT) hToken == -1)
    {
        result = GetDefaultUserProfileDirectoryW(szPath, lpcchPath);
    }
    else
    {
        result = GetUserProfileDirectoryW(hToken, szPath, lpcchPath);
    }
    TRACE("SHELL_GetUserProfileDirectoryW returning %S\n", szPath);
    return result;
}
#endif

/*
	########## Combining and Constructing paths ##########
*/

/*************************************************************************
 * PathAppend		[SHELL32.36]
 */
BOOL WINAPI PathAppendAW(
	LPVOID lpszPath1,
	LPCVOID lpszPath2)
{
	if (SHELL_OsIsUnicode())
	  return PathAppendW(lpszPath1, lpszPath2);
	return PathAppendA(lpszPath1, lpszPath2);
}

/*************************************************************************
 * PathCombine	 [SHELL32.37]
 */
LPVOID WINAPI PathCombineAW(
	LPVOID szDest,
	LPCVOID lpszDir,
	LPCVOID lpszFile)
{
	if (SHELL_OsIsUnicode())
	  return PathCombineW( szDest, lpszDir, lpszFile );
	return PathCombineA( szDest, lpszDir, lpszFile );
}

/*************************************************************************
 * PathAddBackslash		[SHELL32.32]
 */
LPVOID WINAPI PathAddBackslashAW(LPVOID lpszPath)
{
	if(SHELL_OsIsUnicode())
	  return PathAddBackslashW(lpszPath);
	return PathAddBackslashA(lpszPath);
}

/*************************************************************************
 * PathBuildRoot		[SHELL32.30]
 */
LPVOID WINAPI PathBuildRootAW(LPVOID lpszPath, int drive)
{
	if(SHELL_OsIsUnicode())
	  return PathBuildRootW(lpszPath, drive);
	return PathBuildRootA(lpszPath, drive);
}

/*
	Extracting Component Parts
*/

/*************************************************************************
 * PathFindFileName	[SHELL32.34]
 */
LPVOID WINAPI PathFindFileNameAW(LPCVOID lpszPath)
{
	if(SHELL_OsIsUnicode())
	  return PathFindFileNameW(lpszPath);
	return PathFindFileNameA(lpszPath);
}

/*************************************************************************
 * PathFindExtension		[SHELL32.31]
 */
LPVOID WINAPI PathFindExtensionAW(LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathFindExtensionW(lpszPath);
	return PathFindExtensionA(lpszPath);

}

/*************************************************************************
 * PathGetExtensionA		[internal]
 *
 * NOTES
 *  exported by ordinal
 *  return value points to the first char after the dot
 */
static LPSTR PathGetExtensionA(LPCSTR lpszPath)
{
	TRACE("(%s)\n",lpszPath);

	lpszPath = PathFindExtensionA(lpszPath);
	return (LPSTR)(*lpszPath?(lpszPath+1):lpszPath);
}

/*************************************************************************
 * PathGetExtensionW		[internal]
 */
static LPWSTR PathGetExtensionW(LPCWSTR lpszPath)
{
	TRACE("(%s)\n",debugstr_w(lpszPath));

	lpszPath = PathFindExtensionW(lpszPath);
	return (LPWSTR)(*lpszPath?(lpszPath+1):lpszPath);
}

/*************************************************************************
 * PathGetExtension		[SHELL32.158]
 */
LPVOID WINAPI PathGetExtensionAW(LPCVOID lpszPath,DWORD void1, DWORD void2)
{
	if (SHELL_OsIsUnicode())
	  return PathGetExtensionW(lpszPath);
	return PathGetExtensionA(lpszPath);
}

/*************************************************************************
 * PathGetArgs	[SHELL32.52]
 */
LPVOID WINAPI PathGetArgsAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathGetArgsW(lpszPath);
	return PathGetArgsA(lpszPath);
}

/*************************************************************************
 * PathGetDriveNumber	[SHELL32.57]
 */
int WINAPI PathGetDriveNumberAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathGetDriveNumberW(lpszPath);
	return PathGetDriveNumberA(lpszPath);
}

/*************************************************************************
 * PathRemoveFileSpec [SHELL32.35]
 */
BOOL WINAPI PathRemoveFileSpecAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathRemoveFileSpecW(lpszPath);
	return PathRemoveFileSpecA(lpszPath);
}

/*************************************************************************
 * PathStripPath	[SHELL32.38]
 */
void WINAPI PathStripPathAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
            PathStripPathW(lpszPath);
        else
            PathStripPathA(lpszPath);
}

/*************************************************************************
 * PathStripToRoot	[SHELL32.50]
 */
BOOL WINAPI PathStripToRootAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathStripToRootW(lpszPath);
	return PathStripToRootA(lpszPath);
}

/*************************************************************************
 * PathRemoveArgs	[SHELL32.251]
 */
void WINAPI PathRemoveArgsAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
            PathRemoveArgsW(lpszPath);
        else
            PathRemoveArgsA(lpszPath);
}

/*************************************************************************
 * PathRemoveExtension	[SHELL32.250]
 */
void WINAPI PathRemoveExtensionAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
            PathRemoveExtensionW(lpszPath);
        else
            PathRemoveExtensionA(lpszPath);
}


/*
	Path Manipulations
*/

/*************************************************************************
 * PathGetShortPathA [internal]
 */
static void PathGetShortPathA(LPSTR pszPath)
{
	CHAR path[MAX_PATH];

	TRACE("%s\n", pszPath);

	if (GetShortPathNameA(pszPath, path, MAX_PATH))
	{
	  lstrcpyA(pszPath, path);
	}
}

/*************************************************************************
 * PathGetShortPathW [internal]
 */
static void PathGetShortPathW(LPWSTR pszPath)
{
	WCHAR path[MAX_PATH];

	TRACE("%s\n", debugstr_w(pszPath));

	if (GetShortPathNameW(pszPath, path, MAX_PATH))
	{
	  lstrcpyW(pszPath, path);
	}
}

/*************************************************************************
 * PathGetShortPath [SHELL32.92]
 */
VOID WINAPI PathGetShortPathAW(LPVOID pszPath)
{
	if(SHELL_OsIsUnicode())
	  PathGetShortPathW(pszPath);
	PathGetShortPathA(pszPath);
}

/*************************************************************************
 * PathRemoveBlanks [SHELL32.33]
 */
void WINAPI PathRemoveBlanksAW(LPVOID str)
{
	if(SHELL_OsIsUnicode())
            PathRemoveBlanksW(str);
        else
            PathRemoveBlanksA(str);
}

/*************************************************************************
 * PathQuoteSpaces [SHELL32.55]
 */
VOID WINAPI PathQuoteSpacesAW (LPVOID lpszPath)
{
	if(SHELL_OsIsUnicode())
            PathQuoteSpacesW(lpszPath);
        else
            PathQuoteSpacesA(lpszPath);
}

/*************************************************************************
 * PathUnquoteSpaces [SHELL32.56]
 */
VOID WINAPI PathUnquoteSpacesAW(LPVOID str)
{
	if(SHELL_OsIsUnicode())
	  PathUnquoteSpacesW(str);
	else
	  PathUnquoteSpacesA(str);
}

/*************************************************************************
 * PathParseIconLocation	[SHELL32.249]
 */
int WINAPI PathParseIconLocationAW (LPVOID lpszPath)
{
	if(SHELL_OsIsUnicode())
	  return PathParseIconLocationW(lpszPath);
	return PathParseIconLocationA(lpszPath);
}

/*
	########## Path Testing ##########
*/
/*************************************************************************
 * PathIsUNC		[SHELL32.39]
 */
BOOL WINAPI PathIsUNCAW (LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathIsUNCW( lpszPath );
	return PathIsUNCA( lpszPath );
}

/*************************************************************************
 *  PathIsRelative	[SHELL32.40]
 */
BOOL WINAPI PathIsRelativeAW (LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathIsRelativeW( lpszPath );
	return PathIsRelativeA( lpszPath );
}

/*************************************************************************
 * PathIsRoot		[SHELL32.29]
 */
BOOL WINAPI PathIsRootAW(LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathIsRootW(lpszPath);
	return PathIsRootA(lpszPath);
}

/*************************************************************************
 *  PathIsExeA		[internal]
 */
static BOOL PathIsExeA (LPCSTR lpszPath)
{
	LPCSTR lpszExtension = PathGetExtensionA(lpszPath);
        int i;
        static const char * const lpszExtensions[] =
            {"exe", "com", "pif", "cmd", "bat", "scf", "scr", NULL };

	TRACE("path=%s\n",lpszPath);

	for(i=0; lpszExtensions[i]; i++)
	  if (!lstrcmpiA(lpszExtension,lpszExtensions[i])) return TRUE;

	return FALSE;
}

/*************************************************************************
 *  PathIsExeW		[internal]
 */
#ifdef __REACTOS__
BOOL PathIsExeW (LPCWSTR lpszPath)
#else
static BOOL PathIsExeW (LPCWSTR lpszPath)
#endif
{
	LPCWSTR lpszExtension = PathGetExtensionW(lpszPath);
        int i;
        static const WCHAR lpszExtensions[][4] =
            {L"exe", L"com", L"pif",L"cmd", L"bat", L"scf",L"scr",L"" };

	TRACE("path=%s\n",debugstr_w(lpszPath));

	for(i=0; lpszExtensions[i][0]; i++)
	  if (!wcsicmp(lpszExtension,lpszExtensions[i])) return TRUE;

	return FALSE;
}

/*************************************************************************
 *  PathIsExe		[SHELL32.43]
 */
BOOL WINAPI PathIsExeAW (LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return PathIsExeW (path);
	return PathIsExeA(path);
}

/*************************************************************************
 * PathIsDirectory	[SHELL32.159]
 */
BOOL WINAPI PathIsDirectoryAW (LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathIsDirectoryW (lpszPath);
	return PathIsDirectoryA (lpszPath);
}

/*************************************************************************
 * PathFileExists	[SHELL32.45]
 */
BOOL WINAPI PathFileExistsAW (LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathFileExistsW (lpszPath);
	return PathFileExistsA (lpszPath);
}

/*************************************************************************
 * PathMatchSpec	[SHELL32.46]
 */
BOOL WINAPI PathMatchSpecAW(LPVOID name, LPVOID mask)
{
	if (SHELL_OsIsUnicode())
	  return PathMatchSpecW( name, mask );
	return PathMatchSpecA( name, mask );
}

/*************************************************************************
 * PathIsSameRoot	[SHELL32.650]
 */
BOOL WINAPI PathIsSameRootAW(LPCVOID lpszPath1, LPCVOID lpszPath2)
{
	if (SHELL_OsIsUnicode())
	  return PathIsSameRootW(lpszPath1, lpszPath2);
	return PathIsSameRootA(lpszPath1, lpszPath2);
}

/*************************************************************************
 * IsLFNDriveA		[SHELL32.41]
 */
BOOL WINAPI IsLFNDriveA(LPCSTR lpszPath)
{
    DWORD	fnlen;

    if (!GetVolumeInformationA(lpszPath, NULL, 0, NULL, &fnlen, NULL, NULL, 0))
	return FALSE;
    return fnlen > 12;
}

/*************************************************************************
 * IsLFNDriveW		[SHELL32.42]
 */
BOOL WINAPI IsLFNDriveW(LPCWSTR lpszPath)
{
    DWORD	fnlen;

    if (!GetVolumeInformationW(lpszPath, NULL, 0, NULL, &fnlen, NULL, NULL, 0))
	return FALSE;
    return fnlen > 12;
}

/*************************************************************************
 * IsLFNDrive		[SHELL32.119]
 */
BOOL WINAPI IsLFNDriveAW(LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return IsLFNDriveW(lpszPath);
	return IsLFNDriveA(lpszPath);
}

/*
	########## Creating Something Unique ##########
*/
/*************************************************************************
 * PathMakeUniqueNameA	[internal]
 */
static BOOL PathMakeUniqueNameA(
	LPSTR lpszBuffer,
	DWORD dwBuffSize,
	LPCSTR lpszShortName,
	LPCSTR lpszLongName,
	LPCSTR lpszPathName)
{
	FIXME("%p %lu %s %s %s stub\n",
	 lpszBuffer, dwBuffSize, debugstr_a(lpszShortName),
	 debugstr_a(lpszLongName), debugstr_a(lpszPathName));
	return TRUE;
}

/*************************************************************************
 * PathMakeUniqueNameW	[internal]
 */
#ifdef __REACTOS__
/* https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-pathmakeuniquename */
static BOOL PathMakeUniqueNameW(
    _Out_ PWSTR pszUniqueName,
    _In_ UINT cchMax,
    _In_ PCWSTR pszTemplate,
    _In_opt_ PCWSTR pszLongPlate, /* Long template */
    _In_opt_ PCWSTR pszDir)
{
    TRACE("%p %u %s %s %s\n",
          pszUniqueName, cchMax, debugstr_w(pszTemplate),
          debugstr_w(pszLongPlate), debugstr_w(pszDir));

    if (!cchMax || !pszUniqueName)
        return FALSE;

    pszUniqueName[0] = UNICODE_NULL;

    PWSTR pszDest = pszUniqueName;
    UINT dirLength = 0;
    if (pszDir)
    {
        if (StringCchCopyW(pszUniqueName, cchMax - 1, pszDir) != S_OK)
            return FALSE;

        pszDest = PathAddBackslashW(pszUniqueName);
        if (!pszDest)
            return FALSE;

        dirLength = lstrlenW(pszDir);
    }

    PCWSTR pszTitle = pszLongPlate ? pszLongPlate : pszTemplate;
    PCWSTR pchDotExt, formatString = L"%d";
    INT maxCount, cchTitle;

    if (   !pszTitle
        || !IsLFNDriveW(pszDir)
#if (NTDDI_VERSION < NTDDI_VISTA)
        || pszDir
#endif
    )
    {
        if (!pszTemplate)
            return FALSE;

        pchDotExt = PathFindExtensionW(pszTemplate);

        cchTitle = pchDotExt - pszTemplate;
        if (cchTitle > 1)
        {
            PCWSTR pch = pchDotExt - 1;
            while (cchTitle > 1 && (L'0' <= *pch && *pch <= L'9'))
            {
                --cchTitle;
                --pch;
            }
        }

#define MSDOS_8DOT3_FILENAME_TITLE_LEN 8
        if (cchTitle > MSDOS_8DOT3_FILENAME_TITLE_LEN - 1)
            cchTitle = MSDOS_8DOT3_FILENAME_TITLE_LEN - 1;

        INT extLength = lstrlenW(pchDotExt);
        while (cchTitle > 1 && (dirLength + cchTitle + extLength + 1 > (cchMax - 1)))
            --cchTitle;

        if (cchTitle <= 0)
            maxCount = 1;
        else if (cchTitle == 1)
            maxCount = 10;
        else
            maxCount = 100;

        pszTitle = pszTemplate;
    }
    else
    {
        PCWSTR openParen = StrChrW(pszTitle, L'(');
        if (openParen)
        {
            while (openParen)
            {
                PCWSTR pch = openParen + 1;
                while (*pch >= L'0' && *pch <= L'9')
                    ++pch;

                if (*pch == L')')
                    break;

                openParen = StrChrW(openParen + 1, L'(');
            }

            if (openParen)
            {
                pchDotExt = openParen + 1;
                cchTitle = pchDotExt - pszTitle;
            }
            else
            {
                pchDotExt = PathFindExtensionW(pszTitle);
                cchTitle = pchDotExt - pszTitle;
                formatString = L" (%d)";
            }
        }
        else
        {
            pchDotExt = PathFindExtensionW(pszTitle);
            cchTitle = pchDotExt - pszTitle;
            formatString = L" (%d)";
        }

        INT remainingChars = cchMax - (dirLength + cchTitle + (lstrlenW(formatString) - 2));
        if (remainingChars <= 0)
            maxCount = 1;
        else if (remainingChars == 1)
            maxCount = 10;
        else if (remainingChars == 2)
            maxCount = 100;
        else
            maxCount = 1000;
    }

    if (StringCchCopyNW(pszDest, cchMax - dirLength, pszTitle, cchTitle) != S_OK)
        return FALSE;

    PWSTR pchTitle = pszDest + cchTitle;
    INT count;
    for (count = 1; count < maxCount; ++count)
    {
        WCHAR tempName[MAX_PATH];
        if (StringCchPrintfW(tempName, _countof(tempName), formatString, count) != S_OK ||
            StringCchCatW(tempName, _countof(tempName), pchDotExt) != S_OK)
        {
            return FALSE;
        }

        if (StringCchCopyW(pchTitle, cchMax - (pchTitle - pszUniqueName), tempName) != S_OK)
            return FALSE;

        if (!PathFileExistsW(pszUniqueName))
            return TRUE;
    }

    pszUniqueName[0] = UNICODE_NULL;
    return FALSE;
}
#else
static BOOL PathMakeUniqueNameW(
	LPWSTR lpszBuffer,
	DWORD dwBuffSize,
	LPCWSTR lpszShortName,
	LPCWSTR lpszLongName,
	LPCWSTR lpszPathName)
{
	FIXME("%p %lu %s %s %s stub\n",
	 lpszBuffer, dwBuffSize, debugstr_w(lpszShortName),
	 debugstr_w(lpszLongName), debugstr_w(lpszPathName));
	return TRUE;
}
#endif

/*************************************************************************
 * PathMakeUniqueName	[SHELL32.47]
 */
BOOL WINAPI PathMakeUniqueNameAW(
	LPVOID lpszBuffer,
	DWORD dwBuffSize,
	LPCVOID lpszShortName,
	LPCVOID lpszLongName,
	LPCVOID lpszPathName)
{
	if (SHELL_OsIsUnicode())
	  return PathMakeUniqueNameW(lpszBuffer,dwBuffSize, lpszShortName,lpszLongName,lpszPathName);
	return PathMakeUniqueNameA(lpszBuffer,dwBuffSize, lpszShortName,lpszLongName,lpszPathName);
}

/*************************************************************************
 * PathYetAnotherMakeUniqueName [SHELL32.75]
 */
BOOL WINAPI PathYetAnotherMakeUniqueName(LPWSTR buffer, LPCWSTR path, LPCWSTR shortname, LPCWSTR longname)
{
    WCHAR pathW[MAX_PATH], retW[MAX_PATH];
    const WCHAR *file, *ext;
    int i = 2;

    TRACE("(%p, %s, %s, %s)\n", buffer, debugstr_w(path), debugstr_w(shortname), debugstr_w(longname));

    file = longname ? longname : shortname;
    PathCombineW(pathW, path, file);
    lstrcpyW(retW, pathW);
    PathRemoveExtensionW(pathW);

    ext = PathFindExtensionW(file);

    /* now try to make it unique */
    while (PathFileExistsW(retW))
    {
        swprintf(retW, ARRAY_SIZE(retW), L"%s (%d)%s", pathW, i, ext);
        i++;
    }

    lstrcpyW(buffer, retW);
    TRACE("ret - %s\n", debugstr_w(buffer));

    return TRUE;
}

/*
	########## cleaning and resolving paths ##########
 */

/*************************************************************************
 * PathFindOnPath	[SHELL32.145]
 */
BOOL WINAPI PathFindOnPathAW(LPVOID sFile, LPCVOID *sOtherDirs)
{
	if (SHELL_OsIsUnicode())
	  return PathFindOnPathW(sFile, (LPCWSTR *)sOtherDirs);
	return PathFindOnPathA(sFile, (LPCSTR *)sOtherDirs);
}

/*************************************************************************
 * PathCleanupSpec	[SHELL32.171]
 *
 * lpszFile is changed in place.
 */
int WINAPI PathCleanupSpec( LPCWSTR lpszPathW, LPWSTR lpszFileW )
{
    int i = 0;
    DWORD rc = 0;
    int length = 0;

    if (SHELL_OsIsUnicode())
    {
        LPWSTR p = lpszFileW;

        TRACE("Cleanup %s\n",debugstr_w(lpszFileW));

        if (lpszPathW)
            length = lstrlenW(lpszPathW);

        while (*p)
        {
            int gct = PathGetCharTypeW(*p);
            if (gct == GCT_INVALID || gct == GCT_WILD || gct == GCT_SEPARATOR)
            {
                lpszFileW[i]='-';
                rc |= PCS_REPLACEDCHAR;
            }
            else
                lpszFileW[i]=*p;
            i++;
            p++;
            if (length + i == MAX_PATH)
            {
                rc |= PCS_FATAL | PCS_PATHTOOLONG;
                break;
            }
        }
        lpszFileW[i]=0;
    }
    else
    {
        LPSTR lpszFileA = (LPSTR)lpszFileW;
        LPCSTR lpszPathA = (LPCSTR)lpszPathW;
        LPSTR p = lpszFileA;

        TRACE("Cleanup %s\n",debugstr_a(lpszFileA));

        if (lpszPathA)
            length = strlen(lpszPathA);

        while (*p)
        {
            int gct = PathGetCharTypeA(*p);
            if (gct == GCT_INVALID || gct == GCT_WILD || gct == GCT_SEPARATOR)
            {
                lpszFileA[i]='-';
                rc |= PCS_REPLACEDCHAR;
            }
            else
                lpszFileA[i]=*p;
            i++;
            p++;
            if (length + i == MAX_PATH)
            {
                rc |= PCS_FATAL | PCS_PATHTOOLONG;
                break;
            }
        }
        lpszFileA[i]=0;
    }
    return rc;
}

#ifdef __REACTOS__
VOID WINAPI PathQualifyA(LPSTR pszPath)
{
    WCHAR szPath[MAX_PATH];
    TRACE("%s\n",pszPath);
    SHAnsiToUnicode(pszPath, szPath, _countof(szPath));
    PathQualifyW(szPath);
    SHUnicodeToAnsi(szPath, pszPath, MAX_PATH);
}

/*************************************************************************
 * PathQualifyW		[SHELL32]
 */
VOID WINAPI PathQualifyW(LPWSTR pszPath)
{
    TRACE("%s\n",debugstr_w(pszPath));
    PathQualifyExW(pszPath, NULL, 0);
}

/*************************************************************************
 * PathQualify	[SHELL32.49]
 */
VOID WINAPI PathQualifyAW(LPVOID pszPath)
{
    if (SHELL_OsIsUnicode())
        PathQualifyW(pszPath);
    else
        PathQualifyA(pszPath);
}
BOOL WINAPI PathResolveA(LPSTR path, LPCSTR *dirs, DWORD flags)
{
    BOOL ret = FALSE;
    LPWSTR *dirsW = NULL;
    DWORD iDir, cDirs, cbDirs;
    WCHAR pathW[MAX_PATH];

    TRACE("(%s,%p,0x%08x)\n", debugstr_a(path), dirs, flags);

    if (dirs)
    {
        for (cDirs = 0; dirs[cDirs]; ++cDirs)
            ;

        cbDirs = (cDirs + 1) * sizeof(LPWSTR);
        dirsW = SHAlloc(cbDirs);
        if (!dirsW)
            goto Cleanup;

        ZeroMemory(dirsW, cbDirs);
        for (iDir = 0; iDir < cDirs; ++iDir)
        {
            __SHCloneStrAtoW(&dirsW[iDir], dirs[iDir]);
            if (dirsW[iDir] == NULL)
                goto Cleanup;
        }
    }

    SHAnsiToUnicode(path, pathW, _countof(pathW));

    ret = PathResolveW(pathW, (LPCWSTR*)dirsW, flags);
    if (ret)
        SHUnicodeToAnsi(pathW, path, MAX_PATH);

Cleanup:
    if (dirsW)
    {
        for (iDir = 0; iDir < cDirs; ++iDir)
        {
            SHFree(dirsW[iDir]);
        }
        SHFree(dirsW);
    }
    return ret;
}

BOOL WINAPI PathResolveW(_Inout_ LPWSTR path, _Inout_opt_ LPCWSTR *dirs, _In_ DWORD flags)
{
    DWORD dwWhich = WHICH_DEFAULT; /* The extensions to be searched */

    TRACE("(%s,%p,0x%08x)\n", debugstr_w(path), dirs, flags);

    if (flags & PRF_DONTFINDLNK)
        dwWhich &= ~WHICH_LNK; /* Don't search '.LNK' (shortcut) */

    if (flags & PRF_VERIFYEXISTS)
        SetLastError(ERROR_FILE_NOT_FOUND); /* We set this error code at first in verification */

    PathUnquoteSpacesW(path);

    if (PathIsRootW(path)) /* Root path */
    {
        if (path[0] == L'\\' && path[1] == UNICODE_NULL) /* '\' only? */
            PathQualifyExW(path, ((flags & PRF_FIRSTDIRDEF) ? *dirs : NULL), 0); /* Qualify */

        if (flags & PRF_VERIFYEXISTS)
            return PathFileExistsAndAttributesW(path, NULL); /* Check the existence */

        return TRUE;
    }

    if (PathIsFileSpecW(path)) /* Filename only */
    {
        /* Try to find the path with program extensions applied? */
        if ((flags & PRF_TRYPROGRAMEXTENSIONS) &&
            PathSearchOnExtensionsW(path, dirs, TRUE, dwWhich))
        {
            return TRUE; /* Found */
        }

        /* Try to find the filename in the directories */
        if (PathFindOnPathW(path, dirs))
            goto CheckAbsoluteAndFinish;

        return FALSE; /* Not found */
    }

    if (PathIsURLW(path)) /* URL? */
        return FALSE;

    /* Qualify the path */
    PathQualifyExW(path, ((flags & PRF_FIRSTDIRDEF) ? *dirs : NULL), 1);

    TRACE("(%s)\n", debugstr_w(path));

    if (!(flags & PRF_VERIFYEXISTS)) /* Don't verify the existence? */
        return TRUE;

    /* Try to find the path with program extensions applied? */
    if (!(flags & PRF_TRYPROGRAMEXTENSIONS) ||
        !PathSearchOnExtensionsW(path, dirs, FALSE, dwWhich))
    {
        if (!PathFileExistsAndAttributesW(path, NULL))
            return FALSE; /* Not found */
    }

CheckAbsoluteAndFinish:
#if (_WIN32_WINNT >= _WIN32_WINNT_WS03)
    if (!(flags & PRF_REQUIREABSOLUTE) || PathIsAbsoluteW(path))
        return TRUE;

    if (!PathMakeAbsoluteW(path))
        return FALSE;

    return PathFileExistsAndAttributesW(path, NULL);
#else
    return TRUE; /* Found */
#endif
}

/*************************************************************************
 * PathResolve [SHELL32.51]
 */
BOOL WINAPI PathResolveAW(LPVOID path, LPCVOID *paths, DWORD flags)
{
    if (SHELL_OsIsUnicode())
        return PathResolveW(path, (LPCWSTR*)paths, flags);
    else
        return PathResolveA(path, (LPCSTR*)paths, flags);
}
#else
/*************************************************************************
 * PathQualifyA		[SHELL32]
 */
static BOOL PathQualifyA(LPCSTR pszPath)
{
	FIXME("%s\n",pszPath);
	return FALSE;
}

/*************************************************************************
 * PathQualifyW		[SHELL32]
 */
static BOOL PathQualifyW(LPCWSTR pszPath)
{
	FIXME("%s\n",debugstr_w(pszPath));
	return FALSE;
}

/*************************************************************************
 * PathQualify	[SHELL32.49]
 */
BOOL WINAPI PathQualifyAW(LPCVOID pszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathQualifyW(pszPath);
	return PathQualifyA(pszPath);
}

static BOOL PathResolveA(char *path, const char **dirs, DWORD flags)
{
    BOOL is_file_spec = PathIsFileSpecA(path);
    DWORD dwWhich = flags & PRF_DONTFINDLNK ? 0xf : 0xbf;

    TRACE("(%s,%p,0x%08lx)\n", debugstr_a(path), dirs, flags);

    if (flags & PRF_VERIFYEXISTS)
    {
        if (PathFindOnPathExA(path, dirs, dwWhich))
        {
            if (!PathIsFileSpecA(path)) GetFullPathNameA(path, MAX_PATH, path, NULL);
            return TRUE;
        }
        if (!is_file_spec)
        {
            GetFullPathNameA(path, MAX_PATH, path, NULL);
            if (PathFileExistsDefExtA(path, dwWhich))
                return TRUE;
        }
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (is_file_spec)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    GetFullPathNameA(path, MAX_PATH, path, NULL);

    return TRUE;
}

BOOL WINAPI PathResolveW(WCHAR *path, const WCHAR **dirs, DWORD flags)
{
    BOOL is_file_spec = PathIsFileSpecW(path);
    DWORD dwWhich = flags & PRF_DONTFINDLNK ? 0xf : 0xbf;

    TRACE("(%s,%p,0x%08lx)\n", debugstr_w(path), dirs, flags);

    if (flags & PRF_VERIFYEXISTS)
    {
        if (PathFindOnPathExW(path, dirs, dwWhich))
        {
            if (!PathIsFileSpecW(path)) GetFullPathNameW(path, MAX_PATH, path, NULL);
            return TRUE;
        }
        if (!is_file_spec)
        {
            GetFullPathNameW(path, MAX_PATH, path, NULL);
            if (PathFileExistsDefExtW(path, dwWhich))
                return TRUE;
        }
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (is_file_spec)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    GetFullPathNameW(path, MAX_PATH, path, NULL);

    return TRUE;
}

/*************************************************************************
 * PathResolve [SHELL32.51]
 */
BOOL WINAPI PathResolveAW(void *path, const void **paths, DWORD flags)
{
    if (SHELL_OsIsUnicode())
        return PathResolveW(path, (const WCHAR **)paths, flags);
    else
        return PathResolveA(path, (const char **)paths, flags);
}
#endif

/*************************************************************************
*	PathProcessCommandA
*/
static LONG PathProcessCommandA (
	LPCSTR lpszPath,
	LPSTR lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	FIXME("%s %p 0x%04lx 0x%04lx stub\n",
	lpszPath, lpszBuff, dwBuffSize, dwFlags);
	if(!lpszPath) return -1;
	if(lpszBuff) strcpy(lpszBuff, lpszPath);
	return strlen(lpszPath);
}

#ifndef __REACTOS__ // See ../shlexec.cpp
/*************************************************************************
*	PathProcessCommandW
*/
static LONG PathProcessCommandW (
	LPCWSTR lpszPath,
	LPWSTR lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	FIXME("(%s, %p, 0x%04lx, 0x%04lx) stub\n",
	debugstr_w(lpszPath), lpszBuff, dwBuffSize, dwFlags);
	if(!lpszPath) return -1;
	if(lpszBuff) lstrcpyW(lpszBuff, lpszPath);
	return lstrlenW(lpszPath);
}
#endif

/*************************************************************************
*	PathProcessCommand (SHELL32.653)
*/
LONG WINAPI PathProcessCommandAW (
	LPCVOID lpszPath,
	LPVOID lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	if (SHELL_OsIsUnicode())
	  return PathProcessCommandW(lpszPath, lpszBuff, dwBuffSize, dwFlags);
	return PathProcessCommandA(lpszPath, lpszBuff, dwBuffSize, dwFlags);
}

/*
	########## special ##########
*/

/*************************************************************************
 * PathSetDlgItemPath (SHELL32.48)
 */
VOID WINAPI PathSetDlgItemPathAW(HWND hDlg, int id, LPCVOID pszPath)
{
	if (SHELL_OsIsUnicode())
            PathSetDlgItemPathW(hDlg, id, pszPath);
        else
            PathSetDlgItemPathA(hDlg, id, pszPath);
}

typedef enum _CSIDL_Type {
    CSIDL_Type_User,
    CSIDL_Type_AllUsers,
    CSIDL_Type_CurrVer,
    CSIDL_Type_Disallowed,
    CSIDL_Type_NonExistent,
    CSIDL_Type_WindowsPath,
    CSIDL_Type_SystemPath,
    CSIDL_Type_SystemX86Path,
    CSIDL_Type_ProgramData
} CSIDL_Type;

#define CSIDL_CONTACTS         0x0043
#define CSIDL_DOWNLOADS        0x0047
#define CSIDL_LINKS            0x004d
#define CSIDL_APPDATA_LOCALLOW 0x004e
#define CSIDL_SAVED_GAMES      0x0062
#define CSIDL_SEARCHES         0x0063

typedef struct
{
    IApplicationDestinations IApplicationDestinations_iface;
    LONG ref;
} IApplicationDestinationsImpl;

static inline IApplicationDestinationsImpl *impl_from_IApplicationDestinations( IApplicationDestinations *iface )
{
    return CONTAINING_RECORD(iface, IApplicationDestinationsImpl, IApplicationDestinations_iface);
}

static HRESULT WINAPI ApplicationDestinations_QueryInterface(IApplicationDestinations *iface, REFIID riid,
                                                             LPVOID *ppv)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppv);

    if (ppv == NULL)
        return E_POINTER;

    if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IApplicationDestinations, riid))
    {
        *ppv = &This->IApplicationDestinations_iface;
        IUnknown_AddRef((IUnknown*)*ppv);

        TRACE("Returning IApplicationDestinations: %p\n", *ppv);
        return S_OK;
    }

    *ppv = NULL;
    FIXME("(%p)->(%s, %p) interface not supported.\n", This, debugstr_guid(riid), ppv);

    return E_NOINTERFACE;
}

static ULONG WINAPI ApplicationDestinations_AddRef(IApplicationDestinations *iface)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p), new refcount=%li\n", This, ref);

    return ref;
}

static ULONG WINAPI ApplicationDestinations_Release(IApplicationDestinations *iface)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p), new refcount=%li\n", This, ref);

    if (ref == 0)
        free(This);

    return ref;
}

static HRESULT WINAPI ApplicationDestinations_SetAppID(IApplicationDestinations *iface, const WCHAR *appid)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);

    FIXME("(%p, %s) stub!\n", This, debugstr_w(appid));

    return E_NOTIMPL;
}

static HRESULT WINAPI ApplicationDestinations_RemoveDestination(IApplicationDestinations *iface, IUnknown *punk)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);

    FIXME("(%p, %p) stub!\n", This, punk);

    return E_NOTIMPL;
}

static HRESULT WINAPI ApplicationDestinations_RemoveAllDestinations(IApplicationDestinations *iface)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);

    FIXME("(%p) stub!\n", This);

    return E_NOTIMPL;
}

static const IApplicationDestinationsVtbl ApplicationDestinationsVtbl =
{
    ApplicationDestinations_QueryInterface,
    ApplicationDestinations_AddRef,
    ApplicationDestinations_Release,
    ApplicationDestinations_SetAppID,
    ApplicationDestinations_RemoveDestination,
    ApplicationDestinations_RemoveAllDestinations
};

HRESULT WINAPI ApplicationDestinations_Constructor(IUnknown *outer, REFIID riid, LPVOID *ppv)
{
    IApplicationDestinationsImpl *This;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(This = SHAlloc(sizeof(*This))))
        return E_OUTOFMEMORY;

    This->IApplicationDestinations_iface.lpVtbl = &ApplicationDestinationsVtbl;
    This->ref = 0;

    hr = IUnknown_QueryInterface(&This->IApplicationDestinations_iface, riid, ppv);
    if (FAILED(hr))
        SHFree(This);

    return hr;
}

typedef struct
{
    IApplicationDocumentLists IApplicationDocumentLists_iface;
    LONG ref;
} IApplicationDocumentListsImpl;

static inline IApplicationDocumentListsImpl *impl_from_IApplicationDocumentLists( IApplicationDocumentLists *iface )
{
    return CONTAINING_RECORD(iface, IApplicationDocumentListsImpl, IApplicationDocumentLists_iface);
}

static HRESULT WINAPI ApplicationDocumentLists_QueryInterface(IApplicationDocumentLists *iface,
                                                              REFIID riid, LPVOID *ppv)
{
    IApplicationDocumentListsImpl *This = impl_from_IApplicationDocumentLists(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppv);

    if (ppv == NULL)
        return E_POINTER;

    if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IApplicationDocumentLists, riid))
    {
        *ppv = &This->IApplicationDocumentLists_iface;
        IUnknown_AddRef((IUnknown*)*ppv);

        TRACE("Returning IApplicationDocumentLists: %p\n", *ppv);
        return S_OK;
    }

    *ppv = NULL;
    FIXME("(%p)->(%s, %p) interface not supported.\n", This, debugstr_guid(riid), ppv);

    return E_NOINTERFACE;
}

static ULONG WINAPI ApplicationDocumentLists_AddRef(IApplicationDocumentLists *iface)
{
    IApplicationDocumentListsImpl *This = impl_from_IApplicationDocumentLists(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p), new refcount=%li\n", This, ref);

    return ref;
}

static ULONG WINAPI ApplicationDocumentLists_Release(IApplicationDocumentLists *iface)
{
    IApplicationDocumentListsImpl *This = impl_from_IApplicationDocumentLists(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p), new refcount=%li\n", This, ref);

    if (ref == 0)
        free(This);

    return ref;
}

static HRESULT WINAPI ApplicationDocumentLists_SetAppID(IApplicationDocumentLists *iface,
                                                        const WCHAR *appid)
{
    IApplicationDocumentListsImpl *This = impl_from_IApplicationDocumentLists(iface);

    FIXME("(%p, %s) stub!\n", This, debugstr_w(appid));

    return E_NOTIMPL;
}

static HRESULT WINAPI ApplicationDocumentLists_GetList(IApplicationDocumentLists *iface,
                                                       APPDOCLISTTYPE list_type, UINT item_count,
                                                       REFIID riid, void **obj)
{
    IApplicationDocumentListsImpl *This = impl_from_IApplicationDocumentLists(iface);

    FIXME("(%p, %u, %u, %s, %p): stub\n", This, list_type, item_count, debugstr_guid(riid), obj);

    return E_NOTIMPL;
}

static const IApplicationDocumentListsVtbl ApplicationDocumentListsVtbl =
{
    ApplicationDocumentLists_QueryInterface,
    ApplicationDocumentLists_AddRef,
    ApplicationDocumentLists_Release,
    ApplicationDocumentLists_SetAppID,
    ApplicationDocumentLists_GetList
};

HRESULT WINAPI ApplicationDocumentLists_Constructor(IUnknown *outer, REFIID riid, LPVOID *ppv)
{
    IApplicationDocumentListsImpl *This;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(This = SHAlloc(sizeof(*This))))
        return E_OUTOFMEMORY;

    This->IApplicationDocumentLists_iface.lpVtbl = &ApplicationDocumentListsVtbl;
    This->ref = 0;

    hr = IUnknown_QueryInterface(&This->IApplicationDocumentLists_iface, riid, ppv);
    if (FAILED(hr))
        SHFree(This);

    return hr;
}

typedef struct
{
    const KNOWNFOLDERID *id;
    CSIDL_Type type;
    const WCHAR *value;
    const WCHAR *def_path; /* fallback string or resource ID */
    KF_CATEGORY category;
    const WCHAR *name;
    const KNOWNFOLDERID *parent;
const WCHAR *path;
    const WCHAR *parsing;
    DWORD attributes;
    KF_DEFINITION_FLAGS flags;
    const FOLDERTYPEID *typeid;
#ifdef __REACTOS__
    INT        nShell32IconIndex;
#endif
} CSIDL_DATA;

static const CSIDL_DATA CSIDL_Data[] =
{
    { /* 0x00 - CSIDL_DESKTOP */
        .id         = &FOLDERID_Desktop,
        .type       = CSIDL_Type_User,
        .value      = L"Desktop",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Desktop",
        .path       = L"Desktop",
        .attributes = FILE_ATTRIBUTE_READONLY,
    },
    { /* 0x01 - CSIDL_INTERNET */
        .id         = &FOLDERID_InternetFolder,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"InternetFolder",
        .parsing    = L"::{871C5380-42A0-1069-A2EA-08002B30309D}",
    },
    { /* 0x02 - CSIDL_PROGRAMS */
        .id         = &FOLDERID_Programs,
        .type       = CSIDL_Type_User,
        .value      = L"Programs",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Programs",
        .parent     = &FOLDERID_StartMenu,
        .path       = L"Programs",
        .attributes = FILE_ATTRIBUTE_READONLY,
    },
    { /* 0x03 - CSIDL_CONTROLS (.CPL files) */
        .id         = &FOLDERID_ControlPanelFolder,
        .type       = CSIDL_Type_SystemPath,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"ControlPanelFolder",
        .path       = L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}",
        .parsing    = L"::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0",
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_CONTROL_PANEL
#endif
    },
    { /* 0x04 - CSIDL_PRINTERS */
        .id         = &FOLDERID_PrintersFolder,
        .type       = CSIDL_Type_SystemPath,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"PrintersFolder",
        .parsing    = L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}",
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_PRINTERS_FOLDER
#endif
    },
    { /* 0x05 - CSIDL_PERSONAL */
        .id         = &FOLDERID_Documents,
        .type       = CSIDL_Type_User,
        .value      = L"Personal",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Personal",
        .parent     = &FOLDERID_Profile,
        .path       = L"Documents",
        .parsing    = L"::{59031a47-3f72-44a7-89c5-5595fe6b30ee}\\{FDD39AD0-238F-46AF-ADB4-6C85480369C7}",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_ROAMABLE | KFDF_PRECREATE,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_MY_DOCUMENTS
#endif
    },
    { /* 0x06 - CSIDL_FAVORITES */
        .id         = &FOLDERID_Favorites,
        .type       = CSIDL_Type_User,
        .value      = L"Favorites",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Favorites",
        .path       = L"Favorites",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_FAVORITES
#endif
    },
    { /* 0x07 - CSIDL_STARTUP */
        .id         = &FOLDERID_Startup,
        .type       = CSIDL_Type_User,
        .value      = L"StartUp",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Startup",
        .parent     = &FOLDERID_Programs,
        .path       = L"StartUp",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x08 - CSIDL_RECENT */
        .id         = &FOLDERID_Recent,
        .type       = CSIDL_Type_User,
        .value      = L"Recent",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Recent",
        .parent     = &FOLDERID_RoamingAppData,
        .path       = L"Microsoft\\Windows\\Recent",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_RECENT_DOCUMENTS
#endif
    },
    { /* 0x09 - CSIDL_SENDTO */
        .id         = &FOLDERID_SendTo,
        .type       = CSIDL_Type_User,
        .value      = L"SendTo",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"SendTo",
        .parent     = &FOLDERID_RoamingAppData,
        .path       = L"Microsoft\\Windows\\SendTo",
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x0a - CSIDL_BITBUCKET - Recycle Bin */
        .id         = &FOLDERID_RecycleBinFolder,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"RecycleBinFolder",
        .parsing    = L"::{645FF040-5081-101B-9F08-00AA002F954E}",
    },
    { /* 0x0b - CSIDL_STARTMENU */
        .id         = &FOLDERID_StartMenu,
        .type       = CSIDL_Type_User,
        .value      = L"Start Menu",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Start Menu",
        .parent     = &FOLDERID_RoamingAppData,
        .path       = L"Microsoft\\Windows\\Start Menu",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_TSKBAR_STARTMENU
#endif
    },
    { /* 0x0c - CSIDL_MYDOCUMENTS */
        .id         = &GUID_NULL,
        .type       = CSIDL_Type_Disallowed, /* matches WinXP--can't get its path */
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_MY_DOCUMENTS
#endif
    },
    { /* 0x0d - CSIDL_MYMUSIC */
        .id         = &FOLDERID_Music,
        .type       = CSIDL_Type_User,
        .value      = L"My Music",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"My Music",
        .parent     = &FOLDERID_Profile,
        .path       = L"Music",
        .parsing    = L"::{59031a47-3f72-44a7-89c5-5595fe6b30ee}\\{4BD8D571-6D19-48D3-BE97-422220080E43}",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_ROAMABLE | KFDF_PRECREATE,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_MY_MUSIC
#endif
    },
    { /* 0x0e - CSIDL_MYVIDEO */
        .id         = &FOLDERID_Videos,
        .type       = CSIDL_Type_User,
        .value      = L"My Videos",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"My Video",
        .parent     = &FOLDERID_Profile,
        .path       = L"Videos",
        .parsing    = L"::{59031a47-3f72-44a7-89c5-5595fe6b30ee}\\{18989B1D-99B5-455B-841C-AB7C74E4DDFC}",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_ROAMABLE | KFDF_PRECREATE,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_MY_MOVIES
#endif
    },
    { /* 0x0f - unassigned */
        .id         = &GUID_NULL,
        .type       = CSIDL_Type_Disallowed,
    },
    { /* 0x10 - CSIDL_DESKTOPDIRECTORY */
        .id         = &FOLDERID_Desktop,
        .type       = CSIDL_Type_User,
        .value      = L"Desktop",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Desktop",
        .parent     = &FOLDERID_Profile,
        .path       = L"Desktop",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH,
#ifdef __REACTOS__
        .nShell32IconIndex = 0
#endif
    },
    { /* 0x11 - CSIDL_DRIVES */
        .id         = &FOLDERID_ComputerFolder,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"MyComputerFolder",
        .parsing    = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}",
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_COMPUTER_FOLDER
#endif
    },
    { /* 0x12 - CSIDL_NETWORK */
        .id         = &FOLDERID_NetworkFolder,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"NetworkPlacesFolder",
        .parsing    = L"::{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}",
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_NETWORK_FOLDER
#endif
    },
    { /* 0x13 - CSIDL_NETHOOD */
        .id         = &FOLDERID_NetHood,
        .type       = CSIDL_Type_User,
        .value      = L"NetHood",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"NetHood",
        .parent     = &FOLDERID_RoamingAppData,
        .path       = L"Microsoft\\Windows\\Network Shortcuts",
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_NETWORK
#endif
    },
    { /* 0x14 - CSIDL_FONTS */
        .id         = &FOLDERID_Fonts,
        .type       = CSIDL_Type_WindowsPath,
        .value      = L"Fonts",
        .def_path   = L"Fonts",
        .category   = KF_CATEGORY_FIXED,
        .name       = L"Fonts",
        .parent     = &FOLDERID_Windows,
        .typeid     = &FOLDERID_Windows,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_FONTS_FOLDER
#endif
    },
    { /* 0x15 - CSIDL_TEMPLATES */
        .id         = &FOLDERID_Templates,
        .type       = CSIDL_Type_User,
        .value      = L"Templates",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Templates",
        .parent     = &FOLDERID_RoamingAppData,
        .path       = L"Microsoft\\Windows\\Templates",
    },
    { /* 0x16 - CSIDL_COMMON_STARTMENU */
        .id         = &FOLDERID_CommonStartMenu,
        .type       = CSIDL_Type_ProgramData,
        .value      = L"Common Start Menu",
        .category   = KF_CATEGORY_COMMON,
        .name       = L"Common Start Menu",
        .parent     = &FOLDERID_ProgramData,
        .path       = L"Microsoft\\Windows\\Start Menu",
        .attributes = FILE_ATTRIBUTE_READONLY,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_TSKBAR_STARTMENU
#endif
    },
    { /* 0x17 - CSIDL_COMMON_PROGRAMS */
        .id         = &FOLDERID_CommonPrograms,
        .type       = CSIDL_Type_ProgramData,
        .value      = L"Common Programs",
        .category   = KF_CATEGORY_COMMON,
        .name       = L"Common Programs",
        .parent     = &FOLDERID_CommonStartMenu,
        .path       = L"Programs",
        .attributes = FILE_ATTRIBUTE_READONLY,
    },
    { /* 0x18 - CSIDL_COMMON_STARTUP */
        .id         = &FOLDERID_CommonStartup,
        .type       = CSIDL_Type_ProgramData,
        .value      = L"Common StartUp",
        .category   = KF_CATEGORY_COMMON,
        .name       = L"Common Startup",
        .parent     = &FOLDERID_CommonPrograms,
        .path       = L"StartUp",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x19 - CSIDL_COMMON_DESKTOPDIRECTORY */
        .id         = &FOLDERID_PublicDesktop,
        .type       = CSIDL_Type_AllUsers,
        .value      = L"Common Desktop",
        .category   = KF_CATEGORY_COMMON,
        .name       = L"Common Desktop",
        .parent     = &FOLDERID_Public,
        .path       = L"Desktop",
        .attributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x1a - CSIDL_APPDATA */
        .id         = &FOLDERID_RoamingAppData,
        .type       = CSIDL_Type_User,
        .value      = L"AppData",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"AppData",
        .parent     = &FOLDERID_Profile,
        .path       = L"AppData\\Roaming",
    },
    { /* 0x1b - CSIDL_PRINTHOOD */
        .id         = &FOLDERID_PrintHood,
        .type       = CSIDL_Type_User,
        .value      = L"PrintHood",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"PrintHood",
        .parent     = &FOLDERID_RoamingAppData,
        .path       = L"Microsoft\\Windows\\Printer Shortcuts",
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_PRINTERS_FOLDER
#endif
    },
    { /* 0x1c - CSIDL_LOCAL_APPDATA */
        .id         = &FOLDERID_LocalAppData,
        .type       = CSIDL_Type_User,
        .value      = L"Local AppData",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Local AppData",
        .parent     = &FOLDERID_Profile,
        .path       = L"AppData\\Local",
        .flags      = KFDF_LOCAL_REDIRECT_ONLY | KFDF_PUBLISHEXPANDEDPATH,
    },
    { /* 0x1d - CSIDL_ALTSTARTUP */
        .id         = &GUID_NULL,
        .type       = CSIDL_Type_NonExistent,
    },
    { /* 0x1e - CSIDL_COMMON_ALTSTARTUP */
        .id         = &GUID_NULL,
        .type       = CSIDL_Type_NonExistent,
    },
    { /* 0x1f - CSIDL_COMMON_FAVORITES */
        .id         = &FOLDERID_Favorites,
        .type       = CSIDL_Type_AllUsers,
        .value      = L"Common Favorites",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Favorites",
        .parent     = &FOLDERID_Profile,
        .path       = L"Favorites",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_FAVORITES
#endif
    },
    { /* 0x20 - CSIDL_INTERNET_CACHE */
        .id         = &FOLDERID_InternetCache,
        .type       = CSIDL_Type_User,
        .value      = L"Cache",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Cache",
        .parent     = &FOLDERID_LocalAppData,
        .path       = L"Microsoft\\Windows\\INetCache",
        .flags      = KFDF_LOCAL_REDIRECT_ONLY,
    },
    { /* 0x21 - CSIDL_COOKIES */
        .id         = &FOLDERID_Cookies,
        .type       = CSIDL_Type_User,
        .value      = L"Cookies",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Cookies",
        .parent     = &FOLDERID_LocalAppData,
        .path       = L"Microsoft\\Windows\\INetCookies",
    },
    { /* 0x22 - CSIDL_HISTORY */
        .id         = &FOLDERID_History,
        .type       = CSIDL_Type_User,
        .value      = L"History",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"History",
        .parent     = &FOLDERID_LocalAppData,
        .path       = L"Microsoft\\Windows\\History",
        .flags      = KFDF_LOCAL_REDIRECT_ONLY,
    },
    { /* 0x23 - CSIDL_COMMON_APPDATA */
        .id         = &FOLDERID_ProgramData,
        .type       = CSIDL_Type_ProgramData,
        .value      = L"Common AppData",
        .category   = KF_CATEGORY_FIXED,
        .name       = L"Common AppData",
    },
    { /* 0x24 - CSIDL_WINDOWS */
        .id         = &FOLDERID_Windows,
        .type       = CSIDL_Type_WindowsPath,
        .category   = KF_CATEGORY_FIXED,
        .name       = L"Windows",
    },
    { /* 0x25 - CSIDL_SYSTEM */
        .id         = &FOLDERID_System,
        .type       = CSIDL_Type_SystemPath,
        .category   = KF_CATEGORY_FIXED,
        .name       = L"System",
    },
    { /* 0x26 - CSIDL_PROGRAM_FILES */
        .id         = &FOLDERID_ProgramFiles,
        .type       = CSIDL_Type_CurrVer,
        .value      = L"ProgramFilesDir",
        .def_path   = L"Program Files",
        .category   = KF_CATEGORY_FIXED,
        .name       = L"ProgramFiles",
        .attributes = FILE_ATTRIBUTE_READONLY,
    },
    { /* 0x27 - CSIDL_MYPICTURES */
        .id         = &FOLDERID_Pictures,
        .type       = CSIDL_Type_User,
        .value      = L"My Pictures",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"My Pictures",
        .parent     = &FOLDERID_Profile,
        .path       = L"Pictures",
        .parsing    = L"::{59031a47-3f72-44a7-89c5-5595fe6b30ee}\\{33E28130-4E1E-4676-835A-98395C3BC3BB}",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_ROAMABLE | KFDF_PRECREATE,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_MY_PICTURES
#endif
    },
    { /* 0x28 - CSIDL_PROFILE */
        .id         = &FOLDERID_Profile,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_FIXED,
        .name       = L"Profile",
    },
    { /* 0x29 - CSIDL_SYSTEMX86 */
        .id         = &FOLDERID_SystemX86,
        .type       = CSIDL_Type_SystemX86Path,
        .category   = KF_CATEGORY_FIXED,
        .name       = L"SystemX86",
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_SYSTEM_GEAR
#endif
    },
    { /* 0x2a - CSIDL_PROGRAM_FILESX86 */
        .id         = &FOLDERID_ProgramFilesX86,
        .type       = CSIDL_Type_CurrVer,
        .value      = L"ProgramFilesDir (x86)",
        .def_path   = L"Program Files (x86)",
        .category   = KF_CATEGORY_FIXED,
        .name       = L"ProgramFilesX86",
        .attributes = FILE_ATTRIBUTE_READONLY,
    },
    { /* 0x2b - CSIDL_PROGRAM_FILES_COMMON */
        .id         = &FOLDERID_ProgramFilesCommon,
        .type       = CSIDL_Type_CurrVer,
        .value      = L"CommonFilesDir",
        .def_path   = L"Program Files\\Common Files",
        .category   = KF_CATEGORY_FIXED,
        .name       = L"ProgramFilesCommon",
    },
    { /* 0x2c - CSIDL_PROGRAM_FILES_COMMONX86 */
        .id         = &FOLDERID_ProgramFilesCommonX86,
        .type       = CSIDL_Type_CurrVer,
        .value      = L"CommonFilesDir (x86)",
        .def_path   = L"Program Files (x86)\\Common Files",
        .category   = KF_CATEGORY_FIXED,
        .name       = L"ProgramFilesCommonX86",
    },
    { /* 0x2d - CSIDL_COMMON_TEMPLATES */
        .id         = &FOLDERID_CommonTemplates,
        .type       = CSIDL_Type_ProgramData,
        .value      = L"Common Templates",
        .category   = KF_CATEGORY_COMMON,
        .name       = L"Common Templates",
        .parent     = &FOLDERID_ProgramData,
        .path       = L"Microsoft\\Windows\\Templates",
    },
    { /* 0x2e - CSIDL_COMMON_DOCUMENTS */
        .id         = &FOLDERID_PublicDocuments,
        .type       = CSIDL_Type_AllUsers,
        .value      = L"Common Documents",
        .category   = KF_CATEGORY_COMMON,
        .name       = L"Common Documents",
        .parent     = &FOLDERID_Public,
        .path       = L"Documents",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_MY_DOCUMENTS
#endif
    },
    { /* 0x2f - CSIDL_COMMON_ADMINTOOLS */
        .id         = &FOLDERID_CommonAdminTools,
        .type       = CSIDL_Type_ProgramData,
        .value      = L"Common Administrative Tools",
        .category   = KF_CATEGORY_COMMON,
        .name       = L"Common Administrative Tools",
        .parent     = &FOLDERID_CommonPrograms,
        .path       = L"Administrative Tools",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x30 - CSIDL_ADMINTOOLS */
        .id         = &FOLDERID_AdminTools,
        .type       = CSIDL_Type_User,
        .value      = L"Administrative Tools",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Administrative Tools",
        .parent     = &FOLDERID_Programs,
        .path       = L"Administrative Tools",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x31 - CSIDL_CONNECTIONS */
        .id         = &FOLDERID_ConnectionsFolder,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"ConnectionsFolder",
        .path       = L"Administrative Tools",
        .parsing    = L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}",
    },
    { /* 0x32 - unassigned */
        .id         = &GUID_NULL,
        .type       = CSIDL_Type_Disallowed,
    },
    { /* 0x33 - unassigned */
        .id         = &GUID_NULL,
        .type       = CSIDL_Type_Disallowed,
    },
    { /* 0x34 - unassigned */
        .id         = &GUID_NULL,
        .type       = CSIDL_Type_Disallowed,
    },
    { /* 0x35 - CSIDL_COMMON_MUSIC */
        .id         = &FOLDERID_PublicMusic,
        .type       = CSIDL_Type_AllUsers,
        .value      = L"CommonMusic",
        .category   = KF_CATEGORY_COMMON,
        .name       = L"CommonMusic",
        .parent     = &FOLDERID_Public,
        .path       = L"Music",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_MY_MUSIC
#endif
    },
    { /* 0x36 - CSIDL_COMMON_PICTURES */
        .id         = &FOLDERID_PublicPictures,
        .type       = CSIDL_Type_AllUsers,
        .value      = L"CommonPictures",
        .category   = KF_CATEGORY_COMMON,
        .name       = L"CommonPictures",
        .parent     = &FOLDERID_Public,
        .path       = L"Pictures",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_MY_PICTURES
#endif
    },
    { /* 0x37 - CSIDL_COMMON_VIDEO */
        .id         = &FOLDERID_PublicVideos,
        .type       = CSIDL_Type_AllUsers,
        .value      = L"CommonVideo",
        .category   = KF_CATEGORY_COMMON,
        .name       = L"CommonVideo",
        .parent     = &FOLDERID_Public,
        .path       = L"Videos",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
#ifdef __REACTOS__
        .nShell32IconIndex = -IDI_SHELL_MY_MOVIES
#endif
    },
    { /* 0x38 - CSIDL_RESOURCES */
        .id         = &FOLDERID_ResourceDir,
        .type       = CSIDL_Type_WindowsPath,
        .def_path   = L"Resources",
        .category   = KF_CATEGORY_FIXED,
        .name       = L"ResourceDir",
    },
    { /* 0x39 - CSIDL_RESOURCES_LOCALIZED */
        .id         = &FOLDERID_LocalizedResourcesDir,
        .type       = CSIDL_Type_NonExistent,
        .category   = KF_CATEGORY_FIXED,
        .name       = L"LocalizedResourcesDir",
    },
    { /* 0x3a - CSIDL_COMMON_OEM_LINKS */
        .id         = &FOLDERID_CommonOEMLinks,
        .type       = CSIDL_Type_ProgramData,
        .category   = KF_CATEGORY_COMMON,
        .name       = L"OEM Links",
        .parent     = &FOLDERID_ProgramData,
        .path       = L"OEM Links",
    },
    { /* 0x3b - CSIDL_CDBURN_AREA */
        .id         = &FOLDERID_CDBurning,
        .type       = CSIDL_Type_User,
        .value      = L"CD Burning",
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"CD Burning",
        .parent     = &FOLDERID_LocalAppData,
        .path       = L"Microsoft\\Windows\\Burn\\Burn",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_LOCAL_REDIRECT_ONLY,
    },
    { /* 0x3c unassigned */
        .id         = &GUID_NULL,
        .type       = CSIDL_Type_Disallowed,
    },
    { /* 0x3d - CSIDL_COMPUTERSNEARME */
        .id         = &GUID_NULL,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
    },
    { /* 0x3e - CSIDL_PROFILES */
        .id         = &GUID_NULL,
        .type       = CSIDL_Type_Disallowed, /* oddly, this matches WinXP */
    },
    { /* 0x3f */
        .id         = &FOLDERID_AddNewPrograms,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"AddNewProgramsFolder",
        .parsing    = L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{15eae92e-f17a-4431-9f28-805e482dafd4}",
    },
    { /* 0x40 */
        .id         = &FOLDERID_AppUpdates,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"AppUpdatesFolder",
        .parsing    = L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7b81be6a-ce2b-4676-a29e-eb907a5126c5}\\::{d450a8a1-9568-45c7-9c0e-b4f9fb4537bd}",
    },
    { /* 0x41 */
        .id         = &FOLDERID_ChangeRemovePrograms,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"ChangeRemoveProgramsFolder",
        .parsing    = L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7b81be6a-ce2b-4676-a29e-eb907a5126c5}",
    },
    { /* 0x42 */
        .id         = &FOLDERID_ConflictFolder,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"ConflictFolder",
        .parsing    = L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{9C73F5E5-7AE7-4E32-A8E8-8D23B85255BF}\\::{E413D040-6788-4C22-957E-175D1C513A34},",
    },
    { /* 0x43 - CSIDL_CONTACTS */
        .id         = &FOLDERID_Contacts,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Contacts",
        .parent     = &FOLDERID_Profile,
        .path       = L"Contacts",
        .parsing    = L"::{59031a47-3f72-44a7-89c5-5595fe6b30ee}\\{56784854-C6CB-462B-8169-88E350ACB882}",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH,
    },
    { /* 0x44 */
        .id         = &FOLDERID_DeviceMetadataStore,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_COMMON,
        .name       = L"Device Metadata Store",
        .parent     = &FOLDERID_ProgramData,
        .path       = L"Microsoft\\Windows\\DeviceMetadataStore",
    },
    { /* 0x45 */
        .id         = &GUID_NULL,
        .type       = CSIDL_Type_Disallowed,
    },
    { /* 0x46 */
        .id         = &FOLDERID_DocumentsLibrary,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"DocumentsLibrary",
        .parent     = &FOLDERID_Libraries,
        .path       = L"Documents.library-ms",
        .parsing    = L"::{031E4825-7B94-4dc3-B131-E946B44C8DD5}\\{7b0db17d-9cd2-4a93-9733-46cc89022e7c}",
        .flags      = KFDF_PRECREATE | KFDF_STREAM,
    },
    { /* 0x47 - CSIDL_DOWNLOADS */
        .id         = &FOLDERID_Downloads,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Downloads",
        .parent     = &FOLDERID_Profile,
        .path       = L"Downloads",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH,
    },
    { /* 0x48 */
        .id         = &FOLDERID_Games,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"Games",
        .parsing    = L"::{ED228FDF-9EA8-4870-83b1-96b02CFE0D52}",
    },
    { /* 0x49 */
        .id         = &FOLDERID_GameTasks,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"GameTasks",
        .parent     = &FOLDERID_LocalAppData,
        .path       = L"Microsoft\\Windows\\GameExplorer",
        .flags      = KFDF_LOCAL_REDIRECT_ONLY,
    },
    { /* 0x4a */
        .id         = &FOLDERID_HomeGroup,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"HomeGroupFolder",
        .parsing    = L"::{B4FB3F98-C1EA-428d-A78A-D1F5659CBA93}",
    },
    { /* 0x4b */
        .id         = &FOLDERID_ImplicitAppShortcuts,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"ImplicitAppShortcuts",
        .parent     = &FOLDERID_UserPinned,
        .path       = L"ImplicitAppShortcuts",
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x4c */
        .id         = &FOLDERID_Libraries,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Libraries",
        .parent     = &FOLDERID_RoamingAppData,
        .path       = L"Microsoft\\Windows\\Libraries",
        .flags      = KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH,
    },
    { /* 0x4d - CSIDL_LINKS */
        .id         = &FOLDERID_Links,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Links",
        .parent     = &FOLDERID_Profile,
        .path       = L"Links",
        .parsing    = L"::{59031a47-3f72-44a7-89c5-5595fe6b30ee}\\{bfb9d5e0-c6a9-404c-b2b2-ae6db6af4968}",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH,
    },
    { /* 0x4e - CSIDL_APPDATA_LOCALLOW */
        .id         = &FOLDERID_LocalAppDataLow,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"LocalAppDataLow",
        .parent     = &FOLDERID_Profile,
        .path       = L"AppData\\LocalLow",
        .attributes = FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
        .flags      = KFDF_LOCAL_REDIRECT_ONLY | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH,
    },
    { /* 0x4f */
        .id         = &FOLDERID_MusicLibrary,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"MusicLibrary",
        .parent     = &FOLDERID_Libraries,
        .path       = L"Music.library-ms",
        .parsing    = L"::{031E4825-7B94-4dc3-B131-E946B44C8DD5}\\{2112AB0A-C86A-4ffe-A368-0DE96E47012E}",
        .flags      = KFDF_PRECREATE | KFDF_STREAM,
    },
    { /* 0x50 */
        .id         = &FOLDERID_OriginalImages,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Original Images",
        .parent     = &FOLDERID_LocalAppData,
        .path       = L"Microsoft\\Windows Photo Gallery\\Original Images",
    },
    { /* 0x51 */
        .id         = &FOLDERID_PhotoAlbums,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"PhotoAlbums",
        .parent     = &FOLDERID_Pictures,
        .path       = L"Slide Shows",
        .attributes = FILE_ATTRIBUTE_READONLY,
    },
    { /* 0x52 */
        .id         = &FOLDERID_PicturesLibrary,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"PicturesLibrary",
        .parent     = &FOLDERID_Libraries,
        .path       = L"Pictures.library-ms",
        .parsing    = L"::{031E4825-7B94-4dc3-B131-E946B44C8DD5}\\{A990AE9F-A03B-4e80-94BC-9912D7504104}",
        .flags      = KFDF_PRECREATE | KFDF_STREAM,
    },
    { /* 0x53 */
        .id         = &FOLDERID_Playlists,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Playlists",
        .parent     = &FOLDERID_Music,
        .path       = L"Playlists",
        .attributes = FILE_ATTRIBUTE_READONLY,
    },
    { /* 0x54 */
        .id         = &FOLDERID_ProgramFilesX64,
#ifdef _WIN64
        .type       = CSIDL_Type_CurrVer,
        .value      = L"ProgramFilesDir",
        .def_path   = L"Program Files",
#else
        .type       = CSIDL_Type_NonExistent,
#endif
        .category   = KF_CATEGORY_FIXED,
        .name       = L"ProgramFilesX64",
    },
    { /* 0x55 */
        .id         = &FOLDERID_ProgramFilesCommonX64,
#ifdef _WIN64
        .type       = CSIDL_Type_CurrVer,
        .value      = L"ProgramFilesCommonX64",
        .def_path   = L"Program Files\\Common Files",
#else
        .type       = CSIDL_Type_NonExistent,
#endif
        .category   = KF_CATEGORY_FIXED,
        .name       = L"ProgramFilesCommonX64",
    },
    { /* 0x56 */
        .id         = &FOLDERID_Public,
        .type       = CSIDL_Type_AllUsers,
        .category   = KF_CATEGORY_FIXED,
        .name       = L"Public",
        .parsing    = L"::{4336a54d-038b-4685-ab02-99bb52d3fb8b}",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x57 */
        .id         = &FOLDERID_PublicDownloads,
        .type       = CSIDL_Type_AllUsers,
        .category   = KF_CATEGORY_COMMON,
        .name       = L"CommonDownloads",
        .parent     = &FOLDERID_Public,
        .path       = L"Downloads",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x58 */
        .id         = &FOLDERID_PublicGameTasks,
        .type       = CSIDL_Type_ProgramData,
        .category   = KF_CATEGORY_COMMON,
        .name       = L"PublicGameTasks",
        .parent     = &FOLDERID_ProgramData,
        .path       = L"Microsoft\\Windows\\GameExplorer",
        .flags      = KFDF_LOCAL_REDIRECT_ONLY,
    },
    { /* 0x59 */
        .id         = &FOLDERID_PublicLibraries,
        .type       = CSIDL_Type_AllUsers,
        .category   = KF_CATEGORY_COMMON,
        .name       = L"PublicLibraries",
        .parent     = &FOLDERID_Public,
        .path       = L"Libraries",
        .attributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x5a */
        .id         = &FOLDERID_PublicRingtones,
        .type       = CSIDL_Type_ProgramData,
        .category   = KF_CATEGORY_COMMON,
        .name       = L"CommonRingtones",
        .parent     = &FOLDERID_ProgramData,
        .path       = L"Microsoft\\Windows\\Ringtones",
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x5b */
        .id         = &FOLDERID_QuickLaunch,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Quick Launch",
        .parent     = &FOLDERID_RoamingAppData,
        .path       = L"Microsoft\\Internet Explorer\\Quick Launch",
    },
    { /* 0x5c */
        .id         = &FOLDERID_RecordedTVLibrary,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_COMMON,
        .name       = L"RecordedTVLibrary",
        .parent     = &FOLDERID_PublicLibraries,
        .path       = L"RecordedTV.library-ms",
        .flags      = KFDF_PRECREATE | KFDF_STREAM,
    },
    { /* 0x5d */
        .id         = &FOLDERID_Ringtones,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Ringtones",
        .parent     = &FOLDERID_LocalAppData,
        .path       = L"Microsoft\\Windows\\Ringtones",
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x5e */
        .id         = &FOLDERID_SampleMusic,
        .type       = CSIDL_Type_AllUsers,
        .category   = KF_CATEGORY_COMMON,
        .name       = L"SampleMusic",
        .parent     = &FOLDERID_PublicMusic,
        .path       = L"Sample Music",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x5f */
        .id         = &FOLDERID_SamplePictures,
        .type       = CSIDL_Type_AllUsers,
        .category   = KF_CATEGORY_COMMON,
        .name       = L"SamplePictures",
        .parent     = &FOLDERID_PublicPictures,
        .path       = L"Sample Pictures",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x60 */
        .id         = &FOLDERID_SamplePlaylists,
        .type       = CSIDL_Type_AllUsers,
        .category   = KF_CATEGORY_COMMON,
        .name       = L"SamplePlaylists",
        .parent     = &FOLDERID_PublicMusic,
        .path       = L"Sample Playlists",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x61 */
        .id         = &FOLDERID_SampleVideos,
        .type       = CSIDL_Type_AllUsers,
        .category   = KF_CATEGORY_COMMON,
        .name       = L"SampleVideos",
        .parent     = &FOLDERID_PublicVideos,
        .path       = L"Sample Videos",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x62 - CSIDL_SAVED_GAMES */
        .id         = &FOLDERID_SavedGames,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"SavedGames",
        .parent     = &FOLDERID_Profile,
        .path       = L"Saved Games",
        .parsing    = L"::{59031a47-3f72-44a7-89c5-5595fe6b30ee}\\{4C5C32FF-BB9D-43b0-B5B4-2D72E54EAAA4}",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH,
    },
    { /* 0x63 - CSIDL_SEARCHES */
        .id         = &FOLDERID_SavedSearches,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Searches",
        .parent     = &FOLDERID_Profile,
        .path       = L"Searches",
        .parsing    = L"::{59031a47-3f72-44a7-89c5-5595fe6b30ee}\\{7d1d3a04-debb-4115-95cf-2f29da2920da}",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH,
    },
    { /* 0x64 */
        .id         = &FOLDERID_SEARCH_CSC,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"CSCFolder",
        .parsing    = L"shell:::{BD7A2E7B-21CB-41b2-A086-B309680C6B7E}\\*",
    },
    { /* 0x65 */
        .id         = &FOLDERID_SEARCH_MAPI,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"MAPIFolder",
        .parsing    = L"shell:::{89D83576-6BD1-4C86-9454-BEB04E94C819}\\*",
    },
    { /* 0x66 */
        .id         = &FOLDERID_SearchHome,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"SearchHomeFolder",
        .parsing    = L"::{9343812e-1c37-4a49-a12e-4b2d810d956b}",
    },
    { /* 0x67 */
        .id         = &FOLDERID_SidebarDefaultParts,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_COMMON,
        .name       = L"Default Gadgets",
        .parent     = &FOLDERID_ProgramFiles,
        .path       = L"Windows Sidebar\\Gadgets",
    },
    { /* 0x68 */
        .id         = &FOLDERID_SidebarParts,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Gadgets",
        .parent     = &FOLDERID_LocalAppData,
        .path       = L"Microsoft\\Windows Sidebar\\Gadgets",
    },
    { /* 0x69 */
        .id         = &FOLDERID_SyncManagerFolder,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"SyncCenterFolder",
        .parsing    = L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{9C73F5E5-7AE7-4E32-A8E8-8D23B85255BF}",
    },
    { /* 0x6a */
        .id         = &FOLDERID_SyncResultsFolder,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"SyncResultsFolder",
        .parsing    = L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{9C73F5E5-7AE7-4E32-A8E8-8D23B85255BF}\\::{BC48B32F-5910-47F5-8570-5074A8A5636A},",
    },
    { /* 0x6b */
        .id         = &FOLDERID_SyncSetupFolder,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"SyncSetupFolder",
        .parsing    = L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{9C73F5E5-7AE7-4E32-A8E8-8D23B85255BF}\\::{F1390A9A-A3F4-4E5D-9C5F-98F3BD8D935C},",
    },
    { /* 0x6c */
        .id         = &FOLDERID_UserPinned,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"User Pinned",
        .parent     = &FOLDERID_QuickLaunch,
        .path       = L"User Pinned",
        .attributes = FILE_ATTRIBUTE_HIDDEN,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x6d */
        .id         = &FOLDERID_UserProfiles,
        .type       = CSIDL_Type_CurrVer,
        .value      = L"Users",
        .def_path   = L"Users",
        .category   = KF_CATEGORY_FIXED,
        .name       = L"UserProfiles",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE,
    },
    { /* 0x6e */
        .id         = &FOLDERID_UserProgramFiles,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"UserProgramFiles",
        .parent     = &FOLDERID_LocalAppData,
        .path       = L"Programs",
    },
    { /* 0x6f */
        .id         = &FOLDERID_UserProgramFilesCommon,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"UserProgramFilesCommon",
        .parent     = &FOLDERID_UserProgramFiles,
        .path       = L"Common",
    },
    { /* 0x70 */
        .id         = &FOLDERID_UsersFiles,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"UsersFilesFolder",
        .parsing    = L"::{59031a47-3f72-44a7-89c5-5595fe6b30ee}",
    },
    { /* 0x71 */
        .id         = &FOLDERID_UsersLibraries,
        .type       = CSIDL_Type_Disallowed,
        .category   = KF_CATEGORY_VIRTUAL,
        .name       = L"UsersLibrariesFolder",
        .parsing    = L"::{031E4825-7B94-4dc3-B131-E946B44C8DD5}",
    },
    { /* 0x72 */
        .id         = &FOLDERID_VideosLibrary,
        .type       = CSIDL_Type_Disallowed, /* FIXME */
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"VideosLibrary",
        .path       = L"Videos.library-ms",
        .parsing    = L"::{031E4825-7B94-4dc3-B131-E946B44C8DD5}\\{491E922F-5643-4af4-A7EB-4E7A138D8174}",
    },
    { /* 0x73 */
        .id         = &FOLDERID_AccountPictures,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"AccountPictures",
        .parent     = &FOLDERID_RoamingAppData,
        .path       = L"Microsoft\\Windows\\AccountPictures",
        .attributes = FILE_ATTRIBUTE_READONLY,
        .flags      = KFDF_PRECREATE | KFDF_ROAMABLE,
    },
    { /* 0x74 */
        .id         = &FOLDERID_Screenshots,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"Screenshots",
        .parent     = &FOLDERID_Pictures,
        .path       = L"Screenshots",
        .flags      = KFDF_PRECREATE | KFDF_ROAMABLE,
    },
    { /* 0x75 */
        .id         = &FOLDERID_AppDataDocuments,
        .type       = CSIDL_Type_User,
        .category   = KF_CATEGORY_PERUSER,
        .name       = L"AppDataDocuments",
        .parent     = &FOLDERID_LocalAppData,
        .path       = L"Documents",
        .flags      = KFDF_PRECREATE | KFDF_ROAMABLE,
    },
};

static int csidl_from_id( const KNOWNFOLDERID *id )
{
    int i;
    for (i = 0; i < ARRAY_SIZE(CSIDL_Data); i++)
        if (IsEqualGUID( CSIDL_Data[i].id, id )) return i;
    return -1;
}

#ifdef __REACTOS__
INT SHGetSpecialFolderID(_In_ LPCWSTR pszName)
{
    UINT csidl;

    for (csidl = 0; csidl < _countof(CSIDL_Data); ++csidl)
    {
        const CSIDL_DATA *pData = &CSIDL_Data[csidl];
        if (pData->name && lstrcmpiW(pszName, pData->name) == 0)
            return csidl;
    }

    return -1;
}

INT Shell_ParseSpecialFolder(_In_ LPCWSTR pszStart, _Out_ LPWSTR *ppch, _Out_ INT *pcch)
{
    LPCWSTR pszPath, pchBackslash;
    WCHAR szPath[MAX_PATH];

    pchBackslash = StrChrW(pszStart, L'\\');
    if (pchBackslash)
    {
        *ppch = (LPWSTR)(pchBackslash + 1);
        *pcch = (pchBackslash - pszStart) + 1;
        StrCpyNW(szPath, pszStart, max(*pcch, _countof(szPath)));
        pszPath = szPath;
    }
    else
    {
        *ppch = NULL;
        *pcch = lstrlenW(pszStart);
        pszPath = pszStart;
    }

    return SHGetSpecialFolderID(pszPath);
}
#endif

#ifndef __REACTOS__
static HRESULT _SHExpandEnvironmentStrings(LPCWSTR szSrc, LPWSTR szDest);
#else
static HRESULT _SHExpandEnvironmentStrings(HANDLE hToken, LPCWSTR szSrc, LPWSTR szDest, DWORD cchDest);
#endif

/* Gets the value named value from the registry key
 * rootKey\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders
 * (or from rootKey\userPrefix\... if userPrefix is not NULL) into path, which
 * is assumed to be MAX_PATH WCHARs in length.
 * If it exists, expands the value and writes the expanded value to
 * rootKey\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders
 * Returns successful error code if the value was retrieved from the registry,
 * and a failure otherwise.
 */
#ifndef __REACTOS__
static HRESULT _SHGetUserShellFolderPath(HKEY rootKey, LPCWSTR userPrefix,
#else
static HRESULT _SHGetUserShellFolderPath(HKEY rootKey, HANDLE hToken, LPCWSTR userPrefix,
#endif
 LPCWSTR value, LPWSTR path)
{
    HRESULT hr;
    WCHAR shellFolderPath[MAX_PATH], userShellFolderPath[MAX_PATH];
    LPCWSTR pShellFolderPath, pUserShellFolderPath;
    HKEY userShellFolderKey, shellFolderKey;
    DWORD dwType, dwPathLen;

    TRACE("%p,%s,%s,%p\n",rootKey, debugstr_w(userPrefix), debugstr_w(value),
     path);

    if (userPrefix)
    {
        lstrcpyW(shellFolderPath, userPrefix);
        PathAddBackslashW(shellFolderPath);
        lstrcatW(shellFolderPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
        pShellFolderPath = shellFolderPath;
        lstrcpyW(userShellFolderPath, userPrefix);
        PathAddBackslashW(userShellFolderPath);
        lstrcatW(userShellFolderPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders");
        pUserShellFolderPath = userShellFolderPath;
    }
    else
    {
        pUserShellFolderPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";
        pShellFolderPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
    }

    if (RegCreateKeyW(rootKey, pShellFolderPath, &shellFolderKey))
    {
        TRACE("Failed to create %s\n", debugstr_w(pShellFolderPath));
        return E_FAIL;
    }
    if (RegCreateKeyW(rootKey, pUserShellFolderPath, &userShellFolderKey))
    {
        TRACE("Failed to create %s\n",
         debugstr_w(pUserShellFolderPath));
        RegCloseKey(shellFolderKey);
        return E_FAIL;
    }

    dwPathLen = MAX_PATH * sizeof(WCHAR);
    if (!RegQueryValueExW(userShellFolderKey, value, NULL, &dwType,
     (LPBYTE)path, &dwPathLen) && (dwType == REG_EXPAND_SZ || dwType == REG_SZ))
    {
        LONG ret;

        path[dwPathLen / sizeof(WCHAR)] = '\0';
        if (dwType == REG_EXPAND_SZ && path[0] == '%')
        {
            WCHAR szTemp[MAX_PATH];

#ifndef __REACTOS__
            _SHExpandEnvironmentStrings(path, szTemp);
#else
            hr = _SHExpandEnvironmentStrings(hToken, path, szTemp, _countof(szTemp));
            if (FAILED(hr))
                goto end;
#endif
            lstrcpynW(path, szTemp, MAX_PATH);
        }
        ret = RegSetValueExW(shellFolderKey, value, 0, REG_SZ, (LPBYTE)path,
         (lstrlenW(path) + 1) * sizeof(WCHAR));
        if (ret != ERROR_SUCCESS)
            hr = HRESULT_FROM_WIN32(ret);
        else
            hr = S_OK;
    }
    else
        hr = E_FAIL;
#ifdef __REACTOS__
end:
#endif
    RegCloseKey(shellFolderKey);
    RegCloseKey(userShellFolderKey);
    TRACE("returning 0x%08lx\n", hr);
    return hr;
}

static void append_relative_path(BYTE folder, WCHAR *pszPath)
{
    if (CSIDL_Data[folder].path)
    {
        PathAddBackslashW(pszPath);
        lstrcatW(pszPath, CSIDL_Data[folder].path);
    }
    else if (CSIDL_Data[folder].def_path)
    {
        PathAddBackslashW(pszPath);
        lstrcatW(pszPath, CSIDL_Data[folder].def_path);
    }
}

/* Gets a 'semi-expanded' default value of the CSIDL with index folder into
 * pszPath, based on the entries in CSIDL_Data.  By semi-expanded, I mean:
 * - Depending on the entry's type, the path may begin with an (unexpanded)
 *   environment variable name.  The caller is responsible for expanding
 *   environment strings if so desired.
 *   The types that are prepended with environment variables are:
 *   CSIDL_Type_User:     %USERPROFILE%
 *   CSIDL_Type_AllUsers: %ALLUSERSPROFILE%
 *   CSIDL_Type_CurrVer:  %SystemDrive%
 *   (Others might make sense too, but as yet are unneeded.)
 */
#ifndef __REACTOS__
static HRESULT _SHGetDefaultValue(BYTE folder, LPWSTR pszPath)
#else
static HRESULT _SHGetDefaultValue(HANDLE hToken, BYTE folder, LPWSTR pszPath)
#endif
{
    HRESULT hr;

    TRACE("0x%02x,%p\n", folder, pszPath);

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;

    if (!pszPath)
        return E_INVALIDARG;

#ifdef __REACTOS__
    if (hToken != NULL && hToken != (HANDLE)-1)
    {
        FIXME("unsupported for user other than current or default\n");
    }
#endif

    if (!is_win64)
    {
        BOOL is_wow64;

        switch (folder)
        {
        case CSIDL_PROGRAM_FILES:
        case CSIDL_PROGRAM_FILESX86:
            IsWow64Process( GetCurrentProcess(), &is_wow64 );
            folder = is_wow64 ? CSIDL_PROGRAM_FILESX86 : CSIDL_PROGRAM_FILES;
            break;
        case CSIDL_PROGRAM_FILES_COMMON:
        case CSIDL_PROGRAM_FILES_COMMONX86:
            IsWow64Process( GetCurrentProcess(), &is_wow64 );
            folder = is_wow64 ? CSIDL_PROGRAM_FILES_COMMONX86 : CSIDL_PROGRAM_FILES_COMMON;
            break;
        }
    }

    if (!CSIDL_Data[folder].parent)
    {
        /* hit the root, sub in env var */
        switch (CSIDL_Data[folder].type)
        {
            case CSIDL_Type_User:
                lstrcpyW(pszPath, L"%USERPROFILE%");
                break;
            case CSIDL_Type_AllUsers:
                lstrcpyW(pszPath, L"%PUBLIC%");
                break;
            case CSIDL_Type_ProgramData:
                lstrcpyW(pszPath, L"%ProgramData%");
                break;
            case CSIDL_Type_CurrVer:
                lstrcpyW(pszPath, L"%SystemDrive%");
                break;
            default:
                ; /* no corresponding env. var, do nothing */
        }
        hr = S_OK;
    }else{
        /* prepend with parent */
#ifdef __REACTOS__
        hr = _SHGetDefaultValue(NULL, csidl_from_id(CSIDL_Data[folder].parent), pszPath);
#else
        hr = _SHGetDefaultValue(csidl_from_id(CSIDL_Data[folder].parent), pszPath);
#endif
    }

    if (SUCCEEDED(hr))
        append_relative_path(folder, pszPath);

    TRACE("returning 0x%08lx\n", hr);
    return hr;
}

/* Gets the (unexpanded) value of the folder with index folder into pszPath.
 * The folder's type is assumed to be CSIDL_Type_CurrVer.  Its default value
 * can be overridden in the HKLM\\Software\\Microsoft\\Windows\\CurrentVersion key.
 * If dwFlags has SHGFP_TYPE_DEFAULT set or if the value isn't overridden in
 * the registry, uses _SHGetDefaultValue to get the value.
 */
static HRESULT _SHGetCurrentVersionPath(DWORD dwFlags, BYTE folder,
 LPWSTR pszPath)
{
    HRESULT hr;

    TRACE("0x%08lx,0x%02x,%p\n", dwFlags, folder, pszPath);

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
    if (CSIDL_Data[folder].type != CSIDL_Type_CurrVer)
        return E_INVALIDARG;
    if (!pszPath)
        return E_INVALIDARG;

    if (dwFlags & SHGFP_TYPE_DEFAULT)
#ifndef __REACTOS__
        hr = _SHGetDefaultValue(folder, pszPath);
#else
        hr = _SHGetDefaultValue(NULL, folder, pszPath);
#endif
    else
    {
        HKEY hKey;

        if (RegCreateKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion", &hKey))
            hr = E_FAIL;
        else
        {
            DWORD dwType, dwPathLen = MAX_PATH * sizeof(WCHAR);

            if (RegQueryValueExW(hKey, CSIDL_Data[folder].value, NULL,
             &dwType, (LPBYTE)pszPath, &dwPathLen) ||
             (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
            {
#ifndef __REACTOS__
                hr = _SHGetDefaultValue(folder, pszPath);
#else
                hr = _SHGetDefaultValue(NULL, folder, pszPath);
#endif
                dwType = REG_EXPAND_SZ;
                switch (folder)
                {
                case CSIDL_PROGRAM_FILESX86:
                case CSIDL_PROGRAM_FILES_COMMONX86:
                    /* these two should never be set on 32-bit setups */
                    if (!is_win64)
                    {
                        BOOL is_wow64;
                        IsWow64Process( GetCurrentProcess(), &is_wow64 );
                        if (!is_wow64) break;
                    }
                    /* fall through */
                default:
                    RegSetValueExW(hKey, CSIDL_Data[folder].value, 0, dwType,
                                   (LPBYTE)pszPath, (lstrlenW(pszPath)+1)*sizeof(WCHAR));
                }
            }
            else
            {
                pszPath[dwPathLen / sizeof(WCHAR)] = '\0';
                hr = S_OK;
            }
            RegCloseKey(hKey);
        }
    }
    TRACE("returning 0x%08lx (output path is %s)\n", hr, debugstr_w(pszPath));
    return hr;
}

static LPWSTR _GetUserSidStringFromToken(HANDLE Token)
{
    char InfoBuffer[64];
    PTOKEN_USER UserInfo;
    DWORD InfoSize;
    LPWSTR SidStr;

    UserInfo = (PTOKEN_USER) InfoBuffer;
    if (! GetTokenInformation(Token, TokenUser, InfoBuffer, sizeof(InfoBuffer),
                              &InfoSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return NULL;
        UserInfo = malloc(InfoSize);
        if (UserInfo == NULL)
            return NULL;
        if (! GetTokenInformation(Token, TokenUser, UserInfo, InfoSize,
                                  &InfoSize))
        {
            free(UserInfo);
            return NULL;
        }
    }

    if (! ConvertSidToStringSidW(UserInfo->User.Sid, &SidStr))
        SidStr = NULL;

    if (UserInfo != (PTOKEN_USER) InfoBuffer)
        free(UserInfo);

    return SidStr;
}

/* Gets the user's path (unexpanded) for the CSIDL with index folder:
 * If SHGFP_TYPE_DEFAULT is set, calls _SHGetDefaultValue for it.  Otherwise
 * calls _SHGetUserShellFolderPath for it.  Where it looks depends on hToken:
 * - if hToken is -1, looks in HKEY_USERS\.Default
 * - otherwise looks first in HKEY_CURRENT_USER, followed by HKEY_LOCAL_MACHINE
 *   if HKEY_CURRENT_USER doesn't contain any entries.  If both fail, finally
 *   calls _SHGetDefaultValue for it.
 */
static HRESULT _SHGetUserProfilePath(HANDLE hToken, DWORD dwFlags, BYTE folder,
 LPWSTR pszPath)
{
    const WCHAR *szValueName;
    WCHAR buffer[40];
    HRESULT hr;

    TRACE("%p,0x%08lx,0x%02x,%p\n", hToken, dwFlags, folder, pszPath);

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
    if (CSIDL_Data[folder].type != CSIDL_Type_User)
        return E_INVALIDARG;
    if (!pszPath)
        return E_INVALIDARG;

    if (dwFlags & SHGFP_TYPE_DEFAULT)
    {
#ifndef __REACTOS__
        if (hToken != NULL && hToken != (HANDLE)-1)
        {
            FIXME("unsupported for user other than current or default\n");
            return E_FAIL;
        }
        hr = _SHGetDefaultValue(folder, pszPath);
#else
        hr = _SHGetDefaultValue(hToken, folder, pszPath);
#endif
    }
    else
    {
        static const WCHAR DefaultW[] = L".Default";
        LPCWSTR userPrefix = NULL;
        HKEY hRootKey;

        if (hToken == (HANDLE)-1)
        {
            hRootKey = HKEY_USERS;
            userPrefix = DefaultW;
        }
        else if (hToken == NULL)
            hRootKey = HKEY_CURRENT_USER;
        else
        {
            hRootKey = HKEY_USERS;
            userPrefix = _GetUserSidStringFromToken(hToken);
            if (userPrefix == NULL)
            {
                hr = E_FAIL;
                goto error;
            }
        }

        /* For CSIDL_Type_User we also use the GUID if no szValueName is provided */
        szValueName = CSIDL_Data[folder].value;
        if (!szValueName)
        {
            StringFromGUID2( CSIDL_Data[folder].id, buffer, 39 );
            szValueName = &buffer[0];
        }

#ifndef __REACTOS__
        hr = _SHGetUserShellFolderPath(hRootKey, userPrefix, szValueName, pszPath);
        if (FAILED(hr) && hRootKey != HKEY_LOCAL_MACHINE)
            hr = _SHGetUserShellFolderPath(HKEY_LOCAL_MACHINE, NULL, szValueName, pszPath);
        if (FAILED(hr))
            hr = _SHGetDefaultValue(folder, pszPath);
#else
        hr = _SHGetUserShellFolderPath(hRootKey, hToken, userPrefix, szValueName, pszPath);
        if (FAILED(hr) && hRootKey != HKEY_LOCAL_MACHINE)
            hr = _SHGetUserShellFolderPath(HKEY_LOCAL_MACHINE, hToken, NULL, szValueName, pszPath);
        if (FAILED(hr))
            hr = _SHGetDefaultValue(hToken, folder, pszPath);
#endif
        if (userPrefix != NULL && userPrefix != DefaultW)
            LocalFree((HLOCAL) userPrefix);
    }
error:
    TRACE("returning 0x%08lx (output path is %s)\n", hr, debugstr_w(pszPath));
    return hr;
}

/* Gets the (unexpanded) path for the CSIDL with index folder.  If dwFlags has
 * SHGFP_TYPE_DEFAULT set, calls _SHGetDefaultValue.  Otherwise calls
 * _SHGetUserShellFolderPath for it, looking only in HKEY_LOCAL_MACHINE.
 * If this fails, falls back to _SHGetDefaultValue.
 */
static HRESULT _SHGetAllUsersProfilePath(DWORD dwFlags, BYTE folder,
 LPWSTR pszPath)
{
    HRESULT hr;

    TRACE("0x%08lx,0x%02x,%p\n", dwFlags, folder, pszPath);

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
    if (CSIDL_Data[folder].type != CSIDL_Type_AllUsers && CSIDL_Data[folder].type != CSIDL_Type_ProgramData)
        return E_INVALIDARG;
    if (!pszPath)
        return E_INVALIDARG;

    if (dwFlags & SHGFP_TYPE_DEFAULT)
#ifndef __REACTOS__
        hr = _SHGetDefaultValue(folder, pszPath);
#else
        hr = _SHGetDefaultValue(NULL, folder, pszPath);
#endif
    else
    {
#ifndef __REACTOS__
        hr = _SHGetUserShellFolderPath(HKEY_LOCAL_MACHINE, NULL,
#else
        hr = _SHGetUserShellFolderPath(HKEY_LOCAL_MACHINE, NULL, NULL,
#endif
         CSIDL_Data[folder].value, pszPath);
        if (FAILED(hr))
#ifndef __REACTOS__
            hr = _SHGetDefaultValue(folder, pszPath);
#else
            hr = _SHGetDefaultValue(NULL, folder, pszPath);
#endif
    }
    TRACE("returning 0x%08lx (output path is %s)\n", hr, debugstr_w(pszPath));
    return hr;
}

static HRESULT _SHOpenProfilesKey(PHKEY pKey)
{
    LONG lRet;
    DWORD disp;

    lRet = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList", 0, NULL, 0,
     KEY_ALL_ACCESS, NULL, pKey, &disp);
    return HRESULT_FROM_WIN32(lRet);
}

/* Reads the value named szValueName from the key profilesKey (assumed to be
 * opened by _SHOpenProfilesKey) into szValue, which is assumed to be MAX_PATH
 * WCHARs in length.  If it doesn't exist, returns szDefault (and saves
 * szDefault to the registry).
 */
static HRESULT _SHGetProfilesValue(HKEY profilesKey, LPCWSTR szValueName,
 LPWSTR szValue, LPCWSTR szDefault)
{
    HRESULT hr;
    DWORD type, dwPathLen = MAX_PATH * sizeof(WCHAR);
    LONG lRet;

    TRACE("%p,%s,%p,%s\n", profilesKey, debugstr_w(szValueName), szValue,
     debugstr_w(szDefault));
    lRet = RegQueryValueExW(profilesKey, szValueName, NULL, &type,
     (LPBYTE)szValue, &dwPathLen);
    if (!lRet && (type == REG_SZ || type == REG_EXPAND_SZ) && dwPathLen
     && *szValue)
    {
        dwPathLen /= sizeof(WCHAR);
        szValue[dwPathLen] = '\0';
        hr = S_OK;
    }
    else
    {
        /* Missing or invalid value, set a default */
        lstrcpynW(szValue, szDefault, MAX_PATH);
        TRACE("Setting missing value %s to %s\n", debugstr_w(szValueName),
                                                  debugstr_w(szValue));
        lRet = RegSetValueExW(profilesKey, szValueName, 0, REG_EXPAND_SZ,
                              (LPBYTE)szValue,
                              (lstrlenW(szValue) + 1) * sizeof(WCHAR));
        if (lRet)
            hr = HRESULT_FROM_WIN32(lRet);
        else
            hr = S_OK;
    }
    TRACE("returning 0x%08lx (output value is %s)\n", hr, debugstr_w(szValue));
    return hr;
}

/* Attempts to expand environment variables from szSrc into szDest, which is
 * assumed to be MAX_PATH characters in length.  Before referring to the
 * environment, handles a few variables directly, because the environment
 * variables may not be set when this is called (as during Wine's installation
 * when default values are being written to the registry).
 * The directly handled environment variables, and their source, are:
 * - ALLUSERSPROFILE, USERPROFILE: reads from the registry
 * - SystemDrive: uses GetSystemDirectoryW and uses the drive portion of its
 *   path
 * If one of the directly handled environment variables is expanded, only
 * expands a single variable, and only in the beginning of szSrc.
 */
#ifndef __REACTOS__
static HRESULT _SHExpandEnvironmentStrings(LPCWSTR szSrc, LPWSTR szDest)
#else
static HRESULT _SHExpandEnvironmentStrings(HANDLE hToken, LPCWSTR szSrc, LPWSTR szDest, DWORD cchDest)
#endif
{
    HRESULT hr;
    WCHAR szTemp[MAX_PATH], szProfilesPrefix[MAX_PATH] = { 0 };
    HKEY key = NULL;

    TRACE("%s, %p\n", debugstr_w(szSrc), szDest);

    if (!szSrc || !szDest) return E_INVALIDARG;

    /* short-circuit if there's nothing to expand */
    if (szSrc[0] != '%')
    {
        lstrcpyW(szDest, szSrc);
        hr = S_OK;
        goto end;
    }

    /* Get the profile prefix, we'll probably be needing it */
    hr = _SHOpenProfilesKey(&key);
    if (SUCCEEDED(hr))
    {
        WCHAR def_val[MAX_PATH];

        /* get the system drive */
        GetSystemDirectoryW(def_val, MAX_PATH);
        lstrcpyW( def_val + 3, L"users" );

        hr = _SHGetProfilesValue(key, L"ProfilesDirectory", szProfilesPrefix, def_val );
    }

    *szDest = 0;
    lstrcpyW(szTemp, szSrc);
    while (SUCCEEDED(hr) && szTemp[0] == '%')
    {
        if (!wcsnicmp(szTemp, L"%ALLUSERSPROFILE%", lstrlenW(L"%ALLUSERSPROFILE%")))
        {
#ifndef __REACTOS__
            WCHAR szAllUsers[MAX_PATH], def_val[MAX_PATH];

            GetSystemDirectoryW(def_val, MAX_PATH);
            lstrcpyW( def_val + 3, L"users\\Public" );

            hr = _SHGetProfilesValue(key, L"Public", szAllUsers, def_val);
            PathAppendW(szDest, szAllUsers);
#else
            DWORD cchSize = cchDest;
            if (!GetAllUsersProfileDirectoryW(szDest, &cchSize))
                goto fallback_expand;
#endif
            PathAppendW(szDest, szTemp + lstrlenW(L"%ALLUSERSPROFILE%"));
        }
        else if (!wcsnicmp(szTemp, L"%PUBLIC%", lstrlenW(L"%PUBLIC%")))
        {
            WCHAR szAllUsers[MAX_PATH], def_val[MAX_PATH];

            GetSystemDirectoryW(def_val, MAX_PATH);
            lstrcpyW( def_val + 3, L"users\\Public" );

            hr = _SHGetProfilesValue(key, L"Public", szAllUsers, def_val);
            PathAppendW(szDest, szAllUsers);
            PathAppendW(szDest, szTemp + lstrlenW(L"%PUBLIC%"));
        }
        else if (!wcsnicmp(szTemp, L"%ProgramData%", lstrlenW(L"%ProgramData%")))
        {
            WCHAR szProgramData[MAX_PATH], def_val[MAX_PATH];
            HKEY shellFolderKey;
            DWORD dwType, dwPathLen = sizeof(def_val);
            BOOL in_registry = FALSE;

            if (!RegCreateKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", &shellFolderKey))
            {
                if (!RegQueryValueExW(shellFolderKey, L"Common AppData", NULL, &dwType,
                    (LPBYTE)def_val, &dwPathLen) && (dwType == REG_EXPAND_SZ || dwType == REG_SZ))
                    in_registry = TRUE;

                RegCloseKey(shellFolderKey);
            }

            if (!in_registry)
            {
                GetSystemDirectoryW(def_val, MAX_PATH);
                lstrcpyW( def_val + 3, L"ProgramData" );
            }

            hr = _SHGetProfilesValue(key, L"ProgramData", szProgramData, def_val);
            PathAppendW(szDest, szProgramData);
            PathAppendW(szDest, szTemp + lstrlenW(L"%ProgramData%"));
        }
        else if (!wcsnicmp(szTemp, L"%USERPROFILE%", lstrlenW(L"%USERPROFILE%")))
        {
#ifndef __REACTOS__
            WCHAR userName[MAX_PATH];
            DWORD userLen = MAX_PATH;

            lstrcpyW(szDest, szProfilesPrefix);
            GetUserNameW(userName, &userLen);
            PathAppendW(szDest, userName);
#else
            DWORD cchSize = cchDest;
            if (!SHELL_GetUserProfileDirectoryW(hToken, szDest, &cchSize))
                goto fallback_expand;
#endif
            PathAppendW(szDest, szTemp + lstrlenW(L"%USERPROFILE%"));
        }
        else if (!wcsnicmp(szTemp, L"%SystemDrive%", lstrlenW(L"%SystemDrive%")))
        {
#ifndef __REACTOS__
            GetSystemDirectoryW(szDest, MAX_PATH);
#else
            if (!GetSystemDirectoryW(szDest, cchDest))
                goto fallback_expand;
#endif
            lstrcpyW(szDest + 3, szTemp + lstrlenW(L"%SystemDrive%") + 1);
        }
        else
#ifdef __REACTOS__
fallback_expand:
#endif
        {
#ifndef __REACTOS__
            DWORD ret = ExpandEnvironmentStringsW(szTemp, szDest, MAX_PATH);
#else
            DWORD ret = SHExpandEnvironmentStringsForUserW(hToken, szTemp, szDest, cchDest);
#endif

#ifndef __REACTOS__
            if (ret > MAX_PATH)
#else
            if (ret > cchDest)
#endif
                hr = E_NOT_SUFFICIENT_BUFFER;
            else if (ret == 0)
                hr = HRESULT_FROM_WIN32(GetLastError());
            else if (!wcscmp( szTemp, szDest )) break;  /* nothing expanded */
        }
        if (SUCCEEDED(hr)) lstrcpyW(szTemp, szDest);
    }
end:
#ifndef __REACTOS__
    if (key)
        RegCloseKey(key);
#endif
    TRACE("returning 0x%08lx (input was %s, output is %s)\n", hr,
     debugstr_w(szSrc), debugstr_w(szDest));
    return hr;
}

#ifndef __REACTOS__
static char *xdg_config;
static DWORD xdg_config_len;

static BOOL WINAPI init_xdg_dirs( INIT_ONCE *once, void *param, void **context )
{
    const WCHAR *var, *fmt = L"\\??\\unix%s/user-dirs.dirs";
    char *p;
    WCHAR *name, *ptr;
    HANDLE file;
    DWORD len;

    if (!(var = _wgetenv( L"WINE_HOST_XDG_CONFIG_HOME" )) || var[0] != '/')
    {
        if (!(var = _wgetenv( L"WINEHOMEDIR" ))) return TRUE;
        fmt = L"%s/.config/user-dirs.dirs";
    }
    len = lstrlenW(var) + lstrlenW(fmt);
    name = malloc( len * sizeof(WCHAR) );
    swprintf( name, len, fmt, var );
    name[1] = '\\';  /* change \??\ to \\?\ */
    for (ptr = name; *ptr; ptr++) if (*ptr == '/') *ptr = '\\';

    file = CreateFileW( name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    free( name );
    if (file != INVALID_HANDLE_VALUE)
    {
        len = GetFileSize( file, NULL );
        if (!(xdg_config = malloc( len + 1 ))) return TRUE;
        if (!ReadFile( file, xdg_config, len, &xdg_config_len, NULL ))
        {
            free( xdg_config );
            xdg_config = NULL;
        }
        else
        {
            for (p = xdg_config; p < xdg_config + xdg_config_len; p++) if (*p == '\n') *p = 0;
            *p = 0;  /* append null to simplify string parsing */
        }
        CloseHandle( file );
    }
    return TRUE;
}

static char *get_xdg_path( const char *var )
{
    static INIT_ONCE once;
    char *p, *ret = NULL;
    int i;

    InitOnceExecuteOnce( &once, init_xdg_dirs, NULL, NULL );
    if (!xdg_config) return NULL;

    for (p = xdg_config; p < xdg_config + xdg_config_len; p += strlen(p) + 1)
    {
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp( p, var, strlen(var) )) continue;
        p += strlen(var);
        while (*p == ' ' || *p == '\t') p++;
        if (*p != '=') continue;
        p++;
        while (*p == ' ' || *p == '\t') p++;
        if (*p != '"') continue;
        p++;
        if (*p != '/' && strncmp( p, "$HOME/", 6 )) continue;

        if (!(ret = malloc( strlen(p) + 1 ))) break;
        for (i = 0; *p && *p != '"'; i++, p++)
        {
            if (*p == '\\' && p[1]) p++;
            ret[i] = *p;
        }
        ret[i] = 0;
        if (*p != '"')
        {
            free( ret );
            ret = NULL;
        }
        break;
    }
    return ret;
}

static BOOL link_folder( HANDLE mgr, const UNICODE_STRING *path, const char *link )
{
    struct mountmgr_shell_folder *ioctl;
    DWORD len = sizeof(*ioctl) + path->Length + strlen(link) + 1;
    BOOL ret;

    if (!(ioctl = malloc( len ))) return FALSE;
    ioctl->create_backup = FALSE;
    ioctl->folder_offset = sizeof(*ioctl);
    ioctl->folder_size = path->Length;
    memcpy( (char *)ioctl + ioctl->folder_offset, path->Buffer, ioctl->folder_size );
    ioctl->symlink_offset = ioctl->folder_offset + ioctl->folder_size;
    strcpy( (char *)ioctl + ioctl->symlink_offset, link );

    ret = DeviceIoControl( mgr, IOCTL_MOUNTMGR_DEFINE_SHELL_FOLDER, ioctl, len, NULL, 0, NULL, NULL );
    free( ioctl );
    return ret;
}

/******************************************************************************
 * create_link
 *
 * Sets up a symbolic link for one of the 'My Whatever' shell folders to point
 * into the corresponding XDG directory.
 */
static void create_link( const WCHAR *path, const char *xdg_name, const char *default_name )
{
    UNICODE_STRING nt_name;
    char *target = NULL;
    HANDLE mgr;

    if ((mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                            0, 0 )) == INVALID_HANDLE_VALUE)
    {
        FIXME( "failed to connect to mount manager\n" );
        return;
    }

    nt_name.Buffer = NULL;
    if (!RtlDosPathNameToNtPathName_U( path, &nt_name, NULL, NULL )) goto done;

    if ((target = get_xdg_path( xdg_name )))
    {
        if (link_folder( mgr, &nt_name, target )) goto done;
    }
    link_folder( mgr, &nt_name, default_name );

done:
    RtlFreeUnicodeString( &nt_name );
    free( target );
    CloseHandle( mgr );
}

/******************************************************************************
 * _SHCreateSymbolicLink  [Internal]
 *
 * Sets up a symbolic link for one of the special shell folders to point into
 * the users home directory.
 *
 * PARAMS
 *  nFolder [I] CSIDL identifying the folder.
 */
static void _SHCreateSymbolicLink(int nFolder, const WCHAR *path)
{
    DWORD folder = nFolder & CSIDL_FOLDER_MASK;

    switch (folder) {
        case CSIDL_PERSONAL:
            create_link( path, "XDG_DOCUMENTS_DIR", "$HOME/Documents" );
            break;
        case CSIDL_DESKTOPDIRECTORY:
            create_link( path, "XDG_DESKTOP_DIR", "$HOME/Desktop" );
            break;
        case CSIDL_MYPICTURES:
            create_link( path, "XDG_PICTURES_DIR", "$HOME/Pictures" );
            break;
        case CSIDL_MYVIDEO:
            create_link( path, "XDG_VIDEOS_DIR", "$HOME/Movies" );
            break;
        case CSIDL_MYMUSIC:
            create_link( path, "XDG_MUSIC_DIR", "$HOME/Music" );
            break;
        case CSIDL_DOWNLOADS:
            create_link( path, "XDG_DOWNLOAD_DIR", "$HOME/Downloads" );
            break;
        case CSIDL_TEMPLATES:
            create_link( path, "XDG_TEMPLATES_DIR", "$HOME/Templates" );
            break;
    }
}
#endif

/******************************************************************************
 * SHGetFolderPathW			[SHELL32.@]
 *
 * Convert nFolder to path.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: standard HRESULT error codes.
 *
 * NOTES
 * Most values can be overridden in either
 * HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders
 * or in the same location in HKLM.
 * The "Shell Folders" registry key was used in NT4 and earlier systems.
 * Beginning with Windows 2000, the "User Shell Folders" key is used, so
 * changes made to it are made to the former key too.  This synchronization is
 * done on-demand: not until someone requests the value of one of these paths
 * (by calling one of the SHGet functions) is the value synchronized.
 * Furthermore, the HKCU paths take precedence over the HKLM paths.
 */
HRESULT WINAPI SHGetFolderPathW(
	HWND hwndOwner,    /* [I] owner window */
	int nFolder,       /* [I] CSIDL identifying the folder */
	HANDLE hToken,     /* [I] access token */
	DWORD dwFlags,     /* [I] which path to return */
	LPWSTR pszPath)    /* [O] converted path */
{
    HRESULT hr =  SHGetFolderPathAndSubDirW(hwndOwner, nFolder, hToken, dwFlags, NULL, pszPath);
    if(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    return hr;
}

HRESULT WINAPI SHGetFolderPathAndSubDirA(
	HWND hwndOwner,    /* [I] owner window */
	int nFolder,       /* [I] CSIDL identifying the folder */
	HANDLE hToken,     /* [I] access token */
	DWORD dwFlags,     /* [I] which path to return */
	LPCSTR pszSubPath, /* [I] sub directory of the specified folder */
	LPSTR pszPath)     /* [O] converted path */
{
    int length;
    HRESULT hr = S_OK;
    LPWSTR pszSubPathW = NULL;
    LPWSTR pszPathW = NULL;

    TRACE("%p,%#x,%p,%#lx,%s,%p\n", hwndOwner, nFolder, hToken, dwFlags, debugstr_a(pszSubPath), pszPath);

    if(pszPath) {
        pszPathW = malloc(MAX_PATH * sizeof(WCHAR));
        if(!pszPathW) {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
    }
    TRACE("%08x,%08lx,%s\n",nFolder, dwFlags, debugstr_w(pszSubPathW));

    /* SHGetFolderPathAndSubDirW does not distinguish if pszSubPath isn't
     * set (null), or an empty string.therefore call it without the parameter set
     * if pszSubPath is an empty string
     */
    if (pszSubPath && pszSubPath[0]) {
        length = MultiByteToWideChar(CP_ACP, 0, pszSubPath, -1, NULL, 0);
        pszSubPathW = malloc(length * sizeof(WCHAR));
        if(!pszSubPathW) {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, pszSubPath, -1, pszSubPathW, length);
    }

    hr = SHGetFolderPathAndSubDirW(hwndOwner, nFolder, hToken, dwFlags, pszSubPathW, pszPathW);

    if (SUCCEEDED(hr) && pszPath)
        WideCharToMultiByte(CP_ACP, 0, pszPathW, -1, pszPath, MAX_PATH, NULL, NULL);

cleanup:
    free(pszPathW);
    free(pszSubPathW);
    return hr;
}

/*************************************************************************
 * SHGetFolderPathAndSubDirW		[SHELL32.@]
 */
HRESULT WINAPI SHGetFolderPathAndSubDirW(
	HWND hwndOwner,    /* [I] owner window */
	int nFolder,       /* [I] CSIDL identifying the folder */
	HANDLE hToken,     /* [I] access token */
	DWORD dwFlags,     /* [I] which path to return */
	LPCWSTR pszSubPath,/* [I] sub directory of the specified folder */
	LPWSTR pszPath)    /* [O] converted path */
{
    HRESULT    hr;
    WCHAR      szBuildPath[MAX_PATH], szTemp[MAX_PATH];
    DWORD      folder = nFolder & CSIDL_FOLDER_MASK;
    CSIDL_Type type;
    int        ret;

    TRACE("%p,%#x,%p,%#lx,%s,%p\n", hwndOwner, nFolder, hToken, dwFlags, debugstr_w(pszSubPath), pszPath);

    /* Windows always NULL-terminates the resulting path regardless of success
     * or failure, so do so first
     */
    if (pszPath)
        *pszPath = '\0';

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
    if ((SHGFP_TYPE_CURRENT != dwFlags) && (SHGFP_TYPE_DEFAULT != dwFlags))
        return E_INVALIDARG;
    szTemp[0] = 0;
    type = CSIDL_Data[folder].type;
    switch (type)
    {
        case CSIDL_Type_Disallowed:
            hr = E_INVALIDARG;
            break;
        case CSIDL_Type_NonExistent:
            hr = S_FALSE;
            break;
        case CSIDL_Type_WindowsPath:
            GetWindowsDirectoryW(szTemp, MAX_PATH);
            append_relative_path(folder, szTemp);
            hr = S_OK;
            break;
        case CSIDL_Type_SystemPath:
            GetSystemDirectoryW(szTemp, MAX_PATH);
            append_relative_path(folder, szTemp);
            hr = S_OK;
            break;
        case CSIDL_Type_SystemX86Path:
            if (!GetSystemWow64DirectoryW(szTemp, MAX_PATH)) GetSystemDirectoryW(szTemp, MAX_PATH);
            append_relative_path(folder, szTemp);
            hr = S_OK;
            break;
        case CSIDL_Type_CurrVer:
            hr = _SHGetCurrentVersionPath(dwFlags, folder, szTemp);
            break;
        case CSIDL_Type_User:
            hr = _SHGetUserProfilePath(hToken, dwFlags, folder, szTemp);
            break;
        case CSIDL_Type_AllUsers:
        case CSIDL_Type_ProgramData:
            hr = _SHGetAllUsersProfilePath(dwFlags, folder, szTemp);
            break;
        default:
            FIXME("bogus type %d, please fix\n", type);
            hr = E_INVALIDARG;
            break;
    }

    /* Expand environment strings if necessary */
    if (*szTemp == '%')
#ifndef __REACTOS__
        hr = _SHExpandEnvironmentStrings(szTemp, szBuildPath);
#else
        hr = _SHExpandEnvironmentStrings(hToken, szTemp, szBuildPath, _countof(szBuildPath));
#endif
    else
        lstrcpyW(szBuildPath, szTemp);

    if (FAILED(hr)) goto end;

    if(pszSubPath) {
        /* make sure the new path does not exceed the buffer length
         * and remember to backslash and terminate it */
        if(MAX_PATH < (lstrlenW(szBuildPath) + lstrlenW(pszSubPath) + 2)) {
            hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
            goto end;
        }
        PathAppendW(szBuildPath, pszSubPath);
        PathRemoveBackslashW(szBuildPath);
    }
    /* Copy the path if it's available before we might return */
    if (SUCCEEDED(hr) && pszPath)
        lstrcpyW(pszPath, szBuildPath);

    /* if we don't care about existing directories we are ready */
    if(nFolder & CSIDL_FLAG_DONT_VERIFY) goto end;

    if (PathFileExistsW(szBuildPath)) goto end;

    /* not existing but we are not allowed to create it.  The return value
     * is verified against shell32 version 6.0.
     */
    if (!(nFolder & CSIDL_FLAG_CREATE))
    {
        hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        goto end;
    }

#ifndef __REACTOS__
    /* create symbolic links rather than directories for specific
     * user shell folders */
    if (!pszSubPath)
        _SHCreateSymbolicLink(folder, szBuildPath);
#endif

    /* create directory/directories */
    ret = SHCreateDirectoryExW(hwndOwner, szBuildPath, NULL);
    if (ret && ret != ERROR_ALREADY_EXISTS)
    {
        ERR("Failed to create directory %s.\n", debugstr_w(szBuildPath));
        hr = E_FAIL;
        goto end;
    }

    TRACE("Created missing system directory %s\n", debugstr_w(szBuildPath));
end:
#ifdef __REACTOS__
    /* create desktop.ini for custom icon */
    if ((nFolder & CSIDL_FLAG_CREATE) &&
        CSIDL_Data[folder].nShell32IconIndex)
    {
        WCHAR szIconLocation[MAX_PATH];
        DWORD dwAttributes;

        /* make the directory a read-only folder */
        dwAttributes = GetFileAttributesW(szBuildPath);
        dwAttributes |= FILE_ATTRIBUTE_READONLY;
        SetFileAttributesW(szBuildPath, dwAttributes);

        /* build the desktop.ini file path */
        PathAppendW(szBuildPath, L"desktop.ini");

        /* build the icon location */
        StringCchPrintfW(szIconLocation, _countof(szIconLocation),
                         L"%%SystemRoot%%\\system32\\shell32.dll,%d",
                         CSIDL_Data[folder].nShell32IconIndex);

        /* write desktop.ini */
        WritePrivateProfileStringW(L".ShellClassInfo", L"IconResource", szIconLocation, szBuildPath);

        /* flush! */
        WritePrivateProfileStringW(NULL, NULL, NULL, szBuildPath);

        /* make the desktop.ini a system and hidden file */
        dwAttributes = GetFileAttributesW(szBuildPath);
        dwAttributes |= FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN;
        SetFileAttributesW(szBuildPath, dwAttributes);
    }
#endif
    TRACE("returning 0x%08lx (final path is %s)\n", hr, debugstr_w(szBuildPath));
    return hr;
}

/*************************************************************************
 * SHGetFolderPathA			[SHELL32.@]
 *
 * See SHGetFolderPathW.
 */
HRESULT WINAPI SHGetFolderPathA(
	HWND hwndOwner,
	int nFolder,
	HANDLE hToken,
	DWORD dwFlags,
	LPSTR pszPath)
{
    WCHAR szTemp[MAX_PATH];
    HRESULT hr;

    TRACE("%p,%d,%p,%#lx,%p\n", hwndOwner, nFolder, hToken, dwFlags, pszPath);

    if (pszPath)
        *pszPath = '\0';
    hr = SHGetFolderPathW(hwndOwner, nFolder, hToken, dwFlags, szTemp);
    if (SUCCEEDED(hr) && pszPath)
        WideCharToMultiByte(CP_ACP, 0, szTemp, -1, pszPath, MAX_PATH, NULL,
         NULL);

    return hr;
}

/* For each folder in folders, if its value has not been set in the registry,
 * calls _SHGetUserProfilePath or _SHGetAllUsersProfilePath (depending on the
 * folder's type) to get the unexpanded value first.
 * Writes the unexpanded value to User Shell Folders, and queries it with
 * SHGetFolderPathW to force the creation of the directory if it doesn't
 * already exist.  SHGetFolderPathW also returns the expanded value, which
 * this then writes to Shell Folders.
 */
static HRESULT _SHRegisterFolders(HKEY hRootKey, HANDLE hToken,
 LPCWSTR szUserShellFolderPath, LPCWSTR szShellFolderPath, const UINT folders[],
 UINT foldersLen)
{
    const WCHAR *szValueName;
    WCHAR buffer[40];
    UINT i;
    WCHAR path[MAX_PATH];
    HRESULT hr = S_OK;
    HKEY hUserKey = NULL, hKey = NULL;
    DWORD dwType, dwPathLen;
    LONG ret;

    TRACE("%p,%p,%s,%p,%u\n", hRootKey, hToken,
     debugstr_w(szUserShellFolderPath), folders, foldersLen);

    ret = RegCreateKeyW(hRootKey, szUserShellFolderPath, &hUserKey);
    if (ret)
        hr = HRESULT_FROM_WIN32(ret);
    else
    {
        ret = RegCreateKeyW(hRootKey, szShellFolderPath, &hKey);
        if (ret)
            hr = HRESULT_FROM_WIN32(ret);
    }
    for (i = 0; SUCCEEDED(hr) && i < foldersLen; i++)
    {
        dwPathLen = MAX_PATH * sizeof(WCHAR);

        /* For CSIDL_Type_User we also use the GUID if no szValueName is provided */
        szValueName = CSIDL_Data[folders[i]].value;
        if (!szValueName && CSIDL_Data[folders[i]].type == CSIDL_Type_User)
        {
            StringFromGUID2( CSIDL_Data[folders[i]].id, buffer, 39 );
            szValueName = &buffer[0];
        }

        if (RegQueryValueExW(hUserKey, szValueName, NULL,
         &dwType, (LPBYTE)path, &dwPathLen) || (dwType != REG_SZ &&
         dwType != REG_EXPAND_SZ))
        {
            *path = '\0';
            if (CSIDL_Data[folders[i]].type == CSIDL_Type_User)
                _SHGetUserProfilePath(hToken, SHGFP_TYPE_DEFAULT, folders[i],
                 path);
            else if (CSIDL_Data[folders[i]].type == CSIDL_Type_AllUsers ||
                     CSIDL_Data[folders[i]].type == CSIDL_Type_ProgramData)
                _SHGetAllUsersProfilePath(SHGFP_TYPE_DEFAULT, folders[i], path);
            else if (CSIDL_Data[folders[i]].type == CSIDL_Type_WindowsPath)
            {
                GetWindowsDirectoryW(path, MAX_PATH);
                append_relative_path(folders[i], path);
            }
            else
                hr = E_FAIL;
            if (*path)
            {
                ret = RegSetValueExW(hUserKey, szValueName, 0, REG_EXPAND_SZ,
                 (LPBYTE)path, (lstrlenW(path) + 1) * sizeof(WCHAR));
                if (ret)
                    hr = HRESULT_FROM_WIN32(ret);
                else
                {
                    hr = SHGetFolderPathW(NULL, folders[i] | CSIDL_FLAG_CREATE,
                     hToken, SHGFP_TYPE_DEFAULT, path);
                    ret = RegSetValueExW(hKey, szValueName, 0, REG_SZ,
                     (LPBYTE)path, (lstrlenW(path) + 1) * sizeof(WCHAR));
                    if (ret)
                        hr = HRESULT_FROM_WIN32(ret);
                }
            }
        }
        else
        {
            /* create the default dir, which may be different from the path
             * stored in the registry. */
            SHGetFolderPathW(NULL, folders[i] | CSIDL_FLAG_CREATE,
             hToken, SHGFP_TYPE_DEFAULT, path);
        }
    }
    if (hUserKey)
        RegCloseKey(hUserKey);
    if (hKey)
        RegCloseKey(hKey);

    TRACE("returning 0x%08lx\n", hr);
    return hr;
}

static HRESULT _SHRegisterUserShellFolders(BOOL bDefault)
{
    static const UINT folders[] = {
     CSIDL_PROGRAMS,
     CSIDL_PERSONAL,
     CSIDL_FAVORITES,
     CSIDL_APPDATA,
     CSIDL_STARTUP,
     CSIDL_RECENT,
     CSIDL_SENDTO,
     CSIDL_STARTMENU,
     CSIDL_MYMUSIC,
     CSIDL_MYVIDEO,
     CSIDL_DESKTOPDIRECTORY,
     CSIDL_NETHOOD,
     CSIDL_TEMPLATES,
     CSIDL_PRINTHOOD,
     CSIDL_LOCAL_APPDATA,
     CSIDL_INTERNET_CACHE,
     CSIDL_COOKIES,
     CSIDL_HISTORY,
     CSIDL_MYPICTURES,
     CSIDL_FONTS,
     CSIDL_ADMINTOOLS,
     CSIDL_CONTACTS,
     CSIDL_DOWNLOADS,
     CSIDL_LINKS,
     CSIDL_APPDATA_LOCALLOW,
     CSIDL_SAVED_GAMES,
     CSIDL_SEARCHES
    };
    WCHAR userShellFolderPath[MAX_PATH], shellFolderPath[MAX_PATH];
    LPCWSTR pUserShellFolderPath, pShellFolderPath;
    HRESULT hr = S_OK;
    HKEY hRootKey;
    HANDLE hToken;

    TRACE("%s\n", bDefault ? "TRUE" : "FALSE");
    if (bDefault)
    {
        hToken = (HANDLE)-1;
        hRootKey = HKEY_USERS;
        lstrcpyW(userShellFolderPath, L".Default");
        PathAddBackslashW(userShellFolderPath);
        lstrcatW(userShellFolderPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders");
        pUserShellFolderPath = userShellFolderPath;
        lstrcpyW(shellFolderPath, L".Default");
        PathAddBackslashW(shellFolderPath);
        lstrcatW(shellFolderPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
        pShellFolderPath = shellFolderPath;
    }
    else
    {
        hToken = NULL;
        hRootKey = HKEY_CURRENT_USER;
        pUserShellFolderPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";
        pShellFolderPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
    }

    hr = _SHRegisterFolders(hRootKey, hToken, pUserShellFolderPath,
     pShellFolderPath, folders, ARRAY_SIZE(folders));
    TRACE("returning 0x%08lx\n", hr);
    return hr;
}

static HRESULT _SHRegisterCommonShellFolders(void)
{
    static const UINT folders[] = {
     CSIDL_COMMON_STARTMENU,
     CSIDL_COMMON_PROGRAMS,
     CSIDL_COMMON_STARTUP,
     CSIDL_COMMON_DESKTOPDIRECTORY,
     CSIDL_COMMON_FAVORITES,
     CSIDL_COMMON_APPDATA,
     CSIDL_COMMON_TEMPLATES,
     CSIDL_COMMON_DOCUMENTS,
     CSIDL_COMMON_ADMINTOOLS,
     CSIDL_COMMON_MUSIC,
     CSIDL_COMMON_PICTURES,
     CSIDL_COMMON_VIDEO,
    };
    HRESULT hr;

    TRACE("\n");
    hr = _SHRegisterFolders(HKEY_LOCAL_MACHINE, NULL,
                            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
                            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                            folders, ARRAY_SIZE(folders));
    TRACE("returning 0x%08lx\n", hr);
    return hr;
}

#ifndef __REACTOS__
/******************************************************************************
 * create_extra_folders  [Internal]
 *
 * Create some extra folders that don't have a standard CSIDL definition.
 */
static HRESULT create_extra_folders(void)
{
    WCHAR path[MAX_PATH+5];
    HRESULT hr;
    HKEY hkey;
    DWORD type, size, ret;

    ret = RegCreateKeyW( HKEY_CURRENT_USER, L"Environment", &hkey );
    if (ret) return HRESULT_FROM_WIN32( ret );

    hr = SHGetFolderPathAndSubDirW( 0, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL,
                                    SHGFP_TYPE_DEFAULT, L"Temp", path );
    if (SUCCEEDED(hr))
    {
        size = sizeof(path);
        if (RegQueryValueExW( hkey, L"TEMP", NULL, &type, (LPBYTE)path, &size ))
            RegSetValueExW( hkey, L"TEMP", 0, REG_SZ, (LPBYTE)path, (lstrlenW(path) + 1) * sizeof(WCHAR) );
        size = sizeof(path);
        if (RegQueryValueExW( hkey, L"TMP", NULL, &type, (LPBYTE)path, &size ))
            RegSetValueExW( hkey, L"TMP", 0, REG_SZ, (LPBYTE)path, (lstrlenW(path) + 1) * sizeof(WCHAR) );
    }
    RegCloseKey( hkey );

    if (SUCCEEDED(hr))
    {
        hr = SHGetFolderPathAndSubDirW( 0, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL,
                                        SHGFP_TYPE_DEFAULT, L"Microsoft", path );
    }
    if (SUCCEEDED(hr))
    {
        hr = SHGetFolderPathAndSubDirW(0, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL,
                                       SHGFP_TYPE_DEFAULT, L"Microsoft\\Windows\\Themes", path);
    }
    if (SUCCEEDED(hr))
    {
        hr = SHGetFolderPathAndSubDirW(0, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL,
                                       SHGFP_TYPE_DEFAULT, L"Microsoft\\Windows\\AccountPictures", path);
    }
    if (SUCCEEDED(hr))
    {
        hr = SHGetFolderPathAndSubDirW(0, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL,
                                       SHGFP_TYPE_DEFAULT, L"Screenshots", path);
    }
    return hr;
}
#endif


/******************************************************************************
 * set_folder_attributes
 *
 * Set the various folder attributes registry keys.
 */
static HRESULT set_folder_attributes(void)
{
    static const struct
    {
        const CLSID *clsid;
        unsigned int wfparsing : 1;
        unsigned int wfdisplay : 1;
        unsigned int hideasdel : 1;
        DWORD attr;
        DWORD call_for_attr;
    } folders[] =
    {
#ifndef __REACTOS__
        { &CLSID_UnixFolder, TRUE, FALSE, FALSE },
        { &CLSID_UnixDosFolder, TRUE, FALSE, FALSE,
          SFGAO_FILESYSANCESTOR|SFGAO_FOLDER|SFGAO_HASSUBFOLDER, SFGAO_FILESYSTEM },
#endif
        { &CLSID_FolderShortcut, FALSE, FALSE, FALSE,
          SFGAO_FILESYSTEM|SFGAO_FOLDER|SFGAO_LINK,
          SFGAO_HASSUBFOLDER|SFGAO_FILESYSTEM|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR },
        { &CLSID_MyDocuments, TRUE, FALSE, FALSE,
          SFGAO_FILESYSANCESTOR|SFGAO_FOLDER|SFGAO_HASSUBFOLDER, SFGAO_FILESYSTEM },
        { &CLSID_RecycleBin, FALSE, FALSE, FALSE,
          SFGAO_FOLDER|SFGAO_DROPTARGET|SFGAO_HASPROPSHEET },
        { &CLSID_ControlPanel, FALSE, TRUE, TRUE,
          SFGAO_FOLDER|SFGAO_HASSUBFOLDER },
        { &CLSID_Printers, FALSE, FALSE, FALSE,
          SFGAO_FOLDER|SFGAO_CANLINK }
    };

    unsigned int i;
    WCHAR buffer[39 + ARRAY_SIZE(L"CLSID\\") + ARRAY_SIZE(L"\\ShellFolder")];
    LONG res;
    HKEY hkey;

    for (i = 0; i < ARRAY_SIZE(folders); i++)
    {
        lstrcpyW( buffer, L"CLSID\\" );
        StringFromGUID2( folders[i].clsid, buffer + lstrlenW(buffer), 39 );
        lstrcatW( buffer, L"\\ShellFolder" );
        res = RegCreateKeyExW( HKEY_CLASSES_ROOT, buffer, 0, NULL, 0,
                               KEY_READ | KEY_WRITE, NULL, &hkey, NULL);
        if (res) return HRESULT_FROM_WIN32( res );
        if (folders[i].wfparsing)
            res = RegSetValueExW( hkey, L"WantsFORPARSING", 0, REG_SZ, (const BYTE *)L"", sizeof(WCHAR) );
        if (folders[i].wfdisplay)
            res = RegSetValueExW( hkey, L"WantsFORDISPLAY", 0, REG_SZ, (const BYTE *)L"", sizeof(WCHAR) );
        if (folders[i].hideasdel)
            res = RegSetValueExW( hkey, L"HideAsDeletePerUser", 0, REG_SZ, (const BYTE *)L"", sizeof(WCHAR) );
        if (folders[i].attr)
            res = RegSetValueExW( hkey, L"Attributes", 0, REG_DWORD,
                                  (const BYTE *)&folders[i].attr, sizeof(DWORD));
        if (folders[i].call_for_attr)
            res = RegSetValueExW( hkey, L"CallForAttributes", 0, REG_DWORD,
                                 (const BYTE *)&folders[i].call_for_attr, sizeof(DWORD));
        RegCloseKey( hkey );
    }
    return S_OK;
}

/*************************************************************************
 * SHGetSpecialFolderPathA [SHELL32.@]
 */
BOOL WINAPI SHGetSpecialFolderPathA (
	HWND hwndOwner,
	LPSTR szPath,
	int nFolder,
	BOOL bCreate)
{
    return SHGetFolderPathA(hwndOwner, nFolder + (bCreate ? CSIDL_FLAG_CREATE : 0), NULL, 0,
                            szPath) == S_OK;
}

/*************************************************************************
 * SHGetSpecialFolderPathW
 */
BOOL WINAPI SHGetSpecialFolderPathW (
	HWND hwndOwner,
	LPWSTR szPath,
	int nFolder,
	BOOL bCreate)
{
    return SHGetFolderPathW(hwndOwner, nFolder + (bCreate ? CSIDL_FLAG_CREATE : 0), NULL, 0,
                            szPath) == S_OK;
}

#ifdef __REACTOS__
HRESULT SHGetFolderLocationHelper(HWND hwnd, int nFolder, REFCLSID clsid, LPITEMIDLIST *ppidl)
{
    HRESULT hr;
    IShellFolder *psf;
    LPITEMIDLIST parent, child;
    SHSTDAPI SHBindToObject(IShellFolder *psf, LPCITEMIDLIST pidl, IBindCtx *pBindCtx, REFIID riid, void **ppvObj);
    *ppidl = NULL;
    if (FAILED(hr = SHGetFolderLocation(hwnd, nFolder, NULL, 0, &parent)))
        return hr;
    if (SUCCEEDED(hr = SHBindToObject(NULL, parent, NULL, &IID_IShellFolder, (void**)&psf)))
    {
        WCHAR clsidstr[2 + 38 + 1];
        clsidstr[0] = clsidstr[1] = L':';
        StringFromGUID2(clsid, clsidstr + 2, 38 + 1);
        hr = IShellFolder_ParseDisplayName(psf, hwnd, NULL, clsidstr, NULL, &child, NULL);
        if (SUCCEEDED(hr))
            *ppidl = ILCombine(parent, child);
        IShellFolder_Release(psf);
        ILFree(child);
    }
    ILFree(parent);
    return hr;
}
#endif

/*************************************************************************
 * SHGetSpecialFolderPath (SHELL32.175)
 */
BOOL WINAPI SHGetSpecialFolderPathAW (
	HWND hwndOwner,
	LPVOID szPath,
	int nFolder,
	BOOL bCreate)

{
	if (SHELL_OsIsUnicode())
	  return SHGetSpecialFolderPathW (hwndOwner, szPath, nFolder, bCreate);
	return SHGetSpecialFolderPathA (hwndOwner, szPath, nFolder, bCreate);
}

/*************************************************************************
 * SHGetFolderLocation [SHELL32.@]
 *
 * Gets the folder locations from the registry and creates a pidl.
 *
 * PARAMS
 *   hwndOwner  [I]
 *   nFolder    [I] CSIDL_xxxxx
 *   hToken     [I] token representing user, or NULL for current user, or -1 for
 *                  default user
 *   dwReserved [I] must be zero
 *   ppidl      [O] PIDL of a special folder
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Standard OLE-defined error result, S_FALSE or E_INVALIDARG
 *
 * NOTES
 *  Creates missing reg keys and directories.
 *  Mostly forwards to SHGetFolderPathW, but a few values of nFolder return
 *  virtual folders that are handled here.
 */
HRESULT WINAPI SHGetFolderLocation(
	HWND hwndOwner,
	int nFolder,
	HANDLE hToken,
	DWORD dwReserved,
	LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_INVALIDARG;
#ifdef __REACTOS__
    WCHAR szPath[MAX_PATH];
#endif

    TRACE("%p 0x%08x %p 0x%08lx %p\n",
     hwndOwner, nFolder, hToken, dwReserved, ppidl);
    
    if (!ppidl)
        return E_INVALIDARG;
    if (dwReserved)
        return E_INVALIDARG;

#ifdef __REACTOS__
    if ((nFolder & CSIDL_FLAG_NO_ALIAS) &&
        SHGetSpecialFolderPathW(hwndOwner, szPath, (nFolder & CSIDL_FOLDER_MASK), FALSE))
    {
        *ppidl = ILCreateFromPathW(szPath);
        if (*ppidl)
            return S_OK;
    }
#endif
    /* The virtual folders' locations are not user-dependent */
    *ppidl = NULL;
    switch (nFolder & CSIDL_FOLDER_MASK)
    {
        case CSIDL_DESKTOP:
            *ppidl = _ILCreateDesktop();
            break;

        case CSIDL_PERSONAL:
            *ppidl = _ILCreateMyDocuments();
            break;

        case CSIDL_INTERNET:
            *ppidl = _ILCreateIExplore();
            break;

        case CSIDL_CONTROLS:
            *ppidl = _ILCreateControlPanel();
            break;

        case CSIDL_PRINTERS:
            *ppidl = _ILCreatePrinters();
            break;

        case CSIDL_BITBUCKET:
            *ppidl = _ILCreateBitBucket();
            break;

        case CSIDL_DRIVES:
            *ppidl = _ILCreateMyComputer();
            break;

        case CSIDL_NETWORK:
            *ppidl = _ILCreateNetwork();
            break;

#ifdef __REACTOS__
        case CSIDL_CONNECTIONS:
            hr = SHGetFolderLocationHelper(hwndOwner, CSIDL_CONTROLS, &CLSID_NetworkConnections, ppidl);
            break;
#endif

        default:
        {
            WCHAR szPath[MAX_PATH];

            hr = SHGetFolderPathW(hwndOwner, nFolder, hToken,
             SHGFP_TYPE_CURRENT, szPath);
            if (SUCCEEDED(hr))
            {
                DWORD attributes=0;

                TRACE("Value=%s\n", debugstr_w(szPath));
                hr = SHILCreateFromPathW(szPath, ppidl, &attributes);
            }
            else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                /* unlike SHGetFolderPath, SHGetFolderLocation in shell32
                 * version 6.0 returns E_FAIL for nonexistent paths
                 */
                hr = E_FAIL;
            }
        }
    }
    if(*ppidl)
        hr = S_OK;

    TRACE("-- (new pidl %p)\n",*ppidl);
    return hr;
}

/*************************************************************************
 * SHGetSpecialFolderLocation		[SHELL32.@]
 *
 * NOTES
 *   In NT5, SHGetSpecialFolderLocation needs the <winntdir>/Recent
 *   directory.
 */
HRESULT WINAPI SHGetSpecialFolderLocation(
	HWND hwndOwner,
	INT nFolder,
	LPITEMIDLIST * ppidl)
{
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p,0x%x,%p)\n", hwndOwner,nFolder,ppidl);

    if (!ppidl)
        return E_INVALIDARG;

    hr = SHGetFolderLocation(hwndOwner, nFolder, NULL, 0, ppidl);
    return hr;
}

/*************************************************************************
 * SHGetKnownFolderPath           [SHELL32.@]
 */
HRESULT WINAPI SHGetKnownFolderPath(REFKNOWNFOLDERID rfid, DWORD flags, HANDLE token, WCHAR **ret_path)
{
    WCHAR pathW[MAX_PATH];
    HRESULT    hr;
    int folder = csidl_from_id(rfid), shgfp_flags;

    TRACE("%s, 0x%08lx, %p, %p\n", debugstr_guid(rfid), flags, token, ret_path);

    *ret_path = NULL;

    if (folder < 0)
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

    if (flags & ~(KF_FLAG_CREATE|KF_FLAG_SIMPLE_IDLIST|KF_FLAG_DONT_UNEXPAND|
        KF_FLAG_DONT_VERIFY|KF_FLAG_NO_ALIAS|KF_FLAG_INIT|KF_FLAG_DEFAULT_PATH|KF_FLAG_NOT_PARENT_RELATIVE))
    {
        FIXME("flags 0x%08lx not supported\n", flags);
        return E_INVALIDARG;
    }

    if ((flags & (KF_FLAG_DEFAULT_PATH | KF_FLAG_NOT_PARENT_RELATIVE)) == KF_FLAG_NOT_PARENT_RELATIVE)
    {
        WARN("Invalid flags mask %#lx.\n", flags);
        return E_INVALIDARG;
    }

    if (flags & KF_FLAG_NOT_PARENT_RELATIVE)
    {
        FIXME("Ignoring KF_FLAG_NOT_PARENT_RELATIVE.\n");
        flags &= ~KF_FLAG_NOT_PARENT_RELATIVE;
    }

    folder |= flags & CSIDL_FLAG_MASK;
    shgfp_flags = flags & KF_FLAG_DEFAULT_PATH ? SHGFP_TYPE_DEFAULT : SHGFP_TYPE_CURRENT;

    hr = SHGetFolderPathAndSubDirW( 0, folder, token, shgfp_flags, NULL, pathW );
    if (FAILED( hr ))
    {
        TRACE("Failed to get folder path, %#lx.\n", hr);
        return hr;
    }

    TRACE("Final path is %s, %#lx\n", debugstr_w(pathW), hr);

    *ret_path = CoTaskMemAlloc((lstrlenW(pathW) + 1) * sizeof(WCHAR));
    if (!*ret_path)
        return E_OUTOFMEMORY;
    lstrcpyW(*ret_path, pathW);

    return hr;
}

/*************************************************************************
 * SHGetFolderPathEx           [SHELL32.@]
 */
HRESULT WINAPI SHGetFolderPathEx(REFKNOWNFOLDERID rfid, DWORD flags, HANDLE token, LPWSTR path, DWORD len)
{
    HRESULT hr;
    WCHAR *buffer;

    TRACE("%s, 0x%08lx, %p, %p, %lu\n", debugstr_guid(rfid), flags, token, path, len);

    if (!path || !len) return E_INVALIDARG;

    hr = SHGetKnownFolderPath( rfid, flags, token, &buffer );
    if (SUCCEEDED( hr ))
    {
        if (lstrlenW( buffer ) + 1 > len)
        {
            CoTaskMemFree( buffer );
            return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
        }
        lstrcpyW( path, buffer );
        CoTaskMemFree( buffer );
    }
    return hr;
}

/*
 * Internal function to convert known folder identifier to path of registry key
 * associated with known folder.
 *
 * Parameters:
 *  rfid            [I] pointer to known folder identifier (may be NULL)
 *  lpStringGuid    [I] string with known folder identifier (used when rfid is NULL)
 *  lpPath          [O] place to store string address. String should be
 *                      later freed using HeapFree(GetProcessHeap(),0, ... )
 */
static HRESULT get_known_folder_registry_path(
    REFKNOWNFOLDERID rfid,
    LPWSTR lpStringGuid,
    LPWSTR *lpPath)
{
    HRESULT hr = S_OK;
    int length;
    WCHAR sGuid[50];

    TRACE("(%s, %s, %p)\n", debugstr_guid(rfid), debugstr_w(lpStringGuid), lpPath);

    if(rfid)
        StringFromGUID2(rfid, sGuid, ARRAY_SIZE(sGuid));
    else
        lstrcpyW(sGuid, lpStringGuid);

    length = lstrlenW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FolderDescriptions")+51;
    *lpPath = malloc(length * sizeof(WCHAR));
    if(!(*lpPath))
        hr = E_OUTOFMEMORY;

    if(SUCCEEDED(hr))
    {
        lstrcpyW(*lpPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FolderDescriptions\\");
        lstrcatW(*lpPath, sGuid);
    }

    return hr;
}

static HRESULT get_known_folder_wstr(const WCHAR *regpath, const WCHAR *value, WCHAR **out)
{
    DWORD size = 0;
    HRESULT hr;

    size = 0;
    hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, regpath, value, RRF_RT_REG_SZ, NULL, NULL, &size));
    if(FAILED(hr))
        return hr;

    *out = CoTaskMemAlloc(size);
    if(!*out)
        return E_OUTOFMEMORY;

    hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, regpath, value, RRF_RT_REG_SZ, NULL, *out, &size));
    if(FAILED(hr)){
        CoTaskMemFree(*out);
        *out = NULL;
    }

    return hr;
}

static HRESULT get_known_folder_dword(const WCHAR *registryPath, const WCHAR *value, DWORD *out)
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType;
    return HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, registryPath, value, RRF_RT_DWORD, &dwType, out, &dwSize));
}

/*
 * Internal function to get place where folder redirection information are stored.
 *
 * Parameters:
 *  rfid            [I] pointer to known folder identifier (may be NULL)
 *  rootKey         [O] root key where the redirection information are stored
 *                      It can be HKLM for COMMON folders, and HKCU for PERUSER folders.
 */
static HRESULT get_known_folder_redirection_place(
    REFKNOWNFOLDERID rfid,
    HKEY *rootKey)
{
    HRESULT hr;
    LPWSTR lpRegistryPath = NULL;
    DWORD category;

    /* first, get known folder's category */
    hr = get_known_folder_registry_path(rfid, NULL, &lpRegistryPath);

    if(SUCCEEDED(hr))
        hr = get_known_folder_dword(lpRegistryPath, L"Category", &category);

    if(SUCCEEDED(hr))
    {
        if(category == KF_CATEGORY_COMMON)
        {
            *rootKey = HKEY_LOCAL_MACHINE;
            hr = S_OK;
        }
        else if(category == KF_CATEGORY_PERUSER)
        {
            *rootKey = HKEY_CURRENT_USER;
            hr = S_OK;
        }
        else
            hr = E_FAIL;
    }

    free(lpRegistryPath);
    return hr;
}

static HRESULT get_known_folder_path_by_id(REFKNOWNFOLDERID folderId, LPWSTR lpRegistryPath, DWORD dwFlags, LPWSTR *ppszPath);

static HRESULT redirect_known_folder(
    REFKNOWNFOLDERID rfid,
    HWND hwnd,
    KF_REDIRECT_FLAGS flags,
    LPCWSTR pszTargetPath,
    UINT cFolders,
    KNOWNFOLDERID const *pExclusion,
    LPWSTR *ppszError)
{
    HRESULT hr;
    HKEY rootKey = HKEY_LOCAL_MACHINE, hKey;
    WCHAR sGuid[39];
    LPWSTR lpRegistryPath = NULL, lpSrcPath = NULL;
    TRACE("(%s, %p, 0x%08x, %s, %d, %p, %p)\n", debugstr_guid(rfid), hwnd, flags, debugstr_w(pszTargetPath), cFolders, pExclusion, ppszError);

    if (ppszError) *ppszError = NULL;

    hr = get_known_folder_registry_path(rfid, NULL, &lpRegistryPath);

    if(SUCCEEDED(hr))
        hr = get_known_folder_path_by_id(rfid, lpRegistryPath, 0, &lpSrcPath);

    free(lpRegistryPath);

    /* get path to redirection storage */
    if(SUCCEEDED(hr))
        hr = get_known_folder_redirection_place(rfid, &rootKey);

    /* write redirection information */
    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(RegCreateKeyExW(rootKey, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL));

    if(SUCCEEDED(hr))
    {
        StringFromGUID2(rfid, sGuid, ARRAY_SIZE(sGuid));

        hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, sGuid, 0, REG_SZ, (LPBYTE)pszTargetPath, (lstrlenW(pszTargetPath)+1)*sizeof(WCHAR)));

        RegCloseKey(hKey);
    }

    /* make sure destination path exists */
    SHCreateDirectory(NULL, pszTargetPath);

    /* copy content if required */
    if(SUCCEEDED(hr) && (flags & KF_REDIRECT_COPY_CONTENTS) )
    {
        WCHAR srcPath[MAX_PATH+1], dstPath[MAX_PATH+1];
        SHFILEOPSTRUCTW fileOp;

        ZeroMemory(srcPath, sizeof(srcPath));
        lstrcpyW(srcPath, lpSrcPath);
        lstrcatW(srcPath, L"\\*");

        ZeroMemory(dstPath, sizeof(dstPath));
        lstrcpyW(dstPath, pszTargetPath);

        ZeroMemory(&fileOp, sizeof(fileOp));

        if(flags & KF_REDIRECT_DEL_SOURCE_CONTENTS)
            fileOp.wFunc = FO_MOVE;
        else
            fileOp.wFunc = FO_COPY;

        fileOp.pFrom = srcPath;
        fileOp.pTo = dstPath;
        fileOp.fFlags = FOF_NO_UI;

        hr = (SHFileOperationW(&fileOp)==0 ? S_OK : E_FAIL);

        if(flags & KF_REDIRECT_DEL_SOURCE_CONTENTS)
        {
            ZeroMemory(srcPath, sizeof(srcPath));
            lstrcpyW(srcPath, lpSrcPath);

            ZeroMemory(&fileOp, sizeof(fileOp));
            fileOp.wFunc = FO_DELETE;
            fileOp.pFrom = srcPath;
            fileOp.fFlags = FOF_NO_UI;

            hr = (SHFileOperationW(&fileOp)==0 ? S_OK : E_FAIL);
        }
    }

    CoTaskMemFree(lpSrcPath);

    return hr;
}


struct knownfolder
{
    IKnownFolder IKnownFolder_iface;
    LONG refs;
    KNOWNFOLDERID id;
    LPWSTR registryPath;
};

static inline struct knownfolder *impl_from_IKnownFolder( IKnownFolder *iface )
{
    return CONTAINING_RECORD( iface, struct knownfolder, IKnownFolder_iface );
}

static ULONG WINAPI knownfolder_AddRef(
    IKnownFolder *iface )
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    return InterlockedIncrement( &knownfolder->refs );
}

static ULONG WINAPI knownfolder_Release(
    IKnownFolder *iface )
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    LONG refs = InterlockedDecrement( &knownfolder->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", knownfolder);
        free( knownfolder->registryPath );
        free( knownfolder );
    }
    return refs;
}

static HRESULT WINAPI knownfolder_QueryInterface(
    IKnownFolder *iface,
    REFIID riid,
    void **ppv )
{
    struct knownfolder *This = impl_from_IKnownFolder( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppv );

    *ppv = NULL;
    if ( IsEqualGUID( riid, &IID_IKnownFolder ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppv = iface;
    }
    else if ( IsEqualGUID( riid, &IID_IMarshal ) )
    {
        TRACE("IID_IMarshal returning NULL.\n");
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IKnownFolder_AddRef( iface );
    return S_OK;
}

static HRESULT knownfolder_set_id(
    struct knownfolder *knownfolder,
    const KNOWNFOLDERID *kfid)
{
    HKEY hKey;
    HRESULT hr;

    TRACE("%s\n", debugstr_guid(kfid));

    knownfolder->id = *kfid;

    /* check is it registry-registered folder */
    hr = get_known_folder_registry_path(kfid, NULL, &knownfolder->registryPath);
    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(RegOpenKeyExW(HKEY_LOCAL_MACHINE, knownfolder->registryPath, 0, KEY_ENUMERATE_SUB_KEYS, &hKey));

    if(SUCCEEDED(hr))
    {
        hr = S_OK;
        RegCloseKey(hKey);
    }
    else
    {
        /* This known folder is not registered. To mark it, we set registryPath to NULL */
        free(knownfolder->registryPath);
        knownfolder->registryPath = NULL;
        hr = S_OK;
    }

    return hr;
}

static HRESULT WINAPI knownfolder_GetId(
    IKnownFolder *iface,
    KNOWNFOLDERID *pkfid)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );

    TRACE("%p\n", pkfid);

    *pkfid = knownfolder->id;
    return S_OK;
}

static HRESULT WINAPI knownfolder_GetCategory(
    IKnownFolder *iface,
    KF_CATEGORY *pCategory)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p\n", knownfolder, pCategory);

    /* we cannot get a category for a folder which is not registered */
    if(!knownfolder->registryPath)
        hr = E_FAIL;

    if(SUCCEEDED(hr))
        hr = get_known_folder_dword(knownfolder->registryPath, L"Category", (DWORD *)pCategory);

    return hr;
}

static HRESULT WINAPI knownfolder_GetShellItem(
    IKnownFolder *iface,
    DWORD flags,
    REFIID riid,
    void **ppv)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder(iface);
    TRACE("(%p, 0x%08lx, %s, %p)\n", knownfolder, flags, debugstr_guid(riid), ppv);
    return SHGetKnownFolderItem(&knownfolder->id, flags, NULL, riid, ppv);
}

static HRESULT get_known_folder_path(
    LPWSTR sFolderId,
    LPWSTR registryPath,
    LPWSTR *ppszPath)
{
    HRESULT hr;
    DWORD dwSize, dwType;
    WCHAR path[MAX_PATH] = {0};
    WCHAR parentGuid[39];
    DWORD category;
    LPWSTR parentRegistryPath, parentPath;
    HKEY hRedirectionRootKey = NULL;

    TRACE("(%s, %p)\n", debugstr_w(registryPath), ppszPath);
    *ppszPath = NULL;

    /* check if folder has parent */
    dwSize = sizeof(parentGuid);
    hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, registryPath, L"ParentFolder", RRF_RT_REG_SZ, &dwType, parentGuid, &dwSize));
    if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) hr = S_FALSE;

    if(hr == S_OK)
    {
        /* get parent's known folder path (recursive) */
        hr = get_known_folder_registry_path(NULL, parentGuid, &parentRegistryPath);
        if(FAILED(hr)) return hr;

        hr = get_known_folder_path(parentGuid, parentRegistryPath, &parentPath);
        if(FAILED(hr)) {
            free(parentRegistryPath);
            return hr;
        }

        lstrcatW(path, parentPath);
        lstrcatW(path, L"\\");

        free(parentRegistryPath);
        free(parentPath);
    }

    /* check, if folder was redirected */
    if(SUCCEEDED(hr))
        hr = get_known_folder_dword(registryPath, L"Category", &category);

    if(SUCCEEDED(hr))
    {
        if(category == KF_CATEGORY_COMMON)
            hRedirectionRootKey = HKEY_LOCAL_MACHINE;
        else if(category == KF_CATEGORY_PERUSER)
            hRedirectionRootKey = HKEY_CURRENT_USER;

        if(hRedirectionRootKey)
        {
            hr = HRESULT_FROM_WIN32(RegGetValueW(hRedirectionRootKey, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", sFolderId, RRF_RT_REG_SZ, NULL, NULL, &dwSize));

            if(SUCCEEDED(hr))
            {
                *ppszPath = CoTaskMemAlloc(dwSize+(lstrlenW(path)+1)*sizeof(WCHAR));
                if(!*ppszPath) hr = E_OUTOFMEMORY;
            }

            if(SUCCEEDED(hr))
            {
                lstrcpyW(*ppszPath, path);
                hr = HRESULT_FROM_WIN32(RegGetValueW(hRedirectionRootKey, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", sFolderId, RRF_RT_REG_SZ, NULL, *ppszPath + lstrlenW(path), &dwSize));
            }
        }

        if(!*ppszPath)
        {
            /* no redirection, use previous way - read the relative path from folder definition */
            hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, registryPath, L"RelativePath", RRF_RT_REG_SZ, &dwType, NULL, &dwSize));

            if(SUCCEEDED(hr))
            {
                *ppszPath = CoTaskMemAlloc(dwSize+(lstrlenW(path)+1)*sizeof(WCHAR));
                if(!*ppszPath) hr = E_OUTOFMEMORY;
            }

            if(SUCCEEDED(hr))
            {
                lstrcpyW(*ppszPath, path);
                hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, registryPath, L"RelativePath", RRF_RT_REG_SZ, &dwType, *ppszPath + lstrlenW(path), &dwSize));
            }
        }
    }

    TRACE("returning path: %s\n", debugstr_w(*ppszPath));
    return hr;
}

static HRESULT get_known_folder_path_by_id(
    REFKNOWNFOLDERID folderId,
    LPWSTR lpRegistryPath,
    DWORD dwFlags,
    LPWSTR *ppszPath)
{
    HRESULT hr = E_FAIL;
    WCHAR sGuid[39];
    DWORD dwAttributes;

    TRACE("(%s, %s, 0x%08lx, %p)\n", debugstr_guid(folderId), debugstr_w(lpRegistryPath), dwFlags, ppszPath);

    /* if this is registry-registered known folder, get path from registry */
    if(lpRegistryPath)
    {
        StringFromGUID2(folderId, sGuid, ARRAY_SIZE(sGuid));

        hr = get_known_folder_path(sGuid, lpRegistryPath, ppszPath);
    }
    /* in other case, use older way */

    if(FAILED(hr))
        hr = SHGetKnownFolderPath( folderId, dwFlags, NULL, ppszPath );

    if (FAILED(hr)) return hr;

    /* check if known folder really exists */
    dwAttributes = GetFileAttributesW(*ppszPath);
    if(dwAttributes == INVALID_FILE_ATTRIBUTES || !(dwAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
        TRACE("directory %s not found\n", debugstr_w(*ppszPath));
        CoTaskMemFree(*ppszPath);
        *ppszPath = NULL;
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    return hr;
}

static HRESULT WINAPI knownfolder_GetPath(
    IKnownFolder *iface,
    DWORD dwFlags,
    LPWSTR *ppszPath)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    TRACE("(%p, 0x%08lx, %p)\n", knownfolder, dwFlags, ppszPath);

    return get_known_folder_path_by_id(&knownfolder->id, knownfolder->registryPath, dwFlags, ppszPath);
}

static HRESULT WINAPI knownfolder_SetPath(
    IKnownFolder *iface,
    DWORD dwFlags,
    LPCWSTR pszPath)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    HRESULT hr = S_OK;

    TRACE("(%p, 0x%08lx, %s)\n", knownfolder, dwFlags, debugstr_w(pszPath));

    /* check if the known folder is registered */
    if(!knownfolder->registryPath)
        hr = E_FAIL;

    if(SUCCEEDED(hr))
        hr = redirect_known_folder(&knownfolder->id, NULL, 0, pszPath, 0, NULL, NULL);

    return hr;
}

static HRESULT WINAPI knownfolder_GetIDList(
    IKnownFolder *iface,
    DWORD flags,
    PIDLIST_ABSOLUTE *ppidl)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    TRACE("(%p, 0x%08lx, %p)\n", knownfolder, flags, ppidl);
    return SHGetKnownFolderIDList(&knownfolder->id, flags, NULL, ppidl);
}

static HRESULT WINAPI knownfolder_GetFolderType(
    IKnownFolder *iface,
    FOLDERTYPEID *pftid)
{
    FIXME("%p\n", pftid);
    return E_NOTIMPL;
}

static HRESULT WINAPI knownfolder_GetRedirectionCapabilities(
    IKnownFolder *iface,
    KF_REDIRECTION_CAPABILITIES *pCapabilities)
{
    FIXME("%p stub\n", pCapabilities);
    if(!pCapabilities) return E_INVALIDARG;
    *pCapabilities = KF_REDIRECTION_CAPABILITIES_DENY_ALL;
    return S_OK;
}

static HRESULT WINAPI knownfolder_GetFolderDefinition(
    IKnownFolder *iface,
    KNOWNFOLDER_DEFINITION *pKFD)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    HRESULT hr;
    DWORD dwSize;
    WCHAR parentGuid[39];
    TRACE("(%p, %p)\n", knownfolder, pKFD);

    if(!pKFD) return E_INVALIDARG;

    ZeroMemory(pKFD, sizeof(*pKFD));

    /* required fields */
    hr = get_known_folder_dword(knownfolder->registryPath, L"Category", (DWORD *)&pKFD->category);
    if(FAILED(hr))
        return hr;

    hr = get_known_folder_wstr(knownfolder->registryPath, L"Name", &pKFD->pszName);
    if(FAILED(hr))
        return hr;

    /* optional fields */
    dwSize = sizeof(parentGuid);
    hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, knownfolder->registryPath, L"ParentFolder",
                RRF_RT_REG_SZ, NULL, parentGuid, &dwSize));
    if(SUCCEEDED(hr))
    {
        hr = IIDFromString(parentGuid, &pKFD->fidParent);
        if(FAILED(hr))
            return hr;
    }

    get_known_folder_dword(knownfolder->registryPath, L"Attributes", &pKFD->dwAttributes);

    get_known_folder_wstr(knownfolder->registryPath, L"RelativePath", &pKFD->pszRelativePath);

    get_known_folder_wstr(knownfolder->registryPath, L"ParsingName", &pKFD->pszParsingName);

    return S_OK;
}

static const struct IKnownFolderVtbl knownfolder_vtbl =
{
    knownfolder_QueryInterface,
    knownfolder_AddRef,
    knownfolder_Release,
    knownfolder_GetId,
    knownfolder_GetCategory,
    knownfolder_GetShellItem,
    knownfolder_GetPath,
    knownfolder_SetPath,
    knownfolder_GetIDList,
    knownfolder_GetFolderType,
    knownfolder_GetRedirectionCapabilities,
    knownfolder_GetFolderDefinition
};

static HRESULT knownfolder_create( struct knownfolder **knownfolder )
{
    struct knownfolder *kf;

    kf = malloc( sizeof(*kf) );
    if (!kf) return E_OUTOFMEMORY;

    kf->IKnownFolder_iface.lpVtbl = &knownfolder_vtbl;
    kf->refs = 1;
    memset( &kf->id, 0, sizeof(kf->id) );
    kf->registryPath = NULL;

    *knownfolder = kf;

    TRACE("returning iface %p\n", &kf->IKnownFolder_iface);
    return S_OK;
}

struct foldermanager
{
    IKnownFolderManager IKnownFolderManager_iface;
    LONG refs;
    UINT num_ids;
    KNOWNFOLDERID *ids;
};

static inline struct foldermanager *impl_from_IKnownFolderManager( IKnownFolderManager *iface )
{
    return CONTAINING_RECORD( iface, struct foldermanager, IKnownFolderManager_iface );
}

static ULONG WINAPI foldermanager_AddRef(
    IKnownFolderManager *iface )
{
    struct foldermanager *foldermanager = impl_from_IKnownFolderManager( iface );
    return InterlockedIncrement( &foldermanager->refs );
}

static ULONG WINAPI foldermanager_Release(
    IKnownFolderManager *iface )
{
    struct foldermanager *foldermanager = impl_from_IKnownFolderManager( iface );
    LONG refs = InterlockedDecrement( &foldermanager->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", foldermanager);
        free( foldermanager->ids );
        free( foldermanager );
    }
    return refs;
}

static HRESULT WINAPI foldermanager_QueryInterface(
    IKnownFolderManager *iface,
    REFIID riid,
    void **ppv )
{
    struct foldermanager *This = impl_from_IKnownFolderManager( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppv );

    *ppv = NULL;
    if ( IsEqualGUID( riid, &IID_IKnownFolderManager ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppv = iface;
    }
    else if ( IsEqualGUID( riid, &IID_IMarshal ) )
    {
        TRACE("IID_IMarshal returning NULL.\n");
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IKnownFolderManager_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI foldermanager_FolderIdFromCsidl(
    IKnownFolderManager *iface,
    int nCsidl,
    KNOWNFOLDERID *pfid)
{
    TRACE("%d, %p\n", nCsidl, pfid);

    if (nCsidl >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
    *pfid = *CSIDL_Data[nCsidl].id;
    return S_OK;
}

static HRESULT WINAPI foldermanager_FolderIdToCsidl(
    IKnownFolderManager *iface,
    REFKNOWNFOLDERID rfid,
    int *pnCsidl)
{
    int csidl;

    TRACE("%s, %p\n", debugstr_guid(rfid), pnCsidl);

    csidl = csidl_from_id( rfid );
    if (csidl == -1) return E_INVALIDARG;
    *pnCsidl = csidl;
    return S_OK;
}

static HRESULT WINAPI foldermanager_GetFolderIds(
    IKnownFolderManager *iface,
    KNOWNFOLDERID **ppKFId,
    UINT *pCount)
{
    struct foldermanager *fm = impl_from_IKnownFolderManager( iface );

    TRACE("%p, %p\n", ppKFId, pCount);

    *ppKFId = CoTaskMemAlloc(fm->num_ids * sizeof(KNOWNFOLDERID));
    memcpy(*ppKFId, fm->ids, fm->num_ids * sizeof(KNOWNFOLDERID));
    *pCount = fm->num_ids;
    return S_OK;
}

static BOOL is_knownfolder( struct foldermanager *fm, const KNOWNFOLDERID *id )
{
    UINT i;
    HRESULT hr;
    LPWSTR registryPath = NULL;
    HKEY hKey;

    /* TODO: move all entries from "CSIDL_Data" static array to registry known folder descriptions */
    for (i = 0; i < fm->num_ids; i++)
        if (IsEqualGUID( &fm->ids[i], id )) return TRUE;

    hr = get_known_folder_registry_path(id, NULL, &registryPath);
    if(SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegOpenKeyExW(HKEY_LOCAL_MACHINE, registryPath, 0, KEY_ENUMERATE_SUB_KEYS, &hKey));
        free(registryPath);
    }

    if(SUCCEEDED(hr))
    {
        hr = S_OK;
        RegCloseKey(hKey);
    }

    return hr == S_OK;
}

static HRESULT WINAPI foldermanager_GetFolder(
    IKnownFolderManager *iface,
    REFKNOWNFOLDERID rfid,
    IKnownFolder **ppkf)
{
    struct foldermanager *fm = impl_from_IKnownFolderManager( iface );
    struct knownfolder *kf;
    HRESULT hr;

    TRACE("%s, %p\n", debugstr_guid(rfid), ppkf);

    if (!is_knownfolder( fm, rfid ))
    {
        WARN("unknown folder\n");
        return E_INVALIDARG;
    }
    hr = knownfolder_create( &kf );
    if (SUCCEEDED( hr ))
    {
        hr = knownfolder_set_id( kf, rfid );
        *ppkf = &kf->IKnownFolder_iface;
    }
    else
        *ppkf = NULL;

    return hr;
}

static HRESULT WINAPI foldermanager_GetFolderByName(
    IKnownFolderManager *iface,
    LPCWSTR pszCanonicalName,
    IKnownFolder **ppkf)
{
    struct foldermanager *fm = impl_from_IKnownFolderManager( iface );
    struct knownfolder *kf;
    BOOL found = FALSE;
    HRESULT hr;
    UINT i;

    TRACE( "%s, %p\n", debugstr_w(pszCanonicalName), ppkf );

    for (i = 0; i < fm->num_ids; i++)
    {
        WCHAR *path, *name;
        hr = get_known_folder_registry_path( &fm->ids[i], NULL, &path );
        if (FAILED( hr )) return hr;

        hr = get_known_folder_wstr( path, L"Name", &name );
        free( path );
        if (FAILED( hr )) return hr;

        found = !wcsicmp( pszCanonicalName, name );
        CoTaskMemFree( name );
        if (found) break;
    }

    if (found)
    {
        hr = knownfolder_create( &kf );
        if (FAILED( hr )) return hr;

        hr = knownfolder_set_id( kf, &fm->ids[i] );
        if (FAILED( hr ))
        {
            IKnownFolder_Release( &kf->IKnownFolder_iface );
            return hr;
        }
        *ppkf = &kf->IKnownFolder_iface;
    }
    else
    {
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        *ppkf = NULL;
    }

    return hr;
}

static HRESULT register_folder(const KNOWNFOLDERID *rfid, const KNOWNFOLDER_DEFINITION *pKFD)
{
    HRESULT hr;
    HKEY hKey = NULL;
    DWORD dwDisp;
    LPWSTR registryPath = NULL;

    hr = get_known_folder_registry_path(rfid, NULL, &registryPath);
    TRACE("registry path: %s\n", debugstr_w(registryPath));

    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(RegCreateKeyExW(HKEY_LOCAL_MACHINE, registryPath, 0, NULL, 0, KEY_WRITE, 0, &hKey, &dwDisp));

    if(SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"Category", 0, REG_DWORD, (LPBYTE)&pKFD->category, sizeof(pKFD->category)));

        if(SUCCEEDED(hr) && pKFD->dwAttributes != 0)
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"Attributes", 0, REG_DWORD, (LPBYTE)&pKFD->dwAttributes, sizeof(pKFD->dwAttributes)));

        if(SUCCEEDED(hr))
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"Name", 0, REG_SZ, (LPBYTE)pKFD->pszName, (lstrlenW(pKFD->pszName)+1)*sizeof(WCHAR) ));

        if(SUCCEEDED(hr) && pKFD->pszParsingName)
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"ParsingName", 0, REG_SZ, (LPBYTE)pKFD->pszParsingName, (lstrlenW(pKFD->pszParsingName)+1)*sizeof(WCHAR) ));

        if(SUCCEEDED(hr) && !IsEqualGUID(&pKFD->fidParent, &GUID_NULL))
        {
            WCHAR sParentGuid[39];
            StringFromGUID2(&pKFD->fidParent, sParentGuid, ARRAY_SIZE(sParentGuid));

            /* this known folder has parent folder */
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"ParentFolder", 0, REG_SZ, (LPBYTE)sParentGuid, sizeof(sParentGuid)));
        }

        if(SUCCEEDED(hr) && pKFD->category != KF_CATEGORY_VIRTUAL && pKFD->pszRelativePath)
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"RelativePath", 0, REG_SZ, (LPBYTE)pKFD->pszRelativePath, (lstrlenW(pKFD->pszRelativePath)+1)*sizeof(WCHAR) ));

        RegCloseKey(hKey);

        if(FAILED(hr))
            SHDeleteKeyW(HKEY_LOCAL_MACHINE, registryPath);
    }

    free(registryPath);
    return hr;
}

static HRESULT WINAPI foldermanager_RegisterFolder(
    IKnownFolderManager *iface,
    REFKNOWNFOLDERID rfid,
    KNOWNFOLDER_DEFINITION const *pKFD)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(rfid), pKFD);
    return register_folder(rfid, pKFD);
}

static HRESULT WINAPI foldermanager_UnregisterFolder(
    IKnownFolderManager *iface,
    REFKNOWNFOLDERID rfid)
{
    HRESULT hr;
    LPWSTR registryPath = NULL;
    TRACE("(%p, %s)\n", iface, debugstr_guid(rfid));

    hr = get_known_folder_registry_path(rfid, NULL, &registryPath);

    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(SHDeleteKeyW(HKEY_LOCAL_MACHINE, registryPath));

    free(registryPath);
    return hr;
}

static HRESULT WINAPI foldermanager_FindFolderFromPath(
    IKnownFolderManager *iface,
    LPCWSTR pszPath,
    FFFP_MODE mode,
    IKnownFolder **ppkf)
{
    FIXME("%s, 0x%08x, %p\n", debugstr_w(pszPath), mode, ppkf);
    return E_NOTIMPL;
}

static HRESULT WINAPI foldermanager_FindFolderFromIDList(
    IKnownFolderManager *iface,
    PCIDLIST_ABSOLUTE pidl,
    IKnownFolder **ppkf)
{
    FIXME("%p, %p\n", pidl, ppkf);
    return E_NOTIMPL;
}

static HRESULT WINAPI foldermanager_Redirect(
    IKnownFolderManager *iface,
    REFKNOWNFOLDERID rfid,
    HWND hwnd,
    KF_REDIRECT_FLAGS flags,
    LPCWSTR pszTargetPath,
    UINT cFolders,
    KNOWNFOLDERID const *pExclusion,
    LPWSTR *ppszError)
{
    return redirect_known_folder(rfid, hwnd, flags, pszTargetPath, cFolders, pExclusion, ppszError);
}

static const struct IKnownFolderManagerVtbl foldermanager_vtbl =
{
    foldermanager_QueryInterface,
    foldermanager_AddRef,
    foldermanager_Release,
    foldermanager_FolderIdFromCsidl,
    foldermanager_FolderIdToCsidl,
    foldermanager_GetFolderIds,
    foldermanager_GetFolder,
    foldermanager_GetFolderByName,
    foldermanager_RegisterFolder,
    foldermanager_UnregisterFolder,
    foldermanager_FindFolderFromPath,
    foldermanager_FindFolderFromIDList,
    foldermanager_Redirect
};

static HRESULT foldermanager_create( void **ppv )
{
    UINT i, j;
    struct foldermanager *fm;

    fm = malloc( sizeof(*fm) );
    if (!fm) return E_OUTOFMEMORY;

    fm->IKnownFolderManager_iface.lpVtbl = &foldermanager_vtbl;
    fm->refs = 1;
    fm->num_ids = 0;

    for (i = 0; i < ARRAY_SIZE(CSIDL_Data); i++)
    {
        if (!IsEqualGUID( CSIDL_Data[i].id, &GUID_NULL )) fm->num_ids++;
    }
    fm->ids = malloc( fm->num_ids * sizeof(KNOWNFOLDERID) );
    if (!fm->ids)
    {
        free( fm );
        return E_OUTOFMEMORY;
    }
    for (i = j = 0; i < ARRAY_SIZE(CSIDL_Data); i++)
    {
        if (!IsEqualGUID( CSIDL_Data[i].id, &GUID_NULL ))
        {
            fm->ids[j] = *CSIDL_Data[i].id;
            j++;
        }
    }
    TRACE("found %u known folders\n", fm->num_ids);
    *ppv = &fm->IKnownFolderManager_iface;

    TRACE("returning iface %p\n", *ppv);
    return S_OK;
}

HRESULT WINAPI KnownFolderManager_Constructor( IUnknown *punk, REFIID riid, void **ppv )
{
    TRACE("%p, %s, %p\n", punk, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    if (punk)
        return CLASS_E_NOAGGREGATION;

    return foldermanager_create( ppv );
}

HRESULT WINAPI SHGetKnownFolderIDList(REFKNOWNFOLDERID rfid, DWORD flags, HANDLE token, PIDLIST_ABSOLUTE *pidl)
{
    TRACE("%s, 0x%08lx, %p, %p\n", debugstr_guid(rfid), flags, token, pidl);

    if (!pidl)
        return E_INVALIDARG;

    if (flags)
        FIXME("unsupported flags: 0x%08lx\n", flags);

    if (token)
        FIXME("user token is not used.\n");

    *pidl = NULL;
    if (IsEqualIID(rfid, &FOLDERID_Desktop))
        *pidl = _ILCreateDesktop();
    else if (IsEqualIID(rfid, &FOLDERID_RecycleBinFolder))
        *pidl = _ILCreateBitBucket();
    else if (IsEqualIID(rfid, &FOLDERID_ComputerFolder))
        *pidl = _ILCreateMyComputer();
    else if (IsEqualIID(rfid, &FOLDERID_PrintersFolder))
        *pidl = _ILCreatePrinters();
    else if (IsEqualIID(rfid, &FOLDERID_ControlPanelFolder))
        *pidl = _ILCreateControlPanel();
    else if (IsEqualIID(rfid, &FOLDERID_NetworkFolder))
        *pidl = _ILCreateNetwork();
    else if (IsEqualIID(rfid, &FOLDERID_Documents))
        *pidl = _ILCreateMyDocuments();
    else
    {
        DWORD attributes = 0;
        WCHAR *pathW;
        HRESULT hr;

        hr = SHGetKnownFolderPath(rfid, flags, token, &pathW);
        if (FAILED(hr))
            return hr;

        hr = SHILCreateFromPathW(pathW, pidl, &attributes);
        CoTaskMemFree(pathW);
        return hr;
    }

    return *pidl ? S_OK : E_FAIL;
}

HRESULT WINAPI SHGetKnownFolderItem(REFKNOWNFOLDERID rfid, KNOWN_FOLDER_FLAG flags, HANDLE hToken,
    REFIID riid, void **ppv)
{
    PIDLIST_ABSOLUTE pidl;
    HRESULT hr;

    TRACE("%s, 0x%08x, %p, %s, %p\n", debugstr_guid(rfid), flags, hToken, debugstr_guid(riid), ppv);

    hr = SHGetKnownFolderIDList(rfid, flags, hToken, &pidl);
    if (FAILED(hr))
    {
        *ppv = NULL;
        return hr;
    }

    hr = SHCreateItemFromIDList(pidl, riid, ppv);
    CoTaskMemFree(pidl);
    return hr;
}

static void register_system_knownfolders(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(CSIDL_Data); ++i)
    {
        const CSIDL_DATA *folder = &CSIDL_Data[i];
        if(folder->name){
            KNOWNFOLDER_DEFINITION kfd;

            /* register_folder won't modify kfd, so cast away const instead of
             * reallocating */
            kfd.category         = folder->category;
            kfd.pszName          = (WCHAR *)folder->name;
            kfd.pszDescription   = NULL;
            kfd.fidParent        = folder->parent ? *folder->parent : GUID_NULL;
            kfd.pszRelativePath  = (WCHAR *)folder->path;
            kfd.pszParsingName   = (WCHAR *)folder->parsing;
            kfd.pszTooltip       = NULL;
            kfd.pszLocalizedName = NULL;
            kfd.pszIcon          = NULL;
            kfd.pszSecurity      = NULL;
            kfd.dwAttributes     = folder->attributes;
            kfd.kfdFlags         = folder->flags;
            kfd.ftidType         = folder->typeid ? *folder->typeid : GUID_NULL;
            register_folder(folder->id, &kfd);
        }
    }
}

HRESULT SHELL_RegisterShellFolders(void)
{
    HRESULT hr;

    hr = _SHRegisterUserShellFolders(TRUE);
    if (SUCCEEDED(hr))
        hr = _SHRegisterUserShellFolders(FALSE);
    if (SUCCEEDED(hr))
        hr = _SHRegisterCommonShellFolders();
#ifndef __REACTOS__
    if (SUCCEEDED(hr))
        hr = create_extra_folders();
#endif
    if (SUCCEEDED(hr))
        hr = set_folder_attributes();
    if (SUCCEEDED(hr))
        register_system_knownfolders();
    return hr;
}
