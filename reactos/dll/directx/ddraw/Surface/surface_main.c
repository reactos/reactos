/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/surface/surface_main.c
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


HRESULT WINAPI Main_DDrawSurface_Initialize (LPDIRECTDRAWSURFACE7 iface, LPDIRECTDRAW pDD, LPDDSURFACEDESC2 pDDSD2)
{
    return DDERR_ALREADYINITIALIZED;
}

ULONG WINAPI Main_DDrawSurface_AddRef(LPDIRECTDRAWSURFACE7 iface)
{
    LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

    DX_WINDBG_trace();

    if (iface!=NULL)
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
Main_DDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE7 iface, REFIID riid,
				      LPVOID* ppObj)
{
    DX_WINDBG_trace();

	DX_STUB_str("Unimplement\n");

	return E_NOINTERFACE;
}



ULONG WINAPI Main_DDrawSurface_Release(LPDIRECTDRAWSURFACE7 iface)
{
    LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

    /* FIXME
       This is not right exiame how it should be done
     */
    DX_STUB_str("FIXME This is not right exiame how it should be done\n");
    return This->dwIntRefCnt;
}

HRESULT WINAPI Main_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rdst,
			  LPDIRECTDRAWSURFACE7 src, LPRECT rsrc, DWORD dwFlags, LPDDBLTFX lpbltfx)
{
	 LPDDRAWI_DDRAWSURFACE_INT ThisDest = (LPDDRAWI_DDRAWSURFACE_INT)iface;
	 LPDDRAWI_DDRAWSURFACE_INT ThisSrc = (LPDDRAWI_DDRAWSURFACE_INT)src;

	 DDHAL_BLTDATA mDdBlt;

	 DX_WINDBG_trace();

     if (( ThisDest->lpLcl->lpGbl->lpDD->lpDDCBtmp->HALDDSurface.dwFlags
		   & DDHAL_SURFCB32_BLT) != DDHAL_SURFCB32_BLT)
	 {
		 return DDERR_GENERIC;
	 }

	 ZeroMemory(&mDdBlt, sizeof(DDHAL_BLTDATA));
     ZeroMemory(&mDdBlt.bltFX, sizeof(DDBLTFX));

     if (!DdResetVisrgn( ThisDest->lpLcl->lpSurfMore->slist[0], NULL))
     {
		 DX_STUB_str("DdResetVisrgn failed");
     }

	 mDdBlt.lpDD = ThisDest->lpLcl->lpSurfMore->lpDD_lcl->lpGbl;
     mDdBlt.Blt = ThisDest->lpLcl->lpSurfMore->lpDD_lcl->lpGbl->lpDDCBtmp->HALDDSurface.Blt;
     mDdBlt.lpDDDestSurface = ThisDest->lpLcl->lpSurfMore->slist[0];

	 ThisDest->lpLcl->lpSurfMore->slist[0]->hDC =
		       ThisDest->lpLcl->lpSurfMore->lpDD_lcl->hDC;

     /* Setup Src */
	 if (ThisSrc != NULL)
	 {
		 mDdBlt.lpDDSrcSurface = ThisSrc->lpLcl->lpSurfMore->slist[0];

		 ThisSrc->lpLcl->lpSurfMore->slist[0]->hDC =
			      ThisSrc->lpLcl->lpSurfMore->lpDD_lcl->hDC;

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
	  *  compare so we do not write to far
	  *  ThisDest->lpLcl->lpGbl->wWidth; <- surface max width
	  *  ThisDest->lpLcl->lpGbl->wHeight <- surface max heght
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
		if(!GetWindowRect((HWND)ThisDest->lpLcl->lpSurfMore->lpDD_lcl->hWnd,
				(RECT *)&mDdBlt.rDest))
		{
            DX_STUB_str("GetWindowRect failed");
		}

	 }

	 /* FIXME
	  *  compare so we do not write to far
	  *  ThisDest->lpLcl->lpGbl->wWidth; <- surface max width
	  *  ThisDest->lpLcl->lpGbl->wHeight <- surface max heght
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
Main_DDrawSurface_Lock (LPDIRECTDRAWSURFACE7 iface, LPRECT prect,
                        LPDDSURFACEDESC2 pDDSD, DWORD flags, HANDLE events)
{
    LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;
    DDHAL_LOCKDATA mdLock;

    DX_WINDBG_trace();

    DX_WINDBG_trace_res( This->lpLcl->lpGbl->wWidth, This->lpLcl->lpGbl->wHeight, This->lpLcl->lpGbl->lPitch, 0);

    if (events != NULL)
    {
        return DDERR_INVALIDPARAMS; 
    }

    /* FIXME add a check see if lock suport or not */

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

    mdLock.ddRVal = DDERR_NOTPALETTIZED;
    mdLock.Lock = This->lpLcl->lpSurfMore->lpDD_lcl->lpDDCB->HALDDSurface.Lock;
    mdLock.dwFlags = flags;
    mdLock.lpDDSurface = This->lpLcl->lpSurfMore->slist[0];
    mdLock.lpDD = This->lpLcl->lpSurfMore->lpDD_lcl->lpGbl;
    mdLock.lpSurfData = NULL;


    if (!DdResetVisrgn(This->lpLcl->lpSurfMore->slist[0], NULL))
    {
      DX_STUB_str("Here DdResetVisrgn lock");
      return DDERR_UNSUPPORTED;
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
        //    // FIXME the interanl mddsdPrimary shall be DDSURFACEDESC2
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

        pDDSD->dwHeight =This->lpLcl->lpGbl->wHeight;
        pDDSD->dwWidth = This->lpLcl->lpGbl->wWidth;

        pDDSD->ddpfPixelFormat.dwRGBBitCount = This->lpLcl->lpGbl->lpDD->lpModeInfo->dwBPP;// .lpModeInfo->dwBPP; //This->lpLcl->lpGbl->lPitch/ 8;
        pDDSD->lPitch = This->lpLcl->lpGbl->lPitch;
        pDDSD->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    }

    return mdLock.ddRVal;
}


HRESULT WINAPI Main_DDrawSurface_Unlock (LPDIRECTDRAWSURFACE7 iface, LPRECT pRect)
{
	LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;
	DDHAL_UNLOCKDATA mdUnLock;

	DX_WINDBG_trace();

	if (!This->lpLcl->lpSurfMore->lpDD_lcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags & DDHAL_SURFCB32_UNLOCK)
	{
	   DX_STUB_str("DDERR_UNSUPPORTED");
	   return DDERR_UNSUPPORTED;
	}

    mdUnLock.ddRVal = DDERR_NOTPALETTIZED;
    mdUnLock.lpDD = This->lpLcl->lpSurfMore->lpDD_lcl->lpGbl;
	mdUnLock.lpDDSurface =  This->lpLcl->lpSurfMore->slist[0];
	mdUnLock.Unlock = This->lpLcl->lpSurfMore->lpDD_lcl->lpDDCB->HALDDSurface.Unlock;

    if (!DdResetVisrgn( mdUnLock.lpDDSurface, NULL))
    {
        DX_STUB_str("DDERR_UNSUPPORTED");
		return DDERR_UNSUPPORTED;
    }

    if (mdUnLock.Unlock(&mdUnLock)!= DDHAL_DRIVER_HANDLED)
    {
		DX_STUB_str("unLock fail");
        return DDERR_UNSUPPORTED;
    }

    return mdUnLock.ddRVal;
}

HRESULT WINAPI
Main_DDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDIRECTDRAWSURFACE7 pAttach)
{

   // LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;
   // LPDDRAWI_DDRAWSURFACE_INT That = (LPDDRAWI_DDRAWSURFACE_INT)pAttach;

   DX_WINDBG_trace();

   DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
                                     LPDDSCAPS2 pCaps,
                                     LPDIRECTDRAWSURFACE7* ppSurface)
{
    /* FIXME hacked */

    LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

    DX_WINDBG_trace();

    *ppSurface = (LPDIRECTDRAWSURFACE7) This->lpLcl->lpGbl->lpDD->dsList;


    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

	DX_WINDBG_trace();

	if (!This->lpLcl->lpGbl->lpDD->lpDDCBtmp->cbDDSurfaceCallbacks.dwFlags & DDHAL_SURFCB32_FLIP)
	{
		return DDERR_GENERIC;
	}

	DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetCaps(LPDIRECTDRAWSURFACE7 iface, LPDDSCAPS2 pCaps)
{
    LPDDRAWI_DDRAWSURFACE_INT This;

	DX_WINDBG_trace();

    if (iface == NULL)
    {
       return DDERR_INVALIDOBJECT;
    }

    if (pCaps == NULL)
    {
       return DDERR_INVALIDPARAMS;
    }

    This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

    RtlZeroMemory(pCaps,sizeof(DDSCAPS2));

	pCaps->dwCaps = This->lpLcl->ddsCaps.dwCaps;

    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_GetClipper(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER* ppClipper)
{
    LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

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
Main_DDrawSurface_SetClipper (LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER pDDClipper)
{
    LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

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
Main_DDrawSurface_GetDC(LPDIRECTDRAWSURFACE7 iface, HDC *phDC)
{
    LPDDRAWI_DDRAWSURFACE_INT This;

	DX_WINDBG_trace();

    if (iface == NULL)
    {
       return DDERR_INVALIDOBJECT;
    }

    if (phDC == NULL)
    {
       return DDERR_INVALIDPARAMS;
    }

    This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

    *phDC = (HDC)This->lpLcl->lpSurfMore->lpDD_lcl->hDC;

    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE7 iface,
                                 LPDDPIXELFORMAT pDDPixelFormat)
{
    LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;
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
Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
				      LPDDSURFACEDESC2 pDDSD)
{
    DWORD dwSize;


    LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;
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
Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7 iface, HDC hDC)
{
    LPDDRAWI_DDRAWSURFACE_INT This;
	DX_WINDBG_trace();

    if (iface == NULL)
    {
       return DDERR_INVALIDOBJECT;
    }

    if (hDC == NULL)
    {
       return DDERR_INVALIDPARAMS;
    }

    This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

    /* FIXME check if surface exits or not */


    if ((HDC)This->lpLcl->hDC == NULL)
    {
        return DDERR_GENERIC;
    }

    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwFlags, LPDDCOLORKEY pCKey)
{
    LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

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
Main_DDrawSurface_SetOverlayPosition (LPDIRECTDRAWSURFACE7 iface, LONG X, LONG Y)
{
    LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

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

IDirectDrawSurface7Vtbl DirectDrawSurface7_Vtable =
{
      /*** IUnknown ***/
    Main_DDrawSurface_QueryInterface,
    Main_DDrawSurface_AddRef,                        /* (Compact done) */
    Main_DDrawSurface_Release,
    /*** IDirectDrawSurface ***/
    Main_DDrawSurface_AddAttachedSurface,
    Main_DDrawSurface_AddOverlayDirtyRect,
    Main_DDrawSurface_Blt,
    Main_DDrawSurface_BltBatch,
    Main_DDrawSurface_BltFast,
    Main_DDrawSurface_DeleteAttachedSurface,
    Main_DDrawSurface_EnumAttachedSurfaces,
    Main_DDrawSurface_EnumOverlayZOrders,
    Main_DDrawSurface_Flip,
    Main_DDrawSurface_GetAttachedSurface,
    Main_DDrawSurface_GetBltStatus,
    Main_DDrawSurface_GetCaps,
    Main_DDrawSurface_GetClipper,
    Main_DDrawSurface_GetColorKey,
    Main_DDrawSurface_GetDC,
    Main_DDrawSurface_GetFlipStatus,
    Main_DDrawSurface_GetOverlayPosition,
    Main_DDrawSurface_GetPalette,
    Main_DDrawSurface_GetPixelFormat,
    Main_DDrawSurface_GetSurfaceDesc,
    Main_DDrawSurface_Initialize,
    Main_DDrawSurface_IsLost,
    Main_DDrawSurface_Lock,
    Main_DDrawSurface_ReleaseDC,
    Main_DDrawSurface_Restore,
    Main_DDrawSurface_SetClipper,
    Main_DDrawSurface_SetColorKey,
    Main_DDrawSurface_SetOverlayPosition,
    Main_DDrawSurface_SetPalette,
    Main_DDrawSurface_Unlock,
    Main_DDrawSurface_UpdateOverlay,
    Main_DDrawSurface_UpdateOverlayDisplay,
    Main_DDrawSurface_UpdateOverlayZOrder,
    /*** IDirectDrawSurface2 ***/
    Main_DDrawSurface_GetDDInterface,
    Main_DDrawSurface_PageLock,
    Main_DDrawSurface_PageUnlock,
    /*** IDirectDrawSurface3 ***/
    Main_DDrawSurface_SetSurfaceDesc,
    /*** IDirectDrawSurface4 ***/
    Main_DDrawSurface_SetPrivateData,
    Main_DDrawSurface_GetPrivateData,
    Main_DDrawSurface_FreePrivateData,
    Main_DDrawSurface_GetUniquenessValue,
    Main_DDrawSurface_ChangeUniquenessValue,
    /*** IDirectDrawSurface7 ***/
    Main_DDrawSurface_SetPriority,
    Main_DDrawSurface_GetPriority,
    Main_DDrawSurface_SetLOD,
    Main_DDrawSurface_GetLOD
};
