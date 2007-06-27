/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/dd.c
 * PROGRAMER:        Magnus Olsen (greatlord@reactos.org)
 * REVISION HISTORY:
 *       19/7-2006  Magnus Olsen
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#define DdHandleTable GdiHandleTable



DWORD STDCALL NtGdiDdBlt(
    HANDLE hSurfaceDest,
    HANDLE hSurfaceSrc,
    PDD_BLTDATA puBltData
)
{
    NTSTATUS Status = FALSE;
    DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    DD_BLTDATA  Blt;
    PDD_SURFACE pDstSurface = NULL;
    PDD_SURFACE pSrcSurface = NULL;
    PDD_DIRECTDRAW pDirectDraw;

    DPRINT1("NtGdiDdBlt\n");

    _SEH_TRY
    {
        ProbeForRead(puBltData,  sizeof(DD_BLTDATA), 1);
        RtlCopyMemory( &Blt, puBltData, sizeof( DD_BLTDATA ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return ddRVal;
    }

    pDstSurface = GDIOBJ_LockObj(DdHandleTable, hSurfaceDest, GDI_OBJECT_TYPE_DD_SURFACE);
    if (!pDstSurface)
    {
        DPRINT1("Fail\n");
        return ddRVal;
    }

    pDirectDraw = GDIOBJ_LockObj(DdHandleTable, pDstSurface->hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
    if (!pDirectDraw)
    {
        DPRINT1("Fail\n");
        GDIOBJ_UnlockObjByPtr(DdHandleTable, pSrcSurface);
        return ddRVal;
    }

    if (pSrcSurface)
    {
        pSrcSurface = GDIOBJ_LockObj(DdHandleTable, pSrcSurface, GDI_OBJECT_TYPE_DD_SURFACE);
        if (!pSrcSurface)
        {
            DPRINT1("Fail\n");
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDstSurface);
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
            return ddRVal;
        }
    }

    Blt.lpDDDestSurface = &pDstSurface->Local;
    if (pDstSurface)
    {
        Blt.lpDDSrcSurface = &pDstSurface->Local;
    }

    Blt.ddRVal = DDERR_GENERIC;

    /* MSDN say this member is always set to FALSE in windows 2000 or higher */
    Blt.IsClipped = FALSE;

    /* MSDN say this member is always unuse in windows 2000 or higher */
    Blt.dwROPFlags = 0;

    if (pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_BLT)
    {
        Blt.lpDD = &pDirectDraw->Global;
        Blt.Blt = NULL;
        ddRVal = pDirectDraw->Surf.Blt(&Blt);
    }

    DPRINT1("Retun value is %04x and driver return code is %04x\n",ddRVal,Blt.ddRVal);

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDstSurface);

    if(pSrcSurface)
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, pSrcSurface);
    }

    _SEH_TRY
    {
        ProbeForWrite(puBltData,  sizeof(DD_BLTDATA), 1);
        puBltData->ddRVal = Blt.ddRVal;
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    DPRINT1("Retun value is %04x and driver return code is %04x\n",ddRVal,Blt.ddRVal);
    return ddRVal;
   }



/************************************************************************/
/* NtGdiDdCreateSurface                                                 */
/* status : untested                                                    */
/************************************************************************/
DWORD STDCALL NtGdiDdDestroySurface(
    HANDLE hSurface,
    BOOL bRealDestroy
)
{	
	DWORD  ddRVal  = DDHAL_DRIVER_NOTHANDLED;
	PDD_SURFACE pSurface;
	PDD_DIRECTDRAW pDirectDraw;	
	DD_DESTROYSURFACEDATA DestroySurf; 
	
	DPRINT1("NtGdiDdDestroySurface\n");
	
	pSurface = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DD_SURFACE);
	if (pSurface != NULL) 
	{		    
		pDirectDraw = GDIOBJ_LockObj(DdHandleTable, pSurface->hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
		if (pDirectDraw != NULL)
		{		
			if (pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_DESTROYSURFACE)			
			{								
				//DestroySurf.lpDD = pSurface->Global;
				DestroySurf.lpDDSurface = hSurface; 
                
				/*  FIXME
				    in parma bRealDestroy 
				    Specifies how to destroy the surface. Can be one of the following values. 
                    TRUE   =   Destroy the surface and free video memory.
                    FALSE  =   Free the video memory but leave the surface in an uninitialized state
                */

				DestroySurf.DestroySurface = pDirectDraw->Surf.DestroySurface;		
				ddRVal = pDirectDraw->Surf.DestroySurface(&DestroySurf); 
			}

			 GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
		}

		GDIOBJ_UnlockObjByPtr(DdHandleTable, pSurface);
	}

    return ddRVal;			
}



/************************************************************************/
/* NtGdiDdFlip                                                 */
/* status : untested                                                    */
/************************************************************************/

DWORD STDCALL NtGdiDdFlip(
    HANDLE hSurfaceCurrent,
    HANDLE hSurfaceTarget,
    HANDLE hSurfaceCurrentLeft,
    HANDLE hSurfaceTargetLeft,
    PDD_FLIPDATA puFlipData
)
{
	DWORD  ddRVal  = DDHAL_DRIVER_NOTHANDLED;
	PDD_SURFACE pSurface;
	PDD_DIRECTDRAW pDirectDraw;			
	DPRINT1("NtGdiDdFlip\n");
	
	/* DO we need looking all surface or is okay for one */
	pSurface = GDIOBJ_LockObj(DdHandleTable, hSurfaceCurrent, GDI_OBJECT_TYPE_DD_SURFACE);
	if (pSurface != NULL) 
	{				
		pDirectDraw = GDIOBJ_LockObj(DdHandleTable, pSurface->hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);

		if (pDirectDraw != NULL)
		{		
			if (pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_FLIP)			
			{		
				/* FIXME is lpDD typecasted tp driver PEV ?? */ 											
			    ddRVal = pDirectDraw->Surf.Flip(puFlipData);				
			}

			GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
		}

		GDIOBJ_UnlockObjByPtr(DdHandleTable, pSurface);
	}

    return ddRVal;				
}




/************************************************************************/
/* NtGdiDdLock                                                          */
/* status : untested                                                    */
/************************************************************************/

DWORD STDCALL NtGdiDdLock(
    HANDLE hSurface,
    PDD_LOCKDATA puLockData,
    HDC hdcClip)
{
	DWORD  ddRVal  = DDHAL_DRIVER_NOTHANDLED;
	PDD_SURFACE pSurface;
	PDD_DIRECTDRAW pDirectDraw;	
	
	DPRINT1("NtGdiDdLock\n");
		
	pSurface = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DD_SURFACE);
	if (pSurface != NULL) 
	{				
		pDirectDraw = GDIOBJ_LockObj(DdHandleTable, pSurface->hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);

		if (pDirectDraw != NULL)
		{		
			/* Do we need lock hdc from hdcClip ?? */
			if (pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_LOCK)
			{	
				/* FIXME is lpDD typecasted tp driver PEV ?? */ 			
				ddRVal = pDirectDraw->Surf.Lock(puLockData);				
			}

			GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
		}

		GDIOBJ_UnlockObjByPtr(DdHandleTable, pSurface);
	}

    return ddRVal;				
}

/*
00414         PDD_SURFCB_SETCLIPLIST        SetClipList;
00416         PDD_SURFCB_UNLOCK             Unlock;
00417         PDD_SURFCB_BLT                Blt;
00418         PDD_SURFCB_SETCOLORKEY        SetColorKey;
00419         PDD_SURFCB_ADDATTACHEDSURFACE AddAttachedSurface;
00420         PDD_SURFCB_GETBLTSTATUS       GetBltStatus;
00421         PDD_SURFCB_GETFLIPSTATUS      GetFlipStatus;
00422         PDD_SURFCB_UPDATEOVERLAY      UpdateOverlay;
00423         PDD_SURFCB_SETOVERLAYPOSITION SetOverlayPosition;
00424         PVOID                         reserved4;
00425         PDD_SURFCB_SETPALETTE         SetPalette;
00426 } DD_SURFACECALLBACKS, *PDD_SURFACECALLBACKS;
*/

