/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib\ole32\Misc.c
 * PURPOSE:         Ole32.dll helper functions
 * PROGRAMMER:      jurgen van gael [jurgen.vangael@student.kuleuven.ac.be]
 * UPDATE HISTORY:
 *                  Created 14/05/2001
 */
/********************************************************************


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.


********************************************************************/
#include <ole32/ole32.h>

#if 0

WINOLEAPI PropVariantClear(PROPVARIANT *pvar){return S_OK;}
WINOLEAPI FreePropVariantArray(
  ULONG cVariants,     //Count of elements in the structure
  PROPVARIANT *rgvars  //Pointer to the PROPVARIANT structure
){return S_OK;}

WINOLEAPI PropVariantCopy(PROPVARIANT* pvarDest, const PROPVARIANT* pvarSrc){return S_OK;}

WINOLEAPI CreateDataAdviseHolder(OUT LPDATAADVISEHOLDER FAR* ppDAHolder)
{
	return S_OK;
}

WINOLEAPI CreateDataCache(IN LPUNKNOWN pUnkOuter, IN REFCLSID rclsid,
                                        IN REFIID iid, OUT LPVOID FAR* ppv)
{
	return S_OK;
}

WINOLEAPI StringFromCLSID(IN REFCLSID rclsid, OUT LPOLESTR FAR* lplpsz)
{
	return S_OK;
}

WINOLEAPI CLSIDFromString(IN LPOLESTR lpsz, OUT LPCLSID pclsid)
{
    return E_FAIL;
}

WINOLEAPI StringFromIID(IN REFIID rclsid, OUT LPOLESTR FAR* lplpsz)
{
	return S_OK;
}

WINOLEAPI IIDFromString(IN LPOLESTR lpsz, OUT LPIID lpiid)
{
	return S_OK;
}

WINOLEAPI_(BOOL) CoIsOle1Class(IN REFCLSID rclsid)
{
	return S_OK;
}

WINOLEAPI ProgIDFromCLSID (IN REFCLSID clsid, OUT LPOLESTR FAR* lplpszProgID)
{
	return S_OK;
}

WINOLEAPI CLSIDFromProgID (IN LPCOLESTR lpszProgID, OUT LPCLSID lpclsid)
{
	return S_OK;
}

WINOLEAPI CLSIDFromProgIDEx (IN LPCOLESTR lpszProgID, OUT LPCLSID lpclsid)
{
	return S_OK;
}

WINOLEAPI_(int) StringFromGUID2(IN REFGUID rguid, OUT LPOLESTR lpsz, IN int cchMax)
{
	return S_OK;
}

#endif

/******************************************************************************
 *		IsValidInterface	[OLE32.78]
 *
 * RETURNS
 *  True, if the passed pointer is a valid interface
 */
BOOL WINAPI IsValidInterface(
	LPUNKNOWN punk	/* [in] interface to be tested */
) {
	return !(
		IsBadReadPtr(punk,4)					||
		IsBadReadPtr(ICOM_VTBL(punk),4)				||
		IsBadReadPtr(ICOM_VTBL(punk)->QueryInterface,9)	||
		IsBadCodePtr((FARPROC)ICOM_VTBL(punk)->QueryInterface)
	);
}
