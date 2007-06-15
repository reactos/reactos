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

static PVID_FUNCTION_TABLE VidTable;
extern VID_FUNCTION_TABLE VidVgaTable;
extern VID_FUNCTION_TABLE VidVgaTextTable;
extern VID_FUNCTION_TABLE VidXboxTable;

/* FUNCTIONS *****************************************************************/

BOOLEAN NTAPI
VidInitialize(
   IN BOOLEAN SetMode)
{
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
   return VidTable->Initialize(SetMode);
}

VOID STDCALL
VidResetDisplay(VOID)
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
