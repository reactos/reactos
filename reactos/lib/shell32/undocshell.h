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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_UNDOCSHELL_H
#define __WINE_UNDOCSHELL_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "commctrl.h"
#include "shlobj.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/****************************************************************************
 *	IDList Functions
 */
LPITEMIDLIST WINAPI ILClone (LPCITEMIDLIST pidl);
LPITEMIDLIST WINAPI ILCloneFirst(LPCITEMIDLIST pidl);

LPITEMIDLIST WINAPI ILCombine(
	LPCITEMIDLIST iil1,
	LPCITEMIDLIST iil2);

DWORD WINAPI ILGetSize(LPCITEMIDLIST pidl);

LPITEMIDLIST WINAPI ILGetNext(LPCITEMIDLIST pidl);
LPITEMIDLIST WINAPI ILFindLastID(LPCITEMIDLIST pidl);
BOOL WINAPI ILRemoveLastID(LPCITEMIDLIST pidl);

LPITEMIDLIST WINAPI ILFindChild(
	LPCITEMIDLIST pidl1,
	LPCITEMIDLIST pidl2);

LPITEMIDLIST WINAPI ILAppendID(
	LPITEMIDLIST pidl,
	LPCSHITEMID lpItemID,
	BOOL bAddToEnd);

BOOL WINAPI ILIsEqual(
	LPCITEMIDLIST pidl1,
	LPCITEMIDLIST pidl2);

BOOL WINAPI ILIsParent(
	LPCITEMIDLIST pidlParent,
	LPCITEMIDLIST pidlChild,
	BOOL bImmediate);

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

DWORD WINAPI ILFree(LPITEMIDLIST pidl);

HRESULT WINAPI ILSaveToStream(
	LPSTREAM pstrm,
	LPCITEMIDLIST pidl);

HRESULT WINAPI ILLoadFromStream(
	LPSTREAM pstrm,
	LPITEMIDLIST *ppidl);

LPITEMIDLIST WINAPI ILGlobalClone(LPCITEMIDLIST pidl);
void WINAPI ILGlobalFree(LPITEMIDLIST pidl);

LPITEMIDLIST WINAPI SHSimpleIDListFromPathA (LPCSTR lpszPath);
LPITEMIDLIST WINAPI SHSimpleIDListFromPathW (LPCWSTR lpszPath);
LPITEMIDLIST WINAPI SHSimpleIDListFromPathAW (LPCVOID lpszPath);

HRESULT WINAPI SHILCreateFromPathA (
	LPCSTR path,
	LPITEMIDLIST * ppidl,
	DWORD *attributes);

HRESULT WINAPI SHILCreateFromPathW (
	LPCWSTR path,
	LPITEMIDLIST * ppidl,
	DWORD *attributes);

HRESULT WINAPI SHILCreateFromPathAW (
	LPCVOID path,
	LPITEMIDLIST * ppidl,
	DWORD *attributes);

LPITEMIDLIST WINAPI ILCreateFromPathA(LPCSTR path);
LPITEMIDLIST WINAPI ILCreateFromPathW(LPCWSTR path);
LPITEMIDLIST WINAPI ILCreateFromPathAW(LPCVOID path);

/*
	string functions
*/
HRESULT WINAPI StrRetToStrNA (
	LPVOID dest,
	DWORD len,
	LPSTRRET src,
	const ITEMIDLIST *pidl);

HRESULT WINAPI StrRetToStrNW (
	LPVOID dest,
	DWORD len,
	LPSTRRET src,
	const ITEMIDLIST *pidl);

HRESULT WINAPI StrRetToStrNAW (
	LPVOID dest,
	DWORD len,
	LPSTRRET src,
	const ITEMIDLIST *pidl);


/****************************************************************************
* SHChangeNotifyRegister API
*/
#define SHCNRF_InterruptLevel		0x0001
#define SHCNRF_ShellLevel		0x0002
#define SHCNRF_RecursiveInterrupt	0x1000	/* Must be combined with SHCNRF_InterruptLevel */
#define SHCNRF_NewDelivery		0x8000	/* Messages use shared memory */

/****************************************************************************
 * Shell Common Dialogs
 */

BOOL WINAPI PickIconDlg(
	HWND hwndOwner,
	LPSTR lpstrFile,
	DWORD nMaxFile,
	LPDWORD lpdwIconIndex);

/* RunFileDlg flags */
#define RFF_NOBROWSE       0x01
#define RFF_NODEFAULT      0x02
#define RFF_CALCDIRECTORY  0x04
#define RFF_NOLABEL        0x08
#define RFF_NOSEPARATEMEM  0x20  /* NT only */

/* RunFileFlg notification structure */
typedef struct
{
  NMHDR   hdr;
  LPCSTR  lpFile;
  LPCSTR  lpDirectory;
  int     nShow;
} NM_RUNFILEDLG, * LPNM_RUNFILEDLG;

/* RunFileDlg notification return values */
#define RF_OK      0x00
#define RF_CANCEL  0x01
#define RF_RETRY   0x02

void WINAPI RunFileDlg(
	HWND hwndOwner,
	HICON hIcon,
	LPCSTR lpstrDirectory,
	LPCSTR lpstrTitle,
	LPCSTR lpstrDescription,
	UINT uFlags);

void WINAPI ExitWindowsDialog(HWND hwndOwner);

BOOL WINAPI GetFileNameFromBrowse(
	HWND hwndOwner,
	LPSTR lpstrFile,
	DWORD nMaxFile,
	LPCSTR lpstrInitialDir,
	LPCSTR lpstrDefExt,
	LPCSTR lpstrFilter,
	LPCSTR lpstrTitle);

BOOL WINAPI SHFindFiles(
	LPCITEMIDLIST pidlRoot,
	LPCITEMIDLIST pidlSavedSearch);

BOOL WINAPI SHFindComputer(
	LPCITEMIDLIST pidlRoot,
	LPCITEMIDLIST pidlSavedSearch);

/* SHObjectProperties flags */
#define OPF_PRINTERNAME  0x01
#define OPF_PATHNAME     0x02

BOOL WINAPI SHObjectProperties(
	HWND hwndOwner,
	UINT uFlags,
	LPCSTR lpstrName,
	LPCSTR lpstrParameters);

void WINAPI SHHandleDiskFull(HWND hwndOwner,
	UINT uDrive);

int  WINAPI SHOutOfMemoryMessageBox(
	HWND hwndOwner,
	LPCSTR lpCaption,
	UINT uType);

DWORD WINAPI SHNetConnectionDialog(
	HWND hwndOwner,
	LPCSTR lpstrRemoteName,
	DWORD dwType);

int WINAPIV ShellMessageBoxA(
	HINSTANCE hInstance,
	HWND hWnd,
	LPCSTR lpText,
	LPCSTR lpCaption,
	UINT uType,
	...);

int WINAPIV ShellMessageBoxW(
	HINSTANCE hInstance,
	HWND hWnd,
	LPCWSTR lpText,
	LPCWSTR lpCaption,
	UINT uType,
	...);

#define ShellMessageBox WINELIB_NAME_AW(ShellMessageBox)

/****************************************************************************
 * Memory Routines
 */

LPVOID WINAPI SHAlloc(ULONG cb);
void WINAPI SHFree(LPVOID pv);

HANDLE WINAPI SHAllocShared(
	LPVOID pv,
	ULONG cb,
	DWORD pid);

BOOL WINAPI SHFreeShared(
	HANDLE hMem,
	DWORD pid);

LPVOID WINAPI SHLockShared(
	HANDLE hMem,
	DWORD pid);

BOOL WINAPI SHUnlockShared(LPVOID pv);

/****************************************************************************
 * Cabinet Window Messages
 */

#define CWM_SETPATH           (WM_USER + 2)
#define CWM_WANTIDLE	      (WM_USER + 3)
#define CWM_GETSETCURRENTINFO (WM_USER + 4)
#define CWM_SELECTITEM        (WM_USER + 5)
#define CWM_SELECTITEMSTR     (WM_USER + 6)
#define CWM_GETISHELLBROWSER  (WM_USER + 7)
#define CWM_TESTPATH          (WM_USER + 9)
#define CWM_STATECHANGE       (WM_USER + 10)
#define CWM_GETPATH           (WM_USER + 12)

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

int WINAPI Shell_GetCachedImageIndex(
	LPCSTR lpszFileName,
	UINT nIconIndex,
	BOOL bSimulateDoc);

BOOL WINAPI Shell_GetImageLists(
	HIMAGELIST *lphimlLarge,
	HIMAGELIST *lphimlSmall);

HICON WINAPI SHGetFileIcon(
	DWORD dwReserved,
	LPCSTR lpszPath,
	DWORD dwFileAttributes,
	UINT uFlags);

int WINAPI SHMapPIDLToSystemImageListIndex(
	LPSHELLFOLDER psf,
	LPCITEMIDLIST pidl,
	UINT * pOpenIndex);

BOOL WINAPI FileIconInit(BOOL bFullInit);

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
HRESULT WINAPI SHLoadOLE(DWORD dwFlags);

HRESULT WINAPI SHRegisterDragDrop(
	HWND hWnd,
	LPDROPTARGET lpDropTarget);

HRESULT WINAPI SHRevokeDragDrop(HWND hWnd);

HRESULT WINAPI SHDoDragDrop(
	HWND hWnd,
	LPDATAOBJECT lpDataObject,
	LPDROPSOURCE lpDropSource,
	DWORD dwOKEffect,
	LPDWORD pdwEffect);

BOOL WINAPI DAD_DragEnter(HWND hWnd);

BOOL WINAPI DAD_DragEnterEx(
	HWND hWnd,
	POINT pt);

BOOL WINAPI DAD_DragMove(POINT pt);

/* DAD_AutoScroll return values */
#define DAD_SCROLL_UP    1
#define DAD_SCROLL_DOWN  2
#define DAD_SCROLL_LEFT  4
#define DAD_SCROLL_RIGHT 8

/* DAD_AutoScroll sample structure */
typedef struct
{
	DWORD  dwCount;
	DWORD  dwLastTime;
	BOOL   bWrapped;
	POINT  aptPositions[3];
	DWORD  adwTimes[3];
} SCROLLSAMPLES, *LPSCROLLSAMPLES;

DWORD WINAPI DAD_AutoScroll(HWND hWnd,
	LPSCROLLSAMPLES lpSamples,
	LPPOINT lppt);

BOOL WINAPI DAD_DragLeave();

BOOL WINAPI DAD_SetDragImageFromListView(
	HWND hWnd,
	POINT pt);

BOOL WINAPI DAD_SetDragImage(
	HIMAGELIST himlTrack,
	LPPOINT lppt);

BOOL WINAPI DAD_ShowDragImage(BOOL bShow);

HRESULT WINAPI SHCreateStdEnumFmtEtc(
	DWORD cFormats,
	const FORMATETC *lpFormats,
	LPENUMFORMATETC *ppenumFormatetc);

HRESULT WINAPI CIDLData_CreateFromIDArray(
	LPCITEMIDLIST pidlFolder,
	DWORD cpidlFiles,
	LPCITEMIDLIST *lppidlFiles,
	LPDATAOBJECT *ppdataObject);

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

/* PathCleanupSpec return values */
#define PCS_REPLACEDCHARS  0x00000001
#define PCS_REMOVEDCHARS   0x00000002
#define PCS_SHORTENED      0x00000004
#define PCS_PATHTOOLONG    0x80000008

DWORD WINAPI PathCleanupSpecAW(LPCVOID lpszPath, LPVOID lpszFile);

BOOL WINAPI PathQualifyA(LPCSTR path);
BOOL WINAPI PathQualifyW(LPCWSTR path);
#define PathQualify WINELIB_NAME_AW(PathQualify)
BOOL  WINAPI PathQualifyAW(LPCVOID path);


/* PathResolve flags */
#define PRF_CHECKEXISTANCE  0x01
#define PRF_EXECUTABLE      0x02
#define PRF_QUALIFYONPATH   0x04
#define PRF_WINDOWS31       0x08

BOOL WINAPI PathResolveAW(LPVOID lpszPath, LPCVOID *alpszPaths, DWORD dwFlags);

VOID WINAPI PathSetDlgItemPathAW(HWND hDlg, int nIDDlgItem, LPCVOID lpszPath);

/* PathProcessCommand flags */
#define PPCF_QUOTEPATH        0x01 /* implies PPCF_INCLUDEARGS */
#define PPCF_INCLUDEARGS      0x02
#define PPCF_NODIRECTORIES    0x10
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

BOOL WINAPI PathFindOnPathAW(LPVOID sFile, LPCVOID sOtherDirs);

/****************************************************************************
 * Shell Namespace Routines
 */

/* SHCreateShellFolderViewEx callback function */
typedef HRESULT (CALLBACK *LPFNSFVCALLBACK)(
	DWORD dwUser,
	LPSHELLFOLDER pshf,
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

/* SHCreateShellFolderViewEx structure */
typedef struct
{
  DWORD            dwSize;
  LPSHELLFOLDER    pshf;
  DWORD            dwUser;
  LPCITEMIDLIST    pidlFolder;
  DWORD            dwEventId;
  LPFNSFVCALLBACK  lpfnCallback;
  UINT             uViewMode;
} SHELLFOLDERVIEWINFO, * LPSHELLFOLDERVIEWINFO;
typedef const SHELLFOLDERVIEWINFO * LPCSHELLFOLDERVIEWINFO;

HRESULT WINAPI SHCreateShellFolderViewEx(
	LPCSHELLFOLDERVIEWINFO pshfvi,
	LPSHELLVIEW *ppshv);

/* SHCreateShellFolderViewEx callback messages */
#define SFVCB_ADDTOMENU           0x0001
#define SFVCB_INVOKECOMMAND       0x0002
#define SFVCB_GETMENUHELP         0x0003
#define SFVCB_GETTOOLBARTIP       0x0004
#define SFVCB_GETTOOLBARINFO      0x0005
#define SFVCB_ADDTOOLBARITEMS     0x0006
#define SFVCB_INITMENUPOPUP       0x0007
#define SFVCB_SELECTIONCHANGED    0x0008
#define SFVCB_DRAWMENUITEM        0x0009
#define SFVCB_MEASUREMENUITEM     0x000A
#define SFVCB_EXITMENULOOP        0x000B
#define SFVCB_VIEWRELEASE         0x000C
#define SFVCB_GETNAMELENGTH       0x000D
#define SFVCB_CHANGENOTIFY        0x000E
#define SFVCB_WINDOWCREATED       0x000F
#define SFVCB_WINDOWCLOSING       0x0010
#define SFVCB_LISTREFRESHED       0x0011
#define SFVCB_WINDOWFOCUSED       0x0012
#define SFVCB_REGISTERCOPYHOOK    0x0014
#define SFVCB_COPYHOOKCALLBACK    0x0015
#define SFVCB_GETDETAILSOF        0x0017
#define SFVCB_COLUMNCLICK         0x0018
#define SFVCB_GETCHANGENOTIFYPIDL 0x0019
#define SFVCB_GETESTIMATEDCOUNT   0x001A
#define SFVCB_ADJUSTVIEWMODE      0x001B
#define SFVCB_REMOVEFROMMENU      0x001C
#define SFVCB_ADDINGOBJECT        0x001D
#define SFVCB_REMOVINGOBJECT      0x001E
#define SFVCB_UPDATESTATUSBAR     0x001F
#define SFVCB_ISSLOWREFRESH       0x0020
#define SFVCB_GETCOMMANDDIR       0x0021
#define SFVCB_GETCOLUMNSTREAM     0x0022
#define SFVCB_CANSELECTALL        0x0023
#define SFVCB_DRAGSUCCEEDED       0x0024
#define SFVCB_ISSTRICTREFRESH     0x0025
#define SFVCB_ISCHILDOBJECT       0x0026

/* Generic structure used by several messages */
typedef struct
{
  DWORD          dwReserved;
  DWORD          dwReserved2;
  LPCITEMIDLIST  pidl;
  LPDWORD        lpdwUser;
} SFVCBINFO, * LPSFVCBINFO;
typedef const SFVCBINFO * LPCSFVCBINFO;

/* SFVCB_ADDTOMENU structure */
typedef struct
{
  HMENU  hMenu;
  UINT   indexMenu;
  UINT   idCmdFirst;
  UINT   idCmdLast;
} SFVMENUINFO, * LPSFVMENUINFO;

/* SFVCB_GETTOOLBARINFO structure */
typedef struct
{
  UINT  nButtons;
  UINT  uFlags;
} SFVTOOLBARINFO, * LPSFVTOOLBARINFO;

/* SFVTOOLBARINFO flags */
typedef enum
{
  SFVTI_ADDTOEND   = 0,
  SFVTI_ADDTOFRONT = 1,
  SFVTI_OVERWRITE  = 2
} SFVTIF;

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

int WINAPI SHShellFolderView_Message(
	HWND hwndCabinet,
	DWORD dwMessage,
	DWORD dwParam);

/* SHShellFolderView_Message messages */
#define SFVM_REARRANGE          0x0001
#define SFVM_GETARRANGECOLUMN   0x0002
#define SFVM_ADDOBJECT          0x0003
#define SFVM_GETITEMCOUNT       0x0004
#define SFVM_GETITEMPIDL        0x0005
#define SFVM_REMOVEOBJECT       0x0006
#define SFVM_UPDATEOBJECT       0x0007
#define SFVM_SETREDRAW          0x0008
#define SFVM_GETSELECTEDOBJECTS 0x0009
#define SFVM_ISDROPONSOURCE     0x000A
#define SFVM_MOVEICONS          0x000B
#define SFVM_GETDRAGPOINT       0x000C
#define SFVM_GETDROPPOINT       0x000D
#define SFVM_SETOBJECTPOS       0x000E
#define SFVM_ISDROPONBACKGROUND 0x000F
#define SFVM_CUTOBJECTS         0x0010
#define SFVM_TOGGLEAUTOARRANGE  0x0011
#define SFVM_LINEUPICONS        0x0012
#define SFVM_GETAUTOARRANGE     0x0013
#define SFVM_GETSELECTEDCOUNT   0x0014
#define SFVM_GETITEMSPACING     0x0015
#define SFVM_REFRESHOBJECT      0x0016
#define SFVM_SETCLIPBOARDPOINTS 0x0017

/****************************************************************************
 * Misc Stuff
 */

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

DWORD WINAPI SHCLSIDFromStringA (LPCSTR clsid, CLSID *id);
DWORD WINAPI SHCLSIDFromStringW (LPCWSTR clsid, CLSID *id);

void WINAPI SHFreeUnusedLibraries();

/* SHCreateLinks flags */
#define SHCLF_PREFIXNAME       0x01
#define SHCLF_CREATEONDESKTOP  0x02

HRESULT WINAPI SHCreateLinks(
	HWND hWnd,
	LPCSTR lpszDir,
	LPDATAOBJECT lpDataObject,
	UINT uFlags,
	LPITEMIDLIST *lppidlLinks);

/* SHGetNewLinkInfo flags */
#define SHGNLI_PIDL        0x01
#define SHGNLI_PREFIXNAME  0x02
#define SHGNLI_NOUNIQUE    0x04

BOOL WINAPI SHGetNewLinkInfoA(
	LPCSTR pszLinkTo,
	LPCSTR pszDir,
	LPSTR pszName,
	BOOL *pfMustCopy,
	UINT uFlags);

BOOL WINAPI SHGetNewLinkInfoW(
	LPCWSTR pszLinkTo,
	LPCWSTR pszDir,
	LPWSTR pszName,
	BOOL *pfMustCopy,
	UINT uFlags);
#define SHGetNewLinkInfo WINELIB_NAME_AW(SHGetNewLinkInfo)

DWORD WINAPI CheckEscapesA(LPSTR string, DWORD len);
DWORD WINAPI CheckEscapesW(LPWSTR string, DWORD len);

/* policy functions */
BOOL WINAPI SHInitRestricted(LPCVOID unused, LPCVOID inpRegKey);

/* cabinet functions */

#include "pshpack1.h"

typedef struct {
    WORD cLength;
    WORD nVersion;
    BOOL fFullPathTitle:1;
    BOOL fSaveLocalView:1;
    BOOL fNotShell:1;
    BOOL fSimpleDefault:1;
    BOOL fDontShowDescBar:1;
    BOOL fNewWindowMode:1;
    BOOL fShowCompColor:1;
    BOOL fDontPrettyNames:1;
    BOOL fAdminsCreateCommonGroups:1;
    UINT fUnusedFlags:7;
    UINT fMenuEnumFilter;
} CABINETSTATE;

#include "poppack.h"

BOOL WINAPI ReadCabinetState(CABINETSTATE *, int);
BOOL WINAPI WriteCabinetState(CABINETSTATE *);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_UNDOCSHELL_H */
