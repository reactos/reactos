typedef struct _BREAKPOINT_ENTRY
{
    ULONG Flags;
    PKPROCESS Process;
    PVOID Address;
    UCHAR Content;
} BREAKPOINT_ENTRY, *PBREAKPOINT_ENTRY;

typedef enum _KDP_BREAKPOINT_FLAGS
{
    KdpBreakpointActive = 1,
    KdpBreakpointPending = 2,
    KdpBreakpointSuspended = 4,
    KdpBreakpointExpired = 8
} KDP_BREAKPOINT_FLAGS;

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

BOOLEAN
NTAPI
KdpEnterDebuggerException(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
);

VOID
NTAPI
KdpTimeSlipWork(
    IN PVOID Context
);

BOOLEAN
NTAPI
KdpSwitchProcessor(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord,
    IN BOOLEAN SecondChanceException
);

VOID
NTAPI
KdpTimeSlipDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

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

ULONG
NTAPI
KdpPrint(
    IN ULONG ComponentId,
    IN ULONG ComponentMask,
    IN LPSTR String,
    IN ULONG Length,
    IN KPROCESSOR_MODE PreviousMode,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    OUT PBOOLEAN Status
);

ULONG
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

BOOLEAN
NTAPI
KdpPollBreakInWithPortLock(
    VOID
);

BOOLEAN
NTAPI
KdpReportLoadSymbolsStateChange(
    IN PSTRING PathName,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN BOOLEAN Unload,
    IN OUT PCONTEXT Context
);

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
extern BOOLEAN KdpControlCPressed;
extern BOOLEAN KdpControlCWaiting;
extern BOOLEAN KdpPortLocked;
extern KSPIN_LOCK KdpDebuggerLock;
extern LARGE_INTEGER KdTimerStop, KdTimerStart, KdTimerDifference;
extern ULONG KdComponentTableSize;
extern ULONG Kd_WIN2000_Mask;
extern PULONG KdComponentTable[104];
extern CHAR KdpMessageBuffer[4096], KdpPathBuffer[4096];
extern BREAKPOINT_ENTRY KdpBreakpointTable[20];
extern ULONG KdpBreakpointInstruction;
extern BOOLEAN KdpOweBreakpoint;
extern BOOLEAN BreakpointsSuspended;
extern ULONG KdpNumInternalBreakpoints;
extern ULONG KdpCurrentSymbolStart, KdpCurrentSymbolEnd;
