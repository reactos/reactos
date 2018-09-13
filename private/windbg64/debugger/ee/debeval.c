/***    DEBEVAL.C - Expression evaluator main routines
 *
 *
 */


#include "debexpr.h"


#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY


// Return values from Logical ().

#define DL_DEBERR       1       // Error occurred, DebErr is set
#define DL_SUCCESS      2       // Evaluation successful
#define DL_CONTINUE     3       // Inconclusive, continue evaluation


// Actions to be taken by EvalUtil().

#define EU_LOAD 0x0001          // Load node values
#define EU_TYPE 0x0002          // Do implicit type coercion



uint Logical (op_t, bool_t);

bool_t FASTCALL CalcThisExpr (CV_typ_t, UOFFSET, UOFFSET, CV_typ_t);
bool_t DerefVBPtr (CV_typ_t);
bool_t FASTCALL Eval (bnode_t);
bool_t FASTCALL EvalAddrOf (bnode_t);
bool_t FASTCALL EvalArray (bnode_t);
bool_t FASTCALL EvalAssign (bnode_t);
bool_t FASTCALL EvalBang (bnode_t);
bool_t FASTCALL EvalBasePtr (bnode_t);
bool_t FASTCALL EvalBinary (bnode_t);
bool_t FASTCALL EvalBScope (bnode_t);
bool_t FASTCALL EvalByteOps (bnode_t);
bool_t FASTCALL EvalCast (bnode_t);
bool_t FASTCALL EvalContext (bnode_t);
bool_t FASTCALL EvalDMember (bnode_t);
bool_t FASTCALL EvalDot (bnode_t);
bool_t FASTCALL EvalError (register bnode_t);
bool_t FASTCALL EvalFetch (bnode_t);
bool_t FASTCALL EvalFunction (bnode_t);
bool_t FASTCALL EvalLChild (bnode_t);
bool_t FASTCALL EvalLogical (bnode_t);
bool_t FASTCALL EvalRChild (bnode_t);
bool_t FASTCALL EvalParent (bnode_t);
bool_t FASTCALL EvalPlusMinus (bnode_t);
bool_t FASTCALL EvalPMember (bnode_t);
bool_t FASTCALL EvalPostIncDec (bnode_t);
bool_t FASTCALL EvalPreIncDec (bnode_t);
bool_t FASTCALL EvalPointsTo (bnode_t);
bool_t FASTCALL EvalPushNode (bnode_t);
bool_t FASTCALL EvalRelat (bnode_t);
bool_t FASTCALL EvalRetVal (bnode_t);
bool_t FASTCALL EvalSegOp (bnode_t);
bool_t FASTCALL EvalThisInit (bnode_t);
bool_t FASTCALL EvalThisConst (bnode_t);
bool_t FASTCALL EvalThisExpr (bnode_t);
bool_t FASTCALL EvalUnary (bnode_t);
bool_t FASTCALL EvalUScope (bnode_t);

bool_t FASTCALL EVArith (op_t);
bool_t FASTCALL Assign (op_t);
bool_t FASTCALL EvalUtil (op_t, peval_t, peval_t, ulong );
bool_t FASTCALL FetchOp (peval_t pv);
bool_t FASTCALL InitConst (long);
bool_t FASTCALL IsStringLiteral (peval_t);
bool_t FASTCALL EVPlusMinus (op_t);
INLINE  bool_t EVPrePost (op_t);
bool_t FASTCALL Relational (op_t);
bool_t FASTCALL SBitField (void);
bool_t FASTCALL StructEval (bnode_t);
bool_t FASTCALL StructElem (bnode_t);
bool_t ThisOffset (peval_t);
bool_t FASTCALL EVUnary (op_t);
bool_t PushArgs (pnode_t, SHREG *, UOFFSET*);
bool_t PushOffset (UOFFSET, SHREG *, UOFFSET *, ulong );
bool_t PushString (peval_t, SHREG *, CV_typ_t, BOOL Is32);
bool_t PushRef (peval_t, SHREG *, CV_typ_t, BOOL Is32);
bool_t PushUserValue (peval_t, pargd_t, SHREG *, UOFFSET *);
bool_t StoreC (peval_t);
bool_t StoreP (void);
bool_t StoreF (void);
bool_t Store32 (peval_t);
bool_t StoreMips (peval_t);
bool_t StoreAlpha (peval_t);
bool_t StorePPC (peval_t);
bool_t StoreIa64 (peval_t);
INLINE bool_t VFuncAddress (peval_t, ulong );
INLINE bool_t VFuncAddress2 (peval_t, peval_t);
INLINE ULONG MakeLong (USHORT, USHORT);
INLINE USHORT LoWord (ULONG);
INLINE USHORT HiWord (ULONG);
INLINE BOOL getFuncAddrNoThunks (peval_t pv);


eval_t  ThisAddress;
const peval_t pvThis = &ThisAddress;



// Bind dispatch table

bool_t (NEAR FASTCALL *SEGBASED(_segname("_CODE"))pEval[]) (bnode_t) = {
#define OPCNT(name, val)
#define OPCDAT(opc)
#define OPDAT(op, opfprec, opgprec, opclass, opbind, opeval) opeval,
#include "debops.h"
#undef OPDAT
#undef OPCDAT
#undef OPCNT
};


bool_t FASTCALL EvalError (register bnode_t bn)
{
    Unreferenced(bn);

    pExState->err_num = ERR_INTERNAL;
    return (FALSE);
}


bool_t FASTCALL Eval (bnode_t bn)
{
    return ((*pEval[NODE_OP((pnode_t)bn)])(bn));
}

bool_t FASTCALL EvalLChild (bnode_t bn)
{
    register bnode_t bnL = NODE_LCHILD (bn);

    return ((*pEval[NODE_OP(bnL)])(bnL));
}

bool_t FASTCALL EvalRChild (bnode_t bn)
{
    register bnode_t bnR = NODE_RCHILD (bn);

    return ((*pEval[NODE_OP(bnR)])(bnR));
}

// os independent long double (10 byte reals) support

// check for 32-bit compiler and existence of long doubles.
#if (_MSC_VER >= 800) && defined(_M_IX86) && (_M_IX86 >= 300)

#pragma message("WARNING:floating point code not portable to non-x86 CPU's")

FLOAT10 __inline    Float10FromLong ( long lSrc ) {
    FLOAT10 f10Ret;
    __asm {
        fild    lSrc
        fstp    tbyte ptr f10Ret;
        fwait
        }
    return f10Ret;
    }

FLOAT10 __inline    Float10Negate ( FLOAT10 flt10 ) {
    double  dNeg1 = -1.0;
    FLOAT10 f10Ret;
    __asm {
        fld     tbyte ptr flt10
        fmul    qword ptr dNeg1
        fstp    tbyte ptr f10Ret
        fwait
        }
    return f10Ret;
    }

FLOAT10 __inline    Float10OpPlus ( FLOAT10 f1, FLOAT10 f2 ) {
    FLOAT10 f10Ret;
    __asm {
        fld     tbyte ptr f1
        fld     tbyte ptr f2
        faddp   st(1), st(0)
        fstp    tbyte ptr f10Ret
        fwait
        }
    return f10Ret;
    }

FLOAT10 __inline    Float10OpMinus ( FLOAT10 f1, FLOAT10 f2 ) {
    FLOAT10 f10Ret;
    __asm {
        fld     tbyte ptr f1
        fld     tbyte ptr f2
        fsubp   st(1), st(0)
        fstp    tbyte ptr f10Ret
        fwait
        }
    return f10Ret;
    }

FLOAT10 __inline    Float10OpMul ( FLOAT10 f1, FLOAT10 f2 ) {
    FLOAT10 f10Ret;
    __asm {
        fld     tbyte ptr f1
        fld     tbyte ptr f2
        fmulp   st(1), st(0)
        fstp    tbyte ptr f10Ret
        fwait
        }
    return f10Ret;
    }

FLOAT10 __inline    Float10OpDiv ( FLOAT10 f1, FLOAT10 f2 ) {
    FLOAT10 f10Ret;
    __asm {
        fld     tbyte ptr f1
        fld     tbyte ptr f2
        fdivp   st(1), st(0)
        fstp    tbyte ptr f10Ret
        fwait
        }
    return f10Ret;
    }

FLOAT10 __inline    Float10Zero () {
    FLOAT10 f10Ret;
    __asm {
        fldz
        fstp    tbyte ptr f10Ret
        fwait
        }
    return f10Ret;
    }

#pragma warning(disable:4035)

int __inline        Float10Equal ( FLOAT10 f1, FLOAT10 f2 ) {
    __asm {
        fld     tbyte ptr f1
        fld     tbyte ptr f2
        fcompp
        fnstsw  ax
        fwait
        sahf
        mov     eax, 0      // can't use xor or sub eax,eax because of flags!!!
        sete    al
        }
    // note: leaves result in eax!
    }

int __inline        Float10LessThan ( FLOAT10 f1, FLOAT10 f2 ) {
    __asm {
        fld     tbyte ptr f1
        fld     tbyte ptr f2
        fcompp
        fnstsw  ax
        fwait
        sahf
        mov     eax, 0      // can't use xor or sub eax,eax because of flags!!!
        seta    al
        }
    // note: leaves result in eax!
    }


#pragma warning(default:4035)

#else

// float10 is really a long double in this case

FLOAT10 __inline    Float10FromLong ( long lSrc ) {
    FLOAT10 f10;
    DASSERT(FALSE);
    return f10;
    //return (FLOAT10) lSrc;
    }

FLOAT10 __inline    Float10Negate ( FLOAT10 flt10 ) {
    DASSERT(FALSE);
    return flt10;
    //return -flt10;
    }

FLOAT10 __inline    Float10OpPlus ( FLOAT10 f1, FLOAT10 f2 ) {
    DASSERT(FALSE);
    return f1;
    //return f1 + f2;
    }

FLOAT10 __inline    Float10OpMinus ( FLOAT10 f1, FLOAT10 f2 ) {
    DASSERT(FALSE);
    return f1;
    //return f1 - f2;
    }

FLOAT10 __inline    Float10OpMul ( FLOAT10 f1, FLOAT10 f2 ) {
    DASSERT(FALSE);
    return f1;
    //return f1 * f2;
    }

FLOAT10 __inline    Float10OpDiv ( FLOAT10 f1, FLOAT10 f2 ) {
    DASSERT(FALSE);
    return f1;
    //return f1 / f2;
    }

FLOAT10 __inline    Float10Zero () {
    static FLOAT10 f10zero;
    DASSERT(FALSE);
    return f10zero;
    //return 0.0;
    }

int __inline        Float10Equal ( FLOAT10 f1, FLOAT10 f2 ) {
    DASSERT(FALSE);
    return 0;
    //return f1 == f2;
    }

int __inline        Float10LessThan ( FLOAT10 f1, FLOAT10 f2 ) {
    DASSERT(FALSE);
    return 0;
    //return f1 < f2;
    }

#endif

/***    DoEval - Evaluate parse tree
 *
 * SYNOPSIS
 *       error = DoEval ()
 *
 * ENTRY
 *       none
 *
 * RETURNS
 *       True if tree evaluated without error.
 *
 * DESCRIPTION
 *       The parser will call this routine to evaluate the parse tree
 *
 * NOTES
 */


bool_t DoEval (PHTM phTM, HFRAME hFrame, EEDSP style)
{
    ulong       retval = EEGENERAL;
    bool_t      evalstate;
    pexstate_t  pParentTM;
    bool_t      fIncrementalEval = FALSE;

    // lock the expression state structure, save the formatting style
    // and frame

    DASSERT (*phTM != 0);
    if (*phTM == 0) {
        return (EECATASTROPHIC);
    }

    pExState = (pexstate_t) MemLock (*phTM);
    if (pExState->hParentTM) {
        pParentTM = (pexstate_t) MemLock (pExState->hParentTM);
        if (pParentTM->state.bind_ok &&
            pExState->cxt.hBlk == pParentTM->cxt.hBlk &&
            pExState->cxt.hProc == pParentTM->cxt.hProc &&
            pExState->cxt.hMod == pParentTM->cxt.hMod ) {

            // The parent TM has been bound in
            // the same scope as the child TM

            if (pParentTM->state.eval_ok &&
                pParentTM->timeStamp >= GetTickCounter()) {
                // parent TM contains an up-to-date value,
                // that can be used for incremental evaluation
                fIncrementalEval = TRUE;
            }
            else if (pParentTM->err_num == 0){
                // err_num check is just a heuristic:
                // attempt to reevaluate parent TM (even
                // if it has never been evaluated before)
                // but prevent continuous reevaluations of
                // ancestor TMs that can't be evaluated
                // anyway
                pexstate_t pExStateSav;
                EESTATUS error;

                pExStateSav = pExState;
                error = DoEval (&pExStateSav->hParentTM, hFrame, style);
                pExState = pExStateSav;

                if (EENOERROR == error) {
                    fIncrementalEval = TRUE;
                }
            }
        }
        MemUnLock (pExState->hParentTM);
    }

    pExState->style = style;
    pExState->hframe = hFrame;
    pCxt = &pExState->cxt;

    pExState->err_num = ERR_NOTEVALUATABLE;
    //DASSERT (pExState->state.parse_ok == TRUE);
    //DASSERT (pExState->state.bind_ok == TRUE);
    //DASSERT (pExState->hSTree != 0);

    if ((pExState->state.parse_ok == TRUE)
      && (pExState->state.bind_ok == TRUE)
      && pExState->hETree) {
        pTree = (pstree_t) MemLock (pExState->hETree);

        // DASSERT (hEStack != 0);
        // Codeview may reevaluate with a new EE without rebinding
        // In that case the hEStack of the new EE may be null
        // caviar #3804 --gdp 9/29/92

        if (hEStack == 0) {
            if ((hEStack = MHMemAllocate (ESTACK_DEFAULT * sizeof (elem_t))) == 0) {
                return (EECATASTROPHIC);
            }
            StackLen = (belem_t)(ESTACK_DEFAULT * sizeof (elem_t));
        }

        pEStack = (pelem_t) MemLock (hEStack);
        StackMax = 0;
        StackCkPoint = 0;
        StackOffset = 0;
        pExState->err_num = 0;
        ST = NULL;
        STP = NULL;
        pExStr = (char *) MemLock (pExState->hExStr);

        // zero out type of ThisAddress so that users can check to see if
        // ThisAddress has been correctly initialized

        bnCxt = 0;
        EVAL_TYP (pvThis) = 0;
        Evaluating = TRUE;
        {
            SHREG       spReg;
            bnode_t     bnParent = 0;
            bnode_t     bnRoot;
            op_t        opSav;

            bnRoot = (bnode_t)pTree->start_node;

            if (fIncrementalEval &&
                (bnParent = GetParentSubtree(bnRoot, pExState->seTemplate)) !=  0) {
                opSav = NODE_OP (bnParent);
                NODE_OP (bnParent) = OP_parent;
            }


            // do the evaluation -- any function args/temporaries will
            // stay on the users stack for the whole eval [rm]

            evalstate = Eval (bnRoot);

            if (fIncrementalEval && bnParent) {
                // restore evaluation tree to its original form
                NODE_OP (bnParent) = opSav;
            }
        }
        Evaluating = FALSE;
        bnCxt = 0;
        MemUnLock (pExState->hExStr);

        // If the input consisted of a single identifier (i.e., no
        // operators involved), then the resulting value may still
        // be an identifier.  Look it up in the symbols.

        if (evalstate == TRUE) {
            uint access = pExState->result.flags.bits.access;
            if (EVAL_STATE (&pExState->result) == EV_type) {
                EVAL_STATE (ST) = EV_type;
            }
            pExState->result = *ST;
            // restore the access flags from the bind phase --gdp
            pExState->result.flags.bits.access = access;
            pExState->state.eval_ok = TRUE;
            pExState->state.fAEreeval = TRUE;
            // set eval timestamp
            pExState->timeStamp = GetTickCounter();
            pExState->evalThisSav = *pvThis;
            retval = EENOERROR;
        }
        else {
            retval =  EECATASTROPHIC; // EEGENERAL;
        }
        MemUnLock (hEStack);
        MemUnLock (pExState->hETree);
    }
    MemUnLock (*phTM);
    return (retval);
}


bool_t FASTCALL EvalPushNode (bnode_t bn)
{
#if 1   // kentf
    bool_t  Ok;

    Ok = PushStack (&bn->v[0]);

    if ( Ok && EVAL_IS_CURPC( ST ) ) {

        //
        //  Get current PC
        //

#pragma message("Need investigation on SYSGetAddr(&(EVAL_PTR(ST)),adrPC)")

        SYGetAddr(pExState->hframe, &(EVAL_PTR(ST)), adrPC);

        EVAL_IS_ADDR( ST )   = TRUE;
        EVAL_IS_BPREL( ST )  = FALSE;
        EVAL_IS_REGREL( ST ) = FALSE;
        EVAL_IS_TLSREL( ST ) = FALSE;
        //
        //  correctly set the type based on the size of the PC
        //

        if (MODE_IS_FLAT(EVAL_PTR_MODE(ST))) {
            EVAL_TYP(ST) = T_32PCHAR;
        }
        else {
            EVAL_TYP(ST) = T_PCHAR;
        }
    }

    return Ok;

#else // kentf

    bool_t  Ok;
    ADDR    addr;

    Ok = PushStack (&bn->v[0]);

    if ( Ok && EVAL_IS_CURPC( ST ) ) {

        //
        //  Get current PC
        //

        // The original code is commented out.
        // The replacement code may not work.
        // Once the ide tree is running, BryanT
        // will investigate.
        // SYGetAddr( &(EVAL_PTR( ST )), adrPC);

        memset(&addr, 0, sizeof(ADDR));
        emiAddr(addr) = emiAddr(EVAL_SYM(ST));

#pragma message("Need investigation on SYSGetAddr(&(EVAL_PTR(ST)),adrPC)")

        EVAL_SYM_OFF(ST) += GetAddrOff(addr);
        EVAL_SYM_SEG(ST) = GetAddrSeg(addr);

        EVAL_IS_ADDR( ST )   = TRUE;
        EVAL_IS_BPREL( ST )  = FALSE;
        EVAL_IS_REGREL( ST ) = FALSE;
        EVAL_IS_TLSREL( ST ) = FALSE;
    }

    return Ok;
#endif // kentf
}


/**     EvalParent - evaluate parent subexpression
 *
 *      fSuccess = EvalParent (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if no error during evaluation
 *              FALSE if error during evaluation
 *
 * DESCRIPTION
 *      Evaluates a parent subexpression that is embedded
 *      in a child expression by accessing the evaluated
 *      result of the parent TM. The parent TM must have
 *      been evaluated before this function gets control.
 */

bool_t FASTCALL EvalParent (bnode_t bn)
{
    pexstate_t  pTM;
    bool_t      retval;

        if (!pExState->hParentTM) {
                DASSERT (FALSE);
                return FALSE;
        }
    pTM = (pexstate_t) MemLock (pExState->hParentTM);

    DASSERT (pTM->state.eval_ok);
    *pvThis = pTM->evalThisSav;
    retval = PushStack (&pTM->result);
    MemUnLock (pExState->hParentTM);
    return retval;
}





/**     EvalUnary - perform a unary arithmetic operation
 *
 *      fSuccess = EvalUnary (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if no error during evaluation
 *              FALSE if error during evaluation
 *
 * DESCRIPTION
 *      Evaluates the result of an arithmetic operation.  The unary operators
 *      dealt with here are:
 *
 *      ~       -       +
 *
 *      Pointer arithmetic is NOT handled; all operands must be of
 *      arithmetic type.
 */


bool_t FASTCALL EvalUnary (bnode_t bn)
{
    if (EvalLChild (bn)) {
        return (EVUnary (NODE_OP (bn)));
    }
    return (FALSE);
}




/***    EvalBasePtr - Perform a based pointer access (:>)
 *
 *      fSuccess = EvalBasePtr (bnRight)
 *
 *      Entry   bnRight = based pointer to right operand node
 *
 *      Exit
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL EvalBasePtr (bnode_t bn)
{
    return (EvalSegOp (bn));
}




/**     EvalBinary - evaluate a binary arithmetic operation
 *
 *      fSuccess = EvalBinary (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if no error during evaluation
 *              FALSE if error during evaluation
 *
 */


bool_t FASTCALL EvalBinary (bnode_t bn)
{
    if (EvalLChild (bn) && EvalRChild (bn)) {
        return (EVArith (NODE_OP (bn)));
    }
    return (FALSE);
}






/***    EvalPlusMinus - Evaluate binary plus or minus
 *
 *      fSuccess = EvalPlusMinus (bn)
 *
 *      Entry   bn = based pointer to tree node
 *
 *      Exit    ST = STP +- ST
 *
 *      Returns TRUE if Eval successful
 *              FALSE if Eval error
 *
 *
 */


bool_t FASTCALL EvalPlusMinus (bnode_t bn)
{
    if (EvalLChild (bn) && EvalRChild (bn)) {
        return (EVPlusMinus (NODE_OP (bn)));
    }
    return (FALSE);
}






/***    EvalDot - Perform the dot ('.') operation
 *
 *      fSuccess = EvalDot (bn)
 *
 *      Entry   bn = based pointer to tree node
 *
 *      Exit    NODE_STYPE (bn) = type of stack top
 *
 *      Returns TRUE if Eval successful
 *              FALSE if Eval error
 *
 *      Exit    pExState->err_num = error ordinal if Eval error
 *
 */


bool_t FASTCALL EvalDot (bnode_t bn)
{
    if (!EvalLChild (bn)) {
        return (FALSE);
    }
    if (EVAL_STATE (ST) == EV_rvalue && !EVAL_IS_REF (ST)) {
        return (StructElem (bn));
    }
    else {
        return (StructEval (bn));
    }
}


/**     EvalLogical - Handle '&&' and '||' operators
 *
 *      wStatus = EvalLogical (bn)
 *
 *      Entry   op = Operand (OP_andand or OP_oror)
 *              REval = FALSE if right hand not evaluated
 *                      ST = left hand value
 *              REval = TRUE if right hand evaluated
 *                      STP = left hand value
 *                      ST = right hand value
 *
 *      Returns wStatus = evaluation status
 *              (REval = TRUE)
 *                  DL_DEBERR   Evaluation failed, DebErr is set
 *                  DL_SUCCESS  Evaluation succeeded, result in ST
 *
 *              (REVAL == FALSE)
 *                  DL_DEBERR   Evaluation failed,
 *                  DL_SUCCESS  Evaluation succeeded, result in ST
 *                  DL_CONTINUE Evaluation inconclusive, must evaluate right node
 *
 *      DESCRIPTION
 *       If (REval = TRUE), checks that both operands (STP and ST) are of scalar
 *       type and evaluates the result (0 or 1).
 *
 *       If (REval == FALSE), checks that the left operand (ST) is of scalar
 *       type and determines whether evaluation of the right operand is
 *       necessary.
 *
 */


bool_t FASTCALL EvalLogical (bnode_t bn)
{
    uint        wStatus;

    if (!EvalLChild (bn)) {
        return (FALSE);
    }
    wStatus = Logical (NODE_OP (bn), FALSE);
    if (wStatus == DL_DEBERR) {
        return(FALSE);
    }
    else if (wStatus == DL_SUCCESS) {
        // Do not evaluate rhs
        return (TRUE);
    }

    //DASSERT (wStatus == DL_CONTINUE);

    if (!EvalRChild (bn)) {
        return (FALSE);
    }
    return (Logical (NODE_OP (bn), TRUE) == DL_SUCCESS);
}


/**     Logical - Handle '&&' and '||' operators
 *
 *      wStatus = Logical (op, REval)
 *
 *      Entry   op = Operand (OP_andand or OP_oror)
 *              REval = FALSE if right hand not evaluated
 *                      ST = left hand value
 *              REval = TRUE if right hand evaluated
 *                      STP = left hand value
 *                      ST = right hand value
 *
 *      Returns wStatus = evaluation status
 *              (REval = TRUE)
 *                  DL_DEBERR   Evaluation failed, DebErr is set
 *                  DL_SUCCESS  Evaluation succeeded, result in ST
 *
 *              (REVAL == FALSE)
 *                  DL_DEBERR   Evaluation failed,
 *                  DL_SUCCESS  Evaluation succeeded, result in ST
 *                  DL_CONTINUE Evaluation inconclusive, must evaluate right node
 *
 *      DESCRIPTION
 *       If (REval = TRUE), checks that both operands (STP and ST) are of scalar
 *       type and evaluates the result (0 or 1).
 *
 *       If (REval == FALSE), checks that the left operand (ST) is of scalar
 *       type and determines whether evaluation of the right operand is
 *       necessary.
 *
 */


uint Logical (op_t op, bool_t REval)
{
    int     result;
    bool_t  fIsZeroL;
    bool_t  fIsZeroR;

    // Evaluate the result.  We push a constant node with value of
    // zero so we can compare against it (i.e., expr1 && expr2 is
    // equivalent to (expr1 != 0) && (expr2 != 0)).
    // Evaluate whether the left node is zero.

    if (REval == FALSE) {
        if (!PushStack (ST) || !InitConst (0)) {
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
        if (!Relational (OP_eqeq))
            return (DL_DEBERR);

        //DASSERT (EVAL_TYP (ST) == T_SHORT);

        fIsZeroL = (EVAL_SHORT (ST) == 1);

        // remove left hand comparison

        PopStack ();
        result = (op == OP_oror);
        EVAL_STATE (ST) = EV_rvalue;
        if (pExState->state.f32bit) {
            EVAL_LONG (ST) = (long) result;
            SetNodeType (ST, T_INT4);
        }
        else {
            EVAL_SHORT (ST) = (short) result;
            SetNodeType (ST, T_INT2);
        }
        if (
            ((op == OP_andand) && (!fIsZeroL))
            ||
            ((op == OP_oror) && (fIsZeroL))
            ) {
            return(DL_CONTINUE);
        }
        else {
            return (DL_SUCCESS);
        }
    }
    else {
        if (!PushStack (ST) || !InitConst (0)) {
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
        if (!Relational (OP_eqeq))
            return (DL_DEBERR);

        //DASSERT (EVAL_TYP(ST) == T_SHORT);

        fIsZeroR = (EVAL_SHORT (ST) == 1);
        PopStack ();
        fIsZeroL = (EVAL_SHORT (STP) == 1);

        // Finally, determine whether or not we have a result, and if so,
        // what the result is:

        if (
            ((op == OP_andand) && ((!fIsZeroL) && (!fIsZeroR)))
            ||
            ((op == OP_oror) && ((!fIsZeroL) || (!fIsZeroR)))
            )
            result = 1;
        else
            result = 0;
    }
    EVAL_STATE (STP) = EV_rvalue;
    if (pExState->state.f32bit) {
        EVAL_LONG (STP) = (long) result;
        SetNodeType (STP, T_INT4);
    }
    else {
        EVAL_SHORT (STP) = (short) result;
        SetNodeType (STP, T_INT2);
    }
    PopStack ();
    return (DL_SUCCESS);
}





/***    EvalContext - evaluate the context operator
 *
 *      fSuccess = EvalContext (bn)
 *
 *
 *      Exit    ST = address node
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 *       The stack top value (ST) is set to the address of the
 *       stack top operand
 */


bool_t FASTCALL EvalContext (bnode_t bn)
{
    PCXT    oldCxt;
    bool_t  error;
    bnode_t oldbnCxt;
    HFRAME  hframeOld;

    // Set the new context for evaluation of the remainder of this
    // part of the tree

    oldCxt = pCxt;
    oldbnCxt = bnCxt;
    pCxt = SHpCXTFrompCXF ((PCXF)&bn->v[0]);
    hframeOld = pExState->hframe;
    pExState->hframe = SHhFrameFrompCXF ((PCXF)&bn->v[0]);
#if 1 // JLS
    // Why would we ever have a valid "frame" at bind time,  the frame
    // is a strange handle passed in at Eval time.  The best one can do
    // is to either get a new Frame now, or use the normal frame
    //
    if (pExState->hframe == NULL) {
        pExState->hframe = hframeOld;
    }
#else // JLS
    if ((pExState->hframe.BP.seg == 0) && (pExState->hframe.BP.off == 0)) {
        // we did not have a valid frame at bind time
        pExState->hframe = hframeOld;
    }
#endif // JLS
    error = EvalLChild (bn);
    if (error == TRUE) {
        // if there was not an error and if the result of the
        // expression is bp relative, then we must load the value
        // before returning to the original context

        if ((EVAL_STATE (ST) == EV_lvalue) && (EVAL_IS_BPREL (ST) || EVAL_IS_REGREL(ST) || EVAL_IS_TLSREL(ST)) &&
            // if the new context is the same as the old context, we can
            // still treat bp relatives as l-values. It should be sufficient
            // to compare only hProc and hMod (since the explicit context
            // cannot contain a block
            !(SHHMODFrompCXT(pCxt) == SHHMODFrompCXT(oldCxt) &&
              SHHPROCFrompCXT(pCxt) == SHHPROCFrompCXT(oldCxt))
            ) {
            if (EVAL_IS_REF (ST)) {
                if (!EvalUtil (OP_fetch, ST, NULL, EU_LOAD)) {
                    // unable to load value
                    pExState->err_num = ERR_NOTEVALUATABLE;
                    pExState->hframe = hframeOld;
                    return (FALSE);
                }
                EVAL_IS_REF (ST) = FALSE;
                EVAL_STATE (ST) = EV_lvalue;
                EVAL_SYM_OFF (ST) = EVAL_PTR_OFF (ST);
                EVAL_SYM_SEG (ST) = EVAL_PTR_SEG (ST);
                SetNodeType (ST, PTR_UTYPE (ST));
            }
            if (EVAL_IS_ENUM (ST)) {
                SetNodeType (ST, ENUM_UTYPE (ST));
            }
            if (!EvalUtil (OP_fetch, ST, NULL, EU_LOAD)) {
                // unable to load value
                pExState->err_num = ERR_NOTEVALUATABLE;
                pExState->hframe = hframeOld;
                return (FALSE);
            }
#if defined(TARGMAC68K)
            EVAL_STATE (ST) = EV_lvalue;
#endif
        }
    }
    pExState->hframe = hframeOld;
    if ((bnCxt = oldbnCxt) != 0) {
        // the old context was pointing into the expression tree.
        // since the expression tree could have been reallocated,
        // we must recompute the context pointer

       pCxt = SHpCXTFrompCXF ((PCXF)&((pnode_t)bnCxt)->v[0]);
    }
    else {
        // the context pointer is pointing into the expression state structure
        pCxt = oldCxt;
    }
    return (error);
}




/***    EvalAddrOf - Perform the address-of ('&') operation
 *
 *      fSuccess = EvalAddrOf (bn)
 *
 *      Entry
 *
 *      Exit    ST = address node
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 *       The stack top value (ST) is set to the address of the
 *       stack top operand
 */


bool_t FASTCALL EvalAddrOf (bnode_t bn)
{
    CV_typ_t       type;

    if (!EvalLChild (bn)) {
        return (FALSE);
    }

    if (EVAL_STATE (ST) != EV_lvalue) {
        pExState->err_num = ERR_NOTEVALUATABLE;
        return (FALSE);
    }

    // The operand must be an lvalue and cannot be a register variable

    DASSERT (!(EVAL_IS_REG (ST)));
    DASSERT (NODE_STYPE (bn) != 0);

    if (EVAL_IS_REF(ST)) {
        // just clear the ref flag in order to convert
        // ST to a pointer to the object
        EVAL_IS_REF(ST) = FALSE;
        return TRUE;
    }

    if ((type = NODE_STYPE (bn)) == 0) {
        // unable to find proper pointer type
        return (FALSE);
    }
    else {
        EVAL_STATE (ST) = EV_rvalue;
        if (EVAL_IS_BPREL (ST)) {
            ResolveAddr(ST);
        }
        if (EVAL_IS_REGREL(ST)) {
            ResolveAddr(ST);
        }
        EVAL_PTR (ST) = EVAL_SYM (ST);
        if (ADDR_IS_LI (EVAL_PTR (ST))) {
            SHFixupAddr (&EVAL_PTR (ST));
        }
        return (SetNodeType (ST, type));
    }
}





/***    EvalFetch - Perform the fetch ('*') operation
 *
 *      fSuccess = EvalFetch (bn)
 *
 *      Entry   ST = pointer to address value
 *
 *      Exit    ST = dereferenced value
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 *      The resultant node is set to the contents of the location
 *      pointed to by the operand node.
 */


bool_t FASTCALL EvalFetch (bnode_t bn)
{
    if (EvalLChild (bn)) {
        return (FetchOp (ST));
    }
    return (FALSE);
}




/***    EvalThisInit - initialize this calculation
 *
 *      fSuccess = EvalThisInit (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Exit    TempThis = stack top
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL EvalThisInit (bnode_t bn)
{
    Unreferenced(bn);

    *pvThis = *ST;
    if (EVAL_IS_CLASS (pvThis)) {
#if 1
        ResolveAddr(pvThis);
#else
        if (EVAL_IS_BPREL (pvThis)) {
            EVAL_SYM_OFF (pvThis) += pExState->frame.BP.off;
            EVAL_SYM_SEG (pvThis) = pExState->frame.SS; //M00FLAT32
            EVAL_IS_BPREL (pvThis) = FALSE;
            ADDR_IS_LI (EVAL_SYM (pvThis)) = FALSE;
        }
#endif
        return (TRUE);
    }
    else if (EVAL_IS_PTR (pvThis)) {
        if (EVAL_STATE (pvThis) == EV_lvalue) {
            FetchOp (pvThis);
        }
        else {
            EVAL_SYM (pvThis) = EVAL_PTR (pvThis);
            EVAL_STATE (pvThis) = EV_lvalue;

            // Remove a level of indirection from the resultant type.

            RemoveIndir (pvThis);
            EVAL_IS_REF (pvThis) = FALSE;
            return (TRUE);
        }
    }

    // we should not initialize the this address unless the stack top
    // is a class or a pointer to class

    pExState->err_num = ERR_INTERNAL;
    return (FALSE);
}




/***    EvalThisConst - Adjust this temp by constant
 *
 *      fSuccess = EvalThisConst (bn)
 *
 *      Entry   bn = based pointer to node containing constant
 *
 *      Exit    EVAL_SYM_OFF (TempThis) adjusted by constant
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL EvalThisConst (bnode_t bn)
{
    if (EvalLChild (bn) == TRUE) {
        EVAL_SYM_OFF (pvThis) += ((padjust_t)(&bn->v[0]))->disp;
        return (SetNodeType (pvThis, ((padjust_t)(&bn->v[0]))->btype));
    }
    return (FALSE);
}




/***    EvalThisExpr - Adjust this temp by expression
 *
 *      fSuccess = EvalThisExpr (bn)
 *
 *      Entry   bn = based pointer to node containing expression
 *
 *      Exit    EVAL_SYM_OFF (TempThis) adjusted by expression
 *              newaddr = oldaddr + *(*(oldaddr + vbptroff) + vbindex)
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 *      The evaluation of this node will result in
 *
 *          pvThis = (pvThis + vbpoff) + *(*(pvThis +vbpoff) + vbdisp)
 *      where
 *          pvThis = address of base class
 *          ap = current address point
 */


bool_t FASTCALL EvalThisExpr (bnode_t bn)
{
    padjust_t   pa = (padjust_t)(&bn->v[0]);

    // we set the node types of the pointer to char * to prevent the
    // EVPlusMinus () code from attempting an array indexing operation


    if (EvalLChild (bn) == TRUE) {
        return (CalcThisExpr (pa->vbptr, pa->vbpoff, pa->disp, pa->btype));
    }
    return (FALSE);
}




/***    CalcThisExpr - Adjust this temp by expression
 *
 *      fSuccess = CalcThisExpr (vbptr, vbpoff, disp, btype)
 *
 *      Entry   vbptr = type index of virtual base pointer
 *              vbpoff = offset of vbptr from this pointer
 *              disp = index of virtual base displacement from *vbptr
 *              btype = type index of base class
 *
 *      Exit    EVAL_SYM_OFF (TempThis) adjusted by expression
 *              newaddr = oldaddr + *(*(oldaddr + vbptroff) + vbindex)
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 *      The evaluation of this node will result in
 *
 *          pvThis = (pvThis + vbpoff) + *(*(pvThis + vbpoff) + vbdisp)
 *      where
 *          pvThis = address of base class
 */


bool_t FASTCALL CalcThisExpr (CV_typ_t vbptr, UOFFSET vbpoff,
  UOFFSET disp, CV_typ_t btype)
{
    // we set the node types of the pointer to char * or a 32 flat
    // char * to prevent the
    // EVPlusMinus () code from attempting an array indexing operation

    if (PushStack (pvThis) == TRUE) {
        EVAL_PTR (ST) = EVAL_SYM (ST);
        EVAL_STATE (ST) = EV_rvalue;
        if ((SetNodeType (ST, (CV_typ_t) (pExState->state.f32bit ? T_32PCHAR : T_PFCHAR)) == TRUE) &&
// BUGBUG must support 64 bit constants
          (InitConst ((UOFF32)vbpoff) == TRUE) &&
          (EVPlusMinus (OP_plus) == TRUE) &&
          (PushStack (ST) == TRUE) &&
          (DerefVBPtr (vbptr)) &&
// BUGBUG must support 64 bit constants
          (InitConst ((UOFF32)disp) == TRUE) &&
          (EVPlusMinus (OP_plus) == TRUE) &&
          (EVAL_IS_ARRAY (ST) = FALSE, EVAL_STATE (ST) = EV_lvalue, EVAL_SYM (ST) = EVAL_PTR (ST), FetchOp (ST) == TRUE) &&
          (EVAL_STATE (ST) = EV_rvalue, EVAL_PTR (ST) = EVAL_SYM (ST), EVPlusMinus (OP_plus) == TRUE)
          ) {
            SetNodeType (ST, btype);
            EVAL_STATE (ST) = EV_lvalue;
            EVAL_SYM (ST) = EVAL_PTR (ST);
            *pvThis = *ST;
            return (PopStack ());
        }
    }
    return (FALSE);
}



/**     DerefVBPtr - dereference virtual base pointer
 *
 *      flag = DerefVBPtr (type)
 *
 *      Entry   type = type index of virtual base pointer
 *              ST = address of virtual base pointer as T_PFCHAR or T_32PCHAR
 *
 *      Exit    ST = virtual base pointer
 *
 *      Returns TRUE if no error
 *              FALSE if error
 */


bool_t DerefVBPtr (CV_typ_t type)
{

    // The operand cannot be a register variable

    DASSERT (!(EVAL_IS_REG (ST)));
    DASSERT (type != 0);
    DASSERT (EVAL_IS_BPREL (ST) == FALSE);

    if (type != 0) {
        if (SetNodeType (ST, type) == TRUE) {
            EVAL_STATE (ST) = EV_lvalue;
            EVAL_SYM (ST) = EVAL_PTR (ST);
            if (!EvalUtil (OP_fetch, ST, NULL, EU_LOAD | EU_TYPE)) {
                return (FALSE);
            }

            // The resultant node is basically identical to the child except
            // that its EVAL_SYM field is equal to the actual contents of
            // the pointer:

            if (EVAL_IS_BASED (ST)) {
                if (!NormalizeBase (ST)) {
                    return(FALSE);
                }
            }

            // Remove a level of indirection from the resultant type.

            RemoveIndir (ST);
            EVAL_IS_REF (ST) = FALSE;
            return (TRUE);
        }
    }
    return (FALSE);
}




/***    EvalAssign - Perform an assignment operation
 *
 *      fSuccess = EvalAssign (bn)
 *
 *      Entry   bn = based pointer to assignment node
 *
 *      Exit
 *
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL EvalAssign (bnode_t bn)
{
    if (!EvalLChild (bn) || !EvalRChild (bn)) {
        return (FALSE);
    }
    return (Assign (NODE_OP (bn)));
}




/***    Assign - Perform an assignment operation
 *
 *      fSuccess = Assign (op)
 *
 *      Entry   op = assignment operator
 *
 *      Exit
 *
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL Assign (op_t op)
{
    extern CV_typ_t eqop[];
    op_t        nop;

    // Left operand must have evaluated to an lvalue

    if (EVAL_STATE (STP) != EV_lvalue) {
        pExState->err_num = ERR_NEEDLVALUE;
        return (FALSE);
    }
    if (EVAL_IS_REF (ST)) {
        if (FetchOp (ST) == FALSE) {
            return (FALSE);
        }
    }
    if (EVAL_IS_REF (STP)) {
        if (FetchOp (STP) == FALSE) {
            return (FALSE);
        }
    }
    if (EVAL_IS_ENUM (ST)) {
        SetNodeType (ST, ENUM_UTYPE (ST));
    }
    if (EVAL_IS_ENUM (STP)) {
        SetNodeType (STP, ENUM_UTYPE (STP));
    }
    if (op == OP_eq) {

        // for simple assignment, load both nodes

        if (!EvalUtil (OP_eq, ST, NULL, EU_LOAD)) {
            return (FALSE);
        }
        if (EVAL_IS_BASED (ST) && !((EVAL_TYP (STP) == T_SHORT) ||
          (EVAL_TYP (STP) == T_USHORT) || (EVAL_TYP (STP) == T_INT2) ||
          (EVAL_TYP (STP) == T_UINT2))) {
            // if the value to be stored is a based pointer and the type
            // of the destination is not an int, then normalize the pointer.
            // A based pointer can be stored into an int without normalization.

            if (!NormalizeBase (ST)) {
                return (FALSE);
            }
        }
        if (EVAL_IS_BASED (STP)) {
            // if the location to be stored into is a based pointer and the
            // value to be stored is a pointer or is a long value not equal
            // to zero, then the value is denormalized

            if (EVAL_IS_PTR (ST) ||
              (((EVAL_TYP (ST) == T_LONG) || (EVAL_TYP(ST) == T_ULONG)) &&
              (EVAL_ULONG (ST) != 0L)) ||
              (((EVAL_TYP (ST) == T_INT4) || (EVAL_TYP(ST) == T_UINT4)) &&
              (EVAL_ULONG (ST) != 0L))) {
                //M00KLUDGE - this should go through CastNode
                if (!DeNormalizePtr (ST, STP)) {
                    return (FALSE);
                }
            }
        }
    }
    else {
        // map assignment operator to arithmetic operator
        // push address onto top of stack and load the value and
        // perform operation

        if (!PushStack (STP) || !PushStack (STP)) {
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
        switch (nop = (op_t) eqop[op - OP_multeq]) {
            case OP_plus:
            case OP_minus:
                EVPlusMinus (nop);
                break;

            default:
                EVArith (nop);
        }
        // The top of the stack now contains the value of the memory location
        // modified by the value.  Move the value to the right operand of the
        // assignment operand.

        // M00KLUDGE - this will not work with variable sized stack entries

        *STP = *ST;
        PopStack ();
    }

    // store result

    if (EVAL_IS_BITF (STP)) {
        // store bitfield
        return (SBitField ());
    }
#ifdef NEVER
    // this may cause problems in assignment operators.

    else if (EVAL_IS_ADDR (STP)) {
        // the left value is an address.  If the right value is not an address
        // convert it to a long before moving to the left value.
        // M00KLUDGE - this assumes equivalence between far pointers and longs
        if (!EVAL_IS_ADDR (ST)) {
            CastNode (ST, T_LONG, T_LONG);
        }
        EVAL_VAL (STP) = EVAL_VAL (ST);
    }
#endif
    else if (EVAL_IS_PTR (STP)) {
        // dolphin #2109: use a non-NULL hmod before casting
        // EVAL_MOD(ST) is 0 if ST represents a register
        if (EVAL_MOD(ST) == 0) {
            EVAL_MOD(ST) = EVAL_MOD(STP);
        }
        CastNode (ST, EVAL_TYP (STP), PTR_UTYPE (STP));
        if (ADDR_IS_LI (EVAL_PTR (ST)) == TRUE) {
            SHFixupAddr (&EVAL_PTR (ST));
        }
        EVAL_VAL (STP) = EVAL_VAL (ST);
    }
    else {
        if (EVAL_MOD(ST) == 0) {
            EVAL_MOD(ST) = EVAL_MOD(STP);
        }
        CastNode (ST, EVAL_TYP (STP), EVAL_TYP (STP));
        EVAL_VAL (STP) = EVAL_VAL (ST);
    }
    PopStack ();
    EVAL_STATE (ST) = EV_rvalue;
    return (UpdateMem (ST));
}




/**     FetchOp - fetch pointer value
 *
 *      fSuccess = FetchOp (pv)
 *
 *      Entry   ST = pointer node
 *
 *      Exit    EVAL_SYM (ST) = pointer value
 *
 *      Returns TRUE if pointer value fetched without error
 *              FALSE if error
 */


bool_t FASTCALL FetchOp (peval_t pv)
{

    // load the value and perform implicit type conversions.

    if (!EvalUtil (OP_fetch, pv, NULL, EU_LOAD | EU_TYPE)) {
        return (FALSE);
    }

    // The resultant node is basically identical to the child except
    // that its EVAL_SYM field is equal to the actual contents of
    // the pointer:

    if (EVAL_IS_BASED (pv)) {
        if (!NormalizeBase (pv)) {
            return(FALSE);
        }
    }
    EVAL_SYM (pv) = EVAL_PTR (pv);
    EVAL_SYM_OFF (ST) -= PTR_THISADJUST (ST);
    PTR_THISADJUST (ST) = 0;
    EVAL_STATE (pv) = EV_lvalue;

    // Remove a level of indirection from the resultant type.

    RemoveIndir (pv);
    EVAL_IS_REF (pv) = FALSE;
    return (TRUE);
}




/**     InitConst - initialize constand on evaluation stack
 *
 *      fSuccess = InitConst (const)
 *
 *      Entry   const = constant value
 *
 *      Exit    value field of ST = constant eval node
 *
 *      Returns TRUE if node added without error
 *              FALSE if error
 */


bool_t FASTCALL InitConst (long off)
{
    eval_t      evalT;
    peval_t     pvT;

    pvT = &evalT;
    CLEAR_EVAL (pvT);
    EVAL_STATE (pvT) = EV_constant;
    SetNodeType (pvT, T_LONG);
    EVAL_LONG (pvT) = off;
    return (PushStack (pvT));
}




/**     SBitField - store value into bitfield
 *
 *      fSuccess = SBitField ()
 *
 *      Entry   STP = result bitfield
 *              ST = value
 *
 *      Exit    value field of STP = new field value
 *
 *      Returns TRUE if field inserted without error
 *              FALSE if error
 */


bool_t FASTCALL SBitField ()
{
    ulong       cBits;      // Number of bits in field
    ulong       pos;        // Bit position of field
    ulong       mask;       // Bit mask
    UQUAD       mask_q;     // 64 bit version
    ulong       mask_l;     // 32 bit version
    uchar       mask_c;     // 8 bit version
    CV_typ_t    uType;
    bool_t      retval;

    // get information on the bit field.  Shift counts are limited to 5 bits
    // to emulate the hardware

    pos = BITF_POS (STP) & 0x1f;
    cBits = BITF_LEN (STP);
    uType = BITF_UTYPE (STP);

    if (!CV_IS_PRIMITIVE(uType)) {
        //
        // find out the true type, we can have LF_ENUM as underlying types.
        //
        HTYPE   htype = THGetTypeFromIndex(EVAL_MOD(STP), uType);
        if (htype != 0) {
            //
            // check to see if it is an LF_ENUM.  If so, we set uType
            // to be the underlying type of the enum instead.
            //
            TYPPTR  ptype = (TYPPTR) MHOmfLock(htype);
            if (ptype && ptype->leaf == LF_ENUM) {
                lfEnum *    plf = (lfEnum*) &ptype->leaf;
                uType = plf->utype;
            }
            MHOmfUnLock(htype);
        }
    }

    // check if the underlying type is sufficiently large
    // and use a larger type if necessary

    DASSERT (CV_IS_PRIMITIVE(uType));
    if ((long) (cBits + pos) > (TypeSizePrim(uType) << 3)) {
        switch (uType) {
        case T_CHAR:
        case T_RCHAR:
        case T_INT1:
            uType = T_INT2;
            break;

        case T_UCHAR:
        case T_UINT1:
            uType = T_UINT2;
            break;

        case T_SHORT:
        case T_INT2:
            uType = T_INT4;
            break;

        case T_USHORT:
        case T_UINT2:
            uType = T_UINT4;
            break;

        case T_LONG:
            uType = T_QUAD;
            break;

        case T_ULONG:
            uType = T_UQUAD;
            break;

        case T_INT4:
            uType = T_INT8;
            break;

        case T_UINT4:
            uType = T_UINT8;
            break;

        default:
            DASSERT (FALSE);
            pExState->err_num = ERR_BADOMF;
            return (FALSE);
        }
    }

    PushStack (STP);
    SetNodeType (ST, uType);
    EVAL_STATE (ST) = EV_lvalue;
    if (!LoadSymVal (ST)) {
        return (FALSE);
    }

    CastNode (STP, uType, uType);
    switch (uType) {
        case T_CHAR:
        case T_RCHAR:
        case T_UCHAR:
        case T_INT1:
        case T_UINT1:
            mask_c = (uchar) ((1 << cBits) - 1);
            EVAL_UCHAR (STP) = (uchar) (EVAL_UCHAR (STP) & mask_c);
            EVAL_UCHAR (ST) = (uchar) ((EVAL_UCHAR (ST) &
              ~(mask_c << pos)) | (EVAL_UCHAR (STP) << pos));
            break;

        case T_SHORT:
        case T_USHORT:
        case T_INT2:
        case T_UINT2:
            mask = ((1 << cBits) - 1);
            EVAL_USHORT (STP) = (ushort) (EVAL_USHORT (STP) & mask);
            EVAL_USHORT (ST) = (ushort) ((EVAL_USHORT (ST) &
              ~(mask << pos)) | (EVAL_USHORT (STP) << pos));
            break;

        case T_LONG:
        case T_ULONG:
        case T_INT4:
        case T_UINT4:
            mask_l = ((1L << cBits) - 1);
            EVAL_ULONG (STP) = (ulong) (EVAL_ULONG (STP) & mask_l);
            EVAL_ULONG (ST) = (EVAL_ULONG (ST) &
              ~(mask_l << pos)) | (EVAL_ULONG (STP) << pos);
            break;

        case T_QUAD:
        case T_UQUAD:
        case T_INT8:
        case T_UINT8:
            mask_q = (((UQUAD)1 << cBits) - 1);
            EVAL_UQUAD (STP) = (UQUAD) (EVAL_UQUAD (STP) & mask_q);
            EVAL_UQUAD (ST) = (EVAL_UQUAD (ST) &
              ~(mask_q << pos)) | (EVAL_UQUAD (STP) << pos);
            break;

        default:
            DASSERT (FALSE);
            return (FALSE);
    }
    retval = UpdateMem (ST);
    PopStack ();

    // signed extend if signed type
    switch (uType) {
        case T_CHAR:
        case T_RCHAR:
        case T_INT1:
            EVAL_CHAR (ST) <<= (8 - cBits);
            EVAL_CHAR (ST) >>= (8 - cBits);
            break;

        case T_SHORT:
        case T_INT2:
            EVAL_SHORT (ST) <<= (16 - cBits);
            EVAL_SHORT (ST) >>= (16 - cBits);
            break;

        case T_LONG:
        case T_INT4:
            EVAL_LONG (ST) <<= (32 - cBits);
            EVAL_LONG (ST) >>= (32 - cBits);
            break;

        case T_QUAD:
        case T_INT8:
            EVAL_QUAD (ST) <<= (64 - cBits);
            EVAL_QUAD (ST) >>= (64 - cBits);
            break;
    }
    *STP = *ST;
    PopStack ();
    return (retval);
}




/***    EVPlusMinus - Perform an addition or subtraction operation
 *
 *      fSuccess = EVPlusMinus (op)
 *
 *      Entry   op = OP_plus or OP_minus
 *
 *      Exit    STP = STP op ST and stack popped
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 * DESCRIPTION
 *       Special handling is required when one or both operands are
 *       pointers.  Otherwise, the arguments are passed on to EVArith().
 */


bool_t FASTCALL EVPlusMinus (op_t op)
{
    ulong       cbBase;
    eval_t      evalT;
    peval_t     pvT;

    if (EVAL_IS_REF (ST)) {
        if (FetchOp (ST) == FALSE) {
            return (FALSE);
        }
    }
    if (EVAL_IS_REF (STP)) {
        if (FetchOp (STP) == FALSE) {
            return (FALSE);
        }
    }
    if (EVAL_IS_ENUM (ST)) {
        SetNodeType (ST, ENUM_UTYPE (ST));
    }
    if (EVAL_IS_ENUM (STP)) {
        SetNodeType (STP, ENUM_UTYPE (STP));
    }

    // Check to see if either operand is a pointer.
    // If so, the operation is special.  Otherwise,
    // hand it to EVArith ().

    if (!EVAL_IS_PTR (STP) && !EVAL_IS_PTR (ST)) {
        return (EVArith (op));
    }

    // Load values and perform implicit type coercion if required.

    if (!EvalUtil (op, STP, ST, EU_LOAD)) {
        return (FALSE);
    }

    // Perform the evaluation.  There are two cases:
    //
    // I)  ptr + int, int + ptr, ptr - int
    // II) ptr - ptr
    //
    // Do some common setup first.

    pvT = &evalT;

    if ((op == OP_plus) || !(EVAL_IS_PTR (ST))) {
        // Case (I). ptr + int, int + ptr, ptr - int

        if (!EVAL_IS_PTR (STP)) {
            // Switch so int is on right
            *pvT = *STP;
            *STP = *ST;
            *ST  = *pvT;
        }

        // if pointer node is BP relative, compute actual address
        *pvT = *STP;
        RemoveIndir (pvT);
        cbBase = TypeSize (pvT);
#if 0
        if (EVAL_IS_BPREL (STP)) {
            EVAL_SYM_OFF (STP) += pExState->frame.BP.off;
            EVAL_SYM_SEG (STP) = pExState->frame.SS; //M00FLAT32
            EVAL_IS_BPREL (STP) = FALSE;
            ADDR_IS_LI (EVAL_SYM (ST)) = FALSE;
            SHUnFixupAddr (&EVAL_SYM (ST));
        }
#else
        ResolveAddr(STP);
#endif

        // The resultant node has the same type as the pointer:

        EVAL_STATE (STP) = EV_rvalue;

        // Cast the increment node to an unsigned long.

        CastNode (ST, T_ULONG, T_ULONG);

        // Assign the proper value to the resultant node.

        if (op == OP_plus) {
            EVAL_PTR_OFF (STP) += (UOFFSET)(EVAL_ULONG (ST) * cbBase);
        } else {
            EVAL_PTR_OFF (STP) -= (UOFFSET)(EVAL_ULONG (ST) * cbBase);
        }
    }
    else {
        // Case (II): ptr - ptr.  The result is of type ptrdiff_t and
        // is equal to the distance between the two pointers (in the
        // address space) divided by the size of the items pointed to:

        *pvT = *STP;
        RemoveIndir (pvT);
        cbBase = TypeSize (pvT);

        DASSERT (EVAL_IS_PTR (ST));
        if (!EVAL_IS_PTR (STP) || !fCanSubtractPtrs (ST, STP) ||
            // Dolphin #5530: fail gracefully if cbBase is 0. In that
            // case the underlying type is special (e.g., void) and
            // pointer subtraction is not allowed.
            cbBase == 0) {
            pExState->err_num = ERR_OPERANDTYPES;
            return (FALSE);
        }

        EVAL_STATE (STP) = EV_rvalue;

        // we know we are working with pointers so we do not
        // have to check EVAL_IS_PTR (pv)

        if (EVAL_IS_BASED (STP)) {
            NormalizeBase (STP);
        }
        if (EVAL_IS_BASED (ST)) {
            NormalizeBase (ST);
        }
        if (EVAL_IS_NPTR (STP) || EVAL_IS_FPTR (STP)) {
            SetNodeType (STP, T_SHORT);
            EVAL_SHORT (STP) = (short) (EVAL_PTR_OFF (STP) - EVAL_PTR_OFF (ST));
            EVAL_SHORT (STP) /= (ushort) cbBase;
        }
        else if (EVAL_IS_NPTR32 (STP) || EVAL_IS_FPTR32 (STP)) {
            SetNodeType (STP, T_ULONG);
            EVAL_ULONG (STP) = (UOFF32) (EVAL_PTR_OFF (STP) - EVAL_PTR_OFF (ST));
            EVAL_ULONG (STP) /= cbBase;
        }
        else if (EVAL_IS_PTR64 (STP)) { //v-vadimp - needs review
            SetNodeType (STP, T_UQUAD);
            EVAL_UQUAD(STP) = (UOFF64) (EVAL_PTR_OFF (STP) - EVAL_PTR_OFF (ST));
            EVAL_UQUAD (STP) /= cbBase;
        }
        else {
            SetNodeType (STP, T_LONG);
            //  M00KLUDGE  This will not work in 32 bit mode
            // BUGBUG this does not do 64 bit pointers correctly
            EVAL_LONG (STP) =
              (UOFF32)
              (((((ushort)EVAL_PTR_SEG (STP)) << 16) + EVAL_PTR_OFF (STP))
              - ((((ushort)EVAL_PTR_SEG (ST)) << 16) + EVAL_PTR_OFF (ST)));
              EVAL_LONG (STP) /= cbBase;
        }
    }
    return(PopStack ());
}




/**     EvalRelat - Perform relational and equality operations
 *
 *      fSuccess = EvalRelat (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if no evaluation error
 *              FALSE if evaluation error
 *
 *      Description
 *       If both operands are arithmetic, passes them on to EVArith().
 *       Otherwise (one or both operands pointers), does the evaluation
 *       here.
 *
 */


bool_t FASTCALL EvalRelat (bnode_t bn)
{
    if (!EvalLChild (bn) || !EvalRChild (bn)) {
        return (FALSE);
    }
    return (Relational (NODE_OP (bn)));
}


/**     Relational - Perform relational and equality operations
 *
 *      fSuccess = Relational (op)
 *
 *      Entry   op = OP_lt, OP_lteq, OP_gt, OP_gteq, OP_eqeq, or OP_bangeq
 *
 *      Returns TRUE if no evaluation error
 *              FALSE if evaluation error
 *
 *      Description
 *       If both operands are arithmetic, passes them on to EVArith().
 *       Otherwise (one or both operands pointers), does the evaluation
 *       here.
 *
 */


bool_t FASTCALL Relational (op_t op)
{
    int         result;
    ushort      segL;
    ushort      segR;
    UOFFSET     offL;
    UOFFSET     offR;


    if (EVAL_IS_REF (ST)) {
        if (FetchOp (ST) == FALSE) {
            return (FALSE);
        }
    }
    if (EVAL_IS_REF (STP)) {
        if (FetchOp (STP) == FALSE) {
            return (FALSE);
        }
    }
    if (EVAL_IS_ENUM (ST)) {
        SetNodeType (ST, ENUM_UTYPE (ST));
    }
    if (EVAL_IS_ENUM (STP)) {
        SetNodeType (STP, ENUM_UTYPE (STP));
    }

    // Check to see if either operand is a pointer.
    // If so, the operation is special.  Otherwise,
    // hand it to EVArith ().

    if (!EVAL_IS_PTR (STP) || !EVAL_IS_PTR (ST)) {
        return (EVArith (op));
    }

    if (EvalUtil (op, ST, STP, EU_LOAD | EU_TYPE) == FALSE) {
        return (FALSE);
    }

    // Both nodes should now be typed as either near or far
    // pointers.

    DASSERT (EVAL_IS_PTR (STP) && EVAL_IS_PTR (ST));

    //  For the relational operators ('<', '<=', '>', '>='),
    //  only offsets are compared.  For the equality operators ('==', '!='),
    //  both segments and offsets are compared.

    if (ADDR_IS_LI (EVAL_PTR (STP))) {
        SHFixupAddr (&EVAL_PTR (STP));
    }
    if (ADDR_IS_LI (EVAL_PTR (ST))) {
        SHFixupAddr (&EVAL_PTR (ST));
    }

    segL = (ushort)EVAL_PTR_SEG (STP);
    segR = (ushort)EVAL_PTR_SEG (ST);
    offL = EVAL_PTR_OFF (STP);
    offR = EVAL_PTR_OFF (ST);

    switch (op) {
        case OP_lt:
            result = (offL < offR);
            break;

        case OP_lteq:
            result = (offL <= offR);
            break;

        case OP_gt:
            result = (offL > offR);
            break;

        case OP_gteq:
            result = (offL >= offR);
            break;

        case OP_eqeq:
            if (0 == offL && 0 == offR &&
                EVAL_PTRTYPE(ST) == CV_PTR_NEAR &&
                EVAL_PTRTYPE(STP) == CV_PTR_NEAR) {
                result = 1;
            }
            else {
                result = ((segL == segR) && (offL == offR));
            }
            break;

        case OP_bangeq:
            if (0 == offL && 0 == offR &&
                EVAL_PTRTYPE(ST) == CV_PTR_NEAR &&
                EVAL_PTRTYPE(STP) == CV_PTR_NEAR) {
                result = 0;
            }
            else {
                result = ((segL != segR) || (offL != offR));
            }
            break;

        default:
            //DASSERT (FALSE);
            pExState->err_num = ERR_INTERNAL;
            return (FALSE);
    }
    EVAL_STATE (STP) = EV_rvalue;
    if (pExState->state.f32bit) {
        EVAL_LONG (STP) = (long) result;
        SetNodeType (STP, T_INT4);
    }
    else {
        EVAL_SHORT (STP) = (short) result;
        SetNodeType (STP, T_INT2);
    }
    return (PopStack ());
}

 /**    EvalRetVal - Evaluate Return Value of current function
 *
 *      fSuccess = EvalRetVal (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if no evaluation error
 *              FALSE if evaluation error
 *
 *      Description
 *          Evaluates return value of current function
 *
 */


bool_t FASTCALL EvalRetVal (bnode_t bn)
{
    peval_t     pv = &bn->v[0];

    if (!PushStack (pv))
        return FALSE;

    if (EVAL_IS_CLASS (ST)) {
        SHREG reg = {0};
        // this case needs special handling, since
        // eax contains a pointer to the return value
        switch (TargetMachine) {
            case mptmips:
                reg.hReg = CV_M4_IntV0;
                break;

            case mptdaxp:
                reg.hReg = CV_ALPHA_IntV0;
                break;

            case mptm68k:
                DASSERT(FALSE);
                break;

            case mptix86:
                reg.hReg = CV_REG_EAX;
                break;

            case mptia64:
                reg.hReg = CV_IA64_IntV0;
                break;

            case mptmppc:
            case mptntppc:
                reg.hReg = CV_PPC_GPR3;
                break;

            default:
                DASSERT(FALSE);

        }
        if (GetReg (&reg, pCxt, NULL) == NULL) {
            pExState->err_num = ERR_REGNOTAVAIL;
            return (FALSE);
        }
        EVAL_SYM_OFF (ST) = reg.Byte4;
        ADDRLIN32( EVAL_SYM(ST) );
    }
#if defined (TARGMAC68K)
    else if (FCN_CALL (ST) == FCN_PASCAL)
    {
        SHREG reg = {0};

        reg.hReg = CV_R68_A7;

        if (GetReg (&reg, pCxt, NULL) == NULL) {
            pExState->err_num = ERR_REGNOTAVAIL;
            return (FALSE);
        }

        EVAL_SYM_OFF (ST) = reg.Byte4;
        ADDRLIN32( EVAL_SYM(ST) );
    }
#endif

    return TRUE;
}


/***    EvalUScope - Do unary :: scoping
 *
 *      fSuccess = EvalUScope (bn);
 *
 *      Entry   pvRes = based pointer to unary scoping node
 *
 *      Exit    pvRes = evaluated left node of pvRes
 *
 *      Returns TRUE if evaluation successful
 *              FALSE if error
 */


bool_t FASTCALL EvalUScope (bnode_t bn)
{
    register bool_t retval;
    CXT     cxt;

    // save current context packet and set current context to module scope

    cxt = *pCxt;
    SHGetCxtFromHmod (SHHMODFrompCXT (pCxt), pCxt);
    retval = EvalLChild (bn);
    *pCxt = cxt;
    return (retval);
}





/***    DoBScope - Do binary :: scoping
 *
 *      fSuccess = DoBScope (pn);
 *
 *      Entry   pvRes = pointer to binary scoping node
 *
 *      Returns TRUE if evaluation successful
 *              FALSE if error
 */


bool_t FASTCALL EvalBScope (bnode_t bn)
{
    peval_t     pv;

    pv = &NODE_LCHILD (bn)->v[0];

    if (CLASS_NAMESPACE (pv)) {
        return EvalPushNode (bn);
    }

    if (CLASS_GLOBALTYPE (pv)) {
        // the left member of the scope operator was a type not in class
        // scope.  We presumably have an empty stack so we need to fake
        // up a stack entry

        //if (ST == NULL)
        // Dolphin 5503: We GP-faulted while binding an expression like
        // Foo::m_array[Foo::m_staticCount], because we needed to fake
        // up a stack entry even if ST was not NULL.
        // Now we always push a faked entry unless there is a preceding
        // bnOp (".", "->", etc); in that case a class stack entry has
        // already been pushed onto the stack.
        if (! CLASS_FOLLOWSBNOP (pv))
            PushStack (pv);

    }
    return (StructEval (bn));
}




/***    EvalPreIncDec - Do ++expr or --expr
 *
 *      fSuccess = EvalPreIncDec (bnode_t bn);
 *
 *      Entry   bn = based pointer to node
 *
 *      Exit    ST decremented or incremented
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL EvalPreIncDec (bnode_t bn)
{
    op_t    nop = OP_plus;

    if (!EvalLChild (bn)) {
        return(FALSE);
    }

    if (NODE_OP (bn) == OP_predec) {
        nop = OP_minus;
    }

    // push the entry on the stack and then perform incmrement/decrement

    PushStack (ST);

    //  load left node and store as return value

    if (EvalUtil (nop, ST, NULL, EU_LOAD)) {
        //  do the post-increment or post-decrement operation and store

        if (EVPrePost (nop)) {
            if (Assign (OP_eq)) {
                return (TRUE);
            }
        }
    }
    return (FALSE);
}




/***    EvalPostIncDec - Do expr++ or expr--
 *
 *      fSuccess = EvalPostIncDec (op);
 *
 *      Entry   bn = based pointer to node
 *
 *      Exit    ST decremented or incremented
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL EvalPostIncDec (bnode_t bn)
{
    eval_t      evalT;
    peval_t     pvT = &evalT;
    op_t        nop = OP_plus;

    if (!EvalLChild (bn)) {
        return(FALSE);
    }
    if (NODE_OP (bn) == OP_postdec) {
        nop = OP_minus;
    }

    // push the entry on the stack and then perform incmrement/decrement

    PushStack (ST);

    //  load left node and store as return value

    if (EvalUtil (nop, ST, NULL, EU_LOAD)) {
        *pvT = *ST;

        //  do the post-increment or post-decrement operation and store

        if (EVPrePost (nop)) {
            if (Assign (OP_eq)) {
                *ST =  *pvT;
                return (TRUE);
            }
        }
    }
    return (FALSE);
}




/**     EVPrePost - perform the increment operation
 *
 *      fSuccess = EVPrePost (op);
 *
 *      Entry   op = operation to perform (OP_plus or OP_minus)
 *
 *      Exit    increment/decrement performed and result stored in memory
 *              DebErr set if error
 *
 *      Returns TRUE if no error
 *              FALSE if error
 */


INLINE bool_t EVPrePost (op_t op)
{
    if (InitConst (1) == TRUE) {
        return (EVPlusMinus (op));
    }
    return (FALSE);
}




/***    EvalBang - Perform logical negation operation
 *
 *      fSuccess = EvalBang (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Exit    ST = pointer to negated value
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 * DESCRIPTION
 *       Checks for a pointer operand; if found, handles it here, otherwise
 *       passes it on to EVUnary ().
 */


bool_t FASTCALL EvalBang (bnode_t bn)
{
    int         result;
    ushort      seg;
    UOFFSET     off;

    if (!EvalLChild (bn)) {
        return (FALSE);
    }

    // load the value and perform implicit type conversion.

    if (!EvalUtil (OP_bang, ST, NULL, EU_LOAD)) {
        return (FALSE);
    }
    if (EVAL_IS_REF (ST)) {
        if (FetchOp (ST) == FALSE) {
            return (FALSE);
        }
    }

    // If the operand is not of pointer type, just pass it on to EVUnary

    if (!(EVAL_IS_PTR (ST)))
        return (EVUnary (OP_bang));

    // The result is 1 if the pointer is a null pointer and 0 otherwise
    // Note that for a near pointer, we compare the offset against
    // 0, while for a far pointer we compare both segment and offset.

    seg = (ushort)EVAL_PTR_SEG (ST);
    off = EVAL_PTR_OFF (ST);

    if (EVAL_IS_NPTR (ST) || EVAL_IS_NPTR32 (ST)) {
        result = (off == 0);
    }
    else {
        result = ((seg == 0) && (off == 0));
    }

    CLEAR_EVAL (ST);
    EVAL_STATE (ST) = EV_rvalue;
    if (pExState->state.f32bit) {
        EVAL_LONG (ST) = (long) result;
        return (SetNodeType (ST, T_INT4));
    }
    else {
        EVAL_SHORT (ST) = (short) result;
        return (SetNodeType (ST, T_INT2));
    }
}





/***    EvalDMember - Perform a dot member access ('.*')
 *
 *      fSuccess = EvalDMember (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Exit    ST = value of member
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL EvalDMember (bnode_t bn)
{
    Unreferenced(bn);

    pExState->err_num = ERR_OPNOTSUPP;
    return (FALSE);     //M00KLUDGE - not implemented
#ifdef NEVER
    // Resolve identifiers, if any, but do NOT resolve the right-child
    // identifier.  For example, 'foo' could be both a local (or global)
    // variable and the name of a structure element.

    if (!ResolveIdent (ST))
        return (FALSE);

    if (NODE_OP (pnRight) != OP_ident) {
        pExState->err_num = ERR_SYNTAX;
        return (FALSE);
    }

    // Attempt to find the structure or class element.

    if (EVAL_IS_STRUCT (ST)) {
        if (!SetStructRef (pnRight)) {
            pExState->err_num = ERR_NOTELEMENT;
            return (FALSE);
        }
    }
    else if (EVAL_IS_CLASS (ST)) {
        if (!SetClassRef (pnRight)) {
            pExState->err_num = ERR_NOTELEMENT;
            return (FALSE);
        }
    }
    else {
        pExState->err_num = ERR_NEEDSTRUCT;
        return (FALSE);
    }
    return (TRUE);
#endif
}




/***    EvalPMember - Perform a pointer to member access ('->*')
*
* SYNOPSIS
*       fSuccess = EvalPMember (bn)
*
* ENTRY
*       pvRes           Pointer to node in which result is to be stored
*       pvLeft      Pointer to left operand node
*       pvRight     Pointer to right operand node
*
* RETURNS
*       TRUE if successful, FALSE if not and sets DebErr
*
* DESCRIPTION
*
* NOTES
*/

bool_t FASTCALL EvalPMember (bnode_t bn)
{
    Unreferenced(bn);

    // Check to make sure the left operand is a struct/union pointer.
    // To do this, remove a level of indirection from the node's type
    // and see if it's a struct or union.


    pExState->err_num = ERR_OPNOTSUPP;
    return (FALSE);
}




/***    EvalPointsTo - Perform a structure access ('->')
 *
 *      fSuccess = EvalPointsTo (bn)
 *
 *      Entry  bRight = based pointer to node
 *
 *      Exit    ST = value node for member
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 */


bool_t FASTCALL EvalPointsTo (bnode_t bn)
{

    if (!EvalLChild (bn)) {
        return (FALSE);
    }
    if (!FetchOp (ST)) {
        return (FALSE);
    }

    // The result is simple -- (*ST).pnRight.  The call to StructEval ()
    // will set the error code if it fails.

    if (EVAL_STATE (ST) == EV_rvalue) {
        return (StructElem (bn));
    }
    else {
        return (StructEval (bn));
    }
}




/***    EvalArray - Perform an array access ('[]')
 *
 *      fSuccess = EvalArray (bn)
 *
 *      Entry   bn = based pointer to array node
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 *      Obtains the contents of an array member.  This is done by
 *      calling EVPlusMinus() and DoFetch().
 */


bool_t FASTCALL EvalArray (bnode_t bn)
{
    ulong       index;
    eval_t      evalT;
    peval_t     pvT;

    if (EvalLChild (bn) && EvalRChild (bn)) {
        if (EVAL_IS_ARRAY (STP) || EVAL_IS_ARRAY (ST)) {
            // above check is for array[3] or 3[array]
            if (EvalUtil (OP_lbrack, STP, ST, EU_LOAD) && EVPlusMinus (OP_plus)) {
                return (FetchOp (ST));
            }
        }
        else if (EVAL_IS_PTR (STP)) {
            // this code is a hack to allow the locals and quick watch windows
            // display the virtual function table.  The GetChildTM generates
            // an expression of the form a.__vfuncptr[n].  This means that the
            // evaluation of a.__vfuncptr on the left sets STP to a pointer
            // node and ThisAddress to the adjusted this pointer.  Note that
            // it turns out that symbol address of STP and ThisAddress are
            // the same

            pvT = &evalT;
            *pvT = *STP;
            SetNodeType (pvT, PTR_UTYPE (pvT));
            if (EVAL_IS_VTSHAPE (pvT) && (EVAL_STATE (ST) == EV_constant) &&
              ((index = EVAL_USHORT (ST)) < VTSHAPE_COUNT (pvT))) {
                PopStack ();
                DASSERT ((EVAL_SYM_OFF (pvThis) == EVAL_SYM_OFF (ST)) &&
                  (EVAL_SYM_SEG (pvThis) == EVAL_SYM_SEG (ST)));
                return (VFuncAddress (ST, index));
            }
            else {
                if (EvalUtil (OP_lbrack, STP, ST, EU_LOAD) && EVPlusMinus (OP_plus)) {
                    return (FetchOp (ST));
                }
            }
        }
    }
    return (FALSE);
}




/***    EvalCast - Perform a type cast operation
 *
 *      fSuccess = EvalCast (bn)
 *
 *      Entry   bn = based pointer to cast node
 *
 *      Exit
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 */


bool_t FASTCALL EvalCast (bnode_t bn)
{
    peval_t     pv;
    peval_t     pvL;

    if (!EvalRChild (bn) || !EvalUtil (OP_cast, ST, NULL, EU_LOAD)) {
        return (FALSE);
    }
    DASSERT (!EVAL_IS_CLASS (ST));

    // Cast the node to the desired type.  if the cast node is a member node,
    // then the stack top is cast by changing the pointer value by the amount
    // in the value and then setting the type of the stack top to the type
    // of the left node

    pv = (peval_t)&bn->v[0];
    pvL = &(NODE_LCHILD (bn))->v[0];
    if (EVAL_MOD (pvL) != 0) {
        EVAL_MOD (ST) = EVAL_MOD (pvL);
    }
    if (EVAL_IS_MEMBER (pv) == TRUE) {
        // a cast of pointer to derived to pointer to base is not done
        // for a null value
        if (EVAL_PTR_OFF (ST) != 0) {
            if (Eval ((bnode_t)MEMBER_THISEXPR (pv)) == FALSE) {
                return (FALSE);
            }
            *ST = *pvThis;
            EVAL_STATE (ST) = EV_rvalue;
            EVAL_PTR (ST) = EVAL_SYM (pvThis);
        }
        return (SetNodeType (ST, EVAL_TYP (pvL)));
    }
    else {
        if (EVAL_IS_PTR (pvL)) {
            return (CastNode (ST, EVAL_TYP (pvL), PTR_UTYPE (pvL)));
        }
        else {
            return (CastNode (ST, EVAL_TYP (pvL), EVAL_TYP (pvL)));
        }
    }
}




/***    EvalByteOps - Handle 'by', 'wo' and 'dw' operators
 *
 *      fSuccess = EvalByteOps (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Exit    ST = pointer to value
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 * DESCRIPTION
 *       Evaluates the contents of the address operand as a byte
 *       ('by'), word ('wo') or dword ('dw'):
 *
 *       Operand     Result
 *       -------     ------
 *       <register>  *(uchar *)<register>
 *       <address>   *(uchar *)<address>
 *       <variable>  *(uchar *)&variable
 *
 *       Where (uchar *) is replaced by (uint *) for the 'wo' operator,
 *       or by (ulong *) for the 'dw' operator.
 *
 */


bool_t FASTCALL EvalByteOps (bnode_t bn)
{
    CV_typ_t   type;
    register op_t    op;

    if (!EvalLChild (bn)) {
        return (FALSE);
    }

    // If the operand is an lvalue and it is a register,
    // load the value of the register; otherwise, use the
    // address of the variable.
    //
    // If the operand is not an lvalue, use its value as is.

    if (EVAL_STATE (ST) == EV_lvalue) {
        if (EVAL_IS_REG (ST)) {
            if (!LoadVal (ST)) {
                pExState->err_num = ERR_INTERNAL;
                return (FALSE);
            }
            else {
                type = T_USHORT;
            }
        }
        else {
            if ((type = NODE_STYPE (bn)) == 0) {
               // unable to find proper pointer type
               return (FALSE);
            }
            EVAL_STATE (ST) = EV_rvalue;
#if 0
            if (EVAL_IS_BPREL (ST)) {
                EVAL_SYM_OFF (ST) += pExState->frame.BP.off;
                EVAL_SYM_SEG (ST) = pExState->frame.SS;
                EVAL_IS_BPREL (ST) = FALSE;
                ADDR_IS_LI (EVAL_SYM (ST)) = FALSE;
                SHUnFixupAddr (&EVAL_SYM (ST));
            }
#else
            ResolveAddr(ST);
#endif
            EVAL_PTR (ST) = EVAL_SYM (ST);
            SetNodeType (ST, type);
        }
    }

    // Now cast the node to (char far *), (int far *) or
    // (long far *).  If the type is char, uchar, short
    // or ushort, we want to first cast to (char *) so
    // that we properly DS-extend (casting (int)8 to (char
    // far *) will give the result 0:8).

    type = EVAL_TYP (ST);

    //DASSERT(IS_PRIMITIVE (typ));

    if (CV_TYP_IS_REAL (type)) {
        pExState->err_num = ERR_OPERANDTYPES;
        return (FALSE);
    }
    if ((op = NODE_OP (bn)) == OP_by) {
        type = pExState->state.f32bit ? T_32PFUCHAR : T_PFUCHAR;
    }
    else if (op == OP_wo) {
        type = pExState->state.f32bit ? T_32PFUSHORT : T_PFUSHORT;
    }
    else if (op == OP_dw) {
        type = pExState->state.f32bit ? T_32PFULONG : T_PFULONG;
    }
    if (CastNode (ST, type, type) == FALSE) {
        return (FALSE);
    }
    return (FetchOp (ST));
}




/***    EvalSegOp - Handle ':' segmentation operator
 *
 *      fSuccess = EvalSegOp (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Exit    EVAL_SYM (ST) = seg (STP): offset (ST)
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 * DESCRIPTION
 *       Both operands must have integral values (but cannot
 *       be long or ulong).  The result of op1:op2 is a (char
 *       far *) with segment equal to op1 and offset equal to
 *       op2.
 *
 * NOTES
 */

bool_t FASTCALL EvalSegOp (bnode_t bn)
{
    bool_t fLin32 = FALSE;

    if (!EvalLChild (bn) || !EvalRChild (bn)) {
        return(FALSE);
    }

    // check to make sure that neither operand is of type long or ulong.

    switch ( EVAL_TYP ( STP ) ) {
        case T_INT4:
        case T_UINT4:
            if ( VAL_ULONG( STP ) <= 0xffff ) {
                // [cuda #3035 8th Apr 93 sanjays]
                // An expression 0x0:0x0001 has its left operand typed INT by
                // the EEParser and rightly so because it doesn't know the context
                // We have to allow type int for the segment portion but make
                // sure the value is less than 0xffff.
                // We just look at the unsigned value so we will not accept negative
                // numbers either.
                break;
                }
            // else { fallthrough}
        case T_LONG:
        case T_ULONG:
        case T_QUAD:
        case T_UQUAD:
        case T_INT8:
        case T_UINT8:
            pExState->err_num = ERR_OPERANDTYPES;
            return (FALSE);

        default:
            break;
    }

    // set linear mode

    switch ( EVAL_TYP ( ST ) ) {
        case T_LONG:
        case T_ULONG:
        case T_INT4:
        case T_UINT4:
            fLin32 = TRUE;

        default:
            break;
    }

    // Load values and perform implicit type coercion if required.

    if (!EvalUtil (OP_segop, STP, ST, EU_LOAD | EU_TYPE))
        return(FALSE);

      //DASSERT ((EVAL_TYP (STP) == T_SHORT) || (EVAL_TYP (STP) == T_USHORT));
      //DASSERT ((EVAL_TYP (ST)  == T_SHORT) || (EVAL_TYP (ST)  == T_USHORT));

    EVAL_STATE (STP) = EV_rvalue;
    EVAL_PTR_SEG (STP) = EVAL_USHORT (STP);
    EVAL_PTR_OFF (STP) = fLin32 ? EVAL_ULONG (ST) : EVAL_USHORT (ST);
    //REVIEW -billjoy - do we need to set fFlat?
    ADDR_IS_OFF32 ( EVAL_PTR (STP) ) = pExState->state.f32bit ? 1 : 0;
    SetNodeType (STP, (CV_typ_t) (pExState->state.f32bit ? T_32PFCHAR
        : T_PFCHAR));
    return (PopStack ());
}




/**     EVUnary - Evaluate the result of a unary arithmetic operation
 *
 *      fSuccess = EVUnary (op)
 *
 *      Entry   op = Operator (OP_...)
 *
 *      Returns TRUE if no error during evaluation
 *              FALSE if error during evaluation
 *
 * DESCRIPTION
 *       Evaluates the result of an arithmetic operation.  The unary operators
 *       dealt with here are:
 *
 *       !       ~       -      +
 *
 *       Pointer arithmetic is NOT handled; all operands must be of
 *       arithmetic type.
 */


bool_t FASTCALL EVUnary (op_t op)
{
    bool_t      fIsFloat = FALSE;
    bool_t      fIsDouble = FALSE;
    bool_t      fIsLDouble = FALSE;
    bool_t      fIsSigned;
    bool_t      fResInt;
    int         iRes;
    QUAD        lRes, lL;
    UQUAD       ulRes, ulL;
    FLOAT10     ldRes, ldL;
    double      dRes, dL;
    float       fRes, fL;
    CV_typ_t    typRes;

    if (EVAL_IS_REF (ST)) {
        if (FetchOp (ST) == FALSE) {
            return (FALSE);
        }
    }

    // Load the values and perform implicit type conversion.

    if (!EvalUtil (op, ST, NULL, EU_LOAD | EU_TYPE))
        return (FALSE);

    // The resultant type is the same as the type of the left-hand
    // side (assume for now we don't have the special int-result case).

    typRes = EVAL_TYP (ST);
    if (CV_TYP_IS_REAL (typRes) == TRUE) {
        fIsFloat = (CV_SUBT (typRes) == CV_RC_REAL32);
        fIsDouble = (CV_SUBT (typRes) == CV_RC_REAL64);
        fIsLDouble = (CV_SUBT (typRes) == CV_RC_REAL80);
    }
    fIsSigned = CV_TYP_IS_SIGNED (typRes);
    fResInt = FALSE;

    // Common code.  Since we're going to do most of our arithmetic
    // in either long, ulong or double, we do the casts and get the
    // value of the operands here rather than repeating this code
    // in each arm of the switch statement.

    // Note: We now cast to  (U)QUAD instead of (u)long, in order
    // to support __int64 bit operations

    if (fIsFloat) {
        fL = EVAL_FLOAT (ST);
    }
    else if (fIsDouble) {
        dL = EVAL_DOUBLE (ST);
    }
    else if (fIsLDouble) {
        ldL = EVAL_LDOUBLE (ST);
    }
    else if (fIsSigned) {
        CastNode (ST, T_QUAD, T_QUAD);
        lL = EVAL_QUAD (ST);
    }
    else {
        // unsigned
        CastNode (ST, T_UQUAD, T_UQUAD);
        ulL = EVAL_UQUAD (ST);
    }

    // Finally, do the actual arithmetic operation.

    switch (op) {
        case OP_bang:
            // Operand is of arithmetic type; result is always int.

            fResInt = TRUE;
            if (fIsFloat) {
                iRes = !fL;
            }
            else if (fIsDouble) {
                iRes = !dL;
            }
            else if (fIsLDouble) {
                iRes = !LongFromFloat10 ( ldL );
            }
            else if (fIsSigned) {
                iRes = !lL;
            }
            else {
                iRes = !ulL;
            }
            break;

        case OP_tilde:
            // operand must be integral.

            //DASSERT (!fIsReal);

            if (fIsSigned) {
                lRes = ~lL;
            }
            else {
                ulRes = ~ulL;
            }
            break;

        case OP_negate:
            if (fIsFloat) {
                fRes = -fL;
            }
            else if (fIsDouble) {
                dRes = -dL;
            }
            else if (fIsLDouble) {
                ldRes = Float10Negate ( ldL );
            }
            else if (fIsSigned) {
                lRes = -lL;
            }
            else {
                ulRes = -(QUAD)ulL;
            }
            break;

        case OP_uplus:
            if (fIsFloat) {
                fRes = fL;
            }
            else if (fIsDouble) {
                dRes = dL;
            }
            else if (fIsLDouble) {
                ldRes = ldL;
            }
            else if (fIsSigned) {
                lRes = lL;
            }
            else {
                ulRes = ulL;
            }
            break;

        default:
            DASSERT (FALSE);
            pExState->err_num = ERR_INTERNAL;
            return (FALSE);

    }

    // Now set up the resultant node and coerce back to the correct
    // type:

    EVAL_STATE (ST) = EV_rvalue;

    if (fResInt) {
        if (pExState->state.f32bit) {
            EVAL_LONG (ST) = (long) iRes;
            SetNodeType (ST, T_INT4);
        }
        else {
            EVAL_SHORT (ST) = (short) iRes;
            SetNodeType (ST, T_INT2);
        }
    }
    else {
        if (fIsFloat) {
            EVAL_FLOAT (ST) = fRes;
            SetNodeType (ST, T_REAL32);
        }
        else if (fIsDouble) {
            EVAL_DOUBLE (ST) = dRes;
            SetNodeType (ST, T_REAL64);
        }
        else if (fIsLDouble) {
            EVAL_LDOUBLE (ST) = ldRes;
            SetNodeType (ST, T_REAL80);
        }
        else if (fIsSigned) {
            EVAL_QUAD (ST) = lRes;
            SetNodeType (ST, T_QUAD);
        }
        else {
            EVAL_UQUAD (ST) = ulRes;
            SetNodeType (ST, T_UQUAD);
        }
        CastNode (ST, typRes, typRes);
    }

    return(TRUE);
}




/**     EVArith - Evaluate the result of an arithmetic operation
 *
 *      fSuccess = EVArith (op)
 *
 *      Entry   op = Operator (OP_...)
 *
 *      Returns TRUE if no error during evaluation
 *              FALSE if error during evaluation
 *
 * DESCRIPTION
 *       Evaluates the result of an arithmetic operation.  The operators
 *       dealt with here are:
 *
 *       *       /       %
 *       +       -
 *       ==      !=
 *       <       <=      >       >=
 *       <<      >>
 *       &       ^       |
 *
 *       Pointer arithmetic is NOT handled; all operands must be of
 *       arithmetic type.
 */


bool_t FASTCALL EVArith (op_t op)
{
    bool_t      fIsFloat = FALSE;
    bool_t      fIsDouble = FALSE;
    bool_t      fIsLDouble = FALSE;
    bool_t      fIsSigned;
    bool_t      fResInt;
    bool_t      lNearNullPtr = FALSE;
    bool_t      rNearNullPtr = FALSE;
    int         iRes;
    QUAD        lRes, lL, lR;
    UQUAD       ulRes, ulL, ulR;
    float       fRes, fL, fR;
    double      dRes, dL, dR;
    FLOAT10     ldRes, ldL, ldR;
    CV_typ_t    typRes;

    if (EVAL_IS_REF (STP)) {
        if (FetchOp (STP) == FALSE) {
            return (FALSE);
        }
    }
    if (EVAL_IS_REF (ST)) {
        if (FetchOp (ST) == FALSE) {
            return (FALSE);
        }
    }
    if (EVAL_IS_ENUM (ST)) {
        SetNodeType (ST, ENUM_UTYPE (ST));
    }
    if (EVAL_IS_ENUM (STP)) {
        SetNodeType (STP, ENUM_UTYPE (STP));
    }

    // A NULL near pointer should compare equal to 0, although
    // its offset may be non-0 --gdp 9/22/92
    if (EVAL_IS_PTR(ST) && EVAL_PTRTYPE(ST) == CV_PTR_NEAR && EVAL_PTR_OFF(ST) == 0) {
        rNearNullPtr = TRUE;
    }
    if (EVAL_IS_PTR(STP) && EVAL_PTRTYPE(STP) == CV_PTR_NEAR && EVAL_PTR_OFF(STP) == 0) {
        lNearNullPtr = TRUE;
    }

            // Resolve identifiers and check the node types.  If the nodes
    // pass validation, they should not be pointers (only arithmetic
    // operands are handled by this routine).

    // Load the values and perform implicit type conversion.

    if (!EvalUtil (op, STP, ST, EU_LOAD | EU_TYPE))
        return (FALSE);

    // The resultant type is the same as the type of the left-hand
    // side (assume for now we don't have the special int-result case).

    typRes = EVAL_TYP (STP);

    if (CV_TYP_IS_REAL (typRes) == TRUE) {
        fIsFloat = (CV_SUBT (typRes) == CV_RC_REAL32);
        fIsDouble = (CV_SUBT (typRes) == CV_RC_REAL64);
        fIsLDouble = (CV_SUBT (typRes) == CV_RC_REAL80);
    }
    fIsSigned = CV_TYP_IS_SIGNED (typRes);
    fResInt = FALSE;

    // Common code.  Since we're going to do most of our arithmetic
    // in either long, ulong or double, we do the casts and get the
    // value of the operands here rather than repeating this code
    // in each arm of the switch statement.

    if (fIsLDouble) {
        CastNode (STP, T_REAL80, T_REAL80);
        ldL = EVAL_LDOUBLE (STP);
        CastNode (ST, T_REAL80, T_REAL80);
        ldR = EVAL_LDOUBLE (ST);
    }
    else if (fIsDouble) {
        CastNode (STP, T_REAL64, T_REAL64);
        dL = EVAL_DOUBLE (STP);
        CastNode (ST, T_REAL64, T_REAL64);
        dR = EVAL_DOUBLE (ST);
    }
    else if (fIsFloat) {
        CastNode (STP, T_REAL32, T_REAL32);
        fL = EVAL_FLOAT (STP);
        CastNode (ST, T_REAL32, T_REAL32);
        fR = EVAL_FLOAT (ST);
    }
    else if (fIsSigned) {
        CastNode (STP, T_QUAD, T_QUAD);
        lL = EVAL_QUAD (STP);
        CastNode (ST, T_QUAD, T_QUAD);
        lR = EVAL_QUAD (ST);
    }
    else {
        CastNode (STP, T_UQUAD, T_UQUAD);
        ulL = EVAL_UQUAD (STP);
        CastNode (ST, T_UQUAD, T_UQUAD);
        ulR = EVAL_UQUAD (ST);
    }

    // Finally, do the actual arithmetic operation.

    switch (op) {
        case OP_eqeq:
        case OP_bangeq:
        case OP_lt:
        case OP_gt:
        case OP_lteq:
        case OP_gteq:
        {
            // This is kind of a strange way to do things, but it should
            // be pretty obvious what's going on, and it saves code.

            int fEq;
            int fLt;

            fResInt = TRUE;

            if (fIsLDouble) {
                fEq = Float10Equal (ldL, ldR);
                fLt = Float10LessThan (ldL, ldR);
            }
            else if (fIsDouble) {
                fEq = (dL == dR);
                fLt = (dL < dR);
            }
            else if (fIsFloat) {
                fEq = (fL == fR);
                fLt = (fL < fR);
            }
            else if (fIsSigned) {
                fEq = (lL == lR);
                fLt = (lL < lR);
            }
            else {
                fEq = lNearNullPtr && (ulR == 0) ||
                      rNearNullPtr && (ulL == 0) ||
                      (ulL == ulR);
                fLt = (ulL < ulR);
            }

            switch (op) {
                case OP_eqeq:
                    iRes = fEq;
                    break;

                case OP_bangeq:
                    iRes = !fEq;
                    break;

                case OP_lt:
                    iRes = fLt;
                    break;

                case OP_gt:
                    iRes = !fLt && !fEq;
                    break;

                case OP_lteq:
                    iRes = fLt || fEq;
                    break;

                case OP_gteq:
                    iRes = !fLt || fEq;
                    break;
            }
        }
        break;

        case OP_plus:
            if (fIsLDouble) {
                ldRes = Float10OpPlus (ldL, ldR);
            }
            else if (fIsDouble) {
                dRes = dL + dR;
            }
            else if (fIsFloat) {
                fRes = fL + fR;
            }
            else if (fIsSigned) {
                lRes = lL + lR;
            }
            else {
                ulRes = ulL + ulR;
            }
            break;

        case OP_minus:
            if (fIsLDouble) {
                ldRes = Float10OpMinus (ldL, ldR);
            }
            else if (fIsDouble) {
                dRes = dL - dR;
            }
            else if (fIsFloat) {
                fRes = fL - fR;
            }
            else if (fIsSigned) {
                lRes = lL - lR;
            }
            else {
                ulRes = ulL - ulR;
            }
            break;

        case OP_mult:
            if (fIsLDouble) {
                ldRes = Float10OpMul (ldL, ldR);
            }
            else if (fIsDouble) {
                dRes = dL * dR;
            }
            else if (fIsFloat) {
                fRes = fL * fR;
            }
            else if (fIsSigned) {
                lRes = lL * lR;
            }
            else {
                ulRes = ulL * ulR;
            }
            break;

        case OP_div:
            // This looks big, but I can't figure out how to do it
            // with ||'s.  Besides, Hans claims it will tail merge
            // on the error conditions anyway.

            if (fIsLDouble) {
                if (Float10Equal (ldR, Float10Zero())) {
                    pExState->err_num = ERR_DIVIDEBYZERO;
                    return (FALSE);
                }
                ldRes = Float10OpDiv ( ldL, ldR );
            }
            else if (fIsDouble) {
                if (dR == (double)0.0) {
                    pExState->err_num = ERR_DIVIDEBYZERO;
                    return (FALSE);
                }
                dRes = dL / dR;
            }
            else if (fIsFloat) {
                if (fR == (float)0.0) {
                    pExState->err_num = ERR_DIVIDEBYZERO;
                    return (FALSE);
                }
                fRes = fL / fR;
            }
            else if (fIsSigned) {
                if (lR == (long)0) {
                    pExState->err_num = ERR_DIVIDEBYZERO;
                    return (FALSE);
                }
                lRes = lL / lR;
            }
            else {
                if (ulR == (unsigned long)0) {
                    pExState->err_num = ERR_DIVIDEBYZERO;
                    return (FALSE);
                }
                ulRes = ulL / ulR;
            }
            break;

        case OP_mod:
            // Both operands must be integral.

            //DASSERT(!fIsReal);

            if (fIsSigned) {
                if (lR == (long)0) {
                    pExState->err_num = ERR_DIVIDEBYZERO;
                    return (FALSE);
                }
                lRes = lL % lR;
            }
            else {
                if (ulR == (unsigned long)0) {
                    pExState->err_num = ERR_DIVIDEBYZERO;
                    return (FALSE);
                }
                ulRes = ulL % ulR;
            }
            break;

        case OP_shl:
            // Both operands must be integral.

            //DASSERT(!fIsReal);

            if (fIsSigned) {
                lRes = lL << lR;
            }
            else {
                ulRes = ulL << ulR;
            }
            break;

        case OP_shr:
            // Both operands must be integral.

            //DASSERT(!fIsReal);

            if (fIsSigned) {
                lRes = lL >> lR;
            }
            else {
                ulRes = ulL >> ulR;
            }
            break;

        case OP_and:
            // Both operands must have integral type.

            //DASSERT(!fIsReal);

            if (fIsSigned) {
                lRes = lL & lR;
            }
            else {
                ulRes = ulL & ulR;
            }
            break;

        case OP_or:
            // Both operands must have integral type.

            //DASSERT(!fIsReal);

            if (fIsSigned) {
                lRes = lL | lR;
            }
            else {
                ulRes = ulL | ulR;
            }
            break;

        case OP_xor:
            // Both operands must have integral type.

            //DASSERT(!fIsReal);

            if (fIsSigned) {
                lRes = lL ^ lR;
            }
            else {
                ulRes = ulL ^ ulR;
            }
            break;

        default:
            pExState->err_num = ERR_INTERNAL;
            //DASSERT(FALSE);
            return (FALSE);
    }

    // Now set up the resultant node and coerce back to the correct
    // type:

    EVAL_STATE (STP) = EV_rvalue;

    if (fResInt) {
        if (pExState->state.f32bit) {
            EVAL_LONG (STP) = (long) iRes;
            SetNodeType (STP, T_INT4);
        }
        else {
            EVAL_SHORT (STP) = (short) iRes;
            SetNodeType (STP, T_INT2);
        }
    }
    else {
        if (fIsLDouble) {
            EVAL_LDOUBLE (STP) = ldRes;
            SetNodeType (STP, T_REAL80);
        }
        else if (fIsDouble) {
            EVAL_DOUBLE (STP) = dRes;
            SetNodeType (STP, T_REAL64);
        }
        else if (fIsFloat) {
            EVAL_FLOAT (STP) = fRes;
            SetNodeType (STP, T_REAL32);
        }
        else {
            if (fIsSigned) {
                EVAL_QUAD (STP) = lRes;
                SetNodeType (STP, T_QUAD);
            }
            else {
                EVAL_UQUAD (STP) = ulRes;
                SetNodeType (STP, T_UQUAD);
            }
            CastNode (STP, typRes, typRes);
        }
    }
    return (PopStack ());
}




/***    EvalUtil - Set up nodes for evaluation
 *
 *      fSuccess = EvalUtil (op, pvLeft, pvRight, wFlags)
 *
 *      Entry   pvLeft = pointer to left operand node
 *              pvRight = pointer to right operand node, or NULL
 *              wFlags = EU_...
 *
 *      Exit    pvLeft and pvRight loaded and/or typed as requested
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL EvalUtil (op_t op, peval_t pvLeft, peval_t pvRight, ulong  wFlags)
{
    // Load the value of the node(s).

    if (wFlags & EU_LOAD) {
        switch (EVAL_STATE (pvLeft)) {
            case EV_lvalue:
                if (!LoadVal (pvLeft)) {
                    pExState->err_num = ERR_NOTEVALUATABLE;
                    return (FALSE);
                }
                break;

            case EV_rvalue:
            case EV_constant:
                break;

            case EV_type:
                if (EVAL_IS_STMEMBER (pvLeft)) {
                    if (!LoadVal (pvLeft)) {
                        pExState->err_num = ERR_NOTEVALUATABLE;
                        return (FALSE);
                    }
                    else {
                       break;
                    }
                }

            default:
                pExState->err_num = ERR_SYNTAX;
                return (FALSE);
        }
        if (pvRight != NULL) {
            switch (EVAL_STATE (pvRight)) {
                case EV_lvalue:
                    if (!LoadVal (pvRight)) {
                        pExState->err_num = ERR_NOTEVALUATABLE;
                        return (FALSE);
                    }
                    break;

                case EV_rvalue:
                case EV_constant:
                    break;

                case EV_type:
                    if (EVAL_IS_STMEMBER (pvRight)) {
                        if (!LoadVal (pvRight)) {
                            pExState->err_num = ERR_NOTEVALUATABLE;
                            return (FALSE);
                        }
                        else {
                            break;
                        }
                    }

                default:
                    pExState->err_num = ERR_SYNTAX;
                    return (FALSE);
            }
        }
    }

    // Perform implicit type coercion.

    if (wFlags & EU_TYPE) {
        TypeNodes (op, pvLeft, pvRight);
    }

    return (TRUE);
}

/**     EvalPrototype - evaluate a function prototype as for a breakpoint.
 *
 *      fSuccess = EvalPrototype (pn)
 *
 *      Entry   bn = based pointer to OP_fcn
 *
 *      Exit    ST = result of function call
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */

bool_t EvalPrototype (bnode_t bn)
{
    bnode_t bnRight = NODE_RCHILD (bn);
    eval_t  evalF;
    peval_t pvF             = &evalF;

    // NOTE: evaluation of prototypes/bp addrs DOES NOT MODIFY.
    // is_assign = TRUE ;

    if (!EvalLChild (bn)) {
        return FALSE;
    }

    if (EVAL_IS_PTR (ST)) {
        FetchOp (ST);
    }

    *pvF = *ST;
    PopStack ();

    if (EVAL_IS_METHOD (pvF)) {
        if (EVAL_IS_BPREL (pvThis)) {
            ResolveAddr(pvThis);
        }

        if (EVAL_IS_REGREL(pvThis)) {
            SHREG           reg;

            //
            // The problem with this code is that if we are evaluating a
            // prototype or EEBPADDRESS we may not even be running -- or may
            // not be in context whatsoever.  Thus, any registers we may
            // get will be meaningless.  Lets hope this does not happen.

            DASSERT (FALSE);
            return FALSE;

            EVAL_IS_REGREL(pvThis) = FALSE;

            reg.hReg = EVAL_REGREL (pvThis);

            if (GetReg(&reg, pCxt, NULL) == NULL) {
                DASSERT (FALSE);
                return FALSE;
            }
            EVAL_SYM_OFF (pvThis) += reg.Byte4;
            EVAL_SYM_SEG (pvThis) = GetSegmentRegister(pExState->hframe, REG_SS);
            ADDR_IS_LI (EVAL_SYM (pvF)) = FALSE;
            SHUnFixupAddr ((LPADDR)&EVAL_SYM (pvF));
        }
    }

    // The stack is empty here, we need to push address back on or
    // else we will fault.

    PushStack (pvF);
    return TRUE;
}


/**     EvalFunction - Perform an function call
 *
 *      fSuccess = EvalFunction (pn)
 *
 *      Entry   bn = based pointer to OP_fcn
 *
 *      Exit    ST = result of function call
 *              pExState->err_num = error number if error
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL EvalFunction (bnode_t bn)
{
    bnode_t     bnRight = NODE_RCHILD (bn);
    bnode_t     bnT;
    eval_t      evalRet;
    peval_t     pvRet;
    eval_t      evalF;
    peval_t     pvF;
    bnode_t     bnA;
    peval_t     pvA;
    SHREG       spReg;
    bool_t      retval = FALSE;
    HDEP        hReg = 0;
    UOFFSET     maxSP = 0;
    CV_typ_t    typArg;
    pargd_t     pa;
    ADDR        fcnAddr;
    bool_t      fEvalProto = FALSE;
    HDEP        hFunc;
    ADDR        addr;


    // check if we are dealing with a actual call or a function proto
    // we can check this by just checking the first argument in the
    // bArgList, because Function has already checked that they are
    // either all types or all expressions
    // sps - 2/24/92

    if ((bnRight != NULL) && (NODE_OP ((pnode_t)bnRight) != OP_endofargs)) {
        pnode_t pnT = (pnode_t)bnRight;
        pargd_t pa = (pargd_t)&(pnT->v[0]);
        fEvalProto = (pa->flags.istype);
    }

    //
    // Function prototypes and evaluation for the purpose of setting a BP
    // go through EvalPrototype.
    //

    if (pExState->style == EEBPADDRESS || fEvalProto) {
        return EvalPrototype (bn);
    }

    //
    // EvalFunction can possibly update global variables etc...
    //

    is_assign = TRUE;

    if (!EvalLChild (bn)) {

        // CUDA #3949 : can't goto EvalFunctionExit here because hReg is not
        // initialized yet [rm]
        //
        // goto EvalFunctionExit;

        return FALSE;
    }

    if (EVAL_IS_PTR (ST)) {
        FetchOp (ST);
    }

    // the stack top is the function address node.  We save this information
    // and pop the function node.  We then push the argument left to right
    // using the SP relative offset from the bind that is stored in the address
    // field of the OP_arg node.

    // The evaluation of the left node also processed the this pointer
    // adjustment.  ThisAddress contains the value of the this pointer

    pvF = &evalF;
    *pvF = *ST;
    PopStack ();

    //
    // Do not allow function evaluation on the Mac.
    //

    if (TargetMachine == mptmppc) {
        return FALSE;
    }

    //
    // the left child must resolve to a function address and BP must not
    // be zero and the overlay containing the function must be loaded
    //

    SYGetAddr(pExState->hframe, &addr, adrBase);
    if (GetAddrOff(addr) == 0) {
        pExState->err_num = ERR_FCNCALL;
        return FALSE;
    }

    if (DHSetupExecute(&hFunc)) {
        pExState->err_num = ERR_NOMEMORY;
        return FALSE;
    }

    switch (TargetMachine) {
        case mptmips:
            spReg.hReg = CV_M4_IntSP;
            break;

        case mptdaxp:
            spReg.hReg = CV_ALPHA_IntSP;
            break;

        case mptm68k:
            DASSERT(FALSE);
            break;

        case mptix86:
            spReg.hReg = CV_REG_ESP;
            break;

        case mptia64:
            spReg.hReg = CV_IA64_IntSp;
            break;

        case mptmppc:
        case mptntppc:
            DASSERT (FALSE);
            break;

        default:
            DASSERT (FALSE);
    }

    GetReg (&spReg, pCxt, NULL);

    // Evaluate argument nodes until end of arguments reached and count arguments

    for (bnT = bnRight; NODE_OP (bnT) != OP_endofargs; bnT = NODE_RCHILD (bnT)) {

        if (NODE_OP (bnT) != OP_arg) {
            pExState->err_num = ERR_INTERNAL;
            goto EvalFunctionExit;
        }
        bnA = NODE_LCHILD (bnT);
        pvA = &bnA->v[0];
        pa = (pargd_t)&bnT->v[0];
        typArg = pa->type;

        if (Eval (bnA) == FALSE) {
            goto EvalFunctionExit;
        }
        GetReg (&spReg, pCxt, NULL);

        // Canonicalize reference actual parameters.
        if (EVAL_IS_REF (ST)) {
            if (!FetchOp (ST)) {
                goto EvalFunctionExit;
            }
            EVAL_IS_REF (ST) = FALSE;
        }

        if (EVAL_STATE (ST) == EV_constant &&
            pa->flags.ref == FALSE &&
            IsStringLiteral(ST)) {

            if (PushString (ST, &spReg, typArg, EVAL_SYM_IS32(pvF)) == FALSE) {
                goto EvalFunctionExit;
            }
            SetReg (&spReg, pCxt, NULL);
        }
        else {

            // if the OP_arg node contains a nonzero type (not vararg),
            // cast the stack top to the specified value

            if (pa->flags.ref == TRUE) {
                // for a reference argument, the following happens
                // if the actual is a constant, cast and push onto
                // the stack and pass the address.  If the actual
                // is a variable of the correct type, pass the address.
                // If the actual is a variable of the wrong type, load
                // and attempt to cast to the correct type, push the
                // value onto the stack and pass the address.  If the
                // actual is a complex type, pass the address.  If the
                // formal is a base class of the actual, cast the
                // address and pass that.

                if (EVAL_STATE (ST) == EV_constant) {
                    // push the casted constant onto the stack
                    if (!CastNode (ST, pa->utype, pa->utype)) {
                        goto EvalFunctionExit;
                    }
                    PushRef (ST, &spReg, pa->type, EVAL_SYM_IS32(pvF));
                    SetReg (&spReg, pCxt, NULL);
                }
                else if (EVAL_STATE (ST) == EV_lvalue &&
                    (EVAL_IS_CLASS (ST) || EVAL_TYP (ST) == pa->utype)) {
                    // we either have a class or a simple variable that
                    // has an address (lvalue) and matches the formal
                    // argument type exactly.
                    // the address of the class or a base class must
                    // be pushed This is done by changing the type of
                    // the node to a far constructed cv pointer.  The
                    // following cast will set the node to the correct type.

                    SetNodeType (ST,
                            (CV_typ_t) (EVAL_SYM_IS32(pvF) ? T_32NCVPTR : T_FCVPTR));

                    if (!CastNode (ST, pa->type, pa->type)) {
                        goto EvalFunctionExit;
                    }
                    if (!EvalUtil (OP_function, ST, NULL, EU_LOAD)) {
                        goto EvalFunctionExit;
                    }
                    EVAL_PTR (ST) = EVAL_SYM (ST);
                }
                else {
                    // process a "simple" variable

                    if (!EvalUtil (OP_function, ST, NULL, EU_LOAD)) {
                        goto EvalFunctionExit;
                    }

                    // if the variable is not of the correct type,
                    // load and cast the value to the correct type,
                    // push the value onto the stack and pass that
                    // address to the function

                    if (EVAL_TYP (ST) != pa->type) {
                        if (!CastNode (ST, pa->utype, pa->utype)) {
                            goto EvalFunctionExit;
                        }
                    }
                    PushRef (ST, &spReg, pa->type, EVAL_SYM_IS32(pvF));
                    SetReg (&spReg, pCxt, NULL);
                }
            }
            else {
                // an actual that is not a reference cannot be a class.  Load the value
                // and cast it to the proper type.

                if (EVAL_IS_CLASS (ST)) {
                    pExState->err_num = ERR_CANTCONVERT;
                    goto EvalFunctionExit;
                }
                if (EVAL_IS_ENUM (ST)) {
                    // typArg may be a fwd ref to an enum type
                    // Try to resolve it before proceeding or else
                    // CastNode may fail. [Dolphin #3659]
                    eval_t localEval;
                    peval_t pvT = &localEval;

                    *pvT = *ST;
                    if (SetNodeType (pvT, typArg)) {
                        typArg = EVAL_TYP (pvT);
                    }
                }

                if (!EvalUtil (OP_function, ST, NULL, EU_LOAD) ||
                    !CastNode (ST, typArg, typArg)) {
                    goto EvalFunctionExit;
                }
            }
        }
    }

    // evaluate left hand side of tree to get function address

    // set type of this pointer if method

    pvRet = &evalRet;
    if (EVAL_IS_METHOD (pvF)) {

        if (EVAL_IS_BPREL (pvThis)) {
            ResolveAddr(pvThis);
        }

        if (EVAL_IS_REGREL(pvThis)) {
            SHREG           reg;

            EVAL_IS_REGREL(pvThis) = FALSE;

            reg.hReg = EVAL_REGREL (pvThis);
            if (GetReg(&reg, pCxt, pExState->hframe) == NULL) {
                DASSERT(FALSE);
                retval = FALSE;
                goto EvalFunctionExit;
            }
            EVAL_SYM_OFF (pvThis) += reg.Byte4;
            EVAL_SYM_SEG (pvThis) = GetSegmentRegister(pExState->hframe, REG_SS);
            ADDR_IS_LI (EVAL_SYM (pvF)) = FALSE;
            SHUnFixupAddr ((LPADDR)&EVAL_SYM (pvF));
        }

                //
                // Do not call with a NULL this pointer.

                if (!EVAL_IS_STMETHOD (pvThis) && EVAL_SYM_OFF (pvThis) == 0) {
                        retval = FALSE;
                        goto EvalFunctionExit;
                }
    }

        DASSERT (TargetMachine != mptmppc);

    // set type of return node

    *pvRet = *pvF;
    SetNodeType (pvRet, FCN_RETURN (pvRet));
    EVAL_VALLEN (pvRet) = (ulong )TypeSize (pvRet);
    EVAL_STATE (pvRet) = EV_rvalue;

    // for some return types, certain calling conventions requires a hidden
    // argument pointing to space allocated on the user's stack large
    // enough to hold the return value.

    if (EVAL_SYM_IS32(pvF)) {
        // in the 32 bit world all calling conventions returning a struct/class
        // the callee allocates the tmp and passes the address in a hidden
        // parm
        if (EVAL_IS_CLASS (pvRet)) {
            spReg.Byte4 -= (EVAL_VALLEN (pvRet) + 3) & ~3;

            if (TargetMachine == mptix86) {
                                // if fastcall pass the address of return tmp in ECX
                if (FCN_CALL(pvF) == FCN_FAST) {
                                        SHREG           reg;
                                        reg.hReg = CV_REG_ECX;
                                        reg.Byte4 = spReg.Byte4;
                    SetReg (&reg, pCxt, NULL);
                                 }
                 else {
                                        ADDR_IS_LI (EVAL_SYM (pvRet)) = FALSE;
                                        EVAL_STATE (pvRet) = EV_lvalue;
                                        EVAL_SYM_OFF (pvRet) = spReg.Byte4;
                                        EVAL_ULONG (pvRet) = spReg.Byte4;
                    EVAL_SYM_SEG (pvRet) = GetSegmentRegister(pExState->hframe, REG_SS);
                }
            }
        }
    }
    else {
        switch (FCN_CALL (pvF)) {
            case FCN_C:
                // The C and the C++ compiler use different calling
                // conventions for functions that return structures > 4 bytes
                // Until the compiler provides sufficient information
                // to distinguish between these two, the EE will not
                // support such function calls.

                if (!EVAL_IS_REF (pvRet) &&
                  ((!CV_IS_PRIMITIVE (EVAL_TYP (pvRet)) &&
                  ((EVAL_VALLEN (pvRet) > 4) &&
                  (CV_TYPE (EVAL_TYP (pvRet)) != CV_REAL))))) {
                    pExState->err_num = ERR_CALLSEQ;
                    goto EvalFunctionExit;
                }
                break;

            case FCN_PASCAL:
                // If the return value is larger than 4 bytes, then allocate
                // space on the stack and set the return node
                // to point to this address

                if (!EVAL_IS_REF (pvRet) &&
                  ((!CV_IS_PRIMITIVE (EVAL_TYP (pvRet)) ||
                  (CV_TYPE (EVAL_TYP (pvRet)) == CV_REAL) ||
                  (EVAL_VALLEN (pvRet) > 4)))) {
                    ADDR_IS_LI (EVAL_SYM (pvRet)) = FALSE;
                    EVAL_STATE (pvRet) = EV_lvalue;
                    spReg.Byte4 -= (ushort) ((EVAL_VALLEN (pvRet) + 1) & ~1);
                    EVAL_SYM_OFF (pvRet) = spReg.Byte2;
                    EVAL_USHORT (pvRet) = spReg.Byte2;
                    EVAL_SYM_SEG (pvRet) = GetSegmentRegister(pExState->hframe, REG_SS);;
                }
                break;



            case FCN_FAST:
                // If the return value is not real and is larger than 4 bytes,
                // then allocate space on the stack and set the return node
                // to point to this address.  For fastcall, real values are
                // returned on the numeric coprocessor stack

                if (!EVAL_IS_REF (pvRet) &&
                  ((!CV_IS_PRIMITIVE (EVAL_TYP (pvRet)) &&
                  ((EVAL_VALLEN (pvRet) > 4) &&
                  (CV_TYPE (EVAL_TYP (pvRet)) != CV_REAL))))) {
                    ADDR_IS_LI (EVAL_SYM (pvRet)) = FALSE;
                    EVAL_STATE (pvRet) = EV_lvalue;
                    spReg.Byte4 -= (ushort) ((EVAL_VALLEN (pvRet) + 1) & ~1);
                    EVAL_SYM_OFF (pvRet) = spReg.Byte2;
                    EVAL_USHORT (pvRet) = spReg.Byte2;
                    EVAL_SYM_SEG (pvRet) = GetSegmentRegister(pExState->hframe, REG_SS);;
                }
                break;
            default:
                DASSERT (FALSE);
                pExState->err_num = ERR_INTERNAL;
                goto EvalFunctionExit;
        }
    }

    // push arguments for the function

    if (PushArgs ((pnode_t)bnRight, &spReg, &maxSP) == FALSE) {
        goto EvalFunctionExit;
    }

    if (EVAL_STATE (pvRet) == EV_lvalue) {
        if (!PushOffset (EVAL_SYM_OFF (pvRet), &spReg, &maxSP,
          (EVAL_SYM_IS32 (pvF) ? sizeof (CV_uoff32_t) : sizeof (CV_uoff16_t)))) {
            goto EvalFunctionExit;
        }
    }

    if (EVAL_IS_METHOD (pvF)) {
        SetNodeType (pvThis, FCN_THIS (pvF));

        if (ADDR_IS_LI (EVAL_SYM(pvThis))) {
        SHFixupAddr (&EVAL_SYM(pvThis));
        }

        DASSERT (!EVAL_IS_FPTR32(pvThis));      //unsupported
        if (EVAL_IS_NPTR32 (pvThis)) {
            SHREG       argReg;

            DASSERT (EVAL_SYM_IS32(pvF));

            if (FCN_CALL(pvF) == FCN_MIPS) {
                argReg.hReg = CV_M4_IntA0;
// BUGBUG must support 64 bit offsets
                argReg.Byte4 = (UOFF32)EVAL_SYM_OFF(pvThis);
                SetReg(&argReg, pCxt, NULL);
            } else
            if (FCN_CALL(pvF) == FCN_ALPHA) {
                argReg.hReg = CV_ALPHA_IntA0;
// BUGBUG must support 64 bit offsets
                argReg.Byte4 = (UOFF32)EVAL_SYM_OFF(pvThis);
                SetReg(&argReg, pCxt, NULL);
            } else
            if (FCN_CALL(pvF) == FCN_PPC) {
                argReg.hReg = CV_PPC_GPR3;
// BUGBUG must support 64 bit offsets
                argReg.Byte4 = (UOFF32)EVAL_SYM_OFF(pvThis);
                SetReg(&argReg, pCxt, NULL);
            } else
             if (FCN_CALL(pvF) == FCN_IA64) {
 				argReg.hReg = CV_IA64_IntR32;
// BUGBUG must support 64 bit offsets
                argReg.Byte4 = (UOFF32)EVAL_SYM_OFF(pvThis);
 			// v-vadimp - theoretically EM should take care of updating the stacked registers correctly
 			// and no extra functionality is needed here
                SetReg(&argReg, pCxt, NULL);
             } else
            // for _thiscall pass the this pointer in ECX
            if (FCN_CALL(pvF) == FCN_THISCALL) {
                argReg.hReg = CV_REG_ECX;
// BUGBUG must support 64 bit offsets
                argReg.Byte4 = (UOFF32)EVAL_SYM_OFF (pvThis);
                SetReg (&argReg, pCxt, NULL);
            }
            else if (!PushOffset (EVAL_SYM_OFF (pvThis),
                 &spReg, &maxSP, sizeof (CV_uoff32_t))) {
                goto EvalFunctionExit;
            }
        }
        else {
            if (!EVAL_IS_NPTR (pvThis)) {
                if (!PushOffset (EVAL_SYM_SEG (pvThis), &spReg,
                  &maxSP, sizeof (ushort))) {
                    goto EvalFunctionExit;
                }
            }
            if (!PushOffset (EVAL_SYM_OFF (pvThis), &spReg, &maxSP,
              sizeof (CV_uoff16_t))) {
                goto EvalFunctionExit;
            }
        }
    }

    // Call the user's procedure

    spReg.Byte4 -= (ushort) maxSP;
    SetReg (&spReg, pCxt, NULL);

    if (!getFuncAddrNoThunks(pvF)) {
        goto EvalFunctionExit;
    }

    fcnAddr = EVAL_SYM(pvF);

    // make sure that the execution model is Native, if it is not
    // then return ERR_CALLSEQ.
    {
        WORD wModel;
        SYMPTR pSym;
        UOFFSET obMax = 0xFFFFFFFF;

        if (SHModelFromAddr(&fcnAddr, &wModel, (uchar *)&pSym, &obMax )) {
            if ( wModel != CEXM_MDL_native ) {

                // not native calling sequence, return error
                pExState->err_num = ERR_CALLSEQ;
                goto EvalFunctionExit;
            }
        }
    }

    if (ADDR_IS_LI (fcnAddr)) {
        SHFixupAddr (&fcnAddr);
    }

    if (DHStartExecute (hFunc,
                         &fcnAddr,
                         TRUE,
                         EVAL_IS_FFCN (pvF) ? SHFar : SHNear
                         )
    ) {
        pExState->err_num = ERR_FCNCALLERR;
        goto EvalFunctionExit;
    }

    // Procedure succeeded, set return information and set flag to force
    // update

    is_assign = TRUE;
    PushStack (pvRet);
    if (EVAL_TYP (pvRet) != T_VOID) {
        if (EVAL_SYM_IS32 (pvF)) {
            switch (FCN_CALL(pvF)) {
                case FCN_C:
                case FCN_THISCALL:
                case FCN_STD:
                case FCN_FAST:
                    retval = Store32 (pvF);
                    break;

                case FCN_IA64:
                    retval = StoreIa64(pvF);
                    break;

                case FCN_MIPS:
                    retval = StoreMips(pvF);
                    break;

                case FCN_ALPHA:
                    retval = StoreAlpha(pvF);
                    break;

                case FCN_PPC:
                    retval = StorePPC(pvF);
                    break;

                default:
                    goto EvalFunctionExit;
            }
            // CUDA #4020 : mark pointers as 32 bit or else badness...
            if (retval && EVAL_IS_PTR(ST)) {
                ADDRLIN32(EVAL_PTR(ST));
            }
        }
        else {
            switch (FCN_CALL (pvF)) {
                case FCN_C:
                    retval = StoreC (pvF);
                    break;

                case FCN_PASCAL:
                    retval = StoreP ();
                    break;


                case FCN_FAST:
                    retval = StoreF ();
                    break;

                default:
                    goto EvalFunctionExit;
            }
        }
    }
    else {
        retval = TRUE;
    }

    if (retval == TRUE) {
        // According to C++ "a function call is an lvalue only
        // if the result type is a reference". In that case
        // the result node is converted to the lvalue of the
        // referenced object. By doing so, it is possible to
        // evaluate child expressions (such as base classes),
        // that need to get the address of the referenced object .
        if (EVAL_IS_REF (ST)) {
            EVAL_SYM (ST) = EVAL_PTR (ST);
            RemoveIndir (ST);
            EVAL_STATE (ST) = EV_lvalue;
        }
        else {
            EVAL_STATE (ST) = EV_rvalue;
        }
    }

EvalFunctionExit:
    if (DHCleanUpExecute(hFunc)) {
        retval = FALSE;
    }
    return retval;
}


/**     PushArgs - push arguments
 *
 *      fSuccess = PushArgs (pnArg, pspReg, pmaxSP);
 *
 *      Entry   pnArg = pointer to argument nodes
 *              pspReg = pointer to register structure for SP value
 *              pmaxSP = pointer to location to store maximum SP relative offset
 *
 *      Exit    arguments pushed onto stack
 *
 *      Returns TRUE if parameters pushed without error
 *              FALSE if error during push
 */


bool_t
PushArgs (
    pnode_t pnArg,
    SHREG *pspReg,
    UOFFSET *pmaxSP
    )
{
    SHREG       argReg;

    // The arguments have been evaluated left to right which means that the
    // rightmost argument is at ST.  We need to recurse down the right side
    // of the argument tree to find the OP_arg node that corresponds to the
    // stack top.

    if (NODE_OP (pnArg) == OP_endofargs) {
        return (TRUE);
    }
    if (!PushArgs ((pnode_t)NODE_RCHILD (pnArg), pspReg, pmaxSP)) {
        return (FALSE);
    }
    else {
        if (((pargd_t)&(pnArg->v[0]))->flags.isreg == TRUE) {
            argReg.hReg = ((pargd_t)&(pnArg->v[0]))->reg;
            switch (TargetMachine) {
                case mptmips:
                    switch (argReg.hReg) {
                        case CV_M4_IntA0:
                        case CV_M4_IntA1:
                        case CV_M4_IntA2:
                        case CV_M4_IntA3:
                            argReg.Byte4 = EVAL_ULONG( ST );
                            *pmaxSP = max( *pmaxSP, ((pargd_t)&pnArg->v[0])->SPoff );
                            break;

                        case CV_M4_FltF12:
                        case CV_M4_FltF14:
                            memcpy(&argReg.Byte4, &EVAL_FLOAT( ST ), sizeof(float));
                            *pmaxSP = max( *pmaxSP, ((pargd_t)&pnArg->v[0])->SPoff );
                            break;

                        case (CV_M4_IntA1<<8)|CV_M4_IntA0:
                        case (CV_M4_IntA3<<8)|CV_M4_IntA2:
                        case (CV_M4_FltF13<<8)|CV_M4_FltF12:
                        case (CV_M4_FltF15<<8)|CV_M4_FltF14:
                            memcpy(&argReg.Byte1, &EVAL_DOUBLE( ST ), sizeof(double));
                            *pmaxSP = __max( *pmaxSP, ((pargd_t)&pnArg->v[0])->SPoff );
                            break;

                        default:
                            DASSERT(FALSE);
                            break;
                    }
                    break;

                case mptdaxp:
                    switch (argReg.hReg) {
                        case CV_ALPHA_IntA0:
                        case CV_ALPHA_IntA1:
                        case CV_ALPHA_IntA2:
                        case CV_ALPHA_IntA3:
                        case CV_ALPHA_IntA4:
                        case CV_ALPHA_IntA5:
                            *((QUAD *) &argReg.Byte4) = EVAL_QUAD( ST );
                            *pmaxSP = __max( *pmaxSP, ((pargd_t)&pnArg->v[0])->SPoff );
                            break;

                        case CV_ALPHA_FltF16:
                        case CV_ALPHA_FltF17:
                        case CV_ALPHA_FltF18:
                        case CV_ALPHA_FltF19:
                        case CV_ALPHA_FltF20:
                        case CV_ALPHA_FltF21:
                            memcpy(&argReg.Byte1, &EVAL_DOUBLE ( ST ), sizeof(double));
                            *pmaxSP = __max( *pmaxSP, ((pargd_t)&pnArg->v[0])->SPoff );
                            break;

                        default:
                            DASSERT(FALSE);
                            break;
                    }
                    break;

                case mptix86:
                    switch (argReg.hReg & 0xff) {
                        case CV_REG_AL:
                        case CV_REG_CL:
                        case CV_REG_DL:
                        case CV_REG_BL:
                        case CV_REG_AH:
                        case CV_REG_CH:
                        case CV_REG_DH:
                            argReg.Byte1 = EVAL_UCHAR (ST);
                            break;

                        case CV_REG_EAX:
                        case CV_REG_ECX:
                        case CV_REG_EDX:
                        case CV_REG_EBX:
                        case CV_REG_ESP:
                        case CV_REG_EBP:
                        case CV_REG_ESI:
                        case CV_REG_EDI:
                        case CV_REG_CR0:
                        case CV_REG_CR1:
                        case CV_REG_CR2:
                        case CV_REG_CR3:
                        case CV_REG_CR4:
                        case CV_REG_DR0:
                        case CV_REG_DR1:
                        case CV_REG_DR2:
                        case CV_REG_DR3:
                        case CV_REG_DR4:
                        case CV_REG_DR5:
                        case CV_REG_DR6:
                        case CV_REG_DR7:
                            argReg.Byte4 = EVAL_ULONG (ST);
                            break;

                        case CV_REG_ST0:
                        case CV_REG_ST1:
                        case CV_REG_ST2:
                        case CV_REG_ST3:
                        case CV_REG_ST4:
                        case CV_REG_ST5:
                        case CV_REG_ST6:
                        case CV_REG_ST7:
                            argReg.Byte10 = EVAL_LDOUBLE (ST);
                            break;

                        default:
                            argReg.Byte2 = EVAL_USHORT (ST);
                            break;
                    }
                    if ((argReg.hReg >> 8) != CV_REG_NONE) {
                        switch (argReg.hReg >> 8) {
                            case CV_REG_DX:
                            case CV_REG_ES:
                                argReg.Byte2High = *(((ushort *)&(EVAL_ULONG (ST))) + 1);
                                break;
                        }
                    }
                    break;

                case mptia64:
                    switch (argReg.hReg) {
                        case CV_IA64_IntR32:
                        case CV_IA64_IntR33:
                        case CV_IA64_IntR34:
                        case CV_IA64_IntR35:
                        case CV_IA64_IntR36:
                        case CV_IA64_IntR37:
                        case CV_IA64_IntR38:
                        case CV_IA64_IntR39:
                            *((QUAD *) &argReg.Byte4) = EVAL_QUAD( ST );
                            *pmaxSP = __max( *pmaxSP, ((pargd_t)&pnArg->v[0])->SPoff );
                            break;

                        case CV_IA64_FltT2:
                        case CV_IA64_FltT3:
                        case CV_IA64_FltT4:
                        case CV_IA64_FltT5:
                        case CV_IA64_FltT6:
                        case CV_IA64_FltT7:
                        case CV_IA64_FltT8:
                        case CV_IA64_FltT9:
                            memcpy(&argReg.Byte1, &EVAL_DOUBLE ( ST ), sizeof(double));
                            *pmaxSP = __max( *pmaxSP, ((pargd_t)&pnArg->v[0])->SPoff );
                            break;

                        default:
                            DASSERT(FALSE);
                            break;
                    }
                    break;

                case mptm68k:
                case mptmppc:
                case mptntppc:
                default:
                    DASSERT(FALSE);
            }
            SetReg (&argReg, pCxt, NULL);
        }
        else if (!PushUserValue (ST, (pargd_t)&pnArg->v[0], pspReg, pmaxSP)) {
            pExState->err_num = ERR_PTRACE;
            return (FALSE);
        }
    }
    PopStack ();
    return (TRUE);
}






/**     PushRef - push referenced value onto stack
 *
 *      fSuccess = PushRef (pv, spReg, reftype, Is32);
 *
 *      Entry   pv = value
 *              spReg = pointer to sp register structure
 *              reftype = type of the reference
 *
 *      Exit    string pushed onto user stack
 *              SP register structure updated to reflect size of string
 *
 *      Returns TRUE if return value stored without error
 *              FALSE if error during store
 */


bool_t PushRef (peval_t pv, SHREG *spReg, CV_typ_t reftype, BOOL Is32)
{
    uint        cbVal;
    bool_t      retval = TRUE;
    uint        fudgePad = Is32 ? 3 : 1;
    CV_typ_t    lvalueCastType = Is32 ? T_32NCVPTR : T_FCVPTR;

    Unreferenced( reftype );

    switch (EVAL_STATE (pv)) {
        case EV_lvalue:
            // for an lvalue, change the node to a reference to the lvalue
            EVAL_PTR (pv) = EVAL_SYM (pv);
            CastNode (pv, lvalueCastType, lvalueCastType);
            break;

        case EV_rvalue:
        case EV_constant:
            cbVal = ((uint)TypeSize (pv) + fudgePad ) & ~fudgePad;

            // decrement stack pointer to allocate room for string

            spReg->Byte4 -= (ushort) cbVal;

            // get current SS value and set symbol address to SS:SP

            EVAL_SYM_SEG (pv) = GetSegmentRegister(pExState->hframe, REG_SS); //M00FLAT32
            EVAL_SYM_OFF (pv) = spReg->Byte4;
            EVAL_STATE (pv) = EV_lvalue;
            ADDR_IS_LI (EVAL_SYM (pv)) = FALSE;
            retval = (PutDebuggeeBytes (EVAL_SYM (pv), cbVal, EVAL_PVAL (pv), EVAL_TYP(pv)) == cbVal);
            EVAL_PTR (pv) = EVAL_SYM (pv);
            if (retval == FALSE) {
                pExState->err_num = ERR_PTRACE;
            }
            break;

        default:
            DASSERT (FALSE);
            retval = FALSE;
            break;
    }
    return (retval);
}




/**     IsStringLiteral - check if an evaluation node represents a string literal
 *
 *      flag = IsStringLiteral  (pv);
 *
 *      Entry   pv = pointer to evaluation node
 *
 *      Exit    none
 *
 *      Returns TRUE if pv represents a string literal ("..." or L"...")
 *              FALSE otherwise
 */

bool_t FASTCALL IsStringLiteral (peval_t pv)
{
    char   *pb;

    CV_typ_t typ = EVAL_TYP (pv);
    if (CV_MODE(typ) != CV_TM_DIRECT &&
       ((typ = CV_NEWMODE(typ, CV_TM_DIRECT)) == T_RCHAR ||
         typ == T_WCHAR)) {

        pb = pExStr + EVAL_ITOK (pv);
        if (*pb == 'L') {
            pb++;
        }

        if (*pb == '"') {
            return TRUE;
        }
    }
    return FALSE;
}




/**     PushString - push constant string onto stack
 *
 *      fSuccess = PushString (pv, spReg, typArg, Is32);
 *
 *      Entry   pv = value describing string
 *              spReg = pointer to sp register structure
 *              typArg = type of resultant pointer node
 *              is32 = is function 32 bit?
 *
 *      Exit    string pushed onto user stack
 *              SP register structure updated to reflect size of string
 *
 *      Returns TRUE if return value stored without error
 *              FALSE if error during store
 */


bool_t PushString (peval_t pv, SHREG *spReg, CV_typ_t typArg, BOOL Is32)
{
    char   *pb;
    char   *pbEnd;
    uint        cbVal;
    bool_t      errcnt = 0;
    bool_t      fWide;
    ulong       zero = 0;
    ulong       FillCnt;
    ushort      ch;
    OFF32       spSave;
    uint fudgePad = Is32 ? 3 : 1;
    static      int weirdMap[4] = {4, 3, 2, 5};

    // compute location and length of string note that pointer points to
    // initial " or L" of string and the length includes the initial ' or
    // L" and the ending ". The byte count must be rounded to even parity
    // because of restrictions on the stack.  If the string is a wide
    // character string, then the C runtime must be called to translate the
    // string.

    pb = pExStr + EVAL_ITOK (pv);
    pbEnd = pb + EVAL_CBTOK (pv) - 1;
    if ((fWide = (*pb == 'L')) == TRUE) {
        // skip wide character leader and compute number of bytes for stack
        // we add two bytes for forcing a zero termination
        pb++;
        cbVal = 2 * (EVAL_CBTOK (pv) - 3);
        FillCnt = Is32 ? weirdMap[cbVal & ~3] : 2;
    }
    else {
        cbVal = EVAL_CBTOK (pv) - 2;
        FillCnt = 1 + (~cbVal & fudgePad);
    }

    DASSERT (((cbVal + FillCnt) & fudgePad) == 0);
    DASSERT (*pb == '"');
    pb++;

    // decrement stack pointer to allocate room for string.  We know that
    // the string actually pushed can be no longer than cbVal + FillCnt.
    // It can be shorter because of escaped characters such as \n and \001

    spReg->Byte4 -= (ushort) (cbVal + FillCnt);
    spSave = spReg->Byte4;
    SetNodeType (pv, typArg);

    // get current SS value and set symbol address to SS:SP

    EVAL_SYM_SEG (pv) = GetSegmentRegister(pExState->hframe, REG_SS);;
    EVAL_SYM_OFF (pv) = spReg->Byte4;
    EVAL_SYM_IS32 (pv) = (BYTE) Is32;
    ADDR_IS_LI (EVAL_SYM (pv)) = FALSE;
    EVAL_STATE (pv) = EV_lvalue;
    if (fWide == FALSE) {
        // move characters one at a time onto the user's stack, converting
        // while moving.

        while (pb < pbEnd) {
            if ((ch = *pb++) == '\\') {
                ulong  cbChar;
                GetEscapedChar (&pb, &cbChar, &ch);
            }
            if (PutDebuggeeBytes (EVAL_SYM (pv), sizeof (char), &ch, T_RCHAR) != sizeof (char)) {
                errcnt++;
            }
            EVAL_SYM_OFF (pv) += sizeof (char);
        }
    }
    else {
        // M00BUG - this is a fake routine until the runtime routines are
        // M00BUG - available to do the correct translation.  We will also
        // M00BUG - need to get the locale state from the user's space for
        // M00BUG - for the translation

        while (pb < pbEnd) {
            ch = *pb++;
            // move wide characters onto the user's stack
            if (PutDebuggeeBytes (EVAL_SYM (pv), 2, &ch, T_WCHAR) != 2) {
                errcnt++;
            }
            EVAL_SYM_OFF (pv) += 2 * sizeof (char);
        }
    }
    if (PutDebuggeeBytes (EVAL_SYM (pv), FillCnt, &zero, (CV_typ_t)(fWide ? T_WCHAR : T_RCHAR)) != FillCnt) {
        errcnt++;
    }
    EVAL_SYM_OFF (pv) = spSave;
    EVAL_PTR (pv) = EVAL_SYM (pv);
    if (errcnt != 0) {
        pExState->err_num = ERR_PTRACE;
        return (FALSE);
    }
    return (TRUE);
}






/**     StoreC - store return value for  16 bit C call
 *
 *      fSuccess = StoreC (pvF);
 *
 *      Entry   pvF = pointer to function descriptor
 *
 *      Exit    return value stored in ST
 *
 *      Returns TRUE if return value stored without error
 *              FALSE if error during store
 */


bool_t StoreC (peval_t pvF)
{
    SHREG   argReg;
    ulong   len;

    Unreferenced( pvF );

    if (EVAL_IS_PTR (ST)) {
        argReg.hReg = ((CV_REG_DX << 8) | CV_REG_AX);
        GetReg (&argReg, pCxt, NULL);
        EVAL_PTR_OFF (ST) = argReg.Byte2;
        EVAL_USHORT (ST) = argReg.Byte2;
        if (EVAL_VALLEN (ST) > 2) {
            EVAL_PTR_SEG (ST) = argReg.Byte2High;
        }
        if (EVAL_IS_NPTR  (ST) || EVAL_IS_NPTR32 (ST)) {
            // for near pointer, pointer DS:AX
            EVAL_PTR_SEG (ST) = GetSegmentRegister(pExState->hframe, REG_DS); //M00FLAT32
        }
    }
    else if (EVAL_TYP (ST) == T_REAL80) {
        // return value is long double, read value from the coprocessor
        // stack and cast to proper size

        argReg.hReg = CV_REG_ST0;
        GetReg (&argReg, pCxt, NULL);
        EVAL_LDOUBLE (ST) = argReg.Byte10;
        if (EVAL_TYP (ST) == T_REAL64 ) {
            EVAL_DOUBLE (ST) =  DoubleFromFloat10 (EVAL_LDOUBLE (ST));
        }
        else if (EVAL_TYP (ST) == T_REAL32) {
            EVAL_FLOAT (ST) = FloatFromFloat10 (EVAL_LDOUBLE (ST));
        }
    }
    else if (CV_TYPE (EVAL_TYP (ST)) == CV_REAL) {
        // read real return value from the value pointed to by either
        // DS:AX (near) or DX:AX (far)
        argReg.hReg = CV_REG_AX;
        GetReg (&argReg, pCxt, NULL);
        EVAL_SYM_OFF (ST) = argReg.Byte2;
        argReg.hReg = CV_REG_DS;
        GetReg (&argReg, pCxt, NULL);
        EVAL_SYM_SEG (ST) = argReg.Byte2;
        ADDR_IS_LI (EVAL_SYM (ST)) = FALSE;
        if (GetDebuggeeBytes (EVAL_SYM (ST), EVAL_VALLEN (ST),
          (char *)&EVAL_DOUBLE (ST), EVAL_TYP(ST)) != EVAL_VALLEN (ST)) {
            pExState->err_num = ERR_PTRACE;
            return (FALSE);
        }
        if (EVAL_TYP (ST) == T_REAL32) {
            CastNode (ST, T_REAL64, T_REAL64);
        }
    }
    else if ((EVAL_TYP (ST) == T_CHAR) || (EVAL_TYP (ST) == T_RCHAR)) {
        argReg.hReg = CV_REG_AL;
        GetReg (&argReg, pCxt, NULL);
        EVAL_SHORT (ST) = argReg.Byte1;
    }
    else if (EVAL_TYP (ST) == T_UCHAR) {
        argReg.hReg = CV_REG_AL;
        GetReg (&argReg, pCxt, NULL);
        EVAL_USHORT (ST) = argReg.Byte1;
    }
    else {
        len = EVAL_VALLEN (ST);
        argReg.hReg = ((CV_REG_DX << 8) | CV_REG_AX);
        GetReg (&argReg, pCxt, NULL);
        if (CV_IS_PRIMITIVE (EVAL_TYP (ST)) ||
          (EVAL_IS_CLASS (ST) && (len < 4) && (len != 3))) {
            EVAL_USHORT (ST) = argReg.Byte2;
            if (len > 2) {
                *(((ushort *)&(EVAL_ULONG (ST))) + 1) = argReg.Byte2High;
            }
        }
        else {
            // treat (DS)DX:AX as the pointer to the return value
            EVAL_SYM_OFF (ST) = argReg.Byte2;
            argReg.hReg = CV_REG_DS;
            GetReg (&argReg, pCxt, NULL);
            EVAL_SYM_SEG (ST) = argReg.Byte2;
            ADDR_IS_LI (EVAL_SYM (ST)) = FALSE;
            if (GetDebuggeeBytes (EVAL_SYM (ST), EVAL_VALLEN (ST),
              (char *)&EVAL_VAL (ST), EVAL_TYP(ST)) != EVAL_VALLEN (ST)) {
                pExState->err_num = ERR_PTRACE;
                return (FALSE);
            }
        }
    }
    return (TRUE);
}



/**     StoreF - store return value for 16 bit fast call
 *
 *      fSuccess = StoreF ();
 *
 *      Entry   ST pointer to eval describing return value
 *
 *      Exit    return value stored
 *
 *      Returns TRUE if return value stored without error
 *              FALSE if error during store
 */


bool_t StoreF (void)
{
    SHREG   argReg;

    if (EVAL_IS_PTR (ST)) {
        argReg.hReg = ((CV_REG_DX << 8) | CV_REG_AX);
        GetReg (&argReg, pCxt, NULL);
        EVAL_PTR_OFF (ST) = argReg.Byte2;
        if (EVAL_VALLEN (ST) > 2) {
            EVAL_PTR_SEG (ST) = argReg.Byte2High;
        }
        if (EVAL_IS_NPTR  (ST) || EVAL_IS_NPTR32 (ST)) {
            // for near pointer, pointer DS:AX
            EVAL_PTR_SEG (ST) = GetSegmentRegister(pExState->hframe, REG_DS); //M00FLAT32
        }
    }
    else if (CV_IS_PRIMITIVE (EVAL_TYP(ST)) && (CV_TYPE (EVAL_TYP (ST)) == CV_REAL)) {
        // return value is real, read value from the coprocessor
        // stack and cast to proper size

        argReg.hReg = CV_REG_ST0;
        GetReg (&argReg, pCxt, NULL);
        EVAL_LDOUBLE (ST) = argReg.Byte10;
        if (EVAL_TYP (ST) == T_REAL64) {
            EVAL_DOUBLE (ST) = DoubleFromFloat10 (EVAL_LDOUBLE (ST));
        }
        else if (EVAL_TYP (ST) == T_REAL32) {
            EVAL_FLOAT (ST) = FloatFromFloat10 (EVAL_LDOUBLE (ST));
        }
    }
    else if ((EVAL_TYP (ST) == T_CHAR) || (EVAL_TYP (ST) == T_RCHAR)) {
        argReg.hReg = CV_REG_AL;
        GetReg (&argReg, pCxt, NULL);
        EVAL_SHORT (ST) = argReg.Byte1;
    }
    else if (EVAL_TYP (ST) == T_UCHAR) {
        argReg.hReg = CV_REG_AL;
        GetReg (&argReg, pCxt, NULL);
        EVAL_USHORT (ST) = argReg.Byte1;
    }
    else {
        argReg.hReg = ((CV_REG_DX << 8) | CV_REG_AX);
        GetReg (&argReg, pCxt, NULL);
        EVAL_USHORT (ST) = argReg.Byte2;
        if (EVAL_VALLEN (ST) > 2) {
            *(((ushort *)&(EVAL_ULONG (ST))) + 1) = argReg.Byte2High;
        }
    }
    return (TRUE);
}





/**     StoreP - store return value for pascal call
 *
 *      fSuccess = StoreP ();
 *
 *      Entry   ST = pointer to node describing return value
 *
 *      Exit    return value stored
 *
 *      Returns TRUE if return value stored without error
 *              FALSE if error during store
 */


bool_t StoreP ()
{
    SHREG   argReg;

    if (EVAL_IS_PTR (ST)) {
        argReg.hReg = ((CV_REG_DX << 8) | CV_REG_AX);
        GetReg (&argReg, pCxt, NULL);
        EVAL_PTR_OFF (ST) = argReg.Byte2;
        if (EVAL_VALLEN (ST) > 2) {
            EVAL_PTR_SEG (ST) = argReg.Byte2High;
        }
        if (EVAL_IS_NPTR  (ST) || EVAL_IS_NPTR32 (ST)) {
            // for near pointer, pointer DS:AX
            EVAL_PTR_SEG (ST) = GetSegmentRegister(pExState->hframe, REG_DS); //M00FLAT32
        }
    }
    else if (CV_TYPE (EVAL_TYP (ST)) == CV_REAL) {
        // read real return value from the value pointed to by either
        // DS:AX (near) or DX:AX (far)
        if (EVAL_IS_NFCN (ST)) {
            // for near return, pointer to return variable is DS:AX
            argReg.hReg = (CV_REG_DS << 8) | CV_REG_AX;
        }
        else {
            // for far return, pointer to return variable is DX:AX
            argReg.hReg = ((CV_REG_DX << 8) | CV_REG_AX);
        }
        GetReg (&argReg, pCxt, NULL);  // M00FLAT32
        EVAL_SYM_SEG (ST) = argReg.Byte2High;
        EVAL_SYM_OFF (ST) = argReg.Byte2;
        ADDR_IS_LI (EVAL_SYM (ST)) = FALSE;
        if (GetDebuggeeBytes (EVAL_SYM (ST), EVAL_VALLEN (ST),
          (char *)&EVAL_DOUBLE (ST), EVAL_TYP(ST)) != EVAL_VALLEN (ST)) {
            pExState->err_num = ERR_PTRACE;
            return (FALSE);
        }
        if (EVAL_TYP (ST) == T_REAL32) {
            CastNode (ST, T_REAL64, T_REAL64);
        }
    }
    else if ((EVAL_TYP (ST) == T_CHAR) || (EVAL_TYP (ST) == T_RCHAR)) {
        argReg.hReg = CV_REG_AL;
        GetReg (&argReg, pCxt, NULL);
        EVAL_SHORT (ST) = argReg.Byte1;
    }
    else if (EVAL_TYP (ST) == T_UCHAR) {
        argReg.hReg = CV_REG_AL;
        GetReg (&argReg, pCxt, NULL);
        EVAL_USHORT (ST) = argReg.Byte1;
    }
    else {
        argReg.hReg = ((CV_REG_DX << 8) | CV_REG_AX);
        GetReg (&argReg, pCxt, NULL);
        EVAL_USHORT (ST) = argReg.Byte2;
        if (EVAL_VALLEN (ST) > 2) {
            *(((ushort *)&(EVAL_ULONG (ST))) + 1) = argReg.Byte2High;
        }
        if (EVAL_IS_PTR (ST) && (EVAL_IS_NPTR  (ST) || EVAL_IS_NPTR32(ST))) {
            // for near pointer, pointer DS:AX
            EVAL_PTR_SEG (ST) = GetSegmentRegister(pExState->hframe, REG_DS); //M00FLAT32
        }
    }
    return (TRUE);
}


/**     Store32 - store return value for 32 bit  call
 *
 *      fSuccess = Store32 (pvF);
 *
 *      Entry   pvF = pointer to function descriptor
 *
 *      Exit    return value stored in ST
 *
 *      Returns TRUE if return value stored without error
 *              FALSE if error during store
 */

bool_t Store32 (peval_t pvF)
{
    SHREG   argReg;

    Unreferenced( pvF );

    if (CV_IS_PRIMITIVE (EVAL_TYP (ST)) &&
        CV_TYP_IS_REAL (EVAL_TYP (ST))) {
        // return value is real, read value from the coprocessor
        // stack and cast to proper size

        argReg.hReg = CV_REG_ST0;
        GetReg (&argReg, pCxt, NULL);
        switch (EVAL_TYP (ST)) {

            case T_REAL32:
                EVAL_FLOAT (ST) = FloatFromFloat10 (argReg.Byte10);
                return (TRUE);

            case T_REAL48:
            case T_REAL64:
                EVAL_DOUBLE (ST) = DoubleFromFloat10 (argReg.Byte10);
                return (TRUE);

            case T_REAL80:
                EVAL_LDOUBLE (ST) = argReg.Byte10;
                return (TRUE);

            default:
                pExState->err_num = ERR_INTERNAL;
                return (FALSE);

        }
    }

    if (EVAL_IS_CLASS (ST)) {
        // treat EAX as the pointer to the return value
        argReg.hReg = CV_REG_EAX;
        GetReg (&argReg, pCxt, NULL);
        EVAL_SYM_OFF (ST) = argReg.Byte4;
        EVAL_SYM_SEG (ST) = GetSegmentRegister(pExState->hframe, REG_DS);
        ADDR_IS_LI (EVAL_SYM (ST)) = FALSE;
        if (GetDebuggeeBytes (EVAL_SYM (ST), EVAL_VALLEN (ST),
          (char *)&EVAL_VAL (ST), EVAL_TYP(ST)) != EVAL_VALLEN (ST)) {
            pExState->err_num = ERR_PTRACE;
            return (FALSE);
        }
        return (TRUE);
    }

    if (EVAL_IS_PTR (ST)) {
        argReg.hReg = CV_REG_EAX;
        GetReg (&argReg, pCxt, NULL);

        EVAL_PTR_OFF (ST) = argReg.Byte4;
        EVAL_PTR_SEG (ST) = GetSegmentRegister(pExState->hframe, REG_DS);
        ADDR_IS_LI (EVAL_PTR (ST)) = FALSE;

        return (TRUE);
    }

    switch (EVAL_VALLEN (ST)) {
        case 1:
            argReg.hReg = CV_REG_AL;
            GetReg (&argReg, pCxt, NULL);
            EVAL_CHAR (ST) = argReg.Byte1;
            break;

        case 2:
            argReg.hReg = CV_REG_AX;
            GetReg (&argReg, pCxt, NULL);
            EVAL_SHORT (ST) = argReg.Byte2;
            break;

        case 4:
            argReg.hReg = CV_REG_EAX;
            GetReg (&argReg, pCxt, NULL);
            EVAL_LONG (ST) = argReg.Byte4;
            break;

        case 8:
            argReg.hReg = CV_REG_EDX;
            GetReg (&argReg, pCxt, NULL);
            EVAL_UQUAD (ST) = (UQUAD) argReg.Byte4 << 32;
            argReg.hReg = CV_REG_EAX;
            GetReg (&argReg, pCxt, NULL);
            EVAL_UQUAD (ST) |= argReg.Byte4;
            break;

        default:
            pExState->err_num = ERR_INTERNAL;
            return (FALSE);
    }

    return (TRUE);
}

/**     StoreMips - store return value for Mips call
 *
 *      fSuccess = StoreMips (pvF);
 *
 *      Entry   pvF = pointer to function descriptor
 *
 *      Exit    return value stored in ST
 *
 *      Returns TRUE if return value stored without error
 *              FALSE if error during store
 */


bool_t PASCAL
StoreMips (
    peval_t pvF)
{
    SHREG   argReg;
    ulong   len;

    Unreferenced( pvF );

    DASSERT( EVAL_TYP(ST) != T_REAL80 );

    // Floating point values are returned in f0 (Real) or f0:f1 (float)

    if ((EVAL_TYP (ST) == T_REAL32) ||
        (EVAL_TYP (ST) == T_REAL64) ) {
        argReg.hReg = (CV_M4_FltF1 << 8) | CV_M4_FltF0;
        GetReg ( &argReg, pCxt, NULL );

        if (EVAL_TYP (ST) == T_REAL32) {
            EVAL_FLOAT(ST) = *((float *) &argReg.Byte4);
        } else {
            EVAL_DOUBLE(ST) = *((double *) &argReg.Byte4);
        }
    }
    else if ((EVAL_TYP (ST) == T_CHAR) || (EVAL_TYP (ST) == T_RCHAR)) {
        argReg.hReg = CV_M4_IntV0;
        GetReg( &argReg, pCxt, NULL );
        EVAL_SHORT( ST ) = argReg.Byte1;
    }
    else if (EVAL_TYP (ST) == T_UCHAR) {
        argReg.hReg = CV_M4_IntV0;
        GetReg( &argReg, pCxt, NULL );
        EVAL_USHORT( ST ) = argReg.Byte1;
    }

    else if (EVAL_IS_PTR (ST)) {
        DASSERT( EVAL_IS_NPTR32(ST) );
        argReg.hReg = CV_M4_IntV0;
        GetReg( &argReg, pCxt, NULL );

        EVAL_PTR_OFF (ST) = argReg.Byte4;
        EVAL_ULONG (ST) = argReg.Byte4;

        EVAL_PTR_SEG (ST) = 0;
        ADDR_IS_FLAT( EVAL_PTR (ST)) = TRUE;
        ADDR_IS_OFF32( EVAL_PTR (ST)) = TRUE;
        ADDR_IS_REAL( EVAL_PTR( ST)) = FALSE;
        ADDR_IS_LI( EVAL_PTR (ST)) = FALSE;
    }

    else {

        len = EVAL_VALLEN(ST);
        argReg.hReg = CV_M4_IntV0;
        if (len == 8 && CV_IS_PRIMITIVE (EVAL_TYP (ST))) {
            argReg.hReg = (CV_M4_IntV1<<8) | CV_M4_IntV0;
        }
        GetReg( &argReg, pCxt, NULL );

        // Check for primitive lengths

        if (CV_IS_PRIMITIVE (EVAL_TYP (ST)) ||
            (EVAL_IS_CLASS (ST) && (len < 4) && (len != 3))) {

            if (len <= 2) {
                EVAL_USHORT(ST) = argReg.Byte2;
            } else if (len < 8) {
                EVAL_ULONG (ST) = argReg.Byte4;
            } else {
                EVAL_UQUAD (ST) = (argReg.Byte4High << 32i64) | argReg.Byte4;
            }
        } else {
            EVAL_SYM_OFF (ST) = argReg.Byte4;
            EVAL_SYM_SEG (ST) = 0;
            ADDR_IS_FLAT( EVAL_SYM (ST)) = TRUE;
            ADDR_IS_OFF32( EVAL_SYM (ST)) = TRUE;
            ADDR_IS_LI( EVAL_SYM (ST)) = FALSE;

            if (GetDebuggeeBytes(EVAL_SYM(ST),
                                 EVAL_VALLEN(ST),
                                 (char *)&EVAL_VAL(ST),
                                 EVAL_TYP(ST)) != (UINT)EVAL_VALLEN(ST)) {
                pExState->err_num = ERR_PTRACE;
                return FALSE;
            }
        }
    }
    return (TRUE);
}

/**     StoreAlpha - store return value for Alpha call
 *
 *      fSuccess = StoreAlpha (pvF);
 *
 *      Entry   pvF = pointer to function descriptor
 *
 *      Exit    return value stored in ST
 *
 *      Returns TRUE if return value stored without error
 *              FALSE if error during store
 */


bool_t PASCAL
StoreAlpha (
    peval_t pvF
    )
{
    SHREG   argReg;
    ulong   len;

    Unreferenced( pvF );

    DASSERT( EVAL_TYP(ST) != T_REAL80 );


    if (CV_TYP_IS_REAL (EVAL_TYP (ST) ) ) {

        union {
            float f;
            double d;
            unsigned long l[2];
        } u;

        // Floating point values are returned in F0;
        argReg.hReg = CV_ALPHA_FltF0;
        GetReg( &argReg, pCxt, NULL);

        // The SHREG is unaligned; this compiler can only do aligned
        // loads and stores of floating points.

        u.l[0] = argReg.Byte4;
        u.l[1] = argReg.Byte4High;

        if (EVAL_TYP (ST) == T_REAL64) {
            EVAL_DOUBLE(ST) = u.d;
            return (TRUE);
        }

        if (EVAL_TYP (ST) == T_REAL32) {
            // This does the conversion from double to Single

            u.f = (float) u.d;
            EVAL_FLOAT(ST) = u.f;
            return (TRUE);
        }
    }

    // All other values are returned in IntV0 (r0)

    argReg.hReg = CV_ALPHA_IntV0;
    GetReg( &argReg, pCxt, NULL);

    switch (EVAL_TYP (ST) ) {
        case T_CHAR:
        case T_RCHAR:
            EVAL_SHORT(ST) = argReg.Byte1;
            return (TRUE);
            break;
        case T_UCHAR:
            EVAL_USHORT(ST) = argReg.Byte1;
            return (TRUE);
            break;

    }

    if (EVAL_IS_PTR (ST)) {
        DASSERT( EVAL_IS_NPTR32(ST) );

        EVAL_PTR_OFF (ST) = argReg.Byte4;
        EVAL_ULONG (ST) = argReg.Byte4;

        EVAL_PTR_SEG (ST) = 0;
        ADDR_IS_FLAT( EVAL_PTR (ST)) = TRUE;
        ADDR_IS_OFF32( EVAL_PTR (ST)) = TRUE;
//      ADDR_IS_REAL( EVAL_PTR( ST)) = FALSE;
        ADDR_IS_LI( EVAL_PTR (ST)) = FALSE;
    } else {

        len = EVAL_VALLEN(ST);

        // Check for primitive lengths
#pragma message ("Add __int64 return values for ALPHA")

        if (CV_IS_PRIMITIVE (EVAL_TYP (ST)) ||
            (EVAL_IS_CLASS (ST) && (len <= 4) && (len != 3))) {

            if (len <= 2) {
                EVAL_USHORT(ST) = argReg.Byte2;
            } else {
                EVAL_ULONG (ST) = argReg.Byte4;
            }
        } else {
            EVAL_SYM_OFF (ST) = argReg.Byte4;
            EVAL_SYM_SEG (ST) = 0;
//          ADDR_IS_FLAT( EVAL_SYM (ST)) = TRUE;
//          ADDR_IS_OFF32( EVAL_SYM (ST)) = TRUE;
            ADDR_IS_LI( EVAL_SYM (ST)) = FALSE;

            if (GetDebuggeeBytes(EVAL_SYM(ST),
                                 EVAL_VALLEN(ST),
                                 (char *)&EVAL_VAL(ST),
                                 EVAL_TYP(ST)) != (UINT)EVAL_VALLEN(ST)) {
                pExState->err_num = ERR_PTRACE;
                return FALSE;
            }
        }
    }
    return (TRUE);
}


/**     StorePPC - store return value for PPC call
 *
 *      fSuccess = StorePPC (pvF);
 *
 *      Entry   pvF = pointer to function descriptor
 *
 *      Exit    return value stored in ST
 *
 *      Returns TRUE if return value stored without error
 *              FALSE if error during store
 */


bool_t PASCAL
StorePPC (
    peval_t pvF
    )
{
    SHREG   argReg;
    ulong   len;

    Unreferenced( pvF );

    DASSERT( EVAL_TYP(ST) != T_REAL80 );

    /*
     *  Floating point values are returned in FPR1
     */

    if ((EVAL_TYP (ST) == T_REAL32) ||
        (EVAL_TYP (ST) == T_REAL64) ) {
        argReg.hReg = (CV_PPC_FPR1);
        GetReg( &argReg, pCxt, NULL);

        if (EVAL_TYP (ST) == T_REAL32) {
            EVAL_FLOAT(ST) = (float) argReg.Byte8;
        } else {
            EVAL_DOUBLE(ST) = *((double *) &argReg.Byte4);
        }
    }
    else if ((EVAL_TYP (ST) == T_CHAR) || (EVAL_TYP (ST) == T_RCHAR)) {
        argReg.hReg = CV_PPC_GPR3;
        GetReg( &argReg, pCxt, NULL);
        EVAL_SHORT( ST ) = argReg.Byte1;
    }
    else if (EVAL_TYP (ST) == T_UCHAR) {
        argReg.hReg = CV_PPC_GPR3;
        GetReg( &argReg, pCxt, NULL);
        EVAL_USHORT( ST ) = argReg.Byte1;
    }
    else if (EVAL_IS_PTR (ST)) {
        DASSERT( EVAL_IS_NPTR32(ST) );
        argReg.hReg = CV_PPC_GPR3;
        GetReg( &argReg, pCxt, NULL);

        EVAL_PTR_OFF (ST) = argReg.Byte4;
        EVAL_ULONG (ST) = argReg.Byte4;

        EVAL_PTR_SEG (ST) = 0;
        ADDR_IS_FLAT( EVAL_PTR (ST)) = TRUE;
        ADDR_IS_OFF32( EVAL_PTR (ST)) = TRUE;
        ADDR_IS_REAL( EVAL_PTR( ST)) = FALSE;
        ADDR_IS_LI( EVAL_PTR (ST)) = FALSE;
    }

    else {

        len = EVAL_VALLEN(ST);
        argReg.hReg = CV_PPC_GPR3;
        GetReg( &argReg, pCxt, NULL);

        /*
        *  Check for primitive lengths
        */

        if (CV_IS_PRIMITIVE (EVAL_TYP (ST)) ||
            (EVAL_IS_CLASS (ST) && (len < 4) && (len != 3))) {

            if (len <= 2) {
                EVAL_USHORT(ST) = argReg.Byte2;
            } else {
                EVAL_ULONG (ST) = argReg.Byte4;
            }
        } else {
            EVAL_SYM_OFF (ST) = argReg.Byte4;
            EVAL_SYM_SEG (ST) = 0;
            ADDR_IS_FLAT( EVAL_SYM (ST)) = TRUE;
            ADDR_IS_OFF32( EVAL_SYM (ST)) = TRUE;
            ADDR_IS_LI( EVAL_SYM (ST)) = FALSE;

            if (GetDebuggeeBytes(EVAL_SYM(ST), EVAL_VALLEN(ST),
                                    (char *)&EVAL_VAL(ST), EVAL_TYP(ST))
                        != (UINT)EVAL_VALLEN(ST)) {
                pExState->err_num = ERR_PTRACE;
                return FALSE;
            }
        }
    }
    return (TRUE);
}                               /* StorePPC() */

/**     StoreIa64 - store return value for IA64 call
 *
 *      fSuccess = StoreIA64 (pvF);
 *
 *      Entry   pvF = pointer to function descriptor
 *
 *      Exit    return value stored in ST
 *
 *      Returns TRUE if return value stored without error
 *              FALSE if error during store
 *
 */

LOCAL bool_t NEAR PASCAL
StoreIa64 (
    peval_t pvF)
{
    SHREG   argReg;
    ulong  len;

    Unreferenced( pvF );

    DASSERT( EVAL_TYP(ST) != T_REAL80 );

    if (CV_TYP_IS_REAL ( EVAL_TYP (ST) ) ) {

        union {
            float         f;
            double        d;
            unsigned long l[2];
        } u;

        // EM Floating point values are returned in f2  

        argReg.hReg = CV_IA64_FltS0;
        GetReg ( &argReg, pCxt , NULL);

        // The SHREG is unaligned; this compiler can only do aligned
        // loads and stores of floating points.

        u.l[0] = argReg.Byte4;
        u.l[1] = argReg.Byte4High;

        if (EVAL_TYP (ST) == T_REAL64) {
            EVAL_DOUBLE(ST) = u.d;
            return (TRUE);
        }

        if (EVAL_TYP (ST) == T_REAL32) {

            // This does the conversion from double to single

            u.f = (float)u.d;
            EVAL_FLOAT (ST) = u.f;
            return (TRUE);
        }
    }

    //  All other values are returned in IntV0 (r0)

    argReg.hReg = CV_IA64_IntV0;
    GetReg( &argReg, pCxt, NULL);

    switch( EVAL_TYP(ST) ) {
        case T_CHAR:
        case T_RCHAR:
            EVAL_SHORT( ST ) = argReg.Byte1;
            return (TRUE);
            break;

        case T_UCHAR:
            EVAL_USHORT( ST ) = argReg.Byte1;
            return (TRUE);
            break;
    }

    if (EVAL_IS_PTR (ST)) {
        DASSERT( EVAL_IS_NPTR32(ST) );

        EVAL_PTR_OFF (ST) = argReg.Byte4;
        EVAL_ULONG (ST) = argReg.Byte4;

        EVAL_PTR_SEG (ST) = 0;
        ADDR_IS_OFF32 ( EVAL_PTR (ST)) = TRUE;
        ADDR_IS_FLAT ( EVAL_PTR (ST)) = TRUE;
        ADDR_IS_LI( EVAL_PTR (ST)) = FALSE;
    }
    else {

        len = EVAL_VALLEN(ST);

        //  Check for primitive lengths

#pragma message ("Add __int64 return value's for IA64")

        if (CV_IS_PRIMITIVE (EVAL_TYP (ST)) ||
            (EVAL_IS_CLASS (ST) && (len <= 4) && (len != 3))) {

            if (len <= 2) {
                EVAL_USHORT(ST) = argReg.Byte2;
            } else {
                EVAL_ULONG (ST) = argReg.Byte4;
            }
        } else {
            EVAL_SYM_OFF (ST) = argReg.Byte4;
            EVAL_SYM_SEG (ST) = 0;
            ADDR_IS_LI (EVAL_SYM (ST)) = FALSE;

            if (GetDebuggeeBytes(EVAL_SYM(ST),
                                 EVAL_VALLEN(ST),
                                 (char *)&EVAL_VAL(ST),
                                 EVAL_TYP(ST)) != (UINT)EVAL_VALLEN(ST)) {
                pExState->err_num = ERR_PTRACE;
                return FALSE;
            }
        }
    }
    return (TRUE);
}





/**     PushUserValue - push value onto user stack
 *
 *      fSuccess = PushUserValue (pv, pa, pspReg, pmaxSP)
 *
 *      Entry   pv = pointer to value
 *              pa = pointer to argument data describing stack offset
 *              spReg = pointer to register packet for stack pointer
 *              pmaxSP = pointer to location to store maximum SP relative offset
 *
 *      Exit    value pushed onto user stack
 *
 *      Returns TRUE if value pushed
 *              FALSE if error in push
 */


bool_t PushUserValue (peval_t pv, pargd_t pa, SHREG *pspReg, UOFFSET *pmaxSP)
{
    ADDR    addrStk = {0};
    uint    offsize;

    *pmaxSP = max (*pmaxSP, pa->SPoff);
    addrStk = EVAL_SYM (pv);
    addrStk.addr.seg = GetSegmentRegister(pExState->hframe, REG_SS);;
    addrStk.addr.off = pspReg->Byte4 - pa->SPoff;
    ADDR_IS_LI (addrStk) = FALSE;
    if (EVAL_IS_PTR (pv)) {
        // since pointers are stored strangely, we have to put them
        // in two pieces.  First we store the offset (16 or 32 bits)

        if (ADDR_IS_LI (EVAL_PTR (pv))) {
            SHFixupAddr (&EVAL_PTR (pv));
        }
        offsize = modeAddr (EVAL_PTR (pv)).fOff32 ? sizeof (CV_uoff32_t) : sizeof (CV_uoff16_t);
        if (PutDebuggeeBytes (addrStk, offsize, &EVAL_PTR_OFF (pv),
            (CV_typ_t)(modeAddr(EVAL_PTR (pv)).fOff32 ? T_ULONG : T_USHORT)) == offsize) {
            if (EVAL_IS_NPTR (pv) || EVAL_IS_NPTR32 (pv)) {
                return (TRUE);
            }
            else {
                addrStk.addr.off += offsize;
                if (PutDebuggeeBytes (addrStk, sizeof (_segment),
                  (char *)&EVAL_PTR_SEG (pv), T_SEGMENT) == sizeof (_segment)) {
                    return (TRUE);
                }
            }
        }
    }
    else {
        if (PutDebuggeeBytes (addrStk, pa->vallen,
          (char *)&EVAL_VAL (pv), EVAL_TYP(pv)) == pa->vallen) {
            return (TRUE);
        }
    }
    pExState->err_num = ERR_PTRACE;
    return (FALSE);
}



/**     PushOffset - push address value for pascal and fastcall
 *
 *      fSuccess = PushOffset (offset, pspReg, pmaxSP, size);
 *
 *      Entry   offset = offset from user's SP of return value
 *              pspReg = pointer to register structure for SP
 *              pmaxSP = pointer to location to store maximum SP relative offset
 *              size = size in bytes of value to be pushed
 *
 *      Exit
 *
 *      Returns TRUE if offset pushed
 *              FALSE if error
 *
 *      Note    Can be used to push segment also
 */


bool_t PushOffset (UOFFSET offset, SHREG *pspReg,
  UOFFSET *pmaxSP, ulong  size)
{
    ADDR    addrStk = {0};

    Unreferenced( size );

    *pmaxSP += size;
    addrStk.addr.seg = GetSegmentRegister(pExState->hframe, REG_SS);;
    addrStk.addr.off = pspReg->Byte4 - *pmaxSP; //expect a warning for 16 bit
    if (PutDebuggeeBytes (addrStk, size, (char *)&offset, T_USHORT) == size) {
        return (TRUE);
    }
    else {
        pExState->err_num = ERR_PTRACE;
        return (FALSE);
    }
}




/***    StructElem - Extract a structure element from stack
 *
 *      fSuccess = StructElem (bn)
 *
 *      Entry   bn = based pointer to operator node
 *              ST = address of struct (initial this address)
 *
 *      Exit    ST = value node for member
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 */


bool_t FASTCALL StructElem (bnode_t bnOp)
{
    peval_t     pvOp;
    peval_t     pvR;
    bnode_t     bnR;
    char   *pS;

    pvOp = &bnOp->v[0];
    bnR = NODE_RCHILD (bnOp);
    pvR = &bnR->v[0];
    if (EVAL_IS_MEMBER (pvOp) == FALSE) {
        pExState->err_num = ERR_NEEDLVALUE;
        return (FALSE);
    }
    if (EVAL_IS_METHOD (pvR)) {
        // this is to prevent cascaded function calls
        // like ?foo().bar() Currently we do not
        // handle this case --caviar #5425
        pExState->err_num = ERR_NOTEVALUATABLE;
        return (FALSE);
    }

    SetNodeType (ST, EVAL_TYP (pvR));
    EVAL_VALLEN (ST) = (ulong )TypeSize (ST);
    pS = (char *)&EVAL_VAL (ST);
    pS += MEMBER_OFFSET (pvOp);
    memmove (&EVAL_VAL (ST), pS, EVAL_VALLEN (ST));
    return (TRUE);
}





/***    StructEval - Perform a structure access (., ->, ::, .*, ->*)
 *
 *      fSuccess = StructEval (bn)
 *
 *      Entry   bn = based pointer to operator node
 *              ST = address of struct (initial this address)
 *
 *      Exit    ST = value node for member
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 */


bool_t FASTCALL StructEval (bnode_t bn)
{
    peval_t     pv;
    peval_t     pvR;
    bnode_t     bnR;
    CV_typ_t    typ;
    bool_t      retval;
    eval_t      evalT;
    peval_t     pvT;

    if (EVAL_IS_REF (ST)) {
        if (!FetchOp (ST)) {
            return (FALSE);
        }
        EVAL_IS_REF (ST) = FALSE;
    }

    // point to the eval_t field of the operator node.  This field will
    // contain the data needed to adjust the this pointer (*ST).  For
    // any structure reference (., ->, ::, .*, ->*), the stack top is
    // the initial value of the this pointer

    pv = &bn->v[0];
    if ((EVAL_IS_MEMBER (pv) == FALSE) || MEMBER_THISEXPR (pv) == 0) {
        *pvThis = *ST;
    }
    else if (Eval ((bnode_t)MEMBER_THISEXPR (pv)) == FALSE) {
        return (FALSE);
    }
    if ((MEMBER_VBASE (pv) == TRUE) || (MEMBER_IVBASE (pv) == TRUE)) {
        if (CalcThisExpr (MEMBER_VBPTR (pv), MEMBER_VBPOFF (pv),
          MEMBER_VBIND (pv), MEMBER_TYPE (pv)) == FALSE) {
            return (FALSE);
        }
    }
    *ST = *pvThis;
    if (!OP_IS_IDENT (NODE_OP (NODE_RCHILD (bn)))) {
        if ((retval = EvalRChild (bn)) == FALSE) {
            return (FALSE);
        }
    }
    else {
        bnR = NODE_RCHILD (bn);
        pvR = &bnR->v[0];
        if (EVAL_IS_STMEMBER (pvR) && EVAL_HSYM(pvR) == 0) {
            // missing static data member
            pExState->err_num = ERR_STMEMBERNP;
            return FALSE;
        }
        if (EVAL_IS_STMEMBER (pvR) ||
            // if we are looking at a nested enumerate
            // sps 9/15/92
            (EVAL_STATE(pvR) == EV_constant)) {
            *ST = *pvR;
            return (TRUE);
        }
        else if (EVAL_IS_METHOD (pvR)) {
            if (FCN_NOTPRESENT(pvR)) {
                // can't get the address of a function
                // unless it is present
                pExState->err_num = ERR_METHODNP;
                return FALSE;
            }
            if ((FCN_PROPERTY (pvR) == CV_MTvirtual) ||
              (FCN_PROPERTY (pvR) == CV_MTintro)) {
                pvT = &evalT;
                if (VFuncAddress2 (pvT, pvR) == FALSE) {
                    // we may get here if the object
                    // does not have a valid vfptr
                    // (e.g., if it is not initialized)
                    pExState->err_num = ERR_NOTEVALUATABLE;
                    return (FALSE);
                }
                else {
                    *ST = *pvR;
                    EVAL_SYM (ST) = EVAL_PTR (pvT);
                    // dolphin #2223: set EVAL_PTR(ST)
                    // so that FormatNode can display
                    // the function address
                    EVAL_PTR (ST) = EVAL_PTR (pvT);
                }
            }
            else {
                *ST = *pvR;
            }
            return (TRUE);
        }
        else {
            EVAL_SYM_OFF (ST) += MEMBER_OFFSET (pv);
            typ = EVAL_TYP (pvR);
            return (SetNodeType (ST, typ));
        }
    }
    return(TRUE);
}



/**     VFuncAddress - compute virtual function address
 *
 *      fSuccess = VFuncAddress (pv, index)
 *
 *      Entry   pv = pointer to pointer node to adjust
 *              index = vtshape table index
 *              pvThis = initial this pointer
 *
 *      Exit    pv = adjusted pointer
 *
 *      Returns TRUE if adjustment made
 *              FALSE if error
 */


INLINE bool_t VFuncAddress (peval_t pv, ulong  index)
{
    eval_t      evalT;
    peval_t     pvT;
    plfVTShape  pShape;
    uint        desc;
    ulong       i;
    ulong       shape;
    DTI         dti;
    bool_t      fOff32;
    ADDR        addr = {0};

    fOff32 = TRUE;

    pvT = &evalT;
    *pvT = *pv;
    SetNodeType (pvT, PTR_UTYPE (pvT));
    EVAL_STATE (pvT) = EV_lvalue;
    if (!EVAL_IS_VTSHAPE (pvT)) {
        // the only way we should get array referencing on a pointer
        // is if the pointer is a vfuncptr.

        DASSERT (FALSE);
        return (FALSE);
    }
    if (!EvalUtil (OP_fetch, pv, NULL, EU_LOAD)) {
        return (FALSE);
    }
    CLEAR_EVAL_FLAGS (pv);

#if 0
    // We read in the vtable ptr just as a normal 4 byte quantity.
    // Copy over the offset and create a unfixedup address.

    ADDRLIN32(addr);
    // addr has been zero initialized so other fields are just the way
    // we want them.

    SE_SetAddrOff(&addr, EVAL_ULONG(pv));
#else
        addr = EVAL_PTR(pv);

#endif
    SHUnFixupAddr(&addr);

    EVAL_SYM (pv) = addr;
    EVAL_STATE (pv) = EV_lvalue;
    EVAL_IS_ADDR (pv) = TRUE;
    EVAL_IS_PTR (pv) = TRUE;

    // now walk down the descriptor list, incrementing the pointer
    // address by the size of the entry described by the shape table

    pShape = (plfVTShape)(&((TYPPTR)MHOmfLock (EVAL_TYPDEF (pvT)))->leaf);
    for (i = 0; i < index; i++) {
        shape = pShape->desc[i >> 1];
        desc = (shape >> ((~i & 1) * 4)) & 0x0f;
        switch (desc) {
            case CV_VTS_near:
                EVAL_SYM_OFF (pv) += sizeof (CV_uoff16_t);
                break;

            case CV_VTS_far:
                EVAL_SYM_OFF (pv) += sizeof (CV_uoff16_t) + sizeof (_segment);
                break;

            case CV_VTS_outer:  // address point displacement
                EVAL_SYM_OFF (pv) += sizeof (CV_uoff32_t);
                break;

            case CV_VTS_meta:   // far pointer to a metaclass descriptor
#pragma message ("CAUTION - FLAT 32 specific")
                EVAL_SYM_OFF (pv) += sizeof (CV_uoff32_t);
                break;

            case CV_VTS_near32:
                EVAL_SYM_OFF (pv) += sizeof (CV_uoff32_t);
                break;

            case CV_VTS_far32:
                EVAL_SYM_OFF (pv) += sizeof (CV_uoff32_t) + sizeof (_segment);
                break;

            default:
                DASSERT (FALSE);
                MHOmfUnLock (EVAL_TYPDEF (pvT));
                return (FALSE);
        }
    }
    shape = pShape->desc[i >> 1];
    desc = (shape >> ((~i & 1) * 4)) & 0x0f;
    MHOmfUnLock (EVAL_TYPDEF (pvT));
    switch (desc) {
        case CV_VTS_near:
            EVAL_PTRTYPE (pv) = CV_PTR_NEAR;
            break;

        case CV_VTS_far:
            EVAL_PTRTYPE (pv) = CV_PTR_FAR;
            break;

        case CV_VTS_outer:  // address point displacement
            EVAL_PTRTYPE (pv) = CV_PTR_NEAR32;
            break;

        case CV_VTS_meta:   // far pointer to a metaclass descriptor
            EVAL_PTRTYPE (pv) = CV_PTR_NEAR32;
            break;

        case CV_VTS_near32:
            EVAL_PTRTYPE (pv) = CV_PTR_NEAR32;
            break;

        case CV_VTS_far32:
            EVAL_PTRTYPE (pv) = CV_PTR_FAR32;
            break;

        default:
            return (FALSE);
    }
    if (EvalUtil (OP_fetch, pv, NULL, EU_LOAD)) {
        CLEAR_EVAL_FLAGS (pv);
        EVAL_IS_ADDR (pv) = TRUE;
        EVAL_IS_FCN (pv) = TRUE;
        return (TRUE);
    }
    return (FALSE);
}




/**     VFuncAddress2 - compute virtual function address given the function
 *                         offset in the vtable
 *
 *      fSuccess = VFuncAddress (pv, pvFunc)
 *
 *      Entry   pv = pointer to pointer node to hold the function address
 *              pvFunc = evaluation node of virtual function
 *              pvThis = initial this pointer
 *
 *      Exit    pv value is the address of the vfunction
 *
 *      Returns TRUE if adjustment made
 *              FALSE if error
 */

INLINE bool_t VFuncAddress2 (peval_t pv, peval_t pvFunc)
{
    ADDR addr = {0};
    *pv = *pvThis;
    if (!SetNodeType (pv, FCN_VFPTYPE (pvFunc)) ||
        !EvalUtil (OP_fetch, pv, NULL, EU_LOAD)) {
        return (FALSE);
    }
    CLEAR_EVAL_FLAGS (pv);

    // We read in the vtable ptr just as a normal 4 byte quantity.
    // We need to use that to compose the actual address.

    ADDRLIN32(addr);

    // We have a physical address.
    SE_SetAddrOff(&addr, EVAL_ULONG(pv));
    SHUnFixupAddr(&addr);

    EVAL_SYM (pv) = addr;
    EVAL_STATE (pv) = EV_lvalue;
    EVAL_IS_ADDR (pv) = TRUE;
    EVAL_IS_PTR (pv) = TRUE;


    // adjust offset in the vtable,
    // set size of function pointer, and load pointer value

    EVAL_SYM_OFF (pv) += FCN_VTABIND(pvFunc);


    if (FCN_FARCALL(pvFunc)) {
        EVAL_PTRTYPE (pv) = CV_PTR_FAR;
    }
    else {
        EVAL_PTRTYPE (pv) = pExState->state.f32bit ?
            CV_PTR_NEAR32 : CV_PTR_NEAR;
    }

    if (EvalUtil (OP_fetch, pv, NULL, EU_LOAD)) {
        CLEAR_EVAL_FLAGS (pv);
        EVAL_IS_ADDR (pv) = TRUE;
        EVAL_IS_FCN (pv) = TRUE;
        return (TRUE);
    }
    return (FALSE);
}


INLINE ULONG MakeLong (USHORT lo, USHORT hi)
{
    return (ULONG)hi << 16 | lo;
}

INLINE USHORT LoWord (ULONG l)
{
    return (USHORT) (l & 0xffffL);
}

INLINE USHORT HiWord (ULONG l)
{
    return (USHORT) (l >> 16 & 0xffffL);
}

INLINE BOOL getFuncAddrNoThunks (peval_t pv)
{
    ulong  saveState = EVAL_STATE(pv);

    // look real address of a function - that is we will attempt to recognize thunks
    // and skip them to get to the real address of the functions - the address of the
    // function will be return in the EVAL_SYM of the incoming pv

//BUGBUG: I think this should be if (TargetMachine == mptx86)

#if (_MSC_VER >= 800) && defined(_M_IX86) && (_M_IX86 >= 300)
    eval_t evalTmp;
    peval_t pvTmp = &evalTmp;

    // fetch the contents of the purported address of the function, if we find a
    // unconditional jump, we may assume that we are looking at a thunk.  keep
    // looking until we no longer see a jump
    while (TRUE) {
        *pvTmp = *pv;
        EVAL_STATE(pvTmp) = EV_lvalue;
        // fetch opcode byte
        if (!SetNodeType (pvTmp, T_UCHAR) ||
            !EvalUtil (OP_fetch, pvTmp, NULL, EU_LOAD)) {
            return (FALSE);
        }

#define JMP32   ((char) 0xe9)

        if (EVAL_CHAR(pvTmp) == JMP32) {
            SHREG regCS;
            // fetch next four bytes of jump as relative offset
            EVAL_SYM_OFF(pvTmp)++;
            EVAL_STATE(pvTmp) = EV_lvalue;
            if (!SetNodeType (pvTmp, T_ULONG) ||
                !EvalUtil (OP_fetch, pvTmp, NULL, EU_LOAD)) {
                return (FALSE);
            }
            SHFixupAddr(&EVAL_SYM(pvTmp));
            EVAL_SYM_OFF(pv) = EVAL_SYM_OFF(pvTmp) + EVAL_ULONG(pvTmp) + 4;
            regCS.hReg = CV_REG_CS;
            GetReg (&regCS, pCxt, NULL);
            EVAL_SYM_SEG (pv) = regCS.Byte2;
            ADDR_IS_FLAT(EVAL_SYM(pv)) = TRUE;
            ADDR_IS_OFF32(EVAL_SYM(pv)) = TRUE;
            ADDR_IS_LI(EVAL_SYM(pv)) = FALSE;
            SHUnFixupAddr(&EVAL_SYM(pv));
            EVAL_STATE(pv) = EV_lvalue;
        }
        else {
            EVAL_STATE(pv) = saveState;
            return TRUE;
        }
    }
#else
    return TRUE;
#endif
}
