/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/dpc.c
 * PURPOSE:         Handle DPCs (Delayed Procedure Calls)
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                28/05/98: Created
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

static LIST_ENTRY DpcQueueHead={NULL,NULL};
static KSPIN_LOCK DpcQueueLock={0,};
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
   
   DPRINT("KeDrainDpcQueue()\n");
   
   KeAcquireSpinLockAtDpcLevel(&DpcQueueLock);
   KeRaiseIrql(HIGH_LEVEL,&oldlvl);
   current_entry = RemoveHeadList(&DpcQueueHead);
   KeLowerIrql(oldlvl);
   current = CONTAINING_RECORD(current_entry,KDPC,DpcListEntry);
   while (current_entry!=(&DpcQueueHead))
     {
	CHECKPOINT;
	current->DeferredRoutine(current,current->DeferredContext,
				 current->SystemArgument1,
				 current->SystemArgument2);
	current->Lock=FALSE;
	KeRaiseIrql(HIGH_LEVEL,&oldlvl);
	current_entry = RemoveHeadList(&DpcQueueHead);
	DpcQueueSize--;
	KeLowerIrql(oldlvl);
	current = CONTAINING_RECORD(&current_entry,KDPC,DpcListEntry);
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
   if (!Dpc->Lock)
     {
	return(FALSE);
     }
   RemoveEntryList(&Dpc->DpcListEntry);
   DpcQueueSize--;
   Dpc->Lock=0;
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
   DPRINT("KeInsertQueueDpc()\n",0);
   assert(KeGetCurrentIrql()>=DISPATCH_LEVEL);
   
   dpc->Number=0;
   dpc->Importance=Medium;
   dpc->SystemArgument1=SystemArgument1;
   dpc->SystemArgument2=SystemArgument2;
   if (dpc->Lock)
     {
	return(FALSE);
     }
   KeAcquireSpinLockAtDpcLevel(&DpcQueueLock);
   InsertHeadList(&DpcQueueHead,&dpc->DpcListEntry);
   DpcQueueSize++;
   KeReleaseSpinLockFromDpcLevel(&DpcQueueLock);
   dpc->Lock=(PULONG)1;
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
}


