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
#include <internal/kernel.h>
#include <internal/objmgr.h>
#include <internal/string.h>
#include <internal/hal/hal.h>
#include <internal/psmgr.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

OBJECT_TYPE ThreadObjectType = {{NULL,0,0},
                                0,
                                0,
                                ULONG_MAX,
                                ULONG_MAX,
                                sizeof(KTHREAD),
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

static KSPIN_LOCK ThreadListLock;

/*
 * PURPOSE: List of all threads currently active
 */
static LIST_ENTRY ThreadListHead;

/*
 * PURPOSE: List of threads associated with each priority level
 */
static LIST_ENTRY PriorityListHead[NR_THREAD_PRIORITY_LEVELS];
static BOOLEAN DoneInitYet = FALSE;

static PKTHREAD CurrentThread;

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
   return(CurrentThread);
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
   if (CurrentThread->State==THREAD_STATE_RUNNING)     
     {
	CurrentThread->State=THREAD_STATE_RUNNABLE;
     }
   
   /*
    * Simple round robin algorithm, iterate through and dispatch the first
    * runnable thread
    */
   current = CONTAINING_RECORD(ThreadListHead.Flink,KTHREAD,Entry);
   current_entry = ThreadListHead.Flink;

   while (current_entry!=NULL)
     {
	DPRINT("Scanning %x\n",current);
	DPRINT("State %x Runnable %x\n",current->State,THREAD_STATE_RUNNABLE);
        if (current->State == THREAD_STATE_RUNNABLE &&
            current !=CurrentThread)
	  {	     
	     DPRINT("Scheduling this one %x\n",current);
	     CurrentThread = current;
	     CurrentThread->State = THREAD_STATE_RUNNING;
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
   if (CurrentThread->State == THREAD_STATE_RUNNABLE)
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
   PKTHREAD first_thread;
   
   InitializeListHead(&ThreadListHead);
   KeInitializeSpinLock(&ThreadListLock);
   
   ObRegisterType(OBJTYP_THREAD,&ThreadObjectType);
   
   first_thread = ExAllocatePool(NonPagedPool,sizeof(KTHREAD));
   first_thread->State = THREAD_STATE_RUNNING;
   HalInitFirstTask(first_thread);
   ExInterlockedInsertHeadList(&ThreadListHead,&first_thread->Entry,
			       &ThreadListLock);
   CurrentThread = first_thread;
   
   DoneInitYet = TRUE;
}

NTSTATUS PsWakeThread(PKTHREAD Thread)
{
   Thread->State = THREAD_STATE_RUNNABLE;
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
   CurrentThread->State = THREAD_STATE_SUSPENDED;
   PsDispatchThread();
}

NTSTATUS PsCreateThread(HANDLE Parent,
			LPSECURITY_ATTRIBUTES lpThreadAttributes,
			PVOID Stack,
			LPTHREAD_START_ROUTINE lpStartAddress,
			LPVOID Parameter,
			DWORD dwCreationFlags,
			LPDWORD lpThreadId,
			PHANDLE ThreadHandle)
/*
 * FUNCTION: Creates a thread 
 * ARGUMENTS:
 *          Parent = Parent process (or NULL for current)
 *          lpThreadAttributes = Security descriptor for the new thread
 *          Stack = Caller allocated stack
 *          lpStartAddress = Entry point for the thread
 *          Parameter = Parameter to be passed to the entrypoint
 *          dwCreationFlags = Flags for creation
 *          lpThreadId = Pointer which receives thread identifier
 *          ThreadHandle = Variable which receives handle
 * RETURNS: Status
 */
{
   
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
   CurrentThread->State = THREAD_STATE_TERMINATED;
   ExInterlockedRemoveEntryList(&ThreadListHead,&CurrentThread->Entry,
				&ThreadListLock);
   PsDispatchThread();
   for(;;);
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
   PKTHREAD thread;
   ULONG ThreadId;
   ULONG ProcessId;
   
   thread = ObGenericCreateObject(ThreadHandle,0,NULL,OBJTYP_THREAD);
				  
   
   thread->State=THREAD_STATE_RUNNABLE;
   thread->Priority=0;
//   thread->Process=ObjGetObjectByHandle(ProcessHandle);
   thread->ProcessHandle=ProcessHandle;
   HalInitTask(thread,StartRoutine,StartContext);

//   ThreadId = InterlockedIncrement(&NextThreadUniqueId);   
   if (ClientId != NULL)
     {
	ClientId->UniqueThread = ThreadId;
     }
   
   ExInterlockedInsertHeadList(&ThreadListHead,&thread->Entry,
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
