/***    debtyper.c - typer module for expression evaluator
 *
 *      Routines to validate, add, and change typing information in
 *      an expression tree.
 */

#include "debexpr.h"

// Basic type groups.

#define BTYP_INT    0x01        // signed/unsigned char, short, int, long
#define BTYP_FLOAT  0x02        // float, double, long double
#define BTYP_ENUM   0x04        // enums
#define BTYP_VOID   0x08        // void
#define BTYP_PTR    0x10        // pointers

// ANSI-defined type groups.

#define ANSI_BASIC      (BTYP_INT | BTYP_FLOAT)
#define ANSI_ENUM       (BTYP_ENUM | BTYP_VOID)
#define ANSI_INTEGRAL   (BTYP_INT | BTYP_ENUM)
#define ANSI_FLOAT      (BTYP_FLOAT)
#define ANSI_ARITH      (ANSI_INTEGRAL | ANSI_FLOAT)
#define ANSI_SCALAR     (ANSI_ARITH | BTYP_PTR)

// Operator class mapping.

uchar SEGBASED (_segname("_CODE"))  mpopopc[COPS_EXPR] = {
#define OPCNT(name, val)
#define OPCDAT(opc)
#define OPDAT(op, opfprec, opgprec, opclass, opbind, opeval) opclass,
#include "debops.h"
#undef OPDAT
#undef OPCDAT
#undef OPCNT
};

// Data structure for determining legality of operator/operand
// combinations.
//
//      opc = operator class from debops.h.  (OPC_null is not checked)
//      atLeft = flags for types that can left hand operand
//      atRight = flags for types that can be right hand operand

struct {
    char    opc;            // Operator class
    uchar   atLeft;         // Aggregate Type'
    uchar   atRight;
}
optypLegal[] = {
    { OPC_integral,     ANSI_INTEGRAL,      ANSI_INTEGRAL   },
    { OPC_scalar,       ANSI_SCALAR,        ANSI_SCALAR     },
    { OPC_arith,        ANSI_ARITH,         ANSI_ARITH      },
    { OPC_ptr,          BTYP_PTR,           BTYP_PTR        },
    { OPC_ptrint,       BTYP_PTR,           ANSI_INTEGRAL   },
    { OPC_ptrint,       ANSI_INTEGRAL,      BTYP_PTR        },
    { OPC_relat,        ANSI_ARITH,         ANSI_ARITH      },
    { OPC_relat,        BTYP_PTR,           BTYP_PTR        },
    { OPC_relat,        BTYP_PTR,           ANSI_INTEGRAL   },  // M00ANSI
    { OPC_relat,        ANSI_INTEGRAL,      BTYP_PTR        },  // M00ANSI
    { OPC_equiv,        ANSI_ARITH,         ANSI_ARITH      },
    { OPC_equiv,        BTYP_PTR,           BTYP_PTR        },
    { OPC_equiv,        BTYP_PTR,           ANSI_INTEGRAL   },
    { OPC_equiv,        ANSI_INTEGRAL,      BTYP_PTR        },
    { OPC_plus,         ANSI_ARITH,         ANSI_ARITH      },
    { OPC_plus,         BTYP_PTR,           ANSI_INTEGRAL   },
    { OPC_plus,         ANSI_INTEGRAL,      BTYP_PTR        },
    { OPC_minus,        ANSI_ARITH,         ANSI_ARITH      },
    { OPC_minus,        BTYP_PTR,           ANSI_INTEGRAL   },
    { OPC_minus,        BTYP_PTR,           BTYP_PTR        }
};


#define C_OPTYP_LEGAL   (sizeof (optypLegal) / sizeof (optypLegal[0]))


void CastRelational (peval_t, peval_t);
bool_t CastToBase (peval_t, peval_t);
ulong  TyperCheckType (peval_t, ulong );
void CvtPtr (peval_t, CV_typ_t);
void CvtSizeUp (peval_t);
void CvtSizeDown (peval_t);
CV_typ_t FASTCALL IntegralPromote (CV_typ_t);
ulong  LegalTypes (opc_t, peval_t, peval_t);
CV_typ_t PerformUAC (CV_typ_t, CV_typ_t);
LONG ExFilterlongEVAL_FLOAT(EXCEPTION_POINTERS *, peval_t);


/**     CastNode - Cast a node to a specified type
 *
 *      CastNode (pv, dType, uType)
 *
 *      Entry   pv = pointer to value to cast
 *              dType = desired resultant type
 *              dType = T_NOTYPE if cast of based pointer to far pointer
 *              uType = type of underlying type if pointer.  Used only if
 *                      dType is T_*CVPTR
 *
 *      Exit
 *
 *      Returns
 *
 *      Note    The actual casting of the value is performed only if the
 *              Evaluating flag is set true by the top level routines
 */

bool_t
CastNode (
    peval_t pv,
    CV_typ_t dType,
    CV_typ_t uType
    )
{
    ulong       size;

#if CC_LAZYTYPES
    if (THAreTypesEqual( EVAL_MOD(pv), EVAL_TYP (pv), dType ) )
#else
    if (EVAL_TYP (pv) == dType)
#endif
    {
        if (CV_IS_INTERNAL_PTR (dType)) {
            // we have constructed pointers to an object
            if (PTR_UTYPE (pv) == uType) {
                // the underlying type of the pointers match
                return (TRUE);
            }
        }
        else {
            // return if it's already what we want.
            return (TRUE);
        }
    }

    if (dType == T_VOID) {
        // we can cast anything to void
        return (SetNodeType (pv, T_VOID));
    }

    if (EVAL_TYP (pv) == T_VOID) {
        // cannot cast from void to any other type
        goto badcast;
    }

    if (EVAL_IS_BASED (pv)) {
        // this can be one of the following:
        //      cast from based pointer to far pointer
        //
        // M00KLUDGE - this code does not work because the CV information
        // M00KLUDGE - does not not necessarily contain the type record
        // M00KLUDGE - for the far pointer to type.  What we could do is
        // M00KLUDGE - search the type records for a pointer for which the
        // M00KLUDGE - underlying type is the same as for the based pointer

        // if we have a based pointer, then we normalize it
        // to a far pointer to the underlying type

        if (!NormalizeBase (pv)) {
            goto badcast;
        }
        return (TRUE);
    }

    if (EVAL_IS_PTR (pv)) {
        if (!CV_IS_PRIMITIVE (dType)) {
            // this may be one of the following:
            //      cast from a pointer to a based pointer M00KLUDGE - does not work
            //      cast from pointer to derived to pointer to base
            //      cast from reference to derived to reference to base

            if (!SetNodeType (pv, dType)) {
                // the destination type does not exist
                goto badcast;
            }
            if (EVAL_IS_BASED (pv)) {
                // cast pointer to based pointer  M00KLUDGE - this does not work
                DASSERT (FALSE);
                //return (DeNormalizePtr (pv, pvT));
            }
            return (TRUE);
        }
        else if (CV_IS_INTERNAL_PTR (dType)) {
            // this may be one of the following:
            //      one of above with dType being a constructed pointer to class
            EVAL_TYP (pv) = uType;
            if (!SetNodeType (pv, dType)) {
                // the destination type does not exist
                goto badcast;
            }
            return (TRUE);
        }
    }

    if ((CV_IS_PRIMITIVE (dType) && CV_TYP_IS_PTR (dType)) ||
#if CC_LAZYTYPES
      !THAreTypesEqual( EVAL_MOD(pv), dType, uType ) )
#else
     (dType != uType))
#endif
    {
        // the first case is that the desired type is a primitive pointer.
        // The second case is that the desired type is a pointer to the
        // "complex" underlying type.  If the input node is primitive value
        // that is not a real value, then convert the value to a pointer.
        // If the input node is already a pointer, do normal conversion.
        // Casting a real to a pointer is an error.

        if (CV_IS_PRIMITIVE (EVAL_TYP (pv)) &&
            (CV_TYP_IS_REAL (EVAL_TYP (pv)) || CV_TYP_IS_COMPLEX (EVAL_TYP(pv))) ||
            // cannot cast real to pointer.  Note that pointer to real can
            // be cast
            !CV_IS_PRIMITIVE (EVAL_TYP (pv)) && !EVAL_IS_PTR (pv)) {
            // also cannot cast an object to pointer --gdp 8-27-92
            goto badcast;
        }
        if (Evaluating) {
            if (EVAL_IS_PTR (pv) || EVAL_IS_FCN (pv)) {
                // cast pointer to pointer
                // do nothing
            }
            else if (CV_IS_PRIMITIVE (dType)) {
                // cast to primitive pointer

                ADDR_IS_LI (EVAL_PTR (pv)) = FALSE;
                if (CV_MODE (dType) == CV_TM_NPTR ||
                  CV_MODE (dType) == CV_TM_NPTR32 ||
                  CV_MODE (dType) == CV_TM_FPTR32 ||
                  TypeSize (pv) < 4) {
                    // DS extend if node is a short integral value or a
                    // near pointer or a far 32-bit pointer (which has a
                    // 32-bit offset).  M00REVIEW: am I doing this right?

                    // REVIEW -billjoy- ADDRLIN32???
                    if (CV_MODE (dType) == CV_TM_NPTR32 ||
                      CV_MODE (dType) == CV_TM_FPTR32) {
                        EVAL_PTR_OFF (pv) = EVAL_ULONG (pv);
                        ADDR_IS_OFF32(EVAL_PTR(pv)) = TRUE;
                    }
                    else {
                        EVAL_PTR_OFF (pv) = EVAL_USHORT (pv);
                        ADDR_IS_OFF32(EVAL_PTR(pv)) = FALSE;
                    }

                    // Dolphin #9941: We need to set ADDR_IS_FLAT,
                    // (EvalMem was emitting a segmented address
                    // after calling CastNode)

                    ADDR_IS_FLAT  ( EVAL_PTR ( pv ) ) = pExState->state.f32bit ? 1 : 0;

                    if (TargetMachine == mptix86) {
                        if (EVAL_PTR_OFF (pv) != 0 || ADDR_IS_FLAT(EVAL_PTR(pv))) {
                            EVAL_PTR_SEG (pv) = GetSegmentRegister(pExState->hframe, REG_DS); //M00FLAT32
                        }
                        else {
                            EVAL_PTR_SEG (pv) = 0;
                        }
                    } else {
                        EVAL_PTR_SEG (pv) = 0;
                    }

                }
                else {
                    if (TargetMachine != mptix86) {
                        DASSERT(FALSE);
                    } else {
                        ADDR_IS_OFF32 ( EVAL_PTR ( pv ) ) = pExState->state.f32bit ? 1 : 0;
                        ADDR_IS_FLAT  ( EVAL_PTR ( pv ) ) = pExState->state.f32bit ? 1 : 0;
                        if ( !ADDR_IS_FLAT ( EVAL_PTR ( pv ) ) ) {
                            EVAL_PTR_SEG (pv) = (WORD) (EVAL_ULONG (pv) >> 16);
                            EVAL_PTR_OFF (pv) = EVAL_ULONG (pv) & 0xffff;
                        }
                    }
                }
            }
            else {
                // cast to non-primitive pointer

                size = (ulong ) TypeSize (pv);
                EVAL_TYP (pv) = uType;
                if (!SetNodeType (pv, dType)) {
                    return (FALSE);
                }
                ADDR_IS_LI (EVAL_PTR (pv)) = FALSE;
                if (size < 4) {
                    // DS extend if node is a short integral value or a
                    // near pointer.

                    EVAL_PTR_SEG (pv) = GetSegmentRegister(pExState->hframe, REG_DS); //M00FLAT32
                    EVAL_PTR_OFF (pv) = EVAL_USHORT (pv);
                }
                else if (pExState->state.f32bit) {
                    EVAL_PTR_SEG (pv) = GetSegmentRegister(pExState->hframe, REG_DS); //M00FLAT32
                    EVAL_PTR_OFF (pv) = EVAL_ULONG (pv);
                    ADDR_IS_OFF32(EVAL_PTR(pv)) = TRUE;
                }
                else {
                    EVAL_PTR_SEG (pv) = (WORD) (EVAL_ULONG (pv) >> 16);
                    EVAL_PTR_OFF (pv) = EVAL_ULONG (pv) & 0xffff;
                }
            }
        }
        EVAL_TYP (pv) = uType;
        return (SetNodeType (pv, dType));
    }

    if (dType == T_SEGMENT) {
        // If the requested type is T_SEGMENT and the type of the node is
        // pointer, then the value of the resulting node is either DS or
        // the segment portion of the node value.

        if (!EVAL_IS_PTR (pv)) {
            goto badcast;
        }
        if (Evaluating) {
            if (EVAL_IS_NPTR (pv) == CV_TM_NPTR) {
                // M00KLUDGE - if this test never hits, the the ptr is far form
                DASSERT (EVAL_PTR_OFF (pv) == GetSegmentRegister(pExState->hframe, REG_DS)); //M00FLAT32
                EVAL_USHORT (pv) = (USHORT) GetSegmentRegister(pExState->hframe, REG_DS); //M00FLAT32
            }
            else {
                EVAL_USHORT (pv) = (USHORT)EVAL_PTR_SEG (pv);
            }
        }
        return (SetNodeType (pv, dType));
    }

    // If the node is an enum, get the underlying type
    if (EVAL_IS_ENUM (pv)) {
        SetNodeType (pv, ENUM_UTYPE(pv));
    }

    // If the node is not a primitive type but is an address, then convert
    // the type of the node to a near or far pointer to character.

    if (!CV_IS_PRIMITIVE (EVAL_TYP (pv))) {
        if (!EVAL_IS_ADDR (pv)) {
            goto badcast;
        }
        if (ADDR_IS_LI (EVAL_PTR (pv))) {
            SHFixupAddr (&EVAL_PTR (pv));
        }
        SetNodeType (pv, (CV_typ_t) (pExState->state.f32bit ?
            T_32PCHAR : EVAL_IS_NPTR (pv)? T_PCHAR: T_PFCHAR));
    }

    // The code above has already taken care of casting a value to a pointer.
    // Therefore, if the input node is a pointer, then this must be a cast
    // from a pointer to another type

    if (EVAL_IS_PTR (pv)) {
        // If converting from pointer.  The T_ULONG works because we carry
        // all pointers as seg:offset
        if (ADDR_IS_LI (EVAL_PTR (pv))) {
            SHFixupAddr (&EVAL_PTR (pv));
        }
// BUGBUG need to handle 64 bit offsets
        EVAL_LONG (pv) = (ULONG)EVAL_PTR_OFF (pv);
        SetNodeType (pv, T_ULONG);
    }

    if (CV_IS_PRIMITIVE (EVAL_TYP (pv)) && !CV_IS_PRIMITIVE (dType)) {
        goto badcast;
    }

    // We are casting pointers to integral/real types and among the integral
    // types.

    DASSERT (!EVAL_IS_PTR (pv));
    DASSERT (CV_IS_PRIMITIVE (dType));
    DASSERT (CV_IS_PRIMITIVE (EVAL_TYP (pv)));

    // Cast to/from FORTRAN complex is not supported
    if (CV_TYP_IS_COMPLEX (EVAL_TYP (pv)) || CV_TYP_IS_COMPLEX (dType)) {
        goto badcast;
    }

    // If we're converting from a real to a non-real, first convert
    // to a double, then to a long.  Then the node is a long and the
    // desired type is an integer of some sort; let the code below
    // do its work.

    if ((CV_TYP_IS_REAL (EVAL_TYP (pv))) && (!CV_TYP_IS_REAL (dType))) {

        // do not allow fp to QUAD conversions yet
        if (fIsQuadType (dType))
            goto badcast;

        //
        // there is at least 1 case I know of where the T_REAL32 case underflows and causes
        // c0000090 (alpha only)
        // to evaluate this cast in the debugger will crash the debugger
        // windbg test case:
        // run_ eecpp 7t3
        //
        // so handle the exception and try to do the right thing
        //

        if (Evaluating) {
            switch (EVAL_TYP (pv)) {
                case T_REAL32:
                    _try {
                        EVAL_LONG (pv) = (long)EVAL_FLOAT (pv);
                    }
                    _except(ExFilterlongEVAL_FLOAT(GetExceptionInformation(), pv)){

                        //
                        // if we get here, we cound not successfully convert
                        //

                        goto badcast;
                    }
                    break;

                case T_REAL64:
                    EVAL_LONG (pv) = (long)EVAL_DOUBLE (pv);
                    break;

                case T_REAL80:
                    EVAL_LONG (pv) = LongFromFloat10 ( EVAL_LDOUBLE (pv) );
                    break;
            }
        }
        SetNodeType (pv, T_LONG);
    }

    // Otherwise, if we're converting a non-real to a real, first
    // convert to a long, then to a double.  Then the node is a
    // double and the desired type is a real of some sort and the
    // code below works normally.

    else if ((!CV_TYP_IS_REAL (EVAL_TYP (pv))) && (CV_TYP_IS_REAL (dType))) {

        // do not allow QUAD to fp conversions yet
        if (fIsQuadType (EVAL_TYP (pv)))
            goto badcast;

        if (EVAL_TYP (pv) != T_ULONG) {
            CastNode (pv, T_LONG, T_LONG);
            if (Evaluating) {
                EVAL_DOUBLE (pv) = (double)EVAL_LONG (pv);
            }
        }
        else {
            if (Evaluating)
                EVAL_DOUBLE (pv) = (double)EVAL_ULONG (pv);
        }
        SetNodeType (pv, T_REAL64);
    }
    if (EVAL_TYP (pv) == T_SEGMENT) {
        SetNodeType (pv, T_USHORT);
    }

    // Increment or decrement the size of the node's type until it
    // matches the size of the desired type.

    size = (ulong ) TypeSizePrim (dType);
    if ((ulong )TypeSizePrim (EVAL_TYP (pv)) < size) {
        while ((ulong )TypeSizePrim (EVAL_TYP (pv)) < size) {
            CvtSizeUp (pv);
        }
    }
    else if ((ulong )TypeSizePrim (EVAL_TYP (pv)) > size) {
        while ((ulong )TypeSizePrim (EVAL_TYP (pv)) > size) {
            CvtSizeDown (pv);
        }
    }
    return (SetNodeType (pv, dType));

badcast:
    pExState->err_num = ERR_TYPECAST;
    return (FALSE);
}

/***    ExFilterlongEVAL_FLOAT - fix things so we can continue execution on
 *      certain exceptions and still get the right answer, in this case
 *      (long) EVAL_FLOAT
 *
 *      Entry
 *
 *          EXCEPTION_POINTERS *exp, peval_t ppv
 *
 *      Exit
 *
 *          EXCEPTION_CONTINUE_EXECUTION or EXCEPTION_EXECUTE_HANDLER
 *
 */

LONG ExFilterlongEVAL_FLOAT(EXCEPTION_POINTERS *exp, peval_t ppv)
{
    //
    // for overflow and underflow, we can cheat and do the right thing
    //

#define MAX_LONG  0x7FFFFFFFL
#define MAX_ULONG 0xFFFFFFFFL

    switch(exp->ExceptionRecord->ExceptionCode){
        case EXCEPTION_FLT_UNDERFLOW:
            EVAL_FLOAT(ppv) = (float)0.0;
            break;
        case EXCEPTION_FLT_OVERFLOW:
            EVAL_FLOAT(ppv) = (float)MAX_LONG;
            break;
        default:
            return EXCEPTION_EXECUTE_HANDLER;
            break;
    }
    return EXCEPTION_CONTINUE_EXECUTION;
}



/***    TypeNodes - Add type information to child nodes
 *
 *      TypeNodes (op, pvL, pvR)
 *
 *      Entry   op = operator (OP_...)
 *              pvL = pointer to left-child value
 *              pvR = pointer to right-child valuee (or NULL if unary operator)
 *
 *      Exit
 *
 *      Returns none
 *
 *      Adds type information to child nodes for the specified operator.
 *
 */


void
TypeNodes (
    op_t op,
    peval_t pvL,
    peval_t pvR
    )
{
    register CV_typ_t       typT;

    //DASSERT ((op != OP_lparen) && (op != OP_rparen));
    //DASSERT (pvL != NULL);
    //DASSERT (EVAL_STATE (pvL) != EV_ident);
    //DASSERT ((pvR == NULL) || (EVAL_STATE (pvR) != EV_ident));

    // Type the children according to the operator.

    switch (op) {
        case OP_plus:
        case OP_minus:

            // ANSI 3.3.6
            // If pointers are involved (pointer arithmetic), no
            // conversion of the operands is necessary.  Otherwise,
            // the 'usual arithmetic conversions' are performed.

            if ((!EVAL_IS_PTR (pvL)) && (!EVAL_IS_PTR (pvR))) {
                // perform usual arithmetic conversions
                goto L_uac;
            }
            break;

        case OP_shl:
        case OP_shr:
        case OP_segop:          // Special

            // ANSI 3.3.7
            // An integral promotion is performed on each operand.

            typT = IntegralPromote (EVAL_TYP (pvL));
            CastNode (pvL, typT, typT);
            typT = IntegralPromote (EVAL_TYP (pvR));
            CastNode (pvR, typT, typT);
            break;

        case OP_lt:             // ANSI 3.3.8
        case OP_lteq:
        case OP_gt:
        case OP_gteq:
        case OP_eqeq:           // ANSI 3.3.9
        case OP_bangeq:

            // Either both operands are arithmetic, both are
            // pointers, or one is a pointer and the other is
            // integral (M00ANSI: ANSI only allows equality
            // comparisons with the 'null pointer constant' and
            // does not allow relational comparisons of pointers
            // with integers at all).  If both are arithmetic,
            // the usual arithmetic conversions are performed.
            // If either is a pointer, we call a special routine.

            if ((EVAL_IS_PTR (pvL)) || (EVAL_IS_PTR (pvR))) {
                CastRelational (pvL, pvR);
            }
            else {
                // perform usual arithmetic conversions
                goto L_uac;
            }
            break;

        case OP_bang:           // ANSI 3.3.3.3
        case OP_andand:         // ANSI 3.3.13
        case OP_oror:           // ANSI 3.3.14
        case OP_fetch:          // ANSI 3.3.3.2
        case OP_addrof:
        case OP_lbrack:         // ANSI 3.3.2.1
        case OP_pointsto:       // ANSI 3.3.2.3
        case OP_dot:
        case OP_by:             // Special
        case OP_wo:
        case OP_dw:

            // No conversion necessary.

            break;

        case OP_eq:             // ANSI 3.3.16
        case OP_multeq:
        case OP_diveq:
        case OP_modeq:
        case OP_pluseq:
        case OP_minuseq:
        case OP_shleq:
        case OP_shreq:
        case OP_andeq:
        case OP_xoreq:
        case OP_oreq:

            // These operators not yet supported (i.e., we
            // shouldn't get here).

            DASSERT(FALSE);
            break;

        case OP_negate:         // ANSI 3.3.3.3
        case OP_uplus:
        case OP_tilde:

            // An integral promotion is performed on the operand.

            typT = IntegralPromote (EVAL_TYP (pvL));
            CastNode (pvL, typT, typT);
            break;

        case OP_mult:           // ANSI 3.3.5
        case OP_div:
        case OP_mod:
        case OP_and:            // ANSI 3.3.10
        case OP_xor:            // ANSI 3.3.11
        case OP_or:             // ANSI 3.3.12

            // The usual arithmetic conversions are performed on each
            // operand.

L_uac:
            typT = PerformUAC (EVAL_TYP (pvL), EVAL_TYP (pvR));
            CastNode (pvL, typT, typT);
            CastNode (pvR, typT, typT);
            break;

        default:
            DASSERT(FALSE);
            ;
    }
}




/***    ValidateNodes - Ensure that operator node has valid operands
 *
 *      fValid = ValidateNodes (op, pvL, pvR)
 *
 *      Entry   op = operator (OP_...)
 *              pvL = pointer to left-child value
 *              pvR = pointer to right-child value (or NULL if unary operator)
 *
 *      Exit    pExState->err_num = ERR_TYPEINCOMPAT if incorrect operands
 *
 *      Returns TRUE if operands are valid for operator
 *              FALSE if operands are not valid
 */

bool_t
ValidateNodes (
    op_t op,
    peval_t pvL,
    peval_t pvR
    )
{
    register int     i;
    register opc_t   opc;

    if ((opc = (opc_t) mpopopc[op]) == OPC_null) {
        return (TRUE);
    }

    pExState->err_num = 0;

    // if the operand type in the mapping table is not OPC_null, then
    // the operands must be checked

    for (i = 0; i < C_OPTYP_LEGAL; i++) {
        // search table looking for operator class
        if ((op_t)optypLegal[i].opc == opc) {
            if ((TyperCheckType (pvL, optypLegal[i].atLeft)) &&
              ((pvR == NULL) || (TyperCheckType (pvR, optypLegal[i].atRight)))) {
                return (TRUE);
            }
        }
    }
    if (pExState->err_num == 0)
        pExState->err_num = ERR_TYPEINCOMPAT;
    return (FALSE);
}




/**     CastRelational - Cast child nodes for relational operators
 *
 *      CastRelational (pvL, pvR)
 *
 *      Entry   pvL = pointer to left value
 *              pvR = pointer to right child value
 *
 *      Exit    both values cast to the proper types
 *
 *      Returns none
 *
 * DESCRIPTION
 *       Casts two child nodes when one or both are pointers and the
 *       operator is a relational or equality operator ('<', '<=', '>',
 *       '>=', '==', '!=').
 *
 *       ANSI does not discuss this; the rules used are described below.
 */


void
CastRelational (
    peval_t pvL,
    peval_t pvR
    )
{
    CV_typ_t            typL, typR;
    register CV_typ_t   typRes;

    // Since we're going to be doing comparisons on the pointers, we
    // don't really care what sort of thing they point TO (M00ANSI:
    // ANSI says we have to make sure they both point to the same
    // type of object).  Therefore, make sure the pointers are primitive
    // (T_PCHAR, T_PFCHAR, T_32PCHAR, or T_32PFCHAR).

    if (EVAL_IS_PTR (pvL))
        CvtPtr (pvL, EVAL_TYP (pvL));

    if (EVAL_IS_PTR (pvR))
        CvtPtr (pvR, EVAL_TYP (pvR));

    // Fetch the types of the nodes.

    typL = EVAL_TYP (pvL);
    typR = EVAL_TYP (pvR);

    //DASSERT ((CV_IS_PRIMITIVE(typL)) && (CV_IS_PRIMITIVE(typR)));

    // [cuda#2616 4/6/93 mikemo] 32-bit pointers need to be handled
    // differently from 16-bit ones.  If either pointer is far 32-bit,
    // then cast both nodes to far 32-bit; else if either is near 32-bit,
    // then cast both nodes to near 32-bit.
    if (CV_TYP_IS_FPTR32(typL) || CV_TYP_IS_FPTR32(typR))
        typRes = T_32PFCHAR;
    else if (CV_TYP_IS_NPTR32(typL) || CV_TYP_IS_NPTR32(typR))
        typRes = T_32PCHAR;
    else {
        // The cases are:
        //
        // (1)  near ptr    cmp     near ptr
        // (2)  near ptr    cmp     far ptr
        // (3)  far ptr     cmp     far ptr
        // (4)  near ptr    cmp     char, uchar, short, ushort, int, uint
        // (5)  near ptr    cmp     long, ulong
        // (6)  far ptr     cmp     char, uchar, short, ushort, int, uint
        // (7)  far ptr     cmp     long, ulong
        //
        // Cases (1) and (3) require no casting; case (2) requires the
        // near pointer to be cast to a far pointer (DS-extended).  Case
        // (4) requires the integral operand to be cast to a near pointer.
        // Case (5) requires both operands to be cast to far pointers.
        // Cases (6) and (7) require the integral operand to be cast to
        // a far pointer.
        //
        // Note that all this can be reduced to: if either operand has
        // size 4 (long, ulong) or is a far pointer, cast both
        // operands to far pointers; otherwise cast both operands to
        // near pointers.
        //

        typRes = T_PCHAR;                           // Assume //

        if ((CV_TYP_IS_FPTR(typL)) ||
          (CV_TYP_IS_FPTR(typR)) ||
          ((CV_TYP_IS_DIRECT(typL)) && (TypeSizePrim(typL) == 4)) ||
          ((CV_TYP_IS_DIRECT(typR)) && (TypeSizePrim(typR) == 4))) {
            typRes = T_PFCHAR;
        }
    }

    // Now cast both nodes to the resultant type and we're done.
    //

    CastNode (pvL, typRes, typRes);
    CastNode (pvR, typRes, typRes);
}




/**     TyperCheckType - Is a value's type one of a specified aggregate type?
 *
 *      fTypeOK = TyperCheckType (pv, at)
 *
 *      Entry   pv = pointer to node whose type is to be checked
 *              at = aggregate type flags (ANSI_...)
 *
 *      Exit    none
 *
 *      Returns TRUE if node's type is acceptable for the aggregate type
 *              FALSE if the type is not acceptable
 */


ulong
TyperCheckType (
    peval_t pv,
    ulong  at
    )
{
    register CV_typ_t   typ;

    if (EVAL_IS_REF (pv)) {
        // if the node is a reference, then the type is really that of the
        // underlying type

        typ = PTR_UTYPE (pv);
    }
    else if (EVAL_IS_BITF (pv)) {
        // if the node is a bitfield, then the type is really that of the
        // underlying type

        typ = BITF_UTYPE (pv);
    }
    else {
        typ = EVAL_TYP (pv);
    }

    // If the type is a pointer, it is compatible if the aggregate
    // type permits pointers.

    if (EVAL_IS_PTR (pv) || EVAL_IS_ADDR(pv)) {
        // We should exclude void ptrs as we don't know how to do
        // arithmetic on them
        if (CV_IS_PRIMITIVE(typ) && CV_TYPE(typ) == CV_SPECIAL && CV_SUBT(typ) == CV_SP_VOID) {
            return FALSE ;
        }
        else {
            return (at & BTYP_PTR);
        }
    }
    if (!CV_IS_PRIMITIVE (typ)) {
        // If the (underlying) type is not primitive, it cannot match
        return (FALSE);
    }

    // The type is not a pointer (mode is CV_TM_DIRECT).  If the basic
    // type is CV_TM_SIGNED or CV_TM_UNSIGNED, it is compatible iff the
    // aggregate type allows ints (any flavor).

    if ((CV_TYP_IS_SIGNED (typ)) || (CV_TYP_IS_UNSIGNED (typ)))
        return (at & BTYP_INT);

    // If the type is a real of some sort, it is compatible iff the
    // aggregate type contains BTYP_FLOAT.

    if (CV_TYP_IS_REAL (typ))
        return (at & BTYP_FLOAT);

    // M00UNDONE: void, enums.

    // return FALSE for FORTRAN COMPLEX types
    pExState->err_num = ERR_TYPESUPPORT;
    return (FALSE);
}




/**     CvtPtr - Convert pointer to primitive pointer
 *
 *      CvtPtr (pv, target)
 *
 *      Entry   pv = pointer to value to be converted
 *              typTrgt = target type
 *
 *      Exit    value converted to T_PCHAR, T_PFCHAR or T_PHCHAR
 *
 *      Returns None
 *
 */


void
CvtPtr (
    peval_t pv,
    CV_typ_t typTrgt
    )
{
    register CV_typ_t   typ;

    switch (EVAL_PTRTYPE (pv)) {
        case CV_PTR_NEAR:
            typ = T_PCHAR;
            break;

        case CV_PTR_FAR:
            typ = T_PFCHAR;
            break;

        case CV_PTR_HUGE:
            typ = T_PHCHAR;
            break;

        case CV_PTR_NEAR32:
            typ = T_32PCHAR;
            break;

        case CV_PTR_FAR32:
            typ = T_32PFCHAR;
            break;

        case CV_PTR_64:
            typ = T_64PCHAR;
            break;

        default:
            if (CV_IS_PRIMITIVE (typTrgt) &&
              (TypeSizePrim(typTrgt) < 4) &&
              CV_TYP_IS_DIRECT (typTrgt) &&
              (CV_TYPE (typTrgt) < CV_REAL)) {
               typ = T_PCHAR;
            }
            else {
                if (EVAL_STATE (pv) != EV_lvalue) {
                    if (!NormalizeBase (pv)) {
                        if ((pExState->err_num != 0) ||
                          !(CV_TYP_IS_NPTR (typTrgt))) {
                            break;
                        }
                    }
                }
                typ = pExState->state.f32bit ? T_32PCHAR : T_PFCHAR;
            }
            break;
    }
    SetNodeType (pv, typ);
}




/**     CvtSizeDown - Convert size of value's type down one step
 *
 *      CvtSizeDown (pv)
 *
 *      Entry   pv = pointer to value
 *
 *      Exit    value is cast to next smaller size
 *
 *      Returns none
 *
 *      Converts the size of the node's type down one step:
 *
 *          type far *  -> type *
 *          ldouble     -> double
 *          double      -> float
 *          (u)long     -> (u)short
 *          (u)short    -> (u)char
 */


void
CvtSizeDown (
    peval_t pv
    )
{
    register CV_typ_t   typ;

    if (CV_TYP_IS_FPTR (EVAL_TYP (pv))) {

        // Far ptr -> near ptr.

        if (Evaluating) {
            EVAL_PTR_SEG (pv) = GetSegmentRegister(pExState->hframe, REG_DS); //M00FLAT32
        }
        typ = CV_NEWMODE (EVAL_TYP (pv), CV_TM_NPTR);
    }
    else if (CV_TYP_IS_FPTR32 (EVAL_TYP (pv))) {
        typ = CV_NEWMODE (EVAL_TYP (pv), CV_TM_NPTR32);
    }
    else {
        switch (EVAL_TYP (pv)) {
            case T_REAL80:
                typ = T_REAL64;
                break;

            case T_REAL64:
                typ = T_REAL32;
                break;

            case T_QUAD:
            case T_INT8:
                typ = T_LONG;
                break;

            case T_UQUAD:
            case T_UINT8:
                typ = T_ULONG;
                break;

            case T_LONG:
            case T_INT4:
                typ = T_SHORT;
                break;

            case T_ULONG:
            case T_UINT4:
                typ = T_USHORT;
                break;

            case T_SHORT:
            case T_INT2:
                typ = T_CHAR;
                break;

            case T_USHORT:
            case T_UINT2:
                typ = T_UCHAR;
                break;

            default:
                DASSERT(FALSE);
                return;
        }
        if (Evaluating) {
            switch (EVAL_TYP (pv)) {
                case T_REAL80:
                    EVAL_DOUBLE (pv) = DoubleFromFloat10 ( EVAL_LDOUBLE (pv) );
                    break;

                case T_REAL64:
                    EVAL_FLOAT (pv) = (float)EVAL_DOUBLE (pv);
                    break;

                case T_QUAD:
                case T_INT8:
                    EVAL_LONG (pv) = (long)EVAL_QUAD (pv);
                    break;

                case T_UQUAD:
                case T_UINT8:
                    EVAL_ULONG (pv) = (ulong)EVAL_UQUAD (pv);
                    break;

                case T_LONG:
                case T_INT4:
                    EVAL_SHORT (pv) = (short)EVAL_LONG (pv);
                    break;

                case T_ULONG:
                case T_UINT4:
                    EVAL_USHORT (pv) = (ushort)EVAL_ULONG (pv);
                    break;

                case T_SHORT:
                case T_INT2:
                    EVAL_CHAR (pv) = (char)EVAL_SHORT (pv);
                    break;

                case T_USHORT:
                case T_UINT2:
                    EVAL_UCHAR (pv) = (uchar)EVAL_USHORT (pv);
                    break;
            }
        }
    }
    SetNodeType (pv, typ);
}




/**     CvtSizeUp - Convert size of node's type up one step
 *
 *      CvtSizeUp (pv)
 *
 *      Entry   pv = pointer to value
 *
 *      Exit    value is cast to next larger size
 *
 *      Returns none
 *
 *      Converts the size of the node's type up one step:
 *
 *          (u)char     -> (u)short
 *          (u)short    -> (u)long
 *          float       -> double
 *          double      -> ldouble
 *          type *      -> type far *
 */


void
CvtSizeUp (
    peval_t pv
    )
{
    register CV_typ_t   typ;

    if (CV_TYP_IS_NPTR (EVAL_TYP (pv))) {

        // Near ptr -> far ptr.
        //
        // If the node is a constant and its offset is zero, we want
        // to make the segment zero.  Otherwise, we DS-extend (which
        // effectively does nothing since we're already carrying DS
        // around anyway).

        if (Evaluating &&
          (EVAL_STATE (pv) == EV_constant) &&
          (EVAL_PTR_OFF (pv) == 0)) {
            EVAL_PTR_SEG (pv) = 0;
        }
        typ = CV_NEWMODE (EVAL_TYP (pv), CV_TM_FPTR);
    }
    else {
        switch (EVAL_TYP (pv)) {
            case T_CHAR:
            case T_RCHAR:
            case T_INT1:
                typ = T_SHORT;
                break;

            case T_UCHAR:
            case T_UINT1:
                typ = T_USHORT;
                break;

            case T_SHORT:
            case T_INT2:
                typ = T_LONG;
                break;

            case T_USHORT:
            case T_UINT2:
                typ = T_ULONG;
                break;

            case T_LONG:
            case T_INT4:
                typ = T_QUAD;
                break;

            case T_ULONG:
            case T_UINT4:
                typ = T_UQUAD;
                break;

            case T_REAL32:
                typ = T_REAL64;
                break;

            case T_REAL64:
                typ = T_REAL80;
                break;

            default:
                DASSERT(FALSE);
                return;
        }
        if (Evaluating) {
            switch (EVAL_TYP (pv)) {
                case T_CHAR:
                case T_RCHAR:
                case T_INT1:
                    EVAL_SHORT (pv) = EVAL_CHAR (pv);
                    break;

                case T_UCHAR:
                case T_UINT1:
                    EVAL_USHORT (pv) = EVAL_UCHAR (pv);
                    break;

                case T_SHORT:
                case T_INT2:
                    EVAL_LONG (pv) = EVAL_SHORT (pv);
                    break;

                case T_USHORT:
                case T_UINT2:
                    EVAL_ULONG (pv) = EVAL_USHORT (pv);
                    break;

                case T_LONG:
                case T_INT4:
                    EVAL_QUAD (pv) = EVAL_LONG (pv);
                    break;

                case T_ULONG:
                case T_UINT4:
                    EVAL_UQUAD (pv) = EVAL_ULONG (pv);
                    break;

                case T_REAL32:
                    EVAL_DOUBLE (pv) = EVAL_FLOAT (pv);
                    break;

                case T_REAL64:
                    EVAL_LDOUBLE (pv) = Float10FromDouble ( EVAL_DOUBLE (pv) );
            }
        }

    }
    SetNodeType (pv, typ);
}




/**     IntegralPromote - Perform an integral promotion
 *
 *      typPromoted = IntegralPromote (typ)
 *
 *      Entry   typ = type index
 *
 *      Exit    none
 *
 *      Returns type index of promoted type
 *
 * DESCRIPTION
 *       Promotes type as per ANSI spec, section 3.2.1.1:
 *
 *       "If an int can represent all values of the original type,
 *       the value is converted to an int; otherwise it is converted
 *       to an unsigned int."
 *
 *       For us this means:
 *
 *       char   -> int
 *       uchar  -> int
 *       short  -> int
 *       ushort -> uint     (if in 16-bit state)
 *       ushort -> int      (if in 32-bit state)
 *       int    -> int      (no conversion)
 *       uint   -> uint     (no conversion)
 *       long   -> long     (no conversion)
 *       ulong  -> ulong    (no conversion)
 *       QUAD   -> QUAD     (no conversion)
 *       UQUAD  -> UQUAD    (no conversion)
 *       The other conversions are the same as per ANSI.
 */


CV_typ_t FASTCALL
IntegralPromote (
    CV_typ_t typ
    )
{
    //DASSERT (CV_IS_PRIMITIVE (typ));
    //DASSERT (CV_TYP_IS_DIRECT (typ));

    if (CV_IS_PRIMITIVE(typ) &&
        !CV_TYP_IS_REAL (typ) &&
        !CV_TYP_IS_COMPLEX (typ)) {
        if (pExState->state.f32bit) {
            switch (typ) {
                case T_CHAR:
                case T_RCHAR:
                case T_UCHAR:
                case T_INT1:
                case T_UINT1:
                case T_SHORT:
                case T_INT2:
                case T_UINT2:
                case T_USHORT:
                    typ = T_INT4;
                    break;
                case T_INT4:
                case T_UINT4:
                case T_LONG:
                case T_ULONG:
                case T_INT8:
                case T_UINT8:
                case T_QUAD:
                case T_UQUAD:
                default:
                    break;
            }
        }
        else {
            switch (typ) {
                case T_CHAR:
                case T_RCHAR:
                case T_UCHAR:
                case T_SHORT:
                case T_INT1:
                case T_UINT1:
                    typ = T_INT2;
                    break;
                case T_INT4:
                    typ = T_LONG;
                    break;
                case T_UINT4:
                    typ = T_ULONG;
                    break;
                case T_USHORT:
                    typ = T_UINT2;
                case T_INT2:
                case T_UINT2:
                case T_LONG:
                case T_ULONG:
                default:
                    break;
            }
        }
    }

    return (typ);
}




/**     PerformUAC - Perform 'usual arithmetic conversions'
 *
 *      typRes = PerformUAC (typ1, typ2)
 *
 *      Entry   typ1 = type index of first node
 *              typ2 = type index of second node
 *
 *      Exit    none
 *
 *      Returns typRes = type index to which both operands should be
 *              converted
 *
 *      See ANSI spec, section 3.2.1.5.
 */


CV_typ_t
PerformUAC (
    CV_typ_t typ1,
    CV_typ_t typ2
    )
{
    register CV_typ_t       typr;

    // Only permissible types for entry are 'arithmetic' types, i.e.,
    // chars, integers and floats.

    //DASSERT (!CV_TYP_IS_PTR (typ1));
    //DASSERT (!CV_TYP_IS_PTR( typ2));

    if ((typ1 == T_REAL80) || (typ2 == T_REAL80)) {
        // If either operand has type long double, the other
        // operand is converted to long double.

        typr = T_REAL80;
    }
    else if ((typ1 == T_REAL64) || (typ2 == T_REAL64)) {
        // If either operand has type double, the other
        // operand is converted to double.

        typr = T_REAL64;
    }
    else if ((typ1 == T_REAL32) || (typ2 == T_REAL32)) {
        // If either operand has type float, the other
        // operand is converted to float.

        typr = T_REAL32;
    }
    else {
        // The integral promotions are performed on both operands.

        typ1 = IntegralPromote (typ1);
        typ2 = IntegralPromote (typ2);
        if (pExState->state.f32bit) {
            DASSERT (typ1 != T_INT2 && typ1 != T_UINT2 &&
                     typ2 != T_INT2 && typ2 != T_UINT2);
        }
        else {
            DASSERT (typ1 != T_INT4 && typ1 != T_UINT4 &&
                     typ2 != T_INT4 && typ2 != T_UINT4);
        }
        if ((typ1 == T_UQUAD) || (typ2 == T_UQUAD)) {
            // If either operand has type unsigned quad int, the other
            // operand is converted to unsigned quad int.

            typr = T_UQUAD;
        }
        else if (((typ1 == T_QUAD) && (typ2 == T_UINT8)) ||
                 ((typ2 == T_QUAD) && (typ1 == T_UINT8))) {
            // If one operand is 64bit long int and the other is 64bit
            // unsigned int both operands are converted to
            // 64bit unsigned long int.

            typr = T_UQUAD;
        }
        else if ((typ1 == T_QUAD) || (typ2 == T_QUAD)) {
            // If either operand has type quad int, the other
            // operand is converted to quad int.

            typr = T_QUAD;
        }
        else if ((typ1 == T_UINT8) || (typ2 == T_UINT8)) {
            // If either operand has type 64bit unsigned int, the other
            // operand is converted to 64bit unsigned int.

            typr = T_UINT8;
        }
        else if ((typ1 == T_ULONG) || (typ2 == T_ULONG)) {
            // If either operand has type unsigned long int, the other
            // operand is converted to unsigned long int.

            typr = T_ULONG;
        }
        else if (((typ1 == T_LONG) && (typ2 == T_UINT4)) ||
                 ((typ2 == T_LONG) && (typ1 == T_UINT4))) {
            // If one operand is long int and the other is unsigned int,
            // and long int cannot represent all the values in an unsigned
            // int, both operands are converted to unsigned long int.

            typr = T_ULONG;
        }
        else if (((typ1 == T_LONG) && (typ2 == T_UINT2)) ||
                 ((typ2 == T_LONG) && (typ1 == T_UINT2))) {
            // If one operand is long int and the other is unsigned int,
            // and long int can represent all the values in an unsigned
            // int, the unsigned int is convert to long int.

            typr = T_LONG;
        }
        else if ((typ1 == T_LONG) || (typ2 == T_LONG)) {
            // If either operand has type long int, the other
            // operand is converted to long int.

            typr = T_LONG;
        }
        else if ((typ1 == T_UINT4) || (typ2 == T_UINT4)) {
            // If either operand has type unsigned int4, the other
            // operand is converted to unsigned int4.

            typr = T_UINT4;
        }
        else if ((typ1 == T_UINT2) || (typ2 == T_UINT2)) {
            // If either operand has type unsigned int, the
            // other operand is converted to unsigned int.

            typr = T_UINT2;
        }
        else {
            // Both operands have type int.

            typr = (pExState->state.f32bit) ? T_INT4 : T_INT2;
        }
    }
    return (typr);
}



/**     NormalizeBase - convert based pointer to far pointer
 *
 *      fSuccess = NormalizeBase (paddr, pv)
 *
 *      Entry   paddr = pointer to address to place result in
 *              pv = pointer to value node for based value
 *
 *      Exit    *paddr = far pointer
 *
 *      Returns TRUE if far pointer generated
 *              FALSE if error
 *
 *      This routine is used whenever a based pointer is dereferenced
 *      for load or store.
 */


bool_t
NormalizeBase (
    peval_t pv
    )
{
    eval_t      vBase;
    peval_t     pvBase;
    register ulong       bptrtyp;
    register bool_t retval = TRUE;

    //  check for _based (void) and short circuit with an error code

    switch (bptrtyp = EVAL_PTRTYPE (pv)) {
        case CV_PTR_BASE_TYPE:
            // do not normalize based on type

            //pExState->err_num = ERR_NEEDSBIRD;
            //return (FALSE);
            return (TRUE);

        case CV_PTR_BASE_SELF:
            //  check for _based((_segment) _self) and build the address now.
            //   There is not a symbol to get information for.
            //
            if (Evaluating) {
                SHFixupAddr (&EVAL_PTR(pv));
                SHFixupAddr (&EVAL_SYM (pv));
                EVAL_PTR_SEG (pv) = EVAL_SYM_SEG (pv);
            }
            break;

        case CV_PTR_BASE_SEG:
            if (Evaluating) {
                UOFFSET saveOffset = EVAL_PTR_OFF(pv);
                EVAL_PTR_OFF (pv) = 0;
                EVAL_PTR_SEG (pv) = PTR_BSEG (pv);
                EVAL_PTR_EMI (pv) = pCxt->addr.emi;
                ADDR_IS_LI (EVAL_PTR (pv)) = TRUE;
                SHFixupAddr (&EVAL_PTR(pv));
                EVAL_PTR_OFF (pv) = saveOffset;
            }
            break;

        case CV_PTR_BASE_SEGVAL:
            if (Evaluating) {
                // gotta check if we are dealing with a Base on
                // "_stack" - see if original type record base sym
                // is register ss
                // sps - 1-6-93
                HTYPE hType;
                plfEasy pType;
                SYMPTR  pSym;

                if ((hType = THGetTypeFromIndex (EVAL_MOD (pv), EVAL_TYP(pv))) == 0) {
                    retval = FALSE;
                    break;
                }

                pType = (plfEasy)(&((TYPPTR)(MHOmfLock (hType)))->leaf);
                DASSERT((pType->leaf == LF_POINTER) &&
                (((plfPointer)pType)->attr.ptrtype == CV_PTR_BASE_SEGVAL));
                pSym = (SYMPTR)(&((plfPointer)pType)->pbase.Sym);
                if (pSym->rectyp == S_REGISTER) {
                    DASSERT(((REGPTR)pSym)->reg == CV_REG_SS);
                    EVAL_PTR_SEG (pv) = GetSegmentRegister(pExState->hframe, REG_SS);
                    ADDR_IS_LI (EVAL_PTR (pv)) = FALSE;
                    MHOmfUnLock (hType);
                    break;
                }
                MHOmfUnLock (hType);
            }
            else {
                break;
            }


        default:
            pvBase = &vBase;
            *pvBase = *pv;
            CLEAR_EVAL_FLAGS (pvBase);
            EVAL_STATE (pvBase) = EV_lvalue;
            SetNodeType (pvBase, PTR_STYPE (pv));
            EVAL_SYM (pvBase) = PTR_ADDR (pv);
            switch (PTR_BSYMTYPE (pv)) {
                case S_BPREL16:
                case S_BPREL32:
                    EVAL_IS_BPREL (pvBase) = TRUE;
                    break;
            }

            if (Evaluating) {
                if (!LoadSymVal (pvBase)) {
                    retval = FALSE;
                    break;
                }
                SHFixupAddr (&EVAL_PTR(pv));
            }

            switch (bptrtyp) {
                case CV_PTR_BASE_VAL: {
                    // FOO _based (fp)*    or FOO _based (segvar)*
                    // The only types of variables which we may base
                    // on are near pointers, far pointers or _segment.
                    //
                    // Result addr =
                    // type == _segment :: VAL (segvar)  :> bp
                    // type == far ptr  :: SEG (VAL (fp)) :> bp+OFF (VAL (fp))
                    // type == near ptr :: DS :> bp+OFF (VAL (np))

                    unsigned uptrtypeBase = EVAL_PTRTYPE ( pvBase );

                    if (EVAL_TYP (pvBase) != T_SEGMENT) {


                        if (uptrtypeBase > CV_PTR_FAR && uptrtypeBase != CV_PTR_NEAR32) {
                            DASSERT (FALSE);
                            retval = FALSE;
                            break;
                        }
                    }
                    if (Evaluating) {
                        if (EVAL_TYP (pvBase) == T_SEGMENT) {
                            EVAL_PTR_SEG (pv) = EVAL_SHORT (pvBase);
                        }
                        else if (uptrtypeBase == CV_PTR_NEAR) {
                            EVAL_PTR_SEG (pv) = GetSegmentRegister(pExState->hframe, REG_DS); //M00FLAT32
                            EVAL_PTR_OFF (pv) =
                              (CV_uoff16_t)(EVAL_PTR_OFF (pv) + EVAL_PTR_OFF (pvBase));
                        }
                        else if (uptrtypeBase == CV_PTR_FAR) {
                            EVAL_PTR_SEG (pv) = EVAL_PTR_SEG (pvBase);
                            EVAL_PTR_OFF (pv) =
                              (CV_uoff16_t)(EVAL_PTR_OFF (pv) + EVAL_PTR_OFF (pvBase));
                        }
                        else if (uptrtypeBase == CV_PTR_NEAR32) {
                            EVAL_PTR_SEG (pv) = EVAL_PTR_SEG (pvBase);
                            EVAL_PTR_OFF (pv) =
                              (CV_uoff32_t)(EVAL_PTR_OFF (pv) + EVAL_PTR_OFF (pvBase));
                        }
                        else {
                            retval = FALSE;
                        }
                    }
                    break;
                }

                case CV_PTR_BASE_SEGVAL:
                    //    FOO _based (_segment (fp))//

                    DASSERT (EVAL_PTRTYPE (pvBase) <= CV_PTR_FAR);
                    if (EVAL_PTRTYPE (pvBase) > CV_PTR_FAR) {
                        retval = FALSE;
                    }
                    if (Evaluating) {
                        if (EVAL_PTRTYPE (pvBase) == CV_PTR_NEAR) {
                            EVAL_PTR_SEG (pv) = GetSegmentRegister(pExState->hframe, REG_DS);
                        }
                        else if (EVAL_PTRTYPE (pvBase) == CV_PTR_FAR) {
                            EVAL_PTR_SEG (pv) = EVAL_PTR_SEG (pvBase);
                        }
                    }
                    break;

                case CV_PTR_BASE_SEGADDR:
                    // FOO _based (_segment (&fp)) *
                    // result == SEG (ADDR (fp)) :> bp

                    if (Evaluating) {
                        if (EVAL_IS_REG (pvBase)) {
                            EVAL_PTR_SEG (pv) = EVAL_USHORT (pvBase);
                        }
                        else {
                            SHFixupAddr(&EVAL_SYM(pvBase));
                            EVAL_PTR_SEG (pv) = EVAL_SYM_SEG (pvBase);
                        }
                    }
                    break;

                case CV_PTR_BASE_SEG:
                case CV_PTR_BASE_SELF:
                case CV_PTR_BASE_TYPE:
                case CV_PTR_BASE_ADDR:
                default:
                    DASSERT (FALSE);
                    retval = FALSE;
                    break;

            }
    }
    EVAL_PTRTYPE(pv) = CV_PTR_FAR32;
    return (retval);
}




/***    DeNormalizePtr - convert far pointer to based pointer
 *
 *      fSuccess = DeNormalizePtr (pv, pvB)
 *
 *      Entry   pv = pointer to value to denormalize
 *              pvB = pointer to value containing normalize data
 *
 *      Exit    EVAL_PTR (pv) = based pointer
 *
 *      Returns TRUE if based pointer generated
 *              FALSE if error
 *
 *      This routine is used whenever a value is stored into a based pointer
 */


bool_t
DeNormalizePtr (
    peval_t pv,
    peval_t pvB
    )
{
    eval_t      vBase;
    peval_t     pvBase = &vBase;
    CV_typ_t    typLHS = EVAL_TYP (pvB);
    bool_t      retval = TRUE;

    // If we are assigning in a char or a ushort then copy the
    // value straight over without change

    if (CV_IS_PRIMITIVE (typLHS) && (TypeSizePrim(typLHS) < 4) &&
      CV_TYP_IS_DIRECT (typLHS) && CV_TYPE (typLHS) < CV_REAL) {
        if (Evaluating) {
            EVAL_PTR_OFF (ST) = EVAL_USHORT (ST);
        }
        return (TRUE);
    }

    //  Now deal with the complicated items which we can have.
    //  If we get to this point the LHS must be either a near or far
    //  pointer, a long or a floating point.  If it is a floating
    //  point then we will really screw it up.
    //
    //  For all of these cases we will denormalize the expression unless
    //  it is the constant zero -- Which at present we can not detect  M00BUG

    switch (EVAL_PTRTYPE (pvB)) {
        case CV_PTR_BASE_TYPE:                // _based (void)
        case CV_PTR_BASE_SELF:                // _based ( (_segment) _self)
        case CV_PTR_BASE_SEG:                 // _based (_segment ())
        case CV_PTR_BASE_SEGVAL:              // _based ( (_segment) fp)
        case CV_PTR_BASE_SEGADDR:             // _based ( (_segment) &fp)
            //  All of these cases denormalize by ignoring the
            //  segment and leaving the offset unchanged.

            break;

        case CV_PTR_BASE_VAL:                 // _based (fp)
            //  This case will denormalize by ignoring the segment and
            //  subtracting the offset of the pointer (unless we start with
            //  the constant zero).

            *pvBase = *pv;
            CLEAR_EVAL_FLAGS (pvBase);
            EVAL_STATE (pvBase) = EV_lvalue;
            SetNodeType (pvBase, PTR_STYPE (pv));
            EVAL_SYM (pvBase) = PTR_ADDR (pv);
            DASSERT (EVAL_TYP (pvBase) == T_SEGMENT || (EVAL_PTRTYPE (pv) <= CV_PTR_FAR));
            if (Evaluating) {
                if (!LoadSymVal (pvBase)) {
                    return (FALSE);
                }
                if (EVAL_IS_BPREL (pvBase)) {
                    ResolveAddr(pvBase);
                }
            if (EVAL_IS_REGREL (pvBase)) {
                ResolveAddr(pvBase);
                }
                EVAL_STATE (pvB) = EV_lvalue;
            }
            if (Evaluating && (EVAL_TYP (pvBase) != T_SEGMENT)) {
                if (!EVAL_IS_CONST (pv) || (EVAL_ULONG (pv) != 0)) {
                    EVAL_PTR_OFF (pv) -= EVAL_PTR_OFF (pvBase);
                }
            }

            break;

        case CV_PTR_BASE_ADDR:
        default:
            DASSERT (FALSE);
            return (FALSE);
    }
    return (TRUE);
}
