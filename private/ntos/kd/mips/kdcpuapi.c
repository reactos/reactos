/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdcpuapi.c

Abstract:

    This module implements CPU specific remote debug APIs.

Author:

    Mark Lucovsky (markl) 04-Sep-1990

Revision History:

--*/

#include "kdp.h"


#define HEADER_FILE
#include "kxmips.h"

VOID
KdpSetLoadState (
    IN PDBGKD_WAIT_STATE_CHANGE WaitStateChange,
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

    Count = KdpMoveMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                          WaitStateChange->ProgramCounter,
                          DBGKD_MAXSTREAM);

    WaitStateChange->ControlReport.InstructionCount = (USHORT)Count;

    //
    // Clear breakpoints in the copied instruction stream. If any breakpoints
    // are cleared, then recopy the instruction stream.
    //

    End = (PVOID)((PUCHAR)(WaitStateChange->ProgramCounter) + Count - 1);
    if (KdpDeleteBreakpointRange(WaitStateChange->ProgramCounter, End) != FALSE) {
        KdpMoveMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                      WaitStateChange->ProgramCounter,
                      Count);
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
    IN PDBGKD_WAIT_STATE_CHANGE WaitStateChange,
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
    //  Set up description of event, including exception record
    //

    WaitStateChange->NewState = DbgKdExceptionStateChange;
    WaitStateChange->ProcessorLevel = KeProcessorLevel;
    WaitStateChange->Processor = (USHORT)KeGetCurrentPrcb()->Number;
    WaitStateChange->NumberProcessors = (ULONG)KeNumberProcessors;
    WaitStateChange->Thread = (PVOID)KeGetCurrentThread();
    WaitStateChange->ProgramCounter = (PVOID)CONTEXT_TO_PROGRAM_COUNTER(ContextRecord);
    KdpQuickMoveMemory((PCHAR)&WaitStateChange->u.Exception.ExceptionRecord,
                       (PCHAR)ExceptionRecord,
                       sizeof(EXCEPTION_RECORD));

    WaitStateChange->u.Exception.FirstChance = !SecondChance;

    //
    // Copy the immediate instruction stream into the control report structure.
    //

    Count = KdpMoveMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                          WaitStateChange->ProgramCounter,
                          DBGKD_MAXSTREAM);

    WaitStateChange->ControlReport.InstructionCount = (USHORT)Count;

    //
    // Clear breakpoints in the copied instruction stream. If any breakpoints
    // are cleared, then recopy the instruction stream.
    //

    End = (PVOID)((PUCHAR)(WaitStateChange->ProgramCounter) + Count - 1);
    if (KdpDeleteBreakpointRange(WaitStateChange->ProgramCounter, End) != FALSE) {
        KdpMoveMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                      WaitStateChange->ProgramCounter,
                      Count);
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
    IN PDBGKD_MANIPULATE_STATE ManipulateState,
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
    IN PDBGKD_MANIPULATE_STATE m,
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

    PDBGKD_READ_MEMORY a = &m->u.ReadMemory;
    ULONG Length;
    STRING MessageHeader;
    ULONG NumberOfEntries;
    ULONG StartingEntry;
    PULONG EntryBuffer;
    PKPRCB Prcb;
    PTB_ENTRY TbEntry;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);

    if (a->TransferCount > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE))) {
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE);

    } else {
        Length = a->TransferCount;
    }

    //
    // Case on address to determine what part of Control space is being read.
    //

    ASSERT(sizeof(PVOID) == sizeof(ULONG));
    if ( (ULONG)a->TargetBaseAddress >= 0 &&
         (ULONG)a->TargetBaseAddress < KeNumberTbEntries &&
         (m->Processor < (USHORT)KeNumberProcessors )) {

        //
        // Read TB entries.
        //

        NumberOfEntries = Length / sizeof(TB_ENTRY);
        StartingEntry = (ULONG)a->TargetBaseAddress;

        //
        // Trim number of entries to tb range
        //

        if (StartingEntry + NumberOfEntries > KeNumberTbEntries) {
            NumberOfEntries = KeNumberTbEntries - StartingEntry;
        }

        AdditionalData->Length = (USHORT)(NumberOfEntries * sizeof(TB_ENTRY));
        EntryBuffer = (PULONG)AdditionalData->Buffer;
        Prcb = KiProcessorBlock[m->Processor];
        TbEntry = &Prcb->ProcessorState.TbEntry[0];
        while (NumberOfEntries--) {
            *(PENTRYLO)EntryBuffer++ = TbEntry[StartingEntry].Entrylo0;
            *(PENTRYLO)EntryBuffer++ = TbEntry[StartingEntry].Entrylo1;
            *(PENTRYHI)EntryBuffer++ = TbEntry[StartingEntry].Entryhi;
            *(PPAGEMASK)EntryBuffer++ = TbEntry[StartingEntry].Pagemask;
            StartingEntry += 1;
        }

        m->ReturnStatus = STATUS_SUCCESS;
        a->ActualBytesRead = AdditionalData->Length;

    } else {

        //
        // Uninterpreted Special Space
        //

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
    IN PDBGKD_MANIPULATE_STATE m,
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
    PDBGKD_WRITE_MEMORY a = &m->u.WriteMemory;
    STRING MessageHeader;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    AdditionalData->Length = 0;
    m->ReturnStatus = STATUS_UNSUCCESSFUL;
    a->ActualBytesWritten = 0;

    KdpSendPacket(
        PACKET_TYPE_KD_STATE_MANIPULATE,
        &MessageHeader,
        AdditionalData
        );
}

VOID
KdpReadIoSpace (
    IN PDBGKD_MANIPULATE_STATE m,
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
    PDBGKD_READ_WRITE_IO a = &m->u.ReadWriteIo;
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
            b = (PUCHAR)MmDbgReadCheck(a->IoAddress);
            if ( b ) {
                a->DataValue = (ULONG)*b;
            } else {
                m->ReturnStatus = STATUS_ACCESS_VIOLATION;
            }
            break;
        case 2:
            if ((ULONG)a->IoAddress & 1 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                s = (PUSHORT)MmDbgReadCheck(a->IoAddress);
                if ( s ) {
                    a->DataValue = (ULONG)*s;
                } else {
                    m->ReturnStatus = STATUS_ACCESS_VIOLATION;
                }
            }
            break;
        case 4:
            if ((ULONG)a->IoAddress & 3 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                l = (PULONG)MmDbgReadCheck(a->IoAddress);
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
    IN PDBGKD_MANIPULATE_STATE m,
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
    PDBGKD_READ_WRITE_IO a = &m->u.ReadWriteIo;
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
            b = (PUCHAR)MmDbgWriteCheck(a->IoAddress);
            if ( b ) {
                WRITE_REGISTER_UCHAR(b,(UCHAR)a->DataValue);
            } else {
                m->ReturnStatus = STATUS_ACCESS_VIOLATION;
            }
            break;
        case 2:
            if ((ULONG)a->IoAddress & 1 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                s = (PUSHORT)MmDbgWriteCheck(a->IoAddress);
                if ( s ) {
                    WRITE_REGISTER_USHORT(s,(USHORT)a->DataValue);
                } else {
                    m->ReturnStatus = STATUS_ACCESS_VIOLATION;
                }
            }
            break;
        case 4:
            if ((ULONG)a->IoAddress & 3 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                l = (PULONG)MmDbgWriteCheck(a->IoAddress);
                if ( l ) {
                    WRITE_REGISTER_ULONG(l,a->DataValue);
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
