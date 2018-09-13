/***    DEBSYM.C - symbol lookup routines for expression evaluator
 *
 *
 */

#include "debexpr.h"
#include "debsym.h"


#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY



//  enum specifying return from IsDominated

typedef enum {
    DOM_ambiguous,
    DOM_keep,
    DOM_replace
} DOM_t;

//  Return values from SearchClassName

typedef enum  {
    SCN_error,              // error, abort search
    SCN_notfound,           // not found
    SCN_found,              // found
    SCN_rewrite             // found and this pointer inserted
} SCN_t;


//  Return values from MatchMethod

typedef enum {
    MTH_error,              // error - abort search
    MTH_found,              // found without error
} MTH_t;


typedef struct HSL_restart_t {
    search_t    Name;
    HSYM        hSym;
    HSYML_t     state;
    ulong       mask;
} HSL_restart_t;


static char *vtabptr = "#vptr#";
static HDEP     hSymClass = 0;
static psymclass_t  pSymClass = NULL;

static HDEP     hVBDom = 0;
static pdombase_t   pVBDom = NULL;

static HDEP     hVBSearch = 0;
static pdombase_t   pVBSearch = NULL;
static psearch_t    pNameFirst = NULL;

pnode_t AddETConst (pnode_t, UOFFSET, CV_typ_t);
pnode_t AddETExpr (pnode_t, CV_typ_t, UOFFSET, UOFFSET, CV_typ_t);
pnode_t AddETInit (pnode_t, CV_typ_t);
bool_t  AddHSYM (psearch_t, HSYM, PHSL_HEAD *, ulong, HDEP * );
SCN_t   AddVBList (psymclass_t, pdombase_t *, HDEP *);
SCN_t   AddVBType (pdombase_t *, HDEP *, CV_typ_t);
ulong   AmbFromList (psearch_t);
bool_t  AmbToList (psearch_t);
bool_t  CheckDupAmb (psearch_t);
bool_t  DebLoadConst (peval_t, CONSTPTR, HSYM);
SCN_t   DupSymCl (psearch_t);
SCN_t   FindIntro (psearch_t);
SCN_t   GenQualExpr (psearch_t);
HDEP    GenQualName (psearch_t, psymclass_t);
bool_t  GrowTMList (void);
SCN_t   IncrSymBase (void);
bool_t  InitMod (psearch_t);
DOM_t   IsDominated (psymclass_t, psymclass_t);
bool_t  IsIntroVirtInMlist (ulong , CV_typ_t, UOFFSET *);
bool_t  IsIntroVirt (CV_typ_t);
bool_t  LineNumber (psearch_t);
void    MatchArgs (peval_t, psearch_t, CV_fldattr_t, UOFFSET, bool_t);
HR_t    FASTCALL  MatchFunction (psearch_t);
void    MoveSymCl (HDEP hSymCl);
SCN_t   OverloadToAmbList (psearch_t, psymclass_t);
SCN_t   MethodsToAmbList (psearch_t, psymclass_t);
bool_t  ParseRegister (psearch_t);
void    PurgeAmbCl (psearch_t);
SCN_t   RecurseBase (psearch_t, CV_typ_t, CV_typ_t, UOFFSET, UOFFSET, CV_fldattr_t, bool_t);
SCN_t   RemoveAmb (psearch_t);
SCN_t   SearchAERule (psearch_t);
SCN_t   SearchBases (psearch_t);
SCN_t   SearchBType (psearch_t);
SCN_t   SearchClassName (psearch_t);
bool_t  SearchQualName (psearch_t, psymclass_t, HDEP, bool_t);
SCN_t   SearchRoot (psearch_t);
SCN_t   SetBase (psearch_t, CV_typ_t, CV_typ_t, UOFFSET, UOFFSET, CV_fldattr_t, bool_t);
SCN_t   SetBPValue (psearch_t);
SCN_t   SetValue (psearch_t);
HR_t    SymAmbToList (psearch_t);
CV_typ_t SkipModifiers(HMOD mod, CV_typ_t type);
bool_t SymToNode (psearch_t);
bool_t VBSearched (CV_typ_t);
bool_t VBaseFound (psearch_t);

bool_t FASTCALL  InsertThis (psearch_t);
MTH_t MatchMethod (psearch_t, psymclass_t);

// NOTE:  OpName[0] MUST be the this string

OPNAME OpName[] = {
    {"\x004""this"},             //  OP_this
    {"\x00b""operator->*"},      //  OP_Opmember
    {"\x00b""operator>>="},      //  OP_Orightequal
    {"\x00b""operator<<="},      //  OP_Oleftequal
    {"\x00a""operator()"},       //  OP_Ofunction
    {"\x00a""operator[]"},       //  OP_Oarray
    {"\x00a""operator+="},       //  OP_Oplusequal
    {"\x00a""operator-="},       //  OP_Ominusequal
    {"\x00a""operator*="},       //  OP_Otimesequal
    {"\x00a""operator/="},       //  OP_Odivequal
    {"\x00a""operator%="},       //  OP_Opcentequal
    {"\x00a""operator&="},       //  OP_Oandequal
    {"\x00a""operator^="},       //  OP_Oxorequal
    {"\x00a""operator|="},       //  OP_Oorequal
    {"\x00a""operator<<"},       //  OP_Oshl
    {"\x00a""operator>>"},       //  OP_Oshr
    {"\x00a""operator=="},       //  OP_Oequalequal
    {"\x00a""operator!="},       //  OP_Obangequal
    {"\x00a""operator<="},       //  OP_Olessequal
    {"\x00a""operator>="},       //  OP_Ogreatequal
    {"\x00a""operator&&"},       //  OP_Oandand
    {"\x00a""operator||"},       //  OP_Ooror
    {"\x00a""operator++"},       //  OP_Oincrement
    {"\x00a""operator--"},       //  OP_Odecrement
    {"\x00a""operator->"},       //  OP_Opointsto
    {"\x009""operator+"},        //  OP_Oplus
    {"\x009""operator-"},        //  OP_Ominus
    {"\x009""operator*"},        //  OP_Ostar
    {"\x009""operator/"},        //  OP_Odivide
    {"\x009""operator%"},        //  OP_Opercent
    {"\x009""operator^"},        //  OP_Oxor
    {"\x009""operator&"},        //  OP_Oand
    {"\x009""operator|"},        //  OP_Oor
    {"\x009""operator~"},        //  OP_Otilde
    {"\x009""operator!"},        //  OP_Obang
    {"\x009""operator="},        //  OP_Oequal
    {"\x009""operator<"},        //  OP_Oless
    {"\x009""operator>"},        //  OP_Ogreater
    {"\x009""operator,"},        //  OP_Ocomma
    {"\x012""operator new"},     //  OP_Onew
    {"\x015""operator delete"}   //  OP_Odelete
};


//  Symbol searching and search initialization routines



/**     InitSearchBase - initialize search for base class
 *
 *      InitSearchBase (bnOp, typD, typ, pName, pv)
 *
 *      Entry   bnOp = based pointer to OP_cast node
 *              bn = based pointer to cast string
 *              typD = type of derived class
 *              typB = type of desired base class
 *              pName = pointer to symbol search structure
 *              pv = pointer to value node
 *
 *      Exit    search structure initialized for SearchSym
 *
 *      Returns pointer to search symbol structure
 */


void
InitSearchBase (
    bnode_t bnOp,
    CV_typ_t typD,
    CV_typ_t typB,
    psearch_t pName,
    peval_t pv
    )
{
    // set starting context for symbol search to current context

    memset (pName, 0, sizeof (*pName));
    pName->initializer = INIT_base;
    pName->pv = pv;
    pName->typeIn = typD;
    pName->typeOut = typB;
    pName->scope = SCP_class;
    pName->CXTT = *pCxt;
    pName->bn = 0;
    pName->bnOp = bnOp;
    pName->state = SYM_bclass;
    pName->CurClass = typD;
}



#ifdef NEVER
/**     InitSearchtDef - initialize typedef symbol search
 *
 *      InitSearctDef (pName, iClass, tDef, scope, clsmask)
 *
 *      Entry   iClass = initial class if explicit class reference
 *              tDef = index of typedef
 *              scope = mask describing scope of search
 *              clsmask = mask describing permitted class elements
 *
 *      Exit    search structure initialized for SearchSym
 *
 *      Returns pointer to search symbol structure
 */


void
InitSearchtDef (
    psearch_t pName,
    peval_t pv,
    ulong  scope
    )
{
    char    NullString = 0;

    // set starting context for symbol search to current context

    memset (pName, 0, sizeof (*pName));
    pName->initializer = INIT_tdef;
    pName->sstr.lpName = &NullString;
    pName->sstr.cb = 0;
    pName->pfnCmp = TDCMP;
    pName->scope = scope;
    pName->CXTT = *pCxt;
    pName->pv = pv;
    pName->typeIn = EVAL_TYP (pv);
    pName->sstr.searchmask |= SSTR_symboltype | SSTR_NoHash;
    pName->sstr.symtype = S_UDT;
    pName->state = SYM_init;
}
#endif

/**     InitSearchAERule - initialize search for auto-expansion rule
 *
 *      InitSearchAErule (pName, pv, typ, hMod)
 *
 *      Entry   pName = pointer to search structure
 *              pv = pointer to eval_node to receive result
 *              typ = type index of class to be auto-expanded
 *              hMod = handle to module where typ is defined
 *
 *      Exit    search structure initialized for SearchSym
 *
 *      Returns void
 */
void
InitSearchAErule (
    psearch_t pName,
    peval_t pv,
    CV_typ_t typ,
    HMOD hMod
    )
{
    // set starting context for symbol search to current context

    memset (pName, 0, sizeof (*pName));
    pName->initializer = INIT_AErule;
    pName->pfnCmp = 0;
    pName->sstr.lpName = (uchar *) "";
    pName->pv = pv;
    SHGetCxtFromHmod (hMod, &pName->CXTT);

    pName->state = SYM_init;
    // restrict searching to class scope
    pName->ExpClass = typ;
    pName->scope = SCP_class;
}


/**     SearchCFlag - initialize compile flags symbol search
 *
 *      SearchCFlag (pName, iClass, scope, clsmask)
 *
 *      Entry   pName = pointer to symbol search structure
 *
 *      Exit    search structure initialized for SearchSym
 *
 *      Returns pointer to search symbol structure
 */


HSYM
SearchCFlag (void)
{
    search_t    Name;
    CXT         CXTTOut;

    // set starting context for symbol search to current context

    memset (&Name, 0, sizeof (Name));
    Name.pfnCmp = (PFNCMP) CSCMP;
    Name.CXTT = *pCxt;
    Name.sstr.searchmask |= SSTR_symboltype;
    Name.sstr.symtype = S_COMPILE;
    if (InitMod (&Name) == TRUE) {
        SHGetCxtFromHmod (Name.hMod, &Name.CXTT);
        if ((Name.hSym = SHFindNameInContext (Name.hSym,
                                              &Name.CXTT,
                                              &Name.sstr,
                                              pExState->state.fCase,
                                              Name.pfnCmp,
                                              &CXTTOut)) != 0)
        {
            return (Name.hSym);
        }
    }
    // error in context initialization or compile flag symbol not found
    return (0);
}




/**     SetAmbiant - set ambiant code or data model
 *
 *      mode = SetAmbiant (flag)
 *
 *      Entry   flag = TRUE if ambiant data model
 *              flag = FALSE if ambiant code model
 *
 *      Exit    none
 *
 *      Returns ambiant model C7_... from the compile flags symbol
 *              if the compile flags symbol is not found, C7_NEAR is returned
 */


CV_ptrtype_e
SetAmbiant (
    bool_t isdata
    )
{
    HSYM            hCFlag;
    CFLAGPTR        pCFlag;
    CV_ptrtype_e    type;

    Unreferenced( isdata );
    type = (TargetMachine == mptia64)? CV_PTR_64: CV_PTR_NEAR32;

#if !defined(TARGMAC68K)
    if ((hCFlag = SearchCFlag ()) == 0) {
        // compile flag symbol not found, set model to near or near32
    } else {
        pCFlag = (CFLAGPTR) MHOmfLock (hCFlag);
        switch (pCFlag->flags.ambdata) {
            default:
                DASSERT (FALSE);
            case CV_CFL_DNEAR:
                type = (TargetMachine == mptia64)? CV_PTR_64: CV_PTR_NEAR32;
                break;

            case CV_CFL_DFAR:
                type = CV_PTR_FAR32;
                break;

            case CV_CFL_DHUGE:
                type = CV_PTR_HUGE;
                break;
        }
    }
    MHOmfUnLock (hCFlag);
#else
    Unreferenced(hCFlag);
    Unreferenced(pCFlag);
#endif

    return (type);
}




/**     GetHSYMList - get HSYM list for context
 *
 *      status = EEGetHSYMList (phSYMl, pCXT, mask, pRE, fEnableProlog)
 *
 *      Entry   phSYML = pointer to handle to symbol list
 *              pCXT = pointer to context
 *              mask = selection mask
 *              pRE = pointer to regular expression
 *              fEnableProlog = FALSE if function scoped symbols only after prolog
 *              fEnableProlog = TRUE if function scoped symbols during prolog
 *
 *      Exit    *phMem = handle for HSYM  list buffer
 *
 *      Returns EENOERROR if no error
 *              status code if error
 */


EESTATUS
GetHSYMList (
    HDEP *phSYML,
    PCXT pCxt,
    ulong  mask,
    uchar *pRE,
    SHFLAG fEnableProlog
    )
{
    search_t        Name = {0};
    CXT             CXTTOut;
    HSYM            hSym = 0;
    PHSL_HEAD       pHSLHead;
    HSYML_t         state;
    HSL_restart_t *pRestart;
    bool_t          fRestart;
    bool_t          isprolog;
    bool_t         fCaseSensitive;

    if (mask & HSYMR_nocase) {
        mask &= ~HSYMR_nocase;
        fCaseSensitive = FALSE;
    } else
        fCaseSensitive = TRUE;

    if (*phSYML == 0) {
        // allocate and initialize buffer

        if ((*phSYML = MemAllocate (HSYML_SIZE)) == 0) {
            return (EECATASTROPHIC);
        }
        pHSLHead = (PHSL_HEAD) MemLock (*phSYML);
        memset (pHSLHead, 0, pHSLHead->size);

        pHSLHead->size = HSYML_SIZE;
        pHSLHead->remaining = pHSLHead->size - sizeof ( HSL_HEAD );
        pHSLHead->pHSLList = (PHSL_LIST)(((uchar *)pHSLHead) + sizeof (HSL_HEAD));
        state = HSYML_lexical;
        Name.initializer = INIT_RE;
        Name.pfnCmp = (PFNCMP) FNCMP;
        Name.sstr.searchmask |= SSTR_RE;
        Name.CXTT = *pCxt;
        Name.hMod = pCxt->hMod;
        if ( pCxt->hMod ) {
            Name.hExe = SHHexeFromHmod ( pCxt->hMod );
        }
        else {

            // not valid to call SHHexeFromHmod with an Hmod of 0,
            // so set Hexe explicitly
            Name.hExe = 0;
        }
        if ( pRE && *pRE ) {
            Name.sstr.pRE = pRE;
        }
        fRestart = FALSE;
    }
    else {
        pHSLHead = (PHSL_HEAD) MemLock (*phSYML);
        pRestart = (HSL_restart_t *) MemLock (pHSLHead->restart);
        Name = pRestart->Name;
        mask = pRestart->mask;
        state = pRestart->state;
        hSym = pRestart->hSym;
        pHSLHead->blockcnt = 0;
        pHSLHead->symbolcnt = 0;
        pHSLHead->remaining = pHSLHead->size - sizeof ( HSL_HEAD );
        pHSLHead->pHSLList = (PHSL_LIST)(((uchar *)pHSLHead) + sizeof (HSL_HEAD));
        MemUnLock (pHSLHead->restart);
        memset ((uchar *)pHSLHead + sizeof (HSL_HEAD), 0, pHSLHead->size - sizeof (HSL_HEAD));
        AddHSYM (&Name, hSym, &pHSLHead, state, phSYML);
        fRestart = TRUE;
        Name.hSym = hSym;
    }
    switch (state) {
        case HSYML_lexical:
            if (mask & HSYMR_lexical) {
                // search up the lexical scope to but not including the
                // function level

                while (SHIsCXTBlk (&Name.CXTT)) {
                    while ((hSym = SHFindNameInContext (Name.hSym,
                                                        &Name.CXTT,
                                                        &Name.sstr,
                                                        fCaseSensitive,
                                                        (PFNCMP) FNCMP,
                                                        &CXTTOut)) != 0) {
                        if (AddHSYM (&Name, hSym, &pHSLHead, HSYMR_lexical, phSYML) == FALSE) {
                            goto hsyml_exit;
                        }
                        Name.hSym = hSym;
                    }

                    // go to the parent scope

                    SHGoToParent (&Name.CXTT, &CXTTOut);
                    // reset current symbol
                    Name.CXTT = CXTTOut;
                    Name.hSym = 0;
                }
                mask &= ~HSYMR_lexical;
            }

        case HSYML_function:
            if (mask & HSYMR_function) {
                if (fRestart == FALSE) {
                    Name.hSym = 0;
                    Name.CXTT.hBlk = 0;
                }
                fRestart = FALSE;
                state = HSYML_function;

                if ( Name.CXTT.hMod != 0 ) {
                    isprolog = SHIsInProlog (&Name.CXTT);
                }
                while (!SHIsCXTMod (&Name.CXTT) && Name.CXTT.hMod != 0) {
                    while ((hSym = SHFindNameInContext ( Name.hSym,
                                                        &Name.CXTT,
                                                        &Name.sstr,
                                                        fCaseSensitive,
                                                        (PFNCMP) FNCMP,
                                                        &CXTTOut)) != 0
                    ) {
                        BOOL    fAddSym = TRUE;

                        if ((isprolog == TRUE) && (fEnableProlog == FALSE)) {
                            // we want to reject bp_relative and register
                            // stuff if we are in the prolog or epilog of
                            // a function

                            if (!fValidInProlog (hSym, FALSE)) {
                                fAddSym = FALSE;
                            }
                        }

                        if ( fAddSym ) {
                            ulong   len;
                            SYMPTR  pSym = (SYMPTR) MHOmfLock (hSym);

                            switch (pSym->rectyp) {
                                case S_BPREL32:
                                    if (((BPRELPTR32)pSym)->off >= 0) {
                                    // This is a formal argument
                                        len = ((BPRELPTR32)pSym)->name[0];
                                        if ((len == 12) &&
                                            (_tcsncmp((_TXCHAR *) ((BPRELPTR32)pSym)->name+1, "__$ReturnUdt", len) == 0)) {
                                            fAddSym = FALSE;
                                        }
                                    }
                                    break;

                                case S_REGREL32:
                                    if (((LPREGREL32)pSym)->off >= 0) {
                                    // Formal parameter
                                        len = ((LPREGREL32)pSym)->name[0];
                                        if ((len == 12) &&
                                            (_tcsncmp((_TXCHAR *) ((LPREGREL32)pSym)->name+1, "__$ReturnUdt", len) == 0)) {
                                            fAddSym = FALSE;
                                        }
                                    }
                                    break;

                                default:
                                    fAddSym = TRUE;
                                    break;
                            }

                            MHOmfUnLock (hSym);
                        }

                        if ( fAddSym ) {
                            if (AddHSYM (&Name, hSym, &pHSLHead, HSYMR_function, phSYML) == FALSE) {
                                goto hsyml_exit;
                            }
                        }
                        Name.hSym = hSym;
                    }

                    // go to the parent scope

                    SHGoToParent (&Name.CXTT, &CXTTOut);
                    // reset current symbol
                    Name.CXTT = CXTTOut;
                    Name.hSym = 0;
                }
                mask &= ~HSYMR_function;
            }


        case HSYML_class:
            if (mask & HSYMR_class) {
                if (fRestart == FALSE) {
                    Name.hSym = 0;
                    if (ClassImp != 0) {
                        Name.CurClass = ClassImp;
                    }
                }
                fRestart = FALSE;

                if ((Name.CurClass != 0)) {
                    DASSERT (FALSE);
                    // need to do equivalent of EEGetChild on this
                }
                mask &= ~HSYMR_class;
            }

        case HSYML_module:
            if (mask & HSYMR_module) {
                if (fRestart == FALSE) {
                    SHGetCxtFromHmod (Name.hMod, &Name.CXTT);
                    Name.hSym = 0;
                }
                fRestart = FALSE;
                Name.state = (symstate_t) HSYML_module;
                while ((hSym = SHFindNameInContext (Name.hSym,
                                                    &Name.CXTT,
                                                    &Name.sstr,
                                                    fCaseSensitive,
                                                    Name.pfnCmp,
                                                    &CXTTOut)) != 0)
                {
                    if (AddHSYM (&Name, hSym, &pHSLHead, HSYMR_module, phSYML) == FALSE) {
                        goto hsyml_exit;
                    }
                    Name.hSym = hSym;
                }
                mask &= ~HSYMR_module;
            }

        case HSYML_global:
            // search global symbol table
            if (mask & HSYMR_global) {
                if (fRestart == FALSE) {
                    Name.hSym = 0;
                }
                fRestart = FALSE;
                Name.state = (symstate_t) HSYML_global;
                Name.sstr.searchmask |= SSTR_NoHash;
                while ((hSym = SHFindNameInGlobal (Name.hSym,
                                                   &Name.CXTT,
                                                   &Name.sstr,
                                                   fCaseSensitive,
                                                   Name.pfnCmp,
                                                   &CXTTOut)) != 0)
                {
                    if (AddHSYM (&Name, hSym, &pHSLHead, HSYMR_global, phSYML) == FALSE) {
                       goto hsyml_exit;
                    }
                    Name.hSym = hSym;
                }
                mask &= ~HSYMR_global;
            }

        case HSYML_exe:
            if (mask & HSYMR_exe) {
                Name.state = (symstate_t) HSYML_exe;
                if (fRestart == FALSE) {
                    Name.hModCur = SHGetNextMod (Name.hExe, Name.hMod);
                    Name.hSym = 0;
                }
                fRestart = FALSE;
                Name.sstr.searchmask |= SSTR_NoHash;
                do {
                    SHGetCxtFromHmod (Name.hModCur, &Name.CXTT);
                    while ((hSym = SHFindNameInContext (Name.hSym,
                                                        &Name.CXTT,
                                                        &Name.sstr,
                                                        fCaseSensitive,
                                                        Name.pfnCmp,
                                                        &CXTTOut)) != 0)
                    {
                        if (AddHSYM (&Name, hSym, &pHSLHead, HSYMR_exe, phSYML) == FALSE) {
                            goto hsyml_exit;
                        }
                        Name.hSym = hSym;
                    }
                } while (Name.hSym = 0,
                  (Name.hModCur = SHGetNextMod (Name.hExe, Name.hModCur)) != Name.hMod);
                mask &= ~HSYMR_exe;
            }

        case HSYML_public:
            if (mask & HSYMR_public && Name.hExe) {
                if (fRestart == FALSE) {
                    hSym = 0;
                }
                fRestart = FALSE;
                Name.state = (symstate_t) HSYML_public;
                Name.sstr.searchmask |= SSTR_NoHash;
                while ((hSym = PHFindNameInPublics (hSym, Name.hExe,
                  &Name.sstr, fCaseSensitive, Name.pfnCmp)) != 0) {
                    if (AddHSYM (&Name, hSym, &pHSLHead, HSYMR_public, phSYML) == FALSE) {
                        goto hsyml_exit;
                    }
                    Name.hSym = hSym;
                }
                mask &= ~HSYMR_public;
            }
    }
    pHSLHead->status.endsearch = TRUE;
    MemUnLock (*phSYML);
    return (EENOERROR);

hsyml_exit:
    if (pHSLHead->restart == 0) {
        // allocate restart buffer

        if ((pHSLHead->restart = MemAllocate (sizeof (HSL_restart_t))) == 0) {
            pHSLHead->status.fatal = TRUE;
            MemUnLock (*phSYML);
            return (EENOMEMORY);
        }
    }
    pRestart = (HSL_restart_t *) MemLock (pHSLHead->restart);
    pRestart->Name = Name;
    pRestart->mask = mask;
    pRestart->state = state;
    pRestart->hSym = hSym;
    MemUnLock (pHSLHead->restart);
    MemUnLock (*phSYML);
    return (EENOERROR);
}


bool_t
GrowHSL(
    HDEP *phSYML,
    PHSL_HEAD *ppHSLHead
    )
{
    unsigned int bias;
    int newsize;
    HDEP h;

    bias = (unsigned int)((uchar *)(*ppHSLHead)->pHSLList - (uchar *)(*ppHSLHead));
    newsize = (*ppHSLHead)->size + HSYML_SIZE;

    MemUnLock(*phSYML);

    h = MemReAlloc(*phSYML, newsize);
    if (!h) {
        return FALSE;
    }

    *phSYML = h;
    *ppHSLHead = MemLock(h);

    (*ppHSLHead)->size += HSYML_SIZE;
    (*ppHSLHead)->remaining += HSYML_SIZE;
    (*ppHSLHead)->pHSLList = (PHSL_LIST) ( (uchar *)(*ppHSLHead) + bias );
    return TRUE;
}


bool_t
AddHSYM (
    psearch_t pName,
    HSYM hSym,
    PHSL_HEAD *ppHSLHead,
    ulong request,
    HDEP *phSYML
    )
{
    PHSL_LIST   pHSLList;

    pHSLList = (*ppHSLHead)->pHSLList;

    //
    // check usability of current block
    //
    if (pHSLList->status.isused == TRUE) {
        //
        // block has been used, check for same context
        //

        if (memcmp (&pHSLList->Cxt, &pName->CXTT, sizeof (CXT)) != 0) {
            //
            // context has changed
            //

            pHSLList->status.complete = TRUE;

            //
            // Allocate a new HSL_LIST at the end of the list.
            // grow the list if it is full.
            //
            if ((*ppHSLHead)->remaining < sizeof(HSL_LIST) + sizeof(HSYM)) {
                if (!GrowHSL(phSYML, ppHSLHead)) {
                    return FALSE;
                }
            }

            // this will be decremented in the !hascxt code below
            //(*ppHSLHead)->remaining -= sizeof( HSL_LIST );

            //
            // Point the pointer to the new list:
            //

            (*ppHSLHead)->pHSLList = (PHSL_LIST)(
                        (uchar *)((*ppHSLHead)->pHSLList) +
                        sizeof ( HSL_LIST ) +
                        (*ppHSLHead)->pHSLList->symbolcnt * sizeof ( HSYM ) );

            pHSLList = (*ppHSLHead)->pHSLList;

        }
    }

    if (pHSLList->status.hascxt == FALSE) {
        (*ppHSLHead)->blockcnt++;
        (*ppHSLHead)->remaining -= sizeof (HSL_LIST);
        pHSLList->status.hascxt = TRUE;
        pHSLList->Cxt = pName->CXTT;
        pHSLList->request = request;
    }

    //
    // grow the list if it is full.
    //
    if ((*ppHSLHead)->remaining < sizeof (HSYM)) {
        if (!GrowHSL(phSYML, ppHSLHead)) {
            return FALSE;
        } else {
            pHSLList = (*ppHSLHead)->pHSLList;
        }
    }
    pHSLList->status.isused = TRUE;
    pHSLList->hSym[pHSLList->symbolcnt++] = hSym;
    (*ppHSLHead)->remaining -= sizeof (HSYM);
    return (TRUE);
}


//  search for the base sym of the base pointer
//
//  Entry   pName = structure describing the base pointer name
//
//  Exit    pName.eval reflects the bound base info
//
//  Returns HR_ if base sym found

HR_t
SearchBasePtrBase (
    psearch_t pName
    )
{
    search_t lName = *pName;
    plfPointer      pType;
    SYMPTR          pSym;
    unsigned short  lrectyp;
    eval_t  eval;
    peval_t lpv = &eval;
    ulong  savefEProlog = pExState->state.fEProlog;
    HR_t retval = HR_notfound;

    pExState->state.fEProlog = TRUE;    // KLUDGE to get around CXT complications
    eval = *pName->pv;
    CLEAR_EVAL_FLAGS (lpv);
    // an enregistered primitive
    EVAL_IS_REG (lpv) = EVAL_IS_REG(pName->pv);
    EVAL_IS_BPREL (lpv) = EVAL_IS_BPREL(pName->pv);
    lName.hSym = 0; // start search at beginning of CXT
    lName.pv = lpv;
    DASSERT(!CV_IS_PRIMITIVE (EVAL_TYP(lpv)));
    DASSERT (EVAL_MOD (lpv) != 0);
    EVAL_TYPDEF (lpv) =
        THGetTypeFromIndex (EVAL_MOD (lpv), EVAL_TYP(lpv));
    DASSERT (EVAL_TYPDEF (lpv) != 0);
    memset (&lpv->data, 0, sizeof (lpv->data));
    // MOOFUTURE: This may change as new register-handling
    // code is developed.
    // "reg" used to be a union member of "data" but now
    // it is a separate struct in the evaluation node (so
    // that we can keep type information for non-trivial
    // enregistered values). For the time, clear the reg
    // field whenever the data field is cleared, in order to
    // be compatible with the old EE register handling code
    memset (&lpv->reg, 0, sizeof (lpv->reg));
    pType = (plfPointer)(&((TYPPTR)(MHOmfLock (EVAL_TYPDEF(lpv))))->leaf);
    DASSERT(pType->leaf == LF_POINTER);
    DASSERT((pType->attr.ptrtype >= CV_PTR_BASE_VAL) &&
        (pType->attr.ptrtype <= CV_PTR_BASE_SEGADDR));
    pSym = (SYMPTR)(&((plfPointer)pType)->pbase.Sym);
    PTR_BSYMTYPE (pName->pv) = pSym->rectyp;
    emiAddr (PTR_ADDR (pName->pv)) = pCxt->addr.emi;
    switch (pSym->rectyp) {
        case S_BPREL16:
            lName.sstr.lpName = &((BPRELPTR16)pSym)->name[1];
            lName.sstr.cb = ((BPRELPTR16)pSym)->name[0];
            if (SearchSym(&lName) != HR_found) {
                goto ReturnNotFound;;
            }
            PopStack();
            SetAddrSeg (&PTR_ADDR (pName->pv), 0);
            pSym = (SYMPTR)MHOmfLock(lName.hSym);
            if (((BPRELPTR16)pSym)->rectyp == S_BPREL16) {

                SetAddrOff (&PTR_ADDR (pName->pv), ((BPRELPTR16)pSym)->off);
                ADDR_IS_OFF32 (PTR_ADDR (pName->pv)) = FALSE;
                ADDR_IS_FLAT (PTR_ADDR (pName->pv)) = FALSE;
                ADDR_IS_LI (PTR_ADDR (pName->pv)) = FALSE;
                PTR_STYPE (pName->pv) = ((BPRELPTR16)pSym)->typind;

            }
            else {
                goto ReturnNotFound;;
            }
            pExState->state.bprel = TRUE;
            break;

        case S_LDATA16:
        case S_GDATA16:
            lName.sstr.lpName = &((DATAPTR16)pSym)->name[1];
            lName.sstr.cb = ((DATAPTR16)pSym)->name[0];
            lrectyp = pSym->rectyp;
            do {
                if (SearchSym(&lName) != HR_found) {
                    goto ReturnNotFound;;
                }
                PopStack();
                pSym = (SYMPTR)MHOmfLock(lName.hSym);
            } while (pSym->rectyp != lrectyp);
            pExState->state.fLData = TRUE;
            SetAddrSeg (&PTR_ADDR (pName->pv), ((DATAPTR16)pSym)->seg);
            SetAddrOff (&PTR_ADDR (pName->pv), ((DATAPTR16)pSym)->off);
            ADDR_IS_OFF32 (PTR_ADDR (pName->pv)) = FALSE;
            ADDR_IS_FLAT (PTR_ADDR (pName->pv)) = FALSE;
            ADDR_IS_LI (PTR_ADDR (pName->pv)) = TRUE;
            PTR_STYPE (pName->pv) = ((DATAPTR16)pSym)->typind;
            break;

        case S_BPREL32:
            lName.sstr.lpName = &((BPRELPTR32)pSym)->name[1];
            lName.sstr.cb = ((BPRELPTR32)pSym)->name[0];
            if (SearchSym(&lName) != HR_found) {
                goto ReturnNotFound;;
            }
            PopStack();
            SetAddrSeg (&PTR_ADDR (pName->pv), 0);
            if (((BPRELPTR32)pSym)->off != 0) {
                
                SE_SetAddrOff(&PTR_ADDR (pName->pv), ((BPRELPTR32)pSym)->off);
                
                ADDR_IS_OFF32 (PTR_ADDR (pName->pv)) = TRUE;
                ADDR_IS_FLAT (PTR_ADDR (pName->pv)) = TRUE;
                ADDR_IS_LI (PTR_ADDR (pName->pv)) = FALSE;
                PTR_STYPE (pName->pv) = ((BPRELPTR32)pSym)->typind;

            }
            else {
                goto ReturnNotFound;;
            }
            pExState->state.bprel = TRUE;
            break;

        case S_LDATA32:
        case S_GDATA32:
        case S_LTHREAD32:
        case S_GTHREAD32:
            lName.sstr.lpName = &((DATAPTR32)pSym)->name[1];
            lName.sstr.cb = ((DATAPTR32)pSym)->name[0];
            lrectyp = pSym->rectyp;
            do {
                if (SearchSym(&lName) != HR_found) {
                    goto ReturnNotFound;;
                }
                PopStack();
                pSym = (SYMPTR)MHOmfLock(lName.hSym);
            } while (pSym->rectyp != lrectyp);
            pExState->state.fLData = TRUE;
            SetAddrSeg (&PTR_ADDR (pName->pv), ((DATAPTR32)pSym)->seg);
            
            SE_SetAddrOff(&PTR_ADDR (pName->pv), ((DATAPTR32)pSym)->off);
            
            ADDR_IS_OFF32 (PTR_ADDR (pName->pv)) = TRUE;
            ADDR_IS_FLAT (PTR_ADDR (pName->pv)) = TRUE;
            ADDR_IS_LI (PTR_ADDR (pName->pv)) = TRUE;
            PTR_STYPE (pName->pv) = ((DATAPTR32)pSym)->typind;
            break;

        case S_REGISTER:
            break;

        default:
            DASSERT(FALSE);
            break;
        }
    retval = HR_found;

ReturnNotFound:
    pExState->state.fEProlog = savefEProlog;
    return(retval);

}


/**     SearchSym - search for symbol
 *
 *      status = SearchSym (pName)
 *
 *      Entry   pName = structure describing state of search
 *              pName->CXTT = context to start search from
 *              pName->scope = mask that restricts search to specific scope
 *              pName->clsmask = mask for searching in classes.
 *
 *          Typical entry values for specific kinds of search:
 *          Symbol Search (pName->initializer = INIT_sym):
 *              pName->sstr = symbol name
 *              pName->bn = based ptr to symbol node in the expr. tree
 *              pName->pfnCmp = ptr to "compare symbols" function
 *
 *          Right Search (pName->initializer = INIT_right):
 *              (Searches for a symbol at the right of an operator like
 *               '::', '->' or '.')
 *              pName->sstr = symbol name
 *              pName->bn = based ptr to symbol node in the expr. tree
 *              pName->bnOp = based ptr to operator node
 *              pName->pfnCmp = ptr to "compare symbols" function
 *
 *          Base Class Search (pName->initializer = INIT_base):
 *              (Looks for a base class, given the base class type --
 *               e.g., when casting a "Derived *" to a "Base *"
 *              pName->typeOut = Base class type
 *              pName->typeIn = Derived class type
 *              pName->pfnCmp = 0 (no comparison function needed)
 *
 *          TypeDef Search (pName->initializer = INIT_tdef)
 *              (Looks for a UDT symbol of a given type)
 *              pName->typeIn = type of UDT to search for
 *              pName->pfnCmp = ptr to "compare typedef" funciton
 *
 *          Qualified-Name Search (pName->initializer = INIT_qual)
 *              (Special kind of symbol search that looks for a qualified
 *               name)
 *              pName->sstr = symbol name
 *              pName->pfnCmp = ptr to "compare symbols" function
 *
 *          Regular Expression Search (pName->initializer = INIT_RE)
 *              (Search for a symbol name that matches a reg. expression)
 *              pName->sstr = reg. expression
 *              pName->pfnCmp = ptr to "compare symbols" function
 *
 *      Exit    pName updated to reflect search results
 *              pName->hSym = handle of symbol
 *              pName->CXTT = context where symbol was found
 *              pName->pv = loaded symbol node
 *
 *              Side-effects:
 *              (1) In all searches except typedef search the loaded
 *              symbol node is pushed on the stack. The caller should
 *              pop ST in case the push was unnecessary.
 *
 *              (2) A tree rewrite may occur during a class search by
 *              inserting the "this" pointer in the expression tree, when
 *              the expression refers to a class member without naming
 *              the class explicitly. In that case HR_rewrite is returned.
 *
 *              (3) A tree extension may occur during a class search in
 *              order to include a qualified expression for adjusting the
 *              this pointer during evaluation. (The tree extension is done
 *              by GenQualExpr, called by SearchClassName, and in that
 *              case the return value of SearchSym is HR_found.)
 *
 *              (4) In both cases of tree rewrite and tree extension,the
 *              evaluation tree may be reallocated, rendering all non-based
 *              pointers to nodes invalid. Therefore the caller should
 *              not assume that the tree has not changed or moved, even
 *              if the return value of this function does not indicate
 *              a tree rewrite.
 *
 *  Returns     HR_error    : error in search
 *              HR_notfound : symbol was not found
 *              HR_found    : symbol was found
 *              HR_rewrite  : symbol found but tree rewrite required
 *              HR_ambiguous: symbol found but ambiguous
 *              HR_end      : end of search (indicates completion of
 *                adding symbols to the backpatch list while binding bp's)
 *
 */


HR_t
SearchSym (
    psearch_t pName
    )
{
    CXT         CXTTOut;
    HSYM        hSym = 0;
    SCN_t       retval;

    // NOTE: this routine may be called recursively through MatchFunction

    if ((hBPatch != 0) && (pExState->ambiguous != 0) &&
      (pExState->ambiguous == pName->bn)) {
        // this is a request for a symbol that was previously found to
        // be ambiguous.  Pop the next ambiguous symbol off the list and
        // return it to the caller.

        return ((HR_t) AmbFromList (pName));
    }

    if ((pName->sstr.cb == 1 ) &&
        *(pName->sstr.lpName) == '.') {


        EVAL_STATE( pName->pv )    = EV_rvalue;
        EVAL_TYP( pName->pv )      = T_PCHAR;
        EVAL_IS_PTR( pName->pv )   = TRUE;
        EVAL_IS_CURPC( pName->pv ) = TRUE;

        PushStack (pName->pv);
        return (HR_found);
    }

#if 0 // currently not use - be here in case of future needs
    SearchWithSuffix = ( Suffix && !(((LPSSTR)pName)->searchmask & SSTR_RE) );

    if ( SearchWithSuffix ) {
        cbNew   = cbOld+1;
        memcpy( NameNew, NameOld, cbOld );
        NameNew[ cbOld ] = Suffix;
        NameNew[ cbNew ] = '\0';
    }
#endif

    // this routine will terminate on the following conditions:
    //  1.  A symbol is found that is not a function address
    //  2.  A symbol is found that is a function and is not a method
    //      and the end of the symbol table is reached
    //  3.  A symbol is found that is a method and the end of the
    //      inheritance tree for the class is reached.
    // If BindingBP is set, all function/method addresses that match
    // the expression are entered into the TMList pointed to by pTMList

    for (;;) {
        switch (pName->state) {
            case SYM_bclass:
                // this state is a special entry for searching for the base
                // class of a class when only the type of the base is known

                if (InitMod (pName) == FALSE) {
                    // error in context
                    return (HR_notfound);
                }
                pName->state = SYM_class;
                continue;

            case SYM_init:
                if (InitMod (pName) == FALSE) {
                    // error in context so we will allow check for registers
                    return ((HR_t) ParseRegister (pName));
                }
                if ((pName->scope & ~SCP_class) && *pName->sstr.lpName == '@') {
                    //search for @register, @linenumber, fastcall routine

                    if (_istdigit ((_TUCHAR)*(pName->sstr.lpName + 1))) {
                        return ((HR_t) LineNumber (pName));
                    }
                    else if ((hSym = PHFindNameInPublics (NULL,
                      pName->hExe, &pName->sstr, pExState->state.fCase,
                      pName->pfnCmp)) != 0) {
                        goto found;
                    }
                    else if (ParseRegister (pName) == HR_found) {
                        return (HR_found);
                    }
                    else {
                        return (HR_notfound);
                    }
                }
                else if (*pName->sstr.lpName == HSYM_MARKER) {
                    // search for handle to symbol.  The handle to
                    // symbol comes from CV when a specific symbol
                    // is to be searched. The most common case is
                    // when a specific variable is to be displayed
                    // in the locals window
                    hSym = GetHSYMFromHSYMCode( (char *) pName->sstr.lpName + 1 );
                    DASSERT (hSym);
                    goto found;
                }
                // start normal symbol search

                pName->state = SYM_lexical;

            case SYM_lexical:
                if (pName->scope & SCP_lexical) {
                    // search up the lexical scope to the function level
                    // but do not search the module level because of the
                    // class search required for method functions

                    while (!SHIsCXTMod (&pName->CXTT) ) {
                        if ((hSym = SHFindNameInContext (pName->hSym,
                                                        &pName->CXTT,
                                                        &pName->sstr,
                                                        pExState->state.fCase,
                                                        (PFNCMP) FNCMP,
                                                        &CXTTOut)) != 0) {
                            goto foundsave;
                        }

                        // go to the parent scope

                        SHGoToParent (&pName->CXTT, &CXTTOut);
                        // reset current symbol
                        pName->hSym = 0;
                        pName->CXTT = CXTTOut;
                    }
                }

                pName->state = SYM_class;
                if (pName->scope & SCP_class)  {
                    if (pName->ExpClass != 0) {
                        // search an explicit class
                        pName->CurClass = pName->ExpClass;
                    }
                    else if (ClassImp != 0) {
                        pName->CurClass = ClassImp;
                    }
                }

            case SYM_class:
                if ((pName->CurClass != 0) && (pName->scope & SCP_class)) {
                    retval = SearchClassName (pName);
                    switch (retval) {
                        case SCN_found:
                            // the symbol is a member of the class.  If
                            // an explict class was not specified and
                            // the current context is implicitly a class,
                            // then we rewrote the tree to change the
                            // symbol reference to this->symbol

                            DASSERT (pName->hFound == 0);
                            DASSERT (pName->hAmbCl == 0);
                            if (EVAL_TYP (pName->pv) != T_NOTYPE) {
                                if (PushStack (pName->pv) == FALSE)
                                    return (HR_error);
                                goto nopush;
                            }
                            else {
                                pExState->err_num = ERR_BADOMF;
                                return (HR_error);
                            }

                        case SCN_rewrite:
                                return (HR_rewrite);

                        case SCN_error:
                            return (HR_error);

                        default:
                            DASSERT (pName->hFound == 0);
                            DASSERT (pName->hAmbCl == 0);
                            if (pName->initializer == INIT_base) {
                                // we were searching for the base class of
                                // a class which was not found.

                                return (HR_notfound);
                            }
                            break;
                    }
                }
                SHGetCxtFromHmod (pName->hMod, &pName->CXTT);
                pName->hSym = 0;
                pName->state = SYM_module;

            case SYM_module:
                if (pName->scope & SCP_module) {
                    if ((hSym = SHFindNameInContext (pName->hSym,
                                                     &pName->CXTT,
                                                     &pName->sstr,
                                                     pExState->state.fCase,
                                                     pName->pfnCmp,
                                                     &CXTTOut)) != 0)
                    {
                        goto foundsave;
                    }
                }
                pName->state = SYM_global;
                pName->hSym = 0;

            case SYM_global:
                // users specified a context on a break point command
                // we have already searched the module specified so end the search
                // here
                // sps - 7/24/92
                if (pBindBPCxt && pBindBPCxt->hMod &&(pBindBPCxt->hMod != pName->hModCur) &&
                    (pExState->ambiguous != 0) && (pExState->ambiguous == pName->bn)) {

                    return(HR_end);
                }

                // search global symbol table
                if (pName->scope & SCP_global) {

                    if ((hSym = SHFindNameInTypes (&pName->CXTT,
                      &pName->sstr, pExState->state.fCase, pName->pfnCmp,
                      &CXTTOut)) != 0) {
                        goto foundsave;
                    }

                    if ((hSym = SHFindNameInGlobal (pName->hSym,
                                                    &pName->CXTT,
                                                    &pName->sstr,
                                                    pExState->state.fCase,
                                                    pName->pfnCmp,
                                                    &CXTTOut)) != 0)
                    {
                        goto foundsave;
                    }
                }
                pName->hModCur = SHGetNextMod (pName->hExe, pName->hMod);
                pName->hSym = 0;
                pName->state = SYM_exe;

            case SYM_exe:
                if (pName->scope & SCP_global) {
#ifdef LOADALL
                    do {
                        SHGetCxtFromHmod (pName->hModCur, &pName->CXTT);
                        if ((hSym = SHFindNameInContext (pName->hSym,
                                                         &pName->CXTT,
                                                         &pName->sstr,
                                                         pExState->state.fCase,
                                                         pName->pfnCmp,
                                                         &CXTTOut)) != 0)
                        {
                            goto foundsave;
                        }
                    } while (pName->hSym = 0,
                      (pName->hModCur = SHGetNextMod (pName->hExe,
                      pName->hModCur)) != pName->hMod);
#else
                    pName->CXTT.hGrp = pName->CXTT.hMod;
                    pName->CXTT.hMod = 0;
                    if ((hSym = SHFindNameInContext (pName->hSym,
                                                     &pName->CXTT,
                                                     &pName->sstr,
                                                     pExState->state.fCase,
                                                     pName->pfnCmp,
                                                     &CXTTOut) ) != 0)
                    {

                        goto foundsave;
                    }
                    pName->CXTT.hMod = pName->CXTT.hGrp;
#endif
                }
                // we are at the end of the normal symbol search.  If the
                // symbol has not been found, we will search the public
                // symbol table.

                if (!BindingBP) {
                    // if we are not binding breakpoints and we have
                    // found a symbol in previous calls, return.  Otherwise,
                    // we will look in the publics table.  Note that
                    // finding a symbol in the publics table has a
                    // large probability of finding one with a type of
                    // zero which will limit it's usefulness

                    if (pName->hSym != 0) {
                        if (pName->possibles > 1) {
                            pExState->err_num = ERR_AMBIGUOUS;
                            return (HR_ambiguous);
                        }
                        return (HR_notfound);
                    }
                }
                else {
                    // we are binding breakpoints.  We need to see if
                    // one or more symbols have matched.  If so, we
                    // are done with the symbol search.  If we have
                    // not found the symbol, we search the publics table.
                    // For breakpoints, we do not need type information,
                    // only the address, so symbols found in the publics
                    // table are much more useful.

                    if ((pExState->ambiguous != 0) &&
                      (pExState->ambiguous == pName->bn)) {
                        // we have found at least one symbol to this
                        // point. this is indicated by SymAmbToList
                        // setting the node of the function's symbol
                        // into the expression state structure.  By
                        // this point all ambiguous symbols have been
                        // added to the back patch list.  If we have
                        // not found any symbol yet, we search the
                        // publics table.  Note also that there cannot
                        // be any ambiguity in the publics table because
                        // the function names have to be unique in the
                        // publics table.

                        return (HR_end);
                    }
                }

                // search public symbol table
                pName->state = SYM_public;

            case SYM_public:
                // some searches such as finding a UDT by type
                // do not require a symbol
                if (pName->scope & SCP_global) {
                    hSym = PHFindNameInPublics (NULL,
                                                pName->hExe,
                                                &pName->sstr,
                                                pExState->state.fCase,
                                                pName->pfnCmp);
                    if (!hSym) {
                        unsigned char oldmask = pName->sstr.searchmask;
                        pName->sstr.searchmask |= SSTR_FuzzyPublic;
                        pName->sstr.searchmask |= SSTR_NoHash;
                        hSym = PHFindNameInPublics (NULL,
                                                    pName->hExe,
                                                    &pName->sstr,
                                                    pExState->state.fCase,
                                                    pName->pfnCmp);
                        pName->sstr.searchmask = oldmask;
                    }
                    if (hSym != 0) {
                        if (hSym == pName->hSym) {
                            // We found this symbol last call!  There
                            // are no more to be found (or worth finding).
                            return HR_notfound;
                        }

                        // What's this all about????  BryanT 3/11/96
                        if ((TargetMachine == mptmips) ||
                            (TargetMachine == mptdaxp))
                        {
                            goto found;
                        } else {
                            goto foundsave;
                        }
                    }
                }
                if (EVAL_IS_REG (pName->pv)) {
                    return (HR_notfound);   // found it last time - quit looking
                }
                else if ((pName->scope & ~SCP_class) && ParseRegister (pName) == HR_found) {
                    return (HR_found);
                }
                ErrUnknownSymbol(&pName->sstr);
                return (HR_notfound);
        }
    }

foundsave:
    EVAL_CXTT(pName->pv) = pName->CXTT = CXTTOut;
found:
    pName->hSym = hSym;
    EVAL_HSYM (pName->pv) = pName->hSym;
#ifdef NEVER
    if (pName->initializer == INIT_tdef) {
        // the routines that search for UDTs only need the
        // hsym and the stack pointer may not be valid at this
        // point (FormatUDT for example), so return now

        return (HR_found);
    }
#endif
    if (SymToNode (pName) == FALSE) {
        return (HR_notfound);
    }

#ifdef NEVER
    //
    // This is now handled by SetNodeType
    //
    if  (EVAL_IS_PTR(pName->pv) &&
        (EVAL_PTRTYPE(pName->pv) >= CV_PTR_BASE_VAL) &&
        (EVAL_PTRTYPE(pName->pv) <= CV_PTR_BASE_SEGADDR)){
        if (SearchBasePtrBase(pName) != HR_found) {
            return(HR_notfound);
        }
    }
#endif

    if (PushStack (pName->pv) == FALSE)
        return (HR_error);
nopush:
    if (!EVAL_IS_FCN (ST)) {
        // if this symbol is not a function and we have not processed
        // a function previously, then the search is done

        if (pExState->ambiguous == 0) {
            return (HR_found);
        }
        else if (pExState->ambiguous == pName->bn) {
            pExState->err_num = ERR_SYMCONFLICT;
            return (HR_error);
        }
        else {
            return (HR_found);
        }
    }
    else if (EVAL_IS_METHOD (ST)) {
        // ambiguous methods were entered into the ambiguous breakpoint
        // list by SetBPValue which was called in SearchClassName

        return (HR_found);
    }
    else if (pName->state == SYM_public) {
        // we will take the first function symbol found in the
        // publics table since the names have to be unique in the
        // publics table

        return (HR_found);
    }
    else if ((BindingBP == TRUE) && (bArgList == 0)) {
        // if we are binding breakpoints and there is not an
        // argument list, we will enter all functions found with
        // this name into the breakpoint ambiguity list.

        return (SymAmbToList (pName));
    }
    else {
        // the found symbol is non-member function. Continue
        // search to end of symbols to find the best function.
        // in the case of a breakpoint with argument list, there
        // must be an exact match on the argument types

        EVAL_MOD (pName->pv) = SHHMODFrompCXT (&pName->CXTT);

        if (pName->scope & SCP_nomatchfn)
            return (HR_found);

        return (MatchFunction (pName));
    }
}




bool_t
InitMod (
    psearch_t pName
    )
{
    if (((pName->hMod = SHHMODFrompCXT (&pName->CXTT)) == 0) ||
      ((pName->hExe = SHHexeFromHmod (pName->hMod)) == 0)) {
        // error in context
        return (FALSE);
    }
    return (TRUE);
}


//      Class searching and main support routines

/***    SearchClassName - Search a class and bases for element by name
 *
 *      flag = SearchClassName (pName)
 *
 *      Entry   pName = pointer to struct describing search
 *
 *      Exit    pName updated to reflect type
 *
 *      Returns SCN_... enum describing result
 */


SCN_t
SearchClassName (
    psearch_t pName
    )
{
    SCN_t           retval;
    ulong           max;
    CV_fldattr_t    attr = {0};

    // allocate and initialize list of dominated virtual bases

    if (hVBDom == 0) {
        if ((hVBDom = MemAllocate (sizeof (dombase_t) +
          DOMBASE_DEFAULT * sizeof (CV_typ_t))) == 0) {
            pExState->err_num = ERR_NOMEMORY;
            return (SCN_error);
        }
        pVBDom = (pdombase_t) MemLock (hVBDom);
        pVBDom->MaxIndex = DOMBASE_DEFAULT;
    }
    else {
        pVBDom = (pdombase_t) MemLock (hVBDom);
    }
    pVBDom->CurIndex = 0;

    // allocate and initialize list of searched virtual bases

    if (hVBSearch == 0) {
        if ((hVBSearch = MemAllocate (sizeof (dombase_t) +
          DOMBASE_DEFAULT * sizeof (CV_typ_t))) == 0) {
            pExState->err_num = ERR_NOMEMORY;
            MemUnLock (hVBDom);
            return (SCN_error);
        }
        pVBSearch = (pdombase_t) MemLock (hVBSearch);
        pVBSearch->MaxIndex = DOMBASE_DEFAULT;
    }
    else {
        pVBSearch = (pdombase_t) MemLock (hVBSearch);
    }
    pVBSearch->CurIndex = 0;

    // allocate and initialize structure for recursive base searches

    if (hSymClass == 0) {
        if ((hSymClass = MemAllocate (sizeof (symclass_t) +
          SYMBASE_DEFAULT * sizeof (symbase_t))) == 0) {
            pExState->err_num = ERR_NOMEMORY;
            MemUnLock (hVBDom);
            MemUnLock (hVBSearch);
            return (SCN_error);
        }
        pSymClass = (psymclass_t) MemLock (hSymClass);
        pSymClass->MaxIndex = SYMBASE_DEFAULT;
    }
    else {
        pSymClass = (psymclass_t) MemLock (hSymClass);
    }
    pSymClass->CurIndex = 0;
    pSymClass->hNext = 0;
    max = pSymClass->MaxIndex;
    memset (pSymClass, 0, max * sizeof (symbase_t) + sizeof (symclass_t));
    pSymClass->MaxIndex = max;

    // recurse through the inheritance tree searching for the feature

    if ((retval = RecurseBase (pName, pName->CurClass, 0, 0, 0,
      attr, FALSE)) != SCN_error) {
        // at this point we need to check for dominance and then clean
        // up the allocated memory and return the search results

        if (pName->possibles == 0) {
            // if the count was zero, the handle of found list must be zero
            DASSERT (pName->hFound == 0);
            retval = SCN_notfound;
        }
        else if (pName->cFound == 0) {
            if (pName->hFound == 0) {
                DASSERT (pSymClass->viable == TRUE);
                // the found value is in the permanent stack.  This means
                // the feature was found in the most derived class
                retval = SetValue (pName);
            }
        }
        else if (pName->cFound == 1) {
            // the feature was not found in the most derived class
            // but no other matching feature was found.  Move the feature
            // descriptor pointed to by pName->hFound into the permanent
            // feature descriptor pSymClass.  An important thing to remember
            // here is that there can only be one feature descriptor pSymClass
            // when calling SetValue unless we are binding breakpoints.
            // If we are binding breakpoints we can have multiple feature
            // descriptors in pSymClass and the list pointed to by
            // pName->hAmbCl. If we have an argument list, these must
            // resolve to one feature. If we do not have an argument
            // list, all of the features are valid and must be added
            // to the breakpoint list.

            MoveSymCl (pName->hFound);
            pName->hFound = 0;
            pName->cFound--;
            DASSERT (pSymClass->viable == TRUE);
            retval = SetValue (pName);
        }
        else {
            // since two or more features were found, we need to cull the
            // feature list to remove dominated features and features which
            // are the same static item.  Ambiguity is removed strictly on
            // the name of the feature.  Overloaded methods are resolved later.

            if ((retval = RemoveAmb (pName)) == SCN_found) {
                retval = SetValue (pName);
            }
        }
    }
    MemUnLock (hVBDom);
    MemUnLock (hVBSearch);
    MemUnLock (hSymClass);
    DASSERT (pName->hAmbCl == 0);
    return (retval);
}




/***    RecurseBase - Search a class and its bases for element by name
 *
 *      status = RecurseBase (pName, type, vbptr, vbpoff, thisadjust, attr, virtual)
 *
 *      Entry   pName = pointer to struct describing search
 *              type = type index of class
 *              vbptr = type index of virtual base pointer
 *              vbpoff = offset of virtual base pointer from address point
 *              thisadjust = offset of base from previous class
 *              thisadjust = virtual base index if virtual base
 *              attr = attribute of base class
 *              virtual = TRUE if this is a virtual base
 *
 *      Exit    pName updated to reflect type
 *
 *      Returns SCN_... enum describing result
 */


SCN_t
RecurseBase (
    psearch_t pName,
    CV_typ_t type,
    CV_typ_t vbptr,
    UOFFSET vbpoff,
    UOFFSET thisadjust,
    CV_fldattr_t attr,
    bool_t fVirtual
    )
{
    SCN_t       retval;

    // save offset of base from address point for this pointer adjustment

    if (SetBase (pName, type, vbptr, vbpoff, thisadjust, attr, fVirtual) != SCN_found) {
        return (SCN_error);
    }
    if (pName->initializer == INIT_base) {
        retval = SearchBType (pName);
    }
    else if (pName->initializer == INIT_AErule ) {
        retval = SearchAERule (pName);
    }
    else {
        retval = SearchRoot (pName);
    }
    switch (retval) {
        case SCN_found:
            if (((pName->clsmask & CLS_virtintro) == FALSE) &&
                  (pName->initializer != INIT_AErule)) {
                // add virtual bases to searched and dominated base lists
                // if we were searching for the introducing virtual function or auto-expand
                // class name the fact that we found one is sufficient.

                if ((pSymClass->isvbase == TRUE) &&
                  (VBaseFound (pName) == TRUE)) {
                    // the found feature is a previously found
                    // virtual base class.

                    pSymClass->isdupvbase = TRUE;
                }
                if ((retval = AddVBList
                  (pSymClass, &pVBSearch, &hVBSearch)) != SCN_error) {
                    retval = AddVBList (pSymClass, &pVBDom, &hVBDom);
                }
            }

        case SCN_error:
            return (retval);

        case SCN_notfound:
            // we did not find the element in the root of this class
            // so we search the inheritance tree.

            if (pExState->state.fSupBase == FALSE) {
                return (SearchBases (pName));
            }
            else {
                return (SCN_notfound);
            }

        default:
            return (SCN_error);
    }
}




/**     VBaseFound - search to see if virtual base already found
 *
 *      status = VBaseFound (pName)
 *
 *      Entry   pName = structure describing search
 *              pSymClass = structure describing most recent class search
 *
 *      Exit    pName updated to reflect search
 *              pSymClass updated to unambiguous feature
 *
 *      Returns TRUE if duplicate virtual base found
 *              FALSE if not duplicate virtual base
 */


bool_t
VBaseFound (
    psearch_t pName
    )
{
    psymclass_t     pSymCl;
    HDEP            hSymCl;
    CV_typ_t        type = EVAL_TYP (&pSymClass->evalP);

    hSymCl = pName->hFound;
    while (hSymCl != 0) {
        pSymCl = (psymclass_t) MemLock (hSymCl);
        if (pSymCl == pSymClass) {
            return (FALSE);
        }
        if ((pSymCl->isvbase == TRUE) && (type == EVAL_TYP (&pSymCl->evalP))) {
            // the virtual base is already in the list
            return (TRUE);
        }
    }
    return (FALSE);
}


/**     RemoveAmb - remove ambiguity
 *
 *      status = RemoveAmb (pName)
 *
 *      Entry   pName = structure describing search
 *              pSymClass = structure describing most recent class search
 *
 *      Exit    pName updated to reflect search
 *              pSymClass updated to unambiguous feature
 *
 *      Returns SCN_found if unambiguous feature found
 *              SCN_error otherwise
 */


SCN_t
RemoveAmb (
    psearch_t pName
    )
{
    psymclass_t     pSymCl;

    // we know that at this point two or more features were found
    // the list of features are pointed to by pName->hFound.
    // The permanent stack cannot contain a viable feature

    DASSERT (pSymClass->viable == FALSE);
    MoveSymCl (pName->hFound);
    pName->hFound = pSymClass->hNext;
    while (pName->hFound != 0) {
        // lock and check next list element

        pSymCl = (psymclass_t) MemLock (pName->hFound);
        switch (IsDominated (pSymClass, pSymCl)) {
            case DOM_ambiguous:
                // the new feature does not dominate the current best
                MemUnLock (pName->hFound);
                if (BindingBP == FALSE) {
                    pExState->err_num = ERR_AMBIGUOUS;
                    return (SCN_error);
                }
                // feature is ambiguous and we are setting breakpoints
                // save the feature in the ambiguous list and advance to
                // the next feature

                pSymClass->hNext = pSymCl->hNext;
                pSymCl->hNext = pName->hAmbCl;
                pName->hAmbCl = pName->hFound;
                MemUnLock (pName->hAmbCl);
                pName->hFound = pSymClass->hNext;
                break;

            case DOM_replace:
                // decrement the number of possible matches by the
                // count in the current best structure

                pName->possibles -= pSymClass->possibles;
                pName->cFound--;
                MemUnLock (pName->hFound);
                MoveSymCl (pName->hFound);
                pName->hFound = pSymClass->hNext;
                PurgeAmbCl (pName);
                break;

            case DOM_keep:
                // decrement the number of possible matches by the
                // count in the structure being discarded

                pName->possibles -= pSymCl->possibles;
                pName->cFound--;
                pSymClass->hNext = pSymCl->hNext;
                MemUnLock (pName->hFound);
                pName->hFound = pSymClass->hNext;
                break;
        }
    }
    // decrement the count of found features to account for the one we kept
    pName->cFound--;
    return (SCN_found);
}


/***    SearchBases - Search for an element in the bases of a class
 *
 *      flag = SearchBases (pName, pvClass)
 *
 *      Entry   pName = pointer to struct describing search
 *
 *      Exit    pName updated to reflect type
 *
 *      Returns enum describing search state
 *
 */

SCN_t
SearchBases (
    psearch_t pName
    )
{
    ulong           cnt;            // count of number of elements in struct
    HTYPE           hField;         // handle to type record for struct field list
    char           *pField;         // pointer to field list
    uint            fSkip = 0;      // offset in the field list
    SCN_t           retval = SCN_notfound;
    CV_typ_t        newindex;
    CV_typ_t        vbptr;
    UOFFSET         offset;
    UOFFSET         vbpoff;
    CV_fldattr_t    attr;
    CV_fldattr_t    vattr;
    peval_t         pvBase = &pSymClass->symbase[pSymClass->CurIndex].Base;
    bool_t          fVirtual;

    // Set to head of type record and search the base classes in order

    if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), CLASS_FIELD (pvBase))) == 0) {
        // set error and stop search
        DASSERT (FALSE);
        pExState->err_num = ERR_BADOMF;
        return (SCN_error);
    }
    pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);

    for (cnt = CLASS_COUNT (pvBase); cnt > 0; cnt--) {
        fSkip += SkipPad(((uchar *)pField) + fSkip);
        newindex = 0;
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // switch to next part of type record
                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                    DASSERT (FALSE);
                    pExState->err_num = ERR_INTERNAL;
                    return (SCN_error);
                }
                pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                // the LF_INDEX is not part of the field count
                cnt++;
                break;

            case LF_BCLASS:
                // read type index of base class offset of base pointer
                // from address point

                attr = ((plfBClass)(pField + fSkip))->attr;
                newindex = ((plfBClass)(pField + fSkip))->index;
                fSkip += sizeof (struct lfBClass);
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                vbptr = 0;
                vbpoff = 0;
                fVirtual = FALSE;
                goto foo;

            case LF_VBCLASS:
                // read type index of base class, type index of virtual
                // base pointer, offset of virtual base pointer from
                // address point and offset of virtual base displacement
                // from virtual base table

                vattr = ((plfVBClass)(pField + fSkip))->attr;
                newindex = ((plfVBClass)(pField + fSkip))->index;
                vbptr = ((plfVBClass)(pField + fSkip))->vbptr;
                fVirtual = TRUE;
                fSkip += sizeof (struct lfVBClass);
                vbpoff = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                if ((pName->clsmask & CLS_virtintro) == FALSE) {
                    // if we are searching for the introducing virtual method
                    // then we want to search all bases
                    if (VBSearched (newindex) == TRUE) {
                        break;
                    }
                    retval = AddVBType (&pVBSearch, &hVBSearch, newindex);
                }

foo:
                // check base class

                MHOmfUnLock (hField);

                // Advance to next base class structure

                if (IncrSymBase () != SCN_found) {
                    return (SCN_error);
                }
                retval = RecurseBase (pName, newindex, vbptr, vbpoff, offset, attr, fVirtual);
                switch (retval) {
                    case SCN_error:
                        // if we got an error, abort the search
                        newindex = 0;
                        break;

                    case SCN_found:
                        // we have found the feature in a base class.
                        // Dup the class search structure and continue the
                        // search to the end of the inheritance tree

                        if ((pName->clsmask & CLS_virtintro) == FALSE &&
                             (pName->initializer != INIT_AErule))
                        {
                            if ((retval = DupSymCl (pName)) != SCN_found) {
                                // if we got an error duping, abort the search
                                newindex = 0;
                            }
                            // set not found, so the unwinding of the call stack
                            // does not reduplicate the class search structure
                            retval = SCN_notfound;
                        }
                        else {
                            // we were searching for the introducing virtual
                            // method.  Since there can be only one in the
                            // tree above the virtual method, we can terminate
                            // the search immediately  Or we are searching for a
                            // class to autoexpand the node. The first match we found is
                            // what we will use in this case.
                            return (retval);
                        }
                        break;

                    default:
                        break;
                }
                pSymClass->CurIndex--;
                MHOmfLock (hField);
                break;

            default:
                // we have reached the end of the base classes.
                // exit loop and check search results.
                break;

        }
        if (newindex == 0) {
            // all base classes must appear first in the field list.
            break;
        }
    }
    MHOmfUnLock (hField);
    return (retval);
}


/***    SearchBType - Search for an base by type
 *
 *      flag = SearchBType (pName)
 *
 *      Entry   pName = pointer to struct describing search
 *              pSymClass = pointer to base class path list structure
 *
 *      Exit    pName updated to reflect search
 *
 *      Return  enum describing search
 *
 *      If the base class is found, the base class value is stored in the
 *      the symbase[], not in pSymClass->evalP.  This is to simplify the
 *      GenQualExpr code.
 */


INLINE BOOL
getBaseDefnFromDecl(
    CV_typ_t typeIn,
    peval_t pv,
    CV_typ_t* ptypeOut
    )
{
    peval_t         pvBase = &pSymClass->symbase[pSymClass->CurIndex].Base;
    Unreferenced    (pv);
    return  getDefnFromDecl (typeIn, pvBase, ptypeOut);
}

bool_t
getDefnFromDecl(
    CV_typ_t typeIn,
    peval_t pv,
    CV_typ_t* ptypeOut
    )
{
    HTYPE  hType;
    uint            skip;
    TYPPTR pType;
    eval_t localEval;
    neval_t nv = &localEval;
    *nv = *pv;

    DASSERT (!CV_IS_PRIMITIVE (typeIn));
    if ((hType = THGetTypeFromIndex (EVAL_MOD (nv), typeIn)) == 0) {
        pExState->err_num = ERR_BADOMF;
        return FALSE;
    }
    pType = (TYPPTR)MHOmfLock(hType);

    switch (pType->leaf) {
        case LF_STRUCTURE:
        case LF_CLASS:
            {
            plfClass pClass = (plfClass)(&pType->leaf);
            if (pClass->property.fwdref) {
                skip = offsetof (lfClass, data);
                RNumLeaf (((char *)(&pClass->leaf)) + skip, &skip);
                // forward ref - look for the definition of the UDT
                if ((*ptypeOut = GetUdtDefnTindex (typeIn, nv, ((char *)&(pClass->leaf)) + skip)) == T_NOTYPE) {
                    goto failed;
                }
            }
            else
                *ptypeOut = typeIn;

            break;
            }

        case LF_UNION:
            {
            plfUnion pUnion = (plfUnion)(&pType->leaf);
            if (pUnion->property.fwdref) {
                skip = offsetof (lfUnion, data);
                RNumLeaf (((char *)(&pUnion->leaf)) + skip, &skip);
                // forward ref - look for the definition of the UDT
                if ((*ptypeOut = GetUdtDefnTindex (typeIn, nv, ((char *)&(pUnion->leaf)) + skip)) == T_NOTYPE) {
                    goto failed;
                }
            }
            else
                *ptypeOut = typeIn;

            break;
            }

        case LF_ENUM:
            {
            plfEnum pEnum = (plfEnum)(&pType->leaf);
            if (pEnum->property.fwdref) {
                // forward ref - look for the definition of the UDT
                if ((*ptypeOut = GetUdtDefnTindex (typeIn, nv, (char *) pEnum->Name)) == T_NOTYPE) {
                    goto failed;
                }
            }
            else
                *ptypeOut = typeIn;

            break;
            }

        default:
            *ptypeOut = typeIn;
    }

    MHOmfUnLock (hType);
    return TRUE;

failed:
    MHOmfUnLock (hType);
    pExState->err_num = ERR_BADOMF;
    return FALSE;
}

SCN_t
SearchBType (
    psearch_t pName
    )
{
    ulong           cnt;            // count of number of elements in struct
    HTYPE           hField;         // handle to type record for struct field list
    char *pField;       // pointer to field list
    uint            fSkip = 0;      // offset in the field list
    ulong           anchor = 0;     // offset in the field list to start of type
    CV_typ_t        newindex;
    CV_typ_t        vbptr;
    SCN_t           retval = SCN_notfound;
    UOFFSET         offset;
    UOFFSET         vbpoff;
    bool_t          termflag = FALSE;
    CV_fldattr_t    attr;
    CV_fldattr_t    vattr;
    peval_t         pvBase = &pSymClass->symbase[pSymClass->CurIndex].Base;

    // set hField to the handle of the field list for the class

    if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), CLASS_FIELD (pvBase))) == 0) {
        pExState->err_num = ERR_BADOMF;
        return (SCN_error);
    }
    pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);

    //  walk field list for the class

    for (cnt = CLASS_COUNT (pvBase); cnt > 0; cnt--) {
        fSkip += SkipPad(((uchar *)pField) + fSkip);
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // switch to new type record because compiler split it up
                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                    pExState->err_num = ERR_INTERNAL;
                    return (SCN_error);
                }
                pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                // the LF_INDEX is not part of the field count
                cnt++;
                break;

            case LF_BCLASS:
                attr = ((plfBClass)(pField + fSkip))->attr;
                if (!getBaseDefnFromDecl(((plfBClass)(pField + fSkip))->index, pName->pv, &newindex))
                    return SCN_error;
                fSkip += offsetof (lfBClass, offset);
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                if (pName->typeOut == newindex) {
                    // base has been found, set result values

                    MHOmfUnLock (hField);
                    if (IncrSymBase () != SCN_found) {
                        return (SCN_error);
                    }

                    // save offset of base from address point for this
                    // pointer adjustment

                    if (SetBase (pName, newindex, 0, 0, offset, attr, FALSE) != SCN_found) {
                        return (SCN_error);
                    }
                    pSymClass->viable = TRUE;
                    pName->possibles++;
                    return (SCN_found);
                }
                break;

            case LF_VBCLASS:
                vattr = ((plfVBClass)(pField + fSkip))->attr;
                if (!getBaseDefnFromDecl(((plfVBClass)(pField + fSkip))->index, pName->pv, &newindex))
                    return SCN_error;
                vbptr = ((plfVBClass)(pField + fSkip))->vbptr;
                fSkip += offsetof (lfVBClass, vbpoff);
                vbpoff = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                if (pName->typeOut == newindex) {
                    // base has been found, set result values

                    MHOmfUnLock (hField);
                    if (IncrSymBase () != SCN_found) {
                        return (SCN_error);
                    }

                    // save offset of base from address point for this
                    // pointer adjustment

                    if (SetBase (pName, newindex, vbptr, vbpoff, offset, attr, TRUE) != SCN_found) {
                        return (SCN_error);
                    }
                    pSymClass->viable = TRUE;
                    pName->possibles++;
                    return (SCN_found);
                }
                break;

            default:
                termflag = TRUE;
                break;
        }
        if (termflag == TRUE) {
            break;
        }
    }
    MHOmfUnLock (hField);
    return (retval);
}


/***    SearchAERule - Search for auto-expansion rule
 *
 *      flag = SearchAERule (pName)
 *
 *      Description:
 *          The objective is to find an appropriate
 *          auto-expansion rule for a class that has no
 *          expansion rule associated with it. The idea
 *          is to use the expansion rule of a base class
 *          if one exists.
 *          The expansion rule is treated as a feature and
 *          the usual conventions apply regarding dominance
 *          and ambiguity.
 *
 *      Entry   pName = pointer to struct describing search
 *
 *      Exit    pName updated to reflect search
 *
 *      Return  enum describing search
 *
 */


SCN_t
SearchAERule (
    psearch_t pName
    )
{
    peval_t         pvBase = &pSymClass->symbase[pSymClass->CurIndex].Base;
    CV_typ_t        type;
    peval_t         pvF = &pSymClass->evalP;
    LSZ             lsz;
    SCN_t           retval = SCN_notfound;
    static char     buf[256];

    type = EVAL_TYP (pvBase);
    lsz = GetTypeName (type, EVAL_MOD (pvBase));
    if (LoadAERule (lsz, buf, sizeof (buf))) {
        EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
        pvF->CXTT = pName->CXTT;
        SetNodeType (pvF, type);
        EVAL_STATE (pvF) = EV_type;
        pSymClass->possibles = 1;
        pName->possibles++;
        pSymClass->viable = TRUE;
        retval = SCN_found;
    }
    return retval;
}


/***    SearchRoot - Search for an element of a class
 *
 *      flag = SearchRoot (pName)
 *
 *      Entry   pName = pointer to struct describing search
 *              pvBase = pointer to value describing base to be searched
 *
 *      Exit    pName updated to reflect search
 *
 *      Return  enum describing search
 */


SCN_t
SearchRoot (
    psearch_t pName
    )
{
    ulong           cnt;            // count of number of elements in struct
    HTYPE           hField;         // handle to type record for struct field list
    char           *pField;         // pointer to field list
    HTYPE           hBase;          // handle to type record for base class
    uint            fSkip = 0;      // offset in the field list
    uint            anchor;         // offset in the field list to start of type
    uint            tSkip;          // temporary offset in the field list
    bool_t          cmpflag = 1;
    CV_typ_t        newindex;
    CV_typ_t        vbptr;
    SCN_t           retval = SCN_notfound;
    char           *pc;
    UQUAD           value;
    UOFFSET         offset;
    UOFFSET         vbpoff;
    CV_fldattr_t    access;
    int             count;
    CV_typ_t        vfpType = T_NOTYPE;
    peval_t         pvBase = &pSymClass->symbase[pSymClass->CurIndex].Base;
    CV_typ_t        type;
    peval_t         pvF = &pSymClass->evalP;
    UOFFSET         vtabind;
    ulong           vbind;

    // set hField to the handle of the field list for the class

    if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase),
      CLASS_FIELD (pvBase))) == 0) {
        pExState->err_num = ERR_BADOMF;
        return (SCN_error);
    }
    pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);

    //  walk field list for the class

    for (cnt = CLASS_COUNT (pvBase); cnt > 0; cnt--) {
        fSkip += SkipPad(((uchar *)pField) + fSkip);
        anchor = fSkip;
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // switch to new type record because compiler split it up
                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                    pExState->err_num = ERR_BADOMF;
                    return (SCN_error);
                }
                pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                // the LF_INDEX is not part of the field count
                cnt++;
                break;

            case LF_MEMBER:
                fSkip += offsetof (lfMember, offset);
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                access = ((plfMember)(pField + anchor))->attr;
                pc = pField + fSkip;
                fSkip += (*(pField + fSkip) + sizeof (char));
                if (pName->clsmask & CLS_member) {
                    if ((cmpflag = fnCmp (pName, NULL, pc,
                      pExState->state.fCase)) == 0) {
                        type = ((plfMember)(pField + anchor))->index;
                        MHOmfUnLock (hField);
                        hField = 0;
                        EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                        SetNodeType (pvF, type);
                        EVAL_STATE (pvF) = EV_lvalue;

                        // save the casting data for the this pointer

                        pSymClass->offset = offset;
                        pSymClass->access = access;
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                }
                break;

            case LF_ENUMERATE:
                access = ((plfEnumerate)(pField + fSkip))->attr;
                fSkip += offsetof (lfEnumerate, value);
                value = RNumLeaf (pField + fSkip, &fSkip);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (pName->clsmask & CLS_enumerate) {
                    if ((cmpflag = fnCmp (pName, NULL, pc,
                      pExState->state.fCase)) == 0) {
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                }
                break;

            case LF_STMEMBER:
                type = ((plfSTMember)(pField + anchor))->index;
                fSkip += offsetof (lfSTMember, Name);
                if (pName->clsmask & CLS_member) {
                    if ((cmpflag = fnCmp (pName, NULL, pField + fSkip,
                      pExState->state.fCase)) == 0) {
                        pSymClass->access =
                          ((plfSTMember)(pField + anchor))->attr;
                        MHOmfUnLock (hField);
                        hField = 0;
                        EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                        SetNodeType (pvF, type);
                        EVAL_STATE (pvF) = EV_lvalue;
                        EVAL_IS_STMEMBER (pvF) = TRUE;
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                }
                fSkip += *(pField + fSkip) + sizeof (char);
                break;

            case LF_BCLASS:
                type = ((plfBClass)(pField + anchor))->index;
                access = ((plfBClass)(pField + anchor))->attr;
                fSkip += offsetof (lfBClass, offset);
                newindex = ((plfBClass)(pField + anchor))->index;
                offset = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                if ((pName->clsmask & CLS_bclass) != 0) {

                    // switch to the base class type record to check the name

                    MHOmfUnLock (hField);
                    if ((hBase = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                        pExState->err_num = ERR_BADOMF;
                        return (SCN_error);
                    }
                    pField = (char *)(&((TYPPTR)MHOmfLock (hBase))->leaf);
                    tSkip = offsetof (lfClass, data);
                    RNumLeaf (pField + tSkip, &tSkip);
                    pc = pField + tSkip;
                    cmpflag = fnCmp (pName, NULL, pc, pExState->state.fCase);
                    MHOmfUnLock (hBase);

                    // reset to original field list

                    if (cmpflag == 0) {

                        // element has been found, set result values

                        pSymClass->isbase = TRUE;
                        pSymClass->isvbase = FALSE;
                        pSymClass->offset += offset;
                        EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                        SetNodeType (pvF, type);
                        EVAL_STATE (pvF) = EV_type;
                        pSymClass->access = access;
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                    pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                }
                break;

            case LF_VBCLASS:
                type = ((plfBClass)(pField + anchor))->index;
                access = ((plfVBClass)(pField + anchor))->attr;
                newindex = ((plfVBClass)(pField + anchor))->index;
                vbptr =  ((plfVBClass)(pField + anchor))->vbptr;
                fSkip += offsetof (lfVBClass, vbpoff);
                vbpoff = (UOFFSET)RNumLeaf (pField + fSkip, &fSkip);
                vbind = (ulong)RNumLeaf (pField + fSkip, &fSkip);
                if ((pName->clsmask & CLS_bclass) != 0) {

                    // switch to the base class type record to check the name

                    MHOmfUnLock (hField);
                    if ((hBase = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                        pExState->err_num = ERR_BADOMF;
                        return (SCN_error);
                    }
                    pField = (char *)(&((TYPPTR)MHOmfLock (hBase))->leaf);
                    tSkip = offsetof (lfClass, data);
                    RNumLeaf (pField + tSkip, &tSkip);
                    pc = pField + tSkip;
                    cmpflag = fnCmp (pName, NULL, pc, pExState->state.fCase);
                    MHOmfUnLock (hBase);

                    // reset to original field list

                    if (cmpflag == 0) {

                        // element has been found, set result values

                        pSymClass->isbase = FALSE;
                        pSymClass->isvbase = TRUE;
                        pSymClass->vbind = vbind;
                        pSymClass->vbpoff = vbpoff;
                        pSymClass->vbptr = vbptr;
                        EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                        SetNodeType (pvF, type);
                        EVAL_STATE (pvF) = EV_type;
                        pSymClass->access = access;
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                    pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                }
                break;

            case LF_IVBCLASS:
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                break;

            case LF_FRIENDCLS:
                newindex = ((plfFriendCls)(pField + fSkip))->index;
                fSkip += sizeof (struct lfFriendCls);
                if ((pName->clsmask & CLS_fclass) != 0) {

                    DASSERT (FALSE);

                    // switch to the base class type record to check the name

                    MHOmfUnLock (hField);
                    if ((hBase = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                        pExState->err_num = ERR_INTERNAL;
                        return (SCN_error);
                    }
                    pField = (char *)(&((TYPPTR)MHOmfLock (hBase))->data);
                    tSkip = offsetof (lfClass, data);
                    RNumLeaf (pField + tSkip, &tSkip);
                    pc = pField + tSkip;
                    cmpflag = fnCmp (pName, NULL, pc, pExState->state.fCase);
                    MHOmfUnLock (hBase);

                    // reset to original field list

                    pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                    if (cmpflag == 0) {

                        // element has been found, set result values

                        pName->typeOut = newindex;
                    }
                }
                break;

            case LF_FRIENDFCN:
                newindex = ((plfFriendFcn)(pField + fSkip))->index;
                fSkip += sizeof (struct lfFriendFcn) +
                  ((plfFriendFcn)(pField + fSkip))->Name[0];
                if ((pName->clsmask & CLS_frmethod) != 0) {
                    DASSERT (FALSE);
                }
                break;

            case LF_VFUNCTAB:
                fSkip += sizeof (struct lfVFuncTab);
                // save the type of the virtual function pointer
                vfpType = ((plfVFuncTab)(pField + anchor))->type;
                pc = vfuncptr;
                if (pName->clsmask & CLS_vfunc) {
                    if ((cmpflag = fnCmp (pName, NULL, pc,
                      pExState->state.fCase)) == 0) {
                        MHOmfUnLock (hField);
                        hField = 0;
                        EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                        SetNodeType (pvF, vfpType);
                        EVAL_STATE (pvF) = EV_lvalue;

                        // save the casting data for the this pointer

                        if (pName->bnOp != 0) {
                            pSymClass->offset = 0;
                            pSymClass->access.access = CV_public;
                        }
                        pSymClass->possibles = 1;
                        pName->possibles++;
                    }
                }
                break;

            case LF_ONEMETHOD:
                count = 1;
                newindex = ((plfOneMethod)(pField + fSkip))->index;
                pc = pbNameInLfOneMethod((plfOneMethod)(pField + fSkip));
                fSkip += uSkipLfOneMethod((plfOneMethod)(pField + fSkip));
                if (pName->clsmask & CLS_method) {
                    if ((cmpflag = fnCmp (pName, NULL, pc, pExState->state.fCase)) == 0) {
                        // note that the OMF specifies that the vfuncptr will
                        // be emitted as the first field after the bases

                        if (pName->clsmask & CLS_virtintro) {
                            // we are looking for the introducing virtual
                            // method.  We need to find a method with the
                            // correct name of the correct type index that
                            // is introducing.

                            MHOmfUnLock (hField);
                            if (IsIntroVirt (newindex) == FALSE) {
                                cmpflag = 1;
                                pField =
                                    (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                            }
                            else {
                                pField =
                                    (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                                if (FCN_VFPTYPE (pvF) == T_NOTYPE) {
                                    FCN_VFPTYPE (pvF) = vfpType;
                                    FCN_VFPTYPE (pName->pv) = vfpType;
                                    FCN_VTABIND (pvF) = vtabind;
                                    FCN_VTABIND (pName->pv) =
                                        *((plfOneMethod)(pField + anchor))->vbaseoff;
                                }
                                cmpflag = 0;
                            }
                        }
                        else {
                            type = ((plfOneMethod)(pField + anchor))->index;
                            access = ((plfOneMethod)(pField + anchor))->attr;
                            FCN_VTABIND (pvF) = fIntroducingVirtual(access.mprop) ?
                                *((plfOneMethod)(pField + anchor))->vbaseoff : 0;
                            MHOmfUnLock (hField);
                            hField = 0;
                            EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                            SetNodeType (pvF, type);
                            EVAL_STATE (pvF) = EV_lvalue;
                            pSymClass->possibles = 1;
                            pName->possibles = 1;
                            pSymClass->mlist = type;
                            pSymClass->access = access;
                            pSymClass->vfpType = vfpType;
                        }

                    }
                }
                break;

            case LF_METHOD:
                count = ((plfMethod)(pField + fSkip))->count;
                cnt -= count - 1;
                newindex = ((plfMethod)(pField + fSkip))->mList;
                pc = pField + anchor + offsetof (lfMethod, Name);
                fSkip += sizeof (struct lfMethod) + *pc;
                if (pName->clsmask & CLS_method) {
                    if ((cmpflag = fnCmp (pName, NULL, pc,
                      pExState->state.fCase)) == 0) {
                        // note that the OMF specifies that the vfuncptr will
                        // be emitted as the first field after the bases

                        if (pName->clsmask & CLS_virtintro) {
                            // we are looking for the introducing virtual
                            // method.  We need to find a method with the
                            // correct name of the correct type index that
                            // is introducing.

                            MHOmfUnLock (hField);
                            if (IsIntroVirtInMlist (count, newindex, &vtabind) == FALSE) {
                                cmpflag = 1;
                            }
                            else {
                                if (FCN_VFPTYPE (pvF) == T_NOTYPE) {
                                    FCN_VFPTYPE (pvF) = vfpType;
                                    FCN_VFPTYPE (pName->pv) = vfpType;
                                    FCN_VTABIND (pvF) = vtabind;
                                    FCN_VTABIND (pName->pv) = vtabind;
                                }
                                cmpflag = 0;
                            }
                            pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                        }
                        else {
                            // save the type index of the method list, the
                            // count of methods overloaded on this name,
                            // the type index of the vfuncptr.  Also,
                            // increment the number of possible features by
                            // the overload count

                            pSymClass->possibles = count;
                            pName->possibles += count;
                            pSymClass->mlist = newindex;
                            pSymClass->vfpType = vfpType;
                        }
                    }
                }
                break;

            case LF_NESTTYPE:
                newindex = ((plfNestType)(pField + fSkip))->index;
                fSkip += offsetof (lfNestType, Name);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (pName->clsmask & CLS_ntype) {
                    if ((cmpflag = fnCmp (pName, NULL, pc,
                      pExState->state.fCase)) == 0) {
                        // set type of typedef symbol
                        pSymClass->possibles = 1;
                        pName->possibles++;
                        EVAL_STATE (pvF) = EV_type;
                        EVAL_TYP(pvF) = newindex;
                    }
                    else if (pName->clsmask & CLS_enumerate) {
                        // check if the nested type is an enum.  if it is
                        // we gotta search thru its field list for a matching
                        // enumerate
                        // sps - 9/14/92
                        HTYPE       hEnum, hEnumField;         // nested handle
                        plfEnum     pEnum;
                        char *pEnumField;        // field list of the enum record
                        ulong       enumCount;
                        uint        fSkip = 0;      // offset in the field list
                        uint        anchor;         // offset in the field list to start of type

                        if (CV_IS_PRIMITIVE(newindex)) {
                            break;
                        }

                        if ((hEnum = THGetTypeFromIndex (EVAL_MOD (pvBase),
                          newindex)) == 0) {
                            pExState->err_num = ERR_BADOMF;
                            return (SCN_error);
                        }
                        pEnum = (plfEnum) (&((TYPPTR)MHOmfLock (hEnum))->leaf);
                        if (pEnum->leaf == LF_ENUM) {
                            if (pEnum->property.fwdref) {
                                eval_t      localeval = *pvBase;
                                neval_t     nv = &localeval;

                                // forward ref - look for the definition of the UDT
                                if ((newindex = GetUdtDefnTindex (newindex, pvBase, (char *)&(pEnum->Name[0]))) != T_NOTYPE) {
                                    if ((hEnum = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                                        pExState->err_num = ERR_BADOMF;
                                        return (SCN_error);
                                    }
                                    pEnum = (plfEnum) (&((TYPPTR)MHOmfLock (hEnum))->leaf);
                                }
                                else
                                    break;  // No Fwd ref found... no CVInfo for nested type... proceed with search [rm]
                            }
                            if ((hEnumField = THGetTypeFromIndex (EVAL_MOD (pvBase),
                                pEnum->field)) == 0) {
                                if ( BindingBP ) {
                                    break;
                                }
                                else {
                                    pExState->err_num = ERR_BADOMF;
                                    return (SCN_error);
                                }
                            }
                            pEnumField = (char *)(&((TYPPTR)MHOmfLock (hEnumField))->data);
                            for (enumCount = pEnum->count; enumCount > 0; enumCount--) {
                                fSkip += SkipPad(((uchar *)pEnumField) + fSkip);
                                anchor = fSkip;
                                DASSERT ((((plfEasy)(pEnumField + fSkip))->leaf) == LF_ENUMERATE);

                                access = ((plfEnumerate)(pEnumField + fSkip))->attr;
                                fSkip += offsetof (lfEnumerate, value);
                                value = RNumLeaf (pEnumField + fSkip, &fSkip);
                                pc = pEnumField + fSkip;
                                fSkip += *(pEnumField + fSkip) + sizeof (char);
                                if ((cmpflag = fnCmp (pName, NULL, pc,
                                    pExState->state.fCase)) == 0) {
                                    EVAL_MOD (pvF) = SHHMODFrompCXT (&pName->CXTT);
                                    SetNodeType (pvF, pEnum->utype);
                                    EVAL_STATE (pvF) = EV_constant;
                                    EVAL_QUAD (pvF) = value;

                                    // save the casting data for the this pointer

                                    pSymClass->access = access;
                                    pSymClass->possibles = 1;
                                    pName->possibles++;
                                    break;
                                }
                            }
                            MHOmfUnLock (hEnumField);
                        }
                        MHOmfUnLock (hEnum);
                    }
                }
                break;

            default:
                pExState->err_num = ERR_BADOMF;
                retval = SCN_notfound;

        }
#ifdef NEVER
        // if ((cnt == 0) && ************ (pName->tTypeDef != 0)) {
        if ((cnt == 0) && (pName->tTypeDef != 0)) {
            // we found a typedef name that was not hidden by
            // a member or method name
        }
#endif

        if (cmpflag == 0) {
            pSymClass->viable = TRUE;
            retval = SCN_found;
            break;
        }
    }
    if (hField != 0) {
        MHOmfUnLock (hField);
    }
    return (retval);
}




/**     SetBPValue - set the breakpoint list from a class search
 *
 *      status = SetBPValue (pName)
 *
 *      Entry   pName = pointer to structure describing search
 *              pSymCl = pointer to class element
 *
 *      Exit    pName updated to reflect found feature
 *              this pointer inserted in bind tree if feature is part of
 *                  the class pointed to by this pointer of method
 *
 *      Returns SCN_rewrite if this pointer inserted (rebind required)
 *              SCN_found if no error
 *              SCN_error otherwise
 */


SCN_t
SetBPValue (
    psearch_t pName
    )
{
    peval_t     pv;

    DASSERT ((pName->cFound == 0) && (pName->hFound == 0));
    if (pSymClass->mlist != T_NOTYPE) {
        if ((pName->possibles == 1) || (bArgList != 0)) {
            switch (MatchMethod (pName, pSymClass)) {
                default:
                    DASSERT (FALSE);
                case MTH_error:
                    return (SCN_error);

                case MTH_found:
                    break;
            }
        }
        else {
            // we have a list of functions and no argument list.
            // This means that we have an ambiguous breakpoint
            // and need to add the functions to the breakpoint
            // list for user disambiguation.  Basically what
            // we are going to do is force all of the methods
            // into the breakpoint list and let later
            // binding resolve any errors.

            return (OverloadToAmbList (pName, pSymClass));
        }
    }

    // at this point, the following values are set
    //  pName->addr = address of method if not virtual method
    //  pName->typeOut = type index of method
    //  pName->hSym = handle of symbol for function

    pv = &pSymClass->evalP;
    if (fAnyVirtual(FCN_ATTR (pv).mprop)) {
        pExState->err_num = ERR_NOVIRTUALBP;
        return (SCN_error);
    }
    pv = pName->pv;
    *pv = pSymClass->evalP;
    EVAL_ACCESS (pv) = (uchar)(pSymClass->access.access);
    pName->typeOut = EVAL_TYP (pv);

    // save the offset of a class member
    // (caviar #1756) --gdp 9-10-92

    if (pName->bnOp) {
        peval_t pvOp = &pName->bnOp->v[0];
        EVAL_IS_MEMBER(pvOp) = TRUE;
        MEMBER_OFFSET(pvOp) = pName->initializer == INIT_right ?
                    pSymClass->offset : 0;
    }

    return (SCN_found);
}



SCN_t
OverloadToAmbList (
    psearch_t pName,
    psymclass_t pSymCl
    )
{
    SCN_t       retval;

    // add the first list of methods

    if ((retval = MethodsToAmbList (pName, pSymCl)) != SCN_found) {
        return (retval);
    }

    // loop through the remaining class descriptors adding them to the list

    while (pName->hAmbCl != 0) {
        MoveSymCl (pName->hAmbCl);
        if ((retval = MethodsToAmbList (pName,pSymCl)) != SCN_found) {
            return (retval);
        }
    }

    if (pExState->ambiguous == 0) {
        // no instantiated function has been found
        return (SCN_notfound);
    }

    SymToNode (pName);
    DASSERT (EVAL_TYP (pName->pv) != T_NOTYPE);
    return (SCN_found);
}




/**     MethodsToAmbList - add a list of methods to ambiguous breakpoints
 *
 *      flag = MethodsToAmbList (pName, pSymCl)
 *
 *      Entry   pName = pointer to search descriptor
 *              pSymCl = pointer to symbol class structure
 *
 *      Exit    methods added to list of ambiguous breakpoints
 *
 *      Returns SCN_found if methods added
 *              SCN_error if error during addition
 *
 */


SCN_t
MethodsToAmbList (
    psearch_t pName,
    psymclass_t pSymCl
    )
{
    HTYPE           hMethod;
    pmlMethod       pMethod;
    uint            skip;
    CV_fldattr_t    attr;
    CV_typ_t        type;
    UOFFSET         vtabind;
    HDEP            hQual;
    peval_t         pvF;
    ulong           count = pSymCl->possibles;
    peval_t         pvB;
    HMOD            hMod;
    HDEP            hTemp;
    psearch_t       pTemp;
    bool_t          retval;

    pvB = &pSymCl->symbase[pSymCl->CurIndex].Base;
    hMod = EVAL_MOD (pvB);

    // we now walk the list of methods adding each method to the list of
    // ambiguous breakpoints

    if ((hMethod = THGetTypeFromIndex (hMod, pSymCl->mlist)) == NULL) {
        DASSERT (FALSE);
        return (SCN_error);
    }

    // set the number of methods overloaded on this name and the index into
    // the method list

    skip = 0;
    while (count-- > 0) {
        // lock the omf record, extract information for the current entry in
        // the method list and increment to field for next method

        pMethod = (pmlMethod)((&((TYPPTR)MHOmfLock (hMethod))->leaf) + 1);
        pMethod = (pmlMethod)((uchar *)pMethod + skip);
        attr = pMethod->attr;
        type = pMethod->index;
        skip += sizeof (mlMethod);
        if (fIntroducingVirtual(pMethod->attr.mprop)) {
            vtabind = pMethod->vbaseoff[0];
            skip += sizeof(pMethod->vbaseoff[0]);
        }
        else {
            vtabind = 0;
        }
        MHOmfUnLock (hMethod);

        // now add the method to the list

        pvF = &pSymCl->evalP;
        EVAL_MOD (pvF) = hMod;
        DASSERT (type != T_NOTYPE);
        if (SetNodeType (pvF, type) == FALSE) {
            return (SCN_error);
        }
        else {
            FCN_ATTR (pvF) = attr;
            FCN_VTABIND (pvF) = vtabind;
            FCN_VFPTYPE (pvF) = pSymCl->vfpType;
            pName->best.match = type;

            // If we  have an expression such as pFoo->vFunc we
            // should not be setting a bp on CFoo::vFunc where
            // CFoo is the static type to which pFoo points.
            // We have to exclude the case where we are explicitly
            // setting a bp on CFoo::vfunc of course.

            if (BindingBP && (NODE_OP(pName->bnOp) != OP_bscope)
                && fAnyVirtual(FCN_ATTR(pvF).mprop))
            {
                pExState->err_num = ERR_NOVIRTUALBP;
                return (SCN_error);
            }

            if (((hQual = GenQualName (pName, pSymCl)) == 0) ||
              (SearchQualName (pName, pSymCl, hQual, TRUE) == FALSE)) {
                if (hQual != 0) {
                    MemFree (hQual);
                }
                return (SCN_error);
            }
            MemFree (hQual);
        }
        if (FCN_NOTPRESENT (pvF) == FALSE) {
            if (pExState->ambiguous == 0) {
                // we have found the first function.  We need to set all of
                // the return values in the pointer to the search entry

                pExState->ambiguous = pName->bn;

                // at this point, the following values are set
                //  pName->addr = address of method if not virtual method
                //  pName->typeOut = type index of method
                //  pName->hSym = handle of symbol for function

                *pName->pv = pSymClass->evalP;
                EVAL_ACCESS (pName->pv) = (uchar)(pSymClass->access.access);
                pName->typeOut = EVAL_TYP (pName->pv);
                pName->hSym = EVAL_HSYM (pvF);
            }
            else {
                if ((hTemp = MemAllocate (sizeof (search_t))) == 0) {
                    pExState->err_num = ERR_NOMEMORY;
                    return (SCN_error);
                }
                pTemp = (psearch_t) MemLock (hTemp);
                *pTemp = *pName;
                pTemp->hSym = pvF->hSym;
                retval = AmbToList (pTemp);
                MemUnLock (hTemp);
                MemFree (hTemp);
                if (retval == FALSE) {
                    return (SCN_error);
                }
            }
        }
    }
    return (SCN_found);
}


/**     SetValue - set the result of the class search
 *
 *      status = SetValue (pName)
 *
 *      Entry   pName = pointer to structure describing search
 *              pSymClass = pointer to class element
 *
 *      Exit    pName updated to reflect found feature
 *              this pointer inserted in bind tree if feature is part of
 *                  the class pointed to by this pointer of method
 *
 *      Returns SCN_rewrite if this pointer inserted (rebind required)
 *              SCN_found if no error
 *              SCN_error otherwise
 */


SCN_t
SetValue (
    psearch_t pName
    )
{
    peval_t     pv;
    peval_t     pvOp;
    SCN_t       retval;
    HDEP        hQual;

    // When we get here, the situation is a bit complex.
    // If we are not binding breakpoints, the following conditions
    // must be true:
    //      pName->hAmbCl = 0 (only one feature can remain after dominance)
    //      pName->cFound = 0
    //      pName->hFound = 0
    //      pSymClass->viable = TRUE
    //  If the feature is a data item (pSymClass->mlist == T_NOTYPE), then
    //      pName->possibles = 1
    //  If the feature is a method, then
    //      pName->possibles = number of overloads on name
    //      pSymClass->mlist = type index of member list
    //      bArgList = based pointer to argument list tree (cannot be 0)
    //      Argument matching must result in a best match to a single method
    //
    // If we are binding breakpoints, the following conditions must be true
    //      pName->cFound = 0
    //      pName->hFound = 0
    //      pName->possibles = total number of methods in pSymClass and
    //          pName->hAmbCl
    //      pName->hAmbCl > 0 if two or more features survived dominance
    //          Each of the features in pSymClass and the pName->hAmbCl list
    //          must be a method list (pSymClass->mlist != T_NOTYPE)
    //
    // If we do not have an argument list, then we accept all methods in
    // pSymClass->mlist and pName->hAmbCl.mlist.  The number of methods is
    // pName->possibles.
    //
    // If we do have an argument list, then we accept all methods in
    // pSymClass->mlist and pName->hAmbCl.mlist that are exact matches
    // for the argument list.  Implicit conversions are not considered.
    // The number of methods must less than 1 + number of elements in the
    // list pName->hAmbCl and must be less than or equal to pName->possibles.

    if (BindingBP == TRUE) {
        // call the set of routines that will process breakpoints on methods
        // of a class, the inheritance tree of the class and the derivation
        // tree of the class.  If the routine returns without error, pName
        // describes the first function breakpoint and hTMList describes the
        // remainder of the list of methods that match the function and
        // signature if one is specified.

        return (SetBPValue (pName));
    }
    // At this point, there must be only one class after dominance resolution,
    // the count of the found items must be zero, and the handle of the found
    // item list must be null.  Otherwise there was a failure in dominance
    // resolution.

    DASSERT ((pName->hAmbCl == 0) &&
      (pName->cFound == 0) &&
      (pName->hFound == 0) &&
      (pSymClass->viable == TRUE));
    if (pSymClass->mlist == T_NOTYPE) {
        DASSERT (pName->possibles == 1);
    }
    else if ((pName->possibles > 1) && (bArgList == 0)) {
        pExState->err_num = ERR_NOARGLIST;
        return (SCN_error);
    }

    // if we are processing C++, then we can allow for overloaded methods
    // which is specified by the type index of the method list not being
    // zero.  If this is true, then we select the best of the overloaded
    // methods.

    if (pSymClass->mlist != T_NOTYPE) {
        switch (MatchMethod (pName, pSymClass)) {
            default:
                DASSERT (FALSE);
            case MTH_error:
                return (SCN_error);

            case MTH_found:
                // at this point, the following values are set
                //  pName->addr = address of method if not virtual method
                //  pName->typeOut = type index of method
                //  pName->hSym = handle of symbol for function

                pv = &pSymClass->evalP;
                if (fNonIntroVirtual(FCN_ATTR (pv).mprop)) {
                    // search to introducing virtual method
                    if ((retval = FindIntro (pName)) != SCN_found) {
                        return (retval);
                    }
                }
                break;
        }
    }

    if (pName->initializer == INIT_AErule) {
        pv = pName->pv;
        *pv = pSymClass->evalP;
        pName->typeOut = EVAL_TYP (pv);
        return SCN_found;
    }

    // the symbol is a data member of the class (C or C++).  If an
    // explict class was not specified and the current context is
    // implicitly a class (we are in the scope of the method of the class),
    // then we rewrite the tree to change the symbol reference to this->symbol

    if ((pName->initializer != INIT_base) &&
      (pName->initializer != INIT_sym) &&
      (pName->ExpClass == 0)) {
        //DASSERT (FALSE);
        pExState->err_num = ERR_INTERNAL;
        return (SCN_error);
    }

    else if ((pName->ExpClass == 0) && (ClassImp != 0) &&
      (pName->initializer != INIT_base) && !EVAL_IS_STMEMBER(&pSymClass->evalP)) {
        // CUDA #4030: handle access to static members from
        // inside a static member function w/o referring to "this"
        // don't run this code for static members because "this"
        // might not be available [rm]

        // if we are looking at a nested enumerate - don't need the
        // rewrite
        // sps 9/15/92
        // or if we are looking at a base class or nested class type
        // we don't need to rewrite
        // rm 5/6/93

        ulong  state = EVAL_STATE(&(pSymClass->evalP));

        if (state == EV_constant || state == EV_type) {
            *pName->pv = pSymClass->evalP;
            return(SCN_found);
        }

        // if the feature is a member of *this class of a method, then
        // we need to rewrite
        //      feature
        // to
        //      this->feature
        // The calling routine will need to rebind the expression

        InsertThis (pName);
        return (SCN_rewrite);
    }

    // CUDA #4030: do minimal setup of *pv in the INIT_sym case so that we do
    // the right search in the static member access case. Otherwise
    // the initializer for the search must be INIT_right or INIT_base.
    // by the time we get to here we know that bnOp is the based pointer to
    // the node that will receive the casting expression and pv points
    // to the feature node.

    pv = pName->pv;
    pvOp = &pName->bnOp->v[0];
    switch (pName->initializer) {
        case INIT_right:
            *pv = pSymClass->evalP;
            EVAL_ACCESS (pv) = (uchar)(pSymClass->access.access);
            pName->typeOut = EVAL_TYP (pv);
            EVAL_IS_MEMBER (pvOp) = TRUE;
            MEMBER_TYPE (pvOp) = EVAL_TYP (pv);
            MEMBER_OFFSET (pvOp) = pSymClass->offset;
            MEMBER_ACCESS (pvOp) = pSymClass->access;
            if ((pSymClass->isvbase == TRUE) || (pSymClass->isivbase == TRUE)) {
                MEMBER_VBASE (pvOp) = pSymClass->isvbase;
                MEMBER_IVBASE (pvOp) = pSymClass->isivbase;
                MEMBER_VBPTR (pvOp) = pSymClass->vbptr;
                MEMBER_VBPOFF (pvOp) = pSymClass->vbpoff;
                MEMBER_VBIND (pvOp) = pSymClass->vbind;
            }
            break;

        case INIT_base:
            *pv = pSymClass->symbase[pSymClass->CurIndex].Base;
            EVAL_ACCESS (pv) = (uchar)(pSymClass->symbase[pSymClass->
              CurIndex].attrBC.access);
            pName->typeOut = EVAL_TYP (pv);
            EVAL_IS_MEMBER (pvOp) = TRUE;
            EVAL_ACCESS (pvOp) = (uchar)(pSymClass->symbase[pSymClass->
              CurIndex].attrBC.access);
            MEMBER_OFFSET (pvOp) = 0;
            break;

        case INIT_sym:
            // CUDA #4030: handle access to static members from
            // inside a static member function w/o referring to "this" [rm]

            // symbol MUST BE a static member, just setup *pv...

            DASSERT(EVAL_IS_STMEMBER(&pSymClass->evalP));
            *pv = pSymClass->evalP;
            break;

        default:
            DASSERT (FALSE);
            return (SCN_error);
    }

    if (EVAL_IS_STMEMBER (pv) == FALSE) {
        // the feature is not a static data member of the class
        return (GenQualExpr (pName));
    }
    else {
        // the feature is a static member so we need to generate the
        // qualified path and search for a symbol of the correct type
        // so we can set the address

        if (((hQual = GenQualName (pName, pSymClass)) == 0) ||
          (SearchQualName (pName, pSymClass, hQual, FALSE) == FALSE)) {
            if (hQual != 0) {
                MemFree (hQual);
            }
            return (SCN_error);
        }
        *pv = pSymClass->evalP;
        EVAL_STATE (pv) = EV_lvalue;
        MemFree (hQual);
        return (SCN_found);
    }
}


//      Support routines

/**     AddETConst - add evalthisconst node
 *
 *      pn = AddETConst (pnP, off, btype)
 *
 *      Entry   pnP = pointer to previous node
 *
 *      Exit    OP_evalthisconst node added to bind tree
 *              current node linked to pnP as left child if pnP != NULL
 *              pTree->node_next advanced
 *              btype = type of the base class
 *
 *      Returns pointer to node just allocated
 */


pnode_t
AddETConst (
    pnode_t pnP,
    UOFFSET off,
    CV_typ_t btype
    )
{
    pnode_t     pn;
    padjust_t   pa;

    pn = (pnode_t)(((char *)pTree) + pTree->node_next);
    pa = (padjust_t)(&pn->v[0]);
    memset (pn, 0, sizeof (node_t) + sizeof (adjust_t));
    NODE_OP (pn) = OP_thisconst;
    pa->btype = btype;
    pa->disp = off;
    if (pnP != NULL) {
        NODE_LCHILD (pnP) = (bnode_t)pn;
    }
    pTree->node_next += sizeof (node_t) + sizeof (adjust_t);
    return (pn);
}




/**     AddETInit - add evalthisinit node
 *
 *      pn = AddETInit (pnP, btype)
 *
 *      Entry   pnP = pointer to previous node
 *
 *      Exit    OP_evalthisinit node added to bind tree
 *              current node linked to pnP as left child if pnP != NULL
 *              pTree->node_next advanced
 *              btype = type of the base class
 *
 *      Returns pointer to node just allocated
 */


pnode_t
AddETInit (
    pnode_t pnP,
    CV_typ_t btype
    )
{
    pnode_t     pn;
    padjust_t   pa;

    pn = (pnode_t)(((char *)pTree) + pTree->node_next);
    pa = (padjust_t)(&pn->v[0]);
    memset (pn, 0, sizeof (node_t) + sizeof (adjust_t));
    NODE_OP (pn) = OP_thisinit;
    pa->btype = btype;
    if (pnP != NULL) {
        NODE_LCHILD (pnP) = (bnode_t)pn;
    }
    pTree->node_next += sizeof (node_t) + sizeof (adjust_t);
    return (pn);
}




/**     AddETExpr - add evalthisexpr node
 *
 *      pn = AddETExpr (pnP, vbptr, vbpoff, vbind, btype)
 *
 *      Entry   pnP = pointer to previous node
 *              vbptr = type index of virtual base pointer
 *              vboff = offset of vbptr from address point
 *              vbdisp = offset of displacement in virtual base table
 *              btype = type of the base class
 *
 *      Exit    OP_evalthisconst node added to bind tree
 *              current node linked to pnP as left child if pnP != NULL
 *              pTree->node_next advanced
 *
 *      Returns pointer to node just allocated
 *
 *      The evaluation of this node will result in
 *
 *          ab = (ap * vbpoff) + *(*(ap +vbpoff) + vbdisp)
 *      where
 *          ab = address of base class
 *          ap = current address point
 */


pnode_t
AddETExpr (
    pnode_t pnP,
    CV_typ_t vbptr,
    UOFFSET vbpoff,
    UOFFSET vbdisp,
    CV_typ_t btype
    )
{
    pnode_t     pn;
    padjust_t   pa;

    pn = (pnode_t)(((char *)pTree) + pTree->node_next);
    pa = (padjust_t)(&pn->v[0]);
    memset (pn, 0, sizeof (node_t) + sizeof (adjust_t));
    NODE_OP (pn) = OP_thisexpr;
    pa->btype = btype;
    pa->disp = vbdisp;
    pa->vbpoff = vbpoff;
    pa->vbptr = vbptr;
    if (pnP != NULL) {
        NODE_LCHILD (pnP) = (bnode_t)pn;
    }
    pTree->node_next += sizeof (node_t) + sizeof (adjust_t);
    return (pn);
}




/***    AddVBList - Add virtual bases to searched or dominated list
 *
 *      status = AddVBList (pSymClass, ppList, phMem)
 *
 *      Entry   pSymClass = pointer to base class path list structure
 *              ppList = pointer to dominated or searched list
 *              phMem = pointer to handle for ppList
 *
 *      Exit    virtual bases added to list
 *
 *      Return  SCN_found if all bases added to list
 *              SCN_error if bases could not be added
 */


SCN_t
AddVBList (
    psymclass_t pSymClass,
    pdombase_t *ppList,
    HDEP * phList
    )
{
    ulong           cnt;            // count of number of elements in struct
    HTYPE           hField;         // handle to type record for struct field list
    char *pField;       // pointer to field list
    uint            fSkip = 0;      // offset in the field list
    uint            anchor;         // offset in the field list to start of type
    CV_typ_t        newindex;
    SCN_t           retval = SCN_found;
    bool_t          termflag = FALSE;
    peval_t         pvBase = &pSymClass->symbase[pSymClass->CurIndex].Base;

    // set hField to the handle of the field list for the class

    if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), CLASS_FIELD (pvBase))) == 0) {
        pExState->err_num = ERR_BADOMF;
        return (SCN_error);
    }
    pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);

    //  walk field list for the class

    for (cnt = CLASS_COUNT (pvBase); cnt > 0; cnt--) {
        fSkip += SkipPad(((uchar *)pField) + fSkip);
        anchor = fSkip;
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // switch to new type record because compiler split it up
                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pvBase), newindex)) == 0) {
                    pExState->err_num = ERR_INTERNAL;
                    return (SCN_error);
                }
                pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                // the LF_INDEX is not part of the field count
                cnt++;
                break;

            case LF_BCLASS:
                // skip direct base class
                fSkip += offsetof (lfBClass, offset);
                RNumLeaf (pField + fSkip, &fSkip);
                break;

            case LF_VBCLASS:
            case LF_IVBCLASS:
                newindex = ((plfVBClass)(pField + fSkip))->index;
                if ((retval = AddVBType (ppList, phList, newindex)) == SCN_error) {
                    termflag = TRUE;
                    break;
                }
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                break;

            default:
                termflag = TRUE;
                break;
        }
        if (termflag == TRUE) {
            break;
        }
    }
    MHOmfUnLock (hField);
    return (retval);
}


/***    AddVBType - Add virtual base to list
 *
 *      status = AddVBType (ppList, phList, type)
 *
 *      Entry   ppList = pointer to dominated or searched list
 *              phList = pointer to handle for ppList
 *              type = type index
 *
 *      Exit    type added to list
 *
 *      Return  SCN_found if all bases added to list
 *              SCN_error if bases could not be added
 */


SCN_t
AddVBType (
    pdombase_t *ppList,
    HDEP *phList,
    CV_typ_t type
    )
{
    ulong       i;
    bool_t      add = TRUE;
    ulong       len;
    pdombase_t  pList = *ppList;
    HDEP        hList;

    for (i = 0; i < pList->CurIndex; i++) {
        if (type == pList->dom[i]) {
            add = FALSE;
            break;
         }
    }
    if (add == TRUE) {
        if (pList->CurIndex >= pList->MaxIndex) {
            len = sizeof (dombase_t) + (pList->MaxIndex + 10) * sizeof (CV_typ_t);
            MemUnLock (*phList);
            if ((hList = MemReAlloc (*phList, len)) == 0) {
                pExState->err_num = ERR_NOMEMORY;
                pList = (pdombase_t) MemLock (*phList);
                return (SCN_error);
            }
            else {
                *phList = hList;
                *ppList = pList = (pdombase_t) MemLock (*phList);
                pList->MaxIndex += 10;
            }
        }
        pList->dom[pList->CurIndex++] = type;
    }
    return (SCN_found);
}


/**     AmbFromList - add get next ambiguous symbol from list
 *
 *      status = AmbFromList (pName)
 *
 *      Entry   pName = pointer to list describing search
 *
 *      Exit    breakpoint added to list of breakpoints
 *              pName reset to continue breakpoint search
 *
 *      Returns HR_found if breakpoint added
 *              HR_error if breakpoint could not be added to list
 */

ulong
AmbFromList (
    psearch_t pName
    )
{
    psearch_t   pBPatch;

    pBPatch = (psearch_t) MemLock (hBPatch);
    *pName = *pBPatch;
    MemUnLock (hBPatch);
    pName->pv = &pName->bn->v[0];
    EVAL_HSYM (pName->pv) = pName->hSym;
    if ((SymToNode (pName) == TRUE) && (PushStack (pName->pv) == TRUE)) {
        return (HR_found);
    }
    else {
        return (HR_error);
    }
}




/**     AmbToList - add ambiguous symbol to list
 *
 *      fSuccess = AmbToList (pName)
 *
 *      Entry   pName = pointer to list describing search
 *
 *      Exit    ambiguous symbol added to list pointed to by pTMList
 *              pTMLbp
 *
 *      Returns TRUE if symbol added
 *              FALSE if error
 */

bool_t
AmbToList (
    psearch_t pName
    )
{
    HDEP        hBPatch;
    psearch_t   pBPatch;

    if (iBPatch >= pTMLbp->cTMListMax) {
        // the back patch list has filled the TM list
        if (!GrowTMList ()) {
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
    }
    if ((hBPatch = MemAllocate (sizeof (search_t))) != 0) {
        pTMList[iBPatch++] = hBPatch;
        pBPatch = (psearch_t) MemLock (hBPatch);
        *pBPatch = *pName;
        // save address of function in order to check for
        // duplicates
        pBPatch->typeOut = EVAL_TYP(pName->pv);
        pBPatch->addr = EVAL_SYM(pName->pv);
        MemUnLock (hBPatch);
        return (TRUE);
    }
    else {
        pExState->err_num = ERR_NOMEMORY;
        return (FALSE);
    }
}




/**     DupSymCl - duplicate symbol class structure and link to list
 *
 *      DupSymCl (pName);
 *
 *      Entry   pName = handle of structure describing search
 *              *pSymClass = symbol class structure
 *
 *      Exit    *pSymClass = duplicated and linked to list pName->hFound
 *              pSymClass->viable = FALSE
 *
 *      Returns SCN_found if pSymClass duplicated
 *              SCN_error if unable to allocate memory for duplication
 */


SCN_t
DupSymCl (
    psearch_t pName
    )
{
    HDEP        hSymCl;
    psymclass_t pSymCl;
    ulong       size;

    size = (ulong ) (sizeof (symclass_t) + (pSymClass->CurIndex + 1) * sizeof (symbase_t));
    if ((hSymCl = MemAllocate (size)) == 0) {
        pExState->err_num = ERR_NOMEMORY;
        return (SCN_error);
    }
    pSymCl = (psymclass_t) MemLock (hSymCl);
    memmove (pSymCl, pSymClass, size);
    pSymCl->MaxIndex = (ulong )(pSymCl->CurIndex + 1);
    pSymCl->hNext = pName->hFound;
    pName->hFound = hSymCl;
    pName->cFound++;
    pSymClass->viable = FALSE;
    MemUnLock (hSymCl);
    return (SCN_found);
}




/**     FindIntro - find introducing virtual method
 *
 *      status = FindIntro (pName)
 *
 *      Entry   pName = structure describing search
 *              pSymClass = structure describing class search
 *
 *      Exit    pSymClass updated to reflect introducing virtual method
 *
 *      Returns SCN_found if introducing virtual method found
 *              SCN_error if introducing virtual method not found
 */


SCN_t
FindIntro (
    psearch_t pName
    )
{
    ulong       oldmask;
    SCN_t       retval;
    bool_t      fSupBaseSaved;
    unsigned char   cbNameSaved;

    oldmask       = pName->clsmask;
    cbNameSaved   = pName->sstr.cb;
    fSupBaseSaved = pExState->state.fSupBase;

    // CV400 #2863 look at all bases  [rm]
    pExState->state.fSupBase = FALSE;

    // CV400 #2863
    // destructor's don't have same name look only for the '~'  [rm]

    if (pName->sstr.lpName[0] == '~')
        pName->sstr.cb = 1;

    // limit searches to introducing virtual methods
    // the CLS_vfunc is set so that we pick up the vfuncptr information

    pName->clsmask = CLS_virtintro | CLS_vfunc | CLS_method;
    retval = SearchBases (pName);
    pName->clsmask = oldmask;

    // restore name length and fSupBase [rm]

    pExState->state.fSupBase = fSupBaseSaved;
    pName->sstr.cb           = cbNameSaved;
    return (retval);
}


/***    GenQualName - generate qualified method name
 *
 *      handle = GenQualName (pName, pSymClass)
 *
 *      Entry   pSymCLass = pointer to struct describing search
 *
 *      Exit    qualified function name generated
 *
 *      Returns 0 if name string not generated
 *              handle if name string generated
 */


HDEP
GenQualName (
    psearch_t pName,
    psymclass_t pSymCl
    )
{
    HDEP        hQual;
    char       *pQual;
    uint        buflen = 255;
    uint        len;
    uint        fSkip;
    char       *pField;       // pointer to field list
    HTYPE       hBase;          // handle to type record for base class
    char       *pc;
    int         i;
    peval_t     pL;

    if ((hQual = MemAllocate (buflen + 1)) == 0) {
        // unable to allocate memory
        pExState->err_num = ERR_NOMEMORY;
        return (0);
    }
    pQual = (char *) MemLock (hQual);
    memset (pQual, 0, 256);

    // walk up list of search structures adding qualifiers

    for (i = 0; i <= pSymCl->CurIndex; i++) {
        if (CLASS_PROP (&pSymCl->symbase[i].Base).cnested != 0) {
            NOTTESTED (FALSE);
        }
        if ((pSymCl->symbase[i].clsmask & CLS_virtintro) == CLS_virtintro) {
            // if the search turned into a search for the introducing virtual
            // function, break out of this loop so that the method name is
            // correct

            break;
        }
        pL = &pSymCl->symbase[i].Base;
    }

    // copy name of last class encountered

    if ((hBase = THGetTypeFromIndex (EVAL_MOD (pL), EVAL_TYP (pL))) == 0) {
        pExState->err_num = ERR_INTERNAL;
        return (0);
    }
    pField = (char *)(&((TYPPTR)MHOmfLock (hBase))->leaf);
    switch (((plfClass)pField)->leaf) {
        case LF_CLASS:
        case LF_STRUCTURE:
            fSkip = offsetof (lfClass, data);
            RNumLeaf (pField + fSkip, &fSkip);
            pc = pField + fSkip;
            break;

        case LF_UNION:
            fSkip = offsetof (lfUnion, data);
            RNumLeaf (pField + fSkip, &fSkip);
            pc = pField + fSkip;
            break;

        default:
            DASSERT (FALSE);
            MHOmfUnLock (hBase);
            MemUnLock (hQual);
            return (hQual);
    }
    len = *pc++;
    if ((len + 2) <= buflen) {
        memcpy (pQual, pc, len);
        pQual += len;
        *pQual++ = ':';
        *pQual++ = ':';
        buflen -= len + 2;
    }
    memcpy (pQual, pName->sstr.lpName, pName->sstr.cb);
    MHOmfUnLock (hBase);
    MemUnLock (hQual);
    return (hQual);
}




/***    pvThisFromST - generate expression for null path to qualified name
 *
 *      status = pvThisFromST (bnOp)
 *
 *      Entry   bnOp = node to add init expression to
 *
 *      Exit    qualified path expression generated
 *
 *      Returns TRUE if expression generated
 *              FALSE if no memory
 */


bool_t
pvThisFromST (
    bnode_t bnOp
    )
{
    uint        len;
    pnode_t     Parent;
    peval_t     pvOp;
    int         diff;

    if (ST == NULL) {
        return (FALSE);
    }

    // we need to generate an expression tree attached to the operator node
    // which will compute the null this pointer adjustment from ST.  This
    // routine is used for pClass->Class::member

    len = sizeof (node_t) + sizeof (adjust_t);
    if ((diff = pTree->size - pTree->node_next - len) < 0) {
        if (!GrowETree ((uint) (-diff))) {
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
        if (bnCxt != 0) {
            // the context was pointing into the expression tree.
            // since the expression tree could have been reallocated,
            // we must recompute the context pointer

           pCxt = SHpCXTFrompCXF ((PCXF)&((pnode_t)bnCxt)->v[0]);
        }
    }
    pvOp = &bnOp->v[0];
    EVAL_IS_MEMBER (pvOp) = TRUE;
    MEMBER_THISEXPR (pvOp) = pTree->node_next;
    Parent = NULL;

    // now add the node that causes pvThis to be initialized with the
    // value of ST.

    AddETInit (Parent, EVAL_TYP (ST));
    return (TRUE);
}




/***    GenQualExpr - generate expression for path to qualified name
 *
 *      status = GenQualExpr (pName)
 *
 *      Entry   pName = pointer to struct describing search
 *              pSymClass = pointer to class path structure
 *
 *      Exit    qualified path expression generated
 *
 *      Returns SCN_found is expression generated
 *              SCN_error if no memory
 */


SCN_t
GenQualExpr (
    psearch_t pName
    )
{
    int         i;
    peval_t     pvOp;
    uint        len;
    pnode_t     Parent;
    int         diff;
    peval_t     pvB;
    CV_typ_t    btype;
    UOFFSET     off;
    uint        pvOff = 0;

    // we need to generate an expression tree attached to the operator node
    // which will compute the this pointer adjustment.  The number
    // of nodes is potentially the number of base classes + an
    // initializer.

    // M00OPTIMIZE - if the final base is a direct or indirect virtual base
    // M00OPTIMIZE - of the inital class, then the entire expression can be
    // M00OPTIMIZE - reduced to one node + the initializer

    len = (pSymClass->CurIndex + 2) * (sizeof (node_t) + sizeof (adjust_t));
    pvOp = &pName->bnOp->v[0];
    if ((diff = pTree->size - pTree->node_next - len) < 0) {
        if (((char *)(pName->pv) >= (char *)pTree) &&
          ((char *)(pName->pv) < ((char *)pTree) + pTree->size)) {
            pvOff = (uint)(((char *)pName->pv) - ((char *)pTree));
        }
        if (!GrowETree ((uint) (-diff))) {
            pExState->err_num = ERR_NOMEMORY;
            return (SCN_error);
        }
        if (bnCxt != 0) {
            // the context was pointing into the expression tree.
            // since the expression tree could have been reallocated,
            // we must recompute the context pointer

           pCxt = SHpCXTFrompCXF ((PCXF)&((pnode_t)bnCxt)->v[0]);
        }
        if (pvOff != 0) {
            pName->pv = (peval_t)(((char *)pTree) + pvOff);
        }
        pvOp = &pName->bnOp->v[0];
    }

    // walk the path backwards generating the expression required to do
    // the adjustment.  The backwards walk is because evaluation is depth
    // first.

    MEMBER_THISEXPR (pvOp) = pTree->node_next;
    Parent = NULL;
    for (i = pSymClass->CurIndex; i > 0; i--) {
        pvB = &pSymClass->symbase[i].Base;
        btype = EVAL_TYP (pvB);
        off = pSymClass->symbase[i].thisadjust;
        if (pSymClass->symbase[i].fVirtual == TRUE) {
            Parent = AddETExpr (Parent, pSymClass->symbase[i].vbptr,
              pSymClass->symbase[i].vbpoff, off, btype);
        } else {
            Parent = AddETConst (Parent, off, btype);
        }
    }

    // add the first real adjustment if necessary.  What is really happening
    // here is that the the expression was of the form x.CLASS::member and
    // we need an adjustment from the object to the start of the base class

    pvB = &pSymClass->evalP;
    btype = EVAL_TYP (pvB);
    if ((EVAL_STATE (pvB) == EV_type) && EVAL_IS_CLASS (pvB)) {
        Parent = AddETConst (Parent, pSymClass->offset, btype);
    }

    // now add the node that causes pvThis to be initialized with the
    // value of ST.

    pvB = &pSymClass->symbase[i].Base;
    btype = EVAL_TYP (pvB);
    Parent = AddETInit (Parent, btype);
    return (SCN_found);
}




/**     GetArgType - get type of argument to function
 *
 *      fSuccess = GetArgType (pvF, argn, ptype);
 *
 *      Entry   pvF = pointer to function description
 *              argn = argument index (0-based)
 *              ptype = pointer to location to store type
 *
 *      Exit    *ptype = type of argument
 *              *ptype = 0 if vararg
 *
 *      Returns FARG_error if error
 *              FARG_none if no arguments
 *              FARG_vararg if varargs allowed
 *              FARG_exact if exact list
 */


farg_t
GetArgType (
    peval_t pvF,
    int argn,
    CV_typ_t *type
    )
{
    HTYPE           hList;          // handle of the type list
    plfArgList      pList = NULL;
    register farg_t retval;

    if (FCN_PCOUNT (pvF) == 0) {
        // there are no formals.  We need to check for varargs

        *type = 0;
        if (FCN_VARARGS (pvF) == TRUE) {
            return (FARG_vararg);
        }
        else {
            return (FARG_none);
        }
    }

    // set hList to the handle of the type list

    if ((hList = THGetTypeFromIndex (EVAL_MOD (pvF), FCN_PINDEX (pvF))) == NULL) {
        DASSERT (FALSE);
        return (FARG_error);
    }
    if (argn >= FCN_PCOUNT (pvF) - 1) {
        // this argument can possibly be a vararg.

        if (FCN_VARARGS (pvF) == TRUE) {
            *type = 0;
            retval = FARG_vararg;
        }
        else if (argn > FCN_PCOUNT (pvF)) {
            // varargs are not allowed and the maximum arg count is exceeded
            retval = FARG_error;
        }
        else {
            // varargs are not allowed and this is the last argument
            pList = (plfArgList)(&((TYPPTR)MHOmfLock (hList))->leaf);
            *type = pList->arg[argn];
            retval = FARG_exact;
        }
    }
    else {
        // this is before the last argument so it cannot be a vararg
        // load type list and store type of argument

        pList = (plfArgList)(&((TYPPTR)MHOmfLock (hList))->leaf);
        *type = pList->arg[argn];
        retval = FARG_exact;
    }
    if (pList != NULL) {
        MHOmfUnLock (hList);
    }
    return (retval);
}




/**     VBSearched - check to see if virtual base hase already been searched
 *
 *      flag = VBSearched (type)
 *
 *      Entry   type = type index of virtual base
 *
 *      Exit    none
 *
 *      Returns TRUE if virtual base has already been searched
 *              FALSE if virtual base has not been searched
 */


bool_t
VBSearched (
    CV_typ_t type
    )
{
    ulong       i;

    for (i = 0; i < pVBSearch->CurIndex; i++) {
        if (pVBSearch->dom[i] == type) {
            return (TRUE);
        }
    }
    return (FALSE);
}





/**     GrowTMList - grow TML
 *
 *      fSuccess = GrowTMList (void)
 *
 *      Entry   pTMLbp = pointer to TML
 *              pTMList = pointer to TM list
 *
 *      Exit    TML grown
 *              pTMList = pointer to locked table
 *
 *      Returns TRUE if table grown
 *              FALSE if out of memory
 */


bool_t
GrowTMList (
    void
    )
{
    register bool_t retval = FALSE;
    HDEP            hTMList;

    MemUnLock (pTMLbp->hTMList);
    if ((hTMList = MemReAlloc (pTMLbp->hTMList,
      (pTMLbp->cTMListMax + TMLISTCNT) * sizeof (HTM))) != 0) {
        pTMLbp->hTMList = hTMList;
        pTMLbp->cTMListMax += TMLISTCNT;
        retval = TRUE;
    }
    // lock list to maintain lock/unlock synchronization

    pTMList = (HDEP *) MemLock (pTMLbp->hTMList);
    return (retval);
}




/**     IncrSymBase - increment symbase_t index and grow if necessary
 *
 *      status = IncrSymbase ();
 *
 *      Entry   none
 *
 *      Exit    *pSymClass->CurIndex incremented
 *              pSymBase grown if necessary
 *
 *      Returns SCN_found if pSymClass->CurIndex incremented
 *              SCN_error if unable to allocate memory for duplication
 */


SCN_t
IncrSymBase (
    void
    )
{
    uint        size;
    HDEP        hNew;

    if (++pSymClass->CurIndex >= pSymClass->MaxIndex) {
        size = sizeof (symclass_t) + (pSymClass->CurIndex + 5) * sizeof (symbase_t);
        MemUnLock (hSymClass);
        if ((hNew = MemReAlloc (hSymClass, size)) == 0) {
            pExState->err_num = ERR_NOMEMORY;
            pSymClass = (psymclass_t) MemLock (hSymClass);
            pSymClass->CurIndex--;
            return (SCN_error);
        }
        hSymClass = hNew;
        pSymClass = (psymclass_t) MemLock (hSymClass);
        pSymClass->MaxIndex += 5;
    }
    return (SCN_found);
}



/**     IsDominated - check to see of feature is dominated
 *
 *      fStatus = IsDominated (pSymBest, pSymTest)
 *
 *      Entry   pName = structure describing search
 *              pSymClass = structure describing most recent class search
 *
 *      Exit    pName updated to reflect search
 *              pSymClass updated to unambiguous feature
 *
 *      Returns DOM_replace if pSymBest is dominated by pSymTest
 *              DOM_keep if pSymBest is exactly equivalent to pSymTest
 *              DOM_ambiguous if pSymBest is equal to pSymTest
 *              DOM_keep if pSymTest is duplicate virtual base
 */


DOM_t
IsDominated (
    psymclass_t pSymBest,
    psymclass_t pSymTest
    )
{
    int         i;
    int         j;

    if (pSymBest->isdupvbase == TRUE) {
        return (DOM_replace);
    }
    if (pSymTest->isdupvbase == TRUE) {
        return (DOM_keep);
    }

    // check all of the base classes for a dominated virtual base

    for (i = 0; i <= pSymBest->CurIndex; i++) {
        if (pSymBest->symbase[i].fVirtual == TRUE) {
            for (j = pVBDom->CurIndex; j >= 0; j--) {
                if (pVBDom->dom[j] == EVAL_TYP (&pSymBest->symbase[i].Base)) {
                    return (DOM_replace);
                }
            }
        }
    }
    for (i = 0; i <= pSymTest->CurIndex; i++) {
        if (pSymTest->symbase[i].fVirtual == TRUE) {
            for (j = pVBDom->CurIndex; j >= 0; j--) {
                if (pVBDom->dom[j] == EVAL_TYP (&pSymTest->symbase[i].Base)) {
                    return (DOM_keep);
                }
            }
        }
    }

    // we have two paths to potentially two different features.  We now check
    // to see if the features are identical.  This is done by checking for
    // static data members that have identical addresses and types

    if (!EVAL_IS_STMEMBER (&pSymBest->evalP) ||
      !EVAL_IS_STMEMBER (&pSymTest->evalP) ||
      (EVAL_TYP (&pSymBest->evalP) != EVAL_TYP (&pSymTest->evalP)) ||
      (EVAL_SYM_SEG (&pSymBest->evalP) != EVAL_SYM_SEG (&pSymTest->evalP)) ||
      (EVAL_SYM_OFF (&pSymBest->evalP) != EVAL_SYM_OFF (&pSymTest->evalP))) {
        return (DOM_ambiguous);
    }
    return (DOM_keep);
}


/**     IsIntroVirt - is this the introducing virtual method
 *      Entry   mlist = type index of lf mfunc
 *              pvtabind = pointer to virtual function table index
 *
 *      Exit    *pvtabind = virtual fuction table index
 *
 *      Returns TRUE if the introducing virtual method is found
 *              FALSE otherwise
 */
bool_t
IsIntroVirt(
    CV_typ_t tiMfunc
    )
{
    HTYPE           hMethod;
    HMOD            hMod;
    bool_t          retval;
    peval_t         pvF;
    plfMFunc        pMFunc;

    pvF = &pSymClass->evalP;
    hMod = EVAL_MOD (pvF);

    if ((hMethod = THGetTypeFromIndex (hMod, tiMfunc)) == NULL) {
        DASSERT (FALSE);
        return (FALSE);
    }
    pMFunc = (plfMFunc)((&((TYPPTR)MHOmfLock (hMethod))->leaf));
    retval = (pMFunc->rvtype == FCN_RETURN (pvF)) &&
        (pMFunc->arglist == FCN_PINDEX (pvF));
    MHOmfUnLock (hMethod);
    return retval == TRUE;
}

/**     IsIntroVirtInMlist - is this the introducing virtual method in this mlist
 *
 *      fSuccess = IsIntroVirtInMlist (count, mlist, pvtabindex)
 *
 *      Entry   count = number of methods in list
 *              mlist = type index of method list
 *              pvtabind = pointer to virtual function table index
 *
 *      Exit    *pvtabind = virtual fuction table index
 *
 *      Returns TRUE if the introducing virtual method is found
 *              FALSE otherwise
 */


bool_t
IsIntroVirtInMlist(
    ulong  count,
    CV_typ_t mlist,
    UOFFSET *pvtabind
    )
{
    HTYPE           hMethod;
    pmlMethod       pMethod;
    uint            cbSkip;
    bool_t          Mmatch = 0;
    peval_t         pvF;
    HMOD            hMod;

    pvF = &pSymClass->evalP;
    hMod = EVAL_MOD (pvF);

    // we now walk the list of methods looking for a method with the same
    // return type and argument list type

    if ((hMethod = THGetTypeFromIndex (hMod, mlist)) == NULL) {
        DASSERT (FALSE);
        return (FALSE);
    }

    // set the number of methods overloaded on this name and the index into
    // the method list

    cbSkip = 0;
    *pvtabind = 0;

    while (count-- > 0) {
        // lock the omf record, extract information for the current entry in
        // the method list and increment to field for next method

        plfMethodList   plfMList;

        plfMList = (plfMethodList) &((TYPPTR) MHOmfLock(hMethod))->leaf;
        pMethod = (pmlMethod) &plfMList->mList[cbSkip];

        cbSkip += sizeof (mlMethod);
        if (fIntroducingVirtual(pMethod->attr.mprop)) {
            cbSkip += sizeof(pMethod->vbaseoff[0]);
            if (IsIntroVirt(pMethod->index)) {
                *pvtabind = pMethod->vbaseoff[0];
                MHOmfUnLock(hMethod);
                return TRUE;
            }
        }
        MHOmfUnLock (hMethod);
    }
    return (FALSE);
}


/** MatchArgs - match a method against argument list
 *
 *      flag = MatchArgs (pv, pName, fattr, vtabind, fForce)
 *
 *      Entry   pv = pointer to function descriptor
 *              pName = pointer to search structure
 *              attr = attribute if method
 *              vtabind = offset into vtable if virtual method
 *              fForce = TRUE if match on first (only) method is forced
 *                       This is the case if bp method, no args and single
 *                       method.
 *              BindingBP = TRUE if call is via ParseBP API entry
 *                  if BindingBP is true, then only exact matches are allowed
 *              bArgList = based pointer to argument list
 *
 *      Exit    ST = resolved address of method
 *
 *      Returns TRUE if a match was found or ambiguous references are allowed
 *              FALSE if no match or ambiguity not allowed
 */


void
MatchArgs (
    peval_t pvM,
    psearch_t pName,
    CV_fldattr_t attr,
    UOFFSET vtabind,
    bool_t fForce
    )
{
    pnode_t     pnT;
    pargd_t     pa;
    bool_t      update = FALSE;
    argcounters current;
    int         argc;
    CV_typ_t    Ftype;
    bool_t      argmatch;
    eval_t      evalArg;
    peval_t     pvArg = &evalArg;

    // walk down the formal list specified by pvM and the actual list
    // specified by bArgList.  Initialize the conversion counters to
    // zero and set the index to varargs parameter to zero

    argc = 0;
    current.exact = 0;
    current.implicit = 0;
    current.varargs = 0;
    argmatch = FALSE;
    pnT = (pnode_t)bArgList;

    (pName->overloadCount)++;
    if (fForce == TRUE) {
        argmatch = TRUE;
        update = TRUE;
    }
    else if (!N_EVAL_IS_FCN(pvM)) {
        argmatch = TRUE;
        current.exact = 0;
    }
    else while (TRUE) {
        if (NODE_OP (pnT) == OP_endofargs) {
            if ((argc == FCN_PCOUNT (pvM)) ||
              ((argc >= (FCN_PCOUNT (pvM) - 1)) && (FCN_VARARGS (pvM) == TRUE))) {
                argmatch = TRUE;
            }
            break;
        }
        if ((argc > FCN_PCOUNT (pvM)) && (FCN_VARARGS (pvM) == FALSE)) {
            // this function cannot possibly be a match because the
            // number of actuals is greater than the number of formals
            // and the method does not allow varargs.

            break;
        }
        // pa points to the argument descriptor array in the OP_arg node
        pa = (pargd_t)&(pnT->v[0]);
        switch (GetArgType (pvM, argc, &Ftype)) {
            case FARG_error:
                DASSERT (FALSE);
                return;

            case FARG_none:
                // special case foo(void)
                // sps 2/20/92
                if ((argc == 0) && (pa->actual == T_VOID))  {
                    argmatch = TRUE;
                    goto argmatchloop;
                    }
                return;

            case FARG_vararg:
                // varargs are allowed unless we are binding breakpoints
                if (BindingBP == TRUE) {
                    return;
                }
                break;

            case FARG_exact:
                if ((argc + 1) > FCN_PCOUNT (pvM)) {
                    // we must exactly match the argument count and we
                    // have more arguments
                    return;
                }
                break;

        }
        argc++;


        if (Ftype == 0) {
            // all arguments from this point on are vararg
            current.varargs = argc;
            pa->current = OM_vararg;
        }
        else {
            CV_typ_t Atype = pa->actual;
            eval_t vA, vF;
            peval_t pvA = &vA;
            peval_t pvF = &vF;

            if (pa->flags.istype) {
                // If we have a type want identical type indices
                // (This helps with class dumps)
#if CC_LAZYTYPES
                if ( THAreTypesEqual( EVAL_MOD( pvM ), Atype, Ftype ) )
#else
                if ( Atype == Ftype )
#endif
                {
                    current.exact++;
                    pa->current = OM_exact;
                    pnT = (pnode_t)NODE_RCHILD (pnT);
                    continue;
                }
                else {
                    pExState->err_num = ERR_NONE;
                    break;
                }
            }

            Ftype = SkipModifiers(EVAL_MOD(pvM), Ftype);
            Atype = SkipModifiers(EVAL_MOD(pvM), Atype);

            *pvF = *pvM;
            if (!SetNodeType(pvF, Ftype)) {
                // error (not valid type)
                break;
            }

            // Update Ftype (SetNodeType must have resolved potential fwd refs)
            Ftype = EVAL_TYP (pvF);

            // if we are calling a 32 bit func we must promote all const
            // int2's to int4's

            if (EVAL_SYM_IS32(pvM) &&
                (EVAL_STATE((peval_t)(&((pnode_t)NODE_LCHILD (pnT))->v[0])) == EV_constant) &&
                ((Atype == T_INT2) || (Atype == T_UINT2))
            ) {
                Atype++;
            }

            *pvA = *pvM;
            if (!SetNodeType(pvA, Atype)) {
                // error (not valid type)
                break;
            }


            if (EVAL_IS_PTR(pvA) && EVAL_IS_REF(pvA)) {
                Atype = SkipModifiers(EVAL_MOD(pvM), PTR_UTYPE(pvA));
            }

#if CC_LAZYTYPES
            if ( THAreTypesEqual( EVAL_MOD( pvM ), Atype, Ftype ) )
#else
            if (Atype == Ftype)
#endif
            {
                current.exact++;
                pa->current = OM_exact;
                pnT = (pnode_t)NODE_RCHILD (pnT);
                continue;
            }

            if (EVAL_IS_PTR(pvF)) {
                *pvA = *pvM;
                if (!SetNodeType(pvA, Atype)) {
                    // error (not valid type)
                    break;
                }
                if (!EVAL_IS_REF(pvF)) {
                    if (!EVAL_IS_PTR(pvA))  {
                        // do not cast a non-pointer to pointer
                        break;
                    }

                    PTR_UTYPE(pvA) = SkipModifiers(EVAL_MOD(pvM), PTR_UTYPE(pvA));
                    PTR_UTYPE(pvF) = SkipModifiers(EVAL_MOD(pvM), PTR_UTYPE(pvF));

                    if (EVAL_PTRTYPE(pvA) == EVAL_PTRTYPE(pvF) &&
#if CC_LAZYTYPES
                        THAreTypesEqual( EVAL_MOD( pvA ), PTR_UTYPE(pvA), PTR_UTYPE(pvA) ) &&
#else
                        PTR_UTYPE(pvA) == PTR_UTYPE(pvF) &&
#endif
                        !EVAL_IS_BASED(pvA) &&
                        !EVAL_IS_BASED(pvF)) {

                        // we don't match based pointers. if we want to
                        // do this we must examine additional information
                        // we allow mathcing a pointer with an array
                        // however all arrays are considered far
                        // since codeview information does not distinguish
                        // between far or near arrays. --gdp 10/16/92

                        current.exact++;
                        pa->current = OM_exact;
                        pnT = (pnode_t)NODE_RCHILD (pnT);
                        continue;
                    }
                }
                else {

                    //
                    // special handling of a reference
                    //

                    CV_typ_t Utype;

                    Utype = SkipModifiers(EVAL_MOD(pvM), PTR_UTYPE (pvF));
#if CC_LAZYTYPES
                    if ( THAreTypesEqual( EVAL_MOD( pvM ), Utype, Atype ) )
#else
                    if (Utype == Atype)
#endif
                    {
                        current.exact++;
                        pa->current = OM_exact;
                        pnT = (pnode_t)NODE_RCHILD (pnT);
                        continue;
                    }
                    else {
                        break;
                    }
                }
            }


            // see if we can cast the type of the actual to the type of
            // the formal if we are not binding breakpoints.  If we are
            // binding breakpoints, only exact matches are allowed

            *pvArg = *pvM;
            SetNodeType (pvArg, pa->actual);
            if (EVAL_IS_BITF (pvArg)) {
                SetNodeType (pvArg, BITF_UTYPE (pvArg));
            }
            if (CastNode (pvArg, Ftype, Ftype)) {
                pa->current = OM_implicit;
                current.implicit++;
            }
            else {
                pExState->err_num = ERR_NONE;
                break;
            }
        }
        pnT = (pnode_t)NODE_RCHILD (pnT);
    }
argmatchloop:
    if (argmatch == TRUE) {
        // check current against best match

        if (pName->best.match == 0) {
            // we have a first time match
            update = TRUE;
            pName->possibles++;
        }
        else if (fForce == FALSE) {
            // we have already matched on some function so we now
            // check to see if the current is better than the best
            if (current.varargs == 0) {
                // there are no varargs in this method therefor it is
                // better than any previous match with varargs
                if (pName->best.varargs != 0) {
                    update = TRUE;
                    pName->best = current;
                    pName->possibles = 1;
                }
                else if (current.exact > pName->best.exact) {
                    // this function has more exact matches than the
                    // best one so far
                    update = TRUE;
                    pName->possibles = 1;
                }
#if CC_LAZYTYPES
                else if ( !THAreTypesEqual( EVAL_MOD( pvM ), EVAL_TYP (pvM), pName->best.match  ) &&
#else
                else if (EVAL_TYP (pvM) != pName->best.match &&
#endif
                    current.exact == pName->best.exact) {
                    // this function is a good as the best one.
                    // this means the call is ambiguous
                    pName->possibles++;
                }
            }
            else {
                if (pName->best.varargs != 0) {
                    // both the current and the best functions have
                    // varargs

                    if (current.varargs < pName->best.varargs) {
                        // this function uses fewer varargs which
                        // makes it a better one
                        update = TRUE;
                        pName->possibles = 1;
                    }
                    else if (current.varargs == pName->best.varargs) {
                        // this function uses the same number of varargs
                        // pick the one with more exact matches
                        if (current.exact > pName->best.exact) {
                            update = TRUE;
                            pName->possibles = 1;
                        }
#if CC_LAZYTYPES
                        else if ( !THAreTypesEqual( EVAL_MOD( pvM ), EVAL_TYP (pvM), pName->best.match  ) &&
#else
                        else if (EVAL_TYP (pvM) != pName->best.match &&
#endif
                                current.exact == pName->best.exact) {
                            pName->possibles++;
                        }
                    }
                }
            }
        }
        if (update == TRUE) {
            if (fForce == FALSE) {
                // the current match is better
                for (pnT = (pnode_t)bArgList; NODE_OP (pnT) != OP_endofargs; pnT = (pnode_t)NODE_RCHILD (pnT)) {
                    pa = (pargd_t)&(pnT->v[0]);
                    pa->best = pa->current;
                }
                {       //M00KLUDGE - hack for compiler DS != SS bug
                    argcounters *pcurrent = &current;
                    pName->best = *pcurrent;
                }
            }
            pName->best.match = EVAL_TYP (pvM);
            pName->best.attr = attr;
            pName->best.vtabind = vtabind;
            pName->best.hSym = pName->hSym;
            pName->best.CXTT = pName->CXTT;
        }
    }
}


CV_typ_t
SkipModifiers(
    HMOD mod,
    CV_typ_t type
    )
{
    HTYPE       hType;
    plfModifier     pType;

    if (!CV_IS_PRIMITIVE (type)) {
    hType = THGetTypeFromIndex (mod, type);
    pType = (plfModifier)((&((TYPPTR)MHOmfLock (hType))->leaf));
    while (pType->leaf == LF_MODIFIER) {
        type =  pType->type;
        if (CV_IS_PRIMITIVE (type)) {
            break;
        }
        MHOmfUnLock (hType);
        hType = THGetTypeFromIndex (mod, type);
        pType = (plfModifier)((&((TYPPTR)MHOmfLock (hType))->leaf));
    }
    }

    MHOmfUnLock (hType);
    return type;
}



/**     MatchFunction - match a function against argument list
 *
 *      flag = MatchFunction (pName)
 *
 *      Entry   pName = pointer to search status
 *              BindingBP = TRUE if call is via ParseBP API entry
 *              bArgList = based pointer to argument list
 *
 *      Exit    pv = resolved address of method
 *
 *      Returns HR_... describing match result
 */


HR_t FASTCALL
MatchFunction (
    psearch_t pName
    )
{
    HR_t        retval;
    CV_fldattr_t attr = {0};
    CV_typ_t    type;
    ADDR        addr = {0};
    bool_t      dupfcn;

    if (bArgList == NULL) {
        // if there is no argument list, assume we are assigning to a
        // function pointer and take the first function found.  Let the
        // user beware.

        return (HR_found);
    }

    // save the function address and type.  Then if a subsequent match has
    // the same address and type (i.e., comdat code), we can ignore it.

    addr = EVAL_SYM (ST);
    type = EVAL_TYP (ST);

    // pop the entry that SearchSym has added to the evaluation stack
    // the best match will be pushed back on later

    pName->scope |= SCP_nomatchfn;

    PopStack ();
    MatchArgs (pName->pv, pName, attr, 0, FALSE);
    while ((retval = SearchSym (pName)) != HR_notfound) {
        DASSERT (retval != HR_end);
        dupfcn = FALSE;
        if (retval == HR_found) {
            // Mips publics for C functions are not decorated so don't
            // bother with type-check, just see if the addresses are the same
            if (memcmp (&addr, &EVAL_SYM (ST), sizeof (addr)) == 0) {
                dupfcn = TRUE;
            }
            PopStack ();
            if (dupfcn == FALSE) {
                MatchArgs (pName->pv, pName, attr, 0, FALSE);
            }
        }
        else {
            return (retval);
        }
    }

    pName->scope &= ~SCP_nomatchfn;

    // clear the symbol not found error from the recursive symbol search

    pExState->err_num = ERR_NONE;
    if (pName->best.match == 0) {
        pExState->err_num = ERR_ARGLIST;
        return (HR_error);
    }
    else if (pName->possibles > 1) {
        PushStack (pName->pv);
        pExState->err_num = ERR_AMBIGUOUS;
        EVAL_IS_AMBIGUOUS (ST) = TRUE;
        return (HR_ambiguous);
    }
    else if (pName->overloadCount > 1 && pName->best.implicit > 0) {
        //
        // overloaded function that involves implicit casts
        //
        pExState->err_num = ERR_AMBIGUOUS;
        return (HR_ambiguous);
    }
    else {
        pName->hSym = pName->best.hSym;
        pName->CXTT = pName->best.CXTT;
        if ((SymToNode (pName) == FALSE) || (PushStack (pName->pv) == FALSE)) {
            return (HR_notfound);
        }
        else {
            return (HR_found);
        }
    }
}




/**     MatchMethod - match a method against argument list
 *
 *      flag = MatchMethod (pName, pSymCl)
 *
 *      Entry   pName = pointer to search descriptor
 *              pSymCl = pointer tp symbol class structure
 *              select = selection masj
 *              BindingBP = TRUE if call is via ParseBP API entry
 *                  if BindingBP is true, then only exact matches allowed
 *              bArgList = based pointer to argument list
 *
 *      Exit    pName->pv = resolved address of method
 *
 *      Returns MTH_found if a match was found or ambiguous references are allowed
 *              MTH_error if error or ambiguity not allowed
 *
 */


static bool_t
setupMatchArgs(
    psearch_t pName,
    CV_fldattr_t attr,
    UOFFSET vtabind,
    bool_t fForce,
    CV_typ_t type,
    HMOD hMod
    )
{
    eval_t          evalM;
    peval_t         pvM = &evalM;
    // now compare the actual and formal argument lists for the function
    // pvM will be initialized to be a function node whose type index is
    // type.

    CLEAR_EVAL (pvM);
    EVAL_MOD (pvM) = hMod;
    if (SetNodeType (pvM, type) == FALSE) {
        // the type record for the method was not found
        DASSERT (FALSE);
        return FALSE;
        }
    MatchArgs (pvM, pName, attr, vtabind, fForce);
    return TRUE;
}

MTH_t
MatchMethod (
    psearch_t pName,
    psymclass_t pSymCl
    )
{
    HTYPE           hMethod;
    pmlMethod       pMethod;
    uint            skip;
    CV_fldattr_t    attr;
    CV_typ_t        type;
    UOFFSET         vtabind;
    bool_t          Mmatch = 0;
    HDEP            hQual;
    peval_t         pvF;
    ulong           count = pSymCl->possibles;
    peval_t         pvB;
    HMOD            hMod;
    bool_t          fForce = FALSE;
    TYPPTR          ptype;

    pvB = &pSymClass->symbase[pSymClass->CurIndex].Base;
    hMod = EVAL_MOD (pvB);
    pName->possibles = 0;
    pvF = &pSymCl->evalP;

    // we now walk the list of methods looking for an argument match
    // For now, we require an exact match except that we assume any
    // primitive type can be cast to any other primitive type.  A cast from
    // pointer to derived to pointer to base is an implicit conversion

    if ((hMethod = THGetTypeFromIndex (hMod, pSymCl->mlist)) == NULL) {
        DASSERT (FALSE);
        return (MTH_error);
    }

    // set the number of methods overloaded on this name and the index into
    // the method list

    skip = 0;
    if (bArgList == NULL) {
        if (count == 1) {
            fForce = TRUE;
        }
        else {
            // there is not an argument list.  We allow breakpoints if a single
            // method by that name exists.  Otherwise, we require an argument
            // list

            pExState->err_num = ERR_NOARGLIST;
            return (MTH_error);
        }
    }

    ptype = (TYPPTR)MHOmfLock (hMethod);
    if (ptype->leaf == LF_METHODLIST) {
        MHOmfUnLock (hMethod);
        while (count-- > 0) {
            // lock the omf record, extract information for the current entry in
            // the method list and increment to field for next method

            pMethod = (pmlMethod)((&((TYPPTR)MHOmfLock (hMethod))->leaf) + 1);
            pMethod = (pmlMethod)((uchar *)pMethod + skip);
            attr = pMethod->attr;
            type = pMethod->index;
            skip += sizeof (mlMethod);
            if (fIntroducingVirtual(pMethod->attr.mprop)) {
                vtabind = pMethod->vbaseoff[0];
                skip += sizeof(pMethod->vbaseoff[0]);
            }
            else {
                vtabind = 0;
            }
            MHOmfUnLock (hMethod);
            if (!setupMatchArgs(pName, attr, vtabind, fForce, type, hMod))
                return (MTH_error);
        }
    }
    else {
        DASSERT(ptype->leaf == LF_MFUNCTION);
        DASSERT(count <= 1);
        MHOmfUnLock (hMethod);
        if (!setupMatchArgs(pName, pSymCl->access, FCN_VTABIND (pvF) , fForce, pSymCl->mlist, hMod))
            return (MTH_error);
    }

    // since the name was found at this level, it hides all other methods
    // by the same name above it in the inheritance tree.  Therefore we must
    // either have a match or an error because there is no match or two or
    // methods match after conversions are considered

    if (pName->best.match == 0) {
        pExState->err_num = ERR_ARGLIST;
        return (MTH_error);
    }
    else if (pName->possibles > 1) {
        pExState->err_num = ERR_AMBIGUOUS;
        EVAL_IS_AMBIGUOUS (pName->pv) = TRUE;
        return (MTH_error);
    }
    else if (pName->overloadCount > 1 && pName->best.implicit > 0) {
        // we have an overloaded method
        // in this case the EE requires exact matches only
        pExState->err_num = ERR_AMBIGUOUS;
        EVAL_IS_AMBIGUOUS (pName->pv) = TRUE;
        return (MTH_error);
    }
    else {
        // we have found a non-ambiguous match

        EVAL_MOD (pvF) = hMod;
        if (SetNodeType (pvF, pName->best.match) == FALSE) {
            return (MTH_error);
        }
        else {
            pSymCl->access.access = pName->best.attr.access;
            FCN_ATTR (pvF) = pName->best.attr;
            FCN_VTABIND (pvF) = pName->best.vtabind;
            FCN_VFPTYPE (pvF) = pSymCl->vfpType;
            if (NODE_OP (pName->bnOp) == OP_bscope) {
                // binary scoping switches off virtual function evaluation
                FCN_PROPERTY (pvF) = CV_MTvanilla;
            }
            if ((FCN_PROPERTY (pvF) == CV_MTvirtual) ||
              (FCN_PROPERTY (pvF) == CV_MTintro)) {
                // do nothing.  address will have to be found at evaluation
            }
            else {
                if (((hQual = GenQualName (pName, pSymClass)) == 0) ||
                  (SearchQualName (pName, pSymCl, hQual, TRUE) == FALSE)) {
                    if (hQual != 0) {
                        MemFree (hQual);
                    }
                    return (MTH_error);
                }
                MemFree (hQual);
            }
            return (MTH_found);
        }
    }
}


/**     MoveSymCl - move symbol class structure
 *
 *      MoveSymCl (hSymCl);
 *
 *      Entry   hSymCl = handle of symbol class structure
 *
 *      Exit    *pSymClass = symbol class structure
 *
 *      Returns none
 */

void
MoveSymCl (
    HDEP hSymCl
    )
{
    psymclass_t pSymCl;
    ulong       max;

    pSymCl = (psymclass_t) MemLock (hSymCl);
    DASSERT (pSymClass->MaxIndex >= pSymCl->CurIndex);
    max = pSymClass->MaxIndex;
    memmove (pSymClass, pSymCl,
      sizeof (symclass_t) + (pSymCl->CurIndex + 1) * sizeof (symbase_t));
    pSymClass->MaxIndex = max;
    MemUnLock (hSymCl);
    MemFree (hSymCl);
}


/**     PurgeAmbCl - purge ambiguous class structure list
 *
 *      PurgeAmbCl (pName)
 *
 *      Entry   pName = pointer to symbol search structure
 *
 *      Exit    pname->hAmb list freed
 *
 *      Returns none
 */

void
PurgeAmbCl (
    psearch_t pName
    )
{
    psymclass_t pAmbCl;
    HDEP        hNext;

    while (pName->hAmbCl != 0) {
        pAmbCl = (psymclass_t) MHMemLock (pName->hAmbCl);
        hNext = pAmbCl->hNext;
        MHMemUnLock (pName->hAmbCl);
        pName->hAmbCl = hNext;
    }
}


/***    SetBase - set base value in pSymClass->symbase array
 *
 *      status = SetBase (pName, type, vbptr, vbpoff, thisadjust, attr, virtual)
 *
 *      Entry   pName = pointer to struct describing search
 *              type = type index of class
 *              vbptr = type index of virtual base pointer
 *              vbpoff = offset of virtual base pointer from address point
 *              thisadjust = offset of base from previous class
 *              thisadjust = virtual base index if virtual base
 *              attr = field attribute mask
 *              virtual = TRUE if virtual base
 *
 *      Exit    new base class added to  pSymClass
 *
 *      Returns enum describing search state
 */

SCN_t
SetBase (
    psearch_t pName,
    CV_typ_t type,
    CV_typ_t vbptr,
    UOFFSET vbpoff,
    UOFFSET thisadjust,
    CV_fldattr_t attr,
    bool_t fVirtual
    )
{
    peval_t     pvB;

    // save offset of base from address point for this pointer adjustment

    pSymClass->symbase[pSymClass->CurIndex].thisadjust = thisadjust;
    pSymClass->symbase[pSymClass->CurIndex].vbptr = vbptr;
    pSymClass->symbase[pSymClass->CurIndex].vbpoff = vbpoff;
    pSymClass->symbase[pSymClass->CurIndex].attrBC = attr;
    pSymClass->symbase[pSymClass->CurIndex].fVirtual = fVirtual;
    pSymClass->symbase[pSymClass->CurIndex].clsmask = pName->clsmask;
    pvB = &pSymClass->symbase[pSymClass->CurIndex].Base;
    EVAL_MOD (pvB) = pName->hMod;
    EVAL_CXTT(pvB) = pName->CXTT;
    if (SetNodeType (pvB, type) == FALSE) {
        pExState->err_num = ERR_BADOMF;
        return (SCN_error);
    }
    return (SCN_found);
}




/***    SearchQualName - Search for a qualified method name
 *
 *      flag = SearchQualName (pName, pSymCl, hQual, fProc)
 *
 *      Entry   pName = pointer to struct describing search
 *              pSymCl = pointer to structure describing path to symbol
 *              hQual = handle to symbol name
 *              fProc = TRUE if proc symbol to be searched for
 *              fProc = FALSE if data to be searched for
 *
 *      Exit    pName updated to reflect search
 *
 *      Return  enum describing search
 */


bool_t
SearchQualName (
    psearch_t pName,
    psymclass_t pSymCl,
    HDEP hQual,
    bool_t fProc
    )
{
    HDEP        hTemp;
    psearch_t   pTemp;
    uchar      *pQual;
    ulong       retval;

    if ((hTemp = MemAllocate (sizeof (search_t))) == 0) {
        pExState->err_num = ERR_NOMEMORY;
        return (FALSE);
    }
    pTemp = (psearch_t) MemLock (hTemp);
    *pTemp = *pName;
    pTemp->CXTT = *pCxt;

    // set pointer to symbol name

    pQual = (uchar *)MemLock (hQual);
    pTemp->sstr.lpName = pQual;
    pTemp->sstr.cb = (uchar)_tcslen ((char *)pQual);
    pTemp->state = SYM_init;
    if (fProc == TRUE) {
        pTemp->scope = SCP_module | SCP_global;
        pTemp->sstr.searchmask |= SSTR_proc;
    }
    else {
        pTemp->scope = SCP_module | SCP_global | SCP_lexical;

        // data members may not be overloaded, hence there is
        // no need for the type to qualify the name.  Furthermore
        // there are cases where the symbol type doesn't match
        // the member type exactly (e.g. variable length arrays,
        // symbol has actual length and member has zero length)
        // hence this matching is suppressed for data items [rm]

        // pTemp->sstr.searchmask |= SSTR_data;
    }
    pTemp->initializer = INIT_qual;
    pTemp->typeOut = EVAL_TYP (&pSymCl->evalP);
    retval = SearchSym (pTemp);
    MemUnLock (hQual);
    if (retval != HR_found) {
#ifdef NEVER    /* CAVIAR #1081 */
        if ((retval != HR_end) && (pTemp->FcnBP != 0)) {
            // there is another symbol with the same name but a different
            // type.  Since this is set only when we are binding breakpoints,
            // let's go ahead and try it

            pTemp->state = SYM_init;
            pTemp->scope = SCP_module | SCP_global;
            pTemp->sstr.searchmask |= SSTR_proc;
            pTemp->typeOut = pTemp->FcnBP;
            retval = SearchSym (pTemp);
        }
#endif
        if (retval != HR_found) {
            // the only way we can get here is if the method or data is declared
            // in the class but is not defined or is a noninstianted inline method.
            // therefore, we return a special error code

            if (ST == NULL) {
                // this is a hack to get around the case where the expression
                //      bp {,foo.c, foo.exe}X::f
                // and the first function in the module foo is a method.
                // SearchSym ended up calling the class search routines
                // because the context is implicitly a class so class is
                // searched even though it shouldn't be.  Any fix for this
                // causes problems and we are too close to release to find
                // a valid fix.  The major result of this fix is that
                // some breakpoints will not get reset on entry.

                return (FALSE);
            }

            PushStack (ST);

            //  if this is a missing static DATA member function then
            // best.match won't have anything in it at all... we'll need
            // to pick up what we want from typeOut which will have been
            // set by a previous non-qualified name search... [rm]

            if (pName->best.match == T_NOTYPE) {
                SetNodeType (ST, pName->typeOut);
            }
            else {
                SetNodeType (ST, pName->best.match);
            }
            if (EVAL_IS_FCN(ST)) {
                FCN_NOTPRESENT (ST) = TRUE;
            }
            else {
                // indicate missing static data member
                EVAL_HSYM (ST) = 0;
                pExState->state.fNotPresent = TRUE;
            }
        }
    }
    // pop off the stack entry that a successful search found.  Move the
    // static data member flag first so that it will not be lost.

    EVAL_IS_STMEMBER (ST) = EVAL_IS_STMEMBER (&pSymCl->evalP);
    EVAL_ACCESS (ST) = (uchar)(pSymCl->access.access);
    pSymCl->evalP = *ST;
    MemUnLock (hTemp);
    MemFree (hTemp);
    return (PopStack ());
}


/**     SymAmbToList - add ambiguous symbol to list
 *
 *      status = SymAmbToList (pName)
 *
 *      Entry   pName = pointer to list describing search
 *
 *      Exit    ambiguous symbol added to list pointed to by pTMList
 *              pTMLbp
 *              pName reset to continue breakpoint search
 *
 *      Returns HR_... describing state
 */

HR_t
SymAmbToList (
    psearch_t pName
    )
{
    HDEP        hSearch;
    psearch_t   pSearch;
    HR_t        retval;
    HDEP        hevalT;

    PopStack ();
    if (pExState->ambiguous == 0) {
        // this is the first breakpoint symbol found.
        // indicate the only node in the tree that is
        // allowed ambiguity and initialize list of
        // symbols for later back patching into duplicated
        // expression trees.  We save the initial search packet
        // so that the first symbol will be set into the breakpoint
        // list.

        pNameFirst = pName;
        pExState->ambiguous = pName->bn;
        if ((hSearch = MemAllocate (sizeof (search_t))) == 0) {
            pExState->err_num = ERR_NOMEMORY;
            return (HR_error);
        }
        if ((hevalT = MemAllocate (sizeof (eval_t))) == 0) {
            MemFree (hSearch);
            pExState->err_num = ERR_NOMEMORY;
            return (HR_error);
        }
        pSearch = (psearch_t) MemLock (hSearch);
        *pSearch = *pName;
        pSearch->pv = (peval_t)MemLock (hevalT);
        SHGoToParent (&pName->CXTT, &pSearch->CXTT);

        // clear the newly allocated pv

        memset ( pSearch->pv, 0, sizeof (eval_t) );
        retval = SearchSym (pSearch);
        PushStack (pName->pv);
        MemUnLock (hSearch);
        MemFree (hSearch);
        MemUnLock (hevalT);
        MemFree (hevalT);
        if (retval == HR_end) {
            retval = HR_found;
        }
        return (retval);
    }
    else if (pExState->ambiguous != pName->bn) {
        // there has already been a ambiguous symbol that is
        // not at this node in the tree

        pExState->err_num = ERR_BPAMBIGUOUS;
        return (HR_error);
    }
    else {
        if (CheckDupAmb (pName) == FALSE) {
            // the function is not already in the ambiguity list

            if (AmbToList (pName) == FALSE) {
                return (HR_error);
            }
        }

        // reset search to allow more symbols
        pName->possibles = 0;
        return (SearchSym (pName));
    }
}




/**     CheckDupAmb - check for duplicate ambiguity entry
 *
 *      fSuccess = CheckDupAmb (pName)
 *
 *      Entry   pName = pointer to list describing search
 *
 *      Exit    none
 *
 *      Returns TRUE if duplicate symbol found
 *              FALSE if duplicate symbol not found
 */


bool_t
CheckDupAmb (
    psearch_t pName
    )
{
    psearch_t   pN;
    ulong       i;
    bool_t      fdup = FALSE;

    if ((EVAL_TYP (pNameFirst->pv) == EVAL_TYP (pName->pv)) &&
      (memcmp ((void *)&EVAL_SYM (pNameFirst->pv), (void *)&EVAL_SYM (pName->pv),
      sizeof (ADDR)) == 0)) {
        return (TRUE);
    }
    for (i = 1; i < iBPatch; i++) {
        pN = (psearch_t) MemLock (pTMList[i]);
        if (pN->typeOut == EVAL_TYP (pName->pv) &&
          (memcmp ((void *)&pN->addr, (void *)&EVAL_SYM (pName->pv),
          sizeof (ADDR)) == 0)) {
            fdup = TRUE;
        }
        MemUnLock (pTMList[i]);
        if (fdup == TRUE) {
            break;
        }
    }
    return (fdup);
}




/**     SymToNode - store symbol information in evaluation node
 *
 *      fSuccess = SymToNode (hName)
 *
 *      Entry   pName = pointer to search state
 *
 *      Exit    type information and address data stored in value
 *              if the symbol is a typedef record, then the evaluation
 *              state will be set to EV_type.  Otherwise, it will be set
 *              to EV_lvalue.
 *
 *      Returns TRUE if variable was accessible
 *              FALSE if error
 */

bool_t
SymToNode (
    psearch_t pName
    )
{
    CV_typ_t    typ;
    HSYM        hSym;
    peval_t     pv = pName->pv;
    PCXT        pCXT = &pName->CXTT;
    SYMPTR      pSym;
    ulong       cmpThis = 1;
    MEMINFO     mi;

    if ((hSym = pName->hSym) == NULL) {
        // this symbol is found as a class member which means it does
        // not have a symbol.

        DASSERT (pName->typeOut != T_NOTYPE);
        return (SetNodeType (pv, pName->typeOut));
    }
    EVAL_STATE (pv) = EV_lvalue;
    EVAL_MOD (pv) = SHHMODFrompCXT (pCXT);
    CLEAR_EVAL_FLAGS (pv);
    EVAL_SYM_EMI (pv) = pCXT->addr.emi;
    switch((pSym = (SYMPTR) MHOmfLock (hSym))->rectyp) {
        case S_REGISTER:
            EVAL_IS_REG (pv) = TRUE;
            EVAL_REG (pv) = ((REGPTR)pSym)->reg;
            typ = ((REGPTR)pSym)->typind;
            cmpThis = _tcsncmp ( (char *) &((REGPTR)pSym)->name[0],
              (char *) &OpName[0], ((REGPTR)pSym)->name[0] + 1);
            break;

        case S_CONSTANT:
            if (!DebLoadConst (pv, (CONSTPTR)pSym, hSym)) {
                MHOmfUnLock (hSym);
                return (FALSE);
            }
            EVAL_STATE (pv) = EV_constant;
            typ = EVAL_TYP (pv);
            break;


        case S_UDT:
            typ = ((UDTPTR)pSym)->typind;
            EVAL_STATE (pv) = EV_type;
            break;

        case S_BLOCK16:
            EVAL_SYM_SEG (pv) = ((BLOCKPTR16)pSym)->seg;
            EVAL_SYM_OFF (pv) = ((BLOCKPTR16)pSym)->off;
            typ = T_NOTYPE;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            NOTTESTED (FALSE);  // not tested
            break;

        case S_LPROC16:
        case S_GPROC16:
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            if ((typ = ((PROCPTR16)pSym)->typind) == T_NOTYPE) {
                // this is a hack to allow breakpoints on untyped symbols
                typ = T_PFVOID;
                EVAL_SYM_OFF (pv) = ((PROCPTR16)pSym)->off;
                EVAL_SYM_SEG (pv) = ((PROCPTR16)pSym)->seg;
                EVAL_PTR (pv) = EVAL_SYM (pv);
                EVAL_STATE (pv) = EV_rvalue;
            }
            else {
                EVAL_SYM (pv) = *SHpADDRFrompCXT (pCXT);
                EVAL_SYM_SEG (pv) = ((PROCPTR16)pSym)->seg;
                EVAL_SYM_OFF (pv) = ((PROCPTR16)pSym)->off;
                ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            }
            break;

        case S_LABEL16:
            EVAL_IS_LABEL (pv) = TRUE;
            EVAL_SYM_OFF (pv) = ((LABELPTR16)pSym)->off;
            EVAL_SYM_SEG (pv) = ((LABELPTR16)pSym)->seg;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            EVAL_PTR (pv) = EVAL_SYM (pv);
            EVAL_STATE (pv) = EV_rvalue;
            typ = T_PFVOID;
            break;

        case S_BPREL16:
            EVAL_IS_BPREL (pv) = TRUE;
            EVAL_SYM_OFF (pv) = ((BPRELPTR16)pSym)->off;
            EVAL_SYM_SEG (pv) = 0;
            ADDR_IS_LI (EVAL_SYM (pv)) = FALSE;
            typ  = ((BPRELPTR16)pSym)->typind;
            pExState->state.bprel = TRUE;
            cmpThis = _tcsncmp ( (char *) &((BPRELPTR16)pSym)->name[0],
                (char *) &OpName[0], ((BPRELPTR16)pSym)->name[0] + 1);
            break;

        case S_LDATA16:
            pExState->state.fLData = TRUE;
            EVAL_SYM_OFF (pv) = ((DATAPTR16)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR16)pSym)->seg;
            typ = ((DATAPTR16)pSym)->typind;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            break;

        case S_GDATA16:
            pExState->state.fGData = TRUE;
            EVAL_SYM_OFF (pv) = ((DATAPTR16)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR16)pSym)->seg;
            typ = ((DATAPTR16)pSym)->typind;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            break;

        case S_PUB16:
            EVAL_SYM_OFF (pv) = ((DATAPTR16)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR16)pSym)->seg;
            EVAL_STATE (pv) = EV_rvalue;
            if ((typ = ((DATAPTR16)pSym)->typind) == T_NOTYPE) {
                // this is a hack to allow breakpoints on untyped symbold
                typ = T_PFVOID;
            }
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            EVAL_PTR (pv) = EVAL_SYM (pv);
            break;

        case S_BLOCK32:
            EVAL_SYM_SEG (pv) = ((BLOCKPTR32)pSym)->seg;
            EVAL_SYM_OFF (pv) = ((BLOCKPTR32)pSym)->off;
            typ = T_NOTYPE;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            break;

        case S_LPROC32:
        case S_GPROC32:
            ADDRLIN32(EVAL_SYM (pv));
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            if ((typ = ((PROCPTR32)pSym)->typind) == T_NOTYPE) {
                // this is a hack to allow breakpoints on untyped symbols
                typ = T_32PVOID;
                EVAL_SYM_OFF (pv) = ((PROCPTR32)pSym)->off;
                EVAL_SYM_SEG (pv) = ((PROCPTR32)pSym)->seg;
                EVAL_PTR (pv) = EVAL_SYM (pv);
                EVAL_STATE (pv) = EV_rvalue;
            }
            else {
                EVAL_SYM (pv) = *SHpADDRFrompCXT (pCXT);
                EVAL_SYM_SEG (pv) = ((PROCPTR32)pSym)->seg;
                EVAL_SYM_OFF (pv) = ((PROCPTR32)pSym)->off;
            }
            break;

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            ADDRLIN32(EVAL_SYM (pv));
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            if ((typ = ((PROCPTRMIPS)pSym)->typind) == T_NOTYPE) {
                // this is a hack to allow breakpoints on untyped symbols
                typ = T_32PVOID;
                EVAL_SYM_OFF (pv) = ((PROCPTRMIPS)pSym)->off;
                EVAL_SYM_SEG (pv) = ((PROCPTRMIPS)pSym)->seg;
                EVAL_PTR (pv) = EVAL_SYM (pv);
                EVAL_STATE (pv) = EV_rvalue;
            }
            else {
                EVAL_SYM (pv) = *SHpADDRFrompCXT (pCXT);
                EVAL_SYM_SEG (pv) = ((PROCPTRMIPS)pSym)->seg;
                EVAL_SYM_OFF (pv) = ((PROCPTRMIPS)pSym)->off;
            }
            break;

        case S_REGREL32:
            EVAL_IS_REGREL (pv) = TRUE;
            EVAL_SYM_OFF (pv) = ((LPREGREL32)pSym)->off;
            EVAL_SYM_SEG (pv) = 0;
            EVAL_REGREL (pv) = ((LPREGREL32)pSym)->reg;
            typ = ((LPREGREL32)pSym)->typind;
            ADDRLIN32 (EVAL_SYM (pv));
            pExState->state.regrel = TRUE;
            cmpThis = _tcsncmp ( (char *) &((LPREGREL32)pSym)->name[0],
            (char *) &OpName[0], ((LPREGREL32)pSym)->name[0] + 1);
            break;

        case S_LABEL32:
            EVAL_IS_LABEL (pv) = TRUE;
            EVAL_SYM_OFF (pv) = ((LABELPTR32)pSym)->off;
            EVAL_SYM_SEG (pv) = ((LABELPTR32)pSym)->seg;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            EVAL_PTR (pv) = EVAL_SYM (pv);
            EVAL_STATE (pv) = EV_rvalue;
            typ = T_32PVOID;
            break;

        case S_BPREL32:
            EVAL_IS_BPREL (pv) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            EVAL_SYM_OFF (pv) = ((BPRELPTR32)pSym)->off;
            EVAL_SYM_SEG (pv) = 0;
            typ  = ((BPRELPTR32)pSym)->typind;
            ADDR_IS_LI (EVAL_SYM (pv)) = FALSE;
            pExState->state.bprel = TRUE;
            cmpThis = _tcsncmp ( (char *) &((BPRELPTR32)pSym)->name[0],
                (char *) &OpName[0], ((BPRELPTR32)pSym)->name[0] + 1);
            break;

        case S_LDATA32:
        case S_LTHREAD32:
            pExState->state.fLData = TRUE;
            EVAL_SYM_OFF (pv) = ((DATAPTR32)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR32)pSym)->seg;
            typ = ((DATAPTR32)pSym)->typind;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            break;

        case S_GDATA32:
        case S_GTHREAD32:
            pExState->state.fGData = TRUE;
            EVAL_SYM_OFF (pv) = ((DATAPTR32)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR32)pSym)->seg;
            typ = ((DATAPTR32)pSym)->typind;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            break;

        case S_PUB32:
#if 0
            /*
            ** This is a hack so we can set breakpoints on publics.  This
            ** allows EEInfoFromTM() to see that it needs to use the AI
            ** field to get the address to break at.  Otherwise it would
            ** just use the value and not see that it needed to be fixed up
            */
            EVAL_IS_LABEL (pv) = TRUE;
            EVAL_SYM_OFF (pv) = ((DATAPTR32)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR32)pSym)->seg;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            EVAL_PTR (pv) = EVAL_SYM (pv);
            EVAL_STATE (pv) = EV_rvalue;
            typ = T_32PVOID;
            break;
#endif
            EVAL_SYM_OFF (pv) = ((DATAPTR32)pSym)->off;
            EVAL_SYM_SEG (pv) = ((DATAPTR32)pSym)->seg;
            ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
            ADDRLIN32(EVAL_SYM (pv));
            EVAL_PTR (pv) = EVAL_SYM (pv);
            EVAL_STATE (pv) = EV_lvalue;
            if ((typ = ((DATAPTR32)pSym)->typind) == T_NOTYPE) {
#if 0
                if (FNtsdEvalType) {
                    FNtsdEvalType = FALSE;
                    evalstate = EV_rvalue;
                    typ = T_32PULONG;
                } else {
#endif
                    typ = T_UINT4;
#if 0
                    evalstate = EV_lvalue;
                }
#endif
                //
                // this is a hack to allow breakpoints on untyped symbols
                //
                mi.addr = EVAL_SYM(pv);
                SYGetMemInfo(&mi);
                if ((mi.dwState & MEM_COMMIT)
                     && (mi.dwProtect & (PAGE_EXECUTE |
                                     PAGE_EXECUTE_READ |
                                     PAGE_EXECUTE_READWRITE |
                                     PAGE_EXECUTE_WRITECOPY))
                ) {
                    typ = T_32PUCHAR;
                    EVAL_STATE (pv) = EV_rvalue;
                } else {
#if 0
                    //
                    // this gives windbg the "look & feel" of ntsd/kd
                    // if the user does a "dd foo" and there are only
                    // coff publics then the answer will be the same
                    // as ntsd/kd
                    //
                    EVAL_STATE (pv) = evalstate;
#endif
                    EVAL_STATE (pv) = EV_lvalue;
                }
            }
            break;

        case S_REGIA64:
             EVAL_IS_REGIA64 (pv) = TRUE;
             EVAL_REG (pv) = ((REGPTR)pSym)->reg;
             typ = ((REGPTRIA64)pSym)->typind;
             cmpThis = _ftcsncmp ( (char FAR *) &((REGPTRIA64)pSym)->name[0],
               (char FAR *) &OpName[0], ((REGPTRIA64)pSym)->name[0] + 1);
             break;
 
        case S_LPROCIA64:
        case S_GPROCIA64:
             ADDRLIN32(EVAL_SYM (pv));
             ADDR_IS_LI (EVAL_SYM (pv)) = TRUE;
             if ((typ = ((PROCPTRIA64)pSym)->typind) == T_NOTYPE) {
                 // this is a hack to allow breakpoints on untyped symbols
                 typ = T_32PUCHAR;
                 EVAL_SYM_OFF (pv) = ((PROCPTRIA64)pSym)->off;
                 EVAL_SYM_SEG (pv) = ((PROCPTRIA64)pSym)->seg;
                 EVAL_PTR (pv) = EVAL_SYM (pv);
                 EVAL_STATE (pv) = EV_rvalue;
             }
             else {
                 EVAL_SYM (pv) = *SHpADDRFrompCXT (pCXT);
                 EVAL_SYM_SEG (pv) = ((PROCPTRIA64)pSym)->seg;
                 EVAL_SYM_OFF (pv) = ((PROCPTRIA64)pSym)->off;
             }
             break;
 
        case S_REGRELIA64:
             EVAL_IS_REGRELIA64 (pv) = TRUE;
             EVAL_SYM_OFF (pv) = ((LPREGRELIA64)pSym)->off;
             EVAL_SYM_SEG (pv) = 0;
             EVAL_REGREL (pv) = ((LPREGRELIA64)pSym)->reg;
             typ = ((LPREGRELIA64)pSym)->typind;
             ADDRLIN32 (EVAL_SYM (pv));
             pExState->state.regrel = TRUE;
             cmpThis = _ftcsncmp ( (char FAR *) &((LPREGRELIA64)pSym)->name[0],
                 (char FAR *) &OpName[0], ((LPREGRELIA64)pSym)->name[0] + 1);
             break;

        default:
            // these OMF records are no longer supported
            MHOmfUnLock (hSym);
            pExState->err_num = ERR_BADOMF;
            return (FALSE);
    }
    MHOmfUnLock (hSym);
    if (SetNodeType (pv, typ) == FALSE) {
        return (FALSE);
    }
    else if (ClassImp != T_NOTYPE) {
        if (EVAL_IS_PTR (pv) &&
          memcmp (pName->sstr.lpName, &OpName[0] + 1, pName->sstr.cb) == 0) {
            PTR_THISADJUST (pv) = ClassThisAdjust;
        }
        else if (cmpThis == 0) {
            PTR_THISADJUST (pv) = ClassThisAdjust;
        }
    }
    return (TRUE);
}


#define VALIDMPC601 0x01
#define VALIDMPC603 0x02
#define VALIDMPC604 0x04
#define VALIDMPC620 0x08
#define VALIDMPC6xx VALIDMPC601 | VALIDMPC603 | VALIDMPC604 | VALIDMPC620

struct hreg_ppc {
    char    name[8];
    ulong   index;
    ulong   valid;
    CV_typ_t   type;
} const hreg_list_ppc[] = {

    /*
    ** PowerPC General Registers ( User Level )
    */
    { "\004""GPR0",   CV_PPC_GPR0,      VALIDMPC6xx,    T_ULONG },
    { "\002""R0",     CV_PPC_GPR0,      VALIDMPC6xx,    T_ULONG },
    { "\004""GPR1",   CV_PPC_GPR1,      VALIDMPC6xx,    T_ULONG },
    { "\002""R1",     CV_PPC_GPR1,      VALIDMPC6xx,    T_ULONG },
    { "\002""SP",     CV_PPC_GPR1,      VALIDMPC6xx,    T_ULONG }, // GPR1 Alias
    { "\004""GPR2",   CV_PPC_GPR2,      VALIDMPC6xx,    T_ULONG },
    { "\002""R2",     CV_PPC_GPR2,      VALIDMPC6xx,    T_ULONG },
    { "\004""RTOC",   CV_PPC_GPR2,      VALIDMPC6xx,    T_ULONG }, // GPR2 Alias
    { "\004""GPR3",   CV_PPC_GPR3,      VALIDMPC6xx,    T_ULONG },
    { "\002""R3",     CV_PPC_GPR3,      VALIDMPC6xx,    T_ULONG },
    { "\004""GPR4",   CV_PPC_GPR4,      VALIDMPC6xx,    T_ULONG },
    { "\002""R4",     CV_PPC_GPR4,      VALIDMPC6xx,    T_ULONG },
    { "\004""GPR5",   CV_PPC_GPR5,      VALIDMPC6xx,    T_ULONG },
    { "\002""R5",     CV_PPC_GPR5,      VALIDMPC6xx,    T_ULONG },
    { "\004""GPR6",   CV_PPC_GPR6,      VALIDMPC6xx,    T_ULONG },
    { "\002""R6",     CV_PPC_GPR6,      VALIDMPC6xx,    T_ULONG },
    { "\004""GPR7",   CV_PPC_GPR7,      VALIDMPC6xx,    T_ULONG },
    { "\002""R7",     CV_PPC_GPR7,      VALIDMPC6xx,    T_ULONG },
    { "\004""GPR8",   CV_PPC_GPR8,      VALIDMPC6xx,    T_ULONG },
    { "\002""R8",     CV_PPC_GPR8,      VALIDMPC6xx,    T_ULONG },
    { "\004""GPR9",   CV_PPC_GPR9,      VALIDMPC6xx,    T_ULONG },
    { "\002""R9",     CV_PPC_GPR9,      VALIDMPC6xx,    T_ULONG },
    { "\005""GPR10",  CV_PPC_GPR10,     VALIDMPC6xx,    T_ULONG },
    { "\003""R10",    CV_PPC_GPR10,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR11",  CV_PPC_GPR11,     VALIDMPC6xx,    T_ULONG },
    { "\003""R11",    CV_PPC_GPR11,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR12",  CV_PPC_GPR12,     VALIDMPC6xx,    T_ULONG },
    { "\003""R12",    CV_PPC_GPR12,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR13",  CV_PPC_GPR13,     VALIDMPC6xx,    T_ULONG },
    { "\003""R13",    CV_PPC_GPR13,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR14",  CV_PPC_GPR14,     VALIDMPC6xx,    T_ULONG },
    { "\003""R14",    CV_PPC_GPR14,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR15",  CV_PPC_GPR15,     VALIDMPC6xx,    T_ULONG },
    { "\003""R15",    CV_PPC_GPR15,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR16",  CV_PPC_GPR16,     VALIDMPC6xx,    T_ULONG },
    { "\003""R16",    CV_PPC_GPR16,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR17",  CV_PPC_GPR17,     VALIDMPC6xx,    T_ULONG },
    { "\003""R17",    CV_PPC_GPR17,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR18",  CV_PPC_GPR18,     VALIDMPC6xx,    T_ULONG },
    { "\003""R18",    CV_PPC_GPR18,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR19",  CV_PPC_GPR19,     VALIDMPC6xx,    T_ULONG },
    { "\003""R19",    CV_PPC_GPR19,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR20",  CV_PPC_GPR20,     VALIDMPC6xx,    T_ULONG },
    { "\003""R20",    CV_PPC_GPR20,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR21",  CV_PPC_GPR21,     VALIDMPC6xx,    T_ULONG },
    { "\003""R21",    CV_PPC_GPR21,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR22",  CV_PPC_GPR22,     VALIDMPC6xx,    T_ULONG },
    { "\003""R22",    CV_PPC_GPR22,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR23",  CV_PPC_GPR23,     VALIDMPC6xx,    T_ULONG },
    { "\003""R23",    CV_PPC_GPR23,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR24",  CV_PPC_GPR24,     VALIDMPC6xx,    T_ULONG },
    { "\003""R24",    CV_PPC_GPR24,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR25",  CV_PPC_GPR25,     VALIDMPC6xx,    T_ULONG },
    { "\003""R25",    CV_PPC_GPR25,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR26",  CV_PPC_GPR26,     VALIDMPC6xx,    T_ULONG },
    { "\003""R26",    CV_PPC_GPR26,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR27",  CV_PPC_GPR27,     VALIDMPC6xx,    T_ULONG },
    { "\003""R27",    CV_PPC_GPR27,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR28",  CV_PPC_GPR28,     VALIDMPC6xx,    T_ULONG },
    { "\003""R28",    CV_PPC_GPR28,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR29",  CV_PPC_GPR29,     VALIDMPC6xx,    T_ULONG },
    { "\003""R29",    CV_PPC_GPR29,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR30",  CV_PPC_GPR30,     VALIDMPC6xx,    T_ULONG },
    { "\003""R30",    CV_PPC_GPR30,     VALIDMPC6xx,    T_ULONG },
    { "\005""GPR31",  CV_PPC_GPR31,     VALIDMPC6xx,    T_ULONG },
    { "\003""R31",    CV_PPC_GPR31,     VALIDMPC6xx,    T_ULONG },

    /*
    ** PowerPC Condition Register ( User Level )
    */
    { "\002""CR",     CV_PPC_CR,        VALIDMPC6xx,    T_ULONG },
    { "\003""CR0",    CV_PPC_CR0,       VALIDMPC6xx,    T_ULONG },
    { "\003""CR1",    CV_PPC_CR1,       VALIDMPC6xx,    T_ULONG },
    { "\003""CR2",    CV_PPC_CR2,       VALIDMPC6xx,    T_ULONG },
    { "\003""CR3",    CV_PPC_CR3,       VALIDMPC6xx,    T_ULONG },
    { "\003""CR4",    CV_PPC_CR4,       VALIDMPC6xx,    T_ULONG },
    { "\003""CR5",    CV_PPC_CR5,       VALIDMPC6xx,    T_ULONG },
    { "\003""CR6",    CV_PPC_CR6,       VALIDMPC6xx,    T_ULONG },
    { "\003""CR7",    CV_PPC_CR7,       VALIDMPC6xx,    T_ULONG },

    /*
    ** PowerPC Floating Point Registers ( User Level )
    **
    ** The floating point registers aren't really 80 bytes however the
    ** shell expects that they are so we say they are and thusly the em
    ** turns the 64bit doubles into 80bit long doubles before returning them
    */
    { "\004""FPR0",   CV_PPC_FPR0,      VALIDMPC6xx,    T_REAL80 },
    { "\003""FP0",    CV_PPC_FPR0,      VALIDMPC6xx,    T_REAL80 },
    { "\004""FPR1",   CV_PPC_FPR1,      VALIDMPC6xx,    T_REAL80 },
    { "\003""FP1",    CV_PPC_FPR1,      VALIDMPC6xx,    T_REAL80 },
    { "\004""FPR2",   CV_PPC_FPR2,      VALIDMPC6xx,    T_REAL80 },
    { "\003""FP2",    CV_PPC_FPR2,      VALIDMPC6xx,    T_REAL80 },
    { "\004""FPR3",   CV_PPC_FPR3,      VALIDMPC6xx,    T_REAL80 },
    { "\003""FP3",    CV_PPC_FPR3,      VALIDMPC6xx,    T_REAL80 },
    { "\004""FPR4",   CV_PPC_FPR4,      VALIDMPC6xx,    T_REAL80 },
    { "\003""FP4",    CV_PPC_FPR4,      VALIDMPC6xx,    T_REAL80 },
    { "\004""FPR5",   CV_PPC_FPR5,      VALIDMPC6xx,    T_REAL80 },
    { "\003""FP5",    CV_PPC_FPR5,      VALIDMPC6xx,    T_REAL80 },
    { "\004""FPR6",   CV_PPC_FPR6,      VALIDMPC6xx,    T_REAL80 },
    { "\003""FP6",    CV_PPC_FPR6,      VALIDMPC6xx,    T_REAL80 },
    { "\004""FPR7",   CV_PPC_FPR7,      VALIDMPC6xx,    T_REAL80 },
    { "\003""FP7",    CV_PPC_FPR7,      VALIDMPC6xx,    T_REAL80 },
    { "\004""FPR8",   CV_PPC_FPR8,      VALIDMPC6xx,    T_REAL80 },
    { "\003""FP8",    CV_PPC_FPR8,      VALIDMPC6xx,    T_REAL80 },
    { "\004""FPR9",   CV_PPC_FPR9,      VALIDMPC6xx,    T_REAL80 },
    { "\003""FP9",    CV_PPC_FPR9,      VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR10",  CV_PPC_FPR10,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP10",   CV_PPC_FPR10,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR11",  CV_PPC_FPR11,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP11",   CV_PPC_FPR11,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR12",  CV_PPC_FPR12,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP12",   CV_PPC_FPR12,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR13",  CV_PPC_FPR13,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP13",   CV_PPC_FPR13,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR14",  CV_PPC_FPR14,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP14",   CV_PPC_FPR14,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR15",  CV_PPC_FPR15,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP15",   CV_PPC_FPR15,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR16",  CV_PPC_FPR16,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP16",   CV_PPC_FPR16,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR17",  CV_PPC_FPR17,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP17",   CV_PPC_FPR17,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR18",  CV_PPC_FPR18,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP18",   CV_PPC_FPR18,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR19",  CV_PPC_FPR19,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP19",   CV_PPC_FPR19,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR20",  CV_PPC_FPR20,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP20",   CV_PPC_FPR20,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR21",  CV_PPC_FPR21,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP21",   CV_PPC_FPR21,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR22",  CV_PPC_FPR22,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP22",   CV_PPC_FPR22,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR23",  CV_PPC_FPR23,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP23",   CV_PPC_FPR23,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR24",  CV_PPC_FPR24,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP24",   CV_PPC_FPR24,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR25",  CV_PPC_FPR25,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP25",   CV_PPC_FPR25,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR26",  CV_PPC_FPR26,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP26",   CV_PPC_FPR26,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR27",  CV_PPC_FPR27,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP27",   CV_PPC_FPR27,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR28",  CV_PPC_FPR28,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP28",   CV_PPC_FPR28,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR29",  CV_PPC_FPR29,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP29",   CV_PPC_FPR29,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR30",  CV_PPC_FPR30,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP30",   CV_PPC_FPR30,     VALIDMPC6xx,    T_REAL80 },
    { "\005""FPR31",  CV_PPC_FPR31,     VALIDMPC6xx,    T_REAL80 },
    { "\004""FP31",   CV_PPC_FPR31,     VALIDMPC6xx,    T_REAL80 },

    /*
    ** PowerPC Floating Point Status and Control Register ( User Level )
    */
    { "\005""FPSCR",  CV_PPC_FPSCR,     VALIDMPC6xx,    T_ULONG },

    /*
    ** PowerPC Machine State Register ( Supervisor Level )
    */
    { "\003""MSR",    CV_PPC_MSR,       VALIDMPC6xx,    T_ULONG },

    /*
    ** PowerPC Segment Registers ( Supervisor Level )
    */
    { "\003""SR0",    CV_PPC_SR0,       VALIDMPC6xx,    T_ULONG },
    { "\003""SR1",    CV_PPC_SR1,       VALIDMPC6xx,    T_ULONG },
    { "\003""SR2",    CV_PPC_SR2,       VALIDMPC6xx,    T_ULONG },
    { "\003""SR3",    CV_PPC_SR3,       VALIDMPC6xx,    T_ULONG },
    { "\003""SR4",    CV_PPC_SR4,       VALIDMPC6xx,    T_ULONG },
    { "\003""SR5",    CV_PPC_SR5,       VALIDMPC6xx,    T_ULONG },
    { "\003""SR6",    CV_PPC_SR6,       VALIDMPC6xx,    T_ULONG },
    { "\003""SR7",    CV_PPC_SR7,       VALIDMPC6xx,    T_ULONG },
    { "\003""SR8",    CV_PPC_SR8,       VALIDMPC6xx,    T_ULONG },
    { "\003""SR9",    CV_PPC_SR9,       VALIDMPC6xx,    T_ULONG },
    { "\003""SR10",   CV_PPC_SR10,      VALIDMPC6xx,    T_ULONG },
    { "\003""SR11",   CV_PPC_SR11,      VALIDMPC6xx,    T_ULONG },
    { "\003""SR12",   CV_PPC_SR12,      VALIDMPC6xx,    T_ULONG },
    { "\003""SR13",   CV_PPC_SR13,      VALIDMPC6xx,    T_ULONG },
    { "\003""SR14",   CV_PPC_SR14,      VALIDMPC6xx,    T_ULONG },
    { "\003""SR15",   CV_PPC_SR15,      VALIDMPC6xx,    T_ULONG },

    /*
    ** For all of the special purpose registers add 100 to the SPR# that the
    ** Motorola/IBM documentation gives with the exception of any imaginary
    ** registers.
    */

    /*
    ** PowerPC Special Purpose Registers ( User Level )
    */
    { "\002""PC",     CV_PPC_PC,        VALIDMPC6xx,    T_ULONG },    // PC (imaginary register)

    { "\002""MQ",     CV_PPC_MQ,        VALIDMPC601,    T_ULONG },    // MPC601
    { "\003""XER",    CV_PPC_XER,       VALIDMPC6xx,    T_ULONG },
    { "\004""RTCU",   CV_PPC_RTCU,      VALIDMPC601,    T_ULONG },    // MPC601
    { "\004""RTCL",   CV_PPC_RTCL,      VALIDMPC601,    T_ULONG },    // MPC601
    { "\002""LR",     CV_PPC_LR,        VALIDMPC6xx,    T_ULONG },
    { "\003""CTR",    CV_PPC_CTR,       VALIDMPC6xx,    T_ULONG },

    /*
    ** PowerPC Special Purpose Registers ( Supervisor Level )
    */
    { "\005""DSISR",  CV_PPC_DSISR,     VALIDMPC6xx,    T_ULONG },
    { "\003""DAR",    CV_PPC_DAR,       VALIDMPC6xx,    T_ULONG },
    { "\003""DEC",    CV_PPC_DEC,       VALIDMPC6xx,    T_ULONG },
    { "\004""SDR1",   CV_PPC_SDR1,      VALIDMPC6xx,    T_ULONG },
    { "\004""SRR0",   CV_PPC_SRR0,      VALIDMPC6xx,    T_ULONG },
    { "\004""SRR1",   CV_PPC_SRR1,      VALIDMPC6xx,    T_ULONG },
    { "\005""SPRG0",  CV_PPC_SPRG0,     VALIDMPC6xx,    T_ULONG },
    { "\005""SPRG1",  CV_PPC_SPRG1,     VALIDMPC6xx,    T_ULONG },
    { "\005""SPRG2",  CV_PPC_SPRG2,     VALIDMPC6xx,    T_ULONG },
    { "\005""SPRG3",  CV_PPC_SPRG3,     VALIDMPC6xx,    T_ULONG },
    { "\003""ASR",    CV_PPC_ASR,       VALIDMPC6xx,    T_ULONG },    // 64-bit implementations only
    { "\003""EAR",    CV_PPC_EAR,       VALIDMPC6xx,    T_ULONG },
    { "\003""PVR",    CV_PPC_PVR,       VALIDMPC6xx,    T_ULONG },
    { "\005""BAT0U",  CV_PPC_BAT0U,     VALIDMPC6xx,    T_ULONG },
    { "\005""BAT0L",  CV_PPC_BAT0L,     VALIDMPC6xx,    T_ULONG },
    { "\005""BAT1U",  CV_PPC_BAT1U,     VALIDMPC6xx,    T_ULONG },
    { "\005""BAT1L",  CV_PPC_BAT1L,     VALIDMPC6xx,    T_ULONG },
    { "\005""BAT2U",  CV_PPC_BAT2U,     VALIDMPC6xx,    T_ULONG },
    { "\005""BAT2L",  CV_PPC_BAT2L,     VALIDMPC6xx,    T_ULONG },
    { "\005""BAT3U",  CV_PPC_BAT3U,     VALIDMPC6xx,    T_ULONG },
    { "\005""BAT3L",  CV_PPC_BAT3L,     VALIDMPC6xx,    T_ULONG },
    { "\006""DBAT0U", CV_PPC_DBAT0U,    VALIDMPC6xx,    T_ULONG },
    { "\006""DBAT0L", CV_PPC_DBAT0L,    VALIDMPC6xx,    T_ULONG },
    { "\006""DBAT1U", CV_PPC_DBAT1U,    VALIDMPC6xx,    T_ULONG },
    { "\006""DBAT1L", CV_PPC_DBAT1L,    VALIDMPC6xx,    T_ULONG },
    { "\006""DBAT2U", CV_PPC_DBAT2U,    VALIDMPC6xx,    T_ULONG },
    { "\006""DBAT2L", CV_PPC_DBAT2L,    VALIDMPC6xx,    T_ULONG },
    { "\006""DBAT3U", CV_PPC_DBAT3U,    VALIDMPC6xx,    T_ULONG },
    { "\006""DBAT3L", CV_PPC_DBAT3L,    VALIDMPC6xx,    T_ULONG },

    /*
    ** PowerPC Special Purpose Registers Implementation Dependent ( Supervisor Level )
    */

    /*
    ** Doesn't appear that IBM/Motorola has finished defining these.
    */

    { "\004""PMR0",   CV_PPC_PMR0,      VALIDMPC620,    T_ULONG },   // MPC620
    { "\004""PMR1",   CV_PPC_PMR1,      VALIDMPC620,    T_ULONG },   // MPC620
    { "\004""PMR2",   CV_PPC_PMR2,      VALIDMPC620,    T_ULONG },   // MPC620
    { "\004""PMR3",   CV_PPC_PMR3,      VALIDMPC620,    T_ULONG },   // MPC620
    { "\004""PMR4",   CV_PPC_PMR4,      VALIDMPC620,    T_ULONG },   // MPC620
    { "\004""PMR5",   CV_PPC_PMR5,      VALIDMPC620,    T_ULONG },   // MPC620
    { "\004""PMR6",   CV_PPC_PMR6,      VALIDMPC620,    T_ULONG },   // MPC620
    { "\004""PMR7",   CV_PPC_PMR7,      VALIDMPC620,    T_ULONG },   // MPC620
    { "\004""PMR8",   CV_PPC_PMR8,      VALIDMPC620,    T_ULONG },   // MPC620
    { "\004""PMR9",   CV_PPC_PMR9,      VALIDMPC620,    T_ULONG },   // MPC620
    { "\005""PMR10",  CV_PPC_PMR10,     VALIDMPC620,    T_ULONG },   // MPC620
    { "\005""PMR11",  CV_PPC_PMR11,     VALIDMPC620,    T_ULONG },   // MPC620
    { "\005""PMR12",  CV_PPC_PMR12,     VALIDMPC620,    T_ULONG },   // MPC620
    { "\005""PMR13",  CV_PPC_PMR13,     VALIDMPC620,    T_ULONG },   // MPC620
    { "\005""PMR14",  CV_PPC_PMR14,     VALIDMPC620,    T_ULONG },   // MPC620
    { "\005""PMR15",  CV_PPC_PMR15,     VALIDMPC620,    T_ULONG },   // MPC620

    { "\005""DMISS",  CV_PPC_DMISS,     VALIDMPC603,    T_ULONG },   // MPC603
    { "\004""DCMP",   CV_PPC_DCMP,      VALIDMPC603,    T_ULONG },   // MPC603
    { "\005""HASH1",  CV_PPC_HASH1,     VALIDMPC603,    T_ULONG },   // MPC603
    { "\005""HASH2",  CV_PPC_HASH2,     VALIDMPC603,    T_ULONG },   // MPC603
    { "\005""IMISS",  CV_PPC_IMISS,     VALIDMPC603,    T_ULONG },   // MPC603
    { "\004""ICMP",   CV_PPC_ICMP,      VALIDMPC603,    T_ULONG },   // MPC603
    { "\003""RPA",    CV_PPC_RPA,       VALIDMPC603,    T_ULONG },   // MPC603

    { "\004""HID0",   CV_PPC_HID0,      VALIDMPC601 | VALIDMPC603 | VALIDMPC620,    T_ULONG },   // MPC601, MPC603, MPC620
    { "\004""HID1",   CV_PPC_HID1,      VALIDMPC601,    T_ULONG },   // MPC601
    { "\004""HID2",   CV_PPC_HID2,      VALIDMPC601 | VALIDMPC603 | VALIDMPC620,    T_ULONG },   // MPC601, MPC603, MPC620 ( IABR )
    { "\004""HID3",   CV_PPC_HID3,      VALIDMPC6xx,    T_ULONG },   // Not Defined
    { "\004""HID4",   CV_PPC_HID4,      VALIDMPC6xx,    T_ULONG },   // Not Defined
    { "\004""HID5",   CV_PPC_HID5,      VALIDMPC601 | VALIDMPC604 | VALIDMPC620,    T_ULONG },   // MPC601, MPC604, MPC620 ( DABR )
    { "\004""HID6",   CV_PPC_HID6,      VALIDMPC6xx,    T_ULONG },   // Not Defined
    { "\004""HID7",   CV_PPC_HID7,      VALIDMPC6xx,    T_ULONG },   // Not Defined
    { "\004""HID8",   CV_PPC_HID8,      VALIDMPC620,    T_ULONG },   // MPC620 ( BUSCSR )
    { "\004""HID9",   CV_PPC_HID9,      VALIDMPC620,    T_ULONG },   // MPC620 ( L2CSR )
    { "\005""HID10",  CV_PPC_HID10,     VALIDMPC6xx,    T_ULONG },   // Not Defined
    { "\005""HID11",  CV_PPC_HID11,     VALIDMPC6xx,    T_ULONG },   // Not Defined
    { "\005""HID12",  CV_PPC_HID12,     VALIDMPC6xx,    T_ULONG },   // Not Defined
    { "\005""HID13",  CV_PPC_HID13,     VALIDMPC604,    T_ULONG },   // MPC604 ( HCR )
    { "\005""HID14",  CV_PPC_HID14,     VALIDMPC6xx,    T_ULONG },   // Not Defined
    { "\005""HID15",  CV_PPC_HID15,     VALIDMPC601 | VALIDMPC604 | VALIDMPC620,    T_ULONG }    // MPC601, MPC604, MPC620 ( PIR )

};


#define VALID68851  0x40
#define VALID68882  0x80

#define VALID68000  0x01
#define VALID68010  0x02 | VALID68000
#define VALID68020  0x04 | VALID68010
#define VALID68030  0x08 | VALID68020 | VALID68851
#define VALID68040  0x10 | VALID68030

struct hreg_68k {
    char    name[7];
    ulong   index;
    ulong   valid;
    CV_typ_t   type;
} const hreg_list_68k[] = {
    {"\x002""D0",       CV_R68_D0,      VALID68000, T_ULONG},
    {"\x002""D1",       CV_R68_D1,      VALID68000, T_ULONG},
    {"\x002""D2",       CV_R68_D2,      VALID68000, T_ULONG},
    {"\x002""D3",       CV_R68_D3,      VALID68000, T_ULONG},
    {"\x002""D4",       CV_R68_D4,      VALID68000, T_ULONG},
    {"\x002""D5",       CV_R68_D5,      VALID68000, T_ULONG},
    {"\x002""D6",       CV_R68_D6,      VALID68000, T_ULONG},
    {"\x002""D7",       CV_R68_D7,      VALID68000, T_ULONG},
    {"\x002""A0",       CV_R68_A0,      VALID68000, T_ULONG},
    {"\x002""A1",       CV_R68_A1,      VALID68000, T_ULONG},
    {"\x002""A2",       CV_R68_A2,      VALID68000, T_ULONG},
    {"\x002""A3",       CV_R68_A3,      VALID68000, T_ULONG},
    {"\x002""A4",       CV_R68_A4,      VALID68000, T_ULONG},
    {"\x002""A5",       CV_R68_A5,      VALID68000, T_ULONG},
    {"\x002""A6",       CV_R68_A6,      VALID68000, T_ULONG},
    {"\x002""A7",       CV_R68_A7,      VALID68000, T_ULONG},
    {"\x003""CCR",      CV_R68_CCR,     VALID68000, T_UCHAR},
    {"\x002""SR",       CV_R68_SR,      VALID68000, T_USHORT},
#if 0
    {"\x003""USP",      CV_R68_USP,     VALID68000, T_ULONG},
    {"\x003""MSP",      CV_R68_MSP,     VALID68000, T_ULONG},
#endif
    {"\x002""PC",       CV_R68_PC,      VALID68000, T_ULONG},

#if 0
    {"\x003""SFC",      CV_R68_SFC,     VALID68010, T_USHORT},
    {"\x003""DFC",      CV_R68_DFC,     VALID68010, T_USHORT},
    {"\x003""VBR",      CV_R68_VBR,     VALID68010, T_USHORT},

    {"\x004""CACR",     CV_R68_CACR,    VALID68020, T_ULONG},
    {"\x004""CAAR",     CV_R68_CAAR,    VALID68020, T_ULONG},
    {"\x003""ISP",      CV_R68_ISP,     VALID68020, T_ULONG},

    {"\x003""PSR",      CV_R68_PSR,     VALID68851, T_USHORT},
    {"\x004""PCSR",     CV_R68_PCSR,    VALID68851, T_USHORT},
    {"\x003""VAL",      CV_R68_VAL,     VALID68851, T_USHORT},
    {"\x003""CRP",      CV_R68_CRP,     VALID68851, T_UQUAD},
    {"\x003""SRP",      CV_R68_SRP,     VALID68851, T_UQUAD},
    {"\x003""DRP",      CV_R68_DRP,     VALID68851, T_UQUAD},
    {"\x002""TC",       CV_R68_TC,      VALID68851, T_ULONG},
    {"\x002""AC",       CV_R68_AC,      VALID68851, T_USHORT},
    {"\x003""SCC",      CV_R68_SCC,     VALID68851, T_USHORT},
    {"\x003""CAL",      CV_R68_CAL,     VALID68851, T_USHORT},
    {"\x004""BAD0",     CV_R68_BAD0,    VALID68851, T_USHORT},
    {"\x004""BAD1",     CV_R68_BAD1,    VALID68851, T_USHORT},
    {"\x004""BAD2",     CV_R68_BAD2,    VALID68851, T_USHORT},
    {"\x004""BAD3",     CV_R68_BAD3,    VALID68851, T_USHORT},
    {"\x004""BAD4",     CV_R68_BAD4,    VALID68851, T_USHORT},
    {"\x004""BAD5",     CV_R68_BAD5,    VALID68851, T_USHORT},
    {"\x004""BAD6",     CV_R68_BAD6,    VALID68851, T_USHORT},
    {"\x004""BAD7",     CV_R68_BAD7,    VALID68851, T_USHORT},
    {"\x004""BAC0",     CV_R68_BAC0,    VALID68851, T_USHORT},
    {"\x004""BAC1",     CV_R68_BAC1,    VALID68851, T_USHORT},
    {"\x004""BAC2",     CV_R68_BAC2,    VALID68851, T_USHORT},
    {"\x004""BAC3",     CV_R68_BAC3,    VALID68851, T_USHORT},
    {"\x004""BAC4",     CV_R68_BAC4,    VALID68851, T_USHORT},
    {"\x004""BAC5",     CV_R68_BAC5,    VALID68851, T_USHORT},
    {"\x004""BAC6",     CV_R68_BAC6,    VALID68851, T_USHORT},
    {"\x004""BAC7",     CV_R68_BAC7,    VALID68851, T_USHORT},

    {"\x003""TT0",      CV_R68_TT0,     VALID68030, T_ULONG},
    {"\x003""TT1",      CV_R68_TT1,     VALID68030, T_ULONG},
#endif

    {"\x004""FPCR",     CV_R68_FPCR,    VALID68882, T_ULONG},
    {"\x004""FPSR",     CV_R68_FPSR,    VALID68882, T_ULONG},
    {"\x005""FPIAR",    CV_R68_FPIAR,   VALID68882, T_ULONG},
    {"\x003""FP0",      CV_R68_FP0,     VALID68882, T_REAL80},
    {"\x003""FP1",      CV_R68_FP1,     VALID68882, T_REAL80},
    {"\x003""FP2",      CV_R68_FP2,     VALID68882, T_REAL80},
    {"\x003""FP3",      CV_R68_FP3,     VALID68882, T_REAL80},
    {"\x003""FP4",      CV_R68_FP4,     VALID68882, T_REAL80},
    {"\x003""FP5",      CV_R68_FP5,     VALID68882, T_REAL80},
    {"\x003""FP6",      CV_R68_FP6,     VALID68882, T_REAL80},
    {"\x003""FP7",      CV_R68_FP7,     VALID68882, T_REAL80},
};


#define VALIDx86    0x01
#define VALID386    0x02
#define REGHI       0x04
#define REGLO       0x08
#define VALIDx87    0x10

struct hreg_x86 {
    char    name[5];
    int     index;
    ulong   valid;
    CV_typ_t   type;
} const hreg_list_x86[] = {
    {"\x002""AX",   CV_REG_AX,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""BX",   CV_REG_BX,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""CX",   CV_REG_CX,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""DX",   CV_REG_DX,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""SP",   CV_REG_SP,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""BP",   CV_REG_BP,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""SI",   CV_REG_SI,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""DI",   CV_REG_DI,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""DS",   CV_REG_DS,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""ES",   CV_REG_ES,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""SS",   CV_REG_SS,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""CS",   CV_REG_CS,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""IP",   CV_REG_IP,      VALIDx86 | VALID386,            T_USHORT},
    {"\x002""FL",   CV_REG_FLAGS,   VALIDx86 | VALID386,            T_USHORT},
    {"\x003""EFL",  CV_REG_EFLAGS,  VALID386,                       T_ULONG},
    {"\x003""EAX",  CV_REG_EAX,     VALID386,                       T_ULONG},
    {"\x003""EBX",  CV_REG_EBX,     VALID386,                       T_ULONG},
    {"\x003""ECX",  CV_REG_ECX,     VALID386,                       T_ULONG},
    {"\x003""EDX",  CV_REG_EDX,     VALID386,                       T_ULONG},
    {"\x003""ESP",  CV_REG_ESP,     VALID386,                       T_ULONG},
    {"\x003""EBP",  CV_REG_EBP,     VALID386,                       T_ULONG},
    {"\x003""ESI",  CV_REG_ESI,     VALID386,                       T_ULONG},
    {"\x003""EDI",  CV_REG_EDI,     VALID386,                       T_ULONG},
    {"\x003""EIP",  CV_REG_EIP,     VALID386,                       T_ULONG},
    {"\x002""PQ",   CV_REG_QUOTE,   VALIDx86,                       T_USHORT},
    {"\x002""TL",   CV_REG_TEMP,    VALIDx86,                       T_USHORT},
    {"\x002""TH",   CV_REG_TEMPH,   VALIDx86,                       T_USHORT},
    {"\x002""FS",   CV_REG_FS,      VALID386,                       T_USHORT},
    {"\x002""GS",   CV_REG_GS,      VALID386,                       T_USHORT},
    {"\x002""AH",   CV_REG_AH,      VALIDx86 | VALID386 | REGHI,    T_UCHAR},
    {"\x002""BH",   CV_REG_BH,      VALIDx86 | VALID386 | REGHI,    T_UCHAR},
    {"\x002""CH",   CV_REG_CH,      VALIDx86 | VALID386 | REGHI,    T_UCHAR},
    {"\x002""DH",   CV_REG_DH,      VALIDx86 | VALID386 | REGHI,    T_UCHAR},
    {"\x002""AL",   CV_REG_AL,      VALIDx86 | VALID386 | REGLO,    T_UCHAR},
    {"\x002""BL",   CV_REG_BL,      VALIDx86 | VALID386 | REGLO,    T_UCHAR},
    {"\x002""CL",   CV_REG_CL,      VALIDx86 | VALID386 | REGLO,    T_UCHAR},
    {"\x002""DL",   CV_REG_DL,      VALIDx86 | VALID386 | REGLO,    T_UCHAR},
    {"\x002""st",   CV_REG_ST0,     VALIDx86 | VALID386 | VALIDx87, T_REAL80},
    {"\x003""st0",  CV_REG_ST0,     VALIDx86 | VALID386 | VALIDx87, T_REAL80},
    {"\x003""st1",  CV_REG_ST1,     VALIDx86 | VALID386 | VALIDx87, T_REAL80},
    {"\x003""st2",  CV_REG_ST2,     VALIDx86 | VALID386 | VALIDx87, T_REAL80},
    {"\x003""st3",  CV_REG_ST3,     VALIDx86 | VALID386 | VALIDx87, T_REAL80},
    {"\x003""st4",  CV_REG_ST4,     VALIDx86 | VALID386 | VALIDx87, T_REAL80},
    {"\x003""st5",  CV_REG_ST5,     VALIDx86 | VALID386 | VALIDx87, T_REAL80},
    {"\x003""st6",  CV_REG_ST6,     VALIDx86 | VALID386 | VALIDx87, T_REAL80},
    {"\x003""st7",  CV_REG_ST7,     VALIDx86 | VALID386 | VALIDx87, T_REAL80},
    {"\x003""MM0",  CV_REG_MM0,     VALIDx86                      , T_UQUAD},
    {"\x003""MM1",  CV_REG_MM1,     VALIDx86                      , T_UQUAD},
    {"\x003""MM2",  CV_REG_MM2,     VALIDx86                      , T_UQUAD},
    {"\x003""MM3",  CV_REG_MM3,     VALIDx86                      , T_UQUAD},
    {"\x003""MM4",  CV_REG_MM4,     VALIDx86                      , T_UQUAD},
    {"\x003""MM5",  CV_REG_MM5,     VALIDx86                      , T_UQUAD},
    {"\x003""MM6",  CV_REG_MM6,     VALIDx86                      , T_UQUAD},
    {"\x003""MM7",  CV_REG_MM7,     VALIDx86                      , T_UQUAD},
//    {"\x003""TEB",  CV_REG_TEB,     VALIDx86,                       T_ULONG},
//    {"\x003""ERR",  CV_REG_ERR,     VALIDx86,                       T_ULONG},
};

struct hreg_mip {
    char    name[5];
    int     index;
    CV_typ_t   type;
} const hreg_list_mip[] = {
    {"\004""ZERO",      CV_M4_IntZERO,     T_ULONG},
    {"\002""AT",        CV_M4_IntAT,       T_ULONG},
    {"\002""V0",        CV_M4_IntV0,       T_ULONG},
    {"\002""V1",        CV_M4_IntV1,       T_ULONG},
    {"\002""A0",        CV_M4_IntA0,       T_ULONG},
    {"\002""A1",        CV_M4_IntA1,       T_ULONG},
    {"\002""A2",        CV_M4_IntA2,       T_ULONG},
    {"\002""A3",        CV_M4_IntA3,       T_ULONG},
    {"\002""T0",        CV_M4_IntT0,       T_ULONG},
    {"\002""T1",        CV_M4_IntT1,       T_ULONG},
    {"\002""T2",        CV_M4_IntT2,       T_ULONG},
    {"\002""T3",        CV_M4_IntT3,       T_ULONG},
    {"\002""T4",        CV_M4_IntT4,       T_ULONG},
    {"\002""T5",        CV_M4_IntT5,       T_ULONG},
    {"\002""T6",        CV_M4_IntT6,       T_ULONG},
    {"\002""T7",        CV_M4_IntT7,       T_ULONG},
    {"\002""T8",        CV_M4_IntT8,       T_ULONG},
    {"\002""T9",        CV_M4_IntT9,       T_ULONG},
    {"\002""S0",        CV_M4_IntS0,       T_ULONG},
    {"\002""S1",        CV_M4_IntS1,       T_ULONG},
    {"\002""S2",        CV_M4_IntS2,       T_ULONG},
    {"\002""S3",        CV_M4_IntS3,       T_ULONG},
    {"\002""S4",        CV_M4_IntS4,       T_ULONG},
    {"\002""S5",        CV_M4_IntS5,       T_ULONG},
    {"\002""S6",        CV_M4_IntS6,       T_ULONG},
    {"\002""S7",        CV_M4_IntS7,       T_ULONG},
    {"\002""S8",        CV_M4_IntS8,       T_ULONG},
    {"\002""K0",        CV_M4_IntKT0,      T_ULONG},
    {"\002""K1",        CV_M4_IntKT1,      T_ULONG},
    {"\002""GP",        CV_M4_IntGP,       T_ULONG},
    {"\002""SP",        CV_M4_IntSP,       T_ULONG},
    {"\002""RA",        CV_M4_IntRA,       T_ULONG},
    {"\003""FIR",       CV_M4_Fir,         T_ULONG},
    {"\002""PC",        CV_M4_Fir,         T_ULONG},
    {"\003""PSR",       CV_M4_Psr,         T_ULONG},
    {"\003""FSR",       CV_M4_FltFsr,      T_ULONG},
    {"\002""HI",        CV_M4_IntHI,       T_ULONG},
    {"\002""LO",        CV_M4_IntLO,       T_ULONG},
    {"\003""FR0",       CV_M4_FltF0,       T_REAL32},
    {"\003""FR1",       CV_M4_FltF1,       T_REAL32},
    {"\003""FR2",       CV_M4_FltF2,       T_REAL32},
    {"\003""FR3",       CV_M4_FltF3,       T_REAL32},
    {"\003""FR4",       CV_M4_FltF4,       T_REAL32},
    {"\003""FR5",       CV_M4_FltF5,       T_REAL32},
    {"\003""FR6",       CV_M4_FltF6,       T_REAL32},
    {"\003""FR7",       CV_M4_FltF7,       T_REAL32},
    {"\003""FR8",       CV_M4_FltF8,       T_REAL32},
    {"\003""FR9",       CV_M4_FltF9,       T_REAL32},
    {"\004""FR10",      CV_M4_FltF10,      T_REAL32},
    {"\004""FR11",      CV_M4_FltF11,      T_REAL32},
    {"\004""FR12",      CV_M4_FltF12,      T_REAL32},
    {"\004""FR13",      CV_M4_FltF13,      T_REAL32},
    {"\004""FR14",      CV_M4_FltF14,      T_REAL32},
    {"\004""FR15",      CV_M4_FltF15,      T_REAL32},
    {"\004""FR16",      CV_M4_FltF16,      T_REAL32},
    {"\004""FR17",      CV_M4_FltF17,      T_REAL32},
    {"\004""FR18",      CV_M4_FltF18,      T_REAL32},
    {"\004""FR19",      CV_M4_FltF19,      T_REAL32},
    {"\004""FR20",      CV_M4_FltF20,      T_REAL32},
    {"\004""FR21",      CV_M4_FltF21,      T_REAL32},
    {"\004""FR22",      CV_M4_FltF22,      T_REAL32},
    {"\004""FR23",      CV_M4_FltF23,      T_REAL32},
    {"\004""FR24",      CV_M4_FltF24,      T_REAL32},
    {"\004""FR25",      CV_M4_FltF25,      T_REAL32},
    {"\004""FR26",      CV_M4_FltF26,      T_REAL32},
    {"\004""FR27",      CV_M4_FltF27,      T_REAL32},
    {"\004""FR28",      CV_M4_FltF28,      T_REAL32},
    {"\004""FR29",      CV_M4_FltF29,      T_REAL32},
    {"\004""FR30",      CV_M4_FltF30,      T_REAL32},
    {"\004""FR31",      CV_M4_FltF31,      T_REAL32},
    {"\003""FP0",       CV_M4_FltF1 << 8 |  CV_M4_FltF0,        T_REAL64},
    {"\003""FP2",       CV_M4_FltF3 << 8 |  CV_M4_FltF2,        T_REAL64},
    {"\003""FP4",       CV_M4_FltF5 << 8 |  CV_M4_FltF4,        T_REAL64},
    {"\003""FP6",       CV_M4_FltF7 << 8 |  CV_M4_FltF6,        T_REAL64},
    {"\003""FP8",       CV_M4_FltF9 << 8 |  CV_M4_FltF8,        T_REAL64},
    {"\004""FP10",      CV_M4_FltF11 << 8 |  CV_M4_FltF10,      T_REAL64},
    {"\004""FP12",      CV_M4_FltF13 << 8 |  CV_M4_FltF12,      T_REAL64},
    {"\004""FP14",      CV_M4_FltF15 << 8 |  CV_M4_FltF14,      T_REAL64},
    {"\004""FP16",      CV_M4_FltF17 << 8 |  CV_M4_FltF16,      T_REAL64},
    {"\004""FP18",      CV_M4_FltF19 << 8 |  CV_M4_FltF18,      T_REAL64},
    {"\004""FP20",      CV_M4_FltF21 << 8 |  CV_M4_FltF20,      T_REAL64},
    {"\004""FP22",      CV_M4_FltF23 << 8 |  CV_M4_FltF22,      T_REAL64},
    {"\004""FP24",      CV_M4_FltF25 << 8 |  CV_M4_FltF24,      T_REAL64},
    {"\004""FP26",      CV_M4_FltF27 << 8 |  CV_M4_FltF26,      T_REAL64},
    {"\004""FP28",      CV_M4_FltF29 << 8 |  CV_M4_FltF28,      T_REAL64},
    {"\004""FP30",      CV_M4_FltF31 << 8 |  CV_M4_FltF30,      T_REAL64},
};
struct hreg_axp {
    char    name[5];
    int     index;
    CV_typ_t   type;
} hreg_list_axp[] = {
    {"\004""ZERO",      CV_ALPHA_IntZERO,     T_UQUAD},
    {"\002""AT",        CV_ALPHA_IntAT,       T_UQUAD},
    {"\002""V0",        CV_ALPHA_IntV0,       T_UQUAD},
    {"\002""A0",        CV_ALPHA_IntA0,       T_UQUAD},
    {"\002""A1",        CV_ALPHA_IntA1,       T_UQUAD},
    {"\002""A2",        CV_ALPHA_IntA2,       T_UQUAD},
    {"\002""A3",        CV_ALPHA_IntA3,       T_UQUAD},
    {"\002""A4",        CV_ALPHA_IntA4,       T_UQUAD},
    {"\002""A5",        CV_ALPHA_IntA5,       T_UQUAD},
    {"\002""T0",        CV_ALPHA_IntT0,       T_UQUAD},
    {"\002""T1",        CV_ALPHA_IntT1,       T_UQUAD},
    {"\002""T2",        CV_ALPHA_IntT2,       T_UQUAD},
    {"\002""T3",        CV_ALPHA_IntT3,       T_UQUAD},
    {"\002""T4",        CV_ALPHA_IntT4,       T_UQUAD},
    {"\002""T5",        CV_ALPHA_IntT5,       T_UQUAD},
    {"\002""T6",        CV_ALPHA_IntT6,       T_UQUAD},
    {"\002""T7",        CV_ALPHA_IntT7,       T_UQUAD},
    {"\002""T8",        CV_ALPHA_IntT8,       T_UQUAD},
    {"\002""T9",        CV_ALPHA_IntT9,       T_UQUAD},
    {"\003""T10",       CV_ALPHA_IntT10,      T_UQUAD},
    {"\003""T11",       CV_ALPHA_IntT11,      T_UQUAD},
    {"\003""T12",       CV_ALPHA_IntT12,      T_UQUAD},
    {"\002""S0",        CV_ALPHA_IntS0,       T_UQUAD},
    {"\002""S1",        CV_ALPHA_IntS1,       T_UQUAD},
    {"\002""S2",        CV_ALPHA_IntS2,       T_UQUAD},
    {"\002""S3",        CV_ALPHA_IntS3,       T_UQUAD},
    {"\002""S4",        CV_ALPHA_IntS4,       T_UQUAD},
    {"\002""S5",        CV_ALPHA_IntS5,       T_UQUAD},
    {"\002""GP",        CV_ALPHA_IntGP,       T_UQUAD},
    {"\002""SP",        CV_ALPHA_IntSP,       T_UQUAD},
    {"\002""FP",        CV_ALPHA_IntFP,       T_UQUAD},
    {"\004""FPCR",      CV_ALPHA_Fpcr,        T_UQUAD},
    {"\002""RA",        CV_ALPHA_IntRA,       T_UQUAD},
    {"\003""FIR",       CV_ALPHA_Fir,         T_UQUAD},
    {"\003""PC",        CV_ALPHA_Fir,         T_UQUAD},
    {"\003""PSR",       CV_ALPHA_Psr,         T_UQUAD},
    {"\003""FSR",       CV_ALPHA_FltFsr,      T_UQUAD},
    {"\002""F0",        CV_ALPHA_FltF0,       T_REAL64},
    {"\002""F1",        CV_ALPHA_FltF1,       T_REAL64},
    {"\002""F2",        CV_ALPHA_FltF2,       T_REAL64},
    {"\002""F3",        CV_ALPHA_FltF3,       T_REAL64},
    {"\002""F4",        CV_ALPHA_FltF4,       T_REAL64},
    {"\002""F5",        CV_ALPHA_FltF5,       T_REAL64},
    {"\002""F6",        CV_ALPHA_FltF6,       T_REAL64},
    {"\002""F7",        CV_ALPHA_FltF7,       T_REAL64},
    {"\002""F8",        CV_ALPHA_FltF8,       T_REAL64},
    {"\002""F9",        CV_ALPHA_FltF9,       T_REAL64},
    {"\003""F10",       CV_ALPHA_FltF10,      T_REAL64},
    {"\003""F11",       CV_ALPHA_FltF11,      T_REAL64},
    {"\003""F12",       CV_ALPHA_FltF12,      T_REAL64},
    {"\003""F13",       CV_ALPHA_FltF13,      T_REAL64},
    {"\003""F14",       CV_ALPHA_FltF14,      T_REAL64},
    {"\003""F15",       CV_ALPHA_FltF15,      T_REAL64},
    {"\003""F16",       CV_ALPHA_FltF16,      T_REAL64},
    {"\003""F17",       CV_ALPHA_FltF17,      T_REAL64},
    {"\003""F18",       CV_ALPHA_FltF18,      T_REAL64},
    {"\003""F19",       CV_ALPHA_FltF19,      T_REAL64},
    {"\003""F20",       CV_ALPHA_FltF20,      T_REAL64},
    {"\003""F21",       CV_ALPHA_FltF21,      T_REAL64},
    {"\003""F22",       CV_ALPHA_FltF22,      T_REAL64},
    {"\003""F23",       CV_ALPHA_FltF23,      T_REAL64},
    {"\003""F24",       CV_ALPHA_FltF24,      T_REAL64},
    {"\003""F25",       CV_ALPHA_FltF25,      T_REAL64},
    {"\003""F26",       CV_ALPHA_FltF26,      T_REAL64},
    {"\003""F27",       CV_ALPHA_FltF27,      T_REAL64},
    {"\003""F28",       CV_ALPHA_FltF28,      T_REAL64},
    {"\003""F29",       CV_ALPHA_FltF29,      T_REAL64},
    {"\003""F30",       CV_ALPHA_FltF30,      T_REAL64},
    {"\003""F31",       CV_ALPHA_FltF31,      T_REAL64},
};

struct hreg_ia64 {
    char    name[9];  //v-vadimp we have "BSPSTORE"
    int     index;
    CV_typ_t   type;
} hreg_list_ia64[] = { 
// First, the pseudo registers

    {"\x002""$p",   CV_REG_PSEUDO1, T_ULONG},
    {"\x003""$p1",  CV_REG_PSEUDO1, T_ULONG},
    {"\x003""$p2",  CV_REG_PSEUDO2, T_ULONG},
    {"\x003""$p3",  CV_REG_PSEUDO3, T_ULONG},
    {"\x003""$p4",  CV_REG_PSEUDO4, T_ULONG},
    {"\x002""$u",   CV_REG_PSEUDO5, T_ULONG},
    {"\x003""$u1",  CV_REG_PSEUDO5, T_ULONG},
    {"\x003""$u2",  CV_REG_PSEUDO6, T_ULONG},
    {"\x003""$u3",  CV_REG_PSEUDO7, T_ULONG},
    {"\x003""$u4",  CV_REG_PSEUDO8, T_ULONG},
    {"\x004""$exp", CV_REG_PSEUDO9, T_ULONG},

// Then the IA64 registers.

    {"\004""BRRP",  CV_IA64_BrRp,   T_UQUAD },
    {"\004""BRS0",  CV_IA64_BrS0,   T_UQUAD },
    {"\004""BRS1",  CV_IA64_BrS1,   T_UQUAD },
    {"\004""BRS2",  CV_IA64_BrS2,   T_UQUAD },
    {"\004""BRS3",  CV_IA64_BrS3,   T_UQUAD },
    {"\004""BRS4",  CV_IA64_BrS4,   T_UQUAD },
    {"\004""BRT0",  CV_IA64_BrT0,   T_UQUAD },
    {"\004""BRT1",  CV_IA64_BrT1,   T_UQUAD },


    {"\005""PREDS",   CV_IA64_Preds,     T_UQUAD},
//    {"\002""P0",   CV_IA64_P0,     T_BIT },
//    {"\002""P1",   CV_IA64_P1,     T_BIT },
//    {"\002""P2",   CV_IA64_P2,     T_BIT },
//    {"\002""P3",   CV_IA64_P3,     T_BIT },
//    {"\002""P4",   CV_IA64_P4,     T_BIT },
//    {"\002""P5",   CV_IA64_P5,     T_BIT },
//    {"\002""P6",   CV_IA64_P6,     T_BIT },
//    {"\002""P7",   CV_IA64_P7,     T_BIT },
//    {"\002""P8",   CV_IA64_P8,     T_BIT },
//    {"\002""P9",   CV_IA64_P9,     T_BIT },
//    {"\003""P10",  CV_IA64_P10,    T_BIT },
//    {"\003""P11",  CV_IA64_P11,    T_BIT },
//    {"\003""P12",  CV_IA64_P12,    T_BIT },
//    {"\003""P13",  CV_IA64_P13,    T_BIT },
//    {"\003""P14",  CV_IA64_P14,    T_BIT },
//    {"\003""P15",  CV_IA64_P15,    T_BIT },
//    {"\003""P16",  CV_IA64_P16,    T_BIT },
//    {"\003""P17",  CV_IA64_P17,    T_BIT },
//    {"\003""P18",  CV_IA64_P18,    T_BIT },
//    {"\003""P19",  CV_IA64_P19,    T_BIT },
//    {"\003""P20",  CV_IA64_P20,    T_BIT },
//    {"\003""P21",  CV_IA64_P21,    T_BIT },
//    {"\003""P22",  CV_IA64_P22,    T_BIT },
//    {"\003""P23",  CV_IA64_P23,    T_BIT },
//    {"\003""P24",  CV_IA64_P24,    T_BIT },
//    {"\003""P25",  CV_IA64_P25,    T_BIT },
//    {"\003""P26",  CV_IA64_P26,    T_BIT },
//    {"\003""P27",  CV_IA64_P27,    T_BIT },
//    {"\003""P28",  CV_IA64_P28,    T_BIT },
//    {"\003""P29",  CV_IA64_P29,    T_BIT },
//    {"\003""P30",  CV_IA64_P30,    T_BIT },
//    {"\003""P31",  CV_IA64_P31,    T_BIT },
//    {"\003""P32",  CV_IA64_P32,    T_BIT },
//    {"\003""P33",  CV_IA64_P33,    T_BIT },
//    {"\003""P34",  CV_IA64_P34,    T_BIT },
//    {"\003""P35",  CV_IA64_P35,    T_BIT },
//    {"\003""P36",  CV_IA64_P36,    T_BIT },
//    {"\003""P37",  CV_IA64_P37,    T_BIT },
//    {"\003""P38",  CV_IA64_P38,    T_BIT },
//    {"\003""P39",  CV_IA64_P39,    T_BIT },
//    {"\003""P40",  CV_IA64_P40,    T_BIT },
//    {"\003""P41",  CV_IA64_P41,    T_BIT },
//    {"\003""P42",  CV_IA64_P42,    T_BIT },
//    {"\003""P43",  CV_IA64_P43,    T_BIT },
//    {"\003""P44",  CV_IA64_P44,    T_BIT },
//    {"\003""P45",  CV_IA64_P45,    T_BIT },
//    {"\003""P46",  CV_IA64_P46,    T_BIT },
//    {"\003""P47",  CV_IA64_P47,    T_BIT },
//    {"\003""P48",  CV_IA64_P48,    T_BIT },
//    {"\003""P49",  CV_IA64_P49,    T_BIT },
//    {"\003""P50",  CV_IA64_P50,    T_BIT },
//    {"\003""P51",  CV_IA64_P51,    T_BIT },
//    {"\003""P52",  CV_IA64_P52,    T_BIT },
//    {"\003""P53",  CV_IA64_P53,    T_BIT },
//    {"\003""P54",  CV_IA64_P54,    T_BIT },
//    {"\003""P55",  CV_IA64_P55,    T_BIT },
//    {"\003""P56",  CV_IA64_P56,    T_BIT },
//    {"\003""P57",  CV_IA64_P57,    T_BIT },
//    {"\003""P58",  CV_IA64_P58,    T_BIT },
//    {"\003""P59",  CV_IA64_P59,    T_BIT },
//    {"\003""P60",  CV_IA64_P60,    T_BIT },
//    {"\003""P61",  CV_IA64_P61,    T_BIT },
//    {"\003""P62",  CV_IA64_P62,    T_BIT },
//    {"\003""P63",  CV_IA64_P63,    T_BIT },

    {"\002""IP",   CV_IA64_Ip,      T_UQUAD },
    {"\004""NATS", CV_IA64_IntNats, T_UQUAD },
    {"\003""CFM",  CV_IA64_Cfm,     T_UQUAD },
    {"\003""PSR",  CV_IA64_Psr,     T_UQUAD },

    {"\004""ZERO", CV_IA64_IntZero, T_UQUAD },
    {"\002""GP",   CV_IA64_IntGp,   T_UQUAD },
    {"\002""T0",   CV_IA64_IntT0,   T_UQUAD },
    {"\002""T1",   CV_IA64_IntT1,   T_UQUAD },
    {"\002""S0",   CV_IA64_IntS0,   T_UQUAD },
    {"\002""S1",   CV_IA64_IntS1,   T_UQUAD },
    {"\002""S2",   CV_IA64_IntS2,   T_UQUAD },
    {"\002""S3",   CV_IA64_IntS3,   T_UQUAD },
    {"\002""V0",   CV_IA64_IntV0,   T_UQUAD },
    {"\002""Teb",  CV_IA64_IntTeb,  T_UQUAD },
    {"\002""T2",   CV_IA64_IntT2,   T_UQUAD },
    {"\002""T3",   CV_IA64_IntT3,   T_UQUAD },
    {"\002""SP",   CV_IA64_IntSp,   T_UQUAD },
    {"\002""T4",   CV_IA64_IntT4,   T_UQUAD },
    {"\002""T5",   CV_IA64_IntT5,   T_UQUAD },
    {"\002""T6",   CV_IA64_IntT6,   T_UQUAD },
    {"\002""T7",   CV_IA64_IntT7,   T_UQUAD },
    {"\002""T8",   CV_IA64_IntT8,   T_UQUAD },
    {"\002""T9",   CV_IA64_IntT9,   T_UQUAD },
    {"\003""T10",  CV_IA64_IntT10,  T_UQUAD },
    {"\003""T11",  CV_IA64_IntT11,  T_UQUAD },
    {"\003""T12",  CV_IA64_IntT12,  T_UQUAD },
    {"\003""T13",  CV_IA64_IntT13,  T_UQUAD },
    {"\003""T14",  CV_IA64_IntT14,  T_UQUAD },
    {"\003""T15",  CV_IA64_IntT15,  T_UQUAD },
    {"\003""T16",  CV_IA64_IntT16,  T_UQUAD },
    {"\003""T17",  CV_IA64_IntT17,  T_UQUAD },
    {"\003""T18",  CV_IA64_IntT18,  T_UQUAD },
    {"\003""T19",  CV_IA64_IntT19,  T_UQUAD },
    {"\003""T20",  CV_IA64_IntT20,  T_UQUAD },
    {"\003""T21",  CV_IA64_IntT21,  T_UQUAD },
    {"\003""T22",  CV_IA64_IntT22,  T_UQUAD },

    {"\003""R32",  CV_IA64_IntR32,  T_UQUAD },
    {"\003""R33",  CV_IA64_IntR33,  T_UQUAD },
    {"\003""R34",  CV_IA64_IntR34,  T_UQUAD },
    {"\003""R35",  CV_IA64_IntR35,  T_UQUAD },
    {"\003""R36",  CV_IA64_IntR36,  T_UQUAD },
    {"\003""R37",  CV_IA64_IntR37,  T_UQUAD },
    {"\003""R38",  CV_IA64_IntR38,  T_UQUAD },
    {"\003""R39",  CV_IA64_IntR39,  T_UQUAD },
    {"\003""R40",  CV_IA64_IntR40,  T_UQUAD },
    {"\003""R41",  CV_IA64_IntR41,  T_UQUAD },
    {"\003""R42",  CV_IA64_IntR42,  T_UQUAD },
    {"\003""R43",  CV_IA64_IntR43,  T_UQUAD },
    {"\003""R44",  CV_IA64_IntR44,  T_UQUAD },
    {"\003""R45",  CV_IA64_IntR45,  T_UQUAD },
    {"\003""R46",  CV_IA64_IntR46,  T_UQUAD },
    {"\003""R47",  CV_IA64_IntR47,  T_UQUAD },
    {"\003""R48",  CV_IA64_IntR48,  T_UQUAD },
    {"\003""R49",  CV_IA64_IntR49,  T_UQUAD },
    {"\003""R50",  CV_IA64_IntR50,  T_UQUAD },
    {"\003""R51",  CV_IA64_IntR51,  T_UQUAD },
    {"\003""R52",  CV_IA64_IntR52,  T_UQUAD },
    {"\003""R53",  CV_IA64_IntR53,  T_UQUAD },
    {"\003""R54",  CV_IA64_IntR54,  T_UQUAD },
    {"\003""R55",  CV_IA64_IntR55,  T_UQUAD },
    {"\003""R56",  CV_IA64_IntR56,  T_UQUAD },
    {"\003""R57",  CV_IA64_IntR57,  T_UQUAD },
    {"\003""R58",  CV_IA64_IntR58,  T_UQUAD },
    {"\003""R59",  CV_IA64_IntR59,  T_UQUAD },
    {"\003""R60",  CV_IA64_IntR60,  T_UQUAD },
    {"\003""R61",  CV_IA64_IntR61,  T_UQUAD },
    {"\003""R62",  CV_IA64_IntR62,  T_UQUAD },
    {"\003""R63",  CV_IA64_IntR63,  T_UQUAD },
    {"\003""R64",  CV_IA64_IntR64,  T_UQUAD },
    {"\003""R65",  CV_IA64_IntR65,  T_UQUAD },
    {"\003""R66",  CV_IA64_IntR66,  T_UQUAD },
    {"\003""R67",  CV_IA64_IntR67,  T_UQUAD },
    {"\003""R68",  CV_IA64_IntR68,  T_UQUAD },
    {"\003""R69",  CV_IA64_IntR69,  T_UQUAD },
    {"\003""R70",  CV_IA64_IntR70,  T_UQUAD },
    {"\003""R71",  CV_IA64_IntR71,  T_UQUAD },
    {"\003""R72",  CV_IA64_IntR72,  T_UQUAD },
    {"\003""R73",  CV_IA64_IntR73,  T_UQUAD },
    {"\003""R74",  CV_IA64_IntR74,  T_UQUAD },
    {"\003""R75",  CV_IA64_IntR75,  T_UQUAD },
    {"\003""R76",  CV_IA64_IntR76,  T_UQUAD },
    {"\003""R77",  CV_IA64_IntR77,  T_UQUAD },
    {"\003""R78",  CV_IA64_IntR78,  T_UQUAD },
    {"\003""R79",  CV_IA64_IntR79,  T_UQUAD },
    {"\003""R80",  CV_IA64_IntR80,  T_UQUAD },
    {"\003""R81",  CV_IA64_IntR81,  T_UQUAD },
    {"\003""R82",  CV_IA64_IntR82,  T_UQUAD },
    {"\003""R83",  CV_IA64_IntR83,  T_UQUAD },
    {"\003""R84",  CV_IA64_IntR84,  T_UQUAD },
    {"\003""R85",  CV_IA64_IntR85,  T_UQUAD },
    {"\003""R86",  CV_IA64_IntR86,  T_UQUAD },
    {"\003""R87",  CV_IA64_IntR87,  T_UQUAD },
    {"\003""R88",  CV_IA64_IntR88,  T_UQUAD },
    {"\003""R89",  CV_IA64_IntR89,  T_UQUAD },
    {"\003""R90",  CV_IA64_IntR90,  T_UQUAD },
    {"\003""R91",  CV_IA64_IntR91,  T_UQUAD },
    {"\003""R92",  CV_IA64_IntR92,  T_UQUAD },
    {"\003""R93",  CV_IA64_IntR93,  T_UQUAD },
    {"\003""R94",  CV_IA64_IntR94,  T_UQUAD },
    {"\003""R95",  CV_IA64_IntR95,  T_UQUAD },
    {"\003""R96",  CV_IA64_IntR96,  T_UQUAD },
    {"\003""R97",  CV_IA64_IntR97,  T_UQUAD },
    {"\003""R98",  CV_IA64_IntR98,  T_UQUAD },
    {"\003""R99",  CV_IA64_IntR99,  T_UQUAD },
    {"\004""R100", CV_IA64_IntR100, T_UQUAD },
    {"\004""R101", CV_IA64_IntR101, T_UQUAD },
    {"\004""R102", CV_IA64_IntR102, T_UQUAD },
    {"\004""R103", CV_IA64_IntR103, T_UQUAD },
    {"\004""R104", CV_IA64_IntR104, T_UQUAD },
    {"\004""R105", CV_IA64_IntR105, T_UQUAD },
    {"\004""R106", CV_IA64_IntR106, T_UQUAD },
    {"\004""R107", CV_IA64_IntR107, T_UQUAD },
    {"\004""R108", CV_IA64_IntR108, T_UQUAD },
    {"\004""R109", CV_IA64_IntR109, T_UQUAD },
    {"\004""R110", CV_IA64_IntR110, T_UQUAD },
    {"\004""R111", CV_IA64_IntR111, T_UQUAD },
    {"\004""R112", CV_IA64_IntR112, T_UQUAD },
    {"\004""R113", CV_IA64_IntR113, T_UQUAD },
    {"\004""R114", CV_IA64_IntR114, T_UQUAD },
    {"\004""R115", CV_IA64_IntR115, T_UQUAD },
    {"\004""R116", CV_IA64_IntR116, T_UQUAD },
    {"\004""R117", CV_IA64_IntR117, T_UQUAD },
    {"\004""R118", CV_IA64_IntR118, T_UQUAD },
    {"\004""R119", CV_IA64_IntR119, T_UQUAD },
    {"\004""R120", CV_IA64_IntR120, T_UQUAD },
    {"\004""R121", CV_IA64_IntR121, T_UQUAD },
    {"\004""R122", CV_IA64_IntR122, T_UQUAD },
    {"\004""R123", CV_IA64_IntR123, T_UQUAD },
    {"\004""R124", CV_IA64_IntR124, T_UQUAD },
    {"\004""R125", CV_IA64_IntR125, T_UQUAD },
    {"\004""R126", CV_IA64_IntR126, T_UQUAD },
    {"\004""R127", CV_IA64_IntR127, T_UQUAD },


    {"\005""FZERO", CV_IA64_FltZero,  T_REAL80 },
    {"\004""FONE",  CV_IA64_FltOne,   T_REAL80 },
    {"\003""FS0",   CV_IA64_FltS0,    T_REAL80 },
    {"\003""FS1",   CV_IA64_FltS1,    T_REAL80 },
    {"\003""FS2",   CV_IA64_FltS2,    T_REAL80 },
    {"\003""FS3",   CV_IA64_FltS3,    T_REAL80 },
    {"\003""FT0",   CV_IA64_FltT0,    T_REAL80 },
    {"\003""FT1",   CV_IA64_FltT1,    T_REAL80 },
    {"\003""FT2",   CV_IA64_FltT2,    T_REAL80 },
    {"\003""FT3",   CV_IA64_FltT3,    T_REAL80 },
    {"\003""FT4",   CV_IA64_FltT4,    T_REAL80 },
    {"\003""FT5",   CV_IA64_FltT5,    T_REAL80 },
    {"\003""FT6",   CV_IA64_FltT6,    T_REAL80 },
    {"\003""FT7",   CV_IA64_FltT7,    T_REAL80 },
    {"\003""FT8",   CV_IA64_FltT8,    T_REAL80 },
    {"\003""FT9",   CV_IA64_FltT9,    T_REAL80 },
    {"\003""FS4",   CV_IA64_FltS4,    T_REAL80 },
    {"\003""FS5",   CV_IA64_FltS5,    T_REAL80 },
    {"\003""FS6",   CV_IA64_FltS6,    T_REAL80 },
    {"\003""FS7",   CV_IA64_FltS7,    T_REAL80 },
    {"\003""FS8",   CV_IA64_FltS8,    T_REAL80 },
    {"\003""FS9",   CV_IA64_FltS9,    T_REAL80 },
    {"\004""FS10",  CV_IA64_FltS10,   T_REAL80 },
    {"\004""FS11",  CV_IA64_FltS11,   T_REAL80 },
    {"\004""FS12",  CV_IA64_FltS12,   T_REAL80 },
    {"\004""FS13",  CV_IA64_FltS13,   T_REAL80 },
    {"\004""FS14",  CV_IA64_FltS14,   T_REAL80 },
    {"\004""FS15",  CV_IA64_FltS15,   T_REAL80 },
    {"\004""FS16",  CV_IA64_FltS16,   T_REAL80 },
    {"\004""FS17",  CV_IA64_FltS17,   T_REAL80 },
    {"\004""FS18",  CV_IA64_FltS18,   T_REAL80 },
    {"\004""FS19",  CV_IA64_FltS19,   T_REAL80 },

    {"\003""F32",   CV_IA64_FltF32,   T_REAL80 },
    {"\003""F33",   CV_IA64_FltF33,   T_REAL80 },
    {"\003""F34",   CV_IA64_FltF34,   T_REAL80 },
    {"\003""F35",   CV_IA64_FltF35,   T_REAL80 },
    {"\003""F36",   CV_IA64_FltF36,   T_REAL80 },
    {"\003""F37",   CV_IA64_FltF37,   T_REAL80 },
    {"\003""F38",   CV_IA64_FltF38,   T_REAL80 },
    {"\003""F39",   CV_IA64_FltF39,   T_REAL80 },
    {"\003""F40",   CV_IA64_FltF40,   T_REAL80 },
    {"\003""F41",   CV_IA64_FltF41,   T_REAL80 },
    {"\003""F42",   CV_IA64_FltF42,   T_REAL80 },
    {"\003""F43",   CV_IA64_FltF43,   T_REAL80 },
    {"\003""F44",   CV_IA64_FltF44,   T_REAL80 },
    {"\003""F45",   CV_IA64_FltF45,   T_REAL80 },
    {"\003""F46",   CV_IA64_FltF46,   T_REAL80 },
    {"\003""F47",   CV_IA64_FltF47,   T_REAL80 },
    {"\003""F48",   CV_IA64_FltF48,   T_REAL80 },
    {"\003""F49",   CV_IA64_FltF49,   T_REAL80 },
    {"\003""F50",   CV_IA64_FltF50,   T_REAL80 },
    {"\003""F51",   CV_IA64_FltF51,   T_REAL80 },
    {"\003""F52",   CV_IA64_FltF52,   T_REAL80 },
    {"\003""F53",   CV_IA64_FltF53,   T_REAL80 },
    {"\003""F54",   CV_IA64_FltF54,   T_REAL80 },
    {"\003""F55",   CV_IA64_FltF55,   T_REAL80 },
    {"\003""F56",   CV_IA64_FltF56,   T_REAL80 },
    {"\003""F57",   CV_IA64_FltF57,   T_REAL80 },
    {"\003""F58",   CV_IA64_FltF58,   T_REAL80 },
    {"\003""F59",   CV_IA64_FltF59,   T_REAL80 },
    {"\003""F60",   CV_IA64_FltF60,   T_REAL80 },
    {"\003""F61",   CV_IA64_FltF61,   T_REAL80 },
    {"\003""F62",   CV_IA64_FltF62,   T_REAL80 },
    {"\003""F63",   CV_IA64_FltF63,   T_REAL80 },
    {"\003""F64",   CV_IA64_FltF64,   T_REAL80 },
    {"\003""F65",   CV_IA64_FltF65,   T_REAL80 },
    {"\003""F66",   CV_IA64_FltF66,   T_REAL80 },
    {"\003""F67",   CV_IA64_FltF67,   T_REAL80 },
    {"\003""F68",   CV_IA64_FltF68,   T_REAL80 },
    {"\003""F69",   CV_IA64_FltF69,   T_REAL80 },
    {"\003""F70",   CV_IA64_FltF70,   T_REAL80 },
    {"\003""F71",   CV_IA64_FltF71,   T_REAL80 },
    {"\003""F72",   CV_IA64_FltF72,   T_REAL80 },
    {"\003""F73",   CV_IA64_FltF73,   T_REAL80 },
    {"\003""F74",   CV_IA64_FltF74,   T_REAL80 },
    {"\003""F75",   CV_IA64_FltF75,   T_REAL80 },
    {"\003""F76",   CV_IA64_FltF76,   T_REAL80 },
    {"\003""F77",   CV_IA64_FltF77,   T_REAL80 },
    {"\003""F78",   CV_IA64_FltF78,   T_REAL80 },
    {"\003""F79",   CV_IA64_FltF79,   T_REAL80 },
    {"\003""F80",   CV_IA64_FltF80,   T_REAL80 },
    {"\003""F81",   CV_IA64_FltF81,   T_REAL80 },
    {"\003""F82",   CV_IA64_FltF82,   T_REAL80 },
    {"\003""F83",   CV_IA64_FltF83,   T_REAL80 },
    {"\003""F84",   CV_IA64_FltF84,   T_REAL80 },
    {"\003""F85",   CV_IA64_FltF85,   T_REAL80 },
    {"\003""F86",   CV_IA64_FltF86,   T_REAL80 },
    {"\003""F87",   CV_IA64_FltF87,   T_REAL80 },
    {"\003""F88",   CV_IA64_FltF88,   T_REAL80 },
    {"\003""F89",   CV_IA64_FltF89,   T_REAL80 },
    {"\003""F90",   CV_IA64_FltF90,   T_REAL80 },
    {"\003""F91",   CV_IA64_FltF91,   T_REAL80 },
    {"\003""F92",   CV_IA64_FltF92,   T_REAL80 },
    {"\003""F93",   CV_IA64_FltF93,   T_REAL80 },
    {"\003""F94",   CV_IA64_FltF94,   T_REAL80 },
    {"\003""F95",   CV_IA64_FltF95,   T_REAL80 },
    {"\003""F96",   CV_IA64_FltF96,   T_REAL80 },
    {"\003""F97",   CV_IA64_FltF97,   T_REAL80 },
    {"\003""F98",   CV_IA64_FltF98,   T_REAL80 },
    {"\003""F99",   CV_IA64_FltF99,   T_REAL80 },
    {"\004""F100",  CV_IA64_FltF100,  T_REAL80 },
    {"\004""F101",  CV_IA64_FltF101,  T_REAL80 },
    {"\004""F102",  CV_IA64_FltF102,  T_REAL80 },
    {"\004""F103",  CV_IA64_FltF103,  T_REAL80 },
    {"\004""F104",  CV_IA64_FltF104,  T_REAL80 },
    {"\004""F105",  CV_IA64_FltF105,  T_REAL80 },
    {"\004""F106",  CV_IA64_FltF106,  T_REAL80 },
    {"\004""F107",  CV_IA64_FltF107,  T_REAL80 },
    {"\004""F108",  CV_IA64_FltF108,  T_REAL80 },
    {"\004""F109",  CV_IA64_FltF109,  T_REAL80 },
    {"\004""F110",  CV_IA64_FltF110,  T_REAL80 },
    {"\004""F111",  CV_IA64_FltF111,  T_REAL80 },
    {"\004""F112",  CV_IA64_FltF112,  T_REAL80 },
    {"\004""F113",  CV_IA64_FltF113,  T_REAL80 },
    {"\004""F114",  CV_IA64_FltF114,  T_REAL80 },
    {"\004""F115",  CV_IA64_FltF115,  T_REAL80 },
    {"\004""F116",  CV_IA64_FltF116,  T_REAL80 },
    {"\004""F117",  CV_IA64_FltF117,  T_REAL80 },
    {"\004""F118",  CV_IA64_FltF118,  T_REAL80 },
    {"\004""F119",  CV_IA64_FltF119,  T_REAL80 },
    {"\004""F120",  CV_IA64_FltF120,  T_REAL80 },
    {"\004""F121",  CV_IA64_FltF121,  T_REAL80 },
    {"\004""F122",  CV_IA64_FltF122,  T_REAL80 },
    {"\004""F123",  CV_IA64_FltF123,  T_REAL80 },
    {"\004""F124",  CV_IA64_FltF124,  T_REAL80 },
    {"\004""F125",  CV_IA64_FltF125,  T_REAL80 },
    {"\004""F126",  CV_IA64_FltF126,  T_REAL80 },
    {"\004""F127",  CV_IA64_FltF127,  T_REAL80 },

    {"\003""KR0",   CV_IA64_ApKR0,    T_UQUAD },
    {"\003""KR1",   CV_IA64_ApKR1,    T_UQUAD },
    {"\003""KR2",   CV_IA64_ApKR2,    T_UQUAD },
    {"\003""KR3",   CV_IA64_ApKR3,    T_UQUAD },
    {"\003""KR4",   CV_IA64_ApKR4,    T_UQUAD },
    {"\003""KR5",   CV_IA64_ApKR5,    T_UQUAD },
    {"\003""KR6",   CV_IA64_ApKR6,    T_UQUAD },
    {"\003""KR7",   CV_IA64_ApKR7,    T_UQUAD },
    {"\003""AR8",   CV_IA64_AR8,      T_UQUAD },
    {"\004""AR90",  CV_IA64_AR9,      T_UQUAD },
    {"\004""AR10",  CV_IA64_AR10,     T_UQUAD },
    {"\004""AR11",  CV_IA64_AR11,     T_UQUAD },
    {"\004""AR12",  CV_IA64_AR12,     T_UQUAD },
    {"\004""AR13",  CV_IA64_AR13,     T_UQUAD },
    {"\004""AR14",  CV_IA64_AR14,     T_UQUAD },
    {"\004""AR15",  CV_IA64_AR15,     T_UQUAD },
    {"\003""RSC",   CV_IA64_RsRSC,    T_UQUAD },
    {"\003""BSP",   CV_IA64_RsBSP,    T_UQUAD },
    {"\007""BSPSTORE",  CV_IA64_RsBSPSTORE,   T_UQUAD },
    {"\004""RNAT",  CV_IA64_RsRNAT,   T_UQUAD },
    {"\004""AR20",  CV_IA64_AR20,     T_UQUAD },
    {"\004""AR21",  CV_IA64_AR21,     T_UQUAD },
    {"\004""AR22",  CV_IA64_AR22,     T_UQUAD },
    {"\004""AR23",  CV_IA64_AR23,     T_UQUAD },
    {"\004""AR24",  CV_IA64_AR24,     T_UQUAD },
    {"\004""AR25",  CV_IA64_AR25,     T_UQUAD },
    {"\004""AR26",  CV_IA64_AR26,     T_UQUAD },
    {"\004""AR27",  CV_IA64_AR27,     T_UQUAD },
    {"\004""AR28",  CV_IA64_AR28,     T_UQUAD },
    {"\004""AR29",  CV_IA64_AR29,     T_UQUAD },
    {"\004""AR30",  CV_IA64_AR30,     T_UQUAD },
    {"\004""AR31",  CV_IA64_AR31,     T_UQUAD },

    {"\003""CCV",   CV_IA64_ApCCV,    T_UQUAD },
    {"\004""AR33",  CV_IA64_AR33,     T_UQUAD },
    {"\004""AR34",  CV_IA64_AR34,     T_UQUAD },
    {"\004""AR35",  CV_IA64_AR35,     T_UQUAD },
    {"\004""UNAT",  CV_IA64_ApUNAT,   T_UQUAD },
    {"\004""AR37",  CV_IA64_AR37,     T_UQUAD },
    {"\004""AR38",  CV_IA64_AR38,     T_UQUAD },
    {"\004""AR39",  CV_IA64_AR39,     T_UQUAD },
    {"\004""FPSR",  CV_IA64_StFPSR,     T_UQUAD },
    {"\004""AR41",  CV_IA64_AR41,     T_UQUAD },
    {"\004""AR42",  CV_IA64_AR42,     T_UQUAD },
    {"\004""AR43",  CV_IA64_AR43,     T_UQUAD },
    {"\003""ITC",   CV_IA64_ApITC,    T_UQUAD },
    {"\004""AR45",  CV_IA64_AR45,     T_UQUAD },
    {"\004""AR46",  CV_IA64_AR46,     T_UQUAD },
    {"\004""AR47",  CV_IA64_AR47,     T_UQUAD },
    {"\004""AR48",  CV_IA64_AR48,     T_UQUAD },
    {"\004""AR49",  CV_IA64_AR49,     T_UQUAD },
    {"\004""AR50",  CV_IA64_AR50,     T_UQUAD },
    {"\004""AR51",  CV_IA64_AR51,     T_UQUAD },
    {"\004""AR52",  CV_IA64_AR52,     T_UQUAD },
    {"\004""AR53",  CV_IA64_AR53,     T_UQUAD },
    {"\004""AR54",  CV_IA64_AR54,     T_UQUAD },
    {"\004""AR55",  CV_IA64_AR55,     T_UQUAD },
    {"\004""AR56",  CV_IA64_AR56,     T_UQUAD },
    {"\004""AR57",  CV_IA64_AR57,     T_UQUAD },
    {"\004""AR58",  CV_IA64_AR58,     T_UQUAD },
    {"\004""AR59",  CV_IA64_AR59,     T_UQUAD },
    {"\004""AR60",  CV_IA64_AR60,     T_UQUAD },
    {"\004""AR61",  CV_IA64_AR61,     T_UQUAD },
    {"\004""AR62",  CV_IA64_AR62,     T_UQUAD },
    {"\004""AR63",  CV_IA64_AR63,     T_UQUAD },
    {"\003""PFS",   CV_IA64_RsPFS,    T_UQUAD },
    {"\002""LC",    CV_IA64_ApLC,     T_UQUAD },
    {"\002""EC",    CV_IA64_ApEC,     T_UQUAD },
    {"\004""AR67",  CV_IA64_AR67,     T_UQUAD },
    {"\004""AR68",  CV_IA64_AR68,     T_UQUAD },
    {"\004""AR69",  CV_IA64_AR69,     T_UQUAD },
    {"\004""AR70",  CV_IA64_AR70,     T_UQUAD },
    {"\004""AR71",  CV_IA64_AR71,     T_UQUAD },
    {"\004""AR72",  CV_IA64_AR72,     T_UQUAD },
    {"\004""AR73",  CV_IA64_AR73,     T_UQUAD },
    {"\004""AR74",  CV_IA64_AR74,     T_UQUAD },
    {"\004""AR75",  CV_IA64_AR75,     T_UQUAD },
    {"\004""AR76",  CV_IA64_AR76,     T_UQUAD },
    {"\004""AR77",  CV_IA64_AR77,     T_UQUAD },
    {"\004""AR78",  CV_IA64_AR78,     T_UQUAD },
    {"\004""AR79",  CV_IA64_AR79,     T_UQUAD },
    {"\004""AR80",  CV_IA64_AR80,     T_UQUAD },
    {"\004""AR81",  CV_IA64_AR81,     T_UQUAD },
    {"\004""AR82",  CV_IA64_AR82,     T_UQUAD },
    {"\004""AR83",  CV_IA64_AR83,     T_UQUAD },
    {"\004""AR84",  CV_IA64_AR84,     T_UQUAD },
    {"\004""AR85",  CV_IA64_AR85,     T_UQUAD },
    {"\004""AR86",  CV_IA64_AR86,     T_UQUAD },
    {"\004""AR87",  CV_IA64_AR87,     T_UQUAD },
    {"\004""AR88",  CV_IA64_AR88,     T_UQUAD },
    {"\004""AR89",  CV_IA64_AR89,     T_UQUAD },
    {"\004""AR90",  CV_IA64_AR90,     T_UQUAD },
    {"\004""AR91",  CV_IA64_AR91,     T_UQUAD },
    {"\004""AR92",  CV_IA64_AR92,     T_UQUAD },
    {"\004""AR93",  CV_IA64_AR93,     T_UQUAD },
    {"\004""AR94",  CV_IA64_AR94,     T_UQUAD },
    {"\004""AR95",  CV_IA64_AR95,     T_UQUAD },
    {"\004""AR96",  CV_IA64_AR96,     T_UQUAD },
    {"\004""AR97",  CV_IA64_AR97,     T_UQUAD },
    {"\004""AR98",  CV_IA64_AR98,     T_UQUAD },
    {"\004""AR99",  CV_IA64_AR99,     T_UQUAD },
    {"\005""AR100", CV_IA64_AR100,    T_UQUAD },
    {"\005""AR101", CV_IA64_AR101,    T_UQUAD },
    {"\005""AR102", CV_IA64_AR102,    T_UQUAD },
    {"\005""AR103", CV_IA64_AR103,    T_UQUAD },
    {"\005""AR104", CV_IA64_AR104,    T_UQUAD },
    {"\005""AR105", CV_IA64_AR105,    T_UQUAD },
    {"\005""AR106", CV_IA64_AR106,    T_UQUAD },
    {"\005""AR107", CV_IA64_AR107,    T_UQUAD },
    {"\005""AR108", CV_IA64_AR108,    T_UQUAD },
    {"\005""AR109", CV_IA64_AR109,    T_UQUAD },
    {"\005""AR110", CV_IA64_AR110,    T_UQUAD },
    {"\005""AR111", CV_IA64_AR111,    T_UQUAD },
    {"\005""AR112", CV_IA64_AR112,    T_UQUAD },
    {"\005""AR113", CV_IA64_AR113,    T_UQUAD },
    {"\005""AR114", CV_IA64_AR114,    T_UQUAD },
    {"\005""AR115", CV_IA64_AR115,    T_UQUAD },
    {"\005""AR116", CV_IA64_AR116,    T_UQUAD },
    {"\005""AR117", CV_IA64_AR117,    T_UQUAD },
    {"\005""AR118", CV_IA64_AR118,    T_UQUAD },
    {"\005""AR119", CV_IA64_AR119,    T_UQUAD },
    {"\005""AR120", CV_IA64_AR120,    T_UQUAD },
    {"\005""AR121", CV_IA64_AR121,    T_UQUAD },
    {"\005""AR122", CV_IA64_AR122,    T_UQUAD },
    {"\005""AR123", CV_IA64_AR123,    T_UQUAD },
    {"\005""AR124", CV_IA64_AR124,    T_UQUAD },
    {"\005""AR125", CV_IA64_AR125,    T_UQUAD },
    {"\005""AR126", CV_IA64_AR126,    T_UQUAD },
    {"\005""AR127", CV_IA64_AR127,    T_UQUAD },

    {"\003""DCR",   CV_IA64_ApDCR,    T_UQUAD },
    {"\003""ITM",   CV_IA64_ApITM,    T_UQUAD },
    {"\003""IVA",   CV_IA64_ApIVA,    T_UQUAD },
    {"\003""CR3",   CV_IA64_CR3,      T_UQUAD },
    {"\003""CR4",   CV_IA64_CR4,      T_UQUAD },
    {"\003""CR5",   CV_IA64_CR5,      T_UQUAD },
    {"\003""CR6",   CV_IA64_CR6,      T_UQUAD },
    {"\003""CR7",   CV_IA64_CR7,      T_UQUAD },
    {"\003""PTA",   CV_IA64_ApPTA,    T_UQUAD },
    {"\003""CR9",   CV_IA64_CR9,      T_UQUAD },
    {"\004""CR10",  CV_IA64_CR10,     T_UQUAD },
    {"\004""CR11",  CV_IA64_CR11,     T_UQUAD },
    {"\004""CR12",  CV_IA64_CR12,     T_UQUAD },
    {"\004""CR13",  CV_IA64_CR13,     T_UQUAD },
    {"\004""CR14",  CV_IA64_CR14,     T_UQUAD },
    {"\004""CR15",  CV_IA64_CR15,     T_UQUAD },
    {"\004""IPSR",  CV_IA64_StIPSR,   T_UQUAD },
    {"\003""ISR ",  CV_IA64_StISR,    T_UQUAD },
    {"\003""IDA",   CV_IA64_StIDA,    T_UQUAD },
    {"\003""IIP",   CV_IA64_StIIP,    T_UQUAD },
    {"\004""IDTR",  CV_IA64_StIDTR,   T_UQUAD },
    {"\004""IITR",  CV_IA64_StIITR,   T_UQUAD },
    {"\004""IIPA",  CV_IA64_StIIPA,   T_UQUAD },
    {"\003""IFS",   CV_IA64_StIFS,    T_UQUAD },
    {"\003""IIM",   CV_IA64_StIIM,    T_UQUAD },
    {"\003""IHA",   CV_IA64_StIHA,    T_UQUAD },
    {"\004""CR26",  CV_IA64_CR26,     T_UQUAD },
    {"\004""CR27",  CV_IA64_CR27,     T_UQUAD },
    {"\004""CR28",  CV_IA64_CR28,     T_UQUAD },
    {"\004""CR29",  CV_IA64_CR29,     T_UQUAD },
    {"\004""CR30",  CV_IA64_CR30,     T_UQUAD },
    {"\004""CR31",  CV_IA64_CR31,     T_UQUAD },

    {"\004""CR32",  CV_IA64_CR32,     T_UQUAD },
    {"\004""CR33",  CV_IA64_CR33,     T_UQUAD },
    {"\004""CR34",  CV_IA64_CR34,     T_UQUAD },
    {"\004""CR35",  CV_IA64_CR35,     T_UQUAD },
    {"\004""CR36",  CV_IA64_CR36,     T_UQUAD },
    {"\004""CR37",  CV_IA64_CR37,     T_UQUAD },
    {"\004""CR38",  CV_IA64_CR38,     T_UQUAD },
    {"\004""CR39",  CV_IA64_CR39,     T_UQUAD },
    {"\004""CR40",  CV_IA64_CR40,     T_UQUAD },
    {"\004""CR41",  CV_IA64_CR41,     T_UQUAD },
    {"\004""CR42",  CV_IA64_CR42,     T_UQUAD },
    {"\004""CR43",  CV_IA64_CR43,     T_UQUAD },
    {"\004""CR44",  CV_IA64_CR44,     T_UQUAD },
    {"\004""CR45",  CV_IA64_CR45,     T_UQUAD },
    {"\004""CR46",  CV_IA64_CR46,     T_UQUAD },
    {"\004""CR47",  CV_IA64_CR47,     T_UQUAD },
    {"\004""CR48",  CV_IA64_CR48,     T_UQUAD },
    {"\004""CR49",  CV_IA64_CR49,     T_UQUAD },
    {"\004""CR50",  CV_IA64_CR50,     T_UQUAD },
    {"\004""CR51",  CV_IA64_CR51,     T_UQUAD },
    {"\004""CR52",  CV_IA64_CR52,     T_UQUAD },
    {"\004""CR53",  CV_IA64_CR53,     T_UQUAD },
    {"\004""CR54",  CV_IA64_CR54,     T_UQUAD },
    {"\004""CR55",  CV_IA64_CR55,     T_UQUAD },
    {"\004""CR56",  CV_IA64_CR56,     T_UQUAD },
    {"\004""CR57",  CV_IA64_CR57,     T_UQUAD },
    {"\004""CR58",  CV_IA64_CR58,     T_UQUAD },
    {"\004""CR59",  CV_IA64_CR59,     T_UQUAD },
    {"\004""CR60",  CV_IA64_CR60,     T_UQUAD },
    {"\004""CR61",  CV_IA64_CR61,     T_UQUAD },
    {"\004""CR62",  CV_IA64_CR62,     T_UQUAD },
    {"\004""CR63",  CV_IA64_CR63,     T_UQUAD },
    {"\004""CR64",  CV_IA64_CR64,     T_UQUAD },
    {"\004""CR65",  CV_IA64_CR65,     T_UQUAD },
    {"\003""LID ",  CV_IA64_SaLID,    T_UQUAD },
    {"\004""CR67",  CV_IA64_CR67,     T_UQUAD },
    {"\004""CR68",  CV_IA64_CR68,     T_UQUAD },
    {"\004""CR69",  CV_IA64_CR69,     T_UQUAD },
    {"\004""CR70",  CV_IA64_CR70,     T_UQUAD },
    {"\003""IVR ",  CV_IA64_SaIVR,    T_UQUAD },
    {"\003""TPR ",  CV_IA64_SaTPR,    T_UQUAD },
    {"\004""CR73",  CV_IA64_CR73,     T_UQUAD },
    {"\004""CR74",  CV_IA64_CR74,     T_UQUAD },
    {"\003""EOI ",  CV_IA64_SaEOI,    T_UQUAD },
    {"\004""CR76",  CV_IA64_CR76,     T_UQUAD },
    {"\004""CR77",  CV_IA64_CR77,     T_UQUAD },
    {"\004""CR78",  CV_IA64_CR78,     T_UQUAD },
    {"\004""CR79",  CV_IA64_CR79,     T_UQUAD },
    {"\004""CR80",  CV_IA64_CR80,     T_UQUAD },
    {"\004""CR81",  CV_IA64_CR81,     T_UQUAD },
    {"\004""CR82",  CV_IA64_CR82,     T_UQUAD },
    {"\004""CR83",  CV_IA64_CR83,     T_UQUAD },
    {"\004""CR84",  CV_IA64_CR84,     T_UQUAD },
    {"\004""CR85",  CV_IA64_CR85,     T_UQUAD },
    {"\004""CR86",  CV_IA64_CR86,     T_UQUAD },
    {"\004""CR87",  CV_IA64_CR87,     T_UQUAD },
    {"\004""CR88",  CV_IA64_CR88,     T_UQUAD },
    {"\004""CR89",  CV_IA64_CR89,     T_UQUAD },
    {"\004""CR90",  CV_IA64_CR90,     T_UQUAD },
    {"\004""CR91",  CV_IA64_CR91,     T_UQUAD },
    {"\004""CR92",  CV_IA64_CR92,     T_UQUAD },
    {"\004""CR93",  CV_IA64_CR93,     T_UQUAD },
    {"\004""CR94",  CV_IA64_CR94,     T_UQUAD },
    {"\004""CR95",  CV_IA64_CR95,     T_UQUAD },
    {"\004""IRR0",  CV_IA64_SaIRR0,   T_UQUAD },
    {"\004""CR97",  CV_IA64_CR97,     T_UQUAD },
    {"\004""IRR1",  CV_IA64_SaIRR1,   T_UQUAD },
    {"\004""CR99",  CV_IA64_CR99,     T_UQUAD },
    {"\004""IRR2",  CV_IA64_SaIRR2,   T_UQUAD },
    {"\005""CR101", CV_IA64_CR101,    T_UQUAD },
    {"\004""IRR3",  CV_IA64_SaIRR3,   T_UQUAD },
    {"\005""CR103", CV_IA64_CR103,    T_UQUAD },
    {"\005""CR104", CV_IA64_CR104,    T_UQUAD },
    {"\005""CR105", CV_IA64_CR105,    T_UQUAD },
    {"\005""CR106", CV_IA64_CR106,    T_UQUAD },
    {"\005""CR107", CV_IA64_CR107,    T_UQUAD },
    {"\005""CR108", CV_IA64_CR108,    T_UQUAD },
    {"\005""CR109", CV_IA64_CR109,    T_UQUAD },
    {"\005""CR110", CV_IA64_CR110,    T_UQUAD },
    {"\005""CR111", CV_IA64_CR111,    T_UQUAD },
    {"\005""CR112", CV_IA64_CR112,    T_UQUAD },
    {"\005""CR113", CV_IA64_CR113,    T_UQUAD },
    {"\003""ITV",   CV_IA64_SaITV,    T_UQUAD },
    {"\005""CR115", CV_IA64_CR115,    T_UQUAD },
    {"\003""PMV",   CV_IA64_SaPMV,    T_UQUAD },
    {"\004""LRR0",  CV_IA64_SaLRR0,   T_UQUAD },
    {"\004""LRR1",  CV_IA64_SaLRR1,   T_UQUAD },
    {"\004""CMCV",  CV_IA64_SaCMCV,   T_UQUAD },
    {"\005""CR120", CV_IA64_CR120,    T_UQUAD },
    {"\005""CR121", CV_IA64_CR121,    T_UQUAD },
    {"\005""CR122", CV_IA64_CR122,    T_UQUAD },
    {"\005""CR123", CV_IA64_CR123,    T_UQUAD },
    {"\005""CR124", CV_IA64_CR124,    T_UQUAD },
    {"\005""CR125", CV_IA64_CR125,    T_UQUAD },
    {"\005""CR126", CV_IA64_CR126,    T_UQUAD },
    {"\005""CR127", CV_IA64_CR127,    T_UQUAD },

    {"\004""PKR0",  CV_IA64_Pkr0 ,    T_UQUAD },
    {"\004""PKR1",  CV_IA64_Pkr1 ,    T_UQUAD },
    {"\004""PKR2",  CV_IA64_Pkr2 ,    T_UQUAD },
    {"\004""PKR3",  CV_IA64_Pkr3 ,    T_UQUAD },
    {"\004""PKR4",  CV_IA64_Pkr4 ,    T_UQUAD },
    {"\004""PKR5",  CV_IA64_Pkr5 ,    T_UQUAD },
    {"\004""PKR6",  CV_IA64_Pkr6 ,    T_UQUAD },
    {"\004""PKR7",  CV_IA64_Pkr7 ,    T_UQUAD },
    {"\004""PKR8",  CV_IA64_Pkr8 ,    T_UQUAD },
    {"\004""PKR9",  CV_IA64_Pkr9 ,    T_UQUAD },
    {"\005""PKR10", CV_IA64_Pkr10,    T_UQUAD },
    {"\005""PKR11", CV_IA64_Pkr11,    T_UQUAD },
    {"\005""PKR12", CV_IA64_Pkr12,    T_UQUAD },
    {"\005""PKR13", CV_IA64_Pkr13,    T_UQUAD },
    {"\005""PKR14", CV_IA64_Pkr14,    T_UQUAD },
    {"\005""PKR15", CV_IA64_Pkr15,    T_UQUAD },

    {"\003""RR0",   CV_IA64_Rr0,      T_UQUAD },
    {"\003""RR1",   CV_IA64_Rr1,      T_UQUAD },
    {"\003""RR2",   CV_IA64_Rr2,      T_UQUAD },
    {"\003""RR3",   CV_IA64_Rr3,      T_UQUAD },
    {"\003""RR4",   CV_IA64_Rr4,      T_UQUAD },
    {"\003""RR5",   CV_IA64_Rr5,      T_UQUAD },
    {"\003""RR6",   CV_IA64_Rr6,      T_UQUAD },
    {"\003""RR7",   CV_IA64_Rr7,      T_UQUAD },

    {"\004""PFD0",  CV_IA64_PFD0,     T_UQUAD },
    {"\004""PFD1",  CV_IA64_PFD1,     T_UQUAD },
    {"\004""PFD2",  CV_IA64_PFD2,     T_UQUAD },
    {"\004""PFD3",  CV_IA64_PFD3,     T_UQUAD },
    {"\004""PFD4",  CV_IA64_PFD4,     T_UQUAD },
    {"\004""PFD5",  CV_IA64_PFD5,     T_UQUAD },
    {"\004""PFD6",  CV_IA64_PFD6,     T_UQUAD },
    {"\004""PFD7",  CV_IA64_PFD7,     T_UQUAD },

    {"\004""PFC0",  CV_IA64_PFC0,     T_UQUAD },
    {"\004""PFC1",  CV_IA64_PFC1,     T_UQUAD },
    {"\004""PFC2",  CV_IA64_PFC2,     T_UQUAD },
    {"\004""PFC3",  CV_IA64_PFC3,     T_UQUAD },
    {"\004""PFC4",  CV_IA64_PFC4,     T_UQUAD },
    {"\004""PFC5",  CV_IA64_PFC5,     T_UQUAD },
    {"\004""PFC6",  CV_IA64_PFC6,     T_UQUAD },
    {"\004""PFC7",  CV_IA64_PFC7,     T_UQUAD },

    {"\004""TRI0",  CV_IA64_TrI0,     T_UQUAD },
    {"\004""TRI1",  CV_IA64_TrI1,     T_UQUAD },
    {"\004""TRI2",  CV_IA64_TrI2,     T_UQUAD },
    {"\004""TRI3",  CV_IA64_TrI3,     T_UQUAD },
    {"\004""TRI4",  CV_IA64_TrI4,     T_UQUAD },
    {"\004""TRI5",  CV_IA64_TrI5,     T_UQUAD },
    {"\004""TRI6",  CV_IA64_TrI6,     T_UQUAD },
    {"\004""TRI7",  CV_IA64_TrI7,     T_UQUAD },

    {"\004""TRD0",  CV_IA64_TrD0,     T_UQUAD },
    {"\004""TRD1",  CV_IA64_TrD1,     T_UQUAD },
    {"\004""TRD2",  CV_IA64_TrD2,     T_UQUAD },
    {"\004""TRD3",  CV_IA64_TrD3,     T_UQUAD },
    {"\004""TRD4",  CV_IA64_TrD4,     T_UQUAD },
    {"\004""TRD5",  CV_IA64_TrD5,     T_UQUAD },
    {"\004""TRD6",  CV_IA64_TrD6,     T_UQUAD },
    {"\004""TRD7",  CV_IA64_TrD7,     T_UQUAD },

    {"\004""DBI0",  CV_IA64_DbI0,     T_UQUAD },
    {"\004""DBI1",  CV_IA64_DbI1,     T_UQUAD },
    {"\004""DBI2",  CV_IA64_DbI2,     T_UQUAD },
    {"\004""DBI3",  CV_IA64_DbI3,     T_UQUAD },
    {"\004""DBI4",  CV_IA64_DbI4,     T_UQUAD },
    {"\004""DBI5",  CV_IA64_DbI5,     T_UQUAD },
    {"\004""DBI6",  CV_IA64_DbI6,     T_UQUAD },
    {"\004""DBI7",  CV_IA64_DbI7,     T_UQUAD },

    {"\004""DBD0",  CV_IA64_DbD0,     T_UQUAD },
    {"\004""DBD1",  CV_IA64_DbD1,     T_UQUAD },
    {"\004""DBD2",  CV_IA64_DbD2,     T_UQUAD },
    {"\004""DBD3",  CV_IA64_DbD3,     T_UQUAD },
    {"\004""DBD4",  CV_IA64_DbD4,     T_UQUAD },
    {"\004""DBD5",  CV_IA64_DbD5,     T_UQUAD },
    {"\004""DBD6",  CV_IA64_DbD6,     T_UQUAD },
    {"\004""DBD7",  CV_IA64_DbD7,     T_UQUAD }
};

#define REGCNT_X86  (sizeof (hreg_list_x86)/sizeof (struct hreg_x86))
#define REGCNT_MIP  (sizeof (hreg_list_mip)/sizeof (struct hreg_mip))
#define REGCNT_AXP  (sizeof (hreg_list_axp)/sizeof (struct hreg_axp))
#define REGCNT_PPC  (sizeof (hreg_list_ppc)/sizeof (struct hreg_ppc))
#define REGCNT_68K  (sizeof (hreg_list_68k)/sizeof (struct hreg_68k))
#define REGCNT_IA64 (sizeof (hreg_list_ia64)/sizeof (struct hreg_ia64))

bool_t
ParseRegister (
    psearch_t pName
    )
{
    int         i;
    peval_t     pv;

    if (*pName->sstr.lpName == '@') {
        pName->sstr.lpName++;
        pName->sstr.cb--;
    }

    pv = pName->pv;

    switch (TargetMachine) {
        case mptmips:
            for (i = 0; i < REGCNT_MIP; i++) {
                if (fnCmp (pName, NULL, hreg_list_mip[i].name, FALSE) == 0)
                    break;
            }

            if (i >= REGCNT_MIP) {
                return (HR_notfound);
            }
            EVAL_REG (pv) = hreg_list_mip[i].index;
            SetNodeType (pv,  hreg_list_mip[i].type);
            EVAL_IS_HIBYTE (pv) = FALSE;
            break;

        case mptix86:
            for (i = 0; i < REGCNT_X86; i++) {
                if (fnCmp (pName, NULL, hreg_list_x86[i].name, FALSE) == 0)
                    break;
            }
            if ((i >= REGCNT_X86) ||
                (!(hreg_list_x86[i].valid & VALIDx86) && !in386mode))
            {
                return (HR_notfound);
            }
            EVAL_REG (pv) = hreg_list_x86[i].index;
            SetNodeType (pv,  hreg_list_x86[i].type);
            EVAL_IS_HIBYTE (pv) = ((hreg_list_x86[i].valid & REGHI) == REGHI);
            break;

        case mptia64:
            for (i = 0; i < REGCNT_IA64; i++) {
                if (fnCmp (pName, NULL, hreg_list_ia64[i].name, FALSE) == 0)
                    break;
            }
            if (i >= REGCNT_IA64)
            {
                return (HR_notfound);
            }
            EVAL_REG (pv) = hreg_list_ia64[i].index;
            SetNodeType (pv,  hreg_list_ia64[i].type);
            EVAL_IS_HIBYTE (pv) = FALSE;
            break;

        case mptdaxp:
            for (i = 0; i < REGCNT_AXP; i++) {
                if (fnCmp (pName, NULL, hreg_list_axp[i].name, FALSE) == 0)
                    break;
            }
            if (i >= REGCNT_AXP) {
                return (HR_notfound);
            }
            EVAL_REG (pv) = hreg_list_axp[i].index;
            SetNodeType (pv,  hreg_list_axp[i].type);
            EVAL_IS_HIBYTE (pv) = FALSE;
            break;

        case mptmppc:
        case mptntppc:
            for (i = 0; i < REGCNT_PPC; i++) {
                if (fnCmp (pName, NULL, hreg_list_ppc[i].name, FALSE) == 0)
                    break;
            }
            if (i >= REGCNT_PPC) {
                return (HR_notfound);
            }
            EVAL_REG (pv) = hreg_list_ppc[i].index;
            SetNodeType (pv,  hreg_list_ppc[i].type);
            EVAL_IS_HIBYTE (pv) = FALSE;
            break;

        case mptm68k:
            for (i = 0; i < REGCNT_68K; i++) {
                if (fnCmp (pName, NULL, hreg_list_68k[i].name, FALSE) == 0)
                    break;
            }
            if (i >= REGCNT_68K) {
                return (HR_notfound);
            }
            EVAL_REG (pv) = hreg_list_68k[i].index;
            SetNodeType (pv,  hreg_list_68k[i].type);
            EVAL_IS_HIBYTE (pv) = FALSE;
            break;

        default:
            DASSERT(FALSE);
    }

    EVAL_IS_REG (pv) = TRUE;

    EVAL_STATE (pv) = EV_lvalue;
    PushStack (pv);
    return (HR_found);
}


bool_t
LineNumber (
    psearch_t pName
    )
{
    uchar *pb = pName->sstr.lpName + 1;
    uint        i;
    char        c;
    ulong       val = 0;
    //FLS         fls;
    HSF         hsf;
    ADDR        addr;
    SHOFF       cbLine;

    peval_t     pv;

    // convert line number

    for (i = pName->sstr.cb - 1; i > 0; i--) {
        c = *pb;
        if (!_istdigit ((_TUCHAR)c)) {
            // Must have reached the end
            pExState->err_num = ERR_LINENUMBER;
            return (HR_error);
        }
        val *= 10;
        val += (c - '0');
        if (val > 0xffff) {
            // Must have overflowed
            pExState->err_num = ERR_LINENUMBER;
            return (HR_error);
        }
        pb++;
    }

    if ( (hsf = SLHsfFromPcxt ( pCxt ) ) &&
         SLFLineToAddr ( hsf, (WORD) val, &addr, &cbLine, NULL ) ) {

        pv = pName->pv;
        if ( ADDR_IS_OFF32 ( addr ) ) {
            SetNodeType (pv,  T_32PVOID);
        }
        else {
            SetNodeType (pv,  T_PFVOID);
        }

        EVAL_IS_LABEL (pv) = TRUE;
        EVAL_STATE (pv) = EV_rvalue;
        EVAL_SYM (pv) = addr;
        EVAL_PTR (pv) = addr;
        PushStack (pv);
        return (HR_found);
    }
    else {
        pExState->err_num = ERR_NOCODE;
        return (HR_error);
    }

    pExState->err_num = ERR_BADCONTEXT;
    return (HR_error);
}




/**     InsertThis - rewrite bind tree to add this->
 *
 *      fSuccess = InsertThis (pName);
 *
 *      Entry   pName = pointer to search structure
 *
 *      Exit    tree rewritten such that the node pn becomes OP_pointsto
 *              with the left node being OP_this and the right node being
 *              the original symbol
 *
 *      Returns TRUE if tree rewritten
 *              FALSE if error
 */


bool_t FASTCALL
InsertThis (
    psearch_t pName
    )
{
    ulong       len = 2 * (sizeof (node_t) + sizeof (eval_t));
    ulong       Left;
    ulong       Right;
    pnode_t     pn;
    bnode_t     bn = pName->bn;

    Left = pTree->node_next;
    Right = Left + sizeof (node_t) + sizeof (eval_t);
    if ((pTree->size - Left) < len) {
        if (!GrowETree (len)) {
            NOTTESTED (FALSE);
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
        if (bnCxt != 0) {
            // the context was pointing into the expression tree.
            // since the expression tree could have been reallocated,
            // we must recompute the context pointer

           pCxt = SHpCXTFrompCXF ((PCXF)&((pnode_t)bnCxt)->v[0]);
        }
    }

    // copy symbol to right node

    memcpy ((pnode_t)((bnode_t)Right), (pnode_t)bn, sizeof (node_t) + sizeof (eval_t));

    // insert OP_this node as left node

    pn = (pnode_t)((bnode_t)Left);
    memset (pn, 0, sizeof (node_t) + sizeof (eval_t));
    NODE_OP (pn) = OP_this;

    // change original node to OP_pointsto

    pn = (pnode_t)bn;
    memset (pn, 0, sizeof (node_t) + sizeof (eval_t));
    NODE_OP (pn) = OP_pointsto;
    NODE_LCHILD (pn) = (bnode_t)Left;
    NODE_RCHILD (pn) = (bnode_t)Right;
    pTree->node_next += len;
    return (TRUE);
}


/*
 * load a constant symbol into a node.
 */

bool_t
DebLoadConst (
    peval_t pv,
    CONSTPTR pSym,
    HSYM hSym
    )
{
    uint        skip = 0;
    CV_typ_t    type;
    HTYPE       hType;
    plfEnum     pEnum;

    type = pSym->typind;
    if (!CV_IS_PRIMITIVE (type)) {
        // we also allow constants that are enumeration values, so check
        // that the non-primitive type is an enumeration and set the type
        // to the underlying type of the enum

        MHOmfUnLock (hSym);
        hType = THGetTypeFromIndex (EVAL_MOD (pv), type);
        if (hType == 0) {
            DASSERT (FALSE);
            pSym = (CONSTPTR) MHOmfLock (hSym);
            pExState->err_num = ERR_BADOMF;
            return (FALSE);
        }
        pEnum = (plfEnum)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
        if ((pEnum->leaf != LF_ENUM) || !CV_IS_PRIMITIVE (pEnum->utype)) {
            DASSERT (FALSE);
            MHOmfUnLock (hType);
            pSym = (CONSTPTR) MHOmfLock (hSym);
            pExState->err_num = ERR_BADOMF;
            return (FALSE);
        }
        MHOmfUnLock (hType);
        pSym = (CONSTPTR) MHOmfLock (hSym);
        SetNodeType (pv, type);
        // SetNodeType will resolve forward ref Enums - but we need to get the
        // utype from the pv when this happens - sps
        type = ENUM_UTYPE (pv);
    }
    else {
        SetNodeType (pv, type);
    }

    EVAL_STATE (pv) = EV_rvalue;
    if ((CV_MODE (type) != CV_TM_DIRECT) || ((CV_TYPE (type) != CV_SIGNED) &&
      (CV_TYPE (type) != CV_UNSIGNED) && (CV_TYPE (type) != CV_INT))) {
        DASSERT (FALSE);
        pExState->err_num = ERR_BADOMF;
        return (FALSE);
    }
    EVAL_UQUAD (pv) = RNumLeaf (&pSym->value, &skip);
    return (TRUE);
}



/**     fValidInProlog - check if local symbol is valid in the func. prolog
 *
 *      flag = fValidInProlog (hSym, fNoFrame)
 *
 *      Entry   hSym = handle to local symbol
 *
 *      Exit    none
 *
 *      Returns TRUE if hSym is valid
 *              FALSE if hSym is rejectable
 *
 *      A local symbol is rejectable in the prolog if it is a bp-relative
 *      or register item (since the new frame has not been set up yet)
 *      However formal arguments (e.g., bp-relative symbols with +ve
 *      offset) are not rejectable since they are already on the stack.
 *      We let such symbols bind properly during the evaluation phase
 *      the frame passed by the kernel contains a virtual bp value that
 *      points to the right address on the stack.
 *
 *      NOTE: If the frame is indicated as not available then the symbol
 *      is an argument or parameter.  Only labels and the like are
 *      valid without a frame...
 */

bool_t
fValidInProlog (
    HSYM hSym,
    bool_t fNoFrame
    )
{
    SYMPTR  pSym = (SYMPTR) MHOmfLock (hSym);
    bool_t  retval;

    switch (pSym->rectyp) {
        case S_BPREL16:
            retval = !fNoFrame && (((BPRELPTR16)pSym)->off >= 0);
            break;

        case S_BPREL32:
            retval = !fNoFrame && (((BPRELPTR32)pSym)->off >= 0);
            break;

        case S_REGREL32:
            if (TargetMachine == mptmips) {
                // All offsets are SP relative not FP.
                retval = (((LPREGREL32)pSym)->reg != CV_M4_IntSP) &&
                         (((LPREGREL32)pSym)->reg != CV_M4_IntS8);
            } else {
                retval = !fNoFrame && (((LPREGREL32)pSym)->off >= 0);
            }
            break;

        case S_REGISTER:
            if (fNoFrame) return FALSE;

            switch (((REGPTR)pSym)->reg) {
            case CV_REG_ECX:
            case CV_REG_EDX:
                // These registers are used for fastcall arguments
                retval = TRUE;
                break;
            default:
                retval = FALSE;
            }
            break;

        default:
            // non bp-rel, non reg-rel, non-register symbols
            // are always considered bindable during prolog
            retval = TRUE;
    }

    MHOmfUnLock (hSym);
    return retval;
}
