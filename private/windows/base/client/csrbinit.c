/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    csrbinit.c

Abstract:

    This module implements functions that are used during Win32 base initialization to
    communicate with csrss.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/

#include "basedll.h"

NTSTATUS
CsrBaseClientConnectToServer(
    PWSTR szSessionDir,
    PHANDLE phMutant,
    PBOOLEAN pServerProcess
    )
{

#if defined(BUILD_WOW6432)
   return NtWow64CsrBaseClientConnectToServer(szSessionDir,
                                              phMutant,
                                              pServerProcess
                                              );
#else

    BASE_API_MSG m;
    PBASE_CREATETHREAD_MSG a = (PBASE_CREATETHREAD_MSG)&m.u.CreateThread;
    NTSTATUS Status;
    ULONG SizeMutant;
    SizeMutant = sizeof(HANDLE);

    Status = CsrClientConnectToServer( szSessionDir,
                                       BASESRV_SERVERDLL_INDEX,
                                       NULL,
                                       phMutant,
                                       &SizeMutant,
                                       pServerProcess
                                     );


    return m.ReturnValue;

#endif

}


