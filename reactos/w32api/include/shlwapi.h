#ifndef _SHLWAPI_H
#define _SHLWAPI_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __OBJC__
#include <objbase.h>
#include <shlobj.h>
#endif

#ifndef WINSHLWAPI
#define WINSHLWAPI DECLSPEC_IMPORT
#endif

#define DLLVER_PLATFORM_WINDOWS	0x00000001
#define DLLVER_PLATFORM_NT	0x00000002

#define URL_DONT_ESCAPE_EXTRA_INFO 0x02000000
#define URL_DONT_SIMPLIFY	0x08000000
#define URL_ESCAPE_PERCENT	0x00001000
#define URL_ESCAPE_SEGMENT_ONLY	0x00002000
#define URL_ESCAPE_SPACES_ONLY	0x04000000
#define URL_ESCAPE_UNSAFE	0x20000000
#define URL_INTERNAL_PATH	0x00800000
#define URL_PARTFLAG_KEEPSCHEME	0x00000001
#define URL_PLUGGABLE_PROTOCOL	0x40000000
#define URL_UNESCAPE		0x10000000
#define URL_UNESCAPE_HIGH_ANSI_ONLY 0x00400000
#define URL_UNESCAPE_INPLACE	0x00100000

#ifndef RC_INVOKED
#include <pshpack1.h>
typedef struct _DllVersionInfo
{
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
} DLLVERSIONINFO;
typedef struct _DLLVERSIONINFO2
{
    DLLVERSIONINFO info1;
    DWORD dwFlags;
    ULONGLONG ullVersion;
} DLLVERSIONINFO2;
#include <poppack.h>

#define MAKEDLLVERULL(major, minor, build, qfe) \
        (((ULONGLONG)(major) << 48) | \
         ((ULONGLONG)(minor) << 32) | \
         ((ULONGLONG)(build) << 16) | \
         ((ULONGLONG)(  qfe) <<  0))

typedef enum {
    ASSOCSTR_COMMAND,
    ASSOCSTR_EXECUTABLE,
    ASSOCSTR_FRIENDLYDOCNAME,
    ASSOCSTR_FRIENDLYAPPNAME,
    ASSOCSTR_NOOPEN,
    ASSOCSTR_SHELLNEWVALUE,
    ASSOCSTR_DDECOMMAND,
    ASSOCSTR_DDEIFEXEC,
    ASSOCSTR_DDEAPPLICATION,
    ASSOCSTR_DDETOPIC
} ASSOCSTR;
typedef enum
{
    ASSOCKEY_SHELLEXECCLASS = 1,
    ASSOCKEY_APP,
    ASSOCKEY_CLASS,
    ASSOCKEY_BASECLASS
} ASSOCKEY;
typedef enum
{
    ASSOCDATA_MSIDESCRIPTOR = 1,
    ASSOCDATA_NOACTIVATEHANDLER,
    ASSOCDATA_QUERYCLASSSTORE
} ASSOCDATA;
typedef DWORD ASSOCF;
typedef enum
{
    SHREGDEL_DEFAULT = 0x00000000,
    SHREGDEL_HKCU    = 0x00000001,
    SHREGDEL_HKLM    = 0x00000010,
    SHREGDEL_BOTH    = 0x00000011
} SHREGDEL_FLAGS;
typedef enum
{
    SHREGENUM_DEFAULT = 0x00000000,
    SHREGENUM_HKCU    = 0x00000001,
    SHREGENUM_HKLM    = 0x00000010,
    SHREGENUM_BOTH    = 0x00000011
} SHREGENUM_FLAGS;
typedef enum
{
    URLIS_URL,
    URLIS_OPAQUE,
    URLIS_NOHISTORY,
    URLIS_FILEURL,
    URLIS_APPLIABLE,
    URLIS_DIRECTORY,
    URLIS_HASQUERY
} URLIS;

typedef HANDLE HUSKEY, *PHUSKEY;

typedef HRESULT (WINAPI* DLLGETVERSIONPROC)(DLLVERSIONINFO *);

WINSHLWAPI BOOL WINAPI ChrCmpIA(WORD,WORD);
WINSHLWAPI BOOL WINAPI ChrCmpIW(WCHAR,WCHAR);
#define IntlStrEqNA(pStr1, pStr2, nChar) IntlStrEqWorkerA(TRUE, pStr1, pStr2, nChar);
#define IntlStrEqNW(pStr1, pStr2, nChar) IntlStrEqWorkerW(TRUE, pStr1, pStr2, nChar);
#define IntlStrEqNIA(pStr1, pStr2, nChar) IntlStrEqWorkerA(FALSE, pStr1, pStr2, nChar);
#define IntlStrEqNIW(pStr1, pStr2, nChar) IntlStrEqWorkerW(FALSE, pStr1, pStr2, nChar);
WINSHLWAPI BOOL WINAPI IntlStrEqWorkerA(BOOL,LPCSTR,LPCSTR,int);
WINSHLWAPI BOOL WINAPI IntlStrEqWorkerW(BOOL,LPCWSTR,LPCWSTR,int);
WINSHLWAPI HRESULT WINAPI SHStrDupA(LPCSTR,LPWSTR*);
WINSHLWAPI HRESULT WINAPI SHStrDupW(LPCWSTR,LPWSTR*);
WINSHLWAPI LPSTR WINAPI StrCatA(LPSTR,LPCSTR);
WINSHLWAPI LPWSTR WINAPI StrCatW(LPWSTR,LPCWSTR);
WINSHLWAPI LPSTR WINAPI StrCatBuffA(LPSTR,LPCSTR,int);
WINSHLWAPI LPWSTR WINAPI StrCatBuffW(LPWSTR,LPCWSTR,int);
WINSHLWAPI DWORD WINAPI StrCatChainW(LPWSTR,DWORD,DWORD,LPCWSTR);
WINSHLWAPI LPSTR WINAPI StrChrA(LPCSTR,WORD);
WINSHLWAPI LPWSTR WINAPI StrChrW(LPCWSTR,WCHAR);
WINSHLWAPI LPSTR WINAPI StrChrIA(LPCSTR,WORD);
WINSHLWAPI LPWSTR WINAPI StrChrIW(LPCWSTR,WCHAR);
#define StrCmpIA lstrcmpiA
#define StrCmpA lstrcmpA
#define StrCpyA lstrcpyA
#define StrCpyNA lstrcpynA
WINSHLWAPI int WINAPI StrCmpIW(LPCWSTR,LPCWSTR);
WINSHLWAPI int WINAPI StrCmpW(LPCWSTR,LPCWSTR);
WINSHLWAPI LPWSTR WINAPI StrCpyW(LPWSTR,LPCWSTR);
WINSHLWAPI LPWSTR WINAPI StrCpyNW(LPWSTR,LPCWSTR,int);
WINSHLWAPI int WINAPI StrCmpNA(LPCSTR,LPCSTR,int);
WINSHLWAPI int WINAPI StrCmpNW(LPCWSTR,LPCWSTR,int);
WINSHLWAPI int WINAPI StrCmpNIA(LPCSTR,LPCSTR,int);
WINSHLWAPI int WINAPI StrCmpNIW(LPCWSTR,LPCWSTR,int);
WINSHLWAPI int WINAPI StrCSpnA(LPCSTR,LPCSTR);
WINSHLWAPI int WINAPI StrCSpnW(LPCWSTR,LPCWSTR);
WINSHLWAPI int WINAPI StrCSpnIA(LPCSTR,LPCSTR);
WINSHLWAPI int WINAPI StrCSpnIW(LPCWSTR,LPCWSTR);
WINSHLWAPI LPSTR WINAPI StrDupA(LPCSTR);
WINSHLWAPI LPWSTR WINAPI StrDupW(LPCWSTR);
WINSHLWAPI LPSTR WINAPI StrFormatByteSize64A(LONGLONG,LPSTR,UINT);
WINSHLWAPI LPSTR WINAPI StrFormatByteSizeA(DWORD,LPSTR,UINT);
WINSHLWAPI LPWSTR WINAPI StrFormatByteSizeW(LONGLONG,LPWSTR,UINT);
WINSHLWAPI LPSTR WINAPI StrFormatKBSizeA(LONGLONG,LPSTR,UINT);
WINSHLWAPI LPWSTR WINAPI StrFormatKBSizeW(LONGLONG,LPWSTR,UINT);
WINSHLWAPI int WINAPI StrFromTimeIntervalA(LPSTR,UINT,DWORD,int);
WINSHLWAPI int WINAPI StrFromTimeIntervalW(LPWSTR,UINT,DWORD,int);
WINSHLWAPI BOOL WINAPI StrIsIntlEqualA(BOOL,LPCSTR,LPCSTR,int);
WINSHLWAPI BOOL WINAPI StrIsIntlEqualW(BOOL,LPCWSTR,LPCWSTR,int);
WINSHLWAPI LPSTR WINAPI StrNCatA(LPSTR,LPCSTR,int);
WINSHLWAPI LPWSTR WINAPI StrNCatW(LPWSTR,LPCWSTR,int);
WINSHLWAPI LPSTR WINAPI StrPBrkA(LPCSTR,LPCSTR);
WINSHLWAPI LPWSTR WINAPI StrPBrkW(LPCWSTR,LPCWSTR);
WINSHLWAPI LPSTR WINAPI StrRChrA(LPCSTR,LPCSTR,WORD);
WINSHLWAPI LPWSTR WINAPI StrRChrW(LPCWSTR,LPCWSTR,WCHAR);
WINSHLWAPI LPSTR WINAPI StrRChrIA(LPCSTR,LPCSTR,WORD);
WINSHLWAPI LPWSTR WINAPI StrRChrIW(LPCWSTR,LPCWSTR,WCHAR);
#ifndef __OBJC__
WINSHLWAPI HRESULT WINAPI StrRetToBufA(LPSTRRET,LPCITEMIDLIST,LPSTR,UINT);
WINSHLWAPI HRESULT WINAPI StrRetToBufW(LPSTRRET,LPCITEMIDLIST,LPWSTR,UINT);
WINSHLWAPI HRESULT WINAPI StrRetToStrA(LPSTRRET,LPCITEMIDLIST,LPSTR*);
WINSHLWAPI HRESULT WINAPI StrRetToStrW(LPSTRRET,LPCITEMIDLIST,LPWSTR*);
#endif
WINSHLWAPI LPSTR WINAPI StrRStrIA(LPCSTR,LPCSTR,LPCSTR);
WINSHLWAPI LPWSTR WINAPI StrRStrIW(LPCWSTR,LPCWSTR,LPCWSTR);
WINSHLWAPI int WINAPI StrSpnA(LPCSTR,LPCSTR);
WINSHLWAPI int WINAPI StrSpnW(LPCWSTR,LPCWSTR);
WINSHLWAPI LPSTR WINAPI StrStrA(LPCSTR, LPCSTR);
WINSHLWAPI LPSTR WINAPI StrStrIA(LPCSTR,LPCSTR);
WINSHLWAPI LPWSTR WINAPI StrStrIW(LPCWSTR,LPCWSTR);
WINSHLWAPI LPWSTR WINAPI StrStrW(LPCWSTR,LPCWSTR);
WINSHLWAPI int WINAPI StrToIntA(LPCSTR);
WINSHLWAPI int WINAPI StrToIntW(LPCWSTR);
WINSHLWAPI BOOL WINAPI StrToIntExA(LPCSTR,DWORD,int*);
WINSHLWAPI BOOL WINAPI StrToIntExW(LPCWSTR,DWORD,int*);
WINSHLWAPI BOOL WINAPI StrTrimA(LPSTR,LPCSTR);
WINSHLWAPI BOOL WINAPI StrTrimW(LPWSTR,LPCWSTR);
WINSHLWAPI LPSTR WINAPI PathAddBackslashA(LPSTR);
WINSHLWAPI LPWSTR WINAPI PathAddBackslashW(LPWSTR);
WINSHLWAPI BOOL WINAPI PathAddExtensionA(LPSTR,LPCSTR);
WINSHLWAPI BOOL WINAPI PathAddExtensionW(LPWSTR,LPCWSTR);
WINSHLWAPI BOOL WINAPI PathAppendA(LPSTR,LPCSTR);
WINSHLWAPI BOOL WINAPI PathAppendW(LPWSTR,LPCWSTR);
WINSHLWAPI LPSTR WINAPI PathBuildRootA(LPSTR,int);
WINSHLWAPI LPWSTR WINAPI PathBuildRootW(LPWSTR,int);
WINSHLWAPI BOOL WINAPI PathCanonicalizeA(LPSTR,LPCSTR);
WINSHLWAPI BOOL WINAPI PathCanonicalizeW(LPWSTR,LPCWSTR);
WINSHLWAPI LPSTR WINAPI PathCombineA(LPSTR,LPCSTR,LPCSTR);
WINSHLWAPI LPWSTR WINAPI PathCombineW(LPWSTR,LPCWSTR,LPCWSTR);
WINSHLWAPI int WINAPI PathCommonPrefixA(LPCSTR,LPCSTR,LPSTR);
WINSHLWAPI int WINAPI PathCommonPrefixW(LPCWSTR,LPCWSTR,LPWSTR);
WINSHLWAPI BOOL WINAPI PathCompactPathA(HDC,LPSTR,UINT);
WINSHLWAPI BOOL WINAPI PathCompactPathW(HDC,LPWSTR,UINT);
WINSHLWAPI BOOL WINAPI PathCompactPathExA(LPSTR,LPCSTR,UINT,DWORD);
WINSHLWAPI BOOL WINAPI PathCompactPathExW(LPWSTR,LPCWSTR,UINT,DWORD);
WINSHLWAPI HRESULT WINAPI PathCreateFromUrlA(LPCSTR,LPSTR,LPDWORD,DWORD);
WINSHLWAPI HRESULT WINAPI PathCreateFromUrlW(LPCWSTR,LPWSTR,LPDWORD,DWORD);
WINSHLWAPI BOOL WINAPI PathFileExistsA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathFileExistsW(LPCWSTR);
WINSHLWAPI LPSTR WINAPI PathFindExtensionA(LPCSTR);
WINSHLWAPI LPWSTR WINAPI PathFindExtensionW(LPCWSTR);
WINSHLWAPI LPSTR WINAPI PathFindFileNameA(LPCSTR);
WINSHLWAPI LPWSTR WINAPI PathFindFileNameW(LPCWSTR);
WINSHLWAPI LPSTR WINAPI PathFindNextComponentA(LPCSTR);
WINSHLWAPI LPWSTR WINAPI PathFindNextComponentW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathFindOnPathA(LPSTR,LPCSTR*);
WINSHLWAPI BOOL WINAPI PathFindOnPathW(LPWSTR,LPCWSTR*);
WINSHLWAPI LPCSTR WINAPI PathFindSuffixArrayA(LPCSTR,LPCSTR*,int);
WINSHLWAPI LPCWSTR WINAPI PathFindSuffixArrayW(LPCWSTR,LPCWSTR*,int);
WINSHLWAPI LPSTR WINAPI PathGetArgsA(LPCSTR);
WINSHLWAPI LPWSTR WINAPI PathGetArgsW(LPCWSTR);
WINSHLWAPI UINT WINAPI PathGetCharTypeA(UCHAR);
WINSHLWAPI UINT WINAPI PathGetCharTypeW(WCHAR);
WINSHLWAPI int WINAPI PathGetDriveNumberA(LPCSTR);
WINSHLWAPI int WINAPI PathGetDriveNumberW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsContentTypeA(LPCSTR,LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsContentTypeW(LPCWSTR,LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsDirectoryA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsDirectoryEmptyA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsDirectoryEmptyW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsDirectoryW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsFileSpecA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsFileSpecW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsLFNFileSpecA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsLFNFileSpecW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsNetworkPathA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsNetworkPathW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsPrefixA(LPCSTR,LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsPrefixW(LPCWSTR,LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsRelativeA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsRelativeW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsRootA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsRootW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsSameRootA(LPCSTR,LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsSameRootW(LPCWSTR,LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsSystemFolderA(LPCSTR,DWORD);
WINSHLWAPI BOOL WINAPI PathIsSystemFolderW(LPCWSTR,DWORD);
WINSHLWAPI BOOL WINAPI PathIsUNCA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsUNCServerA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsUNCServerShareA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsUNCServerShareW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsUNCServerW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsUNCW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathIsURLA(LPCSTR);
WINSHLWAPI BOOL WINAPI PathIsURLW(LPCWSTR);
WINSHLWAPI BOOL WINAPI PathMakePrettyA(LPSTR);
WINSHLWAPI BOOL WINAPI PathMakePrettyW(LPWSTR);
WINSHLWAPI BOOL WINAPI PathMakeSystemFolderA(LPSTR);
WINSHLWAPI BOOL WINAPI PathMakeSystemFolderW(LPWSTR);
WINSHLWAPI BOOL WINAPI PathMatchSpecA(LPCSTR,LPCSTR);
WINSHLWAPI BOOL WINAPI PathMatchSpecW(LPCWSTR,LPCWSTR);
WINSHLWAPI int WINAPI PathParseIconLocationA(LPSTR);
WINSHLWAPI int WINAPI PathParseIconLocationW(LPWSTR);
WINSHLWAPI void WINAPI PathQuoteSpacesA(LPSTR);
WINSHLWAPI void WINAPI PathQuoteSpacesW(LPWSTR);
WINSHLWAPI BOOL WINAPI PathRelativePathToA(LPSTR,LPCSTR,DWORD,LPCSTR,DWORD);
WINSHLWAPI BOOL WINAPI PathRelativePathToW(LPWSTR,LPCWSTR,DWORD,LPCWSTR,DWORD);
WINSHLWAPI void WINAPI PathRemoveArgsA(LPSTR);
WINSHLWAPI void WINAPI PathRemoveArgsW(LPWSTR);
WINSHLWAPI LPSTR WINAPI PathRemoveBackslashA(LPSTR);
WINSHLWAPI LPWSTR WINAPI PathRemoveBackslashW(LPWSTR);
WINSHLWAPI void WINAPI PathRemoveBlanksA(LPSTR);
WINSHLWAPI void WINAPI PathRemoveBlanksW(LPWSTR);
WINSHLWAPI void WINAPI PathRemoveExtensionA(LPSTR);
WINSHLWAPI void WINAPI PathRemoveExtensionW(LPWSTR);
WINSHLWAPI BOOL WINAPI PathRemoveFileSpecA(LPSTR);
WINSHLWAPI BOOL WINAPI PathRemoveFileSpecW(LPWSTR);
WINSHLWAPI BOOL WINAPI PathRenameExtensionA(LPSTR,LPCSTR);
WINSHLWAPI BOOL WINAPI PathRenameExtensionW(LPWSTR,LPCWSTR);
WINSHLWAPI BOOL WINAPI PathSearchAndQualifyA(LPCSTR,LPSTR,UINT);
WINSHLWAPI BOOL WINAPI PathSearchAndQualifyW(LPCWSTR,LPWSTR,UINT);
WINSHLWAPI void WINAPI PathSetDlgItemPathA(HWND,int,LPCSTR);
WINSHLWAPI void WINAPI PathSetDlgItemPathW(HWND,int,LPCWSTR);
WINSHLWAPI LPSTR WINAPI PathSkipRootA(LPCSTR);
WINSHLWAPI LPWSTR WINAPI PathSkipRootW(LPCWSTR);
WINSHLWAPI void WINAPI PathStripPathA(LPSTR);
WINSHLWAPI void WINAPI PathStripPathW(LPWSTR);
WINSHLWAPI BOOL WINAPI PathStripToRootA(LPSTR);
WINSHLWAPI BOOL WINAPI PathStripToRootW(LPWSTR);
WINSHLWAPI void WINAPI PathUndecorateA(LPSTR);
WINSHLWAPI void WINAPI PathUndecorateW(LPWSTR);
WINSHLWAPI BOOL WINAPI PathUnExpandEnvStringsA(LPCSTR,LPSTR,UINT);
WINSHLWAPI BOOL WINAPI PathUnExpandEnvStringsW(LPCWSTR,LPWSTR,UINT);
WINSHLWAPI BOOL WINAPI PathUnmakeSystemFolderA(LPSTR);
WINSHLWAPI BOOL WINAPI PathUnmakeSystemFolderW(LPWSTR);
WINSHLWAPI void WINAPI PathUnquoteSpacesA(LPSTR);
WINSHLWAPI void WINAPI PathUnquoteSpacesW(LPWSTR);
WINSHLWAPI HRESULT WINAPI SHAutoComplete(HWND,DWORD);
#ifndef __OBJC__
WINSHLWAPI HRESULT WINAPI SHCreateStreamOnFileA(LPCSTR,DWORD,struct IStream**);
WINSHLWAPI HRESULT WINAPI SHCreateStreamOnFileW(LPCWSTR,DWORD,struct IStream**);
WINSHLWAPI struct IStream* WINAPI SHOpenRegStream2A(HKEY,LPCSTR,LPCSTR,DWORD);
WINSHLWAPI struct IStream* WINAPI SHOpenRegStream2W(HKEY,LPCWSTR,LPCWSTR,DWORD);
WINSHLWAPI struct IStream* WINAPI SHOpenRegStreamA(HKEY,LPCSTR,LPCSTR,DWORD);
WINSHLWAPI struct IStream* WINAPI SHOpenRegStreamW(HKEY,LPCWSTR,LPCWSTR,DWORD);
#endif
WINSHLWAPI BOOL WINAPI SHCreateThread(LPTHREAD_START_ROUTINE,void*,DWORD,LPTHREAD_START_ROUTINE);
WINSHLWAPI DWORD WINAPI SHCopyKeyA(HKEY,LPCSTR,HKEY,DWORD);
WINSHLWAPI DWORD WINAPI SHCopyKeyW(HKEY,LPCWSTR,HKEY,DWORD);
WINSHLWAPI DWORD WINAPI SHDeleteEmptyKeyA(HKEY,LPCSTR);
WINSHLWAPI DWORD WINAPI SHDeleteEmptyKeyW(HKEY,LPCWSTR);
WINSHLWAPI DWORD WINAPI SHDeleteKeyA(HKEY,LPCSTR);
WINSHLWAPI DWORD WINAPI SHDeleteKeyW(HKEY,LPCWSTR);
WINSHLWAPI DWORD WINAPI SHEnumKeyExA(HKEY,DWORD,LPSTR,LPDWORD);
WINSHLWAPI DWORD WINAPI SHEnumKeyExW(HKEY,DWORD,LPWSTR,LPDWORD);
WINSHLWAPI DWORD WINAPI SHQueryInfoKeyA(HKEY,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
WINSHLWAPI DWORD WINAPI SHQueryInfoKeyW(HKEY,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
WINSHLWAPI DWORD WINAPI SHQueryValueExA(HKEY,LPCSTR,LPDWORD,LPDWORD,LPVOID,LPDWORD);
WINSHLWAPI DWORD WINAPI SHQueryValueExW(HKEY,LPCWSTR,LPDWORD,LPDWORD,LPVOID,LPDWORD);
#ifndef __OBJC__
WINSHLWAPI HRESULT WINAPI SHGetThreadRef(IUnknown**);
WINSHLWAPI HRESULT WINAPI SHSetThreadRef(IUnknown*);
WINSHLWAPI BOOL WINAPI SHSkipJunction(IBindCtx*,const CLSID*);
#endif
WINSHLWAPI DWORD WINAPI SHEnumValueA(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPVOID,LPDWORD);
WINSHLWAPI DWORD WINAPI SHEnumValueW(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPVOID,LPDWORD);
WINSHLWAPI DWORD WINAPI SHGetValueA(HKEY,LPCSTR,LPCSTR,LPDWORD,LPVOID,LPDWORD);
WINSHLWAPI DWORD WINAPI SHGetValueW(HKEY,LPCWSTR,LPCWSTR,LPDWORD,LPVOID,LPDWORD);
WINSHLWAPI DWORD WINAPI SHSetValueA(HKEY,LPCSTR,LPCSTR,DWORD,LPCVOID,DWORD);
WINSHLWAPI DWORD WINAPI SHSetValueW(HKEY,LPCWSTR,LPCWSTR,DWORD,LPCVOID,DWORD);
WINSHLWAPI DWORD WINAPI SHDeleteValueA(HKEY,LPCSTR,LPCSTR);
WINSHLWAPI DWORD WINAPI SHDeleteValueW(HKEY,LPCWSTR,LPCWSTR);
WINSHLWAPI HRESULT WINAPI AssocCreate(CLSID,const IID* const,LPVOID*);
WINSHLWAPI HRESULT WINAPI AssocQueryKeyA(ASSOCF,ASSOCKEY,LPCSTR,LPCSTR,HKEY*);
WINSHLWAPI HRESULT WINAPI AssocQueryKeyW(ASSOCF,ASSOCKEY,LPCWSTR,LPCWSTR,HKEY*);
WINSHLWAPI HRESULT WINAPI AssocQueryStringA(ASSOCF,ASSOCSTR,LPCSTR,LPCSTR,LPSTR,DWORD*);
WINSHLWAPI HRESULT WINAPI AssocQueryStringByKeyA(ASSOCF,ASSOCSTR,HKEY,LPCSTR,LPSTR,DWORD*);
WINSHLWAPI HRESULT WINAPI AssocQueryStringByKeyW(ASSOCF,ASSOCSTR,HKEY,LPCWSTR,LPWSTR,DWORD*);
WINSHLWAPI HRESULT WINAPI AssocQueryStringW(ASSOCF,ASSOCSTR,LPCWSTR,LPCWSTR,LPWSTR,DWORD*);
   
WINSHLWAPI HRESULT WINAPI UrlApplySchemeA(LPCSTR,LPSTR,LPDWORD,DWORD);
WINSHLWAPI HRESULT WINAPI UrlApplySchemeW(LPCWSTR,LPWSTR,LPDWORD,DWORD);
WINSHLWAPI HRESULT WINAPI UrlCanonicalizeA(LPCSTR,LPSTR,LPDWORD,DWORD);
WINSHLWAPI HRESULT WINAPI UrlCanonicalizeW(LPCWSTR,LPWSTR,LPDWORD,DWORD);
WINSHLWAPI HRESULT WINAPI UrlCombineA(LPCSTR,LPCSTR,LPSTR,LPDWORD,DWORD);
WINSHLWAPI HRESULT WINAPI UrlCombineW(LPCWSTR,LPCWSTR,LPWSTR,LPDWORD,DWORD);
WINSHLWAPI int WINAPI UrlCompareA(LPCSTR,LPCSTR,BOOL);
WINSHLWAPI int WINAPI UrlCompareW(LPCWSTR,LPCWSTR,BOOL);
WINSHLWAPI HRESULT WINAPI UrlCreateFromPathA(LPCSTR,LPSTR,LPDWORD,DWORD);
WINSHLWAPI HRESULT WINAPI UrlCreateFromPathW(LPCWSTR,LPWSTR,LPDWORD,DWORD);
WINSHLWAPI HRESULT WINAPI UrlEscapeA(LPCSTR,LPSTR,LPDWORD,DWORD);
WINSHLWAPI HRESULT WINAPI UrlEscapeW(LPCWSTR,LPWSTR,LPDWORD,DWORD);
WINSHLWAPI LPCSTR WINAPI UrlGetLocationA(LPCSTR);
WINSHLWAPI LPCWSTR WINAPI UrlGetLocationW(LPCWSTR);
WINSHLWAPI HRESULT WINAPI UrlGetPartA(LPCSTR,LPSTR,LPDWORD,DWORD,DWORD);
WINSHLWAPI HRESULT WINAPI UrlGetPartW(LPCWSTR,LPWSTR,LPDWORD,DWORD,DWORD);
WINSHLWAPI HRESULT WINAPI UrlHashA(LPCSTR,LPBYTE,DWORD);
WINSHLWAPI HRESULT WINAPI UrlHashW(LPCWSTR,LPBYTE,DWORD);
WINSHLWAPI BOOL WINAPI UrlIsA(LPCSTR,URLIS);
WINSHLWAPI BOOL WINAPI UrlIsW(LPCWSTR,URLIS);
#define UrlIsFileUrlA(pszURL) UrlIsA(pzURL, URLIS_FILEURL)
#define UrlIsFileUrlW(pszURL) UrlIsW(pszURL, URLIS_FILEURL)
WINSHLWAPI BOOL WINAPI UrlIsNoHistoryA(LPCSTR);
WINSHLWAPI BOOL WINAPI UrlIsNoHistoryW(LPCWSTR);
WINSHLWAPI BOOL WINAPI UrlIsOpaqueA(LPCSTR);
WINSHLWAPI BOOL WINAPI UrlIsOpaqueW(LPCWSTR);
WINSHLWAPI HRESULT WINAPI UrlUnescapeA(LPSTR,LPSTR,LPDWORD,DWORD);
WINSHLWAPI HRESULT WINAPI UrlUnescapeW(LPWSTR,LPWSTR,LPDWORD,DWORD);
#define UrlUnescapeInPlaceA(pszUrl,dwFlags )\
	UrlUnescapeA(pszUrl, NULL, NULL, dwFlags | URL_UNESCAPE_INPLACE)
#define UrlUnescapeInPlaceW(pszUrl,dwFlags )\
	UrlUnescapeW(pszUrl, NULL, NULL, dwFlags | URL_UNESCAPE_INPLACE)
WINSHLWAPI DWORD WINAPI SHRegCloseUSKey(HUSKEY);
WINSHLWAPI LONG WINAPI SHRegCreateUSKeyA(LPCSTR,REGSAM,HUSKEY,PHUSKEY,DWORD);
WINSHLWAPI LONG WINAPI SHRegCreateUSKeyW(LPCWSTR,REGSAM,HUSKEY,PHUSKEY,DWORD);
WINSHLWAPI LONG WINAPI SHRegDeleteEmptyUSKeyA(HUSKEY,LPCSTR,SHREGDEL_FLAGS);
WINSHLWAPI LONG WINAPI SHRegDeleteEmptyUSKeyW(HUSKEY,LPCWSTR,SHREGDEL_FLAGS);
WINSHLWAPI LONG WINAPI SHRegDeleteUSValueA(HUSKEY,LPCSTR,SHREGDEL_FLAGS);
WINSHLWAPI LONG WINAPI SHRegDeleteUSValueW(HUSKEY,LPCWSTR,SHREGDEL_FLAGS);
WINSHLWAPI HKEY WINAPI SHRegDuplicateHKey(HKEY);
WINSHLWAPI DWORD WINAPI SHRegEnumUSKeyA(HUSKEY,DWORD,LPSTR,LPDWORD,SHREGENUM_FLAGS);
WINSHLWAPI DWORD WINAPI SHRegEnumUSKeyW(HUSKEY,DWORD,LPWSTR,LPDWORD,SHREGENUM_FLAGS);
WINSHLWAPI DWORD WINAPI SHRegEnumUSValueA(HUSKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPVOID,LPDWORD,SHREGENUM_FLAGS);
WINSHLWAPI DWORD WINAPI SHRegEnumUSValueW(HUSKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPVOID,LPDWORD,SHREGENUM_FLAGS);
WINSHLWAPI BOOL WINAPI SHRegGetBoolUSValueA(LPCSTR,LPCSTR,BOOL,BOOL);
WINSHLWAPI BOOL WINAPI SHRegGetBoolUSValueW(LPCWSTR,LPCWSTR,BOOL,BOOL);
WINSHLWAPI DWORD WINAPI SHRegGetPathA(HKEY,LPCSTR,LPCSTR,LPSTR,DWORD);
WINSHLWAPI DWORD WINAPI SHRegGetPathW(HKEY,LPCWSTR,LPCWSTR,LPWSTR,DWORD);
WINSHLWAPI LONG WINAPI SHRegGetUSValueA(LPCSTR,LPCSTR,LPDWORD,LPVOID,LPDWORD,BOOL,LPVOID,DWORD);
WINSHLWAPI LONG WINAPI SHRegGetUSValueW(LPCWSTR,LPCWSTR,LPDWORD,LPVOID,LPDWORD,BOOL,LPVOID,DWORD);
WINSHLWAPI LONG WINAPI SHRegOpenUSKeyA(LPCSTR,REGSAM,HUSKEY,PHUSKEY,BOOL);
WINSHLWAPI LONG WINAPI SHRegOpenUSKeyW(LPCWSTR,REGSAM,HUSKEY,PHUSKEY,BOOL);
WINSHLWAPI DWORD WINAPI SHRegQueryInfoUSKeyA(HUSKEY,LPDWORD,LPDWORD,LPDWORD,LPDWORD,SHREGENUM_FLAGS);
WINSHLWAPI DWORD WINAPI SHRegQueryInfoUSKeyW(HUSKEY,LPDWORD,LPDWORD,LPDWORD,LPDWORD,SHREGENUM_FLAGS);
WINSHLWAPI LONG WINAPI SHRegQueryUSValueA(HUSKEY,LPCSTR,LPDWORD,LPVOID,LPDWORD,BOOL,LPVOID,DWORD);
WINSHLWAPI LONG WINAPI SHRegQueryUSValueW(HUSKEY,LPCWSTR,LPDWORD,LPVOID,LPDWORD,BOOL,LPVOID,DWORD);
WINSHLWAPI DWORD WINAPI SHRegSetPathA(HKEY,LPCSTR,LPCSTR,LPCSTR,DWORD);
WINSHLWAPI DWORD WINAPI SHRegSetPathW(HKEY,LPCWSTR,LPCWSTR,LPCWSTR,DWORD);
WINSHLWAPI LONG WINAPI SHRegSetUSValueA(LPCSTR,LPCSTR,DWORD,LPVOID,DWORD,DWORD);
WINSHLWAPI LONG WINAPI SHRegSetUSValueW(LPCWSTR,LPCWSTR,DWORD,LPVOID,DWORD,DWORD);
WINSHLWAPI LONG WINAPI SHRegWriteUSValueA(HUSKEY,LPCSTR,DWORD,LPVOID,DWORD,DWORD);
WINSHLWAPI LONG WINAPI SHRegWriteUSValueW(HUSKEY,LPCWSTR,DWORD,LPVOID,DWORD,DWORD);
WINSHLWAPI HRESULT WINAPI HashData(LPBYTE,DWORD,LPBYTE,DWORD);
WINSHLWAPI HPALETTE WINAPI SHCreateShellPalette(HDC);
WINSHLWAPI COLORREF WINAPI ColorHLSToRGB(WORD,WORD,WORD);
WINSHLWAPI COLORREF WINAPI ColorAdjustLuma(COLORREF,int,BOOL);
WINSHLWAPI void WINAPI ColorRGBToHLS(COLORREF,WORD*,WORD*,WORD*);  
WINSHLWAPI int __cdecl wnsprintfA(LPSTR,int,LPCSTR,...);
WINSHLWAPI int __cdecl wnsprintfW(LPWSTR,int,LPCWSTR,...);
WINSHLWAPI int WINAPI wvnsprintfA(LPSTR,int,LPCSTR,va_list);
WINSHLWAPI int WINAPI wvnsprintfW(LPWSTR,int,LPCWSTR,va_list);

HINSTANCE WINAPI MLLoadLibraryA(LPCSTR,HANDLE,DWORD,LPCSTR,BOOL);
HINSTANCE WINAPI MLLoadLibraryW(LPCWSTR,HANDLE,DWORD,LPCWSTR,BOOL);

HRESULT WINAPI DllInstall(BOOL,LPCWSTR);

#ifdef UNICODE
#define ChrCmpI ChrCmpIW
#define IntlStrEqN IntlStrEqNW
#define IntlStrEqNI IntlStrEqNIW
#define IntlStrEqWorker IntlStrEqWorkerW
#define SHStrDup SHStrDupW
#define StrCat StrCatW
#define StrCatBuff StrCatBuffW
#define StrChr StrChrW
#define StrChrI StrChrIW
#define StrCmp StrCmpW
#define StrCmpI StrCmpIW
#define StrCmpNI StrCmpNIW
#define StrCmpN StrCmpNW
#define StrCpyN StrCpyNW
#define StrCpy StrCpyW
#define StrCSpnI StrCSpnIW
#define StrCSpn StrCSpnW
#define StrDup StrDupW
#define StrFormatByteSize StrFormatByteSizeW
#define StrFormatKBSize StrFormatKBSizeW
#define StrFromTimeInterval StrFromTimeIntervalW
#define StrIsIntlEqual StrIsIntlEqualW
#define StrNCat StrNCatW
#define StrPBrk StrPBrkW
#define StrRChr StrRChrW
#define StrRChrI StrRChrIW
#ifndef __OBJC__
#define StrRetToBuf StrRetToBufW
#define StrRetToStr StrRetToStrW
#endif
#define StrRStrI StrRStrIW
#define StrSpn StrSpnW
#define StrStrI StrStrIW
#define StrStr StrStrW
#define StrToInt StrToIntW
#define StrToIntEx StrToIntExW
#define StrTrim StrTrimW
#define PathAddBackslash PathAddBackslashW
#define PathAddExtension PathAddExtensionW
#define PathAppend PathAppendW
#define PathBuildRoot PathBuildRootW
#define PathCanonicalize PathCanonicalizeW
#define PathCombine PathCombineW
#define PathCommonPrefix PathCommonPrefixW
#define PathCompactPath PathCompactPathW
#define PathCompactPathEx PathCompactPathExW
#define PathCreateFromUrl PathCreateFromUrlW
#define PathFileExists PathFileExistsW
#define PathFindExtension PathFindExtensionW
#define PathFindFileName PathFindFileNameW
#define PathFindNextComponent PathFindNextComponentW
#define PathFindOnPath PathFindOnPathW
#define PathFindSuffixArray PathFindSuffixArrayW
#define PathGetArgs PathGetArgsW
#define PathGetCharType PathGetCharTypeW
#define PathGetDriveNumber PathGetDriveNumberW
#define PathIsContentType PathIsContentTypeW
#define PathIsDirectoryEmpty PathIsDirectoryEmptyW
#define PathIsDirectory PathIsDirectoryW
#define PathIsFileSpec PathIsFileSpecW
#define PathIsLFNFileSpec PathIsLFNFileSpecW
#define PathIsNetworkPath PathIsNetworkPathW
#define PathIsPrefix PathIsPrefixW
#define PathIsRelative PathIsRelativeW
#define PathIsRoot PathIsRootW
#define PathIsSameRoot PathIsSameRootW
#define PathIsSystemFolder PathIsSystemFolderW
#define PathIsUNCServerShare PathIsUNCServerShareW
#define PathIsUNCServer PathIsUNCServerW
#define PathIsUNC PathIsUNCW
#define PathIsURL PathIsURLW
#define PathMakePretty PathMakePrettyW
#define PathMakeSystemFolder PathMakeSystemFolderW
#define PathMatchSpec PathMatchSpecW
#define PathParseIconLocation PathParseIconLocationW
#define PathQuoteSpaces PathQuoteSpacesW
#define PathRelativePathTo PathRelativePathToW
#define PathRemoveArgs PathRemoveArgsW
#define PathRemoveBackslash PathRemoveBackslashW
#define PathRemoveBlanks PathRemoveBlanksW
#define PathRemoveExtension PathRemoveExtensionW
#define PathRemoveFileSpec PathRemoveFileSpecW
#define PathRenameExtension PathRenameExtensionW
#define PathSearchAndQualify PathSearchAndQualifyW
#define PathSetDlgItemPath PathSetDlgItemPathW
#define PathSkipRoot PathSkipRootW
#define PathStripPath PathStripPathW
#define PathStripToRoot PathStripToRootW
#define PathUndecorate PathUndecorateW
#define PathUnExpandEnvStrings PathUnExpandEnvStringsW
#define PathUnmakeSystemFolder PathUnmakeSystemFolderW
#define PathUnquoteSpaces PathUnquoteSpacesW
#ifndef __OBJC__
#define SHCreateStreamOnFile SHCreateStreamOnFileW
#define SHOpenRegStream SHOpenRegStreamW
#define SHOpenRegStream2 SHOpenRegStream2W
#endif
#define SHCopyKey SHCopyKeyW
#define SHDeleteEmptyKey SHDeleteEmptyKeyW
#define SHDeleteKey SHDeleteKeyW
#define SHEnumKeyEx SHEnumKeyExW
#define SHQueryInfoKey SHRegQueryInfoKeyW
#define SHQueryValueEx SHQueryValueExW
#define SHEnumValue SHEnumValueW
#define SHGetValue SHGetValueW
#define SHSetValue SHSetValueW
#define SHDeleteValue SHDeleteValueW
#define AssocQueryKey AssocQueryKeyW
#define AssocQueryStringByKey AssocQueryStringByKeyW
#define AssocQueryString AssocQueryStringW
#define UrlApplyScheme UrlApplySchemeW
#define UrlCanonicalize UrlCanonicalizeW
#define UrlCombine UrlCombineW
#define UrlCompare UrlCompareW
#define UrlCreateFromPath UrlCreateFromPathW
#define UrlEscape UrlEscapeW
#define UrlGetLocation UrlGetLocationW
#define UrlGetPart UrlGetPartW
#define UrlHash UrlHashW
#define UrlIs UrlIsW
#define UrlIsFileUrl UrlIsFileUrlW
#define UrlIsNoHistory UrlIsNoHistoryW
#define UrlIsOpaque UrlIsOpaqueW
#define UrlUnescape UrlUnescapeW
#define UrlUnescapeInPlace UrlUnescapeInPlaceW
#define SHRegCreateUSKey SHRegCreateUSKeyW
#define SHRegDeleteEmptyUSKey SHRegDeleteEmptyUSKeyW
#define SHRegDeleteUSValue SHRegDeleteUSValueW
#define SHRegEnumUSKey SHRegEnumUSKeyW
#define SHRegEnumUSValue SHRegEnumUSValueW
#define SHRegGetBoolUSValue SHRegGetBoolUSValueW
#define SHRegGetPath SHRegGetPathW
#define SHRegGetUSValue SHRegGetUSValueW
#define SHRegOpenUSKey SHRegOpenUSKeyW
#define SHRegQueryInfoUSKey SHRegQueryInfoUSKeyW
#define SHRegQueryUSValue SHRegQueryUSValueW
#define SHRegSetPath SHRegSetPathW
#define SHRegSetUSValue SHRegSetUSValueW
#define SHRegWriteUSValue SHRegWriteUSValueW
#define wnsprintf wnsprintfW
#define wvnsprintf wvnsprintfW
#else /* UNICODE */
#define ChrCmpI ChrCmpIA
#define IntlStrEqN IntlStrEqNA
#define IntlStrEqNI IntlStrEqNIA
#define IntlStrEqWorker IntlStrEqWorkerA
#define SHStrDup SHStrDupA
#define StrCat lstrcatA
#define StrCatBuff StrCatBuffA
#define StrChr StrChrA
#define StrChrI StrChrIA
#define StrCmp lstrcmpA
#define StrCmpI lstrcmpiA
#define StrCmpNI StrCmpNIA
#define StrCmpN StrCmpNA
#define StrCpyN lstrcpynA
#define StrCpy lstrcpyA
#define StrCSpnI StrCSpnIA
#define StrCSpn StrCSpnA
#define StrDup StrDupA
#define StrFormatByteSize StrFormatByteSizeA
#define StrFormatKBSize StrFormatKBSizeA
#define StrFromTimeInterval StrFromTimeIntervalA
#define StrIsIntlEqual StrIsIntlEqualA
#define StrNCat StrNCatA
#define StrPBrk StrPBrkA
#define StrRChr StrRChrA
#define StrRChrI StrRChrIA
#ifndef __OBJC__
#define StrRetToBuf StrRetToBufA
#define StrRetToStr StrRetToStrA
#endif
#define StrRStrI StrRStrIA
#define StrSpn StrSpnA
#define StrStrI StrStrIA
#define StrStr StrStrA
#define StrToInt StrToIntA
#define StrToIntEx StrToIntExA
#define StrTrim StrTrimA
#define PathAddBackslash PathAddBackslashA
#define PathAddExtension PathAddExtensionA
#define PathAppend PathAppendA
#define PathBuildRoot PathBuildRootA
#define PathCanonicalize PathCanonicalizeA
#define PathCombine PathCombineA
#define PathCommonPrefix PathCommonPrefixA
#define PathCompactPath PathCompactPathA
#define PathCompactPathEx PathCompactPathExA
#define PathCreateFromUrl PathCreateFromUrlA
#define PathFileExists PathFileExistsA
#define PathFindExtension PathFindExtensionA
#define PathFindFileName PathFindFileNameA
#define PathFindNextComponent PathFindNextComponentA
#define PathFindOnPath PathFindOnPathA
#define PathFindSuffixArray PathFindSuffixArrayA
#define PathGetArgs PathGetArgsA
#define PathGetCharType PathGetCharTypeA
#define PathGetDriveNumber PathGetDriveNumberA
#define PathIsContentType PathIsContentTypeA
#define PathIsDirectoryEmpty PathIsDirectoryEmptyA
#define PathIsDirectory PathIsDirectoryA
#define PathIsFileSpec PathIsFileSpecA
#define PathIsLFNFileSpec PathIsLFNFileSpecA
#define PathIsNetworkPath PathIsNetworkPathA
#define PathIsPrefix PathIsPrefixA
#define PathIsRelative PathIsRelativeA
#define PathIsRoot PathIsRootA
#define PathIsSameRoot PathIsSameRootA
#define PathIsSystemFolder PathIsSystemFolderA
#define PathIsUNCServerShare PathIsUNCServerShareA
#define PathIsUNCServer PathIsUNCServerA
#define PathIsUNC PathIsUNCA
#define PathIsURL PathIsURLA
#define PathMakePretty PathMakePrettyA
#define PathMakeSystemFolder PathMakeSystemFolderA
#define PathMatchSpec PathMatchSpecA
#define PathParseIconLocation PathParseIconLocationA
#define PathQuoteSpaces PathQuoteSpacesA
#define PathRelativePathTo PathRelativePathToA
#define PathRemoveArgs PathRemoveArgsA
#define PathRemoveBackslash PathRemoveBackslashA
#define PathRemoveBlanks PathRemoveBlanksA
#define PathRemoveExtension PathRemoveExtensionA
#define PathRemoveFileSpec PathRemoveFileSpecA
#define PathRenameExtension PathRenameExtensionA
#define PathSearchAndQualify PathSearchAndQualifyA
#define PathSetDlgItemPath PathSetDlgItemPathA
#define PathSkipRoot PathSkipRootA
#define PathStripPath PathStripPathA
#define PathStripToRoot PathStripToRootA
#define PathUndecorate PathUndecorateA
#define PathUnExpandEnvStrings PathUnExpandEnvStringsA
#define PathUnmakeSystemFolder PathUnmakeSystemFolderA
#define PathUnquoteSpaces PathUnquoteSpacesA
#ifndef __OBJC__
#define SHCreateStreamOnFile SHCreateStreamOnFileA
#define SHOpenRegStream SHOpenRegStreamA
#define SHOpenRegStream2 SHOpenRegStream2A
#endif
#define SHCopyKey SHCopyKeyA
#define SHDeleteEmptyKey SHDeleteEmptyKeyA
#define SHDeleteKey SHDeleteKeyA
#define SHEnumKeyEx SHEnumKeyExA
#define SHQueryInfoKey SHRegQueryInfoKeyA
#define SHQueryValueEx SHQueryValueExA
#define SHEnumValue SHEnumValueA
#define SHGetValue SHGetValueA
#define SHSetValue SHSetValueA
#define SHDeleteValue SHDeleteValueA
#define AssocQueryKey AssocQueryKeyA
#define AssocQueryStringByKey AssocQueryStringByKeyA
#define AssocQueryString AssocQueryStringA
#define UrlApplyScheme UrlApplySchemeA
#define UrlCanonicalize UrlCanonicalizeA
#define UrlCombine UrlCombineA
#define UrlCompare UrlCompareA
#define UrlCreateFromPath UrlCreateFromPathA
#define UrlEscape UrlEscapeA
#define UrlGetLocation UrlGetLocationA
#define UrlGetPart UrlGetPartA
#define UrlHash UrlHashA
#define UrlIs UrlIsA
#define UrlIsFileUrl UrlIsFileUrl
#define UrlIsNoHistory UrlIsNoHistoryA
#define UrlIsOpaque UrlIsOpaqueA
#define UrlUnescape UrlUnescapeA
#define UrlUnescapeInPlace UrlUnescapeInPlaceA
#define SHRegCreateUSKey SHRegCreateUSKeyA
#define SHRegDeleteEmptyUSKey SHRegDeleteEmptyUSKeyA
#define SHRegDeleteUSValue SHRegDeleteUSValueA
#define SHRegEnumUSKey SHRegEnumUSKeyA
#define SHRegEnumUSValue SHRegEnumUSValueA
#define SHRegGetBoolUSValue SHRegGetBoolUSValueA
#define SHRegGetPath SHRegGetPathA
#define SHRegGetUSValue SHRegGetUSValueA
#define SHRegOpenUSKey SHRegOpenUSKeyA
#define SHRegQueryInfoUSKey SHRegQueryInfoUSKeyA
#define SHRegQueryUSValue SHRegQueryUSValueA
#define SHRegSetPath SHRegSetPathA
#define SHRegSetUSValue SHRegSetUSValueA
#define SHRegWriteUSValue SHRegWriteUSValueA
#define wnsprintf wnsprintfA
#define wvnsprintf wvnsprintfA
#endif /* UNICODE */

#define StrToLong StrToInt

#endif /* !RC_INVOKED */

#ifdef __cplusplus
}
#endif
#endif /* ! defined _SHLWAPI_H */
