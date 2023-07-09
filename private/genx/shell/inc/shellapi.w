 /*****************************************************************************\
*                                                                             *
* shellapi.h -  SHELL.DLL functions, types, and definitions                   *
*                                                                             *
* Copyright (c) 1992-1998, Microsoft Corp.  All rights reserved               *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_SHELLAPI
#define _INC_SHELLAPI

#ifndef _SHELAPIP_      ;internal_NT
#define _SHELAPIP_  ;internal_NT

#include <objbase.h>    ; internal_NT
;begin_both

//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHELLAPI
#if !defined(_SHELL32_)
#define WINSHELLAPI       DECLSPEC_IMPORT
#else
#define WINSHELLAPI
#endif
#endif // WINSHELLAPI

#ifndef SHSTDAPI
#if !defined(_SHELL32_)
#define SHSTDAPI          EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define SHSTDAPI_(type)   EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#else
#define SHSTDAPI          STDAPI
#define SHSTDAPI_(type)   STDAPI_(type)
#endif
#endif // SHSTDAPI

#ifndef SHDOCAPI
#if !defined(_SHDOCVW_)
#define SHDOCAPI          EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define SHDOCAPI_(type)   EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#else
#define SHDOCAPI          STDAPI
#define SHDOCAPI_(type)   STDAPI_(type)
#endif
#endif // SHDOCAPI


#include <pshpack1.h>

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


;end_both

DECLARE_HANDLE(HDROP);

SHSTDAPI_(UINT) DragQueryFile%(HDROP,UINT,LPTSTR%,UINT);
SHSTDAPI_(BOOL) DragQueryPoint(HDROP,LPPOINT);
SHSTDAPI_(void) DragFinish(HDROP);
SHSTDAPI_(void) DragAcceptFiles(HWND,BOOL);

SHSTDAPI_(HINSTANCE) ShellExecute%(HWND hwnd, LPCTSTR% lpOperation, LPCTSTR% lpFile, LPCTSTR% lpParameters, LPCTSTR% lpDirectory, INT nShowCmd);
SHSTDAPI_(HINSTANCE) FindExecutable%(LPCTSTR% lpFile, LPCTSTR% lpDirectory, LPTSTR% lpResult);
SHSTDAPI_(LPWSTR *)  CommandLineToArgvW(LPCWSTR lpCmdLine, int*pNumArgs);

SHSTDAPI_(INT) ShellAbout%(HWND hWnd, LPCTSTR% szApp, LPCTSTR% szOtherStuff, HICON hIcon);
SHSTDAPI_(HICON) DuplicateIcon(HINSTANCE hInst, HICON hIcon);
SHSTDAPI_(HICON) ExtractAssociatedIcon%(HINSTANCE hInst, LPTSTR% lpIconPath, LPWORD lpiIcon);
SHSTDAPI_(HICON) ExtractAssociatedIconEx%(HINSTANCE hInst,LPTSTR% lpIconPath,LPWORD lpiIconIndex, LPWORD lpiIconId);  ;internal_win40
SHSTDAPI_(HICON) ExtractIcon%(HINSTANCE hInst, LPCTSTR% lpszExeFileName, UINT nIconIndex);

;begin_winver_400
typedef struct _DRAGINFO% {
    UINT uSize;                 /* init with sizeof(DRAGINFO) */
    POINT pt;
    BOOL fNC;
    LPTSTR% lpFileList;
    DWORD grfKeyState;
} DRAGINFO%, *LPDRAGINFO%;

;begin_internal
// BUGBUG this API needs to be A/W. Don't make it public until it is.
SHSTDAPI_(BOOL) DragQueryInfo(HDROP hDrop, LPDRAGINFO lpdi);
;end_internal

////
//// AppBar stuff
////
#define ABM_NEW           0x00000000
#define ABM_REMOVE        0x00000001
#define ABM_QUERYPOS      0x00000002
#define ABM_SETPOS        0x00000003
#define ABM_GETSTATE      0x00000004
#define ABM_GETTASKBARPOS 0x00000005
#define ABM_ACTIVATE      0x00000006  // lParam == TRUE/FALSE means activate/deactivate
#define ABM_GETAUTOHIDEBAR 0x00000007
#define ABM_SETAUTOHIDEBAR 0x00000008  // this can fail at any time.  MUST check the result
                                        // lParam = TRUE/FALSE  Set/Unset
                                        // uEdge = what edge
#define ABM_WINDOWPOSCHANGED 0x0000009


// these are put in the wparam of callback messages
#define ABN_STATECHANGE    0x0000000
#define ABN_POSCHANGED     0x0000001
#define ABN_FULLSCREENAPP  0x0000002
#define ABN_WINDOWARRANGE  0x0000003 // lParam == TRUE means hide

// flags for get state
#define ABS_AUTOHIDE    0x0000001
#define ABS_ALWAYSONTOP 0x0000002

#define ABE_LEFT        0
#define ABE_TOP         1
#define ABE_RIGHT       2
#define ABE_BOTTOM      3
#define ABE_MAX         4     ;internal_win40

typedef struct _AppBarData
{
    DWORD cbSize;
    HWND hWnd;
    UINT uCallbackMessage;
    UINT uEdge;
    RECT rc;
    LPARAM lParam; // message specific
} APPBARDATA, *PAPPBARDATA;

SHSTDAPI_(UINT) SHAppBarMessage(DWORD dwMessage, PAPPBARDATA pData);

////
////  EndAppBar
////

SHSTDAPI_(HGLOBAL) InternalExtractIcon%(HINSTANCE hInst, LPCTSTR% lpszFile, UINT nIconIndex, UINT nIcons); ;internal_win40
SHSTDAPI_(HGLOBAL) InternalExtractIconList%(HANDLE hInst, LPTSTR% lpszExeFileName, LPINT lpnIcons); ;internal_win40
SHSTDAPI_(DWORD)   DoEnvironmentSubst%(LPTSTR% szString, UINT cchString);
SHSTDAPI_(BOOL)    RegisterShellHook(HWND, BOOL);                           ;internal_win40

#define EIRESID(x) (-1 * (int)(x))
SHSTDAPI_(UINT) ExtractIconEx%(LPCTSTR% lpszFile, int nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIcons);


////
//// Shell File Operations
////
#ifndef FO_MOVE //these need to be kept in sync with the ones in shlobj.h

#define FO_MOVE           0x0001
#define FO_COPY           0x0002
#define FO_DELETE         0x0003
#define FO_RENAME         0x0004

#define FOF_MULTIDESTFILES         0x0001
#define FOF_CONFIRMMOUSE           0x0002
#define FOF_SILENT                 0x0004  // don't create progress/report
#define FOF_RENAMEONCOLLISION      0x0008
#define FOF_NOCONFIRMATION         0x0010  // Don't prompt the user.
#define FOF_WANTMAPPINGHANDLE      0x0020  // Fill in SHFILEOPSTRUCT.hNameMappings
                                      // Must be freed using SHFreeNameMappings
#define FOF_ALLOWUNDO              0x0040
#define FOF_FILESONLY              0x0080  // on *.*, do only files
#define FOF_SIMPLEPROGRESS         0x0100  // means don't show names of files
#define FOF_NOCONFIRMMKDIR         0x0200  // don't confirm making any needed dirs
#define FOF_NOERRORUI              0x0400  // don't put up error UI
#define FOF_NOCOPYSECURITYATTRIBS  0x0800  // dont copy NT file Security Attributes
#define FOF_NORECURSION            0x1000  // don't recurse into directories.
#if (_WIN32_IE >= 0x0500)
#define FOF_NO_CONNECTED_ELEMENTS  0x2000  // don't operate on connected elements.
#define FOF_WANTNUKEWARNING        0x4000  // during delete operation, warn if nuking instead of recycling (partially overrides FOF_NOCONFIRMATION)
#endif // (_WIN32_IE >= 0x500)

typedef WORD FILEOP_FLAGS;

#define PO_DELETE       0x0013  // printer is being deleted
#define PO_RENAME       0x0014  // printer is being renamed
#define PO_PORTCHANGE   0x0020  // port this printer connected to is being changed
                                // if this id is set, the strings received by
                                // the copyhook are a doubly-null terminated
                                // list of strings.  The first is the printer
                                // name and the second is the printer port.
#define PO_REN_PORT     0x0034  // PO_RENAME and PO_PORTCHANGE at same time.

// no POF_ flags currently defined

typedef WORD PRINTEROP_FLAGS;

#endif // FO_MOVE

// implicit parameters are:
//      if pFrom or pTo are unqualified names the current directories are
//      taken from the global current drive/directory settings managed
//      by Get/SetCurrentDrive/Directory
//
//      the global confirmation settings

typedef struct _SHFILEOPSTRUCT%
{
        HWND            hwnd;
        UINT            wFunc;
        LPCTSTR%        pFrom;
        LPCTSTR%        pTo;
        FILEOP_FLAGS    fFlags;
        BOOL            fAnyOperationsAborted;
        LPVOID          hNameMappings;
        LPCTSTR%         lpszProgressTitle; // only used if FOF_SIMPLEPROGRESS
} SHFILEOPSTRUCT%, *LPSHFILEOPSTRUCT%;

SHSTDAPI_(int) SHFileOperation%(LPSHFILEOPSTRUCT% lpFileOp);
SHSTDAPI_(void) SHFreeNameMappings(HANDLE hNameMappings);

typedef struct _SHNAMEMAPPING%
{
    LPTSTR% pszOldPath;
    LPTSTR% pszNewPath;
    int   cchOldPath;
    int   cchNewPath;
} SHNAMEMAPPING%, *LPSHNAMEMAPPING%;

#define SHGetNameMappingCount(_hnm) DSA_GetItemCount(_hnm)              ;internal
#define SHGetNameMappingPtr(_hnm, _iItem) (LPSHNAMEMAPPING)DSA_GetItemPtr(_hnm, _iItem) ;internal

////
//// End Shell File Operations
////

////
////  Begin ShellExecuteEx and family
////
;begin_internal

typedef struct _RUNDLL_NOTIFY% {
    NMHDR     hdr;
    HICON     hIcon;
    LPTSTR%   lpszTitle;
} RUNDLL_NOTIFY%;

typedef void (WINAPI *RUNDLLPROC%)(HWND hwndStub, HINSTANCE hInstance, LPTSTR% pszCmdLine, int nCmdShow);

#define RDN_FIRST       (0U-500U)
#define RDN_LAST        (0U-509U)
#define RDN_TASKINFO    (RDN_FIRST-0)

#define SEN_DDEEXECUTE (SEN_FIRST-0)
;end_internal

/* ShellExecute() and ShellExecuteEx() error codes */

/* regular WinExec() codes */
#define SE_ERR_FNF              2       // file not found
#define SE_ERR_PNF              3       // path not found
#define SE_ERR_ACCESSDENIED     5       // access denied
#define SE_ERR_OOM              8       // out of memory
#define SE_ERR_DLLNOTFOUND              32

;end_winver_400

/* error values for ShellExecute() beyond the regular WinExec() codes */
#define SE_ERR_SHARE                    26
#define SE_ERR_ASSOCINCOMPLETE          27
#define SE_ERR_DDETIMEOUT               28
#define SE_ERR_DDEFAIL                  29
#define SE_ERR_DDEBUSY                  30
#define SE_ERR_NOASSOC                  31

;begin_winver_400
                                            ;internal_NT
HINSTANCE RealShellExecute%(                ;internal_NT
    HWND hwndParent,                        ;internal_NT
    LPCTSTR% lpOperation,                   ;internal_NT
    LPCTSTR% lpFile,                        ;internal_NT
    LPCTSTR% lpParameters,                  ;internal_NT
    LPCTSTR% lpDirectory,                   ;internal_NT
    LPTSTR% lpResult,                       ;internal_NT
    LPCTSTR% lpTitle,                       ;internal_NT
    LPTSTR% lpReserved,                     ;internal_NT
    WORD nShow,                             ;internal_NT
    LPHANDLE lphProcess);                   ;internal_NT

HINSTANCE RealShellExecuteEx%(              ;internal_NT
    HWND hwndParent,                        ;internal_NT
    LPCTSTR% lpOperation,                   ;internal_NT
    LPCTSTR% lpFile,                        ;internal_NT
    LPCTSTR% lpParameters,                  ;internal_NT
    LPCTSTR% lpDirectory,                   ;internal_NT
    LPTSTR% lpResult,                       ;internal_NT
    LPCTSTR% lpTitle,                       ;internal_NT
    LPTSTR% lpReserved,                     ;internal_NT
    WORD nShow,                             ;internal_NT
    LPHANDLE lphProcess,                    ;internal_NT
    DWORD dwFlags);                         ;internal_NT

//                                          ;internal_NT
// RealShellExecuteEx flags                 ;internal_NT
//                                          ;internal_NT
#define EXEC_SEPARATE_VDM     0x00000001    ;internal_NT
#define EXEC_NO_CONSOLE       0x00000002    ;internal

// Note CLASSKEY overrides CLASSNAME
#define SEE_MASK_CLASSNAME        0x00000001
#define SEE_MASK_CLASSKEY         0x00000003
// Note INVOKEIDLIST overrides IDLIST
#define SEE_MASK_IDLIST           0x00000004
#define SEE_MASK_INVOKEIDLIST     0x0000000c
#define SEE_MASK_ICON             0x00000010
#define SEE_MASK_HOTKEY           0x00000020
#define SEE_MASK_NOCLOSEPROCESS   0x00000040
#define SEE_MASK_CONNECTNETDRV    0x00000080
#define SEE_MASK_FLAG_DDEWAIT     0x00000100
#define SEE_MASK_DOENVSUBST       0x00000200
#define SEE_MASK_FLAG_NO_UI       0x00000400
#define SEE_MASK_FLAG_SHELLEXEC   0x00000800                               ;internal_win40
#define SEE_MASK_FORCENOIDLIST    0x00001000                               ;internal_win40
#define SEE_MASK_NO_HOOKS         0x00002000                               ;internal_win40
#define SEE_MASK_UNICODE          0x00004000
#define SEE_MASK_NO_CONSOLE       0x00008000
#define SEE_MASK_HASLINKNAME      0x00010000                               ;internal_win40
#define SEE_MASK_FLAG_SEPVDM      0x00020000                               ;internal_win40
#define SEE_MASK_RESERVED         0x00040000                               ;internal_win40
#define SEE_MASK_HASTITLE         0x00080000                               ;internal_win40
#define SEE_MASK_ASYNCOK          0x00100000
#define SEE_MASK_HMONITOR         0x00200000
#define SEE_MASK_FILEANDURL       0x00400000                               ;internal
#if (_WIN32_IE >= 0x0500)
#define SEE_MASK_NOQUERYCLASSSTORE 0x01000000
#define SEE_MASK_WAITFORINPUTIDLE  0x02000000
#endif // (_WIN32_IE >= 0x500)
// we have two CMIC_MASK_ values that don't have corospongind SEE_MASK_ counterparts    ;internal
//      CMIC_MASK_SHIFT_DOWN      0x10000000                                            ;internal
//      CMIC_MASK_CONTROL_DOWN    0x20000000                                            ;internal

// All other bits are masked off when we do an InvokeCommand             ;internal_win40
#define SEE_VALID_CMIC_BITS     0x308FAFF0                               ;internal_win40
#define SEE_VALID_CMIC_FLAGS    0x000FAFC0                               ;internal_win40

//
// For compilers that don't support nameless unions
//
#ifndef DUMMYUNIONNAME
#ifdef NONAMELESSUNION
#define DUMMYUNIONNAME   u
#define DUMMYUNIONNAME2  u2
#define DUMMYUNIONNAME3  u3
#define DUMMYUNIONNAME4  u4
#define DUMMYUNIONNAME5  u5
#else
#define DUMMYUNIONNAME
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#define DUMMYUNIONNAME4
#define DUMMYUNIONNAME5
#endif
#endif // DUMMYUNIONNAME

// The LPVOID lpIDList parameter is the IDList                                      ;internal_win40
typedef struct _SHELLEXECUTEINFO%
{
        DWORD cbSize;
        ULONG fMask;
        HWND hwnd;
        LPCTSTR% lpVerb;
        LPCTSTR% lpFile;
        LPCTSTR% lpParameters;
        LPCTSTR% lpDirectory;
        int nShow;
        HINSTANCE hInstApp;
        // Optional fields
        LPVOID lpIDList;
        LPCTSTR% lpClass;
        HKEY hkeyClass;
        DWORD dwHotKey;
        union {
        HANDLE hIcon;
        HANDLE hMonitor;
        } DUMMYUNIONNAME;
        HANDLE hProcess;
} SHELLEXECUTEINFO%, *LPSHELLEXECUTEINFO%;

SHSTDAPI_(BOOL) ShellExecuteEx%(LPSHELLEXECUTEINFO% lpExecInfo);
SHSTDAPI_(void) WinExecError%(HWND hwnd, int error, LPCTSTR% lpstrFileName, LPCTSTR% lpstrTitle);

//
//  SHCreateProcessAsUser()
typedef struct _SHCREATEPROCESSINFOW
{
        DWORD cbSize;
        ULONG fMask;
        HWND hwnd;
        LPCWSTR  pszFile;
        LPCWSTR  pszParameters;
        LPCWSTR  pszCurrentDirectory;
        IN HANDLE hUserToken;
        IN LPSECURITY_ATTRIBUTES lpProcessAttributes;
        IN LPSECURITY_ATTRIBUTES lpThreadAttributes;
        IN BOOL bInheritHandles;
        IN DWORD dwCreationFlags;
        IN LPSTARTUPINFOW lpStartupInfo;
        OUT LPPROCESS_INFORMATION lpProcessInformation;
} SHCREATEPROCESSINFOW, *PSHCREATEPROCESSINFOW;

SHSTDAPI_(BOOL) SHCreateProcessAsUserW(PSHCREATEPROCESSINFOW pscpi);

////
////  End ShellExecuteEx and family
////

//
// RecycleBin
//

// struct for query recycle bin info
typedef struct _SHQUERYRBINFO {
    DWORD   cbSize;
#if !defined(_MAC) || defined(_MAC_INT_64)
    __int64 i64Size;
    __int64 i64NumItems;
#else
    DWORDLONG i64Size;
    DWORDLONG i64NumItems;
#endif
} SHQUERYRBINFO, *LPSHQUERYRBINFO;


// flags for SHEmptyRecycleBin
//
#define SHERB_NOCONFIRMATION    0x00000001
#define SHERB_NOPROGRESSUI      0x00000002
#define SHERB_NOSOUND           0x00000004


SHSTDAPI SHQueryRecycleBin%(LPCTSTR% pszRootPath, LPSHQUERYRBINFO pSHQueryRBInfo);
SHSTDAPI SHEmptyRecycleBin%(HWND hwnd, LPCTSTR% pszRootPath, DWORD dwFlags);

////
//// end of RecycleBin


////
//// Tray notification definitions
////
;begin_internal
//
//  We have to define this structure twice.
//  The public definition uses HWNDs and HICONs.
//  The private definition uses DWORDs for Win32/64 interop.
//  The private version is called "NOTIFYICONDATA32" because it is the
//  explicit 32-bit version.
//
//  Make sure to keep them in sync!
//
;end_internal

typedef struct _NOTIFYICONDATA% {
        DWORD cbSize;
        HWND hWnd;
        UINT uID;
        UINT uFlags;
        UINT uCallbackMessage;
        HICON hIcon;
#if (_WIN32_IE < 0x0500)
        TCHAR% szTip[64];
#else
        TCHAR% szTip[128];
#endif
#if (_WIN32_IE >= 0x0500)
        DWORD dwState;
        DWORD dwStateMask;
        TCHAR% szInfo[256];
        union {
            UINT  uTimeout;
            UINT  uVersion;
        } DUMMYUNIONNAME;
        TCHAR% szInfoTitle[64];
        DWORD dwInfoFlags;
#endif
} NOTIFYICONDATA%, *PNOTIFYICONDATA%;

;begin_internal

typedef struct _NOTIFYICONDATA32% {
        DWORD cbSize;
        DWORD dwWnd;                        // NB!
        UINT uID;
        UINT uFlags;
        UINT uCallbackMessage;
        DWORD dwIcon;                       // NB!
#if (_WIN32_IE < 0x0500)
        TCHAR% szTip[64];
#else
        TCHAR% szTip[128];
#endif
#if (_WIN32_IE >= 0x0500)
        DWORD dwState;
        DWORD dwStateMask;
        TCHAR% szInfo[256];
        union {
            UINT  uTimeout;
            UINT  uVersion;
        } DUMMYUNIONNAME;
        TCHAR% szInfoTitle[64];
        DWORD dwInfoFlags;
#endif
} NOTIFYICONDATA32%, *PNOTIFYICONDATA32%;
;end_internal

#define NOTIFYICONDATAA_V1_SIZE     FIELD_OFFSET(NOTIFYICONDATAA, szTip[64])
#define NOTIFYICONDATAW_V1_SIZE     FIELD_OFFSET(NOTIFYICONDATAW, szTip[64])
#ifdef UNICODE
#define NOTIFYICONDATA_V1_SIZE      NOTIFYICONDATAW_V1_SIZE
#else
#define NOTIFYICONDATA_V1_SIZE      NOTIFYICONDATAA_V1_SIZE
#endif

typedef struct _TRAYNOTIFYDATA% {                                        ;internal_win40
        DWORD dwSignature;                                               ;internal_win40
        DWORD dwMessage;                                                 ;internal_win40
        NOTIFYICONDATA32 nid;                                            ;internal_win40
} TRAYNOTIFYDATA%, *PTRAYNOTIFYDATA%;                                    ;internal_win40
                                                                         ;internal_win40
#define NI_SIGNATURE    0x34753423                                       ;internal_win40
                                                                         ;internal_win40
#define WNDCLASS_TRAYNOTIFY     "Shell_TrayWnd"                          ;internal_win40

#if (_WIN32_IE >= 0x0500)
#define NIN_SELECT          (WM_USER + 0)
#define NINF_KEY            0x1
#define NIN_KEYSELECT       (NIN_SELECT | NINF_KEY)
//                          (WM_USER + 1) = NIN_KEYSELECT ;Internal
#endif


#define NIM_ADD         0x00000000
#define NIM_MODIFY      0x00000001
#define NIM_DELETE      0x00000002
#if (_WIN32_IE >= 0x0500)
#define NIM_SETFOCUS    0x00000003
#define NIM_SETVERSION  0x00000004
#define     NOTIFYICON_VERSION 3
#endif

#define NIF_MESSAGE     0x00000001
#define NIF_ICON        0x00000002
#define NIF_TIP         0x00000004
#if (_WIN32_IE >= 0x0500)
#define NIF_STATE       0x00000008
#define NIF_INFO        0x00000010
#endif
#define NIF_VALID_V1    0x00000007          ;internal
#define NIF_VALID       0x0000001F          ;internal

#if (_WIN32_IE >= 0x0500)
#define NIS_HIDDEN      0x00000001
#define NIS_SHAREDICON  0x00000002

// Notify Icon Infotip flags
#define NIIF_NONE       0x00000000
// icon flags are mutualy exclusive
// and take only the lowest 2 bits
#define NIIF_INFO       0x00000001
#define NIIF_WARNING    0x00000002
#define NIIF_ERROR      0x00000003
#endif

SHSTDAPI_(BOOL) Shell_NotifyIcon%(DWORD dwMessage, PNOTIFYICONDATA% lpData);

////
//// End Tray Notification Icons
////


#ifndef SHFILEINFO_DEFINED
#define SHFILEINFO_DEFINED
////
//// Begin SHGetFileInfo
////

/*
 * The SHGetFileInfo API provides an easy way to get attributes
 * for a file given a pathname.
 *
 *   PARAMETERS
 *
 *     pszPath              file name to get info about
 *     dwFileAttributes     file attribs, only used with SHGFI_USEFILEATTRIBUTES
 *     psfi                 place to return file info
 *     cbFileInfo           size of structure
 *     uFlags               flags
 *
 *   RETURN
 *     TRUE if things worked
 */

typedef struct _SHFILEINFO%
{
        HICON       hIcon;                      // out: icon
        int         iIcon;                      // out: icon index
        DWORD       dwAttributes;               // out: SFGAO_ flags
        TCHAR%      szDisplayName[MAX_PATH];    // out: display name (or path)
        TCHAR%      szTypeName[80];             // out: type name
} SHFILEINFO%;


// NOTE: This is also in shlwapi.h.  Please keep in synch.
#endif // !SHFILEINFO_DEFINED

#define SHGFI_ICON              0x000000100     // get icon
#define SHGFI_DISPLAYNAME       0x000000200     // get display name
#define SHGFI_TYPENAME          0x000000400     // get type name
#define SHGFI_ATTRIBUTES        0x000000800     // get attributes
#define SHGFI_ICONLOCATION      0x000001000     // get icon location
#define SHGFI_EXETYPE           0x000002000     // return exe type
#define SHGFI_SYSICONINDEX      0x000004000     // get system icon index
#define SHGFI_LINKOVERLAY       0x000008000     // put a link overlay on icon
#define SHGFI_SELECTED          0x000010000     // show icon in selected state
#define SHGFI_ATTR_SPECIFIED    0x000020000     // get only specified attributes
#define SHGFI_LARGEICON         0x000000000     // get large icon
#define SHGFI_SMALLICON         0x000000001     // get small icon
#define SHGFI_OPENICON          0x000000002     // get open icon
#define SHGFI_SHELLICONSIZE     0x000000004     // get shell size icon
#define SHGFI_PIDL              0x000000008     // pszPath is a pidl
#define SHGFI_USEFILEATTRIBUTES 0x000000010     // use passed dwFileAttribute

#if (_WIN32_IE >= 0x0500)
#define SHGFI_ADDOVERLAYS       0x000000020     // apply the appropriate overlays
#define SHGFI_OVERLAYINDEX      0x000000040     // Get the index of the overlay
                                                // in the upper 8 bits of the iIcon 
#endif

SHSTDAPI_(DWORD_PTR) SHGetFileInfo%(LPCTSTR% pszPath, DWORD dwFileAttributes, SHFILEINFO% *psfi, UINT cbFileInfo, UINT uFlags);


#define SHGetDiskFreeSpace SHGetDiskFreeSpaceEx

SHSTDAPI_(BOOL) SHGetDiskFreeSpaceEx%(LPCTSTR% pszDirectoryName, ULARGE_INTEGER* pulFreeBytesAvailableToCaller, ULARGE_INTEGER* pulTotalNumberOfBytes, ULARGE_INTEGER* pulTotalNumberOfFreeBytes);
SHSTDAPI_(BOOL) SHGetNewLinkInfo%(LPCTSTR% pszLinkTo, LPCTSTR% pszDir, LPTSTR% pszName, BOOL *pfMustCopy, UINT uFlags);

#define SHGNLI_PIDL             0x000000001     // pszLinkTo is a pidl
#define SHGNLI_PREFIXNAME       0x000000002     // Make name "Shortcut to xxx"
#define SHGNLI_NOUNIQUE         0x000000004     // don't do the unique name generation


////
//// End SHGetFileInfo
////

// Printer stuff
#define PRINTACTION_OPEN           0
#define PRINTACTION_PROPERTIES     1
#define PRINTACTION_NETINSTALL     2
#define PRINTACTION_NETINSTALLLINK 3
#define PRINTACTION_TESTPAGE       4
#define PRINTACTION_OPENNETPRN     5
#ifdef WINNT
#define PRINTACTION_DOCUMENTDEFAULTS 6
#define PRINTACTION_SERVERPROPERTIES 7
#endif

SHSTDAPI_(BOOL) SHInvokePrinterCommand%(HWND hwnd, UINT uAction, LPCTSTR% lpBuf1, LPCTSTR% lpBuf2, BOOL fModal);

//                                                                ;internal_NT
// Old NT Compatibility stuff (remove later)                      ;internal_NT
//                                                                ;internal_NT
SHSTDAPI_(VOID) CheckEscapes%(LPTSTR% lpFileA, DWORD cch);        ;internal_NT
SHSTDAPI_(LPTSTR%) SheRemoveQuotes%(LPTSTR% sz);                  ;internal_NT
SHSTDAPI_(WORD) ExtractIconResInfo%(HANDLE hInst,LPTSTR% lpszFileName,WORD wIconIndex,LPWORD lpwSize,LPHANDLE lphIconRes); ;internal_NT
SHSTDAPI_(int) SheSetCurDrive(int iDrive);                        ;internal_NT
SHSTDAPI_(int) SheChangeDir%(register TCHAR% *newdir);            ;internal_NT
SHSTDAPI_(int) SheGetDir%(int iDrive, TCHAR% *str);               ;internal_NT
SHSTDAPI_(BOOL) SheConvertPath%(LPTSTR% lpApp, LPTSTR% lpFile, UINT cchCmdBuf); ;internal_NT
SHSTDAPI_(BOOL) SheShortenPath%(LPTSTR% pPath, BOOL bShorten);    ;internal_NT
SHSTDAPI_(BOOL) RegenerateUserEnvironment(PVOID *pPrevEnv,        ;internal_NT
                                        BOOL bSetCurrentEnv);     ;internal_NT
SHSTDAPI_(INT) SheGetPathOffsetW(LPWSTR lpszDir);                 ;internal_NT
SHSTDAPI_(BOOL) SheGetDirExW(LPWSTR lpszCurDisk, LPDWORD lpcchCurDir,LPWSTR lpszCurDir); ;internal_NT
SHSTDAPI_(DWORD) ExtractVersionResource16W(LPCWSTR  lpwstrFilename, LPHANDLE lphData);   ;internal_NT
SHSTDAPI_(INT) SheChangeDirEx%(register TCHAR% *newdir);          ;internal_NT

//                                                                ;internal_NT
// PRINTQ                                                         ;internal_NT
//                                                                ;internal_NT
VOID Printer_LoadIcons%(LPCTSTR% pszPrinterName, HICON* phLargeIcon, HICON* phSmallIcon); ;internal_NT
LPTSTR% ShortSizeFormat%(DWORD dw, LPTSTR% szBuf);                ;internal_NT
LPTSTR% AddCommas%(DWORD dw, LPTSTR% pszResult);                  ;internal_NT

BOOL Printers_RegisterWindow%(LPCTSTR% pszPrinter, DWORD dwType, PHANDLE phClassPidl, HWND *phwnd); ;internal_NT
VOID Printers_UnregisterWindow(HANDLE hClassPidl, HWND hwnd);     ;internal_NT

#define PRINTER_PIDL_TYPE_PROPERTIES       0x1                    ;internal_NT
#define PRINTER_PIDL_TYPE_DOCUMENTDEFAULTS 0x2                    ;internal_NT
#define PRINTER_PIDL_TYPE_ALL_USERS_DOCDEF 0x3                    ;internal_NT
#define PRINTER_PIDL_TYPE_JOBID            0x80000000             ;internal_NT

;end_winver_400


#if (_WIN32_WINNT >= 0x0500) || (_WIN32_WINDOWS >= 0x0500)  

//
// The SHLoadNonloadedIconOverlayIdentifiers API causes the shell's
// icon overlay manager to load any registered icon overlay
// identifers that are not currently loaded.  This is useful if an
// overlay identifier did not load at shell startup but is needed
// and can be loaded at a later time.  Identifiers already loaded
// are not affected.  Overlay identifiers implement the 
// IShellIconOverlayIdentifier interface.
//
// Returns:
//      S_OK
// 
SHSTDAPI SHLoadNonloadedIconOverlayIdentifiers(void);

//
// The SHIsFileAvailableOffline API determines whether a file
// or folder is available for offline use.
//
// Parameters:
//     pwszPath             file name to get info about
//     pdwStatus            (optional) OFFLINE_STATUS_* flags returned here
//
// Returns:
//     S_OK                 File/directory is available offline, unless
//                            OFFLINE_STATUS_INCOMPLETE is returned.
//     E_INVALIDARG         Path is invalid, or not a net path
//     E_FAIL               File/directory is not available offline
// 
// Notes:
//     OFFLINE_STATUS_INCOMPLETE is never returned for directories.
//     Both OFFLINE_STATUS_LOCAL and OFFLINE_STATUS_REMOTE may be returned,
//     indicating "open in both places." This is common when the server is online.
//
SHSTDAPI SHIsFileAvailableOffline(LPCWSTR pwszPath, LPDWORD pdwStatus);

#define OFFLINE_STATUS_LOCAL        0x0001  // If open, it's open locally
#define OFFLINE_STATUS_REMOTE       0x0002  // If open, it's open remotely
#define OFFLINE_STATUS_INCOMPLETE   0x0004  // The local copy is currently imcomplete.
                                            // The file will not be available offline
                                            // until it has been synchronized.

#endif


;begin_internal
//
// Internal APIs Follow.  NOT FOR PUBLIC CONSUMPTION.
//

//====== ShellMessageBox ================================================

// If lpcTitle is NULL, the title is taken from hWnd
// If lpcText is NULL, this is assumed to be an Out Of Memory message
// If the selector of lpcTitle or lpcText is NULL, the offset should be a
//     string resource ID
// The variable arguments must all be 32-bit values (even if fewer bits
//     are actually used)
// lpcText (or whatever string resource it causes to be loaded) should
//     be a formatting string similar to wsprintf except that only the
//     following formats are available:
//         %%              formats to a single '%'
//         %nn%s           the nn-th arg is a string which is inserted
//         %nn%ld          the nn-th arg is a DWORD, and formatted decimal
//         %nn%lx          the nn-th arg is a DWORD, and formatted hex
//     note that lengths are allowed on the %s, %ld, and %lx, just
//                         like wsprintf
//

int _cdecl ShellMessageBox%(
    HINSTANCE hAppInst,
    HWND hWnd,
    LPCTSTR% lpcText,
    LPCTSTR% lpcTitle,
    UINT fuStyle, ...);

//====== Random stuff ================================================

SHSTDAPI_(BOOL) IsLFNDrive%(LPCTSTR% pszPath);


#if (_WIN32_WINNT >= 0x0500) || (_WIN32_WINDOWS >= 0x0500)  

//
// The SHMultiFileProperties API displays a property sheet for a 
// set of files specified in an IDList Array.
//
// Parameters:
//      pdtobj  - Data object containing list of files.  The data
//                object must provide the "Shell IDList Array" 
//                clipboard format.  The parent folder's implementation of
//                IShellFolder::GetDisplayNameOf must return a fully-qualified
//                filesystem path for each item in response to the 
//                SHGDN_FORPARSING flag.
//
//      dwFlags - Reserved for future use.  Should be set to 0.
//
// Returns:
//      S_OK
// 
SHSTDAPI SHMultiFileProperties(IDataObject *pdtobj, DWORD dwFlags);

#endif 


;end_internal


;begin_both
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#include <poppack.h>
;end_both

#endif  /* _SHELAPIP_ */ ;internal_NT
#endif  /* _INC_SHELLAPI */
