/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/opengl32.c
 * PURPOSE:              OpenGL32 lib
 * PROGRAMMER:           Anich Gregor (blight), Royce Mitchell III
 * UPDATE HISTORY:
 *                       Feb 1, 2004: Created
 */

#include <windows.h>
#include "ddraw.h"
#include "rosddraw.h"

 
HRESULT WINAPI DirectDrawCreate(LPGUID lpGUID, LPVOID* lplpDD, LPUNKNOWN pUnkOuter) 
{    	
    if (pUnkOuter==NULL) return DDERR_INVALIDPARAMS;
	return DDRAW_Create(lpGUID, (LPVOID*) lplpDD, pUnkOuter, &IID_IDirectDraw, FALSE);
}

HRESULT WINAPI DirectDrawCreateEx(LPGUID lpGUID, LPVOID* lplpDD, REFIID iid, LPUNKNOWN pUnkOuter)
{
	if (pUnkOuter==NULL) return DDERR_INVALIDPARAMS;
	if (!IsEqualGUID(iid, &IID_IDirectDraw7)) return DDERR_INVALIDPARAMS;

    return DDRAW_Create(lpGUID, lplpDD, pUnkOuter, iid, TRUE);
}

HRESULT WINAPI DirectDrawEnumerateA(
  DWORD *lpCallback, 
  LPVOID lpContext
)
{
    return DD_OK;
}


HRESULT WINAPI DirectDrawEnumerateW(
  DWORD *lpCallback, 
  LPVOID lpContext
)
{
    return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateExA(
  DWORD lpCallback, 
  LPVOID lpContext, 
  DWORD dwFlags
)
{
    return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateExW(
  DWORD lpCallback, 
  LPVOID lpContext, 
  DWORD dwFlags
)
{
    return DD_OK;
}

HRESULT WINAPI DirectDrawCreateClipper(
  DWORD dwFlags, 
  DWORD FAR* lplpDDClipper, 
  IUnknown FAR* pUnkOuter
)
{
    return DD_OK;
}

HRESULT DDRAW_Create(
	LPGUID lpGUID, LPVOID *lplpDD, LPUNKNOWN pUnkOuter, REFIID iid, BOOL ex) 
{  		      
	HDC desktop;
	
    /* BOOL ex == TRUE it is DirectDrawCreateEx call here. */
	
	/* TODO: 
	   check the GUID are right 
	   add scanner that DirectDrawCreate / DirectDrawCreateEx select right driver.
	   now we will assume it is the current display driver 
	*/

	
	/*
	desktop = GetWindowDC(GetDesktopWindow());
	lplpDD = OsThunkDdCreateDirectDrawObject(desktop);   
	if (lplpDD == NULL) return DDERR_NODIRECTDRAWHW;
	*/
	 	
	return DDERR_NODIRECTDRAWHW;
}

BOOL WINAPI DllMain(HINSTANCE hInstance,DWORD fwdReason, LPVOID lpvReserved)
{
    switch(fwdReason)
    {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
        case DLL_THREAD_DETACH:
            break;
    }
    return(TRUE);
} 
