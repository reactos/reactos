/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    odp.h

Abstract:

    This is part of OSDebug version 4.

    These are types and data which are private to OSDebug and the
    components below it: TL, EM and DM.

Author:

    Kent D. Forschmiedt (kentf)

Environment:

    Win32, User Mode

--*/

#ifndef _ODP_
#define _ODP_

#ifdef  __cplusplus
#pragma warning ( disable: 4200 )
extern "C" {
#endif

#define DECL_EMF(emfName)       emf##emfName,

typedef enum _EMF {
#include "emf.h"
} EMF;

#undef DECL_EMF

typedef struct _DBCPK {
   DBC     dbc;
   HPID    hpid;
   HTID    htid;
   DWORD64  wValue;
   DWORD64  lValue;
} DBCPK; // a dbc package
typedef DBCPK FAR *LPDBCPK;


typedef enum {
    dbcoCreateThread = dbcMax,
    dbcoNewProc,

    dbcoMax
} DBCO;  // Debug CallBacks Osdebug specific


//
// callbacks the TL uses to communicate with shell -- stub or client.
//
typedef enum {
    tlcbDisconnect,     // Transport layer was disconnected normally
    tlcbMax
} _TLCB;
typedef DWORD TLCB;


typedef XOSD (*TLFUNC_ODP) ( TLF, HPID, DWORD64, DWORD64 );
typedef XOSD (*EMFUNC_ODP) ( EMF, HPID, HTID, DWORD64, DWORD64 );
typedef XOSD (*TLFUNCTYPE) ( TLF, HPID, DWORD64, DWORD64 );
typedef XOSD (*DMTLFUNCTYPE) ( TLF, HPID, DWORD64, DWORD64 );
typedef XOSD (*TLCALLBACKTYPE) (HPID, DWORD64, DWORD64 );
typedef XOSD (*LPDMINIT) ( DMTLFUNCTYPE, LPVOID );
typedef VOID (*LPDMFUNC) ( DWORD, LPBYTE );
typedef DWORD (*LPDMDLLINIT) ( LPDBF );
typedef XOSD (*LPUISERVERCB) (TLCB, HPID, HTID, DWORD64, DWORD64 );



DECLARE_HANDLE32(HEMP);

typedef struct _THREADINFO {
    HPID hpid;
    HLLI llemp;
} THREADINFO;
typedef THREADINFO *LPTHREADINFO;   // Thread information

typedef struct _PROCESSINFO {
    HTL     htl;
    HEMP    hempNative;
    HLLI    llemp;
    DWORD   fNative;
    DWORD   lastmodel;
    LPFNSVC lpfnsvcCC;
    HLLI    lltid;
} PROCESSINFO;
typedef PROCESSINFO *LPPROCESSINFO;   // Process information

typedef struct _EMS {
    EMFUNC_ODP  emfunc;
    EMTYPE      emtype;
    HLLI        llhpid;
    DWORD       model;
} EMS; // Execution Model Structure - per EM
typedef EMS *LPEMS;

typedef struct _EMP {
    HEM         hem;
    EMFUNC_ODP  emfunc;
    EMTYPE      emtype;
    DWORD       model;
} EMP; // Execution Model Structure - per process
typedef EMP *LPEMP;

typedef struct _TLS {
    TLFUNC_ODP  tlfunc;
} TLS; // Transport Layer Structure
typedef TLS *LPTL;

typedef struct _OSDFILE {
    HPID  hpid;
    DWORD dwPrivateData;    // EM's representation of the file
} OSDFILE;
typedef OSDFILE * LPOSDFILE;

//
// Compare Address Struct
//
typedef struct _CAS {
    LPADDR lpaddr1;
    LPADDR lpaddr2;
    PDWORD lpResult;
} CAS;
typedef CAS * LPCAS;

//
// Range Step Struct
//
typedef struct _RSS {
    LPADDR lpaddrMin;
    LPADDR lpaddrMax;
    LPEXOP lpExop;
} RSS;
typedef RSS * LPRSS;

//
// read memory struct
//
typedef struct _RWMS {
    LPADDR lpaddr;
    LPVOID lpbBuffer;
    DWORD cbBuffer;
    LPDWORD lpcb;
} RWMS;
typedef RWMS * LPRWMS;

//
// Get Object Length struct
//
typedef struct _GOL {
    LPADDR lpaddr;
    LPUOFFSET lplBase;
    LPUOFFSET lplLen;
} GOL;
typedef GOL * LPGOL;

//
// Get Function Information Structure
//
typedef struct _GFI {
    LPADDR lpaddr;
    LPFUNCTION_INFORMATION lpFunctionInformation;
} GFI;
typedef GFI * LPGFI;

//
// Get Previous Instruction Structure
//
typedef struct _GPIS {
    LPADDR lpaddr;
    LPUOFFSET lpuoffset;
} GPIS;
typedef GPIS * LPGPIS;

//
// Set Debug Mode Structure
//
typedef struct _SDMS {
    DBM dbmService;
    LPVOID lpvData;
    DWORD cbData;
} SDMS;
typedef SDMS * LPSDMS;

typedef struct _SSS {
    SSVC ssvc;
    DWORD cbSend;
    DWORD cbReturned;
    BYTE rgbData[];
} SSS;
typedef SSS * LPSSS;

//
// The following structure is used by the emfSetupExecute message
//
typedef struct _EXECUTE_STRUCT {
    ADDR        addr;           /* Starting address for function        */
    HIND        hindDm;         /* This is the DMs handle               */
    HDEP        lphdep;         /* Handle of save area                  */
    DWORD       fIgnoreEvents:1; /* Ignore events coming back?          */
    DWORD       fFar:1;         /* Is the function a _far routine       */
} EXECUTE_STRUCT;
typedef EXECUTE_STRUCT * LPEXECUTE_STRUCT;

//
// Load DM packet, used by TL
//
typedef struct _LOADDMSTRUCT {
    LPTSTR lpDmName;
    LPTSTR lpDmParams;
} LOADDMSTRUCT, * LPLOADDMSTRUCT;


void ODPDKill  ( LPVOID );

void EMKill    ( LPVOID );
int  EMHpidCmp ( LPVOID, LPVOID, LONG );
void EMPKill   ( LPVOID );

void TLKill    ( LPVOID );

void NullKill  ( LPVOID );
int  NullComp  ( LPVOID, LPVOID, LONG );

typedef XOSD (*FNCALLBACKDB) ( DBC, HPID, HTID, DWORD, DWORD64, DWORD64 );
typedef XOSD (*FNCALLBACKTL) ( TLF, HPID, DWORD64, DWORD64 );
typedef XOSD (*FNCALLBACKNT) ( EMF, HPID, HTID, DWORD64, DWORD64 );
typedef XOSD (*FNCALLBACKEM) ( EMF, HPID, HTID, DWORD, DWORD64, DWORD64 );

typedef struct _EMCB {
    FNCALLBACKDB lpfnCallBackDB;
    FNCALLBACKTL lpfnCallBackTL;
    FNCALLBACKNT lpfnCallBackNT;
    FNCALLBACKEM lpfnCallBackEM;
} EMCB; // Execution Model CallBacks
typedef EMCB *LPEMCB;

typedef struct _REMI {
    HEMI    hemi;
    LPTSTR  lsz;
} REMI;     // Register EMI structure
typedef REMI * LPREMI;

// Packet used by OSDSpawnOrphan
typedef struct _SOS {
    DWORD   dwChildFlags;
    LPTSTR  lszRemoteExe;    // name of remote exe
    LPTSTR  lszCmdLine;      // command line
    LPTSTR  lszRemoteDir;    // initial dir of debuggee
    LPSPAWNORPHAN    lpso;   // info to return about the spawn.
} SOS, *LPSOS;        // Spawn Orphan Structure

// packet used by OSDProgramLoad
// Doesn't use SOS.lpso
typedef SOS PRL;
typedef PRL *   LPPRL;


//
//    Structures used by GetTimeStamp ()
//

typedef struct _TCS {
    LPTSTR    ImageName;
    ULONG    TimeStamp;
    ULONG    CheckSum;
} TCS;

typedef struct TCSR {
    ULONG    TimeStamp;
    ULONG    CheckSum;
} TCSR;

typedef TCS* LPTCS;
typedef TCSR* LPTCSR;

#define MHAlloc(x)   ((*lpdbf->lpfnMHAlloc)(x))
#define MHRealloc(a,b) ((*lpdbf->lpfnMHRealloc)(a,b))
#define MHFree(y)    ((*lpdbf->lpfnMHFree)(y))

#define LLInit    (*lpdbf->lpfnLLInit)
#define LLCreate  (*lpdbf->lpfnLLCreate)
#define LLAdd     (*lpdbf->lpfnLLAdd)
#define LLInsert  (*lpdbf->lpfnLLInsert)
#define LLDelete  (*lpdbf->lpfnLLDelete)
#define LLNext    (*lpdbf->lpfnLLNext)
#define LLDestroy (*lpdbf->lpfnLLDestroy)
#define LLFind    (*lpdbf->lpfnLLFind)
#define LLSize    (*lpdbf->lpfnLLSize)
#define LLLock    (*lpdbf->lpfnLLLock)
#define LLUnlock  (*lpdbf->lpfnLLUnlock)
#define LLLast    (*lpdbf->lpfnLLLast)
#define LLAddHead (*lpdbf->lpfnLLAddHead)
#define LLRemove  (*lpdbf->lpfnLLRemove)

#define LBAssert  (*lpdbf->lpfnLBAssert)
#define DHGetNumber (*lpdbf->lpfnDHGetNumber)

#define SHLocateSymbolFile (*lpdbf->lpfnSHLocateSymbolFile)
#define SHGetSymbol        (*lpdbf->lpfnSHGetSymbol)
#define SHLpGSNGetTable    (*lpdbf->lpfnSHLpGSNGetTable)
#define SHFindSymbol       (*lpdbf->lpfnSHFindSymbol)

#define SHGetDebugData     (*lpdbf->lpfnSHGetDebugData)
#define SHGetPublicAddr    (*lpdbf->lpfnSHGetPublicAddr)
#define SHAddrToPublicName (*lpdbf->lpfnSHAddrToPublicName)
#define GetTargetProcessor (*lpdbf->lpfnGetTargetProcessor)
#ifdef NT_BUILD_ONLY
#define SHWantSymbols(H)      (*lpdbf->lpfnSHWantSymbols)(H)
#else
#define SHWantSymbols(H)      (0)
#endif

#ifdef  __cplusplus
}   // extern "C"
#endif

#endif // _ODP_
