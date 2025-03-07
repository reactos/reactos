/*
 * Copyright 1999, 2000 Juergen Schmied
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
 */

#ifndef __WINE_UNDOCSHELL_H
#define __WINE_UNDOCSHELL_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#if (NTDDI_VERSION < NTDDI_LONGHORN)
#define DBIMF_NOGRIPPER         0x0800
#define DBIMF_ALWAYSGRIPPER     0x1000
#define DBIMF_NOMARGINS         0x2000
#endif  // NTDDI_LONGHORN

#if defined (_SHELLAPI_H) || defined (_INC_SHELLAPI)

/****************************************************************************
 * Taskbar interface WM_COPYDATA structures
 * See http://www.geoffchappell.com/studies/windows/shell/shell32/api/shlnot/copydata.htm
 */
/* Data structure for Shell_NotifyIcon messages */
typedef struct _TRAYNOTIFYDATAW
{
    DWORD dwSignature;
    DWORD dwMessage;
    NOTIFYICONDATAW nid; // Always use the latest NOTIFYICONDATAW structure version.
} TRAYNOTIFYDATAW, *PTRAYNOTIFYDATAW;
// Note: One could also introduce TRAYNOTIFYDATAA

#define NI_NOTIFY_SIG 0x34753423 /* TRAYNOTIFYDATA */

#endif /* defined (_SHELLAPI_H) || defined (_INC_SHELLAPI) */

/****************************************************************************
 * Taskbar WM_COMMAND identifiers
 */
#define TWM_DOEXITWINDOWS (WM_USER + 342)
#define TWM_CYCLEFOCUS (WM_USER + 348)

/****************************************************************************
 * ProgMan messages
 */
#define WM_PROGMAN_OPENSHELLSETTINGS (WM_USER + 22) /* wParam specifies the dialog (and tab page) */
#define WM_PROGMAN_SAVESTATE         (WM_USER + 77)

/****************************************************************************
 *  IDList Functions
 */
BOOL WINAPI ILGetDisplayName(
    LPCITEMIDLIST pidl,
    LPVOID path);

/* type parameter for ILGetDisplayNameEx() */
#define ILGDN_FORPARSING  0
#define ILGDN_NORMAL      1
#define ILGDN_INFOLDER    2

BOOL WINAPI ILGetDisplayNameEx(
    LPSHELLFOLDER psf,
    LPCITEMIDLIST pidl,
    LPVOID path,
    DWORD type);

LPITEMIDLIST WINAPI ILGlobalClone(LPCITEMIDLIST pidl);
void WINAPI ILGlobalFree(LPITEMIDLIST pidl);
LPITEMIDLIST WINAPI SHSimpleIDListFromPathA (LPCSTR lpszPath); //FIXME
LPITEMIDLIST WINAPI SHSimpleIDListFromPathW (LPCWSTR lpszPath);

HRESULT WINAPI SHILCreateFromPathA (
    LPCSTR path,
    LPITEMIDLIST * ppidl,
    DWORD *attributes);

HRESULT WINAPI SHILCreateFromPathW (
    LPCWSTR path,
    LPITEMIDLIST * ppidl,
    DWORD *attributes);

HRESULT WINAPI SHInvokeCommand(
    HWND hWnd,
    IShellFolder* lpFolder,
    LPCITEMIDLIST lpApidl,
    LPCSTR lpVerb);
HRESULT WINAPI SHInvokeCommandOnContextMenu(
    _In_opt_ HWND hWnd,
    _In_opt_ IUnknown* pUnk,
    _In_ IContextMenu* pCM,
    _In_ UINT fCMIC,
    _In_opt_ LPCSTR pszVerb);
BOOL WINAPI IContextMenu_Invoke(
    _In_ IContextMenu *pContextMenu,
    _In_ HWND hwnd,
    _In_ LPCSTR lpVerb,
    _In_ UINT uFlags);

/*
    string functions
*/
BOOL WINAPI StrRetToStrNA(LPSTR,DWORD,LPSTRRET,const ITEMIDLIST*);
BOOL WINAPI StrRetToStrNW(LPWSTR,DWORD,LPSTRRET,const ITEMIDLIST*);

/****************************************************************************
 * SHChangeNotifyRegister API
 */
#define SHCNRF_InterruptLevel       0x0001
#define SHCNRF_ShellLevel           0x0002
#define SHCNRF_RecursiveInterrupt   0x1000  /* Must be combined with SHCNRF_InterruptLevel */
#define SHCNRF_NewDelivery          0x8000  /* Messages use shared memory */

/****************************************************************************
 * SHChangeNotify
 */

typedef struct _SHCNF_PRINTJOB_INFO
{
    DWORD JobId;
    // More info,,,
} SHCNF_PRINTJOB_INFO, *PSHCNF_PRINTJOB_INFO;

//
// Add missing types for print job notifications.
//
#define SHCNF_PRINTJOBA 0x0004
#define SHCNF_PRINTJOBW 0x0007

HRESULT WINAPI SHUpdateRecycleBinIcon(void);

/****************************************************************************
 * Shell Common Dialogs
 */

/* RunFileDlg flags */
#define RFF_NOBROWSE        0x01    /* Removes the browse button */
#define RFF_NODEFAULT       0x02    /* No default item selected */
#define RFF_CALCDIRECTORY   0x04    /* Calculates the working directory from the file name */
#define RFF_NOLABEL         0x08    /* Removes the edit box label */
#define RFF_NOSEPARATEMEM   0x20    /* Removes the Separate Memory Space check box (Windows NT only) */

/* RunFileFlg notification value and structure */
#define RFN_VALIDATE    (-510)
#if 0 // Deprecated ANSI structure
typedef struct _NMRUNFILEDLGA
{
    NMHDR   hdr;
    LPCSTR  lpFile;
    LPCSTR  lpDirectory;
    UINT    nShow;
} NMRUNFILEDLGA, *PNMRUNFILEDLGA, *LPNMRUNFILEDLGA;
#endif
typedef struct _NMRUNFILEDLGW
{
    NMHDR   hdr;
    LPCWSTR lpFile;
    LPCWSTR lpDirectory;
    UINT    nShow;
} NMRUNFILEDLGW, *PNMRUNFILEDLGW, *LPNMRUNFILEDLGW;

typedef NMRUNFILEDLGW NMRUNFILEDLG;
typedef PNMRUNFILEDLGW PNMRUNFILEDLG;
typedef LPNMRUNFILEDLGW LPNMRUNFILEDLG;

/* RunFileDlg notification return values */
#define RF_OK      0x00
#define RF_CANCEL  0x01
#define RF_RETRY   0x02

void WINAPI RunFileDlg(
    HWND hWndOwner,
    HICON hIcon,
    LPCWSTR lpstrDirectory,
    LPCWSTR lpstrTitle,
    LPCWSTR lpstrDescription,
    UINT uFlags);

int WINAPI LogoffWindowsDialog(HWND hWndOwner);
void WINAPI ExitWindowsDialog(HWND hWndOwner);

BOOL WINAPI SHFindComputer(
    LPCITEMIDLIST pidlRoot,
    LPCITEMIDLIST pidlSavedSearch);

void WINAPI SHHandleDiskFull(HWND hwndOwner,
    UINT uDrive);

int  WINAPI SHOutOfMemoryMessageBox(
    HWND hwndOwner,
    LPCSTR lpCaption,
    UINT uType);

HRESULT WINAPI SHShouldShowWizards(_In_ IUnknown *pUnknown);

DWORD WINAPI SHNetConnectionDialog(
    HWND hwndOwner,
    LPCWSTR lpstrRemoteName,
    DWORD dwType);

BOOL WINAPI SHIsTempDisplayMode(VOID);

HRESULT WINAPI
SHGetUserDisplayName(
    _Out_writes_to_(*puSize, *puSize) PWSTR pName,
    _Inout_ PULONG puSize);

/****************************************************************************
 * Cabinet Window Messages
 */

#define CWM_SETPATH           (WM_USER + 2)
#define CWM_WANTIDLE          (WM_USER + 3)
#define CWM_GETSETCURRENTINFO (WM_USER + 4)
#define CWM_SELECTITEM        (WM_USER + 5)
#define CWM_SELECTITEMSTR     (WM_USER + 6)
#define CWM_GETISHELLBROWSER  (WM_USER + 7)
#define CWM_TESTPATH          (WM_USER + 9)
#define CWM_STATECHANGE       (WM_USER + 10)
#define CWM_GETPATH           (WM_USER + 12)

#define WM_GETISHELLBROWSER CWM_GETISHELLBROWSER

/* CWM_TESTPATH types */
#define CWTP_ISEQUAL  0
#define CWTP_ISCHILD  1

/* CWM_TESTPATH structure */
typedef struct
{
    DWORD dwType;
    ITEMIDLIST idl;
} CWTESTPATHSTRUCT,* LPCWTESTPATHSTRUCT;

/****************************************************************************
 * System Imagelist Routines
 */

int WINAPI Shell_GetCachedImageIndexA(
    LPCSTR lpszFileName,
    int nIconIndex,
    UINT bSimulateDoc);

BOOL WINAPI Shell_GetImageLists(
    HIMAGELIST *lphimlLarge,
    HIMAGELIST *lphimlSmall);

HICON WINAPI SHGetFileIcon(
    DWORD dwReserved,
    LPCSTR lpszPath,
    DWORD dwFileAttributes,
    UINT uFlags);

BOOL WINAPI FileIconInit(BOOL bFullInit);

WORD WINAPI
ExtractIconResInfoA(
    _In_ HANDLE hHandle,
    _In_ LPCSTR lpFileName,
    _In_ WORD wIndex,
    _Out_ LPWORD lpSize,
    _Out_ LPHANDLE lpIcon);

WORD WINAPI
ExtractIconResInfoW(
    _In_ HANDLE hHandle,
    _In_ LPCWSTR lpFileName,
    _In_ WORD wIndex,
    _Out_ LPWORD lpSize,
    _Out_ LPHANDLE lpIcon);

/****************************************************************************
 * File Menu Routines
 */

/* FileMenu_Create nSelHeight constants */
#define FM_DEFAULT_SELHEIGHT  -1
#define FM_FULL_SELHEIGHT     0

/* FileMenu_Create flags */
#define FMF_SMALL_ICONS      0x00
#define FMF_LARGE_ICONS      0x08
#define FMF_NO_COLUMN_BREAK  0x10

HMENU WINAPI FileMenu_Create(
    COLORREF crBorderColor,
    int nBorderWidth,
    HBITMAP hBorderBmp,
    int nSelHeight,
    UINT uFlags);

void WINAPI FileMenu_Destroy(HMENU hMenu);

/* FileMenu_AppendItem constants */
#define FM_SEPARATOR       (LPCSTR)1
#define FM_BLANK_ICON      -1
#define FM_DEFAULT_HEIGHT  0

BOOL WINAPI FileMenu_AppendItem(
    HMENU hMenu,
    LPCSTR lpszText,
    UINT uID,
    int iIcon,
    HMENU hMenuPopup,
    int nItemHeight);

/* FileMenu_InsertUsingPidl flags */
#define FMF_NO_EMPTY_ITEM      0x01
#define FMF_NO_PROGRAM_GROUPS  0x04

/* FileMenu_InsertUsingPidl callback function */
typedef void (CALLBACK *LPFNFMCALLBACK)(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlFile);

int WINAPI FileMenu_InsertUsingPidl(
    HMENU hMenu,
    UINT uID,
    LPCITEMIDLIST pidl,
    UINT uFlags,
    UINT uEnumFlags,
    LPFNFMCALLBACK lpfnCallback);

int WINAPI FileMenu_ReplaceUsingPidl(
    HMENU hMenu,
    UINT uID,
    LPCITEMIDLIST pidl,
    UINT uEnumFlags,
    LPFNFMCALLBACK lpfnCallback);

void WINAPI FileMenu_Invalidate(HMENU hMenu);

HMENU WINAPI FileMenu_FindSubMenuByPidl(
    HMENU hMenu,
    LPCITEMIDLIST pidl);

BOOL WINAPI FileMenu_TrackPopupMenuEx(
    HMENU hMenu,
    UINT uFlags,
    int x,
    int y,
    HWND hWnd,
    LPTPMPARAMS lptpm);

BOOL WINAPI FileMenu_GetLastSelectedItemPidls(
    UINT uReserved,
    LPCITEMIDLIST *ppidlFolder,
    LPCITEMIDLIST *ppidlItem);

LRESULT WINAPI FileMenu_MeasureItem(
    HWND hWnd,
    LPMEASUREITEMSTRUCT lpmis);

LRESULT WINAPI FileMenu_DrawItem(
    HWND hWnd,
    LPDRAWITEMSTRUCT lpdis);

BOOL WINAPI FileMenu_InitMenuPopup(HMENU hMenu);

void WINAPI FileMenu_AbortInitMenu(void);

LRESULT WINAPI FileMenu_HandleMenuChar(
    HMENU hMenu,
    WPARAM wParam);

BOOL WINAPI FileMenu_DeleteAllItems(HMENU hMenu);

BOOL WINAPI FileMenu_DeleteItemByCmd(
    HMENU hMenu,
    UINT uID);

BOOL WINAPI FileMenu_DeleteItemByIndex(
    HMENU hMenu,
    UINT uPos);

BOOL WINAPI FileMenu_DeleteMenuItemByFirstID(
    HMENU hMenu,
    UINT uID);

BOOL WINAPI FileMenu_DeleteSeparator(HMENU hMenu);

BOOL WINAPI FileMenu_EnableItemByCmd(
    HMENU hMenu,
    UINT uID,
    BOOL bEnable);

DWORD WINAPI FileMenu_GetItemExtent(
    HMENU hMenu,
    UINT uPos);

int WINAPI FileMenu_AppendFilesForPidl(
    HMENU hMenu,
    LPCITEMIDLIST pidl,
    BOOL bAddSeparator);

int WINAPI FileMenu_AddFilesForPidl(
    HMENU hMenu,
    UINT uReserved,
    UINT uID,
    LPCITEMIDLIST pidl,
    UINT uFlags,
    UINT uEnumFlags,
    LPFNFMCALLBACK lpfnCallback);

/****************************************************************************
 * Drag And Drop Routines
 */

HRESULT WINAPI SHRegisterDragDrop(
    HWND hWnd,
    LPDROPTARGET lpDropTarget);

HRESULT WINAPI SHRevokeDragDrop(HWND hWnd);

BOOL WINAPI DAD_DragEnter(HWND hWnd);

BOOL WINAPI DAD_SetDragImageFromListView(
    HWND hWnd,
    POINT pt);

BOOL WINAPI DAD_ShowDragImage(BOOL bShow);

/****************************************************************************
 * Path Manipulation Routines
 */

BOOL WINAPI PathAppendAW(LPVOID lpszPath1, LPCVOID lpszPath2);

LPVOID WINAPI PathCombineAW(LPVOID szDest, LPCVOID lpszDir, LPCVOID lpszFile);

LPVOID  WINAPI PathAddBackslashAW(LPVOID path);

LPVOID WINAPI PathBuildRootAW(LPVOID lpszPath, int drive);

LPVOID WINAPI PathFindExtensionAW(LPCVOID path);

LPVOID WINAPI PathFindFileNameAW(LPCVOID path);

LPVOID WINAPI PathGetExtensionAW(LPCVOID lpszPath,  DWORD void1, DWORD void2);

LPVOID WINAPI PathGetArgsAW(LPVOID lpszPath);

BOOL WINAPI PathRemoveFileSpecAW(LPVOID lpszPath);

void WINAPI PathRemoveBlanksAW(LPVOID lpszPath);

VOID  WINAPI PathQuoteSpacesAW(LPVOID path);

void WINAPI PathUnquoteSpacesAW(LPVOID lpszPath);

BOOL WINAPI PathIsUNCAW(LPCVOID lpszPath);

BOOL WINAPI PathIsRelativeAW(LPCVOID lpszPath);

BOOL WINAPI PathIsRootAW(LPCVOID x);

BOOL WINAPI PathIsExeAW(LPCVOID lpszPath);

BOOL WINAPI PathIsDirectoryAW(LPCVOID lpszPath);

BOOL WINAPI PathFileExistsAW(LPCVOID lpszPath);

BOOL WINAPI PathMatchSpecAW(LPVOID lpszPath, LPVOID lpszSpec);

BOOL WINAPI PathMakeUniqueNameAW(
    LPVOID lpszBuffer,
    DWORD dwBuffSize,
    LPCVOID lpszShortName,
    LPCVOID lpszLongName,
    LPCVOID lpszPathName);

BOOL WINAPI PathYetAnotherMakeUniqueName(
    LPWSTR lpszBuffer,
    LPCWSTR lpszPathName,
    LPCWSTR lpszShortName,
    LPCWSTR lpszLongName);

VOID WINAPI PathQualifyA(LPSTR pszPath);
VOID WINAPI PathQualifyW(LPWSTR pszPath);
VOID WINAPI PathQualifyAW(LPVOID path);

/* PathResolve flags */
#define PRF_CHECKEXISTANCE  0x01
#define PRF_EXECUTABLE      0x02
#define PRF_QUALIFYONPATH   0x04
#define PRF_WINDOWS31       0x08

BOOL WINAPI PathResolveA(LPSTR path, LPCSTR *dirs, DWORD flags);
BOOL WINAPI PathResolveW(LPWSTR path, LPCWSTR *dirs, DWORD flags);
BOOL WINAPI PathResolveAW(LPVOID lpszPath, LPCVOID *alpszPaths, DWORD dwFlags);

VOID WINAPI PathSetDlgItemPathAW(HWND hDlg, int nIDDlgItem, LPCVOID lpszPath);

/* PathProcessCommand flags */
#define PPCF_QUOTEPATH        0x01 /* implies PPCF_INCLUDEARGS */
#define PPCF_INCLUDEARGS      0x02
//#define PPCF_NODIRECTORIES    0x10 move to shlobj
#define PPCF_DONTRESOLVE      0x20
#define PPCF_PATHISRELATIVE   0x40

HRESULT WINAPI PathProcessCommandAW(LPCVOID lpszPath, LPVOID lpszBuff,
                                    DWORD dwBuffSize, DWORD dwFlags);

void WINAPI PathStripPathAW(LPVOID lpszPath);

BOOL WINAPI PathStripToRootAW(LPVOID lpszPath);

void WINAPI PathRemoveArgsAW(LPVOID lpszPath);

void WINAPI PathRemoveExtensionAW(LPVOID lpszPath);

int WINAPI PathParseIconLocationAW(LPVOID lpszPath);

BOOL WINAPI PathIsSameRootAW(LPCVOID lpszPath1, LPCVOID lpszPath2);

BOOL WINAPI PathFindOnPathAW(LPVOID sFile, LPCVOID *sOtherDirs);

BOOL WINAPI PathIsEqualOrSubFolder(_In_ LPCWSTR pszFile1OrCSIDL, _In_ LPCWSTR pszFile2);

BOOL WINAPI PathIsTemporaryA(_In_ LPCSTR Str);
BOOL WINAPI PathIsTemporaryW(_In_ LPCWSTR Str);

/****************************************************************************
 * Shell File Operations error codes - SHFileOperationA/W
 */

/* Error codes could be pre-Win32 */
#define DE_SAMEFILE         0x71
#define DE_MANYSRC1DEST     0x72
#define DE_DIFFDIR          0x73
#define DE_ROOTDIR          0x74
#define DE_OPCANCELLED      0x75
#define DE_DESTSUBTREE      0x76
#define DE_ACCESSDENIEDSRC  0x78
#define DE_PATHTOODEEP      0x79
#define DE_MANYDEST         0x7A
#define DE_INVALIDFILES     0x7C
#define DE_DESTSAMETREE     0x7D
#define DE_FLDDESTISFILE    0x7E
#define DE_FILEDESTISFLD    0x80
#define DE_FILENAMETOOLONG  0x81
#define DE_DEST_IS_CDROM    0x82
#define DE_DEST_IS_DVD      0x83
#define DE_DEST_IS_CDRECORD 0x84
#define DE_FILE_TOO_LARGE   0x85
#define DE_SRC_IS_CDROM     0x86
#define DE_SRC_IS_DVD       0x87
#define DE_SRC_IS_CDRECORD  0x88
// #define DE_ERROR_MAX
#define ERRORONDEST         0x10000

/****************************************************************************
 * Shell settings
 */

typedef struct _REGSHELLSTATE
{
    DWORD dwSize;
    SHELLSTATE ss;
} REGSHELLSTATE, *PREGSHELLSTATE;
#define REGSHELLSTATE_SIZE 0x24
#define REGSHELLSTATE_VERSION 0xD
C_ASSERT(sizeof(REGSHELLSTATE) == REGSHELLSTATE_SIZE);

/****************************************************************************
 * Shell Namespace Routines
 */

/* Generic structure used by several messages */
typedef struct
{
  DWORD          dwReserved;
  DWORD          dwReserved2;
  LPCITEMIDLIST  pidl;
  LPDWORD        lpdwUser;
} SFVCBINFO, * LPSFVCBINFO;
typedef const SFVCBINFO * LPCSFVCBINFO;

/* SFVCB_SELECTIONCHANGED structure */
typedef struct
{
  UINT           uOldState;
  UINT           uNewState;
  LPCITEMIDLIST  pidl;
  LPDWORD        lpdwUser;
} SFVSELECTSTATE, * LPSFVSELECTSTATE;
typedef const SFVSELECTSTATE * LPCSFVSELECTSTATE;

/* SFVCB_COPYHOOKCALLBACK structure */
typedef struct
{
  HWND    hwnd;
  UINT    wFunc;
  UINT    wFlags;
  LPCSTR  pszSrcFile;
  DWORD   dwSrcAttribs;
  LPCSTR  pszDestFile;
  DWORD   dwDestAttribs;
} SFVCOPYHOOKINFO, * LPSFVCOPYHOOKINFO;
typedef const SFVCOPYHOOKINFO * LPCSFVCOPYHOOKINFO;

/* SFVCB_GETDETAILSOF structure */
typedef struct
{
  LPCITEMIDLIST  pidl;
  int            fmt;
  int            cx;
  STRRET         lpText;
} SFVCOLUMNINFO, * LPSFVCOLUMNINFO;

/****************************************************************************
 * Misc Stuff
 */

BOOL WINAPI
RegenerateUserEnvironment(LPVOID *lpEnvironment, BOOL bUpdateSelf);

/* SHWaitForFileToOpen flags */
#define SHWFF_ADD     0x01
#define SHWFF_REMOVE  0x02
#define SHWFF_WAIT    0x04

BOOL WINAPI SHWaitForFileToOpen(
    LPCITEMIDLIST pidl,
    DWORD dwFlags,
    DWORD dwTimeout);

WORD WINAPI ArrangeWindows(
    HWND hwndParent,
    DWORD dwReserved,
    LPCRECT lpRect,
    WORD cKids,
    CONST HWND * lpKids);

/* Flags for ShellExecCmdLine */
#define SECL_NO_UI          0x2
#define SECL_LOG_USAGE      0x8
#define SECL_USE_IDLIST     0x10
#define SECL_ALLOW_NONEXE   0x20
#define SECL_RUNAS          0x40

HRESULT WINAPI ShellExecCmdLine(
    HWND hwnd,
    LPCWSTR pwszCommand,
    LPCWSTR pwszStartDir,
    int nShow,
    LPVOID pUnused,
    DWORD dwSeclFlags);

HINSTANCE WINAPI
RealShellExecuteA(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCSTR lpOperation,
    _In_opt_ LPCSTR lpFile,
    _In_opt_ LPCSTR lpParameters,
    _In_opt_ LPCSTR lpDirectory,
    _In_opt_ LPSTR lpReturn,
    _In_opt_ LPCSTR lpTitle,
    _In_opt_ LPVOID lpReserved,
    _In_ INT nCmdShow,
    _Out_opt_ PHANDLE lphProcess);

HINSTANCE WINAPI
RealShellExecuteW(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCWSTR lpOperation,
    _In_opt_ LPCWSTR lpFile,
    _In_opt_ LPCWSTR lpParameters,
    _In_opt_ LPCWSTR lpDirectory,
    _In_opt_ LPWSTR lpReturn,
    _In_opt_ LPCWSTR lpTitle,
    _In_opt_ LPVOID lpReserved,
    _In_ INT nCmdShow,
    _Out_opt_ PHANDLE lphProcess);

HINSTANCE WINAPI
RealShellExecuteExA(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCSTR lpOperation,
    _In_opt_ LPCSTR lpFile,
    _In_opt_ LPCSTR lpParameters,
    _In_opt_ LPCSTR lpDirectory,
    _In_opt_ LPSTR lpReturn,
    _In_opt_ LPCSTR lpTitle,
    _In_opt_ LPVOID lpReserved,
    _In_ INT nCmdShow,
    _Out_opt_ PHANDLE lphProcess,
    _In_ DWORD dwFlags);

HINSTANCE WINAPI
RealShellExecuteExW(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCWSTR lpOperation,
    _In_opt_ LPCWSTR lpFile,
    _In_opt_ LPCWSTR lpParameters,
    _In_opt_ LPCWSTR lpDirectory,
    _In_opt_ LPWSTR lpReturn,
    _In_opt_ LPCWSTR lpTitle,
    _In_opt_ LPVOID lpReserved,
    _In_ INT nCmdShow,
    _Out_opt_ PHANDLE lphProcess,
    _In_ DWORD dwFlags);

VOID WINAPI
ShellExec_RunDLL(
    _In_opt_ HWND hwnd,
    _In_opt_ HINSTANCE hInstance,
    _In_ PCSTR pszCmdLine,
    _In_ INT nCmdShow);

VOID WINAPI
ShellExec_RunDLLA(
    _In_opt_ HWND hwnd,
    _In_opt_ HINSTANCE hInstance,
    _In_ PCSTR pszCmdLine,
    _In_ INT nCmdShow);

VOID WINAPI
ShellExec_RunDLLW(
    _In_opt_ HWND hwnd,
    _In_opt_ HINSTANCE hInstance,
    _In_ PCWSTR pszCmdLine,
    _In_ INT nCmdShow);

/* RegisterShellHook types */
#define RSH_DEREGISTER        0
#define RSH_REGISTER          1
#define RSH_REGISTER_PROGMAN  2
#define RSH_REGISTER_TASKMAN  3

BOOL WINAPI RegisterShellHook(
    HWND hWnd,
    DWORD dwType);

/* SHCreateDefClassObject callback function */
typedef HRESULT (CALLBACK *LPFNCDCOCALLBACK)(
    LPUNKNOWN pUnkOuter,
    REFIID riidObject,
    LPVOID *ppvObject);

HRESULT WINAPI SHCreateDefClassObject(
    REFIID riidFactory,
    LPVOID *ppvFactory,
    LPFNCDCOCALLBACK lpfnCallback,
    LPDWORD lpdwUsage,
    REFIID riidObject);

void WINAPI SHFreeUnusedLibraries(void);

/* SHCreateLinks flags */
#define SHCLF_PREFIXNAME       0x01
#define SHCLF_CREATEONDESKTOP  0x02

HRESULT WINAPI SHCreateLinks(
    HWND hWnd,
    LPCSTR lpszDir,
    LPDATAOBJECT lpDataObject,
    UINT uFlags,
    LPITEMIDLIST *lppidlLinks);

VOID WINAPI CheckEscapesA(LPSTR string, DWORD len);
VOID WINAPI CheckEscapesW(LPWSTR string, DWORD len);

/* policy functions */
BOOL WINAPI SHInitRestricted(LPCVOID unused, LPCVOID inpRegKey);

#define CSIDL_FOLDER_MASK   0x00ff

/* Utility functions */
#include <stdio.h>

#define SMC_EXEC 4
INT WINAPI Shell_GetCachedImageIndex(LPCWSTR szPath, INT nIndex, UINT bSimulateDoc);

HRESULT WINAPI SHCreatePropertyBag(_In_ REFIID riid, _Out_ void **ppvObj);
HRESULT WINAPI SHLimitInputCombo(HWND hWnd, IShellFolder *psf);
HRESULT WINAPI SHGetImageList(int iImageList, REFIID riid, void **ppv);

BOOL WINAPI GUIDFromStringA(
    _In_   PCSTR psz,
    _Out_  LPGUID pguid);
BOOL WINAPI GUIDFromStringW(
    _In_   PCWSTR psz,
    _Out_  LPGUID pguid);

PSTR WINAPI
StrRStrA(
    _In_ PCSTR pszSrc,
    _In_opt_ PCSTR pszLast,
    _In_ PCSTR pszSearch);

PWSTR WINAPI
StrRStrW(
    _In_ PCWSTR pszSrc,
    _In_opt_ PCWSTR pszLast,
    _In_ PCWSTR pszSearch);

LPSTR WINAPI SheRemoveQuotesA(LPSTR psz);
LPWSTR WINAPI SheRemoveQuotesW(LPWSTR psz);

/* Flags for Int64ToString and LargeIntegerToString */
#define FMT_USE_NUMDIGITS 0x01
#define FMT_USE_LEADZERO  0x02
#define FMT_USE_GROUPING  0x04
#define FMT_USE_DECIMAL   0x08
#define FMT_USE_THOUSAND  0x10
#define FMT_USE_NEGNUMBER 0x20

INT WINAPI
Int64ToString(
    _In_ LONGLONG llValue,
    _Out_writes_z_(cchOut) LPWSTR pszOut,
    _In_ UINT cchOut,
    _In_ BOOL bUseFormat,
    _In_opt_ const NUMBERFMTW *pNumberFormat,
    _In_ DWORD dwNumberFlags);

INT WINAPI
LargeIntegerToString(
    _In_ const LARGE_INTEGER *pLargeInt,
    _Out_writes_z_(cchOut) LPWSTR pszOut,
    _In_ UINT cchOut,
    _In_ BOOL bUseFormat,
    _In_opt_ const NUMBERFMTW *pNumberFormat,
    _In_ DWORD dwNumberFlags);

LPWSTR WINAPI
ShortSizeFormatW(
    _In_ DWORD dwNumber,
    _Out_writes_(0x8FFF) LPWSTR pszBuffer);

BOOL WINAPI SHOpenEffectiveToken(_Out_ LPHANDLE phToken);
DWORD WINAPI SHGetUserSessionId(_In_opt_ HANDLE hToken);

typedef HRESULT (CALLBACK *PRIVILEGED_FUNCTION)(LPARAM lParam);

HRESULT WINAPI
SHInvokePrivilegedFunctionW(
    _In_z_ LPCWSTR pszName,
    _In_ PRIVILEGED_FUNCTION fn,
    _In_opt_ LPARAM lParam);

BOOL WINAPI
SHTestTokenPrivilegeW(_In_opt_ HANDLE hToken, _In_z_ LPCWSTR lpName);
BOOL WINAPI IsSuspendAllowed(VOID);

BOOL WINAPI
Activate_RunDLL(
    _In_ HWND hwnd,
    _In_ HINSTANCE hinst,
    _In_ LPCWSTR cmdline,
    _In_ INT cmdshow);

BOOL WINAPI SHSettingsChanged(LPCVOID unused, LPCWSTR pszKey);

/*****************************************************************************
 * Shell32 resources
 */
// these resources are in shell32.dll
#define IDB_GOBUTTON_NORMAL         0x0e6
#define IDB_GOBUTTON_HOT            0x0e7

// band ids in internet toolbar
#define ITBBID_MENUBAND             1
#define ITBBID_BRANDBAND            5
#define ITBBID_TOOLSBAND            2
#define ITBBID_ADDRESSBAND          4

// commands in the CGID_PrivCITCommands command group handled by the internet toolbar
// there seems to be some support for hiding the menubar and an auto hide feature that are
// unavailable in the UI
#define ITID_TEXTLABELS             3
#define ITID_TOOLBARBANDSHOWN       4
#define ITID_ADDRESSBANDSHOWN       5
#define ITID_LINKSBANDSHOWN         6
#define ITID_MENUBANDSHOWN          12
#define ITID_AUTOHIDEENABLED        13
#define ITID_CUSTOMIZEENABLED       20
#define ITID_TOOLBARLOCKED          27

// commands in the CGID_BrandCmdGroup command group handled by the brand band
#define BBID_STARTANIMATION         1
#define BBID_STOPANIMATION          2

// undocumented flags for IShellMenu::SetShellFolder
#define SMSET_UNKNOWN08             0x08
#define SMSET_UNKNOWN10             0x10

// explorer tray commands
#define TRAYCMD_STARTMENU           305
#define TRAYCMD_RUN_DIALOG          401
#define TRAYCMD_LOGOFF_DIALOG       402
#define TRAYCMD_CASCADE             403
#define TRAYCMD_TILE_H              404
#define TRAYCMD_TILE_V              405
#define TRAYCMD_TOGGLE_DESKTOP      407
#define TRAYCMD_DATE_AND_TIME       408
#define TRAYCMD_TASKBAR_PROPERTIES  413
#define TRAYCMD_MINIMIZE_ALL        415
#define TRAYCMD_RESTORE_ALL         416
#define TRAYCMD_SHOW_DESKTOP        419
#define TRAYCMD_SHOW_TASK_MGR       420
#define TRAYCMD_CUSTOMIZE_TASKBAR   421
#define TRAYCMD_LOCK_TASKBAR        424
#define TRAYCMD_HELP_AND_SUPPORT    503
#define TRAYCMD_CONTROL_PANEL       505
#define TRAYCMD_SHUTDOWN_DIALOG     506
#define TRAYCMD_PRINTERS_AND_FAXES  510
#define TRAYCMD_LOCK_DESKTOP        517
#define TRAYCMD_SWITCH_USER_DIALOG  5000
#define TRAYCMD_SEARCH_FILES        41093
#define TRAYCMD_SEARCH_COMPUTERS    41094

// Explorer Tray Application Bar Data Message Commands
#define TABDMC_APPBAR     0
#define TABDMC_NOTIFY     1
#define TABDMC_LOADINPROC 2

void WINAPI ShellDDEInit(BOOL bInit);

IStream* WINAPI SHGetViewStream(LPCITEMIDLIST, DWORD, LPCTSTR, LPCTSTR, LPCTSTR);

HRESULT WINAPI SHCreateSessionKey(REGSAM samDesired, PHKEY phKey);

LONG WINAPI SHRegQueryValueExA(
    HKEY hkey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData);
LONG WINAPI SHRegQueryValueExW(
    HKEY hkey,
    LPCWSTR pszValue,
    LPDWORD pdwReserved,
    LPDWORD pdwType,
    LPVOID pvData,
    LPDWORD pcbData);
#ifdef UNICODE
    #define SHRegQueryValueEx SHRegQueryValueExW
#else
    #define SHRegQueryValueEx SHRegQueryValueExA
#endif

BOOL WINAPI
SHIsBadInterfacePtr(
    _In_ LPCVOID pv,
    _In_ UINT_PTR ucb);

HRESULT WINAPI
CopyStreamUI(
    _In_ IStream *pSrc,
    _Out_ IStream *pDst,
    _Inout_opt_ IProgressDialog *pProgress,
    _In_opt_ DWORDLONG dwlSize);

// Flags for SHGetComputerDisplayNameW
#define SHGCDN_NOCACHE 0x1
#define SHGCDN_NOSERVERNAME 0x10000

HRESULT WINAPI
SHGetComputerDisplayNameW(
    _In_opt_ LPWSTR pszServerName,
    _In_ DWORD dwFlags,
    _Out_writes_z_(cchNameMax) LPWSTR pszName,
    _In_ DWORD cchNameMax);

/*****************************************************************************
 * INVALID_FILETITLE_CHARACTERS
 */

#define INVALID_FILETITLE_CHARACTERSA "\\/:*?\"<>|"
#define INVALID_FILETITLE_CHARACTERSW L"\\/:*?\"<>|"

#ifdef UNICODE
    #define INVALID_FILETITLE_CHARACTERS INVALID_FILETITLE_CHARACTERSW
#else
    #define INVALID_FILETITLE_CHARACTERS INVALID_FILETITLE_CHARACTERSA
#endif

/*****************************************************************************
 * Shell Link
 */
#include <pshpack1.h>

typedef struct tagSHELL_LINK_HEADER
{
    /* The size of this structure (always 0x0000004C) */
    DWORD dwSize;
    /* CLSID = class identifier (always 00021401-0000-0000-C000-000000000046) */
    CLSID clsid;
    /* Flags (SHELL_LINK_DATA_FLAGS) */
    DWORD dwFlags;
    /* Informations about the link target: */
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeLow; /* only the least significant 32 bits */
    /* The index of an icon (signed?) */
    DWORD nIconIndex;
    /* The expected window state of an application launched by the link */
    DWORD nShowCommand;
    /* The keystrokes used to launch the application */
    WORD wHotKey;
    /* Reserved (must be zero) */
    WORD wReserved1;
    DWORD dwReserved2;
    DWORD dwReserved3;
} SHELL_LINK_HEADER, *LPSHELL_LINK_HEADER;

/*****************************************************************************
 * SHELL_LINK_INFOA/W
 * If cbHeaderSize == 0x0000001C then use SHELL_LINK_INFOA
 * If cbHeaderSize >= 0x00000024 then use SHELL_LINK_INFOW
 */
typedef struct tagSHELL_LINK_INFOA
{
    /* Size of the link info data */
    DWORD cbSize;
    /* Size of this structure (ANSI: = 0x0000001C) */
    DWORD cbHeaderSize;
    /* Specifies which fields are present/populated (SLI_*) */
    DWORD dwFlags;
    /* Offset of the VolumeID field (SHELL_LINK_INFO_VOLUME_ID) */
    DWORD cbVolumeIDOffset;
    /* Offset of the LocalBasePath field (ANSI, NULL-terminated string) */
    DWORD cbLocalBasePathOffset;
    /* Offset of the CommonNetworkRelativeLink field (SHELL_LINK_INFO_CNR_LINK) */
    DWORD cbCommonNetworkRelativeLinkOffset;
    /* Offset of the CommonPathSuffix field (ANSI, NULL-terminated string) */
    DWORD cbCommonPathSuffixOffset;
} SHELL_LINK_INFOA, *LPSHELL_LINK_INFOA;

typedef struct tagSHELL_LINK_INFOW
{
    /* Size of the link info data */
    DWORD cbSize;
    /* Size of this structure (Unicode: >= 0x00000024) */
    DWORD cbHeaderSize;
    /* Specifies which fields are present/populated (SLI_*) */
    DWORD dwFlags;
    /* Offset of the VolumeID field (SHELL_LINK_INFO_VOLUME_ID) */
    DWORD cbVolumeIDOffset;
    /* Offset of the LocalBasePath field (ANSI, NULL-terminated string) */
    DWORD cbLocalBasePathOffset;
    /* Offset of the CommonNetworkRelativeLink field (SHELL_LINK_INFO_CNR_LINK) */
    DWORD cbCommonNetworkRelativeLinkOffset;
    /* Offset of the CommonPathSuffix field (ANSI, NULL-terminated string) */
    DWORD cbCommonPathSuffixOffset;
    /* Offset of the LocalBasePathUnicode field (Unicode, NULL-terminated string) */
    DWORD cbLocalBasePathUnicodeOffset;
    /* Offset of the CommonPathSuffixUnicode field (Unicode, NULL-terminated string) */
    DWORD cbCommonPathSuffixUnicodeOffset;
} SHELL_LINK_INFOW, *LPSHELL_LINK_INFOW;

/* VolumeID, LocalBasePath, LocalBasePathUnicode(cbHeaderSize >= 0x24) are present */
#define SLI_VALID_LOCAL   0x00000001
/* CommonNetworkRelativeLink is present */
#define SLI_VALID_NETWORK 0x00000002

/*****************************************************************************
 * SHELL_LINK_INFO_VOLUME_IDA/W
 * If cbVolumeLabelOffset != 0x00000014 (should be 0x00000010) then use
 * SHELL_LINK_INFO_VOLUME_IDA
 * If cbVolumeLabelOffset == 0x00000014 then use SHELL_LINK_INFO_VOLUME_IDW
 */
typedef struct tagSHELL_LINK_INFO_VOLUME_IDA
{
    /* Size of the VolumeID field (> 0x00000010) */
    DWORD cbSize;
    /* Drive type of the drive the link target is stored on (DRIVE_*) */
    DWORD dwDriveType;
    /* Serial number of the volume the link target is stored on */
    DWORD nDriveSerialNumber;
    /* Offset of the volume label (ANSI, NULL-terminated string).
       Must be != 0x00000014 (see tagSHELL_LINK_INFO_VOLUME_IDW) */
    DWORD cbVolumeLabelOffset;
} SHELL_LINK_INFO_VOLUME_IDA, *LPSHELL_LINK_INFO_VOLUME_IDA;

typedef struct tagSHELL_LINK_INFO_VOLUME_IDW
{
    /* Size of the VolumeID field (> 0x00000010) */
    DWORD cbSize;
    /* Drive type of the drive the link target is stored on (DRIVE_*) */
    DWORD dwDriveType;
    /* Serial number of the volume the link target is stored on */
    DWORD nDriveSerialNumber;
    /* Offset of the volume label (ANSI, NULL-terminated string).
       If the value of this field is 0x00000014, ignore it and use
       cbVolumeLabelUnicodeOffset! */
    DWORD cbVolumeLabelOffset;
    /* Offset of the volume label (Unicode, NULL-terminated string).
       If the value of the VolumeLabelOffset field is not 0x00000014,
       this field must be ignored (==> it doesn't exists ==> ANSI). */
    DWORD cbVolumeLabelUnicodeOffset;
} SHELL_LINK_INFO_VOLUME_IDW, *LPSHELL_LINK_INFO_VOLUME_IDW;

/*****************************************************************************
 * SHELL_LINK_INFO_CNR_LINKA/W (CNR = Common Network Relative)
 * If cbNetNameOffset == 0x00000014 then use SHELL_LINK_INFO_CNR_LINKA
 * If cbNetNameOffset > 0x00000014 then use SHELL_LINK_INFO_CNR_LINKW
 */
typedef struct tagSHELL_LINK_INFO_CNR_LINKA
{
    /* Size of the CommonNetworkRelativeLink field (>= 0x00000014) */
    DWORD cbSize;
    /* Specifies which fields are present/populated (SLI_CNR_*) */
    DWORD dwFlags;
    /* Offset of the NetName field (ANSI, NULL–terminated string) */
    DWORD cbNetNameOffset;
    /* Offset of the DeviceName field (ANSI, NULL–terminated string) */
    DWORD cbDeviceNameOffset;
    /* Type of the network provider (WNNC_NET_* defined in winnetwk.h) */
    DWORD dwNetworkProviderType;
} SHELL_LINK_INFO_CNR_LINKA, *LPSHELL_LINK_INFO_CNR_LINKA;

typedef struct tagSHELL_LINK_INFO_CNR_LINKW
{
    /* Size of the CommonNetworkRelativeLink field (>= 0x00000014) */
    DWORD cbSize;
    /* Specifies which fields are present/populated (SLI_CNR_*) */
    DWORD dwFlags;
    /* Offset of the NetName field (ANSI, NULL–terminated string) */
    DWORD cbNetNameOffset;
    /* Offset of the DeviceName field (ANSI, NULL–terminated string) */
    DWORD cbDeviceNameOffset;
    /* Type of the network provider (WNNC_NET_* defined in winnetwk.h) */
    DWORD dwNetworkProviderType;
    /* Offset of the NetNameUnicode field (Unicode, NULL–terminated string) */
    DWORD cbNetNameUnicodeOffset;
    /* Offset of the DeviceNameUnicode field (Unicode, NULL–terminated string) */
    DWORD cbDeviceNameUnicodeOffset;
} SHELL_LINK_INFO_CNR_LINKW, *LPSHELL_LINK_INFO_CNR_LINKW;

/* DeviceName is present */
#define SLI_CNR_VALID_DEVICE   0x00000001
/* NetworkProviderType is present */
#define SLI_CNR_VALID_NET_TYPE 0x00000002

/*****************************************************************************
 * Shell Link Extra Data (IShellLinkDataList)
 */
typedef struct tagEXP_TRACKER
{
    /* .cbSize = 0x00000060, .dwSignature = 0xa0000003 */
    DATABLOCK_HEADER dbh;
    /* Length >= 0x00000058 */
    DWORD nLength;
    /* Must be 0x00000000 */
    DWORD nVersion;
    /* NetBIOS name (ANSI, unused bytes are set to zero) */
    CHAR szMachineID[16]; /* "variable" >= 16 (?) */
    /* Some GUIDs for the Link Tracking service (from the FS?) */
    GUID guidDroidVolume;
    GUID guidDroidObject;
    GUID guidDroidBirthVolume;
    GUID guidDroidBirthObject;
} EXP_TRACKER, *LPEXP_TRACKER;

typedef struct tagEXP_SHIM
{
    /* .cbSize >= 0x00000088, .dwSignature = 0xa0000008 */
    DATABLOCK_HEADER dbh;
    /* Name of a shim layer to apply (Unicode, unused bytes are set to zero) */
    WCHAR szwLayerName[64]; /* "variable" >= 64 */
} EXP_SHIM, *LPEXP_SHIM;

typedef struct tagEXP_KNOWN_FOLDER
{
    /* .cbSize = 0x0000001c, .dwSignature = 0xa000000b */
    DATABLOCK_HEADER dbh;
    /* A GUID value that identifies a known folder */
    GUID guidKnownFolder;
    /* Specifies the location of the ItemID of the first child
       segment of the IDList specified by guidKnownFolder */
    DWORD cbOffset;
} EXP_KNOWN_FOLDER, *LPEXP_KNOWN_FOLDER;

typedef struct tagEXP_VISTA_ID_LIST
{
    /* .cbSize >= 0x0000000a, .dwSignature = 0xa000000c */
    DATABLOCK_HEADER dbh;
    /* Specifies an alternate IDList that can be used instead
       of the "normal" IDList (SLDF_HAS_ID_LIST) */
    /* LPITEMIDLIST pIDList; (variable) */
} EXP_VISTA_ID_LIST, *LPEXP_VISTA_ID_LIST;

#define EXP_TRACKER_SIG       0xa0000003
#define EXP_SHIM_SIG          0xa0000008
#define EXP_KNOWN_FOLDER_SIG  0xa000000b
#define EXP_VISTA_ID_LIST_SIG 0xa000000c

/* Not compatible yet */
typedef struct SFVM_CUSTOMVIEWINFO_DATA
{
    ULONG cbSize;
    HBITMAP hbmBack;
    COLORREF clrText;
    COLORREF clrTextBack;
} SFVM_CUSTOMVIEWINFO_DATA, *LPSFVM_CUSTOMVIEWINFO_DATA;

#include <poppack.h>

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_UNDOCSHELL_H */
