/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/timer.c
 * PURPOSE:         Executive Timer Implementation
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* TYPES ********************************************************************/

/* Executive Timer Object */
typedef struct _ETIMER
{
    KTIMER KeTimer;
    KAPC TimerApc;
    KDPC TimerDpc;
    LIST_ENTRY ActiveTimerListEntry;
    KSPIN_LOCK Lock;
    BOOLEAN ApcAssociated;
    BOOLEAN WakeTimer;
    LIST_ENTRY WakeTimerListEntry;
} ETIMER, *PETIMER;

/* GLOBALS ******************************************************************/

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
    ICI_SQ_SAME( sizeof(TIMER_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ),
};

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
ExTimerRundown(VOID)
{
    PETHREAD Thread = PsGetCurrentThread();
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PETIMER Timer;

    /* Lock the Thread's Active Timer List*/
    KeAcquireSpinLock(&Thread->ActiveTimerListLock, &OldIrql);

    while (!IsListEmpty(&Thread->ActiveTimerListHead))
    {
        /* Remove a Timer */
        CurrentEntry = RemoveTailList(&Thread->ActiveTimerListHead);

        /* Get the Timer */
        Timer = CONTAINING_RECORD(CurrentEntry, ETIMER, ActiveTimerListEntry);
        DPRINT("Timer, ThreadList: 0x%p, 0x%p\n", Timer, Thread);

        /* Mark it as deassociated */
        ASSERT (Timer->ApcAssociated);
        Timer->ApcAssociated = FALSE;

        /* Unlock the list */
        KeReleaseSpinLockFromDpcLevel(&Thread->ActiveTimerListLock);

        /* Lock the Timer */
        KeAcquireSpinLockAtDpcLevel(&Timer->Lock);

        /* Cancel the timer and remove its DPC and APC */
        ASSERT(&Thread->Tcb == Timer->TimerApc.Thread);
        KeCancelTimer(&Timer->KeTimer);
        KeRemoveQueueDpc(&Timer->TimerDpc);
        KeRemoveQueueApc(&Timer->TimerApc);

        /* Unlock the Timer */
        KeReleaseSpinLock(&Timer->Lock, OldIrql);

        /* Dereference it */
        ObDereferenceObject(Timer);

        /* Loop again */
        KeAcquireSpinLock(&Thread->ActiveTimerListLock, &OldIrql);
    }

    /* Release lock and return */
    KeReleaseSpinLock(&Thread->ActiveTimerListLock, OldIrql);
    return;
}

VOID
STDCALL
ExpDeleteTimer(PVOID ObjectBody)
{
    KIRQL OldIrql;
    PETIMER Timer = ObjectBody;
    DPRINT("ExpDeleteTimer(Timer: 0x%p)\n", Timer);

    /* Check if it has a Wait List */
    if (Timer->WakeTimer)
    {
        /* Lock the Wake List */
        KeAcquireSpinLock(&ExpWakeListLock, &OldIrql);

        /* Check again, since it might've changed before we locked */
        if (Timer->WakeTimer)
        {
            /* Remove it from the Wait List */
            DPRINT("Removing wake list\n");
            RemoveEntryList(&Timer->WakeTimerListEntry);
            Timer->WakeTimer = FALSE;
        }

        /* Release the Wake List */
        KeReleaseSpinLock(&ExpWakeListLock, OldIrql);
    }

    /* Tell the Kernel to cancel the Timer */
    DPRINT("Cancelling Timer\n");
    KeCancelTimer(&Timer->KeTimer);
}

VOID
STDCALL
ExpTimerDpcRoutine(PKDPC Dpc,
                   PVOID DeferredContext,
                   PVOID SystemArgument1,
                   PVOID SystemArgument2)
{
    PETIMER Timer;
    KIRQL OldIrql;

    DPRINT("ExpTimerDpcRoutine(Dpc: 0x%p)\n", Dpc);

    /* Get the Timer Object */
    Timer = (PETIMER)DeferredContext;

    /* Lock the Timer */
    KeAcquireSpinLock(&Timer->Lock, &OldIrql);

    /* Queue the APC */
    if(Timer->ApcAssociated)
    {
        DPRINT("Queuing APC\n");
        KeInsertQueueApc(&Timer->TimerApc,
                         SystemArgument1,
                         SystemArgument2,
                         IO_NO_INCREMENT);
    }

    /* Release the Timer */
    KeReleaseSpinLock(&Timer->Lock, OldIrql);
    return;
}


VOID
STDCALL
ExpTimerApcKernelRoutine(PKAPC Apc,
                         PKNORMAL_ROUTINE* NormalRoutine,
                         PVOID* NormalContext,
                         PVOID* SystemArgument1,
                         PVOID* SystemArguemnt2)
{
    PETIMER Timer;
    KIRQL OldIrql;
    PETHREAD CurrentThread = PsGetCurrentThread();

    /* We need to find out which Timer we are */
    Timer = CONTAINING_RECORD(Apc, ETIMER, TimerApc);
    DPRINT("ExpTimerApcKernelRoutine(Apc: 0x%p. Timer: 0x%p)\n", Apc, Timer);

    /* Lock the Timer */
    KeAcquireSpinLock(&Timer->Lock, &OldIrql);

    /* Lock the Thread's Active Timer List*/
    KeAcquireSpinLockAtDpcLevel(&CurrentThread->ActiveTimerListLock);

    /*
     * Make sure that the Timer is still valid, and that it belongs to this thread
     * Remove it if it's not periodic
     */
    if ((Timer->ApcAssociated) &&
        (&CurrentThread->Tcb == Timer->TimerApc.Thread) &&
        (!Timer->KeTimer.Period))
    {
        /* Remove it from the Active Timers List */
        DPRINT("Removing Timer\n");
        RemoveEntryList(&Timer->ActiveTimerListEntry);

        /* Disable it */
        Timer->ApcAssociated = FALSE;

        /* Release spinlocks */
        KeReleaseSpinLockFromDpcLevel(&CurrentThread->ActiveTimerListLock);
        KeReleaseSpinLock(&Timer->Lock, OldIrql);

        /* Dereference the Timer Object */
        ObDereferenceObject(Timer);
        return;
    }

    /* Release spinlocks */
    KeReleaseSpinLockFromDpcLevel(&CurrentThread->ActiveTimerListLock);
    KeReleaseSpinLock(&Timer->Lock, OldIrql);
}

VOID
INIT_FUNCTION
STDCALL
ExpInitializeTimerImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;

    DPRINT("Creating Timer Object Type\n");
  
    /* Create the Event Pair Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Timer");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(ETIMER);
    ObjectTypeInitializer.GenericMapping = ExpTimerMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = TIMER_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = ExpDeleteTimer;
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ExTimerType);

    /* Initialize the Wait List and Lock */
    KeInitializeSpinLock(&ExpWakeListLock);
    InitializeListHead(&ExpWakeList);
}

NTSTATUS
STDCALL
NtCancelTimer(IN HANDLE TimerHandle,
              OUT PBOOLEAN CurrentState OPTIONAL)
{
    PETIMER Timer;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    BOOLEAN State;
    KIRQL OldIrql;
    PETHREAD TimerThread;
    BOOLEAN KillTimer = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    DPRINT("NtCancelTimer(0x%p, 0x%x)\n", TimerHandle, CurrentState);

    /* Check Parameter Validity */
    if(CurrentState && PreviousMode != KernelMode)
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

    /* Check for success */
    if(NT_SUCCESS(Status))
    {
        DPRINT("Timer Referenced: 0x%p\n", Timer);

        /* Lock the Timer */
        KeAcquireSpinLock(&Timer->Lock, &OldIrql);

        /* Check if it's enabled */
        if (Timer->ApcAssociated)
        {
            /*
             * First, remove it from the Thread's Active List
             * Get the Thread.
             */
            TimerThread = CONTAINING_RECORD(Timer->TimerApc.Thread, ETHREAD, Tcb);
            DPRINT("Removing from Thread: 0x%p\n", TimerThread);

            /* Lock its active list */
            KeAcquireSpinLockAtDpcLevel(&TimerThread->ActiveTimerListLock);

            /* Remove it */
            RemoveEntryList(&TimerThread->ActiveTimerListHead);

            /* Unlock the list */
            KeReleaseSpinLockFromDpcLevel(&TimerThread->ActiveTimerListLock);

            /* Cancel the Timer */
            KeCancelTimer(&Timer->KeTimer);
            KeRemoveQueueDpc(&Timer->TimerDpc);
            KeRemoveQueueApc(&Timer->TimerApc);
            Timer->ApcAssociated = FALSE;
            KillTimer = TRUE;
        }
        else
        {
            /* If timer was disabled, we still need to cancel it */
            DPRINT("APC was not Associated. Cancelling Timer\n");
            KeCancelTimer(&Timer->KeTimer);
        }

        /* Handle a Wake Timer */
        if (Timer->WakeTimer)
        {
            /* Lock the Wake List */
            KeAcquireSpinLockAtDpcLevel(&ExpWakeListLock);

            /* Check again, since it might've changed before we locked */
            if (Timer->WakeTimer)
            {
                /* Remove it from the Wait List */
                DPRINT("Removing wake list\n");
                RemoveEntryList(&Timer->WakeTimerListEntry);
                Timer->WakeTimer = FALSE;
            }

            /* Release the Wake List */
            KeReleaseSpinLockFromDpcLevel(&ExpWakeListLock);
        }

        /* Unlock the Timer */
        KeReleaseSpinLock(&Timer->Lock, OldIrql);

        /* Read the old State */
        State = KeReadStateTimer(&Timer->KeTimer);

        /* Dereference the Object */
        ObDereferenceObject(Timer);

        /* Dereference if it was previously enabled */
        if (KillTimer) ObDereferenceObject(Timer);
        DPRINT1("Timer disabled\n");

        /* Make sure it's safe to write to the handle */
        if(CurrentState)
        {
            _SEH_TRY
            {
                *CurrentState = State;
            }
            _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
    }

    /* Return to Caller */
    return Status;
}

NTSTATUS
STDCALL
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
    DPRINT("NtCreateTimer(Handle: 0x%p, Type: %d)\n", TimerHandle, TimerType);

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

    /* Check for correct timer type */
    if ((TimerType != NotificationTimer) && (TimerType != SynchronizationTimer))
    {
        DPRINT1("Invalid Timer Type!\n");
        return STATUS_INVALID_PARAMETER_4;
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

    /* Check for Success */
    if(NT_SUCCESS(Status))
    {
        /* Initialize the Kernel Timer */
        DPRINT("Initializing Timer: 0x%p\n", Timer);
        KeInitializeTimerEx(&Timer->KeTimer, TimerType);

        /* Initialize the Timer Lock */
        KeInitializeSpinLock(&Timer->Lock);

        /* Initialize the DPC */
        KeInitializeDpc(&Timer->TimerDpc, ExpTimerDpcRoutine, Timer);

        /* Set Initial State */
        Timer->ApcAssociated = FALSE;
        Timer->WakeTimer = FALSE;

        /* Insert the Timer */
        Status = ObInsertObject((PVOID)Timer,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hTimer);
        DPRINT("Timer Inserted\n");

        /* Make sure it's safe to write to the handle */
        _SEH_TRY
        {
            *TimerHandle = hTimer;
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Return to Caller */
    return Status;
}

NTSTATUS
STDCALL
NtOpenTimer(OUT PHANDLE TimerHandle,
            IN ACCESS_MASK DesiredAccess,
            IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hTimer;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    DPRINT("NtOpenTimer(TimerHandle: 0x%p)\n", TimerHandle);

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
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                &hTimer);

    /* Check for success */
    if(NT_SUCCESS(Status))
    {
        /* Make sure it's safe to write to the handle */
        _SEH_TRY
        {
            *TimerHandle = hTimer;
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Return to Caller */
    return Status;
}


NTSTATUS
STDCALL
NtQueryTimer(IN HANDLE TimerHandle,
             IN TIMER_INFORMATION_CLASS TimerInformationClass,
             OUT PVOID TimerInformation,
             IN ULONG TimerInformationLength,
             OUT PULONG ReturnLength  OPTIONAL)
{
    PETIMER Timer;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PTIMER_BASIC_INFORMATION BasicInfo = (PTIMER_BASIC_INFORMATION)TimerInformation;
    PAGED_CODE();
    DPRINT("NtQueryTimer(TimerHandle: 0x%p, Class: %d)\n", TimerHandle, TimerInformationClass);

    /* Check Validity */
    DefaultQueryInfoBufferCheck(TimerInformationClass,
                                ExTimerInfoClass,
                                TimerInformation,
                                TimerInformationLength,
                                ReturnLength,
                                PreviousMode,
                                &Status);
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryTimer() failed, Status: 0x%x\n", Status);
        return Status;
    }

    /* Get the Timer Object */
    Status = ObReferenceObjectByHandle(TimerHandle,
                                       TIMER_QUERY_STATE,
                                       ExTimerType,
                                       PreviousMode,
                                       (PVOID*)&Timer,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status))
    {
        /* Return the Basic Information */
        _SEH_TRY
        {
            /* Return the remaining time, corrected */
            BasicInfo->TimeRemaining.QuadPart = Timer->KeTimer.DueTime.QuadPart -
                                                KeQueryInterruptTime();

            /* Return the current state */
            BasicInfo->SignalState = KeReadStateTimer(&Timer->KeTimer);

            /* Return the buffer length if requested */
            if(ReturnLength != NULL) *ReturnLength = sizeof(TIMER_BASIC_INFORMATION);

            DPRINT("Returning Information for Timer: 0x%p. Time Remaining: %I64x\n",
                    Timer, BasicInfo->TimeRemaining.QuadPart);
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
STDCALL
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
    PETHREAD CurrentThread = PsGetCurrentThread();
    LARGE_INTEGER TimerDueTime;
    PETHREAD TimerThread;
    BOOLEAN KillTimer = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    DPRINT("NtSetTimer(TimerHandle: 0x%p, DueTime: %I64x, Apc: 0x%p, Period: %d)\n",
            TimerHandle, DueTime->QuadPart, TimerApcRoutine, Period);

    /* Check Parameter Validity */
    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            TimerDueTime = ProbeForReadLargeInteger(DueTime);

            if(PreviousState)
            {
                ProbeForWriteBoolean(PreviousState);
            }
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Check for a valid Period */
    if (Period < 0)
    {
        DPRINT1("Invalid Period for timer\n");
        return STATUS_INVALID_PARAMETER_6;
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
     * functionatliy required to support them, make this check dependent
     * on the actual PM capabilities
     */
    if (WakeTimer) Status = STATUS_TIMER_RESUME_IGNORED;

    /* Check status */
    if (NT_SUCCESS(Status))
    {
        /* Lock the Timer */
        DPRINT("Timer Referencced: 0x%p\n", Timer);
        KeAcquireSpinLock(&Timer->Lock, &OldIrql);

        /* Cancel Running Timer */
        if (Timer->ApcAssociated)
        {
            /*
             * First, remove it from the Thread's Active List
             * Get the Thread.
             */
            TimerThread = CONTAINING_RECORD(Timer->TimerApc.Thread, ETHREAD, Tcb);
            DPRINT("Thread already running. Removing from Thread: 0x%p\n", TimerThread);

            /* Lock its active list */
            KeAcquireSpinLockAtDpcLevel(&TimerThread->ActiveTimerListLock);

            /* Remove it */
            RemoveEntryList(&TimerThread->ActiveTimerListHead);

            /* Unlock the list */
            KeReleaseSpinLockFromDpcLevel(&TimerThread->ActiveTimerListLock);

            /* Cancel the Timer */
            KeCancelTimer(&Timer->KeTimer);
            KeRemoveQueueDpc(&Timer->TimerDpc);
            KeRemoveQueueApc(&Timer->TimerApc);
            Timer->ApcAssociated = FALSE;
            KillTimer = TRUE;

        } else {

            /* If timer was disabled, we still need to cancel it */
            DPRINT("No APCs. Simply cancelling\n");
            KeCancelTimer(&Timer->KeTimer);
        }

        /* Read the State */
        State = KeReadStateTimer(&Timer->KeTimer);

        /* Handle Wake Timers */
        DPRINT("Doing Wake Semantics\n");
        KeAcquireSpinLockAtDpcLevel(&ExpWakeListLock);
        if (WakeTimer && !Timer->WakeTimer)
        {
            /* Insert it into the list */
            Timer->WakeTimer = TRUE;
            InsertTailList(&ExpWakeList, &Timer->WakeTimerListEntry);
        }
        else if (!WakeTimer && Timer->WakeTimer)
        {
            /* Remove it from the list */
            RemoveEntryList(&Timer->WakeTimerListEntry);
            Timer->WakeTimer = FALSE;
        }
        KeReleaseSpinLockFromDpcLevel(&ExpWakeListLock);

        /* Set up the APC Routine if specified */
        if (TimerApcRoutine)
        {
            /* Initialize the APC */
            DPRINT("Initializing APC: 0x%p\n", Timer->TimerApc);
            KeInitializeApc(&Timer->TimerApc,
                            &CurrentThread->Tcb,
                            CurrentApcEnvironment,
                            &ExpTimerApcKernelRoutine,
                            (PKRUNDOWN_ROUTINE)NULL,
                            (PKNORMAL_ROUTINE)TimerApcRoutine,
                            PreviousMode,
                            TimerContext);

            /* Lock the Thread's Active List and Insert */
            KeAcquireSpinLockAtDpcLevel(&CurrentThread->ActiveTimerListLock);
            InsertTailList(&CurrentThread->ActiveTimerListHead,
                           &Timer->ActiveTimerListEntry);
            KeReleaseSpinLockFromDpcLevel(&CurrentThread->ActiveTimerListLock);

         }

        /* Enable and Set the Timer */
        DPRINT("Setting Kernel Timer\n");
        KeSetTimerEx(&Timer->KeTimer,
                     TimerDueTime,
                     Period,
                     TimerApcRoutine ? &Timer->TimerDpc : 0);
        Timer->ApcAssociated = TimerApcRoutine ? TRUE : FALSE;

        /* Unlock the Timer */
        KeReleaseSpinLock(&Timer->Lock, OldIrql);

        /* Dereference if it was previously enabled */
        if (!TimerApcRoutine) ObDereferenceObject(Timer);
        if (KillTimer) ObDereferenceObject(Timer);
        DPRINT("Finished Setting the Timer\n");

        /* Make sure it's safe to write to the handle */
        if(PreviousState != NULL)
        {
            _SEH_TRY
            {
                *PreviousState = State;
            }
            _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
    }

    /* Return to Caller */
    return Status;
}

/* EOF */
