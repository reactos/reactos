#ifndef NTOSKRNL_KDB_H
#define NTOSKRNL_KDB_H

/* INCLUDES ******************************************************************/

#define NTOS_MODE_KERNEL
#include <ntos.h>

#include <internal/ke.h>

/* DEFINES *******************************************************************/

#define TAG_KDBG        (('K' << 24) | ('D' << 16) | ('B' << 8) | 'G')

#ifndef RTL_NUMBER_OF
# define RTL_NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))
#endif


/* TYPES *********************************************************************/

/* from kdb.c */
typedef struct _KDB_KTRAP_FRAME
{
   KTRAP_FRAME  Tf;
   ULONG        Cr0;
   ULONG        Cr1; /* reserved/unused */
   ULONG        Cr2;
   ULONG        Cr3;
   ULONG        Cr4;
} KDB_KTRAP_FRAME, *PKDB_KTRAP_FRAME;

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


/* from kdb_symbols.c */
typedef struct _KDB_MODULE_INFO
{
    WCHAR        Name[256];
    ULONG_PTR    Base;
    ULONG        Size;
    PROSSYM_INFO RosSymInfo;
} KDB_MODULE_INFO, *PKDB_MODULE_INFO;


/* FUNCTIONS *****************************************************************/

/* from i386/i386-dis.c */

LONG
KdbpDisassemble(
   IN ULONG Address,
   IN ULONG IntelSyntax);
   
LONG
KdbpGetInstLength(
   IN ULONG Address);

/* from i386/kdb_help.S */

STDCALL VOID
KdbpStackSwitchAndCall(
   IN PVOID NewStack,
   IN VOID (*Function)(VOID));

/* from kdb_cli.c */

extern PCHAR KdbInitFileBuffer;

VOID
KdbpCliInit();

VOID
KdbpCliMainLoop(
   IN BOOLEAN EnteredOnSingleStep);

VOID
KdbpCliModuleLoaded(
   IN PUNICODE_STRING Name);

VOID
KdbpCliInterpretInitFile();

VOID
KdbpPrint(
   IN PCHAR Format,
   IN ...  OPTIONAL);

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
KdbpSymFindModuleByAddress(IN PVOID Address,
                           OUT PKDB_MODULE_INFO pInfo);

BOOLEAN
KdbpSymFindModuleByName(IN LPCWSTR Name,
                        OUT PKDB_MODULE_INFO pInfo);

BOOLEAN
KdbpSymFindModuleByIndex(IN INT Index,
                         OUT PKDB_MODULE_INFO pInfo);

BOOLEAN 
KdbSymPrintAddress(IN PVOID Address);

NTSTATUS
KdbSymGetAddressInformation(IN PROSSYM_INFO  RosSymInfo,
                            IN ULONG_PTR  RelativeAddress,
                            OUT PULONG LineNumber  OPTIONAL,
                            OUT PCH FileName  OPTIONAL,
                            OUT PCH FunctionName  OPTIONAL);

/* from kdb.c */

extern PEPROCESS KdbCurrentProcess;
extern PETHREAD KdbCurrentThread;
extern LONG KdbLastBreakPointNr;
extern ULONG KdbNumSingleSteps;
extern BOOLEAN KdbSingleStepOver;
extern PKDB_KTRAP_FRAME KdbCurrentTrapFrame;

VOID
KdbInit();

VOID
KdbModuleLoaded(
   IN PUNICODE_STRING Name);

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
   OUT PULONG BreakPointNumber  OPTIONAL);
   
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

/* from profile.c */

VOID
KdbInitProfiling();
VOID
KdbInitProfiling2();
VOID
KdbDisableProfiling();
VOID
KdbEnableProfiling();
VOID
KdbProfileInterrupt(ULONG_PTR Eip);

/* other functions */

#define KdbpSafeReadMemory(dst, src, size)   MmSafeCopyFromUser(dst, src, size)
#define KdbpSafeWriteMemory(dst, src, size)  MmSafeCopyToUser(dst, src, size)

#define KdbpGetCharKeyboard(ScanCode) KdbpTryGetCharKeyboard(ScanCode, 0)
CHAR
KdbpTryGetCharKeyboard(PULONG ScanCode, UINT Retry);

#define KdbpGetCharSerial()  KdbpTryGetCharSerial(0)
CHAR
KdbpTryGetCharSerial(UINT Retry);

VOID
KdbEnter(VOID);
VOID
DbgRDebugInit(VOID);
VOID
DbgShowFiles(VOID);
VOID
DbgEnableFile(PCH Filename);
VOID
DbgDisableFile(PCH Filename);


#endif /* NTOSKRNL_KDB_H */

