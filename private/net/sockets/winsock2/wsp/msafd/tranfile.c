/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    tranfile.c

Abstract:

    This module contains support TransmitFile() extended
    WinSock API.

Author:

    David Treadwell (davidtr)    3-Aug-1994

Revision History:

--*/

#include "winsockp.h"


BOOL
TransmitFile (
    IN SOCKET hSocket,
    IN HANDLE hFile,
    IN DWORD nNumberOfBytesToWrite,
    IN DWORD nNumberOfBytesPerSend,
    IN LPOVERLAPPED lpOverlapped,
    IN LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Transmits file data over a connected socket handle by interfacing
    with the cache manager to retrieve the file data.  Use this routine
    when high performance file data transfer over sockets is required.

Arguments:

    hSocket - a connected socket handle.  The socket may be a datagram
        socket or a virtual circuit socket.

    hFile - an open file handle.  Since the file data is read
        sequentially, it is recommended that the handle be opened with
        FILE_FLAG_SEQUENTIAL_SCAN to improve caching performance.

    nNumberOfBytesToWrite - the count of bytes to send from the file.
        This request will not complete until the entire requested amount
        has been sent, or until an error is encountered.  If this
        parameter is equal to zero, then the entire file is transmited.

    nNumberOfBytesPerSend - informs the sockets layer of the size to use
        for each send it performs.  The value 0 indicates that an
        intelligent default should be used.  This is mainly useful for
        datagram or message protocols that have limitations of the size
        of individual send requests.

    lpOverlapped - specify this parameter when the socket handle is
        opened as overlapped to achieve overlapped I/O.  Note that by
        default sockets are opened as overlapped.

        Also use this parameter to specify an offset within the file at
        which to start the file data transfer.  The file pointer is
        ignored by this API, so if lpOverlapped is NULL the transmission
        always starts at offset 0 in the file.

        When lpOverlapped is non-NULL, TransmitFile() may return
        ERROR_IO_PENDING to allow the calling function to continue
        processing while the operation completes.  The event (or hSocket
        if hEvent is NULL) will be set to the signalled state upon
        completion of the request.

    lpTransmitBuffers - an optional pointer to a TRANSMIT_FILE_BUFFERS
        structure which contains pointers to data to send before the
        file (the head buffer) and after the file (the tail buffer).  If
        only the file is to be sent, this parameter may be specified as
        NULL.

    dwFlags - any combination of the following:

        TF_DISCONNECT - start a transport-level disconnect after all the
            file data has been queued to the transport.

        TF_REUSE_SOCKET - prepare the socket handle to be reused.  When
            the TransmitFile request completes, the socket handle may be
            passed to the SuperAccept API.  Only valid if TF_DISCONNECT
            is also specified.

        TF_WRITE_BEHIND - complete the TransmitFile request immediately,
            without pending.  If this flag is specified and TransmitFile
            succeeds, then the data has been accepted by the system but not
            necessarily acknowledged by the remote end.  Do not use this
            setting with the other two settings.

Return Value:

    TRUE - The operation was successul.

    FALSE - The operation failed.  Extended error status is available
        using GetLastError().

--*/

{

    AFD_TRANSMIT_FILE_INFO transmitInfo;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // Set up general imformation about the request.
    //

    transmitInfo.WriteLength.QuadPart = nNumberOfBytesToWrite;
    transmitInfo.SendPacketLength = nNumberOfBytesPerSend;
    transmitInfo.FileHandle = hFile;
    transmitInfo.Flags = dwFlags;

    __try {
        if ( ARGUMENT_PRESENT( lpTransmitBuffers ) ) {
            transmitInfo.Head = lpTransmitBuffers->Head;
            transmitInfo.HeadLength = lpTransmitBuffers->HeadLength;
            transmitInfo.Tail = lpTransmitBuffers->Tail;
            transmitInfo.TailLength = lpTransmitBuffers->TailLength;
        } else {
            transmitInfo.Head = NULL;
            transmitInfo.HeadLength = 0;
            transmitInfo.Tail = NULL;
            transmitInfo.TailLength = 0;
        }

        //
        // Handle the request differently based on whether an lpOverlapped
        // structure was specified.  If it was specified, then the socket
        // handle must have been opened for asynchronous access.
        //

        if ( ARGUMENT_PRESENT( lpOverlapped ) ) {

            lpOverlapped->Internal = (UINT_PTR)STATUS_PENDING;

            transmitInfo.Offset.LowPart = lpOverlapped->Offset;
            transmitInfo.Offset.HighPart = lpOverlapped->OffsetHigh;

            //
            // Call AFD to transmit the file.  Note that we pass a smaller
            // input buffer than the full structure in order to save the
            // input buffer copy in the I/O system.  Also note that we do
            // pass an output buffer: this ensures that the I/O system
            // allocates a large enough system buffer for AFD to use as a
            // scratch buffer.
            //

            status = NtDeviceIoControlFile(
                         (HANDLE)hSocket,
                         lpOverlapped->hEvent,
                         NULL,
                         (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                         (PIO_STATUS_BLOCK)(&lpOverlapped->Internal),
                         IOCTL_AFD_TRANSMIT_FILE,
                         &transmitInfo,
                         sizeof(transmitInfo),
                         NULL,
                         0
                         );

            if ( NT_SUCCESS(status) && status != STATUS_PENDING) {
                return TRUE;
#ifdef _AFD_SAN_SWITCH_
			} else if (SockSanEnabled && status == STATUS_INVALID_PARAMETER_12) {
				//
				// STATUS_INVALID_PARAMETER_12 is indication from AFD that
				// this socket is connected thru SAN
				//
				return SanTransmitFile(hSocket,
									   hFile,
									   nNumberOfBytesToWrite,
									   nNumberOfBytesPerSend,
									   lpOverlapped,
									   lpTransmitBuffers,
									   dwFlags);
#endif // _AFD_SAN_SWITCH_
            } else {
                SetLastError( SockNtStatusToSocketError(status) );
                return FALSE;
            }

        } else {

            transmitInfo.Offset.LowPart = 0;
            transmitInfo.Offset.HighPart = 0;

            //
            // Call AFD to transmit the file.  Note that we pass a smaller
            // input buffer than the full structure in order to save the
            // input buffer copy in the I/O system.  Also note that we do
            // pass an output buffer: this ensures that the I/O system
            // allocates a large enough system buffer for AFD to use as a
            // scratch buffer.
            //

            status = NtDeviceIoControlFile(
                         (HANDLE)hSocket,
                         NULL,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         IOCTL_AFD_TRANSMIT_FILE,
                         &transmitInfo,
                         sizeof(transmitInfo),
                         NULL,
                         0
                         );
            if ( status == STATUS_PENDING) {
                // Operation must complete before return & IoStatusBlock destroyed
                status = NtWaitForSingleObject( (HANDLE)hSocket, FALSE, NULL );
                if ( NT_SUCCESS(status)) {
                    status = ioStatusBlock.Status;
                }
            }

            if ( NT_SUCCESS(status) ) {
                return TRUE;
#ifdef _AFD_SAN_SWITCH_
			} else if (SockSanEnabled && status == STATUS_INVALID_PARAMETER_12) {
				//
				// STATUS_INVALID_PARAMETER_12 is indication from AFD that
				// this socket is connected thru SAN
				//
				return SanTransmitFile(hSocket,
									   hFile,
									   nNumberOfBytesToWrite,
									   nNumberOfBytesPerSend,
									   lpOverlapped,
									   lpTransmitBuffers,
									   dwFlags);
#endif // _AFD_SAN_SWITCH_
            } else {
                SetLastError( SockNtStatusToSocketError(status) );
                return FALSE;
            }
        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        SetLastError (WSAEFAULT);
        return FALSE;
    }

} // TransmitFile
