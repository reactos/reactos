/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: pointer.c,v 1.13 2003/01/25 23:06:32 ei Exp $
 *
 * PROJECT:         ReactOS VGA16 display driver
 * FILE:            drivers/dd/vga/display/objects/pointer.c
 * PURPOSE:         Draws the mouse pointer.
 */

/* INCLUDES ******************************************************************/

#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"

/* GLOBALS *******************************************************************/

static ULONG oldx, oldy;
static PSAVED_SCREEN_BITS ImageBehindCursor = NULL;
VOID VGADDI_HideCursor(PPDEV ppdev);
VOID VGADDI_ShowCursor(PPDEV ppdev);

/* FUNCTIONS *****************************************************************/

VOID
VGADDI_BltPointerToVGA(ULONG StartX, ULONG StartY, ULONG SizeX,
		       ULONG SizeY, PUCHAR MaskBits, ULONG MaskOp)
{
  ULONG EndX, EndY;
  UCHAR Mask;
  PUCHAR Video;
  PUCHAR Src;
  ULONG MaskPitch;
  UCHAR SrcValue;
  ULONG i, j;
  ULONG Left;
  ULONG Length;

  EndX = StartX + SizeX;
  EndY = StartY + SizeY;
  MaskPitch = SizeX >> 3;

  /* Set write mode zero. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 5);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0);

  /* Select raster op. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 3);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, MaskOp);

  if ((StartX % 8) != 0)
    {
      /* Disable writes to pixels outside of the destination rectangle. */
      Mask = (1 << (8 - (StartX % 8))) - 1;
      if ((EndX - StartX) < (8 - (StartX % 8)))
	{
	  Mask &= ~((1 << (8 - (EndX % 8))) - 1);
	}
      WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x8);
      WRITE_PORT_UCHAR((PUCHAR)GRA_D, Mask);

      /* Write the mask. */
      Video = (PUCHAR)vidmem + StartY * 80 + (StartX >> 3);
      Src = MaskBits;
      for (i = 0; i < SizeY; i++, Video+=80, Src+=MaskPitch)
	{
	  SrcValue = (*Src) >> (StartX % 8);
	  (VOID)READ_REGISTER_UCHAR(Video);
	  WRITE_REGISTER_UCHAR(Video, SrcValue);
	}
    }

  /* Enable writes to all pixels. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x8);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0xFF);

  /* Have we finished. */
  if ((EndX - StartX) < (8 - (StartX % 8)))
    {
      return;
    }

  /* Fill any whole rows of eight pixels. */
  Left = (StartX + 7) & ~0x7;
  Length = (EndX >> 3) - (Left >> 3);
  for (i = StartY; i < EndY; i++)
    {
      Video = (PUCHAR)vidmem + i * 80 + (Left >> 3);
      Src = MaskBits + (i - StartY) * MaskPitch;
      for (j = 0; j < Length; j++, Video++, Src++)
	{
	  if ((StartX % 8) != 0)
	    {
	      SrcValue = (Src[0] << (8 - (StartX % 8)));
	      SrcValue |= (Src[1] >> (StartX % 8));
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

      Video = (PUCHAR)vidmem + StartY * 80 + (EndX >> 3);
      Src = MaskBits + (SizeX >> 3) - 1;
      for (i = StartY; i < EndY; i++, Video+=80, Src+=MaskPitch)
	{
	  SrcValue = (Src[0] << (8 - (StartX % 8)));
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
  ImageBehindCursor = VGADDI_AllocSavedScreenBits(SavedMemSize);

  return(TRUE);
}


VOID STDCALL
DrvMovePointer(IN PSURFOBJ pso,
	       IN LONG x,
	       IN LONG y,
	       IN PRECTL prcl)
{
  PPDEV ppdev = (PPDEV)pso->dhpdev;

  if (x < 0 )
    {
      /* x < 0 and y < 0 indicates we must hide the cursor */
      VGADDI_HideCursor(ppdev);
      return;
    }

  ppdev->xyCursor.x = x;
  ppdev->xyCursor.y = y;

  VGADDI_ShowCursor(ppdev);

  /* Give feedback on the new cursor rectangle */
  /*if (prcl != NULL) ComputePointerRect(ppdev, prcl);*/
}


ULONG STDCALL
DrvSetPointerShape(PSURFOBJ pso,
		   PSURFOBJ psoMask,
		   PSURFOBJ psoColor,
		   PXLATEOBJ pxlo,
		   LONG xHot,
		   LONG yHot,
		   LONG x,
		   LONG y,
		   PRECTL prcl,
		   ULONG fl)
{
  PPDEV ppdev = (PPDEV)pso->dhpdev;
  ULONG NewWidth, NewHeight;
  PUCHAR Src, Dest;
  ULONG i, j;

  NewWidth = psoMask->lDelta << 3;
  NewHeight = (psoMask->cjBits / psoMask->lDelta) / 2;

  /* Hide the cursor */
  if(ppdev->pPointerAttributes->Enable != 0)
    {
      VGADDI_HideCursor(ppdev);
    }

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
      VGADDI_FreeSavedScreenBits(ImageBehindCursor);
      SavedMemSize = ((((NewWidth + 7) & ~0x7) + 16) * NewHeight) >> 3;
      ImageBehindCursor = VGADDI_AllocSavedScreenBits(SavedMemSize);
    }

  /* Copy the new cursor in. */
  for (i = 0; i < (NewHeight * 2); i++)
    {
      Src = (PUCHAR)psoMask->pvBits;
      Src += (i * (NewWidth >> 3));
      Dest = (PUCHAR)ppdev->pPointerAttributes->Pixels;
      if (i >= NewHeight)
	{
	  Dest += (((NewHeight * 3) - i - 1) * (NewWidth >> 3));
	}
      else
	{
	  Dest += ((NewHeight - i - 1) * (NewWidth >> 3));
	}
      memcpy(Dest, Src, NewWidth >> 3);
    }

  /* Set the new cursor position */
  ppdev->xyCursor.x = x;
  ppdev->xyCursor.y = y;

  /* Show the cursor */
  VGADDI_ShowCursor(ppdev);
}

VOID
VGADDI_HideCursor(PPDEV ppdev)
{
  ULONG i, j, cx, cy, bitpos;
  ULONG SizeX;

  /* Display what was behind cursor */
  SizeX = ((oldx + ppdev->pPointerAttributes->Width) + 7) & ~0x7;
  SizeX -= (oldx & ~0x7);
  VGADDI_BltFromSavedScreenBits(oldx & ~0x7,
				oldy,
				ImageBehindCursor,
				SizeX,
				ppdev->pPointerAttributes->Height);

  ppdev->pPointerAttributes->Enable = 0;
}

VOID
VGADDI_ShowCursor(PPDEV ppdev)
{
  ULONG i, j, cx, cy;
  PUCHAR AndMask;
  ULONG SizeX;

  if (ppdev->pPointerAttributes->Enable != 0)
    {
      VGADDI_HideCursor(ppdev);
    }

  /* Capture pixels behind the cursor */
  cx = ppdev->xyCursor.x;
  cy = ppdev->xyCursor.y;

  /* Used to repaint background */
  SizeX = ((cx + ppdev->pPointerAttributes->Width) + 7) & ~0x7;
  SizeX -= (cx & ~0x7);

  VGADDI_BltToSavedScreenBits(ImageBehindCursor,
			      cx & ~0x7,
			      cy,
			      SizeX,
			      ppdev->pPointerAttributes->Height);

  /* Display the cursor. */
  AndMask = ppdev->pPointerAttributes->Pixels +
    ppdev->pPointerAttributes->WidthInBytes *
    ppdev->pPointerAttributes->Height;
  VGADDI_BltPointerToVGA(ppdev->xyCursor.x,
			 ppdev->xyCursor.y,
			 ppdev->pPointerAttributes->Width,
			 ppdev->pPointerAttributes->Height,
			 AndMask,
			 VGA_AND);
  VGADDI_BltPointerToVGA(ppdev->xyCursor.x,
			 ppdev->xyCursor.y,
			 ppdev->pPointerAttributes->Width,
			 ppdev->pPointerAttributes->Height,
			 ppdev->pPointerAttributes->Pixels,
			 VGA_XOR);

  /* Save the new cursor location. */
  oldx = ppdev->xyCursor.x;
  oldy = ppdev->xyCursor.y;

  /* Mark the cursor as currently displayed. */
  ppdev->pPointerAttributes->Enable = 1;
}
