/**** EMDM.HMD - Common structures for Win32/NT EM and DM               ****
 *                                                                         *
 *                                                                         *
 *  Copyright <C> 1990, Microsoft Corp                                     *
 *                                                                         *
 *  Created: November 17, 1990 by David W. Gray                            *
 *                                                                         *
 *  Purpose:                                                               *
 *                                                                         *
 *      This file defines the types, enums, and constants that are common  *
 *      for all execution models, both debugger and debuggee end.          *
 *                                                                         *
 ***************************************************************************/

#ifndef _EMDM
#define _EMDM

#ifdef __cplusplus
extern "C" {
#endif

//
// This is included to define a NONVOLATILE_CONTEXT_POINTERS structure
// of the appropriate size.  The goal is to keep any machine-specific
// reference out of emdp.c here, but we need to know how much data to
// transfer to the thread context on the different architectures.
//

#include "ctxptrs.h"

#define MAXCACHE 16
#define CACHESIZE 0x100

#ifndef try
#define try __try
#endif
#ifndef except
#define except __except
#endif
#ifndef finally
#define finally __finally
#endif

typedef DWORD MTE;

#ifdef TARGET32
#define MAXBIGSEGS      3
#endif


typedef DWORD PID;
typedef DWORD TID;


typedef enum {
    dmfRemoteDied = -1,             /* debugger quit */
    dmfCommError  = -2,             /* transport layer error */

    dmfNull = 0,

    dmfInit,
    dmfUnInit,
    dmfRemoteQuit,
    dmfSelect,
    dmfConnect,

    dmfCreatePid,
    dmfDestroyPid,

    dmfSetPath,
    dmfSpawnOrphan,
    dmfProgLoad,
    dmfProgFree,
    dmfDebugActive,
    dmfTerm,

    dmfGo,
    dmfStop,
    dmfFreeze,
    dmfResume,

    dmfSingleStep,
    dmfReturnStep,
    dmfNonLocalGoto,
    dmfRangeStep,
    dmfGoToReturn,

    dmfBreakpoint,
    dmfGetExceptionState,
    dmfSetExceptionState,

    dmfReadMem,
    dmfReadReg,
    dmfReadFrameReg,
    dmfWriteMem,
    dmfWriteReg,
    dmfWriteFrameReg,
    dmfGetFP,
    dmfSetFP,
    dmfThreadStatus,
    dmfProcessStatus,
    dmfQueryTlsBase,
    dmfQuerySelector,
    dmfVirtualQuery,
    dmfReadRegEx,
    dmfWriteRegEx,
    dmfGetSections,

    dmfInit32SegValues,
    dmfSelLim,

    dmfSetMulti,
    dmfClearMulti,
    dmfDebugger,
    dmfSync,
    dmfSystemService,
    dmfGetPrompt,
    dmfSendChar,
    dmfGetDmInfo,

    dmfSetupExecute,
    dmfStartExecute,
    dmfCleanUpExecute,

    dmfRemoteUtility,       // for the mfile utility
    dmfGetTimeStamp,

    dmfNewSymbolsLoaded,    // Informs the DM that new symbols have been reloaded

    dmfLast

} _DMF;

typedef LONG DMF;


typedef struct _DM_MSG {
    union {
        XOSD  xosdRet;
        DWORDLONG Alignment;
    };
    BYTE  rgb[1];
} DM_MSG;
typedef DM_MSG *LPDM_MSG;

#define iflgMax 12

typedef struct _RTRNSTP {
   EXOP exop;
   ADDR addrRA;         // Address to return to
   ADDR addrStack;       // Address of current SP.
} RTRNSTP; // ReTuRN STeP packet
typedef RTRNSTP *LPRTRNSTP;

#pragma pack(4)

typedef struct _RST {
    EXOP    exop;
#ifdef TARGET32
    UOFFSET     offStart;
    UOFFSET     offEnd;
    UOFFSET     offPC;
#else
    ADDR addrStart;
    ADDR addrEnd;
    ADDR addrCSIP;
#endif
} RST; // Range STep Packet

typedef RST *LPRST;

typedef struct _TCR {
    TID             tid;
    UOFFSET uoffTEB;
} TCR;  // Thread Create Return
typedef TCR *LPTCR;

typedef struct _SETPTH {
    BOOL Set;
    TCHAR Path[1];
} SETPTH;

#pragma pack()

//
// DM Misc info structure.
//
// Some of these correspond to the debug metrics exposed by OSDebug.
// These cover the differences between user and kernel mode, Win32,
// Win32s and Win32c, maybe Cairo, whatever other DMs might be handled
// by the Win32 EM.
//
typedef struct _PROCESSOR {
    MPT Type;
    DWORD Level;
    END Endian;
} PROCESSOR, * LPPROCESSOR;

typedef struct _DMINFO {
    ASYNC mAsync;         // Mask of async capabilities
    DWORD fHasThreads:1;  //
    DWORD fReturnStep:1;  // step out of function?
    DWORD fRemote:1;      // target is not debugger host
    DWORD fAlwaysFlat:1;  // never use 16:16 for this target
    DWORD fHasReload:1;   // supports symbol reload
    DWORD fNonLocalGoto:1; // supports NLG
    DWORD fDMInfoCacheable:1;   // metaflag for EM
    DWORD fKernelMode:1;  //
    DWORD fPtr64:1;       // 64 bit pointers
    //DWORD fIller:23
    DWORD cbSpecialRegs;  // size of private regs struct for dmfGetRegsEx
    BPTS Breakpoints;     // OSDebug breakpoints supported
    PROCESSOR Processor;
    WORD MajorVersion;    // OS Version of target
    WORD MinorVersion;    //
} DMINFO;
typedef DMINFO * LPDMINFO;

typedef struct _GOP {
    USHORT fBpt;
    USHORT fAllThreads;
    ADDR addr;
} GOP; // Go until this address

typedef GOP *PGOP;
typedef GOP *LPGOP;

typedef struct _SBP {
    HPID    id;
    BOOL    fAddr;
    ADDR    addr;
    DWORD   Size;
} SBP;

typedef SBP * LPSBP;

typedef struct _EHP {
    DWORD iException;
    BOOL  fHandle;
} EHP; // Exception Handled Packet
typedef EHP *LPEHP;

typedef struct _RSR {
    WORD    segCS;
    UOFFSET offIP;
    WORD    segSS;
    UOFFSET offBP;
    WORD    segCSNext;
    UOFFSET offIPNext;
} RSR; // Range Step Return
typedef RSR *LPRSR;

typedef struct _OBJD {
    UOFFSET     offset;
    DWORD       cb;
    WORD        wSel;
    WORD        wPad;
} OBJD, *LPOBJD;


#if defined(TARGMACPPC)
typedef struct _MODULELOAD {
    UOFFSET         uoffImageBase;
    DWORD           cbImageLength;
    DWORD           dwTimeStamp;
    LONG            cobj;
    OBJD            rgobjd[];
} MODULELOAD;

#elif defined(TARGJAVA)

typedef struct _MODULELOAD {
    IUnknown        *pRemoteClassField;
    DWORD            dwTimeStamp;
} MODULELOAD;

#else

typedef struct _MODULELOAD {
    UOFFSET         lpBaseOfDll;
    DWORD           dwSizeOfDll;
    LONG            cobj;
    WORD            mte;
    SEGMENT         StartingSegment;
    SEGMENT         CSSel;
    SEGMENT         DSSel;
    UOFFSET         uoffDataBase;
    UOFFSET         uoffiTls;       // linear address in process of this module's tlsindex
    DWORD           isecTLS;        // index of .tls section
    DWORD           iTls;           // tls index, retrieved at initial breakpoint time
    DWORD           fRealMode:1;
    DWORD           fFlatMode:1;
    DWORD           fOffset32:1;
    DWORD           fThreadIsStopped:1;
    OBJD            rgobjd[];
} MODULELOAD;
#endif

typedef MODULELOAD *LPMODULELOAD;

#ifndef TARGJAVA
typedef struct _RWP {
    DWORD cb;
    ADDR addr;
    union {
        DWORDLONG Alignment;
        BYTE rgb[1];
    };
} RWP; // Read Write Packet
#else
typedef struct _RWP {
    WORD cb;
    WORD pad;
    ADDR addr;
    BYTE rgb[];
} RWP; // Read Write Packet
#endif // TARGJAVA

typedef RWP *PRWP;
typedef RWP *LPRWP;

typedef struct _NPP {
    PID     pid;
    BOOL    fReallyNew;
} NPP;  // New Process Packet, used with dbcNewProc.
        // See od.h for description of fReallyNew.
typedef NPP * LPNPP;

typedef struct _WPP {
    ADDR addr;
    WORD cb;
} WPP; // Watch Point Packet
typedef WPP *LPWPP;

#if defined( TARGMAC68K )
typedef struct _SLI {
    DWORD dwBaseAddr;
    DWORD fExecute;
    short sRezID;
    unsigned char szName[];
} SLI;  // Segment Load Info
#else
typedef struct _SLI {
    WORD        wSelector;
    WORD        wSegNo;
    WORD        mte;
} SLI, * LPSLI;
#endif

typedef SLI *LPSLI;

// Exception command packet
typedef struct _EXCMD {
   EXCEPTION_CONTROL exc;
   EXCEPTION_DESCRIPTION exd;
} EXCMD;
typedef EXCMD * LPEXCMD;

typedef struct _EXHDLR {
    DWORD count;
    ADDR addr[];
} EXHDLR;
typedef EXHDLR * LPEXHDLR;

// The DBCEs always come back in an RTP structure, which has additional
// info.  The comments on the DBCEs below refer to the other fields of
// the RTP structure.
typedef enum {
    dbceAssignPID = dbcMax,     // Tell the EM what PID is associated with
                                // a given HPID.  At offset 0 of rtp.rgbVar[]
                                // is the PID.
    dbceLoadBigSegTbl,          // ??
    dbceCheckBpt,               // Find out if EM wants us to single-step
                                // over a specified breakpoint.  Upon return,
                                // rgbVar[0] is fStop to stop at this
                                // breakpoint; if fStop is FALSE, then
                                // rgbVar[1] is the byte with which to
                                // overwrite the INT 3.
    dbceInstructionLen,         // Ask the em how long the instruction is.
                                // rgbVar contains the cs:ip
    dbceSegLoad,                // WOW just loaded a segment
    dbceSegMove,                // WOW just moved a segment
    dbceModFree16,              // Unload of a 16-bit DLL
    dbceModFree32,              // Unload of a 32-bit DLL
    dbceGetOffsetFromSymbol,    // like it sez
    dbceEnableCache,
    dbceGetSymbolFromOffset,
    dbceGetMessageMask,
    dbceExceptionDuringStep,    // Ask count prefix array of exception handlers
    dbceGetFunctionInformation, // See OSDGetFunctionInformation
    dbceMax
} _DBCE;
typedef LONG DBCE;

//  it is important that the rgbVar fields be aligned on a DWORDLONG boundary
#ifndef TARGJAVA
typedef struct _DBB {
    DMF  dmf;
    union {
        HPID hpid;
        DWORDLONG Alignment1;
    };
    union {
        HTID htid;
        DWORDLONG Alignment2;
    };
    union {
        BYTE rgbVar[1];
        DWORDLONG Alignment;
    };
} DBB;
#else
typedef struct _DBB {
    DMF  dmf;
    WORD dummy;
    HPID hpid;
    HTID htid;
    BYTE rgbVar[];
} DBB;
#endif

typedef DBB *LPDBB;

#ifndef TARGJAVA
typedef struct _RTP {
    DBC  dbc;                   // a DBC or a DBCE
    DWORD cb;                    // the length of rgbVar
    union {
        HPID hpid;
        DWORDLONG Alignment1;
    };
    union {
        HTID htid;
        DWORDLONG Alignment2;
    };
    union {
        BYTE rgbVar[1];         // additional information - see the
        DWORDLONG Alignment;    // definitions of the DBCE and DBC codes
    };
} RTP;
#else
typedef struct _RTP {
    DBC  dbc;        // a DBC or a DBCE
    WORD pad0;       // align to 4 bytes
    HPID hpid;
    HTID htid;
    WORD cb;         // the length of rgbVar
    BYTE rgbVar[];   // additional information - see the
                     // definitions of the DBCE and DBC codes
} RTP;
#endif // TARGJAVA

typedef RTP *PRTP;
typedef RTP *LPRTP;

#define lpregDbb(dbb) ( (LPREG) &dbb )
#define lpfprDbb(dbb) ( (LPFPR) &dbb )
#define lszDbb(dbb)   ( (LSZ)   &dbb )

#define addrDbb(dbb)  (*( (LPADDR) &dbb ))
#define stpDbb(dbb)   (*( (LPSTP)  &dbb ))
#define rstDbb(dbb)   (*( (LPRST)  &dbb ))
#define gopDbb(dbb)   (*( (LPGOP)  &dbb ))
#define tstDbb(dbb)   (*( (LPTST)  &dbb ))
#define pstDbb(dbb)   (*( (LPF)    &dbb ))
#define rwpDbb(dbb)   (*( (LPRWP)  &dbb ))
#define fDbb(dbb)     (*( (LPF)    &dbb ))



/****************************************************************************
 *                                                                          *
 * Packets returned from the debuggee execution model to the debugger       *
 *  execution model.                                                        *
 *                                                                          *
 ****************************************************************************/


typedef struct _FRAME_INFO {
    CONTEXT frameRegs;
    KNONVOLATILE_CONTEXT_POINTERS frameRegPtrs;
} FRAME_INFO, * PFRAME_INFO;

typedef struct _NLG {
    HEMI    hemi;
    BOOL    fEnable;
    ADDR    addrNLGDispatch;
    ADDR    addrNLGDestination;
    ADDR    addrNLGReturn;
    ADDR    addrNLGReturn2;
} NLG;
typedef NLG * PNLG;
typedef NLG * LPNLG;

#ifdef __cplusplus
} // extern "C" {
#endif

#endif  // _EMDM
