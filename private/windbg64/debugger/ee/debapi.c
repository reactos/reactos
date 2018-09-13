/***    debapi.c - api interface to expression evaluator
 *
 *      API interface between C and C++ expression evaluator and debugger
 *      kernel.
 */

#include "debexpr.h"
#include "version.h"
#include <float.h>

EESTATUS ParseBind (EEHSTR, HTM, PHTM, ulong *, uint, SHFLAG);

PCVF pCVF;                      // callback routine map
PCRF pCRF;

bool_t      Evaluating;         // set not in evaluation phase
bool_t      BindingBP;          // true if binding a breakpoint expression
pexstate_t  pExState;           // address of locked expression state structure
pstree_t    pTree;              // address of locked syntax or evaluation tree
bnode_t     bArgList;           // based pointer to argument list
HDEP        hEStack;            // handle of evaluation stack if not zero
pelem_t     pEStack;            // address of locked evaluation stack
belem_t     StackLen;           // length of evaluation stack buffer
belem_t     StackMax;           // maximum length reached by evaluation stack
belem_t     StackOffset;        // offset in stack to next element
belem_t     StackCkPoint;       // checkpointed evaluation stack offset
peval_t     ST;                 // pointer to evaluation stack top
peval_t     STP;                // pointer to evaluation stack previous
PCXT        pCxt;               // pointer to current cxt for bind
bnode_t     bnCxt;              // based pointer to node containing {...} context
char       *pExStr;             // pointer to expression string
CV_typ_t    ClassExp;           // current explicit class
CV_typ_t    ClassImp;           // implicit class (set if current context is method)
long        ClassThisAdjust;    // cmplicit class this adjustor
char       *vfuncptr = "\x07""__vfptr";
char       *vbaseptr = "\x07""__vbptr";
HTM        *pTMList;            // pointer to breakpoint return list
PTML        pTMLbp;             // global pointer to TML for breakpoint
HDEP        hBPatch;            // handle to back patch data during BP bind
ulong       iBPatch;            // index into pTMLbp for backpatch data
bool_t      GettingChild;       // true if in EEGetChildTM
BOOL        fAutoClassCast;
uint        nEEReEntry = 0;     // Protect reentry to the EE.

MPT         TargetMachine;      // Init at ee load time.  Should be moved to the TMs when
                                // cross architecture EE's are needed.

// global handle to the CXT list buffer

HCXTL       hCxtl;              // handle for context list buffer during GetCXTL
PCXTL       pCxtl;              // pointer to context list buffer during GetCXTL
ulong       mCxtl;              // maximum number of elements in context list
PCXT        pBindBPCxt;         // pointer to Bp Binding context

CRITICAL_SECTION    csEE;       // maybe more useful than nEEReEntry

#ifdef DEBUGVER
DEBUG_VERSION('E','E',"Expression Evaluator")
#else
RELEASE_VERSION('E','E',"Expression Evaluator")
#endif

DBGVERSIONCHECK()

static char const * accessstr[2][4] = {
    {" ", "private ", "protected ", " " /* public */ },
    {" ", "PRV ",     "PRO ",       " " /* public */ }
};

MPT EESetTarget(MPT mpt);
void EEUnload();

static EXF EEFunctions =
{
    EEFreeStr,
    EEGetError,
    EEParse,
    EEBindTM,
    EEvaluateTM,
    EEGetExprFromTM,
    EEGetValueFromTM,
    EEGetNameFromTM,
    EEGetTypeFromTM,
    EEFormatCXTFromPCXT,
    EEFreeTM,
    EEParseBP,
    EEFreeTML,
    EEInfoFromTM,
    EEFreeTI,
    EEGetCXTLFromTM,
    EEFreeCXTL,
    EEAssignTMToTM,
    EEIsExpandable,
    EEAreTypesEqual,
    EEcChildrenTM,
    EEGetChildTM,
    EEDereferenceTM,
    EEcParamTM,
    EEGetParmTM,
    EEGetTMFromHSYM,
    EEFormatAddress,
    EEGetHSYMList,
    EEFreeHSYMList,
    EEGetExtendedTypeInfo,
    EEGetAccessFromTM,
    EEEnableAutoClassCast,
    EEInvalidateCache,
    EEcSynthChildTM,
    EEGetBCIA,
    EEFreeBCIA,
	NULL,
	NULL,
	NULL,
	EESetTarget,
	EEUnload
};

void
EEInitializeExpr (
    PCI pCVInfo,
    PEI pExprinfo
    )
{
    // assign the callback routines

    pCVF = pCVInfo->pStructCVAPI;
    pCRF = pCVInfo->pStructCRuntime;
    TargetMachine = __dbgver_cpu__;     // Init to a known state for this platform (defined in dbgver.h)

    pExprinfo->Version      = 1;
    pExprinfo->pStructExprAPI   = &EEFunctions;
    pExprinfo->Language     = 0;
    pExprinfo->IdCharacters     = "_$";
    pExprinfo->EETitle = "CPP";
    pExprinfo->EESuffixes = ".cpp.cxx.c.hpp.hxx.h";
    pExprinfo->Assign = "\x001""=";

    InitializeCriticalSection(&csEE);

    ResetTickCounter();
    // by default disable auto class cast feature
    fAutoClassCast = FALSE;
}


MPT
EESetTarget(
	MPT	mpt
	)
/*++

Routine Description:

	Set the TargetMachine global variable.

Comments:

	 Note: this is necessary to support Mac debugging.  After the EE and the
	 EM has been loaded, the shell will tell the EE what the TargetMachine
	 type is.

--*/
{
	MPT		mptT;

	mptT = TargetMachine;
	TargetMachine = mpt;
	return mptT;
}

void
EEUnload(
	)
/*++

Routine Description:

	Tells the EE it is about to be unloaded (new for V5)

Comments:

	 Note: this is necessary for components that need to free COM objects
	 etc i.e. not us

--*/
{
    extern HDEP hSRStack;

	if (hSRStack)
	{
		MemFree (hSRStack);
		hSRStack = NULL;
	}

	if (hEStack)
	{
		MemFree (hEStack);
		hEStack = NULL;
	}

    DeleteCriticalSection(&csEE);
}

/**     EEAreTypesEqual - are TM types equal
 *
 *      flag = EEAreTypesEqual (phTMLeft, phTMRight);
 *
 *      Entry   phTMLeft = pointer to handle of left TM
 *              phTMRight = pointer to handle of right TM
 *
 *      Exit    none
 *
 *      Returns TRUE if TMs have identical types
 */

SHFLAG
EEAreTypesEqual (
    PHTM phTMLeft,
    PHTM phTMRight
    )
{
    return (AreTypesEqual (*phTMLeft, *phTMRight));
}


/**     EEAssignTMToTM - assign TMRight to TMLeft
 *
 *      No longer used
 *
 *      Exit    none
 *
 *      Returns EECATASTROPHIC
 */

EESTATUS
EEAssignTMToTM (
    PHTM phTMLeft,
    PHTM phTMRight
    )
{
    Unreferenced(phTMLeft);
    Unreferenced(phTMRight);

    return(EECATASTROPHIC);
}


/**     EEBindTM - bind syntax tree to a context
 *
 *      ulong  EEBindTM (phExState, pCXT, fForceBind, fEnableProlog);
 *
 *      Entry   phExState = pointer to expression state structure
 *              pCXT = pointer to context packet
 *              fForceBind = TRUE if rebind required.
 *              fForceBind = FALSE if rebind decision left to expression evaluator.
 *              fEnableProlog = FALSE if function scoped symbols only after prolog
 *              fEnableProlog = TRUE if function scoped symbols during prolog
 *
 *      Exit    tree rebound if necessary
 *
 *      Returns EENOERROR if no error in bind
 *              EECATASTROPHIC if null TM pointer
 *              EENOMEMORY if unable to get memory for buffers
 *              EEGENERAL if error (pExState->err_num is error)
 */

EESTATUS
EEBindTM (
    PHTM phTM,
    PCXT pcxt,
    SHFLAG fForceBind,
    SHFLAG fEnableProlog
    )
{
    uint    flags = 0;

    EESTATUS status;

    // bind without suppressing overloaded operators
    if (fForceBind == TRUE) {
        flags |= BIND_fForceBind;
    }
    if (fEnableProlog == TRUE) {
        flags |= BIND_fEnableProlog;
    }
    EnterCriticalSection(&csEE);
    status = (DoBind (phTM, pcxt, flags));
    LeaveCriticalSection(&csEE);
    return status;
}




/**     EEcChildren - return number of children for the TM
 *
 *      void EEcChildrenTM (phTM, pcChildren)
 *
 *      Entry   phTM = pointer to handle of TM
 *              pcChildren = pointer to location to store count
 *
 *      Exit    *pcChildren = number of children for TM
 *
 *      Returns EENOERROR if no error
 *              non-zero if error
 */

EESTATUS
EEcChildrenTM (
    PHTM phTM,
    long *pcChildren,
    PSHFLAG pVar
    )
{
    EESTATUS status;
    EnterCriticalSection(&csEE);
    status = (cChildrenTM (phTM, pcChildren, pVar));
    LeaveCriticalSection(&csEE);
    return status;
}


/**     EEcParamTM - return count of parameters for TM
 *
 *      ulong  EEcParamTM (phTM, pcParam, pVarArg)
 *
 *      Entry   phTM = pointer to TM
 *              pcParam = pointer return count
 *              pVarArg = pointer to vararg flags
 *
 *      Exit    *pcParam = count of number of parameters
 *              *pVarArgs = TRUE if function has varargs
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EENOERROR if no error
 */

EESTATUS
EEcParamTM (
    PHTM phTM,
    ulong  *pcParam,
    PSHFLAG pVarArg
    )
{
    EESTATUS status;
    EnterCriticalSection(&csEE);
    status = (cParamTM (*phTM, pcParam, pVarArg));
    LeaveCriticalSection(&csEE);
    return status;
}




/**     EEDereferenceTM - generate TM from pointer to data TM
 *
 *      ulong  EEDereferenceTM (phTMIn, phTMOut, pEnd)
 *
 *      Entry   phTMIn = pointer to handle to TM to dereference
 *              phTMOut = pointer to handle to dereferencing TM
 *              pEnd = pointer to int to receive index of char that ended parse
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMOut = TM referencing pointer data
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EEGENERAL if TM not dereferencable
 *              EENOERROR if TM generated
 */


EESTATUS
EEDereferenceTM (
    PHTM phTMIn,
    PHTM phTMOut,
    ulong  *pEnd,
    SHFLAG fCase
    )
{
    register   EESTATUS    retval;
    EEHSTR      hDStr = 0;
    EEHSTR      hDClassName = 0;

    EnterCriticalSection(&csEE);
    if ((retval = DereferenceTM (*phTMIn, &hDStr, &hDClassName)) == EENOERROR) {

        // bind with prolog disabled and with operloaded operators suppressed

        if ((retval = ParseBind (hDStr, *phTMIn, phTMOut, pEnd,
          BIND_fSupOvlOps, fCase)) == EENOERROR) {
            // the result of dereferencing a pointer does not have a name
            pExState = (pexstate_t)MemLock (*phTMOut);
            pExState->state.noname = TRUE;
            pExState->state.childtm = TRUE;
            pExState->hDClassName = hDClassName;
            pExState->seTemplate = SE_deref;
            MemUnLock (*phTMOut);

            LinkWithParentTM (*phTMOut, *phTMIn);

            if (hDClassName) {
                // a derived class name has been found and the
                // auto class cast feature is enabled.
                // child 0 of phTMOut should be the downcast node
                // verify that it is possible to bind this node
                // (being able to bind this node means that the
                // cast to the derived class ptr is legal)
                // otherwise clear hDClassName in phTMOut so that
                // the downcast node be suppressed.
                HTM hTM;
                ulong   end;
                DASSERT (fAutoClassCast);

                // "preview" child 0 of the dereferenced TM
                if (EEGetChildTM (phTMOut, 0, &hTM, &end, 0, fCase) == EENOERROR) {
                    // OK, free temporary TM
                    EEFreeTM(&hTM);
                } else {
                    // can't bind child, remove hDClassName
                    MemFree(hDClassName);
                    pExState = (pexstate_t)MemLock(*phTMOut);
                    pExState->hDClassName = 0;
                    MemUnLock(*phTMOut);
                }
            }
        }
    }
    LeaveCriticalSection(&csEE);
    return (retval);
}


/**     EEvaluateTM - evaluate syntax tree
 *
 *      ulong  EEvaluateTM (phExState, pFrame, style);
 *
 *      Entry   phExState = pointer to expression state structure
 *              pFrame = pointer to context frame
 *              style = EEHORIZONTAL for horizontal (command) display
 *                      EEVERTICAL for vertical (watch) display
 *                      EEBPADDRESS for bp address (suppresses fcn evaluation)
 *
 *      Exit    abstract syntax tree evaluated with the saved
 *              context and current frame and the result node stored
 *              in the expression state structure
 *
 *      Returns EENOERROR if no error in evaluation
 *              error number if error
 */

EESTATUS
EEvaluateTM (
    PHTM phTM,
    HFRAME pFrame,
    EEDSP style
    )
{
    EESTATUS retval;

    // This call may involve floating point operations. Since the kernel
    // does not register a SIGFPE handler under windows we need to mask
    // all floating point exceptions

    EnterCriticalSection(&csEE);
    retval = DoEval (phTM, pFrame, style);
    LeaveCriticalSection(&csEE);
    return retval;
}




/**     EEFormatAddress - format address as an ASCII string
 *
 *      EEFormatAddress (seg, offset, pbuf)
 *
 *      Entry   Seg = segment portion of address
 *              Off = offset portion of address
 *              pbuf = pointer to buffer for formatted address
 *                     (must be 20 bytes long)
 *
 *      Exit    buf = formatted address
 *
 *      Returns none
 */

EESTATUS
EEFormatAddress (
    PADDR paddr,
    char *szAddr,
    DWORD cch,
    SHFLAG flags
    )
{
    char    buf[20];

    char    chx = (flags & EEFMT_REAL) ? '#' : ':';
#ifdef NT_BUILD_ONLY
    if (ADDR_IS_FLAT(*paddr)) {
        flags |= EEFMT_32;
    }
#endif

    if (flags & EEFMT_32) {
        if (flags & EEFMT_SEG) {
            if (flags & EEFMT_LOWER) {
                sprintf (buf, "0x%04x%c0x%08x", GetAddrSeg(*paddr), chx, GetAddrOff(*paddr));
            } else {
                sprintf (buf, "0x%04X%c0x%08X", GetAddrSeg(*paddr), chx, GetAddrOff(*paddr));
            }
        } else {
            if (TargetMachine == mptia64) {
				if (flags & EEFMT_LOWER) {
					sprintf (buf, "0x%016I64x", GetAddrOff(*paddr));
				} else {
					sprintf (buf, "0x%016I64X", GetAddrOff(*paddr));
				}
			} else {
				if (flags & EEFMT_LOWER) {
					sprintf (buf, "0x%08x", GetAddrOff(*paddr));
				} else {
					sprintf (buf, "0x%08X", GetAddrOff(*paddr));
				}
			}
        }
    } else {
        sprintf (buf, "0x%04X%c0x%04X", GetAddrSeg(*paddr), chx, GetAddrOff(*paddr));
    }

    /*
     * copy the zero terminated string from the buffer to the buffer
     */

    if (_tcslen(buf)+1 >= cch) {
        return EEGENERAL;
    }

    _tcscpy (szAddr, buf);
    return EENOERROR;
}


/**     EEFormatCXTFromPCXT - format a context operator from a PCXT
 *
 *      ulong  EEFormatCXTFromPCXT (pCXT, phStr)
 *
 *      Entry   pCXT = pointer to CXT packet
 *              phStr = pointer for handle for formatted string buffer
 *
 *      Exit    *phStr = handle of formatted string buffer
 *              *phStr = 0 if buffer not allocated
 *
 *      Returns EENOERROR if no error
 *              error code if error
 */

EESTATUS
EEFormatCXTFromPCXT (
    PCXT pCXT,
    PEEHSTR phStr,
    DWORD dwFormatFlags
    )
{
    register ulong       retval = EECATASTROPHIC;

    EnterCriticalSection(&csEE);

    DASSERT (pCXT);
    pCxt = pCXT;

    //
    // We initialize the global variables pCxt, pExState and hEStack in
    // for NB10 restart with breakpoints.  (Resolving forward referenced UDTs
    // may cause us to invoke SearchSym which needs them.) We also init
    // the various Eval Stack variables
    //
    if (pCXT) {
        HTM hTM;
        if ((hTM = MemAllocate (sizeof (struct exstate_t))) == 0) {
            LeaveCriticalSection(&csEE);
            return (EECATASTROPHIC);
        }

        // lock expression state structure, clear and allocate components

        pExState = (pexstate_t)MemLock (hTM);
        memset (pExState, 0, sizeof (exstate_t));
        pExState->state.fCase = TRUE;

        if (hEStack == 0) {
            if ((hEStack = MemAllocate (ESTACK_DEFAULT * sizeof (elem_t))) == 0) {
                pExState = NULL;
                MemUnLock (hTM);
                EEFreeTM (&hTM);
                LeaveCriticalSection(&csEE);
                return (EECATASTROPHIC);
            }

            // clear the stack top, stack top previous, function argument
            // list pointer and based pointer to operand node

            StackLen = (belem_t)((uint)ESTACK_DEFAULT * sizeof (elem_t));
            StackOffset = 0;
            StackCkPoint = 0;
            StackMax = 0;
            pEStack = (pelem_t) MemLock (hEStack);
            memset (pEStack, 0, (size_t)(UINT_PTR)StackLen);

            ST = 0;
            STP = 0;
        } else {
            pEStack = (pelem_t) MemLock (hEStack);
        }

        retval = FormatCXT (pCXT, phStr, dwFormatFlags);

        pExState = NULL;
        MemUnLock (hTM);
        EEFreeTM (&hTM);
        MemUnLock (hEStack);
    }
    LeaveCriticalSection(&csEE);
    return (retval);
}




/**     EEFreeCXTL - Free  CXT list
 *
 *      EEFreeCXTL (phCXTL)
 *
 *      Entry   phCXTL = pointer to the CXT list handle
 *
 *      Exit    *phCXTL = 0;
 *
 *      Returns none
 */


void
EEFreeCXTL (
    PHCXTL phCXTL
    )
{
    DASSERT (phCXTL != NULL);
    if (*phCXTL != 0) {
        MemFree (*phCXTL);
        *phCXTL = 0;
    }
}




/**     EEFreeHSYMList - Free HSYM list
 *
 *      EEFreeCXTL (phSYML)
 *
 *      Entry   phSYML = pointer to the HSYM list handle
 *
 *      Exit    *phSYML = 0;
 *
 *      Returns none
 */


void
EEFreeHSYMList (
    HDEP *phSYML
    )
{
    PHSL_HEAD  pList;

    DASSERT (phSYML != NULL);
    if (*phSYML != 0) {
        // lock the buffer and free the restart buffer if necessary

        pList = (PHSL_HEAD) MemLock (*phSYML);
        if (pList->restart != 0) {
            MemFree (pList->restart);
        }
        MemUnLock (*phSYML);
        MemFree (*phSYML);
        *phSYML = 0;
    }
}


/**     EEFreeStr - free string buffer memory
 *
 *      ulong  EEFreeStr (hszStr);
 *
 *      Entry   hszExpr = handle to string buffer
 *
 *      Exit    string buffer freed
 *
 *      Returns none
 */

void
EEFreeStr (
    EEHSTR hszStr
    )
{
    if (hszStr != 0) {
        MemFree (hszStr);
    }
}




/**     EEFreeTM - free expression state structure
 *
 *      void EEFreeTM (phTM);
 *
 *      Entry   phTM = pointer to the handle for the expression state structure
 *
 *      Exit    expression state structure freed and the handle cleared
 *
 *      Returns none.
 */


void
EEFreeTM (
    PHTM phTM
    )
{
    pexstate_t  pTM;

    if (*phTM != 0) {
        // DASSERT (!IsMemLocked (*phTM));

        //  lock the expression state structure and free the components
        //  every component must have no locks active

        pTM = (pexstate_t) MemLock (*phTM);

        // free TM only if it is not referenced by another TM
        if (pTM->nRefCount != 0) {
            (pTM->nRefCount)--;
            MemUnLock (*phTM);
        } else {
            // free any TMs that are referenced by this TM
            // unless this TM is an auto-expanded child TM
            // in that case do not free the parent, to
            // avoid recursive loop.
            if (pTM->hParentTM && !pTM->state.autoexpand) {
                EEFreeTM (&pTM->hParentTM);
            }

            // Free auto-expanded child-tm list
            EEFreeTML (&pTM->TMLAutoExpand);

            if (pTM->hExStr != 0) {
                // DASSERT (!IsMemLocked (pTM->hExStr));
                MemFree (pTM->hExStr);
            }
            if (pTM->hErrStr != 0) {
                // DASSERT (!IsMemLocked (pTM->hErrStr));
                MemFree (pTM->hErrStr);
            }
            if (pTM->hCName != 0) {
                // DASSERT (!IsMemLocked (pTM->hCName));
                MemFree (pTM->hCName);
            }
            if (pTM->hExStrSav != 0) {
                // DASSERT (!IsMemLocked (pTM->hExStrSav));
                MemFree (pTM->hExStrSav);
            }
            if (pTM->hDClassName != 0) {
                // DASSERT (!IsMemLocked (pTM->hDClassName));
                MemFree (pTM->hDClassName);
            }
            if (pTM->hAutoExpandRule != 0) {
                // DASSERT (!IsMemLocked (pTM->hAutoExpandRule));
                MemFree (pTM->hAutoExpandRule);
            }
            if (pTM->hSTree != 0) {
                // DASSERT (!IsMemLocked (pTM->hSTree));
                MemFree (pTM->hSTree);
            }
            if (pTM->hETree != 0) {
            //  DASSERT (!IsMemLocked (pTM->hETree));
                MemFree (pTM->hETree);
            }
            MemUnLock (*phTM);
            MemFree (*phTM);
        }
        *phTM = 0;
    }
}


/**     EEFreeTI - free TM Info buffer
 *
 *      void EEFreeTI (hTI);
 *
 *      Entry   hTI = handle to TI Info buffer
 *
 *      Exit    TI Info buffer freed
 *
 *      Returns none
 */

void
EEFreeTI (
    PHTI phTI
    )
{
    if (*phTI != 0) {
        MemFree (*phTI);
        *phTI = 0;
    }
}




/**     EEFreeTML - free TM list
 *
 *      void EEFreeTML (phTML);
 *
 *      Entry   phTML = pointer to the handle for the TM list
 *
 *      Exit    TM list freed and the handle cleared
 *
 *      Returns none.
 */


void
EEFreeTML (
    PTML pTML
    )
{
    uint        i;
    ulong       cTM = 0;
    HTM FAR    *rgHTM;

    DASSERT (pTML);
    if (pTML->hTMList != NULL) {
        // Use rgHTM instead of the global pTMList
        // in order to allow recursive calls of this routine
        rgHTM = (HTM *)MemLock (pTML->hTMList);
        for (i = 0; i < pTML->cTMListMax; i++) {
            if (rgHTM[i] != 0) {
                EEFreeTM (&rgHTM[i]);
                cTM++;
            }
        }
//
//      DASSERT (cTM == pTML->cTMListAct);
        MemUnLock (pTML->hTMList);
        MemFree (pTML->hTMList);
        pTML->hTMList = 0;
    }
    ZeroMemory(pTML, sizeof(TML));
}




/**     EEGetChildTM - get TM representing ith child
 *
 *      status = EEGetChildTM (phTMParent, iChild, phTMChild)
 *
 *      Entry   phTMParent = pointer to handle of parent TM
 *              iChild = child to get TM for
 *              phTMChild = pointer to handle for returned child
 *              pEnd = pointer to int to receive index of char that ended parse
 *              eeRadix = display radix (override current if != NULL )
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMChild = handle of child TM if allocated
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if no error
 *              non-zero if error
 */


EESTATUS
EEGetChildTM (
    PHTM phTMIn,
    long iChild,
    PHTM phTMOut,
    ulong  *pEnd,
    EERADIX eeRadix,
    SHFLAG fCase
    )
{
    register    EESTATUS    retval;
    pexstate_t  pExStateIn;
    char *  pErrStr;
    char *  pErrStrIn;

    EnterCriticalSection(&csEE);

    DASSERT(*phTMIn != 0);
    *phTMOut = (HTM)0;

    pExStateIn = (pexstate_t) MemLock(*phTMIn);
    if ((retval = GetChildTM (*phTMIn, iChild, phTMOut, pEnd, eeRadix, fCase)) != EENOERROR){
        if (*phTMOut != 0) {
            pExState = (pexstate_t) MemLock (*phTMOut);
            pExStateIn->err_num = pExState->err_num;

            // [cuda#5577 7/6/93 mikemo]
            // When copying err_num, we also need copy hErrStr.

            if (pExStateIn->hErrStr != 0) { // old error string?
                // Free pExStateIn's old error string, if any
                MemFree(pExStateIn->hErrStr);
            }

            if (pExState->hErrStr != 0) {   // new error string?
                // Make another copy of hErrStr string

                pErrStr = (char*) MemLock(pExState->hErrStr);
                pExStateIn->hErrStr = MemAllocate(_tcslen(pErrStr)+1);
                if (pExStateIn->hErrStr) {  // alloc succeeded?
                    pErrStrIn = (char *) MemLock(pExStateIn->hErrStr);
                    _tcscpy(pErrStrIn, pErrStr);
                    MemUnLock(pExStateIn->hErrStr);
                } else {
                    // there's no memory to copy symbol name, so change
                    // the error code to "out of memory"
                    pExStateIn->err_num = ERR_NOMEMORY;
                    retval = EENOMEMORY;
                }
            } else {
                // hErrStr is 0, so we don't need to copy a string
                pExStateIn->hErrStr = 0;
            }

            MemUnLock (*phTMOut);
            EEFreeTM(phTMOut);
        }
    }
    MemUnLock(*phTMIn);
    LeaveCriticalSection(&csEE);
    return (retval);
}




/**     EEGetCXTLFromTM - Gets a list of symbols and contexts for expression
 *
 *      status = EEGetCXTLFromTM (phTM, phCXTL)
 *
 *      Entry   phTM = pointer to handle to expression state structure
 *              phCXTL = pointer to handle for CXT list buffer
 *
 *      Exit    *phCXTL = handle for CXT list buffer
 *
 *      Returns EENOERROR if no error
 *              status code if error
 */


EESTATUS
EEGetCXTLFromTM (
    PHTM phTM,
    PHCXTL phCXTL
    )
{
    EESTATUS status;
    EnterCriticalSection(&csEE);
    status = (DoGetCXTL (phTM, phCXTL));
    LeaveCriticalSection(&csEE);
    return status;
}




EESTATUS
EEGetError (
    PHTM phTM,
    EESTATUS Status,
    PEEHSTR phError
    )
{
    EESTATUS status;
    EnterCriticalSection(&csEE);
    status = (GetErrorText (phTM, Status, phError));
    LeaveCriticalSection(&csEE);
    return status;
}


/**     EEGetExprFromTM - get expression representing TM
 *
 *      void EEGetExprFromTM (phTM, radix, phStr, pEnd)
 *
 *      Entry   phTM = pointer to handle of TM
 *              radix = radix to use for formatting
 *              phStr = pointer to handle for returned string
 *              pEnd = pointer to int to receive index of char that ended parse
 *
 *      Exit    *phStr = handle of formatted string if allocated
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if no error
 *              non-zero if error
 */

EESTATUS
EEGetExprFromTM (
    PHTM phTM,
    PEERADIX pRadix,
    PEEHSTR phStr,
    ulong  *pEnd
    )
{
    register EESTATUS    retval = EECATASTROPHIC;

    EnterCriticalSection(&csEE);

    DASSERT (*phTM != 0);
    if (*phTM != 0) {
        pExState = (pexstate_t) MemLock (*phTM);
        if (pExState->state.bind_ok == TRUE) {
            *pRadix = pExState->radix;
            retval = GetExpr (pExState->radix, phStr, pEnd);
        }
        MemUnLock (*phTM);
    }
    LeaveCriticalSection(&csEE);
    return (retval);
}


/**     EEGetHSYMList - Get a list of handle to symbols
 *
 *      status = EEGetHSYMList (phSYML, pCXT, mask, pRE, fEnableProlog)
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
EEGetHSYMList (
    HDEP *phSYML,
    PCXT pCxt,
    ulong  mask,
    uchar * pRE,
    SHFLAG fEnableProlog
    )
{
    EESTATUS status;
    EnterCriticalSection(&csEE);
    status = (GetHSYMList (phSYML, pCxt, mask, pRE, fEnableProlog));
    LeaveCriticalSection(&csEE);
    return status;
}





/**     EEGetNameFromTM - get name from TM
 *
 *      ulong  EEGetNameFromTM (phExState, phszName);
 *
 *      Entry   phExState = pointer to expression state structure
 *              phszName = pointer to handle for string buffer
 *
 *      Exit    phszName = handle for string buffer
 *
 *      Returns 0 if no error in evaluation
 *              error number if error
 */


EESTATUS
EEGetNameFromTM (
    PHTM phTM,
    PEEHSTR phszName
    )
{
    EESTATUS status;
    EnterCriticalSection(&csEE);
    status = (GetSymName (phTM, phszName));
    LeaveCriticalSection(&csEE);
    return status;
}




/**     EEGetParamTM - get TM representing ith parameter
 *
 *      status = EEGetParamTM (phTMParent, iChild, phTMChild)
 *
 *      Entry   phTMparent = pointer to handle of parent TM
 *              iParam = parameter to get TM for
 *              phTMParam = pointer to handle for returned parameter
 *              pEnd = pointer to int to receive index of char that ended parse
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMParam = handle of child TM if allocated
 *              *pEnd = index of character that terminated parse
 *
 *
 *      Returns EENOERROR if no error
 *              non-zero if error
 */


EESTATUS
EEGetParmTM (
    PHTM phTMIn,
    ulong  iParam,
    PHTM phTMOut,
    ulong  *pEnd,
    SHFLAG fCase
    )
{
    register    EESTATUS    retval;
    EEHSTR      hDStr = 0;
    EEHSTR      hName = 0;
    EnterCriticalSection(&csEE);

    if ((retval = GetParmTM (*phTMIn, iParam, &hDStr, &hName)) == EENOERROR) {
        DASSERT (hDStr != 0);
        if ((retval = ParseBind (hDStr, *phTMIn, phTMOut, pEnd,
            BIND_fSupOvlOps | BIND_fSupBase | BIND_fEnableProlog, fCase)) == EENOERROR) {
            // the result of dereferencing a pointer does not have a name
            pExState = (pexstate_t) MemLock (*phTMOut);
            pExState->state.childtm = TRUE;
            if ((pExState->hCName = hName) == 0) {
                pExState->state.noname = TRUE;
            }
            MemUnLock (*phTMOut);
        }
    } else {
        if (hName != 0) {
            MemFree (hName);
        }
    }
    LeaveCriticalSection(&csEE);
    return (retval);
}




/**     EEGetTMFromHSYM - create bound TM from handle to symbol
 *
 *      EESTATUS EEGetTMFromHSYM (hSym, pCxt, phTM, pEnd, fForceBind, fEnableProlog);
 *
 *      Entry   hSym = symbol handle
 *              pcxt = pointer to context
 *              phTM = pointer to the handle for the expression state structure
 *              pEnd = pointer to int to receive index of char that ended parse
 *              fEnableProlog = FALSE if function scoped symbols only after prolog
 *              fEnableProlog = TRUE if function scoped symbols during prolog
 *
 *      Exit    bound TM created
 *              *phTM = handle of TM buffer
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EENOMEMORY if out of memory
 *               0 if no error
 */


EESTATUS
EEGetTMFromHSYM (
    HSYM hSym,
    PCXT pcxt,
    PHTM phTM,
    ulong  *pEnd,
    SHFLAG fForceBind,
    SHFLAG fEnableProlog
    )
{
    register EESTATUS    retval;

    // allocate, lock and clear expression state structure

    DASSERT (hSym != 0);
    if (hSym == 0) {
        return (EECATASTROPHIC);
    }

    if ((*phTM = MemAllocate (sizeof (struct exstate_t))) == 0) {
        return (EENOMEMORY);
    }

    EnterCriticalSection(&csEE);

    // lock expression state structure, clear and allocate components

    pExState = (pexstate_t)MemLock (*phTM);
    memset (pExState, 0, sizeof (exstate_t));

    // allocate buffer for input string and copy

    pExState->ExLen = sizeof (char) + HSYM_CODE_LEN;
    if ((pExState->hExStr = MemAllocate ((uint) pExState->ExLen + 1)) == 0) {
        // clean up after error in allocation of input string buffer
        MemUnLock (*phTM);
        EEFreeTM (phTM);
        pExState = NULL;
        LeaveCriticalSection(&csEE);
        return (EENOMEMORY);
    }
    pExStr = (char *) MemLock (pExState->hExStr);
    *pExStr++ = HSYM_MARKER;
    _tcscpy (pExStr, GetHSYMCodeFromHSYM(hSym));
    MemUnLock (pExState->hExStr);
    MemUnLock (*phTM);
    pExState = NULL;
    if ((retval = DoParse (phTM, 10, TRUE, FALSE, pEnd)) == EENOERROR) {
        retval = EEBindTM (phTM, pcxt, fForceBind, fEnableProlog);
    }
    LeaveCriticalSection(&csEE);
    return (retval);
}




/**     EEGetTypeFromTM - get type name from TM
 *
 *      ulong  EEGetTypeFromTM (phExState, hszName, phszType, select);
 *
 *      Entry   phExState = pointer to expression state structure
 *              hszName = handle to name to insert into string if non-null
 *              phszType = pointer to handle for type string buffer
 *              select = selection mask
 *
 *      Exit    phszType = handle for type string buffer
 *
 *      Returns EENOERROR if no error in evaluation
 *              error number if error
 */


EESTATUS
EEGetTypeFromTM (
    PHTM phTM,
    EEHSTR hszName,
    PEEHSTR phszType,
    ulong select
    )
{
    char       *buf;
    uint        buflen = TYPESTRMAX - 1 + sizeof (HDR_TYPE);
    char       *pName;
    PHDR_TYPE   pHdr;

    DASSERT (*phTM != 0);
    if (*phTM == 0) {
        return (EECATASTROPHIC);
    }
    if ((*phszType = MemAllocate (TYPESTRMAX + sizeof (HDR_TYPE))) == 0) {
        // unable to allocate memory for type string
        return (EECATASTROPHIC);
    }
    EnterCriticalSection(&csEE);
    buf = (char *)MemLock (*phszType);
    memset (buf, 0, TYPESTRMAX + sizeof (HDR_TYPE));
    pHdr = (PHDR_TYPE)buf;
    buf += sizeof (HDR_TYPE);
    pExState = (pexstate_t) MemLock (*phTM);
    pCxt = &pExState->cxt;
    bnCxt = 0;
    if (hszName != 0) {
        pName = (char *) MemLock (hszName);
    } else {
        pName = NULL;
    }
    FormatType (&pExState->result, &buf, &buflen, &pName, select, pHdr);
    if (hszName != 0) {
        MemUnLock (hszName);
    }
    MemUnLock (*phTM);
    MemUnLock (*phszType);
    pExState = NULL;
    LeaveCriticalSection(&csEE);
    return (EENOERROR);
}




/**     EEGetValueFromTM - format result of evaluation
 *
 *      ulong  EEGetValueFromTM (phTM, radix, pFormat, phValue);
 *
 *      Entry   phTM = pointer to handle to TM
 *              radix = default radix for formatting
 *              pFormat = pointer to format string
 *              phValue = pointer to handle for display string
 *
 *      Exit    evaluation result formatted
 *
 *      Returns EENOERROR if no error in formatting
 *              error number if error
 */

EESTATUS
EEGetValueFromTM (
    PHTM phTM,
    uint Radix,
    PEEFORMAT pFormat,
    PEEHSTR phszValue
    )
{
    EESTATUS retval;

    // This call may involve floating point operations. Since the kernel
    // does not register a SIGFPE handler under windows we need to mask
    // all floating point exceptions

    EnterCriticalSection(&csEE);
    retval = FormatNode (phTM, Radix, pFormat, phszValue, TRUE);
    LeaveCriticalSection(&csEE);
    return retval;
}


/**     EEInfoFromTM - return information about TM
 *
 *      EESTATUS EEInfoFromTM (phTM, pReqInfo, phTMInfo);
 *
 *      Entry   phTM = pointer to the handle for the expression state structure
 *              reqInfo = info request structure
 *              phTMInfo = pointer to handle for request info data structure
 *
 *      Exit    *phTMInfo = handle of info structure
 *
 *      Returns EECATASTROPHIC if fatal error
 *               0 if no error
 */


EESTATUS
EEInfoFromTM (
    PHTM phTM,
    PRI pReqInfo,
    PHTI phTMInfo
    )
{
    EESTATUS retval;
    EnterCriticalSection(&csEE);
    retval =(InfoFromTM (phTM, pReqInfo, phTMInfo));
    LeaveCriticalSection(&csEE);
    return retval;
}

/**     EEGetAccessFromTM - return string indicating private/protected/public
 *
 *
 *
 *      Entry   phTM = pointer to the handle for the expression state structure
 *              format = specifier for format of returned string:
 *               0 for full text ("   ", "private ", "protected ", " ")
 *               1 for abbreviated text (" ", "PRV ", "PRO ", " ")
 *
 *      Exit    *phszAccess = one of the strings listed above
 *
 *      Returns EENOMEMORY if allocation failed, 0 if no error
 *
 *
 *
 *
 */

EESTATUS
EEGetAccessFromTM (
    PHTM phTM,
    PEEHSTR phszAccess,
    ulong format
    )
{
    char const *szAccess = " "; // default return is " " if no access attribute
    char       *buf;
    EESTATUS    retval = EENOERROR;

    DASSERT (*phTM != 0);

    EnterCriticalSection(&csEE);

    // Set szAccess to access string: already set to " " above for C or
    // unknown attribute, will be set to something else for C++

    if (*phTM != 0) {
        uint        fmt;
        pExState = (pexstate_t) MemLock (*phTM);

        // if this is not a bound expression, don't bother
        // DASSERT ( pExState->state.bind_ok );
        if ( pExState->state.bind_ok ) {

            fmt = format ? 1 : 0;

            // Set szAccess to the string that's going to be returned

            if (EVAL_ACCESS (&pExState->result) != 0) {
                szAccess = accessstr[fmt][EVAL_ACCESS (&pExState->result)];
            }

        }
        MemUnLock (*phTM);
    }

    // Allocate space for it

    if ( (*phszAccess = MemAllocate (_tcslen(szAccess) + 1)) != 0 ) {

        buf = (char *) MemLock (*phszAccess);
        _tcscpy (buf, szAccess);
        MemUnLock (*phszAccess);
    } else {

        // allocation failed
        retval = EENOMEMORY;
    }

    LeaveCriticalSection(&csEE);
    return retval;
}


/**     EEGetExtendedTypeInfo - determine general category of an expr's type
 *
 *
 *
 *      Entry   phTM = pointer to the handle for the expression state structure
 *
 *      Exit    *pETI = an ETI to indicate the type of the TM:
 *              ETIPRIMITIVE, ETICLASS, ETIARRAY, ETIPOINTER, ETIFUNCTION
 *
 *      Returns EECATASTROPHIC or 0
 *
 *
 *
 *
 */
EESTATUS
EEGetExtendedTypeInfo (
    PHTM phTM,
    PETI pETI
    )
{
    eval_t      evalT;
    peval_t     pvT = &evalT;
    EESTATUS    retval = EENOERROR;

    EnterCriticalSection(&csEE);

    DASSERT (*phTM != 0);
    if (*phTM != 0) {
        pExState = (pexstate_t) MemLock (*phTM);

        // if this is not a bound expression, don't bother
        DASSERT ( pExState->state.bind_ok );
        if ( pExState->state.bind_ok ) {

            *pvT = pExState->result;
            pCxt = &pExState->cxt;
            if (EVAL_IS_REF (pvT)) {
                RemoveIndir (pvT);
            }
            if (EVAL_STATE (pvT) != EV_hsym     &&
                EVAL_STATE (pvT) != EV_type ) {
                if (EVAL_IS_FCN (pvT)) {
                    *pETI = ETIFUNCTION;
                }
                else if (EVAL_IS_ARRAY (pvT)) {
                    *pETI = ETIARRAY;
                }
                else if (EVAL_IS_PTR (pvT)) {
                    *pETI = ETIPOINTER;
                }
                else if (EVAL_IS_CLASS (pvT)) {
                    *pETI = ETICLASS;
                }
                else {
                    *pETI = ETIPRIMITIVE;
                }
            } else {
                retval = EECATASTROPHIC;
            }

        } else {
            retval = EECATASTROPHIC;
        }

        MemUnLock (*phTM);

    } else {
        retval = EECATASTROPHIC;
    }

    LeaveCriticalSection(&csEE);
    return (retval);
}

/**     EEIsExpandable - does TM represent expandable item
 *
 *      fSuccess EEIsExpandable (pHTM);
 *
 *      Entry   phTM = pointer to TM handle
 *
 *      Exit    none
 *
 *      Returns FALSE if TM does not represent expandable item
 *              TRUE if TM represents expandable item
 *                  bounded arrays, structs, unions, classes,
 *                  pointers to compound items.
 *
 */


EEPDTYP
EEIsExpandable (
    PHTM phTM
    )
{
    eval_t      evalT;
    peval_t     pvT = &evalT;
    register ulong       retval = EENOTEXP;

    DASSERT (*phTM != 0);
    EnterCriticalSection(&csEE);
    if (*phTM != 0) {
        pExState = (pexstate_t) MemLock (*phTM);
        // do not allow expansion of expressions that
        // contain missing data items.
        if (pExState->state.fNotPresent) {
            MemUnLock (*phTM);
            LeaveCriticalSection(&csEE);
            return EENOTEXP;
        }
        pCxt = &pExState->cxt;
        *pvT = pExState->result;
        if (EVAL_IS_REF (pvT)) {
            RemoveIndir (pvT);
        }
        if (EVAL_STATE (pvT) == EV_type) {
            if (EVAL_IS_PTR(pvT)) {
                retval = EETYPEPTR;
            }
            else if (EVAL_IS_FCN (pvT) ||
                CV_IS_PRIMITIVE (EVAL_TYP (pvT))) {
                retval = EETYPENOTEXP;
            }
            else {
                retval = EETYPE;
            }
        }
        else if (EVAL_IS_ENUM (pvT)) {
            retval = EENOTEXP;
        }
        else if (EVAL_IS_CLASS (pvT) ||
          (EVAL_IS_ARRAY (pvT) && (PTR_ARRAYLEN (pvT) != 0))) {
            retval = EEAGGREGATE;
        }
        else {
            retval = IsExpandablePtr (pvT);
        }
        MemUnLock (*phTM);
    }
    LeaveCriticalSection(&csEE);
    return ((EEPDTYP)retval);
}


/**     EEEnableAutoClassCast - enable automatic class cast
 *
 *      When dereferencing a ptr to a base class B, the EE is
 *      sometimes able to detect the actual underlying class D of the
 *      pointed object (where D is derived from B). In that case the
 *      EE may automatically cast the dereferenced ptr to (D *).
 *      When expanding the object an additional node is generated;
 *      that node corresponds to the enclosing object (which is of type D).
 *      This feature is not always desirable (e.g., it is problematic
 *      in the watch window since a reevaluation of a pointer may change
 *      the type of the additional node and its children)
 *      EEEnableAutoClassCast controls whether this feature is activated
 *      or not. (By default automatic class cast is off)
 *
 *      fPrevious = EEEnableAutoClassCast(fNew)
 *
 *      Entry   BOOL fNew: Control flag for auto class cast feature:
 *                  TRUE: feature enabled
 *                  FALSE: feature disabled
 *
 *      Exit    none
 *
 *      Returns: previous state of auto class cast feature (TRUE if enabled,
 *          FALSE if disabled)
 *
 */

BOOL
EEEnableAutoClassCast (
    BOOL fNew
    )
{
    BOOL fPrevious = fAutoClassCast;
    fAutoClassCast=fNew;
    return fPrevious;
}


/**     EEcSynthChildTM - return count of number of synthesized children
 *                        this TM could have.
 *      To provide more information when a node is expanded, the EE might
 *      introduce "fake" children. The only example of this currently is
 *      the auto-downcast feature which adds a child correspoding to the
 *      "dynamic" type of the pointer. This function returns the number of
 *      synthesized children a TM will have when it is expanded.
 *
 *      Entry   pointer to the TM in question.
 *
 *      Exit    the pcChildren arg has the number of synthesized children
 *              if successful.
 */

EESTATUS
EEcSynthChildTM(
    PHTM phTM,
    long *pcChildren
    )
{
    EESTATUS retval;
    EnterCriticalSection(&csEE);
    retval = (cSynthChildTM (phTM, pcChildren));
    LeaveCriticalSection(&csEE);
    return retval;
}

/**     EEGetBCIA - return the array of the indexes to the base class of this TM.
 *
 *      Given a TM this function indicates which children of the TM are actually
 *      base classes of the TM.
 *
 *      Entry   pointer to the TM in question, .
 *
 *      Exit    the pHBCPIA has the pointer to the handle to an array of
 *              child positions.
 *
 */

EESTATUS
EEGetBCIA(
    HTM * pHTM,
    PHBCIA pHBCIA
    )
{
    EESTATUS retval;
    EnterCriticalSection(&csEE);
    retval = GetMemberIA(pHTM, pHBCIA, CLS_bclass);
    LeaveCriticalSection(&csEE);
    return retval;
}


void
EEFreeBCIA(
    PHBCIA pHBCIA
    )
{
    if(nEEReEntry)
        return;

    DASSERT(pHBCIA != NULL);
    if (pHBCIA != NULL && *pHBCIA != NULL)
    {
        MemFree(*pHBCIA);
    }
}

/**     EEInvalidateCache - invalidate evaluation cache
 *
 *      EEInvalidateCache()
 *
 *      Entry   none
 *
 *      Exit    evaluation cache invalidated
 *              Between two consequtive calls of this funciton
 *              the EE may use caching to avoid reevaluation of
 *              parent subexpressions embedded in child expression
 *              strings.
 *
 *      Returns: void
 *
 */

void EEInvalidateCache (void)
{
    IncrTickCounter();
}


/**     EEParse - parse expression string to abstract syntax tree
 *
 *      ulong  EEParse (szExpr, radix, fCase, phTM);
 *
 *      Entry   szExpr = pointer to expression string
 *              radix = default number radix for conversions
 *              fCase = case sensitive search if TRUE
 *              phTM = pointer to handle of expression state structure
 *              pEnd = pointer to int to receive index of char that ended parse
 *
 *      Exit    *phTM = handle of expression state structure if allocated
 *              *phTM = 0 if expression state structure could not be allocated
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if no error in parse
 *              EECATASTROPHIC if unable to initialize
 *              error number if error
 */


EESTATUS
EEParse (
    const char *szExpr,
    uint radix,
    SHFLAG fCase,
    PHTM phTM,
    ulong  *pEnd
    )
{
    EESTATUS retval;

    // This call may involve floating point operations. Since the kernel
    // does not register a SIGFPE handler under windows we need to mask
    // all floating point exceptions

    EnterCriticalSection(&csEE);
    retval = Parse (szExpr, radix, fCase, TRUE, phTM, pEnd);
    LeaveCriticalSection(&csEE);
    return retval;
}




/**     EEParseBP - parse breakpoint strings
 *
 *      ulong  EEParseBP (pExpr, radix, fCase, pcxf, pTML, select, End, fEnableProlog)
 *
 *      Entry   pExpr = pointer to breakpoint expression
 *              radix = default numeric radix
 *              fCase = case sensitive search if TRUE
 *              pcxt = pointer to initial context for evaluation
 *              pTML = pointer to TM list for results
 *              select = selection mask
 *              pEnd = pointer to int to receive index of char that ended parse
 *              fEnableProlog = FALSE if function scoped symbols only after prolog
 *              fEnableProlog = TRUE if function scoped symbols during prolog
 *
 *      Exit    *pTML = breakpoint information
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if no error in parse
 *              EECATASTROPHIC if unable to initialize
 *              EEGENERAL if error
 */


EESTATUS
EEParseBP (
    char *pExpr,
    uint radix,
    SHFLAG fCase,
    PCXF pCxf,
    PTML pTML,
    ulong select,
    ulong  *pEnd,
    SHFLAG fEnableProlog
    )
{
    register EESTATUS    retval = EECATASTROPHIC;
    ulong       i;

    Unreferenced(select);

    if ((pCxf == NULL) || (pTML == NULL)) {
        return (EECATASTROPHIC);
    }

    EnterCriticalSection(&csEE);

    // note that pTML is not a pointer to a handle but rather a pointer to an
    // actual structure allocated by the caller

    pTMLbp = pTML;
    memset (pTMLbp, 0, sizeof (TML));

    // allocate and initialize the initial list of TMs for overloaded
    // entries.  The TMList is an array of handles to exstate_t's.
    // This list of handles will grow if it is filled.

    if ((pTMLbp->hTMList = MemAllocate (TMLISTCNT * sizeof (HTM))) != 0) {
        pTMList = (HTM *)MemLock (pTMLbp->hTMList);
        memset (pTMList, 0, TMLISTCNT * sizeof (HTM));
        pTMLbp->cTMListMax = TMLISTCNT;

        // parse the break point expression

        retval = EEParse (pExpr, radix, fCase, &pTMList[0], pEnd);
        pTMLbp->cTMListAct++;

        // initialize the backpatch index into PTML.  If this number remains
        // 1 this means that an ambiguous breakpoint was not detected during
        // the bind phase.  As the binder finds ambiguous symbols, information
        // sufficient to resolve each ambiguity is stored in allocated memory
        // and the handle is saved in PTML by AmbToList

        iBPatch = 1;
        if (retval == EENOERROR)
        {
            // bind the breakpoint expression if no parse error
            BindingBP = TRUE;
            if ((retval = EEBindTM (&pTMList[0], SHpCXTFrompCXF (pCxf),
              TRUE, fEnableProlog)) != EENOERROR) {

                // We could not bind the BP because we had something
                // like pfoo->bar where bar is a virtual function.
                if ( pExState->err_num == ERR_NOVIRTUALBP )
                {
                    char funcName[NAMESTRMAX];

                    // We were succesful in getting
                    if (GetVirtFuncName(&pTMList[0], pCxf, funcName, sizeof(funcName)))
                    {
                        retval = EEParse( funcName, radix, fCase, &pTMList[0], pEnd);
                        pTMLbp->cTMListAct = 1;

                        iBPatch = 1;

                        retval = EEBindTM(&pTMList[0], SHpCXTFrompCXF(pCxf),
                                    TRUE, fEnableProlog);
                    }
                }
            }

            if ( retval != EENOERROR )
            {
                // The binder used the pTMList as a location to
                // store information about backpatching.  If there
                // is an error in the bind, go down the list and
                // free all backpatch structure handles.

                for (i = 1; i < iBPatch; i++) {
                    // note that the back patch structure cannot contain
                    // handles to allocated memory that have to be freed up
                    // here.  Otherwise, memory will be lost

                    MemFree (pTMList[i]);
                    pTMList[i] = 0;
                }
            }
            else {
                // the first form of the expression bound correctly.
                // Go down the list, duplicate the expression state
                // and rebind using the backpatch symbol information
                // to resolve the ambiguous symbol.  SearchSym uses the
                // fact that hBPatch is non-zero to decide to handle the
                // next ambiguous symbol.

                for (i = 1; i < iBPatch; i++) {
                    hBPatch = pTMList[i];
                    pTMList[i] = 0;
                    if (DupTM (&pTMList[0], &pTMList[i]) == EENOERROR) {
                        pTMLbp->cTMListAct++;
                        EEBindTM (&pTMList[i], SHpCXTFrompCXF (pCxf),
                          TRUE, fEnableProlog);
		            }
                    MemFree (hBPatch);
                    hBPatch = 0;
                }
            }
        }
        BindingBP = FALSE;
        MemUnLock (pTMLbp->hTMList);
    }
    // return the result of parsing and binding the initial expression.
    // There may have been errors binding subsequent ambiguous breakpoints.
    // It is the caller's responsibility to handle the errors.

    LeaveCriticalSection(&csEE);
    return (retval);
}




/**     ParseBind - parse and bind generated expression
 *
 *      flag = ParseBind (hExpr, hTMIn, phTMOut, pEnd, flags, fCase)
 *
 *      Entry   hExpr = handle of generated expression
 *              hTMIn = handle to TM dereferenced
 *              phTMOut = pointer to handle to dereferencing TM
 *              pEnd = pointer to int to receive index of char that ended parse
 *              flags.BIND_fForceBind = TRUE if bind to be forced
 *              flags.BIND_fForceBind = FALSE if rebind decision left to binder
 *              flags.BIND_fEnableProlog = TRUE if function scope searched during prolog
 *              flags.BIND_fEnableProlog = FALSE if function scope not searched during prolog
 *              flags.BIND_fSupOvlOps = FALSE if overloaded operator search enabled
 *              flags.BIND_fSupOvlOps = TRUE if overloaded operator search suppressed
 *              flags.BIND_fSupBase = FALSE if base searching is not suppressed
 *              flags.BIND_fSupBase = TRUE if base searching is suppressed
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMOut = TM referencing pointer data
 *              hExpr buffer freed
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EEGENERAL if TM not dereferencable
 *              EENOERROR if TM generated
 */


EESTATUS
ParseBind (
    EEHSTR hExpr,
    HTM hTMIn,
    PHTM phTMOut,
    ulong  *pEnd,
    uint flags,
    SHFLAG fCase
    )
{
    pexstate_t          pTMIn;
    register EESTATUS   retval;

    EnterCriticalSection(&csEE);

    pTMIn = (pexstate_t) MHMemLock (hTMIn);
    // Since we are parsing an expression that has been created
    // by the EE, the template names should already be in canonical form
    // So parse with template transformation disabled
    if ((retval = Parse ((char *) MemLock (hExpr), pTMIn->radix, fCase, FALSE, phTMOut, pEnd)) == EENOERROR) {
        retval = DoBind (phTMOut, &pTMIn->cxt, flags);
    }
    MemUnLock (hTMIn);
    MemUnLock (hExpr);
    MemFree (hExpr);
    LeaveCriticalSection(&csEE);

    return (retval);
}
