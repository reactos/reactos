#ifndef _SHELLAPI_H
#define _SHELLAPI_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#endif

#if !defined(_WIN64)
#include <pshpack1.h>
#endif

#define WINSHELLAPI DECLSPEC_IMPORT
#define ABE_LEFT	0
#define ABE_TOP	1
#define ABE_RIGHT	2
#define ABE_BOTTOM	3
#define ABS_AUTOHIDE	1
#define ABS_ALWAYSONTOP	2

#define SEE_MASK_DEFAULT	0x00000000
#define SEE_MASK_CLASSNAME	0x00000001
#define SEE_MASK_CLASSKEY	0x00000003
#define SEE_MASK_IDLIST	0x00000004
#define SEE_MASK_INVOKEIDLIST	0x0000000C
#define SEE_MASK_ICON	0x00000010
#define SEE_MASK_HOTKEY	0x00000020
#define SEE_MASK_NOCLOSEPROCESS	0x00000040
#define SEE_MASK_CONNECTNETDRV	0x00000080
#define SEE_MASK_NOASYNC	0x00000100
#define SEE_MASK_FLAG_DDEWAIT	SEE_MASK_NOASYNC
#define SEE_MASK_DOENVSUBST	0x00000200
#define SEE_MASK_FLAG_NO_UI	0x00000400
#define SEE_MASK_UNICODE	0x00004000
#define SEE_MASK_NO_CONSOLE	0x00008000
/*
 * NOTE: The following 5 flags are undocumented and are not present in the
 * official Windows SDK. However they are used in shobjidl.idl to define some
 * CMIC_MASK_* flags, these ones being mentioned in the MSDN documentation of
 * the CMINVOKECOMMANDINFOEX structure.
 * I affect them this range of values which seems to be strangely empty. Of
 * course their values may differ from the real ones, however I have no way
 * of discovering them. If somebody else can verify them, it would be great.
 */
#define SEE_MASK_UNKNOWN_0x1000 0x00001000 /* FIXME: Name */
#define SEE_MASK_HASLINKNAME    0x00010000
#define SEE_MASK_FLAG_SEPVDM    0x00020000
#define SEE_MASK_USE_RESERVED   0x00040000
#define SEE_MASK_HASTITLE       0x00080000
/* END NOTE */
#define SEE_MASK_ASYNCOK	0x00100000
#define SEE_MASK_HMONITOR	0x00200000
#define SEE_MASK_NOZONECHECKS	0x00800000
#define SEE_MASK_NOQUERYCLASSSTORE	0x01000000
#define SEE_MASK_WAITFORINPUTIDLE	0x02000000
#define SEE_MASK_FLAG_LOG_USAGE	0x04000000
#define SEE_MASK_FLAG_HINST_IS_SITE 0x08000000

#define ABM_NEW	0
#define ABM_REMOVE	1
#define ABM_QUERYPOS	2
#define ABM_SETPOS	3
#define ABM_GETSTATE	4
#define ABM_GETTASKBARPOS	5
#define ABM_ACTIVATE	6
#define ABM_GETAUTOHIDEBAR	7
#define ABM_SETAUTOHIDEBAR	8
#define ABM_WINDOWPOSCHANGED	9
#define ABM_SETSTATE            10
#define ABN_STATECHANGE		0
#define ABN_POSCHANGED		1
#define ABN_FULLSCREENAPP	2
#define ABN_WINDOWARRANGE	3

#if (_WIN32_IE >= 0x0500)
#define NIN_SELECT          (WM_USER + 0)
#define NINF_KEY            1
#define NIN_KEYSELECT       (NIN_SELECT | NINF_KEY)
#endif

#if (_WIN32_IE >= 0x0501)
#define NIN_BALLOONSHOW         (WM_USER + 2)
#define NIN_BALLOONHIDE         (WM_USER + 3)
#define NIN_BALLOONTIMEOUT      (WM_USER + 4)
#define NIN_BALLOONUSERCLICK    (WM_USER + 5)
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define NIN_POPUPOPEN           (WM_USER + 6)
#define NIN_POPUPCLOSE          (WM_USER + 7)
#endif

#define NIM_ADD	0
#define NIM_MODIFY	1
#define NIM_DELETE	2
#if _WIN32_IE >= 0x0500
#define NIM_SETFOCUS	3
#define NIM_SETVERSION	4
#define NOTIFYICON_VERSION      3
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define NOTIFYICON_VERSION_4    4
#endif
#endif
#define NIF_MESSAGE	1
#define NIF_ICON	2
#define NIF_TIP	4
#if _WIN32_IE >= 0x0500
#define NIF_STATE	8
#define NIF_INFO	16
#define NIS_HIDDEN	1
#define NIS_SHAREDICON	2
#define NIIF_NONE	0
#define NIIF_INFO	1
#define NIIF_WARNING	2
#define NIIF_ERROR	3
#define NIIF_USER	4
#if _WIN32_IE >= 0x0600
#define NIF_GUID	32
#define NIIF_ICON_MASK	0xf
#define NIIF_NOSOUND	0x10
#endif /* _WIN32_IE >= 0x0600 */
#endif /* _WIN32_IE >= 0x0500 */

#define SE_ERR_FNF	2
#define SE_ERR_PNF	3
#define SE_ERR_ACCESSDENIED	5
#define SE_ERR_OOM	8
#define SE_ERR_DLLNOTFOUND	32
#define SE_ERR_SHARE	26
#define SE_ERR_ASSOCINCOMPLETE	27
#define SE_ERR_DDETIMEOUT	28
#define SE_ERR_DDEFAIL	29
#define SE_ERR_DDEBUSY	30
#define SE_ERR_NOASSOC	31
#define FO_MOVE	1
#define FO_COPY	2
#define FO_DELETE	3
#define FO_RENAME	4

#define FOF_MULTIDESTFILES         0x0001
#define FOF_CONFIRMMOUSE           0x0002
#define FOF_SILENT                 0x0004
#define FOF_RENAMEONCOLLISION      0x0008
#define FOF_NOCONFIRMATION         0x0010
#define FOF_WANTMAPPINGHANDLE      0x0020
#define FOF_ALLOWUNDO              0x0040
#define FOF_FILESONLY              0x0080
#define FOF_SIMPLEPROGRESS         0x0100
#define FOF_NOCONFIRMMKDIR         0x0200
#define FOF_NOERRORUI              0x0400
#define FOF_NOCOPYSECURITYATTRIBS  0x0800
#define FOF_NORECURSION            0x1000  /* don't do recursion into directories */
#define FOF_NO_CONNECTED_ELEMENTS  0x2000  /* don't do connected files */
#define FOF_WANTNUKEWARNING        0x4000  /* during delete operation, warn if delete instead
                                              of recycling (even if FOF_NOCONFIRMATION) */
#define FOF_NORECURSEREPARSE       0x8000  /* don't do recursion into reparse points */

#define PO_DELETE 19
#define PO_RENAME 20
#define PO_PORTCHANGE 32
#define PO_REN_PORT 52
#define SHGFI_ADDOVERLAYS	32
#define SHGFI_OVERLAYINDEX	64
#define SHGFI_ICON	256
#define SHGSI_ICON SHGFI_ICON
#define SHGFI_DISPLAYNAME	512
#define SHGFI_TYPENAME	1024
#define SHGFI_ATTRIBUTES	2048
#define SHGFI_ICONLOCATION	4096
#define SHGFI_EXETYPE 8192
#define SHGFI_SYSICONINDEX 16384
#define SHGFI_LINKOVERLAY 32768
#define SHGFI_SELECTED 65536
#define SHGFI_ATTR_SPECIFIED 131072
#define SHGFI_LARGEICON	0
#define SHGFI_SMALLICON	1
#define SHGSI_SMALLICON SHGFI_SMALLICON
#define SHGFI_OPENICON	2
#define SHGFI_SHELLICONSIZE	4
#define SHGFI_PIDL	8
#define SHGFI_USEFILEATTRIBUTES	16

#if (NTDDI_VERSION >= NTDDI_WINXP)
#define SHIL_LARGE        0x0
#define SHIL_SMALL        0x1
#define SHIL_EXTRALARGE   0x2
#define SHIL_SYSSMALL     0x3
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define SHIL_JUMBO        0x4
#define SHIL_LAST         SHIL_JUMBO
#else
#define SHIL_LAST         SHIL_SYSSMALL
#endif
#endif

typedef struct _SHCREATEPROCESSINFOW
{
    DWORD cbSize;
    ULONG fMask;
    HWND hwnd;
    LPCWSTR pszFile;
    LPCWSTR pszParameters;
    LPCWSTR pszCurrentDirectory;
    IN HANDLE hUserToken;
    IN LPSECURITY_ATTRIBUTES lpProcessAttributes;
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes;
    IN BOOL bInheritHandles;
    IN DWORD dwCreationFlags;
    IN LPSTARTUPINFOW lpStartupInfo;
    OUT LPPROCESS_INFORMATION lpProcessInformation;
} SHCREATEPROCESSINFOW, *PSHCREATEPROCESSINFOW;

typedef WORD FILEOP_FLAGS;
typedef WORD PRINTEROP_FLAGS;

typedef struct _AppBarData {
	DWORD	cbSize;
	HWND	hWnd;
	UINT	uCallbackMessage;
	UINT	uEdge;
	RECT	rc;
	LPARAM lParam;
} APPBARDATA,*PAPPBARDATA;
DECLARE_HANDLE(HDROP);

typedef struct _NOTIFYICONDATAA {
	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
#if (NTDDI_VERSION < NTDDI_WIN2K)
	CHAR szTip[64];
#endif
#if (NTDDI_VERSION >= NTDDI_WIN2K)
	CHAR szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	CHAR szInfo[256];
	_ANONYMOUS_UNION union {
		UINT uTimeout;
		UINT uVersion;
	} DUMMYUNIONNAME;
	CHAR szInfoTitle[64];
	DWORD dwInfoFlags;
#endif
#if (NTDDI_VERSION >= NTDDI_WINXP)
	GUID guidItem;
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
	HICON hBalloonIcon;
#endif
} NOTIFYICONDATAA,*PNOTIFYICONDATAA;

typedef struct _NOTIFYICONDATAW {
	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
#if (NTDDI_VERSION < NTDDI_WIN2K)
	WCHAR szTip[64];
#endif
#if (NTDDI_VERSION >= NTDDI_WIN2K)
	WCHAR szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	WCHAR szInfo[256];
	_ANONYMOUS_UNION union {
		UINT uTimeout;
		UINT uVersion;
	} DUMMYUNIONNAME;
	WCHAR szInfoTitle[64];
	DWORD dwInfoFlags;
#endif
#if (NTDDI_VERSION >= NTDDI_WINXP)
	GUID guidItem;
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
	HICON hBalloonIcon;
#endif
} NOTIFYICONDATAW,*PNOTIFYICONDATAW;

#define NOTIFYICONDATAA_V1_SIZE FIELD_OFFSET(NOTIFYICONDATAA, szTip[64])
#define NOTIFYICONDATAW_V1_SIZE FIELD_OFFSET(NOTIFYICONDATAW, szTip[64])
#define NOTIFYICONDATAA_V2_SIZE FIELD_OFFSET(NOTIFYICONDATAA, guidItem)
#define NOTIFYICONDATAW_V2_SIZE FIELD_OFFSET(NOTIFYICONDATAW, guidItem)
#define NOTIFYICONDATAA_V3_SIZE FIELD_OFFSET(NOTIFYICONDATAA, hBalloonIcon)
#define NOTIFYICONDATAW_V3_SIZE FIELD_OFFSET(NOTIFYICONDATAW, hBalloonIcon)

#if WINVER >= 0x400
typedef struct _DRAGINFOA {
	UINT uSize;
	POINT pt;
	BOOL fNC;
	LPSTR lpFileList;
	DWORD grfKeyState;
} DRAGINFOA,*LPDRAGINFOA;
typedef struct _DRAGINFOW {
	UINT uSize;
	POINT pt;
	BOOL fNC;
	LPWSTR lpFileList;
	DWORD grfKeyState;
} DRAGINFOW,*LPDRAGINFOW;
#endif

typedef struct _SHELLEXECUTEINFOA {
	DWORD cbSize;
	ULONG fMask;
	HWND hwnd;
	LPCSTR lpVerb;
	LPCSTR lpFile;
	LPCSTR lpParameters;
	LPCSTR lpDirectory;
	int nShow;
	HINSTANCE hInstApp;
	PVOID lpIDList;
	LPCSTR lpClass;
	HKEY hkeyClass;
	DWORD dwHotKey;
	HANDLE hIcon;
	HANDLE hProcess;
} SHELLEXECUTEINFOA,*LPSHELLEXECUTEINFOA;
typedef struct _SHELLEXECUTEINFOW {
	DWORD cbSize;
	ULONG fMask;
	HWND hwnd;
	LPCWSTR lpVerb;
	LPCWSTR lpFile;
	LPCWSTR lpParameters;
	LPCWSTR lpDirectory;
	int nShow;
	HINSTANCE hInstApp;
	PVOID lpIDList;
	LPCWSTR lpClass;
	HKEY hkeyClass;
	DWORD dwHotKey;
	HANDLE hIcon;
	HANDLE hProcess;
} SHELLEXECUTEINFOW,*LPSHELLEXECUTEINFOW;
typedef struct _SHFILEOPSTRUCTA {
	HWND hwnd;
	UINT wFunc;
	LPCSTR pFrom;
	LPCSTR pTo;
	FILEOP_FLAGS fFlags;
	BOOL fAnyOperationsAborted;
	PVOID hNameMappings;
	LPCSTR lpszProgressTitle;
} SHFILEOPSTRUCTA,*LPSHFILEOPSTRUCTA;
typedef struct _SHFILEOPSTRUCTW {
	HWND hwnd;
	UINT wFunc;
	LPCWSTR pFrom;
	LPCWSTR pTo;
	FILEOP_FLAGS fFlags;
	BOOL fAnyOperationsAborted;
	PVOID hNameMappings;
	LPCWSTR lpszProgressTitle;
} SHFILEOPSTRUCTW,*LPSHFILEOPSTRUCTW;
typedef struct _SHFILEINFOA {
	HICON hIcon;
	int iIcon;
	DWORD dwAttributes;
	CHAR szDisplayName[MAX_PATH];
	CHAR szTypeName[80];
} SHFILEINFOA;
typedef struct _SHFILEINFOW {
	HICON hIcon;
	int iIcon;
	DWORD dwAttributes;
	WCHAR szDisplayName[MAX_PATH];
	WCHAR szTypeName[80];
} SHFILEINFOW;
typedef struct _SHQUERYRBINFO {
	DWORD   cbSize;
	__int64 i64Size;
	__int64 i64NumItems;
} SHQUERYRBINFO, *LPSHQUERYRBINFO;
typedef struct _SHNAMEMAPPINGA {
	LPSTR	pszOldPath;
	LPSTR	pszNewPath;
	int	cchOldPath;
	int	cchNewPath;
} SHNAMEMAPPINGA, *LPSHNAMEMAPPINGA;
typedef struct _SHNAMEMAPPINGW {
	LPWSTR	pszOldPath;
	LPWSTR	pszNewPath;
	int	cchOldPath;
	int	cchNewPath;
} SHNAMEMAPPINGW, *LPSHNAMEMAPPINGW;

#define SHERB_NOCONFIRMATION 0x1
#define SHERB_NOPROGRESSUI   0x2
#define SHERB_NOSOUND        0x4

/******************************************
 * Links
 */

#define SHGNLI_PIDL        0x01
#define SHGNLI_PREFIXNAME  0x02
#define SHGNLI_NOUNIQUE    0x04
#define SHGNLI_NOLNK       0x08

LPWSTR * WINAPI CommandLineToArgvW(_In_ LPCWSTR, _Out_ int*);
void WINAPI DragAcceptFiles(_In_ HWND, _In_ BOOL);
void WINAPI DragFinish(_In_ HDROP);

_Success_(return != 0)
UINT
WINAPI
DragQueryFileA(
  _In_ HDROP hDrop,
  _In_ UINT iFile,
  _Out_writes_opt_(cch) LPSTR lpszFile,
  _In_ UINT cch);

_Success_(return != 0)
UINT
WINAPI
DragQueryFileW(
  _In_ HDROP hDrop,
  _In_ UINT iFile,
  _Out_writes_opt_(cch) LPWSTR lpszFile,
  _In_ UINT cch);

BOOL WINAPI DragQueryPoint(_In_ HDROP, _Out_ LPPOINT);

HICON
WINAPI
ExtractAssociatedIconA(
  _Reserved_ HINSTANCE hInst,
  _Inout_updates_(128) LPSTR pszIconPath,
  _Inout_ WORD *piIcon);

HICON
WINAPI
ExtractAssociatedIconW(
  _Reserved_ HINSTANCE hInst,
  _Inout_updates_(128) LPWSTR pszIconPath,
  _Inout_ WORD *piIcon);

HICON
WINAPI
ExtractIconA(
  _Reserved_ HINSTANCE hInst,
  _In_ LPCSTR pszExeFileName,
  UINT nIconIndex);

HICON
WINAPI
ExtractIconW(
  _Reserved_ HINSTANCE hInst,
  _In_ LPCWSTR pszExeFileName,
  UINT nIconIndex);

UINT
WINAPI
ExtractIconExA(
  _In_ LPCSTR lpszFile,
  _In_ int nIconIndex,
  _Out_writes_opt_(nIcons) HICON *phiconLarge,
  _Out_writes_opt_(nIcons) HICON *phiconSmall,
  _In_ UINT nIcons);

UINT
WINAPI
ExtractIconExW(
  _In_ LPCWSTR lpszFile,
  _In_ int nIconIndex,
  _Out_writes_opt_(nIcons) HICON *phiconLarge,
  _Out_writes_opt_(nIcons) HICON *phiconSmall,
  _In_ UINT nIcons);

_Success_(return > 32)
HINSTANCE
WINAPI
FindExecutableA(
  _In_ LPCSTR lpFile,
  _In_opt_ LPCSTR lpDirectory,
  _Out_writes_(MAX_PATH) LPSTR lpResult);

_Success_(return > 32)
HINSTANCE
WINAPI
FindExecutableW(
  _In_ LPCWSTR lpFile,
  _In_opt_ LPCWSTR lpDirectory,
  _Out_writes_(MAX_PATH) LPWSTR lpResult);

UINT_PTR WINAPI SHAppBarMessage(_In_ DWORD, _Inout_ PAPPBARDATA);
BOOL WINAPI Shell_NotifyIconA(_In_ DWORD, _In_ PNOTIFYICONDATAA);
BOOL WINAPI Shell_NotifyIconW(_In_ DWORD, _In_ PNOTIFYICONDATAW);

int
WINAPI
ShellAboutA(
  _In_opt_ HWND hWnd,
  _In_ LPCSTR szApp,
  _In_opt_ LPCSTR szOtherStuff,
  _In_opt_ HICON hIcon);

int
WINAPI
ShellAboutW(
  _In_opt_ HWND hWnd,
  _In_ LPCWSTR szApp,
  _In_opt_ LPCWSTR szOtherStuff,
  _In_opt_ HICON hIcon);

int
WINAPIV
ShellMessageBoxA(
  _In_opt_ HINSTANCE hAppInst,
  _In_opt_ HWND hWnd,
  _In_ LPCSTR lpcText,
  _In_opt_ LPCSTR lpcTitle,
  _In_ UINT fuStyle,
  ...);

int
WINAPIV
ShellMessageBoxW(
  _In_opt_ HINSTANCE hAppInst,
  _In_opt_ HWND hWnd,
  _In_ LPCWSTR lpcText,
  _In_opt_ LPCWSTR lpcTitle,
  _In_ UINT fuStyle,
  ...);

HINSTANCE
WINAPI
ShellExecuteA(
  _In_opt_ HWND hwnd,
  _In_opt_ LPCSTR lpOperation,
  _In_ LPCSTR lpFile,
  _In_opt_ LPCSTR lpParameters,
  _In_opt_ LPCSTR lpDirectory,
  _In_ INT nShowCmd);

HINSTANCE
WINAPI
ShellExecuteW(
  _In_opt_ HWND hwnd,
  _In_opt_ LPCWSTR lpOperation,
  _In_ LPCWSTR lpFile,
  _In_opt_ LPCWSTR lpParameters,
  _In_opt_ LPCWSTR lpDirectory,
  _In_ INT nShowCmd);

BOOL WINAPI ShellExecuteExA(_Inout_ LPSHELLEXECUTEINFOA);
BOOL WINAPI ShellExecuteExW(_Inout_ LPSHELLEXECUTEINFOW);
int WINAPI SHFileOperationA(_Inout_ LPSHFILEOPSTRUCTA);
int WINAPI SHFileOperationW(_Inout_ LPSHFILEOPSTRUCTW);
void WINAPI SHFreeNameMappings(_In_opt_ HANDLE);

DWORD_PTR
WINAPI
SHGetFileInfoA(
  _In_ LPCSTR pszPath,
  DWORD dwFileAttributes,
  _Inout_updates_bytes_opt_(cbFileInfo) SHFILEINFOA *psfi,
  UINT cbFileInfo,
  UINT uFlags);

DWORD_PTR
WINAPI
SHGetFileInfoW(
  _In_ LPCWSTR pszPath,
  DWORD dwFileAttributes,
  _Inout_updates_bytes_opt_(cbFileInfo) SHFILEINFOW *psfi,
  UINT cbFileInfo,
  UINT uFlags);

_Success_(return != 0)
BOOL
WINAPI
SHGetNewLinkInfoA(
  _In_ LPCSTR pszLinkTo,
  _In_ LPCSTR pszDir,
  _Out_writes_(MAX_PATH) LPSTR pszName,
  _Out_ BOOL *pfMustCopy,
  _In_ UINT uFlags);

_Success_(return != 0)
BOOL
WINAPI
SHGetNewLinkInfoW(
  _In_ LPCWSTR pszLinkTo,
  _In_ LPCWSTR pszDir,
  _Out_writes_(MAX_PATH) LPWSTR pszName,
  _Out_ BOOL *pfMustCopy,
  _In_ UINT uFlags);

HRESULT
WINAPI
SHQueryRecycleBinA(
  _In_opt_ LPCSTR pszRootPath,
  _Inout_ LPSHQUERYRBINFO pSHQueryRBInfo);

HRESULT
WINAPI
SHQueryRecycleBinW(
  _In_opt_ LPCWSTR pszRootPath,
  _Inout_ LPSHQUERYRBINFO pSHQueryRBInfo);

HRESULT
WINAPI
SHEmptyRecycleBinA(
  _In_opt_ HWND hwnd,
  _In_opt_ LPCSTR pszRootPath,
  DWORD dwFlags);

HRESULT
WINAPI
SHEmptyRecycleBinW(
  _In_opt_ HWND hwnd,
  _In_opt_ LPCWSTR pszRootPath,
  DWORD dwFlags);

BOOL WINAPI SHCreateProcessAsUserW(_Inout_ PSHCREATEPROCESSINFOW);

DWORD
WINAPI
DoEnvironmentSubstA(
    _Inout_updates_(cchSrc) LPSTR pszSrc,
    UINT cchSrc);

DWORD
WINAPI
DoEnvironmentSubstW(
    _Inout_updates_(cchSrc) LPWSTR pszSrc,
    UINT cchSrc);

#if (_WIN32_IE >= 0x0601)
BOOL
WINAPI
SHTestTokenMembership(
    _In_opt_ HANDLE hToken,
    _In_ ULONG ulRID);
#endif

#ifdef UNICODE
#define NOTIFYICONDATA_V1_SIZE NOTIFYICONDATAW_V1_SIZE
#define NOTIFYICONDATA_V2_SIZE NOTIFYICONDATAW_V2_SIZE
#define NOTIFYICONDATA_V3_SIZE NOTIFYICONDATAW_V3_SIZE
typedef NOTIFYICONDATAW NOTIFYICONDATA,*PNOTIFYICONDATA;
typedef DRAGINFOW DRAGINFO,*LPDRAGINFO;
typedef SHELLEXECUTEINFOW SHELLEXECUTEINFO,*LPSHELLEXECUTEINFO;
typedef SHFILEOPSTRUCTW SHFILEOPSTRUCT,*LPSHFILEOPSTRUCT;
typedef SHFILEINFOW SHFILEINFO;
typedef SHNAMEMAPPINGW SHNAMEMAPPING;
typedef LPSHNAMEMAPPINGW LPSHNAMEMAPPING;
#define DragQueryFile DragQueryFileW
#define ExtractAssociatedIcon ExtractAssociatedIconW
#define ExtractIcon ExtractIconW
#define ExtractIconEx ExtractIconExW
#define FindExecutable FindExecutableW
#define Shell_NotifyIcon Shell_NotifyIconW
#define ShellAbout ShellAboutW
#define ShellExecute ShellExecuteW
#define ShellExecuteEx ShellExecuteExW
#define ShellMessageBox ShellMessageBoxW
#define SHFileOperation SHFileOperationW
#define SHGetFileInfo SHGetFileInfoW
#define SHQueryRecycleBin SHQueryRecycleBinW
#define SHEmptyRecycleBin SHEmptyRecycleBinW
#define SHGetNewLinkInfo SHGetNewLinkInfoW
#define DoEnvironmentSubst DoEnvironmentSubstW

#else
#define NOTIFYICONDATA_V1_SIZE NOTIFYICONDATAA_V1_SIZE
#define NOTIFYICONDATA_V2_SIZE NOTIFYICONDATAA_V2_SIZE
#define NOTIFYICONDATA_V3_SIZE NOTIFYICONDATAA_V3_SIZE
typedef NOTIFYICONDATAA NOTIFYICONDATA,*PNOTIFYICONDATA;
typedef DRAGINFOA DRAGINFO,*LPDRAGINFO;
typedef SHELLEXECUTEINFOA SHELLEXECUTEINFO,*LPSHELLEXECUTEINFO;
typedef SHFILEOPSTRUCTA SHFILEOPSTRUCT,*LPSHFILEOPSTRUCT;
typedef SHFILEINFOA SHFILEINFO;
typedef SHNAMEMAPPINGA SHNAMEMAPPING;
typedef LPSHNAMEMAPPINGA LPSHNAMEMAPPING;
#define DragQueryFile DragQueryFileA
#define ExtractAssociatedIcon ExtractAssociatedIconA
#define ExtractIcon ExtractIconA
#define ExtractIconEx ExtractIconExA
#define FindExecutable FindExecutableA
#define Shell_NotifyIcon Shell_NotifyIconA
#define ShellAbout ShellAboutA
#define ShellExecute ShellExecuteA
#define ShellExecuteEx ShellExecuteExA
#define ShellMessageBox ShellMessageBoxA
#define SHFileOperation SHFileOperationA
#define SHGetFileInfo SHGetFileInfoA
#define SHQueryRecycleBin SHQueryRecycleBinA
#define SHEmptyRecycleBin SHEmptyRecycleBinA
#define SHGetNewLinkInfo SHGetNewLinkInfoA
#define DoEnvironmentSubst DoEnvironmentSubstA
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)

typedef struct _SHSTOCKICONINFO {
  DWORD cbSize;
  HICON hIcon;
  int iSysImageIndex;
  int iIcon;
  WCHAR szPath[MAX_PATH];
} SHSTOCKICONINFO;

#define SHGSI_ICONLOCATION 0

typedef enum SHSTOCKICONID
{
  SIID_INVALID=-1,
  SIID_DOCNOASSOC,
  SIID_DOCASSOC,
  SIID_APPLICATION,
  SIID_FOLDER,
  SIID_FOLDEROPEN,
  SIID_DRIVE525,
  SIID_DRIVE35,
  SIID_DRIVERREMOVE,
  SIID_DRIVERFIXED,
  SIID_DRIVERNET,
  SIID_DRIVERNETDISABLE,
  SIID_DRIVERCD,
  SIID_DRIVERRAM,
  SIID_WORLD,
  /* Missing: 14 */
  SIID_SERVER = 15,
  SIID_PRINTER,
  SIID_MYNETWORK,
  /* Missing: 18 - 21 */
  SIID_FIND = 22,
  SIID_HELP,
  /* Missing: 24 - 27 */
  SIID_SHARE = 28,
  SIID_LINK,
  SIID_SLOWFILE,
  SIID_RECYCLER,
  SIID_RECYCLERFULL,
  /* Missing: 33 - 39 */
  SIID_MEDIACDAUDIO = 40,
  /* Missing: 41 - 46 */
  SIID_LOCK = 47,
  /* Missing: 48 */
  SIID_AUTOLIST = 49,
  SIID_PRINTERNET,
  SIID_SERVERSHARE,
  SIID_PRINTERFAX,
  SIID_PRINTERFAXNET,
  SIID_PRINTERFILE,
  SIID_STACK,
  SIID_MEDIASVCD,
  SIID_STUFFEDFOLDER,
  SIID_DRIVEUNKNOWN,
  SIID_DRIVEDVD,
  SIID_MEDIADVD,
  SIID_MEDIADVDRAM,
  SIID_MEDIADVDRW,
  SIID_MEDIADVDR,
  SIID_MEDIADVDROM,
  SIID_MEDIACDAUDIOPLUS,
  SIID_MEDIACDRW,
  SIID_MEDIACDR,
  SIID_MEDIACDBURN,
  SIID_MEDIABLANKCD,
  SIID_MEDIACDROM,
  SIID_AUDIOFILES,
  SIID_IMAGEFILES,
  SIID_VIDEOFILES,
  SIID_MIXEDFILES,
  SIID_FOLDERBACK,
  SIID_FOLDERFRONT,
  SIID_SHIELD,
  SIID_WARNING,
  SIID_INFO,
  SIID_ERROR,
  SIID_KEY,
  SIID_SOFTWARE,
  SIID_RENAME,
  SIID_DELETE,
  SIID_MEDIAAUDIODVD,
  SIID_MEDIAMOVIEDVD,
  SIID_MEDIAENHANCEDCD,
  SIID_MEDIAENHANCEDDVD,
  SIID_MEDIAHDDVD,
  SIID_MEDIABLUERAY,
  SIID_MEDIAVCD,
  SIID_MEDIADVDPLUSR,
  SIID_MEDIADVDPLUSRW,
  SIID_DESKTOPPC,
  SIID_MOBILEPC,
  SIID_USERS,
  SIID_MEDIASMARTMEDIA,
  SIID_MEDIACOMPACTFLASH,
  SIID_DEVICECELLPHONE,
  SIID_DEVICECAMERA,
  SIID_DEVICEVIDEOCAMERA,
  SIID_DEVICEAUDIOPLAYER,
  SIID_NETWORKCONNECT,
  SIID_INTERNET,
  SIID_ZIPFILE,
  SIID_SETTINGS,
  /* Missing: 107 - 131 */
  SIID_DRIVEHDDVD = 132,
  SIID_DRIVEBD,
  SIID_MEDIAHDDVDROM,
  SIID_MEDIAHDDVDR,
  SIID_MEDIAHDDVDRAM,
  SIID_MEDIABDROM,
  SIID_MEDIABDR,
  SIID_MEDIABDRE,
  SIID_CLUSTEREDDRIVE,
  /* Missing: 141 - 174 */
  SIID_MAX_ICONS = 175
} SHSTOCKICONID;

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if !defined(_WIN64)
#include <poppack.h>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef __cplusplus
}
#endif
#endif
