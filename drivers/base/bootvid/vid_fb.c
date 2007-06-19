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

//static BOOLEAN VidpInitialized = FALSE;
static PUCHAR VidpMemory = (VOID *)0xFF000000;

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16
#define TOP_BOTTOM_LINES 0

#define DISPLAY_WIDTH 1200

UCHAR BytesPerPixel = 2;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
DisplayCharacter(CHAR Character,
                 ULONG Left,
                 ULONG Top,
                 ULONG TextColor,
                 ULONG BackTextColor)
{
    PUCHAR FontPtr;
    WORD *Pixel;
    UCHAR Mask;
    ULONG Stride = DISPLAY_WIDTH * BytesPerPixel;
    unsigned Line;
    unsigned Col;

    //HACK
    TextColor = 0xFFFF;
    BackTextColor = 0;

    FontPtr = BitmapFont8x16 + Character * 16;
    Pixel = (WORD *) ((char *) VidpMemory + (Top + TOP_BOTTOM_LINES) * Stride
        + Left * BytesPerPixel);
    for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
        Mask = 0x80;
        for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
            Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? TextColor : BackTextColor);
            Mask = Mask >> 1;
        }
        Pixel = (WORD *) ((char *) Pixel + Stride);
    }
}


/* PUBLIC FUNCTIONS ***********************************************************/

static BOOLEAN NTAPI
VidFbInitialize(
   IN BOOLEAN SetMode)
{
#if 0
   PHYSICAL_ADDRESS PhysicalAddress;

   if (!VidpInitialized)
   {
      PhysicalAddress.QuadPart = 0xFD000000;
      VidpMemory = MmMapIoSpace(PhysicalAddress, 0x200000, MmNonCached);
      if (VidpMemory == NULL)
         return FALSE;

      memset(VidpMemory, 0x00, 0x200000);
      //memset((VOID *)0xFF000000, 0x90, 1024*1024);

      VidpInitialized = TRUE;
   }
#endif
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
}

static VOID NTAPI
VidFbDisplayString(
   IN PCSTR String)
{
    ULONG TopDelta = 14;

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
                //VgaScroll(TopDelta);
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
            curr_x += 8;

            /* Check if we should scroll */
            if (curr_x > ScrollRegion[2])
            {
                /* Update Y position and check if we should scroll it */
                curr_y += TopDelta;
                if (curr_y > ScrollRegion[3])
                {
                    /* Do the scroll */
                    //VgaScroll(TopDelta);
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
