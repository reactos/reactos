/* $Id: work.c,v 1.23 2004/11/21 18:38:51 gdalsnes Exp $
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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* DEFINES *******************************************************************/

#define NUMBER_OF_WORKER_THREADS   (5)

/* TYPES *********************************************************************/

/* GLOBALS *******************************************************************/

/*
 * PURPOSE: Queue of items waiting to be processed at normal priority
 */
KQUEUE EiNormalWorkQueue;

KQUEUE EiCriticalWorkQueue;

KQUEUE EiHyperCriticalWorkQueue;

/* FUNCTIONS ****************************************************************/

//static NTSTATUS STDCALL
static VOID STDCALL
ExWorkerThreadEntryPoint(IN PVOID context)
/*
 * FUNCTION: Entry point for a worker thread
 * ARGUMENTS:
 *         context = Parameters
 * RETURNS: Status
 * NOTE: To kill a worker thread you must queue an item whose callback
 * calls PsTerminateSystemThread
 */
{

   PWORK_QUEUE_ITEM item;
   PLIST_ENTRY current;
   
   while (TRUE) 
   {
      current = KeRemoveQueue( (PKQUEUE)context, KernelMode, NULL );
      item = CONTAINING_RECORD( current, WORK_QUEUE_ITEM, List);
      item->WorkerRoutine(item->Parameter);
      
      if (KeGetCurrentIrql() != PASSIVE_LEVEL)
      {
         KeBugCheck(IRQL_NOT_LESS_OR_EQUAL);
      }
   }
   
}

static VOID ExInitializeWorkQueue(PKQUEUE WorkQueue,
				  KPRIORITY Priority)
{
   ULONG i;
   PETHREAD Thread;
   HANDLE   hThread;
   
   
   for (i=0; i<NUMBER_OF_WORKER_THREADS; i++)
     {
        
   PsCreateSystemThread(&hThread,
			     THREAD_ALL_ACCESS,
			     NULL,
			     NULL,
			     NULL,
			     ExWorkerThreadEntryPoint,
              WorkQueue);
   ObReferenceObjectByHandle(hThread,
				  THREAD_ALL_ACCESS,
				  PsThreadType,
				  KernelMode,
				  (PVOID*)&Thread,
				  NULL);
	KeSetPriorityThread(&Thread->Tcb,
			    Priority);
	ObDereferenceObject(Thread);
   ZwClose(hThread);
     }
}

VOID INIT_FUNCTION
ExInitializeWorkerThreads(VOID)
{
   KeInitializeQueue( &EiNormalWorkQueue, NUMBER_OF_WORKER_THREADS );
   KeInitializeQueue( &EiCriticalWorkQueue , NUMBER_OF_WORKER_THREADS );
   KeInitializeQueue( &EiHyperCriticalWorkQueue , NUMBER_OF_WORKER_THREADS );

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
    ASSERT(WorkItem!=NULL);
    ASSERT_IRQL(DISPATCH_LEVEL);
   
   /*
    * Insert the item in the appropiate queue and wake up any thread
    * waiting for something to do
    */
    switch(QueueType)
    {
    case DelayedWorkQueue:
      KeInsertQueue (
          &EiNormalWorkQueue,
          &WorkItem->List
            );
   	break;
	
    case CriticalWorkQueue:
            KeInsertQueue (
              &EiCriticalWorkQueue,
              &WorkItem->List
              );
   	    break;

    case HyperCriticalWorkQueue:
            KeInsertQueue (
             &EiHyperCriticalWorkQueue,
             &WorkItem->List
             );
        	break;

    }
}

/* EOF */
