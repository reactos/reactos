/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    filehops.c

Abstract:

    This module implements Win32 file handle APIs

Author:

    Mark Lucovsky (markl) 25-Sep-1990

Revision History:

--*/

#include "basedll.h"

HANDLE
WINAPI
GetStdHandle(
    DWORD nStdHandle
    )
{
    PPEB Peb;
    HANDLE rv;


    Peb = NtCurrentPeb();
    switch( nStdHandle ) {
        case STD_INPUT_HANDLE:
            rv = Peb->ProcessParameters->StandardInput;
            break;

        case STD_OUTPUT_HANDLE:
            rv = Peb->ProcessParameters->StandardOutput;
            break;

        case STD_ERROR_HANDLE:
            rv = Peb->ProcessParameters->StandardError;
            break;
        default:
            rv = INVALID_HANDLE_VALUE;
            break;
    }
    if ( rv == INVALID_HANDLE_VALUE ) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        }
    return rv;
}

BOOL
WINAPI
SetStdHandle(
    DWORD nStdHandle,
    HANDLE hHandle
    )
{
    PPEB Peb;

    Peb = NtCurrentPeb();
    switch( nStdHandle ) {
        case STD_INPUT_HANDLE:
            Peb->ProcessParameters->StandardInput = hHandle;
            break;

        case STD_OUTPUT_HANDLE:
            Peb->ProcessParameters->StandardOutput = hHandle;
            break;

        case STD_ERROR_HANDLE:
            Peb->ProcessParameters->StandardError = hHandle;
            break;

        default:
            BaseSetLastNTError(STATUS_INVALID_HANDLE);
            return FALSE;
    }

    return( TRUE );
}

DWORD
WINAPI
GetFileType(
    HANDLE hFile
    )

/*++

Routine Description:

    GetFileType is used to determine the file type of the specified file.

Arguments:

    hFile - Supplies an open handle to a file whose type is to be
        determined

Return Value:

    FILE_TYPE_UNKNOWN - The type of the specified file is unknown.

    FILE_TYPE_DISK - The specified file is a disk file.

    FILE_TYPE_CHAR - The specified file is a character file (LPT,
        console...)

    FILE_TYPE_PIPE - The specified file is a pipe (either a named pipe or
        a pipe created by CreatePipe).

--*/

{
    NTSTATUS Status;
    FILE_FS_DEVICE_INFORMATION DeviceInformation;
    IO_STATUS_BLOCK IoStatusBlock;
    PPEB Peb;

    Peb = NtCurrentPeb();

    switch( HandleToUlong(hFile) ) {
        case STD_INPUT_HANDLE:  hFile = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hFile = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hFile = Peb->ProcessParameters->StandardError;
                                break;
        }

    if (CONSOLE_HANDLE(hFile) && VerifyConsoleIoHandle(hFile)) {
        return( FILE_TYPE_CHAR );
        }

    Status = NtQueryVolumeInformationFile( hFile,
                                           &IoStatusBlock,
                                           &DeviceInformation,
                                           sizeof( DeviceInformation ),
                                           FileFsDeviceInformation
                                         );

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return( FILE_TYPE_UNKNOWN );
        }

    switch( DeviceInformation.DeviceType ) {

    case FILE_DEVICE_SCREEN:
    case FILE_DEVICE_KEYBOARD:
    case FILE_DEVICE_MOUSE:
    case FILE_DEVICE_PARALLEL_PORT:
    case FILE_DEVICE_PRINTER:
    case FILE_DEVICE_SERIAL_PORT:
    case FILE_DEVICE_MODEM:
    case FILE_DEVICE_SOUND:
    case FILE_DEVICE_NULL:
        return( FILE_TYPE_CHAR );

    case FILE_DEVICE_CD_ROM:
    case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
    case FILE_DEVICE_CONTROLLER:
    case FILE_DEVICE_DATALINK:
    case FILE_DEVICE_DFS:
    case FILE_DEVICE_DISK:
    case FILE_DEVICE_DISK_FILE_SYSTEM:
    case FILE_DEVICE_VIRTUAL_DISK:
        return( FILE_TYPE_DISK );

    case FILE_DEVICE_NAMED_PIPE:
        return( FILE_TYPE_PIPE );

    case FILE_DEVICE_NETWORK:
    case FILE_DEVICE_NETWORK_FILE_SYSTEM:
    case FILE_DEVICE_PHYSICAL_NETCARD:
    case FILE_DEVICE_TAPE:
    case FILE_DEVICE_TAPE_FILE_SYSTEM:
    case FILE_DEVICE_TRANSPORT:
        // FIX, FIX - how to handle tapes, network devices, etc.

    case FILE_DEVICE_UNKNOWN:
    default:
        SetLastError( NO_ERROR );
        return( FILE_TYPE_UNKNOWN );
    }
}

BOOL
WINAPI
ReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    Data can be read from a file using ReadFile.

    This API is used to read data from a file.  Data is read from the
    file from the position indicated by the file pointer.  After the
    read completes, the file pointer is adjusted by the number of bytes
    actually read.  A return value of TRUE coupled with a bytes read of
    0 indicates that the file pointer was beyond the current end of the
    file at the time of the read.

Arguments:

    hFile - Supplies an open handle to a file that is to be read.  The
        file handle must have been created with GENERIC_READ access to
        the file.

    lpBuffer - Supplies the address of a buffer to receive the data read
        from the file.

    nNumberOfBytesToRead - Supplies the number of bytes to read from the
        file.

    lpNumberOfBytesRead - Returns the number of bytes read by this call.
        This parameter is always set to 0 before doing any IO or error
        checking.

    lpOverlapped - Optionally points to an OVERLAPPED structure to be used with the
    request. If NULL then the transfer starts at the current file position
    and ReadFile will not return until the operation completes.

    If the handle hFile was created without specifying FILE_FLAG_OVERLAPPED
    the file pointer is moved to the specified offset plus
    lpNumberOfBytesRead before ReadFile returns. ReadFile will wait for the
    request to complete before returning (it will not return
    ERROR_IO_PENDING).

    When FILE_FLAG_OVERLAPPED is specified, ReadFile may return
    ERROR_IO_PENDING to allow the calling function to continue processing
    while the operation completes. The event (or hFile if hEvent is NULL) will
    be set to the signalled state upon completion of the request.

    When the handle is created with FILE_FLAG_OVERLAPPED and lpOverlapped
    is set to NULL, ReadFile will return ERROR_INVALID_PARAMTER because
    the file offset is required.


Return Value:

    TRUE - The operation was successul.

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PPEB Peb;
    DWORD InputMode;

    if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
        *lpNumberOfBytesRead = 0;
        }

    Peb = NtCurrentPeb();

    switch( HandleToUlong(hFile) ) {
        case STD_INPUT_HANDLE:  hFile = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hFile = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hFile = Peb->ProcessParameters->StandardError;
                                break;
        }

    if (CONSOLE_HANDLE(hFile)) {
        if (ReadConsoleA(hFile,
                        lpBuffer,
                        nNumberOfBytesToRead,
                        lpNumberOfBytesRead,
                        lpOverlapped
                       )
           ) {
            Status = STATUS_SUCCESS;
            if (!GetConsoleMode( hFile, &InputMode )) {
                InputMode = 0;
                }

            if (InputMode & ENABLE_PROCESSED_INPUT) {
                try {
                    if (*(PCHAR)lpBuffer == 0x1A) {
                        *lpNumberOfBytesRead = 0;
                        }
                    }
                except( EXCEPTION_EXECUTE_HANDLER ) {
                    Status = GetExceptionCode();
                    }
                }

            if (NT_SUCCESS(Status)) {
                return TRUE;
                }
            else {
                BaseSetLastNTError(Status);
                return FALSE;
                }
            }
        else {
            return FALSE;
            }
        }

    if ( ARGUMENT_PRESENT( lpOverlapped ) ) {
        LARGE_INTEGER Li;

        lpOverlapped->Internal = (DWORD)STATUS_PENDING;
        Li.LowPart = lpOverlapped->Offset;
        Li.HighPart = lpOverlapped->OffsetHigh;
        Status = NtReadFile(
                hFile,
                lpOverlapped->hEvent,
                NULL,
                (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                lpBuffer,
                nNumberOfBytesToRead,
                &Li,
                NULL
                );


        if ( NT_SUCCESS(Status) && Status != STATUS_PENDING) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
                try {
                    *lpNumberOfBytesRead = (DWORD)lpOverlapped->InternalHigh;
                    }
                except(EXCEPTION_EXECUTE_HANDLER) {
                    *lpNumberOfBytesRead = 0;
                    }
                }
            return TRUE;
            }
        else
        if (Status == STATUS_END_OF_FILE) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
                *lpNumberOfBytesRead = 0;
                }
            BaseSetLastNTError(Status);
            return FALSE;
            }
        else {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else
        {
        Status = NtReadFile(
                hFile,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                lpBuffer,
                nNumberOfBytesToRead,
                NULL,
                NULL
                );

        if ( Status == STATUS_PENDING) {
            // Operation must complete before return & IoStatusBlock destroyed
            Status = NtWaitForSingleObject( hFile, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = IoStatusBlock.Status;
                }
            }

        if ( NT_SUCCESS(Status) ) {
            *lpNumberOfBytesRead = (DWORD)IoStatusBlock.Information;
            return TRUE;
            }
        else
        if (Status == STATUS_END_OF_FILE) {
            *lpNumberOfBytesRead = 0;
            return TRUE;
            }
        else {
            if ( NT_WARNING(Status) ) {
                *lpNumberOfBytesRead = (DWORD)IoStatusBlock.Information;
                }
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
}


BOOL
WINAPI
WriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    Data can be written to a file using WriteFile.

    This API is used to write data to a file.  Data is written to the
    file from the position indicated by the file pointer.  After the
    write completes, the file pointer is adjusted by the number of bytes
    actually written.

    Unlike DOS, a NumberOfBytesToWrite value of zero does not truncate
    or extend the file.  If this function is required, SetEndOfFile
    should be used.

Arguments:

    hFile - Supplies an open handle to a file that is to be written.  The
        file handle must have been created with GENERIC_WRITE access to
        the file.

    lpBuffer - Supplies the address of the data that is to be written to
        the file.

    nNumberOfBytesToWrite - Supplies the number of bytes to write to the
        file. Unlike DOS, a value of zero is interpreted a null write.

    lpNumberOfBytesWritten - Returns the number of bytes written by this
        call. Before doing any work or error processing, the API sets this
        to zero.


    lpOverlapped - Optionally points to an OVERLAPPED structure to be
        used with the request. If NULL then the transfer starts at the
        current file position and WriteFile will not return until the
        operation completes.

        If the handle <hFile> was created without specifying
        FILE_FLAG_OVERLAPPED the file pointer is moved to the specified
        offset plus lpNumberOfBytesWritten before WriteFile returns.
        WriteFile will wait for the request to complete before returning
        (it will not set ERROR_IO_PENDING).

        When FILE_FLAG_OVERLAPPED is specified, WriteFile may return
        ERROR_IO_PENDING to allow the calling function to continue processing
        while the operation completes. The event (or hFile if hEvent is NULL) will
        be set to the signalled state upon completion of the request.

        When the handle is created with FILE_FLAG_OVERLAPPED and lpOverlapped
        is set to NULL, WriteFile will return ERROR_INVALID_PARAMTER because
        the file offset is required.

Return Value:

    TRUE - The operation was a success.

    FALSE - The operation failed.  Extended error status is
        available using GetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PPEB Peb;

    if ( ARGUMENT_PRESENT(lpNumberOfBytesWritten) ) {
        *lpNumberOfBytesWritten = 0;
        }

    Peb = NtCurrentPeb();
    switch( HandleToUlong(hFile) ) {
        case STD_INPUT_HANDLE:  hFile = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hFile = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hFile = Peb->ProcessParameters->StandardError;
                                break;
        }

    if (CONSOLE_HANDLE(hFile)) {
        return WriteConsoleA(hFile,
                            (LPVOID)lpBuffer,
                            nNumberOfBytesToWrite,
                            lpNumberOfBytesWritten,
                            lpOverlapped
                           );
        }

    if ( ARGUMENT_PRESENT( lpOverlapped ) ) {
        LARGE_INTEGER Li;

        lpOverlapped->Internal = (DWORD)STATUS_PENDING;
        Li.LowPart = lpOverlapped->Offset;
        Li.HighPart = lpOverlapped->OffsetHigh;
        Status = NtWriteFile(
                hFile,
                lpOverlapped->hEvent,
                NULL,
                (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                (volatile PVOID)lpBuffer,
                nNumberOfBytesToWrite,
                &Li,
                NULL
                );

        if ( !NT_ERROR(Status) && Status != STATUS_PENDING) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesWritten) ) {
                try {
                    *lpNumberOfBytesWritten = (DWORD)lpOverlapped->InternalHigh;
                    }
                except(EXCEPTION_EXECUTE_HANDLER) {
                    *lpNumberOfBytesWritten = 0;
                    }
                }
            return TRUE;
            }
        else  {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else {
        Status = NtWriteFile(
                hFile,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                (PVOID)lpBuffer,
                nNumberOfBytesToWrite,
                NULL,
                NULL
                );

        if ( Status == STATUS_PENDING) {
            // Operation must complete before return & IoStatusBlock destroyed
            Status = NtWaitForSingleObject( hFile, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = IoStatusBlock.Status;
                }
            }

        if ( NT_SUCCESS(Status)) {
            *lpNumberOfBytesWritten = (DWORD)IoStatusBlock.Information;
            return TRUE;
            }
        else {
            if ( NT_WARNING(Status) ) {
                *lpNumberOfBytesWritten = (DWORD)IoStatusBlock.Information;
                }
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
}

BOOL
WINAPI
SetEndOfFile(
    HANDLE hFile
    )

/*++

Routine Description:

    The end of file position of an open file can be set to the current
    file pointer using SetEndOfFile.

    This API is used to set the end of file position of a file to the
    same value as the current file pointer.  This has the effect of
    truncating or extending a file.  This functionality is similar to
    DOS (int 21h, function 40H with CX=0).

Arguments:

    hFile - Supplies an open handle to a file that is to be extended or
        truncated.  The file handle must have been created with
        GENERIC_WRITE access to the file.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_POSITION_INFORMATION CurrentPosition;
    FILE_END_OF_FILE_INFORMATION EndOfFile;
    FILE_ALLOCATION_INFORMATION Allocation;

    if (CONSOLE_HANDLE(hFile)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
        }

    //
    // Get the current position of the file pointer
    //

    Status = NtQueryInformationFile(
                hFile,
                &IoStatusBlock,
                &CurrentPosition,
                sizeof(CurrentPosition),
                FilePositionInformation
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    //
    // Set the end of file based on the current file position
    //

    EndOfFile.EndOfFile = CurrentPosition.CurrentByteOffset;

    Status = NtSetInformationFile(
                hFile,
                &IoStatusBlock,
                &EndOfFile,
                sizeof(EndOfFile),
                FileEndOfFileInformation
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    //
    // Set the allocation based on the current file size
    //

    Allocation.AllocationSize = CurrentPosition.CurrentByteOffset;

    Status = NtSetInformationFile(
                hFile,
                &IoStatusBlock,
                &Allocation,
                sizeof(Allocation),
                FileAllocationInformation
                );
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

DWORD
WINAPI
SetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod
    )

/*++

Routine Description:

    An open file's file pointer can be set using SetFilePointer.

    The purpose of this function is to update the current value of a
    file's file pointer.  Care should be taken in multi-threaded
    applications that have multiple threads sharing a file handle with
    each thread updating the file pointer and then doing a read.  This
    sequence should be treated as a critical section of code and should
    be protected using either a critical section object or a mutex
    object.

    This API provides the same functionality as DOS (int 21h, function
    42h) and OS/2's DosSetFilePtr.

Arguments:

    hFile - Supplies an open handle to a file whose file pointer is to be
        moved.  The file handle must have been created with
        GENERIC_READ or GENERIC_WRITE access to the file.

    lDistanceToMove - Supplies the number of bytes to move the file
        pointer.  A positive value moves the pointer forward in the file
        and a negative value moves backwards in the file.

    lpDistanceToMoveHigh - An optional parameter that if specified
        supplies the high order 32-bits of the 64-bit distance to move.
        If the value of this parameter is NULL, this API can only
        operate on files whose maximum size is (2**32)-2.  If this
        parameter is specified, than the maximum file size is (2**64)-2.
        This value also returns the high order 32-bits of the new value
        of the file pointer.  If this value, and the return value
        are 0xffffffff, then an error is indicated.

    dwMoveMethod - Supplies a value that specifies the starting point
        for the file pointer move.

        FILE_BEGIN - The starting point is zero or the beginning of the
            file.  If FILE_BEGIN is specified, then DistanceToMove is
            interpreted as an unsigned location for the new
            file pointer.

        FILE_CURRENT - The current value of the file pointer is used as
            the starting point.

        FILE_END - The current end of file position is used as the
            starting point.


Return Value:

    Not -1 - Returns the low order 32-bits of the new value of the file
        pointer.

    0xffffffff - If the value of lpDistanceToMoveHigh was NULL, then The
        operation failed.  Extended error status is available using
        GetLastError.  Otherwise, this is the low order 32-bits of the
        new value of the file pointer.

--*/

{

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_POSITION_INFORMATION CurrentPosition;
    FILE_STANDARD_INFORMATION StandardInfo;
    LARGE_INTEGER Large;

    if (CONSOLE_HANDLE(hFile)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return (DWORD)-1;
        }

    if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)) {
        Large.HighPart = *lpDistanceToMoveHigh;
        Large.LowPart = lDistanceToMove;
        }
    else {
        Large.QuadPart = lDistanceToMove;
        }
    switch (dwMoveMethod) {
        case FILE_BEGIN :
            CurrentPosition.CurrentByteOffset = Large;
                break;

        case FILE_CURRENT :

            //
            // Get the current position of the file pointer
            //

            Status = NtQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        &CurrentPosition,
                        sizeof(CurrentPosition),
                        FilePositionInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                return (DWORD)-1;
                }
            CurrentPosition.CurrentByteOffset.QuadPart += Large.QuadPart;
            break;

        case FILE_END :
            Status = NtQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        &StandardInfo,
                        sizeof(StandardInfo),
                        FileStandardInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                return (DWORD)-1;
                }
            CurrentPosition.CurrentByteOffset.QuadPart =
                                StandardInfo.EndOfFile.QuadPart + Large.QuadPart;
            break;

        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return (DWORD)-1;
            break;
        }

    //
    // If the resulting file position is negative, or if the app is not
    // prepared for greater than
    // then 32 bits than fail
    //

    if ( CurrentPosition.CurrentByteOffset.QuadPart < 0 ) {
        SetLastError(ERROR_NEGATIVE_SEEK);
        return (DWORD)-1;
        }
    if ( !ARGUMENT_PRESENT(lpDistanceToMoveHigh) &&
        (CurrentPosition.CurrentByteOffset.HighPart & MAXLONG) ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (DWORD)-1;
        }


    //
    // Set the current file position
    //

    Status = NtSetInformationFile(
                hFile,
                &IoStatusBlock,
                &CurrentPosition,
                sizeof(CurrentPosition),
                FilePositionInformation
                );
    if ( NT_SUCCESS(Status) ) {
        if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)){
            *lpDistanceToMoveHigh = CurrentPosition.CurrentByteOffset.HighPart;
            }
        if ( CurrentPosition.CurrentByteOffset.LowPart == -1 ) {
            SetLastError(0);
            }
        return CurrentPosition.CurrentByteOffset.LowPart;
        }
    else {
        BaseSetLastNTError(Status);
        if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)){
            *lpDistanceToMoveHigh = -1;
            }
        return (DWORD)-1;
        }
}


BOOL
WINAPI
SetFilePointerEx(
    HANDLE hFile,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER lpNewFilePointer,
    DWORD dwMoveMethod
    )

/*++

Routine Description:

    An open file's file pointer can be set using SetFilePointer.

    The purpose of this function is to update the current value of a
    file's file pointer.  Care should be taken in multi-threaded
    applications that have multiple threads sharing a file handle with
    each thread updating the file pointer and then doing a read.  This
    sequence should be treated as a critical section of code and should
    be protected using either a critical section object or a mutex
    object.

    This API provides the same functionality as DOS (int 21h, function
    42h) and OS/2's DosSetFilePtr.

Arguments:

    hFile - Supplies an open handle to a file whose file pointer is to be
        moved.  The file handle must have been created with
        GENERIC_READ or GENERIC_WRITE access to the file.

    liDistanceToMove - Supplies the number of bytes to move the file
        pointer.  A positive value moves the pointer forward in the file
        and a negative value moves backwards in the file.

    lpNewFilePointer - An optional parameter that if specified returns
        the new file pointer

    dwMoveMethod - Supplies a value that specifies the starting point
        for the file pointer move.

        FILE_BEGIN - The starting point is zero or the beginning of the
            file.  If FILE_BEGIN is specified, then DistanceToMove is
            interpreted as an unsigned location for the new
            file pointer.

        FILE_CURRENT - The current value of the file pointer is used as
            the starting point.

        FILE_END - The current end of file position is used as the
            starting point.


Return Value:

    TRUE - The operation was successful

    FALSE - The operation failed. Extended error status is available using
        GetLastError.

--*/

{

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_POSITION_INFORMATION CurrentPosition;
    FILE_STANDARD_INFORMATION StandardInfo;
    LARGE_INTEGER Large;

    if (CONSOLE_HANDLE(hFile)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
        }
    Large = liDistanceToMove;

    switch (dwMoveMethod) {
        case FILE_BEGIN :
            CurrentPosition.CurrentByteOffset = Large;
                break;

        case FILE_CURRENT :

            //
            // Get the current position of the file pointer
            //

            Status = NtQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        &CurrentPosition,
                        sizeof(CurrentPosition),
                        FilePositionInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                FALSE;
                }
            CurrentPosition.CurrentByteOffset.QuadPart += Large.QuadPart;
            break;

        case FILE_END :
            Status = NtQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        &StandardInfo,
                        sizeof(StandardInfo),
                        FileStandardInformation
                        );
            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                FALSE;
                }
            CurrentPosition.CurrentByteOffset.QuadPart =
                                StandardInfo.EndOfFile.QuadPart + Large.QuadPart;
            break;

        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
            break;
        }

    //
    // If the resulting file position is negative fail
    //

    if ( CurrentPosition.CurrentByteOffset.QuadPart < 0 ) {
        SetLastError(ERROR_NEGATIVE_SEEK);
        return FALSE;
        }


    //
    // Set the current file position
    //

    Status = NtSetInformationFile(
                hFile,
                &IoStatusBlock,
                &CurrentPosition,
                sizeof(CurrentPosition),
                FilePositionInformation
                );
    if ( NT_SUCCESS(Status) ) {
        if (ARGUMENT_PRESENT(lpNewFilePointer)){
            *lpNewFilePointer = CurrentPosition.CurrentByteOffset;
            }
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}



BOOL
WINAPI
GetFileInformationByHandle(
    HANDLE hFile,
    LPBY_HANDLE_FILE_INFORMATION lpFileInformation
    )

/*++

Routine Description:


Arguments:

    hFile - Supplies an open handle to a file whose modification date and
        times are to be read.  The file handle must have been created with
        GENERIC_READ access to the file.

    lpCreationTime - An optional parameter that if specified points to
        the location to return the date and time the file was created.
        A returned time of all zero indicates that the file system
        containing the file does not support this time value.


Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    BY_HANDLE_FILE_INFORMATION LocalFileInformation;
    FILE_ALL_INFORMATION FileInformation;
    FILE_FS_VOLUME_INFORMATION VolumeInfo;

    if (CONSOLE_HANDLE(hFile)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
        }

    Status = NtQueryVolumeInformationFile(
                hFile,
                &IoStatusBlock,
                &VolumeInfo,
                sizeof(VolumeInfo),
                FileFsVolumeInformation
                );
    if ( !NT_ERROR(Status) ) {
        LocalFileInformation.dwVolumeSerialNumber = VolumeInfo.VolumeSerialNumber;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }


    Status = NtQueryInformationFile(
                hFile,
                &IoStatusBlock,
                &FileInformation,
                sizeof(FileInformation),
                FileAllInformation
                );

    //
    // we really plan for buffer overflow
    //

    if ( !NT_ERROR(Status) ) {
        LocalFileInformation.dwFileAttributes = FileInformation.BasicInformation.FileAttributes;
        LocalFileInformation.ftCreationTime = *(LPFILETIME)&FileInformation.BasicInformation.CreationTime;
        LocalFileInformation.ftLastAccessTime = *(LPFILETIME)&FileInformation.BasicInformation.LastAccessTime;
        LocalFileInformation.ftLastWriteTime = *(LPFILETIME)&FileInformation.BasicInformation.LastWriteTime;
        LocalFileInformation.nFileSizeHigh = FileInformation.StandardInformation.EndOfFile.HighPart;
        LocalFileInformation.nFileSizeLow = FileInformation.StandardInformation.EndOfFile.LowPart;
        LocalFileInformation.nNumberOfLinks = FileInformation.StandardInformation.NumberOfLinks;
        LocalFileInformation.nFileIndexHigh = FileInformation.InternalInformation.IndexNumber.HighPart;
        LocalFileInformation.nFileIndexLow = FileInformation.InternalInformation.IndexNumber.LowPart;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    *lpFileInformation = LocalFileInformation;
    return TRUE;
}

BOOL
APIENTRY
GetFileTime(
    HANDLE hFile,
    LPFILETIME lpCreationTime,
    LPFILETIME lpLastAccessTime,
    LPFILETIME lpLastWriteTime
    )

/*++

Routine Description:

    The date and time that a file was created, last accessed or last
    modified can be read using GetFileTime.  File time stamps are
    returned as 64-bit values, that represent the number of 100
    nanoseconds since January 1st, 1601.  This date was chosen because
    it is the start of a new quadricentury.  At 100ns resolution 32 bits
    is good for about 429 seconds (or 7 minutes) and a 63-bit integer is
    good for about 29,247 years, or around 10,682,247 days.

    This API provides the same functionality as DOS (int 21h, function
    47H with AL=0), and provides a subset of OS/2's DosQueryFileInfo.

Arguments:

    hFile - Supplies an open handle to a file whose modification date and
        times are to be read.  The file handle must have been created with
        GENERIC_READ access to the file.

    lpCreationTime - An optional parameter that if specified points to
        the location to return the date and time the file was created.
        A returned time of all zero indicates that the file system
        containing the file does not support this time value.

    lpLastAccessTime - An optional parameter that if specified points to
        the location to return the date and time the file was last accessed.
        A returned time of all zero indicates that the file system
        containing the file does not support this time value.

    lpLastWriteTime - An optional parameter that if specified points to
        the location to return the date and time the file was last written.
        A file system must support this time and thus a valid value will
        always be returned for this time value.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicInfo;

    if (CONSOLE_HANDLE(hFile)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
        }

    //
    // Get the attributes
    //

    Status = NtQueryInformationFile(
                hFile,
                &IoStatusBlock,
                &BasicInfo,
                sizeof(BasicInfo),
                FileBasicInformation
                );

    if ( NT_SUCCESS(Status) ) {
        if (ARGUMENT_PRESENT( lpCreationTime )) {
            *lpCreationTime = *(LPFILETIME)&BasicInfo.CreationTime;
            }

        if (ARGUMENT_PRESENT( lpLastAccessTime )) {
            *lpLastAccessTime = *(LPFILETIME)&BasicInfo.LastAccessTime;
            }

        if (ARGUMENT_PRESENT( lpLastWriteTime )) {
            *lpLastWriteTime = *(LPFILETIME)&BasicInfo.LastWriteTime;
            }
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
WINAPI
SetFileTime(
    HANDLE hFile,
    CONST FILETIME *lpCreationTime,
    CONST FILETIME *lpLastAccessTime,
    CONST FILETIME *lpLastWriteTime
    )

/*++

Routine Description:

    The date and time that a file was created, last accessed or last
    modified can be modified using SetFileTime.  File time stamps are
    returned as 64-bit values, that represent the number of 100
    nanoseconds since January 1st, 1601.  This date was chosen because
    it is the start of a new quadricentury.  At 100ns resolution 32 bits
    is good for about 429 seconds (or 7 minutes) and a 63-bit integer is
    good for about 29,247 years, or around 10,682,247 days.

    This API provides the same functionality as DOS (int 21h, function
    47H with AL=1), and provides a subset of OS/2's DosSetFileInfo.

Arguments:

    hFile - Supplies an open handle to a file whose modification date and
        times are to be written.  The file handle must have been created
        with GENERIC_WRITE access to the file.

    lpCreationTime - An optional parameter, that if specified supplies
        the new creation time for the file.  Some file system's do not
        support this time value, so this parameter may be ignored.

    lpLastAccessTime - An optional parameter, that if specified supplies
        the new last access time for the file.  Some file system's do
        not support this time value, so this parameter may be ignored.

    lpLastWriteTime - An optional parameter, that if specified supplies
        the new last write time for the file.  A file system must support
        this time value.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicInfo;

    if (CONSOLE_HANDLE(hFile)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
        }

    //
    // Zero all the time values we can set.
    //

    RtlZeroMemory(&BasicInfo,sizeof(BasicInfo));

    //
    // For each time value that is specified, copy it to the I/O system
    // record.
    //
    if (ARGUMENT_PRESENT( lpCreationTime )) {
        BasicInfo.CreationTime.LowPart = lpCreationTime->dwLowDateTime;
        BasicInfo.CreationTime.HighPart = lpCreationTime->dwHighDateTime;
        }

    if (ARGUMENT_PRESENT( lpLastAccessTime )) {
        BasicInfo.LastAccessTime.LowPart = lpLastAccessTime->dwLowDateTime;
        BasicInfo.LastAccessTime.HighPart = lpLastAccessTime->dwHighDateTime;
        }

    if (ARGUMENT_PRESENT( lpLastWriteTime )) {
        BasicInfo.LastWriteTime.LowPart = lpLastWriteTime->dwLowDateTime;
        BasicInfo.LastWriteTime.HighPart = lpLastWriteTime->dwHighDateTime;
        }

    //
    // Set the requested times.
    //

    Status = NtSetInformationFile(
                hFile,
                &IoStatusBlock,
                &BasicInfo,
                sizeof(BasicInfo),
                FileBasicInformation
                );

    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
WINAPI
FlushFileBuffers(
    HANDLE hFile
    )

/*++

Routine Description:

    Buffered data may be flushed out to the file using the
    FlushFileBuffers service.

    The FlushFileBuffers service causes all buffered data to be written
    to the specified file.

Arguments:

    hFile - Supplies an open handle to a file whose buffers are to be
        flushed.  The file handle must have been created with
        GENERIC_WRITE access to the file.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PPEB Peb;

    Peb = NtCurrentPeb();

    switch( HandleToUlong(hFile) ) {
        case STD_INPUT_HANDLE:  hFile = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hFile = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hFile = Peb->ProcessParameters->StandardError;
                                break;
        }

    if (CONSOLE_HANDLE(hFile)) {
        return( FlushConsoleInputBuffer( hFile ) );
        }

    Status = NtFlushBuffersFile(hFile,&IoStatusBlock);

    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
WINAPI
LockFile(
    HANDLE hFile,
    DWORD dwFileOffsetLow,
    DWORD dwFileOffsetHigh,
    DWORD nNumberOfBytesToLockLow,
    DWORD nNumberOfBytesToLockHigh
    )

/*++

Routine Description:

    A byte range within an open file may be locked for exclusive access
    using LockFile.

    Locking a region of a file is used to aquire exclusive access to the
    specified region of the file.  File locks are not inherited by the
    new process during process creation.

    Locking a portion of a file denies all other processes both read and
    write access to the specified region of the file.  Locking a region
    that goes beyond the current end-of-file position is not an error.

    Locks may not overlap an existing locked region of the file.

    For DOS based systems running share.exe the lock semantics work as
    described above.  Without share.exe, all attempts to lock or unlock
    a file will fail.

Arguments:

    hFile - Supplies an open handle to a file that is to have a range of
        bytes locked for exclusive access.  The handle must have been
        created with either GENERIC_READ or GENERIC_WRITE access to the
        file.

    dwFileOffsetLow - Supplies the low order 32-bits of the starting
        byte offset of the file where the lock should begin.

    dwFileOffsetHigh - Supplies the high order 32-bits of the starting
        byte offset of the file where the lock should begin.

    nNumberOfBytesToLockLow - Supplies the low order 32-bits of the length
        of the byte range to be locked.

    nNumberOfBytesToLockHigh - Supplies the high order 32-bits of the length
        of the byte range to be locked.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    LARGE_INTEGER ByteOffset;
    LARGE_INTEGER Length;
    IO_STATUS_BLOCK IoStatusBlock;

    if (CONSOLE_HANDLE(hFile)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
        }

    ByteOffset.LowPart = dwFileOffsetLow;
    ByteOffset.HighPart = dwFileOffsetHigh;

    Length.LowPart = nNumberOfBytesToLockLow;
    Length.HighPart = nNumberOfBytesToLockHigh;

    Status = NtLockFile( hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &ByteOffset,
                         &Length,
                         0,
                         TRUE,
                         TRUE
                       );

    if (Status == STATUS_PENDING) {

        Status = NtWaitForSingleObject( hFile, FALSE, NULL );
        if (NT_SUCCESS( Status )) {
            Status = IoStatusBlock.Status;
            }
        }

    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}


BOOL
WINAPI
LockFileEx(
    HANDLE hFile,
    DWORD dwFlags,
    DWORD dwReserved,
    DWORD nNumberOfBytesToLockLow,
    DWORD nNumberOfBytesToLockHigh,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    A byte range within an open file may be locked for shared or
    exclusive access using LockFileEx.

    Locking a region of a file is used to aquire shared or exclusive
    access to the specified region of the file.  File locks are not
    inherited by the new process during process creation.

    Locking a portion of a file for exclusive access denies all other
    processes both read and write access to the specified region of the
    file.  Locking a region that goes beyond the current end-of-file
    position is not an error.

    Locking a portion of a file for shared access denies all other
    processes write access to the specified region of the file, but
    allows other processes to read the locked region.

    If requesting an exclusive lock for a file that is already locked
    shared or exclusively by other threads, then this call will wait
    until the lock is granted unless the LOCKFILE_FAIL_IMMEDIATELY
    flag is specified.

    Locks may not overlap an existing locked region of the file.

Arguments:

    hFile - Supplies an open handle to a file that is to have a range of
        bytes locked for exclusive access.  The handle must have been
        created with either GENERIC_READ or GENERIC_WRITE access to the
        file.

    dwFlags - Supplies flag bits that modify the behavior of this function.

        LOCKFILE_FAIL_IMMEDIATELY - if set, then this function will return
            immediately if it is unable to acquire the requested lock.
            Otherwise it will wait.

        LOCKFILE_EXCLUSIVE_LOCK - if set, then this function requests an
            exclusive lock, otherwise it requested a shared lock.

    dwReserved - Reserved parameter that must be zero.

    nNumberOfBytesToLockLow - Supplies the low order 32-bits of the length
        of the byte range to be locked.

    nNumberOfBytesToLockHigh - Supplies the high order 32-bits of the length
        of the byte range to be locked.

    lpOverlapped - Required pointer to an OVERLAPPED structure to be
        used with the request.  It contains the file offset of the
        beginning of the lock range.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/


{
    NTSTATUS Status;
    LARGE_INTEGER ByteOffset;
    LARGE_INTEGER Length;

    if (CONSOLE_HANDLE(hFile)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
        }

    if (dwReserved != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }

    ByteOffset.LowPart = lpOverlapped->Offset;
    ByteOffset.HighPart = lpOverlapped->OffsetHigh;

    Length.LowPart = nNumberOfBytesToLockLow;
    Length.HighPart = nNumberOfBytesToLockHigh;
    lpOverlapped->Internal = (DWORD)STATUS_PENDING;

    Status = NtLockFile( hFile,
                         lpOverlapped->hEvent,
                         NULL,
                         (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                         (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                         &ByteOffset,
                         &Length,
                         0,
                         (BOOLEAN)((dwFlags & LOCKFILE_FAIL_IMMEDIATELY) ? TRUE : FALSE),
                         (BOOLEAN)((dwFlags & LOCKFILE_EXCLUSIVE_LOCK) ? TRUE : FALSE)
                       );

    if ( NT_SUCCESS(Status) && Status != STATUS_PENDING) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}


BOOL
WINAPI
UnlockFile(
    HANDLE hFile,
    DWORD dwFileOffsetLow,
    DWORD dwFileOffsetHigh,
    DWORD nNumberOfBytesToUnlockLow,
    DWORD nNumberOfBytesToUnlockHigh
    )

/*++

Routine Description:

    A previously locked byte range within an open file may be Unlocked
    using UnlockFile.

    Unlocking a region of a file is used release a previously aquired
    lock on a file.  The region to unlock must exactly correspond to an
    existing locked region.  Two adjacent regions of a file can not be
    locked seperately and then be unlocked using a single region that
    spans both locked regions.

    If a process terminates with a portion of a file locked, or closes a
    file that has outstanding locks, the behavior is not specified.

    For DOS based systems running share.exe the lock semantics work as
    described above.  Without share.exe, all attempts to lock or unlock
    a file will fail.

Arguments:

    hFile - Supplies an open handle to a file that is to have an
        existing locked region unlocked.  The handle must have been
        created with either GENERIC_READ or GENERIC_WRITE access to the
        file.

    dwFileOffsetLow - Supplies the low order 32-bits of an existing
        locked region to be unlocked.

    dwFileOffsetHigh - Supplies the high order 32-bits of an existing
        locked region to be unlocked.

    nNumberOfBytesToUnlockLow - Supplies the low order 32-bits of the
        length of the byte range to be unlocked.

    nNumberOfBytesToUnlockHigh - Supplies the high order 32-bits of the
        length of the byte range to be unlocked.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    BOOL bResult;
    OVERLAPPED Overlapped;
    NTSTATUS Status;

    Overlapped.Offset = dwFileOffsetLow;
    Overlapped.OffsetHigh = dwFileOffsetHigh;
    bResult = UnlockFileEx( hFile,
                            0,
                            nNumberOfBytesToUnlockLow,
                            nNumberOfBytesToUnlockHigh,
                            &Overlapped
                          );
    if (!bResult && GetLastError() == ERROR_IO_PENDING) {
        Status = NtWaitForSingleObject( hFile, FALSE, NULL );
        if (NT_SUCCESS( Status )) {
            Status = (NTSTATUS)Overlapped.Internal;
            }

        if ( NT_SUCCESS(Status) ) {
            return TRUE;
            }
        else {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else {
        return bResult;
        }
}



BOOL
WINAPI
UnlockFileEx(
    HANDLE hFile,
    DWORD dwReserved,
    DWORD nNumberOfBytesToUnlockLow,
    DWORD nNumberOfBytesToUnlockHigh,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    A previously locked byte range within an open file may be Unlocked
    using UnlockFile.

    Unlocking a region of a file is used release a previously aquired
    lock on a file.  The region to unlock must exactly correspond to an
    existing locked region.  Two adjacent regions of a file can not be
    locked seperately and then be unlocked using a single region that
    spans both locked regions.

    If a process terminates with a portion of a file locked, or closes a
    file that has outstanding locks, the behavior is not specified.

Arguments:

    hFile - Supplies an open handle to a file that is to have an
        existing locked region unlocked.  The handle must have been
        created with either GENERIC_READ or GENERIC_WRITE access to the
        file.

    dwReserved - Reserved parameter that must be zero.

    nNumberOfBytesToUnlockLow - Supplies the low order 32-bits of the
        length of the byte range to be unlocked.

    nNumberOfBytesToUnlockHigh - Supplies the high order 32-bits of the
        length of the byte range to be unlocked.

    lpOverlapped - Required pointer to an OVERLAPPED structure to be
        used with the request.  It contains the file offset of the
        beginning of the lock range.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/


{
    NTSTATUS Status;
    LARGE_INTEGER ByteOffset;
    LARGE_INTEGER Length;

    if (CONSOLE_HANDLE(hFile)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
        }

    if (dwReserved != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }

    ByteOffset.LowPart = lpOverlapped->Offset;
    ByteOffset.HighPart = lpOverlapped->OffsetHigh;

    Length.LowPart = nNumberOfBytesToUnlockLow;
    Length.HighPart = nNumberOfBytesToUnlockHigh;

    Status = NtUnlockFile(
                hFile,
                (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                &ByteOffset,
                &Length,
                0
                );

    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

UINT
WINAPI
SetHandleCount(
    UINT uNumber
    )

/*++

Routine Description:

    This function changes the number of file handles available to a
    process.  For DOS based Win32, the default maximum number of file
    handles available to a process is 20.  For NT/Win32 systems, this
    API has no effect.

Arguments:

    uNumber - Specifies the number of file handles needed by the
        application.  The maximum is 255.

Return Value:

    The return value specifies the number of file handles actually
    available to the application.  It may be less than the number
    specified by the wNumber parameter.

--*/

{
    return uNumber;
}

DWORD
WINAPI
GetFileSize(
    HANDLE hFile,
    LPDWORD lpFileSizeHigh
    )

/*++

Routine Description:

    This function returns the size of the file specified by
    hFile. It is capable of returning 64-bits worth of file size.

    The return value contains the low order 32-bits of the file's size.
    The optional lpFileSizeHigh returns the high order 32-bits of the
    file's size.

Arguments:

    hFile - Supplies an open handle to a file whose size is to be
        returned.  The handle must have been created with either
        GENERIC_READ or GENERIC_WRITE access to the file.

    lpFileSizeHigh - An optional parameter, that if specified, returns
        the high order 64-bits of the file's size.


Return Value:

    Not -1 - Returns the low order 32-bits of the specified file's size.


    0xffffffff - If the value of size of the file cannot be determined,
        or an invalid handle or handle with inappropriate access, or a
        handle to a non-file is specified, this error is returned.  If
        the file's size (low 32-bits) is -1, then this value is
        returned, and GetLastError() will return 0.  Extended error
        status is available using GetLastError.


--*/

{
    BOOL b;
    LARGE_INTEGER Li;

    b = GetFileSizeEx(hFile,&Li);

    if ( b ) {

        if ( ARGUMENT_PRESENT(lpFileSizeHigh) ) {
            *lpFileSizeHigh = (DWORD)Li.HighPart;
            }
        if (Li.LowPart == -1 ) {
            SetLastError(0);
            }
        }
    else {
        Li.LowPart = -1;
        }

    return Li.LowPart;
}

BOOL
WINAPI
GetFileSizeEx(
    HANDLE hFile,
    PLARGE_INTEGER lpFileSize
    )

/*++

Routine Description:

    This function returns the size of the file specified by
    hFile. It is capable of returning 64-bits worth of file size.

Arguments:

    hFile - Supplies an open handle to a file whose size is to be
        returned.  The handle must have been created with either
        GENERIC_READ or GENERIC_WRITE access to the file.

    lpFileSize - Returns the files size


Return Value:

    TRUE - The operation was successful


    FALSE - The operation failed. Extended error
        status is available using GetLastError.


--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardInfo;

    Status = NtQueryInformationFile(
                hFile,
                &IoStatusBlock,
                &StandardInfo,
                sizeof(StandardInfo),
                FileStandardInformation
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    else {
        *lpFileSize = StandardInfo.EndOfFile;
        return TRUE;
        }
}

VOID
WINAPI
BasepIoCompletion(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    DWORD Reserved
    )

/*++

Routine Description:

    This procedure is called to complete ReadFileEx and WriteFileEx
    asynchronous I/O. Its primary function is to extract the
    appropriate information from the passed IoStatusBlock and call the
    users completion routine.

    The users completion routine is called as:

        Routine Description:

            When an outstanding I/O completes with a callback, this
            function is called.  This function is only called while the
            thread is in an alertable wait (SleepEx,
            WaitForSingleObjectEx, or WaitForMultipleObjectsEx with the
            bAlertable flag set to TRUE).  Returning from this function
            allows another pendiong I/O completion callback to be
            processed.  If this is the case, this callback is entered
            before the termination of the thread's wait with a return
            code of WAIT_IO_COMPLETION.

            Note that each time your completion routine is called, the
            system uses some of your stack.  If you code your completion
            logic to do additional ReadFileEx's and WriteFileEx's within
            your completion routine, AND you do alertable waits in your
            completion routine, you may grow your stack without ever
            trimming it back.

        Arguments:

            dwErrorCode - Supplies the I/O completion status for the
                related I/O.  A value of 0 indicates that the I/O was
                successful.  Note that end of file is indicated by a
                non-zero dwErrorCode value of ERROR_HANDLE_EOF.

            dwNumberOfBytesTransfered - Supplies the number of bytes
                transfered during the associated I/O.  If an error
                occured, a value of 0 is supplied.

            lpOverlapped - Supplies the address of the OVERLAPPED
                structure used to initiate the associated I/O.  The
                hEvent field of this structure is not used by the system
                and may be used by the application to provide additional
                I/O context.  Once a completion routine is called, the
                system will not use the OVERLAPPED structure.  The
                completion routine is free to deallocate the overlapped
                structure.

Arguments:

    ApcContext - Supplies the users completion routine. The format of
        this routine is an LPOVERLAPPED_COMPLETION_ROUTINE.

    IoStatusBlock - Supplies the address of the IoStatusBlock that
        contains the I/O completion status. The IoStatusBlock is
        contained within the OVERLAPPED structure.

    Reserved - Not used; reserved for future use.

Return Value:

    None.

--*/

{

    LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine;
    DWORD dwErrorCode;
    DWORD dwNumberOfBytesTransfered;
    LPOVERLAPPED lpOverlapped;

    dwErrorCode = 0;

    if ( NT_ERROR(IoStatusBlock->Status) ) {
        dwErrorCode = RtlNtStatusToDosError(IoStatusBlock->Status);
        dwNumberOfBytesTransfered = 0;
        }
    else {
        dwErrorCode = 0;
        dwNumberOfBytesTransfered = (DWORD)IoStatusBlock->Information;
        }

    CompletionRoutine = (LPOVERLAPPED_COMPLETION_ROUTINE)ApcContext;
    lpOverlapped = (LPOVERLAPPED)CONTAINING_RECORD(IoStatusBlock,OVERLAPPED,Internal);

    (CompletionRoutine)(dwErrorCode,dwNumberOfBytesTransfered,lpOverlapped);

    Reserved;
}



BOOL
WINAPI
ReadFileEx(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )

/*++

Routine Description:

    Data can be read from a file using ReadFileEx.

    This API reports its completion status asynchronously by calling the
    specified lpCompletionRoutine.

    The caller of this routine uses the lpOverlappedStructure to specify
    the byte offset within the file where the read is to begin from.
    For files that do not support this concept (pipes...), the starting
    file offset is ignored.

    Upon successful completion of this API (return value of TRUE), the
    calling thread has an I/O outstanding.  When the I/O completes, and
    the thread is blocked in an alertable wait, the lpCompletionRoutine
    will be called and the wait will return with a return code of
    WAIT_IO_COMPLETION.  If the I/O completes, but the thread issuing
    the I/O is not in an alertable wait, the call to the completion
    routine is queued until the thread executes an alertable wait.

    If this API fails (by returning FALSE), GetLastError can be used to
    get additional error information.  If this call fails because the
    thread issued a read beyond the end of file, GetLastError will
    return a value of ERROR_HANDLE_EOF.

Arguments:

    hFile - Supplies an open handle to a file that is to be read.  The
        file handle must have been created with GENERIC_READ access to
        the file.  The file must have been created with the
        FILE_FLAG_OVERLAPPED flag.

    lpBuffer - Supplies the address of a buffer to receive the data read
        from the file.

    nNumberOfBytesToRead - Supplies the number of bytes to read from the
        file.

    lpOverlapped - Supplies the address of an OVERLAPPED structure to be
        used with the request.  The caller of this function must specify
        a starting byte offset within the file to start the read from.
        It does this using the Offset and OffsetHigh fields of the
        overlapped structure.  This call does not use or modify the
        hEvent field of the overlapped structure.  The caller may use
        this field for any purpose.  This API does use the Internal and
        InternalHigh fields of the overlapped structure, the thread
        should not manipulate this.  The lpOverlapped structure must
        remain valid for the duration of the I/O.  It is not a good idea
        to make it a local variable and then possibly returning from the
        routine with the I/O that is using this structure still pending.

Return Value:

    TRUE - The operation was successul.  Completion status will be
        propagated to the caller using the completion callback
        mechanism.  Note that this information is only made available to
        the thread that issued the I/O, and only when the I/O completes,
        and the thread is executing in an alertable wait.

    FALSE - The operation failed.  Extended error status is available
        using GetLastError. Note that end of file is treated as a failure
        with an error code of ERROR_HANDLE_EOF.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER Li;

    Li.LowPart = lpOverlapped->Offset;
    Li.HighPart = lpOverlapped->OffsetHigh;

    Status = NtReadFile(
                hFile,
                NULL,
                BasepIoCompletion,
                (PVOID) lpCompletionRoutine,
                (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                lpBuffer,
                nNumberOfBytesToRead,
                &Li,
                NULL
                );
    if ( NT_ERROR(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    else {
        return TRUE;
        }
}

BOOL
WINAPI
WriteFileEx(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )

/*++

Routine Description:

    Data can be written to a file using WriteFileEx.

    This API reports its completion status asynchronously by calling the
    specified lpCompletionRoutine.

    The caller of this routine uses the lpOverlappedStructure to specify
    the byte offset within the file where the write is to begin.
    For files that do not support this concept (pipes...), the starting
    file offset is ignored.

    Upon successful completion of this API (return value of TRUE), the
    calling thread has an I/O outstanding.  When the I/O completes, and
    the thread is blocked in an alertable wait, the lpCompletionRoutine
    will be called and the wait will return with a return code of
    WAIT_IO_COMPLETION.  If the I/O completes, but the thread issuing
    the I/O is not in an alertable wait, the call to the completion
    routine is queued until the thread executes an alertable wait.

    If this API fails (by returning FALSE), GetLastError can be used to
    get additional error information.

    Unlike DOS, a NumberOfBytesToWrite value of zero does not truncate
    or extend the file.  If this function is required, SetEndOfFile
    should be used.

Arguments:

    hFile - Supplies an open handle to a file that is to be written.  The
        file handle must have been created with GENERIC_WRITE access to
        the file.

    lpBuffer - Supplies the address of the data that is to be written to
        the file.

    nNumberOfBytesToWrite - Supplies the number of bytes to write to the
        file. Unlike DOS, a value of zero is interpreted a null write.

    lpOverlapped - Supplies the address of an OVERLAPPED structure to be
        used with the request.  The caller of this function must specify
        a starting byte offset within the file to start the write to.
        It does this using the Offset and OffsetHigh fields of the
        overlapped structure.  This call does not use or modify the
        hEvent field of the overlapped structure.  The caller may use
        this field for any purpose.  This API does use the Internal and
        InternalHigh fields of the overlapped structure, the thread
        should not manipulate this.  The lpOverlapped structure must
        remain valid for the duration of the I/O.  It is not a good idea
        to make it a local variable and then possibly returning from the
        routine with the I/O that is using this structure still pending.

Return Value:

    TRUE - The operation was successul.  Completion status will be
        propagated to the caller using the completion callback
        mechanism.  Note that this information is only made available to
        the thread that issued the I/O, and only when the I/O completes,
        and the thread is executing in an alertable wait.

    FALSE - The operation failed.  Extended error status is available
        using GetLastError. Note that end of file is treated as a failure
        with an error code of ERROR_HANDLE_EOF.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER Li;

    Li.LowPart = lpOverlapped->Offset;
    Li.HighPart = lpOverlapped->OffsetHigh;

    Status = NtWriteFile(
                hFile,
                NULL,
                BasepIoCompletion,
                (PVOID) lpCompletionRoutine,
                (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                (LPVOID)lpBuffer,
                nNumberOfBytesToWrite,
                &Li,
                NULL
                );
    if ( NT_ERROR(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    else {
        return TRUE;
        }
}

BOOL
WINAPI
DeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    An operation on a device may be performed by calling the device driver
    directly using the DeviceIoContrl function.

    The device driver must first be opened to get a valid handle.

Arguments:

    hDevice - Supplies an open handle a device on which the operation is to
        be performed.

    dwIoControlCode - Supplies the control code for the operation. This
        control code determines on which type of device the operation must
        be performed and determines exactly what operation is to be
        performed.

    lpInBuffer - Suplies an optional pointer to an input buffer that contains
        the data required to perform the operation.  Whether or not the
        buffer is actually optional is dependent on the IoControlCode.

    nInBufferSize - Supplies the length of the input buffer in bytes.

    lpOutBuffer - Suplies an optional pointer to an output buffer into which
        the output data will be copied. Whether or not the buffer is actually
        optional is dependent on the IoControlCode.

    nOutBufferSize - Supplies the length of the output buffer in bytes.

    lpBytesReturned - Supplies a pointer to a dword which will receive the
        actual length of the data returned in the output buffer.

    lpOverlapped - An optional parameter that supplies an overlap structure to
        be used with the request. If NULL or the handle was created without
        FILE_FLAG_OVERLAPPED then the DeviceIoControl will not return until
        the operation completes.

        When lpOverlapped is supplied and FILE_FLAG_OVERLAPPED was specified
        when the handle was created, DeviceIoControl may return
        ERROR_IO_PENDING to allow the caller to continue processing while the
        operation completes. The event (or File handle if hEvent == NULL) will
        be set to the not signalled state before ERROR_IO_PENDING is
        returned. The event will be set to the signalled state upon completion
        of the request. GetOverlappedResult is used to determine the result
        when ERROR_IO_PENDING is returned.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{

    NTSTATUS Status;
    BOOLEAN DevIoCtl;

    if ( dwIoControlCode >> 16 == FILE_DEVICE_FILE_SYSTEM ) {
        DevIoCtl = FALSE;
        }
    else {
        DevIoCtl = TRUE;
        }

    if ( ARGUMENT_PRESENT( lpOverlapped ) ) {
        lpOverlapped->Internal = (DWORD)STATUS_PENDING;

        if ( DevIoCtl ) {

            Status = NtDeviceIoControlFile(
                        hDevice,
                        lpOverlapped->hEvent,
                        NULL,             // APC routine
                        (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                        (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );
            }
        else {

            Status = NtFsControlFile(
                        hDevice,
                        lpOverlapped->hEvent,
                        NULL,             // APC routine
                        (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                        (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );

            }

        // handle warning value STATUS_BUFFER_OVERFLOW somewhat correctly
        if ( !NT_ERROR(Status) && ARGUMENT_PRESENT(lpBytesReturned) ) {
            try {
                *lpBytesReturned = (DWORD)lpOverlapped->InternalHigh;
                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                *lpBytesReturned = 0;
                }
            }
        if ( NT_SUCCESS(Status) && Status != STATUS_PENDING) {
            return TRUE;
            }
        else {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else
        {
        IO_STATUS_BLOCK Iosb;

        if ( DevIoCtl ) {
            Status = NtDeviceIoControlFile(
                        hDevice,
                        NULL,
                        NULL,             // APC routine
                        NULL,             // APC Context
                        &Iosb,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );
            }
        else {
            Status = NtFsControlFile(
                        hDevice,
                        NULL,
                        NULL,             // APC routine
                        NULL,             // APC Context
                        &Iosb,
                        dwIoControlCode,  // IoControlCode
                        lpInBuffer,       // Buffer for data to the FS
                        nInBufferSize,
                        lpOutBuffer,      // OutputBuffer for data from the FS
                        nOutBufferSize    // OutputBuffer Length
                        );
            }

        if ( Status == STATUS_PENDING) {
            // Operation must complete before return & Iosb destroyed
            Status = NtWaitForSingleObject( hDevice, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = Iosb.Status;
                }
            }

        if ( NT_SUCCESS(Status) ) {
            *lpBytesReturned = (DWORD)Iosb.Information;
            return TRUE;
            }
        else {
            // handle warning value STATUS_BUFFER_OVERFLOW somewhat correctly
            if ( !NT_ERROR(Status) ) {
                *lpBytesReturned = (DWORD)Iosb.Information;
            }
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
}

BOOL
WINAPI
CancelIo(
    HANDLE hFile
    )

/*++

Routine Description:

    This routine cancels all of the outstanding I/O for the specified handle
    for the specified file.

Arguments:

    hFile - Supplies the handle to the file whose pending I/O is to be
        canceled.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed.  Extended error status is available using
        GetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Simply cancel the I/O for the specified file.
    //

    Status = NtCancelIoFile(hFile, &IoStatusBlock);

    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }

}

BOOL
WINAPI
ReadFileScatter(
    HANDLE hFile,
    FILE_SEGMENT_ELEMENT aSegementArray[],
    DWORD nNumberOfBytesToRead,
    LPDWORD lpReserved,
    LPOVERLAPPED lpOverlapped
    )
/*++

Routine Description:

    Data can be read from a file using ReadFileScatter.  The data
    is then scatter to specified buffer segements.

    This API is used to read data from a file.  Data is read from the
    file from the position indicated by the file pointer.  After the
    read completes, the file pointer is adjusted by the number of bytes
    actually read.  A return value of TRUE coupled with a bytes read of
    0 indicates that the file pointer was beyond the current end of the
    file at the time of the read.

Arguments:

    hFile - Supplies an open handle to a file that is to be read.  The
        file handle must have been created with GENERIC_READ access to
        the file.

    aSegementArray - Supplies a pointer an array of virtual segments.
        A virtual segment is a memory buffer where part of the transferred data
        should be placed.  Segments are have a fix size of PAGE_SIZE
        and must be aligned on a PAGE_SIZE boundary.

    nNumberOfBytesToRead - Supplies the number of bytes to read from the file.

    lpReserved - Reserved for now.

    lpOverlapped - Optionally points to an OVERLAPPED structure to be used with the
    request. If NULL then the transfer starts at the current file position
    and ReadFile will not return until the operation completes.

    If the handle hFile was created without specifying FILE_FLAG_OVERLAPPED
    the file pointer is moved to the specified offset plus
    lpNumberOfBytesRead before ReadFile returns. ReadFile will wait for the
    request to complete before returning (it will not return
    ERROR_IO_PENDING).

    When FILE_FLAG_OVERLAPPED is specified, ReadFile may return
    ERROR_IO_PENDING to allow the calling function to continue processing
    while the operation completes. The event (or hFile if hEvent is NULL) will
    be set to the signalled state upon completion of the request.

    When the handle is created with FILE_FLAG_OVERLAPPED and lpOverlapped
    is set to NULL, ReadFile will return ERROR_INVALID_PARAMTER because
    the file offset is required.


Return Value:

    TRUE - The operation was successul.

    FALSE - The operation failed.  Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LPDWORD lpNumberOfBytesRead = NULL;

    if ( ARGUMENT_PRESENT(lpReserved) ||
         !ARGUMENT_PRESENT( lpOverlapped )) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

        }

    if (CONSOLE_HANDLE(hFile)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
        }

    if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
        *lpNumberOfBytesRead = 0;
        }

    if ( ARGUMENT_PRESENT( lpOverlapped ) ) {
        LARGE_INTEGER Li;

        lpOverlapped->Internal = (DWORD)STATUS_PENDING;
        Li.LowPart = lpOverlapped->Offset;
        Li.HighPart = lpOverlapped->OffsetHigh;
        Status = NtReadFileScatter(
                hFile,
                lpOverlapped->hEvent,
                NULL,
                (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                aSegementArray,
                nNumberOfBytesToRead,
                &Li,
                NULL
                );


        if ( NT_SUCCESS(Status) && Status != STATUS_PENDING) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
                try {
                    *lpNumberOfBytesRead = (DWORD)lpOverlapped->InternalHigh;
                    }
                except(EXCEPTION_EXECUTE_HANDLER) {
                    *lpNumberOfBytesRead = 0;
                    }
                }
            return TRUE;
            }
        else
        if (Status == STATUS_END_OF_FILE) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesRead) ) {
                *lpNumberOfBytesRead = 0;
                }
            BaseSetLastNTError(Status);
            return FALSE;
            }
        else {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else
        {
        Status = NtReadFileScatter(
                hFile,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                aSegementArray,
                nNumberOfBytesToRead,
                NULL,
                NULL
                );

        if ( Status == STATUS_PENDING) {
            // Operation must complete before return & IoStatusBlock destroyed
            Status = NtWaitForSingleObject( hFile, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = IoStatusBlock.Status;
                }
            }

        if ( NT_SUCCESS(Status) ) {
            *lpNumberOfBytesRead = (DWORD)IoStatusBlock.Information;
            return TRUE;
            }
        else
        if (Status == STATUS_END_OF_FILE) {
            *lpNumberOfBytesRead = 0;
            return TRUE;
            }
        else {
            if ( NT_WARNING(Status) ) {
                *lpNumberOfBytesRead = (DWORD)IoStatusBlock.Information;
                }
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
}


BOOL
WINAPI
WriteFileGather(
    HANDLE hFile,
    FILE_SEGMENT_ELEMENT aSegementArray[],
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpReserved,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    Data can be written to a file using WriteFileGather.  The data can
    be in multple file segement buffers.

    This API is used to write data to a file.  Data is written to the
    file from the position indicated by the file pointer.  After the
    write completes, the file pointer is adjusted by the number of bytes
    actually written.

    Unlike DOS, a NumberOfBytesToWrite value of zero does not truncate
    or extend the file.  If this function is required, SetEndOfFile
    should be used.

Arguments:

    hFile - Supplies an open handle to a file that is to be written.  The
        file handle must have been created with GENERIC_WRITE access to
        the file.

    aSegementArray - Supplies a pointer an array of virtual segments.
        A virtual segment is a memory buffer where part of the transferred data
        should be placed.  Segments are have a fix size of PAGE_SIZE
        and must be aligned on a PAGE_SIZE boundary. The number of
        entries in the array must be equal to nNumberOfBytesToRead /
        PAGE_SIZE.

    nNumberOfBytesToWrite - Supplies the number of bytes to write to the
        file. Unlike DOS, a value of zero is interpreted a null write.

    lpReserved - Unused for now.

    lpOverlapped - Optionally points to an OVERLAPPED structure to be
        used with the request. If NULL then the transfer starts at the
        current file position and WriteFileGather will not return until the
        operation completes.

        If the handle <hFile> was created without specifying
        FILE_FLAG_OVERLAPPED the file pointer is moved to the specified
        offset plus lpNumberOfBytesWritten before WriteFile returns.
        WriteFile will wait for the request to complete before returning
        (it will not set ERROR_IO_PENDING).

        When FILE_FLAG_OVERLAPPED is specified, WriteFile may return
        ERROR_IO_PENDING to allow the calling function to continue processing
        while the operation completes. The event (or hFile if hEvent is NULL) will
        be set to the signalled state upon completion of the request.

        When the handle is created with FILE_FLAG_OVERLAPPED and lpOverlapped
        is set to NULL, WriteFile will return ERROR_INVALID_PARAMTER because
        the file offset is required.

Return Value:

    TRUE - The operation was a success.

    FALSE - The operation failed.  Extended error status is
        available using GetLastError.

--*/

{

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LPDWORD lpNumberOfBytesWritten = NULL;

    if ( ARGUMENT_PRESENT(lpReserved) ||
         !ARGUMENT_PRESENT( lpOverlapped )) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

        }

    if (CONSOLE_HANDLE(hFile)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
        }

    if ( ARGUMENT_PRESENT(lpNumberOfBytesWritten) ) {
        *lpNumberOfBytesWritten = 0;
        }

    if ( ARGUMENT_PRESENT( lpOverlapped ) ) {
        LARGE_INTEGER Li;

        lpOverlapped->Internal = (DWORD)STATUS_PENDING;
        Li.LowPart = lpOverlapped->Offset;
        Li.HighPart = lpOverlapped->OffsetHigh;
        Status = NtWriteFileGather(
                hFile,
                lpOverlapped->hEvent,
                NULL,
                (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                aSegementArray,
                nNumberOfBytesToWrite,
                &Li,
                NULL
                );

        if ( !NT_ERROR(Status) && Status != STATUS_PENDING) {
            if ( ARGUMENT_PRESENT(lpNumberOfBytesWritten) ) {
                try {
                    *lpNumberOfBytesWritten = (DWORD)lpOverlapped->InternalHigh;
                    }
                except(EXCEPTION_EXECUTE_HANDLER) {
                    *lpNumberOfBytesWritten = 0;
                    }
                }
            return TRUE;
            }
        else  {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else {
        Status = NtWriteFileGather(
                hFile,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                aSegementArray,
                nNumberOfBytesToWrite,
                NULL,
                NULL
                );

        if ( Status == STATUS_PENDING) {
            // Operation must complete before return & IoStatusBlock destroyed
            Status = NtWaitForSingleObject( hFile, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = IoStatusBlock.Status;
                }
            }

        if ( NT_SUCCESS(Status)) {
            *lpNumberOfBytesWritten = (DWORD)IoStatusBlock.Information;
            return TRUE;
            }
        else {
            if ( NT_WARNING(Status) ) {
                *lpNumberOfBytesWritten = (DWORD)IoStatusBlock.Information;
                }
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
}
