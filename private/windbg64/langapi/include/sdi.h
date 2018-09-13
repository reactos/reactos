//
// SQL Debug Information API
// Copyright (C) 1995, Microsoft Corp.	All Rights Reserved.
//

#include <stddef.h>

#ifndef __SDI_INCLUDED__
#define __SDI_INCLUDED__

#pragma pack(push, enter_SDI)
#pragma pack(2)

// Basic typedefs
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned long ULONG;
typedef unsigned short USHORT;

typedef ULONG   INTV;       // interface version number
typedef ULONG   IMPV;       // implementation version number
typedef ULONG   SPID;       // connection id
typedef ULONG   PRID;       // stored procedure id
typedef USHORT  IDX;        // statement index
typedef USHORT  NLVL;       // nesting level
typedef USHORT  OFF;        // offset into stored proc/batch
typedef ULONG   PID;        // process id
typedef ULONG   THID;        // thread id

// Interface Version Number
enum {	SDIIntv = 951027, SDIIntv2 = 961025 };

// Interesting values
enum {	
		cbMchNmMax   = 32,	// maximum m/c name length (sql has a limit of 30)
		cbSqlSrvrMax = 32	// maximum sql server name length
};

// dll/entrypoint names
#define SDIDLL_ENTRYPOINT   "SDIInit"

#ifndef EXTERN_C
#if __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif
#endif

// init API
typedef struct _SQLFNTBL SQLFNTBL;
typedef struct _DBGFNTBL DBGFNTBL;
typedef long SDI_EC;            // error code

BOOL _stdcall SDIInit (SQLFNTBL *psqlfntbl, INTV intvsql, DBGFNTBL **ppdbgfntbl, INTV *pintvdbg);

typedef BOOL (_stdcall *pfnSDIInit)(SQLFNTBL *psqlfntbl, INTV intvsql, DBGFNTBL **ppdbgfntbl, INTV *pintvdbg);

// execution control API
typedef BOOL (_stdcall *pfnSDINewSPID)(SPID spid, char szSqlServer[cbSqlSrvrMax], char szMachine[cbMchNmMax], void *pvDbgData,ULONG cbDbgData,PID pid, PID dbgpid);
typedef BOOL (_stdcall *pfnSDINewSP)  (SPID spid, char *DBName, USHORT DBLen, PRID prid, NLVL nlvl);
typedef BOOL (_stdcall *pfnSDIStep) (SPID spid, PRID prid, IDX idx, OFF off, NLVL nlvl);
typedef BOOL (_stdcall *pfnSDINewBatch)(SPID spid, char *CmdBuf, ULONG cbCmdBuf, NLVL nlvl);
typedef BOOL (_stdcall *pfnSDIPop)(SPID spid, NLVL nlvl);
typedef BOOL (_stdcall *pfnSDICloseConnection)(SPID spid, ULONG sqlerror, SDI_EC sdierror);
typedef BOOL (_stdcall *pfnSDISpidContext)(SPID spid, PID pid, THID tid);

// data handling API
typedef enum {symGlobals, symLocals, symParams} SYMS;       // class of syms

typedef enum {                                              // sym types
    stInvalid,  
    stBIT,
    stTIMESTAMP,
    stTINYINT,
    stSMALLINT,
    stINT,
    stREAL,
    stFLOAT,
    stNUMERIC,
    stDECIMAL,
    stCHAR,
    stVARCHAR,
    stBINARY,
    stVARBINARY,
    stSMALLMONEY,
    stMONEY,
    stSMALLDATETIME,
    stDATETIME,
    stTEXT,
    stIMAGE,
	stGUID,
	stLARGEINT,
    stLast
} SYM_TYPE;                 

typedef struct _SYMINFO {
    SYM_TYPE    st;                 // symbol type
    void        *pv;                // ptr to symbol value
    USHORT      cb;                 // length 
    USHORT      cbName;             // length of name
    char        *Name;              // symbol name
    BYTE        cbPrec;             // precision info
    BYTE        cbScale;            // scale info
} SYMINFO, *PSYMINFO;

typedef BOOL (_stdcall *pfnSDIGetSym) (SPID spid, SYMS syms, PSYMINFO *prgsyminfo, USHORT *pcsym, NLVL nlvl);
typedef BOOL (_stdcall *pfnSDISetSym) (SPID spid, SYMS syms, PSYMINFO psyminfo, NLVL nlvl);

// version checking API

// error handling/shutdown API
typedef BOOL (_stdcall *pfnSDIDbgOff)(SPID);
typedef BOOL (_stdcall *pfnSDIError) (SPID spid, char *DBName, USHORT DBLen, PRID prid, IDX idx, OFF off, long numErr, char *szErr, ULONG cbErr);

enum SDIErrors {                // possible error codes
    SDI_OK,                     // looking good
    SDI_USAGE,                  // invalid paramter etc. should never happen
    SDI_VERSION,                // version mismatch; cannot proceed
    SDI_OUT_OF_MEMORY,          // out of memory
    SDI_SYM_NOTFOUND,           // invalid sym name
    SDI_INVALID_SYMTYPE,        // invalid sym type
    SDI_INVALID_SYMVAL,         // invalid sym value
    SDI_INVALID_SPID,           // invalid spid
    SDI_SHUTDOWN,               // code set during SDIDbgOff
    SDI_MAX                     // last code we know of
};

typedef SDI_EC (_stdcall *pfnSDIGetLastError) (void);    

// memory management routines
typedef void * (_stdcall *pfnSDIPvAlloc)  (size_t cb);
typedef void * (_stdcall *pfnSDIPvAllocZ) (size_t cb);
typedef void * (_stdcall *pfnSDIPvRealloc)(void *pv, size_t cb);
typedef void   (_stdcall *pfnSDIFreePv)   (void *pv);

// function tables
typedef struct _SQLFNTBL {                      // function table filled in by sql server
    pfnSDIGetLastError          SDIGetLastError;
    pfnSDIGetSym                SDIGetSym;
    pfnSDISetSym                SDISetSym;
    pfnSDIDbgOff                SDIDbgOff;
} SQLFNTBL, *PSQLFNTBL;

typedef struct _DBGFNTBL {                      // function table filled in by debug dll
    pfnSDINewSPID				SDINewSPID;
    pfnSDINewSP                 SDINewSP;
    pfnSDIStep                  SDIStep;
    pfnSDINewBatch              SDINewBatch;
    pfnSDIPvAlloc               SDIPvAlloc;
    pfnSDIPvAllocZ              SDIPvAllocZ;
    pfnSDIPvRealloc             SDIPvRealloc;
    pfnSDIFreePv                SDIFreePv;
    pfnSDIPop                   SDIPop;
    pfnSDICloseConnection       SDICloseConnection;
    pfnSDISpidContext           SDISpidContext;
	pfnSDIError					SDIError;
} DBGFNTBL, *PDBGFNTBL;

// macros for ease of use
#define SDINEWSPID(pfntbl,spid,szSqlSrvr,szMachine,pvDbgData,cbDbgData,pid,dbgpid) ((*((pfntbl)->SDINewSPID))((spid),(szSqlSrvr),(szMachine),(pvDbgData),(cbDbgData),(pid),(dbgpid)))
#define SDINEWBATCH(pfntbl,spid,CmdBuf,cbCmdBuf,nlvl) ((*((pfntbl)->SDINewBatch))((spid),(CmdBuf),(cbCmdBuf),(nlvl)))
#define SDINEWSP(pfntbl, spid, DBName, DBLen, prid, nlvl) ((*((pfntbl)->SDINewSP))((spid),(DBName),(DBLen),(prid),(nlvl)))
#define SDISTEP(pfntbl, spid, prid, idx, off, nlvl) ((*((pfntbl)->SDIStep))((spid),(prid),(idx),(off),(nlvl)))
#define SDIPOP(pfntbl, spid, nlvl) ((*((pfntbl)->SDIPop))((spid),(nlvl)))
#define SDICLOSECONNECTION(pfntbl,spid,sqlerror,sdierror) ((*((pfntbl)->SDICloseConnection))((spid),(sqlerror),(sdierror)))
#define	SDISPIDCONTEXT(pfntbl,spid,pid,tid) ((*((pfntbl)->SDISpidContext))((spid),(pid),(tid)))

#define SDIGETSYM(pfntbl, spid, syms, rgsyminfo, cnt, nlvl)((*((pfntbl)->SDIGetSym))((spid),(syms),(rgsyminfo),(cnt),(nlvl)))
#define SDISETSYM(pfntbl, spid, syms, psyminfo, nlvl)((*((pfntbl)->SDISetSym))((spid),(syms),(psyminfo),(nlvl)))

#define SDIDBGOFF(pfntbl, spid)((*((pfntbl)->SDIDbgOff))((spid)))

#define SDIGETLASTERROR(pfntbl)((*((pfntbl)->SDIGetLastError))())
#define SDIERROR(pfntbl, spid, DBName, DBLen, prid, idx, off, numErr, szErr, cbErr) ((*((pfntbl)->SDIError))((spid),(DBName), (DBLen),(prid),(idx),(off),(numErr),(szErr),(cbErr)))

#define SDIPVALLOC(pfntbl, cb)((*((pfntbl)->SDIPvAlloc))((cb)))
#define SDIPVALLOCZ(pfntbl, cb)((*((pfntbl)->SDIPvAllocZ))((cb)))
#define SDIPVREALLOC(pfntbl, pv, cb)((*((pfntbl)->SDIPvReAlloc))((pv), (cb)))
#define SDIFREEPV(pfntbl, pv)((*((pfntbl)->SDIPvReAlloc))((pv)))

#pragma pack(pop, enter_SDI)

#endif // __SDI_INCLUDED__
