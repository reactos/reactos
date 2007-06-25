/*
 * ReactOS Boot video driver
 *
 * Copyright (C) 2004-2007 Filip Navara, Aleksey Bragin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* INCLUDES ******************************************************************/

#include "bootvid.h"
#define NDEBUG
#include <debug.h>

extern ULONG ScrollRegion[4];
extern ULONG TextColor, curr_x, curr_y;
extern BOOLEAN NextLine;

extern UCHAR BitmapFont8x16[256 * 16];
extern UCHAR BitmapFont10x18[256 * 18 * 2];

USHORT VgaPalette[] = {
    0x0000, /* 0 black */
    0xA800, /* 1 blue */
    0x0540, /* 2 green */
    0xAD40, /* 3 cyan */
    0x0015, /* 4 red */
    0xA815, /* 5 magenta */
    0x02B5, /* 6 brown */
    0xAD55, /* 7 light gray */
    0x52AA, /* 8 dark gray */
    0xFAAA, /* 9 bright blue */
    0x57EA, /* 10 bright green */
    0xFFEA, /* 11 bright cyan */
    0x52BF, /* 12 bright red */
    0xFABF, /* 13 bright magenta */
    0x57FF, /* 14 bright yellow */
    0xFFFF  /* 15 bright white */
};

static BOOLEAN VidpInitialized = FALSE;
static PUCHAR VidpMemory = (VOID *)0xEE000000;

#define CHAR_WIDTH  10
#define CHAR_HEIGHT 18
#define TOP_BOTTOM_LINES 0

#define DISPLAY_WIDTH 1200

UCHAR BytesPerPixel = 2;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
DisplayCharacter(CHAR Character,
                 ULONG Left,
                 ULONG Top,
                 ULONG Color,
                 ULONG BackTextColor)
{
    PUSHORT FontPtr;
    WORD *Pixel;
    USHORT Mask;
    ULONG Stride = DISPLAY_WIDTH * BytesPerPixel;
    unsigned Line;
    unsigned Col;
    UCHAR BytesPerChar = (CHAR_WIDTH < 8) ? 1 : 2;
    BOOLEAN Transparent = FALSE;

    /* Transform from VGA palette to RGB565 */
    if (Color < 16)
        Color = VgaPalette[Color];
    
    if (BackTextColor < 16)
        BackTextColor = VgaPalette[BackTextColor];
    else
        Transparent = TRUE;

    FontPtr = (PUSHORT)(BitmapFont10x18 + Character * CHAR_HEIGHT * BytesPerChar);

    Pixel = (WORD *) ((char *) VidpMemory + (Top + TOP_BOTTOM_LINES) * Stride
        + Left * BytesPerPixel);
    for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
        Mask = (1 << (CHAR_WIDTH-1));
        for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
            if (!Transparent)
                Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? Color : BackTextColor);
            else
                Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? Color : Pixel[Col]); // transparent background

            Mask = Mask >> 1;
        }
        Pixel = (WORD *) ((char *) Pixel + Stride);
    }
}

VOID
NTAPI
FbScroll(ULONG Scroll)
{
    ULONG Top;
    PUCHAR SourceOffset, DestOffset, j;
    ULONG Offset;
    ULONG i;

    /* Set memory positions of the scroll */
    SourceOffset = VidpMemory +
        (ScrollRegion[1] * DISPLAY_WIDTH * BytesPerPixel) +
        ScrollRegion[0] * BytesPerPixel;

    DestOffset = SourceOffset + Scroll * DISPLAY_WIDTH * BytesPerPixel;

    /* Save top and check if it's above the bottom */
    Top = ScrollRegion[1];
    if (Top > ScrollRegion[3]) return;

    /* Start loop */
    do
    {
        /* Set number of bytes to loop and start offset */
        Offset = ScrollRegion[0] * BytesPerPixel;
        j = SourceOffset;

        /* Check if this is part of the scroll region */
        if (Offset <= (ScrollRegion[2] * BytesPerPixel))
        {
            /* Update position */
            i = DestOffset - SourceOffset;

            /* Loop the X axis */
            do
            {
                /* Write value in the new position so that we can do the scroll */
                //WRITE_REGISTER_UCHAR((PUCHAR)j,
                //                     READ_REGISTER_UCHAR((PUCHAR)j + i));
                RtlCopyMemory(j, j+i, BytesPerPixel);

                /* Move to the next memory location to write to */
                j += BytesPerPixel;

                /* Move to the next byte in the region */
                Offset++;

                /* Make sure we don't go past the scroll region */
            } while (Offset <= (ScrollRegion[2] * BytesPerPixel));
        }

        /* Move to the next line */
        SourceOffset += DISPLAY_WIDTH * BytesPerPixel;
        DestOffset += DISPLAY_WIDTH * BytesPerPixel;

        /* Increase top */
        Top++;

        /* Make sure we don't go past the scroll region */
    } while (Top <= ScrollRegion[3]);
}


/* PUBLIC FUNCTIONS ***********************************************************/

static BOOLEAN NTAPI
VidFbInitialize(
   IN BOOLEAN SetMode)
{
   PHYSICAL_ADDRESS PhysicalAddress;

   if (!VidpInitialized)
   {
      PhysicalAddress.QuadPart = 0xFD000000;
      VidpMemory = MmMapIoSpace(PhysicalAddress, 0x200000, MmNonCached);
      if (VidpMemory == NULL)
         return FALSE;

      VidpInitialized = TRUE;
   }

   DPRINT1("VidFbInitialize(SetMode %d)\n", SetMode);
   return TRUE;
}

static VOID NTAPI
VidFbResetDisplay(VOID)
{
}

static VOID NTAPI
VidFbCleanUp(VOID)
{
}

static VOID NTAPI
VidFbBufferToScreenBlt(
   IN PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height,
   IN ULONG Delta)
{
}

static VOID NTAPI
VidFbScreenToBufferBlt(
   OUT PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height,
   IN LONG Delta)
{
}

static VOID NTAPI
VidFbBitBlt(
   IN PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top)
{
}

static VOID NTAPI
VidFbSolidColorFill(
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Right,
   IN ULONG Bottom,
   IN ULONG Color)
{
    ULONG Line, Col;
    PUSHORT p;

    /* Sanity checks */
    if (Top > Bottom)
        return;
    if (Left > Right)
        return;

    /* Translate color to the VGA palette */
    if (Color < 16)
        Color = VgaPalette[Color];

    for (Line = Top; Line <= Bottom; Line++)
    {
        p = (PUSHORT)(VidpMemory + (Line * DISPLAY_WIDTH * BytesPerPixel) + Left * BytesPerPixel);
        for (Col = 0; Col < (Right-Left); Col++)
        {
            *p++ = Color;
        }
    }
}

static VOID NTAPI
VidFbDisplayString(
   IN PCSTR String)
{
    ULONG TopDelta = CHAR_HEIGHT-2;

    /* Start looping the string */
    while (*String)
    {
        /* Treat new-line separately */
        if (*String == '\n')
        {
            /* Modify Y position */
            curr_y += TopDelta;
            if (curr_y >= ScrollRegion[3])
            {
                /* Scroll the view */
                FbScroll(TopDelta);
                curr_y -= TopDelta;

                /* Preserve row */
                //PreserveRow(curr_y, TopDelta, TRUE);
            }

            /* Update current X */
            curr_x = ScrollRegion[0];

            /* Preseve the current row */
            //PreserveRow(curr_y, TopDelta, FALSE);
        }
        else if (*String == '\r')
        {
            /* Update current X */
            curr_x = ScrollRegion[0];

            /* Check if we're being followed by a new line */
            if (String[1] != '\n') NextLine = TRUE;
        }
        else
        {
            /* Check if we had a \n\r last time */
            if (NextLine)
            {
                /* We did, preserve the current row */
                //PreserveRow(curr_y, TopDelta, TRUE);
                NextLine = FALSE;
            }

            /* Display this character */
            DisplayCharacter(*String, curr_x, curr_y, TextColor, 16);
            curr_x += CHAR_WIDTH;

            /* Check if we should scroll */
            if (curr_x > ScrollRegion[2])
            {
                /* Update Y position and check if we should scroll it */
                curr_y += TopDelta;
                if (curr_y > ScrollRegion[3])
                {
                    /* Do the scroll */
                    FbScroll(TopDelta);
                    curr_y -= TopDelta;

                    /* Save the row */
                    //PreserveRow(curr_y, TopDelta, TRUE);
                }

                /* Update X */
                curr_x = ScrollRegion[0];
            }
        }

        /* Get the next character */
        String++;
    }
}

static VOID NTAPI
VidFbDisplayStringXY(
   IN PUCHAR String,
   IN ULONG Top,
   IN ULONG Left,
   IN BOOLEAN Transparent)
{
}

VID_FUNCTION_TABLE VidFramebufTable = {
   VidFbInitialize,
   VidFbCleanUp,
   VidFbResetDisplay,
   VidFbBufferToScreenBlt,
   VidFbScreenToBufferBlt,
   VidFbBitBlt,
   VidFbSolidColorFill,
   VidFbDisplayString,
   VidFbDisplayStringXY
};
