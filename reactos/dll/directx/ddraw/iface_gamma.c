/* $Id: gamma.c 24690 2006-11-05 21:19:53Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/gamma.c
 * PURPOSE:              IDirectDrawGamma Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"

ULONG WINAPI
DirectDrawGammaControl_AddRef( LPDIRECTDRAWGAMMACONTROL iface)
{         
   LPDDRAWI_DDGAMMACONTROL_INT This = (LPDDRAWI_DDGAMMACONTROL_INT)iface;
    
    DX_WINDBG_trace();

    if (iface!=NULL)
    {
        This->dwIntRefCnt++;
        //This->lpLcl->dwLocalRefCnt++;

        //if (This->lpLcl->lpGbl != NULL)
        //{
        //    This->lpLcl->lpGbl->dwRefCnt++;
        //}
    }
    return This->dwIntRefCnt;
}

ULONG WINAPI
DirectDrawGammaControl_Release( LPDIRECTDRAWGAMMACONTROL iface)
{    
    LPDDRAWI_DDGAMMACONTROL_INT This = (LPDDRAWI_DDGAMMACONTROL_INT)iface;

    DX_WINDBG_trace();
    /* FIXME 
       This is not right exiame how it should be done 
     */
    DX_STUB_str("FIXME This is not right exiame how it should be done\n");
    return This->dwIntRefCnt;
}

HRESULT WINAPI
DirectDrawGammaControl_QueryInterface( LPDIRECTDRAWGAMMACONTROL iface, 
                                       REFIID riid,
                                       LPVOID *ppObj)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DirectDrawGammaControl_GetGammaRamp( LPDIRECTDRAWGAMMACONTROL iface, 
                                     DWORD dwFlags, 
                                     LPDDGAMMARAMP lpGammaRamp)
{
   DX_WINDBG_trace();
   DX_STUB;  
}

HRESULT WINAPI
DirectDrawGammaControl_SetGammaRamp( LPDIRECTDRAWGAMMACONTROL iface, 
                                     DWORD dwFlags, 
                                     LPDDGAMMARAMP lpGammaRamp)
{
   DX_WINDBG_trace();
   DX_STUB;  
}

IDirectDrawGammaControlVtbl DirectDrawGammaControl_Vtable =
{
    DirectDrawGammaControl_QueryInterface,
    DirectDrawGammaControl_AddRef,
    DirectDrawGammaControl_Release,
    DirectDrawGammaControl_GetGammaRamp,
    DirectDrawGammaControl_SetGammaRamp
};
