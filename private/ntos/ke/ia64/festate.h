/******************************
Intel Confidential
******************************/

#ifndef _EM_STATE_H
#define _EM_STATE_H

/*****************************/
/* Processor's State */
/*****************************/

#ifndef FPSR
#define FPSR              ps->state_AR[ar_fpsr].fpsr
#endif

#ifndef FPSR_value
#define FPSR_value        ps->state_AR[ar_fpsr].uint_value
#endif

#ifndef PSR
#define PSR               ps->state_PSR
#endif

#ifndef IP
#define IP                ps->state_IP
#endif

#ifndef AR
#define AR                ps->state_AR
#endif

#ifndef FR
#define FR                ps->state_FR
#endif

#ifndef GR
#define GR                ps->state_GR
#endif

#ifndef PR
#define PR                ps->state_PR
#endif

#ifndef MEM
#define MEM               ps->state_MEM
#endif

#ifndef UM
#define UM                ps->state_PSR
#endif

#define FPSR_traps        (EM_uint_t)((ps->state_AR[ar_fpsr].uint_value>> 0)&0x3F)

#define FPSR_sf0_flags    (EM_uint_t)((ps->state_AR[ar_fpsr].uint_value>>13)&0x3F)
#define FPSR_sf1_flags    (EM_uint_t)((ps->state_AR[ar_fpsr].uint_value>>26)&0x3F)
#define FPSR_sf2_flags    (EM_uint_t)((ps->state_AR[ar_fpsr].uint_value>>39)&0x3F)
#define FPSR_sf3_flags    (EM_uint_t)((ps->state_AR[ar_fpsr].uint_value>>52)&0x3F)

#define FPSR_sf0_controls (EM_uint_t)((ps->state_AR[ar_fpsr].uint_value>> 6)&0x7F)
#define FPSR_sf1_controls (EM_uint_t)((ps->state_AR[ar_fpsr].uint_value>>19)&0x7F)
#define FPSR_sf2_controls (EM_uint_t)((ps->state_AR[ar_fpsr].uint_value>>32)&0x7F)
#define FPSR_sf3_controls (EM_uint_t)((ps->state_AR[ar_fpsr].uint_value>>45)&0x7F)

#define GENERAL_FORM           ps->state_form.general_form
#define DATA_FORM              ps->state_form.data_form
#define CONTROL_FORM           ps->state_form.control_form
#define FP82_FLOATING_FORM     ps->state_form.fp82_floating_form
#define CLEAR_FORM             ps->state_form.clear_form
#define NO_CLEAR_FORM          ps->state_form.no_clear_form

#define HIGH_UNSIGNED_FORM     ps->state_form.high_unsigned_form    
#define LOW_FORM               ps->state_form.low_form    
#define HIGH_FORM              ps->state_form.high_form    
#define NEG_SIGN_FORM          ps->state_form.neg_sign_form
#define SIGN_FORM              ps->state_form.sign_form
#define SIGN_EXP_FORM          ps->state_form.sign_exp_form
#define MIX_L_FORM             ps->state_form.mix_l_form
#define MIX_R_FORM             ps->state_form.mix_r_form
#define MIX_LR_FORM            ps->state_form.mix_lr_form
#define SXT_L_FORM             ps->state_form.sxt_l_form
#define SXT_R_FORM             ps->state_form.sxt_r_form
#define PACK_FORM              ps->state_form.pack_form
#define SWAP_FORM              ps->state_form.swap_form
#define SWAP_NL_FORM           ps->state_form.swap_nl_form
#define SWAP_NR_FORM           ps->state_form.swap_nr_form
#define SIGNED_FORM            ps->state_form.signed_form
#define TRUNC_FORM             ps->state_form.trunc_form
#define UNSIGNED_FORM          ps->state_form.unsigned_form

#define SINGLE_FORM            ps->state_form.single_form
#define DOUBLE_FORM            ps->state_form.double_form
#define EXPONENT_FORM          ps->state_form.exponent_form
#define SIGNIFICAND_FORM       ps->state_form.significand_form

#define NO_BASE_UPDATE_FORM                ps->state_form.no_base_update_form
#define REGISTER_BASE_UPDATE_FORM          ps->state_form.register_base_update_form
#define IMMEDIATE_BASE_UPDATE_FORM         ps->state_form.immediate_base_update_form

#define FCHECK_BRANCH_IMPLEMENTED ps->state_form.fcheck_branch_implemented

/*
The following concern software assistance. For an implementation-determined
software assistance fault or trap to occur (when encountering
denormals MERCED_RTL must be 1. An architecturally-mandated swa
can still occur when MERCED_RTL is 0. Such an assist occurs in frcpa and
fprcpa.
*/

#define MERCED_RTL ps->state_MERCED_RTL

#endif /* _EM_STATE_H */

