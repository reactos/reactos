/*
 * PROJECT:         ReactOS VGA display driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/drivers/displays/vga/objects/paint.c
 * PURPOSE:         
 * PROGRAMMERS:     
 */

#include <vgaddi.h>

#include "brush.h"

BOOL VGADDIFillSolid(SURFOBJ *Surface, RECTL Dimensions, ULONG iColor)
{
    int x, y, x2, y2, w, h, j;
    ULONG orgx, pre1, midpre1, tmppre1;
    //ULONG offset, orgpre1;
    int ileftpix, imidpix, irightpix;
/*    double leftpix, midpix, rightpix;*/

    /* Swap dimensions so that x, y are at topmost left */
    if ( Dimensions.right < Dimensions.left )
    {
        x  = Dimensions.right;
        x2 = Dimensions.left;
    }
    else
    {
        x2 = Dimensions.right;
        x  = Dimensions.left;
    }
    if ( Dimensions.bottom < Dimensions.top )
    {
        y  = Dimensions.bottom;
        y2 = Dimensions.top;
    }
    else
    {
        y2 = Dimensions.bottom;
        y  = Dimensions.top;
    }

    /* Calculate the width and height */
    w = x2 - x;
    h = y2 - y;

    DPRINT("VGADDIFillSolid: x:%d, y:%d, w:%d, h:%d\n", x, y, w, h);

    /* Calculate the starting offset */
    //offset = xconv[x]+y80[y];

    /* Make a note of original x */
    orgx = x;

    /* Calculate the left mask pixels, middle bytes and right mask pixel */
    ileftpix = 7 - mod8(x-1);
    irightpix = mod8(x+w);
    imidpix = (w-ileftpix-irightpix) / 8;

    pre1 = xconv[(x-1)&~7] + y80[y];
    //orgpre1=pre1;

    /* check for overlap ( very horizontally skinny rect ) */
    if ( (ileftpix+irightpix) > w )
    {
        int mask = startmasks[ileftpix] & endmasks[irightpix];

        WRITE_PORT_UCHAR((PUCHAR)GRA_I,0x08);     // set the mask
        WRITE_PORT_UCHAR((PUCHAR)GRA_D,mask);

        tmppre1 = pre1;
        for ( j = y; j < y+h; j++ )
        {
            READ_REGISTER_UCHAR ( vidmem+tmppre1 );
            WRITE_REGISTER_UCHAR ( vidmem+tmppre1, iColor );
            tmppre1 += 80;
        }
        return TRUE;
    }

    if ( ileftpix > 0 )
    {
        /* Write left pixels */
        WRITE_PORT_UCHAR((PUCHAR)GRA_I,0x08);     // set the mask
        WRITE_PORT_UCHAR((PUCHAR)GRA_D,startmasks[ileftpix]);

        tmppre1 = pre1;
        for ( j = y; j < y+h; j++ )
        {
            READ_REGISTER_UCHAR(vidmem + tmppre1);
            WRITE_REGISTER_UCHAR(vidmem + tmppre1, iColor);
            tmppre1 += 80;
        }

        /* Prepare new x for the middle */
        x = orgx + 8;
    }

    if ( imidpix > 0 )
    {
        midpre1=xconv[x] + y80[y];

        /* Set mask to all pixels in byte */
        WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);

        WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0xff);

        for ( j = y; j < y+h; j++ )
        {
            memset(vidmem+midpre1, iColor, imidpix); // write middle pixels, no need to read in latch because of the width
            midpre1 += 80;
        }
    }

    x = orgx + w - irightpix;
    pre1 = xconv[x] + y80[y];

    /* Write right pixels */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I,0x08);     // set the mask bits
    WRITE_PORT_UCHAR((PUCHAR)GRA_D,endmasks[irightpix]);

    for ( j = y; j < y+h; j++ )
    {
        READ_REGISTER_UCHAR(vidmem + pre1);
        WRITE_REGISTER_UCHAR(vidmem + pre1, iColor);
        pre1 += 80;
    }

    return TRUE;
}

BOOL VGADDIPaintRgn(
    IN SURFOBJ *Surface,
    IN CLIPOBJ *ClipRegion,
    IN ULONG iColor,
    IN MIX Mix,
    IN BRUSHINST *BrushInst,
    IN POINTL *BrushPoint)
{
    RECT_ENUM RectEnum;
    BOOL EnumMore;

    DPRINT("VGADDIPaintRgn: iMode: %d, iDComplexity: %d\n Color:%d\n", ClipRegion->iMode, ClipRegion->iDComplexity, iColor);
    switch(ClipRegion->iMode)
    {
        case TC_RECTANGLES:

        /* Rectangular clipping can be handled without enumeration.
           Note that trivial clipping is not possible, since the clipping
           region defines the area to fill */

        if (ClipRegion->iDComplexity == DC_RECT)
        {
            DPRINT("VGADDIPaintRgn Rect:%d %d %d %d\n", ClipRegion->rclBounds.left, ClipRegion->rclBounds.top, ClipRegion->rclBounds.right, ClipRegion->rclBounds.bottom);
            VGADDIFillSolid(Surface, ClipRegion->rclBounds, iColor);
        }
        else
        {
            /* Enumerate all the rectangles and draw them */

            CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, CD_ANY, 0);

            do
            {
                UINT i;
                EnumMore = CLIPOBJ_bEnum(ClipRegion, sizeof(RectEnum), (PVOID) &RectEnum);
                DPRINT("EnumMore: %d, count: %d\n", EnumMore, RectEnum.c);
                for( i=0; i<RectEnum.c; i++)
                {
                    DPRINT("VGADDI enum Rect:%d %d %d %d\n", RectEnum.arcl[i].left, RectEnum.arcl[i].top, RectEnum.arcl[i].right, RectEnum.arcl[i].bottom);
                    VGADDIFillSolid(Surface, RectEnum.arcl[i], iColor);
                }
            } while (EnumMore);
        }
        return TRUE;

        default:
            return FALSE;
    }
}


BOOL APIENTRY
DrvPaint(
    IN SURFOBJ *Surface,
    IN CLIPOBJ *ClipRegion,
    IN BRUSHOBJ *Brush,
    IN POINTL *BrushOrigin,
    IN MIX Mix)
{
    ULONG iSolidColor;

    iSolidColor = Brush->iSolidColor; // FIXME: Realizations and the like

    // If the foreground and background Mixes are the same,
    // (LATER or if there's no brush mask)
    // then see if we can use the solid brush accelerators

    // FIXME: Put in the mix switch below
    // Brush color parameter doesn't matter for these rops
    return(VGADDIPaintRgn(Surface, ClipRegion, iSolidColor, Mix, NULL, BrushOrigin));

    if ((Mix & 0xFF) == ((Mix >> 8) & 0xFF))
    {
        switch (Mix & 0xFF)
        {
            case 0:
                break;

            // FIXME: Implement all these millions of ROPs
            // For now we don't support brushes -- everything is solid

            case R2_MASKNOTPEN:
            case R2_NOTCOPYPEN:
            case R2_XORPEN:
            case R2_MASKPEN:
            case R2_NOTXORPEN:
            case R2_MERGENOTPEN:
            case R2_COPYPEN:
            case R2_MERGEPEN:
            case R2_NOTMERGEPEN:
            case R2_MASKPENNOT:
            case R2_NOTMASKPEN:
            case R2_MERGEPENNOT:

            // Rops that are implicit solid colors
            case R2_NOT:
            case R2_WHITE:
            case R2_BLACK:


            // FIXME: The Paint region belongs HERE

            case R2_NOP:
                return TRUE;

            default:
                break;
        }
    }

/*
doBitBlt:

    // If VGADDIPaint can't do it, VGADDIBitBlt can.. or it might just loop back
    // here and we have a nice infinite loop

    return( VGADDIBitBlt(Surface, NULL, NULL, ClipRegion,
                        NULL, &ClipRegion->rclBounds,
                        NULL, NULL, Brush, BrushOrigin,
                        NULL) ); UNIMPLEMENTED */
}
