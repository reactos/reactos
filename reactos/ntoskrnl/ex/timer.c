/* $Id: nttimer.c 12779 2005-01-04 04:45:00Z gdalsnes $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/timer.c
 * PURPOSE:         User-mode timers
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) - Reimplemented
 *                  David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* TYPES ********************************************************************/

/* Executive Timer Object */
typedef struct _ETIMER {
    KTIMER KeTimer;
    KAPC TimerApc;
    KDPC TimerDpc;
    LIST_ENTRY ActiveTimerListEntry;
    KSPIN_LOCK Lock;
    LONG Period;
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
static GENERIC_MAPPING ExpTimerMapping = {
    STANDARD_RIGHTS_READ    | TIMER_QUERY_STATE,
    STANDARD_RIGHTS_WRITE   | TIMER_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    TIMER_ALL_ACCESS
};

/* Timer Information Classes */
static const INFORMATION_CLASS_INFO ExTimerInfoClass[] = {
    
    /* TimerBasicInformation */
    ICI_SQ_SAME( sizeof(TIMER_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ),
};

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
ExpDeleteTimer(PVOID ObjectBody)
{
    KIRQL OldIrql;
    PETIMER Timer = ObjectBody;

    DPRINT("ExpDeleteTimer(Timer: %x)\n", Timer);

    /* Lock the Wake List */
    KeAcquireSpinLock(&ExpWakeListLock, &OldIrql);
    
    /* Check if it has a Wait List */
    if (!IsListEmpty(&Timer->WakeTimerListEntry)) {
    
        /* Remove it from the Wait List */
        DPRINT("Removing wake list\n");
        RemoveEntryList(&Timer->WakeTimerListEntry);
    }
    
    /* Release the Wake List */
    KeReleaseSpinLock(&ExpWakeListLock, OldIrql);

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

    DPRINT("ExpTimerDpcRoutine(Dpc: %x)\n", Dpc);

    /* Get the Timer Object */
    Timer = (PETIMER)DeferredContext;

    /* Lock the Timer */
    KeAcquireSpinLock(&Timer->Lock, &OldIrql);
    
    /* Queue the APC */
    if(Timer->ApcAssociated) {
        
        DPRINT("Queuing APC\n");
        KeInsertQueueApc(&Timer->TimerApc,
                         SystemArgument1,
                         SystemArgument2,
                         IO_NO_INCREMENT);
    }
    
    /* Release the Timer */
    KeReleaseSpinLock(&Timer->Lock, OldIrql);
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
    PETHREAD CurrentThread = PsGetCurrentThread();
    KIRQL OldIrql;
    
    /* We need to find out which Timer we are */
    Timer = CONTAINING_RECORD(Apc, ETIMER, TimerApc);
    DPRINT("ExpTimerApcKernelRoutine(Apc: %x. Timer: %x)\n", Apc, Timer);
    
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
        (!Timer->Period)) {
    
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
ExpInitializeTimerImplementation(VOID)
{
    DPRINT("ExpInitializeTimerImplementation()\n");
            
    /* Allocate Memory for the Timer */
    ExTimerType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

    /* Create the Executive Timer Object */
    RtlpCreateUnicodeString(&ExTimerType->TypeName, L"Timer", NonPagedPool);
    ExTimerType->Tag = TAG('T', 'I', 'M', 'T');
    ExTimerType->PeakObjects = 0;
    ExTimerType->PeakHandles = 0;
    ExTimerType->TotalObjects = 0;
    ExTimerType->TotalHandles = 0;
    ExTimerType->PagedPoolCharge = 0;
    ExTimerType->NonpagedPoolCharge = sizeof(ETIMER);
    ExTimerType->Mapping = &ExpTimerMapping;
    ExTimerType->Dump = NULL;
    ExTimerType->Open = NULL;
    ExTimerType->Close = NULL;
    ExTimerType->Delete = ExpDeleteTimer;
    ExTimerType->Parse = NULL;
    ExTimerType->Security = NULL;
    ExTimerType->QueryName = NULL;
    ExTimerType->OkayToClose = NULL;
    ExTimerType->Create = NULL;
    ExTimerType->DuplicationNotify = NULL;
    ObpCreateTypeObject(ExTimerType);
    
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
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN State;
    KIRQL OldIrql;
    PETHREAD TimerThread;
    BOOLEAN KillTimer = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();
   
    DPRINT("NtCancelTimer(0x%x, 0x%x)\n", TimerHandle, CurrentState);
   
    /* Check Parameter Validity */
    if(CurrentState != NULL && PreviousMode != KernelMode) {
        _SEH_TRY {
            ProbeForWrite(CurrentState,
                          sizeof(BOOLEAN),
                          sizeof(BOOLEAN));
        } _SEH_HANDLE {
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
     
        if(!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    /* Get the Timer Object */
    Status = ObReferenceObjectByHandle(TimerHandle,
                                       TIMER_ALL_ACCESS,
                                       ExTimerType,
                                       PreviousMode,
                                       (PVOID*)&Timer,
                                       NULL);
    
    /* Check for success */
    if(NT_SUCCESS(Status)) {
       
        DPRINT("Timer Referencced: %x\n", Timer);
        
        /* Lock the Timer */
        KeAcquireSpinLock(&Timer->Lock, &OldIrql);
        
        /* Check if it's enabled */
        if (Timer->ApcAssociated) {
        
            /* 
             * First, remove it from the Thread's Active List 
             * Get the Thread.
             */
            TimerThread = CONTAINING_RECORD(Timer->TimerApc.Thread, ETHREAD, Tcb);
            DPRINT("Removing from Thread: %x\n", TimerThread);
            
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
            DPRINT("APC was not Associated. Cancelling Timer\n");
            KeCancelTimer(&Timer->KeTimer);
        }
        
        /* Read the old State */
        State = KeReadStateTimer(&Timer->KeTimer);
        
        /* Dereference the Object */
        ObDereferenceObject(Timer);
        
        /* Unlock the Timer */
        KeReleaseSpinLock(&Timer->Lock, OldIrql);
                
        /* Dereference if it was previously enabled */
        if (KillTimer) ObDereferenceObject(Timer);
        DPRINT1("Timer disabled\n");

        /* Make sure it's safe to write to the handle */
        if(CurrentState != NULL) {
            _SEH_TRY {
                *CurrentState = State;
            } _SEH_HANDLE {
                Status = _SEH_GetExceptionCode();
            } _SEH_END;
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
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();
   
    DPRINT("NtCreateTimer(Handle: %x, Type: %d)\n", TimerHandle, TimerType);

    /* Check Parameter Validity */
    if (PreviousMode != KernelMode) {
        _SEH_TRY {
            ProbeForWrite(TimerHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        } _SEH_HANDLE {
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
     
        if(!NT_SUCCESS(Status)) {
            return Status;
        }
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
    if(NT_SUCCESS(Status)) {
        
        /* Initialize the Kernel Timer */
        DPRINT("Initializing Timer: %x\n", Timer);
        KeInitializeTimerEx(&Timer->KeTimer, TimerType);

        /* Initialize the Timer Lock */
        KeInitializeSpinLock(&Timer->Lock);
        
        /* Initialize the DPC */
        KeInitializeDpc(&Timer->TimerDpc, ExpTimerDpcRoutine, Timer);

        /* Set Initial State */
        Timer->ApcAssociated = FALSE;
        InitializeListHead(&Timer->WakeTimerListEntry);
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
        _SEH_TRY {
            *TimerHandle = hTimer;
        } _SEH_HANDLE {
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
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
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();

    DPRINT("NtOpenTimer(TimerHandle: %x)\n", TimerHandle);

    /* Check Parameter Validity */
    if (PreviousMode != KernelMode) {
        _SEH_TRY {
            ProbeForWrite(TimerHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        } _SEH_HANDLE {
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
     
        if(!NT_SUCCESS(Status)) {
            return Status;
        }
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
    if(NT_SUCCESS(Status)) {
        
        /* Make sure it's safe to write to the handle */
        _SEH_TRY {
            *TimerHandle = hTimer;
        } _SEH_HANDLE {
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
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
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;
    PTIMER_BASIC_INFORMATION BasicInfo = (PTIMER_BASIC_INFORMATION)TimerInformation;
    
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();

    DPRINT("NtQueryTimer(TimerHandle: %x, Class: %d)\n", TimerHandle, TimerInformationClass);

    /* Check Validity */
    DefaultQueryInfoBufferCheck(TimerInformationClass,
                                ExTimerInfoClass,
                                TimerInformation,
                                TimerInformationLength,
                                ReturnLength,
                                PreviousMode,
                                &Status);
    if(!NT_SUCCESS(Status)) {
        
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
    if(NT_SUCCESS(Status)) {

        /* Return the Basic Information */
        _SEH_TRY {

            /* FIXME: Interrupt correction based on Interrupt Time */
            DPRINT("Returning Information for Timer: %x. Time Remaining: %d\n", Timer, Timer->KeTimer.DueTime.QuadPart);
            BasicInfo->TimeRemaining.QuadPart = Timer->KeTimer.DueTime.QuadPart;
            BasicInfo->SignalState = KeReadStateTimer(&Timer->KeTimer);

            if(ReturnLength != NULL) *ReturnLength = sizeof(TIMER_BASIC_INFORMATION);

        } _SEH_HANDLE {
            
                  Status = _SEH_GetExceptionCode();
        } _SEH_END;
        
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
    KPROCESSOR_MODE PreviousMode;
    PETHREAD CurrentThread;
    LARGE_INTEGER TimerDueTime;
    PETHREAD TimerThread;
    BOOLEAN KillTimer = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();
    CurrentThread = PsGetCurrentThread();

    DPRINT("NtSetTimer(TimerHandle: %x, DueTime: %d, Apc: %x, Period: %d)\n", TimerHandle, DueTime->QuadPart, TimerApcRoutine, Period);

    /* Check Parameter Validity */
    if (PreviousMode != KernelMode) {
        _SEH_TRY {
            ProbeForRead(DueTime,
                         sizeof(LARGE_INTEGER),
                         sizeof(ULONG));
            TimerDueTime = *DueTime;
            
            if(PreviousState != NULL) {
                ProbeForWrite(PreviousState,
                              sizeof(BOOLEAN),
                              sizeof(BOOLEAN));
            }
            
        } _SEH_HANDLE {
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
     
        if(!NT_SUCCESS(Status)) {
            return Status;
        }
    }
    
    /* Get the Timer Object */   
    Status = ObReferenceObjectByHandle(TimerHandle,
                                       TIMER_ALL_ACCESS,
                                       ExTimerType,
                                       PreviousMode,
                                       (PVOID*)&Timer,
                                       NULL);
    
    /* Check status */
    if (NT_SUCCESS(Status)) {
    
        /* Lock the Timer */
        DPRINT("Timer Referencced: %x\n", Timer);
        KeAcquireSpinLock(&Timer->Lock, &OldIrql);
        
        /* Cancel Running Timer */
        if (Timer->ApcAssociated) {
        
            /* 
             * First, remove it from the Thread's Active List 
             * Get the Thread.
             */
            TimerThread = CONTAINING_RECORD(Timer->TimerApc.Thread, ETHREAD, Tcb);
            DPRINT("Thread already running. Removing from Thread: %x\n", TimerThread);
            
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
        if (WakeTimer) {
        
            /* Insert it into the list */
            InsertTailList(&ExpWakeList, &Timer->WakeTimerListEntry);
        
        } else {
            
            /* Remove it from the list */
            RemoveEntryList(&Timer->WakeTimerListEntry);
            Timer->WakeTimerListEntry.Flink = NULL;
        }
        KeReleaseSpinLockFromDpcLevel(&ExpWakeListLock);
        
        /* Set up the APC Routine if specified */
        if (TimerApcRoutine) {
            
            /* Initialize the APC */
            DPRINT("Initializing APC: %x\n", Timer->TimerApc);
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

        /* Dereference the Object */
        ObDereferenceObject(Timer);
        
        /* Unlock the Timer */
        KeReleaseSpinLock(&Timer->Lock, OldIrql);
                
        /* Dereference if it was previously enabled */
        if (!TimerApcRoutine) ObDereferenceObject(Timer);
        if (KillTimer) ObDereferenceObject(Timer);
        DPRINT("Finished Setting the Timer\n");

        /* Make sure it's safe to write to the handle */
        if(PreviousState != NULL) {
            _SEH_TRY {
                *PreviousState = State;
            } _SEH_HANDLE {
                Status = _SEH_GetExceptionCode();
            } _SEH_END;
        }
    }

    /* Return to Caller */
    return Status;
}

/* EOF */
