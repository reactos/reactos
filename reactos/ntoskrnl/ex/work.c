/*
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS kernel
 * FILE:               mkernel/kernel/work.c
 * PURPOSE:            Manage system work queues
 * PROGRAMMER:         David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *             29/06/98: Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* TYPES *********************************************************************/

typedef struct
{
   /*
    * PURPOSE: Head of the list of waiting work items
    */
   LIST_ENTRY Head;
   
   /*
    * PURPOSE: Sychronize access to the access
    */
   KSPIN_LOCK Lock;
   
   /*
    * PURPOSE: Worker threads with nothing to do wait on this event
    */
   KEVENT Busy;
   
   /*
    * PURPOSE: Thread associated with work queue
    */
   HANDLE Thread;
} WORK_QUEUE, *PWORK_QUEUE;

/* GLOBALS *******************************************************************/

/*
 * PURPOSE: Queue of items waiting to be processed at normal priority
 */
WORK_QUEUE normal_work_queue = {0,};

#define WAIT_INTERVAL (0)

/* FUNCTIONS ****************************************************************/

static NTSTATUS ExWorkerThreadEntryPoint(PVOID context)
/*
 * FUNCTION: Entry point for a worker thread
 * ARGUMENTS:
 *         context = Parameters
 * RETURNS: Status
 * NOTE: To kill a worker thread you must queue an item whose callback
 * calls PsTerminateSystemThread
 */
{
   PWORK_QUEUE param = (PWORK_QUEUE)context;
   PWORK_QUEUE_ITEM item;
   PLIST_ENTRY current;
   
   while (1)
     {
	current = ExInterlockedRemoveHeadList(&param->Head,&param->Lock);
	if (current!=NULL)
	  {
	     item = CONTAINING_RECORD(current,WORK_QUEUE_ITEM,Entry);
	     item->Routine(item->Context);
	  }
	else
	  {
	     KeClearEvent(&param->Busy);
	     KeWaitForSingleObject((PVOID)&param->Busy,Executive,KernelMode,
				   FALSE,WAIT_INTERVAL);
	  }
     };   
}

static VOID ExKillWorkerThreadCallback(PVOID Context)
{
   PsTerminateSystemThread(STATUS_SUCCESS);
}

void ExKillWorkerThreads(void)
/*
 * FUNCTION: Kill all running worker threads in preparation for a shutdown
 */
{
   WORK_QUEUE_ITEM item1;
   
   ExInitializeWorkItem(&item1,ExKillWorkerThreadCallback,NULL);
   ExQueueWorkItem(&item1,DelayedWorkQueue);
}

void ExInitializeWorkerThreads(void)
{
   InitializeListHead(&normal_work_queue.Head);
   KeInitializeSpinLock(&normal_work_queue.Lock);
   KeInitializeEvent(&normal_work_queue.Busy,NotificationEvent,FALSE);   
   PsCreateSystemThread(&normal_work_queue.Thread,THREAD_ALL_ACCESS,
			NULL,NULL,NULL,ExWorkerThreadEntryPoint,
			&normal_work_queue);
}

VOID ExInitializeWorkItem(PWORK_QUEUE_ITEM Item,
			  PWORKER_THREAD_ROUTINE Routine,
	 		  PVOID Context)
/*
 * FUNCTION: Initializes a work item to be processed by one of the system
 * worker threads
 * ARGUMENTS:
 *         Item = Pointer to the item to be initialized
 *         Routine = Routine to be called by the worker thread
 *         Context = Parameter to be passed to the callback
 */
{
   ASSERT_IRQL(DISPATCH_LEVEL);
   
   Item->Routine=Routine;
   Item->Context=Context;
   Item->Entry.Flink=NULL;
   Item->Entry.Blink=NULL;
}

VOID ExQueueWorkItem(PWORK_QUEUE_ITEM WorkItem,
		     WORK_QUEUE_TYPE QueueType)
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
    * Insert the item in the appropiate queue and wake on any thread
    * waiting for something to do
    */
   switch(QueueType)
     {
      case DelayedWorkQueue:
	ExInterlockedInsertTailList(&normal_work_queue.Head,&(WorkItem->Entry),
				    &normal_work_queue.Lock);
	KeSetEvent(&normal_work_queue.Busy,IO_NO_INCREMENT,FALSE);
	break;
     };
}
