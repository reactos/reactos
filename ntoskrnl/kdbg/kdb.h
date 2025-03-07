#pragma once
#include "../kd/kd.h"

/* TYPES *********************************************************************/

/* from kdb.c */
typedef CONTEXT KDB_KTRAP_FRAME, *PKDB_KTRAP_FRAME;

typedef enum _KDB_BREAKPOINT_TYPE
{
   KdbBreakPointNone = 0,
   KdbBreakPointSoftware,
   KdbBreakPointHardware,
   KdbBreakPointTemporary
} KDB_BREAKPOINT_TYPE;

typedef enum _KDB_ACCESS_TYPE
{
   KdbAccessRead,
   KdbAccessWrite,
   KdbAccessReadWrite,
   KdbAccessExec
} KDB_ACCESS_TYPE;

typedef struct _KDB_BREAKPOINT
{
   KDB_BREAKPOINT_TYPE    Type;         /* Type of breakpoint */
   BOOLEAN                Enabled;      /* Whether the bp is enabled */
   ULONG_PTR              Address;      /* Address of the breakpoint */
   BOOLEAN                Global;       /* Whether the breakpoint is global or local to a process */
   PEPROCESS              Process;      /* Owning process */
   PCHAR                  ConditionExpression;
   PVOID                  Condition;
   union {
      /* KdbBreakPointSoftware */
      UCHAR               SavedInstruction;
      /* KdbBreakPointHardware */
      struct {
         UCHAR            DebugReg : 2;
         UCHAR            Size : 3;
         KDB_ACCESS_TYPE  AccessType;
      } Hw;
   } Data;
} KDB_BREAKPOINT, *PKDB_BREAKPOINT;

typedef enum _KDB_ENTER_CONDITION
{
   KdbDoNotEnter,
   KdbEnterAlways,
   KdbEnterFromKmode,
   KdbEnterFromUmode
} KDB_ENTER_CONDITION;

typedef enum _KD_CONTINUE_TYPE
{
    kdContinue = 0,
    kdDoNotHandleException,
    kdHandleException
} KD_CONTINUE_TYPE;


/* GLOBALS *******************************************************************/

extern volatile PCHAR KdbInitFileBuffer;

extern PEPROCESS KdbCurrentProcess;
extern PETHREAD KdbCurrentThread;
extern LONG KdbLastBreakPointNr;
extern ULONG KdbNumSingleSteps;
extern BOOLEAN KdbSingleStepOver;
extern PKDB_KTRAP_FRAME KdbCurrentTrapFrame;


/* FUNCTIONS *****************************************************************/

/* from i386/i386-dis.c */

LONG
KdbpDisassemble(
   IN ULONG_PTR Address,
   IN ULONG IntelSyntax);

LONG
KdbpGetInstLength(
   IN ULONG_PTR Address);

/* from i386/kdb_help.S */

VOID NTAPI
KdbpStackSwitchAndCall(
   IN PVOID NewStack,
   IN VOID (*Function)(VOID));

/* from kdb_cli.c */

NTSTATUS
NTAPI
KdbInitialize(
    _In_ PKD_DISPATCH_TABLE DispatchTable,
    _In_ ULONG BootPhase);

BOOLEAN
NTAPI
KdbRegisterCliCallback(
    PVOID Callback,
    BOOLEAN Deregister);

NTSTATUS
KdbpCliInit(VOID);

VOID
KdbpCliMainLoop(
   IN BOOLEAN EnteredOnSingleStep);

VOID
KdbpCliInterpretInitFile(VOID);

VOID
KdbpCommandHistoryAppend(
    _In_ PCSTR Command);

PCSTR
KdbGetHistoryEntry(
    _Inout_ PLONG NextIndex,
    _In_ BOOLEAN Next);

VOID
KdbpPager(
    _In_ PCHAR Buffer,
    _In_ ULONG BufLength);

VOID
KdbpPrint(
    _In_ PSTR Format,
    _In_ ...);

VOID
KdbpPrintUnicodeString(
    _In_ PCUNICODE_STRING String);

BOOLEAN
NTAPI
KdbpGetHexNumber(
    IN PCHAR pszNum,
    OUT ULONG_PTR *pulValue);

/* from kdb_expr.c */

BOOLEAN
KdbpRpnEvaluateExpression(
   IN  PCHAR Expression,
   IN  PKDB_KTRAP_FRAME TrapFrame,
   OUT PULONGLONG Result,
   OUT PLONG ErrOffset  OPTIONAL,
   OUT PCHAR ErrMsg  OPTIONAL);

PVOID
KdbpRpnParseExpression(
   IN  PCHAR Expression,
   OUT PLONG ErrOffset  OPTIONAL,
   OUT PCHAR ErrMsg  OPTIONAL);

BOOLEAN
KdbpRpnEvaluateParsedExpression(
   IN  PVOID Expression,
   IN  PKDB_KTRAP_FRAME TrapFrame,
   OUT PULONGLONG Result,
   OUT PLONG ErrOffset  OPTIONAL,
   OUT PCHAR ErrMsg  OPTIONAL);

/* from kdb_symbols.c */

BOOLEAN
KdbpSymFindModule(
    IN PVOID Address  OPTIONAL,
    IN INT Index  OPTIONAL,
    OUT PLDR_DATA_TABLE_ENTRY* pLdrEntry);

BOOLEAN
KdbSymPrintAddress(
    IN PVOID Address,
    IN PCONTEXT Context);

VOID
KdbSymProcessSymbols(
    _Inout_ PLDR_DATA_TABLE_ENTRY LdrEntry,
    _In_ BOOLEAN Load);

BOOLEAN
KdbSymInit(
    _In_ ULONG BootPhase);

/* from kdb.c */

LONG
KdbpGetNextBreakPointNr(
   IN ULONG Start  OPTIONAL);

BOOLEAN
KdbpGetBreakPointInfo(
   IN  ULONG BreakPointNr,
   OUT ULONG_PTR *Address  OPTIONAL,
   OUT KDB_BREAKPOINT_TYPE *Type  OPTIONAL,
   OUT UCHAR *Size  OPTIONAL,
   OUT KDB_ACCESS_TYPE *AccessType  OPTIONAL,
   OUT UCHAR *DebugReg  OPTIONAL,
   OUT BOOLEAN *Enabled  OPTIONAL,
   OUT BOOLEAN *Global  OPTIONAL,
   OUT PEPROCESS *Process  OPTIONAL,
   OUT PCHAR *ConditionExpression  OPTIONAL);

NTSTATUS
KdbpInsertBreakPoint(
   IN  ULONG_PTR Address,
   IN  KDB_BREAKPOINT_TYPE Type,
   IN  UCHAR Size  OPTIONAL,
   IN  KDB_ACCESS_TYPE AccessType  OPTIONAL,
   IN  PCHAR ConditionExpression  OPTIONAL,
   IN  BOOLEAN Global,
   OUT PLONG BreakPointNr  OPTIONAL);

BOOLEAN
KdbpDeleteBreakPoint(
   IN LONG BreakPointNr  OPTIONAL,
   IN OUT PKDB_BREAKPOINT BreakPoint  OPTIONAL);

BOOLEAN
KdbpEnableBreakPoint(
   IN LONG BreakPointNr  OPTIONAL,
   IN OUT PKDB_BREAKPOINT BreakPoint  OPTIONAL);

BOOLEAN
KdbpDisableBreakPoint(
   IN LONG BreakPointNr  OPTIONAL,
   IN OUT PKDB_BREAKPOINT BreakPoint  OPTIONAL);

BOOLEAN
KdbpGetEnterCondition(
   IN LONG ExceptionNr,
   IN BOOLEAN FirstChance,
   OUT KDB_ENTER_CONDITION *Condition);

BOOLEAN
KdbpSetEnterCondition(
   IN LONG ExceptionNr,
   IN BOOLEAN FirstChance,
   IN KDB_ENTER_CONDITION Condition);

BOOLEAN
KdbpAttachToThread(
   PVOID ThreadId);

BOOLEAN
KdbpAttachToProcess(
   PVOID ProcessId);

VOID
KdbpGetCommandLineSettings(
    _In_ PCSTR p1);

KD_CONTINUE_TYPE
KdbEnterDebuggerException(IN PEXCEPTION_RECORD64 ExceptionRecord,
                          IN KPROCESSOR_MODE PreviousMode,
                          IN OUT PCONTEXT Context,
                          IN BOOLEAN FirstChance);

/* other functions */

BOOLEAN
NTAPI
KdpSafeReadMemory(
    IN ULONG_PTR Addr,
    IN LONG Len,
    OUT PVOID Value
);

BOOLEAN
NTAPI
KdpSafeWriteMemory(
    IN ULONG_PTR Addr,
    IN LONG Len,
    IN ULONGLONG Value
);

NTSTATUS
KdbpSafeReadMemory(OUT PVOID Dest,
                   IN PVOID Src,
                   IN ULONG Bytes);

NTSTATUS
KdbpSafeWriteMemory(OUT PVOID Dest,
                    IN PVOID Src,
                    IN ULONG Bytes);

VOID
KbdDisableMouse(VOID);

VOID
KbdEnableMouse(VOID);


/* From kdb_print.c */

VOID
KdbPrintString(
    _In_ const CSTRING* Output);

USHORT
KdbPromptString(
    _In_ const CSTRING* PromptString,
    _Inout_ PSTRING ResponseString);

VOID
KdbPutsN(
    _In_ PCCH String,
    _In_ USHORT Length);

VOID
KdbPuts(
    _In_ PCSTR String);

VOID
__cdecl
KdbPrintf(
    _In_ PCSTR Format,
    ...);

SIZE_T
KdbPrompt(
    _In_ PCSTR Prompt,
    _Out_ PCHAR Buffer,
    _In_ SIZE_T Size);
