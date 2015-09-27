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

/****************************************************************************
 * Taskbar WM_COMMAND identifiers
 */

#define TWM_DOEXITWINDOWS (WM_USER + 342)
#define TWM_CYCLEFOCUS (WM_USER + 348)


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

int WINAPI LogoffWindowsDialog(HWND hwndOwner);
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

#define CSIDL_FOLDER_MASK	0x00ff

/* Utility functions */
#include <stdio.h>

#define SMC_EXEC 4
INT WINAPI Shell_GetCachedImageIndex(LPCWSTR szPath, INT nIndex, UINT bSimulateDoc);

HRESULT WINAPI SHGetImageList(int iImageList, REFIID riid, void **ppv);

BOOL WINAPI GUIDFromStringW(
    _In_   PCWSTR psz,
    _Out_  LPGUID pguid
    );
    
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

        GetMenuItemInfoW(hmenu, i, TRUE, &mii);

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


static inline
void DumpIdList(LPCITEMIDLIST pcidl)
{
    DbgPrint("Begin IDList Dump\n");

    for (; pcidl != NULL; pcidl = ILGetNext(pcidl))
    {
        int i;
        int cb = pcidl->mkid.cb;
        BYTE * sh = (BYTE*) &(pcidl->mkid);
        if (cb == 0) // ITEMIDLISTs are terminatedwith a null SHITEMID.
            break;
        DbgPrint("Begin SHITEMID (cb=%d)\n", cb);
        if ((cb & 3) != 0)
            DbgPrint(" - WARNING: cb is not a multiple of 4\n");
        for (i = 0; (i + 4) <= cb; i += 4)
        {
            DbgPrint(" - abID[%08x]: %02x %02x %02x %02x\n",
                     i,
                     sh[i + 0],
                     sh[i + 1],
                     sh[i + 2],
                     sh[i + 3]);
        }
        if (i < cb)
        {
            cb -= i;
            if (cb == 3)
            {
                DbgPrint(" - abID[%08x]: %02x %02x %02x --\n",
                         i,
                         sh[i + 0],
                         sh[i + 1],
                         sh[i + 2]);
            }
            else if (cb == 2)
            {
                DbgPrint(" - abID[%08x]: %02x %02x -- --\n",
                         i,
                         sh[i + 0],
                         sh[i + 1]);
            }
            else if (cb == 1)
            {
                DbgPrint(" - abID[%08x]: %02x -- -- --\n",
                         i,
                         sh[i + 0]);
            }
        }
        DbgPrint("End SHITEMID\n");
    }
    DbgPrint("End IDList Dump.\n");
}


/*****************************************************************************
 * Shell32 resources
 */
// these resources are in shell32.dll
#define IDB_GOBUTTON_NORMAL			0x0e6
#define IDB_GOBUTTON_HOT			0x0e7

// band ids in internet toolbar
#define ITBBID_MENUBAND				1
#define ITBBID_BRANDBAND			5
#define ITBBID_TOOLSBAND			2
#define ITBBID_ADDRESSBAND			4

// commands in the CGID_PrivCITCommands command group handled by the internet toolbar
// there seems to be some support for hiding the menubar and an auto hide feature that are
// unavailable in the UI
#define ITID_TEXTLABELS				3
#define ITID_TOOLBARBANDSHOWN		4
#define ITID_ADDRESSBANDSHOWN		5
#define ITID_LINKSBANDSHOWN			6
#define ITID_MENUBANDSHOWN			12
#define ITID_AUTOHIDEENABLED		13
#define ITID_CUSTOMIZEENABLED		20
#define ITID_TOOLBARLOCKED			27

// commands in the CGID_BrandCmdGroup command group handled by the brand band
#define BBID_STARTANIMATION			1
#define BBID_STOPANIMATION			2

// undocumented flags for IShellMenu::SetShellFolder
#define SMSET_UNKNOWN08				0x08
#define SMSET_UNKNOWN10				0x10

void WINAPI ShellDDEInit(BOOL bInit);
DWORD WINAPI WinList_Init(void);

IStream* WINAPI SHGetViewStream(LPCITEMIDLIST, DWORD, LPCTSTR, LPCTSTR, LPCTSTR);

EXTERN_C HRESULT WINAPI SHCreateSessionKey(REGSAM samDesired, PHKEY phKey);

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

#include <poppack.h>

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_UNDOCSHELL_H */
