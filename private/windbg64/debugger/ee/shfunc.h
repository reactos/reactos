#ifdef __cplusplus
extern "C" {
#endif

extern PCVF pCVF;
extern PCRF pCRF;

#define MHlpvAlloc          (*pCVF->pMHlpvAlloc)
#define MHFreeLpv           (*pCVF->pMHFreeLpv)
#define SHGetNextExe        (*pCVF->pSHGetNextExe)
#define SHHexeFromHmod      (*pCVF->pSHHexeFromHmod)
#define SHGetNextMod        (*pCVF->pSHGetNextMod)
#define SHGetCxtFromHmod    (*pCVF->pSHGetCxtFromHmod)
#define SHGetCxtFromHexe    (*pCVF->pSHGetCxtFromHexe)
#define SHSetCxt            (*pCVF->pSHSetCxt)
#define SHSetCxtMod         (*pCVF->pSHSetCxtMod)
#define SHFindNameInGlobal  (*pCVF->pSHFindNameInGlobal)
#define SHFindNameInTypes   (*pCVF->pSHFindNameInTypes)
#define SHFindNameInContext (*pCVF->pSHFindNameInContext)
#define SHGoToParent        (*pCVF->pSHGoToParent)
#define SHHsymFromPcxt      (*pCVF->pSHHsymFromPcxt)
#define SHNextHsym          (*pCVF->pSHNextHsym)
#define SHGetFuncCxf        (*pCVF->pSHGetFuncCxf)
#define SHGetModName        (*pCVF->pSHGetModName)
//#define SHGetFileName     (*pCVF->pSHGetFileName)
#define SHGetExeName        (*pCVF->pSHGetExeName)
//#define SHGethFileFromhMod    (*pCVF->pSHGethFileFromhMod)
//#define SHGethModFromName (*pCVF->pSHGethModFromName)
#define SHGetModNameFromHexe (*pCVF->pSHGetModNameFromHexe)
#define SHGethExeFromName   (*pCVF->pSHGethExeFromName)
#define SHGethExeFromModuleName (*pCVF->pSHGethExeFromModuleName)
#define SHGetNearestHsym    (*pCVF->pSHGetNearestHsym)
#define SHIsInProlog        (*pCVF->pSHIsInProlog)
#define SHIsAddrInCxt       (*pCVF->pSHIsAddrInCxt)
//#define SHLineFromADDR        (*pCVF->pSHLineFromADDR)
//#define SHADDRFromLine        (*pCVF->pSHADDRFromLine)
#define SHModelFromAddr     (*pCVF->pSHModelFromAddr)
#define SHCompareRE         (*pCVF->pSHCompareRE)
#define SHFixupAddr         (*pCVF->pSHFixupAddr)
#define SHUnFixupAddr       (*pCVF->pSHUnFixupAddr)
#define SHFindSLink32       (*pCVF->pSHFindSLink32)
#define PHGetNearestHsym    (*pCVF->pPHGetNearestHsym)
#define PHFindNameInPublics (*pCVF->pPHFindNameInPublics)
#define THGetTypeFromIndex  (*pCVF->pTHGetTypeFromIndex)
#define THGetNextType       (*pCVF->pTHGetNextType)
#if CC_LAZYTYPES
#pragma message("using LAZYTYPES")
#define THAreTypesEqual     (*pCVF->pTHAreTypesEqual)
#endif
#define MHMemAllocate       (*pCVF->pMHMemAllocate)
#define MHMemReAlloc        (*pCVF->pMHMemReAlloc)
#define MHMemFree           (*pCVF->pMHMemFree)
#define MHMemLock           (*pCVF->pMHMemLock)
#define MHMemUnLock         (*pCVF->pMHMemUnLock)
#define MHIsMemLocked       (*pCVF->pMHIsMemLocked)
#define DHExecProc          (*pCVF->pDHExecProc)
#define DHSetupExecute      (*pCVF->pDHSetupExecute)
#define DHStartExecute      (*pCVF->pDHStartExecute)
#define DHCleanUpExecute    (*pCVF->pDHCleanUpExecute)
#define SYProcessor         (*pCVF->pSYProcessor)
#define SYGetAddr           (*pCVF->pSYGetAddr)
#define SYSetAddr           (*pCVF->pSYSetAddr)
#define SYGetMemInfo        (*pCVF->pSYGetMemInfo)

// MAGIC NUMBERS

//#define adrPC               1
//#define adrBase             2

//  Controls for EEFormatCXTFromPCXT

#define HCXTFMT_Short                   1
#define HCXTFMT_No_Procedure            2

// disable the OMF lock and unlock calls for non NT builds

#define MHOmfLock(h)        (h)
#define MHOmfUnLock(p)      (0)

#define GetReg              (*pCVF->pDHGetReg)
#define SetReg              (*pCVF->pDHSetReg)
#define SaveReg             (*pCVF->pDHSaveReg)
#define RestoreReg          (*pCVF->pDHRestoreReg)

#define in386mode           (*pCVF->pin386mode)
#define is_assign           (*pCVF->pis_assign)
#ifdef HEXCASE
#define fHexUpper           (*pCVF->pfHexUpper)
#endif
#define ArrayDefault        (*pCVF->pArrayDefault)
#define quit                (*pCVF->pquit)

//
// Source Line Handler
//

#define SLLineFromAddr      (*pCVF->pSLLineFromAddr)
#define SLFLineToAddr       (*pCVF->pSLFLineToAddr)
#define SLNameFromHsf       (*pCVF->pSLNameFromHsf)
#define SLHmodFromHsf       (*pCVF->pSLHmodFromHsf)
#define SLHsfFromPcxt       (*pCVF->pSLHsfFromPcxt)
#define SLHsfFromFile       (*pCVF->pSLHsfFromFile)

//
//      Unicode

#define GetUnicodeStrings       (pCVF->pGetUnicodeStrings)


//  **********************************************************************
//  *                                                                    *
//  *   ASSERTION FAILURE                                                *
//  *                                                                    *
//  **********************************************************************
#define AssertOut           (*pCVF->pCVAssertOut)

//  **********************************************************************
//  *                                                                    *
//  *   RUN TIME CALLBACKS                                               *
//  *                                                                    *
//  **********************************************************************

// These may already be defined in NT; undef to avoid warnings
#undef ultoa
#undef itoa
#undef ltoa
#undef _strtold

#define ultoa(a,b,c)    ((pCRF->pultoa)(a, b, c))
#define itoa(a,b,c)     ((pCRF->pitoa)(a, b, c))
#define ltoa(a,b,c)     ((pCRF->pltoa)(a, b, c))
#define _strtold(a, b)  ((pCRF->p_strtold)(a, b))

#define eprintf      (*pCRF->peprintf)
#define sprintf      (*pCRF->psprintf)

//  **********************************************************************
//  *                                                                    *
//  *   Expr Evaluator Declarations                                      *
//  *                                                                    *
//  **********************************************************************
void    EEInitializeExpr (PCI, PEI);
void    EEFreeStr (EEHSTR);
EESTATUS  EEGetError (PHTM, EESTATUS, PEEHSTR);

EESTATUS  EEParse (const char *, EERADIX, SHFLAG, PHTM, ulong *);
EESTATUS  EEBindTM (PHTM, PCXT, SHFLAG, SHFLAG);
EESTATUS  EEvaluateTM (PHTM, HFRAME, EEDSP);

EESTATUS  EEGetExprFromTM (PHTM, PEERADIX, PEEHSTR, ulong *);
EESTATUS  EEGetValueFromTM (PHTM, EERADIX, PEEFORMAT, PEEHSTR);
EESTATUS  EEGetNameFromTM (PHTM, PEEHSTR);
EESTATUS  EEGetTypeFromTM (PHTM, EEHSTR, PEEHSTR, ulong);
EESTATUS  EEFormatCXTFromPCXT (PCXT, PEEHSTR, DWORD);
void    EEFreeTM (PHTM);

EESTATUS  EEParseBP (char *, EERADIX, SHFLAG, PCXF, PTML, ulong, ulong *, SHFLAG);
void    EEFreeTML (PTML);

EESTATUS  EEInfoFromTM (PHTM, PRI, PHTI);
void    EEFreeTI (PHTI);

EESTATUS  EEGetCXTLFromTM(PHTM, PHCXTL);
void    EEFreeCXTL(PHCXTL);

EESTATUS  EEAssignTMToTM (PHTM, PHTM);

EEPDTYP   EEIsExpandable (PHTM);
SHFLAG    EEAreTypesEqual (PHTM, PHTM);
EESTATUS  EEcChildrenTM (PHTM, long *, PSHFLAG);
EESTATUS  EEGetChildTM (PHTM, long, PHTM, ulong *, EERADIX, SHFLAG);
EESTATUS  EEDereferenceTM (PHTM, PHTM, ulong *, SHFLAG);

EESTATUS  EEcParamTM (PHTM, ulong *, PSHFLAG);
EESTATUS  EEGetParmTM (PHTM, ulong, PHTM, ulong *, SHFLAG);
EESTATUS  EEGetTMFromHSYM (HSYM, PCXT, PHTM, ulong *, SHFLAG, SHFLAG);

EESTATUS  EEFormatAddress( PADDR, char *, DWORD, SHFLAG);
EESTATUS  EEGetHSYMList (HDEP *, PCXT, ulong, uchar *, SHFLAG);
void      EEFreeHSYMList (HDEP *);
EESTATUS  EEGetExtendedTypeInfo (PHTM, PETI);
EESTATUS  EEGetAccessFromTM (PHTM, PEEHSTR, ULONG);
BOOL      EEEnableAutoClassCast(BOOL);
void      EEInvalidateCache(void);
EESTATUS  EEcSynthChildTM(PHTM, long *);
EESTATUS  EEGetBCIA(PHTM, PHBCIA);
void      EEFreeBCIA(PHBCIA);


INLINE HDEP
MemAllocate (
    UINT cb
    )
{
    return (MHMemAllocate (cb));
}


INLINE void
MemFree (
    HDEP hMem
    )
{
    MHMemFree (hMem);
}

INLINE HVOID
MemLock (
    HDEP hMem
    )
{
    return (MHMemLock (hMem));
}

INLINE HDEP
MemReAlloc (
    HDEP hMem,
    uint cb
    )
{
    return (MHMemReAlloc (hMem, cb));
}


INLINE void
MemUnLock (
    HDEP hMem
    )
{
    MHMemUnLock (hMem);
}

#ifdef __cplusplus
} // extern "C" {
#endif
