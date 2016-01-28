/*
 *  Common Dialog Boxes interface (32 bit)
 *
 * Copyright 1998 Bertho A. Stultiens
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

#ifndef _WINE_DLL_CDLG_H
#define _WINE_DLL_CDLG_H

#include <wine/config.h>

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <wincon.h>
#include <winternl.h>
#include <objbase.h>
#include <commdlg.h>
#include <shlobj.h>
#include <dlgs.h>
#include <cderr.h>

/* RegGetValueW is supported by Win2k3 SP1 but headers need Win Vista */
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <winreg.h>

#define NO_SHLWAPI_STREAM
#include <shlwapi.h>

#include <wine/unicode.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "filedlgbrowser.h"
#include "resource.h"

/* Common dialogs implementation globals */
#define COMDLG32_Atom   MAKEINTATOM(0xa000)     /* MS uses this one to identify props */

extern HINSTANCE	COMDLG32_hInstance DECLSPEC_HIDDEN;

void	COMDLG32_SetCommDlgExtendedError(DWORD err) DECLSPEC_HIDDEN;
LPVOID	COMDLG32_AllocMem(int size) __WINE_ALLOC_SIZE(1) DECLSPEC_HIDDEN;

/* Find/Replace local definitions */

#define FR_WINE_UNICODE		0x80000000
#define FR_WINE_REPLACE		0x40000000

typedef struct {
	FINDREPLACEA	fr;	/* Internally used structure */
        union {
		FINDREPLACEA	*fra;	/* Reference to the user supplied structure */
		FINDREPLACEW	*frw;
        } user_fr;
} COMDLG32_FR_Data;

/* Constructors */
HRESULT FileOpenDialog_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv) DECLSPEC_HIDDEN;
HRESULT FileSaveDialog_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv) DECLSPEC_HIDDEN;

/* Shared helper functions */
void COMDLG32_GetCanonicalPath(PCIDLIST_ABSOLUTE pidlAbsCurrent, LPWSTR lpstrFile, LPWSTR lpstrPathAndFile) DECLSPEC_HIDDEN;
int FILEDLG95_ValidatePathAction(LPWSTR lpstrPathAndFile, IShellFolder **ppsf,
                                 HWND hwnd, DWORD flags, BOOL isSaveDlg, int defAction) DECLSPEC_HIDDEN;
int COMDLG32_SplitFileNames(LPWSTR lpstrEdit, UINT nStrLen, LPWSTR *lpstrFileList, UINT *sizeUsed) DECLSPEC_HIDDEN;
void FILEDLG95_OnOpenMessage(HWND hwnd, int idCaption, int idText) DECLSPEC_HIDDEN;

extern BOOL GetFileName31A( OPENFILENAMEA *lpofn, UINT dlgType ) DECLSPEC_HIDDEN;
extern BOOL GetFileName31W( OPENFILENAMEW *lpofn, UINT dlgType ) DECLSPEC_HIDDEN;

/* ITEMIDLIST */

extern LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILClone) (LPCITEMIDLIST);
extern LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILCombine)(LPCITEMIDLIST,LPCITEMIDLIST);
extern LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILGetNext)(LPITEMIDLIST);
extern BOOL (WINAPI *COMDLG32_PIDL_ILRemoveLastID)(LPCITEMIDLIST);
extern BOOL (WINAPI *COMDLG32_PIDL_ILIsEqual)(LPCITEMIDLIST, LPCITEMIDLIST);
extern UINT (WINAPI *COMDLG32_PIDL_ILGetSize)(LPCITEMIDLIST);

/* SHELL */
extern LPVOID (WINAPI *COMDLG32_SHAlloc)(DWORD);
extern DWORD (WINAPI *COMDLG32_SHFree)(LPVOID);
extern BOOL (WINAPI *COMDLG32_SHGetFolderPathW)(HWND,int,HANDLE,DWORD,LPWSTR);
extern LPITEMIDLIST (WINAPI *COMDLG32_SHSimpleIDListFromPathAW)(LPCVOID);

#define ONOPEN_BROWSE 1
#define ONOPEN_OPEN   2
#define ONOPEN_SEARCH 3

#endif /* _WINE_DLL_CDLG_H */
