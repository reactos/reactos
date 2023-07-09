/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    csrthrd.c

Abstract:

    This module implements functions that are used by the Win32 Thread Object APIs
    to communicate with csrss.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/
#include "basedll.h"

NTSTATUS
CsrBasepCreateThread(
    HANDLE ThreadHandle,
    CLIENT_ID ClientId
    )
{

#if defined(BUILD_WOW6432)
    return NtWow64CsrBasepCreateThread(ThreadHandle,
                                       ClientId);
#else

    BASE_API_MSG m;
    PBASE_CREATETHREAD_MSG a = (PBASE_CREATETHREAD_MSG)&m.u.CreateThread;

    a->ThreadHandle = ThreadHandle;
    a->ClientId = ClientId;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepCreateThread
                                            ),
                         sizeof( *a )
                       );

    return m.ReturnValue;

#endif
}


