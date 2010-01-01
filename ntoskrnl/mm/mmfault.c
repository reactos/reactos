/*
 * COPYRIGHT:       See COPYING in the top directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/mmfault.c
 * PURPOSE:         Kernel memory managment functions
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

/* GLOBALS ********************************************************************/

KEVENT MmWaitPageEvent;

/* PRIVATE FUNCTIONS **********************************************************/

typedef struct _WORK_QUEUE_WITH_CONTEXT
{
	WORK_QUEUE_ITEM WorkItem;
	PMMSUPPORT AddressSpace;
	PMEMORY_AREA MemoryArea;
	PMM_REQUIRED_RESOURCES Required;
	NTSTATUS Status;
	KEVENT Wait;
	AcquireResource DoAcquisition;
} WORK_QUEUE_WITH_CONTEXT, *PWORK_QUEUE_WITH_CONTEXT;

VOID
NTAPI
MmpFaultWorker
(PWORK_QUEUE_WITH_CONTEXT WorkItem)
{
	DPRINT("Calling work\n");
	WorkItem->Status = 
		WorkItem->Required->DoAcquisition
		(WorkItem->AddressSpace,
		 WorkItem->MemoryArea,
		 WorkItem->Required);
	DPRINT("Status %x\n", WorkItem->Status);
	KeSetEvent(&WorkItem->Wait, IO_NO_INCREMENT, FALSE);
}

VOID
FASTCALL
MiSyncForProcessAttach(IN PKTHREAD Thread,
                       IN PEPROCESS Process)
{
    PETHREAD Ethread = CONTAINING_RECORD(Thread, ETHREAD, Tcb);

    /* Hack Sync because Mm is broken */
    MmUpdatePageDir(Process, Ethread, sizeof(ETHREAD));
    MmUpdatePageDir(Process, Ethread->ThreadsProcess, sizeof(EPROCESS));
    MmUpdatePageDir(Process,
                    (PVOID)Thread->StackLimit,
                    Thread->LargeStack ?
                    KERNEL_LARGE_STACK_SIZE : KERNEL_STACK_SIZE);
}

VOID
FASTCALL
MiSyncForContextSwitch(IN PKTHREAD Thread)
{
    PVOID Process = PsGetCurrentProcess();
    PETHREAD Ethread = CONTAINING_RECORD(Thread, ETHREAD, Tcb);

    /* Hack Sync because Mm is broken */
    MmUpdatePageDir(Process, Ethread->ThreadsProcess, sizeof(EPROCESS));
    MmUpdatePageDir(Process,
                    (PVOID)Thread->StackLimit,
                    Thread->LargeStack ?
                    KERNEL_LARGE_STACK_SIZE : KERNEL_STACK_SIZE);
}

NTSTATUS
NTAPI
MmpAccessFaultInner
(KPROCESSOR_MODE Mode,
 PMMSUPPORT AddressSpace,
 ULONG_PTR Address,
 BOOLEAN FromMdl,
 PETHREAD Thread)
{
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;
   MM_REQUIRED_RESOURCES Resources = { 0 };

   DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);

   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
   {
      DPRINT1("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Find the memory area for the faulting address
    */
   if (Address >= (ULONG_PTR)MmSystemRangeStart)
   {
      /*
       * Check permissions
       */
      if (Mode != KernelMode)
      {
         DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);
         return(STATUS_ACCESS_VIOLATION);
      }
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->Vm;
   }

   if (!FromMdl)
   {
      MmLockAddressSpace(AddressSpace);
   }

   do
   {
      MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)Address);
      if (MemoryArea == NULL || 
		  MemoryArea->DeleteInProgress ||
		  !MemoryArea->AccessFault)
      {
         if (!FromMdl)
         {
            MmUnlockAddressSpace(AddressSpace);
         }
		 DPRINT1("Address: %x\n", Address);
         return (STATUS_ACCESS_VIOLATION);
      }

	  DPRINT
		  ("Type %x (%x -> %x)\n", 
		   MemoryArea->Type, 
		   MemoryArea->StartingAddress, 
		   MemoryArea->EndingAddress);

	  Resources.DoAcquisition = NULL;

	  // Note: fault handlers are called with address space locked
	  // We return STATUS_MORE_PROCESSING_REQUIRED if anything is needed
	  Status = MemoryArea->AccessFault
		  (AddressSpace, MemoryArea, (PVOID)Address, Locked, &Resources);

	  if (!FromMdl)
	  {
		  MmUnlockAddressSpace(AddressSpace);
	  }

	  if (Status == STATUS_SUCCESS + 1)
	  {
		  // Wait page ...
		  if (!NT_SUCCESS
			  (KeWaitForSingleObject
			   (&MmWaitPageEvent,
				0,
				KernelMode,
				FALSE,
				NULL)))
			  ASSERT(FALSE);
		  Status = STATUS_MM_RESTART_OPERATION;
	  }
	  else if (Status == STATUS_MORE_PROCESSING_REQUIRED)
	  {
		  if (Thread->ActiveFaultCount > 1)
		  {
			  WORK_QUEUE_WITH_CONTEXT Context = { };
			  DPRINT("Already fault handling ... going to work item (%x)\n", Address);
			  Context.AddressSpace = AddressSpace;
			  Context.MemoryArea = MemoryArea;
			  Context.Required = &Resources;
			  KeInitializeEvent(&Context.Wait, NotificationEvent, FALSE);
			  ExInitializeWorkItem(&Context.WorkItem, (PWORKER_THREAD_ROUTINE)MmpFaultWorker, &Context);
			  DPRINT("Queue work item\n");
			  ExQueueWorkItem(&Context.WorkItem, DelayedWorkQueue);
			  DPRINT("Wait\n");
			  KeWaitForSingleObject(&Context.Wait, 0, KernelMode, FALSE, NULL);
			  Status = Context.Status;
			  DPRINT("Status %x\n", Status);
		  }
		  else
		  {
			  Status = Resources.DoAcquisition(AddressSpace, MemoryArea, &Resources);
		  }
		  
		  if (NT_SUCCESS(Status))
		  {
			  Status = STATUS_MM_RESTART_OPERATION;
		  }
	  }
	  
	  if (!FromMdl)
	  {
		  MmLockAddressSpace(AddressSpace);
	  }
   }
   while (Status == STATUS_MM_RESTART_OPERATION);

   if (!NT_SUCCESS(Status))
   {
	   DPRINT1("Completed page fault handling %x %x\n", Address, Status);
	   DPRINT1
		   ("Type %x (%x -> %x)\n", 
			MemoryArea->Type, 
			MemoryArea->StartingAddress, 
			MemoryArea->EndingAddress);
   }

   if (!FromMdl)
   {
      MmUnlockAddressSpace(AddressSpace);
   }
   return(Status);
}

NTSTATUS
NTAPI
MmpAccessFault
(KPROCESSOR_MODE Mode,
 ULONG_PTR Address,
 BOOLEAN FromMdl)
{
	PETHREAD Thread;
	PMMSUPPORT AddressSpace;
	NTSTATUS Status;
	
	DPRINT("MmpAccessFault(Mode %d, Address %x)\n", Mode, Address);
	
	Thread = PsGetCurrentThread();

	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		DPRINT1("Page fault at high IRQL %d, address %x\n", KeGetCurrentIrql(), Address);
		return(STATUS_UNSUCCESSFUL);
	}
	
	/*
	 * Find the memory area for the faulting address
	 */
	if (Address >= (ULONG_PTR)MmSystemRangeStart)
	{
		/*
		 * Check permissions
		 */
		if (Mode != KernelMode)
		{
			DPRINT1("Address: %x\n", Address);
			return(STATUS_ACCESS_VIOLATION);
		}
		AddressSpace = MmGetKernelAddressSpace();
	}
	else
	{
		AddressSpace = &PsGetCurrentProcess()->Vm;
	}
	
	Thread->ActiveFaultCount++;	
	Status = MmpAccessFaultInner(Mode, AddressSpace, Address, FromMdl, Thread);
	Thread->ActiveFaultCount--;

	return(Status);
}

NTSTATUS
NTAPI
MmFaultAcquirePage(PMM_REQUIRED_RESOURCES Resources)
{
	return MmRequestPageMemoryConsumer
		(Resources->Consumer,
		 TRUE,
		 &Resources->Page[Resources->Offset]);
}

NTSTATUS
NTAPI
MmNotPresentFaultInner
(KPROCESSOR_MODE Mode,
 PMMSUPPORT AddressSpace,
 ULONG_PTR Address,
 BOOLEAN FromMdl,
 PETHREAD Thread)
{
	BOOLEAN Locked = FromMdl;
	PMEMORY_AREA MemoryArea;
	MM_REQUIRED_RESOURCES Resources = { 0 };
	NTSTATUS Status = STATUS_SUCCESS;

	if (!FromMdl)
	{
		MmLockAddressSpace(AddressSpace);
	}
	
	/*
	 * Call the memory area specific fault handler
	 */
	do
	{
		MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)Address);
		if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
		{
			Status = STATUS_ACCESS_VIOLATION;
			DPRINT("Address %x\n", Address);
			break;
		}
		
		DPRINT1
			("Type %x (%x -> %x)\n", 
			 MemoryArea->Type, 
			 MemoryArea->StartingAddress, 
			 MemoryArea->EndingAddress);
		
		Resources.DoAcquisition = NULL;
		
		// Note: fault handlers are called with address space locked
		// We return STATUS_MORE_PROCESSING_REQUIRED if anything is needed
		if (MemoryArea->NotPresent)
			Status = MemoryArea->NotPresent
				(AddressSpace, MemoryArea, (PVOID)Address, Locked, &Resources);
		else
		{
			DPRINT("Address %x\n", Address);
			Status = STATUS_ACCESS_VIOLATION;
		}
		
		if (!FromMdl)
		{
			MmUnlockAddressSpace(AddressSpace);
		}
		
		if (Status == STATUS_SUCCESS + 1)
		{
			// Wait page ...
			if (!NT_SUCCESS
				(KeWaitForSingleObject
				 (&MmWaitPageEvent,
				  0,
				  KernelMode,
				  FALSE,
				  NULL)))
				ASSERT(FALSE);
			Status = STATUS_MM_RESTART_OPERATION;
		}
		else if (Status == STATUS_MORE_PROCESSING_REQUIRED)
		{
			if (Thread->ActiveFaultCount > 1)
			{
				WORK_QUEUE_WITH_CONTEXT Context = { };
				DPRINT("Already fault handling ... going to work item (%x)\n", Address);
				Context.AddressSpace = AddressSpace;
				Context.MemoryArea = MemoryArea;
				Context.Required = &Resources;
				KeInitializeEvent(&Context.Wait, NotificationEvent, FALSE);
				ExInitializeWorkItem(&Context.WorkItem, (PWORKER_THREAD_ROUTINE)MmpFaultWorker, &Context);
				DPRINT("Queue work item\n");
				ExQueueWorkItem(&Context.WorkItem, DelayedWorkQueue);
				DPRINT("Wait\n");
				KeWaitForSingleObject(&Context.Wait, 0, KernelMode, FALSE, NULL);
				Status = Context.Status;
				DPRINT("Status %x\n", Status);
			}
			else
			{
				Status = Resources.DoAcquisition
					(AddressSpace, MemoryArea, &Resources);
			}

			if (NT_SUCCESS(Status))
			{
				Status = STATUS_MM_RESTART_OPERATION;
			}
		}
		
		if (!FromMdl)
		{
			MmLockAddressSpace(AddressSpace);
		}
	}
	while (Status == STATUS_MM_RESTART_OPERATION);

	DPRINT("Completed page fault handling: %x\n", Status);
	if (!FromMdl)
	{
		MmUnlockAddressSpace(AddressSpace);
	}

	DPRINT("Done %x\n", Status);

	return Status;
}

NTSTATUS
NTAPI
MmNotPresentFault
(KPROCESSOR_MODE Mode,
 ULONG_PTR Address,
 BOOLEAN FromMdl)
{
	PETHREAD Thread;
	PMMSUPPORT AddressSpace;
	NTSTATUS Status;
	
	DPRINT("MmNotPresentFault(Mode %d, Address %x)\n", Mode, Address);
	
	Thread = PsGetCurrentThread();

	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		DPRINT1("Page fault at high IRQL %d, address %x\n", KeGetCurrentIrql(), Address);
		return(STATUS_UNSUCCESSFUL);
	}
	
	/*
	 * Find the memory area for the faulting address
	 */
	if (Address >= (ULONG_PTR)MmSystemRangeStart)
	{
		/*
		 * Check permissions
		 */
		if (Mode != KernelMode)
		{
			DPRINT1("Address: %x\n", Address);
			return(STATUS_ACCESS_VIOLATION);
		}
		AddressSpace = MmGetKernelAddressSpace();
	}
	else
	{
		AddressSpace = &PsGetCurrentProcess()->Vm;
	}
	
	Thread->ActiveFaultCount++;	
	Status = MmNotPresentFaultInner(Mode, AddressSpace, Address, FromMdl, Thread);
	Thread->ActiveFaultCount--;

	return(Status);
}

extern BOOLEAN Mmi386MakeKernelPageTableGlobal(PVOID Address);

NTSTATUS
NTAPI
MmAccessFault(IN BOOLEAN StoreInstruction,
              IN PVOID Address,
              IN KPROCESSOR_MODE Mode,
              IN PVOID TrapInformation)
{
    PMEMORY_AREA MemoryArea;

    /* Cute little hack for ROS */
    if ((ULONG_PTR)Address >= (ULONG_PTR)MmSystemRangeStart)
    {
#ifdef _M_IX86
        /* Check for an invalid page directory in kernel mode */
        if (Mmi386MakeKernelPageTableGlobal(Address))
        {
            /* All is well with the world */
            return STATUS_SUCCESS;
        }
#endif
    }
    
    //
    // Check if this is an ARM3 memory area
    //
    MemoryArea = MmLocateMemoryAreaByAddress(MmGetKernelAddressSpace(), Address);
    if ((MemoryArea) && (MemoryArea->Type == MEMORY_AREA_OWNED_BY_ARM3))
    {
        //
        // Hand it off to more competent hands...
        //
        return MmArmAccessFault(StoreInstruction, Address, Mode, TrapInformation);
    }   

    /* Keep same old ReactOS Behaviour */
    if (StoreInstruction)
    {
        /* Call access fault */
        return MmpAccessFault(Mode, (ULONG_PTR)Address, TrapInformation ? FALSE : TRUE);
    }
    else
    {
        /* Call not present */
        return MmNotPresentFault(Mode, (ULONG_PTR)Address, TrapInformation ? FALSE : TRUE);
    }
}

NTSTATUS
NTAPI
MmCommitPagedPoolAddress
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PVOID Address,
 BOOLEAN Locked,
 PMM_REQUIRED_RESOURCES Required)
{
   NTSTATUS Status;
   KIRQL OldIrql;

   if (MmIsPagePresent(NULL, Address))
	   return STATUS_SUCCESS;

   if (!Required->Page[0])
   {
	   Required->Consumer = MC_PPOOL;
	   Required->Amount = 1;
	   Required->DoAcquisition = MiGetOnePage;
	   return STATUS_MORE_PROCESSING_REQUIRED;
   }

   Status =
      MmCreateVirtualMapping(NULL,
                             (PVOID)PAGE_ROUND_DOWN(Address),
                             PAGE_READWRITE,
                             &Required->Page[0],
                             1);
   if (Locked)
   {
      OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
      MmLockPage(Required->Page[0]);
      KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
   }
   return(Status);
}
