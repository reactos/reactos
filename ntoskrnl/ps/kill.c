/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/kill.c
 * PURPOSE:         Process Manager: Process and Thread Termination
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (xnavara@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY PspReaperListHead = { NULL, NULL };
WORK_QUEUE_ITEM PspReaperWorkItem;
LARGE_INTEGER ShortTime = {{-10 * 100 * 1000, -1}};

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
PspCatchCriticalBreak(IN PCHAR Message,
                      IN PVOID ProcessOrThread,
                      IN PCHAR ImageName)
{
    CHAR Action[2];
    BOOLEAN Handled = FALSE;
    PAGED_CODE();

    /* Check if a debugger is enabled */
    if (KdDebuggerEnabled)
    {
        /* Print out the message */
        DbgPrint(Message, ProcessOrThread, ImageName);
        do
        {
            /* If a debugger isn't present, don't prompt */
            if (KdDebuggerNotPresent) break;

            /* A debuger is active, prompt for action */
            DbgPrompt("Break, or Ignore (bi)?", Action, sizeof(Action));
            switch (Action[0])
            {
                /* Break */
                case 'B': case 'b':

                    /* Do a breakpoint */
                    DbgBreakPoint();

                /* Ignore */
                case 'I': case 'i':

                    /* Handle it */
                    Handled = TRUE;

                /* Unrecognized */
                default:
                    break;
            }
        } while (!Handled);
    }

    /* Did we ultimately handle this? */
    if (!Handled)
    {
        /* We didn't, bugcheck */
        KeBugCheckEx(CRITICAL_OBJECT_TERMINATION,
                     ((PKPROCESS)ProcessOrThread)->Header.Type,
                     (ULONG_PTR)ProcessOrThread,
                     (ULONG_PTR)ImageName,
                     (ULONG_PTR)Message);
    }
}

NTSTATUS
NTAPI
PspTerminateProcess(IN PEPROCESS Process,
                    IN NTSTATUS ExitStatus)
{
    PETHREAD Thread;
    NTSTATUS Status = STATUS_NOTHING_TO_TERMINATE;
    PAGED_CODE();
    PSTRACE(PS_KILL_DEBUG,
            "Process: %p ExitStatus: %d\n", Process, ExitStatus);
    PSREFTRACE(Process);

    /* Check if this is a Critical Process */
    if (Process->BreakOnTermination)
    {
        /* Break to debugger */
        PspCatchCriticalBreak("Terminating critical process 0x%p (%s)\n",
                              Process,
                              Process->ImageFileName);
    }

    /* Set the delete flag */
    InterlockedOr((PLONG)&Process->Flags, PSF_PROCESS_DELETE_BIT);

    /* Get the first thread */
    Thread = PsGetNextProcessThread(Process, NULL);
    while (Thread)
    {
        /* Kill it */
        PspTerminateThreadByPointer(Thread, ExitStatus, FALSE);
        Thread = PsGetNextProcessThread(Process, Thread);

        /* We had at least one thread, so termination is OK */
        Status = STATUS_SUCCESS;
    }

    /* Check if there was nothing to terminate or if we have a debug port */
    if ((Status == STATUS_NOTHING_TO_TERMINATE) || (Process->DebugPort))
    {
        /* Clear the handle table anyway */
        ObClearProcessHandleTable(Process);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
PsTerminateProcess(IN PEPROCESS Process,
                   IN NTSTATUS ExitStatus)
{
    /* Call the internal API */
    return PspTerminateProcess(Process, ExitStatus);
}

VOID
NTAPI
PspShutdownProcessManager(VOID)
{
    PEPROCESS Process = NULL;

    /* Loop every process */
    Process = PsGetNextProcess(Process);
    while (Process)
    {
        /* Make sure this isn't the idle or initial process */
        if ((Process != PsInitialSystemProcess) && (Process != PsIdleProcess))
        {
            /* Kill it */
            PspTerminateProcess(Process, STATUS_SYSTEM_SHUTDOWN);
        }

        /* Get the next process */
        Process = PsGetNextProcess(Process);
    }
}

VOID
NTAPI
PspExitApcRundown(IN PKAPC Apc)
{
    PAGED_CODE();

    /* Free the APC */
    ExFreePool(Apc);
}

VOID
NTAPI
PspReapRoutine(IN PVOID Context)
{
    PSINGLE_LIST_ENTRY NextEntry;
    PETHREAD Thread;
    PSTRACE(PS_KILL_DEBUG, "Context: %p\n", Context);

    /* Start main loop */
    do
    {
        /* Write magic value and return the next entry to process */
        NextEntry = InterlockedExchangePointer((PVOID*)&PspReaperListHead.Flink,
                                               (PVOID)1);
        ASSERT((NextEntry != NULL) && (NextEntry != (PVOID)1));

        /* Start inner loop */
        do
        {
            /* Get the first Thread Entry */
            Thread = CONTAINING_RECORD(NextEntry, ETHREAD, ReaperLink);

            /* Delete this entry's kernel stack */
            MmDeleteKernelStack((PVOID)Thread->Tcb.StackBase,
                                Thread->Tcb.LargeStack);
            Thread->Tcb.InitialStack = NULL;

            /* Move to the next entry */
            NextEntry = NextEntry->Next;

            /* Dereference this thread */
            ObDereferenceObject(Thread);
        } while ((NextEntry != NULL) && (NextEntry != (PVOID)1));

        /* Remove magic value, keep looping if it got changed */
    } while (InterlockedCompareExchangePointer((PVOID*)&PspReaperListHead.Flink,
                                               NULL,
                                               (PVOID)1) != (PVOID)1);
}

#if DBG
VOID
NTAPI
PspCheckProcessList(VOID)
{
    PLIST_ENTRY Entry;

    KeAcquireGuardedMutex(&PspActiveProcessMutex);
    DbgPrint("# checking PsActiveProcessHead @ %p\n", &PsActiveProcessHead);
    for (Entry = PsActiveProcessHead.Flink;
         Entry != &PsActiveProcessHead;
         Entry = Entry->Flink)
    {
        PEPROCESS Process = CONTAINING_RECORD(Entry, EPROCESS, ActiveProcessLinks);
        POBJECT_HEADER Header;
        PVOID Info, HeaderLocation;

        /* Get the header and assume this is what we'll free */
        Header = OBJECT_TO_OBJECT_HEADER(Process);
        HeaderLocation = Header;

        /* To find the header, walk backwards from how we allocated */
        if ((Info = OBJECT_HEADER_TO_CREATOR_INFO(Header)))
        {
            HeaderLocation = Info;
        }
        if ((Info = OBJECT_HEADER_TO_NAME_INFO(Header)))
        {
            HeaderLocation = Info;
        }
        if ((Info = OBJECT_HEADER_TO_HANDLE_INFO(Header)))
        {
            HeaderLocation = Info;
        }
        if ((Info = OBJECT_HEADER_TO_QUOTA_INFO(Header)))
        {
            HeaderLocation = Info;
        }

        ExpCheckPoolAllocation(HeaderLocation, NonPagedPool, 'corP');
    }

    KeReleaseGuardedMutex(&PspActiveProcessMutex);
}
#endif

VOID
NTAPI
PspDeleteProcess(IN PVOID ObjectBody)
{
    PEPROCESS Process = (PEPROCESS)ObjectBody;
    KAPC_STATE ApcState;
    PAGED_CODE();
    PSTRACE(PS_KILL_DEBUG, "ObjectBody: %p\n", ObjectBody);
    PSREFTRACE(Process);

    /* Check if it has an Active Process Link */
    if (Process->ActiveProcessLinks.Flink)
    {
        /* Remove it from the Active List */
        KeAcquireGuardedMutex(&PspActiveProcessMutex);
        RemoveEntryList(&Process->ActiveProcessLinks);
        Process->ActiveProcessLinks.Flink = NULL;
        Process->ActiveProcessLinks.Blink = NULL;
        KeReleaseGuardedMutex(&PspActiveProcessMutex);
    }

    /* Check for Auditing information */
    if (Process->SeAuditProcessCreationInfo.ImageFileName)
    {
        /* Free it */
        ExFreePoolWithTag(Process->SeAuditProcessCreationInfo.ImageFileName,
                          TAG_SEPA);
        Process->SeAuditProcessCreationInfo.ImageFileName = NULL;
    }

    /* Check if we have a job */
    if (Process->Job)
    {
        /* Remove the process from the job */
        PspRemoveProcessFromJob(Process, Process->Job);

        /* Dereference it */
        ObDereferenceObject(Process->Job);
        Process->Job = NULL;
    }

    /* Increase the stack count */
    Process->Pcb.StackCount++;

    /* Check if we have a debug port */
    if (Process->DebugPort)
    {
        /* Deference the Debug Port */
        ObDereferenceObject(Process->DebugPort);
        Process->DebugPort = NULL;
    }

    /* Check if we have an exception port */
    if (Process->ExceptionPort)
    {
        /* Deference the Exception Port */
        ObDereferenceObject(Process->ExceptionPort);
        Process->ExceptionPort = NULL;
    }

    /* Check if we have a section object */
    if (Process->SectionObject)
    {
        /* Deference the Section Object */
        ObDereferenceObject(Process->SectionObject);
        Process->SectionObject = NULL;
    }

#if defined(_X86_)
    /* Clean Ldt and Vdm objects */
    PspDeleteLdt(Process);
    PspDeleteVdmObjects(Process);
#endif

    /* Delete the Object Table */
    if (Process->ObjectTable)
    {
        /* Attach to the process */
        KeStackAttachProcess(&Process->Pcb, &ApcState);

        /* Kill the Object Info */
        ObKillProcess(Process);

        /* Detach */
        KeUnstackDetachProcess(&ApcState);
    }

    /* Check if we have an address space, and clean it */
    if (Process->HasAddressSpace)
    {
        /* Attach to the process */
        KeStackAttachProcess(&Process->Pcb, &ApcState);

        /* Clean the Address Space */
        PspExitProcess(FALSE, Process);

        /* Detach */
        KeUnstackDetachProcess(&ApcState);

        /* Completely delete the Address Space */
        MmDeleteProcessAddressSpace(Process);
    }

    /* See if we have a PID */
    if (Process->UniqueProcessId)
    {
        /* Delete the PID */
        if (!(ExDestroyHandle(PspCidTable, Process->UniqueProcessId, NULL)))
        {
            /* Something wrong happened, bugcheck */
            KeBugCheck(CID_HANDLE_DELETION);
        }
    }

    /* Cleanup security information */
    PspDeleteProcessSecurity(Process);

    /* Check if we have kept information on the Working Set */
    if (Process->WorkingSetWatch)
    {
        /* Free it */
        ExFreePool(Process->WorkingSetWatch);

        /* And return the quota it was taking up */
        PsReturnProcessNonPagedPoolQuota(Process, 0x2000);
    }

    /* Dereference the Device Map */
    ObDereferenceDeviceMap(Process);

    /*
     * Dereference the quota block, the function
     * will invoke a quota block cleanup if the
     * block itself is no longer used by anybody.
     */
    PspDereferenceQuotaBlock(Process, Process->QuotaBlock);
}

VOID
NTAPI
PspDeleteThread(IN PVOID ObjectBody)
{
    PETHREAD Thread = (PETHREAD)ObjectBody;
    PEPROCESS Process = Thread->ThreadsProcess;
    PAGED_CODE();
    PSTRACE(PS_KILL_DEBUG, "ObjectBody: %p\n", ObjectBody);
    PSREFTRACE(Thread);
    ASSERT(Thread->Tcb.Win32Thread == NULL);

    /* Check if we have a stack */
    if (Thread->Tcb.InitialStack)
    {
        /* Release it */
        MmDeleteKernelStack((PVOID)Thread->Tcb.StackBase,
                            Thread->Tcb.LargeStack);
    }

    /* Check if we have a CID Handle */
    if (Thread->Cid.UniqueThread)
    {
        /* Delete the CID Handle */
        if (!(ExDestroyHandle(PspCidTable, Thread->Cid.UniqueThread, NULL)))
        {
            /* Something wrong happened, bugcheck */
            KeBugCheck(CID_HANDLE_DELETION);
        }
    }

    /* Cleanup impersionation information */
    PspDeleteThreadSecurity(Thread);

    /* Make sure the thread was inserted, before continuing */
    if (!Process) return;

    /* Check if the thread list is valid */
    if (Thread->ThreadListEntry.Flink)
    {
        /* Lock the thread's process */
        KeEnterCriticalRegion();
        ExAcquirePushLockExclusive(&Process->ProcessLock);

        /* Remove us from the list */
        RemoveEntryList(&Thread->ThreadListEntry);

        /* Release the lock */
        ExReleasePushLockExclusive(&Process->ProcessLock);
        KeLeaveCriticalRegion();
    }

    /* Dereference the Process */
    ObDereferenceObject(Process);
}

/*
 * FUNCTION: Terminates the current thread
 * See "Windows Internals" - Chapter 13, Page 50-53
 */
VOID
NTAPI
PspExitThread(IN NTSTATUS ExitStatus)
{
    CLIENT_DIED_MSG TerminationMsg;
    NTSTATUS Status;
    PTEB Teb;
    PEPROCESS CurrentProcess;
    PETHREAD Thread, OtherThread, PreviousThread = NULL;
    PVOID DeallocationStack;
    SIZE_T Dummy;
    BOOLEAN Last = FALSE;
    PTERMINATION_PORT TerminationPort, NextPort;
    PLIST_ENTRY FirstEntry, CurrentEntry;
    PKAPC Apc;
    PTOKEN PrimaryToken;
    PAGED_CODE();
    PSTRACE(PS_KILL_DEBUG, "ExitStatus: %d\n", ExitStatus);

    /* Get the Current Thread and Process */
    Thread = PsGetCurrentThread();
    CurrentProcess = Thread->ThreadsProcess;
    ASSERT((Thread) == PsGetCurrentThread());

    /* Can't terminate a thread if it attached another process */
    if (KeIsAttachedProcess())
    {
        /* Bugcheck */
        KeBugCheckEx(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)CurrentProcess,
                     (ULONG_PTR)Thread->Tcb.ApcState.Process,
                     (ULONG_PTR)Thread->Tcb.ApcStateIndex,
                     (ULONG_PTR)Thread);
    }

    /* Lower to Passive Level */
    KeLowerIrql(PASSIVE_LEVEL);

    /* Can't be a worker thread */
    if (Thread->ActiveExWorker)
    {
        /* Bugcheck */
        KeBugCheckEx(ACTIVE_EX_WORKER_THREAD_TERMINATION,
                     (ULONG_PTR)Thread,
                     0,
                     0,
                     0);
    }

    /* Can't have pending APCs */
    if (Thread->Tcb.CombinedApcDisable != 0)
    {
        /* Bugcheck */
        KeBugCheckEx(KERNEL_APC_PENDING_DURING_EXIT,
                     0,
                     Thread->Tcb.CombinedApcDisable,
                     0,
                     1);
    }

    /* Lock the thread */
    ExWaitForRundownProtectionRelease(&Thread->RundownProtect);

    /* Cleanup the power state */
    PopCleanupPowerState((PPOWER_STATE)&Thread->Tcb.PowerState);

    /* Call the WMI Callback for Threads */
    //WmiTraceThread(Thread, NULL, FALSE);

    /* Run Thread Notify Routines before we desintegrate the thread */
    PspRunCreateThreadNotifyRoutines(Thread, FALSE);

    /* Lock the Process before we modify its thread entries */
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&CurrentProcess->ProcessLock);

    /* Decrease the active thread count, and check if it's 0 */
    if (!(--CurrentProcess->ActiveThreads))
    {
        /* Set the delete flag */
        InterlockedOr((PLONG)&CurrentProcess->Flags, PSF_PROCESS_DELETE_BIT);

        /* Remember we are last */
        Last = TRUE;

        /* Check if this termination is due to the thread dying */
        if (ExitStatus == STATUS_THREAD_IS_TERMINATING)
        {
            /* Check if the last thread was pending */
            if (CurrentProcess->ExitStatus == STATUS_PENDING)
            {
                /* Use the last exit status */
                CurrentProcess->ExitStatus = CurrentProcess->
                                             LastThreadExitStatus;
            }
        }
        else
        {
            /* Just a normal exit, write the code */
            CurrentProcess->ExitStatus = ExitStatus;
        }

        /* Loop all the current threads */
        FirstEntry = &CurrentProcess->ThreadListHead;
        CurrentEntry = FirstEntry->Flink;
        while (FirstEntry != CurrentEntry)
        {
            /* Get the thread on the list */
            OtherThread = CONTAINING_RECORD(CurrentEntry,
                                            ETHREAD,
                                            ThreadListEntry);

            /* Check if it's a thread that's still alive */
            if ((OtherThread != Thread) &&
                !(KeReadStateThread(&OtherThread->Tcb)) &&
                (ObReferenceObjectSafe(OtherThread)))
            {
                /* It's a live thread and we referenced it, unlock process */
                ExReleasePushLockExclusive(&CurrentProcess->ProcessLock);
                KeLeaveCriticalRegion();

                /* Wait on the thread */
                KeWaitForSingleObject(OtherThread,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                /* Check if we had a previous thread to dereference */
                if (PreviousThread) ObDereferenceObject(PreviousThread);

                /* Remember the thread and re-lock the process */
                PreviousThread = OtherThread;
                KeEnterCriticalRegion();
                ExAcquirePushLockExclusive(&CurrentProcess->ProcessLock);
            }

            /* Go to the next thread */
            CurrentEntry = CurrentEntry->Flink;
        }
    }
    else if (ExitStatus != STATUS_THREAD_IS_TERMINATING)
    {
        /* Write down the exit status of the last thread to get killed */
        CurrentProcess->LastThreadExitStatus = ExitStatus;
    }

    /* Unlock the Process */
    ExReleasePushLockExclusive(&CurrentProcess->ProcessLock);
    KeLeaveCriticalRegion();

    /* Check if we had a previous thread to dereference */
    if (PreviousThread) ObDereferenceObject(PreviousThread);

    /* Check if the process has a debug port and if this is a user thread */
    if ((CurrentProcess->DebugPort) && !(Thread->SystemThread))
    {
        /* Notify the Debug API. */
        Last ? DbgkExitProcess(CurrentProcess->ExitStatus) :
               DbgkExitThread(ExitStatus);
    }

    /* Check if this is a Critical Thread */
    if ((KdDebuggerEnabled) && (Thread->BreakOnTermination))
    {
        /* Break to debugger */
        PspCatchCriticalBreak("Critical thread 0x%p (in %s) exited\n",
                              Thread,
                              CurrentProcess->ImageFileName);
    }

    /* Check if it's the last thread and this is a Critical Process */
    if ((Last) && (CurrentProcess->BreakOnTermination))
    {
        /* Check if a debugger is here to handle this */
        if (KdDebuggerEnabled)
        {
            /* Break to debugger */
            PspCatchCriticalBreak("Critical  process 0x%p (in %s) exited\n",
                                  CurrentProcess,
                                  CurrentProcess->ImageFileName);
        }
        else
        {
            /* Bugcheck, we can't allow this */
            KeBugCheckEx(CRITICAL_PROCESS_DIED,
                         (ULONG_PTR)CurrentProcess,
                         0,
                         0,
                         0);
        }
    }

    /* Sanity check */
    ASSERT(Thread->Tcb.CombinedApcDisable == 0);

    /* Process the Termination Ports */
    TerminationPort = Thread->TerminationPort;
    if (TerminationPort)
    {
        /* Setup the message header */
        TerminationMsg.h.u2.ZeroInit = 0;
        TerminationMsg.h.u2.s2.Type = LPC_CLIENT_DIED;
        TerminationMsg.h.u1.s1.TotalLength = sizeof(TerminationMsg);
        TerminationMsg.h.u1.s1.DataLength = sizeof(TerminationMsg) -
                                            sizeof(PORT_MESSAGE);

        /* Loop each port */
        do
        {
            /* Save the Create Time */
            TerminationMsg.CreateTime = Thread->CreateTime;

            /* Loop trying to send message */
            while (TRUE)
            {
                /* Send the LPC Message */
                Status = LpcRequestPort(TerminationPort->Port,
                                        &TerminationMsg.h);
                if ((Status == STATUS_NO_MEMORY) ||
                    (Status == STATUS_INSUFFICIENT_RESOURCES))
                {
                    /* Wait a bit and try again */
                    KeDelayExecutionThread(KernelMode, FALSE, &ShortTime);
                    continue;
                }
                break;
            }

            /* Dereference this LPC Port */
            ObDereferenceObject(TerminationPort->Port);

            /* Move to the next one */
            NextPort = TerminationPort->Next;

            /* Free the Termination Port Object */
            ExFreePoolWithTag(TerminationPort, '=TsP');

            /* Keep looping as long as there is a port */
            TerminationPort = NextPort;
        } while (TerminationPort);
    }
    else if (((ExitStatus == STATUS_THREAD_IS_TERMINATING) &&
              (Thread->DeadThread)) ||
             !(Thread->DeadThread))
    {
        /*
         * This case is special and deserves some extra comments. What
         * basically happens here is that this thread doesn't have a termination
         * port, which means that it died before being fully created. Since we
         * still have to notify an LPC Server, we'll use the exception port,
         * which we know exists. However, we need to know how far the thread
         * actually got created. We have three possibilities:
         *
         *  - NtCreateThread returned an error really early: DeadThread is set.
         *  - NtCreateThread managed to create the thread: DeadThread is off.
         *  - NtCreateThread was creating the thread (with DeadThread set,
         *    but the thread got killed prematurely: STATUS_THREAD_IS_TERMINATING
         *    is our exit code.)
         *
         * For the 2 & 3rd scenarios, the thread has been created far enough to
         * warrant notification to the LPC Server.
         */

        /* Setup the message header */
        TerminationMsg.h.u2.ZeroInit = 0;
        TerminationMsg.h.u2.s2.Type = LPC_CLIENT_DIED;
        TerminationMsg.h.u1.s1.TotalLength = sizeof(TerminationMsg);
        TerminationMsg.h.u1.s1.DataLength = sizeof(TerminationMsg) -
                                            sizeof(PORT_MESSAGE);

        /* Make sure the process has an exception port */
        if (CurrentProcess->ExceptionPort)
        {
            /* Save the Create Time */
            TerminationMsg.CreateTime = Thread->CreateTime;

            /* Loop trying to send message */
            while (TRUE)
            {
                /* Send the LPC Message */
                Status = LpcRequestPort(CurrentProcess->ExceptionPort,
                                        &TerminationMsg.h);
                if ((Status == STATUS_NO_MEMORY) ||
                    (Status == STATUS_INSUFFICIENT_RESOURCES))
                {
                    /* Wait a bit and try again */
                    KeDelayExecutionThread(KernelMode, FALSE, &ShortTime);
                    continue;
                }
                break;
            }
        }
    }

    /* Rundown Win32 Thread if there is one */
    if (Thread->Tcb.Win32Thread) PspW32ThreadCallout(Thread,
                                                     PsW32ThreadCalloutExit);

    /* If we are the last thread and have a W32 Process */
    if ((Last) && (CurrentProcess->Win32Process))
    {
        /* Run it down too */
        PspW32ProcessCallout(CurrentProcess, FALSE);
    }

    /* Make sure Stack Swap is enabled */
    if (!Thread->Tcb.EnableStackSwap)
    {
        /* Stack swap really shouldn't be disabled during exit! */
        KeBugCheckEx(KERNEL_STACK_LOCKED_AT_EXIT, 0, 0, 0, 0);
    }

    /* Cancel I/O for the thread. */
    IoCancelThreadIo(Thread);

    /* Rundown Timers */
    ExTimerRundown();

    /* FIXME: Rundown Registry Notifications (NtChangeNotify)
    CmNotifyRunDown(Thread); */

    /* Rundown Mutexes */
    KeRundownThread();

    /* Check if we have a TEB */
    Teb = Thread->Tcb.Teb;
    if (Teb)
    {
        /* Check if the thread is still alive */
        if (!Thread->DeadThread)
        {
            /* Check if we need to free its stack */
            if (Teb->FreeStackOnTermination)
            {
                /* Set the TEB's Deallocation Stack as the Base Address */
                Dummy = 0;
                DeallocationStack = Teb->DeallocationStack;

                /* Free the Thread's Stack */
                ZwFreeVirtualMemory(NtCurrentProcess(),
                                    &DeallocationStack,
                                    &Dummy,
                                    MEM_RELEASE);
            }

            /* Free the debug handle */
            if (Teb->DbgSsReserved[1]) ObCloseHandle(Teb->DbgSsReserved[1],
                                                     UserMode);
        }

        /* Decommit the TEB */
        MmDeleteTeb(CurrentProcess, Teb);
        Thread->Tcb.Teb = NULL;
    }

    /* Free LPC Data */
    LpcExitThread(Thread);

    /* Save the exit status and exit time */
    Thread->ExitStatus = ExitStatus;
    KeQuerySystemTime(&Thread->ExitTime);

    /* Sanity check */
    ASSERT(Thread->Tcb.CombinedApcDisable == 0);

    /* Check if this is the final thread or not */
    if (Last)
    {
        /* Set the process exit time */
        CurrentProcess->ExitTime = Thread->ExitTime;

        /* Exit the process */
        PspExitProcess(TRUE, CurrentProcess);

        /* Get the process token and check if we need to audit */
        PrimaryToken = PsReferencePrimaryToken(CurrentProcess);
        if (SeDetailedAuditingWithToken(PrimaryToken))
        {
            /* Audit the exit */
            SeAuditProcessExit(CurrentProcess);
        }

        /* Dereference the process token */
        ObFastDereferenceObject(&CurrentProcess->Token, PrimaryToken);

        /* Check if this is a VDM Process and rundown the VDM DPCs if so */
        if (CurrentProcess->VdmObjects) { /* VdmRundownDpcs(CurrentProcess); */ }

        /* Kill the process in the Object Manager */
        ObKillProcess(CurrentProcess);

        /* Check if we have a section object */
        if (CurrentProcess->SectionObject)
        {
            /* Dereference and clear the Section Object */
            ObDereferenceObject(CurrentProcess->SectionObject);
            CurrentProcess->SectionObject = NULL;
        }

        /* Check if the process is part of a job */
        if (CurrentProcess->Job)
        {
            /* Remove the process from the job */
            PspExitProcessFromJob(CurrentProcess->Job, CurrentProcess);
        }
    }

    /* Disable APCs */
    KeEnterCriticalRegion();

    /* Disable APC queueing, force a resumption */
    Thread->Tcb.ApcQueueable = FALSE;
    KeForceResumeThread(&Thread->Tcb);

    /* Re-enable APCs */
    KeLeaveCriticalRegion();

    /* Flush the User APCs */
    FirstEntry = KeFlushQueueApc(&Thread->Tcb, UserMode);
    if (FirstEntry)
    {
        /* Start with the first entry */
        CurrentEntry = FirstEntry;
        do
        {
           /* Get the APC */
           Apc = CONTAINING_RECORD(CurrentEntry, KAPC, ApcListEntry);

           /* Move to the next one */
           CurrentEntry = CurrentEntry->Flink;

           /* Rundown the APC or de-allocate it */
           if (Apc->RundownRoutine)
           {
              /* Call its own routine */
              Apc->RundownRoutine(Apc);
           }
           else
           {
              /* Do it ourselves */
              ExFreePool(Apc);
           }
        }
        while (CurrentEntry != FirstEntry);
    }

    /* Clean address space if this was the last thread */
    if (Last) MmCleanProcessAddressSpace(CurrentProcess);

    /* Call the Lego routine */
    if (Thread->Tcb.LegoData) PspRunLegoRoutine(&Thread->Tcb);

    /* Flush the APC queue, which should be empty */
    FirstEntry = KeFlushQueueApc(&Thread->Tcb, KernelMode);
    if ((FirstEntry) || (Thread->Tcb.CombinedApcDisable != 0))
    {
        /* Bugcheck time */
        KeBugCheckEx(KERNEL_APC_PENDING_DURING_EXIT,
                     (ULONG_PTR)FirstEntry,
                     Thread->Tcb.CombinedApcDisable,
                     KeGetCurrentIrql(),
                     0);
    }

    /* Signal the process if this was the last thread */
    if (Last) KeSetProcess(&CurrentProcess->Pcb, 0, FALSE);

    /* Terminate the Thread from the Scheduler */
    KeTerminateThread(0);
}

VOID
NTAPI
PsExitSpecialApc(IN PKAPC Apc,
                 IN OUT PKNORMAL_ROUTINE* NormalRoutine,
                 IN OUT PVOID* NormalContext,
                 IN OUT PVOID* SystemArgument1,
                 IN OUT PVOID* SystemArgument2)
{
    NTSTATUS Status;
    PAGED_CODE();
    PSTRACE(PS_KILL_DEBUG,
            "Apc: %p SystemArgument2: %p\n", Apc, SystemArgument2);

    /* Don't do anything unless we are in User-Mode */
    if (Apc->SystemArgument2)
    {
        /* Free the APC */
        Status = PtrToUlong(Apc->NormalContext);
        PspExitApcRundown(Apc);

        /* Terminate the Thread */
        PspExitThread(Status);
    }
}

VOID
NTAPI
PspExitNormalApc(IN PVOID NormalContext,
                 IN PVOID SystemArgument1,
                 IN PVOID SystemArgument2)
{
    PKAPC Apc = (PKAPC)SystemArgument1;
    PETHREAD Thread = PsGetCurrentThread();
    PAGED_CODE();
    PSTRACE(PS_KILL_DEBUG, "SystemArgument2: %p\n", SystemArgument2);

    /* This should never happen */
    ASSERT(!(((ULONG_PTR)SystemArgument2) & 1));

    /* If we're here, this is not a System Thread, so kill it from User-Mode */
    KeInitializeApc(Apc,
                    &Thread->Tcb,
                    OriginalApcEnvironment,
                    PsExitSpecialApc,
                    PspExitApcRundown,
                    PspExitNormalApc,
                    UserMode,
                    NormalContext);

    /* Now insert the APC with the User-Mode Flag */
    if (!(KeInsertQueueApc(Apc,
                           Apc,
                           (PVOID)((ULONG_PTR)SystemArgument2 | 1),
                           2)))
    {
        /* Failed to insert, free the APC */
        PspExitApcRundown(Apc);
    }

    /* Set the APC Pending flag */
    Thread->Tcb.ApcState.UserApcPending = TRUE;
}

/*
 * See "Windows Internals" - Chapter 13, Page 49
 */
NTSTATUS
NTAPI
PspTerminateThreadByPointer(IN PETHREAD Thread,
                            IN NTSTATUS ExitStatus,
                            IN BOOLEAN bSelf)
{
    PKAPC Apc;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Flags;
    PAGED_CODE();
    PSTRACE(PS_KILL_DEBUG, "Thread: %p ExitStatus: %d\n", Thread, ExitStatus);
    PSREFTRACE(Thread);

    /* Check if this is a Critical Thread, and Bugcheck */
    if (Thread->BreakOnTermination)
    {
        /* Break to debugger */
        PspCatchCriticalBreak("Terminating critical thread 0x%p (%s)\n",
                              Thread,
                              Thread->ThreadsProcess->ImageFileName);
    }

    /* Check if we are already inside the thread */
    if ((bSelf) || (PsGetCurrentThread() == Thread))
    {
        /* This should only happen at passive */
        ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

        /* Mark it as terminated */
        PspSetCrossThreadFlag(Thread, CT_TERMINATED_BIT);

        /* Directly terminate the thread */
        PspExitThread(ExitStatus);
    }

    /* This shouldn't be a system thread */
    if (Thread->SystemThread) return STATUS_ACCESS_DENIED;

    /* Allocate the APC */
    Apc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), TAG_TERMINATE_APC);
    if (!Apc) return STATUS_INSUFFICIENT_RESOURCES;

    /* Set the Terminated Flag */
    Flags = Thread->CrossThreadFlags | CT_TERMINATED_BIT;

    /* Set it, and check if it was already set while we were running */
    if (!(InterlockedExchange((PLONG)&Thread->CrossThreadFlags, Flags) &
          CT_TERMINATED_BIT))
    {
        /* Initialize a Kernel Mode APC to Kill the Thread */
        KeInitializeApc(Apc,
                        &Thread->Tcb,
                        OriginalApcEnvironment,
                        PsExitSpecialApc,
                        PspExitApcRundown,
                        PspExitNormalApc,
                        KernelMode,
                        UlongToPtr(ExitStatus));

        /* Insert it into the APC Queue */
        if (!KeInsertQueueApc(Apc, Apc, NULL, 2))
        {
            /* The APC was already in the queue, fail */
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            /* Forcefully resume the thread and return */
            KeForceResumeThread(&Thread->Tcb);
            return Status;
        }
    }

    /* We failed, free the APC */
    ExFreePoolWithTag(Apc, TAG_TERMINATE_APC);

    /* Return Status */
    return Status;
}

BOOLEAN
NTAPI
PspIsProcessExiting(IN PEPROCESS Process)
{
    return Process->Flags & PSF_PROCESS_EXITING_BIT;
}

VOID
NTAPI
PspExitProcess(IN BOOLEAN LastThread,
               IN PEPROCESS Process)
{
    ULONG Actual;
    PAGED_CODE();
    PSTRACE(PS_KILL_DEBUG,
            "LastThread: %u Process: %p\n", LastThread, Process);
    PSREFTRACE(Process);

    /* Set Process Exit flag */
    InterlockedOr((PLONG)&Process->Flags, PSF_PROCESS_EXITING_BIT);

    /* Check if we are the last thread */
    if (LastThread)
    {
        /* Notify the WMI Process Callback */
        //WmiTraceProcess(Process, FALSE);

        /* Run the Notification Routines */
        PspRunCreateProcessNotifyRoutines(Process, FALSE);
    }

    /* Cleanup the power state */
    PopCleanupPowerState((PPOWER_STATE)&Process->Pcb.PowerState);

    /* Clear the security port */
    if (!Process->SecurityPort)
    {
        /* So we don't double-dereference */
        Process->SecurityPort = (PVOID)1;
    }
    else if (Process->SecurityPort != (PVOID)1)
    {
        /* Dereference it */
        ObDereferenceObject(Process->SecurityPort);
        Process->SecurityPort = (PVOID)1;
    }

    /* Check if we are the last thread */
    if (LastThread)
    {
        /* Check if we have to set the Timer Resolution */
        if (Process->SetTimerResolution)
        {
            /* Set it to default */
            ZwSetTimerResolution(KeMaximumIncrement, 0, &Actual);
        }

        /* Check if we are part of a Job that has a completion port */
        if ((Process->Job) && (Process->Job->CompletionPort))
        {
            /* FIXME: Check job status code and do I/O completion if needed */
        }

        /* FIXME: Notify the Prefetcher */
    }
    else
    {
        /* Clear process' address space here */
        MmCleanProcessAddressSpace(Process);
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsTerminateSystemThread(IN NTSTATUS ExitStatus)
{
    PETHREAD Thread = PsGetCurrentThread();

    /* Make sure this is a system thread */
    if (!Thread->SystemThread) return STATUS_INVALID_PARAMETER;

    /* Terminate it for real */
    return PspTerminateThreadByPointer(Thread, ExitStatus, TRUE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtTerminateProcess(IN HANDLE ProcessHandle OPTIONAL,
                   IN NTSTATUS ExitStatus)
{
    NTSTATUS Status;
    PEPROCESS Process, CurrentProcess = PsGetCurrentProcess();
    PETHREAD Thread, CurrentThread = PsGetCurrentThread();
    BOOLEAN KillByHandle;
    PAGED_CODE();
    PSTRACE(PS_KILL_DEBUG,
            "ProcessHandle: %p ExitStatus: %d\n", ProcessHandle, ExitStatus);

    /* Were we passed a process handle? */
    if (ProcessHandle)
    {
        /* Yes we were, use it */
        KillByHandle = TRUE;
    }
    else
    {
        /* We weren't... we assume this is suicide */
        KillByHandle = FALSE;
        ProcessHandle = NtCurrentProcess();
    }

    /* Get the Process Object */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_TERMINATE,
                                       PsProcessType,
                                       KeGetPreviousMode(),
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return(Status);

    /* Check if this is a Critical Process, and Bugcheck */
    if (Process->BreakOnTermination)
    {
        /* Break to debugger */
        PspCatchCriticalBreak("Terminating critical process 0x%p (%s)\n",
                              Process,
                              Process->ImageFileName);
    }

    /* Lock the Process */
    if (!ExAcquireRundownProtection(&Process->RundownProtect))
    {
        /* Failed to lock, fail */
        ObDereferenceObject(Process);
        return STATUS_PROCESS_IS_TERMINATING;
    }

    /* Set the delete flag, unless the process is comitting suicide */
    if (KillByHandle) PspSetProcessFlag(Process, PSF_PROCESS_DELETE_BIT);

    /* Get the first thread */
    Status = STATUS_NOTHING_TO_TERMINATE;
    Thread = PsGetNextProcessThread(Process, NULL);
    if (Thread)
    {
        /* We know we have at least a thread */
        Status = STATUS_SUCCESS;

        /* Loop and kill the others */
        do
        {
            /* Ensure it's not ours*/
            if (Thread != CurrentThread)
            {
                /* Kill it */
                PspTerminateThreadByPointer(Thread, ExitStatus, FALSE);
            }

            /* Move to the next thread */
            Thread = PsGetNextProcessThread(Process, Thread);
        } while (Thread);
    }

    /* Unlock the process */
    ExReleaseRundownProtection(&Process->RundownProtect);

    /* Check if we are killing ourselves */
    if (Process == CurrentProcess)
    {
        /* Also make sure the caller gave us our handle */
        if (KillByHandle)
        {
            /* Dereference the process */
            ObDereferenceObject(Process);

            /* Terminate ourselves */
            PspTerminateThreadByPointer(CurrentThread, ExitStatus, TRUE);
        }
    }
    else if (ExitStatus == DBG_TERMINATE_PROCESS)
    {
        /* Disable debugging on this process */
        DbgkClearProcessDebugObject(Process, NULL);
    }

    /* Check if there was nothing to terminate, or if we have a Debug Port */
    if ((Status == STATUS_NOTHING_TO_TERMINATE) ||
        ((Process->DebugPort) && (KillByHandle)))
    {
        /* Clear the handle table */
        ObClearProcessHandleTable(Process);

        /* Return status now */
        Status = STATUS_SUCCESS;
    }

    /* Decrease the reference count we added */
    ObDereferenceObject(Process);

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtTerminateThread(IN HANDLE ThreadHandle,
                  IN NTSTATUS ExitStatus)
{
    PETHREAD Thread;
    PETHREAD CurrentThread = PsGetCurrentThread();
    NTSTATUS Status;
    PAGED_CODE();
    PSTRACE(PS_KILL_DEBUG,
            "ThreadHandle: %p ExitStatus: %d\n", ThreadHandle, ExitStatus);

    /* Handle the special NULL case */
    if (!ThreadHandle)
    {
        /* Check if we're the only thread left */
        if (PsGetCurrentProcess()->ActiveThreads == 1)
        {
            /* This is invalid */
            return STATUS_CANT_TERMINATE_SELF;
        }

        /* Terminate us directly */
        goto TerminateSelf;
    }
    else if (ThreadHandle == NtCurrentThread())
    {
TerminateSelf:
        /* Terminate this thread */
        return PspTerminateThreadByPointer(CurrentThread,
                                           ExitStatus,
                                           TRUE);
    }

    /* We are terminating another thread, get the Thread Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_TERMINATE,
                                       PsThreadType,
                                       KeGetPreviousMode(),
                                       (PVOID*)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check to see if we're running in the same thread */
    if (Thread != CurrentThread)
    {
        /* Terminate it */
        Status = PspTerminateThreadByPointer(Thread, ExitStatus, FALSE);

        /* Dereference the Thread and return */
        ObDereferenceObject(Thread);
    }
    else
    {
        /* Dereference the thread and terminate ourselves */
        ObDereferenceObject(Thread);
        goto TerminateSelf;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtRegisterThreadTerminatePort(IN HANDLE PortHandle)
{
    NTSTATUS Status;
    PTERMINATION_PORT TerminationPort;
    PVOID TerminationLpcPort;
    PETHREAD Thread;
    PAGED_CODE();
    PSTRACE(PS_KILL_DEBUG, "PortHandle: %p\n", PortHandle);

    /* Get the Port */
    Status = ObReferenceObjectByHandle(PortHandle,
                                       PORT_ALL_ACCESS,
                                       LpcPortObjectType,
                                       KeGetPreviousMode(),
                                       &TerminationLpcPort,
                                       NULL);
    if (!NT_SUCCESS(Status)) return(Status);

    /* Allocate the Port and make sure it suceeded */
    TerminationPort = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(TERMINATION_PORT),
                                            '=TsP');
    if(TerminationPort)
    {
        /* Associate the Port */
        Thread = PsGetCurrentThread();
        TerminationPort->Port = TerminationLpcPort;
        TerminationPort->Next = Thread->TerminationPort;
        Thread->TerminationPort = TerminationPort;

        /* Return success */
        return STATUS_SUCCESS;
    }

    /* Dereference and Fail */
    ObDereferenceObject(TerminationLpcPort);
    return STATUS_INSUFFICIENT_RESOURCES;
}
