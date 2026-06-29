/*
 * Temporary place for ole2 stubs.
 *
 * Copyright (C) 1999 Corel Corporation
 * Move these functions to dlls/ole32/ole2.c when you implement them.
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
#include "winuser.h"
#include "ole2.h"
#include "objidl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/******************************************************************************
 *               OleCreateLinkToFile        [OLE32.@]
 */
HRESULT WINAPI  OleCreateLinkToFile(LPCOLESTR lpszFileName, REFIID riid,
	  		DWORD renderopt, LPFORMATETC lpFormatEtc,
			LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj)
{
    FIXME("%p, %p, %ld, %p, %p, %p, %p stub!\n",lpszFileName, riid, renderopt, lpFormatEtc, pClientSite, pStg, ppvObj);
    return E_NOTIMPL;
}

/******************************************************************************
 *              OleCreateLink        [OLE32.@]
 */
HRESULT WINAPI OleCreateLink(LPMONIKER pmkLinkSrc, REFIID riid, DWORD renderopt, LPFORMATETC lpFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID* ppvObj)
{
  FIXME("(not shown), stub!\n");
  return E_NOTIMPL;
}

/******************************************************************************
 *              OleGetIconOfClass        [OLE32.@]
 */
HGLOBAL WINAPI OleGetIconOfClass(REFCLSID rclsid, LPOLESTR lpszLabel, BOOL fUseTypeAsLabel)
{
  FIXME("(%p,%p,%x), stub!\n", rclsid, lpszLabel, fUseTypeAsLabel);
  return NULL;
}

/***********************************************************************
 *              OleGetIconOfFile        [OLE32.@]
 */
HGLOBAL WINAPI OleGetIconOfFile(LPOLESTR path, BOOL use_file_as_label)
{
    FIXME("(%s, %d), stub!\n", debugstr_w(path), use_file_as_label);
    return NULL;
}

/***********************************************************************
 *           OleRegEnumFormatEtc    [OLE32.@]
 */
HRESULT WINAPI DECLSPEC_HOTPATCH OleRegEnumFormatEtc (
  REFCLSID clsid,
  DWORD    dwDirection,
  LPENUMFORMATETC* ppenumFormatetc)
{
    FIXME("%p, %ld, %p stub!\n", clsid, dwDirection, ppenumFormatetc);

    return E_NOTIMPL;
}
