/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    psldt.c

Abstract:

    This module contains mips stubs for the process and thread ldt support

Author:

    Dave Hastings (daveh) 20 May 1991

Revision History:

--*/

#include "psp.h"


NTSTATUS
PspQueryLdtInformation( 
    IN PEPROCESS Process,
    OUT PVOID LdtInformation,
    IN ULONG LdtInformationLength,
    OUT PULONG ReturnLength
    )
/*++

Routine Description:

    This routine returns STATUS_NOT_IMPLEMENTED

Arguments:

    Process -- Supplies a pointer to the process to return LDT info for
    LdtInformation -- Supplies a pointer to the buffer 
    ReturnLength -- Returns the number of bytes put into the buffer
    
Return Value:

    STATUS_NOT_IMPLEMENTED
--*/
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
PspSetLdtSize(
    IN PEPROCESS Process,
    IN PVOID LdtSize,
    IN ULONG LdtSizeLength
    )

/*++

Routine Description:

    This function returns STATUS_NOT_IMPLEMENTED

Arguments:

    Process -- Supplies a pointer to the process whose Ldt is to be sized
    LdtSize -- Supplies a pointer to the size information

    
Return Value:

    STATUS_NOT_IMPLEMENTED
--*/
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
PspSetLdtInformation(
    IN PEPROCESS Process,
    IN PVOID LdtInformation,
    IN ULONG LdtInformationLength
    )

/*++

Routine Description:

    This function returns STATUS_NOT_IMPLEMENTED

Arguments:

    Process -- Supplies a pointer to the process whose Ldt is to be modified
    LdtInformation -- Supplies a pointer to the information about the Ldt
        modifications
    LdtInformationLength -- Supplies the length of the LdtInformation 
        structure.
Return Value:

    
Return Value:

    STATUS_NOT_IMPLEMENTED
--*/
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
PspQueryDescriptorThread (
    PETHREAD Thread,
    PVOID ThreadInformation,
    ULONG ThreadInformationLength,
    PULONG ReturnLength
    )
/*++

Routine Description:

    This function returns STATUS_NOT_IMPLEMENTED

Arguments:

    Thread -- Supplies a pointer to the thread.
    ThreadInformation -- Supplies information on the descriptor.
    ThreadInformationLength -- Supplies the length of the information.
    ReturnLength -- Returns the number of bytes returned.

Return Value:
    
    STATUS_NOT_IMPLEMENTED
--*/
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
PspDeleteLdt(
    IN PEPROCESS Process
    )
/*++

Routine Description:
    
    This is a stub for the Ldt delete routine

Arguments:

    Process -- Supplies a pointer to the process

Return Value:

    None
--*/
{
}

NTSTATUS
NtSetLdtEntries(
    IN ULONG Selector0,
    IN ULONG Entry0Low,
    IN ULONG Entry0Hi,
    IN ULONG Selector1,
    IN ULONG Entry1Low,
    IN ULONG Entry1High
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
