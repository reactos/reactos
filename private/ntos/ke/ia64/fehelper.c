/******************************
Intel Confidential
******************************/

// MACH
#include "ki.h"

#ifndef WIN32_OR_WIN64
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#endif

#include "fepublic.h"
#include "fehelper.h"
#include "festate.h"

// MACH #ifdef WIN32_OR_WIN64
// MACH #include <process.h>
// MACH #endif

// *************************************************************
// The functions fp_reg_read_hi() and fp_reg_read_lo()
// Take a packed floating-point register, 
// Reads the hi (or lo) part
// Returns a register-biased number with implicit made explicit.
// *************************************************************
EM_uint64_t
fp_concatenate(EM_uint_t hi_val, EM_uint_t lo_val) {
   return(  ((EM_uint64_t)hi_val <<32)
          | ((EM_uint64_t)lo_val & CONST_FORMAT(0x00000000FFFFFFFF )) );
}


EM_uint_t 
fp_extract_bits(
   EM_uint64_t  input_value, 
   unsigned int hi_bound, 
   unsigned int lo_bound)
{
EM_uint64_t value;
   if(lo_bound >63) return(0x00000000);

   value = (input_value >> lo_bound) &
           ~(CONST_FORMAT(0xFFFFFFFFFFFFFFFF) << (hi_bound - lo_bound + 1));

   return((EM_uint_t)value);
}


INLINE EM_fp_reg_type
fp_reg_read_hi(EM_uint_t freg)
{
EM_fp_reg_type tmp_freg = FR[freg];
EM_memory_type mem;

    if (freg == 0)
        return (FP_ZERO);
    else if (freg == 1) {
        return (FP_NEG_ZERO);
    }
    else {
        mem.uint_32.uvalue = (EM_uint_t)(tmp_freg.significand >> 32);
        tmp_freg = fp_mem_to_fr_format(mem, 4, 0);
        return (tmp_freg);
    }
}

INLINE EM_fp_reg_type
fp_reg_read_lo(EM_uint_t freg)
{
EM_fp_reg_type tmp_freg = FR[freg];
EM_memory_type mem;
EM_uint64_t tmp_val;
EM_uint_t   tmp32_val;

    if (freg == 0)
        return (FP_ZERO);
    else if (freg == 1) {
        return (FP_ZERO);
    }
    else {
        tmp_val = (tmp_freg.significand & CONST_FORMAT(0x00000000ffffffff));
        tmp32_val = (EM_uint_t)tmp_val;
        mem.uint_32.uvalue = tmp32_val;
        tmp_freg = fp_mem_to_fr_format(mem, 4, 0);
        return (tmp_freg);
    }
}


#undef fp_reg_read_hi
#undef fp_reg_read_lo

#define fp_reg_read_hi(f2)            fp82_reg_read_hi(ps,f2)
#define fp_reg_read_lo(f3)            fp82_reg_read_lo(ps,f3)


// ***********************************************************
// fp_fr_to_mem_format()
// ************************************************************
EM_memory_type
fp_fr_to_mem_format(
    EM_fp_reg_type freg,
    EM_uint_t      size,
    EM_uint_t      integer_form)
{
EM_memory_type tmp_mem;
EM_uint64_t    tmp_significand;

    switch(size) {
        case 4:
            tmp_mem.fp_single.sign = freg.sign;
            if (freg.exponent == 0)
                tmp_mem.fp_single.exponent = 0;
            else if ((freg.significand>>63) == 0)
                tmp_mem.fp_single.exponent = 0;
            else if (freg.exponent == FP_REG_EXP_ONES)
                tmp_mem.fp_single.exponent = FP_SGL_EXP_ONES;
            else 
                tmp_mem.fp_single.exponent    = 
                                         ((freg.exponent 
                                           >> (FP_REG_EXP_WIDTH-1) & 1) 
                                           << (FP_SGL_EXP_WIDTH-1))
                                        |( freg.exponent & FP_SGL_BIAS);

            tmp_mem.fp_single.significand =
                    (EM_uint_t)((freg.significand <<1) >>(63-22));
            break;

        case 8: /* double */
            if (integer_form)
                tmp_mem.uint_64.uvalue = freg.significand;

            else { /* !integer_form */
                tmp_mem.fp_double.sign = freg.sign;
                if (freg.exponent == 0)
                    tmp_mem.fp_double.exponent = 0;
                else if ((freg.significand>>63) == 0)
                    tmp_mem.fp_double.exponent = 0;
                else if (freg.exponent == FP_REG_EXP_ONES)
                    tmp_mem.fp_double.exponent = FP_DBL_EXP_ONES;
                else
                    tmp_mem.fp_double.exponent    = 
                                         ((freg.exponent 
                                           >> (FP_REG_EXP_WIDTH-1) & 1) 
                                           << (FP_DBL_EXP_WIDTH-1))
                                        |( freg.exponent & FP_DBL_BIAS);

                tmp_significand = 
                        (freg.significand <<1) >> (63-51);

                tmp_mem.fp_double.significand_hi = 
                  (EM_uint_t)((tmp_significand >> 32) & CONST_FORMAT( 0x00000000ffffffff));
                tmp_mem.fp_double.significand_lo = 
                  (EM_uint_t)(tmp_significand & CONST_FORMAT( 0x00000000ffffffff));
            }    
            break;

        case 10: /* double extended */
          tmp_mem.fp_double_extended.sign = freg.sign;
            if (freg.exponent == 0) {
                    /* Zero or (Pseudo-)Denormal */
                tmp_mem.fp_double_extended.exponent = 0;
            } else if (freg.exponent == FP_REG_EXP_ONES) {
                    /* Inf/NaN/NatVAL */
                tmp_mem.fp_double_extended.exponent = FP_EXT_EXP_ONES;
            } else {
                    /* Normal or Unnormal */
                    tmp_mem.fp_double_extended.exponent    = 
                                         ((freg.exponent 
                                           >> (FP_REG_EXP_WIDTH-1) & 1) 
                                           << (FP_EXT_EXP_WIDTH-1))
                                        |( freg.exponent & FP_EXT_BIAS);

            }
	    memcpy(tmp_mem.fp_double_extended.significand,
		    &freg.significand, 8);

            break;

        case 16: /* spill */
            tmp_mem.fp_spill_fill.reserved1    = 0;
            tmp_mem.fp_spill_fill.reserved2    = 0;
            tmp_mem.fp_spill_fill.sign         = freg.sign;
            tmp_mem.fp_spill_fill.exponent     = freg.exponent;
            tmp_mem.fp_spill_fill.significand  = freg.significand;
            break;
    }
    return (tmp_mem);
}


INLINE EM_memory_type
fr_to_mem4_bias_adjust(EM_fp_reg_type freg)
{
EM_memory_type tmp_mem;

   tmp_mem.fp_single.sign = freg.sign;
   if (freg.exponent == 0)
      tmp_mem.fp_single.exponent = 0;
   else if (freg.exponent == FP_REG_EXP_ONES)
      tmp_mem.fp_single.exponent = FP_SGL_EXP_ONES;
   else if ((freg.significand>>63) == 0)
      tmp_mem.fp_single.exponent = (EM_uint_t)
                                 (((EM_int_t)freg.exponent)
                                 - FP_REG_BIAS + FP_SGL_BIAS - 1);
   else
      tmp_mem.fp_single.exponent = (EM_uint_t)
                                 (((EM_int_t)freg.exponent)
                                 - FP_REG_BIAS + FP_SGL_BIAS);
      tmp_mem.fp_single.significand = (EM_uint_t) (
                    (freg.significand<<(64-62-1))>>(40+64-62-1));

    return (tmp_mem);
}




// *****************************************************
// helper functions definitions 
// *****************************************************

INLINE EM_boolean_t
fp_equal(EM_fp_reg_type fr1, EM_fp_reg_type fr2)
{
    EM_fp_dp_type fp_dp1;
    EM_fp_dp_type fp_dp2;

    if (   fp_is_nan(fr1)         || fp_is_nan(fr2)
        || fp_is_unsupported(fr1) || fp_is_unsupported(fr2) )
        return (0);

    fp_dp1 = fp_fr_to_dp(fr1);
    fp_dp2 = fp_fr_to_dp(fr2);

    if (   (fp_dp1.sign           == fp_dp2.sign          )
        && (fp_dp1.exponent       == fp_dp2.exponent      )
        && (fp_dp1.significand.hi == fp_dp2.significand.hi)
        && (fp_dp1.significand.lo == fp_dp2.significand.lo) )
        return (1);
    else if ( fp_is_zero_dp(fp_dp1) && fp_is_zero_dp(fp_dp2) )
        return (1);
    else
        return (0);
}


INLINE EM_boolean_t
fp_less_than(
   EM_fp_reg_type fr1, 
   EM_fp_reg_type fr2) 
{
EM_fp_dp_type fp_dp1;
EM_fp_dp_type fp_dp2;

    if (   fp_is_nan(fr1)         || fp_is_nan(fr2)
        || fp_is_unsupported(fr1) || fp_is_unsupported(fr2) )
        return (0);

    fp_dp1 = fp_fr_to_dp(fr1);
    fp_dp2 = fp_fr_to_dp(fr2);

    if (fp_is_neg_dp(fp_dp1) && fp_is_pos_dp(fp_dp2)) {
        if (!fp_is_zero_dp(fp_dp1) || !fp_is_zero_dp(fp_dp2) )
            return (1);             /* for non-zero's neg is lt pos */
        else
            return (0);             /* zeros are equal */
    } else if (fp_is_pos_dp(fp_dp1) && fp_is_neg_dp(fp_dp2)) {
        return (0);                 /* pos is not lt neg */
    } else if (fp_is_neg_dp(fp_dp1) && fp_is_neg_dp(fp_dp2)) {
        if (fp_dp1.exponent > fp_dp2.exponent)
            return (1);             /* fp_dp1 much less than fp_dp2 */
        else if ((fp_dp1.exponent == fp_dp2.exponent)
                && (fp_U128_gt(fp_dp1.significand, fp_dp2.significand)))
            return (1);             /* fp_dp1 just less than fp_dp2 */
        else
            return (0);
    } else if (fp_is_pos_dp(fp_dp1) && fp_is_pos_dp(fp_dp2)) {
        if (fp_dp1.exponent < fp_dp2.exponent)
                return (1);         /* fp_dp1 much less than fp_dp2 */
        else if ((fp_dp1.exponent == fp_dp2.exponent)
                && (fp_U128_lt(fp_dp1.significand, fp_dp2.significand)))
            return (1);             /* fp_dp1 just less than fp_dp2 */
        else
            return (0);
    } else {
      return (0); // MACH ADDED
    }
}

INLINE EM_boolean_t
fp_lesser_or_equal(EM_fp_reg_type fr1, EM_fp_reg_type fr2)
{
    EM_fp_dp_type fp_dp1;
    EM_fp_dp_type fp_dp2;

    if (   fp_is_nan(fr1)         || fp_is_nan(fr2)
        || fp_is_unsupported(fr1) || fp_is_unsupported(fr2) )
        return (0);

    fp_dp1 = fp_fr_to_dp(fr1);
    fp_dp2 = fp_fr_to_dp(fr2);

    if (fp_is_neg_dp(fp_dp1) && fp_is_pos_dp(fp_dp2)) {
        return (1);              /* for non-zero's and zeros's neg is le pos */
    } else if (fp_is_pos_dp(fp_dp1) && fp_is_neg_dp(fp_dp2)) {
        if (fp_is_zero_dp(fp_dp1) && fp_is_zero_dp(fp_dp2))
            return (1);          /* zero's are le */
        else
            return (0);          /* pos is not lt neg */
    } else if (fp_is_neg_dp(fp_dp1) && fp_is_neg_dp(fp_dp2)) {
        if (fp_dp1.exponent > fp_dp2.exponent)
            return (1);          /* fp_dp1 much less than fp_dp2 */
        else if ((fp_dp1.exponent == fp_dp2.exponent)
                && (fp_U128_ge(fp_dp1.significand, fp_dp2.significand)))
            return (1);          /* fp_dp1 just less than or equal fp_dp2 */
        else
            return (0);
    } else if (fp_is_pos_dp(fp_dp1) && fp_is_pos_dp(fp_dp2)) {
        if (fp_dp1.exponent < fp_dp2.exponent)
                return (1);      /* fp_dp1 much less than fp_dp2 */
        else if ((fp_dp1.exponent == fp_dp2.exponent)
                && (fp_U128_le(fp_dp1.significand, fp_dp2.significand)))
            return (1);          /* fp_dp1 just less than or equal fp_dp2 */
        else
            return (0);
    } else {
      return (0); // MACH ADDED
    }
}

INLINE EM_boolean_t
fp_unordered(EM_fp_reg_type fr1, EM_fp_reg_type fr2)
{
    if (   fp_is_nan(fr1)         || fp_is_nan(fr2)
        || fp_is_unsupported(fr1) || fp_is_unsupported(fr2) )
        return (1);
    else 
        return (0);

}

EM_uint_t
fp82_fp_decode_fault(EM_tmp_fp_env_type tmp_fp_env)
{
EM_uint_t tmp_ret = 0;

    if (!tmp_fp_env.simd) { // MACH ADDED

      if (tmp_fp_env.em_faults.swa)
          return (8);
      else if (tmp_fp_env.em_faults.v)
          return (1);
      else if (tmp_fp_env.em_faults.z)
          return (4);
      else if (tmp_fp_env.em_faults.d)
          return (2);
      else {
          tmp_ret = 0;
          return (0); // MACH ADDED
      }

   } else {

// ****************************************************
// hi_faults are recorded in the low  four bits of temp_ret.
// lo_faults are recorded in the high four bits of temp_ret.
// ****************************************************

        if (tmp_fp_env.hi_faults.swa)
            tmp_ret = 8;
        else if (tmp_fp_env.hi_faults.v)
            tmp_ret = 1;
        else if (tmp_fp_env.hi_faults.z)
            tmp_ret = 4;
        else if (tmp_fp_env.hi_faults.d)
            tmp_ret = 2;

        if (tmp_fp_env.lo_faults.swa)
            tmp_ret |= 8<<4;
        else if (tmp_fp_env.lo_faults.v)
            tmp_ret |= 1<<4;
        else if (tmp_fp_env.lo_faults.z)
            tmp_ret |= 4<<4;
        else if (tmp_fp_env.lo_faults.d)
            tmp_ret |= 2<<4;
        return (tmp_ret);
    }
}


EM_uint_t
fp82_fp_decode_trap(EM_tmp_fp_env_type tmp_fp_env)
{
EM_uint_t tmp_ret;

    if (!tmp_fp_env.simd) {
        tmp_ret  = (tmp_fp_env.ebc       <<15
                  | tmp_fp_env.fpa       <<14
                  | tmp_fp_env.em_traps.i<<13
                  | tmp_fp_env.em_traps.un<<12
                  | tmp_fp_env.em_traps.o<<11 ); // MACH
    } 
    else {
       tmp_ret = 0;
       if(tmp_fp_env.hi_traps.i || 
          tmp_fp_env.hi_traps.un || 
          tmp_fp_env.hi_traps.o ) { // MACH
       
          tmp_ret  = tmp_fp_env.hi_fpa    <<14
                   | tmp_fp_env.hi_traps.i<<13
                   | tmp_fp_env.hi_flags.i<<13
                   | tmp_fp_env.hi_traps.un<<12
                   | tmp_fp_env.hi_traps.o<<11; // MACH
       }
       
       if(tmp_fp_env.lo_traps.i || 
          tmp_fp_env.lo_traps.un || 
          tmp_fp_env.lo_traps.o ) { // MACH

          tmp_ret |= tmp_fp_env.lo_fpa    <<10
                   | tmp_fp_env.lo_traps.i<<9
                   | tmp_fp_env.lo_flags.i<<9
                   | tmp_fp_env.lo_traps.un<<8
                   | tmp_fp_env.lo_traps.o<<7; // MACH
      }
    } 
    return (tmp_ret);
}


void
fp_decode_environment(
    EM_opcode_pc_type pc,
    EM_opcode_sf_type sf,
    EM_tmp_fp_env_type *tmp_fp_env)
{
EM_sf_type         tmp_sf;

    if (sf == sfS0) {
        tmp_sf.controls.ftz = FPSR.sf0_controls_ftz;
        tmp_sf.controls.wre = FPSR.sf0_controls_wre;
        tmp_sf.controls.pc  = FPSR.sf0_controls_pc;
        tmp_sf.controls.rc  = FPSR.sf0_controls_rc;
        tmp_sf.controls.td  = FPSR.sf0_controls_td;
    } else if (sf == sfS1) {
        tmp_sf.controls.ftz = FPSR.sf1_controls_ftz;
        tmp_sf.controls.wre = FPSR.sf1_controls_wre;
        tmp_sf.controls.pc  = FPSR.sf1_controls_pc;
        tmp_sf.controls.rc  = FPSR.sf1_controls_rc;
        tmp_sf.controls.td  = FPSR.sf1_controls_td;
    } else if (sf == sfS2) {
        tmp_sf.controls.ftz = FPSR.sf2_controls_ftz;
        tmp_sf.controls.wre = FPSR.sf2_controls_wre;
        tmp_sf.controls.pc  = FPSR.sf2_controls_pc;
        tmp_sf.controls.rc  = FPSR.sf2_controls_rc;
        tmp_sf.controls.td  = FPSR.sf2_controls_td;
    } else if (sf == sfS3) {
        tmp_sf.controls.ftz = FPSR.sf3_controls_ftz;
        tmp_sf.controls.wre = FPSR.sf3_controls_wre;
        tmp_sf.controls.pc  = FPSR.sf3_controls_pc;
        tmp_sf.controls.rc  = FPSR.sf3_controls_rc;
        tmp_sf.controls.td  = FPSR.sf3_controls_td;
    } else {
        tmp_sf.controls.ftz = 0;
        tmp_sf.controls.wre = 1;
        tmp_sf.controls.pc  = sf_double_extended;
        tmp_sf.controls.rc  = rc_rn;
        tmp_sf.controls.td  = 1;
    }

    if (sf == sf_none) {
        tmp_fp_env->controls.vd = 0;
        tmp_fp_env->controls.dd = 0;
        tmp_fp_env->controls.zd = 0;
        tmp_fp_env->controls.od = 0;
        tmp_fp_env->controls.ud = 0;
        tmp_fp_env->controls.id = 0;

    } else if (tmp_sf.controls.td )  {
        tmp_fp_env->controls.vd = 1;
        tmp_fp_env->controls.dd = 1;
        tmp_fp_env->controls.zd = 1;
        tmp_fp_env->controls.od = 1;
        tmp_fp_env->controls.ud = 1;
        tmp_fp_env->controls.id = 1;

    } else {
        tmp_fp_env->controls.vd = FPSR.traps_vd;
        tmp_fp_env->controls.dd = FPSR.traps_dd;
        tmp_fp_env->controls.zd = FPSR.traps_zd;
        tmp_fp_env->controls.od = FPSR.traps_od;
        tmp_fp_env->controls.ud = FPSR.traps_ud;
        tmp_fp_env->controls.id = FPSR.traps_id;
    }

    if (pc == pc_none) {
        tmp_fp_env->ss = ss_double_extended_64;
        tmp_fp_env->es = es_seventeen_bits;
        tmp_fp_env->simd = 0;

    } else if (pc == pc_simd) {
        tmp_fp_env->ss = ss_single_24;
        tmp_fp_env->es = es_eight_bits;
        tmp_fp_env->simd = 1;
        if (tmp_sf.controls.wre)
            tmp_sf.controls.wre = 0;
        tmp_fp_env->hi_flags.v = 0;
        tmp_fp_env->hi_flags.d = 0;
        tmp_fp_env->hi_flags.z = 0;
        tmp_fp_env->hi_flags.o = 0;
        tmp_fp_env->hi_flags.un = 0; 
        tmp_fp_env->hi_flags.i = 0;
        tmp_fp_env->lo_flags.v = 0;
        tmp_fp_env->lo_flags.d = 0;
        tmp_fp_env->lo_flags.z = 0;
        tmp_fp_env->lo_flags.o = 0;
        tmp_fp_env->lo_flags.un = 0; 
        tmp_fp_env->lo_flags.i = 0;

    } else if (pc == pc_s) {
        tmp_fp_env->ss = ss_single_24;
        tmp_fp_env->simd = 0;
        if (tmp_sf.controls.wre)
            tmp_fp_env->es = es_seventeen_bits;
        else
            tmp_fp_env->es = es_eight_bits;

    } else if (pc == pc_d) {
        tmp_fp_env->ss = ss_double_53;
        tmp_fp_env->simd = 0;
        if (tmp_sf.controls.wre)
            tmp_fp_env->es = es_seventeen_bits;
        else
            tmp_fp_env->es = es_eleven_bits;

    } else if (pc == pc_sf) {
        tmp_fp_env->simd = 0;
        if (tmp_sf.controls.pc == sf_single)
            tmp_fp_env->ss = ss_single_24;
        else if (tmp_sf.controls.pc == sf_double)
            tmp_fp_env->ss = ss_double_53;
        else if (tmp_sf.controls.pc == sf_double_extended)
            tmp_fp_env->ss = ss_double_extended_64;

        if (tmp_sf.controls.wre)
            tmp_fp_env->es = es_seventeen_bits;
        else
            tmp_fp_env->es = es_fifteen_bits;
    }

    if (sf == sf_none) {
        tmp_fp_env->rc  = rc_rz;
        tmp_fp_env->ftz = 0;
    } else {
        tmp_fp_env->rc  = tmp_sf.controls.rc;
        tmp_fp_env->ftz = tmp_sf.controls.ftz && tmp_fp_env->controls.ud;
    }

    tmp_fp_env->flags.v       = 0;
    tmp_fp_env->flags.d       = 0;
    tmp_fp_env->flags.z       = 0;
    tmp_fp_env->flags.o       = 0;
    tmp_fp_env->flags.un       = 0; 
    tmp_fp_env->flags.i       = 0;
    tmp_fp_env->ebc           = 0;
    tmp_fp_env->mdl           = 0;
    tmp_fp_env->mdh           = 0;

    tmp_fp_env->em_faults.v   = 0;
    tmp_fp_env->em_faults.d   = 0;
    tmp_fp_env->em_faults.z   = 0;
    tmp_fp_env->em_faults.swa = 0;
    tmp_fp_env->em_traps.i    = 0;
    tmp_fp_env->em_traps.o    = 0;
    tmp_fp_env->em_traps.un    = 0; 
    tmp_fp_env->fpa           = 0;

    tmp_fp_env->hi_faults.v   = 0;
    tmp_fp_env->hi_faults.d   = 0;
    tmp_fp_env->hi_faults.z   = 0;
    tmp_fp_env->hi_faults.swa = 0;
    tmp_fp_env->hi_traps.i    = 0;
    tmp_fp_env->hi_traps.o    = 0;
    tmp_fp_env->hi_traps.un    = 0; 
    tmp_fp_env->hi_fpa        = 0;

    tmp_fp_env->lo_faults.v   = 0;
    tmp_fp_env->lo_faults.d   = 0;
    tmp_fp_env->lo_faults.z   = 0;
    tmp_fp_env->lo_faults.swa = 0;
    tmp_fp_env->lo_traps.i    = 0;
    tmp_fp_env->lo_traps.o    = 0;
    tmp_fp_env->lo_traps.un    = 0; 
    tmp_fp_env->lo_fpa        = 0;

    return;
}

#undef  fp_decode_environment
#define fp_decode_environment(arg1, arg2, arg3) \
        fp82_fp_decode_environment(ps, arg1, arg2, arg3)
      


// ****************************************************
// Returns
//   1: if a specified register is <= disabled limit and dfl is 1
//   0: if a specified register is >  disabled limit and dfh is 1
// The disabled limit is 31 after ACR106.
// *****************************************************
EM_uint_t
fp_reg_disabled(
    EM_uint_t f1,
    EM_uint_t f2,
    EM_uint_t f3,
    EM_uint_t f4)
{
EM_uint_t tmp_ret;
EM_uint_t disabled_limit;

   tmp_ret=0;
   disabled_limit = 31;

    if (    ((f1 >= 2) && (f1 <=disabled_limit) && (PSR.dfl))
        ||  ((f2 >= 2) && (f2 <=disabled_limit) && (PSR.dfl))
        ||  ((f3 >= 2) && (f3 <=disabled_limit) && (PSR.dfl))
        ||  ((f4 >= 2) && (f4 <=disabled_limit) && (PSR.dfl))
       )
        tmp_ret |= (1<<0);
    if (    ((f1 > disabled_limit) && (f1 <= 127) && (PSR.dfh))
        ||  ((f2 > disabled_limit) && (f2 <= 127) && (PSR.dfh))
        ||  ((f3 > disabled_limit) && (f3 <= 127) && (PSR.dfh))
        ||  ((f4 > disabled_limit) && (f4 <= 127) && (PSR.dfh))
       )
        tmp_ret |= (1<<1);
    return(tmp_ret);
}

INLINE EM_boolean_t
fp_is_nan_or_inf(EM_fp_reg_type tmp_res)
{
    if (fp_is_nan(tmp_res) || fp_is_inf(tmp_res))
        return (1);
    else
        return (0);
}


INLINE EM_fp_reg_type
fp_dp_to_fr(EM_fp_dp_type tmp_res)
{
    EM_fp_reg_type tmp_ret;
    /* MACH FIX CMPLR BUG tmp_ret.sign = tmp_res.sign; */
    if (tmp_res.exponent == FP_DP_EXP_ONES)
        tmp_ret.exponent = FP_REG_EXP_ONES;
    else if (tmp_res.exponent == 0)
        tmp_ret.exponent = 0;
    else 
        tmp_ret.exponent = (EM_uint_t)(((EM_int_t)tmp_res.exponent)
                                   - FP_DP_BIAS + FP_REG_BIAS);
    tmp_ret.sign = tmp_res.sign; /* MACH FIX CMPLR BUG */
    tmp_ret.significand = tmp_res.significand.hi;
    return (tmp_ret);
}


// ***************************************************************
// fp_add()
// Adds a dp value to an freg value
// Returns a dp value
// ***************************************************************
INLINE EM_fp_dp_type
fp_add(EM_fp_dp_type fp_dp, EM_fp_reg_type fr2, EM_tmp_fp_env_type tmp_fp_env)
// fp_dp has been normalized and fr2 may not be normalized 
{
    EM_fp_dp_type tmp_res;
    EM_uint256_t tmp_a, tmp_b, tmp_c;
    EM_int_t exp_diff;
    EM_uint_t normalize_count;

//     all cases which might have faulted have been screened out 
//     we still may trap on overflow, underflow and/or inexact later 

    if (fp_is_zero_dp(fp_dp) && (fp_is_zero(fr2) || fp_is_pseudo_zero(fr2))) {
        /* correctly signed zero */
        tmp_res = fp_fr_to_dp(FP_ZERO);
        if (fp_dp.sign == fr2.sign) {
            tmp_res.sign = fr2.sign;
        } else if (tmp_fp_env.rc == rc_rm) {
            tmp_res.sign = 1;
        } else {
            tmp_res.sign = 0;
        }
        return(tmp_res);
    } else if (fp_is_inf_dp(fp_dp)) {
        /* correctly signed infinity */
        return(fp_dp);
    } else if (fp_is_inf(fr2)) {
        /* correctly signed infinity */
        return(fp_fr_to_dp(fr2));
    } else if( fp_is_zero_dp(fp_dp)) {
        return(fp_fr_to_dp(fr2));
    } else if( fp_is_zero(fr2) || fp_is_pseudo_zero(fr2) ) {
        return(fp_dp);
    } else {
        /* we have non-all-zeros and non-all-ones exponents in both operands */
        exp_diff = (((EM_int_t)fp_dp.exponent) - FP_DP_BIAS )
                 - (((EM_int_t)fr2.exponent)  -  FP_REG_BIAS);

        tmp_res.sign = fp_dp.sign;

        tmp_a = fp_U128_to_U256(fp_dp.significand);
        tmp_a = fp_U256_lsh(tmp_a,64);

        tmp_b = fp_U64_to_U256(fr2.significand);
        tmp_b = fp_U256_lsh(tmp_b,128);

        if (exp_diff >= 0) {
            tmp_res.exponent = fp_dp.exponent;

            tmp_c = fp_U256_rsh(tmp_b,exp_diff);
            tmp_res.sticky = !fp_U256_eq(tmp_b,fp_U256_lsh(tmp_c,exp_diff));
            tmp_b = tmp_c;

            if(fp_dp.sign != fr2.sign) {
                /* add sticky */
                if (tmp_res.sticky)
                    tmp_b = fp_U256_inc(tmp_b);
                if (fp_dp.sign)
                    tmp_a = fp_U256_neg(tmp_a);
                if (fr2.sign)
                    tmp_b = fp_U256_neg(tmp_b);
            }
        } else {
            tmp_res.exponent = fp_dp.exponent - exp_diff;

            tmp_c = fp_U256_rsh(tmp_a,-exp_diff);
            tmp_res.sticky = !fp_U256_eq(tmp_a,fp_U256_lsh(tmp_c,-exp_diff));
            tmp_a = tmp_c;

            if(fp_dp.sign != fr2.sign) {
                /* add sticky */
                if (tmp_res.sticky)
                    tmp_a = fp_U256_inc(tmp_a);
                if (fp_dp.sign)
                    tmp_a = fp_U256_neg(tmp_a);
                if (fr2.sign)
                    tmp_b = fp_U256_neg(tmp_b);
            }
        }

        tmp_c = fp_U256_add(tmp_a, tmp_b);


        if (fp_dp.sign != fr2.sign) {
            if (tmp_c.hh != 0) {
                tmp_res.sign = 1;
                tmp_c = fp_U256_neg(tmp_c);
            } else {
                tmp_res.sign = 0;
            }
        }

        if (!fp_U256_eq(tmp_c,U256_0)) {
            normalize_count = fp_U256_lead0(tmp_c);
            tmp_res.exponent -= (normalize_count - 64);
            tmp_res.significand = fp_U256_to_U128(
                                      fp_U256_rsh(
                                          fp_U256_lsh(tmp_c, normalize_count),
                                          128
                                      )
                                  );

            if(normalize_count > 128) {
               tmp_res.sticky |= !fp_U256_eq(
                                     fp_U256_rsh(
                                         fp_U128_to_U256(tmp_res.significand),
                                         normalize_count-128
                                     ),
                                     tmp_c
                                 );
            } else {
               tmp_res.sticky |= !fp_U256_eq(
                                     fp_U256_lsh(
                                         fp_U128_to_U256(tmp_res.significand),
                                         128-normalize_count
                                     ),
                                     tmp_c
                                 );
            }

        } else {
             if (fp_dp.sign == fr2.sign)
                 tmp_res.sign = fp_dp.sign;
             else if (tmp_fp_env.rc == rc_rm)
                 tmp_res.sign = 1;
             else
                 tmp_res.sign = 0;
             tmp_res.exponent = 0;
             tmp_res.significand = U128_0;
        }
        return(tmp_res);
    }
}



// *******************************************************************
// IEEE rounds
// *******************************************************************


INLINE EM_uint_t
fp_single(EM_fp_reg_type freg)
{
EM_memory_type tmp_mem;
    tmp_mem = fp_fr_to_mem_format(freg, 4, 0);
    return (tmp_mem.uint_32.uvalue);
}

INLINE void
fp_ieee_to_hilo(
    EM_simd_hilo hilo,
    EM_tmp_fp_env_type *tmp_fp_env)
{
    if(hilo == high) {
       tmp_fp_env->hi_flags.o = tmp_fp_env->flags.o;
       tmp_fp_env->flags.o    = 0;

       tmp_fp_env->hi_flags.un = tmp_fp_env->flags.un; // MACH
       tmp_fp_env->flags.un    = 0; // MACH

       tmp_fp_env->hi_flags.i = tmp_fp_env->flags.i;
       tmp_fp_env->flags.i    = 0;


       tmp_fp_env->hi_traps.o = tmp_fp_env->em_traps.o;
       tmp_fp_env->em_traps.o = 0;

       tmp_fp_env->hi_traps.un = tmp_fp_env->em_traps.un; // MACH
       tmp_fp_env->em_traps.un = 0; // MACH

       tmp_fp_env->hi_traps.i = tmp_fp_env->em_traps.i;
       tmp_fp_env->em_traps.i = 0;

       tmp_fp_env->hi_faults.d = tmp_fp_env->em_faults.d;
       tmp_fp_env->em_faults.d = 0;

       tmp_fp_env->hi_faults.z = tmp_fp_env->em_faults.z;
       tmp_fp_env->em_faults.z = 0;

       tmp_fp_env->hi_faults.v = tmp_fp_env->em_faults.v;
       tmp_fp_env->em_faults.v = 0;

       tmp_fp_env->hi_fpa = tmp_fp_env->fpa;
       tmp_fp_env->fpa = 0;

    } else {
       tmp_fp_env->lo_flags.o = tmp_fp_env->flags.o;
       tmp_fp_env->flags.o    = 0;

       tmp_fp_env->lo_flags.un = tmp_fp_env->flags.un; // MACH
       tmp_fp_env->flags.un    = 0; // MACH

       tmp_fp_env->lo_flags.i = tmp_fp_env->flags.i;
       tmp_fp_env->flags.i    = 0;


       tmp_fp_env->lo_traps.o = tmp_fp_env->em_traps.o;
       tmp_fp_env->em_traps.o = 0;

       tmp_fp_env->lo_traps.un = tmp_fp_env->em_traps.un; // MACH
       tmp_fp_env->em_traps.un = 0; // MACH

       tmp_fp_env->lo_traps.i = tmp_fp_env->em_traps.i;
       tmp_fp_env->em_traps.i = 0;

       tmp_fp_env->lo_faults.d = tmp_fp_env->em_faults.d;
       tmp_fp_env->em_faults.d = 0;

       tmp_fp_env->lo_faults.z = tmp_fp_env->em_faults.z;
       tmp_fp_env->em_faults.z = 0;

       tmp_fp_env->lo_faults.v = tmp_fp_env->em_faults.v;
       tmp_fp_env->em_faults.v = 0;

       tmp_fp_env->lo_fpa = tmp_fp_env->fpa;
       tmp_fp_env->fpa = 0;
    }
}

EM_fp_reg_type
fp_ieee_round(
    EM_fp_dp_type      fp_dp,
    EM_tmp_fp_env_type *tmp_fp_env)
{
    const EM_uint_t FPA[64] = {
                        0,0,0,1,
                        0,0,1,1,
                        0,0,0,1,
                        0,0,1,1, /* Nearest */
                        0,0,0,0,
                        0,0,0,0,
                        0,1,1,1,
                        0,1,1,1, /* -inf */
                        0,1,1,1,
                        0,1,1,1,
                        0,0,0,0,
                        0,0,0,0, /* +inf */
                        0,0,0,0,
                        0,0,0,0,
                        0,0,0,0,
                        0,0,0,0, /* Zero */
    };

    EM_fp_reg_type tmp_rtn;
    EM_fp_dp_type  tmp_res, tmp_unbounded_round;
    EM_int_t       cnt, tmp_shift;

    EM_uint_t      e_max, e_min, 
                   tmp_unbounded_ebc = 0, tmp_unbounded_fpa = 0;

    EM_uint128_t significand_mask;
    EM_uint128_t significand_even;
    EM_uint128_t significand_round;
    EM_uint128_t significand_not_mask;
    EM_uint128_t significand_sticky;


/************************************************
SETUP
set e_max, e_min
Note that the exponents are still dp-biased.
*************************************************/
    if (tmp_fp_env->es == es_eight_bits) {
        e_max = FP_DP_BIAS + FP_SGL_BIAS;
        e_min = FP_DP_BIAS - FP_SGL_BIAS + 1;
    } else if (tmp_fp_env->es == es_eleven_bits) {
        e_max = FP_DP_BIAS + FP_DBL_BIAS;
        e_min = FP_DP_BIAS - FP_DBL_BIAS + 1;
    } else if (tmp_fp_env->es == es_fifteen_bits) {
        e_max = FP_DP_BIAS + FP_EXT_BIAS;
        e_min = FP_DP_BIAS - FP_EXT_BIAS + 1;
    } else if (tmp_fp_env->es == es_seventeen_bits) {
        e_max = FP_DP_BIAS + FP_REG_BIAS;
        e_min = FP_DP_BIAS - FP_REG_BIAS + 1;
    }

/************************************************
SETUP
set significand_mask, significand_even, significand_round
    significand_not_mask, significand_sticky
*************************************************/
   if( tmp_fp_env->ss == ss_single_24) {
        significand_mask     = U128_0xFFFFFF00000000000000000000000000;
        significand_even     = U128_0x00000100000000000000000000000000;
        significand_round    = U128_0x00000080000000000000000000000000;
        significand_not_mask = U128_0x000000FFFFFFFFFFFFFFFFFFFFFFFFFF;
        significand_sticky   = U128_0x0000007FFFFFFFFFFFFFFFFFFFFFFFFF;
    } else if( tmp_fp_env->ss == ss_double_53) {
        significand_mask     = U128_0xFFFFFFFFFFFFF8000000000000000000;
        significand_even     = U128_0x00000000000008000000000000000000;
        significand_round    = U128_0x00000000000004000000000000000000;
        significand_not_mask = U128_0x00000000000007FFFFFFFFFFFFFFFFFF;
        significand_sticky   = U128_0x00000000000003FFFFFFFFFFFFFFFFFF;
    } else if( tmp_fp_env->ss == ss_double_extended_64) {
        significand_mask     = U128_0xFFFFFFFFFFFFFFFF0000000000000000;
        significand_even     = U128_0x00000000000000010000000000000000;
        significand_round    = U128_0x00000000000000008000000000000000;
        significand_not_mask = U128_0x0000000000000000FFFFFFFFFFFFFFFF;
        significand_sticky   = U128_0x00000000000000007FFFFFFFFFFFFFFF;
    }

/***************************************************
INPUT CHECK
Inf?
****************************************************/
    if ( fp_is_inf_dp(fp_dp) ) {
        tmp_res             = fp_dp;
        tmp_res.significand = fp_U128_band(tmp_res.significand,
                                        significand_mask);
        tmp_rtn = fp_dp_to_fr(tmp_res);
        return(tmp_rtn);

/***************************************************
INPUT CHECK
Nan?
****************************************************/
   } else if ( fp_is_nan_dp(fp_dp) ) {
        tmp_res             = fp_dp;
        tmp_res.significand = fp_U128_band(tmp_res.significand,
                                        significand_mask);
        tmp_rtn = fp_dp_to_fr(tmp_res);
        return(tmp_rtn);

/***************************************************
INPUT CHECK
Zero?
****************************************************/
    } else if ( fp_is_zero_dp(fp_dp) ) {

        if (      (fp_dp.sticky) && (tmp_fp_env->rc == rc_rm) )
            tmp_rtn.sign = 1;
        else if ( (fp_dp.sticky) && (tmp_fp_env->rc != rc_rm) )
            tmp_rtn.sign = 0;
        else
            tmp_rtn.sign = fp_dp.sign;
        tmp_rtn.exponent    = fp_dp.exponent;
        tmp_rtn.significand = 0;
        return(tmp_rtn);

/******************************************************
INPUT CHECK
Answer is finite and non-zero.
*******************************************************/
    } else { 
        tmp_res.sign     = fp_dp.sign;
        tmp_res.exponent = fp_dp.exponent;
        tmp_res.sticky   = fp_dp.sticky;

/****************************************************** 
UNBOUNDED SETUP
Set cnt -- depends on rounding control, +/-, even/odd, round?, sticky?
Set sticky to be either round or sticky 
*******************************************************/
        cnt = (tmp_fp_env->rc<<4) | (fp_dp.sign<<3);
                        
        cnt |= !fp_U128_eq(U128_0, fp_U128_band(fp_dp.significand,
        /* even */                       significand_even))  << 2;
                        
        cnt |= !fp_U128_eq(U128_0, fp_U128_band(fp_dp.significand,
        /* round */                      significand_round)) << 1;
                        
        tmp_res.sticky |= !fp_U128_eq(U128_0, fp_U128_band(fp_dp.significand,
        /* sticky */                                significand_sticky));

        cnt |= tmp_res.sticky;
        tmp_res.sticky |= ((cnt&2) != 0); /* round and sticky */

/*************************************************************************
UNBOUNDED ROUNDING
If necessary, round the significand
   This is the FIRST (or UNBOUNDED) rounding
   If rounding the significand results in a carry out of
   the significand, inc exponent and set significand to 10..0
else
   mask out lower bits of significand
**************************************************************************/
        if (FPA[cnt]) {
            tmp_res.significand = fp_U128_bor(fp_dp.significand,
                                           significand_not_mask);
            tmp_res.significand = fp_U128_inc(tmp_res.significand);
            if ( fp_U128_eq(tmp_res.significand, U128_0) ) { /* carry out */
                tmp_res.exponent++;
                tmp_res.significand = U128_0x80000000000000000000000000000000;
            }
        } else {
            tmp_res.significand = fp_U128_band(fp_dp.significand,
                                            significand_mask);
        }

/*************************************************************************
UNBOUNDED ROUNDING
If significand = 0, set exponent to 0.
CAN THIS EVER HAPPEN IF tmp_res IS NORMALIZED?
**************************************************************************/
        if ( fp_U128_eq(tmp_res.significand, U128_0) ) { /* underflow -> zero */
                tmp_res.exponent = 0;
        }

/*************************************************************************
UNBOUNDED
Save the result of the FIRST ROUNDING in tmp_unbounded_round.
Then, set i flag.
**************************************************************************/        
        tmp_unbounded_round.sign            = tmp_res.sign;
        tmp_unbounded_round.significand     = tmp_res.significand;
        tmp_unbounded_round.exponent        = tmp_res.exponent;
        tmp_unbounded_round.sticky          = tmp_res.sticky;
        tmp_unbounded_fpa                   = FPA[cnt];
        
        if (  ((tmp_unbounded_round.exponent>>17)&1) 
            ^ ((tmp_unbounded_round.exponent>>16)&1) 
           )
                tmp_unbounded_ebc = 1;

        tmp_fp_env->flags.i   = tmp_res.sticky;


/************************************************************
HUGE
if HUGE, set o_flag; 
if o traps enabled, also set o_trap, ebc, fpa
   then if i_flag set, set i_trap and 
   return tmp_unbounded_round with mod17 exponent;
   the fp_dp_to_fr() mods the exponent.
   (sometimes inappropriately called the wrapped value)
else set set tmp_res to max or inf, set i_flag
   if i traps enabled, set i_trap, fpa
   return tmp_res 
*************************************************************/
        if ( tmp_res.exponent > e_max ) { /* huge */
          tmp_fp_env->flags.o = 1;

           if ( !tmp_fp_env->controls.od) {
               tmp_fp_env->ebc = tmp_unbounded_ebc;
               tmp_fp_env->fpa = tmp_unbounded_fpa;
               tmp_fp_env->em_traps.o = 1;
               if(tmp_fp_env->flags.i) {
                  tmp_fp_env->em_traps.i = 1;
               }
               return(fp_dp_to_fr(tmp_unbounded_round));      
 
          } else {
               tmp_res = fp_max_or_infinity(fp_dp.sign, tmp_fp_env,
                                            e_max, significand_mask);
               tmp_fp_env->flags.i = 1;
/****************************************************************
The IEEE standard specifies (7.5) that if you overflow without enabling
O traps, then inexact is always set. Hence, the above assignment.
*****************************************************************/

               if ( !tmp_fp_env->controls.id ) {
                   tmp_fp_env->em_traps.i = 1;
                   tmp_fp_env->fpa        = fp_is_inf_dp(tmp_res);
                   tmp_fp_env->ebc        = 0;
               }
               return(fp_dp_to_fr(tmp_res));
           }

/************************************************************
TINY
If MERCED_RTL, return unbounded, rounded result with mod17 exponent
*************************************************************/

        } else if ( tmp_res.exponent < e_min ) { /* tiny */

/************************************************************
TINY
Undo the rounding.
*************************************************************/

             tmp_res.sign     = fp_dp.sign;
             tmp_res.exponent = fp_dp.exponent;
             tmp_res.sticky   = fp_dp.sticky;

/************************************************************
TINY
Calculate the shift to bring exponent to e_min
if shift >=128 and significand is not zero, 
   set sticky and clear significand
else
   do the shift and set sticky if lost bits from significand
*************************************************************/
            tmp_shift = ((EM_int_t)e_min) - ((EM_int_t)fp_dp.exponent);
            tmp_res.exponent     += tmp_shift;

            if (tmp_shift >= 128) {
                tmp_res.sticky |= !fp_U128_eq( fp_dp.significand, U128_0);
                tmp_res.significand = U128_0;
            } else {
                tmp_res.sticky |= !fp_U128_eq( U128_0,
                                       fp_U128_lsh(
                                          fp_dp.significand,
                                          (128-tmp_shift)));
                tmp_res.significand  = fp_U128_rsh(fp_dp.significand,
                                                   tmp_shift);
            }

/****************************************************** 
TINY SETUP
Set cnt -- depends on rounding control, +/-, even/odd, round?, sticky?
Set sticky to be either round or sticky 
*******************************************************/
            cnt = (tmp_fp_env->rc<<4) | (tmp_res.sign<<3);
                            /* even */
            cnt |= !fp_U128_eq(U128_0, fp_U128_band(tmp_res.significand,
                                             significand_even))  << 2;
                            /* round */
            cnt |= !fp_U128_eq(U128_0, fp_U128_band(tmp_res.significand,
                                             significand_round)) << 1;
                            /* sticky */
            tmp_res.sticky |= !fp_U128_eq(U128_0, fp_U128_band(tmp_res.significand,
                                                        significand_sticky));
            cnt |= tmp_res.sticky;
            tmp_res.sticky |= ((cnt&2) != 0); /* round and sticky */

/*************************************************************************
TINY ROUNDING -- answer is in tmp_res
If necessary, round the significand
   This is the SECOND (as opposed to the FIRST or UNBOUNDED) rounding
   If rounding the significand results in a carry out of
   the significand, inc exponent and set significand to 10..0
else
   mask out lower bits of significand
**************************************************************************/
            if (FPA[cnt]) {
                tmp_res.significand = fp_U128_bor(tmp_res.significand,
                                               significand_not_mask);
                tmp_res.significand = fp_U128_inc(tmp_res.significand);
                if ( fp_U128_eq(tmp_res.significand, U128_0) ) { /* carry out */
                    tmp_res.exponent++;
                    tmp_res.significand =
                            U128_0x80000000000000000000000000000000;
                }
            } else {
                tmp_res.significand = fp_U128_band(tmp_res.significand,
                                                significand_mask);
            }


/******************************************************
TINY ROUNDING
If significand = 0, set exponent to 0.
Then, or in new sticky to the i flag 
*******************************************************/
           if ( fp_U128_eq(tmp_res.significand, U128_0) ) { /* underflow to 0 */
                tmp_res.exponent = 0;
            }

            tmp_fp_env->flags.i |= tmp_res.sticky;


/******************************************************
TINY
Set underflow, if inexact.
*******************************************************/
             if( tmp_fp_env->flags.i )
                 tmp_fp_env->flags.un = 1; /* tiny and inexact */ // MACH

/******************************************************
TINY
If u traps enabled, 
   set u_flag, u_trap, ebc, fpa, and possibly i_trap
   return unbounded result with mod17 exponent;
   the fp_dp_to_fr() mods the exponent.
else 
   if ftz
     set i_flag, set u_flag, clear ebc, clear fpa
     if inexact set i_trap
   else 
     if inexact trap and inexact
        set fpa, set i_trap
   set tmp_rtn (freg) to tmp_res (fp_dp).
   tmp_rtn now has the result of the SECOND rounding.
   Do not return tmp_res yet, because we may have to
   make a canonical double_ext denormal.
*******************************************************/
            tmp_fp_env->fpa        = FPA[cnt];
            if (!tmp_fp_env->controls.ud) {
                tmp_fp_env->flags.un    = 1; // MACH
                tmp_fp_env->em_traps.un = 1; // MACH
                tmp_fp_env->ebc        = tmp_unbounded_ebc;
                tmp_fp_env->fpa        = tmp_unbounded_fpa;
                tmp_fp_env->flags.i    = tmp_unbounded_round.sticky;
                if(tmp_fp_env->flags.i) {
                   tmp_fp_env->em_traps.i = 1; 
                }
                return(fp_dp_to_fr(tmp_unbounded_round));
            }
            else {
                if (tmp_fp_env->ftz) {
                   tmp_res.exponent    = 0;
                   tmp_res.significand = U128_0;
                   tmp_res.sticky      = 1;
                   tmp_fp_env->flags.i = 1;
                   tmp_fp_env->flags.un = 1; // MACH
                   tmp_fp_env->ebc     = 0;
                   tmp_fp_env->fpa     = 0;
                   if (!tmp_fp_env->controls.id) {
                      tmp_fp_env->em_traps.i = 1;
                   }
                } 
                else {
                   if (!tmp_fp_env->controls.id && tmp_fp_env->flags.i) {
                     tmp_fp_env->fpa        = FPA[cnt];
                     tmp_fp_env->em_traps.i = 1;
                   }
               }
            }

            tmp_rtn                = fp_dp_to_fr(tmp_res);

/******************************************************
TINY
if double_extended, set tmp_rtn to canonical denormal
return result of SECOND ROUNDING
*******************************************************/
            if (  (tmp_fp_env->es == es_fifteen_bits)
               && (tmp_rtn.exponent == 0x0C001)
               &&((tmp_rtn.significand & U64_0x8000000000000000) == 0) ) {
               /* canonical double-extended denormal */
                tmp_rtn.exponent = 0x00000;
            }

            return(tmp_rtn);

/******************************************************
NOT HUGE, NOT TINY
if i traps enabled and i flag, set i_trap 
set fpa
*******************************************************/
        } else { 
            if (!tmp_fp_env->controls.id && tmp_fp_env->flags.i) {
                tmp_fp_env->fpa        = tmp_unbounded_fpa;
                tmp_fp_env->em_traps.i = 1;
            }
            tmp_rtn  = fp_dp_to_fr(tmp_unbounded_round);

/******************************************************
NOT HUGE, NOT TINY
if double_extended, set tmp_rtn to canonical denormal
return result of FIRST rounding
*******************************************************/
            if (  (tmp_fp_env->es == es_fifteen_bits)
               && (tmp_rtn.exponent == 0x0C001)
               &&((tmp_rtn.significand & U64_0x8000000000000000) == 0) ) {
               /* canonical double-extended denormal */
                tmp_rtn.exponent = 0x00000;
            }

            return(tmp_rtn);
        } /* end of not huge, not tiny */
    } /* end of infinitely precise and nonzero */
}

#undef fp_ieee_round
#define fp_ieee_round(arg1, arg2)  fp82_fp_ieee_round(ps, arg1, arg2)

// *******************************************************************
// fp_ieee_round_sp()
// Takes a dp register value (which is the hi or lo of a simd)
// Rounds to single precision, setting flags
// Returns the value as a single-precision memory format value
// ********************************************************************
EM_uint_t
fp_ieee_round_sp(
    EM_fp_dp_type      fp_dp,
    EM_simd_hilo       hilo,
    EM_tmp_fp_env_type *tmp_fp_env)
{
EM_fp_reg_type fp_reg;
EM_memory_type tmp_mem;
    fp_reg = fp_ieee_round( fp_dp, tmp_fp_env);
    fp_ieee_to_hilo(hilo, tmp_fp_env);

    if( tmp_fp_env->hi_traps.un || tmp_fp_env->hi_traps.o ||
        tmp_fp_env->lo_traps.un || tmp_fp_env->lo_traps.o  ) { // MACH

       tmp_mem = fr_to_mem4_bias_adjust(fp_reg);
       return (tmp_mem.uint_32.uvalue);
    }
    else {   
     return (fp_single(fp_reg));
    }
}

#undef fp_ieee_round_sp
#define fp_ieee_round_sp(arg1, arg2, arg3) \
        fp82_fp_ieee_round_sp(ps, arg1, arg2, arg3) 


EM_fp_reg_type
fp_ieee_rnd_to_int(
    EM_fp_reg_type fr1,
    EM_tmp_fp_env_type *tmp_fp_env)
{
EM_fp_dp_type      tmp_res;
EM_tmp_fp_env_type tmp_fp_env_local;
    
    tmp_res = fp_fr_to_dp(fr1);
    memcpy ((char *)(&tmp_fp_env_local), (char *)tmp_fp_env,
        sizeof (EM_tmp_fp_env_type)); 

    if (tmp_res.exponent < FP_DP_INTEGER_EXP) {
        if (tmp_res.sign) {
            tmp_res = fp_add(tmp_res, FP_NEG_2_TO_63, *tmp_fp_env);
            tmp_res = fp_fr_to_dp(fp_ieee_round( tmp_res, tmp_fp_env));
            tmp_res = fp_add(tmp_res, FP_POS_2_TO_63, *tmp_fp_env);
            return(fp_ieee_round( tmp_res, &tmp_fp_env_local));
        } else {
            tmp_res = fp_add(tmp_res, FP_POS_2_TO_63, *tmp_fp_env);
            tmp_res = fp_fr_to_dp(fp_ieee_round( tmp_res, tmp_fp_env));
            tmp_res = fp_add(tmp_res, FP_NEG_2_TO_63, *tmp_fp_env);
            return(fp_ieee_round( tmp_res, &tmp_fp_env_local));
        }
    } else
        return (fr1);
}

#undef  fp_ieee_rnd_to_int
#define fp_ieee_rnd_to_int(arg1,arg2) \
        fp82_fp_ieee_rnd_to_int(ps, arg1, arg2)

EM_fp_reg_type
fp_ieee_rnd_to_int_sp(
    EM_fp_reg_type     fr1,
    EM_simd_hilo       hilo,
    EM_tmp_fp_env_type *tmp_fp_env)
{
EM_fp_reg_type      tmp_fix;
EM_tmp_fp_env_type  tmp_fp_env_save;

   tmp_fp_env_save  = *tmp_fp_env;
   tmp_fp_env->ss   = ss_double_extended_64;
   tmp_fp_env->es   = es_seventeen_bits;

   tmp_fix          = fp_ieee_rnd_to_int(fr1, tmp_fp_env);

   fp_ieee_to_hilo(hilo, tmp_fp_env);
   tmp_fp_env->ss   = tmp_fp_env_save.ss;
   tmp_fp_env->es   = tmp_fp_env_save.es;
   return(tmp_fix);
}


    
// ***************************************************************
// Exception fault checks
// *****************************************************************

// ****************************************************************
// fcmp_exception_fault_check()
// *****************************************************************
INLINE void
fcmp_exception_fault_check(
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3,
    EM_opcode_frel_type frel,
    EM_opcode_sf_type   sf,
    EM_tmp_fp_env_type  *tmp_fp_env)
{
EM_fp_reg_type fr2, fr3;

    fr2 = FR[f2];
    fr3 = FR[f3];

    fp_decode_environment( pc_none, sf, tmp_fp_env );

    if (fp_software_assistance_required(ps, op_fcmp, fr2, fr3)) {
        tmp_fp_env->em_faults.swa = 1;
    }

    if (fp_is_unsupported(fr2) || fp_is_unsupported(fr3)) {
        tmp_fp_env->flags.v = 1;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
        }

    } else if (fp_is_nan(fr2) || fp_is_nan(fr3)) {
        if (fp_is_snan(fr2)   || fp_is_snan(fr3)     ||
           (frel == frelLT)  || (frel == frelNLT)  ||
           (frel == frelLE)  || (frel == frelNLE)) {
           tmp_fp_env->flags.v = 1;
           if (!tmp_fp_env->controls.vd) {
              tmp_fp_env->em_faults.v = 1;
           }
        }

    } else if (fp_is_unorm(fr2) || fp_is_unorm(fr3)) {
        tmp_fp_env->flags.d = 1;
        if(!tmp_fp_env->controls.dd)
            tmp_fp_env->em_faults.d = 1;
    }

}

// ****************************************************************
// fpcmp_exception_fault_check()
// *****************************************************************
INLINE void 
fpcmp_exception_fault_check(
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3,
    EM_opcode_frel_type frel,
    EM_opcode_sf_type sf,
    EM_tmp_fp_env_type *tmp_fp_env)
{
EM_fp_reg_type      tmp_fr2 = FR[f2], tmp_fr3 = FR[f3];

    fp_decode_environment( pc_simd, sf, tmp_fp_env );

// ***********
// high
// ************
    tmp_fr2 = fp_reg_read_hi(f2);
    tmp_fr3 = fp_reg_read_hi(f3);

    if (fp_software_assistance_required(ps, op_fpcmp, tmp_fr2, tmp_fr3)) {
        tmp_fp_env->hi_faults.swa = 1;

    } else if (fp_is_nan(tmp_fr2) || fp_is_nan(tmp_fr3))  {
        if ((fp_is_snan(tmp_fr2)  || fp_is_snan(tmp_fr3)   ||
           (frel == frelLT)      || (frel == frelNLT)    ||
           (frel == frelLE)      || (frel == frelNLE))) {
              tmp_fp_env->hi_flags.v = 1;
           if (!tmp_fp_env->controls.vd) {
               tmp_fp_env->hi_faults.v = 1;
           }
       }


    } else if (fp_is_unorm(tmp_fr2) || fp_is_unorm(tmp_fr3)) {
        tmp_fp_env->hi_flags.d = 1;
        if (!tmp_fp_env->controls.dd)
            tmp_fp_env->hi_faults.d = 1;
    }
    

// ***********
// low
// ************
    tmp_fr2 = fp_reg_read_lo(f2);
    tmp_fr3 = fp_reg_read_lo(f3);

    if (fp_software_assistance_required(ps, op_fpcmp, tmp_fr2, tmp_fr3)) {
        tmp_fp_env->lo_faults.swa = 1;

    } else if (fp_is_nan(tmp_fr2) || fp_is_nan(tmp_fr3)) {
       if ((fp_is_snan(tmp_fr2)   || fp_is_snan(tmp_fr3)   ||
           (frel == frelLT)      || (frel == frelNLT)   ||
           (frel == frelLE)      || (frel == frelNLE))) {
              tmp_fp_env->lo_flags.v = 1;
           if (!tmp_fp_env->controls.vd) {
               tmp_fp_env->lo_faults.v = 1;
           }
       }


    } else if (fp_is_unorm(tmp_fr2) || fp_is_unorm(tmp_fr3)) {
        tmp_fp_env->lo_flags.d = 1;
        if (!tmp_fp_env->controls.dd)
            tmp_fp_env->lo_faults.d = 1;
    }
    return;
}

// *******************************************************************
// fcvt_exception_fault_check()
// ********************************************************************
INLINE EM_fp_reg_type
fcvt_exception_fault_check(
   EM_fp_reg_specifier f2,
   EM_opcode_sf_type sf,
   EM_boolean_t signed_form,
   EM_boolean_t trunc_form,
   EM_tmp_fp_env_type *tmp_fp_env)
{
   EM_fp_reg_type     tmp_res, fr2;
   EM_tmp_fp_env_type tmp_fp_env_local;

   fr2 = FR[f2];
   fp_decode_environment( pc_none, sf, tmp_fp_env );
   if (trunc_form)
       tmp_fp_env->rc = rc_rz;

   tmp_res          = fp_reg_read(fr2);
   memcpy ((char *)(&tmp_fp_env_local), (char *)tmp_fp_env,
       sizeof (EM_tmp_fp_env_type)); 
   tmp_res          = fp_ieee_rnd_to_int( tmp_res, &tmp_fp_env_local);

   if( signed_form && fp_software_assistance_required(ps, op_fcvt_fx, fr2)) {
       tmp_fp_env->em_faults.swa = 1;
       return (FP_ZERO);

   } else if( !signed_form  && fp_software_assistance_required(ps, op_fcvt_fxu, fr2)) {
       tmp_fp_env->em_faults.swa = 1;
       return (FP_ZERO);
   }

   if (fp_is_unsupported(fr2)) {
       tmp_fp_env->flags.v = 1;
       tmp_res             = FP_QNAN;
       if (!tmp_fp_env->controls.vd) {
           tmp_fp_env->em_faults.v = 1;
       }

   } else if (fp_is_nan(fr2)) {
       tmp_fp_env->flags.v = 1;
       if (!tmp_fp_env->controls.vd) {
           tmp_fp_env->em_faults.v = 1;
       }
       tmp_res = fp_is_snan(fr2)?fp_make_quiet_nan(fr2):fr2;

   } else if ( signed_form                                     &&
               (!fp_lesser_or_equal(FP_NEG_2_TO_63, tmp_res) ||
                !fp_less_than(tmp_res,FP_POS_2_TO_63)) ) {
             tmp_fp_env->flags.v = 1;
             tmp_res             = FP_QNAN;
             if (!tmp_fp_env->controls.vd)
                 tmp_fp_env->em_faults.v = 1;

   } else if ( !signed_form                              &&
               (!fp_lesser_or_equal(FP_ZERO, tmp_res) ||
                !fp_less_than(tmp_res,FP_POS_2_TO_64)) ) {
             tmp_fp_env->flags.v = 1;
             if (!tmp_fp_env->controls.vd)
                tmp_fp_env->em_faults.v = 1;
             tmp_res = FP_QNAN;

   } else if (fp_is_unorm(fr2)) {
       tmp_fp_env->flags.d = 1;
       if( !tmp_fp_env->controls.dd)
          tmp_fp_env->em_faults.d = 1;
   }

   return (tmp_res);
}

// *******************************************************************
// fpcvt_exception_fault_check()
// ********************************************************************
EM_pair_fp_reg_type
fpcvt_exception_fault_check(
   EM_fp_reg_specifier f2,
   EM_opcode_sf_type   sf,
   EM_boolean_t        signed_form,
   EM_boolean_t        trunc_form,
   EM_tmp_fp_env_type *tmp_fp_env)
{
EM_tmp_fp_env_type  tmp_fp_env_local;
EM_pair_fp_reg_type tmp_reg_pair;
EM_fp_reg_type      tmp_fr2 = FR[f2];

    fp_decode_environment( pc_simd, sf, tmp_fp_env );

    tmp_reg_pair.hi = FP_ZERO;
    tmp_reg_pair.lo = FP_ZERO;

    if (trunc_form)
        tmp_fp_env->rc = rc_rz;

// *************
// high
// **************
    tmp_fr2             = fp_reg_read_hi(f2);
    tmp_fp_env_local    = *tmp_fp_env;
    tmp_fp_env_local.ss = ss_double_extended_64;
    tmp_fp_env_local.es = es_seventeen_bits;
    tmp_reg_pair.hi     = fp_ieee_rnd_to_int( tmp_fr2, &tmp_fp_env_local);

    if ( signed_form &&
         fp_software_assistance_required(ps, op_fpcvt_fx, tmp_fr2)) {
        tmp_fp_env->hi_faults.swa = 1;

    } else if( !signed_form &&
               fp_software_assistance_required(ps, op_fpcvt_fxu, tmp_fr2)) {
        tmp_fp_env->hi_faults.swa = 1;

    } else if (fp_is_nan(tmp_fr2)) {
        tmp_fp_env->hi_flags.v = 1;
        tmp_reg_pair.hi = fp_is_snan(tmp_fr2)?fp_make_quiet_nan(tmp_fr2):tmp_fr2;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->hi_faults.v = 1;


    } else if (signed_form &&
               (!fp_lesser_or_equal(FP_NEG_2_TO_31, tmp_reg_pair.hi) ||
                !fp_less_than(tmp_reg_pair.hi,FP_POS_2_TO_31)) ) {
       tmp_fp_env->hi_flags.v = 1;
       tmp_reg_pair.hi = FP_QNAN;
       if (!tmp_fp_env->controls.vd)
          tmp_fp_env->hi_faults.v = 1;

    } else if (!signed_form &&
               (!fp_lesser_or_equal(FP_ZERO, tmp_reg_pair.hi) ||
                !fp_less_than(tmp_reg_pair.hi,FP_POS_2_TO_32)) ) {
       tmp_fp_env->hi_flags.v = 1;
       tmp_reg_pair.hi = FP_QNAN;
       if (!tmp_fp_env->controls.vd)
          tmp_fp_env->hi_faults.v = 1;

    } else if (fp_is_unorm(tmp_fr2)) {
            tmp_fp_env->hi_flags.d = 1;
            if (!tmp_fp_env->controls.dd)
                tmp_fp_env->hi_faults.d = 1;
    }

// *************
// low
// **************
    tmp_fr2             = fp_reg_read_lo(f2);
    tmp_fp_env_local    = *tmp_fp_env;
    tmp_fp_env_local.ss = ss_double_extended_64;
    tmp_fp_env_local.es = es_seventeen_bits;
    tmp_reg_pair.lo     = fp_ieee_rnd_to_int( tmp_fr2, &tmp_fp_env_local);

    if ( signed_form &&
         fp_software_assistance_required(ps, op_fpcvt_fx, tmp_fr2)) {
        tmp_fp_env->lo_faults.swa = 1;

    } else if( !signed_form &&
               fp_software_assistance_required(ps, op_fpcvt_fxu, tmp_fr2)) {
        tmp_fp_env->lo_faults.swa = 1;

    } else if (fp_is_nan(tmp_fr2)) {
        tmp_fp_env->lo_flags.v = 1;
        tmp_reg_pair.lo = fp_is_snan(tmp_fr2)?fp_make_quiet_nan(tmp_fr2):tmp_fr2;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->lo_faults.v = 1;

    } else if (signed_form &&
               (!fp_lesser_or_equal(FP_NEG_2_TO_31, tmp_reg_pair.lo) ||
                !fp_less_than(tmp_reg_pair.lo,FP_POS_2_TO_31)) ) {
        tmp_fp_env->lo_flags.v = 1;
        tmp_reg_pair.lo = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
           tmp_fp_env->lo_faults.v = 1;

    } else if (!signed_form &&
               (!fp_lesser_or_equal(FP_ZERO, tmp_reg_pair.lo) ||
                !fp_less_than(tmp_reg_pair.lo,FP_POS_2_TO_32)) ) {
        tmp_fp_env->lo_flags.v = 1;
        tmp_reg_pair.lo = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
           tmp_fp_env->lo_faults.v = 1;

    } else if (fp_is_unorm(tmp_fr2)) {
            tmp_fp_env->lo_flags.d = 1;
            if (!tmp_fp_env->controls.dd)
                tmp_fp_env->lo_faults.d = 1;
    }

    return (tmp_reg_pair);
}

// *******************************************************************
// fma_exception_fault_check()
// ********************************************************************
EM_fp_reg_type
fma_exception_fault_check(
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3, 
    EM_fp_reg_specifier f4,
    EM_opcode_pc_type   pc,
    EM_opcode_sf_type   sf,
    EM_tmp_fp_env_type  *tmp_fp_env)
{
    EM_fp_reg_type tmp_res;
    EM_fp_reg_type fr2, fr3, fr4;
   
// printf ("MACH DEBUG: BEGIN fma_exception_fault_check\n");
    fr2 = FR[f2];
    fr3 = FR[f3];
    fr4 = FR[f4];
// printf ("MACH DEBUG: FR2 = %x %x "LX"\n", fr2.sign, fr2.exponent, fr2.significand);
// printf ("MACH DEBUG: FR3 = %x %x "LX"\n", fr3.sign, fr3.exponent, fr3.significand);
// printf ("MACH DEBUG: FR4 = %x %x "LX"\n", fr4.sign, fr4.exponent, fr4.significand);

    fp_decode_environment( pc, sf, tmp_fp_env );

    if(f4==1 && f2==0) {
       if (fp_software_assistance_required(ps, op_fnorm, fr3, *tmp_fp_env)) {
          tmp_fp_env->em_faults.swa = 1;
          return (FP_ZERO);
       }
    } else {
       if (fp_software_assistance_required(ps, op_fma, fr2, fr3, fr4)) {
          tmp_fp_env->em_faults.swa = 1;
          return (FP_ZERO);
       }
    }

    tmp_res = FP_ZERO;

    if (fp_is_unsupported(fr2) || fp_is_unsupported(fr3) || fp_is_unsupported(fr4)) {
        tmp_fp_env->flags.v = 1;
        tmp_res = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
            return (tmp_res);
        }

    } else if (fp_is_nan(fr2) || fp_is_nan(fr3) || fp_is_nan(fr4)) {
         if (fp_is_snan(fr2) || fp_is_snan(fr3) || fp_is_snan(fr4)) {
            tmp_fp_env->flags.v = 1;
            if (!tmp_fp_env->controls.vd) 
               tmp_fp_env->em_faults.v = 1;
         }

         if (fp_is_nan(fr4))
             tmp_res = fp_is_snan(fr4)?fp_make_quiet_nan(fr4):fr4;
         else if (fp_is_nan(fr2))
             tmp_res = fp_is_snan(fr2)?fp_make_quiet_nan(fr2):fr2;
         else if (fp_is_nan(fr3))
             tmp_res = fp_is_snan(fr3)?fp_make_quiet_nan(fr3):fr3;

    } else if (( fp_is_pos_inf(fr3) && fp_is_pos_non_zero(fr4) && fp_is_neg_inf(fr2) )
            || ( fp_is_pos_inf(fr3) && fp_is_neg_non_zero(fr4) && fp_is_pos_inf(fr2) )
   
			|| ( fp_is_neg_inf(fr3) && fp_is_pos_non_zero(fr4) && fp_is_pos_inf(fr2) )
            || ( fp_is_neg_inf(fr3) && fp_is_neg_non_zero(fr4) && fp_is_neg_inf(fr2) )
 
			|| ( fp_is_pos_non_zero(fr3) && fp_is_pos_inf(fr4) && fp_is_neg_inf(fr2) )
            || ( fp_is_pos_non_zero(fr3) && fp_is_neg_inf(fr4) && fp_is_pos_inf(fr2) )

            || ( fp_is_neg_non_zero(fr3) && fp_is_pos_inf(fr4) && fp_is_pos_inf(fr2) )
            || ( fp_is_neg_non_zero(fr3) && fp_is_neg_inf(fr4) && fp_is_neg_inf(fr2) )) {

        tmp_fp_env->flags.v = 1;
        tmp_res = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
            return (tmp_res);
        }

    } else if ((fp_is_inf(fr3) && fp_is_zero(fr4)) || (fp_is_zero(fr3) && fp_is_inf(fr4))) {
        tmp_fp_env->flags.v = 1;
        tmp_res = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
            return (tmp_res);
        }

    
    } else if (fp_is_unorm(fr2) || fp_is_unorm(fr3) || fp_is_unorm(fr4)) {
// printf ("MACH DEBUG: setting the D flag in fma_exception_fault_check\n");
        tmp_fp_env->flags.d = 1;
        if(!tmp_fp_env->controls.dd) { // MACH DEBUG
// printf ("MACH DEBUG: setting the D fault in fma_exception_fault_check\n");
            tmp_fp_env->em_faults.d = 1; 
      } // MACH DEBUG
    }

    return (tmp_res);
}

// *******************************************************************
// fpma_exception_fault_check()
// ********************************************************************
EM_pair_fp_reg_type
fpma_exception_fault_check(
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3,
    EM_fp_reg_specifier f4,
    EM_opcode_sf_type   sf,
    EM_tmp_fp_env_type  *tmp_fp_env)
{
EM_pair_fp_reg_type tmp_reg_pair;
EM_fp_reg_type      tmp_fr2 = FR[f2], tmp_fr3 = FR[f3], tmp_fr4 = FR[f4];

    fp_decode_environment( pc_simd, sf, tmp_fp_env );

    tmp_reg_pair.hi = FP_ZERO;
    tmp_reg_pair.lo = FP_ZERO;

// *********
// high
// *********
    tmp_fr2 = fp_reg_read_hi(f2);
    tmp_fr3 = fp_reg_read_hi(f3);
    tmp_fr4 = fp_reg_read_hi(f4);

    if (fp_software_assistance_required(ps, op_fpma, tmp_fr2, tmp_fr3, tmp_fr4)) {
        tmp_fp_env->hi_faults.swa = 1;

    } else if (fp_is_nan(tmp_fr2) || fp_is_nan(tmp_fr3) || fp_is_nan(tmp_fr4)) {
        if (fp_is_snan(tmp_fr2) || fp_is_snan(tmp_fr3) || fp_is_snan(tmp_fr4)) {
            tmp_fp_env->hi_flags.v = 1;
            if (!tmp_fp_env->controls.vd)
                tmp_fp_env->hi_faults.v = 1;
        }

         if (fp_is_nan(tmp_fr4))
             tmp_reg_pair.hi = fp_is_snan(tmp_fr4)?fp_make_quiet_nan(tmp_fr4):tmp_fr4;
         else if (fp_is_nan(tmp_fr2))
             tmp_reg_pair.hi = fp_is_snan(tmp_fr2)?fp_make_quiet_nan(tmp_fr2):tmp_fr2;
         else if (fp_is_nan(tmp_fr3))
             tmp_reg_pair.hi = fp_is_snan(tmp_fr3)?fp_make_quiet_nan(tmp_fr3):tmp_fr3;

    } else if (( fp_is_pos_inf(tmp_fr3) && fp_is_pos_non_zero(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_pos_inf(tmp_fr3) && fp_is_neg_non_zero(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )

            || ( fp_is_neg_inf(tmp_fr3) && fp_is_pos_non_zero(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )
            || ( fp_is_neg_inf(tmp_fr3) && fp_is_neg_non_zero(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )

            || ( fp_is_pos_non_zero(tmp_fr3) && fp_is_pos_inf(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_pos_non_zero(tmp_fr3) && fp_is_neg_inf(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )

            || ( fp_is_neg_non_zero(tmp_fr3) && fp_is_pos_inf(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )
            || ( fp_is_neg_non_zero(tmp_fr3) && fp_is_neg_inf(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )) {
        tmp_fp_env->hi_flags.v = 1;
        tmp_reg_pair.hi = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->hi_faults.v = 1;

    } else if ((fp_is_inf(tmp_fr3) && fp_is_zero(tmp_fr4))
            || (fp_is_zero(tmp_fr3) && fp_is_inf(tmp_fr4))) {
        tmp_fp_env->hi_flags.v = 1;
        tmp_reg_pair.hi = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->hi_faults.v = 1;


    } else if (fp_is_unorm(tmp_fr2) || fp_is_unorm(tmp_fr3) || fp_is_unorm(tmp_fr4)) {

        tmp_fp_env->hi_flags.d = 1;
        if (!tmp_fp_env->controls.dd)
            tmp_fp_env->hi_faults.d = 1;
    }

// *********
// low
// **********   
    tmp_fr2 = fp_reg_read_lo(f2);
    tmp_fr3 = fp_reg_read_lo(f3);
    tmp_fr4 = fp_reg_read_lo(f4);

    if (fp_software_assistance_required(ps, op_fpma, tmp_fr2, tmp_fr3, tmp_fr4)) {
        tmp_fp_env->lo_faults.swa = 1;
    }

    if (fp_is_nan(tmp_fr2) || fp_is_nan(tmp_fr3) || fp_is_nan(tmp_fr4)) {
        if (fp_is_snan(tmp_fr2) || fp_is_snan(tmp_fr3) || fp_is_snan(tmp_fr4)) {
            tmp_fp_env->lo_flags.v = 1;
            if (!tmp_fp_env->controls.vd)
                tmp_fp_env->lo_faults.v = 1;
        }

         if (fp_is_nan(tmp_fr4))
             tmp_reg_pair.lo = fp_is_snan(tmp_fr4)?fp_make_quiet_nan(tmp_fr4):tmp_fr4;
         else if (fp_is_nan(tmp_fr2))
             tmp_reg_pair.lo = fp_is_snan(tmp_fr2)?fp_make_quiet_nan(tmp_fr2):tmp_fr2;
         else if (fp_is_nan(tmp_fr3))
             tmp_reg_pair.lo = fp_is_snan(tmp_fr3)?fp_make_quiet_nan(tmp_fr3):tmp_fr3;

    } else if (( fp_is_pos_inf(tmp_fr3) && fp_is_pos_non_zero(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_pos_inf(tmp_fr3) && fp_is_neg_non_zero(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )

            || ( fp_is_neg_inf(tmp_fr3) && fp_is_pos_non_zero(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )
            || ( fp_is_neg_inf(tmp_fr3) && fp_is_neg_non_zero(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )

            || ( fp_is_pos_non_zero(tmp_fr3) && fp_is_pos_inf(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_pos_non_zero(tmp_fr3) && fp_is_neg_inf(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )

            || ( fp_is_neg_non_zero(tmp_fr3) && fp_is_pos_inf(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )
            || ( fp_is_neg_non_zero(tmp_fr3) && fp_is_neg_inf(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )) {

        tmp_fp_env->lo_flags.v = 1;
        tmp_reg_pair.lo = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->lo_faults.v = 1;

    } else if ((fp_is_inf(tmp_fr3) && fp_is_zero(tmp_fr4))
            || (fp_is_zero(tmp_fr3) && fp_is_inf(tmp_fr4))) {
        tmp_fp_env->lo_flags.v = 1;
        tmp_reg_pair.lo = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->lo_faults.v = 1;

    } else if (fp_is_unorm(tmp_fr2) || fp_is_unorm(tmp_fr3) || fp_is_unorm(tmp_fr4)) {
        tmp_fp_env->lo_flags.d = 1;
        if (!tmp_fp_env->controls.dd)
            tmp_fp_env->lo_faults.d = 1;
    }

    return (tmp_reg_pair);
}


// *******************************************************************
// fpminmax_exception_fault_check()
// No return value
// If input contains a NATVAL, just return.
// Otherwise set flags appropriately so that fpsr will
// be correct or a fault taken in caller.
// ********************************************************************
INLINE void 
fpminmax_exception_fault_check(
    EM_uint_t f2,
    EM_uint_t f3,
    EM_opcode_sf_type sf,
    EM_tmp_fp_env_type *tmp_fp_env)
{
EM_fp_reg_type      tmp_fr2 = FR[f2], tmp_fr3 = FR[f3];

// MACH
    fp_decode_environment( pc_simd, sf, tmp_fp_env );

// ************
// high
// ************

    tmp_fr2 = fp_reg_read_hi(f2);
    tmp_fr3 = fp_reg_read_hi(f3);

    if (fp_software_assistance_required(ps, op_fpminmax, tmp_fr2, tmp_fr3)) {
        tmp_fp_env->hi_faults.swa = 1; 

    } else if (fp_is_nan(tmp_fr2) || fp_is_nan(tmp_fr3)) {
        tmp_fp_env->hi_flags.v = 1;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->hi_faults.v = 1;
        }

    } else if (fp_is_unorm(tmp_fr2) || fp_is_unorm(tmp_fr3)) {
        tmp_fp_env->hi_flags.d = 1;
        if (!tmp_fp_env->controls.dd) {
            tmp_fp_env->hi_faults.d = 1;
        }
    }

// ************
// low
// ************
    tmp_fr2 = fp_reg_read_lo(f2);
    tmp_fr3 = fp_reg_read_lo(f3);
 
    if (fp_software_assistance_required(ps, op_fpminmax, tmp_fr2, tmp_fr3)) {
        tmp_fp_env->lo_faults.swa = 1;

    } else if (fp_is_nan(tmp_fr2) || fp_is_nan(tmp_fr3)) {
        tmp_fp_env->lo_flags.v = 1;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->lo_faults.v = 1;
        }


    } else if (fp_is_unorm(tmp_fr2) || fp_is_unorm(tmp_fr3)) {
        tmp_fp_env->lo_flags.d = 1;
        if (!tmp_fp_env->controls.dd)
            tmp_fp_env->lo_faults.d = 1;
    }

    return;
}

// *******************************************************************
// fminmax_exception_fault_check()
// *******************************************************************
INLINE void
fminmax_exception_fault_check(
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3,
    EM_opcode_sf_type sf,
    EM_tmp_fp_env_type *tmp_fp_env)
{
EM_fp_reg_type fr2, fr3;

    fr2 = FR[f2];
    fr3 = FR[f3];

    fp_decode_environment( pc_none, sf, tmp_fp_env );

    if (fp_software_assistance_required(ps, op_fminmax, fr2, fr3)) {
        tmp_fp_env->em_faults.swa = 1;
    }

    if (fp_is_unsupported(fr2) || fp_is_unsupported(fr3)) {
        tmp_fp_env->flags.v = 1;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
        }

    } else if (fp_is_nan(fr2) || fp_is_nan(fr3)) {
        tmp_fp_env->flags.v = 1;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
        }


    } else if (fp_is_unorm(fr2) || fp_is_unorm(fr3)) {
        tmp_fp_env->flags.d = 1;
        if (!tmp_fp_env->controls.dd)
           tmp_fp_env->em_faults.d = 1;
    }

}

// *******************************************************************
// fms_fnma_()
// *******************************************************************
EM_fp_reg_type
fms_fnma_exception_fault_check(
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3, 
    EM_fp_reg_specifier f4, 
    EM_opcode_pc_type   pc,
    EM_opcode_sf_type   sf,
    EM_tmp_fp_env_type  *tmp_fp_env)
{
EM_fp_reg_type fr2, fr3, fr4;
EM_fp_reg_type tmp_res;

    fr2 = FR[f2];
    fr3 = FR[f3];
    fr4 = FR[f4];

    fp_decode_environment( pc, sf, tmp_fp_env );

    if (fp_software_assistance_required(ps, op_fms_fnma, fr2, fr3, fr4)) {
        tmp_fp_env->em_faults.swa = 1;
        return (FP_ZERO);
    }

    tmp_res = FP_ZERO;

    if (fp_is_unsupported(fr2) || fp_is_unsupported(fr3) || fp_is_unsupported(fr4)) {
        tmp_fp_env->flags.v = 1;
        tmp_res = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
            return (tmp_res);
        }

    } else if (fp_is_nan(fr2) || fp_is_nan(fr3) || fp_is_nan(fr4)) {
         if (fp_is_snan(fr2) || fp_is_snan(fr3) || fp_is_snan(fr4)) {
            tmp_fp_env->flags.v = 1;
            if (!tmp_fp_env->controls.vd)  
               tmp_fp_env->em_faults.v = 1;
         }


         if (fp_is_nan(fr4))
             tmp_res = fp_is_snan(fr4)?fp_make_quiet_nan(fr4):fr4;
         else if (fp_is_nan(fr2))
             tmp_res = fp_is_snan(fr2)?fp_make_quiet_nan(fr2):fr2;
         else if (fp_is_nan(fr3))
             tmp_res = fp_is_snan(fr3)?fp_make_quiet_nan(fr3):fr3;

    } else if (( fp_is_pos_inf(fr3) && fp_is_pos_non_zero(fr4) && fp_is_pos_inf(fr2) )
            || ( fp_is_pos_inf(fr3) && fp_is_neg_non_zero(fr4) && fp_is_neg_inf(fr2) )
            || ( fp_is_neg_inf(fr3) && fp_is_pos_non_zero(fr4) && fp_is_neg_inf(fr2) )
            || ( fp_is_neg_inf(fr3) && fp_is_neg_non_zero(fr4) && fp_is_pos_inf(fr2) )
            || ( fp_is_pos_non_zero(fr3) && fp_is_pos_inf(fr4) && fp_is_pos_inf(fr2) )
            || ( fp_is_pos_non_zero(fr3) && fp_is_neg_inf(fr4) && fp_is_neg_inf(fr2) )
            || ( fp_is_neg_non_zero(fr3) && fp_is_pos_inf(fr4) && fp_is_neg_inf(fr2) )
            || ( fp_is_neg_non_zero(fr3) && fp_is_neg_inf(fr4) && fp_is_pos_inf(fr2) )) {
        tmp_fp_env->flags.v = 1;
        tmp_res = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
            return (tmp_res);
        }

    } else if ((fp_is_inf(fr3) && fp_is_zero(fr4)) || (fp_is_zero(fr3) && fp_is_inf(fr4))) {
        tmp_fp_env->flags.v = 1;
        tmp_res = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
            return (tmp_res);
        }


    } else if (fp_is_unorm(fr2) || fp_is_unorm(fr3) || fp_is_unorm(fr4)) {
        tmp_fp_env->flags.d = 1;
        if (!tmp_fp_env->controls.dd)
           tmp_fp_env->em_faults.d = 1;
    }

    return (tmp_res);
}

// *******************************************************************
// fpms_fpnma_exception_fault_check()
// ********************************************************************
EM_pair_fp_reg_type
fpms_fpnma_exception_fault_check(
    EM_fp_reg_specifier f2,
    EM_fp_reg_specifier f3,
    EM_fp_reg_specifier f4,
    EM_opcode_sf_type   sf,
    EM_tmp_fp_env_type  *tmp_fp_env)
{
EM_pair_fp_reg_type tmp_reg_pair;
EM_fp_reg_type      tmp_fr2 = FR[f2], tmp_fr3 = FR[f3], tmp_fr4 = FR[f4];

    fp_decode_environment( pc_simd, sf, tmp_fp_env );

    tmp_reg_pair.hi = FP_ZERO;
    tmp_reg_pair.lo = FP_ZERO;

// ***************
// high
// ***************
    tmp_fr2 = fp_reg_read_hi(f2);
    tmp_fr3 = fp_reg_read_hi(f3);
    tmp_fr4 = fp_reg_read_hi(f4);

    if (fp_software_assistance_required(ps, op_fpms_fpnma, tmp_fr2, tmp_fr3, tmp_fr4)) {
        tmp_fp_env->hi_faults.swa = 1;

    } else if (fp_is_unsupported(tmp_fr2) || fp_is_unsupported(tmp_fr3) || fp_is_unsupported(tmp_fr4)) {
        tmp_fp_env->hi_flags.v = 1;
        tmp_reg_pair.hi        = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->hi_faults.v = 1;

    } else if (fp_is_nan(tmp_fr2) || fp_is_nan(tmp_fr3) || fp_is_nan(tmp_fr4)) {
        if (fp_is_snan(tmp_fr2) || fp_is_snan(tmp_fr3) || fp_is_snan(tmp_fr4)) {
            tmp_fp_env->hi_flags.v = 1;
            if (!tmp_fp_env->controls.vd)
                tmp_fp_env->hi_faults.v = 1;
        }

         if (fp_is_nan(tmp_fr4))
             tmp_reg_pair.hi = fp_is_snan(tmp_fr4)?fp_make_quiet_nan(tmp_fr4):tmp_fr4;
         else if (fp_is_nan(tmp_fr2))
             tmp_reg_pair.hi = fp_is_snan(tmp_fr2)?fp_make_quiet_nan(tmp_fr2):tmp_fr2;
         else if (fp_is_nan(tmp_fr3))
             tmp_reg_pair.hi = fp_is_snan(tmp_fr3)?fp_make_quiet_nan(tmp_fr3):tmp_fr3;

    } else if (( fp_is_pos_inf(tmp_fr3) && fp_is_pos_non_zero(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )
            || ( fp_is_pos_inf(tmp_fr3) && fp_is_neg_non_zero(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_neg_inf(tmp_fr3) && fp_is_pos_non_zero(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_neg_inf(tmp_fr3) && fp_is_neg_non_zero(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )
            || ( fp_is_pos_non_zero(tmp_fr3) && fp_is_pos_inf(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )
            || ( fp_is_pos_non_zero(tmp_fr3) && fp_is_neg_inf(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_neg_non_zero(tmp_fr3) && fp_is_pos_inf(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_neg_non_zero(tmp_fr3) && fp_is_neg_inf(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )) {
        tmp_fp_env->hi_flags.v = 1;
        tmp_reg_pair.hi        = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->hi_faults.v = 1;

    } else if ((fp_is_inf(tmp_fr3) && fp_is_zero(tmp_fr4))
            || (fp_is_zero(tmp_fr3) && fp_is_inf(tmp_fr4))) {
        tmp_fp_env->hi_flags.v = 1;
        tmp_reg_pair.hi = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->hi_faults.v = 1;

    } else if (fp_is_unorm(tmp_fr2) || fp_is_unorm(tmp_fr3) || fp_is_unorm(tmp_fr4)) {
        tmp_fp_env->hi_flags.d = 1;
        if (!tmp_fp_env->controls.dd)
            tmp_fp_env->hi_faults.d = 1;
    }

// ***************
// low
// ***************
    tmp_fr2 = fp_reg_read_lo(f2);
    tmp_fr3 = fp_reg_read_lo(f3);
    tmp_fr4 = fp_reg_read_lo(f4);

    if (fp_software_assistance_required(ps, op_fpms_fpnma, tmp_fr2, tmp_fr3, tmp_fr4)) {
        tmp_fp_env->lo_faults.swa = 1;

    } else if (fp_is_unsupported(tmp_fr2) || fp_is_unsupported(tmp_fr3) || fp_is_unsupported(tmp_fr4)) {
        tmp_fp_env->lo_flags.v = 1;
        tmp_reg_pair.lo = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->lo_faults.v = 1;

    } else if (fp_is_nan(tmp_fr2) || fp_is_nan(tmp_fr3) || fp_is_nan(tmp_fr4)) {
        if (fp_is_snan(tmp_fr2) || fp_is_snan(tmp_fr3) || fp_is_snan(tmp_fr4)) {
            tmp_fp_env->lo_flags.v = 1;
            if (!tmp_fp_env->controls.vd)
                tmp_fp_env->lo_faults.v = 1;
        }

         if (fp_is_nan(tmp_fr4))
             tmp_reg_pair.lo = fp_is_snan(tmp_fr4)?fp_make_quiet_nan(tmp_fr4):tmp_fr4;
         else if (fp_is_nan(tmp_fr2))
             tmp_reg_pair.lo = fp_is_snan(tmp_fr2)?fp_make_quiet_nan(tmp_fr2):tmp_fr2;
         else if (fp_is_nan(tmp_fr3))
             tmp_reg_pair.lo = fp_is_snan(tmp_fr3)?fp_make_quiet_nan(tmp_fr3):tmp_fr3;

    } else if (( fp_is_pos_inf(tmp_fr3) && fp_is_pos_non_zero(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )
            || ( fp_is_pos_inf(tmp_fr3) && fp_is_neg_non_zero(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_neg_inf(tmp_fr3) && fp_is_pos_non_zero(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_neg_inf(tmp_fr3) && fp_is_neg_non_zero(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )
            || ( fp_is_pos_non_zero(tmp_fr3) && fp_is_pos_inf(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )
            || ( fp_is_pos_non_zero(tmp_fr3) && fp_is_neg_inf(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_neg_non_zero(tmp_fr3) && fp_is_pos_inf(tmp_fr4)
                                     && fp_is_neg_inf(tmp_fr2) )
            || ( fp_is_neg_non_zero(tmp_fr3) && fp_is_neg_inf(tmp_fr4)
                                     && fp_is_pos_inf(tmp_fr2) )) {
        tmp_fp_env->lo_flags.v = 1;
        tmp_reg_pair.lo = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->lo_faults.v = 1;

    } else if ((fp_is_inf(tmp_fr3) && fp_is_zero(tmp_fr4))
            || (fp_is_zero(tmp_fr3) && fp_is_inf(tmp_fr4))) {
        tmp_fp_env->lo_flags.v = 1;
        tmp_reg_pair.lo = FP_QNAN;
        if (!tmp_fp_env->controls.vd)
            tmp_fp_env->lo_faults.v = 1;

    } else if (fp_is_unorm(tmp_fr2) || fp_is_unorm(tmp_fr3) || fp_is_unorm(tmp_fr4)) {
        tmp_fp_env->lo_flags.d = 1;
        if (!tmp_fp_env->controls.dd)
            tmp_fp_env->lo_faults.d = 1;
    }

    return (tmp_reg_pair);
}



INLINE EM_fp_dp_type
fp_max_or_infinity(EM_uint_t sign, EM_tmp_fp_env_type *tmp_fp_env,
     EM_uint_t e_max, EM_uint128_t max_significand)
{
    EM_fp_dp_type tmp_res;
    tmp_res.sign = sign;

    if (tmp_fp_env->rc == rc_rm) {
        if (tmp_res.sign) {
            tmp_res.exponent    = FP_DP_EXP_ONES;
            tmp_res.significand = U128_0x80000000000000000000000000000000;
        }else {
            tmp_res.exponent    = e_max;
            tmp_res.significand = max_significand;
        }

    } else if (tmp_fp_env->rc == rc_rz) {
        tmp_res.exponent    = e_max;
        tmp_res.significand = max_significand;

    } else if (tmp_fp_env->rc == rc_rp) {
        if (tmp_res.sign) {
            tmp_res.exponent    = e_max;
            tmp_res.significand = max_significand;
        }else {
            tmp_res.exponent    = FP_DP_EXP_ONES;
            tmp_res.significand = U128_0x80000000000000000000000000000000;
        }

    } else {
        tmp_res.exponent    = FP_DP_EXP_ONES;
        tmp_res.significand = U128_0x80000000000000000000000000000000;
    }

    return(tmp_res);
}

INLINE EM_fp_dp_type
fp_mul(EM_fp_reg_type fr3, EM_fp_reg_type fr4)
{
    EM_fp_dp_type tmp_res;
    EM_int_t normalize_count;

// all cases which might have faulted have been screened out 
// we still may trap on overflow, underflow and/or inexact later

    if (fp_is_zero(fr3) || fp_is_zero(fr4)) {
            /* correctly signed zero */
        tmp_res = fp_fr_to_dp(FP_ZERO);
        tmp_res.sign = fr3.sign ^ fr4.sign;
    } else if (fp_is_inf(fr3) || fp_is_inf(fr4)) {
            /* correctly signed inf*/
        tmp_res = fp_fr_to_dp(FP_INFINITY);
        tmp_res.sign = fr3.sign ^ fr4.sign;
    } else if (fp_is_pseudo_zero(fr3) || fp_is_pseudo_zero(fr4)) {
            /* pseudo zero  if one operand is a pseudo-zero, return real zero.
               pz * NaN = Nan, but we already tested for Nan                   */
        tmp_res = fp_fr_to_dp(FP_ZERO);
        tmp_res.sign = fr3.sign ^ fr4.sign;
    } else {
            /* (un)normal * (un)normal */
        tmp_res.sign         = fr3.sign ^ fr4.sign;
        tmp_res.exponent     = (EM_uint_t)(
                                         (((EM_int_t)fr3.exponent)-FP_REG_BIAS)
                                       + (((EM_int_t)fr4.exponent)-FP_REG_BIAS)
                                       + FP_DP_BIAS);
                /* x.xxx (64-bits) * y.yyy (64-bits)
                => zz.zzzzzz (128-bits) */
        tmp_res.significand = fp_U64_x_U64_to_U128(fr3.significand,
                                                fr4.significand);
        if (fp_U128_lead0(tmp_res.significand) == 0) {
                /* 1.xxx (64-bits) * 1.yyy (64-bits)
                => 1z.zzzzzz (128-bits) */
            tmp_res.exponent += 1;
                /* 1z.zzzzzz
                => 1.zzzzzzz (128-bits) */
        } else if (fp_U128_lead0(tmp_res.significand) == 1) {
                /* 1.xxx (64-bits) * 1.yyy (64-bits)
                => 0z.zzzzzz (128-bits) */
            tmp_res.significand = fp_U128_lsh(tmp_res.significand,1);
                /* 0z.zzzzzz => z.zzzzzz0 (128-bits) */
        } else {
                /* 0.xxx (64-bits) * 0.yyy (64-bits)
                => 00.zzzzzz (128-bits) all unsigned int's */
            normalize_count = fp_U128_lead0(tmp_res.significand);
            tmp_res.exponent -= normalize_count-1;
            tmp_res.significand = fp_U128_lsh(tmp_res.significand,
                                                 normalize_count);
        }
    }
    tmp_res.sticky = 0;
    return(tmp_res);
}

INLINE EM_fp_reg_type
fp_normalize(EM_fp_reg_type freg)
{
    EM_int_t tmp_normalize_count;

    if (fp_is_nan(freg) || fp_is_inf(freg) || fp_is_normal(freg)
    || fp_is_unsupported(freg) || fp_is_zero(freg) || fp_is_natval(freg))
        return (freg);

    tmp_normalize_count = fp_U64_lead0(freg.significand);
    if (tmp_normalize_count == 64) { /* ftz pseudo-zero */
        if(freg.exponent)
            freg.exponent = 0;
        return (freg);
    } else if(freg.exponent == 1) {
        return(freg);


    } else if ((((EM_int_t)freg.exponent) - tmp_normalize_count) <= 0) {
        tmp_normalize_count = freg.exponent -1;
        freg.exponent       = 1;
        freg.significand  <<= tmp_normalize_count;
        return (freg);

    } else { /* normalize */
        freg.exponent -= tmp_normalize_count;
        freg.significand <<= tmp_normalize_count;
        return(freg);
    }
}

INLINE EM_fp_dp_type
fp_normalize_dp(EM_fp_dp_type fp_dp)
{
    EM_int_t tmp_normalize_count;

    if (fp_is_nan_dp(fp_dp) || fp_is_inf_dp(fp_dp) || fp_is_normal_dp(fp_dp)
    || fp_is_zero_dp(fp_dp))
        return (fp_dp);
    else if (fp_is_unsupported_dp(fp_dp)) /* unsupported are turned into nans */
        return (fp_fr_to_dp(FP_QNAN));
    tmp_normalize_count = fp_U128_lead0(fp_dp.significand);
    if (tmp_normalize_count == 128) { /* ftz pseudo-zero */
        if (fp_dp.exponent)
            fp_dp.exponent = 0;
        return (fp_dp);
        
    } else if ((((EM_int_t)fp_dp.exponent) - tmp_normalize_count) <= 0) {
        /* ftz register file format (pseudo-)denormals */
        fp_dp.exponent = 0;
        fp_dp.significand = U128_0;
        return (fp_dp);
    } else { /* normalize */
        fp_dp.exponent -= tmp_normalize_count;
        fp_dp.significand = fp_U128_lsh(fp_dp.significand,
                                           tmp_normalize_count);
        return(fp_dp);
    }
}


EM_fp_dp_type
fp82_fp_fr_to_dp(EM_fp_reg_type fr1)
{
    EM_fp_dp_type tmp_res;
    tmp_res.sign = fr1.sign;
    if (fr1.exponent == 0)
        tmp_res.exponent = 0;
    else if (fr1.exponent == FP_REG_EXP_ONES)
        tmp_res.exponent = FP_DP_EXP_ONES;
    else
        tmp_res.exponent = (EM_uint_t)(((EM_int_t)fr1.exponent)
                                   - FP_REG_BIAS + FP_DP_BIAS);

    tmp_res.significand.hi = fr1.significand;
    tmp_res.significand.lo = U64_0;
    tmp_res.sticky = 0;
    return(fp_normalize_dp(tmp_res));
}

// *******************************************************************
// frcpa_exception_fault_check()
// *******************************************************************
EM_fp_reg_type
frcpa_exception_fault_check(
   EM_fp_reg_specifier f2,
   EM_fp_reg_specifier f3, 
   EM_opcode_sf_type   sf,
   EM_tmp_fp_env_type  *tmp_fp_env)
{
    EM_fp_reg_type fr2, fr3;
    EM_fp_reg_type tmp_res;
    EM_int_t estimated_exponent;

    fr2 = FR[f2];
    fr3 = FR[f3];
    fp_decode_environment( pc_none, sf, tmp_fp_env );

    if (fp_software_assistance_required(ps, op_frcpa, fr2, fr3) ) {
        tmp_fp_env->em_faults.swa = 1;
        return (FP_ZERO);
    }

    tmp_res = FP_ZERO;

    if (fp_is_unsupported(fr2) || fp_is_unsupported(fr3)) {
        tmp_fp_env->flags.v = 1;
        tmp_res             = FP_QNAN;

        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
        }

    } else if (fp_is_nan(fr2) || fp_is_nan(fr3)) {
         if (fp_is_snan(fr2) || fp_is_snan(fr3)) {
             tmp_fp_env->flags.v = 1;
             if (!tmp_fp_env->controls.vd) {
                 tmp_fp_env->em_faults.v = 1;
             }
         }
         if (fp_is_nan(fr2)) {
             tmp_res = fp_is_snan(fr2)?fp_make_quiet_nan(fr2):fr2;
         } else if (fp_is_nan(fr3)) {
             tmp_res = fp_is_snan(fr3)?fp_make_quiet_nan(fr3):fr3;
         }

    } else if ( (fp_is_inf(fr2)  && fp_is_inf(fr3))
              || ( (fp_is_zero(fr2) || fp_is_pseudo_zero(fr2))
                && (fp_is_zero(fr3) || fp_is_pseudo_zero(fr3)) ) ) {
        tmp_fp_env->flags.v = 1;
        tmp_res = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
        }

    } else if (  ( ( fp_is_normal(fr2) && !fp_is_zero(fr2) )
                || ( fp_is_unorm(fr2)  && !fp_is_pseudo_zero(fr2) ) )
              && (   fp_is_zero(fr3)   ||  fp_is_pseudo_zero(fr3) ) ) {
        tmp_fp_env->flags.z = 1;
        tmp_res = FP_INFINITY;
        tmp_res.sign = fr2.sign ^ fr3.sign;
        if (!tmp_fp_env->controls.zd) {
            tmp_fp_env->em_faults.z = 1;
        }


    } else if (fp_is_unorm(fr2) || fp_is_unorm(fr3)) {
        tmp_fp_env->flags.d = 1;
        if (!tmp_fp_env->controls.dd) {
            tmp_fp_env->em_faults.d = 1;
        }
    } 
/************************************************************
This is the architecturally mandated swa fault check.

The precision of the working type is 17-bit exponent.
   fp_normalize() will normalize except in the case of a register denormal.
   In this context, fp_is_unorm() returns 1 if integer bit is 0
   and that occurs when fr3.exponent < emin.
Note that the estimated exponent is unbiased, because the bias
is subtracted out.
*************************************************************/
    if ( !fp_is_zero(fr2) && fp_is_finite(fr2) && !fp_is_pseudo_zero(fr2)
      && !fp_is_zero(fr3) && fp_is_finite(fr3) && !fp_is_pseudo_zero(fr3)
       ) {

        fr2 = fp_normalize(fp_reg_read(fr2));
        fr3 = fp_normalize(fp_reg_read(fr3));

        estimated_exponent = (EM_int_t)(fr2.exponent)
                           - (EM_int_t)(fr3.exponent);
        
	    if ( fp_is_unorm(fr3) 
          || ( fr3.exponent       >=  (FP_REG_BIAS+FP_REG_BIAS-2) )
          || ( estimated_exponent >=  ((EM_int_t)(FP_REG_BIAS)) )
          || ( estimated_exponent <= (2 - (EM_int_t)FP_REG_BIAS) )
          || ( fr2.exponent       <= (ss_double_extended_64)     )
           ) {
                 tmp_fp_env->em_faults.swa = 1; 
        }
    }
    return (tmp_res);
}

// *******************************************************************
// fprcpa_exception_fault_check()
// *******************************************************************
EM_pair_fp_reg_type
fprcpa_exception_fault_check(
   EM_fp_reg_specifier    f2,
   EM_fp_reg_specifier    f3,
   EM_opcode_sf_type      sf,
   EM_tmp_fp_env_type     *tmp_fp_env, 
   EM_limits_check_fprcpa  *limits_check)
{
EM_pair_fp_reg_type tmp_reg_pair;
EM_fp_reg_type      tmp_fr2 = FR[f2], tmp_fr3 = FR[f3];
EM_int_t            estimated_exponent;

    fp_decode_environment( pc_simd, sf, tmp_fp_env );

    tmp_reg_pair.hi = FP_ZERO;
    tmp_reg_pair.lo = FP_ZERO;

// ************
// high
// ************
    tmp_fr2 = fp_reg_read_hi(f2);
    tmp_fr3 = fp_reg_read_hi(f3);

    if (fp_software_assistance_required(ps, op_fprcpa, tmp_fr2, tmp_fr3)) {
        tmp_fp_env->hi_faults.swa = 1;

    } else if (fp_is_nan(tmp_fr2) || fp_is_nan(tmp_fr3)) {
        if (fp_is_snan(tmp_fr2) || fp_is_snan(tmp_fr3)) {
            tmp_fp_env->hi_flags.v = 1;
            if (!tmp_fp_env->controls.vd) {
                tmp_fp_env->hi_faults.v = 1;
            }
        }
        if (fp_is_nan(tmp_fr2)) {
            tmp_reg_pair.hi = fp_is_snan(tmp_fr2)
                            ? fp_make_quiet_nan(tmp_fr2) : tmp_fr2;
        } else if (fp_is_nan(tmp_fr3)) {
            tmp_reg_pair.hi = fp_is_snan(tmp_fr3)
                            ? fp_make_quiet_nan(tmp_fr3) : tmp_fr3;
        }

/*******************************************************************************
(f2 and f3 are inf) or (f2 and f3 are 0); returns qnan
********************************************************************************/
    } else if ( (fp_is_inf(tmp_fr2)  && fp_is_inf(tmp_fr3)  )
             || (fp_is_zero(tmp_fr2) && fp_is_zero(tmp_fr3) ) ) {
        tmp_fp_env->hi_flags.v = 1;
        tmp_reg_pair.hi = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->hi_faults.v = 1;
        }
/*******************************************************************************
(f2 is non-zero (normal or denormal but not inf) and f3 is zero; returns inf
The reason for the "but not inf" is because inf/0 shoudl not set the
divide-by-zero flag.
********************************************************************************/
    } else if ( !fp_is_inf(tmp_fr2) && !fp_is_zero(tmp_fr2) && fp_is_zero(tmp_fr3) ) {
        tmp_fp_env->hi_flags.z = 1;
        tmp_reg_pair.hi        = FP_INFINITY;
        tmp_reg_pair.hi.sign   = tmp_fr2.sign ^ tmp_fr3.sign;
        if (!tmp_fp_env->controls.zd) {
            tmp_fp_env->hi_faults.z = 1;
        }

    } else if (fp_is_unorm(tmp_fr2) || fp_is_unorm(tmp_fr3)) {
        tmp_fp_env->hi_flags.d = 1;
        if (!tmp_fp_env->controls.dd) {
            tmp_fp_env->hi_faults.d = 1;
        }
    } 

    if ( !fp_is_zero(tmp_fr2) && fp_is_finite(tmp_fr2)
      && !fp_is_zero(tmp_fr3) && fp_is_finite(tmp_fr3) ) {

        tmp_fr2 = fp_normalize(tmp_fr2);

        if ( fp_is_unorm(tmp_fr3) ) {
            limits_check->hi_fr3        = 1; /* recip(fr3_hi) not rep. */
            tmp_reg_pair.hi             = FP_INFINITY;  
            tmp_reg_pair.hi.sign        = tmp_fr3.sign;
            tmp_fr3                     = fp_normalize(tmp_fr3);

        } else if ( tmp_fr3.exponent >= (FP_REG_BIAS+FP_SGL_BIAS-2) ) {
            limits_check->hi_fr3        = 1; /* recip(fr3_hi) not rep. */
            tmp_reg_pair.hi             = FP_ZERO;             
            tmp_reg_pair.hi.sign        = tmp_fr3.sign;
        }

        estimated_exponent = (EM_int_t)tmp_fr2.exponent
                           - (EM_int_t)tmp_fr3.exponent;


        if (  (estimated_exponent >= (((EM_int_t)(FP_SGL_BIAS)))          )
           || (estimated_exponent <= (2-((EM_int_t)FP_SGL_BIAS))            )
           || (tmp_fr2.exponent   <= (ss_single_24+FP_REG_BIAS-FP_SGL_BIAS) )
           ) {
            limits_check->hi_fr2_or_quot = 1; /* hi est. quot. or fr2_hi */
        }
    }

// ************
// low
// ************
    tmp_fr2 = fp_reg_read_lo(f2);
    tmp_fr3 = fp_reg_read_lo(f3);

    if (fp_software_assistance_required(ps, op_fprcpa, tmp_fr2, tmp_fr3)) {
        tmp_fp_env->lo_faults.swa = 1;

    } else if (fp_is_nan(tmp_fr2) || fp_is_nan(tmp_fr3)) {
        if (fp_is_snan(tmp_fr2) || fp_is_snan(tmp_fr3)) {
            tmp_fp_env->lo_flags.v = 1;
            if (!tmp_fp_env->controls.vd) {
                tmp_fp_env->lo_faults.v = 1;
            }
        }
        if (fp_is_nan(tmp_fr2)) {
            tmp_reg_pair.lo = fp_is_snan(tmp_fr2)
                            ? fp_make_quiet_nan(tmp_fr2) : tmp_fr2;
        } else if (fp_is_nan(tmp_fr3)) {
            tmp_reg_pair.lo = fp_is_snan(tmp_fr3)
                            ? fp_make_quiet_nan(tmp_fr3) : tmp_fr3;
        }
/*******************************************************************************
(f2 and f3 are inf) or (f2 and f3 are 0); returns qnan
********************************************************************************/
   } else if ( ( fp_is_inf(tmp_fr2)  && fp_is_inf(tmp_fr3)  )
            || ( fp_is_zero(tmp_fr2) && fp_is_zero(tmp_fr3) ) ) {
        tmp_fp_env->lo_flags.v = 1;
        tmp_reg_pair.lo        = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->lo_faults.v = 1;
        }
/*******************************************************************************
(f2 is non-zero (normal or denormal but not inf) and f3 is zero; returns inf
The reason for the "but not inf" is because inf/0 should not set the
divide-by-zero flag.
********************************************************************************/
    } else if ( !fp_is_inf(tmp_fr2) && !fp_is_zero(tmp_fr2) && fp_is_zero(tmp_fr3) ) {
        tmp_fp_env->lo_flags.z = 1;
        tmp_reg_pair.lo = FP_INFINITY;
        tmp_reg_pair.lo.sign = tmp_fr2.sign ^ tmp_fr3.sign;
        if (!tmp_fp_env->controls.zd) {
            tmp_fp_env->lo_faults.z = 1;
        }

    } else if (fp_is_unorm(tmp_fr2) || fp_is_unorm(tmp_fr3)) {
        tmp_fp_env->lo_flags.d = 1;
        if (!tmp_fp_env->controls.dd) {
            tmp_fp_env->lo_faults.d = 1;
        }
    } 

    if ( !fp_is_zero(tmp_fr2) && fp_is_finite(tmp_fr2)
      && !fp_is_zero(tmp_fr3) && fp_is_finite(tmp_fr3) ) {

        tmp_fr2 = fp_normalize(tmp_fr2);

        if ( fp_is_unorm(tmp_fr3) ) {
            limits_check->lo_fr3 = 1; /* recip(fr3_lo) not rep. */
            tmp_reg_pair.lo      = FP_INFINITY;          
            tmp_reg_pair.lo.sign        = tmp_fr3.sign;
            tmp_fr3              = fp_normalize(tmp_fr3);

        } else if ( tmp_fr3.exponent >= (FP_REG_BIAS+FP_SGL_BIAS-2) ) {
            limits_check->lo_fr3 = 1; /* recip(fr3_lo) not rep. */
            tmp_reg_pair.lo      = FP_ZERO;             
            tmp_reg_pair.lo.sign        = tmp_fr3.sign;
        }

        estimated_exponent = (EM_int_t)tmp_fr2.exponent
                           - (EM_int_t)tmp_fr3.exponent;

        if (  (estimated_exponent >= (((EM_int_t)(FP_SGL_BIAS)))          )
           || (estimated_exponent <= (2-((EM_int_t)FP_SGL_BIAS))            )
           || (tmp_fr2.exponent   <= (ss_single_24+FP_REG_BIAS-FP_SGL_BIAS) )
           ) {
            limits_check->lo_fr2_or_quot = 1; /* lo est. quot. or fr2_lo */
        }
          
    }   

    return (tmp_reg_pair);
}

// *******************************************************************
// frsqrta_exception_fault_check()
// *******************************************************************
EM_fp_reg_type
frsqrta_exception_fault_check(
     EM_fp_reg_specifier f3,
     EM_opcode_sf_type   sf,
     EM_tmp_fp_env_type  *tmp_fp_env)
{
EM_fp_reg_type tmp_res, fr3;
    
    fr3 = FR[f3];
    fp_decode_environment( pc_none, sf, tmp_fp_env );

    if (fp_software_assistance_required(ps, op_frsqrta, fr3)) {
        tmp_fp_env->em_faults.swa = 1;
        return (FP_ZERO);
    }

    tmp_res = FP_ZERO;

    if (fp_is_unsupported(fr3)) {
        tmp_fp_env->flags.v = 1;
        tmp_res = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
        }

    } else if (fp_is_nan(fr3)) {
        if(fp_is_snan(fr3)){
           tmp_fp_env->flags.v = 1;
           if (!tmp_fp_env->controls.vd)  {
               tmp_fp_env->em_faults.v = 1;
           }
        }
        tmp_res = fp_is_snan(fr3)
                ? fp_make_quiet_nan(fr3) : fr3;

    } else if (fp_is_neg_inf(fr3)) {
        tmp_fp_env->flags.v = 1;
        tmp_res             = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
        }

    } else if ( fp_is_neg_non_zero(fr3) && !fp_is_pseudo_zero(fr3)) {
        tmp_fp_env->flags.v = 1;
        tmp_res             = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->em_faults.v = 1;
        }

 
    } else if (fp_is_unorm(fr3)) {
        tmp_fp_env->flags.d = 1;
        if( !tmp_fp_env->controls.dd) {
           tmp_fp_env->em_faults.d = 1;
        }
    }


    if( (fp_is_pos_non_zero(fr3) && !fp_is_pseudo_zero(fr3) )&& fp_is_finite(fr3)) {
       fr3 = fp_normalize(fp_reg_read(fr3));
       if(fr3.exponent <= ss_double_extended_64) {
          tmp_fp_env->em_faults.swa = 1; 
       } 
    }
 
    return (tmp_res);
}

// *******************************************************************
// fprsqrta_exception_fault_check()
// *******************************************************************
EM_pair_fp_reg_type
fprsqrta_exception_fault_check(
    EM_fp_reg_specifier      f3,
    EM_opcode_sf_type        sf,
    EM_tmp_fp_env_type       *tmp_fp_env, 
    EM_limits_check_fprsqrta *limits_check)
{
EM_pair_fp_reg_type tmp_reg_pair;
EM_fp_reg_type      tmp_fr3 = FR[f3];

    fp_decode_environment( pc_simd, sf, tmp_fp_env );

    tmp_reg_pair.hi = FP_ZERO;
    tmp_reg_pair.lo = FP_ZERO;

// ********
// high
// ********
    tmp_fr3 = fp_reg_read_hi(f3);
    if (fp_software_assistance_required(ps, op_fprsqrta, tmp_fr3)) {
        tmp_fp_env->hi_faults.swa = 1;

    } else if (fp_is_nan(tmp_fr3)) {
        if (fp_is_snan(tmp_fr3)) {
            tmp_fp_env->hi_flags.v = 1;
            if (!tmp_fp_env->controls.vd) {
                tmp_fp_env->hi_faults.v = 1;
            }
        }
        tmp_reg_pair.hi = fp_is_snan(tmp_fr3)
                        ? fp_make_quiet_nan(tmp_fr3) : tmp_fr3;

    } else if (fp_is_neg_inf(tmp_fr3)) {
        tmp_fp_env->hi_flags.v = 1;
        tmp_reg_pair.hi = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->hi_faults.v = 1;
        }
    } else if (fp_is_neg_non_zero(tmp_fr3)) {
        tmp_fp_env->hi_flags.v = 1;
        tmp_reg_pair.hi        = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->hi_faults.v = 1;
        }

    } else if (fp_is_unorm(tmp_fr3)) {
        tmp_fp_env->hi_flags.d = 1;
        if (!tmp_fp_env->controls.dd) {
            tmp_fp_env->hi_faults.d = 1;
        }

    } 
    if(fp_is_pos_non_zero(tmp_fr3) && fp_is_finite(tmp_fr3)) {
       tmp_fr3 = fp_normalize(tmp_fr3);
       if (tmp_fr3.exponent <= (FP_REG_BIAS - FP_SGL_BIAS + ss_single_24)) {
            limits_check->hi = 1;
       } else {
            limits_check->hi = 0;
       }
    }

// ********
// low
// ********
    tmp_fr3 = fp_reg_read_lo(f3);

    if (fp_software_assistance_required(ps, op_fprsqrta, tmp_fr3)) {
        tmp_fp_env->lo_faults.swa = 1;

    } else if (fp_is_nan(tmp_fr3)) {
        if (fp_is_snan(tmp_fr3)) {
            tmp_fp_env->lo_flags.v = 1;
            if (!tmp_fp_env->controls.vd) {
                tmp_fp_env->lo_faults.v = 1;
            }
        }
        tmp_reg_pair.lo = fp_is_snan(tmp_fr3)
                        ? fp_make_quiet_nan(tmp_fr3) : tmp_fr3;

    } else if (fp_is_neg_inf(tmp_fr3)) {
        tmp_fp_env->lo_flags.v = 1;
        tmp_reg_pair.lo = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->lo_faults.v = 1;
        }
    } else if (fp_is_neg_non_zero(tmp_fr3)) {
        tmp_fp_env->lo_flags.v = 1;
        tmp_reg_pair.lo = FP_QNAN;
        if (!tmp_fp_env->controls.vd) {
            tmp_fp_env->lo_faults.v = 1;
        }

    } else if (fp_is_unorm(tmp_fr3)) {
        tmp_fp_env->lo_flags.d = 1;
        if (!tmp_fp_env->controls.dd) {
            tmp_fp_env->lo_faults.d = 1;
        }
    }

    if(fp_is_pos_non_zero(tmp_fr3) && fp_is_finite(tmp_fr3)) {
       tmp_fr3 = fp_normalize(tmp_fr3);
       if (tmp_fr3.exponent <= (FP_REG_BIAS - FP_SGL_BIAS + ss_single_24)) {
            limits_check->lo = 1;
       } else {
            limits_check->lo = 0;
       }
    }


    return (tmp_reg_pair);
}



INLINE EM_boolean_t
fp_is_finite(EM_fp_reg_type freg)
{
    if ( fp_is_inf(freg) || fp_is_nan(freg) || fp_is_unsupported(freg) ) {
        return(0);
    } else {
        return(1);
    }
}

INLINE EM_boolean_t
fp_is_inf(EM_fp_reg_type freg)
{
    if ( (freg.exponent == FP_REG_EXP_ONES)
       &&(freg.significand == U64_0x8000000000000000) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_inf_dp(EM_fp_dp_type tmp_res)
{
    if ( (tmp_res.exponent == FP_DP_EXP_ONES)
       && fp_U128_eq(tmp_res.significand,
                      U128_0x80000000000000000000000000000000) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_nan(EM_fp_reg_type freg)
{
    if (  (freg.exponent == FP_REG_EXP_ONES)
      && ((freg.significand & U64_0x8000000000000000) != 0)
      && ((freg.significand & U64_0x7FFFFFFFFFFFFFFF) != 0) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_nan_dp(EM_fp_dp_type tmp_res)
{
    if ( (tmp_res.exponent == FP_DP_EXP_ONES)
       && fp_U128_eq(U128_0x80000000000000000000000000000000,
                  fp_U128_band(tmp_res.significand,
                            U128_0x80000000000000000000000000000000))
       && !fp_U128_eq(U128_0,
                  fp_U128_band(tmp_res.significand,
                            U128_0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF))
       ) {
        return (1);
    } else {
        return (0);
    }
}

INLINE EM_boolean_t
fp_is_natval(EM_fp_reg_type freg)
{
    if ( (freg.sign == 0)
     && (freg.exponent == 0x1FFFE)
     && (freg.significand == U64_0) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_neg_dp(EM_fp_dp_type tmp_res)
{
    if (tmp_res.sign) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_neg_inf(EM_fp_reg_type freg)
{
    if ( (freg.sign == 1)
      && (freg.exponent == FP_REG_EXP_ONES)
      && (freg.significand == U64_0x8000000000000000) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_neg_non_zero(EM_fp_reg_type freg)
{
    if ( (freg.sign == 1) && !fp_is_zero(freg) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_normal(EM_fp_reg_type freg)
{
    if ( (freg.exponent != 0)
      && (freg.exponent != FP_REG_EXP_ONES)
      && ((freg.significand & U64_0x8000000000000000) != 0) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_normal_dp(EM_fp_dp_type tmp_res)
{
    if ( (tmp_res.exponent != 0)
      && (tmp_res.exponent != FP_DP_EXP_ONES)
      && fp_U128_eq(U128_0x80000000000000000000000000000000,
                 fp_U128_band(tmp_res.significand,
                           U128_0x80000000000000000000000000000000))
      ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_pos_dp(EM_fp_dp_type tmp_res)
{
    if (!tmp_res.sign) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_pos_inf(EM_fp_reg_type freg)
{
    if ( (freg.sign == 0)
      && (freg.exponent == FP_REG_EXP_ONES)
      && (freg.significand == U64_0x8000000000000000) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_pos_non_zero(EM_fp_reg_type freg)
{
    if ( (freg.sign == 0) && !fp_is_zero(freg) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_pseudo_zero(EM_fp_reg_type freg)
{
    if ( (freg.exponent != 0)
      && (freg.exponent != FP_REG_EXP_ONES)
      && (freg.significand == U64_0 && !fp_is_natval (freg)) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_qnan(EM_fp_reg_type freg)
{
    if ( (freg.exponent == FP_REG_EXP_ONES)
      &&((freg.significand & U64_0xC000000000000000) == U64_0xC000000000000000) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_snan(EM_fp_reg_type freg)
{
    if ( (freg.exponent == FP_REG_EXP_ONES)
      &&((freg.significand & U64_0xC000000000000000) == U64_0x8000000000000000)
      &&((freg.significand & U64_0x3FFFFFFFFFFFFFFF) != 0) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_unorm(EM_fp_reg_type freg)
{
    if ( ( (freg.exponent != 0)
        && (freg.exponent != FP_REG_EXP_ONES)
        &&((freg.significand & U64_0x8000000000000000) == 0) )
    /* double-extended pseudo-denormal or double-extended denormal */
      || ( (freg.exponent == 0) && (freg.significand != 0) ) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_unorm_dp(EM_fp_dp_type tmp_res)
{
    if ( (tmp_res.exponent != 0)
      && (tmp_res.exponent != FP_DP_EXP_ONES)
      &&((tmp_res.significand.hi & U64_0x8000000000000000) == 0) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_unsupported(EM_fp_reg_type freg)
{    
    if (  fp_is_natval(freg) || fp_is_nan(freg)   || fp_is_inf(freg)
       || fp_is_normal(freg) || fp_is_unorm(freg) || fp_is_zero(freg) ) {
        return(0);
    } else {
        return(1);
    }
}

INLINE EM_boolean_t
fp_is_unsupported_dp(EM_fp_dp_type tmp_res)
{
    if (  fp_is_nan_dp(tmp_res)    || fp_is_inf_dp(tmp_res)
       || fp_is_normal_dp(tmp_res) || fp_is_unorm_dp(tmp_res)
       || fp_is_zero_dp(tmp_res) ) {
        return(0);
    } else {
        return(1);
    }
}

INLINE EM_boolean_t
fp_is_zero(EM_fp_reg_type freg)
{
    if ( (freg.exponent == 0) && (freg.significand == U64_0) ) {
        return(1);
    } else {
        return(0);
    }
}

INLINE EM_boolean_t
fp_is_zero_dp(EM_fp_dp_type tmp_res)
{
    if ( (tmp_res.exponent == 0) && fp_U128_eq(tmp_res.significand, U128_0) ) {
        return(1);
    } else {
        return(0);
    }
}

EM_int_t
fp82_fp_U64_lead0(EM_uint64_t value)
{
EM_int_t tmp_i, offset=0;
EM_uint64_t tmp_mask;

    if( value == U64_0) 
       return(64);

    tmp_mask = U64_0x8000000000000000;

    if( (value & U64_0xFFFFFFFF00000000) != U64_0) {

       if( (value & U64_0xFFFF000000000000) != U64_0) {

         if( (value & U64_0xFF00000000000000) != U64_0) {
            for (tmp_i=0; tmp_i<8; tmp_i++, tmp_mask>>=1) {
               if ( (value & tmp_mask) != U64_0 ) {
                  return(tmp_i);
               }
            }
         }
         else { /* 0x00FF000000000000 */
            value  <<= 8;
            offset  += 8;
            for (tmp_i=0; tmp_i<8; tmp_i++, tmp_mask>>=1) {
               if ( (value & tmp_mask) != U64_0 ) {
                  return(tmp_i + offset);
               }
            }
         }
      } 
      else { /* 0x0000FFFF00000000 */
         value   <<= 16;
         offset   += 16;
         if( (value & U64_0xFF00000000000000) != U64_0) {
            for (tmp_i=0; tmp_i<8; tmp_i++, tmp_mask>>=1) {
               if ( (value & tmp_mask) != U64_0 ) {
                  return(tmp_i + offset);
               }
            }
         }
         else {
            value  <<= 8;
            offset  += 8;
            for (tmp_i=0; tmp_i<8; tmp_i++, tmp_mask>>=1) {
               if ( (value & tmp_mask) != U64_0 ) {
                  return(tmp_i + offset);
               }
            }
         }
      }
   }
   else { /*  0x00000000 FFFFFFFF */
      value  <<= 32;
      offset  += 32;
       if( (value & U64_0xFFFF000000000000) != U64_0) {

         if( (value & U64_0xFF00000000000000) != U64_0) {
            for (tmp_i=0; tmp_i<8; tmp_i++, tmp_mask>>=1) {
               if ( (value & tmp_mask) != U64_0 ) {
                  return(tmp_i + offset);
               }
            }
         }
         else { /* 0x00000000 00FF0000 */
            value  <<= 8;
            offset  += 8;
            for (tmp_i=0; tmp_i<8; tmp_i++, tmp_mask>>=1) {
               if ( (value & tmp_mask) != U64_0 ) {
                  return(tmp_i + offset);
               }
            }
         }
      } 
      else {   /* 0x00000000 0000FFFF */
         value   <<= 16;
         offset   += 16;
         if( (value & U64_0xFF00000000000000) != U64_0) {
            for (tmp_i=0; tmp_i<8; tmp_i++, tmp_mask>>=1) {
               if ( (value & tmp_mask) != U64_0 ) {
                  return(tmp_i + offset);
               }
            }
         }
         else {
            value  <<= 8;
            offset  += 8;
            for (tmp_i=0; tmp_i<8; tmp_i++, tmp_mask>>=1) {
               if ( (value & tmp_mask) != U64_0 ) {
                  return(tmp_i + offset);
               }
            }
         }
      }
   }

   return(64); // MACH ADDED

}

EM_int_t
fp_U128_lead0(EM_uint128_t value)
{
    EM_int_t tmp_i;

    tmp_i = fp_U64_lead0(value.hi);
    if (tmp_i == 64) {
        tmp_i += fp_U64_lead0(value.lo);
    }
    return(tmp_i);
}

EM_int_t
fp82_fp_U256_lead0(EM_uint256_t value)
{
    EM_int_t tmp_i;

    tmp_i = fp_U64_lead0(value.hh);
    if (tmp_i == 64) {
        tmp_i += fp_U64_lead0(value.hl);
        if (tmp_i == 128) {
            tmp_i += fp_U64_lead0(value.lh);
            if (tmp_i == 192) {
                tmp_i += fp_U64_lead0(value.ll);
            }
        }
    }
    return(tmp_i);
}

// *******************************************************************
// fp_mem_to_fr_format()
// *******************************************************************
EM_fp_reg_type
fp_mem_to_fr_format(
    EM_memory_type mem,
    EM_uint_t      size,
    EM_uint_t      integer_form)
{

/******************************************************
integer_form = 0           floating point
integer_form = 1           simd, integer
*******************************************************/

    EM_fp_reg_type tmp_freg;
    EM_uint64_t    tmp_significand, tmp_significand_hi, tmp_significand_lo;

    switch (size) {
        case 4:/* single */
            tmp_freg.sign = mem.fp_single.sign;
            if ( (mem.fp_single.exponent    == 0)
              && (mem.fp_single.significand == 0) ) { /* zero */
                tmp_freg.exponent = 0;
            } else if (mem.fp_single.exponent == 0) { /* denormal */
                tmp_freg.exponent = (EM_uint_t)(FP_REG_BIAS - FP_SGL_BIAS + 1);
            } else if (mem.fp_single.exponent == FP_SGL_EXP_ONES) { /* Inf, NaN, NaTVal */
                tmp_freg.exponent = FP_REG_EXP_ONES;
            } else {
                tmp_freg.exponent = (EM_uint_t)
                                  (((EM_int_t)mem.fp_single.exponent)
                                  - FP_SGL_BIAS + FP_REG_BIAS);
            }
            tmp_freg.significand =
                    (((EM_uint64_t)mem.fp_single.significand)<<40)
#ifdef HPC_BUGS
                    | (((mem.fp_single.exponent != U64_0)?U64_1:U64_0)<<63);
#else
                    | (((mem.fp_single.exponent != 0)?U64_1:U64_0)<<63);
#endif

            break;

        case 8: /* double */
            if (integer_form) {
                tmp_freg.sign = 0;
                tmp_freg.significand = mem.uint_64.uvalue;
                tmp_freg.exponent    = FP_INTEGER_EXP;
            } else {
                tmp_freg.sign = mem.fp_double.sign;
                if ( (mem.fp_double.exponent    == 0)
                  && (mem.fp_double.significand_hi == 0) 
                  && (mem.fp_double.significand_lo == 0) ){    /* zero */
                    tmp_freg.exponent = 0;
                } else if (mem.fp_double.exponent == 0) {     /* denormal */
                     tmp_freg.exponent = (EM_uint_t)(FP_REG_BIAS - FP_DBL_BIAS + 1);
                } else if (mem.fp_double.exponent == FP_DBL_EXP_ONES) { /* Inf, NaN, NaTVal */
                    tmp_freg.exponent = FP_REG_EXP_ONES;
                } else {
                    tmp_freg.exponent = (EM_uint_t)
                                      (((EM_int_t)mem.fp_double.exponent)
                                      - FP_DBL_BIAS + FP_REG_BIAS);
                }
                tmp_significand_lo = ((EM_uint64_t)(mem.fp_double.significand_lo)) ;
                tmp_significand_hi = (((EM_uint64_t)(mem.fp_double.significand_hi)) << 32);
                tmp_significand = tmp_significand_lo | tmp_significand_hi;

                tmp_freg.significand =
                         (tmp_significand<<11)
#ifdef HPC_BUGS
                     | (((mem.fp_double.exponent != U64_0)?U64_1:U64_0)<<63);
#else
                     | (((mem.fp_double.exponent != 0)?U64_1:U64_0)<<63);
#endif
            }
            break;

        case 10: /* double extended */
            tmp_freg.sign = mem.fp_double_extended.sign;
            if (mem.fp_double_extended.exponent == 0) {
                    /* Zero or (Pseudo-)Denormal */
                tmp_freg.exponent = 0;
            } else if (mem.fp_double_extended.exponent == FP_EXT_EXP_ONES) {
                    /* Inf, NaN, NaTVal */
                tmp_freg.exponent = FP_REG_EXP_ONES;
            } else { /* Normal */
                tmp_freg.exponent = (EM_uint_t)
                                  (((EM_int_t)mem.fp_double_extended.exponent)
                                  - FP_EXT_BIAS + FP_REG_BIAS);
            }
	    memcpy(&tmp_freg.significand,
		    mem.fp_double_extended.significand, 8);
            break;

        case 16: /* fill */
            tmp_freg.sign        = mem.fp_spill_fill.sign;
            tmp_freg.exponent    = mem.fp_spill_fill.exponent;
            tmp_freg.significand = mem.fp_spill_fill.significand;
            break;
    }
    return (tmp_freg);
}


INLINE EM_fp_reg_type
fp_make_quiet_nan(EM_fp_reg_type freg)
{
    freg.significand |= U64_0x4000000000000000;
    return (freg);
}



EM_boolean_t
fp82_fp_raise_fault(EM_tmp_fp_env_type tmp_fp_env)
{
    if(tmp_fp_env.simd == 1) {
         if  (tmp_fp_env.lo_faults.swa || tmp_fp_env.lo_faults.v
         ||   tmp_fp_env.lo_faults.d   || tmp_fp_env.lo_faults.z 

         ||   tmp_fp_env.hi_faults.swa || tmp_fp_env.hi_faults.v 
         ||   tmp_fp_env.hi_faults.d   || tmp_fp_env.hi_faults.z )

          return(1);
    } else if ( tmp_fp_env.em_faults.swa || tmp_fp_env.em_faults.v
         ||   tmp_fp_env.em_faults.d   || tmp_fp_env.em_faults.z )
          return (1);
    return (0);
}


EM_boolean_t
fp82_fp_raise_traps(EM_tmp_fp_env_type tmp_fp_env)
{
    if(tmp_fp_env.simd == 1) {
       if   (tmp_fp_env.hi_traps.o || tmp_fp_env.hi_traps.un || tmp_fp_env.hi_traps.i
       ||    tmp_fp_env.lo_traps.o || tmp_fp_env.lo_traps.un || tmp_fp_env.lo_traps.i) // MACH
        return (1);
    } else if (tmp_fp_env.em_traps.o || tmp_fp_env.em_traps.un || tmp_fp_env.em_traps.i) // MACH
        return (1);
    return (0);
}


INLINE EM_fp_reg_type
fp_reg_read(EM_fp_reg_type freg)
{
    EM_fp_reg_type tmp_freg;
    tmp_freg = freg;
 /* insert true register file exponent for double-extended (pseudo-)denormal */
    if ((tmp_freg.exponent == 0) && (tmp_freg.significand != U64_0))
        tmp_freg.exponent=0x0C001;
    return (tmp_freg);
}



// *******************************************************************
// fp_update_fpsr()
// *******************************************************************
INLINE void
fp_update_fpsr(
    EM_opcode_sf_type  sf,
    EM_tmp_fp_env_type tmp_fp_env )
{
    if (sf == sf_none) {
        return;
    }
    else if (sf == sfS0) {

/*******************************************************************
SF0
*******************************************************************/
       if(tmp_fp_env.simd == 1) {

/* SF0 simd fault: if either hi or low is set, set the s0 flag */
         if (tmp_fp_env.hi_flags.v || tmp_fp_env.lo_flags.v) {
            SET_STATUS_FLAG(FPSR.sf0_flags_v);
         }
         if (tmp_fp_env.hi_flags.d || tmp_fp_env.lo_flags.d) {
            SET_STATUS_FLAG(FPSR.sf0_flags_d);
         }
         if (tmp_fp_env.hi_flags.z || tmp_fp_env.lo_flags.z) {
            SET_STATUS_FLAG(FPSR.sf0_flags_z);
         }

/* SF0 simd trap: if either hi or low is set, set the s0 flag 
if the flag is over or underflow, also set inexact  */
         if (tmp_fp_env.hi_flags.o || tmp_fp_env.lo_flags.o) {
            SET_STATUS_FLAG(FPSR.sf0_flags_o);
         }
         if (tmp_fp_env.hi_flags.un || tmp_fp_env.lo_flags.un) { // MACH
            SET_STATUS_FLAG(FPSR.sf0_flags_u);
         }
         if (tmp_fp_env.hi_flags.i || tmp_fp_env.lo_flags.i) {
            SET_STATUS_FLAG(FPSR.sf0_flags_i);
         }
      } /* end of simd */

      else { /* not simd */

/* SF0 non-simd fault: if tmp flag is set and s0 flag is not, set the flag */
         if (tmp_fp_env.flags.v) {
            SET_STATUS_FLAG(FPSR.sf0_flags_v);
         }
         if (tmp_fp_env.flags.d) {
// printf ("MACH DEBUG: setting the D flag in update_fpsr ()\n");
            SET_STATUS_FLAG(FPSR.sf0_flags_d);
         }
         if (tmp_fp_env.flags.z) {
            SET_STATUS_FLAG(FPSR.sf0_flags_z);
         }
   
/* SF0 non-simd trap: if tmp flag is set, set the flag.
if the flag is over or underflow, also check inexact  */
         if (tmp_fp_env.flags.o) {
            SET_STATUS_FLAG(FPSR.sf0_flags_o);
            if ( tmp_fp_env.flags.i) {
               SET_STATUS_FLAG(FPSR.sf0_flags_i);
            }
          }
          else if (tmp_fp_env.flags.un) { // MACH
             SET_STATUS_FLAG(FPSR.sf0_flags_u);
             if ( tmp_fp_env.flags.i )  {
                SET_STATUS_FLAG(FPSR.sf0_flags_i);
             }
          }
          else if (tmp_fp_env.flags.i) {
             SET_STATUS_FLAG(FPSR.sf0_flags_i);
          }
       } /* end of not simd */
    } /* end of SF0 */

/*******************************************************************
SF1
*******************************************************************/
    else if (sf == sfS1) {
        if(tmp_fp_env.simd == 1) {

/* SF1 simd fault: if either hi or low is set, set the s1 flag
*/
           if (tmp_fp_env.hi_flags.v || tmp_fp_env.lo_flags.v) {
              SET_STATUS_FLAG(FPSR.sf1_flags_v);
           }
           if (tmp_fp_env.hi_flags.d || tmp_fp_env.lo_flags.d) {
              SET_STATUS_FLAG(FPSR.sf1_flags_d);
           }
           if (tmp_fp_env.hi_flags.z || tmp_fp_env.lo_flags.z) {
              SET_STATUS_FLAG(FPSR.sf1_flags_z);
           }

/* SF1 simd trap: if either hi or low is set and the s1 flag is not, set the s1 flag 
If the flag is over or underflow, also check inexact  */
           if (tmp_fp_env.hi_flags.o || tmp_fp_env.lo_flags.o) {
              SET_STATUS_FLAG(FPSR.sf1_flags_o);
           }
           if (tmp_fp_env.hi_flags.un || tmp_fp_env.lo_flags.un) { // MACH
              SET_STATUS_FLAG(FPSR.sf1_flags_u);
           }
           if (tmp_fp_env.hi_flags.i || tmp_fp_env.lo_flags.i) {
                SET_STATUS_FLAG(FPSR.sf1_flags_i);
            }
        } /* end of simd SF1 */

        else { /* not simd SF1 */

/* SF1 non-simd fault: if tmp flag is set and s1 flag is not, set the flag 
*/
           if (tmp_fp_env.flags.v ) {
              SET_STATUS_FLAG(FPSR.sf1_flags_v);
           }
           if (tmp_fp_env.flags.d ) {
              SET_STATUS_FLAG(FPSR.sf1_flags_d);
           }
           if (tmp_fp_env.flags.z ) {
              SET_STATUS_FLAG(FPSR.sf1_flags_z);
           }
    
/* SF1 non-simd traps: if tmp flag is set and s1 flag is not, set the flag.
if the flag is over or underflow, also check inexact  */
           if ( tmp_fp_env.flags.o ) {
              SET_STATUS_FLAG(FPSR.sf1_flags_o);
              if ( tmp_fp_env.flags.i ) {
                 SET_STATUS_FLAG(FPSR.sf1_flags_i);
              }
           }
           else if (tmp_fp_env.flags.un ) { // MACH
              SET_STATUS_FLAG(FPSR.sf1_flags_u);
              if ( tmp_fp_env.flags.i ) {
                 SET_STATUS_FLAG(FPSR.sf1_flags_i);
              }
           }
           else if (tmp_fp_env.flags.i ) {
              SET_STATUS_FLAG(FPSR.sf1_flags_i);
           }
        } /*end of not simd  SF1 */
      } /* end of SF1 */

/*******************************************************************
SF2
*******************************************************************/
      else if (sf == sfS2) {
         if(tmp_fp_env.simd == 1) {

/* SF2 simd fault: if either hi or low is set and the s2 flag is not, set the s2 flag
*/
            if (tmp_fp_env.hi_flags.v || tmp_fp_env.lo_flags.v) {
               SET_STATUS_FLAG(FPSR.sf2_flags_v);
            }
            if (tmp_fp_env.hi_flags.d || tmp_fp_env.lo_flags.d) {
               SET_STATUS_FLAG(FPSR.sf2_flags_d);
            }
            if (tmp_fp_env.hi_flags.z || tmp_fp_env.lo_flags.z) {
               SET_STATUS_FLAG(FPSR.sf2_flags_z);
            }

/* SF2 simd trap: if either hi or low is set and the s2 flag is not, set the s2 flag 
If the flag is over or underflow, also check inexact  */
            if (tmp_fp_env.hi_flags.o || tmp_fp_env.lo_flags.o) {
                SET_STATUS_FLAG(FPSR.sf2_flags_o);
            }
            if (tmp_fp_env.hi_flags.un || tmp_fp_env.lo_flags.un) { // MACH
               SET_STATUS_FLAG(FPSR.sf2_flags_u);
            }
            if (tmp_fp_env.hi_flags.i || tmp_fp_env.lo_flags.i) {
                SET_STATUS_FLAG(FPSR.sf2_flags_i);
            }
         } /* end of simd SF2 */

         else { /* not simd SF2 */

/* SF2 non-simd fault: if tmp flag is set and s2 flag is not, set the flag 
*/
            if (tmp_fp_env.flags.v ) {
               SET_STATUS_FLAG(FPSR.sf2_flags_v);
            }
            if (tmp_fp_env.flags.d ) {
               SET_STATUS_FLAG(FPSR.sf2_flags_d);
            }
            if (tmp_fp_env.flags.z ) {
               SET_STATUS_FLAG(FPSR.sf2_flags_z);
            }

/* SF2 non-simd traps: if tmp flag is set and s2 flag is not, set the flag.
if the flag is over or underflow, also check inexact  */
            if ( tmp_fp_env.flags.o ) {
               SET_STATUS_FLAG(FPSR.sf2_flags_o);
               if ( tmp_fp_env.flags.i ) {
                  SET_STATUS_FLAG(FPSR.sf2_flags_i);
               }
            }
            else if (tmp_fp_env.flags.un ) { // MACH
               SET_STATUS_FLAG(FPSR.sf2_flags_u);
               if ( tmp_fp_env.flags.i ) {
                    SET_STATUS_FLAG(FPSR.sf2_flags_i);
               }
            }
            else if (tmp_fp_env.flags.i ) {
               SET_STATUS_FLAG(FPSR.sf2_flags_i);
            }
       } /* end of not simd SF2 */
    } /* end of SF2 */

/*******************************************************************
SF3
*******************************************************************/
    else if (sf == sfS3) {
       if(tmp_fp_env.simd == 1) {

/* SF3 simd fault: if either hi or low is set and the s3 flag is not, set the s3 flag
*/
          if (tmp_fp_env.hi_flags.v || tmp_fp_env.lo_flags.v) {
             SET_STATUS_FLAG(FPSR.sf3_flags_v);
          }
          if (tmp_fp_env.hi_flags.d || tmp_fp_env.lo_flags.d) {
             SET_STATUS_FLAG(FPSR.sf3_flags_d);
          }
          if (tmp_fp_env.hi_flags.z || tmp_fp_env.lo_flags.z) {
             SET_STATUS_FLAG(FPSR.sf3_flags_z);
          }

/* SF3 simd trap: if either hi or low is set and the s3 flag is not, set the s3 flag 
If the flag is over or underflow, also check inexact  */
          if (tmp_fp_env.hi_flags.o || tmp_fp_env.lo_flags.o) {
             SET_STATUS_FLAG(FPSR.sf3_flags_o);
          }
          if (tmp_fp_env.hi_flags.un || tmp_fp_env.lo_flags.un) { // MACH
             SET_STATUS_FLAG(FPSR.sf3_flags_u);
          }
          if (tmp_fp_env.hi_flags.i || tmp_fp_env.lo_flags.i) {
             SET_STATUS_FLAG(FPSR.sf3_flags_i);
          }
       } /* end of simd SF3 */

       else { /* not simd SF3 */

/* SF3 non-simd fault: if tmp flag is set and s3 flag is not, set the flag 
 */
          if (tmp_fp_env.flags.v ) {
             SET_STATUS_FLAG(FPSR.sf3_flags_v);
          }
          if (tmp_fp_env.flags.d ) {
             SET_STATUS_FLAG(FPSR.sf3_flags_d);
          }
          if (tmp_fp_env.flags.z ) {
             SET_STATUS_FLAG(FPSR.sf3_flags_z);
          }
    
/* SF3 non-simd traps: if tmp flag is set and s3 flag is not, set the flag.
if the flag is over or underflow, also check inexact  */
          if ( tmp_fp_env.flags.o ) {
             SET_STATUS_FLAG(FPSR.sf3_flags_o);
             if ( tmp_fp_env.flags.i ) {
                SET_STATUS_FLAG(FPSR.sf3_flags_i);
             }
          }
          else if (tmp_fp_env.flags.un ) { // MACH
             SET_STATUS_FLAG(FPSR.sf3_flags_u);
             if ( tmp_fp_env.flags.i ) {
                SET_STATUS_FLAG(FPSR.sf3_flags_i);
             }
          }
          else if (tmp_fp_env.flags.i ) {
             SET_STATUS_FLAG(FPSR.sf3_flags_i);
          }
       } /* end of not simd SF3 */
    } /* end of SF3 */
} /* end of fp_update_fpsr */

INLINE void
fp_update_psr(EM_uint_t dest_freg)
{
EM_uint_t disabled_limit = 31;

    if ( (dest_freg >= 2) && (dest_freg <= disabled_limit) ){
        SET_STATUS_FLAG(PSR.mfl);
    }
    else if ( (dest_freg > disabled_limit) ) {
        SET_STATUS_FLAG(PSR.mfh);
    }
}


/* EM_int64_t, EM_uint64_t, EM_uint128_t and EM_uint256_t support routines */

/* 128-bit unsigned int support routines */

EM_boolean_t
fp82_fp_U128_eq(EM_uint128_t value1, EM_uint128_t value2)
{
    if ( (value1.hi == value2.hi)
      && (value1.lo == value2.lo) )
        return (1);
    else
        return (0);
}

static INLINE EM_boolean_t
fp_U128_ge(EM_uint128_t value1, EM_uint128_t value2)
{
    if (value1.hi >  value2.hi)
        return (1);
    else if ( (value1.hi == value2.hi)
           && (value1.lo >= value2.lo) )
        return (1);
    else
        return (0);
}

static INLINE EM_boolean_t
fp_U128_gt(EM_uint128_t value1, EM_uint128_t value2)
{
    if (value1.hi > value2.hi)
        return (1);
    else if ( (value1.hi == value2.hi)
           && (value1.lo >  value2.lo) )
        return (1);
    else
        return (0);
}

static INLINE EM_boolean_t
fp_U128_le(EM_uint128_t value1, EM_uint128_t value2)
{
    if (value1.hi <  value2.hi)
        return (1);
    else if ( (value1.hi == value2.hi)
           && (value1.lo <= value2.lo) )
        return (1);
    else
        return (0);
}

EM_boolean_t
fp82_fp_U128_lt(EM_uint128_t value1, EM_uint128_t value2)
{
    if (value1.hi < value2.hi)
        return (1);
    else if ( (value1.hi == value2.hi)
           && (value1.lo <  value2.lo) )
        return (1);
    else
        return (0);
}

EM_uint128_t
fp82_fp_U128_lsh(EM_uint128_t value, EM_uint_t count)
{
EM_uint128_t tmp;

    if (count == 0) {
        return(value);
    } else if (count >= 128) {
        return (U128_0);

    } else if (count >  64) {
        tmp.lo  = U64_0;
        tmp.hi = (value.lo<<(count-64));
        return (tmp);
    } else if (count == 64) {
        tmp.lo  = U64_0;
        tmp.hi = value.lo;
        return (tmp);
    } else if (count >  0) {
        tmp.lo = (value.lo<<count);
        tmp.hi = (value.hi<<count) | (value.lo>>(64-count)) ;
        return (tmp);
    }
   return(value); // MACH ADDED
}

EM_uint128_t
fp82_fp_U128_rsh(EM_uint128_t value, EM_uint_t count)
{
EM_uint128_t tmp;

    if (count == 0) {
        return (value);
    } else if (count >= 128) {
        return (U128_0);

    } else if (count > 64) {
        tmp.lo = (value.hi>>(count-64));
        tmp.hi  = U64_0;
        return (tmp);
    } else if (count == 64) {
        tmp.lo = value.hi;
        tmp.hi  = U64_0;
        return (tmp);
    } else if (count >  0) {
        tmp.lo = (value.lo>>count) | (value.hi<<(64-count));
        tmp.hi = (value.hi>>count);
        return (tmp);
    } 
    return(U128_0); // MACH ADDED
}

EM_uint128_t
fp82_fp_U64_x_U64_to_U128(EM_uint64_t value1, EM_uint64_t value2)
{
    EM_uint128_t tmp_res;
    EM_uint64_t r0, s0, t0;
    EM_uint64_t r1, s1, t1;

    s0 = (value1<<32)>>32;
    s1 = (value1>>32);

    t0 = (value2<<32)>>32;
    t1 = (value2>>32);

#ifdef HPC_BUGS
    s0 = ((EM_uint64_t)( ( ( ((EM_int64_t)s0) << 32 ) >> 32 ) ));
    s1 = ((EM_uint64_t)( ( ( ((EM_int64_t)s1) << 32 ) >> 32 ) ));
    t0 = ((EM_uint64_t)( ( ( ((EM_int64_t)t0) << 32 ) >> 32 ) ));
    t1 = ((EM_uint64_t)( ( ( ((EM_int64_t)t1) << 32 ) >> 32 ) ));
#endif

    tmp_res.lo = s0 * t0;

#ifdef HPC_BUGS
    if(s0 & U64_0x0000000080000000)
        tmp_res.lo += t0<<32;
    if(t0 & U64_0x0000000080000000)
        tmp_res.lo += s0<<32;
#endif


    r0 = s0 * t1;

#ifdef HPC_BUGS
    if(s0 & U64_0x0000000080000000)
        r0 += t1<<32;
    if(t1 & U64_0x0000000080000000)
        r0 += s0<<32;
#endif


    r1 = s1 * t0;

#ifdef HPC_BUGS
    if(s1 & U64_0x0000000080000000)
        r1 += t0<<32;
    if(t0 & U64_0x0000000080000000)
        r1 += s1<<32;
#endif


    tmp_res.hi = s1 * t1;

#ifdef HPC_BUGS
    if(s1 & U64_0x0000000080000000)
        tmp_res.hi += t1<<32;
    if(t1 & U64_0x0000000080000000)
        tmp_res.hi += s1<<32;
#endif


    if ( (tmp_res.lo + (r0<<32)) < tmp_res.lo)
        tmp_res.hi++;

    tmp_res.lo += (r0<<32);

    if ( (tmp_res.lo + (r1<<32)) < tmp_res.lo)
        tmp_res.hi++;

    tmp_res.lo += (r1<<32);
    tmp_res.hi += (r0>>32);
    tmp_res.hi += (r1>>32);

    return (tmp_res);
}

INLINE EM_uint128_t
fp_I64_x_I64_to_I128(EM_uint64_t value1, EM_uint64_t value2)
{
    EM_uint128_t tmp_res;
    EM_uint128_t scratch;

    tmp_res = fp_U64_x_U64_to_U128(value1, value2);

    if (value1 & U64_0x8000000000000000) {
        scratch = fp_U64_to_U128(value2);
        scratch = fp_U128_lsh(scratch,64);
        scratch = fp_U128_neg(scratch);
        tmp_res = fp_U128_add(scratch, tmp_res);
    } 

    if (value2 & U64_0x8000000000000000) {
        scratch = fp_U64_to_U128(value1);
        scratch = fp_U128_lsh(scratch,64);
        scratch = fp_U128_neg(scratch);
        tmp_res = fp_U128_add(scratch, tmp_res);
    } 

    return (tmp_res);
}

EM_uint128_t
fp82_fp_U128_inc(EM_uint128_t value)
{
    EM_uint128_t tmp;
            /* add one */
    tmp.lo = value.lo + 1;
    tmp.hi = value.hi + (tmp.lo < value.lo);

    return (tmp);
}

static INLINE EM_uint128_t
fp_U128_neg(EM_uint128_t value)
{
    EM_uint128_t tmp;

            /* complement */
    value.lo = ~value.lo;
    value.hi = ~value.hi;
            /* add one */
    tmp.lo = value.lo + 1;
    tmp.hi = value.hi + (tmp.lo < value.lo);
    return (tmp);
}

EM_uint128_t
fp82_fp_U128_add(EM_uint128_t value1,
                 EM_uint128_t value2)
{
    EM_uint128_t tmp;

        /* sum  */
    value2.lo = value1.lo + value2.lo;
    value2.hi = value1.hi + value2.hi;
        /* carry */
    tmp.lo = 0;
    tmp.hi = (value2.lo < value1.lo);

        /* carry propagate adder */
    tmp.lo =  value2.lo;
    tmp.hi += value2.hi;
    return (tmp);
}

EM_uint128_t
fp82_fp_U128_bor(EM_uint128_t value1, EM_uint128_t value2)
{
    EM_uint128_t tmp_res;
    tmp_res.lo = value1.lo | value2.lo;
    tmp_res.hi = value1.hi | value2.hi;
    return (tmp_res);
}

EM_uint128_t
fp82_fp_U128_band(EM_uint128_t value1, EM_uint128_t value2)
{
    EM_uint128_t tmp_res;
    tmp_res.lo = value1.lo & value2.lo;
    tmp_res.hi = value1.hi & value2.hi;
    return (tmp_res);
}

/* 256-bit unsigned int support routines */

EM_boolean_t
fp82_fp_U256_eq(EM_uint256_t value1, EM_uint256_t value2)
{
    if ( (value1.hh == value2.hh)
      && (value1.hl == value2.hl )
      && (value1.lh == value2.lh )
      && (value1.ll == value2.ll ) )
        return (1);
    else
        return (0);
}


EM_uint256_t
fp82_fp_U256_lsh(EM_uint256_t value, EM_uint_t count)
{
EM_uint256_t tmp;
    if (count == 0) {
        return (value);
    } else if (count >= 256) {
        return (U256_0);

    } else if (count >  192) {
        tmp.ll  = U64_0;
        tmp.lh  = U64_0;
        tmp.hl  = U64_0;
        tmp.hh = (value.ll<<(count-192));
        return (tmp);
    } else if (count == 192) {
        tmp.ll  = U64_0;
        tmp.lh  = U64_0;
        tmp.hl  = U64_0;
        tmp.hh = value.ll;
        return (tmp);
    } else if (count >  128) {
        tmp.ll  = U64_0;
        tmp.lh  = U64_0;
        tmp.hl = (value.ll<<(count-128));
        tmp.hh = (value.lh<<(count-128)) | (value.ll>>(192-count));
        return (tmp);
    } else if (count == 128) {
        tmp.ll  = U64_0;
        tmp.lh  = U64_0;
        tmp.hl = value.ll;
        tmp.hh = value.lh;
        return (tmp);
    } else if (count >  64) {
        tmp.ll  = U64_0;
        tmp.lh = (value.ll<<(count-64));
        tmp.hl = (value.lh<<(count-64)) | (value.ll>>(128-count)) ;
        tmp.hh = (value.hl<<(count-64)) | (value.lh>>(128-count)) ;
        return (tmp);
    } else if (count == 64) {
        tmp.ll = 0;
        tmp.lh = value.ll;
        tmp.hl = value.lh;
        tmp.hh = value.hl;
        return (tmp);
    } else if (count >  0) {
        tmp.ll = (value.ll<<count);
        tmp.lh = (value.lh<<count) | (value.ll>>(64-count)) ;
        tmp.hl = (value.hl<<count) | (value.lh>>(64-count)) ;
        tmp.hh = (value.hh<<(count)) | (value.hl>>(64-count)) ;
        return (tmp);
    } 
    return(U256_0); // MACH ADDED

}

EM_uint256_t
fp82_fp_U256_rsh(EM_uint256_t value, EM_uint_t count)
{
EM_uint256_t tmp;
    if (count == 0) {
        return (value);
    } else if (count >= 256) {
        return (U256_0);

    } else if (count >  192) {
        tmp.ll = (value.hh>>(count-192));
        tmp.lh  = U64_0;
        tmp.hl  = U64_0;
        tmp.hh  = U64_0;
        return (tmp);
    } else if (count == 192) {
        tmp.ll = value.hh;
        tmp.lh  = U64_0;
        tmp.hl  = U64_0;
        tmp.hh  = U64_0;
        return (tmp);
    } else if (count >  128) {
        tmp.ll = (value.hl>>(count-128)) | (value.hh<<(192-count));
        tmp.lh = (value.hh>>(count-128));
        tmp.hl  = U64_0;
        tmp.hh  = U64_0;
        return (tmp);
    } else if (count == 128) {
        tmp.ll = value.hl;
        tmp.lh = value.hh;
        tmp.hl  = U64_0;
        tmp.hh  = U64_0;
        return (tmp);
    } else if (count >  64) {
        tmp.ll = (value.lh>>(count-64)) | (value.hl<<(128-count));
        tmp.lh = (value.hl>>(count-64)) | (value.hh<<(128-count));
        tmp.hl = (value.hh>>(count-64));
        tmp.hh  = U64_0;
        return (tmp);
    } else if (count == 64) {
        tmp.ll = value.lh;
        tmp.lh = value.hl;
        tmp.hl = value.hh;
        tmp.hh  = U64_0;
        return (tmp);
    } else if (count >  0) {
        tmp.ll = (value.ll>>count) | (value.lh<<(64-count));
        tmp.lh = (value.lh>>count) | (value.hl<<(64-count));
        tmp.hl = (value.hl>>count) | (value.hh<<(64-count));
        tmp.hh = (value.hh>>count);
        return (tmp);
    } 
    return(U256_0); // MACH ADDED

}

EM_uint256_t
fp82_fp_U256_inc(EM_uint256_t value)
{
    EM_uint256_t tmp;

            /* add one */
    tmp.ll = value.ll + 1;
    tmp.lh = value.lh + (tmp.ll < value.ll);
    tmp.hl = value.hl + (tmp.lh < value.lh);
    tmp.hh = value.hh + (tmp.hl < value.hl);
    return (tmp);
}

static INLINE EM_uint256_t
fp_U256_neg(EM_uint256_t value)
{
    EM_uint256_t tmp;

            /* complement */
    value.ll = ~value.ll;
    value.lh = ~value.lh;
    value.hl = ~value.hl;
    value.hh = ~value.hh;
            /* add one */
    tmp.ll = value.ll + 1;
    tmp.lh = value.lh + (tmp.ll < value.ll);
    tmp.hl = value.hl + (tmp.lh < value.lh);
    tmp.hh = value.hh + (tmp.hl < value.hl);
    return (tmp);
}

static INLINE EM_uint256_t
fp_U256_add(EM_uint256_t value1, EM_uint256_t value2)
{
    EM_uint256_t tmp;

        /* sum  */
    value2.ll = value1.ll + value2.ll;
    value2.lh = value1.lh + value2.lh;
    value2.hl = value1.hl + value2.hl;
    value2.hh = value1.hh + value2.hh;
        /* carry */
    tmp.ll = 0;
    tmp.lh = (value2.ll < value1.ll);
    tmp.hl = (value2.lh < value1.lh);
    tmp.hh = (value2.hl < value1.hl);
/*  c_out  = (value2.hh < value1.hh); */
        /* carry propagate adder */
    tmp.ll =  value2.ll;
    tmp.lh += value2.lh;
    tmp.hl += value2.hl + (tmp.lh < value2.lh);
    tmp.hh += value2.hh + (tmp.hl < value2.hl);
/*  c_out  +=             (tmp.hh < value2.hh); */
    return (tmp);
}

/* Basic Conversion Routines */

INLINE EM_uint128_t
fp_U64_to_U128(EM_uint64_t value)
{
    EM_uint128_t tmp;
    tmp.lo = value;
    tmp.hi = U64_0;
    return (tmp);
}

INLINE EM_uint64_t
fp_U128_to_U64(EM_uint128_t value)
{
    EM_uint64_t tmp;
    tmp = value.lo;
    return (tmp);
}

static INLINE EM_uint256_t
fp_U64_to_U256(EM_uint64_t value)
{
    EM_uint256_t tmp;
    tmp.ll = value;
    tmp.lh = U64_0;
    tmp.hl = U64_0;
    tmp.hh = U64_0;
    return (tmp);
}

static INLINE EM_uint64_t
fp_U256_to_U64(EM_uint256_t value)
{
    EM_uint64_t tmp;
    tmp = value.ll;
    return (tmp);
}

EM_uint256_t
fp82_fp_U128_to_U256(EM_uint128_t value)
{
    EM_uint256_t tmp;
    tmp.ll = value.lo;
    tmp.lh = value.hi;
    tmp.hl  = U64_0;
    tmp.hh  = U64_0;
    return (tmp);
}

static INLINE EM_uint128_t
fp_U256_to_U128(EM_uint256_t value)
{
    EM_uint128_t tmp;
    tmp.lo = value.ll;
    tmp.hi = value.lh;
    return (tmp);
}

