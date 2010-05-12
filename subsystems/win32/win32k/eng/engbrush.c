/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Brush Functions
 * FILE:              subsystem/win32/win32k/eng/engbrush.c
 * PROGRAMER:         Jason Filby
 *                    Timo Kreuzer
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/** Internal functions ********************************************************/

VOID
NTAPI
EBRUSHOBJ_vInit(EBRUSHOBJ *pebo, PBRUSH pbrush, PDC pdc)
{
    HPALETTE hpal = NULL;

    ASSERT(pebo);
    ASSERT(pbrush);
    ASSERT(pdc);

    pebo->BrushObject.flColorType = 0;
    pebo->BrushObject.pvRbrush = NULL;
    pebo->pbrush = pbrush;
    pebo->pengbrush = NULL;
    pebo->flattrs = pbrush->flAttrs;

    /* Initialize 1 bpp fore and back colors */
    pebo->crCurrentBack = pdc->pdcattr->crBackgroundClr;
    pebo->crCurrentText = pdc->pdcattr->crForegroundClr;

    pebo->psurfTrg = pdc->dclevel.pSurface;
//    ASSERT(pebo->psurfTrg); // FIXME: some dcs don't have a surface

    if (pebo->psurfTrg)
        hpal = pebo->psurfTrg->hDIBPalette;
    if (!hpal) hpal = pPrimarySurface->devinfo.hpalDefault;
    pebo->ppalSurf = PALETTE_ShareLockPalette(hpal);
    if (!pebo->ppalSurf)
        pebo->ppalSurf = &gpalRGB;

    if (pbrush->flAttrs & GDIBRUSH_IS_NULL)
    {
        /* NULL brushes don't need a color */
        pebo->BrushObject.iSolidColor = 0;
    }
    else if (pbrush->flAttrs & GDIBRUSH_IS_SOLID)
    {
        /* Set the RGB color */
        EBRUSHOBJ_vSetSolidBrushColor(pebo, pbrush->BrushAttr.lbColor);
    }
    else
    {
        /* This is a pattern brush that needs realization */
        pebo->BrushObject.iSolidColor = 0xFFFFFFFF;

        /* Use foreground color of hatch brushes */
        if (pbrush->flAttrs & GDIBRUSH_IS_HATCH)
            pebo->crCurrentText = pbrush->BrushAttr.lbColor;
    }
}

VOID
FASTCALL
EBRUSHOBJ_vSetSolidBrushColor(EBRUSHOBJ *pebo, COLORREF crColor)
{
    ULONG iSolidColor;
    EXLATEOBJ exlo;

    /* Never use with non-solid brushes */
    ASSERT(pebo->flattrs & GDIBRUSH_IS_SOLID);

    /* Set the RGB color */
    pebo->crRealize = crColor;
    pebo->ulRGBColor = crColor;

    /* Initialize an XLATEOBJ RGB -> surface */
    EXLATEOBJ_vInitialize(&exlo, &gpalRGB, pebo->ppalSurf, 0, 0, 0);

    /* Translate the brush color to the target format */
    iSolidColor = XLATEOBJ_iXlate(&exlo.xlo, crColor);
    pebo->BrushObject.iSolidColor = iSolidColor;

    /* Clean up the XLATEOBJ */
    EXLATEOBJ_vCleanup(&exlo);
}

VOID
NTAPI
EBRUSHOBJ_vCleanup(EBRUSHOBJ *pebo)
{
    /* Check if there's a GDI realisation */
    if (pebo->pengbrush)
    {
        EngDeleteSurface(pebo->pengbrush);
        pebo->pengbrush = NULL;
    }

    /* Check if there's a driver's realisation */
    if (pebo->BrushObject.pvRbrush)
    {
        /* Free allocated driver memory */
        EngFreeMem(pebo->BrushObject.pvRbrush);
        pebo->BrushObject.pvRbrush = NULL;
    }

    if (pebo->ppalSurf != &gpalRGB)
        PALETTE_ShareUnlockPalette(pebo->ppalSurf);
}

VOID
NTAPI
EBRUSHOBJ_vUpdate(EBRUSHOBJ *pebo, PBRUSH pbrush, PDC pdc)
{
    /* Cleanup the brush */
    EBRUSHOBJ_vCleanup(pebo);

    /* Reinitialize */
    EBRUSHOBJ_vInit(pebo, pbrush, pdc);
}

/**
 * This function is not exported, because it makes no sense for
 * The driver to punt back to this function */
BOOL
APIENTRY
EngRealizeBrush(
    BRUSHOBJ *pbo,
    SURFOBJ  *psoDst,
    SURFOBJ  *psoPattern,
    SURFOBJ  *psoMask,
    XLATEOBJ *pxlo,
    ULONG    iHatch)
{
    EBRUSHOBJ *pebo;
    HBITMAP hbmpRealize;
    SURFOBJ *psoRealize;
    POINTL ptlSrc = {0, 0};
    RECTL rclDest;
    ULONG lWidth;

    /* Calculate width in bytes of the realized brush */
    lWidth = DIB_GetDIBWidthBytes(psoPattern->sizlBitmap.cx,
                                  BitsPerFormat(psoDst->iBitmapFormat));

    /* Allocate a bitmap */
    hbmpRealize = EngCreateBitmap(psoPattern->sizlBitmap,
                                  lWidth,
                                  psoDst->iBitmapFormat,
                                  BMF_NOZEROINIT,
                                  NULL);
    if (!hbmpRealize)
    {
        return FALSE;
    }

    /* Lock the bitmap */
    psoRealize = EngLockSurface(hbmpRealize);
    if (!psoRealize)
    {
        EngDeleteSurface(hbmpRealize);
        return FALSE;
    }

    /* Copy the bits to the new format bitmap */
    rclDest.left = rclDest.top = 0;
    rclDest.right = psoPattern->sizlBitmap.cx;
    rclDest.bottom = psoPattern->sizlBitmap.cy;
    EngCopyBits(psoRealize, psoPattern, NULL, pxlo, &rclDest, &ptlSrc);

    /* Unlock the bitmap again */
    EngUnlockSurface(psoRealize);

    pebo = CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject);
    pebo->pengbrush = (PVOID)hbmpRealize;

    return TRUE;
}

BOOL
NTAPI
EBRUSHOBJ_bRealizeBrush(EBRUSHOBJ *pebo, BOOL bCallDriver)
{
    BOOL bResult;
    PFN_DrvRealizeBrush pfnRealzizeBrush = NULL;
    PSURFACE psurfPattern, psurfMask;
    PPDEVOBJ ppdev = NULL;
    EXLATEOBJ exlo;

    // FIXME: all EBRUSHOBJs need a surface, see EBRUSHOBJ_vInit
    if (!pebo->psurfTrg)
    {
        DPRINT1("Pattern brush has no target surface!\n");
        return FALSE;
    }

    ppdev = (PPDEVOBJ)pebo->psurfTrg->SurfObj.hdev;

    // FIXME: all SURFACEs need a PDEV
    if (ppdev && bCallDriver)
        pfnRealzizeBrush = ppdev->DriverFunctions.RealizeBrush;

    if (!pfnRealzizeBrush)
        pfnRealzizeBrush = EngRealizeBrush;

    psurfPattern = SURFACE_ShareLockSurface(pebo->pbrush->hbmPattern);
    ASSERT(psurfPattern);

    /* FIXME: implement mask */
    psurfMask = NULL;

    /* Initialize XLATEOBJ for the brush */
    EXLATEOBJ_vInitBrushXlate(&exlo,
                              pebo->pbrush,
                              pebo->psurfTrg,
                              pebo->crCurrentText,
                              pebo->crCurrentBack);

    /* Create the realization */
    bResult = pfnRealzizeBrush(&pebo->BrushObject,
                               &pebo->psurfTrg->SurfObj,
                               &psurfPattern->SurfObj,
                               psurfMask ? &psurfMask->SurfObj : NULL,
                               &exlo.xlo,
                               -1); // FIXME: what about hatch brushes?

    /* Cleanup the XLATEOBJ */
    EXLATEOBJ_vCleanup(&exlo);

    /* Unlock surfaces */
    if (psurfPattern)
        SURFACE_ShareUnlockSurface(psurfPattern);
    if (psurfMask)
        SURFACE_ShareUnlockSurface(psurfMask);

    return bResult;
}

PVOID
NTAPI
EBRUSHOBJ_pvGetEngBrush(EBRUSHOBJ *pebo)
{
    BOOL bResult;

    if (!pebo->pengbrush)
    {
        bResult = EBRUSHOBJ_bRealizeBrush(pebo, FALSE);
        if (!bResult)
        {
            if (pebo->pengbrush)
                EngDeleteSurface(pebo->pengbrush);
            pebo->pengbrush = NULL;
        }
    }

    return pebo->pengbrush;
}


/** Exported DDI functions ****************************************************/

/*
 * @implemented
 */
PVOID APIENTRY
BRUSHOBJ_pvAllocRbrush(
    IN BRUSHOBJ *pbo,
    IN ULONG cj)
{
    pbo->pvRbrush = EngAllocMem(0, cj, 'rbdG');
    return pbo->pvRbrush;
}

/*
 * @implemented
 */
PVOID APIENTRY
BRUSHOBJ_pvGetRbrush(
    IN BRUSHOBJ *pbo)
{
    EBRUSHOBJ *pebo = CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject);
    BOOL bResult;

    if (!pbo->pvRbrush)
    {
        bResult = EBRUSHOBJ_bRealizeBrush(pebo, TRUE);
        if (!bResult)
        {
            if (pbo->pvRbrush)
            {
                EngFreeMem(pbo->pvRbrush);
                pbo->pvRbrush = NULL;
            }
        }
    }

    return pbo->pvRbrush;
}

/*
 * @implemented
 */
ULONG APIENTRY
BRUSHOBJ_ulGetBrushColor(
    IN BRUSHOBJ *pbo)
{
    EBRUSHOBJ *pebo = CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject);
    return pebo->ulRGBColor;
}

/* EOF */
