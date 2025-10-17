/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     shlwapi header
 * COPYRIGHT:   Copyright 2025 Carl Bialorucki (carl.bialorucki@reactos.org)
 */

#ifndef _INC_SHLWAPI
#define _INC_SHLWAPI
#ifndef NOSHLWAPI

#include <specstrings.h>
#include <objbase.h>
#include <shtypes.h>
#include <pshpack8.h>

#ifndef WINSHLWAPI
#ifdef __GNUC__
/* FIXME: HACK: CORE-6504 */
#define LWSTDAPI_(t)   t WINAPI
#define LWSTDAPIV_(t)  t WINAPIV
#else
#ifndef _SHLWAPI_
#define LWSTDAPI_(t)   EXTERN_C DECLSPEC_IMPORT t STDAPICALLTYPE
#define LWSTDAPIV_(t)  EXTERN_C DECLSPEC_IMPORT t STDAPIVCALLTYPE
#else
#define LWSTDAPI_(t)   STDAPI_(t)
#define LWSTDAPIV_(t)  STDAPIV_(t)
#endif // _SHLWAPI_
#endif // __GNUC__
#define LWSTDAPI       LWSTDAPI_(HRESULT)
#define LWSTDAPIV      LWSTDAPIV_(HRESULT)
#endif // WINSHLWAPI

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GLOBAL SHLWAPI DEFINITIONS
 * These definitions are always available.
 */

/* SHAutoComplete definitions */
#define SHACF_DEFAULT               0x00000000
#define SHACF_FILESYSTEM            0x00000001
#define SHACF_URLHISTORY            0x00000002
#define SHACF_URLMRU                0x00000004
#define SHACF_URLALL                (SHACF_URLHISTORY|SHACF_URLMRU)
#define SHACF_USETAB                0x00000008
#define SHACF_FILESYS_ONLY          0x00000010
#if (_WIN32_IE >= _WIN32_IE_IE60)
#define SHACF_FILESYS_DIRS          0x00000020
#endif // _WIN32_IE_IE60
#if (_WIN32_IE >= _WIN32_IE_IE70)
#define SHACF_VIRTUAL_NAMESPACE     0x00000040
#endif // _WIN32_IE_IE70
#define SHACF_AUTOSUGGEST_FORCE_ON  0x10000000
#define SHACF_AUTOSUGGEST_FORCE_OFF 0x20000000
#define SHACF_AUTOAPPEND_FORCE_ON   0x40000000
#define SHACF_AUTOAPPEND_FORCE_OFF  0x80000000

LWSTDAPI        SHAutoComplete(_In_ HWND, DWORD);
LWSTDAPI        SHGetThreadRef(_Outptr_ IUnknown**);
LWSTDAPI        SHSetThreadRef(_In_opt_ IUnknown*);
LWSTDAPI_(BOOL) SHCreateThread(_In_ LPTHREAD_START_ROUTINE pfnThreadProc, _In_opt_ void* pData, _In_ DWORD flags, _In_opt_ LPTHREAD_START_ROUTINE pfnCallback);
LWSTDAPI_(BOOL) SHSkipJunction(_In_opt_ struct IBindCtx*, _In_ const CLSID*);

/* SHCreateThread definitions */
enum {
    CTF_INSIST             = 0x0001,
    CTF_THREAD_REF         = 0x0002,
    CTF_PROCESS_REF        = 0x0004,
    CTF_COINIT_STA         = 0x0008,
    CTF_COINIT             = 0x0008,
#if (_WIN32_IE >= _WIN32_IE_IE60)
    CTF_FREELIBANDEXIT     = 0x0010,
    CTF_REF_COUNTED        = 0x0020,
    CTF_WAIT_ALLOWCOM      = 0x0040,
#endif // _WIN32_IE_IE60
#if (_WIN32_IE >= _WIN32_IE_IE70)
    CTF_UNUSED             = 0x0080,
    CTF_INHERITWOW64       = 0x0100,
#endif // _WIN32_IE_IE70
#if (NTDDI_VERSION >= NTDDI_VISTA)
    CTF_WAIT_NO_REENTRANCY = 0x0200,
#endif // NTDDI_VERSION >= NTDDI_VISTA
#if (NTDDI_VERSION >= NTDDI_WIN7)
    CTF_KEYBOARD_LOCALE    = 0x0400,
    CTF_OLEINITIALIZE      = 0x0800,
    CTF_COINIT_MTA         = 0x1000,
    CTF_NOADDREFLIB        = 0x2000,
#endif // NTDDI_VERSION >= NTDDI_WIN7
};

/* SHFormatDateTime definitions */
#define FDTF_SHORTTIME          0x00000001
#define FDTF_SHORTDATE          0x00000002
#define FDTF_DEFAULT            (FDTF_SHORTDATE | FDTF_SHORTTIME)
#define FDTF_LONGDATE           0x00000004
#define FDTF_LONGTIME           0x00000008
#define FDTF_RELATIVE           0x00000010
#define FDTF_LTRDATE            0x00000100
#define FDTF_RTLDATE            0x00000200
#define FDTF_NOAUTOREADINGORDER 0x00000400

/* DLL version definitions */
#define DLLVER_MAJOR_MASK 0xFFFF000000000000
#define DLLVER_MINOR_MASK 0x0000FFFF00000000
#define DLLVER_BUILD_MASK 0x00000000FFFF0000
#define DLLVER_QFE_MASK   0x000000000000FFFF

#define MAKEDLLVERULL(major, minor, build, qfe) \
    (((ULONGLONG)(major) << 48) | ((ULONGLONG)(minor) << 32) | ((ULONGLONG)(build) << 16) | (ULONGLONG)(qfe))

#define DLLVER_PLATFORM_WINDOWS 0x01  // Windows 9.x
#define DLLVER_PLATFORM_NT      0x02  // Windows NT

typedef struct _DllVersionInfo {
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
} DLLVERSIONINFO;

typedef HRESULT (CALLBACK *DLLGETVERSIONPROC)(DLLVERSIONINFO *);

typedef struct _DLLVERSIONINFO2 {
    DLLVERSIONINFO info1;
    DWORD dwFlags;
    ULONGLONG ullVersion;
} DLLVERSIONINFO2;

STDAPI DllInstall(BOOL, _In_opt_ LPCWSTR) DECLSPEC_HIDDEN;

/* QueryInterface definitions */
typedef struct
{
    const IID *piid;
#if defined(__REACTOS__) || (WINVER >= _WIN32_WINNT_WIN10)
    DWORD dwOffset;
#else
    int dwOffset;
#endif
} QITAB, *LPQITAB;

#define OFFSETOFCLASS(base, derived) \
    ((DWORD)(DWORD_PTR)(static_cast<base*>((derived*)8))-8)

#define QITABENTMULTI(Cthis, Ifoo, Iimpl) { &IID_##Ifoo, OFFSETOFCLASS(Iimpl, Cthis) }
#define QITABENT(Cthis, Ifoo) QITABENTMULTI(Cthis, Ifoo, Ifoo)
STDAPI QISearch(_Inout_ void* base, _In_ const QITAB *pqit, _In_ REFIID riid, _Outptr_ void **ppv);

/* Miscellaneous shell functions */
LWSTDAPI_(int)  GetMenuPosFromID(_In_ HMENU hMenu, _In_ UINT uID);
LWSTDAPI_(void) IUnknown_AtomicRelease(_Inout_opt_ IUnknown **punk);
LWSTDAPI_(void) IUnknown_Set(_Inout_ IUnknown **ppunk, _In_opt_ IUnknown *punk);
LWSTDAPI        IUnknown_GetWindow(_In_ IUnknown *punk, _Out_ HWND *phwnd);
LWSTDAPI        IUnknown_SetSite(_In_ IUnknown *punk, _In_opt_ IUnknown *punkSite);
LWSTDAPI        IUnknown_GetSite(_In_ IUnknown *punk, _In_ REFIID riid, _Outptr_ void **ppv);
LWSTDAPI        IUnknown_QueryService(_In_opt_ IUnknown *punk, _In_ REFGUID guidService, _In_ REFIID riid, _Outptr_ void **ppvOut);
LWSTDAPI        IStream_Size(_In_ IStream *pstm, _Out_ ULARGE_INTEGER *pui);

/* Version gated definitions */
#if (_WIN32_IE >= _WIN32_IE_IE60)
#define SHGVSPB_PERUSER           0x00000001
#define SHGVSPB_ALLUSERS          0x00000002
#define SHGVSPB_PERFOLDER         0x00000004
#define SHGVSPB_ALLFOLDERS        0x00000008
#define SHGVSPB_INHERIT           0x00000010
#define SHGVSPB_ROAM              0x00000020
#define SHGVSPB_NOAUTODEFAULTS    0x80000000
#define SHGVSPB_FOLDER            (SHGVSPB_PERUSER | SHGVSPB_PERFOLDER)
#define SHGVSPB_FOLDERNODEFAULTS  (SHGVSPB_PERUSER | SHGVSPB_PERFOLDER | SHGVSPB_NOAUTODEFAULTS)
#define SHGVSPB_USERDEFAULTS      (SHGVSPB_PERUSER | SHGVSPB_ALLFOLDERS)
#define SHGVSPB_GLOBALDEFAULTS    (SHGVSPB_ALLUSERS | SHGVSPB_ALLFOLDERS)

LWSTDAPI SHGetViewStatePropertyBag(_In_opt_ PCIDLIST_ABSOLUTE pidl, _In_opt_ LPCWSTR bag_name, _In_ DWORD flags, _In_ REFIID riid, _Outptr_ void **ppv);
LWSTDAPI SHReleaseThreadRef(void);
#endif  // (_WIN32_IE >= _WIN32_IE_IE60)

#if (_WIN32_IE >= 0x0602)
LWSTDAPI_(BOOL) IsInternetESCEnabled(void);
#endif // (_WIN32_IE >= 0x0602)

/* According to Geoff Chappell, SHAllocShared, SHFreeShared, SHLockShared, and SHUnlockShared 
 * were available in shlwapi.dll starting with IE40. However, Microsoft didn't expose them
 * in their public shlwapi header until IE60SP2.
 */
#if (_WIN32_IE >= _WIN32_IE_IE60SP2)
LWSTDAPI_(HANDLE) SHAllocShared(_In_opt_ const void *pvData, _In_ DWORD dwSize, _In_ DWORD dwDestinationProcessId);
LWSTDAPI_(BOOL)   SHFreeShared(_In_ HANDLE hData, _In_ DWORD dwProcessId);
LWSTDAPI_(PVOID)  SHLockShared(_In_ HANDLE hData, _In_ DWORD dwProcessId);
LWSTDAPI_(BOOL)   SHUnlockShared(_In_  void *pvData);
LWSTDAPI          SHCreateThreadRef(_Inout_ LONG*, _Outptr_ IUnknown**);
#endif // _WIN32_IE >= _WIN32_IE_IE60SP2

/*
 * OPTIONAL SHLWAPI DEFINITIONS
 * These definitions can be turned on or off depending on what constants are defined.
 *
 * | Constant           | Effect                      |
 * |--------------------|-----------------------------|
 * | NO_SHLWAPI_GDI     | Disables GDI functions      |
 * | NO_SHLWAPI_ISOS    | Disables IsOS function      |
 * | NO_SHLWAPI_PATH    | Disables path functions     |
 * | NO_SHLWAPI_REG     | Disables registry functions |
 * | NO_SHLWAPI_STREAM  | Disables stream functions   |
 * | NO_SHLWAPI_STRFCNS | Disables string functions   |
 */

/* GDI functions */
#ifndef NO_SHLWAPI_GDI

LWSTDAPI_(COLORREF) ColorAdjustLuma(COLORREF,int,BOOL);
LWSTDAPI_(COLORREF) ColorHLSToRGB(WORD,WORD,WORD);
LWSTDAPI_(void)     ColorRGBToHLS(COLORREF, _Out_ LPWORD, _Out_ LPWORD, _Out_ LPWORD);
LWSTDAPI_(HPALETTE) SHCreateShellPalette(_In_opt_ HDC);

#endif // NO_SHLWAPI_GDI

/* IsOS function */
#ifndef NO_SHLWAPI_ISOS

#define OS_WINDOWS                0
#define OS_NT                     1
#define OS_WIN95ORGREATER         2
#define OS_NT4ORGREATER           3
/* 4 is omitted on purpose */
#define OS_WIN98ORGREATER         5
#define OS_WIN98_GOLD             6
#define OS_WIN2000ORGREATER       7
#define OS_WIN2000PRO             8
#define OS_WIN2000SERVER          9
#define OS_WIN2000ADVSERVER       10
#define OS_WIN2000DATACENTER      11
#define OS_WIN2000TERMINAL        12
#define OS_EMBEDDED               13
#define OS_TERMINALCLIENT         14
#define OS_TERMINALREMOTEADMIN    15
#define OS_WIN95_GOLD             16
#define OS_MEORGREATER            17
#define OS_XPORGREATER            18
#define OS_HOME                   19
#define OS_PROFESSIONAL           20
#define OS_DATACENTER             21
#define OS_ADVSERVER              22
#define OS_SERVER                 23
#define OS_TERMINALSERVER         24
#define OS_PERSONALTERMINALSERVER 25
#define OS_FASTUSERSWITCHING      26
#define OS_WELCOMELOGONUI         27
#define OS_DOMAINMEMBER           28
#define OS_ANYSERVER              29
#define OS_WOW6432                30
#define OS_WEBSERVER              31
#define OS_SMALLBUSINESSSERVER    32
#define OS_TABLETPC               33
#define OS_SERVERADMINUI          34
#define OS_MEDIACENTER            35
#define OS_APPLIANCE              36

LWSTDAPI_(BOOL) IsOS(DWORD dwOS);

#endif // NO_SHLWAPI_ISOS

/* Path functions */
#ifndef NO_SHLWAPI_PATH

#define GCT_INVALID     0x0
#define GCT_LFNCHAR     0x1
#define GCT_SHORTCHAR   0x2
#define GCT_WILD        0x4
#define GCT_SEPARATOR   0x8

LWSTDAPI           HashData(_In_reads_bytes_(cbData) const unsigned char *, DWORD cbData, _Out_writes_bytes_(cbHash) unsigned char *lpDest, DWORD cbHash);
#if (_WIN32_IE >= _WIN32_IE_IE70)
LWSTDAPI           PathCreateFromUrlAlloc(_In_ LPCWSTR pszUrl, _Outptr_ LPWSTR* pszPath, DWORD dwReserved);
#endif // _WIN32_IE_IE70

LWSTDAPI_(LPSTR)   PathAddBackslashA(_Inout_updates_(MAX_PATH) LPSTR);
LWSTDAPI_(LPWSTR)  PathAddBackslashW(_Inout_updates_(MAX_PATH) LPWSTR);
LWSTDAPI_(BOOL)    PathAddExtensionA(_Inout_updates_(MAX_PATH) LPSTR, _In_opt_ LPCSTR);
LWSTDAPI_(BOOL)    PathAddExtensionW(_Inout_updates_(MAX_PATH) LPWSTR,_In_opt_ LPCWSTR);
LWSTDAPI_(BOOL)    PathAppendA(_Inout_updates_(MAX_PATH) LPSTR, _In_ LPCSTR);
LWSTDAPI_(BOOL)    PathAppendW(_Inout_updates_(MAX_PATH) LPWSTR, _In_ LPCWSTR);
LWSTDAPI_(LPSTR)   PathBuildRootA(_Out_writes_(4) LPSTR, int);
LWSTDAPI_(LPWSTR)  PathBuildRootW(_Out_writes_(4) LPWSTR, int);
LWSTDAPI_(BOOL)    PathCanonicalizeA(_Out_writes_(MAX_PATH) LPSTR, _In_ LPCSTR);
LWSTDAPI_(BOOL)    PathCanonicalizeW(_Out_writes_(MAX_PATH) LPWSTR, _In_ LPCWSTR);
LWSTDAPI_(LPSTR)   PathCombineA(_Out_writes_(MAX_PATH) LPSTR, _In_opt_ LPCSTR, _In_opt_ LPCSTR);
LWSTDAPI_(LPWSTR)  PathCombineW(_Out_writes_(MAX_PATH) LPWSTR, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR);
LWSTDAPI_(BOOL)    PathCompactPathA(_In_opt_ HDC, _Inout_updates_(MAX_PATH) LPSTR, _In_ UINT);
LWSTDAPI_(BOOL)    PathCompactPathW(_In_opt_ HDC, _Inout_updates_(MAX_PATH) LPWSTR, _In_ UINT);
LWSTDAPI_(BOOL)    PathCompactPathExA(_Out_writes_(cchMax) LPSTR, _In_ LPCSTR, _In_ UINT cchMax, _In_ DWORD);
LWSTDAPI_(BOOL)    PathCompactPathExW(_Out_writes_(cchMax) LPWSTR, _In_ LPCWSTR, _In_ UINT cchMax, _In_ DWORD);
LWSTDAPI_(int)     PathCommonPrefixA(_In_ LPCSTR, _In_ LPCSTR, _Out_writes_opt_(MAX_PATH) LPSTR);
LWSTDAPI_(int)     PathCommonPrefixW(_In_ LPCWSTR, _In_ LPCWSTR, _Out_writes_opt_(MAX_PATH) LPWSTR);
LWSTDAPI           PathCreateFromUrlA(_In_ LPCSTR, _Out_writes_to_(*pcchPath, *pcchPath) LPSTR, _Inout_ LPDWORD pcchPath, DWORD);
LWSTDAPI           PathCreateFromUrlW(_In_ LPCWSTR pszUrl, _Out_writes_to_(*pcchPath, *pcchPath) LPWSTR pszPath, _Inout_ LPDWORD pcchPath, DWORD dwFlags);
LWSTDAPI_(BOOL)    PathFileExistsA(_In_ LPCSTR pszPath);
LWSTDAPI_(BOOL)    PathFileExistsW(_In_ LPCWSTR pszPath);
LWSTDAPI_(LPSTR)   PathFindExtensionA(_In_ LPCSTR pszPath);
LWSTDAPI_(LPWSTR)  PathFindExtensionW(_In_ LPCWSTR pszPath);
LWSTDAPI_(LPSTR)   PathFindFileNameA(_In_ LPCSTR pszPath);
LWSTDAPI_(LPWSTR)  PathFindFileNameW(_In_ LPCWSTR pszPath);
LWSTDAPI_(LPSTR)   PathFindNextComponentA(_In_ LPCSTR);
LWSTDAPI_(LPWSTR)  PathFindNextComponentW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathFindOnPathA(_Inout_updates_(MAX_PATH) LPSTR, _In_opt_ LPCSTR*);
LWSTDAPI_(BOOL)    PathFindOnPathW(_Inout_updates_(MAX_PATH) LPWSTR, _In_opt_ LPCWSTR*);
LWSTDAPI_(LPCSTR)  PathFindSuffixArrayA(_In_ LPCSTR, _In_reads_(iArraySize) LPCSTR *, int iArraySize);
LWSTDAPI_(LPCWSTR) PathFindSuffixArrayW(_In_ LPCWSTR, _In_reads_(iArraySize) LPCWSTR *, int iArraySize);
LWSTDAPI_(LPSTR)   PathGetArgsA(_In_ LPCSTR pszPath);
LWSTDAPI_(LPWSTR)  PathGetArgsW(_In_ LPCWSTR pszPath);
LWSTDAPI_(UINT)    PathGetCharTypeA(_In_ UCHAR ch);
LWSTDAPI_(UINT)    PathGetCharTypeW(_In_ WCHAR ch);
LWSTDAPI_(int)     PathGetDriveNumberA(_In_ LPCSTR);
LWSTDAPI_(int)     PathGetDriveNumberW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsContentTypeA(_In_ LPCSTR, _In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsContentTypeW(_In_ LPCWSTR, _In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsDirectoryA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsDirectoryW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsDirectoryEmptyA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsDirectoryEmptyW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsFileSpecA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsFileSpecW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsLFNFileSpecA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsLFNFileSpecW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsNetworkPathA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsNetworkPathW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsPrefixA(_In_ LPCSTR, _In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsPrefixW(_In_ LPCWSTR, _In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsRelativeA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsRelativeW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsRootA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsRootW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsSameRootA(_In_ LPCSTR, _In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsSameRootW(_In_ LPCWSTR, _In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsSystemFolderA(_In_opt_ LPCSTR, _In_ DWORD);
LWSTDAPI_(BOOL)    PathIsSystemFolderW(_In_opt_ LPCWSTR, _In_ DWORD);
LWSTDAPI_(BOOL)    PathIsUNCA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsUNCW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsUNCServerA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsUNCServerW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsUNCServerShareA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsUNCServerShareW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathIsURLA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathIsURLW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathMakePrettyA(_Inout_ LPSTR);
LWSTDAPI_(BOOL)    PathMakePrettyW(_Inout_ LPWSTR);
LWSTDAPI_(BOOL)    PathMakeSystemFolderA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathMakeSystemFolderW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathMatchSpecA(_In_ LPCSTR pszFile, _In_ LPCSTR pszSpec);
LWSTDAPI_(BOOL)    PathMatchSpecW(_In_ LPCWSTR pszFile, _In_ LPCWSTR pszSpec);
#if (_WIN32_IE >= _WIN32_IE_IE70)
LWSTDAPI           PathMatchSpecExA(_In_ LPCSTR, _In_ LPCSTR, _In_ DWORD);
LWSTDAPI           PathMatchSpecExW(_In_ LPCWSTR, _In_ LPCWSTR, _In_ DWORD);
#endif // _WIN32_IE_IE70
LWSTDAPI_(int)     PathParseIconLocationA(_Inout_ LPSTR);
LWSTDAPI_(int)     PathParseIconLocationW(_Inout_ LPWSTR);
LWSTDAPI_(BOOL)    PathQuoteSpacesA(_Inout_updates_(MAX_PATH) LPSTR);
LWSTDAPI_(BOOL)    PathQuoteSpacesW(_Inout_updates_(MAX_PATH) LPWSTR);
LWSTDAPI_(BOOL)    PathRelativePathToA(_Out_writes_(MAX_PATH) LPSTR, _In_ LPCSTR, _In_ DWORD, _In_ LPCSTR, _In_ DWORD);
LWSTDAPI_(BOOL)    PathRelativePathToW(_Out_writes_(MAX_PATH) LPWSTR, _In_ LPCWSTR, _In_ DWORD, _In_ LPCWSTR, _In_ DWORD);
LWSTDAPI_(void)    PathRemoveArgsA(_Inout_ LPSTR);
LWSTDAPI_(void)    PathRemoveArgsW(_Inout_ LPWSTR);
LWSTDAPI_(LPSTR)   PathRemoveBackslashA(_Inout_ LPSTR);
LWSTDAPI_(LPWSTR)  PathRemoveBackslashW(_Inout_ LPWSTR);
LWSTDAPI_(void)    PathRemoveBlanksA(_Inout_ LPSTR);
LWSTDAPI_(void)    PathRemoveBlanksW(_Inout_ LPWSTR);
LWSTDAPI_(void)    PathRemoveExtensionA(_Inout_ LPSTR);
LWSTDAPI_(void)    PathRemoveExtensionW(_Inout_ LPWSTR);
LWSTDAPI_(BOOL)    PathRemoveFileSpecA(_Inout_ LPSTR);
LWSTDAPI_(BOOL)    PathRemoveFileSpecW(_Inout_ LPWSTR);
LWSTDAPI_(BOOL)    PathRenameExtensionA(_Inout_updates_(MAX_PATH) LPSTR, _In_ LPCSTR);
LWSTDAPI_(BOOL)    PathRenameExtensionW(_Inout_updates_(MAX_PATH) LPWSTR, _In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathSearchAndQualifyA(_In_ LPCSTR, _Out_writes_(cchBuf) LPSTR, _In_ UINT cchBuf);
LWSTDAPI_(BOOL)    PathSearchAndQualifyW(_In_ LPCWSTR, _Out_writes_(cchBuf) LPWSTR, _In_ UINT cchBuf);
LWSTDAPI_(void)    PathSetDlgItemPathA(_In_ HWND, int, LPCSTR);
LWSTDAPI_(void)    PathSetDlgItemPathW(_In_ HWND, int, LPCWSTR);
LWSTDAPI_(LPSTR)   PathSkipRootA(_In_ LPCSTR);
LWSTDAPI_(LPWSTR)  PathSkipRootW(_In_ LPCWSTR);
LWSTDAPI_(void)    PathStripPathA(_Inout_ LPSTR);
LWSTDAPI_(void)    PathStripPathW(_Inout_ LPWSTR);
LWSTDAPI_(BOOL)    PathStripToRootA(_Inout_ LPSTR);
LWSTDAPI_(BOOL)    PathStripToRootW(_Inout_ LPWSTR);
LWSTDAPI_(void)    PathUndecorateA(_Inout_ LPSTR);
LWSTDAPI_(void)    PathUndecorateW(_Inout_ LPWSTR);
LWSTDAPI_(BOOL)    PathUnExpandEnvStringsA(_In_ LPCSTR, _Out_writes_(cchBuf) LPSTR, _In_ UINT cchBuf);
LWSTDAPI_(BOOL)    PathUnExpandEnvStringsW(_In_ LPCWSTR, _Out_writes_(cchBuf) LPWSTR, _In_ UINT cchBuf);
LWSTDAPI_(BOOL)    PathUnmakeSystemFolderA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    PathUnmakeSystemFolderW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    PathUnquoteSpacesA(_Inout_ LPSTR);
LWSTDAPI_(BOOL)    PathUnquoteSpacesW(_Inout_ LPWSTR);

#ifdef UNICODE
#define PathAddBackslash        PathAddBackslashW
#define PathAddExtension        PathAddExtensionW
#define PathAppend              PathAppendW
#define PathBuildRoot           PathBuildRootW
#define PathCanonicalize        PathCanonicalizeW
#define PathCombine             PathCombineW
#define PathCompactPath         PathCompactPathW
#define PathCompactPathEx       PathCompactPathExW
#define PathCommonPrefix        PathCommonPrefixW
#define PathCreateFromUrl       PathCreateFromUrlW
#define PathFileExists          PathFileExistsW
#define PathFindExtension       PathFindExtensionW
#define PathFindFileName        PathFindFileNameW
#define PathFindNextComponent   PathFindNextComponentW
#define PathFindOnPath          PathFindOnPathW
#define PathFindSuffixArray     PathFindSuffixArrayW
#define PathGetArgs             PathGetArgsW
#define PathGetCharType         PathGetCharTypeW
#define PathGetDriveNumber      PathGetDriveNumberW
#define PathIsContentType       PathIsContentTypeW
#define PathIsDirectory         PathIsDirectoryW
#define PathIsDirectoryEmpty    PathIsDirectoryEmptyW
#define PathIsFileSpec          PathIsFileSpecW
#define PathIsLFNFileSpec       PathIsLFNFileSpecW
#define PathIsNetworkPath       PathIsNetworkPathW
#define PathIsPrefix            PathIsPrefixW
#define PathIsRelative          PathIsRelativeW
#define PathIsRoot              PathIsRootW
#define PathIsSameRoot          PathIsSameRootW
#define PathIsSystemFolder      PathIsSystemFolderW
#define PathIsUNC               PathIsUNCW
#define PathIsUNCServer         PathIsUNCServerW
#define PathIsUNCServerShare    PathIsUNCServerShareW
#define PathIsURL               PathIsURLW
#define PathMakePretty          PathMakePrettyW
#define PathMakeSystemFolder    PathMakeSystemFolderW
#define PathMatchSpec           PathMatchSpecW
#if (_WIN32_IE >= _WIN32_IE_IE70)
#define PathMatchSpecEx         PathMatchSpecExW
#endif // _WIN32_IE_IE70
#define PathParseIconLocation   PathParseIconLocationW
#define PathQuoteSpaces         PathQuoteSpacesW
#define PathRelativePathTo      PathRelativePathToW
#define PathRemoveArgs          PathRemoveArgsW
#define PathRemoveBackslash     PathRemoveBackslashW
#define PathRemoveBlanks        PathRemoveBlanksW
#define PathRemoveExtension     PathRemoveExtensionW
#define PathRemoveFileSpec      PathRemoveFileSpecW
#define PathRenameExtension     PathRenameExtensionW
#define PathSearchAndQualify    PathSearchAndQualifyW
#define PathSetDlgItemPath      PathSetDlgItemPathW
#define PathSkipRoot            PathSkipRootW
#define PathStripPath           PathStripPathW
#define PathStripToRoot         PathStripToRootW
#define PathUndecorate          PathUndecorateW
#define PathUnExpandEnvStrings  PathUnExpandEnvStringsW
#define PathUnmakeSystemFolder  PathUnmakeSystemFolderW
#define PathUnquoteSpaces       PathUnquoteSpacesW
#else
#define PathAddBackslash        PathAddBackslashA
#define PathAddExtension        PathAddExtensionA
#define PathAppend              PathAppendA
#define PathBuildRoot           PathBuildRootA
#define PathCanonicalize        PathCanonicalizeA
#define PathCombine             PathCombineA
#define PathCompactPath         PathCompactPathA
#define PathCompactPathEx       PathCompactPathExA
#define PathCommonPrefix        PathCommonPrefixA
#define PathCreateFromUrl       PathCreateFromUrlA
#define PathFileExists          PathFileExistsA
#define PathFindExtension       PathFindExtensionA
#define PathFindFileName        PathFindFileNameA
#define PathFindNextComponent   PathFindNextComponentA
#define PathFindOnPath          PathFindOnPathA
#define PathFindSuffixArray     PathFindSuffixArrayA
#define PathGetArgs             PathGetArgsA
#define PathGetCharType         PathGetCharTypeA
#define PathGetDriveNumber      PathGetDriveNumberA
#define PathIsContentType       PathIsContentTypeA
#define PathIsDirectory         PathIsDirectoryA
#define PathIsDirectoryEmpty    PathIsDirectoryEmptyA
#define PathIsFileSpec          PathIsFileSpecA
#define PathIsLFNFileSpec       PathIsLFNFileSpecA
#define PathIsNetworkPath       PathIsNetworkPathA
#define PathIsPrefix            PathIsPrefixA
#define PathIsRelative          PathIsRelativeA
#define PathIsRoot              PathIsRootA
#define PathIsSameRoot          PathIsSameRootA
#define PathIsSystemFolder      PathIsSystemFolderA
#define PathIsUNC               PathIsUNCA
#define PathIsUNCServer         PathIsUNCServerA
#define PathIsUNCServerShare    PathIsUNCServerShareA
#define PathIsURL               PathIsURLA
#define PathMakePretty          PathMakePrettyA
#define PathMakeSystemFolder    PathMakeSystemFolderA
#define PathMatchSpec           PathMatchSpecA
#if (_WIN32_IE >= _WIN32_IE_IE70)
#define PathMatchSpecEx         PathMatchSpecExA
#endif // _WIN32_IE_IE70
#define PathParseIconLocation   PathParseIconLocationA
#define PathQuoteSpaces         PathQuoteSpacesA
#define PathRelativePathTo      PathRelativePathToA
#define PathRemoveArgs          PathRemoveArgsA
#define PathRemoveBackslash     PathRemoveBackslashA
#define PathRemoveBlanks        PathRemoveBlanksA
#define PathRemoveExtension     PathRemoveExtensionA
#define PathRemoveFileSpec      PathRemoveFileSpecA
#define PathRenameExtension     PathRenameExtensionA
#define PathSearchAndQualify    PathSearchAndQualifyA
#define PathSetDlgItemPath      PathSetDlgItemPathA
#define PathSkipRoot            PathSkipRootA
#define PathStripPath           PathStripPathA
#define PathStripToRoot         PathStripToRootA
#define PathUndecorate          PathUndecorateA
#define PathUnExpandEnvStrings  PathUnExpandEnvStringsA
#define PathUnmakeSystemFolder  PathUnmakeSystemFolderA
#define PathUnquoteSpaces       PathUnquoteSpacesA
#endif // UNICODE

#define URL_APPLY_DEFAULT            0x00000001
#define URL_APPLY_GUESSSCHEME        0x00000002
#define URL_APPLY_GUESSFILE          0x00000004
#define URL_APPLY_FORCEAPPLY         0x00000008
#define URL_ESCAPE_PERCENT           0x00001000
#define URL_ESCAPE_SEGMENT_ONLY      0x00002000
#define URL_FILE_USE_PATHURL         0x00010000
#define URL_ESCAPE_AS_UTF8           0x00040000
#define URL_UNESCAPE_INPLACE         0x00100000
#define URL_CONVERT_IF_DOSPATH       0x00200000
#define URL_UNESCAPE_HIGH_ANSI_ONLY  0x00400000
#define URL_INTERNAL_PATH            0x00800000
#define URL_DONT_ESCAPE_EXTRA_INFO   0x02000000
#define URL_ESCAPE_SPACES_ONLY       0x04000000
#define URL_DONT_SIMPLIFY            0x08000000
#define URL_UNESCAPE                 0x10000000
#define URL_ESCAPE_UNSAFE            0x20000000
#define URL_PLUGGABLE_PROTOCOL       0x40000000
#define URL_WININET_COMPATIBILITY    0x80000000
#define URL_UNESCAPE_AS_UTF8         URL_ESCAPE_AS_UTF8
#define URL_UNESCAPE_URI_COMPONENT   URL_UNESCAPE_AS_UTF8
#define URL_NO_META                  URL_DONT_SIMPLIFY
#define URL_DONT_UNESCAPE_EXTRA_INFO URL_DONT_ESCAPE_EXTRA_INFO
#define URL_BROWSER_MODE             URL_DONT_ESCAPE_EXTRA_INFO

typedef enum {
    URL_SCHEME_INVALID     = -1,
    URL_SCHEME_UNKNOWN     =  0,
    URL_SCHEME_FTP,
    URL_SCHEME_HTTP,
    URL_SCHEME_GOPHER,
    URL_SCHEME_MAILTO,
    URL_SCHEME_NEWS,
    URL_SCHEME_NNTP,
    URL_SCHEME_TELNET,
    URL_SCHEME_WAIS,
    URL_SCHEME_FILE,
    URL_SCHEME_MK,
    URL_SCHEME_HTTPS,
    URL_SCHEME_SHELL,
    URL_SCHEME_SNEWS,
    URL_SCHEME_LOCAL,
    URL_SCHEME_JAVASCRIPT,
    URL_SCHEME_VBSCRIPT,
    URL_SCHEME_ABOUT,
    URL_SCHEME_RES,
#if (_WIN32_IE >= _WIN32_IE_IE60)
    URL_SCHEME_MSSHELLROOTED,
    URL_SCHEME_MSSHELLIDLIST,
    URL_SCHEME_MSHELP,
#endif // _WIN32_IE_IE60
#if (_WIN32_IE >= _WIN32_IE_IE70)
    URL_SCHEME_MSSHELLDEVICE,
    URL_SCHEME_WILDCARD,
#endif // _WIN32_IE_IE70
#if (NTDDI_VERSION >= NTDDI_VISTA)
    URL_SCHEME_SEARCH_MS,
#endif // NTDDI_VISTA
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
    URL_SCHEME_SEARCH,
#endif // NTDDI_VISTASP1
#if (NTDDI_VERSION >= NTDDI_WIN7)
    URL_SCHEME_KNOWNFOLDER,
#endif // NTDDI_WIN7
    URL_SCHEME_MAXVALUE
} URL_SCHEME;

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

typedef enum {
    URLIS_URL,
    URLIS_OPAQUE,
    URLIS_NOHISTORY,
    URLIS_FILEURL,
    URLIS_APPLIABLE,
    URLIS_DIRECTORY,
    URLIS_HASQUERY
} URLIS;

typedef struct tagPARSEDURLA {
    DWORD cbSize;
    LPCSTR pszProtocol;
    UINT cchProtocol;
    LPCSTR pszSuffix;
    UINT cchSuffix;
    UINT nScheme;
} PARSEDURLA, *PPARSEDURLA;

typedef struct tagPARSEDURLW {
    DWORD cbSize;
    LPCWSTR pszProtocol;
    UINT cchProtocol;
    LPCWSTR pszSuffix;
    UINT cchSuffix;
    UINT nScheme;
} PARSEDURLW, *PPARSEDURLW;

LWSTDAPI           UrlApplySchemeA(_In_ LPCSTR, _Out_writes_(*pcchOut) LPSTR, _Inout_ LPDWORD pcchOut, DWORD);
LWSTDAPI           UrlApplySchemeW(_In_ LPCWSTR, _Out_writes_(*pcchOut) LPWSTR, _Inout_ LPDWORD pcchOut, DWORD);
LWSTDAPI           UrlCanonicalizeA(_In_ LPCSTR, _Out_writes_to_(*pcchCanonicalized, *pcchCanonicalized) LPSTR, _Inout_ LPDWORD pcchCanonicalized, DWORD);
LWSTDAPI           UrlCanonicalizeW(_In_ LPCWSTR, _Out_writes_to_(*pcchCanonicalized, *pcchCanonicalized) LPWSTR, _Inout_ LPDWORD pcchCanonicalized, DWORD);
LWSTDAPI           UrlCombineA(_In_ LPCSTR, _In_ LPCSTR, _Out_writes_to_opt_(*pcchCombined, *pcchCombined) LPSTR, _Inout_ LPDWORD pcchCombined, DWORD);
LWSTDAPI           UrlCombineW(_In_ LPCWSTR, _In_ LPCWSTR, _Out_writes_to_opt_(*pcchCombined, *pcchCombined) LPWSTR, _Inout_ LPDWORD pcchCombined, DWORD);
LWSTDAPI_(int)     UrlCompareA(_In_ LPCSTR, _In_ LPCSTR, BOOL);
LWSTDAPI_(int)     UrlCompareW(_In_ LPCWSTR, _In_ LPCWSTR, BOOL);
LWSTDAPI           UrlEscapeA(_In_ LPCSTR, _Out_writes_to_(*pcchEscaped, *pcchEscaped) LPSTR, _Inout_ LPDWORD pcchEscaped, DWORD);
LWSTDAPI           UrlEscapeW(_In_ LPCWSTR, _Out_writes_to_(*pcchEscaped, *pcchEscaped) LPWSTR, _Inout_ LPDWORD pcchEscaped, DWORD);
LWSTDAPI_(LPCSTR)  UrlGetLocationA(_In_ LPCSTR);
LWSTDAPI_(LPCWSTR) UrlGetLocationW(_In_ LPCWSTR);
LWSTDAPI           UrlGetPartA(_In_ LPCSTR, _Out_writes_(*pcchOut) LPSTR, _Inout_ LPDWORD pcchOut, DWORD, DWORD);
LWSTDAPI           UrlGetPartW(_In_ LPCWSTR, _Out_writes_(*pcchOut) LPWSTR, _Inout_ LPDWORD pcchOut, DWORD, DWORD);
LWSTDAPI           UrlHashA(_In_ LPCSTR, _Out_writes_bytes_(cbHash) unsigned char *, DWORD cbHash);
LWSTDAPI           UrlHashW(_In_ LPCWSTR, _Out_writes_bytes_(cbHash) unsigned char *, DWORD cbHash);
LWSTDAPI_(BOOL)    UrlIsA(_In_ LPCSTR, URLIS);
LWSTDAPI_(BOOL)    UrlIsW(_In_ LPCWSTR, URLIS);
#define            UrlIsFileUrlA(x) UrlIsA(x, URLIS_FILEURL)
#define            UrlIsFileUrlW(x) UrlIsW(x, URLIS_FILEURL)
LWSTDAPI_(BOOL)    UrlIsNoHistoryA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    UrlIsNoHistoryW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    UrlIsOpaqueA(_In_ LPCSTR);
LWSTDAPI_(BOOL)    UrlIsOpaqueW(_In_ LPCWSTR);
LWSTDAPI           UrlUnescapeA(_Inout_ LPSTR, _Out_writes_to_opt_(*pcchUnescaped, *pcchUnescaped) LPSTR, _Inout_opt_ LPDWORD pcchUnescaped, DWORD);
LWSTDAPI           UrlUnescapeW(_Inout_ LPWSTR, _Out_writes_to_opt_(*pcchUnescaped, *pcchUnescaped) LPWSTR, _Inout_opt_ LPDWORD pcchUnescaped, DWORD);
LWSTDAPI           UrlCreateFromPathA(_In_ LPCSTR, _Out_writes_to_(*pcchUrl, *pcchUrl) LPSTR, _Inout_ LPDWORD pcchUrl, DWORD);
LWSTDAPI           UrlCreateFromPathW(_In_ LPCWSTR, _Out_writes_to_(*pcchUrl, *pcchUrl) LPWSTR, _Inout_ LPDWORD pcchUrl, DWORD);
LWSTDAPI           ParseURLA(_In_ LPCSTR pszUrl, _Inout_ PARSEDURLA *ppu);
LWSTDAPI           ParseURLW(_In_ LPCWSTR pszUrl, _Inout_ PARSEDURLW *ppu);

#ifdef UNICODE
#define UrlApplyScheme     UrlApplySchemeW
#define UrlCanonicalize    UrlCanonicalizeW
#define UrlCombine         UrlCombineW
#define UrlCompare         UrlCompareW
#define UrlEscape          UrlEscapeW
#define UrlGetLocation     UrlGetLocationW
#define UrlGetPart         UrlGetPartW
#define UrlHash            UrlHashW
#define UrlIs              UrlIsW
#define UrlIsFileUrl       UrlIsFileUrlW
#define UrlIsNoHistory     UrlIsNoHistoryW
#define UrlIsOpaque        UrlIsOpaqueW
#define UrlUnescape        UrlUnescapeW
#define UrlCreateFromPath  UrlCreateFromPathW
#define ParseURL           ParseURLW
#else
#define UrlApplyScheme     UrlApplySchemeA
#define UrlCanonicalize    UrlCanonicalizeA
#define UrlCombine         UrlCombineA
#define UrlCompare         UrlCompareA
#define UrlEscape          UrlEscapeA
#define UrlGetLocation     UrlGetLocationA
#define UrlGetPart         UrlGetPartA
#define UrlHash            UrlHashA
#define UrlIs              UrlIsA
#define UrlIsFileUrl       UrlIsFileUrlA
#define UrlIsNoHistory     UrlIsNoHistoryA
#define UrlIsOpaque        UrlIsOpaqueA
#define UrlUnescape        UrlUnescapeA
#define UrlCreateFromPath  UrlCreateFromPathA
#define ParseURL           ParseURLA
#endif // UNICODE

#define UrlEscapeSpaces(x, y, z) UrlCanonicalize(x, y, z, URL_ESCAPE_SPACES_ONLY | URL_DONT_ESCAPE_EXTRA_INFO)
#define UrlUnescapeInPlace(x, y) UrlUnescape(x, NULL, NULL, y | URL_UNESCAPE_INPLACE)

#endif // NO_SHLWAPI_PATH

/* Registry functions */
#ifndef NO_SHLWAPI_REG

#define SRRF_RT_REG_NONE       0x0001
#define SRRF_RT_REG_SZ         0x0002
#define SRRF_RT_REG_EXPAND_SZ  0x0004
#define SRRF_RT_REG_BINARY     0x0008
#define SRRF_RT_REG_DWORD      0x0010
#define SRRF_RT_REG_MULTI_SZ   0x0020
#define SRRF_RT_REG_QWORD      0x0040
#define SRRF_RT_ANY            0xFFFF
#define SRRF_RT_DWORD          (SRRF_RT_REG_BINARY | SRRF_RT_REG_DWORD)
#define SRRF_RT_QWORD          (SRRF_RT_REG_BINARY | SRRF_RT_REG_QWORD)

#define SRRF_RM_ANY            0x00000
#define SRRF_RM_NORMAL         0x10000
#define SRRF_RM_SAFE           0x20000
#define SRRF_RM_SAFENETWORK    0x40000

#define SRRF_NOEXPAND          0x10000000
#define SRRF_ZEROONFAILURE     0x20000000
#define SRRF_NOVIRT            0x40000000

typedef INT SRRF;

#define SHREGSET_HKCU        0x1
#define SHREGSET_FORCE_HKCU  0x2
#define SHREGSET_HKLM        0x4
#define SHREGSET_FORCE_HKLM  0x8
#define SHREGSET_DEFAULT     (SHREGSET_FORCE_HKCU | SHREGSET_HKLM)

typedef enum {
  SHREGDEL_DEFAULT = 0,
  SHREGDEL_HKCU    = 0x1,
  SHREGDEL_HKLM    = 0x10,
  SHREGDEL_BOTH    = SHREGDEL_HKLM | SHREGDEL_HKCU
} SHREGDEL_FLAGS;

typedef enum {
  SHREGENUM_DEFAULT  = 0,
  SHREGENUM_HKCU     = 0x1,
  SHREGENUM_HKLM     = 0x10,
  SHREGENUM_BOTH     = SHREGENUM_HKLM | SHREGENUM_HKCU
} SHREGENUM_FLAGS;

typedef HANDLE HUSKEY;
typedef HUSKEY *PHUSKEY;

LWSTDAPI_(LONG)    SHRegCloseUSKey(_In_ HUSKEY);
LWSTDAPI_(HKEY)    SHRegDuplicateHKey(_In_ HKEY);

LWSTDAPI_(DWORD)   SHCopyKeyA(_In_ HKEY, _In_opt_ LPCSTR, _In_ HKEY, _Reserved_ DWORD);
LWSTDAPI_(DWORD)   SHCopyKeyW(_In_ HKEY, _In_opt_ LPCWSTR, _In_ HKEY, _Reserved_ DWORD);
LWSTDAPI_(DWORD)   SHDeleteEmptyKeyA(_In_ HKEY, _In_opt_ LPCSTR);
LWSTDAPI_(DWORD)   SHDeleteEmptyKeyW(_In_ HKEY, _In_opt_ LPCWSTR);
LWSTDAPI_(DWORD)   SHDeleteKeyA(_In_ HKEY, _In_opt_ LPCSTR);
LWSTDAPI_(DWORD)   SHDeleteKeyW(_In_ HKEY, _In_opt_ LPCWSTR);
LWSTDAPI_(DWORD)   SHDeleteOrphanKeyA(HKEY, LPCSTR);
LWSTDAPI_(DWORD)   SHDeleteOrphanKeyW(HKEY, LPCWSTR);
LWSTDAPI_(DWORD)   SHDeleteValueA(_In_ HKEY, _In_opt_ LPCSTR, _In_ LPCSTR);
LWSTDAPI_(DWORD)   SHDeleteValueW(_In_ HKEY, _In_opt_ LPCWSTR, _In_ LPCWSTR);
LWSTDAPI_(LONG)    SHEnumKeyExA(_In_ HKEY, _In_ DWORD, _Out_writes_(*pcchName) LPSTR, _Inout_ LPDWORD pcchName);
LWSTDAPI_(LONG)    SHEnumKeyExW(_In_ HKEY, _In_ DWORD, _Out_writes_(*pcchName) LPWSTR, _Inout_ LPDWORD pcchName);
LWSTDAPI_(LONG)    SHEnumValueA(_In_ HKEY, _In_ DWORD, _Out_writes_opt_(*pcchValueName) LPSTR, _Inout_opt_ LPDWORD pcchValueName, _Out_opt_ LPDWORD, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData);
LWSTDAPI_(LONG)    SHEnumValueW(_In_ HKEY, _In_ DWORD, _Out_writes_opt_(*pcchValueName) LPWSTR, _Inout_opt_ LPDWORD pcchValueName, _Out_opt_ LPDWORD, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData);
LWSTDAPI_(DWORD)   SHGetValueA(_In_ HKEY, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _Out_opt_ LPDWORD, _Out_writes_bytes_opt_(*pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData);
LWSTDAPI_(DWORD)   SHGetValueW(_In_ HKEY, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _Out_opt_ LPDWORD, _Out_writes_bytes_opt_(*pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData);
LWSTDAPI_(DWORD)   SHSetValueA(_In_ HKEY, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _In_ DWORD, _In_reads_bytes_opt_(cbData) LPCVOID, _In_ DWORD cbData);
LWSTDAPI_(DWORD)   SHSetValueW(_In_ HKEY, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _In_ DWORD, _In_reads_bytes_opt_(cbData) LPCVOID, _In_ DWORD cbData);
LWSTDAPI_(DWORD)   SHQueryValueExA(_In_ HKEY, _In_opt_ LPCSTR, _Reserved_ LPDWORD, _Out_opt_ LPDWORD, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData);
LWSTDAPI_(DWORD)   SHQueryValueExW(_In_ HKEY, _In_opt_ LPCWSTR, _Reserved_ LPDWORD, _Out_opt_ LPDWORD, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData);
LWSTDAPI_(LONG)    SHQueryInfoKeyA(_In_ HKEY, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD);
LWSTDAPI_(LONG)    SHQueryInfoKeyW(_In_ HKEY, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD);
LWSTDAPI_(LONG)    SHRegCreateUSKeyA(_In_ LPCSTR, _In_ REGSAM, _In_opt_ HUSKEY, _Out_ PHUSKEY, _In_ DWORD);
LWSTDAPI_(LONG)    SHRegCreateUSKeyW(_In_ LPCWSTR, _In_ REGSAM, _In_opt_ HUSKEY, _Out_ PHUSKEY, _In_ DWORD);
LWSTDAPI_(LONG)    SHRegDeleteEmptyUSKeyA(_In_ HUSKEY, _In_ LPCSTR, _In_ SHREGDEL_FLAGS);
LWSTDAPI_(LONG)    SHRegDeleteEmptyUSKeyW(_In_ HUSKEY, _In_ LPCWSTR, _In_ SHREGDEL_FLAGS);
LWSTDAPI_(LONG)    SHRegDeleteUSValueA(_In_ HUSKEY, _In_ LPCSTR, _In_ SHREGDEL_FLAGS);
LWSTDAPI_(LONG)    SHRegDeleteUSValueW(_In_ HUSKEY, _In_ LPCWSTR, _In_ SHREGDEL_FLAGS);
LWSTDAPI_(LONG)    SHRegEnumUSKeyA(_In_ HUSKEY, _In_ DWORD, _Out_writes_to_(*pcchName, *pcchName) LPSTR, _Inout_ LPDWORD pcchName, _In_ SHREGENUM_FLAGS);
LWSTDAPI_(LONG)    SHRegEnumUSKeyW(_In_ HUSKEY, _In_ DWORD, _Out_writes_to_(*pcchName, *pcchName) LPWSTR, _Inout_ LPDWORD pcchName, _In_ SHREGENUM_FLAGS);
LWSTDAPI_(LONG)    SHRegEnumUSValueA(_In_ HUSKEY, _In_ DWORD, _Out_writes_to_(*pcchValueName, *pcchValueName) LPSTR, _Inout_ LPDWORD pcchValueName, _Out_opt_ LPDWORD, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData, _In_ SHREGENUM_FLAGS);
LWSTDAPI_(LONG)    SHRegEnumUSValueW(_In_ HUSKEY, _In_ DWORD, _Out_writes_to_(*pcchValueName, *pcchValueName) LPWSTR, _Inout_ LPDWORD pcchValueName, _Out_opt_ LPDWORD, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData, _In_ SHREGENUM_FLAGS);
LWSTDAPI_(BOOL)    SHRegGetBoolUSValueA(_In_ LPCSTR, _In_opt_ LPCSTR, _In_ BOOL, _In_ BOOL);
LWSTDAPI_(BOOL)    SHRegGetBoolUSValueW(_In_ LPCWSTR, _In_opt_ LPCWSTR, _In_ BOOL, _In_ BOOL);
LWSTDAPI_(DWORD)   SHRegGetPathA(_In_ HKEY, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _Out_writes_(MAX_PATH) LPSTR, _In_ DWORD);
LWSTDAPI_(DWORD)   SHRegGetPathW(_In_ HKEY, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _Out_writes_(MAX_PATH) LPWSTR, _In_ DWORD);
LWSTDAPI_(LONG)    SHRegGetUSValueA(_In_ LPCSTR, _In_opt_ LPCSTR, _Inout_opt_ LPDWORD, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData, _In_ BOOL, _In_reads_bytes_opt_(dwDefaultDataSize) LPVOID, _In_ DWORD dwDefaultDataSize);
LWSTDAPI_(LONG)    SHRegGetUSValueW(_In_ LPCWSTR, _In_opt_ LPCWSTR, _Inout_opt_ LPDWORD, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData, _In_ BOOL, _In_reads_bytes_opt_(dwDefaultDataSize) LPVOID, _In_ DWORD dwDefaultDataSize);
LWSTDAPI_(LSTATUS) SHRegGetValueA(_In_ HKEY hkey, _In_opt_ LPCSTR pszSubKey, _In_opt_ LPCSTR pszValue, _In_ SRRF srrfFlags, _Out_opt_ LPDWORD pdwType, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID pvData, _Inout_opt_ LPDWORD pcbData);
LWSTDAPI_(LSTATUS) SHRegGetValueW(_In_ HKEY hkey, _In_opt_ LPCWSTR pszSubKey, _In_opt_ LPCWSTR pszValue, _In_ SRRF srrfFlags, _Out_opt_ LPDWORD pdwType, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID pvData, _Inout_opt_ LPDWORD pcbData);
LWSTDAPI_(LONG)    SHRegOpenUSKeyA(_In_ LPCSTR, _In_ REGSAM, _In_opt_ HUSKEY, _Out_ PHUSKEY, _In_ BOOL);
LWSTDAPI_(LONG)    SHRegOpenUSKeyW(_In_ LPCWSTR, _In_ REGSAM, _In_opt_ HUSKEY, _Out_ PHUSKEY, _In_ BOOL);
LWSTDAPI_(LONG)    SHRegQueryInfoUSKeyA(_In_ HUSKEY, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD, _In_ SHREGENUM_FLAGS);
LWSTDAPI_(LONG)    SHRegQueryInfoUSKeyW(_In_ HUSKEY, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD, _Out_opt_ LPDWORD, _In_ SHREGENUM_FLAGS);
LWSTDAPI_(LONG)    SHRegQueryUSValueA(_In_ HUSKEY, _In_opt_ LPCSTR, _Inout_opt_ LPDWORD, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData, _In_ BOOL, _In_reads_bytes_opt_(dwDefaultDataSize) LPVOID, _In_opt_ DWORD dwDefaultDataSize);
LWSTDAPI_(LONG)    SHRegQueryUSValueW(_In_ HUSKEY, _In_opt_ LPCWSTR, _Inout_opt_ LPDWORD, _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID, _Inout_opt_ LPDWORD pcbData, _In_ BOOL, _In_reads_bytes_opt_(dwDefaultDataSize) LPVOID, _In_opt_ DWORD dwDefaultDataSize);
LWSTDAPI_(DWORD)   SHRegSetPathA(_In_ HKEY, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _In_ LPCSTR, _In_ DWORD);
LWSTDAPI_(DWORD)   SHRegSetPathW(_In_ HKEY, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _In_ LPCWSTR, _In_ DWORD);
LWSTDAPI_(LONG)    SHRegSetUSValueA(_In_ LPCSTR, _In_ LPCSTR, _In_ DWORD, _In_reads_bytes_opt_(cbData) LPVOID, _In_opt_ DWORD cbData, _In_opt_ DWORD);
LWSTDAPI_(LONG)    SHRegSetUSValueW(_In_ LPCWSTR, _In_ LPCWSTR, _In_ DWORD, _In_reads_bytes_opt_(cbData) LPVOID, _In_opt_ DWORD cbData, _In_opt_ DWORD);
LWSTDAPI_(LONG)    SHRegWriteUSValueA(_In_ HUSKEY, _In_ LPCSTR, _In_ DWORD, _In_reads_bytes_(cbData) LPVOID, _In_ DWORD cbData, _In_ DWORD);
LWSTDAPI_(LONG)    SHRegWriteUSValueW(_In_ HUSKEY, _In_ LPCWSTR, _In_ DWORD, _In_reads_bytes_(cbData) LPVOID, _In_ DWORD cbData, _In_ DWORD);
/* This only exists for the -W variant. */
LWSTDAPI_(int)     SHRegGetIntW(_In_ HKEY, _In_opt_ LPCWSTR, _In_ int);

#ifdef UNICODE
#define SHCopyKey              SHCopyKeyW
#define SHDeleteEmptyKey       SHDeleteEmptyKeyW
#define SHDeleteKey            SHDeleteKeyW
#define SHDeleteOrphanKey      SHDeleteOrphanKeyW
#define SHDeleteValue          SHDeleteValueW
#define SHEnumKeyEx            SHEnumKeyExW
#define SHEnumValue            SHEnumValueW
#define SHGetValue             SHGetValueW
#define SHSetValue             SHSetValueW
#define SHQueryValueEx         SHQueryValueExW
#define SHQueryInfoKey         SHQueryInfoKeyW
#define SHRegCreateUSKey       SHRegCreateUSKeyW
#define SHRegDeleteEmptyUSKey  SHRegDeleteEmptyUSKeyW
#define SHRegDeleteUSValue     SHRegDeleteUSValueW
#define SHRegEnumUSKey         SHRegEnumUSKeyW
#define SHRegEnumUSValue       SHRegEnumUSValueW
#define SHRegGetBoolUSValue    SHRegGetBoolUSValueW
#define SHRegGetPath           SHRegGetPathW
#define SHRegGetUSValue        SHRegGetUSValueW
#define SHRegGetValue          SHRegGetValueW
#define SHRegOpenUSKey         SHRegOpenUSKeyW
#define SHRegQueryInfoUSKey    SHRegQueryInfoUSKeyW
#define SHRegQueryUSValue      SHRegQueryUSValueW
#define SHRegSetPath           SHRegSetPathW
#define SHRegSetUSValue        SHRegSetUSValueW
#define SHRegWriteUSValue      SHRegWriteUSValueW
#define SHRegGetInt            SHRegGetIntW
#else
#define SHCopyKey              SHCopyKeyA
#define SHDeleteEmptyKey       SHDeleteEmptyKeyA
#define SHDeleteKey            SHDeleteKeyA
#define SHDeleteOrphanKey      SHDeleteOrphanKeyA
#define SHDeleteValue          SHDeleteValueA
#define SHEnumKeyEx            SHEnumKeyExA
#define SHEnumValue            SHEnumValueA
#define SHGetValue             SHGetValueA
#define SHSetValue             SHSetValueA
#define SHQueryValueEx         SHQueryValueExA
#define SHQueryInfoKey         SHQueryInfoKeyA
#define SHRegCreateUSKey       SHRegCreateUSKeyA
#define SHRegDeleteEmptyUSKey  SHRegDeleteEmptyUSKeyA
#define SHRegDeleteUSValue     SHRegDeleteUSValueA
#define SHRegEnumUSKey         SHRegEnumUSKeyA
#define SHRegEnumUSValue       SHRegEnumUSValueA
#define SHRegGetBoolUSValue    SHRegGetBoolUSValueA
#define SHRegGetPath           SHRegGetPathA
#define SHRegGetUSValue        SHRegGetUSValueA
#define SHRegGetValue          SHRegGetValueA
#define SHRegOpenUSKey         SHRegOpenUSKeyA
#define SHRegQueryInfoUSKey    SHRegQueryInfoUSKeyA
#define SHRegQueryUSValue      SHRegQueryUSValueA
#define SHRegSetPath           SHRegSetPathA
#define SHRegSetUSValue        SHRegSetUSValueA
#define SHRegWriteUSValue      SHRegWriteUSValueA
#endif // UNICODE

enum {
    ASSOCF_NONE                  = 0x00000000,
    ASSOCF_INIT_NOREMAPCLSID     = 0x00000001,
    ASSOCF_INIT_BYEXENAME        = 0x00000002,
    ASSOCF_OPEN_BYEXENAME        = 0x00000002,
    ASSOCF_INIT_DEFAULTTOSTAR    = 0x00000004,
    ASSOCF_INIT_DEFAULTTOFOLDER  = 0x00000008,
    ASSOCF_NOUSERSETTINGS        = 0x00000010,
    ASSOCF_NOTRUNCATE            = 0x00000020,
    ASSOCF_VERIFY                = 0x00000040,
    ASSOCF_REMAPRUNDLL           = 0x00000080,
    ASSOCF_NOFIXUPS              = 0x00000100,
    ASSOCF_IGNOREBASECLASS       = 0x00000200,
    ASSOCF_INIT_IGNOREUNKNOWN    = 0x00000400,
#if (NTDDI_VERSION >= NTDDI_WIN8)
    ASSOCF_INIT_FIXED_PROGID     = 0x00000800,
    ASSOCF_IS_PROTOCOL           = 0x00001000,
    ASSOCF_INIT_FOR_FILE         = 0x00002000,
#endif // NTDDI_WIN8
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
    ASSOCF_IS_FULL_URI           = 0x00004000,
    ASSOCF_PER_MACHINE_ONLY      = 0x00008000,
#endif // NTDDI_WIN10_RS1
#if (NTDDI_VERSION >= NTDDI_WIN10_RS4)
ASSOCF_APP_TO_APP                = 0x00010000,
#endif // NTDDI_WIN10_RS4
};

typedef enum {
    ASSOCSTR_COMMAND = 1,
    ASSOCSTR_EXECUTABLE,
    ASSOCSTR_FRIENDLYDOCNAME,
    ASSOCSTR_FRIENDLYAPPNAME,
    ASSOCSTR_NOOPEN,
    ASSOCSTR_SHELLNEWVALUE,
    ASSOCSTR_DDECOMMAND,
    ASSOCSTR_DDEIFEXEC,
    ASSOCSTR_DDEAPPLICATION,
    ASSOCSTR_DDETOPIC,
    ASSOCSTR_INFOTIP,
#if (_WIN32_IE >= _WIN32_IE_IE60)
    ASSOCSTR_QUICKTIP,
    ASSOCSTR_TILEINFO,
    ASSOCSTR_CONTENTTYPE,
    ASSOCSTR_DEFAULTICON,
    ASSOCSTR_SHELLEXTENSION,
#endif // _WIN32_IE_IE60
#if (_WIN32_IE >= _WIN32_IE_IE80)
    ASSOCSTR_DROPTARGET,
    ASSOCSTR_DELEGATEEXECUTE,
#endif // _WIN32_IE_IE80
    ASSOCSTR_SUPPORTED_URI_PROTOCOLS,
#if (NTDDI_VERSION >= NTDDI_WIN10)
    ASSOCSTR_PROGID,
    ASSOCSTR_APPID,
    ASSOCSTR_APPPUBLISHER,
    ASSOCSTR_APPICONREFERENCE,
#endif // NTDDI_WIN10
    ASSOCSTR_MAX
} ASSOCSTR;

typedef enum {
    ASSOCKEY_SHELLEXECCLASS = 1,
    ASSOCKEY_APP,
    ASSOCKEY_CLASS,
    ASSOCKEY_BASECLASS,
    ASSOCKEY_MAX
} ASSOCKEY;

typedef enum {
    ASSOCDATA_MSIDESCRIPTOR = 1,
    ASSOCDATA_NOACTIVATEHANDLER,
    ASSOCDATA_QUERYCLASSSTORE,
    ASSOCDATA_HASPERUSERASSOC,
#if (_WIN32_IE >= _WIN32_IE_IE60)
    ASSOCDATA_EDITFLAGS,
    ASSOCDATA_VALUE,
#endif // _WIN32_IE_IE60
    ASSOCDATA_MAX
} ASSOCDATA;

typedef enum {
    ASSOCENUM_NONE
} ASSOCENUM;

typedef enum {
    FTA_None                  = 0x00000000,
    FTA_Exclude               = 0x00000001,
    FTA_Show                  = 0x00000002,
    FTA_HasExtension          = 0x00000004,
    FTA_NoEdit                = 0x00000008,
    FTA_NoRemove              = 0x00000010,
    FTA_NoNewVerb             = 0x00000020,
    FTA_NoEditVerb            = 0x00000040,
    FTA_NoRemoveVerb          = 0x00000080,
    FTA_NoEditDesc            = 0x00000100,
    FTA_NoEditIcon            = 0x00000200,
    FTA_NoEditDflt            = 0x00000400,
    FTA_NoEditVerbCmd         = 0x00000800,
    FTA_NoEditVerbExe         = 0x00001000,
    FTA_NoDDE                 = 0x00002000,
    FTA_NoEditMIME            = 0x00008000,
    FTA_OpenIsSafe            = 0x00010000,
    FTA_AlwaysUnsafe          = 0x00020000,
    FTA_NoRecentDocs          = 0x00100000,
    FTA_SafeForElevation      = 0x00200000,
    FTA_AlwaysUseDirectInvoke = 0x00400000
} FILETYPEATTRIBUTEFLAGS;
DEFINE_ENUM_FLAG_OPERATORS(FILETYPEATTRIBUTEFLAGS)

typedef DWORD ASSOCF;

#define INTERFACE IQueryAssociations
DECLARE_INTERFACE_(IQueryAssociations,IUnknown)
{
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD(Init)(THIS_ _In_ ASSOCF flags, _In_opt_ LPCWSTR pszAssoc, _In_opt_ HKEY hkProgid, _In_opt_ HWND hwnd) PURE;
    STDMETHOD(GetString)(THIS_ _In_ ASSOCF flags, _In_ ASSOCSTR str, _In_opt_ LPCWSTR pszExtra, _Out_writes_opt_(*pcchOut) LPWSTR pszOut, _Inout_ DWORD *pcchOut) PURE;
    STDMETHOD(GetKey)(THIS_ _In_ ASSOCF flags, _In_ ASSOCKEY key, _In_opt_ LPCWSTR pszExtra, _Out_ HKEY *phkeyOut) PURE;
    STDMETHOD(GetData)(THIS_ _In_ ASSOCF flags, _In_ ASSOCDATA data, _In_opt_ LPCWSTR pszExtra, _Out_writes_bytes_opt_(*pcbOut) LPVOID pvOut, _Inout_opt_ DWORD *pcbOut) PURE;
    STDMETHOD(GetEnum)(THIS_ _In_ ASSOCF flags, _In_ ASSOCENUM assocenum, _In_opt_ LPCWSTR pszExtra, _In_ REFIID riid, _Outptr_ LPVOID *ppvOut) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IQueryAssociations_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IQueryAssociations_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define IQueryAssociations_Release(p)              (p)->lpVtbl->Release(p)
#define IQueryAssociations_Init(p,a,b,c,d)         (p)->lpVtbl->Init(p,a,b,c,d)
#define IQueryAssociations_GetString(p,a,b,c,d,e)  (p)->lpVtbl->GetString(p,a,b,c,d,e)
#define IQueryAssociations_GetKey(p,a,b,c,d)       (p)->lpVtbl->GetKey(p,a,b,c,d)
#define IQueryAssociations_GetData(p,a,b,c,d,e)    (p)->lpVtbl->GetData(p,a,b,c,d,e)
#define IQueryAssociations_GetEnum(p,a,b,c,d,e)    (p)->lpVtbl->GetEnum(p,a,b,c,d,e)
#endif

typedef struct IQueryAssociations *LPQUERYASSOCIATIONS;

LWSTDAPI           AssocCreate(_In_ CLSID, _In_ REFIID, _Outptr_ LPVOID*);
#if (_WIN32_IE >= _WIN32_IE_IE60SP1)
LWSTDAPI_(BOOL)    AssocIsDangerous(_In_ LPCWSTR);
#endif // _WIN32_IE_IE60SP1

LWSTDAPI           AssocQueryStringA(_In_ ASSOCF, _In_ ASSOCSTR, _In_ LPCSTR, _In_opt_ LPCSTR, _Out_writes_opt_(*pcchOut) LPSTR, _Inout_ LPDWORD pcchOut);
LWSTDAPI           AssocQueryStringW(_In_ ASSOCF, _In_ ASSOCSTR, _In_ LPCWSTR, _In_opt_ LPCWSTR, _Out_writes_opt_(*pcchOut) LPWSTR, _Inout_ LPDWORD pcchOut);
LWSTDAPI           AssocQueryStringByKeyA(_In_ ASSOCF, _In_ ASSOCSTR, _In_ HKEY, _In_opt_ LPCSTR, _Out_writes_opt_(*pcchOut) LPSTR, _Inout_ LPDWORD pcchOut);
LWSTDAPI           AssocQueryStringByKeyW(_In_ ASSOCF, _In_ ASSOCSTR, _In_ HKEY, _In_opt_ LPCWSTR, _Out_writes_opt_(*pcchOut) LPWSTR, _Inout_ LPDWORD pcchOut);
LWSTDAPI           AssocQueryKeyA(_In_ ASSOCF, _In_ ASSOCKEY, _In_ LPCSTR, _In_opt_ LPCSTR, _Out_ PHKEY);
LWSTDAPI           AssocQueryKeyW(_In_ ASSOCF, _In_ ASSOCKEY, _In_ LPCWSTR, _In_opt_ LPCWSTR, _Out_ PHKEY);

#ifdef UNICODE
#define AssocQueryString       AssocQueryStringW
#define AssocQueryStringByKey  AssocQueryStringByKeyW
#define AssocQueryKey          AssocQueryKeyW
#else
#define AssocQueryString       AssocQueryStringA
#define AssocQueryStringByKey  AssocQueryStringByKeyA
#define AssocQueryKey          AssocQueryKeyA
#endif

#endif // NO_SHLWAPI_REG

/* Stream functions */
#ifndef NO_SHLWAPI_STREAM

LWSTDAPI             SHCreateStreamWrapper(LPBYTE,DWORD,DWORD,struct IStream**);
LWSTDAPI_(IStream *) SHCreateMemStream(_In_reads_bytes_opt_(cbInit) const BYTE *pInit, _In_ UINT cbInit);
LWSTDAPI             SHCreateStreamOnFileEx(_In_ LPCWSTR, _In_ DWORD, _In_ DWORD, _In_ BOOL, _In_opt_ struct IStream*, _Outptr_ struct IStream**);

LWSTDAPI             SHCreateStreamOnFileA(_In_ LPCSTR, _In_ DWORD, _Outptr_ struct IStream**);
LWSTDAPI             SHCreateStreamOnFileW(_In_ LPCWSTR, _In_ DWORD, _Outptr_ struct IStream**);
LWSTDAPI_(IStream *) SHOpenRegStreamA(_In_ HKEY, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _In_ DWORD);
LWSTDAPI_(IStream *) SHOpenRegStreamW(_In_ HKEY, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _In_ DWORD);
LWSTDAPI_(IStream *) SHOpenRegStream2A(_In_ HKEY, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _In_ DWORD);
LWSTDAPI_(IStream *) SHOpenRegStream2W(_In_ HKEY, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _In_ DWORD);

#ifdef UNICODE
#define SHCreateStreamOnFile  SHCreateStreamOnFileW
#define SHOpenRegStream       SHOpenRegStream2W
#define SHOpenRegStream2      SHOpenRegStream2W
#else
#define SHCreateStreamOnFile  SHCreateStreamOnFileA
#define SHOpenRegStream       SHOpenRegStream2A
#define SHOpenRegStream2      SHOpenRegStream2A
#endif // UNICODE

#ifndef _SHLWAPI_
LWSTDAPI IStream_Reset(_In_ struct IStream*);
#if !defined(IStream_Read) && defined(__cplusplus)
LWSTDAPI IStream_Read(_In_ struct IStream*, _Out_ void*, _In_ ULONG);
LWSTDAPI IStream_Write(_In_ struct IStream*, _In_ const void*, _In_ ULONG);
#endif
#endif // _SHLWAPI_

#endif // NO_SHLWAPI_STREAM

/* String functions */
#ifndef NO_SHLWAPI_STRFCNS

#define STIF_DEFAULT     0x0L
#define STIF_SUPPORT_HEX 0x1L

// These functions don't have -A and -W variants
LWSTDAPI           SHLoadIndirectString(_In_ LPCWSTR, _Out_writes_(cchOutBuf) LPWSTR, _In_ UINT cchOutBuf, _Reserved_ PVOID*);
LWSTDAPI           StrRetToBSTR(_Inout_ STRRET*, _In_opt_ PCUITEMID_CHILD, _Outptr_ BSTR*);

// These functions only exist as -W variants and don't have suffixless definitions
LWSTDAPI_(DWORD)   StrCatChainW(_Out_writes_(cchDst) LPWSTR, DWORD cchDst, DWORD, _In_ LPCWSTR);
LWSTDAPI_(int)     StrCmpLogicalW(_In_ LPCWSTR, _In_ LPCWSTR);
#if (_WIN32_IE >= _WIN32_IE_IE60)
LWSTDAPI_(LPWSTR)  StrStrNW(_In_ LPCWSTR, _In_ LPCWSTR, UINT);
LWSTDAPI_(LPWSTR)  StrStrNIW(_In_ LPCWSTR, _In_ LPCWSTR, UINT);
#endif // _WIN32_IE_IE60

// IntlStrEqWorker exists in both an -A and -W variant but doesn't have a suffixless definition
LWSTDAPI_(BOOL)    IntlStrEqWorkerA(BOOL, _In_reads_(nChar) LPCSTR, _In_reads_(nChar) LPCSTR, int nChar);
LWSTDAPI_(BOOL)    IntlStrEqWorkerW(BOOL, _In_reads_(nChar) LPCWSTR, _In_reads_(nChar) LPCWSTR, int nChar);

// Normal -A and -W functions with a suffixless definition
LWSTDAPI_(BOOL)    ChrCmpIA (WORD,WORD);
LWSTDAPI_(BOOL)    ChrCmpIW (WCHAR,WCHAR);
#define            IntlStrEqNA(s1,s2,n) IntlStrEqWorkerA(TRUE,s1,s2,n)
#define            IntlStrEqNW(s1,s2,n) IntlStrEqWorkerW(TRUE,s1,s2,n)
#define            IntlStrEqNIA(s1,s2,n) IntlStrEqWorkerA(FALSE,s1,s2,n)
#define            IntlStrEqNIW(s1,s2,n) IntlStrEqWorkerW(FALSE,s1,s2,n)
LWSTDAPI           SHStrDupA(_In_ LPCSTR psz, _Outptr_ WCHAR** ppwsz);
LWSTDAPI           SHStrDupW(_In_ LPCWSTR psz, _Outptr_ WCHAR** ppwsz);
#define            StrCatA lstrcatA
LWSTDAPI_(LPWSTR)  StrCatW(_Inout_ LPWSTR, _In_ LPCWSTR);
LWSTDAPI_(LPSTR)   StrCatBuffA(_Inout_updates_(cchDestBuffSize) LPSTR, _In_ LPCSTR, int cchDestBuffSize);
LWSTDAPI_(LPWSTR)  StrCatBuffW(_Inout_updates_(cchDestBuffSize) LPWSTR, _In_ LPCWSTR, int cchDestBuffSize);
LWSTDAPI_(LPSTR)   StrChrA(_In_ LPCSTR, WORD);
LWSTDAPI_(LPWSTR)  StrChrW(_In_ LPCWSTR, WCHAR);
LWSTDAPI_(LPSTR)   StrChrIA(_In_ LPCSTR, WORD);
LWSTDAPI_(LPWSTR)  StrChrIW(_In_ LPCWSTR, WCHAR);
#define            StrCmpA lstrcmpA
LWSTDAPI_(int)     StrCmpW(_In_ LPCWSTR, _In_ LPCWSTR);
#define            StrCmpIA lstrcmpiA
LWSTDAPI_(int)     StrCmpIW(_In_ LPCWSTR, _In_ LPCWSTR);
LWSTDAPI_(int)     StrCmpNA(_In_ LPCSTR, _In_ LPCSTR, INT);
LWSTDAPI_(int)     StrCmpNW(_In_ LPCWSTR, _In_ LPCWSTR, INT);
LWSTDAPI_(int)     StrCmpNIA(_In_ LPCSTR, _In_ LPCSTR, INT);
LWSTDAPI_(int)     StrCmpNIW(_In_ LPCWSTR, _In_ LPCWSTR, INT);
#define            StrCpyA lstrcpyA
LWSTDAPI_(LPWSTR)  StrCpyW(_Out_ LPWSTR, _In_ LPCWSTR);
#define            StrCpyNA lstrcpynA
LWSTDAPI_(LPWSTR)  StrCpyNW(_Out_writes_(cchMax) LPWSTR, _In_ LPCWSTR, int cchMax);
LWSTDAPI_(int)     StrCSpnA(_In_ LPCSTR, _In_ LPCSTR);
LWSTDAPI_(int)     StrCSpnW(_In_ LPCWSTR, _In_ LPCWSTR);
LWSTDAPI_(int)     StrCSpnIA(_In_ LPCSTR, _In_ LPCSTR);
LWSTDAPI_(int)     StrCSpnIW(_In_ LPCWSTR, _In_ LPCWSTR);
LWSTDAPI_(LPSTR)   StrDupA(_In_ LPCSTR);
LWSTDAPI_(LPWSTR)  StrDupW(_In_ LPCWSTR);
LWSTDAPI_(LPSTR)   StrFormatByteSize64A(LONGLONG, _Out_writes_(cchBuf) LPSTR, UINT cchBuf);
LWSTDAPI_(LPSTR)   StrFormatByteSizeA(DWORD, _Out_writes_(cchBuf) LPSTR, UINT cchBuf);
LWSTDAPI_(LPWSTR)  StrFormatByteSizeW(LONGLONG, _Out_writes_(cchBuf) LPWSTR, UINT cchBuf);
LWSTDAPI_(LPSTR)   StrFormatKBSizeA(LONGLONG, _Out_writes_(cchBuf) LPSTR, UINT cchBuf);
LWSTDAPI_(LPWSTR)  StrFormatKBSizeW(LONGLONG, _Out_writes_(cchBuf) LPWSTR, UINT cchBuf);
LWSTDAPI_(int)     StrFromTimeIntervalA(_Out_writes_(cchMax) LPSTR, UINT cchMax, DWORD, int);
LWSTDAPI_(int)     StrFromTimeIntervalW(_Out_writes_(cchMax) LPWSTR, UINT cchMax, DWORD, int);
#define            StrIntlEqNA(a,b,c) StrIsIntlEqualA(TRUE,a,b,c)
#define            StrIntlEqNW(a,b,c) StrIsIntlEqualW(TRUE,a,b,c)
#define            StrIntlEqNIA(a,b,c) StrIsIntlEqualA(FALSE,a,b,c)
#define            StrIntlEqNIW(a,b,c) StrIsIntlEqualW(FALSE,a,b,c)
LWSTDAPI_(BOOL)    StrIsIntlEqualA(BOOL, _In_ LPCSTR, _In_ LPCSTR, int);
LWSTDAPI_(BOOL)    StrIsIntlEqualW(BOOL, _In_ LPCWSTR, _In_ LPCWSTR, int);
LWSTDAPI_(LPSTR)   StrNCatA(_Inout_updates_(cchMax) LPSTR, LPCSTR, int cchMax);
LWSTDAPI_(LPWSTR)  StrNCatW(_Inout_updates_(cchMax) LPWSTR, LPCWSTR, int cchMax);
LWSTDAPI_(LPSTR)   StrPBrkA(_In_ LPCSTR, _In_ LPCSTR);
LWSTDAPI_(LPWSTR)  StrPBrkW(_In_ LPCWSTR, _In_ LPCWSTR);
LWSTDAPI_(LPSTR)   StrRChrA(_In_ LPCSTR, _In_opt_ LPCSTR, WORD);
LWSTDAPI_(LPWSTR)  StrRChrW(_In_ LPCWSTR, _In_opt_ LPCWSTR, WCHAR);
LWSTDAPI_(LPSTR)   StrRChrIA(_In_ LPCSTR, _In_opt_ LPCSTR, WORD);
LWSTDAPI_(LPWSTR)  StrRChrIW(_In_ LPCWSTR, _In_opt_ LPCWSTR, WCHAR);
LWSTDAPI_(LPSTR)   StrRStrIA(_In_ LPCSTR, _In_opt_ LPCSTR, _In_ LPCSTR);
LWSTDAPI_(LPWSTR)  StrRStrIW(_In_ LPCWSTR, _In_opt_ LPCWSTR, _In_ LPCWSTR);
LWSTDAPI           StrRetToBufA(_Inout_ STRRET*, _In_opt_ PCUITEMID_CHILD, _Out_writes_(cchBuf) LPSTR, UINT cchBuf);
LWSTDAPI           StrRetToBufW(_Inout_ STRRET*, _In_opt_ PCUITEMID_CHILD, _Out_writes_(cchBuf) LPWSTR, UINT cchBuf);
LWSTDAPI           StrRetToStrA(_Inout_ STRRET*, _In_opt_ PCUITEMID_CHILD, _Outptr_ LPSTR*);
LWSTDAPI           StrRetToStrW(_Inout_ STRRET*, _In_opt_ PCUITEMID_CHILD, _Outptr_ LPWSTR*);
LWSTDAPI_(int)     StrSpnA(_In_ LPCSTR psz, _In_ LPCSTR pszSet);
LWSTDAPI_(int)     StrSpnW(_In_ LPCWSTR psz, _In_ LPCWSTR pszSet);
LWSTDAPI_(LPSTR)   StrStrA(_In_ LPCSTR pszFirst, _In_ LPCSTR pszSrch);
LWSTDAPI_(LPWSTR)  StrStrW(_In_ LPCWSTR pszFirst, _In_ LPCWSTR pszSrch);
LWSTDAPI_(LPSTR)   StrStrIA(_In_ LPCSTR pszFirst, _In_ LPCSTR pszSrch);
LWSTDAPI_(LPWSTR)  StrStrIW(_In_ LPCWSTR pszFirst, _In_ LPCWSTR pszSrch);
LWSTDAPI_(int)     StrToIntA(_In_ LPCSTR);
LWSTDAPI_(int)     StrToIntW(_In_ LPCWSTR);
LWSTDAPI_(BOOL)    StrToIntExA(_In_ LPCSTR, DWORD, _Out_ int*);
LWSTDAPI_(BOOL)    StrToIntExW(_In_ LPCWSTR, DWORD, _Out_ int*);
LWSTDAPI_(BOOL)    StrTrimA(_Inout_ LPSTR, _In_ LPCSTR);
LWSTDAPI_(BOOL)    StrTrimW(_Inout_ LPWSTR, _In_ LPCWSTR);
LWSTDAPIV_(int)    wnsprintfA(_Out_writes_(cchDest) LPSTR, _In_ INT cchDest, _In_ _Printf_format_string_ LPCSTR, ...);
LWSTDAPIV_(int)    wnsprintfW(_Out_writes_(cchDest) LPWSTR, _In_ INT cchDest, _In_ _Printf_format_string_ LPCWSTR, ...);
LWSTDAPI_(int)     wvnsprintfA(_Out_writes_(cchDest) LPSTR, _In_ INT cchDest, _In_ _Printf_format_string_ LPCSTR, _In_ va_list);
LWSTDAPI_(int)     wvnsprintfW(_Out_writes_(cchDest) LPWSTR, _In_ INT cchDest, _In_ _Printf_format_string_ LPCWSTR, _In_ va_list);
#if (_WIN32_IE >= _WIN32_IE_IE60)
LWSTDAPI_(BOOL)    StrToInt64ExA(_In_ LPCSTR, DWORD, _Out_ LONGLONG*);
LWSTDAPI_(BOOL)    StrToInt64ExW(_In_ LPCWSTR, DWORD, _Out_ LONGLONG*);
#endif // _WIN32_IE_IE60
#if (_WIN32_IE >= _WIN32_IE_IE60SP2)
LWSTDAPI_(BOOL)    IsCharSpaceA(CHAR);
LWSTDAPI_(BOOL)    IsCharSpaceW(WCHAR);
#endif // _WIN32_IE_IE60SP2


#ifdef UNICODE
#define ChrCmpI              ChrCmpIW
#define IntlStrEqN           IntlStrEqNW
#define IntlStrEqNI          IntlStrEqNIW
#define SHStrDup             SHStrDupW
#define StrCat               StrCatW
#define StrCatBuff           StrCatBuffW
#define StrChr               StrChrW
#define StrChrI              StrChrIW
#define StrCmp               StrCmpW
#define StrCmpI              StrCmpIW
#define StrCmpN              StrCmpNW
#define StrCmpNI             StrCmpNIW
#define StrCpy               StrCpyW
#define StrCpyN              StrCpyNW
#define StrCSpn              StrCSpnW
#define StrCSpnI             StrCSpnIW
#define StrDup               StrDupW
#define StrFormatByteSize    StrFormatByteSizeW
#define StrFormatByteSize64  StrFormatByteSizeW
#define StrFormatKBSize      StrFormatKBSizeW
#define StrFromTimeInterval  StrFromTimeIntervalW
#define StrIntlEqN           StrIntlEqNW
#define StrIntlEqNI          StrIntlEqNIW
#define StrIsIntlEqual       StrIsIntlEqualW
#define StrNCat              StrNCatW
#define StrPBrk              StrPBrkW
#define StrRChr              StrRChrW
#define StrRChrI             StrRChrIW
#define StrRStrI             StrRStrIW
#define StrRetToBuf          StrRetToBufW
#define StrRetToStr          StrRetToStrW
#define StrSpn               StrSpnW
#define StrStr               StrStrW
#define StrStrI              StrStrIW
#define StrToInt             StrToIntW
#define StrToIntEx           StrToIntExW
#define StrTrim              StrTrimW
#define wnsprintf            wnsprintfW
#define wvnsprintf           wvnsprintfW
#if (_WIN32_IE >= _WIN32_IE_IE60)
#define StrToInt64Ex         StrToInt64ExW
#endif // _WIN32_IE_IE60
#if (_WIN32_IE >= _WIN32_IE_IE60SP2)
#define IsCharSpace          IsCharSpaceW
#endif // _WIN32_IE_IE60SP2
#else
#define ChrCmpI              ChrCmpIA
#define IntlStrEqN           IntlStrEqNA
#define IntlStrEqNI          IntlStrEqNIA
#define SHStrDup             SHStrDupA
#define StrCat               StrCatA
#define StrCatBuff           StrCatBuffA
#define StrChr               StrChrA
#define StrChrI              StrChrIA
#define StrCmp               StrCmpA
#define StrCmpI              StrCmpIA
#define StrCmpN              StrCmpNA
#define StrCmpNI             StrCmpNIA
#define StrCpy               StrCpyA
#define StrCpyN              StrCpyNA
#define StrCSpn              StrCSpnA
#define StrCSpnI             StrCSpnIA
#define StrDup               StrDupA
#define StrFormatByteSize    StrFormatByteSizeA
#define StrFormatByteSize64  StrFormatByteSize64A
#define StrFormatKBSize      StrFormatKBSizeA
#define StrFromTimeInterval  StrFromTimeIntervalA
#define StrIntlEqN           StrIntlEqNA
#define StrIntlEqNI          StrIntlEqNIA
#define StrIsIntlEqual       StrIsIntlEqualA
#define StrNCat              StrNCatA
#define StrPBrk              StrPBrkA
#define StrRChr              StrRChrA
#define StrRChrI             StrRChrIA
#define StrRStrI             StrRStrIA
#define StrRetToBuf          StrRetToBufA
#define StrRetToStr          StrRetToStrA
#define StrSpn               StrSpnA
#define StrStr               StrStrA
#define StrStrI              StrStrIA
#define StrToInt             StrToIntA
#define StrToIntEx           StrToIntExA
#define StrTrim              StrTrimA
#define wnsprintf            wnsprintfA
#define wvnsprintf           wvnsprintfA
#if (_WIN32_IE >= _WIN32_IE_IE60)
#define StrToInt64Ex         StrToInt64ExA
#endif // _WIN32_IE_IE60
#if (_WIN32_IE >= _WIN32_IE_IE60SP2)
#define IsCharSpace          IsCharSpaceA
#endif // _WIN32_IE_IE60SP2
#endif // UNICODE

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
typedef int SFBS_FLAGS;

enum tagSFBS_FLAGS
{
    SFBS_FLAGS_ROUND_TO_NEAREST_DISPLAYED_DIGIT    = 0x0001,
    SFBS_FLAGS_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS = 0x0002
};

LWSTDAPI StrFormatByteSizeEx(ULONGLONG ull, SFBS_FLAGS flags, _Out_writes_(cchBuf) PWSTR pszBuf, _In_range_(>,0) UINT cchBuf);
#endif // NTDDI_VISTASP1

#endif // NO_SHLWAPI_STRFCNS

#ifdef __cplusplus
}
#endif // __cplusplus

#include <poppack.h>

#endif // NOSHLWAPI
#endif // _INC_SHLWAPI
