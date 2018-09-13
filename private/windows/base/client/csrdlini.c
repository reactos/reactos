/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    csrdlini.c

Abstract:

    This module implements functions that are used by the Win32 Initialization
    File APIs to communicate with csrss.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/

#include "basedll.h"

NTSTATUS
CsrBasepRefreshIniFileMapping(
    PUNICODE_STRING BaseFileName
    )
{       
    
#if defined(BUILD_WOW6432)
    return NtWow64CsrBasepRefreshIniFileMapping(BaseFileName); 
#else

    NTSTATUS Status;

    BASE_API_MSG m;
    PBASE_REFRESHINIFILEMAPPING_MSG ap = &m.u.RefreshIniFileMapping;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CaptureBuffer = NULL;

    if (BaseFileName->Length > (MAX_PATH * sizeof( WCHAR ))) {
        return STATUS_INVALID_PARAMETER;
    }

    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                              BaseFileName->MaximumLength
                                            );
    if (CaptureBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }
    
    CsrCaptureMessageString( CaptureBuffer,
                             (PCHAR)BaseFileName->Buffer,
                             BaseFileName->Length,
                             BaseFileName->MaximumLength,
                             (PSTRING)&ap->IniFileName
                          );

   CsrClientCallServer( (PCSR_API_MSG)&m,
                        CaptureBuffer,
                        CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                             BasepRefreshIniFileMapping
                                           ),
                        sizeof( *ap )
                     );

   Status = (NTSTATUS)m.ReturnValue;

   if (CaptureBuffer != NULL) {
       CsrFreeCaptureBuffer( CaptureBuffer );
   }

   return Status;

#endif
   
}


