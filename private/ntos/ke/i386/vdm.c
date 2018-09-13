/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    VDM.C

Abstract:

    This module contains support routines for the x86 monitor for
    running Dos applications in V86 mode.

Author:

    Dave Hastings (daveh) 20 Mar 1991

Environment:

    The code in this module is all x86 specific.

Notes:

    In its current implementation, this code is less robust than it needs
    to be.  This will be fixed.  Specifically, parameter verification needs
    to be done. (daveh 7/15/91)

    Support for 32 bit segements (2/2/92)
Revision History:

    20-Mar-1991 daveh
        created
--*/

#include "ki.h"
#pragma hdrstop
#include "vdmntos.h"

#define VDM_IO_TEST 0

#if VDM_IO_TEST
VOID
TestIoHandlerStuff(
    VOID
    );
#endif

BOOLEAN
Ki386GetSelectorParameters(
    IN USHORT Selector,
    OUT PULONG Flags,
    OUT PULONG Base,
    OUT PULONG Limit
    );


BOOLEAN
Ki386VdmDispatchIo(
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Read,
    IN UCHAR InstructionSize,
    IN PKTRAP_FRAME TrapFrame
    );

BOOLEAN
Ki386VdmDispatchStringIo(
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Rep,
    IN BOOLEAN Read,
    IN ULONG Count,
    IN ULONG Address,
    IN UCHAR InstructionSize,
    IN PKTRAP_FRAME TrapFrame
    );


BOOLEAN
VdmDispatchIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG Context,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Read,
    IN OUT PULONG Data
    );

BOOLEAN
VdmDispatchUnalignedIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG Context,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Read,
    IN OUT PULONG Data
    );

BOOLEAN
VdmDispatchStringIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG Context,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN ULONG Count,
    IN BOOLEAN Read,
    IN ULONG Data
    );

BOOLEAN
VdmCallStringIoHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN PVOID StringIoRoutine,
    IN ULONG Context,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN ULONG Count,
    IN BOOLEAN Read,
    IN ULONG Data
    );

BOOLEAN
VdmConvertToLinearAddress(
    IN ULONG SegmentedAddress,
    IN PVOID *LinearAddress
    );

VOID
KeI386VdmInitialize(
    VOID
    );

ULONG
Ki386VdmEnablePentiumExtentions(
    ULONG
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ki386GetSelectorParameters)
#pragma alloc_text(PAGE, Ki386VdmDispatchIo)
#pragma alloc_text(PAGE, Ki386VdmDispatchStringIo)
#pragma alloc_text(PAGE, VdmDispatchIoToHandler)
#pragma alloc_text(PAGE, VdmDispatchUnalignedIoToHandler)
#pragma alloc_text(PAGE, VdmDispatchStringIoToHandler)
#pragma alloc_text(PAGE, VdmCallStringIoHandler)
#pragma alloc_text(PAGE, VdmConvertToLinearAddress)
#pragma alloc_text(INIT, KeI386VdmInitialize)
#endif

KMUTEX VdmStringIoMutex;
ULONG VdmFixedStateLinear;

ULONG KeI386EFlagsAndMaskV86 = EFLAGS_USER_SANITIZE;
ULONG KeI386EFlagsOrMaskV86 = EFLAGS_INTERRUPT_MASK;
BOOLEAN KeI386VdmIoplAllowed = FALSE;
ULONG KeI386VirtualIntExtensions = 0;


BOOLEAN
Ki386GetSelectorParameters(
    IN USHORT Selector,
    OUT PULONG Flags,
    OUT PULONG Base,
    OUT PULONG Limit
    )

/*++

Routine Description:

    This routine gets information about a selector in the ldt, and
    returns it to the caller.

Arguments:

    IN USHORT Selector -- selector number for selector to return info for
    OUT PULONG Flags -- flags indicating the type of the selector.
    OUT PULONG Base -- base linear address of the selector
    OUT PULONG Limit -- limit of the selector.

Return Value:

    return-value - True if the selector is in the LDT, and present.
                    False otherwise.
Note:

    This routine should probably be somewhere else.  There are a number
    of issues to clear up with respect to selectors and the kernel, and
    after they have been cleared up, this code will be moved to its
    correct place

--*/

{

    PLDT_ENTRY Ldt,OldLdt;
    ULONG LdtLimit,OldLdtLimit,RetryCount = 0;
    PKPROCESS Process;
    BOOLEAN ReturnValue;

    *Flags = 0;

    if ((Selector & (SELECTOR_TABLE_INDEX | DPL_USER))
        != (SELECTOR_TABLE_INDEX | DPL_USER)) {
        return FALSE;
    }


    Process = KeGetCurrentThread()->ApcState.Process;
    Ldt = (PLDT_ENTRY)((Process->LdtDescriptor.BaseLow) |
        (Process->LdtDescriptor.HighWord.Bytes.BaseMid << 16) |
        (Process->LdtDescriptor.HighWord.Bytes.BaseHi << 24));

    LdtLimit = ((Process->LdtDescriptor.LimitLow) |
        (Process->LdtDescriptor.HighWord.Bits.LimitHi << 16));

    Selector &= ~(SELECTOR_TABLE_INDEX | DPL_USER);

    //
    // Under normal circumstances, we will only execute the following loop
    // once.  If there is a bug in the user mode wow code however, the LDT
    // may change while we execute the following code.  We don't want to take
    // the Ldt mutex, because that is expensive.
    //

    do {

        RetryCount++;

        if (((ULONG)Selector >= LdtLimit) || (!Ldt)) {
            return FALSE;
        }

        try {

            if (!Ldt[Selector/sizeof(LDT_ENTRY)].HighWord.Bits.Pres) {
                *Flags = SEL_TYPE_NP;
                ReturnValue = FALSE;
            } else {

                *Base = (Ldt[Selector/sizeof(LDT_ENTRY)].BaseLow |
                    (Ldt[Selector/sizeof(LDT_ENTRY)].HighWord.Bytes.BaseMid << 16) |
                    (Ldt[Selector/sizeof(LDT_ENTRY)].HighWord.Bytes.BaseHi << 24));

                *Limit = (Ldt[Selector/sizeof(LDT_ENTRY)].LimitLow |
                    (Ldt[Selector/sizeof(LDT_ENTRY)].HighWord.Bits.LimitHi << 16));

                *Flags = 0;

                if ((Ldt[Selector/sizeof(LDT_ENTRY)].HighWord.Bits.Type & 0x18) == 0x18) {
                    *Flags |= SEL_TYPE_EXECUTE;

                    if (Ldt[Selector/sizeof(LDT_ENTRY)].HighWord.Bits.Type & 0x02) {
                        *Flags |= SEL_TYPE_READ;
                    }
                } else {
                    *Flags |= SEL_TYPE_READ;
                    if (Ldt[Selector/sizeof(LDT_ENTRY)].HighWord.Bits.Type & 0x02) {
                        *Flags |= SEL_TYPE_WRITE;
                    }
                    if (Ldt[Selector/sizeof(LDT_ENTRY)].HighWord.Bits.Type & 0x04) {
                        *Flags |= SEL_TYPE_ED;
                    }
                }

                if (Ldt[Selector/sizeof(LDT_ENTRY)].HighWord.Bits.Default_Big) {
                    *Flags |= SEL_TYPE_BIG;
                }

                if (Ldt[Selector/sizeof(LDT_ENTRY)].HighWord.Bits.Granularity) {
                    *Flags |= SEL_TYPE_2GIG;
                }
            }
            ReturnValue = TRUE;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            // Don't do anything here.  We took the fault because the
            // Ldt moved.  We will get an answer the next time around
        }

        //
        // If we can't get an answer in 10 tries, we never will
        //
        if ((RetryCount > 10)) {
            ReturnValue = FALSE;
        }

        if (ReturnValue == FALSE) {
            break;
        }

        OldLdt = Ldt;
        OldLdtLimit = LdtLimit;

        Ldt = (PLDT_ENTRY)((Process->LdtDescriptor.BaseLow) |
            (Process->LdtDescriptor.HighWord.Bytes.BaseMid << 16) |
            (Process->LdtDescriptor.HighWord.Bytes.BaseHi << 24));

        LdtLimit = ((Process->LdtDescriptor.LimitLow) |
            (Process->LdtDescriptor.HighWord.Bits.LimitHi << 16));

    } while ((Ldt != OldLdt) || (LdtLimit != OldLdtLimit));

    return ReturnValue;
}

BOOLEAN
Ki386VdmDispatchIo(
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Read,
    IN UCHAR InstructionSize,
    IN PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    This routine sets up the Event info for an IO event, and causes the
    event to be reflected to the Monitor.

    It is assumed that interrupts are enabled upon entry, and Irql is
    at APC level.

Arguments:

    PortNumber -- Supplies the port number the IO was done to
    Size -- Supplies the size of the IO operation.
    Read -- Indicates whether the IO operation was a read or a write.
    InstructionSize -- Supplies the size of the IO instruction in bytes.

Return Value:

    True if the io instruction will be reflected to User mode.

--*/
{
    PVDM_TIB VdmTib;
    EXCEPTION_RECORD ExceptionRecord;
    VDM_IO_HANDLER VdmIoHandler;
    ULONG Result;
    BOOLEAN Success = FALSE;
    ULONG Context;

    Success = Ps386GetVdmIoHandler(
        PsGetCurrentProcess(),
        PortNumber & ~0x3,
        &VdmIoHandler,
        &Context
        );

    if (Success) {
        Result = TrapFrame->Eax;
        // if port is not aligned, perform unaligned IO
        // else do the io the easy way
        if (PortNumber % Size) {
            Success = VdmDispatchUnalignedIoToHandler(
                &VdmIoHandler,
                Context,
                PortNumber,
                Size,
                Read,
                &Result
                );
        } else {
            Success = VdmDispatchIoToHandler(
                &VdmIoHandler,
                Context,
                PortNumber,
                Size,
                Read,
                &Result
                );
        }
    }

    if (Success) {
        if (Read) {
            switch (Size) {
            case 4:
                TrapFrame->Eax = Result;
                break;
            case 2:
                *(PUSHORT)(&TrapFrame->Eax) = (USHORT)Result;
                break;
            case 1:
                *(PUCHAR)(&TrapFrame->Eax) = (UCHAR)Result;
                break;
            }
        }
        TrapFrame->Eip += (ULONG) InstructionSize;
        return TRUE;
    } else {
        try {
            VdmTib = 
                ((PVDM_PROCESS_OBJECTS)PsGetCurrentProcess()->VdmObjects)->VdmTib;
            VdmTib->EventInfo.InstructionSize = (ULONG) InstructionSize;
            VdmTib->EventInfo.Event = VdmIO;
            VdmTib->EventInfo.IoInfo.PortNumber = (USHORT)PortNumber;
            VdmTib->EventInfo.IoInfo.Size = (USHORT)Size;
            VdmTib->EventInfo.IoInfo.Read = Read;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ExceptionRecord.ExceptionCode = STATUS_ACCESS_VIOLATION;
            ExceptionRecord.ExceptionFlags = 0;
            ExceptionRecord.NumberParameters = 0;
            ExRaiseException(&ExceptionRecord);
            return FALSE;
        }
    }

    VdmEndExecution(TrapFrame, VdmTib);

    return TRUE;

}


BOOLEAN
Ki386VdmDispatchStringIo(
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Rep,
    IN BOOLEAN Read,
    IN ULONG Count,
    IN ULONG Address,
    IN UCHAR InstructionSize,
    IN PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    This routine sets up the Event info for a string IO event, and causes the
    event to be reflected to the Monitor.

    It is assumed that interrupts are enabled upon entry, and Irql is
    at APC level.

Arguments:

    PortNumber -- Supplies the port number the IO was done to
    Size -- Supplies the size of the IO operation.
    Read -- Indicates whether the IO operation was a read or a write.
    Count -- indicates the number of IO operations of Size size
    Address -- Indicates address for string io
    InstructionSize -- Supplies the size of the IO instruction in bytes.


Return Value:

    True if the io instruction will be reflected to User mode.



--*/
{
    PVDM_TIB VdmTib;
    EXCEPTION_RECORD ExceptionRecord;
    BOOLEAN Success = FALSE;
    VDM_IO_HANDLER VdmIoHandler;
    ULONG Context;

    Success = Ps386GetVdmIoHandler(
        PsGetCurrentProcess(),
        PortNumber & ~0x3,
        &VdmIoHandler,
        &Context
        );


    if (Success) {
        Success = VdmDispatchStringIoToHandler(
            &VdmIoHandler,
            Context,
            PortNumber,
            Size,
            Count,
            Read,
            Address
            );
    }

    if (Success) {
        PUSHORT pIndexRegister;
        USHORT Index;

        // WARNING no 32 bit address support

        pIndexRegister = Read ? (PUSHORT)&TrapFrame->Edi
                              : (PUSHORT)&TrapFrame->Esi;

        if (TrapFrame->EFlags & EFLAGS_DF_MASK) {
            Index = *pIndexRegister - (USHORT)(Count * Size);
            }
        else {
            Index = *pIndexRegister + (USHORT)(Count * Size);
            }

        *pIndexRegister = Index;

        if (Rep) {
            (USHORT)TrapFrame->Ecx = 0;
            }

        TrapFrame->Eip += (ULONG) InstructionSize;
        return TRUE;
    }

    try {
        VdmTib = 
            ((PVDM_PROCESS_OBJECTS)PsGetCurrentProcess()->VdmObjects)->VdmTib;
        VdmTib->EventInfo.InstructionSize = (ULONG) InstructionSize;
        VdmTib->EventInfo.Event = VdmStringIO;
        VdmTib->EventInfo.StringIoInfo.PortNumber = (USHORT)PortNumber;
        VdmTib->EventInfo.StringIoInfo.Size = (USHORT)Size;
        VdmTib->EventInfo.StringIoInfo.Rep = Rep;
        VdmTib->EventInfo.StringIoInfo.Read = Read;
        VdmTib->EventInfo.StringIoInfo.Count = Count;
        VdmTib->EventInfo.StringIoInfo.Address = Address;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ExceptionRecord.ExceptionCode = STATUS_ACCESS_VIOLATION;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.NumberParameters = 0;
        ExRaiseException(&ExceptionRecord);
        return FALSE;
    }


    VdmEndExecution(TrapFrame, VdmTib);

    return TRUE;
}


BOOLEAN
VdmDispatchIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG Context,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Read,
    IN OUT PULONG Data
    )
/*++

Routine Description:

     This routine calls the handler for the IO.  If there is not a handler
     of the proper size, it will call this function for 2 io's to the next
     smaller size.  If the size was a byte, and there was no handler, FALSE
     is returned.

Arguments:

    VdmIoHandler -- Supplies a pointer to the handler table
    Context -- Supplies 32 bits of data set when the port was trapped
    PortNumber -- Supplies the port number the IO was done to
    Size -- Supplies the size of the IO operation.
    Read -- Indicates whether the IO operation was a read or a write.
    Result -- Supplies a pointer to the location to put the result

Return Value:

    True if one or more handlers were called to take care of the IO.
    False if no handler was called to take care of the IO.

--*/
{
    NTSTATUS Status;
    BOOLEAN Success1, Success2;
    USHORT FnIndex;
    UCHAR AccessType;

    // Insure that Io is aligned
    ASSERT((!(PortNumber % Size)));

    if (Read) {
        FnIndex = 0;
        AccessType = EMULATOR_READ_ACCESS;
    } else {
        FnIndex = 1;
        AccessType = EMULATOR_WRITE_ACCESS;
    }

    switch (Size) {
    case 1:
        if (VdmIoHandler->IoFunctions[FnIndex].UcharIo[PortNumber % 4]) {
            Status = (*(VdmIoHandler->IoFunctions[FnIndex].UcharIo[PortNumber % 4]))(
                Context,
                PortNumber,
                AccessType,
                (PUCHAR)Data
                );
            if (NT_SUCCESS(Status)) {
                return TRUE;
            }
        }
        // No handler for this port
        return FALSE;

    case 2:
        if (VdmIoHandler->IoFunctions[FnIndex].UshortIo[PortNumber % 2]) {
            Status = (*(VdmIoHandler->IoFunctions[FnIndex].UshortIo[PortNumber % 2]))(
                Context,
                PortNumber,
                AccessType,
                (PUSHORT)Data
                );
            if (NT_SUCCESS(Status)) {
                return TRUE;
            }
        } else {
            // Dispatch to the two uchar handlers for this ushort port
            Success1 = VdmDispatchIoToHandler(
                VdmIoHandler,
                Context,
                PortNumber,
                Size /2,
                Read,
                Data
                );

            Success2 = VdmDispatchIoToHandler(
                VdmIoHandler,
                Context,
                PortNumber + 1,
                Size / 2,
                Read,
                (PULONG)((PUCHAR)Data + 1)
                );

            return (Success1 || Success2);

        }
        return FALSE;

    case 4:
        if (VdmIoHandler->IoFunctions[FnIndex].UlongIo) {
            Status = (*(VdmIoHandler->IoFunctions[FnIndex].UlongIo))(
                Context,
                PortNumber,
                AccessType,
                Data
                );
            if (NT_SUCCESS(Status)) {
                return TRUE;
            }
        } else {
            // Dispatch to the two ushort handlers for this port
            Success1 = VdmDispatchIoToHandler(
                VdmIoHandler,
                Context,
                PortNumber,
                Size /2,
                Read,
                Data);
            Success2 = VdmDispatchIoToHandler(
                VdmIoHandler,
                Context,
                PortNumber + 2,
                Size / 2,
                Read,
                (PULONG)((PUSHORT)Data + 1)
                );

            return (Success1 || Success2);
        }
        return FALSE;
    }
}

BOOLEAN
VdmDispatchUnalignedIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG Context,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Read,
    IN OUT PULONG Data
    )
/*++

Routine Description:

     This routine converts the unaligned IO to the necessary number of aligned
     IOs to smaller ports.

Arguments:

    VdmIoHandler -- Supplies a pointer to the handler table
    Context -- Supplies 32 bits of data set when the port was trapped
    PortNumber -- Supplies the port number the IO was done to
    Size -- Supplies the size of the IO operation.
    Read -- Indicates whether the IO operation was a read or a write.
    Result -- Supplies a pointer to the location to put the result

Return Value:

    True if one or more handlers were called to take care of the IO.
    False if no handler was called to take care of the IO.

--*/
{
    ULONG Offset;
    BOOLEAN Success;

    ASSERT((Size > 1));
    ASSERT((PortNumber % Size));

    Offset = 0;

    //
    //  The possible unaligned io situations are as follows.
    //
    //  1.  Uchar aligned Ulong io
    //          We have to dispatch a uchar io, a ushort io, and a uchar io
    //
    //  2.  Ushort aligned Ulong Io
    //          We have to dispatch a ushort io, and a ushort io
    //
    //  3.  Uchar aligned Ushort Io
    //          We have to dispatch a uchar io and a uchar io
    //

    // if the port is uchar aligned
    if ((PortNumber % Size) & 1) {
        Success = VdmDispatchIoToHandler(
            VdmIoHandler,
            Context,
            PortNumber,
            1,
            Read,
            Data
            );
        Offset += 1;
    // else it is ushort aligned (and therefore must be a ulong port)
    } else {
        Success = VdmDispatchIoToHandler(
            VdmIoHandler,
            Context,
            PortNumber,
            2,
            Read,
            Data
            );
        Offset += 2;
    }

    // if it is a ulong port, we know we have a ushort IO to dispatch
    if (Size == 4) {
        Success |= VdmDispatchIoToHandler(
            VdmIoHandler,
            Context,
            PortNumber + Offset,
            2,
            Read,
            (PULONG)((PUCHAR)Data + Offset)
            );
        Offset += 2;
    }

    // If we haven't dispatched the entire port, dispatch the final uchar
    if (Offset != 4) {
        Success |= VdmDispatchIoToHandler(
            VdmIoHandler,
            Context,
            PortNumber + Offset,
            1,
            Read,
            (PULONG)((PUCHAR)Data + Offset)
            );
    }

    return Success;
}

BOOLEAN
VdmDispatchStringIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG Context,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN ULONG Count,
    IN BOOLEAN Read,
    IN ULONG Data
    )
/*++

Routine Description:

     This routine calls the handler for the IO.  If there is not a handler
     of the proper size, or the io is not aligned, it will simulate the io
     to the normal io handlers.

Arguments:

    VdmIoHandler -- Supplies a pointer to the handler table
    Context -- Supplies 32 bits of data set when the port was trapped
    PortNumber -- Supplies the port number the IO was done to
    Size -- Supplies the size of the IO operation.
    Count -- Supplies the number of IO operations.
    Read -- Indicates whether the IO operation was a read or a write.
    Data -- Supplies a segmented address at which to put the result.

Return Value:

    True if one or more handlers were called to take care of the IO.
    False if no handler was called to take care of the IO.

--*/
{
    BOOLEAN Success = FALSE;
    USHORT FnIndex;
    NTSTATUS Status;

    if (Read) {
        FnIndex = 0;
    } else {
        FnIndex = 1;
    }

    Status = KeWaitForSingleObject(
        &VdmStringIoMutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    switch (Size) {
    case 1:
        Success = VdmCallStringIoHandler(
            VdmIoHandler,
            (PVOID)VdmIoHandler->IoFunctions[FnIndex].UcharStringIo[PortNumber % 4],
            Context,
            PortNumber,
            Size,
            Count,
            Read,
            Data
            );
        break;

    case 2:
        Success = VdmCallStringIoHandler(
            VdmIoHandler,
            (PVOID)VdmIoHandler->IoFunctions[FnIndex].UshortStringIo[PortNumber % 2],
            Context,
            PortNumber,
            Size,
            Count,
            Read,
            Data
            );
        break;

    case 4:
        Success = VdmCallStringIoHandler(
            VdmIoHandler,
            (PVOID)VdmIoHandler->IoFunctions[FnIndex].UlongStringIo,
            Context,
            PortNumber,
            Size,
            Count,
            Read,
            Data
            );
        break;

    }
    KeReleaseMutex(&VdmStringIoMutex, FALSE);
    return Success;
}

#define STRINGIO_BUFFER_SIZE 1024
UCHAR VdmStringIoBuffer[STRINGIO_BUFFER_SIZE];

BOOLEAN
VdmCallStringIoHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN PVOID StringIoRoutine,
    IN ULONG Context,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN ULONG Count,
    IN BOOLEAN Read,
    IN ULONG Data
    )
/*++

Routine Description:

    This routine actually performs the call to string io routine.  It takes
    care of buffering the user data in kernel space so that the device driver
    does not have to.  If there is not a string io function, or the io is
    misaligned, it will be simulated as a series of normal io operations

Arguments:

    StringIoRoutine -- Supplies a pointer to the string Io routine
    Context -- Supplies 32 bits of data set when the port was trapped
    PortNumber -- Supplies the number of the port to perform Io to
    Size -- Supplies the size of the io operations
    Count -- Supplies the number of Io operations in the string.
    Read -- Indicates a read operation
    Data -- Supplies a pointer to the user buffer to perform the io on.

Returns

    TRUE if a handler was called
    FALSE if not.

--*/
{
    ULONG TotalBytes,BytesDone,BytesToDo,LoopCount,NumberIo;
    PUCHAR CurrentDataPtr;
    UCHAR AccessType;
    EXCEPTION_RECORD ExceptionRecord;
    NTSTATUS Status;
    BOOLEAN Success;

    Success = VdmConvertToLinearAddress(
        Data,
        &CurrentDataPtr
        );

    if (!Success) {
        ExceptionRecord.ExceptionCode = STATUS_ACCESS_VIOLATION;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.NumberParameters = 0;
        ExRaiseException(&ExceptionRecord);
        // Cause kernel exit, rather than Io reflection
        return TRUE;
    }


    TotalBytes = Count * Size;
    BytesDone = 0;

    if (PortNumber % Size) {
        StringIoRoutine = NULL;
    }

    if (Read) {
        AccessType = EMULATOR_READ_ACCESS;
    } else {
        AccessType = EMULATOR_WRITE_ACCESS;
    }


    // Set up try out here to avoid overhead in loop
    try {
        while (BytesDone < TotalBytes) {
            if ((BytesDone + STRINGIO_BUFFER_SIZE) > TotalBytes) {
                BytesToDo = TotalBytes - BytesDone;
            } else {
                BytesToDo = STRINGIO_BUFFER_SIZE;
            }

            ASSERT((!(BytesToDo % Size)));

            if (!Read) {
                RtlMoveMemory(VdmStringIoBuffer, CurrentDataPtr, BytesToDo);
            }

            NumberIo = BytesToDo / Size;

            if (StringIoRoutine) {
                // in order to avoid having 3 separate calls, one for each size
                // we simply cast the parameters appropriately for the
                // byte routine.

                Status = (*((PDRIVER_IO_PORT_UCHAR_STRING)StringIoRoutine))(
                    Context,
                    PortNumber,
                    AccessType,
                    VdmStringIoBuffer,
                    NumberIo
                    );

                if (NT_SUCCESS(Status)) {
                    Success |= TRUE;
                }
            } else {
                if (PortNumber % Size) {
                    for (LoopCount = 0; LoopCount < NumberIo; LoopCount++ ) {
                        Success |= VdmDispatchUnalignedIoToHandler(
                            VdmIoHandler,
                            Context,
                            PortNumber,
                            Size,
                            Read,
                            (PULONG)(VdmStringIoBuffer + LoopCount * Size)
                            );
                    }
                } else {
                    for (LoopCount = 0; LoopCount < NumberIo; LoopCount++ ) {
                        Success |= VdmDispatchIoToHandler(
                            VdmIoHandler,
                            Context,
                            PortNumber,
                            Size,
                            Read,
                            (PULONG)(VdmStringIoBuffer + LoopCount * Size)
                            );
                    }

                }
            }

            if (Read) {
                RtlMoveMemory(CurrentDataPtr, VdmStringIoBuffer, BytesToDo);
            }

            BytesDone += BytesToDo;
            CurrentDataPtr += BytesToDo;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ExceptionRecord.ExceptionCode = GetExceptionCode();
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.NumberParameters = 0;
        ExRaiseException(&ExceptionRecord);
        // Cause kernel exit, rather than Io reflection
        Success = TRUE;
    }
    return Success;

}

BOOLEAN
VdmConvertToLinearAddress(
    IN ULONG SegmentedAddress,
    OUT PVOID *LinearAddress
    )
/*++

Routine Description:

    This routine converts the specified segmented address into a linear
    address, based on processor mode in user mode.

Arguments:

    SegmentedAddress -- Supplies the segmented address to convert.
    LinearAddress -- Supplies a pointer to the destination for the
        coresponding linear address

Return Value:

    True if the address was converted.
    False otherwise

Note:

    A linear address of 0 is a valid return
--*/
{
    PKTHREAD Thread;
    PKTRAP_FRAME TrapFrame;
    BOOLEAN Success;
    ULONG Base, Limit, Flags;

    Thread = KeGetCurrentThread();
    TrapFrame = VdmGetTrapFrame(Thread);

    if (TrapFrame->EFlags & EFLAGS_V86_MASK) {
        *LinearAddress = (PVOID)(((SegmentedAddress & 0xFFFF0000) >> 12) +
            (SegmentedAddress & 0xFFFF));
        Success = TRUE;
    } else {
        Success = Ki386GetSelectorParameters(
            (USHORT)((SegmentedAddress & 0xFFFF0000) >> 16),
            &Flags,
            &Base,
            &Limit
            );
        if (Success) {
            *LinearAddress = (PVOID)(Base + (SegmentedAddress & 0xFFFF));
        }
    }
    return Success;
}

VOID
KeI386VdmInitialize(
    VOID
    )
/*++

Routine Description:

    This routine initializes the vdm stuff

Arguments:

    None

Return Value:

    None
--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE RegistryHandle = NULL;
    UNICODE_STRING WorkString;
    UCHAR KeyInformation[sizeof(KEY_VALUE_BASIC_INFORMATION) + 30];
    ULONG ResultLength;

    KeInitializeMutex( &VdmStringIoMutex, MUTEX_LEVEL_VDM_IO );

    //
    // Set up and open KeyPath to wow key
    //

    RtlInitUnicodeString(
        &WorkString,
        L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Wow"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        NULL
        );

    Status = ZwOpenKey(
        &RegistryHandle,
        KEY_READ,
        &ObjectAttributes
        );

    //
    // If there is no Wow key, don't allow Vdms to run
    //
    if (!NT_SUCCESS(Status)) {
        return;
    }

    //
    // Set up for using virtual interrupt extensions if they are available
    //

    //
    // Get the Pentium Feature disable value.
    // If this value is present, don't enable vme stuff.
    //
    RtlInitUnicodeString(
        &WorkString,
        L"DisableVme"
        );

    Status = ZwQueryValueKey(
        RegistryHandle,
        &WorkString,
        KeyValueBasicInformation,
        &KeyInformation,
        sizeof(KEY_VALUE_BASIC_INFORMATION) + 30,
        &ResultLength
        );

    if (!NT_SUCCESS(Status)) {

        //
        // If we have the extensions, set the appropriate bits
        // in cr4
        //
        if (KeFeatureBits & KF_V86_VIS) {
            KiIpiGenericCall(
                Ki386VdmEnablePentiumExtentions,
                TRUE
                );
            KeI386VirtualIntExtensions = V86_VIRTUAL_INT_EXTENSIONS;
        }
    }

    //
    // If we have V86 mode int extensions, we don't want to run with
    // IOPL in v86 mode
    //
    if (!KeI386VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) {
        //
        // Read registry to determine if Vdms will run with IOPL in v86 mode
        //

        //
        // Get the VdmIOPL value.
        //
        RtlInitUnicodeString(
            &WorkString,
            L"VdmIOPL"
            );

        Status = ZwQueryValueKey(
            RegistryHandle,
            &WorkString,
            KeyValueBasicInformation,
            &KeyInformation,
            sizeof(KEY_VALUE_BASIC_INFORMATION) + 30,
            &ResultLength
            );

        //
        // If the value exists, let Vdms run with IOPL in V86 mode
        //
        if (NT_SUCCESS(Status)) {
            //
            // KeEflagsAndMaskV86 and KeEflagsOrMaskV86 are used
            // in SANITIZE_FLAGS, and the Vdm code to make sure the
            // values in EFlags for v86 mode trap frames are acceptable
            //
            KeI386EFlagsAndMaskV86 = EFLAGS_USER_SANITIZE | EFLAGS_INTERRUPT_MASK;
            KeI386EFlagsOrMaskV86 = EFLAGS_IOPL_MASK;

            //
            // KeVdmIoplAllowed is used by the Vdm code to determine if
            // the virtual interrupt flag is in EFlags, or 40:xx
            //
            KeI386VdmIoplAllowed = TRUE;

        }
    }

    ZwClose(RegistryHandle);

    //
    //  Initialize the address of the Vdm communications area based on
    //  machine type because of non-AT Japanese PCs.  Note that we only
    //  have to change the op-code for PC-98 machines as the default is
    //  the PC/AT value.
    //

    if (KeI386MachineType & MACHINE_TYPE_PC_9800_COMPATIBLE) {

        //
        //  Set NTVDM state liner for PC-9800 Series
        //

        VdmFixedStateLinear = FIXED_NTVDMSTATE_LINEAR_PC_98;
    } else {

        //
        //  We are running on an normal PC/AT or a Fujitsu FMR comaptible.
        //

        VdmFixedStateLinear = FIXED_NTVDMSTATE_LINEAR_PC_AT;
    }
}


BOOLEAN
Ke386VdmInsertQueueApc (
    IN PKAPC             Apc,
    IN PKTHREAD          Thread,
    IN KPROCESSOR_MODE   ApcMode,
    IN PKKERNEL_ROUTINE  KernelRoutine,
    IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
    IN PKNORMAL_ROUTINE  NormalRoutine  OPTIONAL,
    IN PVOID             NormalContext   OPTIONAL,
    IN KPRIORITY         Increment
    )

/*++

Routine Description:

    This function initializes, and queues a vdm type of APC to the
    specified thread.


    A Vdm type of APC:
       - OriginalApcEnvironment
       - will only be queued to one thread at a time
       - if UserMode Fires on the next system exit. A UserMode apc should
         not be queued if the current vdm context is not application mode.

Arguments:

    Apc - Supplies a pointer to a control object of type APC.

    Thread - Supplies a pointer to a dispatcher object of type thread.

    ApcMode - Supplies the processor mode user\kernel of the Apc

    KernelRoutine - Supplies a pointer to a function that is to be
        executed at IRQL APC_LEVEL in kernel mode.

    RundownRoutine - Supplies an optional pointer to a function that is to be
        called if the APC is in a thread's APC queue when the thread terminates.

    NormalRoutine - Supplies an optional pointer to a function that is
        to be executed at IRQL 0 in the specified processor mode. If this
        parameter is not specified, then the ProcessorMode and NormalContext
        parameters are ignored.

    NormalContext - Supplies a pointer to an arbitrary data structure which is
        to be passed to the function specified by the NormalRoutine parameter.

    Increment - Supplies the priority increment that is to be applied if
        queuing the APC causes a thread wait to be satisfied.


Return Value:

    If APC queuing is disabled, then a value of FALSE is returned.
    Otherwise a value of TRUE is returned.


--*/

{

    PKAPC_STATE ApcState;
    PKTHREAD ApcThread;
    KIRQL   OldIrql;
    BOOLEAN Inserted;

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the apc object not initialized, then initialize it and acquire
    // the target thread APC queue lock.
    //

    if (Apc->Type != ApcObject) {
        Apc->Type = ApcObject;
        Apc->Size = sizeof(KAPC);
        Apc->ApcStateIndex  = OriginalApcEnvironment;
    } else {

        //
        // Acquire the APC thread APC queue lock.
        //
        // If the APC is inserted in the corresponding APC queue, and the
        // APC thread is not the same thread as the target thread, then
        // the APC is removed from its current queue, the APC pending state
        // is updated, the APC thread APC queue lock is released, and the
        // target thread APC queue lock is acquired. Otherwise, the APC
        // thread and the target thread are same thread and the APC is already
        // queued to the correct thread.
        //
        // If the APC is not inserted in an APC queue, then release the
        // APC thread APC queue lock and acquire the target thread APC queue
        // lock.
        //

        ApcThread = Apc->Thread;
        if (ApcThread) {
            KiAcquireSpinLock(&ApcThread->ApcQueueLock);
            if (Apc->Inserted) {
                if (ApcThread == Apc->Thread && Apc->Thread != Thread) {
                    Apc->Inserted = FALSE;
                    RemoveEntryList(&Apc->ApcListEntry);
                    ApcState = Apc->Thread->ApcStatePointer[Apc->ApcStateIndex];
                    if (IsListEmpty(&ApcState->ApcListHead[Apc->ApcMode]) != FALSE) {
                        if (Apc->ApcMode == KernelMode) {
                            ApcState->KernelApcPending = FALSE;

                        } else {
                            ApcState->UserApcPending = FALSE;
                        }
                    }

                } else {
                    KiReleaseSpinLock(&ApcThread->ApcQueueLock);
                    KiUnlockDispatcherDatabase(OldIrql);
                    return TRUE;
                }
            }

            KiReleaseSpinLock(&ApcThread->ApcQueueLock);
        }
    }


    KiAcquireSpinLock(&Thread->ApcQueueLock);

    Apc->ApcMode = ApcMode;
    Apc->Thread  = Thread;
    Apc->KernelRoutine   = KernelRoutine;
    Apc->RundownRoutine  = RundownRoutine;
    Apc->NormalRoutine   = NormalRoutine;
    Apc->SystemArgument1 = NULL;
    Apc->SystemArgument2 = NULL;
    Apc->NormalContext   = NormalContext;

    //
    // Unlock the target thread APC queue.
    //

    KiReleaseSpinLock(&Thread->ApcQueueLock);

    //
    // If APC queuing is enable, then attempt to queue the APC object.
    //

    if (Thread->ApcQueueable && KiInsertQueueApc(Apc, Increment)) {
        Inserted = TRUE;

        //
        // If UserMode:
        //    For vdm a UserMode Apc is only queued by a kernel mode
        //    apc which is on the current thread for the target thread.
        //    Force UserApcPending for User mode apcstate, so that
        //    the apc will fire when this thread exits the kernel.
        //

        if (ApcMode == UserMode) {
            KiBoostPriorityThread(Thread, Increment);
            Thread->ApcState.UserApcPending = TRUE;
        }

    } else {
        Inserted = FALSE;
    }

    //
    // Unlock the dispatcher database, lower IRQL to its previous value, and
    // return whether the APC object was inserted.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return Inserted;
}


VOID
Ke386VdmClearApcObject(
    IN PKAPC Apc
    )
/*++

Routine Description:

    Clears a VDM APC object, synchronously with Ke386VdmInsertQueueApc, and
    is expected to be called by one of the vdm kernel apc routine or the
    rundown routine.


Arguments:

    Apc - Supplies a pointer to a control object of type APC.


Return Value:

    void

--*/
{

    KIRQL   OldIrql;

    //
    // Take Dispatcher database lock, to sync with Ke386VDMInsertQueueApc
    //

    KiLockDispatcherDatabase(&OldIrql);
    Apc->Thread  = NULL;
    KiUnlockDispatcherDatabase(OldIrql);

}







//
//  END of ACTIVE CODE
//







#if VDM_IO_TEST
NTSTATUS
TestIoByteRoutine(
    IN ULONG Port,
    IN UCHAR AccessMode,
    IN OUT PUCHAR Data
    )
{
    if (AccessMode & EMULATOR_READ_ACCESS) {
        *Data = Port - 400;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
TestIoWordReadRoutine(
    IN ULONG Port,
    IN UCHAR AccessMode,
    IN OUT PUSHORT Data
    )
{
    if (AccessMode & EMULATOR_READ_ACCESS) {
        *Data = Port - 200;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
TestIoWordWriteRoutine(
    IN ULONG Port,
    IN UCHAR AccessMode,
    IN OUT PUSHORT Data
    )
{
    DbgPrint("Word Write routine port # %lx, %x\n",Port,*Data);

    return STATUS_SUCCESS;
}

NTSTATUS
TestIoDwordRoutine(
    IN ULONG Port,
    IN USHORT AccessMode,
    IN OUT PULONG Data
    )
{
    if (AccessMode & EMULATOR_READ_ACCESS) {
        *Data = Port;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
TestIoStringRoutine(
    IN ULONG Port,
    IN USHORT AccessMode,
    IN OUT PSHORT Data,
    IN ULONG Count
    )
{
    ULONG i;

    if (AccessMode & EMULATOR_READ_ACCESS) {
        for (i = 0;i < Count ;i++ ) {
            Data[i] = i;
        }
    } else {
        DbgPrint("String Port Called for write port #%lx,",Port);
        for (i = 0;i < Count ;i++ ) {
            DbgPrint("%x\n",Data[i]);
        }
    }

    return STATUS_SUCCESS;
}

PROCESS_IO_PORT_HANDLER_INFORMATION IoPortHandler;
EMULATOR_ACCESS_ENTRY Entry[4];
BOOLEAN Connect = TRUE, Disconnect = FALSE;

VOID
TestIoHandlerStuff(
    VOID
    )
{
    NTSTATUS Status;

    IoPortHandler.Install = TRUE;
    IoPortHandler.NumEntries = 5L;
    IoPortHandler.EmulatorAccessEntries = Entry;

    Entry[0].BasePort = 0x400;
    Entry[0].NumConsecutivePorts = 0x30;
    Entry[0].AccessType = Uchar;
    Entry[0].AccessMode = EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS;
    Entry[0].StringSupport = FALSE;
    Entry[0].Routine = TestIoByteRoutine;

    Entry[1].BasePort = 0x400;
    Entry[1].NumConsecutivePorts = 0x18;
    Entry[1].AccessType = Ushort;
    Entry[1].AccessMode = EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS;
    Entry[1].StringSupport = FALSE;
    Entry[1].Routine = TestIoWordReadRoutine;

    Entry[2].BasePort = 0x400;
    Entry[2].NumConsecutivePorts = 0xc;
    Entry[2].AccessType = Ulong;
    Entry[2].AccessMode = EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS;
    Entry[2].StringSupport = FALSE;
    Entry[2].Routine = TestIoDwordRoutine;

    Entry[3].BasePort = 0x400;
    Entry[3].NumConsecutivePorts = 0x18;
    Entry[3].AccessType = Ushort;
    Entry[3].AccessMode = EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS;
    Entry[3].StringSupport = TRUE;
    Entry[3].Routine = TestIoStringRoutine;

     if (Connect) {
        Status = ZwSetInformationProcess(
            NtCurrentProcess(),
            ProcessIoPortHandlers,
            &IoPortHandler,
            sizeof(PROCESS_IO_PORT_HANDLER_INFORMATION)
            ) ;
        if (!NT_SUCCESS(Status)) {
            DbgBreakPoint();
        }
        Connect = FALSE;
    }

    IoPortHandler.Install = FALSE;
    if (Disconnect) {
        Status = ZwSetInformationProcess(
            NtCurrentProcess(),
            ProcessIoPortHandlers,
            &IoPortHandler,
            sizeof(PROCESS_IO_PORT_HANDLER_INFORMATION)
            );
        if (!NT_SUCCESS(Status)) {
            DbgBreakPoint();
        }
        Disconnect = FALSE;
    }
}
#endif
