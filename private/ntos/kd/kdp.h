/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdp.h

Abstract:

    Private include file for the Kernel Debugger subcomponent
    of the NTOS project

Author:

    Mike O'Leary (mikeol) 29-June-1989

Revision History:

--*/

#include "ntos.h"
#include "ki.h"
#include "ntdbg.h"
#include "string.h"
#include "stdlib.h"
#include "kdpcpu.h"

#if defined(_WIN64)
#error "Build KD64 for 64-bit systems"
#endif

//
// Define constants.
//

//
// Addresses above GLOBAL_BREAKPOINT_LIMIT are either in system space
// or part of dynlink, so we treat them as global.
//

#define GLOBAL_BREAKPOINT_LIMIT 1610612736L // 1.5gigabytes

//
// Define breakpoint table entry structure.
//

#define KD_BREAKPOINT_IN_USE        0x00000001
#define KD_BREAKPOINT_NEEDS_WRITE   0x00000002
#define KD_BREAKPOINT_SUSPENDED     0x00000004
#define KD_BREAKPOINT_NEEDS_REPLACE 0x00000008
// IA64 specific defines
#define KD_BREAKPOINT_STATE_MASK    0x0000000f
#define KD_BREAKPOINT_IA64_MASK     0x000f0000
#define KD_BREAKPOINT_IA64_MODE     0x00010000   // IA64 mode
#define KD_BREAKPOINT_IA64_MOVL     0x00020000   // MOVL instruction displaced

//
// status Constants for Packet waiting
//

#define KDP_PACKET_RECEIVED 0
#define KDP_PACKET_TIMEOUT 1
#define KDP_PACKET_RESEND 2


typedef struct _BREAKPOINT_ENTRY {
    ULONG Flags;
    ULONG_PTR DirectoryTableBase;
    PVOID Address;
    KDP_BREAKPOINT_TYPE Content;
} BREAKPOINT_ENTRY, *PBREAKPOINT_ENTRY;


//
// Misc defines
//

#define MAXIMUM_RETRIES 20

#define DBGKD_MAX_SPECIAL_CALLS 10

typedef struct _TRACE_DATA_SYM {
    ULONG SymMin;
    ULONG SymMax;
} TRACE_DATA_SYM, *PTRACE_DATA_SYM;

//
// Define function prototypes.
//

VOID
KdpReboot (
    VOID
    );

BOOLEAN
KdpPrintString (
    IN PSTRING Output
    );

BOOLEAN
KdpPromptString (
    IN PSTRING Output,
    IN OUT PSTRING Input
    );

ULONG
KdpAddBreakpoint (
    IN PVOID Address
    );

BOOLEAN
KdpDeleteBreakpoint (
    IN ULONG Handle
    );

BOOLEAN
KdpDeleteBreakpointRange (
    IN PVOID Lower,
    IN PVOID Upper
    );

#if defined(_IA64_)

BOOLEAN
KdpSuspendBreakpointRange (
    IN PVOID Lower,
    IN PVOID Upper
    );

BOOLEAN
KdpRestoreBreakpointRange (
    IN PVOID Lower,
    IN PVOID Upper
    );
#endif

ULONG
KdpMoveMemory (
    IN PCHAR Destination,
    IN PCHAR Source,
    IN ULONG Length
    );

VOID
KdpQuickMoveMemory (
    IN PCHAR Destination,
    IN PCHAR Source,
    IN ULONG Length
    );

ULONG
KdpReceivePacket (
    IN ULONG ExpectedPacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength
    );

VOID
KdpSetLoadState(
    IN PDBGKD_WAIT_STATE_CHANGE WaitStateChange,
    IN PCONTEXT ContextRecord
    );

VOID
KdpSetStateChange(
    IN PDBGKD_WAIT_STATE_CHANGE WaitStateChange,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN BOOLEAN SecondChance
    );

VOID
KdpGetStateChange(
    IN PDBGKD_MANIPULATE_STATE ManipulateState,
    IN PCONTEXT ContextRecord
    );

VOID
KdpSendPacket (
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData OPTIONAL
    );

BOOLEAN
KdpStub (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    );

BOOLEAN
KdpTrap (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    );

VOID
KdpDisplayString (
    IN PCHAR Output
    );

VOID
KdpWriteComPacket (
    USHORT,
    USHORT,
    PVOID,
    PVOID,
    PVOID
    );

BOOLEAN
KdpReadComPacket (
    VOID
    );

BOOLEAN
KdpSwitchProcessor (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord,
    IN BOOLEAN SecondChance
    );

BOOLEAN
KdpReportExceptionStateChange (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord,
    IN BOOLEAN SecondChance
    );

BOOLEAN
KdpReportLoadSymbolsStateChange (
    IN PSTRING PathName,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN BOOLEAN UnloadSymbols,
    IN OUT PCONTEXT ContextRecord
    );

KCONTINUE_STATUS
KdpSendWaitContinue(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData OPTIONAL,
    IN OUT PCONTEXT ContextRecord
    );

VOID
KdpReadVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpReadVirtualMemory64(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteVirtualMemory64(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpReadPhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWritePhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpGetContext(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpSetContext(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteBreakpoint(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpRestoreBreakpoint(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpReadControlSpace(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteControlSpace(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpReadIoSpace(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpReadMachineSpecificRegister(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteIoSpace(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteMachineSpecificRegister(
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

#ifdef _ALPHA_

VOID
KdpReadIoSpaceExtended (
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteIoSpaceExtended (
    IN PDBGKD_MANIPULATE_STATE m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

#endif


VOID
KdpSuspendBreakpoint (
    ULONG Handle
    );

VOID
KdpSuspendAllBreakpoints (
    VOID
    );

VOID
KdpRestoreAllBreakpoints (
    VOID
    );

VOID
KdpTimeSlipDpcRoutine (
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

VOID
KdpTimeSlipWork (
    IN PVOID Context
    );

//
// Define dummy prototype so the address of the standard breakpoint instruction
// can be captured.
//
// N.B. This function is NEVER called.
//

VOID
RtlpBreakWithStatusInstruction (
    VOID
    );

//
// Define external references.
//

#define KDP_MESSAGE_BUFFER_SIZE 4096

extern BREAKPOINT_ENTRY KdpBreakpointTable[BREAKPOINT_TABLE_SIZE];
extern BOOLEAN KdpControlCPending;
extern KSPIN_LOCK KdpDebuggerLock;
extern PKDEBUG_ROUTINE KiDebugRoutine;
extern PKDEBUG_SWITCH_ROUTINE KiDebugSwitchRoutine;
extern KDP_BREAKPOINT_TYPE KdpBreakpointInstruction;
extern UCHAR KdpMessageBuffer[KDP_MESSAGE_BUFFER_SIZE];
extern UCHAR KdpPathBuffer[KDP_MESSAGE_BUFFER_SIZE];
extern ULONG KdpOweBreakpoint;
extern ULONG KdpNextPacketIdToSend;
extern ULONG KdpPacketIdExpected;

extern LARGE_INTEGER KdPerformanceCounterRate;
extern LARGE_INTEGER KdTimerStart;
extern LARGE_INTEGER KdTimerStop;
extern LARGE_INTEGER KdTimerDifference;

extern BOOLEAN BreakpointsSuspended;
extern PVOID KdpNtosImageBase;
extern LIST_ENTRY KdpDebuggerDataListHead;

typedef struct {
    ULONG_PTR Addr;             // pc address of breakpoint
    ULONG Flags;                // Flags bits
    ULONG Calls;                // # of times traced routine called
    ULONG CallsLastCheck;       // # of calls at last periodic (1s) check
    ULONG MaxCallsPerPeriod;
    ULONG MinInstructions;      // largest number of instructions for 1 call
    ULONG MaxInstructions;      // smallest # of instructions for 1 call
    ULONG TotalInstructions;    // total instructions for all calls
    ULONG Handle;               // handle in (regular) bpt table
    PVOID Thread;               // Thread that's skipping this BP
    ULONG_PTR ReturnAddress;    // return address (if not COUNTONLY)
} DBGKD_INTERNAL_BREAKPOINT, *PDBGKD_INTERNAL_BREAKPOINT;


#define DBGKD_MAX_INTERNAL_BREAKPOINTS 20
extern DBGKD_INTERNAL_BREAKPOINT KdpInternalBPs[DBGKD_MAX_INTERNAL_BREAKPOINTS];

extern ULONG_PTR   KdpCurrentSymbolStart;
extern ULONG_PTR   KdpCurrentSymbolEnd;
extern LONG    KdpNextCallLevelChange;
extern ULONG_PTR   KdSpecialCalls[];
extern ULONG   KdNumberOfSpecialCalls;
extern ULONG_PTR   InitialSP;
extern ULONG   KdpNumInternalBreakpoints;
extern KTIMER  InternalBreakpointTimer;
extern KDPC    InternalBreakpointCheckDpc;
extern BOOLEAN KdpPortLocked;
extern LARGE_INTEGER   KdpTimeEntered;

extern DBGKD_TRACE_DATA TraceDataBuffer[];
extern ULONG            TraceDataBufferPosition;
extern TRACE_DATA_SYM   TraceDataSyms[];
extern UCHAR NextTraceDataSym;
extern UCHAR NumTraceDataSyms;
extern ULONG IntBPsSkipping;
extern BOOLEAN WatchStepOver;
extern PVOID WSOThread;
extern ULONG WSOEsp;
extern ULONG WatchStepOverHandle;
extern ULONG_PTR WatchStepOverBreakAddr;
extern BOOLEAN WatchStepOverSuspended;
extern ULONG InstructionsTraced;
extern BOOLEAN SymbolRecorded;
extern LONG CallLevelChange;
extern LONG oldpc;
extern BOOLEAN InstrCountInternal;
extern BOOLEAN BreakpointsSuspended;
extern BOOLEAN KdpControlCPending;
extern BOOLEAN KdpControlCPressed;
extern ULONG KdpRetryCount;
extern ULONG KdpNumberRetries;

extern KDP_BREAKPOINT_TYPE KdpBreakpointInstruction;
extern ULONG KdpOweBreakpoint;
extern ULONG KdpNextPacketIdToSend;
extern ULONG KdpPacketIdExpected;
extern PVOID KdpNtosImageBase;
extern UCHAR  KdPrintCircularBuffer[KDPRINTBUFFERSIZE];
extern PUCHAR KdPrintWritePointer;
extern ULONG  KdPrintRolloverCount;
extern KSPIN_LOCK KdpPrintSpinLock;
extern DEBUG_PARAMETERS KdDebugParameters;
extern KSPIN_LOCK KdpDataSpinLock;
extern LIST_ENTRY KdpDebuggerDataListHead;
extern KDDEBUGGER_DATA KdDebuggerDataBlock;
extern KDPC KdpTimeSlipDpc;
extern WORK_QUEUE_ITEM KdpTimeSlipWorkItem;
extern KTIMER KdpTimeSlipTimer;
extern ULONG KdpTimeSlipPending;
extern KSPIN_LOCK KdpTimeSlipEventLock;
extern PVOID KdpTimeSlipEvent;
extern BOOLEAN KdpDebuggerStructuresInitialized;
extern ULONG KdEnteredDebugger;

//
// Private procedure prototypes
//

VOID
KdpInitCom(
    VOID
    );

VOID
KdpPortLock(
    VOID
    );

VOID
KdpPortUnlock(
    VOID
    );

BOOLEAN
KdpPollBreakInWithPortLock(
    VOID
    );

USHORT
KdpReceivePacketLeader (
    IN ULONG PacketType,
    OUT PULONG PacketLeader
    );

#if DBG

#include <stdio.h>
#define DPRINT(s) KdpDprintf s

VOID
KdpDprintf(
    IN PCHAR f,
    ...
    );

#else

#define DPRINT(s)

#endif
