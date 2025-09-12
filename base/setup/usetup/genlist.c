/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/genlist.c
 * PURPOSE:         Generic list functions
 * PROGRAMMER:      Christoph von Wittich <christoph at reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

VOID
InitGenericListUi(
    IN OUT PGENERIC_LIST_UI ListUi,
    IN PGENERIC_LIST List,
    IN PGET_ENTRY_DESCRIPTION GetEntryDescriptionProc)
{
    ListUi->List = List;
    ListUi->FirstShown = NULL;
    ListUi->LastShown = NULL;
    ListUi->BackupEntry = NULL;

    ListUi->GetEntryDescriptionProc = GetEntryDescriptionProc;

    ListUi->Left = 0;
    ListUi->Top = 0;
    ListUi->Right = 0;
    ListUi->Bottom = 0;
    ListUi->Redraw = TRUE;

    ListUi->CurrentItemText[0] = ANSI_NULL;

    /* SaveGenericListUiState(ListUi); */
    ListUi->BackupEntry = ListUi->List->CurrentEntry;
}

VOID
RestoreGenericListUiState(
    IN PGENERIC_LIST_UI ListUi)
{
    ListUi->List->CurrentEntry = ListUi->BackupEntry;
}

static
VOID
DrawListFrame(
    IN PGENERIC_LIST_UI ListUi)
{
    COORD coPos;
    DWORD Written;
    SHORT i;

    /* Draw upper left corner */
    coPos.X = ListUi->Left;
    coPos.Y = ListUi->Top;
    FillConsoleOutputCharacterA(StdOutput,
                                CharUpperLeftCorner, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw upper edge */
    coPos.X = ListUi->Left + 1;
    coPos.Y = ListUi->Top;
    FillConsoleOutputCharacterA(StdOutput,
                                CharHorizontalLine, // '-',
                                ListUi->Right - ListUi->Left - 1,
                                coPos,
                                &Written);

    /* Draw upper right corner */
    coPos.X = ListUi->Right;
    coPos.Y = ListUi->Top;
    FillConsoleOutputCharacterA(StdOutput,
                                CharUpperRightCorner, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw left and right edge */
    for (i = ListUi->Top + 1; i < ListUi->Bottom; i++)
    {
        coPos.X = ListUi->Left;
        coPos.Y = i;
        FillConsoleOutputCharacterA(StdOutput,
                                    CharVerticalLine, // '|',
                                    1,
                                    coPos,
                                    &Written);

        coPos.X = ListUi->Right;
        FillConsoleOutputCharacterA(StdOutput,
                                    CharVerticalLine, //'|',
                                    1,
                                    coPos,
                                    &Written);
    }

    /* Draw lower left corner */
    coPos.X = ListUi->Left;
    coPos.Y = ListUi->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                CharLowerLeftCorner, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw lower edge */
    coPos.X = ListUi->Left + 1;
    coPos.Y = ListUi->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                CharHorizontalLine, // '-',
                                ListUi->Right - ListUi->Left - 1,
                                coPos,
                                &Written);

    /* Draw lower right corner */
    coPos.X = ListUi->Right;
    coPos.Y = ListUi->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                CharLowerRightCorner, // '+',
                                1,
                                coPos,
                                &Written);
}

static
VOID
DrawListEntries(
    IN PGENERIC_LIST_UI ListUi)
{
    PGENERIC_LIST List = ListUi->List;
    PGENERIC_LIST_ENTRY ListEntry;
    PLIST_ENTRY Entry;
    COORD coPos;
    DWORD Written;
    USHORT Width;

    coPos.X = ListUi->Left + 1;
    coPos.Y = ListUi->Top + 1;
    Width = ListUi->Right - ListUi->Left - 1;

    Entry = ListUi->FirstShown;
    while (Entry != &List->ListHead)
    {
        ListEntry = CONTAINING_RECORD(Entry, GENERIC_LIST_ENTRY, Entry);

        if (coPos.Y == ListUi->Bottom)
            break;
        ListUi->LastShown = Entry;

        ListUi->CurrentItemText[0] = ANSI_NULL;
        if (ListUi->GetEntryDescriptionProc)
        {
            ListUi->GetEntryDescriptionProc(ListEntry,
                                            ListUi->CurrentItemText,
                                            ARRAYSIZE(ListUi->CurrentItemText));
        }

        FillConsoleOutputAttribute(StdOutput,
                                   (List->CurrentEntry == ListEntry) ?
                                   FOREGROUND_BLUE | BACKGROUND_WHITE :
                                   FOREGROUND_WHITE | BACKGROUND_BLUE,
                                   Width,
                                   coPos,
                                   &Written);

        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    Width,
                                    coPos,
                                    &Written);

        coPos.X++;
        WriteConsoleOutputCharacterA(StdOutput,
                                     ListUi->CurrentItemText,
                                     min(strlen(ListUi->CurrentItemText), (SIZE_T)Width - 2),
                                     coPos,
                                     &Written);
        coPos.X--;

        coPos.Y++;
        Entry = Entry->Flink;
    }

    while (coPos.Y < ListUi->Bottom)
    {
        FillConsoleOutputAttribute(StdOutput,
                                   FOREGROUND_WHITE | BACKGROUND_BLUE,
                                   Width,
                                   coPos,
                                   &Written);

        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    Width,
                                    coPos,
                                    &Written);
        coPos.Y++;
    }
}

static
VOID
DrawScrollBarGenericList(
    IN PGENERIC_LIST_UI ListUi)
{
    PGENERIC_LIST List = ListUi->List;
    COORD coPos;
    DWORD Written;

    coPos.X = ListUi->Right + 1;
    coPos.Y = ListUi->Top;

    if (ListUi->FirstShown != List->ListHead.Flink)
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    CharUpArrow,
                                    1,
                                    coPos,
                                    &Written);
    }
    else
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    1,
                                    coPos,
                                    &Written);
    }

    coPos.Y = ListUi->Bottom;
    if (ListUi->LastShown != List->ListHead.Blink)
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    CharDownArrow,
                                    1,
                                    coPos,
                                    &Written);
    }
    else
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    1,
                                    coPos,
                                    &Written);
    }
}

static
VOID
CenterCurrentListItem(
    IN PGENERIC_LIST_UI ListUi)
{
    PGENERIC_LIST List = ListUi->List;
    PLIST_ENTRY Entry;
    ULONG MaxVisibleItems, ItemCount, i;

    if ((ListUi->Top == 0 && ListUi->Bottom == 0) ||
        IsListEmpty(&List->ListHead) ||
        List->CurrentEntry == NULL)
    {
        return;
    }

    MaxVisibleItems = (ULONG)(ListUi->Bottom - ListUi->Top - 1);

/*****************************************
    ItemCount = 0;
    Entry = List->ListHead.Flink;
    while (Entry != &List->ListHead)
    {
        ItemCount++;
        Entry = Entry->Flink;
    }
*****************************************/
    ItemCount = List->NumOfEntries; // GetNumberOfListEntries(List);

    if (ItemCount > MaxVisibleItems)
    {
        Entry = &List->CurrentEntry->Entry;
        for (i = 0; i < MaxVisibleItems / 2; i++)
        {
            if (Entry->Blink != &List->ListHead)
                Entry = Entry->Blink;
        }

        ListUi->FirstShown = Entry;

        for (i = 0; i < MaxVisibleItems; i++)
        {
            if (Entry->Flink != &List->ListHead)
                Entry = Entry->Flink;
        }

        ListUi->LastShown = Entry;
    }
}

VOID
DrawGenericList(
    IN PGENERIC_LIST_UI ListUi,
    IN SHORT Left,
    IN SHORT Top,
    IN SHORT Right,
    IN SHORT Bottom)
{
    PGENERIC_LIST List = ListUi->List;

    ListUi->FirstShown = List->ListHead.Flink;
    ListUi->Left = Left;
    ListUi->Top = Top;
    ListUi->Right = Right;
    ListUi->Bottom = Bottom;

    DrawListFrame(ListUi);

    if (IsListEmpty(&List->ListHead))
        return;

    CenterCurrentListItem(ListUi);

    DrawListEntries(ListUi);
    DrawScrollBarGenericList(ListUi);
}

VOID
DrawGenericListCurrentItem(
    IN PGENERIC_LIST List,
    IN PGET_ENTRY_DESCRIPTION GetEntryDescriptionProc,
    IN SHORT Left,
    IN SHORT Top)
{
    CHAR CurrentItemText[256];

    if (GetEntryDescriptionProc &&
        GetNumberOfListEntries(List) > 0)
    {
        GetEntryDescriptionProc(GetCurrentListEntry(List),
                                CurrentItemText,
                                ARRAYSIZE(CurrentItemText));
        CONSOLE_SetTextXY(Left, Top, CurrentItemText);
    }
    else
    {
        CONSOLE_SetTextXY(Left, Top, "");
    }
}

VOID
ScrollDownGenericList(
    IN PGENERIC_LIST_UI ListUi)
{
    PGENERIC_LIST List = ListUi->List;
    PLIST_ENTRY Entry;

    if (List->CurrentEntry == NULL)
        return;

    if (List->CurrentEntry->Entry.Flink != &List->ListHead)
    {
        Entry = List->CurrentEntry->Entry.Flink;
        if (ListUi->LastShown == &List->CurrentEntry->Entry)
        {
            ListUi->FirstShown = ListUi->FirstShown->Flink;
            ListUi->LastShown = ListUi->LastShown->Flink;
        }
        List->CurrentEntry = CONTAINING_RECORD(Entry, GENERIC_LIST_ENTRY, Entry);

        if (ListUi->Redraw)
        {
            DrawListEntries(ListUi);
            DrawScrollBarGenericList(ListUi);
        }
    }
}

VOID
ScrollUpGenericList(
    IN PGENERIC_LIST_UI ListUi)
{
    PGENERIC_LIST List = ListUi->List;
    PLIST_ENTRY Entry;

    if (List->CurrentEntry == NULL)
        return;

    if (List->CurrentEntry->Entry.Blink != &List->ListHead)
    {
        Entry = List->CurrentEntry->Entry.Blink;
        if (ListUi->FirstShown == &List->CurrentEntry->Entry)
        {
            ListUi->FirstShown = ListUi->FirstShown->Blink;
            ListUi->LastShown = ListUi->LastShown->Blink;
        }
        List->CurrentEntry = CONTAINING_RECORD(Entry, GENERIC_LIST_ENTRY, Entry);

        if (ListUi->Redraw)
        {
            DrawListEntries(ListUi);
            DrawScrollBarGenericList(ListUi);
        }
    }
}

VOID
ScrollPageDownGenericList(
    IN PGENERIC_LIST_UI ListUi)
{
    SHORT i;

    /* Suspend auto-redraw */
    ListUi->Redraw = FALSE;

    for (i = ListUi->Top + 1; i < ListUi->Bottom - 1; i++)
    {
        ScrollDownGenericList(ListUi);
    }

    /* Update user interface */
    DrawListEntries(ListUi);
    DrawScrollBarGenericList(ListUi);

    /* Re enable auto-redraw */
    ListUi->Redraw = TRUE;
}

VOID
ScrollPageUpGenericList(
    IN PGENERIC_LIST_UI ListUi)
{
    SHORT i;

    /* Suspend auto-redraw */
    ListUi->Redraw = FALSE;

    for (i = ListUi->Bottom - 1; i > ListUi->Top + 1; i--)
    {
        ScrollUpGenericList(ListUi);
    }

    /* Update user interface */
    DrawListEntries(ListUi);
    DrawScrollBarGenericList(ListUi);

    /* Re enable auto-redraw */
    ListUi->Redraw = TRUE;
}

VOID
ScrollToPositionGenericList(
    IN PGENERIC_LIST_UI ListUi,
    IN ULONG uIndex)
{
    PGENERIC_LIST List = ListUi->List;
    PLIST_ENTRY Entry;
    ULONG uCount = 0;

    if (List->CurrentEntry == NULL || uIndex == 0)
        return;

    do
    {
        if (List->CurrentEntry->Entry.Flink != &List->ListHead)
        {
            Entry = List->CurrentEntry->Entry.Flink;
            if (ListUi->LastShown == &List->CurrentEntry->Entry)
            {
                ListUi->FirstShown = ListUi->FirstShown->Flink;
                ListUi->LastShown = ListUi->LastShown->Flink;
            }
            List->CurrentEntry = CONTAINING_RECORD(Entry, GENERIC_LIST_ENTRY, Entry);
        }
        uCount++;
    }
    while (uIndex != uCount);

    if (ListUi->Redraw)
    {
        DrawListEntries(ListUi);
        DrawScrollBarGenericList(ListUi);
    }
}

VOID
RedrawGenericList(
    IN PGENERIC_LIST_UI ListUi)
{
    if (ListUi->List->CurrentEntry == NULL)
        return;

    if (ListUi->Redraw)
    {
        DrawListEntries(ListUi);
        DrawScrollBarGenericList(ListUi);
    }
}

VOID
GenericListKeyPress(
    IN PGENERIC_LIST_UI ListUi,
    IN CHAR AsciiChar)
{
    PGENERIC_LIST List = ListUi->List;
    PGENERIC_LIST_ENTRY ListEntry;
    PGENERIC_LIST_ENTRY OldListEntry;
    BOOLEAN Flag = FALSE;

    ListEntry = List->CurrentEntry;
    OldListEntry = List->CurrentEntry;

    ListUi->Redraw = FALSE;

    ListUi->CurrentItemText[0] = ANSI_NULL;
    if (ListUi->GetEntryDescriptionProc)
    {
        ListUi->GetEntryDescriptionProc(ListEntry,
                                        ListUi->CurrentItemText,
                                        ARRAYSIZE(ListUi->CurrentItemText));
    }

    if ((strlen(ListUi->CurrentItemText) > 0) && (tolower(ListUi->CurrentItemText[0]) == AsciiChar) &&
         (List->CurrentEntry->Entry.Flink != &List->ListHead))
    {
        ScrollDownGenericList(ListUi);
        ListEntry = List->CurrentEntry;

        ListUi->CurrentItemText[0] = ANSI_NULL;
        if (ListUi->GetEntryDescriptionProc)
        {
            ListUi->GetEntryDescriptionProc(ListEntry,
                                            ListUi->CurrentItemText,
                                            ARRAYSIZE(ListUi->CurrentItemText));
        }

        if ((strlen(ListUi->CurrentItemText) > 0) && (tolower(ListUi->CurrentItemText[0]) == AsciiChar))
            goto End;
    }

    while (List->CurrentEntry->Entry.Blink != &List->ListHead)
        ScrollUpGenericList(ListUi);

    ListEntry = List->CurrentEntry;

    for (;;)
    {
        ListUi->CurrentItemText[0] = ANSI_NULL;
        if (ListUi->GetEntryDescriptionProc)
        {
            ListUi->GetEntryDescriptionProc(ListEntry,
                                            ListUi->CurrentItemText,
                                            ARRAYSIZE(ListUi->CurrentItemText));
        }

        if ((strlen(ListUi->CurrentItemText) > 0) && (tolower(ListUi->CurrentItemText[0]) == AsciiChar))
        {
            Flag = TRUE;
            break;
        }

        if (List->CurrentEntry->Entry.Flink == &List->ListHead)
            break;

        ScrollDownGenericList(ListUi);
        ListEntry = List->CurrentEntry;
    }

    if (!Flag)
    {
        while (List->CurrentEntry->Entry.Blink != &List->ListHead)
        {
            if (List->CurrentEntry != OldListEntry)
                ScrollUpGenericList(ListUi);
            else
                break;
        }
    }

End:
    DrawListEntries(ListUi);
    DrawScrollBarGenericList(ListUi);

    ListUi->Redraw = TRUE;
}

/* EOF */
