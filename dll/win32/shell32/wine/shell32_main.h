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

#ifdef __cplusplus
extern "C" {
#endif

#include "shell32_version.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

/*******************************************
*  global SHELL32.DLL variables
*/
extern HMODULE	huser32 DECLSPEC_HIDDEN;
extern HINSTANCE shell32_hInstance DECLSPEC_HIDDEN;

BOOL WINAPI Shell_GetImageLists(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList);

/* Iconcache */
#define INVALID_INDEX -1
BOOL SIC_Initialize(void);
void SIC_Destroy(void) DECLSPEC_HIDDEN;
BOOL PidlToSicIndex (IShellFolder * sh, LPCITEMIDLIST pidl, BOOL bBigIcon, UINT uFlags, int * pIndex) DECLSPEC_HIDDEN;
INT SIC_GetIconIndex (LPCWSTR sSourceFile, INT dwSourceIndex, DWORD dwFlags ) DECLSPEC_HIDDEN;

/* Classes Root */
HRESULT HCR_GetProgIdKeyOfExtension(PCWSTR szExtension, PHKEY phKey, BOOL AllowFallback);
BOOL HCR_MapTypeToValueW(LPCWSTR szExtension, LPWSTR szFileType, LONG len, BOOL bPrependDot) DECLSPEC_HIDDEN;
BOOL HCR_GetDefaultVerbW( HKEY hkeyClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len ) DECLSPEC_HIDDEN;
BOOL HCR_GetExecuteCommandW( HKEY hkeyClass, LPCWSTR szClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len ) DECLSPEC_HIDDEN;
BOOL HCR_GetIconW(LPCWSTR szClass, LPWSTR szDest, LPCWSTR szName, DWORD len, int* picon_idx);
BOOL HCR_GetClassNameW(REFIID riid, LPWSTR szDest, DWORD len) DECLSPEC_HIDDEN;

#ifdef __REACTOS__
/* Current User */
BOOL HCU_GetIconW(LPCWSTR szClass, LPWSTR szDest, LPCWSTR szName, DWORD len, int* picon_idx) DECLSPEC_HIDDEN;

/* Local Machine */
BOOL HLM_GetIconW(int reg_idx, LPWSTR szDest, DWORD len, int* picon_idx) DECLSPEC_HIDDEN;
#endif

/* ANSI versions of above functions, supposed to go away as soon as they are not used anymore */
BOOL HCR_MapTypeToValueA(LPCSTR szExtension, LPSTR szFileType, LONG len, BOOL bPrependDot) DECLSPEC_HIDDEN;
BOOL HCR_GetIconA(LPCSTR szClass, LPSTR szDest, LPCSTR sName, DWORD len, int* picon_idx);
BOOL HCR_GetClassNameA(REFIID riid, LPSTR szDest, DWORD len) DECLSPEC_HIDDEN;

BOOL HCR_GetFolderAttributes(LPCITEMIDLIST pidlFolder, LPDWORD dwAttributes) DECLSPEC_HIDDEN;

/* File associations */
#define SHELL32_AssocGetFolderDescription SHELL32_AssocGetFSDirectoryDescription
HRESULT SHELL32_AssocGetFSDirectoryDescription(PWSTR Buf, UINT cchBuf);
HRESULT SHELL32_AssocGetFileDescription(PCWSTR Name, PWSTR Buf, UINT cchBuf);


DWORD WINAPI ParseFieldA(LPCSTR src, DWORD nField, LPSTR dst, DWORD len) DECLSPEC_HIDDEN;
DWORD WINAPI ParseFieldW(LPCWSTR src, DWORD nField, LPWSTR dst, DWORD len) DECLSPEC_HIDDEN;

/****************************************************************************
 * Class constructors
 */
HRESULT IDataObject_Constructor(HWND hwndOwner, LPCITEMIDLIST pMyPidl, PCUITEMID_CHILD_ARRAY apidl, UINT cidl, BOOL bExtendedObject, IDataObject **dataObject);
HRESULT IEnumFORMATETC_Constructor(UINT cfmt, const FORMATETC afmt[], IEnumFORMATETC **enumerator);

LPCLASSFACTORY	IClassFactory_Constructor(REFCLSID);
IContextMenu2 *	ISvItemCm_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST pidl, const LPCITEMIDLIST *aPidls, UINT uItemCount);
HRESULT WINAPI INewItem_Constructor(IUnknown * pUnkOuter, REFIID riif, LPVOID *ppv);
IContextMenu2 * ISvStaticItemCm_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST pidl, LPCITEMIDLIST *apidl, UINT cidl, HKEY hKey);
IContextMenu2 *	ISvBgCm_Constructor(LPSHELLFOLDER pSFParent, BOOL bDesktop);
HRESULT WINAPI CDefViewDual_Constructor(REFIID riid, LPVOID * ppvOut);
HRESULT WINAPI CShellDispatch_Constructor(REFIID riid, LPVOID * ppvOut);

HRESULT WINAPI IShellLink_ConstructFromPath(WCHAR *path, REFIID riid, LPVOID *ppv);
HRESULT WINAPI IShellLink_ConstructFromFile(IShellFolder * psf, LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv);
HRESULT WINAPI IFileSystemBindData_Constructor(const WIN32_FIND_DATAW *pfd, LPBC *ppV);
HRESULT WINAPI CPanel_ExtractIconA(LPITEMIDLIST pidl, LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize) DECLSPEC_HIDDEN;
HRESULT WINAPI CPanel_ExtractIconW(LPITEMIDLIST pidl, LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize) DECLSPEC_HIDDEN;

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
    (((kst)&(MK_CONTROL)) ? DROPEFFECT_COPY :\
    (((kst)&(MK_SHIFT)) ? DROPEFFECT_MOVE :\
    DROPEFFECT_NONE)))


HGLOBAL RenderHDROP(LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl) DECLSPEC_HIDDEN;
HGLOBAL RenderSHELLIDLIST (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl) DECLSPEC_HIDDEN;
HGLOBAL RenderFILENAMEA (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl) DECLSPEC_HIDDEN;
HGLOBAL RenderFILENAMEW (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl) DECLSPEC_HIDDEN;

/* Change Notification */
void InitChangeNotifications(void) DECLSPEC_HIDDEN;
void FreeChangeNotifications(void) DECLSPEC_HIDDEN;

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

static __inline BOOL SHELL_OsIsUnicode(void)
{
    /* if high-bit of version is 0, we are emulating NT */
    return !(GetVersion() & 0x80000000);
}

#define __SHFreeAndNil(ptr) \
	{\
	  SHFree(*ptr); \
	  *ptr = NULL; \
	};
static __inline void __SHCloneStrA(char **target, const char *source)
{
	*target = (char *)SHAlloc(strlen(source) + 1);
	strcpy(*target, source);
}

static __inline void __SHCloneStrWtoA(char **target, const WCHAR *source)
{
	int len = WideCharToMultiByte(CP_ACP, 0, source, -1, NULL, 0, NULL, NULL);
	*target = (char *)SHAlloc(len);
	WideCharToMultiByte(CP_ACP, 0, source, -1, *target, len, NULL, NULL);
}

static __inline void __SHCloneStrW(WCHAR **target, const WCHAR *source)
{
	*target = (WCHAR *)SHAlloc((lstrlenW(source) + 1) * sizeof(WCHAR) );
	lstrcpyW(*target, source);
}

static __inline LPWSTR __SHCloneStrAtoW(WCHAR **target, const char *source)
{
	int len = MultiByteToWideChar(CP_ACP, 0, source, -1, NULL, 0);
	*target = (WCHAR *)SHAlloc(len * sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, source, -1, *target, len);
	return *target;
}

/* handle conversions */
#define HICON_16(h32)		(LOWORD(h32))
#define HICON_32(h16)		((HICON)(ULONG_PTR)(h16))
#define HINSTANCE_32(h16)	((HINSTANCE)(ULONG_PTR)(h16))
#define HINSTANCE_16(h32)	(LOWORD(h32))

extern WCHAR swShell32Name[MAX_PATH] DECLSPEC_HIDDEN;

BOOL UNIXFS_is_rooted_at_desktop(void) DECLSPEC_HIDDEN;
extern const GUID CLSID_UnixFolder DECLSPEC_HIDDEN;
extern const GUID CLSID_UnixDosFolder DECLSPEC_HIDDEN;

/* Default shell folder value registration */
HRESULT SHELL_RegisterShellFolders(void) DECLSPEC_HIDDEN;

/* Detect Shell Links */
BOOL SHELL_IsShortcut(LPCITEMIDLIST) DECLSPEC_HIDDEN;

HPROPSHEETPAGE SH_CreatePropertySheetPage(WORD wDialogId, DLGPROC pfnDlgProc, LPARAM lParam, LPCWSTR pwszTitle);
HPROPSHEETPAGE SH_CreatePropertySheetPageEx(WORD wDialogId, DLGPROC pfnDlgProc, LPARAM lParam,
                                            LPCWSTR pwszTitle, LPFNPSPCALLBACK Callback);
LPWSTR SH_FormatFileSizeWithBytes(PULARGE_INTEGER lpQwSize, LPWSTR pszBuf, UINT cchBuf);

HRESULT WINAPI DoRegisterServer(void);
HRESULT WINAPI DoUnregisterServer(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __WINE_SHELL_MAIN_H */
