/* $Id: pageop.c,v 1.1 2001/02/16 18:32:20 dwelch Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/pageop.c
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY: 
 *               27/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <internal/mm.h>
#include <internal/mmhal.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define PAGEOP_HASH_TABLE_SIZE       (32)

KSPIN_LOCK MmPageOpHashTableLock;
PMM_PAGEOP MmPageOpHashTable[PAGEOP_HASH_TABLE_SIZE];

/* FUNCTIONS *****************************************************************/

VOID
MmReleasePageOp(PMM_PAGEOP PageOp)
{
  ULONG h;
  KIRQL oldIrql;
  PMM_PAGEOP PPageOp;

  PageOp->ReferenceCount--;
  if (PageOp->ReferenceCount > 0)
    {
      return;
    }

  /*
   * Calcuate the hash value for pageop structure
   */
  if (MArea->Type == MEMORY_AREA_SECTION_VIEW_COMMIT)
    {
      h = (((ULONG)Segment) | Offset) % PAGEOP_HASH_TABLE_SIZE;
    }
  else
    {
      h = (((ULONG)Pid) | (ULONG)Address) % PAGEOP_HASH_TABLE_SIZE;
    }

  KeAcquireSpinLock(&MmPageOpHashTableLock, &oldIrql);
  PPageOp = MmPageOpHashTable[h];
  if (PPageOp == PageOp)
    {
      MmPageOpHashTable[h] = PageOp->Next;
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      ExFreePool(PageOp);
      return;
    }
  PPageOp = PPageOp->Next;
  while (PPageOp != NULL)
    {
      if (PPageOp == PageOp)
	{
	  PPageOp->Next = PageOp->Next;
	  KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
	  ExFreePool(PageOp);
	  return;
	}
      PPageOp = PPageOp->Next;
    }
  KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
  KeBugCheck(0);
}

PMM_PAGEOP
MmGetPageOp(PMEMORY_AREA MArea, ULONG Pid, PVOID Address,
	    PMM_SECTION_SEGMENT Segment, ULONG Offset)
{
  ULONG h;
  KIRQL oldIrql;
  PMM_PAGEOP PageOp;

  /*
   * Calcuate the hash value for pageop structure
   */
  if (MArea->Type == MEMORY_AREA_SECTION_VIEW_COMMIT)
    {
      h = (((ULONG)Segment) | Offset) % PAGEOP_HASH_TABLE_SIZE;
    }
  else
    {
      h = (((ULONG)Pid) | (ULONG)Address) % PAGEOP_HASH_TABLE_SIZE;
    }

  KeAcquireSpinLock(&MmPageOpHashTableLock, &oldIrql);

  /*
   * Check for an existing pageop structure
   */
  PageOp = MmPageOpHashTable[h];
  while (PageOp != NULL)
    {
      if (MArea->Type == MEMORY_AREA_SECTION_VIEW_COMMIT)
	{
	  if (PageOp->Segment == Segment &&
	      PageOp->Offset == Offset)
	    {
	      break;
	    }
	}
      else
	{
	  if (PageOp->Pid == Pid &&
	      PageOp->Address == Address)
	    {
	      break;
	    }
	}
      PageOp = PageOp->Next;
    }
  
  /*
   * If we found an existing pageop then increment the reference count
   * and return it.
   */
  if (PageOp != NULL)
    {
      PageOp->ReferenceCount++;
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      return(PageOp);
    }

  /*
   * Otherwise add a new pageop.
   */
  PageOp = ExAllocatePool(NonPagedPool, sizeof(MM_PAGEOP));
  if (PageOp == NULL)
    {
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      return(NULL);
    }
  
  if (MArea->Type == MEMORY_AREA_SECTION_VIEW_COMMIT)
    {
      PageOp->Pid = Pid;
      PageOp->Address = Address;
    }
  else
    {
      PageOp->Segment = Segment;
      PageOp->Offset = Offset;
    }
  PageOp->ReferenceCount = 1;
  PageOp->Next = MmPageOpHashTable[h];
  MmPageOpHashTable[h] = PageOp;

  KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
  return(PageOp);
}
