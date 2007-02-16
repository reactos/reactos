//
// Kernel Debugger Port Definition
//
extern BOOLEAN _KdDebuggerEnabled;
extern BOOLEAN _KdDebuggerNotPresent;
extern BOOLEAN KdBreakAfterSymbolLoad;

/* KD GLOBALS  ***************************************************************/

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

extern PKDEBUG_ROUTINE KiDebugRoutine;
