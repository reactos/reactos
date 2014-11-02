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
* SHChangeNotifyRegister API
*/
#define SHCNRF_InterruptLevel		0x0001
#define SHCNRF_ShellLevel		0x0002
#define SHCNRF_RecursiveInterrupt	0x1000	/* Must be combined with SHCNRF_InterruptLevel */
#define SHCNRF_NewDelivery		0x8000	/* Messages use shared memory */


/****************************************************************************
 * Shell Common Dialogs
 */

/* RunFileDlg flags */
#define RFF_NOBROWSE       0x01
#define RFF_NODEFAULT      0x02
#define RFF_CALCDIRECTORY  0x04
#define RFF_NOLABEL        0x08
#define RFF_NOSEPARATEMEM  0x20  /* NT only */

#define DE_SAMEFILE     0x71
#define DE_DESTSAMETREE 0x7D

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
	LPCWSTR lpstrDirectory,
	LPCWSTR lpstrTitle,
	LPCWSTR lpstrDescription,
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
	LPCWSTR lpstrRemoteName,
	DWORD dwType);

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

#define WM_GETISHELLBROWSER (WM_USER+7)

BOOL WINAPI SHDesktopMessageLoop(HANDLE);

#define CSIDL_FOLDER_MASK	0x00ff

/* Utility functions */
#include <stdio.h>

#define SMC_EXEC 4
INT WINAPI Shell_GetCachedImageIndex(LPCWSTR szPath, INT nIndex, UINT bSimulateDoc);

static inline ULONG
Win32DbgPrint(const char *filename, int line, const char *lpFormat, ...)
{
    char szMsg[512];
    char *szMsgStart;
    const char *fname;
    va_list vl;
    ULONG uRet;

    fname = strrchr(filename, '\\');
    if (fname == NULL)
    {
        fname = strrchr(filename, '/');
        if (fname != NULL)
            fname++;
    }
    else
        fname++;

    if (fname == NULL)
        fname = filename;

    szMsgStart = szMsg + sprintf(szMsg, "%s:%d: ", fname, line);

    va_start(vl, lpFormat);
    uRet = (ULONG) vsprintf(szMsgStart, lpFormat, vl);
    va_end(vl);

    OutputDebugStringA(szMsg);

    return uRet;
}

#define DbgPrint(fmt, ...) \
    Win32DbgPrint(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

static inline void DbgDumpMenuInternal(HMENU hmenu, char* padding, int padlevel)
{
    WCHAR label[128];
    int i;
    int count = GetMenuItemCount(hmenu);

    padding[padlevel] = '.';
    padding[padlevel + 1] = '.';
    padding[padlevel + 2] = 0;

    for (i = 0; i < count; i++)
    {
        MENUITEMINFOW mii = { 0 };

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_SUBMENU | MIIM_STATE | MIIM_ID;
        mii.dwTypeData = label;
        mii.cch = _countof(label);

        GetMenuItemInfo(hmenu, i, TRUE, &mii);

        if (mii.fType & MFT_BITMAP)
            DbgPrint("%s%2d - %08x: BITMAP %08p (state=%d, has submenu=%s)\n", padding, i, mii.wID, mii.hbmpItem, mii.fState, mii.hSubMenu ? "TRUE" : "FALSE");
        else if (mii.fType & MFT_SEPARATOR)
            DbgPrint("%s%2d - %08x ---SEPARATOR---\n", padding, i, mii.wID);
        else
            DbgPrint("%s%2d - %08x: %S (state=%d, has submenu=%s)\n", padding, i, mii.wID, mii.dwTypeData, mii.fState, mii.hSubMenu ? "TRUE" : "FALSE");

        if (mii.hSubMenu)
            DbgDumpMenuInternal(mii.hSubMenu, padding, padlevel + 2);

    }

    padding[padlevel] = 0;
}

static __inline void DbgDumpMenu(HMENU hmenu)
{
    char padding[128];
    DbgDumpMenuInternal(hmenu, padding, 0);
}

#if 1
#define FAILED_UNEXPECTEDLY(hr) (FAILED(hr) && (DbgPrint("Unexpected failure %08x.\n", hr), TRUE))
#else
#define FAILED_UNEXPECTEDLY(hr) FAILED(hr)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#ifdef __cplusplus
template <class Base>
class CComDebugObject : public Base
{
public:
    CComDebugObject(void * = NULL)
    {
#if DEBUG_CCOMOBJECT_CREATION
        DbgPrint("%S, this=%08p\n", __FUNCTION__, static_cast<Base*>(this));
#endif
        _pAtlModule->Lock();
    }

    virtual ~CComDebugObject()
    {
        this->FinalRelease();
        _pAtlModule->Unlock();
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        int rc = this->InternalAddRef();
#if DEBUG_CCOMOBJECT_REFCOUNTING
        DbgPrint("%s, RefCount is now %d(--)! \n", __FUNCTION__, rc);
#endif
        return rc;
    }

    STDMETHOD_(ULONG, Release)()
    {
        int rc = this->InternalRelease();

#if DEBUG_CCOMOBJECT_REFCOUNTING
        DbgPrint("%s, RefCount is now %d(--)! \n", __FUNCTION__, rc);
#endif

        if (rc == 0)
        {
#if DEBUG_CCOMOBJECT_DESTRUCTION
            DbgPrint("%s, RefCount reached 0 Deleting!\n", __FUNCTION__);
#endif
            delete this;
        }
        return rc;
    }

    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
    {
        return this->_InternalQueryInterface(iid, ppvObject);
    }

    static HRESULT WINAPI CreateInstance(CComDebugObject<Base> **pp)
    {
        CComDebugObject<Base>				*newInstance;
        HRESULT								hResult;

        ATLASSERT(pp != NULL);
        if (pp == NULL)
            return E_POINTER;

        hResult = E_OUTOFMEMORY;
        newInstance = NULL;
        ATLTRY(newInstance = new CComDebugObject<Base>());
        if (newInstance != NULL)
        {
            newInstance->SetVoid(NULL);
            newInstance->InternalFinalConstructAddRef();
            hResult = newInstance->_AtlInitialConstruct();
            if (SUCCEEDED(hResult))
                hResult = newInstance->FinalConstruct();
            if (SUCCEEDED(hResult))
                hResult = newInstance->_AtlFinalConstruct();
            newInstance->InternalFinalConstructRelease();
            if (hResult != S_OK)
            {
                delete newInstance;
                newInstance = NULL;
            }
        }
        *pp = newInstance;
        return hResult;
    }
};

#ifdef DEBUG_CCOMOBJECT
#   define _CComObject CComDebugObject
#else
#   define _CComObject CComObject
#endif

template<class T>
void ReleaseCComPtrExpectZero(CComPtr<T>& cptr, BOOL forceRelease = FALSE)
{
    if (cptr.p != NULL)
    {
        int nrc = cptr->Release();
        if (nrc > 0)
        {
            DbgPrint("WARNING: Unexpected RefCount > 0 (%d)!\n", nrc);
            if (forceRelease)
            {
                while (nrc > 0)
                {
                    nrc = cptr->Release();
                }
            }
        }
        cptr.Detach();
    }
}

template<class T, class R>
HRESULT inline ShellDebugObjectCreator(REFIID riid, R ** ppv)
{
    CComPtr<T>       obj;
    HRESULT          hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY(obj = new CComDebugObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, reinterpret_cast<void **>(ppv));
    if (FAILED(hResult))
        return hResult;
    return S_OK;
}

template<class T, class R>
HRESULT inline ShellObjectCreator(REFIID riid, R ** ppv)
{
    CComPtr<T>       obj;
    HRESULT          hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
ATLTRY(obj = new _CComObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, reinterpret_cast<void **>(ppv));
    if (FAILED(hResult))
        return hResult;
    return S_OK;
}

template<class T, class R>
HRESULT inline ShellObjectCreatorInit(REFIID riid, R ** ppv)
{
    CComPtr<T>  obj;
    CComPtr<R>  result;
    HRESULT     hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY(obj = new _CComObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, reinterpret_cast<void **>(&result));
    if (FAILED(hResult))
        return hResult;

    hResult = obj->Initialize();
    if (FAILED(hResult))
        return hResult;

    *ppv = result.Detach();

    return S_OK;
}

template<class T>
HRESULT inline ShellObjectCreatorInit(REFIID riid, void ** ppv)
{
    CComPtr<T>  obj;
    CComPtr<IUnknown>  result;
    HRESULT     hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY(obj = new _CComObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, reinterpret_cast<void **>(&result));
    if (FAILED(hResult))
        return hResult;

    hResult = obj->Initialize();
    if (FAILED(hResult))
        return hResult;

    *ppv = result.Detach();

    return S_OK;
}

template<class T, class T1>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, REFIID riid, void ** ppv)
{
    CComPtr<T>  obj;
    HRESULT     hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY(obj = new _CComObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, ppv);
    if (FAILED(hResult))
        return hResult;

    hResult = obj->Initialize(initArg1);
    if (FAILED(hResult))
        return hResult;

    return S_OK;
}

template<class T, class T1, class R>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, REFIID riid, R ** ppv)
{
    CComPtr<T>  obj;
    CComPtr<R>  result;
    HRESULT     hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY(obj = new _CComObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, reinterpret_cast<void **>(&result));
    if (FAILED(hResult))
        return hResult;

    hResult = obj->Initialize(initArg1);
    if (FAILED(hResult))
        return hResult;

    *ppv = result.Detach();

    return S_OK;
}

template<class T, class T1, class T2, class R>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, T2 initArg2, REFIID riid, R ** ppv)
{
    CComPtr<T>  obj;
    CComPtr<R>  result;
    HRESULT     hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY(obj = new _CComObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, reinterpret_cast<void **>(&result));
    if (FAILED(hResult))
        return hResult;

    hResult = obj->Initialize(initArg1, initArg2);
    if (FAILED(hResult))
        return hResult;

    *ppv = result.Detach();

    return S_OK;
}

template<class T, class T1, class T2, class T3, class R>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, T2 initArg2, T3 initArg3, REFIID riid, R ** ppv)
{
    CComPtr<T>  obj;
    CComPtr<R>  result;
    HRESULT     hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY(obj = new _CComObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, reinterpret_cast<void **>(&result));
    if (FAILED(hResult))
        return hResult;

    hResult = obj->Initialize(initArg1, initArg2, initArg3);
    if (FAILED(hResult))
        return hResult;

    *ppv = result.Detach();

    return S_OK;
}

template<class T, class T1, class T2, class T3, class T4, class R>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, T2 initArg2, T3 initArg3, T4 initArg4, REFIID riid, R ** ppv)
{
    CComPtr<T>  obj;
    CComPtr<R>  result;
    HRESULT     hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY(obj = new _CComObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, reinterpret_cast<void **>(&result));
    if (FAILED(hResult))
        return hResult;

    hResult = obj->Initialize(initArg1, initArg2, initArg3, initArg4);
    if (FAILED(hResult))
        return hResult;

    *ppv = result.Detach();

    return S_OK;
}

template<class T, class T1, class T2, class T3, class T4, class T5, class R>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, T2 initArg2, T3 initArg3, T4 initArg4, T5 initArg5, REFIID riid, R ** ppv)
{
    CComPtr<T>  obj;
    CComPtr<R>  result;
    HRESULT     hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY(obj = new _CComObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, reinterpret_cast<void **>(&result));
    if (FAILED(hResult))
        return hResult;

    hResult = obj->Initialize(initArg1, initArg2, initArg3, initArg4, initArg5);
    if (FAILED(hResult))
        return hResult;

    *ppv = result.Detach();

    return S_OK;
}
#endif /* __cplusplus */

#endif /* __WINE_UNDOCSHELL_H */
