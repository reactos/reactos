// ******************************
// Intel Confidential
// ******************************

#ifndef _EM_INSTR_H
#define _EM_INSTR_H

#include "fepublic.h"

static EM_fp_reg_type
fp_ieee_recip(
    EM_fp_reg_type den);

static EM_fp_reg_type
fp_ieee_recip_sqrt(
    EM_fp_reg_type root);

// ********************************
// Instruction Pages Prototypes 
// ********************************

// Floating-point Multiply Add 
static INLINE void
_fma(EM_state_type *ps,
    EM_opcode_pc_type pc,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f3,
    EM_uint_t f4,
    EM_uint_t f2);

// Floating-point Parallel Multiply Add 
static INLINE void
_fpma(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f3,
    EM_uint_t f4,
    EM_uint_t f2);

// Floating-point Multiply Subtract 
static INLINE void
_fms(EM_state_type *ps,
    EM_opcode_pc_type pc,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f3,
    EM_uint_t f4,
    EM_uint_t f2);

// Floating-point Parallel Multiply Subtract 
static INLINE void
_fpms(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f3,
    EM_uint_t f4,
    EM_uint_t f2);

// Floating-point Negative Multiply Add 
static INLINE void
_fnma(EM_state_type *ps,
    EM_opcode_pc_type pc,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f3,
    EM_uint_t f4,
    EM_uint_t f2);


// Floating-point Parallel Negative Multiply Add 
static INLINE void
_fpnma(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f3,
    EM_uint_t f4,
    EM_uint_t f2);


// Floating-point Compare 
static INLINE void
_fcmp(EM_state_type *ps,
    EM_opcode_frel_type frel,
    EM_opcode_ctype_type ctype,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t p1,
    EM_uint_t p2,
    EM_uint_t f2,
    EM_uint_t f3);

// Floating-point Reciprocal Approximation 
static INLINE void
_frcpa(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t p2,
    EM_uint_t f2,
    EM_uint_t f3);

// Floating-point Parallel Reciprocal Approximation 
static INLINE void
_fprcpa(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t p2,
    EM_uint_t f2,
    EM_uint_t f3);

// Floating-point Reciprocal Square Root Approximation 
static INLINE void
_frsqrta(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t p2,
    EM_uint_t f3);

// Floating-point Parallel Reciprocal Square Root Approximation 
static INLINE void
_fprsqrta(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t p2,
    EM_uint_t f3);

// Floating-point Minimum 
static INLINE void
_fmin(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f2,
    EM_uint_t f3);

static INLINE void
_fmax(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f2,
    EM_uint_t f3);

// Floating-point Absolute Minimum 
static INLINE void
_famin(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f2,
    EM_uint_t f3);

// Floating-point Absolute Maximum 
static INLINE void
_famax(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f2,
    EM_uint_t f3);

// Floating-point Parallel Minimum 
static INLINE void
_fpmin(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f2,
    EM_uint_t f3);

// Floating-point Parallel Minimum 
static INLINE void
_fpmax(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f2,
    EM_uint_t f3);


// Floating-point Parallel Absolute Minimum 
static INLINE void
_fpamin(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f2,
    EM_uint_t f3);

// Floating-point Parallel Absolute Maximum 
static INLINE void
_fpamax(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f2,
    EM_uint_t f3);

// Floating-point Parallel Compare 
static INLINE void
_fpcmp(EM_state_type *ps,
    EM_opcode_frel_type frel,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f2,
    EM_uint_t f3);

// Convert Floating-point to Integer 
static INLINE void
_fcvt_fx(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f2);

// Parallel Convert Floating-point to Integer 
static INLINE void
_fpcvt_fx(EM_state_type *ps,
    EM_opcode_sf_type sf,
    EM_uint_t qp,
    EM_uint_t f1,
    EM_uint_t f2);

#endif

#undef GETSTATE_F1
#undef PUTSTATE_F1

#undef GETSTATE_F4
#undef PUTSTATE_F4

#undef GETSTATE_F6
#undef PUTSTATE_F6

#undef GETSTATE_F7
#undef PUTSTATE_F7

#undef GETSTATE_F8
#undef PUTSTATE_F8

#undef GETSTATE_F10
#undef PUTSTATE_F10

#define GETSTATE_F1(qp,f1,f3,f4,f2)        _GETSTATE_F1(ps, qp,f1,f3,f4,f2)
#define PUTSTATE_F1(f1)                    _PUTSTATE_F1(ps, f1)

#define GETSTATE_F4(qp,p1,p2,f2,f3)         _GETSTATE_F4(ps, qp,p1,p2,f2,f3)
#define PUTSTATE_F4(p1,p2)                  _PUTSTATE_F4(ps, p1,p2)

#define GETSTATE_F6(qp,f1,p2,f2,f3)         _GETSTATE_F6(ps,qp,f1,p2,f2,f3)
#define PUTSTATE_F6(f1,p2)                  _PUTSTATE_F6(ps,f1,p2)

#define GETSTATE_F7(qp,f1,p2,f3)            _GETSTATE_F7(ps, qp,f1,p2,f3)
#define PUTSTATE_F7(f1,p2)                  _PUTSTATE_F7(ps, f1,p2)

#define GETSTATE_F8(qp,f1,f2,f3)           _GETSTATE_F8(ps, qp,f1,f2,f3)
#define PUTSTATE_F8(f1)                    _PUTSTATE_F8(ps, f1)

#define GETSTATE_F10(qp,f1,f2)             _GETSTATE_F10(ps, qp,f1,f2)
#define PUTSTATE_F10(f1)                   _PUTSTATE_F10(ps, f1)

/***********************************************************
undefs 
************************************************************/

#undef fp_reg_disabled
#undef fp_reg_read_hi
#undef fp_reg_read_lo
#undef fp_ieee_rnd_to_int
#undef fp_ieee_rnd_to_int_sp
#undef fp_ieee_round_sp
#undef fp_ieee_round

#undef fminmax_exception_fault_check 
#undef fpminmax_exception_fault_check 
#undef fcmp_exception_fault_check
#undef fpcmp_exception_fault_check

#undef fcvt_exception_fault_check
#undef fpcvt_exception_fault_check

#undef fma_exception_fault_check
#undef fpma_exception_fault_check
#undef fms_fnma_exception_fault_check
#undef fpms_fpnma_exception_fault_check

#undef frcpa_exception_fault_check
#undef fprcpa_exception_fault_check
#undef frsqrta_exception_fault_check
#undef fprsqrta_exception_fault_check

#undef illegal_operation_fault
#undef fp_check_target_register 

#undef fp_exception_fault
#undef fp_exception_trap

#undef disabled_fp_register_fault

#undef fp_update_fpsr 
#undef fp_update_psr 

/***********************************************************
redefinitions 
************************************************************/
#define fp_reg_disabled(f1,f2,f3,f4)         fp82_fp_reg_disabled(ps, f1,f2,f3,f4)


#define fp_reg_read_hi(f2)            fp82_reg_read_hi(ps,f2)
#define fp_reg_read_lo(f3)            fp82_reg_read_lo(ps,f3)

#define fp_ieee_rnd_to_int(arg1,arg2) \
        fp82_fp_ieee_rnd_to_int(ps, arg1, arg2)
#define fp_ieee_rnd_to_int_sp(arg1,arg2, arg3) \
        fp82_fp_ieee_rnd_to_int_sp(ps, arg1, arg2, arg3)


#define fp_ieee_round_sp(arg1, arg2, arg3) \
        fp82_fp_ieee_round_sp(ps, arg1, arg2, arg3)
#define fp_ieee_round(arg1, arg2) \
        fp82_fp_ieee_round(ps, arg1, arg2)

#define fminmax_exception_fault_check(arg1, arg2, arg3, arg4) \
        fp82_fminmax_exception_fault_check(ps, arg1, arg2, arg3, arg4) 
#define fpminmax_exception_fault_check(arg1, arg2, arg3, arg4) \
        fp82_fpminmax_exception_fault_check(ps, arg1, arg2, arg3, arg4) 

#define fcmp_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fcmp_exception_fault_check(ps, arg1, arg2, arg3, arg4, arg5)
#define fpcmp_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fpcmp_exception_fault_check(ps, arg1, arg2, arg3, arg4, arg5)

#define fcvt_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fcvt_exception_fault_check(ps, arg1, arg2, arg3, arg4, arg5)
#define fpcvt_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fpcvt_exception_fault_check(ps, arg1, arg2, arg3, arg4, arg5)

#define fma_exception_fault_check(arg1, arg2, arg3, arg4, arg5, arg6) \
        fp82_fma_exception_fault_check(ps, arg1, arg2, arg3, arg4, arg5, arg6)
#define fpma_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fpma_exception_fault_check(ps, arg1, arg2, arg3, arg4, arg5)

#define fms_fnma_exception_fault_check(arg1, arg2, arg3, arg4, arg5, arg6) \
        fp82_fms_fnma_exception_fault_check(ps, arg1, arg2, arg3, arg4, arg5, arg6)

#define fpms_fpnma_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fpms_fpnma_exception_fault_check(ps, arg1, arg2, arg3, arg4, arg5)

#define frcpa_exception_fault_check(arg1, arg2, arg3, arg4) \
        fp82_frcpa_exception_fault_check(ps, arg1, arg2, arg3, arg4)

#define fprcpa_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fprcpa_exception_fault_check(ps, arg1, arg2, arg3, arg4, arg5)

#define frsqrta_exception_fault_check(arg1, arg2, arg3) \
        fp82_frsqrta_exception_fault_check(ps, arg1, arg2, arg3)

#define fprsqrta_exception_fault_check(arg1, arg2, arg3, arg4) \
        fp82_fprsqrta_exception_fault_check(ps, arg1, arg2, arg3, arg4)

#define illegal_operation_fault( NON_RS) \
        fp82_illegal_operation_fault(ps, NON_RS) 

#define fp_check_target_register( reg_specifier) \
        fp82_fp_check_target_register(ps, reg_specifier)

#define fp_exception_fault( tmp) \
        fp82_fp_exception_fault(ps, tmp) 

#define fp_exception_trap( tmp) \
        fp82_fp_exception_trap(ps, tmp) 

#define disabled_fp_register_fault(isr_code, itype) \
        fp82_disabled_fp_register_fault(ps, isr_code, itype) 

#define fp_update_fpsr(sf, tmp_env)    fp82_fp_update_fpsr(ps,sf, tmp_env)
#define fp_update_psr(dest_freg)       fp82_fp_update_psr(ps,dest_freg)
    

