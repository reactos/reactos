/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    assert.c

Abstract:

    This module implements the RtlAssert function that is referenced by the
    debugging version of the ASSERT macro defined in NTDEF.H

Author:

    Steve Wood (stevewo) 03-Oct-1989

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <zwapi.h>

//
// RtlAssert is not called unless the caller is compiled with DBG non-zero
// therefore it does no harm to always have this routin in the kernel.  
// This allows checked drivers to be thrown on the system and have their
// asserts be meaningful.
//

#define RTL_ASSERT_ALWAYS_ENABLED 1

VOID
RtlAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
#if DBG || RTL_ASSERT_ALWAYS_ENABLED
    char Response[ 2 ];
    CONTEXT Context;

#ifndef BLDR_KERNEL_RUNTIME
    RtlCaptureContext( &Context );
#endif

    while (TRUE) {
        DbgPrint( "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                );

        DbgPrompt( "Break, Ignore, Terminate Process or Terminate Thread (bipt)? ",
                   Response,
                   sizeof( Response )
                 );
        switch (Response[0]) {
            case 'B':
            case 'b':
                DbgPrint( "Execute '!cxr %p' to dump context\n", &Context);
                DbgBreakPoint();
                break;

            case 'I':
            case 'i':
                return;

            case 'P':
            case 'p':
                ZwTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
                break;

            case 'T':
            case 't':
                ZwTerminateThread( NtCurrentThread(), STATUS_UNSUCCESSFUL );
                break;
            }
        }

    DbgBreakPoint();
    ZwTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
#endif
}
