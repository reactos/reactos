/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Brush Functions
 * FILE:              subsystem/win32/win32k/eng/engbrush.c
 * PROGRAMER:         Jason Filby
 *                    Timo Kreuzer
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/** Internal functions ********************************************************/

VOID
NTAPI
EBRUSHOBJ_vInit(EBRUSHOBJ *pebo, PBRUSH pbrush, PDC pdc)
{
    ULONG iSolidColor;
    XLATEOBJ *pxlo;
    PSURFACE psurfTrg;

    ASSERT(pebo);
    ASSERT(pbrush);
    ASSERT(pdc);

    psurfTrg = pdc->dclevel.pSurface;

    pebo->psurfTrg = psurfTrg;
    pebo->BrushObject.flColorType = 0;
    pebo->pbrush = pbrush;
    pebo->flattrs = pbrush->flAttrs;
    pebo->crCurrentText = pdc->pdcattr->crForegroundClr;
    pebo->crCurrentBack = pdc->pdcattr->crBackgroundClr;
    pebo->BrushObject.pvRbrush = NULL;
    pebo->pengbrush = NULL;

    if (pbrush->flAttrs & GDIBRUSH_IS_NULL)
    {
        pebo->BrushObject.iSolidColor = 0;
    }
    else if (pbrush->flAttrs & GDIBRUSH_IS_SOLID)
    {
        /* Set the RGB color */
        pebo->crRealize = pbrush->BrushAttr.lbColor;
        pebo->ulRGBColor = pbrush->BrushAttr.lbColor;

        /* Translate the brush color to the target format */
        pxlo = IntCreateBrushXlate(pbrush, psurfTrg, pebo->crCurrentBack);
        iSolidColor = XLATEOBJ_iXlate(pxlo, pbrush->BrushAttr.lbColor);
        pebo->BrushObject.iSolidColor = iSolidColor;
        if (pxlo)
            EngDeleteXlate(pxlo);
    }
    else
    {
        /* This is a pattern brush that needs realization */
        pebo->BrushObject.iSolidColor = 0xFFFFFFFF;
    }
}

VOID
FASTCALL
EBRUSHOBJ_vSetSolidBrushColor(EBRUSHOBJ *pebo, COLORREF crColor, XLATEOBJ *pxlo)
{
    ULONG iSolidColor;

    /* Never use with non-solid brushes */
    ASSERT(pebo->flattrs & GDIBRUSH_IS_SOLID);

    /* Set the RGB color */
    pebo->crRealize = crColor;
    pebo->ulRGBColor = crColor;

    /* Translate the brush color to the target format */
    iSolidColor = XLATEOBJ_iXlate(pxlo, crColor);
    pebo->BrushObject.iSolidColor = iSolidColor;

    pebo->BrushObject.iSolidColor = iSolidColor;
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
    rclDest = (RECTL){0, 0, psoPattern->sizlBitmap.cx, psoPattern->sizlBitmap.cy};
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
    PSURFACE psurfTrg, psurfPattern, psurfMask;
    PPDEVOBJ ppdev = NULL;
    XLATEOBJ *pxlo;

    psurfTrg = pebo->psurfTrg; // FIXME: all EBRUSHOBJs need a surface
    if (!psurfTrg)
    {
        DPRINT1("Pattern brush has no target surface!\n");
        return FALSE;
    }

    ppdev = (PPDEVOBJ)psurfTrg->SurfObj.hdev; // FIXME: all SURFACEs need a PDEV
    if (ppdev && bCallDriver)
        pfnRealzizeBrush = ppdev->DriverFunctions.RealizeBrush;
    if (!pfnRealzizeBrush)
    {
        pfnRealzizeBrush = EngRealizeBrush;
    }

    psurfPattern = SURFACE_LockSurface(pebo->pbrush->hbmPattern);
    if (!psurfPattern)
    {
        DPRINT1("No pattern, nothing to realize!\n");
        return FALSE;
    }

    /* FIXME: implement mask */
    psurfMask = NULL;

    /* Create xlateobj for the brush */
    pxlo = IntCreateBrushXlate(pebo->pbrush, psurfTrg, pebo->crCurrentBack);

    /* Perform the realization */
    bResult = pfnRealzizeBrush(&pebo->BrushObject,
                               &pebo->psurfTrg->SurfObj,
                               &psurfPattern->SurfObj,
                               psurfMask ? &psurfMask->SurfObj : NULL,
                               pxlo,
                               -1); // FIXME: what about hatch brushes?

    EngDeleteXlate(pxlo);

    if (psurfPattern)
        SURFACE_UnlockSurface(psurfPattern);

    if (psurfMask)
        SURFACE_UnlockSurface(psurfMask);

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
                EngFreeMem(pbo->pvRbrush);
            pbo->pvRbrush = NULL;
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
