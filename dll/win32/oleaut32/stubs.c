/*
 * OlePro32 Stubs
 *
 * Copyright 1999 Corel Corporation
 *
 * Sean Langley
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "ole2.h"
#include "olectl.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 * OleCreatePropertyFrameIndirect (OLEAUT32.416)
 */
HRESULT WINAPI OleCreatePropertyFrameIndirect( LPOCPFIPARAMS lpParams)
{
	FIXME("(%p), not implemented (olepro32.dll)\n",lpParams);
	return S_OK;
}

/***********************************************************************
 * OleCreatePropertyFrame (OLEAUT32.417)
 */
HRESULT WINAPI OleCreatePropertyFrame(
    HWND hwndOwner, UINT x, UINT y, LPCOLESTR lpszCaption,ULONG cObjects,
    LPUNKNOWN* ppUnk, ULONG cPages, LPCLSID pPageClsID, LCID lcid,
    DWORD dwReserved, LPVOID pvReserved )
{
	FIXME("(%p,%d,%d,%s,%d,%p,%d,%p,%x,%d,%p), not implemented (olepro32.dll)\n",
		hwndOwner,x,y,debugstr_w(lpszCaption),cObjects,ppUnk,cPages,
		pPageClsID, (int)lcid,dwReserved,pvReserved);
	return S_OK;
}
