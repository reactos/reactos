/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Od.h

Abstract:

    This file contains types and prototypes which are exposed
    to all OSDebug components and clients.

Author:

    Kent Forschmiedt (kentf) 10-Sep-1993

Environment:

    Win32, User Mode

--*/

#if ! defined _OD_
#define _OD_

#include "odtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//     Constants and other magic

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
//     OSDebug types and status codes
//

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


typedef XOSD (OSDAPI *LPFNSVC) ( DBC, HPID, HTID, DWORD64, DWORD64 );

typedef XOSD (OSDAPI *EMFUNC)();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
//     OSDebug API set
//

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
//     OSDebug Initialization/Termination
//


XOSD
OSDAPI
OSDInit(
    LPDBF lpdbf
    );

XOSD
OSDAPI
OSDTerm(
    VOID
    );



//
//     EM Management
//


enum {
    emNative,
    emNonNative
};
typedef DWORD EMTYPE;

#define EMISINFOSIZE 80
typedef struct _EMIS {
    DWORD fCanSetup;
    DWORD dwMaxPacket;
    DWORD dwOptPacket;
    DWORD dwInfoSize;
    TCHAR rgchInfo[EMISINFOSIZE];
} EMIS;
typedef EMIS * LPEMIS;

typedef struct _EMSS {
    //
    // emfSetup is called to tell the EM to fetch workspace
    // values from the shell.
    //

    // The EM may support three phases of setup;
    // Load, interact and save.
    // Load means call GetSet for each workspace parameter
    //  with fSet == FALSE.  The value will be returned, and
    //  the EM must remember it.
    // Interact means allow the user to perform configuration
    //  operations, like clicking things in a dialog.  The new
    //  values are remembered by the EM.
    // Save means the EM will call GetSet with fSet == TRUE for
    //  each configuration value; the shell-provided GetSet
    //  will store these as workspace values.
    //
    DWORD fLoad;
    //DWORD fInteractive;
    DWORD fSave;

    //
    // lParam is an instance or "this" value supplied by the shell,
    // and is only used when calling lpfnGetSet().
    //
    //LPARAM lParam;

    LPGET_MODULE_INFO pfnGetModuleInfo;
} EMSS;
typedef EMSS * LPEMSS;


XOSD
OSDAPI
OSDAddEM(
    EMFUNC emfunc,
    LPDBF lpdbf,
    LPHEM lphem,
    EMTYPE emtype
    );

XOSD
OSDAPI
OSDDeleteEM(
    HEM hem
    );

XOSD
OSDAPI
OSDGetCurrentEM(
    HPID hpid,
    HTID htid,
    LPHEM lphem
    );

XOSD
OSDAPI
OSDNativeOnly(
    HPID hpid,
    HTID htid,
    DWORD fNativeOnly
    );

XOSD
OSDAPI
OSDUseEM(
    HPID hpid,
    HEM hem
    );

XOSD
OSDAPI
OSDDiscardEM(
    HPID hpid,
    HTID htid,
    HEM hem
    );

XOSD
OSDAPI
OSDDiscardTL(
    HPID hpid,
    HTL htl
    );

XOSD
OSDAPI
OSDEMGetInfo(
    HEM hem,
    LPEMIS lpemis
    );

XOSD
OSDAPI
OSDEMSetup(
    HEM hem,
    LPEMSS lpemss
    );



//
//     TL Management
//

#define TLISINFOSIZE 80
typedef struct _TLIS {
    DWORD fCanSetup;
    DWORD dwMaxPacket;
    DWORD dwOptPacket;
    DWORD dwInfoSize;
    DWORD fRemote;
    MPT   mpt;
    MPT   mptRemote;
    TCHAR rgchInfo[TLISINFOSIZE];
} TLIS;
typedef TLIS * LPTLIS;

typedef struct _TLSS {
    DWORD fLoad;
    //DWORD fInteractive;
    //DWORD fSave;
    //LPVOID lpvPrivate;
    //LPARAM lParam;
    LPGET_MODULE_INFO pfnGetModuleInfo;
    MPT mpt;
    BOOL fRMAttached;
} TLSS;
typedef TLSS * LPTLSS;

// the set of transport layer commands process by TLFunc and DMTLFunc
typedef enum {
    tlfRegisterDBF,     // register the debugger helper functions
    tlfInit,            // initialize/create a (specific) transport layer
    tlfDestroy,         // vaporize any tl structs created
    tlfConnect,         // connect to the companion transport layer
    tlfDisconnect,      // disconnected from the companion transport layer
    tlfSendVersion,     // Send the version packet to the remote side
    tlfGetVersion,      // Request the version packet from the remote side
    tlfSetBuffer,       // set the data buffer to be used for incoming packets
    tlfDebugPacket,     // send the debug packet to the debug monitor
    tlfRequest,         // request data from the companion transport layer
    tlfReply,           // reply to a data request message
    tlfGetInfo,         // return an id string and other data
    tlfSetup,           // set up the transport layer
    tlfGetProc,         // return the true TLFUNC proc for the htl
    tlfLoadDM,          // load the DM module
    tlfSetErrorCB,      // Set the address of the error callback function
    tlfPoll,            // WIN32S: enter polling loop
    tlfRemoteQuit,      // signal loss of connection
    tlfPassiveConnect,      // the remote monitor connects to transport with this
    tlfGetLastError,    // get the string associated with the last error
    tlfMax
} _TLF;
typedef _TLF TLF;


typedef XOSD (OSDAPI *TLFUNC)(TLF wCommand, HPID hpid, DWORD64 wParam, DWORD64 lParam);


XOSD
OSDAPI
OSDAddTL(
    TLFUNC tlfunc,
    LPDBF lpdbf,
    LPHTL lphtl
    );

XOSD
OSDAPI
OSDStartTL(
    HTL htl
    );

XOSD
OSDAPI
OSDDeleteTL(
    HTL htl
    );

XOSD
OSDAPI
OSDTLGetInfo(
    HTL htl,
    LPTLIS lptlis
    );

XOSD
OSDAPI
OSDTLSetup(
    HTL htl,
    LPTLSS lptlss
    );

XOSD
OSDAPI
OSDDisconnect(
    HPID hpid,
    HTID htid
    );

//
//     Process, thread management
//

XOSD
OSDAPI
OSDCreateHpid(
    LPFNSVC lpfnsvcCallBack,
    HEM hemNative,
    HTL htl,
    LPHPID lphpid
    );

XOSD
OSDAPI
OSDDestroyHpid(
    HPID hpid
    );

XOSD
OSDAPI
OSDDestroyHtid(
    HPID hpid,
    HTID htid
    );


#define IDSTRINGSIZE 12
#define STATESTRINGSIZE 60
typedef struct _PST {
    DWORD dwProcessID;
    DWORD dwProcessState;
    TCHAR rgchProcessID[IDSTRINGSIZE];
    TCHAR rgchProcessState[STATESTRINGSIZE];
} PST;
typedef PST * LPPST;

typedef struct _TST {
    DWORD dwThreadID;
    DWORD dwSuspendCount;
    DWORD dwSuspendCountMax;
    DWORD dwPriority;
    DWORD dwPriorityMax;
    DWORD dwState;
    DWORDLONG dwTeb;
    TCHAR rgchThreadID[IDSTRINGSIZE];
    TCHAR rgchState[STATESTRINGSIZE];
    TCHAR rgchPriority[STATESTRINGSIZE];
} TST;
typedef TST * LPTST;

XOSD
OSDAPI
OSDGetThreadStatus(
    HPID hpid,
    HTID htid,
    LPTST lptst
    );

XOSD
OSDAPI
OSDGetProcessStatus(
    HPID hpid,
    LPPST lppst
    );

XOSD
OSDAPI
OSDFreezeThread(
    HPID hpid,
    HTID htid,
    DWORD fFreeze
    );

XOSD
OSDAPI
OSDSetThreadPriority(
    HPID hpid,
    HTID htid,
    DWORD dwPriority
    );


//
//     Address manipulation
//


XOSD
OSDAPI
OSDGetAddr(
    HPID hpid,
    HTID htid,
    ADR adr,
    LPADDR lpaddr
    );

XOSD
OSDAPI
OSDSetAddr(
    HPID hpid,
    HTID htid,
    ADR adr,
    LPADDR lpaddr
    );

XOSD
OSDAPI
OSDFixupAddr(
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    );

XOSD
OSDAPI
OSDUnFixupAddr(
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    );

XOSD
OSDAPI
OSDSetEmi(
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    );

XOSD
OSDAPI
OSDRegisterEmi(
    HPID hpid,
    HEMI hemi,
    LPTSTR lsz
    );

XOSD
OSDAPI
OSDUnRegisterEmi(
    HPID hpid,
    HEMI hemi
    );

XOSD
OSDAPI
OSDCompareAddrs(
    HPID hpid,
    LPADDR lpaddr1,
    LPADDR lpaddr2,
    LPDWORD lpdwResult
    );


XOSD
OSDAPI
OSDGetMemoryInformation(
    HPID hpid,
    HTID htid,
    LPMEMINFO lpMemInfo
    );



XOSD
OSDAPI
OSDGetTimeStamp(
        HPID    hpid,
        HTID    htid,
        LPTSTR  Image,
        ULONG*  TimeStamp,      OPTIONAL
        ULONG*  CheckSum        OPTIONAL
        );

//
//     Module lists
//

typedef struct _MODULE_LIST {
    DWORD           Count;
} MODULE_LIST;
typedef struct _MODULE_LIST * LPMODULE_LIST;

typedef struct _MODULE_ENTRY {
    DWORD   Flat;
    DWORD   Real;
    DWORD   Segment;
    DWORD   Selector;
    DWORDLONG   Base;
    DWORDLONG   Limit;
    DWORD   Type;
    DWORD   SectionCount;
    HEMI    Emi;
    TCHAR   Name[ MAX_PATH ];
} MODULE_ENTRY;
typedef struct _MODULE_ENTRY * LPMODULE_ENTRY;

#define ModuleListCount(m)                      ((m)->Count)
#define FirstModuleEntry(m)                     ((LPMODULE_ENTRY)((m)+1))
#define NextModuleEntry(e)                      ((e)+1)
#define NthModuleEntry(m,n)                     (FirstModuleEntry(m)+(n))

#define ModuleEntryFlat(e)                      ((e)->Flat)
#define ModuleEntryReal(e)                      ((e)->Real)
#define ModuleEntrySegment(e)                   ((e)->Segment)
#define ModuleEntrySelector(e)                  ((e)->Selector)
#define ModuleEntryBase(e)                      ((e)->Base)
#define ModuleEntryLimit(e)                     ((e)->Limit)
#define ModuleEntryType(e)                      ((e)->Type)
#define ModuleEntrySectionCount(e)              ((e)->SectionCount)
#define ModuleEntryName(e)                      ((e)->Name)
#define ModuleEntryEmi(e)                       ((e)->Emi)


XOSD
OSDAPI
OSDGetModuleNameFromAddress(
    IN  HPID            hpid,
    IN  DWORD64         Address,
    OUT LPTSTR          pszModuleName,
    IN  size_t          SizeOf
    );

XOSD
OSDAPI
OSDGetModuleList(
    HPID hpid,
    HTID htid,
    LPTSTR lszModuleName,
    LPMODULE_LIST * lplpModuleList
    );




//
//     Target Application load/unload
//

typedef struct _SPAWNORPHAN {
    DWORD   dwPid;          // pid of newly spawned process (0 on some systems)
    CHAR    rgchErr[512];   // error string, or "" for successful spawn
} SPAWNORPHAN;  // Data returned from OSDSpawnOrphan
typedef SPAWNORPHAN *LPSPAWNORPHAN;

XOSD
OSDAPI
OSDSpawnOrphan (
        HPID hpid,
        LPCTSTR lszRemoteExe,
        LPCTSTR lszCmdLine,
        LPCTSTR lszRemoteDir,
        LPSPAWNORPHAN FAR lpso,
        DWORD   dwFlags
        );


// Bit flags for dwFlags in ProgramLoad
#define ulfMultiProcess             0x0001L     // OS2, NT, and ?MAC?
#define ulfMinimizeApp              0x0002L     // Win32
#define ulfNoActivate               0x0004L     // Win32
#define ulfInheritHandles           0x0008L     // Win32  (DM only?)
#define ulfWowVdm                   0x0010L     // Win32
#define ulfJavaDebugUsingBrowser    0x0020L     // debug Java program using browser (instead of stand-alone)
#define ulfSqlDebug                 0x0040L     // SQL debugging wanted

XOSD
OSDAPI
OSDProgramLoad(
    HPID hpid,
    LPTSTR lszRemoteExe,
    LPTSTR lszArgs,
    LPTSTR lszWorkingDir,
    LPTSTR lszDebugger,
    DWORD dwFlags
    );

XOSD
OSDAPI
OSDProgramFree(
    HPID hpid
    );

XOSD
OSDAPI
OSDDebugActive(
    HPID hpid,
    LPVOID lpvData,
    DWORD cbData
    );

XOSD
OSDAPI
OSDSetPath(
    HPID hpid,
    DWORD fSet,
    LPTSTR lszPath
    );


XOSD
OSDAPI
OSDNewSymbolsLoaded(
    HPID
    );

XOSD
OSDAPI
OSDSignalKernelLoadCompleted(
    HPID
    );




//
//     Target execution control
//

typedef struct _EXOP {
    BYTE fSingleThread;
    BYTE fStepOver;
    BYTE fQueryStep;
    BYTE fInitialBP;
    BYTE fPassException;
    BYTE fSetFocus;
    BYTE fReturnValues;     // send back dbcExitedFunction
    BYTE fGo;               // => ConsumeAllProcessEvents () before going
} EXOP;
typedef EXOP * LPEXOP;



XOSD
OSDAPI
OSDGo(
    HPID hpid,
    HTID htid,
    LPEXOP lpexop
    );

XOSD
OSDAPI
OSDSingleStep(
    HPID hpid,
    HTID htid,
    LPEXOP lpexop
    );

XOSD
OSDAPI
OSDRangeStep(
    HPID hpid,
    HTID htid,
    LPADDR lpaddrMin,
    LPADDR lpaddrMax,
    LPEXOP lpexop
    );

XOSD
OSDAPI
OSDReturnStep(
    HPID hpid,
    HTID htid,
    LPEXOP lpexop
    );

XOSD
OSDAPI
OSDAsyncStop(
    HPID hpid,
    DWORD fSetFocus
    );



//
//     Target function evaluation
//

XOSD
OSDAPI
OSDSetupExecute(
    HPID hpid,
    HTID htid,
    LPHIND lphind
    );

XOSD
OSDAPI
OSDStartExecute(
    HPID hpid,
    HIND hind,
    LPADDR lpaddr,
    DWORD fIgnoreEvents,
    DWORD fFar
    );

XOSD
OSDAPI
OSDCleanUpExecute(
    HPID hpid,
    HIND hind
    );



//
//     Target information
//

XOSD
OSDAPI
OSDGetDebugMetric(
    HPID hpid,
    HTID htid,
    MTRC mtrc,
    LPVOID lpv
    );



//
//     Target memory and objects
//


XOSD
OSDAPI
OSDReadMemory(
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPVOID lpBuffer,
    DWORD cbBuffer,
    LPDWORD lpcbRead
    );

XOSD
OSDAPI
OSDWriteMemory(
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPVOID lpBuffer,
    DWORD cbBuffer,
    LPDWORD lpcbWritten
    );

XOSD
OSDAPI
OSDGetObjectLength(
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPUOFFSET lpuoffStart,
    LPUOFFSET lpuoffLength
    );

XOSD
OSDGetFunctionInformation(
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPFUNCTION_INFORMATION lpFunctionInformation
    );



//
//     Target register manipulation
//


/*
**  Register types --- flags describing recommendations on
**      register display
*/

enum {
    rtProcessMask   = 0x0f,     // Mask for processor type bits
                                // these are enumerates, not bitfields.
    rtCPU           = 0x01,     // Central Processing Unit
    rtFPU           = 0x02,     // Floating Point Unit
    rtMMU           = 0x03,     // Memory Manager Unit

    rtGroupMask     = 0xff0,    // Which group(s) register falls into
                                // Bitfields
    rtInvisible     = 0x010,    // Recommend no display
    rtRegular       = 0x020,    // Recommend regular display
    rtExtended      = 0x040,    // Recommend extended display
    rtSpecial       = 0x080,    // Special and hidden regs for user and kernel
    rtKmode         = 0x100,    // kernel mode only regs

    rtFmtTypeMask   = 0xf000,   // Mask of display formats
                                // these are enumerates, not bitfields.
    rtInteger       = 0x1000,   // Unsigned integer format
    rtFloat         = 0x2000,   // Floating point format
    rtAddress       = 0x3000,   // Address format
    rtBit                   = 0x4000,   // IA64 registers displayed as bits (UNAT, RNAT, PREDS, etc.)

    rtMiscMask      = 0xf0000,  // misc info
                                // Bitfields
    rtPC            = 0x10000,  // this is the PC
    rtFrame         = 0x20000,  // this reg affects the stack frame
    rtNewLine       = 0x40000,  // print a newline when listing

    rtFlags         = 0x80000,   // Flags register (cast is to avoid warning)

    rtIA64Mask      = 0xf00000,  // Mask for IA64 special values
    rtStacked       = 0x100000,  // stacked IA64 register - check the stack frame before displaying
    rtNat           = 0x200000  // has a NAT bit - check the NAT register and format accordingly
};
typedef DWORD RT;   // Register Types

#define rtFmtTypeShift  12

enum {
    ftProcessMask   = 0x0f,     // Mask for processor type bits
                                // these are enumerates, not bitfields.
    ftCPU           = 0x01,     // Central Processing Unit
    ftFPU           = 0x02,     // Floating Point Unit
    ftMMU           = 0x03,     // Memory Manager Unit

    ftGroupMask     = 0xff0,    // Which group(s) register falls into
                                // Bitfields
    ftInvisible     = 0x010,    // Recommend no display
    ftRegular       = 0x020,    // Recommend regular display
    ftExtended      = 0x040,    // Recommend extended display
    ftSpecial       = 0x080,    // Special and hidden regs, user and kernel
    ftKmode         = 0x100,    // kernel mode only

    ftFmtTypeMask   = 0xf000,   // Mask of display formats
                                // these are enumerates, not bitfields.
    ftInteger       = 0x1000,   // Unsigned integer format
    ftFloat         = 0x2000,   // Floating point format
    ftAddress       = 0x3000,   // Address format

    ftMiscMask      = 0xf0000,  // misc info
                                // Bitfields
    ftPC            = 0x10000,  // this is the PC
    ftFrame         = 0x20000,  // this reg affects the stack frame
    ftNewLine       = 0x40000   // print a newline when listing
};
typedef DWORD FT;   // Flag Types

#define ftFmtTypeShift  8

/*
**  Register description:  This structure contains the description for
**      a register on the machine.  Note that dwId must be used to get
**      the value for this register but a different index is used to get
**      this description structure.
*/

typedef struct {
    LPTSTR      lszName;        /* Pointer into EM for registers name   */
    RT          rt;             /* Register Type flags                  */
    DWORD       dwcbits;        /* Number of bits in the register       */
    DWORD       dwGrp;
    DWORD       dwId;           /* Value to use with Read/Write Register*/
} RD;               // Register Description
typedef RD * LPRD;

/*
**  Flag Data description: This structure contains the description for
**      a flag on the machine.  Note that the dwId field contains the
**      value to be used with Read/Write register to get the register which
**      contains this flag.
*/
typedef struct _FD {
    LPTSTR      lszName;
    FT          ft;
    DWORD       dwcbits;
    DWORD       dwGrp;
    DWORD       dwId;
} FD;
typedef FD * LPFD;


XOSD
OSDAPI
OSDGetRegDesc(
    HPID hpid,
    HTID htid,
    DWORD ird,
    LPRD lprd
    );

XOSD
OSDAPI
OSDGetFlagDesc(
    HPID hpid,
    HTID htid,
    DWORD ifd,
    LPFD lpfd
    );

XOSD
OSDAPI
OSDReadRegister(
    HPID hpid,
    HTID htid,
    DWORD dwid,
    LPVOID lpValue
    );

XOSD
OSDAPI
OSDWriteRegister(
    HPID hpid,
    HTID htid,
    DWORD dwId,
    LPVOID lpValue
    );

XOSD
OSDAPI
OSDReadFlag(
    HPID hpid,
    HTID htid,
    DWORD dwId,
    LPVOID lpValue
    );

XOSD
OSDAPI
OSDWriteFlag(
    HPID hpid,
    HTID htid,
    DWORD dwId,
    LPVOID lpValue
    );

XOSD
OSDAPI
OSDSaveRegs(
    HPID hpid,
    HTID htid,
    LPHIND lphReg
    );

XOSD
OSDAPI
OSDRestoreRegs(
    HPID hpid,
    HTID htid,
    HIND hregs
    );





//
//     Breakpoints
//

enum {
    bptpExec,
    bptpDataC,
    bptpDataW,
    bptpDataR,
    bptpDataExec,
    bptpRegC,
    bptpRegW,
    bptpRegR,
    bptpMessage,
    bptpMClass,
    bptpInt,
    bptpRange
};
typedef DWORD BPTP;

enum {
    bpnsStop,
    bpnsContinue,
    bpnsCheck,
    bpnsMax             // donc for mac dm
};
typedef DWORD BPNS;

typedef struct _BPIS {
    BPTP   bptp;
    BPNS   bpns;
    DWORD  fOneThd;
    union {
        DWORDLONG Alignment1;
        HTID   htid;
    };
    union {
        struct {
            ADDR addr;
        } exec;
        struct {
            ADDR addr;
            DWORD cb;
            BOOL fEmulate;
        } data;
        struct {
            DWORD dwId;
        } reg;
        struct {
            ADDR addr;
            DWORD imsg;
            DWORD cmsg;
        } msg;
        struct {
            ADDR addr;
            DWORD dwmask;
        } mcls;
        struct {
            DWORD ipt;
        } ipt;
        struct {
            ADDR addr;
            DWORD cb;
        } rng;
    };
} BPIS;
typedef BPIS * LPBPIS;

typedef struct _BPS {
    DWORD cbpis;
    DWORD cmsg;
    DWORD fSet;
    DWORD Alignment;  //v-vadimp for proper positioning of rgbpis in 64<->32 bits
    //    BPIS   rgbpis[];
    //    DWORD  rgdwMessage[];
    //    XOSD   rgxosd[];
    //    DWORD64 rgqwNotification[];
} BPS;
typedef BPS * LPBPS;

#define RgBpis(B)         ((LPBPIS)(((LPBPS)(B)) + 1))
#define DwMessage(B)      ((LPDWORD)(RgBpis((B)) + ((LPBPS)(B))->cbpis))
#define RgXosd(B)         ((LPXOSD)(DwMessage((B)) + ((LPBPS)(B))->cmsg))
#define QwNotification(B) ((PDWORDLONG)(RgXosd((B)) + ((LPBPS)(B))->cbpis))
#define SizeofBPS(B)      ( sizeof(BPS) +                                    \
                          (((LPBPS)(B))->cbpis *                             \
                            (sizeof(BPIS) + sizeof(XOSD) + sizeof(DWORDLONG))) + \
                          (((LPBPS)(B))->cmsg * sizeof(DWORD)) )

XOSD
OSDAPI
OSDBreakpoint(
    HPID hpid,
    LPBPS lpbps
    );






//
//     Assembly, Unassembly
//

enum {
    dopNone     = 0x00000000,
    dopAddr     = 0x00000001,   // put address (w/ seg) in front of disassm
    dopFlatAddr = 0x00000002,   // put flat address (no seg)
    dopOpcode   = 0x00000004,   // dump the Opcode
    dopOperands = 0x00000008,   // dump the Operands
    dopRaw      = 0x00000010,   // dump the raw code bytes
    dopEA       = 0x00000020,   // calculate the effective address
    dopSym      = 0x00000040,   // output symbols
    dopUpper    = 0x00000080,   // force upper case for all chars except syms
    dopHexUpper = 0x00000100,   // force upper case for all hex constants
                                // (implied true if dopUpper is set)
    dopReserved = 0x01000000    // these are for the shell to use
};
typedef DWORD DOP;              // Disassembly OPtions


typedef struct _SDI {
    DOP    dop;              // Disassembly OPtions (see above)
    ADDR   addr;             // The address to disassemble
    BOOL   fAssocNext;       // This instruction is associated w/ the next one
    BOOL   fIsBranch;
    BOOL   fIsCall;
    BOOL   fJumpTable;
    ADDR   addrEA0;          // First effective address
    ADDR   addrEA1;          // Second effective address
    ADDR   addrEA2;          // Third effective address
    DWORD  cbEA0;            // First effective address size
    DWORD  cbEA1;            // Second effective address size
    DWORD  cbEA2;            // Third effective address size
    LONG   ichAddr;
    LONG   ichBytes;
    LONG   ichPreg;
    LONG   ichOpcode;
    LONG   ichOperands;
    LONG   ichComment;
    LONG   ichEA0;
    LONG   ichEA1;
    LONG   ichEA2;
    LPTSTR lpch;
} SDI;  // Structured DiSsassembly
typedef SDI *LPSDI;

XOSD
OSDAPI
OSDUnassemble(
    HPID hpid,
    HTID htid,
    LPSDI lpsdi
    );

XOSD
OSDAPI
OSDGetPrevInst(
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPUOFFSET lpuoff
    );

XOSD
OSDAPI
OSDAssemble(
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPTSTR lsz
    );




//
//     Stack tracing
//

XOSD
OSDAPI
OSDGetFrame(
    HPID hpid,
    HTID htid,
    DWORD cFrame,
    LPHTID lphtid
    );





//
//     Target host file i/o
//

XOSD
OSDAPI
OSDMakeFileHandle(
    HPID hpid,
    LPARAM lPrivateHandle,
    HOSDFILE * lphosdFile
    );

XOSD
OSDAPI
OSDDupFileHandle(
    HOSDFILE hosdFile,
    HOSDFILE * lphosdDup
    );


XOSD
OSDAPI
OSDCloseFile(
    HOSDFILE hosdFile
    );

XOSD
OSDAPI
OSDSeekFile(
    HOSDFILE hosdFile,
    DWORD dwLocationLo,
    DWORD dwLocationHi,
    DWORD dwOrigin
    );

XOSD
OSDAPI
OSDReadFile(
    HOSDFILE hosdFile,
    LPBYTE lpbBuffer,
    DWORD cbData,
    LPDWORD lpcbBytesRead
    );

XOSD
OSDAPI
OSDWriteFile(
    HOSDFILE hosdFile,
    LPBYTE lpbBuffer,
    DWORD cbData,
    LPDWORD lpdwBytesWritten
    );





//
//     Exception handling
//

//
// commands understood by OSDGetExceptionState
//

typedef enum _EXCEPTION_CONTROL {
    exfFirst,
    exfNext,
    exfSpecified,
    exfDefault
} EXCEPTION_CONTROL;
typedef EXCEPTION_CONTROL * LPEXCEPTION_CONTROL;

//
// Exception information packet
//
#define EXCEPTION_STRING_SIZE 60
typedef struct _EXCEPTION_DESCRIPTION {
    DWORD                    dwExceptionCode;
    EXCEPTION_FILTER_DEFAULT efd;
    EXCEPTION_CONTROL        exc;
    TCHAR                    rgchDescription[EXCEPTION_STRING_SIZE];
} EXCEPTION_DESCRIPTION;
typedef EXCEPTION_DESCRIPTION * LPEXCEPTION_DESCRIPTION;

XOSD
OSDAPI
OSDGetExceptionState(
    HPID hpid,
    HTID htid,
    LPEXCEPTION_DESCRIPTION lpExd
    );

XOSD
OSDAPI
OSDSetExceptionState (
    HPID hpid,
    HTID htid,
    LPEXCEPTION_DESCRIPTION lpExd
    );



//
//     Message information
//

enum {
    msgMaskNone  = 0x0,
    msgMaskWin   = 0x1,
    msgMaskInit  = 0x2,
    msgMaskInput = 0x4,
    msgMaskMouse = 0x8,
    msgMaskSys   = 0x10,
    msgMaskClip  = 0x20,
    msgMaskNC    = 0x40,
    msgMaskDDE   = 0x80,
    msgMaskOther = 0x100,
    msgMaskAll   = 0x0FFF,
};


typedef struct _MESSAGEINFO {
    DWORD   dwMsg;         //  Message number
    LPTSTR  lszMsgText;    //  Message Text
    DWORD   dwMsgMask;     //  Message mask
} MESSAGEINFO;
typedef struct _MESSAGEINFO *LPMESSAGEINFO;

//
//  MSG Map structure
//
typedef struct _MESSAGEMAP {
    DWORD          dwCount;      //  Number of elements
    LPMESSAGEINFO  lpMsgInfo;    //  Pointer to array
} MESSAGEMAP;
typedef struct _MESSAGEMAP *LPMESSAGEMAP;

XOSD
OSDAPI
OSDGetMessageMap(
    HPID hpid,
    HTID htid,
    LPMESSAGEMAP * lplpMessageMap
    );


typedef struct _MASKINFO {
    DWORD dwMask;
    LPTSTR lszMaskText;
} MASKINFO;
typedef MASKINFO * LPMASKINFO;

typedef struct _MASKMAP {
    DWORD dwCount;
    LPMASKINFO lpMaskInfo;
} MASKMAP;
typedef MASKMAP * LPMASKMAP;

XOSD
OSDAPI
OSDGetMessageMaskMap(
    HPID hpid,
    HTID htid,
    LPMASKMAP * lplpMaskMap
    );




//
//     Miscellaneous control functions
//

XOSD
OSDAPI
OSDShowDebuggee(
    HPID hpid,
    DWORD fShow
    );





//
//     Communication and synchronization with DM
//

XOSD
OSDAPI
OSDInfoReply(
    HPID hpid,
    HTID htid,
    LPVOID lpvData,
    DWORD cbData
    );




//
//     OS Specific info and control
//

typedef struct _TASKENTRY {
    DWORD dwProcessID;
    TCHAR szProcessName[MAX_PATH];
} TASKENTRY;
typedef TASKENTRY * LPTASKENTRY;

typedef struct _TASKLIST {
    DWORD dwCount;
    LPTASKENTRY lpTaskEntry;
} TASKLIST;
typedef TASKLIST * LPTASKLIST;


XOSD
OSDAPI
OSDGetTaskList(
    HPID hpid,
    LPTASKLIST * lplpTaskList
    );


#include "ssvc.h"
#define FIRST_PRIVATE_SSVC 0x8000

XOSD
OSDAPI
OSDSystemService(
    HPID hpid,
    HTID htid,
    SSVC ssvc,
    LPVOID lpvData,
    DWORD cbData,
    LPDWORD lpcbReturned
    );




enum {
    dbmSoftMode,
    dbmHardMode
};

typedef DWORD DBM;

typedef struct _DBMI {
    HWND hWndFrame;
    HWND hWndMDIClient;
    HANDLE hAccelTable;
} DBMI;

XOSD
OSDAPI
OSDSetDebugMode(
    HPID hpid,
    DBM dbmService,
    LPVOID lpvData,
    DWORD cbData
    );


#define TL_ERROR_BUFFER_LENGTH  1024

XOSD
OSDAPI
OSDGetLastTLError(
        HTL             hTL,
        HPID    hpid,
        LPSTR   Buffer,
        ULONG   Length
        );

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
//    OSDebug notifications
//

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// Data passed with dbc messages
//

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _OD_
