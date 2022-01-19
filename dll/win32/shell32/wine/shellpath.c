/*
 * Path Functions
 *
 * Copyright 1998, 1999, 2000 Juergen Schmied
 * Copyright 2004 Juan Lang
 * Copyright 2018-2021 Katayama Hirofumi MZ
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

#include <shlwapi_undoc.h>
#include <shellutils.h>

#include <userenv.h>

#include "pidl.h"
#include "shell32_main.h"
#include "shresdef.h"

#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WS03

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static const BOOL is_win64 = sizeof(void *) > sizeof(int);

#ifdef __REACTOS__

/* FIXME: Remove this */
typedef enum _NT_PRODUCT_TYPE
{
    NtProductWinNt = 1,
    NtProductLanManNt,
    NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

/* FIXME: We cannot refresh the RtlGetNtProductType value before reboot. */
static BOOL
DoGetProductType(PNT_PRODUCT_TYPE ProductType)
{
    HKEY hKey;
    LONG error;
    WCHAR szValue[9];
    DWORD cbValue;
    static DWORD s_dwProductType = 0;

    if (s_dwProductType != 0)
    {
        *ProductType = s_dwProductType;
        return TRUE;
    }

    *ProductType = NtProductServer;

    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions", 0, KEY_READ, &hKey);
    if (error)
        return FALSE;

    cbValue = sizeof(szValue);
    error = RegGetValueW(hKey, NULL, L"ProductType", RRF_RT_REG_SZ, NULL, (PVOID)szValue, &cbValue);
    if (!error)
    {
        if (lstrcmpW(szValue, L"WinNT") == 0)
            *ProductType = NtProductWinNt;
        else if (lstrcmpW(szValue, L"LanmanNT") == 0)
            *ProductType = NtProductLanManNt;
    }

    s_dwProductType = *ProductType;

    RegCloseKey(hKey);
    return TRUE;
}

#endif // __REACTOS__

/*
	########## Combining and Constructing paths ##########
*/

/* @implemented */
static BOOL WINAPI
PathSearchOnExtensionsW(LPWSTR pszPath, LPCWSTR *ppszDirs, BOOL bDoSearch, DWORD dwWhich)
{
    if (*PathFindExtensionW(pszPath) != 0)
        return FALSE;

    if (bDoSearch)
        return PathFindOnPathExW(pszPath, ppszDirs, dwWhich);
    else
        return PathFileExistsDefExtW(pszPath, dwWhich);
}

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
/* @implemented */
static BOOL WINAPI PathIsAbsoluteW(LPCWSTR path)
{
    return PathIsUNCW(path) || (PathGetDriveNumberW(path) != -1 && path[2] == L'\\');
}

/* @implemented */
static BOOL WINAPI PathMakeAbsoluteW(LPWSTR path)
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

/* NOTE: GetShortPathName fails if the pathname didn't exist.
         GetShortPathNameAbsentW should set the short path name that even doesn't exist. */
static DWORD GetShortPathNameAbsentW(LPCWSTR pszLong, LPWSTR pszShort, DWORD cchShort)
{
    FIXME("GetShortPathNameAbsentW(%ls, %p, %ld): stub\n", pszLong, pszShort, cchShort);
    StringCchCopyW(pszShort, cchShort, pszLong);
    return lstrlenW(pszShort);
}

BOOL WINAPI IsLFNDriveW(LPCWSTR lpszPath);

/* @unconfirmed */
static VOID WINAPI PathQualifyExW(LPWSTR pszPath, LPCWSTR pszDir, DWORD dwFlags)
{
    WCHAR szRoot[MAX_PATH], szCopy[MAX_PATH], szCurDir[MAX_PATH];
    LPWSTR pch;
    LONG cch;
    BOOL bCheckLFN;

    if (FAILED(StringCchCopyW(szCopy, _countof(szCopy), pszPath)))
        return;

    FixSlashesAndColonW(szCopy);

    if (pszDir)
    {
        cch = GetCurrentDirectoryW(_countof(szCurDir), szCurDir);
        if (cch <= 0 || cch >= _countof(szCurDir) || !SetCurrentDirectoryW(pszDir))
            pszDir = NULL;
    }

    if (!GetFullPathNameW(szCopy, _countof(szRoot), szRoot, NULL))
        goto Quit;

    if (PathIsUNCW(szRoot)) /* it begins with double backslash */
    {
        pch = StrChrW(&szRoot[2], L'\\');
        if (pch)
        {
            pch = StrChrW(&pch[1], L'\\');
            if (pch)
                *pch = 0;
            if (!PathAddBackslashW(szRoot))
                goto Quit;
            /* szRoot is like \\MyServer\MyShare\ */
            bCheckLFN = TRUE;
        }
        else
        {
            bCheckLFN = FALSE;
        }
    }
    else
    {
        if (!PathStripToRootW(szRoot) || !PathAddBackslashW(szRoot))
            goto Quit;
        /* szRoot is like X:\ */
        bCheckLFN = TRUE;
    }

    if (bCheckLFN && !IsLFNDriveW(szRoot)) /* not a long filename drive */
    {
        if (!GetFullPathNameW(szCopy, _countof(szRoot), szRoot, NULL))
            goto Quit;
        if (!GetShortPathNameW(szRoot, szCopy, _countof(szCopy)) &&
            !GetShortPathNameAbsentW(szRoot, szCopy, _countof(szCopy)))
        {
            goto Quit;
        }
    }

    PathRemoveBackslashW(szCopy);
    StringCchCopyW(pszPath, MAX_PATH, szCopy);

    if ((dwFlags & 1) == 0)
    {
        cch = lstrlenW(pszPath);
        if (cch > 0 && pszPath[cch - 1] == L'.')
            pszPath[cch - 1] = 0;
    }

Quit:
    if (pszDir)
        SetCurrentDirectoryW(szCurDir);
}

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
 * SHPathGetExtension        [SHELL32.158]
 */
LPVOID WINAPI SHPathGetExtensionW(LPCWSTR lpszPath, DWORD void1, DWORD void2)
{
    return PathGetExtensionW(lpszPath);
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

/*
	########## Path Testing ##########
*/

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
BOOL PathIsExeW (LPCWSTR lpszPath)
{
	LPCWSTR lpszExtension = PathGetExtensionW(lpszPath);
        int i;
        static const WCHAR lpszExtensions[][4] =
            {L"exe", L"com", L"pif", L"cmd", L"bat", L"scf", L"scr", L"" };

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
 * PathFileExists	[SHELL32.45]
 */
BOOL WINAPI PathFileExistsAW (LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathFileExistsW (lpszPath);
	return PathFileExistsA (lpszPath);
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
	FIXME("%p %u %s %s %s stub\n",
	 lpszBuffer, dwBuffSize, debugstr_a(lpszShortName),
	 debugstr_a(lpszLongName), debugstr_a(lpszPathName));
	return TRUE;
}

/*************************************************************************
 * PathMakeUniqueNameW	[internal]
 */
static BOOL PathMakeUniqueNameW(
	LPWSTR lpszBuffer,
	DWORD dwBuffSize,
	LPCWSTR lpszShortName,
	LPCWSTR lpszLongName,
	LPCWSTR lpszPathName)
{
	FIXME("%p %u %s %s %s stub\n",
	 lpszBuffer, dwBuffSize, debugstr_w(lpszShortName),
	 debugstr_w(lpszLongName), debugstr_w(lpszPathName));
	return TRUE;
}

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
    strcpyW(retW, pathW);
    PathRemoveExtensionW(pathW);

    ext = PathFindExtensionW(file);

    /* now try to make it unique */
    while (PathFileExistsW(retW))
    {
        sprintfW(retW, L"%s (%d)%s", pathW, i, ext);
        i++;
    }

    strcpyW(buffer, retW);
    TRACE("ret - %s\n", debugstr_w(buffer));

    return TRUE;
}

/*
	########## cleaning and resolving paths ##########
 */

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
            length = strlenW(lpszPathW);

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

/*************************************************************************
 * PathQualifyA		[SHELL32]
 */
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

    TRACE("PathResolveA(%s,%p,0x%08x)\n", debugstr_a(path), dirs, flags);

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

BOOL WINAPI PathResolveW(LPWSTR path, LPCWSTR *dirs, DWORD flags)
{
    DWORD dwWhich = ((flags & PRF_DONTFINDLNK) ? (WHICH_DEFAULT & ~WHICH_LNK) : WHICH_DEFAULT);

    TRACE("PathResolveW(%s,%p,0x%08x)\n", debugstr_w(path), dirs, flags);

    if (flags & PRF_VERIFYEXISTS)
        SetLastError(ERROR_FILE_NOT_FOUND);

    PathUnquoteSpacesW(path);

    if (PathIsRootW(path))
    {
        if ((path[0] == L'\\' && path[1] == 0) ||
            PathIsUNCServerW(path) || PathIsUNCServerShareW(path))
        {
            if (flags & PRF_FIRSTDIRDEF)
                PathQualifyExW(path, dirs[0], 0);
            else
                PathQualifyExW(path, NULL, 0);
        }

        if (flags & PRF_VERIFYEXISTS)
            return PathFileExistsAndAttributesW(path, NULL);
        return TRUE;
    }
    else if (PathIsFileSpecW(path))
    {
        if ((flags & PRF_TRYPROGRAMEXTENSIONS) && PathSearchOnExtensionsW(path, dirs, TRUE, dwWhich))
            return TRUE;

        if (PathFindOnPathW(path, dirs))
        {
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
            if (!(flags & PRF_REQUIREABSOLUTE))
                return TRUE;

            if (!PathIsAbsoluteW(path))
                return PathMakeAbsoluteW(path) && PathFileExistsAndAttributesW(path, NULL);
#else
            return TRUE;
#endif
        }
    }
    else if (!PathIsURLW(path))
    {
        if (flags & PRF_FIRSTDIRDEF)
            PathQualifyExW(path, *dirs, 1);
        else
            PathQualifyExW(path, NULL, 1);

        if (flags & PRF_VERIFYEXISTS)
        {
            if ((flags & PRF_TRYPROGRAMEXTENSIONS) &&
                PathSearchOnExtensionsW(path, dirs, FALSE, dwWhich))
            {
                return TRUE;
            }
            else if (!PathFileExistsAndAttributesW(path, NULL))
            {
                return FALSE;
            }
        }

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
        if (flags & PRF_REQUIREABSOLUTE)
        {
            if (!PathIsAbsoluteW(path))
                return PathMakeAbsoluteW(path) && PathFileExistsAndAttributesW(path, NULL);
        }
#endif
        return TRUE;
    }

    return FALSE;
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

/*************************************************************************
*	PathProcessCommandA
*/
static LONG PathProcessCommandA (
	LPCSTR lpszPath,
	LPSTR lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	FIXME("%s %p 0x%04x 0x%04x stub\n",
	lpszPath, lpszBuff, dwBuffSize, dwFlags);
	if(!lpszPath) return -1;
	if(lpszBuff) strcpy(lpszBuff, lpszPath);
	return strlen(lpszPath);
}

/*************************************************************************
*	PathProcessCommandW
*/
static LONG PathProcessCommandW (
	LPCWSTR lpszPath,
	LPWSTR lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	FIXME("(%s, %p, 0x%04x, 0x%04x) stub\n",
	debugstr_w(lpszPath), lpszBuff, dwBuffSize, dwFlags);
	if(!lpszPath) return -1;
	if(lpszBuff) strcpyW(lpszBuff, lpszPath);
	return strlenW(lpszPath);
}

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

/* !! MISSING Win2k3-compatible paths from the list below; absent from Wine !! */
#ifndef __REACTOS__
static const WCHAR Application_DataW[] = L"Application Data";
static const WCHAR Local_Settings_Application_DataW[] = L"Local Settings\\Application Data";
static const WCHAR Local_Settings_HistoryW[] = L"Local Settings\\History";
static const WCHAR Local_Settings_Temporary_Internet_FilesW[] = L"Local Settings\\Temporary Internet Files";
static const WCHAR MusicW[] = L"Music";
static const WCHAR PicturesW[] = L"Pictures";
static const WCHAR Program_FilesW[] = L"Program Files";
static const WCHAR Program_Files_Common_FilesW[] = L"Program Files\\Common Files";
static const WCHAR Start_Menu_ProgramsW[] = L"Start Menu\\Programs";
static const WCHAR Start_Menu_Admin_ToolsW[] = L"Start Menu\\Programs\\Administrative Tools";
static const WCHAR Start_Menu_StartupW[] = L"Start Menu\\Programs\\StartUp";
#endif

/* Long strings that are repeated many times: keep them here */
static const WCHAR szSHFolders[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
static const WCHAR szSHUserFolders[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";
#ifndef __REACTOS__
static const WCHAR szKnownFolderDescriptions[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FolderDescriptions";
static const WCHAR szKnownFolderRedirections[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";
#endif

typedef enum _CSIDL_Type {
    CSIDL_Type_User,
#ifdef __REACTOS__
    CSIDL_Type_InMyDocuments,
#endif
    CSIDL_Type_AllUsers,
    CSIDL_Type_CurrVer,
    CSIDL_Type_Disallowed,
    CSIDL_Type_NonExistent,
    CSIDL_Type_WindowsPath,
    CSIDL_Type_SystemPath,
    CSIDL_Type_SystemX86Path,
} CSIDL_Type;

/* Cannot use #if _WIN32_WINNT >= 0x0600 because _WIN32_WINNT == 0x0600 here. */
#ifndef __REACTOS__
#define CSIDL_CONTACTS         0x0043
#define CSIDL_DOWNLOADS        0x0047
#define CSIDL_LINKS            0x004d
#define CSIDL_APPDATA_LOCALLOW 0x004e
#define CSIDL_SAVED_GAMES      0x0062
#define CSIDL_SEARCHES         0x0063
#endif

typedef struct
{
    const KNOWNFOLDERID *id;
    CSIDL_Type type;
    LPCWSTR    szValueName;
    LPCWSTR    szDefaultPath; /* fallback string or resource ID */
    INT        nShell32IconIndex;
} CSIDL_DATA;

static const CSIDL_DATA CSIDL_Data[] =
{
    { /* 0x00 - CSIDL_DESKTOP */
        &FOLDERID_Desktop,
        CSIDL_Type_User,
        L"Desktop",
        MAKEINTRESOURCEW(IDS_DESKTOPDIRECTORY),
#ifdef __REACTOS__
        0
#else
        -IDI_SHELL_DESKTOP
#endif
    },
    { /* 0x01 - CSIDL_INTERNET */
        &FOLDERID_InternetFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x02 - CSIDL_PROGRAMS */
        &FOLDERID_Programs,
        CSIDL_Type_User,
        L"Programs",
        MAKEINTRESOURCEW(IDS_PROGRAMS),
#ifdef __REACTOS__
        0
#else
        -IDI_SHELL_PROGRAMS_FOLDER
#endif
    },
    { /* 0x03 - CSIDL_CONTROLS (.CPL files) */
        &FOLDERID_ControlPanelFolder,
        CSIDL_Type_SystemPath,
        NULL,
        NULL,
        -IDI_SHELL_CONTROL_PANEL
    },
    { /* 0x04 - CSIDL_PRINTERS */
        &FOLDERID_PrintersFolder,
        CSIDL_Type_SystemPath,
        NULL,
        NULL,
        -IDI_SHELL_PRINTERS_FOLDER
    },
    { /* 0x05 - CSIDL_PERSONAL */
        &FOLDERID_Documents,
        CSIDL_Type_User,
        L"Personal",
        MAKEINTRESOURCEW(IDS_PERSONAL),
        -IDI_SHELL_MY_DOCUMENTS
    },
    { /* 0x06 - CSIDL_FAVORITES */
        &FOLDERID_Favorites,
        CSIDL_Type_User,
        L"Favorites",
        MAKEINTRESOURCEW(IDS_FAVORITES),
        -IDI_SHELL_FAVORITES
    },
    { /* 0x07 - CSIDL_STARTUP */
        &FOLDERID_Startup,
        CSIDL_Type_User,
        L"StartUp",
        MAKEINTRESOURCEW(IDS_STARTUP)
    },
    { /* 0x08 - CSIDL_RECENT */
        &FOLDERID_Recent,
        CSIDL_Type_User,
        L"Recent",
        MAKEINTRESOURCEW(IDS_RECENT),
        -IDI_SHELL_RECENT_DOCUMENTS
    },
    { /* 0x09 - CSIDL_SENDTO */
        &FOLDERID_SendTo,
        CSIDL_Type_User,
        L"SendTo",
        MAKEINTRESOURCEW(IDS_SENDTO)
    },
    { /* 0x0a - CSIDL_BITBUCKET - Recycle Bin */
        &FOLDERID_RecycleBinFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x0b - CSIDL_STARTMENU */
        &FOLDERID_StartMenu,
        CSIDL_Type_User,
        L"Start Menu",
        MAKEINTRESOURCEW(IDS_STARTMENU),
        -IDI_SHELL_TSKBAR_STARTMENU
    },
    { /* 0x0c - CSIDL_MYDOCUMENTS */
        &GUID_NULL,
        CSIDL_Type_Disallowed, /* matches WinXP--can't get its path */
        NULL,
        NULL,
        -IDI_SHELL_MY_DOCUMENTS
    },
    { /* 0x0d - CSIDL_MYMUSIC */
        &FOLDERID_Music,
#ifdef __REACTOS__
        CSIDL_Type_InMyDocuments,
#else
        CSIDL_Type_User,
#endif
        L"My Music",
        MAKEINTRESOURCEW(IDS_MYMUSIC),
        -IDI_SHELL_MY_MUSIC
    },
    { /* 0x0e - CSIDL_MYVIDEO */
        &FOLDERID_Videos,
#ifdef __REACTOS__
        CSIDL_Type_InMyDocuments,
#else
        CSIDL_Type_User,
#endif
        L"My Video",
        MAKEINTRESOURCEW(IDS_MYVIDEO),
        -IDI_SHELL_MY_MOVIES
    },
    { /* 0x0f - unassigned */
        &GUID_NULL,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,
    },
    { /* 0x10 - CSIDL_DESKTOPDIRECTORY */
        &FOLDERID_Desktop,
        CSIDL_Type_User,
        L"Desktop",
        MAKEINTRESOURCEW(IDS_DESKTOPDIRECTORY),
#ifdef __REACTOS__
        0
#else
        -IDI_SHELL_DESKTOP
#endif
    },
    { /* 0x11 - CSIDL_DRIVES */
        &FOLDERID_ComputerFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,
        -IDI_SHELL_COMPUTER_FOLDER
    },
    { /* 0x12 - CSIDL_NETWORK */
        &FOLDERID_NetworkFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,
        -IDI_SHELL_NETWORK_FOLDER
    },
    { /* 0x13 - CSIDL_NETHOOD */
        &FOLDERID_NetHood,
        CSIDL_Type_User,
        L"NetHood",
        MAKEINTRESOURCEW(IDS_NETHOOD),
        -IDI_SHELL_NETWORK
    },
    { /* 0x14 - CSIDL_FONTS */
        &FOLDERID_Fonts,
        CSIDL_Type_WindowsPath,
        L"Fonts",
        L"Fonts",
        -IDI_SHELL_FONTS_FOLDER
    },
    { /* 0x15 - CSIDL_TEMPLATES */
        &FOLDERID_Templates,
        CSIDL_Type_User,
        L"Templates",
        MAKEINTRESOURCEW(IDS_TEMPLATES)
    },
    { /* 0x16 - CSIDL_COMMON_STARTMENU */
        &FOLDERID_CommonStartMenu,
        CSIDL_Type_AllUsers,
        L"Common Start Menu",
        MAKEINTRESOURCEW(IDS_STARTMENU),
        -IDI_SHELL_TSKBAR_STARTMENU
    },
    { /* 0x17 - CSIDL_COMMON_PROGRAMS */
        &FOLDERID_CommonPrograms,
        CSIDL_Type_AllUsers,
        L"Common Programs",
        MAKEINTRESOURCEW(IDS_PROGRAMS),
#ifdef __REACTOS__
        0
#else
        -IDI_SHELL_PROGRAMS_FOLDER
#endif
    },
    { /* 0x18 - CSIDL_COMMON_STARTUP */
        &FOLDERID_CommonStartup,
        CSIDL_Type_AllUsers,
        L"Common StartUp",
        MAKEINTRESOURCEW(IDS_STARTUP)
    },
    { /* 0x19 - CSIDL_COMMON_DESKTOPDIRECTORY */
        &FOLDERID_PublicDesktop,
        CSIDL_Type_AllUsers,
        L"Common Desktop",
        MAKEINTRESOURCEW(IDS_DESKTOPDIRECTORY),
#ifdef __REACTOS__
        0
#else
        -IDI_SHELL_DESKTOP
#endif
    },
    { /* 0x1a - CSIDL_APPDATA */
        &FOLDERID_RoamingAppData,
        CSIDL_Type_User,
        L"AppData",
        MAKEINTRESOURCEW(IDS_APPDATA)
    },
    { /* 0x1b - CSIDL_PRINTHOOD */
        &FOLDERID_PrintHood,
        CSIDL_Type_User,
        L"PrintHood",
        MAKEINTRESOURCEW(IDS_PRINTHOOD),
        -IDI_SHELL_PRINTERS_FOLDER
    },
    { /* 0x1c - CSIDL_LOCAL_APPDATA */
        &FOLDERID_LocalAppData,
        CSIDL_Type_User,
        L"Local AppData",
        MAKEINTRESOURCEW(IDS_LOCAL_APPDATA)
    },
    { /* 0x1d - CSIDL_ALTSTARTUP */
        &GUID_NULL,
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x1e - CSIDL_COMMON_ALTSTARTUP */
        &GUID_NULL,
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x1f - CSIDL_COMMON_FAVORITES */
        &FOLDERID_Favorites,
        CSIDL_Type_AllUsers,
        L"Common Favorites",
        MAKEINTRESOURCEW(IDS_FAVORITES),
        -IDI_SHELL_FAVORITES
    },
    { /* 0x20 - CSIDL_INTERNET_CACHE */
        &FOLDERID_InternetCache,
        CSIDL_Type_User,
        L"Cache",
        MAKEINTRESOURCEW(IDS_INTERNET_CACHE)
    },
    { /* 0x21 - CSIDL_COOKIES */
        &FOLDERID_Cookies,
        CSIDL_Type_User,
        L"Cookies",
        MAKEINTRESOURCEW(IDS_COOKIES)
    },
    { /* 0x22 - CSIDL_HISTORY */
        &FOLDERID_History,
        CSIDL_Type_User,
        L"History",
        MAKEINTRESOURCEW(IDS_HISTORY)
    },
    { /* 0x23 - CSIDL_COMMON_APPDATA */
        &FOLDERID_ProgramData,
        CSIDL_Type_AllUsers,
        L"Common AppData",
        MAKEINTRESOURCEW(IDS_APPDATA)
    },
    { /* 0x24 - CSIDL_WINDOWS */
        &FOLDERID_Windows,
        CSIDL_Type_WindowsPath,
        NULL,
        NULL,
        -IDI_SHELL_SYSTEM_GEAR
    },
    { /* 0x25 - CSIDL_SYSTEM */
        &FOLDERID_System,
        CSIDL_Type_SystemPath,
        NULL,
        NULL,
        -IDI_SHELL_SYSTEM_GEAR
    },
    { /* 0x26 - CSIDL_PROGRAM_FILES */
        &FOLDERID_ProgramFiles,
        CSIDL_Type_CurrVer,
        L"ProgramFilesDir",
        MAKEINTRESOURCEW(IDS_PROGRAM_FILES),
#ifdef __REACTOS__
        0
#else
        -IDI_SHELL_PROGRAMS_FOLDER
#endif
    },
    { /* 0x27 - CSIDL_MYPICTURES */
        &FOLDERID_Pictures,
#ifdef __REACTOS__
        CSIDL_Type_InMyDocuments,
#else
        CSIDL_Type_User,
#endif
        L"My Pictures",
        MAKEINTRESOURCEW(IDS_MYPICTURES),
        -IDI_SHELL_MY_PICTURES
    },
    { /* 0x28 - CSIDL_PROFILE */
        &FOLDERID_Profile,
        CSIDL_Type_User,
        NULL,
        NULL
    },
    { /* 0x29 - CSIDL_SYSTEMX86 */
        &FOLDERID_SystemX86,
        CSIDL_Type_SystemX86Path,
        NULL,
        NULL,
        -IDI_SHELL_SYSTEM_GEAR
    },
    { /* 0x2a - CSIDL_PROGRAM_FILESX86 */
        &FOLDERID_ProgramFilesX86,
        CSIDL_Type_CurrVer,
        L"ProgramFilesDir (x86)",
        L"Program Files (x86)",
        -IDI_SHELL_PROGRAMS_FOLDER
    },
    { /* 0x2b - CSIDL_PROGRAM_FILES_COMMON */
        &FOLDERID_ProgramFilesCommon,
        CSIDL_Type_CurrVer,
        L"CommonFilesDir",
        MAKEINTRESOURCEW(IDS_PROGRAM_FILES_COMMON),
        -IDI_SHELL_PROGRAMS_FOLDER
    },
    { /* 0x2c - CSIDL_PROGRAM_FILES_COMMONX86 */
        &FOLDERID_ProgramFilesCommonX86,
        CSIDL_Type_CurrVer,
        L"CommonFilesDir (x86)",
        L"Program Files (x86)\\Common Files",
        -IDI_SHELL_PROGRAMS_FOLDER
    },
    { /* 0x2d - CSIDL_COMMON_TEMPLATES */
        &FOLDERID_CommonTemplates,
        CSIDL_Type_AllUsers,
        L"Common Templates",
        MAKEINTRESOURCEW(IDS_TEMPLATES)
    },
    { /* 0x2e - CSIDL_COMMON_DOCUMENTS */
        &FOLDERID_PublicDocuments,
        CSIDL_Type_AllUsers,
        L"Common Documents",
        MAKEINTRESOURCEW(IDS_PERSONAL),
        -IDI_SHELL_MY_DOCUMENTS
    },
    { /* 0x2f - CSIDL_COMMON_ADMINTOOLS */
        &FOLDERID_CommonAdminTools,
        CSIDL_Type_AllUsers,
        L"Common Administrative Tools",
        MAKEINTRESOURCEW(IDS_ADMINTOOLS)
    },
    { /* 0x30 - CSIDL_ADMINTOOLS */
        &FOLDERID_AdminTools,
        CSIDL_Type_User,
        L"Administrative Tools",
        MAKEINTRESOURCEW(IDS_ADMINTOOLS)
    },
    { /* 0x31 - CSIDL_CONNECTIONS */
        &FOLDERID_ConnectionsFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,
        -IDI_SHELL_NETWORK_CONNECTIONS
    },
    { /* 0x32 - unassigned */
        &GUID_NULL,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x33 - unassigned */
        &GUID_NULL,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x34 - unassigned */
        &GUID_NULL,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x35 - CSIDL_COMMON_MUSIC */
        &FOLDERID_PublicMusic,
        CSIDL_Type_AllUsers,
        L"CommonMusic",
        MAKEINTRESOURCEW(IDS_COMMON_MUSIC),
        -IDI_SHELL_MY_MUSIC
    },
    { /* 0x36 - CSIDL_COMMON_PICTURES */
        &FOLDERID_PublicPictures,
        CSIDL_Type_AllUsers,
        L"CommonPictures",
        MAKEINTRESOURCEW(IDS_COMMON_PICTURES),
        -IDI_SHELL_MY_PICTURES
    },
    { /* 0x37 - CSIDL_COMMON_VIDEO */
        &FOLDERID_PublicVideos,
        CSIDL_Type_AllUsers,
        L"CommonVideo",
        MAKEINTRESOURCEW(IDS_COMMON_VIDEO),
        -IDI_SHELL_MY_MOVIES
    },
    { /* 0x38 - CSIDL_RESOURCES */
        &FOLDERID_ResourceDir,
        CSIDL_Type_WindowsPath,
        NULL,
        L"Resources"
    },
    { /* 0x39 - CSIDL_RESOURCES_LOCALIZED */
        &FOLDERID_LocalizedResourcesDir,
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x3a - CSIDL_COMMON_OEM_LINKS */
        &FOLDERID_CommonOEMLinks,
        CSIDL_Type_AllUsers,
        NULL,
        L"OEM Links"
    },
    { /* 0x3b - CSIDL_CDBURN_AREA */
        &FOLDERID_CDBurning,
        CSIDL_Type_User,
        L"CD Burning",
        L"Local Settings\\Application Data\\Microsoft\\CD Burning"
    },
    { /* 0x3c unassigned */
        &GUID_NULL,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x3d - CSIDL_COMPUTERSNEARME */
        &GUID_NULL,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x3e - CSIDL_PROFILES */
        &GUID_NULL,
        CSIDL_Type_Disallowed, /* oddly, this matches WinXP */
        NULL,
        NULL
    },
/* Cannot use #if _WIN32_WINNT >= 0x0600 because _WIN32_WINNT == 0x0600 here. */
#ifndef __REACTOS__
    { /* 0x3f */
        &FOLDERID_AddNewPrograms,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x40 */
        &FOLDERID_AppUpdates,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x41 */
        &FOLDERID_ChangeRemovePrograms,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x42 */
        &FOLDERID_ConflictFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x43 - CSIDL_CONTACTS */
        &FOLDERID_Contacts,
        CSIDL_Type_User,
        NULL,
        L"Contacts"
    },
    { /* 0x44 */
        &FOLDERID_DeviceMetadataStore,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x45 */
        &GUID_NULL,
        CSIDL_Type_User,
        NULL,
        L"Documents"
    },
    { /* 0x46 */
        &FOLDERID_DocumentsLibrary,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x47 - CSIDL_DOWNLOADS */
        &FOLDERID_Downloads,
#ifdef __REACTOS__
        CSIDL_Type_InMyDocuments,
#else
        CSIDL_Type_User,
#endif
        NULL,
        L"Downloads"
    },
    { /* 0x48 */
        &FOLDERID_Games,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x49 */
        &FOLDERID_GameTasks,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x4a */
        &FOLDERID_HomeGroup,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x4b */
        &FOLDERID_ImplicitAppShortcuts,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x4c */
        &FOLDERID_Libraries,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x4d - CSIDL_LINKS */
        &FOLDERID_Links,
        CSIDL_Type_User,
        NULL,
        L"Links"
    },
    { /* 0x4e - CSIDL_APPDATA_LOCALLOW */
        &FOLDERID_LocalAppDataLow,
        CSIDL_Type_User,
        NULL,
        L"AppData\\LocalLow"
    },
    { /* 0x4f */
        &FOLDERID_MusicLibrary,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x50 */
        &FOLDERID_OriginalImages,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x51 */
        &FOLDERID_PhotoAlbums,
        CSIDL_Type_User,
        NULL,
        L"Pictures\\Slide Shows"
    },
    { /* 0x52 */
        &FOLDERID_PicturesLibrary,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x53 */
        &FOLDERID_Playlists,
        CSIDL_Type_User,
        NULL,
        L"Music\\Playlists"
    },
    { /* 0x54 */
        &FOLDERID_ProgramFilesX64,
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x55 */
        &FOLDERID_ProgramFilesCommonX64,
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x56 */
        &FOLDERID_Public,
        CSIDL_Type_CurrVer, /* FIXME */
        NULL,
        L"Users\\Public"
    },
    { /* 0x57 */
        &FOLDERID_PublicDownloads,
        CSIDL_Type_AllUsers,
        NULL,
        L"Downloads"
    },
    { /* 0x58 */
        &FOLDERID_PublicGameTasks,
        CSIDL_Type_AllUsers,
        NULL,
        L"Microsoft\\Windows\\GameExplorer"
    },
    { /* 0x59 */
        &FOLDERID_PublicLibraries,
        CSIDL_Type_AllUsers,
        NULL,
        L"Microsoft\\Windows\\Libraries"
    },
    { /* 0x5a */
        &FOLDERID_PublicRingtones,
        CSIDL_Type_AllUsers,
        NULL,
        L"Microsoft\\Windows\\Ringtones"
    },
    { /* 0x5b */
        &FOLDERID_QuickLaunch,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x5c */
        &FOLDERID_RecordedTVLibrary,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x5d */
        &FOLDERID_Ringtones,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x5e */
        &FOLDERID_SampleMusic,
        CSIDL_Type_AllUsers,
        NULL,
        L"Music\\Sample Music"
    },
    { /* 0x5f */
        &FOLDERID_SamplePictures,
        CSIDL_Type_AllUsers,
        NULL,
        L"Pictures\\Sample Pictures"
    },
    { /* 0x60 */
        &FOLDERID_SamplePlaylists,
        CSIDL_Type_AllUsers,
        NULL,
        L"Music\\Sample Playlists"
    },
    { /* 0x61 */
        &FOLDERID_SampleVideos,
        CSIDL_Type_AllUsers,
        NULL,
        L"Videos\\Sample Videos"
    },
    { /* 0x62 - CSIDL_SAVED_GAMES */
        &FOLDERID_SavedGames,
        CSIDL_Type_User,
        NULL,
        L"Saved Games"
    },
    { /* 0x63 - CSIDL_SEARCHES */
        &FOLDERID_SavedSearches,
        CSIDL_Type_User,
        NULL,
        L"Searches"
    },
    { /* 0x64 */
        &FOLDERID_SEARCH_CSC,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x65 */
        &FOLDERID_SEARCH_MAPI,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x66 */
        &FOLDERID_SearchHome,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x67 */
        &FOLDERID_SidebarDefaultParts,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x68 */
        &FOLDERID_SidebarParts,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x69 */
        &FOLDERID_SyncManagerFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x6a */
        &FOLDERID_SyncResultsFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x6b */
        &FOLDERID_SyncSetupFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x6c */
        &FOLDERID_UserPinned,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x6d */
        &FOLDERID_UserProfiles,
        CSIDL_Type_CurrVer,
        L"Users",
        L"Users"
    },
    { /* 0x6e */
        &FOLDERID_UserProgramFiles,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x6f */
        &FOLDERID_UserProgramFilesCommon,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x70 */
        &FOLDERID_UsersFiles,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x71 */
        &FOLDERID_UsersLibraries,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x72 */
        &FOLDERID_VideosLibrary,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    }
#endif
};

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
        strcpyW(shellFolderPath, userPrefix);
        PathAddBackslashW(shellFolderPath);
        strcatW(shellFolderPath, szSHFolders);
        pShellFolderPath = shellFolderPath;
        strcpyW(userShellFolderPath, userPrefix);
        PathAddBackslashW(userShellFolderPath);
        strcatW(userShellFolderPath, szSHUserFolders);
        pUserShellFolderPath = userShellFolderPath;
    }
    else
    {
        pUserShellFolderPath = szSHUserFolders;
        pShellFolderPath = szSHFolders;
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
         (strlenW(path) + 1) * sizeof(WCHAR));
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
    TRACE("returning 0x%08x\n", hr);
    return hr;
}

BOOL _SHGetUserProfileDirectoryW(HANDLE hToken, LPWSTR szPath, LPDWORD lpcchPath)
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
    TRACE("_SHGetUserProfileDirectoryW returning %S\n", szPath);
    return result;
}

/* Gets a 'semi-expanded' default value of the CSIDL with index folder into
 * pszPath, based on the entries in CSIDL_Data.  By semi-expanded, I mean:
 * - The entry's szDefaultPath may be either a string value or an integer
 *   resource identifier.  In the latter case, the string value of the resource
 *   is written.
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
    WCHAR resourcePath[MAX_PATH];
#ifdef __REACTOS__
    NT_PRODUCT_TYPE ProductType;
#endif

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

    switch (CSIDL_Data[folder].type)
    {
        case CSIDL_Type_User:
            strcpyW(pszPath, L"%USERPROFILE%");
            break;
#ifdef __REACTOS__
        case CSIDL_Type_InMyDocuments:
            strcpyW(pszPath, L"%USERPROFILE%");
            if (DoGetProductType(&ProductType) && ProductType == NtProductWinNt)
            {
                if (IS_INTRESOURCE(CSIDL_Data[CSIDL_MYDOCUMENTS].szDefaultPath))
                {
                    WCHAR szItem[MAX_PATH];
                    LoadStringW(shell32_hInstance,
                                LOWORD(CSIDL_Data[CSIDL_MYDOCUMENTS].szDefaultPath),
                                szItem, ARRAY_SIZE(szItem));
                    PathAppendW(pszPath, szItem);
                }
                else
                {
                    PathAppendW(pszPath, CSIDL_Data[CSIDL_MYDOCUMENTS].szDefaultPath);
                }
            }
            break;
#endif
        case CSIDL_Type_AllUsers:
#ifndef __REACTOS__
            strcpyW(pszPath, L"%PUBLIC%");
#else
            strcpyW(pszPath, L"%ALLUSERSPROFILE%");
#endif
            break;
        case CSIDL_Type_CurrVer:
            strcpyW(pszPath, L"%SystemDrive%");
            break;
        default:
            ; /* no corresponding env. var, do nothing */
    }

    hr = S_OK;
    if (CSIDL_Data[folder].szDefaultPath)
    {
        if (IS_INTRESOURCE(CSIDL_Data[folder].szDefaultPath))
        {
            if (LoadStringW(shell32_hInstance,
                LOWORD(CSIDL_Data[folder].szDefaultPath), resourcePath, MAX_PATH))
            {
                PathAppendW(pszPath, resourcePath);
            }
            else
            {
                ERR("(%d,%s), LoadString failed, missing translation?\n", folder,
                      debugstr_w(pszPath));
                hr = E_FAIL;
            }
        }
        else
        {
            PathAppendW(pszPath, CSIDL_Data[folder].szDefaultPath);
        }
    }
    TRACE("returning 0x%08x\n", hr);
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

    TRACE("0x%08x,0x%02x,%p\n", dwFlags, folder, pszPath);

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

            if (RegQueryValueExW(hKey, CSIDL_Data[folder].szValueName, NULL,
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
                    RegSetValueExW(hKey, CSIDL_Data[folder].szValueName, 0, dwType,
                                   (LPBYTE)pszPath, (strlenW(pszPath)+1)*sizeof(WCHAR));
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
    TRACE("returning 0x%08x (output path is %s)\n", hr, debugstr_w(pszPath));
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
        UserInfo = HeapAlloc(GetProcessHeap(), 0, InfoSize);
        if (UserInfo == NULL)
            return NULL;
        if (! GetTokenInformation(Token, TokenUser, UserInfo, InfoSize,
                                  &InfoSize))
        {
            HeapFree(GetProcessHeap(), 0, UserInfo);
            return NULL;
        }
    }

    if (! ConvertSidToStringSidW(UserInfo->User.Sid, &SidStr))
        SidStr = NULL;

    if (UserInfo != (PTOKEN_USER) InfoBuffer)
        HeapFree(GetProcessHeap(), 0, UserInfo);

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

    TRACE("%p,0x%08x,0x%02x,%p\n", hToken, dwFlags, folder, pszPath);

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
#ifdef __REACTOS__
    if (CSIDL_Data[folder].type != CSIDL_Type_User &&
        CSIDL_Data[folder].type != CSIDL_Type_InMyDocuments)
#else
    if (CSIDL_Data[folder].type != CSIDL_Type_User)
#endif
    {
        return E_INVALIDARG;
    }
    if (!pszPath)
        return E_INVALIDARG;

    if (dwFlags & SHGFP_TYPE_DEFAULT)
    {
#ifndef __REACTOS__
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
        szValueName = CSIDL_Data[folder].szValueName;
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
    TRACE("returning 0x%08x (output path is %s)\n", hr, debugstr_w(pszPath));
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

    TRACE("0x%08x,0x%02x,%p\n", dwFlags, folder, pszPath);

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
    if (CSIDL_Data[folder].type != CSIDL_Type_AllUsers)
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
         CSIDL_Data[folder].szValueName, pszPath);
        if (FAILED(hr))
#ifndef __REACTOS__
            hr = _SHGetDefaultValue(folder, pszPath);
#else
            hr = _SHGetDefaultValue(NULL, folder, pszPath);
#endif
    }
    TRACE("returning 0x%08x (output path is %s)\n", hr, debugstr_w(pszPath));
    return hr;
}

#ifndef __REACTOS__
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
                              (strlenW(szValue) + 1) * sizeof(WCHAR));
        if (lRet)
            hr = HRESULT_FROM_WIN32(lRet);
        else
            hr = S_OK;
    }
    TRACE("returning 0x%08x (output value is %s)\n", hr, debugstr_w(szValue));
    return hr;
}
#endif

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
#ifndef __REACTOS__
    WCHAR szTemp[MAX_PATH], szProfilesPrefix[MAX_PATH] = { 0 };
    HKEY key = NULL;
#else
    WCHAR szTemp[MAX_PATH];
#endif

    TRACE("%s, %p\n", debugstr_w(szSrc), szDest);

    if (!szSrc || !szDest) return E_INVALIDARG;

    /* short-circuit if there's nothing to expand */
    if (szSrc[0] != '%')
    {
        strcpyW(szDest, szSrc);
        hr = S_OK;
        goto end;
    }
#ifndef __REACTOS__
    /* Get the profile prefix, we'll probably be needing it */
    hr = _SHOpenProfilesKey(&key);
    if (SUCCEEDED(hr))
    {
        WCHAR def_val[MAX_PATH];

        /* get the system drive */
        GetSystemDirectoryW(def_val, MAX_PATH);
        strcpyW( def_val + 3, L"Users" );

        hr = _SHGetProfilesValue(key, L"ProfilesDirectory", szProfilesPrefix, def_val );
    }
#else
    hr = S_OK;
#endif

    *szDest = 0;
    strcpyW(szTemp, szSrc);
    while (SUCCEEDED(hr) && szTemp[0] == '%')
    {
        if (!strncmpiW(szTemp, L"%ALLUSERSPROFILE%", ARRAY_SIZE(L"%ALLUSERSPROFILE%")-1))
        {
#ifndef __REACTOS__
            WCHAR szAllUsers[MAX_PATH];

            strcpyW(szDest, szProfilesPrefix);
            hr = _SHGetProfilesValue(key, L"AllUsersProfile", szAllUsers, L"Public");
            PathAppendW(szDest, szAllUsers);
#else
            DWORD cchSize = cchDest;
            if (!GetAllUsersProfileDirectoryW(szDest, &cchSize))
                goto fallback_expand;
#endif
            PathAppendW(szDest, szTemp + ARRAY_SIZE(L"%ALLUSERSPROFILE%")-1);
        }
#ifndef __REACTOS__
        else if (!strncmpiW(szTemp, L"%PUBLIC%", ARRAY_SIZE(L"%PUBLIC%")-1))
        {
            WCHAR szAllUsers[MAX_PATH], def_val[MAX_PATH];

            GetSystemDirectoryW(def_val, MAX_PATH);
            strcpyW( def_val + 3, L"Users\\Public" );

            hr = _SHGetProfilesValue(key, L"Public", szAllUsers, def_val);
            PathAppendW(szDest, szAllUsers);
            PathAppendW(szDest, szTemp + ARRAY_SIZE(L"%PUBLIC%")-1);
        }
#endif
        else if (!strncmpiW(szTemp, L"%USERPROFILE%", ARRAY_SIZE(L"%USERPROFILE%")-1))
        {
#ifndef __REACTOS__
            WCHAR userName[MAX_PATH];
            DWORD userLen = MAX_PATH;

            strcpyW(szDest, szProfilesPrefix);
            GetUserNameW(userName, &userLen);
            PathAppendW(szDest, userName);
#else
            DWORD cchSize = cchDest;
            if (!_SHGetUserProfileDirectoryW(hToken, szDest, &cchSize))
                goto fallback_expand;
#endif
            PathAppendW(szDest, szTemp + ARRAY_SIZE(L"%USERPROFILE%")-1);
        }
        else if (!strncmpiW(szTemp, L"%SystemDrive%", ARRAY_SIZE(L"%SystemDrive%")-1))
        {
#ifndef __REACTOS__
            GetSystemDirectoryW(szDest, MAX_PATH);
#else
            if (!GetSystemDirectoryW(szDest, cchDest))
                goto fallback_expand;
#endif
            strcpyW(szDest + 3, szTemp + ARRAY_SIZE(L"%SystemDrive%")-1 + 1);
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
            else if (!strcmpW( szTemp, szDest )) break;  /* nothing expanded */
        }
        if (SUCCEEDED(hr)) strcpyW(szTemp, szDest);
    }
end:
#ifndef __REACTOS__
    if (key)
        RegCloseKey(key);
#endif
    TRACE("returning 0x%08x (input was %s, output is %s)\n", hr,
     debugstr_w(szSrc), debugstr_w(szDest));
    return hr;
}

/*************************************************************************
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

    TRACE("%p,%#x,%p,%#x,%s,%p\n", hwndOwner, nFolder, hToken, dwFlags, debugstr_a(pszSubPath), pszPath);

    if(pszPath) {
        pszPathW = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
        if(!pszPathW) {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
    }
    TRACE("%08x,%08x,%s\n",nFolder, dwFlags, debugstr_w(pszSubPathW));

    /* SHGetFolderPathAndSubDirW does not distinguish if pszSubPath isn't
     * set (null), or an empty string.therefore call it without the parameter set
     * if pszSubPath is an empty string
     */
    if (pszSubPath && pszSubPath[0]) {
        length = MultiByteToWideChar(CP_ACP, 0, pszSubPath, -1, NULL, 0);
        pszSubPathW = HeapAlloc(GetProcessHeap(), 0, length * sizeof(WCHAR));
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
    HeapFree(GetProcessHeap(), 0, pszPathW);
    HeapFree(GetProcessHeap(), 0, pszSubPathW);
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
    
    TRACE("%p,%#x,%p,%#x,%s,%p\n", hwndOwner, nFolder, hToken, dwFlags, debugstr_w(pszSubPath), pszPath);

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
            if (CSIDL_Data[folder].szDefaultPath &&
             !IS_INTRESOURCE(CSIDL_Data[folder].szDefaultPath) &&
             *CSIDL_Data[folder].szDefaultPath)
            {
                PathAddBackslashW(szTemp);
                strcatW(szTemp, CSIDL_Data[folder].szDefaultPath);
            }
            hr = S_OK;
            break;
        case CSIDL_Type_SystemPath:
            GetSystemDirectoryW(szTemp, MAX_PATH);
            if (CSIDL_Data[folder].szDefaultPath &&
             !IS_INTRESOURCE(CSIDL_Data[folder].szDefaultPath) &&
             *CSIDL_Data[folder].szDefaultPath)
            {
                PathAddBackslashW(szTemp);
                strcatW(szTemp, CSIDL_Data[folder].szDefaultPath);
            }
            hr = S_OK;
            break;
        case CSIDL_Type_SystemX86Path:
            if (!GetSystemWow64DirectoryW(szTemp, MAX_PATH)) GetSystemDirectoryW(szTemp, MAX_PATH);
            if (CSIDL_Data[folder].szDefaultPath &&
             !IS_INTRESOURCE(CSIDL_Data[folder].szDefaultPath) &&
             *CSIDL_Data[folder].szDefaultPath)
            {
                PathAddBackslashW(szTemp);
                strcatW(szTemp, CSIDL_Data[folder].szDefaultPath);
            }
            hr = S_OK;
            break;
        case CSIDL_Type_CurrVer:
            hr = _SHGetCurrentVersionPath(dwFlags, folder, szTemp);
            break;
        case CSIDL_Type_User:
#ifdef __REACTOS__
        case CSIDL_Type_InMyDocuments:
#endif
            hr = _SHGetUserProfilePath(hToken, dwFlags, folder, szTemp);
            break;
        case CSIDL_Type_AllUsers:
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
        strcpyW(szBuildPath, szTemp);

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
        strcpyW(pszPath, szBuildPath);

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

    TRACE("returning 0x%08x (final path is %s)\n", hr, debugstr_w(szBuildPath));
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

    TRACE("%p,%d,%p,%#x,%p\n", hwndOwner, nFolder, hToken, dwFlags, pszPath);

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
        szValueName = CSIDL_Data[folders[i]].szValueName;
#ifdef __REACTOS__
        if (!szValueName &&
            (CSIDL_Data[folders[i]].type == CSIDL_Type_User ||
             CSIDL_Data[folders[i]].type == CSIDL_Type_InMyDocuments))
#else
        if (!szValueName && CSIDL_Data[folders[i]].type == CSIDL_Type_User)
#endif
        {
            StringFromGUID2( CSIDL_Data[folders[i]].id, buffer, 39 );
            szValueName = &buffer[0];
        }

        if (!RegQueryValueExW(hUserKey, szValueName, NULL,
                              &dwType, (LPBYTE)path, &dwPathLen) &&
            (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
        {
            hr = SHGetFolderPathW(NULL, folders[i] | CSIDL_FLAG_CREATE,
                                  hToken, SHGFP_TYPE_CURRENT, path);
        }
        else
        {
            *path = '\0';
#ifdef __REACTOS__
            if (CSIDL_Data[folders[i]].type == CSIDL_Type_User ||
                CSIDL_Data[folders[i]].type == CSIDL_Type_InMyDocuments)
#else
            if (CSIDL_Data[folders[i]].type == CSIDL_Type_User)
#endif
                _SHGetUserProfilePath(hToken, SHGFP_TYPE_CURRENT, folders[i],
                 path);
            else if (CSIDL_Data[folders[i]].type == CSIDL_Type_AllUsers)
                _SHGetAllUsersProfilePath(SHGFP_TYPE_CURRENT, folders[i], path);
            else if (CSIDL_Data[folders[i]].type == CSIDL_Type_WindowsPath)
            {
                GetWindowsDirectoryW(path, MAX_PATH);
                if (CSIDL_Data[folders[i]].szDefaultPath &&
                    !IS_INTRESOURCE(CSIDL_Data[folders[i]].szDefaultPath))
                {
                    PathAddBackslashW(path);
                    strcatW(path, CSIDL_Data[folders[i]].szDefaultPath);
                }
            }
            else
                hr = E_FAIL;
            if (*path)
            {
                ret = RegSetValueExW(hUserKey, szValueName, 0, REG_EXPAND_SZ,
                 (LPBYTE)path, (strlenW(path) + 1) * sizeof(WCHAR));
                if (ret)
                    hr = HRESULT_FROM_WIN32(ret);
                else
                {
                    hr = SHGetFolderPathW(NULL, folders[i] | CSIDL_FLAG_CREATE,
                     hToken, SHGFP_TYPE_CURRENT, path);
                    ret = RegSetValueExW(hKey, szValueName, 0, REG_SZ,
                     (LPBYTE)path, (strlenW(path) + 1) * sizeof(WCHAR));
                    if (ret)
                        hr = HRESULT_FROM_WIN32(ret);
                }
            }
        }
    }
    if (hUserKey)
        RegCloseKey(hUserKey);
    if (hKey)
        RegCloseKey(hKey);

    TRACE("returning 0x%08x\n", hr);
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
/* Cannot use #if _WIN32_WINNT >= 0x0600 because _WIN32_WINNT == 0x0600 here. */
#ifndef __REACTOS__
     CSIDL_CONTACTS,
     CSIDL_DOWNLOADS,
     CSIDL_LINKS,
     CSIDL_APPDATA_LOCALLOW,
     CSIDL_SAVED_GAMES,
     CSIDL_SEARCHES
#endif
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
        strcpyW(userShellFolderPath, L".Default");
        PathAddBackslashW(userShellFolderPath);
        strcatW(userShellFolderPath, szSHUserFolders);
        pUserShellFolderPath = userShellFolderPath;
        strcpyW(shellFolderPath, L".Default");
        PathAddBackslashW(shellFolderPath);
        strcatW(shellFolderPath, szSHFolders);
        pShellFolderPath = shellFolderPath;
    }
    else
    {
        hToken = NULL;
        hRootKey = HKEY_CURRENT_USER;
        pUserShellFolderPath = szSHUserFolders;
        pShellFolderPath = szSHFolders;
    }

    hr = _SHRegisterFolders(hRootKey, hToken, pUserShellFolderPath,
     pShellFolderPath, folders, ARRAY_SIZE(folders));
    TRACE("returning 0x%08x\n", hr);
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
    hr = _SHRegisterFolders(HKEY_LOCAL_MACHINE, NULL, szSHUserFolders,
     szSHFolders, folders, ARRAY_SIZE(folders));
    TRACE("returning 0x%08x\n", hr);
    return hr;
}

/* Register the default values in the registry, as some apps seem to depend
 * on their presence.  The set registered was taken from Windows XP.
 */
HRESULT SHELL_RegisterShellFolders(void)
{
    HRESULT hr;

    hr = _SHRegisterUserShellFolders(TRUE);
    if (SUCCEEDED(hr))
        hr = _SHRegisterUserShellFolders(FALSE);
    if (SUCCEEDED(hr))
        hr = _SHRegisterCommonShellFolders();
    return hr;
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

    TRACE("%p 0x%08x %p 0x%08x %p\n",
     hwndOwner, nFolder, hToken, dwReserved, ppidl);
    
    if (!ppidl)
        return E_INVALIDARG;
    if (dwReserved)
        return E_INVALIDARG;

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
