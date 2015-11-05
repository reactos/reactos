/*
 * PROJECT:         ReactOS VGA display driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/drivers/displays/vga/objects/transblt.c
 * PURPOSE:         
 * PROGRAMMERS:     
 */

#include <vgaddi.h>

BOOL APIENTRY
DrvTransparentBlt(
    IN SURFOBJ* Dest,
    IN SURFOBJ* Source,
    IN CLIPOBJ* Clip,
    IN XLATEOBJ* ColorTranslation,
    IN RECTL* DestRect,
    IN RECTL* SourceRect,
    IN ULONG TransparentColor,
    IN ULONG Reserved)
{
    LONG dx, dy, sx, sy;

    dx = abs(DestRect->right  - DestRect->left);
    dy = abs(DestRect->bottom - DestRect->top);

    sx = abs(SourceRect->right  - SourceRect->left);
    sy = abs(SourceRect->bottom - SourceRect->top);

    if (sx < dx) dx = sx;
    if (sy < dy) dy = sy;

    /* FIXME: adjust using SourceRect */
    DIB_TransparentBltToVGA(DestRect->left, DestRect->top, dx, dy, Source->pvScan0, Source->lDelta, TransparentColor);

    return TRUE;
}
