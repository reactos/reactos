/*
 * defines helperfunctions to manipulate the contents of a IShellFolder
 *
 * Copyright 2000 Juergen Schmied
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

#ifndef __WINE_SHELLFOLDER_HELP_H
#define __WINE_SHELLFOLDER_HELP_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "shlobj.h"

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_ISFHelper, 0x1fe68efbL, 0x1874, 0x9812, 0x56, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
typedef struct ISFHelper ISFHelper, *LPISFHELPER;

/*****************************************************************************
 * ISFHelper interface
 */

#define INTERFACE ISFHelper
#define ISFHelper_METHODS \
	IUnknown_METHODS \
	STDMETHOD(GetUniqueName)(THIS_ LPSTR  lpName, UINT  uLen) PURE; \
	STDMETHOD(AddFolder)(THIS_ HWND  hwnd, LPCSTR  lpName, LPITEMIDLIST * ppidlOut) PURE; \
	STDMETHOD(DeleteItems)(THIS_ UINT  cidl, LPCITEMIDLIST * apidl) PURE; \
	STDMETHOD(CopyItems)(THIS_ IShellFolder * pSFFrom, UINT  cidl, LPCITEMIDLIST * apidl) PURE;
ICOM_DEFINE(ISFHelper, IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define ISFHelper_QueryInterface(p,a,b)         (p)->lpVtbl->QueryInterface(p,a,b)
#define ISFHelper_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define ISFHelper_Release(p)                    (p)->lpVtbl->Release(p)
/*** ISFHelper methods ***/
#define ISFHelper_GetUniqueName(p,a,b)          (p)->lpVtbl->GetUniqueName(p,a,b)
#define ISFHelper_AddFolder(p,a,b,c)            (p)->lpVtbl->AddFolder(p,a,b,c)
#define ISFHelper_DeleteItems(p,a,b)            (p)->lpVtbl->DeleteItems(p,a,b)
#define ISFHelper_CopyItems(p,a,b,c)            (p)->lpVtbl->CopyItems(p,a,b,c)
#endif

#endif /* __WINE_SHELLFOLDER_HELP_H */
