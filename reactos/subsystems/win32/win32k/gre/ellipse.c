/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/ellipse.c
 * PURPOSE:         Graphic engine: ellipses
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

VOID NTAPI
GreEllipse(PDC dc,
           INT Left,
           INT Top,
           INT Right,
           INT Bottom)
{
    RECTL RectBounds;
    BOOLEAN ret = TRUE;
    LONG PenWidth, PenOrigWidth;
    LONG RadiusX, RadiusY, CenterX, CenterY;
    POINTL BrushOrigin;
    PBRUSH pbrush;

    /* Update line brush and temporarily change its width */
    pbrush = dc->dclevel.pbrLine;

    PenOrigWidth = PenWidth = pbrush->ptPenWidth.x;
    if (pbrush->ulPenStyle == PS_NULL) PenWidth = 0;

    if (pbrush->ulPenStyle == PS_INSIDEFRAME)
    {
       if (2*PenWidth > (Right - Left)) PenWidth = (Right -Left + 1)/2;
       if (2*PenWidth > (Bottom - Top)) PenWidth = (Bottom -Top + 1)/2;
       Left   += PenWidth / 2;
       Right  -= (PenWidth - 1) / 2;
       Top    += PenWidth / 2;
       Bottom -= (PenWidth - 1) / 2;
    }

    if (!PenWidth) PenWidth = 1;
    pbrush->ptPenWidth.x = PenWidth;

    RectBounds.left   = Left;
    RectBounds.right  = Right;
    RectBounds.top    = Top;
    RectBounds.bottom = Bottom;

    // Setup for dynamic width and height.
    RadiusX = max((RectBounds.right - RectBounds.left) / 2, 2); // Needs room
    RadiusY = max((RectBounds.bottom - RectBounds.top) / 2, 2);
    CenterX = (RectBounds.right + RectBounds.left) / 2;
    CenterY = (RectBounds.bottom + RectBounds.top) / 2;

    DPRINT("Ellipse 1: Left: %d, Top: %d, Right: %d, Bottom: %d\n",
               RectBounds.left,RectBounds.top,RectBounds.right,RectBounds.bottom);

    DPRINT("Ellipse 2: XLeft: %d, YLeft: %d, Width: %d, Height: %d\n",
               CenterX - RadiusX, CenterY + RadiusY, RadiusX*2, RadiusY*2);

    BrushOrigin.x = dc->dclevel.ptlBrushOrigin.x;
    BrushOrigin.y = dc->dclevel.ptlBrushOrigin.y;

    /* Draw filled part */
    if (dc->dclevel.pbrFill && !(dc->dclevel.pbrFill->flAttrs & GDIBRUSH_IS_NULL))
    {
        ret = GrepFillEllipse(dc,
                              CenterX - RadiusX,
                              CenterY - RadiusY,
                              RadiusX*2, // Width
                              RadiusY*2, // Height
                              dc->dclevel.pbrFill,
                              &BrushOrigin);
    }

    /* Draw line part */
    if (ret)
    {
        ret = GrepDrawEllipse(dc,
                              CenterX - RadiusX,
                              CenterY - RadiusY,
                              RadiusX*2, // Width
                              RadiusY*2, // Height
                              pbrush,
                              &BrushOrigin);
    }

    /* Restore changed width */
    pbrush->ptPenWidth.x = PenOrigWidth;
}

/* EOF */
