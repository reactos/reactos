/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 dll/directx/ddraw/Palette/createpalette.c
 * PURPOSE:              IDirectDrawPalette Creation
 * PROGRAMMER:           Jérôme Gardou
 *
 */
#include "rosdraw.h"

DWORD ConvertPCapsFlags(DWORD dwFlags)
{
    DWORD ret = 0;
    if(dwFlags & DDPCAPS_4BIT)
        ret|=DDRAWIPAL_16;
    if(dwFlags & DDPCAPS_8BIT)
        ret|=DDRAWIPAL_256;
    if(dwFlags & DDPCAPS_8BITENTRIES)
        ret|=DDRAWIPAL_STORED_8INDEX;
    if(dwFlags & DDPCAPS_ALLOW256)
        ret|=DDRAWIPAL_ALLOW256;
    if(dwFlags & DDPCAPS_ALPHA)
        ret|=DDRAWIPAL_ALPHA;
    if(dwFlags & DDPCAPS_1BIT)
        ret|=DDRAWIPAL_2;
    if(dwFlags & DDPCAPS_2BIT)
        ret|=DDRAWIPAL_4;

    return ret;
}

HRESULT
Internal_CreatePalette( LPDDRAWI_DIRECTDRAW_INT pDDraw, DWORD dwFlags,
                  LPPALETTEENTRY palent, LPDIRECTDRAWPALETTE* ppPalette, LPUNKNOWN pUnkOuter)
{
    DDHAL_CREATEPALETTEDATA mDdCreatePalette = { 0 };

    LPDDRAWI_DDRAWPALETTE_INT ThisPalInt = NULL;
    LPDDRAWI_DDRAWPALETTE_LCL ThisPalLcl = NULL;
    LPDDRAWI_DDRAWPALETTE_GBL ThisPalGbl = NULL;

    HRESULT ret;

    if(pUnkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }

    if(!(pDDraw->lpLcl->dwLocalFlags & DDRAWILCL_SETCOOPCALLED))
    {
        return DDERR_NOCOOPERATIVELEVELSET;
    }


    if (pDDraw->lpLcl->dwProcessId != GetCurrentProcessId() )
    {
        return DDERR_INVALIDOBJECT;
    }

    /* Allocate the palette interface and needed members */
    DxHeapMemAlloc(ThisPalInt,  sizeof( DDRAWI_DDRAWPALETTE_INT ) );
    if( ThisPalInt == NULL )
    {
        ret = DDERR_OUTOFMEMORY;
        goto cleanup;
    }

    DxHeapMemAlloc(ThisPalLcl,  sizeof( DDRAWI_DDRAWPALETTE_LCL ) );
    if( ThisPalLcl == NULL )
    {
        ret = DDERR_OUTOFMEMORY;
        goto cleanup;
    }

    DxHeapMemAlloc(ThisPalGbl,  sizeof( DDRAWI_DDRAWPALETTE_GBL ) );
    if( ThisPalGbl == NULL )
    {
        ret = DDERR_OUTOFMEMORY;
        goto cleanup;
    }

    /*Some initial setup*/

    ThisPalInt->lpLcl = ThisPalLcl;
    ThisPalLcl->lpGbl = ThisPalGbl;

    ThisPalLcl->lpDD_lcl = ThisPalGbl->lpDD_lcl = pDDraw->lpLcl;
    ThisPalGbl->dwFlags = ConvertPCapsFlags(dwFlags);

    ThisPalInt->lpVtbl = (PVOID)&DirectDrawPalette_Vtable;
    ThisPalGbl->dwProcessId = GetCurrentProcessId();

    mDdCreatePalette.lpDD = pDDraw->lpLcl->lpGbl;
    mDdCreatePalette.lpDDPalette = ThisPalGbl;
    if(pDDraw->lpLcl->lpGbl->lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_CREATEPALETTE) {
        mDdCreatePalette.CreatePalette = pDDraw->lpLcl->lpGbl->lpDDCBtmp->HALDD.CreatePalette;
        DX_STUB_str("Using HAL CreatePalette\n");
    }
    else {
        mDdCreatePalette.CreatePalette = pDDraw->lpLcl->lpGbl->lpDDCBtmp->HELDD.CreatePalette;
        DX_STUB_str("Using HEL CreatePalette\n");
    }
    mDdCreatePalette.ddRVal = DDERR_GENERIC;
    mDdCreatePalette.lpColorTable = palent;

    if (mDdCreatePalette.CreatePalette(&mDdCreatePalette) == DDHAL_DRIVER_NOTHANDLED)
    {
        DX_STUB_str("mDdCreateSurface failed with DDHAL_DRIVER_NOTHANDLED.");
        ret = DDERR_NOTINITIALIZED;
        goto cleanup;
    }

    if (mDdCreatePalette.ddRVal != DD_OK)
    {
        DX_STUB_str("mDdCreateSurface failed.");
        ret = mDdCreatePalette.ddRVal;
        goto cleanup;
    }

    *ppPalette = (LPDIRECTDRAWPALETTE)ThisPalInt;
    ThisPalInt->lpLink = pDDraw->lpLcl->lpGbl->palList;
    pDDraw->lpLcl->lpGbl->palList = ThisPalInt;
    ThisPalInt->lpLcl->dwReserved1 = (ULONG_PTR)pDDraw;
    IDirectDrawPalette_AddRef(*ppPalette);

    return DD_OK;

cleanup:
    if(ThisPalInt) DxHeapMemFree(ThisPalInt);
    if(ThisPalLcl) DxHeapMemFree(ThisPalLcl);
    if(ThisPalGbl) DxHeapMemFree(ThisPalGbl);

    return ret;
}
