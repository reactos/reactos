/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2003 ReactOS Team
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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_MAREA   TAG('M', 'A', 'R', 'E')

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
               (char*)current->BaseAddress+current->Length,current->Attributes,
               current->Entry.Flink);
      current_entry = current_entry->Flink;
   }
   DbgPrint("Finished MmDumpMemoryAreas()\n");
}

MEMORY_AREA* MmOpenMemoryAreaByAddress(PMADDRESS_SPACE AddressSpace,
                                       PVOID Address)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   PLIST_ENTRY previous_entry;

   DPRINT("MmOpenMemoryAreaByAddress(AddressSpace %x, Address %x)\n",
          AddressSpace, Address);

   previous_entry = &AddressSpace->MAreaListHead;
   current_entry = AddressSpace->MAreaListHead.Flink;
   while (current_entry != &AddressSpace->MAreaListHead)
   {
      current = CONTAINING_RECORD(current_entry,
                                  MEMORY_AREA,
                                  Entry);
      ASSERT(current_entry->Blink->Flink == current_entry);
      ASSERT(current_entry->Flink->Blink == current_entry);
      ASSERT(previous_entry->Flink == current_entry);
      if (current->BaseAddress <= Address &&
            (PVOID)((char*)current->BaseAddress + current->Length) > Address)
      {
         DPRINT("%s() = %x\n",__FUNCTION__,current);
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
            current->BaseAddress < (PVOID)((char*)Address+Length))
      {
         DPRINT("Finished MmOpenMemoryAreaByRegion() = %x\n",
                current);
         return(current);
      }
      Extent = (ULONG)current->BaseAddress + current->Length;
      if (Extent > (ULONG)Address &&
            Extent < (ULONG)((char*)Address+Length))
      {
         DPRINT("Finished MmOpenMemoryAreaByRegion() = %x\n",
                current);
         return(current);
      }
      if (current->BaseAddress <= Address &&
            Extent >= (ULONG)((char*)Address+Length))
      {
         DPRINT("Finished MmOpenMemoryAreaByRegion() = %x\n",
                current);
         return(current);
      }
      if (current->BaseAddress >= (PVOID)((char*)Address+Length))
      {
         DPRINT("Finished MmOpenMemoryAreaByRegion()= NULL\n",0);
         return(NULL);
      }
      current_entry = current_entry->Flink;
   }
   DPRINT("Finished MmOpenMemoryAreaByRegion() = NULL\n",0);
   return(NULL);
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
   if (IsListEmpty(ListHead))
   {
      InsertHeadList(ListHead,&marea->Entry);
      return;
   }
   current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
   if (current->BaseAddress > marea->BaseAddress)
   {
      InsertHeadList(ListHead,&marea->Entry);
      return;
   }
   while (current_entry->Flink!=ListHead)
   {
      current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
      next = CONTAINING_RECORD(current_entry->Flink,MEMORY_AREA,Entry);
      if (current->BaseAddress < marea->BaseAddress &&
            current->Entry.Flink==ListHead)
      {
         current_entry->Flink = inserted_entry;
         inserted_entry->Flink=ListHead;
         inserted_entry->Blink=current_entry;
         ListHead->Blink = inserted_entry;
         return;
      }
      if (current->BaseAddress < marea->BaseAddress &&
            next->BaseAddress > marea->BaseAddress)
      {
         inserted_entry->Flink = current_entry->Flink;
         inserted_entry->Blink = current_entry;
         inserted_entry->Flink->Blink = inserted_entry;
         current_entry->Flink=inserted_entry;
         return;
      }
      current_entry = current_entry->Flink;
   }
   InsertTailList(ListHead,inserted_entry);
}

static PVOID MmFindGapBottomUp(PMADDRESS_SPACE AddressSpace, ULONG Length, ULONG Granularity)
{
   PLIST_ENTRY ListHead;
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   MEMORY_AREA* next;
   ULONG Gap;
   PVOID Address;

   DPRINT("MmFindGapBottomUp(Length %x)\n",Length);

#ifdef DBG
   Length += PAGE_SIZE; /* For a guard page following the area */
#endif

   ListHead = &AddressSpace->MAreaListHead;

   current_entry = ListHead->Flink;
   while (current_entry->Flink!=ListHead)
   {
      current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
      next = CONTAINING_RECORD(current_entry->Flink,MEMORY_AREA,Entry);
      Address = (PVOID) ((char*)current->BaseAddress + PAGE_ROUND_UP(current->Length));
#ifdef DBG
      Address = (PVOID) ((char *) Address + PAGE_SIZE); /* For a guard page preceding the area */
#endif
      Address = (PVOID) MM_ROUND_UP(Address, Granularity);
      if (Address < next->BaseAddress)
      {
         Gap = (char*)next->BaseAddress - ((char*)current->BaseAddress + PAGE_ROUND_UP(current->Length));
         if (Gap >= Length)
         {
            return Address;
         }
      }
      current_entry = current_entry->Flink;
   }

   if (current_entry == ListHead)
   {
      Address = (PVOID) MM_ROUND_UP(AddressSpace->LowestAddress, Granularity);
   }
   else
   {
      current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
      Address = (char*)current->BaseAddress + PAGE_ROUND_UP(current->Length);
#ifdef DBG
      Address = (PVOID) ((char *) Address + PAGE_SIZE); /* For a guard page preceding the area */
#endif
      Address = (PVOID) MM_ROUND_UP(Address, Granularity);
   }
   /* Check if enough space for the block */
   if (AddressSpace->LowestAddress < KERNEL_BASE)
   {
      if ((ULONG_PTR) Address >= KERNEL_BASE || Length > KERNEL_BASE - (ULONG_PTR) Address)
      {
         DPRINT1("Failed to find gap\n");
         return NULL;
      }
   }
   else
   {
      if (Length >= ~ ((ULONG_PTR) 0) - (ULONG_PTR) Address)
      {
         DPRINT1("Failed to find gap\n");
         return NULL;
      }
   }
   return Address;
}


static PVOID MmFindGapTopDown(PMADDRESS_SPACE AddressSpace, ULONG Length, ULONG Granularity)
{
   PLIST_ENTRY ListHead;
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   ULONG Gap;
   PVOID Address;
   PVOID TopAddress;
   PVOID BottomAddress;
   PVOID HighestAddress;

   DPRINT("MmFindGapTopDown(Length %lx)\n",Length);

#ifdef DBG
   Length += PAGE_SIZE; /* For a guard page following the area */
#endif

   if (AddressSpace->LowestAddress < KERNEL_BASE) //(ULONG_PTR)MmSystemRangeStart)
   {
      HighestAddress = MmHighestUserAddress;
   }
   else
   {
      HighestAddress = (PVOID)0xFFFFFFFF;
   }

   TopAddress = HighestAddress;

   ListHead = &AddressSpace->MAreaListHead;
   current_entry = ListHead->Blink;
   while (current_entry->Blink != ListHead)
   {
      current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
      BottomAddress = (char*)current->BaseAddress + PAGE_ROUND_UP(current->Length);
#ifdef DBG
      BottomAddress = (PVOID) ((char *) BottomAddress + PAGE_SIZE); /* For a guard page preceding the area */
#endif
      BottomAddress = (PVOID) MM_ROUND_UP(BottomAddress, Granularity);
      DPRINT("Base %p  Length %lx\n", current->BaseAddress, PAGE_ROUND_UP(current->Length));

      if (BottomAddress < TopAddress && BottomAddress < HighestAddress)
      {
         Gap = (char*)TopAddress - (char*) BottomAddress + 1;
         DPRINT("Bottom %p  Top %p  Gap %lx\n", BottomAddress, TopAddress, Gap);
         if (Gap >= Length)
         {
            DPRINT("Found gap at %p\n", (char*) TopAddress - Length);
            return (PVOID) MM_ROUND_DOWN((char*) TopAddress - Length + 1, Granularity);
         }
      }
      TopAddress = (char*)current->BaseAddress - 1;
      current_entry = current_entry->Blink;
   }

   if (current_entry == ListHead)
   {
      Address = (PVOID) MM_ROUND_DOWN((char*) HighestAddress - Length + 1, Granularity);
   }
   else
   {
      Address = (PVOID) MM_ROUND_DOWN((char*)TopAddress - Length + 1, Granularity);
   }

   /* Check if enough space for the block */
   if (AddressSpace->LowestAddress < KERNEL_BASE)
   {
      if ((ULONG_PTR) Address >= KERNEL_BASE || Length > KERNEL_BASE - (ULONG_PTR) Address)
      {
         DPRINT1("Failed to find gap\n");
         return NULL;
      }
   }
   else
   {
      if (Length >= ~ ((ULONG_PTR) 0) - (ULONG_PTR) Address)
      {
         DPRINT1("Failed to find gap\n");
         return NULL;
      }
   }

   DPRINT("Found gap at %p\n", Address);
   return Address;
}


PVOID MmFindGap(PMADDRESS_SPACE AddressSpace, ULONG Length, ULONG Granularity, BOOL TopDown)
{
   if (TopDown)
      return MmFindGapTopDown(AddressSpace, Length, Granularity);

   return MmFindGapBottomUp(AddressSpace, Length, Granularity);
}

ULONG MmFindGapAtAddress(PMADDRESS_SPACE AddressSpace, PVOID Address)
{
   PLIST_ENTRY current_entry, ListHead;
   PMEMORY_AREA current;

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   if (AddressSpace->LowestAddress < KERNEL_BASE)
   {
      if (Address >= (PVOID)KERNEL_BASE)
      {
         return 0;
      }
   }
   else
   {
      if ((ULONG_PTR)Address < AddressSpace->LowestAddress)
      {
         return 0;
      }
   }

   ListHead = &AddressSpace->MAreaListHead;

   current_entry = ListHead->Flink;
   while (current_entry != ListHead)
   {
      current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
      if (current->BaseAddress <= Address && (char*)Address < (char*)current->BaseAddress + current->Length)
      {
         return 0;
      }
      else if (current->BaseAddress > Address)
      {
         return (ULONG_PTR)current->BaseAddress - (ULONG_PTR)Address;
      }
      current_entry = current_entry->Flink;
   }
   if (AddressSpace->LowestAddress < KERNEL_BASE)
   {
      return KERNEL_BASE - (ULONG_PTR)Address;
   }
   else
   {
      return 0 - (ULONG_PTR)Address;
   }
}

NTSTATUS INIT_FUNCTION
MmInitMemoryAreas(VOID)
/*
 * FUNCTION: Initialize the memory area list
 */
{
   DPRINT("MmInitMemoryAreas()\n",0);
   return(STATUS_SUCCESS);
}

NTSTATUS
MmFreeMemoryArea(PMADDRESS_SPACE AddressSpace,
                 PVOID BaseAddress,
                 ULONG Length,
                 VOID (*FreePage)(PVOID Context, MEMORY_AREA* MemoryArea,
                                  PVOID Address, PFN_TYPE Page,
                                  SWAPENTRY SwapEntry, BOOLEAN Dirty),
                 PVOID FreePageContext)
{
   MEMORY_AREA* MemoryArea;
   char* Address;
   char* EndAddress;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();

   DPRINT("MmFreeMemoryArea(AddressSpace %x, BaseAddress %x, Length %x,"
          "FreePageContext %d)\n",AddressSpace,BaseAddress,Length,
          FreePageContext);

   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
                                          BaseAddress);
   if (MemoryArea == NULL)
   {
      KEBUGCHECK(0);
      return(STATUS_UNSUCCESSFUL);
   }
   if (AddressSpace->Process != NULL &&
         AddressSpace->Process != CurrentProcess)
   {
      KeAttachProcess((PKPROCESS)AddressSpace->Process);
   }
   EndAddress = (char*)MemoryArea->BaseAddress + PAGE_ROUND_UP(MemoryArea->Length); 
   for (Address = MemoryArea->BaseAddress; Address < EndAddress; Address += PAGE_SIZE)
   {

      if (MemoryArea->Type == MEMORY_AREA_IO_MAPPING)
      {
         MmRawDeleteVirtualMapping(Address);
      }
      else
      {
	 BOOL Dirty = FALSE;
         SWAPENTRY SwapEntry = 0;
	 PFN_TYPE Page = 0;


         if (MmIsPageSwapEntry(AddressSpace->Process, Address))
         {
            MmDeletePageFileMapping(AddressSpace->Process, Address, &SwapEntry);
         }
         else
         {
            MmDeleteVirtualMapping(AddressSpace->Process, Address, FALSE, &Dirty, &Page);
         }
         if (FreePage != NULL)
         {
            FreePage(FreePageContext, MemoryArea, Address, 
		     Page, SwapEntry, (BOOLEAN)Dirty);
         }
      }
   }
   if (AddressSpace->Process != NULL &&
         AddressSpace->Process != CurrentProcess)
   {
      KeDetachProcess();
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
   Result->Process = Process;

   if (BaseAddress == OriginalMemoryArea->BaseAddress)
   {
      OriginalMemoryArea->BaseAddress = (char*)BaseAddress + Length;
      OriginalMemoryArea->Length = OriginalMemoryArea->Length - Length;
      MmInsertMemoryArea(AddressSpace, Result);
      return(Result);
   }
   if (((char*)BaseAddress + Length) ==
         ((char*)OriginalMemoryArea->BaseAddress + OriginalMemoryArea->Length))
   {
      OriginalMemoryArea->Length = OriginalMemoryArea->Length - Length;
      MmInsertMemoryArea(AddressSpace, Result);

      return(Result);
   }

   Split = ExAllocatePoolWithTag(NonPagedPool, sizeof(MEMORY_AREA),
                                 TAG_MAREA);
   RtlCopyMemory(Split,OriginalMemoryArea,sizeof(MEMORY_AREA));
   Split->BaseAddress = (char*)BaseAddress + Length;
   Split->Length = OriginalMemoryArea->Length - (((ULONG)BaseAddress)
                   + Length);

   OriginalMemoryArea->Length = (char*)BaseAddress - (char*)OriginalMemoryArea->BaseAddress;

   return(Split);
}

NTSTATUS MmCreateMemoryArea(PEPROCESS Process,
                            PMADDRESS_SPACE AddressSpace,
                            ULONG Type,
                            PVOID* BaseAddress,
                            ULONG Length,
                            ULONG Attributes,
                            MEMORY_AREA** Result,
                            BOOL FixedAddress,
                            BOOL TopDown,
                            PHYSICAL_ADDRESS BoundaryAddressMultiple)
/*
 * FUNCTION: Create a memory area
 * ARGUMENTS:
 *     AddressSpace = Address space to create the area in
 *     Type = Type of the address space
 *     BaseAddress = 
 *     Length = Length to allocate
 *     Attributes = Protection attributes for the memory area
 *     Result = Receives a pointer to the memory area on exit
 * RETURNS: Status
 * NOTES: Lock the address space before calling this function
 */
{
   PVOID EndAddress;
   ULONG Granularity;
   ULONG tmpLength;
   DPRINT("MmCreateMemoryArea(Type %d, BaseAddress %x,"
          "*BaseAddress %x, Length %x, Attributes %x, Result %x)\n",
          Type,BaseAddress,*BaseAddress,Length,Attributes,Result);

   Granularity = (MEMORY_AREA_VIRTUAL_MEMORY == Type ? MM_VIRTMEM_GRANULARITY : PAGE_SIZE);
   if ((*BaseAddress) == 0 && !FixedAddress)
   {
      tmpLength = PAGE_ROUND_UP(Length);
      *BaseAddress = MmFindGap(AddressSpace,
                               PAGE_ROUND_UP(Length),
                               Granularity,
                               TopDown);
      if ((*BaseAddress) == 0)
      {
         DPRINT("No suitable gap\n");
         return STATUS_NO_MEMORY;
      }
   }
   else
   {
      tmpLength =  Length + ((ULONG_PTR) *BaseAddress
                             - (ULONG_PTR) MM_ROUND_DOWN(*BaseAddress, Granularity));
      *BaseAddress = MM_ROUND_DOWN(*BaseAddress, Granularity);

      if (AddressSpace->LowestAddress == KERNEL_BASE &&
            (*BaseAddress) < (PVOID)KERNEL_BASE)
      {
         return STATUS_ACCESS_VIOLATION;
      }

      if (AddressSpace->LowestAddress < KERNEL_BASE &&
            (PVOID)((char*)(*BaseAddress) + tmpLength) > (PVOID)KERNEL_BASE)
      {
         return STATUS_ACCESS_VIOLATION;
      }

      if (BoundaryAddressMultiple.QuadPart != 0)
      {
         EndAddress = ((char*)(*BaseAddress)) + tmpLength-1;
         ASSERT(((DWORD_PTR)*BaseAddress/BoundaryAddressMultiple.QuadPart) == ((DWORD_PTR)EndAddress/BoundaryAddressMultiple.QuadPart));
      }

      if (MmOpenMemoryAreaByRegion(AddressSpace,
                                   *BaseAddress,
                                   tmpLength)!=NULL)
      {
         DPRINT("Memory area already occupied\n");
         return STATUS_CONFLICTING_ADDRESSES;
      }
   }

   *Result = ExAllocatePoolWithTag(NonPagedPool, sizeof(MEMORY_AREA),
                                   TAG_MAREA);
   RtlZeroMemory(*Result,sizeof(MEMORY_AREA));
   (*Result)->Type = Type;
   (*Result)->BaseAddress = *BaseAddress;
   (*Result)->Length = tmpLength;
   (*Result)->Attributes = Attributes;
   (*Result)->LockCount = 0;
   (*Result)->Process = Process;
   (*Result)->PageOpCount = 0;
   (*Result)->DeleteInProgress = FALSE;

   MmInsertMemoryArea(AddressSpace, *Result);

   DPRINT("MmCreateMemoryArea() succeeded\n");
   return STATUS_SUCCESS;
}


void
MmReleaseMemoryAreaIfDecommitted(PEPROCESS Process,
                                 PMADDRESS_SPACE AddressSpace,
                                 PVOID BaseAddress)
{
  PMEMORY_AREA MemoryArea;
  PLIST_ENTRY Entry;
  PMM_REGION Region;
  BOOLEAN Reserved;
  
  MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, BaseAddress);
  if (NULL != MemoryArea)
    {
      Entry = MemoryArea->Data.VirtualMemoryData.RegionListHead.Flink;
      Reserved = TRUE;
      while (Reserved && Entry != &MemoryArea->Data.VirtualMemoryData.RegionListHead)
        {
          Region = CONTAINING_RECORD(Entry, MM_REGION, RegionListEntry);
          Reserved = (MEM_RESERVE == Region->Type);
          Entry = Entry->Flink;
        }

      if (Reserved)
        {
          MmFreeVirtualMemory(Process, MemoryArea);
        }
    }
}

/* EOF */
