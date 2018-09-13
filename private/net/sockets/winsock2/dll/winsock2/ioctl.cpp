/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    ioctl.c

Abstract:

    This module contains the socket I/O control calls of the winsock
    API.  The following functions are contained in ths module.

    ioctlsocket()
    WSAIoctl()

Author:

    Dirk Brandewie dirk@mink.intel.com  14-06-1995

Revision History:

    22-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved includes to precomp.h

--*/

#include "precomp.h"



int WSAAPI
ioctlsocket (
    IN SOCKET s,
    IN long cmd,
    IN OUT u_long FAR *argp
    )
/*++
Routine Description:

    Control the mode of a socket.

Arguments:

    s - A descriptor identifying a socket.

    cmd - The command to perform on the socket s.

    argp - A pointer to a parameter for cmd.

Returns:
   Upon successful completion, the ioctlsocket() returns 0.
   Otherwise, a value of SOCKET_ERROR is returned, and a specific
   error code is stored with SetErrorCode().

--*/
{
    DWORD DontCare;

    return(WSAIoctl(
        s,                     // Socket handle
        cmd,                   //Command
        argp,                  // Input buffer
        sizeof(unsigned long), // Input buffer size
        argp,                  // Output buffer
        sizeof(unsigned long), // Output buffer size
        &DontCare,             // bytes returned
        NULL,                  // Overlapped struct
        NULL                   // Completion Routine
        ));
}




int WSAAPI
WSAIoctl(
    IN SOCKET s,
    IN DWORD dwIoControlCode,
    IN LPVOID lpvInBuffer,
    IN DWORD cbInBuffer,
    OUT LPVOID lpvOutBuffer,
    OUT DWORD cbOutBuffer,
    OUT LPDWORD lpcbBytesReturned,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
/*++
Routine Description:

    Control the mode of a socket.

Arguments:

    s - Handle to a socket

    dwIoControlCode - Control code of operation to perform

    lpvInBuffer - Address of input buffer

    cbInBuffer - Size of input buffer

    lpvOutBuffer - Address of output buffer

    cbOutBuffer - Size of output buffer

    lpcbBytesReturned - Address of actual bytes of output

    lpOverlapped - Address of WSAOVERLAPPED structure

    lpCompletionRoutine - A pointer to the completion routine called
                          when the operation  has been completed.

Returns:
    Zero on success else SOCKET_ERROR. The error code is stored with
    SetErrorCode().
--*/
{
    LPWSATHREADID      ThreadId;
    INT                ErrorCode, ReturnValue = ERROR_SUCCESS;
    PDPROVIDER         Provider;
    PDSOCKET           Socket;

	ErrorCode = TURBO_PROLOG_OVLP(&ThreadId);
    if (ErrorCode==ERROR_SUCCESS)
	{
        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPIoctl(
                s,
                dwIoControlCode,
                lpvInBuffer,
                cbInBuffer,
                lpvOutBuffer,
                cbOutBuffer,
                lpcbBytesReturned,
                lpOverlapped,
                lpCompletionRoutine,
                ThreadId,
                &ErrorCode);
            Socket->DropDSocketReference();
            if (ReturnValue==ERROR_SUCCESS)
                return ReturnValue;
        } //if
        else {
            ErrorCode = WSAENOTSOCK;
        }
    }

    SetLastError(ErrorCode);
    return(SOCKET_ERROR);
}




