/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdcpuapi.c

Abstract:

    This module implements CPU specific remote debug APIs.

Author:

    Chuck Bauman 14-Aug-1993

Revision History:

    Based on Mark Lucovsky (markl) MIPS version 04-Sep-1990

--*/

#include "kdp.h"
#define END_OF_CONTROL_SPACE    (sizeof(KPROCESSOR_STATE))


VOID
KdpSetLoadState (
    IN PDBGKD_WAIT_STATE_CHANGE64 WaitStateChange,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    Fill in the Wait_State_Change message record for the load symbol case.

Arguments:

    WaitStateChange - Supplies pointer to record to fill in

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{

    ULONG Count;
    PVOID End;

    //
    // Copy the immediate instruction stream into the control report structure.
    //

    Count =  KdpMoveMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                           (PCHAR)WaitStateChange->ProgramCounter,
                           DBGKD_MAXSTREAM);

    WaitStateChange->ControlReport.InstructionCount = (USHORT)Count;

    //
    // Clear breakpoints in the copied instruction stream. If any breakpoints
    // are cleared, then recopy the instruction stream and restore the break-
    // points afterward.
    //

    End = (PVOID)((PUCHAR)(WaitStateChange->ProgramCounter) + Count - 1);
    if (KdpSuspendBreakpointRange((PVOID)WaitStateChange->ProgramCounter, End) != FALSE) {
        KdpMoveMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                      (PCHAR)WaitStateChange->ProgramCounter,
                      WaitStateChange->ControlReport.InstructionCount);
        KdpRestoreBreakpointRange((PVOID)WaitStateChange->ProgramCounter, End);
    }

    //
    // Copy the context record into the wait state change structure.
    //

    KdpMoveMemory((PCHAR)&WaitStateChange->Context,
                  (PCHAR)ContextRecord,
                  sizeof(*ContextRecord));

    return;
}

VOID
KdpSetStateChange (
    IN PDBGKD_WAIT_STATE_CHANGE64 WaitStateChange,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

    Fill in the Wait_State_Change message record.

Arguments:

    WaitStateChange - Supplies pointer to record to fill in

    ExceptionRecord - Supplies a pointer to an exception record.

    ContextRecord - Supplies a pointer to a context record.

    SecondChance - Supplies a boolean value that determines whether this is
        the first or second chance for the exception.

Return Value:

    None.

--*/

{

    ULONG Count;
    PVOID End;

    //
    // Set up description of event, including exception record
    //

    WaitStateChange->NewState = DbgKdExceptionStateChange;
    WaitStateChange->ProcessorLevel = KeProcessorLevel;
    WaitStateChange->Processor = (USHORT)KeGetCurrentPrcb()->Number;
    WaitStateChange->NumberProcessors = (ULONG)KeNumberProcessors;
    WaitStateChange->Thread = (ULONG64)KeGetCurrentThread();
    WaitStateChange->ProgramCounter = (ULONG64)CONTEXT_TO_PROGRAM_COUNTER(ContextRecord);
    KdpQuickMoveMemory((PCHAR)&WaitStateChange->u.Exception.ExceptionRecord,
                       (PCHAR)ExceptionRecord,
                       sizeof(EXCEPTION_RECORD));

    WaitStateChange->u.Exception.FirstChance = !SecondChance;

    //
    // Copy the immediate instruction stream into the control report structure.
    //

    Count =  KdpMoveMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                           (PCHAR)WaitStateChange->ProgramCounter,
                           DBGKD_MAXSTREAM);

    WaitStateChange->ControlReport.InstructionCount = (USHORT)Count;

    //
    // Clear breakpoints in the copied instruction stream. If any breakpoints
    // are cleared, then recopy the instruction stream and restore the break-
    // points afterward.
    //

    End = (PVOID)((PUCHAR)(WaitStateChange->ProgramCounter) + Count - 1);
    if (KdpSuspendBreakpointRange((PVOID)WaitStateChange->ProgramCounter, End) != FALSE) {
        KdpMoveMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                      (PCHAR)WaitStateChange->ProgramCounter,
                      WaitStateChange->ControlReport.InstructionCount);
        KdpRestoreBreakpointRange((PVOID)WaitStateChange->ProgramCounter, End);
    }

    //
    // Copy the context record into the wait state change structure.
    //

    KdpMoveMemory((PCHAR)&WaitStateChange->Context,
                  (PCHAR)ContextRecord,
                  sizeof(*ContextRecord));

    return;
}

VOID
KdpGetStateChange (
    IN PDBGKD_MANIPULATE_STATE64 ManipulateState,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    Extract continuation control data from Manipulate_State message

    N.B. This is a noop for MIPS.

Arguments:

    ManipulateState - supplies pointer to Manipulate_State packet

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{
}

VOID
KdpReadControlSpace (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a read control space state
    manipulation message.  Its function is to read implementation
    specific system data.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_READ_MEMORY64 a = &m->u.ReadMemory;
    ULONG Length;
    STRING MessageHeader;
    PVOID Buffer = AdditionalData->Buffer;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);

    if (a->TransferCount > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64))) {
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);

    } else {
        Length = a->TransferCount;
    }

    //
    // Case on address to determine what part of Control space is being read.
    //

    switch ( a->TargetBaseAddress ) {

        //
        // Return the pcr address for the current processor.
        //

    case DEBUG_CONTROL_SPACE_PCR:

        *(PKPCR *)Buffer = (PKPCR)(KSEG3_BASE + 
                               (KiProcessorBlock[m->Processor]->PcrPage << PAGE_SHIFT));
        AdditionalData->Length = sizeof( PKPCR );
        a->ActualBytesRead = AdditionalData->Length;
        m->ReturnStatus = STATUS_SUCCESS;
        break;

        //
        // Return the prcb address for the current processor.
        //

    case DEBUG_CONTROL_SPACE_PRCB:

        *(PKPRCB *)Buffer = KiProcessorBlock[m->Processor];
        AdditionalData->Length = sizeof( PKPRCB );
        a->ActualBytesRead = AdditionalData->Length;
        m->ReturnStatus = STATUS_SUCCESS;
        break;

        //
        // Return the pointer to the current thread address for the
        // current processor.
        //

    case DEBUG_CONTROL_SPACE_THREAD:

        *(PKTHREAD *)Buffer = KiProcessorBlock[m->Processor]->CurrentThread;
        AdditionalData->Length = sizeof( PKTHREAD );
        a->ActualBytesRead = AdditionalData->Length;
        m->ReturnStatus = STATUS_SUCCESS;
        break;


    case DEBUG_CONTROL_SPACE_KSPECIAL:

        KdpMoveMemory (Buffer, 
                       (PVOID)&(KiProcessorBlock[m->Processor]->ProcessorState.SpecialRegisters),
                       sizeof( KSPECIAL_REGISTERS )
                      );
        AdditionalData->Length = sizeof( KSPECIAL_REGISTERS );
        a->ActualBytesRead = AdditionalData->Length;
        m->ReturnStatus = STATUS_SUCCESS;
        break;

    default:

        AdditionalData->Length = 0;
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        a->ActualBytesRead = 0;

    }

    KdpSendPacket(
        PACKET_TYPE_KD_STATE_MANIPULATE,
        &MessageHeader,
        AdditionalData
        );
}

VOID
KdpWriteControlSpace (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a write control space state
    manipulation message.  Its function is to write implementation
    specific system data.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_WRITE_MEMORY64 a = &m->u.WriteMemory;
    STRING MessageHeader;
    ULONG  Length;
    PVOID Buffer = AdditionalData->Buffer;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    switch ( (ULONG_PTR)a->TargetBaseAddress ) {

    case DEBUG_CONTROL_SPACE_KSPECIAL:

        KdpMoveMemory ((PVOID)&(KiProcessorBlock[m->Processor]->ProcessorState.SpecialRegisters),
                       Buffer,
                       sizeof( KSPECIAL_REGISTERS )
                      );
        AdditionalData->Length = sizeof( KSPECIAL_REGISTERS );
        a->ActualBytesWritten = AdditionalData->Length;
        m->ReturnStatus = STATUS_SUCCESS;
        break;

    default:

        AdditionalData->Length = 0;
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        a->ActualBytesWritten = 0;

    }

    KdpSendPacket(
        PACKET_TYPE_KD_STATE_MANIPULATE,
        &MessageHeader,
        AdditionalData
        );
}

VOID
KdpReadIoSpace (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a read io space state
    manipulation message.  Its function is to read system io
    locations.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_READ_WRITE_IO64 a = &m->u.ReadWriteIo;
    STRING MessageHeader;
    PUCHAR b;
    PUSHORT s;
    PULONG l;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);

    m->ReturnStatus = STATUS_SUCCESS;

    //
    // Check Size and Alignment
    //

    switch ( a->DataSize ) {
        case 1:
            b = (PUCHAR)MmDbgReadCheck((PVOID)a->IoAddress);
            if ( b ) {
                a->DataValue = (ULONG)*b;
            } else {
                m->ReturnStatus = STATUS_ACCESS_VIOLATION;
            }
            break;
        case 2:
            if ((ULONG_PTR)a->IoAddress & 1 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                s = (PUSHORT)MmDbgReadCheck((PVOID)a->IoAddress);
                if ( s ) {
                    a->DataValue = (ULONG)*s;
                } else {
                    m->ReturnStatus = STATUS_ACCESS_VIOLATION;
                }
            }
            break;
        case 4:
            if ((ULONG_PTR)a->IoAddress & 3 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                l = (PULONG)MmDbgReadCheck((PVOID)a->IoAddress);
                if ( l ) {
                    a->DataValue = (ULONG)*l;
                } else {
                    m->ReturnStatus = STATUS_ACCESS_VIOLATION;
                }
            }
            break;
        default:
            m->ReturnStatus = STATUS_INVALID_PARAMETER;
    }

    KdpSendPacket(
        PACKET_TYPE_KD_STATE_MANIPULATE,
        &MessageHeader,
        NULL
        );
}

VOID
KdpWriteIoSpace (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a write io space state
    manipulation message.  Its function is to write to system io
    locations.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_READ_WRITE_IO64 a = &m->u.ReadWriteIo;
    STRING MessageHeader;
    PUCHAR b;
    PUSHORT s;
    PULONG l;
    HARDWARE_PTE Opaque;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);

    m->ReturnStatus = STATUS_SUCCESS;

    //
    // Check Size and Alignment
    //

    switch ( a->DataSize ) {
        case 1:
            b = (PUCHAR)MmDbgWriteCheck((PVOID)a->IoAddress, &Opaque);
            if ( b ) {
                WRITE_REGISTER_UCHAR(b,(UCHAR)a->DataValue);
                MmDbgReleaseAddress(b, &Opaque);
            } else {
                m->ReturnStatus = STATUS_ACCESS_VIOLATION;
            }
            break;
        case 2:
            if ((ULONG_PTR)a->IoAddress & 1 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                s = (PUSHORT)MmDbgWriteCheck((PVOID)a->IoAddress, &Opaque);
                if ( s ) {
                    WRITE_REGISTER_USHORT(s,(USHORT)a->DataValue);
                    MmDbgReleaseAddress(s, &Opaque);
                } else {
                    m->ReturnStatus = STATUS_ACCESS_VIOLATION;
                }
            }
            break;
        case 4:
            if ((ULONG_PTR)a->IoAddress & 3 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                l = (PULONG)MmDbgWriteCheck((PVOID)a->IoAddress, &Opaque);
                if ( l ) {
                    WRITE_REGISTER_ULONG(l,a->DataValue);
                    MmDbgReleaseAddress(l, &Opaque);
                } else {
                    m->ReturnStatus = STATUS_ACCESS_VIOLATION;
                }
            }
            break;
        default:
            m->ReturnStatus = STATUS_INVALID_PARAMETER;
    }

    KdpSendPacket(
        PACKET_TYPE_KD_STATE_MANIPULATE,
        &MessageHeader,
        NULL
        );
}
