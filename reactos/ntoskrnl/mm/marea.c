/*
 *  ReactOS kernel
 *  Copyright (C) 1998-2002 ReactOS Team
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
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/marea.c
 * PURPOSE:         Implements memory areas
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <roscfg.h>
#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_MAREA   TAG('M', 'A', 'R', 'E')

#ifdef DBG
PVOID MiMemoryAreaBugCheckAddress = (PVOID) NULL;
#endif /* DBG */

/* Define to track memory area references */
//#define TRACK_MEMORY_AREA_REFERENCES

/* FUNCTIONS *****************************************************************/

VOID MmDumpMemoryAreas(PLIST_ENTRY ListHead)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   
   DbgPrint("MmDumpMemoryAreas()\n");
   
   current_entry = ListHead->Flink;
   while (current_entry!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	DbgPrint("Base %x Length %x End %x Attributes %x Flink %x\n",
	       current->BaseAddress,current->Length,
	       current->BaseAddress+current->Length,current->Attributes,
	       current->Entry.Flink);
	current_entry = current_entry->Flink;
     }
   DbgPrint("Finished MmDumpMemoryAreas()\n");
}

#ifdef DBG

VOID
MiValidateMemoryAreaPTEs(IN PMEMORY_AREA  MemoryArea)
{
	ULONG PteProtect;
	ULONG i;

	if (!MiInitialized)
		return;

	for (i = 0; i <= (MemoryArea->Length / PAGESIZE); i++)
		{
			if (MmIsPagePresent(MemoryArea->Process, MemoryArea->BaseAddress + (i * PAGESIZE)))
				{
					PteProtect = MmGetPageProtect(MemoryArea->Process, MemoryArea->BaseAddress + (i * PAGESIZE));
					if (PteProtect != MemoryArea->Attributes)
						{
							if (MmIsCopyOnWriteMemoryArea(MemoryArea))
								{
									if ((PteProtect != PAGE_READONLY) && (PteProtect != PAGE_EXECUTE_READ))
										{
											DPRINT1("COW memory area attributes 0x%.08x\n", MemoryArea->Attributes);
											DbgMmDumpProtection(MemoryArea->Attributes);
											DPRINT1("PTE attributes 0x%.08x\n", PteProtect);
											DbgMmDumpProtection(PteProtect);
											assertmsg(FALSE, ("PTE attributes and memory area protection are different. Area 0x%.08x\n",
												MemoryArea->BaseAddress));
										}
								}
								else
								{
									DPRINT1("Memory area attributes 0x%.08x\n", MemoryArea->Attributes);
									DbgMmDumpProtection(MemoryArea->Attributes);
									DPRINT1("PTE attributes 0x%.08x\n", PteProtect);
									DbgMmDumpProtection(PteProtect);
									assertmsg(FALSE, ("PTE attributes and memory area protection are different. Area 0x%.08x\n",
										MemoryArea->BaseAddress));
								}
						}
				}
		}
}


VOID
MiValidateMemoryArea(IN PMEMORY_AREA  MemoryArea)
{
  assertmsg(MemoryArea != NULL,
   ("No memory area can exist at 0x%.08x\n", MemoryArea));

  assertmsg(MemoryArea->Magic == TAG_MAREA,
   ("Bad magic (0x%.08x) for memory area (0x%.08x). It should be 0x%.08x\n",
     MemoryArea->Magic, MemoryArea, TAG_MAREA));

	/* FIXME: Can cause page faults and deadlock on the address space lock */
  //MiValidateMemoryAreaPTEs(MemoryArea);
}

#endif /* DBG */

VOID
MmApplyMemoryAreaProtection(IN PMEMORY_AREA  MemoryArea)
{
	ULONG i;
	
	if (!MiInitialized)
		return;

	for (i = 0; i <= (MemoryArea->Length / PAGESIZE); i++)
		{
			if (MmIsPagePresent(MemoryArea->Process, MemoryArea->BaseAddress + (i * PAGESIZE)))
				{
					MmSetPageProtect(MemoryArea->Process,
						MemoryArea->BaseAddress + (i * PAGESIZE),
						MemoryArea->Attributes);
				}
		}
}


/*
 * NOTE: If the memory area is found, then it is referenced. The caller must
 *       call MmCloseMemoryArea() after use.
 */
PMEMORY_AREA
MmOpenMemoryAreaByAddress(IN PMADDRESS_SPACE  AddressSpace,
  IN PVOID  Address)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   PLIST_ENTRY previous_entry;

   DPRINT("MmOpenMemoryAreaByAddress(AddressSpace %x, Address %x)\n",
	   AddressSpace, Address);
	
//   MmDumpMemoryAreas(&AddressSpace->MAreaListHead);

   previous_entry = &AddressSpace->MAreaListHead;
   current_entry = AddressSpace->MAreaListHead.Flink;
   while (current_entry != &AddressSpace->MAreaListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    MEMORY_AREA,
				    Entry);
	DPRINT("Scanning %x BaseAddress %x Length %x\n",
		current, current->BaseAddress, current->Length);
	assert(current_entry->Blink->Flink == current_entry);
	if (current_entry->Flink->Blink != current_entry)
	  {
	     DPRINT1("BaseAddress %x\n", current->BaseAddress);
	     DPRINT1("current_entry->Flink %x ", current_entry->Flink);
	     DPRINT1("&current_entry->Flink %x\n",
		     &current_entry->Flink);
	     DPRINT1("current_entry->Flink->Blink %x\n",
		     current_entry->Flink->Blink);
	     DPRINT1("&current_entry->Flink->Blink %x\n",
		     &current_entry->Flink->Blink);
	     DPRINT1("&current_entry->Flink %x\n",
		     &current_entry->Flink);
	  }
	assert(current_entry->Flink->Blink == current_entry);
	assert(previous_entry->Flink == current_entry);
	if (current->BaseAddress <= Address &&
	    (current->BaseAddress + current->Length) > Address)
	  {
	     DPRINT("%s() = %x\n",__FUNCTION__,current);
       MmReferenceMemoryArea(current);
	     return(current);
	  }
	if (current->BaseAddress > Address)
	  {
	     DPRINT("%s() = NULL\n",__FUNCTION__);
	     return(NULL);
	  }
	previous_entry = current_entry;
	current_entry = current_entry->Flink;
     }
   DPRINT("%s() = NULL\n",__FUNCTION__);
   return(NULL);
}

/*
 * NOTE: If the memory area is found, then it is referenced. The caller must
 *       call MmCloseMemoryArea() after use.
 */
MEMORY_AREA* MmOpenMemoryAreaByRegion(PMADDRESS_SPACE AddressSpace, 
				      PVOID Address,
				      ULONG Length)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   ULONG Extent;
   
   DPRINT("MmOpenMemoryByRegion(AddressSpace %x, Address %x, Length %x)\n",
	  AddressSpace, Address, Length);
   
   current_entry = AddressSpace->MAreaListHead.Flink;
   while (current_entry != &AddressSpace->MAreaListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    MEMORY_AREA,
				    Entry);
	DPRINT("current->BaseAddress %x current->Length %x\n",
	       current->BaseAddress,current->Length);
	if (current->BaseAddress >= Address &&
	    current->BaseAddress < (Address+Length))
	  {
	     DPRINT("Finished MmOpenMemoryAreaByRegion() = %x\n",
		    current);
       MmReferenceMemoryArea(current);
	     return(current);
	  }
	Extent = (ULONG)current->BaseAddress + current->Length;
	if (Extent > (ULONG)Address &&
	    Extent < (ULONG)(Address+Length))
	  {
	     DPRINT("Finished MmOpenMemoryAreaByRegion() = %x\n",
		    current);
       MmReferenceMemoryArea(current);
	     return(current);
	  }
	if (current->BaseAddress <= Address &&
	    Extent >= (ULONG)(Address+Length))
	  {
	     DPRINT("Finished MmOpenMemoryAreaByRegion() = %x\n",
		    current);
       MmReferenceMemoryArea(current);
	     return(current);
	  }
	if (current->BaseAddress >= (Address+Length))
	  {
	     DPRINT("Finished MmOpenMemoryAreaByRegion()= NULL\n",0);
	     return(NULL);
	  }
	current_entry = current_entry->Flink;
     }
   DPRINT("Finished MmOpenMemoryAreaByRegion() = NULL\n",0);
   return(NULL);
}


VOID
MmCloseMemoryArea(IN PMEMORY_AREA  MemoryArea)
{
  MmDereferenceMemoryArea(MemoryArea);
}


static VOID MmInsertMemoryArea(PMADDRESS_SPACE AddressSpace,
			       MEMORY_AREA* marea)
{
   PLIST_ENTRY ListHead;
   PLIST_ENTRY current_entry;
   PLIST_ENTRY inserted_entry = &marea->Entry;
   MEMORY_AREA* current;
   MEMORY_AREA* next;   

   DPRINT("MmInsertMemoryArea(marea %x)\n", marea);
   DPRINT("marea->BaseAddress %x\n", marea->BaseAddress);
   DPRINT("marea->Length %x\n", marea->Length);
   
   ListHead = &AddressSpace->MAreaListHead;
   
   current_entry = ListHead->Flink;
   CHECKPOINT;
   if (IsListEmpty(ListHead))
     {
	CHECKPOINT;
	InsertHeadList(ListHead,&marea->Entry);
	DPRINT("Inserting at list head\n");
	CHECKPOINT;
	return;
     }
   CHECKPOINT;
   current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
   CHECKPOINT;
   if (current->BaseAddress > marea->BaseAddress)
     {
	CHECKPOINT;
	InsertHeadList(ListHead,&marea->Entry);
	DPRINT("Inserting at list head\n");
	CHECKPOINT;
	return;
     }
   CHECKPOINT;
   while (current_entry->Flink!=ListHead)
     {
//	CHECKPOINT;
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	next = CONTAINING_RECORD(current_entry->Flink,MEMORY_AREA,Entry);
//	assert(current->BaseAddress != marea->BaseAddress);	
//	assert(next->BaseAddress != marea->BaseAddress);
	if (current->BaseAddress < marea->BaseAddress &&
	    current->Entry.Flink==ListHead)
	  {
	     DPRINT("Insert after %x\n", current_entry);
	     current_entry->Flink = inserted_entry;
	     inserted_entry->Flink=ListHead;
	     inserted_entry->Blink=current_entry;
	     ListHead->Blink = inserted_entry;	    	     
	     return;
	  }
	if (current->BaseAddress < marea->BaseAddress &&
	    next->BaseAddress > marea->BaseAddress)
	  {
	     DPRINT("Inserting before %x\n", current_entry);
	     inserted_entry->Flink = current_entry->Flink;
	     inserted_entry->Blink = current_entry;
	     inserted_entry->Flink->Blink = inserted_entry;
	     current_entry->Flink=inserted_entry;
	     return;
	  }
	current_entry = current_entry->Flink;
     }
   CHECKPOINT;
   DPRINT("Inserting at list tail\n");
   InsertTailList(ListHead,inserted_entry);
}

static PVOID MmFindGap(PMADDRESS_SPACE AddressSpace,
		       ULONG Length)
{
   PLIST_ENTRY ListHead;
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   MEMORY_AREA* next;
   ULONG Gap;
   
   DPRINT("MmFindGap(Length %x)\n",Length);
   
   ListHead = &AddressSpace->MAreaListHead;
     
   current_entry = ListHead->Flink;
   while (current_entry->Flink!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	next = CONTAINING_RECORD(current_entry->Flink,MEMORY_AREA,Entry);
	DPRINT("current %x current->BaseAddress %x ",current,
	       current->BaseAddress);
	DPRINT("current->Length %x\n",current->Length);
	DPRINT("next %x next->BaseAddress %x ",next,next->BaseAddress);
	Gap = (next->BaseAddress ) -(current->BaseAddress + current->Length);
	DPRINT("Base %x Gap %x\n",current->BaseAddress,Gap);
	if (Gap >= Length)
	  {
	     return(current->BaseAddress + PAGE_ROUND_UP(current->Length));
	  }
	current_entry = current_entry->Flink;
     }
   
   if (current_entry == ListHead)
     {
	return((PVOID)AddressSpace->LowestAddress);
     }
   
   current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
   //DbgPrint("current %x returning %x\n",current,current->BaseAddress+
//	    current->Length);
   return(current->BaseAddress + PAGE_ROUND_UP(current->Length));
}

NTSTATUS MmInitMemoryAreas(VOID)
/*
 * FUNCTION: Initialize the memory area list
 */
{
   DPRINT("MmInitMemoryAreas()\n",0);
   return(STATUS_SUCCESS);
}

/* NOTE: The address space lock must be held when called */
NTSTATUS
MmFreeMemoryArea(IN PMADDRESS_SPACE  AddressSpace,
	IN PVOID  BaseAddress,
	IN ULONG  Length,
	IN PFREE_MEMORY_AREA_PAGE_CALLBACK  FreePage,
	IN PVOID FreePageContext)
{
	MEMORY_AREA* MemoryArea;
	ULONG i;
	
	DPRINT("MmFreeMemoryArea(AddressSpace %x, BaseAddress %x, Length %x, "
	  "FreePageContext %d)\n",AddressSpace,BaseAddress,Length,FreePageContext);
	
  MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, BaseAddress);
  if (MemoryArea == NULL)
    {
      assertmsg(FALSE, ("Freeing non-existant memory area at 0x%.08x\n", BaseAddress));
      return(STATUS_UNSUCCESSFUL);
    }

  MmCloseMemoryArea(MemoryArea);
  InterlockedDecrement(&MemoryArea->ReferenceCount);
#if 0
	assertmsg(MemoryArea->ReferenceCount == 0,
    ("Memory area at address 0x%.08x has %d outstanding references\n",
      BaseAddress, MemoryArea->ReferenceCount));
#endif
   for (i=0; i<(PAGE_ROUND_UP(MemoryArea->Length)/PAGESIZE); i++)
     {
       ULONG_PTR PhysicalPage = 0;
       BOOLEAN Dirty = FALSE;
       SWAPENTRY SwapEntry = 0;
       PVOID VirtualPage = NULL;

       VirtualPage = MemoryArea->BaseAddress + (i * PAGESIZE);

#ifdef DBG
       if ((MiMemoryAreaBugCheckAddress != NULL)
         && ((MiMemoryAreaBugCheckAddress >= VirtualPage)
         && MiMemoryAreaBugCheckAddress < VirtualPage + PAGESIZE))
        {
          assertmsg(FALSE, ("VirtualPage 0x%.08x  MiMemoryAreaBugCheckAddress 0x%.08x \n",
            VirtualPage));
        }
#endif

       if (FreePage != NULL)
	 {
	   FreePage(TRUE, FreePageContext, MemoryArea,
		   VirtualPage, 0, 0, FALSE);
	 }

       if (MmIsPageSwapEntry(AddressSpace->Process, VirtualPage))
	 {
	   MmDeletePageFileMapping(AddressSpace->Process,
				   VirtualPage,
				   &SwapEntry);
	 }
       else
	 {
	   MmDeleteVirtualMapping(AddressSpace->Process, 
				  VirtualPage,
				  FALSE, &Dirty, &PhysicalPage);
	 }
       if (FreePage != NULL)
	 {
	   FreePage(FALSE, FreePageContext, MemoryArea,
		    VirtualPage, PhysicalPage, SwapEntry, Dirty);
	 }
     }
   
   RemoveEntryList(&MemoryArea->Entry);
   ExFreePool(MemoryArea);
   
   DPRINT("MmFreeMemoryArea() succeeded\n");
   
   return(STATUS_SUCCESS);
}

PMEMORY_AREA MmSplitMemoryArea(PEPROCESS Process,
			       PMADDRESS_SPACE AddressSpace,
			       PMEMORY_AREA OriginalMemoryArea,
			       PVOID BaseAddress,
			       ULONG Length,
			       ULONG NewType,
			       ULONG NewAttributes)
{
   PMEMORY_AREA Result;
   PMEMORY_AREA Split;
   
   Result = ExAllocatePoolWithTag(NonPagedPool, sizeof(MEMORY_AREA),
				  TAG_MAREA);
   RtlZeroMemory(Result,sizeof(MEMORY_AREA));
   Result->Type = NewType;
   Result->BaseAddress = BaseAddress;
   Result->Length = Length;
   Result->Attributes = NewAttributes;
   Result->LockCount = 0;
   Result->ReferenceCount = 1;
   Result->Process = Process;

   if (BaseAddress == OriginalMemoryArea->BaseAddress)
     {
	OriginalMemoryArea->BaseAddress = BaseAddress + Length;
	OriginalMemoryArea->Length = OriginalMemoryArea->Length - Length;
	MmInsertMemoryArea(AddressSpace, Result);
	return(Result);
     }
   if ((BaseAddress + Length) == 
       (OriginalMemoryArea->BaseAddress + OriginalMemoryArea->Length))
     {
	OriginalMemoryArea->Length = OriginalMemoryArea->Length - Length; 
	MmInsertMemoryArea(AddressSpace, Result);

	return(Result);
     }

   Split = ExAllocatePoolWithTag(NonPagedPool, sizeof(MEMORY_AREA),
				 TAG_MAREA);
   RtlCopyMemory(Split,OriginalMemoryArea,sizeof(MEMORY_AREA));
   Split->BaseAddress = BaseAddress + Length;
   Split->Length = OriginalMemoryArea->Length - (((ULONG)BaseAddress) 
						 + Length);
   
   OriginalMemoryArea->Length = BaseAddress - OriginalMemoryArea->BaseAddress;
      
   return(Split);
}

NTSTATUS
MmCreateMemoryArea(IN PEPROCESS  Process,
	IN PMADDRESS_SPACE  AddressSpace,
	IN ULONG  Type,
	IN OUT PVOID*  BaseAddress,
	IN ULONG  Length,
	IN ULONG  Attributes,
	OUT PMEMORY_AREA*  Result,
	IN BOOLEAN  FixedAddress)
/*
 * FUNCTION: Create a memory area
 * ARGUMENTS:
 *     AddressSpace = Address space to create the area in
 *     Type = Type of the address space
 *     BaseAddress = 
 *     Length = Length to allocate
 *     Attributes = Protection attributes for the memory area
 *     Result = Receives a pointer to the memory area on exit
 *     FixedAddress = Wether the memory area must be based at BaseAddress or not
 * RETURNS: Status
 * NOTES: Lock the address space before calling this function
 */
{
	 PMEMORY_AREA  MemoryArea;
	
   DPRINT("MmCreateMemoryArea(Type %d, BaseAddress %x,"
	   "*BaseAddress %x, Length %x, Attributes %x, Result %x)\n",
	   Type,BaseAddress,*BaseAddress,Length,Attributes,Result);

   if ((*BaseAddress)==0 && !FixedAddress)
     {
	*BaseAddress = MmFindGap(AddressSpace,
				 PAGE_ROUND_UP(Length) +(PAGESIZE*2));
	if ((*BaseAddress)==0)
	  {
	     DPRINT("No suitable gap\n");
	     return(STATUS_NO_MEMORY);
	  }
	(*BaseAddress)=(*BaseAddress)+PAGESIZE;
     }
   else
     {
	(*BaseAddress) = (PVOID)PAGE_ROUND_DOWN((*BaseAddress));
	MemoryArea = MmOpenMemoryAreaByRegion(AddressSpace, *BaseAddress, Length);
	if (MemoryArea)
	  {
			 MmCloseMemoryArea(MemoryArea);
	     DPRINT("Memory area already occupied\n");
	     return(STATUS_CONFLICTING_ADDRESSES);
	  }
     }

	DPRINT("MmCreateMemoryArea(*BaseAddress %x)\n", *BaseAddress);

   *Result = ExAllocatePoolWithTag(NonPagedPool, sizeof(MEMORY_AREA),
				   TAG_MAREA);
   RtlZeroMemory(*Result,sizeof(MEMORY_AREA));
   SET_MAGIC(*Result, TAG_MAREA)
   (*Result)->Type = Type;
   (*Result)->BaseAddress = *BaseAddress;
   (*Result)->Length = Length;
   (*Result)->Attributes = Attributes;
   (*Result)->LockCount = 0;
   (*Result)->ReferenceCount = 1;
   (*Result)->Process = Process;

   MmApplyMemoryAreaProtection(*Result);
	 
   MmInsertMemoryArea(AddressSpace, *Result);
   
   DPRINT("MmCreateMemoryArea() succeeded\n");
   return(STATUS_SUCCESS);
}

#ifdef DBG

VOID
MiReferenceMemoryArea(IN PMEMORY_AREA  MemoryArea,
  IN LPSTR  FileName,
	IN ULONG  LineNumber)
{
  VALIDATE_MEMORY_AREA(MemoryArea);

  InterlockedIncrement(&MemoryArea->ReferenceCount);

#ifdef TRACK_MEMORY_AREA_REFERENCES
	DbgPrint("(0x%.08x)(%s:%d) Referencing memory area 0x%.08x (New ref.count %d)\n",
		KeGetCurrentThread(), FileName, LineNumber,
		MemoryArea->BaseAddress,
		MemoryArea->ReferenceCount);
#endif /* TRACK_MEMORY_AREA_REFERENCES */
}


VOID
MiDereferenceMemoryArea(IN PMEMORY_AREA  MemoryArea,
  IN LPSTR  FileName,
	IN ULONG  LineNumber)
{
  VALIDATE_MEMORY_AREA(MemoryArea);

  InterlockedDecrement(&MemoryArea->ReferenceCount);

#ifdef TRACK_MEMORY_AREA_REFERENCES
	DbgPrint("(0x%.08x)(%s:%d) Dereferencing memory area 0x%.08x (New ref.count %d)\n",
		KeGetCurrentThread(), FileName, LineNumber,
		MemoryArea->BaseAddress,
		MemoryArea->ReferenceCount);
#endif /* TRACK_MEMORY_AREA_REFERENCES */

  assertmsg(MemoryArea->ReferenceCount > 0,
    ("No outstanding references on memory area (0x%.08x)\n", MemoryArea));
}

#else /* !DBG */

VOID
MiReferenceMemoryArea(IN PMEMORY_AREA  MemoryArea)
{
  InterlockedIncrement(&MemoryArea->ReferenceCount);
}


VOID
MiDereferenceMemoryArea(IN PMEMORY_AREA  MemoryArea)
{
  InterlockedDecrement(&MemoryArea->ReferenceCount);
}

#endif /* !DBG */
