/***    debbind.c - Expression evaluator bind routines
 *
 *  GLOBAL
 *      Bind            Main evaluation routine
 *
 *
 * DESCRIPTION
 *      Routines to bind the expression tree.
 *      All routines require tokens to be no greater than 255 characters.
 *
 */

#include "debexpr.h"
#include "debsym.h"

#define TY_SIGNED   0x000001
#define TY_UNSIGNED 0x000002
#define TY_CHAR     0x000004
#define TY_SHORT    0x000008
#define TY_LONG     0x000010
#define TY_FLOAT    0x000020
#define TY_DOUBLE   0x000040
#define TY_SEGMENT  0x000080
#define TY_CLASS    0x000100
#define TY_STRUCT   0x000200
#define TY_UNION    0x000400
#define TY_REF      0x000800
#define TY_NEAR     0x001000
#define TY_FAR      0x002000
#define TY_HUGE     0x004000
#define TY_POINTER  0x008000
#define TY_UDT      0x010000
#define TY_VOID     0x020000
#define TY_CONST    0x040000
#define TY_VOLATILE 0x080000
#define TY_INT      0x100000
#define TY_QUAD     0x200000

#define TY_ARITH    (TY_SIGNED | TY_UNSIGNED | TY_CHAR | TY_SHORT | TY_LONG | TY_QUAD | TY_FLOAT | TY_DOUBLE)
#define TY_INTEGRAL (TY_CHAR | TY_SHORT | TY_LONG | TY_QUAD | TY_INT)
#define TY_REAL     (TY_FLOAT | TY_DOUBLE)
#define TY_NOTREAL  (TY_SIGNED | TY_UNSIGNED | TY_CHAR | TY_SHORT | TY_INT | TY_QUAD)
#define TY_PTR      (TY_NEAR | TY_FAR | TY_HUGE | TY_POINTER)
#define TY_AGGR     (TY_CLASS | TY_STRUCT | TY_UNION)
#define TY_SIGN     (TY_SIGNED | TY_UNSIGNED)


struct typrec {
    uchar   token[11];
    unsigned long flags;
};
static struct typrec SEGBASED(_segname("_CODE")) Predef[] = {
    { "\x006""signed",   TY_SIGNED},
    { "\x008""unsigned", TY_UNSIGNED},
    { "\x004""void",     TY_VOID},
    { "\x004""char",     TY_CHAR},
    { "\x003""int",      TY_INT},
    { "\x007""__int64",  TY_QUAD},
    { "\x005""short",    TY_SHORT},
    { "\x004""long",     TY_LONG},
    { "\x005""float",    TY_FLOAT},
    { "\x006""double",   TY_DOUBLE},
    { "\x008""_segment", TY_SEGMENT},
    { "\x009""__segment", TY_SEGMENT},
    { "\x006""struct",   TY_STRUCT},
    { "\x005""class",    TY_CLASS},
    { "\x005""union",    TY_UNION},
    { "\x001""*",        TY_POINTER},
    { "\x001""&",        TY_REF},
    { "\x004""near",     TY_NEAR},
    { "\x005""_near",    TY_NEAR},
    { "\x006""__near",   TY_NEAR},
    { "\x003""far",      TY_FAR},
    { "\x004""_far",     TY_FAR},
    { "\x005""__far",    TY_FAR},
    { "\x004""huge",     TY_HUGE},
    { "\x005""_huge",    TY_HUGE},
    { "\x006""__huge",   TY_HUGE},
    { "\x005""const",    TY_CONST},
    { "\x008""volatile", TY_VOLATILE},
    { "",                0}
};


//  Table to map from assignment operator to evaluation operator
//  Depends upon number and order of assignment operators

CV_typ_t eqop[OP_oreq + 1 - OP_multeq] = {
                OP_mult,
                OP_div,
                OP_mod,
                OP_plus,
                OP_minus,
                OP_shl,
                OP_shr,
                OP_and,
                OP_xor,
                OP_or
        };

bool_t  FASTCALL  AddrOf (bnode_t);
bool_t  FASTCALL  BDArith (op_t);
bool_t    BinaryOverload (bnode_t);
bool_t  FASTCALL  Bind (bnode_t);
bool_t  FASTCALL  BindLChild (bnode_t);
bool_t  FASTCALL  BindRchild (bnode_t);
bool_t  FASTCALL  BindAddrOf (bnode_t);
bool_t  FASTCALL  BindBinary (bnode_t);
bool_t  FASTCALL  BindArray (bnode_t);
bool_t  FASTCALL  BindAssign (bnode_t);
bool_t  FASTCALL  BindBang (bnode_t);
bool_t  FASTCALL  BindBasePtr (bnode_t);
bool_t  FASTCALL  BindByteOps(bnode_t);
bool_t  FASTCALL  BindCast (bnode_t);
bool_t  FASTCALL  BindConst (bnode_t);
bool_t  FASTCALL  BindContext (bnode_t);
bool_t  FASTCALL  BindExeContext(bnode_t);
bool_t  FASTCALL  BindDot (bnode_t bn);
bool_t  FASTCALL  BindFetch (bnode_t);
bool_t  FASTCALL  BindFunction (bnode_t);
bool_t  FASTCALL  BindDMember (bnode_t);
bool_t  FASTCALL  BindPlusMinus (bnode_t);
bool_t  FASTCALL  BindPMember (bnode_t);
bool_t  FASTCALL  BindPointsTo (bnode_t);
bool_t  FASTCALL  BindPostIncDec (bnode_t);
bool_t  FASTCALL  BindPreIncDec (bnode_t);
bool_t  FASTCALL  BindRelat (bnode_t);
bool_t  FASTCALL  BindRetVal (bnode_t);
bool_t  FASTCALL  BindBScope (bnode_t);
bool_t  FASTCALL  BindSegOp (bnode_t);
bool_t  FASTCALL  BindSizeOf (bnode_t);
bool_t  FASTCALL  BindSymbol (bnode_t);
bool_t  FASTCALL  BindUnary (bnode_t);
bool_t  FASTCALL  BuildType (CV_typ_t *, ulong *, ulong  *, ulong  *, ulong  *);
bool_t  FASTCALL  BindUScope (bnode_t);
bool_t    CastPtrToPtr (bnode_t);
bool_t  FASTCALL  ContextToken (char * *, char * *, int *);
HDEP      DupETree (ulong , pstree_t *);
bool_t  FASTCALL  FastCallReg (pargd_t, peval_t, ulong  *);
bool_t  FASTCALL  FastCallReg32 (pargd_t, peval_t, ulong  *);
bool_t  FASTCALL  FcnCast (bnode_t bn);
bool_t  FASTCALL  Fetch (void);
bool_t    FindUDT (bnode_t, peval_t, char *, char *, uchar);
bool_t  FASTCALL  Function (bnode_t);
uchar   FASTCALL  GetID (char *);
bool_t  FASTCALL  GetStructTDef (char *, int, pnode_t);
bool_t    FASTCALL  MipsCallReg (pargd_t, peval_t, uint *);
bool_t    FASTCALL  AlphaCallReg (pargd_t, peval_t, uint *);
bool_t    FASTCALL  IA64CallReg (pargd_t, peval_t, uint *);
CV_typ_t      GetProcType (HPROC);
bool_t  FASTCALL  ParseType (bnode_t, bool_t);
bool_t  FASTCALL  BDPlusMinus(op_t);
bool_t  FASTCALL  BDPrePost (op_t);
bool_t  FASTCALL  PushCArgs (peval_t, pnode_t, UOFFSET *, int, peval_t);
bool_t  FASTCALL  PushFArgs (peval_t, pnode_t, UOFFSET *, peval_t);
bool_t  FASTCALL  PushPArgs (peval_t, pnode_t, UOFFSET *, peval_t);
bool_t    FASTCALL  PushMArgs (peval_t, pnode_t, UOFFSET *, peval_t);
bool_t    FASTCALL  PushMArgs2 (peval_t, pnode_t, UOFFSET *, int, uint, peval_t);
bool_t    FASTCALL  PushAArgs (peval_t, pnode_t, UOFFSET *, peval_t);
bool_t    FASTCALL  PushAArgs2 (peval_t, pnode_t, UOFFSET *, int, uint, peval_t);
bool_t    FASTCALL  PushIA64Args (peval_t, pnode_t, UOFFSET *, peval_t);
bool_t    FASTCALL  PushIA64Args2 (peval_t, pnode_t, UOFFSET *, int, uint, peval_t);
bool_t  FASTCALL  SBitField (pnode_t);
bool_t  FASTCALL  SearchRight (bnode_t);
CV_typ_t      SetImpClass (PCXT, long *);
bool_t  FASTCALL  BDUnary (op_t);
bool_t    UnaryOverload (bnode_t);
bool_t    PointsToOverload (bnode_t);
bool_t  FASTCALL    BindError (bnode_t);
bool_t  FASTCALL    BindTRUE (bnode_t);
bnode_t FASTCALL    BnMatchOp ( bnode_t bn, op_t opMatch );
bool_t      CastBaseToDeriv (bnode_t, bool_t*);
CV_typ_t        TranslateClassIndex(CV_typ_t, HMOD);

static  bool_t  BindingFuncArgs = FALSE;
static  bool_t  BindingScopeOperand = FALSE;
static  bool_t  fNoFuncCxf = FALSE;     // TRUE if the hProc of the current
                                        // context has not been called (i.e.,
                                        // if it does not have a context frame)
bnode_t bnOp;       // based node pointer when binding the right side of
                    // ., ->,
// Bind dispatch table

bool_t (FASTCALL *SEGBASED(_segname("_CODE"))pBind[]) (bnode_t) = {
#define OPCNT(name, val)
#define OPCDAT(opc)
#define OPDAT(op, opfprec, opgprec, opclass, opbind, opeval) opbind,
#include "debops.h"
#undef OPDAT
#undef OPCDAT
#undef OPCNT
};

/*
 *  Defines relating to the MIPS and ALPHA calling convention
 *  One nibble is used for each register argument position
 *  There are a max of four for MIPS, six for ALPHA
 */


#define PARAM_EMPTY     0
#define PARAM_INT       1
#define PARAM_FLOAT     2
#define PARAM_DOUBLE    3
#define PARAM_SKIPPED   4

#define IS_PARAM_TYPE(mask, n, type) ((*mask & (3 << 4*n)) == type)
#define IS_PARAM_EMPTY(mask, n) (IS_PARAM_TYPE(mask, n, PARAM_EMPTY))
#define IS_PARAM_INT(mask, n) (IS_PARAM_TYPE(mask, n, PARAM_INT))
#define IS_PARAM_FLOAT(mask, n) (IS_PARAM_TYPE(mask, n, PARAM_FLOAT))
#define IS_PARAM_DOUBLE(mask, n) (IS_PARAM_TYPE(mask, n, PARAM_DOUBLE))
#define IS_PARAM_SKIPPED(mask, n) (IS_PARAM_TYPE(mask, n, PARAM_SKIPPED))


#define SET_PARAM_TYPE(mask, n, type) (*mask |= (type << 4*n))
#define SET_PARAM_INT(mask, n) SET_PARAM_TYPE(mask, n, PARAM_INT)
#define SET_PARAM_FLOAT(mask, n) SET_PARAM_TYPE(mask, n, PARAM_FLOAT)
#define SET_PARAM_DOUBLE(mask, n) SET_PARAM_TYPE(mask, n, PARAM_DOUBLE)
#define SET_PARAM_SKIPPED(mask, n) SET_PARAM_TYPE(mask, n, PARAM_SKIPPED)


/***    DoBind - bind evaluation tree tree
 *
 *      DoBind is the public entry to this module.  The bind copy of the
 *      parsed expression is initialized and the tree is bound in a
 *      leftmost bottom up order.
 *
 *      error = DoBind (phTM, pcxt, flags)
 *
 *      Entry   phTM = pointer to handle for TM
 *              pcxt = pointer to context packet
 *              flags.fForceBind = TRUE if bind to be forced
 *              flags.fForceBind = FALSE if rebind decision left to binder
 *              flags.fEnableProlog = TRUE if function scope searched during prolog
 *              flags.fEnableProlog = FALSE if function scope not searched during prolog
 *              flags.fSupOvlOps = FALSE if overloaded operator search enabled
 *              flags.fSupOvlOps = TRUE if overloaded operator search suppressed
 *              flags.fSupBase = FALSE if base searching is not suppressed
 *              flags.fSupBase = TRUE if base searching is suppressed
 *
 *      Exit    pExState->hETree = handle of bound evaluation tree
 *              pExState->hETree->estacksize = size of evaluation stack
 *              pExState->state.eval_ok = FALSE
 *              pExState->state.bind_ok = TRUE if no errors
 *
 *      Returns EENOERROR if syntax tree bound without error
 *              EENOMEMORY if unable to allocate memory
 *              EEGENERAL if error in bind (pExState->err_num = error)
 */


EESTATUS
DoBind (
    PHTM phTM,
    PCXT pcxt,
    uint flags
    )
{
    pstree_t    pSTree;
    ulong       error = EENOERROR;
    int         excess;

    // lock the expression state structure and copy the context package

    DASSERT (*phTM != 0);
    if (*phTM == 0) {
        return (EECATASTROPHIC);
    }
    DASSERT (*phTM != 0);
    if (hEStack == 0) {
        if ((hEStack = MemAllocate (ESTACK_DEFAULT * sizeof (elem_t))) == 0) {
            return (EECATASTROPHIC);
        }
        StackLen = (belem_t)((uint)ESTACK_DEFAULT * sizeof (elem_t));
    }
    pEStack = (pelem_t) MemLock (hEStack);
    pExState = (pexstate_t) MemLock (*phTM);
    pExState->state.fEProlog = (flags & BIND_fEnableProlog) == BIND_fEnableProlog;
    pExState->state.fSupOvlOps = (flags & BIND_fSupOvlOps) == BIND_fSupOvlOps;
    pExState->state.fSupBase = (flags & BIND_fSupBase) == BIND_fSupBase;
    pExState->state.fFunction = FALSE;
    if (GetAddrSeg(pcxt->addr) || GetAddrOff(pcxt->addr) || emiAddr(pcxt->addr)) {
        pExState->state.f32bit = ADDR_IS_OFF32 (pcxt->addr);
    }
    else {
        pExState->state.f32bit = TRUE;
    }
    if (pExState->state.parse_ok == TRUE) {
        pExState->err_num = 0;
        pExState->cxt = *pcxt;
        if ((pExState->state.bind_ok == FALSE) ||
          ( (flags & BIND_fForceBind) == BIND_fForceBind) ||
          (pExState->state.nullcontext == TRUE)) {
            // the expression has not been successfully bound, the caller
            // has forced the bind or the expression contains a null
            // context {} that forces a bind.  If none of these cases are
            // true, then we can exit without rebinding

            pExState->state.bind_ok = FALSE;
            pExState->state.eval_ok = FALSE;
            pExState->state.cChildren_ok = FALSE;
            pExState->state.fNotPresent = FALSE;

            // flag the fact that any auto-expand child TMs
            // should be rebound when needed
            pExState->state.fAErebind = TRUE;
            pExState->state.fAEreeval = TRUE;

            // Free auto-expand TM list
            EEFreeTML (&pExState->TMLAutoExpand);
            if (pExState->hAutoExpandRule)
            {
                MemFree(pExState->hAutoExpandRule);
                pExState->hAutoExpandRule = 0;
            }

            // Reset search state for GetClassiChild
            memset (&pExState->searchState, 0, sizeof (pExState->searchState));

            if (pExState->hETree != 0) {
                // free current evaluation tree if it exists
                MHMemFree (pExState->hETree);
            }

            // lock syntax tree and copy to evaluation tree for binding

            DASSERT ( pExState->hSTree != 0 );
            pSTree = (pstree_t) MemLock (pExState->hSTree);
            if ((pExState->hETree = MemAllocate (pSTree->size)) != 0) {

                // if evaluation tree is allocated, initialize and bind

                DASSERT ( pExState->hExStr != 0 );
                pExStr = (char *) MemLock (pExState->hExStr);

                DASSERT ( pExState->hETree != 0 );
                pTree = (pstree_t) MemLock (pExState->hETree);
                memcpy (pTree, pSTree, pSTree->size);

                // set pointer to context and flag fact that it is not
                // a pointer into the expression tree

                pCxt = &pExState->cxt;
                bnCxt = 0;
                ClassExp = T_NOTYPE;
                ClassImp = SetImpClass (pCxt, &ClassThisAdjust);

                // indicate that the stack is not in use by the parser

                pTree->stack_base = 0;
                pTree->stack_next = 0;

                // set the evaluation stack to the default fixed buffer.
                // bind will allocate a new buffer and move the pointers
                // if the stack overflows.  This work is effecient because
                // most expressions consist of a single token.

                StackOffset = 0;
                StackCkPoint = 0;
                StackMax = 0;
                memset (pEStack, 0, (size_t)(UINT_PTR)StackLen);

                // clear the stack top, stack top previous, function argument
                // list pointer and based pointer to operand node

                ST = NULL;
                STP = NULL;
                bArgList = NULL;
                bnOp = 0;
                if (Bind ((bnode_t)pTree->start_node) == TRUE) {
                    pExState->state.bind_ok = TRUE;
                    pExState->err_num = 0;
                    // set bind result in case API user asks for expression type
                    pExState->result = *ST;
                    if ((EVAL_IS_PTR (ST) == FALSE) &&
                      ((excess = (uint)TypeSize (ST) - sizeof (val_t)) > 0)) {
                        HTM hTM;

                        // since the return value is larger than normal, we
                        // need to reallocate the size of the expression state
                        // structure to include the extra return data

                        DASSERT (*phTM != 0);
                        MHMemUnLock (*phTM);
                        if ((hTM = MHMemReAlloc (*phTM, sizeof (exstate_t) + excess)) != 0) {

                            *phTM = hTM;
                            DASSERT ( *phTM != 0 );
                            pExState = (pexstate_t) MemLock (*phTM);
                            memcpy (&pExState->result, ST, sizeof (eval_t));
                        }
                        else {

                            DASSERT ( *phTM != 0 );
                            pExState = (pexstate_t) MemLock (*phTM);
                            pExState->err_num = ERR_NOMEMORY;
                            error = EEGENERAL;
                        }
                    }
                    if (EVAL_TYP (ST) == 0) {
                        error = EEGENERAL;
                    }
                }
                else {
                    error = EEGENERAL;
                }
                bArgList = NULL;
                bnCxt = 0;
                DASSERT (pExState->hExStr != 0);
                MemUnLock (pExState->hExStr);
                DASSERT ( pExState->hETree!= 0);
                MemUnLock (pExState->hETree);
            }
            else {
                error = EENOMEMORY;
            }
            DASSERT ( pExState->hSTree!= 0);
            MemUnLock (pExState->hSTree);
        }
    }
    DASSERT ( *phTM != 0 );
    MemUnLock (*phTM);
    MemUnLock (hEStack);

    return (error);
}



/**     SetImpClass - set implicit class
 *
 *      type = SetImpClass (pCxt, pThisAdjust);
 *
 *      Entry   pCxt = pointer to context packet
 *              pThisAdjust = pointer to implicit this adjustor value
 *
 *      Exit    none
 *
 *      Returns type index of implied class if context is method
 *              0 if context is not method
 */

CV_typ_t
SetImpClass (
    PCXT pCxt,
    long *pThisAdjust
    )
{
    HSYM        hProc;
    CV_typ_t    type;
    CV_typ_t    rettype = T_NOTYPE;
    HTYPE       hMFunc;
    plfMFunc    pMFunc;

    *pThisAdjust = 0;
    if ((hProc = SHHPROCFrompCXT (pCxt)) != 0) {
        // the current context is within some function.  Set the node
        // to the type of the function and see if it is a method of a class

        type = GetProcType (hProc);
        if ((hMFunc = THGetTypeFromIndex (SHHMODFrompCXT (pCxt), type)) != NULL) {
            pMFunc = (plfMFunc)((&((TYPPTR)MHOmfLock (hMFunc))->leaf));
            if (pMFunc->leaf == LF_MFUNCTION) {
                rettype = pMFunc->classtype;
                *pThisAdjust = pMFunc->thisadjust;
            }
            MHOmfUnLock (hMFunc);
        }
    }
    return (rettype);
}

/**     GetProcType - Get procedure type index
 *
 *      type = GetProcType(hProc);
 *
 *      Entry   hProc = handle to procedure
 *
 *      Exit    none
 *
 *      Returns type index of procedure
 */

CV_typ_t
GetProcType (
    HPROC hProc
    )
{
    SYMPTR      pProc;
    CV_typ_t    type;

    DASSERT (hProc);
    pProc = (SYMPTR)MHOmfLock (hProc);
    switch (pProc->rectyp) {
        case S_LPROC16:
        case S_GPROC16:
            type = ((PROCPTR16)pProc)->typind;
            break;

        case S_LPROC32:
        case S_GPROC32:
            type = ((PROCPTR32)pProc)->typind;
            break;
        case S_LPROCMIPS:
        case S_GPROCMIPS:
            type = ((PROCPTRMIPS)pProc)->typind;
            break;

        case S_LPROCIA64:
        case S_GPROCIA64:
            type = ((PROCPTRIA64)pProc)->typind;
            break;

        default:
            DASSERT (FALSE);
            type = 0;
    }
    MHOmfUnLock (hProc);
    return type;
}



/**     Bind - bind a node
 *
 *      Call the bind routine indexed by the the node type.  This could
 *      easily be a macro but is done as a function to save code space
 *
 *      fSuccess = Bind (bn)
 *
 *      Entry   bn = base pointer to node in evaluation tree
 *
 *      Exit    node and all children of node bound
 *
 *      Returns TRUE if no error in bind
 *              FALSE if error binding node or any child of node
 */


bool_t FASTCALL
Bind (
    register bnode_t bn
    )
{
    return ((*pBind[NODE_OP((pnode_t)bn)])(bn));
}



/**     BindLChild - bind the left child of a node
 *
 *      Call the bind routine indexed by the the node type of the left
 *      child of this node.  This could easily be a macro but
 *      is done as a function to save code space
 *
 *      fSuccess = BindLChild (bn)
 *
 *      Entry   bn = base pointer to node in evaluation tree
 *
 *      Exit    left child and children of node bound
 *
 *      Returns TRUE if no error in bind
 *              FALSE if error binding node or any child of node
 */



bool_t FASTCALL
BindLChild (
    register bnode_t bn
    )
{
    register bnode_t bnL = NODE_LCHILD (bn);

    return ((*pBind[NODE_OP(bnL)])(bnL));
}



/**     BindRChild - bind the right child of a node
 *
 *      Call the bind routine indexed by the the node type of the right
 *      child of this node.  This could easily be a macro but
 *      is done as a function to save code space
 *
 *      fSuccess = BindRChild (bn)
 *
 *      Entry   bn = base pointer to node in evaluation tree
 *
 *      Exit    node and all children of node bound
 *
 *      Returns TRUE if no error in bind
 *              FALSE if error binding left child of node or any child
 */



bool_t FASTCALL
BindRChild (
    register bnode_t bn
    )
{
    register bnode_t bnR = NODE_RCHILD (bn);

    return ((*pBind[NODE_OP(bnR)])(bnR));
}



/**     BindError - return bind error
 *
 *      Return bind error for an attempt to bind a node.  Normally this
 *      routine is the entry for a node type such as OP_rparen that
 *      should never appear in the final parse tree.
 *
 *      FALSE = BindError (bn)
 *
 *      Entry   bn = base pointer to node in evaluation tree
 *
 *      Exit    none
 *
 *      Returns FALSE
 */



bool_t FASTCALL
BindError (
    register bnode_t bn
    )
{
    Unreferenced( bn );

    pExState->err_num = ERR_INTERNAL;
    return (FALSE);
}




/**     BindTRUE - return bind successful
 *
 *      Return bind error for an attempt to bind a node.
 *
 *      TRUE = BindTRUE (bn)
 *
 *      Entry   bn = base pointer to node in evaluation tree
 *
 *      Exit    none
 *
 *      Returns TRUE
 */


bool_t FASTCALL
BindTRUE (
    register bnode_t bn
    )
{
    Unreferenced( bn );

    return (TRUE);
}




/***    BindAddrOf - Perform the address-of (&) operation
 *
 *      fSuccess = BindAddrOf (bn)
 *
 *      Entry   pn = pointer to tree node
 *
 *      Exit    NODE_STYPE (bn) = type of stack top
 *
 *      Returns TRUE if bind successful
 *              FALSE if bind error
 *
 *      Exit    pExState->err_num = error ordinal if bind error
 *
 */


bool_t FASTCALL
BindAddrOf (
    bnode_t bn
    )
{
    CV_typ_t    type = 0;

    if (!BindLChild (bn)) {
        return (FALSE);
    }

#ifdef NEVER
    // Dolphin #10293:
    // Disabled overloaded operator functionality for "&"
    // This code would make the watch window
    // extremely slugish in certain cases (e.g., when
    // expanding a recursively defined template), due
    // to the fact that "&" is being used by the EE
    // for constructing child expressions. Child expressions
    // are bound for the first time with overloaded operators
    // suppressed (see use of ParseBind); however if the kernel needs
    // to rebind a child expression or promote it to a parent
    // expression, the EE goes through this path and may become
    // several orders of magnitude slower; as a result the IDE
    // may appear to be hung.
    if ((pExState->state.fSupOvlOps == FALSE) && EVAL_IS_CLASS (ST) && (
      CLASS_PROP (ST).ovlops == TRUE)) {
        if (UnaryOverload (bn) == TRUE) {
            return (TRUE);
        }
    }
#endif
    return (AddrOf (bn));
}




/***    BindArray - Perform an array access ([])
 *
 *      fSuccess = BindArray (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Exit    ST = value of array element
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL
BindArray (
    bnode_t bn
    )
{
    eval_t      evalT;
    peval_t     pvT;

    if (BindLChild (bn) && BindRChild (bn)) {
        if ((pExState->state.fSupOvlOps == FALSE) && EVAL_IS_CLASS (STP) &&
          (CLASS_PROP (STP).ovlops == TRUE)) {
            return (BinaryOverload (bn));
        }
        else
            if (EVAL_IS_ARRAY (STP) || EVAL_IS_ARRAY (ST)) {
            // above check is for array[3] or 3[array]
            if (ValidateNodes (OP_lbrack, STP, ST) && BDPlusMinus (OP_plus)) {
                return (Fetch ());
            }
        }
        else if (EVAL_IS_PTR (STP)) {
            pvT = &evalT;
            *pvT = *STP;
            SetNodeType (pvT, PTR_UTYPE (pvT));
            if (EVAL_IS_VTSHAPE (pvT) && (EVAL_STATE (ST) == EV_constant) &&
              (EVAL_USHORT (ST) < VTSHAPE_COUNT (pvT))) {
                // we have a valid index into the shape table
                // set the node to code address
                // [dans 13 June 1993] removed dead code
                CLEAR_EVAL_FLAGS (STP);
                EVAL_IS_ADDR (STP);
                return (PopStack ());
            }
            else {
                if (ValidateNodes (OP_lbrack, STP, ST) && BDPlusMinus (OP_plus)) {
                    return (Fetch ());
                }
            }

        }
    }
    return (FALSE);
}





/***    BindAssign - Bind an assignment operation
 *
 *      fSuccess = BindAssign (op)
 *
 *      Entry   op  = operation
 *
 *      Returns TRUE if bind successful
 *              FALSE if bind error
 *
 *      Exit    pExState->err_num = error ordinal if bind error
 */


bool_t FASTCALL
BindAssign (
    bnode_t bn
    )
{
    CV_typ_t    nop;
    op_t        op = NODE_OP (bn);

    if (!BindLChild (bn) || !BindRChild (bn)) {
        return (FALSE);
    }

    // Left operand must have evaluated to an lvalue

    if (EVAL_STATE (STP) != EV_lvalue) {
        pExState->err_num = ERR_NEEDLVALUE;
        return (FALSE);
    }

    // In addition, l-value has to be modifiable,
    // i.e., cannot assign to arrays, functions and constants

    if (EVAL_IS_ARRAY(STP) || EVAL_IS_FCN(STP) || EVAL_IS_CONST(STP)) {
        pExState->err_num = ERR_NOMODIFIABLELV;
        return (FALSE);
    }
    if (EVAL_IS_CLASS (STP)) {
        pExState->err_num = ERR_NOCLASSASSIGN;
        return (FALSE);
    }
    if (EVAL_IS_REF (STP)) {
        RemoveIndir (STP);
    }
    if (EVAL_IS_REF (ST)) {
        RemoveIndir (ST);
    }
    if (EVAL_IS_ENUM (ST)) {
        SetNodeType (ST, ENUM_UTYPE (ST));
    }
    if (EVAL_IS_ENUM (STP)) {
        SetNodeType (STP, ENUM_UTYPE (STP));
    }
    if (NODE_OP (bn) == OP_eq) {

        // for simple assignment, load both nodes and do proper casting

        if (EVAL_IS_BASED (ST) && (EVAL_IS_ADDR (ST) ||
          (((EVAL_TYP (ST) == T_INT4) || (EVAL_TYP(ST) == T_UINT4)) &&
          (EVAL_ULONG (ST) != 0L)) ||
          (((EVAL_TYP (ST) == T_LONG) || (EVAL_TYP(ST) == T_ULONG)) &&
          (EVAL_ULONG (ST) != 0L)))) {
            //M00KLUDGE - this should go through CastNode
            if (!DeNormalizePtr (ST, STP)) {
                return (FALSE);
            }
        }
        if (EVAL_IS_BASED (STP)) {
            if (!NormalizeBase (STP)) {
                return (FALSE);
            }
        }
    }
    else {
        // map assignment operator to arithmetic operator
        // push address and value onto top of stack and
        // perform operation

        if (!PushStack (STP) || !PushStack (STP)) {
            pExState->err_num = ERR_NOMEMORY;
            return (FALSE);
        }
        switch (nop = eqop[op - OP_multeq]) {
            case OP_plus:
            case OP_minus:
                BDPlusMinus ((op_t) nop);
                break;

            default:
                BDArith ((op_t) nop);
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
    }
    else if (EVAL_IS_ADDR (STP)) {
        if (!EVAL_IS_ADDR (ST)) {
            // M00FLAT32 - assumes equivalence between far pointer and long
            // M00FLAT32 - this is a problem for 32 bit model

            if (CastNode (ST, T_LONG, T_LONG) == FALSE) {
                return (FALSE);
            }
        }
        // REVIEW: This code was added to handle the case of
        // ptr = array (both are also addr)
        // It was too late to try changing the overall order of
        // the test (move the PTR test before the ADDR test)
        // BRUCEJO 6-15-93
        else if (EVAL_IS_PTR (STP)) {
            if (CastNode (ST, EVAL_TYP (STP), PTR_UTYPE (STP)) == FALSE) {
                return (FALSE);
            }
        }
    }
    else if (EVAL_IS_PTR (STP)) {
        if (CastNode (ST, EVAL_TYP (STP), PTR_UTYPE (STP)) == FALSE) {
            return (FALSE);
        }
    }
    else {
        if (CastNode (ST, EVAL_TYP (STP), EVAL_TYP (STP)) == FALSE) {
            return (FALSE);
        }
    }
    *STP = *ST;
    return (PopStack ());

}



/***    BindBang - bind logical negation operation
 *
 *      fSuccess = BindBang (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if find successful
 *              FALSE if bind error
 */


bool_t FASTCALL
BindBang (
    bnode_t bn
    )
{
    if (!BindLChild (bn)) {
        return (FALSE);
    }

    // we need to check for a reference to a class without losing the fact
    // that this is a reference

    if (EVAL_IS_REF (ST)) {
        RemoveIndir (ST);
    }
    if ((pExState->state.fSupOvlOps == FALSE) && EVAL_IS_CLASS (ST) &&
      (CLASS_PROP (ST).ovlops == TRUE)) {
        return (UnaryOverload (bn));
    }
    if (!ValidateNodes (OP_bang, ST, NULL)) {
        return (FALSE);
    }

    // If the operand is not of pointer type, just pass it on to BDUnary

    if (!EVAL_IS_PTR (ST)) {
        return (BDUnary (OP_bang));
    }

    // The result is 1 if the pointer is a null pointer and 0 otherwise

    EVAL_STATE (ST) = EV_rvalue;
    SetNodeType (ST, (CV_typ_t) (pExState->state.f32bit ? T_INT4 : T_INT2));
    return (TRUE);
}




/***    BindBasePtr - Perform a based pointer access (:>)
 *
 *      fSuccess = BindBasePtr (bnRight)
 *
 *      Entry   bnRight = based pointer to right operand node
 *
 *      Exit
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL
BindBasePtr (
    bnode_t bn
    )
{
    return (BindSegOp (bn));
}




/**     BindBinary - bind an unary arithmetic operation
 *
 *      fSuccess = BindBinary (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if no error during evaluation
 *              FALSE if error during evaluation
 *
 */


bool_t FASTCALL
BindBinary (
    bnode_t bn
    )
{
    if (!BindLChild (bn) || !BindRChild (bn)) {
        return (FALSE);
    }
    if (EVAL_IS_REF (STP)) {
        RemoveIndir (STP);
    }
    if (EVAL_IS_REF (ST)) {
        RemoveIndir (ST);
    }
    if ((pExState->state.fSupOvlOps == FALSE) &&
      (EVAL_IS_CLASS (ST) && (CLASS_PROP (ST).ovlops == TRUE)) ||
      (EVAL_IS_CLASS (STP) && (CLASS_PROP (STP).ovlops == TRUE))) {
        return (BinaryOverload (bn));
    }
    if (EVAL_IS_ENUM (ST)) {
        SetNodeType (ST, ENUM_UTYPE (ST));
    }
    if (EVAL_IS_ENUM (STP)) {
        SetNodeType (STP, ENUM_UTYPE (STP));
    }
    return (BDArith (NODE_OP (bn)));
}





/***    BindBScope - Bind binary :: scoping operator
 *
 *      fSuccess = BindBScope (bn);
 *
 *      Entry   bn = based pointer to :: node
 *
 *      Exit    ST = evaluated class::ident
 *
 *      Returns TRUE if bind successful
 *              FALSE if error
 */


bool_t FASTCALL
BindBScope (
    bnode_t bn
    )
{
    CV_typ_t    oldClassExp;
    bool_t      retval;
    bnode_t     oldbnOp;
    char   *pbName;
    ulong       len;
    CV_typ_t    CurClass;
    HTYPE       hBase;          // handle to type record for base class
    uchar  *pField;         // pointer to field list
    char   *pc;
    uint        tSkip;
    ulong       cmpflag = 1;
    peval_t     pv;
    bool_t      fGlobal = FALSE;
    search_t    Name;
    HR_t        sRet;
    CV_typ_t    oldClassImp;
    op_t        savedop;
    eval_t      savedeval;
    bool_t      oldBindingScopeOp;

    // bind the left child using the current explicit class.
    // set the explicit class to the type of the left child and
    // bind the right hand side.  Then move the right hand bind
    // result over the left hand bind result and discard the stack
    // top.  This has the effect of bubbling the result of the right
    // hand bind to the top.

    // first we must check for pClass->Class::member or Class.Class::member

    savedop = NODE_OP(NODE_LCHILD(bn)); //save operator in case of tree rewrite
    if (savedop == OP_ident) {
        savedeval = (NODE_LCHILD(bn))->v[0];
    }
    pv = &NODE_LCHILD (bn)->v[0];
    pbName = pExStr + EVAL_ITOK (pv);
    len = EVAL_CBTOK (pv);
    if (bnOp != 0) {
        if ((ClassExp != T_NOTYPE) || (ClassImp != T_NOTYPE)) {
            if (ClassExp != T_NOTYPE) {
                // search an explicit class
                CurClass = ClassExp;
            }
            else if (ClassImp != T_NOTYPE) {
                CurClass = ClassImp;
            }

            // check to see if the left operand is the same class as the current
            // explicit or implicit class

            if ((hBase = THGetTypeFromIndex (pCxt->hMod, CurClass)) == 0) {
                pExState->err_num = ERR_BADOMF;
                return (FALSE);
            }
            pField = (uchar *)(&((TYPPTR)MHOmfLock (hBase))->leaf);
            tSkip = offsetof (lfClass, data);
            RNumLeaf (pField + tSkip, &tSkip);
            pc = (char *)pField + tSkip;
            if (len == (ulong)*pc) {
                if (pExState->state.fCase == TRUE) {
                   cmpflag = _tcsncmp (pbName, pc + 1, len);
                }
                else {
                    cmpflag = _tcsnicmp (pbName, pc + 1, len);
                }
            }
            MHOmfUnLock (hBase);
            if (cmpflag == 0) {
                if (pvThisFromST (bnOp) == FALSE) {
                    return (FALSE);
                }
                PushStack (ST);
                EVAL_STATE (ST) = EV_type;
                goto found;
            }
        }
        else {
            fGlobal = TRUE;
        }
    }

    // Use a global flag (BindingScopeOperand) to notify
    // BindSymbol that we are binding the left operand of ::
    // and that it should limit its search to class names only.
    // This prevents erroneous binding of the class at the
    // left of :: to a constructor that has the same name
    oldBindingScopeOp = BindingScopeOperand;
    BindingScopeOperand = TRUE;
    retval = BindLChild (bn);
    BindingScopeOperand = oldBindingScopeOp;

    if (retval == FALSE) {
        if (fGlobal == FALSE) {
            // we searched an explicit or implicit class scope and did
            // not find the left operand. we now must search outwards
            // and find only global symbols

            oldClassImp = ClassImp;
            ClassImp = T_NOTYPE;
            InitSearchSym (NODE_LCHILD (bn), &(NODE_LCHILD (bn)->v[0]), &Name,
              T_NOTYPE, SCP_module | SCP_global, CLS_defn);
            sRet = SearchSym (&Name);
            ClassImp = oldClassImp;
            switch (sRet) {
                case HR_found:
                    // The symbol was in global scope and pushed onto
                    // the stack
                    fGlobal = TRUE;
                    break;


                default: {
                    bnode_t bnL     = NODE_LCHILD(bn);
                    bnode_t bnR     = NODE_RCHILD(bn);
                    peval_t pvLeft  = &bnL->v[0];
                    peval_t pvRight = &bnR->v[0];
                    peval_t pvCur   = &bn->v[0];

                    // try this only at the outermost level of ::'s
                    if (!BindingScopeOperand) {
                        len = EVAL_CBTOK(pvLeft);
                        EVAL_CBTOK(pvLeft) = EVAL_ITOK(pvRight) - EVAL_ITOK(pvLeft) + EVAL_CBTOK(pvRight);

                        retval = BindSymbol(bnL);

                        EVAL_CBTOK(pvLeft) = (BYTE)len;

                        if (retval) {
                            CLASS_NAMESPACE(pvLeft) = TRUE;
                            bn->v[0] = *ST;
                            bnR->v[0] = *ST;
                        }
                        return (retval);
                    }
                    else {
                        // otherwise just propogate the left starting
                        // position for a higher level scope resolution
                        // via the above code [rm]

                        EVAL_ITOK(pvCur) = EVAL_ITOK(pvLeft);
                    }
                    return (FALSE);
                }
            }
        }
        else {
            // we did not find the symbol at global scope
            return (FALSE);
        }
    }
    else {

        // A simple tree rewrite of "C::foo" to "this->C::foo" produces
        // the wrong parse tree if node C is replaced by the expression
        // "this->C" (which is the case after a simple tree rewrite).
        // The reason for this is that '::' has higher priority than '->'.
        // The code below reorganizes the tree if such a situation
        // occurs. --caviar #932,2571

        static bool_t rewriteInProgress = FALSE; //flag for preventing recursion

        if (rewriteInProgress == FALSE &&
            savedop == OP_ident &&
            NODE_OP(NODE_LCHILD(bn)) == OP_pointsto &&
            NODE_OP(NODE_LCHILD(NODE_LCHILD(bn))) == OP_this) {

            // tree has been rewritten but needs to be reorganized in
            // order to restore proper operator priority

            eval_t tmp;
            CV_typ_t typtmp;
            bnode_t bL,bR,bLR;
            tmp = bn->v[0];
            typtmp = NODE_STYPE(bn);
            bL = NODE_LCHILD(bn);
            bR = NODE_RCHILD(bn);

            rewriteInProgress = TRUE;

            NODE_OP(bn) = OP_pointsto;
            NODE_STYPE(bn) = NODE_STYPE(bL);
            bn->v[0] = bL->v[0];

            NODE_OP(bL) = OP_bscope;
            NODE_STYPE(bL) = typtmp;
            bL->v[0] = tmp;

            NODE_LCHILD(bn) = NODE_LCHILD(bL);
            NODE_RCHILD(bn) = bL;

            bLR = NODE_RCHILD(bL);
            NODE_LCHILD(bL) = NODE_RCHILD(bL);
            NODE_RCHILD(bL) = bR;

            DASSERT(NODE_OP(bLR) == OP_ident);
            if (NODE_OP(bLR) == OP_ident) {
                bLR->v[0] = savedeval;
            }

            if (PopStack()) {
                retval = Bind(bn);
            }
            else {
                pExState->err_num = ERR_INTERNAL;
                retval = FALSE;
            }
            rewriteInProgress = FALSE;
            return retval;
        }
        fGlobal = TRUE;
    }
found:
    if (fGlobal == TRUE) {
        // flag the fact that the left operand was not a nested type
        pv = &NODE_LCHILD (bn)->v[0];
        CLASS_GLOBALTYPE (pv) = TRUE;
        EVAL_IS_MEMBER (&bn->v[0]) = TRUE;
        if (bnOp != 0) {
            // Dolphin #5503:
            // flag the fact that we found the global type while
            // binding the right subtree of a bnOp, in order to avoid
            // pushing an extra node on the stack during evaluation
            CLASS_FOLLOWSBNOP (pv) = TRUE;
        }
    }
    if ((EVAL_STATE (ST) != EV_type) || (!EVAL_IS_CLASS (ST))) {
        pExState->err_num = ERR_BSCOPE;
        return (FALSE);
    }
    oldClassExp = ClassExp;
    ClassExp = EVAL_TYP (ST);
    oldbnOp = bnOp;
    bnOp = bn;
    if ((retval = BindRChild (bn)) == FALSE) {
        return (FALSE);
    }
    ClassExp = oldClassExp;
    bnOp = oldbnOp;
    if (retval == TRUE) {
        if ((fGlobal == TRUE) &&
          (bnOp == 0) &&
          (EVAL_IS_METHOD (ST) == FALSE) &&
          (EVAL_IS_STMEMBER (ST) == FALSE) &&
          // in case we get a nested enumerate
          // sps m9/15/92
          (EVAL_STATE(ST) != EV_constant)) {
            pExState->err_num = ERR_NOTSTATIC;
            return (FALSE);
        }
        if ((EVAL_IS_METHOD (ST) == TRUE) && (FCN_NOTPRESENT (ST) == TRUE)) {
            pExState->err_num = ERR_METHODNP;
            return (FALSE);
        }
        *STP = *ST;
        return (PopStack ());
    }
    return (retval);
}




/***    BindByteOps - Handle 'by', 'wo' and 'dw' operators
 *
 *      fSuccess = BindByteOps (op)
 *
 *      Entry   op = operator (OP_by, OP_wo or OP_dw)
 *
 *      Exit
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 *      Description
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
 * NOTES
 */


bool_t FASTCALL
BindByteOps (
    bnode_t bn
    )
{
    op_t        op;
    CV_typ_t    type;

    if (!BindLChild (bn)) {
        return (FALSE);
    }

    // Resolve identifiers and do type checking.

    if (!ValidateNodes (op = NODE_OP (bn), ST, NULL)) {
        return(FALSE);
    }

    // If the operand is an lvalue and it is a register,
    // load the value of the register.  If the operand is an
    // lvalue and is not a register, use the address of the variable.
    //
    // If the operand is not an lvalue, use its value as is.

    if (EVAL_STATE (ST) == EV_lvalue) {
        // if the value is a register, the code below will set up a pointer
        // to the correct type and then dereference it.  The evaluation phase
        // will have to actually generate the pointer.

        if (!EVAL_IS_REG (ST)) {
            if (AddrOf (bn) == FALSE) {
                return (FALSE);
            }
        }
    }
    else if (!EVAL_IS_PTR (ST)) {
        type = pExState->state.f32bit ? T_32PFUCHAR : T_PFUCHAR;
        if (CastNode (ST, type, type) == FALSE) {
            pExState->err_num = ERR_OPERANDTYPES;
            return (FALSE);
        }
    }

    // Now cast the node to (char far *), (int far *) or
    // (long far *).  If the type is char, uchar, short
    // or ushort, we want to first cast to (char *) so
    // that we properly DS-extend (casting (int)8 to (char
    // far *) will give the result 0:8).

    type = EVAL_TYP (ST);

    //DASSERT(CV_IS_PRIMITIVE (typ));

    if (CV_IS_PRIMITIVE (type) &&
    (CV_TYP_IS_REAL (type) || CV_TYP_IS_COMPLEX (type)) ) {
        pExState->err_num = ERR_OPERANDTYPES;
        return (FALSE);
    }
    if (op == OP_by) {
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
    return (Fetch ());
}




/**     BindCast - bind a cast
 *
 *      fSuccess = BindCast (bn)
 *
 *      Entry   bn = based pointer to cast node
 *
 *      Returns TRUE if bind successful
 *              FALSE if bind error
 */


bool_t FASTCALL
BindCast (
    bnode_t bn
    )
{
    peval_t     pv;
    bnode_t     bnLeft;
    bool_t      fIllegalCast = FALSE;

    // Bind right node which is the value

    if (!BindRChild (bn)) {
        return (FALSE);
    }
    bnLeft = NODE_LCHILD (bn);

    // Check for casting a class to anything, not having a typestring or
    // the typestring containing an error

    if (EVAL_IS_CLASS (ST) ||
      (NODE_OP ((pnode_t)bnLeft) != OP_typestr) ||
      !ParseType (bnLeft, FALSE)) {
        pExState->err_num = ERR_TYPECAST;
        return (FALSE);
    }
    if (EVAL_IS_BITF (ST)) {
        // change the type of the node to the underlying type
        EVAL_TYP (ST) = BITF_UTYPE (ST);
    }

    //propagate type information to parent node. this is useful for
    //function evaluation with cast string literals as arguments --gdp 9/17/92
    *(peval_t)&bn->v[0] = *(peval_t)&bnLeft->v[0];

    // copy the base type node up to the cast node and then try to find a
    // way to cast the stack to to the base type

    /*
    ** CastPtrToPtr can reallocate the expression tree so we get the left child
    ** and pv for it once again after potentially calling CastPtrToPtr()
    */
    if ( EVAL_IS_PTR (ST) &&
        ( CastPtrToPtr ( bn ) || CastBaseToDeriv(bn, &fIllegalCast) ) ) {

        bnLeft = NODE_LCHILD (bn);
        pv = (peval_t)&bnLeft->v[0];

        if (fIllegalCast) {
            pExState->err_num = ERR_TYPECAST;
            return FALSE;
        }

        if ( !CV_TYP_IS_PTR ( EVAL_TYP ( pv ) ) &&
            CV_IS_INTERNAL_PTR ( EVAL_TYP ( pv ) )
        ) {
            // the desired type is a base class so we can just set the node type.
            // the value portion of bn contains the data to cast right to left

            return (SetNodeType (ST, EVAL_TYP (pv)));
        }
    }
    else {
        bnLeft = NODE_LCHILD (bn);
        pv = (peval_t)&bnLeft->v[0];
    }

    // the to which we cast may reside in a different module
    // so we'll have to update EVAL_MOD(ST) before calling
    // SetNodeType

    EVAL_MOD(ST) = EVAL_MOD(pv);

    if (EVAL_IS_PTR (pv)) {
        return (CastNode (ST, EVAL_TYP (pv), PTR_UTYPE (pv)));
    }
    else {
        return (CastNode (ST, EVAL_TYP (pv), EVAL_TYP (pv)));
    }

}





/***    CastPtrToPtr - cast a pointer to derived to a pointer to base
 *
 *      fSuccess = CastPtrToPtr  (bn)
 *
 *      Entry   bn = based pointer to cast node
 *              ST = value to cast
 *
 *      Exit    value portion of node changed to member to indicate cast data
 *
 *      Returns TRUE if possible to cast derived to base
 *              FALSE if not
 */


bool_t
CastPtrToPtr (
    bnode_t bn
    )
{
    static eval_t      evalD;
    static eval_t      evalB;
    peval_t     pvD = &evalD;
    peval_t     pvB = &evalB;
    search_t    Name;
    CV_typ_t    typD;
    CV_typ_t    typB;

    *pvD = *ST;
    *pvB = *((peval_t)&NODE_LCHILD (bn)->v[0]);
    if ((SetNodeType (pvD, PTR_UTYPE (pvD)) == FALSE) ||
      (SetNodeType (pvB, PTR_UTYPE (pvB)) == FALSE) ||
      !EVAL_IS_CLASS (pvD) ||
      !EVAL_IS_CLASS (pvB)) {
        // we do not have pointers to classes on both sides
        return (FALSE);
    }

    // type indices may come from different contexts and should be
    // translated to corresponding types in the current .exe or .dll
    if ((typB = TranslateClassIndex( EVAL_TYP(pvB), EVAL_MOD(pvB) )) == 0 ||
        (typD = TranslateClassIndex( EVAL_TYP(pvD), EVAL_MOD(pvD) )) == 0) {
        return FALSE;
    }

    InitSearchBase (bn, typD, typB, &Name, pvB);
    switch (SearchSym (&Name)) {
        case HR_found:
            // remove the stack entry that was pushed by successful search
            return (PopStack ());

        case HR_rewrite:
            DASSERT (FALSE);

        default:
            break;
    }
    return (FALSE);
}

/***    CastBaseToDeriv - cast a pointer to base to a pointer to derived
 *
 *      fSuccess = CastBaseToDeriv (bn, pfIllegal)
 *
 *      Entry   bn = based pointer to cast node
 *              ST = value to cast
 *              pfIllegal = pointer to flag set by this function
 *
 *      Exit    value portion of node changed to member to indicate cast data
 *              *pfIllegal is TRUE if an illegal cast has been detected
 *                  (e.g., cast from a virtual base to a derived class)
 *              *pfIllegal is undefined if CastBaseToDeriv returns FALSE
 *
 *      Returns TRUE if the cast can be performed or if an illegal cast
 *                  has been detected.
 *              FALSE otherwise.
 *
 */

bool_t
CastBaseToDeriv (
    bnode_t bn,
    bool_t *pfIllegal
    )
{
    static eval_t      evalD;
    static eval_t      evalB;
    eval_t      evalSav;
    peval_t     pvD = &evalD;
    peval_t     pvB = &evalB;
    peval_t     pevalSav = &evalSav;
    search_t    Name;
    bnode_t     bnThis;
    bnode_t     bnT;
    peval_t     pvOp;
    CV_typ_t    typD;
    CV_typ_t    typB;

    *pfIllegal = FALSE;
    *pvB = *ST;
    *pvD = *((peval_t)&NODE_LCHILD (bn)->v[0]);
    if ((SetNodeType (pvD, PTR_UTYPE (pvD)) == FALSE) ||
      (SetNodeType (pvB, PTR_UTYPE (pvB)) == FALSE) ||
      !EVAL_IS_CLASS (pvD) ||
      !EVAL_IS_CLASS (pvB)) {
        // we do not have pointers to classes on both sides
        return (FALSE);
    }

    // type indices may come from different contexts and should be
    // translated to corresponding types in the current .exe or .dll
    if ((typB = TranslateClassIndex( EVAL_TYP(pvB), EVAL_MOD(pvB) )) == 0 ||
        (typD = TranslateClassIndex( EVAL_TYP(pvD), EVAL_MOD(pvD) )) == 0) {
        return FALSE;
    }

    *pevalSav = bn->v[0];
    InitSearchBase (bn, typD, typB, &Name, pvB);
    switch (SearchSym (&Name)) {
        case HR_found:
            // remove the stack entry that was pushed by successful search
            if (PopStack ())
                break;

            // else fall trhough
        case HR_rewrite:
            DASSERT (FALSE);

        default:
            bn->v[0] = *pevalSav;
            if (pExState->err_num == ERR_AMBIGUOUS) {
                // Illegal cast
                *pfIllegal = TRUE;
                return TRUE;
            }
            return FALSE;
    }

    // SearchSym has created a new subtree for computing and
    // adjusting the this pointer. If this tree contains only
    // constant adjustments for the this pointer, we can traverse
    // it and negate the corresponding offsets (since we cast from
    // base to derived, not from derived to base. Otherwise we do
    // not perform the cast.

    pvOp = &bn->v[0];
    if (!EVAL_IS_MEMBER(pvOp) ||
        (bnThis = (bnode_t)MEMBER_THISEXPR (pvOp)) == 0) {
        *pvOp = *pevalSav;
        return FALSE;
    }

    for(bnT = bnThis; bnT != 0; bnT = NODE_LCHILD(bnT)) {
        if (NODE_OP(bnT) != OP_thisconst &&
            NODE_OP(bnT) != OP_thisinit) {
            *pvOp = *pevalSav;
            // an illegal cast has been detected
            *pfIllegal = TRUE;
            return TRUE;
        }
    }

    // the subtree contains only OP_thisconst nodes
    // (besides the OP_thisinit node)
    // traverse the tree and negate disp offsets

    for(bnT = bnThis; bnT != 0; bnT = NODE_LCHILD(bnT)) {
        if (NODE_OP(bnT) == OP_thisconst) {
            adjust_t *pa;
            pa = (adjust_t *)(&bnT->v[0]);
            pa->disp = -(pa->disp);
        }
    }
    return TRUE;
}



/***    BindConst - bind constant
 *
 *      fSuccess = BindConst (bn)
 *
 *      Entry   bn = based pointer to tree node
 *
 *      Exit    ST = constant
 *
 *      Returns TRUE if bind successful
 *              FALSE if bind error
 *
 *
 */


bool_t FASTCALL
BindConst (
    bnode_t bn
    )
{
    peval_t     pv = &((pnode_t)bn)->v[0];

    // Set the type flags back into the node, copy
    // the flags and value into the evaluation stack and return
    // The handle to module is set so that a cast of a constant to
    // a user-defined type will work.

    EVAL_MOD (pv) = SHHMODFrompCXT(pCxt);

#ifdef NEVER
    // this has been disabled in order to handle overloaded
    // operator function calls. E.g., fooobj << "abcd" may
    // imply a function call fooobj.operator<<("abcd"). At
    // the time "abcd" is bound BindingFuncArgs is false.
    // --gdp 9/17/92  (related to caviar #919)

    if (BindingFuncArgs == FALSE && EVAL_TYP (pv) == T_PRCHAR) {
        // we are binding a string constant (ie. "foobar") but
        // not for function args... this is not allowed, since
        // we can never return a correct address to the string
        // that we pushed on the stack

        pExState->err_num = ERR_NOTEVALUATABLE;
        return (FALSE);
    }
#endif

    if (SetNodeType (pv, EVAL_TYP (pv)) == TRUE) {
        EVAL_STATE (pv) = EV_constant;
        return (PushStack (pv));
    }
    else {
        return (FALSE);
    }
}


/******************************************************************************

contextDefault and ContextHelper are common code which has been reorganized to
simplify support for the ! context operator.

******************************************************************************/
contextDefault(
    bnode_t     bn,
    PCXF        nCxf,
    bool_t      fNoCxf,
    bool_t      fUsepCXTforBP
    )
{
    bool_t      BindStatus;
    PCXT        oldCxt;
    bnode_t     oldbnCxt;
    CV_typ_t    oldClassImp;
    long        oldThisAdjust;
    bool_t      fNoFuncCxfSav;
    // save old context and implicit class and set new ones

    oldCxt = pCxt;
    oldbnCxt = bnCxt;
    oldClassImp = ClassImp;
    oldThisAdjust = ClassThisAdjust;
    pCxt = SHpCXTFrompCXF (nCxf);
    if (fUsepCXTforBP) {
        pBindBPCxt = pCxt;
    }
    bnCxt = bn;
    ClassImp = SetImpClass (pCxt, &ClassThisAdjust);
    fNoFuncCxfSav = fNoFuncCxf;
    fNoFuncCxf = fNoCxf;

    BindStatus = BindLChild (bn);

    fNoFuncCxf = fNoFuncCxfSav;
    pBindBPCxt = NULL;

    // if the result of the expression is bp relative, then we must
    // load the value before returning to the original context

    if ((BindStatus == TRUE) && (EVAL_STATE (ST) == EV_lvalue) &&
      EVAL_IS_BPREL (ST) &&
      // if the new context is the same as the old context, we can
      // still treat bp relatives as l-values. It should be sufficient
      // to compare only hProc and hMod (since the explicit context
      // cannot contain a block
     !(SHHMODFrompCXT(pCxt) == SHHMODFrompCXT(oldCxt) &&
       SHHPROCFrompCXT(pCxt) == SHHPROCFrompCXT(oldCxt))
        ) {
        if (EVAL_IS_REF (ST)) {
            if (!Fetch ()) {
                pExState->err_num = ERR_BADCONTEXT;
                return FALSE;
            }
            EVAL_IS_REF (ST) = FALSE;
        }
        EVAL_STATE (ST) = EV_rvalue;
    }

    // restore previous context and implicit class

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
    ClassImp = oldClassImp;
    ClassThisAdjust = oldThisAdjust;
    return BindStatus;
}



bool_t
FASTCALL
CxtHelper(
    bnode_t     bn,
    HMOD        hMod,
    HSF         hsf,
    int         cMod,
    int         cProc,
    char *      pProc
    )
{
    eval_t      evalT = {0};
    PCXF        nCxf;
    peval_t     pvT;
    PCXT        oldCxt;
    search_t    Name;
    bnode_t     oldAmb;
    bool_t      oldBindingBP;
    HR_t        retval;
    bool_t      fNoCxf = FALSE;

    //
    // initialize the context packet in the node to have the same contents
    // as the current context.  We will then set new fields in the order
    // exe, module, proc.
    //

    nCxf = (PCXF)&((pnode_t)bn)->v[0];
    *SHpCXTFrompCXF (nCxf) = *pCxt;
    SHpCXTFrompCXF(nCxf)->hProc = 0;
    SHpCXTFrompCXF(nCxf)->hBlk = 0;
    SHhFrameFrompCXF (nCxf) = pExState->hframe;

    //
    // set new context from handle to module

    //

    if (!SHGetCxtFromHmod ( hMod, SHpCXTFrompCXF (nCxf))) {
        SHGetCxtFromHexe (SHHexeFromHmod( hMod ), SHpCXTFrompCXF (nCxf));
    }

    if (cMod > 0) {
        DWORD           rgLn[2];
        ADDR            addr;
        SHOFF           cbLn;

        /*
         * If we have a source file, then we want to get the address
         *      of the first line in the source file.  This is obtained
         *      by trying to get the address of line 1, and if does not
         *      exist then get the address of the first line after
         *      line 1 in the file.
         */

        if(SLFLineToAddr (hsf,1,&addr,&cbLn,rgLn) ||
           SLFLineToAddr (hsf,rgLn[1],&addr,&cbLn,NULL)) {
                SHSetCxt (&addr, SHpCXTFrompCXF (nCxf));
        }
    }

    if (cProc <= 0) {

        SHpCXTFrompCXF(nCxf)->hProc = 0;
        SHpCXTFrompCXF(nCxf)->hBlk = 0;

    } else {
        // a proc was specified, initialize the context and search for
        // the proc within the current context.  Note that if a proc was
        // not specified, the hProc and hBlk in nCxf are null.

        //M00SYMBOL - doesn't allow for T::foo() as proc

        oldCxt = pCxt;
        pCxt = SHpCXTFrompCXF (nCxf);
        pvT = &evalT;
        EVAL_ITOK (pvT) = (ULONG)(pProc - pExStr);
        EVAL_CBTOK (pvT) = (uchar)cProc;

        // do not allow ambiguous symbols during context symbol searching

        oldAmb = pExState->ambiguous;
        pExState->ambiguous = 0;
        oldBindingBP = BindingBP;
        BindingBP = FALSE;
        InitSearchSym (bn, pvT, &Name, 0,
          SCP_lexical | SCP_module | SCP_global, CLS_method);
        retval = SearchSym (&Name);
        BindingBP = oldBindingBP;
        if (pExState->ambiguous != 0) {
            pExState->err_num = ERR_AMBCONTEXT;
            return FALSE;
        }
        pExState->ambiguous = oldAmb;
        switch (retval) {
            case HR_found:
                //
                // if the symbol was found, it was pushed onto the stack
                //

                PopStack ();
                if (EVAL_IS_FCN (pvT)) {
                    break;
                }

                    // name is not a procedure reachable from
                    // the specified context

            default:
                goto contextbad;
        }

        // attempt to set the context to the specified instance of the function.
        // if the attempt fails, then set the context to the address of the
        // function

        if (SHGetFuncCxf (&pvT->addr, nCxf) == NULL) {
            fNoCxf = TRUE;
            if (SHSetCxt (&pvT->addr, SHpCXTFrompCXF (nCxf)) == NULL) {
                goto contextbad;
            }
        }
        pCxt = oldCxt;
        if (SHHPROCFrompCXT (SHpCXTFrompCXF (nCxf)) == 0) {
            goto contextbad;
        }
    }

    return contextDefault(bn, nCxf, (BindingBP && (cMod > 0)), fNoCxf );

contextbad:
    pExState->err_num = ERR_BADCONTEXT;
    return FALSE;
}


/**     BindContext - bind context operator
 *
 *      fSuccess = BindContext (bn)
 *
 *      Entry   bn = based pointer to context operator node
 *                  (token pointers point to {...} context string
 *              pCxt = pointer to current context packet
 *              bnCxt = based pointer to node containing current context
 *
 *      Exit    value portion of node is bound context
 *
 *      Returns TRUE if context parsed and bound without error
 *              FALSE if error
 *
 *      Note    context operator has the form
 *              {[number] (proc)[,[(module)][,[(exe)]]]}
 *              where number the base 10 instance of proc on the stack
 *                  n > 0 means count from top of stack down
 *                  n < 0 means count from current stack pointer up
 *                  n = 0 means take first instance up (will find current proc)
 *              proc is the proc name (if overloaded then argument types must
 *                  specifed to disambiguate
 *              module is the starting module name in the exe for search
 *              exe is the .exe or .dll name to search
 *              The () around proc, module and exe are optional and are required
 *              only if the string has commas not enclosed in parenthesis
 */
char * szDflCxtMarker = "{*}";

bool_t FASTCALL
BindContext (
    bnode_t bn
    )
{
    int         instance = 0;
    bool_t      isnegative = FALSE;
    char       *pProc;
    int         cProc;
    char       *pMod;
    int         cMod;
    char       *pExe;
    int         cExe;
    char       *pb;
    HEXE        hExe = 0;
    HMOD        hMod = 0;
    char        savedChar;

    pb = pExStr + EVAL_ITOK (&((pnode_t)bn)->v[0]);

    if (!_tcsncmp(pb, szDflCxtMarker, _tcslen(szDflCxtMarker))) {
        // use the context specified in pExState
        PCXF nCxf = (PCXF)&((pnode_t)bn)->v[0];
        *SHpCXTFrompCXF (nCxf) = pExState->cxt;
        SHhFrameFrompCXF (nCxf) = pExState->hframe;
        return contextDefault(bn, nCxf, FALSE, FALSE);
    }

    if (*pb++ != '{') {
        goto contextbad;
    }

    // skip white space and process instance specification of instance number
    // where the number is base 10 and can be signed

    while ((*pb == ' ') || (*pb == '\t')) {
        pb++;
    }
    if (*pb == '-') {
        isnegative = TRUE;
        pb++;
    }
    else if (*pb == '+') {
        pb++;
    }
    while (_istdigit ((_TUCHAR)*pb)) {
        instance = instance * 10 + (*pb++ - '0');
    }
    if (isnegative) {
        instance = -instance;
    }

    // set the pointer to the procedure and skip to a comma that is not
    // enclosed in parenthesis

    if (!ContextToken (&pb, &pProc, &cProc) ||
        !ContextToken (&pb, &pMod, &cMod) ||
        !ContextToken (&pb, &pExe, &cExe)) {
        goto contextbad;
    }

    if ((cProc == -1) && (cMod == -1) && (cExe == -1)) {
        // the null context {} forces a rebind
        // this is not yet supported by the kernel so I am making this
        // an error to reserve the meaning for future versions
        goto contextbad;
    }


    // process exe name

    if (cExe > 0) {
        //
        // find the exe handle if {...,exe} was specified
        //

        savedChar = *(pExe + cExe);
        *(pExe + cExe) = 0;
        hExe = SHGethExeFromName (pExe);
        *(pExe + cExe) = savedChar;
        if (hExe == 0) {
            goto contextbad;
        }

        //
        // if an exe is specified, then set module to first module in exe
        //
        if ((hMod = SHGetNextMod (hExe, hMod)) == 0) {
            // error in context
            goto contextbad;
        }
    }
    else if (cExe == -1) {
        //
        // {proc,mod} or {proc} was specified so set exe to current
        // module or first module
        //

        if ((hMod = SHHMODFrompCXT (pCxt)) == 0) {
            // Can't call SHGetNextMod(hExe, hMod) at this point,
            // since hExe is 0
                goto contextbad;
        }
        if ((hExe = SHHexeFromHmod (hMod)) == 0) {
            // error in context
            goto contextbad;
        }
    }
    else {
        // it is not possible to specifiy an exe by {,,,}
        goto contextbad;
    }

    // process module specification.  At this point we have the handle to the
    // exe and either the handle to the first module or the handle to the
    // current module

    if (cMod <= 0) {

        return CxtHelper(bn, hMod, 0, cMod, cProc, pProc);

    } else {
        HMOD    hModTemp;
        HSF     hsfTemp;

        // find the module handle if {...,mod...} was specified
        savedChar = *(pMod + cMod);
        *(pMod + cMod) = 0;
        hMod = hModTemp = (HMOD) NULL;
        while (hModTemp = SHGetNextMod (hExe, hModTemp)) {
            if (hsfTemp = SLHsfFromFile (hModTemp, pMod)) {
                if (CxtHelper(bn, hModTemp, hsfTemp, cMod, cProc, pProc)) {
                    *(pMod + cMod) = savedChar;
                    return TRUE;
                }
            }
        }
        *(pMod + cMod) = savedChar;
    }

contextbad:
    if (pExState->err_num == 0) {
        pExState->err_num = ERR_BADCONTEXT;
    }
    return FALSE;
}


bool_t
FASTCALL
BindExeContext(
    bnode_t bn
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    LSZ         pb;
    LSZ         pb1;
    char        ch;
    HEXE        hExe = 0;
    HMOD        hMod = 0;

    pb = pExStr + EVAL_ITOK (&((pnode_t)bn)->v[0]);

    /*
     * skip white space
     */

    while ((*pb == ' ') || (*pb == '\t')) {
        pb++;
    }
    for (pb1=pb; *pb1 && *pb1 != '!'; ) {
        pb1++;
    }
    ch = *pb1;
    *pb1 = '\0';
    hExe = SHGethExeFromModuleName (pb);
    if (!hExe) {
        hExe = SHGethExeFromName (pb);
    }
    *pb1 = ch;

    if (hExe) {
        hMod = SHGetNextMod (hExe, hMod);
        return CxtHelper(bn, hMod, 0, 0, 0, 0);
    } else {
        pExState->err_num = ERR_BADCONTEXT;
        return FALSE;
    }
}





/***    BindDMember - Perform a dot member access (.*)
 *
 *      fSuccess = BindDMember (bnRight)
 *
 *      Entry   bnRight = based pointer to right operand node
 *
 *      Exit
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 */

bool_t FASTCALL
BindDMember (
    bnode_t bn
    )
{
    bool_t      retval;
    CV_typ_t    oldClassExp;

    pExState->err_num = ERR_OPNOTSUPP;
    return (FALSE);

    if (!BindLChild (bn)) {
        return (FALSE);
    }
    if (EVAL_STATE (ST) != EV_lvalue) {
        pExState->err_num = ERR_NEEDLVALUE;
        return (FALSE);
    }
    if (EVAL_IS_REF (ST)) {
        if (!Fetch ()) {
            return (FALSE);
        }
        EVAL_IS_REF (ST) = FALSE;
    }
    oldClassExp = ClassExp;
    ClassExp = EVAL_TYP (ST);
    retval = BindRChild (bn);
    ClassExp = oldClassExp;
    if (retval == TRUE) {
        // move element descriptor to previous stack entry and pop stack
        *STP = *ST;
        PopStack ();
        NOTTESTED (FALSE);
        // M00SYMBOL - need to check that the stack top is a pointer to member
    }
    return (FALSE);

}

/***    BindDot - Perform the dot (.) operation
 *
 *      fSuccess = BindDot (bn)
 *
 *      Entry   pn = pointer to tree node
 *              bnOp = based pointer to operand node
 *
 *      Exit    NODE_STYPE (bn) = type of stack top
 *
 *      Returns TRUE if bind successful
 *              FALSE if bind error
 *
 *      Exit    pExState->err_num = error ordinal if bind error
 *
 */


bool_t FASTCALL
BindDot (
    bnode_t bn
    )
{
    bool_t      retval;
    CV_typ_t    oldClassExp;
    ulong       state;
    bnode_t     oldbnOp;
    bool_t      fFcnOnly = FALSE;

    if (!BindLChild (bn)) {
        if (pExState->err_num == ERR_NOSTACKFRAME &&
            OP_IS_IDENT (NODE_OP (NODE_RCHILD (bn)))) {
            // dolphin #2433:
            // cannot bind the symbol because it
            // is not in context yet. We can however
            // allow expressions of the form foo.bar
            // where bar is a (non virtual) method.
            // So retry binding by enabling prolog:
            DASSERT (pExState->state.fEProlog == FALSE);
            pExState->state.fEProlog = TRUE;
            retval = BindLChild(bn);
            pExState->state.fEProlog = FALSE;
            if (retval == TRUE) {
                // allow only member functions
                fFcnOnly = TRUE;
            }
            else {
                return FALSE;
            }
        }
        else {
            return (FALSE);
        }
    }
    if (EVAL_IS_REF (ST)) {
        if (!Fetch ()) {
            return (FALSE);
        }
        EVAL_IS_REF (ST) = FALSE;
    }
    if (!EVAL_IS_CLASS (ST)) {
        pExState->err_num = ERR_NEEDSTRUCT;
        return (FALSE);
    }
    oldClassExp = ClassExp;
    ClassExp = EVAL_TYP (ST);

    if ((NODE_OP (NODE_RCHILD (bn)) == OP_bscope) ||
      OP_IS_IDENT (NODE_OP (NODE_RCHILD (bn))))  {
#ifdef NEVER
        // disabled -- gdp 1/10/94
        // context propagation is no longer needed
        // as synthesized expressions preserve context scope

        // set the based node pointer for the operator
        // save/restore the CXT so we can apply any context
        // operator from the left side to the right side.
        bnode_t bnT = BnMatchOp ( NODE_LCHILD ( bn ), OP_context );
        PCXT    pcxtSav = pCxt;

        if ( bnT ) {
            pCxt = SHpCXTFrompCXF ( (PCXF) &( (pnode_t)bnT)->v[0] );
        }
#endif
        oldbnOp = bnOp;
        bnOp = bn;
        retval = BindRChild (bn);
        bnOp = oldbnOp;
#ifdef NEVER
        pCxt = pcxtSav;
#endif
    }
    else {
        pExState->err_num = ERR_SYNTAX;
        retval = FALSE;
    }
    ClassExp = oldClassExp;
    if (retval == TRUE) {
        if (fFcnOnly) {
            // allow nothing else but non-virtual methods
            if (! EVAL_IS_FCN(ST) ||
                FCN_PROPERTY (ST) == CV_MTvirtual ||
                FCN_PROPERTY (ST) == CV_MTintro) {
                return FALSE;
            }
        }
        // move element descriptor to previous stack entry and pop stack
        state = EVAL_STATE (STP);
        *STP = *ST;
        if (state == EV_type) {
            EVAL_STATE (STP) = EV_type;
        }
        else {
            EVAL_STATE (STP) = EV_lvalue;
        }
        return (PopStack ());
    }
    return (FALSE);
}

#ifdef NEVER

/***    BnMatchOp - do a search on a bnode_t tree in order
 *
 *      bnode = BnMatchOp ( bn, op_tMatchIt )
 *
 *      returns bnode_t of the top-left-most node that matches, else 0
 */

bnode_t FASTCALL
BnMatchOp (
    bnode_t bn,
    op_t opMatch
    )
{
    bnode_t bnT;
    // fast fail or match
    if ( bn == 0 || NODE_OP ( bn ) == opMatch ) {
        return bn;
    }

    // check left subtree, then right subtree
    bnT = BnMatchOp ( NODE_LCHILD ( bn ), opMatch );
    if ( !bnT ) {
        bnT = BnMatchOp ( NODE_RCHILD ( bn ), opMatch );
    }
    return bnT;
}

#endif



/***    BindFetch - Bind the fetch (*) operation
 *
 *      fSuccess = BindFetch (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if bind successful
 *              FALSE if bind error
 *
 *      Exit    pExState->err_num = error ordinal if bind error
 */


bool_t FASTCALL
BindFetch (
    bnode_t bn
    )
{
    if (!BindLChild (bn)) {
        return (FALSE);
    }
    return (Fetch ());
}




/**     BindFunction - bind a function call and arguments
 *
 *      fSuccess = BindFunction (bn)
 *
 *      Entry   bn = based pointer to function node
 *
 *      Returns TRUE if bind successful
 *              FALSE if bind error
 */


bool_t FASTCALL
BindFunction (
    bnode_t bn
    )
{
    belem_t     oldStackCkPoint = StackCkPoint;
    ulong       len;
    ulong       OpDot;
    ulong       Right;
    pnode_t     pn;

    CkPointStack ();
    if (Function (bn) == TRUE) {
        StackCkPoint = oldStackCkPoint;
        return (TRUE);
    }
    ResetStack ();
    StackCkPoint = oldStackCkPoint;
    if (pExState->err_num != ERR_CLASSNOTFCN) {
        return (FALSE);
    }

    // rewrite object (arglist) as object.operator() (arglist)

    OpDot = pTree->node_next;
    Right = OpDot + sizeof (node_t) + sizeof (eval_t);
    len = 2 * (sizeof (node_t) + sizeof (eval_t));
    if ((pTree->size - OpDot) < len) {
        if (!GrowETree (len)) {
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

    // set operator node to OP_dot

    pn = (pnode_t)((bnode_t)OpDot);
    memset (pn, 0, sizeof (node_t) + sizeof (eval_t));
    NODE_OP (pn) = OP_dot;
    NODE_LCHILD (pn) = NODE_LCHILD (bn);
    NODE_RCHILD (pn) = (bnode_t)Right;
    NODE_LCHILD (bn) = (bnode_t)OpDot;

    // insert OP_Ofunction node as right node

    pn = (pnode_t)((bnode_t)Right);
    memset (pn, 0, sizeof (node_t) + sizeof (eval_t));
    NODE_OP (pn) = OP_Ofunction;

    pTree->node_next += len;
    return (Function (bn));
}





/**     Functions are bound assuming the following conventions:
 *
 *      C calling sequence
 *          Arguments are pushed right to left.  Varargs are not cast.
 *          If the function is a method, the this pointer is pushed after
 *          all of the actuals.
 *
 *          Returns
 *              char                        al
 *              short                       ax
 *              long                        dx:ax
 *              near float                  4 bytes pointed to by ds:ax
 *              far float                   4 bytes pointed to by dx:ax
 *              near double                 8 bytes pointed to by ds:ax
 *              far double                  8 bytes pointed to by dx:ax
 *              long double                 numeric coprocessor st0
 *              struct()  1|2|4 bytes       dx:ax
 *              near struct() 3 & > 4 bytes bytes pointed to by ds:ax
 *              far struct()  3 & > 4 bytes bytes pointed to by dx:ax
 *              near pointer                ax
 *              far pointer                 dx:ax
 *
 *      Pascal calling sequence
 *          Arguments are pushed left to right.  If the return value is a
 *          primitive type larger than 4 bytes or is real or is any user
 *          defined type that is not an alias for a primitive type, then
 *          the caller must allocate space on the stack and push the SS
 *          offset of this space as a hidden argument after all of the
 *          of this hidden argument after all of the actual arguments
 *          have been pushed.  If the function is a method, then the this
 *          pointer is pushed as the last (hidden) argument.  There must
 *          be an exact match on the number and types of arguments (after
 *          conversion).
 *
 *          Returns
 *              char                    al
 *              short                   ax
 *              long                    dx:ax
 *              float                   4 bytes pointed to by hidden argument
 *              near double             8 bytes pointed to by hidden argument
 *              long double             10 bytes pointed to by hidden argument
 *              any UDT not primitive   bytes pointed to by hidden argument
 *              near pointer            ax
 *              far pointer             dx:ax
 *
 *      fastcall calling sequence
 *          Arguments are pushed left to right.  If the return value is a
 *          real type, the it is returned in the numeric coprocessor st0.
 *          If the return value is a user defined type that is not an alias
 *          for a primitive type, then the caller must allocate space on the
 *          stack  and push the last (hidden) argument as the SS offset of
 *          this space.  There must be an exact match on the number and types
 *          of arguments (after conversion).
 *
 *
 *          Returns
 *              char                    al
 *              short                   ax
 *              long                    dx:ax
 *              all real values         numeric coprocessor st0
 *              any UDT not primitive   bytes pointed to by hidden argument
 *              near pointer            ax
 *              far pointer             dx:ax
 */


bool_t FASTCALL
Function (
    bnode_t bn
    )
{
    bnode_t     bnT;
    pnode_t     pnT;
    pnode_t     pnRight;
    int         argc = 0;
    long        retsize;
    bnode_t     OldArgList;
    eval_t      evalF;
    peval_t     pvF;
    eval_t      evalRet;
    peval_t     pvRet;
    UOFFSET     SPOff = 0;
    bool_t      retval;
    pargd_t     pa;
    bnode_t     bnRight = NODE_RCHILD (bn);
    bool_t      argsAllTypes = FALSE;

    // Bind argument nodes until end of arguments reached and count arguments

    // set BindingFuncArgs to true; notifies anybody who cares that
    // we are binding arguments (currently only BindConst cares)

    BindingFuncArgs = TRUE;

    for (bnT = bnRight; NODE_OP ((pnode_t)bnT) != OP_endofargs; bnT = NODE_RCHILD ((pnode_t)bnT)) {
        if (NODE_OP ((pnode_t)bnT) != OP_arg) {
            // Dolphin #9660:
            // The parser has failed to catch a syntax error
            pExState->err_num = ERR_SYNTAX;
            return (FALSE);
        }
        argc++;
        if (!BindLChild (bnT)) {
            return (FALSE);
        }
        else {
            if (EVAL_STATE (ST) == EV_type) {
                pnT = (pnode_t)bnT;
                pa = (pargd_t)&(pnT->v[0]);
                pa->actual = EVAL_TYP (ST);
                pa->flags.isconst = EVAL_IS_CONST (ST);
                pa->flags.isvolatile = EVAL_IS_VOLATILE (ST);
                // tell MatchArgs that the argument is a type and
                // that exact match is required.
                pa->flags.istype = TRUE;
                if ((argc > 1) && (!argsAllTypes)) {
                    // either all the args are types or expressions
                    // ow. we got an error
                    // sps - 2/21/92

                    return(FALSE);
                }
                argsAllTypes = TRUE;
            }
            else {
                pnT = (pnode_t)bnT;
                pa = (pargd_t)&(pnT->v[0]);
                pa->actual = EVAL_TYP (ST);
                if ((argc > 1) && (argsAllTypes)) {
                    // either all the args are types or expressions
                    // ow. we got an error
                    // sps - 2/21/92

                    return(FALSE);
                }
                argsAllTypes= FALSE;
            }
        }
    }
    // reset BindingFuncArgs

    BindingFuncArgs = FALSE;

    // set the argument list address for overload resolution
    // This is recursive because there can be function calls on the
    // left hand side of the function tree

    OldArgList = bArgList;
    bArgList = bnRight;

    // the left child must resolve to a function address

    // M00SYMBOL - need to make sure symbol search returns method address
    // M00SYMBOL - or vtable info

    retval = BindLChild (bn);
    bArgList = OldArgList;
    if (retval == FALSE) {
        return (FALSE);
    }
    pExState->state.fFunction = TRUE;
    if (EVAL_STATE (ST) == EV_type) {
        if (EVAL_IS_FCN (ST)) {
            return (TRUE);
        }

        // the function name resolved to a type.  we now look for the
        // name as apredefined type or a UDT and attempt to cast the
        // argument toargument to that type.  If the cast could be
        // performed, the tree was rewrittento an OP_cast

        if (FCN_NOTPRESENT (ST) == TRUE) {
            return (TRUE);
        }
        if (argc == 1) {
            return (FcnCast (bn));
        }

        // we must have at least one argument for a casting function

        pExState->err_num = ERR_ARGLIST;
        return (FALSE);
    }
    if (EVAL_IS_AMBIGUOUS (ST)) {
        pExState->err_num = ERR_AMBIGUOUS;
        return (FALSE);
    }
    if (FCN_NOTPRESENT (ST) == TRUE) {
        pExState->err_num = ERR_METHODNP;
        return (FALSE);
    }
    if (EVAL_IS_PTR (ST)) {
        Fetch ();
    }
    if (EVAL_IS_CLASS (ST)) {
        pExState->err_num = ERR_CLASSNOTFCN;
        return (FALSE);
    }
    if ((EVAL_STATE (ST) != EV_lvalue) || !EVAL_IS_FCN (ST)) {
        pExState->err_num = ERR_SYNTAX;
        return (FALSE);
    }

    // look at argsAllType - if all the args were types then we are simply
    // dealing with function prototype expression and not a function call
    // simply return true and let the evaluator determine the actual
    // address.  this is needed for virtual functions.

    if (argsAllTypes) {
        return (TRUE);
    }

    // the stack top is the function address node.  We save this information.
    // The stack now contains the arguments left to right plus the function
    // node.

    pvF = &evalF;
    *pvF = *ST;

    // do the user's stack setup.  On return, the OP_arg nodes will contain
    // the type of the argument and the address field will contain the offset
    // of the argument relative to the user's SP.  If the argument type is
    // zero, then the argument is a vararg and will be pushed uncasted onto
    // user's stack.

    pnRight = (pnode_t)bnRight;
    switch (FCN_CALL (pvF)) {
        case FCN_C:
        case FCN_STD:
        case FCN_THISCALL:
            retval = PushCArgs (pvF, pnRight, &SPOff, 0, ST);
            break;

        case FCN_PASCAL:
            if (argc != FCN_PCOUNT (pvF)) {
                retval = FALSE;
            }
            else {
                retval = PushPArgs (pvF, pnRight, &SPOff, ST);
            }
            break;

        case FCN_FAST:
            if (argc != FCN_PCOUNT (pvF)) {
                retval = FALSE;
            }
            else {
                retval = PushFArgs (pvF, pnRight, &SPOff, ST);
            }
            break;

        case FCN_MIPS:
            retval = PushMArgs (pvF, pnRight, &SPOff, ST);
            break;

        case FCN_ALPHA:
            retval = PushAArgs (pvF, pnRight, &SPOff, ST);
            break;

        case FCN_PPC:
            retval = PushAArgs (pvF, pnRight, &SPOff, ST);
            break;

        case FCN_IA64:
            retval = PushIA64Args (pvF, pnRight, &SPOff, ST);
            break;

        default:
            pExState->err_num = ERR_CALLSEQ;
            return (FALSE);
    }

    if (retval == FALSE) {
        pExState->err_num = ERR_FCNERROR;
        return (FALSE);
    }

    // We pop function node and the actual arguments

    for (; argc >= 0; argc--) {
        if (!PopStack ()) {
            DASSERT (FALSE);
            pExState->err_num = ERR_INTERNAL;
            return (FALSE);
        }
    }

    // push the type of the return value from the function.  If the return
    // type is void, then later attempts to use the value will cause an error

    pvRet = &evalRet;
    *pvRet = *pvF;
    SetNodeType (pvRet, FCN_RETURN (pvF));
    if (CV_IS_PRIMITIVE (EVAL_TYP(pvRet)) && CV_TYP_IS_COMPLEX (EVAL_TYP(pvRet)) ) {
        pExState->err_num = ERR_TYPESUPPORT;
        return(FALSE);
    }
    if ((retsize = TypeSize (pvRet)) > MAXRETURN) {
        pExState->err_num = ERR_FCNERROR;
        return (FALSE);
    }
    EVAL_VALLEN (pvRet) = (ulong )retsize;

    // According to C++ "a function call is an lvalue only
    // if the result type is a reference". In that case
    // the result node is converted to the lvalue of the
    // referenced object. By doing so, it is possible to
    // bind child expressions (such as base classes), that
    // need to get the address of the referenced object .
    if (EVAL_IS_REF (pvRet)) {
        EVAL_SYM (pvRet) = EVAL_PTR (pvRet);
        RemoveIndir (pvRet);
        EVAL_STATE (pvRet) = EV_lvalue;
    }
    else {
        EVAL_STATE (pvRet) = EV_rvalue;
    }

    if (!PushStack (pvRet)) {
        return (FALSE);
    }
    else {
        return (TRUE);
    }

    //M00KLUDGE - must handle function style casts to udt's here
}




/***    BindPlusMinus - bind binary plus or minus
 *
 *      fSuccess = BindPlusMinus (bn)
 *
 *      Entry   bn = based pointer to tree node
 *
 *      Exit    ST = STP +- ST
 *
 *      Returns TRUE if bind successful
 *              FALSE if bind error
 *
 *
 */


bool_t FASTCALL
BindPlusMinus (
    bnode_t bn
    )
{
    if (!BindLChild (bn) || !BindRChild (bn)) {
        return (FALSE);
    }
    if ((pExState->state.fSupOvlOps == FALSE) &&
      (EVAL_IS_CLASS (ST) && (CLASS_PROP (ST).ovlops == TRUE)) ||
      (EVAL_IS_CLASS (STP) && (CLASS_PROP (STP).ovlops == TRUE))) {
        return (BinaryOverload (bn));
    }
    return (BDPlusMinus (NODE_OP (bn)));
}



/***    BindPMember - Perform a pointer to member access (->*)
 *
 *      fSuccess = BindPMember (bnRight)
 *
 *      Entry   bnRight = based pointer to node
 *
 *      Exit
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL
BindPMember (
    bnode_t bn
    )
{
    Unreferenced(bn);

    pExState->err_num = ERR_OPNOTSUPP;
    return (FALSE);
}




/***    BindPointsTo - Perform a structure access (->)
 *
 *      fSuccess = BindPointsTo (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Exit
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL
BindPointsTo (
    bnode_t bn
    )
{
    eval_t      evalT;
    peval_t     pvT;
    bool_t      retval;
    CV_typ_t    oldClassExp;
    bnode_t     oldbnOp;

    if (!BindLChild (bn)) {
        return (FALSE);
    }
    if (EVAL_IS_REF (ST)) {
        RemoveIndir (ST);
    }

    if (EVAL_IS_CLASS (ST)) {
        return (PointsToOverload (bn));
    }

    // Check to make sure the left operand is a struct/union pointer.
    // To do this, remove a level of indirection from the node's type
    // and see if it's a struct or union.

    if (!EVAL_IS_PTR (ST)) {
        pExState->err_num = ERR_NOTSTRUCTPTR;
        return (FALSE);
    }
    pvT = &evalT;
    *pvT = *ST;
    RemoveIndir (pvT);
    if (!EVAL_IS_CLASS (pvT)) {
        pExState->err_num = ERR_NEEDSTRUCT;
        return (FALSE);
    }
    if (!Fetch ()) {
        return (FALSE);
    }
    if (EVAL_IS_REF (ST)) {
        if (!Fetch ()) {
            return (FALSE);
        }
        EVAL_IS_REF (ST) = FALSE;
    }

#ifdef NEVER
    // disabled -- gdp 1/10/94
    // context should propagate automatically since OP_context
    // has lower precedence than OP_pointsto

    /* Block */ {
        // save/restore the CXT so we can apply any context
        // operator from the left side to the right side.
        bnode_t bnT = BnMatchOp ( NODE_LCHILD ( bn ), OP_context );
        PCXT    pcxtSav = pCxt;

        if ( bnT ) {
            pCxt = SHpCXTFrompCXF ( (PCXF) &( (pnode_t)bnT)->v[0] );
        }

        oldClassExp = ClassExp;
        ClassExp = EVAL_TYP (ST);
        oldbnOp = bnOp;
        bnOp = bn;
        retval = BindRChild (bn);
        ClassExp = oldClassExp;
        bnOp = oldbnOp;
        pCxt = pcxtSav;

    } /* end of Block */
#else

    oldClassExp = ClassExp;
    ClassExp = EVAL_TYP (ST);
    oldbnOp = bnOp;
    bnOp = bn;
    retval = BindRChild (bn);
    ClassExp = oldClassExp;
    bnOp = oldbnOp;

#endif

    if (retval == TRUE) {
        // move element descriptor to previous stack entry and pop stack
        *STP = *ST;
        EVAL_STATE (STP) = EV_lvalue;
        return (PopStack ());
    }
    return (FALSE);
}




/***    BindPostIncDec - Bind expr++ or expr--
 *
 *      fSuccess = BindPostIncDec (bn);
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 */


bool_t FASTCALL
BindPostIncDec (
    bnode_t bn
    )
{
    register op_t        nop = OP_plus;

    if (NODE_OP (bn) == OP_postdec) {
        nop = OP_minus;
    }

    //  load left node and store as return value

    if (!BindLChild (bn)) {
        return(FALSE);
    }
    if ((pExState->state.fSupOvlOps == FALSE) && EVAL_IS_CLASS (ST) &&
      (CLASS_PROP (ST).ovlops == TRUE)) {
        return (BinaryOverload (bn));
    }
    if (!ValidateNodes (nop, ST, NULL)) {
        return(FALSE);
    }

    //  do the post-increment or post-decrement operation and store

    return (BDPrePost (nop));
}




/***    BindPreIncDec - Bind ++expr or --expr
 *
 *      fSuccess = BindPreIncDec (op);
 *
 *      Entry   op = operator
 *
 *      Returns TRUE if successful
 *              FALSE if error
 *
 */


bool_t FASTCALL
BindPreIncDec (
    bnode_t bn
    )
{
    register op_t    nop = OP_plus;

    if (NODE_OP (bn) == OP_predec) {
        nop = OP_minus;
    }
    if (!BindLChild (bn)) {
        return(FALSE);
    }
    if ((pExState->state.fSupOvlOps == FALSE) && EVAL_IS_CLASS (ST) &&
      (CLASS_PROP (ST).ovlops == TRUE)) {
        return (UnaryOverload (bn));
    }
    if (!ValidateNodes (nop, ST, NULL)) {
        return(FALSE);
    }

    //  do the increment or decrement operation and return the result

    return (BDPrePost (nop));
}




/**     BindRelat - bind relational and equality operations
 *
 *      fSuccess = BindRelat (op)
 *
 *      Entry   op = OP_lt, OP_lteq, OP_gt, OP_gteq, OP_eqeq, or OP_bangeq
 *
 *      Returns TRUE if no evaluation error
 *              FALSE if evaluation error
 *
 *      Description
 *       If both operands are arithmetic, passes them on to BDArith().
 *       Otherwise (one or both operands pointers), does the evaluation
 *       here.
 *
 */


bool_t FASTCALL
BindRelat (
    bnode_t bn
    )
{
    if (!BindLChild (bn) || !BindRChild (bn)) {
        return (FALSE);
    }

    if (EVAL_IS_REF (STP)) {
        RemoveIndir (STP);
    }
    if (EVAL_IS_REF (ST)) {
        RemoveIndir (ST);
    }

    if ((pExState->state.fSupOvlOps == FALSE) &&
      (EVAL_IS_CLASS (ST) && (CLASS_PROP (ST).ovlops == TRUE)) ||
      (EVAL_IS_CLASS (STP) && (CLASS_PROP (STP).ovlops == TRUE))) {
        return (BinaryOverload (bn));
    }
    if (EVAL_IS_ENUM (ST)) {
        SetNodeType (ST, ENUM_UTYPE (ST));
    }
    if (EVAL_IS_ENUM (STP)) {
        SetNodeType (STP, ENUM_UTYPE (STP));
    }

    // Check to see if either operand is a pointer
    // If so, the operation is special.  Otherwise,
    // hand it to BDArith ().

    if (!EVAL_IS_PTR (STP) && !EVAL_IS_PTR (ST)) {
        // neither side is a pointer or a reference to a pointer
        return (BDArith (NODE_OP (bn)));
    }

    // Both nodes should now be typed as either near or far
    // pointers.

    //DASSERT ((CV_TYP_IS_PTR (EVAL_TYP (STP))) && (CV_TYP_IS_PTR (EVAL_TYP (ST))));

    //  For the relational operators (<, <=, >, >=),
    //  only offsets are compared.  For the equality operators (==, !=),
    //  both segments and offsets are compared.

    EVAL_STATE (STP) = EV_rvalue;
    SetNodeType (STP, (CV_typ_t) (pExState->state.f32bit ? T_INT4 : T_INT2));
    return (PopStack ());
}


/**     BindRetVal - bind return value of current function
 *
 *      fSuccess = BindRetVal (bn)
 *
 *      Entry   bn = pointer to node
 *
 *      Returns TRUE if no bind error
 *              FALSE if bind error
 *
 *      Description
 *          bn is bound to the return value of the active
 *          function in the current context, assuming that
 *          this function is ready to return.
 */

bool_t FASTCALL
BindRetVal (
    bnode_t bn
    )
{
    peval_t     pv = &bn->v[0];
    HPROC       hProc;
    CV_typ_t    type;

    if (ClassExp != 0) {
        // we are trying to bind the special retval symbol as a
        // class member.
        pExState->err_num = ERR_SYNTAX;
        return FALSE;
    }

    // Get the current function and the type of the
    // return value
    if ((hProc = SHHPROCFrompCXT (pCxt)) == 0 ||
        (type = GetProcType (hProc)) == 0 ||
        (EVAL_MOD (pv) = SHHMODFrompCXT (pCxt)) == 0 ||
        !SetNodeType (pv, type) ||
        !EVAL_IS_FCN (pv) ||
        !SetNodeType (pv, FCN_RETURN(pv))) {

        pExState->err_num = ERR_BADCONTEXT;
        return FALSE;
    }

    // Treat the node as an lvalue in order to allow
    // modification of the return value by the user

    EVAL_STATE (pv) = EV_lvalue;
    pv->CXTT = *pCxt;

    // The return value can be either be a primitive,
    // a pointer, or a class. In the first two cases
    // it can be treated as a register variable that
    // resides in EAX.  If it is a class, the actual
    // object is pointed by eax, and its address will
    // be computed at evaluation time.

    if (EVAL_TYP (pv) != T_VOID &&
#if defined (TARGMAC68K)
            FCN_CALL (pv) != FCN_PASCAL &&
#endif
            !EVAL_IS_CLASS (pv))
    {

        EVAL_IS_REG (pv) = TRUE;

        if (EVAL_IS_PTR (pv))
        {
            // an enregistered pointer: set register to eax
            switch (TargetMachine) {
                case mptmips:
                    PTR_REG_IREG (pv) = CV_M4_IntV0;
                    break;

                case mptdaxp:
                    PTR_REG_IREG (pv) = CV_ALPHA_IntV0;
                    break;

                case mptm68k:
                    PTR_REG_IREG (pv) = CV_R68_D0;
                    break;

                case mptix86:
                    PTR_REG_IREG (pv) = CV_REG_EAX;
                    break;

                case mptmppc:
                case mptntppc:
                    PTR_REG_IREG (pv) = CV_PPC_GPR3;
                    break;

                default:
                    DASSERT(FALSE);
            }

        } else {

            // Return value is either a floating
            // point value in ST(0) or an integer value in EAX

            if (CV_IS_PRIMITIVE (EVAL_TYP (pv)) &&
                CV_TYP_IS_REAL (EVAL_TYP (pv))) {
                switch (TargetMachine) {
                    case mptmips:
                        switch (TypeSize (pv)) {
                            case 4:
                                EVAL_REG (pv) = CV_M4_FltF0;
                                break;

                            case 8:
                                EVAL_REG (pv) = (CV_M4_FltF1<<8)|CV_M4_FltF0;
                                break;

                            default:
                                pExState->err_num = ERR_INTERNAL;
                                return (FALSE);
                        }
                        break;

                    case mptdaxp:
                        EVAL_REG (pv) = CV_ALPHA_FltF0;
                        break;

                    case mptm68k:
                        EVAL_REG (pv) = (CV_R68_D1 | (CV_R68_A1<<8));
                        break;

                    case mptix86:
                        EVAL_REG (pv) = CV_REG_ST0;
                        break;

                    case mptmppc:
                    case mptntppc:
                        EVAL_REG (pv) = CV_PPC_FPR1;
                        break;

                    default:
                        DASSERT(FALSE);
                }
            }
            else {
                switch (TargetMachine) {
                    case mptmips:
                        switch (TypeSize (pv)) {
                            case 1:
                            case 2:
                            case 4:
                                EVAL_REG (pv) = CV_M4_IntV0;
                                break;

                            case 8:
                                EVAL_REG (pv) = (CV_M4_IntV1<<8)|CV_M4_IntV0;
                                break;

                            default:
                                pExState->err_num = ERR_INTERNAL;
                                return (FALSE);
                        }
                        break;

                    case mptdaxp:
                        EVAL_REG (pv) = CV_ALPHA_IntV0;
                        break;

                    case mptm68k:
                        switch (TypeSize (pv)) {
                            case 1:
                            case 2:
                            case 4:
                                EVAL_REG (pv) = CV_R68_D0;
                                break;
                            case 8:
                                //M00INT64: return value
                                pExState->err_num = ERR_TYPESUPPORT;
                                return (FALSE);

                            default:
                                pExState->err_num = ERR_INTERNAL;
                                return (FALSE);
                        }
                        break;

                    case mptix86:
                        switch (TypeSize (pv)) {
                            case 1:
                                EVAL_REG (pv) = CV_REG_AL;
                                break;

                            case 2:
                                EVAL_REG (pv) = CV_REG_AX;
                                break;

                            case 4:
                                EVAL_REG (pv) = CV_REG_EAX;
                                break;
                            case 8:
                                //M00INT64: return value spans edx and eax
                                // needs special handling
                                pExState->err_num = ERR_TYPESUPPORT;
                                return (FALSE);

                            default:
                                pExState->err_num = ERR_INTERNAL;
                                return (FALSE);
                        }
                        break;

                    case mptmppc:
                    case mptntppc:
                        switch (TypeSize (pv)) {
                            case 1:
                            case 2:
                            case 4:
                                EVAL_REG (pv) = CV_PPC_GPR3;
                                break;

                            case 8:
                                //M00INT64: return value spans edx and eax
                                // needs special handling
                                pExState->err_num = ERR_TYPESUPPORT;
                                return (FALSE);

                            default:
                                pExState->err_num = ERR_INTERNAL;
                                return (FALSE);
                        }
                        break;

                }
            }
        }
    }
    return (PushStack (pv));
}


/***    BindSegOp - Handle ':' segmentation operator
 *
 *      fSuccess = BindSegOp (bn)
 *
 *      Entry   bn = based pointer to node
 *              STP = segment value
 *              ST = offset value
 *
 *      Returns TRUE if successful
 *              FALSE is error
 *
 *      DESCRIPTION
 *       Both operands must have integral values (but cannot
 *       be long or ulong).  The result of op1:op2 is a (char
 *       far *) with segment equal to op1 and offset equal to
 *       op2.
 */


bool_t FASTCALL
BindSegOp (
    bnode_t bn
    )
{
    if (TargetMachine != mptix86) {
        pExState->err_num = ERR_OPERANDTYPES;
        return(FALSE);
    }

    if (!BindLChild (bn) || !BindRChild (bn) || !ValidateNodes(OP_segop, STP, ST)) {
        return(FALSE);
    }

    // In addition, check to make sure that segment
    // operand is of type long or ulong.


    switch ( EVAL_TYP ( STP ) ) {
        case T_LONG:
        case T_ULONG:
        case T_QUAD:
        case T_UQUAD:
        case T_INT8:
        case T_UINT8:
            // [cuda:3035 8th Apr 93 sanjays]
            // Accept T_INT4 and T_UINT4 because any integral constant is
            // typed atleast INT. We will check at evaluation time that the
            // value of the integral constant is in range <=  0xffff .
            pExState->err_num = ERR_OPERANDTYPES;
            return (FALSE);
        default:
            break;
    }

    //DASSERT((EVAL_TYP (STP) == T_SHORT) || (EVAL_TYP (STP) == T_USHORT));

    EVAL_STATE (STP) = EV_rvalue;
    SetNodeType (STP, (CV_typ_t) (pExState->state.f32bit ? T_32PFCHAR : T_PFCHAR));
    return (PopStack ());
}





/***    BindSizeOf - Bind sizeof operation
 *
 *      fSuccess = BindSizeOf (bn)
 *
 *      Entry   bn = based pointer to operand node
 *
 *      Exit
 *
 *      Returns TRUE if successful
 *              FALSE if error
 */


bool_t FASTCALL
BindSizeOf (
    bnode_t bn
    )
{
    bnode_t     bnLeft = NODE_LCHILD (bn);
    CV_typ_t    type = (pExState->state.f32bit) ? T_UINT4 : T_UINT2;

    if (NODE_OP (bnLeft) == OP_typestr) {
        // the operand of the sizeof was a type string not an expression
        // we now need to parse the type string and push a type node onto
        // the stack so the following code can determine the type
        if (!ParseType (bnLeft, TRUE)) {
            pExState->err_num = ERR_SYNTAX;
            return (FALSE);
        }
        else if (!PushStack (&((pnode_t)bnLeft)->v[0])) {
            return (FALSE);
        }
    }
    else {
        if (!BindLChild (bn)) {
            return (FALSE);
        }
    }

    // The type of the result of a sizeof operation is unsigned int
    // except for huge arrays which are long to get the full length

    EVAL_STATE (ST) = EV_constant;
    if (type == T_UINT2 && EVAL_IS_ARRAY (ST) && (PTR_ARRAYLEN (ST) > 0xffff)) {
        type = T_ULONG;
    }
    EVAL_ULONG (ST) = TypeSize (ST);
    SetNodeType (ST, type);
    ((pnode_t)bn)->v[0] = *ST;
    return (TRUE);
}




/***    BindSymbol - bind symbol according to scope specification mask
 *
 *      fSuccess = BindSymbol (bn)
 *
 *      Entry   bn = based pointer to tree node
 *
 *      Exit    ST = symbol
 *
 *      Returns TRUE if bind successful
 *              FALSE if bind error
 *
 *      Exit    pExState->err_num = error ordinal if bind error
 *
 */

bool_t FASTCALL
BindSymbol (
    bnode_t bn
    )
{
    search_t    Name;
    token_t     Tok;
    peval_t     pv;
    ulong       clsmask;

    clsmask = BindingScopeOperand ?
              CLS_bclass | CLS_vbase | CLS_ntype :
              CLS_defn;
    if (ClassExp == T_NOTYPE) {
        // look up the identifier using the current context and
        // set the symbol information.  If the symbol is a typedef
        // then the state will be set to EV_type.  Otherwise it will
        // be set to EV_lvalue

        InitSearchSym (bn, &bn->v[0], &Name, ClassExp, SCP_all, clsmask);
        switch (SearchSym (&Name)) {
            case HR_rewrite:
            return (Bind (bn));

            case HR_notfound:
                // if symbol was not found, search for it as a primitive
                if (!ParseType (bn, TRUE)) {
                    // if the current radix is hex and the symbol potentially
                    // could be a number, then change the type of the node
                    if (ParseConst (Name.sstr.lpName, &Tok,
                      pExState->radix) == ERR_NONE) {
                        if (Tok.pbEnd ==
                          (char *)Name.sstr.lpName + Name.sstr.cb) {
                            pExState->err_num = ERR_NONE;
                            NODE_OP (bn) = OP_const;
                            pv = &((pnode_t)bn)->v[0];
                            EVAL_UQUAD (pv) = VAL_UQUAD (&Tok);
                            if (SetNodeType (pv, Tok.typ) == TRUE) {
                                EVAL_STATE (pv) = EV_constant;
                                return (PushStack (pv));
                            }
                        }
                    }
                    ErrUnknownSymbol(&Name.sstr);
                    return (FALSE);
                }
                return (PushStack (&bn->v[0]));

            case HR_found:
                // if the symbol was found, it was pushed onto the stack
                if (SHIsInProlog(pCxt) && (pExState->state.fEProlog == FALSE)) {
                    // we want to reject bp_relative and register
                    // stuff if we are in the prolog or epilog of
                    // a function

                    if ((EVAL_HSYM (ST) && !fValidInProlog (EVAL_HSYM(ST), fNoFuncCxf))){
                        // we have
                        // already found a symbol, but cannot
                        // evaluate it. --caviar #5898
                        pExState->err_num = ERR_NOSTACKFRAME;
                        return (FALSE);
                    }
                }
                break;

            default:
                return (FALSE);
        }
    }
    else {

        // look up the identifier using the current context and
        // set the symbol information.  If the symbol is a typedef
        // then the state will be set to EV_type.  Otherwise it will
        // be set to EV_lvalue

        InitSearchRight (bnOp, bn, &Name, clsmask);
        switch (SearchSym (&Name)) {
            case HR_rewrite:
                return (Bind (bn));
            case HR_notfound:
                // if symbol was not found, search for it as a primitive
                if (!ParseType (bn, TRUE)) {
                    ErrUnknownSymbol(&Name.sstr);
                    return (FALSE);
                }
                return (PushStack (&bn->v[0]));

            case HR_found:
                // if the symbol was found, it was pushed onto the stack
                break;

            default:
                return (FALSE);
        }
    }

    // Dolphin #10794
    // This is a hack to allow distinguishing between
    // struct/class/union tags and identifier names.
    // If "foo" is both a struct tag and an identifier
    // (e.g., as in "struct foo {...} foo;") the EE would
    // bind "foo" to whatever symbol was emitted first by
    // the compiler (i.e., either the UDT or the variable)
    // In general the variable is more interesting than
    // the UDT. So we perform an additional search and
    // prefer the non-UDT symbol, if there is one in the
    // same scope.
    // (This should not affect type casting, which
    // is handled by FindUDT)
    // Class search is excluded in order to avoid potential
    // tree rewrite.

    if (EVAL_STATE (ST) == EV_type && Name.state != SYM_class) {
        // save current scope information
        HMOD hModSav = SHHMODFrompCXT(&Name.CXTT);
        HPROC hProcSav = SHHPROCFrompCXT(&Name.CXTT);
        HBLK hBlkSav = SHHBLKFrompCXT(&Name.CXTT);

        Name.scope &= ~SCP_class;
        if (SearchSym (&Name) == HR_found) {
            if (EVAL_STATE (ST) != EV_type &&
                hModSav == SHHMODFrompCXT(&Name.CXTT) &&
                hProcSav == SHHPROCFrompCXT(&Name.CXTT) &&
                hBlkSav == SHHBLKFrompCXT(&Name.CXTT)) {
                // found an identifier with the same name and
                // in the same scope as the UDT.
                // Prefer this over the UDT we found earlier.
                *STP = *ST;
            }
            PopStack();
        }
    }

    return TRUE;
}




/**     BindUnary - bind an unary arithmetic operation
 *
 *      fSuccess = BindUnary (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if no error during evaluation
 *              FALSE if error during evaluation
 *
 * DESCRIPTION
 *      Binds the result of an arithmetic operation.  The unary operators
 *      dealt with here are:
 *
 *      ~       -       +
 *
 *      Pointer arithmetic is NOT handled; all operands must be of
 *      arithmetic type.
 */


bool_t FASTCALL
BindUnary (
    bnode_t bn
    )
{
    if (!BindLChild (bn)) {
        return (FALSE);
    }

    // we need to check for a reference to a class without losing the fact
    // that this is a reference

    if (EVAL_IS_REF (ST)) {
        RemoveIndir (ST);
    }
    if ((pExState->state.fSupOvlOps == FALSE) && EVAL_IS_CLASS (ST) &&
      (CLASS_PROP (ST).ovlops == TRUE)) {
        return (UnaryOverload (bn));
    }
    if (!ValidateNodes (NODE_OP (bn), ST, NULL)) {
        return (FALSE);
    }
    return (BDUnary (NODE_OP (bn)));
}




/***    BindUScope - Bind unary :: scoping
 *
 *      fSuccess = BindUScope (bnRes);
 *
 *      Entry   bnRes = based pointer to unary scoping node
 *
 *      Exit    *ST = evaluated left node of pnRes
 *
 *      Returns TRUE if evaluation successful
 *              FALSE if error
 */


bool_t FASTCALL
BindUScope (
    bnode_t bn
    )
{
    register bool_t retval;
    CXT         oldCxt;
    CV_typ_t    oldClassImp;

    // save current context packet and set current context to module scope

    oldCxt = *pCxt;
    oldClassImp = ClassImp;
    SHGetCxtFromHmod (SHHMODFrompCXT (pCxt), pCxt);
    // the unary scoping operator specifically means no implicit class
    ClassImp = 0;
    retval = BindLChild (bn);
    *pCxt = oldCxt;
    ClassImp = oldClassImp;
    return (retval);
}



/**     Second level routines.  These routines are called by the various
 *      Bind... routines.
 */




/**     AddrOf - bind an address of node
 *
 *      fSuccess = AddrOf (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if no error during evaluation
 *              FALSE if error during evaluation
 *
 */


bool_t FASTCALL
AddrOf (
    bnode_t bn
    )
{
    CV_typ_t    type;
    eval_t      evalT;
    peval_t     pvT;
    CV_modifier_t   Mod = {0};

    if (!ValidateNodes (OP_addrof, ST, NULL))
        return (FALSE);

    // The operand must be an lvalue and cannot be a register variable

    if ((EVAL_STATE (ST) != EV_lvalue) && (EVAL_STATE (ST) != EV_type)) {
        pExState->err_num = ERR_OPERANDTYPES;
        return (FALSE);
    }
    if (EVAL_IS_REG (ST)) {
        pExState->err_num = ERR_BADREGISTER;
        return (FALSE);
    }

    if (EVAL_IS_PTR (ST)) {
        if (EVAL_IS_REF (ST)) {
            // the address of a reference is a pointer to the value
            // referred to.  This is just a pointer with the reference
            // bit cleared

            EVAL_IS_REF (ST) = FALSE;
            type = EVAL_TYP (ST);
        }
        else {
            pvT = &evalT;
            ProtoPtr (pvT, ST, FALSE, Mod);
            if (MatchType (pvT, FALSE) == MTYP_none) {
                // searching the context of the pointer type for a type
                // record which is a pointer record and has the current
                // pointer type as its underlying type has failed, set
                // the type to pointer to character

                switch EVAL_PTRTYPE (ST) {
                    case CV_PTR_NEAR:
                        type = T_PCHAR;
                        break;

                    case CV_PTR_FAR:
                        type = T_PFCHAR;
                        break;

                    case CV_PTR_HUGE:
                        type = T_PHCHAR;
                        break;

                    case CV_PTR_NEAR32:
                        type = T_32PCHAR;
                        break;

                    case CV_PTR_FAR32:
                        type = T_32PFCHAR;
                        break;

                    case CV_PTR_64:
                        type = T_64PCHAR;
                        break;

                    default:
                        type = (pExState->state.f32bit) ? ((TargetMachine == mptia64)? T_64PCHAR : T_32PCHAR) : T_PFCHAR;
                        break;
                }
            }
            else {
                type = EVAL_TYP (pvT);
            }
        }
    }
    else if (CV_IS_PRIMITIVE (EVAL_TYP (ST))) {
        // if the node is primitive, then a pointer to the primitive type
        // can be created.  We will create the pointer as a near pointer
        // and assume that subsequent code will cast to a far pointer if
        // necessary

        // since I am creating a pointer, I am guessing the type
        // based upon the mode of the current context packet
        type = ADDR_IS_OFF32 (pCxt->addr)?  ((TargetMachine == mptia64)? CV_NEWMODE(EVAL_TYP (ST), CV_TM_NPTR64): CV_NEWMODE(EVAL_TYP (ST), CV_TM_NPTR32)) : CV_NEWMODE(EVAL_TYP (ST), CV_TM_NPTR);
    }
    else if (EVAL_IS_CLASS (ST)) {
        pvT = &evalT;

        ProtoPtr (pvT, ST, FALSE, Mod);
        if (MatchType (pvT, FALSE) == MTYP_none) {
            // searching the context of the class type for a type
            // record which is a pointer record and has the current
            // class type as its underlying type has failed, set
            // the type to pointer to special CV pointer
            type = ADDR_IS_OFF32 (pCxt->addr) ? ((TargetMachine == mptia64) ? T_64NCVPTR: T_32NCVPTR) : T_FCVPTR;
        }
        else {
            type = EVAL_TYP (pvT);
        }
    }
    else {
        // we are punting here and calling the address of anything else
        // a pointer to far character

        type = (pExState->state.f32bit) ? ((TargetMachine == mptia64) ? T_64PCHAR : T_32PCHAR) : T_PFCHAR;
    }

    if ((NODE_STYPE (bn) = type) == 0) {
        // unable to find proper pointer type
        pExState->err_num = ERR_OPERANDTYPES;
        return (FALSE);
    }
    else {
        if (EVAL_STATE (ST) != EV_type) {
            EVAL_STATE (ST) = EV_rvalue;
        }
        return (SetNodeType (ST, type));
    }
}





/**     BDArith - bind an arithmetic operation
 *
 *      fSuccess = BDArith (op)
 *
 *      Entry   op = operator (OP_...)
 *
 *      Returns TRUE if no error during evaluation
 *              FALSE if error during evaluation
 *
 * DESCRIPTION
 *       Binds the result of an arithmetic operation.  The binary operators
 *       dealt with here are:
 *
 *       &&     ||      (both are bound here but evaluation is different)
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


bool_t FASTCALL
BDArith (
    op_t op
    )
{
    CV_typ_t    typRes;
    bool_t      fIsReal;
    bool_t      fIsSigned;
    bool_t      fResInt;

    if (EVAL_IS_REF (ST)) {
        RemoveIndir (ST);
    }
    if (EVAL_IS_REF (STP)) {
        RemoveIndir (STP);
    }
    // Resolve identifiers and check the node types.  If the nodes
    // pass validation, they should not be pointers (only arithmetic
    // operands are handled by this routine).

    if (EVAL_IS_ENUM (ST)) {
        SetNodeType (ST, ENUM_UTYPE (ST));
    }
    if (EVAL_IS_ENUM (STP)) {
        SetNodeType (STP, ENUM_UTYPE (STP));
    }
    if (!ValidateNodes (op, STP, ST)) {
        return (FALSE);
    }
    if (EVAL_IS_BITF (ST)) {
        SetNodeType (ST, BITF_UTYPE (ST));
    }
    if (EVAL_IS_BITF (STP)) {
        SetNodeType (STP, BITF_UTYPE (STP));
    }

    // M00KLUDGE - this is commented out because &&, etc. come through
    // M00KLUDGE - and they allow pointers.
    //DASSERT (!EVAL_IS_PTR (ST) && !EVAL_IS_PTR (STP));

    // The resultant type is the same as the type of the left-hand
    // side (assume for now we don't have the special int-result case).

    typRes = EVAL_TYP (STP);

    fIsReal = CV_TYP_IS_REAL (typRes);
    fIsSigned = CV_TYP_IS_SIGNED (typRes);
    fResInt = FALSE;


    // Finally, check the actual arithmetic operation.

    switch (op) {
        case OP_eqeq:
        case OP_bangeq:
        case OP_lt:
        case OP_gt:
        case OP_lteq:
        case OP_gteq:
        case OP_oror:
        case OP_andand:
            fResInt = TRUE;
            break;

        case OP_plus:
        case OP_minus:
        case OP_mult:
        case OP_div:
            break;

        case OP_mod:
        case OP_shl:
        case OP_shr:
        case OP_and:
        case OP_or:
        case OP_xor:
            // Both operands must have integral type.

            DASSERT(!fIsReal);
            if (fIsReal) {
               return (FALSE);
            }
            else {
                break;
            }

        default:
            DASSERT (FALSE);
            return (FALSE);
    }


    // Now set up the resultant node and coerce back to the correct
    // type:

    if (EVAL_STATE (STP) != EV_type) {
        EVAL_STATE (STP) = EV_rvalue;
    }
    if (fResInt) {
        SetNodeType (STP, (CV_typ_t) (pExState->state.f32bit ? T_INT4 : T_INT2));
    }
    else if (fIsReal) {
        //SetNodeType (STP, T_REAL80);
        SetNodeType (STP, typRes);
    }
    else if (fIsSigned) {
        SetNodeType (STP, T_QUAD);
    }
    else {
        SetNodeType (STP, T_UQUAD);
    }
    if (!fResInt) {
        if (CastNode (STP, typRes, typRes) == FALSE) {
            return (FALSE);
        }
    }
    return (PopStack ());
}





/***    Fetch - complete the fetch (*) operation
 *
 *      fSuccess = Fetch (bn)
 *
 *      Entry   bn = based pointer to node
 *
 *      Returns TRUE if bind successful
 *              FALSE if bind error
 *
 *      Exit    pExState->err_num = error ordinal if bind error
 */


bool_t FASTCALL
Fetch (
    void
    )
{

    // validate the node type

    if (!ValidateNodes (OP_fetch, ST, NULL)) {
        return(FALSE);
    }

    // FUTURE : the right place for this is in the TyperCheckType code.
    if (EVAL_IS_FCN (ST) ) {
        pExState->err_num = ERR_TYPEINCOMPAT;
        return FALSE;
    }

    if (EVAL_IS_BASED (ST)) {
        if (!NormalizeBase (ST)) {
            return(FALSE);
        }
    }
    if (EVAL_STATE (ST) != EV_type) {
        EVAL_STATE (ST) = EV_lvalue;
    }

    // Remove a level of indirection from the resultant type.

    RemoveIndir (ST);
    return (TRUE);
}




/***    BDPlusMinus - Perform an addition or subtraction operation
 *
 *      fSuccess = BDPlusMinus (op)
 *
 *      Entry   op = operator (OP_plus or OP_Minus)
 *              STP = left operand
 *              ST = right operand
 *
 *      Returns TRUE if bind successful
 *              FALSE if bind error
 *
 *      Exit    pExState->err_num = error ordinal if bind error
 *
 *      Notes   Special handling is required when one or both operands are
 *              pointers.  Otherwise, the arguments are passed on to
 *              BDArith ().
 */


bool_t FASTCALL
BDPlusMinus (
    op_t op
    )
{
    if (EVAL_IS_REF (STP)) {
        RemoveIndir (STP);
    }
    if (EVAL_IS_REF (ST)) {
        RemoveIndir (ST);
    }
    if (EVAL_IS_ENUM (ST)) {
        SetNodeType (ST, ENUM_UTYPE (ST));
    }
    if (EVAL_IS_ENUM (STP)) {
        SetNodeType (STP, ENUM_UTYPE (STP));
    }

    // validate node types

    if (!ValidateNodes (op, STP, ST)) {
        return(FALSE);
    }

    // Check to see if either operand is a pointer or a reference to
    // a pointer. If so, the operation is special.  Otherwise,
    // hand it to BDArith ().

    if (!EVAL_IS_PTR (STP) && !EVAL_IS_PTR (ST)) {
        return (BDArith (op));
    }

    // Perform the bind.  There are two cases:
    //
    // I)  ptr + int, int + ptr, ptr - int
    // II) ptr - ptr

    if ((op == OP_plus) || !(EVAL_IS_PTR (ST))) {
        // Case (I). ptr + int, int + ptr, ptr - int
        // The resultant node has the same type as the pointer:
        if (!EVAL_IS_PTR (STP)) {
            *STP = *ST;
        }
        if ((EVAL_STATE (STP) == EV_type) && (EVAL_STATE (ST) == EV_type)) {
            EVAL_STATE (STP) = EV_type;
        }
        else {
            EVAL_STATE (STP) = EV_lvalue;
        }
    }
    else {
        // Case (II): ptr - ptr.  The result is of type ptrdiff_t and
        // is equal to the distance between the two pointers (in the
        // address space) divided by the size of the items pointed to:

        DASSERT (EVAL_IS_PTR (ST));
        if (!EVAL_IS_PTR (STP) || !fCanSubtractPtrs (ST, STP)) {
            pExState->err_num = ERR_OPERANDTYPES;
            return (FALSE);
        }
        if ((EVAL_STATE (STP) == EV_type) && (EVAL_STATE (ST) == EV_type)) {
            EVAL_STATE (STP) = EV_type;
        }
        else {
            EVAL_STATE (STP) = EV_rvalue;
        }
        // we know we are working with pointers so we do not have to check
        // EVAL_IS_PTR (pv)

        if (EVAL_IS_BASED (STP)) {
            NormalizeBase (STP);
        }
        if (EVAL_IS_BASED (ST)) {
            NormalizeBase (ST);
        }
        if (EVAL_IS_NPTR (STP) || EVAL_IS_FPTR (STP)) {
            SetNodeType (STP, T_SHORT);
        }
        if (EVAL_IS_NPTR32 (STP)) {
            SetNodeType (STP, T_LONG);
        }
        if (EVAL_IS_PTR64 (STP)) {
            SetNodeType (STP, T_QUAD); //v-vadimp - needs review
        }
        else {
            SetNodeType (STP, T_LONG);
        }
    }
    return (PopStack());
}




/**     BDPrePost - perform the increment/decrement operation
 *
 *      fSuccess = BDPrePost (op);
 *
 *      Entry   op = operation to perform (OP_plus or OP_minus)
 *
 *      Exit    increment/decrement performed and result stored in memory
 *
 *      Returns TRUE if no error
 *              FALSE if error
 */


bool_t FASTCALL
BDPrePost (
    op_t op
    )
{
    eval_t      evalT;
    peval_t     pvT;

    //  initialize the increment/decrement to a constant 1

    pvT = &evalT;
    CLEAR_EVAL (pvT);
    SetNodeType (pvT, T_USHORT);
    EVAL_STATE (pvT) = EV_constant;
    EVAL_USHORT (pvT) = 1;
    if (!PushStack (pvT)) {
        return (FALSE);
    }
    if (BDPlusMinus (op)) {
        return (TRUE);
    }
    return (FALSE);
}




/**     BDUnary - bind an unary arithmetic operation
 *
 *      fSuccess = BDUnary (op)
 *
 *      Entry   op = operator
 *              ST = operand (must be dereferenced)
 *
 *      Returns TRUE if no error during evaluation
 *              FALSE if error during evaluation
 *
 * DESCRIPTION
 *      Binds the result of an arithmetic operation.  The unary operators
 *      dealt with here are:
 *
 *      !       ~       -       +
 *
 *      Pointer arithmetic is NOT handled; all operands must be of
 *      arithmetic type.
 */


bool_t FASTCALL
BDUnary (
    op_t op
    )
{
    CV_typ_t        typRes;
    bool_t          fIsReal;
    bool_t          fIsSigned;
    register ulong  fResInt;

    if (EVAL_IS_BITF (ST)) {
        SetNodeType (ST, BITF_UTYPE (ST));
    }

    DASSERT (!EVAL_IS_PTR (ST) && !EVAL_IS_CLASS (ST));

    // The resultant type is the same as the type of the left-hand
    // side (assume for now we don't have the special int-result case).

    typRes = EVAL_TYP (ST);

    fIsReal = CV_TYP_IS_REAL (typRes);
    fIsSigned = CV_TYP_IS_SIGNED (typRes);
    fResInt = FALSE;


    // Finally, check the actual arithmetic operation.

    switch (op) {
        case OP_bang:
            fResInt = TRUE;
            break;

        case OP_negate:
        case OP_uplus:
            break;

        case OP_tilde:
            // The operand must have integral type.

            DASSERT (!fIsReal);
            if (fIsReal) {
               return (FALSE);
            }
            else {
                break;
            }

        default:
            DASSERT (FALSE);
            return (FALSE);
    }


    // Now set up the resultant node and coerce back to the correct
    // type:

    EVAL_STATE (ST) = EV_rvalue;
    if (fResInt) {
        SetNodeType (ST, (CV_typ_t) (pExState->state.f32bit ? T_INT4 : T_INT2));
    }
    else if (fIsReal) {
        SetNodeType (ST, typRes);
    }
    else if (fIsSigned) {
        SetNodeType (ST, T_QUAD);
    }
    else {
        SetNodeType (ST, T_UQUAD);
    }
    if (!fResInt) {
        if (CastNode (ST, typRes, typRes) == FALSE) {
            return (FALSE);
        }
    }
    return (TRUE);
}




/**     Function call support routines
 *
 */



/**     PushCArgs - setup argument tree for C style calling
 *
 *      fSuccess = PushCArgs (pvF, pn, pSPOff, argn);
 *
 *      Entry   pvF = pointer to function description
 *              pn = pointer to argument node
 *              pSPOff = pointer to SP relative offset counter
 *              argn = argument number
 *
 *      Exit    type field of node = type of formal argument
 *              type field of node = 0 if vararg
 *              *pSPOff incremented by size of formal or size of actual if vararg
 *
 *      Returns TRUE if no error
 *              FALSE if error
 */


bool_t FASTCALL
PushCArgs (
    peval_t pvF,
    pnode_t pn,
    UOFFSET *pSPOff,
    int argn,
    peval_t pvScr
    )
{
    CV_typ_t    type;
    pargd_t     pa;
    uint        cbVal;
    int         argc;
    farg_t      argtype;
    int         fudgePad = (EVAL_SYM_IS32(pvF)) ? 3 : 1;

    // If C calling convention, push arguments in reverse

    if (NODE_OP (pn) == OP_endofargs) {
        // set the number of required parameters
        argc = FCN_PCOUNT (pvF);
        switch (argtype = GetArgType (pvF, argc, &type)) {
            case FARG_error:
                // there is an error in the OMF or the number of arguments
                // exceeds the number of formals in an exact match list
                pExState->err_num = ERR_FCNERROR;
                return (FALSE);

            case FARG_none:
                // return TRUE if number of actuals is 0
                return (argn == 0);

            case FARG_vararg:
                // if the formals count is zero then this can be
                // either voidargs or varargs.  We cannot tell the
                // difference so we allow either case.  If varargs,
                // then the number of actuals must be at least one
                // less than the number of formals

                if ((argc == 0) || (argn >= argc - 1)) {
                    return (TRUE);
                }
                else {
                    return (FALSE);
                }

            case FARG_exact:
                // varargs are not allowed.  Exact match required
                return (argc == argn);
        }
    }

    // recurse to end of actual argument list

    if (!PushCArgs (pvF, (pnode_t)NODE_RCHILD (pn), pSPOff, argn + 1, pvScr)) {
        return (FALSE);
    }
    else {
        switch (argtype = GetArgType (pvF, argn, &type)) {
            case FARG_error:
            case FARG_none:
            default:
                pExState->err_num = ERR_FCNERROR;
                return (FALSE);

            case FARG_vararg:
            case FARG_exact:
                pa = (pargd_t)&(pn->v[0]);
                pa->type = type;

                // increment relative SP offset by size of item rounded up to the
                // next word and set address field of OP_arg node to relative
                // SP offset.

                SetNodeType (pvScr, pa->type);
                cbVal = (uint)(TypeSize (pvScr) + fudgePad) & ~fudgePad;
                *pSPOff += (UOFFSET)cbVal;
                if (EVAL_IS_REF (pvScr)) {
                    pa->flags.ref = TRUE;
                    SetNodeType (pvScr, PTR_UTYPE(pvScr));
                    // this assignment follows the call to SetNodeType
                    // so that utype be a non-qualified type --gdp 9/21/92
                    pa->utype = EVAL_TYP (pvScr);
                    if (EVAL_IS_CLASS (pvScr)) {
                        pa->flags.utclass = TRUE;
                    }
                }
                pa->flags.isreg = FALSE;
                pa->vallen = cbVal;
                pa->SPoff = *pSPOff;
                return (TRUE);
        }
    }
}

/**     PushPArgs - push arguments for Pascal call
 *
 *      fSuccess = PushPArgs (pvF, pn, pSPOff);
 *
 *      Entry   pvF = pointer to function description
 *              pn = pointer to argument node
 *              pSPOff = pointer to SP relative offset counter
 *
 *      Exit    type field of node = type of formal argument
 *              *pSPOff incremented by size of formal
 *
 *      Returns TRUE if parameters pushed without error
 *              FALSE if error during push
 */


bool_t FASTCALL
PushPArgs (
    peval_t pvF,
    pnode_t pnArg,
    UOFFSET *pSPOff,
    peval_t pvScr
    )
{
    pargd_t     pa;
    int         argn = 0;
    CV_typ_t    type;
    long        cbVal;
    int         fudgePad = (EVAL_SYM_IS32(pvF)) ? 3 : 1;

    // push arguments onto stack left to right

    for (; NODE_OP (pnArg) != OP_endofargs; pnArg = (pnode_t)NODE_RCHILD (pnArg)) {
        switch (GetArgType (pvF, argn, &type)) {
            case FARG_error:
            case FARG_vararg:
                pExState->err_num = ERR_FCNERROR;
                return (FALSE);

            case FARG_none:
                return (TRUE);

            case FARG_exact:
                pa = (pargd_t)&pnArg->v[0];

                // increment relative SP offset by size of item rounded up to the
                // next word and set address field of OP_arg node to relative
                // SP offset.

                pa->type = type;
                SetNodeType (pvScr, type);

                // increment relative SP offset by size of item rounded up to the
                // next word and set address field of OP_arg node to relative
                // SP offset.

                cbVal = (ulong )(TypeSize (pvScr) + fudgePad) & ~fudgePad;
                *pSPOff += (UOFFSET)cbVal;
                pa->vallen = (ulong )cbVal;
                pa->SPoff = *pSPOff;
                pa->flags.isreg = FALSE;
                if (EVAL_IS_REF (pvScr)) {
                    pa->flags.ref = TRUE;
                    SetNodeType (pvScr, PTR_UTYPE(pvScr));
                    // this assignment follows the call to SetNodeType
                    // so that utype be a non-qualified type --gdp 9/21/92
                    pa->utype = EVAL_TYP (pvScr);
                    if (EVAL_IS_CLASS (pvScr)) {
                        pa->flags.utclass = TRUE;
                    }
                }
                argn++;
        }
    }
    return (TRUE);
}


/**     PushFArgs - push arguments for fastcall call
 *
 *      fSuccess = PushFArgs (pvF, pn, pSPOff);
 *
 *      Entry   pvF = pointer to function description
 *              pn = pointer to argument node
 *              pSPOff = pointer to SP relative offset counter
 *
 *      Exit    type field of node = type of formal argument
 *              *pSPOff incremented by size of formal
 *
 *      Returns TRUE if parameters pushed without error
 *              FALSE if error during push
 */

#define AX_PARAM        0x1
#define DX_PARAM        0x2
#define BX_PARAM        0x4
#define ES_PARAM        0x8
#define CX_PARAM        0x10


bool_t FASTCALL
PushFArgs (
    peval_t pvF,
    pnode_t pnArg,
    UOFFSET *pSPOff,
    peval_t pvScr
    )
{
    ulong       regmask = 0;
    int         argn = 0;
    CV_typ_t    type;
    pargd_t     pa;
    uint        cbVal;
    int         fudgePad;
    bool_t (FASTCALL *pFastCallReg) (pargd_t pa, peval_t pv, ulong  *mask);

    if (EVAL_SYM_IS32(pvF)) {
        plfEasy         pType;
        fudgePad = 3;
        pFastCallReg = &FastCallReg32;
        // if this is a 32 bit fast call which returns a UDT by value - we gotta reserve the ECX register for
        // address of the return tmp
        if (!CV_IS_PRIMITIVE (FCN_RETURN(pvF))) {
            HTYPE hType =THGetTypeFromIndex (EVAL_MOD (pvF), FCN_RETURN(pvF));
            if (hType == 0)
                return (FALSE);
            pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
            switch (pType->leaf) {
                case LF_CLASS:
                case LF_STRUCTURE:
                case LF_UNION:
                    regmask |= CX_PARAM;
                    break;
            }
        }
    }
    else {
        fudgePad =1;
        pFastCallReg = &FastCallReg;
    }

    for (; NODE_OP (pnArg) != OP_endofargs; pnArg = (pnode_t)NODE_RCHILD (pnArg)) {
        switch (GetArgType (pvF, argn, &type)) {
            case FARG_error:
            case FARG_vararg:
                pExState->err_num = ERR_FCNERROR;
                return (FALSE);

            case FARG_none:
                return (TRUE);

            case FARG_exact:
                pa = (pargd_t)&pnArg->v[0];
                pa->type = type;
                SetNodeType (pvScr, type);
                if (!(*pFastCallReg) (pa, pvScr, &regmask)) {
                    // increment relative SP offset by size of item rounded up to the
                    // next word and set address field of OP_arg node to relative
                    // SP offset.

                    cbVal = (uint)(TypeSize (pvScr) + fudgePad) & ~fudgePad;
                    *pSPOff += (UOFFSET)cbVal;
                    pa->flags.isreg = FALSE;
                    pa->vallen = cbVal;
                    pa->SPoff = *pSPOff;
                }
                if (EVAL_IS_REF (pvScr)) {
                    pa->flags.ref = TRUE;
                    SetNodeType (pvScr, PTR_UTYPE(pvScr));
                    // this assignment follows the call to SetNodeType
                    // so that utype be a non-qualified type --gdp 9/21/92
                    pa->utype = EVAL_TYP (pvScr);
                    if (EVAL_IS_CLASS (pvScr)) {
                        pa->flags.utclass = TRUE;
                    }
                }
                argn++;
        }
    }
    return (TRUE);
}





/***    FastCallReg and FastCallReg32 - assign fast call parameter to register
 *
 *      fSuccess = FastCallReg (pa, pv, pmask)
 *
 *      Entry   pa = pointer to argument data
 *              pv = pointer to value
 *              pmask = pointer to allocation mask.  *pmask must be
 *                      zero on first call
 *
 *      Exit    EVAL_IS_REG (pv) = TRUE if assigned to register
 *              EVAL_REG (pv) = register ordinal if assigned to register
 *              *pmask updated if assigned to register
 *
 *      Returns TRUE if parameter is passed in register
 *              FALSE if parameter is not passed in register
 */


bool_t FASTCALL
FastCallReg (
    pargd_t pa,
    peval_t pv,
    ulong  *mask
    )
{
    if (!SetNodeType (pv, pa->type)) {
        DASSERT (FALSE);
        return (FALSE);
    }
    pa->vallen = (ulong )TypeSize (pv);

    switch (pa->type) {

        case T_UCHAR:
        case T_CHAR:
        case T_RCHAR:
        case T_INT1:
        case T_UINT1:
        case T_USHORT:
        case T_SHORT:
        case T_INT2:
        case T_UINT2:
            // assign these types to registers ax, dx,bx
            // note that the character types will use the full register
int_order:
            // Allocation order is hard-wired
            if (!(*mask & AX_PARAM)) {
                *mask |= AX_PARAM;
                pa->flags.isreg = TRUE;
                pa->reg = pa->vallen == 1 ? CV_REG_AL : CV_REG_AX;
            }
            else if (!(*mask & DX_PARAM)) {
                *mask |= DX_PARAM;
                pa->flags.isreg = TRUE;
                pa->reg = pa->vallen == 1 ? CV_REG_DL : CV_REG_DX;
            }
            else if (!(*mask & BX_PARAM)) {
                *mask |= BX_PARAM;
                pa->flags.isreg = TRUE;
                pa->reg = pa->vallen == 1 ? CV_REG_BL : CV_REG_BX;
            } else {
                return (FALSE);
            }
            break;

        case T_ULONG:
        case T_LONG:
        case T_INT4:
        case T_UINT4:
            // assign long values to dx:ax
            if (!(*mask & AX_PARAM) && !(*mask & DX_PARAM)) {
                *mask |= AX_PARAM | DX_PARAM;
                pa->flags.isreg = TRUE;
                pa->reg = (CV_REG_DX << 8) | CV_REG_AX;
            }
            else {
                return (FALSE);
            }
            break;


        default:
            if (EVAL_IS_PTR (pv) && EVAL_IS_NPTR (pv)) {
                // assign short pointers (including references)
                // to bx, ax, dx.  Allocation order is hard-wired
                if (!(*mask & BX_PARAM)) {
                    *mask |= BX_PARAM;
                    pa->flags.isreg = TRUE;
                    pa->reg = CV_REG_BX;
                }
                else {
                    goto int_order; // nasty tail merging of mine
                }
            }
            else if (EVAL_IS_PTR (pv) && EVAL_IS_NPTR32 (pv)) {
                DASSERT (FALSE);    // M00FLAT32
            }
            else {
                //M00KLUDGE - it is assumed that far pointers go on the stack
                return (FALSE);
            }
            break;
    }
    return (TRUE);
}


bool_t FASTCALL
FastCallReg32 (
    pargd_t pa,
    peval_t pv,
    ulong  *mask
    )
{
    if (!SetNodeType (pv, pa->type)) {
        DASSERT (FALSE);
        return (FALSE);
    }
    pa->vallen = (ulong )TypeSize (pv);

    switch (pa->type) {

        default:
            if (!EVAL_IS_PTR (pv)) {
                return (FALSE);
            }
            // else fall thru

        case T_UCHAR:
        case T_CHAR:
        case T_RCHAR:
        case T_INT1:
        case T_UINT1:
        case T_USHORT:
        case T_SHORT:
        case T_INT2:
        case T_UINT2:
        case T_ULONG:
        case T_LONG:
        case T_INT4:
        case T_UINT4:

            // assign these types to registers ECX or EDX
            // note that the character types will use the full register

            // Allocation order is hard-wired
            if (!(*mask & CX_PARAM)) {
                *mask |= CX_PARAM;
                pa->flags.isreg = TRUE;
                pa->reg = CV_REG_ECX;
            }
            else if (!(*mask & DX_PARAM)) {
                *mask |= DX_PARAM;
                pa->flags.isreg = TRUE;
                pa->reg = CV_REG_EDX;
            }
            else {
                return (FALSE);
            }
            break;

    }
    return (TRUE);
}

bool_t FASTCALL
PushMArgs (
    peval_t pvF,
    pnode_t pnArg,
    UOFFSET *pSPOff,
    peval_t pvScr
 )
/*++

Routine Description:

    This routine computes the offsets for a MIPS calling convention routine.

Arguments:

    pvF - Supplies a pointer to the function description
    pn  - Supplies a pointer to the arugment node
    pSPOff - Supplies pointer to the Stack Pointer relative offset counter
                this value is updated to reflect pushed parameters

Return Value:

    TRUE if parameters pushed without error else FALSE

--*/

{
    uint        regmask = 0;
    eval_t      evalRet;
    peval_t     pvRet;


    /*
     *  Must deal with return type and this parameters before
     *  dealing with anything else.
     */

    pvRet = &evalRet;
    *pvRet = *ST;
    SetNodeType( pvRet, FCN_RETURN(pvRet));

    if (EVAL_IS_METHOD(pvF)) {
        SET_PARAM_INT(&regmask, 0);
    }

    if (!EVAL_IS_REF(pvRet) &&
        !CV_IS_PRIMITIVE(EVAL_TYP(pvRet)) &&
        (TypeSize(pvRet) > 4) &&
        (CV_TYPE ( EVAL_TYP (pvRet)) != CV_REAL)) {

        if (IS_PARAM_EMPTY(&regmask, 0)) {
            SET_PARAM_INT(&regmask, 0);
        } else {
            SET_PARAM_INT(&regmask, 1);
        }
    }

    /*
     *  Now deal with the actual declared parameter list.
     */

    return PushMArgs2( pvF, pnArg, pSPOff, 0, regmask, pvScr);
}

bool_t FASTCALL
PushAArgs (
    peval_t pvF,
    pnode_t pnArg,
    UOFFSET *pSPOff,
    peval_t pvScr
    )
/*++

Routine Description:

    This routine computes the offsets for a ALPHA calling convention routine.

Arguments:

    pvF - Supplies a pointer to the function description
    pn  - Supplies a pointer to the argument node
    pSPOff - Supplies pointer to the Stack Pointer relative offset counter
                this value is updated to reflect pushed parameters

Return Value:

    TRUE if parameters pushed without error else FALSE

--*/

{
    uint        regmask = 0;
    eval_t      evalRet;
    peval_t     pvRet;


    /*
     *  Must deal with return type and this parameters before
     *  dealing with anything else.
     */

    pvRet = &evalRet;
    *pvRet = *ST;
    SetNodeType( pvRet, FCN_RETURN(pvRet));

    if (EVAL_IS_METHOD(pvF)) {
        SET_PARAM_INT(&regmask, 0);
    }

    /*
     * By rights, the check below should be > 8, but other quad support
     * might still be missing
     */
    if(!EVAL_IS_REF(pvRet) && !CV_IS_PRIMITIVE(EVAL_TYP(pvRet)) &&
       (TypeSize(pvRet) > 4) && (CV_TYPE (EVAL_TYP (pvRet)) != CV_REAL)) {

        if(IS_PARAM_EMPTY(&regmask, 0)) {
            SET_PARAM_INT(&regmask, 0);
        } else {
            SET_PARAM_INT(&regmask, 1);
        }
    }

    /*
     *  Now deal with the actual declared parameter list.
     */

    return PushAArgs2( pvF, pnArg, pSPOff, 0, regmask, pvScr);
}


bool_t FASTCALL
PushMArgs2 (
    peval_t pvF,
    pnode_t pnArg,
    UOFFSET *pSPOff,
    int     argn,
    uint    regmask,
    peval_t pvScr
    )
/*++

Routine Description:

    This routine computes the offsets for a MIPS calling convention routine.

Arguments:

    pvF - Supplies a pointer to the function description

    pn  - Supplies a pointer to the arugment node

    pSPOff - Supplies pointer to the Stack Pointer relative offset counter
                this value is updated to reflect pushed parameters

    argn   - Supplies the count of arguments pushed to date

Return Value:

    TRUE if parameters pushed without error else FALSE

--*/

{
    int         argc;
    CV_typ_t    type;
    pargd_t     pa;
    uint        cbVal;
    int         cbR;
    farg_t      argtype;
    BOOL        fReg;

    /*
     *  Arguments are pushed in reverse (C) order
     */

    if (NODE_OP(pnArg) == OP_endofargs) {
        /*
         *      Set number of required parameters
         */

        argc = FCN_PCOUNT( pvF );
        switch( argtype = GetArgType( pvF, (int  ) argc, &type ) ) {
            case FARG_error:

                //  Error in the OMF or the number of arguments
                //    exceeds the number of formals in an exact match list

                pExState->err_num = ERR_FCNERROR;
                return FALSE;

            case FARG_none:

                //  return TRUE if number of actuals is 0

                *pSPOff = 16;
                return (argn == 0);

            case FARG_vararg:

                //  if the formals count is zero then this can be
                //  either voidargs or varargs.  We cannot tell the
                //  difference so we allow either case.  If varargs,
                //  then the number of actuals must be at least one
                //  less than the number of formals

                if ((argc == 0) || (argn >= argc - 1)) {
                    return (TRUE);
                }
                else {
                    return (FALSE);
                }

            case FARG_exact:
                if (*pSPOff < 16) {
                    *pSPOff = 16;
                } else {
                    *pSPOff = (*pSPOff + 8 - 1) & ~(8 - 1);
                }
                return (argc == argn);
            }
    }

    /*
     *  Need to get the size of the item to be pushed so that we can
     *  do correct alignment of the stack for this data item.
     */

    switch ( argtype = GetArgType( pvF, (int  ) argn, &type )) {

    default:
        DASSERT(FALSE);

        /*
         *  If no type or error then return error
         */
    case FARG_error:
    case FARG_none:
        pExState->err_num = ERR_FCNERROR;
        return FALSE;

    case FARG_vararg:
    case FARG_exact:
        pa = (pargd_t)&pnArg->v[0];
        pa->type = type;
        pa->flags.isreg = FALSE;

        SetNodeType (pvScr, type);

        fReg = MipsCallReg( pa, pvScr, &regmask);

        /*
         *  We always allocate space on the stack for any argument
         *  even if it is placed in a register.
         */

        /*
         *  To compute location on stack take the size of the
         *  item and round to DWORDS.  The stack is then aligned
         *  to this size.
         *
         *  NOTENOTE??? - I don't know if this is correct for structures.
         */


        cbVal = (uint)(TypeSize(pvScr) + 3) & ~3;
        cbR = (cbVal > 8) ? 8 : cbVal;
        *pSPOff = (*pSPOff + cbR - 1 ) & ~(cbR - 1);

        cbR = (DWORD)*pSPOff;

        *pSPOff += cbVal;

        break;

    }


    /*
     *  At an actual arguement.  Recurse down the list to the end
     *  and then process this argument
     */

    if (!PushMArgs2( pvF, NODE_RCHILD (pnArg), pSPOff,
                    argn+1, regmask, pvScr)) {
        return FALSE;
    } else {

        /*
         *   Allocate space on stack (in increments of 4) and
         *   save the offset of the stack for the item.  Offsets
         *  are saved backwards (i.e. from the end of the stack) so
         *  we can push them on the stack easier.
         */

        pa->SPoff = *pSPOff - cbR;

        if (EVAL_IS_REF( pvScr )) {
            pa->flags.ref = TRUE;
            pa->utype = PTR_UTYPE ( pvScr );
            SetNodeType (pvScr, pa->utype );
            if (EVAL_IS_CLASS (pvScr)) {
                pa->flags.utclass = TRUE;
            }
        }
    }
    return (TRUE);
}                               /* PushMArgs() */

bool_t FASTCALL
PushAArgs2 (
    peval_t pvF,
    pnode_t pnArg,
    UOFFSET *pSPOff,
    int     argn,
    uint    regmask,
    peval_t pvScr
    )
/*++

Routine Description:

    This routine computes the offsets for a ALPHA calling convention routine.

Arguments:

    pvF - Supplies a pointer to the function description
    pn  - Supplies a pointer to the arugment node
    pSPOff - Supplies pointer to the Stack Pointer relative offset counter
                this value is updated to reflect pushed parameters
    argn   - Supplies the count of arguments pushed to date

Return Value:

    TRUE if parameters pushed without error else FALSE

--*/

{
    int         argc;
    CV_typ_t    type;
    pargd_t     pa;
    uint        cbVal;
    int         cbR = 0;
    farg_t      argtype;
    BOOL        fReg;

    /*
     *  Arguments are pushed in reverse (C) order
     */

    if (NODE_OP(pnArg) == OP_endofargs) {
        /*
         *      Set number of required parameters
         */

        argc = FCN_PCOUNT( pvF );
        switch( argtype = GetArgType( pvF, (int  ) argc, &type ) ) {
            case FARG_error:

                //  Error in the OMF or the number of arguments
                //    exceeds the number of formals in an exact match list

                pExState->err_num = ERR_FCNERROR;
                return FALSE;

            case FARG_none:

                //  return TRUE if number of actuals is 0

                *pSPOff = 16;
                return (argn == 0);

            case FARG_vararg:

                //  if the formals count is zero then this can be
                //  either voidargs or varargs.  We cannot tell the
                //  difference so we allow either case.  If varargs,
                //  then the number of actuals must be at least one
                //  less than the number of formals

                if ((argc == 0) || (argn >= argc - 1)) {
                    return (TRUE);
                }
                else {
                    return (FALSE);
                }

            case FARG_exact:
                if (*pSPOff < 16) {
                    *pSPOff = 16;
                } else {
                    *pSPOff = (*pSPOff + 8 - 1) & ~(8 - 1);
                }
                return (argc == argn);
            }
    }

    /*
     *  Need to get the size of the item to be pushed so that we can
     *  do correct alignment of the stack for this data item.
     */

    switch ( argtype = GetArgType( pvF, (int  ) argn, &type )) {

    default:
        DASSERT(FALSE);

        /*
         *  If no type or error then return error
         */
    case FARG_error:
    case FARG_none:
        pExState->err_num = ERR_FCNERROR;
        return FALSE;

    case FARG_vararg:
    case FARG_exact:
        pa = (pargd_t)&pnArg->v[0];
        pa->type = type;
        pa->flags.isreg = FALSE;

        SetNodeType (pvScr, type);

        fReg = AlphaCallReg( pa, pvScr, &regmask);

        /*
         *  Space is only allocated on the stack for arguments
         *  that aren't in registers.  The argument home area
         *  is in the stack space of the callee, so allocating
         *  it here would be double-allocation.
         */

        /*
         *  To compute location on stack take the size of the
         *  item and round to QUADWORDS.  The stack is then aligned
         *  to this size.
         *
         *  NOTENOTE??? - I don't know if this is correct for structures.
         */

        if (fReg == FALSE) {
            cbVal = (uint)(TypeSize(pvScr) + 7) & ~7;
            cbR = (cbVal > 16) ? 16 : cbVal;
            *pSPOff = (*pSPOff + cbR - 1 ) & ~(cbR - 1);

            cbR = (DWORD)*pSPOff;

            *pSPOff += cbVal;

            break;
        }

    }


    /*
     *  At an actual arguement.  Recurse down the list to the end
     *  and then process this argument
     */

    if (!PushAArgs2( pvF, NODE_RCHILD (pnArg), pSPOff,
                    argn+1, regmask, pvScr)) {
        return FALSE;
    } else {

        /*
         *  Indicate where on the stack this goes, if it goes anywhere.
         *  If cbR isn't reasonable (ie 0) for Register variables, the
         *  routine evaluation won't work.
         *  They are saved backwards (i.e. from the end of the stack) so
         *  we can push them on the stack easier.
         */

        pa->SPoff = *pSPOff - cbR;

        if (EVAL_IS_REF( pvScr )) {
            pa->flags.ref = TRUE;
            pa->utype = PTR_UTYPE ( pvScr );
            SetNodeType (pvScr, pa->utype );
            if (EVAL_IS_CLASS (pvScr)) {
                pa->flags.utclass = TRUE;
            }
        }
    }
    return (TRUE);
}                               /* PushAArgs2() */

LOCAL bool_t FASTCALL
PushIA64Args (
    peval_t pvF,
    pnode_t pnArg,
    UOFFSET *pSPOff,
    peval_t pvScr
    )

/*++

Routine Description:

    This routine computes the offsets for a routine using
    the Alpha calling convention.

Arguments:

    pvF - Supplies a pointer to the function description
    pn  - Supplies a pointer to the arugment node
    pSPOff - Supplies pointer to the Stack Pointer relative offset counter
                this value is updated to reflect pushed parameters

Return Value:

    TRUE if parameters pushed without error else FALSE

--*/

{
    uint        regmask = 0;
    eval_t      evalRet;
    peval_t     pvRet;


    /*
     *  Must deal with return type and this parameters before
     *  dealing with anything else.
     */

    pvRet = &evalRet;
    *pvRet = *ST;
    SetNodeType( pvRet, FCN_RETURN(pvRet));

    if (EVAL_IS_METHOD(pvF)) {
        SET_PARAM_INT(&regmask, 0);
    }

    if (!EVAL_IS_REF(pvRet) &&
        !CV_IS_PRIMITIVE(EVAL_TYP(pvRet)) &&
// MBH - bugbug
// for us, we should check against a size of 8, not 4,
// but the other quad support is still missing, so leave it be.
//
        (TypeSize(pvRet) > 4) &&
        (CV_TYPE ( EVAL_TYP (pvRet)) != CV_REAL)) {

        if (IS_PARAM_EMPTY(&regmask, 0)) {
            SET_PARAM_INT(&regmask, 0);
        } else {
            SET_PARAM_INT(&regmask, 1);
        }
    }


    /*
     *  Now deal with the actual declared parameter list.
     */

    return PushIA64Args2( pvF, pnArg, pSPOff, 0, regmask, pvScr);
}


LOCAL bool_t FASTCALL
PushIA64Args2 (
    peval_t pvF,
    pnode_t pnArg,
    UOFFSET *pSPOff,
    int     argn,
    uint    regmask,
    peval_t pvScr
    )

/*++

Routine Description:

    IA64 - see PushIA64Args, above

Arguments:

    pvF - Supplies a pointer to the function description
    pn  - Supplies a pointer to the arugment node
    pSPOff - Supplies pointer to the Stack Pointer relative offset counter
                this value is updated to reflect pushed parameters
    argn   - Supplies the count of arguments pushed to date

Return Value:

    TRUE if parameters pushed without error else FALSE

--*/

{
    int         argc;
    CV_typ_t    type;
    pargd_t     pa;
    uint        cbVal;
    int         cbR = 0;
    farg_t      argtype;
    BOOL        fReg;

    /*
     *  Arguments are pushed in reverse (C) order
     */

    if (NODE_OP(pnArg) == OP_endofargs) {
        /*
         *      Set number of required parameters
         */

        argc = FCN_PCOUNT( pvF );
        switch( argtype = GetArgType( pvF, (short) argc, &type ) ) {

               /*
                *    Error in the OMF or the number of arguments
                *       exceeds the number of formals in an exact match list
                */
           case FARG_error:
               pExState->err_num = ERR_FCNERROR;
               return FALSE;

               /*
                *       return TRUE if number of actuals is 0
                */

           case FARG_none:
               *pSPOff = 16;
               return (argn == 0);

               /*
                *   if the formals count is zero then this can be
                *       either voidargs or varargs.  We cannot tell
                *       the difference so we allow either case.  If
                *       varargs, then the number of acutals must
                *       be at least one less than the number of formals
                */

               if ((argc == 0) || (argn >= argc - 1)) {
                   return TRUE;
               }
               return FALSE;

               /*
                *  Varargs are not allowed.  Exact match required
                */

           case FARG_vararg:
               // if the formals count is zero then this can be
               // either voidargs or varargs.  We cannot tell the
               // difference so we allow either case.  If varargs,
               // then the number of actuals must be at least one
               // less than the number of formals

               if ((argc == 0) || (argn >= argc - 1)) {
                   return (TRUE);
               }
               else {
                   return (FALSE);
               }

           case FARG_exact:
               if (*pSPOff < 16) {
                   *pSPOff = 16;
               } else {
                   *pSPOff = (*pSPOff + 8 - 1) & ~(8 - 1);
               }
               return (argc == argn);
           }
    }

    /*
     *  Need to get the size of the item to be pushed so that we can
     *  do correct alignment of the stack for this data item.
     */

    switch ( argtype = GetArgType( pvF, (short) argn, &type )) {

    default:
        DASSERT(FALSE);

        /*
         *  If no type or error then return error
         */
    case FARG_error:
    case FARG_none:
        pExState->err_num = ERR_FCNERROR;
        return FALSE;

    case FARG_vararg:
    case FARG_exact:
        pa = (pargd_t)&pnArg->v[0];
        pa->type = type;
        pa->flags.isreg = FALSE;

        SetNodeType (pvScr, type);

        fReg = IA64CallReg( pa, pvScr, &regmask);

        /*
         *  Space is only allocated on the stack for arguments
         *  that aren't in registers.  The argument home area
         *  is in the stack space of the callee, so allocating
         *  it here would be double-allocation.
         */

        /*
         *  To compute location on stack take the size of the
         *  item and round to QUADWORDS.  The stack is then aligned
         *  to this size.
         *
         *  NOTENOTE??? - I don't know if this is correct for structures.
         */

        if (fReg == FALSE) {
            cbVal = (uint)(TypeSize(pvScr) + 7) & ~7;
            cbR = (cbVal > 16) ? 16 : cbVal;
            *pSPOff = (*pSPOff + cbR - 1 ) & ~(cbR - 1);

            cbR = (DWORD)*pSPOff;

            *pSPOff += cbVal;

            break;

        }
    }

    /*
     *  At an actual arguement.  Recurse down the list to the end
     *  and then process this argument
     */

    if (!PushIA64Args2( pvF, NODE_RCHILD (pnArg), pSPOff,
                    argn+1, regmask, pvScr)) {
        return FALSE;
    } else {
        /*
         *  Indicate where on the stack this goes, if it goes anywhere.
         *  If cbR isn't reasonable (ie 0) for Register variables, the
         *  routine evaluation won't work.
         *  They are saved backwards (i.e. from the end of the stack) so
         *  we can push them on the stack easier.
         */

        pa->SPoff = *pSPOff - cbR;

        if (EVAL_IS_REF( pvScr )) {
            pa->flags.ref = TRUE;
            pa->utype = PTR_UTYPE ( pvScr );
            SetNodeType (pvScr, pa->utype );
            if (EVAL_IS_CLASS (pvScr)) {
                pa->flags.utclass = TRUE;
            }
        }
    }
    return (TRUE);
}                               /* PushIA64Args2() */


/***    MipsCallReg - assign mips call parameter to register
 *
 *      fSuccess = MipsCallReg (pa, pv, pmask)
 *
 *      Entry   pa = pointer to argument data
 *              pv = pointer to value
 *              pmask = pointer to allocation mask.  *pmask must be
 *                      zero on first call
 *
 *      Exit    EVAL_IS_REG (pv) = TRUE if assigned to register
 *              EVAL_REG (pv) = register ordinal if assigned to register
 *              *pmask updated if assigned to register
 *
 *      Returns TRUE if parameter is passed in register
 *              FALSE if parameter is not passed in register
 */

bool_t FASTCALL
MipsCallReg (
    pargd_t    pa,
    peval_t    pv,
    uint *mask)
{

    if (!SetNodeType (pv, pa->type)) {
        DASSERT (FALSE);
        return (FALSE);
    }

    /*
     * Are there any slots free?
     */

    pa->vallen = (ulong )TypeSize (pv);

    if (!IS_PARAM_EMPTY(mask, 3)) {
        return FALSE;
    }

    /*
     *  Depending on the type we need to allocate something to the
     *  correct set of registers and to void other registers as
     *  appriopirate
     */

    switch( pa->type ) {
        /*
         *  These are all assigned to $4, $5, $6, $7 -- which ever
         *      is first available.  When a register is used it
         *      is marked as unavailable.
         */

        default:
            if (pa->vallen > 4) {
                break;
            }

        case T_UCHAR:
        case T_CHAR:
        case T_RCHAR:
        case T_USHORT:
        case T_SHORT:
        case T_INT2:
        case T_UINT2:
        case T_ULONG:
        case T_LONG:
        case T_INT4:
        case T_UINT4:
        case T_32PCHAR:
        case T_32PUCHAR:
        case T_32PRCHAR:
        case T_32PWCHAR:
        case T_32PINT2:
        case T_32PUINT2:
        case T_32PSHORT:
        case T_32PUSHORT:
        case T_32PINT4:
        case T_32PUINT4:
        case T_32PLONG:
        case T_32PULONG:
        case T_32PINT8:
        case T_32PUINT8:
        case T_32PREAL32:
        case T_32PREAL48:
        case T_32PREAL64:

            if (IS_PARAM_EMPTY(mask, 0)) {
                SET_PARAM_INT(mask, 0);
                pa->flags.isreg = TRUE;
                pa->reg = CV_M4_IntA0;

            } else if (IS_PARAM_EMPTY(mask, 1)) {
                SET_PARAM_INT(mask, 1);
                pa->flags.isreg = TRUE;
                pa->reg = CV_M4_IntA1;

            } else if (IS_PARAM_EMPTY(mask, 2)) {
                SET_PARAM_INT(mask, 2);
                pa->flags.isreg = TRUE;
                pa->reg = CV_M4_IntA2;

            } else if (IS_PARAM_EMPTY(mask, 3)) {
                SET_PARAM_INT(mask, 3);
                pa->flags.isreg = TRUE;
                pa->reg = CV_M4_IntA3;

            } else {
                DASSERT(FALSE);
                return FALSE;

            }
            return TRUE;

            /*
             *
             */

        case T_REAL32:

            if (IS_PARAM_EMPTY(mask, 0)) {
                SET_PARAM_FLOAT(mask, 0);
                pa->flags.isreg = TRUE;
                pa->reg = CV_M4_FltF12;

            } else if (IS_PARAM_EMPTY(mask, 1)) {
                SET_PARAM_FLOAT(mask, 1);
                pa->flags.isreg = TRUE;
                if (IS_PARAM_FLOAT(mask, 0)) {
                    pa->reg = CV_M4_FltF14;
                } else {
                    pa->reg = CV_M4_IntA1;
                }

            } else if (IS_PARAM_EMPTY(mask, 2)) {
                SET_PARAM_FLOAT(mask, 2);
                pa->flags.isreg = TRUE;
                pa->reg = CV_M4_IntA2;

            } else if (IS_PARAM_EMPTY(mask, 3)) {
                SET_PARAM_FLOAT(mask, 3);
                pa->flags.isreg = TRUE;
                pa->reg = CV_M4_IntA3;
            } else {
                DASSERT(FALSE);
                return FALSE;
            }

            return TRUE;

            /*
             *
             */

        case T_REAL64:

            if (IS_PARAM_EMPTY(mask, 0)) {
                SET_PARAM_DOUBLE(mask, 0);
                SET_PARAM_DOUBLE(mask, 1);
                pa->flags.isreg = TRUE;
                pa->reg = ( CV_M4_FltF13 << 8 ) | CV_M4_FltF12;

            } else if (IS_PARAM_EMPTY(mask, 2)) {
                SET_PARAM_DOUBLE(mask, 2);
                SET_PARAM_DOUBLE(mask, 3);
                pa->flags.isreg = TRUE;

                if (IS_PARAM_DOUBLE(mask, 0)) {
                    pa->reg = ( CV_M4_FltF15 << 8 ) | CV_M4_FltF14;
                } else {
                    pa->reg = ( CV_M4_IntA3 << 8) | CV_M4_IntA2;
                }
            } else {
                DASSERT(FALSE);
                return FALSE;
            }

            return TRUE;


    }

    *mask = 0xffffffff;
    return FALSE;
}                               /* MipsCallReg() */

/***    AlphaCallReg - assign Alpha call parameter to register
 *
 *      fSuccess = AlphaCallReg (pa, pv, pmask)
 *
 *      Entry   pa = pointer to argument data
 *              pv = pointer to value
 *              pmask = pointer to allocation mask.  *pmask must be
 *                      zero on first call
 *
 *      Exit    EVAL_IS_REG (pv) = TRUE if assigned to register
 *              EVAL_REG (pv) = register ordinal if assigned to register
 *              *pmask updated if assigned to register
 *
 *      Returns TRUE if parameter is passed in register
 *              FALSE if parameter is not passed in register
 */

bool_t FASTCALL
AlphaCallReg (
    pargd_t  pa,
    peval_t  pv,
    uint    *mask
    )
{

    if (!SetNodeType (pv, pa->type)) {
        DASSERT (FALSE);
        return (FALSE);
    }

    /*
     * Are there any slots free?
     */

    pa->vallen = (ulong )TypeSize (pv);

    if (!IS_PARAM_EMPTY(mask, 5)) {
        return FALSE;
    }

    /*
     *  Depending on the type we need to allocate something to the
     *  correct set of registers and to void other registers as
     *  appriopirate
     */

    switch( pa->type ) {
        /*
         *  These are all assigned to IntA0 - IntA5 -- which ever
         *      is first available.  When a register is used it
         *      is marked as unavailable.
         */

        default:
            if (pa->vallen > 8) {
                break;
            }

        case T_UCHAR:
        case T_CHAR:
        case T_RCHAR:
        case T_USHORT:
        case T_SHORT:
        case T_INT2:
        case T_UINT2:
        case T_ULONG:
        case T_LONG:
        case T_INT4:
        case T_UINT4:
        case T_QUAD:
        case T_UQUAD:
        case T_INT8:
        case T_UINT8:
        case T_32PCHAR:
        case T_32PUCHAR:
        case T_32PRCHAR:
        case T_32PWCHAR:
        case T_32PINT2:
        case T_32PUINT2:
        case T_32PSHORT:
        case T_32PUSHORT:
        case T_32PINT4:
        case T_32PUINT4:
        case T_32PLONG:
        case T_32PULONG:
//        case T_32PQUAD:
//        case T_32PUQUAD:
        case T_32PINT8:
        case T_32PUINT8:
        case T_32PREAL32:
        case T_32PREAL48:
        case T_32PREAL64:

            if (IS_PARAM_EMPTY(mask, 0)) {
                SET_PARAM_INT(mask, 0);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_IntA0;

            } else if (IS_PARAM_EMPTY(mask, 1)) {
                SET_PARAM_INT(mask, 1);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_IntA1;

            } else if (IS_PARAM_EMPTY(mask, 2)) {
                SET_PARAM_INT(mask, 2);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_IntA2;

            } else if (IS_PARAM_EMPTY(mask, 3)) {
                SET_PARAM_INT(mask, 3);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_IntA3;

            } else if (IS_PARAM_EMPTY(mask, 4)) {
                SET_PARAM_INT(mask, 4);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_IntA4;

            } else if (IS_PARAM_EMPTY(mask, 5)) {
                SET_PARAM_INT(mask, 5);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_IntA5;

            } else {
                DASSERT(FALSE);
                return FALSE;

            }
            return TRUE;

            /*
             *
             */

        case T_REAL32:

            if (IS_PARAM_EMPTY(mask, 0)) {
                SET_PARAM_FLOAT(mask, 0);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF16;

            } else if (IS_PARAM_EMPTY(mask, 1)) {
                SET_PARAM_FLOAT(mask, 1);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF17;

            } else if (IS_PARAM_EMPTY(mask, 2)) {
                SET_PARAM_FLOAT(mask, 2);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF18;

            } else if (IS_PARAM_EMPTY(mask, 3)) {
                SET_PARAM_FLOAT(mask, 3);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF19;

            } else if (IS_PARAM_EMPTY(mask, 4)) {
                SET_PARAM_FLOAT(mask, 4);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF20;

            } else if (IS_PARAM_EMPTY(mask, 5)) {
                SET_PARAM_FLOAT(mask, 5);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF21;

            } else {
                DASSERT(FALSE);
                return FALSE;
            }

            return TRUE;

        case T_REAL64:

            if (IS_PARAM_EMPTY(mask, 0)) {
                SET_PARAM_DOUBLE(mask, 0);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF16;

            } else if (IS_PARAM_EMPTY(mask, 1)) {
                SET_PARAM_DOUBLE(mask, 1);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF17;

            } else if (IS_PARAM_EMPTY(mask, 2)) {
                SET_PARAM_DOUBLE(mask, 2);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF18;

            } else if (IS_PARAM_EMPTY(mask, 3)) {
                SET_PARAM_DOUBLE(mask, 3);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF19;

            } else if (IS_PARAM_EMPTY(mask, 4)) {
                SET_PARAM_DOUBLE(mask, 4);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF20;

            } else if (IS_PARAM_EMPTY(mask, 5)) {
                SET_PARAM_DOUBLE(mask, 5);
                pa->flags.isreg = TRUE;
                pa->reg = CV_ALPHA_FltF21;

            } else {
                DASSERT(FALSE);
                return FALSE;
            }

            return TRUE;
    }

    *mask = 0xffffffff;
    return FALSE;
}                               /* AlphaCallReg() */


/***    Ia64CallReg - assign IA64 call parameter to register
 *
 *      fSuccess = Ia64CallReg (pa, pv, pmask)
 *
 *      Entry   pa = pointer to argument data
 *              pv = pointer to value
 *              pmask = pointer to allocation mask.  *pmask must be
 *                      zero on first call
 *
 *      Exit    EVAL_IS_REG (pv) = TRUE if assigned to register
 *              EVAL_REG (pv) = register ordinal if assigned to register
 *              *pmask updated if assigned to register
 *
 *      Returns TRUE if parameter is passed in register
 *              FALSE if parameter is not passed in register
 */

LOCAL bool_t FASTCALL
IA64CallReg (
    pargd_t    pa,
    peval_t    pv,
    uint *mask
    )
{

    if (!SetNodeType (pv, pa->type)) {
        DASSERT (FALSE);
        return (FALSE);
    }

    /*
     * Are there any slots free?
     */

    pa->vallen = (ushort)TypeSize (pv);

    if (!IS_PARAM_EMPTY(mask, 5)) {
        return FALSE;
    }

    /*
     *  Depending on the type we need to allocate something to the
     *  correct set of registers and to void other registers as
     *  appropriate
     */

//	TBD **** IA64 calling convention passes argument via RSE stack
//  v-vadimp - EM should take care of all the specifics for setting stacked registers
    switch( pa->type ) {

    default:
        if (pa->vallen > 8) {
            break;
        }

    case T_UCHAR:
    case T_CHAR:
    case T_RCHAR:
    case T_USHORT:
    case T_SHORT:
    case T_INT2:
    case T_UINT2:
    case T_ULONG:
    case T_LONG:
    case T_INT4:
    case T_UINT4:
    case T_QUAD:
    case T_UQUAD:
    case T_INT8:
    case T_UINT8:
    case T_32PCHAR:
    case T_32PUCHAR:
    case T_32PRCHAR:
    case T_32PWCHAR:
    case T_32PINT2:
    case T_32PUINT2:
    case T_32PSHORT:
    case T_32PUSHORT:
    case T_32PINT4:
    case T_32PUINT4:
    case T_32PLONG:
    case T_32PULONG:
    case T_32PINT8:
    case T_32PUINT8:
    case T_32PREAL32:
    case T_32PREAL48:
    case T_32PREAL64:
    case T_64PCHAR:
    case T_64PUCHAR:
    case T_64PRCHAR:
    case T_64PWCHAR:
    case T_64PINT2:
    case T_64PUINT2:
    case T_64PSHORT:
    case T_64PUSHORT:
    case T_64PINT4:
    case T_64PUINT4:
    case T_64PLONG:
    case T_64PULONG:
    case T_64PINT8:
    case T_64PUINT8:
    case T_64PREAL32:
    case T_64PREAL48:
    case T_64PREAL64:

        if (IS_PARAM_EMPTY(mask, 0)) {
            SET_PARAM_INT(mask, 0);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_IntR32;

        } else if (IS_PARAM_EMPTY(mask, 1)) {
            SET_PARAM_INT(mask, 1);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_IntR33;

        } else if (IS_PARAM_EMPTY(mask, 2)) {
            SET_PARAM_INT(mask, 2);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_IntR34;

        } else if (IS_PARAM_EMPTY(mask, 3)) {
            SET_PARAM_INT(mask, 3);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_IntR35;

        } else if (IS_PARAM_EMPTY(mask, 4)) {
            SET_PARAM_INT(mask, 4);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_IntR36;

        } else if (IS_PARAM_EMPTY(mask, 5)) {
            SET_PARAM_INT(mask, 5);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_IntR37;

        } else if (IS_PARAM_EMPTY(mask, 6)) {
            SET_PARAM_INT(mask, 6);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_IntR38;

        } else if (IS_PARAM_EMPTY(mask, 7)) {
            SET_PARAM_INT(mask, 7);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_IntR39;

        } else {
            DASSERT(FALSE);
            return FALSE;

        }
        return TRUE;

    case T_REAL32:

        if (IS_PARAM_EMPTY(mask, 0)) {
            SET_PARAM_FLOAT(mask, 0);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT2;

        } else if (IS_PARAM_EMPTY(mask, 1)) {
            SET_PARAM_FLOAT(mask, 1);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT3;

        } else if (IS_PARAM_EMPTY(mask, 2)) {
            SET_PARAM_FLOAT(mask, 2);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT4;

        } else if (IS_PARAM_EMPTY(mask, 3)) {
            SET_PARAM_FLOAT(mask, 3);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT5;

        } else if (IS_PARAM_EMPTY(mask, 4)) {
            SET_PARAM_FLOAT(mask, 4);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT6;

        } else if (IS_PARAM_EMPTY(mask, 5)) {
            SET_PARAM_FLOAT(mask, 5);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT7;

        } else if (IS_PARAM_EMPTY(mask, 6)) {
            SET_PARAM_FLOAT(mask, 6);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT8;

        } else if (IS_PARAM_EMPTY(mask, 7)) {
            SET_PARAM_FLOAT(mask, 7);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT9;

        } else {
            DASSERT(FALSE);
            return FALSE;
        }

        return TRUE;

    case T_REAL64:

        if (IS_PARAM_EMPTY(mask, 0)) {
            SET_PARAM_DOUBLE(mask, 0);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT2;

        } else if (IS_PARAM_EMPTY(mask, 1)) {
            SET_PARAM_DOUBLE(mask, 1);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT3;

        } else if (IS_PARAM_EMPTY(mask, 2)) {
            SET_PARAM_DOUBLE(mask, 2);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT4;

        } else if (IS_PARAM_EMPTY(mask, 3)) {
            SET_PARAM_DOUBLE(mask, 3);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT5;

        } else if (IS_PARAM_EMPTY(mask, 4)) {
            SET_PARAM_DOUBLE(mask, 4);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT6;

        } else if (IS_PARAM_EMPTY(mask, 5)) {
            SET_PARAM_DOUBLE(mask, 5);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT7;

        } else if (IS_PARAM_EMPTY(mask, 6)) {
            SET_PARAM_DOUBLE(mask, 6);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT8;

        } else if (IS_PARAM_EMPTY(mask, 7)) {
            SET_PARAM_DOUBLE(mask, 7);
            pa->flags.isreg = TRUE;
            pa->reg = CV_IA64_FltT9;

        } else {
            DASSERT(FALSE);
            return FALSE;
        }

        return TRUE;

    } 
    *mask = 0xffffffff;
    return FALSE;
}                               /* Ia64CallReg() */

struct _OvlMap {
    op_t    op;
    op_t    ovlfcn;
};

struct _OvlMap SEGBASED (_segname("_CODE")) BinaryOvlMap[] = {
    {OP_preinc      ,OP_Oincrement  },
    {OP_predec      ,OP_Odecrement  },
    {OP_postinc     ,OP_Oincrement  },
    {OP_postdec     ,OP_Odecrement  },
    {OP_function    ,OP_Ofunction   },
    {OP_lbrack      ,OP_Oarray      },
    {OP_pmember     ,OP_Opmember    },
    {OP_mult        ,OP_Ostar       },
    {OP_div         ,OP_Odivide     },
    {OP_mod         ,OP_Opercent    },
    {OP_plus        ,OP_Oplus       },
    {OP_minus       ,OP_Ominus      },
    {OP_shl         ,OP_Oshl        },
    {OP_shr         ,OP_Oshr        },
    {OP_lt          ,OP_Oless       },
    {OP_lteq        ,OP_Olessequal  },
    {OP_gt          ,OP_Ogreater    },
    {OP_gteq        ,OP_Ogreatequal },
    {OP_eqeq        ,OP_Oequalequal },
    {OP_bangeq      ,OP_Obangequal  },
    {OP_and         ,OP_Oand        },
    {OP_xor         ,OP_Oxor        },
    {OP_or          ,OP_Oor         },
    {OP_andand      ,OP_Oandand     },
    {OP_oror        ,OP_Ooror       },
    {OP_eq          ,OP_Oequal      },
    {OP_multeq      ,OP_Otimesequal },
    {OP_diveq       ,OP_Odivequal   },
    {OP_modeq       ,OP_Opcentequal },
    {OP_pluseq      ,OP_Oplusequal  },
    {OP_minuseq     ,OP_Ominusequal },
    {OP_shleq       ,OP_Oleftequal  },
    {OP_shreq       ,OP_Orightequal },
    {OP_andeq       ,OP_Oandequal   },
    {OP_xoreq       ,OP_Oxorequal   },
    {OP_oreq        ,OP_Oorequal    },
    {OP_comma       ,OP_Ocomma      }
};
#define BINARYOVLMAPCNT  (sizeof (BinaryOvlMap)/sizeof (struct _OvlMap))





/**     BinaryOverload - process overloaded binary operator
 *
 *      fSuccess = BinaryOverload (bn)
 *
 *      Entry   bn = based pointer to operator node
 *              STP = pointer to left operand if not function operator
 *              ST = pointer to right operand if not function operator
 *              STP = pointer to argument list if function operator
 *              ST = pointer to class object if function operator
 *
 *      Exit    tree rewritten to function call and bound
 *
 *      Returns TRUE if tree rewritten and bound correctly
 *              FALSE if error
 *
 *      Note:   If the node operator is post increment or decrement, then
 *              the code below will supply an implicit second argument of
 *              0;
 */


bool_t
BinaryOverload (
    bnode_t bn
    )
{
    ulong       lenClass;
    ulong       lenGlobal;
    HDEP        hOld = 0;
    HDEP        hClass = 0;
    HDEP        hGlobal = 0;
    pstree_t    pOld = NULL;
    pstree_t    pClass = NULL;
    pstree_t    pGlobal = NULL;
    bool_t      fClass = FALSE;
    bool_t      fGlobal = FALSE;
    bnode_t     Fcn;
    bnode_t     Left;
    bnode_t     LeftRight;
    bnode_t     Arg1;
    bnode_t     Arg2;
    bnode_t     EndArg;
    bnode_t     Zero;
    eval_t      evalSTP;
    eval_t      evalST;
    eval_t      evalClass;
    eval_t      evalGlobal;
    op_t        OldOper = NODE_OP (bn);
    op_t        Oper;
    bool_t      PostID = FALSE;
    bool_t      RightOp = TRUE;
    peval_t     pv;
    ulong       i;


    // search for the overload operator name

    for (i = 0; i < BINARYOVLMAPCNT; i++) {
        if (BinaryOvlMap[i].op == OldOper) {
            break;
        }
    }
    if (i == BINARYOVLMAPCNT) {
        pExState->err_num = ERR_INTERNAL;
        return (FALSE);
    }
    Oper = BinaryOvlMap[i].ovlfcn;

    lenClass = 3 * (sizeof (node_t) + sizeof (eval_t)) + sizeof (node_t)
      + sizeof (node_t) + sizeof (argd_t);

    lenGlobal = 2 * (sizeof (node_t) + sizeof (eval_t)) + sizeof (node_t) +
      2 * (sizeof (node_t) + sizeof (argd_t));

    if ((OldOper == OP_postinc) || (OldOper == OP_postdec)) {
        // if we are processing post increment/decrement, then we have to
        // supply the implicit zero second argument

        PostID = TRUE;
        RightOp = FALSE;
        lenClass += sizeof (node_t) + sizeof (eval_t);
        lenGlobal += sizeof (node_t) + sizeof (eval_t);
    }

    if ((hClass = DupETree (lenClass, &pClass)) == 0) {
        return (FALSE);
    }
    if ((hGlobal = DupETree (lenGlobal, &pGlobal)) == 0) {
        MemUnLock (hClass);
        MemFree (hClass);
        return (FALSE);
    }

    // save and pop the left and right operands

    evalST = *ST;
    PopStack ();
    if (RightOp == TRUE) {
        // if we have class--, class++ or class->, there is only one operand
        // on the evaluation stack.  Otherwise, we have to pop and save the
        // right operand.
        evalSTP = *ST;
        PopStack ();
    }

    // generate the expression tree for "a.operator@ (b)"
    // and link it to the current node which is the made into an OP_noop

    hOld = pExState->hETree;
    pOld = pTree;
    pExState->hETree = hClass;
    pTree = pClass;

    Fcn = (bnode_t)pTree->node_next;
    Left = (bnode_t)((char *)Fcn + sizeof (node_t) + sizeof (eval_t));
    LeftRight = (bnode_t)((char *)Left + sizeof (node_t) + sizeof (eval_t));
    Arg1 = (bnode_t)((char *)LeftRight + sizeof (node_t) + sizeof (eval_t));
    EndArg = (bnode_t)((char *)Arg1 + sizeof (node_t) + sizeof (argd_t));
    if (PostID == TRUE) {
        // if we are processing post increment/decrement, then we have to
        // supply the implicit zero second argument

        Zero = (bnode_t)((char *)EndArg + sizeof (node_t));
        NODE_OP (Zero) = OP_const;
        pv = &Zero->v[0];
        EVAL_STATE (pv) = EV_constant;
        if (pExState->state.f32bit) {
            SetNodeType (pv, T_INT4);
            EVAL_LONG (pv) = 0;
        }
        else {
            SetNodeType (pv, T_INT2);
            EVAL_SHORT (pv) = 0;
        }
    }
    pTree->node_next += lenClass;
    NODE_OP (Fcn) = OP_function;
    NODE_LCHILD (Fcn) = Left;
    NODE_OP (Left) = OP_dot;
    NODE_LCHILD (Left) = NODE_LCHILD (bn);
    NODE_RCHILD (Left) = LeftRight;
    NODE_OP (LeftRight) = Oper;
    NODE_RCHILD (Fcn) = Arg1;
    NODE_OP (Arg1) = OP_arg;
    if (PostID == TRUE) {
        NODE_LCHILD (Arg1) = Zero;
    }
    else {
        NODE_LCHILD (Arg1) = NODE_RCHILD (bn);
    }
    NODE_RCHILD (Arg1) = EndArg;
    NODE_OP (EndArg) = OP_endofargs;
    NODE_LCHILD (bn) = Fcn;
    NODE_RCHILD (bn) = 0;
    NODE_OP (bn) = OP_noop;

    // bind method call

    CkPointStack ();
    if ((fClass = Function (Fcn)) == TRUE) {
        evalClass = *ST;
        PopStack ();
    }

    // Function( ) could cause a re-alloc.
    hClass = pExState->hETree ;

    if (ResetStack () == FALSE) {
        pExState->err_num = ERR_INTERNAL;
        return (FALSE);
    }

    // the expression tree may have been altered during bind

    pClass = pTree;
    pExState->err_num = ERR_NONE;

    if ((OldOper != OP_function) && (OldOper != OP_eq) &&
        (OldOper != OP_lbrack)) {

        // generate the expression tree for "operator@ (a, b)"
        // and link it to the current node which is the made into an OP_noop

        pExState->hETree = hGlobal;
        pTree = pGlobal;

        Fcn = (bnode_t)pTree->node_next;
        Left = (bnode_t)((char *)Fcn + sizeof (node_t) + sizeof (eval_t));
        Arg1 = (bnode_t)((char *)Left + sizeof (node_t) + sizeof (eval_t));
        Arg2 = (bnode_t)((char *)Arg1 + sizeof (node_t) + sizeof (argd_t));
        EndArg = (bnode_t)((char *)Arg2 + sizeof (node_t) + sizeof (argd_t));
        if (PostID == TRUE) {
            // if we are processing post increment/decrement, then we have to
            // supply the implicit zero second argument

            Zero = (bnode_t)((char *)EndArg + sizeof (node_t));
            NODE_OP (Zero) = OP_const;
            pv = &Zero->v[0];
            EVAL_STATE (pv) = EV_constant;
            if (pExState->state.f32bit) {
                SetNodeType (pv, T_INT4);
                EVAL_LONG (pv) = 0;
            }
            else {
                SetNodeType (pv, T_INT2);
                EVAL_SHORT (pv) = 0;
            }
        }
        pTree->node_next += lenGlobal;
        NODE_OP (Fcn) = OP_function;
        NODE_LCHILD (Fcn) = Left;
        NODE_OP (Left) = Oper;
        NODE_RCHILD (Fcn) = Arg1;
        NODE_OP (Arg1) = OP_arg;
        NODE_LCHILD (Arg1) = NODE_LCHILD (bn);
        NODE_RCHILD (Arg1) = Arg2;
        NODE_OP (Arg2) = OP_arg;
        if (PostID == TRUE) {
            NODE_LCHILD (Arg2) = Zero;
        }
        else {
            NODE_LCHILD (Arg2) = NODE_RCHILD (bn);
        }
        NODE_RCHILD (Arg2) = EndArg;
        NODE_OP (EndArg) = OP_endofargs;
        NODE_LCHILD (bn) = Fcn;
        NODE_RCHILD (bn) = 0;
        NODE_OP (bn) = OP_noop;

        // bind function call

        CkPointStack ();
        if ((fGlobal = Function (Fcn)) == TRUE) {
            evalGlobal = *ST;
            PopStack ();
        }
        // Function could cause a realloc and the memory handle might change
        // due to this.
        hGlobal = pExState->hETree ;
        if (ResetStack () == FALSE) {
            pExState->err_num = ERR_INTERNAL;
            return (FALSE);
        }

        // the expression tree may have been altered during bind

        pGlobal = pTree;
        pExState->err_num = ERR_NONE;
    }

    if ((fClass == FALSE) && (fGlobal == FALSE)) {
        pExState->err_num = ERR_NOOVERLOAD;
        pExState->hETree = hOld;
        pTree = pOld;
        MemUnLock (hClass);
        MemFree (hClass);
        MemUnLock (hGlobal);
        MemFree (hGlobal);
        PushStack (&evalSTP);
        if (PostID == FALSE) {
            PushStack (&evalST);
        }
        return (FALSE);
    }
    else if ((fClass == TRUE) && (fGlobal == TRUE)) {
        pExState->err_num = ERR_AMBIGUOUS;
        pExState->hETree = hOld;
        pTree = pOld;
        MemUnLock (hClass);
        MemFree (hClass);
        MemUnLock (hGlobal);
        MemFree (hGlobal);
        return (FALSE);
    }
    else if (fClass == TRUE) {
        pExState->hETree = hClass;
        pTree = pClass;
        MemUnLock (hGlobal);
        MemFree (hGlobal);
        MemUnLock (hOld);
        MemFree (hOld);
        return (PushStack (&evalClass));
    }
    else {
        MemUnLock (hClass);
        MemFree (hClass);
        MemUnLock (hOld);
        MemFree (hOld);
        return (PushStack (&evalGlobal));
    }
}




struct _OvlMap SEGBASED (_segname("_CODE")) UnaryOvlMap[] = {
    {OP_bang        ,OP_Obang       },
    {OP_tilde       ,OP_Otilde      },
    {OP_negate      ,OP_Ominus      },
    {OP_uplus       ,OP_Oplus       },
    {OP_fetch       ,OP_Ostar       },
    {OP_addrof      ,OP_Oand        },
};
#define UNARYOVLMAPCNT  (sizeof (UnaryOvlMap)/sizeof (struct _OvlMap))




/**     UnaryOverload - process overloaded unary operator
 *
 *      fSuccess = UnaryOverload (bn)
 *
 *      Entry   bn = based pointer to operator node
 *              ST = pointer to operand (actual operand if pv is reference)
 *
 *      Exit    tree rewritten to function call and bound
 *
 *      Returns TRUE if tree rewritten and bound correctly
 *              FALSE if error
 */


bool_t
UnaryOverload (
    bnode_t bn
    )
{
    ulong       lenClass;
    ulong       lenGlobal;
    HDEP        hOld = 0;
    HDEP        hClass = 0;
    HDEP        hGlobal = 0;
    pstree_t    pOld = NULL;
    pstree_t    pClass = NULL;
    pstree_t    pGlobal = NULL;
    bool_t      fClass;
    bool_t      fGlobal;
    bnode_t     Fcn;
    bnode_t     Left;
    bnode_t     LeftRight;
    bnode_t     Right;
    bnode_t     RightRight;
    eval_t      evalST;
    eval_t      evalClass;
    eval_t      evalGlobal;
    op_t        Oper = NODE_OP (bn);
    ulong       i;

    // search for the overload operator name


    for (i = 0; i < UNARYOVLMAPCNT; i++) {
        if (UnaryOvlMap[i].op == Oper) {
            break;
        }
    }
    if (i == UNARYOVLMAPCNT) {
        pExState->err_num = ERR_INTERNAL;
        return (FALSE);
    }
    Oper = UnaryOvlMap[i].ovlfcn;

    // the amount of space required for an overloaded unary method is
    //      OP_function + OP_dot + OP_ident + OP_endofargs
    // There is actually another node which is the unary operand but we
    // reuse that (subtree) node

    lenClass = 3 * (sizeof (node_t) + sizeof (eval_t)) + sizeof (node_t);

    // the amount of space required for an overloaded unary global is
    //      OP_function + OP_ident + OP_arg + OP_ident + OP_endofargs

    lenGlobal = 3 * (sizeof (node_t) + sizeof (eval_t)) + sizeof (node_t) +
      sizeof (node_t) + sizeof (argd_t);

    if ((hClass = DupETree (lenClass, &pClass)) == 0) {
        return (FALSE);
    }
    if ((hGlobal = DupETree (lenGlobal, &pGlobal)) == 0) {
        MemUnLock (hClass);
        MemFree (hClass);
        return (FALSE);
    }

    // save and pop the current stack top

    evalST = *ST;
    PopStack ();

    // generate the expression tree for "a.operator@ ()"
    // and link it to the current node which is the made into an OP_noop

    hOld = pExState->hETree;
    pOld = pTree;
    pExState->hETree = hClass;
    pTree = pClass;

    Fcn = (bnode_t)pTree->node_next;
    Left = (bnode_t)((char *)Fcn + sizeof (node_t) + sizeof (eval_t));
    LeftRight = (bnode_t)((char *)Left + sizeof (node_t) + sizeof (eval_t));
    Right = (bnode_t)((char *)LeftRight + sizeof (node_t) + sizeof (eval_t));
    pTree->node_next += lenClass;
    NODE_OP (Fcn) = OP_function;
    NODE_LCHILD (Fcn) = Left;
    NODE_OP (Left) = OP_dot;
    NODE_LCHILD (Left) = NODE_LCHILD (bn);
    NODE_RCHILD (Left) = LeftRight;
    NODE_OP (LeftRight) = Oper;
    NODE_RCHILD (Fcn) = Right;
    NODE_OP (Right) = OP_endofargs;
    NODE_LCHILD (bn) = Fcn;
    NODE_RCHILD (bn) = 0;
    NODE_OP (bn) = OP_noop;

    // bind method call

    CkPointStack ();
    if ((fClass = Function (Fcn)) == TRUE) {
        evalClass = *ST;
        PopStack ();
    }

    // Function() could cause a realloc and the hClass that we stored
    // might not be valid anymore. Remember the new memory handle.
    hClass = pExState->hETree ;

    if (ResetStack () == FALSE) {
        pExState->err_num = ERR_INTERNAL;
        return (FALSE);
    }

    // the expression tree may have been altered during bind

    pClass = pTree;
    pExState->err_num = ERR_NONE;

    // generate the expression tree for "operator@ (a)"
    // and link it to the current node which is the made into an OP_noop

    pExState->hETree = hGlobal;
    pTree = pGlobal;

    Fcn = (bnode_t)pTree->node_next;
    Left = (bnode_t)((char *)Fcn + sizeof (node_t) + sizeof (eval_t));
    Right = (bnode_t)((char *)Left + sizeof (node_t) + sizeof (eval_t));
    RightRight = (bnode_t)((char *)Right + sizeof (node_t) + sizeof (argd_t));
    pTree->node_next += lenGlobal;
    NODE_OP (Fcn) = OP_function;
    NODE_LCHILD (Fcn) = Left;
    NODE_OP (Left) = Oper;
    NODE_RCHILD (Fcn) = Right;
    NODE_OP (Right) = OP_arg;
    NODE_LCHILD (Right) = NODE_LCHILD (bn);
    NODE_RCHILD (Right) = RightRight;
    NODE_OP (RightRight) = OP_endofargs;
    NODE_LCHILD (bn) = Fcn;
    NODE_RCHILD (bn) = 0;
    NODE_OP (bn) = OP_noop;

    // bind function call

    CkPointStack ();
    if ((fGlobal = Function (Fcn)) == TRUE) {
        evalGlobal = *ST;
        PopStack ();
    }

    // Function( ) could cause a realloc, remember the new handle
    // so we free the correct handle eventually.
    hGlobal = pExState->hETree ;

    if (ResetStack () == FALSE) {
        pExState->err_num = ERR_INTERNAL;
        return (FALSE);
    }

    // the expression tree may have been altered during bind

    pGlobal = pTree;
    pExState->err_num = ERR_NONE;

    if ((fClass == FALSE) && (fGlobal == FALSE)) {
        pExState->err_num = ERR_NOOVERLOAD;
        pExState->hETree = hOld;
        pTree = pOld;
        MemUnLock (hClass);
        MemFree (hClass);
        MemUnLock (hGlobal);
        MemFree (hGlobal);
        PushStack (&evalST);
        return (FALSE);
    }
    else if ((fClass == TRUE) && (fGlobal == TRUE)) {
        pExState->err_num = ERR_AMBIGUOUS;
        pExState->hETree = hOld;
        pTree = pOld;
        MemUnLock (hClass);
        MemFree (hClass);
        MemUnLock (hGlobal);
        MemFree (hGlobal);
        return (FALSE);
    }
    else if (fClass == TRUE) {
        pExState->hETree = hClass;
        pTree = pClass;
        MemUnLock (hGlobal);
        MemFree (hGlobal);
        MemUnLock (hOld);
        MemFree (hOld);
        return (PushStack (&evalClass));
    }
    else {
        MemUnLock (hClass);
        MemFree (hClass);
        MemUnLock (hOld);
        MemFree (hOld);
        return (PushStack (&evalGlobal));
    }
}




/**     PointsToOverload - process overloaded -> operator
 *
 *      fSuccess = PointsToOverload (bn)
 *
 *      Entry   bn = based pointer to operator node
 *              ST = pointer to operand (actual operand if pv is reference)
 *
 *      Exit    tree rewritten to function call and bound
 *
 *      Returns TRUE if tree rewritten and bound correctly
 *              FALSE if error
 */


bool_t
PointsToOverload (
    bnode_t bn
    )
{
    pExState->err_num = ERR_OVLPOINTSTO;
    return (FALSE);
}




/**     DupETree - Duplicate Expression Tree
 *
 *      hNew = DupETree (count, ppTree)
 *
 *      Entry   count = number of free bytes required in new expression tree
 *              ppTree = pointer to expression tree address
 *
 *      Exit    Current expression tree duplicated
 *              ppTree = address of locked expression tree
 *              additional memory cleared
 *
 *      Returns 0 if expression tree not duplicated
 *              memory handle if expression tree duplicated
 */

HDEP
DupETree (
    ulong  count,
    pstree_t *ppTree
    )
{
    HDEP        hNew;

    // copy syntax tree

    if ((hNew = MemAllocate (pTree->node_next + count)) == 0) {
        pExState->err_num = ERR_NOMEMORY;
        return (hNew);
    }
    *ppTree = (pstree_t)MemLock (hNew);
    memcpy (*ppTree, pTree, pTree->node_next);
    (*ppTree)->size = pTree->node_next + count;
    memset ((char *)*ppTree + (*ppTree)->node_next, 0, count);
    return (hNew);
}




/**     Type and context parsing
 *
 */



/**     FcnCast - check to see if function call is a functional style cast
 *
 *      fSuccess = FcnCast (bn)
 *
 *      Entry   bn = based pointer to OP_function node which has exactly
 *              one argument node.
 *
 *      Exit    the OP_function node is changed to an OP_cast node
 *
 *      Returns TRUE if the "function name" is a primitive type or a UDT
 *              and the tree was rewritten as an OP_cast
 *              FALSE if the function is not a cast node
 */


bool_t FASTCALL
FcnCast (
    bnode_t bn
    )
{
    peval_t     pv;
    bnode_t     bnLeft = NODE_LCHILD (bn);

    // Check for casting a class to anything, not having a symbol or
    // the symbol not being a type

    if (EVAL_IS_CLASS (ST)) {
        pExState->err_num = ERR_CONSTRUCTOR;
        return (FALSE);
    }
    if (EVAL_IS_BITF (STP)) {
        // change the type of the node to the underlying type
        EVAL_TYP (STP) = BITF_UTYPE (STP);
    }
    NODE_OP (bn) = OP_cast;
    NODE_RCHILD (bn) = NODE_LCHILD (NODE_RCHILD (bn));

    // copy the base type node up to the cast node and then try to find a
    // way to cast the stack to to the base type

    pv = (peval_t)&bnLeft->v[0];
    PopStack ();
    if (CastPtrToPtr (bn) == TRUE) {
        // the desired type is a base class so we can just set the node type.
        // the value portion of bn contains the data to cast right to left

        return (SetNodeType (ST, EVAL_TYP (pv)));
    }
    else {
        return (CastNode (ST, EVAL_TYP (pv), PTR_UTYPE (pv)));
    }
}




/**     GetID - get identifier length from string
 *
 *      len = GetID (pb)
 *
 *      Entry   pb = pointer to string
 *
 *      Exit    none
 *
 *      Returns length of next token
 *              if *pb is a digit, len = 0
 */


uchar FASTCALL
GetID (
    char *pb
    )
{
    char   *start = pb;
    char        c = *pb;

    pb = _tcsinc (pb);
    if (_istdigit ((_TUCHAR)c))
        return (0);
    if ((c == '*') || (c == '&'))
        return (1);
    while (_istcsym((_TUCHAR)c) || c == '$' || c == '@'
        // allow nested class names
        // ParseTypeName should have validated the type string,
        // so a ':' character should belong to a bscope operator
        || c == ':') {
        c = *pb++;
    }

    if (c == '<') {
        // we have a template class name
        // find matching '>'
        int count=1;
        char *pbsav = pb;
        while((c = *pb) != 0) {
            pb = _tcsinc(pb);
            if (c == '<')
                count++;
            else if (c == '>'&& --count == 0) {
                return (uchar)(pb-start);
            }

        }
        // if no matching '>' exists,
        // restore original pointer
        pb = pbsav;
    }

    /* return length of string */
    return ((uchar)(pb - start - 1));
}


/**     ParseType - parse a type string
 *
 *      fSuccess = ParseType (bn)
 *
 *      Entry   bn = based pointer to node referencing "(typestring)"
 *
 *      Exit
 *
 *      Returns TRUE if valid type string
 *              FALSE if error in type string
 */


bool_t FASTCALL
ParseType (
    bnode_t bn,
    bool_t fExact
    )
{
    pnode_t     pn = (pnode_t)bn;
    char   *pb;
    char   *pbEnd;
    peval_t     pv;
    bool_t      cmpflag;
    uchar       len;
    ulong       mask = 0;
    CV_typ_t    type = 0;
    ulong       mode = 0;
    ulong       btype = 0;
    ulong       size = 0;
    struct typrec *p;
    eval_t      evalT;
    peval_t     pvT = &evalT;
    CV_modifier_t    Mod = {0};
    MTYP_t      retval;
    DTI         dti;

    memset ( &evalT, 0, sizeof(evalT) );    // can't use struct init, C8/16 bug
    pb = pExStr + EVAL_ITOK (&pn->v[0]);
    pbEnd = pb + EVAL_CBTOK (&pn->v[0]);
    if (*pb == '(') {
        pb++;
        pbEnd--;
    }
    pv = &pn->v[0];
    EVAL_TYPDEF (pv) = 0;
    for (;;) {
        while (_istspace ((_TUCHAR)*pb))
            ++pb;
        if (pb >= pbEnd) {
            // end of type string
            break;
        }
        if (*pb == 0) {
            goto typebad;
        }
        len = GetID (pb);

        if (len > (pbEnd - pb)) len = (uchar)(pbEnd - pb);

        if ((cmpflag = len) == 0) {
            goto typebad;
        }
        else {
            for (p = Predef; p->token[0] != 0; p++) {
                if ((p->token[0] == len) &&
                  ((cmpflag = _tcsncmp ((char *)&p->token[1], pb, len)) == 0)) {
                    break;
                }
            }
            if (cmpflag == 0) {
                // a predefined token was encountered
                mask |= p->flags;
            }
            else {
                if (((mask & TY_UDT) != 0) ||
                  !FindUDT (bn, pvT, pExStr, pb, len)) {
                    // we either already have a UDT or we could not
                    // find this one
                    goto typebad;
                }
                else {
                    mask |= TY_UDT;
                }
            }
            // skip to end of token and continue
            pb += len;
        }
    }
#if defined(TARGMAC68K)
    type = EVAL_TYP (pvT);
#endif
    // check error conditions  At this point we are checking obvious errors
    // such as a user defined type without a type index, multiple pointer modes,
    // no valid type specifiers, mixed arithmetic types

    if (mask == 0) {
        // no type specifiers found
        goto typebad;
    }

    if (mask & (TY_POINTER|TY_REF)) {
        switch (mask & (TY_NEAR|TY_FAR|TY_HUGE)) {
            case 0:
                // set ambiant model from compile flag symbol
                switch (SetAmbiant (TRUE)) {
                    default:
                    case CV_PTR_NEAR:
                        mask |= TY_NEAR;
                        mode = CV_TM_NPTR;
                        break;

                    case CV_PTR_FAR:
                        mask |= TY_FAR;
                        mode = CV_TM_FPTR;
                        break;

                    case CV_PTR_NEAR32:
                        mask |= TY_NEAR;
                        mode = CV_TM_NPTR32;
                        break;

                    case CV_PTR_FAR32:
                        mask |= TY_FAR;
                        mode = CV_TM_FPTR32;
                        break;

                    case CV_PTR_HUGE:
                        mask |= TY_HUGE;
                        mode = CV_TM_HPTR;
                        break;

                    case CV_PTR_64:
                        mask |= TY_NEAR;
                        mode = CV_TM_NPTR64;
                        break;
                }
                break;

            case TY_NEAR:
                mode = CV_TM_NPTR32;
                break;

            case TY_FAR:
                mode = CV_TM_FPTR32;
                break;

            case TY_HUGE:
                mode = CV_TM_HPTR;
                break;

            default:
                // pointer mode conflict
                goto typebad;
        }
    }
    else {
        mode = CV_TM_DIRECT;
    }
    switch (mask & (TY_AGGR | TY_UDT)) {
        case TY_UDT:
        case TY_UDT | TY_CLASS:
        case TY_UDT | TY_STRUCT:
        case TY_UDT | TY_UNION:
        case 0:
             break;

        default:
            // conflict in aggregrate type
            goto typebad;
    }
    if (((mask & TY_REAL) != 0) && ((mask & TY_NOTREAL) != 0)) {
        // real type specified with conflicting modifiers
        goto typebad;
    }
    if (((mask & TY_UDT) != 0) && ((mask & TY_ARITH) != 0)) {
        // user defined type and arithmetic type specified
        goto typebad;
    }
    if ((mask & TY_SIGN) == TY_SIGN) {
        // both sign modifers specified
        goto typebad;
    }

    if ((mask & TY_UDT) != 0) {
        // user defined type specified
        int savedTok = EVAL_ITOK(pv);
        int savedCb = EVAL_CBTOK(pv);
        *pv = *pvT;
        //restore original start and length --gdp 8-27-92
        EVAL_ITOK(pv) = savedTok;
        EVAL_CBTOK(pv) = savedCb;

        if (CV_IS_PRIMITIVE (EVAL_TYP (pvT))) {
            // if the user defined type is an alias for a primitive type
            // set the pointer mode bits into the type

            type = (CV_typ_t)(EVAL_TYP(pvT) | (mode << CV_MSHIFT));
        }
        else {
            if ((mask & (TY_PTR | TY_REF)) == 0) {
                //  the UDT was not modified to a pointer or reference
                type = EVAL_TYP (pvT);
            }
            else {
                // the UDT was modified to a pointer. try to find the
                // correct pointer type

                if ((mask & TY_CONST) == TY_CONST) {
                    Mod.MOD_const = TRUE;
                }
                else if ((mask & TY_VOLATILE) == TY_VOLATILE) {
                    Mod.MOD_volatile = TRUE;
                }

                ProtoPtr (pvT, pv, !!(mask & TY_REF), Mod);

                switch (mask & (TY_NEAR|TY_FAR|TY_HUGE)) {

#if !defined(TARGMAC68K)
                    case 0:
                        // set ambiant model from compile flag symbol
                        EVAL_PTRTYPE (pvT) = (uchar)SetAmbiant (TRUE);
                        break;

                    case TY_NEAR:
                        EVAL_PTRTYPE (pvT) = CV_PTR_NEAR32;
                        break;

                    case TY_FAR:
                        EVAL_PTRTYPE (pvT) = CV_PTR_FAR32;
                        break;

                    case TY_HUGE:
                        EVAL_PTRTYPE (pvT) = CV_PTR_HUGE;
                        break;
#else
                    default:
                        // set ambiant model from compile flag symbol
                        EVAL_PTRTYPE (pvT) = (uchar)SetAmbiant (TRUE);
                        break;
#endif
                }

                retval = MatchType(pvT, fExact);

                switch (retval) {
                    case MTYP_exact:
                    case MTYP_inexact:
                        // searching the context of the class type for
                        // a type record which is a pointer record and
                        // has the current type as its underlying type
                        // has succeeded

                        type = EVAL_TYP (pvT);
                        break;

                    case MTYP_none:
                        // fake out the caster by using a created pointer
                        switch (mask & TY_PTR) {
                            case TY_POINTER:
                                type = T_32NCVPTR;
                                break;

                            case TY_POINTER | TY_NEAR:
                                type = T_32NCVPTR;
                                break;

                            case TY_POINTER | TY_FAR:
                                type = T_32FCVPTR;
                                break;

                            case TY_POINTER | TY_HUGE:
                                type = T_HCVPTR;
                                break;

                            default:
                                goto typebad;
                        }
                        break;
                }
            }
        }
    }
    else {
        // type must be primitive or a pointer to a primitive type

        if (!BuildType (&type, &mask, &mode, &btype, &size)) {
            goto typebad;
        }
        if ((mask & (TY_REF | TY_CONST | TY_VOLATILE)) != 0) {
            // the primitive type was modified to a const, volatile or
            // reference.  We will need to search for a type record that
            // has the primitive type as the underlying type

            SetNodeType (pv, type);
            EVAL_MOD (pv) = SHHMODFrompCXT (pCxt);
            *pvT = *pv;
            if ((mask & TY_REF) != 0) {
                ProtoPtr (pvT, pv, ((mask & TY_REF) == TY_REF), Mod);

                switch (mask & (TY_NEAR|TY_FAR|TY_HUGE)) {
#if !defined(TARGMAC68K)
                    case 0:
                        // set ambiant model from compile flag symbol
                        EVAL_PTRTYPE (pvT) = (uchar)SetAmbiant (TRUE);
                        break;

                    case TY_NEAR:
                        EVAL_PTRTYPE (pvT) = CV_PTR_NEAR32;
                        break;

                    case TY_FAR:
                        EVAL_PTRTYPE (pvT) = CV_PTR_FAR32;
                        break;

                    case TY_HUGE:
                        EVAL_PTRTYPE (pvT) = CV_PTR_HUGE;
                        break;
#else
                    default:
                        // set ambiant model from compile flag symbol
                        EVAL_PTRTYPE (pvT) = (uchar)SetAmbiant (TRUE);
                        break;
#endif

                }
            }

            if (mask & TY_CONST)
                EVAL_IS_CONST (pvT) = TRUE;

            if (mask & TY_VOLATILE)
                EVAL_IS_VOLATILE (pvT) = TRUE;

            retval = MatchType (pvT, fExact);
            switch (retval) {
                case MTYP_exact:
                case MTYP_inexact:
                    // searching the context of the class type for
                    // a type record which is a pointer record and
                    // has the current type as its underlying type
                    // has succeeded

                    type = EVAL_TYP (pvT);
                    break;

                case MTYP_none:
                    goto typebad;
            }
        }
    }
    EVAL_STATE (pv) = EV_type;
    if (!SetNodeType (pv, type)) {
        goto typebad;
    }

    return TRUE;

typebad:
    // If not "(type-name)"
    pExState->err_num = ERR_TYPECAST;
    return (FALSE);
}


bool_t FASTCALL
BuildType (
    CV_typ_t *type,
    ulong *mask,
    ulong  *mode,
    ulong  *btype,
    ulong  *size
    )
{
    // type must be primitive or a pointer to a primitive type

    if (!(*mask & (TY_VOID|TY_REAL|TY_INTEGRAL|TY_SEGMENT))) {
        // no type specified so set default to short
        if ( pExState->state.f32bit ) {
            *mask |= TY_LONG;
        }
        else {
            *mask |= TY_SHORT;
        }
    }

    if (*mask & TY_REAL) {
        *btype = CV_REAL;
        switch (*mask & (TY_REAL | TY_LONG)) {
            case TY_FLOAT:
                *size = CV_RC_REAL32;
                break;

            case TY_DOUBLE:
                *size = CV_RC_REAL64;
                break;

            case TY_DOUBLE | TY_LONG:
                if ( pExState->state.f32bit ) {
                    *size = CV_RC_REAL64;
                }
                else {
                    *size = CV_RC_REAL80;
                }
                break;

            default:
                DASSERT (FALSE);
                return (FALSE);
        }
    }
    else if (*mask & TY_INTEGRAL) {
        if (*mask & TY_INT) {
            *btype = CV_INT;
            // user specified int possibly along with sign and size
            switch (*mask & (TY_SIGN | TY_SHORT | TY_LONG)) {

                case 0:
                case TY_SIGNED:
                    if ( pExState->state.f32bit ) {
                        *size = CV_RI_INT4;
                    }
                    else {
                        *size = CV_RI_INT2;
                    }
                    break;

                case TY_UNSIGNED:
                    if ( pExState->state.f32bit ) {
                        *size = CV_RI_UINT4;
                    }
                    else {
                        *size = CV_RI_UINT2;
                    }
                    break;

                case TY_SHORT:
                case TY_SHORT | TY_SIGNED:
                    // set default integral types to signed two byte int
                    *size = CV_RI_INT2;
                    break;

                case TY_SHORT | TY_UNSIGNED:
                    // set default integral types to signed two byte int
                    *size = CV_RI_UINT2;
                    break;

                case TY_LONG:
                case TY_LONG | TY_SIGNED:
                    // set default integral types to signed two byte int
                    *size = CV_RI_INT4;
                    break;

                case TY_LONG | TY_UNSIGNED:
                    // set default integral types to signed two byte int
                    *size = CV_RI_UINT4;
                    break;

                default:
                    DASSERT (FALSE);
                    return (FALSE);
            }
        }
        else if ((*mask & TY_CHAR) != 0) {
            // user specified a character type

            switch (*mask & TY_SIGN) {
                case 0:
                    // if no sign was specified, we are looking for a
                    // real character

                    *btype = CV_INT;
                    *size = CV_RI_CHAR;
                    break;

                case TY_SIGNED:
                    *btype = CV_SIGNED;
                    *size = CV_IN_1BYTE;
                    break;

                case TY_UNSIGNED:
                    *btype = CV_UNSIGNED;
                    *size = CV_IN_1BYTE;
                    break;
            }
        }
        else {
            switch (*mask & TY_SIGN) {
                case 0:
                    // set default integral types to signed
                    *mask |= TY_SIGNED;
                case TY_SIGNED:
                    *btype = CV_SIGNED;
                    break;

                case TY_UNSIGNED:
                    *btype = CV_UNSIGNED;
                    break;
            }
            switch (*mask & TY_INTEGRAL) {
                case TY_CHAR:
                    *size = CV_IN_1BYTE;
                    break;

                case TY_SHORT:
                    *size = CV_IN_2BYTE;
                    break;

                case TY_LONG:
                    *size = CV_IN_4BYTE;
                    break;

                case TY_QUAD:
                    *size = CV_IN_8BYTE;
                    break;
            }
        }

    }
    else if ((*mask & TY_VOID) != 0) {
        if (*mask & (TY_ARITH | TY_REAL | TY_AGGR | TY_SIGN)) {
            return (FALSE);
        }
        *btype = CV_SPECIAL;
        *size  = CV_SP_VOID;
    }
    else if (*mask & TY_SEGMENT) {
        if ( pExState->state.f32bit ||
          ( *mask & (TY_ARITH | TY_REAL | TY_AGGR | TY_SIGN) ) ) {
            return (FALSE);
        }
        *btype = CV_SPECIAL;
        *size  = CV_SP_SEGMENT;
    }
    else {
        DASSERT (FALSE);
        return (FALSE);
    }

    // reference types do not have the mode bits in the type that they reference
    if (*mask & TY_REF)
        *type =  (CV_typ_t)((*btype << CV_TSHIFT) | (*size << CV_SSHIFT));
    else
        *type = (CV_typ_t)((*mode << CV_MSHIFT) | (*btype << CV_TSHIFT) | (*size << CV_SSHIFT));

    return (TRUE);
}




/**     FindUDT - find user defined type
 *
 *      fSuccess = FindUDT (bn, pv, pStr, pb, len)
 *
 *      Entry   pv = pointer to evaluation node
 *              pStr = pointer to beginning of input string
 *              pb = pointer to structure name
 *              len = length of name
 *
 *      Exit    EVAL_TYP (pv) = type index
 *
 *      Returns TRUE if UDT found
 *              FALSE if error
 *
 *      Looks in the current module only.
 */


bool_t
FindUDT (
    bnode_t bn,
    peval_t pv,
    char *pStr,
    char *pb,
    uchar len
    )
{
    search_t    Name;

    EVAL_TYP (pv) = 0;
    EVAL_ITOK (pv) = (ULONG)(pb - pStr);
    EVAL_CBTOK (pv) = len;

    // M00SYMBOL - need to allow for T::U::V::type

    InitSearchSym (bn, pv, &Name, 0, SCP_all & ~SCP_class, CLS_enumerate | CLS_ntype);
    // modify search to look only for UDTs

    Name.sstr.searchmask = SSTR_symboltype;
    Name.sstr.symtype = S_UDT;
    switch (SearchSym (&Name)) {
        case HR_found:
            // if the symbol was found, it was pushed onto the stack
            PopStack ();
            if (EVAL_STATE (pv) == EV_type) {
                return (TRUE);
            }
            break;

        case HR_rewrite:
            DASSERT (FALSE);

        default:
            break;
    }
    return (FALSE);
}




/**     ContextToken - find next comma separated context token
 *
 *      fSuccess = ContextToken (ppStr, ppTok, pcTok)
 *
 *      Entry   ppStr = address of pointer to string
 *              ppTok = address of pointer to first character of token
 *              pcTok = address of character count of token
 *
 *      Exit    *ppTok = address of first character of token with the
 *                      enclosing () stripped off
 *              *pcTok = count of characters in token with enclosing ()
 *                      stripped off.  If *pcTok is zero, then the token was
 *                      null.  If *pcTok is -1, then the token was not
 *                      specified before the }.
 *              *ppStr = pointer to first character of next token
 *
 *      Returns TRUE if no error
 *              FALSE if error
 */


bool_t FASTCALL
ContextToken (
    char **ppStr,
    char **ppTok,
    int   *pcTok
    )
{
    bool_t      parenopen = FALSE;
    int         pdepth = 0;
    char        ch;
    int         cTokSav;

    while ((**ppStr == ' ') || (**ppStr == '\t')) {
        (*ppStr)++;
    }
    if (**ppStr == '}') {
        // if end of string, return -1 to indicate no token
        *pcTok = -1;
        return (TRUE);
    }

    *pcTok = 0;
    if (**ppStr == '(') {
        // if the first character of the token is a (, then the token is the
        // string enclosed in the ()

        parenopen = TRUE;
        (*ppStr)++;
        pdepth = 1;
    }

    // if the first character of the token is an open quote, then the token
    // is simply the string enclosed in the quotes.
    if (**ppStr == '\"') {
        (*ppStr)++;

        *ppTok = *ppStr;
        while (( ch = *(*ppStr)) != 0)
        {
            if (ch == '\"')
            {
                do {
                    *ppStr = _tcsinc (*ppStr);
                } while ((ch = *(*ppStr)) != 0 && ch != ',' && ch != '}');

                if (ch==0) {
                    return FALSE;
                } else if (ch == ',') {
                    // leave pointer past comma so next scan will find following token
                    *ppStr = _tcsinc (*ppStr);
                    return TRUE;
                } else if (ch == '}') {
                    // leave pointer before the curly so next scan will find it
                    return TRUE;
                }
            }

            // increment count of characters in token
            *pcTok += _tclen(*ppStr);
            *ppStr = _tcsinc (*ppStr);

        }

        // ch == 0 ==> no close quote
        DASSERT(ch == 0);
        return FALSE;
    }

    *ppTok = *ppStr;
    while ((ch = *(*ppStr)) != 0) {
        // increment count of characters in token
        *pcTok += _tclen(*ppStr);
        *ppStr = _tcsinc (*ppStr);
        switch (ch) {
            case '(':
                // increment parentheses depth
                pdepth++;
                if (pdepth == 1) {
                    // save length of token that precedes
                    // the opening parenthesis
                    // (parenopen is FALSE in this case)
                    cTokSav = *pcTok - 1;
                }
                break;

            case ')':
                if (--pdepth < 0) {
                    // unbalanced parentheses
                    return (FALSE);
                }
                else if (pdepth == 0) {
                    if (parenopen) {
                        // for a parentheses enclosed string, adjust count and
                        // skip blanks to either , or } that terminates the
                        // token.  Any other character is an error
                        (*pcTok)--;
                        while ((**ppStr == ' ') || (**ppStr == '\t')) {
                            (*ppStr)++;
                        }
                        switch (**ppStr) {
                            case ',':
                                // skip over the , terminating the token
                                (*ppStr)++;

                            case '}':
                                return (TRUE);

                            default:
                                return (FALSE);
                        }
                    }
                    else {
                        // this allows parsing a function name that
                        // contains an argument list in the form generated
                        // by FormatCXT, but ignores the arg list, since
                        // the EE cannot disambiguate properly.
                        (*pcTok) = cTokSav;

                        while ((**ppStr == ' ') || (**ppStr == '\t')) {
                            (*ppStr)++;
                        }
                        switch (**ppStr) {
                            case ',':
                                // skip over the , terminating the token
                                (*ppStr)++;

                            case '}':
                                return (TRUE);

                            default:
                                return (FALSE);
                        }
                    }
                }
                break;

            default:
                if (pdepth > 0) {
                    // any character inside parentheses is ignored
                    break;
                }
                else if (ch == '}') {
                    // decrement character count of token and reset pointer to }
                    // so next scan will find it
                    (*ppStr)--;
                    (*pcTok)--;
                    return (TRUE);
                }
                else if (ch == ',') {
                    // decrement character count of token but leave pointer past comma
                    // so next scan will find following token
                    (*pcTok)--;
                    return (TRUE);
                }
                break;
        }
    }
    return (FALSE);
}





/**     InitSearchSym - initialize symbol search
 *
 *      InitSearchSym (bn, pName, iClass, scope, clsmask)
 *
 *      Entry   bn = based pointer to node of symbol
 *              pName = pointer to symbol search structure
 *              iClass = initial class if explicit class reference
 *              scope = mask describing scope of search
 *              clsmask = mask describing permitted class elements
 *
 *      Exit    search structure initialized for SearchSym
 *
 *      Returns pointer to search symbol structure
 */


void
InitSearchSym (
    bnode_t bn,
    peval_t pv,
    psearch_t pName,
    CV_typ_t iClass,
    ulong  scope,
    ulong  clsmask
    )
{
    op_t    op = NODE_OP (bn);

    // set starting context for symbol search to current context

    memset (pName, 0, sizeof (*pName));
    pName->initializer = INIT_sym;
    pName->pfnCmp = (PFNCMP) FNCMP;
    pName->pv = pv;
    pName->scope = scope;
    pName->clsmask = clsmask;
    pName->CXTT = *pCxt;
    pName->bn = bn;
    pName->bnOp = 0;

    // set pointer to symbol name

    if ((op >= OP_this) && (op <= OP_Odelete)) {
        pName->sstr.lpName = (uchar *)&OpName[op - OP_this].str[1];
        pName->sstr.cb = OpName[op - OP_this].str[0];
    }
    else {
        pName->sstr.lpName = (uchar *)pExStr + EVAL_ITOK (pv);
        pName->sstr.cb = (uchar)EVAL_CBTOK (pv);
    }
    pName->state = SYM_init;
    if ((pName->ExpClass = iClass) != 0) {
        // restrict searching to class scope
        pName->scope &= SCP_class;
    }
}




/**     InitSearchRight - initialize right symbol search
 *
 *      InitSearchRight (bnOp, bn, pName, clsmask)
 *
 *      Entry   bnOp = based pointer to node of operator
 *              bn = based pointer to node of symbol
 *              pName = pointer to symbol search structure
 *              iClass = initial class if explicit class reference
 *              scope = mask describing scope of search
 *              clsmask = mask describing permitted class elements
 *
 *      Exit    search structure initialized for SearchSym
 *
 *      Returns pointer to search symbol structure
 */


void
InitSearchRight (
    bnode_t bnOp,
    bnode_t bn,
    psearch_t pName,
    ulong  clsmask
    )
{
    peval_t     pv = &bn->v[0];
    op_t        op = NODE_OP (bn);

    // set starting context for symbol search to current context

    memset (pName, 0, sizeof (*pName));
    pName->initializer = INIT_right;
    pName->pfnCmp = (PFNCMP) FNCMP;
    pName->pv = pv;
    pName->scope = SCP_class;
    pName->clsmask = clsmask;
    pName->CXTT = *pCxt;
    pName->bn = bn;
    pName->bnOp = bnOp;

    // set pointer to symbol name
    if ((op >= OP_this) && (op <= OP_Odelete)) {
        pName->sstr.lpName = (uchar *)&OpName[op - OP_this].str[1];
        pName->sstr.cb = OpName[op - OP_this].str[0];
    }
    else {
        pName->sstr.lpName = (uchar *)pExStr + EVAL_ITOK (pv);
        pName->sstr.cb = (char)EVAL_CBTOK (pv);
    }
    pName->state = SYM_init;
    // restrict searching to class scope
    pName->ExpClass = ClassExp;
    pName->scope = SCP_class;
}


/**     TranslateClassIndex - translate class index of another dll to
 *              a class index of the current execution module
 *
 *      CV_typ_t type = TranslateClassIndex(typ, hMod)
 *
 *      Entry   typ: the class type index to be translated
 *              hMod: the module in which typ resides
 *
 *      Exit
 *
 *      Returns 0 if no corresponding class exists in the current context
 *              otherwise the corresponding type index is returned
 */

CV_typ_t
TranslateClassIndex(
    CV_typ_t typ,
    HMOD hMod
    )
{
    HTYPE       hType;
    plfEasy     pType;
    search_t    Name;
    psearch_t   pName = &Name;
    eval_t      Eval;
    peval_t     pv = &Eval;
    char        buf[NAMESTRMAX];
    uint        len;
    uint        skip;

    DASSERT(!CV_IS_PRIMITIVE(typ))
    DASSERT(hMod != 0)
    if (SHHexeFromHmod( hMod ) == SHHexeFromHmod( pCxt->hMod )) {
        // type exists in the current .exe or .dll so
        // no translation is required
        return typ;
    }

    if ((hType = THGetTypeFromIndex (hMod, typ)) == 0) {
        return 0;
    }

    // get the class name
retry:
    pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
    switch (pType->leaf) {
        case LF_MODIFIER:
            if ((hType = THGetTypeFromIndex (hMod, ((plfModifier)pType)->type)) == 0) {
                return 0;
            }
            goto retry;

        case LF_STRUCTURE:
        case LF_CLASS:
            skip = offsetof (lfClass, data);
            RNumLeaf (((unsigned char *)(&pType->leaf)) + skip, &skip);
            len = *(((unsigned char *)&(pType->leaf)) + skip);
            _tcsncpy (buf, ((char *)pType) + skip + 1, len);
            MHOmfUnLock (hType);
            break;

        default:
            return 0;
    }

    // look for a UDT with the same name in the current context
    memset (pName, 0, sizeof (*pName));
    memset (pv, 0, sizeof (*pv));
    pName->initializer = INIT_sym;
    pName->pfnCmp = (PFNCMP) FNCMP;
    pName->pv = pv;
    // UDTs are global symbols -- restrict the scope of the search
    pName->scope = SCP_module | SCP_global;
    pName->CXTT = *pCxt;
    pName->state = SYM_init;
    pName->sstr.lpName = (uchar *) buf;
    pName->sstr.cb = (uchar)len;
    Name.sstr.searchmask = SSTR_symboltype;
    Name.sstr.symtype = S_UDT;

    if (SearchSym(pName) != HR_found)
        return 0;

    PopStack(); // remove found symbol from stack
    return (EVAL_TYP(pv));
}
