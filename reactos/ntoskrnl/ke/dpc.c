/*
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

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

static LIST_ENTRY DpcQueueHead;
static KSPIN_LOCK DpcQueueLock;
ULONG DpcQueueSize = 0;

/* FUNCTIONS ****************************************************************/

VOID KeInitializeDpc(PKDPC Dpc, PKDEFERRED_ROUTINE DeferredRoutine,
		     PVOID DeferredContext)
/*
 * FUNCTION: Initalizes a DPC
 * ARGUMENTS:
 *          Dpc = Caller supplied DPC to be initialized
 *          DeferredRoutine = Associated DPC callback
 *          DeferredContext = Parameter to be passed to the callback
 * NOTE: Callers must be running at IRQL PASSIVE_LEVEL
 */
{
   Dpc->Type=0;
   Dpc->DeferredRoutine=DeferredRoutine;
   Dpc->DeferredContext=DeferredContext;
   Dpc->Lock=0;
}

void KeDrainDpcQueue(void)
/*
 * FUNCTION: Called to execute queued dpcs
 */
{
   PLIST_ENTRY current_entry;
   PKDPC current;
   KIRQL oldlvl;
   
   assert_irql(DISPATCH_LEVEL);
   
   if (DpcQueueSize == 0)
     {
	return;
     }
   DPRINT("KeDrainDpcQueue()\n");
   
   KeAcquireSpinLockAtDpcLevel(&DpcQueueLock);
   KeRaiseIrql(HIGH_LEVEL,&oldlvl);
   current_entry = RemoveHeadList(&DpcQueueHead);
   KeLowerIrql(oldlvl);
   current = CONTAINING_RECORD(current_entry,KDPC,DpcListEntry);
   while (current_entry!=(&DpcQueueHead))
     {
	CHECKPOINT;
	DPRINT("DpcQueueSize %d current %x current->DeferredContext %x\n", 
	       DpcQueueSize, current, current->DeferredContext);
	DPRINT("current->Flink %x\n", current->DpcListEntry.Flink);
	current->DeferredRoutine(current,current->DeferredContext,
				 current->SystemArgument1,
				 current->SystemArgument2);
	CHECKPOINT;
	current->Lock=FALSE;
	KeRaiseIrql(HIGH_LEVEL,&oldlvl);
	current_entry = RemoveHeadList(&DpcQueueHead);
	DPRINT("current_entry %x\n", current_entry);
	DpcQueueSize--;
	KeLowerIrql(oldlvl);
	current = CONTAINING_RECORD(current_entry,KDPC,DpcListEntry);
	DPRINT("current %x\n", current);
     }
   KeReleaseSpinLockFromDpcLevel(&DpcQueueLock);
}

BOOLEAN KeRemoveQueueDpc(PKDPC Dpc)
/*
 * FUNCTION: Removes DPC object from the system dpc queue
 * ARGUMENTS:
 *          Dpc = DPC to remove
 * RETURNS: TRUE if the DPC was in the queue
 *          FALSE otherwise
 */
{
   KIRQL oldIrql;
   
   KeAcquireSpinLockAtDpcLevel( &DpcQueueLock );
   KeRaiseIrql( HIGH_LEVEL, &oldIrql );
   if (!Dpc->Lock)
     {
	KeReleaseSpinLock(&DpcQueueLock, oldIrql);
	return(FALSE);
     }
   RemoveEntryList(&Dpc->DpcListEntry);
   DpcQueueSize--;
   Dpc->Lock=0;
   KeReleaseSpinLock(&DpcQueueLock, oldIrql);
   return(TRUE);
}

BOOLEAN KeInsertQueueDpc(PKDPC dpc, PVOID SystemArgument1,
			 PVOID SystemArgument2)
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
   DPRINT("KeInsertQueueDpc(dpc %x, SystemArgument1 %x, SystemArgument2 %x)\n",
	  dpc, SystemArgument1, SystemArgument2);

   assert(KeGetCurrentIrql()>=DISPATCH_LEVEL);

   dpc->Number=0;
   dpc->Importance=Medium;
   dpc->SystemArgument1=SystemArgument1;
   dpc->SystemArgument2=SystemArgument2;
   if (dpc->Lock)
     {
	return(FALSE);
     }
   KeRaiseIrql( HIGH_LEVEL, &oldlvl );
   KeAcquireSpinLockAtDpcLevel(&DpcQueueLock);
   InsertHeadList(&DpcQueueHead,&dpc->DpcListEntry);
   DPRINT("dpc->DpcListEntry.Flink %x\n", dpc->DpcListEntry.Flink);
   DpcQueueSize++;
   dpc->Lock=(PULONG)1;
   KeReleaseSpinLock( &DpcQueueLock, oldlvl );
   DPRINT("DpcQueueHead.Flink %x\n",DpcQueueHead.Flink);
   DPRINT("Leaving KeInsertQueueDpc()\n",0);
   return(TRUE);
}

void KeInitDpc(void)
/*
 * FUNCTION: Initialize DPC handling
 */
{
   InitializeListHead(&DpcQueueHead);
   KeInitializeSpinLock(&DpcQueueLock);
}

