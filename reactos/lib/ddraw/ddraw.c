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
               
HRESULT WINAPI DirectDrawCreate(LPGUID lpGUID, LPDIRECTDRAW* lplpDD, LPUNKNOWN pUnkOuter) 
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
  LPDDENUMCALLBACKA lpCallback, 
  LPVOID lpContext
)
{
    return DD_OK;
}


HRESULT WINAPI DirectDrawEnumerateW(
  LPDDENUMCALLBACKW lpCallback, 
  LPVOID lpContext
)
{
    return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateExA(
  LPDDENUMCALLBACKEXA lpCallback, 
  LPVOID lpContext, 
  DWORD dwFlags
)
{
    return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateExW(
  LPDDENUMCALLBACKEXW lpCallback, 
  LPVOID lpContext, 
  DWORD dwFlags
)
{
    return DD_OK;
}
 
HRESULT WINAPI DirectDrawCreateClipper(
  DWORD dwFlags, 
  LPDIRECTDRAWCLIPPER* lplpDDClipper, 
  LPUNKNOWN pUnkOuter
)
{
    return DD_OK;
}

HRESULT DDRAW_Create(
	LPGUID lpGUID, LPVOID *lplpDD, LPUNKNOWN pUnkOuter, REFIID iid, BOOL ex) 
{  		      
	
	 HRESULT hr;

     
    /* TODO 1: 
	   check the GUID are right 
	   add scanner that DirectDrawCreate / DirectDrawCreateEx select right driver.
	   now we will assume it is the current display driver 
	*/

	 /* TODO 2: 
	   do not only use hardware mode.
	*/

	hr = HAL_DirectDraw_Create(lpGUID, lplpDD, pUnkOuter, iid,  ex);

	/* old code 
	 //HDC desktop;		
	
	desktop = GetWindowDC(GetDesktopWindow());
	lplpDD = OsThunkDdCreateDirectDrawObject(desktop);   
	if (lplpDD == NULL) return DDERR_NODIRECTDRAWHW;
	*/
	 	
	return hr;
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
