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
/* $Id: offscreen.c,v 1.1 2002/09/25 21:21:35 dwelch Exp $
 *
 * PROJECT:         ReactOS VGA16 display driver
 * FILE:            drivers/dd/vga/display/objects/offscreen.c
 * PURPOSE:         Manages off-screen video memory.
 */

/* INCLUDES ******************************************************************/

#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"
#include <debug.h>

/* GLOBALS *******************************************************************/

static LIST_ENTRY SavedBitsList;

/* FUNCTIONS *****************************************************************/

VOID
VGADDI_BltFromSavedScreenBits(ULONG DestX,
			      ULONG DestY,
			      PSAVED_SCREEN_BITS Src,
			      ULONG SizeX,
			      ULONG SizeY)
{
  PUCHAR DestOffset;
  PUCHAR SrcOffset;
  ULONG i, j;

  /* Select write mode 1. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 5);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 1);

  SrcOffset = (PUCHAR)vidmem + Src->Offset;
  for (i = 0; i < SizeY; i++)
    {
      DestOffset = (PUCHAR)vidmem + (i + DestY) * 80 + (DestX >> 3);
      for (j = 0; j < SizeX; j++, SrcOffset++, DestOffset++)
	{
	  (VOID)READ_REGISTER_UCHAR(SrcOffset);
	  WRITE_REGISTER_UCHAR(DestOffset, 0);
	}
    }

  /* Select write mode 2. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 5);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 2);
}

VOID
VGADDI_BltToSavedScreenBits(PSAVED_SCREEN_BITS Dest,
			    ULONG SourceX,
			    ULONG SourceY,
			    ULONG SizeX,
			    ULONG SizeY)
{
  PUCHAR DestOffset;
  PUCHAR SrcOffset;
  ULONG i, j;

  /* Select write mode 1. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 5);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 1);

  DestOffset = (PUCHAR)vidmem + Dest->Offset;

  for (i = 0; i < SizeY; i++)
    {
      SrcOffset = (PUCHAR)vidmem + (SourceY + i) * 80 + (SourceX >> 3);
      for (j = 0; j < SizeX; j++, SrcOffset++, DestOffset++)
	{
	  (VOID)READ_REGISTER_UCHAR(SrcOffset);
	  WRITE_REGISTER_UCHAR(DestOffset, 0);
	}
    }

  /* Select write mode 2. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 5);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 2);
}

VOID
VGADDI_FreeSavedScreenBits(PSAVED_SCREEN_BITS SavedBits)
{
  SavedBits->Free = TRUE;

  if (SavedBits->ListEntry.Blink != &SavedBitsList)
    {
      PSAVED_SCREEN_BITS Previous;

      Previous = CONTAINING_RECORD(SavedBits->ListEntry.Blink, 
				   SAVED_SCREEN_BITS, ListEntry);
      if (Previous->Free)
	{
	  Previous->Size += SavedBits->Size;
	  RemoveEntryList(&SavedBits->ListEntry);
	  EngFreeMem(SavedBits);
	  SavedBits = Previous;
	}
    }
  if (SavedBits->ListEntry.Flink != &SavedBitsList)
    {
      PSAVED_SCREEN_BITS Next;

      Next = CONTAINING_RECORD(SavedBits->ListEntry.Flink, SAVED_SCREEN_BITS, 
			       ListEntry);
      if (Next->Free)
	{
	  SavedBits->Size += Next->Size;
	  RemoveEntryList(&SavedBits->ListEntry);
	  EngFreeMem(SavedBits);
	}
    }
}

PSAVED_SCREEN_BITS
VGADDI_AllocSavedScreenBits(ULONG Size)
{
  PSAVED_SCREEN_BITS Current;
  PLIST_ENTRY CurrentEntry;
  PSAVED_SCREEN_BITS Best;
  PSAVED_SCREEN_BITS New;

  Best = NULL;
  CurrentEntry = SavedBitsList.Flink;
  while (CurrentEntry != &SavedBitsList)
    {
      Current = CONTAINING_RECORD(CurrentEntry, SAVED_SCREEN_BITS, ListEntry);

      if (Current->Free && Current->Size >= Size &&
	  (Best == NULL || (Current->Size - Size) < (Best->Size - Size)))
	{
	  Best = Current;
	}

      CurrentEntry = CurrentEntry->Flink;
    }

  if (Best == NULL)
    {
      return(NULL);
    }
  if (Best->Size == Size)
    {
      Best->Free = FALSE;
      return(Best);
    }
  else
    {
      New = EngAllocMem(0, sizeof(SAVED_SCREEN_BITS), ALLOC_TAG);
      New->Free = FALSE;
      New->Offset = Best->Offset + Size;
      New->Size = Size;
      Best->Size -= Size;
      InsertHeadList(&Best->ListEntry, &New->ListEntry);
      return(New);
    }
}

VOID
VGADDI_InitializeOffScreenMem(ULONG Start, ULONG Length)
{
  PSAVED_SCREEN_BITS FreeBits;

  InitializeListHead(&SavedBitsList);

  FreeBits = EngAllocMem(0, sizeof(SAVED_SCREEN_BITS), ALLOC_TAG);
  FreeBits->Free = TRUE;
  FreeBits->Offset = Start;
  FreeBits->Size = Length;
  InsertHeadList(&SavedBitsList, &FreeBits->ListEntry);
}
