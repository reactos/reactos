/*
 *  ReactOS kernel
 *  Copyright (C) 2000, 1999, 1998 David Welch <welch@cwcom.net>,
 *                                 Philip Susi <phreak@iag.net>,
 *                                 Eric Kohl <ekohl@abo.rhein-zeitung.de>
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
/* $Id: dpc.c,v 1.44 2004/10/31 17:02:31 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/dpc.c
 * PURPOSE:         Handle DPCs (Delayed Procedure Calls)
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                28/05/98: Created
 *                12/3/99:  Phillip Susi: Fixed IRQL problem
 */

/*
 * NOTE: See also the higher level support routines in ntoskrnl/io/dpc.c
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
VOID STDCALL
KeInitializeDpc (PKDPC			Dpc,
		 PKDEFERRED_ROUTINE	DeferredRoutine,
		 PVOID			DeferredContext)
/*
 * FUNCTION: Initalizes a DPC
 * ARGUMENTS:
 *          Dpc = Caller supplied DPC to be initialized
 *          DeferredRoutine = Associated DPC callback
 *          DeferredContext = Parameter to be passed to the callback
 * NOTE: Callers must be running at IRQL PASSIVE_LEVEL
 */
{
   Dpc->Type = 0;
   Dpc->Number=0;
   Dpc->Importance=MediumImportance;
   Dpc->DeferredRoutine = DeferredRoutine;
   Dpc->DeferredContext = DeferredContext;
   Dpc->Lock = 0;
}

/*
 * @implemented
 */
VOID STDCALL
KiDispatchInterrupt(VOID)
/*
 * FUNCTION: Called to execute queued dpcs
 */
{
   PLIST_ENTRY current_entry;
   PKDPC current;
   KIRQL oldlvl;
   PKPCR Pcr;
   PKTHREAD CurrentThread;
   PKPROCESS CurrentProcess;

   ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

   Pcr = KeGetCurrentKPCR();

   if (Pcr->PrcbData.DpcData[0].DpcQueueDepth > 0)
     {

       KeRaiseIrql(HIGH_LEVEL, &oldlvl);
       KiAcquireSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);

       while (!IsListEmpty(&Pcr->PrcbData.DpcData[0].DpcListHead))
       {
         ASSERT(Pcr->PrcbData.DpcData[0].DpcQueueDepth > 0);

         current_entry = RemoveHeadList(&Pcr->PrcbData.DpcData[0].DpcListHead);
         Pcr->PrcbData.DpcData[0].DpcQueueDepth--;
         Pcr->PrcbData.DpcData[0].DpcCount++;

	 if (Pcr->PrcbData.DpcData[0].DpcQueueDepth == 0)
	 {
	    ASSERT(IsListEmpty(&Pcr->PrcbData.DpcData[0].DpcListHead));
	 }
	 else
	 {
	    ASSERT(!IsListEmpty(&Pcr->PrcbData.DpcData[0].DpcListHead));	    
	 }

         current = CONTAINING_RECORD(current_entry,KDPC,DpcListEntry);
         current->Lock=FALSE;
         Pcr->PrcbData.DpcRoutineActive++;
         KiReleaseSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
         KeLowerIrql(oldlvl);
         current->DeferredRoutine(current,current->DeferredContext,
			          current->SystemArgument1,
			          current->SystemArgument2);

         KeRaiseIrql(HIGH_LEVEL, &oldlvl);
         KiAcquireSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
         Pcr->PrcbData.DpcRoutineActive--;
#ifdef MP
	 /* 
	  * If the dpc routine drops the irql below DISPATCH_LEVEL,
	  * a thread switch can occur and after the next thread switch 
	  * the execution may start on an other processor.
	  */
	 if (Pcr != KeGetCurrentKPCR())
	 {
           KiReleaseSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
	   Pcr = KeGetCurrentKPCR();
           KiAcquireSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
	 }
#endif
       }

       KiReleaseSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
       KeLowerIrql(oldlvl);
     }
   if (Pcr->PrcbData.QuantumEnd)
     {
       /*
        * FIXME: Various special cases should be handled here. The scripts
        * from David B. Probert that describe it under KiQuantumEnd.
        */
       CurrentThread = /* Pcr->PcrbData.CurrentThread */ KeGetCurrentThread();
       CurrentProcess = CurrentThread->ApcState.Process;
       CurrentThread->Quantum = CurrentProcess->ThreadQuantum;
       Pcr->PrcbData.QuantumEnd = FALSE;
       PsDispatchThread(THREAD_STATE_READY);
     }

}

/*
 * @unimplemented
 */
VOID
STDCALL
KeFlushQueuedDpcs(
	VOID
	)
{
	UNIMPLEMENTED;
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
 * @implemented
 */
BOOLEAN STDCALL
KeRemoveQueueDpc (PKDPC	Dpc)
/*
 * FUNCTION: Removes DPC object from the system dpc queue
 * ARGUMENTS:
 *          Dpc = DPC to remove
 * RETURNS: TRUE if the DPC was in the queue
 *          FALSE otherwise
 */
{
   KIRQL oldIrql;
   BOOLEAN WasInQueue;
   PKPCR Pcr;

   KeRaiseIrql(HIGH_LEVEL, &oldIrql);
#ifdef MP
   if (Dpc->Number >= MAXIMUM_PROCESSORS)
   {
      ASSERT (Dpc->Number - MAXIMUM_PROCESSORS < KeNumberProcessors);
      Pcr = (PKPCR)(KPCR_BASE + (Dpc->Number - MAXIMUM_PROCESSORS) * PAGE_SIZE);
   }
   else
   {
      ASSERT (Dpc->Number < KeNumberProcessors);
      Pcr = (PKPCR)(KPCR_BASE + Dpc->Number * PAGE_SIZE);
   }
#else
   Pcr = KeGetCurrentKPCR();
#endif
   KiAcquireSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
   WasInQueue = Dpc->Lock ? TRUE : FALSE;
   if (WasInQueue)
     {
	RemoveEntryList(&Dpc->DpcListEntry);
	Pcr->PrcbData.DpcData[0].DpcQueueDepth--;
	Dpc->Lock=0;
     }

   if (Pcr->PrcbData.DpcData[0].DpcQueueDepth == 0)
   {
     ASSERT(IsListEmpty(&Pcr->PrcbData.DpcData[0].DpcListHead));
   }
   else
   {
     ASSERT(!IsListEmpty(&Pcr->PrcbData.DpcData[0].DpcListHead));	    
   }

   KiReleaseSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
   KeLowerIrql(oldIrql);

   return WasInQueue;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeInsertQueueDpc (PKDPC	Dpc,
		  PVOID	SystemArgument1,
		  PVOID	SystemArgument2)
/*
 * FUNCTION: Queues a DPC for execution when the IRQL of a processor
 * drops below DISPATCH_LEVEL
 * ARGUMENTS:
 *          Dpc = Initalizes DPC
 *          SystemArguments[1-2] = Undocumented
 * RETURNS: TRUE if the DPC object wasn't already in the queue
 *          FALSE otherwise
 */
{
   KIRQL oldlvl;
   PKPCR Pcr;

   DPRINT("KeInsertQueueDpc(dpc %x, SystemArgument1 %x, SystemArgument2 %x)\n",
	  Dpc, SystemArgument1, SystemArgument2);

   ASSERT(KeGetCurrentIrql()>=DISPATCH_LEVEL);

   Dpc->SystemArgument1=SystemArgument1;
   Dpc->SystemArgument2=SystemArgument2;
   if (Dpc->Lock)
     {
	return(FALSE);
     }

   KeRaiseIrql(HIGH_LEVEL, &oldlvl);
#ifdef MP
   if (Dpc->Number >= MAXIMUM_PROCESSORS)
   {
      ASSERT (Dpc->Number - MAXIMUM_PROCESSORS < KeNumberProcessors);
      Pcr = (PKPCR)(KPCR_BASE + (Dpc->Number - MAXIMUM_PROCESSORS) * PAGE_SIZE);
   }
   else
   {
      ASSERT (Dpc->Number < KeNumberProcessors);
      Pcr = KeGetCurrentKPCR();
      Dpc->Number = KeGetCurrentProcessorNumber();
   }
#else
   Pcr = KeGetCurrentKPCR();
#endif

   KiAcquireSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
   if (Pcr->PrcbData.DpcData[0].DpcQueueDepth == 0)
   {
     ASSERT(IsListEmpty(&Pcr->PrcbData.DpcData[0].DpcListHead));
   }
   else
   {
     ASSERT(!IsListEmpty(&Pcr->PrcbData.DpcData[0].DpcListHead));	    
   }
   InsertHeadList(&Pcr->PrcbData.DpcData[0].DpcListHead,&Dpc->DpcListEntry);
   DPRINT("Dpc->DpcListEntry.Flink %x\n", Dpc->DpcListEntry.Flink);
   Pcr->PrcbData.DpcData[0].DpcQueueDepth++;
   Dpc->Lock=(PULONG)1;
   if (Pcr->PrcbData.MaximumDpcQueueDepth < Pcr->PrcbData.DpcData[0].DpcQueueDepth)
     {
       Pcr->PrcbData.MaximumDpcQueueDepth = Pcr->PrcbData.DpcData[0].DpcQueueDepth;    
     }
   KiReleaseSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
   KeLowerIrql(oldlvl);
   DPRINT("DpcQueueHead.Flink %x\n", Pcr->PrcbData.DpcData[0].DpcListHead.Flink);
   DPRINT("Leaving KeInsertQueueDpc()\n",0);
   return(TRUE);
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
VOID STDCALL
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
 * @unimplemented
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

VOID INIT_FUNCTION
KeInitDpc(PKPCR Pcr)
/*
 * FUNCTION: Initialize DPC handling
 */
{
   InitializeListHead(&Pcr->PrcbData.DpcData[0].DpcListHead);
   KeInitializeSpinLock(&Pcr->PrcbData.DpcData[0].DpcLock);
   Pcr->PrcbData.MaximumDpcQueueDepth = 0;
   Pcr->PrcbData.DpcData[0].DpcQueueDepth = 0;    
}

/* EOF */
