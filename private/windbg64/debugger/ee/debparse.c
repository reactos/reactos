/***    DEBPARSE.C - Main parser module for C expression evaluator
 *
 *       Operator precedence parser for parsing arbitrary (almost)
 *       C expressions.
 */


/*  The parser deals with nearly all C operators, with the exception of ?:
 *  operator.
 *
 *  The parser is an operator precedence parser; because of the large
 *  number of operators, an operator precedence function has been designed,
 *  and is used instead of the precedence matrix.  See "Compilers, principles,
 *  techniques, and tools" (the 'Dragon' book) by Aho, Sethi and Ullman,
 *  section 4.6.
 *
 *  Five operators (::, +, -, & and *) are ambiguous; they can be used in
 *  either the unary or binary sense.  Again, the main parsing loop cannot
 *  tell the difference; therefore, we keep track of the previous token:  if it
 *  is id, const, ) or ], an ambiguous token is interpreted as being binary;
 *  otherwise, it is unary.  Note that the lexer will always claim to have
 *  found the unary version of these three ops: '*' will always be returned
 *  as OP_fetch, and the parser will convert to OP_mult if necessary.
 */

#include "debexpr.h"
#ifndef _SBCS
#include <tchar.h>
#endif

//  Size of shift-reduce stack, below.

#define SRSTACKSIZE     30
#define SRSTACKGROW     10
#define FCNDEPTH         5


// Macros for pushes and pops from the shift-reduce stack.

#define SRPUSH(tok) (pSRStack[--SRsp] = (tok))
#define SRCUR()     (pSRStack[SRsp])
// SRPOP() has been turned into a function
#define SRPREV()    (pSRStack[SRsp+1])
#define SRPPREV()   (pSRStack[SRsp+2])
#define SRSIZE()    (maxSRsp - SRsp)


// Precedence function arrays.

uchar SEGBASED(_segname("_CODE")) F[COPS_EXPR] =
{
#define OPCNT(name, val)
#define OPCDAT(opc)
#define OPDAT(op, opfprec, opgprec, opclass, opbind, opeval) opfprec,
#include "debops.h"
#undef OPDAT
#undef OPCDAT
#undef OPCNT
};

uchar SEGBASED(_segname("_CODE")) G[COPS_EXPR] =
{
#define OPCNT(name, val)
#define OPCDAT(opc)
#define OPDAT(op, opfprec, opgprec, opclass, opbind, opeval) opgprec,
#include "debops.h"
#undef OPDAT
#undef OPCDAT
#undef OPCNT
};

typedef enum PTN_flag {
    PTN_error,          // error encountered
    PTN_nottype,        // not a type
    PTN_typestr,        // type string (type...)
    PTN_typefcn,        // (type)(....
    PTN_formal          // ...type, or ...type)
} PTN_flag;

HDEP        hSRStack = 0;
ulong       maxSRsp = SRSTACKSIZE;
token_t FAR *pSRStack;
ulong       SRsp;
ulong       argcnt[FCNDEPTH];
int         fdepth;

ulong  CvtOp (ptoken_t, ptoken_t, ulong );
ulong  CheckErr (op_t, op_t, ulong );
bool_t GrowSR (void);
EESTATUS FParseExpr (uint, bool_t);
PTN_flag ParseTypeName (ptoken_t, char FAR *, uint, EESTATUS FAR *);
void ParseContext (ptoken_t, EESTATUS FAR *);
EESTATUS XFormExpr (uint);

ulong  GetToken (uchar FAR*, ptoken_t, uint, op_t);

static uchar    SEGBASED(_segname("_CODE")) SimpleTypeName[][11] = {
     "\x006""signed",
     "\x008""unsigned",
     "\x004""void",
     "\x004""char",
     "\x003""int",
     "\x005""short",
     "\x004""long",
     "\x005""float",
     "\x006""double",
     "\x008""_segment",
     "\x009""__segment",
     "\x000"
};



token_t
SRPOP()
{
    token_t ptoken_t = pSRStack[SRsp];
    ZeroMemory(&pSRStack[SRsp], sizeof(token_t));
    SRsp++;

    return ptoken_t;
}


/**     Parse - parse expression string to abstract syntax tree
 *
 *      ulong  Parse (szExpr, radix, fCase, phTM);
 *
 *      Entry   szExpr = pointer to expression string
 *              radix = default number radix for conversions
 *              fCase = case sensitive search if TRUE
 *              fXForm = allow transforming template names to canonical form
 *              phTM = pointer to handle of expression state structure
 *              pEnd = pointer to ulong  for index of char that ended parse
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
Parse(
    const char *szExpr,
    uint radix,
    SHFLAG fCase,
    SHFLAG fXForm,
    PHTM phTM,
    ulong  *pEnd
    )
{
    ulong       len = 0;

    /* DBPrintf("Parsing %s\r\n", szExpr);  [debug helper] */

    if ((*phTM = MemAllocate (sizeof (struct exstate_t))) == 0) {
        return (EECATASTROPHIC);
    }

    // lock expression state structure, clear and allocate components

    pExState = (pexstate_t)MemLock (*phTM);
    memset (pExState, 0, sizeof (exstate_t));

    // allocate buffer for input string and copy

#ifdef NEVER
    // Diabled. HSYM binary data are now embedded in encoded ascii form,
    // so no special treatment is required to find the string length
    for (; *p != 0; p++) {
        if (*p == HSYM_MARKER) {
            p += sizeof (HSYM);
        }
    }
#endif
    pExState->ExLen = _tcslen (szExpr);
    if ((pExState->hExStr = MemAllocate (pExState->ExLen + 1)) == 0) {
        // clean up after error in allocation of input string buffer
        MemUnLock (*phTM);
        EEFreeTM (phTM);
        return (EECATASTROPHIC);
    }
    memcpy (MemLock (pExState->hExStr), szExpr, pExState->ExLen + 1);
    MemUnLock (pExState->hExStr);
    MemUnLock (*phTM);
    return (DoParse (phTM, radix, fCase, fXForm, pEnd));
}


/***    DoParse - parse expression
 *
 *      error = DoParse (phTM, radix, fCase, pEnd)
 *
 *      Entry   phTM = pointer to handle to TM
 *              radix = numberic radix for conversions
 *              fCase = case sensitive search if TRUE
 *              fXForm = allow transforming template names to canonical form
 *              pEnd = pointer to ulong for index of char that ended parse
 *
 *      Exit    expression parsed
 *              pExState->hExStr = handle of expression string
 *              pExState->ExLen = length of expression string
 *              *pEnd = index of character that terminated parse
 *
 *      Returns EENOERROR if expression parsed without error
 *              EECATASTROPHIC if parser was unable to initialize
 *              EEGENERAL if syntax error in expression
 */


EESTATUS
DoParse (
    PHTM phTM,
    uint radix,
    bool_t fCase,
    bool_t fXForm,
    ulong *pEnd
    )
{
    EESTATUS    error;
    ulong      len;
    DTI         dti;

    pExState = (pexstate_t)MemLock (*phTM);
    pExState->radix = radix;
    pExState->state.fCase = fCase;
    pExState->state.f32bit = TRUE;
    len = sizeof (stree_t) + NODE_DEFAULT + NSTACK_MAX * sizeof (bnode_t);
    if ((pExState->hSTree = MemAllocate (len)) == 0) {
        // clean up if error in allocation of syntax tree buffer
        MemUnLock (*phTM);
        EEFreeTM (phTM);
        return (EECATASTROPHIC);
    }
    else {
        memset ((pTree = (pstree_t) MemLock (pExState->hSTree)), 0, len);
        pTree->size = len;
        pTree->stack_base = sizeof (stree_t) + NODE_DEFAULT;
        // note that stack_next is zero from memset above
        pTree->node_next = pTree->start_node = offsetof (stree_t, nodebase);
    }

    // call the parser
    // note that the expression state structure and the abstract syntax tree
    // buffers are locked.

    if ((error = FParseExpr (radix, fXForm)) == EECATASTROPHIC) {
        // clean up if parser unable to initialize
        MemUnLock (pExState->hSTree);
        MemUnLock (*phTM);
        EEFreeTM (phTM);
    }

    //  compute the correct size of the abstract syntax tree buffer and
    //  reallocate the buffer to that size

    len = pTree->node_next;
    pTree->size = len;
    MemUnLock (pExState->hSTree);
    pExState->hSTree = MHMemReAlloc (pExState->hSTree, len);
    *pEnd = pExState->strIndex;
    MemUnLock (*phTM);
    return (error);
}




/***    FParseExpr - Parse a C expression
 *
 *      fSuccess = FParseExpr (radix);
 *
 *      Entry   radix = default numeric radix
 *              fXForm = allow transforming template names to canonical form
 *
 *      Exit    expression state structure updated
 *
 *      Returns EENOERROR if expression was successfully parsed
 *              EENOMEMORY if out of memory
 *              EEGENERAL if parse error
 *
 *      Description
 *              Lexes and parses the expression string into an abstract
 *              syntax tree.
 *
 */

EESTATUS
FParseExpr (
    uint radix,
    bool_t fXForm
    )
{
    EESTATUS    error = EENOERROR;
    ERRNUM      lexerr = ERR_NONE;
    token_t     tokOld;
    token_t     tokNext;
    token_t     tokT;
    int         pdepth = 0;
    char *szExpr;

    // allocate memory for shift-reduce stack if not already allocated
    if (hSRStack == 0) {
        UINT uSize = maxSRsp * sizeof (token_t);
        if ((hSRStack = MemAllocate(uSize)) == 0) {
            // unable to allocate space for shift reduce stack
            return (EECATASTROPHIC);
        } else {
            // Zero the memory out
            PVOID pv = MemLock(hSRStack);
            if (pv) {
                memset(pv, 0, uSize);
                MemUnLock(hSRStack);
            }
        }
    }
    pSRStack = (token_t *)MemLock (hSRStack);
    SRsp = maxSRsp;

    // lock the expression buffer.  Note that all exits from this
    // routine must unlock the expression buffer and unlock
    // the shift-reduce stack

    pExStr = (char *) MemLock (pExState->hExStr);

    // transform expression so that possible template names follow
    // predefined naming conventions

    if (fXForm && (error = XFormExpr(radix)) != EENOERROR) {
        MemUnLock (hSRStack);
        MemUnLock (pExState->hExStr);
        return error;
    }

    szExpr = pExStr;
    fdepth = 0;

    // push the lowest-precedence terminal token on the shift-reduce stack

    tokOld.opTok = OP_lowprec;
    SRPUSH (tokOld);

    // Fetch first token

    while ((*szExpr == ' ') || (*szExpr == '\t')) {
        szExpr++;
    }
    if (*szExpr == 0) {
        lexerr = ERR_SYNTAX;
        tokNext.opTok = OP_badtok;
    }
    else if ((lexerr = GetToken ((uchar *)szExpr, &tokNext, radix,
      SRCUR().opTok)) == ERR_NONE) {
        // compute the index of the start of the first token from
        // the beginning of the input
        tokNext.iTokStart = (ulong)(tokNext.pbTok - pExStr);
    }

    // process tokens from the input string until either a bad token is
    // encountered, an illegal combination of operators and operators occurrs
    // or the end of string is found.

    for (;;) {
kludge:
        if (tokNext.opTok == OP_badtok) {
            if (lexerr != ERR_NONE) {
                pExState->err_num = lexerr;
            }
            else {
                pExState->err_num = ERR_SYNTAX;
            }
            error = EEGENERAL;
            break;
        }
        if (error != EENOERROR) {
            if (pExState->err_num == ERR_NONE) {
                pExState->err_num = ERR_SYNTAX;
            }
            break;
        }
        if (SRsp == 0) {
            // shift/reduce stack overflow
            if (!GrowSR ()) {
                pExState->err_num = ERR_TOOCOMPLEX;
                error = EEGENERAL;
                break;
            }
        }

        // Change increment and decrement operators to pre or post form.
        // process opening parenthesis that is beginning of a function,
        // cast expression or sizeof expression

        if (tokNext.opTok == OP_incr) {
            if (F[SRCUR().opTok] <= G[OP_incr]) {
                tokNext.opTok = OP_preinc;
            }
            else {
                tokNext.opTok = OP_postinc;
            }
        }
        else if (tokNext.opTok == OP_decr) {
            if (F[SRCUR().opTok] <= G[OP_decr]) {
                tokNext.opTok = OP_predec;
            }
            else {
                tokNext.opTok = OP_postdec;
            }
        }
        else if (tokNext.opTok == OP_lcurly) {
            ParseContext (&tokNext, &error);
        }
        else if ((tokNext.opTok == OP_lparen) &&
          ((SRCUR().opTok == OP_ident) ||
          (tokOld.opTok == OP_rparen))) {

            // If the next token is a left parenthesis and the
            // shift/reduce top is an identifier or the previous
            // token was a right paren "(*pfcn)(...)

            tokNext.opTok = OP_function;
        }
        else if ((tokNext.opTok == OP_by || tokNext.opTok == OP_wo ||
              tokNext.opTok == OP_dw) &&
             (tokOld.opTok == OP_dot || tokOld.opTok == OP_pointsto)) {
            // by, wo, dw following a '.' or '->' are probably struct
            // members, not operators, so treat them as such (dolphin #977)
            tokNext.opTok = OP_ident;
        }
        else if ((tokNext.opTok == OP_lparen) ||
          ((tokNext.opTok == OP_ident) &&
            (SRCUR().opTok == OP_arg) && (fdepth > 0))) {
            // we possibly have either a type string of the form (type) or an
            // identifier which is the first token of an argument

            switch (ParseTypeName (&tokNext, szExpr, radix, &error)) {
                case PTN_nottype:
                case PTN_error:
                    break;

                case PTN_typestr:
                    if (SRCUR().opTok == OP_sizeof) {
                        // sizeof (type string)
                        tokNext.opTok = OP_typestr;
                    }
                    else {
                        // OP_cast is a unary op.  However, we will treat it as
                        // a binary op and put the type string onto the tree
                        // and change the current token to an OP_cast

                        tokT = tokNext;
                        tokT.opTok = OP_typestr;
                        if ((error = PushToken (&tokT)) != 0) {
                            break;
                        }
                        tokNext.opTok = OP_cast;
                    }
                    break;

                case PTN_typefcn:
                    if (SRCUR().opTok == OP_sizeof) {
                        // sizeof (type string)(.... is an error
                        pExState->err_num = ERR_NOOPERAND;
                        error = EEGENERAL;
                    }
                    else {
                        // we have something of the form (type string)(....
                        // which is a cast.  We will treat this as in the
                        // case above.

                        tokT = tokNext;
                        tokT.opTok = OP_typestr;
                        if ((error = PushToken (&tokT)) != 0)
                            break;
                        tokNext.opTok = OP_cast;
                    }
                    break;

                case PTN_formal:
                    // let parser push as an argument
                    break;

                default:
                    DASSERT (FALSE);
                    pExState->err_num = ERR_INTERNAL;
                    error = EEGENERAL;
                    break;
            }
        }
        if (error != 0) {
            break;
        }
        if ((tokOld.opTok == OP_function) || (tokOld.opTok == OP_lparen)) {
            // increment paren depth to detect proper nesting
            pdepth++;
        }
        if (tokOld.opTok == OP_function) {
            // increment function depth to allow comma terminated typestring
            // arguments.  Also initialize function argument counter
            argcnt[fdepth++] = 0;
            if (fdepth == FCNDEPTH) {
                error = EEGENERAL;
                lexerr = ERR_FCNTOODEEP;
            }
            if (tokNext.opTok != OP_rparen) {
                // insert token for first argument onto stack
                tokOld.opTok = OP_arg;
                SRPUSH(tokOld);
                argcnt[fdepth - 1]++;
                // this allows foo(const class &... and similar forms
                goto kludge;
            }
        }

        // Convert unary op to binary if necessary

        if ((pExState->err_num =
          CvtOp (&tokOld, &tokNext, pdepth)) != ERR_NONE) {
            error = EEGENERAL;
            break;
        }

        if (tokNext.opTok == OP_execontext) {
            //
            // there is an identifier on top of the stack
            // discard the identifier and keep the string in this token.
            //
            DASSERT(SRCUR().opTok == OP_ident);

            tokNext.pbTok = SRCUR().pbTok;
            tokNext.iTokStart = SRCUR().iTokStart;
            tokNext.cbTok = (uchar)(tokNext.pbEnd - tokNext.pbTok);
            szExpr = pExStr + tokNext.iTokStart;

            (void)SRPOP();
        }

        // check for scan termination

        if (((SRCUR().opTok == OP_lowprec) && (tokNext.opTok == OP_lowprec))) {
            if ((pTree->stack_next != 1) || (SRsp != (ulong)(maxSRsp - 1)) ||
              (pdepth != 0) || (fdepth != 0)) {
                // statement did not reduce to a single node
                pExState->err_num = ERR_SYNTAX;
                error = EEGENERAL;
            }
            else {
                pExState->strIndex = (ulong)(szExpr - pExStr);
                pExState->state.parse_ok = TRUE;
                if (pExState->hExStrSav) {
                    // compute value of strIndex for the saved string
                    pExState->strIndexSav = pExState->strIndex +
                        (pExState->ExLenSav - pExState->ExLen);
                }
            }
            break;
        }

        // process ) as either end of function or end of grouping

        if (tokNext.opTok == OP_rparen) {
            if ((SRCUR().opTok == OP_function) &&
              (argcnt[fdepth - 1] == 0)) {
                // For a function with no arguments, shift the end of
                // arguments token onto the stack and convert the right
                // parenthesis into the end of function token

                tokOld.opTok = OP_endofargs;
                SRPUSH(tokOld);
                tokNext.opTok = OP_fcnend;
                fdepth--;
            }
            else if (((SRCUR().opTok != OP_lparen) && (SRCUR().opTok != OP_arg))
               && ((SRSIZE() > 1) && SRPREV().opTok == OP_arg)) {

                // For a function with one or more arguments, pop the last
                // argument, shift the end of arguments token onto the stack
                // and then convert the right parenthesis into the end of function

                tokT = SRPOP();
                if (tokT.opTok != OP_grouped) {
                    if ((error = PushToken (&tokT)) != 0) {
                        break;
                    }
                }
                tokOld.opTok = OP_endofargs;
                SRPUSH(tokOld);
                tokNext.opTok = OP_fcnend;
                fdepth--;
            }
            else if ( ((SRSIZE() > 1) && (SRPREV().opTok == OP_function)) &&
              (SRCUR().opTok == OP_arg)) {
                tokOld.opTok = OP_endofargs;
                SRPUSH(tokOld);
                tokNext.opTok = OP_fcnend;
                fdepth--;
            }
            else if (
                // Decide if we are really at the end of a nested
                // function call. To do so we need to check the
                // top 3 stack elements and see if the OP_function
                // node is preceded by an OP_arg node (i.e., if the
                // current function serves as an argument to another
                // function call).
                // Improper check generated internal error (dolphin 3404)

              (SRSIZE() >= 3) &&
              (SRPPREV().opTok == OP_arg) &&
              (SRPREV().opTok == OP_function) &&
              (SRCUR().opTok == OP_fcnend)) {

                // handle nested function calls of the
                // form fun1(arg1, fun2(arg2)) --caviar #3678
                // treat this as a special case, otherwise
                // the nested function call will reduce and the
                // OP_arg node will become the stack top and
                // OP_endofargs will *not* be pushed on the stack.

                DASSERT (fdepth > 0); // must be a nested fcn call

                // "reduce" the inner function call and
                // push the function token on the parse tree
                tokT = SRPOP();
                tokT = SRPOP();
                if ((error = PushToken (&tokT)) != 0) {
                    break;
                }

                // "shift" the OP_endofargs token on the stack
                tokOld.opTok = OP_endofargs;
                SRPUSH(tokOld);
                tokNext.opTok = OP_fcnend;
                fdepth--;
            }
        }
        if ((pExState->err_num =
          CheckErr (SRCUR().opTok, tokNext.opTok, pdepth)) != ERR_NONE) {
            error = EEGENERAL;
            break;
        }

        if (F[SRCUR().opTok] <= G[tokNext.opTok]) {
            // Shift next token onto stack since it has higher precedence than token
            // on top of the stack.  Note that f values larger than g values mean higher
            // precedence

            if((SRCUR().opTok == OP_lparen) && (tokNext.opTok == OP_rparen)) {
                // change the left paren on the stack to a grouping operator
                // which will be discarded later.  This is to solve the problem
                // of (id)++ becoming a preincrement
                SRCUR().opTok = OP_grouped;
            }
            else {
                SRPUSH(tokNext);
            }
            if ((tokNext.opTok == OP_rparen) || (tokNext.opTok == OP_fcnend)) {
                if (pdepth-- < 0) {
                    pExState->err_num = ERR_MISSINGLP;
                    error = EEGENERAL;
                    break;
                }
            }
            // Skip token and trailing spaces in string
            szExpr += tokNext.cbTok;
            while ((*szExpr == ' ') || (*szExpr == '\t')) {
                ++szExpr;
            }

            // save contents of next token in old token for later

            tokOld = tokNext;
            if (*szExpr) {
                // If not at end of input
                if ((lexerr = GetToken ((uchar *)szExpr, &tokNext, radix, SRCUR().opTok)) == ERR_NONE) {
                    tokNext.iTokStart = (ulong) (tokNext.pbTok - pExStr);
                }
            }
            else {
                tokNext.opTok = OP_lowprec;
                tokNext.cbTok = 0;
            }
        }
        else {
            // Reduce ...
            // This loop pops tokens off the stack while the token removed has
            // lower precedence than the token remaining on the top of the stack.
            // This has the effect of throwing away the right parenthesis of
            // () and the right bracket of [] and pushing only the left paren or
            // left bracket;

            do {
                // Pop off stack (struct copy)
                tokT = SRPOP();
            }
            while (F[SRCUR().opTok] >= G[tokT.opTok]);

            // Push onto RPN stack or whatever
            if (tokT.opTok != OP_grouped) {
                if ((error = PushToken (&tokT)) != 0) {
                    break;
                }
            }
        }
    }

    //  unlock and free the shift-reduce stack and unlock the
    //  input string

    MemUnLock (hSRStack);
    MemUnLock (pExState->hExStr);
    return (error);
}




bool_t GrowSR ()
{
    ulong  oldSRsp = maxSRsp;
    HDEP    hNew;

    MemUnLock (hSRStack);
    if ((hNew = MHMemReAlloc (hSRStack, (maxSRsp + SRSTACKGROW) * sizeof (token_t))) == 0) {
        pSRStack = (ptoken_t) MemLock (hSRStack);
        return (FALSE);
    }
    hSRStack = hNew;
    maxSRsp += SRSTACKGROW;
    pSRStack = (ptoken_t) MemLock (hSRStack);
    memmove (((char *)pSRStack) + SRSTACKGROW * sizeof (token_t),
      pSRStack, oldSRsp * sizeof (token_t));
    SRsp += SRSTACKGROW;
    return (TRUE);
}



/***    CvtOp - Convert a token from unary to binary if necessary
 *
 *      error = CvtOp (ptokPrev, ptokNext, pdepth)
 *
 *
 *      Entry   ptokPrev = pointer to previously fetched token
 *              ptokNext = pointer to token just fetched
 *              pdepth = parenthesis depth
 *
 *      Exit    ptokNext->opTok = binary form of operator if necessary
 *
 *      Returns 0 if no error
 *              error index if error
 *
 *      Because operator precedence parsing can't deal with ambiguous
 *      operators (such as '-': is it unary or binary?), this routine
 *      looks at the previous token to determine whether the operator
 *      is unary or binary.
 *
 *      Note that ALL tokens come through here; this routine ignores
 *      tokens that have no ambiguity.  The lexer will always return
 *      the unary form of the operator; thus, OP_fetch instead of
 *      OP_mult for '*'.
 */


ulong
CvtOp (
    ptoken_t ptokOld,
    ptoken_t ptokNext,
    ulong  pdepth
    )
{
    if (ptokNext->opTok == OP_comma) {
        if (pdepth == 0) {
            ptokNext->opTok = OP_lowprec;
        }
        else {
            if (ptokOld->opTok == OP_arg) {
                // error if OP_arg followed immediately by a comma
                pExState->err_num = ERR_SYNTAX;
                return (EEGENERAL);
            }
            else {
                ptokNext->opTok = OP_arg;
                argcnt[pdepth - 1]++;
            }
        }
    }

    switch (ptokOld->opTok) {
        case OP_ident:
        case OP_hsym:
        case OP_retval:
        case OP_const:
        case OP_rparen:
        case OP_fcnend:
        case OP_rbrack:
        case OP_postinc:
        case OP_postdec:
        case OP_typestr:

            // If the previous token was an identifier, a constant, or
            // a right parenthesis or bracket, and the token is ambiguous,
            // it is taken to be of the binary form.

            switch (ptokNext->opTok) {
                case OP_fetch:
                    ptokNext->opTok = OP_mult;
                    break;

                case OP_uplus:
                    ptokNext->opTok = OP_plus;
                    break;

                case OP_negate:
                    ptokNext->opTok = OP_minus;
                    break;

                case OP_addrof:
                    ptokNext->opTok = OP_and;
                    break;

                case OP_uscope:
                    ptokNext->opTok = OP_bscope;
                    break;

                case OP_bang:
                    if (ptokOld->opTok == OP_ident) {
                        ptokNext->opTok = OP_execontext;
                    }
                    break;
            }
            break;

        default:
            break;
    }
    return (ERR_NONE);
}




/***    CheckErr - Check for syntax errors which parser won't find
 *
 *      err = CheckErr (op1, op2, pdepth)
 *
 *      Entry   op1 = (OP_...) token at top of shift-reduce stack
 *              op2 = (OP_...) token at head of input string
 *              pdepth = parenthesis nesting depth
 *
 *      Returns error index
 *
 * DESCRIPTION
 *       Checks for errors which the parser is incapable of finding
 *       (since we use precedence functions rather than a matrix).
 *       Simple code, could probably be optimized.
 */


ulong
CheckErr (
    op_t op1,
    op_t op2,
    ulong  pdepth
    )
{
    switch (op1) {
        // Top token on shift-reduce stack
        case OP_ident:
        case OP_const:
        case OP_retval:
            if ((op2 == OP_ident) || (op2 == OP_const) || (op2 == OP_retval)) {
                return (ERR_NOOPERATOR);
            }
            if ((op2 == OP_bang) || (op2 == OP_tilde)) {
                return (ERR_SYNTAX);
            }
            break;

        case OP_lparen:
            if (op2 == OP_rbrack) {
                return (ERR_SYNTAX);
            }
            else if (op2 == OP_lowprec) {
                return (ERR_MISSINGRP);
            }
            break;

        case OP_rparen:
            if ((op2 == OP_bang) || (op2 == OP_tilde)) {
                return (ERR_SYNTAX);
            }
            break;

        case OP_lbrack:
            if (op2 == OP_rparen) {
                return (ERR_SYNTAX);
            }
            else if (op2 == OP_lowprec) {
                return (ERR_MISSINGRB);
            }
            break;

        case OP_rbrack:
            if ((op2 == OP_ident) || (op2 == OP_lparen) || (op2 == OP_retval)) {
                return (ERR_NOOPERATOR);
            }
            if ((op2 == OP_bang) || (op2 == OP_tilde)) {
                return (ERR_SYNTAX);
            }
            break;

        case OP_lowprec:
            if (op2 == OP_rparen) {
                return (ERR_MISSINGLP);
            }
            else if (op2 == OP_rbrack) {
                return (ERR_MISSINGLB);
            }
            else if (op2 == OP_rcurly) {
                return (ERR_MISSINGLC);
            }
            else if ((op2 == OP_lowprec) && (pdepth != 0)) {
                return (ERR_SYNTAX);
            }
            break;
    }
    return (ERR_NONE);
}



/**     ParseTypeName - Parse a type name  (e.g., "(int)")
 *
 *      value = ParseTypeName (pn, szExpr, radix, perror)
 *
 *      Entry   pn = pointer to initial token
 *              szExpr = pointer to expression
 *              radix = radix for numeric constants
 *              perror = pointer to error return
 *
 *      Exit    pExState->err-num = ERR_MISSINGRP if end of string encountered
 *              *perror = EEGENERAL if end of string encountered
 *              token pointers in pn updated if valid type name
 *
 *      Returns PTN_error if error
 *              PTN_nottype if not a type string
 *              PTN_typefcn if (type string)(......
 *              PTN_typestr if (type string){ident | constant | op}
 *              PTN_formal  if ...typestring, or ...typestring)
 *
 * DESCRIPTION
 *      Examines string for possible type strings
 *      Note that the actual type flags are not set in the node.
 *      This is left to the bind phase.
 */


PTN_flag
ParseTypeName (
    ptoken_t ptokEntry,
    char *szExpr,
    uint radix,
    EESTATUS *perror
    )
{
    enum {
        PT_S0,
        PT_S1,
        PT_S2,
        PT_S3,
        PT_S4,
        PT_S5,
        PT_S6
    };
    token_t     tokNext;
    char *pParen;
    bool_t      ParenReq;
    ulong       state = PT_S0;
    op_t        tokTerm;
    char *pId = 0;
    ulong       lenId = 0;



    // Save entry token for later update if this is a type string


    tokNext = *ptokEntry;

    // if the initial character is a ( then the termination must be a )

    ParenReq =  (*szExpr == '(')? TRUE: FALSE;

    //  check tokens up to next right parenthesis or comma.  There are the
    //  following cases:
    //
    //  1.  (token)
    //      token can be either an identifier or a type name.
    //      If opprev is OP_sizeof, call it an identifier and let the
    //      binder detect that is a type string.  If the token after the
    //      ')' is a constant, identifier or '(' then '(token)' must be a
    //      cast.
    //
    //  2.  (token op token)
    //      This can only be an expression
    //
    //  3.  (token token...)
    //      This can only be a type.  Let the binder evaluate it to a type
    //
    //  4.  (token op)
    //      if op is * or & then it is a type.  Otherwise, it is an expression
    //
    //  5.  (op token)
    //      This can only be an expression

    for (;;) {
        // Skip token and trailing spaces in string

        if ((state != PT_S0) || (ParenReq == TRUE)) {
           // skip the previous token or skip the initial paren if
           // this is the first time through the loop
           szExpr += tokNext.cbTok;
           while ((*szExpr == ' ') || (*szExpr == '\t')) {
               szExpr++;
           }
           if (*szExpr != 0) {
               if ((GetToken ((uchar *)szExpr, &tokNext, radix, OP_lowprec) != ERR_NONE) ||
                 (tokNext.opTok == OP_badtok)) {
                   pExState->err_num = ERR_SYNTAX;
                   *perror = EEGENERAL;
                   return (PTN_error);
               }
           }
           else {
               // flag end of statement
               tokNext.opTok = OP_lowprec;
           }
        }
        switch (state) {
            case PT_S0:
                // state 0 looks at the first token and and if it is an
                // identifier, continues the scan

                switch (tokNext.opTok) {
                    case OP_ident:
                        //save id name and length
                        pId = tokNext.pbTok;
                        lenId = tokNext.cbTok;
                    case OP_hsym:
                        // 'token' can be either type or expression
                        state = PT_S1;
                        break;
                    default:
                        // let parser handle everything else
                        return (PTN_nottype);
                }
                break;

            case PT_S1:
                // state 1 looks at the token after the initial identifier
                switch (tokNext.opTok) {
                    case OP_rparen:
                        if (ParenReq) {
                            // go to state that will examine token after ')'
                            // to see if (token) is a cast or expression.
                            // save location of ')'

                            pParen = szExpr;
                            state = PT_S3;
                        }
                        else {
                            // we have ...token) which must be an argument
                            return (PTN_nottype);
                        }
                        break;

                    case OP_comma:
                        if (ParenReq) {
                            // if the opening character was a '(', then
                            // a ',' is an error
                            return (PTN_error);
                        }
                        else  {
                            // if we have "...token," then it must be an expression
                            return (PTN_nottype);
                        }
                        break;

                    case OP_ident:
                    case OP_hsym:
                        // '(token token...' must be a type string
                        // enter the state that is looking for the terminating
                        // ')' or ','

                        state = PT_S4;
                        break;

                    case OP_fetch:
                    case OP_addrof:
                        // '(token *' or '(token &' can be expression or type
                        // go to state where we are looking for ident, ')'
                        // or ','

                        state = PT_S2;
                        break;

                    case OP_bscope:
                    case OP_uscope:
                        // '(token::'
                        // this may be a nested class name in a type string

                        state = PT_S6;
                        break;

                    default:
                        // '(token op' must be an expression
                        return (PTN_nottype);
                }
                break;

            case PT_S2:
                // state 2 looks at the token after an ambiguous operator
                // if the token is '*' or '&', then the string is a type string

                switch (tokNext.opTok) {
                    case OP_rparen:

                        // '...token *)' or '...token &)' is a type string

                        pParen = tokNext.pbTok;
                        tokTerm = tokNext.opTok;
                        state = PT_S5;
                        break;

                    case OP_comma:
                        if (ParenReq) {
                            // an string of the form '(token *,' is an error
                            // because the function processing has already
                            // processed the opening paren
                            return (PTN_error);
                        }

                        // 'token *,' or 'token &,' is a type string

                        ptokEntry->pbEnd = szExpr;
                        ptokEntry->cbTok = (uchar)(ptokEntry->pbEnd - ptokEntry->pbTok);
                        return (PTN_formal);

                    default:
                        // '(token * ...' or '(token & ...' must be an expr
                        return (PTN_nottype);
                }
                break;

            case PT_S3:
                // we have found  '(token)...'  We now look at the next
                // token to determine if we have type string or the parenthesized
                // name of a function call or pointer to function

                if (pId) {
                    // if token is a simple type treat is as a type cast
                    // (caviar #3836 --gdp 10/4/92)

                    uchar (*p)[11];
                    bool_t cmpflag;

                    cmpflag = 1;
                    for (p = SimpleTypeName; p[0][0] != 0; p++) {
                        if ((p[0][0] == (uchar)lenId) &&
                            ((cmpflag = _tcsncmp ((char *)&p[0][1], pId, lenId)) == 0)) {
                            break;
                        }
                    }
                    if (cmpflag == 0) {
                        // reset parse to ')' at end of type string
                        ptokEntry->pbEnd = ++pParen;
                        ptokEntry->cbTok = (uchar)(ptokEntry->pbEnd - ptokEntry->pbTok);
                        return (PTN_typestr);
                    }
                }

                switch (tokNext.opTok) {
                    case OP_const:
                    case OP_ident:
                    case OP_hsym:
                    case OP_lparen:
                    case OP_sizeof:
                    case OP_bang:
                    case OP_tilde:
                    case OP_uscope:
                    case OP_context:
                        // reset parse to ')' at end of type string
                        ptokEntry->pbEnd = ++pParen;
                        ptokEntry->cbTok = (uchar)(ptokEntry->pbEnd - ptokEntry->pbTok);
                        if (tokNext.opTok == OP_lparen) {
                            return (PTN_typefcn);
                        }
                        else {
                            return (PTN_typestr);
                        }

                    default:
                        // '(token) op ...' is an expression not a type
                        // note that we for the operators + - * &
                        // we assume the binary forms.  If the user wants
                        // to cast these then the operand must be in (...)
                        // for example (type)(+var)

                        return (PTN_nottype);
                }
                break;


            case PT_S4:
                // state 4 looks for the terminating ')' or ',' at the end
                // of a type string

                switch (tokNext.opTok) {
                    case OP_lowprec:
                        // end of statement without ')' or ','
                        pExState->err_num = ERR_MISSINGRP;
                        *perror = EEGENERAL;
                        return (PTN_error);

                    case OP_rparen:
                        // save location of ')' and set state to look
                        // the next token
                        pParen = szExpr;
                        tokTerm = tokNext.opTok;
                        state = PT_S5;
                        break;

                    case OP_comma:
                        ptokEntry->pbEnd = szExpr;
                        ptokEntry->cbTok = (uchar)(ptokEntry->pbEnd - ptokEntry->pbTok);
                        return (PTN_formal);


                    default:
                        // loop to closing )
                        break;
                }
                break;

            case PT_S5:
                // we know we have a type string.  Set the end of string
                // to the closing ).  If the next token is ( then the type
                // string is functional style, i.e. (type)(....).  Otherwise,
                // it is a normal typestring '(type)token.....' or '(token)\0'

                if (ParenReq) {
                    ptokEntry->pbEnd = ++pParen;
                }
                else {
                    ptokEntry->pbEnd = pParen;
                }
                ptokEntry->cbTok = (uchar)(ptokEntry->pbEnd - ptokEntry->pbTok);

                switch (tokNext.opTok) {
                    case OP_lparen:
                        // (typestr)(...
                        return (PTN_typefcn);

                    default:
                        if (ParenReq) {
                            return (PTN_typestr);
                        }
                        else {
                            return (PTN_formal);
                        }
                }
                break;

            case PT_S6:
                // state PT_S6 accepts only identifiers
                // that follow a "token::token:: ... token::' string
                // which may represent a nested class name in a type
                // string

                switch (tokNext.opTok) {
                    case OP_ident:
                    case OP_hsym:
                        state = PT_S1;
                        break;
                    default:
                        // let the parser handle this case
                        return PTN_nottype;
                }
                break;

        }
    }
}


/**     XFormExpr -- Transform expression for template support
 *
 *      value = XFormExpr (radix)
 *
 *      Entry   radix = radix for numeric constants
 *              pExStr = locked pointer to expression string
 *
 *      Exit    pExStr transformed and reallocated if necessary
 *              pExStrSav points to original string if pExStr was transformed
 *
 *      Reurns
 *            EESTATUS
 *
 * DESCRIPTION
 *      Examines string for potential template class names. It is
 *      considered adequate to look for matching <>'s that are
 *      preceded by an identifier and do not contain operators like
 *      && and ||.
 *      If it finds substrings that satisfy these criteria it transforms
 *      them as follows:
 *        -strips spaces
 *        -changes radix of int constants to decimal
 *        -removes suffixes from constants
 *        -converts character literals to decimal constants
 *        -reallocates the string buffer if necessary
 *      The original expression string may be saved if necessary (so that
 *      it can be used by EEGetExprFromTM)
 *
 */

#define MAXBUFSZ 255 /* size of static buffer for template parsing */
#define MAXDIG   (1+10)  /* max number of digits for a long in the form <sign><abs_val> */

enum {
    XF_S0,
    XF_S1,
    XF_S2
};

EESTATUS
XFormExpr (
    uint radix
    )
{
    token_t     tokNext;
    TCHAR      *szExpr;
    TCHAR      *szTmpl;
    int         ndepth;
    int         nBuf;
    static      TCHAR buf[MAXBUFSZ+1];
    BOOL        fFound = FALSE;
    BOOL        fUnsigned;
    int         count;
    int         state = XF_S0;
    QUAD        val;
    UQUAD       uval;
    op_t        opTokPrev;

    tokNext.opTok = (op_t) 0;

    for (szExpr = pExStr; *szExpr ; szExpr += tokNext.cbTok) {
        // Skip token and trailing spaces in string
        while ((*szExpr == _T(' ')) || (*szExpr == _T('\t'))) {
           szExpr++;
        }

        opTokPrev = tokNext.opTok;

        if (((GetDBToken ((uchar *)szExpr, &tokNext, radix, OP_lowprec) != ERR_NONE) ||
            tokNext.opTok == OP_badtok)) {
            //if something goes wrong do not continue transformation
            //let the actual parsing catch the error
            return EENOERROR;
        }

        switch (state) {
            BOOL     fCopyToBuf;

            case XF_S0:
                // state 0 looks at the first token and and if it is an
                // identifier, moves to state S1

                if (tokNext.opTok == OP_ident) {
                        state = XF_S1;
                }

                break;

            case XF_S1:
                // state 1 checks if the token after the initial identifier
                // is a '<'
                if (tokNext.opTok == OP_lt) {
                    TCHAR   *pch;
                    // mark the beginning of the template name
                    szTmpl = szExpr;

                    // include preceding spaces in szTmpl
                    while (pExStr < szTmpl && *(pch = _tcsdec(pExStr, szTmpl)) == _T(' '))
                        szTmpl = pch;

                    nBuf = 0;
                    buf[nBuf++] = _T('<');
                    ndepth = 1;
                    state = XF_S2;
                }
                else {
                    state = XF_S0;
                }
                break;



            case XF_S2:
                // state 2 checks for matching '>'

                fCopyToBuf = FALSE;
                switch (tokNext.opTok) {

                    case OP_ident:
                        if (opTokPrev == OP_ident) {
                            if (nBuf + 1 > MAXBUFSZ)
                                state = XF_S0;
                            else
                                buf[nBuf++] = _T(' ');
                        }
                        fCopyToBuf = TRUE;
                        break;

                    case OP_gt:
                        // in this case we should put '>' to buf
                        // but we should avoid putting two '>'
                        // without a space between them because
                        // they would form the rshift operator

                        if (nBuf + 2 > MAXBUFSZ) {
                            // expression too long
                            // assume it is not a template
                            state = XF_S0;
                        }
                        else {
                            if (buf[nBuf-1] == _T('>')) {
                                buf[nBuf++] = _T(' ');
                            }
                            buf[nBuf++] = _T('>');
                        }

                        if (--ndepth == 0) {
                            // we have found a potential template class name

                            int diff = (int)(nBuf - (szExpr - szTmpl + 1));
                            int nExpr = (int)(szExpr - pExStr);
                            int nTmpl = (int)(szTmpl - pExStr);
                            HDEP hStr;

                            if (! fFound) {
                                char *pExStrSav;

                                // keep a copy of the original string
                                // in order to preserve the template class name
                                // (to be used by routines lile EEGetExprFromTM)

                                if ((pExState->hExStrSav = MemAllocate ((pExState->ExLenSav = pExState->ExLen) + 1)) == 0) {
                                    pExState->err_num = ERR_NOMEMORY;
                                    return (EEGENERAL);
                                }

                                pExStrSav = (char *) MemLock (pExState->hExStrSav);
                                memcpy (pExStrSav, pExStr, pExState->ExLen+1);
                                pExStrSav[pExState->ExLen] = 0;
                                MemUnLock (pExState->hExStrSav);
                                fFound = TRUE;
                            }

                            buf[nBuf] = '\0';
                            if (diff != 0) {
                                int newsz = _tcslen(pExStr) * sizeof(TCHAR) + diff +1;
                                szExpr = pExStr + nExpr;
                                szTmpl = pExStr + nTmpl;

                                if (diff < 0) {
                                    memmove (szExpr+diff, szExpr, (_tcslen(szExpr)+1) * sizeof (TCHAR));
                                    memmove (szTmpl, buf, nBuf * sizeof(TCHAR));
                                    szExpr += diff;
                                }

                                MemUnLock (pExState->hExStr);
                                hStr = MHMemReAlloc (pExState->hExStr, newsz);
                                if (hStr == 0){
                                    // restore pExStr
                                    pExStr = (char *) MemLock (pExState->hExStr);
                                    pExState->err_num = ERR_NOMEMORY;
                                    return (EEGENERAL);
                                }

                                pExState->hExStr = hStr;
                                pExState->ExLen += diff;
                                pExState->strIndex += diff;
                                pExStr = (char *) MemLock (pExState->hExStr);
                                szExpr = pExStr + nExpr;
                                szTmpl = pExStr + nTmpl;
                                if (diff > 0) {
                                    memmove (szExpr+diff, szExpr, (_tcslen(szExpr)+1) * sizeof (TCHAR));
                                    memmove (szTmpl, buf, nBuf * sizeof (TCHAR));
                                    szExpr += diff;
                                }
                            }
                            else { // diff == 0
                                memmove (szTmpl, buf, nBuf * sizeof (TCHAR));
                            }

                            state = XF_S0;
                        }

                        break;

                    case OP_const:

                        // put constant in buffer using base10 representation

                        fUnsigned = FALSE;
                        switch (tokNext.typ) {
                            case T_CHAR:
                            case T_RCHAR:
                                val = (QUAD) tokNext.val.vchar;
                                break;
                            case T_INT2:
                            case T_SHORT:
                                val = (QUAD) tokNext.val.vshort;
                                break;
                            case T_INT4:
                            case T_LONG:
                                val = (QUAD) tokNext.val.vlong;
                                break;
                            case T_INT8:
                            case T_QUAD:
                                val = tokNext.val.vquad;
                                break;
                            case T_UCHAR:
                                uval = (UQUAD) tokNext.val.vuchar;
                                fUnsigned = TRUE;
                                break;
                            case T_UINT2:
                            case T_USHORT:
                                uval = (UQUAD) tokNext.val.vushort;
                                fUnsigned = TRUE;
                                break;
                            case T_UINT4:
                            case T_ULONG:
                                uval = (UQUAD) tokNext.val.vulong;
                                fUnsigned = TRUE;
                                break;
                            case T_UINT8:
                            case T_UQUAD:
                                uval = tokNext.val.vuquad;
                                fUnsigned = TRUE;
                                break;

                            default:
                                // floating point constants, strings, etc
                                // copy as is
                                fCopyToBuf = TRUE;
                        }


                        if (!fCopyToBuf) {
                            // we have a constant that should be transformed

                            char localBuf[MAXDIG+1]; // use local buf for sprintf
                            if (nBuf > MAXBUFSZ - MAXDIG) {
                                // assume expression too complex to be
                                // a template
                                state = XF_S0;
                                szExpr = szTmpl;
                                break;
                            }

                            count = fUnsigned?
                                sprintf(localBuf,"%I64d",uval):
                                sprintf(localBuf,"%I64d", val);

                            memcpy(buf+nBuf, localBuf, count);

                            if (count > 0)
                                nBuf += count;
                        }

                        break;

                    case OP_oror:
                    case OP_andand:
                        // We do not allow these operators in
                        // canonicalized template names
                        state = XF_S0;
                        break;

                    case OP_lt:
                        ndepth++;
                        // FALL THROUGH

                    default:
                        fCopyToBuf = TRUE;
                }

                if (fCopyToBuf) {
                    // copy token to template buffer
                    if (nBuf + tokNext.cbTok > MAXBUFSZ) {
                        // expression too long
                        // assume it is not a template
                        state = XF_S0;
                    }
                    else {
                        memcpy (buf+nBuf,szExpr, tokNext.cbTok);
                        nBuf += tokNext.cbTok;
                    }
                }

                break;

            default:
                pExState->err_num = ERR_INTERNAL;
                return EEGENERAL;
        }
    }

    return EENOERROR;
}



/**     ParseContext - Parse a context description {...}
 *
 *      flag = ParseContext (pn, perror)
 *
 *      Entry   pn = Pointer to token containing {
 *
 *      Exit    perror = ERR_BADCONTEXT if end of string encountered
 *              token pointers in pn updated if valid context
 *
 *      Returns none
 *
 * DESCRIPTION
 *      Examines type string for primitive types such as "int", "long",
 *      etc.    Also checks for "struct" keyword.  Note that the actual type
 *      flags are not set in the node.  This is left to the evaluation phase.
 *
 * NOTES
 */


void
ParseContext (
    ptoken_t ptokNext,
    EESTATUS *perror
    )
{
    char *pb;
    char *pbSave;
    char *pbCurly;
    BOOL fInQuote = FALSE;

    // Initialization.

    pb = pbSave = ptokNext->pbTok + 1 - ptokNext->cbTok;

#ifndef _SBCS
    while (_istspace( *(_TUCHAR *)pb )) {
        pb = (char *)_tcsinc( (TCHAR *)pb );
    }
#else // !_SBCS
    while (isspace(*pb))
        ++pb;
#endif // !_SBCS

    DASSERT (*pb == '{');

    // Save position and skip '{'

    pbCurly = pb++;

    //  skip to closing }

    while ( (*pb != 0) && ((*pb != '}') || fInQuote) )
    {
        if (*pb == '\"')
        {
            fInQuote = !fInQuote;
        }
#ifndef _SBCS
        pb = (char *)_tcsinc( (TCHAR *)pb );
#else // !_SBCS
        pb++;
#endif // !_SBCS
    }

    ptokNext->cbTok = (uchar)(pb - pbCurly + 1);
    ptokNext->opTok = OP_context;

    if (*pb != '}' || fInQuote) {
        pExState->err_num = ERR_BADCONTEXT;
        *perror = EEGENERAL;
    }
}


/***    GetToken - Fetch next token from expression string and support
 *                parsing of template class names.
 *
 *      status = GetToken (pbExpr, ptoken, radix, oper)
 *
 *      Entry   pbExpr = far pointer to expression string
 *              ptoken = pointer to token return location
 *              radix = default radix for numeric conversion
 *              oper = previous operator
 *
 *      Exit    *ptoken = token as lexed from input string.  If an
 *              error occurred, the token will be of type OP_badtok.
 *              If the token is a constant, its value will be determined and
 *              placed in the token's 'val' field, and its type
 *              (e.g., T_USHORT) will be placed in the token's 'typ' field.
 *              If the previous operator is ., ->, ::, then ~ as the next
 *              character is taken to be part of an identifier
 *              If the token is an identifier and the next token is OP_lt
 *              it looks for a matching OP_gt that would form a template
 *              class name.
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered in parse
 *
 *
 *          This is a wrapper of GetDBToken in order to facilitate
 *         template support.
 */

ulong
GetToken (
    uchar *pbExpr,
    ptoken_t pTok,
    uint radix,
    op_t oper
    )
{
    ulong  error;
    uchar * szExpr = pbExpr;
    token_t tokNext;
    char prev;

    if ( (error = GetDBToken (szExpr, pTok, radix, oper)) != ERR_NONE ||
         pTok->opTok != OP_ident) {
        return error;
    }

    szExpr += pTok->cbTok;
    if (*szExpr &&
        GetDBToken (szExpr, &tokNext, radix, OP_lowprec) == ERR_NONE &&
        tokNext.opTok == OP_lt) {

        int ndepth = 1;
        szExpr += tokNext.cbTok;

        // find matching '>'
        while ((prev = *szExpr) != 0) {
            szExpr = (uchar *) _tcsinc( (TCHAR *) szExpr );
            switch (*szExpr) {
                case '>':
                    if (--ndepth == 0) {
                        // we have found a potential template class name

                        pTok->pbEnd = (char *) szExpr + 1;
                        pTok->cbTok = (uchar)(pTok->pbEnd - pTok->pbTok);
                        return ERR_NONE;
                    }
                    break;

                case '<':
                    ndepth++;
                    break;

                case '|':
                    // We do not allow this operator in
                    // canonicalized template class names
                    goto end;

                case '&':
                    // We do not allow the  && operator in
                    // canonicalized template class names
                    if (prev == '&')
                        goto end;
            }
        }
    }
end:
    return error;
}


/***    AEParseRule - Parse auto-expansion rule
 *
 *      AE_t = AEParseRule (lsz)
 *
 *      Entry   lsz = null terminated pointer to a string describing
 *                      the kind of expansion to be performed
 *
 *      Returns AE_t
 */

AE_t
AEParseRule(
    LSZ lsz
    )
{
    if (!_tcscmp (lsz, ",t")) {
        // <,t> corresponds to a special specifier denoting
        // derived-most class type display
        return AE_downcasttype;
    } else {
        // regular case: lsz does not point to a special
        // string, so the expansion rule defaults to creating
        // a child TM
        return AE_childTM;
    }
}
