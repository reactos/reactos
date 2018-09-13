/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    dupsock.cpp

Abstract:

    This   module   contains   the   winsock   API   functions   dealing   with
    duplicating/sharing sockets.  The following functions are contained in this
    module.

    WSADuplicateSocketA()
    WSADuplicateSocketW()

Author:

    Dirk Brandewie dirk@mink.intel.com  14-06-1995


Revision History:

    09-19-95  drewsxpa@ashland.intel.com
        Changed over to C++, actually implemented the function

--*/


#include "precomp.h"




int WSAAPI
WSADuplicateSocketW(
    IN  SOCKET          s,
    IN  DWORD           dwProcessId,
    OUT LPWSAPROTOCOL_INFOW lpProtocolInfo
    )
/*++

Routine Description:

    Return a WSAPROTOCOL_INFOW structure that can be used to create a new socket
    descriptor for a shared socket.

Arguments:

    s              - Supplies the local socket descriptor.

    dwProcessId    - Supplies the ID of the target process for which the shared
                     socket will be used.

    lpProtocolInfo - Returns a WSAPROTOCOL_INFOW struct identifying the socket
                     in the target process.

Return Value:

    If  the  function  is  successful,  it  returns ERROR_SUCCESS, otherwise it
    returns  SOCKET_ERROR,  and  a specific error message can be retrieved with
    WSAGetLastError().
--*/
{
    INT        ErrorCode, ReturnValue;
    PDPROVIDER Provider;
    PDSOCKET   Socket;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode == ERROR_SUCCESS) {

        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPDuplicateSocket(
                s,
                dwProcessId,
                lpProtocolInfo,
                & ErrorCode);
            Socket->DropDSocketReference();
            if (ReturnValue==ERROR_SUCCESS)
                return ReturnValue;
            assert (ErrorCode!=NO_ERROR);
            if (ErrorCode==NO_ERROR)
                ErrorCode = WSASYSCALLFAILURE;
        }
        else {
            ErrorCode = WSAENOTSOCK;
        }
    }

    SetLastError(ErrorCode);
    return SOCKET_ERROR;
} // WSADuplicateSocketW




int WSAAPI
WSADuplicateSocketA(
    IN  SOCKET          s,
    IN  DWORD           dwProcessId,
    OUT LPWSAPROTOCOL_INFOA lpProtocolInfo
    )
/*++

Routine Description:

    ANSI thunk to WSADuplicateSocketW.

Arguments:

    s              - Supplies the local socket descriptor.

    dwProcessId    - Supplies the ID of the target process for which the shared
                     socket will be used.

    lpProtocolInfo - Returns a WSAPROTOCOL_INFOA struct identifying the socket
                     in the target process.

Return Value:

    If  the  function  is  successful,  it  returns ERROR_SUCCESS, otherwise it
    returns  SOCKET_ERROR,  and  a specific error message can be retrieved with
    WSAGetLastError().
--*/
{

    INT               result;
    INT               error;
    WSAPROTOCOL_INFOW ProtocolInfoW;

    //
    // Call through to the UNICODE version.
    //

    result = WSADuplicateSocketW(
                 s,
                 dwProcessId,
                 &ProtocolInfoW
                 );

    if( result == ERROR_SUCCESS ) {

        //
        // Map the UNICODE WSAPROTOCOL_INFOW to ANSI.
        //

        if( lpProtocolInfo == NULL ) {

            error = WSAEFAULT;

        } else {

            error = MapUnicodeProtocolInfoToAnsi(
                        &ProtocolInfoW,
                        lpProtocolInfo
                        );

        }

        if( error != ERROR_SUCCESS ) {

            SetLastError( error );
            result = SOCKET_ERROR;

        }

    }

    return result;

} // WSADuplicateSocketA

