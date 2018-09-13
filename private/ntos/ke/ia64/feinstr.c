// ******************************
// Intel Confidential
// ******************************

// ******************************
// asm mappings to instructions 
// ******************************

// MACH
#include "ki.h"

#ifndef _STDIO_H
#include <stdio.h>
#endif

#include "fepublic.h"
#include "fehelper.h"
#include "festate.h"
#include "feinstr.h"


// ******************************************************************
// Public functions
// ******************************************************************

// ******************************************************************
// fma: Floating point multiply add
// ******************************************************************
void
fp82_fma(EM_state_type *ps,
    EM_opcode_pc_type     pc,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f3,
    EM_fp_reg_specifier   f4,
    EM_fp_reg_specifier   f2)
{
    GETSTATE_F1(qp,f1,f3,f4,f2);
    _fma(ps, pc, sf, qp, f1, f3, f4, f2);
    PUTSTATE_F1(f1);
}


// ******************************************************************
// fpma: Floating point parallel multiply add
// ******************************************************************
void
fp82_fpma(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f3,
    EM_fp_reg_specifier   f4,
    EM_fp_reg_specifier   f2)
{
    GETSTATE_F1(qp,f1,f3,f4,f2);
    _fpma(ps, sf, qp, f1, f3, f4, f2);
    PUTSTATE_F1(f1);
}

// ******************************************************************
// Floating-point Multiply Subtract 
// ******************************************************************
void
fp82_fms(EM_state_type *ps,
    EM_opcode_pc_type pc,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f3,
    EM_fp_reg_specifier f4,
    EM_fp_reg_specifier f2)
{
    GETSTATE_F1(qp,f1,f3,f4,f2);
    _fms(ps, pc, sf, qp, f1, f3, f4, f2);
    PUTSTATE_F1(f1);
}


// ******************************************************************
// Floating-point Parallel Multiply Subtract 
// ******************************************************************
void
fp82_fpms(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f3,
    EM_fp_reg_specifier f4,
    EM_fp_reg_specifier f2)
{
    GETSTATE_F1(qp,f1,f3,f4,f2);
    _fpms(ps, sf, qp, f1, f3, f4, f2);
    PUTSTATE_F1(f1);
}


// ******************************************************************
// Floating-point Negative Multiply Add 
// ******************************************************************
void
fp82_fnma(EM_state_type *ps,
    EM_opcode_pc_type     pc,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f3,
    EM_fp_reg_specifier   f4,
    EM_fp_reg_specifier   f2)
{
    GETSTATE_F1(qp,f1,f3,f4,f2);
    _fnma(ps, pc, sf, qp, f1, f3, f4, f2);
    PUTSTATE_F1(f1);
}

// ******************************************************************
// Floating-point Parallel Negative Multiply Add 
// ******************************************************************
void
fp82_fpnma(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f3,
    EM_fp_reg_specifier   f4,
    EM_fp_reg_specifier   f2)
{
    GETSTATE_F1(qp,f1,f3,f4,f2);
    _fpnma(ps, sf, qp, f1, f3, f4, f2);
    PUTSTATE_F1(f1);
}


// ******************************************************************
// Floating-point Compare 
// ******************************************************************
void
fp82_fcmp_eq(EM_state_type         *ps,
    EM_opcode_ctype_type  fctype,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_pred_reg_specifier p1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F4(qp,p1,p2,f2,f3);
    _fcmp(ps, frelEQ, fctype, sf, (EM_uint_t)qp,
        (EM_uint_t)p1, (EM_uint_t)p2, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F4(p1,p2);
}

void
fp82_fcmp_lt(EM_state_type *ps,
    EM_opcode_ctype_type  fctype,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_pred_reg_specifier p1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F4(qp,p1,p2,f2,f3);
    _fcmp(ps, frelLT, fctype, sf, (EM_uint_t)qp,
        (EM_uint_t)p1, (EM_uint_t)p2, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F4(p1,p2);
}

void
fp82_fcmp_le(EM_state_type *ps,
    EM_opcode_ctype_type  fctype,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_pred_reg_specifier p1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F4(qp,p1,p2,f2,f3);
    _fcmp(ps, frelLE, fctype, sf, (EM_uint_t)qp,
        (EM_uint_t)p1, (EM_uint_t)p2, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F4(p1,p2);
}

void
fp82_fcmp_unord(EM_state_type *ps,
    EM_opcode_ctype_type  fctype,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_pred_reg_specifier p1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F4(qp,p1,p2,f2,f3);
    _fcmp(ps, frelUNORD, fctype, sf, (EM_uint_t)qp,
    (EM_uint_t)p1, (EM_uint_t)p2, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F4(p1,p2);
}


// ******************************************************************
// Floating-point Reciprocal Approximation 
// ******************************************************************
void
fp82_frcpa(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F6(qp,f1,p2,f2,f3);
    _frcpa(ps, sf, qp, f1, p2, f2, f3);
    PUTSTATE_F6(f1,p2);
}

// ******************************************************************
// Floating-point Parallel Reciprocal Approximation 
// ******************************************************************
void
fp82_fprcpa(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F6(qp,f1,p2,f2,f3);
    _fprcpa(ps, sf, qp, f1, p2, f2, f3);
    PUTSTATE_F6(f1,p2);
}


// ******************************************************************
// Floating-point Reciprocal Square Root Approximation 
// ******************************************************************

void
fp82_frsqrta(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F7(qp,f1,p2,f3);
    _frsqrta(ps, sf, qp, f1, p2, f3);
    PUTSTATE_F7(f1,p2);
}

// ******************************************************************
// Floating-point Parallel Reciprocal Square Root Approximation 
// ******************************************************************
void
fp82_fprsqrta(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_pred_reg_specifier p2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F7(qp,f1,p2,f3);
    _fprsqrta(ps, sf, qp, f1, p2, f3);
    PUTSTATE_F7(f1,p2);
}

// ******************************************************************
// Floating-point Minimum 
// ******************************************************************
void
fp82_fmin(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fmin(ps, sf, qp, f1, f2, f3);
    PUTSTATE_F8(f1);
}

// ******************************************************************
// Floating-point Maximum 
// ******************************************************************
void
fp82_fmax(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fmax(ps, sf, qp, f1, f2, f3);
    PUTSTATE_F8(f1);
}

// ******************************************************************
// Floating-point Absolute Minimum 
// ******************************************************************
void
fp82_famin(EM_state_type         *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _famin(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F8(f1);
}

// ******************************************************************
// Floating-point Absolute Maximum 
// ******************************************************************
void
fp82_famax(EM_state_type         *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _famax(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F8(f1);
}


// ******************************************************************
// Floating-point Parallel Minimum 
// ******************************************************************
void
fp82_fpmin(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier f1,
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fpmin(ps, sf, qp, f1, f2, f3);
    PUTSTATE_F8(f1);
}


// ******************************************************************
// Floating-point Parallel Maximum 
// ******************************************************************
void
fp82_fpmax(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fpmax(ps, sf, qp, f1, f2, f3);
    PUTSTATE_F8(f1);
}

// ******************************************************************
// Floating-point Parallel Absolute Minimum 
// ******************************************************************
void
fp82_fpamin(EM_state_type         *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fpamin(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F8(f1);
}


// ******************************************************************
// Floating-point Parallel Absolute Maximum 
// ******************************************************************
void
fp82_fpamax(EM_state_type         *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fpamax(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F8(f1);
}


// ******************************************************************
// Floating-point Parallel Compare 
// ******************************************************************
void
fp82_fpcmp_eq(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fpcmp(ps, frelEQ, sf, (EM_uint_t)qp,
          (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F8(f1);
}


void
fp82_fpcmp_lt(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fpcmp(ps, frelLT, sf, (EM_uint_t)qp,
          (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F8(f1);
}

void
fp82_fpcmp_le(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
   GETSTATE_F8(qp,f1,f2,f3);
   _fpcmp(ps, frelLE, sf, (EM_uint_t)qp,
          (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
   PUTSTATE_F8(f1);
}

void
fp82_fpcmp_unord(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fpcmp(ps, frelUNORD, sf, (EM_uint_t)qp,
          (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F8(f1);
}

void
fp82_fpcmp_neq(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fpcmp(ps,
           frelNEQ,                 sf, (EM_uint_t)qp,
           (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F8(f1);
}

void
fp82_fpcmp_nlt(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fpcmp(ps, frelNLT, sf, (EM_uint_t)qp,
          (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F8(f1);
}

void
fp82_fpcmp_nle(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fpcmp(ps, frelNLE, sf, (EM_uint_t)qp,
          (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F8(f1);
}

void
fp82_fpcmp_ord(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2,
    EM_fp_reg_specifier   f3)
{
    GETSTATE_F8(qp,f1,f2,f3);
    _fpcmp(ps, frelORD, sf, (EM_uint_t)qp,
          (EM_uint_t)f1, (EM_uint_t)f2, (EM_uint_t)f3);
    PUTSTATE_F8(f1);
}

// ******************************************************************
// Convert Floating-point to Integer 
// ******************************************************************
void
fp82_fcvt_fx(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2)
{
    GETSTATE_F10(qp,f1,f2);
    SIGNED_FORM   = 1;
    _fcvt_fx(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2);
    PUTSTATE_F10(f1);
}

void
fp82_fcvt_fxu(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2)
{
    GETSTATE_F10(qp,f1,f2);
    UNSIGNED_FORM = 1;
    _fcvt_fx(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2);
    PUTSTATE_F10(f1);
}


void
fp82_fcvt_fx_trunc(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2)
{

    GETSTATE_F10(qp,f1,f2);
    SIGNED_FORM   = 1;
    TRUNC_FORM    = 1;
    _fcvt_fx(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2);
    PUTSTATE_F10(f1);
}


void
fp82_fcvt_fxu_trunc(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2)
{
    GETSTATE_F10(qp,f1,f2);
    TRUNC_FORM    = 1;
    UNSIGNED_FORM = 1;
    _fcvt_fx(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2);
    PUTSTATE_F10(f1);
}


// ******************************************************************
// Parallel Convert Floating-point to Integer 
// ******************************************************************
void
fp82_fpcvt_fx(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2)
{
    GETSTATE_F10(qp,f1,f2);
    SIGNED_FORM   = 1;
    _fpcvt_fx(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2);
    PUTSTATE_F10(f1);
}


void
fp82_fpcvt_fxu(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2)
{
    GETSTATE_F10(qp,f1,f2);
    UNSIGNED_FORM = 1;
    _fpcvt_fx(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2);
    PUTSTATE_F10(f1);
}

void
fp82_fpcvt_fx_trunc(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2)
{

    GETSTATE_F10(qp,f1,f2);
    SIGNED_FORM   = 1;
    TRUNC_FORM    = 1;
    _fpcvt_fx(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2);
    PUTSTATE_F10(f1);
}

void
fp82_fpcvt_fxu_trunc(EM_state_type *ps,
    EM_opcode_sf_type     sf,
    EM_pred_reg_specifier qp,
    EM_fp_reg_specifier   f1,
    EM_fp_reg_specifier   f2)
{
    GETSTATE_F10(qp,f1,f2);
    TRUNC_FORM    = 1;
    UNSIGNED_FORM = 1;
    _fpcvt_fx(ps, sf, (EM_uint_t)qp, (EM_uint_t)f1, (EM_uint_t)f2);
    PUTSTATE_F10(f1);
}

//***************************************************************
// Instruction pages: The Underbar Routines
//***************************************************************

// ******************************************************************
// _fma
// ******************************************************************
static INLINE void
_fma(EM_state_type    *ps,
    EM_opcode_pc_type pc,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         f3,
    EM_uint_t         f4,
    EM_uint_t         f2)
{
EM_uint_t          tmp_isrcode;
EM_fp_reg_type     tmp_default_result;
EM_tmp_fp_env_type tmp_fp_env;
EM_fp_dp_type      tmp_res;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, f4)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3]) || fp_is_natval(FR[f4])) {
         FR[f1] = NATVAL;
         fp_update_psr(f1);
      } else {
         tmp_default_result = fma_exception_fault_check(f2, f3, f4,
                              pc, sf, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         if (fp_is_nan_or_inf(tmp_default_result))  {
            FR[f1] = tmp_default_result;
         } else {
            tmp_res = fp_mul(fp_reg_read(FR[f3]), 
                             fp_reg_read(FR[f4]));
            if (f2 != 0)
               tmp_res = fp_add(tmp_res, fp_reg_read(FR[f2]), tmp_fp_env);
            FR[f1] = fp_ieee_round(tmp_res, &tmp_fp_env);
         }

         fp_update_fpsr(sf, tmp_fp_env);
         fp_update_psr(f1);
         if (fp_raise_traps(tmp_fp_env))
            fp_exception_trap(fp_decode_trap(tmp_fp_env));
      }
   }
/* EAS END */
}


// ******************************************************************
// _fpma 
// ******************************************************************
static INLINE void
_fpma(EM_state_type   *ps,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         f3,
    EM_uint_t         f4,
    EM_uint_t         f2)
{
EM_uint_t           tmp_isrcode, tmp_res_hi, tmp_res_lo;
EM_pair_fp_reg_type tmp_default_result_pair;
EM_tmp_fp_env_type  tmp_fp_env;
EM_fp_dp_type       tmp_res;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, f4))  
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3]) || fp_is_natval(FR[f4]))  {
         FR[f1] = NATVAL;
         fp_update_psr(f1);
      } else {
         tmp_default_result_pair = fpma_exception_fault_check(f2, 
                                   f3, f4, sf, &tmp_fp_env);

         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }
         
         if (fp_is_nan_or_inf(tmp_default_result_pair.hi)) {
            tmp_res_hi = fp_single(tmp_default_result_pair.hi);
         } else {
            tmp_res = fp_mul(fp_reg_read_hi(f3), fp_reg_read_hi(f4));                
            if (f2 != 0)
               tmp_res = fp_add(tmp_res, fp_reg_read_hi(f2), tmp_fp_env);
            tmp_res_hi = fp_ieee_round_sp(tmp_res, high, &tmp_fp_env);
         }

         if (fp_is_nan_or_inf(tmp_default_result_pair.lo)) {
            tmp_res_lo = fp_single(tmp_default_result_pair.lo);
         } else {
            tmp_res = fp_mul(fp_reg_read_lo(f3), fp_reg_read_lo(f4));                
            if (f2 != 0)
               tmp_res = fp_add(tmp_res, fp_reg_read_lo(f2), tmp_fp_env);
            tmp_res_lo = fp_ieee_round_sp(tmp_res, low, &tmp_fp_env);
         }

         FR[f1].significand = fp_concatenate(tmp_res_hi, tmp_res_lo);
         FR[f1].exponent    = FP_INTEGER_EXP;
         FR[f1].sign        = FP_SIGN_POSITIVE;

         fp_update_fpsr(sf, tmp_fp_env);
         fp_update_psr(f1);
         if (fp_raise_traps(tmp_fp_env))
            fp_exception_trap(fp_decode_trap(tmp_fp_env));
      }
   }
/* EAS END */
}


// ******************************************************************
// _fms 
// ******************************************************************
static INLINE void
_fms(EM_state_type    *ps,
    EM_opcode_pc_type pc,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         f3,
    EM_uint_t         f4,
    EM_uint_t         f2)
{
EM_uint_t          tmp_isrcode;
EM_fp_reg_type     tmp_fr2, tmp_default_result;
EM_fp_dp_type      tmp_res;
EM_tmp_fp_env_type tmp_fp_env;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, f4)) 
            disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3]) || fp_is_natval(FR[f4])) {
         FR[f1] = NATVAL;
         fp_update_psr(f1);
      } else {
         tmp_default_result = fms_fnma_exception_fault_check(f2, f3, f4,
                              pc, sf, &tmp_fp_env);

         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         if (fp_is_nan_or_inf(tmp_default_result))  {
            FR[f1] = tmp_default_result;
         } else {
            tmp_res = fp_mul(fp_reg_read(FR[f3]), fp_reg_read(FR[f4]));
            tmp_fr2 = fp_reg_read(FR[f2]);
            tmp_fr2.sign = !tmp_fr2.sign;
            if (f2 != 0)
               tmp_res = fp_add(tmp_res, tmp_fr2, tmp_fp_env);
            FR[f1] = fp_ieee_round(tmp_res, &tmp_fp_env);
         }

         fp_update_fpsr(sf, tmp_fp_env);
         fp_update_psr(f1);
         if (fp_raise_traps(tmp_fp_env))
            fp_exception_trap(fp_decode_trap(tmp_fp_env));
      }
   }
/* EAS END */
}


// ******************************************************************
// _fpms 
// ******************************************************************
static INLINE void
_fpms(EM_state_type   *ps,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         f3,
    EM_uint_t         f4,
    EM_uint_t         f2)
{
EM_uint_t           tmp_isrcode, tmp_res_hi, tmp_res_lo;
EM_pair_fp_reg_type tmp_default_result_pair;
EM_fp_reg_type      tmp_sub;
EM_fp_dp_type       tmp_res;
EM_tmp_fp_env_type  tmp_fp_env;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, f4)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3]) || fp_is_natval(FR[f4])) {
         FR[f1] = NATVAL;
         fp_update_psr(f1);
      } else {
         tmp_default_result_pair = fpms_fpnma_exception_fault_check(f2, f3, f4,
                                   sf, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         if (fp_is_nan_or_inf(tmp_default_result_pair.hi))  {
            tmp_res_hi = fp_single(tmp_default_result_pair.hi);
         } else {
            tmp_res = fp_mul(fp_reg_read_hi(f3), fp_reg_read_hi(f4));
            if (f2 != 0) {
               tmp_sub      = fp_reg_read_hi(f2);
               tmp_sub.sign = !tmp_sub.sign;
               tmp_res      = fp_add(tmp_res, tmp_sub, tmp_fp_env);
            }
            tmp_res_hi = fp_ieee_round_sp(tmp_res, high, &tmp_fp_env);
         }

         if (fp_is_nan_or_inf(tmp_default_result_pair.lo))  {
            tmp_res_lo = fp_single(tmp_default_result_pair.lo);
         } else {
            tmp_res = fp_mul(fp_reg_read_lo(f3), fp_reg_read_lo(f4));
            if (f2 != 0) {
               tmp_sub      = fp_reg_read_lo(f2);
               tmp_sub.sign = !tmp_sub.sign;
               tmp_res      = fp_add(tmp_res, tmp_sub, tmp_fp_env);
            }
            tmp_res_lo = fp_ieee_round_sp(tmp_res, low, &tmp_fp_env);
         }

         FR[f1].significand = fp_concatenate(tmp_res_hi, tmp_res_lo);
         FR[f1].exponent    = FP_INTEGER_EXP;
         FR[f1].sign        = FP_SIGN_POSITIVE;

         fp_update_fpsr(sf, tmp_fp_env);
         fp_update_psr(f1);
         if (fp_raise_traps(tmp_fp_env))
            fp_exception_trap(fp_decode_trap(tmp_fp_env));
      }
   }
/* EAS END */
}


// ******************************************************************
// _fnma 
// ******************************************************************
static INLINE void
_fnma(EM_state_type   *ps,
    EM_opcode_pc_type pc,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         f3,
    EM_uint_t         f4,
    EM_uint_t         f2)
{
EM_uint_t          tmp_isrcode;
EM_fp_reg_type     tmp_default_result;
EM_tmp_fp_env_type tmp_fp_env;
EM_fp_dp_type      tmp_res;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, f4)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3]) || fp_is_natval(FR[f4])) {
         FR[f1] = NATVAL;
         fp_update_psr(f1);
      } else {
         tmp_default_result = fms_fnma_exception_fault_check(f2, f3, f4,
                              pc, sf, &tmp_fp_env);

         if (fp_raise_fault(tmp_fp_env)) {
                fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         if (fp_is_nan_or_inf(tmp_default_result))  {
            FR[f1] = tmp_default_result;
         } else {
            tmp_res      = fp_mul(fp_reg_read(FR[f3]), fp_reg_read(FR[f4]));
            tmp_res.sign = !tmp_res.sign;
            if (f2 != 0)
               tmp_res = fp_add(tmp_res, fp_reg_read(FR[f2]), tmp_fp_env);
            FR[f1] = fp_ieee_round(tmp_res, &tmp_fp_env);
         }

         fp_update_fpsr(sf, tmp_fp_env);
         fp_update_psr(f1);
         if (fp_raise_traps(tmp_fp_env))
            fp_exception_trap(fp_decode_trap(tmp_fp_env));
      }
   }
/* EAS END */
}

// ******************************************************************
// _fpnma 
// ******************************************************************
static INLINE void
_fpnma(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         f3,
    EM_uint_t         f4,
    EM_uint_t         f2)
{
EM_uint_t           tmp_isrcode, tmp_res_hi, tmp_res_lo;
EM_pair_fp_reg_type tmp_default_result_pair;
EM_tmp_fp_env_type  tmp_fp_env;
EM_fp_dp_type       tmp_res;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, f4)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3]) || fp_is_natval(FR[f4]))  {
         FR[f1] = NATVAL;
         fp_update_psr(f1);
      } else {
         tmp_default_result_pair = fpms_fpnma_exception_fault_check(f2, f3, f4,
                                   sf, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         if (fp_is_nan_or_inf(tmp_default_result_pair.hi))  {
            tmp_res_hi = fp_single(tmp_default_result_pair.hi);
         } else {
            tmp_res = fp_mul(fp_reg_read_hi(f3), fp_reg_read_hi(f4));
            tmp_res.sign = !tmp_res.sign;
            if (f2 != 0)
               tmp_res = fp_add(tmp_res, fp_reg_read_hi(f2), tmp_fp_env);
            tmp_res_hi = fp_ieee_round_sp(tmp_res, high, &tmp_fp_env);
         }

         if (fp_is_nan_or_inf(tmp_default_result_pair.lo))  {
            tmp_res_lo = fp_single(tmp_default_result_pair.lo);
         } else {
            tmp_res = fp_mul(fp_reg_read_lo(f3), fp_reg_read_lo(f4));
            tmp_res.sign = !tmp_res.sign;
            if (f2 != 0)
               tmp_res = fp_add(tmp_res, fp_reg_read_lo(f2), tmp_fp_env);
            tmp_res_lo = fp_ieee_round_sp(tmp_res, low, &tmp_fp_env);
         }

         FR[f1].significand = fp_concatenate(tmp_res_hi, tmp_res_lo);
         FR[f1].exponent    = FP_INTEGER_EXP;
         FR[f1].sign        = FP_SIGN_POSITIVE;

         fp_update_fpsr(sf, tmp_fp_env);
         fp_update_psr(f1);
         if (fp_raise_traps(tmp_fp_env))
            fp_exception_trap(fp_decode_trap(tmp_fp_env));
      }
   }
/* EAS END */
}


// ******************************************************************
// _fcmp 
// ******************************************************************
static INLINE void
_fcmp(EM_state_type      *ps,
    EM_opcode_frel_type  frel,
    EM_opcode_ctype_type fctype,
    EM_opcode_sf_type    sf,
    EM_uint_t            qp,
    EM_uint_t            p1,
    EM_uint_t            p2,
    EM_uint_t            f2,
    EM_uint_t            f3)
{
EM_uint_t          tmp_isrcode;
EM_fp_reg_type     tmp_fr2, tmp_fr3;
EM_tmp_fp_env_type tmp_fp_env;
EM_boolean_t       tmp_rel;

/* EAS START */
   if (PR[qp]) {
      if(p1==p2)
         illegal_operation_fault(0);
      if (tmp_isrcode = fp_reg_disabled(f2, f3, 0, 0))
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3])) {
         PR[p1] = 0;
         PR[p2] = 0;
      } else {
         fcmp_exception_fault_check(f2, f3, frel, sf, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         tmp_fr2 = fp_reg_read(FR[f2]);
         tmp_fr3 = fp_reg_read(FR[f3]);

         if (frel == frelEQ)         tmp_rel = fp_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelLT)    tmp_rel = fp_less_than(tmp_fr2, tmp_fr3);
         else if (frel == frelLE)    tmp_rel = fp_lesser_or_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelGT)    tmp_rel = fp_less_than(tmp_fr3, tmp_fr2);
         else if (frel == frelGE)    tmp_rel = fp_lesser_or_equal(tmp_fr3, tmp_fr2);
         else if (frel == frelUNORD) tmp_rel = fp_unordered(tmp_fr2, tmp_fr3);
         else if (frel == frelNEQ)   tmp_rel = !fp_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelNLT)   tmp_rel = !fp_less_than(tmp_fr2, tmp_fr3);
         else if (frel == frelNLE)   tmp_rel = !fp_lesser_or_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelNGT)   tmp_rel = !fp_less_than(tmp_fr3, tmp_fr2);
         else if (frel == frelNGE)   tmp_rel = !fp_lesser_or_equal(tmp_fr3, tmp_fr2);
         else                         tmp_rel = !fp_unordered(tmp_fr2, tmp_fr3);

         PR[p1] = tmp_rel;
         PR[p2] = !tmp_rel;
         fp_update_fpsr(sf, tmp_fp_env);
      }

   } else {
      if (fctype == fctypeUNC) {
         if(p1==p2)
            illegal_operation_fault(0);
         PR[p1] = 0;
         PR[p2] = 0;
      }
   }
/* EAS END */
}

// ******************************************************************
// _frcpa 
// ******************************************************************
static INLINE void
_frcpa(EM_state_type  *ps,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         p2,
    EM_uint_t         f2,
    EM_uint_t         f3)
{
EM_uint_t          tmp_isrcode;
EM_fp_reg_type     tmp_default_result, num, den;
EM_tmp_fp_env_type tmp_fp_env;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3]))  {
         FR[f1] = NATVAL;
         PR[p2] = 0;
      } else {
         tmp_default_result = frcpa_exception_fault_check(f2, f3, sf, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         if (fp_is_nan_or_inf(tmp_default_result)) {
            FR[f1] = tmp_default_result;
            PR[p2] = 0;
          
         } else {
            num          = fp_normalize(fp_reg_read(FR[f2]));
            den          = fp_normalize(fp_reg_read(FR[f3]));
            if (fp_is_inf(num) && fp_is_finite(den)) {
               FR[f1]      = FP_INFINITY;
               FR[f1].sign = num.sign ^ den.sign;
               PR[p2] = 0;

            } else if (fp_is_finite(num) && fp_is_inf(den)) {
               FR[f1]      = FP_ZERO;
               FR[f1].sign = num.sign ^ den.sign;
               PR[p2] = 0;

            } else if (fp_is_zero(num) && fp_is_finite(den)) {
               FR[f1]      = FP_ZERO;
               FR[f1].sign = num.sign ^ den.sign;
               PR[p2] = 0;

            } else {
               FR[f1]      = fp_ieee_recip(den);
               PR[p2] = 1;
            }
         }
         fp_update_fpsr(sf, tmp_fp_env);
      }
      fp_update_psr(f1);
   } else  {
        PR[p2] = 0;
   }
/* EAS END */
}

// ******************************************************************
// _fprcpa 
// ******************************************************************
static INLINE void
_fprcpa(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         p2,
    EM_uint_t         f2,
    EM_uint_t         f3)
{
EM_uint_t              tmp_isrcode, tmp_res_hi, tmp_res_lo;
EM_pair_fp_reg_type    tmp_default_result_pair;
EM_fp_reg_type         tmp_res, num, den;
EM_boolean_t           tmp_pred_hi, tmp_pred_lo;
EM_tmp_fp_env_type     tmp_fp_env;
EM_limits_check_fprcpa limits_check = {0,0,0,0};

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3])) {
         FR[f1] = NATVAL;
         PR[p2] = 0;
      } else {
         tmp_default_result_pair = fprcpa_exception_fault_check(f2, f3, sf, 
                                      &tmp_fp_env, &limits_check);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         if (fp_is_nan_or_inf(tmp_default_result_pair.hi) || limits_check.hi_fr3) {
            tmp_res_hi  = fp_single(tmp_default_result_pair.hi);
            tmp_pred_hi = 0;

         } else {
            num          = fp_normalize(fp_reg_read_hi(f2));
            den          = fp_normalize(fp_reg_read_hi(f3));

            if (fp_is_inf(num) && fp_is_finite(den)) {
               tmp_res      = FP_INFINITY;
               tmp_res.sign = num.sign ^ den.sign;
               tmp_pred_hi = 0;

            } else if (fp_is_finite(num) && fp_is_inf(den)) {
               tmp_res      = FP_ZERO;
               tmp_res.sign = num.sign ^ den.sign;
               tmp_pred_hi = 0;

            } else if (fp_is_zero(num) && fp_is_finite(den)) {
               tmp_res      = FP_ZERO;
               tmp_res.sign = num.sign ^ den.sign;
               tmp_pred_hi = 0;
         
            } else {
               tmp_res = fp_ieee_recip(den);
               if (limits_check.hi_fr2_or_quot) {
                  tmp_pred_hi  = 0;
               } else {
                  tmp_pred_hi  = 1;
               }
            }
            tmp_res_hi = fp_single(tmp_res);
         }


         if (fp_is_nan_or_inf(tmp_default_result_pair.lo) || limits_check.lo_fr3) {
             tmp_res_lo  = fp_single(tmp_default_result_pair.lo);
             tmp_pred_lo = 0;
         } else {
            num          = fp_normalize(fp_reg_read_lo(f2));
            den          = fp_normalize(fp_reg_read_lo(f3));

            if (fp_is_inf(num) && fp_is_finite(den)) {
               tmp_res      = FP_INFINITY;
               tmp_res.sign = num.sign ^ den.sign;
               tmp_pred_lo = 0;

            } else if (fp_is_finite(num) && fp_is_inf(den)) {
               tmp_res      = FP_ZERO;
               tmp_res.sign = num.sign ^ den.sign;
               tmp_pred_lo = 0;

            } else if (fp_is_zero(num) && fp_is_finite(den)) {
               tmp_res      = FP_ZERO;
               tmp_res.sign = num.sign ^ den.sign;
               tmp_pred_lo = 0;
         
            } else {
               tmp_res = fp_ieee_recip(den);
               if (limits_check.lo_fr2_or_quot) {
                  tmp_pred_lo = 0;
               } else {
                  tmp_pred_lo = 1;
               }
           }
           tmp_res_lo = fp_single(tmp_res);
        }

        FR[f1].significand = fp_concatenate(tmp_res_hi, tmp_res_lo);
        FR[f1].exponent    = FP_INTEGER_EXP;
        FR[f1].sign        = FP_SIGN_POSITIVE;
        PR[p2]             = tmp_pred_hi && tmp_pred_lo;

        fp_update_fpsr(sf, tmp_fp_env);
     }
      fp_update_psr(f1);
   } else  {
      PR[p2] = 0; /* unconditional semantics */
   }
/* EAS END */
}

// ******************************************************************
// _frsqrta 
// ******************************************************************
static INLINE void
_frsqrta(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         p2,
    EM_uint_t         f3)
{
EM_uint_t           tmp_isrcode;
EM_fp_reg_type      tmp_default_result, tmp_fr3;
EM_tmp_fp_env_type  tmp_fp_env;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f3, 0, 0)) 
            disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f3])) {
         FR[f1] = NATVAL;
         PR[p2] = 0;
      } else {
         tmp_default_result = frsqrta_exception_fault_check(f3, sf, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         if (fp_is_nan_or_inf(tmp_default_result)) {
            FR[f1] = tmp_default_result;
            PR[p2] = 0;
         } else {
            tmp_fr3 = fp_normalize(fp_reg_read(FR[f3]));
            if (fp_is_zero(tmp_fr3)) {
               FR[f1] = tmp_fr3;
               PR[p2] = 0;
            } else if (fp_is_pos_inf(tmp_fr3)) {
               FR[f1] = tmp_fr3;
               PR[p2] = 0;
            } else {
               FR[f1]  = fp_ieee_recip_sqrt(tmp_fr3);
               PR[p2]  = 1;
            }
         }
         fp_update_fpsr(sf, tmp_fp_env);
      }
      fp_update_psr(f1);
   } else {
      PR[p2] = 0;
   }
/* EAS END */
}

// ******************************************************************
// _fprsqrta 
// ******************************************************************
static INLINE void
_fprsqrta(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         p2,
    EM_uint_t         f3)
{
EM_uint_t                tmp_isrcode, tmp_res_hi, tmp_res_lo;
EM_pair_fp_reg_type      tmp_default_result_pair;
EM_fp_reg_type           tmp_res, tmp_fr3;
EM_boolean_t             tmp_pred_hi, tmp_pred_lo;
EM_tmp_fp_env_type       tmp_fp_env;
EM_limits_check_fprsqrta limits_check = {0, 0}; 

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f3, 0, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if( fp_is_natval(FR[f3])) {
         PR[p2]  = 0;
         FR[f1]  = NATVAL;
      } else {
         tmp_default_result_pair = fprsqrta_exception_fault_check(f3, sf,
                                   &tmp_fp_env, &limits_check);
         if(fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         if (fp_is_nan(tmp_default_result_pair.hi) ) {
            tmp_res_hi = fp_single(tmp_default_result_pair.hi);
            tmp_pred_hi = 0;
         } else {
            tmp_fr3 = fp_normalize(fp_reg_read_hi(f3));
            if (fp_is_zero(tmp_fr3)) {
               tmp_res      = FP_INFINITY;
               tmp_res.sign = tmp_fr3.sign;
               tmp_pred_hi  = 0;
            } else if (fp_is_pos_inf(tmp_fr3)) {
               tmp_res     = FP_ZERO;
               tmp_pred_hi = 0;
            } else {
               tmp_res = fp_ieee_recip_sqrt(tmp_fr3);
               if (limits_check.hi) 
                  tmp_pred_hi  = 0;
               else 
                  tmp_pred_hi  = 1;
            }
            tmp_res_hi = fp_single(tmp_res);
         }


         if (fp_is_nan(tmp_default_result_pair.lo) ) {
            tmp_res_lo  = fp_single(tmp_default_result_pair.lo);
            tmp_pred_lo = 0;
         } else {
            tmp_fr3 = fp_normalize(fp_reg_read_lo(f3));
            if (fp_is_zero(tmp_fr3)) {
               tmp_res      = FP_INFINITY;
               tmp_res.sign = tmp_fr3.sign;
               tmp_pred_lo  = 0;
            } else if (fp_is_pos_inf(tmp_fr3)) {
               tmp_res      = FP_ZERO;
               tmp_pred_lo  = 0;
            } else {
               tmp_res = fp_ieee_recip_sqrt(tmp_fr3);
               if (limits_check.lo) 
                  tmp_pred_lo = 0;
               else 
                  tmp_pred_lo = 1;
            }
         tmp_res_lo = fp_single(tmp_res);
         }

         FR[f1].significand = fp_concatenate(tmp_res_hi,tmp_res_lo);
         FR[f1].exponent    = FP_INTEGER_EXP;
         FR[f1].sign        = FP_SIGN_POSITIVE;
         PR[p2]             = tmp_pred_hi & tmp_pred_lo;
         
         fp_update_fpsr(sf, tmp_fp_env);
      }
      fp_update_psr(f1);
   } else {
      PR[p2] = 0;
   }
/* EAS END */
}


// ******************************************************************
// _fmin 
// ******************************************************************
static INLINE void
_fmin(EM_state_type   *ps,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         f2,
    EM_uint_t         f3)
{
EM_uint_t          tmp_isrcode;
EM_tmp_fp_env_type tmp_fp_env;
EM_boolean_t       tmp_bool_res;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3])) {
         FR[f1] = NATVAL;
      } else {
         fminmax_exception_fault_check(f2, f3, sf, &tmp_fp_env);

         if (fp_raise_fault(tmp_fp_env)) {
             fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         tmp_bool_res = fp_less_than(fp_reg_read(FR[f2]), fp_reg_read(FR[f3]));
         FR[f1] = tmp_bool_res ? FR[f2] : FR[f3];

         fp_update_fpsr(sf, tmp_fp_env);
      }
      fp_update_psr(f1);
   }
/* EAS END */
}


// ******************************************************************
// _fmax 
// ******************************************************************
static INLINE void
_fmax(EM_state_type   *ps,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         f2,
    EM_uint_t         f3)
{
EM_uint_t          tmp_isrcode;
EM_tmp_fp_env_type tmp_fp_env;
EM_boolean_t       tmp_bool_res;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3])) {
         FR[f1] = NATVAL;
      } else {
         fminmax_exception_fault_check(f2, f3, sf, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         tmp_bool_res = fp_less_than(fp_reg_read(FR[f3]), fp_reg_read(FR[f2]));
         FR[f1] = (tmp_bool_res ? FR[f2] : FR[f3]);

         fp_update_fpsr(sf, tmp_fp_env);
      }
      fp_update_psr(f1);
   }
/* EAS END */
}


// ******************************************************************
// _famin 
// ******************************************************************
static INLINE void
_famin(EM_state_type *ps,
   EM_opcode_sf_type sf,
   EM_uint_t         qp,
   EM_uint_t         f1,
   EM_uint_t         f2,
   EM_uint_t         f3)
{
EM_uint_t          tmp_isrcode;
EM_fp_reg_type     tmp_right, tmp_left;
EM_tmp_fp_env_type tmp_fp_env;
EM_boolean_t       tmp_bool_res;


/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3])) {
         FR[f1] = NATVAL;
      } else {
         fminmax_exception_fault_check(f2, f3, sf, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         tmp_left       = fp_reg_read(FR[f2]);
         tmp_right      = fp_reg_read(FR[f3]);
         tmp_left.sign  = FP_SIGN_POSITIVE;
         tmp_right.sign = FP_SIGN_POSITIVE;
         tmp_bool_res   = fp_less_than(tmp_left, tmp_right);
         FR[f1]         = tmp_bool_res ? FR[f2] : FR[f3];

         fp_update_fpsr(sf, tmp_fp_env);
      }
      fp_update_psr(f1);
   }
/* EAS END */
}


// ******************************************************************
// _famax 
// ******************************************************************
static INLINE void
_famax(EM_state_type *ps,
   EM_opcode_sf_type sf,
   EM_uint_t         qp,
   EM_uint_t         f1,
   EM_uint_t         f2,
   EM_uint_t         f3)
{
EM_uint_t          tmp_isrcode;
EM_fp_reg_type     tmp_right, tmp_left;
EM_tmp_fp_env_type tmp_fp_env;
EM_boolean_t       tmp_bool_res;


/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, 0) ) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3])) {
         FR[f1] = NATVAL;
      } else {
         fminmax_exception_fault_check(f2, f3, sf, &tmp_fp_env);

         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         tmp_right      = fp_reg_read(FR[f2]);
         tmp_left       = fp_reg_read(FR[f3]);
         tmp_right.sign = FP_SIGN_POSITIVE;
         tmp_left.sign  = FP_SIGN_POSITIVE;
         tmp_bool_res   = fp_less_than(tmp_left, tmp_right);

         FR[f1]         = tmp_bool_res ? FR[f2] : FR[f3];
         fp_update_fpsr(sf, tmp_fp_env);
      }
   fp_update_psr(f1);
   }
/* EAS END */
}

// ******************************************************************
// _fpmin
// ******************************************************************
static INLINE void
_fpmin(EM_state_type  *ps,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         f2,
    EM_uint_t         f3)
{
EM_uint_t           tmp_isrcode, tmp_res_hi, tmp_res_lo;
EM_fp_reg_type      tmp_right, tmp_left, tmp_fr2, tmp_fr3;
EM_tmp_fp_env_type  tmp_fp_env;
EM_boolean_t        tmp_bool_res;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2] ) || fp_is_natval(FR[f3]) )  {
         FR[f1] = NATVAL;
      } else {
         fpminmax_exception_fault_check(f2, f3, sf, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         tmp_fr2 = tmp_left  = fp_reg_read_hi(f2);
         tmp_fr3 = tmp_right = fp_reg_read_hi(f3);
         tmp_bool_res        = fp_less_than(tmp_left, tmp_right);
         tmp_res_hi          = fp_single(tmp_bool_res ? tmp_fr2: tmp_fr3);

         tmp_fr2 = tmp_left  = fp_reg_read_lo(f2);
         tmp_fr3 = tmp_right = fp_reg_read_lo(f3);
         tmp_bool_res        = fp_less_than(tmp_left, tmp_right);
         tmp_res_lo          = fp_single(tmp_bool_res ? tmp_fr2: tmp_fr3);

         FR[f1].significand = fp_concatenate(tmp_res_hi, tmp_res_lo);
         FR[f1].exponent    = FP_INTEGER_EXP;
         FR[f1].sign        = FP_SIGN_POSITIVE;

         fp_update_fpsr(sf, tmp_fp_env);
      }
   fp_update_psr(f1);
   }
/* EAS END */
}

// ******************************************************************
// _fpmax
// ******************************************************************
static INLINE void
_fpmax(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t         qp,
    EM_uint_t         f1,
    EM_uint_t         f2,
    EM_uint_t         f3)
{
EM_uint_t           tmp_isrcode, tmp_res_hi, tmp_res_lo;
EM_fp_reg_type      tmp_right, tmp_left, tmp_fr2, tmp_fr3;
EM_tmp_fp_env_type  tmp_fp_env;
EM_boolean_t        tmp_bool_res;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2] ) || fp_is_natval(FR[f3]) )  {
         FR[f1] = NATVAL;
      } else {
         fpminmax_exception_fault_check(f2, f3, sf, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         tmp_fr2 = tmp_right = fp_reg_read_hi(f2);
         tmp_fr3 = tmp_left  = fp_reg_read_hi(f3);
         tmp_bool_res        = fp_less_than(tmp_left, tmp_right);
         tmp_res_hi          = fp_single(tmp_bool_res ? tmp_fr2 : tmp_fr3);

         tmp_fr2 = tmp_right = fp_reg_read_lo(f2);
         tmp_fr3 = tmp_left  = fp_reg_read_lo(f3);
         tmp_bool_res        = fp_less_than(tmp_left, tmp_right);
         tmp_res_lo          = fp_single(tmp_bool_res ? tmp_fr2: tmp_fr3);

         FR[f1].significand = fp_concatenate(tmp_res_hi, tmp_res_lo);
         FR[f1].exponent    = FP_INTEGER_EXP;
         FR[f1].sign        = FP_SIGN_POSITIVE;
         fp_update_fpsr(sf, tmp_fp_env);
      }
      fp_update_psr(f1);
   }
/* EAS END */
}

// ******************************************************************
// _fpamin
// ******************************************************************
static INLINE void
_fpamin(EM_state_type *ps,
   EM_opcode_sf_type sf,
   EM_uint_t         qp,
   EM_uint_t         f1,
   EM_uint_t         f2,
   EM_uint_t         f3)
{
EM_uint_t           tmp_isrcode, tmp_res_hi, tmp_res_lo;
EM_fp_reg_type      tmp_right, tmp_left, tmp_fr2, tmp_fr3;
EM_tmp_fp_env_type  tmp_fp_env;
EM_boolean_t        tmp_bool_res;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);
         
      if (fp_is_natval(FR[f2] ) || fp_is_natval(FR[f3]) )  {
         FR[f1] = NATVAL;
      } else {
         fpminmax_exception_fault_check(f2,f3, sf, &tmp_fp_env);
         if(fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }
   

         tmp_fr2 = tmp_left  = fp_reg_read_hi(f2);
         tmp_fr3 = tmp_right = fp_reg_read_hi(f3);
         tmp_left.sign       = FP_SIGN_POSITIVE;
         tmp_right.sign      = FP_SIGN_POSITIVE;
         tmp_bool_res        = fp_less_than(tmp_left, tmp_right);
         tmp_res_hi          = fp_single(tmp_bool_res ? tmp_fr2 : tmp_fr3);


         tmp_fr2 = tmp_left  = fp_reg_read_lo(f2);
         tmp_fr3 = tmp_right = fp_reg_read_lo(f3);
         tmp_left.sign       = FP_SIGN_POSITIVE;
         tmp_right.sign      = FP_SIGN_POSITIVE;
         tmp_bool_res        = fp_less_than(tmp_left, tmp_right);
         tmp_res_lo          = fp_single(tmp_bool_res ? tmp_fr2: tmp_fr3);

         FR[f1].significand  = fp_concatenate(tmp_res_hi, tmp_res_lo);
         FR[f1].exponent    = FP_INTEGER_EXP;
         FR[f1].sign         = FP_SIGN_POSITIVE;

         fp_update_fpsr(sf, tmp_fp_env);
      }
      fp_update_psr(f1);
   }
/* EAS END */
}


// ******************************************************************
// _fpamax
// ******************************************************************
static INLINE void
_fpamax(EM_state_type *ps,
   EM_opcode_sf_type sf,
   EM_uint_t         qp,
   EM_uint_t         f1,
   EM_uint_t         f2,
   EM_uint_t         f3)
{
EM_uint_t           tmp_isrcode, tmp_res_hi, tmp_res_lo;
EM_fp_reg_type      tmp_right, tmp_left, tmp_fr2, tmp_fr3;
EM_tmp_fp_env_type  tmp_fp_env;
EM_boolean_t        tmp_bool_res;


/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, f3, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2] ) || fp_is_natval(FR[f3]) )  {
         FR[f1] = NATVAL;
      } else {
         fpminmax_exception_fault_check(f2, f3, sf, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }


         tmp_fr2 = tmp_right = fp_reg_read_hi(f2);
         tmp_fr3 = tmp_left  = fp_reg_read_hi(f3);
         tmp_right.sign      = FP_SIGN_POSITIVE;
         tmp_left.sign       = FP_SIGN_POSITIVE;
         tmp_bool_res        = fp_less_than(tmp_left, tmp_right);
         tmp_res_hi          = fp_single(tmp_bool_res ? tmp_fr2: tmp_fr3);


         tmp_fr2 = tmp_right = fp_reg_read_lo(f2);
         tmp_fr3 = tmp_left  = fp_reg_read_lo(f3);
         tmp_right.sign      = FP_SIGN_POSITIVE;
         tmp_left.sign       = FP_SIGN_POSITIVE;
         tmp_bool_res        = fp_less_than(tmp_left, tmp_right);
         tmp_res_lo          = fp_single(tmp_bool_res ? tmp_fr2: tmp_fr3);

         FR[f1].significand = fp_concatenate(tmp_res_hi, tmp_res_lo);
         FR[f1].exponent    = FP_INTEGER_EXP;
         FR[f1].sign        = FP_SIGN_POSITIVE;

         fp_update_fpsr(sf, tmp_fp_env);
      } 
      fp_update_psr(f1);
   }
/* EAS END */
}

// ******************************************************************
// _fpcmp 
// ******************************************************************
static INLINE void
_fpcmp(EM_state_type   *ps,
   EM_opcode_frel_type frel, 
   EM_opcode_sf_type   sf,
   EM_uint_t           qp, 
   EM_uint_t           f1,
   EM_uint_t           f2,
   EM_uint_t           f3)
{
EM_uint_t           tmp_isrcode, tmp_res_hi, tmp_res_lo;
EM_fp_reg_type      tmp_fr2, tmp_fr3;
EM_tmp_fp_env_type  tmp_fp_env;
EM_boolean_t        tmp_rel;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if(tmp_isrcode = fp_reg_disabled(f1, f2,f3, 0))
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) || fp_is_natval(FR[f3]) )  {
         FR[f1] = NATVAL;
      } else {
         fpcmp_exception_fault_check(f2, f3, frel, sf, &tmp_fp_env);
   
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }
         

         tmp_fr2 = fp_reg_read_hi(f2);
         tmp_fr3 = fp_reg_read_hi(f3);

         if (frel == frelEQ)         tmp_rel = fp_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelLT)    tmp_rel = fp_less_than(tmp_fr2, tmp_fr3);
         else if (frel == frelLE)    tmp_rel = fp_lesser_or_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelGT)    tmp_rel = fp_less_than(tmp_fr3, tmp_fr2);
         else if (frel == frelGE)    tmp_rel = fp_lesser_or_equal(tmp_fr3, tmp_fr2);
         else if (frel == frelUNORD) tmp_rel = fp_unordered(tmp_fr2, tmp_fr3);
         else if (frel == frelNEQ)   tmp_rel = !fp_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelNLT)   tmp_rel = !fp_less_than(tmp_fr2, tmp_fr3);
         else if (frel == frelNLE)   tmp_rel = !fp_lesser_or_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelNGT)   tmp_rel = !fp_less_than(tmp_fr3, tmp_fr2);
         else if (frel == frelNGE)   tmp_rel = !fp_lesser_or_equal(tmp_fr3, tmp_fr2);
         else                        tmp_rel = !fp_unordered(tmp_fr2, tmp_fr3);
   
         tmp_res_hi = (tmp_rel ? 0xFFFFFFFF : 0x00000000);


         tmp_fr2 = fp_reg_read_lo(f2);
         tmp_fr3 = fp_reg_read_lo(f3);
   
         if (frel == frelEQ)         tmp_rel = fp_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelLT)    tmp_rel = fp_less_than(tmp_fr2, tmp_fr3);
         else if (frel == frelLE)    tmp_rel = fp_lesser_or_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelGT)    tmp_rel = fp_less_than(tmp_fr3, tmp_fr2);
         else if (frel == frelGE)    tmp_rel = fp_lesser_or_equal(tmp_fr3, tmp_fr2);
         else if (frel == frelUNORD) tmp_rel = fp_unordered(tmp_fr2, tmp_fr3);
         else if (frel == frelNEQ)   tmp_rel = !fp_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelNLT)   tmp_rel = !fp_less_than(tmp_fr2, tmp_fr3);
         else if (frel == frelNLE)   tmp_rel = !fp_lesser_or_equal(tmp_fr2, tmp_fr3);
         else if (frel == frelNGT)   tmp_rel = !fp_less_than(tmp_fr3, tmp_fr2);
         else if (frel == frelNGE)   tmp_rel = !fp_lesser_or_equal(tmp_fr3, tmp_fr2);
         else                        tmp_rel = !fp_unordered(tmp_fr2, tmp_fr3);
   
         tmp_res_lo = (tmp_rel ? 0xFFFFFFFF : 0x00000000);

         FR[f1].significand = fp_concatenate(tmp_res_hi, tmp_res_lo);
         FR[f1].exponent    = FP_INTEGER_EXP;
         FR[f1].sign        = FP_SIGN_POSITIVE;

         fp_update_fpsr(sf, tmp_fp_env);
      }
      fp_update_psr(f1);
   }
/* EAS END */
}

// ******************************************************************
// _fcvt_fx
// ******************************************************************
static INLINE void
_fcvt_fx(EM_state_type *ps,
   EM_opcode_sf_type sf,
   EM_uint_t         qp,
   EM_uint_t         f1,
   EM_uint_t         f2)
{
EM_uint_t          tmp_isrcode;
EM_fp_reg_type     tmp_default_result, tmp_res;
EM_tmp_fp_env_type tmp_fp_env;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, 0, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2])) {
         FR[f1] = NATVAL;
         fp_update_psr(f1);
      } else {
         tmp_default_result = fcvt_exception_fault_check(f2, sf,
                              SIGNED_FORM, TRUNC_FORM, &tmp_fp_env);

         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         if (fp_is_nan(tmp_default_result))  {
            FR[f1].significand = INTEGER_INDEFINITE;
            FR[f1].exponent    = FP_INTEGER_EXP;
            FR[f1].sign        = FP_SIGN_POSITIVE;
         } else {
            tmp_res = fp_ieee_rnd_to_int(fp_reg_read(FR[f2]), &tmp_fp_env);
            if (tmp_res.exponent)
               tmp_res.significand = fp_U64_rsh(
                           tmp_res.significand, FP_INTEGER_EXP - (EM_int_t)tmp_res.exponent);
            if (SIGNED_FORM && tmp_res.sign) 
               tmp_res.significand = (~tmp_res.significand) + 1;

            FR[f1].significand = tmp_res.significand;
            FR[f1].exponent    = FP_INTEGER_EXP;
            FR[f1].sign        = FP_SIGN_POSITIVE;
         }

         fp_update_fpsr(sf, tmp_fp_env);
         fp_update_psr(f1);
         if (fp_raise_traps(tmp_fp_env))
            fp_exception_trap(fp_decode_trap(tmp_fp_env));
      }
   }
/* EAS END */
}

// ******************************************************************
// _fpcvt_fx
// ******************************************************************
static INLINE void
_fpcvt_fx(EM_state_type     *ps,
   EM_opcode_sf_type sf,
   EM_uint_t         qp,
   EM_uint_t         f1,
   EM_uint_t         f2)
{
EM_uint_t           tmp_isrcode, tmp_res_lo, tmp_res_hi;
EM_pair_fp_reg_type tmp_default_result_pair;
EM_fp_reg_type      tmp_res;
EM_tmp_fp_env_type  tmp_fp_env;

/* EAS START */
   if (PR[qp]) {
      fp_check_target_register(f1);
      if (tmp_isrcode = fp_reg_disabled(f1, f2, 0, 0)) 
         disabled_fp_register_fault(tmp_isrcode,0);

      if (fp_is_natval(FR[f2]) )  {
         FR[f1] = NATVAL;
         fp_update_psr(f1);
      } else {
         tmp_default_result_pair = fpcvt_exception_fault_check(f2, sf, 
                                   SIGNED_FORM, TRUNC_FORM, &tmp_fp_env);
         if (fp_raise_fault(tmp_fp_env)) {
            fp_exception_fault(fp_decode_fault(tmp_fp_env));
            return; // MACH
         }

         if (fp_is_nan(tmp_default_result_pair.hi)) {
            tmp_res_hi = INTEGER_INDEFINITE_32_BIT;
         } else {
            tmp_res = fp_ieee_rnd_to_int_sp(fp_reg_read_hi(f2), high, &tmp_fp_env);
            if (tmp_res.exponent)
               tmp_res.significand = fp_U64_rsh(
                        tmp_res.significand, (FP_INTEGER_EXP - tmp_res.exponent)); 
            if (SIGNED_FORM && tmp_res.sign) 
               tmp_res.significand = (~tmp_res.significand) + 1;

            tmp_res_hi = fp_extract_bits(tmp_res.significand, 31, 0);
         }

         if (fp_is_nan(tmp_default_result_pair.lo)) {
            tmp_res_lo = INTEGER_INDEFINITE_32_BIT;
         } else {
            tmp_res = fp_ieee_rnd_to_int_sp(fp_reg_read_lo(f2), low, &tmp_fp_env);
            if (tmp_res.exponent)
               tmp_res.significand = fp_U64_rsh(
                        tmp_res.significand,(FP_INTEGER_EXP - tmp_res.exponent)); 
            if (SIGNED_FORM && tmp_res.sign) 
               tmp_res.significand = (~tmp_res.significand) + 1;

            tmp_res_lo = fp_extract_bits(tmp_res.significand, 31, 0);
         }
         FR[f1].significand = fp_concatenate(tmp_res_hi, tmp_res_lo);
         FR[f1].exponent    = FP_INTEGER_EXP;
         FR[f1].sign        = FP_SIGN_POSITIVE;

         fp_update_fpsr(sf, tmp_fp_env);
         fp_update_psr(f1);
         if (fp_raise_traps(tmp_fp_env))
            fp_exception_trap(fp_decode_trap(tmp_fp_env));
      } 
   }
/* EAS END */
}


/********** fp_ieee_recip **********/

static EM_fp_reg_type
fp_ieee_recip(
    EM_fp_reg_type den)
{
EM_uint_t      tmp_index;
EM_fp_reg_type tmp_res;

/* EAS START */
    const EM_uint_t RECIP_TABLE[256] = {
         0x3fc, 0x3f4, 0x3ec, 0x3e4, 0x3dd, 0x3d5, 0x3cd, 0x3c6,
         0x3be, 0x3b7, 0x3af, 0x3a8, 0x3a1, 0x399, 0x392, 0x38b,
         0x384, 0x37d, 0x376, 0x36f, 0x368, 0x361, 0x35b, 0x354,
         0x34d, 0x346, 0x340, 0x339, 0x333, 0x32c, 0x326, 0x320,
         0x319, 0x313, 0x30d, 0x307, 0x300, 0x2fa, 0x2f4, 0x2ee,
         0x2e8, 0x2e2, 0x2dc, 0x2d7, 0x2d1, 0x2cb, 0x2c5, 0x2bf,
         0x2ba, 0x2b4, 0x2af, 0x2a9, 0x2a3, 0x29e, 0x299, 0x293,
         0x28e, 0x288, 0x283, 0x27e, 0x279, 0x273, 0x26e, 0x269,
         0x264, 0x25f, 0x25a, 0x255, 0x250, 0x24b, 0x246, 0x241,
         0x23c, 0x237, 0x232, 0x22e, 0x229, 0x224, 0x21f, 0x21b,
         0x216, 0x211, 0x20d, 0x208, 0x204, 0x1ff, 0x1fb, 0x1f6,
         0x1f2, 0x1ed, 0x1e9, 0x1e5, 0x1e0, 0x1dc, 0x1d8, 0x1d4,
         0x1cf, 0x1cb, 0x1c7, 0x1c3, 0x1bf, 0x1bb, 0x1b6, 0x1b2,
         0x1ae, 0x1aa, 0x1a6, 0x1a2, 0x19e, 0x19a, 0x197, 0x193,
         0x18f, 0x18b, 0x187, 0x183, 0x17f, 0x17c, 0x178, 0x174,
         0x171, 0x16d, 0x169, 0x166, 0x162, 0x15e, 0x15b, 0x157,
         0x154, 0x150, 0x14d, 0x149, 0x146, 0x142, 0x13f, 0x13b,
         0x138, 0x134, 0x131, 0x12e, 0x12a, 0x127, 0x124, 0x120,
         0x11d, 0x11a, 0x117, 0x113, 0x110, 0x10d, 0x10a, 0x107,
         0x103, 0x100, 0x0fd, 0x0fa, 0x0f7, 0x0f4, 0x0f1, 0x0ee,
         0x0eb, 0x0e8, 0x0e5, 0x0e2, 0x0df, 0x0dc, 0x0d9, 0x0d6,
         0x0d3, 0x0d0, 0x0cd, 0x0ca, 0x0c8, 0x0c5, 0x0c2, 0x0bf,
         0x0bc, 0x0b9, 0x0b7, 0x0b4, 0x0b1, 0x0ae, 0x0ac, 0x0a9,
         0x0a6, 0x0a4, 0x0a1, 0x09e, 0x09c, 0x099, 0x096, 0x094,
         0x091, 0x08e, 0x08c, 0x089, 0x087, 0x084, 0x082, 0x07f,
         0x07c, 0x07a, 0x077, 0x075, 0x073, 0x070, 0x06e, 0x06b,
         0x069, 0x066, 0x064, 0x061, 0x05f, 0x05d, 0x05a, 0x058,
         0x056, 0x053, 0x051, 0x04f, 0x04c, 0x04a, 0x048, 0x045,
         0x043, 0x041, 0x03f, 0x03c, 0x03a, 0x038, 0x036, 0x033,
         0x031, 0x02f, 0x02d, 0x02b, 0x029, 0x026, 0x024, 0x022,
         0x020, 0x01e, 0x01c, 0x01a, 0x018, 0x015, 0x013, 0x011,
         0x00f, 0x00d, 0x00b, 0x009, 0x007, 0x005, 0x003, 0x001,
    };


    tmp_index            = fp_extract_bits(den.significand,62,55);
    tmp_res.significand  = ((EM_uint64_t)1 << 63) | ((EM_uint64_t)RECIP_TABLE[tmp_index] << 53);
    tmp_res.exponent     = (EM_int_t)FP_REG_EXP_ONES - 2 - (EM_int_t)den.exponent;
    tmp_res.sign         = den.sign;
    return (tmp_res);

/* EAS END */
}


/********** fp_ieee_recip_sqrt **********/

static EM_fp_reg_type
fp_ieee_recip_sqrt(
    EM_fp_reg_type root)
{
EM_uint_t tmp_index;
EM_fp_reg_type tmp_res;

/* EAS START */
    const EM_uint_t RECIP_SQRT_TABLE[256] = {
         0x1a5, 0x1a0, 0x19a, 0x195, 0x18f, 0x18a, 0x185, 0x180,
         0x17a, 0x175, 0x170, 0x16b, 0x166, 0x161, 0x15d, 0x158,
         0x153, 0x14e, 0x14a, 0x145, 0x140, 0x13c, 0x138, 0x133,
         0x12f, 0x12a, 0x126, 0x122, 0x11e, 0x11a, 0x115, 0x111,
         0x10d, 0x109, 0x105, 0x101, 0x0fd, 0x0fa, 0x0f6, 0x0f2,
         0x0ee, 0x0ea, 0x0e7, 0x0e3, 0x0df, 0x0dc, 0x0d8, 0x0d5,
         0x0d1, 0x0ce, 0x0ca, 0x0c7, 0x0c3, 0x0c0, 0x0bd, 0x0b9,
         0x0b6, 0x0b3, 0x0b0, 0x0ad, 0x0a9, 0x0a6, 0x0a3, 0x0a0,
         0x09d, 0x09a, 0x097, 0x094, 0x091, 0x08e, 0x08b, 0x088,
         0x085, 0x082, 0x07f, 0x07d, 0x07a, 0x077, 0x074, 0x071,
         0x06f, 0x06c, 0x069, 0x067, 0x064, 0x061, 0x05f, 0x05c,
         0x05a, 0x057, 0x054, 0x052, 0x04f, 0x04d, 0x04a, 0x048,
         0x045, 0x043, 0x041, 0x03e, 0x03c, 0x03a, 0x037, 0x035,
         0x033, 0x030, 0x02e, 0x02c, 0x029, 0x027, 0x025, 0x023,
         0x020, 0x01e, 0x01c, 0x01a, 0x018, 0x016, 0x014, 0x011,
         0x00f, 0x00d, 0x00b, 0x009, 0x007, 0x005, 0x003, 0x001,
         0x3fc, 0x3f4, 0x3ec, 0x3e5, 0x3dd, 0x3d5, 0x3ce, 0x3c7,
         0x3bf, 0x3b8, 0x3b1, 0x3aa, 0x3a3, 0x39c, 0x395, 0x38e,
         0x388, 0x381, 0x37a, 0x374, 0x36d, 0x367, 0x361, 0x35a,
         0x354, 0x34e, 0x348, 0x342, 0x33c, 0x336, 0x330, 0x32b,
         0x325, 0x31f, 0x31a, 0x314, 0x30f, 0x309, 0x304, 0x2fe,
         0x2f9, 0x2f4, 0x2ee, 0x2e9, 0x2e4, 0x2df, 0x2da, 0x2d5,
         0x2d0, 0x2cb, 0x2c6, 0x2c1, 0x2bd, 0x2b8, 0x2b3, 0x2ae,
         0x2aa, 0x2a5, 0x2a1, 0x29c, 0x298, 0x293, 0x28f, 0x28a,
         0x286, 0x282, 0x27d, 0x279, 0x275, 0x271, 0x26d, 0x268,
         0x264, 0x260, 0x25c, 0x258, 0x254, 0x250, 0x24c, 0x249,
         0x245, 0x241, 0x23d, 0x239, 0x235, 0x232, 0x22e, 0x22a,
         0x227, 0x223, 0x220, 0x21c, 0x218, 0x215, 0x211, 0x20e,
         0x20a, 0x207, 0x204, 0x200, 0x1fd, 0x1f9, 0x1f6, 0x1f3,
         0x1f0, 0x1ec, 0x1e9, 0x1e6, 0x1e3, 0x1df, 0x1dc, 0x1d9,
         0x1d6, 0x1d3, 0x1d0, 0x1cd, 0x1ca, 0x1c7, 0x1c4, 0x1c1,
         0x1be, 0x1bb, 0x1b8, 0x1b5, 0x1b2, 0x1af, 0x1ac, 0x1aa,
    };

        tmp_index =  (fp_extract_bits((EM_uint64_t)root.exponent,0,0) << 7) |
                     fp_extract_bits(root.significand,62,56);

        tmp_res.significand  = ((EM_uint64_t)1 << 63) |((EM_uint64_t)RECIP_SQRT_TABLE[tmp_index] << 53);
        tmp_res.exponent     = ((EM_int_t)FP_REG_EXP_HALF) - ((((EM_int_t)root.exponent) - ((EM_int_t)FP_REG_BIAS)) >> 1);
        tmp_res.sign         = FP_SIGN_POSITIVE;
        return(tmp_res);
    
/* EAS END */
}
