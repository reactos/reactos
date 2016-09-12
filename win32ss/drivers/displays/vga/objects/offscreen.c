/*
 * PROJECT:         ReactOS VGA display driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/drivers/displays/vga/objects/offscreen.c
 * PURPOSE:         Manages off-screen video memory
 * PROGRAMMERS:     Copyright (C) 1998-2001 ReactOS Team
 */

/* INCLUDES ******************************************************************/

#include <vgaddi.h>

/* GLOBALS *******************************************************************/

static LIST_ENTRY SavedBitsList;

/* FUNCTIONS *****************************************************************/

VOID
VGADDI_BltFromSavedScreenBits(
    IN ULONG DestX,
    IN ULONG DestY,
    IN PSAVED_SCREEN_BITS Src,
    IN ULONG SizeX,
    IN ULONG SizeY)
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
        //FIXME: in the loop below we should treat the case when SizeX is not divisible by 8, i.e. partial bytes
        for (j = 0; j < SizeX>>3; j++, SrcOffset++, DestOffset++)
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
VGADDI_BltToSavedScreenBits(
    IN PSAVED_SCREEN_BITS Dest,
    IN ULONG SourceX,
    IN ULONG SourceY,
    IN ULONG SizeX,
    IN ULONG SizeY)
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
        //FIXME: in the loop below we should treat the case when SizeX is not divisible by 8, i.e. partial bytes
        for (j = 0; j < SizeX>>3; j++, SrcOffset++, DestOffset++)
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

    if (!Best)
      return NULL;

    if (Best->Size == Size)
    {
        Best->Free = FALSE;
        return Best;
    }
    else
    {
        New = EngAllocMem(0, sizeof(SAVED_SCREEN_BITS), ALLOC_TAG);
        New->Free = FALSE;
        New->Offset = Best->Offset + Size;
        New->Size = Size;
        Best->Size -= Size;
        InsertHeadList(&Best->ListEntry, &New->ListEntry);
        return New;
    }
}

VOID
VGADDI_InitializeOffScreenMem(
    IN ULONG Start,
    IN ULONG Length)
{
    PSAVED_SCREEN_BITS FreeBits;

    InitializeListHead(&SavedBitsList);

    FreeBits = EngAllocMem(0, sizeof(SAVED_SCREEN_BITS), ALLOC_TAG);
    FreeBits->Free = TRUE;
    FreeBits->Offset = Start;
    FreeBits->Size = Length;
    InsertHeadList(&SavedBitsList, &FreeBits->ListEntry);
}
