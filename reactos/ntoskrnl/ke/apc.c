/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/apc.c
 * PURPOSE:         Possible implementation of APCs
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>
#include <internal/string.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID KeDrainApcQueue(VOID)
{
   PLIST_ENTRY current_entry;
   PKAPC current;
   
   current_entry = KeGetCurrentThread()->ApcQueueHead.Flink;
   while (current_entry!=NULL)
     {
	current = CONTAINING_RECORD(current_entry,KAPC,ApcListEntry);
	current->NormalRoutine(current->NormalContext,
			       current->SystemArgument1,
			       current->SystemArgument2);
	current_entry = current_entry->Flink;
     }
}

VOID KeInitializeApc(PKAPC Apc, PKNORMAL_ROUTINE NormalRoutine,
		     PVOID NormalContext,
		     PKTHREAD TargetThread)
{
   memset(Apc,0,sizeof(KAPC));
   Apc->Thread = TargetThread;
   Apc->NormalRoutine=NormalRoutine;
   Apc->NormalContext=NormalContext;
   Apc->Inserted=FALSE;
}

BOOLEAN KeInsertQueueApc(PKAPC Apc)
{
   if (Apc->Inserted)
     {
	return(FALSE);
     }
   Apc->Inserted=TRUE;
   InsertTailList(&Apc->Thread->ApcQueueHead,&Apc->ApcListEntry);
   return(TRUE);
}

