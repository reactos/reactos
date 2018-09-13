

XOSD
EMFunc (
    EMF  emf,
    HPID hpid,
    HTID htid,
    DWORD64 wValue,
    DWORD64 lValue
    );

XOSD
DebugPacket (
    DBC dbc,
    HPID hpid,
    HTID htid,
    DWORD wValue,
    LPBYTE lpb
    );

// EMEXPORT is used to export native EM fns to NMs
#ifndef EMEXPORT
#define EMEXPORT __declspec( dllexport )
#endif

EMEXPORT HPRC ValidHprcFromHpid ( HPID );
EMEXPORT HPID HpidFromHprc ( HPRC );
EMEXPORT HTID HtidFromHthd ( HTHD );
EMEXPORT HPRC HprcFromHpid ( HPID );
EMEXPORT HPRC HprcFromPid ( PID );
EMEXPORT HTHD HthdFromHtid ( HPRC, HTID );
EMEXPORT HTHD HthdFromTid ( HPRC, TID );
EMEXPORT HPID HpidFromHthd(HTHD);
EMEXPORT XOSD SendRequest ( DMF, HPID, HTID );
EMEXPORT XOSD SendRequestX ( DMF dmf, HPID hpid, HTID htid, DWORD wLen, LPVOID lpv );

PID  PidFromHprc ( HPRC );
TID  TidFromHthd ( HTHD );
void FlushPTCache ( void );
void PurgeCache ( void );
HLLI LlthdFromHprc ( HPRC );
HLLI LlmdiFromHprc ( HPRC );
STAT StatFromHprc ( HPRC );

VOID SyncHprcWithDM( HPID hpid );

HMDI SwGetMdi( HPID hpid, UOFFSET Address );

XOSD Go ( HPID, HTID, LPEXOP );
XOSD SingleStep ( HPID, HTID, LPEXOP );
XOSD RangeStep ( HPID, HTID, LPRSS );
XOSD ReturnStep ( HPID, HTID, LPEXOP );
XOSD WriteBufferCache ( HPID, HTID, LPADDR, DWORD, LPBYTE, LPDWORD );
XOSD WriteBuffer ( HPID, HTID, LPADDR, DWORD, LPBYTE, LPDWORD );
XOSD CompareAddrs( HPID, HTID, LPCAS );
XOSD UpdateChild ( HPID, HTID, DMF );
XOSD ProcessStatus( HPID, LPPST );
XOSD GetTimeStamp (HPID, HTID, LPTCS);
XOSD ThreadStatus ( HPID hpid, HTID htid, LPTST lptst );
XOSD GetExceptionState(HPID, HTID, LPEXCEPTION_DESCRIPTION);
XOSD SetExceptionState( HPID, HTID, LPEXCEPTION_DESCRIPTION );
XOSD HandleBreakpoints( HPID hpid, DWORD wValue, UINT_PTR lValue );
XOSD FreezeThread( HPID hpid, HTID htid, BOOL fFreeze );
XOSD GetMemoryInfo( HPID hpid, HTID htid, LPMEMINFO lpmi );
XOSD GetModuleNameFromAddress( HPID, DWORD64, PTSTR );
XOSD GetModuleList( HPID, HTID, LPTSTR, LPMODULE_LIST * );
XOSD UnLoadFixups ( HPID, HEMI );
XOSD NewSymbolsLoaded ();

void
UnRegisterEmi(
    LPPRC lpprc,
    HMDI hmdi
    );

XOSD SystemService(HPID hpid, HTID htid, UINT_PTR wValue, LPSSS lpsss);
XOSD GetFrameRegValue (HPID hpid, HTID htid, DWORD ireg, LPVOID lpvRegValue);

int EMENTRY EXCComp(LPVOID, LPVOID, LONG);


XOSD ProgramLoad ( HPID, DWORD, LPPRL );
XOSD ProgramFree ( HPID, HTID );
XOSD GetAddr ( HPID, HTID, ADR, LPADDR );
XOSD SetAddr ( HPID, HTID, ADR, LPADDR );
XOSD SetAddrFromCSIP ( HTHD hthd );
XOSD SetWatchPoint ( HPID, HTID, DWORD );
XOSD RemoveWatchPoint ( HPID, HTID, DWORD );
void InitUsage ( void );
XOSD ReadBuffer ( HPID, HTID, LPADDR, DWORD, LPBYTE, LPDWORD );
void UpdateRegisters ( HPRC, HTHD );
void UpdateSpecialRegisters ( HPRC hprc, HTHD hthd );
XOSD DoGetContext( HPID hpid, HTID htid, LPVOID  lpv );
XOSD DoSetContext( HPID hpid, HTID htid, LPVOID  lpv );
XOSD LoadFixups ( HPID, HTID, MODULELOAD *);
void RegisterEmi ( HPID, HTID, LPREMI );
XOSD CreateThreadStruct ( HPID, TID, HTID * );
XOSD CreateHprc ( HPID );
VOID DestroyHprc ( HPRC );
VOID DestroyHthd ( HTHD );

XOSD
XXSetFlagValue (
    HPID   hpid,
    HTID   htid,
    DWORD  iFlag,
    LPVOID lpvRegValue
    );

XOSD
XXGetFlagValue (
    HPID hpid,
    HTID htid,
    DWORD iFlag,
    LPVOID lpvRegValue
    );

RestoreRegs(
    HPID hpid,
    HTID htid,
    HIND hmem
    );

XOSD
SaveRegs(
    HPID hpid,
    HTID htid,
    LPHIND lphmem
    );

void EMENTRY PiDKill ( LPVOID );
void EMENTRY TiDKill ( LPVOID );
void EMENTRY MDIKill(LPVOID lpv);
int  EMENTRY PDComp ( LPVOID, LPVOID, LONG );
int  EMENTRY TDComp ( LPVOID, LPVOID, LONG );
int  EMENTRY BPComp ( LPVOID, LPVOID, LONG );
int  EMENTRY TBComp ( LPVOID, LPVOID, LONG );
int  EMENTRY MDIComp ( LPVOID, LPVOID, LONG );

XOSD DebugMetric ( HPID, HTID, MTRC, LPLONG );
XOSD FixupAddr ( HPID, HTID, LPADDR );
XOSD UnFixupAddr ( HPID, HTID, LPADDR );
XOSD SetEmi ( HPID, LPADDR );
XOSD SetFlagValue ( HPID, HTID , DWORD , LPVOID );


XOSD WMSGTranslate( LPWORD, LPWORD, LPTSTR, LPWORD );

HEMI HemiFromHmdi ( HMDI );
XOSD GetPrevInst ( HPID, HTID, LPADDR );

XOSD Disasm ( HPID, HTID, LPSDI );
XOSD BackDisasm( HPID hpid, HTID htid, LPGPIS lpgpis );


XOSD IsCall ( HPID, HTID, LPADDR, LPDWORD );

XOSD Assemble ( HPID, HTID, LPADDR, LPTSTR );                          // [00]

BOOL UpdateFPRegisters ( HPRC, HTHD );
XOSD CleanCacheOfEmi ( void );


XOSD SetPath ( HPID, HTID, BOOL, LPTSTR );
VOID FreeEmErrorStrings( VOID );
LPCTSTR EmError(XOSD xosd);
XOSD EnableCache( HPID  hpid, HTID  htid, BOOL  state );

DWORD GetMessageMask(DWORD);

LPTSTR MHStrdup(LPTSTR lptstr);
VOID InvalidateTlsIndexCache( HPRC );
XOSD ReadTlsIndex(HPRC, LPADDR, DWORD *);

UOFFSET
GetTlsBase (
    HPRC hprc,
    HTHD hthd,
    UOFFSET uoffiTls,
    DWORD * lpiTls
    );

XOSD
GetObjLength(
    HPID hpid,
    LPGOL lpgol
    );

UOFFSET
ConvertOmapFromSrc(
    LPMDI       lpmdi,
    UOFFSET     addr
    );

UOFFSET
ConvertOmapToSrc(
    LPMDI       lpmdi,
    UOFFSET     addr
    );

XOSD
GetSectionObjectsFromDM(
    HPID   hpid,
    LPMDI  lpmdi
    );


//*************************************************************************
//
// stack walking apis
//
//*************************************************************************
BOOL
SwReadMemory(
    LPVOID  lpvhpid,
    ULONG64 lpBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    );

LPVOID
SwFunctionTableAccess(
    LPVOID  lpvhpid,
    ULONG64   AddrBase
    );

ULONG64
SwGetModuleBase(
    LPVOID  lpvhpid,
    ULONG64   ReturnAddress
    );

ULONG64
SwTranslateAddress(
    LPVOID    lpvhpid,
    LPVOID    lpvhtid,
    LPADDRESS64 lpaddr
    );


//
// helper functions for CPU-specific code
//

MPT
MPTFromHthd(
    HTHD hthd
    );

MPT
MPTFromHprc(
    HPRC hprc
    );

PCPU_POINTERS
PointersFromMPT(
    MPT mpt
    );

PCPU_POINTERS
XXPointersFromHprc(
    HPRC hprc
    );

PCPU_POINTERS
PointersFromHpid(
    HPID hpid
    );


//
// macros for CPU-specific functions
//

//#define PointersFromHpid(hpid) ((hpid == hpidCurr)? \
    //(pointersCurr): (ValidHprcFromHpid(hpid)?pointersCurr:((PCPU_POINTERS)0)))

#define GetAddr(hpid, htid, adr, lpaddr)    (PointersFromHpid(hpid)->pfnGetAddr(hpid, htid, adr, lpaddr))
#define SetAddr(hpid, htid, adr, lpaddr)    (PointersFromHpid(hpid)->pfnSetAddr(hpid, htid, adr, lpaddr))
#define DoGetReg(hpid, lpregs, reg, lpv)    (PointersFromHpid(hpid)->pfnDoGetReg(lpregs, reg, lpv))
#define GetRegValue(hpid, htid, reg, lpv)   (PointersFromHpid(hpid)->pfnGetRegValue(hpid, htid, reg, lpv))
#define DoSetReg(hpid, lpregs, reg, lpv)    (PointersFromHpid(hpid)->pfnDoSetReg(lpregs, reg, lpv))
#define SetRegValue(hpid, htid, reg, lpv)   (PointersFromHpid(hpid)->pfnSetRegValue(hpid, htid, reg, lpv))
#define GetFlagValue(hpid, htid, flag, lpv) (PointersFromHpid(hpid)->pfnGetFlagValue(hpid, htid, flag, lpv))
#define SetFlagValue(hpid, htid, flag, lpv) (PointersFromHpid(hpid)->pfnSetFlagValue(hpid, htid, flag, lpv))
#define GetFrame(hpid, htid, wval, lval)    (PointersFromHpid(hpid)->pfnGetFrame(hpid, htid, wval, lval))
#define UpdateChild(hpid, htid, dmf)        (PointersFromHpid(hpid)->pfnUpdateChild(hpid, htid, dmf))
#define GetFrameEH(hpid, htid, ppex, addr)  (PointersFromHpid(hpid)->pfnGetFrameEH(hpid, htid, ppex, addr))
#define CopyFrameRegs(hpid, lpthd, lpbpr)   (PointersFromHpid(hpid)->pfnCopyFrameRegs(lpthd, lpbpr))
#define AdjustForProlog(hpid, htid, paddr, canstep) (PointersFromHpid(hpid)->pfnAdjustForProlog(hpid, htid, paddr, canstep))
#define GetFunctionInfo(hpid, gfi)          (PointersFromHpid(hpid)->pfnGetFunctionInfo(hpid, gfi))

#define Rgfd(hpid)          (PointersFromHpid(hpid)->Rgfd)
#define Rgrd(hpid)          (PointersFromHpid(hpid)->Rgrd)
#define CRgfd(hpid)         (PointersFromHpid(hpid)->CRgfd)
#define CRgrd(hpid)         (PointersFromHpid(hpid)->CRgrd)
#define SizeOfContext(hpid) (PointersFromHpid(hpid)->SizeOfContext)
#define SizeOfStackRegisters(hpid) (PointersFromHpid(hpid)->SizeOfStackRegisters)

