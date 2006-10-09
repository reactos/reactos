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

/* FUNCTIONS *****************************************************************/

static BOOLEAN NTAPI
VidXboxInitialize(
   IN BOOLEAN SetMode)
{
   return TRUE;
}

static VOID NTAPI
VidXboxResetDisplay(VOID)
{
}

static VOID NTAPI
VidXboxCleanUp(VOID)
{
}

static VOID NTAPI
VidXboxBufferToScreenBlt(
   IN PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height,
   IN ULONG Delta)
{
}

static VOID NTAPI
VidXboxScreenToBufferBlt(
   OUT PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height,
   IN ULONG Delta)
{
}

static VOID NTAPI
VidXboxBitBlt(
   IN PUCHAR Buffer,
   IN ULONG Left,
   IN ULONG Top)
{
}

static VOID NTAPI
VidXboxSolidColorFill(
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Right,
   IN ULONG Bottom,
   IN ULONG Color)
{
}

static VOID NTAPI
VidXboxDisplayString(
   IN PUCHAR String)
{
}

VID_FUNCTION_TABLE VidXboxTable = {
   VidXboxInitialize,
   VidXboxCleanUp,
   VidXboxResetDisplay,
   VidXboxBufferToScreenBlt,
   VidXboxScreenToBufferBlt,
   VidXboxBitBlt,
   VidXboxSolidColorFill,
   VidXboxDisplayString
};
