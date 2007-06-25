/*
 * ReactOS Boot video driver
 *
 * Copyright (C) 2005 Filip Navara
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

/* GLOBALS *******************************************************************/

extern VID_FUNCTION_TABLE VidFramebufTable;
extern VID_FUNCTION_TABLE VidVgaTable;
extern VID_FUNCTION_TABLE VidVgaTextTable;
extern VID_FUNCTION_TABLE VidXboxTable;
static PVID_FUNCTION_TABLE VidTable = &VidFramebufTable;

ULONG ScrollRegion[4] =
{
    0,
    0,
    1200 - 1,
    900 - 1
};

ULONG TextColor = 0xF;
ULONG curr_x = 0, curr_y = 0;
BOOLEAN NextLine = FALSE;

/* FUNCTIONS *****************************************************************/

BOOLEAN NTAPI
VidInitialize(
   IN BOOLEAN SetMode)
{
#if I_FINISHED_DEVELOPING
   ULONG PciId;
   
   /*
    * Check for Xbox by identifying device at PCI 0:0:0, if it's
    * 0x10de/0x02a5 then we're running on an Xbox.
    */
   CHECKPOINT;
   WRITE_PORT_ULONG((PULONG)0xcf8, 0x80000000);
   PciId = READ_PORT_ULONG((PULONG)0xcfc);
   if (0x02a510de == PciId)
      VidTable = &VidXboxTable;
   else if (SetMode)
      VidTable = &VidVgaTable;
   else
      VidTable = &VidVgaTextTable;
#else
    VidTable = &VidFramebufTable;
#endif

   return VidTable->Initialize(SetMode);
}

VOID NTAPI
VidResetDisplay(IN BOOLEAN HalReset)
{
   VidTable->ResetDisplay();
}

VOID NTAPI
VidCleanUp(VOID)
{
   VidTable->CleanUp();
}

VOID NTAPI
VidBufferToScreenBlt(
   IN PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height,
   IN ULONG Delta)
{
   VidTable->BufferToScreenBlt(Buffer, Left, Top, Width, Height, Delta);
}

VOID NTAPI
VidScreenToBufferBlt(
   OUT PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height,
   IN ULONG Delta)
{
   VidTable->ScreenToBufferBlt(Buffer, Left, Top, Width, Height, Delta);
}

VOID NTAPI
VidBitBlt(
   IN PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top)
{
   VidTable->BitBlt(Buffer, Left, Top);
}

VOID NTAPI
VidSolidColorFill(
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Right,
   IN ULONG Bottom,
   IN ULONG Color)
{
   VidTable->SolidColorFill(Left, Top, Right, Bottom, Color);
}

VOID NTAPI
VidDisplayString(
   IN PCSTR String)
{
   VidTable->DisplayString(String);
}

VOID NTAPI
VidSetScrollRegion(IN ULONG x1,
                   IN ULONG y1,
                   IN ULONG x2,
                   IN ULONG y2)
{
    /* Assert alignment */
    ASSERT((x1 & 0x7) == 0);
    ASSERT((x2 & 0x7) == 7);

    /* Set Scroll Region */
    ScrollRegion[0] = x1;
    ScrollRegion[1] = y1;
    ScrollRegion[2] = x2;
    ScrollRegion[3] = y2;

    /* Set current X and Y */
    curr_x = x1;
    curr_y = y1;
}

VOID NTAPI
VidDisplayStringXY(IN PUCHAR String,
                   IN ULONG Left,
                   IN ULONG Top,
                   IN BOOLEAN Transparent)
{
    VidTable->DisplayStringXY(String, Left, Top, Transparent);
}

ULONG NTAPI
VidSetTextColor(IN ULONG Color)
{
    ULONG OldColor;

    /* Save the old color and set the new one */
    OldColor = TextColor;
    TextColor = Color;
    return OldColor;
}
