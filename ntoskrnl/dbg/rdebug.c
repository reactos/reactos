/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/rdebug.c
 * PURPOSE:         Runtime debugging support
 * 
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

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

VOID INIT_FUNCTION
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
      ASSERT(Entry);
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
