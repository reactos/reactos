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
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include "string.h"
#include "stdlib.h"


#if defined(_ALPHA_)

#include "alphaops.h"

//
// Define KD private PCR routines.
//
// Using the following private KD routines allows the kernel debugger to
// step over breakpoints in modules that call the standard PCR routines.
//

PKPCR KdpGetPcr();

ULONG KdpReadInternalProcessorState(PVOID, ULONG);
ULONG KdpReadInternalProcessorCounters(PVOID, ULONG);

struct _KPRCB *
KdpGetCurrentPrcb();

struct _KTHREAD *
KdpGetCurrentThread();

//
// Redefine the standard PCR routines
//
#undef KiPcr
#define KiPcr KdpGetPcr()

#undef KeGetPcr
#undef KeGetCurrentPrcb
#undef KeGetCurrentThread
#undef KeIsExecutingDpc
#define KeGetPcr() KdpGetPcr()
#define KeGetCurrentPrcb() KdpGetCurrentPrcb()
#define KeGetCurrentThread() KdpGetCurrentThread()

//
// Define TYPES
//

#define KDP_BREAKPOINT_TYPE  ULONG

// longword aligned
#define KDP_BREAKPOINT_ALIGN 3

// actual instruction is "call_pal kbpt"
#define KDP_BREAKPOINT_VALUE KBPT_FUNC


#elif defined(_IA64_)

// IA64 instruction is in a 128-bit bundle. Each bundle consists of 3 instruction slots.
// Each instruction slot is 41-bit long.
//
//
//            127           87 86           46 45            5 4      1 0
//            ------------------------------------------------------------
//            |    slot 2     |    slot 1     |    slot 0     |template|S|
//            ------------------------------------------------------------
//
//            127        96 95         64 63          32 31             0
//            ------------------------------------------------------------
//            |  byte 3    |  byte 2     |  byte 1      |   byte 0       |
//            ------------------------------------------------------------
//
// This presents two incompatibilities with conventional processors:
//              1. The IA64 IP address is at the bundle bundary. The instruction slot number is
//                 stored in ISR.ei at the time of exception.
//              2. The 41-bit instruction format is not byte-aligned.
//
// Break instruction insertion must be done with proper bit-shifting to align with the selected
// instruction slot. Further, to insert break instruction insertion at a specific slot, we must
// be able to specify instruction slot as part of the address. We therefore define an EM address as
// bundle address + slot number with the least significant two bit always zero:
//
//                      31                 4 3  2  1  0
//                      --------------------------------
//                      |  bundle address    |slot#|0 0|
//                      --------------------------------
//
// The EM address as defined is the byte-aligned address that is closest to the actual instruction slot.
// i.e., The EM instruction address of slot #0 is equal to bundle address.
//                                     slot #1 is equal to bundle address + 4.
//                                     slot #2 is equal to bundle address + 8.

//
//  Upon exception, the bundle address is kept in IIP, and the instruction slot which caused
//  the exception is in ISR.ei. Kernel exception handler will construct the flat address and
//  export it in ExceptionRecord.ExceptionAddress.


#define KDP_BREAKPOINT_TYPE  ULONGLONG          // 64-bit ULONGLONG type is needed to cover 41-bit EM break instruction.
#define KDP_BREAKPOINT_ALIGN 0x3                // An EM address consists of bundle and slot number and is 32-bit aligned.
#define KDP_BREAKPOINT_VALUE (BREAK_INSTR | (DEBUG_STOP_BREAKPOINT << 6))	

#elif defined(_X86_)

#define KDP_BREAKPOINT_TYPE  UCHAR
#define KDP_BREAKPOINT_ALIGN 0
#define KDP_BREAKPOINT_VALUE 0xcc

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

#if i386

NTSTATUS
KdGetTraceInformation (
    OUT PVOID TraceInformation,
    IN ULONG TraceInformationLength,
    OUT PULONG RequiredLength
    );

VOID
KdSetInternalBreakpoint (
    IN PDBGKD_MANIPULATE_STATE64 m
    );

#endif

NTSTATUS
KdQuerySpecialCalls (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN ULONG Length,
    OUT PULONG RequiredLength
    );

VOID
KdSetSpecialCall (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PCONTEXT ContextRecord
    );

VOID
KdClearSpecialCalls (
    VOID
    );

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
    IN PDBGKD_WAIT_STATE_CHANGE64 WaitStateChange,
    IN PCONTEXT ContextRecord
    );

VOID
KdpSetStateChange(
    IN PDBGKD_WAIT_STATE_CHANGE64 WaitStateChange,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN BOOLEAN SecondChance
    );

VOID
KdpGetStateChange(
    IN PDBGKD_MANIPULATE_STATE64 ManipulateState,
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
    IN PEXCEPTION_RECORD ExceptionRecord64,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
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
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

#if 0
VOID
KdpReadVirtualMemory64(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );
#endif

VOID
KdpWriteVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

#if 0
VOID
KdpWriteVirtualMemory64(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );
#endif

VOID
KdpReadPhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWritePhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpCheckLowMemory(
    IN PDBGKD_MANIPULATE_STATE64 m
    );

VOID
KdpGetContext(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpSetContext(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteBreakpoint(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpRestoreBreakpoint(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpReadControlSpace(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteControlSpace(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpReadIoSpace(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpReadMachineSpecificRegister(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteIoSpace(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteMachineSpecificRegister(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

#ifdef _ALPHA_

VOID
KdpReadIoSpaceExtended (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpWriteIoSpaceExtended (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpGetBusData (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
KdpSetBusData (
    IN PDBGKD_MANIPULATE_STATE64 m,
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
    ULONG64 Addr;                 // pc address of breakpoint
    ULONG Flags;                // Flags bits
    ULONG Calls;                // # of times traced routine called
    ULONG CallsLastCheck;       // # of calls at last periodic (1s) check
    ULONG MaxCallsPerPeriod;
    ULONG MinInstructions;      // largest number of instructions for 1 call
    ULONG MaxInstructions;      // smallest # of instructions for 1 call
    ULONG TotalInstructions;    // total instructions for all calls
    ULONG Handle;               // handle in (regular) bpt table
    PVOID Thread;               // Thread that's skipping this BP
    ULONG64 ReturnAddress;        // return address (if not COUNTONLY)
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
extern ULONG_PTR WSOEsp;
extern ULONG WatchStepOverHandle;
extern ULONG_PTR WatchStepOverBreakAddr;
extern BOOLEAN WatchStepOverSuspended;
extern ULONG InstructionsTraced;
extern BOOLEAN SymbolRecorded;
extern LONG CallLevelChange;
extern LONG_PTR oldpc;
extern BOOLEAN InstrCountInternal;
extern BOOLEAN BreakpointsSuspended;
extern BOOLEAN KdpControlCPending;
extern BOOLEAN KdpControlCPressed;
extern ULONG KdpRetryCount;
extern ULONG KdpNumberRetries;
extern ULONG KdpDefaultRetries;

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
extern KDDEBUGGER_DATA64 KdDebuggerDataBlock;
extern KDPC KdpTimeSlipDpc;
extern WORK_QUEUE_ITEM KdpTimeSlipWorkItem;
extern KTIMER KdpTimeSlipTimer;
extern ULONG KdpTimeSlipPending;
extern KSPIN_LOCK KdpTimeSlipEventLock;
extern PVOID KdpTimeSlipEvent;
extern BOOLEAN KdpDebuggerStructuresInitialized;
extern ULONG KdEnteredDebugger;

//
// !search support (page hit database)
//

//
// Hit database where search results are stored (kddata.c). 
// The debugger extensions know how to extract the information 
// from here.
//
// Note that the size of the hit database is large enough to
// accomodate any searches because the !search extension works
// in batches of pages < PAGE_SIZE and for every page we register only 
// one hit.
//

#define SEARCH_PAGE_HIT_DATABASE_SIZE PAGE_SIZE

extern PFN_NUMBER KdpSearchPageHits[SEARCH_PAGE_HIT_DATABASE_SIZE];
extern ULONG KdpSearchPageHitOffsets[SEARCH_PAGE_HIT_DATABASE_SIZE];

extern ULONG KdpSearchPageHitIndex;

//
// Set to true while a physical memory search is in progress.
// Reset at the end of the search. This is done in the debugger
// extension and it is a flag used by KdpCheckLowMemory to get
// onto a different code path.
//

extern LOGICAL KdpSearchInProgress;

//
// These variables store the current state of the search operation.
// They can be used to restore an interrupted search.
//

extern PFN_NUMBER KdpSearchStartPageFrame;
extern PFN_NUMBER KdpSearchEndPageFrame;

extern ULONG_PTR KdpSearchAddressRangeStart;
extern ULONG_PTR KdpSearchAddressRangeEnd;

//
// Checkpoint variable used to test if we have the right
// debugging symbols.
//

#define KDP_SEARCH_SYMBOL_CHECK 0xABCDDCBA

extern ULONG KdpSearchCheckPoint;

//
// Page search flags
//

#define KDP_SEARCH_ALL_OFFSETS_IN_PAGE 0x0001



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


