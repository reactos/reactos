/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    qos.c

Abstract:

    This modules contains the quality of service related entrypoints
    from the winsock API.  This module contains the following functions.

    WSAGetQosByName()


Author:

    Dirk Brandewie dirk@mink.intel.com  14-06-1995

Revision History:


--*/

#include "precomp.h"


BOOL WSAAPI
WSAGetQOSByName(
                SOCKET s,
                LPWSABUF lpQOSName,
                LPQOS lpQOS
                )
/*++
Routine Description:

     Initializes the QOS based on a template.

Arguments:

    s - A descriptor identifying a socket.

    lpQOSName - Specifies the QOS template name.

    lpQOS - A pointer to the QOS structure to be filled.

Returns:
    If the function succeeds, the return value is TRUE.  If the
    function fails, the return value is FALSE.
--*/
{
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDSOCKET            Socket;
    BOOL                ReturnValue;


    ErrorCode = TURBO_PROLOG();
    if (ErrorCode==ERROR_SUCCESS) {

        Socket = DSOCKET::GetCountedDSocketFromSocket(s);
        if(Socket != NULL){
            Provider = Socket->GetDProvider();
            ReturnValue = Provider->WSPGetQOSByName( s,
                                       lpQOSName,
                                       lpQOS,
                                       &ErrorCode);
            Socket->DropDSocketReference();
            if (ReturnValue)
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
    return FALSE;
}

