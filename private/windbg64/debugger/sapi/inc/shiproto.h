//  API callback function prototypes (internal to SAPI)

VOID        SHUnloadSymbolHandler( BOOL );
VOID        SHFree( PVOID );

HEXE        SHHexeAddNew( HPDS, HEXG, UOFFSET );
UOFFSET     SHGetDebugStart( HSYM );
LSZ         SHGetSymName( HSYM, LSZ );
BOOL        SHAddrFromHsym( LPADDR, HSYM );
HMOD        SHHmodGetNextGlobal( HEXE *, HMOD );
BOOL        SHModHasSrc( HMOD );
LSZ         SHGetSymbol( LPADDR, LPADDR, SOP, LPODR );
LSZ         SHGetModule( LPADDR, LSZ );
BOOL        SHGetPublicAddr( LPADDR, LSZ );
SHE         SHAddDll( LSZ, BOOL );
BOOL        SHIsLabel( HSYM );
VOID        SHSetDebuggeeDir( LSZ );
VOID        SHUnloadDll( HEXE );
SHE         SHLoadDll( LSZ, BOOL );
BOOL        PHGetAddr ( LPADDR, LSZ );
SHE         SHAddDllsToProcess ( VOID );

HEXE        SHHexeFromHmod ( HMOD );
HEXE        SHGetNextExe(HEXE);
HMOD        SHGetNextMod( HEXE, HMOD );
HMOD        SHHmodGetNext( HEXE, HMOD );

PCXT        SHGetCxtFromHexe(HEXE, PCXT);
PCXT        SHGetCxtFromHmod( HMOD, PCXT );
PCXT        SHSetCxt( LPADDR, PCXT );
PCXT        SHSetCxtMod( LPADDR, PCXT );
HSYM        SHFindNameInGlobal( HSYM, PCXT, LPSSTR, SHFLAG, PFNCMP, PCXT );
HSYM        SHFindNameInTypes( PCXT, LPSSTR, SHFLAG, PFNCMP, PCXT);
HSYM        SHFindNameInContext( HSYM, PCXT, LPSSTR, SHFLAG, PFNCMP, PCXT );
HSYM        SHGoToParent( PCXT, PCXT );
HSYM        SHHsymFromPcxt(PCXT);
HSYM        SHNextHsym(HMOD, HSYM);
SHFLAG      SHCompareRE (LPCH, LPCH, BOOL);
SHFLAG      SHFixupAddr (LPADDR);
SHFLAG      SHUnFixupAddr (LPADDR);
PCHAR       SHGetModName(HMOD);
PCHAR       SHGetExeName(HEXE);
LSZ         SHGetSymFName(HEXE);
HEXE        SHGethExeFromModuleName(LSZ);
LSZ         SHLszGetErrorText(SHE);
BOOL        SHWantSymbols(HEXE);
SHE         SHUnloadSymbols(HEXE);

HFL         SHGethFileFromhMod(HMOD);
HMOD        SHGethModFromName(HEXE, PCHAR);
HEXE        SHGethExeFromName(PCHAR);
BOOL        SHCanDisplay ( HSYM );
UOFF32      SHGetNearestHsym(LPADDR, HMOD, int, PHSYM);
HSYM        SHFindSymInExe(HEXE, LPSSTR, BOOL);

// questionable API calls
int         SHPublicNameToAddr(LPADDR, LPADDR, PCHAR, PFNCMP);
int         SHModelFromAddr ( LPADDR, WORD *, LPB, UOFFSET * );
SHFLAG      SHIsInProlog(PCXT);          // it can be done by EE
SHFLAG      SHIsAddrInCxt(PCXT, LPADDR);
BOOL        SHFindSymbol ( LSZ, PADDR, LPASR );

// end questionable API calls

UOFF32      PHGetNearestHsym(LPADDR, HEXE, PHSYM);
HSYM        PHFindNameInPublics(HSYM, HEXE, LPSSTR, SHFLAG, PFNCMP);

HTYPE       THGetTypeFromIndex( HMOD, THIDX );
HTYPE       THGetNextType(HMOD, HTYPE);
BOOL        THAreTypesEqual(HMOD, CV_typ_t, CV_typ_t);

// Source Line Handler

BOOL        SLLineFromAddr ( LPADDR, LPDWORD, SHOFF *, SHOFF * );
BOOL        SLFLineToAddr  ( HSF, DWORD, LPADDR, SHOFF * , DWORD * );
LPCH        SLNameFromHsf  ( HVOID );
LPCH        SLNameFromHmod ( HMOD, WORD );
BOOL        SLFQueryModSrc ( HMOD );
HMOD        SLHmodFromHsf  ( HEXE, HSF );
HSF         SLHsfFromPcxt  ( PCXT );
HSF         SLHsfFromFile  ( HMOD, LSZ );
int         SLCAddrFromLine( HEXE, HMOD, LSZ, WORD, LPSLP *);

HDEP        MHMemAllocate( unsigned short);
HDEP        MHMemReAlloc(HDEP, unsigned short);
VOID        MHMemFree(HDEP);

HVOID       MHMemLock(HDEP);
VOID        MHMemUnLock(HDEP);
HVOID       MHOmfLock(HVOID);
VOID        MHOmfUnLock(HVOID);
SHFLAG      MHIsMemLocked(HDEP);

SHFLAG      DHExecProc(LPADDR, SHCALL);
USHORT      DHGetDebugeeBytes(ADDR, unsigned short, PVOID);
USHORT      DHPutDebugeeBytes(ADDR, unsigned short, PVOID);
PSHREG      DHGetReg(PSHREG, HFRAME);
PSHREG      DHSetReg(PSHREG, HFRAME);
HDEP        DHSaveReg(VOID);
VOID        DHRestoreReg(HDEP);

HFL         SHHFLFromCXT(PCXT);
HSYM        SHFindNameInSym( HSYM, PCXT, LPSSTR, SHFLAG, PFNCMP, PCXT );

VOID        SHSetEmiOfAddr( LPADDR );

int         SYLoadOmf( PCHAR, unsigned short * );

HFL         SHGETMODHFL( HMOD );

extern HPID hpidCurr;

LPB         SHlszGetSymName ( SYMPTR );
SHFLAG      ExactCmp ( LSZ, HSYM, LSZ, SHFLAG );
HEXG        SHHexgFromHmod ( HMOD hmod );
HEXG        SHHexgFromHmod ( HMOD );
HEXE        SHHexeFromHmod ( HMOD );
VOID        KillPdsNode ( PVOID );
VOID        SHpSymlplLabLoc ( LPLBS );
HPDS        SHFAddNewPds ( VOID );
VOID        SHSetUserDir ( LSZ );
LSZ         SHGetSourceName ( HFL, LPCH );
BOOL        SHAddrToLabel ( LPADDR, LSZ );
BOOL        SHIsEmiLoaded ( HEXE );
BOOL        SHSymbolsLoaded( HEXE, SHE * );
BOOL        SHSymbolsLoadError( HEXE, SHE * );
BOOL        SHFIsAddrNonVirtual ( LPADDR );
BOOL        SHIsFarProc ( HSYM );
int         SHGetSymLoc ( HSYM, LSZ, UINT, PCXT );
SHE         OLLoadOmf ( HEXG, VLDCHK *, UOFFSET);
BOOL        OLUnloadOmf (LPEXG);
PVOID       SHLpGSNGetTable( HEXE );
VOID        SHPdbNameFromExe( LSZ, LSZ, UINT );

HPDS        SHCreateProcess ( VOID );
VOID        SHSetHpid ( HPID );
BOOL        SHDeleteProcess ( HPDS );
HPDS        SHChangeProcess ( HPDS );
SHE         SHAddDllExt( LSZ, BOOL, BOOL, VLDCHK *, HEXG * );
VOID        LoadDefered( HEXG );
VOID        UnloadDefered( HEXG );
SHE         UnloadNow(HEXG);

VOID        VoidCaches(VOID);


VOID        SHSplitPath ( LSZ, LSZ, LSZ, LSZ, LSZ );
int         SumUCChar ( LPSSTR, int );

// REVIEW: piersh
SHE         SHGetExeTimeStamp( LSZ, ULONG * );

HEXE        SHHexeFromHmod ( HMOD );

VOID        SetAddrFromMod(LPMDS lpmds, UNALIGNED ADDR* paddr);

LPVOID      SHGetDebugData( HEXE );
LSZ         SHGetModNameFromHexe(HEXE);

BOOL        SHIsThunk ( HSYM );
HSYM        SHFindSLink32 ( PCXT );

BOOL        FInitLists(VOID);

VOID        KillExgNode( PVOID );
VOID        KillExeNode( PVOID );
VOID        KillMdsNode( PVOID );
VOID        KillPdsNode( PVOID );
VOID        KillGst( LPGST );

int         CmpExgNode( PVOID, PVOID, LONG );
int         CmpExeNode( PVOID, PVOID, LONG );
int         CmpMdsNode( PVOID, PVOID, LONG );
int         CmpPdsNode( PVOID, PVOID, LONG );
int         SHFindBpOrReg( LPADDR, UOFFSET, WORD, PCHAR );
VOID        SHdNearestSymbol( PCXT, SOP, LPODR );
PCXT        SHSetCxtMod( LPADDR, PCXT );
LSZ         NameOnly( LSZ );
BOOL        IsAddrInMod(LPMDS, LPADDR, ISECT*, OFF*, CB*);

LPALM       BuildALM (BOOL, WORD, LPB, DWORD, WORD);
VOID        FixAlign (LPB, PVOID, WORD);
PVOID       LpvFromAlmLfo (LPALM, DWORD);
SYMPTR      GetNextSym (SYMPTR, LPALM);

HSYM        FindNameInStatics (HSYM, PCXT, LPSSTR, SHFLAG, PFNCMP, PCXT);

PVOID       GetSymbols (LPMDS);

BOOL    STABOpen(STAB **ppstab);
BOOL    STABFindUDTSym(STAB* pstab, LPSSTR lpsstr, PFNCMP pfnCmp, SHFLAG fCase, UDTPTR *ppsym, unsigned *piHash);
BOOL    STABAddUDTSym(STAB* pstab, LPSSTR lpsstr, unsigned iHash, UDTPTR* ppsym);
void    STABClose(STAB* pstab);
unsigned hashPbCb(PB pb, CB cb, unsigned long ulMod);

BOOL        VerifyHexe ( HEXE hexe );

BOOL    StartLazyLoader(void);
void    StopLazyLoader(void);

__inline MPT
GetTargetMachine(
    )
{
    MPT     TargetMachine;
    TargetMachine = SYProcessor();
    return(TargetMachine);
}


// Interesting data items

extern HLLI         HlliPds;    // List of processes
extern HPDS         hpdsCur;    // Current process which is being debugged
extern CRITICAL_SECTION csSh;   // Global synchronization object
extern HMODULE      hLib;       // Handle to symcvt dll.
extern CONVERTPROC  pfConvertSymbolsForImage;   // Symcvt ptr.
