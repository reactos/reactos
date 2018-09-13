/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mailslot.c

Abstract:

    This module contains the Win32 Mailslot API

Author:

    Manny Weiser (mannyw) 4-Mar-1991

Revision History:

--*/

#include "basedll.h"

HANDLE
APIENTRY
CreateMailslotW(
    IN LPCWSTR lpName,
    IN DWORD nMaxMessageSize,
    IN DWORD lReadTimeout,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes OPTIONAL
    )

/*++

Routine Description:

    The create mailslot API creates a local mailslot and return a
    server-side handle to the mailslot.

Arguments:

    lpName - The name of the mailslot.  This must be a local mailslot
        name.

    nMaxMessageSize - The size (in bytes) of the largest message that
        can be written to the mailslot.

    lReadTimeout - The initial read timeout, in milliseconds.  This
        is the amount of time a read operation will block waiting for
        a message to be written to the mailslot.  This value can be
        changed with the SetMailslotInfo API.

    lpSecurityAttributes - An optional pointer to security information
        for this mailslot.

Return Value:

    Returns one of the following:

    0xFFFFFFFF --An error occurred.  Call GetLastError for more
    information.

    Anything else --Returns a handle for use in the server side of
    subsequent mailslot operations.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING fileName;
    LPWSTR filePart;
    RTL_RELATIVE_NAME relativeName;
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER readTimeout;
    HANDLE handle;
    NTSTATUS status;
    PVOID freeBuffer;
    BOOLEAN TranslationStatus;

    RtlInitUnicodeString( &fileName, lpName );

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpName,
                            &fileName,
                            &filePart,
                            &relativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    freeBuffer = fileName.Buffer;

    if ( relativeName.RelativeName.Length ) {
        fileName = *(PUNICODE_STRING)&relativeName.RelativeName;
    } else {
        relativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        relativeName.ContainingDirectory,
        NULL
        );

    if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
        objectAttributes.SecurityDescriptor =
            lpSecurityAttributes->lpSecurityDescriptor;
        if ( lpSecurityAttributes->bInheritHandle ) {
            objectAttributes.Attributes |= OBJ_INHERIT;
        }
    }

    if (lReadTimeout == MAILSLOT_WAIT_FOREVER) {
        readTimeout.HighPart = 0x7FFFFFFF;
        readTimeout.LowPart = 0xFFFFFFFF;
    } else {
        readTimeout.QuadPart = - (LONGLONG)UInt32x32To64( lReadTimeout, 10 * 1000 );
    }

    status = NtCreateMailslotFile (
                &handle,
                GENERIC_READ | SYNCHRONIZE | WRITE_DAC,
                &objectAttributes,
                &ioStatusBlock,
                FILE_CREATE,
                0,
                nMaxMessageSize,
                (PLARGE_INTEGER)&readTimeout
                );

    if ( status == STATUS_NOT_SUPPORTED ||
         status == STATUS_INVALID_DEVICE_REQUEST ) {

        //
        // The request must have been processed by some other device driver
        // (other than MSFS).  Map the error to something reasonable.
        //

        status = STATUS_OBJECT_NAME_INVALID;
    }

    RtlFreeHeap( RtlProcessHeap(), 0, freeBuffer );

    if (!NT_SUCCESS(status)) {
        BaseSetLastNTError( status );
        return INVALID_HANDLE_VALUE;
    }

    return handle;
}


HANDLE
APIENTRY
CreateMailslotA(
    IN LPCSTR lpName,
    IN DWORD nMaxMessageSize,
    IN DWORD lReadTimeout,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes OPTIONAL
    )
{
    PUNICODE_STRING unicode;
    ANSI_STRING ansiString;
    NTSTATUS status;

    unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString( &ansiString, lpName );
    status = RtlAnsiStringToUnicodeString( unicode, &ansiString, FALSE );

    if ( !NT_SUCCESS( status ) ) {
        if ( status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
        } else {
            BaseSetLastNTError(status);
        }
        return INVALID_HANDLE_VALUE;
    }

    return ( CreateMailslotW( unicode->Buffer,
                              nMaxMessageSize,
                              lReadTimeout,
                              lpSecurityAttributes
                              ) );

}

BOOL
APIENTRY
GetMailslotInfo(
    IN HANDLE hMailslot,
    OUT LPDWORD lpMaxMessageSize OPTIONAL,
    OUT LPDWORD lpNextSize OPTIONAL,
    OUT LPDWORD lpMessageCount OPTIONAL,
    OUT LPDWORD lpReadTimeout OPTIONAL
    )

/*++

Routine Description:

    This function will return the requested information about the
    specified mailslot.

Arguments:

    hMailslot - A handle to the mailslot.

    lpMaxMessageSize - If specified returns the size of the largest
        message that can be written to the mailslot.

    lpNextSize - If specified returns the size of the next message in
        the mailslot buffer.  It will return MAILSLOT_NO_MESSAGE if
        there are no messages in the mailslot.

    lpMessageCount - If specified returns the number of unread message
        currently in the mailslot.

    lpReadTimeout - If specified returns the read timeout, in
        milliseconds.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_MAILSLOT_QUERY_INFORMATION mailslotInfo;
    LARGE_INTEGER millisecondTimeout, tmp;

    status = NtQueryInformationFile( hMailslot,
                                     &ioStatusBlock,
                                     &mailslotInfo,
                                     sizeof( mailslotInfo ),
                                     FileMailslotQueryInformation );

    if ( !NT_SUCCESS( status ) ) {
        BaseSetLastNTError( status );
        return ( FALSE );
    }

    if ( ARGUMENT_PRESENT( lpMaxMessageSize ) ) {
        *lpMaxMessageSize = mailslotInfo.MaximumMessageSize;
    }

    if ( ARGUMENT_PRESENT( lpNextSize ) ) {
        *lpNextSize = mailslotInfo.NextMessageSize;
    }

    if ( ARGUMENT_PRESENT( lpMessageCount ) ) {
        *lpMessageCount = mailslotInfo.MessagesAvailable;
    }

    if ( ARGUMENT_PRESENT( lpReadTimeout ) ) {

        //
        // Convert read timeout from 100 ns intervals to milliseconds.
        // The readtime is currently negative, since it is a relative time.
        //

        if ( mailslotInfo.ReadTimeout.HighPart != 0x7FFFFFFF
             || mailslotInfo.ReadTimeout.LowPart != 0xFFFFFFFF ) {

            tmp.QuadPart = - mailslotInfo.ReadTimeout.QuadPart;
            millisecondTimeout = RtlExtendedLargeIntegerDivide(
                                     tmp,
                                     10 * 1000,
                                     NULL );

            if ( millisecondTimeout.HighPart == 0 ) {
                *lpReadTimeout = millisecondTimeout.LowPart;
            } else {

                //
                // The millisecond calculation would overflow the dword.
                // Approximate a large number as best we can.
                //

                *lpReadTimeout = 0xFFFFFFFE;

            }

        } else {

            //
            // The mailslot timeout is infinite.
            //

            *lpReadTimeout = MAILSLOT_WAIT_FOREVER;

        }
    }

    return( TRUE );
}

BOOL
APIENTRY
SetMailslotInfo(
    IN HANDLE hMailslot,
    IN DWORD lReadTimeout
    )

/*++

Routine Description:

    This function will set the read timeout for the specified mailslot.

Arguments:

    hMailslot - A handle to the mailslot.

    lReadTimeout - The new read timeout, in milliseconds.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_MAILSLOT_SET_INFORMATION mailslotInfo;
    LARGE_INTEGER timeout;

    if ( lReadTimeout == MAILSLOT_WAIT_FOREVER ) {
        timeout.HighPart = 0x7FFFFFFF;
        timeout.LowPart = 0xFFFFFFFF;
    } else {
        timeout.QuadPart = - (LONGLONG)UInt32x32To64( lReadTimeout, 10 * 1000 );
    }

    mailslotInfo.ReadTimeout = &timeout;
    status = NtSetInformationFile( hMailslot,
                                   &ioStatusBlock,
                                   &mailslotInfo,
                                   sizeof( mailslotInfo ),
                                   FileMailslotSetInformation );

    if ( !NT_SUCCESS( status ) ) {
        BaseSetLastNTError( status );
        return ( FALSE );
    }

    return TRUE;
}
