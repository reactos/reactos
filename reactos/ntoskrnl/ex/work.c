/* $Id: work.c,v 1.16 2003/07/10 06:27:13 royce Exp $
 *
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS kernel
 * FILE:               mkernel/kernel/work.c
 * PURPOSE:            Manage system work queues
 * PROGRAMMER:         David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *             29/06/98: Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* DEFINES *******************************************************************/

#define NUMBER_OF_WORKER_THREADS   (5)

/* TYPES *********************************************************************/

typedef struct _WORK_QUEUE
{
   /*
    * PURPOSE: Head of the list of waiting work items
    */
   LIST_ENTRY Head;
   
   /*
    * PURPOSE: Sychronize access to the work queue
    */
   KSPIN_LOCK Lock;
   
   /*
    * PURPOSE: Worker threads with nothing to do wait on this event
    */
   KSEMAPHORE Sem;
   
   /*
    * PURPOSE: Thread associated with work queue
    */
   HANDLE Thread[NUMBER_OF_WORKER_THREADS];
} WORK_QUEUE, *PWORK_QUEUE;

/* GLOBALS *******************************************************************/

/*
 * PURPOSE: Queue of items waiting to be processed at normal priority
 */
WORK_QUEUE EiNormalWorkQueue;

WORK_QUEUE EiCriticalWorkQueue;

WORK_QUEUE EiHyperCriticalWorkQueue;

/* FUNCTIONS ****************************************************************/

static NTSTATUS STDCALL
ExWorkerThreadEntryPoint(PVOID context)
/*
 * FUNCTION: Entry point for a worker thread
 * ARGUMENTS:
 *         context = Parameters
 * RETURNS: Status
 * NOTE: To kill a worker thread you must queue an item whose callback
 * calls PsTerminateSystemThread
 */
{
   PWORK_QUEUE queue = (PWORK_QUEUE)context;
   PWORK_QUEUE_ITEM item;
   PLIST_ENTRY current;
   
   for(;;)
     {
	current = ExInterlockedRemoveHeadList(&queue->Head,
					      &queue->Lock);
	if (current!=NULL)
	  {
	     item = CONTAINING_RECORD(current,WORK_QUEUE_ITEM,Entry);
	     item->Routine(item->Context);
	  }
	else
	  {
	     KeWaitForSingleObject((PVOID)&queue->Sem,
				   Executive,
				   KernelMode,
				   FALSE,
				   NULL);
	     DPRINT("Woke from wait\n");
	  }
     }
}

static VOID ExInitializeWorkQueue(PWORK_QUEUE WorkQueue,
				  KPRIORITY Priority)
{
   ULONG i;
   PETHREAD Thread;
   
   InitializeListHead(&WorkQueue->Head);
   KeInitializeSpinLock(&WorkQueue->Lock);
   KeInitializeSemaphore(&WorkQueue->Sem,
			 0,
			 256);
   for (i=0; i<NUMBER_OF_WORKER_THREADS; i++)
     {
	PsCreateSystemThread(&WorkQueue->Thread[i],
			     THREAD_ALL_ACCESS,
			     NULL,
			     NULL,
			     NULL,
			     ExWorkerThreadEntryPoint,
			     WorkQueue);
	ObReferenceObjectByHandle(WorkQueue->Thread[i],
				  THREAD_ALL_ACCESS,
				  PsThreadType,
				  KernelMode,
				  (PVOID*)&Thread,
				  NULL);
	KeSetPriorityThread(&Thread->Tcb,
			    Priority);
	ObDereferenceObject(Thread);
     }
}

VOID ExInitializeWorkerThreads(VOID)
{
   ExInitializeWorkQueue(&EiNormalWorkQueue,
			 LOW_PRIORITY);
   ExInitializeWorkQueue(&EiCriticalWorkQueue,
			 LOW_REALTIME_PRIORITY);
   ExInitializeWorkQueue(&EiHyperCriticalWorkQueue,
			 HIGH_PRIORITY);
}

/*
 * @implemented
 */
VOID STDCALL
ExQueueWorkItem (PWORK_QUEUE_ITEM	WorkItem,
		 WORK_QUEUE_TYPE		QueueType)
/*
 * FUNCTION: Inserts a work item in a queue for one of the system worker
 * threads to process
 * ARGUMENTS:
 *        WorkItem = Item to insert
 *        QueueType = Queue to insert it in
 */
{
   assert(WorkItem!=NULL);
   ASSERT_IRQL(DISPATCH_LEVEL);
   
   /*
    * Insert the item in the appropiate queue and wake up any thread
    * waiting for something to do
    */
   switch(QueueType)
     {
      case DelayedWorkQueue:
	ExInterlockedInsertTailList(&EiNormalWorkQueue.Head,
				    &WorkItem->Entry,
				    &EiNormalWorkQueue.Lock);
	KeReleaseSemaphore(&EiNormalWorkQueue.Sem,
			   IO_NO_INCREMENT,
			   1,
			   FALSE);
	break;
	
      case CriticalWorkQueue:
	ExInterlockedInsertTailList(&EiCriticalWorkQueue.Head,
				    &WorkItem->Entry,
				    &EiCriticalWorkQueue.Lock);
	KeReleaseSemaphore(&EiCriticalWorkQueue.Sem,
			   IO_NO_INCREMENT,
			   1,
			   FALSE);
	break;

      case HyperCriticalWorkQueue:
	ExInterlockedInsertTailList(&EiHyperCriticalWorkQueue.Head,
				    &WorkItem->Entry,
				    &EiHyperCriticalWorkQueue.Lock);
	KeReleaseSemaphore(&EiHyperCriticalWorkQueue.Sem,
			   IO_NO_INCREMENT,
			   1,
			   FALSE);
	break;

     }
}

/* EOF */
