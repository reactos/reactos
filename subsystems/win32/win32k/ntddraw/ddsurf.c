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
#include <debug.h>

/************************************************************************/
/* NtGdiDdDestroySurface                                                */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdDestroySurface(HANDLE hSurface, BOOL bRealDestroy)
{
    PGD_DXDDDESTROYSURFACE pfnDdDestroySurface = (PGD_DXDDDESTROYSURFACE)gpDxFuncs[DXG_INDEX_DxDdDestroySurface].pfn;
    
    if (pfnDdDestroySurface == NULL)
    {
        DPRINT1("Warring no pfnDdDestroySurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdDestroySurface");
    return pfnDdDestroySurface(hSurface, bRealDestroy);
}

/************************************************************************/
/* NtGdiDdFlip                                                          */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdFlip(HANDLE hSurfaceCurrent,
            HANDLE hSurfaceTarget,
            HANDLE hSurfaceCurrentLeft,
            HANDLE hSurfaceTargetLeft,
            PDD_FLIPDATA puFlipData)
{
    PGD_DXDDFLIP pfnDdDdFlip = (PGD_DXDDFLIP)gpDxFuncs[DXG_INDEX_DxDdFlip].pfn;
   
    if (pfnDdDdFlip == NULL)
    {
        DPRINT1("Warring no pfnDdDdFlip");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdDdFlip");
    return pfnDdDdFlip(hSurfaceCurrent, hSurfaceTarget, hSurfaceCurrentLeft, hSurfaceTargetLeft, puFlipData);
}

/************************************************************************/
/* NtGdiDdUnlock                                                        */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdLock(HANDLE hSurface,
            PDD_LOCKDATA puLockData,
            HDC hdcClip)
{
    PGD_DXDDLOCK pfnDdLock = (PGD_DXDDLOCK)gpDxFuncs[DXG_INDEX_DxDdLock].pfn;
    
    if (pfnDdLock == NULL)
    {
        DPRINT1("Warring no pfnDdLock");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdLock");
    return pfnDdLock(hSurface, puLockData, hdcClip);
}

/************************************************************************/
/* NtGdiDdunlock                                                        */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdUnlock(HANDLE hSurface, 
              PDD_UNLOCKDATA puUnlockData)
{
    PGD_DXDDUNLOCK pfnDdUnlock = (PGD_DXDDUNLOCK)gpDxFuncs[DXG_INDEX_DxDdUnlock].pfn;
   
    if (pfnDdUnlock == NULL)
    {
        DPRINT1("Warring no pfnDdUnlock");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdUnlock");
    return pfnDdUnlock(hSurface, puUnlockData);
}

/************************************************************************/
/* NtGdiDdBlt                                                           */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdBlt(HANDLE hSurfaceDest,
           HANDLE hSurfaceSrc,
           PDD_BLTDATA puBltData)
{
    PGD_DDBLT pfnDdBlt = (PGD_DDBLT)gpDxFuncs[DXG_INDEX_DxDdBlt].pfn;
    
    if (pfnDdBlt == NULL)
    {
        DPRINT1("Warring no pfnDdBlt");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdBlt");
    return pfnDdBlt(hSurfaceDest,hSurfaceSrc,puBltData);
}

/************************************************************************/
/* NtGdiDdSetColorKey                                                   */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdSetColorKey(HANDLE hSurface,
                   PDD_SETCOLORKEYDATA puSetColorKeyData)
{
    PGD_DXDDSETCOLORKEY pfnDdSetColorKey = (PGD_DXDDSETCOLORKEY)gpDxFuncs[DXG_INDEX_DxDdSetColorKey].pfn;
    
    if (pfnDdSetColorKey == NULL)
    {
        DPRINT1("Warring no pfnDdSetColorKey");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdSetColorKey");
    return pfnDdSetColorKey(hSurface,puSetColorKeyData);

}

/************************************************************************/
/* NtGdiDdAddAttachedSurface                                            */
/************************************************************************/

DWORD
APIENTRY
NtGdiDdAddAttachedSurface(HANDLE hSurface,
                          HANDLE hSurfaceAttached,
                          PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData)
{
    PGD_DDADDATTACHEDSURFACE pfnDdAddAttachedSurface = (PGD_DDADDATTACHEDSURFACE)gpDxFuncs[DXG_INDEX_DxDdAddAttachedSurface].pfn;
    
    if (pfnDdAddAttachedSurface == NULL)
    {
        DPRINT1("Warring no pfnDdAddAttachedSurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdAddAttachedSurface");
    return pfnDdAddAttachedSurface(hSurface,hSurfaceAttached,puAddAttachedSurfaceData);
}

/************************************************************************/
/* NtGdiDdGetBltStatus                                                  */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdGetBltStatus(HANDLE hSurface,
                    PDD_GETBLTSTATUSDATA puGetBltStatusData)
{
    PGD_DXDDGETBLTSTATUS pfnDdGetBltStatus = (PGD_DXDDGETBLTSTATUS)gpDxFuncs[DXG_INDEX_DxDdGetBltStatus].pfn;
    
    if (pfnDdGetBltStatus == NULL)
    {
        DPRINT1("Warring no pfnDdGetBltStatus");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetBltStatus");
    return pfnDdGetBltStatus(hSurface,puGetBltStatusData);
}

/************************************************************************/
/* NtGdiDdGetFlipStatus                                                 */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdGetFlipStatus(HANDLE hSurface,
                     PDD_GETFLIPSTATUSDATA puGetFlipStatusData)
{
    PGD_DXDDGETFLIPSTATUS pfnDdGetFlipStatus = (PGD_DXDDGETFLIPSTATUS)gpDxFuncs[DXG_INDEX_DxDdGetFlipStatus].pfn;
    
    if (pfnDdGetFlipStatus == NULL)
    {
        DPRINT1("Warring no pfnDdGetFlipStatus");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetFlipStatus");
    return pfnDdGetFlipStatus(hSurface,puGetFlipStatusData);
}

/************************************************************************/
/* NtGdiDdUpdateOverlay                                                 */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdUpdateOverlay(HANDLE hSurfaceDestination,
                     HANDLE hSurfaceSource,
                     PDD_UPDATEOVERLAYDATA puUpdateOverlayData)
{
    PGD_DXDDUPDATEOVERLAY pfnDdUpdateOverlay = (PGD_DXDDUPDATEOVERLAY)gpDxFuncs[DXG_INDEX_DxDdUpdateOverlay].pfn;
   
    if (pfnDdUpdateOverlay == NULL)
    {
        DPRINT1("Warring no pfnDdUpdateOverlay");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdUpdateOverlay");
    return pfnDdUpdateOverlay(hSurfaceDestination,hSurfaceSource,puUpdateOverlayData);
}

/************************************************************************/
/* NtGdiDdSetOverlayPosition                                            */
/************************************************************************/

DWORD
APIENTRY
NtGdiDdSetOverlayPosition(HANDLE hSurfaceSource,
                          HANDLE hSurfaceDestination,
                          PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData)
{
    PGD_DXDDSETOVERLAYPOSITION pfnDdSetOverlayPosition = (PGD_DXDDSETOVERLAYPOSITION)gpDxFuncs[DXG_INDEX_DxDdSetOverlayPosition].pfn;
  
    if (pfnDdSetOverlayPosition == NULL)
    {
        DPRINT1("Warring no pfnDdSetOverlayPosition");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdSetOverlayPosition");
    return pfnDdSetOverlayPosition(hSurfaceSource,hSurfaceDestination,puSetOverlayPositionData);
}

/************************************************************************/
/* This is not part of the ddsurface interface but it have              */
/* deal with the surface                                                */
/************************************************************************/


/************************************************************************/
/* NtGdiDdAlphaBlt                                                      */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdAlphaBlt(HANDLE hSurfaceDest,
                HANDLE hSurfaceSrc,
                PDD_BLTDATA puBltData)
{
    PGD_DDALPHABLT pfnDdAlphaBlt = (PGD_DDALPHABLT)gpDxFuncs[DXG_INDEX_DxDdAlphaBlt].pfn;
   
    if (pfnDdAlphaBlt == NULL)
    {
        DPRINT1("Warring no pfnDdAlphaBlt");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdAlphaBlt");
    return pfnDdAlphaBlt(hSurfaceDest,hSurfaceSrc,puBltData);
}

/************************************************************************/
/* NtGdiDdAttachSurface                                                 */
/************************************************************************/
BOOL
APIENTRY
NtGdiDdAttachSurface(HANDLE hSurfaceFrom,
                     HANDLE hSurfaceTo
)
{
    PGD_DDATTACHSURFACE pfnDdAttachSurface = (PGD_DDATTACHSURFACE)gpDxFuncs[DXG_INDEX_DxDdAttachSurface].pfn;
  
    if (pfnDdAttachSurface == NULL)
    {
        DPRINT1("Warring no pfnDdAttachSurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdAttachSurface");
    return pfnDdAttachSurface(hSurfaceFrom,hSurfaceTo);
}

/************************************************************************/
/* NtGdiDdUnattachSurface                                               */
/************************************************************************/
/* Note : msdn protypes is VOID APIENTRY NtGdiDdUnattachSurface(HANDLE hSurface, HANDLE hSurfaceAttached)
          But it say it return either DDHAL_DRIVER_NOTHANDLED or DDHAL_DRIVER_HANDLED
          so I guess it is a typo in MSDN for this protypes for the info talk against it self
*/
DWORD
APIENTRY
NtGdiDdUnattachSurface(HANDLE hSurface,
                       HANDLE hSurfaceAttached)
{
    PGD_DXDDUNATTACHSURFACE pfnDdUnattachSurface = (PGD_DXDDUNATTACHSURFACE)gpDxFuncs[DXG_INDEX_DxDdUnattachSurface].pfn;  
    if (pfnDdUnattachSurface == NULL)
    {
        DPRINT1("Warring no pfnDdUnattachSurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdUnattachSurface");
    return pfnDdUnattachSurface(hSurface,hSurfaceAttached);
}


