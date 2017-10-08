/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/kd64.h
 * PURPOSE:         Internal header for the KD64 Library
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

//
// Default size of the DbgPrint log buffer
//
#if DBG
#define KD_DEFAULT_LOG_BUFFER_SIZE                      0x8000
#else
#define KD_DEFAULT_LOG_BUFFER_SIZE                      0x1000
#endif

//
// Maximum supported number of breakpoints
//
#define KD_BREAKPOINT_MAX   32

//
// Highest limit starting which we consider that breakpoint addresses
// are either in system space, or in user space but inside shared DLLs.
//
// I'm wondering whether this can be computed using MmHighestUserAddress
// or whether there is already some #define somewhere else...
// See http://www.drdobbs.com/windows/faster-dll-load-load/184416918
// and http://www.drdobbs.com/rebasing-win32-dlls/184416272
// for a tentative explanation.
//
#define KD_HIGHEST_USER_BREAKPOINT_ADDRESS  (PVOID)0x60000000  // MmHighestUserAddress

//
// Breakpoint Status Flags
//
#define KD_BREAKPOINT_ACTIVE    0x01
#define KD_BREAKPOINT_PENDING   0x02
#define KD_BREAKPOINT_SUSPENDED 0x04
#define KD_BREAKPOINT_EXPIRED   0x08

//
// Structure for Breakpoints
//
typedef struct _BREAKPOINT_ENTRY
{
    ULONG Flags;
    ULONG_PTR DirectoryTableBase;
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
// Determines if the kernel debugger must handle a particular trap
//
BOOLEAN
NTAPI
KdIsThisAKdTrap(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN KPROCESSOR_MODE PreviousMode
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
// Debugger Enter, Exit, Enable and Disable
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
    IN BOOLEAN Enable
);

NTSTATUS
NTAPI
KdEnableDebuggerWithLock(
    IN BOOLEAN NeedLock
);

NTSTATUS
NTAPI
KdDisableDebuggerWithLock(
    IN BOOLEAN NeedLock
);

//
// Debug Event Handlers
//
NTSTATUS
NTAPI
KdpPrint(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN LPSTR String,
    IN USHORT Length,
    IN KPROCESSOR_MODE PreviousMode,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    OUT PBOOLEAN Handled
);

USHORT
NTAPI
KdpPrompt(
    IN LPSTR PromptString,
    IN USHORT PromptLength,
    OUT LPSTR ResponseString,
    IN USHORT MaximumResponseLength,
    IN KPROCESSOR_MODE PreviousMode,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

VOID
NTAPI
KdpSymbol(
    IN PSTRING DllPath,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN BOOLEAN Unload,
    IN KPROCESSOR_MODE PreviousMode,
    IN PCONTEXT ContextRecord,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

VOID
NTAPI
KdpCommandString(
    IN PSTRING NameString,
    IN PSTRING CommandString,
    IN KPROCESSOR_MODE PreviousMode,
    IN PCONTEXT ContextRecord,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

//
// State Change Notifications
//
VOID
NTAPI
KdpReportLoadSymbolsStateChange(
    IN PSTRING PathName,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN BOOLEAN Unload,
    IN OUT PCONTEXT Context
);

VOID
NTAPI
KdpReportCommandStringStateChange(
    IN PSTRING NameString,
    IN PSTRING CommandString,
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
ULONG
NTAPI
KdpAddBreakpoint(
    IN PVOID Address
);

VOID
NTAPI
KdSetOwedBreakpoints(
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

VOID
NTAPI
KdpSuspendBreakPoint(
    IN ULONG BpEntry
);

VOID
NTAPI
KdpRestoreAllBreakpoints(
    VOID
);

VOID
NTAPI
KdpSuspendAllBreakPoints(
    VOID
);

//
// Routine to determine if it is safe to disable the debugger
//
NTSTATUS
NTAPI
KdpAllowDisable(
    VOID
);

//
// Safe memory read & write Support
//
NTSTATUS
NTAPI
KdpCopyMemoryChunks(
    IN ULONG64 Address,
    IN PVOID Buffer,
    IN ULONG TotalSize,
    IN ULONG ChunkSize,
    IN ULONG Flags,
    OUT PULONG ActualSize OPTIONAL
);

//
// Internal memory handling routines for KD isolation
//
VOID
NTAPI
KdpMoveMemory(
    IN PVOID Destination,
    IN PVOID Source,
    IN SIZE_T Length
);

VOID
NTAPI
KdpZeroMemory(
    IN PVOID Destination,
    IN SIZE_T Length
);

//
// Low Level Support Routines for the KD API
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
    IN PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange,
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
    IN ULONG Offset,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG ActualLength
);

NTSTATUS
NTAPI
KdpSysWriteBusData(
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    IN PVOID Buffer,
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
    IN PVOID DataValue,
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
    IN PVOID DataValue,
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
extern BOOLEAN KdAutoEnableOnEvent;
extern BOOLEAN KdBlockEnable;
extern BOOLEAN KdIgnoreUmExceptions;
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
extern BOOLEAN KdpContextSent;
extern KSPIN_LOCK KdpDebuggerLock;
extern LARGE_INTEGER KdTimerStop, KdTimerStart, KdTimerDifference;
extern ULONG KdComponentTableSize;
extern ULONG Kd_WIN2000_Mask;
extern PULONG KdComponentTable[104];
extern CHAR KdpMessageBuffer[0x1000], KdpPathBuffer[0x1000];
extern CHAR KdPrintDefaultCircularBuffer[KD_DEFAULT_LOG_BUFFER_SIZE];
extern BREAKPOINT_ENTRY KdpBreakpointTable[KD_BREAKPOINT_MAX];
extern KD_BREAKPOINT_TYPE KdpBreakpointInstruction;
extern BOOLEAN KdpOweBreakpoint;
extern BOOLEAN BreakpointsSuspended;
extern ULONG KdpNumInternalBreakpoints;
extern ULONG_PTR KdpCurrentSymbolStart, KdpCurrentSymbolEnd;
extern ULONG TraceDataBuffer[40];
extern ULONG TraceDataBufferPosition;
