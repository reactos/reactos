/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/process.c
 * PURPOSE:         Microkernel process management
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

static inline void
UpdatePageDirs(PKTHREAD Thread, PKPROCESS Process)
{
   /* The stack and the thread structure of the current process may be 
      located in a page which is not present in the page directory of 
      the process we're attaching to. That would lead to a page fault 
      when this function returns. However, since the processor can't 
      call the page fault handler 'cause it can't push EIP on the stack, 
      this will show up as a stack fault which will crash the entire system.
      To prevent this, make sure the page directory of the process we're
      attaching to is up-to-date. */
   MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread->StackLimit, MM_STACK_SIZE);
   MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread, sizeof(ETHREAD));
}

/*
 * @implemented
 */
VOID 
STDCALL
KeAttachProcess(PKPROCESS Process)
{
	KIRQL OldIrql;
	PKTHREAD Thread = KeGetCurrentThread();
	
	DPRINT("KeAttachProcess: %x\n", Process);

	UpdatePageDirs(Thread, Process);

	/* Lock Dispatcher */
	OldIrql = KeAcquireDispatcherDatabaseLock();
	
	/* Crash system if DPC is being executed! */
	if (KeIsExecutingDpc()) {
		DPRINT1("Invalid attach (Thread is executing a DPC!)\n");
		KEBUGCHECK(INVALID_PROCESS_ATTACH_ATTEMPT);
	}
	
	/* Check if the Target Process is already attached */
	if (Thread->ApcState.Process == Process || Thread->ApcStateIndex != OriginalApcEnvironment) {
		DPRINT("Process already Attached. Exitting\n");
		KeReleaseDispatcherDatabaseLock(OldIrql);
	} else { 
		KiAttachProcess(Thread, Process, OldIrql, &Thread->SavedApcState);
	}
}

VOID
STDCALL
KiAttachProcess(PKTHREAD Thread, PKPROCESS Process, KIRQL ApcLock, PRKAPC_STATE SavedApcState)
{
  
	DPRINT("KiAttachProcess(Thread: %x, Process: %x, SavedApcState: %x\n", Thread, Process, SavedApcState);
   
	/* Increase Stack Count */
	Process->StackCount++;
	
	/* Swap the APC Environment */
	KiMoveApcState(&Thread->ApcState, SavedApcState);
	
	/* Reinitialize Apc State */
	InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);
	InitializeListHead(&Thread->ApcState.ApcListHead[UserMode]);
	Thread->ApcState.Process = Process;
	Thread->ApcState.KernelApcInProgress = FALSE;
	Thread->ApcState.KernelApcPending = FALSE;
	Thread->ApcState.UserApcPending = FALSE;
    
	/* Update Environment Pointers if needed*/
	if (SavedApcState == &Thread->SavedApcState) {
		Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->SavedApcState;
		Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->ApcState;
		Thread->ApcStateIndex = AttachedApcEnvironment;
	}
	
	/* Swap the Processes */
	KiSwapProcess(Process, SavedApcState->Process);
	
	/* Return to old IRQL*/
	KeReleaseDispatcherDatabaseLock(ApcLock);
	
	DPRINT("KiAttachProcess Completed Sucesfully\n");
}

VOID
STDCALL
KiSwapProcess(PKPROCESS NewProcess, PKPROCESS OldProcess) 
{
	//PKPCR Pcr = KeGetCurrentKpcr();

	/* Do they have an LDT? */
	if ((NewProcess->LdtDescriptor) || (OldProcess->LdtDescriptor)) {
		/* FIXME : SWitch GDT/IDT */
	}
	DPRINT("Switching CR3 to: %x\n", NewProcess->DirectoryTableBase.u.LowPart);
	Ke386SetPageTableDirectory(NewProcess->DirectoryTableBase.u.LowPart);
	
	/* FIXME: Set IopmOffset in TSS */
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeIsAttachedProcess(
	VOID
	)
{
	return KeGetCurrentThread()->ApcStateIndex;
}

/*
 * @implemented
 */
VOID
STDCALL
KeStackAttachProcess (
    IN PKPROCESS Process,
    OUT PRKAPC_STATE ApcState
    )
{
	KIRQL OldIrql;
	PKTHREAD Thread = KeGetCurrentThread();

	UpdatePageDirs(Thread, Process);

	OldIrql = KeAcquireDispatcherDatabaseLock();
	
	/* Crash system if DPC is being executed! */
	if (KeIsExecutingDpc()) {
		DPRINT1("Invalid attach (Thread is executing a DPC!)\n");
		KEBUGCHECK(INVALID_PROCESS_ATTACH_ATTEMPT);
	}
	
	/* Check if the Target Process is already attached */
	if (Thread->ApcState.Process == Process) {
		ApcState->Process = (PKPROCESS)1;  /* Meaning already attached to the same Process */
	} else { 
		/* Check if the Current Thread is already attached and call the Internal Function*/
		if (Thread->ApcStateIndex != OriginalApcEnvironment) {
			KiAttachProcess(Thread, Process, OldIrql, ApcState);
		} else {
			KiAttachProcess(Thread, Process, OldIrql, &Thread->SavedApcState);
			ApcState->Process = NULL; 
		}
	}
}

/*
 * @implemented
 */
VOID STDCALL
KeDetachProcess (VOID)
{
	PKTHREAD Thread;
	KIRQL OldIrql;
   
	DPRINT("KeDetachProcess()\n");
   
	/* Get Current Thread and Lock */
	Thread = KeGetCurrentThread();
	OldIrql = KeAcquireDispatcherDatabaseLock();
	
	/* Check if it's attached */
	DPRINT("Current ApcStateIndex: %x\n", Thread->ApcStateIndex);
	
	if (Thread->ApcStateIndex == OriginalApcEnvironment) {
		DPRINT1("Invalid detach (thread was not attached)\n");
		KEBUGCHECK(INVALID_PROCESS_DETACH_ATTEMPT);
	}
   
	/* Decrease Stack Count */
	Thread->ApcState.Process->StackCount--;
	
	/* Restore the APC State */
	KiMoveApcState(&Thread->SavedApcState, &Thread->ApcState);
	Thread->SavedApcState.Process = NULL;
	Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
	Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
	Thread->ApcStateIndex = OriginalApcEnvironment;
	
	/* Swap Processes */
	KiSwapProcess(Thread->ApcState.Process, Thread->ApcState.Process);

	/* Unlock Dispatcher */
	KeReleaseDispatcherDatabaseLock(OldIrql);
}

/*
 * @implemented
 */
VOID
STDCALL
KeUnstackDetachProcess (
    IN PRKAPC_STATE ApcState
    )
{
	KIRQL OldIrql;
	PKTHREAD Thread;
	   
	/* If the special "We tried to attach to the process already being attached to" flag is there, don't do anything */
	if (ApcState->Process == (PKPROCESS)1) return;
	
	Thread = KeGetCurrentThread();
	OldIrql = KeAcquireDispatcherDatabaseLock();
	
	/* Sorry Buddy, can't help you if you've got APCs or just aren't attached */
	if ((Thread->ApcStateIndex == OriginalApcEnvironment) || (Thread->ApcState.KernelApcInProgress)) {
		DPRINT1("Invalid detach (Thread not Attached, or Kernel APC in Progress!)\n");
		KEBUGCHECK(INVALID_PROCESS_DETACH_ATTEMPT);
	}
	
	/* Restore the Old APC State if a Process was present */
	if (ApcState->Process) {
		RtlMoveMemory(ApcState, &Thread->ApcState, sizeof(KAPC_STATE));
	} else {
		/* The ApcState parameter is useless, so use the saved data and reset it */
		RtlMoveMemory(&Thread->SavedApcState, &Thread->ApcState, sizeof(KAPC_STATE));
		Thread->SavedApcState.Process = NULL;
		Thread->ApcStateIndex = OriginalApcEnvironment;
		Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
		Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
	}

	/* Restore the APC State */
	KiMoveApcState(&Thread->SavedApcState, &Thread->ApcState);
	
	/* Swap Processes */
	KiSwapProcess(Thread->ApcState.Process, Thread->ApcState.Process);
	
	/* Return to old IRQL*/
	KeReleaseDispatcherDatabaseLock(OldIrql);
}

// This function should be used by win32k.sys to add its own user32/gdi32 services
// TableIndex is 0 based
// ServiceCountTable its not used at the moment
/*
 * @implemented
 */
BOOLEAN STDCALL
KeAddSystemServiceTable (
	PSSDT	SSDT,
	PULONG	ServiceCounterTable,
	ULONG	NumberOfServices,
	PSSPT	SSPT,
	ULONG	TableIndex
	)
{
    /* check if descriptor table entry is free */
    if ((TableIndex > SSDT_MAX_ENTRIES - 1) ||    
        (KeServiceDescriptorTable[TableIndex].SSDT != NULL) ||
        (KeServiceDescriptorTableShadow[TableIndex].SSDT != NULL))
        return FALSE;

    /* initialize the shadow service descriptor table */
    KeServiceDescriptorTableShadow[TableIndex].SSDT = SSDT;
    KeServiceDescriptorTableShadow[TableIndex].SSPT = SSPT;
    KeServiceDescriptorTableShadow[TableIndex].NumberOfServices = NumberOfServices;
    KeServiceDescriptorTableShadow[TableIndex].ServiceCounterTable = ServiceCounterTable;
    
    return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
KeRemoveSystemServiceTable(
    IN PUCHAR Number
)
{
	UNIMPLEMENTED;
	return FALSE;
}
/* EOF */
