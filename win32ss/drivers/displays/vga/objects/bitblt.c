/*
 * PROJECT:         ReactOS VGA display driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/drivers/displays/vga/objects/bitblt.c
 * PURPOSE:
 * PROGRAMMERS:
 */

#include <vgaddi.h>

#include "bitblt.h"

typedef BOOL (*PFN_VGABlt)(SURFOBJ*, SURFOBJ*, XLATEOBJ*, RECTL*, POINTL*);
typedef BOOL  (APIENTRY *PBLTRECTFUNC)(SURFOBJ* OutputObj,
                                       SURFOBJ* InputObj,
                                       SURFOBJ* Mask,
                                       XLATEOBJ* ColorTranslation,
                                       RECTL* OutputRect,
                                       POINTL* InputPoint,
                                       POINTL* MaskOrigin,
                                       BRUSHOBJ* Brush,
                                       POINTL* BrushOrigin,
                                       ROP4 Rop4);

static BOOL FASTCALL VGADDI_IntersectRect(
    OUT RECTL* prcDst,
    IN RECTL* prcSrc1,
    IN RECTL* prcSrc2)
{
    static const RECTL rclEmpty = { 0, 0, 0, 0 };

    prcDst->left  = max(prcSrc1->left, prcSrc2->left);
    prcDst->right = min(prcSrc1->right, prcSrc2->right);

    if (prcDst->left < prcDst->right)
    {
        prcDst->top    = max(prcSrc1->top, prcSrc2->top);
        prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

        if (prcDst->top < prcDst->bottom) return(TRUE);
    }

    *prcDst = rclEmpty;

    return FALSE;
}

void DIB_BltToVGA_Fixed(int x, int y, int w, int h, void *b, int Source_lDelta, int mod);

BOOL
DIBtoVGA(
    IN SURFOBJ *Dest,
    IN SURFOBJ *Source,
    IN XLATEOBJ *ColorTranslation,
    IN RECTL *DestRect,
    IN POINTL *SourcePoint)
{
    LONG dx, dy;

    dx = DestRect->right  - DestRect->left;
    dy = DestRect->bottom - DestRect->top;

    if (NULL == ColorTranslation || 0 != (ColorTranslation->flXlate & XO_TRIVIAL))
    {
        DIB_BltToVGA(DestRect->left, DestRect->top, dx, dy,
                     (PVOID)((ULONG_PTR)Source->pvScan0 + SourcePoint->y * Source->lDelta + (SourcePoint->x >> 1)),
                     Source->lDelta, SourcePoint->x % 2);
    }
    else
    {
        /* Perform color translation */
        DIB_BltToVGAWithXlate(DestRect->left, DestRect->top, dx, dy,
                              (PVOID)((ULONG_PTR)Source->pvScan0 + SourcePoint->y * Source->lDelta + (SourcePoint->x >> 1)),
                              Source->lDelta, ColorTranslation);
    }
    return FALSE;
}

BOOL
VGAtoDIB(
    IN SURFOBJ *Dest,
    IN SURFOBJ *Source,
    IN XLATEOBJ *ColorTranslation,
    IN RECTL *DestRect,
    IN POINTL *SourcePoint)
{
    LONG i, j, dx, dy;
    UCHAR *GDIpos, *initial;

    /* Used by the temporary DFB */
    //DEVSURF DestDevSurf;

    /* FIXME: Optimize to retrieve entire bytes at a time (see ../vgavideo/vgavideo.c:vgaGetByte) */

    GDIpos = Dest->pvScan0 /* + (DestRect->top * Dest->lDelta) + (DestRect->left >> 1) */ ;
    dx = DestRect->right  - DestRect->left;
    dy = DestRect->bottom - DestRect->top;

    if (ColorTranslation == NULL)
    {
        /* Prepare a Dest Dev Target and copy from the DFB to the DIB */
        //DestDevSurf.NextScan = Dest->lDelta;
        //DestDevSurf.StartBmp = Dest->pvScan0;

        DIB_BltFromVGA(SourcePoint->x, SourcePoint->y, dx, dy, Dest->pvScan0, Dest->lDelta);
    }
    else
    {
        /* Color translation */
        for (j = SourcePoint->y; j < SourcePoint->y + dy; j++)
        {
            initial = GDIpos;
            for (i = SourcePoint->x; i < SourcePoint->x + dx; i++)
            {
                *GDIpos = XLATEOBJ_iXlate(ColorTranslation, vgaGetPixel(i, j));
                GDIpos++;
            }
            GDIpos = initial + Dest->lDelta;
        }
    }
    return FALSE;
}

BOOL
DFBtoVGA(
    IN SURFOBJ *Dest,
    IN SURFOBJ *Source,
    IN XLATEOBJ *ColorTranslation,
    IN RECTL *DestRect,
    IN POINTL *SourcePoint)
{
    /* Do DFBs need color translation?? */
    return FALSE;
}

BOOL
VGAtoDFB(
    IN SURFOBJ *Dest,
    IN SURFOBJ *Source,
    IN XLATEOBJ *ColorTranslation,
    IN RECTL *DestRect,
    IN POINTL *SourcePoint)
{
    /* Do DFBs need color translation?? */
    return FALSE;
}

BOOL
VGAtoVGA(
    IN SURFOBJ *Dest,
    IN SURFOBJ *Source,
    IN XLATEOBJ *ColorTranslation,
    IN RECTL *DestRect,
    IN POINTL *SourcePoint)
{
    LONG i, i2, j, dx, dy, alterx, altery;
    static char buf[SCREEN_X];

    /* Calculate deltas */
    dx = DestRect->right  - DestRect->left;
    dy = DestRect->bottom - DestRect->top;

    alterx = DestRect->left - SourcePoint->x;
    altery = DestRect->top - SourcePoint->y;

    i = SourcePoint->x;
    i2 = i + alterx;

    if (SourcePoint->y >= DestRect->top)
    {
        for (j = SourcePoint->y; j < SourcePoint->y + dy; j++)
        {
            LONG j2 = j + altery;
            vgaReadScan  ( i,  j,  dx, buf );
            vgaWriteScan ( i2, j2, dx, buf );
        }
    }
    else
    {
        for(j = (SourcePoint->y + dy - 1); j >= SourcePoint->y; j--)
        {
            LONG j2 = j + altery;
            vgaReadScan  ( i,  j,  dx, buf );
            vgaWriteScan ( i2, j2, dx, buf );
        }
    }

    return TRUE;
}

BOOL APIENTRY
VGADDI_BltBrush(
    IN SURFOBJ* Dest,
    IN SURFOBJ* Source,
    IN SURFOBJ* MaskSurf,
    IN XLATEOBJ* ColorTranslation,
    IN RECTL* DestRect,
    IN POINTL* SourcePoint,
    IN POINTL* MaskPoint,
    IN BRUSHOBJ* Brush,
    IN POINTL* BrushPoint,
    IN ROP4 Rop4)
{
    UCHAR SolidColor = 0;
    LONG Left;
    LONG Length;
    PUCHAR Video;
    UCHAR Mask;
    INT i, j;
    ULONG RasterOp = VGA_NORMAL;

    /* Punt brush blts to non-device surfaces. */
    if (Dest->iType != STYPE_DEVICE)
        return FALSE;

    /* Punt pattern fills. */
    if ((GET_OPINDEX_FROM_ROP4(Rop4) == GET_OPINDEX_FROM_ROP3(PATCOPY)
         || GET_OPINDEX_FROM_ROP4(Rop4) == GET_OPINDEX_FROM_ROP3(PATINVERT)) &&
        Brush->iSolidColor == 0xFFFFFFFF)
    {
      return FALSE;
    }

    /* Get the brush colour. */
    switch (GET_OPINDEX_FROM_ROP4(Rop4))
    {
        case GET_OPINDEX_FROM_ROP3(PATCOPY): SolidColor = Brush->iSolidColor; break;
        case GET_OPINDEX_FROM_ROP3(PATINVERT): SolidColor = Brush->iSolidColor; RasterOp = VGA_XOR; break;
        case GET_OPINDEX_FROM_ROP3(WHITENESS): SolidColor = 0xF; break;
        case GET_OPINDEX_FROM_ROP3(BLACKNESS): SolidColor = 0x0; break;
        case GET_OPINDEX_FROM_ROP3(DSTINVERT): SolidColor = 0xF; RasterOp = VGA_XOR; break;
    }

    /* Select write mode 3. */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x03);

    /* Setup set/reset register. */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x00);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, (UCHAR)SolidColor);

    /* Enable writes to all pixels. */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0xFF);

    /* Set up data rotate. */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, RasterOp);

    /* Fill any pixels on the left which don't fall into a full row of eight. */
    if ((DestRect->left % 8) != 0)
    {
        /* Disable writes to pixels outside of the destination rectangle. */
        Mask = (1 << (8 - (DestRect->left % 8))) - 1;
        if ((DestRect->right - DestRect->left) < (8 - (DestRect->left % 8)))
            Mask &= ~((1 << (8 - (DestRect->right % 8))) - 1);

        /* Write the same color to each pixel. */
        Video = (PUCHAR)vidmem + DestRect->top * 80 + (DestRect->left >> 3);
        for (i = DestRect->top; i < DestRect->bottom; i++, Video += 80)
        {
            (VOID)READ_REGISTER_UCHAR(Video);
            WRITE_REGISTER_UCHAR(Video, Mask);
        }

        /* Have we finished. */
        if ((DestRect->right - DestRect->left) < (8 - (DestRect->left % 8)))
        {
            /* Restore write mode 2. */
            WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);
            WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x02);

            /* Set up data rotate. */
            WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);
            WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);

            return TRUE;
        }
    }

    /* Fill any whole rows of eight pixels. */
    Left = (DestRect->left + 7) & ~0x7;
    Length = (DestRect->right >> 3) - (Left >> 3);
    for (i = DestRect->top; i < DestRect->bottom; i++)
    {
        Video = (PUCHAR)vidmem + i * 80 + (Left >> 3);
        for (j = 0; j < Length; j++, Video++)
        {
#if 0
            (VOID)READ_REGISTER_UCHAR(Video);
            WRITE_REGISTER_UCHAR(Video, 0xFF);
#else
            char volatile Temp = *Video;
            Temp |= 0;
            *Video = 0xFF;
#endif
        }
    }

    /* Fill any pixels on the right which don't fall into a complete row. */
    if ((DestRect->right % 8) != 0)
    {
        /* Disable writes to pixels outside the destination rectangle. */
        Mask = ~((1 << (8 - (DestRect->right % 8))) - 1);

        Video = (PUCHAR)vidmem + DestRect->top * 80 + (DestRect->right >> 3);
        for (i = DestRect->top; i < DestRect->bottom; i++, Video += 80)
        {
            (VOID)READ_REGISTER_UCHAR(Video);
            WRITE_REGISTER_UCHAR(Video, Mask);
        }
    }

    /* Restore write mode 2. */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x02);

    /* Set up data rotate. */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);

    return TRUE;
}

BOOL APIENTRY
VGADDI_BltSrc(
    IN SURFOBJ* Dest,
    IN SURFOBJ* Source,
    IN SURFOBJ* Mask,
    IN XLATEOBJ* ColorTranslation,
    IN RECTL* DestRect,
    IN POINTL* SourcePoint,
    IN POINTL* MaskOrigin,
    IN BRUSHOBJ* Brush,
    IN POINTL* BrushOrigin,
    IN ROP4 Rop4)
{
    PFN_VGABlt BltOperation;
    ULONG SourceType;

    SourceType = Source->iType;

    if (SourceType == STYPE_BITMAP && Dest->iType == STYPE_DEVICE)
        BltOperation = DIBtoVGA;
    else if (SourceType == STYPE_DEVICE && Dest->iType == STYPE_BITMAP)
        BltOperation = VGAtoDIB;
    else if (SourceType == STYPE_DEVICE && Dest->iType == STYPE_DEVICE)
        BltOperation = VGAtoVGA;
    else if (SourceType == STYPE_DEVBITMAP && Dest->iType == STYPE_DEVICE)
        BltOperation = DFBtoVGA;
    else if (SourceType == STYPE_DEVICE && Dest->iType == STYPE_DEVBITMAP)
        BltOperation = VGAtoDFB;
    else
    {
        /* Punt blts not involving a device or a device-bitmap. */
        return FALSE;
    }

    BltOperation(Dest, Source, ColorTranslation, DestRect, SourcePoint);
    return TRUE;
}

BOOL APIENTRY
VGADDI_BltMask(
    IN SURFOBJ* Dest,
    IN SURFOBJ* Source,
    IN SURFOBJ* Mask,
    IN XLATEOBJ* ColorTranslation,
    IN RECTL* DestRect,
    IN POINTL* SourcePoint,
    IN POINTL* MaskPoint,
    IN BRUSHOBJ* Brush,
    IN POINTL* BrushPoint,
    IN ROP4 Rop4)
{
    LONG i, j, dx, dy, c8;
    BYTE *tMask, *lMask;

    dx = DestRect->right  - DestRect->left;
    dy = DestRect->bottom - DestRect->top;

    if (ColorTranslation == NULL)
    {
        if (Mask != NULL)
        {
            tMask = Mask->pvScan0;
            for (j = 0; j < dy; j++)
            {
                lMask = tMask;
                c8 = 0;
                for (i = 0; i < dx; i++)
                {
                    if((*lMask & maskbit[c8]) != 0)
                        vgaPutPixel(DestRect->left + i, DestRect->top + j, Brush->iSolidColor);
                    c8++;
                     if(c8 == 8)
                    {
                        lMask++;
                        c8=0;
                    }
                }
                tMask += Mask->lDelta;
            }
        }
    }
    return TRUE;
}

BOOL APIENTRY
DrvBitBlt(
    IN SURFOBJ *Dest,
    IN SURFOBJ *Source,
    IN SURFOBJ *Mask,
    IN CLIPOBJ *Clip,
    IN XLATEOBJ *ColorTranslation,
    IN RECTL *DestRect,
    IN POINTL *SourcePoint,
    IN POINTL *MaskPoint,
    IN BRUSHOBJ *Brush,
    IN POINTL *BrushPoint,
    IN ROP4 rop4)
{
    PBLTRECTFUNC BltRectFunc;
    RECTL CombinedRect;
    BOOL Ret = FALSE;
    RECT_ENUM RectEnum;
    BOOL EnumMore;
    UINT i;
    POINTL Pt;
    ULONG Direction;
    POINTL FinalSourcePoint;

    if (Source && SourcePoint)
    {
        FinalSourcePoint.x = SourcePoint->x;
        FinalSourcePoint.y = SourcePoint->y;
    }
    else
    {
        FinalSourcePoint.x = 0;
        FinalSourcePoint.y = 0;
    }

    switch (rop4)
    {
        case ROP3_TO_ROP4(BLACKNESS):
        case ROP3_TO_ROP4(PATCOPY):
        case ROP3_TO_ROP4(WHITENESS):
        case ROP3_TO_ROP4(PATINVERT):
        case ROP3_TO_ROP4(DSTINVERT):
            BltRectFunc = VGADDI_BltBrush;
            break;

        case ROP3_TO_ROP4(SRCCOPY):
            if (BMF_4BPP == Source->iBitmapFormat && BMF_4BPP == Dest->iBitmapFormat)
                BltRectFunc = VGADDI_BltSrc;
            else
                return FALSE;
            break;

        case R4_MASK:
            BltRectFunc = VGADDI_BltMask;
            break;

        default:
            return FALSE;
    }

    switch (NULL == Clip ? DC_TRIVIAL : Clip->iDComplexity)
    {
        case DC_TRIVIAL:
            Ret = (*BltRectFunc)(Dest, Source, Mask, ColorTranslation, DestRect,
                                 SourcePoint, MaskPoint, Brush, BrushPoint,
                                 rop4);
            break;
        case DC_RECT:
            /* Clip the blt to the clip rectangle */
            VGADDI_IntersectRect(&CombinedRect, DestRect, &(Clip->rclBounds));
            Pt.x = FinalSourcePoint.x + CombinedRect.left - DestRect->left;
            Pt.y = FinalSourcePoint.y + CombinedRect.top - DestRect->top;
            Ret = (*BltRectFunc)(Dest, Source, Mask, ColorTranslation, &CombinedRect,
                                 &Pt, MaskPoint, Brush, BrushPoint,
                                 rop4);
            break;
        case DC_COMPLEX:
            Ret = TRUE;
            if (Dest == Source)
            {
                if (DestRect->top <= FinalSourcePoint.y)
                    Direction = DestRect->left < FinalSourcePoint.y ? CD_RIGHTDOWN : CD_LEFTDOWN;
                else
                    Direction = DestRect->left < FinalSourcePoint.x ? CD_RIGHTUP : CD_LEFTUP;
            }
            else
            {
                Direction = CD_ANY;
            }
            CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, Direction, 0);
            do
            {
                EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);

                for (i = 0; i < RectEnum.c; i++)
                {
                    VGADDI_IntersectRect(&CombinedRect, DestRect, RectEnum.arcl + i);
                    Pt.x = FinalSourcePoint.x + CombinedRect.left - DestRect->left;
                    Pt.y = FinalSourcePoint.y + CombinedRect.top - DestRect->top;
                    Ret = (*BltRectFunc)(Dest, Source, Mask, ColorTranslation, &CombinedRect,
                                         &Pt, MaskPoint, Brush, BrushPoint, rop4) &&
                                         Ret;
                }
            } while (EnumMore);
            break;
    }

    return Ret;
}
