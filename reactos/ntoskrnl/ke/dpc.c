/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            mkernel/kernel/dpc.cc
 * PURPOSE:         Handle DPCs (Delayed Procedure Calls)
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/*
 * NOTE: See also the higher level support routines in mkernel/iomgr/iodpc.cc
 */

/* INCLUDES ***************************************************************/

#include <ddk/ntddk.h>

#include <internal/kernel.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

LIST_ENTRY DpcQueueHead;

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
   PLIST_ENTRY current_entry = ExInterlockedRemoveHeadList(&DpcQueueHead,NULL);
   PKDPC current = CONTAINING_RECORD(&current_entry,KDPC,DpcListEntry);
   
   while (current_entry!=NULL)
     {
	current->DeferredRoutine(current,current->DeferredContext,
				 current->SystemArgument1,
				 current->SystemArgument2);
	current_entry = ExInterlockedRemoveHeadList(&DpcQueueHead,NULL);
	current = CONTAINING_RECORD(&current_entry,KDPC,DpcListEntry);
     }
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
   ExInterlockedRemoveEntryList(&DpcQueueHead,&Dpc->DpcListEntry,NULL);
   Dpc->Lock=0;
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
   dpc->Number=0;
   dpc->Importance=Medium;
   dpc->SystemArgument1=SystemArgument1;
   dpc->SystemArgument2=SystemArgument2;
   if (dpc->Lock)
     {
	return(FALSE);
     }
   ExInterlockedInsertHeadList(&DpcQueueHead,&dpc->DpcListEntry,NULL);
   dpc->Lock=1;
   return(TRUE);
}

void KeInitDpc(void)
/*
 * FUNCTION: Initialize DPC handling
 */
{
   InitializeListHead(&DpcQueueHead);
}


