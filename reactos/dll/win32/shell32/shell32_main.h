/*
 * 	internal Shell32 Library definitions
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Juergen Schmied (jsch)  *  <juergen.schmied@metronet.de>
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

#ifndef __WINE_SHELL_MAIN_H
#define __WINE_SHELL_MAIN_H

#include <stdarg.h>
#include <shlobj.h>

/*
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "objbase.h"
#include "docobj.h"
#include "undocshell.h"
#include "shlobj.h"
#include "shellapi.h"
#include "wine/windef16.h"
#include "wine/unicode.h"

*/
/*******************************************
*  global SHELL32.DLL variables
*/
extern HMODULE	huser32;
extern HINSTANCE shell32_hInstance;
extern HIMAGELIST	ShellSmallIconList;
extern HIMAGELIST	ShellBigIconList;

BOOL WINAPI Shell_GetImageLists(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList);

/* Iconcache */
#define INVALID_INDEX -1
BOOL SIC_Initialize(void);
void SIC_Destroy(void);
BOOL PidlToSicIndex (IShellFolder * sh, LPCITEMIDLIST pidl, BOOL bBigIcon, UINT uFlags, int * pIndex);
INT SIC_GetIconIndex (LPCWSTR sSourceFile, INT dwSourceIndex, DWORD dwFlags );

/* Classes Root */
BOOL HCR_MapTypeToValueW(LPCWSTR szExtension, LPWSTR szFileType, LONG len, BOOL bPrependDot);
BOOL HCR_GetDefaultVerbW( HKEY hkeyClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len );
BOOL HCR_GetExecuteCommandW( HKEY hkeyClass, LPCWSTR szClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len );
BOOL HCR_GetDefaultIconW(LPCWSTR szClass, LPWSTR szDest, DWORD len, int* picon_idx);
BOOL HCR_GetDefaultIconFromGUIDW(REFIID riid, LPWSTR szDest, DWORD len, int* picon_idx);
BOOL HCR_GetClassNameW(REFIID riid, LPWSTR szDest, DWORD len);

/* ANSI versions of above functions, supposed to go away as soon as they are not used anymore */
BOOL HCR_MapTypeToValueA(LPCSTR szExtension, LPSTR szFileType, LONG len, BOOL bPrependDot);
BOOL HCR_GetDefaultIconA(LPCSTR szClass, LPSTR szDest, DWORD len, int* picon_idx);
BOOL HCR_GetClassNameA(REFIID riid, LPSTR szDest, DWORD len);

BOOL HCR_GetFolderAttributes(LPCITEMIDLIST pidlFolder, LPDWORD dwAttributes);

INT_PTR CALLBACK AboutDlgProc(HWND,UINT,WPARAM,LPARAM);
DWORD WINAPI ParseFieldA(LPCSTR src, DWORD nField, LPSTR dst, DWORD len);
DWORD WINAPI ParseFieldW(LPCWSTR src, DWORD nField, LPWSTR dst, DWORD len);

/****************************************************************************
 * Class constructors
 */
LPDATAOBJECT	IDataObject_Constructor(HWND hwndOwner, LPCITEMIDLIST myPidl, LPCITEMIDLIST * apidl, UINT cidl);
LPENUMFORMATETC	IEnumFORMATETC_Constructor(UINT, const FORMATETC []);

LPCLASSFACTORY	IClassFactory_Constructor(REFCLSID);
IContextMenu2 *	ISvItemCm_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST pidl, const LPCITEMIDLIST *aPidls, UINT uItemCount);
HRESULT WINAPI INewItem_Constructor(IUnknown * pUnkOuter, REFIID riif, LPVOID *ppv);
IContextMenu2 * ISvStaticItemCm_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST pidl, LPCITEMIDLIST *apidl, UINT cidl, HKEY hKey);
IContextMenu2 *	ISvBgCm_Constructor(LPSHELLFOLDER pSFParent, BOOL bDesktop);
LPSHELLVIEW	IShellView_Constructor(LPSHELLFOLDER);

HRESULT WINAPI IFSFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IShellLink_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IShellLink_ConstructFromFile(IUnknown * pUnkOuter, REFIID riid, LPCITEMIDLIST pidl, LPVOID * ppv);
HRESULT WINAPI ISF_Desktop_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI ISF_MyComputer_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI ISF_Printers_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI ISF_MyDocuments_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI ISF_NetworkPlaces_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI ISF_Fonts_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI ISF_AdminTools_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IDropTargetHelper_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IFileSystemBindData_Constructor(const WIN32_FIND_DATAW *pfd, LPBC *ppV);
HRESULT WINAPI IControlPanel_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI UnixFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI UnixDosFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv);
HRESULT WINAPI FolderShortcut_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv);
HRESULT WINAPI MyDocuments_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv);
HRESULT WINAPI RecycleBin_Constructor(IUnknown * pUnkOuter, REFIID riif, LPVOID *ppv);
HRESULT WINAPI SHEOW_Constructor(IUnknown * pUnkOuter, REFIID riif, LPVOID *ppv);
HRESULT WINAPI ShellFSFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv);
HRESULT WINAPI StartMenu_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv);
extern HRESULT CPanel_GetIconLocationW(LPCITEMIDLIST, LPWSTR, UINT, int*);
HRESULT WINAPI CPanel_ExtractIconA(LPITEMIDLIST pidl, LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
HRESULT WINAPI CPanel_ExtractIconW(LPITEMIDLIST pidl, LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

HRESULT WINAPI IAutoComplete_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

LPEXTRACTICONA	IExtractIconA_Constructor(LPCITEMIDLIST);
LPEXTRACTICONW	IExtractIconW_Constructor(LPCITEMIDLIST);

/* initialisation for FORMATETC */
#define InitFormatEtc(fe, cf, med) \
	{\
	(fe).cfFormat=cf;\
	(fe).dwAspect=DVASPECT_CONTENT;\
	(fe).ptd=NULL;\
	(fe).tymed=med;\
	(fe).lindex=-1;\
	};

#define KeyStateToDropEffect(kst)\
    ((((kst)&(MK_CONTROL|MK_SHIFT))==(MK_CONTROL|MK_SHIFT)) ? DROPEFFECT_LINK :\
    (((kst)&(MK_CONTROL|MK_SHIFT)) ? DROPEFFECT_COPY :\
    DROPEFFECT_MOVE))


HGLOBAL RenderHDROP(LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderSHELLIDLIST (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderSHELLIDLISTOFFSET (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderFILECONTENTS (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderFILEDESCRIPTOR (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderFILENAMEA (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderFILENAMEW (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderPREFEREDDROPEFFECT (DWORD dwFlags);

/* Change Notification */
void InitChangeNotifications(void);
void FreeChangeNotifications(void);
void InitIconOverlays(void);

/* file operation */
#define ASK_DELETE_FILE           1
#define ASK_DELETE_FOLDER         2
#define ASK_DELETE_MULTIPLE_ITEM  3
#define ASK_CREATE_FOLDER         4
#define ASK_OVERWRITE_FILE        5
#define ASK_DELETE_SELECTED       6
#define ASK_TRASH_FILE            7
#define ASK_TRASH_FOLDER          8
#define ASK_TRASH_MULTIPLE_ITEM   9
#define ASK_CANT_TRASH_ITEM      10
#define ASK_OVERWRITE_FOLDER     11

BOOL SHELL_DeleteDirectoryW(HWND hwnd, LPCWSTR pwszDir, BOOL bShowUI);
BOOL SHELL_ConfirmYesNoW(HWND hWnd, int nKindOfDialog, LPCWSTR szDir);

void WINAPI _InsertMenuItemW (HMENU hmenu, UINT indexMenu, BOOL fByPosition,
			UINT wID, UINT fType, LPCWSTR dwTypeData, UINT fState);

static BOOL __inline SHELL_OsIsUnicode(void)
{
    /* if high-bit of version is 0, we are emulating NT */
    return !(GetVersion() & 0x80000000);
}

#define __SHFreeAndNil(ptr) \
	{\
	  SHFree(*ptr); \
	  *ptr = NULL; \
	};
static void __inline __SHCloneStrA(char ** target,const char * source)
{
	*target = SHAlloc(strlen(source)+1);
	strcpy(*target, source);
}

static void __inline __SHCloneStrWtoA(char ** target, const WCHAR * source)
{
	int len = WideCharToMultiByte(CP_ACP, 0, source, -1, NULL, 0, NULL, NULL);
	*target = SHAlloc(len);
	WideCharToMultiByte(CP_ACP, 0, source, -1, *target, len, NULL, NULL);
}

static void __inline __SHCloneStrW(WCHAR ** target, const WCHAR * source)
{
	*target = SHAlloc( (lstrlenW(source)+1) * sizeof(WCHAR) );
	lstrcpyW(*target, source);
}

static LPWSTR __inline __SHCloneStrAtoW(WCHAR ** target, const char * source)
{
	int len = MultiByteToWideChar(CP_ACP, 0, source, -1, NULL, 0);
	*target = SHAlloc(len*sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, source, -1, *target, len);
	return *target;
}

/* handle conversions */
#define HICON_16(h32)		(LOWORD(h32))
#define HICON_32(h16)		((HICON)(ULONG_PTR)(h16))
#define HINSTANCE_32(h16)	((HINSTANCE)(ULONG_PTR)(h16))
#define HINSTANCE_16(h32)	(LOWORD(h32))

typedef UINT_PTR (*SHELL_ExecuteW32)(const WCHAR *lpCmd, WCHAR *env, BOOL shWait,
			    const SHELLEXECUTEINFOW *sei, LPSHELLEXECUTEINFOW sei_out);

BOOL SHELL_execute(LPSHELLEXECUTEINFOW sei, SHELL_ExecuteW32 execfunc);

extern WCHAR swShell32Name[MAX_PATH];

BOOL UNIXFS_is_rooted_at_desktop(void);
extern const GUID CLSID_UnixFolder;
extern const GUID CLSID_UnixDosFolder;

/* Default shell folder value registration */
HRESULT SHELL_RegisterShellFolders(void);

/* Detect Shell Links */
BOOL SHELL_IsShortcut(LPCITEMIDLIST);

INT_PTR CALLBACK SH_FileGeneralDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SH_FileVersionDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
HPROPSHEETPAGE SH_CreatePropertySheetPage(LPSTR resname, DLGPROC dlgproc, LPARAM lParam, LPWSTR szTitle);
BOOL SH_ShowDriveProperties(WCHAR * drive, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST * apidl);
BOOL SH_ShowRecycleBinProperties(WCHAR sDrive);
BOOL SH_ShowPropertiesDialog(LPWSTR lpf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST * apidl);
BOOL SH_ShowFolderProperties(LPWSTR pwszFolder, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST * apidl);
#endif
