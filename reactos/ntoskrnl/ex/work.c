/* $Id$
 *
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS kernel
 * FILE:               ntoskrnl/ex/work.c
 * PURPOSE:            Manage system work queues
 * 
 * PROGRAMMERS:        David Welch (welch@mcmail.com)
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
      
      /* can't happend since we do a KernelMode wait (and we're a system thread) */
      ASSERT((NTSTATUS)current != STATUS_USER_APC);
      
      /* this should never happend either, since we wait with NULL timeout,
       * but there's a slight possibility that STATUS_TIMEOUT is returned
       * at queue rundown in NT (unlikely) -Gunnar
       */
      ASSERT((NTSTATUS)current != STATUS_TIMEOUT);
      
      /* based on INVALID_WORK_QUEUE_ITEM bugcheck desc. */
      if (current->Flink == NULL || current->Blink == NULL)
      {
         KeBugCheck(INVALID_WORK_QUEUE_ITEM);
      }
      
      /* "reinitialize" item (same as done in ExInitializeWorkItem) */
      current->Flink = NULL;
      
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
    HANDLE hThread;
    NTSTATUS Status;

    PAGED_CODE();

    /* Loop through how many threads we need to create */
    for (i = 0; i < NUMBER_OF_WORKER_THREADS; i++) {

        /* Create the System Thread */
        Status = PsCreateSystemThread(&hThread,
                                      THREAD_ALL_ACCESS,
                                      NULL,
                                      NULL,
                                      NULL,
                                      ExWorkerThreadEntryPoint,
                                      (PVOID)WorkQueue);
        if(NT_SUCCESS(Status))
        {
          /* Get the Thread */
          Status = ObReferenceObjectByHandle(hThread,
                                             THREAD_SET_INFORMATION,
                                             PsThreadType,
                                             KernelMode,
                                             (PVOID*)&Thread,
                                             NULL);

          if(NT_SUCCESS(Status))
          {
            /* Set the Priority */
            KeSetPriorityThread(&Thread->Tcb, Priority);

            /* Dereference and close handle */
            ObDereferenceObject(Thread);
            ZwClose(hThread);
          }
          else
          {
            DPRINT1("Unable to reference worker thread handle 0x%x, Status: 0x%x!\n", hThread, Status);
            KEBUGCHECK(0);
          }
        }
        else
        {
          DPRINT1("Unable to create worker thread, Status: 0x%x!\n", Status);
          KEBUGCHECK(0);
        }
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
    ASSERT(WorkItem->List.Flink == NULL);
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
     
    default:
        break;

    }
}

/* EOF */
