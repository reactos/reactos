/*
 * SHLWAPI.DLL functions
 *
 * Copyright (C) 2000 Juergen Schmied
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

#ifndef __WINE_SHLWAPI_H
#define __WINE_SHLWAPI_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifndef NO_SHLWAPI_REG

/* Registry functions */

DWORD WINAPI SHDeleteEmptyKeyA(HKEY,LPCSTR);
DWORD WINAPI SHDeleteEmptyKeyW(HKEY,LPCWSTR);
#define SHDeleteEmptyKey WINELIB_NAME_AW(SHDeleteEmptyKey)

DWORD WINAPI SHDeleteKeyA(HKEY,LPCSTR);
DWORD WINAPI SHDeleteKeyW(HKEY,LPCWSTR);
#define SHDeleteKey WINELIB_NAME_AW(SHDeleteKey)

DWORD WINAPI SHDeleteValueA(HKEY,LPCSTR,LPCSTR);
DWORD WINAPI SHDeleteValueW(HKEY,LPCWSTR,LPCWSTR);
#define SHDeleteValue WINELIB_NAME_AW(SHDeleteValue)

DWORD WINAPI SHGetValueA(HKEY,LPCSTR,LPCSTR,LPDWORD,LPVOID,LPDWORD);
DWORD WINAPI SHGetValueW(HKEY,LPCWSTR,LPCWSTR,LPDWORD,LPVOID,LPDWORD);
#define SHGetValue WINELIB_NAME_AW(SHGetValue)

DWORD WINAPI SHSetValueA(HKEY,LPCSTR,LPCSTR,DWORD,LPCVOID,DWORD);
DWORD WINAPI SHSetValueW(HKEY,LPCWSTR,LPCWSTR,DWORD,LPCVOID,DWORD);
#define SHSetValue WINELIB_NAME_AW(SHSetValue)

DWORD WINAPI SHQueryValueExA(HKEY,LPCSTR,LPDWORD,LPDWORD,LPVOID,LPDWORD);
DWORD WINAPI SHQueryValueExW(HKEY,LPCWSTR,LPDWORD,LPDWORD,LPVOID,LPDWORD);
#define SHQueryValueEx WINELIB_NAME_AW(SHQueryValueEx)

LONG WINAPI SHEnumKeyExA(HKEY,DWORD,LPSTR,LPDWORD);
LONG WINAPI SHEnumKeyExW(HKEY,DWORD,LPWSTR,LPDWORD);
#define SHEnumKeyEx WINELIB_NAME_AW(SHEnumKeyEx)

LONG WINAPI SHEnumValueA(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPVOID,LPDWORD);
LONG WINAPI SHEnumValueW(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPVOID,LPDWORD);
#define SHEnumValue WINELIB_NAME_AW(SHEnumValue)

LONG WINAPI SHQueryInfoKeyA(HKEY,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
LONG WINAPI SHQueryInfoKeyW(HKEY,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
#define SHQueryInfoKey WINELIB_NAME_AW(SHQueryInfoKey)

DWORD WINAPI SHRegGetPathA(HKEY,LPCSTR,LPCSTR,LPSTR,DWORD);
DWORD WINAPI SHRegGetPathW(HKEY,LPCWSTR,LPCWSTR,LPWSTR,DWORD);
#define SHRegGetPath WINELIB_NAME_AW(SHRegGetPath)

DWORD WINAPI SHRegSetPathA(HKEY,LPCSTR,LPCSTR,LPCSTR,DWORD);
DWORD WINAPI SHRegSetPathW(HKEY,LPCWSTR,LPCWSTR,LPCWSTR,DWORD);
#define SHRegSetPath WINELIB_NAME_AW(SHRegSetPath)

DWORD WINAPI SHCopyKeyA(HKEY,LPCSTR,HKEY,DWORD);
DWORD WINAPI SHCopyKeyW(HKEY,LPCWSTR,HKEY,DWORD);
#define SHCopyKey WINELIB_NAME_AW(SHCopyKey)

/* Undocumented registry functions */

HKEY WINAPI SHRegDuplicateHKey(HKEY hKey);

/* FIXME: SHDeleteOrphanKey */


/* User registry functions */

typedef enum
{
  SHREGDEL_DEFAULT = 0,
  SHREGDEL_HKCU    = 0x1,
  SHREGDEL_HKLM    = 0x10,
  SHREGDEL_BOTH    = SHREGDEL_HKLM | SHREGDEL_HKCU
} SHREGDEL_FLAGS;

typedef enum
{
  SHREGENUM_DEFAULT = 0,
  SHREGENUM_HKCU    = 0x1,
  SHREGENUM_HKLM    = 0x10,
  SHREGENUM_BOTH    = SHREGENUM_HKLM | SHREGENUM_HKCU
} SHREGENUM_FLAGS;

#define SHREGSET_HKCU       0x1 /* Apply to HKCU if empty */
#define SHREGSET_FORCE_HKCU 0x2 /* Always apply to HKCU */
#define SHREGSET_HKLM       0x4 /* Apply to HKLM if empty */
#define SHREGSET_FORCE_HKLM 0x8 /* Always apply to HKLM */
#define SHREGSET_DEFAULT    (SHREGSET_FORCE_HKCU | SHREGSET_HKLM)

typedef HANDLE HUSKEY;
typedef HUSKEY *PHUSKEY;

LONG WINAPI SHRegCreateUSKeyA(LPCSTR,REGSAM,HUSKEY,PHUSKEY,DWORD);
LONG WINAPI SHRegCreateUSKeyW(LPCWSTR,REGSAM,HUSKEY,PHUSKEY,DWORD);
#define SHRegCreateUSKey WINELIB_NAME_AW(SHRegCreateUSKey)

LONG WINAPI SHRegOpenUSKeyA(LPCSTR,REGSAM,HUSKEY,PHUSKEY,BOOL);
LONG WINAPI SHRegOpenUSKeyW(LPCWSTR,REGSAM,HUSKEY,PHUSKEY,BOOL);
#define SHRegOpenUSKey WINELIB_NAME_AW(SHRegOpenUSKey)

LONG WINAPI SHRegQueryUSValueA(HUSKEY,LPCSTR,LPDWORD,LPVOID,LPDWORD,
                               BOOL,LPVOID,DWORD);
LONG WINAPI SHRegQueryUSValueW(HUSKEY,LPCWSTR,LPDWORD,LPVOID,LPDWORD,
                               BOOL,LPVOID,DWORD);
#define SHRegQueryUSValue WINELIB_NAME_AW(SHRegQueryUSValue)

LONG WINAPI SHRegWriteUSValueA(HUSKEY,LPCSTR,DWORD,LPVOID,DWORD,DWORD);
LONG WINAPI SHRegWriteUSValueW(HUSKEY,LPCWSTR,DWORD,LPVOID,DWORD,DWORD);
#define SHRegWriteUSValue WINELIB_NAME_AW(SHRegWriteUSValue)

LONG WINAPI SHRegDeleteUSValueA(HUSKEY,LPCSTR,SHREGDEL_FLAGS);
LONG WINAPI SHRegDeleteUSValueW(HUSKEY,LPCWSTR,SHREGDEL_FLAGS);
#define SHRegDeleteUSValue WINELIB_NAME_AW(SHRegDeleteUSValue)

LONG WINAPI SHRegDeleteEmptyUSKeyA(HUSKEY,LPCSTR,SHREGDEL_FLAGS);
LONG WINAPI SHRegDeleteEmptyUSKeyW(HUSKEY,LPCWSTR,SHREGDEL_FLAGS);
#define SHRegDeleteEmptyUSKey WINELIB_NAME_AW(SHRegDeleteEmptyUSKey)

LONG WINAPI SHRegEnumUSKeyA(HUSKEY,DWORD,LPSTR,LPDWORD,SHREGENUM_FLAGS);
LONG WINAPI SHRegEnumUSKeyW(HUSKEY,DWORD,LPWSTR,LPDWORD,SHREGENUM_FLAGS);
#define SHRegEnumUSKey WINELIB_NAME_AW(SHRegEnumUSKey)

LONG WINAPI SHRegEnumUSValueA(HUSKEY,DWORD,LPSTR,LPDWORD,LPDWORD,
                              LPVOID,LPDWORD,SHREGENUM_FLAGS);
LONG WINAPI SHRegEnumUSValueW(HUSKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,
                              LPVOID,LPDWORD,SHREGENUM_FLAGS);
#define SHRegEnumUSValue WINELIB_NAME_AW(SHRegEnumUSValue)

LONG WINAPI SHRegQueryInfoUSKeyA(HUSKEY,LPDWORD,LPDWORD,LPDWORD,
                                 LPDWORD,SHREGENUM_FLAGS);
LONG WINAPI SHRegQueryInfoUSKeyW(HUSKEY,LPDWORD,LPDWORD,LPDWORD,
                                 LPDWORD,SHREGENUM_FLAGS);
#define SHRegQueryInfoUSKey WINELIB_NAME_AW(SHRegQueryInfoUSKey)

LONG WINAPI SHRegCloseUSKey(HUSKEY);

LONG WINAPI SHRegGetUSValueA(LPCSTR,LPCSTR,LPDWORD,LPVOID,LPDWORD,
                             BOOL,LPVOID,DWORD);
LONG WINAPI SHRegGetUSValueW(LPCWSTR,LPCWSTR,LPDWORD,LPVOID,LPDWORD,
                             BOOL,LPVOID,DWORD);
#define SHRegGetUSValue WINELIB_NAME_AW(SHRegGetUSValue)

LONG WINAPI SHRegSetUSValueA(LPCSTR,LPCSTR,DWORD,LPVOID,DWORD,DWORD);
LONG WINAPI SHRegSetUSValueW(LPCWSTR,LPCWSTR,DWORD,LPVOID,DWORD,DWORD);
#define SHRegSetUSValue WINELIB_NAME_AW(SHRegSetUSValue)

BOOL WINAPI SHRegGetBoolUSValueA(LPCSTR,LPCSTR,BOOL,BOOL);
BOOL WINAPI SHRegGetBoolUSValueW(LPCWSTR,LPCWSTR,BOOL,BOOL);
#define SHRegGetBoolUSValue WINELIB_NAME_AW(SHRegGetBoolUSValue)

#endif /* NO_SHLWAPI_REG */


/* Path functions */
#ifndef NO_SHLWAPI_PATH

/* GetPathCharType return flags */
#define GCT_INVALID     0x0
#define GCT_LFNCHAR     0x1
#define GCT_SHORTCHAR   0x2
#define GCT_WILD        0x4
#define GCT_SEPARATOR   0x8

LPSTR  WINAPI PathAddBackslashA(LPSTR);
LPWSTR WINAPI PathAddBackslashW(LPWSTR);
#define PathAddBackslash WINELIB_NAME_AW(PathAddBackslash)

BOOL WINAPI PathAddExtensionA(LPSTR,LPCSTR);
BOOL WINAPI PathAddExtensionW(LPWSTR,LPCWSTR);
#define PathAddExtension WINELIB_NAME_AW(PathAddExtension)

BOOL WINAPI PathAppendA(LPSTR,LPCSTR);
BOOL WINAPI PathAppendW(LPWSTR,LPCWSTR);
#define PathAppend WINELIB_NAME_AW(PathAppend)

LPSTR  WINAPI PathBuildRootA(LPSTR,int);
LPWSTR WINAPI PathBuildRootW(LPWSTR,int);
#define PathBuildRoot WINELIB_NAME_AW(PathBuiltRoot)

BOOL WINAPI PathCanonicalizeA(LPSTR,LPCSTR);
BOOL WINAPI PathCanonicalizeW(LPWSTR,LPCWSTR);
#define PathCanonicalize WINELIB_NAME_AW(PathCanonicalize)

LPSTR  WINAPI PathCombineA(LPSTR,LPCSTR,LPCSTR);
LPWSTR WINAPI PathCombineW(LPWSTR,LPCWSTR,LPCWSTR);
#define PathCombine WINELIB_NAME_AW(PathCombine)

BOOL WINAPI PathCompactPathA(HDC,LPSTR,UINT);
BOOL WINAPI PathCompactPathW(HDC,LPWSTR,UINT);
#define PathCompactPath WINELIB_NAME_AW(PathCompactPath)

BOOL WINAPI PathCompactPathExA(LPSTR,LPCSTR,UINT,DWORD);
BOOL WINAPI PathCompactPathExW(LPWSTR,LPCWSTR,UINT,DWORD);
#define PathCompactPathEx WINELIB_NAME_AW(PathCompactPathEx)

int WINAPI PathCommonPrefixA(LPCSTR,LPCSTR,LPSTR);
int WINAPI PathCommonPrefixW(LPCWSTR,LPCWSTR,LPWSTR);
#define PathCommonPrefix WINELIB_NAME_AW(PathCommonPrefix)

BOOL WINAPI PathFileExistsA(LPCSTR);
BOOL WINAPI PathFileExistsW(LPCWSTR);
#define PathFileExists WINELIB_NAME_AW(PathFileExists)

LPSTR  WINAPI PathFindExtensionA(LPCSTR);
LPWSTR WINAPI PathFindExtensionW(LPCWSTR);
#define PathFindExtension WINELIB_NAME_AW(PathFindExtension)

LPSTR  WINAPI PathFindFileNameA(LPCSTR);
LPWSTR WINAPI PathFindFileNameW(LPCWSTR);
#define PathFindFileName WINELIB_NAME_AW(PathFindFileName)

LPSTR  WINAPI PathFindNextComponentA(LPCSTR);
LPWSTR WINAPI PathFindNextComponentW(LPCWSTR);
#define PathFindNextComponent WINELIB_NAME_AW(PathFindNextComponent)

BOOL WINAPI PathFindOnPathA(LPSTR,LPCSTR*);
BOOL WINAPI PathFindOnPathW(LPWSTR,LPCWSTR*);
#define PathFindOnPath WINELIB_NAME_AW(PathFindOnPath)

LPSTR  WINAPI PathGetArgsA(LPCSTR);
LPWSTR WINAPI PathGetArgsW(LPCWSTR);
#define PathGetArgs WINELIB_NAME_AW(PathGetArgs)

UINT WINAPI PathGetCharTypeA(UCHAR);
UINT WINAPI PathGetCharTypeW(WCHAR);
#define PathGetCharType WINELIB_NAME_AW(PathGetCharType)

int WINAPI PathGetDriveNumberA(LPCSTR);
int WINAPI PathGetDriveNumberW(LPCWSTR);
#define PathGetDriveNumber WINELIB_NAME_AW(PathGetDriveNumber)

BOOL WINAPI PathIsDirectoryA(LPCSTR);
BOOL WINAPI PathIsDirectoryW(LPCWSTR);
#define PathIsDirectory WINELIB_NAME_AW(PathIsDirectory)

BOOL WINAPI PathIsDirectoryEmptyA(LPCSTR);
BOOL WINAPI PathIsDirectoryEmptyW(LPCWSTR);
#define PathIsDirectoryEmpty WINELIB_NAME_AW(PathIsDirectoryEmpty)

BOOL WINAPI PathIsFileSpecA(LPCSTR);
BOOL WINAPI PathIsFileSpecW(LPCWSTR);
#define PathIsFileSpec WINELIB_NAME_AW(PathIsFileSpec);

BOOL WINAPI PathIsPrefixA(LPCSTR,LPCSTR);
BOOL WINAPI PathIsPrefixW(LPCWSTR,LPCWSTR);
#define PathIsPrefix WINELIB_NAME_AW(PathIsPrefix)

BOOL WINAPI PathIsRelativeA(LPCSTR);
BOOL WINAPI PathIsRelativeW(LPCWSTR);
#define PathIsRelative WINELIB_NAME_AW(PathIsRelative)

BOOL WINAPI PathIsRootA(LPCSTR);
BOOL WINAPI PathIsRootW(LPCWSTR);
#define PathIsRoot WINELIB_NAME_AW(PathIsRoot)

BOOL WINAPI PathIsSameRootA(LPCSTR,LPCSTR);
BOOL WINAPI PathIsSameRootW(LPCWSTR,LPCWSTR);
#define PathIsSameRoot WINELIB_NAME_AW(PathIsSameRoot)

BOOL WINAPI PathIsUNCA(LPCSTR);
BOOL WINAPI PathIsUNCW(LPCWSTR);
#define PathIsUNC WINELIB_NAME_AW(PathIsUNC)

BOOL WINAPI PathIsUNCServerA(LPCSTR);
BOOL WINAPI PathIsUNCServerW(LPCWSTR);
#define PathIsUNCServer WINELIB_NAME_AW(PathIsUNCServer)

BOOL WINAPI PathIsUNCServerShareA(LPCSTR);
BOOL WINAPI PathIsUNCServerShareW(LPCWSTR);
#define PathIsUNCServerShare WINELIB_NAME_AW(PathIsUNCServerShare)

BOOL WINAPI PathIsContentTypeA(LPCSTR,LPCSTR);
BOOL WINAPI PathIsContentTypeW(LPCWSTR,LPCWSTR);
#define PathIsContentType WINELIB_NAME_AW(PathIsContentType)

BOOL WINAPI PathIsURLA(LPCSTR);
BOOL WINAPI PathIsURLW(LPCWSTR);
#define PathIsURL WINELIB_NAME_AW(PathIsURL)

BOOL WINAPI PathMakePrettyA(LPSTR);
BOOL WINAPI PathMakePrettyW(LPWSTR);
#define PathMakePretty WINELIB_NAME_AW(PathMakePretty)

BOOL WINAPI PathMatchSpecA(LPCSTR,LPCSTR);
BOOL WINAPI PathMatchSpecW(LPCWSTR,LPCWSTR);
#define PathMatchSpec WINELIB_NAME_AW(PathMatchSpec)

int WINAPI PathParseIconLocationA(LPSTR);
int WINAPI PathParseIconLocationW(LPWSTR);
#define PathParseIconLocation WINELIB_NAME_AW(PathParseIconLocation)

VOID WINAPI PathQuoteSpacesA(LPSTR);
VOID WINAPI PathQuoteSpacesW(LPWSTR);
#define PathQuoteSpaces WINELIB_NAME_AW(PathQuoteSpaces)

BOOL WINAPI PathRelativePathToA(LPSTR,LPCSTR,DWORD,LPCSTR,DWORD);
BOOL WINAPI PathRelativePathToW(LPWSTR,LPCWSTR,DWORD,LPCWSTR,DWORD);
#define PathRelativePathTo WINELIB_NAME_AW(PathRelativePathTo)

VOID WINAPI PathRemoveArgsA(LPSTR);
VOID WINAPI PathRemoveArgsW(LPWSTR);
#define PathRemoveArgs WINELIB_NAME_AW(PathRemoveArgs)

LPSTR  WINAPI PathRemoveBackslashA(LPSTR);
LPWSTR WINAPI PathRemoveBackslashW(LPWSTR);
#define PathRemoveBackslash WINELIB_NAME_AW(PathRemoveBackslash)

VOID WINAPI PathRemoveBlanksA(LPSTR);
VOID WINAPI PathRemoveBlanksW(LPWSTR);
#define PathRemoveBlanks WINELIB_NAME_AW(PathRemoveBlanks)

VOID WINAPI PathRemoveExtensionA(LPSTR);
VOID WINAPI PathRemoveExtensionW(LPWSTR);
#define PathRemoveExtension WINELIB_NAME_AW(PathRemoveExtension)

BOOL WINAPI PathRemoveFileSpecA(LPSTR);
BOOL WINAPI PathRemoveFileSpecW(LPWSTR);
#define PathRemoveFileSpec WINELIB_NAME_AW(PathRemoveFileSpec)

BOOL WINAPI PathRenameExtensionA(LPSTR,LPCSTR);
BOOL WINAPI PathRenameExtensionW(LPWSTR,LPCWSTR);
#define PathRenameExtension WINELIB_NAME_AW(PathRenameExtension)

BOOL WINAPI PathSearchAndQualifyA(LPCSTR,LPSTR,UINT);
BOOL WINAPI PathSearchAndQualifyW(LPCWSTR,LPWSTR,UINT);
#define PathSearchAndQualify WINELIB_NAME_AW(PathSearchAndQualify)

VOID WINAPI PathSetDlgItemPathA(HWND,int,LPCSTR);
VOID WINAPI PathSetDlgItemPathW(HWND,int,LPCWSTR);
#define PathSetDlgItemPath WINELIB_NAME_AW(PathSetDlgItemPath)

LPSTR  WINAPI PathSkipRootA(LPCSTR);
LPWSTR WINAPI PathSkipRootW(LPCWSTR);
#define PathSkipRoot WINELIB_NAME_AW(PathSkipRoot)

VOID WINAPI PathStripPathA(LPSTR);
VOID WINAPI PathStripPathW(LPWSTR);
#define PathStripPath WINELIB_NAME_AW(PathStripPath)

BOOL WINAPI PathStripToRootA(LPSTR);
BOOL WINAPI PathStripToRootW(LPWSTR);
#define PathStripToRoot WINELIB_NAME_AW(PathStripToRoot)

VOID WINAPI PathUnquoteSpacesA(LPSTR);
VOID WINAPI PathUnquoteSpacesW(LPWSTR);
#define PathUnquoteSpaces WINELIB_NAME_AW(PathUnquoteSpaces)

BOOL WINAPI PathMakeSystemFolderA(LPCSTR);
BOOL WINAPI PathMakeSystemFolderW(LPCWSTR);
#define PathMakeSystemFolder WINELIB_NAME_AW(PathMakeSystemFolder)

BOOL WINAPI PathUnmakeSystemFolderA(LPCSTR);
BOOL WINAPI PathUnmakeSystemFolderW(LPCWSTR);
#define PathUnmakeSystemFolder WINELIB_NAME_AW(PathUnmakeSystemFolder)

BOOL WINAPI PathIsSystemFolderA(LPCSTR,DWORD);
BOOL WINAPI PathIsSystemFolderW(LPCWSTR,DWORD);
#define PathIsSystemFolder WINELIB_NAME_AW(PathIsSystemFolder)

BOOL WINAPI PathIsNetworkPathA(LPCSTR);
BOOL WINAPI PathIsNetworkPathW(LPCWSTR);
#define PathIsNetworkPath WINELIB_NAME_AW(PathIsNetworkPath)

BOOL WINAPI PathIsLFNFileSpecA(LPCSTR);
BOOL WINAPI PathIsLFNFileSpecW(LPCWSTR);
#define PathIsLFNFileSpec WINELIB_NAME_AW(PathIsLFNFileSpec)

int WINAPI PathFindSuffixArrayA(LPCSTR,LPCSTR *,int);
int WINAPI PathFindSuffixArrayW(LPCWSTR,LPCWSTR *,int);
#define PathFindSuffixArray WINELIB_NAME_AW(PathFindSuffixArray)

VOID WINAPI PathUndecorateA(LPSTR);
VOID WINAPI PathUndecorateW(LPWSTR);
#define PathUndecorate WINELIB_NAME_AW(PathUndecorate)

BOOL WINAPI PathUnExpandEnvStringsA(LPCSTR,LPSTR,UINT);
BOOL WINAPI PathUnExpandEnvStringsW(LPCWSTR,LPWSTR,UINT);
#define PathUnExpandEnvStrings WINELIB_NAME_AW(PathUnExpandEnvStrings)

/* Url functions */

/* These are used by UrlGetPart routine */
typedef enum {
    URL_PART_NONE    = 0,
    URL_PART_SCHEME  = 1,
    URL_PART_HOSTNAME,
    URL_PART_USERNAME,
    URL_PART_PASSWORD,
    URL_PART_PORT,
    URL_PART_QUERY
} URL_PART;

#define URL_PARTFLAG_KEEPSCHEME  0x00000001

/* These are used by the UrlIs... routines */
typedef enum {
    URLIS_URL,
    URLIS_OPAQUE,
    URLIS_NOHISTORY,
    URLIS_FILEURL,
    URLIS_APPLIABLE,
    URLIS_DIRECTORY,
    URLIS_HASQUERY
} URLIS;

/* This is used by the UrlApplyScheme... routines */
#define URL_APPLY_FORCEAPPLY         0x00000008
#define URL_APPLY_GUESSFILE          0x00000004
#define URL_APPLY_GUESSSCHEME        0x00000002
#define URL_APPLY_DEFAULT            0x00000001

/* The following are used by UrlEscape..., UrlUnEscape...,
 * UrlCanonicalize..., and UrlCombine... routines
 */
#define URL_WININET_COMPATIBILITY    0x80000000
#define URL_PLUGGABLE_PROTOCOL       0x40000000
#define URL_ESCAPE_UNSAFE            0x20000000
#define URL_UNESCAPE                 0x10000000

#define URL_DONT_SIMPLIFY            0x08000000
#define URL_NO_META                  URL_DONT_SIMPLIFY
#define URL_ESCAPE_SPACES_ONLY       0x04000000
#define URL_DONT_ESCAPE_EXTRA_INFO   0x02000000
#define URL_DONT_UNESCAPE_EXTRA_INFO URL_DONT_ESCAPE_EXTRA_INFO
#define URL_BROWSER_MODE             URL_DONT_ESCAPE_EXTRA_INFO

#define URL_INTERNAL_PATH            0x00800000  /* Will escape #'s in paths */
#define URL_UNESCAPE_HIGH_ANSI_ONLY  0x00400000
#define URL_CONVERT_IF_DOSPATH       0x00200000
#define URL_UNESCAPE_INPLACE         0x00100000

#define URL_FILE_USE_PATHURL         0x00010000

#define URL_ESCAPE_SEGMENT_ONLY      0x00002000
#define URL_ESCAPE_PERCENT           0x00001000

HRESULT WINAPI UrlApplySchemeA(LPCSTR,LPSTR,LPDWORD,DWORD);
HRESULT WINAPI UrlApplySchemeW(LPCWSTR,LPWSTR,LPDWORD,DWORD);
#define UrlApplyScheme WINELIB_NAME_AW(UrlApplyScheme)

HRESULT WINAPI UrlCanonicalizeA(LPCSTR,LPSTR,LPDWORD,DWORD);
HRESULT WINAPI UrlCanonicalizeW(LPCWSTR,LPWSTR,LPDWORD,DWORD);
#define UrlCanonicalize WINELIB_NAME_AW(UrlCanoncalize)

HRESULT WINAPI UrlCombineA(LPCSTR,LPCSTR,LPSTR,LPDWORD,DWORD);
HRESULT WINAPI UrlCombineW(LPCWSTR,LPCWSTR,LPWSTR,LPDWORD,DWORD);
#define UrlCombine WINELIB_NAME_AW(UrlCombine)

INT WINAPI UrlCompareA(LPCSTR,LPCSTR,BOOL);
INT WINAPI UrlCompareW(LPCWSTR,LPCWSTR,BOOL);
#define UrlCompare WINELIB_NAME_AW(UrlCompare)

HRESULT WINAPI UrlEscapeA(LPCSTR,LPSTR,LPDWORD,DWORD);
HRESULT WINAPI UrlEscapeW(LPCWSTR,LPWSTR,LPDWORD,DWORD);
#define UrlEscape WINELIB_NAME_AW(UrlEscape)

#define UrlEscapeSpacesA(x,y,z) UrlCanonicalizeA(x, y, z, \
                         URL_DONT_ESCAPE_EXTRA_INFO|URL_ESCAPE_SPACES_ONLY)
#define UrlEscapeSpacesW(x,y,z) UrlCanonicalizeW(x, y, z, \
                         URL_DONT_ESCAPE_EXTRA_INFO|URL_ESCAPE_SPACES_ONLY)
#define UrlEscapeSpaces WINELIB_NAME_AW(UrlEscapeSpaces)

LPCSTR  WINAPI UrlGetLocationA(LPCSTR);
LPCWSTR WINAPI UrlGetLocationW(LPCWSTR);
#define UrlGetLocation WINELIB_NAME_AW(UrlGetLocation)

HRESULT WINAPI UrlGetPartA(LPCSTR,LPSTR,LPDWORD,DWORD,DWORD);
HRESULT WINAPI UrlGetPartW(LPCWSTR,LPWSTR,LPDWORD,DWORD,DWORD);
#define UrlGetPart WINELIB_NAME_AW(UrlGetPart)

BOOL    WINAPI HashData(const unsigned char *,INT,unsigned char *lpDest,INT);

HRESULT WINAPI UrlHashA(LPCSTR,unsigned char *,INT);
HRESULT WINAPI UrlHashW(LPCWSTR,unsigned char *,INT);
#define UrlHash WINELIB_NAME_AW(UrlHash)

BOOL    WINAPI UrlIsA(LPCSTR,URLIS);
BOOL    WINAPI UrlIsW(LPCWSTR,URLIS);
#define UrlIs WINELIB_NAME_AW(UrlIs)

BOOL    WINAPI UrlIsNoHistoryA(LPCSTR);
BOOL    WINAPI UrlIsNoHistoryW(LPCWSTR);
#define UrlIsNoHistory WINELIB_NAME_AW(UrlIsNoHistory)

BOOL    WINAPI UrlIsOpaqueA(LPCSTR);
BOOL    WINAPI UrlIsOpaqueW(LPCWSTR);
#define UrlIsOpaque WINELIB_NAME_AW(UrlIsOpaque)

#define UrlIsFileUrlA(x) UrlIsA(x, URLIS_FILEURL)
#define UrlIsFileUrlW(y) UrlIsW(x, URLIS_FILEURL)
#define UrlIsFileUrl WINELIB_NAME_AW(UrlIsFileUrl)

HRESULT WINAPI UrlUnescapeA(LPCSTR,LPSTR,LPDWORD,DWORD);
HRESULT WINAPI UrlUnescapeW(LPCWSTR,LPWSTR,LPDWORD,DWORD);
#define UrlUnescape WINELIB_AW_NAME(UrlUnescape)

#define UrlUnescapeInPlaceA(x,y) UrlUnescapeA(x, NULL, NULL, \
                                              y | URL_UNESCAPE_INPLACE)
#define UrlUnescapeInPlaceW(x,y) UrlUnescapeW(x, NULL, NULL, \
                                              y | URL_UNESCAPE_INPLACE)
#define UrlUnescapeInPlace WINELIB_AW_NAME(UrlUnescapeInPlace)

HRESULT WINAPI UrlCreateFromPathA(LPCSTR,LPSTR,LPDWORD,DWORD);
HRESULT WINAPI UrlCreateFromPathW(LPCWSTR,LPWSTR,LPDWORD,DWORD);
#define UrlCreateFromPath WINELIB_AW_NAME(UrlCreateFromPath)

#endif /* NO_SHLWAPI_PATH */


/* String functions */
#ifndef NO_SHLWAPI_STRFCNS

/* StrToIntEx flags */
#define STIF_DEFAULT     0x0L
#define STIF_SUPPORT_HEX 0x1L

BOOL WINAPI ChrCmpIA (WORD w1, WORD w2);
BOOL WINAPI ChrCmpIW (WCHAR w1, WCHAR w2);
#define ChrCmpI WINELIB_NAME_AW(ChrCmpI)

INT WINAPI StrCSpnA(LPCSTR,LPCSTR);
INT WINAPI StrCSpnW(LPCWSTR,LPCWSTR);
#define StrCSpn WINELIB_NAME_AW(StrCSpn)

INT WINAPI StrCSpnIA(LPCSTR,LPCSTR);
INT WINAPI StrCSpnIW(LPCWSTR,LPCWSTR);
#define StrCSpnI WINELIB_NAME_AW(StrCSpnI)

#define StrCatA lstrcatA
LPWSTR WINAPI StrCatW(LPWSTR,LPCWSTR);
#define StrCat WINELIB_NAME_AW(StrCat)

LPSTR WINAPI StrCatBuffA(LPSTR,LPCSTR,INT);
LPWSTR WINAPI StrCatBuffW(LPWSTR,LPCWSTR,INT);
#define StrCatBuff WINELIB_NAME_AW(StrCatBuff)

LPSTR WINAPI StrChrA(LPCSTR,WORD);
LPWSTR WINAPI StrChrW(LPCWSTR,WCHAR);
#define StrChr WINELIB_NAME_AW(StrChr)

LPSTR WINAPI StrChrIA(LPCSTR,CHAR);
LPWSTR WINAPI StrChrIW(LPCWSTR,WCHAR);
#define StrChrI WINELIB_NAME_AW(StrChrI)

#define StrCmpA lstrcmpA
int WINAPI StrCmpW(LPCWSTR,LPCWSTR);
#define StrCmp WINELIB_NAME_AW(StrCmp)

#define StrCmpIA lstrcmpiA
int WINAPI StrCmpIW(LPCWSTR,LPCWSTR);
#define StrCmpI WINELIB_NAME_AW(StrCmpI)

#define StrCpyA lstrcpyA
LPWSTR WINAPI StrCpyW(LPWSTR,LPCWSTR);
#define StrCpy WINELIB_NAME_AW(StrCpy)

//#define StrCpyNA lstrcpynA
LPWSTR WINAPI StrCpyNW(LPWSTR,LPCWSTR,int);
#define StrCpyN WINELIB_NAME_AW(StrCpyN)
#define StrNCpy WINELIB_NAME_AW(StrCpyN)

INT WINAPI StrCmpNA(LPCSTR,LPCSTR,INT);
INT WINAPI StrCmpNW(LPCWSTR,LPCWSTR,INT);
#define StrCmpN WINELIB_NAME_AW(StrCmpN)
#define StrNCmp WINELIB_NAME_AW(StrCmpN)

INT WINAPI StrCmpNIA(LPCSTR,LPCSTR,INT);
INT WINAPI StrCmpNIW(LPCWSTR,LPCWSTR,INT);
#define StrCmpNI WINELIB_NAME_AW(StrCmpNI)
#define StrNCmpI WINELIB_NAME_AW(StrCmpNI)

LPSTR WINAPI StrDupA(LPCSTR);
LPWSTR WINAPI StrDupW(LPCWSTR);
#define StrDup WINELIB_NAME_AW(StrDup)

LPSTR WINAPI StrFormatByteSizeA (DWORD,LPSTR,UINT);
LPWSTR WINAPI StrFormatByteSizeW (DWORD,LPWSTR,UINT);
#define StrFormatByteSize WINELIB_NAME_AW(StrFormatByteSize)

int WINAPI StrFromTimeIntervalA(LPSTR,UINT,DWORD,int);
int WINAPI StrFromTimeIntervalW(LPWSTR,UINT,DWORD,int);
#define StrFromTimeInterval WINELIB_NAME_AW(StrFromTimeInterval)

BOOL WINAPI StrIsIntlEqualA(BOOL,LPCSTR,LPCSTR,int);
BOOL WINAPI StrIsIntlEqualW(BOOL,LPCWSTR,LPCWSTR,int);
#define StrIsIntlEqual WINELIB_NAME_AW(StrIsIntlEqual)

#define StrIntlEqNA(a,b,c) StrIsIntlEqualA(TRUE,a,b,c)
#define StrIntlEqNW(a,b,c) StrIsIntlEqualW(TRUE,a,b,c)

#define StrIntlEqNIA(a,b,c) StrIsIntlEqualA(FALSE,a,b,c)
#define StrIntlEqNIW(a,b,c) StrIsIntlEqualW(FALSE,a,b,c)

LPSTR  WINAPI StrNCatA(LPSTR,LPCSTR,int);
LPWSTR WINAPI StrNCatW(LPWSTR,LPCWSTR,int);
#define StrNCat WINELIB_NAME_AW(StrNCat)
#define StrCatN WINELIB_NAME_AW(StrNCat)

LPSTR  WINAPI StrPBrkA(LPCSTR,LPCSTR);
LPWSTR WINAPI StrPBrkW(LPCWSTR,LPCWSTR);
#define StrPBrk WINELIB_NAME_AW(StrPBrk)

LPSTR  WINAPI StrRChrA(LPCSTR,LPCSTR,WORD);
LPWSTR WINAPI StrRChrW(LPCWSTR,LPCWSTR,WORD);
#define StrRChr WINELIB_NAME_AW(StrRChr)

LPSTR  WINAPI StrRChrIA(LPCSTR,LPCSTR,WORD);
LPWSTR WINAPI StrRChrIW(LPCWSTR,LPCWSTR,WORD);
#define StrRChrI WINELIB_NAME_AW(StrRChrI)

LPSTR  WINAPI StrRStrIA(LPCSTR,LPCSTR,LPCSTR);
LPWSTR WINAPI StrRStrIW(LPCWSTR,LPCWSTR,LPCWSTR);
#define StrRStrI WINELIB_NAME_AW(StrRStrI)

int WINAPI StrSpnA(LPCSTR,LPCSTR);
int WINAPI StrSpnW(LPCWSTR,LPCWSTR);
#define StrSpn WINELIB_NAME_AW(StrSpn)

LPSTR  WINAPI StrStrA(LPCSTR,LPCSTR);
LPWSTR WINAPI StrStrW(LPCWSTR,LPCWSTR);
#define StrStr WINELIB_NAME_AW(StrStr)

LPSTR  WINAPI StrStrIA(LPCSTR,LPCSTR);
LPWSTR WINAPI StrStrIW(LPCWSTR,LPCWSTR);
#define StrStrI WINELIB_NAME_AW(StrStrI)

int WINAPI StrToIntA(LPCSTR);
int WINAPI StrToIntW(LPCWSTR);
#define StrToInt WINELIB_NAME_AW(StrToInt)
#define StrToLong WINELIB_NAME_AW(StrToInt)

BOOL WINAPI StrToIntExA(LPCSTR,DWORD,int*);
BOOL WINAPI StrToIntExW(LPCWSTR,DWORD,int*);
#define StrToIntEx WINELIB_NAME_AW(StrToIntEx)

BOOL WINAPI StrTrimA(LPSTR,LPCSTR);
BOOL WINAPI StrTrimW(LPWSTR,LPCWSTR);
#define StrTrim WINELIB_NAME_AW(StrTrim)

INT WINAPI wvnsprintfA(LPSTR lpOut, INT cchLimitIn, LPCSTR lpFmt, va_list arglist);
INT WINAPI wvnsprintfW(LPWSTR lpOut, INT cchLimitIn, LPCWSTR lpFmt, va_list arglist);
#define wvnsprintf WINELIB_NAME_AW(wvnsprintf)

INT WINAPIV wnsprintfA(LPSTR lpOut, INT cchLimitIn, LPCSTR lpFmt, ...);
INT WINAPIV wnsprintfW(LPWSTR lpOut, INT cchLimitIn, LPCWSTR lpFmt, ...);
#define wnsprintf WINELIB_NAME_AW(wnsprintf)

/* Undocumented */
struct _STRRET;
struct _ITEMIDLIST;
HRESULT WINAPI StrRetToBufA(struct _STRRET *src, const struct _ITEMIDLIST *pidl, LPSTR dest, DWORD len);
HRESULT WINAPI StrRetToBufW(struct _STRRET *src, const struct _ITEMIDLIST *pidl, LPWSTR dest, DWORD len);
#define StrRetToBuf WINELIB_NAME_AW(StrRetToBuf)

#endif /* NO_SHLWAPI_STRFCNS */


/* GDI functions */
#ifndef NO_SHLWAPI_GDI

HPALETTE WINAPI SHCreateShellPalette(HDC);

COLORREF WINAPI ColorHLSToRGB(WORD,WORD,WORD);

COLORREF WINAPI ColorAdjustLuma(COLORREF,int,BOOL);

VOID WINAPI ColorRGBToHLS(COLORREF,LPWORD,LPWORD,LPWORD);

#endif /* NO_SHLWAPI_GDI */


/* Stream functions */
#if !defined(NO_SHLWAPI_STREAM) && defined(IStream_IMETHODS)

IStream * WINAPI SHOpenRegStreamA(HKEY,LPCSTR,LPCSTR,DWORD);
IStream * WINAPI SHOpenRegStreamW(HKEY,LPCWSTR,LPCWSTR,DWORD);
#define SHOpenRegStream WINELIB_NAME_AW(SHOpenRegStream)

IStream * WINAPI SHOpenRegStream2A(HKEY,LPCSTR,LPCSTR,DWORD);
IStream * WINAPI SHOpenRegStream2W(HKEY,LPCWSTR,LPCWSTR,DWORD);
#define SHOpenRegStream2 WINELIB_NAME_AW(SHOpenRegStream2)

#endif /* NO_SHLWAPI_STREAM */


/* Version Information */

typedef struct _DllVersionInfo {
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
} DLLVERSIONINFO;

#define DLLVER_PLATFORM_WINDOWS 0x01 /* Win9x */
#define DLLVER_PLATFORM_NT      0x02 /* WinNT */

typedef HRESULT (CALLBACK *DLLGETVERSIONPROC)(DLLVERSIONINFO *);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_SHLWAPI_H */
