/* $Id: dsound.c,v 1.1 2004/02/03 23:09:48 rcampbell Exp $
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
 
HRESULT WINAPI DirectSoundCreate(
  LPGUID lpGuid, 
  DWORD* ppDS, 
  LPUNKNOWN  pUnkOuter 
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
