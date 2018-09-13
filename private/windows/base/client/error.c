/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    error.c

Abstract:

    This module contains the Win32 error APIs.

Author:

    Mark Lucovsky (markl) 24-Sep-1990

Revision History:

--*/

#include "basedll.h"

UINT
GetErrorMode()
{

    UINT PreviousMode;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(
                NtCurrentProcess(),
                ProcessDefaultHardErrorMode,
                (PVOID) &PreviousMode,
                sizeof(PreviousMode),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return 0;
        }

    if (PreviousMode & 1) {
        PreviousMode &= ~SEM_FAILCRITICALERRORS;
        }
    else {
        PreviousMode |= SEM_FAILCRITICALERRORS;
        }
    return PreviousMode;
}


UINT
SetErrorMode(
    UINT uMode
    )
{

    UINT PreviousMode;
    UINT NewMode;

    PreviousMode = GetErrorMode();

    NewMode = uMode;
    if (NewMode & SEM_FAILCRITICALERRORS ) {
        NewMode &= ~SEM_FAILCRITICALERRORS;
        }
    else {
        NewMode |= SEM_FAILCRITICALERRORS;
        }
    if ( NT_SUCCESS(NtSetInformationProcess(
                        NtCurrentProcess(),
                        ProcessDefaultHardErrorMode,
                        (PVOID) &NewMode,
                        sizeof(NewMode)
                        ) ) ){
        }

    return( PreviousMode );
}

DWORD
GetLastError(
    VOID
    )

/*++

Routine Description:

    This function returns the most recent error code set by a Win32 API
    call.  Applications should call this function immediately after a
    Win32 API call returns a failure indications (e.g.  FALSE, NULL or
    -1) to determine the cause of the failure.

    The last error code value is a per thread field, so that multiple
    threads do not overwrite each other's last error code value.

Arguments:

    None.

Return Value:

    The return value is the most recent error code as set by a Win32 API
    call.

--*/

{
    return (DWORD)NtCurrentTeb()->LastErrorValue;
}

VOID
SetLastError(
    DWORD dwErrCode
    )

/*++

Routine Description:

    This function set the most recent error code and error string in per
    thread storage.  Win32 API functions call this function whenever
    they return a failure indication (e.g.  FALSE, NULL or -1).
    This function
    is not called by Win32 API function calls that are successful, so
    that if three Win32 API function calls are made, and the first one
    fails and the second two succeed, the error code and string stored
    by the first one are still available after the second two succeed.

    Applications can retrieve the values saved by this function using
    GetLastError.  The use of this function is optional, as an
    application need only call if it is interested in knowing the
    specific reason for an API function failure.

    The last error code value is kept in thread local storage so that
    multiple threads do not overwrite each other's values.

Arguments:

    dwErrCode - Specifies the error code to store in per thread storage
        for the current thread.

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    NtCurrentTeb()->LastErrorValue = (LONG)dwErrCode;
}

ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This API sets the "last error value" and the "last error string"
    based on the value of Status. For status codes that don't have
    a corresponding error string, the string is set to null.

Arguments:

    Status - Supplies the status value to store as the last error value.

Return Value:

    The corresponding Win32 error code that was stored in the
    "last error value" thread variable.

--*/

{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}

HANDLE
WINAPI
CreateIoCompletionPort(
    HANDLE FileHandle,
    HANDLE ExistingCompletionPort,
    ULONG_PTR CompletionKey,
    DWORD NumberOfConcurrentThreads
    )

/*++

Routine Description:

    This function creates an I/O completion port.  Completion ports
    provide another mechanism that can be used to to recive I/O
    completion notification.

    Completion ports act as a queue.  The Win32 I/O system can be
    instructed to queue I/O completion notification packets to
    completion ports.  This API provides this mechanism.  If a file
    handle is created for overlapped I/O completion
    (FILE_FLAG_OVERLAPPED) , a completion port can be associated with
    the file handle.  When I/O operations are done on a file handle that
    has an associated completion port, the I/O system will queue a
    completion packet when the I/O operation completes.  The
    GetQueuedCompletionStatus is used to pick up these queued I/O
    completion packets.

    This API can be used to create a completion port and associate it
    with a file.  If you supply a completion port, it can be used to
    associate the specified file with the specified completion port.

Arguments:

    FileHandle - Supplies a handle to a file opened for overlapped I/O
        completion.  This file is associated with either the specified
        completion port, or a new completion port is created, and the
        file is associated with that port.  Once associated with a
        completion port, the file handle may not be used in ReadFileEx
        or WriteFileEx operations.  It is not advisable to share an
        associated file handle through either handle inheritence or
        through DuplicateHandle.  I/O operations done on these
        duplicates will also generate a completion notification.

    ExistingCompletionPort - If this parameter is specified, it supplies
        an existing completion port that is to be associated with the
        specified file handle.  Otherwise, a new completion port is
        created and associated with the specified file handle.

    CompletionKey - Supplies a per-file completion key that is part of
        every I/O completion packet for this file.

    NumberOfConcurrentThreads - This is the number of threads that are
        alowed to be concurrently active and can be used to avoid
        spurious context switches, e.g., context switches that would
        occur simply because of quantum end.  Up to the number of
        threads specified are allowed to execute concurrently.  If one
        of the threads enters a wait state, then another thread is
        allowed to procede.  There may be times when more then the
        specified number of threads are active, but this will be quickly
        throttled.  A value of 0 tells the system to allow the same
        number of threads as there are processors to run.

Return Value:

    Not NULL - Returns the completion port handle associated with the file.

    NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    NTSTATUS Status;
    HANDLE Port;
    IO_STATUS_BLOCK IoSb;
    FILE_COMPLETION_INFORMATION CompletionInfo;

    Port = ExistingCompletionPort;
    if ( !ARGUMENT_PRESENT(ExistingCompletionPort) ) {
        Status = NtCreateIoCompletion (
                    &Port,
                    IO_COMPLETION_ALL_ACCESS,
                    NULL,
                    NumberOfConcurrentThreads
                    );
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return NULL;
            }
        }

    if ( FileHandle != INVALID_HANDLE_VALUE ) {
        CompletionInfo.Port = Port;
        CompletionInfo.Key = (PVOID)CompletionKey;

        Status = NtSetInformationFile(
                    FileHandle,
                    &IoSb,
                    &CompletionInfo,
                    sizeof(CompletionInfo),
                    FileCompletionInformation
                    );
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            if ( !ARGUMENT_PRESENT(ExistingCompletionPort) ) {
                NtClose(Port);
                }
            return NULL;
            }
        }
    else {

        //
        // file handle is INVALID_HANDLE_VALUE. Usually this is
        // used to create a new unassociated completion port.
        //
        // Special case here to see if existing completion port was
        // specified and fail if it is
        //

        if ( ARGUMENT_PRESENT(ExistingCompletionPort) ) {
            Port = NULL;
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            }
        }

    return Port;
}

BOOL
WINAPI
PostQueuedCompletionStatus(
    HANDLE CompletionPort,
    DWORD dwNumberOfBytesTransferred,
    ULONG_PTR dwCompletionKey,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    This function allows the caller to post an I/O completion packet to
    a completion port. This packet will satisfy an outstanding call to
    GetQueuedCompletionStatus and will provide that caller with the three values
    normally returned from that call.

Arguments:

    CompletionPort - Supplies a handle to a completion port that the caller wants to
        post a completion packet to.

    dwNumberOfBytesTransferred - Supplies the value that is to be
        returned through the lpNumberOfBytesTransfered parameter of the
        GetQueuedCompletionStatus API.

    dwCompletionKey - Supplies the value that is to be returned through
        the lpCompletionKey parameter of the GetQueuedCompletionStatus
        API.

    lpOverlapped - Supplies the value that is to be returned through the
        lpOverlapped parameter of the GetQueuedCompletionStatus API.

Return Value:

    TRUE - The operation was successful

    FALSE - The operation failed, use GetLastError to get detailed error information

--*/

{
    NTSTATUS Status;
    BOOL rv;

    rv = TRUE;
    Status = NtSetIoCompletion(
                CompletionPort,
                (PVOID)dwCompletionKey,
                (PVOID)lpOverlapped,
                STATUS_SUCCESS,
                dwNumberOfBytesTransferred
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        rv = FALSE;
        }
    return rv;
}



BOOL
WINAPI
GetQueuedCompletionStatus(
    HANDLE CompletionPort,
    LPDWORD lpNumberOfBytesTransferred,
    PULONG_PTR lpCompletionKey,
    LPOVERLAPPED *lpOverlapped,
    DWORD dwMilliseconds
    )

/*++

Routine Description:

    This function waits for pending I/O operations associated with the
    specified completion port to complete.  Server applications may have
    several threads issuing this call on the same completion port.  As
    I/O operations complete, they are queued to this port.  If threads
    are actively waiting in this call, queued requests complete their
    call.

    This API returns a boolean value.

    A value of TRUE means that a pending I/O completed successfully.
    The the number of bytes transfered during the I/O, the completion
    key that indicates which file the I/O occured on, and the overlapped
    structure address used in the original I/O are all returned.

    A value of FALSE indicates one ow two things:

    If *lpOverlapped is NULL, no I/O operation was dequeued.  This
    typically means that an error occured while processing the
    parameters to this call, or that the CompletionPort handle has been
    closed or is otherwise invalid.  GetLastError() may be used to
    further isolate this.

    If *lpOverlapped is non-NULL, an I/O completion packet was dequeud,
    but the I/O operation resulted in an error.  GetLastError() can be
    used to further isolate the I/O error.  The the number of bytes
    transfered during the I/O, the completion key that indicates which
    file the I/O occured on, and the overlapped structure address used
    in the original I/O are all returned.

Arguments:

    CompletionPort - Supplies a handle to a completion port to wait on.

    lpNumberOfBytesTransferred - Returns the number of bytes transfered during the
        I/O operation whose completion is being reported.

    lpCompletionKey - Returns a completion key value specified during
        CreateIoCompletionPort.  This is a per-file key that can be used
        to tall the caller the file that an I/O operation completed on.

    lpOverlapped - Returns the address of the overlapped structure that
        was specified when the I/O was issued.  The following APIs may
        complete using completion ports.  This ONLY occurs if the file
        handle is associated with with a completion port AND an
        overlapped structure was passed to the API.

        LockFileEx
        WriteFile
        ReadFile
        DeviceIoControl
        WaitCommEvent
        ConnectNamedPipe
        TransactNamedPipe

    dwMilliseconds - Supplies an optional timeout value that specifies
        how long the caller is willing to wait for an I/O completion
        packet.

Return Value:

    TRUE - An I/O operation completed successfully.
        lpNumberOfBytesTransferred, lpCompletionKey, and lpOverlapped
        are all valid.

    FALSE - If lpOverlapped is NULL, the operation failed and no I/O
        completion data is retured.  GetLastError() can be used to
        further isolate the cause of the error (bad parameters, invalid
        completion port handle).  Otherwise, a pending I/O operation
        completed, but it completed with an error.  GetLastError() can
        be used to further isolate the I/O error.
        lpNumberOfBytesTransferred, lpCompletionKey, and lpOverlapped
        are all valid.

--*/

{

    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    IO_STATUS_BLOCK IoSb;
    NTSTATUS Status;
    LPOVERLAPPED LocalOverlapped;
    BOOL rv;


    pTimeOut = BaseFormatTimeOut(&TimeOut,dwMilliseconds);
    Status = NtRemoveIoCompletion(
                CompletionPort,
                (PVOID *)lpCompletionKey,
                (PVOID *)&LocalOverlapped,
                &IoSb,
                pTimeOut
                );

    if ( !NT_SUCCESS(Status) || Status == STATUS_TIMEOUT ) {
        *lpOverlapped = NULL;
        if ( Status == STATUS_TIMEOUT ) {
            SetLastError(WAIT_TIMEOUT);
            }
        else {
            BaseSetLastNTError(Status);
            }
        rv = FALSE;
        }
    else {
        *lpOverlapped = LocalOverlapped;

        *lpNumberOfBytesTransferred = (DWORD)IoSb.Information;

        if ( !NT_SUCCESS(IoSb.Status) ){
            BaseSetLastNTError( IoSb.Status );
            rv = FALSE;
            }
        else {
            rv = TRUE;
            }
        }
    return rv;
}

BOOL
WINAPI
GetOverlappedResult(
    HANDLE hFile,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred,
    BOOL bWait
    )

/*++

Routine Description:

    The GetOverlappedResult function returns the result of the last
    operation that used lpOverlapped and returned ERROR_IO_PENDING.

Arguments:

    hFile - Supplies the open handle to the file that the overlapped
        structure lpOverlapped was supplied to ReadFile, WriteFile,
        ConnectNamedPipe, WaitNamedPipe or TransactNamedPipe.

    lpOverlapped - Points to an OVERLAPPED structure previously supplied to
        ReadFile, WriteFile, ConnectNamedPipe, WaitNamedPipe or
        TransactNamedPipe.

    lpNumberOfBytesTransferred - Returns the number of bytes transferred
        by the operation.

    bWait -  A boolean value that affects the behavior when the operation
        is still in progress. If TRUE and the operation is still in progress,
        GetOverlappedResult will wait for the operation to complete before
        returning. If FALSE and the operation is incomplete,
        GetOverlappedResult will return FALSE. In this case the extended
        error information available from the GetLastError function will be
        set to ERROR_IO_INCOMPLETE.

Return Value:

    TRUE -- The operation was successful, the pipe is in the
        connected state.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{
    DWORD WaitReturn;

    //
    // Did caller specify an event to the original operation or was the
    // default (file handle) used?
    //

    if (lpOverlapped->Internal == (DWORD)STATUS_PENDING ) {
        if ( bWait ) {
            WaitReturn = WaitForSingleObject(
                            ( lpOverlapped->hEvent != NULL ) ?
                                lpOverlapped->hEvent : hFile,
                            INFINITE
                            );
            }
        else {
            WaitReturn = WAIT_TIMEOUT;
            }

        if ( WaitReturn == WAIT_TIMEOUT ) {
            //  !bWait and event in not signalled state
            SetLastError( ERROR_IO_INCOMPLETE );
            return FALSE;
            }

        if ( WaitReturn != 0 ) {
             return FALSE;    // WaitForSingleObject calls BaseSetLastError
             }
        }

    *lpNumberOfBytesTransferred = (DWORD)lpOverlapped->InternalHigh;

    if ( NT_SUCCESS((NTSTATUS)lpOverlapped->Internal) ){
        return TRUE;
        }
    else {
        BaseSetLastNTError( (NTSTATUS)lpOverlapped->Internal );
        return FALSE;
        }
}
