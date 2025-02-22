/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Brush Functions
 * FILE:              win32ss/gdi/eng/engbrush.c
 * PROGRAMER:         Jason Filby
 *                    Timo Kreuzer
 */

#include <win32k.h>

DBG_DEFAULT_CHANNEL(EngBrush);

static const ULONG gaulHatchBrushes[HS_DDI_MAX][8] =
{
    {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF}, /* HS_HORIZONTAL */
    {0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7}, /* HS_VERTICAL   */
    {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F}, /* HS_FDIAGONAL  */
    {0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE}, /* HS_BDIAGONAL  */
    {0xF7, 0xF7, 0xF7, 0xF7, 0x00, 0xF7, 0xF7, 0xF7}, /* HS_CROSS      */
    {0x7E, 0xBD, 0xDB, 0xE7, 0xE7, 0xDB, 0xBD, 0x7E}  /* HS_DIAGCROSS  */
};

HSURF gahsurfHatch[HS_DDI_MAX];

/** Internal functions ********************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitBrushImpl(VOID)
{
    ULONG i;
    SIZEL sizl = {8, 8};

    /* Loop all hatch styles */
    for (i = 0; i < HS_DDI_MAX; i++)
    {
        /* Create a default hatch bitmap */
        gahsurfHatch[i] = (HSURF)EngCreateBitmap(sizl,
                                                 0,
                                                 BMF_1BPP,
                                                 0,
                                                 (PVOID)gaulHatchBrushes[i]);
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
EBRUSHOBJ_vInit(EBRUSHOBJ *pebo,
    PBRUSH pbrush,
    PSURFACE psurf,
    COLORREF crBackgroundClr,
    COLORREF crForegroundClr,
    PPALETTE ppalDC)
{
    ASSERT(pebo);
    ASSERT(pbrush);

    pebo->BrushObject.flColorType = 0;
    pebo->BrushObject.pvRbrush = NULL;
    pebo->pbrush = pbrush;
    pebo->pengbrush = NULL;
    pebo->flattrs = pbrush->flAttrs;
    pebo->psoMask = NULL;

    /* Initialize 1 bpp fore and back colors */
    pebo->crCurrentBack = crBackgroundClr;
    pebo->crCurrentText = crForegroundClr;

    pebo->psurfTrg = psurf;
    /* We are initializing for a new memory DC */
    if(!pebo->psurfTrg)
        pebo->psurfTrg = psurfDefaultBitmap;
    ASSERT(pebo->psurfTrg);
    ASSERT(pebo->psurfTrg->ppal);

    /* Initialize palettes */
    pebo->ppalSurf = pebo->psurfTrg->ppal;
    GDIOBJ_vReferenceObjectByPointer(&pebo->ppalSurf->BaseObject);
    pebo->ppalDC = ppalDC;
    if(!pebo->ppalDC)
        pebo->ppalDC = gppalDefault;
    GDIOBJ_vReferenceObjectByPointer(&pebo->ppalDC->BaseObject);
    pebo->ppalDIB = NULL;

    if (pbrush->flAttrs & BR_IS_NULL)
    {
        /* NULL brushes don't need a color */
        pebo->BrushObject.iSolidColor = 0;
    }
    else if (pbrush->flAttrs & BR_IS_SOLID)
    {
        /* Set the RGB color */
        EBRUSHOBJ_vSetSolidRGBColor(pebo, pbrush->BrushAttr.lbColor);
    }
    else
    {
        /* This is a pattern brush that needs realization */
        pebo->BrushObject.iSolidColor = 0xFFFFFFFF;

        /* Use foreground color of hatch brushes */
        if (pbrush->flAttrs & BR_IS_HATCH)
            pebo->crCurrentText = pbrush->BrushAttr.lbColor;
    }
}

VOID
NTAPI
EBRUSHOBJ_vInitFromDC(EBRUSHOBJ *pebo,
    PBRUSH pbrush, PDC pdc)
{
    EBRUSHOBJ_vInit(pebo, pbrush, pdc->dclevel.pSurface,
        pdc->pdcattr->crBackgroundClr, pdc->pdcattr->crForegroundClr,
        pdc->dclevel.ppal);
}

VOID
FASTCALL
EBRUSHOBJ_vSetSolidRGBColor(EBRUSHOBJ *pebo, COLORREF crColor)
{
    ULONG iSolidColor;
    EXLATEOBJ exlo;

    /* Never use with non-solid brushes */
    ASSERT(pebo->flattrs & BR_IS_SOLID);

    /* Set the RGB color */
    crColor &= 0xFFFFFF;
    pebo->crRealize = crColor;
    pebo->ulRGBColor = crColor;

    /* Initialize an XLATEOBJ RGB -> surface */
    EXLATEOBJ_vInitialize(&exlo,
                          &gpalRGB,
                          pebo->ppalSurf,
                          pebo->crCurrentBack,
                          0,
                          0);

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
        /* Unlock the bitmap again */
        SURFACE_ShareUnlockSurface(pebo->pengbrush);
        pebo->pengbrush = NULL;
    }

    /* Check if there's a driver's realisation */
    if (pebo->BrushObject.pvRbrush)
    {
        /* Free allocated driver memory */
        EngFreeMem(pebo->BrushObject.pvRbrush);
        pebo->BrushObject.pvRbrush = NULL;
    }

    if (pebo->psoMask != NULL)
    {
        SURFACE_ShareUnlockSurface(pebo->psoMask);
        pebo->psoMask = NULL;
    }

    /* Dereference the palettes */
    if (pebo->ppalSurf)
    {
        PALETTE_ShareUnlockPalette(pebo->ppalSurf);
    }
    if (pebo->ppalDC)
    {
        PALETTE_ShareUnlockPalette(pebo->ppalDC);
    }
    if (pebo->ppalDIB)
    {
        PALETTE_ShareUnlockPalette(pebo->ppalDIB);
    }
}

VOID
NTAPI
EBRUSHOBJ_vUpdateFromDC(
    EBRUSHOBJ *pebo,
    PBRUSH pbrush,
    PDC pdc)
{
    /* Cleanup the brush */
    EBRUSHOBJ_vCleanup(pebo);

    /* Reinitialize */
    EBRUSHOBJ_vInitFromDC(pebo, pbrush, pdc);
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
    PSURFACE psurfRealize;
    POINTL ptlSrc = {0, 0};
    RECTL rclDest;
    ULONG lWidth;

    /* Calculate width in bytes of the realized brush */
    lWidth = WIDTH_BYTES_ALIGN32(psoPattern->sizlBitmap.cx,
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
    psurfRealize = SURFACE_ShareLockSurface(hbmpRealize);

    /* Already delete the pattern bitmap (will be kept until dereferenced) */
    EngDeleteSurface((HSURF)hbmpRealize);

    if (!psurfRealize)
    {
        return FALSE;
    }

    /* Copy the bits to the new format bitmap */
    rclDest.left = rclDest.top = 0;
    rclDest.right = psoPattern->sizlBitmap.cx;
    rclDest.bottom = psoPattern->sizlBitmap.cy;
    psoRealize = &psurfRealize->SurfObj;
    EngCopyBits(psoRealize, psoPattern, NULL, pxlo, &rclDest, &ptlSrc);


    pebo = CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject);
    pebo->pengbrush = (PVOID)psurfRealize;

    return TRUE;
}

static
PPALETTE
FixupDIBBrushPalette(
    _In_ PPALETTE ppalDIB,
    _In_ PPALETTE ppalDC)
{
    PPALETTE ppalNew;
    ULONG i, iPalIndex, crColor;

    /* Allocate a new palette */
    ppalNew = PALETTE_AllocPalette(PAL_INDEXED,
                                   ppalDIB->NumColors,
                                   NULL,
                                   0,
                                   0,
                                   0);
    if (ppalNew == NULL)
    {
        ERR("Failed to allcate palette for brush\n");
        return NULL;
    }

    /* Loop all colors */
    for (i = 0; i < ppalDIB->NumColors; i++)
    {
        /* Get the RGB color, which is the index into the DC palette */
        iPalIndex = PALETTE_ulGetRGBColorFromIndex(ppalDIB, i);

        /* Roll over when index is too big */
        iPalIndex %= ppalDC->NumColors;

        /* Set the indexed DC color as the new color */
        crColor = PALETTE_ulGetRGBColorFromIndex(ppalDC, iPalIndex);
        PALETTE_vSetRGBColorForIndex(ppalNew, i, crColor);
    }

    /* Return the new palette */
    return ppalNew;
}

BOOL
NTAPI
EBRUSHOBJ_bRealizeBrush(EBRUSHOBJ *pebo, BOOL bCallDriver)
{
    BOOL bResult;
    PFN_DrvRealizeBrush pfnRealizeBrush = NULL;
    PSURFACE psurfPattern;
    SURFOBJ *psoMask;
    PPDEVOBJ ppdev;
    EXLATEOBJ exlo;
    PPALETTE ppalPattern;
    PBRUSH pbr = pebo->pbrush;
    HBITMAP hbmPattern;
    ULONG iHatch;

    /* All EBRUSHOBJs have a surface, see EBRUSHOBJ_vInit */
    ASSERT(pebo->psurfTrg);

    ppdev = (PPDEVOBJ)pebo->psurfTrg->SurfObj.hdev;
    if (!ppdev)
        ppdev = gpmdev->ppdevGlobal;

    if (bCallDriver)
    {
        /* Get the Drv function */
        pfnRealizeBrush = ppdev->DriverFunctions.RealizeBrush;
        if (pfnRealizeBrush == NULL)
        {
            ERR("No DrvRealizeBrush. Cannot realize brush\n");
            return FALSE;
        }

        /* Get the mask */
        psoMask = EBRUSHOBJ_psoMask(pebo);
    }
    else
    {
        /* Use the Eng function */
        pfnRealizeBrush = EngRealizeBrush;

        /* We don't handle the mask bitmap here. We do this only on demand */
        psoMask = NULL;
    }

    /* Check if this is a hatch brush */
    if (pbr->flAttrs & BR_IS_HATCH)
    {
        /* Get the hatch brush pattern from the PDEV */
        hbmPattern = (HBITMAP)ppdev->ahsurf[pbr->iHatch];
        iHatch = pbr->iHatch;
    }
    else
    {
        /* Use the brushes pattern */
        hbmPattern = pbr->hbmPattern;
        iHatch = -1;
    }

    psurfPattern = SURFACE_ShareLockSurface(hbmPattern);
    ASSERT(psurfPattern);
    ASSERT(psurfPattern->ppal);

    /* DIB brushes with DIB_PAL_COLORS usage need a new palette */
    if (pbr->flAttrs & BR_IS_DIBPALCOLORS)
    {
        /* Create a palette with the colors from the DC */
        ppalPattern = FixupDIBBrushPalette(psurfPattern->ppal, pebo->ppalDC);
        if (ppalPattern == NULL)
        {
            ERR("FixupDIBBrushPalette() failed.\n");
            return FALSE;
        }

        pebo->ppalDIB = ppalPattern;
    }
    else
    {
        /* The palette is already as it should be */
        ppalPattern = psurfPattern->ppal;
    }

    /* Initialize XLATEOBJ for the brush */
    EXLATEOBJ_vInitialize(&exlo,
                          ppalPattern,
                          pebo->psurfTrg->ppal,
                          0,
                          pebo->crCurrentBack,
                          pebo->crCurrentText);

    /* Create the realization */
    bResult = pfnRealizeBrush(&pebo->BrushObject,
                              &pebo->psurfTrg->SurfObj,
                              &psurfPattern->SurfObj,
                              psoMask,
                              &exlo.xlo,
                              iHatch);

    /* Cleanup the XLATEOBJ */
    EXLATEOBJ_vCleanup(&exlo);

    /* Unlock surface */
    SURFACE_ShareUnlockSurface(psurfPattern);

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

SURFOBJ*
NTAPI
EBRUSHOBJ_psoPattern(EBRUSHOBJ *pebo)
{
    PSURFACE psurfPattern;

    psurfPattern = EBRUSHOBJ_pvGetEngBrush(pebo);

    return psurfPattern ? &psurfPattern->SurfObj : NULL;
}

SURFOBJ*
NTAPI
EBRUSHOBJ_psoMask(EBRUSHOBJ *pebo)
{
    HBITMAP hbmMask;
    PSURFACE psurfMask;
    PPDEVOBJ ppdev;

    /* Check if we don't have a mask yet */
    if (pebo->psoMask == NULL)
    {
        /* Check if this is a hatch brush */
        if (pebo->flattrs & BR_IS_HATCH)
        {
            /* Get the PDEV */
            ppdev = (PPDEVOBJ)pebo->psurfTrg->SurfObj.hdev;
            if (!ppdev)
                ppdev = gpmdev->ppdevGlobal;

            /* Use the hatch bitmap as the mask */
            hbmMask = (HBITMAP)ppdev->ahsurf[pebo->pbrush->iHatch];
            psurfMask = SURFACE_ShareLockSurface(hbmMask);
            if (psurfMask == NULL)
            {
                ERR("Failed to lock hatch brush for PDEV %p, iHatch %lu\n",
                    ppdev, pebo->pbrush->iHatch);
                return NULL;
            }

            NT_ASSERT(psurfMask->SurfObj.iBitmapFormat == BMF_1BPP);
            pebo->psoMask = &psurfMask->SurfObj;
        }
    }

    return pebo->psoMask;
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
    pbo->pvRbrush = EngAllocMem(0, cj, GDITAG_RBRUSH);
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
