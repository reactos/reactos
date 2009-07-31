/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engsurf.c
 * PURPOSE:         Surface and Bitmap Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HBITMAP
APIENTRY
EngCreateBitmap(IN SIZEL Size,
                IN LONG Width,
                IN ULONG Format,
                IN ULONG Flags,
                IN PVOID Bits)
{
    HBITMAP hNewBitmap;

    /* HACK: Why?! */
    Flags ^= BMF_TOPDOWN;

    /* Call the internal routine */
    hNewBitmap = GreCreateBitmap(Size, Width, Format, Flags, Bits);

    /* Make it global */
    GDIOBJ_SetOwnership(hNewBitmap, NULL);

    return hNewBitmap;
}

BOOL
APIENTRY
EngCopyBits(SURFOBJ *psoDest,
            SURFOBJ *psoSource,
            CLIPOBJ *Clip,
            XLATEOBJ *ColorTranslation,
            RECTL *DestRect,
            POINTL *SourcePoint)
{
    BOOLEAN   ret;
    BYTE      clippingType;
    RECT_ENUM RectEnum;
    BOOL      EnumMore;
    BLTINFO   BltInfo;
    SURFACE *psurfDest;
    SURFACE *psurfSource;

    ASSERT(psoDest != NULL && psoSource != NULL && DestRect != NULL && SourcePoint != NULL);

    psurfSource = CONTAINING_RECORD(psoSource, SURFACE, SurfObj);
    SURFACE_LockBitmapBits(psurfSource);

    psurfDest = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);
    if (psoDest != psoSource)
    {
        SURFACE_LockBitmapBits(psurfDest);
    }

    // FIXME: Don't punt to the driver's DrvCopyBits immediately. Instead,
    //        mark the copy block function to be DrvCopyBits instead of the
    //        GDI's copy bit function so as to remove clipping from the
    //        driver's responsibility

    // If one of the surfaces isn't managed by the GDI
    if((psoDest->iType!=STYPE_BITMAP) || (psoSource->iType!=STYPE_BITMAP))
    {
        // Destination surface is device managed
        if(psoDest->iType!=STYPE_BITMAP)
        {
            /* FIXME: Eng* functions shouldn't call Drv* functions. ? */
            if (psurfDest->flHooks & HOOK_COPYBITS)
            {
                ret = GDIDEVFUNCS(psoDest).CopyBits(
                    psoDest, psoSource, Clip, ColorTranslation, DestRect, SourcePoint);

                if (psoDest != psoSource)
                {
                    SURFACE_UnlockBitmapBits(psurfDest);
                }
                SURFACE_UnlockBitmapBits(psurfSource);

                return ret;
            }
        }

        // Source surface is device managed
        if(psoSource->iType!=STYPE_BITMAP)
        {
            /* FIXME: Eng* functions shouldn't call Drv* functions. ? */
            if (psurfSource->flHooks & HOOK_COPYBITS)
            {
                ret = GDIDEVFUNCS(psoSource).CopyBits(
                    psoDest, psoSource, Clip, ColorTranslation, DestRect, SourcePoint);

                if (psoDest != psoSource)
                {
                    SURFACE_UnlockBitmapBits(psurfDest);
                }
                SURFACE_UnlockBitmapBits(psurfSource);

                return ret;
            }
        }

        // If CopyBits wasn't hooked, BitBlt must be
        ret = GrepBitBltEx(psoDest, psoSource,
            NULL, Clip, ColorTranslation, DestRect, SourcePoint,
            NULL, NULL, NULL, ROP3_TO_ROP4(SRCCOPY), TRUE);

        if (psoDest != psoSource)
        {
            SURFACE_UnlockBitmapBits(psurfDest);
        }
        SURFACE_UnlockBitmapBits(psurfSource);

        return ret;
    }

    // Determine clipping type
    if (Clip == (CLIPOBJ *) NULL)
    {
        clippingType = DC_TRIVIAL;
    } else {
        clippingType = Clip->iDComplexity;
    }

    BltInfo.DestSurface = psoDest;
    BltInfo.SourceSurface = psoSource;
    BltInfo.PatternSurface = NULL;
    BltInfo.XlateSourceToDest = ColorTranslation;
    BltInfo.XlatePatternToDest = NULL;
    BltInfo.Rop4 = SRCCOPY;

    switch(clippingType)
    {
    case DC_TRIVIAL:
        BltInfo.DestRect = *DestRect;
        BltInfo.SourcePoint = *SourcePoint;

        DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo);

        //MouseSafetyOnDrawEnd(psoDest);
        if (psoDest != psoSource)
        {
            SURFACE_UnlockBitmapBits(psurfDest);
        }
        SURFACE_UnlockBitmapBits(psurfSource);

        return(TRUE);

    case DC_RECT:
        // Clip the blt to the clip rectangle
        RECTL_bIntersectRect(&BltInfo.DestRect, DestRect, &Clip->rclBounds);

        BltInfo.SourcePoint.x = SourcePoint->x + BltInfo.DestRect.left - DestRect->left;
        BltInfo.SourcePoint.y = SourcePoint->y + BltInfo.DestRect.top  - DestRect->top;

        DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo);

        if (psoDest != psoSource)
        {
            SURFACE_UnlockBitmapBits(psurfDest);
        }
        SURFACE_UnlockBitmapBits(psurfSource);

        return(TRUE);

    case DC_COMPLEX:

        CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do {
            EnumMore = CLIPOBJ_bEnum(Clip,(ULONG) sizeof(RectEnum), (PVOID) &RectEnum);

            if (RectEnum.c > 0)
            {
                RECTL* prclEnd = &RectEnum.arcl[RectEnum.c];
                RECTL* prcl    = &RectEnum.arcl[0];

                do {
                    RECTL_bIntersectRect(&BltInfo.DestRect, prcl, DestRect);

                    BltInfo.SourcePoint.x = SourcePoint->x + prcl->left - DestRect->left;
                    BltInfo.SourcePoint.y = SourcePoint->y + prcl->top - DestRect->top;

                    if(!DibFunctionsForBitmapFormat[psoDest->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo))
                        return FALSE;

                    prcl++;

                } while (prcl < prclEnd);
            }

        } while(EnumMore);

        if (psoDest != psoSource)
        {
            SURFACE_UnlockBitmapBits(psurfDest);
        }
        SURFACE_UnlockBitmapBits(psurfSource);

        return(TRUE);
    }

    if (psoDest != psoSource)
    {
        SURFACE_UnlockBitmapBits(psurfDest);
    }
    SURFACE_UnlockBitmapBits(psurfSource);

    return FALSE;
}

HBITMAP
APIENTRY
EngCreateDeviceBitmap(IN DHSURF dhSurf,
                      IN SIZEL Size,
                      IN ULONG Format)
{
    UNIMPLEMENTED;
    return NULL;
}

HSURF
APIENTRY
EngCreateDeviceSurface(IN DHSURF dhSurf,
                       IN SIZEL Size,
                       IN ULONG Format)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
EngDeleteSurface(IN HSURF hSurf)
{
    /* Get ownership */
    GDIOBJ_SetOwnership(hSurf, PsGetCurrentProcess());

    /* Delete this bitmap */
    GreDeleteBitmap((HGDIOBJ)hSurf);

    /* Indicate success */
    return TRUE;
}

SURFOBJ*
APIENTRY
EngLockSurface(IN HSURF hSurf)
{
    PSURFACE Surface;
    DPRINT("EngLockSurface(%x)\n", hSurf);

    /* Get a pointer to the surface */
    Surface = SURFACE_ShareLock(hSurf);

    /* Return pointer to SURFOBJ object */
    return &Surface->SurfObj;
}

VOID
APIENTRY
EngUnlockSurface(IN SURFOBJ* pSurfObj)
{
    SURFACE *pSurface = CONTAINING_RECORD(pSurfObj, SURFACE, SurfObj);
    DPRINT("EngUnlockSurface(%p)\n", pSurface);

    /* Release the surface */
    SURFACE_ShareUnlock(pSurface);
}

BOOL
APIENTRY
EngAssociateSurface(IN HSURF hSurf,
                    IN HDEV hDev,
                    IN FLONG flHooks)
{
    PSURFACE pSurface;
    PPDEVOBJ pDevObj = (PPDEVOBJ)hDev;
    DPRINT("EngAssociateSurface(%x %x)\n", hSurf, hDev);

    /* Get a pointer to the surface */
    pSurface = SURFACE_Lock(hSurf);

    /* Associate it */
    pSurface->SurfObj.hdev = hDev;
    pSurface->SurfObj.dhpdev = pDevObj->hPDev;

    /* Save hooks flags */
    pSurface->flHooks = flHooks;

    DPRINT1("flHooks: 0x%x\n", flHooks);

    /* Release the pointer */
    SURFACE_Unlock(pSurface);

    /* Indicate success */
    return TRUE;
}

BOOL
APIENTRY
EngEraseSurface(IN SURFOBJ* Surface,
                IN PRECTL prcl,
                IN ULONG iColor)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
EngModifySurface(IN HSURF hSurf,
                 IN HDEV hDev,
                 IN FLONG flHooks,
                 IN FLONG flSurface,
                 IN DHSURF dhSurf,
                 IN PVOID pvScan0,
                 IN LONG lDelta,
                 IN PVOID pvReserved)
{
    UNIMPLEMENTED;
    return FALSE;
}
