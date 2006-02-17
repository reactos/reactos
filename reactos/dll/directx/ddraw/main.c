
/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              DirectDraw Library 
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include <windows.h>
#include "rosdraw.h"


HRESULT WINAPI Create_DirectDraw (LPGUID pGUID, LPDIRECTDRAW* pIface, REFIID id, BOOL ex)
{   
    IDirectDrawImpl* This = (IDirectDrawImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawImpl));

	if (This == NULL) 
		return E_OUTOFMEMORY;

	ZeroMemory(This,sizeof(IDirectDrawImpl));

	This->lpVtbl = &DirectDraw7_Vtable;
	This->lpVtbl_v1 = &DDRAW_IDirectDraw_VTable;
	This->lpVtbl_v2 = &DDRAW_IDirectDraw2_VTable;
	This->lpVtbl_v4 = &DDRAW_IDirectDraw4_VTable;

	This->DirectDrawGlobal.dwRefCnt = 1;
	*pIface = (LPDIRECTDRAW)This;

	if(This->lpVtbl->QueryInterface ((LPDIRECTDRAW7)This, id, (void**)&pIface) != S_OK)
		return DDERR_INVALIDPARAMS;

	return This->lpVtbl->Initialize ((LPDIRECTDRAW7)This, pGUID);
}

HRESULT WINAPI DirectDrawCreate (LPGUID lpGUID, LPDIRECTDRAW* lplpDD, LPUNKNOWN pUnkOuter) 
{   
	/* check see if pUnkOuter is null or not */
	if (pUnkOuter)
	{
		/* we do not use same error code as MS, ms use 0x8004110 */
		return DDERR_INVALIDPARAMS; 
	}
	
	return Create_DirectDraw (lpGUID, lplpDD, &IID_IDirectDraw, FALSE);
}
 
HRESULT WINAPI DirectDrawCreateEx(LPGUID lpGUID, LPVOID* lplpDD, REFIID id, LPUNKNOWN pUnkOuter)
{    	
	/* check see if pUnkOuter is null or not */
	if (pUnkOuter)
	{
		/* we do not use same error code as MS, ms use 0x8004110 */
		return DDERR_INVALIDPARAMS; 
	}
	
	/* Is it a DirectDraw 7 Request or not */
	if (!IsEqualGUID(id, &IID_IDirectDraw7)) 
	{
	  return DDERR_INVALIDPARAMS;
	}

    return Create_DirectDraw (lpGUID, (LPDIRECTDRAW*)lplpDD, id, TRUE);
}

HRESULT WINAPI DirectDrawEnumerateA(
  LPDDENUMCALLBACKA lpCallback, 
  LPVOID lpContext
)
{
    DX_STUB;
}


HRESULT WINAPI DirectDrawEnumerateW(
  LPDDENUMCALLBACKW lpCallback, 
  LPVOID lpContext
)
{
    DX_STUB;
}

HRESULT WINAPI DirectDrawEnumerateExA(
  LPDDENUMCALLBACKEXA lpCallback, 
  LPVOID lpContext, 
  DWORD dwFlags
)
{
    DX_STUB;
}

HRESULT WINAPI DirectDrawEnumerateExW(
  LPDDENUMCALLBACKEXW lpCallback, 
  LPVOID lpContext, 
  DWORD dwFlags
)
{
     DX_STUB;
}
 
