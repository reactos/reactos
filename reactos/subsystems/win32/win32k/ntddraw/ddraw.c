/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/ddraw.c
 * PROGRAMER:        Peter Bajusz (hyp-x@stormregion.com)
 * REVISION HISTORY:
 *       25-10-2003  PB  Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* swtich this off to get rid of all dx debug msg */
#define DX_DEBUG

#define DdHandleTable GdiHandleTable


/************************************************************************/
/* DIRECT DRAW OBJECT                                                   */
/************************************************************************/

BOOL INTERNAL_CALL
DD_Cleanup(PVOID ObjectBody)
{       
	PDD_DIRECTDRAW pDirectDraw = (PDD_DIRECTDRAW) ObjectBody;
#ifdef DX_DEBUG
	DPRINT1("DD_Cleanup\n");
#endif
	
	if (!pDirectDraw)
		return FALSE;
    
    if (pDirectDraw->Global.dhpdev == NULL)
        return FALSE;
        
    if (pDirectDraw->DrvDisableDirectDraw == NULL)
        return FALSE;
        
	pDirectDraw->DrvDisableDirectDraw(pDirectDraw->Global.dhpdev);
	return TRUE;
}

HANDLE STDCALL NtGdiDdCreateDirectDrawObject(
    HDC hdc
)
{
	DD_CALLBACKS callbacks;
	DD_SURFACECALLBACKS surface_callbacks;
	DD_PALETTECALLBACKS palette_callbacks;
    DC *pDC;
    BOOL success;
    HANDLE hDirectDraw;
    PDD_DIRECTDRAW pDirectDraw;
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdCreateDirectDrawObject\n");
#endif

	RtlZeroMemory(&callbacks, sizeof(DD_CALLBACKS));
	callbacks.dwSize = sizeof(DD_CALLBACKS);
	RtlZeroMemory(&surface_callbacks, sizeof(DD_SURFACECALLBACKS));
	surface_callbacks.dwSize = sizeof(DD_SURFACECALLBACKS);
	RtlZeroMemory(&palette_callbacks, sizeof(DD_PALETTECALLBACKS));
	palette_callbacks.dwSize = sizeof(DD_PALETTECALLBACKS);

	/* FIXME hdc can be zero for d3d9 */
    /* we need create it, if in that case */
	if (hdc == NULL)
	{
	    return NULL;
    }
    
	pDC = DC_LockDc(hdc);
	if (!pDC)
		return NULL;

	if (!pDC->DriverFunctions.EnableDirectDraw)
	{
		// Driver doesn't support DirectDraw
		DC_UnlockDc(pDC);
		return NULL;
	}
	
	success = pDC->DriverFunctions.EnableDirectDraw(
		pDC->PDev, &callbacks, &surface_callbacks, &palette_callbacks);

	if (!success)
	{
#ifdef DX_DEBUG
        DPRINT1("DirectDraw creation failed\n"); 
#endif
		// DirectDraw creation failed
		DC_UnlockDc(pDC);
		return NULL;
	}

	hDirectDraw = GDIOBJ_AllocObj(DdHandleTable, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (!hDirectDraw)
	{
		/* No more memmory */
#ifdef DX_DEBUG
		DPRINT1("No more memmory\n"); 
#endif
		DC_UnlockDc(pDC);
		return NULL;
	}

	pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDraw, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (!pDirectDraw)
	{
		/* invalid handle */
#ifdef DX_DEBUG
		DPRINT1("invalid handle\n"); 
#endif
		DC_UnlockDc(pDC);
		return NULL;
	}
	

	pDirectDraw->Global.dhpdev = pDC->PDev;
	pDirectDraw->Local.lpGbl = &pDirectDraw->Global;

	pDirectDraw->DrvGetDirectDrawInfo = pDC->DriverFunctions.GetDirectDrawInfo;
	pDirectDraw->DrvDisableDirectDraw = pDC->DriverFunctions.DisableDirectDraw;

    /* DD_CALLBACKS setup */	
	RtlMoveMemory(&pDirectDraw->DD, &callbacks, sizeof(DD_CALLBACKS));
	
   	/* DD_SURFACECALLBACKS  setup*/	
	RtlMoveMemory(&pDirectDraw->Surf, &surface_callbacks, sizeof(DD_SURFACECALLBACKS));

	/* DD_PALETTECALLBACKS setup*/	
	RtlMoveMemory(&pDirectDraw->Pal, &surface_callbacks, sizeof(DD_PALETTECALLBACKS));
	
	GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
	DC_UnlockDc(pDC);

	return hDirectDraw;
}

BOOL STDCALL NtGdiDdDeleteDirectDrawObject(
    HANDLE hDirectDrawLocal
)
{
#ifdef DX_DEBUG
    DPRINT1("NtGdiDdDeleteDirectDrawObject\n");
#endif
	return GDIOBJ_FreeObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
}

BOOL STDCALL NtGdiDdQueryDirectDrawObject(
    HANDLE hDirectDrawLocal,
    DD_HALINFO  *pHalInfo,
    DWORD *pCallBackFlags,
    LPD3DNTHAL_CALLBACKS puD3dCallbacks,
    LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData,
    PDD_D3DBUFCALLBACKS puD3dBufferCallbacks,
    LPDDSURFACEDESC puD3dTextureFormats,
    DWORD *puNumHeaps,
    VIDEOMEMORY *puvmList,
    DWORD *puNumFourCC,
    DWORD *puFourCC
)
{
#ifdef DX_DEBUG
    PDD_DIRECTDRAW pDirectDraw;
    BOOL success;
    DPRINT1("NtGdiDdQueryDirectDrawObject\n");
#endif

    /* Check for NULL pointer to prevent any one doing a mistake */

    if (hDirectDrawLocal == NULL)
    {
#ifdef DX_DEBUG
       DPRINT1("warning hDirectDraw handler is NULL, the handler is  DDRAWI_DIRECTDRAW_GBL.hDD\n");
       DPRINT1("and it is NtGdiDdCreateDirectDrawObject return value\n");
#endif
       return FALSE;
    }


    if (pHalInfo == NULL)
    {
#ifdef DX_DEBUG
       DPRINT1("warning pHalInfo buffer is NULL \n");
#endif
       return FALSE;
    }

    if ( pCallBackFlags == NULL)
    {
#ifdef DX_DEBUG
       DPRINT1("warning pCallBackFlags s NULL, the size must be 3*DWORD in follow order \n");
       DPRINT1("pCallBackFlags[0] = flags in DD_CALLBACKS\n");
       DPRINT1("pCallBackFlags[1] = flags in DD_SURFACECALLBACKS\n");
       DPRINT1("pCallBackFlags[2] = flags in DD_PALETTECALLBACKS\n");
#endif
       return FALSE;
    }
   
    
	pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
	
	
	if (!pDirectDraw)
	{
        /* Fail to Lock DirectDraw handle */
#ifdef DX_DEBUG
        DPRINT1(" Fail to Lock DirectDraw handle \n");        
#endif
		return FALSE;
    }

	success = pDirectDraw->DrvGetDirectDrawInfo(
		pDirectDraw->Global.dhpdev,
		pHalInfo,
		puNumHeaps,
		puvmList,
		puNumFourCC,
		puFourCC);

	if (!success)
	{
#ifdef DX_DEBUG
        DPRINT1(" Fail to get DirectDraw driver info \n");
#endif
		GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
		return FALSE;
	}

      
    /* rest the flag so we do not need do it later */
    pCallBackFlags[0]=0;
    pCallBackFlags[1]=0;
    pCallBackFlags[2]=0;

	if (pHalInfo)
	{       
      
          {
             DDHALINFO* pHalInfo2 = ((DDHALINFO*) pHalInfo);
#ifdef DX_DEBUG
             DPRINT1("Found DirectDraw CallBack for 2D and 3D Hal\n");
#endif
             RtlMoveMemory(&pDirectDraw->Hal, pHalInfo2, sizeof(DDHALINFO));

             if (pHalInfo2->lpDDExeBufCallbacks)
	         {
#ifdef DX_DEBUG
                 DPRINT1("Found DirectDraw CallBack for 3D Hal Bufffer  \n");                                
#endif
                 /* msdn DDHAL_D3DBUFCALLBACKS = DD_D3DBUFCALLBACKS */
                 RtlMoveMemory(puD3dBufferCallbacks, pHalInfo2->lpDDExeBufCallbacks, sizeof(DD_D3DBUFCALLBACKS));
             }
              
#ifdef DX_DEBUG               
             DPRINT1("Do not support CallBack for 3D Hal\n");
#endif
             /* FIXME we need D3DHAL be include 

             if (pHalInfo2->lpD3DHALCallbacks )
	         {    
#ifdef DX_DEBUG
                    DPRINT1("Found DirectDraw CallBack for 3D Hal\n");
#endif
		            RtlMoveMemory(puD3dCallbacks, (ULONG *)pHalInfo2->lpD3DHALCallbacks, sizeof( D3DHAL_CALLBACKS ));		
	         } 
             */              


             /* msdn say D3DHAL_GLOBALDRIVERDATA and D3DNTHAL_GLOBALDRIVERDATA are not same 
                but if u compare these in msdn it is exacly same */

	         if (pHalInfo->lpD3DGlobalDriverData)
	         {
#ifdef DX_DEBUG
                   DPRINT1("Found DirectDraw CallBack for 3D Hal Private  \n");
#endif
		           RtlMoveMemory(puD3dDriverData, (ULONG *)pHalInfo2->lpD3DGlobalDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA));
	         }
              
             /* build the flag */
               
             if (pHalInfo2->lpDDCallbacks!=NULL)
             {
#ifdef DX_DEBUG
                    DPRINT1("Dectect DirectDraw lpDDCallbacks for 2D Hal flag = %d\n",pHalInfo2->lpDDCallbacks->dwFlags);
#endif
                    pCallBackFlags[0] = pHalInfo2->lpDDCallbacks->dwFlags;
             }
     
             if (pHalInfo2->lpDDCallbacks!=NULL)
             {
#ifdef DX_DEBUG
                   DPRINT1("Dectect DirectDraw lpDDSurfaceCallbacks for 2D Hal flag = %d\n",pHalInfo2->lpDDSurfaceCallbacks->dwFlags);
#endif
                   pCallBackFlags[1] = pHalInfo2->lpDDSurfaceCallbacks->dwFlags;
             }
       
             if (pHalInfo2->lpDDCallbacks!=NULL)
             {
#ifdef DX_DEBUG
                   DPRINT1("Dectect DirectDraw lpDDCallbacks for 2D Hal flag = %d\n",pHalInfo2->lpDDPaletteCallbacks->dwFlags);
#endif
                   pCallBackFlags[2] = pHalInfo2->lpDDPaletteCallbacks->dwFlags;
             }

          }
             
#ifdef DX_DEBUG
          DPRINT1("Found DirectDraw CallBack for 3D Hal\n");
#endif
          RtlMoveMemory(&pDirectDraw->Hal, pHalInfo, sizeof(DD_HALINFO));

          if (pHalInfo->lpD3DBufCallbacks)
	      {
#ifdef DX_DEBUG
                   DPRINT1("Found DirectDraw CallBack for 3D Hal Bufffer  \n");
#endif
		           RtlMoveMemory(puD3dBufferCallbacks, pHalInfo->lpD3DBufCallbacks, sizeof(DD_D3DBUFCALLBACKS));
	      }

          if (pHalInfo->lpD3DHALCallbacks)
	      {
#ifdef DX_DEBUG
                   DPRINT1("Found DirectDraw CallBack for 3D Hal\n");
#endif
		           RtlMoveMemory(puD3dCallbacks, pHalInfo->lpD3DHALCallbacks, sizeof(D3DNTHAL_CALLBACKS));		
	      }

	      if (pHalInfo->lpD3DGlobalDriverData)
	      {
#ifdef DX_DEBUG
                   DPRINT1("Found DirectDraw CallBack for 3D Hal Private  \n");
#endif
		           RtlMoveMemory(puD3dDriverData, pHalInfo->lpD3DGlobalDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA));
	      }
          
#ifdef DX_DEBUG
          DPRINT1("Unkown DirectX driver interface\n");
#endif
                            	   	          	   	                           	   	
	   }

#ifdef DX_DEBUG
     else
	 {
	   DPRINT1("No DirectDraw Hal info have been found, it did not fail, it did gather some other info \n");
    }
#endif
        
	GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);

	return TRUE;
}


DWORD STDCALL NtGdiDdGetDriverInfo(
    HANDLE hDirectDrawLocal,
    PDD_GETDRIVERINFODATA puGetDriverInfoData)

{
	DWORD  ddRVal = 0;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdGetDriverInfo\n");
#endif
	
	if (pDirectDraw == NULL) 
	{
#ifdef DX_DEBUG
        DPRINT1("Can not lock DirectDraw handle \n");
#endif
		return DDHAL_DRIVER_NOTHANDLED;
    }

    
    /* it exsist two version of NtGdiDdGetDriverInfo we need check for both flags */
    if (!(pDirectDraw->Hal.dwFlags & DDHALINFO_GETDRIVERINFOSET))
         ddRVal++;
         
    if (!(pDirectDraw->Hal.dwFlags & DDHALINFO_GETDRIVERINFO2))
         ddRVal++;
                    
    
    /* Now we are doing the call to drv DrvGetDriverInfo */
	if   (ddRVal == 2)
	{
#ifdef DX_DEBUG
         DPRINT1("NtGdiDdGetDriverInfo DDHAL_DRIVER_NOTHANDLED");         
#endif
	     ddRVal = DDHAL_DRIVER_NOTHANDLED;
    }
	else
	     ddRVal = pDirectDraw->Hal.GetDriverInfo(puGetDriverInfoData);
   
    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);

	return ddRVal;
}

/************************************************************************/
/* DD CALLBACKS                                                         */
/* FIXME NtGdiDdCreateSurface we do not call to ddCreateSurface         */
/************************************************************************/

DWORD STDCALL NtGdiDdCreateSurface(
    HANDLE hDirectDrawLocal,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    PDD_CREATESURFACEDATA puCreateSurfaceData,
    HANDLE *puhSurface
)
{
	DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    PDD_DIRECTDRAW pDirectDraw;
	PDD_DIRECTDRAW_GLOBAL lgpl;
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdCreateSurface\n");
#endif

	pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
	{
#ifdef DX_DEBUG
       	DPRINT1("Can not lock the DirectDraw handle\n");
#endif
		return DDHAL_DRIVER_NOTHANDLED;
    }
	
	/* backup the orignal PDev and info */
	lgpl = puCreateSurfaceData->lpDD;

	/* use our cache version instead */
	puCreateSurfaceData->lpDD = &pDirectDraw->Global;
	
	/* make the call */
	if (!(pDirectDraw->DD.dwFlags & DDHAL_CB32_CANCREATESURFACE))
	{
#ifdef DX_DEBUG
        DPRINT1("DirectDraw HAL does not support Create Surface"); 
#endif
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
    }
	else
	{	  
	   ddRVal = pDirectDraw->DD.CreateSurface(puCreateSurfaceData);	 
	}

	/* But back the orignal PDev */
	puCreateSurfaceData->lpDD = lgpl;
    
	GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);	
	return ddRVal;
}

DWORD STDCALL NtGdiDdWaitForVerticalBlank(
    HANDLE hDirectDrawLocal,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;
    PDD_DIRECTDRAW pDirectDraw;
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdWaitForVerticalBlank\n");
#endif


	pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puWaitForVerticalBlankData->lpDD;

	/* use our cache version instead */
	puWaitForVerticalBlankData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->DD.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else	
  	    ddRVal = pDirectDraw->DD.WaitForVerticalBlank(puWaitForVerticalBlankData);

	/* But back the orignal PDev */
	puWaitForVerticalBlankData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
	return ddRVal;
}

DWORD STDCALL NtGdiDdCanCreateSurface(
    HANDLE hDirectDrawLocal,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;	

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdCanCreateSurface\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puCanCreateSurfaceData->lpDD;

	/* use our cache version instead */
	puCanCreateSurfaceData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->DD.dwFlags & DDHAL_CB32_CANCREATESURFACE))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else	
	    ddRVal = pDirectDraw->DD.CanCreateSurface(puCanCreateSurfaceData);

	/* But back the orignal PDev */
	puCanCreateSurfaceData->lpDD = lgpl;

	GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
	return ddRVal;
}

DWORD STDCALL NtGdiDdGetScanLine(
    HANDLE hDirectDrawLocal,
    PDD_GETSCANLINEDATA puGetScanLineData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdGetScanLine\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puGetScanLineData->lpDD;

	/* use our cache version instead */
	puGetScanLineData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->DD.dwFlags & DDHAL_CB32_GETSCANLINE))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else	
	    ddRVal = pDirectDraw->DD.GetScanLine(puGetScanLineData);

	/* But back the orignal PDev */
	puGetScanLineData->lpDD = lgpl;

	GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
	return ddRVal;
}



/************************************************************************/
/* Surface CALLBACKS                                                    */
/* FIXME                                                                */
/* NtGdiDdDestroySurface                                                */ 
/************************************************************************/

DWORD STDCALL NtGdiDdDestroySurface(
    HANDLE hSurface,
    BOOL bRealDestroy
)
{	
	DWORD  ddRVal  = DDHAL_DRIVER_NOTHANDLED;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdDestroySurface\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_DESTROYSURFACE))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else	
	{
		DD_DESTROYSURFACEDATA DestroySurf; 
	
		/* FIXME 
		 * bRealDestroy 
		 * are we doing right ??
		 */
        DestroySurf.lpDD =  &pDirectDraw->Global;

        DestroySurf.lpDDSurface = hSurface;  // ?
        DestroySurf.DestroySurface = pDirectDraw->Surf.DestroySurface;		
		
        ddRVal = pDirectDraw->Surf.DestroySurface(&DestroySurf); 
	}

	
    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;			
}

DWORD STDCALL NtGdiDdFlip(
    HANDLE hSurfaceCurrent,
    HANDLE hSurfaceTarget,
    HANDLE hSurfaceCurrentLeft,
    HANDLE hSurfaceTargetLeft,
    PDD_FLIPDATA puFlipData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurfaceTarget, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdFlip\n");
#endif
	
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puFlipData->lpDD;

	/* use our cache version instead */
	puFlipData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_FLIP))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.Flip(puFlipData);

	/* But back the orignal PDev */
	puFlipData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;		
}

DWORD STDCALL NtGdiDdLock(
    HANDLE hSurface,
    PDD_LOCKDATA puLockData,
    HDC hdcClip
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdLock\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puLockData->lpDD;

	/* use our cache version instead */
	puLockData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_LOCK))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.Lock(puLockData);

	/* But back the orignal PDev */
	puLockData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;		
}

DWORD STDCALL NtGdiDdUnlock(
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdUnlock\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puUnlockData->lpDD;

	/* use our cache version instead */
	puUnlockData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_UNLOCK))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.Unlock(puUnlockData);

	/* But back the orignal PDev */
	puUnlockData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);    
	return ddRVal;
}

DWORD STDCALL NtGdiDdBlt(
    HANDLE hSurfaceDest,
    HANDLE hSurfaceSrc,
    PDD_BLTDATA puBltData
)
{
    DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

    PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurfaceDest, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
    DPRINT1("NtGdiDdBlt\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puBltData->lpDD;

	/* use our cache version instead */
	puBltData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_BLT))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.Blt(puBltData);

	/* But back the orignal PDev */
	puBltData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;
   }

DWORD STDCALL NtGdiDdSetColorKey(
    HANDLE hSurface,
    PDD_SETCOLORKEYDATA puSetColorKeyData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdSetColorKey\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puSetColorKeyData->lpDD;

	/* use our cache version instead */
	puSetColorKeyData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_SETCOLORKEY))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.SetColorKey(puSetColorKeyData);

	/* But back the orignal PDev */
	puSetColorKeyData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;	
}


DWORD STDCALL NtGdiDdAddAttachedSurface(
    HANDLE hSurface,
    HANDLE hSurfaceAttached,
    PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurfaceAttached, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdAddAttachedSurface\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puAddAttachedSurfaceData->lpDD;

	/* use our cache version instead */
	puAddAttachedSurfaceData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_ADDATTACHEDSURFACE))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.AddAttachedSurface(puAddAttachedSurfaceData);

	/* But back the orignal PDev */
	puAddAttachedSurfaceData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;	
}

DWORD STDCALL NtGdiDdGetBltStatus(
    HANDLE hSurface,
    PDD_GETBLTSTATUSDATA puGetBltStatusData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdGetBltStatus\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puGetBltStatusData->lpDD;

	/* use our cache version instead */
	puGetBltStatusData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_GETBLTSTATUS))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.GetBltStatus(puGetBltStatusData);

	/* But back the orignal PDev */
	puGetBltStatusData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;		
}

DWORD STDCALL NtGdiDdGetFlipStatus(
    HANDLE hSurface,
    PDD_GETFLIPSTATUSDATA puGetFlipStatusData
)
{
    DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdGetFlipStatus\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puGetFlipStatusData->lpDD;

	/* use our cache version instead */
	puGetFlipStatusData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_GETFLIPSTATUS))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.GetFlipStatus(puGetFlipStatusData);

	/* But back the orignal PDev */
	puGetFlipStatusData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;		
}

DWORD STDCALL NtGdiDdUpdateOverlay(
    HANDLE hSurfaceDestination,
    HANDLE hSurfaceSource,
    PDD_UPDATEOVERLAYDATA puUpdateOverlayData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

    PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurfaceDestination, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
    DPRINT1("NtGdiDdUpdateOverlay\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puUpdateOverlayData->lpDD;

	/* use our cache version instead */
	puUpdateOverlayData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_UPDATEOVERLAY))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.UpdateOverlay(puUpdateOverlayData);

	/* But back the orignal PDev */
	puUpdateOverlayData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;
}

DWORD STDCALL NtGdiDdSetOverlayPosition(
    HANDLE hSurfaceSource,
    HANDLE hSurfaceDestination,
    PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

    PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurfaceDestination, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
    DPRINT1("NtGdiDdSetOverlayPosition\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puSetOverlayPositionData->lpDD;

	/* use our cache version instead */
	puSetOverlayPositionData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_SETOVERLAYPOSITION))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.SetOverlayPosition(puSetOverlayPositionData);

	/* But back the orignal PDev */
	puSetOverlayPositionData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;
}


/************************************************************************/
/* SURFACE OBJECT                                                       */
/************************************************************************/

BOOL INTERNAL_CALL
DDSURF_Cleanup(PVOID pDDSurf)
{
	/* FIXME: implement 
	 * PDD_SURFACE pDDSurf = PVOID pDDSurf
	 */
#ifdef DX_DEBUG
    DPRINT1("DDSURF_Cleanup\n");
#endif
	return TRUE;
}

HANDLE STDCALL NtGdiDdCreateSurfaceObject(
    HANDLE hDirectDrawLocal,
    HANDLE hSurface,
    PDD_SURFACE_LOCAL puSurfaceLocal,
    PDD_SURFACE_MORE puSurfaceMore,
    PDD_SURFACE_GLOBAL puSurfaceGlobal,
    BOOL bComplete
)
{
	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
    PDD_SURFACE pSurface;
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdCreateSurfaceObject\n");
#endif
	if (!pDirectDraw)
		return NULL;

	if (!hSurface)
		hSurface = GDIOBJ_AllocObj(DdHandleTable, GDI_OBJECT_TYPE_DD_SURFACE);

	pSurface = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DD_SURFACE);
        /* FIXME - Handle pSurface == NULL!!!! */

	RtlMoveMemory(&pSurface->Local, puSurfaceLocal, sizeof(DD_SURFACE_LOCAL));
	RtlMoveMemory(&pSurface->More, puSurfaceMore, sizeof(DD_SURFACE_MORE));
	RtlMoveMemory(&pSurface->Global, puSurfaceGlobal, sizeof(DD_SURFACE_GLOBAL));
	pSurface->Local.lpGbl = &pSurface->Global;
	pSurface->Local.lpSurfMore = &pSurface->More;
	pSurface->Local.lpAttachList = NULL;
	pSurface->Local.lpAttachListFrom = NULL;
	pSurface->More.lpVideoPort = NULL;
	// FIXME: figure out how to use this
	pSurface->bComplete = bComplete;

	GDIOBJ_UnlockObjByPtr(DdHandleTable, pSurface);
	GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);

	return hSurface;
}

BOOL STDCALL NtGdiDdDeleteSurfaceObject(
    HANDLE hSurface
)
{
#ifdef DX_DEBUG
    DPRINT1("NtGdiDdDeleteSurfaceObject\n");
#endif
    /* FIXME add right GDI_OBJECT_TYPE_ for everthing for now 
       we are using same type */
	/* return GDIOBJ_FreeObj(hSurface, GDI_OBJECT_TYPE_DD_SURFACE); */
	return GDIOBJ_FreeObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DD_SURFACE);
	
}



/************************************************************************/
/* DIRECT DRAW SURFACR END                                                   */
/************************************************************************/


/*
BOOL STDCALL NtGdiDdAttachSurface(
    HANDLE hSurfaceFrom,
    HANDLE hSurfaceTo
)
{
	PDD_SURFACE pSurfaceFrom = GDIOBJ_LockObj(hSurfaceFrom, GDI_OBJECT_TYPE_DD_SURFACE);
	if (!pSurfaceFrom)
		return FALSE;
	PDD_SURFACE pSurfaceTo = GDIOBJ_LockObj(hSurfaceTo, GDI_OBJECT_TYPE_DD_SURFACE);
	if (!pSurfaceTo)
	{
		GDIOBJ_UnlockObjByPtr(pSurfaceFrom);
		return FALSE;
	}

	if (pSurfaceFrom->Local.lpAttachListFrom)
	{
		pSurfaceFrom->Local.lpAttachListFrom = pSurfaceFrom->AttachListFrom;
	}

	GDIOBJ_UnlockObjByPtr(pSurfaceFrom);
	GDIOBJ_UnlockObjByPtr(pSurfaceTo);
	return TRUE;
}
*/

DWORD STDCALL NtGdiDdGetAvailDriverMemory(
    HANDLE hDirectDrawLocal,
    PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData
)
{
	DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdGetAvailDriverMemory\n");
#endif

	/* backup the orignal PDev and info */
	lgpl = puGetAvailDriverMemoryData->lpDD;

	/* use our cache version instead */
	puGetAvailDriverMemoryData->lpDD = &pDirectDraw->Global;

	/* make the call */
   // ddRVal = pDirectDraw->DdGetAvailDriverMemory(puGetAvailDriverMemoryData); 
 
	GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);


	/* But back the orignal PDev */
	puGetAvailDriverMemoryData->lpDD = lgpl;

	return ddRVal;
}




DWORD STDCALL NtGdiDdSetExclusiveMode(
    HANDLE hDirectDraw,
    PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDraw, GDI_OBJECT_TYPE_DIRECTDRAW);

#ifdef DX_DEBUG
	DPRINT1("NtGdiDdSetExclusiveMode\n");
#endif

	/* backup the orignal PDev and info */
	lgpl = puSetExclusiveModeData->lpDD;

	/* use our cache version instead */
	puSetExclusiveModeData->lpDD = &pDirectDraw->Global;

	/* make the call */
    ddRVal = pDirectDraw->DdSetExclusiveMode(puSetExclusiveModeData);

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);

	/* But back the orignal PDev */
	puSetExclusiveModeData->lpDD = lgpl;
    
	return ddRVal;	
}




/* EOF */
