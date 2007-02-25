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

