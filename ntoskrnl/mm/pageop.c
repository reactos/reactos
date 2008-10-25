/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/pageop.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializePageOp)
#endif


/* GLOBALS *******************************************************************/

#define PAGEOP_HASH_TABLE_SIZE       (32)

static KSPIN_LOCK MmPageOpHashTableLock;
static PMM_PAGEOP MmPageOpHashTable[PAGEOP_HASH_TABLE_SIZE];
static NPAGED_LOOKASIDE_LIST MmPageOpLookasideList;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
MmReleasePageOp(PMM_PAGEOP PageOp)
/*
 * FUNCTION: Release a reference to a page operation descriptor
 */
{
   KIRQL oldIrql;
   PMM_PAGEOP PrevPageOp;

   KeAcquireSpinLock(&MmPageOpHashTableLock, &oldIrql);
   PageOp->ReferenceCount--;
   if (PageOp->ReferenceCount > 0)
   {
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      return;
   }
   (void)InterlockedDecrementUL(&PageOp->MArea->PageOpCount);
   PrevPageOp = MmPageOpHashTable[PageOp->Hash];
   if (PrevPageOp == PageOp)
   {
      MmPageOpHashTable[PageOp->Hash] = PageOp->Next;
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      ExFreeToNPagedLookasideList(&MmPageOpLookasideList, PageOp);
      return;
   }
   while (PrevPageOp->Next != NULL)
   {
      if (PrevPageOp->Next == PageOp)
      {
         PrevPageOp->Next = PageOp->Next;
         KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
         ExFreeToNPagedLookasideList(&MmPageOpLookasideList, PageOp);
         return;
      }
      PrevPageOp = PrevPageOp->Next;
   }
   KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
   ASSERT(FALSE);
}

PMM_PAGEOP
NTAPI
MmCheckForPageOp(PMEMORY_AREA MArea, HANDLE Pid, PVOID Address,
                 PMM_SECTION_SEGMENT Segment, ULONG Offset)
{
   ULONG_PTR Hash;
   KIRQL oldIrql;
   PMM_PAGEOP PageOp;

   /*
    * Calcuate the hash value for pageop structure
    */
   if (MArea->Type == MEMORY_AREA_SECTION_VIEW)
   {
      Hash = (((ULONG_PTR)Segment) | (((ULONG_PTR)Offset) / PAGE_SIZE));
   }
   else
   {
      Hash = (((ULONG_PTR)Pid) | (((ULONG_PTR)Address) / PAGE_SIZE));
   }
   Hash = Hash % PAGEOP_HASH_TABLE_SIZE;

   KeAcquireSpinLock(&MmPageOpHashTableLock, &oldIrql);

   /*
    * Check for an existing pageop structure
    */
   PageOp = MmPageOpHashTable[Hash];
   while (PageOp != NULL)
   {
      if (MArea->Type == MEMORY_AREA_SECTION_VIEW)
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
   KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
   return(NULL);
}

PMM_PAGEOP
NTAPI
MmGetPageOp(PMEMORY_AREA MArea, HANDLE Pid, PVOID Address,
            PMM_SECTION_SEGMENT Segment, ULONG Offset, ULONG OpType, BOOLEAN First)
/*
 * FUNCTION: Get a page operation descriptor corresponding to
 * the memory area and either the segment, offset pair or the
 * pid, address pair.
 */
{
   ULONG_PTR Hash;
   KIRQL oldIrql;
   PMM_PAGEOP PageOp;

   /*
    * Calcuate the hash value for pageop structure
    */
   if (MArea->Type == MEMORY_AREA_SECTION_VIEW)
   {
      Hash = (((ULONG_PTR)Segment) | (((ULONG_PTR)Offset) / PAGE_SIZE));
   }
   else
   {
      Hash = (((ULONG_PTR)Pid) | (((ULONG_PTR)Address) / PAGE_SIZE));
   }
   Hash = Hash % PAGEOP_HASH_TABLE_SIZE;

   KeAcquireSpinLock(&MmPageOpHashTableLock, &oldIrql);

   /*
    * Check for an existing pageop structure
    */
   PageOp = MmPageOpHashTable[Hash];
   while (PageOp != NULL)
   {
      if (MArea->Type == MEMORY_AREA_SECTION_VIEW)
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
      if (First)
      {
         PageOp = NULL;
      }
      else
      {
         PageOp->ReferenceCount++;
      }
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      return(PageOp);
   }

   /*
    * Otherwise add a new pageop.
    */
   PageOp = ExAllocateFromNPagedLookasideList(&MmPageOpLookasideList);
   if (PageOp == NULL)
   {
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      ASSERT(FALSE);
      return(NULL);
   }

   if (MArea->Type != MEMORY_AREA_SECTION_VIEW)
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
   PageOp->MArea = MArea;
   KeInitializeEvent(&PageOp->CompletionEvent, NotificationEvent, FALSE);
   MmPageOpHashTable[Hash] = PageOp;
   (void)InterlockedIncrementUL(&MArea->PageOpCount);

   KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
   return(PageOp);
}

VOID
INIT_FUNCTION
NTAPI
MmInitializePageOp(VOID)
{
   memset(MmPageOpHashTable, 0, sizeof(MmPageOpHashTable));
   KeInitializeSpinLock(&MmPageOpHashTableLock);

   ExInitializeNPagedLookasideList (&MmPageOpLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(MM_PAGEOP),
                                    TAG_MM_PAGEOP,
                                    50);
}









