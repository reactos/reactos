/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             win32ss/reactx/ntddraw/ddsurf.c
 * PROGRAMER:        Magnus Olsen (greatlord@reactos.org)
 * REVISION HISTORY:
 *       19/7-2006  Magnus Olsen
 */

#include <win32k.h>

// #define NDEBUG
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
        DPRINT1("Warning: no pfnDdDestroySurface\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdDestroySurface\n");
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
        DPRINT1("Warning: no pfnDdDdFlip\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdDdFlip\n");
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
        DPRINT1("Warning: no pfnDdLock\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdLock\n");
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
        DPRINT1("Warning: no pfnDdUnlock\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdUnlock\n");
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
        DPRINT1("Warning: no pfnDdBlt\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdBlt\n");
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
        DPRINT1("Warning: no pfnDdSetColorKey\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdSetColorKey\n");
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
        DPRINT1("Warning: no pfnDdAddAttachedSurface\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdAddAttachedSurface\n");
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
        DPRINT1("Warning: no pfnDdGetBltStatus\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdGetBltStatus\n");
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
        DPRINT1("Warning: no pfnDdGetFlipStatus\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdGetFlipStatus\n");
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
        DPRINT1("Warning: no pfnDdUpdateOverlay\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdUpdateOverlay\n");
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
        DPRINT1("Warning: no pfnDdSetOverlayPosition\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdSetOverlayPosition\n");
    return pfnDdSetOverlayPosition(hSurfaceSource,hSurfaceDestination,puSetOverlayPositionData);
}

/************************************************************************/
/* This is not part of the ddsurface interface but it                   */
/* deals with the surface                                               */
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
        DPRINT1("Warning: no pfnDdAlphaBlt\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdAlphaBlt\n");
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
        DPRINT1("Warning: no pfnDdAttachSurface\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDdAttachSurface\n");
    return pfnDdAttachSurface(hSurfaceFrom,hSurfaceTo);
}

/************************************************************************/
/* NtGdiDdUnattachSurface                                               */
/************************************************************************/
/* Note:  MSDN protypes is VOID APIENTRY NtGdiDdUnattachSurface(HANDLE hSurface, HANDLE hSurfaceAttached)
          But it say it return either DDHAL_DRIVER_NOTHANDLED or DDHAL_DRIVER_HANDLED
          so I guess it is a typo in MSDN for this prototype for the info contradicts itself.
*/
NTSTATUS
APIENTRY
NtGdiDdUnattachSurface(HANDLE hSurface,
                       HANDLE hSurfaceAttached)
{
    PGD_DXDDUNATTACHSURFACE pfnDdUnattachSurface = (PGD_DXDDUNATTACHSURFACE)gpDxFuncs[DXG_INDEX_DxDdUnattachSurface].pfn;
    if (pfnDdUnattachSurface == NULL)
    {
        DPRINT1("Warning: no pfnDdUnattachSurface\n");
        //return DDHAL_DRIVER_NOTHANDLED;
        return STATUS_NOT_IMPLEMENTED;
    }

    DPRINT("Calling dxg.sys pfnDdUnattachSurface\n");
    return pfnDdUnattachSurface(hSurface,hSurfaceAttached);
}
