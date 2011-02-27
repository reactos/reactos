/*
*  ReactOS W32 Subsystem
*  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along
*  with this program; if not, write to the Free Software Foundation, Inc.,
*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
/* $Id: bitmaps.c 28300 2007-08-12 15:20:09Z tkreuzer $ */

#include <win32k.h>

#define NDEBUG
#include <debug.h>



BOOL APIENTRY
NtGdiAlphaBlend(
    HDC hDCDest,
    LONG XOriginDest,
    LONG YOriginDest,
    LONG WidthDest,
    LONG HeightDest,
    HDC hDCSrc,
    LONG XOriginSrc,
    LONG YOriginSrc,
    LONG WidthSrc,
    LONG HeightSrc,
    BLENDFUNCTION BlendFunc,
    HANDLE hcmXform)
{
    PDC DCDest;
    PDC DCSrc;
    HDC ahDC[2];
    PGDIOBJ apObj[2];
    SURFACE *BitmapDest, *BitmapSrc;
    RECTL DestRect, SourceRect;
    BOOL bResult;
    EXLATEOBJ exlo;
    BLENDOBJ BlendObj;
    BlendObj.BlendFunction = BlendFunc;

    if (WidthDest < 0 || HeightDest < 0 || WidthSrc < 0 || HeightSrc < 0)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DPRINT("Locking DCs\n");
    ahDC[0] = hDCDest;
    ahDC[1] = hDCSrc ;
    GDIOBJ_LockMultipleObjs(2, ahDC, apObj);
    DCDest = apObj[0];
    DCSrc = apObj[1];

    if ((NULL == DCDest) || (NULL == DCSrc))
    {
        DPRINT1("Invalid dc handle (dest=0x%08x, src=0x%08x) passed to NtGdiAlphaBlend\n", hDCDest, hDCSrc);
        EngSetLastError(ERROR_INVALID_HANDLE);
        if(DCSrc) GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
        if(DCDest) GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
        return FALSE;
    }

    if (DCDest->dctype == DC_TYPE_INFO || DCDest->dctype == DCTYPE_INFO)
    {
        GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
        GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    DestRect.left   = XOriginDest;
    DestRect.top    = YOriginDest;
    DestRect.right  = XOriginDest + WidthDest;
    DestRect.bottom = YOriginDest + HeightDest;
    IntLPtoDP(DCDest, (LPPOINT)&DestRect, 2);

    DestRect.left   += DCDest->ptlDCOrig.x;
    DestRect.top    += DCDest->ptlDCOrig.y;
    DestRect.right  += DCDest->ptlDCOrig.x;
    DestRect.bottom += DCDest->ptlDCOrig.y;

    SourceRect.left   = XOriginSrc;
    SourceRect.top    = YOriginSrc;
    SourceRect.right  = XOriginSrc + WidthSrc;
    SourceRect.bottom = YOriginSrc + HeightSrc;
    IntLPtoDP(DCSrc, (LPPOINT)&SourceRect, 2);

    SourceRect.left   += DCSrc->ptlDCOrig.x;
    SourceRect.top    += DCSrc->ptlDCOrig.y;
    SourceRect.right  += DCSrc->ptlDCOrig.x;
    SourceRect.bottom += DCSrc->ptlDCOrig.y;

    if (!DestRect.right ||
        !DestRect.bottom ||
        !SourceRect.right ||
        !SourceRect.bottom)
    {
        GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
        GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
        return TRUE;
    }

    /* Prepare DCs for blit */
    DPRINT("Preparing DCs for blit\n");
    DC_vPrepareDCsForBlit(DCDest, DestRect, DCSrc, SourceRect);

    /* Determine surfaces to be used in the bitblt */
    BitmapDest = DCDest->dclevel.pSurface;
    if (!BitmapDest)
    {
        bResult = FALSE ;
        goto leave ;
    }

    BitmapSrc = DCSrc->dclevel.pSurface;
    if (!BitmapSrc)
    {
        bResult = FALSE;
        goto leave;
    }

    /* Create the XLATEOBJ. */
    EXLATEOBJ_vInitXlateFromDCs(&exlo, DCSrc, DCDest);

    /* Perform the alpha blend operation */
    DPRINT("Performing the alpha Blend\n");
    bResult = IntEngAlphaBlend(&BitmapDest->SurfObj,
                               &BitmapSrc->SurfObj,
                               DCDest->rosdc.CombinedClip,
                               &exlo.xlo,
                               &DestRect,
                               &SourceRect,
                               &BlendObj);

    EXLATEOBJ_vCleanup(&exlo);
leave :
    DPRINT("Finishing blit\n");
    DC_vFinishBlit(DCDest, DCSrc);
    GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
    GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);

    return bResult;
}

BOOL APIENTRY
NtGdiBitBlt(
    HDC hDCDest,
    INT XDest,
    INT YDest,
    INT Width,
    INT Height,
    HDC hDCSrc,
    INT XSrc,
    INT YSrc,
    DWORD ROP,
    IN DWORD crBackColor,
    IN FLONG fl)
{
    PDC DCDest;
    PDC DCSrc = NULL;
    HDC ahDC[2];
    PGDIOBJ apObj[2];
    PDC_ATTR pdcattr = NULL;
    SURFACE *BitmapDest, *BitmapSrc = NULL;
    RECTL DestRect, SourceRect;
    POINTL SourcePoint;
    BOOL Status = FALSE;
    EXLATEOBJ exlo;
    XLATEOBJ *XlateObj = NULL;
    BOOL UsesSource = ROP3_USES_SOURCE(ROP);

    DPRINT("Locking DCs\n");
    ahDC[0] = hDCDest;
    ahDC[1] = hDCSrc ;
    GDIOBJ_LockMultipleObjs(2, ahDC, apObj);
    DCDest = apObj[0];
    DCSrc = apObj[1];

    if (NULL == DCDest)
    {
        if(DCSrc) GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
        DPRINT("Invalid destination dc handle (0x%08x) passed to NtGdiBitBlt\n", hDCDest);
        return FALSE;
    }

    if (DCDest->dctype == DC_TYPE_INFO)
    {
        if(DCSrc) GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
        GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    if (UsesSource)
    {
        if (NULL == DCSrc)
        {
            GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
            DPRINT("Invalid source dc handle (0x%08x) passed to NtGdiBitBlt\n", hDCSrc);
            return FALSE;
        }
        if (DCSrc->dctype == DC_TYPE_INFO)
        {
            GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
            GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
            /* Yes, Windows really returns TRUE in this case */
            return TRUE;
        }
    }
    else if(DCSrc)
    {
        DPRINT1("Getting a valid Source handle without using source!!!\n");
        GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
        DCSrc = NULL ;
    }

    pdcattr = DCDest->pdcattr;

    DestRect.left   = XDest;
    DestRect.top    = YDest;
    DestRect.right  = XDest+Width;
    DestRect.bottom = YDest+Height;
    IntLPtoDP(DCDest, (LPPOINT)&DestRect, 2);

    DestRect.left   += DCDest->ptlDCOrig.x;
    DestRect.top    += DCDest->ptlDCOrig.y;
    DestRect.right  += DCDest->ptlDCOrig.x;
    DestRect.bottom += DCDest->ptlDCOrig.y;

    SourcePoint.x = XSrc;
    SourcePoint.y = YSrc;

    if (UsesSource)
    {
        IntLPtoDP(DCSrc, (LPPOINT)&SourcePoint, 1);

        SourcePoint.x += DCSrc->ptlDCOrig.x;
        SourcePoint.y += DCSrc->ptlDCOrig.y;
        /* Calculate Source Rect */
        SourceRect.left = SourcePoint.x;
        SourceRect.top = SourcePoint.y;
        SourceRect.right = SourcePoint.x + DestRect.right - DestRect.left;
        SourceRect.bottom = SourcePoint.y + DestRect.bottom - DestRect.top ;
    }

    /* Prepare blit */
    DC_vPrepareDCsForBlit(DCDest, DestRect, DCSrc, SourceRect);

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(DCDest);

    /* Determine surfaces to be used in the bitblt */
    BitmapDest = DCDest->dclevel.pSurface;
    if (!BitmapDest)
        goto cleanup;

    if (UsesSource)
    {
        {
            BitmapSrc = DCSrc->dclevel.pSurface;
            if (!BitmapSrc)
                goto cleanup;
        }
    }

    /* Create the XLATEOBJ. */
    if (UsesSource)
    {
        EXLATEOBJ_vInitXlateFromDCs(&exlo, DCSrc, DCDest);
        XlateObj = &exlo.xlo;
    }

    /* Perform the bitblt operation */
    Status = IntEngBitBlt(&BitmapDest->SurfObj,
                          BitmapSrc ? &BitmapSrc->SurfObj : NULL,
                          NULL,
                          DCDest->rosdc.CombinedClip,
                          XlateObj,
                          &DestRect,
                          &SourcePoint,
                          NULL,
                          &DCDest->eboFill.BrushObject,
                          &DCDest->dclevel.pbrFill->ptOrigin,
                          ROP3_TO_ROP4(ROP));

    if (UsesSource)
        EXLATEOBJ_vCleanup(&exlo);
cleanup:
    DC_vFinishBlit(DCDest, DCSrc);
    if (UsesSource)
    {
        GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
    }
    GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);

    return Status;
}

BOOL APIENTRY
NtGdiTransparentBlt(
    HDC hdcDst,
    INT xDst,
    INT yDst,
    INT cxDst,
    INT cyDst,
    HDC hdcSrc,
    INT xSrc,
    INT ySrc,
    INT cxSrc,
    INT cySrc,
    COLORREF TransColor)
{
    PDC DCDest, DCSrc;
    HDC ahDC[2];
    PGDIOBJ apObj[2];
    RECTL rcDest, rcSrc;
    SURFACE *BitmapDest, *BitmapSrc = NULL;
    ULONG TransparentColor = 0;
    BOOL Ret = FALSE;
    EXLATEOBJ exlo;

    DPRINT("Locking DCs\n");
    ahDC[0] = hdcDst;
    ahDC[1] = hdcSrc ;
    GDIOBJ_LockMultipleObjs(2, ahDC, apObj);
    DCDest = apObj[0];
    DCSrc = apObj[1];

    if ((NULL == DCDest) || (NULL == DCSrc))
    {
        DPRINT1("Invalid dc handle (dest=0x%08x, src=0x%08x) passed to NtGdiAlphaBlend\n", hdcDst, hdcSrc);
        EngSetLastError(ERROR_INVALID_HANDLE);
        if(DCSrc) GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
        if(DCDest) GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
        return FALSE;
    }

    if (DCDest->dctype == DC_TYPE_INFO || DCDest->dctype == DCTYPE_INFO)
    {
        GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
        GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    rcDest.left   = xDst;
    rcDest.top    = yDst;
    rcDest.right  = rcDest.left + cxDst;
    rcDest.bottom = rcDest.top + cyDst;
    IntLPtoDP(DCDest, (LPPOINT)&rcDest, 2);

    rcDest.left   += DCDest->ptlDCOrig.x;
    rcDest.top    += DCDest->ptlDCOrig.y;
    rcDest.right  += DCDest->ptlDCOrig.x;
    rcDest.bottom += DCDest->ptlDCOrig.y;

    rcSrc.left   = xSrc;
    rcSrc.top    = ySrc;
    rcSrc.right  = rcSrc.left + cxSrc;
    rcSrc.bottom = rcSrc.top + cySrc;
    IntLPtoDP(DCSrc, (LPPOINT)&rcSrc, 2);

    rcSrc.left   += DCSrc->ptlDCOrig.x;
    rcSrc.top    += DCSrc->ptlDCOrig.y;
    rcSrc.right  += DCSrc->ptlDCOrig.x;
    rcSrc.bottom += DCSrc->ptlDCOrig.y;

    /* Prepare for blit */
    DC_vPrepareDCsForBlit(DCDest, rcDest, DCSrc, rcSrc);

    BitmapDest = DCDest->dclevel.pSurface;
    if (!BitmapDest)
    {
        goto done;
    }

    BitmapSrc = DCSrc->dclevel.pSurface;
    if (!BitmapSrc)
    {
        goto done;
    }

    /* Translate Transparent (RGB) Color to the source palette */
    EXLATEOBJ_vInitialize(&exlo, &gpalRGB, BitmapSrc->ppal, 0, 0, 0);
    TransparentColor = XLATEOBJ_iXlate(&exlo.xlo, (ULONG)TransColor);
    EXLATEOBJ_vCleanup(&exlo);

    EXLATEOBJ_vInitXlateFromDCs(&exlo, DCSrc, DCDest);

    Ret = IntEngTransparentBlt(&BitmapDest->SurfObj, &BitmapSrc->SurfObj,
        DCDest->rosdc.CombinedClip, &exlo.xlo, &rcDest, &rcSrc,
        TransparentColor, 0);

    EXLATEOBJ_vCleanup(&exlo);

done:
    DC_vFinishBlit(DCDest, DCSrc);
    GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
    GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);

    return Ret;
}

/***********************************************************************
* MaskBlt
* Ported from WINE by sedwards 11-4-03
*
* Someone thought it would be faster to do it here and then switch back
* to GDI32. I dunno. Write a test and let me know.
* A. It should be in here!
*/

static const DWORD ROP3Table[256] =
{
  0x000042, 0x010289, 0x020C89, 0x0300AA, 0x040C88, 0x0500A9, 0x060865, 0x0702C5,
  0x080F08, 0x090245, 0x0A0329, 0x0B0B2A, 0x0C0324, 0x0D0B25, 0x0E08A5, 0x0F0001,
  0x100C85, 0x1100A6, 0x120868, 0x1302C8, 0x140869, 0x1502C9, 0x165CCA, 0x171D54,
  0x180D59, 0x191CC8, 0x1A06C5, 0x1B0768, 0x1C06CA, 0x1D0766, 0x1E01A5, 0x1F0385,
  0x200F09, 0x210248, 0x220326, 0x230B24, 0x240D55, 0x251CC5, 0x2606C8, 0x271868,
  0x280369, 0x2916CA, 0x2A0CC9, 0x2B1D58, 0x2C0784, 0x2D060A, 0x2E064A, 0x2F0E2A,
  0x30032A, 0x310B28, 0x320688, 0x330008, 0x3406C4, 0x351864, 0x3601A8, 0x370388,
  0x38078A, 0x390604, 0x3A0644, 0x3B0E24, 0x3C004A, 0x3D18A4, 0x3E1B24, 0x3F00EA,
  0x400F0A, 0x410249, 0x420D5D, 0x431CC4, 0x440328, 0x450B29, 0x4606C6, 0x47076A,
  0x480368, 0x4916C5, 0x4A0789, 0x4B0605, 0x4C0CC8, 0x4D1954, 0x4E0645, 0x4F0E25,
  0x500325, 0x510B26, 0x5206C9, 0x530764, 0x5408A9, 0x550009, 0x5601A9, 0x570389,
  0x580785, 0x590609, 0x5A0049, 0x5B18A9, 0x5C0649, 0x5D0E29, 0x5E1B29, 0x5F00E9,
  0x600365, 0x6116C6, 0x620786, 0x630608, 0x640788, 0x650606, 0x660046, 0x6718A8,
  0x6858A6, 0x690145, 0x6A01E9, 0x6B178A, 0x6C01E8, 0x6D1785, 0x6E1E28, 0x6F0C65,
  0x700CC5, 0x711D5C, 0x720648, 0x730E28, 0x740646, 0x750E26, 0x761B28, 0x7700E6,
  0x7801E5, 0x791786, 0x7A1E29, 0x7B0C68, 0x7C1E24, 0x7D0C69, 0x7E0955, 0x7F03C9,
  0x8003E9, 0x810975, 0x820C49, 0x831E04, 0x840C48, 0x851E05, 0x8617A6, 0x8701C5,
  0x8800C6, 0x891B08, 0x8A0E06, 0x8B0666, 0x8C0E08, 0x8D0668, 0x8E1D7C, 0x8F0CE5,
  0x900C45, 0x911E08, 0x9217A9, 0x9301C4, 0x9417AA, 0x9501C9, 0x960169, 0x97588A,
  0x981888, 0x990066, 0x9A0709, 0x9B07A8, 0x9C0704, 0x9D07A6, 0x9E16E6, 0x9F0345,
  0xA000C9, 0xA11B05, 0xA20E09, 0xA30669, 0xA41885, 0xA50065, 0xA60706, 0xA707A5,
  0xA803A9, 0xA90189, 0xAA0029, 0xAB0889, 0xAC0744, 0xAD06E9, 0xAE0B06, 0xAF0229,
  0xB00E05, 0xB10665, 0xB21974, 0xB30CE8, 0xB4070A, 0xB507A9, 0xB616E9, 0xB70348,
  0xB8074A, 0xB906E6, 0xBA0B09, 0xBB0226, 0xBC1CE4, 0xBD0D7D, 0xBE0269, 0xBF08C9,
  0xC000CA, 0xC11B04, 0xC21884, 0xC3006A, 0xC40E04, 0xC50664, 0xC60708, 0xC707AA,
  0xC803A8, 0xC90184, 0xCA0749, 0xCB06E4, 0xCC0020, 0xCD0888, 0xCE0B08, 0xCF0224,
  0xD00E0A, 0xD1066A, 0xD20705, 0xD307A4, 0xD41D78, 0xD50CE9, 0xD616EA, 0xD70349,
  0xD80745, 0xD906E8, 0xDA1CE9, 0xDB0D75, 0xDC0B04, 0xDD0228, 0xDE0268, 0xDF08C8,
  0xE003A5, 0xE10185, 0xE20746, 0xE306EA, 0xE40748, 0xE506E5, 0xE61CE8, 0xE70D79,
  0xE81D74, 0xE95CE6, 0xEA02E9, 0xEB0849, 0xEC02E8, 0xED0848, 0xEE0086, 0xEF0A08,
  0xF00021, 0xF10885, 0xF20B05, 0xF3022A, 0xF40B0A, 0xF50225, 0xF60265, 0xF708C5,
  0xF802E5, 0xF90845, 0xFA0089, 0xFB0A09, 0xFC008A, 0xFD0A0A, 0xFE02A9, 0xFF0062,
};

static __inline BYTE
SwapROP3_SrcDst(BYTE bRop3)
{
    return (bRop3 & 0x99) | ((bRop3 & 0x22) << 1) | ((bRop3 & 0x44) >> 1);
}

#define FRGND_ROP3(ROP4)    ((ROP4) & 0x00FFFFFF)
#define BKGND_ROP3(ROP4)    (ROP3Table[(SwapROP3_SrcDst((ROP4)>>24)) & 0xFF])
#define DSTCOPY    0x00AA0029
#define DSTERASE    0x00220326 /* dest = dest & (~src) : DSna */

/* NOTE: An alternative algorithm could use a pattern brush, created from
 * the mask bitmap and then use raster operation 0xCA to combine the fore
 * and background bitmaps. In this case erasing the bits beforehand would be
 * unneccessary. On the other hand the Operation does not provide an optimized
 * version in the DIB code, while SRCAND and SRCPAINT do.
 * A fully correct implementation would call Eng/DrvBitBlt, but our
 * EngBitBlt completely ignores the mask surface.
 *
 * Msk Fg Bk => x
 *  P  S  D        DPSDxax
 * ------------------------------------------
 *  0  0  0     0  0000xax = 000ax = 00x = 0
 *  0  0  1     1  1001xax = 101ax = 10x = 1
 *  0  1  0     0  0010xax = 001ax = 00x = 0
 *  0  1  1     1  1011xax = 100ax = 10x = 1
 *  1  0  0     0  0100xax = 010ax = 00x = 0
 *  1  0  1     0  1101xax = 111ax = 11x = 0
 *  1  1  0     1  0110xax = 011ax = 01x = 1
 *  1  1  1     1  1111xax = 110ax = 10x = 1
 *
 * Operation index = 11001010 = 0xCA = PSaDPnao = DPSDxax
 *                                     ^ no, this is not random letters, its reverse Polish notation
 */

BOOL APIENTRY
NtGdiMaskBlt(
    HDC hdcDest,
    INT nXDest,
    INT nYDest,
    INT nWidth,
    INT nHeight,
    HDC hdcSrc,
    INT nXSrc,
    INT nYSrc,
    HBITMAP hbmMask,
    INT xMask,
    INT yMask,
    DWORD dwRop,
    IN DWORD crBackColor)
{
    HBITMAP hbmFore, hbmBack;
    HDC hdcMask, hdcFore, hdcBack;
    PDC pdc;
    HBRUSH hbr;
    COLORREF crFore, crBack;

    if (!hbmMask)
        return NtGdiBitBlt(hdcDest,
                           nXDest,
                           nYDest,
                           nWidth,
                           nHeight,
                           hdcSrc,
                           nXSrc,
                           nYSrc,
                           FRGND_ROP3(dwRop),
                           crBackColor,
                           0);

    /* Lock the dest DC */
    pdc = DC_LockDc(hdcDest);
    if (!pdc) return FALSE;

    /* Get brush and colors from dest dc */
    hbr = pdc->pdcattr->hbrush;
    crFore = pdc->pdcattr->crForegroundClr;
    crBack = pdc->pdcattr->crBackgroundClr;

    /* Unlock the DC */
    DC_UnlockDc(pdc);

    /* 1. Create mask bitmap's dc */
    hdcMask = NtGdiCreateCompatibleDC(hdcDest);
    NtGdiSelectBitmap(hdcMask, hbmMask);

    /* 2. Create masked Background bitmap */

    /* 2.1 Create bitmap */
    hdcBack = NtGdiCreateCompatibleDC(hdcDest);
    hbmBack = NtGdiCreateCompatibleBitmap(hdcDest, nWidth, nHeight);
    NtGdiSelectBitmap(hdcBack, hbmBack);

    /* 2.2 Copy source bitmap */
    NtGdiSelectBrush(hdcBack, hbr);
    IntGdiSetBkColor(hdcBack, crBack);
    IntGdiSetTextColor(hdcBack, crFore);
    NtGdiBitBlt(hdcBack, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, SRCCOPY, 0, 0);

    /* 2.3 Do the background rop */
    NtGdiBitBlt(hdcBack, 0, 0, nWidth, nHeight, hdcDest, nXDest, nYDest, BKGND_ROP3(dwRop), 0, 0);

    /* 2.4 Erase the foreground pixels */
    IntGdiSetBkColor(hdcBack, RGB(0xFF, 0xFF, 0xFF));
    IntGdiSetTextColor(hdcBack, RGB(0, 0, 0));
    NtGdiBitBlt(hdcBack, 0, 0, nWidth, nHeight, hdcMask, xMask, yMask, SRCAND, 0, 0);

    /* 3. Create masked Foreground bitmap */

    /* 3.1 Create bitmap */
    hdcFore = NtGdiCreateCompatibleDC(hdcDest);
    hbmFore = NtGdiCreateCompatibleBitmap(hdcDest, nWidth, nHeight);
    NtGdiSelectBitmap(hdcFore, hbmFore);

    /* 3.2 Copy the dest bitmap */
    NtGdiSelectBrush(hdcFore, hbr);
    IntGdiSetBkColor(hdcFore, crBack);
    IntGdiSetTextColor(hdcFore, crFore);
    NtGdiBitBlt(hdcFore, 0, 0, nWidth, nHeight, hdcDest, nXDest, nYDest, SRCCOPY, 0, 0);

    /* 2.3 Do the foreground rop */
    NtGdiBitBlt(hdcFore, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, FRGND_ROP3(dwRop), 0,0);

    /* 2.4 Erase the background pixels */
    IntGdiSetBkColor(hdcFore, RGB(0, 0, 0));
    IntGdiSetTextColor(hdcFore, RGB(0xFF, 0xFF, 0xFF));
    NtGdiBitBlt(hdcFore, 0, 0, nWidth, nHeight, hdcMask, xMask, yMask, SRCAND, 0, 0);

    /* 3. Combine the fore and background into the background bitmap */
    NtGdiBitBlt(hdcBack, 0, 0, nWidth, nHeight, hdcFore, 0, 0, SRCPAINT, 0, 0);

    /* 4. Copy the result to hdcDest */
    NtGdiBitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcBack, 0, 0, SRCCOPY, 0, 0);

    /* 5. delete all temp objects */
    NtGdiDeleteObjectApp(hdcBack);
    NtGdiDeleteObjectApp(hdcFore);
    NtGdiDeleteObjectApp(hdcMask);
    GreDeleteObject(hbmFore);
    GreDeleteObject(hbmBack);

    return TRUE;
}

BOOL
APIENTRY
NtGdiPlgBlt(
    IN HDC hdcTrg,
    IN LPPOINT pptlTrg,
    IN HDC hdcSrc,
    IN INT xSrc,
    IN INT ySrc,
    IN INT cxSrc,
    IN INT cySrc,
    IN HBITMAP hbmMask,
    IN INT xMask,
    IN INT yMask,
    IN DWORD crBackColor)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY
GreStretchBltMask(
    HDC hDCDest,
    INT XOriginDest,
    INT YOriginDest,
    INT WidthDest,
    INT HeightDest,
    HDC hDCSrc,
    INT XOriginSrc,
    INT YOriginSrc,
    INT WidthSrc,
    INT HeightSrc,
    DWORD ROP,
    IN DWORD dwBackColor,
    HDC hDCMask,
    INT XOriginMask,
    INT YOriginMask)
{
    PDC DCDest;
    PDC DCSrc  = NULL;
    PDC DCMask = NULL;
    HDC ahDC[3];
    PGDIOBJ apObj[3];
    PDC_ATTR pdcattr;
    SURFACE *BitmapDest, *BitmapSrc = NULL;
    SURFACE *BitmapMask = NULL;
    RECTL DestRect;
    RECTL SourceRect;
    POINTL MaskPoint;
    BOOL Status = FALSE;
    EXLATEOBJ exlo;
    XLATEOBJ *XlateObj = NULL;
    POINTL BrushOrigin;
    BOOL UsesSource = ROP3_USES_SOURCE(ROP);

    if (0 == WidthDest || 0 == HeightDest || 0 == WidthSrc || 0 == HeightSrc)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DPRINT("Locking DCs\n");
    ahDC[0] = hDCDest;
    ahDC[1] = hDCSrc ;
    ahDC[2] = hDCMask ;
    GDIOBJ_LockMultipleObjs(3, ahDC, apObj);
    DCDest = apObj[0];
    DCSrc = apObj[1];
    DCMask = apObj[2];

    if (NULL == DCDest)
    {
        if(DCSrc) GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
        if(DCMask) GDIOBJ_UnlockObjByPtr(&DCMask->BaseObject);
        DPRINT("Invalid destination dc handle (0x%08x) passed to NtGdiBitBlt\n", hDCDest);
        return FALSE;
    }

    if (DCDest->dctype == DC_TYPE_INFO)
    {
        if(DCSrc) GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
        if(DCMask) GDIOBJ_UnlockObjByPtr(&DCMask->BaseObject);
        GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    if (UsesSource)
    {
        if (NULL == DCSrc)
        {
            GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
            if(DCMask) GDIOBJ_UnlockObjByPtr(&DCMask->BaseObject);
            DPRINT("Invalid source dc handle (0x%08x) passed to NtGdiBitBlt\n", hDCSrc);
            return FALSE;
        }
        if (DCSrc->dctype == DC_TYPE_INFO)
        {
            GDIOBJ_UnlockObjByPtr(&DCDest->BaseObject);
            GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
            if(DCMask) GDIOBJ_UnlockObjByPtr(&DCMask->BaseObject);
            /* Yes, Windows really returns TRUE in this case */
            return TRUE;
        }
    }
    else if(DCSrc)
    {
        DPRINT1("Getting a valid Source handle without using source!!!\n");
        GDIOBJ_UnlockObjByPtr(&DCSrc->BaseObject);
        DCSrc = NULL ;
    }

    pdcattr = DCDest->pdcattr;

    DestRect.left   = XOriginDest;
    DestRect.top    = YOriginDest;
    DestRect.right  = XOriginDest+WidthDest;
    DestRect.bottom = YOriginDest+HeightDest;
    IntLPtoDP(DCDest, (LPPOINT)&DestRect, 2);

    DestRect.left   += DCDest->ptlDCOrig.x;
    DestRect.top    += DCDest->ptlDCOrig.y;
    DestRect.right  += DCDest->ptlDCOrig.x;
    DestRect.bottom += DCDest->ptlDCOrig.y;

    SourceRect.left   = XOriginSrc;
    SourceRect.top    = YOriginSrc;
    SourceRect.right  = XOriginSrc+WidthSrc;
    SourceRect.bottom = YOriginSrc+HeightSrc;

    if (UsesSource)
    {
        IntLPtoDP(DCSrc, (LPPOINT)&SourceRect, 2);

        SourceRect.left   += DCSrc->ptlDCOrig.x;
        SourceRect.top    += DCSrc->ptlDCOrig.y;
        SourceRect.right  += DCSrc->ptlDCOrig.x;
        SourceRect.bottom += DCSrc->ptlDCOrig.y;
    }

    BrushOrigin.x = 0;
    BrushOrigin.y = 0;

    /* Only prepare Source and Dest, hdcMask represents a DIB */
    DC_vPrepareDCsForBlit(DCDest, DestRect, DCSrc, SourceRect);

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(DCDest);

    /* Determine surfaces to be used in the bitblt */
    BitmapDest = DCDest->dclevel.pSurface;
    if (BitmapDest == NULL)
        goto failed;
    if (UsesSource)
    {
        BitmapSrc = DCSrc->dclevel.pSurface;
        if (BitmapSrc == NULL)
            goto failed;

        /* Create the XLATEOBJ. */
        EXLATEOBJ_vInitXlateFromDCs(&exlo, DCSrc, DCDest);
        XlateObj = &exlo.xlo;
    }

    /* Offset the brush */
    BrushOrigin.x += DCDest->ptlDCOrig.x;
    BrushOrigin.y += DCDest->ptlDCOrig.y;

    /* Make mask surface for source surface */
    if (BitmapSrc && DCMask)
    {
        BitmapMask = DCMask->dclevel.pSurface;
        if (BitmapMask &&
            (BitmapMask->SurfObj.sizlBitmap.cx < WidthSrc ||
             BitmapMask->SurfObj.sizlBitmap.cy < HeightSrc))
        {
            DPRINT1("%dx%d mask is smaller than %dx%d bitmap\n",
                    BitmapMask->SurfObj.sizlBitmap.cx, BitmapMask->SurfObj.sizlBitmap.cy,
                    WidthSrc, HeightSrc);
            EXLATEOBJ_vCleanup(&exlo);
            goto failed;
        }
        /* Create mask offset point */
        MaskPoint.x = XOriginMask;
        MaskPoint.y = YOriginMask;
        IntLPtoDP(DCMask, &MaskPoint, 1);
        MaskPoint.x += DCMask->ptlDCOrig.x;
        MaskPoint.y += DCMask->ptlDCOrig.x;
    }

    /* Perform the bitblt operation */
    Status = IntEngStretchBlt(&BitmapDest->SurfObj,
                              &BitmapSrc->SurfObj,
                              BitmapMask ? &BitmapMask->SurfObj : NULL,
                              DCDest->rosdc.CombinedClip,
                              XlateObj,
                              &DestRect,
                              &SourceRect,
                              BitmapMask ? &MaskPoint : NULL,
                              &DCDest->eboFill.BrushObject,
                              &BrushOrigin,
                              ROP3_TO_ROP4(ROP));
    if (UsesSource)
    {
        EXLATEOBJ_vCleanup(&exlo);
    }

failed:
    DC_vFinishBlit(DCDest, DCSrc);
    if (UsesSource)
    {
        DC_UnlockDc(DCSrc);
    }
    if (DCMask)
    {
        DC_UnlockDc(DCMask);
    }
    DC_UnlockDc(DCDest);

    return Status;
}


BOOL APIENTRY
NtGdiStretchBlt(
    HDC hDCDest,
    INT XOriginDest,
    INT YOriginDest,
    INT WidthDest,
    INT HeightDest,
    HDC hDCSrc,
    INT XOriginSrc,
    INT YOriginSrc,
    INT WidthSrc,
    INT HeightSrc,
    DWORD ROP,
    IN DWORD dwBackColor)
{
    return GreStretchBltMask(
                hDCDest,
                XOriginDest,
                YOriginDest,
                WidthDest,
                HeightDest,
                hDCSrc,
                XOriginSrc,
                YOriginSrc,
                WidthSrc,
                HeightSrc,
                ROP,
                dwBackColor,
                NULL,
                0,
                0);
}


BOOL FASTCALL
IntPatBlt(
    PDC pdc,
    INT XLeft,
    INT YLeft,
    INT Width,
    INT Height,
    DWORD dwRop,
    PBRUSH pbrush)
{
    RECTL DestRect;
    SURFACE *psurf;
    EBRUSHOBJ eboFill ;
    POINTL BrushOrigin;
    BOOL ret;

    ASSERT(pbrush);

    if (pbrush->flAttrs & GDIBRUSH_IS_NULL)
    {
        return TRUE;
    }

    if (Width > 0)
    {
        DestRect.left = XLeft;
        DestRect.right = XLeft + Width;
    }
    else
    {
        DestRect.left = XLeft + Width + 1;
        DestRect.right = XLeft + 1;
    }

    if (Height > 0)
    {
        DestRect.top = YLeft;
        DestRect.bottom = YLeft + Height;
    }
    else
    {
        DestRect.top = YLeft + Height + 1;
        DestRect.bottom = YLeft + 1;
    }

    IntLPtoDP(pdc, (LPPOINT)&DestRect, 2);

    DestRect.left   += pdc->ptlDCOrig.x;
    DestRect.top    += pdc->ptlDCOrig.y;
    DestRect.right  += pdc->ptlDCOrig.x;
    DestRect.bottom += pdc->ptlDCOrig.y;

    BrushOrigin.x = pbrush->ptOrigin.x + pdc->ptlDCOrig.x;
    BrushOrigin.y = pbrush->ptOrigin.y + pdc->ptlDCOrig.y;

    DC_vPrepareDCsForBlit(pdc, DestRect, NULL, DestRect);

    psurf = pdc->dclevel.pSurface;

    if (pdc->pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(pdc);

    EBRUSHOBJ_vInit(&eboFill, pbrush, pdc);

    ret = IntEngBitBlt(
        &psurf->SurfObj,
        NULL,
        NULL,
        pdc->rosdc.CombinedClip,
        NULL,
        &DestRect,
        NULL,
        NULL,
        &eboFill.BrushObject,
        &BrushOrigin,
        ROP3_TO_ROP4(dwRop));

    DC_vFinishBlit(pdc, NULL);

    EBRUSHOBJ_vCleanup(&eboFill);

    return ret;
}

BOOL FASTCALL
IntGdiPolyPatBlt(
    HDC hDC,
    DWORD dwRop,
    PPATRECT pRects,
    INT cRects,
    ULONG Reserved)
{
    INT i;
    PBRUSH pbrush;
    PDC pdc;

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pdc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(pdc);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    for (i = 0; i < cRects; i++)
    {
        pbrush = BRUSH_LockBrush(pRects->hBrush);
        if(pbrush != NULL)
        {
            IntPatBlt(
                pdc,
                pRects->r.left,
                pRects->r.top,
                pRects->r.right,
                pRects->r.bottom,
                dwRop,
                pbrush);
            BRUSH_UnlockBrush(pbrush);
        }
        pRects++;
    }

    DC_UnlockDc(pdc);

    return TRUE;
}


BOOL APIENTRY
NtGdiPatBlt(
    HDC hDC,
    INT XLeft,
    INT YLeft,
    INT Width,
    INT Height,
    DWORD ROP)
{
    PBRUSH pbrush;
    DC *dc;
    PDC_ATTR pdcattr;
    BOOL ret;

    BOOL UsesSource = ROP3_USES_SOURCE(ROP);
    if (UsesSource)
    {
        /* in this case we call on GdiMaskBlt */
        return NtGdiMaskBlt(hDC, XLeft, YLeft, Width, Height, 0,0,0,0,0,0,ROP,0);
    }

    dc = DC_LockDc(hDC);
    if (dc == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    pdcattr = dc->pdcattr;

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(dc);

    pbrush = BRUSH_LockBrush(pdcattr->hbrush);
    if (pbrush == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        DC_UnlockDc(dc);
        return FALSE;
    }

    ret = IntPatBlt(dc, XLeft, YLeft, Width, Height, ROP, pbrush);

    BRUSH_UnlockBrush(pbrush);
    DC_UnlockDc(dc);

    return ret;
}

BOOL APIENTRY
NtGdiPolyPatBlt(
    HDC hDC,
    DWORD dwRop,
    IN PPOLYPATBLT pRects,
    IN DWORD cRects,
    IN DWORD Mode)
{
    PPATRECT rb = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Ret;

    if (cRects > 0)
    {
        rb = ExAllocatePoolWithTag(PagedPool, sizeof(PATRECT) * cRects, GDITAG_PLGBLT_DATA);
        if (!rb)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        _SEH2_TRY
        {
            ProbeForRead(pRects,
                cRects * sizeof(PATRECT),
                1);
            RtlCopyMemory(rb,
                pRects,
                cRects * sizeof(PATRECT));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(rb, GDITAG_PLGBLT_DATA);
            SetLastNtError(Status);
            return FALSE;
        }
    }

    Ret = IntGdiPolyPatBlt(hDC, dwRop, rb, cRects, Mode);

    if (cRects > 0)
        ExFreePoolWithTag(rb, GDITAG_PLGBLT_DATA);

    return Ret;
}
