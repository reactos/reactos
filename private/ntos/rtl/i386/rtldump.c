/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rtldump.c

Abstract:

    This module implements dump procedures for:

	ContextRecords,
	ExceptionReportRecords,
	ExceptionRegistrationRecords

Author:

    Bryan Willman (bryanwi)  12 April 90

Environment:

    Callable in any mode in which DbgPrint works.

Revision History:

--*/

#include    "ntrtlp.h"

VOID
RtlpContextDump(
    IN PVOID Object,
    IN ULONG Control OPTIONAL
    )
/*++

Routine Description:

    This function dumps the contents of a context record.

    Currently, it does not dump floating point context.

Arguments:

    Object - Address of the record to dump.

    Control - Ignored, here so we look like a standard dump procedure.

Return Value:

    none

--*/

{
    PCONTEXT	Context;

    Context = (PCONTEXT)Object;

    DbgPrint("      Record @ %lx\n", (ULONG)Context);
    DbgPrint(" ContextFlags: %lx\n", Context->ContextFlags);
    DbgPrint("\n");

    DbgPrint("        SegGs: %lx\n", Context->SegGs);
    DbgPrint("        SegFs: %lx\n", Context->SegFs);
    DbgPrint("        SegEs: %lx\n", Context->SegEs);
    DbgPrint("        SegDs: %lx\n", Context->SegDs);
    DbgPrint("\n");

    DbgPrint("          Edi: %lx\n", Context->Edi);
    DbgPrint("          Esi: %lx\n", Context->Esi);
    DbgPrint("          Ebx: %lx\n", Context->Ebx);
    DbgPrint("          Edx: %lx\n", Context->Edx);
    DbgPrint("          Ecx: %lx\n", Context->Ecx);
    DbgPrint("          Eax: %lx\n", Context->Eax);
    DbgPrint("\n");

    DbgPrint("          Ebp: %lx\n", Context->Ebp);
    DbgPrint("          Eip: %lx\n", Context->Eip);
    DbgPrint("        SegCs: %lx\n", Context->SegCs);
    DbgPrint("       EFlags: %lx\n", Context->EFlags);
    DbgPrint("          Esp: %lx\n", Context->Esp);
    DbgPrint("        SegSs: %lx\n", Context->SegSs);
    DbgPrint("\n");

    return;
}



VOID
RtlpExceptionReportDump(
    IN PVOID Object,
    IN ULONG Control OPTIONAL
    )
/*++

Routine Description:

    This function dumps the contents of an Exception report record.

Arguments:

    Object - Address of the record to dump.

    Control - Ignored, here so we look like a standard dump procedure.

Return Value:

    none

--*/

{
    ULONG i;

    PEXCEPTION_RECORD	Exception;

    Exception = (PEXCEPTION_RECORD)Object;

    DbgPrint("                Record @ %lx\n", (ULONG)Exception);
    DbgPrint("          ExceptionCode: %lx\n", Exception->ExceptionCode);
    DbgPrint("         ExceptionFlags: %lx\n", Exception->ExceptionFlags);
    DbgPrint("        ExceptionRecord: %lx\n", Exception->ExceptionRecord);
    DbgPrint("       ExceptionAddress: %lx\n", Exception->ExceptionAddress);
    DbgPrint("       NumberParameters: %lx\n", Exception->NumberParameters);
    for (i = 0; i < Exception->NumberParameters; i++)
	DbgPrint("ExceptionInformation[%d]: %lx\n",
		 i, Exception->ExceptionInformation[i]);
    DbgPrint("\n");
    return;
}



VOID
RtlpExceptionRegistrationDump(
    IN PVOID Object,
    IN ULONG Control OPTIONAL
    )
/*++

Routine Description:

    This function dumps the contents of an exception registration record,
    unless Object == NULL, in which case it dumps the entire registration
    chain.

    Currently, it does not dump floating point context.

Arguments:

    Object - Address of the record to dump.

    Control - Ignored, here so we look like a standard dump procedure.

Return Value:

    none

--*/

{
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;

    RegistrationPointer = (PEXCEPTION_REGISTRATION_RECORD)Object;

    if (RegistrationPointer != EXCEPTION_CHAIN_END) {
	DbgPrint("Record @ %lx\n", (ULONG)RegistrationPointer);
	DbgPrint("   Next: %lx\n", RegistrationPointer->Next);
	DbgPrint("Handler: %lx\n", RegistrationPointer->Handler);
	DbgPrint("\n");
    } else {
	RegistrationPointer = RtlpGetRegistrationHead();

	while (RegistrationPointer != EXCEPTION_CHAIN_END) {
	    DbgPrint("Record @ %lx\n", (ULONG)RegistrationPointer);
	    DbgPrint("   Next: %lx\n", RegistrationPointer->Next);
	    DbgPrint("Handler: %lx\n", RegistrationPointer->Handler);
	    DbgPrint("\n");
	}
    }
    return;
}
