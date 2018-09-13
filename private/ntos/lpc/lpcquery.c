/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcquery.c

Abstract:

    Local Inter-Process Communication (LPC) query services

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtQueryInformationPort)
#endif


NTSTATUS
NTAPI
NtQueryInformationPort (
    IN HANDLE PortHandle OPTIONAL,
    IN PORT_INFORMATION_CLASS PortInformationClass,
    OUT PVOID PortInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This routine should be used to query an lpc port, but is pretty much a
    noop.  Currently it can only indicate if the input handle is for a port
    object.

Arguments:

    PortHandle - Supplies the handle for the port being queried

    PortInformationClass - Specifies the type information class being asked
        for.  Currently ignored.

    PortInformation - Supplies a pointer to the buffer to receive the
        information.  Currently just probed and then ignored.

    Length - Specifies, in bytes, the size of the port information buffer.

    ReturnLength  - Optionally receives the size, in bytes, of the information
        being returned.  Currently just probed and then ignored.

Return Value:

    NTSTATUS - An appropriate status value.

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PLPCP_PORT_OBJECT PortObject;

    PAGED_CODE();

    //
    //  Get previous processor mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWrite( PortInformation,
                           Length,
                           sizeof( ULONG ));

            if (ARGUMENT_PRESENT( ReturnLength )) {

                ProbeForWriteUlong( ReturnLength );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }
    }

    //
    //  If the user gave us a handle then reference the object.  And return
    //  success if we got a good reference and an error otherwise.
    //

    if (ARGUMENT_PRESENT( PortHandle )) {

        Status = ObReferenceObjectByHandle( PortHandle,
                                            GENERIC_READ,
                                            LpcPortObjectType,
                                            PreviousMode,
                                            &PortObject,
                                            NULL );

        if (!NT_SUCCESS( Status )) {

            return( Status );
        }

        ObDereferenceObject( PortObject );

        return STATUS_SUCCESS;

    } else {

        return STATUS_INVALID_INFO_CLASS;
    }
}
