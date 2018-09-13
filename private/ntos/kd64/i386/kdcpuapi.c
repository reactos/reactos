/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdcpuapi.c

Abstract:

    This module implements CPU specific remote debug APIs.

Author:

    Mark Lucovsky (markl) 04-Sep-1990

Revision History:

    24-sep-90   bryanwi

        Port to the x86.

--*/

#include <stdio.h>

#include "kdp.h"
#define END_OF_CONTROL_SPACE

extern ULONG KdpCurrentSymbolStart, KdpCurrentSymbolEnd;
extern ULONG KdSpecialCalls[];
extern ULONG KdNumberOfSpecialCalls;

LONG
KdpLevelChange (
    ULONG Pc,
    PCONTEXT ContextRecord,
    PBOOLEAN SpecialCall
    );

LONG
regValue(
    UCHAR reg,
    PCONTEXT ContextRecord
    );

BOOLEAN
KdpIsSpecialCall (
    ULONG Pc,
    PCONTEXT ContextRecord,
    UCHAR opcode,
    UCHAR ModRM
    );

ULONG
KdpGetReturnAddress (
    PCONTEXT ContextRecord
    );

ULONG
KdpGetCallNextOffset (
    ULONG Pc,
    PCONTEXT ContextRecord
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpLevelChange)
#pragma alloc_text(PAGEKD, regValue)
#pragma alloc_text(PAGEKD, KdpIsSpecialCall)
#pragma alloc_text(PAGEKD, KdpGetReturnAddress)
#pragma alloc_text(PAGEKD, KdpSetLoadState)
#pragma alloc_text(PAGEKD, KdpSetStateChange)
#pragma alloc_text(PAGEKD, KdpGetStateChange)
#pragma alloc_text(PAGEKD, KdpReadControlSpace)
#pragma alloc_text(PAGEKD, KdpWriteControlSpace)
#pragma alloc_text(PAGEKD, KdpReadIoSpace)
#pragma alloc_text(PAGEKD, KdpWriteIoSpace)
#pragma alloc_text(PAGEKD, KdpReadMachineSpecificRegister)
#pragma alloc_text(PAGEKD, KdpWriteMachineSpecificRegister)
#pragma alloc_text(PAGEKD, KdpGetCallNextOffset)
#endif

/**** KdpIsTryFinallyReturn - detect finally optimization
*
*  Input:
*       pc - program counter of instruction to check
*       ContextRecord - machine specific context
*
*  Output:
*       returns TRUE if this is a try-finally returning to the same
*       scope
***************************************************************************/


BOOLEAN
KdpIsTryFinallyReturn (
    ULONG Pc,
    PCONTEXT ContextRecord
    )
{
    ULONG retaddr;
    ULONG calldisp;
    UCHAR inst;

    //
    //  The complier generates code for a try-finally that involves having
    //  a ret instruction that does not match with a call instruction.
    //  This ret never returns a value (ie, it's a c3 return and not a
    //  c2).  It always returns into the current symbol scope.  It is never
    //  preceeded by a leave, which (hopefully) should differentiate it
    //  from recursive returns.  Check for this, and if we find it count
    //  it as *0* level change.
    //
    //  As an optimization, the compiler will often change:
    //      CALL
    //      RET
    //  into:
    //      JMP
    //  In either case, we figure out the return address.  It's the first 4 bytes
    //  on the stack.
    //

    KdpMoveMemory( (PCHAR)&retaddr, (PCHAR)ContextRecord->Esp, 4 );

//  DPRINT(( "Start %x return %x end %x\n", KdpCurrentSymbolStart, retaddr, KdpCurrentSymbolEnd ));

    if ( (KdpCurrentSymbolStart < retaddr) && (retaddr < KdpCurrentSymbolEnd) ) {

        //
        //  Well, things aren't this nice.  We may have transferred but not yet
        //  updated the start/end.  This case occurs in a call to a thunk.  We
        //  look to see if the instruction before the return address is a call.
        //  Gross and not 100% reliable.
        //

        KdpMoveMemory( (PCHAR)&inst, (PCHAR)retaddr - 5, 1 );
        KdpMoveMemory( (PCHAR)&calldisp, (PCHAR)retaddr - 4, 4 );

        if (inst == 0xe8 && calldisp + retaddr == Pc) {
//  DPRINT(( "call to thunk @ %x\n", Pc ));
            return FALSE;
        }

        //
        //  returning to the current function.  Either a finally
        //  or a recursive return.  Check for a leave.  This is not 100%
        //  reliable since we are betting on an instruction longer than a byte
        //  and not ending with 0xc9.
        //

        KdpMoveMemory( (PCHAR)&inst, (PCHAR)Pc-1, 1 );

        if ( inst != 0xc9 ) {
            // not a leave.  Assume a try-finally.
//  DPRINT(( "transfer at %x is try-finally\n", Pc ));
            return TRUE;
        }
    }

    //
    //  This appears to be a true RET instruction
    //

    return FALSE;
}

/**** KdpLevelChange - say how the instruction affects the call level
*
*  Input:
*       pc - program counter of instruction to check
*       ContextRecord - machine specific context
*       SpecialCall - pointer to returned boolean indicating if the
*           instruction is a transfer to a special routine
*
*  Output:
*       returns -1 for a level pop, 1 for a push and 0 if it is
*       unchanged.
*  NOTE: This function belongs in some other file.  I should move it.
***************************************************************************/


LONG
KdpLevelChange (
    ULONG Pc,
    PCONTEXT ContextRecord,
    PBOOLEAN SpecialCall
    )
{
    UCHAR membuf[2];
    ULONG Addr;

    KdpMoveMemory( (PCHAR)membuf, (PCHAR)Pc, 2 );

    switch (membuf[0]) {
    case 0xe8:  //  CALL direct w/32 bit displacement
        //
        //  For try/finally, the compiler may, in addition to the push/ret trick
        //  below, use a call to the finally thunk.  Since we treat a RET to
        //  within the same symbol scope as not changing levels, we will also
        //  treat such a call as not changing levels either
        //

        KdpMoveMemory( (PCHAR)&Addr, (PCHAR)Pc+1, 4 );
        Addr += Pc + 5;

        if ((KdpCurrentSymbolStart <= Addr) && (Addr < KdpCurrentSymbolEnd)) {
            *SpecialCall = FALSE;
            return 0;
        }


    case 0x9a:  //  CALL segmented 16:32

        *SpecialCall = KdpIsSpecialCall( Pc, ContextRecord, membuf[0], membuf[1] );
        return 1;

    case 0xff:
        //
        //  This is a compound instruction.  Dispatch on operation
        //
        switch (membuf[1] & 0x38) {
        case 0x10:  //  CALL with mod r/m
            *SpecialCall = KdpIsSpecialCall( Pc, ContextRecord, membuf[0], membuf[1] );
            return 1;
        case 0x20:  //  JMP with mod r/m
            *SpecialCall = KdpIsSpecialCall( Pc, ContextRecord, membuf[0], membuf[1] );

            //
            //  If this is a try/finally, we'd like to treat it as call since the
            //  return inside the destination will bring us back to this context.
            //  However, if it is a jmp to a special routine, we must treat it
            //  as a no-level change operation since we won't see the special
            //  routines's return.
            //
            //  If it is not a try/finally, we'd like to treat it as a no-level
            //  change, unless again, it is a transfer to a special call which
            //  views this as a level up.
            //

            if (KdpIsTryFinallyReturn( Pc, ContextRecord )) {
                if (*SpecialCall) {
                    //
                    //  We won't see the return, so pretend it is just
                    //  inline code
                    //

                    return 0;

                } else {
                    //
                    //  The destinations return will bring us back to this
                    //  context
                    //

                    return 1;
                }
            } else if (*SpecialCall) {
                //
                //  We won't see the return but we are, indeed, doing one.
                //
                return -1;
            } else {
                return 0;
            }

        default:
            *SpecialCall = FALSE;
            return 0;
        }

    case 0xc3:  //  RET

        //
        //  If we are a try/finally ret, then we indicate that it is NOT a level
        //  change
        //

        if (KdpIsTryFinallyReturn( Pc, ContextRecord )) {
            *SpecialCall = FALSE;
            return 0;
        }

    case 0xc2:  //  RET  w/16 bit esp change
    case 0xca:  //  RETF w/16 bit esp change
    case 0xcb:  //  RETF
        *SpecialCall = FALSE;
        return -1;

    default:
        *SpecialCall = FALSE;
        return 0;
    }

} // KdpLevelChange

LONG
regValue(
    UCHAR reg,
    PCONTEXT ContextRecord
    )
{
    switch (reg) {
    case 0x0:
        return(ContextRecord->Eax);
        break;
    case 0x1:
        return(ContextRecord->Ecx);
        break;
    case 0x2:
        return(ContextRecord->Edx);
        break;
    case 0x3:
        return(ContextRecord->Ebx);
        break;
    case 0x4:
        return(ContextRecord->Esp);
        break;
    case 0x5:
        return(ContextRecord->Ebp);
        break;
    case 0x6:
        return(ContextRecord->Esi);
        break;
    case 0x7:
        return(ContextRecord->Edi);
        break;
    }
}

BOOLEAN
KdpIsSpecialCall (
    ULONG Pc,
    PCONTEXT ContextRecord,
    UCHAR opcode,
    UCHAR modRM
    )

/*++

Routine Description:

    Check to see if the instruction at pc is a call to one of the
    SpecialCall routines.

Argument:

    Pc - program counter of instruction in question.

--*/
{
    UCHAR sib;
    USHORT twoBytes;
    ULONG callAddr;
    ULONG addrAddr;
    LONG offset;
    ULONG i;
    char d8;

    if ( opcode == 0xe8 ) {

        //
        // Signed offset from pc
        //

        KdpMoveMemory( (PCHAR)&offset, (PCHAR)Pc+1, 4 );

        callAddr = Pc + offset + 5; // +5 for instr len.

    } else if ( opcode == 0xff ) {

        if ( ((modRM & 0x38) != 0x10) && ((modRM & 0x38) != 0x20) ) {
            // not call or jump
            return FALSE;
        }
        if ( (modRM & 0x08) == 0x08 ) {
            // m16:16 or m16:32 -- we don't handle this
            return FALSE;
        }

        if ( (modRM & 0xc0) == 0xc0 ) {

            /* Direct register addressing */
            callAddr = regValue( (UCHAR)(modRM&0x7), ContextRecord );

        } else if ( (modRM & 0xc7) == 0x05 ) {
            //
            // Calls across dll boundaries involve a call into a jump table,
            // wherein the jump address is set to the real called routine at DLL
            // load time.  Check to see if we're calling such an instruction,
            // and if so, compute its target address and set callAddr there.
            //
            //  ff15 or ff25 -- call or jump indirect with disp32.  Get
            //  address of address
            //
            KdpMoveMemory( (PCHAR)&addrAddr, (PCHAR)Pc+2, 4 );

            //
            //  Get real destination address
            //
            KdpMoveMemory( (PCHAR)&callAddr, (PCHAR)addrAddr, 4 );
//  DPRINT(( "Indirect call/jmp @ %x\n", Pc ));
        } else if ( (modRM & 0x7) == 0x4 ) {

            LONG indexValue;

            /* sib byte present */
            KdpMoveMemory( (PCHAR)&sib, (PCHAR)Pc+2, 1 );
            indexValue = regValue( (UCHAR)((sib & 0x31) >> 3), ContextRecord );
            switch ( sib&0xc0 ) {
            case 0x0:  /* x1 */
                break;
            case 0x40:
                indexValue *= 2;
                break;
            case 0x80:
                indexValue *= 4;
                break;
            case 0xc0:
                indexValue *= 8;
                break;
            } /* switch */

            switch ( modRM & 0xc0 ) {

            case 0x0: /* no displacement */
                if ( (sib & 0x7) == 0x5 ) {
//                  DPRINT(("funny call #1 at %x\n", Pc));
                    return FALSE;
                }
                callAddr = indexValue + regValue((UCHAR)(sib&0x7), ContextRecord );
                break;

            case 0x40:
                if ( (sib & 0x6) == 0x4 ) {
//                  DPRINT(("Funny call #2\n")); /* calling into the stack */
                    return FALSE;
                }
                KdpMoveMemory( &d8, (PCHAR)Pc+3,1 );
                callAddr = indexValue + d8 +
                                    regValue((UCHAR)(sib&0x7), ContextRecord );
                break;

            case 0x80:
                if ( (sib & 0x6) == 0x4 ) {
//                  DPRINT(("Funny call #3\n")); /* calling into the stack */
                    return FALSE;
                }
                KdpMoveMemory( (PCHAR)&offset, (PCHAR)Pc+3, 4 );
                callAddr = indexValue + offset +
                                    regValue((UCHAR)(sib&0x7), ContextRecord );
                break;

            case 0xc0:
                ASSERT( FALSE );
                break;

            }

        } else {
            //KdPrint(( "undecoded call at %x\n",
            //            CONTEXT_TO_PROGRAM_COUNTER(ContextRecord) ));
            return FALSE;
        }

    } else if ( opcode == 0x9a ) {

        /* Absolute address call (best I can tell, cc doesn't generate this) */
        KdpMoveMemory( (PCHAR)&callAddr, (PCHAR)Pc+1, 4 );

    } else {
        return FALSE;
    }

    //
    // Calls across dll boundaries involve a call into a jump table,
    // wherein the jump address is set to the real called routine at DLL
    // load time.  Check to see if we're calling such an instruction,
    // and if so, compute its target address and set callAddr there.
    //

#if 0
    KdpMoveMemory( (PCHAR)&twoBytes, (PCHAR)callAddr, 2 );
    if ( twoBytes == 0x25ff ) { /* i386 is little-Endian; really 0xff25 */

        //
        // This is a 'jmp dword ptr [mem]' instruction, which is the sort of
        // jump used for a dll-boundary crossing call.  Fixup callAddr.
        //

        KdpMoveMemory( (PCHAR)&addrAddr, (PCHAR)callAddr+2, 4 );
        KdpMoveMemory( (PCHAR)&callAddr, (PCHAR)addrAddr, 4 );
    }
#endif

    for ( i = 0; i < KdNumberOfSpecialCalls; i++ ) {
        if ( KdSpecialCalls[i] == callAddr ) {
            return TRUE;
        }
    }
    return FALSE;

}

/*
 * Find the return address of the current function.  Only works when
 * locals haven't yet been pushed (ie, on the first instruction of the
 * function).
 */

ULONG
KdpGetReturnAddress (
    PCONTEXT ContextRecord
    )
{
    ULONG retaddr;

    KdpMoveMemory((PCHAR)(&retaddr), (PCHAR)(ContextRecord->Esp), 4 );
    return retaddr;

} // KdpGetReturnAddress

VOID
KdpSetLoadState(
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
    PKPRCB Prcb;

    //
    // Store the special x86 register into the control report structure.
    //

    Prcb = KeGetCurrentPrcb();
    WaitStateChange->ControlReport.Dr6 = Prcb->ProcessorState.SpecialRegisters.KernelDr6;
    WaitStateChange->ControlReport.Dr7 = Prcb->ProcessorState.SpecialRegisters.KernelDr7;

    //
    // Copy the immediate instruction stream into the control report structure.
    //

    Count = KdpMoveMemory((PCHAR)(&(WaitStateChange->ControlReport.InstructionStream[0])),
                          (PCHAR)(WaitStateChange->ProgramCounter),
                          DBGKD_MAXSTREAM);

    WaitStateChange->ControlReport.InstructionCount = (USHORT)Count;

    //
    // Clear breakpoints in the copied instruction stream. If any breakpoints
    // are cleared, then recopy the instruction stream.
    //

    End = (PVOID)((PUCHAR)(WaitStateChange->ProgramCounter) + Count - 1);
    if (KdpDeleteBreakpointRange((PVOID)WaitStateChange->ProgramCounter, End) != FALSE) {
        KdpMoveMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                      (PVOID)WaitStateChange->ProgramCounter,
                      Count);
    }

    //
    // Store the segment registers into the control report structure and set the
    // control flags.
    //

    WaitStateChange->ControlReport.SegCs = (USHORT)(ContextRecord->SegCs);
    WaitStateChange->ControlReport.SegDs = (USHORT)(ContextRecord->SegDs);
    WaitStateChange->ControlReport.SegEs = (USHORT)(ContextRecord->SegEs);
    WaitStateChange->ControlReport.SegFs = (USHORT)(ContextRecord->SegFs);
    WaitStateChange->ControlReport.EFlags = ContextRecord->EFlags;
    WaitStateChange->ControlReport.ReportFlags = REPORT_INCLUDES_SEGS;

    //
    //  Copy context record into wait state change structure.
    //

    KdpMoveMemory((PCHAR)(&WaitStateChange->Context),
                  (PCHAR)ContextRecord,
                  sizeof(CONTEXT));

    return;
}


VOID
KdpSetStateChange(
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
    PKPRCB Prcb;
    BOOLEAN status;

    //
    //  Set up description of event, including exception record
    //

    WaitStateChange->NewState = DbgKdExceptionStateChange;
    WaitStateChange->ProcessorLevel = KeProcessorLevel;
    WaitStateChange->Processor = (USHORT)KeGetCurrentPrcb()->Number;
    WaitStateChange->NumberProcessors = (ULONG)KeNumberProcessors;
    WaitStateChange->Thread = (ULONG64)(LONG64)(LONG_PTR) KeGetCurrentThread();
    WaitStateChange->ProgramCounter = (ULONG64)(LONG64)(LONG_PTR) CONTEXT_TO_PROGRAM_COUNTER(ContextRecord);
    if (sizeof(EXCEPTION_RECORD) == sizeof(WaitStateChange->u.Exception.ExceptionRecord)) {
        KdpQuickMoveMemory((PCHAR)&WaitStateChange->u.Exception.ExceptionRecord,
                           (PCHAR)ExceptionRecord,
                           sizeof(EXCEPTION_RECORD));
    } else {
        ExceptionRecord32To64((PEXCEPTION_RECORD32)ExceptionRecord,
                              &WaitStateChange->u.Exception.ExceptionRecord
                              );
    }
    WaitStateChange->u.Exception.FirstChance = !SecondChance;

    //
    //  Copy instruction stream immediately following location of event
    //

    WaitStateChange->ControlReport.InstructionCount =
        (USHORT)KdpMoveMemory(
            (PCHAR)(&(WaitStateChange->ControlReport.InstructionStream[0])),
            (PCHAR)(WaitStateChange->ProgramCounter),
            DBGKD_MAXSTREAM
            );

    //
    //  Copy context record immediately following instruction stream
    //

    KdpMoveMemory(
        (PCHAR)(&WaitStateChange->Context),
        (PCHAR)ContextRecord,
        sizeof(*ContextRecord)
        );

    //
    //  Clear breakpoints in copied area
    //

    status = KdpDeleteBreakpointRange(
        (PVOID)WaitStateChange->ProgramCounter,
        (PVOID)((PUCHAR)WaitStateChange->ProgramCounter +
            WaitStateChange->ControlReport.InstructionCount - 1)
        );

    //
    //  If there were any breakpoints cleared, recopy the area without them
    //

    if (status == TRUE) {
        KdpMoveMemory(
            (PUCHAR) &(WaitStateChange->ControlReport.InstructionStream[0]),
            (PUCHAR) WaitStateChange->ProgramCounter,
            WaitStateChange->ControlReport.InstructionCount
            );
    }


    //
    //  Special registers for the x86
    //
    Prcb = KeGetCurrentPrcb();

    WaitStateChange->ControlReport.Dr6 =
        Prcb->ProcessorState.SpecialRegisters.KernelDr6;

    WaitStateChange->ControlReport.Dr7 =
        Prcb->ProcessorState.SpecialRegisters.KernelDr7;

    WaitStateChange->ControlReport.SegCs  = (USHORT)(ContextRecord->SegCs);
    WaitStateChange->ControlReport.SegDs  = (USHORT)(ContextRecord->SegDs);
    WaitStateChange->ControlReport.SegEs  = (USHORT)(ContextRecord->SegEs);
    WaitStateChange->ControlReport.SegFs  = (USHORT)(ContextRecord->SegFs);
    WaitStateChange->ControlReport.EFlags = ContextRecord->EFlags;

    WaitStateChange->ControlReport.ReportFlags = REPORT_INCLUDES_SEGS;

}

VOID
KdpGetStateChange(
    IN PDBGKD_MANIPULATE_STATE64 ManipulateState,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    Extract continuation control data from Manipulate_State message

Arguments:

    ManipulateState - supplies pointer to Manipulate_State packet

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{
    PKPRCB Prcb;
    ULONG  Processor;

    if (NT_SUCCESS(ManipulateState->u.Continue2.ContinueStatus) == TRUE) {

        //
        // If NT_SUCCESS returns TRUE, then the debugger is doing a
        // continue, and it makes sense to apply control changes.
        // Otherwise the debugger is saying that it doesn't know what
        // to do with this exception, so control values are ignored.
        //

        if (ManipulateState->u.Continue2.ControlSet.TraceFlag == TRUE) {
            ContextRecord->EFlags |= 0x100L;

        } else {
            ContextRecord->EFlags &= ~0x100L;

        }

        for (Processor = 0; Processor < (ULONG)KeNumberProcessors; Processor++) {
            Prcb = KiProcessorBlock[Processor];

            Prcb->ProcessorState.SpecialRegisters.KernelDr7 =
                ManipulateState->u.Continue2.ControlSet.Dr7;

            Prcb->ProcessorState.SpecialRegisters.KernelDr6 = 0L;
        }
        if (ManipulateState->u.Continue2.ControlSet.CurrentSymbolStart != 1) {
            KdpCurrentSymbolStart = ManipulateState->u.Continue2.ControlSet.CurrentSymbolStart;
            KdpCurrentSymbolEnd = ManipulateState->u.Continue2.ControlSet.CurrentSymbolEnd;
        }
    }
}


VOID
KdpReadControlSpace(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a read control space state
    manipulation message.  Its function is to read implementation
    specific system data.

    IMPLEMENTATION NOTE:

        On the X86, control space is defined as follows:

            0:  Base of KPROCESSOR_STATE structure. (KPRCB.ProcessorState)
                    This includes CONTEXT record,
                    followed by a SPECIAL_REGISTERs record

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_READ_MEMORY64 a = &m->u.ReadMemory;
    STRING MessageHeader;
    ULONG Length, t;
    PVOID StartAddr;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);

    if (a->TransferCount > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64))) {
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);
    } else {
        Length = a->TransferCount;
    }
    if ((a->TargetBaseAddress < (ULONG64)(sizeof(KPROCESSOR_STATE))) &&
        (m->Processor < (USHORT)KeNumberProcessors)) {
        t = (ULONG)(sizeof(KPROCESSOR_STATE)) - (ULONG)(a->TargetBaseAddress);
        if (t < Length) {
            Length = t;
        }
        StartAddr = (PVOID)((ULONG)a->TargetBaseAddress +
                            (ULONG)&(KiProcessorBlock[m->Processor]->ProcessorState));
        AdditionalData->Length = (USHORT)KdpMoveMemory(
                                    AdditionalData->Buffer,
                                    StartAddr,
                                    Length
                                    );

        if (Length == AdditionalData->Length) {
            m->ReturnStatus = STATUS_SUCCESS;
        } else {
            m->ReturnStatus = STATUS_UNSUCCESSFUL;
        }
        a->ActualBytesRead = AdditionalData->Length;

    } else {
        AdditionalData->Length = 0;
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        a->ActualBytesRead = 0;
    }

    KdpSendPacket(
        PACKET_TYPE_KD_STATE_MANIPULATE,
        &MessageHeader,
        AdditionalData
        );
    UNREFERENCED_PARAMETER(Context);
}

VOID
KdpWriteControlSpace(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a write control space state
    manipulation message.  Its function is to write implementation
    specific system data.

    Control space for x86 is as defined above.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_WRITE_MEMORY64 a = &m->u.WriteMemory;
    ULONG Length;
    STRING MessageHeader;
    PVOID StartAddr;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    if ((((PUCHAR)a->TargetBaseAddress + a->TransferCount) <=
        (PUCHAR)(sizeof(KPROCESSOR_STATE))) && (m->Processor < (USHORT)KeNumberProcessors)) {

        StartAddr = (PVOID)((ULONG)a->TargetBaseAddress +
                            (ULONG)&(KiProcessorBlock[m->Processor]->ProcessorState));

        Length = KdpMoveMemory(
                            StartAddr,
                            AdditionalData->Buffer,
                            AdditionalData->Length
                            );

        if (Length == AdditionalData->Length) {
            m->ReturnStatus = STATUS_SUCCESS;
        } else {
            m->ReturnStatus = STATUS_UNSUCCESSFUL;
        }
        a->ActualBytesWritten = Length;

    } else {
        AdditionalData->Length = 0;
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        a->ActualBytesWritten = 0;
    }

    KdpSendPacket(
        PACKET_TYPE_KD_STATE_MANIPULATE,
        &MessageHeader,
        AdditionalData
        );
    UNREFERENCED_PARAMETER(Context);
}

VOID
KdpReadIoSpace(
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

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);

    m->ReturnStatus = STATUS_SUCCESS;

    //
    // Check Size and Alignment
    //

    switch ( a->DataSize ) {
        case 1:
            a->DataValue = (ULONG)READ_PORT_UCHAR((PUCHAR)a->IoAddress);
            break;
        case 2:
            if ((ULONG)a->IoAddress & 1 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                a->DataValue = (ULONG)READ_PORT_USHORT((PUSHORT)a->IoAddress);
            }
            break;
        case 4:
            if ((ULONG)a->IoAddress & 3 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                a->DataValue = READ_PORT_ULONG((PULONG)a->IoAddress);
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
    UNREFERENCED_PARAMETER(Context);
}

VOID
KdpWriteIoSpace(
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

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);

    m->ReturnStatus = STATUS_SUCCESS;

    //
    // Check Size and Alignment
    //

    switch ( a->DataSize ) {
        case 1:
            WRITE_PORT_UCHAR((PUCHAR)a->IoAddress, (UCHAR)a->DataValue);
            break;
        case 2:
            if ((ULONG)a->IoAddress & 1 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                WRITE_PORT_USHORT((PUSHORT)a->IoAddress, (USHORT)a->DataValue);
            }
            break;
        case 4:
            if ((ULONG)a->IoAddress & 3 ) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                WRITE_PORT_ULONG((PULONG)a->IoAddress, a->DataValue);
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
    UNREFERENCED_PARAMETER(Context);
}

VOID
KdpReadMachineSpecificRegister(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a read MSR
    manipulation message.  Its function is to read the MSR.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_READ_WRITE_MSR a = &m->u.ReadWriteMsr;
    STRING MessageHeader;
    LARGE_INTEGER l;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);

    m->ReturnStatus = STATUS_SUCCESS;

    try {
        l.QuadPart = RDMSR(a->Msr);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        l.QuadPart = 0;
        m->ReturnStatus = STATUS_NO_SUCH_DEVICE;
    }

    a->DataValueLow  = l.LowPart;
    a->DataValueHigh = l.HighPart;

    KdpSendPacket(
        PACKET_TYPE_KD_STATE_MANIPULATE,
        &MessageHeader,
        NULL
        );
    UNREFERENCED_PARAMETER(Context);
}

VOID
KdpWriteMachineSpecificRegister(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a write of a MSR
    manipulation message.  Its function is to write to the MSR

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_READ_WRITE_MSR a = &m->u.ReadWriteMsr;
    STRING MessageHeader;
    LARGE_INTEGER l;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    ASSERT(AdditionalData->Length == 0);

    m->ReturnStatus = STATUS_SUCCESS;

    l.HighPart = a->DataValueHigh;
    l.LowPart = a->DataValueLow;

    try {
        WRMSR (a->Msr, l.QuadPart);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        m->ReturnStatus = STATUS_NO_SUCH_DEVICE;
    }

    KdpSendPacket(
        PACKET_TYPE_KD_STATE_MANIPULATE,
        &MessageHeader,
        NULL
        );
    UNREFERENCED_PARAMETER(Context);
}

/*** KdpGetCallNextOffset - compute "next" instruction on a call-like instruction
*
*   Purpose:
*       Compute how many bytes are in a call-type instruction
*       so that a breakpoint can be set upon this instruction's
*       return.  Treat indirect jmps as if they were call/ret/ret
*
*   Returns:
*       offset to "next" instruction, or 0 if it wasn't a call instruction.
*
*************************************************************************/

ULONG
KdpGetCallNextOffset (
    ULONG Pc,
    PCONTEXT ContextRecord
    )
{
    UCHAR membuf[2];
    UCHAR opcode;
    ULONG sib;
    ULONG disp;

    KdpMoveMemory( membuf, (PVOID)Pc, 2 );
    opcode = membuf[0];

    if ( opcode == 0xe8 ) {         //  CALL 32 bit disp
        return Pc+5;
    } else if ( opcode == 0x9a ) {  //  CALL 16:32
        return Pc+7;
    } else if ( opcode == 0xff ) {
        if ( membuf[1] == 0x25) {   //  JMP indirect
            return KdpGetReturnAddress( ContextRecord );
        }
        sib = ((membuf[1] & 0x07) == 0x04) ? 1 : 0;
        disp = (membuf[1] & 0xc0) >> 6;
        switch (disp) {
        case 0:
            if ( (membuf[1] & 0x07) == 0x05 ) {
                disp = 4; // disp32 alone
            } else {
                // disp = 0; // no displacement with reg or sib
            }
            break;
        case 1:
            // disp = 1; // disp8 with reg or sib
            break;
        case 2:
            disp = 4; // disp32 with reg or sib
            break;
        case 3:
            disp = 0; // direct register addressing (e.g., call esi)
            break;
        }
        return Pc + 2 + sib + disp;
    }

    return 0;

} // KdpGetCallNextOffset
