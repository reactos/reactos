/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    psldt.c

Abstract:

    This module contains mips stubs for the Io port handler support

Author:

    Dave Hastings (daveh) 26 Jan 1991

Revision History:

--*/

#include "psp.h"


NTSTATUS
PspSetProcessIoHandlers(
    IN PEPROCESS Process,
    IN PVOID IoHandlerInformation,
    IN ULONG IoHandlerLength
    )
/*++

Routine Description:

    This routine returns STATUS_NOT_IMPLEMENTED

Arguments:

    Process -- Supplies a pointer to the process for which Io port handlers
        are to be installed
    IoHandlerInformation -- Supplies a pointer to the information about the
        io port handlers
    IoHandlerLength -- Supplies the length of the IoHandlerInformation
        structure.

Return Value:

    Returns STATUS_NOT_IMPLEMENTED

--*/
{
    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(IoHandlerInformation);
    UNREFERENCED_PARAMETER(IoHandlerLength);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
PspDeleteIoHandlers(
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This is a stub for the IoHandler delete routine

Arguments:

    Process -- Supplies a pointer to the process

Return Value:

    None
--*/
{
    UNREFERENCED_PARAMETER(Process);
}


VOID
PspDeleteVdmObjects(
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This is a stub for the Vdm Objects delete routine

Arguments:

    Process -- Supplies a pointer to the process

Return Value:

    None
--*/
{
    UNREFERENCED_PARAMETER(Process);
}
