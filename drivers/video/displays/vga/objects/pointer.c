/*
 * PROJECT:         ReactOS VGA display driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/video/displays/vga/objects/pointer.c
 * PURPOSE:         Draws the mouse pointer
 * PROGRAMMERS:     Copyright (C) 1998-2001 ReactOS Team
 */

/* INCLUDES ******************************************************************/

#include <vgaddi.h>

/* GLOBALS *******************************************************************/

static VOID VGADDI_HideCursor(PPDEV ppdev);
static VOID VGADDI_ShowCursor(PPDEV ppdev, PRECTL prcl);

/* FUNCTIONS *****************************************************************/

VOID
VGADDI_BltPointerToVGA(
    IN LONG StartX,
    IN LONG StartY,
    IN ULONG SizeX,
    IN ULONG SizeY,
    IN PUCHAR MaskBits,
    IN ULONG MaskPitch,
    IN ULONG MaskOp)
{
    ULONG DestX, EndX, DestY, EndY;
    UCHAR Mask;
    PUCHAR Video;
    PUCHAR Src;
    UCHAR SrcValue;
    ULONG i, j;
    ULONG Left;
    ULONG Length;
    LONG Bits;

    DestX = StartX < 0 ? 0 : StartX;
    DestY = StartY < 0 ? 0 : StartY;
    EndX = StartX + SizeX;
    EndY = StartY + SizeY;

    /* Set write mode zero. */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 5);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0);

    /* Select raster op. */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 3);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, MaskOp);

    if ((DestX % 8) != 0)
    {
        /* Disable writes to pixels outside of the destination rectangle. */
        Mask = (1 << (8 - (DestX % 8))) - 1;
        if ((EndX - DestX) < (8 - (DestX % 8)))
        {
            Mask &= ~((1 << (8 - (EndX % 8))) - 1);
        }
        WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x8);
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, Mask);

        /* Write the mask. */
        Video = (PUCHAR)vidmem + DestY * 80 + (DestX >> 3);
        Src = MaskBits + (SizeY - (DestY - StartY)) * MaskPitch;
        for (i = DestY; i < EndY; i++, Video += 80)
        {
            Src -= MaskPitch;
            SrcValue = (*Src) >> (DestX % 8);
            (VOID)READ_REGISTER_UCHAR(Video);
            WRITE_REGISTER_UCHAR(Video, SrcValue);
        }
    }

    /* Enable writes to all pixels. */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x8);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0xFF);

    /* Have we finished. */
    if ((EndX - DestX) < (8 - (DestX % 8)))
        return;

    /* Fill any whole rows of eight pixels. */
    Left = (DestX + 7) & ~0x7;
    Length = (EndX >> 3) - (Left >> 3);
    Bits = StartX;
    while (Bits < 0)
        Bits += 8;
    Bits = Bits % 8;
    for (i = DestY; i < EndY; i++)
    {
        Video = (PUCHAR)vidmem + i * 80 + (Left >> 3);
        Src = MaskBits + (EndY - i - 1) * MaskPitch + ((DestX - StartX) >> 3);
        for (j = 0; j < Length; j++, Video++, Src++)
        {
            if (Bits != 0)
            {
                SrcValue = (Src[0] << (8 - Bits));
                SrcValue |= (Src[1] >> Bits);
            }
            else
            {
                SrcValue = Src[0];
            }
            (VOID)READ_REGISTER_UCHAR(Video);
            WRITE_REGISTER_UCHAR(Video, SrcValue);
        }
    }

    /* Fill any pixels on the right which don't fall into a complete row. */
    if ((EndX % 8) != 0)
    {
        /* Disable writes to pixels outside the destination rectangle. */
        Mask = ~((1 << (8 - (EndX % 8))) - 1);
        WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x8);
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, Mask);

        Video = (PUCHAR)vidmem + DestY * 80 + (EndX >> 3);
        Src = MaskBits + (SizeY - (DestY - StartY)) * MaskPitch + (SizeX >> 3) - 1;
        for (i = DestY; i < EndY; i++, Video += 80)
        {
            Src -= MaskPitch;
            SrcValue = (Src[0] << (8 - Bits));
            (VOID)READ_REGISTER_UCHAR(Video);
            WRITE_REGISTER_UCHAR(Video, SrcValue);
        }

        /* Restore the default write masks. */
        WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x8);
        WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0xFF);
    }

    /* Set write mode two. */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 5);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 2);

    /* Select raster op replace. */
    WRITE_PORT_UCHAR((PUCHAR)GRA_I, 3);
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0);
}

BOOL InitPointer(PPDEV ppdev)
{
    ULONG CursorWidth = 32, CursorHeight = 32;
    ULONG PointerAttributesSize;
    ULONG SavedMemSize;

    ppdev->xyHotSpot.x = 0;
    ppdev->xyHotSpot.y = 0;

    /* Determine the size of the pointer attributes */
    PointerAttributesSize = sizeof(VIDEO_POINTER_ATTRIBUTES) +
      ((CursorWidth * CursorHeight * 2) >> 3);

    /* Allocate memory for pointer attributes */
    ppdev->pPointerAttributes = EngAllocMem(0, PointerAttributesSize, ALLOC_TAG);

    ppdev->pPointerAttributes->Flags = 0; /* FIXME: Do this right */
    ppdev->pPointerAttributes->Width = CursorWidth;
    ppdev->pPointerAttributes->Height = CursorHeight;
    ppdev->pPointerAttributes->WidthInBytes = CursorWidth >> 3;
    ppdev->pPointerAttributes->Enable = 0;
    ppdev->pPointerAttributes->Column = 0;
    ppdev->pPointerAttributes->Row = 0;

    /* Allocate memory for the pixels behind the cursor */
    SavedMemSize = ((((CursorWidth + 7) & ~0x7) + 16) * CursorHeight) >> 3;
    ppdev->ImageBehindCursor = VGADDI_AllocSavedScreenBits(SavedMemSize);

    return TRUE;
}

VOID APIENTRY
DrvMovePointer(
    IN SURFOBJ* pso,
    IN LONG x,
    IN LONG y,
    IN PRECTL prcl)
{
    PPDEV ppdev = (PPDEV)pso->dhpdev;

    VGADDI_HideCursor(ppdev);

    if(x != -1)
    {
        ppdev->pPointerAttributes->Column = x;
        ppdev->pPointerAttributes->Row = y;

        VGADDI_ShowCursor(ppdev, prcl);
    }
}


ULONG APIENTRY
DrvSetPointerShape(
    IN SURFOBJ* pso,
    IN SURFOBJ* psoMask,
    IN SURFOBJ* psoColor,
    IN XLATEOBJ* pxlo,
    IN LONG xHot,
    IN LONG yHot,
    IN LONG x,
    IN LONG y,
    IN PRECTL prcl,
    IN ULONG fl)
{
    PPDEV ppdev = (PPDEV)pso->dhpdev;
    ULONG NewWidth, NewHeight;
    PUCHAR Src, Dest;
    ULONG i;

    if (!psoMask)
        return SPS_DECLINE;

    /* Hide the cursor */
    VGADDI_HideCursor(ppdev);

    NewWidth = abs(psoMask->lDelta) << 3;
    NewHeight = (psoMask->cjBits / abs(psoMask->lDelta)) / 2;

    /* Reallocate the space for the cursor if necessary. */
    if (ppdev->pPointerAttributes->Width != NewWidth ||
        ppdev->pPointerAttributes->Height != NewHeight)
    {
        ULONG PointerAttributesSize;
        PVIDEO_POINTER_ATTRIBUTES NewPointerAttributes;
        ULONG SavedMemSize;

        /* Determine the size of the pointer attributes */
        PointerAttributesSize = sizeof(VIDEO_POINTER_ATTRIBUTES) +
            ((NewWidth * NewHeight * 2) >> 3);

        /* Allocate memory for pointer attributes */
        NewPointerAttributes = EngAllocMem(0, PointerAttributesSize, ALLOC_TAG);
        *NewPointerAttributes = *ppdev->pPointerAttributes;
        NewPointerAttributes->Width = NewWidth;
        NewPointerAttributes->Height = NewHeight;
        NewPointerAttributes->WidthInBytes = NewWidth >> 3;
        EngFreeMem(ppdev->pPointerAttributes);
        ppdev->pPointerAttributes = NewPointerAttributes;

        /* Reallocate the space for the saved bits. */
        VGADDI_FreeSavedScreenBits(ppdev->ImageBehindCursor);
        SavedMemSize = ((((NewWidth + 7) & ~0x7) + 16) * NewHeight) >> 3;
        ppdev->ImageBehindCursor = VGADDI_AllocSavedScreenBits(SavedMemSize);
    }

    Src = (PUCHAR)psoMask->pvScan0;
    /* Copy the new cursor in. */
    for (i = 0; i < (NewHeight * 2); i++)
    {
        Dest = (PUCHAR)ppdev->pPointerAttributes->Pixels;
        if (i >= NewHeight)
            Dest += (((NewHeight * 3) - i - 1) * (NewWidth >> 3));
        else
            Dest += ((NewHeight - i - 1) * (NewWidth >> 3));
        memcpy(Dest, Src, NewWidth >> 3);
        Src += psoMask->lDelta;
    }

    /* Set the new cursor position */
    ppdev->xyHotSpot.x = xHot;
    ppdev->xyHotSpot.y = yHot;

    if(x != -1)
    {
        ppdev->pPointerAttributes->Column = x;
        ppdev->pPointerAttributes->Row = y;

      /* show the cursor */
      VGADDI_ShowCursor(ppdev, prcl);
    }

    return SPS_ACCEPT_NOEXCLUDE;
}

static VOID FASTCALL
VGADDI_ComputePointerRect(
    IN PPDEV ppdev,
    IN LONG X,
    IN LONG Y,
    IN PRECTL Rect)
{
    ULONG SizeX, SizeY;

    SizeX = min(((X + (LONG)ppdev->pPointerAttributes->Width) + 7) & ~0x7, ppdev->sizeSurf.cx);
    SizeX -= (X & ~0x7);
    SizeY = min((LONG)ppdev->pPointerAttributes->Height, ppdev->sizeSurf.cy - Y);

    Rect->left = max(X, 0) & ~0x7;
    Rect->top = max(Y, 0);
    Rect->right = Rect->left + SizeX;
    Rect->bottom = Rect->top + SizeY;
}

static VOID
VGADDI_HideCursor(PPDEV ppdev)
{
    if(ppdev->pPointerAttributes->Enable)
    {
        LONG cx, cy;
        RECTL Rect;

        ppdev->pPointerAttributes->Enable = 0;

        cx = ppdev->pPointerAttributes->Column - ppdev->xyHotSpot.x;
        cy = ppdev->pPointerAttributes->Row - ppdev->xyHotSpot.y;

        VGADDI_ComputePointerRect(ppdev, cx, cy, &Rect);

        /* Display what was behind cursor */
        VGADDI_BltFromSavedScreenBits(Rect.left,
                                      Rect.top,
                                      ppdev->ImageBehindCursor,
                                      Rect.right - Rect.left,
                                     Rect.bottom - Rect.top);
    }
}

static VOID
VGADDI_ShowCursor(PPDEV ppdev, PRECTL prcl)
{
    LONG cx, cy;
    PUCHAR AndMask, XorMask;
    ULONG SizeX, SizeY;
    RECTL Rect;

    if(ppdev->pPointerAttributes->Enable)
        return;

    /* Mark the cursor as currently displayed. */
    ppdev->pPointerAttributes->Enable = 1;

    cx = ppdev->pPointerAttributes->Column - ppdev->xyHotSpot.x;
    cy = ppdev->pPointerAttributes->Row - ppdev->xyHotSpot.y;

    /* Capture pixels behind the cursor */
    VGADDI_ComputePointerRect(ppdev, cx, cy, &Rect);

    VGADDI_BltToSavedScreenBits(ppdev->ImageBehindCursor,
                                Rect.left,
                                Rect.top,
                                Rect.right - Rect.left,
                                Rect.bottom - Rect.top);

    /* Display the cursor. */
    SizeX = min((LONG)ppdev->pPointerAttributes->Width, ppdev->sizeSurf.cx - cx);
    SizeY = min((LONG)ppdev->pPointerAttributes->Height, ppdev->sizeSurf.cy - cy);
    AndMask = ppdev->pPointerAttributes->Pixels +
              (ppdev->pPointerAttributes->Height - SizeY) * ppdev->pPointerAttributes->WidthInBytes;
    VGADDI_BltPointerToVGA(cx,
                           cy,
                           SizeX,
                           SizeY,
                           AndMask,
                           ppdev->pPointerAttributes->WidthInBytes,
                           VGA_AND);
    XorMask = AndMask +
        ppdev->pPointerAttributes->WidthInBytes *
        ppdev->pPointerAttributes->Height;
    VGADDI_BltPointerToVGA(cx,
                           cy,
                           SizeX,
                           SizeY,
                           XorMask,
                           ppdev->pPointerAttributes->WidthInBytes,
                           VGA_XOR);

    if (NULL != prcl)
        *prcl = Rect;
}
