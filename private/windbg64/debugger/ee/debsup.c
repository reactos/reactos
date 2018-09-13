/***    debsup.c - debapi support routines.
 *
 *      The routines in this module can only be called from debapi.c.
 */


#include "debexpr.h"
#include "debsym.h"


#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY


EESTATUS CountClassElem (HTM, peval_t, long *, ulong );
EESTATUS GetClassiChild (HTM, long, uint, PHTM, ulong  *, SHFLAG);
ulong  SetFcniParm (peval_t, long, PEEHSTR);
bool_t QChildFcnBind (HTM, peval_t, peval_t, EEHSTR);
bool_t UndecorateScope(char *, char far *, uint);
op_t StartNodeOp (HTM);
void LShiftCxtString (EEHSTR);
BOOL fNotPresent (HTM);
EESTATUS BindStMember (HTM, const char *, PHTM, ulong  *, SHFLAG);

static char *pExStrP;


/**     AreTypesEqual - are TM types equal
 *
 *      flag = AreTypesEqual (hTMLeft, hTMRight);
 *
 *      Entry   hTMLeft = handle of left TM
 *              hTMRight = handle of right TM
 *
 *      Exit    none
 *
 *      Returns TRUE if TMs have identical types
 */


bool_t
AreTypesEqual (
    HTM hTMLeft,
    HTM hTMRight
    )
{
    bool_t      retval = FALSE;
    pexstate_t  pExLeft;
    pexstate_t  pExRight;

    if ((hTMLeft != 0) && (hTMRight != 0)) {
        pExLeft = (pexstate_t) MemLock (hTMLeft);
        pExRight = (pexstate_t) MemLock (hTMRight);
#if CC_LAZYTYPES
        if (THAreTypesEqual( EVAL_MOD(&pExLeft->result), EVAL_TYP(&pExLeft->result),
            EVAL_TYP(&pExRight->result) ) )
#else
        if (EVAL_TYP(&pExLeft->result) == EVAL_TYP (&pExRight->result))
#endif
        {
            retval = TRUE;
        }
        MemUnLock (hTMLeft);
        MemUnLock (hTMRight);
    }
    return (retval);
}




/**     cChildrenTM - return number of children for the TM
 *
 *      flag = cChildrenTM (phTM, pcChildren, pVar)
 *
 *      Entry   phTM = pointer to handle of TM
 *              pcChildren = pointer to location to store count
 *
 *      Exit    *pcChildren = number of children for TM
 *
 *      Returns EENOERROR if no error
 *              non-zero if error
 */


ulong
cChildrenTM (
    PHTM phTM,
    long *pcChildren,
    PSHFLAG pVar
    )
{
    ulong       retval = EENOERROR;
    eval_t      eval;
    peval_t     pv = &eval;
    long        len;

    DASSERT (*phTM != 0);

    *pVar = FALSE;
    *pcChildren = 0;
    if (*phTM == 0) {
        return (EECATASTROPHIC);
    }
    pExState = (pexstate_t) MemLock (*phTM);
    pCxt = &pExState->cxt;
    // The kernel may call EEcChildrenTM several times for the
    // same TM. Since the cost of EEcChildrenTM is pretty high now
    // due to the potential bindings of static members, it pays
    // to cache the number of children in the TM.
    if (pExState->state.cChildren_ok) {
        *pcChildren = pExState->cChildren;
    }
    else if (pExState->state.bind_ok == TRUE) {
        eval = pExState->result;
        if (EVAL_IS_REF (pv)) {
            RemoveIndir (pv);
        }
        pExState->err_num = 0;
        if (!CV_IS_PRIMITIVE (EVAL_TYP (pv))) {
            if (EVAL_IS_CLASS (pv)) {
                retval = CountClassElem (*phTM, pv, pcChildren,
                 (ulong ) ((EVAL_STATE (pv) == EV_type)? CLS_defn: CLS_data));
            }
            else if (EVAL_IS_ARRAY (pv) && (PTR_ARRAYLEN (pv) != 0)) {
                // Otherwise, the number of elements is the sizeof the
                // array divided by the size of the underlying type

                len = PTR_ARRAYLEN (pv);
                if(!SetNodeType (pv, PTR_UTYPE (pv))){
                    MemUnLock (*phTM);
                    pExState->err_num = ERR_NOTEXPANDABLE;
                    return (EEGENERAL);
                }
                *pcChildren = len / TypeSize (pv);
            }
            else if (EVAL_IS_ARRAY (pv) && (PTR_ARRAYLEN (pv) == 0)) {
                // if an array is undimensioned in the source then we
                // do not guess how many elements it really has.

                *pcChildren = 0;
                *pVar = TRUE;
            }
            else if (EVAL_IS_PTR (pv)) {
                SetNodeType (pv, PTR_UTYPE (pv));
                if (EVAL_IS_VTSHAPE (pv)) {
                    *pcChildren = VTSHAPE_COUNT (pv);
                }
                else {
                    *pcChildren = 1;
                }
            }
            else {
                pExState->err_num = ERR_INTERNAL;
                retval = EEGENERAL;
            }
        }
        else if (EVAL_IS_PTR (pv)) {
            *pcChildren = 1;
        }
        pExState->cChildren = *pcChildren;
        pExState->state.cChildren_ok = TRUE;
    }
    else {
        pExState->err_num = ERR_NOTEVALUATABLE;
        retval = EEGENERAL;
    }
    MemUnLock (*phTM);
    return (retval);
}


/**     cSynthChildTM - return number of synthesized children for the TM
 *
 *      flag = cSynthChildTM (phTM, pcChildren)
 *
 *      Entry   phTM = pointer to handle of TM
 *              pcChildren = pointer to location to store count
 *
 *      Exit    *pcChildren = number of children for TM
 *
 *      Returns EENOERROR if no error
 *              non-zero if error
 */

ulong
cSynthChildTM(
    PHTM phTM,
    long * pcChildren
    )
{
    ulong  retval = EENOERROR;

    *pcChildren = 0;

    DASSERT(*phTM != 0);
    if ( *phTM == 0 )
    {
        return EECATASTROPHIC;
    }

    pExState = (pexstate_t) MemLock ( *phTM );

    if ( pExState->state.bind_ok == TRUE)
    {
        if (pExState->hDClassName)
        {
            *pcChildren = 1;
        }
    }
    else {
        pExState->err_num = ERR_INTERNAL;
        retval = EEGENERAL;
    }

    MemUnLock(*phTM);

    return retval;
}

/**     cParamTM - return count of parameters for TM
 *
 *      ulong  cParamTM (phTM, pcParam, pVarArg)
 *
 *      Entry   hTM = handle to TM
 *              pcParam = pointer return count
 *              pVarArg = pointer to vararg flags
 *
 *      Exit    *pcParam = count of number of parameters
 *              *pVarArgs = TRUE if function has varargs
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EENOERROR if no error
 */



ulong
cParamTM (
    HTM hTM,
    ulong  *pcParam,
    PSHFLAG pVarArg
    )
{
    peval_t     pv;
    ulong       retval = EECATASTROPHIC;

    DASSERT (hTM != 0);
    if (hTM != 0) {
        pExState = (pexstate_t) MemLock (hTM);
        if (pExState->state.bind_ok == TRUE) {
            pv = &pExState->result;
            if (EVAL_IS_FCN (pv)) {
                if ((*pVarArg = FCN_VARARGS (pv)) == TRUE) {
                    if ((*pcParam = FCN_PCOUNT (pv)) > 0) {
                        (*pcParam)--;
                    }
                }
                else {
                    *pcParam = FCN_PCOUNT (pv);
                }
                if (EVAL_IS_METHOD (pv) && (FCN_THIS(pv) != T_NOTYPE)) {
                    // add one for the this
                    (*pcParam)++;
                }
                retval = EENOERROR;
            }
            else if (EVAL_IS_LABEL (pv)) {
                *pcParam = 0;
                retval = EENOERROR;
            }
            else {
                pExState->err_num = ERR_NOTFCN;
                retval = EEGENERAL;
            }
        }
        else {
            pExState->err_num = ERR_NOTEVALUATABLE;
            retval = EEGENERAL;
        }
        MemUnLock (hTM);
    }
    return (retval);
}




/**     DupTM - duplicate TM
 *
 *      flag = DupTM (phTMIn, phTMOut)
 *
 *      Entry   phTMIn = pointer to handle for input TM
 *              phTMOut = pointer to handle for output TM
 *
 *      Exit    TM and buffers duplicated
 *
 *      Returns EENOERROR if TM duplicated
 *              EENOMEMORY if unable to allocate memory
 */


ulong
DupTM (
    PHTM phTMIn,
    PHTM phTMOut
    )
{
    ulong       retval = EENOMEMORY;
    pexstate_t  pExOut;
    char       *pStr;
    char       *pcName;
    char       *pErrStr;
    uint        len;

    pExState = (pexstate_t) MemLock (*phTMIn);
    pExStr = (char *) MemLock (pExState->hExStr);
    pTree = (pstree_t) MemLock (pExState->hSTree);
    if ((*phTMOut = MemAllocate (sizeof (exstate_t))) != 0) {
        pExOut = (pexstate_t) MemLock (*phTMOut);
        memset (pExOut, 0, sizeof (exstate_t));
        pExOut->ambiguous = pExState->ambiguous;
        pExOut->state.parse_ok = TRUE;
        pExOut->state.fCase   = pExState->state.fCase;

        // copy expression string

        if ((pExOut->hExStr = MemAllocate (pExOut->ExLen = pExState->ExLen)) == 0) {
            goto failure;
        }
        pStr = (char *) MemLock (pExOut->hExStr);
        memcpy (pStr, pExStr, pExOut->ExLen);
        MemUnLock (pExOut->hExStr);

        // copy syntax tree

        if ((pExOut->hSTree = MHMemAllocate (pTree->size)) == 0) {
            goto failure;
        }
        pStr = (char *) MemLock (pExOut->hSTree);
        memcpy (pStr, pTree, pTree->size);
        MemUnLock (pExOut->hSTree);

        // copy name string

        if (pExState->hCName != 0) {
            pcName = (char *) MemLock (pExState->hCName);
            len = _tcslen (pcName) + 1;
            if ((pExOut->hCName = MHMemAllocate (len)) == 0) {
                MemUnLock (pExState->hCName);
                goto failure;
            }
            pStr = (char *) MemLock (pExOut->hCName);
            memcpy (pStr, pcName, len);
            MemUnLock (pExOut->hCName);
            MemUnLock (pExState->hCName);
        }

        // copy saved expression string

        if (pExState->hExStrSav) {
            char * pExStrSav;
            if ((pExOut->hExStrSav = MemAllocate ((pExOut->ExLenSav = pExState->ExLenSav) + 1)) == 0) {
                goto failure;
            }
            pStr = (char *) MemLock (pExOut->hExStrSav);
            pExStrSav = (char *) MemLock (pExState->hExStrSav);
            memcpy (pStr, pExStrSav, pExOut->ExLenSav + 1);
            MemUnLock (pExState->hExStrSav);
            MemUnLock (pExOut->hExStrSav);
            pExOut->strIndexSav = pExState->strIndexSav;
        }

        // copy derived class name string

        if (pExState->hDClassName != 0) {
            pcName = (char *) MemLock (pExState->hDClassName);
            len = _tcslen (pcName) + 1;
            if ((pExOut->hDClassName = MHMemAllocate (len)) == 0) {
                MemUnLock (pExState->hDClassName);
                goto failure;
            }
            pStr = (char *) MemLock (pExOut->hDClassName);
            memcpy (pStr, pcName, len);
            MemUnLock (pExOut->hDClassName);
            MemUnLock (pExState->hDClassName);
        }

        // copy error string

        if (pExState->hErrStr != 0) {
            pErrStr = (char *) MemLock (pExState->hErrStr);
            len = _tcslen (pErrStr) + 1;
            if ((pExOut->hErrStr = MHMemAllocate (len)) == 0) {
                MemUnLock (pExState->hErrStr);
                goto failure;
            }
            pStr = (char *) MemLock (pExOut->hErrStr);
            memcpy (pStr, pErrStr, len);
            MemUnLock (pExOut->hErrStr);
            MemUnLock (pExState->hErrStr);
        }

        MemUnLock (*phTMOut);
        retval = EENOERROR;
    }
succeed:
    MemUnLock (pExState->hExStr);
    MemUnLock (pExState->hSTree);
    MemUnLock (*phTMIn);
    return (retval);

failure:
    if (pExOut->hExStr != 0) {
        MemFree (pExOut->hExStr);
    }
    if (pExOut->hSTree != 0) {
        MemFree (pExOut->hSTree);
    }
    if (pExOut->hCName != 0) {
        MemFree (pExOut->hCName);
    }
    if (pExOut->hExStrSav != 0) {
        MemFree (pExOut->hExStrSav);
    }
    if (pExOut->hDClassName != 0) {
        MemFree (pExOut->hDClassName);
    }
    if (pExOut->hErrStr != 0) {
        MemFree (pExOut->hErrStr);
    }
    MemUnLock (*phTMOut);
    MemFree (*phTMOut);
    *phTMOut = 0;
    goto succeed;
}


/**     GetChildTM - get TM representing ith child
 *
 *      status = GetChildTM (hTMIn, iChild, phTMOut, pEnd, eeRadix, fCase)
 *
 *      Entry   hTMIn = handle of parent TM
 *              iChild = child to get TM for
 *              phTMOut = pointer to handle for returned child
 *              pEnd = pointer to int to receive index of char that ended parse
 *              eeRadix = display radix (override current if != NULL )
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMOut = handle of child TM if allocated
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if no error
 *              non-zero if error: in that case the error code is in
 *              *phTMOut if *phTMOut!=0, otherwise in hTMIn.
 */

EESTATUS
GetChildTM (
    HTM hTMIn,
    long iChild,
    PHTM phTMOut,
    ulong  *pEnd,
    EERADIX eeRadix,
    SHFLAG fCase
    )
{
    eval_t      evalP;
    peval_t     pvP;
    EESTATUS    retval = EENOERROR;
    char        tempbuf[16];
    ulong       len;
    ulong       plen;
    uint        excess;
    pexstate_t  pTM;
    pexstate_t  pTMOut;
    EEHSTR      hDStr = 0;
    EEHSTR      hName = 0;
    char *pDStr;
    char *pName;
    char *format = "[%ld]";
    SE_t        seTemplate;

    DASSERT (hTMIn != 0);
    if (hTMIn == 0) {
        return (EECATASTROPHIC);
    }
    pExState = pTM = (pexstate_t) MemLock (hTMIn);
    if (pTM->state.bind_ok != TRUE) {
        pExState->err_num = ERR_NOTEVALUATABLE;
        MemUnLock (hTMIn);
        return (EEGENERAL);
    }
    pvP = &evalP;
    *pvP = pTM->result;
    pCxt = &pTM->cxt;

    if (EVAL_IS_REF (pvP)) {
        RemoveIndir (pvP);
    }

    GettingChild = TRUE;

    if (EVAL_IS_CLASS (pvP)) {
        // the type of the parent node is a class.  We need to search for
        // the data members if an object is pointed to or the entire definition
        // if the class type is pointed to

        retval = GetClassiChild (hTMIn, iChild,
                (EVAL_STATE (pvP) == EV_type)? CLS_defn: CLS_data,
                phTMOut, pEnd, fCase);
    }
    else {
        pExStrP = (char *) MemLock (pTM->hExStr);
        DASSERT (pTM->strIndex <= pTM->ExLen);
        plen = pTM->strIndex;
        excess = pTM->ExLen - plen;
        switch( eeRadix ? eeRadix : pTM->radix ) {
        case 16:
        format = "[0x%lx]";
        break;
        case 8:
            format = "[0%lo]";
            break;
        }

        if (EVAL_IS_ARRAY (pvP)) {
            // fake up name as [i]  ultoa not used here because 0 converts
            // as null string

            seTemplate = SE_array;
            len = sprintf (tempbuf, format, iChild);
            if (((hName = MemAllocate (len + 1)) == 0) ||
             ((hDStr = MemAllocate (plen + excess + len + 1)) == 0)) {
                goto nomemory;
            }
            pName = (char *) MemLock (hName);
            pDStr = (char *) MemLock (hDStr);
            _tcscpy (pName, tempbuf);
            memcpy (pDStr, pExStrP, plen);
            memcpy (pDStr + plen, pName, len);
            memcpy (pDStr + plen + len, pExStrP + plen, excess);
            *(pDStr + plen + len + excess) = 0;
            MemUnLock (hDStr);
            MemUnLock (hName);
        }
        else if (EVAL_IS_PTR (pvP)) {
            SetNodeType (pvP, PTR_UTYPE (pvP));
            if (!EVAL_IS_VTSHAPE (pvP)) {
                seTemplate = SE_ptr;

                // set name to null
                if (((hName = MemAllocate (1)) == 0) ||
                  ((hDStr = MemAllocate (plen + excess + 3)) == 0)) {
                    goto nomemory;
                }
                pName = (char *) MemLock (hName);
                pDStr = (char *) MemLock (hDStr);
                *pName = 0;
                *pDStr++ = '(';
                memcpy (pDStr, pExStrP, plen);
                pDStr += plen;
                *pDStr++ = ')';
                memcpy (pDStr, pExStrP + plen, excess);
                pDStr += excess;
                *pDStr = 0;
                MemUnLock (hDStr);
                MemUnLock (hName);
            }
            else {
                // fake up name as [i]  ultoa not used here because 0 converts
                // as null string

                seTemplate = SE_array;
                len = sprintf (tempbuf, format, iChild);
                if (((hName = MemAllocate (len + 1)) == 0) ||
                 ((hDStr = MemAllocate (plen + excess + len + 1)) == 0)) {
                    goto nomemory;
                }
                pName = (char *) MemLock (hName);
                pDStr = (char *) MemLock (hDStr);
                _tcscpy (pName, tempbuf);
                memcpy (pDStr, pExStrP, plen);
                memcpy (pDStr + plen, pName, len);
                memcpy (pDStr + plen + len, pExStrP + plen, excess);
                *(pDStr + plen + len + excess) = 0;
                MemUnLock (hDStr);
                MemUnLock (hName);
            }
        }
        else if (EVAL_IS_FCN (pvP)) {
            // the type of the parent node is a function.  We walk down the
            // formal argument list and return a TM that references the ith
            // actual argument.  We return an error if the ith actual is a vararg.

            seTemplate = SE_totallynew;
            if ((retval = SetFcniParm (pvP, iChild, &hName)) == EENOERROR) {
                pName = (char *) MemLock (hName);
                if ((hDStr = MemAllocate ((len = _tcslen (pName)) + 1)) == 0) {
                    MemUnLock (hName);
                    goto nomemory;
                }
                pDStr = (char *) MemLock (hDStr);
                memcpy (pDStr, pName, len);
                *(pDStr + len) = 0;
                MemUnLock (hDStr);
                MemUnLock (hName);
            }
        }
        else {
            pTM->err_num = ERR_NOTEXPANDABLE;
            goto general;
        }

        if ( OP_context == StartNodeOp (hTMIn)) {
            // if the parent expression contains a global context
            // shift the context string to the very left of
            // the child expression (so that this becomes a
            // global context of the child expression too)
            LShiftCxtString ( hDStr );
        }

        retval = ParseBind (hDStr, hTMIn, phTMOut, pEnd,
                  BIND_fSupOvlOps, fCase);
        hDStr = 0; //ParseBind has freed hDStr

        MemUnLock (pTM->hExStr);
        if (retval != EENOERROR) {
            goto general;
        }

        pTMOut = (pexstate_t) MemLock (*phTMOut);
        pTMOut->state.childtm = TRUE;
        if ((pTMOut->hCName = hName) == 0) {
            pTMOut->state.noname = TRUE;
        }
        if ((pTMOut->seTemplate = seTemplate) != SE_totallynew) {
            LinkWithParentTM (*phTMOut, hTMIn);
        }
        MemUnLock (*phTMOut);
    }
    MemUnLock (hTMIn);
    GettingChild = FALSE;
    return (retval);

nomemory:
    pTM->err_num = ERR_NOMEMORY;
general:
    if (hName)
        MemFree (hName);
    if (hDStr)
        MemFree (hDStr);
    MemUnLock (pTM->hExStr);
    MemUnLock (hTMIn);
    GettingChild = FALSE;
    return (EEGENERAL);
}


/**     GetParmTM - get TM representing ith parameter of a TM
 *
 *      status = GetParmTM (hTM, iChild, phDStr, phName)
 *
 *      Entry   hTM = handle of parent TM
 *              iChild = child to get TM for
 *              phDStr = pointer to the expression to be generated
 *              phName = pointer to the child name to be generated
 *              pExState = address of locked expression state
 *
 *      Exit    *phDStr = expression string of child TM
 *              *phName = name of child TM
 *
 *      Returns EESTATUS
*/


EESTATUS
GetParmTM (
    HTM hTM,
    ulong iChild,
    PEEHSTR phDStr,
    PEEHSTR phName
    )
{
    eval_t      evalP;
    peval_t     pvP;
    bool_t      retval = EENOERROR;
    ulong       len;
    ulong       plen;
    uint        excess;
    pexstate_t  pTM;
    char       *pDStr;
    char       *pName;

    DASSERT (hTM != 0);
    if (hTM == 0) {
        return (EECATASTROPHIC);
    }
    pExState = pTM = (pexstate_t) MemLock (hTM);
    if (pTM->state.bind_ok != TRUE) {
        pTM->err_num = ERR_NOTEVALUATABLE;
        MemUnLock (hTM);
        return (EEGENERAL);
    }
    pExStrP = (char *) MemLock (pTM->hExStr);
    pCxt = &pTM->cxt;
    DASSERT (pTM->strIndex <= pTM->ExLen);
    plen = pTM->strIndex;
    excess = pTM->ExLen - plen;
    pvP = &evalP;
    *pvP = pTM->result;
    if (EVAL_IS_REF (pvP)) {
        RemoveIndir (pvP);
    }
    GettingChild = TRUE;
    DASSERT (EVAL_IS_FCN (pvP));

    if (EVAL_IS_FCN (pvP)) {
        // the type of the parent node is a function.  We walk down the
        // formal argument list and return a TM that references the ith
        // actual argument.  We return an error if the ith actual is a vararg.

        if ((retval = SetFcniParm (pvP, iChild, phName)) == EENOERROR) {
            pName = (char *) MemLock (*phName);
            if ((*phDStr = MemAllocate ((len = _tcslen (pName)) + 1)) == 0) {
                MemUnLock (*phName);
                goto nomemory;
            }
            pDStr = (char *) MemLock (*phDStr);
            memcpy (pDStr, pName, len);
            *(pDStr + len) = 0;
            MemUnLock (*phDStr);
            MemUnLock (*phName);
        }
    }
    else {
        pTM->err_num = ERR_NOTEXPANDABLE;
        goto general;
    }
    MemUnLock (pTM->hExStr);
    MemUnLock (hTM);
    GettingChild = FALSE;
    return (retval);

nomemory:
    pTM->err_num = ERR_NOMEMORY;
general:
    MemUnLock (pTM->hExStr);
    MemUnLock (hTM);
    GettingChild = FALSE;
    return (EEGENERAL);
}


/** GetSymName - get name of symbol from node
 *
 *      fSuccess = GetSymName (buf, buflen)
 *
 *      Entry   buf = pointer to buffer for name
 *              buflen = length of buffer
 *
 *      Exit    *buf = symbol name
 *
 *      Returns TRUE if no error retreiving name
 *              FALSE if error
 *
 *      Note    if pExState->hChildName is not zero, then the name in in
 *              the buffer pointed to by hChildName
 */


EESTATUS
GetSymName (
    PHTM phTM,
    PEEHSTR phszName
    )
{
    SYMPTR      pSym;
    int         len = 0;
    UOFFSET     offset = 0;
    char       *pExStr;
    peval_t     pv;
    int         retval = EECATASTROPHIC;
    int         buflen = TYPESTRMAX - 1;
    char       *buf;
    HSYM        hProc = 0;
    ADDR        addr;


    // M00SYMBOL - we now need to allow for a symbol name to be imbedded
    // M00SYMBOL - in a type.  Particularly for scoped types and enums.

    DASSERT (*phTM != 0);
    if ((*phTM != 0) && ((*phszName = MemAllocate (TYPESTRMAX)) != 0)) {
        retval = EEGENERAL;
        buf = (char *) MemLock (*phszName);
        memset (buf, 0, TYPESTRMAX);
        pExState = (pexstate_t) MemLock (*phTM);
        if (pExState->state.bind_ok == TRUE) {
            pv = &pExState->result;
            if ((pExState->state.childtm == TRUE) && (pExState->state.noname == TRUE)) {
                // if there is no name
                MemUnLock (*phTM);
                MemUnLock (*phszName);
                return (EENOERROR);
            }
            else if (pExState->hCName != 0) {

                // M00SYMBOL - for scoped types and symbols, we may be able to
                // M00SYMBOL - hCName to hold the imbedded symbol name

                pExStr = (char *) MemLock (pExState->hCName);
                len = (int)_tcslen (pExStr);
                len = min (len, buflen);
                _tcsncpy (buf, pExStr, len);
                MemUnLock (pExState->hCName);
                retval = EENOERROR;
            }
            else if (EVAL_HSYM (pv) == 0) {
                if ((EVAL_IS_PTR (pv) == TRUE) && (EVAL_STATE (pv) == EV_rvalue)) {
                    addr = EVAL_PTR (pv);
                }
                else {
                    addr = EVAL_SYM (pv);
                }
                if (!ADDR_IS_LI (addr)) {
                    SHUnFixupAddr (&addr);
                }
                if (SHGetNearestHsym (&addr, EVAL_MOD (pv), EECODE, &hProc) == 0) {
                    EVAL_HSYM (pv) = hProc;
                }
            }
            else {

                // if (EVAL_HSYM (pv) != 0)

                if (((pSym = (SYMPTR) MHOmfLock (EVAL_HSYM (pv)))->rectyp) == S_UDT) {
                    // for a UDT, we do not return a name so that a
                    // display of the type will display the type name
                    // only once
                    *buf = 0;
                    MHOmfUnLock(EVAL_HSYM(pv));
                }
                else {
                    MHOmfUnLock(EVAL_HSYM(pv));
                    if (GetNameFromHSYM(buf, EVAL_HSYM(pv))) {
                        retval = EENOERROR;
                    }
                    else {
                        // symbol name was not found
                        // we have either an internal error
                        // or a bad omf
                        pExState->err_num = ERR_INTERNAL;
                        MemUnLock (*phTM);
                        MemUnLock (*phszName);
                        return EEGENERAL;
                    }
                }
            }
        }
        else {
            // if the expression did not bind, return the expression and
            // the error message if one is available

            pExStr = (char *) MemLock (pExState->hExStr);
            len = (int)_tcslen (pExStr);
            len = min (buflen, len);
            _tcsncpy (buf, pExStr, len);
            buf += len;
            buflen -= len;
            MemUnLock (pExState->hExStr);
        }
        MemUnLock (*phszName);
        MemUnLock (*phTM);
    }
    return (retval);
}

/**     GetNameFromHSYM - get symbol name from handle to symbol
 *
 *      BOOL GetNameFromHSYM (lpsz, hSym)
 *
 *      Entry lpsz = pointer to buffer to receive the name string
 *          The buffer should be at least NAMESTRMAX bytes long
 *          hSym = handle to symbol, the name of which is requested
 *
 *      Exit
 *          On success the null terminated name is copied to lpsz
 *
 *      Return value
 *          TRUE if name was found
 *          FALSE if name was not found
 */

BOOL
GetNameFromHSYM(
    char *lpsz,
    HSYM hSym
    )
{
    SYMPTR      pSym;
    ulong       len = 0;
    UOFFSET     offset = 0;
    uint        skip;

    DASSERT (lpsz != 0);
    DASSERT (hSym != 0);
    switch ((pSym = (SYMPTR) MHOmfLock (hSym))->rectyp) {
    case S_REGISTER:
        len = ((REGPTR)pSym)->name[0];
        offset = offsetof (REGSYM, name[1]);
        break;

    case S_CONSTANT:
        // Dolphin #9323:
        // Do not use "offsetof(CONSTSYM, name)"
        // The symbol name is preceded by a numeric leaf
        // ("value") which may have a variable length and the
        // compile time offset may be bogus.
        skip = offsetof (CONSTSYM, value);
        RNumLeaf ((char *)pSym + skip, &skip);
        len = * ((char *)pSym + skip);
        offset = skip + 1;
        break;

    case S_UDT:
        len = ((UDTPTR)pSym)->name[0];
        offset = offsetof (UDTSYM, name[1]);
        break;

    case S_BLOCK16:
        len = ((BLOCKPTR16)pSym)->name[0];
        offset = offsetof (BLOCKSYM16, name[1]);
        break;

    case S_LPROC16:
    case S_GPROC16:
        len = ((PROCPTR16)pSym)->name[0];
        offset = offsetof (PROCSYM16, name[1]);
        break;
    case S_LABEL16:
        len = ((LABELPTR16)pSym)->name[0];
        offset = offsetof (LABELSYM16, name[1]);
        break;

    case S_BPREL16:
        len = ((BPRELPTR16)pSym)->name[0];
        offset = offsetof (BPRELSYM16, name[1]);
        break;

    case S_LDATA16:
    case S_GDATA16:
    case S_PUB16:
        len = ((DATAPTR16)pSym)->name[0];
        offset = offsetof (DATASYM16, name[1]);
        break;

    case S_BLOCK32:
        len = ((BLOCKPTR32)pSym)->name[0];
        offset = offsetof (BLOCKSYM32, name[1]);
        break;

    case S_LPROC32:
    case S_GPROC32:
        len = ((PROCPTR32)pSym)->name[0];
        offset = offsetof (PROCSYM32, name[1]);
        break;

    case S_LPROCMIPS:
    case S_GPROCMIPS:
        len = ((PROCPTRMIPS)pSym)->name[0];
        offset = offsetof (PROCSYMMIPS, name[1]);
        break;

    case S_REGREL32:
        len = ((LPREGREL32)pSym)->name[0];
                offset = offsetof (REGREL32, name[1]);
                break;

    case S_LABEL32:
        len = ((LABELPTR32)pSym)->name[0];
        offset = offsetof (LABELSYM32, name[1]);
        break;

    case S_BPREL32:
        len = ((BPRELPTR32)pSym)->name[0];
        offset = offsetof (BPRELSYM32, name[1]);
        break;

    case S_LDATA32:
    case S_GDATA32:
    case S_LTHREAD32:
    case S_GTHREAD32:
    case S_PUB32:
        len = ((DATAPTR32)pSym)->name[0];
        offset = offsetof (DATASYM32, name[1]);
        break;

     case S_REGIA64:
		len = ((REGPTRIA64)pSym)->name[0];
        offset = offsetof (REGSYMIA64, name[1]);
        break;
 
	 case S_REGRELIA64:
		len = ((LPREGRELIA64)pSym)->name[0];
		offset = offsetof (REGRELIA64, name[1]);
        break;
 
     case S_LPROCIA64:
     case S_GPROCIA64:
		len = ((PROCPTRIA64)pSym)->name[0];
		offset = offsetof (PROCSYMIA64, name[1]);
		break;
 
    default:
        MHOmfUnLock (hSym);
        return FALSE;
    }
    len = min (len, NAMESTRMAX-1);
    _tcsncpy (lpsz, ((char *)pSym) + offset, len);
    MHOmfUnLock (hSym);
    *(lpsz + len) = 0;
    return TRUE;
}



#if 0
/**     InfoFromTM - return information about TM
 *
 *      EESTATUS InfoFromTM (phTM, pReqInfo, phTMInfo);
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
InfoFromTM (
    PHTM phTM,
    PRI pReqInfo,
    PHTI phTMInfo
    )
{
    PTI         pTMInfo;
    eval_t      evalT;
    peval_t     pvT;
    ulong       retval = EEGENERAL;

    DASSERT (*phTM != 0);
    if (*phTM == 0) {
        return (EECATASTROPHIC);
    }
    if ((*phTMInfo = MemAllocate (sizeof (TMI) + sizeof (val_t))) == 0) {
        return (EENOMEMORY);
    }
    pTMInfo = (PTI) MemLock (*phTMInfo);
    memset (pTMInfo, 0, sizeof (TMI));
    pExState = (pexstate_t)MemLock (*phTM);
    if (pExState->state.bind_ok != TRUE) {
        // we must have at least bound the expression

        MemUnLock (*phTMInfo);
        MemUnLock (*phTM);
        return (EEGENERAL);
    }

    pvT = &evalT;
    *pvT = pExState->result;

    // if the node is an lvalue, store address and set response flags

    if (EVAL_STATE (pvT) == EV_lvalue) {
        // set segment information

        pTMInfo->fResponse.fSegType = TRUE;

        if (EVAL_TYP (pvT) == 0) {
            // the type of zero can only come from the publics table
            // this means we don't know anything about the symbol.

            pTMInfo->SegType = EEDATA | EECODE;
        }
        else if (EVAL_IS_DPTR (pvT) == TRUE) {
            pTMInfo->SegType = EEDATA;
        }
        else {
            pTMInfo->SegType = EECODE;
        }
        if (EVAL_IS_REG (pvT)) {
            pTMInfo->hReg = EVAL_REG (pvT);
            pTMInfo->fAddrInReg = TRUE;
            pTMInfo->fResponse.fLvalue = TRUE;
        }
        else if (EVAL_IS_BPREL (pvT) && (pExState->state.eval_ok == TRUE)) {
            pTMInfo->fBPRel = TRUE;
            if (pExState->state.eval_ok == TRUE) {
                ADDR    addr;
                // Address of BP relative lvalue can only be returned after eval
                pTMInfo->AI = EVAL_SYM (pvT);
                SYGetAddr(pExState->hframe, &addr, adrBase);
                pTMInfo->AI.addr.off += GetAddrOff(addr);
                pTMInfo->AI.addr.seg = GetAddrSeg(addr);

                // fixup to a logical address
                SHUnFixupAddr (&pTMInfo->AI);
                pTMInfo->fResponse.fAddr = TRUE;
                pTMInfo->fResponse.fLvalue = TRUE;
            }
        }
        else if ((TargetMachine == mptmips) ||
                 (TargetMachine == mptdaxp))
        {
            if (EVAL_IS_REGREL (pvT) && (pExState->state.eval_ok == TRUE)) {
                if (!ResolveAddr(pvT)) {
                    DASSERT(FALSE);
                }
                if (((TargetMachine == mptmips) &&
                     (EVAL_REGREL(pvT) != CV_M4_IntGP)) ||
                    ((TargetMachine == mptdaxp) &&
                     (EVAL_REGREL(pvT) != CV_ALPHA_IntGP)))
                {
                    pTMInfo->fBPRel = TRUE;
                }

                pTMInfo->fResponse.fAddr = TRUE;
                pTMInfo->fResponse.fLvalue = TRUE;
                pTMInfo->AI = EVAL_SYM (pvT);
            }
        }
        else {
            pTMInfo->fResponse.fAddr = TRUE;
            pTMInfo->fResponse.fLvalue = TRUE;
            pTMInfo->AI = EVAL_SYM (pvT);

        }
        if ((pExState->state.eval_ok == TRUE) && LoadSymVal (pvT)) {
            EVAL_STATE (pvT) = EV_rvalue;
        }
    }

    // if the node is an rvalue containing the address of a function, store
    // the function address and set the response flags

    if ((EVAL_STATE (pvT) == EV_rvalue) &&
      (EVAL_IS_FCN (pvT) || EVAL_IS_LABEL (pvT))) {
        pTMInfo->fResponse.fAddr = TRUE;
        pTMInfo->fResponse.fLvalue = FALSE;
        pTMInfo->AI = EVAL_SYM (pvT);
        if ( ! ADDR_IS_LI (pTMInfo->AI)) {
            SHUnFixupAddr (&pTMInfo->AI);
        }
        pTMInfo->SegType = EECODE;
        pTMInfo->fResponse.fValue = FALSE;
        pTMInfo->fResponse.Type = EVAL_TYP (pvT);
    }
#if 0 // {
    //
    // [cuda#3155 4/20/93 mikemo]
    //
    // This code causes pointers to be automatically dereferenced.  This was
    // by design: it was considered more useful under CodeView that, for
    // example, "dw pch" would dump at the memory pointed to by "pch" rather
    // than at the address of "pch" itself.  However, this unfortunately led
    // to inconsistency between "dw pch" and "dw i"; the latter would dump at
    // the address of "i".  People who didn't realize this were never really
    // sure what their "dw" command was going to do.
    //
    // This also caused lots of confusion for breakpoints: if you said "break
    // when expr <i> changes", it would stop when the value of "i" changed,
    // but if you said "break when expr <pch> changes", it would stop when
    // the value *pointed to* by "pch" changed.  This was an even bigger
    // problem for types like HWND, which looks to the user basically like a
    // scalar, but looks to the debugger like a pointer, so the debugger would
    // try to deref it.
    //
    // So we've decided to do away with the automatic dereference.
    //
    else if ((EVAL_STATE (pvT) == EV_rvalue) &&
      (EVAL_IS_ADDR (pvT))) {
        Evaluating = TRUE;
        if (EVAL_IS_BASED (pvT)) {
            NormalizeBase (pvT);
        }
        Evaluating = FALSE;
        pTMInfo->fResponse.fAddr = TRUE;
        pTMInfo->fResponse.fLvalue = FALSE;
        pTMInfo->AI = EVAL_PTR (pvT);
        pTMInfo->SegType = EECODE | EEDATA;
        if ( ! ADDR_IS_LI (pTMInfo->AI)) {
            SHUnFixupAddr (&pTMInfo->AI);
        }
        pTMInfo->fResponse.fValue = FALSE;
        pTMInfo->fResponse.Type = EVAL_TYP (pvT);
    }
#endif // } 0
    else if ((EVAL_STATE (pvT) == EV_rvalue) ||
      (EVAL_STATE (pvT) == EV_constant)) {

        //  Make sure pointers are unfixed up
        if (EVAL_IS_ADDR(pvT) && ADDR_IS_LI(EVAL_PTR(pvT))) {
            SHFixupAddr(&EVAL_PTR(pvT));
        }

        // if the node is an rvalue or a constant, store value and set response

        if ((EVAL_STATE (pvT) == EV_constant) ||
           (pExState->state.eval_ok == TRUE)) {
            if (CV_IS_PRIMITIVE (pReqInfo->Type)) {
                if (pReqInfo->Type == 0)
                    pReqInfo->Type = EVAL_TYP (pvT);
                Evaluating = TRUE;
                if (CastNode (pvT, pReqInfo->Type, pReqInfo->Type)) {
                    memcpy (&pTMInfo->Value, &pvT->val, sizeof (pvT->val));
                    pTMInfo->fResponse.Type = EVAL_TYP (pvT);
                    pTMInfo->fResponse.fValue = TRUE;
                }
                Evaluating = FALSE;
            }
        }
    }

    // set flag if bind tree contains function call

    pTMInfo->fFunction = pExState->state.fFunction;

    // set flag if we are a synthesized child TM.
    if ( pReqInfo->fSynthChild )
    {
        if ( pExState->seTemplate ==  SE_downcast )
        {
            DASSERT(pExState->state.childtm == TRUE);
            pTMInfo->fSynthChild = TRUE;
        }
        else
            pTMInfo->fSynthChild = FALSE;

        pTMInfo->fResponse.fSynthChild = TRUE;
    }

    if ( pReqInfo->fLabel )
    {
        if ( EVAL_IS_LABEL(pvT) )
            pTMInfo->fLabel = TRUE;
        else
            pTMInfo->fLabel = FALSE;

        pTMInfo->fResponse.fLabel = TRUE;
    }

    // set size of field in bytes unless bits are requested
    // for a bitfield, the bitfield size is returned if bits are
    // requested.  Otherwise, the size of the underlying type is returned

    if (EVAL_IS_BITF (pvT)) {
        if (pReqInfo->fSzBits == TRUE) {
            pTMInfo->cbValue = BITF_LEN (pvT);
            pTMInfo->fResponse.fSzBits = TRUE;
        }
        else {
            EVAL_TYP (pvT) = BITF_UTYPE (pvT);
            pTMInfo->cbValue = TypeSize (pvT);
            pTMInfo->fResponse.fSzBytes = TRUE;
        }
    }
    else if (EVAL_TYP (pvT) != 0) {
        pTMInfo->cbValue = TypeSize (pvT);
        if (pReqInfo->fSzBits == TRUE) {
            pTMInfo->cbValue *= 8;
            pTMInfo->fResponse.fSzBits = TRUE;
        }
        else {
            pTMInfo->fResponse.fSzBytes = TRUE;
        }
    }
    retval = EENOERROR;
    MemUnLock (*phTMInfo);
    MemUnLock (*phTM);
    return (retval);
}


#else // 0

/**     InfoFromTM - return information about TM
 *
 *      EESTATUS InfoFromTM (phTM, pReqInfo, phTMInfo);
 *
 *      Entry   phTM = pointer to the handle for the expression state structure
 *              reqInfo = info request structure
 *              phTMInfo = pointer to handle for request info data structure
 *
 *      Exit    *phTMInfo = handle of info structure
 *
 *      Returns EECATASTROPHIC if fatal error
 *               0 if no error
 *
 *     The return information is based on the input request structure:
 *
 *              fSegType  - Requests the segment type the TM resides in.
 *                              returned in TI.fCode
 *              fAddr     - Return result as an address
 *              fValue    - Return value of TM
 *              fLvalue   - Return address of TM if lValue.  This and
 *                              fValue are mutually exclusive
 *              fSzBits   - Return size of value in bits
 *              fSzBytes  - Return size of value in bytes.  This and
 *                              fSzBits are mutually exclusive.
 *              Type      - If not T_NOTYPE then cast value to this type.
 *                              fValue must be set.
 */


EESTATUS
InfoFromTM (
    PHTM phTM,
    PRI pReqInfo,
    PHTI phTMInfo
    )
{
    EESTATUS    eestatus = EEGENERAL;
    PTI         pTMInfo;
    eval_t      evalT;
    peval_t     pvT;
    SHREG       reg;
    char *p;

    *phTMInfo = 0;

    /*
     *  Verify that there is a TM to play with
     */

    DASSERT( *phTM != 0 );
    if (*phTM == 0) {
        return  EECATASTROPHIC;
    }

    /*
     *  Check for consistancy on the requested information
     */

    if (((pReqInfo->fValue) && (pReqInfo->fLvalue)) ||
        ((pReqInfo->fSzBits) && (pReqInfo->fSzBytes)) ||
        ((pReqInfo->Type != T_NOTYPE) && (!pReqInfo->fValue))) {
        return EEGENERAL;
    }

    /*
     *  Allocate and lock down the TI which is used to return the answers
     */

    if (( *phTMInfo = MHMemAllocate( sizeof(TMI) + sizeof(val_t) )) == 0) {
        return  EENOMEMORY;
    }
    pTMInfo = MHMemLock( *phTMInfo );
    DASSERT( pTMInfo != NULL );
    memset( pTMInfo, 0, sizeof(TMI) + sizeof(val_t) );

    /*
     *  Lock down the TM passed in
     */

    //DASSERT(pExState == NULL);

    pExState = (pexstate_t) MHMemLock( *phTM );
    if ( pExState->state.bind_ok != TRUE ) {
        /*
         *  If the expression has not been bound, then we can't actually
         *      answer any of the questions being asked.
         */

        MHMemUnLock( *phTMInfo );
        MHMemUnLock( *phTM );
        pExState = NULL;
        return EEGENERAL;
    }

    pvT = &evalT;
    *pvT = pExState->result;

    eestatus = EENOERROR;

    /*
     *  If the user asked about the segment type for the expression,
     *  get it.
     */

    if (pReqInfo->fSegType || pReqInfo->fAddr) {

        if (EVAL_STATE( pvT ) == EV_lvalue) {
            pTMInfo->fResponse.fSegType = TRUE;

            /*
             * Check for type of 0.  If so then this must be a public
             *  as all compiler symbols have some type information
             */

            if (EVAL_TYP( pvT ) == 0) {
                pTMInfo->SegType = EEDATA | EECODE;
            }

            /*
             *  If item is of type pointer to data then must be in
             *  data segment
             */

            else if (EVAL_IS_DPTR( pvT ) == TRUE) {
                pTMInfo->SegType = EEDATA;
            }

            /*
             *  in all other cases it must have been a code segment
             */

            else {
                pTMInfo->SegType = EECODE;
            }

        } else if ((EVAL_STATE( pvT ) == EV_rvalue) &&
                   (EVAL_IS_FCN( pvT ) ||
                    EVAL_IS_LABEL( pvT ))) {

            pTMInfo->fResponse.fSegType = TRUE;
            pTMInfo->SegType = EECODE;

        } else if ((EVAL_STATE( pvT ) == EV_rvalue) &&
                   (EVAL_IS_ADDR( pvT ))) {

            pTMInfo->fResponse.fSegType = TRUE;
            pTMInfo->SegType = EECODE | EEDATA;

        } else if ((EVAL_STATE( pvT ) == EV_rvalue) ||
                   (EVAL_STATE( pvT ) == EV_constant)) {
            ;
        }
    }

    /*
     *  If the user asked for the value then get it
     */

    if (pReqInfo->fValue) {
        if ((pExState->state.eval_ok == TRUE) && LoadSymVal(pvT)) {
            EVAL_STATE (pvT) = EV_rvalue;
        }


        if ((EVAL_STATE(pvT) == EV_rvalue) &&
            (EVAL_IS_FCN(pvT) ||
             EVAL_IS_LABEL(pvT))) {

            if ( pReqInfo->Type == T_NOTYPE ) {
                pTMInfo->fResponse.fValue   = TRUE;
                pTMInfo->fResponse.fAddr    = TRUE;
                pTMInfo->fResponse.fLvalue  = FALSE;
                pTMInfo->AI                 = EVAL_SYM( pvT );
                pTMInfo->fResponse.Type     = EVAL_TYP( pvT );
                if (!ADDR_IS_LI(pTMInfo->AI)) {
                    SHUnFixupAddr(&pTMInfo->AI);
                }
            } else {
                Evaluating = TRUE;
                if (CastNode( pvT, pReqInfo->Type, pReqInfo->Type )) {
                    memcpy( pTMInfo->Value, &pvT->val, sizeof( pvT->val ));
                    pTMInfo->fResponse.fValue   = TRUE;
                    pTMInfo->fResponse.Type     = EVAL_TYP( pvT );
                }
                Evaluating = FALSE;
            }

        } else if ((EVAL_STATE( pvT ) == EV_rvalue) &&
                   (EVAL_IS_ADDR( pvT ))) {

            if (EVAL_IS_BASED( pvT )) {
                Evaluating = TRUE;
                NormalizeBase( pvT );
                Evaluating = FALSE;
            }

            if ( pReqInfo->Type == T_NOTYPE ) {

                pTMInfo->fResponse.fValue   = TRUE;
                pTMInfo->fResponse.fAddr    = TRUE;
                pTMInfo->fResponse.fLvalue  = FALSE;
                pTMInfo->AI                 = EVAL_PTR( pvT );
                pTMInfo->fResponse.Type     = EVAL_TYP(pvT);
                if (!ADDR_IS_LI( pTMInfo->AI )) {
                    SHUnFixupAddr( &pTMInfo->AI );
                }
            } else {
                Evaluating = TRUE;
                if (CastNode( pvT, pReqInfo->Type, pReqInfo->Type )) {
                    memcpy( pTMInfo->Value, &pvT->val, sizeof( pvT->val ));
                    pTMInfo->fResponse.fValue   = TRUE;
                    pTMInfo->fResponse.Type     = EVAL_TYP( pvT );
                }
                Evaluating = FALSE;
            }
        } else if ((EVAL_STATE( pvT ) == EV_rvalue) ||
                   (EVAL_STATE( pvT ) == EV_constant)) {

            if ((EVAL_STATE( pvT ) == EV_constant ) ||
                (pExState->state.eval_ok == TRUE)) {

                if (CV_IS_PRIMITIVE( pReqInfo->Type )) {
                    if (pReqInfo->Type == 0) {
                        pReqInfo->Type = EVAL_TYP( pvT );
                    }

                    Evaluating = TRUE;
                    if (CastNode( pvT, pReqInfo->Type, pReqInfo->Type )) {
                        memcpy( pTMInfo->Value, &pvT->val, sizeof( pvT->val ));
                        pTMInfo->fResponse.fValue = TRUE;
                        pTMInfo->fResponse.Type = EVAL_TYP( pvT );
                    }
                    Evaluating = FALSE;
                }
            }
        }

    }


    /*
     *  If the user asked for the lvalue as an address
     */

    if (pReqInfo->fAddr && pReqInfo->fLvalue) {
        pTMInfo->AI = pvT->addr;
        //eestatus = EEGENERAL;
    }

    /*
     *  If the user asked for the value as an address
     */

    if (pReqInfo->fAddr && !pReqInfo->fLvalue) {
        if ((EVAL_STATE(pvT) == EV_lvalue) &&
            (pExState->state.eval_ok == TRUE) &&
            LoadSymVal(pvT)) {
            EVAL_STATE (pvT) = EV_rvalue;
        }


        if ((EVAL_STATE(pvT) == EV_rvalue) &&
            (EVAL_IS_FCN(pvT) ||
             EVAL_IS_LABEL(pvT))) {

            pTMInfo->AI = EVAL_SYM( pvT );
            pTMInfo->fResponse.fAddr = TRUE;
            pTMInfo->fResponse.Type = EVAL_TYP( pvT );

            if (!ADDR_IS_LI(pTMInfo->AI)) {
                SHUnFixupAddr(&pTMInfo->AI);
            }

            eestatus = EENOERROR;
        } else if ((EVAL_STATE(pvT) == EV_rvalue) &&
                   (EVAL_IS_ADDR( pvT ))) {
            if (EVAL_IS_BASED( pvT )) {
                Evaluating = TRUE;
                NormalizeBase( pvT );
                Evaluating = FALSE;
            }

            pTMInfo->fResponse.fAddr = TRUE;
            pTMInfo->AI = EVAL_PTR( pvT );
            pTMInfo->fResponse.Type = EVAL_TYP( pvT );

            if (!ADDR_IS_LI( pTMInfo->AI )) {
                SHUnFixupAddr( &pTMInfo->AI );
            }

        } else if ((EVAL_STATE( pvT ) == EV_rvalue) ||
                   (EVAL_STATE( pvT ) == EV_constant)) {

            if ((EVAL_STATE( pvT ) == EV_constant) ||
                (pExState->state.eval_ok == TRUE)) {

                //pReqInfo->Type = T_ULONG; v-vadimp - why change the type of preqInfo?i386
                Evaluating = TRUE;
                if (CastNode(pvT, pReqInfo->Type, pReqInfo->Type)) {
                    switch( TypeSize( pvT ) ) {
                    case 1:
                        pTMInfo->AI.addr.off = VAL_UCHAR( pvT );
                        break;

                    case 2:
                        pTMInfo->AI.addr.off = VAL_USHORT( pvT );
                        break;

                    case 4:
                        pTMInfo->AI.addr.off = SE32To64( VAL_ULONG( pvT ) );
                        break;

                    case 8:
                        pTMInfo->AI.addr.off = VAL_UQUAD( pvT );
                        if ( !Is64PtrSE(pTMInfo->AI.addr.off) ) {
                            pTMInfo->AI.addr.off = SE32To64(pTMInfo->AI.addr.off);
                        }
                        break;

                    default:
                        eestatus = EEGENERAL;
                        break;
                    }

                    pTMInfo->fResponse.fAddr = TRUE;
                    pTMInfo->fResponse.Type = pReqInfo->Type;

                    if (TargetMachine == mptix86) {
                        if (pTMInfo->SegType & EECODE) {
                            reg.hReg = CV_REG_CS;
                        } else {
                            reg.hReg = CV_REG_DS;
                        }

                        GetReg(&reg, pCxt, pExState->hframe);
                        pTMInfo->AI.addr.seg = reg.Byte2;
                    } else  {
                        pTMInfo->AI.addr.seg = 0;
                    }

                    SHUnFixupAddr( &pTMInfo->AI);
                } else {
                    eestatus = EEGENERAL;
                }
                Evaluating = FALSE;
            } else {
                eestatus = EEGENERAL;
            }
        } else {
            eestatus = EEGENERAL;
        }
    }


    /*
     *  Set the size fields if requested
     */

    if (pReqInfo->fSzBits) {
        if (EVAL_IS_BITF( pvT )) {
            pTMInfo->cbValue = BITF_LEN( pvT );
            pTMInfo->fResponse.fSzBits = TRUE;
        } else {
            if (EVAL_TYP( pvT ) != 0) {
                pTMInfo->cbValue = 8 * TypeSize( pvT );
                pTMInfo->fResponse.fSzBits = TRUE;
            }
        }
    }

    if (pReqInfo->fSzBytes) {
        if (EVAL_IS_BITF( pvT )) {
            EVAL_TYP( pvT ) = BITF_UTYPE( pvT );
        }

        if (EVAL_TYP( pvT ) != 0) {
            pTMInfo->cbValue = TypeSize( pvT );
            pTMInfo->fResponse.fSzBytes = TRUE;
        }
    }

    /*
     *  Set random flags
     */

    pTMInfo->fFunction = pExState->state.fFunction;

    pExStr = MHMemLock(pExState->hExStr);
    p = &pExStr[pExState->strIndex];

    if (*p == ',') {
        p++;
        pTMInfo->fFmtStr = TRUE;
    } else {
        pTMInfo->fFmtStr = FALSE;
    }

    MHMemUnLock( pExState->hExStr );

    MHMemUnLock( *phTMInfo );
    MHMemUnLock( *phTM );
    pExState = NULL;

    return      eestatus;
}

#endif

/**     IsExpandablePtr - check for pointer to displayable data
 *
 *      fSuccess = IsExpandablePtr (pn)
 *
 *      Entry   pn = pointer to node for variable
 *
 *      Exit    none
 *
 *      Returns EEPOINTER if node is a pointer to primitive data or,
 *                  class/struct/union
 *              EEAGGREGATE if node is an array with a non-zero size or is
 *                  a pointer to a virtual function shape table
 *              EENOTEXP otherwise
 */


ulong
IsExpandablePtr (
    peval_t pv
    )
{
    eval_t      evalT;
    peval_t     pvT;
    ulong       retval = EENOTEXP;

    if (EVAL_IS_PTR (pv)) {
        // this will also handle the reference cases
        CV_typ_t typ = PTR_UTYPE(pv);
         if (CV_IS_PRIMITIVE (typ)) {
            // void pointers are not expandable.
            if (CV_TYPE(typ) == CV_SPECIAL && CV_SUBT(typ) == CV_SP_VOID) {
                retval = EENOTEXP;
            } else {
                retval = EEPOINTER;
            }
        }
        else {
            pvT = &evalT;
            CLEAR_EVAL (pvT);
            EVAL_MOD (pvT) = EVAL_MOD (pv);
            SetNodeType (pvT, PTR_UTYPE (pv));
            if (EVAL_IS_CLASS (pvT) || EVAL_IS_PTR (pvT)) {
                retval = EEPOINTER;
            }
            else if (EVAL_IS_VTSHAPE (pvT) ||
              (EVAL_IS_ARRAY (pvT) && (PTR_ARRAYLEN (pv) != 0))) {
                 retval = EEAGGREGATE;
            }
        }
    }
    return (retval);
}


/**     GetDerivClassName - Get name of derived class
 *
 *      Entry   pv = pointer to eval node
 *              bDistinct = if TRUE will only return the value if dynamic class is
 *                 different from the static class of the eval node.
 *              buf = buffer to hold derived class name
 *              lenMax = buffer size (max allowable string length,
 *                  including terminating '\0')
 *
 *      Exit    if eval node corresponds to a pointer to a base
 *              class that points to an object of a derived class
 *              and the base class has a vtable, then we use the
 *              vtable name in the publics section to find out
 *              the actual type of the underlying object. In that
 *              case the derived class name is copied to buf.
 *
 *      Returns TRUE on success (derived class name found & copied)
 *              FALSE otherwise
 *
 */

bool_t
GetDerivClassName(
    peval_t pv,
    BOOL bDistinct,
    char *buf,
    uint lenMax
    )
{
    ADDR        addr;
    ADDR        addrNew;
    CXT         cxt;
    HSYM        hSym;
    SYMPTR      pSym;
    ulong       offset;
    uint        len;
    const int   DPREFIXLEN = 4;
    HTYPE       hType;
    plfEasy     pType;
    uint        lenBClass;
    uint        skip = 1;
    eval_t      eval;
    peval_t     pEval = &eval;
    char *lszExe;
    char *lsz;

    if (EVAL_STATE(pv) == EV_type || !EVAL_IS_PTR(pv)) {
        return FALSE;
    }

    *pEval = *pv;
    if (SetNodeType(pEval, PTR_UTYPE(pv)) == FALSE ||
        !EVAL_IS_CLASS(pEval) || CLASS_VTSHAPE(pEval) == 0 ) {
        return FALSE;
    }

    // the class object has a vfptr.
    // By convention the pointer to the vtable is
    // the first field of the object. We assume that this is
    // a 32bit pointer and we try to find its value.

    memset(&cxt, 0, sizeof(CXT));
    memset(&addrNew, 0, sizeof(addrNew));

    // reinitialize temporary eval node and compute ptr value
    *pEval = *pv;
    if (LoadSymVal(pEval) == FALSE) {
        return FALSE;
    }

    // compute the actual address of vfptr pointer
    addr = EVAL_PTR (pEval);
    if (ADDR_IS_LI (addr)) {
        SHFixupAddr (&addr);
    }
    if (EVAL_IS_PTR (pEval) && (EVAL_IS_NPTR (pEval) || EVAL_IS_NPTR32 (pEval))) {
        addr.addr.seg =  GetSegmentRegister(pExState->hframe, REG_DS);
    }

    // now get the contents of addr in order to
    // find where vfptr points to. Assume a 32bit vfptr
    // if addr is a 32bit address
    // currently we don't handle the 16-bit case
    if (!ADDR_IS_OFF32(addr) ||
        sizeof(UOFFSET) != GetDebuggeeBytes(addr,
                                            sizeof(UOFFSET),
                                            &addrNew.addr.off,
                                            (TargetMachine == mptia64)? T_UQUAD : T_ULONG)
       ) {

        return FALSE;
    }
    // Sign extend this before SHUnFixupAddr is called
    if (TargetMachine != mptia64) {
        addrNew.addr.off = SE32To64( *(DWORD*)(&addrNew.addr.off) );
    }
    if (!SHUnFixupAddr(&addrNew)) {
        return FALSE;
    }

    // create context string
    if (lszExe = SHGetExeName ((HIND) emiAddr(addrNew))) {
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];
        uint  lenName;
        uint  lenExt;

        // discard the full path-- keep only the filename.and extension
        _tsplitpath(lszExe, drive, dir, fname, ext);

        lenName = _tcslen(fname);
        lenExt = _tcslen(ext);
        if (lenName + lenExt + 4 >= lenMax) {
            return FALSE;
        }
        lenMax -= (lenName + lenExt + 4);
        memcpy(buf, "{,,", 3);
        buf += 3;
        memcpy(buf, fname, lenName);
        buf += lenName;
        memcpy(buf, ext, lenExt);
        buf += lenExt;
        *buf++ = '}';
    }

    // Based on the address of the vtable, try to find the
    // corresponding vtable symbol in the publics section
    if (PHGetNearestHsym (&addrNew, (HIND) emiAddr(addrNew), &hSym) == 0) {
        pSym = (SYMPTR) MHOmfLock (hSym);
        switch (pSym->rectyp) {
        case S_PUB16:
            len = ((PUBPTR16)pSym)->name[0];
            offset = offsetof (PUBSYM16, name) + sizeof (char);
            break;
        case S_PUB32:
            len = ((PUBPTR32)pSym)->name[0];
            offset = offsetof (PUBSYM32, name) + sizeof (char);
            break;
        case S_GDATA32:
            len = ((DATAPTR32)pSym)->name[0];
            offset = offsetof (DATASYM32, name) + sizeof (char);
            break;
        default:
            MHOmfUnLock(hSym);
            return FALSE;
        }

        // now we have the public symbol for the vtable
        // of the object
        // the decorated name is ??_7foo@class1@class2...@@type
        // where foo is the derived class name
        // we should convert this string to class2::class1::foo

        lsz = (char *)pSym + offset;
        if (_tcsncmp(lsz, "??_7", 4) ||
            UndecorateScope(lsz + 4, buf, lenMax) == FALSE) {
            return FALSE;
        }
        MHOmfUnLock(hSym);

        // compute undecorated name length
        len = _tcslen(buf);

        // now check if this name is the same as the
        // class name of pEval
        if ((hType = THGetTypeFromIndex (EVAL_MOD (pEval), PTR_UTYPE (pEval))) == 0) {
            return FALSE;
        }
    retry:
        pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
        switch (pType->leaf) {
            case LF_MODIFIER:
                if ((hType = THGetTypeFromIndex (EVAL_MOD (pEval), ((plfModifier)pType)->type)) == 0) {
                    return FALSE;
                }
                goto retry;

            case LF_STRUCTURE:
            case LF_CLASS:
                skip = offsetof (lfClass, data);
                RNumLeaf (((char *)(&pType->leaf)) + skip, &skip);
                lenBClass = *(((char *)&(pType->leaf)) + skip);
                if (!bDistinct || lenBClass != len || _tcsncmp (buf, ((char *)pType) + skip + 1, len) != 0) {
                    // the name found using the vfptr is different from
                    // the original class name; apparently buf contains
                    // the derived class name
                    MHOmfUnLock (hType);
                    return TRUE;
                }
                MHOmfUnLock (hType);
        }
    }
    return FALSE;
}

/**  GetVirtFuncName - Given an expression which evaluates to a virtual function,
 *              figure out the name of the actual function that will get called.
 *
 *   Entry pv =  Pointer to eval node.
 *         buf = buffer to write the string into.
 *         lenmax = amount of memory available in the buffer.
 *
 *   Exit - If the eval node corresponds to a bound virtual function expression,
 *          and we can figure out the actual class of the object returns the
 *          function name of the bound function.
 *
 *   NOTE: This function is currently not general enough. It expects the passed in
 *         TM to correspond to a bound virtual function. Since it has not been tested
 *         in more general scenarios, I have added a test to only do this is the
 *         TM already has an ERR_NOVIRTUALBP. If you want to use it in a more general
 *         context you might need to revisit the code and make sure the general
 *         case works as well.
 */


bool_t
GetVirtFuncName(
    PHTM phTM,
    PCXF pCxf,
    CHAR *buf,
    uint lenmax
    )
{
    bnode_t bnRoot, bnL, bnR ;
    BOOL retVal = TRUE;

    if (phTM == NULL)
        return FALSE;

    pExState = (pexstate_t) MemLock(*phTM);

    // If the expression is not bound correctly, we could have attempted to bind it
    // as a bp and failed because it was a bound virtual function.
    if (pExState->state.bind_ok != TRUE && pExState->err_num != ERR_NOVIRTUALBP) {
        MemUnLock(*phTM);
        return FALSE;
    }

    pTree = (pstree_t) MemLock(pExState->hETree);

    if (pTree == NULL) {
        MemUnLock(*phTM);
        return FALSE;
    }

    bnRoot = (bnode_t)pTree->start_node;
    if ( bnRoot == NULL || (bnL = NODE_LCHILD(bnRoot)) == NULL ||
            (bnR = NODE_RCHILD(bnRoot)) == NULL )
    {
        MemUnLock(pExState->hETree);
        MemUnLock(*phTM);
    }

    // get derived class needs a frame pointer because
    // it needs to figure out the address where the
    // ptr lies and load it in order to determine the
    // real class of the object.
    pExState->hframe = SHhFrameFrompCXF(pCxf);

    if (GetDerivClassName(&(bnL->v[0]), FALSE, buf, lenmax))
    {
        uint len = _tcslen(buf);
        peval_t  pvRight = &bnR->v[0];

        if ( len + 2 < lenmax)
        {
            _tcscat(buf, "::");
            len += 2;
        }
        else {
            retVal = FALSE;
        }

        // Get the member function part of the name of the right node.
        if (retVal && len + EVAL_CBTOK(pvRight) < lenmax)
        {
            char * psz = (char *) MemLock(pExState->hExStr);
            _tcsncat(buf, &psz[EVAL_ITOK(pvRight)], EVAL_CBTOK(pvRight));
            MemUnLock(pExState->hExStr);
        }
    }
    else
    {
        retVal = FALSE;
    }

    MemUnLock(pExState->hETree);
    MemUnLock(*phTM);

    return retVal;
}


/**     UndecorateScope - Undecorate scope info
 *
 *      Entry   lsz = pointer to decorated string
 *                  "Nest0@Nest1@...@NestTop@@type_info"
 *              buf = buffer to receive undecorated scope
 *              lenMax = buffer size (max allowable string length,
 *                  including terminating '\0')
 *
 *      Exit    buf contains a string of the form
 *                  "NestTop:: ... ::Nest1::Nest0"
 *              terminated by a NULL character
 *
 *      Returns TRUE if scope string succesfully transformed
 *              FALSE otherwise
 *
 */

bool_t
UndecorateScope(
    char *lsz,
    char far *buf,
    uint lenMax
    )
{
    char *lszStart;
    char *lszEnd;
    uint        len;

    lszStart = lsz;
    if ((lszEnd = _tcsstr(lsz, "@@")) == 0) {
        return FALSE;
    }

    // every '@' character will be replaced with "::"
    // check if there is enough space  in the destination buffer
    for (len = 0; lsz < lszEnd; lsz++)
        if (*lsz == '@')
            len += 2;
        else
            len++;

    if (len >= lenMax) {
        return FALSE;
    }

    // traverse the string backwards and every time a scope item
    // is found copy it to buf

    lsz = lszEnd - 1;
    while (lsz >= lszStart) {
        for (len = 0; lsz >= lszStart && *lsz != '@'; lsz--) {
            len++;
        }
        // scope item starts at lsz+1
        memcpy (buf, lsz+1, len);
        buf += len;
        if (*lsz == '@') {
            memcpy (buf, "::", 2);
            buf += 2;
            lsz--;
        }
    }
    *buf = '\0';
    return TRUE;
}


/***    CountClassElem - count number of class elements according to mask
 *
 *      error = CountClassElem (hTMIn, pv, pcChildren, search)
 *
 *      Entry   hTMIn = handle of parent TM
 *              pv = pointer to node containing class data
 *              pcChildren = pointer to long to receive number of elements
 *              search = mask specifying which element types to count
 *
 *      Exit    *pcChildren =
 *              count of number of class elements meeting search requirments
 *
 *      Returns EESTATUS
 */


EESTATUS
CountClassElem (
    HTM hTMIn,
    peval_t pv,
    long *pcChildren,
    ulong  search
    )
{
    ulong           cnt;            // total number of elements in class
    HTYPE           hField;         // type record handle for class field list
    char *pField;       // current position within field list
    uint            fSkip = 0;      // current offset in the field list
    uint            anchor;
    ulong           retval = EENOERROR;
    CV_typ_t        newindex;
    char *pc;

    if (pExState->hDClassName) {
        // in this case we can add an extra child of the
        // form (DerivedClass *)pBaseClass, so that the
        // user can see the actual underlying object of
        // a class pointer
        (*pcChildren)++;
    }

    // set the handle of the field list

    if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), CLASS_FIELD (pv))) == 0) {
        DASSERT (FALSE);
        return (0);
    }
    pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);

    //  walk field list to the end counting elements

    for (cnt = CLASS_COUNT (pv); cnt > 0; cnt--) {
        fSkip += SkipPad(((uchar *)pField) + fSkip);
        anchor = fSkip;
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // move to next list in chain

                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) == 0) {
                    DASSERT (FALSE);
                    return (0);
                }
                pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                // the LF_INDEX is not part of the field count
                cnt++;
                break;

            case LF_MEMBER:
                // skip offset of member and name of member
                fSkip += offsetof (lfMember, offset);
                RNumLeaf (pField + fSkip, &fSkip);
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_member) {
                    (*pcChildren)++;
                }
                break;

            case LF_ENUMERATE:
                // skip value name of enumerate
                fSkip += offsetof (lfEnumerate, value);
                RNumLeaf (pField + fSkip, &fSkip);
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_enumerate) {
                    (*pcChildren)++;
                }
                break;

            case LF_STMEMBER:
                fSkip += offsetof (lfSTMember, Name);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_member) {
                    HTM         hTMOut;
                    ulong       end;
                    SHFLAG      fCase = pExState->state.fCase;
                    // Count only static members that are present
                    // try to bind static data member and see if it is present
                    retval = BindStMember(hTMIn, pc,
                                    &hTMOut, &end, fCase);

                    if (retval == EENOERROR && fNotPresent (hTMOut)) {
                        // Just ignore this member and go on with
                        // traversing the field list
                    }
                    else {
                        (*pcChildren)++;
                    }

                    // clean up
                    if (hTMOut)
                        EEFreeTM (&hTMOut);
                }
                break;

            case LF_BCLASS:
                fSkip += offsetof (lfBClass, offset);
                RNumLeaf (pField + fSkip, &fSkip);
                if (search & CLS_bclass) {
                    (*pcChildren)++;
                }
                break;

            case LF_VBCLASS:
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                if (search & CLS_bclass) {
                    (*pcChildren)++;
                }
                break;

            case LF_IVBCLASS:
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                break;

            case LF_FRIENDCLS:
                fSkip += sizeof (lfFriendCls);
                if (search & CLS_fclass) {
                    (*pcChildren)++;
                }
                break;

            case LF_FRIENDFCN:
                fSkip += sizeof (struct lfFriendFcn) +
                  ((plfFriendFcn)(pField + fSkip))->Name[0];
                if (search & CLS_frmethod) {
                    (*pcChildren)++;
                }
                break;

            case LF_VFUNCTAB:
                fSkip += sizeof (lfVFuncTab);
                if (search & CLS_vfunc) {
                    (*pcChildren)++;
                }
                break;


            case LF_METHOD:
                fSkip += sizeof (struct lfMethod) +
                  ((plfMethod)(pField + fSkip))->Name[0];
                cnt -= ((plfMethod)(pField + anchor))->count - 1;
                if (search & CLS_method) {
                    *pcChildren += ((plfMethod)(pField + anchor))->count;
                }
                break;

            case LF_ONEMETHOD:
                fSkip += uSkipLfOneMethod((plfOneMethod)(pField + fSkip));
                if (search & CLS_method)
                    (*pcChildren)++;
                break;


            case LF_NESTTYPE:
                fSkip += sizeof (struct lfNestType) + ((plfNestType)(pField + fSkip))->Name[0];
                if (search & CLS_ntype) {
                    (*pcChildren)++;
                }
                break;

            default:
                pExState->err_num = ERR_BADOMF;
                MHOmfUnLock (hField);
                *pcChildren = 0;
                return (EEGENERAL);
        }
    }
    if (hField != 0) {
        MHOmfUnLock (hField);
    }
    return (retval);
}

const long celemIncr = 4;

// Helper function which adds the next index to the structure.

EESTATUS
AddToIndexArray(
    PHBCIA pHBCIA,
    long * pCount,
    long index
    )
{
    PHINDEX_ARRAY pHIA = (PHINDEX_ARRAY) MemLock(*pHBCIA);

    if ( pHIA == NULL)
        return EEGENERAL;

    if ( pHIA->count == (ulong)*pCount )
    {
        *pCount += celemIncr;
        MemUnLock(*pHBCIA);
        if ( (*pHBCIA =
                 MemReAlloc(*pHBCIA, sizeof(HINDEX_ARRAY) + *pCount * sizeof(long)))
              == 0)
        {
            return EENOMEMORY;
        }
        else {
            pHIA = (PHINDEX_ARRAY) MemLock(*pHBCIA);
        }
    }

    DASSERT(pHIA->count < (ulong)*pCount);
    pHIA->rgIndex[pHIA->count] = index;
    pHIA->count++;
    MemUnLock(*pHBCIA);

    return EENOERROR;
}



/**     GetMemberIA - Return indexes to the members satisfying the search criterion.
 *
 *
 *      Entry   hTM = handle to TM
 *              search - the type of members whose indices are to be included.
 *
 *      Exit    A handle to an index array structure.
 *
 */

EESTATUS
GetMemberIA(
    PHTM phTM,
    PHBCIA pHBCIA,
    ulong  search
    )
{
    ulong  retval = EENOERROR;
    eval_t eval;
    peval_t pv = &eval;
    long celem = celemIncr;
    PHINDEX_ARRAY phIA = NULL;

    DASSERT(*phTM != 0);

    *pHBCIA = NULL;
    if (*phTM == 0) {
        return (EECATASTROPHIC);
    }

    pExState = (pexstate_t) MemLock(*phTM);

    if (pExState->state.bind_ok != TRUE) {
        MemUnLock(*phTM);
        return EEGENERAL;
    }

    eval = pExState->result;
    if (EVAL_IS_REF (pv)) {
        RemoveIndir (pv);
    }

    if ( ((*pHBCIA) = MemAllocate(sizeof(HINDEX_ARRAY) + celem * sizeof(long))) == NULL)
    {
        MemUnLock(*phTM);
        return EENOMEMORY;
    }
    phIA = (PHINDEX_ARRAY) MemLock(*pHBCIA);
    phIA->count = 0;
    MemUnLock(*pHBCIA);

    if (!CV_IS_PRIMITIVE(EVAL_TYP(pv)) && EVAL_IS_CLASS (pv)) {
        long   curIndex = (pExState->hDClassName)? 1 : 0;
        HTYPE hField;
        char *pField;
        uint fSkip = 0;
        uint anchor;
        ulong  cnt;
        CV_typ_t newindex;
        char * pc;
        uint fFoundNonBaseClass = FALSE;

        // set the handle of the field list

        if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), CLASS_FIELD (pv))) == 0) {
            DASSERT (FALSE);
            return (0);
        }

        pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);


        //  walk field list to the end counting elements
        for (cnt = CLASS_COUNT (pv); cnt > 0 && retval == EENOERROR; cnt--, curIndex++) {

            // The CVInfo guarantees that the base classes will be the first elements
            // of the member list. The only case we call this function for is to get
            // the base class list currently. To optimize for this case we keep track of
            // if we found a non-base class member. If that is the case and the caller
            // has only asked us for base classes we do the quick exit.
            if (fFoundNonBaseClass && search == CLS_bclass )
                break;

            fSkip += SkipPad(((uchar *)pField) + fSkip);
            anchor = fSkip;
            switch (((plfEasy)(pField + fSkip))->leaf) {
                case LF_INDEX:
                    // move to next list in chain

                    newindex = ((plfIndex)(pField + fSkip))->index;
                    MHOmfUnLock (hField);
                    if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) == 0) {
                        DASSERT (FALSE);
                        return (0);
                    }
                    pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                    fSkip = 0;
                    // the LF_INDEX is not part of the field count
                    cnt++;
                    curIndex--;
                    break;

                case LF_MEMBER:
                    // skip offset of member and name of member
                    fSkip += offsetof (lfMember, offset);
                    RNumLeaf (pField + fSkip, &fSkip);
                    fSkip += *(pField + fSkip) + sizeof (char);
                    if (search & CLS_member) {
                        retval = AddToIndexArray(pHBCIA, &celem, curIndex);
                    }
                    fFoundNonBaseClass = TRUE;
                    break;

                case LF_ENUMERATE:
                    // skip value name of enumerate
                    fSkip += offsetof (lfEnumerate, value);
                    RNumLeaf (pField + fSkip, &fSkip);
                    fSkip += *(pField + fSkip) + sizeof (char);
                    if (search & CLS_enumerate) {
                        retval = AddToIndexArray(pHBCIA, &celem, curIndex);
                    }
                    fFoundNonBaseClass = TRUE;
                    break;

                case LF_STMEMBER:
                    fSkip += offsetof (lfSTMember, Name);
                    pc = pField + fSkip;
                    fSkip += *(pField + fSkip) + sizeof (char);
                    if (search & CLS_member) {
                        HTM         hTMOut;
                        ulong       end;
                        SHFLAG      fCase = pExState->state.fCase;
                        // Count only static members that are present
                        // try to bind static data member and see if it is present
                        retval = BindStMember(*phTM, pc,
                                        &hTMOut, &end, fCase);

                        if (retval == EENOERROR && fNotPresent (hTMOut)) {
                            // Just ignore this member and go on with
                            // traversing the field list
                        }
                        else if (retval == EENOERROR) {
                            retval = AddToIndexArray(pHBCIA, &celem, curIndex);
                        }

                        // clean up
                        if (hTMOut)
                            EEFreeTM (&hTMOut);
                    }
                    fFoundNonBaseClass = TRUE;
                    break;

                case LF_BCLASS:
                    fSkip += offsetof (lfBClass, offset);
                    RNumLeaf (pField + fSkip, &fSkip);
                    if (search & CLS_bclass) {
                        retval = AddToIndexArray(pHBCIA, &celem, curIndex);
                    }
                    break;

                case LF_VBCLASS:
                    fSkip += offsetof (lfVBClass, vbpoff);
                    RNumLeaf (pField + fSkip, &fSkip);
                    RNumLeaf (pField + fSkip, &fSkip);
                    if (search & CLS_bclass) {
                        retval = AddToIndexArray(pHBCIA, &celem, curIndex);
                    }
                    break;

                case LF_IVBCLASS:
                    fSkip += offsetof (lfVBClass, vbpoff);
                    RNumLeaf (pField + fSkip, &fSkip);
                    RNumLeaf (pField + fSkip, &fSkip);
                    break;

                case LF_FRIENDCLS:
                    fSkip += sizeof (lfFriendCls);
                    if (search & CLS_fclass) {
                        retval = AddToIndexArray(pHBCIA, &celem, curIndex);
                    }
                    fFoundNonBaseClass = TRUE;
                    break;

                case LF_FRIENDFCN:
                    fSkip += sizeof (struct lfFriendFcn) +
                      ((plfFriendFcn)(pField + fSkip))->Name[0];
                    if (search & CLS_frmethod) {
                        retval = AddToIndexArray(pHBCIA, &celem, curIndex);
                    }
                    fFoundNonBaseClass = TRUE;
                    break;

                case LF_VFUNCTAB:
                    fSkip += sizeof (lfVFuncTab);
                    if (search & CLS_vfunc) {
                        retval = AddToIndexArray(pHBCIA, &celem, curIndex);
                    }
                    break;


                case LF_METHOD:
                    fSkip += sizeof (struct lfMethod) + ((plfMethod)(pField + fSkip))->Name[0];
                    cnt -= ((plfMethod)(pField + anchor))->count - 1;
                    if (search & CLS_method) {
                        int count = ((plfMethod)(pField + anchor))->count;
                        while ( count-- )
                        {
                            if ((retval = AddToIndexArray(pHBCIA, &celem, curIndex++)) != EENOERROR)
                                break;
                        }
                        curIndex--;
                    }
                    fFoundNonBaseClass = TRUE;
                    break;

                case LF_ONEMETHOD:
                    fSkip += uSkipLfOneMethod((plfOneMethod)(pField + fSkip));
                    if (search & CLS_method)
                        retval = AddToIndexArray(pHBCIA, &celem, curIndex);
                    fFoundNonBaseClass = TRUE;
                    break;

                case LF_NESTTYPE:
                    fSkip += sizeof (struct lfNestType) + ((plfNestType)(pField + fSkip))->Name[0];
                    if (search & CLS_ntype) {
                        retval = AddToIndexArray(pHBCIA, &celem, curIndex);
                    }
                    fFoundNonBaseClass = TRUE;
                    break;

                default:
                    pExState->err_num = ERR_BADOMF;
                    MHOmfUnLock (hField);
                    MemUnLock(*phTM);
                    return (EEGENERAL);
            }
        }

        if (hField != 0) {
            MHOmfUnLock (hField);
        }
    }

    MemUnLock(*phTM);
    return (retval);
}

/**     fNotPresent - Check if TM contains non-present static data
 *
 *      flag = fNotPresent (hTM)
 *
 *      Entry   hTM = handle to TM
 *
 *      Exit    none
 *
 *      Returns TRUE if TM conatains non-present static data
 *              FALSE otherwise
 */

BOOL
fNotPresent (
    HTM hTM
    )
{
    pexstate_t  pTM;
    BOOL        retval = FALSE;

    DASSERT (hTM);
    pTM = (pexstate_t) MemLock (hTM);

    if (pTM->state.bind_ok && pTM->state.fNotPresent)
        retval = TRUE;
    MemUnLock (hTM);

    return retval;
}


/**     DereferenceTM - generate expression string from pointer to data TM
 *
 *      flag = DereferenceTM (hTMIn, phEStr, phDClassName)
 *
 *      Entry   phTMIn = handle to TM to dereference
 *              phEStr = pointer to handle to dereferencing expression
 *              phDClassName = pointer to handle to derived class name
 *                  in case the TM points to an enclosed base clas object
 *
 *      Exit    *phEStr = expression referencing pointer data
 *              *phDClassName = enclosing object (derived) class name
 *
 *      Returns EECATASTROPHIC if fatal error
 *              EEGENERAL if TM not dereferencable
 *              EENOERROR if expression generated
 */


EESTATUS
DereferenceTM (
    HTM hTM,
    PEEHSTR phDStr,
    PEEHSTR phDClassName
    )
{
    peval_t     pvTM;
    EESTATUS    retval = EECATASTROPHIC;
    ulong       plen;
    uint        excess;
    BOOL        fIsCSymbol = TRUE, fIsReference = FALSE;
    BOOL        fIsRefToPtr = FALSE;
    BOOL        fIsWrapped = FALSE, fShouldWrap;
    char *szEx;

    DASSERT (hTM != 0);
    if (hTM != 0) {
        // lock TM and set pointer to result field of TM

        pExState = (pexstate_t) MemLock (hTM);
        pvTM = &pExState->result;
        if (EVAL_IS_ARRAY (pvTM) || (IsExpandablePtr (pvTM) != EEPOINTER)) {
            pExState->err_num = ERR_NOTEXPANDABLE;
            retval = EEGENERAL;
        }
        else {
            // allocate buffer for *(input string) and copy

            if ((*phDStr = MemAllocate (pExState->ExLen + 4)) != 0) {
                // if not reference and not a C symbol then
                //      generate: expression = *(old_expr)
                //
                // if not reference and is a C symbol then
                //      generate: expression = *old_expr
                //
                // if reference, expression = (old_expr)
                //
                // if reference to a ptr, expression = *old_expr

                pExStr = (char *) MemLock (*phDStr);
                plen = pExState->strIndex;
                excess = pExState->ExLen - plen;
                pExStrP = (char *) MemLock (pExState->hExStr);

                // Determine if the symbol is a pure C symbol
                // Check 1st character
                fIsCSymbol = _istcsymf((_TUCHAR)*pExStrP);

                // Check the rest of the characters
                for(szEx = _tcsinc (pExStrP);
                    fIsCSymbol && szEx < &pExStrP[plen];
                    szEx = _tcsinc (szEx))
                {
                    fIsCSymbol = _istcsym((_TUCHAR)*szEx);
                }

                if (EVAL_IS_REF (pvTM)) {
                    eval_t  EvalTmp;
                    peval_t pv = &EvalTmp;

                    fIsReference = TRUE;
                    *pv = *pvTM;
                    RemoveIndir (pv);
                    if (EVAL_IS_PTR (pv)) {
                        // Dolphin 8336
                        // If we have a reference to a pointer we
                        // should dereference the underlying pointer
                        DASSERT (!EVAL_IS_REF (pv));
                        fIsRefToPtr = TRUE;
                    }
                }

                if (!fIsReference || fIsRefToPtr) {
                    *pExStr++ = '*';
                }

                if(pExStrP[0] == '(' && pExStrP[plen-1] == ')')
                {
                    fIsWrapped = TRUE;
                }

                fShouldWrap = !fIsCSymbol && !fIsReference && !fIsWrapped;

                // If it is not a pure CSymbol then throw in
                // in an extra pair of parens
                if(fShouldWrap)
                {
                    *pExStr++ = '(';
                }
                memcpy (pExStr, pExStrP, plen);
                pExStr += plen;
                if(fShouldWrap)
                {
                    *pExStr++ = ')';
                }
                memcpy (pExStr, pExStrP + plen, excess);
                pExStr += excess;
                *pExStr = 0;
                MemUnLock (pExState->hExStr);
                MemUnLock (*phDStr);

                if ( OP_context == StartNodeOp (hTM)) {
                    // if the parent expression contains a global context
                    // shift the context string to the very left of
                    // the child expression (so that this becomes a
                    // global context of the child expression too)
                    LShiftCxtString ( *phDStr );
                }

                // the code below implements a feature that allows
                // automatic casting of a pointer, as follows:
                // Assume: class B: a base class of class D
                //      D d; // an object of class D
                //      B pB = &d;
                //
                // In certain cases we can automatically detect
                // that the object pointed by pB is actually a
                // D object, and generate an extra child when
                // expanding pB, that has the form
                //      (D *)pB
                //
                // If Class B contains a vtable, auto detection
                // is performed as follows: we find the vtable
                // address (by convention the vfptr is the first
                // record in the class object),then we find a name
                // in the publics that corresponds to this address
                // By undecorating the vtable name, we can get
                // the actual class name of the object pointed to
                // by pB. We store this name in TM->hDClassName;
                // EEcChildrenTM and EEGetChildTM use hDClassName
                // in order to generate the additional child.
                // In order for auto detection to occur, TMIn
                // must have been evaluated.

                /* Block */ {
                    char *pName;
                    int         len;
                    char        buf[TYPESTRMAX];

                    if (fAutoClassCast && !fIsReference && pExState->state.eval_ok &&
                        GetDerivClassName(pvTM, TRUE, buf, sizeof (buf))) {
                        if (*phDClassName) {
                            MemFree(*phDClassName);
                            *phDClassName = 0;
                        }
                        len = _tcslen(buf);
                        if ((*phDClassName = MemAllocate (len + 1)) == 0) {
                            MemUnLock(hTM);
                            return EENOMEMORY;
                        }
                        pName = (char *) MemLock(*phDClassName);
                        _tcscpy(pName, buf);
                        MemUnLock(*phDClassName);
                    }
                } /* end of Block */
                retval = EENOERROR;
            }
        }
        MemUnLock (hTM);
    }
    return (retval);
}

/***    GetClassiChild - Get ith child of a class
 *
 *      status = GetClassiChild (hTMIn, ordinal, search, phTMOut, pEnd, fCase)
 *
 *      Entry   hTMIN = handle to the parent TM
 *              ordinal = number of class element to initialize for
 *                        (zero based)
 *              search = mask specifying which element types to count
 *              phTMOut = pointer to handle for the child TM
 *              pEnd = pointer to int to receive index of char that ended parse
 *                          (M00API: consider removing pEnd from EEGetChildTM API)
 *              fCase = case sensitivity (TRUE is case sensitive)
 *
 *      Exit    *phTMOut contains the child TM that was created
 *
 *      Returns EESTATUS
 */

EESTATUS
GetClassiChild (
    HTM hTMIn,
    long ordinal,
    uint search,
    PHTM phTMOut,
    ulong  *pEnd,
    SHFLAG fCase
    )
{
    HTYPE           hField;         // handle to type record for struct field list
    char *pField;       // current position withing field list
    uint            fSkip = 0;      // current offset in the field list
    uint            anchor;
    uint            len;
    bool_t          retval = ERR_NONE;
    bool_t          fBound = FALSE;
    CV_typ_t        newindex;
    char *pName;
    char *pc;
    char *pDStr;
    char            FName[255];
    char *pFName = FName;
    EEHSTR          hDStr = 0;
    EEHSTR          hName = 0;
    ulong           plen;
    ulong           excess;
    peval_t         pv;
    eval_t          evalP;
    pexstate_t      pTMIn;
    pexstate_t      pTMOut;
    char *pStrP;
    SE_t            seTemplate;
    long            tmpOrdinal;
    GCC_state_t    *pLast;
    eval_t          evalT;
    peval_t         pvT;
    PHDR_TYPE       pHdr;


    pTMIn = (pexstate_t) MemLock (hTMIn);
    if (pTMIn->state.bind_ok != TRUE) {
        pExState->err_num = ERR_NOTEVALUATABLE;
        MemUnLock (hTMIn);
        return (EEGENERAL);
    }

    pCxt = &pTMIn->cxt;
    pStrP = (char *) MemLock (pTMIn->hExStr);

    plen = pTMIn->strIndex;
    excess = pTMIn->ExLen - plen;
    pv = &evalP;
    *pv = pTMIn->result;

    if (EVAL_IS_REF (pv)) {
        RemoveIndir (pv);
    }

    // set fField to the handle of the field list

    if (pTMIn->hDClassName) {
        ordinal--;
        if (ordinal < 0) {
            ulong           lenTypeStr;
            ulong           lenCxtStr;
            ulong           lenDStr;
            ulong           lenDflCxt;
            char *pDClassName;
            char *pTypeStr;
            extern char *   szDflCxtMarker; // CXT string denoting default CXT

            // pDClassName contains a string "{,,<dll>}<type>"
            // pStrP is either "{<cxt>}*<expr>" or "*<expr>"
            // Generate string for downcast node:
            //      {,,<dll>}*(<type> *){<cxt>}<expr> or
            //      {,,<dll>}*(<type> *){<default_cxt>}<expr>
            // respectively

            pDClassName = (char *) MemLock (pTMIn->hDClassName);
            len = _tcslen(pDClassName);

            // skip the context operator that exists in hDClassName
            pTypeStr = _tcschr(pDClassName, '}');
            DASSERT (pTypeStr);
            pTypeStr++;
            lenCxtStr = (ulong)(pTypeStr - pDClassName);
            lenTypeStr = len - lenCxtStr;
            lenDflCxt = _tcslen (szDflCxtMarker);

            DASSERT ( *pStrP == '*' || *pStrP == '{' );

            // The dereferencing '*' found in the parent string
            // will be skipped; use plen - 1 instead of plen
            lenDStr = (plen - 1) + len + excess + 4 + 1;
            if (*pStrP != '{') {
                // There is no explicit context in the parent expression
                // Leave space for a default context operator
                lenDStr += lenDflCxt;
            }

            // allocation for hName includes space for enclosing brackets
            if (((hName = MemAllocate (lenTypeStr + 3)) == 0) ||
              (hDStr = MemAllocate (lenDStr)) == 0) {
                MemUnLock(pTMIn->hDClassName);
                goto nomemory;
            }

            pDStr = (char *) MemLock (hDStr);
            memcpy (pDStr, pDClassName, lenCxtStr);
            pDStr += lenCxtStr;
            memcpy (pDStr, "*(", 2);
            pDStr += 2;
            memcpy (pDStr, pTypeStr, lenTypeStr);
            pDStr += lenTypeStr;
            memcpy (pDStr, "*)", 2);
            pDStr += 2;
            if (*pStrP == '{') {
                char * pch;
                // pStrP should be in the form {...}*parent_expr
                // split pStrP into context and parent_expr
                pch = _tcschr (pStrP, '*');
                DASSERT (pch);
                // add explicit context if different from the
                // one inserted at the beginning
                if (_tcsncmp (pStrP, pDClassName, lenCxtStr)) {
                    memcpy (pDStr, pStrP, (size_t)(pch - pStrP));
                    pDStr += pch - pStrP;
                }
                plen -= (ulong)(pch - pStrP + 1);
                pStrP = pch + 1;
            }
            else {
                // insert default context operator
                memcpy (pDStr, szDflCxtMarker, lenDflCxt);
                pDStr += lenDflCxt;
                // skip dereferencing '*' in parent expression
                DASSERT (*pStrP == '*');
                pStrP ++;
                plen --;
            }

            memcpy (pDStr, pStrP, plen);
            pDStr += plen;
            memcpy (pDStr, pStrP + plen, excess);
            pDStr += excess;
            *pDStr = 0;

            MemUnLock (hDStr);

            // use the derived class name as the child name
            // enclose it in brackets to indicate that this
            // is not an ordinary base class
            // also suppress the context operator
            pName = (char *) MemLock(hName);
            *pName = '[';
            memcpy(pName+1, pTypeStr, lenTypeStr);
            *(pName + lenTypeStr + 1) = ']';
            *(pName + lenTypeStr + 2) = '\0';
            MemUnLock (pTMIn->hDClassName);
            MemUnLock(hName);

            retval = ParseBind (hDStr, hTMIn, phTMOut, pEnd,
                BIND_fSupOvlOps, fCase);
            hDStr = 0; //ParseBind has freed hDStr

            if (retval != EENOERROR) {
                goto general;
            }

            pTMOut = (pexstate_t) MemLock (*phTMOut);
            pTMOut->state.childtm = TRUE;
            pTMOut->hCName = hName;
            pTMOut->seTemplate = SE_downcast;

            // link with parent's parent:
            // parent expr. is a deref'ed ptr
            // parent's parent is the actual ptr
            // being reused when downcasting
            DASSERT (pTMIn->hParentTM);
            LinkWithParentTM (*phTMOut, pTMIn->hParentTM);

            MemUnLock (*phTMOut);
            MemUnLock (pTMIn->hExStr);

            MemUnLock (hTMIn);
            // do not free hName,since it is being used by the TM
            return (retval);
        }
    }

    tmpOrdinal = ordinal;
    pLast = &pExState->searchState;
    if (hTMIn == pLast->hTMIn && ordinal > pLast->ordinal) {
        ordinal -= (pLast->ordinal + 1);
        hField = pLast->hField;
        fSkip = pLast->fSkip;
    }
    else {
        if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), CLASS_FIELD (pv))) == 0) {
            DASSERT (FALSE);
            pTMIn->err_num = ERR_BADOMF;
            MemUnLock (pTMIn->hExStr);
            MemUnLock (hTMIn);
            return EEGENERAL;
        }
    }

    pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
    //  walk field list to iElement-th field
    while (ordinal >= 0) {
        fSkip += SkipPad(((uchar *)pField) + fSkip);
        anchor = fSkip;
        switch (((plfEasy)(pField + fSkip))->leaf) {
            case LF_INDEX:
                // move to next list in chain

                newindex = ((plfIndex)(pField + fSkip))->index;
                MHOmfUnLock (hField);
                if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) == 0) {
                    pTMIn->err_num = ERR_BADOMF;
                    MemUnLock (pTMIn->hExStr);
                    MemUnLock (hTMIn);
                    return EEGENERAL;
                }
                pField = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                break;

            case LF_MEMBER:
                // skip offset of member and name of member
                fSkip += offsetof (lfMember, offset);
                RNumLeaf (pField + fSkip, &fSkip);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_member) {
                    ordinal--;
                }
                break;

            case LF_ENUMERATE:
                // skip value name of enumerate
                fSkip += offsetof (lfEnumerate, value);
                RNumLeaf (pField + fSkip, &fSkip);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_enumerate) {
                    ordinal--;
                }
                break;

            case LF_STMEMBER:
                fSkip += offsetof (lfSTMember, Name);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_member) {
                    // try to bind static data member and see if it is present
                    retval = BindStMember(hTMIn, pc,
                                    phTMOut, pEnd, fCase);

                    if (retval == EENOERROR && fNotPresent (*phTMOut)) {
                        // Non-present static data member
                        // Just ignore this member and go on with
                        // traversing the field list
                        EEFreeTM (phTMOut);
                    }
                    else {
                        ordinal--;
                        if (ordinal < 0) {
                            // this is the member we are looking for
                            fBound = TRUE;  // do not attempt to rebind later
                        }
                        else if (*phTMOut) {
                            EEFreeTM (phTMOut);  // clean up
                        }
                    }
                }
                break;

            case LF_BCLASS:
                fSkip += offsetof (lfBClass, offset);
                RNumLeaf (pField + fSkip, &fSkip);
                if (search & CLS_bclass) {
                    ordinal--;
                }
                break;

            case LF_VBCLASS:
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                if (search & CLS_bclass) {
                    ordinal--;
                }
                break;

            case LF_IVBCLASS:
                fSkip += offsetof (lfVBClass, vbpoff);
                RNumLeaf (pField + fSkip, &fSkip);
                RNumLeaf (pField + fSkip, &fSkip);
                break;

            case LF_FRIENDCLS:
                fSkip += sizeof (struct lfFriendCls);
                if (search & CLS_fclass) {
                    ordinal--;
                }
                break;

            case LF_FRIENDFCN:
                fSkip += sizeof (struct lfFriendFcn) +
                  ((plfFriendFcn)(pField + fSkip))->Name[0];
                if (search & CLS_frmethod) {
                    ordinal--;
                }
                break;

            case LF_VFUNCTAB:
                fSkip += sizeof (struct lfVFuncTab);
                pc = vfuncptr;
                if (search & CLS_vfunc) {
                    ordinal--;
                }
                break;

            case LF_METHOD:
                pc = pField + anchor + offsetof (lfMethod, Name);
                fSkip += sizeof (struct lfMethod) + *pc;
                if (search & CLS_method) {
                    ordinal -= ((plfMethod)(pField + anchor))->count;
                }
                break;

            case LF_ONEMETHOD:
                pc = pbNameInLfOneMethod((plfOneMethod)(pField + anchor));
                fSkip += uSkipLfOneMethod((plfOneMethod)(pField + anchor));
                if (search & CLS_method)
                    ordinal--;
                break;

            case LF_NESTTYPE:
                fSkip += offsetof (lfNestType, Name);
                pc = pField + fSkip;
                fSkip += *(pField + fSkip) + sizeof (char);
                if (search & CLS_ntype) {
                    ordinal--;
                }
                break;

            default:
                MHOmfUnLock (hField);
                pTMIn->err_num = ERR_BADOMF;
                MemUnLock (pTMIn->hExStr);
                MemUnLock  (hTMIn);
                return EEGENERAL;
        }
        if (ordinal < 0) {
            break;
        }
    }

    if (((plfEasy)(pField + anchor))->leaf != LF_METHOD ||
        ordinal == -1) {
        // Update cached information; Methods need special
        // attention since the method list has to be searched
        // based on the value of ordinal. We update the cache
        // only if this is the last method in the method-list
        // (i.e., ordinal == -1)
        pLast->hTMIn = hTMIn;
        pLast->ordinal = tmpOrdinal;
        pLast->hField = hField;
        pLast->fSkip = fSkip;
    }

    // we have found the ith element of the class.  Now create the
    // name and the expression to reference the name

    switch (((plfEasy)(pField + anchor))->leaf) {
        case LF_STMEMBER:
            seTemplate = SE_member;
            // get member name
            len = *pc;
            if ((hName = MemAllocate (len + 1)) == 0)
                goto nomemory;
            pName = (char *) MemLock (hName);
            _tcsncpy (pName, pc + 1, len);
            *(pName + len) = 0;
            MemUnLock (hName);

            // the rest should have already been handled by BindStMember
            break;

        case LF_MEMBER:
        case LF_VFUNCTAB:
        case LF_NESTTYPE:
            seTemplate = SE_member;
            len = *pc;
            if (((hName = MemAllocate (len + 1)) == 0) ||
              ((hDStr = MemAllocate (plen + excess + len + 4)) == 0)) {
                goto nomemory;
            }
            pName = (char *) MemLock (hName);
            _tcsncpy (pName, pc + 1, len);
            *(pName + len) = 0;
            pDStr = (char *) MemLock (hDStr);
            *pDStr++ = '(';
            memcpy (pDStr, pStrP, plen);
            pDStr += plen;
            *pDStr++ = ')';
            *pDStr++ = '.';
            memcpy (pDStr, pName, len);
            pDStr += len;
            memcpy (pDStr, pStrP + plen, excess);
            pDStr += excess;
            *pDStr = 0;
            MemUnLock (hDStr);
            MemUnLock (hName);
            break;

        case LF_BCLASS:
            seTemplate = SE_bclass;
            newindex = ((plfBClass)(pField + anchor))->index;
            MHOmfUnLock (hField);
            if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) != 0) {
                // find the name of the base class from the referenced class

                pField = (char *)(&((TYPPTR)MHOmfLock (hField))->leaf);
                fSkip = offsetof (lfClass, data);
                RNumLeaf (pField + fSkip, &fSkip);
                len = *(pField + fSkip);

                // generate (*(base *)(&expr))

                if (((hName = MemAllocate (len + 1)) == 0) ||
                  ((hDStr = MemAllocate (plen + len + excess + 10)) == 0)) {
                    goto nomemory;
                }
                pName = (char *) MemLock (hName);
                _tcsncpy (pName, pField + fSkip + sizeof (char), len);
                *(pName + len) = 0;
                pDStr = (char *) MemLock (hDStr);
                memcpy (pDStr, "(*(", 3);
                memcpy (pDStr + 3, pField + fSkip + sizeof (char), len);
                memcpy (pDStr + 3 + len, "*)(&", 4);
                memcpy (pDStr + 7 + len, pStrP, plen);
                memcpy (pDStr + 7 + len + plen, "))", 2);
                memcpy (pDStr + 7 + len + plen + 2, pStrP + plen, excess);
                *(pDStr + 9 + len + plen + excess) = 0;
                MemUnLock (hDStr);
                MemUnLock (hName);
            }
            break;

        case LF_VBCLASS:
            seTemplate = SE_bclass;
            newindex = ((plfVBClass)(pField + anchor))->index;
            MHOmfUnLock (hField);
            if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) != 0) {
                // find the name of the base class from the referenced class

                pField = (char *)(&((TYPPTR)MHOmfLock (hField))->leaf);
                fSkip = offsetof (lfClass, data);
                RNumLeaf (pField + fSkip, &fSkip);
                len = *(pField + fSkip);

                // generate (*(base *)(&expr))

                if (((hName = MemAllocate (len + 1)) == 0) ||
                  ((hDStr = MemAllocate (plen + len + excess + 10)) == 0)) {
                    goto nomemory;
                }
                pName = (char *) MemLock (hName);
                //*pName = 0;
                _tcsncpy (pName, pField + fSkip + sizeof (char), len);
                *(pName + len) = 0;
                pDStr = (char *) MemLock (hDStr);
                memcpy (pDStr, "(*(", 3);
                memcpy (pDStr + 3, pField + fSkip + sizeof (char), len);
                memcpy (pDStr + 3 + len, "*)(&", 4);
                memcpy (pDStr + 7 + len, pStrP, plen);
                memcpy (pDStr + 7 + len + plen, "))", 2);
                memcpy (pDStr + 7 + len + plen + 2, pStrP + plen, excess);
                *(pDStr + 9 + len + plen + excess) = 0;
                MemUnLock (hDStr);
                MemUnLock (hName);
            }
            break;

        case LF_FRIENDCLS:
            // look at referenced type record to get name of class
            // M00KLUDGE - figure out what to do here - not bindable

            newindex = ((plfFriendCls)(pField + anchor))->index;
            MHOmfUnLock (hField);
            if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) != 0) {
                pField = (char *)(&((TYPPTR)MHOmfLock (hField))->leaf);
                fSkip = offsetof (lfClass, data);
                RNumLeaf (pField + fSkip, &fSkip);
                len = *(pField + fSkip);
            }
            break;

        case LF_FRIENDFCN:
            // look at referenced type record to get name of function
            // M00KLUDGE - figure out what to do here - not bindable

            newindex = ((plfFriendFcn)(pField + anchor))->index;
            pc = (char *)(((plfFriendFcn)(pField + anchor))->Name[0]);
            break;

        case LF_ONEMETHOD:
            seTemplate = SE_method;
            // copy function name to temporary buffer

            len = *pc;
            memcpy (FName, pc + 1, len);
            FName[len] = 0;
            newindex = ((plfOneMethod)(pField + anchor))->index;

            pvT = &evalT;
            CLEAR_EVAL (pvT);
            // EVAL_MOD (pvT) = SHHMODFrompCXT (pCxt);
            // Need to use EVAL_MOD(pv) instead, as the
            // parent epxression may contain an explicit context
            EVAL_MOD (pvT) = EVAL_MOD (pv);
            FCN_ATTR(pvT) = ((plfOneMethod)(pField + anchor))->attr;
            goto finishMethod;

        case LF_METHOD:
            seTemplate = SE_method;
            // copy function name to temporary buffer

            len = *pc;
            memcpy (FName, pc + 1, len);
            FName[len] = 0;
            newindex = ((plfMethod)(pField + anchor))->mList;
            MHOmfUnLock (hField);

            // index down method list to find correct method

            if ((hField = THGetTypeFromIndex (EVAL_MOD (pv), newindex)) != 0) {
                char *pMethod;

                pMethod = (char *)(&((TYPPTR)MHOmfLock (hField))->data);
                fSkip = 0;
                while (++ordinal < 0) {
                    fSkip += sizeof (mlMethod);
                    if (fIntroducingVirtual(((pmlMethod)(pMethod + fSkip))->attr.mprop)) {
                        fSkip += sizeof(long);
                    }
                }
                pvT = &evalT;
                CLEAR_EVAL (pvT);
                // EVAL_MOD (pvT) = SHHMODFrompCXT (pCxt);
                // Need to use EVAL_MOD(pv) instead, as the
                // parent epxression may contain an explicit context
                EVAL_MOD (pvT) = EVAL_MOD (pv);
                newindex = ((pmlMethod)(pMethod + fSkip))->index;
                MHOmfUnLock (hField);
                hField = 0;
                FCN_ATTR(pvT) = ((pmlMethod)(pMethod + fSkip))->attr;

finishMethod:
                SetNodeType (pvT, newindex);
                EVAL_ACCESS(pvT) = (uchar) FCN_ACCESS(pvT);
                if ((hName = MemAllocate (FCNSTRMAX + sizeof (HDR_TYPE))) == 0) {
                    goto nomemory;
                }

                // FormatType places a structure at the beginning of the buffer
                // containing offsets into the type string.  We need to skip this
                // structure

                pName = (char *) MemLock (hName);
                pHdr = (PHDR_TYPE)pName;
                memset (pName, 0, FCNSTRMAX + sizeof (HDR_TYPE));
                pName = pName + sizeof (HDR_TYPE);
                pc = pName;
                len = FCNSTRMAX - 1;
                FormatType (pvT, &pName, &len, &pFName, 0L, pHdr);
                len = FCNSTRMAX - len;

                // ignore buffer header from FormatType

                memmove ((char *)pHdr, pc, len);
                pc = (char *)pHdr;
                if ((hDStr = MemAllocate (plen + FCNSTRMAX + excess + 2)) == 0) {
                    MemUnLock (hName);
                    goto nomemory;
                }

                pDStr = (char *) MemLock (hDStr);
                memcpy (pDStr, pStrP, plen);
                memcpy (pDStr + plen, ".", 1);
                memcpy (pDStr + 1 + plen, pc, len);
                memcpy (pDStr + 1 + plen + len, pStrP + plen, excess);
                *(pDStr + len + plen + 1 + excess) = 0;
                MemUnLock (hDStr);
                // truncate name to first (
                for (len = 0; (*pc != '(') && (*pc != 0); pc++) {
                    len++;
                }
                *pc = 0;
                MemUnLock (hName);
                if ((hName = MHMemReAlloc (hName, len + 1)) == 0) {
                    goto nomemory;
                }
                if (EVAL_STATE(&pTMIn->result) == EV_type) {

                    fBound = TRUE; // do not call ParseBind
                    seTemplate = SE_totallynew;
                    pDStr = (char *) MemLock(hDStr);
                    *phTMOut = 0;
                    // Ignore return value
                    // What we really care for is the creation of a TM
                    (void) Parse (pDStr, pTMIn->radix, fCase, FALSE, phTMOut, pEnd);
                    if (*phTMOut == 0) {
                        MemUnLock(hDStr);
                        goto general;
                    }
                    pTMOut = (pexstate_t) MemLock (*phTMOut);
                    pTMOut->cxt = pTMIn->cxt;
                    pTMOut->hframe = pTMIn->hframe;
                    MemUnLock(*phTMOut);
                    MemUnLock(hDStr);
                    MemFree(hDStr);
                    hDStr = 0;
                    if (QChildFcnBind (*phTMOut, pv, pvT, hName) == FALSE){
                        retval = EEGENERAL;
                        goto general;
                    }
                    else {
                        retval = EENOERROR;
                    }
                }
            }
            break;

        default:
            pTMIn->err_num = ERR_BADOMF;
            retval = EEGENERAL;
            break;
    }

    if (!fBound) {
        if (hDStr == 0) {
            pTMIn->err_num = ERR_NOTEVALUATABLE;
            retval = EEGENERAL;
        }
        else {

            if ( OP_context == StartNodeOp (hTMIn)) {
                // if the parent expression contains a global context
                // shift the context string to the very left of
                // the child expression (so that this becomes a
                // global context of the child expression too)
                LShiftCxtString ( hDStr );
            }

            retval = ParseBind (hDStr, hTMIn, phTMOut, pEnd,
                BIND_fSupOvlOps, fCase);
            hDStr = 0; //ParseBind has freed hDStr
        }
    }


    if (hField != 0) {
        MHOmfUnLock (hField);
    }

    if (retval != EENOERROR) {
        goto general;
    }

    pTMOut = (pexstate_t) MemLock (*phTMOut);
    pTMOut->state.childtm = TRUE;
    if ((pTMOut->hCName = hName) == 0) {
        pTMOut->state.noname = TRUE;
    }
    if ((pTMOut->seTemplate = seTemplate) != SE_totallynew) {
        LinkWithParentTM (*phTMOut, hTMIn);
    }
    MemUnLock (*phTMOut);

    MemUnLock (pTMIn->hExStr);
    MemUnLock (hTMIn);

    // do not free hName,since it is being used by the TM
    return (retval);

nomemory:
    pTMIn->err_num = ERR_NOMEMORY;
general:
    if (hDStr)
        MemFree (hDStr);
    if (hField != 0) {
        MHOmfUnLock (hField);
    }
    if (hName != 0) {
        MemFree (hName);
    }
    MemUnLock (pTMIn->hExStr);
    MemUnLock (hTMIn);
    return (EEGENERAL);
}


/***    BindStMember - Bind Static Member
 *
 *      Workhorse for handling static members in GetClassiChild
 *      and CountClassElem
 *
 *      error = BindStMember (hTMIn, lstName, phTMOut, pEnd, fCase)
 *
 *      Entry   hTMIn = handle to parent (class) TM
 *              lstName = lenth prefixed name string
 *              phTMOut = pointer to handle to receive bound child TM
 *              pEnd = pointer to int to receive index of char that ended parse
 *              fCase = case sensitivity (TRUE is case sensitive)
 *              pExState points to the parent TM and is preserved
 *
 *
 *      Exit    *phTMOut = handle to parsed and bound TM
 *              corresponding to static data member
 *
 *      Returns EESTATUS
 */

EESTATUS
BindStMember (
    HTM hTMIn,
    const char *lstName,
    PHTM phTMOut,
    ulong  *pEnd,
    SHFLAG fCase
    )
{
    EEHSTR          hDStr = 0;
    pexstate_t      pExStateSav = pExState;
    PCXT            pCxtSav = pCxt;
    uint            len;
    ulong           plen;
    ulong           excess;
    char *pDStr;
    char *pStrP;
    EESTATUS        retval;

    DASSERT (lstName);
    // get parent string
    pStrP = (char *) MemLock (pExState->hExStr);
    plen = pExState->strIndex;
    excess = pExState->ExLen - plen;

    len = *lstName;
    if ((hDStr = MemAllocate (plen + excess + len + 4)) == 0) {
        pExState->err_num = ERR_NOMEMORY;
        return EEGENERAL;
    }
    pDStr = (char *) MemLock (hDStr);
    *pDStr++ = '(';
    memcpy (pDStr, pStrP, plen);
    pDStr += plen;
    *pDStr++ = ')';
    *pDStr++ = '.';
    memcpy (pDStr, lstName+1, len);
    pDStr += len;
    memcpy (pDStr, pStrP + plen, excess);
    pDStr += excess;
    *pDStr = 0;
    MemUnLock (hDStr);
    MemUnLock (pExState->hExStr);

    if ( OP_context == StartNodeOp (hTMIn)) {
        // if the parent expression contains a global context
        // shift the context string to the very left of
        // the child expression (so that this becomes a
        // global context of the child expression too)
        LShiftCxtString ( hDStr );
    }

    retval = ParseBind (hDStr, hTMIn, phTMOut, pEnd,
        BIND_fSupOvlOps, fCase);

    pExState = pExStateSav;
    pCxt = pCxtSav;
    return retval;
}



/***    QChildFcnBind - Quick bind of a child TM that represents a function
 *
 *      success =  QChildFcnBind (hTMOut, pvP, pv, hName)
 *
 *      Entry   hTMOut = handle to the child TM
 *              pvP = evaluation node of the parent TM (a class)
 *              pv = evaluation node of the child (a function)
 *              hName = handle to (non-qualified) function name
 *
 *      Exit    *phTMOut updated
 *
 *      Returns TRUE on success, FALSE otherwise
 */

bool_t
QChildFcnBind (
    HTM hTMOut,
    peval_t pvP,
    peval_t pv,
    EEHSTR hName
    )
{
    HTYPE        hClass;
    HDEP         hQual;
    char        *pQual;
    char        *pName;
    uint         lenCl;
    uint         lenSav;
    peval_t      pvR;
    pexstate_t   pTMOut;
    char        *pField;
    uint         fSkip = 0;
    char        *pc;
    search_t     Temp;
    psearch_t    pTemp = &Temp;
    HR_t         search;


    pTMOut = (pexstate_t) MemLock(hTMOut);
    pvR = &pTMOut->result;
    memcpy(pvR, pv, sizeof (eval_t));
    EVAL_STATE(pvR) = EV_type;

    if ((hClass = THGetTypeFromIndex (EVAL_MOD (pvP), EVAL_TYP (pvP))) != 0) {
        pField = (char *)(&((TYPPTR)MHOmfLock (hClass))->leaf);
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
            MHOmfUnLock (hClass);
            MemUnLock(hTMOut);
            return FALSE;
        }

        lenCl = *pc++;
        pName = (char *) MemLock (hName);
        lenSav = _tcslen(pName);
        if ((hQual = MemAllocate(lenCl+2+lenSav+1)) == 0) {
            MemUnLock(hTMOut);
            return FALSE;
        }

        // form qualified name
        pQual = (char *) MemLock(hQual);
        memcpy (pQual, pc, lenCl);
        *(pQual+lenCl) = ':';
        *(pQual+lenCl+1) = ':';
        memcpy (pQual+lenCl+2, pName, lenSav+1);
        memset (pTemp, 0, sizeof (*pTemp));
        MemUnLock(hName);
        pTemp->pfnCmp = (PFNCMP) FNCMP;
        pTemp->pv = pv;
        pTemp->CXTT = *pCxt;
        pTemp->sstr.lpName = (uchar *) pQual;
        pTemp->sstr.cb = (uchar)_tcslen ((char *)pQual);
        pTemp->state = SYM_init;
        pTemp->scope = SCP_module | SCP_global;
        pTemp->sstr.searchmask |= SSTR_proc;
        pTemp->initializer = INIT_qual;
        pTemp->typeOut = EVAL_TYP (pv);
        search = SearchSym (pTemp);
        MemUnLock (hQual);
        MemFree (hQual);
        if (search != HR_found) {
            FCN_NOTPRESENT (pvR) = TRUE;
        }
        else {
            // pop off the stack entry that a successful search found.  Move the
            // static data member flag first so that it will not be lost.
            EVAL_IS_STMEMBER (ST) = EVAL_IS_STMEMBER (pvR);
            //*pvR = *(ST);
            PopStack ();
        }
        pTMOut->state.bind_ok = TRUE;
        MHOmfUnLock (hClass);
    }
    MemUnLock(hTMOut);
    return TRUE;
}


/***    SetFcniParm - Set a node to a specified parameter of a function
 *
 *      fFound = SetFcniParm (pv, ordinal, pHStr)
 *
 *      Entry   pv = pointer to node to be initialized
 *              ordinal = number of struct element to initialize for
 *                        (zero based)
 *              pHStr = pointer to handle for parameter name
 *
 *      Exit    pv initialized if no error
 *              *pHStr = handle for name
 *
 *      Returns EENOERROR if parameter found
 *              EEGENERAL if parameter not found
 *
 *      This routine is essentially a kludge.  We are depending upon the
 *      the compiler to output the formals in order of declaration before
 *      any of the hidden parameters or local variables.  We also are
 *      depending upon the presence of an S_END record to break us out of
 *      the search loop.
 */

static      HSYM    lastFunc    = 0;
static      HSYM    lastParm    = 0;
static      long    lastOrdinal = 0;

ulong
SetFcniParm (
    peval_t pv,
    long ordinal,
    PEEHSTR pHStr
    )
{
    char *pStr;
    HSYM        hSym;
    SYMPTR      pSym;
    ulong       offset;
    ulong       len;
    bool_t      retval;
    long        tmpOrdinal;
    bool_t      checkThis = FALSE;

    if ((ordinal > FCN_PCOUNT (pv)) ||
      ((ordinal == (FCN_PCOUNT (pv) - 1)) && (FCN_VARARGS (pv) == TRUE))) {
        // attempting to reference a vararg or too many parameters

        pExState->err_num = ERR_FCNERROR;
        return (EEGENERAL);
    }
    hSym = EVAL_HSYM (pv);

    //try to start up where we where last time instead of swimming thru
    // the entire list again
    // sps - 11/25/92
    tmpOrdinal = ordinal;
    if ((hSym == lastFunc) && (ordinal > lastOrdinal)) {
        ordinal -= (lastOrdinal + 1);   //zero based step function
        hSym = lastParm;
    }
    else {
        lastFunc = hSym;
        checkThis = TRUE;
    }
    lastOrdinal = tmpOrdinal;


    for (;;) {

        if ((hSym =  SHNextHsym (EVAL_MOD (pv), hSym)) == 0) {
            goto ErrReturn;
        }
        // lock the symbol and check the type
        pSym = (SYMPTR) MHOmfLock (hSym);
        switch (pSym->rectyp) {
            case S_BPREL16:
                if (((BPRELPTR16)pSym)->off >= 0) {
                // This is a formal argument
                    ordinal--;
                    len = ((BPRELPTR16)pSym)->name[0];
                    offset = offsetof (BPRELSYM16, name) + sizeof (char);
                }
                break;

            case S_BPREL32:
                if (((BPRELPTR32)pSym)->off >= 0) {
                // This is a formal argument
                    len = ((BPRELPTR32)pSym)->name[0];
                    offset = offsetof (BPRELSYM32, name) + sizeof (char);
                    if ((len == 12) &&
                        (_tcsncmp((_TXCHAR *) ((BPRELPTR32)pSym)->name+1, "__$ReturnUdt", len) == 0)) {
                        break;
                    }
                    ordinal--;
                }
                break;

            case S_REGREL32:
                if (((LPREGREL32)pSym)->off >= 0) {
                // Formal parameter
                    len = ((LPREGREL32)pSym)->name[0];
                    offset = offsetof (REGREL32, name[1]);
                    // Dolphin 7908
                    // Intel's 'this' has negative offset but Mips
                    // has offset 0 so special case
                    if (checkThis && EVAL_IS_METHOD(pv) &&
                        (len == (ulong)OpName[0].str[0]) &&
                        (_tcsncmp((_TXCHAR *) ((LPREGREL32)pSym)->name+1, OpName[0].str+1, len) == 0)) {
                        checkThis = FALSE;
                        break;
                    }
                    checkThis = FALSE;
                    if ((len == 12) &&
                        (_tcsncmp((_TXCHAR *) ((LPREGREL32)pSym)->name+1, "__$ReturnUdt", len) == 0)) {
                        break;
                    }
                   ordinal--;
                }
                break;

            case S_REGISTER:
                // ALPHA parameters are homed when compiled with debugging
                // but this pointer may be kept in a register
                if (FCN_CALL (pv) == FCN_ALPHA) {
                    break;
                }
#if 0
                // this can be a formal argument for fastcall
                if (FCN_CALL (pv) == FCN_FAST) {
                    ordinal--;
                    len = ((REGPTR)pSym)->name[0];
                    offset = offsetof (BPRELSYM16, name) + sizeof (char);
                }
                else {
                    MHOmfUnLock (hSym);
                    goto ErrReturn;
                }
#endif
                ordinal--;
                len = ((REGPTR)pSym)->name[0];
                offset = offsetof (REGSYM, name) + sizeof (char);
                break;

            case S_END:
            case S_BLOCK16:
            case S_BLOCK32:
            case S_ENDARG:
                // we should never get here
                MHOmfUnLock (hSym);
                goto ErrReturn;

            default:
                break;
        }
        if (ordinal < 0) {
            break;
        }
        MHOmfUnLock (hSym);
    }

    // if we get here, pSym points to the symbol record for the parameter

    if ((*pHStr = MemAllocate (len + 1)) != 0) {
        pStr = (char *) MemLock (*pHStr);
        _tcsncpy (pStr, ((char *)pSym) + offset, len);
        *(pStr + len) = 0;
        MemUnLock (*pHStr);
        retval = EENOERROR;
    }
    else {
        MHOmfUnLock (hSym);
        goto ErrReturn;
    }
    lastParm = hSym;
    MHOmfUnLock (hSym);
    return (retval);

ErrReturn:
    lastFunc = lastParm = 0;
    lastOrdinal = 0;
    pExState->err_num = ERR_BADOMF;
    return (EEGENERAL);
}

bool_t
ResolveAddr(
    peval_t     pv
    )
{
    UOFFSET     ul;
    ADDR        addr;
    SHREG       reg;

    /*
     *  Fixup BP Relative addresses.  The BP register always comes in
     *  as part of the frame.
     *
     *  This form is currently only used by x86 systems.
     */

    if (EVAL_IS_BPREL (pv)) {
        SYGetAddr(pExState->hframe, &addr, adrBase);
        EVAL_SYM_OFF (pv) += GetAddrOff(addr);
        EVAL_SYM_SEG (pv) = GetAddrSeg(addr);
        EVAL_SYM_EMI (pv) = NULL;
        EVAL_IS_BPREL (pv) = FALSE;
        ADDR_IS_LI (EVAL_SYM (pv)) = FALSE;
        SHUnFixupAddr (&EVAL_SYM (pv));
    }

    /*
     *  Fixup register relative addresses.  This form is currently used
     *  by all non-x86 systems.
     *
     *  We need to see if we are relative to the "Frame register" for the
     *  machine.  If so then we need to pick the address up from the
     *  frame packet rather than going out and getting the register
     *  directly.  This has implications for getting variables up a stack.
     *
     *  This code is from windbg's EE. It used to reference pCxt but that will
     *  fault when called from EEInfoFromTM so I've changed it to reference
     *  pExState->cxt instead. [Dolphin 13042]
     */

    //
    //  We get to simplify this code somewhat.  Since we now have a magic
    //  cookie corresponding to a specific frame, we can now go directly to
    //  GetRegistr for all register relative variables.  We automatically
    //  get the register corresponding to the frame we want.
    //

    else if (EVAL_IS_REGREL (pv)) {
        reg.hReg = EVAL_REGREL (pv);

        if ((TargetMachine == mptmips) &&
            (reg.hReg == CV_M4_IntSP || reg.hReg == CV_M4_IntS8))
        {
            ADDR        addrBP;

            SYGetAddr(pExState->hframe, &addrBP, adrBase);

            if (reg.hReg == CV_M4_IntS8) {
                // See if we're Top of Stack
                reg.hReg = CV_M4_IntSP;
                if (GetReg(&reg, &pExState->cxt, NULL) == NULL) {
                    DASSERT (FALSE);
                } else {
                    ul = reg.Byte4;
                    if (ul == addrBP.addr.off) {
                        reg.hReg = CV_M4_IntS8; // Yes, then use S8
                        if (GetReg(&reg, &pExState->cxt, NULL) == NULL) {
                            DASSERT(FALSE);
                        } else {
                            ul = reg.Byte4;
                        }
                    } else {
                        ul = addrBP.addr.off;
                    }
                }
            } else {
                ul = addrBP.addr.off;
            }
            if (pExState->cxt.hProc) {
                SYMPTR pSym = (SYMPTR) MHOmfLock(pExState->cxt.hProc);
                if ((pSym->rectyp == S_LPROCMIPS) ||
                    (pSym->rectyp == S_GPROCMIPS)) {
                    if (((PROCPTRMIPS)pSym)->pParent) { // nested?
                        HSYM hSLink32 = SHFindSLink32(&pExState->cxt);
                        CXT cxtChild;
                        ADDR addr = {0};
                        if (hSLink32) {
                            SLINK32* pSLink32 = (SLINK32*) MHOmfLock(hSLink32);
                            reg.hReg = pSLink32->reg;
                            if (reg.hReg != CV_M4_IntSP && reg.hReg != CV_M4_IntS8) {
                                if (GetReg(&reg, &pExState->cxt, NULL) == NULL) {
                                    DASSERT (FALSE);
                                } else {
                                    ul = reg.Byte4;
                                }
                            }
                            if (pSLink32->off != 0) {
                                addr.addr.off = ul + pSLink32->off;
                                ADDR_IS_OFF32(addr) = TRUE;
                                ADDR_IS_FLAT(addr) = TRUE;
                                GetDebuggeeBytes(addr, sizeof(UOFF32), &ul, T_ULONG);
                            }
                            cxtChild = pExState->cxt;
                            cxtChild.hBlk = 0;
                            for (;;) {
                                CXT cxtParent;
                                HSYM parent = SHGoToParent(&cxtChild, &cxtParent);
                                if (parent) {
                                    SYMPTR pParent = MHOmfLock(parent);
                                    switch (pParent->rectyp) {
                                    case S_BLOCK16:
                                    case S_BLOCK32:
                                        cxtChild = cxtParent;
                                        continue;

                                    case S_GPROCMIPS:
                                    case S_LPROCMIPS:
                                        if (((PROCPTRMIPS)pParent)->pParent) {
                                            addr.addr.off = ul - 4;
                                            cxtChild = cxtParent;
                                            ADDR_IS_OFF32(addr) = TRUE;
                                            ADDR_IS_FLAT(addr) = TRUE;
                                            GetDebuggeeBytes(addr, sizeof(UOFF32), &ul, T_ULONG);
                                            break;
                                        }
                                        // Fall through
                                    default:
                                        pParent = 0;
                                        break;
                                    }
                                    MHOmfUnLock(parent);
                                    if (!pParent) {
                                        break;
                                    }
                                }
                            }
#pragma message("Stack will always grow down, right?")
                            ul -= pSLink32->framesize;
                            MHOmfUnLock(hSLink32);
                        }
                    }
                }
                MHOmfUnLock(pExState->cxt.hProc);
            }

        } else
        if ((TargetMachine == mptdaxp) &&
            (reg.hReg == CV_ALPHA_IntSP || reg.hReg == CV_ALPHA_IntFP))
        {
            ADDR        addrBP;

            SYGetAddr(pExState->hframe, &addrBP, adrBase);

            if (reg.hReg == CV_ALPHA_IntFP) {
                // See if we're Top of Stack
                reg.hReg = CV_ALPHA_IntSP;
                if (GetReg(&reg, &pExState->cxt, NULL) == NULL) {
                    DASSERT (FALSE);
                } else {
                    ul = reg.Byte8i;
                    if (ul == addrBP.addr.off) {
                        reg.hReg = CV_ALPHA_IntFP; // Yes, then use FP
                        if (GetReg(&reg, &pExState->cxt, NULL) == NULL) {
                            DASSERT(FALSE);
                        } else {
                            ul = reg.Byte8i;
                        }
                    } else {
                        ul = addrBP.addr.off;
                    }
                }
            } else {
                ul = addrBP.addr.off;
            }
            if (pExState->cxt.hProc) {
                SYMPTR pSym = (SYMPTR) MHOmfLock(pExState->cxt.hProc);
                if ((pSym->rectyp == S_LPROC32) ||
                    (pSym->rectyp == S_GPROC32)) {
                    if (((PROCPTR32)pSym)->pParent) { // nested?
                        HSYM hSLink32 = SHFindSLink32(&pExState->cxt);
                        CXT cxtChild;
                        ADDR addr = {0};
                        if (hSLink32) {
                            SLINK32* pSLink32 = (SLINK32*) MHOmfLock(hSLink32);
                            reg.hReg = pSLink32->reg;
                            if (reg.hReg != CV_ALPHA_IntSP && reg.hReg != CV_ALPHA_IntFP) {
                                if (GetReg(&reg, &pExState->cxt, NULL) == NULL) {
                                    DASSERT (FALSE);
                                } else {
                                    ul = reg.Byte8i;
                                }
                            }
                            if (pSLink32->off != 0) {
                                addr.addr.off = ul + pSLink32->off;
                                ADDR_IS_OFF32(addr) = TRUE;
                                ADDR_IS_FLAT(addr) = TRUE;
                                GetDebuggeeBytes(addr, sizeof(UOFF32), &ul, T_ULONG);
                            }
                            cxtChild = pExState->cxt;
                            cxtChild.hBlk = 0;
                            for (;;) {
                                CXT cxtParent;
                                HSYM parent = SHGoToParent(&cxtChild, &cxtParent);
                                if (parent) {
                                    SYMPTR pParent = MHOmfLock(parent);
                                    switch (pParent->rectyp) {
                                    case S_BLOCK16:
                                    case S_BLOCK32:
                                        cxtChild = cxtParent;
                                        continue;

                                    default:
                                        pParent = 0;
                                        break;
                                    }
                                    MHOmfUnLock(parent);
                                    if (!pParent) {
                                        break;
                                    }
                                }
                            }
#pragma message("Stack will always grow down, right?")
                            ul -= pSLink32->framesize;
                            MHOmfUnLock(hSLink32);
                        }
                    }
                }
                MHOmfUnLock(pExState->cxt.hProc);
            }
        } else
    	if ((TargetMachine == mptia64) &&
            (reg.hReg == CV_IA64_IntSp))
        {
            ADDR        addrBP;

            SYGetAddr(pExState->hframe, &addrBP, adrBase);

            ul = addrBP.addr.off;
            if (pExState->cxt.hProc) {
                SYMPTR pSym = (SYMPTR) MHOmfLock(pExState->cxt.hProc);
                if ((pSym->rectyp == S_LPROCIA64) ||
                    (pSym->rectyp == S_GPROCIA64)) {
                    if (((PROCPTRMIPS)pSym)->pParent) { // nested?
                        HSYM hSLink32 = SHFindSLink32(&pExState->cxt);
                        CXT cxtChild;
                        ADDR addr = {0};
                        if (hSLink32) {
                            SLINK32* pSLink32 = (SLINK32*) MHOmfLock(hSLink32);
                            reg.hReg = pSLink32->reg;
                            if (reg.hReg != CV_IA64_IntSp) {
                                if (GetReg(&reg, &pExState->cxt, NULL) == NULL) {
                                    DASSERT (FALSE);
                                } else {
                                    ul = reg.Byte4;
                                }
                            }
                            if (pSLink32->off != 0) {
                                addr.addr.off = ul + pSLink32->off;
                                ADDR_IS_OFF32(addr) = TRUE;
                                ADDR_IS_FLAT(addr) = TRUE;
                                GetDebuggeeBytes(addr, sizeof(UOFF32), &ul, T_ULONG);
                            }
                            cxtChild = pExState->cxt;
                            cxtChild.hBlk = 0;
                            for (;;) {
                                CXT cxtParent;
                                HSYM parent = SHGoToParent(&cxtChild, &cxtParent);
                                if (parent) {
                                    SYMPTR pParent = MHOmfLock(parent);
                                    switch (pParent->rectyp) {
                                    case S_BLOCK16:
                                    case S_BLOCK32:
                                        cxtChild = cxtParent;
                                        continue;

                                    case S_GPROCIA64:
                                    case S_LPROCIA64: //v-vadimp 64-bit?
                                        if (((PROCPTRIA64)pParent)->pParent) {
                                            addr.addr.off = ul - 4;
                                            cxtChild = cxtParent;
                                            ADDR_IS_OFF32(addr) = TRUE;
                                            ADDR_IS_FLAT(addr) = TRUE;
                                            GetDebuggeeBytes(addr, sizeof(UOFF32), &ul, T_ULONG);
                                            break;
                                        }
                                        // Fall through
                                    default:
                                        pParent = 0;
                                        break;
                                    }
                                    MHOmfUnLock(parent);
                                    if (!pParent) {
                                        break;
                                    }
                                }
                            }
#pragma message("Stack will always grow down, right?")
                            ul -= pSLink32->framesize;
                            MHOmfUnLock(hSLink32);
                        }
                    }
                }
                MHOmfUnLock(pExState->cxt.hProc);
            }

        } else if (GetReg (&reg, &pExState->cxt, NULL) == NULL) {
            DASSERT (FALSE);
        } else {
            ul = reg.Byte4;
            if (!ADDR_IS_OFF32(*SHpADDRFrompCXT(&pExState->cxt))) {
                ul &= 0xffff;
            }
        }
        EVAL_SYM_OFF (pv) += ul;
        EVAL_SYM_SEG (pv) = GetSegmentRegister(pExState->hframe, REG_SS);
        EVAL_IS_REGREL (pv) = FALSE;
        ADDR_IS_LI (EVAL_SYM (pv)) = FALSE;
        emiAddr (EVAL_SYM (pv)) = 0;
        SHUnFixupAddr (&EVAL_SYM (pv));
    }
    /*
     *  Fixup Thread local storage relative addresses.  This form is
     *  currently used by all platforms.
     */

    else if (EVAL_IS_TLSREL (pv)) {
        DASSERT(FALSE); // This EE treats THREAD32 like DATA32 and expects
                        // DM to "do the right thing"
        EVAL_IS_TLSREL( pv ) = FALSE;

        /*
         * Query the EM for the TLS base on this (current) thread
         */

        memset(&addr, 0, sizeof(ADDR));
        emiAddr( addr ) = emiAddr( EVAL_SYM( pv ));

//        SYGetAddr(&addr, adrTlsBase);

        EVAL_SYM_OFF( pv ) += GetAddrOff(addr);
        EVAL_SYM_SEG( pv ) = GetAddrSeg(addr);
        ADDR_IS_LI (EVAL_SYM( pv )) = ADDR_IS_LI (addr);
        emiAddr(EVAL_SYM( pv )) = 0;
        SHUnFixupAddr( &EVAL_SYM( pv ));
    }

  	else if (EVAL_IS_REGRELIA64(pv))
	{
		DASSERT(!"v-vadimp may need code for isregrelia64");
	}
	else if (EVAL_IS_REGIA64(pv))
	{
		DASSERT(!"v-vadimp may need code for isregia64");
	}

    return TRUE;
}                               /* ResolveAddr() */


/***    StartNodeOp - Get Start Node operator of a TM
 *
 *      OP =  StartNodeOp (hTM)
 *
 *      Entry   hTM = handle to TM
 *
 *      Exit    None
 *
 *      Returns OP_... operator found in root node of TM
 */

op_t
StartNodeOp (
    HTM hTM
    )
{
    pexstate_t  pTM;
    pstree_t    pTreeSav;
    op_t        retval;

    DASSERT (hTM);
    pTreeSav = pTree;
    pTM = (pexstate_t) MemLock(hTM);
    DASSERT (pTM->hSTree);
    pTree = (pstree_t) MemLock(pTM->hSTree);
    retval = NODE_OP((bnode_t) (pTree->start_node));
    pTree = pTreeSav;
    MemUnLock (pTM->hSTree);
    MemUnLock (hTM);
    return retval;
}


/***    LShiftCxtString - Left Shift Context String
 *
 *      void LShiftCxtString( hStr )
 *
 *      Entry   hStr = handle to expression string
 *
 *      Exit    Modifies expression string by shifting the
 *              first context string it encounters (i.e., a
 *              string enclosed in "{}") to the very left of
 *              the expression.
 *
 *      Returns void
 */

void
LShiftCxtString (
    EEHSTR hStr
    )
{
    LSZ     szExpr;
    HDEP    hBuf;
    char   *pBuf;
    char   *pStart;
    char   *pEnd;
    ulong   len;
    ulong   lenCxt;

    DASSERT (hStr);
    szExpr = (LSZ) MemLock (hStr);
    pStart = _tcschr (szExpr, '{');
    pEnd = _tcschr (szExpr, '}');

    if (pStart) {
        DASSERT (pEnd);
        // length of the expression at the left of the context
        len = (ulong)(pStart - szExpr);
        // length of the context string
        lenCxt = (ulong)(pEnd - pStart + 1);

        if ((hBuf = MemAllocate ( len + 1 )) != 0) {
            pBuf = (char *) MemLock (hBuf);

            // save leftmost part of the expression to temp. buffer
            // Then shift context to the left and copy saved part
            memcpy (pBuf, szExpr, len);
            memmove (szExpr, pStart, lenCxt);
            memcpy (szExpr + lenCxt, pBuf, len);

            MemUnLock (hBuf);
            MemFree (hBuf);
        }
        else {
            char ch;
            // shift context in place
            for (; pStart > szExpr; pStart--) {
                ch = * (pStart - 1);
                memmove (pStart - 1, pStart, lenCxt);
                * (pStart + lenCxt - 1) = ch;
            }
        }
    }
    MemUnLock ((HDEP) szExpr);
}



/***    GetParentSubtree - Get parent subtree
 *
 *      bnParent = GetParentSubtree(bnRoot, seTemplate)
 *
 *      Entry   bnRoot = based pointer to root of child eval tree
 *              seTemplate = SE_t template used for generating child expr.
 *
 *      Exit    none
 *
 *      Returns based pointer to subtree that corresponds to parent expr.
 */

bnode_t
GetParentSubtree (
    bnode_t bnRoot,
    SE_t seTemplate
    )
{
    bnode_t     bn = bnRoot;

    // skip optional initial context
    if (NODE_OP (bn) == OP_context)
        bn = NODE_LCHILD (bn);

    switch (seTemplate) {

    case SE_ptr:
        //  parent expression is identical with child expression
        break;

    case SE_deref:
        // Dolphin #8336:
        // DereferenceTM may generate two kinds of expressions
        //      "*(parent_expression)" or
        //       "(parent_expression)" for references only
        // In that case we shouldn't be checking for OP_fetch
        if (NODE_OP (bn) == OP_fetch)
            bn = NODE_LCHILD (bn);
        break;

    case SE_array:
        DASSERT ( NODE_OP (bn) == OP_lbrack );
        if (NODE_OP (bn) != OP_lbrack)
            break;
        bn = NODE_LCHILD (bn);
        break;

    case SE_member:
        DASSERT ( NODE_OP (bn) == OP_dot );
        if (NODE_OP (bn) != OP_dot)
            break;
        bn = NODE_LCHILD (bn);
        break;

    case SE_bclass:
        DASSERT (NODE_OP (bn) == OP_fetch);
        if (NODE_OP (bn) != OP_fetch)
            break;
        bn = NODE_LCHILD (bn);
        DASSERT (NODE_OP (bn) == OP_cast);
        if (NODE_OP (bn) != OP_cast)
            break;
        bn = NODE_RCHILD (bn);
        DASSERT (NODE_OP (bn) == OP_addrof);
        if (NODE_OP (bn) != OP_addrof)
            break;
        bn = NODE_LCHILD (bn);
        break;

    case SE_downcast:
        DASSERT (NODE_OP (bn) == OP_fetch);
        if (NODE_OP (bn) != OP_fetch)
            break;
        bn = NODE_LCHILD (bn);
        DASSERT (NODE_OP (bn) == OP_cast);
        if (NODE_OP (bn) != OP_cast)
            break;
        bn = NODE_RCHILD (bn);
        // skip optional context
        if (NODE_OP (bn) == OP_context)
            bn = NODE_LCHILD (bn);
        break;

    case SE_derefmember:
        // member obtained after dereferencing parent expr.
        //  (*(<parent_expr>)).<lszRule>
        DASSERT ( NODE_OP (bn) == OP_dot );
        if (NODE_OP (bn) != OP_dot)
            break;
        bn = NODE_LCHILD (bn);
        if (NODE_OP (bn) == OP_fetch)
            bn = NODE_LCHILD (bn);
        break;

    case SE_downcastmember:
        // member obtained after downcasting parent expr:
        //  {,,<foo.exe>}(*(<DClass>*){*}(<parent_expr>)).<lszRule>

        DASSERT ( NODE_OP (bn) == OP_dot );
        if (NODE_OP (bn) != OP_dot)
            break;
        bn = NODE_LCHILD (bn);
        DASSERT (NODE_OP (bn) == OP_fetch);
        if (NODE_OP (bn) != OP_fetch)
            break;
        bn = NODE_LCHILD (bn);
        DASSERT (NODE_OP (bn) == OP_cast);
        if (NODE_OP (bn) != OP_cast)
            break;
        bn = NODE_RCHILD (bn);
        // skip optional context
        if (NODE_OP (bn) == OP_context)
            bn = NODE_LCHILD (bn);
        break;


default:
        bn = 0;
        break;
    }

    return bn;
}


/***    LinkWithParentTM - Create a link between a child and parent TM
 *
 *      void LinkWithParentTM (hTM, hParentTM)
 *
 *      Entry   hTM = handle to child TM
 *              hParentTM = handle to parent TM
 *
 *      Exit    Links the child TM with the parent TM
 *
 *      Returns void
 */
void
LinkWithParentTM(
    HTM hTM,
    HTM hParentTM
    )
{
    pexstate_t pTM;
    pexstate_t pParentTM;

    DASSERT ( hTM );
    DASSERT ( hParentTM );
    pTM = (pexstate_t) MemLock (hTM);
    pParentTM = (pexstate_t) MemLock (hParentTM);

    pTM->hParentTM = hParentTM;
    (pParentTM -> nRefCount) ++;

    MemUnLock (hTM);
    MemUnLock (hParentTM);
}

/***    GetDerivedTM - Get TM derived from parent TM using an expansion rule
 *
 *      status = GetDerivedTM (hTMIn, lszRule, phTMOut)
 *
 *      Entry   hTMIN = handle to the parent TM
 *              lsz = string containing expansion rule
 *                  in the form: member_name
 *                  or:          member_name,format_specifier
 *              phTMOut = pointer to handle for the child TM
 *
 *      Exit    *phTMOut contains the handle of the child TM that was created
 *              The derived expression is one of the following:
 *
 *              a) if hTMIn is a ptr that needs no downcasting:
 *              (*(<parent_expr>)).<lszRule>
 *              b) if hTMIn is a ptr that can be downcast:
 *              {,,<foo.exe>}(*(<DClass>*){*}(<parent_expr>)).<lszRule>
 *              c) if hTMIn is a class:
 *              (<parent_expr>).<lszRule>
 *
 *      Returns EESTATUS
 */


EESTATUS
GetDerivedTM (
    HTM hTMIn,
    LSZ lszRule,
    HTM *phTMOut
    )
{
    uint        lenP;
    uint        len;
    uint        lenName;
    uint        lenCXT;
    uint        lenD;
    uint        cchExtra;
    char       *pch;
    EEHSTR      hName;
    EEHSTR      hDStr;
    LSZ         pName;
    LSZ         pDStr;
    LSZ         lsz;
    ulong       end;
    bool_t      fCase;
    EESTATUS    retval;
    SE_t        seTemplate;
    bool_t      fDeref = FALSE;
    bool_t      fDownCast = FALSE;
    bool_t      fSimpleIdent = TRUE;
    uint        index = 0;

    DASSERT (hTMIn);
    pExState = (pexstate_t) MemLock (hTMIn);


    // set flag if parent expression needs to be dereferenced
    fDeref = EVAL_IS_PTR (&pExState->result) &&
            !EVAL_IS_REF (&pExState->result);
    lenP = pExState->strIndex; //ignore parent's format string
    len = _tcslen (lszRule);
    // find the length of the name string contained in sz
    // (sz may contain a format string after the name string
    // delimited with a comma)
    lenName = ((pch = _tcschr (lszRule, _T(','))) != NULL) ?
        (uint)(pch - lszRule) : len;

    // check if the autoexpand name part is a simple identifier.
    // No MBCS considertions these should be valid C identifiers.
    for (index = 0; index < lenName; index++ )
    {
        char ch = lszRule[index];
        if (!isalnum(ch) && ch != '_')
        {
            fSimpleIdent = FALSE;
            break;
        }
    }

    // the derived expression will be:
    //  (*(<parent_expr>)).<lszRule> or
    //  {,,<foo.exe>}(*(<DClass>*){*}(<parent_expr>)).<lszRule>
    //  (<parent_expr>).<lszRule>
    // depending on whether the parent expression needs
    // to be dereferenced or downcast
    // The corresponding new strings contain 6 and 3 extra
    // characters respectively

    cchExtra = fDeref ? 6 : 3;
    if (pExState->hDClassName) {
        lsz = (char *) MemLock (pExState->hDClassName);
        lenD = _tcslen(lsz);
        MemUnLock (pExState->hDClassName);
        if (fDeref)
            cchExtra += lenD + 6;
    }
    if ((hName = MemAllocate ( lenName + 1 )) == NULL ||
        (hDStr = MemAllocate ( lenP + len + cchExtra + 1)) == NULL ) {
         pExState->err_num = ERR_NOMEMORY;
         if (hName)
            MemFree (hName);
         MemUnLock (hTMIn);
         return EEGENERAL;
    }

    pName = (char *) MemLock (hName);
    memcpy (pName, lszRule, lenName);
    *(pName + lenName) = 0;
    MemUnLock (hName);

    pDStr = (char *) MemLock (hDStr);
    pExStr = (char *) MemLock (pExState->hExStr);
    if (fDeref) {
        if (pExState->hDClassName == NULL) {
            //  (*(<parent_expr>)).<lszRule>
            seTemplate = SE_derefmember;
            memcpy (pDStr, "(*(", 3);
            memcpy (pDStr + 3, pExStr, lenP);
            memcpy (pDStr + 3 + lenP, ")).", 3);
            memcpy (pDStr + 6 + lenP, lszRule, len);
            * (pDStr + 6 + lenP + len) = 0;
        }
        else {
            fDownCast = TRUE;
            // {,,<foo.exe>}(*(<DClass>*)(<parent_expr>)).<lszRule>
            lsz = (char *) MemLock (pExState->hDClassName);
            if ((pch = _tcschr (lsz, '}'))  != NULL)
                pch ++;
            else
                pch = lsz;
            lenCXT = (uint)(pch - lsz);
            seTemplate = SE_downcastmember;
            pch = pDStr;
            memcpy (pch, lsz, lenCXT);
            pch += lenCXT;
            memcpy (pch, "(*(", 3);
            pch += 3;
            memcpy (pch, lsz + lenCXT, lenD - lenCXT);
            pch += lenD - lenCXT;
            memcpy (pch, "*){*}(", 6);
            pch += 6;
            memcpy (pch, pExStr, lenP);
            pch += lenP;
            memcpy (pch, ")).", 3);
            pch += 3;
            memcpy (pch, lszRule, len);
            pch += len;
            *pch = 0;
            MemUnLock (pExState->hDClassName);
        }
    }
    else {
        //  (<parent_expr>).<sz>

        seTemplate = SE_member ;
        * pDStr = '(';
        memcpy (pDStr + 1, pExStr, lenP);
        memcpy (pDStr + 1 + lenP, ").", 2);
        memcpy (pDStr + 3 + lenP, lszRule, len);
        * (pDStr + 3 + lenP + len) = 0;
    }
    MemUnLock (pExState->hExStr);
    MemUnLock (hDStr);

    // if we have a complex auto-expand rule such as a.b->c don't set the template type.
    // The relation between the in-coming TM and the resulting TM in this case cannot be expressed
    // as a single operation.

    if (!fSimpleIdent) {
        seTemplate = SE_totallynew ;
    }

    if ( !fDownCast && OP_context == StartNodeOp (hTMIn)) {
        // if the parent expression contains a global context
        // shift the context string to the very left of
        // the child expression (so that this becomes a
        // global context of the child expression too)
        LShiftCxtString ( hDStr );
    }

    fCase = pExState->state.fCase;
    MemUnLock (hTMIn);

    retval = ParseBind (hDStr, hTMIn, phTMOut, &end,
                BIND_fSupOvlOps, fCase);

    if (EENOERROR == retval) {
        pExState = (pexstate_t) MemLock (*phTMOut);
        pExState->state.childtm = TRUE;
        pExState->state.autoexpand = TRUE;
        pExState->hCName = hName;
        if ((pExState->seTemplate = seTemplate) != SE_totallynew) {
            // don't increment parent TM's ref count
            // since this TM has the same lifetime with its parent
            pExState->hParentTM = hTMIn;
        }
        MemUnLock (*phTMOut);
    }
    else {
        MemFree (hName);
        if (*phTMOut)
            EEFreeTM (phTMOut);
    }

    return retval;
}

uint
uSkipLfOneMethod(
    plfOneMethod plf
    )
{
    uint uVbase = fIntroducingVirtual(plf->attr.mprop) ? sizeof(long) : 0;
    return sizeof (struct lfOneMethod) + 1 + uVbase + ((char *)(plf->vbaseoff))[uVbase];
}

char *
pbNameInLfOneMethod(
    plfOneMethod plf
    )
{
    return (char *)plf + offsetof(lfOneMethod, vbaseoff) +
        (fIntroducingVirtual(plf->attr.mprop) ? sizeof(long) : 0);
}


SEGMENT GetSegmentRegister(HFRAME hframe, int iWhich)
{
    SHREG       spReg;

    if (TargetMachine != mptix86) {
        return 0;
    }

    spReg.hReg = iWhich;
    GetReg(&spReg, NULL, hframe);
    return spReg.Byte2;
}
