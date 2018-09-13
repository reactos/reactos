/*++



--*/


#define _tsizeof(str) (sizeof(str)/sizeof(TCHAR))

#define DONT_NOTIFY     ((METHOD*)0)

#define NO_ACTION       ((ACVECTOR)0)

#define INVALID         (-1L)

#define NO_SUBCLASS     ((DWORD_PTR)(-1L))

//
// This should really be defined by shapi or osdebug or something
//
#define CMODULEDEMARCATOR _T('|')

// WaitForDebugEvent() timeout, milliseconds
#define WAITFORDEBUG_MS (50L)

// Wait for loader breakpoint timeout sec * ticks/sec
#define LDRBP_MAXTICKS  (60L * 1000L/WAITFORDEBUG_MS)

#define SetFile()

extern DBF *lpdbf;


#ifndef KERNEL

#include <crash.h>

extern BOOL CrashDump;

#endif


//
// Definitions for backward compatibility handling
//

#if defined(TARGET_i386)

#define CONTEXT_SIZE_PRE_NT5 FIELD_OFFSET(CONTEXT, ExtendedRegisters)
#define CONTEXT_SIZE_NT5_VERSION 1732

#endif

//
// Macros for common registers
//

#if defined(TARGET_i386)

#define IP_TYPE             ULONG
#define PC(x)               SE32To64( ((x)->context.Eip) )
#define cPC(x)              SE32To64( ((x)->Eip) )
#define STACK_POINTER(x)    SE32To64( ((x)->context.Esp) )
#define FRAME_POINTER(x)    SE32To64( ((x)->context.Ebp) )

#define Set_PC(x, y)                (((x)->context.Eip) = (IP_TYPE) (y))
#define Set_cPC(x, y)               (((x)->Eip)         = (IP_TYPE) (y))
#define Set_STACK_POINTER(x, y)     (((x)->context.Esp) = (IP_TYPE) (y))
#define Set_FRAME_POINTER(x, y)     (((x)->context.Ebp) = (IP_TYPE) (y))

#define PcSegOfHthdx(x)     ((SEGMENT) (x->context.SegCs))
#define SsSegOfHthdx(x)     ((SEGMENT) (x->context.SegSs))


#define MPT_CURRENT mptix86

#elif defined (TARGET_ALPHA) || defined (TARGET_AXP64)

#include "alphaops.h"
#include "ctxptrs.h"

#define IP_TYPE             ULONGLONG
#define PC(x)               ((x)->context.Fir)
#define cPC(x)              ((x)->Fir)
#define STACK_POINTER(x)    ((x)->context.IntSp)
#define FRAME_POINTER(x)    ((x)->context.IntSp)

#define Set_PC(x, y)                (PC(x)            = (IP_TYPE) (y))
#define Set_cPC(x, y)               (cPC(x)           = (IP_TYPE) (y))
#define Set_STACK_POINTER(x, y)     (STACK_POINTER(x) = (IP_TYPE) (y))
#define Set_FRAME_POINTER(x, y)     (FRAME_POINTER(x) = (IP_TYPE) (y))

#define PcSegOfHthdx(x)         (0)
#define SsSegOfHthdx(x)         (0)

#define MPT_CURRENT mptdaxp

#elif defined (TARGET_IA64)

#define IP_TYPE        ULONGLONG
#define PC(x)          ((x)->context.StIIP | ((((x)->context.StIPSR >> PSR_RI) & 0x3) << 2))
#define cPC(x)         ((x)->StIIP | ((((x)->StIPSR >> PSR_RI) & 0x3) << 2))
#define STACK_POINTER(x)  ((x)->context.IntSp)
#define FRAME_POINTER(x)  ((x)->context.IntSp)

#define Set_PC(x, y)                (PC(x)            = (IP_TYPE) (y))
#define Set_cPC(x, y)               (cPC(x)           = (IP_TYPE) (y))
#define Set_STACK_POINTER(x, y)     (STACK_POINTER(x) = (IP_TYPE) (y))
#define Set_FRAME_POINTER(x, y)     (FRAME_POINTER(x) = (IP_TYPE) (y))

#define PcSegOfHthdx(x)   (0)
#define SsSegOfHthdx(x)   (0)
#define TF_BIT_MASK ((ULONGLONG)0x1 << PSR_SS)


#include <ia64inst.h>


#define MPT_CURRENT mptia64

#else

#error "Undefined processor"

#endif


//
// Breakpoint stuff
//

#if defined(TARGET_i386)

typedef BYTE BP_UNIT;
#define BP_OPCODE   0xCC
#define DELAYED_BRANCH_SLOT_SIZE    0

#define MAX_INSTRUCTION_SIZE    20

#define HAS_DEBUG_REGS
// #undef NO_TRACE_FLAG

#define NUMBER_OF_DEBUG_REGISTERS   4
#define DEBUG_REG_DATA_SIZES        { 1, 2, 4 }
#define MAX_DEBUG_REG_DATA_SIZE     4
#define DEBUG_REG_LENGTH_MASKS      {   \
                            0xffffffff,          \
                            0,          \
                            1,          \
                            0Xffffffff, \
                            3           \
                            }


#define TF_BIT_MASK 0x00000100  /* This is the right bit map for */
/* the 286, make sure its correct */
/* for the 386. */

#elif defined (TARGET_ALPHA) || defined (TARGET_AXP64)

typedef DWORD BP_UNIT;
#define BP_OPCODE   0x80L
#define DELAYED_BRANCH_SLOT_SIZE    0

#define MAX_INSTRUCTION_SIZE    4

// #undef HAS_DEBUG_REGS
#define NO_TRACE_FLAG

#elif defined(TARGET_IA64)

typedef ULONGLONG BP_UNIT;
#define KDP_BREAKPOINT_ALIGN 0x3
#define BP_OPCODE  (BREAK_INSTR | (ULONGLONG)(DEBUG_STOP_BREAKPOINT << 6))
#define KERNEL_BREAKIN_OPCODE  (BREAK_INSTR | (ULONGLONG)(BREAKIN_BREAKPOINT << 6))
#define DELAYED_BRANCH_SLOT_SIZE    0
#define MAX_INSTRUCTION_SIZE    4  //v-vadimp ?

// HAS_DEBUG_REGS must be defined to enable ExtendedContext support
#define HAS_DEBUG_REGS
//#define NO_TRACE_FLAG  //v-vadimp - there's some code for setting the SS flag (PSR.ss), rem'-ing this out should enable it

// IA64 has 8 DBR's and 8 IBR's that can emulate up to 4 full feature BR's
#define NUMBER_OF_DEBUG_REGISTERS   4
#define DEBUG_REG_DATA_SIZES        { 1, 2, 4, 8 }
#define MAX_DEBUG_REG_DATA_SIZE     8
#define DEBUG_REG_LENGTH_MASKS      {       \
                                    (DWORDLONG)0,                \
                                    (DWORDLONG)0xff,             \
                                    (DWORDLONG)0xffff,           \
                                    (DWORDLONG)0,                \
                                    (DWORDLONG)0xffffffff,       \
                                    (DWORDLONG)0,                \
                                    (DWORDLONG)0,                \
                                    (DWORDLONG)0,                \
                                    (DWORDLONG)0xffffffffffffff, \
                                    }

#else

#error "Unknown target CPU"

#endif

//
// constant from windbgkd.h:
//
#define MAX_KD_BPS  BREAKPOINT_TABLE_SIZE
//
// machine-dependent BP instruction size
//
#define BP_SIZE     sizeof(BP_UNIT)

#ifdef HAS_DEBUG_REGS
typedef struct DEBUGREG {
    DWORDLONG   DataAddr;       //  Data Address
    DWORD       DataSize;       //  Data Size
    BPTP        BpType;         //  read, write, execute, etc
    BOOL        InUse;          //  In use
    DWORD       ReferenceCount;
} DEBUGREG;
typedef DEBUGREG *PDEBUGREG;

extern DWORD DebugRegDataSizes[];

#endif



#define EXADDR(pde)    ((pde)->u.Exception.ExceptionRecord.ExceptionAddress)

#define AddrFromHthdx(paddr, hthd)          \
        AddrInit(paddr,                     \
                 0,                         \
                 PcSegOfHthdx(hthd),        \
                 PC(hthd),                  \
                 (BOOL)hthd->fAddrIsFlat,   \
                 (BOOL)hthd->fAddrOff32,    \
                 FALSE,                     \
                 (BOOL)hthd->fAddrIsReal)

typedef struct _CALLSTRUCT *PCALLSTRUCT;

/*
 * These are "debug events" which are generated internally by the DM.
 * They are either remappings of certain exceptions or events which
 * do not correspond directly to a system-generated event or exception.
 */

enum    {
    BREAKPOINT_DEBUG_EVENT=(RIP_EVENT+1),
    CHECK_BREAKPOINT_DEBUG_EVENT,
    SEGMENT_LOAD_DEBUG_EVENT,
    DESTROY_PROCESS_DEBUG_EVENT,
    DESTROY_THREAD_DEBUG_EVENT,
    ATTACH_DEADLOCK_DEBUG_EVENT,
    ATTACH_EXITED_DEBUG_EVENT,
    ENTRYPOINT_DEBUG_EVENT,
    LOAD_COMPLETE_DEBUG_EVENT,
    INPUT_DEBUG_STRING_EVENT,
    MESSAGE_DEBUG_EVENT,
    MESSAGE_SEND_DEBUG_EVENT,
    FUNC_EXIT_EVENT,
    OLE_DEBUG_EVENT,
    FIBER_DEBUG_EVENT,
    GENERIC_DEBUG_EVENT,
    BOGUS_WIN95_SINGLESTEP_EVENT,
    MAX_EVENT_CODE
  };

/*
 * This is the set of legal return values from IsCall.  The function of
 *      that routine is to analyze the instruction and determine if the
 *      debugger can simply step over it.
 */

typedef enum    {
    INSTR_TRACE_BIT,            /* Use the trace bit stepping or emulation
                                        thereof */
    INSTR_BREAKPOINT,           /* This is a breakpoint instruction     */
    INSTR_CANNOT_TRACE,         /* Can not trace this instruction       */
    INSTR_SOFT_INTERRUPT,       /* This is an interrupt opcode          */
    INSTR_IS_CALL,              /* This is a call instruction           */
    INSTR_CANNOT_STEP,                  /* In Win95 system code                                 */
} INSTR_TYPES;

typedef enum {
    THUNK_NONE = 0,
    THUNK_USER,
    THUNK_SYSTEM,
} DM32ThunkTypes;

typedef enum {
    RETURN_NONE = 0,
    RETURN_USER,
    RETURN_SYSTEM,
} DM32ReturnTypes;

typedef enum {
    ps_root       = 0x0001,     /* This is the root process, do not send a */
                                /* dbcDeleteProc when this is continued */
                                /* after a dbcProcTerm. */
    ps_preStart   = 0x0002,     /* Process is expecting loader BP */
    ps_preEntry   = 0x0004,     /* Process is expecting Entry BP */
    ps_dead       = 0x0010,     /* This process is dead. */
    ps_deadThread = 0x0020,     /* This process owns dead threads */
    ps_exited     = 0x0040,     /* We have notified the debugger that this */
                                /* process has exited. */
    ps_destroyed  = 0x0080,     /* This process has been destroyed (deleted) */
    ps_killed     = 0x0100,     /* This process is being killed */
    ps_connect    = 0x0200
} DMPSTATE;

typedef void (*VECTOR)();

typedef struct  _EXCEPTION_LIST {
    struct _EXCEPTION_LIST *next;
    EXCEPTION_DESCRIPTION  excp;
} EXCEPTION_LIST, *LPEXCEPTION_LIST;

typedef struct _DLLLOAD_ITEM {
    BOOL        fValidDll;         // is this entry filled?
    DWORDLONG   offBaseOfImage;    // offset for base of Image
    DWORD       cbImage;           // size of image in bytes
    LPTSTR      szDllName;         // dll name

    PIMAGE_SECTION_HEADER Sections;          // pointer to section headers
    DWORD                 NumberOfSections;  // number of section headers

#ifndef KERNEL

    BOOL        fReal;
    BOOL        fWow;
    OFFSET      offTlsIndex;    // The offset of the TLS index for the DLL
    // kentf The following comment is what I found in the sources which I
    //       hacked the OLE stuff from.
                                // ptr (in debuggee's memory space) to this
                                // DLL's 1-byte boolean flag indicating whether
                                // OLE RPC debugging is enabled.  If this DLL
                                // does not support OLE RPC, then this field
                                // will be zero.
    // However, the code in dmole.c uses this as a pointer to a function which
    // takes two args, the first of which is the above described flag, and the
    // second of which is zero.
    LPVOID      lpvOleRpc;

    BOOL        fContainsOle;   // does this DLL contain any OLE RPC segments?

#else

    DWORD                 TimeStamp;         //
    DWORD                 CheckSum;          //
    WORD                  SegCs;             //
    WORD                  SegDs;             //
    PIMAGE_SECTION_HEADER sec;               //

#endif

} DLLLOAD_ITEM, * PDLLLOAD_ITEM;

#if defined(INTERNAL)
typedef struct _DLL_DEFER_LIST {
    struct _DLL_DEFER_LIST  * next;
    LOAD_DLL_DEBUG_INFO       LoadDll;
} DLL_DEFER_LIST, *PDLL_DEFER_LIST;
#endif

/*
 * CWPI is the number of Wndproc-invoking functions that exist:
 *              SendMessage
 *              SendMessageTimeout
 *              SendMessageCallback
 *              SendNotifyMessage
 *              SendDlgItemMessage
 *              DispatchMessage
 *              CallWindowProc
 * times two (A version and W version)
 */

#define CWPI    14

//
// Misc forward declarations for OLE types
//

typedef struct _OLERG *POLERG;
typedef enum _ORPCKEYSTATE ORPCKEYSTATE;
typedef struct _OLERET *POLERET;


//
//  When the user requests that we begin orpc debugging, we set the
//  OrpcDebugging variable in the process structure to be
//  ORPC_START_DEBUGGING.  The next appropiate time -- during a step, for
//  example -- we check the value of the OrpcDebgging and if it's
//  ORPC_START_DEBUGGING, we call the trojan and set OrpcDebugging to to
//  ORPC_DEBUGGING.  When we are ORPC_DEBUGGING and the user requests to
//  stop orpc debugging we set the OrpcDebugging value to ORPC_STOP_DEBUGGING
//  and at the next appropiate time, call the trojan to stop debugging.
//
//  We cannot call the trojan immediately because this fails on W95.
//

typedef enum _ORPC_DEBUGGING_STATE
{
    ORPC_NOT_DEBUGGING      = 0,
    ORPC_START_DEBUGGING,
    ORPC_DEBUGGING,
    ORPC_STOP_DEBUGGING
} ORPC_DEBUGGING_STATE;

typedef struct _HFBRX {
    // linked list
    struct _HFBRX   *next;
    LPVOID  fbrstrt;
    LPVOID  fbrcntx;

} HFBRXSTRUCT,*HFBRX;

typedef struct  _HPRCX {
    // linked lists
    struct _HPRCX   *next;
    struct _HTHDX   *hthdChild;
    struct _HFBRX   *FbrLst;
    PID             pid;            // OS provided process ID
    HANDLE          rwHand;         // OS provided Process handle
    BOOL            CloseProcessHandle; // If we have a private
                                    // handle to this process, close it.
                                    // Otherwise, it belongs to smss.
    DWORD           dwExitCode;     // Process exit status

    HPID            hpid;           // binding to EM object

    DMPSTATE        pstate;         // DM state model
    BOOL            f16bit;         // CreateProcess EXE was 16 bit
    EXCEPTION_LIST *exceptionList;  // list of exceptions to silently
                                    // continue unhandled
    int             cLdrBPWait;     // timeout counter while waiting for ldr BP
    HANDLE          hExitEvent;     // synchronization object for
                                    // process termination
    PDLLLOAD_ITEM   rgDllList;      // module list
    int             cDllList;       // item count for module list

    HANDLE          hEventCreateThread;  // Sync object for thread creation

#ifndef KERNEL
    BOOL            fUseFbrs;       // Use fiber context or thread context
    PVOID           pFbrCntx;       // Pointer to a fiber context to display
                                    // NULL = use thread context
    DWORD_PTR        dwKernel32Base; // lpBaseOfDll for kernel32.

    DWORD           colerg;         // number of OLE ranges in *rgolerg
    POLERG          rgolerg;        // array of OLERGs: sorted list of all
                                    // addresses in this process (including
                                    // its DLLs) which are special OLE
                                    // segments.  May be
                                    // NULL if colerg is zero.

    ORPC_DEBUGGING_STATE    OrpcDebugging;  // orpc debugging state (see above)
    ORPCKEYSTATE            orpcKeyState;

    DWORD_PTR        rgwpoff[CWPI];  // addrs of Wndproc-invoking functions

    HLLI            llnlg;          // non-local goto

    UOFFSET         PebAddress;     // NT Process Environment Block

#else

    BOOL            fRomImage;      // rom image
    BOOL            fRelocatable;   // relocatable code

#endif

#if defined(TARGET_i386)
    SEGMENT         segCode;
#endif

#if defined(INTERNAL)
    // during process startup, dll name resolution may be
    // deferred until the loader BP.  Once the process is
    // fully initialized, this deferral is no longer allowed.
    BOOL            fNameRequired;
    PDLL_DEFER_LIST pDllDeferList;
#endif

} HPRCXSTRUCT, *HPRCX;

#define hprcxNull       ((HPRCX) 0)

typedef enum {
    //ts_preStart =0x1000,        /* Before the starting point of the thread */
                                /* from this state a registers and trace   */
                                /* are dealt with specially                */
    ts_running  =     1,        /* Execution is proceeding on the thead    */
    ts_stopped  =     2,        /* An event has stopped execution          */
    ts_frozen   = 0x010,        /* Debugger froze thread.                  */
    ts_dead     = 0x020,        /* Thread is dead.                         */
    ts_destroyed =0x040,        /* Thread is destroyed (deleted)           */
    ts_first    = 0x100,        /* Thread is at first chance exception     */
    ts_second   = 0x200,        /* Thread is at second chance exception    */
    ts_rip      = 0x400,        /* Thread is in RIP state                  */
    ts_stepping = 0x800,        /*                                         */
    ts_funceval = 0x40000000    /* Thread is being used for function call  */
} TSTATEX;

typedef struct  _WTNODE {
    struct _WTNODE      *caller;      // caller's wtnode
    struct _WTNODE      *callee;      // current function called by this function
    DWORDLONG           offset;       // address of this function
    DWORDLONG           sp;           // SP for this frame
    int                 icnt;         // number of instructions executed
    int                 scnt;         // subordinate count
    int                 lex;          // lexical level of this function
    LPSTR               fname;        // function name
} WTNODE, *LPWTNODE;


typedef struct  _HTHDX {

    //
    // list of all threads
    //
    struct  _HTHDX    *nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse;

    //
    // list of threads in one process
    //
    struct  _HTHDX    *nextSibling;

    HPRCX             hprc;         // OSDebug handle for process
    HTID              htid;         // OSDebug handle for thread
    TID               tid;          // OS thread ID
    HANDLE            rwHand;       // OS handle to thread
    ULONG64           lpStartAddress;// Thread start address
    CONTEXT           context;      // Thread context
    LPVOID            atBP;
    TSTATEX           tstate;
    BOOL              fExceptionHandled;
    DWORDLONG         stackRA;
    DWORDLONG         stackBase;
    int               cFuncEval;
    DWORD             dwExitCode;
    DWORDLONG         offTeb;

    BOOL              fContextDirty;// has the context changed?
    BOOL              fContextStale;// does the context need to be refreshed?

    BOOL              fAddrIsFlat;  // Is this address segmented?
    BOOL              fAddrIsReal;  // Is this address in real mode?
    BOOL              fAddrOff32;   // Is the offset of this addres 32 bits?
    BOOL              fDontStepOff; //

    BOOL              fWowEvent;    // Was the last event WOW?

    ADDR              addrIsCall;
    int               iInstrIsCall;

    EXCEPTION_RECORD64 ExceptionRecord;

    BOOL              fIsCallDone;
    BOOL              fDisplayReturnValues;
    BOOL              fStopOnNLG;
    BOOL              fReturning;

    ADDR              addrFrom;
    ADDR              addrStack;

    WTNODE            wthead;       // root of the call tree for a wt command
    LPWTNODE          wtcurr;       // current wtnode
    DWORD             wtmode;       // wt command executing?

    LIST_ENTRY        WalkList;         //  Walks associated with thread

#ifdef HAS_DEBUG_REGS
    DEBUGREG          DebugRegs[NUMBER_OF_DEBUG_REGISTERS];
#endif

#ifndef KERNEL
    CRASH_THREAD      CrashThread;      // State info from crashdump
    PCALLSTRUCT       pcs;              // used for DM initiated function calling
    POLERET           poleret;
#endif // !KERNEL

} HTHDXSTRUCT, *HTHDX;

#define PassingExceptionForThisThread(h) \
    (((h)->tstate & (ts_first | ts_second)) && !(h)->fExceptionHandled)

typedef void (*ACVECTOR)(DEBUG_EVENT64*, HTHDX, DWORDLONG, DWORDLONG);
typedef void (*DDVECTOR)(DEBUG_EVENT64*, HTHDX);

#define hthdxNull ((HTHDX) NULL)

#if defined(TARGET_IA64)
//
// these are used in the flags field of a BREAKPOINT (IA64-specific)
//
typedef enum {
      BREAKPOINT_IA64_MASK = 0x000f0000,
      BREAKPOINT_IA64_MODE = 0x00010000,   // IA64 EM mode
      BREAKPOINT_IA64_MOVL = 0x00020000,   // displaced instruction is MOVL
} BPFLG;
#endif

typedef struct _BREAKPOINT {
    struct _BREAKPOINT *next;
    HPRCX      hprc;        // The process the BP belongs to
    HTHDX      hthd;        // The thread the BP belongs to
    BPTP       bpType;      // OSDebug BP type
    BPNS       bpNotify;    // OSDebug notify type

    ADDR       addr;        // The address of the Breakpoint
    BP_UNIT    instr1;      // The displaced instruction
    HANDLE     hWalk;       // walk list handle if it is a watchpoint

    BYTE       instances;   // The # of instances that exist
    HPID       id;          // Id supplied by the EM
    BOOL       isStep;      // Single step flag
    DWORD      hBreakPoint; // kernel debugger breakpoint handle
#if defined(TARGET_IA64)
        BPFLG      flags;       // IA64 mode mask
#endif
} BREAKPOINT;
typedef BREAKPOINT *PBREAKPOINT;

//
// these are magic values used in the hthd->atBP field.
//

#define EMBEDDED_BP     ((PBREAKPOINT)(-1))

//
// These are used in the id field of a BREAKPOINT.
//
#define ENTRY_BP        ((ULONG) -2)
#define ASYNC_STOP_BP   ((ULONG) -3)

extern  BREAKPOINT      masterBP , *bpList;

typedef struct _METHOD {
    ACVECTOR notifyFunction; /* The notification function to call */
    DWORDLONG  lparam;        /* The parameter to pass to it */
    void    *lparam2;       /* Extra pointer in case the method */
    /* needs to be freed afterwards */
} METHOD;
typedef METHOD *PMETHOD;

typedef struct _EXPECTED_EVENT {
    struct   _EXPECTED_EVENT  *next;
    HPRCX    hprc;
    HTHDX    hthd;
    DWORD    eventCode;
    DWORD_PTR  subClass;
    METHOD*  notifier;
    ACVECTOR action;
    BOOL     fPersistent;
    DWORDLONG  lparam;
} EXPECTED_EVENT;
typedef EXPECTED_EVENT *PEXPECTED_EVENT;


typedef VOID    (*STEPPER)(HTHDX,METHOD*,BOOL, BOOL);

typedef DWORD   (*CDVECTOR)(HPRCX,HTHDX,LPDBB);

typedef struct {
    DMF         dmf;
    CDVECTOR    function;
    WORD        type;
} CMD_DESC;


enum {
    BLOCKING,
    NON_BLOCKING,
    REPLY
};


/*
 *      Setup for a CreateProcess to occur
 */

typedef struct _SPAWN_STRUCT {
    BOOL                fSpawn;
    HANDLE              hEventApiDone;

    BOOL                fReturn;    // return from API
    DWORD               dwError;

    char *              szAppName;  // args to API etc
    char *              szArgs;
    char *              pszCurrentDirectory; // directory to spawn process.
    DWORD               fdwCreate;
    BOOL                fInheritHandles;
    STARTUPINFO         si;
} SPAWN_STRUCT, *PSPAWN_STRUCT;

/*
 *      Setup for a DebugActiveProcess to occur
 */

typedef struct _DEBUG_ACTIVE_STRUCT {
    volatile BOOL fAttach;          // tell DmPoll to act
    HANDLE        hEventApiDone;    // signal shell that API finished
    HANDLE        hEventReady;      // clear until finished loading

    BOOL          fReturn;          // API return value
    DWORD         dwError;          // GetLastError() value

    DWORD         dwProcessId;      // pid to debug
    HANDLE        hEventGo;         // signal after hitting ldr BP
    HANDLE        hProcess;         // handle for waiting on
} DEBUG_ACTIVE_STRUCT, *PDEBUG_ACTIVE_STRUCT;

//
// packet for starting WT (Watch Trace)
//
typedef struct _WT_STRUCT {
    BOOL          fWt;
    DWORD         dwType;
    HTHDX         hthd;
} WT_STRUCT, *LPWT_STRUCT;

//
// Packet for killing a process
//
typedef struct _KILLSTRUCT {
    struct _KILLSTRUCT * next;
    HPRCX                hprc;
} KILLSTRUCT, *PKILLSTRUCT;

//
// usermode reload notification packet
//

typedef struct _RELOAD_STRUCT {
    BOOL    Flag;
    HTHDX   Hthd;
    PTCHAR  String;
} RELOAD_STRUCT, *PRELOAD_STRUCT;

extern  BOOL    StartDmPollThread(void);
extern  BOOL    StartCrashPollThread(void);


extern BOOL SearchPathSet;
extern char SearchPathString[];

//
//  Single stepping stuff
//
typedef struct _BRANCH_NODE {
    BOOL    TargetKnown;     //  Know target address
    BOOL    IsCall;          //  Is a call instruction
    ADDR    Addr;            //  Branch instruction address
    ADDR    Target;          //  Target address
} BRANCH_NODE;

#pragma warning( disable: 4200)

typedef struct _BRANCH_LIST {
    ADDR        AddrStart;      //  Start of range
    ADDR        AddrEnd;        //  End of range
    DWORD       Count;          //  Count of branch nodes
    BRANCH_NODE BranchNode[0];  //  List of branch nodes
} BRANCH_LIST;

#pragma warning( default: 4200 )

DWORD
BranchUnassemble(
    HTHDX   hthd,
    void   *Memory,
    DWORD   Size,
    ADDR   *Addr,
    BOOL   *IsBranch,
    BOOL   *TargetKnown,
    BOOL   *IsCall,
    BOOL   *IsTable,
    ADDR   *Target
    );


//
//  Structure for doing range stepping
//
typedef struct _RANGESTRUCT {
    HTHDX        hthd;          //  thread
    BOOL         fStepOver;     //  Step over flag
    BOOL         fStopOnBP;     //  Stop on BP flag
    METHOD       *Method;       //  Method
    DWORD        BpCount;       //  Count of temporary breakpoints
    ADDR         *BpAddrs;      //  List of breakpoint addresses
    BREAKPOINT   **BpList;      //  List of breakpoints
    BRANCH_LIST  *BranchList;   //  branch list
    ADDR         PrevAddr;      //  For single stepping
    BOOL         fSingleStep;   //  For single stepping
    ADDR         TmpAddr;       //  For single stepping
    BOOL         fInCall;       //  For single stepping
    BREAKPOINT   *TmpBp;        //  For single stepping
} RANGESTRUCT;

BOOL
SmartRangeStep(
    HTHDX       hthd,
    DWORDLONG   offStart,
    DWORDLONG   offEnd,
    BOOL        fStopOnBP,
    BOOL        fStepOver
    );

VOID
MethodSmartRangeStep(
    DEBUG_EVENT64* pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );


typedef struct  _RANGESTEP {
    HTHDX       hthd;           // The thread's structure
    SEGMENT     segCur;         // Segment to do range stepping in
    DWORDLONG   addrStart;      // starting address of range step
    DWORDLONG   addrEnd;        // ending address of range step
    SEGMENT     SavedSeg;       // Save locations for thunk stepping
    DWORDLONG   SavedAddrStart; //  "       "
    DWORDLONG   SavedAddrEnd;   //  "       "
    STEPPER     stepFunction;   // The step function to call
    METHOD      *method;        // The method to handle this event
    BREAKPOINT  *safetyBP;      // Safety BP
    FUNCTION_INFORMATION CallSiteInfo;
    BOOL        fIsCall;        // just traced a call instruction?
    BOOL        fIsRet;         // just traced a ret?
    BOOL        fInThunk;       // stepping in a thunk?
    BOOL        fSkipProlog;    // step past prolog on function entry
    BOOL        fGetReturnValue;// Getting a return value.
} RANGESTEP;
typedef RANGESTEP * PRANGESTEP;

extern DEBUG_EVENT64 FuncExitEvent;
extern HINSTANCE hInstance; // The DM DLLs hInstance

VOID
RangeStep(
    HTHDX       hthd,
    DWORDLONG   offStart,
    DWORDLONG   offEnd,
    BOOL        fStopOnBP,
    BOOL        fstepOver
    );

VOID
MethodRangeStep(
    DEBUG_EVENT64* pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

VOID
IsCall(
    HTHDX hthd,
    LPADDR lpAddr,
    LPINT lpFlag,
    BOOL fStepOver
    );

VOID
DecrementIP(
    HTHDX hthd
    );

VOID
IncrementIP(
    HTHDX hthd
    );

BOOL
IsRet(
    HTHDX hthd,
    LPADDR addr
    );

VOID
ContinueFromBP(
    HTHDX hthd,
    PBREAKPOINT pbp
    );

#if defined(TARGET_IA64)
#define CB_THUNK_MAX    64  //4 bundles
#else
#define CB_THUNK_MAX    32
#endif

BOOL
IsThunk (
    HTHDX       hthd,
    DWORDLONG   uoffset,
    LPINT       lpfThunkType,
    DWORDLONG * lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
FIsDirectJump(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    DWORDLONG   uoffset,
    DWORDLONG * lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
FIsIndirectJump(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    DWORDLONG   uoffset,
    DWORDLONG * lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
FIsVCallThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    DWORDLONG   uoffset,
    DWORDLONG * lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
FIsVTDispAdjustorThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    DWORDLONG   uoffset,
    DWORDLONG * lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
FIsAdjustorThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    DWORDLONG   uoffset,
    DWORDLONG * lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    );

BOOL
GetPMFDest(
    HTHDX hthd,
    DWORDLONG uThis,
    DWORDLONG uPMF,
    DWORDLONG *lpuOffDest
    );

BOOL
SetupSingleStep(
    HTHDX hthd,
    BOOL DoContinue
    );

BOOL
SetupReturnStep(
    HTHDX hthd,
    BOOL  DoContinue,
    LPADDR lpaddr,
    LPADDR addrStack
    );

DWORD
GetCanStep (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPCANSTEP lpCanStep
    );

DWORDLONG
GetEndOfRange (
    HPRCX   hprc,
    HTHDX   hthd,
    DWORDLONG Addr
    );

VOID
SingleStep(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval
    );

VOID
SingleStepEx(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval,
    BOOL fDoContinue
    );

VOID
ReturnStep(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval,
    LPADDR addrRA,
    LPADDR addrStack
    );

VOID
ReturnStepEx(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval,
    LPADDR addrRA,
    LPADDR addrStack,
    BOOL fDoContinue
    );

VOID
StepOver(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval
    );

VOID
MoveIPToException(
    HTHDX hthd,
    LPDEBUG_EVENT64 pde
    );

void
MethodContinueSS(
    DEBUG_EVENT64 *pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );


BOOL
DecodeSingleStepEvent(
    HTHDX hthd,
    DEBUG_EVENT64 *de,
    PDWORD eventCode,
    PDWORD_PTR subClass
    );

VOID
WtRangeStep(
    HTHDX hthd
    );

VOID
WtMethodRangeStep(
    DEBUG_EVENT64* pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

BOOL
GetWndProcMessage(
    HTHDX   hthd,
    UINT*   message
    );

//
// Function calling, for internal use
//

typedef struct _CALLSTRUCT {
    PBREAKPOINT pbp;
    LPVOID      atBP;
    CONTEXT     context;
    ACVECTOR    Action;
    LPARAM      lparam;
    BOOL        HasReturnValue;
} CALLSTRUCT, *PCALLSTRUCT;

BOOL
WINAPIV
CallFunction(
    HTHDX       hthd,
    ACVECTOR    Action,
    LPARAM      lparam,
    BOOL        HasReturnValue,
    DWORDLONG     Function,
    int         cArgs,
    ...
    );

//
// This function is machine-specific
//
VOID
vCallFunctionHelper(
    HTHDX hthd,
    DWORDLONG lpFunction,
    int cArgs,
    va_list vargs
    );

//
// This function is machine-specific
//

DWORDLONG
GetFunctionResult(
    PCALLSTRUCT pcs
    );


//
// Win95 support
//

BOOL IsInSystemDll ( DWORDLONG uoffDest );
void SendDBCErrorStep(HPRCX hprc);

/*
 *
 */

#ifdef KERNEL
extern  void    ProcessDebugEvent( DEBUG_EVENT64 *de, DBGKD_WAIT_STATE_CHANGE64  *sc );
extern  VOID    ProcessHandleExceptionCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessIgnoreExceptionCmd(HPRCX,HTHDX,LPDBB);
extern  BOOL    ProcessFrameStackWalkNextCmd( HPRCX, HTHDX, PCONTEXT, LPVOID );
extern  VOID    ProcessGetExtendedContextCmd(HPRCX hprc,HTHDX hthd,LPDBB lpdbb);
extern  VOID    ProcessSetExtendedContextCmd(HPRCX hprc,HTHDX hthd,LPDBB lpdbb);
extern  void    DeleteAllBps( VOID );
extern  VOID    DmPollTerminate( VOID );

#else

extern  VOID    ProcessBPAcceptedCmd( HPRCX hprcx, HTHDX hthdx, LPDBB lpdbb );
extern  VOID    ProcessGetDRegsCmd(HPRCX hprc, HTHDX hthd, LPDBB lpdbb);
extern  VOID    ProcessSetDRegsCmd(HPRCX hprc, HTHDX hthd, LPDBB lpdbb);

#endif


extern  void    ProcessExceptionEvent(DEBUG_EVENT64*, HTHDX);
extern  void    ProcessCreateThreadEvent(DEBUG_EVENT64*, HTHDX);
extern  void    ProcessCreateProcessEvent(DEBUG_EVENT64*, HTHDX);
extern  void    ProcessExitThreadEvent(DEBUG_EVENT64*, HTHDX);
extern  void    ProcessExitProcessEvent(DEBUG_EVENT64*, HTHDX);
extern  void    ProcessLoadDLLEvent(DEBUG_EVENT64*, HTHDX);
extern  void    ProcessUnloadDLLEvent(DEBUG_EVENT64*, HTHDX);
extern  void    ProcessOutputDebugStringEvent(DEBUG_EVENT64*, HTHDX);
extern  void    ProcessBreakpointEvent(DEBUG_EVENT64*, HTHDX);
extern  void    ProcessRipEvent(DEBUG_EVENT64*, HTHDX);
extern  void    ProcessBogusSSEvent(DEBUG_EVENT64*, HTHDX);

extern  void    ProcessSegmentLoadEvent(DEBUG_EVENT64 *, HTHDX);
extern  void    ProcessEntryPointEvent(DEBUG_EVENT64 *pde, HTHDX hthdx);

extern  void    NotifyEM(DEBUG_EVENT64*, HTHDX, DWORDLONG, DWORDLONG);
extern  void    ConsumeThreadEventsAndNotifyEM(DEBUG_EVENT64*, HTHDX, DWORDLONG, DWORDLONG);
extern  void    FreeHthdx(HTHDX hthd);
extern  XOSD    FreeProcess( HPRCX hprc, BOOL fKillRoot);

extern  VOID    ProcessCreateProcessCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessProcStatCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessThreadStatCmd(HPRCX,HTHDX,LPDBB);
extern  void    ProcessSpawnOrphanCmd(HPRCX,HTHDX,LPDBB);
extern  void    ProcessProgLoadCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessUnloadCmd(HPRCX,HTHDX,LPDBB);

extern  VOID    ProcessReadMemoryCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessWriteMemoryCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessGetContextCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessGetSectionsCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessSetContextCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessSingleStepCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessRangeStepCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessReturnStepCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessExecuteCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessContinueCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessFreezeThreadCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessTerminateThreadCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessTerminateProcessCmd(HPRCX,HTHDX,LPDBB);
extern  DWORD   ProcessAsyncGoCmd(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessGetFP(HPRCX,HTHDX,LPDBB);
extern  VOID    ProcessIoctlCmd( HPRCX, HTHDX, LPDBB );
extern  VOID    ProcessSSVCCustomCmd( HPRCX, HTHDX, LPDBB );
extern  VOID    ProcessSelLimCmd( HPRCX, HTHDX, LPDBB );
extern  VOID    ClearContextPointers(PKNONVOLATILE_CONTEXT_POINTERS);
extern  VOID    ProcessDebugActiveCmd(HPRCX, HTHDX, LPDBB);
extern  VOID    ProcessAsyncStopCmd(HPRCX, HTHDX, LPDBB );
extern  VOID    ProcessAllProgFreeCmd( HPRCX hprcXX, HTHDX hthd, LPDBB lpdbb );
extern  VOID    ProcessSetPathCmd( HPRCX hprcXX, HTHDX hthd, LPDBB lpdbb );
extern  VOID    ProcessQueryTlsBaseCmd( HPRCX hprcx, HTHDX hthdx, LPDBB lpdbb );
extern  VOID    ProcessQuerySelectorCmd(HPRCX, HTHDX, LPDBB);
extern  VOID    ProcessReloadModulesCmd(HPRCX hprcx, HTHDX hthdx, LPDBB lpdbb );
extern  VOID    ProcessVirtualQueryCmd(HPRCX hprcx, LPDBB lpdbb);
extern  VOID    ProcessGetDmInfoCmd(HPRCX hprc, LPDBB lpdbb, DWORD cb);
extern  VOID    ProcessRemoteQuit(VOID);
extern  ULONG   ProcessGetTimeStamp (HPRCX, HTHDX, LPDBB);

VOID
ProcessGetFrameContextCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    );

VOID
ProcessGetExceptionState(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    );

VOID
ProcessSetExceptionState(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    );

EXCEPTION_FILTER_DEFAULT
ExceptionAction(
    HPRCX hprc,
    DWORD dwExceptionCode
    );

VOID
RemoveExceptionList(
    HPRCX hprc
    );

EXCEPTION_LIST *
InsertException(
    EXCEPTION_LIST ** ppeList,
    LPEXCEPTION_DESCRIPTION lpexc
    );

VOID
ProcessBreakpointCmd(
    HPRCX hprcx,
    HTHDX hthdx,
    LPDBB lpdbb
    );

VOID
ProcessSystemServiceCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    );


VOID
InitExceptionList(
    HPRCX hprc
    );

VOID
CompleteTerminateProcessCmd(
    VOID
    );


//
// Public functions from walk.c
//

VOID
ExprBPCreateThread(
    HPRCX hprc,
    HTHDX hthd
    );

VOID
ExprBPExitThread(
    HPRCX hprc,
    HTHDX hthd
    );

VOID
ExprBPContinue(
    HPRCX hprc,
    HTHDX hthd
    );

VOID
ExprBPRestoreDebugRegs(
    HTHDX hthd
    );

VOID
ExprBPClearBPForStep(
    HTHDX hthd
    );

VOID
ExprBPResetBP(
    HTHDX hthd,
    PBREAKPOINT bp
    );

VOID
ExprBPInitialize(
    VOID
    );

PBREAKPOINT
GetWalkBPFromBits(
    HTHDX   hthd,
    DWORD   bits
    );

BOOL
IsWalkInGroup(
    HANDLE hWalk,
    PVOID pWalk
    );

HANDLE
SetWalk(
    HPRCX   hprc,
    HTHDX   hthd,
    DWORDLONG Addr,
    DWORD   Size,
    DWORD   BpType
    );

BOOL
RemoveWalk(
    HANDLE hWalk,
    BOOL Global
    );

BOOL
CheckDataBP(
    HTHDX hthd,
    PBREAKPOINT Bp
    );

//
//
//


#ifdef HAS_DEBUG_REGS
BOOL
SetupDebugRegister(         // implemented in mach.c
    HTHDX       hthd,
    int         Register,
    int         DataSize,
    DWORDLONG   DataAddr,
    DWORD       BpType
    );

VOID
ClearDebugRegister(
    HTHDX hthd,
    int Register
    );

VOID
ClearAllDebugRegisters(
    HPRCX hprc
);

#endif



extern
void
SSActionReplaceByte(
    DEBUG_EVENT64 *de,
    HTHDX hthdx,
    DWORDLONG unused,
    DWORDLONG lparam
    );

extern
void SSActionRemoveBP(
    DEBUG_EVENT64 *de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

extern
void
ActionDefineProcess(
    DEBUG_EVENT64 *de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

extern
void
ActionAllDllsLoaded(
    DEBUG_EVENT64 *de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

extern
void
ActionDebugActiveReady(
    DEBUG_EVENT64 *pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

extern
void
ActionDebugNewReady(
    DEBUG_EVENT64 *pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

extern
void
ActionExceptionDuringStep(
    DEBUG_EVENT64* de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

extern
void *
InfoExceptionDuringStep(
    HTHDX hthd
    );


BOOL
CDECL
DMPrintShellMsg(
    PCHAR szFormat,
    ...
    );

//
// event.c
//

PEXPECTED_EVENT
RegisterExpectedEvent(
    HPRCX hprc,
    HTHDX hthd,
    DWORD eventcode,
    DWORD_PTR subclass,
    METHOD* method,
    ACVECTOR action,
    BOOL persistent,
    DWORDLONG lparam
    );

PEXPECTED_EVENT
PeeIsEventExpected(
    HTHDX hthd,
    DWORD eventcode,
    DWORD_PTR subclass,
    BOOL bRemove
    );

VOID
ConsumeAllThreadEvents(
    HTHDX hthd,
    BOOL ConsumePersistent
    );

VOID
ConsumeAllProcessEvents(
    HPRCX hprc,
    BOOL ConsumePersistent
    );

VOID
ConsumeSpecifiedEvent(
    PEXPECTED_EVENT ee
    );

//
//
//

XOSD
Load(
    HPRCX hprc,
    LPCTSTR szAppName,
    LPCTSTR szArg,
    LPVOID pattrib,
    LPVOID tattrib,
    DWORD creationFlags,
    BOOL inheritHandles,
    CONST LPCTSTR* environment,
    LPCTSTR currentDirectory,
    STARTUPINFO FAR * pstartupInfo,
    LPPROCESS_INFORMATION lppi
    );

HPRCX
InitProcess(
    HPID hpid
    );

#if defined (TARGET_ALPHA) || defined (TARGET_AXP64) || defined(TARGET_IA64)
VOID
RemoveFuncList(
    HPRCX hprc
    );
#endif

VOID
WINAPI
DMFunc(
    DWORD cb,
    LPDBB lpdbb
    );


VOID
ReConnectDebugger(
    DEBUG_EVENT64 *de,
    BOOL fNoDllLoad
    );


DWORDLONG
GetNextOffset (
    HTHDX,
    BOOL
    );

extern void SetupEntryBP(HTHDX hthd);
void DestroyDllLoadItem(PDLLLOAD_ITEM pDll);


VOID
Reply(
    UINT length,
    void * lpbBuffer,
    HPID hpid
    );


// Use this specifically to send errors about process startup.
VOID
SendNTError(
        HPRCX hprc,
        DWORD dwErr,
        LPTSTR lszString
        );

// Used for other dbcError's.

VOID
SendDBCError(
        HPRCX hprc,
        DWORD dwErr,
        LPTSTR lszString
        );

/*
 **
 */


//
// Why a negative number, becuase someone set a whole dunch of DPRINTS to 0.
// So now we just continue the tradition.
//
#define MIN_VERBOSITY_LEVEL (-3)


#if DBG

#define assert(exp) if (!(exp)) {lpdbf->lpfnLBAssert(#exp,__FILE__,__LINE__);}

extern int  nVerbose;
extern BOOL FUseOutputDebugString;
extern char rgchDebug[];
extern void DebugPrint(char *, ...);

#define DPRINT(level, args) \
  if (nVerbose >= level) {  \
    extern HPID hpidRoot;     \
    ( (FUseOutputDebugString || (HPID)INVALID == hpidRoot) ? (DebugPrint) : (DMPrintShellMsg)) args; \
  }

#define DEBUG_PRINT(str) DPRINT(5, (str))
#define DEBUG_PRINT_1(str, a1) DPRINT(5, (str, a1))
#define DEBUG_PRINT_2(str, a1, a2) DPRINT(5, (str, a1, a2))
#define DEBUG_PRINT_3(str, a1, a2, a3) DPRINT(5, (str, a1, a2, a3))
#define DEBUG_PRINT_4(str, a1, a2, a3, a4) DPRINT(5, (str, a1, a2, a3, a4))
#define DEBUG_PRINT_5(str, a1, a2, a3, a4, a5) DPRINT(5, (str, a1, a2, a3, a4, a5))
#define DEBUG_LEVEL_PRINT(level, str) DPRINT(level, (str))

#else

#define assert(exp)

#define DPRINT(level, args)

#define DEBUG_PRINT(str)
#define DEBUG_PRINT_1(str, a1)
#define DEBUG_PRINT_2(str, a1, a2)
#define DEBUG_PRINT_3(str, a1, a2, a3)
#define DEBUG_PRINT_4(str, a1, a2, a3, a4)
#define DEBUG_PRINT_5(str, a1, a2, a3, a4, a5)

#define DEBUG_LEVEL_PRINT(level, str)
#endif

#define VERIFY(X) if (!(X)) assert(FALSE)

extern  DMTLFUNCTYPE        DmTlFunc;

/*
**   Win95/Chicago related functions
*/

BOOL IsChicago(VOID);

/*
**   WOW functions
*/

BOOL TranslateAddress(HPRCX, HTHDX, LPADDR, BOOL);
BOOL IsWOWPresent(VOID);


/*
**  Prototypes from util.c
*/

ULONG
SetReadPointer(
    ULONG cbOffset,
    int iFrom
    );

VOID
SetPointerToFile(
    HANDLE hFile
    );

VOID
SetPointerToMemory(
    HPRCX hprcx,
    DWORDLONG qw
    );

BOOL
DoRead(
    LPVOID lpv,
    DWORD cb
    );

BOOL
AreAddrsEqual(
    HPRCX     hprc,
    HTHDX     hthd,
    LPADDR    paddr1,
    LPADDR    paddr2
    );

HTHDX HTHDXFromPIDTID(PID, TID);
HTHDX HTHDXFromHPIDHTID(HPID, HTID);
HPRCX HPRCFromPID(PID);
HPRCX HPRCFromHPID(HPID);
HPRCX HPRCFromHPRC(HANDLE);


BOOL    WOWGetThreadContext(HTHDX hthdx, LPCONTEXT lpcxt);
BOOL    WOWSetThreadContext(HTHDX hthdx, LPCONTEXT lpcxt);

BOOL
CheckBpt(
    HTHDX       hthd,
    PBREAKPOINT pbp
    );

LPTSTR
MHStrdup(
    LPCTSTR s
    );

XOSD
DMSendRequestReply (
    DBC dbc,
    HPID hpid,
    HTID htid,
    DWORD cbInput,
    LPVOID lpInput,
    DWORD cbOutput,
    LPVOID lpOutput
    );

PVOID
DMCopyLargeReply(
    DWORD size
    );

XOSD
DMSendDebugPacket(
    DBC dbc,
    HPID hpid,
    HTID htid,
    DWORD cbInput,
    LPVOID lpInput
    );

UOFFSET
FileOffFromVA(
    PDLLLOAD_ITEM           pdi,
    HFILE                   hfile,
    UOFFSET                 uoffBasePE,
    const IMAGE_NT_HEADERS *pnthdr,
    UOFFSET                 va
    );

DWORD
CbReadDllHdr(
    HFILE hfile,
    UOFFSET uoff,
    LPVOID lpvBuf,
    DWORD cb
    );

ULONGLONG
GetRegValue(
    PCONTEXT regs,
    int cvindex
    );

VOID
DmSetFocus (
    HPRCX phprc
    );

BOOL
FGetExport(
    PDLLLOAD_ITEM pdi,
    HFILE       hfile,
    LPCTSTR     szExport,
    LPVOID*     plpvValue
    );

VOID
GetTaskList(
    PTASK_LIST pTask,
    DWORD dwNumTasks,
    LPDWORD lpdwNumReturned
    );

BOOL
AddrReadMemory(
    HPRCX       hprc,
    HTHDX       hthd,
    LPADDR      paddr,
    LPVOID      lpb,
    DWORD       cb,
    LPDWORD     pcbRead
    );

BOOL
AddrWriteMemory(
    HPRCX       hprc,
    HTHDX       hthd,
    LPADDR      paddr,
    LPVOID      lpv,
    DWORD       cb,
    LPDWORD     pcbWritten
    );

int
NumberOfThreadsInProcess(
    HPRCX hprc
    );

void
SetHandledStateInStoppedThreads(
    HPRCX hprc,
    BOOL ContinueHandled
    );

HTHDX
FindStoppedThread(
    HPRCX hprc
    );

//
// userapi.c / kdapi.c
//

BOOL
DbgReadMemory(
    HPRCX       hprc,
    DWORDLONG   lpOffset,
    LPVOID      lpv,
    DWORD       cb,
    LPDWORD     pcbRead
    );

BOOL
DbgWriteMemory(
    HPRCX       hprc,
    DWORDLONG   lpOffset,
    LPVOID      lpb,
    DWORD       cb,
    LPDWORD     pcbWritten
    );

BOOL
DbgGetThreadContext(
    HTHDX hthd,
    LPCONTEXT lpContext
    );

BOOL
DbgSetThreadContext(
    IN HTHDX hthd,
    IN LPCONTEXT lpContext
    );

EXHDLR *
GetExceptionCatchLocations(
    IN HTHDX,
    IN LPVOID
    );

VOID
GetMachineType(
    LPPROCESSOR p
    );

BOOL
DequeueAllEvents(
    BOOL fForce,
    BOOL fConsume
    );

VOID
InitEventQueue(
    VOID
    );

void
ContinueProcess(
    HPRCX hprc
    );

void
ContinueThread(
    HTHDX hthd
    );

void
ContinueThreadEx(
    HTHDX hthd,
    DWORD ContinueStatus,
    DWORD EventType,
    TSTATEX NewState
    );

BOOL
LoadDll(
    DEBUG_EVENT64 * de,
    HTHDX           hthd,
    LPWORD          lpcbPacket,
    LPBYTE *        lplpbPacket,
    BOOL            fThreadIsStopped
    );

#ifndef KERNEL
VOID
UnloadAllModules(
    HPRCX           hprc,
    HTHDX           hthd,
    BOOL            AlwaysNotify,
    BOOL            ReallyDestroy
    );

VOID
ReloadUsermodeModules(
    HTHDX hthd,
    PTCHAR String
    );

BOOL
DMWaitForDebugEvent(
    LPDEBUG_EVENT64 de64,
    DWORD timeout
    );

BOOL
MakeThreadSuspendItself(
    HTHDX   hthd
    );

void
ClearPendingDebugEvents(
    PID pid,
    TID tid
    );

#endif // KERNEL

VOID
AddQueue(
    DWORD   dwType,
    DWORD   dwProcessId,
    DWORD   dwThreadId,
    DWORD64 dwData,
    DWORD   dwLen
    );

#define QT_CONTINUE_DEBUG_EVENT     1
#define QT_RELOAD_MODULES           2
#define QT_TRACE_DEBUG_EVENT        3
#define QT_REBOOT                   4
#define QT_RESYNC                   5
#define QT_DEBUGSTRING              6
#define QT_CRASH                    7

//
// any ssvc not recognized by ProcessSystemServiceCmd is
// punted to this, which is provided separately by the user
// and kernel versions.
//
VOID
LocalProcessSystemServiceCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    );


//
//
//



//
//  Non Local Goto support
//

typedef HDEP HNLG;  // Handle to NLG
typedef struct _NLG_DESTINATION {
    UOFFSET uoffDestination;
    UOFFSET uoffFramePointer;
    DWORD   dwSig;
    DWORD   dwCode;
} NLG_DESTINATION;
typedef NLG_DESTINATION FAR * LPNLG_DESTINATION;

#define NLG_LONGJMP             0x00000000
#define NLG_EXCEPT_ENTER        0x00000001
#define NLG_CATCH_LEAVE         0x00000002
#define NLG_LONGJMPEX           0x00000003

#define NLG_CATCH_ENTER         0x00000100
#define NLG_FINALLY_ENTER       0x00000101
#define NLG_FILTER_ENTER        0x00000102
#define NLG_DESTRUCTOR_ENTER    0x00000103
// Post V4
#define NLG_GLOBAL_CONSTRUCTOR_ENTER 0x104
#define NLG_GLOBAL_DESTRUCTOR_ENTER  0x105
#define NLG_DLL_ENTRY                0x106
#define NLG_DLL_EXIT                 0x107
// Mac has no 2nd chance notification
#define NLG_EXCEPT_LAST_CHANCE       0x108

#define NLG_SIG                 0x19930520

typedef enum _NLG_LOCATION {
    NLG_DISPATCH,
    NLG_RETURN
} NLG_LOCATION, FAR * LPNLG_LOCATION;

#define hnlgNull    ((HNLG)NULL)
INT FAR PASCAL NLGComp  ( LPNLG, LPVOID, LONG );
VOID
ActionNLGDispatch(
    DEBUG_EVENT64* de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

VOID
ActionNLGDestination   (
    DEBUG_EVENT64* de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

HNLG CheckNLG ( HPRCX, HTHDX, NLG_LOCATION, LPADDR );
BOOL SetupNLG ( HTHDX, LPADDR );
UOFFSET GetSPFromNLGDest(HTHDX, LPNLG_DESTINATION);

void ProcessNonLocalGoto( HPRCX, HTHDX, LPDBB );

typedef enum _NFI {
    nfiHEMI,
} NFI; // NonLocalGoto Find Information
typedef NFI FAR * LPNFI;


#ifndef KERNEL

//
// OLE debugging support
//

typedef enum _OLESEG OLESEG;
typedef enum _ORPC ORPC;

typedef VOID (*COMPLETION_FUNCTION) (HTHDX, LPVOID);

OLESEG  GetOleSegType(LPVOID);
OLESEG  OleSegFromAddr(HPRCX, UOFFSET);
VOID    EnsureOleRpcStatus(HTHDX, COMPLETION_FUNCTION, LPVOID Argument);
BOOL    FClientNotifyStep(HTHDX, DEBUG_EVENT64*);
BOOL    FServerNotifyStop(HTHDX, DEBUG_EVENT64*);
ORPC    OrpcFromPthd(HTHDX, DEBUG_EVENT64*);
VOID    PushOleRetAddr(HTHDX, UOFFSET, UOFFSET);
VOID    PopOleRetAddr(HTHDX);
UOFFSET UoffOleRet(HTHDX);
UOFFSET EspOleRet(HTHDX);
VOID    ProcessOleEvent(DEBUG_EVENT64*, HTHDX);

BOOL
CheckAndSetupForOrpcSection(
    HTHDX   hthd
    );

UOFFSET
GetReturnDestination(
    HTHDX   hthd
    );


VOID
ActionOrpcClientGetBufferSize (
    DEBUG_EVENT64 * pde,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    );

VOID
ActionOrpcClientFillBuffer (
    DEBUG_EVENT64 * pde,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    );

VOID
ActionOrpcClientNotify (
    LPDEBUG_EVENT64 pde,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    );

VOID
ActionOrpcServerNotify(
    LPDEBUG_EVENT64 pde,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    );

VOID
ActionOrpcServerGetBufferSize(
    LPDEBUG_EVENT64 pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

void
ActionOrpcSkipToSource(
    LPDEBUG_EVENT64 pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );


//
//  Fiber Support
//
VOID ProcessFiberEvent(DEBUG_EVENT64*,HTHDX);
VOID RemoveFiberList(HPRCX);

typedef struct _DETOSAVE {
    LIST_ENTRY List;
    DEBUG_EVENT64 de;
} DETOSAVE, * PDETOSAVE;

PDETOSAVE
GetMostRecentDebugEvent(
    PID pid,
    TID tid
    );

#endif // !KERNEL


#ifdef KERNEL
/*
**  Kernel Debugger Specific Functions
*/

extern BOOL DmKdPtr64;
extern BOOL DmKdApi64;

BOOL
DmKdConnectAndInitialize(
    LPSTR lpProgName
    );

BOOL
WriteBreakPointEx(
    IN HTHDX hthd,
    IN ULONG BreakPointCount,
    IN OUT PDBGKD_WRITE_BREAKPOINT64 BreakPoints,
    IN ULONG ContinueStatus
    );

BOOL
RestoreBreakPointEx(
    IN ULONG BreakPointCount,
    IN PDBGKD_RESTORE_BREAKPOINT BreakPointHandles
    );


VOID
ContinueTargetSystem(
    DWORD ContinueStatus,
    PDBGKD_CONTROL_SET ControlSet
    );

VOID
RestoreKernelBreakpoints(
    HTHDX hthd,
    DWORDLONG Offset
    );

BOOL
ReadControlSpace(
    USHORT Processor,
    DWORDLONG TargetBaseAddress,
    PVOID UserInterfaceBuffer,
    ULONG TransferCount,
    PULONG ActualBytesRead
    );


NTSTATUS
DmKdReadDebuggerDataBlock(
    ULONG64 Address,
    OUT PDBGKD_DEBUG_DATA_HEADER64 DataBlock,
    ULONG SizeToRead
    );

NTSTATUS
DmKdReadDebuggerDataHeader(
    ULONG64 Address,
    OUT PDBGKD_DEBUG_DATA_HEADER64 DataHeader
    );

NTSTATUS
DmKdReadListEntry(
    ULONG64 Address,
    PLIST_ENTRY64 List64
    );

NTSTATUS
DmKdReadPointer(
    ULONG64 Address,
    PULONG64 Pointer64
    );

NTSTATUS
DmKdReadLoaderEntry(
    ULONG64 Address,
    PLDR_DATA_TABLE_ENTRY64 b64
    );

#if defined(HAS_DEBUG_REGS)
BOOL  GetExtendedContext(HTHDX hthd, PKSPECIAL_REGISTERS pksr);
BOOL  SetExtendedContext(HTHDX hthd, PKSPECIAL_REGISTERS pksr);
#endif

#define KERNEL_MODULE_NAME     "nt"
#define KERNEL_IMAGE_NAME      "ntoskrnl.exe"
#define KERNEL_IMAGE_NAME_MP   "ntkrnlmp.exe"
#define OSLOADER_IMAGE_NAME    "osloader.exe"
#define HAL_IMAGE_NAME         "hal.dll"
#define HAL_MODULE_NAME        "HAL"


extern CRITICAL_SECTION csApiInterlock;
extern CRITICAL_SECTION csSynchronizeTargetInterlock;

#ifdef DBG
#define DEBUG_API_INTERLOCK 1
#endif

#ifndef DEBUG_API_INTERLOCK
#define TakeApiLock()       EnterCriticalSection(&csApiInterlock)
#define ReleaseApiLock()    LeaveCriticalSection(&csApiInterlock)
#define TryApiLock()        TryEnterCriticalSection(&csApiInterlock)
#else

VOID
TakeApiLockFunc(
    PCSTR pszFile,
    int nLine
    );

BOOL
TryApiLockFunc(
    PCSTR pszFile,
    int nLine
    );

VOID
ReleaseApiLockFunc(
    PCSTR pszFile,
    int nLine
    );

#define TakeApiLock()       TakeApiLockFunc(__FILE__, __LINE__)
#define TryApiLock()        TryApiLockFunc(__FILE__, __LINE__)
#define ReleaseApiLock()    ReleaseApiLockFunc(__FILE__, __LINE__)

#endif


typedef struct MODULEALIAS {
    CHAR    ModuleName[16];
    CHAR    Alias[16];
    BOOL    Special;
} MODULEALIAS, *LPMODULEALIAS;

#define MAX_MODULEALIAS 100

LPMODULEALIAS
FindAliasByImageName(
    LPSTR lpImageName
    );

LPMODULEALIAS
FindAddAliasByModule(
    LPSTR lpImageName,
    LPSTR lpModuleName
    );

typedef struct IMAGEINFO {
    DWORD                 CheckSum;
    DWORD                 TimeStamp;
    DWORD                 SizeOfImage;
    DWORDLONG             BaseOfImage;
    DWORD                 NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
} IMAGEINFO, *LPIMAGEINFO;

void
ParseDmParams(
    LPSTR p
    );

BOOL
ReadImageInfo(
    LPSTR                  lpImageName,
    LPSTR                  lpFoundName,
    LPSTR                  lpPath,
    LPIMAGEINFO            ii
    );


#endif  // KERNEL
