/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    buserror.c

Abstract:

    This module implements the code necessary to process data and instruction
    bus errors and to set the address of the cache error routine.

Author:

    David N. Cutler (davec) 31-Oct-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

BOOLEAN
KeBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN PVOID VirtualAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress
    )

/*++

Routine Description:

    This function provides the default bus error handling routine for NT.

    N.B. There is no return from this routine.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

    VirtualAddress - Supplies the virtual address of the bus error.

    PhysicalAddress - Supplies the physical address of the bus error.

Return Value:

    None.

--*/

{

    //
    // Bug check specifying the exception code, the virtual address, the
    // low part of the physical address, the current processor state, and
    // the exception PC.
    //

    KeBugCheckEx(ExceptionRecord->ExceptionCode & 0xffff,
                 (ULONG)VirtualAddress,
                 PhysicalAddress.LowPart,
                 TrapFrame->Psr,
                 TrapFrame->Fir);

    return FALSE;
}

PHYSICAL_ADDRESS
KiGetPhysicalAddress (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function computes the physical address for a given virtual address.

Arguments:

    VirtualAddress - Supplies the virtual address whose physical address is
        to be computed.

Return Value:

    The physical address that correcponds to the specified virtual address.

--*/

{
    PHYSICAL_ADDRESS PhysicalAddress;

    //
    // If the address is a KSEG0 or KSEG1 address, then mask off the high
    // three address bits and return the result as the physical address.
    // Otherwise, call memory management to convert the virtual address to
    // a physical address.
    //

    if (((ULONG)VirtualAddress >= KSEG0_BASE) &&
        ((ULONG)VirtualAddress < (KSEG1_BASE + 0x20000000))) {
        PhysicalAddress.LowPart = (ULONG)VirtualAddress & 0x1fffffff;
        PhysicalAddress.HighPart = 0;
        return PhysicalAddress;

    } else {
        return MmGetPhysicalAddress(VirtualAddress);
    }
}

VOID
KiDataBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to process a data bus error. The virtual and
    physical address of the error are computed and the data bus error
    processing routine is called indirectly through the PCR. NT provides
    a standard routine to process the error and shutdown the system. A
    vendor, however, can replace the standard NT routine and do additional
    processing if necessary via the HAL.

    N.B. There is no return from this routine.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{

    PVOID VirtualAddress;
    PHYSICAL_ADDRESS PhysicalAddress;
    MIPS_INSTRUCTION FaultInstruction;

    //
    // Any exception that occurs during the attempted calculation of the
    // virtual address causes the virtual address calculation to be
    // aborted and the virtual address of the instruction itself is used
    // instead.
    //

    try {

        //
        // Compute the effective address of the reference.
        //

        FaultInstruction.Long = *((PULONG)ExceptionRecord->ExceptionAddress);
        VirtualAddress = (PVOID)(KiGetRegisterValue(FaultInstruction.i_format.Rs,
                                                    ExceptionFrame,
                                                    TrapFrame) +
                                               FaultInstruction.i_format.Simmediate);

    //
    // If an exception occurs, then abort the calculation of the virtual
    // address and set the virtual address equal to the instruction address.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        VirtualAddress = ExceptionRecord->ExceptionAddress;
    }

    //
    // Compute the physical address that corresponds to the data address.
    //

    PhysicalAddress = KiGetPhysicalAddress(VirtualAddress);

    //
    // If a value of FALSE is returned by the data bus error handling routine,
    // then bug check. Otherwise, assume that the error has been handled and
    // return.
    //

    if ((PCR->DataBusError)(ExceptionRecord,
                        ExceptionFrame,
                        TrapFrame,
                        VirtualAddress,
                        PhysicalAddress) == FALSE) {

        KeBugCheck(DATA_BUS_ERROR);
    }

    return;
}

VOID
KiInstructionBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to process an instruction bus error. The virtual
    and physical address of the error are computed and the instruction bus
    error processing routine is called indirectly through the PCR. NT provides
    a standard routine to process the error and shutdown the system. A vendor,
    however, can replace the standard NT routine and do additional processing
    if necessary via the HAL.

    N.B. There is no return from this routine.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{

    PVOID VirtualAddress;
    PHYSICAL_ADDRESS PhysicalAddress;

    //
    // Compute the physical address that corresponds to the data address.
    //

    VirtualAddress = ExceptionRecord->ExceptionAddress;
    PhysicalAddress = KiGetPhysicalAddress(VirtualAddress);

    //
    // If a value of FALSE is returned by the instructiona bus error handling
    // routine, then bug check. Otherwise, assume that the error has been
    // handled and return.
    //

    if ((PCR->InstructionBusError)(ExceptionRecord,
                               ExceptionFrame,
                               TrapFrame,
                               VirtualAddress,
                               PhysicalAddress) == FALSE) {

        KeBugCheck(INSTRUCTION_BUS_ERROR);
    }

    return;
}

VOID
KeSetCacheErrorRoutine (
    IN PKCACHE_ERROR_ROUTINE Routine
    )

/*++

Routine Description:

    This function is called to set the address of the cache error routine.
    The cache error routine is called whenever a cache error occurs.

Arguments:

    Routine - Supplies a pointer to the cache error routine.

Return Value:

    None.

--*/

{

    //
    // Set the address of the cache error routine.
    //

    *((PULONG)CACHE_ERROR_VECTOR) = (ULONG)Routine | KSEG1_BASE;
    return;
}
