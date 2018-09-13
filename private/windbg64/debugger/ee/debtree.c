/***    DEBTREE.C - Routines for building expression tree
 *
 *      Routines to take tokens lexed from expression string
 *      and add to expression tree (tree building routines).
 */

#include "debexpr.h"

ulong  AddLeaf (ptoken_t);
ulong  AddNode (ptoken_t);

/***    PushToken - Push a token onto expression stack
 *
 *
 *      fSuccess = PushToken (ptok)
 *
 *      Entry   ptok = pointer to token_t structure to add
 *
 *
 *      Returns TRUE if token pushed ont expression stack
 *              FALSE if error
 */

ulong
PushToken (
    ptoken_t ptok
    )
{
    ulong       retVal;

    if (OP_IS_IDENT (ptok->opTok)) {
        // if the node is an identifier, constant or end of function arguments
        // just add it to the tree
        retVal = AddLeaf (ptok);
    } else {
        retVal = AddNode (ptok);
    }
    return (retVal);
}


/***    AddLeaf - Add a leaf node to the expression tree
 *
 *      fSuccess = AddLeaf (ptok)
 *
 *      Entry   ptok = pointer to token to add to tree
 *              pExState->hStree = handle of syntax tree buffer
 *
 *      Returns TRUE if successful
 *              FALSE if not and sets DebErr
 *
 *      Notes   The start_node index in the stree_t structure is not changed
 *              by AddLeaf.  If there is only a single node, (OP_ident or
 *              OP_const), then start_node points to the correct node that
 *              was set by the routine that initialized the tree buffer.
 *              AddNode will set the correct node if there is a
 *              more complex parse tree.
 */

ulong
AddLeaf (
    ptoken_t ptok
    )
{
    pnode_t     pn;
    peval_t     pv;
    bnode_t *ia;
    uint        len;

    //  check for space

    // check for stack space too... [Dolphin 10293]
    if (pTree->stack_next >= NSTACK_MAX) {
        pExState->err_num = ERR_TOOCOMPLEX;
        return (EEGENERAL);
    }

    len = sizeof (node_t) + sizeof (eval_t);
    if ((uint)(pTree->stack_base - pTree->node_next) < len) {
        // syntax tree is full, grow it
        if (!GrowTree (NODE_DEFAULT)) {
            // grow failed, clean up and return error
            pExState->err_num = ERR_TOOCOMPLEX;
            return (EEGENERAL);
        }
    }
    pn = (pnode_t)((bnode_t)(pTree->node_next));

    // Initialize the Node.

    memset ((char *)pn, 0, len);
    NODE_OP (pn) = ptok->opTok;
    pv = &pn->v[0];
    EVAL_ITOK (pv) = ptok->iTokStart;
    EVAL_CBTOK (pv) = ptok->cbTok;

    // If the node is a constant, retrieve its value from the token.

    if (NODE_OP (pn) == OP_const) {
        EVAL_TYP (pv) = ptok->typ;
        EVAL_VAL (pv) = VAL_VAL (ptok);
    }

    ia = (bnode_t *)((char *)pTree + pTree->stack_base);
    ia[pTree->stack_next++] = (bnode_t)(pTree->node_next);
    pTree->node_next += len;
    return (0);
}


/**     AddNode - Add an operator node to the expression tree
 *
 *      fSuccess = AddNode (ptok)
 *
 *      Entry   ptok = pointer to token to add
 *
 *      Returns TRUE if node was added to tree
 *              FALSE if error
 *
 *      Notes   AddNode will set the start_node index in the stree_t structure
 *              to point to the node just added.  If the statement is
 *              correctly parsed, start_node will contain the index of the
 *              last operator and there will be only one node on the stack
 *
 */

ulong
AddNode (
    ptoken_t ptok
    )
{
    pnode_t     pn;
    ulong       retval = 0;
    ulong       iStack;
    bnode_t *ia;
    uint        len;

    //  check for space

    iStack = pTree->stack_next;

    // check for stack space too... [Dolphin 10293]
    if (iStack >= NSTACK_MAX) {
        pExState->err_num = ERR_TOOCOMPLEX;
        return (EEGENERAL);
    }

    switch (ptok->opTok) {
        case OP_cast:
        case OP_function:
        case OP_sizeof:
        case OP_dot:
        case OP_pointsto:
        case OP_bscope:
        case OP_uscope:
        case OP_pmember:
        case OP_dotmember:
            // these operators use the eval_t portion of the node to store
            // casting information.  In the case of the OP_dot, etc., the
            // casting information is the implicit cast of the left member
            // to a base class if one is required.

            len = sizeof (node_t) + sizeof (eval_t);
            break;

        case OP_arg:
            len = sizeof (node_t) + sizeof (argd_t);
            break;

        case OP_context:
        case OP_execontext:
            len = sizeof (node_t) + sizeof (CXF);
            break;

        // these are weird asm ops - that could also be idents
        // if no operand - assume ident
        // sps - 9/9/92
        case OP_dw:
        case OP_by:
        case OP_wo:
            if (iStack < 1) {
                ptok->opTok = OP_ident;
                return (AddLeaf (ptok));
            }


        default:
            len = sizeof (node_t);
            break;
    }
    len = (len + 1) & ~1;
    if ((uint)(pTree->stack_base - pTree->node_next) < len) {
        // syntax tree is full, grow it
        if (!GrowTree (NODE_DEFAULT)) {
            // grow failed, clean up and return error
            pExState->err_num = ERR_TOOCOMPLEX;
            return (EEGENERAL);
        }
    }
    pn = (pnode_t)((bnode_t)(pTree->node_next));

    // Initialize the Node.

    memset (pn, 0, len);
    NODE_OP (pn) = ptok->opTok;
    if (ptok->opTok == OP_context || ptok->opTok == OP_execontext) {
        EVAL_ITOK (&pn->v[0]) = ptok->iTokStart;
        EVAL_CBTOK (&pn->v[0]) = ptok->cbTok;
    }

    // Set up the left and right children (left only if unary).

    ia = (bnode_t *)((char *)pTree + pTree->stack_base);
    if (OP_IS_BINARY (ptok->opTok)) {
        if (iStack < 2) {
            pExState->err_num = ERR_NOOPERAND;
            retval = EEGENERAL;
        }
        else {
            pn->pnRight = ia[--iStack];
            pn->pnLeft  = ia[--iStack];
        }
    } else {
        if (iStack < 1) {
            pExState->err_num = ERR_NOOPERAND;
            retval = EEGENERAL;
        }
        else {
            pn->pnLeft  = ia[--iStack];
            pn->pnRight = NULL;
        }
    }

    // set this node into the start_node

    if (retval == 0) {
        ia[iStack++] = (bnode_t)(pTree->node_next);
        pTree->start_node = pTree->node_next;
        pTree->node_next += len;
        pTree->stack_next = iStack;
    }
    return (retval);
}


/***    GrowTree - Grow space for the expression tree
 *
 *      error = GrowTree (incr)
 *
 *      Entry   incr = amount to increase tree size
 *              pExState = address of expression state structure
 *              pExState->hSTree locked
 *
 *      Exit    pExState->hSTree locked
 *              pTree = address of locked syntax tree
 *
 *      Returns TRUE if successful
 *              FALSE if unsuccessful
 *
 */


bool_t FASTCALL
GrowTree (
    uint incr
    )
{
    register HDEP    NewTree;
    register uint    len;

    len = pTree->size + incr;
    MHMemUnLock (pExState->hSTree);
    if ((NewTree = MHMemReAlloc (pExState->hSTree, len)) == 0) {
        pTree = (pstree_t) MHMemLock (pExState->hSTree);
        return (FALSE);
    } else {
        // reallocation succeeded, move data within buffer and update header

        pExState->hSTree = NewTree;
        pTree = (pstree_t) MHMemLock (pExState->hSTree);
        pTree->size = len;

        // move parse stack down by allocated amount if the stack is in use
        // by the parser.  Stack in use is indicated by non-zero stack_base.
        // If we are in the bind phase, stack_base is zero,

        if (pTree->stack_base != 0) {
            memcpy ((char *)pTree + pTree->stack_base + NODE_DEFAULT,
              (char *)pTree + pTree->stack_base,
              NSTACK_MAX * sizeof (bnode_t));
            pTree->stack_base += NODE_DEFAULT;
        }
        return (TRUE);
    }
}


/***    GrowETree - Grow space for the expression tree
 *
 *      error = GrowETree (incr)
 *
 *      Entry   incr = amount to increase tree size
 *              pExState = address of expression state structure
 *              pExState->hETree locked
 *
 *      Exit    pExState->hETree locked
 *              pTree = address of locked syntax tree
 *
 *      Returns TRUE if successful
 *              FALSE if unsuccessful
 *
 */

bool_t FASTCALL
GrowETree (
    uint incr
    )
{
    register HDEP       NewTree;
    register uint       len;

    len = pTree->size + incr;
    MHMemUnLock (pExState->hETree);
    if ((NewTree = MHMemReAlloc (pExState->hETree, len)) == 0) {
        pTree = (pstree_t) MHMemLock (pExState->hETree);
        return (FALSE);
    } else {
        // reallocation succeeded, move data within buffer and update header

        pExState->hETree = NewTree;
        pTree = (pstree_t) MHMemLock (pExState->hETree);
        pTree->size = len;

        // move parse stack down by allocated amount if the stack is in use
        // by the parser.  Stack in use is indicated by non-zero stack_base.
        // If we are in the bind phase, stack_base is zero,

        if (pTree->stack_base != 0) {
            memcpy ((char *)pTree + pTree->stack_base + NODE_DEFAULT,
              (char *)pTree + pTree->stack_base,
              NSTACK_MAX * sizeof (bnode_t));
            pTree->stack_base += NODE_DEFAULT;
        }
        return (TRUE);
    }
}
