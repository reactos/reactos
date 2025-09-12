/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/dbgk/dbgkobj.c
 * PURPOSE:         User-Mode Debugging Support, Debug Object Management.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

POBJECT_TYPE DbgkDebugObjectType;
FAST_MUTEX DbgkpProcessDebugPortMutex;
ULONG DbgkpTraceLevel = 0;

GENERIC_MAPPING DbgkDebugObjectMapping =
{
    STANDARD_RIGHTS_READ    | DEBUG_OBJECT_WAIT_STATE_CHANGE,
    STANDARD_RIGHTS_WRITE   | DEBUG_OBJECT_ADD_REMOVE_PROCESS,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    DEBUG_OBJECT_ALL_ACCESS
};

static const INFORMATION_CLASS_INFO DbgkpDebugObjectInfoClass[] =
{
    /* DebugObjectUnusedInformation */
    IQS_SAME(ULONG, ULONG, 0),
    /* DebugObjectKillProcessOnExitInformation */
    IQS_SAME(DEBUG_OBJECT_KILL_PROCESS_ON_EXIT_INFORMATION, ULONG, ICIF_SET),
};

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
DbgkpQueueMessage(IN PEPROCESS Process,
                  IN PETHREAD Thread,
                  IN PDBGKM_MSG Message,
                  IN ULONG Flags,
                  IN PDEBUG_OBJECT TargetObject OPTIONAL)
{
    PDEBUG_EVENT DebugEvent;
    DEBUG_EVENT LocalDebugEvent;
    PDEBUG_OBJECT DebugObject;
    NTSTATUS Status;
    BOOLEAN NewEvent;
    PAGED_CODE();
    DBGKTRACE(DBGK_MESSAGE_DEBUG,
              "Process: %p Thread: %p Message: %p Flags: %lx\n",
              Process, Thread, Message, Flags);

    /* Check if we have to allocate a debug event */
    NewEvent = (Flags & DEBUG_EVENT_NOWAIT) ? TRUE : FALSE;
    if (NewEvent)
    {
        /* Allocate it */
        DebugEvent = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(DEBUG_EVENT),
                                           TAG_DEBUG_EVENT);
        if (!DebugEvent) return STATUS_INSUFFICIENT_RESOURCES;

        /* Set flags */
        DebugEvent->Flags = Flags | DEBUG_EVENT_INACTIVE;

        /* Reference the thread and process */
        ObReferenceObject(Thread);
        ObReferenceObject(Process);

        /* Set the current thread */
        DebugEvent->BackoutThread = PsGetCurrentThread();

        /* Set the debug object */
        DebugObject = TargetObject;
    }
    else
    {
        /* Use the debug event on the stack */
        DebugEvent = &LocalDebugEvent;
        DebugEvent->Flags = Flags;

        /* Acquire the port lock */
        ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

        /* Get the debug object */
        DebugObject = Process->DebugPort;

        /* Check what kind of API message this is */
        switch (Message->ApiNumber)
        {
            /* Process or thread creation */
            case DbgKmCreateThreadApi:
            case DbgKmCreateProcessApi:

                /* Make sure we're not skipping creation messages */
                if (Thread->SkipCreationMsg) DebugObject = NULL;
                break;

            /* Process or thread exit */
            case DbgKmExitThreadApi:
            case DbgKmExitProcessApi:

                /* Make sure we're not skipping exit messages */
                if (Thread->SkipTerminationMsg) DebugObject = NULL;

            /* No special handling for other messages */
            default:
                break;
        }
    }

    /* Setup the Debug Event */
    KeInitializeEvent(&DebugEvent->ContinueEvent, SynchronizationEvent, FALSE);
    DebugEvent->Process = Process;
    DebugEvent->Thread = Thread;
    DebugEvent->ApiMsg = *Message;
    DebugEvent->ClientId = Thread->Cid;

    /* Check if we have a port object */
    if (!DebugObject)
    {
        /* Fail */
        Status = STATUS_PORT_NOT_SET;
    }
    else
    {
        /* Acquire the debug object mutex */
        ExAcquireFastMutex(&DebugObject->Mutex);

        /* Check if a debugger is active */
        if (!DebugObject->DebuggerInactive)
        {
            /* Add the event into the object's list */
            DBGKTRACE(DBGK_MESSAGE_DEBUG, "Inserting: %p %d\n",
                      DebugEvent, Message->ApiNumber);
            InsertTailList(&DebugObject->EventList, &DebugEvent->EventList);

            /* Check if we have to signal it */
            if (!NewEvent)
            {
                /* Signal it */
                KeSetEvent(&DebugObject->EventsPresent,
                           IO_NO_INCREMENT,
                           FALSE);
            }

            /* Set success */
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* No debugger */
            Status = STATUS_DEBUGGER_INACTIVE;
        }

        /* Release the object lock */
        ExReleaseFastMutex(&DebugObject->Mutex);
    }

    /* Check if we had acquired the port lock */
    if (!NewEvent)
    {
        /* Release it */
        ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

        /* Check if we got here through success */
        if (NT_SUCCESS(Status))
        {
            /* Wait on the continue event */
            KeWaitForSingleObject(&DebugEvent->ContinueEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            /* Copy API Message back */
            *Message = DebugEvent->ApiMsg;

            /* Set return status */
            Status = DebugEvent->Status;
        }
    }
    else
    {
        /* Check if we failed */
        if (!NT_SUCCESS(Status))
        {
            /* Dereference the process and thread */
            ObDereferenceObject(Thread);
            ObDereferenceObject(Process);

            /* Free the debug event */
            ExFreePoolWithTag(DebugEvent, TAG_DEBUG_EVENT);
        }
    }

    /* Return status */
    DBGKTRACE(DBGK_MESSAGE_DEBUG, "Status: %lx\n", Status);
    return Status;
}

NTSTATUS
NTAPI
DbgkpSendApiMessageLpc(IN OUT PDBGKM_MSG Message,
                       IN PVOID Port,
                       IN BOOLEAN SuspendProcess)
{
    NTSTATUS Status;
    UCHAR Buffer[PORT_MAXIMUM_MESSAGE_LENGTH];
    BOOLEAN Suspended = FALSE;
    PAGED_CODE();

    /* Suspend process if required */
    if (SuspendProcess) Suspended = DbgkpSuspendProcess();

    /* Set return status */
    Message->ReturnedStatus = STATUS_PENDING;

    /* Set create process reported state */
    PspSetProcessFlag(PsGetCurrentProcess(), PSF_CREATE_REPORTED_BIT);

    /* Send the LPC command */
    Status = LpcRequestWaitReplyPort(Port,
                                     (PPORT_MESSAGE)Message,
                                     (PPORT_MESSAGE)&Buffer[0]);

    /* Flush the instruction cache */
    ZwFlushInstructionCache(NtCurrentProcess(), NULL, 0);

    /* Copy the buffer back */
    if (NT_SUCCESS(Status)) RtlCopyMemory(Message, Buffer, sizeof(DBGKM_MSG));

    /* Resume the process if it was suspended */
    if (Suspended) DbgkpResumeProcess();
    return Status;
}

NTSTATUS
NTAPI
DbgkpSendApiMessage(IN OUT PDBGKM_MSG ApiMsg,
                    IN BOOLEAN SuspendProcess)
{
    NTSTATUS Status;
    BOOLEAN Suspended = FALSE;
    PAGED_CODE();
    DBGKTRACE(DBGK_MESSAGE_DEBUG, "ApiMsg: %p SuspendProcess: %lx\n", ApiMsg, SuspendProcess);

    /* Suspend process if required */
    if (SuspendProcess) Suspended = DbgkpSuspendProcess();

    /* Set return status */
    ApiMsg->ReturnedStatus = STATUS_PENDING;

    /* Set create process reported state */
    PspSetProcessFlag(PsGetCurrentProcess(), PSF_CREATE_REPORTED_BIT);

    /* Send the LPC command */
    Status = DbgkpQueueMessage(PsGetCurrentProcess(),
                               PsGetCurrentThread(),
                               ApiMsg,
                               0,
                               NULL);

    /* Flush the instruction cache */
    ZwFlushInstructionCache(NtCurrentProcess(), NULL, 0);

    /* Resume the process if it was suspended */
    if (Suspended) DbgkpResumeProcess();
    return Status;
}

VOID
NTAPI
DbgkCopyProcessDebugPort(IN PEPROCESS Process,
                         IN PEPROCESS Parent)
{
    PDEBUG_OBJECT DebugObject;
    PAGED_CODE();
    DBGKTRACE(DBGK_PROCESS_DEBUG, "Process: %p Parent: %p\n", Process, Parent);

    /* Clear this process's port */
    Process->DebugPort = NULL;

    /* Check if the parent has one */
    if (!Parent->DebugPort) return;

    /* It does, acquire the mutex */
    ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

    /* Make sure it still has one, and that we should inherit */
    DebugObject = Parent->DebugPort;
    if ((DebugObject) && !(Process->NoDebugInherit))
    {
        /* Acquire the debug object's lock */
        ExAcquireFastMutex(&DebugObject->Mutex);

        /* Make sure the debugger is active */
        if (!DebugObject->DebuggerInactive)
        {
            /* Reference the object and set it */
            ObReferenceObject(DebugObject);
            Process->DebugPort = DebugObject;
        }

        /* Release the debug object */
        ExReleaseFastMutex(&DebugObject->Mutex);
    }

    /* Release the port mutex */
    ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);
}

BOOLEAN
NTAPI
DbgkForwardException(IN PEXCEPTION_RECORD ExceptionRecord,
                     IN BOOLEAN DebugPort,
                     IN BOOLEAN SecondChance)
{
    DBGKM_MSG ApiMessage;
    PDBGKM_EXCEPTION DbgKmException = &ApiMessage.Exception;
    NTSTATUS Status;
    PEPROCESS Process = PsGetCurrentProcess();
    PVOID Port;
    BOOLEAN UseLpc = FALSE;
    PAGED_CODE();
    DBGKTRACE(DBGK_EXCEPTION_DEBUG,
              "ExceptionRecord: %p Port: %u\n", ExceptionRecord, DebugPort);

    /* Setup the API Message */
    ApiMessage.h.u1.Length = sizeof(DBGKM_MSG) << 16 |
                             (8 + sizeof(DBGKM_EXCEPTION));
    ApiMessage.h.u2.ZeroInit = 0;
    ApiMessage.h.u2.s2.Type = LPC_DEBUG_EVENT;
    ApiMessage.ApiNumber = DbgKmExceptionApi;

    /* Check if this is to be sent on the debug port */
    if (DebugPort)
    {
        /* Use the debug port, unless the thread is being hidden */
        Port = PsGetCurrentThread()->HideFromDebugger ?
               NULL : Process->DebugPort;
    }
    else
    {
        /* Otherwise, use the exception port */
        Port = Process->ExceptionPort;
        ApiMessage.h.u2.ZeroInit = 0;
        ApiMessage.h.u2.s2.Type = LPC_EXCEPTION;
        UseLpc = TRUE;
    }

    /* Break out if there's no port */
    if (!Port) return FALSE;

    /* Fill out the exception information */
    DbgKmException->ExceptionRecord = *ExceptionRecord;
    DbgKmException->FirstChance = !SecondChance;

    /* Check if we should use LPC */
    if (UseLpc)
    {
        /* Send the message on the LPC Port */
        Status = DbgkpSendApiMessageLpc(&ApiMessage, Port, DebugPort);
    }
    else
    {
        /* Use native debug object */
        Status = DbgkpSendApiMessage(&ApiMessage, DebugPort);
    }

    /* Check if we failed, and for a debug port, also check the return status */
    if (!(NT_SUCCESS(Status)) ||
        ((DebugPort) &&
         (!(NT_SUCCESS(ApiMessage.ReturnedStatus)) ||
           (ApiMessage.ReturnedStatus == DBG_EXCEPTION_NOT_HANDLED))))
    {
        /* Fail */
        return FALSE;
    }

    /* Otherwise, we're ok */
    return TRUE;
}

VOID
NTAPI
DbgkpFreeDebugEvent(IN PDEBUG_EVENT DebugEvent)
{
    PHANDLE Handle = NULL;
    PAGED_CODE();
    DBGKTRACE(DBGK_OBJECT_DEBUG, "DebugEvent: %p\n", DebugEvent);

    /* Check if this event had a file handle */
    switch (DebugEvent->ApiMsg.ApiNumber)
    {
        /* Create process has a handle */
        case DbgKmCreateProcessApi:

            /* Get the pointer */
            Handle = &DebugEvent->ApiMsg.CreateProcess.FileHandle;
            break;

        /* As does DLL load */
        case DbgKmLoadDllApi:

            /* Get the pointer */
            Handle = &DebugEvent->ApiMsg.LoadDll.FileHandle;

        default:
            break;
    }

    /* Close the handle if it exsts */
    if ((Handle) && (*Handle)) ObCloseHandle(*Handle, KernelMode);

    /* Dereference process and thread and free the event */
    ObDereferenceObject(DebugEvent->Process);
    ObDereferenceObject(DebugEvent->Thread);
    ExFreePoolWithTag(DebugEvent, TAG_DEBUG_EVENT);
}

VOID
NTAPI
DbgkpWakeTarget(IN PDEBUG_EVENT DebugEvent)
{
    PETHREAD Thread = DebugEvent->Thread;
    PAGED_CODE();
    DBGKTRACE(DBGK_OBJECT_DEBUG, "DebugEvent: %p\n", DebugEvent);

    /* Check if we have to wake the thread */
    if (DebugEvent->Flags & DEBUG_EVENT_SUSPEND) PsResumeThread(Thread, NULL);

    /* Check if we had locked the thread */
    if (DebugEvent->Flags & DEBUG_EVENT_RELEASE)
    {
        /* Unlock it */
        ExReleaseRundownProtection(&Thread->RundownProtect);
    }

    /* Check if we have to wake up the event */
    if (DebugEvent->Flags & DEBUG_EVENT_NOWAIT)
    {
        /* Otherwise, free the debug event */
        DbgkpFreeDebugEvent(DebugEvent);
    }
    else
    {
        /* Signal the continue event */
        KeSetEvent(&DebugEvent->ContinueEvent, IO_NO_INCREMENT, FALSE);
    }
}

NTSTATUS
NTAPI
DbgkpPostFakeModuleMessages(IN PEPROCESS Process,
                            IN PETHREAD Thread,
                            IN PDEBUG_OBJECT DebugObject)
{
    PPEB Peb = Process->Peb;
    PPEB_LDR_DATA LdrData;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY ListHead, NextEntry;
    DBGKM_MSG ApiMessage;
    PDBGKM_LOAD_DLL LoadDll = &ApiMessage.LoadDll;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeader;
    UNICODE_STRING ModuleName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    UNICODE_STRING FullDllName;
    PAGED_CODE();
    DBGKTRACE(DBGK_PROCESS_DEBUG, "Process: %p Thread: %p DebugObject: %p\n",
              Process, Thread, DebugObject);

    /* Quit if there's no PEB */
    if (!Peb) return STATUS_SUCCESS;

    /* Accessing user memory, need SEH */
    _SEH2_TRY
    {
        /* Get the Loader Data List */
        ProbeForRead(Peb, sizeof(*Peb), 1);
        LdrData = Peb->Ldr;
        ProbeForRead(LdrData, sizeof(*LdrData), 1);
        ListHead = &LdrData->InLoadOrderModuleList;
        ProbeForRead(ListHead, sizeof(*ListHead), 1);
        NextEntry = ListHead->Flink;

        /* Loop the modules */
        i = 0;
        while ((NextEntry != ListHead) && (i < 500))
        {
            ProbeForRead(NextEntry, sizeof(*NextEntry), 1);
            /* Skip the first entry */
            if (!i)
            {
                /* Go to the next module */
                NextEntry = NextEntry->Flink;
                i++;
                continue;
            }

            /* Get the entry */
            LdrEntry = CONTAINING_RECORD(NextEntry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks);
            ProbeForRead(LdrEntry, sizeof(*LdrEntry), 1);

            /* Setup the API Message */
            RtlZeroMemory(&ApiMessage, sizeof(DBGKM_MSG));
            ApiMessage.ApiNumber = DbgKmLoadDllApi;

            /* Set base and clear the name */
            LoadDll->BaseOfDll = LdrEntry->DllBase;
            LoadDll->NamePointer = NULL;

            /* Get the NT Headers */
            NtHeader = RtlImageNtHeader(LoadDll->BaseOfDll);
            if (NtHeader)
            {
                /* Save debug data */
                LoadDll->DebugInfoFileOffset = NtHeader->FileHeader.
                                               PointerToSymbolTable;
                LoadDll->DebugInfoSize = NtHeader->FileHeader.NumberOfSymbols;
            }

            /* Trace */
            FullDllName = LdrEntry->FullDllName;
            ProbeForRead(FullDllName.Buffer, FullDllName.MaximumLength, 1);
            DBGKTRACE(DBGK_PROCESS_DEBUG, "Name: %wZ. Base: %p\n",
                      &FullDllName, LdrEntry->DllBase);

            /* Get the name of the DLL */
            Status = MmGetFileNameForAddress(NtHeader, &ModuleName);
            if (NT_SUCCESS(Status))
            {
                /* Setup the object attributes */
                InitializeObjectAttributes(&ObjectAttributes,
                                           &ModuleName,
                                           OBJ_FORCE_ACCESS_CHECK |
                                           OBJ_KERNEL_HANDLE |
                                           OBJ_CASE_INSENSITIVE,
                                           NULL,
                                           NULL);

                /* Open the file to get a handle to it */
                Status = ZwOpenFile(&LoadDll->FileHandle,
                                    GENERIC_READ | SYNCHRONIZE,
                                    &ObjectAttributes,
                                    &IoStatusBlock,
                                    FILE_SHARE_READ |
                                    FILE_SHARE_WRITE |
                                    FILE_SHARE_DELETE,
                                    FILE_SYNCHRONOUS_IO_NONALERT);
                if (!NT_SUCCESS(Status)) LoadDll->FileHandle = NULL;

                /* Free the name now */
                RtlFreeUnicodeString(&ModuleName);
            }

            /* Send the fake module load message */
            Status = DbgkpQueueMessage(Process,
                                       Thread,
                                       &ApiMessage,
                                       DEBUG_EVENT_NOWAIT,
                                       DebugObject);
            if (!NT_SUCCESS(Status))
            {
                /* Message send failed, close the file handle if we had one */
                if (LoadDll->FileHandle) ObCloseHandle(LoadDll->FileHandle,
                                                       KernelMode);
            }

            /* Go to the next module */
            NextEntry = NextEntry->Flink;
            i++;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        NOTHING;
    }
    _SEH2_END;

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DbgkpPostFakeThreadMessages(IN PEPROCESS Process,
                            IN PDEBUG_OBJECT DebugObject,
                            IN PETHREAD StartThread,
                            OUT PETHREAD *FirstThread,
                            OUT PETHREAD *LastThread)
{
    PETHREAD pFirstThread = NULL, ThisThread, OldThread = NULL, pLastThread;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN IsFirstThread;
    ULONG Flags;
    DBGKM_MSG ApiMessage;
    PDBGKM_CREATE_THREAD CreateThread = &ApiMessage.CreateThread;
    PDBGKM_CREATE_PROCESS CreateProcess = &ApiMessage.CreateProcess;
    BOOLEAN First;
    PIMAGE_NT_HEADERS NtHeader;
    PAGED_CODE();
    DBGKTRACE(DBGK_THREAD_DEBUG, "Process: %p StartThread: %p Object: %p\n",
              Process, StartThread, DebugObject);

    /* Check if we have a start thread */
    if (StartThread)
    {
        /* Then the one we'll find won't be the first one */
        IsFirstThread = FALSE;
        pFirstThread = StartThread;
        ThisThread = StartThread;

        /* Reference it */
        ObReferenceObject(StartThread);
    }
    else
    {
        /* Get the first thread ourselves */
        ThisThread = PsGetNextProcessThread(Process, NULL);
        IsFirstThread = TRUE;
    }

    /* Start thread loop */
    do
    {
        /* Dereference the previous thread if we had one */
        if (OldThread) ObDereferenceObject(OldThread);

        /* Set this as the last thread and lock it */
        pLastThread = ThisThread;
        ObReferenceObject(ThisThread);
        if (ExAcquireRundownProtection(&ThisThread->RundownProtect))
        {
            /* Acquire worked, set flags */
            Flags = DEBUG_EVENT_RELEASE | DEBUG_EVENT_NOWAIT;

            /* Check if this is a user thread */
            if (!ThisThread->SystemThread)
            {
                /* Suspend it */
                if (NT_SUCCESS(PsSuspendThread(ThisThread, NULL)))
                {
                    /* Remember this */
                    Flags |= DEBUG_EVENT_SUSPEND;
                }
            }
        }
        else
        {
            /* Couldn't acquire rundown */
            Flags = DEBUG_EVENT_PROTECT_FAILED | DEBUG_EVENT_NOWAIT;
        }

        /* Clear the API Message */
        RtlZeroMemory(&ApiMessage, sizeof(ApiMessage));

        /* Check if this is the first thread */
        if ((IsFirstThread) &&
            !(Flags & DEBUG_EVENT_PROTECT_FAILED) &&
            !(ThisThread->SystemThread) &&
            (ThisThread->GrantedAccess))
        {
            /* It is, save the flag */
            First = TRUE;
        }
        else
        {
            /* It isn't, save the flag */
            First = FALSE;
        }

        /* Check if this is the first */
        if (First)
        {
            /* So we'll start with the create process message */
            ApiMessage.ApiNumber = DbgKmCreateProcessApi;

            /* Get the file handle */
            if (Process->SectionObject)
            {
                /* Use the section object */
                CreateProcess->FileHandle =
                    DbgkpSectionToFileHandle(Process->SectionObject);
            }
            else
            {
                /* Don't return any handle */
                CreateProcess->FileHandle = NULL;
            }

            /* Set the base address */
            CreateProcess->BaseOfImage = Process->SectionBaseAddress;

            /* Get the NT Header */
            NtHeader = RtlImageNtHeader(Process->SectionBaseAddress);
            if (NtHeader)
            {
                /* Fill out data from the header */
                CreateProcess->DebugInfoFileOffset = NtHeader->FileHeader.
                                                     PointerToSymbolTable;
                CreateProcess->DebugInfoSize = NtHeader->FileHeader.
                                               NumberOfSymbols;
            }
        }
        else
        {
            /* Otherwise it's a thread message */
            ApiMessage.ApiNumber = DbgKmCreateThreadApi;
            CreateThread->StartAddress = ThisThread->StartAddress;
        }

        /* Trace */
        DBGKTRACE(DBGK_THREAD_DEBUG, "Thread: %p. First: %lx, OldThread: %p\n",
                  ThisThread, First, OldThread);
        DBGKTRACE(DBGK_THREAD_DEBUG, "Start Address: %p\n",
                  ThisThread->StartAddress);

        /* Queue the message */
        Status = DbgkpQueueMessage(Process,
                                   ThisThread,
                                   &ApiMessage,
                                   Flags,
                                   DebugObject);
        if (!NT_SUCCESS(Status))
        {
            /* Resume the thread if it was suspended */
            if (Flags & DEBUG_EVENT_SUSPEND) PsResumeThread(ThisThread, NULL);

            /* Check if we acquired rundown */
            if (Flags & DEBUG_EVENT_RELEASE)
            {
                /* Release it */
                ExReleaseRundownProtection(&ThisThread->RundownProtect);
            }

            /* If this was a process create, check if we got a handle */
            if ((ApiMessage.ApiNumber == DbgKmCreateProcessApi) &&
                 (CreateProcess->FileHandle))
            {
                /* Close it */
                ObCloseHandle(CreateProcess->FileHandle, KernelMode);
            }

            /* Release our reference and break out */
            ObDereferenceObject(ThisThread);
            break;
        }

        /* Check if this was the first message */
        if (First)
        {
            /* It isn't the first thread anymore */
            IsFirstThread = FALSE;

            /* Reference this thread and set it as first */
            ObReferenceObject(ThisThread);
            pFirstThread = ThisThread;
        }

        /* Get the next thread */
        ThisThread = PsGetNextProcessThread(Process, ThisThread);
        OldThread = pLastThread;
    } while (ThisThread);

    /* Check the API status */
    if (!NT_SUCCESS(Status))
    {
        /* Dereference and fail */
        if (pFirstThread) ObDereferenceObject(pFirstThread);
        ObDereferenceObject(pLastThread);
        return Status;
    }

    /* Make sure we have a first thread */
    if (!pFirstThread) return STATUS_UNSUCCESSFUL;

    /* Return thread pointers */
    *FirstThread = pFirstThread;
    *LastThread = pLastThread;
    return Status;
}

NTSTATUS
NTAPI
DbgkpPostFakeProcessCreateMessages(IN PEPROCESS Process,
                                   IN PDEBUG_OBJECT DebugObject,
                                   OUT PETHREAD *LastThread)
{
    KAPC_STATE ApcState;
    PETHREAD FirstThread, FinalThread;
    PETHREAD ReturnThread = NULL;
    NTSTATUS Status;
    PAGED_CODE();
    DBGKTRACE(DBGK_PROCESS_DEBUG, "Process: %p DebugObject: %p\n",
              Process, DebugObject);

    /* Attach to the process */
    KeStackAttachProcess(&Process->Pcb, &ApcState);

    /* Post the fake thread messages */
    Status = DbgkpPostFakeThreadMessages(Process,
                                         DebugObject,
                                         NULL,
                                         &FirstThread,
                                         &FinalThread);
    if (NT_SUCCESS(Status))
    {
        /* Send the fake module messages too */
        Status = DbgkpPostFakeModuleMessages(Process,
                                             FirstThread,
                                             DebugObject);
        if (!NT_SUCCESS(Status))
        {
            /* We failed, dereference the final thread */
            ObDereferenceObject(FinalThread);
        }
        else
        {
            /* Set the final thread */
            ReturnThread = FinalThread;
        }

        /* Dereference the first thread */
        ObDereferenceObject(FirstThread);
    }

    /* Detach from the process */
    KeUnstackDetachProcess(&ApcState);

    /* Return the last thread */
    *LastThread = ReturnThread;
    return Status;
}

VOID
NTAPI
DbgkpConvertKernelToUserStateChange(IN PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
                                    IN PDEBUG_EVENT DebugEvent)
{
    DBGKTRACE(DBGK_OBJECT_DEBUG, "DebugEvent: %p\n", DebugEvent);

    /* Start by copying the client ID */
    WaitStateChange->AppClientId = DebugEvent->ClientId;

    /* Now check which kind of event this was */
    switch (DebugEvent->ApiMsg.ApiNumber)
    {
        /* New process */
        case DbgKmCreateProcessApi:

            /* Set the right native code */
            WaitStateChange->NewState = DbgCreateProcessStateChange;

            /* Copy the information */
            WaitStateChange->StateInfo.CreateProcessInfo.NewProcess =
                DebugEvent->ApiMsg.CreateProcess;

            /* Clear the file handle for us */
            DebugEvent->ApiMsg.CreateProcess.FileHandle = NULL;
            break;

        /* New thread */
        case DbgKmCreateThreadApi:

            /* Set the right native code */
            WaitStateChange->NewState = DbgCreateThreadStateChange;

            /* Copy information */
            WaitStateChange->StateInfo.CreateThread.NewThread.StartAddress =
                DebugEvent->ApiMsg.CreateThread.StartAddress;
            WaitStateChange->StateInfo.CreateThread.NewThread.SubSystemKey =
                DebugEvent->ApiMsg.CreateThread.SubSystemKey;
            break;

        /* Exception (or breakpoint/step) */
        case DbgKmExceptionApi:

            /* Look at the exception code */
            if ((NTSTATUS)DebugEvent->ApiMsg.Exception.ExceptionRecord.ExceptionCode ==
                STATUS_BREAKPOINT)
            {
                /* Update this as a breakpoint exception */
                WaitStateChange->NewState = DbgBreakpointStateChange;
            }
            else if ((NTSTATUS)DebugEvent->ApiMsg.Exception.ExceptionRecord.ExceptionCode ==
                     STATUS_SINGLE_STEP)
            {
                /* Update this as a single step exception */
                WaitStateChange->NewState = DbgSingleStepStateChange;
            }
            else
            {
                /* Otherwise, set default exception */
                WaitStateChange->NewState = DbgExceptionStateChange;
            }

            /* Copy the exception record */
            WaitStateChange->StateInfo.Exception.ExceptionRecord =
                DebugEvent->ApiMsg.Exception.ExceptionRecord;
            /* Copy FirstChance flag */
            WaitStateChange->StateInfo.Exception.FirstChance =
                DebugEvent->ApiMsg.Exception.FirstChance;
            break;

        /* Process exited */
        case DbgKmExitProcessApi:

            /* Set the right native code and copy the exit code */
            WaitStateChange->NewState = DbgExitProcessStateChange;
            WaitStateChange->StateInfo.ExitProcess.ExitStatus =
                DebugEvent->ApiMsg.ExitProcess.ExitStatus;
            break;

        /* Thread exited */
        case DbgKmExitThreadApi:

            /* Set the right native code */
            WaitStateChange->NewState = DbgExitThreadStateChange;
            WaitStateChange->StateInfo.ExitThread.ExitStatus =
                DebugEvent->ApiMsg.ExitThread.ExitStatus;
            break;

        /* DLL Load */
        case DbgKmLoadDllApi:

            /* Set the native code */
            WaitStateChange->NewState = DbgLoadDllStateChange;

            /* Copy the data */
            WaitStateChange->StateInfo.LoadDll = DebugEvent->ApiMsg.LoadDll;

            /* Clear the file handle for us */
            DebugEvent->ApiMsg.LoadDll.FileHandle = NULL;
            break;

        /* DLL Unload */
        case DbgKmUnloadDllApi:

            /* Set the native code and copy the address */
            WaitStateChange->NewState = DbgUnloadDllStateChange;
            WaitStateChange->StateInfo.UnloadDll.BaseAddress =
                DebugEvent->ApiMsg.UnloadDll.BaseAddress;
            break;

        default:

            /* Shouldn't happen */
            ASSERT(FALSE);
    }
}

VOID
NTAPI
DbgkpMarkProcessPeb(IN PEPROCESS Process)
{
    KAPC_STATE ApcState;
    PAGED_CODE();
    DBGKTRACE(DBGK_PROCESS_DEBUG, "Process: %p\n", Process);

    /* Acquire process rundown */
    if (!ExAcquireRundownProtection(&Process->RundownProtect)) return;

    /* Make sure we have a PEB */
    if (Process->Peb)
    {
        /* Attach to the process */
        KeStackAttachProcess(&Process->Pcb, &ApcState);

        /* Acquire the debug port mutex */
        ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

        /* Set the IsBeingDebugged member of the PEB */
        Process->Peb->BeingDebugged = (Process->DebugPort) ? TRUE: FALSE;

        /* Release lock */
        ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

        /* Detach from the process */
        KeUnstackDetachProcess(&ApcState);
    }

    /* Release rundown protection */
    ExReleaseRundownProtection(&Process->RundownProtect);
}

VOID
NTAPI
DbgkpOpenHandles(IN PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
                 IN PEPROCESS Process,
                 IN PETHREAD Thread)
{
    NTSTATUS Status;
    HANDLE Handle;
    PHANDLE DupHandle;
    PAGED_CODE();
    DBGKTRACE(DBGK_OBJECT_DEBUG, "Process: %p Thread: %p State: %lx\n",
              Process, Thread, WaitStateChange->NewState);

    /* Check which state this is */
    switch (WaitStateChange->NewState)
    {
        /* New thread */
        case DbgCreateThreadStateChange:

            /* Get handle to thread */
            Status = ObOpenObjectByPointer(Thread,
                                           0,
                                           NULL,
                                           THREAD_ALL_ACCESS,
                                           PsThreadType,
                                           KernelMode,
                                           &Handle);
            if (NT_SUCCESS(Status))
            {
                /* Save the thread handle */
                WaitStateChange->
                    StateInfo.CreateThread.HandleToThread = Handle;
            }
            return;

        /* New process */
        case DbgCreateProcessStateChange:

            /* Get handle to thread */
            Status = ObOpenObjectByPointer(Thread,
                                           0,
                                           NULL,
                                           THREAD_ALL_ACCESS,
                                           PsThreadType,
                                           KernelMode,
                                           &Handle);
            if (NT_SUCCESS(Status))
            {
                /* Save the thread handle */
                WaitStateChange->
                    StateInfo.CreateProcessInfo.HandleToThread = Handle;
            }

            /* Get handle to process */
            Status = ObOpenObjectByPointer(Process,
                                           0,
                                           NULL,
                                           PROCESS_ALL_ACCESS,
                                           PsProcessType,
                                           KernelMode,
                                           &Handle);
            if (NT_SUCCESS(Status))
            {
                /* Save the process handle */
                WaitStateChange->
                    StateInfo.CreateProcessInfo.HandleToProcess = Handle;
            }

            /* Fall through to duplicate file handle */
            DupHandle = &WaitStateChange->
                            StateInfo.CreateProcessInfo.NewProcess.FileHandle;
            break;

        /* DLL Load */
        case DbgLoadDllStateChange:

            /* Fall through to duplicate file handle */
            DupHandle = &WaitStateChange->StateInfo.LoadDll.FileHandle;
            break;

        /* Anything else has no handles */
        default:
            return;
    }

    /* If we got here, then we have to duplicate a handle, possibly */
    Handle = *DupHandle;
    if (Handle)
    {
        /* Duplicate it */
        Status = ObDuplicateObject(PsGetCurrentProcess(),
                                   Handle,
                                   PsGetCurrentProcess(),
                                   DupHandle,
                                   0,
                                   0,
                                   DUPLICATE_SAME_ACCESS,
                                   KernelMode);
        if (!NT_SUCCESS(Status)) *DupHandle = NULL;

        /* Close the original handle */
        ObCloseHandle(Handle, KernelMode);
    }
}

VOID
NTAPI
DbgkpDeleteObject(IN PVOID DebugObject)
{
    PAGED_CODE();

    /* Sanity check */
    ASSERT(IsListEmpty(&((PDEBUG_OBJECT)DebugObject)->EventList));
}

VOID
NTAPI
DbgkpCloseObject(IN PEPROCESS OwnerProcess OPTIONAL,
                 IN PVOID ObjectBody,
                 IN ACCESS_MASK GrantedAccess,
                 IN ULONG HandleCount,
                 IN ULONG SystemHandleCount)
{
    PDEBUG_OBJECT DebugObject = ObjectBody;
    PEPROCESS Process = NULL;
    BOOLEAN DebugPortCleared = FALSE;
    PLIST_ENTRY DebugEventList;
    PDEBUG_EVENT DebugEvent;
    PAGED_CODE();
    DBGKTRACE(DBGK_OBJECT_DEBUG, "OwnerProcess: %p DebugObject: %p\n",
              OwnerProcess, DebugObject);

    /* If this isn't the last handle, do nothing */
    if (SystemHandleCount > 1) return;

    /* Otherwise, lock the debug object */
    ExAcquireFastMutex(&DebugObject->Mutex);

    /* Set it as inactive */
    DebugObject->DebuggerInactive = TRUE;

    /* Remove it from the debug event list */
    DebugEventList = DebugObject->EventList.Flink;
    InitializeListHead(&DebugObject->EventList);

    /* Release the lock */
    ExReleaseFastMutex(&DebugObject->Mutex);

    /* Signal the wait event */
    KeSetEvent(&DebugObject->EventsPresent, IO_NO_INCREMENT, FALSE);

    /* Start looping each process */
    while ((Process = PsGetNextProcess(Process)))
    {
        /* Check if the process has us as their debug port */
        if (Process->DebugPort == DebugObject)
        {
            /* Acquire the process debug port lock */
            ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

            /* Check if it's still us */
            if (Process->DebugPort == DebugObject)
            {
                /* Clear it and remember */
                Process->DebugPort = NULL;
                DebugPortCleared = TRUE;
            }

            /* Release the port lock */
            ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

            /* Check if we cleared the debug port */
            if (DebugPortCleared)
            {
                /* Mark this in the PEB */
                DbgkpMarkProcessPeb(Process);

                /* Check if we terminate on exit */
                if (DebugObject->KillProcessOnExit)
                {
                    /* Terminate the process */
                    PsTerminateProcess(Process, STATUS_DEBUGGER_INACTIVE);
                }

                /* Dereference the debug object */
                ObDereferenceObject(DebugObject);
            }
        }
    }

    /* Loop debug events */
    while (DebugEventList != &DebugObject->EventList)
    {
        /* Get the debug event */
        DebugEvent = CONTAINING_RECORD(DebugEventList, DEBUG_EVENT, EventList);

        /* Go to the next entry */
        DebugEventList = DebugEventList->Flink;

        /* Wake it up */
        DebugEvent->Status = STATUS_DEBUGGER_INACTIVE;
        DbgkpWakeTarget(DebugEvent);
    }
}

NTSTATUS
NTAPI
DbgkpSetProcessDebugObject(IN PEPROCESS Process,
                           IN PDEBUG_OBJECT DebugObject,
                           IN NTSTATUS MsgStatus,
                           IN PETHREAD LastThread)
{
    NTSTATUS Status;
    LIST_ENTRY TempList;
    BOOLEAN GlobalHeld = FALSE, DoSetEvent = TRUE;
    PETHREAD ThisThread, FirstThread;
    PLIST_ENTRY NextEntry;
    PDEBUG_EVENT DebugEvent;
    PETHREAD EventThread;
    PAGED_CODE();
    DBGKTRACE(DBGK_PROCESS_DEBUG, "Process: %p DebugObject: %p\n",
              Process, DebugObject);

    /* Initialize the temporary list */
    InitializeListHead(&TempList);

    /* Check if we have a success message */
    if (NT_SUCCESS(MsgStatus))
    {
        /* Then default to STATUS_SUCCESS */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* No last thread, and set the failure code */
        LastThread = NULL;
        Status = MsgStatus;
    }

    /* Now check what status we have here */
    if (NT_SUCCESS(Status))
    {
        /* Acquire the global lock */
ThreadScan:
        GlobalHeld = TRUE;
        ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

        /* Check if we already have a port */
        if (Process->DebugPort)
        {
            /* Set failure */
            Status = STATUS_PORT_ALREADY_SET;
        }
        else
        {
            /* Otherwise, set the port and reference the thread */
            Process->DebugPort = DebugObject;
            ObReferenceObject(LastThread);

            /* Get the next thread */
            ThisThread  = PsGetNextProcessThread(Process, LastThread);
            if (ThisThread)
            {
                /* Clear the debug port and release the lock */
                Process->DebugPort = NULL;
                ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);
                GlobalHeld = FALSE;

                /* Dereference the thread */
                ObDereferenceObject(LastThread);

                /* Post fake messages */
                Status = DbgkpPostFakeThreadMessages(Process,
                                                     DebugObject,
                                                     ThisThread,
                                                     &FirstThread,
                                                     &LastThread);
                if (!NT_SUCCESS(Status))
                {
                    /* Clear the last thread */
                    LastThread = NULL;
                }
                else
                {
                    /* Dereference the first thread and re-acquire the lock */
                    ObDereferenceObject(FirstThread);
                    goto ThreadScan;
                }
            }
        }
    }

    /* Acquire the debug object's lock */
    ExAcquireFastMutex(&DebugObject->Mutex);

    /* Check our status here */
    if (NT_SUCCESS(Status))
    {
        /* Check if we're disconnected */
        if (DebugObject->DebuggerInactive)
        {
            /* Set status */
            Process->DebugPort = NULL;
            Status = STATUS_DEBUGGER_INACTIVE;
        }
        else
        {
            /* Set the process flags */
            PspSetProcessFlag(Process,
                              PSF_NO_DEBUG_INHERIT_BIT |
                              PSF_CREATE_REPORTED_BIT);

            /* Reference the debug object */
            ObReferenceObject(DebugObject);
        }
    }

    /* Loop the events list */
    NextEntry = DebugObject->EventList.Flink;
    while (NextEntry != &DebugObject->EventList)
    {
        /* Get the debug event and go to the next entry */
        DebugEvent = CONTAINING_RECORD(NextEntry, DEBUG_EVENT, EventList);
        NextEntry = NextEntry->Flink;
        DBGKTRACE(DBGK_PROCESS_DEBUG, "DebugEvent: %p Flags: %lx TH: %p/%p\n",
                  DebugEvent, DebugEvent->Flags,
                  DebugEvent->BackoutThread, PsGetCurrentThread());

        /* Check for if the debug event queue needs flushing */
        if ((DebugEvent->Flags & DEBUG_EVENT_INACTIVE) &&
            (DebugEvent->BackoutThread == PsGetCurrentThread()))
        {
            /* Get the event's thread */
            EventThread = DebugEvent->Thread;
            DBGKTRACE(DBGK_PROCESS_DEBUG, "EventThread: %p MsgStatus: %lx\n",
                      EventThread, MsgStatus);

            /* Check if the status is success */
            if ((MsgStatus == STATUS_SUCCESS) &&
                (EventThread->GrantedAccess) &&
                (!EventThread->SystemThread))
            {
                /* Check if we couldn't acquire rundown for it */
                if (DebugEvent->Flags & DEBUG_EVENT_PROTECT_FAILED)
                {
                    /* Set the skip termination flag */
                    PspSetCrossThreadFlag(EventThread, CT_SKIP_CREATION_MSG_BIT);

                    /* Insert it into the temp list */
                    RemoveEntryList(&DebugEvent->EventList);
                    InsertTailList(&TempList, &DebugEvent->EventList);
                }
                else
                {
                    /* Do we need to signal the event */
                    if (DoSetEvent)
                    {
                        /* Do it */
                        DebugEvent->Flags &= ~DEBUG_EVENT_INACTIVE;
                        KeSetEvent(&DebugObject->EventsPresent,
                                   IO_NO_INCREMENT,
                                   FALSE);
                        DoSetEvent = FALSE;
                    }

                    /* Clear the backout thread */
                    DebugEvent->BackoutThread = NULL;

                    /* Set skip flag */
                    PspSetCrossThreadFlag(EventThread, CT_SKIP_CREATION_MSG_BIT);
                }
            }
            else
            {
                /* Insert it into the temp list */
                RemoveEntryList(&DebugEvent->EventList);
                InsertTailList(&TempList, &DebugEvent->EventList);
            }

            /* Check if the lock is held */
            if (DebugEvent->Flags & DEBUG_EVENT_RELEASE)
            {
                /* Release it */
                DebugEvent->Flags &= ~DEBUG_EVENT_RELEASE;
                ExReleaseRundownProtection(&EventThread->RundownProtect);
            }
        }
    }

    /* Release the debug object */
    ExReleaseFastMutex(&DebugObject->Mutex);

    /* Release the global lock if acquired */
    if (GlobalHeld) ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

    /* Check if there's a thread to dereference */
    if (LastThread) ObDereferenceObject(LastThread);

    /* Loop our temporary list */
    while (!IsListEmpty(&TempList))
    {
        /* Remove the event */
        NextEntry = RemoveHeadList(&TempList);
        DebugEvent = CONTAINING_RECORD(NextEntry, DEBUG_EVENT, EventList);

        /* Wake it */
        DbgkpWakeTarget(DebugEvent);
    }

    /* Check if we got here through success and mark the PEB, then return */
    if (NT_SUCCESS(Status)) DbgkpMarkProcessPeb(Process);
    return Status;
}

NTSTATUS
NTAPI
DbgkClearProcessDebugObject(IN PEPROCESS Process,
                            IN PDEBUG_OBJECT SourceDebugObject OPTIONAL)
{
    PDEBUG_OBJECT DebugObject;
    PDEBUG_EVENT DebugEvent;
    LIST_ENTRY TempList;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();
    DBGKTRACE(DBGK_OBJECT_DEBUG, "Process: %p DebugObject: %p\n",
              Process, SourceDebugObject);

    /* Acquire the port lock */
    ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

    /* Get the Process Debug Object */
    DebugObject = Process->DebugPort;

    /*
     * Check if the process had an object and it matches,
     * or if the process had an object but none was specified
     * (in which we are called from NtTerminateProcess)
     */
    if ((DebugObject) &&
        ((DebugObject == SourceDebugObject) ||
         (SourceDebugObject == NULL)))
    {
        /* Clear the debug port */
        Process->DebugPort = NULL;

        /* Release the port lock and remove the PEB flag */
        ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);
        DbgkpMarkProcessPeb(Process);
    }
    else
    {
        /* Release the port lock and fail */
        ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);
        return STATUS_PORT_NOT_SET;
    }

    /* Initialize the temporary list */
    InitializeListHead(&TempList);

    /* Acquire the Object */
    ExAcquireFastMutex(&DebugObject->Mutex);

    /* Loop the events */
    NextEntry = DebugObject->EventList.Flink;
    while (NextEntry != &DebugObject->EventList)
    {
        /* Get the Event and go to the next entry */
        DebugEvent = CONTAINING_RECORD(NextEntry, DEBUG_EVENT, EventList);
        NextEntry = NextEntry->Flink;

        /* Check that it belongs to the specified process */
        if (DebugEvent->Process == Process)
        {
            /* Insert it into the temporary list */
            RemoveEntryList(&DebugEvent->EventList);
            InsertTailList(&TempList, &DebugEvent->EventList);
        }
    }

    /* Release the Object */
    ExReleaseFastMutex(&DebugObject->Mutex);

    /* Release the initial reference */
    ObDereferenceObject(DebugObject);

    /* Loop our temporary list */
    while (!IsListEmpty(&TempList))
    {
        /* Remove the event */
        NextEntry = RemoveHeadList(&TempList);
        DebugEvent = CONTAINING_RECORD(NextEntry, DEBUG_EVENT, EventList);

        /* Wake it up */
        DebugEvent->Status = STATUS_DEBUGGER_INACTIVE;
        DbgkpWakeTarget(DebugEvent);
    }

    /* Return Success */
    return STATUS_SUCCESS;
}

CODE_SEG("INIT")
VOID
NTAPI
DbgkInitialize(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    PAGED_CODE();

    /* Initialize the process debug port mutex */
    ExInitializeFastMutex(&DbgkpProcessDebugPortMutex);

    /* Create the Debug Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"DebugObject");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DEBUG_OBJECT);
    ObjectTypeInitializer.GenericMapping = DbgkDebugObjectMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = DEBUG_OBJECT_ALL_ACCESS;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    ObjectTypeInitializer.CloseProcedure = DbgkpCloseObject;
    ObjectTypeInitializer.DeleteProcedure = DbgkpDeleteObject;
    ObCreateObjectType(&Name,
                       &ObjectTypeInitializer,
                       NULL,
                       &DbgkDebugObjectType);
}

NTSTATUS
NTAPI
DbgkOpenProcessDebugPort(IN PEPROCESS Process,
                         IN KPROCESSOR_MODE PreviousMode,
                         OUT HANDLE *DebugHandle)
{
    PDEBUG_OBJECT DebugObject;
    NTSTATUS Status;
    PAGED_CODE();

    /* If there's no debug port, just exit */
    if (!Process->DebugPort) return STATUS_PORT_NOT_SET;

    /* Otherwise, acquire the lock while we grab the port */
    ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

    /* Grab it and reference it if it exists */
    DebugObject = Process->DebugPort;
    if (DebugObject) ObReferenceObject(DebugObject);

    /* Release the lock now */
    ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

    /* Bail out if it doesn't exist */
    if (!DebugObject) return STATUS_PORT_NOT_SET;

    /* Now get a handle to it */
    Status = ObOpenObjectByPointer(DebugObject,
                                   0,
                                   NULL,
                                   MAXIMUM_ALLOWED,
                                   DbgkDebugObjectType,
                                   PreviousMode,
                                   DebugHandle);
    if (!NT_SUCCESS(Status)) ObDereferenceObject(DebugObject);

    /* Return status */
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreateDebugObject(OUT PHANDLE DebugHandle,
                    IN ACCESS_MASK DesiredAccess,
                    IN POBJECT_ATTRIBUTES ObjectAttributes,
                    IN ULONG Flags)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PDEBUG_OBJECT DebugObject;
    HANDLE hDebug;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if we were called from user mode*/
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the handle */
            ProbeForWriteHandle(DebugHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        } _SEH2_END;
    }

    /* Check for invalid flags */
    if (Flags & ~DBGK_ALL_FLAGS) return STATUS_INVALID_PARAMETER;

    /* Create the Object */
    Status = ObCreateObject(PreviousMode,
                            DbgkDebugObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(DEBUG_OBJECT),
                            0,
                            0,
                            (PVOID*)&DebugObject);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the Debug Object's Fast Mutex */
        ExInitializeFastMutex(&DebugObject->Mutex);

        /* Initialize the State Event List */
        InitializeListHead(&DebugObject->EventList);

        /* Initialize the Debug Object's Wait Event */
        KeInitializeEvent(&DebugObject->EventsPresent,
                          NotificationEvent,
                          FALSE);

        /* Set the Flags */
        DebugObject->Flags = 0;
        if (Flags & DBGK_KILL_PROCESS_ON_EXIT)
        {
            DebugObject->KillProcessOnExit = TRUE;
        }

        /* Insert it */
        Status = ObInsertObject((PVOID)DebugObject,
                                 NULL,
                                 DesiredAccess,
                                 0,
                                 NULL,
                                 &hDebug);
        if (NT_SUCCESS(Status))
        {
            /* Enter SEH to protect the write */
            _SEH2_TRY
            {
                /* Return the handle */
                *DebugHandle = hDebug;
            }
            _SEH2_EXCEPT(ExSystemExceptionFilter())
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            } _SEH2_END;
        }
    }

    /* Return Status */
    DBGKTRACE(DBGK_OBJECT_DEBUG, "Handle: %p DebugObject: %p\n",
              hDebug, DebugObject);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtDebugContinue(IN HANDLE DebugHandle,
                IN PCLIENT_ID AppClientId,
                IN NTSTATUS ContinueStatus)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PDEBUG_OBJECT DebugObject;
    NTSTATUS Status;
    PDEBUG_EVENT DebugEvent = NULL, DebugEventToWake = NULL;
    PLIST_ENTRY ListHead, NextEntry;
    BOOLEAN NeedsWake = FALSE;
    CLIENT_ID ClientId;
    PAGED_CODE();
    DBGKTRACE(DBGK_OBJECT_DEBUG, "Handle: %p Status: %d\n",
              DebugHandle, ContinueStatus);

    /* Check if we were called from user mode*/
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the handle */
            ProbeForRead(AppClientId, sizeof(CLIENT_ID), sizeof(ULONG));
            ClientId = *AppClientId;
            AppClientId = &ClientId;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        } _SEH2_END;
    }

    /* Make sure that the status is valid */
    if ((ContinueStatus != DBG_CONTINUE) &&
        (ContinueStatus != DBG_EXCEPTION_HANDLED) &&
        (ContinueStatus != DBG_EXCEPTION_NOT_HANDLED) &&
        (ContinueStatus != DBG_TERMINATE_THREAD) &&
        (ContinueStatus != DBG_TERMINATE_PROCESS))
    {
        /* Invalid status */
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Get the debug object */
        Status = ObReferenceObjectByHandle(DebugHandle,
                                           DEBUG_OBJECT_WAIT_STATE_CHANGE,
                                           DbgkDebugObjectType,
                                           PreviousMode,
                                           (PVOID*)&DebugObject,
                                           NULL);
        if (NT_SUCCESS(Status))
        {
            /* Acquire the mutex */
            ExAcquireFastMutex(&DebugObject->Mutex);

            /* Loop the state list */
            ListHead = &DebugObject->EventList;
            NextEntry = ListHead->Flink;
            while (ListHead != NextEntry)
            {
                /* Get the current debug event */
                DebugEvent = CONTAINING_RECORD(NextEntry,
                                               DEBUG_EVENT,
                                               EventList);

                /* Compare process ID */
                if (DebugEvent->ClientId.UniqueProcess ==
                    AppClientId->UniqueProcess)
                {
                    /* Check if we already found a match */
                    if (NeedsWake)
                    {
                        /* Wake it up and break out */
                        DebugEvent->Flags &= ~DEBUG_EVENT_INACTIVE;
                        KeSetEvent(&DebugObject->EventsPresent,
                                   IO_NO_INCREMENT,
                                   FALSE);
                        break;
                    }

                    /* Compare thread ID and flag */
                    if ((DebugEvent->ClientId.UniqueThread ==
                        AppClientId->UniqueThread) && (DebugEvent->Flags & DEBUG_EVENT_READ))
                    {
                        /* Remove the event from the list */
                        RemoveEntryList(NextEntry);

                        /* Remember who to wake */
                        NeedsWake = TRUE;
                        DebugEventToWake = DebugEvent;
                    }
                }

                /* Go to the next entry */
                NextEntry = NextEntry->Flink;
            }

            /* Release the mutex */
            ExReleaseFastMutex(&DebugObject->Mutex);

            /* Dereference the object */
            ObDereferenceObject(DebugObject);

            /* Check if need a wait */
            if (NeedsWake)
            {
                /* Set the continue status */
                DebugEventToWake->ApiMsg.ReturnedStatus = ContinueStatus;
                DebugEventToWake->Status = STATUS_SUCCESS;

                /* Wake the target */
                DbgkpWakeTarget(DebugEventToWake);
            }
            else
            {
                /* Fail */
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtDebugActiveProcess(IN HANDLE ProcessHandle,
                     IN HANDLE DebugHandle)
{
    PEPROCESS Process;
    PDEBUG_OBJECT DebugObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PETHREAD LastThread;
    NTSTATUS Status;
    PAGED_CODE();
    DBGKTRACE(DBGK_PROCESS_DEBUG, "Process: %p Handle: %p\n",
              ProcessHandle, DebugHandle);

    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_SUSPEND_RESUME,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Don't allow debugging the current process or the system process */
    if ((Process == PsGetCurrentProcess()) ||
         (Process == PsInitialSystemProcess))
    {
        /* Dereference and fail */
        ObDereferenceObject(Process);
        return STATUS_ACCESS_DENIED;
    }

    /* Reference the debug object */
    Status = ObReferenceObjectByHandle(DebugHandle,
                                       DEBUG_OBJECT_ADD_REMOVE_PROCESS,
                                       DbgkDebugObjectType,
                                       PreviousMode,
                                       (PVOID*)&DebugObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Dereference the process and exit */
        ObDereferenceObject(Process);
        return Status;
    }

    /* Acquire process rundown protection */
    if (!ExAcquireRundownProtection(&Process->RundownProtect))
    {
        /* Dereference the process and debug object and exit */
        ObDereferenceObject(Process);
        ObDereferenceObject(DebugObject);
        return STATUS_PROCESS_IS_TERMINATING;
    }

    /* Send fake create messages for debuggers to have a consistent state */
    Status = DbgkpPostFakeProcessCreateMessages(Process,
                                                DebugObject,
                                                &LastThread);
    Status = DbgkpSetProcessDebugObject(Process,
                                        DebugObject,
                                        Status,
                                        LastThread);

    /* Release rundown protection */
    ExReleaseRundownProtection(&Process->RundownProtect);

    /* Dereference the process and debug object and return status */
    ObDereferenceObject(Process);
    ObDereferenceObject(DebugObject);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtRemoveProcessDebug(IN HANDLE ProcessHandle,
                     IN HANDLE DebugHandle)
{
    PEPROCESS Process;
    PDEBUG_OBJECT DebugObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    DBGKTRACE(DBGK_PROCESS_DEBUG, "Process: %p Handle: %p\n",
              ProcessHandle, DebugHandle);

    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_SUSPEND_RESUME,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Reference the debug object */
    Status = ObReferenceObjectByHandle(DebugHandle,
                                       DEBUG_OBJECT_ADD_REMOVE_PROCESS,
                                       DbgkDebugObjectType,
                                       PreviousMode,
                                       (PVOID*)&DebugObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Dereference the process and exit */
        ObDereferenceObject(Process);
        return Status;
    }

    /* Remove the debug object */
    Status = DbgkClearProcessDebugObject(Process, DebugObject);

    /* Dereference the process and debug object and return status */
    ObDereferenceObject(Process);
    ObDereferenceObject(DebugObject);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetInformationDebugObject(IN HANDLE DebugHandle,
                            IN DEBUGOBJECTINFOCLASS DebugObjectInformationClass,
                            IN PVOID DebugInformation,
                            IN ULONG DebugInformationLength,
                            OUT PULONG ReturnLength OPTIONAL)
{
    PDEBUG_OBJECT DebugObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PDEBUG_OBJECT_KILL_PROCESS_ON_EXIT_INFORMATION DebugInfo = DebugInformation;
    PAGED_CODE();

    /* Check buffers and parameters */
    Status = DefaultSetInfoBufferCheck(DebugObjectInformationClass,
                                       DbgkpDebugObjectInfoClass,
                                       sizeof(DbgkpDebugObjectInfoClass) /
                                       sizeof(DbgkpDebugObjectInfoClass[0]),
                                       DebugInformation,
                                       DebugInformationLength,
                                       PreviousMode);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if the caller wanted the return length */
    if (ReturnLength)
    {
        /* Enter SEH for probe */
        _SEH2_TRY
        {
            /* Return required length to user-mode */
            ProbeForWriteUlong(ReturnLength);
            *ReturnLength = sizeof(*DebugInfo);
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Open the Object */
    Status = ObReferenceObjectByHandle(DebugHandle,
                                       DEBUG_OBJECT_WAIT_STATE_CHANGE,
                                       DbgkDebugObjectType,
                                       PreviousMode,
                                       (PVOID*)&DebugObject,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Acquire the object */
        ExAcquireFastMutex(&DebugObject->Mutex);

        /* Set the proper flag */
        if (DebugInfo->KillProcessOnExit)
        {
            /* Enable killing the process */
            DebugObject->KillProcessOnExit = TRUE;
        }
        else
        {
            /* Disable */
            DebugObject->KillProcessOnExit = FALSE;
        }

        /* Release the mutex */
        ExReleaseFastMutex(&DebugObject->Mutex);

        /* Release the Object */
        ObDereferenceObject(DebugObject);
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtWaitForDebugEvent(IN HANDLE DebugHandle,
                    IN BOOLEAN Alertable,
                    IN PLARGE_INTEGER Timeout OPTIONAL,
                    OUT PDBGUI_WAIT_STATE_CHANGE StateChange)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER LocalTimeOut;
    PEPROCESS Process;
    LARGE_INTEGER StartTime;
    PETHREAD Thread;
    BOOLEAN GotEvent;
    LARGE_INTEGER NewTime;
    PDEBUG_OBJECT DebugObject;
    DBGUI_WAIT_STATE_CHANGE WaitStateChange;
    NTSTATUS Status;
    PDEBUG_EVENT DebugEvent = NULL, DebugEvent2;
    PLIST_ENTRY ListHead, NextEntry, NextEntry2;
    PAGED_CODE();
    DBGKTRACE(DBGK_OBJECT_DEBUG, "Handle: %p\n", DebugHandle);

    /* Clear the initial wait state change structure and the timeout */
    RtlZeroMemory(&WaitStateChange, sizeof(WaitStateChange));
    LocalTimeOut.QuadPart = 0;

    /* Check if we were called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Protect probe in SEH */
        _SEH2_TRY
        {
            /* Check if we came with a timeout */
            if (Timeout)
            {
                /* Probe it */
                ProbeForReadLargeInteger(Timeout);

                /* Make a local copy */
                LocalTimeOut = *Timeout;
                Timeout = &LocalTimeOut;
            }

            /* Probe the state change structure */
            ProbeForWrite(StateChange, sizeof(*StateChange), sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Copy directly */
        if (Timeout) LocalTimeOut = *Timeout;
    }

    /* If we were passed a timeout, query the current time */
    if (Timeout) KeQuerySystemTime(&StartTime);

    /* Get the debug object */
    Status = ObReferenceObjectByHandle(DebugHandle,
                                       DEBUG_OBJECT_WAIT_STATE_CHANGE,
                                       DbgkDebugObjectType,
                                       PreviousMode,
                                       (PVOID*)&DebugObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Clear process and thread */
    Process = NULL;
    Thread = NULL;

    /* Wait on the debug object given to us */
    while (TRUE)
    {
        Status = KeWaitForSingleObject(&DebugObject->EventsPresent,
                                       Executive,
                                       PreviousMode,
                                       Alertable,
                                       Timeout);
        if (!NT_SUCCESS(Status) ||
            (Status == STATUS_TIMEOUT) ||
            (Status == STATUS_ALERTED) ||
            (Status == STATUS_USER_APC))
        {
            /* Break out the wait */
            break;
        }

        /* Lock the object */
        GotEvent = FALSE;
        ExAcquireFastMutex(&DebugObject->Mutex);

        /* Check if a debugger is connected */
        if (DebugObject->DebuggerInactive)
        {
            /* Not connected */
            Status = STATUS_DEBUGGER_INACTIVE;
        }
        else
        {
            /* Loop the events */
            ListHead = &DebugObject->EventList;
            NextEntry =  ListHead->Flink;
            while (ListHead != NextEntry)
            {
                /* Get the debug event */
                DebugEvent = CONTAINING_RECORD(NextEntry,
                                               DEBUG_EVENT,
                                               EventList);
                DBGKTRACE(DBGK_PROCESS_DEBUG, "DebugEvent: %p Flags: %lx\n",
                          DebugEvent, DebugEvent->Flags);

                /* Check flags */
                if (!(DebugEvent->Flags & (DEBUG_EVENT_INACTIVE | DEBUG_EVENT_READ)))
                {
                    /* We got an event */
                    GotEvent = TRUE;

                    /* Loop the list internally */
                    NextEntry2 = DebugObject->EventList.Flink;
                    while (NextEntry2 != NextEntry)
                    {
                        /* Get the debug event */
                        DebugEvent2 = CONTAINING_RECORD(NextEntry2,
                                                        DEBUG_EVENT,
                                                        EventList);

                        /* Try to match process IDs */
                        if (DebugEvent2->ClientId.UniqueProcess ==
                            DebugEvent->ClientId.UniqueProcess)
                        {
                            /* Found it, break out */
                            DebugEvent->Flags |= DEBUG_EVENT_INACTIVE;
                            DebugEvent->BackoutThread = NULL;
                            GotEvent = FALSE;
                            break;
                        }

                        /* Move to the next entry */
                        NextEntry2 = NextEntry2->Flink;
                    }

                    /* Check if we still have a valid event */
                    if (GotEvent) break;
                }

                /* Move to the next entry */
                NextEntry = NextEntry->Flink;
            }

            /* Check if we have an event */
            if (GotEvent)
            {
                /* Save and reference the process and thread */
                Process = DebugEvent->Process;
                Thread = DebugEvent->Thread;
                ObReferenceObject(Process);
                ObReferenceObject(Thread);

                /* Convert to user-mode structure */
                DbgkpConvertKernelToUserStateChange(&WaitStateChange,
                                                    DebugEvent);

                /* Set flag */
                DebugEvent->Flags |= DEBUG_EVENT_READ;
            }
            else
            {
                /* Unsignal the event */
                KeClearEvent(&DebugObject->EventsPresent);
            }

            /* Set success */
            Status = STATUS_SUCCESS;
        }

        /* Release the mutex */
        ExReleaseFastMutex(&DebugObject->Mutex);
        if (!NT_SUCCESS(Status)) break;

        /* Check if we got an event */
        if (!GotEvent)
        {
            /* Check if we can wait again */
            if (LocalTimeOut.QuadPart < 0)
            {
                /* Query the new time */
                KeQuerySystemTime(&NewTime);

                /* Substract times */
                LocalTimeOut.QuadPart += (NewTime.QuadPart - StartTime.QuadPart);
                StartTime = NewTime;

                /* Check if we've timed out */
                if (LocalTimeOut.QuadPart >= 0)
                {
                    /* We have, break out of the loop */
                    Status = STATUS_TIMEOUT;
                    break;
                }
            }
        }
        else
        {
            /* Open the handles and dereference the objects */
            DbgkpOpenHandles(&WaitStateChange, Process, Thread);
            ObDereferenceObject(Process);
            ObDereferenceObject(Thread);
            break;
        }
    }

    /* We're done, dereference the object */
    ObDereferenceObject(DebugObject);

    /* Protect write with SEH */
    _SEH2_TRY
    {
        /* Return our wait state change structure */
        *StateChange = WaitStateChange;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Get SEH Exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return status */
    return Status;
}
