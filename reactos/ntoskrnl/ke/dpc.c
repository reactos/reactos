/*
 *  ReactOS kernel
 *  Copyright (C) 2000, 1999, 1998 David Welch <welch@cwcom.net>,
 *                                 Philip Susi <phreak@iag.net>,
 *                                 Eric Kohl <ekohl@abo.rhein-zeitung.de>
 *                                 Alex Ionescu <alex@relsoft.net>
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
/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/dpc.c
 * PURPOSE:         Handle DPCs (Delayed Procedure Calls)
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                28/05/98: Created
 *                12/3/99:  Phillip Susi: Fixed IRQL problem
 *                12/11/04: Alex Ionescu - Major rewrite.
 */

/*
 * NOTE: See also the higher level support routines in ntoskrnl/io/dpc.c
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

#define MAX_QUANTUM 0x7F
/* GLOBALS ******************************************************************/

/* FUNCTIONS ****************************************************************/

VOID INIT_FUNCTION
KeInitDpc(PKPCR Pcr)
/*
 * FUNCTION: Initialize DPC handling
 */
{
   InitializeListHead(&Pcr->PrcbData.DpcData[0].DpcListHead);
   KeInitializeEvent(Pcr->PrcbData.DpcEvent, 0, 0);
   KeInitializeSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
   Pcr->PrcbData.MaximumDpcQueueDepth = 4;
   Pcr->PrcbData.MinimumDpcRate = 3;
   Pcr->PrcbData.DpcData[0].DpcQueueDepth = 0;    
}

/*
 * @implemented
 */
VOID
STDCALL
KeInitializeThreadedDpc(PKDPC			Dpc,
		 	PKDEFERRED_ROUTINE	DeferredRoutine,
		 	PVOID			DeferredContext)
/*
 * FUNCTION: 
 *          Initalizes a Threaded DPC and registers the DeferredRoutine for it.
 * ARGUMENTS:
 *          Dpc = Pointer to a caller supplied DPC to be initialized. The caller must allocate this memory.
 *          DeferredRoutine = Pointer to associated DPC callback routine.
 *          DeferredContext = Parameter to be passed to the callback routine.
 * NOTE: Callers can be running at any IRQL.
 */
{
	DPRINT("Threaded DPC Initializing: %x with Routine: %x\n", Dpc, DeferredRoutine);
	//Dpc->Type = KThreadedDpc;
	Dpc->Number= 0;
	Dpc->Importance= MediumImportance;
	Dpc->DeferredRoutine = DeferredRoutine;
	Dpc->DeferredContext = DeferredContext;
	Dpc->DpcData = NULL;
}

/*
 * @implemented
 */
VOID
STDCALL
KeInitializeDpc (PKDPC			Dpc,
		 PKDEFERRED_ROUTINE	DeferredRoutine,
		 PVOID			DeferredContext)
/*
 * FUNCTION: 
 *          Initalizes a DPC and registers the DeferredRoutine for it.
 * ARGUMENTS:
 *          Dpc = Pointer to a caller supplied DPC to be initialized. The caller must allocate this memory.
 *          DeferredRoutine = Pointer to associated DPC callback routine.
 *          DeferredContext = Parameter to be passed to the callback routine.
 * NOTE: Callers can be running at any IRQL.
 */
{
	DPRINT("DPC Initializing: %x with Routine: %x\n", Dpc, DeferredRoutine);
	Dpc->Type = KDpc;
	Dpc->Number= 0;
	Dpc->Importance= MediumImportance;
	Dpc->DeferredRoutine = DeferredRoutine;
	Dpc->DeferredContext = DeferredContext;
	Dpc->DpcData = NULL;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeInsertQueueDpc (PKDPC	Dpc,
		  PVOID	SystemArgument1,
		  PVOID	SystemArgument2)
/*
 * FUNCTION: 
 *          Queues a DPC for execution when the IRQL of a processor
 *          drops below DISPATCH_LEVEL
 * ARGUMENTS:
 *          Dpc = Pointed to a DPC Object Initalized by KeInitializeDpc.
 *          SystemArgument1 = Driver Determined context data
 *          SystemArgument2 = Driver Determined context data
 * RETURNS: 
 *          TRUE if the DPC object wasn't already in the queue
 *          FALSE otherwise
 * NOTES:  
 *          If there is currently a DPC active on the target processor, or a DPC
 * interrupt has already been requested on the target processor when a
 * DPC is queued, then no further action is necessary. The DPC will be
 * executed on the target processor when its queue entry is processed.
 *
 *          If there is not a DPC active on the target processor and a DPC interrupt
 * has not been requested on the target processor, then the exact treatment
 * of the DPC is dependent on whether the host system is a UP system or an
 * MP system.
 *
 * UP system.
 * ----------
 *          If the DPC is of medium or high importance, the current DPC queue depth
 * is greater than the maximum target depth, or current DPC request rate is
 * less the minimum target rate, then a DPC interrupt is requested on the
 * host processor and the DPC will be processed when the interrupt occurs.
 * Otherwise, no DPC interupt is requested and the DPC execution will be
 * delayed until the DPC queue depth is greater that the target depth or the
 * minimum DPC rate is less than the target rate.
 *
 * MP system.
 * ----------
 *          If the DPC is being queued to another processor and the depth of the DPC
 * queue on the target processor is greater than the maximum target depth or
 * the DPC is of high importance, then a DPC interrupt is requested on the
 * target processor and the DPC will be processed when the interrupt occurs.
 * Otherwise, the DPC execution will be delayed on the target processor until
 * the DPC queue depth on the target processor is greater that the maximum
 * target depth or the minimum DPC rate on the target processor is less than
 * the target mimimum rate.
 *
 *          If the DPC is being queued to the current processor and the DPC is not of
 * low importance, the current DPC queue depth is greater than the maximum
 * target depth, or the minimum DPC rate is less than the minimum target rate,
 * then a DPC interrupt is request on the current processor and the DPV will
 * be processed whne the interrupt occurs. Otherwise, no DPC interupt is
 * requested and the DPC execution will be delayed until the DPC queue depth
 * is greater that the target depth or the minimum DPC rate is less than the
 * target rate.
 */
{
	KIRQL OldIrql;
	PKPCR Pcr;

	DPRINT("KeInsertQueueDpc(DPC %x, SystemArgument1 %x, SystemArgument2 %x)\n",
		Dpc, SystemArgument1, SystemArgument2);

	/* Check IRQL and Raise it to HIGH_LEVEL */
	ASSERT(KeGetCurrentIrql()>=DISPATCH_LEVEL);
	KeRaiseIrql(HIGH_LEVEL, &OldIrql);
	
	/* Check if this is a Thread DPC, which we don't support (yet) */
	//if (Dpc->Type == KThreadedDpc) {
	//	return FALSE;
	//	KeLowerIrql(OldIrql);
	//}

#ifdef MP
	/* Get the right PCR for this CPU */
	if (Dpc->Number >= MAXIMUM_PROCESSORS) {
		ASSERT (Dpc->Number - MAXIMUM_PROCESSORS < KeNumberProcessors);
		Pcr = (PKPCR)(KPCR_BASE + (Dpc->Number - MAXIMUM_PROCESSORS) * PAGE_SIZE);
	} else {
		ASSERT (Dpc->Number < KeNumberProcessors);
		Pcr = KeGetCurrentKPCR();
		Dpc->Number = KeGetCurrentProcessorNumber();
	}
	KiAcquireSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
#else
	Pcr = (PKPCR)KPCR_BASE;
#endif

	/* Get the DPC Data */
	if (InterlockedCompareExchangeUL(&Dpc->DpcData, &Pcr->PrcbData.DpcData[0].DpcLock, 0)) {
		DPRINT("DPC Already Inserted");
#ifdef MP
		KiReleaseSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
#endif
		KeLowerIrql(OldIrql);
		return(FALSE);
	}
	
	/* Make sure the lists are free if the Queue is 0 */
	if (Pcr->PrcbData.DpcData[0].DpcQueueDepth == 0) {
		ASSERT(IsListEmpty(&Pcr->PrcbData.DpcData[0].DpcListHead));
	} else {
		ASSERT(!IsListEmpty(&Pcr->PrcbData.DpcData[0].DpcListHead));    
	}

	/* Now we can play with the DPC safely */
	Dpc->SystemArgument1=SystemArgument1;
	Dpc->SystemArgument2=SystemArgument2;
	Pcr->PrcbData.DpcData[0].DpcQueueDepth++;
	Pcr->PrcbData.DpcData[0].DpcCount++;
	
	/* Insert the DPC into the list. HighImportance DPCs go at the beginning  */
	if (Dpc->Importance == HighImportance) {
		InsertHeadList(&Pcr->PrcbData.DpcData[0].DpcListHead, &Dpc->DpcListEntry);
	} else { 
		InsertTailList(&Pcr->PrcbData.DpcData[0].DpcListHead, &Dpc->DpcListEntry);
	}
	DPRINT("New DPC Added. Dpc->DpcListEntry.Flink %x\n", Dpc->DpcListEntry.Flink);
   
	/* Make sure a DPC isn't executing already and respect rules outlined above. */
	if ((!Pcr->PrcbData.DpcRoutineActive) && (!Pcr->PrcbData.DpcInterruptRequested)) {
		
#ifdef MP	
		/* Check if this is the same CPU */
		if (Pcr != KeGetCurrentKPCR()) {
			/* Send IPI if High Importance */
			if ((Dpc->Importance == HighImportance) ||
			    (Pcr->PrcbData.DpcData[0].DpcQueueDepth >= Pcr->PrcbData.MaximumDpcQueueDepth)) {
				if (Dpc->Number >= MAXIMUM_PROCESSORS) {
				    KiIpiSendRequest(1 << (Dpc->Number - MAXIMUM_PROCESSORS), IPI_REQUEST_DPC);
				} else {
				    KiIpiSendRequest(1 << Dpc->Number, IPI_REQUEST_DPC);
				}

			}
		} else {
			/* Request an Interrupt only if the DPC isn't low priority */
			if ((Dpc->Importance != LowImportance) || 
			     (Pcr->PrcbData.DpcData[0].DpcQueueDepth >= Pcr->PrcbData.MaximumDpcQueueDepth) ||
				(Pcr->PrcbData.DpcRequestRate < Pcr->PrcbData.MinimumDpcRate)) {
			
				/* Request Interrupt */
				HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
				Pcr->PrcbData.DpcInterruptRequested = TRUE;
			}
		}
#else
		DPRINT("Requesting Interrupt. Importance: %x. QueueDepth: %x. MaxQueue: %x . RequestRate: %x. MinRate:%x \n", Dpc->Importance, Pcr->PrcbData.DpcData[0].DpcQueueDepth, Pcr->PrcbData.MaximumDpcQueueDepth, Pcr->PrcbData.DpcRequestRate, Pcr->PrcbData.MinimumDpcRate);
		/* Request an Interrupt only if the DPC isn't low priority */
		if ((Dpc->Importance != LowImportance) || 
		     (Pcr->PrcbData.DpcData[0].DpcQueueDepth >= Pcr->PrcbData.MaximumDpcQueueDepth) ||
			(Pcr->PrcbData.DpcRequestRate < Pcr->PrcbData.MinimumDpcRate)) {
			
			/* Request Interrupt */
			DPRINT("Requesting Interrupt\n");
			HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
			Pcr->PrcbData.DpcInterruptRequested = TRUE;
		}
#endif
	}
#ifdef MP
	KiReleaseSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
#endif
	/* Lower IRQL */	
	KeLowerIrql(OldIrql);
	return(TRUE);
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeRemoveQueueDpc (PKDPC	Dpc)
/*
 * FUNCTION: 
 *          Removes DPC object from the system dpc queue
 * ARGUMENTS:
 *          Dpc = Pointer to DPC to remove from the queue.
 * RETURNS: 
 *          TRUE if the DPC was in the queue
 *          FALSE otherwise
 */
{
	BOOLEAN WasInQueue;
	KIRQL OldIrql;
	
	/* Raise IRQL */
	DPRINT("Removing DPC: %x\n", Dpc);
	KeRaiseIrql(HIGH_LEVEL, &OldIrql);
#ifdef MP
	KiAcquireSpinLock(&((PKDPC_DATA)Dpc->DpcData)->DpcLock);
#endif
	
	/* First make sure the DPC lock isn't being held */
	WasInQueue = Dpc->DpcData ? TRUE : FALSE;
	if (Dpc->DpcData) {	
		
		/* Remove the DPC */
		((PKDPC_DATA)Dpc->DpcData)->DpcQueueDepth--;
		RemoveEntryList(&Dpc->DpcListEntry);

	}
#ifdef MP
        KiReleaseSpinLock(&((PKDPC_DATA)Dpc->DpcData)->DpcLock);
#endif

	/* Return if the DPC was in the queue or not */
	KeLowerIrql(OldIrql);
	return WasInQueue;
}

/*
 * @implemented
 */
VOID
STDCALL
KeFlushQueuedDpcs(VOID)
/*
 * FUNCTION: 
 *          Called to Deliver DPCs if any are pending.
 * NOTES:
 *          Called when deleting a Driver.
 */
{
	if (KeGetCurrentKPCR()->PrcbData.DpcData[0].DpcQueueDepth) HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
BOOLEAN 
STDCALL
KeIsExecutingDpc(
	VOID
)
{
 	return KeGetCurrentKPCR()->PrcbData.DpcRoutineActive;
}

/*
 * FUNCTION: Specifies the DPCs importance
 * ARGUMENTS:
 *          Dpc = Initalizes DPC
 *          Importance = DPC importance
 * RETURNS: None
 *
 * @implemented
 */
VOID 
STDCALL
KeSetImportanceDpc (IN	PKDPC		Dpc,
		    IN	KDPC_IMPORTANCE	Importance)
{
	Dpc->Importance = Importance;
}

/*
 * FUNCTION: Specifies on which processor the DPC will run
 * ARGUMENTS:
 *          Dpc = Initalizes DPC
 *          Number = Processor number
 * RETURNS: None
 *
 * @implemented
 */
VOID STDCALL
KeSetTargetProcessorDpc (IN	PKDPC	Dpc,
			 IN	CCHAR	Number)
{
   if (Number >= MAXIMUM_PROCESSORS)
   {
      Dpc->Number = 0;
   }
   else
   {
      ASSERT(Number < KeNumberProcessors);
      Dpc->Number = Number + MAXIMUM_PROCESSORS;
   }
}

VOID
STDCALL
KiQuantumEnd(VOID)
/*
 * FUNCTION: 
 *          Called when a quantum end occurs to check if priority should be changed
 *          and wether a new thread should be dispatched.
 * NOTES:
 *          Called when deleting a Driver.
 */
{
	PKPRCB Prcb;
	PKTHREAD CurrentThread;
	KIRQL OldIrql;
	PKPROCESS Process;
	KPRIORITY OldPriority;
	KPRIORITY NewPriority;
	
	/* Lock dispatcher, get current thread */
	Prcb = &KeGetCurrentKPCR()->PrcbData;
	CurrentThread = KeGetCurrentThread();
	OldIrql = KeRaiseIrqlToSynchLevel();
	
	/* Get the Thread's Process */
	Process = CurrentThread->ApcState.Process;
	
	/* Set DPC Event if requested */
	if (Prcb->DpcSetEventRequest) {
		KeSetEvent(Prcb->DpcEvent, 0, 0);
	}
	
	/* Check if Quantum expired */
	if (CurrentThread->Quantum <= 0) {
		/* Set the new Quantum */
		CurrentThread->Quantum = Process->ThreadQuantum;
		
		/* Calculate new priority */
		OldPriority = CurrentThread->Priority;
		if (OldPriority < LOW_REALTIME_PRIORITY) {
			NewPriority = OldPriority - CurrentThread->PriorityDecrement - 1;
   			if (NewPriority < CurrentThread->BasePriority) {
				NewPriority = CurrentThread->BasePriority;
			}
			CurrentThread->PriorityDecrement = 0;
   			if (OldPriority != NewPriority) {
				/* Set new Priority */
				CurrentThread->Priority = NewPriority;
			} else {
				/* Queue new thread if none is already */
				if (Prcb->NextThread == NULL) {
					/* FIXME: Schedule a New Thread, when ROS will have NT Scheduler */
				} else {
					/* Make the current thread non-premeptive if a new thread is queued */
					CurrentThread->Preempted = FALSE;
				}
			}
		} else {
			/* Set the Quantum back to Maximum */
			//if (CurrentThread->DisableQuantum) {
			//	CurrentThread->Quantum = MAX_QUANTUM;
			//}
		}
	}
	/* Dispatch the Thread */
	KeLowerIrql(DISPATCH_LEVEL);
	PsDispatchThread(THREAD_STATE_READY);
}	

/*
 * @implemented
 */
VOID
STDCALL
KiDispatchInterrupt(VOID)
/*
 * FUNCTION: 
 *          Called whenever a system interrupt is generated at DISPATCH_LEVEL.
 *          It delivers queued DPCs and dispatches a new thread if need be.
 */
{
	PLIST_ENTRY DpcEntry;
	PKDPC Dpc;
	KIRQL OldIrql;
	PKPCR Pcr;

	DPRINT("Dispatching Interrupts\n");
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	/* Set DPC Deliver to Active */
	Pcr = KeGetCurrentKPCR();

	if (Pcr->PrcbData.DpcData[0].DpcQueueDepth > 0) {
		/* Raise IRQL */
		KeRaiseIrql(HIGH_LEVEL, &OldIrql);
#ifdef MP		
		KiAcquireSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
#endif
	        Pcr->PrcbData.DpcRoutineActive = TRUE;

		DPRINT("&Pcr->PrcbData.DpcData[0].DpcListHead: %x\n", &Pcr->PrcbData.DpcData[0].DpcListHead);
		/* Loop while we have entries */
		while (!IsListEmpty(&Pcr->PrcbData.DpcData[0].DpcListHead)) {
			ASSERT(Pcr->PrcbData.DpcData[0].DpcQueueDepth > 0);
			DPRINT("Queue Depth: %x\n", Pcr->PrcbData.DpcData[0].DpcQueueDepth);
			
			/* Get the DPC call it */
			DpcEntry = RemoveHeadList(&Pcr->PrcbData.DpcData[0].DpcListHead);
			Dpc = CONTAINING_RECORD(DpcEntry, KDPC, DpcListEntry);
			DPRINT("Dpc->DpcListEntry.Flink %x\n", Dpc->DpcListEntry.Flink);
			Dpc->DpcData = NULL;
			Pcr->PrcbData.DpcData[0].DpcQueueDepth--;
#ifdef MP
			KiReleaseSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
#endif
			/* Disable/Enabled Interrupts and Call the DPC */
         		KeLowerIrql(OldIrql);
			DPRINT("Calling DPC: %x\n", Dpc);
			Dpc->DeferredRoutine(Dpc,
					     Dpc->DeferredContext,
					     Dpc->SystemArgument1,
					     Dpc->SystemArgument2);
			KeRaiseIrql(HIGH_LEVEL, &OldIrql);
			
#ifdef MP
			KiAcquireSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
			/* 
			 * If the dpc routine drops the irql below DISPATCH_LEVEL,
			 * a thread switch can occur and after the next thread switch 
			 * the execution may start on an other processor.
			 */
			if (Pcr != KeGetCurrentKPCR()) {
		                Pcr->PrcbData.DpcRoutineActive = FALSE;
				KiReleaseSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
				Pcr = KeGetCurrentKPCR();
				KiAcquireSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
				Pcr->PrcbData.DpcRoutineActive = TRUE;
			}
#endif
		}
		/* Clear DPC Flags */
		Pcr->PrcbData.DpcRoutineActive = FALSE;
		Pcr->PrcbData.DpcInterruptRequested = FALSE;
#ifdef MP
		KiReleaseSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
#endif
		
		/* DPC Dispatching Ended, re-enable interrupts */
		KeLowerIrql(OldIrql);
	}
	
	DPRINT("Checking for Quantum End\n");
	/* If we have Quantum End, call the function */
	if (Pcr->PrcbData.QuantumEnd) {
		Pcr->PrcbData.QuantumEnd = FALSE;
		KiQuantumEnd();
	}
}

/* EOF */
