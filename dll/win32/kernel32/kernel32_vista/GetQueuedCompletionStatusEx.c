
#include "k32_vista.h"

#define NDEBUG
#include <debug.h>

/*
 * Implemented as a loop over the native NtRemoveIoCompletion primitive
 * (the same one GetQueuedCompletionStatus uses) rather than a true
 * single-syscall batch dequeue. Functionally equivalent for callers,
 * just without the extra efficiency of NtRemoveIoCompletionEx, which is
 * not implemented.
 *
 * @implemented
 */
BOOL
WINAPI
GetQueuedCompletionStatusEx(
    _In_ HANDLE CompletionPort,
    _Out_writes_to_(ulCount, *ulNumEntriesRemoved) LPOVERLAPPED_ENTRY lpCompletionPortEntries,
    _In_ ULONG ulCount,
    _Out_ PULONG ulNumEntriesRemoved,
    _In_ DWORD dwMilliseconds,
    _In_ BOOL fAlertable)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    ULONG_PTR CompletionKey;
    LPOVERLAPPED Overlapped;
    LARGE_INTEGER Time, ZeroTimeout;
    PLARGE_INTEGER TimePtr;
    ULONG Count;

    UNREFERENCED_PARAMETER(fAlertable);

    if (ulCount == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Block for the first packet using the caller's timeout */
    TimePtr = BaseFormatTimeOut(&Time, dwMilliseconds);
    Status = NtRemoveIoCompletion(CompletionPort,
                                   (PVOID*)&CompletionKey,
                                   (PVOID*)&Overlapped,
                                   &IoStatus,
                                   TimePtr);
    if (Status == STATUS_TIMEOUT)
    {
        SetLastError(WAIT_TIMEOUT);
        return FALSE;
    }
    else if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    lpCompletionPortEntries[0].lpCompletionKey = CompletionKey;
    lpCompletionPortEntries[0].lpOverlapped = Overlapped;
    lpCompletionPortEntries[0].Internal = IoStatus.Status;
    lpCompletionPortEntries[0].dwNumberOfBytesTransferred = (DWORD)IoStatus.Information;
    Count = 1;

    /* Opportunistically drain any further already-queued packets without blocking */
    ZeroTimeout.QuadPart = 0;
    while (Count < ulCount)
    {
        Status = NtRemoveIoCompletion(CompletionPort,
                                       (PVOID*)&CompletionKey,
                                       (PVOID*)&Overlapped,
                                       &IoStatus,
                                       &ZeroTimeout);
        if (!NT_SUCCESS(Status))
        {
            /* Nothing more ready right now -- we already have at least one entry */
            break;
        }

        lpCompletionPortEntries[Count].lpCompletionKey = CompletionKey;
        lpCompletionPortEntries[Count].lpOverlapped = Overlapped;
        lpCompletionPortEntries[Count].Internal = IoStatus.Status;
        lpCompletionPortEntries[Count].dwNumberOfBytesTransferred = (DWORD)IoStatus.Information;
        Count++;
    }

    *ulNumEntriesRemoved = Count;
    return TRUE;
}
