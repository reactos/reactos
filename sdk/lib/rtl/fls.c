/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of fiber local storage (FLS) functions
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <rtl.h>

#define NDEBUG
#include <debug.h>

//
// References:
// - https://devblogs.microsoft.com/oldnewthing/20191011-00?p=102989
// - https://dronesec.net/blog/code-execution-via-fiber-local-storage/
// - https://wine-devel.winehq.narkive.com/63970k7i/implementing-fiber-local-storage-callbacks-sharing-data-between-dlls
// - https://github.com/ElliotKillick/operating-system-design-review/blob/main/code/windows/fls-concurrency/fls-experiment.c
// - https://ntquery.wordpress.com/2014/03/29/anti-debug-fiber-local-storage-fls/
//

/* The FLS lock. We don't use it yet since kernel32 uses the PEB lock and we need to synchronize with this for now */
//RTL_SRWLOCK RtlpFlsLock = RTL_SRWLOCK_INIT;

typedef struct _FLS_CALLBACK_INFO
{
    PFLS_CALLBACK_FUNCTION Callback;
    PVOID Unknown;
} FLS_CALLBACK_INFO, *PFLS_CALLBACK_INFO;

static
VOID
RtlpAcquireFlsLock(VOID)
{
    /* Acquire the lock */
    //RtlAcquireSRWLockExclusive(&RtlpFlsLock);
    RtlAcquirePebLock();
}

static
VOID
RtlpReleaseFlsLock(VOID)
{
    /* Release the lock */
    //RtlReleaseSRWLockExclusive(&RtlpFlsLock);
    RtlReleasePebLock();
}

static
NTSTATUS
RtlpAllocateFlsCallbackArray(VOID)
{
    PPEB peb = NtCurrentPeb();
    PVOID* newCallbacks;

    /* Allocate a new callback array */
    newCallbacks = RtlAllocateHeap(RtlGetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   RTL_FLS_MAXIMUM_AVAILABLE * sizeof(FLS_CALLBACK_INFO));
    if (newCallbacks == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    RtlpAcquireFlsLock();

    if (peb->FlsCallback == NULL)
    {
        peb->FlsCallback = newCallbacks;
        newCallbacks = NULL;
    }

    RtlpReleaseFlsLock();

    /* Clean up, if needed */
    if (newCallbacks != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, newCallbacks);
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
RtlpAllocateThreadFlsData(VOID)
{
    PTEB teb = NtCurrentTeb();
    PRTL_FLS_DATA newFlsData;

    ASSERT(teb->FlsData == NULL);

    /* Allocate a new thread FLS data structure */
    newFlsData = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 sizeof(RTL_FLS_DATA));
    if (newFlsData == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    PPEB peb = NtCurrentPeb();

    RtlpAcquireFlsLock();
    InsertTailList(&peb->FlsListHead, &newFlsData->ListEntry);
    RtlpReleaseFlsLock();

    teb->FlsData = newFlsData;

    return STATUS_SUCCESS;
}

// Windows 11
NTSTATUS
NTAPI
RtlFlsAllocEx(
    _In_ PFLS_CALLBACK_FUNCTION Callback,
    _Out_opt_ PULONG Unknown,
    _Out_ PULONG FlsIndex)
{
    PPEB peb = NtCurrentPeb();
    PTEB teb = NtCurrentTeb();
    NTSTATUS status;
    ULONG index;

    /* Check if we have a thread FLS data structure */
    if (teb->FlsData == NULL)
    {
        /* Allocate a new thread FLS data structure */
        status = RtlpAllocateThreadFlsData();
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    /* Check if we need to allocate the callback array */
    if (peb->FlsCallback == NULL)
    {
        status = RtlpAllocateFlsCallbackArray();
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    RtlpAcquireFlsLock();

    /* Find a free index */
    index = RtlFindClearBitsAndSet(peb->FlsBitmap, 1, 0);
    if (index == -1)
    {
        RtlpReleaseFlsLock();
        return STATUS_NO_MEMORY;
    }

    /* Set the callback */
    PFLS_CALLBACK_INFO callbacks = (PFLS_CALLBACK_INFO)peb->FlsCallback;
    callbacks[index].Callback = Callback;

    /* Update high index */
    peb->FlsHighIndex = max(peb->FlsHighIndex, index);

    RtlpReleaseFlsLock();

    /* Return the index */
    *FlsIndex = index;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlFlsAlloc(
    _In_ PFLS_CALLBACK_FUNCTION Callback,
    _Out_ PULONG FlsIndex)
{
    return RtlFlsAllocEx(Callback, NULL, FlsIndex);
}

NTSTATUS
NTAPI
RtlFlsFree(
    _In_ ULONG FlsIndex)
{
    PPEB peb = NtCurrentPeb();
    PRTL_BITMAP flsBitmap = peb->FlsBitmap;

    /* Check if the FLS index is within the valid range */
    if ((FlsIndex == 0) || (FlsIndex >= FLS_MAXIMUM_AVAILABLE))
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlpAcquireFlsLock();

    /* Validate that the bit is set */
    if (!RtlCheckBit(flsBitmap, FlsIndex))
    {
        RtlpReleaseFlsLock();
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if there is a callback */
    PFLS_CALLBACK_INFO callbackArray = (PFLS_CALLBACK_INFO)peb->FlsCallback;
    PFLS_CALLBACK_FUNCTION callbackFunction = callbackArray[FlsIndex].Callback;

    /* Clear the FLS index bit */
    RtlClearBit(peb->FlsBitmap, FlsIndex);

    /* Protect with SEH to guard against a broken callback.
       Note: Vista doesn't do that and will leak the FLS lock!
       Windows 10 protects the lock and still manages to clear the values as well.
       We replicate Windows 10 behavior. */
    _SEH2_VOLATILE PLIST_ENTRY listEntry;
    _SEH2_TRY
    {
        /* Loop all thread FLS data structures */
        for (listEntry = peb->FlsListHead.Flink;
             listEntry != &peb->FlsListHead;
             listEntry = listEntry->Flink)
        {
            /* Get the thread FLS data structure */
            PRTL_FLS_DATA flsData = CONTAINING_RECORD(listEntry, RTL_FLS_DATA, ListEntry);

            /* Check if the value is set (!= NULL) */
            if (flsData->Data[FlsIndex] != NULL)
            {
                if (callbackFunction != NULL)
                {
                    callbackFunction(flsData->Data[FlsIndex]);
                }

                flsData->Data[FlsIndex] = NULL;
            }
        }

        /* Clear the callback */
        callbackArray[FlsIndex].Callback = NULL;
    }
    _SEH2_FINALLY
    {
        /* Check if we got an exception */
        if (_SEH2_AbnormalTermination())
        {
            /* We need to finish clearing the values */
            for (/* the current entry */;
                 listEntry != &peb->FlsListHead;
                 listEntry = listEntry->Flink)
            {
                /* Get the thread FLS data structure and clear the value */
                PRTL_FLS_DATA flsData = CONTAINING_RECORD(listEntry, RTL_FLS_DATA, ListEntry);
                flsData->Data[FlsIndex] = NULL;
            }

            /* Clear the callback */
            callbackArray[FlsIndex].Callback = NULL;
        }

        RtlpReleaseFlsLock();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

// This function has different signatures between Vista and Windows 10
// See https://debugactiveprocess.medium.com/rtlprocessflsdata-as-anti-debugging-technique-c531174c6dc8
// and https://ntquery.wordpress.com/2014/03/29/anti-debug-fiber-local-storage-fls/#more-18
#if (DLL_EXPORT_VERSION < _WIN32_WINNT_WIN10)
NTSTATUS
NTAPI
RtlProcessFlsData(
    _In_ PRTL_FLS_DATA FlsData)
{
    return STATUS_NOT_IMPLEMENTED;
}
#else
NTSTATUS
NTAPI
RtlProcessFlsData(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* FlsData)
{
    return STATUS_NOT_IMPLEMENTED;
}
#endif

#if (DLL_EXPORT_VERSION >= _WIN32_WINNT_WIN10)

// Windows 10
NTSTATUS
NTAPI
RtlFlsGetValue(
    _In_ ULONG FlsIndex,
    _Out_ PVOID* FlsData)
{
    /* Check if the FLS index is within the valid range */
    if ((FlsIndex == 0) || (FlsIndex >= FLS_MAXIMUM_AVAILABLE))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we have a thread FLS data structure */
    PTEB teb = NtCurrentTeb();
    if (teb->FlsData == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Return the value (the function doesnt check the bitmap!) */
    PRTL_FLS_DATA flsData = teb->FlsData;
    *FlsData = flsData->Data[FlsIndex];

    return STATUS_SUCCESS;
}

// Windows 11
PVOID
WINAPI
RtlFlsGetValue2(
    _In_ ULONG FlsIndex)
{
    NTSTATUS status;
    PVOID flsData;

    status = RtlFlsGetValue(FlsIndex, &flsData);
    if (!NT_SUCCESS(status))
    {
        return NULL;
    }

    return flsData;
}

// Windows 10
NTSTATUS
NTAPI
RtlFlsSetValue(
    _In_ ULONG FlsIndex,
    _In_ PVOID FlsData)
{
    NTSTATUS status;

    /* Check if the FLS index is within the valid range */
    if ((FlsIndex == 0) || (FlsIndex >= FLS_MAXIMUM_AVAILABLE))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we have a thread FLS data structure */
    PTEB teb = NtCurrentTeb();
    if (teb->FlsData == NULL)
    {
        /* Allocate a new thread FLS data structure */
        status = RtlpAllocateThreadFlsData();
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    /* Set the value (the function doesnt check the bitmap!) */
    PRTL_FLS_DATA flsData = teb->FlsData;
    flsData->Data[FlsIndex] = FlsData;

    return STATUS_SUCCESS;
}

#endif // (DLL_EXPORT_VERSION >= DLL_EXPORT_VERSION_WIN10)
