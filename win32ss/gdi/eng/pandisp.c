/*
 * PROJECT:     ReactOS Win32k subsystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Pan-Display driver
 * COPYRIGHT:   Copyright 2022 Hervé Poussineau <hpoussin@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

#define TAG_PAN 'DnaP'
#define GETPFN(name) ((PFN_Drv##name)(pandev->apfn[INDEX_Drv##name]))

static DHPDEV gPan; /* FIXME: remove once PanSynchronize() is called periodically */

typedef struct _PANDEV
{
    ULONG iBitmapFormat;
    HDEV hdev;
    SIZEL szlDesktop; /* Size of the whole desktop */
    RECTL rclViewport; /* Viewport area */
    HSURF hsurf; /* Global surface */
    HSURF hsurfShadow; /* Our shadow surface, used for drawing */
    SURFOBJ *psoShadow;

    /* Underlying PDEVOBJ */
    ULONG flUnderlyingGraphicsCaps;
    SIZEL szlScreen;
    DHPDEV dhpdevScreen;
    BOOL enabledScreen; /* underlying surface enabled */
    SURFOBJ *surfObjScreen;
    PFN apfn[INDEX_LAST]; /* Functions of underlying PDEVOBJ */
} PANDEV, *PPANDEV;

BOOL
APIENTRY
PanSynchronize(
    _In_ DHPDEV dhpdev,
    _In_ PRECTL prcl)
{
    PPANDEV pandev = (PPANDEV)dhpdev;
    RECTL rclDest;
    POINTL ptlSrc;

    /* FIXME: for now, copy the whole shadow buffer. We might try to optimize that */

    ptlSrc.x = pandev->rclViewport.left;
    ptlSrc.y = pandev->rclViewport.top;

    rclDest.top = rclDest.left = 0;
    rclDest.bottom = pandev->szlScreen.cy;
    rclDest.right = pandev->szlScreen.cx;

    return EngCopyBits(pandev->surfObjScreen, pandev->psoShadow, NULL, NULL, &rclDest, &ptlSrc);
}

DHPDEV
APIENTRY
PanEnablePDEV(
    _In_ PDEVMODEW pdm,
    _In_ LPWSTR pwszLogAddress,
    _In_ ULONG cPat,
    _In_reads_opt_(cPat) HSURF *phsurfPatterns,
    _In_ ULONG cjCaps,
    _Out_writes_bytes_(cjCaps) PULONG pdevcaps,
    _In_ ULONG cjDevInfo,
    _Out_writes_bytes_(cjDevInfo) PDEVINFO pdi,
    _In_ HDEV hdev,
    _In_ LPWSTR pwszDeviceName,
    _In_ HANDLE hDriver)
{
    PPANDEV pandev;
    PPDEVOBJ ppdev = (PPDEVOBJ)hdev;
    DEVMODEW underlyingDevmode;
    PGDIINFO pGdiInfo = (PGDIINFO)pdevcaps;

    DPRINT("PanEnablePDEV(ppdev %p %dx%dx%d -> %dx%dx%d %d Hz)\n",
           ppdev,
           pdm->dmPelsWidth, pdm->dmPelsHeight, pdm->dmBitsPerPel,
           pdm->dmPanningWidth, pdm->dmPanningHeight, pdm->dmBitsPerPel, pdm->dmDisplayFrequency);

    ASSERT(pdm->dmPanningWidth <= pdm->dmPelsWidth && pdm->dmPanningHeight <= pdm->dmPelsHeight);

    /* Allocate PANDEV */
    pandev = EngAllocMem(FL_ZERO_MEMORY, sizeof(PANDEV), TAG_PAN);
    if (!pandev)
    {
        DPRINT1("Failed to allocate memory\n");
        return NULL;
    }

    /* Copy device pointers to our structure (do not copy ppdev->apfn,
     * as it contains our local pointers!) */
    RtlCopyMemory(pandev->apfn, ppdev->pldev->apfn, sizeof(pandev->apfn));

    /* Enable underlying PDEV */
    underlyingDevmode = *pdm;
    underlyingDevmode.dmPelsWidth = pdm->dmPanningWidth;
    underlyingDevmode.dmPelsHeight = pdm->dmPanningHeight;
    pandev->dhpdevScreen = GETPFN(EnablePDEV)(&underlyingDevmode,
                                              pwszLogAddress,
                                              cPat,
                                              phsurfPatterns,
                                              cjCaps,
                                              pdevcaps,
                                              cjDevInfo,
                                              pdi,
                                              hdev,
                                              pwszDeviceName,
                                              hDriver);
    if (!pandev->dhpdevScreen)
    {
        DPRINT1("Failed to enable underlying PDEV\n");
        EngFreeMem(pandev);
        return NULL;
    }

    pandev->iBitmapFormat = pdi->iDitherFormat;
    pandev->szlDesktop.cx = pdm->dmPelsWidth;
    pandev->szlDesktop.cy = pdm->dmPelsHeight;
    pandev->szlScreen.cx = pdm->dmPanningWidth;
    pandev->szlScreen.cy = pdm->dmPanningHeight;
    pandev->flUnderlyingGraphicsCaps = pdi->flGraphicsCaps;

    /* Upgrade some capabilities */
    pGdiInfo->ulHorzRes = pdm->dmPelsWidth;
    pGdiInfo->ulVertRes = pdm->dmPelsHeight;
    pdi->flGraphicsCaps |= GCAPS_PANNING;
    pdi->flGraphicsCaps2 = GCAPS2_SYNCFLUSH | GCAPS2_SYNCTIMER;

    gPan = (DHPDEV)pandev;
    return (DHPDEV)pandev;
}

VOID
APIENTRY
PanCompletePDEV(
    _In_ DHPDEV dhpdev,
    _In_ HDEV hdev)
{
    PPANDEV pandev = (PPANDEV)dhpdev;

    DPRINT("PanCompletePDEV(%p %p)\n", dhpdev, hdev);

    pandev->hdev = hdev;
    GETPFN(CompletePDEV)(pandev->dhpdevScreen, hdev);
}

VOID
APIENTRY
PanDisablePDEV(
    _In_ DHPDEV dhpdev)
{
    PPANDEV pandev = (PPANDEV)dhpdev;

    DPRINT("PanDisablePDEV(%p)\n", dhpdev);

    GETPFN(DisablePDEV)(pandev->dhpdevScreen);
    EngFreeMem(pandev);
}

VOID
APIENTRY
PanDisableSurface(
    _In_ DHPDEV dhpdev)
{
    PPANDEV pandev = (PPANDEV)dhpdev;

    DPRINT("PanDisableSurface(%p)\n", dhpdev);

    /* Note that we must handle a not fully initialized pandev, as this
     * function is also called from PanEnableSurface in case of error. */

    if (pandev->psoShadow)
    {
        EngUnlockSurface(pandev->psoShadow);
        pandev->psoShadow = NULL;
    }
    if (pandev->hsurfShadow)
    {
        EngDeleteSurface(pandev->hsurfShadow);
        pandev->hsurfShadow = NULL;
    }
    if (pandev->hsurf)
    {
        EngDeleteSurface(pandev->hsurf);
        pandev->hsurf = NULL;
    }
    if (pandev->surfObjScreen)
    {
        EngUnlockSurface(pandev->surfObjScreen);
        pandev->surfObjScreen = NULL;
    }
    if (pandev->enabledScreen)
    {
        GETPFN(DisableSurface)(pandev->dhpdevScreen);
        pandev->enabledScreen = FALSE;
    }
}

HSURF
APIENTRY
PanEnableSurface(
    _In_ DHPDEV dhpdev)
{
    PPANDEV pandev = (PPANDEV)dhpdev;
    HSURF surfScreen;

    /* Move viewport to center of screen */
    pandev->rclViewport.left = (pandev->szlDesktop.cx - pandev->szlScreen.cx) / 2;
    pandev->rclViewport.top = (pandev->szlDesktop.cy - pandev->szlScreen.cy) / 2;
    pandev->rclViewport.right = pandev->rclViewport.left + pandev->szlScreen.cx;
    pandev->rclViewport.bottom  = pandev->rclViewport.top + pandev->szlScreen.cy;

    /* Enable underlying device */
    surfScreen = GETPFN(EnableSurface)(pandev->dhpdevScreen);
    if (!surfScreen)
    {
        DPRINT1("Failed to enable underlying surface\n");
        goto failure;
    }
    pandev->enabledScreen = TRUE;

    pandev->surfObjScreen = EngLockSurface(surfScreen);
    if (!pandev->surfObjScreen)
    {
        DPRINT1("Failed to lock underlying surface\n");
        goto failure;
    }

    /* Create device surface */
    pandev->hsurf = EngCreateDeviceSurface(NULL, pandev->szlDesktop, pandev->iBitmapFormat);
    if (!pandev->hsurf)
    {
        DPRINT1("EngCreateDeviceSurface() failed\n");
        goto failure;
    }

    if (!EngAssociateSurface(pandev->hsurf, pandev->hdev, HOOK_ALPHABLEND |
                                                          HOOK_BITBLT |
                                                          HOOK_COPYBITS |
                                                          HOOK_GRADIENTFILL |
                                                          HOOK_STROKEPATH |
                                                          HOOK_SYNCHRONIZE |
                                                          HOOK_TEXTOUT))
    {
        DPRINT1("EngAssociateSurface() failed\n");
        goto failure;
    }

    /* Create shadow surface */
    pandev->hsurfShadow = (HSURF)EngCreateBitmap(pandev->szlDesktop, pandev->szlDesktop.cx, pandev->iBitmapFormat, BMF_TOPDOWN, NULL);
    if (!pandev->hsurfShadow)
    {
        DPRINT1("EngCreateBitmap() failed\n");
        goto failure;
    }

    pandev->psoShadow = EngLockSurface(pandev->hsurfShadow);
    if (!pandev->psoShadow)
    {
        DPRINT1("EngLockSurface() failed\n");
        goto failure;
    }

    return pandev->hsurf;

failure:
    PanDisableSurface(dhpdev);
    return NULL;
}

BOOL
APIENTRY
PanBitBlt(
    _Inout_ SURFOBJ *psoTrg,
    _In_opt_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ PRECTL prclTrg,
    _In_opt_ PPOINTL pptlSrc,
    _In_opt_ PPOINTL pptlMask,
    _In_opt_ BRUSHOBJ *pbo,
    _In_opt_ PPOINTL pptlBrush,
    _In_ ROP4 rop4)
{
    PPANDEV pandev;
    BOOL res;

    if (psoTrg->iType == STYPE_DEVICE)
    {
        pandev = (PPANDEV)psoTrg->dhpdev;
        psoTrg = pandev->psoShadow;
    }
    if (psoSrc && psoSrc->iType == STYPE_DEVICE)
    {
        pandev = (PPANDEV)psoSrc->dhpdev;
        psoSrc = pandev->psoShadow;
    }

    res = EngBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo, prclTrg, pptlSrc, pptlMask, pbo, pptlBrush, rop4);

    /* FIXME: PanSynchronize must be called periodically, as we have the GCAPS2_SYNCTIMER flag.
     * That's not the case, so call PanSynchronize() manually */
    if (res)
        PanSynchronize(gPan, NULL);

    return res;
}

BOOL
APIENTRY
PanCopyBits(
    _Inout_ SURFOBJ *psoDest,
    _In_opt_ SURFOBJ *psoSrc,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ PRECTL prclDest,
    _In_opt_ PPOINTL pptlSrc)
{
    /* Easy. Just call the more general function PanBitBlt */
    return PanBitBlt(psoDest, psoSrc, NULL, pco, pxlo, prclDest, pptlSrc, NULL, NULL, NULL, ROP4_SRCCOPY);
}

BOOL
APIENTRY
PanStrokePath(
     _Inout_ SURFOBJ *pso,
     _In_ PATHOBJ *ppo,
     _In_ CLIPOBJ *pco,
     _In_opt_ XFORMOBJ *pxo,
     _In_ BRUSHOBJ *pbo,
     _In_ PPOINTL pptlBrushOrg,
     _In_ PLINEATTRS plineattrs,
     _In_ MIX mix)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return FALSE;
}

BOOL
APIENTRY
PanTextOut(
    _Inout_ SURFOBJ *pso,
    _In_ STROBJ *pstro,
    _In_ FONTOBJ *pfo,
    _In_ CLIPOBJ *pco,
    _In_opt_ PRECTL prclExtra,
    _In_opt_ PRECTL prclOpaque,
    _In_ BRUSHOBJ *pboFore,
    _In_ BRUSHOBJ *pboOpaque,
    _In_ PPOINTL pptlOrg,
    _In_ MIX mix)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return FALSE;
}

VOID
APIENTRY
PanMovePointer(
    _Inout_ SURFOBJ *pso,
    _In_ LONG x,
    _In_ LONG y,
    _In_ PRECTL prcl)
{
    PPANDEV pandev = (PPANDEV)pso->dhpdev;
    PFN_DrvMovePointer pfnMovePointer;
    BOOL bChanged = FALSE;

    DPRINT("PanMovePointer(%d, %d)\n", x, y);

    /* FIXME: MouseSafetyOnDrawStart/MouseSafetyOnDrawEnd like to call us to hide/show the cursor,
     * without real reason (see 5b0f6dcceeae3cf21f2535e2c523c0bf2749ea6b). Ignore those calls */
    if (x < 0 || y >= 0) return;

    /* We don't set DrvSetPointerShape, so we must not be called
     * with x < 0 (hide cursor) or y >= 0 (we have GCAPS_PANNING) */
    ASSERT(x >= 0 && y < 0);

    /* Call underlying MovePointer function */
    if (pandev->flUnderlyingGraphicsCaps & GCAPS_PANNING)
    {
        pfnMovePointer = GETPFN(MovePointer);
        if (pfnMovePointer)
            pfnMovePointer(pandev->surfObjScreen, x, y, prcl);
    }

    y += pso->sizlBitmap.cy;

    if (x < pandev->rclViewport.left)
    {
        pandev->rclViewport.left = x;
        pandev->rclViewport.right = x + pandev->szlScreen.cx;
        bChanged = TRUE;
    }
    else if (x >= pandev->rclViewport.right)
    {
        pandev->rclViewport.right = x + 1;
        pandev->rclViewport.left = x - pandev->szlScreen.cx;
        bChanged = TRUE;
    }
    if (y < pandev->rclViewport.top)
    {
        pandev->rclViewport.top = y;
        pandev->rclViewport.bottom = y + pandev->szlScreen.cy;
        bChanged = TRUE;
    }
    else if (y >= pandev->rclViewport.bottom)
    {
        pandev->rclViewport.bottom = y + 1;
        pandev->rclViewport.top = y - pandev->szlScreen.cy;
        bChanged = TRUE;
    }

    if (bChanged)
        PanSynchronize(pso->dhpdev, NULL);
}

BOOL
APIENTRY
PanAlphaBlend(
    _Inout_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSrc,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ PRECTL prclDest,
    _In_ PRECTL prclSrc,
    _In_ BLENDOBJ *pBlendObj)
{
    PPANDEV pandev = (PPANDEV)psoDest->dhpdev;
    BOOL res;

    res = EngAlphaBlend(pandev->psoShadow, psoSrc, pco, pxlo, prclDest, prclSrc, pBlendObj);

    /* FIXME: PanSynchronize must be called periodically, as we have the GCAPS2_SYNCTIMER flag.
     * That's not the case, so call PanSynchronize() manually */
    if (res)
        PanSynchronize(gPan, NULL);

    return res;
}

BOOL
APIENTRY
PanGradientFill(
    _Inout_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ PTRIVERTEX pVertex,
    _In_ ULONG nVertex,
    _In_ PVOID pMesh,
    _In_ ULONG nMesh,
    _In_ PRECTL prclExtents,
    _In_ PPOINTL pptlDitherOrg,
    _In_ ULONG ulMode)
{
    PPANDEV pandev = (PPANDEV)pso->dhpdev;
    BOOL res;

    res = EngGradientFill(pandev->psoShadow, pco, pxlo, pVertex, nVertex, pMesh, nMesh, prclExtents, pptlDitherOrg, ulMode);

    /* FIXME: PanSynchronize must be called periodically, as we have the GCAPS2_SYNCTIMER flag.
     * That's not the case, so call PanSynchronize() manually */
    if (res)
        PanSynchronize(gPan, NULL);

    return res;
}

DRVFN gPanDispDrvFn[] =
{
    /* required */
    { INDEX_DrvEnablePDEV,     (PFN) PanEnablePDEV     },
    { INDEX_DrvCompletePDEV,   (PFN) PanCompletePDEV   },
    { INDEX_DrvDisablePDEV,    (PFN) PanDisablePDEV    },
    { INDEX_DrvEnableSurface,  (PFN) PanEnableSurface  },
    { INDEX_DrvDisableSurface, (PFN) PanDisableSurface },

    /* required for device-managed surfaces */
    { INDEX_DrvCopyBits,       (PFN) PanCopyBits       },
    { INDEX_DrvStrokePath,     (PFN) PanStrokePath     },
    { INDEX_DrvTextOut,        (PFN) PanTextOut        },

    /* optional, but required for panning functionality */
    { INDEX_DrvSynchronize,    (PFN) PanSynchronize    },
    { INDEX_DrvMovePointer,    (PFN) PanMovePointer    },

    /* optional */
    { INDEX_DrvBitBlt,         (PFN) PanBitBlt         },
    { INDEX_DrvAlphaBlend,     (PFN) PanAlphaBlend     },
    { INDEX_DrvGradientFill,   (PFN) PanGradientFill   },
};

ULONG gPanDispDrvCount = RTL_NUMBER_OF(gPanDispDrvFn);
