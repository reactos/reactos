/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    perthrd.c

Abstract:

    This module contains the winsock API functions that query and set
    per thread information contained in winsock DLL. The following
    functions are contained in this module.

    WSAGetLastError()
    WSASetLastError()

Author:

    Dirk Brandewie dirk@mink.intel.com  14-06-1995

[Environment:]

[Notes:]

Revision History:


--*/

#include "precomp.h"


int WSAAPI
WSAGetLastError(
    IN void
    )
/*++
Routine Description:

    Get the error status for the last operation which failed.

Arguments:

    NONE

Returns:
    The return value indicates the error code for the last failed WinSock
    routine performed by this thread.

--*/
{
    return(GetLastError());
}




void WSAAPI
WSASetLastError(
    IN int iError
    )
/*++
Routine Description:

    Set the error code which can be retrieved by WSAGetLastError().

Arguments:

    iError - Specifies the error code to be returned by a subsequent
             WSAGetLastError() call.

Returns:
    NONE

--*/
{
    SetLastError(iError);
}

