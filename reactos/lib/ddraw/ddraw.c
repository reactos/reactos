/* $Id: ddraw.c,v 1.1 2004/02/03 23:09:48 rcampbell Exp $
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
//#include <ddraw.h>

 #define DD_OK 0
 
HRESULT STDCALL DirectDrawCreate(
  GUID FAR* lpGUID, 
  DWORD FAR* lplpDD, 
  IUnknown FAR* pUnkOuter
)
{
    return DD_OK;
}

HRESULT STDCALL DirectDrawCreateEx(
  GUID FAR* lpGUID, 
  DWORD FAR* lplpDD, 
  DWORD Unknown3,
  IUnknown FAR* pUnkOuter
)
{
    return DD_OK;
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
