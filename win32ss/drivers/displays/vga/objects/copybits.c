/*
 * PROJECT:         ReactOS VGA display driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/drivers/displays/vga/objects/copybits.c
 * PURPOSE:
 * PROGRAMMERS:
 */

#include <vgaddi.h>

BOOL APIENTRY
DrvCopyBits(
    OUT SURFOBJ*  DestObj,
    IN  SURFOBJ*  SourceObj,
    IN  CLIPOBJ*  ClipObj,
    IN  XLATEOBJ* XLateObj,
    IN  RECTL*    DestRectL,
    IN  POINTL*   SrcPointL)
{
    BOOL Done = FALSE;

    if (STYPE_BITMAP == DestObj->iType && BMF_4BPP == DestObj->iBitmapFormat &&
        STYPE_DEVICE == SourceObj->iType)
    {
        /* Screen to 4 BPP DIB */
        DIB_BltFromVGA(SrcPointL->x, SrcPointL->y,
                       DestRectL->right - DestRectL->left,
                       DestRectL->bottom - DestRectL->top,
                       DestObj->pvScan0, DestObj->lDelta);
        Done = TRUE;
    }
    else if (STYPE_DEVICE == DestObj->iType &&
             STYPE_BITMAP == SourceObj->iType && BMF_4BPP == SourceObj->iBitmapFormat)
    {
        /* 4 BPP DIB to Screen */
        DIB_BltToVGA(DestRectL->left, DestRectL->top,
                     DestRectL->right - DestRectL->left,
                     DestRectL->bottom - DestRectL->top,
                     SourceObj->pvScan0, SourceObj->lDelta,
                     0);
        Done = TRUE;
    }

    return Done;
}
