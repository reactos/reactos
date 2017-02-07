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

VOID
NTAPI
ExpInitializeKeyedEvent(
    _Out_ PEX_KEYED_EVENT KeyedEvent);

#define KeGetCurrentProcess() ((PKPROCESS)PsGetCurrentProcess())

/* GLOBALS *******************************************************************/

PEX_KEYED_EVENT ExpCritSecOutOfMemoryEvent;
POBJECT_TYPE ExKeyedEventObjectType;

static
GENERIC_MAPPING ExpKeyedEventMapping =
{
    STANDARD_RIGHTS_READ | EVENT_QUERY_STATE,
    STANDARD_RIGHTS_WRITE | EVENT_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | EVENT_QUERY_STATE,
    EVENT_ALL_ACCESS
};


/* FUNCTIONS *****************************************************************/

VOID
NTAPI
ExpInitializeKeyedEventImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer = {0};
    UNICODE_STRING TypeName = RTL_CONSTANT_STRING(L"KeyedEvent");
    NTSTATUS Status;

    /* Set up the object type initializer */
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.GenericMapping = ExpKeyedEventMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    //ObjectTypeInitializer.DeleteProcedure = ???;
    //ObjectTypeInitializer.OkayToCloseProcedure = ???;

    /* Create the keyed event object type */
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                NULL,
                                &ExKeyedEventObjectType);

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        // FIXME
        KeBugCheck(0);
    }

    /* Create the global keyed event for critical sections on low memory */
    Status = ObCreateObject(KernelMode,
                            ExKeyedEventObjectType,
                            NULL,
                            UserMode,
                            NULL,
                            sizeof(EX_KEYED_EVENT),
                            0,
                            0,
                            (PVOID*)&ExpCritSecOutOfMemoryEvent);

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        // FIXME
        KeBugCheck(0);
    }

    /* Initalize the keyed event */
    ExpInitializeKeyedEvent(ExpCritSecOutOfMemoryEvent);
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

NTSTATUS
NTAPI
ExpReleaseOrWaitForKeyedEvent(
    _Inout_ PEX_KEYED_EVENT KeyedEvent,
    _In_ PVOID KeyedWaitValue,
    _In_ BOOLEAN Alertable,
    _In_ PLARGE_INTEGER Timeout,
    _In_ BOOLEAN Release)
{
    PETHREAD Thread, CurrentThread;
    PKPROCESS CurrentProcess;
    PLIST_ENTRY ListEntry, WaitListHead1, WaitListHead2;
    NTSTATUS Status;
    ULONG_PTR HashIndex;

    /* Get the current process */
    CurrentProcess = KeGetCurrentProcess();

    /* Calculate the hash index */
    HashIndex = (ULONG_PTR)KeyedWaitValue >> 5;
    HashIndex ^= (ULONG_PTR)CurrentProcess >> 6;
    HashIndex %= NUM_KEY_HASH_BUCKETS;

    /* Lock the lists */
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
        Thread = CONTAINING_RECORD(ListEntry, ETHREAD, KeyedWaitChain);

        /* Check if this thread is a correct waiter */
        if ((Thread->Tcb.Process == CurrentProcess) &&
            (Thread->KeyedWaitValue == KeyedWaitValue))
        {
            /* Remove the thread from the list */
            RemoveEntryList(&Thread->KeyedWaitChain);

            /* Wake the thread */
            KeReleaseSemaphore(&Thread->KeyedWaitSemaphore, 0, 1, FALSE);

            /* Unlock the lists */
            ExReleasePushLockExclusive(&KeyedEvent->HashTable[HashIndex].Lock);

            return STATUS_SUCCESS;
        }
    }

    /* Get the current thread */
    CurrentThread = PsGetCurrentThread();

    /* Set the wait key */
    CurrentThread->KeyedWaitValue = KeyedWaitValue;

    /* Initialize the wait semaphore */
    KeInitializeSemaphore(&CurrentThread->KeyedWaitSemaphore, 0, 1);

    /* Insert the current thread into the secondary wait list */
    InsertTailList(WaitListHead2, &CurrentThread->KeyedWaitChain);

    /* Unlock the lists */
    ExReleasePushLockExclusive(&KeyedEvent->HashTable[HashIndex].Lock);

    /* Wait for the keyed wait semaphore */
    Status = KeWaitForSingleObject(&CurrentThread->KeyedWaitSemaphore,
                                   WrKeyedEvent,
                                   KernelMode,
                                   Alertable,
                                   Timeout);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ExpWaitForKeyedEvent(
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
                                         FALSE);
}

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

    /* Initalize the keyed event */
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

NTSTATUS
NTAPI
NtWaitForKeyedEvent(
    _In_ HANDLE Handle,
    _In_ PVOID Key,
    _In_ BOOLEAN Alertable,
    _In_ PLARGE_INTEGER Timeout)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PEX_KEYED_EVENT KeyedEvent;
    NTSTATUS Status;

    /* Check if the caller provided a handle */
    if (Handle != NULL)
    {
        /* Get the keyed event object */
        Status = ObReferenceObjectByHandle(Handle,
                                           EVENT_MODIFY_STATE,
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

    /* Dereference the keyed event */
    ObDereferenceObject(KeyedEvent);

    /* Return the status */
    return Status;
}

NTSTATUS
NTAPI
NtReleaseKeyedEvent(
    _In_ HANDLE Handle,
    _In_ PVOID Key,
    _In_ BOOLEAN Alertable,
    _In_ PLARGE_INTEGER Timeout)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PEX_KEYED_EVENT KeyedEvent;
    NTSTATUS Status;

    /* Check if the caller provided a handle */
    if (Handle != NULL)
    {
        /* Get the keyed event object */
        Status = ObReferenceObjectByHandle(Handle,
                                           EVENT_MODIFY_STATE,
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

    /* Dereference the keyed event */
    ObDereferenceObject(KeyedEvent);

    /* Return the status */
    return Status;
}

/* EOF */
