/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS project
 * FILE:                 ntoskrnl/ps/wait.c
 * PURPOSE:              Manages non-busy waiting
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *           21/07/98: Created
 */

/* NOTES ********************************************************************
 * 
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/kernel.h>
#include <internal/wait.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

KSPIN_LOCK DispatcherDatabaseLock;
BOOLEAN WaitSet;

/* FUNCTIONS *****************************************************************/

VOID KeDispatcherObjectWake(DISPATCHER_HEADER* hdr)
{
   PKWAIT_BLOCK current;
   PLIST_ENTRY current_entry;
   
   DPRINT("Entering KeDispatcherObjectWake(hdr %x)\n",hdr);
   DPRINT("hdr->WaitListHead %x hdr->WaitListHead.Flink %x\n",
	  &hdr->WaitListHead,hdr->WaitListHead.Flink);
   while ((current_entry=RemoveHeadList(&hdr->WaitListHead))!=NULL)
     {
	current = CONTAINING_RECORD(current_entry,KWAIT_BLOCK,
					    WaitListEntry);
	DPRINT("Waking %x\n",current->Thread);
	PsWakeThread(current->Thread);
     };
}
 
   
NTSTATUS KeWaitForSingleObject(PVOID Object,
			       KWAIT_REASON WaitReason,
			       KPROCESSOR_MODE WaitMode,
			       BOOLEAN Alertable,
			       PLARGE_INTEGER Timeout)
{
   DISPATCHER_HEADER* hdr = (DISPATCHER_HEADER *)Object;
   KWAIT_BLOCK blk;
   
   DPRINT("Entering KeWaitForSingleObject(Object %x)\n",Object);
   
   blk.Object=Object;
   blk.Thread=KeGetCurrentThread();
   InsertTailList(&hdr->WaitListHead,&blk.WaitListEntry);
   PsSuspendThread();
}

NTSTATUS KeWaitForMultipleObjects(ULONG Count,
				  PVOID Object[],
				  WAIT_TYPE WaitType,
				  KWAIT_REASON WaitReason,
				  KPROCESSOR_MODE WaitMode,
				  BOOLEAN Alertable,
				  PLARGE_INTEGER Timeout,
				  PKWAIT_BLOCK WaitBlockArray)
{
   UNIMPLEMENTED;
}

VOID KeInitializeDispatcher(VOID)
{
   KeInitializeSpinLock(&DispatcherDatabaseLock);
}
