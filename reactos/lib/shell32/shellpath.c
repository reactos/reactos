/*
 * Path Functions
 *
 * Copyright 1998, 1999, 2000 Juergen Schmied
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
 *
 * NOTES:
 *
 * Many of these functions are in SHLWAPI.DLL also
 *
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "shlobj.h"
#include "shell32_main.h"
#include "undocshell.h"
#include "pidl.h"
#include "wine/unicode.h"
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

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
	  if (!strcasecmp(lpszExtension,lpszExtensions[i])) return TRUE;

	return FALSE;
}

/*************************************************************************
 *  PathIsExeW		[internal]
 */
static BOOL PathIsExeW (LPCWSTR lpszPath)
{
	LPCWSTR lpszExtension = PathGetExtensionW(lpszPath);
        int i;
        static const WCHAR lpszExtensions[][4] =
            {{'e','x','e','\0'}, {'c','o','m','\0'}, {'p','i','f','\0'},
             {'c','m','d','\0'}, {'b','a','t','\0'}, {'s','c','f','\0'},
             {'s','c','r','\0'}, {'\0'} };

	TRACE("path=%s\n",debugstr_w(lpszPath));

	for(i=0; lpszExtensions[i][0]; i++)
	  if (!strcmpiW(lpszExtension,lpszExtensions[i])) return TRUE;

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
BOOL WINAPI PathMakeUniqueNameA(
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
BOOL WINAPI PathMakeUniqueNameW(
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
 *
 * NOTES
 *     exported by ordinal
 */
BOOL WINAPI PathYetAnotherMakeUniqueName(
	LPWSTR lpszBuffer,
	LPCWSTR lpszPathName,
	LPCWSTR lpszShortName,
	LPCWSTR lpszLongName)
{
    FIXME("(%p, %s, %s ,%s):stub.\n",
          lpszBuffer, debugstr_w(lpszPathName), debugstr_w(lpszShortName), debugstr_w(lpszLongName));
    return TRUE;
}


/*
	########## cleaning and resolving paths ##########
 */

/*************************************************************************
 * PathFindOnPath	[SHELL32.145]
 */
BOOL WINAPI PathFindOnPathAW(LPVOID sFile, LPCVOID sOtherDirs)
{
	if (SHELL_OsIsUnicode())
	  return PathFindOnPathW(sFile, (LPCWSTR *)sOtherDirs);
	return PathFindOnPathA(sFile, (LPCSTR *)sOtherDirs);
}

/*************************************************************************
 * PathCleanupSpec	[SHELL32.171]
 */
DWORD WINAPI PathCleanupSpecAW (LPCVOID x, LPVOID y)
{
    FIXME("(%p, %p) stub\n",x,y);
    return TRUE;
}

/*************************************************************************
 * PathQualifyA		[SHELL32]
 */
BOOL WINAPI PathQualifyA(LPCSTR pszPath)
{
	FIXME("%s\n",pszPath);
	return 0;
}

/*************************************************************************
 * PathQualifyW		[SHELL32]
 */
BOOL WINAPI PathQualifyW(LPCWSTR pszPath)
{
	FIXME("%s\n",debugstr_w(pszPath));
	return 0;
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

/*************************************************************************
 * PathResolveA [SHELL32.51]
 */
BOOL WINAPI PathResolveA(
	LPSTR lpszPath,
	LPCSTR *alpszPaths,
	DWORD dwFlags)
{
	FIXME("(%s,%p,0x%08lx),stub!\n",
	  lpszPath, *alpszPaths, dwFlags);
	return 0;
}

/*************************************************************************
 * PathResolveW [SHELL32]
 */
BOOL WINAPI PathResolveW(
	LPWSTR lpszPath,
	LPCWSTR *alpszPaths,
	DWORD dwFlags)
{
	FIXME("(%s,%p,0x%08lx),stub!\n",
	  debugstr_w(lpszPath), debugstr_w(*alpszPaths), dwFlags);
	return 0;
}

/*************************************************************************
 * PathResolve [SHELL32.51]
 */
BOOL WINAPI PathResolveAW(
	LPVOID lpszPath,
	LPCVOID *alpszPaths,
	DWORD dwFlags)
{
	if (SHELL_OsIsUnicode())
	  return PathResolveW(lpszPath, (LPCWSTR*)alpszPaths, dwFlags);
	return PathResolveA(lpszPath, (LPCSTR*)alpszPaths, dwFlags);
}

/*************************************************************************
*	PathProcessCommandA	[SHELL32.653]
*/
HRESULT WINAPI PathProcessCommandA (
	LPCSTR lpszPath,
	LPSTR lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	FIXME("%s %p 0x%04lx 0x%04lx stub\n",
	lpszPath, lpszBuff, dwBuffSize, dwFlags);
	strcpy(lpszBuff, lpszPath);
	return 0;
}

/*************************************************************************
*	PathProcessCommandW
*/
HRESULT WINAPI PathProcessCommandW (
	LPCWSTR lpszPath,
	LPWSTR lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	FIXME("(%s, %p, 0x%04lx, 0x%04lx) stub\n",
	debugstr_w(lpszPath), lpszBuff, dwBuffSize, dwFlags);
	strcpyW(lpszBuff, lpszPath);
	return 0;
}

/*************************************************************************
*	PathProcessCommand (SHELL32.653)
*/
HRESULT WINAPI PathProcessCommandAW (
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

/*************************************************************************
 * SHGetFolderPathW			[SHELL32.@]
 *
 * converts csidl to path
 */

static const WCHAR szSHFolders[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','S','h','e','l','l',' ','F','o','l','d','e','r','s','\0'};
static const WCHAR szSHUserFolders[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','U','s','e','r',' ','S','h','e','l','l',' ','F','o','l','d','e','r','s','\0'};
static const WCHAR szSetup[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','S','e','t','u','p','\0'};
static const WCHAR szCurrentVersion[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\0'};



static const WCHAR Administrative_ToolsW[] = {'A','d','m','i','n','i','s','t','r','a','t','i','v','e',' ','T','o','o','l','s','\0'};
static const WCHAR All_Users__Application_DataW[] = {'A','l','l',' ','U','s','e','r','s','\\',
                                                     'A','p','p','l','i','c','a','t','i','o','n',' ','D','a','t','a','\0'};
static const WCHAR All_Users__DesktopW[] = {'A','l','l',' ','U','s','e','r','s','\\',
                                              'D','e','s','k','t','o','p','\0'};
static const WCHAR All_Users__DocumentsW[] = {'A','l','l',' ','U','s','e','r','s','\\',
                                              'D','o','c','u','m','e','n','t','s','\0'};
static const WCHAR All_Users__Documents__My_MusicW[] = {'A','l','l',' ','U','s','e','r','s','\\',
                                                        'D','o','c','u','m','e','n','t','s','\\',
                                                        'M','y',' ','M','u','s','i','c','\0'};
static const WCHAR All_Users__Documents__My_PicturesW[] = {'A','l','l',' ','U','s','e','r','s','\\',
                                                           'D','o','c','u','m','e','n','t','s','\\',
                                                           'M','y',' ','P','i','c','t','u','r','e','s','\0'};
static const WCHAR All_Users__Documents__My_VideoW[] = {'A','l','l',' ','U','s','e','r','s','\\',
                                                        'D','o','c','u','m','e','n','t','s','\\',
                                                        'M','y',' ','V','i','d','e','o','\0'};
static const WCHAR All_Users__Start_MenuW[] = {'A','l','l',' ','U','s','e','r','s','\\','S','t','a','r','t',' ','M','e','n','u','\0'};
static const WCHAR All_Users__Start_Menu__ProgramsW[] = {'A','l','l',' ','U','s','e','r','s','\\','S','t','a','r','t',' ','M','e','n','u','\\','P','r','o','g','r','a','m','s','\0'};
static const WCHAR All_Users__Start_Menu__Programs__Administrative_ToolsW[] = {'A','l','l',' ','U','s','e','r','s','\\',
                                'S','t','a','r','t',' ','M','e','n','u','\\','P','r','o','g','r','a','m','s','\\',
                                'A','d','m','i','n','i','s','t','r','a','t','i','v','e',' ','T','o','o','l','s','\0'};
static const WCHAR All_Users__Start_Menu__Programs__StartUpW[] = {'A','l','l',' ','U','s','e','r','s','\\',
                                                               'S','t','a','r','t',' ','M','e','n','u','\\',
                                                               'P','r','o','g','r','a','m','s','\\',
                                                               'S','t','a','r','t','U','p','\0'};
static const WCHAR All_Users__TemplatesW[] = {'A','l','l',' ','U','s','e','r','s','\\',
                                              'T','e','m','p','l','a','t','e','s','\0'};
static const WCHAR AppDataW[] = {'A','p','p','D','a','t','a','\0'};
static const WCHAR Application_DataW[] = {'A','p','p','l','i','c','a','t','i','o','n',' ','D','a','t','a','\0'};
static const WCHAR CacheW[] = {'C','a','c','h','e','\0'};
static const WCHAR CD_BurningW[] = {'C','D',' ','B','u','r','n','i','n','g','\0'};
static const WCHAR Common_Administrative_ToolsW[] = {'C','o','m','m','o','n',' ','A','d','m','i','n','i','s','t','r','a','t','i','v','e',' ','T','o','o','l','s','\0'};
static const WCHAR Common_AppDataW[] = {'C','o','m','m','o','n',' ','A','p','p','D','a','t','a','\0'};
static const WCHAR Common_DesktopW[] = {'C','o','m','m','o','n',' ','D','e','s','k','t','o','p','\0'};
static const WCHAR Common_DocumentsW[] = {'C','o','m','m','o','n',' ','D','o','c','u','m','e','n','t','s','\0'};
static const WCHAR CommonFilesDirW[] = {'C','o','m','m','o','n','F','i','l','e','s','D','i','r','\0'};
static const WCHAR CommonMusicW[] = {'C','o','m','m','o','n','M','u','s','i','c','\0'};
static const WCHAR CommonPicturesW[] = {'C','o','m','m','o','n','P','i','c','t','u','r','e','s','\0'};
static const WCHAR Common_ProgramsW[] = {'C','o','m','m','o','n',' ','P','r','o','g','r','a','m','s','\0'};
static const WCHAR Common_StartUpW[] = {'C','o','m','m','o','n',' ','S','t','a','r','t','U','p','\0'};
static const WCHAR Common_Start_MenuW[] = {'C','o','m','m','o','n',' ','S','t','a','r','t',' ','M','e','n','u','\0'};
static const WCHAR Common_TemplatesW[] = {'C','o','m','m','o','n',' ','T','e','m','p','l','a','t','e','s','\0'};
static const WCHAR CommonVideoW[] = {'C','o','m','m','o','n','V','i','d','e','o','\0'};
static const WCHAR CookiesW[] = {'C','o','o','k','i','e','s','\0'};
static const WCHAR DesktopW[] = {'D','e','s','k','t','o','p','\0'};
static const WCHAR Empty_StringW[] = {'\0'};
static const WCHAR FavoritesW[] = {'F','a','v','o','r','i','t','e','s','\0'};
static const WCHAR FontsW[] = {'F','o','n','t','s','\0'};
static const WCHAR HistoryW[] = {'H','i','s','t','o','r','y','\0'};
static const WCHAR Local_AppDataW[] = {'L','o','c','a','l',' ','A','p','p','D','a','t','a','\0'};
static const WCHAR Local_Settings__Application_DataW[] = {'L','o','c','a','l',' ','S','e','t','t','i','n','g','s','\\',
                                                         'A','p','p','l','i','c','a','t','i','o','n',' ','D','a','t','a','\0'};
static const WCHAR Local_Settings__Application_Data__Microsoft__CD_BurningW[] = {
    'L','o','c','a','l',' ','S','e','t','t','i','n','g','s','\\',
    'A','p','p','l','i','c','a','t','i','o','n',' ','D','a','t','a','\\',
    'M','i','c','r','o','s','o','f','t','\\','C','D',' ','B','u','r','n','i','n','g','\0'};
static const WCHAR My_DocumentsW[] = {'M','y',' ','D','o','c','u','m','e','n','t','s','\0'};
static const WCHAR My_Documents__My_MusicW[] = {'M','y',' ','D','o','c','u','m','e','n','t','s','\\',
                                                'M','y',' ','M','u','s','i','c','\0'};
static const WCHAR My_Documents__My_PicturesW[] = {'M','y',' ','D','o','c','u','m','e','n','t','s','\\',
                                                  'M','y',' ','P','i','c','t','u','r','e','s','\0'};
static const WCHAR My_Documents__My_VideoW[] = {'M','y',' ','D','o','c','u','m','e','n','t','s','\\',
                                               'M','y',' ','V','i','d','e','o','\0'};
static const WCHAR My_MusicW[] = {'M','y',' ','M','u','s','i','c','\0'};
static const WCHAR My_PicturesW[] = {'M','y',' ','P','i','c','t','u','r','e','s','\0'};
static const WCHAR My_VideoW[] = {'M','y',' ','V','i','d','e','o','\0'};
static const WCHAR NetHoodW[] = {'N','e','t','H','o','o','d','\0'};
static const WCHAR PersonalW[] = {'P','e','r','s','o','n','a','l','\0'};
static const WCHAR PrintHoodW[] = {'P','r','i','n','t','H','o','o','d','\0'};
static const WCHAR ProgramFilesDirW[] = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r','\0'};
static const WCHAR Program_FilesW[] = {'P','r','o','g','r','a','m',' ','F','i','l','e','s','\0'};
static const WCHAR Program_Files__Common_FilesW[] = {'P','r','o','g','r','a','m',' ','F','i','l','e','s','\\',
                                                     'C','o','m','m','o','n',' ','F','i','l','e','s','\0'};
static const WCHAR ProgramsW[] = {'P','r','o','g','r','m','s','\0'};
static const WCHAR RecentW[] = {'R','e','c','e','n','t','\0'};
static const WCHAR ResourcesW[] = {'R','e','s','o','u','r','c','e','s','\0'};
static const WCHAR SendToW[] = {'S','e','n','d','T','o','\0'};
static const WCHAR ShellNewW[] = {'S','h','e','l','l','N','e','w','\0'};
static const WCHAR Start_Menu__ProgramsW[] = {'S','t','a','r','t',' ','M','e','n','u','\\','P','r','o','g','r','a','m','s','\0'};
static const WCHAR SysDirW[] = {'S','y','s','D','i','r','\0'};
static const WCHAR SYSTEMW[] = {'S','Y','S','T','E','M','\0'};
static const WCHAR StartUpW[] = {'S','t','a','r','t','U','p','\0'};
static const WCHAR Start_MenuW[] = {'S','t','a','r','t',' ','M','e','n','u','\0'};
static const WCHAR Start_Menu__Programs__Administrative_ToolsW[] = {
                                'S','t','a','r','t',' ','M','e','n','u','\\','P','r','o','g','r','a','m','s','\\',
                                'A','d','m','i','n','i','s','t','r','a','t','i','v','e',' ','T','o','o','l','s','\0'};
static const WCHAR Start_Menu__Programs__StartUpW[] = {'S','t','a','r','t',' ','M','e','n','u','\\',
                                                     'P','r','o','g','r','a','m','s','\\',
                                                     'S','t','a','r','t','U','p','\0'};
static const WCHAR TemplatesW[] = {'T','e','m','p','l','a','t','e','s','\0'};
static const WCHAR Temporary_Internet_FilesW[] = {'T','e','m','p','o','r','a','r','y',' ','I','n','t','e','r','n','e','t',' ','F','i','l','e','s','\0'};
static const WCHAR WinDirW[] = {'W','i','n','D','i','r','\0'};
static const WCHAR WindowsW[] = {'W','i','n','d','o','w','s','\0'};




typedef struct
{
    DWORD dwFlags;
    HKEY hRootKey;
    LPCWSTR szValueName;
    LPCWSTR szDefaultPath; /* fallback string; sub dir of windows directory */
} CSIDL_DATA;

#define CSIDL_MYFLAG_SHFOLDER	1
#define CSIDL_MYFLAG_SETUP	2
#define CSIDL_MYFLAG_CURRVER	4
#define CSIDL_MYFLAG_RELATIVE	8

#define HKLM HKEY_LOCAL_MACHINE
#define HKCU HKEY_CURRENT_USER
#define HKEY_DISALLOWED    (HKEY)0
#define HKEY_UNIMPLEMENTED (HKEY)1
#define HKEY_WINDOWSPATH   (HKEY)2
#define HKEY_NONEXISTENT   (HKEY)3
static const CSIDL_DATA CSIDL_Data[] =
{
    { /* CSIDL_DESKTOP */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	DesktopW,
	DesktopW
    },
    { /* CSIDL_INTERNET */
	0,
        HKEY_DISALLOWED,
	NULL,
	NULL
    },
    { /* CSIDL_PROGRAMS */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	ProgramsW,
	Start_Menu__ProgramsW
    },
    { /* CSIDL_CONTROLS (.CPL files) */
	CSIDL_MYFLAG_SETUP | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	SysDirW,
	SYSTEMW
    },
    { /* CSIDL_PRINTERS */
	CSIDL_MYFLAG_SETUP | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	SysDirW,
	SYSTEMW
    },
    { /* CSIDL_PERSONAL */
	CSIDL_MYFLAG_SHFOLDER,
        HKCU,
	PersonalW,
	My_DocumentsW
    },
    { /* CSIDL_FAVORITES */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	FavoritesW,
	FavoritesW
    },
    { /* CSIDL_STARTUP */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	StartUpW,
	Start_Menu__Programs__StartUpW
    },
    { /* CSIDL_RECENT */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	RecentW,
	RecentW
    },
    { /* CSIDL_SENDTO */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	SendToW,
	SendToW
    },
    { /* CSIDL_BITBUCKET - Recycle Bin */
	0,
        HKEY_DISALLOWED,
	NULL,
	NULL,
    },
    { /* CSIDL_STARTMENU */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	Start_MenuW,
	Start_MenuW
    },
    { /* CSIDL_MYDOCUMENTS */
	0,
        HKEY_UNIMPLEMENTED, /* FIXME */
	NULL,
	NULL
    },
    { /* CSIDL_MYMUSIC */
	CSIDL_MYFLAG_SHFOLDER,
        HKCU,
	My_MusicW,
	My_Documents__My_MusicW
    },
    { /* CSIDL_MYVIDEO */
	CSIDL_MYFLAG_SHFOLDER,
        HKCU,
	My_VideoW,
	My_Documents__My_VideoW
    },
    { /* unassigned */
	0,
        HKEY_DISALLOWED,
	NULL,
	NULL,
    },
    { /* CSIDL_DESKTOPDIRECTORY */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	DesktopW,
	DesktopW
    },
    { /* CSIDL_DRIVES */
	0,
        HKEY_DISALLOWED,
	NULL,
	NULL,
    },
    { /* CSIDL_NETWORK */
	0,
        HKEY_DISALLOWED,
	NULL,
	NULL,
    },
    { /* CSIDL_NETHOOD */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	NetHoodW,
	NetHoodW
    },
    { /* CSIDL_FONTS */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
        FontsW,
	FontsW
    },
    { /* CSIDL_TEMPLATES */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	TemplatesW,
	ShellNewW
    },
    { /* CSIDL_COMMON_STARTMENU */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	Common_Start_MenuW,
	All_Users__Start_MenuW
    },
    { /* CSIDL_COMMON_PROGRAMS */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	Common_ProgramsW,
	All_Users__Start_Menu__ProgramsW
    },
    { /* CSIDL_COMMON_STARTUP */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKLM,
        Common_StartUpW,
	All_Users__Start_Menu__Programs__StartUpW
    },
    { /* CSIDL_COMMON_DESKTOPDIRECTORY */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	Common_DesktopW,
	All_Users__DesktopW
    },
    { /* CSIDL_APPDATA */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	AppDataW,
	Application_DataW
    },
    { /* CSIDL_PRINTHOOD */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	PrintHoodW,
	PrintHoodW
    },
    { /* CSIDL_LOCAL_APPDATA (win2k only/undocumented) */
	CSIDL_MYFLAG_SHFOLDER,
        HKCU,
	Local_AppDataW,
	Local_Settings__Application_DataW,
    },
    { /* CSIDL_ALTSTARTUP */
	0,
        HKEY_NONEXISTENT,
	NULL,
	NULL
    },
    { /* CSIDL_COMMON_ALTSTARTUP */
	0,
        HKEY_NONEXISTENT,
	NULL,
	NULL
    },
    { /* CSIDL_COMMON_FAVORITES */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	FavoritesW,
	FavoritesW
    },
    { /* CSIDL_INTERNET_CACHE (32) */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	CacheW,
	Temporary_Internet_FilesW
    },
    { /* CSIDL_COOKIES (33) */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	CookiesW,
	CookiesW
    },
    { /* CSIDL_HISTORY (34) */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	HistoryW,
	HistoryW
    },
    { /* CSIDL_COMMON_APPDATA */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	Common_AppDataW,
	All_Users__Application_DataW
    },
    { /* CSIDL_WINDOWS */
	CSIDL_MYFLAG_SETUP,
        HKLM,
	WinDirW,
	WindowsW
    },
    { /* CSIDL_SYSTEM */
	CSIDL_MYFLAG_SETUP | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	SysDirW,
	SYSTEMW
    },
    { /* CSIDL_PROGRAM_FILES */
	CSIDL_MYFLAG_CURRVER,
        HKLM,
	ProgramFilesDirW,
	Program_FilesW
    },
    { /* CSIDL_MYPICTURES */
	CSIDL_MYFLAG_SHFOLDER,
        HKCU,
	My_PicturesW,
	My_Documents__My_PicturesW
    },
    { /* CSIDL_PROFILE */
	CSIDL_MYFLAG_SETUP | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	WinDirW, /* correct ? */
	Empty_StringW
    },
    { /* CSIDL_SYSTEMX86 */
	CSIDL_MYFLAG_SETUP | CSIDL_MYFLAG_RELATIVE,
        HKLM,
 	SysDirW,
	SYSTEMW
    },
    { /* CSIDL_PROGRAM_FILESX86 */
	CSIDL_MYFLAG_CURRVER,
        HKLM,
	ProgramFilesDirW,
	Program_FilesW
    },
    { /* CSIDL_PROGRAM_FILES_COMMON */
	CSIDL_MYFLAG_CURRVER,
        HKLM,
	CommonFilesDirW,
	Program_Files__Common_FilesW /* ? */
    },
    { /* CSIDL_PROGRAM_FILES_COMMONX86 */
	CSIDL_MYFLAG_CURRVER,
        HKLM,
	CommonFilesDirW,
	Program_Files__Common_FilesW /* ? */
    },
    { /* CSIDL_COMMON_TEMPLATES */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	Common_TemplatesW,
	/*"Documents and Settings\\"*/ All_Users__TemplatesW
    },
    { /* CSIDL_COMMON_DOCUMENTS */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	Common_DocumentsW,
	/*"Documents and Settings\\"*/ All_Users__DocumentsW
    },
    { /* CSIDL_COMMON_ADMINTOOLS */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	Common_Administrative_ToolsW,
	/*"Documents and Settings\\"*/ All_Users__Start_Menu__Programs__Administrative_ToolsW
    },
    { /* CSIDL_ADMINTOOLS */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKCU,
	Administrative_ToolsW,
	Start_Menu__Programs__Administrative_ToolsW
    },
    { /* CSIDL_CONNECTIONS */
	0,
        HKEY_DISALLOWED,
	NULL,
	NULL
    },
    { /* unassigned 32 */
	0,
        HKEY_DISALLOWED,
	NULL,
	NULL
    },
    { /* unassigned 33 */
	0,
        HKEY_DISALLOWED,
	NULL,
	NULL
    },
    { /* unassigned 34 */
	0,
        HKEY_DISALLOWED,
	NULL,
	NULL
    },
    { /* CSIDL_COMMON_MUSIC */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	CommonMusicW,
	/*"Documents and Settings\\"*/ All_Users__Documents__My_MusicW
    },
    { /* CSIDL_COMMON_PICTURES */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	CommonPicturesW,
	/*"Documents and Settings\\"*/ All_Users__Documents__My_PicturesW
    },
    { /* CSIDL_COMMON_VIDEO */
	CSIDL_MYFLAG_SHFOLDER | CSIDL_MYFLAG_RELATIVE,
        HKLM,
	CommonVideoW,
	/*"Documents and Settings\\"*/ All_Users__Documents__My_VideoW
    },
    { /* CSIDL_RESOURCES */
	0,
        HKEY_WINDOWSPATH,
	NULL,
	ResourcesW
    },
    { /* CSIDL_RESOURCES_LOCALIZED */
	0,
        HKEY_DISALLOWED, /* FIXME */
	NULL,
	NULL
    },
    { /* CSIDL_COMMON_OEM_LINKS */
	0,
        HKEY_DISALLOWED, /* FIXME */
	NULL,
	NULL
    },
    { /* CSIDL_CDBURN_AREA */
	CSIDL_MYFLAG_SHFOLDER,
        HKCU,
	CD_BurningW,
	Local_Settings__Application_Data__Microsoft__CD_BurningW
    },
    { /* unassigned 3C */
	0,
        HKEY_DISALLOWED,
	NULL,
	NULL
    },
    { /* CSIDL_COMPUTERSNEARME */
	0,
        HKEY_DISALLOWED, /* FIXME */
	NULL,
	NULL
    },
    { /* CSIDL_PROFILES */
	0,
        HKEY_DISALLOWED, /* FIXME */
	NULL,
	NULL
    }
};
#undef HKCU
#undef HKLM

/**********************************************************************/

HRESULT WINAPI SHGetFolderPathW(
	HWND hwndOwner,
	int csidl,
	HANDLE hToken,	/* [in] FIXME: get paths for specific user */
	DWORD dwFlags,	/* [in] FIXME: SHGFP_TYPE_CURRENT|SHGFP_TYPE_DEFAULT */
	LPWSTR pszPath)
{
	WCHAR   szBuildPath[MAX_PATH];
	HKEY	hRootKey, hKey;
	DWORD	dwCsidlFlags;
	DWORD	dwType, dwDisp, dwPathLen = MAX_PATH;
	DWORD	folder = csidl & CSIDL_FOLDER_MASK;
	WCHAR	*p;

	TRACE("%p,%p,csidl=0x%04x\n", hwndOwner,pszPath,csidl);

        if (!pszPath)
            return E_INVALIDARG;

        *pszPath = '\0';
	if ((folder >= sizeof(CSIDL_Data) / sizeof(CSIDL_Data[0])) ||
	    (CSIDL_Data[folder].hRootKey == HKEY_DISALLOWED))
	    return E_INVALIDARG;
	if (CSIDL_Data[folder].hRootKey == HKEY_UNIMPLEMENTED)
	{
	    FIXME("folder 0x%04lx unknown, please add.\n", folder);
	    return E_FAIL;
	}
        if (CSIDL_Data[folder].hRootKey == HKEY_NONEXISTENT)
            return S_FALSE;

	/* Special case for some values that don't exist in registry */
	if (CSIDL_Data[folder].hRootKey == HKEY_WINDOWSPATH)
	{
	    GetWindowsDirectoryW(pszPath, MAX_PATH);
	    PathAddBackslashW(pszPath);
	    strcatW(pszPath, CSIDL_Data[folder].szDefaultPath);
	    return S_OK;
	}
        
	dwCsidlFlags = CSIDL_Data[folder].dwFlags;
	hRootKey = CSIDL_Data[folder].hRootKey;

	if (dwCsidlFlags & CSIDL_MYFLAG_SHFOLDER)
	{
	  /*   user shell folders */
	  if   (RegCreateKeyExW(hRootKey,szSHUserFolders,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp)) return E_FAIL;

	  if   (RegQueryValueExW(hKey,CSIDL_Data[folder].szValueName,NULL,&dwType,(LPBYTE)pszPath,&dwPathLen))
	  {
	    RegCloseKey(hKey);

	    /* shell folders */
	    if (RegCreateKeyExW(hRootKey,szSHFolders,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp)) return E_FAIL;

	    if (RegQueryValueExW(hKey,CSIDL_Data[folder].szValueName,NULL,&dwType,(LPBYTE)pszPath,&dwPathLen))
	    {

	      /* value not existing */
	      if (dwCsidlFlags & CSIDL_MYFLAG_RELATIVE)
	      {
	        GetWindowsDirectoryW(pszPath, MAX_PATH);
	        PathAddBackslashW(pszPath);
	        strcatW(pszPath, CSIDL_Data[folder].szDefaultPath);
	      }
	      else
	      {
	        GetSystemDirectoryW(pszPath, MAX_PATH);
	        strcpyW(pszPath + 3, CSIDL_Data[folder].szDefaultPath);
	      }
              dwType=REG_SZ;
	      RegSetValueExW(hKey,CSIDL_Data[folder].szValueName,0,REG_SZ,(LPBYTE)pszPath,
                         (strlenW(pszPath)+1)*sizeof(WCHAR));
	    }
	  }
	  RegCloseKey(hKey);
        }
	else
	{
	  LPCWSTR pRegPath;

	  if (dwCsidlFlags & CSIDL_MYFLAG_SETUP)
	    pRegPath = szSetup;
	  else if (dwCsidlFlags & CSIDL_MYFLAG_CURRVER)
	    pRegPath = szCurrentVersion;
	  else
	  {
	    ERR("folder settings broken, please correct !\n");
	    return E_FAIL;
	  }

	  if   (RegCreateKeyExW(hRootKey,pRegPath,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp)) return E_FAIL;

	  if   (RegQueryValueExW(hKey,CSIDL_Data[folder].szValueName,NULL,&dwType,(LPBYTE)pszPath,&dwPathLen))
	  {
	    /* value not existing */
	    if (dwCsidlFlags & CSIDL_MYFLAG_RELATIVE)
	    {
	      GetWindowsDirectoryW(pszPath, MAX_PATH);
	      PathAddBackslashW(pszPath);
	      strcatW(pszPath, CSIDL_Data[folder].szDefaultPath);
	    }
	    else
	    {
	      GetSystemDirectoryW(pszPath, MAX_PATH);
	      strcpyW(pszPath + 3, CSIDL_Data[folder].szDefaultPath);
	    }
            dwType=REG_SZ;
	    RegSetValueExW(hKey,CSIDL_Data[folder].szValueName,0,REG_SZ,(LPBYTE)pszPath,
                       (strlenW(pszPath)+1)*sizeof(WCHAR));
	  }
	  RegCloseKey(hKey);
	}

	/* expand paths like %USERPROFILE% */
	if (dwType == REG_EXPAND_SZ)
	{
	  ExpandEnvironmentStringsW(pszPath, szBuildPath, MAX_PATH);
	  strcpyW(pszPath, szBuildPath);
	}

	/* if we don't care about existing directories we are ready */
	if(csidl & CSIDL_FLAG_DONT_VERIFY) return S_OK;

	if (PathFileExistsW(pszPath)) return S_OK;

	/* not existing but we are not allowed to create it */
	if (!(csidl & CSIDL_FLAG_CREATE)) return E_FAIL;

	/* create directory/directories */
	strcpyW(szBuildPath, pszPath);
	p = strchrW(szBuildPath, '\\');
	while (p)
	{
	    *p = 0;
	    if (!PathFileExistsW(szBuildPath))
	    {
		if (!CreateDirectoryW(szBuildPath,NULL))
		{
		    ERR("Failed to create directory '%s'.\n", debugstr_w(pszPath));
		    return E_FAIL;
		}
	    }
	    *p = '\\';
	    p = strchrW(p+1, '\\');
	}
	/* last component must be created too. */
	if (!PathFileExistsW(szBuildPath))
	{
	    if (!CreateDirectoryW(szBuildPath,NULL))
	    {
		ERR("Failed to create directory '%s'.\n", debugstr_w(pszPath));
		return E_FAIL;
	    }
	}

	TRACE("Created missing system directory '%s'\n", debugstr_w(pszPath));
	return S_OK;
}

/*************************************************************************
 * SHGetFolderPathA			[SHELL32.@]
 */
HRESULT WINAPI SHGetFolderPathA(
	HWND hwndOwner,
	int csidl,
	HANDLE hToken,
	DWORD dwFlags,
	LPSTR pszPath)
{
    WCHAR szTemp[MAX_PATH];
    HRESULT hr;

    if (!pszPath)
        return E_INVALIDARG;

    *pszPath = '\0';
    hr = SHGetFolderPathW(hwndOwner, csidl, hToken, dwFlags, szTemp);
    if (SUCCEEDED(hr))
        WideCharToMultiByte(CP_ACP, 0, szTemp, -1, pszPath, MAX_PATH, NULL,
         NULL);

    TRACE("%p,%p,csidl=0x%04x\n",hwndOwner,pszPath,csidl);

    return hr;
}

/*************************************************************************
 * SHGetSpecialFolderPathA [SHELL32.@]
 */
BOOL WINAPI SHGetSpecialFolderPathA (
	HWND hwndOwner,
	LPSTR szPath,
	int csidl,
	BOOL bCreate)
{
	return (SHGetFolderPathA(
		hwndOwner,
		csidl + (bCreate ? CSIDL_FLAG_CREATE : 0),
		NULL,
		0,
		szPath)) == S_OK ? TRUE : FALSE;
}

/*************************************************************************
 * SHGetSpecialFolderPathW
 */
BOOL WINAPI SHGetSpecialFolderPathW (
	HWND hwndOwner,
	LPWSTR szPath,
	int csidl,
	BOOL bCreate)
{
	return (SHGetFolderPathW(
		hwndOwner,
		csidl + (bCreate ? CSIDL_FLAG_CREATE : 0),
		NULL,
		0,
		szPath)) == S_OK ? TRUE : FALSE;
}

/*************************************************************************
 * SHGetSpecialFolderPath (SHELL32.175)
 */
BOOL WINAPI SHGetSpecialFolderPathAW (
	HWND hwndOwner,
	LPVOID szPath,
	int csidl,
	BOOL bCreate)

{
	if (SHELL_OsIsUnicode())
	  return SHGetSpecialFolderPathW (hwndOwner, szPath, csidl, bCreate);
	return SHGetSpecialFolderPathA (hwndOwner, szPath, csidl, bCreate);
}

/*************************************************************************
 * SHGetSpecialFolderLocation		[SHELL32.@]
 *
 * gets the folder locations from the registry and creates a pidl
 * creates missing reg keys and directories
 *
 * PARAMS
 *   hwndOwner [I]
 *   nFolder   [I] CSIDL_xxxxx
 *   ppidl     [O] PIDL of a special folder
 *
 * NOTES
 *   In NT5, SHGetSpecialFolderLocation needs the <winntdir>/Recent
 *   directory. If the directory is missing it returns a x80070002.
 *   In most cases, this forwards to SHGetSpecialFolderPath, but
 *   CSIDLs with virtual folders (not real paths) must be handled
 *   here.
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

    *ppidl = NULL;
    switch (nFolder)
    {
        case CSIDL_DESKTOP:
            *ppidl = _ILCreateDesktop();
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

        case CSIDL_FONTS:
            FIXME("virtual font folder");
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

        case CSIDL_ALTSTARTUP:
        case CSIDL_COMMON_ALTSTARTUP:
            hr = E_FAIL;
            break;

        case CSIDL_COMPUTERSNEARME:
            hr = E_FAIL;
            break;

        default:
        {
            WCHAR szPath[MAX_PATH];

            if (SHGetSpecialFolderPathW(hwndOwner, szPath, nFolder, TRUE))
            {
                DWORD attributes=0;

                TRACE("Value=%s\n", debugstr_w(szPath));
                hr = SHILCreateFromPathW(szPath, ppidl, &attributes);
            }
        }
    }
    if(*ppidl)
        hr = NOERROR;

    TRACE("-- (new pidl %p)\n",*ppidl);
    return hr;
}

/*************************************************************************
 * SHGetFolderLocation [SHELL32.@]
 *
 * NOTES
 *  the pidl can be a simple one. since we can't get the path out of the pidl
 *  we have to take all data from the pidl
 *  Mostly we forward to SHGetSpecialFolderLocation, but a few special cases
 *  we handle here.
 */
HRESULT WINAPI SHGetFolderLocation(
	HWND hwnd,
	int csidl,
	HANDLE hToken,
	DWORD dwFlags,
	LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    TRACE_(shell)("%p 0x%08x %p 0x%08lx %p\n",
     hwnd, csidl, hToken, dwFlags, ppidl);
    
    if (!ppidl)
        return E_INVALIDARG;

    switch (csidl)
    {
        case CSIDL_ALTSTARTUP:
        case CSIDL_COMMON_ALTSTARTUP:
            *ppidl = NULL;
            hr = S_FALSE;
            break;
        default:
            hr = SHGetSpecialFolderLocation(hwnd, csidl, ppidl);
    }
    return hr;
}
