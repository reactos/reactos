/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS GDI32
 * PURPOSE:          GDI DirectX inteface
 * FILE:             lib/gdi32/misc/gdientry.c
 * PROGRAMERS:       Alex Ionescu (alex@relsoft.net)
 *                   Magnus Olsen (magnus@greatlord.com)
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"
#include <ddraw.h>
#include <ddrawi.h>
#include <ddrawint.h>
#include <ddrawgdi.h>
#include <ntgdi.h>

/* DATA **********************************************************************/

HANDLE ghDirectDraw;
ULONG gcDirectDraw;

#define GetDdHandle(Handle) ((HANDLE)Handle ? (HANDLE)Handle : ghDirectDraw)


/* CALLBACKS *****************************************************************/

/*
 * Dd Surface Callbacks
 */
DWORD
WINAPI
DdAddAttachedSurface(LPDDHAL_ADDATTACHEDSURFACEDATA Attach)
{
    /* Call win32k */
    return NtGdiDdAddAttachedSurface((HANDLE)Attach->lpDDSurface->hDDSurface,
                                      (HANDLE)Attach->lpSurfAttached->hDDSurface,
                                      (PDD_ADDATTACHEDSURFACEDATA)Attach);
}

DWORD
WINAPI
DdBlt(LPDDHAL_BLTDATA Blt)
{
    HANDLE Surface = 0;

    /* Use the right surface */
    if (Blt->lpDDSrcSurface) Surface = (HANDLE)Blt->lpDDSrcSurface->hDDSurface;

    /* Call win32k */
    return NtGdiDdBlt((HANDLE)Blt->lpDDDestSurface->hDDSurface, Surface, (PDD_BLTDATA)Blt);
}

DWORD
APIENTRY
DdDestroySurface(LPDDHAL_DESTROYSURFACEDATA pDestroySurface)
{
    DWORD Return = DDHAL_DRIVER_NOTHANDLED;
    BOOL RealDestroy = TRUE;
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal;

    /* Get the local surface */
    pSurfaceLocal = pDestroySurface->lpDDSurface;

    /* Make sure there's a surface */
    if (pSurfaceLocal->hDDSurface)
    {
        /* Check if we shoudl really destroy it */
        if ((pSurfaceLocal->dwFlags & DDRAWISURF_DRIVERMANAGED) &&
            (pSurfaceLocal->dwFlags & DDRAWISURF_INVALID))
        {
            RealDestroy = FALSE;
        }
        
        /* Call win32k */
        Return = NtGdiDdDestroySurface((HANDLE)pSurfaceLocal->hDDSurface,
                                       RealDestroy);
    }

    return Return;
}

DWORD
WINAPI
DdFlip(LPDDHAL_FLIPDATA Flip)
{
    /* Call win32k */
	
    return NtGdiDdFlip( (HANDLE)Flip->lpSurfCurr->hDDSurface,
                        (HANDLE)Flip->lpSurfTarg->hDDSurface,
   /* FIXME  the two last should be current left handler */ 
						(HANDLE)Flip->lpSurfCurr->hDDSurface,
                        (HANDLE)Flip->lpSurfTarg->hDDSurface,
                        (PDD_FLIPDATA)Flip);
}

WORD
WINAPI
DdLock(LPDDHAL_LOCKDATA Lock)
{
    /* Call win32k */	
    return NtGdiDdLock((HANDLE)Lock->lpDDSurface->hDDSurface,
                        (PDD_LOCKDATA)Lock,
						(HANDLE)Lock->lpDDSurface->hDC);
}

DWORD
WINAPI
DdUnlock(LPDDHAL_UNLOCKDATA Unlock)
{
    /* Call win32k */
    return NtGdiDdUnlock((HANDLE)Unlock->lpDDSurface->hDDSurface,
                          (PDD_UNLOCKDATA)Unlock);
}

DWORD
WINAPI
DdGetBltStatus(LPDDHAL_GETBLTSTATUSDATA GetBltStatus)
{
    /* Call win32k */
    return NtGdiDdGetBltStatus((HANDLE)GetBltStatus->lpDDSurface->hDDSurface,
                                (PDD_GETBLTSTATUSDATA)GetBltStatus);
}

DWORD
WINAPI
DdGetFlipStatus(LPDDHAL_GETFLIPSTATUSDATA GetFlipStatus)
{
    /* Call win32k */
    return NtGdiDdGetFlipStatus((HANDLE)GetFlipStatus->lpDDSurface->hDDSurface,
                                 (PDD_GETFLIPSTATUSDATA)GetFlipStatus);
}

DWORD
APIENTRY
DdUpdateOverlay(LPDDHAL_UPDATEOVERLAYDATA UpdateOverlay)
{
    /* We have to handle this manually here */
    if (UpdateOverlay->dwFlags & DDOVER_KEYDEST)
    {
        /* Use the override */
        UpdateOverlay->dwFlags &= ~DDOVER_KEYDEST;
        UpdateOverlay->dwFlags |=  DDOVER_KEYDESTOVERRIDE;

        /* Set the overlay */
        UpdateOverlay->overlayFX.dckDestColorkey =
            UpdateOverlay->lpDDDestSurface->ddckCKDestOverlay;
    }
    if (UpdateOverlay->dwFlags & DDOVER_KEYSRC)
    {
        /* Use the override */
        UpdateOverlay->dwFlags &= ~DDOVER_KEYSRC;
        UpdateOverlay->dwFlags |=  DDOVER_KEYSRCOVERRIDE;

        /* Set the overlay */
        UpdateOverlay->overlayFX.dckSrcColorkey =
            UpdateOverlay->lpDDSrcSurface->ddckCKSrcOverlay;
    }

    /* Call win32k */
    return NtGdiDdUpdateOverlay((HANDLE)UpdateOverlay->lpDDDestSurface->hDDSurface,
                                 (HANDLE)UpdateOverlay->lpDDSrcSurface->hDDSurface,
                                 (PDD_UPDATEOVERLAYDATA)UpdateOverlay);
}

DWORD
APIENTRY
DdSetOverlayPosition(LPDDHAL_SETOVERLAYPOSITIONDATA SetOverlayPosition)
{
    /* Call win32k */
    return NtGdiDdSetOverlayPosition((HANDLE)SetOverlayPosition->
                                      lpDDSrcSurface->hDDSurface,
                                      (HANDLE)SetOverlayPosition->
                                      lpDDDestSurface->hDDSurface,
                                      (PDD_SETOVERLAYPOSITIONDATA)
                                      SetOverlayPosition);
}

/*
 * Dd Callbacks
 */
DWORD
WINAPI
DdWaitForVerticalBlank(LPDDHAL_WAITFORVERTICALBLANKDATA WaitForVerticalBlank)
{
    /* Call win32k */
    return NtGdiDdWaitForVerticalBlank(GetDdHandle(
                                       WaitForVerticalBlank->lpDD->hDD),
                                       (PDD_WAITFORVERTICALBLANKDATA)
                                       WaitForVerticalBlank);
}

DWORD
WINAPI
DdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA CanCreateSurface)
{
    /* Call win32k */
    return NtGdiDdCanCreateSurface(GetDdHandle(CanCreateSurface->lpDD->hDD),
                                   (PDD_CANCREATESURFACEDATA)CanCreateSurface);
}

DWORD
APIENTRY
DdCreateSurface(LPDDHAL_CREATESURFACEDATA pCreateSurface)
{
    DWORD Return = DDHAL_DRIVER_NOTHANDLED;
    ULONG SurfaceCount = pCreateSurface->dwSCnt;
    DD_SURFACE_LOCAL DdSurfaceLocal;
    DD_SURFACE_MORE DdSurfaceMore;
    DD_SURFACE_GLOBAL DdSurfaceGlobal;
    HANDLE hPrevSurface, hSurface;
    DD_SURFACE_LOCAL* pDdSurfaceLocal;
    DD_SURFACE_MORE* pDdSurfaceMore;
    DD_SURFACE_GLOBAL* pDdSurfaceGlobal;
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal;
    //LPDDRAWI_DDRAWSURFACE_MORE pSurfaceMore;
    LPDDRAWI_DDRAWSURFACE_GBL pSurfaceGlobal;
    PHANDLE phSurface = NULL, puhSurface = NULL;
    ULONG i;
    LPDDSURFACEDESC pSurfaceDesc;

    /* Check how many surfaces there are */
    if (SurfaceCount != 1)
    {
        /* We'll have to allocate more data, our stack isn't big enough */

    }
    else
    {
        /* We'll use what we have on the stack */
        pDdSurfaceLocal = &DdSurfaceLocal;
        pDdSurfaceMore = &DdSurfaceMore;
        pDdSurfaceGlobal = &DdSurfaceGlobal;
        phSurface = &hPrevSurface;
        puhSurface = &hSurface;
        
        /* Clear the structures */
        RtlZeroMemory(&DdSurfaceLocal, sizeof(DdSurfaceLocal));
        RtlZeroMemory(&DdSurfaceGlobal, sizeof(DdSurfaceGlobal));
        RtlZeroMemory(&DdSurfaceMore, sizeof(DdSurfaceMore));  
    }

    /* Loop for each surface */
    for (i = 0; i < pCreateSurface->dwSCnt; i++)
    {
        /* Get data */
        pSurfaceLocal = pCreateSurface->lplpSList[i];
        pSurfaceGlobal = pSurfaceLocal->lpGbl;
        pSurfaceDesc = pCreateSurface->lpDDSurfaceDesc;

        /* Check if it has pixel data */
        if (pSurfaceDesc->dwFlags & DDRAWISURF_HASPIXELFORMAT)
        {
            /* Use its pixel data */
            DdSurfaceGlobal.ddpfSurface = pSurfaceDesc->ddpfPixelFormat;
            DdSurfaceGlobal.ddpfSurface.dwSize = sizeof(DDPIXELFORMAT);
        }
        else
        {
            /* Use the one from the global surface */
            DdSurfaceGlobal.ddpfSurface = pSurfaceGlobal->lpDD->vmiData.ddpfDisplay;
        }

        /* Convert data */
        DdSurfaceGlobal.wWidth = pSurfaceGlobal->wWidth;
        DdSurfaceGlobal.wHeight = pSurfaceGlobal->wHeight;
        DdSurfaceGlobal.lPitch = pSurfaceGlobal->lPitch;
        DdSurfaceGlobal.fpVidMem = pSurfaceGlobal->fpVidMem;
        DdSurfaceGlobal.dwBlockSizeX = pSurfaceGlobal->dwBlockSizeX;
        DdSurfaceGlobal.dwBlockSizeY = pSurfaceGlobal->dwBlockSizeY;
        // DdSurfaceGlobal.ddsCaps = pSurfaceLocal->ddsCaps | 0xBF0000;

        /* FIXME: Ddscapsex stuff missing */

        /* Call win32k now */
        pCreateSurface->ddRVal = E_FAIL;
		
        Return = NtGdiDdCreateSurface(GetDdHandle(pCreateSurface->lpDD->hDD),
                                     (HANDLE *)phSurface,
                                     pSurfaceDesc,
                                     &DdSurfaceGlobal,
                                     &DdSurfaceLocal,
                                     &DdSurfaceMore,
                                     (PDD_CREATESURFACEDATA)pCreateSurface,
                                     puhSurface);
          
	   
        /* FIXME: Ddscapsex stuff missing */
        
        /* Convert the data back */
        pSurfaceGlobal->lPitch = DdSurfaceGlobal.lPitch;
        pSurfaceGlobal->fpVidMem = DdSurfaceGlobal.fpVidMem;
        pSurfaceGlobal->dwBlockSizeX = DdSurfaceGlobal.dwBlockSizeX;
        pSurfaceGlobal->dwBlockSizeY = DdSurfaceGlobal.dwBlockSizeY;
        pCreateSurface->lplpSList[i]->hDDSurface = (DWORD) hSurface;

        /* FIXME: Ddscapsex stuff missing */
    }
    
    /* Check if we have to free all our local allocations */
    if (SurfaceCount > 1)
    {
     /* FIXME: */
    }

    /* Return */
    return Return;
}

DWORD
APIENTRY
DdSetColorKey(LPDDHAL_SETCOLORKEYDATA pSetColorKey)
{
    /* Call win32k */
    return NtGdiDdSetColorKey((HANDLE)pSetColorKey->lpDDSurface->hDDSurface,
                               (PDD_SETCOLORKEYDATA)pSetColorKey);
}

DWORD
APIENTRY
DdGetScanLine(LPDDHAL_GETSCANLINEDATA pGetScanLine)
{
    /* Call win32k */
    return NtGdiDdGetScanLine(GetDdHandle(pGetScanLine->lpDD->hDD),
                               (PDD_GETSCANLINEDATA)pGetScanLine);
}

/* PRIVATE FUNCTIONS *********************************************************/
BOOL
WINAPI
bDDCreateSurface(LPDDRAWI_DDRAWSURFACE_LCL pSurface, 
                 BOOL bComplete)
{
    DD_SURFACE_LOCAL SurfaceLocal;
    DD_SURFACE_GLOBAL SurfaceGlobal;
    DD_SURFACE_MORE SurfaceMore;

    /* Zero struct */
    RtlZeroMemory(&SurfaceLocal, sizeof(DD_SURFACE_LOCAL));
    RtlZeroMemory(&SurfaceGlobal, sizeof(DD_SURFACE_GLOBAL));
    RtlZeroMemory(&SurfaceMore, sizeof(DD_SURFACE_MORE));

    /* Set up SurfaceLocal struct */
    SurfaceLocal.ddsCaps.dwCaps = pSurface->ddsCaps.dwCaps;
    SurfaceLocal.dwFlags = pSurface->dwFlags;

    /* Set up SurfaceMore struct */
    RtlMoveMemory(&SurfaceMore.ddsCapsEx,
                  &pSurface->ddckCKDestBlt,
                  sizeof(DDSCAPSEX));
    SurfaceMore.dwSurfaceHandle = (DWORD)pSurface->dbnOverlayNode.object_int->lpVtbl;

    /* Set up SurfaceGlobal struct */
    SurfaceGlobal.fpVidMem = pSurface->lpGbl->fpVidMem;
    SurfaceGlobal.dwLinearSize = pSurface->lpGbl->dwLinearSize;
    SurfaceGlobal.wHeight = pSurface->lpGbl->wHeight;
    SurfaceGlobal.wWidth = pSurface->lpGbl->wWidth;

    /* Check if we have a pixel format */
    if (pSurface->dwFlags & DDSD_PIXELFORMAT)
    {	
        /* Use global one */
        SurfaceGlobal.ddpfSurface = pSurface->lpGbl->lpDD->vmiData.ddpfDisplay;
        SurfaceGlobal.ddpfSurface.dwSize = sizeof(DDPIXELFORMAT);
    }
    else
    {
        /* Use local one */
        SurfaceGlobal.ddpfSurface = pSurface->lpGbl->lpDD->vmiData.ddpfDisplay;
    }

    /* Create the object */
    pSurface->hDDSurface = (DWORD)NtGdiDdCreateSurfaceObject(GetDdHandle(pSurface->lpGbl->lpDD->hDD),
                                                             (HANDLE)pSurface->hDDSurface,
                                                             &SurfaceLocal,
                                                             &SurfaceMore,
                                                             &SurfaceGlobal,
                                                             bComplete);

    /* Return status */
    if (pSurface->hDDSurface) return TRUE;
    return FALSE;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 *
 * GDIEntry 1 
 */
BOOL 
WINAPI 
DdCreateDirectDrawObject(LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
                         HDC hdc)
{  
    BOOL Return = FALSE;

    /* Check if the global hDC (hdc == 0) is being used */
    if (!hdc)
  {
        /* We'll only allow this if the global object doesn't exist yet */
        if (!ghDirectDraw)
        {
            /* Create the DC */
            if ((hdc = CreateDC(L"Display", NULL, NULL, NULL)))
            {
                /* Create the DDraw Object */
                ghDirectDraw = NtGdiDdCreateDirectDrawObject(hdc);

                /* Delete our DC */                
				NtGdiDeleteObjectApp(hdc);
            }
        }

        /* If we created the object, or had one ...*/
        if (ghDirectDraw)
        {
            /* Increase count and set success */
            gcDirectDraw++;
            Return = TRUE;
        }

        /* Zero the handle */
        pDirectDrawGlobal->hDD = 0;
    }
    else
    {
        /* Using the per-process object, so create it */
    pDirectDrawGlobal->hDD = (ULONG_PTR)NtGdiDdCreateDirectDrawObject(hdc); 
    
        /* Set the return value */
        Return = pDirectDrawGlobal->hDD ? TRUE : FALSE;
    }

    /* Return to caller */
    return Return;
}




static LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobalInternal;
static ULONG RemberDdQueryDisplaySettingsUniquenessID = 0;

BOOL
intDDCreateSurface ( LPDDRAWI_DDRAWSURFACE_LCL pSurface, 
				     BOOL bComplete);




/*
 * @unimplemented
 */
BOOL
STDCALL
DdQueryDirectDrawObject(LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
                        LPDDHALINFO pHalInfo,
                        LPDDHAL_DDCALLBACKS pDDCallbacks,
                        LPDDHAL_DDSURFACECALLBACKS pDDSurfaceCallbacks,
                        LPDDHAL_DDPALETTECALLBACKS pDDPaletteCallbacks,
                        LPD3DHAL_CALLBACKS pD3dCallbacks,
                        LPD3DHAL_GLOBALDRIVERDATA pD3dDriverData,
                        LPDDHAL_DDEXEBUFCALLBACKS pD3dBufferCallbacks,
                        LPDDSURFACEDESC pD3dTextureFormats,
                        LPDWORD pdwFourCC,
                        LPVIDMEM pvmList)
{
    BOOL bStatus = FALSE;
	DD_HALINFO DDHalInfo;
	LPVOID pCallBackFlags[3];
	DWORD NumHeaps;
	DWORD NumFourCC; 

	DDHalInfo.dwSize = sizeof(DD_HALINFO);

	pCallBackFlags[0] = pDDCallbacks;
    pCallBackFlags[1] = pDDSurfaceCallbacks;
	pCallBackFlags[2] = pDDPaletteCallbacks;
    	
	bStatus = NtGdiDdQueryDirectDrawObject(
		      (HANDLE)pDirectDrawGlobal->hDD,
			  (DD_HALINFO *)&DDHalInfo,
			  (DWORD *)pCallBackFlags,
			  (LPD3DNTHAL_CALLBACKS)pD3dCallbacks,
              (LPD3DNTHAL_GLOBALDRIVERDATA)pD3dDriverData,
			  (PDD_D3DBUFCALLBACKS)pD3dBufferCallbacks,
			  (LPDDSURFACEDESC)pD3dTextureFormats,
			  (DWORD *)&NumHeaps,
			  (VIDEOMEMORY *)pvmList,
			  (DWORD *)&NumFourCC,
              (DWORD *)pdwFourCC);

    	
	//SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return bStatus;
}

/*
 * @unimplemented
 */
HBITMAP
STDCALL
DdCreateDIBSection(HDC hdc,
                   CONST BITMAPINFO *pbmi,
                   UINT iUsage,
                   VOID **ppvBits,
                   HANDLE hSectionApp,
                   DWORD dwOffset)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @implemented
 *
 * GDIEntry 3
 */
BOOL 
STDCALL 
DdDeleteDirectDrawObject(LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal)
{
  BOOL status;                                               
  /* if pDirectDrawGlobal->hDD == NULL and pDirectDrawGlobalInternal->hDD == NULL
     return false */

  if (!pDirectDrawGlobal->hDD)
  {
     if (!pDirectDrawGlobalInternal->hDD)
     {
       return FALSE;
     }
    return NtGdiDdDeleteDirectDrawObject((HANDLE)pDirectDrawGlobalInternal->hDD); 
  }
  
  status = NtGdiDdDeleteDirectDrawObject((HANDLE)pDirectDrawGlobal->hDD); 	
  if ((status == TRUE) && (pDirectDrawGlobalInternal->hDD != 0))
  {
     pDirectDrawGlobalInternal->hDD = 0;        
  }
     
  return status; 	
}

/*
 * @implemented
 *
 * GDIEntry 4
 */
BOOL 
STDCALL 
DdCreateSurfaceObject( LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
                       BOOL bPrimarySurface)
{
	return intDDCreateSurface(pSurfaceLocal,1);	
}

/*
 * @implemented
 *
 * GDIEntry 5
 */
BOOL 
STDCALL 
DdDeleteSurfaceObject(LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal)
{
  if (!pSurfaceLocal->hDDSurface)
  {
    return FALSE;
  }

  return NtGdiDdDeleteSurfaceObject((HANDLE)pSurfaceLocal->hDDSurface);
}

/*
 * @implemented
 *
 * GDIEntry 6
 */
BOOL 
STDCALL 
DdResetVisrgn(LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal, 
              HWND hWnd)
{
 return NtGdiDdResetVisrgn((HANDLE) pSurfaceLocal->hDDSurface, hWnd);
}

/*
 * @implemented
 *
 * GDIEntry 7
 */
HDC STDCALL DdGetDC( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
LPPALETTEENTRY pColorTable
)
{
	return NtGdiDdGetDC(pColorTable, (HANDLE) pSurfaceLocal->hDDSurface);
}

/*
 * @implemented
 *
 * GDIEntry 8
 */
BOOL STDCALL DdReleaseDC( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal
)
{
 return NtGdiDdReleaseDC((HANDLE) pSurfaceLocal->hDDSurface);
}



/*
 * @implemented
 *
 * GDIEntry 10
 */
BOOL 
STDCALL 
DdReenableDirectDrawObject(LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
                           BOOL *pbNewMode)
{
 if (!pDirectDrawGlobal->hDD)
  {
     if (!pDirectDrawGlobalInternal->hDD)
     {
       return FALSE;
     }
    return NtGdiDdReenableDirectDrawObject((HANDLE)pDirectDrawGlobalInternal->hDD, pbNewMode); 
  }

  return NtGdiDdReenableDirectDrawObject((HANDLE)pDirectDrawGlobal->hDD, pbNewMode); 	
} 

/*
 * @implemented
 *
 * GDIEntry 11
 */
BOOL 
STDCALL 
DdAttachSurface( LPDDRAWI_DDRAWSURFACE_LCL pSurfaceFrom,
                 LPDDRAWI_DDRAWSURFACE_LCL pSurfaceTo)
{

 /* Create Surface if it does not exits one */
 if (pSurfaceFrom->hDDSurface)
 {    
	if (!intDDCreateSurface(pSurfaceFrom,FALSE))
	{
	  return FALSE;
	}
 }

 /* Create Surface if it does not exits one */
 if (pSurfaceTo->hDDSurface)
 {    
	if (!intDDCreateSurface(pSurfaceTo,FALSE))
	{
	  return FALSE;
	}
 }

 return NtGdiDdAttachSurface( (HANDLE) pSurfaceFrom->hDDSurface, (HANDLE) pSurfaceTo->hDDSurface);
}

/*
 * @implemented
 *
 * GDIEntry 12
 */
VOID 
STDCALL 
DdUnattachSurface(LPDDRAWI_DDRAWSURFACE_LCL pSurface,
                  LPDDRAWI_DDRAWSURFACE_LCL pSurfaceAttached)
{
  NtGdiDdUnattachSurface((HANDLE) pSurface->hDDSurface, (HANDLE) pSurfaceAttached->hDDSurface);	
}

/*
 * @implemented
 *
 * GDIEntry 13
 */
ULONG
STDCALL 
DdQueryDisplaySettingsUniqueness()
{
 return RemberDdQueryDisplaySettingsUniquenessID;
}

/*
 * @implemented
 *
 * GDIEntry 14
 */
HANDLE 
STDCALL 
DdGetDxHandle(LPDDRAWI_DIRECTDRAW_LCL pDDraw,
              LPDDRAWI_DDRAWSURFACE_LCL pSurface,
              BOOL bRelease)
{
 if (pSurface) 
 {                              
   return ((HANDLE) NtGdiDdGetDxHandle(NULL, (HANDLE)pSurface->hDDSurface, bRelease));    
 }

 
 if (!pDDraw->lpGbl->hDD)
  {
     if (!pDirectDrawGlobalInternal->hDD)
     {
       return FALSE;
     }
   return ((HANDLE) NtGdiDdGetDxHandle( (HANDLE) pDirectDrawGlobalInternal->hDD, (HANDLE) pSurface->hDDSurface, bRelease));
  }

  return ((HANDLE) NtGdiDdGetDxHandle((HANDLE)pDDraw->lpGbl->hDD, (HANDLE) pSurface->hDDSurface, bRelease));
}

/*
 * @implemented
 *
 * GDIEntry 15
 */
BOOL STDCALL DdSetGammaRamp( 
LPDDRAWI_DIRECTDRAW_LCL pDDraw,
HDC hdc,
LPVOID lpGammaRamp
)
{
	if (!pDDraw->lpGbl->hDD)
  {
     if (!pDirectDrawGlobalInternal->hDD)
     {
       return FALSE;
     }
    return NtGdiDdSetGammaRamp((HANDLE)pDirectDrawGlobalInternal->hDD,hdc,lpGammaRamp);
  }

  return NtGdiDdSetGammaRamp((HANDLE)pDDraw->lpGbl->hDD,hdc,lpGammaRamp);
}

/*
 * @implemented
 *
 * GDIEntry 16
 */
DWORD STDCALL DdSwapTextureHandles( 
LPDDRAWI_DIRECTDRAW_LCL pDDraw,
LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl1,
LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl2
)
{  
	return TRUE;
}


/* interal create surface */
BOOL
intDDCreateSurface ( LPDDRAWI_DDRAWSURFACE_LCL pSurface, 
				     BOOL bComplete)
{
  DD_SURFACE_LOCAL SurfaceLocal;
  DD_SURFACE_GLOBAL SurfaceGlobal;
  DD_SURFACE_MORE SurfaceMore;

  /* Zero struct */
  RtlZeroMemory(&SurfaceLocal, sizeof(DD_SURFACE_LOCAL));
  RtlZeroMemory(&SurfaceGlobal, sizeof(DD_SURFACE_GLOBAL));
  RtlZeroMemory(&SurfaceMore, sizeof(DD_SURFACE_MORE));

  /* Set up SurfaceLocal struct */
  SurfaceLocal.ddsCaps.dwCaps = pSurface->ddsCaps.dwCaps;
  SurfaceLocal.dwFlags = pSurface->dwFlags;

  /* Set up SurfaceMore struct */
  /* copy  pSurface->ddckCKDestBlt and pSurface->ddckCKSrcBlt to SurfaceMore.ddsCapsEx */  
  memcpy(&SurfaceMore.ddsCapsEx, &pSurface->ddckCKDestBlt, sizeof(DDSCAPSEX));   
  SurfaceMore.dwSurfaceHandle =  (DWORD) pSurface->dbnOverlayNode.object_int->lpVtbl; 


  /* Set up SurfaceGlobal struct */
  SurfaceGlobal.fpVidMem = pSurface->lpGbl->fpVidMem;
  SurfaceGlobal.dwLinearSize = pSurface->lpGbl->dwLinearSize;
  SurfaceGlobal.wHeight = pSurface->lpGbl->wHeight;
  SurfaceGlobal.wWidth = pSurface->lpGbl->wWidth;

  /* check which memory type should be use */
  if ((pSurface->dwFlags & DDRAWISURFGBL_LOCKVRAMSTYLE) == DDRAWISURFGBL_LOCKVRAMSTYLE)
  {	
	  memcpy(&SurfaceGlobal.ddpfSurface,&pSurface->lpGbl->lpDD->vmiData.ddpfDisplay, sizeof(DDPIXELFORMAT));
  }
  else
  {
	  memcpy(&SurfaceGlobal.ddpfSurface,&pSurface->lpGbl->ddpfSurface, sizeof(DDPIXELFORMAT));
  }

  /* Determer if Gdi32 chace of directdraw handler or not */
  if (pSurface->lpGbl->lpDD->hDD)
  {
     pSurface->hDDSurface = ((DWORD) NtGdiDdCreateSurfaceObject( (HANDLE) pSurface->lpGbl->lpDD->hDD,
	                                                (HANDLE) pSurface->hDDSurface, &SurfaceLocal, 
												    &SurfaceMore, &SurfaceGlobal, bComplete));
  }
  else
  {
     pSurface->hDDSurface = ((DWORD) NtGdiDdCreateSurfaceObject( (HANDLE) pDirectDrawGlobalInternal->hDD,
	                                                (HANDLE) pSurface->hDDSurface, &SurfaceLocal, 
												    &SurfaceMore, 
													&SurfaceGlobal, 
													bComplete));
  }

  /* return status */
  if (pSurface->hDDSurface) 
  {
    return TRUE;
  }

  return FALSE;
}
