/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    csrdosdv.c

Abstract:

    This module implements functions that are used by the DefineDosDevice API
    to communicate with csrss.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/

#include "basedll.h"

NTSTATUS
CsrBasepDefineDosDevice(
    DWORD dwFlags,
    PUNICODE_STRING pDeviceName,
    PUNICODE_STRING pTargetPath
    )
{

#if defined(BUILD_WOW6432)
   return NtWow64CsrBasepDefineDosDevice(dwFlags,
                                         pDeviceName,
                                         pTargetPath
                                         );
#else

    BASE_API_MSG m;
    PBASE_DEFINEDOSDEVICE_MSG a = (PBASE_DEFINEDOSDEVICE_MSG)&m.u.DefineDosDeviceApi;
    PCSR_CAPTURE_HEADER p;
    ULONG PointerCount, n;   
 
    if (0 == pTargetPath->MaximumLength) {
       PointerCount = 1;
       n = pDeviceName->MaximumLength;
    } 
    else {
       PointerCount = 2;
       n = pDeviceName->MaximumLength + pTargetPath->MaximumLength;
    }

    p = CsrAllocateCaptureBuffer( PointerCount, n );
    if (p == NULL) {
        return STATUS_NO_MEMORY;
    }

    a->Flags = dwFlags;
    a->DeviceName.MaximumLength =
        (USHORT)CsrAllocateMessagePointer( p,
                                           pDeviceName->MaximumLength,
                                           (PVOID *)&a->DeviceName.Buffer
                                         );
    RtlUpcaseUnicodeString( &a->DeviceName, pDeviceName, FALSE );
    if (pTargetPath->Length != 0) {
        a->TargetPath.MaximumLength =
            (USHORT)CsrAllocateMessagePointer( p,
                                               pTargetPath->MaximumLength,
                                               (PVOID *)&a->TargetPath.Buffer
                                             );
        RtlCopyUnicodeString( &a->TargetPath, pTargetPath );
    }
    else {
        RtlInitUnicodeString( &a->TargetPath, NULL );
    }

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         p,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepDefineDosDevice
                                            ),
                         sizeof( *a )
                       );
    CsrFreeCaptureBuffer( p );

    return m.ReturnValue;

#endif

}


