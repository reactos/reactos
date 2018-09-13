/***    debutil.c - Expression evaluator utility routines
 *
 */

#include "debexpr.h"
#include "debsym.h"


#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY


void CheckFcnArgs (neval_t, HTYPE *, CV_call_e);
bool_t GrowStack (uint);
INLINE void SetDPtr (neval_t, HTYPE *);

char size_special[8]  = { 0,  2,  2,  0,  8,  0,  0,  0};
char size_special2[8] = { 0,  0,  0,  0,  0,  0,  0,  0};
char size_integral[8] = { 1,  2,  4,  8,  0,  0,  0,  0};
char size_real[8]     = { 4,  8, 10, 16,  6,  0,  0,  0};
char size_ptr[8]      = { 0,  2,  4,  4,  4,  6,  0,  0};
char size_int[8]      = { 1,  2,  2,  2,  4,  4,  8,  8};


#define MAX_STACKLEN 0x8000L


 /**    PushStack - push bind data onto stack
 *
 *      fSuccess = PushStack (pv);
 *
 *      Entry   pv = pointer to evaluation entry
 *
 *      Exit    pv->vflags pushed onto evaluation stack
 *
 *      Returns TRUE if entry pushed
 *              FALSE if error in push
 */

bool_t
PushStack (
    peval_t pv
    )
{
    uint        len;
    pelem_t     pEL;
    pelem_t     pELP;

    DASSERT ((ST == NULL) || (((char *)ST > (char *)pEStack) &&
      ((char *)ST < (char *)pEStack + (UINT_PTR)StackOffset)));

    DASSERT ((STP == NULL) || (((char *)STP > (char *)pEStack) && (STP < ST)));

    if (EVAL_STATE(pv) == EV_type)
        len = sizeof(elem_t);
    else {
        len = sizeof (elem_t) +
          max (EVAL_VALLEN (pv), sizeof (val_t)) - sizeof (val_t);
    }

    if (((UINT_PTR)StackLen - (UINT_PTR)StackOffset) < len)  {
        if (!GrowStack (len)) {
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
    }
    if (ST == NULL) {
        // first element onto stack and set based pointer to previous element
        // to 0xffff which indicates null

        pEL = (pelem_t)pEStack;
        pELP = NULL;
        pEL->pe = (belem_t)0xffff;
    }
    else {
        pEL  = (pelem_t)(((char *)pEStack) + (UINT_PTR)StackOffset);
        // don't use sizeof(belem_t) to adjust--assumes that se is
        // directly after pe, untrue for -Zp8!
        pELP =
            (pelem_t)
            (
                ((char *)ST) -
                (offsetof(elem_t, se) - offsetof(elem_t, pe))
            );
        pEL->pe = (belem_t)(pELP);
    }

    StackOffset = (belem_t)(((UINT_PTR)StackOffset) + len);
    StackMax    = (belem_t)(max ((UINT_PTR)StackMax, (UINT_PTR)StackOffset));

    STP = ST;
    ST = (peval_t)&pEL->se;

    DASSERT (ST != NULL);
    DASSERT ((char *)ST > (char *)pEStack);
    DASSERT ((char *)ST < ((char *)pEStack) + (UINT_PTR)StackOffset);
    DASSERT (STP == NULL || (((char *)STP > (char *)pEStack) && (STP < ST)));


    *ST = *pv;

    return (TRUE);
}




/**     PopStack - pop bind data from stack
 *
 *      fSuccess = PopStack (void);
 *
 *      Entry   none
 *
 *      Exit    stack popped by one
 *
 *      Returns TRUE if stack popped
 *              FALSE if error in pop
 */


bool_t
PopStack (
    void
    )
{
    pelem_t     pEL;
    UINT_PTR    bELP;

    DASSERT (ST != NULL);
    DASSERT ((ST == NULL) || (((char *)ST > (char *)pEStack) &&
      ((char *)ST < (char *)pEStack + (UINT_PTR)StackOffset)));
    DASSERT ((STP == NULL) || (((char *)STP > (char *)pEStack) && (STP < ST)));
    if (ST == NULL) {
        pExState->err_num = ERR_INTERNAL;
        return (FALSE);
    }
    else {
        // determine the beginning of the current stack element and
        // reset the stack offset to the beginning of the top stack element

        if (STP != NULL) {
            DASSERT (((char *)STP > (char *)pEStack) && (STP < ST));
        }
        pEL = (pelem_t)((char *)ST - offsetof (elem_t, se));

        // set the based pointer to the previous stack element

        bELP = (UINT_PTR)pEL->pe;
        if (bELP == 0xffff) {
            // we are popping off the only stack element

            StackOffset = 0;
            STP = NULL;
            ST = NULL;
        }
        else if (bELP == 0) {
            // we are popping to the last stack element

            StackOffset = (belem_t)((char *)pEL - (char *)pEStack);
            STP = NULL;
            ST = (peval_t)&((pelem_t)pEStack)->se;
        }
        else {
            StackOffset = (belem_t)((char *)pEL - (char *)pEStack);
            ST = STP;
            pEL = (pelem_t)((char *)ST - offsetof (elem_t, se));
            bELP = (UINT_PTR)pEL->pe;
            STP = (peval_t)&((pelem_t)(((char *)pEStack) + bELP))->se;
        }
        DASSERT ((ST == NULL) || (((char *)ST > (char *)pEStack) &&
          ((char *)ST < (char *)pEStack + (UINT_PTR)StackOffset)));
        DASSERT ((STP == NULL) || (((char *)STP > (char *)pEStack) && (STP < ST)));
        return (TRUE);
    }
}




/**     CkPointStack - checkpoint stack position
 *
 *      CkPointStack (void);
 *
 *      Entry   none
 *
 *      Exit    stack position saved in StackCkPoint
 *
 *      Returns nothing
 */


void
CkPointStack (
    void
    )
{
    StackCkPoint = StackOffset;
}




/**     ResetStack - reset stack to checkpoint position
 *
 *      fSuccess = CkPointStack (void);
 *
 *      Entry   StackCkPoint = checkpointed position
 *
 *      Exit    stack position reset
 *
 *      Returns TRUE if successfully reset
 *              FALSE if error
 */


bool_t
ResetStack (
    void
    )
{
    while (StackOffset > StackCkPoint) {
        if (PopStack () == FALSE) {
            return (FALSE);
        }
    }
    if (StackOffset != StackCkPoint) {
        return (FALSE);
    }
    return (TRUE);
}




/**     GrowStack - grow evaluation stack and reset pointers
 *
 *      fSuccess = GrowStack (len);
 *
 *      Entry   len = minimum increase size
 *
 *      Exit    stack grown
 *
 *      Returns TRUE if stack grown
 *              FALSE if error
 */


bool_t
GrowStack (
    uint len
    )
{
    UINT_PTR    bST = (UINT_PTR)-1;
    UINT_PTR    bSTP = (UINT_PTR)-1;
    HDEP        hNS;        // handle of new evaluation stack
    pelem_t     pNS;        // address of new evaluation stack
    uint        size;

    // convert current stack pointers to based form

    if (ST != NULL) {
        bST = (uchar *)ST - (uchar *)pEStack - offsetof (elem_t, se);
    }
    if (STP != NULL) {
        bSTP = (uchar *)STP - (uchar *)pEStack - offsetof (elem_t, se);
    }

    // allocate new evaluation stack and copy old to new

    size = (uint) max ((UINT_PTR)StackLen + 5 * sizeof (elem_t), (UINT_PTR)StackLen + len);

    if (size > MAX_STACKLEN || (hNS = MHMemAllocate (size)) == 0) {
        // if no memory
        pExState->err_num = ERR_NOMEMORY;
        return (FALSE);
    } else {
        pNS = (pelem_t) MemLock (hNS);
        memcpy (pNS, pEStack,
          (size_t)((char *)(pelem_t)StackOffset - (char *)pEStack));

        // if old stack was not the standard fixed buffer, release it

        MemUnLock (hEStack);
        MHMemFree (hEStack);
        hEStack = hNS;
        pEStack = pNS;
        if (bST != 0xffff) {
            ST = (peval_t)&((pelem_t)(((char *)pEStack) + bST))->se;
        }
        if (bSTP != 0xffff) {
            STP = (peval_t)&((pelem_t)(((char *)pEStack) + bSTP))->se;
        }
        StackLen = (belem_t)size;
        return (TRUE);
    }
}


/*      RNumLeaf - read numeric leaf
 *
 *      value = RNumLeaf (
 *      Read numeric leaf and return value and size of leaf
 */


UQUAD
RNumLeaf (
    void *pleaf,
    uint *skip
    )
{
    ushort  val;

    if ((val = ((plfEasy)pleaf)->leaf) < LF_NUMERIC) {
        // No leaf can have an index less than LF_NUMERIC (0x8000) so word is value
        *skip += 2;
        return (val);
    }

    switch (val) {
        case LF_CHAR:
            *skip += 3;
            return (((plfChar)pleaf)->val);

        case LF_USHORT:
            *skip += 4;
            return (((plfUShort)pleaf)->val);

        case LF_SHORT:
            *skip += 4;
            return (((plfShort)pleaf)->val);

        case LF_LONG:
        case LF_ULONG:
            *skip += 6;
            return (((lfULong UNALIGNED *)pleaf)->val);

        case LF_QUADWORD:
        case LF_UQUADWORD:
            *skip += 10;
            return *(UQUAD *)&(((lfUQuad UNALIGNED *)pleaf)->val);

        default:
            DASSERT (FALSE);
            return (0L);
    }
}




/**     fnCmp - name compare routine.
 *
 *      Compares the name described by the hInfo packet with a length
 *      prefixed name
 *
 *      fFlag = fnCmp (psearch_t pName, SYMPTR pSym, char *stName, int fCase);
 *
 *      Entry   pName = pointer to psearch_t packet describing name
 *              pSym = pointer to symbol structure (NULL if internal call)
 *              stName = pointer to a length preceeded name
 *                      can be NULL
 *              fCase = TRUE if case sensitive search
 *                      FALSE if case insensitive search
 *
 *      Exit    pName->lastsym = type of symbol checked
 *
 *      Returns 0 if names compared equal
 *              non-zero if names did not compare
 */


bool_t
fnCmp (
    psearch_t pName,
    SYMPTR pSym,
    char const *stName,
    UINT fCase
    )
{
    int cbstName;
    DASSERT (pName != NULL);

    if (stName == NULL || stName[0] == 0) {
        return (1);
    }

    if (pName->sstr.searchmask & SSTR_RE) {
        // this is regular expression search

        pName->lastsym = pSym->rectyp;
        if (pName->sstr.pRE != NULL) {
            char buffer [256];
            char * lpBuffer = buffer;

            // stName is a length prefixed string.  convert to a
            // null terminated string
            _tcsncpy ( lpBuffer, stName+1, (unsigned char)stName[0] );
            lpBuffer [ (unsigned char)stName[0] ] = '\0';

            // do the compare using lpBuffer
            return (SHCompareRE ( lpBuffer, (char *)pName->sstr.pRE, fCase));
        }
        else {
            // return match if no regular expression specified

            return (0);
        }
    }

    // CV400 #2863 -- dtor name will not match with base class name
    // if we're in the one letter dtor name case then match any base
    // classes dtor name

    if (pName->sstr.cb == 1 &&
        pName->sstr.lpName[0] == '~' &&
        stName[1] == '~') return 0;

    // strings do not compare if lengths are different

    cbstName = *((unsigned char *)stName)++;
    if (pName->sstr.cb == (uchar)cbstName ||
        pName->sstr.searchmask & SSTR_FuzzyPublic) {

        int cmpflag;

        // Lengths are the same
        if (pSym != NULL) {
            // save type of last symbol checked
            pName->lastsym = pSym->rectyp;
        } else {
            // try the normal match, don't try dereferencing pSym since it is NULL...
            goto domatch;
        }
        if (pName->sstr.searchmask & SSTR_proc) {
            CV_typ_t type;

            // we are checking only procs with the correct type
            // If this flag is set, then the initializer set the desired
            // proc index into pName->typeOut

            switch (pSym->rectyp) {
                case S_LPROC16:
                case S_GPROC16:
                    type = ((PROCPTR16)pSym)->typind;
                    break;

                case S_LPROC32:
                case S_GPROC32:
                    type = ((PROCPTR32)pSym)->typind;
                    break;

                case S_LPROCMIPS:
                case S_GPROCMIPS:
                    type = ((PROCPTRMIPS)pSym)->typind;
                    break;

                case S_LPROCIA64:
                case S_GPROCIA64:
                    type = ((PROCPTRIA64)pSym)->typind;
                    break;
            }
            if (BindingBP == TRUE) {
                // REVIEW:HACK if only one method present, type index is immaterial
                // this fixes the case where we have duplicate types and the type index
                // in the symbol record is out of the wrong set of type indices.
#if CC_LAZYTYPES
                if ( THAreTypesEqual( pName->hMod, type, pName->typeOut ) ||
                    (pName->overloadCount == 1 && fCase))
#else
                if (type ==  pName->typeOut || (pName->overloadCount == 1 && fCase))
#endif
                {
                    // we have an exact match on type,
                    // or the type index is immaterial
                    goto domatch;
                }
                if (pName->FcnBP == 0) {
                    // save alternate type so we can search again
                    pName->FcnBP = type;
                }
            }
            // REVIEW:HACK see above
#if CC_LAZYTYPES
            else if ( THAreTypesEqual( pName->hMod, type, pName->typeOut ) ||
                (pName->overloadCount == 1 && fCase))
#else
            else if (type ==  pName->typeOut || (pName->overloadCount == 1 && fCase))
#endif
            {
                // we have an exact match on type
                goto domatch;
            }
            return (1);
        }
        else if (pName->sstr.searchmask & SSTR_data) {
            // we are checking only global data with the correct type
            // If this flag is set, then the initializer set the desired
            // proc index into pName->typeOut

            switch (pSym->rectyp) {
                case S_GDATA16:
#if CC_LAZYTYPES
                    if ( !THAreTypesEqual( pName->hMod, ((DATAPTR16)pSym)->typind, pName->typeOut ) )
#else
                    if (((DATAPTR16)pSym)->typind != pName->typeOut)
#endif
                    {
                        return (1);
                    }
                    break;

                case S_GDATA32:
                case S_GTHREAD32:
#if CC_LAZYTYPES
                    if ( !THAreTypesEqual( pName->hMod, ((DATAPTR32)pSym)->typind, pName->typeOut ) )
#else
                    if (((DATAPTR32)pSym)->typind != pName->typeOut)
#endif
                    {
                        return (1);
                    }
                    break;
            }
        }

    domatch:
        if (pName->sstr.cb != cbstName) {
            cmpflag = 1;
        } else if (fCase == TRUE) {
            cmpflag = _tcsncmp ((char *)pName->sstr.lpName, stName, cbstName);
        } else {
            cmpflag = _tcsnicmp ((char *)pName->sstr.lpName, stName, cbstName);
        }

        if (cmpflag && (pName->sstr.searchmask & SSTR_FuzzyPublic)) {
            char * ppatAlloc = MemAllocate(cbstName + 1);
            char * ppat = ppatAlloc;
            char * ptemp;
            BOOL doit = FALSE;

            strncpy(ppat, stName, cbstName);
            ppat[cbstName] = 0;

            // strip 1 _
            if (*ppat == '_') {
                ppat++;
                doit = TRUE;
            }
            // or two ..
            else if (ppat[0] == '.' && ppat[1] == '.') {
                ppat += 2;
                doit = TRUE;
            }

            //
            // search for @
            //
            ptemp = _tcschr(ppat, '@');
            if (ptemp) {
                *ptemp = '\0';
                doit = TRUE;
            }

            if (doit && _tcslen(ppat) == pName->sstr.cb) {
                if (fCase == TRUE) {
                    cmpflag = _tcsncmp (pName->sstr.lpName, ppat, pName->sstr.cb);
                }
                else {
                    cmpflag = _tcsnicmp (pName->sstr.lpName, ppat, pName->sstr.cb);
                }
            }

            MemFree(ppatAlloc);
        }
        return cmpflag;

    }
    return (1);
}


#ifdef NEVER
/**     tdCmp - typedef compare routine.
 *
 *      Compares the type described by the hInfo packet with a typedef symbol
 *
 *      fFlag = tdCmp (psearch_t pName, SYMPTR pSym, char far *stName, int fCase);
 *
 *      Entry   pName = pointer to psearch_t packet describing name
 *              pSym = pointer to symbol structure (NULL if internal call)
 *              stName = pointer to a length preceeded name
 *              fCase = ignored
 *
 *      Exit    none
 *
 *      Returns 0 if typedef symbol of proper type found
 */


SHFLAG
tdCmp (
    psearch_t pName,
    SYMPTR pSym,
    char *stName,
    SHFLAG fCase
    )
{
    Unreferenced( stName );
    Unreferenced( fCase );

    DASSERT (pName != NULL);
    DASSERT (pSym != NULL);

    // strings do not compare if lengths are different

    if ((pSym->rectyp == S_UDT) && (((UDTPTR)pSym)->typind == pName->typeIn)) {
        pName->lastsym = pSym->rectyp;
        return (0);
    }
    return (1);
}
#endif



/**     csCmp - compile symbol compare routine.
 *
 *      Compares the type described by the hInfo packet with a compile symbol
 *
 *      fFlag = csCmp (psearch_t pName, SYMPTR pSym, char *stName, int fCase);
 *
 *      Entry   pName = pointer to psearch_t packet describing name
 *              pSym = pointer to symbol structure (NULL if internal call)
 *              stName = pointer to a length preceeded name
 *              fCase = ignored
 *
 *      Exit    none
 *
 *      Returns 0 if compile symbol found
 */

bool_t
csCmp (
    psearch_t pName,
    SYMPTR pSym,
    char const *stName,
    UINT fCase
    )
{
    Unreferenced( stName );
    Unreferenced( fCase );

    DASSERT (pName != NULL);
    DASSERT (pSym != NULL);

    // strings do not compare if lengths are different

    if (pSym->rectyp == S_COMPILE) {
        pName->lastsym = pSym->rectyp;
        return (0);
    }
    return (1);
}


/***    InsertNode - Insert node in parse tree
 *
 *      error = InsertNode (ptok)
 *
 *      Entry   pExState = address of expression state structure
 *              pExState->hSTree locked
 *
 *      Exit    pExState->hSTree locked
 *              pTree = address of locked syntax tree
 *
 *      Returns TRUE if successful
 *              FALSE if unsuccessful
 *
 */

bool_t
InsertNode (
    void
    )
{
    return (FALSE);
}


/**     RemoveIndir - Remove a level of indirection from a node
 *
 *      RemoveIndir (pv)
 *
 *      Entry   pv = pointer to value
 *
 *      Exit
 *
 *      Returns none
 *
 * DESCRIPTION
 *       Strips a level of indirection from a node's type; thus (char **)
 *       becomes (char *), and (char *) becomes (char).
 */


void
RemoveIndir (
    peval_t pv
    )
{
    CV_typ_t    typ;

    // Initialization, error checking.  Find the base type of the
    // pointer type.

    DASSERT (EVAL_IS_PTR (pv));

    // Since we are removing a level of indirection on the pointer, the
    // value can no longer be in a register so we clear the flag indicating
    // the value is in a register

    EVAL_IS_REG (pv) = FALSE;

    // dolphin #5783: In addition, clear the islabel flag, since
    // the new node in not necessarily a label

    EVAL_IS_LABEL (pv) = FALSE;

    //  Set the type of the node to the underlying type

    if (CV_IS_PRIMITIVE (EVAL_TYP (pv))) {
        switch (EVAL_TYP (pv)) {
            case T_FCVPTR:
            case T_HCVPTR:
            case T_32FCVPTR:
                EVAL_SYM_SEG (pv) = EVAL_PTR_SEG (pv);
                // fall through

            case T_32NCVPTR:
            case T_64NCVPTR:
            case T_NCVPTR:
                typ = PTR_UTYPE (pv);
                break;

            default:
                typ = CV_NEWMODE (EVAL_TYP (pv), CV_TM_DIRECT);
                break;
        }
    }
    else if (EVAL_IS_ARRAY (pv)) {
        typ = PTR_UTYPE (pv);
    }
    else if (EVAL_IS_PTR (pv)) {
        typ = PTR_UTYPE (pv);
        if (EVAL_PTRTYPE (pv) != CV_PTR_NEAR &&
            EVAL_PTRTYPE (pv) != CV_PTR_NEAR32) {
            EVAL_SYM_SEG (pv) = EVAL_PTR_SEG (pv);
        }
    }
    else {
        DASSERT (FALSE);
        return;
    }
    SetNodeType (pv, typ);
}




/***    SetNodeType - set node flags for a type index
 *
 *      fSuccess = SetNodeType (pv, type)
 *
 *      Entry   pv = pointer to value
 *              type = type index
 *
 *      Exit    EVAL_TYPDEF (pv) = handle to typedef record if not primitive
 *              pv->flags set for type
 *              pv->data set for type
 *
 *      Returns TRUE if type flags set
 *              FALSE if not (invalid type)
 */

INLINE HTYPE
GetHTypeFromTindex (
    peval_t pv,
    CV_typ_t type
    )
{
    EVAL_TYPDEF (pv) = THGetTypeFromIndex (EVAL_MOD (pv), type);
    // DASSERT (EVAL_TYPDEF (nv) != 0);
    return (EVAL_TYPDEF (pv));
}

bool_t
SetNodeType (
    peval_t pv,
    CV_typ_t type
    )
{
    plfEasy         pType;
    uint            skip;
    bool_t          retflag = TRUE;
    HTYPE           hType;
    CV_typ_t        oldType;
    CV_call_e       call;
    CV_modifier_t   cvol;
    CV_ptrmode_e    mode;
    SYMPTR          pSym;
    static  uchar   cvptr[6] = {CV_PTR_NEAR, CV_PTR_FAR, CV_PTR_HUGE, CV_PTR_NEAR32, CV_PTR_FAR32, CV_PTR_64};
    search_t        Name;
    psearch_t       pName = &Name;
    eval_t          eval, savedeval;
    peval_t         lpv     = &eval;
    peval_t         lpsaved = &savedeval;
    DTI             dti;
    uint            iregSav;
    bool_t          hibyteSav;
    eval_t          tmpeval;

    // M00FUTURE: Clearing out the reg field is a temporary
    // hack until the register handling code is expanded and
    // takes advantage of the separate reg field.
    //memset (&pv->reg, 0, sizeof (pv->reg));

    tmpeval = *pv;
    CLEAR_EVAL_FLAGS (pv);
    if (EVAL_IS_REG (&tmpeval)) {

        // an enregistered variable
        EVAL_IS_REG (pv) = TRUE;
        // save register information before clearing data
        iregSav = EVAL_REG(pv);
        hibyteSav = EVAL_IS_HIBYTE (pv);
    }
    else if (EVAL_IS_BPREL (&tmpeval)) {
        EVAL_IS_BPREL (pv) = TRUE;
    }
    else if (EVAL_IS_LABEL (&tmpeval)) {  // CUDA #4067: must preserve islabel bit
        EVAL_IS_LABEL (pv) = TRUE;
    }
    else if (EVAL_IS_REGREL(&tmpeval)) {
        EVAL_IS_REGREL(pv) = TRUE;
    }
    else if (EVAL_IS_TLSREL(&tmpeval)) {
        EVAL_IS_TLSREL(pv) = TRUE;
    }
modifier:
    oldType = EVAL_TYP (pv);
    EVAL_TYP (pv) = type;
    if (!CV_IS_PRIMITIVE (type)) {
        DASSERT (EVAL_MOD (pv) != 0);
        if ((hType = GetHTypeFromTindex (pv, type)) == 0) {
            return FALSE;
        }
    }

    // from this point, it is assumed that a value of FALSE is zero and
    // the memset of the node set all bit values to FALSE

    if (CV_IS_INTERNAL_PTR (type)) {
        // we are creating a special pointer to class type

        if (oldType == T_NOTYPE) {
            DASSERT (FALSE);
            return (FALSE);
        }
        EVAL_IS_ADDR (pv) = TRUE;
        EVAL_IS_PTR (pv) = TRUE;
        EVAL_IS_DPTR (pv) = TRUE;
        // EVAL_IS_CONST (pv) = FALSE;
        // EVAL_IS_VOLATILE (pv) = FALSE;
        // EVAL_IS_REF (pv) = FALSE;
        PTR_UTYPE (pv) = oldType;

        // The following code assumes that the ordering of the
        // pointer modes is the same as the ordering of the CV created
        // pointer types

        EVAL_PTRTYPE (pv) = cvptr[CV_MODE (type) - CV_TM_NPTR];
    }
    else if (CV_IS_PRIMITIVE (type)) {

        //  If the type is primitive then it must reference data

        EVAL_IS_DATA (pv) = TRUE;
        EVAL_IS_DPTR (pv) = TRUE;
        if (CV_TYP_IS_PTR (type)) {

            // can't cast from 32 bit ptr to a 16 or vice versa

            if ( EVAL_IS_PTR (pv) )
            {
                if (EVAL_PTRTYPE(pv) == CV_PTR_NEAR32 ||
                    EVAL_PTRTYPE(pv) == CV_PTR_FAR32)
                {
                    if (CV_MODE (type) < CV_TM_NPTR32)
                    {
                        return (FALSE);
                    }
                }
                else
                {
                    if (CV_MODE (type) >= CV_TM_NPTR32)
                    {
                        return (FALSE);
                    }
                }
            }

            EVAL_IS_PTR (pv) = TRUE;
            EVAL_IS_ADDR (pv) = TRUE;

            // The following code assumes that the ordering of the
            // pointer modes is the same as the ordering of the CV created
            // pointer types

            EVAL_PTRTYPE (pv) = cvptr[CV_MODE (type) - CV_TM_NPTR];
            PTR_UTYPE (pv) = CV_NEWMODE(type, CV_TM_DIRECT);
        }
    } else {
        memset (&pv->data, 0, sizeof (pv->data));
        // MOOFUTURE: This may change as new register-handling
        // code is developed.
        // "reg" used to be a union member of "data" but now
        // it is a separate struct in the evaluation node (so
        // that we can keep type information for non-trivial
        // enregistered values). For the time, clear the reg
        // field whenever the data field is cleared, in order to
        // be compatible with the old EE register handling code
        memset (&pv->reg, 0, sizeof (pv->reg));
        pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
        switch (pType->leaf) {
            case LF_NULL:
                break;

            case LF_CLASS:
            case LF_STRUCTURE:
                if (((plfClass)pType)->property.fwdref) {
                    skip = offsetof (lfClass, data);
                    RNumLeaf (((char *)(&pType->leaf)) + skip, &skip);
                    // forward ref - look for the definition of the UDT
                    if ((type = GetUdtDefnTindex (type, pv, ((char *)&(pType->leaf)) + skip)) == T_NOTYPE) {
                        retflag = FALSE;
                        break;
                    }
                    MHOmfUnLock (hType);
                    if ((hType = GetHTypeFromTindex (pv, type)) == 0) {
                        retflag = FALSE;
                        break;
                    }
                    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
                    EVAL_TYP (pv) = type;
                }
                if (((plfClass)pType)->property.fwdref) {
                    retflag = FALSE;
                    break;
                }
                EVAL_IS_DATA (pv) = TRUE;
                EVAL_IS_CLASS (pv) = TRUE;
                CLASS_COUNT (pv) = ((plfClass)pType)->count;
                CLASS_FIELD (pv) = ((plfClass)pType)->field;
                CLASS_DERIVE (pv) = ((plfClass)pType)->derived;
                CLASS_VTSHAPE (pv) = ((plfClass)pType)->vshape;
                CLASS_PROP (pv) = ((plfClass)pType)->property;
                skip = offsetof (lfClass, data);
                CLASS_LEN (pv) = (ulong) RNumLeaf (((char *)(&pType->leaf)) + skip, &skip);
                break;

            case LF_UNION:
                if (((plfUnion)pType)->property.fwdref) {
                    skip = offsetof (lfUnion, data);
                    RNumLeaf (((char *)(&pType->leaf)) + skip, &skip);
                    // forward ref - look for the definition of the UDT
                    if ((type = GetUdtDefnTindex (type, pv, ((char *)&(pType->leaf)) + skip)) == T_NOTYPE) {
                        retflag = FALSE;
                        break;
                    }
                    MHOmfUnLock (hType);
                    if ((hType = GetHTypeFromTindex (pv, type)) == 0) {
                        retflag = FALSE;
                        break;
                    }
                    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
                    EVAL_TYP (pv) = type;
                }
                if (((plfUnion)pType)->property.fwdref) {
                    retflag = FALSE;
                    break;
                }
                EVAL_IS_DATA (pv) = TRUE;
                EVAL_IS_CLASS (pv) = TRUE;
                CLASS_COUNT (pv) = ((plfUnion)pType)->count;
                CLASS_FIELD (pv) = ((plfUnion)pType)->field;
                CLASS_PROP (pv) = ((plfUnion)pType)->property;
                skip = offsetof (lfUnion, data);
                CLASS_LEN (pv) = (ulong) RNumLeaf (((char *)(&pType->leaf)) + skip, &skip);
                break;

            case LF_ENUM:
                if (((plfEnum)pType)->property.fwdref) {
                    // forward ref - look for the definition of the UDT
                    if ((type = GetUdtDefnTindex (type, pv, (char *)&(((plfEnum)pType)->Name[0]))) == T_NOTYPE) {
                        retflag = FALSE;
                        break;
                    }
                    MHOmfUnLock (hType);
                    if ((hType = GetHTypeFromTindex (pv, type)) == 0) {
                        retflag = FALSE;
                        break;
                    }
                    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
                    EVAL_TYP (pv) = type;
                }
                if (((plfEnum)pType)->property.fwdref) {
                    retflag = FALSE;
                    break;
                }
                EVAL_IS_ENUM (pv) = TRUE;
                ENUM_COUNT (pv) = ((plfEnum)pType)->count;
                ENUM_FIELD (pv) = ((plfEnum)pType)->field;
                ENUM_UTYPE (pv) = ((plfEnum)pType)->utype;
                ENUM_PROP (pv) = ((plfEnum)pType)->property;
                break;


            case LF_BITFIELD:
                EVAL_IS_DATA (pv) = TRUE;
                EVAL_IS_BITF (pv) = TRUE;
                skip = 1;

                // read number of bits in field

                BITF_LEN (pv) = ((plfBitfield)pType)->length;
                BITF_POS (pv) = ((plfBitfield)pType)->position;

//BUGBUG: this looks suspicious to me

                if (TargetMachine != mptmppc)
                {
                    // adjust off and pos so we can do a byte op if need be
                    // sps 11/16/92
                    EVAL_SYM_OFF(pv) += BITF_POS(pv) / 8;
                    BITF_POS(pv) = BITF_POS(pv) % 8;
                }

                BITF_UTYPE (pv) = ((plfBitfield)pType)->type;
                skip = sizeof (lfBitfield);
                break;


            case LF_POINTER:

                EVAL_IS_ADDR (pv) = TRUE;
                EVAL_IS_PTR (pv) = TRUE;
                EVAL_IS_CONST (pv) = ((plfPointer)pType)->attr.isconst;
                EVAL_IS_VOLATILE (pv) = ((plfPointer)pType)->attr.isvolatile;
                mode = (CV_ptrmode_e) ((plfPointer)pType)->attr.ptrmode;
                PTR_UTYPE (pv) = ((plfPointer)&(pType->leaf))->utype;
                if (!CV_IS_PRIMITIVE (PTR_UTYPE (pv))) {
                    // Avoid leaving unresolved forward references in the
                    // evaluation node, in order to work around context-related
                    // problems. Resolving a fwd ref requires a symbol search
                    // and the appropriate context may be unavailable at a later
                    // time.
                    CV_typ_t newindex;
                    if (getDefnFromDecl(PTR_UTYPE (pv), pv, &newindex)) {
                        PTR_UTYPE (pv) = newindex;
                    }
                }

                switch (EVAL_PTRTYPE (pv) = ((plfPointer)pType)->attr.ptrmode) {
                    case CV_PTR_MODE_PTR:
                        break;

                    case CV_PTR_MODE_REF:
                        EVAL_IS_REF (pv) = TRUE;
                        break;

                    case CV_PTR_MODE_PMEM:
                        EVAL_IS_PMEMBER (pv) = TRUE;
                        PTR_PMCLASS (pv) = ((plfPointer)pType)->pbase.pm.pmclass;
                        PTR_PMENUM (pv) = ((plfPointer)pType)->pbase.pm.pmenum;
                        break;

                    case CV_PTR_MODE_PMFUNC:
                        EVAL_IS_PMETHOD (pv) = TRUE;
                        PTR_PMCLASS (pv) = ((plfPointer)pType)->pbase.pm.pmclass;
                        PTR_PMENUM (pv) = ((plfPointer)pType)->pbase.pm.pmenum;
                        break;

                    default:
                        pExState->err_num = ERR_BADOMF;
                        retflag = FALSE;
                        break;

                }
                switch (EVAL_PTRTYPE (pv) = ((plfPointer)pType)->attr.ptrtype) {
                    case CV_PTR_NEAR32:
                    case CV_PTR_FAR32:
                        // can't cast from 32 bit ptr to a 16 or vice versa
                        if (EVAL_IS_PTR (pv) &&
                            (EVAL_PTRTYPE(pv) != CV_PTR_NEAR32) &&
                            (EVAL_PTRTYPE(pv) != CV_PTR_FAR32)
                            ) {
                            retflag = FALSE;
                        }
                        break;

                    default:
                        if (EVAL_IS_PTR (pv) &&
                            ((EVAL_PTRTYPE(pv) == CV_PTR_NEAR32) ||
                            (EVAL_PTRTYPE(pv) == CV_PTR_FAR32))
                            ) {
                            retflag = FALSE;
                            break;
                        }

                        switch (EVAL_PTRTYPE (pv)) {
                            case CV_PTR_BASE_SEG:
                                // based on a segment.  Use the segment value from the leaf
                                PTR_BSEG (pv) = ((plfPointer)pType)->pbase.bseg;
                                break;

                            case CV_PTR_BASE_VAL:
                            case CV_PTR_BASE_SEGVAL:
                            case CV_PTR_BASE_ADDR:
                            case CV_PTR_BASE_SEGADDR:

                                // We need to do an extra symbol search to find
                                // the symbol on which the pointer is based.
                                // The copy of the symbol record in the type
                                // section is not good. We need to do the
                                // extra search even if the base is bp-relative
                                // The compiler no longer sets the correct offset
                                // in copy of the symbol record found in the type
                                // section.

                                memset (pName, 0, sizeof (*pName));

                                // initialize search_t struct
                                // M00KLUDGE: We use the context stored
                                // in the TM during the bind phase. This
                                // does not work properly If the actual
                                // base is shadowed by a local variable

                                pName->pfnCmp = (PFNCMP) FNCMP;
                                pName->pv = (peval_t) pv;
                                pName->scope = SCP_lexical | SCP_module | SCP_global;
                                pName->clsmask = 0;
                                pName->CXTT = *pCxt;
                                pName->bn = 0;
                                pName->bnOp = 0;
                                pName->state = SYM_init;

                                pSym = (SYMPTR)(&((plfPointer)pType)->pbase.Sym);
                                PTR_BSYMTYPE (pv) = pSym->rectyp;
                                emiAddr (PTR_ADDR (pv)) = pCxt->addr.emi;

                                if (SearchBasePtrBase(pName) != HR_found) {
                                    pExState->err_num = ERR_NOTEVALUATABLE;
                                    return FALSE;
                                }
                        }
                }
                SetDPtr (pv, &hType);
                break;

            case LF_ARRAY:
                // The CodeView information doesn't tell us whether arrays
                // are near or far, so we always make them far.

                EVAL_IS_DATA (pv) = TRUE;
                EVAL_IS_ADDR (pv) = TRUE;
                EVAL_IS_PTR (pv) = TRUE;
                EVAL_IS_ARRAY (pv) = TRUE;
                EVAL_PTRTYPE (pv) = CV_PTR_NEAR32;
                PTR_UTYPE (pv) = ((plfArray)pType)->elemtype;
                if (!CV_IS_PRIMITIVE (PTR_UTYPE (pv))) {
                    // Avoid leaving unresolved forward references in the
                    // evaluation node, in order to work around context-related
                    // problems. Resolving a fwd ref requires a symbol search
                    // and the appropriate context may be unavailable at a later
                    // time.
                    CV_typ_t newindex;
                    if (getDefnFromDecl(PTR_UTYPE (pv), pv, &newindex)) {
                        PTR_UTYPE (pv) = newindex;
                    }
                }
                skip = offsetof (lfArray, data);
                PTR_ARRAYLEN (pv) = (ulong) RNumLeaf (((char *)(&pType->leaf)) + skip, &skip);
                break;

            case LF_PROCEDURE:
                EVAL_IS_ADDR (pv) = TRUE;
                EVAL_IS_FCN (pv) = TRUE;
                FCN_RETURN (pv) = ((plfProc)pType)->rvtype;
                if (FCN_RETURN (pv) == 0) {
                    FCN_RETURN (pv) = T_VOID;
                }
                call = (CV_call_e) ((plfProc)pType)->calltype;
                FCN_PCOUNT (pv) = ((plfProc)pType)->parmcount;
                FCN_PINDEX (pv) = ((plfProc)pType)->arglist;
                skip = sizeof (lfProc);
                CheckFcnArgs (pv, &hType, call);
                break;

            case LF_MFUNCTION:
                EVAL_IS_ADDR (pv) = TRUE;
                EVAL_IS_FCN (pv) = TRUE;
                EVAL_IS_METHOD (pv) = TRUE;
                FCN_CLASS (pv) = ((plfMFunc)pType)->classtype;
                FCN_THIS (pv) = ((plfMFunc)pType)->thistype;
                FCN_RETURN (pv) = ((plfMFunc)pType)->rvtype;
                if (FCN_RETURN (pv) == 0) {
                    FCN_RETURN (pv) = T_VOID;
                }
                call = (CV_call_e) ((plfMFunc)pType)->calltype;
                FCN_PCOUNT (pv) = ((plfMFunc)pType)->parmcount;
                FCN_PINDEX (pv) = ((plfMFunc)pType)->arglist;
                FCN_THISADJUST (pv) = ((plfMFunc)pType)->thisadjust;
                skip = sizeof (lfMFunc);
                CheckFcnArgs (pv, &hType, call);
                break;

            case LF_MODIFIER:
                cvol = ((plfModifier)pType)->attr;
                type = ((plfModifier)pType)->type;
                MHOmfUnLock (hType);
                hType = 0;
                if (cvol.MOD_const == TRUE) {
                    EVAL_IS_CONST (pv) = TRUE;
                }
                else if (cvol.MOD_volatile == TRUE){
                    EVAL_IS_VOLATILE (pv) = TRUE;
                }
                goto modifier;

            case LF_VTSHAPE:
                EVAL_IS_VTSHAPE (pv) = TRUE;
                VTSHAPE_COUNT (pv) = ((plfVTShape)pType)->count;
                break;

            case LF_LABEL:
                EVAL_IS_LABEL (pv) = TRUE;
                break;

            default:
                pExState->err_num = ERR_BADOMF;
                retflag = FALSE;
                break;
        }
        if (hType != 0) {
            MHOmfUnLock (hType);
        }
    }

    if (EVAL_IS_REG (pv) && EVAL_IS_PTR (pv)) {
        // an enregistered pointer
        // restore register information
        PTR_REG_IREG (pv) = iregSav;
        PTR_REG_HIBYTE (pv) = hibyteSav;
    }

    return (retflag);
}


/**     CheckFcnArgs - check for function arguments
 *
 *      CheckVarArgs (nv, phType, call, pcParam);
 *
 *      Entry   nv = near pointer to value node
 *              phType = pointer to type handle
 *              call = calling convention
 *              pcParam = pointer to paramater count
 *
 *      Exit    N_FCN_VARARGS (nv) = TRUE if varargs
 *
 *      Returns none
 */

void
CheckFcnArgs (
    neval_t nv,
    HTYPE *phType,
    CV_call_e call
    )
{
    plfArgList  pType;
    uint        skip = 0;

    switch(call) {
        case CV_CALL_NEAR_C:
            //near C call - caller pops stack
            N_FCN_CALL (nv) = FCN_C;
            // N_FCN_FARCALL (nv) = FALSE;
            N_FCN_CALLERPOP (nv) = TRUE;
            break;

        case CV_CALL_FAR_C:
            // far C call - caller pops stack
            N_FCN_CALL (nv) = FCN_C;
            N_FCN_FARCALL (nv) = TRUE;
            N_FCN_CALLERPOP (nv) = TRUE;
            break;

        case CV_CALL_NEAR_PASCAL:
            // near pascal call - callee pops stack
            N_FCN_CALL (nv) = FCN_PASCAL;
            // N_FCN_FARCALL (nv) = FALSE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_FAR_PASCAL:
            // far pascal call - callee pops stack
            N_FCN_CALL (nv) = FCN_PASCAL;
            N_FCN_FARCALL (nv) = TRUE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_NEAR_FAST:
            // near fast call - callee pops stack
            N_FCN_CALL (nv) = FCN_FAST;
            // N_FCN_FARCALL (nv) = FALSE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_FAR_FAST:
            // far fast call - callee pops stack
            N_FCN_CALL (nv) = FCN_FAST;
            N_FCN_FARCALL (nv) = TRUE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_NEAR_STD:
            // near standard call - callee pops stack
            N_FCN_CALL (nv) = FCN_STD;
            // N_FCN_FARCALL (nv) = FALSE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_FAR_STD:
            // far fast call - callee pops stack
            N_FCN_CALL (nv) = FCN_STD;
            N_FCN_FARCALL (nv) = TRUE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_THISCALL:       // this call (this passed in register)
            N_FCN_CALL (nv) = FCN_THISCALL;
            N_FCN_FARCALL (nv) = FALSE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

        case CV_CALL_MIPSCALL:
            N_FCN_CALL(nv) = FCN_MIPS;
            break;

        case CV_CALL_ALPHACALL:
            N_FCN_CALL(nv) = FCN_ALPHA;
            break;

        case CV_CALL_PPCCALL:
            N_FCN_CALL(nv) = FCN_PPC;
            break;

        case CV_CALL_IA64CALL:
            N_FCN_CALL(nv) = FCN_IA64;
            break;

        default:
            // unknown function
            DASSERT (FALSE);
            N_FCN_CALL (nv) = (enum fcn_call) 0;
            // N_FCN_FARCALL (nv) = FALSE;
            // N_FCN_CALLERPOP (nv) = FALSE;
            break;

    }
    //M00KLUDGE - this check is to avoid a cvpack problem to be fixed
    if (N_FCN_PINDEX (nv) == 0) {
        return;
    }
    if ((N_FCN_CALL (nv) != FCN_C) || (FCN_PINDEX (nv) == T_VOID)) {
        return;
    }
    MHOmfUnLock (*phType);
    *phType = THGetTypeFromIndex (EVAL_MOD (nv), N_FCN_PINDEX (nv));
    DASSERT (*phType != 0);
    if (*phType == 0) {
        return;
    }
    pType = (plfArgList)(&((TYPPTR)(MHOmfLock (*phType)))->leaf);

    if (FCN_PCOUNT (nv) == 0) {
        // there are no arguments.  We need to check for old C
        // style varargs and voidarg function calls.  These are
        // indicated by an argument count of zero and an
        // argument type list which is a LF_EASY list.

        if ((pType->count == 0) || (pType->arg[0] == T_NOTYPE)) {
            // This is either void or no args.  We cannot
            // tell the difference so we set the varargs flag
            N_FCN_VARARGS (nv) = TRUE;
        }
    }
    else {
        // There are formal parameters.  Skip down the list to the last
        // parameter and check for varargs

        if (pType->arg[pType->count - 1] == T_NOTYPE) {
            // the last argument has a type of zero which is the specification
            // of varargs.  Set the type to 0 to indicate vararg
            N_FCN_VARARGS (nv) = TRUE;
        }
        else if (pType->arg[pType->count - 1] == LF_DEFARG) {
            N_FCN_DEFARGS (nv) = TRUE;
        }
    }
}




/**     SetDPtr - set data pointer flag
 *
 *      SetDPtr (nv, phType)
 *
 *      Entry   nv = near pointer to value node
 *              phType = pointer to type handle
 *
 *      Exit    EVAL_IS_DPTR (nv) = TRUE if pointer to data
 *
 *      Returns none
 */


INLINE void
SetDPtr (
    neval_t nv,
    HTYPE *phType
    )
{
    // the type is considered a data pointer unless the type is
    // a) non-primitive
    // b) fully fetchable
    // c) not a procedure
    //
    // this code is functionally the same as the old code below
    // except that it doesn't fault and reads easier [rm]

    N_EVAL_IS_DPTR (nv) = TRUE;

    if (!CV_IS_PRIMITIVE (N_PTR_UTYPE (nv))) {
        MHOmfUnLock (*phType);
        *phType = THGetTypeFromIndex (EVAL_MOD (nv), N_PTR_UTYPE (nv));

        if (*phType) {
            TYPPTR typptr = (TYPPTR)MHOmfLock(*phType);
            if (typptr && typptr->leaf == LF_PROCEDURE) {
                N_EVAL_IS_DPTR (nv) = FALSE;
            }
        }
    }
}

#ifdef OLD_CODE_THAT_CAN_FAULT_SOMETIMES_LEFT_FOR_REFERENCE // [rm]
INLINE void
SetDPtr (
    neval_t nv,
    HTYPE *phType
    )
{
    if (!CV_IS_PRIMITIVE (N_PTR_UTYPE (nv))) {
        MHOmfUnLock (*phType);
        *phType = THGetTypeFromIndex (EVAL_MOD (nv), N_PTR_UTYPE (nv));
        if (((plfEasy)(&((TYPPTR)(MHOmfLock (*phType)))->leaf))->leaf != LF_PROCEDURE) {
            N_EVAL_IS_DPTR (nv) = TRUE;
        }
    } else {
        N_EVAL_IS_DPTR (nv) = TRUE;
    }
}

#endif


/***    LoadVal - Load the value of a node
 *
 *      fSuccess = LoadVal (pv)
 *
 *      Entry   pv = pointer to value
 *
 *      Exit    EVAL_VAL (pv) = value
 *              EVAL_STATE (pv) = EV_rvalue
 *
 *      Returns TRUE if value loaded
 *              FALSE if not (complex symbol type such as structure).
 *
 */


bool_t
LoadVal (
    peval_t pv
    )
{
    DASSERT (EVAL_STATE (pv) == EV_lvalue);

    if (LoadSymVal (pv)) {
        EVAL_STATE (pv) = EV_rvalue;
        return (TRUE);
    }
    return (FALSE);
}


/*
 *  TypeSize
 *
 *  Returns size in bytes of value on expression stack
 */


long
TypeSize (
    peval_t pv
    )
{
    if (CV_IS_PRIMITIVE (EVAL_TYP (pv))) {
        // primitive type
        return (TypeSizePrim (EVAL_TYP (pv)));
    } else {
        // complex type
        return (TypeDefSize (pv));
    }
}


/*
 *  TypeDefSize
 *
 *  Returns size in bytes of a non-primitive type.
 */

long
TypeDefSize (
    peval_t pv
    )
{
    long    retval;

    if (EVAL_IS_ARRAY (pv)) {
        retval = PTR_ARRAYLEN (pv);
    }
    else if (EVAL_IS_PTR (pv)) {
        switch (EVAL_PTRTYPE (pv)) {
            case CV_PTR_FAR:
            case CV_PTR_HUGE:
                retval = 4;
                break;

            case CV_PTR_NEAR32:
                retval = 4;
                break;

            case CV_PTR_FAR32:
                retval = 6;
                break;

            case CV_PTR_64:
                retval = 8;
                break;

            case CV_PTR_NEAR:
                retval = 2;
                break;

            default:
                // This is a based pointer. Since CV information
                // does not distinguish between 16 and 32 bit
                // based pointers, use the f32bit flag in the TM to
                // determine the pointer size. -- dolphin #979
                retval = pExState->state.f32bit ? 4 : 2;
        }
    }
    else if (EVAL_IS_CLASS (pv)) {
        retval = CLASS_LEN (pv);
    }
    else if (EVAL_IS_FCN (pv)) {
        if (pv->data.fcn.flags.farcall == TRUE) {
            retval = 4;
        }
        else {
            retval = sizeof (UOFFSET);
        }
    }
    else if (EVAL_IS_BITF (pv)) {
        switch (BITF_UTYPE (pv)) {
            case T_CHAR:
            case T_UCHAR:
            case T_RCHAR:
            case T_INT1:
            case T_UINT1:
                retval = 1;
                break;

            case T_SHORT:
            case T_USHORT:
            case T_INT2:
            case T_UINT2:
                retval = 2;
                break;

            case T_LONG:
            case T_ULONG:
            case T_INT4:
            case T_UINT4:
                retval = 4;
                break;

            case T_QUAD:
            case T_UQUAD:
            case T_INT8:
            case T_UINT8:
                retval = 8;
                break;

            default:
                pExState->err_num = ERR_BADOMF;
                retval = 0;
                break;
        }
    }
    else if (EVAL_IS_ENUM (pv)) {
        DASSERT ( CV_IS_PRIMITIVE (ENUM_UTYPE (pv)) );

        retval = TypeSizePrim (ENUM_UTYPE (pv));
    }
    else {
        retval = 0;
    }
    return (retval);
}


/**     TypeSizePrim - return size of primitive type
 *
 *      len = TypeSizePrim (type)
 *
 *      Entry   type = primitive type index
 *
 *      Exit    none
 *
 *      Returns size in byte of primitive type
 */

int
TypeSizePrim (
    CV_typ_t itype
    )
{
    if (itype == T_NOTYPE) {
        // we need to have a assert here but we cannot because a
        // pointer to function can (will) have a null argumtent list
        // which makes it look like a varargs which means all bets are off

        return (sizeof (UOFFSET));
    }
    else if (CV_MODE (itype) != CV_TM_DIRECT) {
        return (size_ptr [CV_MODE (itype)]);
    }
    else switch (CV_TYPE (itype)) {
        case CV_SPECIAL:
            return (size_special [CV_SUBT (itype)]);

        case CV_SPECIAL2:
            return (size_special2 [CV_SUBT (itype)]);

        case CV_INT:
            return (size_int [CV_SUBT (itype)]);

        case CV_SIGNED:
        case CV_UNSIGNED:
        case CV_BOOLEAN:
            return (size_integral [CV_SUBT (itype)]);

        case CV_REAL:
        case CV_COMPLEX:
            return (size_real [CV_SUBT (itype)]);

        default:
            DASSERT (FALSE);
            return (0);
    }
}


/**     UpdateMem - update memory or register for assignment operation
 *
 *      fSuccess = UpdateMem (pv);
 *
 *      Entry   pv = pointer to node containg update data
 *
 *      Exit    is_assign = TRUE to indicate memory changed
 *
 *      Returns TRUE if memory updated without error
 *              FALSE if error during memory update
 */

bool_t
UpdateMem (
    peval_t pv
    )
{
    SHREG   reg;
    uint    cbVal;
    ushort  dummy[2];
    ADDR    addr;

    is_assign = TRUE;         /* Indicate assignment operation */
    if (!EVAL_IS_REG (pv)) {
        // destination is not a register
        if (EVAL_IS_BPREL (pv)) {
            if (!ResolveAddr(pv)) {
                return FALSE;
            }
        }
        if (TargetMachine == mptmips || TargetMachine == mptdaxp) {
            if (EVAL_IS_REGREL (pv)) {
                if (!ResolveAddr(pv)) {
                    return FALSE;
                }
            }
        }
        cbVal =  TypeSize (pv);
        addr = EVAL_SYM (pv);
        if (ADDR_IS_LI (addr)) {
            SHFixupAddr (&addr);
        }
        if (EVAL_IS_PTR (pv) && (EVAL_IS_FPTR (pv) || EVAL_IS_HPTR (pv))) {
            dummy[0] = (ushort) EVAL_PTR_OFF (pv);
            dummy[1] = (ushort) EVAL_PTR_SEG (pv);
            return (PutDebuggeeBytes (addr, cbVal, (char *)dummy, EVAL_TYP(pv)) == cbVal);
        } else {
            return (PutDebuggeeBytes (addr, cbVal, (char *)&EVAL_VAL (pv), EVAL_TYP(pv)) == cbVal);
        }
    }
    reg.hReg = EVAL_REG (pv);
    switch (EVAL_TYP (pv)) {
        case T_REAL32:
            if (TargetMachine == mptmips || TargetMachine == mptdaxp) {
                *(float *)&reg.Byte4 = EVAL_FLOAT (pv);
            } else {
                reg.Byte10 = Float10FromDouble ( (double)EVAL_FLOAT (pv) );
            }
            break;

        case T_REAL64:
            if (TargetMachine == mptmips || TargetMachine == mptdaxp) {
                reg.Byte8 = EVAL_DOUBLE (pv);
            } else {
                reg.Byte10 = Float10FromDouble ( EVAL_DOUBLE (pv) );
            }
            break;

        case T_REAL80:
            reg.Byte10 = EVAL_LDOUBLE (pv);
            break;

            // Fall through
        case T_CPLX32:
        case T_CPLX64:
        case T_CPLX80:
            pExState->err_num = ERR_TYPESUPPORT;
            return (FALSE);

        default:
            switch (TypeSize (pv)) {
                case 1:
                    reg.Byte1 = EVAL_UCHAR (pv);
                    break;
                case 2:
                    reg.Byte2 = EVAL_USHORT (pv);
                    break;
                case 4:
                    reg.Byte4 = EVAL_ULONG  (pv);
                    break;
                case 8:
                    reg.Byte8i = EVAL_UQUAD  (pv);
                    break;

                default:
                    pExState->err_num = ERR_TYPESUPPORT;
                    return (FALSE);
            }

    }
    if (SetReg (&reg, NULL, NULL) == NULL) {
        pExState->err_num = ERR_REGNOTAVAIL;
        return (FALSE);
    }

    if ((reg.hReg == CV_REG_CS) ||  (reg.hReg == CV_REG_IP)) {
        //  M00KLUDGE   what do I do here?????
        //fEnvirGbl.fAll &= mdUserPc;  /* clear the user pc mask */
        //UpdateUserEnvir (mUserPc); /* restore the user pc */
    }
    return (TRUE);
}


void
FlipBytes (
    uchar *pval,
    CV_typ_t type,
    UINT cbSize
    )
{
    uchar *pb, bT;

    DASSERT (TargetMachine == mptmppc);

    if ( (type != T_PASCHAR) &&
       (type != T_CHAR) &&
       (type != T_UCHAR) &&
       (type != T_RCHAR))
    {
        pb = pval;

        while (cbSize > 1) {
            cbSize--;

            bT = *pb;
            *pb = *(pb+cbSize);
            *(pb+cbSize) = bT;

            pb++;
            cbSize--;
        }
    }
}

/**     GetDebuggeeBytes
 **     PutDebuggeeBytes
 **     GetReg
 **     SetReg
 **     SaveReg
 **     RestoreReg
 *
 *      These functions get bytes from the debuggee and swap endianness
 *      if necessary; currently, only when TargetMachine == mptmppc.
 */

UINT
GetDebuggeeBytes (
    ADDR addr,
    UINT cb,
    void *pv,
    CV_typ_t type
    )
{
    UINT retval;

    if ( !Is64PtrSE(addr.addr.off) ) {
        addr.addr.off = SE32To64(addr.addr.off);
    }

    retval = (*pCVF->pDHGetDebuggeeBytes)(addr, cb, pv);

    if (TargetMachine == mptmppc)
        FlipBytes((uchar *)pv, type, cb);

    return(retval);
}

UINT
PutDebuggeeBytes (
    ADDR addr,
    UINT cb,
    void *pv,
    CV_typ_t type
    )
{
    UINT retval;

    if (TargetMachine == mptmppc)
        FlipBytes((uchar *)pv, type, cb);

    retval = (*pCVF->pDHPutDebuggeeBytes)(addr, cb, pv);

    if (TargetMachine == mptmppc)
        FlipBytes((uchar *)pv, type, cb);

    return(retval);
}


//  GetUdtDefnTindex
//
//  Do a symbol search looking for the UDT sym in the current context.  From
//  that we should be able to get at the UDT type record that defines the UDT.
//      Entry
//          TypeIn  type index of forward ref UDT type record (used for cache)
//          nv      current eval node
//          lpStr   length preceeded string of UDT name to look for
//
//      Exit
//          Return  type index of defined UDT type record
//
// Note: due to the possibility of recursion (e.g. struct X fwd ref exists without a corresponding
// struct X definition), GetUdtDefnTindex uses a static variable to guard against infinite regress.
//

INLINE CV_prop_t
GetProperty(
    CV_typ_t type,
    neval_t nv
    )
{
    HTYPE hType;
    plfEasy pType;
    CV_prop_t retval = {0};

    if ((hType = GetHTypeFromTindex (nv, type)) == 0) {
        DASSERT(FALSE);
        return retval;
    }
    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);

    switch (pType->leaf) {
            case LF_CLASS:
            case LF_STRUCTURE:
                retval = ((plfClass)pType)->property;
                break;

            case LF_UNION:
                retval = ((plfUnion)pType)->property;
                break;

            case LF_ENUM:
                retval = ((plfEnum)pType)->property;
                break;

            default:
                DASSERT(FALSE);
    }

    MHOmfUnLock(hType);
    return retval;
}

CV_typ_t
GetUdtDefnTindex (
    CV_typ_t TypeIn,
    neval_t nv,
    char *lpStr
    )
{
    static BOOL fIn = FALSE;
    search_t    Name;
    eval_t      localEval = *nv;
    CV_typ_t    tiResult = T_NOTYPE;
    CV_prop_t   propIn, propSeek;

    // recursion check
    if (fIn)
        return FALSE;
    fIn = TRUE; // set recursion guard

    EVAL_TYP (&localEval) = 0;
    EVAL_ITOK (&localEval) = 0;
    EVAL_CBTOK (&localEval) = 0;

    memset (&Name, 0, sizeof (search_t));
    Name.initializer = INIT_sym;
    Name.pfnCmp = (PFNCMP) FNCMP;
    Name.pv = &localEval;
    // Look in all scopes except class scope: if we are in a member
    // fn of the current class, this will lead to infinite recursion
    // as we look for class X in the scope of class X in the ...
    Name.scope = SCP_all & ~SCP_class;
    Name.clsmask = CLS_enumerate | CLS_ntype;

#if 0
    // the problem of what context to try to look up the type
    // here seems to be a very touch one.  This code looks
    // pretty good but it doesn't handle the case where
    // the type name is defined in the scope of the function
    // and it seems any kind of test against EVAL_MOD(nv)
    // is doomed to failure.

    // in particular while in foo() { enum A {red};  A a = red; }
    //
    // you can't evaluate 'red' without major fireworks


    if (SHHexeFromHmod (EVAL_MOD (nv)) != SHHMODFrompCXT (pCxt))
            SHGetCxtFromHmod(EVAL_MOD(nv), &Name.CXTT);

#endif

#if 0
    // instead of the above we go with a strategy where we try
    // the most recent symbol scope if there is one available
    // or just the global scope.  For V2 we actually went with
    // just the global scope, the below is a natural improvement
    // as if there is a symbol scope it is much more likely to
    // be correct than the global scope (but in most cases they
    // are the same anyway which is why the global scope works
    // pretty well) [rm]

    if (nv->CXTT.hMod != 0)
        Name.CXTT = nv->CXTT;
    else
        Name.CXTT = *pCXT;
#endif

    // Modified the above mentioned scheme a little. Note that we should never look for the
    // type in a mod different from EVAL_MOD(nv). That is where the forward reference type was
    // found and is the only place we should get the defn from.. However to deal with types defined
    // in local contexts such as the "enum A" example in the comments above,
    // we might need richer information such as the hProc.
    // So if the passed in context has the same hMod as EVAL_MOD(nv) we will use
    // that context since it potentially more detailed and will let us detect local types.
    // else we drop back to using just EVAL_MOD(nv) in the context. Lastly if we have no EVAL_MOD(nv)
    // we just go ahead and use the global context and hope for the best [sgs].

    if ( EVAL_MOD(nv) == 0 )
        Name.CXTT = *pCxt;
    else if ( EVAL_MOD(nv) == SHHMODFrompCXT(&(nv->CXTT)))
        Name.CXTT = nv->CXTT;
    else
        SHGetCxtFromHmod(EVAL_MOD(nv), &Name.CXTT);

    Name.bn = 0;
    Name.bnOp = 0;
    Name.sstr.lpName = (uchar *) (lpStr + 1);
    Name.sstr.cb = *lpStr;
    Name.state = SYM_init;

    // modify search to look only for UDTs

    Name.sstr.searchmask = SSTR_symboltype;
    Name.sstr.symtype = S_UDT;

#if 0
    propIn = GetProperty(TypeIn, nv);

    while (SearchSym (&Name) == HR_found) {
        PopStack ();
        if (EVAL_STATE (&localEval) == EV_type) {
            if (CV_IS_PRIMITIVE (EVAL_TYP(&localEval)))
                continue;
            propSeek = GetProperty(EVAL_TYP(&localEval), &localEval);
            if ((propIn.isnested == propSeek.isnested) &&
                (propIn.scoped == propSeek.scoped)) {

                // check that modules match
                // (something is very wrong if they don't)

                if (SHHexeFromHmod(EVAL_MOD(nv)) == SHHexeFromHmod(EVAL_MOD(&localEval))) {
                    tiResult = EVAL_TYP (&localEval);
                }
                else {
                    // DASSERT(FALSE);
                }

                break;
            }
        }
    }
#endif

        //
        //      The above is wrong because there is no iteration variable.  Whenever
        //      fail to find the symbol the first time, you will do the SearchSym
        //      again and find the same symbol again => infinite loop.
        //

    propIn = GetProperty(TypeIn, nv);

        if (SearchSym (&Name) == HR_found)
        {
        PopStack ();

                if (EVAL_STATE (&localEval) == EV_type)
                {
            if (!CV_IS_PRIMITIVE (EVAL_TYP(&localEval)))
                        {
                                propSeek = GetProperty(EVAL_TYP(&localEval), &localEval);

                                if (propIn.isnested == propSeek.isnested &&
                                        propIn.scoped == propSeek.scoped)
                                {

                                        //
                                        //      check that modules match -- something is very wrong
                                        //      if they dont
                                        //

                                        if (SHHexeFromHmod(EVAL_MOD(nv)) ==
                                                SHHexeFromHmod(EVAL_MOD(&localEval))
                                        )
                                        {
                                                tiResult = EVAL_TYP (&localEval);
                                        }
                                }
                        }
                }
        }


    fIn = FALSE; // clear recursion guard
    return tiResult;
}


// Tick counter functions
static ULONG    nTickCounter;

void
ResetTickCounter (
    void
    )
{
    // Reset value to 1 (reserve 0 as a special timestamp value)
    nTickCounter = 1;
}

void
IncrTickCounter (
    void
    )
{
    nTickCounter ++;
}

ULONG
GetTickCounter (
    void
    )
{
    return nTickCounter;
}

/**     GetHSYMCodeFromHSYM - Get HSYM encoded form from HSYM value
 *
 *      lsz = GetHSYMFromHSYMCode (hSym)
 *
 *      Entry   hSm = hSym to be encoded
 *
 *      Exit    none
 *
 *      Returns pointer to static buffer containing a string
 *              representation of hSym. The encoding is merely
 *              a conversion to a string that expresses the
 *              hSym value in hex notation.
 *
 */

char *
GetHSYMCodeFromHSYM(
    HSYM hSym
    )
{
#if defined (_WIN64)
#define HSYM_SPRINTF_FORMAT "%016.016p\0"
#elif defined(_WIN32)
#define HSYM_SPRINTF_FORMAT "%08.08lx\0"
#endif
    static char buf[HSYM_CODE_LEN + 1];
    sprintf(buf, HSYM_SPRINTF_FORMAT, (UINT_PTR)hSym);
    return (char *)buf;
}

/**     GetHSYMFromHSYMCode - Get HSYM from encoded HSYM string
 *
 *      hSym = GetHSYMFromHSYMCode (lsz)
 *
 *      Entry   lsz = pointer to encoded HSYM string
 *
 *      Exit    none
 *
 *      Returns hSym value
 *
 */

HSYM
GetHSYMFromHSYMCode(
    char *lsz
    )
{
    ULONG_PTR ul = 0;
    char ch;
    int digit;
    int i;
    for (i=0; i < HSYM_CODE_LEN; i++) {
        DASSERT (_istxdigit ((_TUCHAR)*lsz));
        ch = *lsz++;
        if (_istdigit ((_TUCHAR)ch))
            digit = ch - '0';
        else
            digit = _totupper((_TUCHAR)ch) - 'A' + 10;
        ul <<= 4;
        ul += digit;
    }
    return (HSYM) ul;
}


/**     fCanSubtractPtrs - Check if ptrs can be subtracted
 *
 *      flag = fCanSubtractPtrs (pvleft, pvright)
 *
 *      Entry   pvleft, pvRight = pointers to corresponding
 *                  evaluation nodes.
 *
 *      Exit    none
 *
 *      Returns TRUE if ptr subtraction is allowed for the
 *              corresponding pointer types.
 *
 */
bool_t
fCanSubtractPtrs (
    peval_t pvleft,
    peval_t pvright
    )
{
    bool_t      retval = FALSE;
    eval_t      evalL;
    eval_t      evalR;
    peval_t     pvL = &evalL;
    peval_t     pvR = &evalR;

    DASSERT (EVAL_IS_PTR (pvleft) && EVAL_IS_PTR (pvright));
    DASSERT (!EVAL_IS_REF (pvleft) && !EVAL_IS_REF (pvright));

#if CC_LAZYTYPES
    if (THAreTypesEqual( EVAL_MOD(pvleft), EVAL_TYP (pvleft), EVAL_TYP (pvright) ))
#else
    if (EVAL_TYP (pvleft) == EVAL_TYP (pvright))
#endif
    {
        retval = TRUE;
    }
    else if ( EVAL_PTRTYPE (pvleft) == EVAL_PTRTYPE (pvright) ) {
        *pvL = *pvleft;
        *pvR = *pvright;

        // check the underlying types
        // RemoveIndir will resolve fwd. references and
        // skip modifier nodes.
        RemoveIndir (pvL);
        RemoveIndir (pvR);

#if CC_LAZYTYPES
        retval = THAreTypesEqual( EVAL_MOD(pvL), EVAL_TYP (pvL), EVAL_TYP (pvR) );
#else
        retval = ( EVAL_TYP (pvL) == EVAL_TYP (pvR) );
#endif
    }

    return retval;
}


/**     TMLAddHTM - Add HTM to TM list
 *
 *      flag = TMLAddHTM (pTML, hTM)
 *
 *      Entry   pTML = pointer to TM list
 *              hTM = handle of TM to be added to the list
 *
 *      Exit    TMlist may grow to accomodate new handle if necessary
 *
 *      Returns TRUE if hTM was successfully added to the list
 *              FALSE otherwise
 */

bool_t
TMLAddHTM (
    PTML pTML,
    HTM hTM
    )
{
    HDEP    hTMList;
    HTM    *rgHTM;

    if (pTML->hTMList == NULL) {
        if ((pTML->hTMList = MemAllocate (TMLISTCNT * sizeof (HTM))) == NULL ) {
            return FALSE;
        }
        rgHTM = (HTM *) MemLock (pTML->hTMList);
        memset (rgHTM, 0, TMLISTCNT * sizeof (HTM));
        MemUnLock (pTML->hTMList);
        pTML->cTMListMax = TMLISTCNT;
    }
    if (pTML->cTMListAct >= pTML->cTMListMax ) {
        if ((hTMList = MemReAlloc (pTML->hTMList,
          (pTML->cTMListMax + TMLISTCNT) * sizeof (HTM))) == 0) {
            return FALSE;
        }
        rgHTM = (HTM *) MemLock (hTMList);
        memset(rgHTM+pTML->cTMListAct, 0, TMLISTCNT * sizeof(HTM) );
        MemUnLock (hTMList);
        pTML->hTMList = hTMList;
        pTML->cTMListMax += TMLISTCNT;
    }

    rgHTM = (HTM *) MemLock (pTML->hTMList);
    rgHTM[pTML->cTMListAct++] = hTM;
    MemUnLock (pTML->hTMList);
    return TRUE;
}

/***    GetTypeName - Get Type name
 *
 *      lsz = GetTypeName (typ, hMod)
 *
 *      Entry   typ = type index
 *              hMod = handle to module within which typ is defined
 *
 *      Exit    type name stored in static buffer
 *
 *      Returns pointer to static buffer containing type name, or
 *              NULL if no type name was found
 */

LSZ
GetTypeName(
    CV_typ_t typ,
    HMOD hMod
    )
{
    static char lszName[NAMESTRMAX];
    plfEasy     pType;
    HTYPE       hType;
    uint        skip;
    uint        len;

    if ((hType = THGetTypeFromIndex (hMod, typ)) == 0) {
        return NULL;
    }
retry:
    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
    switch (pType->leaf) {
        case LF_MODIFIER:
            if ((hType = THGetTypeFromIndex (hMod, ((plfModifier)pType)->type)) == 0) {
                return NULL;
            }
            goto retry;

        case LF_STRUCTURE:
        case LF_CLASS:
            skip = offsetof (lfClass, data);
            RNumLeaf (((char *)(&pType->leaf)) + skip, &skip);
            len = *(((unsigned char *)&(pType->leaf)) + skip);
            DASSERT (len < sizeof (lszName));
            memcpy (lszName, ((char *)pType) + skip + 1, len);
            * (lszName + len) = '\0';
            MHOmfUnLock (hType);
            break;

        case LF_UNION:
            skip = offsetof (lfUnion, data);
            RNumLeaf (((char *)(&pType->leaf)) + skip, &skip);
            len = *(((unsigned char *)&(pType->leaf)) + skip);
            DASSERT (len < sizeof (lszName));
            memcpy (lszName, ((char *)pType) + skip + 1, len);
            * (lszName + len) = '\0';
            MHOmfUnLock (hType);
            break;

        default:
            // For the time this routine is only used for getting
            // class names
            DASSERT (FALSE);
            MHOmfUnLock (hType);
            return NULL;
    }
    return lszName;
}
