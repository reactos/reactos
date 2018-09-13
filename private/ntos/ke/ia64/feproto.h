/******************************
Intel Confidential
******************************/

#ifndef _EM_PROTOTYPES_H
#define _EM_PROTOTYPES_H

#ifndef INLINE
#define INLINE
#endif

#if !(defined(BIG_ENDIAN) || defined(LITTLE_ENDIAN))
    #error Endianness not established; define BIG_ENDIAN or LITTLE_ENDIAN
#endif


/**********************************************/
/* Assembler Supported Instruction Prototypes */
/**********************************************/

/* Floating-point Absolute Maximum */
void
fp82_famax(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

/* Floating-point Parallel Absolute Maximum */

void
fp82_fpamax(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

/* Floating-point Absolute Minimum */
void
fp82_famin(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);


/* Floating-point Parallel Absolute Minimum */

void
fp82_fpamin(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);


/* Floating-point Compare */

void
fp82_fcmp_eq(EM_state_type *ps,
    EM_opcode_ctype_type ctype,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_pred_reg_specifier p1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

void
fp82_fcmp_lt(EM_state_type *ps,
    EM_opcode_ctype_type ctype,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_pred_reg_specifier p1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

void
fp82_fcmp_le(EM_state_type *ps,
    EM_opcode_ctype_type ctype,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_pred_reg_specifier p1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);


void
fp82_fcmp_unord(EM_state_type *ps,
    EM_opcode_ctype_type ctype,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_pred_reg_specifier p1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);


/* Floating-point Paralel Compare */

void
fp82_fpcmp_eq(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

void
fp82_fpcmp_lt(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

void
fp82_fpcmp_le(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

void
fp82_fpcmp_unord(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

void
fp82_fpcmp_neq(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

void
fp82_fpcmp_nlt(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

void
fp82_fpcmp_nle(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

void
fp82_fpcmp_ord(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);


/* Convert Floating-point to Integer */

void
fp82_fcvt_fx(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2);

void
fp82_fcvt_fx_trunc(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2);

void
fp82_fcvt_fxu(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2);

void
fp82_fcvt_fxu_trunc(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2);


/* Parallel Convert Floating-point to Integer */

void
fp82_fpcvt_fx(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2);

void
fp82_fpcvt_fx_trunc(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2);

void
fp82_fpcvt_fxu(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2);

void
fp82_fpcvt_fxu_trunc(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2);



/* Floating-point Multiply Add */

void
fp82_fma(EM_state_type *ps,
    EM_opcode_pc_type pc,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f3,
    EM_fp_reg_specifier f4,
    EM_fp_reg_specifier f2);


/* Floating Point Parallel Multiply Add */
void
fp82_fpma(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f3,
    EM_fp_reg_specifier f4,
    EM_fp_reg_specifier f2);


/* Floating-point Maximum */
void
fp82_fmax(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);


/* Floating-point Parallel Maximum */
void
fp82_fpmax(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

/* Floating-point Minimum */
void
fp82_fmin(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);


/* Floating-point Parallel Minimum */
void
fp82_fpmin(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);


/* Floating-point Multiply Subtract */
void
fp82_fms(EM_state_type *ps,
    EM_opcode_pc_type pc,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f3,
     EM_fp_reg_specifier f4,
    EM_fp_reg_specifier f2);


/* Floating-point Parallel Multiply Subtract */
void
fp82_fpms(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f3,
    EM_fp_reg_specifier f4,
    EM_fp_reg_specifier f2);



/* Floating-point Negative Multiply Add */
void
fp82_fnma(EM_state_type *ps,
    EM_opcode_pc_type pc,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f3,
     EM_fp_reg_specifier f4,
    EM_fp_reg_specifier f2);


/* Floating-point Parallel Negative Multiply Add */

void
fp82_fpnma(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f3,
     EM_fp_reg_specifier f4,
    EM_fp_reg_specifier f2);


/* Floating-point Reciprocal Approximation */
void
fp82_frcpa(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

/* Floating-point Parallel Reciprocal Approximation */
void
fp82_fprcpa(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3);

/* Floating-point Reciprocal Square Root Approximation */
void
fp82_frsqrta(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier f3);


/* Floating-point Parallel Reciprocal Square Root Approximation */
void
fp82_fprsqrta(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier f3);


/******************************************************************************/
/* Define macros to simplify access to the fp82_ functions.  This is done so  */
/*  the namespace doesn't get cluttered, while retaining convenient access.   */
/*  The FP82_NO_SHORTCUTS macro can be defined to prevent creation of these.  */
/******************************************************************************/

#ifndef FP82_NO_SHORTCUTS
#define famax           fp82_famax
#define fpamax          fp82_fpamax
#define famin           fp82_famin
#define fpamin          fp82_fpamin

#define fcmp_eq         fp82_fcmp_eq
#define fcmp_lt         fp82_fcmp_lt
#define fcmp_le         fp82_fcmp_le
#define fcmp_unord      fp82_fcmp_unord

#define fpcmp_eq        fp82_fpcmp_eq
#define fpcmp_lt        fp82_fpcmp_lt
#define fpcmp_le        fp82_fpcmp_le
#define fpcmp_unord     fp82_fpcmp_unord
#define fpcmp_neq       fp82_fpcmp_neq
#define fpcmp_nlt       fp82_fpcmp_nlt
#define fpcmp_nle       fp82_fpcmp_nle
#define fpcmp_ord       fp82_fpcmp_ord

#define fcvt_fx         fp82_fcvt_fx
#define fcvt_fx_trunc   fp82_fcvt_fx_trunc
#define fcvt_fxu        fp82_fcvt_fxu
#define fcvt_fxu_trunc  fp82_fcvt_fxu_trunc

#define fpcvt_fxu_trunc fp82_fpcvt_fxu_trunc
#define fpcvt_fxu       fp82_fpcvt_fxu
#define fpcvt_fx        fp82_fpcvt_fx
#define fpcvt_fx_trunc  fp82_fpcvt_fx_trunc


#define fma             fp82_fma
#define fpma            fp82_fpma
#define fmax            fp82_fmax
#define fpmax           fp82_fpmax
#define fmin            fp82_fmin
#define fpmin           fp82_fpmin
#define fms             fp82_fms
#define fpms            fp82_fpms
#define fnma            fp82_fnma
#define fpnma           fp82_fpnma
#define frcpa           fp82_frcpa
#define fprcpa          fp82_fprcpa
#define frsqrta         fp82_frsqrta
#define fprsqrta        fp82_fprsqrta

#endif /* FP82_NO_SHORTCUTS */


#endif /* _EM_PROTOTYPES_H */

