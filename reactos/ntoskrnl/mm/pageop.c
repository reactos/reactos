/* $Id: pageop.c,v 1.8 2002/05/13 18:10:40 chorns Exp $
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
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define PAGEOP_HASH_TABLE_SIZE       (32)

KSPIN_LOCK MmPageOpHashTableLock;
PMM_PAGEOP MmPageOpHashTable[PAGEOP_HASH_TABLE_SIZE] = {NULL, } ;

#define TAG_MM_PAGEOP   TAG('M', 'P', 'O', 'P')

/* FUNCTIONS *****************************************************************/

#ifdef DBG

VOID
MiValidatePageOp(IN PMM_PAGEOP  PageOp)
{
  if (PageOp != NULL)
    {
	    DPRINT("MiValidatePageOp(PageOp 0x%.08x)\n", PageOp);

			assertmsg(PageOp->Magic == TAG_MM_PAGEOP,
		    ("Bad PageOp (0x%.08x). Wrong magic (0x%.08x)\n", PageOp->Magic));

			assertmsg((PageOp->OpType >= MM_PAGEOP_MINIMUM) && (PageOp->OpType <= MM_PAGEOP_MAXIMUM),
		    ("Bad PageOp (0x%.08x). Wrong type (%d)\n", PageOp->OpType));
		}
}

#endif /* DBG */

VOID
MmReleasePageOp(IN PMM_PAGEOP  PageOp)
     /*
      * FUNCTION: Release a reference to a page operation descriptor
      */
{
  KIRQL oldIrql;
  PMM_PAGEOP PrevPageOp;

  assert(PageOp);
  VALIDATE_PAGEOP(PageOp);

  KeAcquireSpinLock(&MmPageOpHashTableLock, &oldIrql);
  PageOp->ReferenceCount--;
  if (PageOp->ReferenceCount > 0)
    {
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      return;
    }
  PrevPageOp = MmPageOpHashTable[PageOp->Hash];
  if (PrevPageOp == PageOp)
    {
      MmPageOpHashTable[PageOp->Hash] = PageOp->Next;
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      ExFreePool(PageOp);
      return;
    }
  while (PrevPageOp->Next != NULL)
    {
      if (PrevPageOp->Next == PageOp)
	{
	  PrevPageOp->Next = PageOp->Next;
	  KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
	  ExFreePool(PageOp);
	  return;
	}
      PrevPageOp = PrevPageOp->Next;
    }
  KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
  CPRINT("PageOp (0x%.08x) not found\n", PageOp);
  KeBugCheck(0);
}


PMM_PAGEOP
MmGetPageOp(IN PMEMORY_AREA  MArea,
  IN ULONG  Pid,
  IN PVOID  Address,
  IN PMM_SECTION_SEGMENT  Segment,
  IN ULONG  Offset,
  IN ULONG  OpType)
/*
 * FUNCTION: Get a page operation descriptor corresponding to
 * the memory area and either the segment, offset pair or the
 * pid, address pair. Create a new pageop if one does not exist.
 * FIXME: Make Offset 64-bit
 */
{
  ULONG Hash;
  KIRQL oldIrql;
  PMM_PAGEOP PageOp;

  /*
   * Calculate the hash value for pageop structure
   */
  if (MArea->Type == MEMORY_AREA_SECTION_VIEW_COMMIT)
    {
      Hash = (((ULONG)Segment) | (((ULONG)Offset) / PAGESIZE));
    }
  else
    {
      assertmsg(((ULONG_PTR)Address % PAGESIZE) == 0,
        ("Address must be page aligned (0x%.08x)\n", Address));

      Hash = (((ULONG)Pid) | (((ULONG)Address) / PAGESIZE));
    }
  Hash = Hash % PAGEOP_HASH_TABLE_SIZE;

  KeAcquireSpinLock(&MmPageOpHashTableLock, &oldIrql);

  /*
   * Check for an existing pageop structure
   */
  PageOp = MmPageOpHashTable[Hash];
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
      VALIDATE_PAGEOP(PageOp);

      PageOp->ReferenceCount++;
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      return(PageOp);
    }

  /*
   * Otherwise add a new pageop.
   */
  PageOp = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_PAGEOP), 
				 TAG_MM_PAGEOP);
  if (PageOp == NULL)
    {
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      return(NULL);
    }
  
  SET_MAGIC(PageOp, TAG_MM_PAGEOP);

  if (MArea->Type != MEMORY_AREA_SECTION_VIEW_COMMIT)
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
  PageOp->Next = MmPageOpHashTable[Hash];
  PageOp->Hash = Hash;
  PageOp->Thread = PsGetCurrentThread();
  PageOp->Abandoned = FALSE;
  PageOp->Status = STATUS_PENDING;
  PageOp->OpType = OpType;
  KeInitializeEvent(&PageOp->CompletionEvent, NotificationEvent, FALSE);
  MmPageOpHashTable[Hash] = PageOp;

  VALIDATE_PAGEOP(PageOp);

  KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
  return(PageOp);
}


PMM_PAGEOP
MmGotPageOp(IN PMEMORY_AREA  MArea,
  IN ULONG  Pid,
  IN PVOID  Address,
  IN PMM_SECTION_SEGMENT  Segment,
  IN ULONG  Offset)
/*
 * FUNCTION: Get a page operation descriptor corresponding to
 * the memory area and either the segment, offset pair or the
 * pid, address pair. Returns NULL if a pageop does not exist.
 * FIXME: Make Offset 64-bit
 */
{
  ULONG Hash;
  KIRQL oldIrql;
  PMM_PAGEOP PageOp;

  /*
   * Calculate the hash value for pageop structure
   */
  if (MArea->Type == MEMORY_AREA_SECTION_VIEW_COMMIT)
    {
      Hash = (((ULONG)Segment) | (((ULONG)Offset) / PAGESIZE));
    }
  else
    {
      assertmsg(((ULONG_PTR)Address % PAGESIZE) == 0,
        ("Address must be page aligned (0x%.08x)\n", Address));

      Hash = (((ULONG)Pid) | (((ULONG)Address) / PAGESIZE));
    }
  Hash = Hash % PAGEOP_HASH_TABLE_SIZE;

  KeAcquireSpinLock(&MmPageOpHashTableLock, &oldIrql);

  /*
   * Check for an existing pageop structure
   */
  PageOp = MmPageOpHashTable[Hash];
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
      VALIDATE_PAGEOP(PageOp);

      PageOp->ReferenceCount++;
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      return(PageOp);
    }

  KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);

  /*
   * Otherwise return NULL.
   */
  return(NULL);
}
