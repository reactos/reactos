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

#include <ddk/ntddk.h>
#include <internal/string.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID KeDrainApcQueue(VOID)
{
   PLIST_ENTRY current_entry;
   PKAPC current;
   PKTHREAD CurrentThread=KeGetCurrentThread();
   
   while ((current_entry=RemoveHeadList(CurrentThread->ApcList))!=NULL)
     {
	current = CONTAINING_RECORD(current_entry,KAPC,ApcListEntry);
	current->NormalRoutine(current->NormalContext,
			       current->SystemArgument1,
			       current->SystemArgument2);
	current_entry = current_entry->Flink;
     }
}

VOID KeInitializeApc(PKAPC Apc,
		     PKTHREAD Thread,
		     UCHAR StateIndex,
		     PKKERNEL_ROUTINE KernelRoutine,
		     PKRUNDOWN_ROUTINE RundownRoutine,
		     PKNORMAL_ROUTINE NormalRoutine,
		     UCHAR Mode,
		     PVOID Context)
{
   memset(Apc,0,sizeof(KAPC));
   Apc->Thread = Thread;
   Apc->ApcListEntry.Flink=NULL;
   Apc->ApcListEntry.Blink=NULL;
   Apc->KernelRoutine=KernelRoutine;
   Apc->RundownRoutine=RundownRoutine;
   Apc->NormalRoutine=NormalRoutine;
   Apc->NormalContext=Context;
   Apc->Inserted=FALSE;
   Apc->ApcStateIndex=StateIndex;
   Apc->ApcMode=Mode;
}

void KeInsertQueueApc(PKAPC Apc, PVOID SystemArgument1,
			 PVOID SystemArgument2, UCHAR Mode)
{
   Apc->SystemArgument1=SystemArgument1;
   Apc->SystemArgument2=SystemArgument2;
   Apc->ApcMode=Mode;
   if (Apc->Inserted)
     {
	return;
     }
   Apc->Inserted=TRUE;
   InsertTailList(Apc->Thread->ApcList,&Apc->ApcListEntry);
   return;
}

