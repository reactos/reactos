/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/dbgk/debug.c
 * PURPOSE:         User-Mode Debugging Support, Debug Object Management.
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

POBJECT_TYPE DbgkDebugObjectType;
KGUARDED_MUTEX DbgkpProcessDebugPortMutex;

GENERIC_MAPPING DbgkDebugObjectMapping =
{
    STANDARD_RIGHTS_READ    | DEBUG_OBJECT_WAIT_STATE_CHANGE,
    STANDARD_RIGHTS_WRITE   | DEBUG_OBJECT_ADD_REMOVE_PROCESS,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    DEBUG_OBJECT_ALL_ACCESS
};

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
DbgkpDeleteObject(IN PVOID Object)
{
    PDBGK_DEBUG_OBJECT DebugObject = Object;
    PAGED_CODE();

    /* Sanity check */
    ASSERT(IsListEmpty(&DebugObject->StateEventListEntry));
}

VOID
NTAPI
DbgkpCloseObject(IN PEPROCESS Process OPTIONAL,
                 IN PVOID ObjectBody,
                 IN ACCESS_MASK GrantedAccess,
                 IN ULONG HandleCount,
                 IN ULONG SystemHandleCount)
{
    /* FIXME: Implement */
    ASSERT(FALSE);
}

VOID
INIT_FUNCTION
NTAPI
DbgkInitialize(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    PAGED_CODE();

    /* Initialize the process debug port mutex */
    KeInitializeGuardedMutex(&DbgkpProcessDebugPortMutex);

    /* Create the Event Pair Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"DebugObject");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DBGK_DEBUG_OBJECT);
    ObjectTypeInitializer.GenericMapping = DbgkDebugObjectMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = DEBUG_OBJECT_WAIT_STATE_CHANGE;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.CloseProcedure = DbgkpCloseObject;
    ObjectTypeInitializer.DeleteProcedure = DbgkpDeleteObject;
    ObCreateObjectType(&Name,
                       &ObjectTypeInitializer,
                       NULL,
                       &DbgkDebugObjectType);
}

VOID
NTAPI
DbgkCopyProcessDebugPort(IN PEPROCESS Process,
                         IN PEPROCESS Parent)
{
    /* FIXME: Implement */
}

BOOLEAN
NTAPI
DbgkForwardException(IN PEXCEPTION_RECORD ExceptionRecord,
                     IN BOOLEAN DebugPort,
                     IN BOOLEAN SecondChance)
{
    /* FIXME: Implement */
    return FALSE;
}

VOID
NTAPI
DbgkpWakeTarget(IN PDEBUG_EVENT DebugEvent)
{
    /* FIXME: TODO */
    return;
}

NTSTATUS
NTAPI
DbgkpPostFakeProcessCreateMessages(IN PEPROCESS Process,
                                   IN PDBGK_DEBUG_OBJECT DebugObject,
                                   IN PETHREAD *LastThread)
{
    /* FIXME: Implement */
    *LastThread = NULL;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
DbgkpSetProcessDebugObject(IN PEPROCESS Process,
                           IN PDBGK_DEBUG_OBJECT DebugObject,
                           IN NTSTATUS MsgStatus,
                           IN PETHREAD LastThread)
{
    /* FIXME: TODO */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
DbgkClearProcessDebugObject(IN PEPROCESS Process,
                            IN PDBGK_DEBUG_OBJECT SourceDebugObject)
{
    /* FIXME: TODO */
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
DbgkpConvertKernelToUserStateChange(IN PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
                                    IN PDEBUG_EVENT DebugEvent)
{
    /* FIXME: TODO */
    return;
}

VOID
NTAPI
DbgkpOpenHandles(IN PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
                 IN PEPROCESS Process,
                 IN PETHREAD Thread)
{
    /* FIXME: TODO */
    return;
}


/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
NtCreateDebugObject(OUT PHANDLE DebugHandle,
                    IN ACCESS_MASK DesiredAccess,
                    IN POBJECT_ATTRIBUTES ObjectAttributes,
                    IN BOOLEAN KillProcessOnExit)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PDBGK_DEBUG_OBJECT DebugObject;
    HANDLE hDebug;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if we were called from user mode*/
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH_TRY
        {
            /* Probe the handle */
            ProbeForWrite(DebugHandle, sizeof(HANDLE), sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            /* Get exception error */
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Create the Object */
    Status = ObCreateObject(PreviousMode,
                            DbgkDebugObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(PDBGK_DEBUG_OBJECT),
                            0,
                            0,
                            (PVOID*)&DebugObject);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the Debug Object's Fast Mutex */
        ExInitializeFastMutex(&DebugObject->Mutex);

        /* Initialize the State Event List */
        InitializeListHead(&DebugObject->StateEventListEntry);

        /* Initialize the Debug Object's Wait Event */
        KeInitializeEvent(&DebugObject->Event, NotificationEvent, 0);

        /* Set the Flags */
        DebugObject->KillProcessOnExit = KillProcessOnExit;

        /* Insert it */
        Status = ObInsertObject((PVOID)DebugObject,
                                 NULL,
                                 DesiredAccess,
                                 0,
                                 NULL,
                                 &hDebug);
        ObDereferenceObject(DebugObject);

        /* Check for success and return handle */
        if (NT_SUCCESS(Status))
        {
            _SEH_TRY
            {
                *DebugHandle = hDebug;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            } _SEH_END;
        }
    }

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
NtDebugContinue(IN HANDLE DebugHandle,
                IN PCLIENT_ID AppClientId,
                IN NTSTATUS ContinueStatus)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PDBGK_DEBUG_OBJECT DebugObject;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEBUG_EVENT DebugEvent = NULL, DebugEventToWake = NULL;
    PLIST_ENTRY ListHead, NextEntry;
    BOOLEAN NeedsWake = FALSE;
    CLIENT_ID ClientId;
    PAGED_CODE();

    /* Check if we were called from user mode*/
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH_TRY
        {
            /* Probe the handle */
            ProbeForRead(AppClientId, sizeof(CLIENT_ID), sizeof(ULONG));
            ClientId = *AppClientId;
        }
        _SEH_HANDLE
        {
            /* Get exception error */
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Make sure that the status is valid */
    if ((ContinueStatus != DBG_EXCEPTION_NOT_HANDLED) &&
        (ContinueStatus != DBG_REPLY_LATER) &&
        (ContinueStatus != DBG_UNABLE_TO_PROVIDE_HANDLE) &&
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
            ListHead = &DebugObject->StateEventListEntry;
            NextEntry = ListHead->Flink;
            while (ListHead != NextEntry)
            {
                /* Get the current debug event */
                DebugEvent = CONTAINING_RECORD(NextEntry,
                                               DEBUG_EVENT,
                                               EventList);

                /* Compare process ID */
                if (DebugEvent->ClientId.UniqueProcess ==
                    ClientId.UniqueProcess)
                {
                    /* Check if we already found a match */
                    if (NeedsWake)
                    {
                        /* Wake it up and break out */
                        DebugEvent->Flags &= ~4;
                        KeSetEvent(&DebugEvent->ContinueEvent,
                                   IO_NO_INCREMENT,
                                   FALSE);
                        break;
                    }

                    /* Compare thread ID and flag */
                    if ((DebugEvent->ClientId.UniqueThread ==
                        ClientId.UniqueThread) && (DebugEvent->Flags & 1))
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
                DebugEvent->ApiMsg.ReturnedStatus = Status;
                DebugEvent->Status = STATUS_SUCCESS;

                /* Wake the target */
                DbgkpWakeTarget(DebugEvent);
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

NTSTATUS
NTAPI
NtDebugActiveProcess(IN HANDLE ProcessHandle,
                     IN HANDLE DebugHandle)
{
    PEPROCESS Process;
    PDBGK_DEBUG_OBJECT DebugObject;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PETHREAD LastThread;
    NTSTATUS Status;

    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_SUSPEND_RESUME,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Don't allow debugging the initial system process */
    if (Process == PsInitialSystemProcess) return STATUS_ACCESS_DENIED;

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

NTSTATUS
NTAPI
NtRemoveProcessDebug(IN HANDLE ProcessHandle,
                     IN HANDLE DebugHandle)
{
    PEPROCESS Process;
    PDBGK_DEBUG_OBJECT DebugObject;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status;

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

static const INFORMATION_CLASS_INFO DbgkpDebugObjectInfoClass[] =
{
    /* DebugObjectUnusedInformation */
    ICI_SQ_SAME(sizeof(ULONG), sizeof(ULONG), 0),
    /* DebugObjectKillProcessOnExitInformation */
    ICI_SQ_SAME(sizeof(DEBUG_OBJECT_KILL_PROCESS_ON_EXIT_INFORMATION), sizeof(ULONG), ICIF_SET),
};

NTSTATUS
NTAPI
NtSetInformationDebugObject(IN HANDLE DebugHandle,
                            IN DEBUGOBJECTINFOCLASS DebugObjectInformationClass,
                            IN PVOID DebugInformation,
                            IN ULONG DebugInformationLength,
                            OUT PULONG ReturnLength OPTIONAL)
{
    PDBGK_DEBUG_OBJECT DebugObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
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

    /* Return required length to user-mode */
    if (ReturnLength) *ReturnLength = sizeof(*DebugInfo);
    if (!NT_SUCCESS(Status)) return Status;

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
            DebugObject->Flags |= 2;
        }
        else
        {
            /* Disable */
            DebugObject->Flags &= ~2;
        }

        /* Release the mutex */
        ExReleaseFastMutex(&DebugObject->Mutex);

        /* Release the Object */
        ObDereferenceObject(DebugObject);
    }

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
NtWaitForDebugEvent(IN HANDLE DebugHandle,
                    IN BOOLEAN Alertable,
                    IN PLARGE_INTEGER Timeout OPTIONAL,
                    OUT PDBGUI_WAIT_STATE_CHANGE StateChange)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER SafeTimeOut;
    PEPROCESS Process;
    LARGE_INTEGER StartTime;
    PETHREAD Thread;
    BOOLEAN GotEvent;
    LARGE_INTEGER NewTime;
    PDBGK_DEBUG_OBJECT DebugObject;
    DBGUI_WAIT_STATE_CHANGE WaitStateChange;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEBUG_EVENT DebugEvent, DebugEvent2;
    PLIST_ENTRY ListHead, NextEntry;

    /* Clear the initial wait state change structure */
    RtlZeroMemory(&WaitStateChange, sizeof(WaitStateChange));

    /* Check if we came with a timeout from user mode */
    if ((Timeout) && (PreviousMode != KernelMode))
    {
        _SEH_TRY
        {
            /* Make a copy on the stack */
            SafeTimeOut = ProbeForReadLargeInteger(Timeout);
            Timeout = &SafeTimeOut;
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;

        /* Query the current time */
        KeQuerySystemTime(&StartTime);
    }

    /* Check if the call is from user mode */
    if (PreviousMode == UserMode)
    {
        /* FIXME: Probe the state change structure */
    }

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
    Status = KeWaitForSingleObject(DebugObject,
                                   Executive,
                                   PreviousMode,
                                   Alertable,
                                   Timeout);

    /* Start the wait loop */
    while (TRUE)
    {
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
        if (DebugObject->Flags & 1)
        {
            /* Not connected */
            Status = STATUS_DEBUGGER_INACTIVE;
        }
        else
        {
            /* Loop the events */
            ListHead = &DebugObject->StateEventListEntry;
            NextEntry =  ListHead->Flink;
            while (ListHead != NextEntry)
            {
                /* Get the debug event */
                DebugEvent = CONTAINING_RECORD(NextEntry,
                                               DEBUG_EVENT,
                                               EventList);

                /* Check flags */
                if (!(DebugEvent->Flags & (4 | 1)))
                {
                    /* We got an event */
                    GotEvent = TRUE;

                    /* Loop the list internally */
                    while (&DebugEvent->EventList != NextEntry)
                    {
                        /* Get the debug event */
                        DebugEvent2 = CONTAINING_RECORD(NextEntry,
                                                        DEBUG_EVENT,
                                                        EventList);

                        /* Try to match process IDs */
                        if (DebugEvent2->ClientId.UniqueProcess ==
                            DebugEvent->ClientId.UniqueProcess)
                        {
                            /* Found it, break out */
                            DebugEvent->Flags |= 4;
                            DebugEvent->BackoutThread = NULL;
                            GotEvent = FALSE;
                            break;
                        }

                        /* Move to the next entry */
                        NextEntry = NextEntry->Flink;
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
                DebugEvent->Flags |= 1;
                Status = STATUS_SUCCESS;
            }
            else
            {
                /* Unsignal the event */
                DebugObject->Event.Header.SignalState = 0;
                Status = STATUS_SUCCESS;
            }
        }

        /* Release the mutex */
        ExReleaseFastMutex(&DebugObject->Mutex);
        if (!NT_SUCCESS(Status)) break;

        /* Check if we got an event */
        if (GotEvent)
        {
            /* Check if we can wait again */
            if (!SafeTimeOut.QuadPart)
            {
                /* Query the new time */
                KeQuerySystemTime(&NewTime);

                /* Substract times */
                /* FIXME: TODO */
            }
        }
        else
        {
            /* Open the handles and dereference the objects */
            DbgkpOpenHandles(&WaitStateChange, Process, Thread);
            ObDereferenceObject(Process);
            ObDereferenceObject(Thread);
        }
    }

    /* We're, dereference the object */
    ObDereferenceObject(DebugObject);

    /* Return our wait state change structure */
    RtlMoveMemory(StateChange,
                  &WaitStateChange,
                  sizeof(DBGUI_WAIT_STATE_CHANGE));
    return Status;
}

/* EOF */
