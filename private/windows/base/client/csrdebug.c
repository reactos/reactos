/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    csrpro.c

Abstract:

    This module implements functions that are used by the Win32 Debug APIs
    to communicate with csrss.

Author:

    Michael Zoran (mzoran) 7-Jan-1998

Revision History:

--*/

#include "basedll.h"

NTSTATUS
CsrBasepDebugProcess(
    IN CLIENT_ID DebuggerClientId,
    IN DWORD dwProcessId,
    IN PVOID AttachCompleteRoutine
    )
{

#if defined(BUILD_WOW6432)

    return NtWow64CsrBasepDebugProcess(DebuggerClientId,
                                       dwProcessId,
                                       AttachCompleteRoutine);

#else

    BASE_API_MSG m;
    PBASE_DEBUGPROCESS_MSG a = (PBASE_DEBUGPROCESS_MSG)&m.u.DebugProcess;

   
    a->dwProcessId = dwProcessId;
    a->DebuggerClientId = DebuggerClientId;
    a->AttachCompleteRoutine = AttachCompleteRoutine;
 
    CsrClientCallServer( (PCSR_API_MSG)&a,
                         NULL,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepDebugProcess
                                              ),
                         sizeof( BASE_DEBUGPROCESS_MSG )
                       );
   
    return m.ReturnValue;

#endif

}
