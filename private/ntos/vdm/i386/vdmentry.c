/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmentry.c

Abstract:

    This function dispatches to the vdm services

Author:

    Dave Hastings (daveh) 6-Apr-1992

Notes:

    This module will be fleshed out when the great vdm code consolidation
    occurs, sometime soon after the functionality is done.

Revision History:

     24-Sep-1993 Jonle: reoptimize dispatcher to suit the number of services
                        add QueueInterrupt service

--*/

#include "vdmp.h"
#include <ntvdmp.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtVdmControl)
#endif

#if DBG
void AssertIrqlPassive(void)
{
if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
    DbgPrint("NtVdmControl:returning at raised irql!\n");
    DbgBreakPoint();
    }
}
#else
#define AssertIrqlPassive()
#endif



NTSTATUS
NtVdmControl(
    IN VDMSERVICECLASS Service,
    IN OUT PVOID ServiceData
    )
/*++

Routine Description:

    386 specific routine which dispatches to the appropriate function
    based on service number.

Arguments:

    Service -- Specifies what service is to be performed
    ServiceData -- Supplies a pointer to service specific data

Return Value:

    if invalid service number: STATUS_INVALID_PARAMETER_1
    else see individual services.


--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Dispatch in descending order of frequency
    //
    if (Service == VdmStartExecution) {
        Status = VdmpStartExecution();
    } else if (Service == VdmQueueInterrupt) {
        Status = VdmpQueueInterrupt(ServiceData);
    } else if (Service == VdmDelayInterrupt) {
        Status = VdmpDelayInterrupt(ServiceData);
    } else if (Service == VdmQueryDir) {
        Status = VdmQueryDirectoryFile(ServiceData);
    } else if (Service == VdmInitialize) {
        Status = VdmpInitialize(ServiceData);
    } else if (Service == VdmFeatures) {
        try {
            //
            // Verify that we were passed a valid user address
            //
            ProbeForWriteBoolean((PBOOLEAN)ServiceData);

            //
            // Return the appropriate feature bits to notify
            // ntvdm which modes (if any) fast IF emulation is
            // available for
            //
            if (KeI386VdmIoplAllowed) {
                *((PULONG)ServiceData) = V86_VIRTUAL_INT_EXTENSIONS;
            } else {
                // remove this if pm extensions to be used
                *((PULONG)ServiceData) = KeI386VirtualIntExtensions &
                    ~PM_VIRTUAL_INT_EXTENSIONS;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
        }
        Status = STATUS_SUCCESS;

    } else if (Service == VdmSetInt21Handler) {
        try {
            ProbeForRead(ServiceData, sizeof(VDMSET_INT21_HANDLER_DATA), 1);

            Status = Ke386SetVdmInterruptHandler(
                KeGetCurrentThread()->ApcState.Process,
                0x21L,
                (USHORT)(((PVDMSET_INT21_HANDLER_DATA)ServiceData)->Selector),
                ((PVDMSET_INT21_HANDLER_DATA)ServiceData)->Offset,
                ((PVDMSET_INT21_HANDLER_DATA)ServiceData)->Gate32
                );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
        }

    } else if (Service == VdmPrinterDirectIoOpen) {
        Status = VdmpPrinterDirectIoOpen(ServiceData);
    } else if (Service == VdmPrinterDirectIoClose) {
        Status = VdmpPrinterDirectIoClose(ServiceData);
    } else if (Service == VdmPrinterInitialize) {
        Status = VdmpPrinterInitialize(ServiceData);
    } else {
        Status = STATUS_INVALID_PARAMETER_1;
    }


    AssertIrqlPassive();
    return Status;

}
