/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    odtypes.h

Abstract:

Author:

    David J. Gilman (davegi) 05-Apr-1992

Environment:

    Win32, User Mode


--*/

#if ! defined _ODTYPES_
#define _ODTYPES_

#include "types.h"
#include "cvtypes.h"

#ifdef  __cplusplus
#pragma warning ( disable: 4200 )
extern "C" {
#endif


//  If the following definitions do not produce a 32 bit type on your
//  platform, please fix it.
//

typedef void FAR * HANDLE32;
typedef HANDLE32 FAR * LPHANDLE32;

#if !defined(DECLARE_HANDLE32)
#ifdef STRICT
#define DECLARE_HANDLE32(name) struct name##__32 { int unused; }; typedef struct name##__32 FAR *name
#else
#define DECLARE_HANDLE32(name) typedef HANDLE32 name
#endif
#endif
//

typedef void FAR * HANDLE32;
typedef HANDLE32 FAR * LPHANDLE32;

#if !defined(DECLARE_HANDLE32)
#ifdef STRICT
#define DECLARE_HANDLE32(name) struct name##__32 { int unused; }; typedef struct name##__32 FAR *name
#else
#define DECLARE_HANDLE32(name) typedef HANDLE32 name
#endif
#endif

#ifdef STRICT

    DECLARE_HANDLE32(HTL);
    DECLARE_HANDLE32(HEM);
    DECLARE_HANDLE32(HOSDFILE);

#else

    typedef HIND HTL;               // handle to a transport layer
    typedef HIND HEM;               // handle to an execution model
    typedef HIND HOSDFILE;

#endif

typedef HTL  FAR *LPHTL;
typedef HEM  FAR *LPHEM;
typedef HEMI FAR *LPHEMI;


typedef char FAR *  LSZ;

#define OSDAPI WINAPI
#define OSDAPIV WINAPIV


//
// Error status codes
//

#define DECL_XOSD(n,v,s) n = v,

enum {
#include "xosd.h"
};
typedef int XOSD;
typedef XOSD FAR *LPXOSD;

#undef DECL_XOSD



//
//     Address manipulation
//
typedef enum {
    adrCurrent,
    adrPC,
    adrBase,
    adrStack,
    adrData,
    adrTlsBase,
    adrBaseProlog
} ADR;



//
// Debugger callback types
//

typedef enum DBCT {     // debugger callback types
    dbctStop,           // debuggee has stopped -- no more dbc's will be sent
    dbctContinue,       // debuggee is continuing to run
    dbctMaybeContinue,  // debuggee may or may not continue, depending on other
                        //  information.  Interpretation is DBC-specific.
} DBCT;

//
// Debugger callbacks
//

#define DECL_DBC(name, fRequest, dbct)  dbc##name,


typedef enum {
        #include "dbc.h"
} _DBC;

typedef DWORD DBC;

#undef DECL_DBC

//
// callback for components to get and set workspace params
//
typedef BOOL (*LPGET_MODULE_INFO)(TCHAR[MAX_PATH], TCHAR[MAX_PATH]);



//
// Debugger services export table
//

typedef PVOID      (OSDAPI *  LPMHALLOC)          ( size_t );
typedef PVOID      (OSDAPI *  LPMHREALLOC)        ( LPVOID, size_t );
typedef VOID       (OSDAPI *  LPMHFREE)           ( LPVOID );

typedef HLLI       (OSDAPI *  LPLLINIT)           ( DWORD,
                                                    LLF,
                                                    LPFNKILLNODE,
                                                    LPFNFCMPNODE );
typedef HLLE       (OSDAPI *  LPLLCREATE)         ( HLLI );
typedef VOID       (OSDAPI *  LPLLADD)            ( HLLI, HLLE );
typedef VOID       (OSDAPI *  LPLLINSERT)         ( HLLI, HLLE, DWORD );
typedef BOOL       (OSDAPI *  LPLLDELETE)         ( HLLI, HLLE );
typedef HLLE       (OSDAPI *  LPLLNEXT)           ( HLLI, HLLE );
typedef DWORD      (OSDAPI *  LPLLDESTROY)        ( HLLI );
typedef HLLE       (OSDAPI *  LPLLFIND)           ( HLLI,
                                                    HLLE,
                                                    LPVOID,
                                                    DWORD );
typedef DWORD      (OSDAPI *  LPLLSIZE)           ( HLLI );
typedef PVOID      (OSDAPI *  LPLLLOCK)           ( HLLE );
typedef VOID       (OSDAPI *  LPLLUNLOCK)         ( HLLE );
typedef HLLE       (OSDAPI *  LPLLLAST)           ( HLLI );
typedef VOID       (OSDAPI *  LPLLADDHEAD)        ( HLLI, HLLE );
typedef BOOL       (OSDAPI *  LPLLREMOVE)         ( HLLI, HLLE );

typedef int        (OSDAPI *  LPLBASSERT)         ( LPSTR, LPSTR, DWORD);
typedef int        (OSDAPI *  LPLBQUIT)           ( DWORD );

// Forward declaration if shapi.h is not included
#ifndef SH_API
struct ODR;
typedef struct ODR * LPODR;
#endif

typedef LPSTR      (OSDAPI *  LPSHGETSYMBOL)      ( LPADDR  addr1,
                                                    LPADDR  addr2,
                                                    SHORT   sop, // should be SOP
                                                    LPODR   lpodr // should be LPODR
                                                  );
typedef BOOL       (OSDAPI * LPSHGETPUBLICADDR)   ( LPADDR, LSZ );

#ifdef NT_BUILD_ONLY
typedef LPSTR      (OSDAPI * LPSHADDRTOPUBLICNAME)(LPADDR, LPADDR);
#else
typedef LPSTR      (OSDAPI * LPSHADDRTOPUBLICNAME)(LPADDR);
#endif

typedef LPVOID     (OSDAPI * LPSHGETDEBUGDATA)    ( HIND );

typedef PVOID      (OSDAPI *  LPSHLPGSNGETTABLE)  ( HIND );

#ifdef NT_BUILD_ONLY
typedef BOOL       (OSDAPI *  LPSHWANTSYMBOLS)    ( HIND );
#endif

typedef DWORD      (OSDAPI *  LPDHGETNUMBER)      ( LPSTR, LPLONG );
typedef MPT        (OSDAPI *  LPGETTARGETPROCESSOR)( HPID );



typedef struct {
    LPMHALLOC               lpfnMHAlloc;
    LPMHREALLOC             lpfnMHRealloc;
    LPMHFREE                lpfnMHFree;

    LPLLINIT                lpfnLLInit;
    LPLLCREATE              lpfnLLCreate;
    LPLLADD                 lpfnLLAdd;
    LPLLINSERT              lpfnLLInsert;
    LPLLDELETE              lpfnLLDelete;
    LPLLNEXT                lpfnLLNext;
    LPLLDESTROY             lpfnLLDestroy;
    LPLLFIND                lpfnLLFind;
    LPLLSIZE                lpfnLLSize;

    LPLLLOCK                lpfnLLLock;
    LPLLUNLOCK              lpfnLLUnlock;
    LPLLLAST                lpfnLLLast;
    LPLLADDHEAD             lpfnLLAddHead;
    LPLLREMOVE              lpfnLLRemove;

    LPLBASSERT              lpfnLBAssert;
    LPLBQUIT                lpfnLBQuit;

    LPSHGETSYMBOL           lpfnSHGetSymbol;
    LPSHGETPUBLICADDR       lpfnSHGetPublicAddr;

    LPSHADDRTOPUBLICNAME    lpfnSHAddrToPublicName;
    LPSHGETDEBUGDATA        lpfnSHGetDebugData;

    LPSHLPGSNGETTABLE       lpfnSHLpGSNGetTable;

#ifdef NT_BUILD_ONLY
    LPSHWANTSYMBOLS         lpfnSHWantSymbols;
#endif

    LPDHGETNUMBER           lpfnDHGetNumber;
    LPGETTARGETPROCESSOR    lpfnGetTargetProcessor;
    //LPGETSETPROFILEPROC     lpfnGetSet;

} DBF;  // DeBugger callback Functions

typedef DBF FAR *LPDBF;

// Thread State bits
typedef enum {
   tstRunnable   = 0,        // New thread, has not run yet.
   tstStopped    = 1,        // Thread is at a debug event
   tstRunning    = 2,        // Thread is currently running/runnable
   tstExiting    = 3,        // Thread is in the process of exiting
   tstDead       = 4,        // Thread is no longer schedulable
   tstRunMask    = 0xf,

   tstExcept1st  = 0x10,     // Thread is at first chance exception
   tstExcept2nd  = 0x20,     // Thread is at second change exception
   tstRip        = 0x30,     // Thread is in a RIP state
   tstExceptionMask = 0xf0,

   tstFrozen     = 0x100,    // Thread has been frozen by Debugger
   tstSuspended  = 0x200,    // Thread has been frozen by Other
   tstBlocked    = 0x300,    // Thread is blocked on something
                             // (i.e. a semaphore)
   tstSuspendMask= 0xf00,

   tstCritSec    = 0x1000,   // Thread is currently in a critical
                             // section.
   tstOtherMask  = 0xf000
} TSTATE;


// Process state bits
typedef enum {
    pstRunning = 0,
    pstStopped = 1,
    pstExited  = 2,
    pstDead    = 3
} PSTATE;


//
// Debug metrics.
//

enum _MTRC {
    mtrcProcessorType,
    mtrcProcessorLevel,
    mtrcEndian,
    mtrcThreads,
    mtrcCRegs,
    mtrcCFlags,
    mtrcExtRegs,
    mtrcExtFP,
    mtrcExtMMU,
    mtrcPidSize,
    mtrcTidSize,
    mtrcExceptionHandling,
    mtrcAssembler,
    mtrcAsync,
    mtrcAsyncStop,
    mtrcBreakPoints,
    mtrcReturnStep,
    mtrcShowDebuggee,
    mtrcHardSoftMode,
    mtrcRemote,
    mtrcOleRpc,         // Supports OLE Remote Procedure Call debugging?
    mtrcNativeDebugger, // Supports low-level debugging (eg MacsBug)
    mtrcOSVersion,
    mtrcMultInstances,
    mtrcTidValue // HACK for IDE
};
typedef DWORD MTRC;


enum _BPTS {
    bptsExec     = 0x0001,
    bptsDataC    = 0x0002,
    bptsDataW    = 0x0004,
    bptsDataR    = 0x0008,
    bptsRegC     = 0x0010,
    bptsRegW     = 0x0020,
    bptsRegR     = 0x0040,
    bptsMessage  = 0x0080,
    bptsMClass   = 0x0100,
    bptsRange    = 0x0200,
    bptsDataExec = 0x0400,
    bptsPrologOk = 0x0800   // BP in prolog is harmless
};
typedef DWORD BPTS;

enum {
    asyncRun    = 0x0001,   // Debuggee runs asynchronously from debugger
    asyncMem    = 0x0002,   // Can read/write memory asynchronously
    asyncStop   = 0x0004,   // Can stop/restart debuggee asynchronously
    asyncBP     = 0x0008,   // Can change breakpoints asynchronously
    asyncKill   = 0x0010,   // Can kill child asynchronously
    asyncWP     = 0x0020,   // Can change watchpoints asyncronously
    asyncSpawn  = 0x0040,   // Can spawn another process asynchronously
};
typedef DWORD ASYNC;

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

//
// Types which we can't seem to escape
//

typedef struct {
    BYTE b[10];
} REAL10;
typedef REAL10 FAR * LPREAL10;



//
// Packet supplied to shell by dbcCanStep
//
//
typedef struct _CANSTEP {
    DWORD   Flags;
    UOFF32  PrologOffset;
} CANSTEP;

typedef CANSTEP FAR *LPCANSTEP;

#define CANSTEP_NO      0x00000000
#define CANSTEP_YES     0x00000001
#define CANSTEP_THUNK   0x00000002

/*
 *  This structure is used in communicating a stop event to the EM.  It
 *      contains the most basic of information about the stopped thread.
 *      A "frame" pointer, a program counter and bits describing the type
 *      of segment stopped in.
 */

typedef struct _BPR {
    DWORD64     qwNotify;       /* Tag to identify BP #          */
    UOFFSET     offEIP;         /* Program Counter offset        */
    UOFFSET     offEBP;         /* Frame pointer offset          */
    UOFFSET     offESP;         /* Stack pointer offset          */
    SEGMENT     segCS;          /* Program counter seletor       */
    SEGMENT     segSS;          /* Frame & Stack pointer offset  */
    DWORD       fFlat:1;
    DWORD       fOff32:1;
    DWORD       fReal:1;
} BPR; // BreakPoint Return

typedef BPR FAR *LPBPR;


//
// These are the actions which the debugger may take
// in response to an exception raised in the debuggee.
//
typedef enum _EXCEPTION_FILTER_DEFAULT {
    efdIgnore,
    efdNotify,
    efdCommand,
    efdStop
} EXCEPTION_FILTER_DEFAULT;
typedef EXCEPTION_FILTER_DEFAULT * LPEXCEPTION_FILTER_DEFAULT;

//
// Exception reporting packet
//
//
typedef struct _EPR {
    BPR   bpr;
    EXCEPTION_FILTER_DEFAULT efd;
    DWORD dwFirstChance;
    DWORD ExceptionCode;
    DWORD ExceptionFlags;
    DWORD NumberParameters;
    DWORD64 ExceptionInformation[];
} EPR; // Exception Return

typedef EPR FAR *LPEPR;

//
// Structure passed with dbcInfoAvail
//
typedef struct _INFOAVAIL {
    DWORD   fReply;
    DWORD   fUniCode;
    BYTE    buffer[];   // the string
} INFOAVAIL; // InfoAvail return
typedef INFOAVAIL FAR * LPINFOAVAIL;

//
// Structure returned via dbcMsg*
//
typedef struct _MSGI {
    DWORD dwMessage;
    DWORD dwMask;
    ADDR  addr;
    CHAR  rgch [ ];
} MSGI;     // MeSsaGe Info
typedef MSGI FAR *LPMSGI;

//
// function information; derived from
// FPO, PDATA or whatever else there may be.
//
// This will contain information pertaining to the block
// containing the address specified in OSDGetFunctionInformation().
// It may be a nested block; it need not be an entire function.
//
typedef struct _FUNCTION_INFORMATION {
    ADDR    AddrStart;          // fixedup addresses
    ADDR    AddrPrologEnd;
    ADDR    AddrEnd;            // end of function
    //ADDR    FilterAddress;      // Address of exception filter
} FUNCTION_INFORMATION, *LPFUNCTION_INFORMATION;

#ifdef __cplusplus
} // extern "C" {
#endif

#endif // _ODTYPES_
