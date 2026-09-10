/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/wait.c
 * PURPOSE:         Wait-on-address functions
 * PROGRAMMER:      Ahmed ARIF <arif.ing@outlook.com>
 * UPDATE HISTORY:
 *                  Created 29/07/26
 */

#include <ntdll.h>

#define NDEBUG
#include <debug.h>

typedef struct _RTL_WAIT_ON_ADDRESS_ENTRY
{
    LIST_ENTRY ListEntry;
    const VOID *Address;
    HANDLE EventHandle;
    BOOLEAN Removed;
} RTL_WAIT_ON_ADDRESS_ENTRY, *PRTL_WAIT_ON_ADDRESS_ENTRY;

static RTL_SRWLOCK RtlpWaitOnAddressLock = RTL_SRWLOCK_INIT;
static LIST_ENTRY RtlpWaitOnAddressList =
{
    &RtlpWaitOnAddressList,
    &RtlpWaitOnAddressList
};

static
BOOLEAN
RtlpIsValidWaitOnAddressSize(SIZE_T Size)
{
    return (Size == 1 || Size == 2 || Size == 4 || Size == 8);
}

static
BOOLEAN
RtlpWaitOnAddressMatches(const VOID *Address,
                         const VOID *CompareAddress,
                         SIZE_T Size)
{
    return RtlCompareMemory(Address, CompareAddress, Size) == Size;
}

/*
 * @implemented
 */
NTSTATUS
WINAPI
RtlWaitOnAddress(const VOID *Address,
                 const VOID *CompareAddress,
                 SIZE_T AddressSize,
                 const LARGE_INTEGER *Timeout)
{
    RTL_WAIT_ON_ADDRESS_ENTRY Entry;
    NTSTATUS Status;

    if (!RtlpIsValidWaitOnAddressSize(AddressSize))
        return STATUS_INVALID_PARAMETER;

    Status = NtCreateEvent(&Entry.EventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    Entry.Address = Address;
    Entry.Removed = FALSE;

    RtlAcquireSRWLockExclusive(&RtlpWaitOnAddressLock);

    if (!RtlpWaitOnAddressMatches(Address, CompareAddress, AddressSize))
    {
        RtlReleaseSRWLockExclusive(&RtlpWaitOnAddressLock);
        NtClose(Entry.EventHandle);
        return STATUS_SUCCESS;
    }

    InsertTailList(&RtlpWaitOnAddressList, &Entry.ListEntry);

    RtlReleaseSRWLockExclusive(&RtlpWaitOnAddressLock);

    Status = NtWaitForSingleObject(Entry.EventHandle, FALSE, (PLARGE_INTEGER)Timeout);

    RtlAcquireSRWLockExclusive(&RtlpWaitOnAddressLock);

    if (!Entry.Removed)
    {
        RemoveEntryList(&Entry.ListEntry);
        Entry.Removed = TRUE;
    }

    RtlReleaseSRWLockExclusive(&RtlpWaitOnAddressLock);

    NtClose(Entry.EventHandle);
    return Status;
}

static
VOID
RtlpWakeAddress(const VOID *Address,
                BOOLEAN WakeAll)
{
    PLIST_ENTRY Current;

    if (!Address)
        return;

    RtlAcquireSRWLockExclusive(&RtlpWaitOnAddressLock);

    Current = RtlpWaitOnAddressList.Flink;
    while (Current != &RtlpWaitOnAddressList)
    {
        PRTL_WAIT_ON_ADDRESS_ENTRY Entry;
        PLIST_ENTRY Next;
        NTSTATUS Status;

        Entry = CONTAINING_RECORD(Current, RTL_WAIT_ON_ADDRESS_ENTRY, ListEntry);
        Next = Current->Flink;

        if (Entry->Address == Address)
        {
            Status = NtSetEvent(Entry->EventHandle, NULL);
            if (NT_SUCCESS(Status))
            {
                RemoveEntryList(&Entry->ListEntry);
                Entry->Removed = TRUE;

                if (!WakeAll)
                    break;
            }
        }

        Current = Next;
    }

    RtlReleaseSRWLockExclusive(&RtlpWaitOnAddressLock);
}

/*
 * @implemented
 */
VOID
WINAPI
RtlWakeAddressAll(const VOID *Address)
{
    RtlpWakeAddress(Address, TRUE);
}

/*
 * @implemented
 */
VOID
WINAPI
RtlWakeAddressSingle(const VOID *Address)
{
    RtlpWakeAddress(Address, FALSE);
}
