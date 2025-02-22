/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/keyedevt.c
 * PURPOSE:         Support for keyed events
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* INTERNAL TYPES *************************************************************/

#define NUM_KEY_HASH_BUCKETS 23
typedef struct _EX_KEYED_EVENT
{
    struct
    {
        EX_PUSH_LOCK Lock;
        LIST_ENTRY WaitListHead;
        LIST_ENTRY ReleaseListHead;
    } HashTable[NUM_KEY_HASH_BUCKETS];
} EX_KEYED_EVENT, *PEX_KEYED_EVENT;

/* GLOBALS *******************************************************************/

PEX_KEYED_EVENT ExpCritSecOutOfMemoryEvent;
POBJECT_TYPE ExKeyedEventObjectType;

static
GENERIC_MAPPING ExpKeyedEventMapping =
{
    STANDARD_RIGHTS_READ | KEYEDEVENT_WAIT,
    STANDARD_RIGHTS_WRITE | KEYEDEVENT_WAKE,
    STANDARD_RIGHTS_EXECUTE,
    KEYEDEVENT_ALL_ACCESS
};

/* FUNCTIONS *****************************************************************/

_IRQL_requires_max_(APC_LEVEL)
CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeKeyedEventImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer = {0};
    UNICODE_STRING TypeName = RTL_CONSTANT_STRING(L"KeyedEvent");
    UNICODE_STRING Name = RTL_CONSTANT_STRING(L"\\KernelObjects\\CritSecOutOfMemoryEvent");
    NTSTATUS Status;
    HANDLE EventHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Set up the object type initializer */
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.GenericMapping = ExpKeyedEventMapping;
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.ValidAccessMask = KEYEDEVENT_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;

    /* Create the keyed event object type */
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                NULL,
                                &ExKeyedEventObjectType);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Create the out of memory event for critical sections */
    InitializeObjectAttributes(&ObjectAttributes, &Name, OBJ_PERMANENT, NULL, NULL);
    Status = ZwCreateKeyedEvent(&EventHandle,
                                KEYEDEVENT_ALL_ACCESS,
                                &ObjectAttributes,
                                0);
    if (NT_SUCCESS(Status))
    {
        /* Take a reference so we can get rid of the handle */
        Status = ObReferenceObjectByHandle(EventHandle,
                                           KEYEDEVENT_ALL_ACCESS,
                                           ExKeyedEventObjectType,
                                           KernelMode,
                                           (PVOID*)&ExpCritSecOutOfMemoryEvent,
                                           NULL);
        ZwClose(EventHandle);
        return TRUE;
    }

    return FALSE;
}

VOID
NTAPI
ExpInitializeKeyedEvent(
    _Out_ PEX_KEYED_EVENT KeyedEvent)
{
    ULONG i;

    /* Loop all hash buckets */
    for (i = 0; i < NUM_KEY_HASH_BUCKETS; i++)
    {
        /* Initialize the mutex and the wait lists */
        ExInitializePushLock(&KeyedEvent->HashTable[i].Lock);
        InitializeListHead(&KeyedEvent->HashTable[i].WaitListHead);
        InitializeListHead(&KeyedEvent->HashTable[i].ReleaseListHead);
    }
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NTAPI
ExpReleaseOrWaitForKeyedEvent(
    _Inout_ PEX_KEYED_EVENT KeyedEvent,
    _In_ PVOID KeyedWaitValue,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ BOOLEAN Release)
{
    PETHREAD Thread, CurrentThread;
    PEPROCESS CurrentProcess;
    PLIST_ENTRY ListEntry, WaitListHead1, WaitListHead2;
    NTSTATUS Status;
    ULONG_PTR HashIndex;
    PVOID PreviousKeyedWaitValue;

    /* Get the current process */
    CurrentProcess = PsGetCurrentProcess();

    /* Calculate the hash index */
    HashIndex = (ULONG_PTR)KeyedWaitValue >> 5;
    HashIndex ^= (ULONG_PTR)CurrentProcess >> 6;
    HashIndex %= NUM_KEY_HASH_BUCKETS;

    /* Lock the lists */
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&KeyedEvent->HashTable[HashIndex].Lock);

    /* Get the lists for search and wait, depending on whether
       we want to wait for the event or signal it */
    if (Release)
    {
        WaitListHead1 = &KeyedEvent->HashTable[HashIndex].WaitListHead;
        WaitListHead2 = &KeyedEvent->HashTable[HashIndex].ReleaseListHead;
    }
    else
    {
        WaitListHead1 = &KeyedEvent->HashTable[HashIndex].ReleaseListHead;
        WaitListHead2 = &KeyedEvent->HashTable[HashIndex].WaitListHead;
    }

    /* loop the first wait list */
    ListEntry = WaitListHead1->Flink;
    while (ListEntry != WaitListHead1)
    {
        /* Get the waiting thread. Note that this thread cannot be terminated
           as long as we hold the list lock, since it either needs to wait to
           be signaled by this thread or, when the wait is aborted due to thread
           termination, then it first needs to acquire the list lock. */
        Thread = CONTAINING_RECORD(ListEntry, ETHREAD, KeyedWaitChain);
        ListEntry = ListEntry->Flink;

        /* Check if this thread is a correct waiter */
        if ((Thread->Tcb.Process == &CurrentProcess->Pcb) &&
            (Thread->KeyedWaitValue == KeyedWaitValue))
        {
            /* Remove the thread from the list */
            RemoveEntryList(&Thread->KeyedWaitChain);

            /* Initialize the list entry to show that it was removed */
            InitializeListHead(&Thread->KeyedWaitChain);

            /* Wake the thread */
            KeReleaseSemaphore(&Thread->KeyedWaitSemaphore,
                               IO_NO_INCREMENT,
                               1,
                               FALSE);
            Thread = NULL;

            /* Unlock the list. After this it is not safe to access Thread */
            ExReleasePushLockExclusive(&KeyedEvent->HashTable[HashIndex].Lock);
            KeLeaveCriticalRegion();

            return STATUS_SUCCESS;
        }
    }

    /* Get the current thread */
    CurrentThread = PsGetCurrentThread();

    /* Set the wait key and remember the old value */
    PreviousKeyedWaitValue = CurrentThread->KeyedWaitValue;
    CurrentThread->KeyedWaitValue = KeyedWaitValue;

    /* Initialize the wait semaphore */
    KeInitializeSemaphore(&CurrentThread->KeyedWaitSemaphore, 0, 1);

    /* Insert the current thread into the secondary wait list */
    InsertTailList(WaitListHead2, &CurrentThread->KeyedWaitChain);

    /* Unlock the list */
    ExReleasePushLockExclusive(&KeyedEvent->HashTable[HashIndex].Lock);
    KeLeaveCriticalRegion();

    /* Wait for the keyed wait semaphore */
    Status = KeWaitForSingleObject(&CurrentThread->KeyedWaitSemaphore,
                                   WrKeyedEvent,
                                   KernelMode,
                                   Alertable,
                                   Timeout);

    /* Check if the wait was aborted or timed out */
    if (Status != STATUS_SUCCESS)
    {
        /* Lock the lists to make sure no one else messes with the entry */
        KeEnterCriticalRegion();
        ExAcquirePushLockExclusive(&KeyedEvent->HashTable[HashIndex].Lock);

        /* Check if the wait list entry is still in the list */
        if (!IsListEmpty(&CurrentThread->KeyedWaitChain))
        {
            /* Remove the thread from the list */
            RemoveEntryList(&CurrentThread->KeyedWaitChain);
            InitializeListHead(&CurrentThread->KeyedWaitChain);
        }

        /* Unlock the list */
        ExReleasePushLockExclusive(&KeyedEvent->HashTable[HashIndex].Lock);
        KeLeaveCriticalRegion();
    }

    /* Restore the previous KeyedWaitValue, since this is a union member */
    CurrentThread->KeyedWaitValue = PreviousKeyedWaitValue;

    return Status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NTAPI
ExpWaitForKeyedEvent(
    _Inout_ PEX_KEYED_EVENT KeyedEvent,
    _In_ PVOID KeyedWaitValue,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout)
{
    /* Call the generic internal function */
    return ExpReleaseOrWaitForKeyedEvent(KeyedEvent,
                                         KeyedWaitValue,
                                         Alertable,
                                         Timeout,
                                         FALSE);
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NTAPI
ExpReleaseKeyedEvent(
    _Inout_ PEX_KEYED_EVENT KeyedEvent,
    _In_ PVOID KeyedWaitValue,
    _In_ BOOLEAN Alertable,
    _In_ PLARGE_INTEGER Timeout)
{
    /* Call the generic internal function */
    return ExpReleaseOrWaitForKeyedEvent(KeyedEvent,
                                         KeyedWaitValue,
                                         Alertable,
                                         Timeout,
                                         TRUE);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
NtCreateKeyedEvent(
    _Out_ PHANDLE OutHandle,
    _In_ ACCESS_MASK AccessMask,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Flags)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PEX_KEYED_EVENT KeyedEvent;
    HANDLE KeyedEventHandle;
    NTSTATUS Status;

    /* Check flags */
    if (Flags != 0)
    {
        /* We don't support any flags yet */
        return STATUS_INVALID_PARAMETER;
    }

    /* Create the object */
    Status = ObCreateObject(PreviousMode,
                            ExKeyedEventObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(EX_KEYED_EVENT),
                            0,
                            0,
                            (PVOID*)&KeyedEvent);

    /* Check for success */
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the keyed event */
    ExpInitializeKeyedEvent(KeyedEvent);

    /* Insert it */
    Status = ObInsertObject(KeyedEvent,
                            NULL,
                            AccessMask,
                            0,
                            NULL,
                            &KeyedEventHandle);

    /* Check for success (ObInsertObject dereferences!) */
    if (!NT_SUCCESS(Status)) return Status;

    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for return */
        _SEH2_TRY
        {
            /* Return the handle to the caller */
            ProbeForWrite(OutHandle, sizeof(HANDLE), sizeof(HANDLE));
            *OutHandle = KeyedEventHandle;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();

            /* Cleanup */
            ObCloseHandle(KeyedEventHandle, PreviousMode);
        }
        _SEH2_END;
    }
    else
    {
        *OutHandle = KeyedEventHandle;
    }

    /* Return Status */
    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
NtOpenKeyedEvent(
    _Out_ PHANDLE OutHandle,
    _In_ ACCESS_MASK AccessMask,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    HANDLE KeyedEventHandle;
    NTSTATUS Status;

    /* Open the object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExKeyedEventObjectType,
                                PreviousMode,
                                NULL,
                                AccessMask,
                                NULL,
                                &KeyedEventHandle);

    /* Check for success */
    if (!NT_SUCCESS(Status)) return Status;

    /* Enter SEH for return */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Return the handle to the caller */
            ProbeForWrite(OutHandle, sizeof(HANDLE), sizeof(HANDLE));
            *OutHandle = KeyedEventHandle;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();

            /* Cleanup */
            ObCloseHandle(KeyedEventHandle, PreviousMode);
        }
        _SEH2_END;
    }
    else
    {
        *OutHandle = KeyedEventHandle;
    }

    /* Return status */
    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
NtWaitForKeyedEvent(
    _In_opt_ HANDLE Handle,
    _In_ PVOID Key,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PEX_KEYED_EVENT KeyedEvent;
    NTSTATUS Status;
    LARGE_INTEGER TimeoutCopy;

    /* Key must always be two-byte aligned */
    if ((ULONG_PTR)Key & 1)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Check if the caller passed a timeout value and this is from user mode */
    if ((Timeout != NULL) && (PreviousMode != KernelMode))
    {
        _SEH2_TRY
        {
            ProbeForRead(Timeout, sizeof(*Timeout), 1);
            TimeoutCopy = *Timeout;
            Timeout = &TimeoutCopy;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Check if the caller provided a handle */
    if (Handle != NULL)
    {
        /* Get the keyed event object */
        Status = ObReferenceObjectByHandle(Handle,
                                           KEYEDEVENT_WAIT,
                                           ExKeyedEventObjectType,
                                           PreviousMode,
                                           (PVOID*)&KeyedEvent,
                                           NULL);

        /* Check for success */
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Use the default keyed event for low memory critical sections */
        KeyedEvent = ExpCritSecOutOfMemoryEvent;
    }

    /* Do the wait */
    Status = ExpWaitForKeyedEvent(KeyedEvent, Key, Alertable, Timeout);

    if (Handle != NULL)
    {
        /* Dereference the keyed event */
        ObDereferenceObject(KeyedEvent);
    }

    /* Return the status */
    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
NtReleaseKeyedEvent(
    _In_opt_ HANDLE Handle,
    _In_ PVOID Key,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PEX_KEYED_EVENT KeyedEvent;
    NTSTATUS Status;
    LARGE_INTEGER TimeoutCopy;

    /* Key must always be two-byte aligned */
    if ((ULONG_PTR)Key & 1)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Check if the caller passed a timeout value and this is from user mode */
    if ((Timeout != NULL) && (PreviousMode != KernelMode))
    {
        _SEH2_TRY
        {
            ProbeForRead(Timeout, sizeof(*Timeout), 1);
            TimeoutCopy = *Timeout;
            Timeout = &TimeoutCopy;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Check if the caller provided a handle */
    if (Handle != NULL)
    {
        /* Get the keyed event object */
        Status = ObReferenceObjectByHandle(Handle,
                                           KEYEDEVENT_WAKE,
                                           ExKeyedEventObjectType,
                                           PreviousMode,
                                           (PVOID*)&KeyedEvent,
                                           NULL);

        /* Check for success */
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Use the default keyed event for low memory critical sections */
        KeyedEvent = ExpCritSecOutOfMemoryEvent;
    }

    /* Do the wait */
    Status = ExpReleaseKeyedEvent(KeyedEvent, Key, Alertable, Timeout);

    if (Handle != NULL)
    {
        /* Dereference the keyed event */
        ObDereferenceObject(KeyedEvent);
    }

    /* Return the status */
    return Status;
}

/* EOF */
