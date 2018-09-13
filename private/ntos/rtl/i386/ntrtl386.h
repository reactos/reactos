/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ntrtl386.h

Abstract:

    i386 specific parts of ntrtlp.h

Author:

    Bryan Willman   10 April 90

Environment:

    These routines are statically linked in the caller's executable and
    are callable in either kernel mode or user mode.

Revision History:

--*/

//
// Exception handling procedure prototypes.
//
VOID
RtlpCaptureContext (
    OUT PCONTEXT ContextRecord
    );

VOID
RtlpUnlinkHandler (
    PEXCEPTION_REGISTRATION_RECORD UnlinkPointer
    );

PEXCEPTION_REGISTRATION_RECORD
RtlpGetRegistrationHead (
    VOID
    );

PVOID
RtlpGetReturnAddress (
    VOID
    );


//
//  Record dump procedures.
//

VOID
RtlpContextDump(
    IN PVOID Object,
    IN ULONG Control OPTIONAL
    );

VOID
RtlpExceptionReportDump(
    IN PVOID Object,
    IN ULONG Control OPTIONAL
    );

VOID
RtlpExceptionRegistrationDump(
    IN PVOID Object,
    IN ULONG Control OPTIONAL
    );
