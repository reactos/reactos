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
/* $Id: rdebug.c,v 1.1 2001/05/05 19:13:09 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/rdebug.c
 * PURPOSE:         Runtime debugging support
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      01-05-2001  CSH  Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

typedef struct _RDEBUG_ENTRY {
  LIST_ENTRY ListEntry;
  CHAR Filename[MAX_PATH];
} RDEBUG_ENTRY, *PRDEBUG_ENTRY;

LIST_ENTRY RDebugListHead;
BOOLEAN RDebugInitialized = FALSE;

/* FUNCTIONS ****************************************************************/

PRDEBUG_ENTRY
DbgpFind(PCH Filename)
{
  PLIST_ENTRY Current;
  PRDEBUG_ENTRY Entry;

  Current = RDebugListHead.Flink;
  while (Current != &RDebugListHead)
    {
      Entry = CONTAINING_RECORD(Current, RDEBUG_ENTRY, ListEntry);

      if (strcmp(Filename, Entry->Filename) == 0)
        {
          return Entry;
        }
      Current = Current->Flink;
    }

  return(NULL);
}

VOID
DbgRDebugInit(VOID)
{
  if (RDebugInitialized)
    return;

  InitializeListHead(&RDebugListHead);
  RDebugInitialized = TRUE;
}

VOID
DbgShowFiles(VOID)
{
  PLIST_ENTRY Current;
  PRDEBUG_ENTRY Entry;
  ULONG Count;

  if (!RDebugInitialized)
    return;

  Count = 0;
  Current = RDebugListHead.Flink;
  while (Current != &RDebugListHead)
    {
      Entry = CONTAINING_RECORD(Current, RDEBUG_ENTRY, ListEntry);

      DbgPrint("  %s\n", Entry->Filename);
      Count++;

      Current = Current->Flink;
    }

  if (Count == 1)
    {
      DbgPrint("  1 file listed\n");
    }
  else
    {
      DbgPrint("  %d files listed\n", Count);
    }
}

VOID
DbgEnableFile(PCH Filename)
{
  PRDEBUG_ENTRY Entry;

  if (!RDebugInitialized)
    return;

  if (!DbgpFind(Filename))
    {
      Entry = ExAllocatePool(NonPagedPool, sizeof(RDEBUG_ENTRY));
      assert(Entry);
      RtlMoveMemory(Entry->Filename, Filename, strlen(Filename) + 1);
      InsertTailList(&RDebugListHead, &Entry->ListEntry);
    }
}

VOID
DbgDisableFile(PCH Filename)
{
  PRDEBUG_ENTRY Entry;

  if (!RDebugInitialized)
    return;

  Entry = DbgpFind(Filename);
  
  if (Entry)
    {
      RemoveEntryList(&Entry->ListEntry);
    }
}

BOOLEAN
DbgShouldPrint(PCH Filename)
{
  if (!RDebugInitialized)
    return FALSE;

  return(DbgpFind(Filename) != NULL);
}

/* EOF */
