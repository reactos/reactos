/*
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/thread.c
 * PURPOSE:                Thread managment
 * PROGRAMMER:             David Welch (welch@mcmail.com)
 * REVISION HISTORY: 
 *               23/06/98: Created
 */

/*
 * NOTE:
 * 
 * All of the routines that manipulate the thread queue synchronize on
 * a single spinlock
 * 
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ob.h>
#include <internal/string.h>
#include <internal/hal.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

OBJECT_TYPE ThreadObjectType = {{NULL,0,0},
                                0,
                                0,
                                ULONG_MAX,
                                ULONG_MAX,
                                sizeof(ETHREAD),
                                0,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               };

#define NR_THREAD_PRIORITY_LEVELS (32)

static KSPIN_LOCK ThreadListLock = {0,};

/*
 * PURPOSE: List of all threads currently active
 */
static LIST_ENTRY ThreadListHead = {NULL,NULL};

/*
 * PURPOSE: List of threads associated with each priority level
 */
static LIST_ENTRY PriorityListHead[NR_THREAD_PRIORITY_LEVELS]={{NULL,NULL},};
static BOOLEAN DoneInitYet = FALSE;

static PETHREAD CurrentThread = NULL;

static ULONG NextThreadUniqueId = 0;

/* FUNCTIONS ***************************************************************/

NTSTATUS ZwSetInformationThread(HANDLE ThreadHandle,
				THREADINFOCLASS ThreadInformationClass,
				PVOID ThreadInformation,
				ULONG ThreadInformationLength)
{
   UNIMPLEMENTED;
}

PKTHREAD KeGetCurrentThread(VOID)
{
   return((PKTHREAD)CurrentThread);
}

PETHREAD PsGetCurrentThread(VOID)
{
   return((PETHREAD)KeGetCurrentThread());
}

#if CAN_WE_DARE_TO_TRY_THIS
void PsDispatchThread(void)
{
   int i;
   
   for (i=0; i<NR_THREAD_PRIORITY_LEVELS; i++)
     {
	if (PsDispatchSpecificPriorityThread(i))
	  {
	     return;
	  }
     }
}
#endif

void PsDispatchThread(void)
/*
 * FUNCTION: Chooses a thread, possibly the current one if it is runnable
 * and dispatches it
 */
{
   KIRQL irql;
   PLIST_ENTRY current_entry;
   PKTHREAD current;
   
   if (!DoneInitYet)
     {
	return;
     }
   
   DPRINT("PsDispatchThread() Current %x\n",CurrentThread);
   
   /*
    * Bump overselves up to a higher IRQ level during this
    */
   KeAcquireSpinLock(&ThreadListLock,&irql);
   
   /*
    * If this was an involuntary reschedule then the current thread will still
    * be eligible to run later
    */
   if (CurrentThread->Tcb.ThreadState==THREAD_STATE_RUNNING)     
     {
	CurrentThread->Tcb.ThreadState=THREAD_STATE_RUNNABLE;
     }
   
   /*
    * Simple round robin algorithm, iterate through and dispatch the first
    * runnable thread
    */
   current = CONTAINING_RECORD(ThreadListHead.Flink,KTHREAD,Entry);
   current_entry = ThreadListHead.Flink;

   while (current_entry!=(&ThreadListHead))
     {
        DPRINT("Scanning %x ",current);
	DPRINT("State %x Runnable %x\n",current->ThreadState,
	       THREAD_STATE_RUNNABLE);
        if (current->ThreadState == THREAD_STATE_RUNNABLE &&
            current != (PKTHREAD)CurrentThread)
	  {	     
	     DPRINT("Scheduling this one %x\n",current);	   	     
	     CurrentThread = current;
	     CurrentThread->Tcb.ThreadState = THREAD_STATE_RUNNING;
	     KeReleaseSpinLock(&ThreadListLock,irql);
	     HalTaskSwitch(current);
	     return;
	  }
        current_entry = current->Entry.Flink;
        current = CONTAINING_RECORD(current_entry,KTHREAD,Entry);
     }
   
   /*
    * If there are no other threads then continue with the current one if
    * possible 
    */
   if (CurrentThread->Tcb.ThreadState == THREAD_STATE_RUNNABLE)
     {
	return;
     }
   
   /*
    * Disaster
    */
   printk("Out of threads at %s:%d\n",__FILE__,__LINE__);
   for(;;);
}

void PsInitThreadManagment(void)
/*
 * FUNCTION: Initialize thread managment
 */
{
   PETHREAD first_thread;
   
   InitializeListHead(&ThreadListHead);
   KeInitializeSpinLock(&ThreadListLock);
   
   ObRegisterType(OBJTYP_THREAD,&ThreadObjectType);
   
   first_thread = ExAllocatePool(NonPagedPool,sizeof(ETHREAD));
   first_thread->Tcb.ThreadState = THREAD_STATE_RUNNING;
   HalInitFirstTask((PKTHREAD)first_thread);
   ExInterlockedInsertHeadList(&ThreadListHead,&first_thread->Tcb.Entry,
			       &ThreadListLock);
   CurrentThread = first_thread;
   
   DoneInitYet = TRUE;
}

NTSTATUS PsWakeThread(PETHREAD Thread)
{
   Thread->Tcb.ThreadState = THREAD_STATE_RUNNABLE;
   return(STATUS_SUCCESS);
}

NTSTATUS PsSuspendThread(VOID)
/*
 * FUNCTION: Suspend the current thread
 */
{
   KIRQL oldlvl;
   
   DPRINT("suspending %x\n",CurrentThread);
   
   /*
    * NOTE: When we return from PsDispatchThread the spinlock will be
    * released
    */
   CurrentThread->Tcb.ThreadState = THREAD_STATE_SUSPENDED;
   PsDispatchThread();
   return(STATUS_SUCCESS);
}



NTSTATUS PsTerminateSystemThread(NTSTATUS ExitStatus)
/*
 * FUNCTION: Terminates the current thread
 * ARGUMENTS:
 *         ExitStatus = Status to pass to the creater
 * RETURNS: Doesn't
 */
{
   KIRQL oldlvl;
   
   DPRINT("terminating %x\n",CurrentThread);
   KeRaiseIrql(DISPATCH_LEVEL,&oldlvl);
   CurrentThread->Tcb.ThreadState = THREAD_STATE_TERMINATED;
   RemoveEntryList(&CurrentThread->Tcb.Entry);
   PsDispatchThread();
   for(;;);
}

NTSTATUS NtCreateThread(PHANDLE ThreadHandle,
			ACCESS_MASK DesiredAccess,
			POBJECT_ATTRIBUTES ObjectAttributes,
			HANDLE ProcessHandle,
			PCLIENT_ID Client,
			PCONTEXT ThreadContext,
			PINITIAL_TEB InitialTeb,
			BOOLEAN CreateSuspended)
{
}

NTSTATUS PsCreateSystemThread(PHANDLE ThreadHandle,
			      ACCESS_MASK DesiredAccess,
			      POBJECT_ATTRIBUTES ObjectAttributes,
			      HANDLE ProcessHandle,
			      PCLIENT_ID ClientId,
			      PKSTART_ROUTINE StartRoutine,
			      PVOID StartContext)
/*
 * FUNCTION: Creates a thread which executes in kernel mode
 * ARGUMENTS:
 *       ThreadHandle (OUT) = Caller supplied storage for the returned thread 
 *                            handle
 *       DesiredAccess = Requested access to the thread
 *       ObjectAttributes = Object attributes (optional)
 *       ProcessHandle = Handle of process thread will run in
 *                       NULL to use system process
 *       ClientId (OUT) = Caller supplied storage for the returned client id
 *                        of the thread (optional)
 *       StartRoutine = Entry point for the thread
 *       StartContext = Argument supplied to the thread when it begins
 *                     execution
 * RETURNS: Success or failure status
 */
{
   PETHREAD thread;
   ULONG ThreadId;
   ULONG ProcessId;
   
   thread = ObGenericCreateObject(ThreadHandle,0,NULL,OBJTYP_THREAD);
   DPRINT("Allocating thread %x\n",thread);                                
   
   thread->Tcb.ThreadState=THREAD_STATE_RUNNABLE;
   thread->Tcb.BasePriority=0;
   thread->Tcb.CurrentPriority=0;
   thread->Tcb.ApcList=ExAllocatePool(NonPagedPool,sizeof(LIST_ENTRY));
   InitializeListHead(thread->Tcb.ApcList);
   HalInitTask(&(thread->Tcb),StartRoutine,StartContext);
   InitializeListHead(&(thread->IrpList));
   thread->Cid.UniqueThread=NextThreadUniqueId++;
//   thread->Cid.ThreadId=InterlockedIncrement(&NextThreadUniqueId);
   if (ClientId!=NULL)
     {
	*ClientId=thread->Cid;
     }
   
   if (ProcessHandle!=NULL)
     {   
	thread->ThreadsProcess=ObGetObjectByHandle(ProcessHandle);
     }
   else
     {
	thread->ThreadsProcess=&SystemProcess;
     }
   thread->StartAddress=StartRoutine;
   
   
   ExInterlockedInsertHeadList(&ThreadListHead,&thread->Tcb.Entry,
			       &ThreadListLock);
   return(STATUS_SUCCESS);
}

LONG KeSetBasePriorityThread(PKTHREAD Thread, LONG Increment)
{
   UNIMPLEMENTED;
}

KPRIORITY KeSetPriorityThread(PKTHREAD Thread, KPRIORITY Priority)
{
   UNIMPLEMENTED;
}
