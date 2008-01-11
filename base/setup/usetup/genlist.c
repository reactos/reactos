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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/genlist.c
 * PURPOSE:         Generic list functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

PGENERIC_LIST
CreateGenericList(VOID)
{
  PGENERIC_LIST List;

  List = (PGENERIC_LIST)RtlAllocateHeap(ProcessHeap,
					0,
					sizeof(GENERIC_LIST));
  if (List == NULL)
    return NULL;

  InitializeListHead(&List->ListHead);

  List->Left = 0;
  List->Top = 0;
  List->Right = 0;
  List->Bottom = 0;

  List->CurrentEntry = NULL;

  return List;
}


VOID
DestroyGenericList(PGENERIC_LIST List,
		   BOOLEAN FreeUserData)
{
  PGENERIC_LIST_ENTRY ListEntry;
  PLIST_ENTRY Entry;

  /* Release list entries */
  while (!IsListEmpty (&List->ListHead))
    {
      Entry = RemoveHeadList (&List->ListHead);
      ListEntry = CONTAINING_RECORD (Entry, GENERIC_LIST_ENTRY, Entry);

      /* Release user data */
      if (FreeUserData && ListEntry->UserData != NULL)
      RtlFreeHeap (ProcessHeap, 0, ListEntry->UserData);

      /* Release list entry */
      RtlFreeHeap (ProcessHeap, 0, ListEntry);
    }

  /* Release list head */
  RtlFreeHeap (ProcessHeap, 0, List);
}


BOOLEAN
AppendGenericListEntry(PGENERIC_LIST List,
		       PCHAR Text,
		       PVOID UserData,
		       BOOLEAN Current)
{
  PGENERIC_LIST_ENTRY Entry;

  Entry = (PGENERIC_LIST_ENTRY)RtlAllocateHeap(ProcessHeap,
					       0,
					       sizeof(GENERIC_LIST_ENTRY) + strlen(Text));
  if (Entry == NULL)
    return FALSE;

  strcpy (Entry->Text, Text);
  Entry->UserData = UserData;

  InsertTailList(&List->ListHead,
		 &Entry->Entry);

  if (Current || List->CurrentEntry == NULL)
    {
      List->CurrentEntry = Entry;
    }

  return TRUE;
}


static VOID
DrawListFrame(PGENERIC_LIST GenericList)
{
  COORD coPos;
  DWORD Written;
  SHORT i;

  /* Draw upper left corner */
  coPos.X = GenericList->Left;
  coPos.Y = GenericList->Top;
  FillConsoleOutputCharacterA (StdOutput,
			      0xDA, // '+',
			      1,
			      coPos,
			      &Written);

  /* Draw upper edge */
  coPos.X = GenericList->Left + 1;
  coPos.Y = GenericList->Top;
  FillConsoleOutputCharacterA (StdOutput,
			      0xC4, // '-',
			      GenericList->Right - GenericList->Left - 1,
			      coPos,
			      &Written);

  /* Draw upper right corner */
  coPos.X = GenericList->Right;
  coPos.Y = GenericList->Top;
  FillConsoleOutputCharacterA (StdOutput,
			      0xBF, // '+',
			      1,
			      coPos,
			      &Written);

  /* Draw left and right edge */
  for (i = GenericList->Top + 1; i < GenericList->Bottom; i++)
    {
      coPos.X = GenericList->Left;
      coPos.Y = i;
      FillConsoleOutputCharacterA (StdOutput,
				  0xB3, // '|',
				  1,
				  coPos,
				  &Written);

      coPos.X = GenericList->Right;
      FillConsoleOutputCharacterA (StdOutput,
				  0xB3, //'|',
				  1,
				  coPos,
				  &Written);
    }

  /* Draw lower left corner */
  coPos.X = GenericList->Left;
  coPos.Y = GenericList->Bottom;
  FillConsoleOutputCharacterA (StdOutput,
			      0xC0, // '+',
			      1,
			      coPos,
			      &Written);

  /* Draw lower edge */
  coPos.X = GenericList->Left + 1;
  coPos.Y = GenericList->Bottom;
  FillConsoleOutputCharacterA (StdOutput,
			      0xC4, // '-',
			      GenericList->Right - GenericList->Left - 1,
			      coPos,
			      &Written);

  /* Draw lower right corner */
  coPos.X = GenericList->Right;
  coPos.Y = GenericList->Bottom;
  FillConsoleOutputCharacterA (StdOutput,
			      0xD9, // '+',
			      1,
			      coPos,
			      &Written);
}


static VOID
DrawListEntries(PGENERIC_LIST GenericList)
{
  PGENERIC_LIST_ENTRY ListEntry;
  PLIST_ENTRY Entry;
  COORD coPos;
  DWORD Written;
  USHORT Width;

  coPos.X = GenericList->Left + 1;
  coPos.Y = GenericList->Top + 1;
  Width = GenericList->Right - GenericList->Left - 1;

  Entry = GenericList->ListHead.Flink;
  while (Entry != &GenericList->ListHead)
    {
      ListEntry = CONTAINING_RECORD (Entry, GENERIC_LIST_ENTRY, Entry);

      if (coPos.Y == GenericList->Bottom)
	break;

      FillConsoleOutputAttribute (StdOutput,
				  (GenericList->CurrentEntry == ListEntry) ?
				      FOREGROUND_BLUE | BACKGROUND_WHITE :
				      FOREGROUND_WHITE | BACKGROUND_BLUE,
				  Width,
				  coPos,
				  &Written);

      FillConsoleOutputCharacterA (StdOutput,
				  ' ',
				  Width,
				  coPos,
				  &Written);

      coPos.X++;
      WriteConsoleOutputCharacterA (StdOutput,
				    ListEntry->Text,
				    min (strlen(ListEntry->Text), (SIZE_T)Width - 2),
				    coPos,
				    &Written);
      coPos.X--;

      coPos.Y++;
      Entry = Entry->Flink;
    }

  while (coPos.Y < GenericList->Bottom)
    {
      FillConsoleOutputAttribute (StdOutput,
				  FOREGROUND_WHITE | BACKGROUND_BLUE,
				  Width,
				  coPos,
				  &Written);

      FillConsoleOutputCharacterA (StdOutput,
				' ',
				  Width,
				  coPos,
				  &Written);
      coPos.Y++;
    }
}


VOID
DrawGenericList(PGENERIC_LIST List,
		SHORT Left,
		SHORT Top,
		SHORT Right,
		SHORT Bottom)
{
  List->Left = Left;
  List->Top = Top;
  List->Right = Right;
  List->Bottom = Bottom;

  DrawListFrame(List);

  if (IsListEmpty(&List->ListHead))
    return;

  DrawListEntries(List);
}


VOID
ScrollDownGenericList (PGENERIC_LIST List)
{
  PLIST_ENTRY Entry;

  if (List->CurrentEntry == NULL)
    return;

  if (List->CurrentEntry->Entry.Flink != &List->ListHead)
    {
      Entry = List->CurrentEntry->Entry.Flink;
      List->CurrentEntry = CONTAINING_RECORD (Entry, GENERIC_LIST_ENTRY, Entry);
      DrawListEntries(List);
    }
}


VOID
ScrollUpGenericList (PGENERIC_LIST List)
{
  PLIST_ENTRY Entry;

  if (List->CurrentEntry == NULL)
    return;

  if (List->CurrentEntry->Entry.Blink != &List->ListHead)
    {
      Entry = List->CurrentEntry->Entry.Blink;
      List->CurrentEntry = CONTAINING_RECORD (Entry, GENERIC_LIST_ENTRY, Entry);
      DrawListEntries(List);
    }
}


PGENERIC_LIST_ENTRY
GetGenericListEntry(PGENERIC_LIST List)
{
  return List->CurrentEntry;
}


VOID
SaveGenericListState(PGENERIC_LIST List)
{
  List->BackupEntry = List->CurrentEntry;
}


VOID
RestoreGenericListState(PGENERIC_LIST List)
{
  List->CurrentEntry = List->BackupEntry;
}

/* EOF */
