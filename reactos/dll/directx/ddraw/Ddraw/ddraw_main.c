/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/ddraw/ddraw_main.c
 * PURPOSE:              IDirectDraw7 Implementation
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */


#include "rosdraw.h"

/* PSEH for SEH Support */
#include <pseh/pseh.h>

HRESULT WINAPI
Main_DirectDraw_QueryInterface (LPDIRECTDRAW7 iface,
                                REFIID id,
                                LPVOID *obj)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    /* FIXME
       the D3D object can be optained from here
       Direct3D7
    */
    if (IsEqualGUID(&IID_IDirectDraw7, id))
    {
        /* DirectDraw7 Vtable */
        This->lpVtbl = &DirectDraw7_Vtable;
        This->lpLcl->dwLocalFlags = This->lpLcl->dwLocalFlags + DDRAWILCL_DIRECTDRAW7;
        *obj = &This->lpVtbl;
    }
    else
    {
        *obj = NULL;
        DX_STUB_str("E_NOINTERFACE");
        return E_NOINTERFACE;
    }

    Main_DirectDraw_AddRef(iface);
    DX_STUB_str("DD_OK");
    return DD_OK;
}

ULONG WINAPI
Main_DirectDraw_AddRef (LPDIRECTDRAW7 iface)
{
    ULONG retValue = 0;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    _SEH_TRY
    {
        This->dwIntRefCnt++;
        This->lpLcl->dwLocalRefCnt++;

        if (This->lpLcl->lpGbl != NULL)
        {
            This->lpLcl->lpGbl->dwRefCnt++;
        }
    }
    _SEH_HANDLE
    {
    }
    _SEH_END;

    _SEH_TRY
    {
        retValue = This->dwIntRefCnt;
    }
    _SEH_HANDLE
    {
        retValue = 0;
    }
    _SEH_END;

    return retValue;
}




ULONG WINAPI
Main_DirectDraw_Release (LPDIRECTDRAW7 iface)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    if (iface!=NULL)
    {
        This->lpLcl->dwLocalRefCnt--;
        This->dwIntRefCnt--;

        if (This->lpLcl->lpGbl != NULL)
        {
            This->lpLcl->lpGbl->dwRefCnt--;
        }

        if ( This->lpLcl->lpGbl->dwRefCnt == 0)
        {
            // set resoltion back to the one in registry
            /*if(This->cooperative_level & DDSCL_EXCLUSIVE)
            {
                ChangeDisplaySettings(NULL, 0);
            }*/

            Cleanup(iface);
            return 0;
        }
    }
    return This->dwIntRefCnt;
}

HRESULT WINAPI
Main_DirectDraw_Initialize (LPDIRECTDRAW7 iface, LPGUID lpGUID)
{
	return DDERR_ALREADYINITIALIZED;
}

/* 
 * Main_DirectDraw_Compact
 * ms say this one is not implement but it return  DDERR_NOEXCLUSIVEMODE
 * when no exclusive owner are set in corpativelevel 
 */
HRESULT WINAPI
Main_DirectDraw_Compact(LPDIRECTDRAW7 iface)
{
    HRESULT retVal = DD_OK;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT) iface;

    DX_WINDBG_trace();
    // EnterCriticalSection(&ddcs);

    if (This->lpLcl->lpGbl->lpExclusiveOwner == This->lpLcl)
    {
        retVal = DDERR_NOEXCLUSIVEMODE;
    }
    // LeaveCriticalSection(&ddcs);
    return retVal;
}

HRESULT WINAPI 
Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
                   LPDWORD dwTotal, LPDWORD dwFree)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    DX_WINDBG_trace();

    // There is no HEL implentation of this api
    if (!(This->lpLcl->lpDDCB->cbDDMiscellaneousCallbacks.dwFlags & DDHAL_MISCCB32_GETAVAILDRIVERMEMORY))
    {
        return DDERR_NODIRECTDRAWHW;
    }

    if ((!dwTotal && !dwFree) || !ddscaps)
    {
        return DDERR_INVALIDPARAMS;
    }

    DDHAL_GETAVAILDRIVERMEMORYDATA  memdata;
    ZeroMemory(&memdata, sizeof(DDHAL_GETAVAILDRIVERMEMORYDATA));
    memdata.lpDD = This->lpLcl->lpGbl;
    memdata.ddRVal = DDERR_INVALIDPARAMS;
    memcpy(&memdata.DDSCaps, ddscaps, sizeof(DDSCAPS2));

    if (This->lpLcl->lpDDCB->HALDDMiscellaneous.GetAvailDriverMemory(&memdata) == DDHAL_DRIVER_NOTHANDLED)
        return DDERR_NODIRECTDRAWHW;

    if (dwTotal)
       *dwTotal = memdata.dwTotal;

    if (dwFree)
       *dwFree = memdata.dwFree;

    return memdata.ddRVal;
}

HRESULT WINAPI
Main_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD lpNumCodes, LPDWORD lpCodes)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    HRESULT retVal = DD_OK;

    DX_WINDBG_trace();

    
     // EnterCriticalSection(&ddcs);

    _SEH_TRY
    {
        if(IsBadWritePtr(lpNumCodes,sizeof(LPDWORD)))
        {
            retVal = DDERR_INVALIDPARAMS;
        }
        else
        {
            if(!IsBadWritePtr(lpCodes,sizeof(LPDWORD)))
            {
                memcpy(lpCodes, This->lpLcl->lpGbl->lpdwFourCC, sizeof(DWORD)* min(This->lpLcl->lpGbl->dwNumFourCC, *lpNumCodes));
            }
            else
            {
                *lpNumCodes = This->lpLcl->lpGbl->dwNumFourCC;
            }
        }
    }
    _SEH_HANDLE
    {
    }
    _SEH_END;

    //LeaveCriticalSection(&ddcs);
    return retVal;
}



/* 
 * We can optain the version of the directdraw object by compare the 
 * vtl table pointer from iface we do not need pass which version 
 * we whant to use
 *
 * Main_DirectDraw_CreateSurface is dead at moment we do only support
 * directdraw 7 at moment 
 */

/* For DirectDraw 1 - 3 */
HRESULT WINAPI 
Main_DirectDraw_CreateSurface (LPDIRECTDRAW iface, LPDDSURFACEDESC pDDSD,
                               LPDIRECTDRAWSURFACE *ppSurf, IUnknown *pUnkOuter)
{
   HRESULT ret = DDERR_GENERIC;
   DDSURFACEDESC2 dd_desc_v2;

   DX_WINDBG_trace();

    // EnterCriticalSection(&ddcs);
    _SEH_TRY
    {
        if (pDDSD->dwSize == sizeof(DDSURFACEDESC))
        {
            CopyDDSurfDescToDDSurfDesc2(&dd_desc_v2, (LPDDSURFACEDESC)pDDSD);
            ret = Internal_CreateSurface((LPDDRAWI_DIRECTDRAW_INT)iface,
                                         &dd_desc_v2,
                                         (LPDIRECTDRAWSURFACE7 *)ppSurf,
                                         pUnkOuter);
        }
        else
        {
            ret = DDERR_INVALIDPARAMS;
        }
    }
    _SEH_HANDLE
    {
        ret = DDERR_GENERIC;
    }
    _SEH_END;
  // LeaveCriticalSection(&ddcs);
  return ret;
}


/* For DirectDraw 4 - 7 */
HRESULT WINAPI 
Main_DirectDraw_CreateSurface4(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
                               LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter)
{
   HRESULT ret;
    DX_WINDBG_trace();
    // EnterCriticalSection(&ddcs);
    _SEH_TRY
    {
        ret = Internal_CreateSurface( (LPDDRAWI_DIRECTDRAW_INT)iface,pDDSD, ppSurf,pUnkOuter);
    }
        _SEH_HANDLE
    {
        ret = DDERR_GENERIC;
    }
    _SEH_END;

    // LeaveCriticalSection(&ddcs);
    return ret;
}
IDirectDraw7Vtbl DirectDraw7_Vtable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Main_DirectDraw_Compact,
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface4,
    Main_DirectDraw_DuplicateSurface,
    Main_DirectDraw_EnumDisplayModes,
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,
    Main_DirectDraw_GetFourCCCodes,
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,
    Main_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    Main_DirectDraw_SetDisplayMode,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    Main_DirectDraw_GetDeviceIdentifier,
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};
