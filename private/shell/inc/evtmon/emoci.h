/*****************************************************************************
    emoci.h

    Owner: DaleG
    Copyright (c) 1996-1997 Microsoft Corporation

    Op-Code Interpreter header file

*****************************************************************************/

#ifndef EMOCI_H
#define EMOCI_H

#include "emocii.h"


MSOEXTERN_C_BEGIN   // ***************** Begin extern "C" ********************

//############################################################################
//
// PUBLIC Interfaces
//
//############################################################################


// System Limits
#define msoilOciStackMax    2000



/* M  S  O  O  C  I  S */
/*----------------------------------------------------------------------------
    %%Struct: MSOOCIS
    %%Contact: daleg

    Op-Code Interpreter State structure
----------------------------------------------------------------------------*/

// The Interpreter op-code fns must always be compatible with MSOAPICALLTYPE
//  so that MSO-exported and app-local functions have the same type
#define MSOOCVAPI MSOOCV MSOAPICALLTYPE

// Define normal function: all args are evaluated and received on the stack
typedef MSOOCV (MSOAPICALLTYPE *MSOPFNOCI)(MSOOCV *pocvSP);

// Define function to return address of variable data
typedef MSOOCV *(MSOAPICALLTYPE *MSOPFNPOCV)(MSOOCII);


typedef struct _MSOOCIS
    {
    MSOPFNOCI const    *rgpfn;                          // Function ptr array
    MSOOCAD const      *rgocadArgDesc;                  // #Args/Offset arg len
    MSOOCV             *pocvSP;                         // Stack pointer
    MSOOCV             *pocvFP;                         // Frame pointer
    MSOOCV              rglStack[msoilOciStackMax];     // Call/Return stack
    unsigned char const*rgcbImmedArg;                   // Immediate arg lens
    void               *rgocvVar;                       // Variable array
    MSOPFNPOCV          pfnPocvGetVar;                  // Get addr of var data
#ifdef DEBUG
    unsigned char     **rgszFnNames;                    // Function names
#endif /* DEBUG */
    } MSOOCIS;


// Define non-evaluating function: it must interpret instructions itself
typedef MSOOCV (MSOAPICALLTYPE *MSOPFNOCI_NE)
                    (MSOOCII **ppocii, MSOOCIS *pocis);



/*************************************************************************
    Prototypes and macros
 *************************************************************************/

MSOAPI_(MSOOCIS *) MsoPocisInit(                        // Init Interp Fn tbl
    MSOOCIS            *pocis,
    MSOPFNOCI const    *rgpfn,                          // Function table
    MSOOCAD const      *rgocadArgDesc,                  // Arg descript table
    unsigned char const*rgcbImmedArg,                   // Immed arg desc tbl
    int                 ipfnMax,                        // Num builtin fns
    void               *rgocvVar,                       // Variable array
    MSOPFNPOCV          pfnPocvGetVar                   // Get addr of var data
    );

MSOAPI_(MSOOCV) MsoOcvEvalPocii(                        // Interpret 1 instr
    MSOOCII           **ppocii,
    MSOOCIS            *pocis
    );

MSOAPI_(MSOOCV *) MsoPocvPushVarArgs(                   // Push var args on stk
    int                 docvArgsFixed,
    MSOOCII           **ppocii,
    MSOOCIS            *pocis
    );


// NOTE:
// NOTE:  The macros below require the parent routine to declare pocvSP!!
// NOTE:

// Return the value of the iocv-th argment on the stack
#define MsoOcvArg(iocv) \
            (pocvSP[iocv])

// Return the value of the pointer in the iocv-th argment on the stack
#define MsoPvOcvArg(iocv) \
            ((void *) (pocvSP[iocv]))

// Convert the pointer value to a interpreter return type
#define MsoOcvFromPv(pv) \
            ((MSOOCV) (pv))

// Return the value of the char in the iocv-th argment on the stack
#define MsoChOcvArg(iocv) \
            ((char) (pocvSP[iocv]))

// Convert the char value to a interpreter return type
#define MsoOcvFromCh(ch) \
            ((MSOOCV) (ch))

// Return the value of the unsigned char in the iocv-th argment on the stack
#define MsoUchOcvArg(iocv) \
            ((unsigned char) (pocvSP[iocv]))

// Convert the unsigned char value to a interpreter return type
#define MsoOcvFromUch(uch) \
            ((MSOOCV) (uch))

// Return the value of the int in the iocv-th argment on the stack
#define MsoIntOcvArg(iocv) \
            ((int) (pocvSP[iocv]))

// Convert the integer value to a interpreter return type
#define MsoOcvFromInt(v) \
            ((MSOOCV) (v))

// Return the value of the unsigned int in the iocv-th argment on the stack
#define MsoUintOcvArg(iocv) \
            ((unsigned int) (pocvSP[iocv]))

// Convert the unsigned int value to a interpreter return type
#define MsoOcvFromUint(v) \
            ((MSOOCV) (v))

// Return the value of the short iocv-th argment on the stack
#define MsoWOcvArg(iocv) \
            ((short) (pocvSP[iocv]))

// Convert the short value to a interpreter return type
#define MsoOcvFromW(w) \
            ((MSOOCV) (w))

// Return the value of the unsigned short iocv-th argment on the stack
#define MsoUwOcvArg(iocv) \
            ((unsigned short) (pocvSP[iocv]))

// Convert the unsigned short value to a interpreter return type
#define MsoOcvFromWw(uw) \
            ((MSOOCV) (uw))

// Return the value of the long in the iocv-th argment on the stack
#define MsoLOcvArg(iocv) \
            ((long) (pocvSP[iocv]))

// Convert the long value to a interpreter return type
#define MsoOcvFromL(l) \
            ((MSOOCV) (l))

// Return the value of the unsigned long in the iocv-th argment on the stack
#define MsoUlOcvArg(iocv) \
            ((unsigned long) (pocvSP[iocv]))

// Convert the unsigned long value to a interpreter return type
#define MsoOcvFromUl(l) \
            ((MSOOCV) (l))

// Return the address of the event (var) of index iocv: Application callback
#define MsoOcvEventAddr(iocv, pocis) \
            ((*(pocis)->pfnPocvGetVar)(iocv))

// Return the address of the stack index
#define MsoOcvStackAddr(docv, pocis) \
            (&(pocis)->pocvFP[docv])

// Return the address of a global variable or function
#define MsoOciiGlobalAddr(ipv, pocis) \
            (&(pocis)->rgpfn[ipv])

// Return the value of iocv-th argment on the stack
#define MsoOcvStack(iocv) \
            ((long) (pocvSP[iocv]))

// Supply the rest of the calling arguments to a var-args function call
#define MsoOcv_varargs_0 \
          MsoOcvStack(0),  MsoOcvStack(1),  MsoOcvStack(2), MsoOcvStack(3), \
          MsoOcv_varargs_4

#define MsoOcv_varargs_1 \
                           MsoOcvStack(1),  MsoOcvStack(2),  MsoOcvStack(3), \
          MsoOcv_varargs_4

#define MsoOcv_varargs_2 \
                                            MsoOcvStack(2),  MsoOcvStack(3), \
          MsoOcv_varargs_4

#define MsoOcv_varargs_3 \
                                                             MsoOcvStack(3), \
          MsoOcv_varargs_4

#define MsoOcv_varargs_4 \
          MsoOcvStack(5),  MsoOcvStack(5),  MsoOcvStack(6),  MsoOcvStack(7), \
          MsoOcv_varargs_8

#define MsoOcv_varargs_5 \
                           MsoOcvStack(5),  MsoOcvStack(6),  MsoOcvStack(7), \
          MsoOcv_varargs_8

#define MsoOcv_varargs_6 \
                                            MsoOcvStack(6),  MsoOcvStack(7), \
          MsoOcv_varargs_8

#define MsoOcv_varargs_7 \
                                                             MsoOcvStack(7), \
          MsoOcv_varargs_8

#define MsoOcv_varargs_8 \
          MsoOcvStack(8),  MsoOcvStack(9),  MsoOcvStack(10), MsoOcvStack(11), \
          MsoOcv_varargs_12

#define MsoOcv_varargs_9 \
                           MsoOcvStack(9),  MsoOcvStack(10), MsoOcvStack(11), \
          MsoOcv_varargs_12

#define MsoOcv_varargs_10 \
                                            MsoOcvStack(10), MsoOcvStack(11), \
          MsoOcv_varargs_12

#define MsoOcv_varargs_11 \
                                                             MsoOcvStack(11), \
          MsoOcv_varargs_12

#define MsoOcv_varargs_12 \
          MsoOcvStack(12), MsoOcvStack(13), MsoOcvStack(14), MsoOcvStack(15), \
          MsoOcv_varargs_16

#define MsoOcv_varargs_13 \
                           MsoOcvStack(13), MsoOcvStack(14), MsoOcvStack(15), \
          MsoOcv_varargs_16

#define MsoOcv_varargs_14 \
                                            MsoOcvStack(14), MsoOcvStack(15), \
          MsoOcv_varargs_16

#define MsoOcv_varargs_15 \
                                                             MsoOcvStack(15), \
          MsoOcv_varargs_16

#define MsoOcv_varargs_16 \
          MsoOcvStack(16), MsoOcvStack(17), MsoOcvStack(18), MsoOcvStack(19), \
          MsoOcv_varargs_20

#define MsoOcv_varargs_17 \
                           MsoOcvStack(17), MsoOcvStack(18), MsoOcvStack(19), \
          MsoOcv_varargs_20

#define MsoOcv_varargs_18 \
                                            MsoOcvStack(18), MsoOcvStack(19), \
          MsoOcv_varargs_20

#define MsoOcv_varargs_19 \
                                                             MsoOcvStack(19), \
          MsoOcv_varargs_20

#define MsoOcv_varargs_20 \
          MsoOcvStack(20), MsoOcvStack(21), MsoOcvStack(22), MsoOcvStack(23), \
          MsoOcv_varargs_24

#define MsoOcv_varargs_21 \
                           MsoOcvStack(21), MsoOcvStack(22), MsoOcvStack(23), \
          MsoOcv_varargs_24

#define MsoOcv_varargs_22 \
                                            MsoOcvStack(22), MsoOcvStack(23), \
          MsoOcv_varargs_24

#define MsoOcv_varargs_23 \
                                                             MsoOcvStack(23), \
          MsoOcv_varargs_24

#define MsoOcv_varargs_24 \
          MsoOcvStack(24), MsoOcvStack(25), MsoOcvStack(26), MsoOcvStack(27), \
          MsoOcv_varargs_28

#define MsoOcv_varargs_25 \
                           MsoOcvStack(25), MsoOcvStack(26), MsoOcvStack(27), \
          MsoOcv_varargs_28

#define MsoOcv_varargs_26 \
                                            MsoOcvStack(26), MsoOcvStack(27), \
          MsoOcv_varargs_28

#define MsoOcv_varargs_27 \
                                                             MsoOcvStack(27), \
          MsoOcv_varargs_28

#define MsoOcv_varargs_28 \
          MsoOcvStack(28), MsoOcvStack(29), MsoOcvStack(30), MsoOcvStack(31)

#define MsoOcv_varargs_29 \
                           MsoOcvStack(29), MsoOcvStack(30), MsoOcvStack(31)

#define MsoOcv_varargs_30 \
                                            MsoOcvStack(30), MsoOcvStack(31)

#define MsoOcv_varargs_31 \
                                                             MsoOcvStack(31)



/*----------------------------------------------------------------------------
    Interpreter function list.
    Should be list in same order as enum for clarity.
----------------------------------------------------------------------------*/

MSOAPI_(MSOOCV) MsoOcv_log_and(
    MSOOCII           **ppocii,
    struct _MSOOCIS    *pocis
    );
MSOAPI_(MSOOCV) MsoOcv_log_or(
    MSOOCII           **ppocii,
    struct _MSOOCIS    *pocis
    );
MSOAPI_(MSOOCV) MsoOcv_log_not(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_less_than(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_less_eql(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_eql(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_gtr_eql(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_gtr_than(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_not_eql(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_assign(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_plus(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_minus(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_mult(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_divide(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_mod(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_increment(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_decrement(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_unary_plus(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_unary_minus(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_bitwise_not(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_bitwise_and(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_bitwise_or(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_bitwise_xor(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_shift_l(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_shift_r(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_dereference(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_addr_of(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_cast_as(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_if(
    MSOOCII           **ppocii,
    struct _MSOOCIS    *pocis
    );
MSOAPI_(MSOOCV) MsoOcv_inline_if(
    MSOOCII           **ppocii,
    struct _MSOOCIS    *pocis
    );
MSOAPI_(MSOOCV) MsoOcv_let(
    MSOOCII           **ppocii,
    struct _MSOOCIS    *pocis
    );
MSOAPI_(MSOOCV) MsoOcv_compound_stmt(
    MSOOCII           **ppocii,
    struct _MSOOCIS    *pocis
    );
MSOAPI_(MSOOCV) MsoOcv_progn(
    MSOOCII           **ppocii,
    struct _MSOOCIS    *pocis
    );
MSOAPI_(MSOOCV) MsoOcv_get_char(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_get_uchar(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_get_short(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_get_ushort(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_get_int(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_get_uint(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_get_long(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_get_ulong(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_get_float(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_get_double(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_get_ldouble(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_set_char(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_set_uchar(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_set_short(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_set_ushort(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_set_int(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_set_uint(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_set_long(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_set_ulong(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_set_float(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_set_double(MSOOCV *pocvSP);
MSOAPI_(MSOOCV) MsoOcv_set_ldouble(MSOOCV *pocvSP);




//############################################################################
//
// PRIVATE Interfaces
//
// To use, #define _OCI_PRIVATE
//
//############################################################################

#ifdef _OCI_PRIVATE


// Return the Argument descriptor associated with the function
#define OcadArgDecripOcii(ocii, pocis) \
            ((pocis)->rgocadArgDesc[ocii])

// Return the Argument descriptor list associated with the function
#define PcbArgFromOcad(ocad, pocis) \
            (&(pocis)->rgcbImmedArg[ocad])


#endif /* _OCI_PRIVATE */

MSOEXTERN_C_END     // ****************** End extern "C" *********************

#endif /* !EMOCI_H */
