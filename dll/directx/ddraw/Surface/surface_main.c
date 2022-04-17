/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 dll/directx/ddraw/Surface/surface_main.c
 * PURPOSE:              IDirectDrawSurface7 Implementation
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"


/* FIXME adding hal and hel stub
    DestroySurface;
    SetClipList;
    AddAttachedSurface;
    GetFlipStatus;
    SetOverlayPosition;
    SetPalette;
*/

LPDDRAWI_DDRAWSURFACE_INT
internal_directdrawsurface_int_alloc(LPDDRAWI_DDRAWSURFACE_INT This)
{
    LPDDRAWI_DDRAWSURFACE_INT  newThis;
    DxHeapMemAlloc(newThis, sizeof(DDRAWI_DDRAWSURFACE_INT));
    if (newThis)
    {
        newThis->lpLcl = This->lpLcl;
        newThis->lpLink = This;
    }
    return  newThis;
}

HRESULT WINAPI Main_DDrawSurface_Initialize (LPDDRAWI_DDRAWSURFACE_INT iface, LPDIRECTDRAW pDD, LPDDSURFACEDESC2 pDDSD2)
{
    return DDERR_ALREADYINITIALIZED;
}

ULONG WINAPI Main_DDrawSurface_AddRef(LPDDRAWI_DDRAWSURFACE_INT This)
{

    DX_WINDBG_trace();

    if (This!=NULL)
    {
        This->dwIntRefCnt++;
        This->lpLcl->dwLocalRefCnt++;

        if (This->lpLcl->lpGbl != NULL)
        {
            This->lpLcl->lpGbl->dwRefCnt++;
        }
    }
    return This->dwIntRefCnt;

}

HRESULT WINAPI
Main_DDrawSurface_QueryInterface(LPDDRAWI_DDRAWSURFACE_INT This, REFIID riid, LPVOID* ppObj)
{
    HRESULT retVal = DD_OK;
    *ppObj = NULL;

    DX_WINDBG_trace();

    _SEH2_TRY
    {
        if (IsEqualGUID(&IID_IDirectDrawSurface7, riid))
        {
            if (This->lpVtbl != &DirectDrawSurface7_Vtable)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &DirectDrawSurface7_Vtable;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirectDrawSurface4, riid))
        {
            if (This->lpVtbl != &DirectDrawSurface4_Vtable)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &DirectDrawSurface4_Vtable;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirectDrawSurface3, riid))
        {
            if (This->lpVtbl != &DirectDrawSurface3_Vtable)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &DirectDrawSurface3_Vtable;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirectDrawSurface2, riid))
        {
            if (This->lpVtbl != &DirectDrawSurface2_Vtable)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &DirectDrawSurface2_Vtable;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirectDrawSurface, riid))
        {
            if (This->lpVtbl != &DirectDrawSurface_Vtable)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &DirectDrawSurface_Vtable;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirectDrawColorControl, riid))
        {
            if (This->lpVtbl != &DirectDrawColorControl_Vtable)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &DirectDrawColorControl_Vtable;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirectDrawGammaControl, riid))
        {
            if (This->lpVtbl != &DirectDrawGammaControl_Vtable)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &DirectDrawGammaControl_Vtable;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirectDrawSurfaceKernel, riid))
        {
            if (This->lpVtbl != &DirectDrawSurfaceKernel_Vtable)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &DirectDrawSurfaceKernel_Vtable;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirect3D, riid))
        {
            if (This->lpVtbl != &IDirect3D_Vtbl)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &IDirect3D_Vtbl;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirect3D2, riid))
        {
            if (This->lpVtbl != &IDirect3D2_Vtbl)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &IDirect3D2_Vtbl;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirect3D3, riid))
        {
            if (This->lpVtbl != &IDirect3D3_Vtbl)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &IDirect3D3_Vtbl;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirect3D7, riid))
        {
            if (This->lpVtbl != &IDirect3D7_Vtbl)
            {
                This = internal_directdrawsurface_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &IDirect3D7_Vtbl;
            *ppObj = This;
            Main_DDrawSurface_AddRef(This);
        }
        else
        {
            DX_STUB_str("E_NOINTERFACE");
            retVal = E_NOINTERFACE;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;
    return retVal;
}



ULONG WINAPI Main_DDrawSurface_Release(LPDDRAWI_DDRAWSURFACE_INT This)
{
    /* FIXME
       This is not right exiame how it should be done
     */
    ULONG ret = --This->dwIntRefCnt;
    if(!ret)
    {
        DX_STUB_str("Release is a bit simplistic right now\n");
        AcquireDDThreadLock();
        DxHeapMemFree(This);
        ReleaseDDThreadLock();
    }
    return ret;
}

ULONG WINAPI Main_DDrawSurface_Release4(LPDDRAWI_DDRAWSURFACE_INT This)
{
    ULONG ref = Main_DDrawSurface_Release(This) ;

    if(ref == 0) Main_DirectDraw_Release(This->lpLcl->lpSurfMore->lpDD_int);

    return ref;
}

HRESULT WINAPI Main_DDrawSurface_Blt(LPDDRAWI_DDRAWSURFACE_INT ThisDest, LPRECT rdst,
                                     LPDDRAWI_DDRAWSURFACE_INT ThisSrc, LPRECT rsrc, DWORD dwFlags, LPDDBLTFX lpbltfx)
{
    DDHAL_BLTDATA mDdBlt;

    DX_WINDBG_trace();

    if (ThisDest == NULL)
    {
        return DDERR_INVALIDPARAMS;
    }

    /* Zero out members in DDHAL_BLTDATA */
    ZeroMemory(&mDdBlt, sizeof(DDHAL_BLTDATA));
    ZeroMemory(&mDdBlt.bltFX, sizeof(DDBLTFX));

    /* Check if we got HAL support for this api */
    if (( ThisDest->lpLcl->lpGbl->lpDD->lpDDCBtmp->HALDDSurface.dwFlags &
          DDHAL_SURFCB32_BLT) == DDHAL_SURFCB32_BLT)
    {
        mDdBlt.Blt = ThisDest->lpLcl->lpSurfMore->lpDD_lcl->lpGbl->lpDDCBtmp->HALDDSurface.Blt;
    }
    /* Check if we got HEL support for this api */
    else if (( ThisDest->lpLcl->lpGbl->lpDD->lpDDCBtmp->HELDDSurface.dwFlags &
          DDHAL_SURFCB32_BLT) == DDHAL_SURFCB32_BLT)
    {
        mDdBlt.Blt = ThisDest->lpLcl->lpSurfMore->lpDD_lcl->lpGbl->lpDDCBtmp->HELDDSurface.Blt;
    }

    if (mDdBlt.Blt == NULL)
    {
        /* This API is unsupported */
        return DDERR_UNSUPPORTED;
    }

    /* Prepare for draw, if we do not reset the DdResetVisrgn some graphics card will not draw on the screen */
    if (!DdResetVisrgn( ThisDest->lpLcl->lpSurfMore->slist[0], NULL))
    {
        DX_STUB_str("DdResetVisrgn failed");
    }

    mDdBlt.lpDD = ThisDest->lpLcl->lpSurfMore->lpDD_lcl->lpGbl;
    mDdBlt.lpDDDestSurface = ThisDest->lpLcl->lpSurfMore->slist[0];
    ThisDest->lpLcl->lpSurfMore->slist[0]->hDC = ThisDest->lpLcl->lpSurfMore->lpDD_lcl->hDC;

    /* Setup Src */
    if (( ThisSrc != NULL ) )
    {

        mDdBlt.lpDDSrcSurface = ThisSrc->lpLcl->lpSurfMore->slist[0];
        ThisSrc->lpLcl->lpSurfMore->slist[0]->hDC = ThisSrc->lpLcl->lpSurfMore->lpDD_lcl->hDC;

        if (rsrc != NULL)
        {
            memmove(&mDdBlt.rSrc, rsrc, sizeof (RECTL));
        }
        else
        {
            if(!GetWindowRect((HWND)ThisSrc->lpLcl->lpSurfMore->lpDD_lcl->hWnd,
                (RECT *)&mDdBlt.rSrc))
            {
                DX_STUB_str("GetWindowRect failed");
            }
        }

    /* FIXME
    *  compare so we do not write too far
    *  ThisDest->lpLcl->lpGbl->wWidth; <- surface max width
    *  ThisDest->lpLcl->lpGbl->wHeight <- surface max height
    *  ThisDest->lpLcl->lpGbl->lPitch  <- surface bpp
    */

    }

    /* Setup dest */
    if (rdst != NULL)
    {
        memmove(&mDdBlt.rDest, rdst, sizeof (RECTL));
    }
    else
    {
        if (!GetWindowRect((HWND)ThisDest->lpLcl->lpSurfMore->lpDD_lcl->hWnd,
            (RECT *)&mDdBlt.rDest))
        {
            DX_STUB_str("GetWindowRect failed");
        }
    }

    /* FIXME
    *  compare so we do not write too far
    *  ThisDest->lpLcl->lpGbl->wWidth; <- surface max width
    *  ThisDest->lpLcl->lpGbl->wHeight <- surface max height
    *  ThisDest->lpLcl->lpGbl->lPitch  <- surface bpp
    */


    /* setup bltFX */
    if (lpbltfx != NULL)
    {
        memmove(&mDdBlt.bltFX, lpbltfx, sizeof (DDBLTFX));
    }

    /* setup value that are not config yet */
    mDdBlt.dwFlags = dwFlags;
    mDdBlt.IsClipped = FALSE;
    mDdBlt.bltFX.dwSize = sizeof(DDBLTFX);


     /* FIXME
        BltData.dwRectCnt
        BltData.dwROPFlags
        BltData.IsClipped
        BltData.prDestRects
        BltData.rOrigDest
        BltData.rOrigSrc
        BltData.ddRVal
    */

    if (mDdBlt.Blt(&mDdBlt) != DDHAL_DRIVER_HANDLED)
    {
        DX_STUB_str("mDdBlt DDHAL_DRIVER_HANDLED");
        return DDERR_NOBLTHW;
     }

    return mDdBlt.ddRVal;
}


HRESULT WINAPI
Main_DDrawSurface_Lock (LPDDRAWI_DDRAWSURFACE_INT ThisDest, LPRECT prect,
                        LPDDSURFACEDESC2 pDDSD, DWORD flags, HANDLE events)
{
    DDHAL_LOCKDATA mdLock;

    DX_WINDBG_trace();

    DX_WINDBG_trace_res( (DWORD)ThisDest->lpLcl->lpGbl->wWidth, (DWORD)ThisDest->lpLcl->lpGbl->wHeight, (DWORD)ThisDest->lpLcl->lpGbl->lPitch, (DWORD) 0);

     /* Zero out members in DDHAL_LOCKDATA */
    ZeroMemory(&mdLock, sizeof(DDHAL_LOCKDATA));

     /* Check if we got HAL support for this api */
    if (( ThisDest->lpLcl->lpGbl->lpDD->lpDDCBtmp->HALDDSurface.dwFlags &
        DDHAL_SURFCB32_LOCK) == DDHAL_SURFCB32_LOCK)
    {
        mdLock.Lock = ThisDest->lpLcl->lpSurfMore->lpDD_lcl->lpGbl->lpDDCBtmp->HALDDSurface.Lock;
    }
    /* Check if we got HEL support for this api */
    else if (( ThisDest->lpLcl->lpGbl->lpDD->lpDDCBtmp->HELDDSurface.dwFlags &
          DDHAL_SURFCB32_LOCK) == DDHAL_SURFCB32_LOCK)
    {
        mdLock.Lock = ThisDest->lpLcl->lpSurfMore->lpDD_lcl->lpGbl->lpDDCBtmp->HELDDSurface.Lock;
    }

    if (mdLock.Lock == NULL)
    {
        /* This api are unsupported */
        return DDERR_UNSUPPORTED;
    }

    if (events != NULL)
    {
        return DDERR_INVALIDPARAMS;
    }

    /* FIXME add a check see if lock support or not */

    if (prect!=NULL)
    {
        mdLock.bHasRect = TRUE;
        memcpy(&mdLock.rArea,prect,sizeof(RECTL));
    }
    else
    {
        mdLock.bHasRect = FALSE;
    }

    //FIXME check if it primary or not and use primary or pixelformat data, at moment it is hardcode to primary

    mdLock.ddRVal = DDERR_CANTLOCKSURFACE;
    mdLock.dwFlags = flags;
    mdLock.lpDDSurface = ThisDest->lpLcl->lpSurfMore->slist[0];
    mdLock.lpDD = ThisDest->lpLcl->lpSurfMore->lpDD_lcl->lpGbl;
    mdLock.lpSurfData = NULL;


    if (!DdResetVisrgn(ThisDest->lpLcl->lpSurfMore->slist[0], NULL))
    {
      DX_STUB_str("Here DdResetVisrgn lock");
      // return DDERR_UNSUPPORTED;
    }

    if (mdLock.Lock(&mdLock)!= DDHAL_DRIVER_HANDLED)
    {
      DX_STUB_str("Here DDHAL_DRIVER_HANDLED lock");
      return DDERR_UNSUPPORTED;
    }

    // FIXME ??? is this right ??

    if (pDDSD != NULL)
    {
        ZeroMemory(pDDSD,sizeof(DDSURFACEDESC2));
        pDDSD->dwSize = sizeof(DDSURFACEDESC2);

        //if (pDDSD->dwSize == sizeof(DDSURFACEDESC2))
        //{
        //    ZeroMemory(pDDSD,sizeof(DDSURFACEDESC2));
        //    // FIXME the internal mddsdPrimary shall be DDSURFACEDESC2
        //    memcpy(pDDSD,&This->Surf->mddsdPrimary,sizeof(DDSURFACEDESC));
        //    pDDSD->dwSize = sizeof(DDSURFACEDESC2);
        //}
        //if (pDDSD->dwSize == sizeof(DDSURFACEDESC))
        //{
        //    RtlZeroMemory(pDDSD,sizeof(DDSURFACEDESC));
        //    memcpy(pDDSD,&This->Surf->mddsdPrimary,sizeof(DDSURFACEDESC));
        //    pDDSD->dwSize = sizeof(DDSURFACEDESC);
        //}


        pDDSD->lpSurface = (LPVOID)  mdLock.lpSurfData;

        pDDSD->dwHeight = ThisDest->lpLcl->lpGbl->wHeight;
        pDDSD->dwWidth = ThisDest->lpLcl->lpGbl->wWidth;

        pDDSD->ddpfPixelFormat.dwRGBBitCount = ThisDest->lpLcl->lpGbl->lpDD->lpModeInfo->dwBPP;// .lpModeInfo->dwBPP; //This->lpLcl->lpGbl->lPitch/ 8;
        pDDSD->lPitch = ThisDest->lpLcl->lpGbl->lPitch;
        pDDSD->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    }

    return mdLock.ddRVal;
}


HRESULT WINAPI Main_DDrawSurface_Unlock (LPDDRAWI_DDRAWSURFACE_INT This, LPRECT pRect)
{
    DDHAL_UNLOCKDATA mdUnLock;

    DX_WINDBG_trace();

    /* Zero out members in DDHAL_UNLOCKDATA */
    ZeroMemory(&mdUnLock, sizeof(DDHAL_UNLOCKDATA));

     /* Check if we got HAL support for this api */
    if (( This->lpLcl->lpGbl->lpDD->lpDDCBtmp->HALDDSurface.dwFlags &
        DDHAL_SURFCB32_UNLOCK) == DDHAL_SURFCB32_UNLOCK)
    {
        mdUnLock.Unlock = This->lpLcl->lpSurfMore->lpDD_lcl->lpGbl->lpDDCBtmp->HALDDSurface.Unlock;
    }
    /* Check if we got HEL support for this api */
    else if (( This->lpLcl->lpGbl->lpDD->lpDDCBtmp->HELDDSurface.dwFlags &
          DDHAL_SURFCB32_UNLOCK) == DDHAL_SURFCB32_UNLOCK)
    {
        mdUnLock.Unlock = This->lpLcl->lpSurfMore->lpDD_lcl->lpGbl->lpDDCBtmp->HELDDSurface.Unlock;
    }

    if (mdUnLock.Unlock == NULL)
    {
        /* This api are unsupported */
        return DDERR_UNSUPPORTED;
    }

    mdUnLock.ddRVal = DDERR_NOTPALETTIZED;
    mdUnLock.lpDD = This->lpLcl->lpSurfMore->lpDD_lcl->lpGbl;
    mdUnLock.lpDDSurface = This->lpLcl->lpSurfMore->slist[0];

    if (!DdResetVisrgn( mdUnLock.lpDDSurface, NULL))
    {
        DX_STUB_str("DdResetVisrgn fail");
        //return DDERR_UNSUPPORTED; /* this can fail */
    }

    if (mdUnLock.Unlock(&mdUnLock)!= DDHAL_DRIVER_HANDLED)
    {
        DX_STUB_str("unLock fail");
        return DDERR_UNSUPPORTED;
    }

    return mdUnLock.ddRVal;
}

HRESULT WINAPI
Main_DDrawSurface_AddAttachedSurface(LPDDRAWI_DDRAWSURFACE_INT iface,
					  LPDDRAWI_DDRAWSURFACE_INT pAttach)
{

   // LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;
   // LPDDRAWI_DDRAWSURFACE_INT That = (LPDDRAWI_DDRAWSURFACE_INT)pAttach;

   DX_WINDBG_trace();

   DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetAttachedSurface(LPDDRAWI_DDRAWSURFACE_INT This,
                                     LPDDSCAPS2 pCaps,
                                     LPDDRAWI_DDRAWSURFACE_INT* ppSurface)
{
    /* FIXME hacked */


    DX_WINDBG_trace();

    *ppSurface = This->lpLcl->lpGbl->lpDD->dsList;


    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_GetBltStatus(LPDDRAWI_DDRAWSURFACE_INT This, DWORD dwFlags)
{

	DX_WINDBG_trace();

	if (!(This->lpLcl->lpGbl->lpDD->lpDDCBtmp->cbDDSurfaceCallbacks.dwFlags & DDHAL_SURFCB32_FLIP))
	{
		return DDERR_GENERIC;
	}

	DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetCaps(LPDDRAWI_DDRAWSURFACE_INT This, LPDDSCAPS2 pCaps)
{

	DX_WINDBG_trace();

    if (This == NULL)
    {
       return DDERR_INVALIDOBJECT;
    }

    if (pCaps == NULL)
    {
       return DDERR_INVALIDPARAMS;
    }

    RtlZeroMemory(pCaps,sizeof(DDSCAPS2));

	pCaps->dwCaps = This->lpLcl->ddsCaps.dwCaps;

    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_GetClipper(LPDDRAWI_DDRAWSURFACE_INT This,
				  LPDIRECTDRAWCLIPPER* ppClipper)
{

	DX_WINDBG_trace();

    if (This == NULL)
    {
       return DDERR_INVALIDOBJECT;
    }

    if (ppClipper == NULL)
    {
       return DDERR_INVALIDPARAMS;
    }

    if (This->lpLcl->lp16DDClipper == NULL)
    {
        return DDERR_NOCLIPPERATTACHED;
    }

    *ppClipper = (LPDIRECTDRAWCLIPPER)This->lpLcl->lp16DDClipper;

    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_SetClipper (LPDDRAWI_DDRAWSURFACE_INT This,
				  LPDIRECTDRAWCLIPPER pDDClipper)
{

	DX_WINDBG_trace();

    if (This == NULL)
    {
       return DDERR_INVALIDOBJECT;
    }

    if(pDDClipper == NULL)
    {
        if(!This->lpLcl->lp16DDClipper)
            return DDERR_NOCLIPPERATTACHED;

        DirectDrawClipper_Release((LPDIRECTDRAWCLIPPER)This->lpLcl->lp16DDClipper);
        This->lpLcl->lp16DDClipper = NULL;
        return DD_OK;
    }

    // FIXME: Check Surface type and return DDERR_INVALIDSURFACETYPE

    DirectDrawClipper_AddRef((LPDIRECTDRAWCLIPPER)pDDClipper);
    This->lpLcl->lp16DDClipper = (LPDDRAWI_DDRAWCLIPPER_INT)pDDClipper;

    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_GetDC(LPDDRAWI_DDRAWSURFACE_INT This, HDC *phDC)
{

	DX_WINDBG_trace();

    if (This == NULL)
    {
       return DDERR_INVALIDOBJECT;
    }

    if (phDC == NULL)
    {
       return DDERR_INVALIDPARAMS;
    }


    *phDC = (HDC)This->lpLcl->lpSurfMore->lpDD_lcl->hDC;

    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_GetPixelFormat(LPDDRAWI_DDRAWSURFACE_INT This,
                                 LPDDPIXELFORMAT pDDPixelFormat)
{
    HRESULT retVale = DDERR_INVALIDPARAMS;

    DX_WINDBG_trace();

    if (pDDPixelFormat != NULL)
    {
        if (This->lpLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT)
        {
            memcpy(pDDPixelFormat,&This->lpLcl->lpGbl->ddpfSurface,sizeof(DDPIXELFORMAT));
        }
        else
        {
            memcpy(pDDPixelFormat,&This->lpLcl->lpSurfMore->
                              lpDD_lcl->lpGbl->vmiData.ddpfDisplay,sizeof(DDPIXELFORMAT));
        }
        retVale = DD_OK;
    }

  return retVale;
}

HRESULT WINAPI
Main_DDrawSurface_GetSurfaceDesc(LPDDRAWI_DDRAWSURFACE_INT This,
				      LPDDSURFACEDESC2 pDDSD)
{
    DWORD dwSize;

	DX_WINDBG_trace();

    dwSize =  pDDSD->dwSize;

    if ((dwSize != sizeof(DDSURFACEDESC)) &&
    	(dwSize != sizeof(DDSURFACEDESC2)))
    {
	   return DDERR_GENERIC;
    }

	ZeroMemory(pDDSD,dwSize);

	if (dwSize == sizeof(DDSURFACEDESC))
	{
		LPDDSURFACEDESC lpDS = (LPDDSURFACEDESC) pDDSD;
		memcpy(&lpDS->ddckCKDestBlt, &This->lpLcl->ddckCKDestBlt, sizeof(DDCOLORKEY));
		memcpy(&lpDS->ddckCKDestOverlay, &This->lpLcl->ddckCKDestOverlay, sizeof(DDCOLORKEY));
		memcpy(&lpDS->ddckCKSrcBlt, &This->lpLcl->ddckCKSrcBlt, sizeof(DDCOLORKEY));
		memcpy(&lpDS->ddckCKSrcOverlay, &This->lpLcl->ddckCKSrcOverlay, sizeof(DDCOLORKEY));
		memcpy(&lpDS->ddpfPixelFormat, &This->lpLcl->lpGbl->ddpfSurface, sizeof(DDPIXELFORMAT));
		memcpy(&lpDS->ddsCaps, &This->lpLcl->ddsCaps, sizeof(DDSCAPS));

		lpDS->dwAlphaBitDepth = This->lpLcl->dwAlpha;
		lpDS->dwBackBufferCount = This->lpLcl->dwBackBufferCount;

		/* FIXME setting the flags right */
		// lpDS->dwFlags = This->lpLcl->dwFlags;

		lpDS->dwHeight = This->lpLcl->lpGbl->wHeight;
		lpDS->dwWidth =  This->lpLcl->lpGbl->wWidth;

		/* This two are a union in lpDS  and in This->lpLcl->lpGbl
		  so I comment out lPitch
		  lpDS->lPitch = This->lpLcl->lpGbl->lPitch;
		*/
		lpDS->dwLinearSize = This->lpLcl->lpGbl->dwLinearSize;


		/* This tree are a union */
		//lpDS->dwMipMapCount
		//lpDS->dwRefreshRate
		//lpDS->dwZBufferBitDepth

		/* Unknown */
		// lpDS->dwReserved
		// lpDS->lpSurface
	}
	else
	{
		memcpy(&pDDSD->ddckCKDestBlt, &This->lpLcl->ddckCKDestBlt, sizeof(DDCOLORKEY));

		/*
		   pDDSD->dwEmptyFaceColor is a union to ddckCKDestOverlay
        */
		memcpy(&pDDSD->ddckCKDestOverlay, &This->lpLcl->ddckCKDestOverlay, sizeof(DDCOLORKEY));
		memcpy(&pDDSD->ddckCKSrcBlt, &This->lpLcl->ddckCKSrcBlt, sizeof(DDCOLORKEY));
		memcpy(&pDDSD->ddckCKSrcOverlay, &This->lpLcl->ddckCKSrcOverlay, sizeof(DDCOLORKEY));

		/*
		   pDDSD->dwFVF is a union to ddpfPixelFormat
		*/
		memcpy(&pDDSD->ddpfPixelFormat, &This->lpLcl->lpGbl->ddpfSurface, sizeof(DDPIXELFORMAT));
		memcpy(&pDDSD->ddsCaps, &This->lpLcl->ddsCaps, sizeof(DDSCAPS));


		pDDSD->dwAlphaBitDepth = This->lpLcl->dwAlpha;
		pDDSD->dwBackBufferCount = This->lpLcl->dwBackBufferCount;

		/* FIXME setting the flags right */
		// lpDS->dwFlags = This->lpLcl->dwFlags;

		pDDSD->dwHeight = This->lpLcl->lpGbl->wHeight;
		pDDSD->dwWidth =  This->lpLcl->lpGbl->wWidth;

		/* This two are a union in lpDS  and in This->lpLcl->lpGbl
		  so I comment out lPitch
		  lpDS->lPitch = This->lpLcl->lpGbl->lPitch;
		*/
		pDDSD->dwLinearSize = This->lpLcl->lpGbl->dwLinearSize;

		/* This tree are a union */
		// pDDSD->dwMipMapCount
		// pDDSD->dwRefreshRate
		// pDDSD->dwSrcVBHandle

		/* Unknown */
		// lpDS->dwReserved
		// lpDS->lpSurface
		// pDDSD->dwTextureStage
	}

    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_ReleaseDC(LPDDRAWI_DDRAWSURFACE_INT This, HDC hDC)
{
	DX_WINDBG_trace();

    if (This == NULL)
    {
       return DDERR_INVALIDOBJECT;
    }

    if (hDC == NULL)
    {
       return DDERR_INVALIDPARAMS;
    }

    /* FIXME check if surface exits or not */


    if ((HDC)This->lpLcl->hDC == NULL)
    {
        return DDERR_GENERIC;
    }

    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_SetColorKey (LPDDRAWI_DDRAWSURFACE_INT This,
				   DWORD dwFlags, LPDDCOLORKEY pCKey)
{

	DDHAL_SETCOLORKEYDATA ColorKeyData;

	DX_WINDBG_trace();

    ColorKeyData.ddRVal = DDERR_COLORKEYNOTSET;

	if (This->lpLcl->lpGbl->lpDD->lpDDCBtmp->cbDDSurfaceCallbacks.dwFlags & DDHAL_SURFCB32_SETCOLORKEY)
	{

		ColorKeyData.lpDD = This->lpLcl->lpGbl->lpDD;
		ColorKeyData.SetColorKey = 	This->lpLcl->lpGbl->lpDD->lpDDCBtmp->cbDDSurfaceCallbacks.SetColorKey;

		//ColorKeyData.lpDDSurface = &This->lpLcl->hDDSurface;
		ColorKeyData.dwFlags = dwFlags;
		/* FIXME
		   ColorKeyData.ckNew = ?
		   add / move dwFlags to This->lpLcl->dwFlags ??
	     */

		if (ColorKeyData.SetColorKey(&ColorKeyData) == DDHAL_DRIVER_HANDLED )
		{
		    return  ColorKeyData.ddRVal;
		}
	}
	return DDERR_COLORKEYNOTSET;
}



HRESULT WINAPI
Main_DDrawSurface_SetOverlayPosition (LPDDRAWI_DDRAWSURFACE_INT This, LONG X, LONG Y)
{

	DDHAL_SETOVERLAYPOSITIONDATA OverLayPositionData;

	DX_WINDBG_trace();

    OverLayPositionData.ddRVal = DDERR_COLORKEYNOTSET;

	if (This->lpLcl->lpGbl->lpDD->lpDDCBtmp->cbDDSurfaceCallbacks.dwFlags & DDHAL_SURFCB32_SETOVERLAYPOSITION)
	{

		OverLayPositionData.lpDD = This->lpLcl->lpGbl->lpDD;
		OverLayPositionData.SetOverlayPosition = This->lpLcl->lpGbl->lpDD->lpDDCBtmp->cbDDSurfaceCallbacks.SetOverlayPosition;

		//OverLayPositionData.lpDDSrcSurface = This->lpLcl->lpSurfaceOverlaying->lpLcl->hDDSurface;
		//OverLayPositionData.lpDDDestSurface = This->lpLcl->hDDSurface;

		OverLayPositionData.lXPos = X;
		OverLayPositionData.lYPos = Y;


		/* FIXME
		   Should X and Y be save ??
	     */

		if (OverLayPositionData.SetOverlayPosition(&OverLayPositionData) == DDHAL_DRIVER_HANDLED )
		{
		    return  OverLayPositionData.ddRVal;
		}
	}

	return DDERR_GENERIC;
}
