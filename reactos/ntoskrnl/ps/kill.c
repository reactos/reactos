/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/kill.c
 * PURPOSE:         Thread Termination and Reaping
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/
      
#define TAG_TERMINATE_APC   TAG('T', 'A', 'P', 'C')

PETHREAD PspReaperList = NULL;
WORK_QUEUE_ITEM PspReaperWorkItem;
BOOLEAN PspReaping = FALSE;
extern LIST_ENTRY PsActiveProcessHead;
extern FAST_MUTEX PspActiveProcessMutex;

VOID
STDCALL
MmDeleteTeb(PEPROCESS Process,
            PTEB Teb);

/* FUNCTIONS *****************************************************************/

STDCALL
VOID
PspReapRoutine(PVOID Context)
{
    KIRQL OldIrql;
    PETHREAD Thread, NewThread;
 
    /* Acquire lock */
    DPRINT("Evil reaper running!!\n");
    OldIrql = KeAcquireDispatcherDatabaseLock();
    
    /* Get the first Thread Entry */
    Thread = PspReaperList;
    PspReaperList = NULL;
    DPRINT("PspReaperList: %x\n", Thread);
    
    /* Check to see if the list is empty */
    do {
        
        /* Unlock the Dispatcher */
        KeReleaseDispatcherDatabaseLock(OldIrql);
        
        /* Is there a thread on the list? */
        while (Thread) {
            
            /* Get the next Thread */
            DPRINT("Thread: %x\n", Thread);
            DPRINT("Thread: %x\n", Thread->ReaperLink);
            NewThread = Thread->ReaperLink;
            
            /* Remove reference to current thread */
            ObDereferenceObject(Thread);
        
            /* Move to next Thread */
            Thread = NewThread;
        }
        
        /* No more linked threads... Reacquire the Lock */
        OldIrql = KeAcquireDispatcherDatabaseLock();
        
        /* Now try to get a new thread from the list */
        Thread = PspReaperList;
        PspReaperList = NULL;
        DPRINT("PspReaperList: %x\n", Thread);
        
        /* Loop again if there is a new thread */
    } while (Thread);
    
    PspReaping = FALSE;
    DPRINT("Done reaping\n");
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

VOID
STDCALL
PspKillMostProcesses(VOID)
{
    PLIST_ENTRY current_entry;
    PEPROCESS current;
   
    /* Acquire the Active Process Lock */
    ExAcquireFastMutex(&PspActiveProcessMutex);   
    
    /* Loop all processes on the list */
    current_entry = PsActiveProcessHead.Flink;
    while (current_entry != &PsActiveProcessHead)
    {
        current = CONTAINING_RECORD(current_entry, EPROCESS, ProcessListEntry);
        current_entry = current_entry->Flink;
    
        if (current->UniqueProcessId != PsInitialSystemProcess->UniqueProcessId &&
            current->UniqueProcessId != PsGetCurrentProcessId())
        {
            /* Terminate all the Threads in this Process */
            PspTerminateProcessThreads(current, STATUS_SUCCESS);
        }
    }
   
    /* Release the lock */
    ExReleaseFastMutex(&PspActiveProcessMutex);
}

VOID
STDCALL
PspTerminateProcessThreads(PEPROCESS Process,
                           NTSTATUS ExitStatus)
{
    PLIST_ENTRY CurrentEntry;
    PETHREAD Thread, CurrentThread = PsGetCurrentThread();
   
    CurrentEntry = Process->ThreadListHead.Flink;
    while (CurrentEntry != &Process->ThreadListHead) {
       
        /* Get the Current Thread */
        Thread = CONTAINING_RECORD(CurrentEntry, ETHREAD, ThreadListEntry);
        
        /* Move to the Next Thread */
        CurrentEntry = CurrentEntry->Flink;
        
        /* Make sure it's not the one we're in */
        if (Thread != CurrentThread) {
        
            /* Make sure it didn't already terminate */
            if (!Thread->HasTerminated) {

                Thread->HasTerminated = TRUE;
                
                /* Terminate it by APC */
                PspTerminateThreadByPointer(Thread, ExitStatus);
            }
        }
    }
}

VOID 
STDCALL 
PspDeleteProcess(PVOID ObjectBody)
{
    PEPROCESS Process = (PEPROCESS)ObjectBody;

    DPRINT("PiDeleteProcess(ObjectBody %x)\n", ObjectBody);

    /* Delete the CID Handle */   
    if(Process->UniqueProcessId != NULL) {
    
        PsDeleteCidHandle(Process->UniqueProcessId, PsProcessType);
    }
    
    /* KDB hook */
    KDB_DELETEPROCESS_HOOK(Process);
    
    /* Dereference the Token and release Memory Information */
    ObDereferenceObject(Process->Token);
    MmReleaseMmInfo(Process);
 
    /* Delete the W32PROCESS structure if there's one associated */
    if(Process->Win32Process != NULL) ExFreePool(Process->Win32Process);
}

VOID 
STDCALL
PspDeleteThread(PVOID ObjectBody)
{
    PETHREAD Thread = (PETHREAD)ObjectBody;
    PEPROCESS Process = Thread->ThreadsProcess;

    DPRINT("PiDeleteThread(ObjectBody 0x%x, process 0x%x)\n",ObjectBody, Thread->ThreadsProcess);

    /* Deassociate the Process */
    Thread->ThreadsProcess = NULL;

    /* Delete the CID Handle */
    if(Thread->Cid.UniqueThread != NULL) {
        
        PsDeleteCidHandle(Thread->Cid.UniqueThread, PsThreadType);
    }
  
    /* Free the W32THREAD structure if present */
    if(Thread->Tcb.Win32Thread != NULL) ExFreePool (Thread->Tcb.Win32Thread);

    /* Release the Thread */
    KeReleaseThread(ETHREAD_TO_KTHREAD(Thread)); 
    
    /* Dereference the Process */
    ObDereferenceObject(Process);
}
              
/*
 * FUNCTION: Terminates the current thread
 * See "Windows Internals" - Chapter 13, Page 50-53
 */
VOID
STDCALL
PspExitThread(NTSTATUS ExitStatus)
{
    PETHREAD CurrentThread;
    BOOLEAN Last;
    PEPROCESS CurrentProcess;
    PTERMINATION_PORT TerminationPort;
    PTEB Teb;

    DPRINT("PspExitThread(ExitStatus %x), Current: 0x%x\n", ExitStatus, PsGetCurrentThread());

    /* Get the Current Thread and Process */
    CurrentThread = PsGetCurrentThread();
    CurrentProcess = CurrentThread->ThreadsProcess;
    
    /* Set the Exit Status and Exit Time */
    CurrentThread->ExitStatus = ExitStatus;
    KeQuerySystemTime(&CurrentThread->ExitTime);

    /* Can't terminate a thread if it attached another process */
    if (KeIsAttachedProcess()) {
        
        KEBUGCHECKEX(INVALID_PROCESS_ATTACH_ATTEMPT, (ULONG) CurrentProcess,
                     (ULONG) CurrentThread->Tcb.ApcState.Process,
                     (ULONG) CurrentThread->Tcb.ApcStateIndex,
                     (ULONG) CurrentThread);
    }
    
    /* Lower to Passive Level */
    KeLowerIrql(PASSIVE_LEVEL);

    /* Lock the Process before we modify its thread entries */
    PsLockProcess(CurrentProcess, FALSE);
    
    /* wake up the thread so we don't deadlock on PsLockProcess */
    KeForceResumeThread(&CurrentThread->Tcb);
    
    /* Run Thread Notify Routines before we desintegrate the thread */
    PspRunCreateThreadNotifyRoutines(CurrentThread, FALSE);

    /* Remove the thread from the thread list of its process */
    RemoveEntryList(&CurrentThread->ThreadListEntry);
    Last = IsListEmpty(&CurrentProcess->ThreadListHead);
    
    /* Set the last Thread Exit Status */
    CurrentProcess->LastThreadExitStatus = ExitStatus;
    
    if (Last) {

       /* Save the Exit Time if not already done by NtTerminateProcess. This
          happens when the last thread just terminates without explicitly
          terminating the process. */
       CurrentProcess->ExitTime = CurrentThread->ExitTime;
    }
    
    /* Check if the process has a debug port */
    if (CurrentProcess->DebugPort) {
    
        /* Notify the Debug API. TODO */
        //Last ? DbgkExitProcess(ExitStatus) : DbgkExitThread(ExitStatus);
    }
    
    /* Process the Termination Ports */  
    TerminationPort = CurrentThread->TerminationPort;
    DPRINT("TerminationPort: %p\n", TerminationPort);
    while (TerminationPort) {
        
        /* Send the LPC Message */
        LpcSendTerminationPort(TerminationPort->Port, CurrentThread->CreateTime);
        
        /* Free the Port */
        ExFreePool(TerminationPort);
        
        /* Get the next one */
        TerminationPort = TerminationPort->Next;
        DPRINT("TerminationPort: %p\n", TerminationPort);
    }
      
    /* Rundown Win32 Structures */
    PsTerminateWin32Thread(CurrentThread);
    if (Last) PsTerminateWin32Process(CurrentProcess);
   
    /* Rundown Registry Notifications. TODO (refers to NtChangeNotify, not Cm callbacks) */
    //CmNotifyRunDown(CurrentThread);
    
    /* Free the TEB */
    if((Teb = CurrentThread->Tcb.Teb)) {

        DPRINT("Decommit teb at %p\n", Teb);
        MmDeleteTeb(CurrentProcess, Teb);
        CurrentThread->Tcb.Teb = NULL;
    }
   
    /* The last Thread shuts down the Process */
    if (Last) PspExitProcess(CurrentProcess);
    
    /* Unlock the Process */
    PsUnlockProcess(CurrentProcess);
    
    /* Cancel I/O for the thread. */
    IoCancelThreadIo(CurrentThread);
    
    /* Rundown Timers */
    ExTimerRundown();
    KeCancelTimer(&CurrentThread->Tcb.Timer);
    
    /* If the Processor Control Block's NpxThread points to the current thread
     * unset it.
     */
    InterlockedCompareExchangePointer(&KeGetCurrentPrcb()->NpxThread,
                                      NULL,
                                      (PKPROCESS)CurrentThread);
    
    /* Rundown Mutexes */
    KeRundownThread();

    /* Terminate the Thread from the Scheduler */
    KeTerminateThread(0);
    DPRINT1("Unexpected return, CurrentThread %x PsGetCurrentThread() %x\n", CurrentThread, PsGetCurrentThread());
    KEBUGCHECK(0);
}

VOID 
STDCALL
PsExitSpecialApc(PKAPC Apc,
                 PKNORMAL_ROUTINE* NormalRoutine,
                 PVOID* NormalContext,
                 PVOID* SystemArgument1,
                 PVOID* SystemArguemnt2)
{
    NTSTATUS ExitStatus = (NTSTATUS)Apc->NormalContext;
    
    DPRINT("PsExitSpecialApc called: 0x%x (proc: 0x%x)\n", PsGetCurrentThread(), PsGetCurrentProcess());
    
    /* Free the APC */
    ExFreePool(Apc);
    
    /* Terminate the Thread */
    PspExitThread(ExitStatus);
    
    /* we should never reach this point! */
    KEBUGCHECK(0);
}

VOID 
STDCALL
PspExitNormalApc(PVOID NormalContext,
                 PVOID SystemArgument1,
                 PVOID SystemArgument2)
{
    /* Not fully supported yet... must work out some issues that 
     * I don't understand yet -- Alex 
     */
    DPRINT1("APC2\n");
    PspExitThread((NTSTATUS)NormalContext);
    
    /* we should never reach this point! */
    KEBUGCHECK(0);
}

/*
 * See "Windows Internals" - Chapter 13, Page 49
 */
VOID
STDCALL
PspTerminateThreadByPointer(PETHREAD Thread,
                            NTSTATUS ExitStatus)
{
    PKAPC Apc;  
    
    DPRINT("PspTerminatedThreadByPointer(Thread %x, ExitStatus %x)\n",
            Thread, ExitStatus);
    
    /* Check if we are already in the right context */
    if (PsGetCurrentThread() == Thread) {

        /* Directly terminate the thread */
        PspExitThread(ExitStatus);

        /* we should never reach this point! */
        KEBUGCHECK(0);
    }
    
    /* Allocate the APC */
    Apc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), TAG_TERMINATE_APC);
    
    /* Initialize a Kernel Mode APC to Kill the Thread */
    KeInitializeApc(Apc,
                    &Thread->Tcb,
                    OriginalApcEnvironment,
                    PsExitSpecialApc,
                    NULL,
                    PspExitNormalApc,
                    KernelMode,
                    (PVOID)ExitStatus);
    
    /* Insert it into the APC Queue */
    KeInsertQueueApc(Apc,
                     Apc,
                     NULL,
                     2);
    
    /* Forcefully resume the thread */
    KeForceResumeThread(&Thread->Tcb);
}
   
NTSTATUS 
STDCALL
PspExitProcess(PEPROCESS Process)
{
    DPRINT("PspExitProcess 0x%x\n", Process);
           
    PspRunCreateProcessNotifyRoutines(Process, FALSE);
           
    /* Remove it from the Active List */
    ExAcquireFastMutex(&PspActiveProcessMutex);
    RemoveEntryList(&Process->ProcessListEntry);
    ExReleaseFastMutex(&PspActiveProcessMutex);
    
    /* close all handles associated with our process, this needs to be done
       when the last thread still runs */
    ObKillProcess(Process);

    KeSetProcess(&Process->Pcb, IO_NO_INCREMENT);
    
    return(STATUS_SUCCESS);
}

NTSTATUS 
STDCALL
NtTerminateProcess(IN HANDLE ProcessHandle  OPTIONAL,
                   IN NTSTATUS ExitStatus)
{
    NTSTATUS Status;
    PEPROCESS Process;
    PETHREAD CurrentThread;
    BOOLEAN KillByHandle;
   
    PAGED_CODE();
   
    DPRINT("NtTerminateProcess(ProcessHandle %x, ExitStatus %x)\n",
            ProcessHandle, ExitStatus);
    
    KillByHandle = (ProcessHandle != NULL);

    /* Get the Process Object */
    Status = ObReferenceObjectByHandle((KillByHandle ? ProcessHandle : NtCurrentProcess()),
                                       PROCESS_TERMINATE,
                                       PsProcessType,
                                       KeGetPreviousMode(),
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) {

        DPRINT1("Invalid handle to Process\n");
        return(Status);
    }
    
    CurrentThread = PsGetCurrentThread();
    
    PsLockProcess(Process, FALSE);
    
    if(Process->ExitTime.QuadPart != 0)
    {
      PsUnlockProcess(Process);
      return STATUS_PROCESS_IS_TERMINATING;
    }
    
    /* Terminate all the Process's Threads */
    PspTerminateProcessThreads(Process, ExitStatus);
    
    /* only kill the calling thread if it either passed a process handle or
       NtCurrentProcess() */
    if (KillByHandle) {

        /* set the exit time as we're about to release the process lock before
           we kill ourselves to prevent threads outside of our process trying
           to kill us */
        KeQuerySystemTime(&Process->ExitTime);

        /* Only master thread remains... kill it off */
        if (CurrentThread->ThreadsProcess == Process) {

            /* mark our thread as terminating so attempts to terminate it, when
               unlocking the process, fail */
            CurrentThread->HasTerminated = TRUE;

            PsUnlockProcess(Process);

            /* we can safely dereference the process because the current thread
               holds a reference to it until it gets reaped */
            ObDereferenceObject(Process);

            /* now the other threads get a chance to terminate, we don't wait but
               just kill ourselves right now. The process will be run down when the
               last thread terminates */

            PspExitThread(ExitStatus);

            /* we should never reach this point! */
            KEBUGCHECK(0);
        }
    }

    /* unlock and dereference the process so the threads can kill themselves */
    PsUnlockProcess(Process);
    ObDereferenceObject(Process);
    
    return(STATUS_SUCCESS);
}

NTSTATUS 
STDCALL
NtTerminateThread(IN HANDLE ThreadHandle,
                  IN NTSTATUS ExitStatus)
{
    PETHREAD Thread;
    NTSTATUS Status;
   
    PAGED_CODE();
    
    /* Get the Thread Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_TERMINATE,
                                       PsThreadType,
                                       KeGetPreviousMode(),
                                       (PVOID*)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) {
        
        DPRINT1("Could not reference thread object\n");
        return(Status);
    }
   
    /* Make sure this is not a system thread */
    if (PsIsSystemThread(Thread)) {
    
        DPRINT1("Trying to Terminate a system thread!\n");
        ObDereferenceObject(Thread);
        return STATUS_INVALID_PARAMETER;
    }
     
    /* Check to see if we're running in the same thread */
    if (Thread != PsGetCurrentThread())  {
                 
        /* we need to lock the process to make sure it's not already terminating */
        PsLockProcess(Thread->ThreadsProcess, FALSE);
        
        /* This isn't our thread, terminate it if not already done */
        if (!Thread->HasTerminated) {
         
             Thread->HasTerminated = TRUE;
             
             /* Terminate it */
             PspTerminateThreadByPointer(Thread, ExitStatus);
        }
        
        PsUnlockProcess(Thread->ThreadsProcess);
        
        /* Dereference the Thread and return */
        ObDereferenceObject(Thread);
        
    } else {

        Thread->HasTerminated = TRUE;
        
        /* it's safe to dereference thread, there's at least the keep-alive
           reference which will be removed by the thread reaper causing the
           thread to be finally destroyed */
        ObDereferenceObject(Thread);
            
        /* Terminate him, he's ours */
        PspExitThread(ExitStatus);

        /* We do never reach this point */
        KEBUGCHECK(0);
    }
    
    return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
NTSTATUS 
STDCALL
PsTerminateSystemThread(NTSTATUS ExitStatus)
{
    PETHREAD Thread = PsGetCurrentThread();
    
    /* Make sure this is a system thread */
    if (!PsIsSystemThread(Thread)) {
    
        DPRINT1("Trying to Terminate a non-system thread!\n");
        return STATUS_INVALID_PARAMETER;
    }
        
    /* Terminate it for real */
    PspExitThread(ExitStatus);
    
    /* we should never reach this point! */
    KEBUGCHECK(0);
    
    return(STATUS_SUCCESS);
}

NTSTATUS 
STDCALL
NtRegisterThreadTerminatePort(HANDLE PortHandle)
{
    NTSTATUS Status;
    PTERMINATION_PORT TerminationPort;
    PVOID TerminationLpcPort;
    PETHREAD Thread;
   
    PAGED_CODE();
    
    /* Get the Port */
    Status = ObReferenceObjectByHandle(PortHandle,
                                       PORT_ALL_ACCESS,
                                       LpcPortObjectType,
                                       KeGetPreviousMode(),
                                       &TerminationLpcPort,
                                       NULL);   
    if (!NT_SUCCESS(Status)) {
        
        DPRINT1("Failed to reference Port\n");
        return(Status);
    }
   
    /* Allocate the Port and make sure it suceeded */
    if((TerminationPort = ExAllocatePoolWithTag(NonPagedPool, 
                                                sizeof(PTERMINATION_PORT), 
                                                TAG('P', 's', 'T', '=')))) {
        
        /* Associate the Port */
        Thread = PsGetCurrentThread();
        TerminationPort->Port = TerminationLpcPort;
        DPRINT("TerminationPort: %p\n", TerminationPort);
        TerminationPort->Next = Thread->TerminationPort;
        Thread->TerminationPort = TerminationPort;
        DPRINT("TerminationPort: %p\n", Thread->TerminationPort);

        /* Return success */
        return(STATUS_SUCCESS);
   
    } else {
        
        /* Dereference and Fail */
        ObDereferenceObject(TerminationPort);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
}
