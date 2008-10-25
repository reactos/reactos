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

/*
	string functions
*/
BOOL WINAPI StrRetToStrNA(LPSTR,DWORD,LPSTRRET,const ITEMIDLIST*);
BOOL WINAPI StrRetToStrNW(LPWSTR,DWORD,LPSTRRET,const ITEMIDLIST*);

/****************************************************************************
 * Shell Common Dialogs
 */

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

BOOL WINAPI SHFindComputer(
	LPCITEMIDLIST pidlRoot,
	LPCITEMIDLIST pidlSavedSearch);

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

/****************************************************************************
 * Memory Routines
 */

/* The Platform SDK's shlobj.h header defines similar functions with a
 * leading underscore. However those are unusable because of the leading
 * underscore, because they have an incorrect calling convention, and
 * because these functions are not exported by name anyway.
 */
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

DWORD WINAPI CheckEscapesA(LPSTR string, DWORD len);
DWORD WINAPI CheckEscapesW(LPWSTR string, DWORD len);

/* policy functions */
BOOL WINAPI SHInitRestricted(LPCVOID unused, LPCVOID inpRegKey);

/* Shell Desktop functions */

#undef INTERFACE
#define INTERFACE IShellDesktop
DECLARE_INTERFACE_(IShellDesktop,IUnknown)
{
    /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IShellDesktopTray ***/
    STDMETHOD_(ULONG,GetState)(THIS) PURE;
    STDMETHOD(GetTrayWindow)(THIS_ HWND*) PURE;
    STDMETHOD(RegisterDesktopWindow)(THIS_ HWND) PURE;
    STDMETHOD(Unknown)(THIS_ DWORD,DWORD) PURE;
};
#undef INTERFACE

#define IShellDesktop_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IShellDesktop_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IShellDesktop_Release(T) (T)->lpVtbl->Release(T)
#define IShellDesktop_GetState(T) (T)->lpVtbl->GetState(T)
#define IShellDesktop_GetTrayWindow(T,a) (T)->lpVtbl->GetTrayWindow(T,a)
#define IShellDesktop_RegisterDesktopWindow(T,a) (T)->lpVtbl->RegisterDesktopWindow(T,a)
#define IShellDesktop_Unknown(T,a,b) (T)->lpVtbl->Unknown(T,a,b)

#define WM_GETISHELLBROWSER (WM_USER+7)

HANDLE WINAPI SHCreateDesktop(IShellDesktop*);
BOOL WINAPI SHDesktopMessageLoop(HANDLE);

#define CSIDL_FOLDER_MASK	0x00ff

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_UNDOCSHELL_H */
