/*
 * Path Functions
 *
 * Copyright 1998, 1999, 2000 Juergen Schmied
 * Copyright 2004 Juan Lang
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

#include <precomp.h>

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
	  wcscpy(pszPath, path);
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
	FIXME("%p %u %s %s %s stub\n",
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
            length = wcslen(lpszPathW);

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
	FIXME("(%s,%p,0x%08x),stub!\n",
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
	FIXME("(%s,%p,0x%08x),stub!\n",
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
LONG WINAPI PathProcessCommandA (
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
LONG WINAPI PathProcessCommandW (
	LPCWSTR lpszPath,
	LPWSTR lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	FIXME("(%s, %p, 0x%04x, 0x%04x) stub\n",
	debugstr_w(lpszPath), lpszBuff, dwBuffSize, dwFlags);
	if(!lpszPath) return -1;
	if(lpszBuff) wcscpy(lpszBuff, lpszPath);
	return wcslen(lpszPath);
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

static const WCHAR szCurrentVersion[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\0'};
static const WCHAR Administrative_ToolsW[] = {'A','d','m','i','n','i','s','t','r','a','t','i','v','e',' ','T','o','o','l','s','\0'};
static const WCHAR AppDataW[] = {'A','p','p','D','a','t','a','\0'};
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
static const WCHAR FavoritesW[] = {'F','a','v','o','r','i','t','e','s','\0'};
static const WCHAR FontsW[] = {'F','o','n','t','s','\0'};
static const WCHAR HistoryW[] = {'H','i','s','t','o','r','y','\0'};
static const WCHAR Local_AppDataW[] = {'L','o','c','a','l',' ','A','p','p','D','a','t','a','\0'};
static const WCHAR My_MusicW[] = {'M','y',' ','M','u','s','i','c','\0'};
static const WCHAR My_PicturesW[] = {'M','y',' ','P','i','c','t','u','r','e','s','\0'};
static const WCHAR My_VideoW[] = {'M','y',' ','V','i','d','e','o','\0'};
static const WCHAR NetHoodW[] = {'N','e','t','H','o','o','d','\0'};
static const WCHAR PersonalW[] = {'P','e','r','s','o','n','a','l','\0'};
static const WCHAR PrintHoodW[] = {'P','r','i','n','t','H','o','o','d','\0'};
static const WCHAR ProgramFilesDirW[] = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r','\0'};
static const WCHAR ProgramsW[] = {'P','r','o','g','r','a','m','s','\0'};
static const WCHAR RecentW[] = {'R','e','c','e','n','t','\0'};
static const WCHAR ResourcesW[] = {'R','e','s','o','u','r','c','e','s','\0'};
static const WCHAR SendToW[] = {'S','e','n','d','T','o','\0'};
static const WCHAR StartUpW[] = {'S','t','a','r','t','U','p','\0'};
static const WCHAR Start_MenuW[] = {'S','t','a','r','t',' ','M','e','n','u','\0'};
static const WCHAR TemplatesW[] = {'T','e','m','p','l','a','t','e','s','\0'};
static const WCHAR DefaultW[] = {'.','D','e','f','a','u','l','t','\0'};
static const WCHAR AllUsersProfileW[] = {'%','A','L','L','U','S','E','R','S','P','R','O','F','I','L','E','%','\0'};
static const WCHAR UserProfileW[] = {'%','U','S','E','R','P','R','O','F','I','L','E','%','\0'};
static const WCHAR SystemDriveW[] = {'%','S','y','s','t','e','m','D','r','i','v','e','%','\0'};
static const WCHAR ProfileListW[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',' ','N','T','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','P','r','o','f','i','l','e','L','i','s','t',0};
static const WCHAR ProfilesDirectoryW[] = {'P','r','o','f','i','l','e','s','D','i','r','e','c','t','o','r','y',0};
static const WCHAR AllUsersProfileValueW[] = {'A','l','l','U','s','e','r','s','P','r','o','f','i','l','e','\0'};
static const WCHAR szSHFolders[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','S','h','e','l','l',' ','F','o','l','d','e','r','s','\0'};
static const WCHAR szSHUserFolders[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','U','s','e','r',' ','S','h','e','l','l',' ','F','o','l','d','e','r','s','\0'};
/* This defaults to L"Documents and Settings" on Windows 2000/XP, but we're
 * acting more Windows 9x-like for now.
 */
static const WCHAR szDefaultProfileDirW[] = {'p','r','o','f','i','l','e','s','\0'};
static const WCHAR AllUsersW[] = {'A','l','l',' ','U','s','e','r','s','\0'};

typedef enum _CSIDL_Type {
    CSIDL_Type_User,
    CSIDL_Type_AllUsers,
    CSIDL_Type_CurrVer,
    CSIDL_Type_Disallowed,
    CSIDL_Type_NonExistent,
    CSIDL_Type_WindowsPath,
    CSIDL_Type_SystemPath,
} CSIDL_Type;

typedef struct
{
    CSIDL_Type type;
    LPCWSTR    szValueName;
    LPCWSTR    szDefaultPath; /* fallback string or resource ID */
} CSIDL_DATA;

static const CSIDL_DATA CSIDL_Data[] =
{
    { /* 0x00 - CSIDL_DESKTOP */
        CSIDL_Type_User,
        DesktopW,
        MAKEINTRESOURCEW(IDS_DESKTOPDIRECTORY)
    },
    { /* 0x01 - CSIDL_INTERNET */
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x02 - CSIDL_PROGRAMS */
        CSIDL_Type_User,
        ProgramsW,
        MAKEINTRESOURCEW(IDS_PROGRAMS)
    },
    { /* 0x03 - CSIDL_CONTROLS (.CPL files) */
        CSIDL_Type_SystemPath,
        NULL,
        NULL
    },
    { /* 0x04 - CSIDL_PRINTERS */
        CSIDL_Type_SystemPath,
        NULL,
        NULL
    },
    { /* 0x05 - CSIDL_PERSONAL */
        CSIDL_Type_User,
        PersonalW,
        MAKEINTRESOURCEW(IDS_PERSONAL)
    },
    { /* 0x06 - CSIDL_FAVORITES */
        CSIDL_Type_User,
        FavoritesW,
        MAKEINTRESOURCEW(IDS_FAVORITES)
    },
    { /* 0x07 - CSIDL_STARTUP */
        CSIDL_Type_User,
        StartUpW,
        MAKEINTRESOURCEW(IDS_STARTUP)
    },
    { /* 0x08 - CSIDL_RECENT */
        CSIDL_Type_User,
        RecentW,
        MAKEINTRESOURCEW(IDS_RECENT)
    },
    { /* 0x09 - CSIDL_SENDTO */
        CSIDL_Type_User,
        SendToW,
        MAKEINTRESOURCEW(IDS_SENDTO)
    },
    { /* 0x0a - CSIDL_BITBUCKET - Recycle Bin */
        CSIDL_Type_Disallowed,
        NULL,
        NULL,
    },
    { /* 0x0b - CSIDL_STARTMENU */
        CSIDL_Type_User,
        Start_MenuW,
        MAKEINTRESOURCEW(IDS_STARTMENU)
    },
    { /* 0x0c - CSIDL_MYDOCUMENTS */
        CSIDL_Type_Disallowed, /* matches WinXP--can't get its path */
        NULL,
        NULL
    },
    { /* 0x0d - CSIDL_MYMUSIC */
        CSIDL_Type_User,
        My_MusicW,
        MAKEINTRESOURCEW(IDS_MYMUSIC)
    },
    { /* 0x0e - CSIDL_MYVIDEO */
        CSIDL_Type_User,
        My_VideoW,
        MAKEINTRESOURCEW(IDS_MYVIDEO)
    },
    { /* 0x0f - unassigned */
        CSIDL_Type_Disallowed,
        NULL,
        NULL,
    },
    { /* 0x10 - CSIDL_DESKTOPDIRECTORY */
        CSIDL_Type_User,
        DesktopW,
        MAKEINTRESOURCEW(IDS_DESKTOPDIRECTORY)
    },
    { /* 0x11 - CSIDL_DRIVES */
        CSIDL_Type_Disallowed,
        NULL,
        NULL,
    },
    { /* 0x12 - CSIDL_NETWORK */
        CSIDL_Type_Disallowed,
        NULL,
        NULL,
    },
    { /* 0x13 - CSIDL_NETHOOD */
        CSIDL_Type_User,
        NetHoodW,
        MAKEINTRESOURCEW(IDS_NETHOOD)
    },
    { /* 0x14 - CSIDL_FONTS */
        CSIDL_Type_WindowsPath,
        FontsW,
        FontsW
    },
    { /* 0x15 - CSIDL_TEMPLATES */
        CSIDL_Type_User,
        TemplatesW,
        MAKEINTRESOURCEW(IDS_TEMPLATES)
    },
    { /* 0x16 - CSIDL_COMMON_STARTMENU */
        CSIDL_Type_AllUsers,
        Common_Start_MenuW,
        MAKEINTRESOURCEW(IDS_STARTMENU)
    },
    { /* 0x17 - CSIDL_COMMON_PROGRAMS */
        CSIDL_Type_AllUsers,
        Common_ProgramsW,
        MAKEINTRESOURCEW(IDS_PROGRAMS)
    },
    { /* 0x18 - CSIDL_COMMON_STARTUP */
        CSIDL_Type_AllUsers,
        Common_StartUpW,
        MAKEINTRESOURCEW(IDS_STARTUP)
    },
    { /* 0x19 - CSIDL_COMMON_DESKTOPDIRECTORY */
        CSIDL_Type_AllUsers,
        Common_DesktopW,
        MAKEINTRESOURCEW(IDS_DESKTOP)
    },
    { /* 0x1a - CSIDL_APPDATA */
        CSIDL_Type_User,
        AppDataW,
        MAKEINTRESOURCEW(IDS_APPDATA)
    },
    { /* 0x1b - CSIDL_PRINTHOOD */
        CSIDL_Type_User,
        PrintHoodW,
        MAKEINTRESOURCEW(IDS_PRINTHOOD)
    },
    { /* 0x1c - CSIDL_LOCAL_APPDATA */
        CSIDL_Type_User,
        Local_AppDataW,
        MAKEINTRESOURCEW(IDS_LOCAL_APPDATA)
    },
    { /* 0x1d - CSIDL_ALTSTARTUP */
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x1e - CSIDL_COMMON_ALTSTARTUP */
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x1f - CSIDL_COMMON_FAVORITES */
        CSIDL_Type_AllUsers,
        FavoritesW,
        MAKEINTRESOURCEW(IDS_FAVORITES)
    },
    { /* 0x20 - CSIDL_INTERNET_CACHE */
        CSIDL_Type_User,
        CacheW,
        MAKEINTRESOURCEW(IDS_INTERNET_CACHE)
    },
    { /* 0x21 - CSIDL_COOKIES */
        CSIDL_Type_User,
        CookiesW,
        MAKEINTRESOURCEW(IDS_COOKIES)
    },
    { /* 0x22 - CSIDL_HISTORY */
        CSIDL_Type_User,
        HistoryW,
        MAKEINTRESOURCEW(IDS_HISTORY)
    },
    { /* 0x23 - CSIDL_COMMON_APPDATA */
        CSIDL_Type_AllUsers,
        Common_AppDataW,
        MAKEINTRESOURCEW(IDS_APPDATA)
    },
    { /* 0x24 - CSIDL_WINDOWS */
        CSIDL_Type_WindowsPath,
        NULL,
        NULL
    },
    { /* 0x25 - CSIDL_SYSTEM */
        CSIDL_Type_SystemPath,
        NULL,
        NULL
    },
    { /* 0x26 - CSIDL_PROGRAM_FILES */
        CSIDL_Type_CurrVer,
        ProgramFilesDirW,
        MAKEINTRESOURCEW(IDS_PROGRAM_FILES)
    },
    { /* 0x27 - CSIDL_MYPICTURES */
        CSIDL_Type_User,
        My_PicturesW,
        MAKEINTRESOURCEW(IDS_MYPICTURES)
    },
    { /* 0x28 - CSIDL_PROFILE */
        CSIDL_Type_User,
        NULL,
        NULL
    },
    { /* 0x29 - CSIDL_SYSTEMX86 */
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x2a - CSIDL_PROGRAM_FILESX86 */
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x2b - CSIDL_PROGRAM_FILES_COMMON */
        CSIDL_Type_CurrVer,
        CommonFilesDirW,
        MAKEINTRESOURCEW(IDS_PROGRAM_FILES_COMMON)
    },
    { /* 0x2c - CSIDL_PROGRAM_FILES_COMMONX86 */
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x2d - CSIDL_COMMON_TEMPLATES */
        CSIDL_Type_AllUsers,
        Common_TemplatesW,
        MAKEINTRESOURCEW(IDS_TEMPLATES)
    },
    { /* 0x2e - CSIDL_COMMON_DOCUMENTS */
        CSIDL_Type_AllUsers,
        Common_DocumentsW,
        MAKEINTRESOURCEW(IDS_COMMON_DOCUMENTS)
    },
    { /* 0x2f - CSIDL_COMMON_ADMINTOOLS */
        CSIDL_Type_AllUsers,
        Common_Administrative_ToolsW,
        MAKEINTRESOURCEW(IDS_ADMINTOOLS)
    },
    { /* 0x30 - CSIDL_ADMINTOOLS */
        CSIDL_Type_User,
        Administrative_ToolsW,
        MAKEINTRESOURCEW(IDS_ADMINTOOLS)
    },
    { /* 0x31 - CSIDL_CONNECTIONS */
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x32 - unassigned */
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x33 - unassigned */
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x34 - unassigned */
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x35 - CSIDL_COMMON_MUSIC */
        CSIDL_Type_AllUsers,
        CommonMusicW,
        MAKEINTRESOURCEW(IDS_COMMON_MUSIC)
    },
    { /* 0x36 - CSIDL_COMMON_PICTURES */
        CSIDL_Type_AllUsers,
        CommonPicturesW,
        MAKEINTRESOURCEW(IDS_COMMON_PICTURES)
    },
    { /* 0x37 - CSIDL_COMMON_VIDEO */
        CSIDL_Type_AllUsers,
        CommonVideoW,
        MAKEINTRESOURCEW(IDS_COMMON_VIDEO)
    },
    { /* 0x38 - CSIDL_RESOURCES */
        CSIDL_Type_WindowsPath,
        NULL,
        ResourcesW
    },
    { /* 0x39 - CSIDL_RESOURCES_LOCALIZED */
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x3a - CSIDL_COMMON_OEM_LINKS */
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x3b - CSIDL_CDBURN_AREA */
        CSIDL_Type_User,
        CD_BurningW,
        MAKEINTRESOURCEW(IDS_CDBURN_AREA)
    },
    { /* 0x3c unassigned */
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x3d - CSIDL_COMPUTERSNEARME */
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x3e - CSIDL_PROFILES */
        CSIDL_Type_Disallowed, /* oddly, this matches WinXP */
        NULL,
        NULL
    }
};

static HRESULT _SHExpandEnvironmentStrings(LPCWSTR szSrc, LPWSTR szDest);

/* Gets the value named value from the registry key
 * rootKey\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders
 * (or from rootKey\userPrefix\... if userPrefix is not NULL) into path, which
 * is assumed to be MAX_PATH WCHARs in length.
 * If it exists, expands the value and writes the expanded value to
 * rootKey\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders
 * Returns successful error code if the value was retrieved from the registry,
 * and a failure otherwise.
 */
static HRESULT _SHGetUserShellFolderPath(HKEY rootKey, LPCWSTR userPrefix,
 LPCWSTR value, LPWSTR path)
{
    HRESULT hr;
    WCHAR shellFolderPath[MAX_PATH], userShellFolderPath[MAX_PATH];
    LPCWSTR pShellFolderPath, pUserShellFolderPath;
    DWORD dwDisp, dwType, dwPathLen = MAX_PATH;
    HKEY userShellFolderKey, shellFolderKey;

    TRACE("%p,%s,%s,%p\n",rootKey, debugstr_w(userPrefix), debugstr_w(value),
     path);

    if (userPrefix)
    {
        wcscpy(shellFolderPath, userPrefix);
        PathAddBackslashW(shellFolderPath);
        wcscat(shellFolderPath, szSHFolders);
        pShellFolderPath = shellFolderPath;
        wcscpy(userShellFolderPath, userPrefix);
        PathAddBackslashW(userShellFolderPath);
        wcscat(userShellFolderPath, szSHUserFolders);
        pUserShellFolderPath = userShellFolderPath;
    }
    else
    {
        pUserShellFolderPath = szSHUserFolders;
        pShellFolderPath = szSHFolders;
    }

    if (RegCreateKeyExW(rootKey, pShellFolderPath, 0, NULL, 0, KEY_ALL_ACCESS,
     NULL, &shellFolderKey, &dwDisp))
    {
        TRACE("Failed to create %s\n", debugstr_w(pShellFolderPath));
        return E_FAIL;
    }
    if (RegCreateKeyExW(rootKey, pUserShellFolderPath, 0, NULL, 0,
     KEY_ALL_ACCESS, NULL, &userShellFolderKey, &dwDisp))
    {
        TRACE("Failed to create %s\n",
         debugstr_w(pUserShellFolderPath));
        RegCloseKey(shellFolderKey);
        return E_FAIL;
    }

    if (!RegQueryValueExW(userShellFolderKey, value, NULL, &dwType,
     (LPBYTE)path, &dwPathLen) && (dwType == REG_EXPAND_SZ || dwType == REG_SZ))
    {
        LONG ret;

        path[dwPathLen / sizeof(WCHAR)] = '\0';
        if (dwType == REG_EXPAND_SZ && path[0] == '%')
        {
            WCHAR szTemp[MAX_PATH];

            _SHExpandEnvironmentStrings(path, szTemp);
            lstrcpynW(path, szTemp, MAX_PATH);
        }
        ret = RegSetValueExW(shellFolderKey, value, 0, REG_SZ, (LPBYTE)path,
         (wcslen(path) + 1) * sizeof(WCHAR));
        if (ret != ERROR_SUCCESS)
            hr = HRESULT_FROM_WIN32(ret);
        else
            hr = S_OK;
    }
    else
        hr = E_FAIL;
    RegCloseKey(shellFolderKey);
    RegCloseKey(userShellFolderKey);
    TRACE("returning 0x%08x\n", hr);
    return hr;
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
static HRESULT _SHGetDefaultValue(BYTE folder, LPWSTR pszPath)
{
    HRESULT hr;
    WCHAR resourcePath[MAX_PATH];
    LPCWSTR pDefaultPath = NULL;

    TRACE("0x%02x,%p\n", folder, pszPath);

    if (folder >= sizeof(CSIDL_Data) / sizeof(CSIDL_Data[0]))
        return E_INVALIDARG;
    if (!pszPath)
        return E_INVALIDARG;

    if (CSIDL_Data[folder].szDefaultPath &&
     IS_INTRESOURCE(CSIDL_Data[folder].szDefaultPath))
    {
        if (LoadStringW(shell32_hInstance,
         LOWORD(CSIDL_Data[folder].szDefaultPath), resourcePath, MAX_PATH))
        {
            hr = S_OK;
            pDefaultPath = resourcePath;
        }
        else
        {
            FIXME("(%d,%s), LoadString failed, missing translation?\n", folder,
             debugstr_w(pszPath));
            hr = E_FAIL;
        }
    }
    else
    {
        hr = S_OK;
        pDefaultPath = CSIDL_Data[folder].szDefaultPath;
    }
    if (SUCCEEDED(hr))
    {
        switch (CSIDL_Data[folder].type)
        {
            case CSIDL_Type_User:
                wcscpy(pszPath, UserProfileW);
                break;
            case CSIDL_Type_AllUsers:
                wcscpy(pszPath, AllUsersProfileW);
                break;
            case CSIDL_Type_CurrVer:
                wcscpy(pszPath, SystemDriveW);
                break;
            default:
                ; /* no corresponding env. var, do nothing */
        }
        if (pDefaultPath)
        {
            PathAddBackslashW(pszPath);
            wcscat(pszPath, pDefaultPath);
        }
    }
    TRACE("returning 0x%08x\n", hr);
    return hr;
}

/* Gets the (unexpanded) value of the folder with index folder into pszPath.
 * The folder's type is assumed to be CSIDL_Type_CurrVer.  Its default value
 * can be overridden in the HKLM\\szCurrentVersion key.
 * If dwFlags has SHGFP_TYPE_DEFAULT set or if the value isn't overridden in
 * the registry, uses _SHGetDefaultValue to get the value.
 */
static HRESULT _SHGetCurrentVersionPath(DWORD dwFlags, BYTE folder,
 LPWSTR pszPath)
{
    HRESULT hr;

    TRACE("0x%08x,0x%02x,%p\n", dwFlags, folder, pszPath);

    if (folder >= sizeof(CSIDL_Data) / sizeof(CSIDL_Data[0]))
        return E_INVALIDARG;
    if (CSIDL_Data[folder].type != CSIDL_Type_CurrVer)
        return E_INVALIDARG;
    if (!pszPath)
        return E_INVALIDARG;

    if (dwFlags & SHGFP_TYPE_DEFAULT)
        hr = _SHGetDefaultValue(folder, pszPath);
    else
    {
        HKEY hKey;
        DWORD dwDisp;

        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, szCurrentVersion, 0,
         NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp))
            hr = E_FAIL;
        else
        {
            DWORD dwType, dwPathLen = MAX_PATH * sizeof(WCHAR);

            if (RegQueryValueExW(hKey, CSIDL_Data[folder].szValueName, NULL,
             &dwType, (LPBYTE)pszPath, &dwPathLen) ||
             (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
            {
                hr = _SHGetDefaultValue(folder, pszPath);
                dwType = REG_EXPAND_SZ;
                RegSetValueExW(hKey, CSIDL_Data[folder].szValueName, 0, dwType,
                 (LPBYTE)pszPath, (wcslen(pszPath)+1)*sizeof(WCHAR));
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
    HRESULT hr;

    TRACE("%p,0x%08x,0x%02x,%p\n", hToken, dwFlags, folder, pszPath);

    if (folder >= sizeof(CSIDL_Data) / sizeof(CSIDL_Data[0]))
        return E_INVALIDARG;

    if (CSIDL_Data[folder].type != CSIDL_Type_User)
        return E_INVALIDARG;

    if (!pszPath)
        return E_INVALIDARG;

    if (dwFlags & SHGFP_TYPE_DEFAULT)
    {
        hr = _SHGetDefaultValue(folder, pszPath);
    }
    else
    {
        LPWSTR userPrefix;
        HKEY hRootKey;

        if (hToken == (HANDLE)-1)
        {
            /* Get the folder of the default user */
            hRootKey = HKEY_USERS;
            userPrefix = (LPWSTR)DefaultW;
        }
        else if(!hToken)
        {
            /* Get the folder of the current user */
            hRootKey = HKEY_CURRENT_USER;
            userPrefix = NULL;
        }
        else
        {
            /* Get the folder of the specified user */
            DWORD InfoLength;
            PTOKEN_USER UserInfo;

            hRootKey = HKEY_USERS;

            GetTokenInformation(hToken, TokenUser, NULL, 0, &InfoLength);
            UserInfo = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, InfoLength);

            if(!GetTokenInformation(hToken, TokenUser, UserInfo, InfoLength, &InfoLength))
            {
                WARN("GetTokenInformation failed for %x!\n", hToken);
                HeapFree(GetProcessHeap(), 0, UserInfo);
                return E_FAIL;
            }

            if(!ConvertSidToStringSidW(UserInfo->User.Sid, &userPrefix))
            {
                WARN("ConvertSidToStringSidW failed for %x!\n", hToken);
                HeapFree(GetProcessHeap(), 0, UserInfo);
                return E_FAIL;
            }

            HeapFree(GetProcessHeap(), 0, UserInfo);
        }

        hr = _SHGetUserShellFolderPath(hRootKey, userPrefix, CSIDL_Data[folder].szValueName, pszPath);

        /* Free the memory allocated by ConvertSidToStringSidW */
        if(hToken && hToken != (HANDLE)-1)
            LocalFree(userPrefix);

        if (FAILED(hr) && hRootKey != HKEY_LOCAL_MACHINE)
            hr = _SHGetUserShellFolderPath(HKEY_LOCAL_MACHINE, NULL, CSIDL_Data[folder].szValueName, pszPath);

        if (FAILED(hr))
            hr = _SHGetDefaultValue(folder, pszPath);
    }

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

    if (folder >= sizeof(CSIDL_Data) / sizeof(CSIDL_Data[0]))
        return E_INVALIDARG;
    if (CSIDL_Data[folder].type != CSIDL_Type_AllUsers)
        return E_INVALIDARG;
    if (!pszPath)
        return E_INVALIDARG;

    if (dwFlags & SHGFP_TYPE_DEFAULT)
        hr = _SHGetDefaultValue(folder, pszPath);
    else
    {
        hr = _SHGetUserShellFolderPath(HKEY_LOCAL_MACHINE, NULL,
         CSIDL_Data[folder].szValueName, pszPath);
        if (FAILED(hr))
            hr = _SHGetDefaultValue(folder, pszPath);
    }
    TRACE("returning 0x%08x (output path is %s)\n", hr, debugstr_w(pszPath));
    return hr;
}

static HRESULT _SHOpenProfilesKey(PHKEY pKey)
{
    LONG lRet;
    DWORD disp;

    lRet = RegCreateKeyExW(HKEY_LOCAL_MACHINE, ProfileListW, 0, NULL, 0,
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
                              (wcslen(szValue) + 1) * sizeof(WCHAR));
        if (lRet)
            hr = HRESULT_FROM_WIN32(lRet);
        else
            hr = S_OK;
    }
    TRACE("returning 0x%08x (output value is %s)\n", hr, debugstr_w(szValue));
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
static HRESULT _SHExpandEnvironmentStrings(LPCWSTR szSrc, LPWSTR szDest)
{
    HRESULT hr;
    WCHAR szTemp[MAX_PATH], szProfilesPrefix[MAX_PATH] = { 0 };
    HKEY key = NULL;

    TRACE("%s, %p\n", debugstr_w(szSrc), szDest);

    if (!szSrc || !szDest) return E_INVALIDARG;

    /* short-circuit if there's nothing to expand */
    if (szSrc[0] != '%')
    {
        wcscpy(szDest, szSrc);
        hr = S_OK;
        goto end;
    }
    /* Get the profile prefix, we'll probably be needing it */
    hr = _SHOpenProfilesKey(&key);
    if (SUCCEEDED(hr))
    {
        WCHAR szDefaultProfilesPrefix[MAX_PATH];

        GetWindowsDirectoryW(szDefaultProfilesPrefix, MAX_PATH);
        PathAddBackslashW(szDefaultProfilesPrefix);
        PathAppendW(szDefaultProfilesPrefix, szDefaultProfileDirW);
        hr = _SHGetProfilesValue(key, ProfilesDirectoryW, szProfilesPrefix,
         szDefaultProfilesPrefix);
    }

    *szDest = 0;
    wcscpy(szTemp, szSrc);
    while (SUCCEEDED(hr) && szTemp[0] == '%')
    {
        if (!strncmpiW(szTemp, AllUsersProfileW, wcslen(AllUsersProfileW)))
        {
            WCHAR szAllUsers[MAX_PATH];

            wcscpy(szDest, szProfilesPrefix);
            hr = _SHGetProfilesValue(key, AllUsersProfileValueW,
             szAllUsers, AllUsersW);
            PathAppendW(szDest, szAllUsers);
            PathAppendW(szDest, szTemp + wcslen(AllUsersProfileW));
        }
        else if (!strncmpiW(szTemp, UserProfileW, wcslen(UserProfileW)))
        {
            WCHAR userName[MAX_PATH];
            DWORD userLen = MAX_PATH;

            wcscpy(szDest, szProfilesPrefix);
            GetUserNameW(userName, &userLen);
            PathAppendW(szDest, userName);
            PathAppendW(szDest, szTemp + wcslen(UserProfileW));
        }
        else if (!strncmpiW(szTemp, SystemDriveW, wcslen(SystemDriveW)))
        {
            GetSystemDirectoryW(szDest, MAX_PATH);
            if (szDest[1] != ':')
            {
                FIXME("non-drive system paths unsupported\n");
                hr = E_FAIL;
            }
            else
            {
                wcscpy(szDest + 3, szTemp + wcslen(SystemDriveW) + 1);
                hr = S_OK;
            }
        }
        else
        {
            DWORD ret = ExpandEnvironmentStringsW(szSrc, szDest, MAX_PATH);

            if (ret > MAX_PATH)
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            else if (ret == 0)
                hr = HRESULT_FROM_WIN32(GetLastError());
            else
                hr = S_OK;
        }
        if (SUCCEEDED(hr) && szDest[0] == '%')
            wcscpy(szTemp, szDest);
        else
        {
            /* terminate loop */
            szTemp[0] = '\0';
        }
    }
end:
    if (key)
        RegCloseKey(key);
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
    TRACE("%08x,%08x,%s\n",nFolder, dwFlags, debugstr_w(pszSubPathW));

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
    DWORD      folder = nFolder & CSIDL_FOLDER_MASK; //FIXME
    CSIDL_Type type;
    int        ret;

    TRACE("%p,%p,nFolder=0x%04x,%s\n", hwndOwner,pszPath,nFolder,debugstr_w(pszSubPath));

    /* Windows always NULL-terminates the resulting path regardless of success
     * or failure, so do so first
     */
    if (pszPath)
        *pszPath = '\0';

    if (folder >= sizeof(CSIDL_Data) / sizeof(CSIDL_Data[0]))
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
                wcscat(szTemp, CSIDL_Data[folder].szDefaultPath);
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
                wcscat(szTemp, CSIDL_Data[folder].szDefaultPath);
            }
            hr = S_OK;
            break;
        case CSIDL_Type_CurrVer:
            hr = _SHGetCurrentVersionPath(dwFlags, folder, szTemp);
            break;
        case CSIDL_Type_User:
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
        hr = _SHExpandEnvironmentStrings(szTemp, szBuildPath);
    else
        wcscpy(szBuildPath, szTemp);

    if (FAILED(hr)) goto end;

    if(pszSubPath) {
        /* make sure the new path does not exceed th bufferlength
         * rememebr to backslash and the termination */
        if(MAX_PATH < (wcslen(szBuildPath) + wcslen(pszSubPath) + 2)) {
            hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
            goto end;
        }
        PathAppendW(szBuildPath, pszSubPath);
        PathRemoveBackslashW(szBuildPath);
    }
    /* Copy the path if it's available before we might return */
    if (SUCCEEDED(hr) && pszPath)
        wcscpy(pszPath, szBuildPath);

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

    TRACE("%p,%p,nFolder=0x%04x\n",hwndOwner,pszPath,nFolder);

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
    UINT i;
    WCHAR path[MAX_PATH];
    HRESULT hr = S_OK;
    HKEY hUserKey = NULL, hKey = NULL;
    DWORD dwDisp, dwType, dwPathLen;
    LONG ret;

    TRACE("%p,%p,%s,%p,%u\n", hRootKey, hToken,
     debugstr_w(szUserShellFolderPath), folders, foldersLen);

    ret = RegCreateKeyExW(hRootKey, szUserShellFolderPath, 0, NULL, 0,
     KEY_ALL_ACCESS, NULL, &hUserKey, &dwDisp);
    if (ret)
        hr = HRESULT_FROM_WIN32(ret);
    else
    {
        ret = RegCreateKeyExW(hRootKey, szShellFolderPath, 0, NULL, 0,
         KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
        if (ret)
            hr = HRESULT_FROM_WIN32(ret);
    }
    for (i = 0; SUCCEEDED(hr) && i < foldersLen; i++)
    {
        dwPathLen = MAX_PATH * sizeof(WCHAR);
        if (RegQueryValueExW(hUserKey, CSIDL_Data[folders[i]].szValueName, NULL,
         &dwType, (LPBYTE)path, &dwPathLen) || (dwType != REG_SZ &&
         dwType != REG_EXPAND_SZ))
        {
            *path = '\0';
            if (CSIDL_Data[folders[i]].type == CSIDL_Type_User)
                _SHGetUserProfilePath(hToken, SHGFP_TYPE_DEFAULT, folders[i],
                 path);
            else if (CSIDL_Data[folders[i]].type == CSIDL_Type_AllUsers)
                _SHGetAllUsersProfilePath(SHGFP_TYPE_DEFAULT, folders[i], path);
            else if (CSIDL_Data[folders[i]].type == CSIDL_Type_WindowsPath)
                GetWindowsDirectoryW(path, MAX_PATH);
            else
                hr = E_FAIL;
            if (*path)
            {
                ret = RegSetValueExW(hUserKey,
                 CSIDL_Data[folders[i]].szValueName, 0, REG_EXPAND_SZ,
                 (LPBYTE)path, (wcslen(path) + 1) * sizeof(WCHAR));
                if (ret)
                    hr = HRESULT_FROM_WIN32(ret);
                else
                {
                    hr = SHGetFolderPathW(NULL, folders[i] | CSIDL_FLAG_CREATE,
                     hToken, SHGFP_TYPE_DEFAULT, path);
                    ret = RegSetValueExW(hKey,
                     CSIDL_Data[folders[i]].szValueName, 0, REG_SZ,
                     (LPBYTE)path, (wcslen(path) + 1) * sizeof(WCHAR));
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
     CSIDL_FONTS
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
        wcscpy(userShellFolderPath, DefaultW);
        PathAddBackslashW(userShellFolderPath);
        wcscat(userShellFolderPath, szSHUserFolders);
        pUserShellFolderPath = userShellFolderPath;
        wcscpy(shellFolderPath, DefaultW);
        PathAddBackslashW(shellFolderPath);
        wcscat(shellFolderPath, szSHFolders);
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
     pShellFolderPath, folders, sizeof(folders) / sizeof(folders[0]));
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
    };
    HRESULT hr;

    TRACE("\n");
    hr = _SHRegisterFolders(HKEY_LOCAL_MACHINE, NULL, szSHUserFolders,
     szSHFolders, folders, sizeof(folders) / sizeof(folders[0]));
    TRACE("returning 0x%08x\n", hr);
    return hr;
}

/******************************************************************************
 * _SHAppendToUnixPath  [Internal]
 *
 * Helper function for _SHCreateSymbolicLinks. Appends pwszSubPath (or the
 * corresponding resource, if IS_INTRESOURCE) to the unix base path 'szBasePath'
 * and replaces backslashes with slashes.
 *
 * PARAMS
 *  szBasePath  [IO] The unix base path, which will be appended to (CP_UNXICP).
 *  pwszSubPath [I]  Sub-path or resource id (use MAKEINTRESOURCEW).
 *
 * RETURNS
 *  Success: TRUE,
 *  Failure: FALSE
 */
static BOOL __inline _SHAppendToUnixPath(char *szBasePath, LPCWSTR pwszSubPath) {
    WCHAR wszSubPath[MAX_PATH];
    int cLen = strlen(szBasePath);
    char *pBackslash;

    if (IS_INTRESOURCE(pwszSubPath)) {
        if (!LoadStringW(shell32_hInstance, LOWORD(pwszSubPath), wszSubPath, MAX_PATH)) {
            /* Fall back to hard coded defaults. */
            switch (LOWORD(pwszSubPath)) {
                case IDS_PERSONAL:
                    wcscpy(wszSubPath, PersonalW);
                    break;
                case IDS_MYMUSIC:
                    wcscpy(wszSubPath, My_MusicW);
                    break;
                case IDS_MYPICTURES:
                    wcscpy(wszSubPath, My_PicturesW);
                    break;
                case IDS_MYVIDEO:
                    wcscpy(wszSubPath, My_VideoW);
                    break;
                default:
                    ERR("LoadString(%d) failed!\n", LOWORD(pwszSubPath));
                    return FALSE;
            }
        }
    } else {
        wcscpy(wszSubPath, pwszSubPath);
    }

    if (szBasePath[cLen-1] != '/') szBasePath[cLen++] = '/';

    if (!WideCharToMultiByte(CP_ACP, 0, wszSubPath, -1, szBasePath + cLen,
                             FILENAME_MAX - cLen, NULL, NULL))
    {
        return FALSE;
    }

    pBackslash = szBasePath + cLen;
    while ((pBackslash = strchr(pBackslash, '\\'))) *pBackslash = '/';

    return TRUE;
}
#if 0
/******************************************************************************
 * _SHCreateSymbolicLinks  [Internal]
 *
 * Sets up symbol links for various shell folders to point into the users home
 * directory. We do an educated guess about what the user would probably want:
 * - If there is a 'My Documents' directory in $HOME, the user probably wants
 *   wine's 'My Documents' to point there. Furthermore, we imply that the user
 *   is a Windows lover and has no problem with wine creating 'My Pictures',
 *   'My Music' and 'My Video' subfolders under '$HOME/My Documents', if those
 *   do not already exits. We put appropriate symbolic links in place for those,
 *   too.
 * - If there is no 'My Documents' directory in $HOME, we let 'My Documents'
 *   point directly to $HOME. We assume the user to be a unix hacker who does not
 *   want wine to create anything anywhere besides the .wine directory. So, if
 *   there already is a 'My Music' directory in $HOME, we symlink the 'My Music'
 *   shell folder to it. But if not, we symlink it to $HOME directly. The same
 *   holds fo 'My Pictures' and 'My Video'.
 * - The Desktop shell folder is symlinked to '$HOME/Desktop', if that does
 *   exists and left alone if not.
 * ('My Music',... above in fact means LoadString(IDS_MYMUSIC))
 */
static void _SHCreateSymbolicLinks(void)
{
    UINT aidsMyStuff[] = { IDS_MYPICTURES, IDS_MYVIDEO, IDS_MYMUSIC }, i;
    int acsidlMyStuff[] = { CSIDL_MYPICTURES, CSIDL_MYVIDEO, CSIDL_MYMUSIC };
    WCHAR wszTempPath[MAX_PATH];
    char szPersonalTarget[FILENAME_MAX], *pszPersonal;
    char szMyStuffTarget[FILENAME_MAX], *pszMyStuff;
    char szDesktopTarget[FILENAME_MAX], *pszDesktop;
    struct stat statFolder;
    const char *pszHome;
    HRESULT hr;

    /* Create all necessary profile sub-dirs up to 'My Documents' and get the unix path. */
    hr = SHGetFolderPathW(NULL, CSIDL_PERSONAL|CSIDL_FLAG_CREATE, NULL,
                          SHGFP_TYPE_DEFAULT, wszTempPath);
    if (FAILED(hr)) return;
    pszPersonal = wine_get_unix_file_name(wszTempPath);
    if (!pszPersonal) return;

    pszHome = getenv("HOME");
    if (pszHome && !stat(pszHome, &statFolder) && S_ISDIR(statFolder.st_mode)) {
        strcpy(szPersonalTarget, pszHome);
        if (_SHAppendToUnixPath(szPersonalTarget, MAKEINTRESOURCEW(IDS_PERSONAL)) &&
            !stat(szPersonalTarget, &statFolder) && S_ISDIR(statFolder.st_mode))
        {
            /* '$HOME/My Documents' exists. Create 'My Pictures', 'My Videos' and
             * 'My Music' subfolders or fail silently if they already exist. */
            for (i = 0; i < sizeof(aidsMyStuff)/sizeof(aidsMyStuff[0]); i++) {
                strcpy(szMyStuffTarget, szPersonalTarget);
                if (_SHAppendToUnixPath(szMyStuffTarget, MAKEINTRESOURCEW(aidsMyStuff[i])))
                    mkdir(szMyStuffTarget);
            }
        }
        else
        {
            /* '$HOME/My Documents' doesn't exists, but '$HOME' does. */
            strcpy(szPersonalTarget, pszHome);
        }

        /* Replace 'My Documents' directory with a symlink of fail silently if not empty. */
        rmdir(pszPersonal);
        symlink(szPersonalTarget, pszPersonal);
    }
    else
    {
        /* '$HOME' doesn't exist. Create 'My Pictures', 'My Videos' and 'My Music' subdirs
         * in '%USERPROFILE%\\My Documents' or fail silently if they already exist. */
        strcpy(szPersonalTarget, pszPersonal);
        for (i = 0; i < sizeof(aidsMyStuff)/sizeof(aidsMyStuff[0]); i++) {
            strcpy(szMyStuffTarget, szPersonalTarget);
            if (_SHAppendToUnixPath(szMyStuffTarget, MAKEINTRESOURCEW(aidsMyStuff[i])))
                mkdir(szMyStuffTarget);
        }
    }

    /* Create symbolic links for 'My Pictures', 'My Video' and 'My Music'. */
    for (i=0; i < sizeof(aidsMyStuff)/sizeof(aidsMyStuff[0]); i++) {
        /* Create the current 'My Whatever' folder and get it's unix path. */
        hr = SHGetFolderPathW(NULL, acsidlMyStuff[i]|CSIDL_FLAG_CREATE, NULL,
                              SHGFP_TYPE_DEFAULT, wszTempPath);
        if (FAILED(hr)) continue;
        pszMyStuff = wine_get_unix_file_name(wszTempPath);
        if (!pszMyStuff) continue;

        strcpy(szMyStuffTarget, szPersonalTarget);
        if (_SHAppendToUnixPath(szMyStuffTarget, MAKEINTRESOURCEW(aidsMyStuff[i])) &&
            !stat(szMyStuffTarget, &statFolder) && S_ISDIR(statFolder.st_mode))
        {
            /* If there's a 'My Whatever' directory where 'My Documents' links to, link to it. */
            rmdir(pszMyStuff);
            symlink(szMyStuffTarget, pszMyStuff);
        }
        else
        {
            /* Else link to where 'My Documents' itself links to. */
            rmdir(pszMyStuff);
            symlink(szPersonalTarget, pszMyStuff);
        }
        HeapFree(GetProcessHeap(), 0, pszMyStuff);
    }

    /* Last but not least, the Desktop folder */
    if (pszHome)
        strcpy(szDesktopTarget, pszHome);
    else
        strcpy(szDesktopTarget, pszPersonal);
    HeapFree(GetProcessHeap(), 0, pszPersonal);

    if (_SHAppendToUnixPath(szDesktopTarget, DesktopW) &&
        !stat(szDesktopTarget, &statFolder) && S_ISDIR(statFolder.st_mode))
    {
        hr = SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY|CSIDL_FLAG_CREATE, NULL,
                              SHGFP_TYPE_DEFAULT, wszTempPath);
        if (SUCCEEDED(hr) && (pszDesktop = wine_get_unix_file_name(wszTempPath)))
        {
            rmdir(pszDesktop);
            symlink(szDesktopTarget, pszDesktop);
            HeapFree(GetProcessHeap(), 0, pszDesktop);
        }
    }
}
#endif

/* Register the default values in the registry, as some apps seem to depend
 * on their presence.  The set registered was taken from Windows XP.
 */
HRESULT SHELL_RegisterShellFolders(void)
{
    HRESULT hr;

    /* Set up '$HOME' targeted symlinks for 'My Documents', 'My Pictures',
     * 'My Video', 'My Music' and 'Desktop' in advance, so that the
     * _SHRegister*ShellFolders() functions will find everything nice and clean
     * and thus will not attempt to create them in the profile directory. */
#if 0
    _SHCreateSymbolicLinks();
#endif

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
	return (SHGetFolderPathA(
		hwndOwner,
		nFolder + (bCreate ? CSIDL_FLAG_CREATE : 0),
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
	int nFolder,
	BOOL bCreate)
{
	return (SHGetFolderPathW(
		hwndOwner,
		nFolder + (bCreate ? CSIDL_FLAG_CREATE : 0),
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

    TRACE("%p 0x%08x %p 0x%08x %p\n",
     hwndOwner, nFolder, hToken, dwReserved, ppidl);

    if (!ppidl)
        return E_INVALIDARG;
    if (dwReserved)
        return E_INVALIDARG;

    /* The virtual folders' locations are not user-dependent */
    *ppidl = NULL;
    switch (nFolder)
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
        hr = NOERROR;

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
