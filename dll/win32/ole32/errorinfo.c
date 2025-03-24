/*
 * ErrorInfo API
 *
 * Copyright 2000 Patrik Stridvall, Juergen Schmied
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
 *
 * NOTES:
 *
 * The errorinfo is a per-thread object. The reference is stored in the
 * TEB at offset 0xf80.
 */

#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oleauto.h"
#include "winerror.h"

#include "compobj_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 *		GetErrorInfo (OLE32.@)
 *
 * Retrieves the error information object for the current thread.
 *
 * PARAMS
 *  dwReserved [I]. Reserved. Must be zero.
 *  pperrinfo  [O]. Address where error information object will be stored on return.
 *
 * RETURNS
 *  Success: S_OK if an error information object was set for the current thread.
 *           S_FALSE if otherwise.
 *  Failure: E_INVALIDARG if dwReserved is not zero.
 *
 * NOTES
 *  This function causes the current error info object for the thread to be
 *  cleared if one was set beforehand.
 */
HRESULT WINAPI GetErrorInfo(ULONG dwReserved, IErrorInfo **pperrinfo)
{
	TRACE("(%d, %p, %p)\n", dwReserved, pperrinfo, COM_CurrentInfo()->errorinfo);

	if (dwReserved)
	{
		ERR("dwReserved (0x%x) != 0\n", dwReserved);
		return E_INVALIDARG;
	}

	if(!pperrinfo) return E_INVALIDARG;

	if (!COM_CurrentInfo()->errorinfo)
	{
	   *pperrinfo = NULL;
	   return S_FALSE;
	}

	*pperrinfo = COM_CurrentInfo()->errorinfo;
        
	/* clear thread error state */
	COM_CurrentInfo()->errorinfo = NULL;
	return S_OK;
}

/***********************************************************************
 *		SetErrorInfo (OLE32.@)
 *
 * Sets the error information object for the current thread.
 *
 * PARAMS
 *  dwReserved [I] Reserved. Must be zero.
 *  perrinfo   [I] Error info object.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG if dwReserved is not zero.
 */
HRESULT WINAPI SetErrorInfo(ULONG dwReserved, IErrorInfo *perrinfo)
{
	IErrorInfo * pei;

	TRACE("(%d, %p)\n", dwReserved, perrinfo);

	if (dwReserved)
	{
		ERR("dwReserved (0x%x) != 0\n", dwReserved);
		return E_INVALIDARG;
	}

	/* release old errorinfo */
	pei = COM_CurrentInfo()->errorinfo;
	if (pei) IErrorInfo_Release(pei);

	/* set to new value */
	COM_CurrentInfo()->errorinfo = perrinfo;
	if (perrinfo) IErrorInfo_AddRef(perrinfo);
        
	return S_OK;
}
