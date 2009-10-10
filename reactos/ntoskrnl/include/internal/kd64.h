/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/kd64.h
 * PURPOSE:         Internal header for the KD64 Library
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

//
// Maximum supported number of breakpoints
//
#define KD_BREAKPOINT_MAX                               32

//
// Breakpoint Status Flags
//
typedef enum _KDP_BREAKPOINT_FLAGS
{
    KdpBreakpointActive = 1,
    KdpBreakpointPending = 2,
    KdpBreakpointSuspended = 4,
    KdpBreakpointExpired = 8
} KDP_BREAKPOINT_FLAGS;

//
// Structure for Breakpoints
//
typedef struct _BREAKPOINT_ENTRY
{
    ULONG Flags;
    PKPROCESS Process;
    PVOID Address;
    KD_BREAKPOINT_TYPE Content;
} BREAKPOINT_ENTRY, *PBREAKPOINT_ENTRY;

//
// Debug and Multi-Processor Switch Routine Definitions
//
typedef
BOOLEAN
(NTAPI *PKDEBUG_ROUTINE)(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
);

typedef
BOOLEAN
(NTAPI *PKDEBUG_SWITCH_ROUTINE)(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN BOOLEAN SecondChance
);

//
// Initialization Routines
//
BOOLEAN
NTAPI
KdInitSystem(
    ULONG Reserved,
    PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
KdUpdateDataBlock(
    VOID
);

//
// Multi-Processor Switch Support
//
BOOLEAN
NTAPI
KdpSwitchProcessor(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord,
    IN BOOLEAN SecondChanceException
);

//
// Time Slip Support
//
VOID
NTAPI
KdpTimeSlipWork(
    IN PVOID Context
);

VOID
NTAPI
KdpTimeSlipDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

//
// Debug Trap Handlers
//
BOOLEAN
NTAPI
KdpStub(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChanceException
);

BOOLEAN
NTAPI
KdpTrap(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChanceException
);

//
// Port Locking
//
VOID
NTAPI
KdpPortLock(
    VOID
);

VOID
NTAPI
KdpPortUnlock(
    VOID
);

BOOLEAN
NTAPI
KdpPollBreakInWithPortLock(
    VOID
);

//
// Debugger Enable, Enter and Exit
//
BOOLEAN
NTAPI
KdEnterDebugger(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

VOID
NTAPI
KdExitDebugger(
    IN BOOLEAN Entered
);

NTSTATUS
NTAPI
KdEnableDebuggerWithLock(
    IN BOOLEAN NeedLock
);

//
// Debug Event Handlers
//
NTSTATUS
NTAPI
KdpPrint(
    IN ULONG ComponentId,
    IN ULONG ComponentMask,
    IN LPSTR String,
    IN USHORT Length,
    IN KPROCESSOR_MODE PreviousMode,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    OUT PBOOLEAN Status
);

BOOLEAN
NTAPI
KdpPrompt(
    IN LPSTR InString,
    IN USHORT InStringLength,
    OUT LPSTR OutString,
    IN USHORT OutStringLength,
    IN KPROCESSOR_MODE PreviousMode,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

VOID
NTAPI
KdpSymbol(
    IN PSTRING DllPath,
    IN PKD_SYMBOLS_INFO DllBase,
    IN BOOLEAN Unload,
    IN KPROCESSOR_MODE PreviousMode,
    IN PCONTEXT ContextRecord,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

VOID
NTAPI
KdpCommandString(
    IN ULONG Length,
    IN LPSTR String,
    IN KPROCESSOR_MODE PreviousMode,
    IN PCONTEXT ContextRecord,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

//
// State Change Notifications
//
BOOLEAN
NTAPI
KdpReportLoadSymbolsStateChange(
    IN PSTRING PathName,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN BOOLEAN Unload,
    IN OUT PCONTEXT Context
);

BOOLEAN
NTAPI
KdpReportExceptionStateChange(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT Context,
    IN BOOLEAN SecondChanceException
);

//
// Breakpoint Support
//
VOID
NTAPI
KdpRestoreAllBreakpoints(
    VOID
);

BOOLEAN
NTAPI
KdpDeleteBreakpoint(
    IN ULONG BpEntry
);

BOOLEAN
NTAPI
KdpDeleteBreakpointRange(
    IN PVOID Base,
    IN PVOID Limit
);

ULONG
NTAPI
KdpAddBreakpoint(
    IN PVOID Address
);

//
// Architecture dependent support routines
//

//
// Version
//
VOID
NTAPI
KdpSysGetVersion(
    IN PDBGKD_GET_VERSION64 Version
);

//
// Context
//
VOID
NTAPI
KdpGetStateChange(
    IN PDBGKD_MANIPULATE_STATE64 State,
    IN PCONTEXT Context
);

VOID
NTAPI
KdpSetContextState(
    IN PDBGKD_WAIT_STATE_CHANGE64 WaitStateChange,
    IN PCONTEXT Context
);

//
// MSR
//
NTSTATUS
NTAPI
KdpSysReadMsr(
    IN ULONG Msr,
    OUT PLARGE_INTEGER MsrValue
);

NTSTATUS
NTAPI
KdpSysWriteMsr(
    IN ULONG Msr,
    IN PLARGE_INTEGER MsrValue
);

//
// Bus
//
NTSTATUS
NTAPI
KdpSysReadBusData(
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    OUT PULONG ActualLength
);

NTSTATUS
NTAPI
KdpSysWriteBusData(
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    OUT PULONG ActualLength
);

//
// Control Space
//
NTSTATUS
NTAPI
KdpSysReadControlSpace(
    IN ULONG Processor,
    IN ULONG64 BaseAddress,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG ActualLength
);

NTSTATUS
NTAPI
KdpSysWriteControlSpace(
    IN ULONG Processor,
    IN ULONG64 BaseAddress,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG ActualLength
);

//
// I/O Space
//
NTSTATUS
NTAPI
KdpSysReadIoSpace(
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 IoAddress,
    IN PULONG DataValue,
    IN ULONG DataSize,
    OUT PULONG ActualDataSize
);

NTSTATUS
NTAPI
KdpSysWriteIoSpace(
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 IoAddress,
    IN PULONG DataValue,
    IN ULONG DataSize,
    OUT PULONG ActualDataSize
);

//
// Low Memory
//
NTSTATUS
NTAPI
KdpSysCheckLowMemory(
    IN ULONG Flags
);

//
// Internal routine for sending strings directly to the debugger
//
VOID
__cdecl
KdpDprintf(
    IN PCHAR Format,
    ...
);

//
// Global KD Data
//
extern DBGKD_GET_VERSION64 KdVersionBlock;
extern KDDEBUGGER_DATA64 KdDebuggerDataBlock;
extern LIST_ENTRY KdpDebuggerDataListHead;
extern KSPIN_LOCK KdpDataSpinLock;
extern LARGE_INTEGER KdPerformanceCounterRate;
extern LARGE_INTEGER KdTimerStart;
extern ULONG KdDisableCount;
extern KD_CONTEXT KdpContext;
extern PKDEBUG_ROUTINE KiDebugRoutine;
extern PKDEBUG_SWITCH_ROUTINE KiDebugSwitchRoutine;
extern BOOLEAN KdBreakAfterSymbolLoad;
extern BOOLEAN KdPitchDebugger;
extern BOOLEAN _KdDebuggerNotPresent;
extern BOOLEAN _KdDebuggerEnabled;
extern BOOLEAN KdAutoEnableOnEvent;
extern BOOLEAN KdPreviouslyEnabled;
extern BOOLEAN KdpDebuggerStructuresInitialized;
extern BOOLEAN KdEnteredDebugger;
extern KDPC KdpTimeSlipDpc;
extern KTIMER KdpTimeSlipTimer;
extern WORK_QUEUE_ITEM KdpTimeSlipWorkItem;
extern LONG KdpTimeSlipPending;
extern PKEVENT KdpTimeSlipEvent;
extern KSPIN_LOCK KdpTimeSlipEventLock;
extern BOOLEAN KdpPortLocked;
extern BOOLEAN KdpControlCPressed;
extern KSPIN_LOCK KdpDebuggerLock;
extern LARGE_INTEGER KdTimerStop, KdTimerStart, KdTimerDifference;
extern ULONG KdComponentTableSize;
extern ULONG Kd_WIN2000_Mask;
extern PULONG KdComponentTable[104];
extern CHAR KdpMessageBuffer[4096], KdpPathBuffer[4096];
extern BREAKPOINT_ENTRY KdpBreakpointTable[KD_BREAKPOINT_MAX];
extern KD_BREAKPOINT_TYPE KdpBreakpointInstruction;
extern BOOLEAN KdpOweBreakpoint;
extern BOOLEAN BreakpointsSuspended;
extern ULONG KdpNumInternalBreakpoints;
extern ULONG KdpCurrentSymbolStart, KdpCurrentSymbolEnd;
extern ULONG TraceDataBuffer[40];
extern ULONG TraceDataBufferPosition;
