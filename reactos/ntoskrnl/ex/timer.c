/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/timer.c
 * PURPOSE:         Executive Timer Implementation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* Timer Object Type */
POBJECT_TYPE ExTimerType = NULL;

KSPIN_LOCK ExpWakeListLock;
LIST_ENTRY ExpWakeList;

/* Timer Mapping */
static GENERIC_MAPPING ExpTimerMapping =
{
    STANDARD_RIGHTS_READ    | TIMER_QUERY_STATE,
    STANDARD_RIGHTS_WRITE   | TIMER_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    TIMER_ALL_ACCESS
};

/* Timer Information Classes */
static const INFORMATION_CLASS_INFO ExTimerInfoClass[] =
{
    /* TimerBasicInformation */
    ICI_SQ_SAME(sizeof(TIMER_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY),
};

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
ExTimerRundown(VOID)
{
    PETHREAD Thread = PsGetCurrentThread();
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PETIMER Timer;
    ULONG DerefsToDo;

    /* Lock the Thread's Active Timer List and loop it */
    KeAcquireSpinLock(&Thread->ActiveTimerListLock, &OldIrql);
    CurrentEntry = Thread->ActiveTimerListHead.Flink;
    while (CurrentEntry != &Thread->ActiveTimerListHead)
    {
        /* Get the timer */
        Timer = CONTAINING_RECORD(CurrentEntry, ETIMER, ActiveTimerListEntry);

        /* Reference it */
        ObReferenceObject(Timer);
        DerefsToDo = 1;

        /* Unlock the list */
        KeReleaseSpinLock(&Thread->ActiveTimerListLock, OldIrql);

        /* Lock the Timer */
        KeAcquireSpinLock(&Timer->Lock, &OldIrql);

        /* Lock the list again */
        KeAcquireSpinLockAtDpcLevel(&Thread->ActiveTimerListLock);

        /* Make sure that the timer is valid */
        if ((Timer->ApcAssociated) && (&Thread->Tcb == Timer->TimerApc.Thread))
        {
            /* Remove it from the list */
            RemoveEntryList(&Timer->ActiveTimerListEntry);
            Timer->ApcAssociated = FALSE;

            /* Cancel the timer and remove its DPC and APC */
            KeCancelTimer(&Timer->KeTimer);
            KeRemoveQueueDpc(&Timer->TimerDpc);
            if (KeRemoveQueueApc(&Timer->TimerApc)) DerefsToDo++;

            /* Add another dereference to do */
            DerefsToDo++;
        }

        /* Unlock the list */
        KeReleaseSpinLockFromDpcLevel(&Thread->ActiveTimerListLock);

        /* Unlock the Timer */
        KeReleaseSpinLock(&Timer->Lock, OldIrql);

        /* Dereference it */
        ObDereferenceObjectEx(Timer, DerefsToDo);

        /* Loop again */
        KeAcquireSpinLock(&Thread->ActiveTimerListLock, &OldIrql);
        CurrentEntry = Thread->ActiveTimerListHead.Flink;
    }

    /* Release lock and return */
    KeReleaseSpinLock(&Thread->ActiveTimerListLock, OldIrql);
}

VOID
NTAPI
ExpDeleteTimer(IN PVOID ObjectBody)
{
    KIRQL OldIrql;
    PETIMER Timer = ObjectBody;

    /* Check if it has a Wait List */
    if (Timer->WakeTimerListEntry.Flink)
    {
        /* Lock the Wake List */
        KeAcquireSpinLock(&ExpWakeListLock, &OldIrql);

        /* Check again, since it might've changed before we locked */
        if (Timer->WakeTimerListEntry.Flink)
        {
            /* Remove it from the Wait List */
            RemoveEntryList(&Timer->WakeTimerListEntry);
            Timer->WakeTimerListEntry.Flink = NULL;
        }

        /* Release the Wake List */
        KeReleaseSpinLock(&ExpWakeListLock, OldIrql);
    }

    /* Tell the Kernel to cancel the Timer and flush all queued DPCs */
    KeCancelTimer(&Timer->KeTimer);
    KeFlushQueuedDpcs();
}

VOID
NTAPI
ExpTimerDpcRoutine(IN PKDPC Dpc,
                   IN PVOID DeferredContext,
                   IN PVOID SystemArgument1,
                   IN PVOID SystemArgument2)
{
    PETIMER Timer = DeferredContext;
    BOOLEAN Inserted = FALSE;

    /* Reference the timer */
    if (!ObReferenceObjectSafe(Timer)) return;

    /* Lock the Timer */
    KeAcquireSpinLockAtDpcLevel(&Timer->Lock);

    /* Check if the timer is associated */
    if (Timer->ApcAssociated)
    {
        /* Queue the APC */
        Inserted = KeInsertQueueApc(&Timer->TimerApc,
                                    SystemArgument1,
                                    SystemArgument2,
                                    IO_NO_INCREMENT);
    }

    /* Release the Timer */
    KeReleaseSpinLockFromDpcLevel(&Timer->Lock);

    /* Dereference it if we couldn't queue the APC */
    if (!Inserted) ObDereferenceObject(Timer);
}

VOID
NTAPI
ExpTimerApcKernelRoutine(IN PKAPC Apc,
                         IN OUT PKNORMAL_ROUTINE* NormalRoutine,
                         IN OUT PVOID* NormalContext,
                         IN OUT PVOID* SystemArgument1,
                         IN OUT PVOID* SystemArguemnt2)
{
    PETIMER Timer;
    KIRQL OldIrql;
    ULONG DerefsToDo = 1;
    PETHREAD Thread = PsGetCurrentThread();

    /* We need to find out which Timer we are */
    Timer = CONTAINING_RECORD(Apc, ETIMER, TimerApc);

    /* Lock the Timer */
    KeAcquireSpinLock(&Timer->Lock, &OldIrql);

    /* Lock the Thread's Active Timer List*/
    KeAcquireSpinLockAtDpcLevel(&Thread->ActiveTimerListLock);

    /* Make sure that the Timer is valid, and that it belongs to this thread */
    if ((Timer->ApcAssociated) && (&Thread->Tcb == Timer->TimerApc.Thread))
    {
        /* Check if it's not periodic */
        if (!Timer->Period)
        {
            /* Remove it from the Active Timers List */
            RemoveEntryList(&Timer->ActiveTimerListEntry);

            /* Disable it */
            Timer->ApcAssociated = FALSE;
            DerefsToDo++;
        }
    }
    else
    {
        /* Clear the normal routine */
        *NormalRoutine = NULL;
    }

    /* Release locks */
    KeReleaseSpinLockFromDpcLevel(&Thread->ActiveTimerListLock);
    KeReleaseSpinLock(&Timer->Lock, OldIrql);

    /* Dereference as needed */
    ObDereferenceObjectEx(Timer, DerefsToDo);
}

VOID
INIT_FUNCTION
NTAPI
ExpInitializeTimerImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;

    /* Create the Timer Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Timer");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(ETIMER);
    ObjectTypeInitializer.GenericMapping = ExpTimerMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = TIMER_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = ExpDeleteTimer;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &ExTimerType);

    /* Initialize the Wait List and Lock */
    KeInitializeSpinLock(&ExpWakeListLock);
    InitializeListHead(&ExpWakeList);
}

/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
NtCancelTimer(IN HANDLE TimerHandle,
              OUT PBOOLEAN CurrentState OPTIONAL)
{
    PETIMER Timer;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    BOOLEAN State;
    KIRQL OldIrql;
    PETHREAD TimerThread;
    ULONG DerefsToDo = 1;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check Parameter Validity */
    if ((CurrentState) && (PreviousMode != KernelMode))
    {
        _SEH_TRY
        {
            ProbeForWriteBoolean(CurrentState);
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Get the Timer Object */
    Status = ObReferenceObjectByHandle(TimerHandle,
                                       TIMER_MODIFY_STATE,
                                       ExTimerType,
                                       PreviousMode,
                                       (PVOID*)&Timer,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Lock the Timer */
        KeAcquireSpinLock(&Timer->Lock, &OldIrql);

        /* Check if it's enabled */
        if (Timer->ApcAssociated)
        {
            /* Get the Thread. */
            TimerThread = CONTAINING_RECORD(Timer->TimerApc.Thread,
                                            ETHREAD,
                                            Tcb);

            /* Lock its active list */
            KeAcquireSpinLockAtDpcLevel(&TimerThread->ActiveTimerListLock);

            /* Remove it */
            RemoveEntryList(&TimerThread->ActiveTimerListHead);
            Timer->ApcAssociated = FALSE;

            /* Unlock the list */
            KeReleaseSpinLockFromDpcLevel(&TimerThread->ActiveTimerListLock);

            /* Cancel the Timer */
            KeCancelTimer(&Timer->KeTimer);
            KeRemoveQueueDpc(&Timer->TimerDpc);
            if (KeRemoveQueueApc(&Timer->TimerApc)) DerefsToDo++;
            DerefsToDo++;
        }
        else
        {
            /* If timer was disabled, we still need to cancel it */
            KeCancelTimer(&Timer->KeTimer);
        }

        /* Handle a Wake Timer */
        if (Timer->WakeTimerListEntry.Flink)
        {
            /* Lock the Wake List */
            KeAcquireSpinLockAtDpcLevel(&ExpWakeListLock);

            /* Check again, since it might've changed before we locked */
            if (Timer->WakeTimerListEntry.Flink)
            {
                /* Remove it from the Wait List */
                RemoveEntryList(&Timer->WakeTimerListEntry);
                Timer->WakeTimerListEntry.Flink = NULL;
            }

            /* Release the Wake List */
            KeReleaseSpinLockFromDpcLevel(&ExpWakeListLock);
        }

        /* Unlock the Timer */
        KeReleaseSpinLock(&Timer->Lock, OldIrql);

        /* Read the old State */
        State = KeReadStateTimer(&Timer->KeTimer);

        /* Dereference the Object */
        ObDereferenceObjectEx(Timer, DerefsToDo);

        /* Make sure it's safe to write to the handle */
        if (CurrentState)
        {
            _SEH_TRY
            {
                *CurrentState = State;
            }
            _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
            {

            }
            _SEH_END;
        }
    }

    /* Return to Caller */
    return Status;
}

NTSTATUS
NTAPI
NtCreateTimer(OUT PHANDLE TimerHandle,
              IN ACCESS_MASK DesiredAccess,
              IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
              IN TIMER_TYPE TimerType)
{
    PETIMER Timer;
    HANDLE hTimer;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check for correct timer type */
    if ((TimerType != NotificationTimer) &&
        (TimerType != SynchronizationTimer))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER_4;
    }

    /* Check Parameter Validity */
    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(TimerHandle);
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Create the Object */
    Status = ObCreateObject(PreviousMode,
                            ExTimerType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(ETIMER),
                            0,
                            0,
                            (PVOID*)&Timer);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the DPC */
        KeInitializeDpc(&Timer->TimerDpc, ExpTimerDpcRoutine, Timer);

        /* Initialize the Kernel Timer */
        KeInitializeTimerEx(&Timer->KeTimer, TimerType);

        /* Initialize the timer fields */
        KeInitializeSpinLock(&Timer->Lock);
        Timer->ApcAssociated = FALSE;
        Timer->WakeTimer = FALSE;
        Timer->WakeTimerListEntry.Flink = NULL;

        /* Insert the Timer */
        Status = ObInsertObject((PVOID)Timer,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hTimer);

        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            /* Make sure it's safe to write to the handle */
            _SEH_TRY
            {
                *TimerHandle = hTimer;
            }
            _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
            {

            }
            _SEH_END;
        }
    }

    /* Return to Caller */
    return Status;
}

NTSTATUS
NTAPI
NtOpenTimer(OUT PHANDLE TimerHandle,
            IN ACCESS_MASK DesiredAccess,
            IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hTimer;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check Parameter Validity */
    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(TimerHandle);
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Timer */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExTimerType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &hTimer);
    if (NT_SUCCESS(Status))
    {
        /* Make sure it's safe to write to the handle */
        _SEH_TRY
        {
            *TimerHandle = hTimer;
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {

        }
        _SEH_END;
    }

    /* Return to Caller */
    return Status;
}

NTSTATUS
NTAPI
NtQueryTimer(IN HANDLE TimerHandle,
             IN TIMER_INFORMATION_CLASS TimerInformationClass,
             OUT PVOID TimerInformation,
             IN ULONG TimerInformationLength,
             OUT PULONG ReturnLength OPTIONAL)
{
    PETIMER Timer;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PTIMER_BASIC_INFORMATION BasicInfo = TimerInformation;
    PAGED_CODE();

    /* Check Validity */
    Status = DefaultQueryInfoBufferCheck(TimerInformationClass,
                                         ExTimerInfoClass,
                                         sizeof(ExTimerInfoClass) /
                                         sizeof(ExTimerInfoClass[0]),
                                         TimerInformation,
                                         TimerInformationLength,
                                         ReturnLength,
                                         NULL,
                                         PreviousMode);
    if(!NT_SUCCESS(Status)) return Status;

    /* Get the Timer Object */
    Status = ObReferenceObjectByHandle(TimerHandle,
                                       TIMER_QUERY_STATE,
                                       ExTimerType,
                                       PreviousMode,
                                       (PVOID*)&Timer,
                                       NULL);
    if(NT_SUCCESS(Status))
    {
        /* Return the Basic Information */
        _SEH_TRY
        {
            /* Return the remaining time, corrected */
            BasicInfo->TimeRemaining.QuadPart = Timer->
                                                KeTimer.DueTime.QuadPart -
                                                KeQueryInterruptTime();

            /* Return the current state */
            BasicInfo->SignalState = KeReadStateTimer(&Timer->KeTimer);

            /* Return the buffer length if requested */
            if (ReturnLength) *ReturnLength = sizeof(TIMER_BASIC_INFORMATION);
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Dereference Object */
        ObDereferenceObject(Timer);
    }

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
NtSetTimer(IN HANDLE TimerHandle,
           IN PLARGE_INTEGER DueTime,
           IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
           IN PVOID TimerContext OPTIONAL,
           IN BOOLEAN WakeTimer,
           IN LONG Period OPTIONAL,
           OUT PBOOLEAN PreviousState OPTIONAL)
{
    PETIMER Timer;
    KIRQL OldIrql;
    BOOLEAN State;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PETHREAD Thread = PsGetCurrentThread();
    LARGE_INTEGER TimerDueTime;
    PETHREAD TimerThread;
    ULONG DerefsToDo = 1;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check for a valid Period */
    if (Period < 0) return STATUS_INVALID_PARAMETER_6;

    /* Check Parameter Validity */
    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            TimerDueTime = ProbeForReadLargeInteger(DueTime);
            if (PreviousState) ProbeForWriteBoolean(PreviousState);
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Capture the time directly */
        TimerDueTime = *DueTime;
    }

    /* Get the Timer Object */
    Status = ObReferenceObjectByHandle(TimerHandle,
                                       TIMER_MODIFY_STATE,
                                       ExTimerType,
                                       PreviousMode,
                                       (PVOID*)&Timer,
                                       NULL);

    /* 
     * Tell the user we don't support Wake Timers...
     * when we have the ability to use/detect the Power Management 
     * functionality required to support them, make this check dependent
     * on the actual PM capabilities
     */
    if (WakeTimer) Status = STATUS_TIMER_RESUME_IGNORED;

    /* Check status */
    if (NT_SUCCESS(Status))
    {
        /* Lock the Timer */
        KeAcquireSpinLock(&Timer->Lock, &OldIrql);

        /* Cancel Running Timer */
        if (Timer->ApcAssociated)
        {
            /* Get the Thread. */
            TimerThread = CONTAINING_RECORD(Timer->TimerApc.Thread,
                                            ETHREAD,
                                            Tcb);

            /* Lock its active list */
            KeAcquireSpinLockAtDpcLevel(&TimerThread->ActiveTimerListLock);

            /* Remove it */
            RemoveEntryList(&TimerThread->ActiveTimerListHead);
            Timer->ApcAssociated = FALSE;

            /* Unlock the list */
            KeReleaseSpinLockFromDpcLevel(&TimerThread->ActiveTimerListLock);

            /* Cancel the Timer */
            KeCancelTimer(&Timer->KeTimer);
            KeRemoveQueueDpc(&Timer->TimerDpc);
            if (KeRemoveQueueApc(&Timer->TimerApc)) DerefsToDo++;
            DerefsToDo++;
        }
        else
        {
            /* If timer was disabled, we still need to cancel it */
            KeCancelTimer(&Timer->KeTimer);
        }

        /* Read the State */
        State = KeReadStateTimer(&Timer->KeTimer);

        /* Handle Wake Timers */
        Timer->WakeTimer = WakeTimer;
        KeAcquireSpinLockAtDpcLevel(&ExpWakeListLock);
        if ((WakeTimer) && !(Timer->WakeTimerListEntry.Flink))
        {
            /* Insert it into the list */
            InsertTailList(&ExpWakeList, &Timer->WakeTimerListEntry);
        }
        else if (!(WakeTimer) && (Timer->WakeTimerListEntry.Flink))
        {
            /* Remove it from the list */
            RemoveEntryList(&Timer->WakeTimerListEntry);
            Timer->WakeTimerListEntry.Flink = NULL;
        }
        KeReleaseSpinLockFromDpcLevel(&ExpWakeListLock);

        /* Set up the APC Routine if specified */
        Timer->Period = Period;
        if (TimerApcRoutine)
        {
            /* Initialize the APC */
            KeInitializeApc(&Timer->TimerApc,
                            &Thread->Tcb,
                            CurrentApcEnvironment,
                            ExpTimerApcKernelRoutine,
                            (PKRUNDOWN_ROUTINE)NULL,
                            (PKNORMAL_ROUTINE)TimerApcRoutine,
                            PreviousMode,
                            TimerContext);

            /* Lock the Thread's Active List and Insert */
            KeAcquireSpinLockAtDpcLevel(&Thread->ActiveTimerListLock);
            InsertTailList(&Thread->ActiveTimerListHead,
                           &Timer->ActiveTimerListEntry);
            Timer->ApcAssociated = TRUE;
            KeReleaseSpinLockFromDpcLevel(&Thread->ActiveTimerListLock);

            /* One less dereference to do */
            DerefsToDo--;
         }

        /* Enable and Set the Timer */
        KeSetTimerEx(&Timer->KeTimer,
                     TimerDueTime,
                     Period,
                     TimerApcRoutine ? &Timer->TimerDpc : NULL);

        /* Unlock the Timer */
        KeReleaseSpinLock(&Timer->Lock, OldIrql);

        /* Dereference if it was previously enabled */
        if (DerefsToDo) ObDereferenceObjectEx(Timer, DerefsToDo);

        /* Make sure it's safe to write to the handle */
        if (PreviousState)
        {
            _SEH_TRY
            {
                *PreviousState = State;
            }
            _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
            {
            }
            _SEH_END;
        }
    }

    /* Return to Caller */
    return Status;
}
