/******************************
Intel Confidential
******************************/

#ifndef _EM_HELPER_H
#define _EM_HELPER_H

#include "fepublic.h"
#undef FP_QNAN
#undef FP_SNAN

/***************************************************************************
defines to get rid of ps in the prototypes and the 
definitions in EM_helper.c
***************************************************************************/

#define fp_reg_read_hi(arg1) \
        fp82_reg_read_hi(EM_state_type *ps, arg1)
#define fp_reg_read_lo(arg1) \
        fp82_reg_read_lo(EM_state_type *ps, arg1)

#define fp_decode_environment(arg1, arg2, arg3) \
        fp82_fp_decode_environment(EM_state_type *ps, arg1, arg2, arg3)

#define fp_ieee_rnd_to_int(arg1, arg2) \
        fp82_fp_ieee_rnd_to_int(EM_state_type *ps, arg1, arg2) 

#define fp_ieee_rnd_to_int_sp(arg1, arg2, arg3) \
        fp82_fp_ieee_rnd_to_int_sp(EM_state_type *ps, arg1, arg2, arg3) 

#define fp_ieee_round_sp(arg1, arg2, arg3) \
        fp82_fp_ieee_round_sp(EM_state_type *ps, arg1, arg2, arg3)

#define fp_ieee_round(arg1,arg2) \
        fp82_fp_ieee_round(EM_state_type *ps, arg1, arg2)

#define fp_reg_disabled(arg1, arg2, arg3, arg4) \
        fp82_fp_reg_disabled(EM_state_type *ps, arg1, arg2, arg3, arg4)

#define fminmax_exception_fault_check(arg1, arg2, arg3, arg4) \
        fp82_fminmax_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4)
#define fpminmax_exception_fault_check(arg1, arg2, arg3, arg4) \
        fp82_fpminmax_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4)

#define fcmp_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fcmp_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4, arg5) 
#define fpcmp_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fpcmp_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4, arg5) 

#define fcvt_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fcvt_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4, arg5) 

#define fpcvt_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fpcvt_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4, arg5) 

#define fpma_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fpma_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4, arg5) 
    
#define fma_exception_fault_check(arg1, arg2, arg3, arg4, arg5, arg6) \
        fp82_fma_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4, arg5, arg6) 

#define fms_fnma_exception_fault_check(arg1, arg2, arg3, arg4, arg5, arg6) \
        fp82_fms_fnma_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4, arg5, arg6) 

#define fpms_fpnma_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fpms_fpnma_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4, arg5) 

#define frcpa_exception_fault_check(arg1, arg2, arg3, arg4) \
        fp82_frcpa_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4) 
    
#define fprcpa_exception_fault_check(arg1, arg2, arg3, arg4, arg5) \
        fp82_fprcpa_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4, arg5) 

#define frsqrta_exception_fault_check(arg1, arg2, arg3) \
        fp82_frsqrta_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3) 
    
#define fprsqrta_exception_fault_check(arg1, arg2, arg3, arg4) \
        fp82_fprsqrta_exception_fault_check(EM_state_type *ps, arg1, arg2, arg3, arg4) 
    
#define fp_update_fpsr(arg1, arg2)     fp82_fp_update_fpsr(EM_state_type *ps, arg1, arg2)
#define fp_update_psr(arg1)            fp82_fp_update_psr(EM_state_type *ps, arg1)



/* Function Prototypes for Helper functions */

#define SET_STATUS_FLAG(status_flag) { \
   status_flag = 1; \
}

#define CLEAR_STATUS_FLAG(status_flag) { \
   status_flag = 0; \
}



// helper functions declarations 

EM_uint_t
fp_extract_bits(
   EM_uint64_t  input_value,
   unsigned int hi_bound,
   unsigned int lo_bound);

EM_uint64_t
fp_concatenate(EM_uint_t hi_val, EM_uint_t lo_val);

INLINE EM_boolean_t
fp_equal(
   EM_fp_reg_type fr1, EM_fp_reg_type fr2);

INLINE EM_boolean_t
fp_less_than(
   EM_fp_reg_type fr1, 
   EM_fp_reg_type fr2);

INLINE EM_boolean_t
fp_lesser_or_equal(
   EM_fp_reg_type fr1, EM_fp_reg_type fr2);

INLINE EM_boolean_t
fp_unordered(
   EM_fp_reg_type fr1, EM_fp_reg_type fr2);

EM_uint_t
fp82_fp_decode_fault(
   EM_tmp_fp_env_type tmp_fp_env);

EM_uint_t
fp82_fp_decode_trap(
   EM_tmp_fp_env_type tmp_fp_env);

// MACH
void
fp_decode_environment(
   EM_opcode_pc_type pc,
   EM_opcode_sf_type sf,
   EM_tmp_fp_env_type *tmp_fp_env);

EM_uint_t
fp_reg_disabled(
   EM_uint_t f1,
   EM_uint_t f2,
   EM_uint_t f3,
   EM_uint_t f4);

INLINE EM_boolean_t
fp_is_nan_or_inf(
   EM_fp_reg_type tmp_res);

INLINE EM_fp_reg_type
fp_dp_to_fr(
   EM_fp_dp_type tmp_res);

INLINE EM_fp_dp_type
fp_add(
   EM_fp_dp_type fp_dp, EM_fp_reg_type fr2, 
   EM_tmp_fp_env_type tmp_fp_env);

INLINE void
fcmp_exception_fault_check(
   EM_fp_reg_specifier f2,
   EM_fp_reg_specifier f3,
   EM_opcode_frel_type frel,
   EM_opcode_sf_type   sf,   
   EM_tmp_fp_env_type  *tmp_fp_env);

INLINE void
fpcmp_exception_fault_check(
   EM_fp_reg_specifier f2,
   EM_fp_reg_specifier f3,
   EM_opcode_frel_type frel,
   EM_opcode_sf_type   sf,
   EM_tmp_fp_env_type  *tmp_fp_env);

INLINE EM_fp_reg_type
fcvt_exception_fault_check(
   EM_fp_reg_specifier f2, 
   EM_opcode_sf_type   sf,
   EM_boolean_t        signed_form,
   EM_boolean_t        trunc_form, 
   EM_tmp_fp_env_type  *tmp_fp_env);

EM_pair_fp_reg_type
fpcvt_exception_fault_check(
   EM_fp_reg_specifier f2, 
   EM_opcode_sf_type   sf, 
   EM_boolean_t        signed_form,
   EM_boolean_t        trunc_form, 
   EM_tmp_fp_env_type  *tmp_fp_env);

EM_fp_reg_type
fma_exception_fault_check(
   EM_fp_reg_specifier f2,
   EM_fp_reg_specifier f3,
   EM_fp_reg_specifier f4,
   EM_opcode_pc_type   pc,
   EM_opcode_sf_type   sf, 
   EM_tmp_fp_env_type  *tmp_fp_env);

EM_pair_fp_reg_type
fpma_exception_fault_check(
   EM_fp_reg_specifier f2,
   EM_fp_reg_specifier f3,
   EM_fp_reg_specifier f4,
   EM_opcode_sf_type   sf,
   EM_tmp_fp_env_type *tmp_fp_env);

INLINE void
fminmax_exception_fault_check(
   EM_fp_reg_specifier f2,
   EM_fp_reg_specifier f3,
   EM_opcode_sf_type   sf,  
   EM_tmp_fp_env_type  *tmp_fp_env);


INLINE void 
fpminmax_exception_fault_check(
   EM_uint_t          f2,
   EM_uint_t          f3,
   EM_opcode_sf_type  sf,
   EM_tmp_fp_env_type *tmp_fp_env);

EM_fp_reg_type
fms_fnma_exception_fault_check(
   EM_fp_reg_specifier f2,
   EM_fp_reg_specifier f3,
   EM_fp_reg_specifier f4,
   EM_opcode_pc_type   pc,
   EM_opcode_sf_type   sf, 
   EM_tmp_fp_env_type  *tmp_fp_env);


EM_pair_fp_reg_type
fpms_fpnma_exception_fault_check(
   EM_fp_reg_specifier f2,
   EM_fp_reg_specifier f3,
   EM_fp_reg_specifier f4,
   EM_opcode_sf_type   sf,
   EM_tmp_fp_env_type  *tmp_fp_env);


INLINE EM_boolean_t
fp_flag_set_is_enabled(
   EM_uint_t flags, EM_uint_t traps);
 
INLINE EM_boolean_t
fp_flag_set_is_clear(
   EM_uint_t flags, EM_uint_t traps);

INLINE EM_fp_dp_type
fp_max_or_infinity(
   EM_uint_t sign,  EM_tmp_fp_env_type *tmp_fp_env,
   EM_uint_t e_max, EM_uint128_t       max_significand);

INLINE EM_fp_dp_type
fp_mul(
   EM_fp_reg_type fr3, EM_fp_reg_type fr4);

INLINE EM_fp_reg_type
fp_normalize(EM_fp_reg_type freg);

INLINE EM_fp_dp_type
fp_normalize_dp(EM_fp_dp_type fp_dp);

EM_fp_dp_type
fp82_fp_fr_to_dp(EM_fp_reg_type fr1);

INLINE EM_memory_type
fr_to_mem4_bias_adjust(EM_fp_reg_type freg);

EM_fp_reg_type
frcpa_exception_fault_check(
   EM_fp_reg_specifier f2,
   EM_fp_reg_specifier f3,
   EM_opcode_sf_type   sf,
   EM_tmp_fp_env_type  *tmp_fp_env);

EM_pair_fp_reg_type
fprcpa_exception_fault_check(
   EM_fp_reg_specifier     f2,
   EM_fp_reg_specifier     f3,
   EM_opcode_sf_type       sf,
   EM_tmp_fp_env_type      *tmp_fp_env, 
   EM_limits_check_fprcpa  *limits_check);

EM_fp_reg_type
frsqrta_exception_fault_check(
   EM_fp_reg_specifier f3, 
   EM_opcode_sf_type   sf,
   EM_tmp_fp_env_type  *tmp_fp_env);

EM_pair_fp_reg_type
fprsqrta_exception_fault_check(
   EM_fp_reg_specifier       f3, 
   EM_opcode_sf_type         sf,
   EM_tmp_fp_env_type        *tmp_fp_env,
   EM_limits_check_fprsqrta  *limits_check);

EM_fp_reg_type
fp_ieee_rnd_to_int(
   EM_fp_reg_type     fr1, 
   EM_tmp_fp_env_type *tmp_fp_env);

EM_fp_reg_type
fp_ieee_rnd_to_int_sp(
    EM_fp_reg_type     fr2, 
    EM_simd_hilo       hilo,
    EM_tmp_fp_env_type *tmp_fp_env);

EM_fp_reg_type
fp_ieee_round(
   EM_fp_dp_type      fp_dp,
   EM_tmp_fp_env_type *tmp_fp_env);

INLINE EM_boolean_t
fp_is_finite(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_inf(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_inf_dp(EM_fp_dp_type tmp_res);

INLINE EM_boolean_t
fp_is_nan(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_nan_dp(EM_fp_dp_type tmp_res);

INLINE EM_boolean_t
fp_is_natval(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_neg_dp(EM_fp_dp_type tmp_res);

INLINE EM_boolean_t
fp_is_neg_inf(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_neg_non_zero(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_normal(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_normal_dp(EM_fp_dp_type tmp_res);

INLINE EM_boolean_t
fp_is_pos_dp(EM_fp_dp_type tmp_res);

INLINE EM_boolean_t
fp_is_pos_inf(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_pos_non_zero(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_pseudo_zero(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_qnan(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_snan(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_unorm(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_unsupported(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_unsupported_dp(EM_fp_dp_type tmp_res);

INLINE EM_boolean_t
fp_is_zero(EM_fp_reg_type freg);

INLINE EM_boolean_t
fp_is_zero_dp(EM_fp_dp_type tmp_res);

EM_int_t
fp82_fp_U64_lead0(EM_uint64_t value);

INLINE EM_int_t
fp_U128_lead0(EM_uint128_t value);

EM_int_t
fp82_fp_U256_lead0(EM_uint256_t value);

EM_fp_reg_type
fp_mem_to_fr_format(
   EM_memory_type mem,
   EM_uint_t      size, 
   EM_uint_t      integer_form);

EM_memory_type
fp_fr_to_mem_format(
   EM_fp_reg_type freg,
   EM_uint_t      size, 
   EM_uint_t      integer_form);

INLINE EM_fp_reg_type
fp_make_quiet_nan(EM_fp_reg_type freg);

EM_boolean_t
fp82_fp_raise_fault(EM_tmp_fp_env_type tmp_fp_env);

EM_boolean_t
fp82_fp_raise_traps(EM_tmp_fp_env_type tmp_fp_env);

INLINE EM_fp_reg_type
fp_reg_read(EM_fp_reg_type freg);

INLINE EM_fp_reg_type
fp_reg_read_hi(EM_uint_t freg);

INLINE EM_fp_reg_type
fp_reg_read_lo(EM_uint_t freg);

INLINE EM_uint_t
fp_single(EM_fp_reg_type freg);

EM_uint_t
fp_ieee_round_sp(
   EM_fp_dp_type      fp_dp,
   EM_simd_hilo       hilo,
   EM_tmp_fp_env_type *tmp_fp_env);

INLINE void
fp_ieee_to_hilo(
    EM_simd_hilo       hilo,
    EM_tmp_fp_env_type *tmp_fp_env);

EM_uint_t fp82_no_bytes(char *str);

EM_uint_t fp82_not_int(char *str);

INLINE void
fp_update_fpsr(
   EM_opcode_sf_type  sf,
   EM_tmp_fp_env_type tmp_fp_env);

INLINE void
fp_update_psr(
   EM_uint_t dest_freg);

/* Basic Types Support Functions */

/* 128-bit unsigned int support routines */

EM_boolean_t
fp82_fp_U128_eq(EM_uint128_t value1, EM_uint128_t value2);

static INLINE EM_boolean_t
fp_U128_ge(EM_uint128_t value1, EM_uint128_t value2);

static INLINE EM_boolean_t
fp_U128_gt(EM_uint128_t value1, EM_uint128_t value2);

static INLINE EM_boolean_t
fp_U128_le(EM_uint128_t value1, EM_uint128_t value2);

EM_boolean_t
fp82_fp_U128_lt(EM_uint128_t value1, EM_uint128_t value2);

EM_boolean_t
fp82_ne_U128(EM_uint128_t value1, EM_uint128_t value2);

static INLINE EM_uint128_t
bld_U128(EM_uint64_t hi, EM_uint64_t lo);

EM_uint128_t
fp82_fp_U128_lsh(EM_uint128_t value, EM_uint_t count);

EM_uint128_t
fp82_fp_U128_rsh(EM_uint128_t value, EM_uint_t count);
 
EM_uint128_t
fp82_fp_U64_x_U64_to_U128(EM_uint64_t value1, EM_uint64_t value2);
 
INLINE EM_uint128_t
fp_I64_x_I64_to_I128(EM_uint64_t value1, EM_uint64_t value2);

EM_uint128_t
fp82_fp_U128_inc(EM_uint128_t value1);

static INLINE EM_uint128_t
fp_U128_neg(EM_uint128_t value);

EM_uint128_t
fp82_fp_U128_add(EM_uint128_t value1, EM_uint128_t value2);

EM_uint128_t
fp82_fp_U128_bor(EM_uint128_t value1, EM_uint128_t value2);

EM_uint128_t
fp82_fp_U128_band(EM_uint128_t value1, EM_uint128_t value2);

/* 256-bit unsigned int support routines */

EM_boolean_t
fp82_fp_U256_eq(EM_uint256_t value1, EM_uint256_t value2);

EM_boolean_t
fp82_ne_U256(EM_uint256_t value1, EM_uint256_t value2);

static INLINE EM_uint256_t
bld_U256(
   EM_uint64_t hh, EM_uint64_t hl, 
   EM_uint64_t lh, EM_uint64_t ll);

EM_uint256_t
fp82_fp_U256_lsh(EM_uint256_t value, EM_uint_t count);

EM_uint256_t
fp82_fp_U256_rsh(EM_uint256_t value, EM_uint_t count);

EM_uint256_t
fp82_fp_U256_inc(EM_uint256_t value1);

static INLINE EM_uint256_t
fp_U256_neg(EM_uint256_t value);

static INLINE EM_uint256_t
fp_U256_add(EM_uint256_t value1,
            EM_uint256_t value2);

/* Basic Conversion Routines */

INLINE EM_uint128_t
fp_U64_to_U128(EM_uint64_t value);

INLINE EM_uint64_t
fp_U128_to_U64(EM_uint128_t value);

static INLINE EM_uint256_t
fp_U64_to_U256(EM_uint64_t value);

static INLINE EM_uint64_t
fp_U256_to_U64(EM_uint256_t value);

EM_uint256_t
fp82_fp_U128_to_U256(EM_uint128_t value);

static INLINE EM_uint128_t
fp_U256_to_U128(EM_uint256_t value);

EM_boolean_t
is_reserved_field(
   EM_opcode_type    calling_instruction,
   EM_opcode_sf_type sf,
   EM_uint_t         val) ;



/* Basic Constants */

static const EM_uint64_t  U64_0                  = CONST_FORMAT( 0x0000000000000000 );
static const EM_uint64_t  U64_1                  = CONST_FORMAT( 0x0000000000000001 );
static const EM_uint64_t  U64_0x0000000080000000 = CONST_FORMAT( 0x0000000080000000 );
static const EM_uint64_t  U64_0x2000000000000000 = CONST_FORMAT( 0x2000000000000000 );
static const EM_uint64_t  U64_0x4000000000000000 = CONST_FORMAT( 0x4000000000000000 );
static const EM_uint64_t  U64_0x8000000000000000 = CONST_FORMAT( 0x8000000000000000 );
static const EM_uint64_t  U64_0xC000000000000000 = CONST_FORMAT( 0xC000000000000000 );
static const EM_uint64_t  U64_0x3FFFFFFFFFFFFFFF = CONST_FORMAT( 0x3FFFFFFFFFFFFFFF );
static const EM_uint64_t  U64_0x7FFFFFFFFFFFFFFF = CONST_FORMAT( 0x7FFFFFFFFFFFFFFF );
static const EM_uint64_t  U64_0xFFFFFF0000000000 = CONST_FORMAT( 0xFFFFFF0000000000 );
static const EM_uint64_t  U64_0xFFFFFFFFFFFFF800 = CONST_FORMAT( 0xFFFFFFFFFFFFF800 );
static const EM_uint64_t  U64_0xFFFFFFFFFFFFFFFF = CONST_FORMAT( 0xFFFFFFFFFFFFFFFF );
static const EM_uint64_t  U64_0xFFFFFFFF00000000 = CONST_FORMAT( 0xFFFFFFFF00000000 );
static const EM_uint64_t  U64_0xFFFF000000000000 = CONST_FORMAT( 0xFFFF000000000000 );
static const EM_uint64_t  U64_0xFF00000000000000 = CONST_FORMAT( 0xFF00000000000000 );


static const EM_uint128_t U128_0
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
                    };



static const EM_uint128_t U128_0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF 
                    = {
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF )
                    };


static const EM_uint128_t U128_0x00000000000000000000000000003FFF 
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000003FFF ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000003FFF ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
                    };


static const EM_uint128_t U128_0x00000000000000000000000000007FFF 
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000007FFF ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000007FFF ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
                    };


static const EM_uint128_t U128_0x00000000000000000000000000004000 
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000004000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000004000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
                    };


static const EM_uint128_t U128_0x00000000000000000000000000008000 
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000008000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000008000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
                    };


static const EM_uint128_t U128_0xFFFFFFFFFFFFFFFFFFFFFFFFFFFF8000 
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0x0000000000008000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000008000 ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
#endif
                    };


static const EM_uint128_t U128_0x40000000000000000000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x4000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x4000000000000000 ),
#endif
                    };


static const EM_uint128_t U128_0x00000100000000000000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000010000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000010000000000 ),
#endif
                    };


static const EM_uint128_t U128_0x00000080000000000000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000008000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000008000000000 ),
#endif
                    };


static const EM_uint128_t U128_0x00000000000008000000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000800 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000000800 ),
#endif
                    };


static const EM_uint128_t U128_0x00000000000004000000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000400 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000000400 ),
#endif
                    };


static const EM_uint128_t U128_0x00000000000000010000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000001 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000000001 ),
#endif
                    };


static const EM_uint128_t U128_0x00000000000000008000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x8000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x8000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
                    };


static const EM_uint128_t U128_0x00000000000000007FFFFFFFFFFFFFFF
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x7FFFFFFFFFFFFFFF ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x7FFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
                    };


static const EM_uint128_t U128_0x0000000000000000FFFFFFFFFFFFFFFF
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
                    };


static const EM_uint128_t U128_0xFFFFFFFFFFFFFFFF0000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
#endif
                    };


static const EM_uint128_t U128_0xFFFFFFFFFFFFFFFF8000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0x8000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x8000000000000000 ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
#endif
                    };


static const EM_uint128_t U128_0x00000000000003FFFFFFFFFFFFFFFFFF
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x00000000000003FF ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0x00000000000003FF ),
#endif
                    };


static const EM_uint128_t U128_0x00000000000007FFFFFFFFFFFFFFFFFF
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x00000000000007FF ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0x00000000000007FF ),
#endif
                    };


static const EM_uint128_t U128_0xFFFFFFFFFFFFF8000000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFF800 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFF800 ),
#endif
                    };


static const EM_uint128_t U128_0xFFFFFFFFFFFFFC000000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFFC00 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFC00 ),
#endif
                    };


static const EM_uint128_t U128_0x0000007FFFFFFFFFFFFFFFFFFFFFFFFF
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000007FFFFFFFFF ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0x0000007FFFFFFFFF ),
#endif
                    };


static const EM_uint128_t U128_0x000000FFFFFFFFFFFFFFFFFFFFFFFFFF
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x000000FFFFFFFFFF ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0x000000FFFFFFFFFF ),
#endif
                    };


static const EM_uint128_t U128_0xFFFFFF00000000000000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0xFFFFFF0000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0xFFFFFF0000000000 ),
#endif
                    };


static const EM_uint128_t U128_0xFFFFFF80000000000000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0xFFFFFF8000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0xFFFFFF8000000000 ),
#endif
                    };


static const EM_uint128_t U128_0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x7FFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0x7FFFFFFFFFFFFFFF ),
#endif
                    };


static const EM_uint128_t U128_0xC0000000000000000000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0xC000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0xC000000000000000 ),
#endif
                    };


static const EM_uint128_t U128_0x3FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x3FFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0xFFFFFFFFFFFFFFFF ),
                        CONST_FORMAT( 0x3FFFFFFFFFFFFFFF ),
#endif
                    };


static const EM_uint128_t U128_0x80000000000000000000000000000000
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x8000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x8000000000000000 ),
#endif
                    };


static const EM_uint256_t U256_0
                    = {
#ifdef BIG_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
#ifdef LITTLE_ENDIAN
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
                        CONST_FORMAT( 0x0000000000000000 ),
#endif
                    };


/* Floating Sign Constants */
 
static const EM_uint_t FP_SIGN_POSITIVE = 0;
 
static const EM_uint_t FP_SIGN_NEGATIVE = 1;

/* Floating Exponent Constants */
 
static const EM_uint_t FP_SGL_BIAS       = 0x0007F;
static const EM_uint_t FP_DBL_BIAS       = 0x003FF;
static const EM_uint_t FP_EXT_BIAS       = 0x03FFF;
static const EM_uint_t FP_REG_BIAS       = 0x0FFFF;
static const EM_uint_t FP_DP_BIAS        = 0x3FFFF;
 
static const EM_uint_t FP_SGL_EXP_ONES   = 0x000FF;
static const EM_uint_t FP_DBL_EXP_ONES   = 0x007FF;
static const EM_uint_t FP_EXT_EXP_ONES   = 0x07FFF;
static const EM_uint_t FP_REG_EXP_ONES   = 0x1FFFF;
static const EM_uint_t FP_DP_EXP_ONES    = 0x7FFFF;

static const EM_uint_t FP_SGL_EXP_WIDTH  = 8;
static const EM_uint_t FP_DBL_EXP_WIDTH  = 11;
static const EM_uint_t FP_EXT_EXP_WIDTH  = 15;
static const EM_uint_t FP_REG_EXP_WIDTH  = 17;

static const EM_uint_t FP_SGL_SIGNIFICAND_WIDTH  = 23;
static const EM_uint_t FP_DBL_SIGNIFICAND_WIDTH  = 52;
static const EM_uint_t FP_EXT_SIGNIFICAND_WIDTH  = 64;
static const EM_uint_t FP_REG_SIGNIFICAND_WIDTH  = 64;

static const EM_uint_t FP_REG_EXP_HALF   = 0x0FFFE;
 
static const EM_uint_t FP_INTEGER_EXP    = 0x1003E;
 
static const EM_uint_t FP_DP_INTEGER_EXP = 0x4003E;

/* Floating Constants */
 
static const EM_fp_reg_type
    FP_ZERO            = {CONST_FORMAT( 0x0000000000000000 ), 0x00000, 0x0 }; /* 0.0 */

static const EM_fp_reg_type
    FP_NEG_ZERO            = {CONST_FORMAT( 0x0000000000000000 ), 0x00000, 0x1 }; /* -0.0 */

static const EM_fp_reg_type
    FP_HALF            = {CONST_FORMAT( 0x8000000000000000 ), 0x0FFFE, 0x0 }; /* 0.5 */
 
static const EM_fp_reg_type
    FP_ONE             = {CONST_FORMAT( 0x8000000000000000 ), 0x0FFFF, 0x0 }; /* 1.0 */

static const EM_fp_reg_type
    FP_ONE_PAIR        = {CONST_FORMAT( 0x3F8000003F800000 ), 0x1003E, 0x0 };
/* Pair of ones for SIMD non-memory ops */

static const EM_fp_reg_type
    FP_INFINITY        = {CONST_FORMAT( 0x8000000000000000 ), 0x1FFFF, 0x0 }; /* Inf */
 
static const EM_fp_reg_type
    FP_QNAN            = {CONST_FORMAT( 0xC000000000000000 ), 0x1FFFF, 0x1 }; /* QNaN Indefinite */
 
static const EM_fp_reg_type
    FP_SNAN            = {CONST_FORMAT( 0x8000000000000000 ), 0x1FFFF, 0x0 }; /* SNaN*/
 
static const EM_fp_reg_type
    FP_POS_2_TO_63     = {CONST_FORMAT( 0x8000000000000000 ), 0x1003E, 0x0 }; /* 2.0**63 */
 
static const EM_fp_reg_type
    FP_NEG_2_TO_63     = {CONST_FORMAT( 0x8000000000000000 ), 0x1003E, 0x1 }; /* -2.0**63 */
 
static const EM_fp_reg_type
    FP_POS_2_TO_64     = {CONST_FORMAT( 0x8000000000000000 ), 0x1003F, 0x0 }; /* 2.0**64 */
 
static const EM_fp_reg_type
    FP_NEG_2_TO_64     = {CONST_FORMAT( 0x8000000000000000 ), 0x1003F, 0x1 }; /* -2.0**64 */
 
static const EM_fp_reg_type
    FP_POS_2_TO_31     = {CONST_FORMAT( 0x0000000080000000 ), 0x1003E, 0x0 }; /* 2.0**31 */
 
static const EM_fp_reg_type
    FP_NEG_2_TO_31     = {CONST_FORMAT( 0x0000000080000000 ), 0x1003E, 0x1 }; /* -2.0**31 */
 
static const EM_fp_reg_type
    FP_POS_2_TO_32     = {CONST_FORMAT( 0x0000000080000000 ), 0x1003F, 0x0 }; /* 2.0**32 */
 
static const EM_fp_reg_type
    FP_NEQ_2_TO_32     = {CONST_FORMAT( 0x0000000080000000 ), 0x1003F, 0x1 }; /* -2.0**32 */
 
static const EM_fp_reg_type
    NATVAL = {CONST_FORMAT( 0x0000000000000000 ), 0x1FFFE, 0x0 }; /* NaTVal */

static const EM_uint64_t
   INTEGER_INDEFINITE = CONST_FORMAT( 0x8000000000000000 ); /* -(2**63) */

static const EM_uint_t
   INTEGER_INDEFINITE_32_BIT = 0x80000000; /* */








/******************************************************************************/
/* Define macros to simplify access to the fp82_ functions.  This is done so  */
/*  the namespace doesn't get cluttered, while retaining convenient access.   */
/*  The FP82_NO_SHORTCUTS macro can be defined to prevent creation of these.  */
/******************************************************************************/

#ifndef FP82_NO_SHORTCUTS
#define fp_U64_x_U64_to_U128         fp82_fp_U64_x_U64_to_U128
#define fp_U128_add                  fp82_fp_U128_add 
#define fp_U128_band                 fp82_fp_U128_band
#define fp_U128_bor                  fp82_fp_U128_bor
#define fp_U128_eq                   fp82_fp_U128_eq
#define fp_U256_eq                   fp82_fp_U256_eq
#define fp_U128_to_U256              fp82_fp_U128_to_U256
#define fp_U128_lt                   fp82_fp_U128_lt
#define fp_U128_inc                  fp82_fp_U128_inc
#define fp_U256_inc                  fp82_fp_U256_inc
#define fp_U128_rsh                  fp82_fp_U128_rsh
#define fp_U128_lsh                  fp82_fp_U128_lsh
#define fp_U256_rsh                  fp82_fp_U256_rsh
#define fp_U256_lsh                  fp82_fp_U256_lsh
#define fp_U256_rsh                  fp82_fp_U256_rsh
#define fp_U64_lead0                 fp82_fp_U64_lead0
#define fp_U256_lead0                fp82_fp_U256_lead0
#define fp_raise_fault               fp82_fp_raise_fault
#define fp_raise_traps               fp82_fp_raise_traps
#define fp_decode_fault              fp82_fp_decode_fault
#define fp_decode_trap               fp82_fp_decode_trap
#define fp_fr_to_dp                  fp82_fp_fr_to_dp
#endif /* FP82_NO_SHORTCUTS */

#endif /* _EM_HELPER_H */

