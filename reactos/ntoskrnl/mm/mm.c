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
/* $Id: mm.c,v 1.55.2.1 2002/05/13 20:37:00 chorns Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/mm.c
 * PURPOSE:     kernel memory managment functions
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              Created 9/4/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/i386/segment.h>
#include <internal/mm.h>
#include <internal/ntoskrnl.h>
#include <internal/io.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

PVOID EXPORTED MmUserProbeAddress = NULL; 
PVOID EXPORTED MmHighestUserAddress = NULL;
MM_STATS MmStats; 
extern PVOID MmSharedDataPagePhysicalAddress;
ULONG MiMaximumModifiedPageListSize;
ULONG MiMaximumStandbyPageListSize;

/* FUNCTIONS ****************************************************************/

#ifdef DBG

VOID
DbgMmDumpProtection(IN ULONG  Value)
{
	if (Value & PAGE_NOACCESS)
		DbgPrint("No access\n");

	if (Value & PAGE_GUARD)
		DbgPrint("Guard\n");

	if (Value & PAGE_READWRITE)
		DbgPrint("Read/Write\n");

	if (Value & PAGE_EXECUTE_READWRITE)
		DbgPrint("Execute/Read/Write\n");

	if (Value & PAGE_READONLY)
		DbgPrint("Read only\n");

	if (Value & PAGE_EXECUTE)
		DbgPrint("Execute\n");

	if (Value & PAGE_EXECUTE_READ)
		DbgPrint("Execute/Read\n");

	if (Value & PAGE_SYSTEM)
		DbgPrint("System\n");

	if (Value & PAGE_NOCACHE)
		DbgPrint("No cache\n");

	if (Value & PAGE_WRITETHROUGH)
		DbgPrint("No cache\n");
}

#endif /* DBG */


NTSTATUS MmReleaseMemoryArea(PEPROCESS Process, PMEMORY_AREA Marea)
{
   NTSTATUS Status;
   
   DPRINT("MmReleaseMemoryArea(Process %x, Marea %x)\n",Process,Marea);
   
   DPRINT("Releasing %x between %x %x (type %d)\n",
	   Marea, Marea->BaseAddress, Marea->BaseAddress + Marea->Length,
	   Marea->Type);
   
   switch (Marea->Type)
     {
     case MEMORY_AREA_SECTION_VIEW_COMMIT:
     case MEMORY_AREA_SECTION_VIEW_RESERVE:
	Status = MmUnmapViewOfSection(Process, Marea->BaseAddress);
	assert(Status == STATUS_SUCCESS);
	return(STATUS_SUCCESS);

     case MEMORY_AREA_VIRTUAL_MEMORY:
       MmFreeVirtualMemory(Process, Marea);
       break;	

     case MEMORY_AREA_SHARED_DATA:
       Status = MmFreeMemoryArea(&Process->AddressSpace,
				 Marea->BaseAddress,
				 0,
				 NULL,
				 NULL);
       break;

     default:
       KeBugCheck(0);
     }
   
   return(STATUS_SUCCESS);
}

NTSTATUS MmReleaseMmInfo(PEPROCESS Process)
{
   PLIST_ENTRY CurrentEntry;
   PMEMORY_AREA Current;
   
   DPRINT("MmReleaseMmInfo(Process %x (%s))\n", Process,
	   Process->ImageFileName);
   
   MmLockAddressSpace(&Process->AddressSpace);

   CurrentEntry = Process->AddressSpace.MAreaListHead.Flink;
   while (CurrentEntry != &Process->AddressSpace.MAreaListHead)
     {
	Current = CONTAINING_RECORD(CurrentEntry, MEMORY_AREA, Entry);
	CurrentEntry = CurrentEntry->Flink;
	
	MmReleaseMemoryArea(Process, Current);
     }
   
   Mmi386ReleaseMmInfo(Process);
   
   MmUnlockAddressSpace(&Process->AddressSpace);
   MmDestroyAddressSpace(&Process->AddressSpace);
   
   DPRINT("Finished MmReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}

BOOLEAN STDCALL MmIsNonPagedSystemAddressValid(PVOID VirtualAddress)
{
   UNIMPLEMENTED;
}

BOOLEAN STDCALL MmIsAddressValid(PVOID VirtualAddress)
/*
 * FUNCTION: Checks whether the given address is valid for a read or write
 * ARGUMENTS:
 *          VirtualAddress = address to check
 * RETURNS: True if the access would be valid
 *          False if the access would cause a page fault
 * NOTES: This function checks whether a byte access to the page would
 *        succeed. Is this realistic for RISC processors which don't
 *        allow byte granular access?
 */
{
	MEMORY_AREA* MemoryArea;
	PMADDRESS_SPACE AddressSpace;
	
	AddressSpace = &PsGetCurrentProcess()->AddressSpace;
	MmLockAddressSpace(AddressSpace);
  MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, VirtualAddress);
	if (MemoryArea == NULL)
    {
      MmUnlockAddressSpace(AddressSpace);
      return(FALSE);
    }
  MmCloseMemoryArea(MemoryArea);
	MmUnlockAddressSpace(AddressSpace);
	return(TRUE);
}

NTSTATUS MmAccessFault(KPROCESSOR_MODE Mode,
		       ULONG Address,
		       BOOLEAN FromMdl)
{
   PMADDRESS_SPACE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;
   
   DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);

   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
     {
	DbgPrint("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
	assert(FALSE);
	return(STATUS_UNSUCCESSFUL);
     }

	assertmsg(PsGetCurrentProcess() != NULL, ("No current process\n"));
	if (PsGetCurrentProcess() == NULL)
		return(STATUS_UNSUCCESSFUL);

   /*
    * Find the memory area for the faulting address
    */
   if (Address >= KERNEL_BASE)
     {
	/*
	 * Check permissions
	 */
	if (Mode != KernelMode)
	  {
	     DbgPrint("%s:%d\n",__FILE__,__LINE__);
	     return(STATUS_UNSUCCESSFUL);
	  }
	AddressSpace = MmGetKernelAddressSpace();
     }
   else
     {
	AddressSpace = &PsGetCurrentProcess()->AddressSpace;
     }
   
   if (!FromMdl)
     {
       MmLockAddressSpace(AddressSpace);
     }
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, (PVOID)Address);
   if (MemoryArea == NULL)
     {
	DbgPrint("%s:%d\n",__FILE__,__LINE__);
	if (!FromMdl)
	  {
	    MmUnlockAddressSpace(AddressSpace);
	  }
	return(STATUS_UNSUCCESSFUL);
     }

   DPRINT("MmAccessFault(). MemoryArea->Type %d\n", MemoryArea->Type);

   switch (MemoryArea->Type)
     {
      case MEMORY_AREA_SYSTEM:
     assertmsg(FALSE, ("Access fault on system memory area\n"));
	Status = STATUS_UNSUCCESSFUL;
	break;

     case MEMORY_AREA_PAGED_POOL:
       Status = STATUS_SUCCESS;
       break;
	
      case MEMORY_AREA_SECTION_VIEW_COMMIT:
	Status = MmAccessFaultSectionView(AddressSpace,
		MemoryArea, 
		(PVOID)Address,
		Locked);
	break;
	
      case MEMORY_AREA_VIRTUAL_MEMORY:
     assertmsg(FALSE, ("Access fault on virtual memory area\n"));
	Status = STATUS_UNSUCCESSFUL;
	break;
	
      case MEMORY_AREA_SHARED_DATA:
     assertmsg(FALSE, ("Access fault on shared memory area\n"));
	Status = STATUS_UNSUCCESSFUL;
	break;
	
      default:
    assertmsg(FALSE, ("Unknown memory area type %d\n", MemoryArea->Type));
	Status = STATUS_UNSUCCESSFUL;
	break;
     }
   if (!FromMdl)
     {
       MmUnlockAddressSpace(AddressSpace);
     }
   DPRINT("Completed access fault handling for 0x%.08x with status 0x%.08x\n",
		 Address, Status);
   MmCloseMemoryArea(MemoryArea);
   return(Status);
}

NTSTATUS MmCommitPagedPoolAddress(PVOID Address)
{
  NTSTATUS Status;
  ULONG_PTR AllocatedPage;

  Status = MmRequestPageMemoryConsumer(MC_PPOOL, FALSE, &AllocatedPage);
  if (!NT_SUCCESS(Status))
    {
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      Status = MmRequestPageMemoryConsumer(MC_PPOOL, TRUE, &AllocatedPage);
      MmLockAddressSpace(MmGetKernelAddressSpace());
    }
  Status = 
    MmCreateVirtualMapping(NULL,
			   (PVOID)PAGE_ROUND_DOWN(Address),
			   PAGE_READWRITE,
			   (ULONG)AllocatedPage,
			   FALSE);
  if (!NT_SUCCESS(Status))
    {
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      Status = 
	MmCreateVirtualMapping(NULL,
			       (PVOID)PAGE_ROUND_DOWN(Address),
			       PAGE_READWRITE,
			       (ULONG)AllocatedPage,
			       FALSE);
      MmLockAddressSpace(MmGetKernelAddressSpace());
    }
  return(Status);
}


/*
 * Wait on a page operation if one is in progress
 * The address space is locked when called
 */
NTSTATUS
MiCheckFaultState(IN PMADDRESS_SPACE  AddressSpace,
  IN PMEMORY_AREA  MemoryArea,
  IN PVOID  Address)
{
  PMM_PAGEOP PageOp;

  DPRINT("MiCheckFaultState(MemoryArea 0x%.08x,  Address 0x%.08x)\n",
    MemoryArea, Address);

	/*
	 * See if there is a page operation in progress for this address
	 */

  if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW_COMMIT)
    {
      LARGE_INTEGER Offset;
      ULONG_PTR Page;

      Page = (ULONG_PTR) PAGE_ROUND_DOWN(((ULONG_PTR) Address));
      Offset.QuadPart = (Page - (ULONG_PTR)MemoryArea->BaseAddress) +
        MemoryArea->Data.SectionData.ViewOffset;

      DPRINT("Offset 0x%.08x\n", Offset.u.LowPart);

      PageOp = MmGotPageOp(MemoryArea,
        0,
        NULL,
        MemoryArea->Data.SectionData.Segment,
        Offset.u.LowPart);
    }
  else
    {
      PageOp = MmGotPageOp(MemoryArea,
        (ULONG)PsGetCurrentProcessId(), 
		    (PVOID)PAGE_ROUND_DOWN(Address),
        NULL,
        0);
    }

	if (PageOp != NULL)
    {
      NTSTATUS Status;

      DPRINT("Got a page operation 0x%.08x\n", PageOp);

			/*
			 * Check if someone else is already handling this fault, if so
			 * wait for them
			 */
		  if (PageOp->Thread != PsGetCurrentThread())
		    {
          DPRINT("Waiting for someone else to handle the page operation 0x%.08x\n",
            PageOp);

          MmUnlockAddressSpace(AddressSpace);

			    Status = KeWaitForSingleObject(&PageOp->CompletionEvent,
						0,
						KernelMode,
						FALSE,
						NULL);

          MmLockAddressSpace(AddressSpace);
		
		      if (!NT_SUCCESS(Status))
		        {
		          assertmsg(FALSE, ("Failed to wait for page operation (Status 0x%.08x)\n", Status));
		          MmReleasePageOp(PageOp);
		          return(Status);
			      }

		      if (PageOp->Status == STATUS_PENDING)
		        {
		          assertmsg(FALSE, ("Woke for page operation before completion\n"));
		          MmReleasePageOp(PageOp);
		          return(STATUS_UNSUCCESSFUL);
		        }

          DPRINT("Someone else handled the page operation 0x%.08x\n", PageOp);
		
					/*
					 * Restart the handling
					 */
				  MmReleasePageOp(PageOp);
				  return(STATUS_MM_RESTART_OPERATION);
        }
      else
				{
          assertmsg(FALSE, ("I am handling the page operation 0x%.08x\n", PageOp));
				}
    }

  DPRINT("No one else is handling the page operation\n");

  /*
   * If there is a physical page on either the standby or modified page list
   * backing the virtual address, then put it back on the used list.
   */
  if (MiPageState(PsGetCurrentProcess(), Address, PAGE_STATE_TRANSITION))
		{
      ULONG_PTR PhysicalAddress;
      NTSTATUS Status;

      DPRINT("Page at 0x%.08x is in transition\n", Address);
      PhysicalAddress = MmGetPhysicalAddressForProcess(PsGetCurrentProcess(), Address);
      assertmsg((ULONG_PTR) PhysicalAddress != 0, ("PhysicalAddress for PS/VA (0x%.08x, 0x%.08x) is 0\n",
        PsGetCurrentProcess(), Address));

      assertmsg(MmGetFlagsPage(PhysicalAddress) != MM_PHYSICAL_PAGE_MPW,
        ("Page at 0x%.08x is beeing flushed to secondary storage.\n", PhysicalAddress));

      MmUnlockAddressSpace(AddressSpace);
      Status = MiAbortTransition(PhysicalAddress);
      MmLockAddressSpace(AddressSpace);

      DPRINT("MiAbortTransition() said 0x%.08x\n", Status);
      return Status;
		}

  return(STATUS_MM_RESTART_OPERATION);
}


NTSTATUS MmNotPresentFault(KPROCESSOR_MODE Mode,
			   ULONG Address, 
			   BOOLEAN FromMdl)
{
   PMADDRESS_SPACE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;
   
   DPRINT("MmNotPresentFault(Mode %d, Address %x)\n", Mode, Address);

   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
     {
       DbgPrint("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
	   assert(FALSE);
       return(STATUS_UNSUCCESSFUL);
     }

   assertmsg(PsGetCurrentProcess() != NULL, ("No current process\n"));
	if (PsGetCurrentProcess() == NULL)
		return(STATUS_UNSUCCESSFUL);
   
   /*
    * Find the memory area for the faulting address
    */
   if (Address >= KERNEL_BASE)
     {
	/*
	 * Check permissions
	 */
	if (Mode != KernelMode)
	  {
	     assert(FALSE);
	     return(STATUS_UNSUCCESSFUL);
	  }
	AddressSpace = MmGetKernelAddressSpace();
     }
   else
     {
	AddressSpace = &PsGetCurrentProcess()->AddressSpace;
     }

   if (!FromMdl)
     {
       MmLockAddressSpace(AddressSpace);
     }

   /*
    * Call the memory area specific fault handler
    */
   do
     {
       MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, (PVOID)Address);
       if (MemoryArea == NULL)
				 {
				   if (!FromMdl)
				     {
				       MmUnlockAddressSpace(AddressSpace);
				     }
			     assertmsg(FALSE, ("No memory area for address 0x%.08x\n", Address));
				   return (STATUS_UNSUCCESSFUL);
				 }

        DPRINT("Memory area type %d\n", MemoryArea->Type);

        Status = MiCheckFaultState(AddressSpace, MemoryArea, (PVOID) Address);
	      if (Status == STATUS_SUCCESS)
          {
						MmCloseMemoryArea(MemoryArea);
	          if (!FromMdl)
	            {
	              MmUnlockAddressSpace(AddressSpace);
	            }
	          return (Status);
	        }

       switch (MemoryArea->Type)
	 {
	 case MEMORY_AREA_PAGED_POOL:
	   {
	     Status = MmCommitPagedPoolAddress((PVOID)Address);
	     break;
	   }

	 case MEMORY_AREA_SYSTEM:
	   Status = STATUS_UNSUCCESSFUL;
	   break;
	   
	 case MEMORY_AREA_SECTION_VIEW_COMMIT:
	   Status = MmNotPresentFaultSectionView(AddressSpace,
						 MemoryArea, 
						 (PVOID)Address,
						 Locked);
	   break;
	   
	 case MEMORY_AREA_VIRTUAL_MEMORY:
	   Status = MmNotPresentFaultVirtualMemory(AddressSpace,
						   MemoryArea,
						   (PVOID)Address,
						   Locked);
	       break;
	       
	 case MEMORY_AREA_SHARED_DATA:
	   Status = 
	     MmCreateVirtualMapping(PsGetCurrentProcess(),
				    (PVOID)PAGE_ROUND_DOWN(Address),
				    PAGE_READONLY,
				    (ULONG)MmSharedDataPagePhysicalAddress,
				    FALSE);
	   if (!NT_SUCCESS(Status))
	     {
	       MmUnlockAddressSpace(&PsGetCurrentProcess()->AddressSpace);
	       Status = MmCreateVirtualMapping(PsGetCurrentProcess(),
					(PVOID)PAGE_ROUND_DOWN(Address),
					PAGE_READONLY,
					(ULONG_PTR)MmSharedDataPagePhysicalAddress,
					TRUE);
	       MmLockAddressSpace(&PsGetCurrentProcess()->AddressSpace);
	     }
	   break;
	   
	 default:
     assertmsg(FALSE, ("Unknown memory area type %d\n", MemoryArea->Type));
	   Status = STATUS_UNSUCCESSFUL;
	   break;
	 }
	 MmCloseMemoryArea(MemoryArea);
     }
   while (Status == STATUS_MM_RESTART_OPERATION);

   DPRINT("Completed not-present fault handling for 0x%.08x with status 0x%.08x\n",
		 Address, Status);

   if (!FromMdl)
     {
       MmUnlockAddressSpace(AddressSpace);
     }
   return(Status);
}

/* Miscellanea functions: they may fit somewhere else */

DWORD STDCALL
MmAdjustWorkingSetSize (DWORD	Unknown0,
			DWORD	Unknown1,
			DWORD	Unknown2)
{
	UNIMPLEMENTED;
	return (0);
}


DWORD
STDCALL
MmDbgTranslatePhysicalAddress (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED;
	return (0);
}


NTSTATUS
STDCALL
MmGrowKernelStack (
	DWORD	Unknown0
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


BOOLEAN
STDCALL
MmSetAddressRangeModified (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED;
	return (FALSE);
}

/* EOF */
