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

    rclDest = (RECTL){0, 0, psoPattern->sizlBitmap.cx, psoPattern->sizlBitmap.cy};

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
    EngCopyBits(psoRealize, psoPattern, NULL, pxlo, &rclDest, &ptlSrc);

    /* Unlock the bitmap again */
    EngUnlockSurface(psoRealize);

    pebo = CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject);
    pebo->pengbrush = (PVOID)hbmpRealize;

    return TRUE;
}

VOID
FASTCALL
EBRUSHOBJ_vInit(EBRUSHOBJ *pebo, PBRUSH pbrush, XLATEOBJ *pxlo)
{
    ULONG iSolidColor;

    ASSERT(pebo);
    ASSERT(pbrush);

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
        iSolidColor = XLATEOBJ_iXlate(pxlo, pbrush->BrushAttr.lbColor);
        pebo->BrushObject.iSolidColor = iSolidColor;
    }
    else
    {
        /* This is a pattern brush that needs realization */
        pebo->BrushObject.iSolidColor = 0xFFFFFFFF;
//        EBRUSHOBJ_bRealizeBrush(pebo);
    }

//    pebo->psurfTrg = psurfTrg;
    pebo->BrushObject.pvRbrush = pbrush->ulRealization;
    pebo->BrushObject.flColorType = 0;
    pebo->pbrush = pbrush;
    pebo->flattrs = pbrush->flAttrs;
    pebo->XlateObject = pxlo;
}

VOID
FASTCALL
EBRUSHOBJ_vSetSolidBrushColor(EBRUSHOBJ *pebo, ULONG iSolidColor)
{
    /* Never use with non-solid brushes */
    ASSERT(pebo->flattrs & GDIBRUSH_IS_SOLID);

    pebo->BrushObject.iSolidColor = iSolidColor;
}

BOOL
FASTCALL
EBRUSHOBJ_bRealizeBrush(EBRUSHOBJ *pebo)
{
    BOOL bResult;
    PFN_DrvRealizeBrush pfnRealzizeBrush;
    PSURFACE psurfTrg, psurfPattern, psurfMask;
    PPDEVOBJ ppdev;
    XLATEOBJ *pxlo;

    psurfTrg = pebo->psurfTrg; // FIXME: all EBRUSHOBJs need a surface
    ppdev = (PPDEVOBJ)psurfTrg->SurfObj.hdev; // FIXME: all SURFACEs need a PDEV

    pfnRealzizeBrush = NULL;//ppdev->DriverFunctions.RealizeBrush;
    if (!pfnRealzizeBrush)
    {
        pfnRealzizeBrush = EngRealizeBrush;
    }

    psurfPattern = SURFACE_LockSurface(pebo->pbrush->hbmPattern);

    /* FIXME: implement mask */
    psurfMask = NULL;

    // FIXME
    pxlo = NULL;

    bResult = pfnRealzizeBrush(&pebo->BrushObject, 
                               &pebo->psurfTrg->SurfObj,
                               psurfPattern ? &psurfPattern->SurfObj : NULL,
                               psurfMask ? &psurfMask->SurfObj : NULL,
                               pxlo,
                               -1); // FIXME: what about hatch brushes?

    if (psurfPattern)
        SURFACE_UnlockSurface(psurfPattern);

    if (psurfMask)
        SURFACE_UnlockSurface(psurfMask);

    return bResult;
}

VOID
FASTCALL
EBRUSHOBJ_vUnrealizeBrush(EBRUSHOBJ *pebo)
{
    /* Check if it's a GDI realisation */
    if (pebo->pengbrush)
    {
        
    }
    else if (pebo->BrushObject.pvRbrush)
    {
        /* Free allocated driver memory */
        EngFreeMem(pebo->BrushObject.pvRbrush);
    }
}



VOID
FASTCALL
EBRUSHOBJ_vUpdate(EBRUSHOBJ *pebo, PBRUSH pbrush, XLATEOBJ *pxlo)
{
    /* Unrealize the brush */
    EBRUSHOBJ_vUnrealizeBrush(pebo);

    EBRUSHOBJ_vInit(pebo, pbrush, pxlo);
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
    // FIXME: this is wrong! Read msdn.
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
