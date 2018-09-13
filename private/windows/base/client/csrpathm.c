/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    csrpathm.c

Abstract:

    This module implements functions that are used by the Win32 path functions
    to communicate with csrss.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/

#include "basedll.h"

UINT
CsrBasepGetTempFile(
    VOID
    )
{

#if defined(BUILD_WOW6432)
    return NtWow64CsrBasepGetTempFile();
#else   
    
    BASE_API_MSG m;
    PBASE_GETTEMPFILE_MSG a = (PBASE_GETTEMPFILE_MSG)&m.u.GetTempFile;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepGetTempFile
                                            ),
                         sizeof( *a )
                      );
    
    return (UINT)m.ReturnValue;
#endif
}


