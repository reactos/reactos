/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/lock.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */


/* INCLUDES ****************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
LockFile(IN HANDLE hFile,
         IN DWORD dwFileOffsetLow,
         IN DWORD dwFileOffsetHigh,
         IN DWORD nNumberOfBytesToLockLow,
         IN DWORD nNumberOfBytesToLockHigh)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    LARGE_INTEGER BytesToLock, Offset;

    /* Is this a console handle? */
    if (IsConsoleHandle(hFile))
    {
        /* Can't "lock" a console! */
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
    }

    /* Setup the parameters in NT style and call the native API */
    BytesToLock.u.LowPart = nNumberOfBytesToLockLow;
    BytesToLock.u.HighPart = nNumberOfBytesToLockHigh;
    Offset.u.LowPart = dwFileOffsetLow;
    Offset.u.HighPart = dwFileOffsetHigh;
    Status = NtLockFile(hFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        &Offset,
                        &BytesToLock,
                        0,
                        TRUE,
                        TRUE);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion if needed */
        Status = NtWaitForSingleObject(hFile, FALSE, NULL);
        if (NT_SUCCESS(Status)) Status = IoStatusBlock.Status;
    }

    /* Check if we failed */
    if (!NT_SUCCESS(Status))
    {
        /* Convert the error code and fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Success! */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
LockFileEx(IN HANDLE hFile,
           IN DWORD dwFlags,
           IN DWORD dwReserved,
           IN DWORD nNumberOfBytesToLockLow,
           IN DWORD nNumberOfBytesToLockHigh,
           IN LPOVERLAPPED lpOverlapped)
{
    LARGE_INTEGER BytesToLock, Offset;
    NTSTATUS Status;

    /* Is this a console handle? */
    if (IsConsoleHandle(hFile))
    {
        /* Can't "lock" a console! */
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
    }

    /* This parameter should be zero */
    if (dwReserved)
    {
        /* Fail since it isn't */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Set the initial status in the IO_STATUS_BLOCK to pending... */
    lpOverlapped->Internal = STATUS_PENDING;

    /* Convert the parameters to NT format and call the native API */
    Offset.u.LowPart = lpOverlapped->Offset;
    Offset.u.HighPart = lpOverlapped->OffsetHigh;
    BytesToLock.u.LowPart = nNumberOfBytesToLockLow;
    BytesToLock.u.HighPart = nNumberOfBytesToLockHigh;
    Status = NtLockFile(hFile,
                        lpOverlapped->hEvent,
                        NULL,
                        NULL,
                        (PIO_STATUS_BLOCK)lpOverlapped,
                        &Offset,
                        &BytesToLock,
                        0,
                        dwFlags & LOCKFILE_FAIL_IMMEDIATELY ? TRUE : FALSE,
                        dwFlags & LOCKFILE_EXCLUSIVE_LOCK ? TRUE: FALSE);
    if ((NT_SUCCESS(Status)) && (Status != STATUS_PENDING))
    {
        /* Pending status is *not* allowed in the Ex API */
        return TRUE;
    }

    /* Convert the error code and fail */
    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
UnlockFile(IN HANDLE hFile,
           IN DWORD dwFileOffsetLow,
           IN DWORD dwFileOffsetHigh,
           IN DWORD nNumberOfBytesToUnlockLow,
           IN DWORD nNumberOfBytesToUnlockHigh)
{
    OVERLAPPED Overlapped;
    NTSTATUS Status;
    BOOLEAN Result;

    /* Convert parameters to Ex format and call the new API */
    Overlapped.Offset = dwFileOffsetLow;
    Overlapped.OffsetHigh = dwFileOffsetHigh;
    Result = UnlockFileEx(hFile,
                          0,
                          nNumberOfBytesToUnlockLow,
                          nNumberOfBytesToUnlockHigh,
                          &Overlapped);
    if (!(Result) && (GetLastError() == ERROR_IO_PENDING))
    {
        /* Ex fails during STATUS_PENDING, handle that here by waiting */
        Status = NtWaitForSingleObject(hFile, FALSE, NULL);
        if (NT_SUCCESS(Status)) Status = Overlapped.Internal;

        /* Now if the status is successful, return */
        if (!NT_SUCCESS(Status)) return TRUE;

        /* Otherwise the asynchronous operation had a failure, so fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Success or error case -- Ex took care of the rest, just return */
    return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
UnlockFileEx(IN HANDLE hFile,
             IN DWORD dwReserved,
             IN DWORD nNumberOfBytesToUnLockLow,
             IN DWORD nNumberOfBytesToUnLockHigh,
             IN LPOVERLAPPED lpOverlapped)
{
    LARGE_INTEGER BytesToUnLock, StartAddress;
    NTSTATUS Status;

    /* Is this a console handle? */
    if (IsConsoleHandle(hFile))
    {
        /* Can't "unlock" a console! */
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
    }

    /* This parameter should be zero */
    if (dwReserved)
    {
        /* Fail since it isn't */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Convert to NT format and call the native function */
    BytesToUnLock.u.LowPart = nNumberOfBytesToUnLockLow;
    BytesToUnLock.u.HighPart = nNumberOfBytesToUnLockHigh;
    StartAddress.u.LowPart = lpOverlapped->Offset;
    StartAddress.u.HighPart = lpOverlapped->OffsetHigh;
    Status = NtUnlockFile(hFile,
                          (PIO_STATUS_BLOCK)lpOverlapped,
                          &StartAddress,
                          &BytesToUnLock,
                          0);
    if (!NT_SUCCESS(Status))
    {
        /* Convert the error and fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* All good */
    return TRUE;
}

/* EOF */
