/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1996  Intel Corporation

Module Name:

    floatem.c

Abstract:

    This module implements IA64 machine dependent floating point emulation
    functions to support the IEEE floating point standard.

Author:

    Marius Cornea-Hasegan  Sep-96

Environment:

    Kernel mode only.

Revision History:
 
    Modfied  Jan. 97, Jan 98, Jun 98 (new API)

--*/

#if 0
/* #define this in floatem.c, fedefs.h and fesupport.c */
#define DEBUG_UNIX
#endif

#include "ki.h"
#include "fedefs.h"
#include "fetypes.h"
#include "fesupprt.h"
#include "feproto.h"

#ifdef TRUE
#undef TRUE
#endif
#define TRUE            0

#ifdef FALSE
#undef FALSE
#endif
#define FALSE           1

#define FP_EMUL_ERROR          -1
#define FAULT_TO_TRAP           2
#define SIMD_INSTRUCTION        4
#define FPFLT 1
#define FPTRAP 0
#define	FP_REG_EMIN		-65534
#define	FP_REG_EMAX		65535
#define	N64			64

typedef struct _BUNDLE {
  EM_uint64_t BundleLow;
  EM_uint64_t BundleHigh;
} BUNDLE;

#ifdef WIN32_OR_WIN64
typedef struct __declspec(align(16)) _FLOAT128_TYPE {
#else
typedef struct _FLOAT128_TYPE {
#endif
     EM_uint64_t loFlt64;
     EM_uint64_t hiFlt64;
} FLOAT128_TYPE;


#ifndef CONST_FORMAT

#ifndef WIN32_OR_WIN64
#define CONST_FORMAT(num) num##LL
#else
#define CONST_FORMAT(num) ((EM_uint64_t)(num))
#endif

#endif


// Functions (static or external)

int
swa_trap (EM_opcode_sf_type sf, EM_uint64_t FPSR, EM_uint_t ISRlow);

EM_fp_reg_type
FP128ToFPReg (
    FLOAT128_TYPE f128
    );

FLOAT128_TYPE
FPRegToFP128 (
    EM_fp_reg_type fpreg
    );

FLOAT128_TYPE
get_fp_register (
    int reg,
    void *fp_state
    );

void
set_fp_register (
    int reg,
    FLOAT128_TYPE value,
    void *fp_state
    );


void run_fms (EM_uint64_t *fpsr, 
    FLOAT128_TYPE *d, FLOAT128_TYPE *a, FLOAT128_TYPE *b, FLOAT128_TYPE *c);
void thmF (EM_uint64_t *fpsr, FLOAT128_TYPE *a, FLOAT128_TYPE *b,
    FLOAT128_TYPE *c);
void thmL (EM_uint64_t *fpsr, FLOAT128_TYPE *a, FLOAT128_TYPE *s);



// Masks and patterns for the different faulting FP instruction types 
// Note: Fn_MIN_MASK and Fn_PATTERN need to be checked if new opcodes 
// are inserted in this function


#define F1_MIN_MASK                     CONST_FORMAT(0x010000000000)
#define F1_PATTERN                      CONST_FORMAT(0x010000000000)

#define F1_MASK                         CONST_FORMAT(0x01F000000000)

#define FMA_PATTERN                     CONST_FORMAT(0x010000000000)
#define FMA_S_PATTERN                   CONST_FORMAT(0x011000000000)
#define FMA_D_PATTERN                   CONST_FORMAT(0x012000000000)
#define FPMA_PATTERN                    CONST_FORMAT(0x013000000000)

#define FMS_PATTERN                     CONST_FORMAT(0x014000000000)
#define FMS_S_PATTERN                   CONST_FORMAT(0x015000000000)
#define FMS_D_PATTERN                   CONST_FORMAT(0x016000000000)
#define FPMS_PATTERN                    CONST_FORMAT(0x017000000000)

#define FNMA_PATTERN                    CONST_FORMAT(0x018000000000)
#define FNMA_S_PATTERN                  CONST_FORMAT(0x019000000000)
#define FNMA_D_PATTERN                  CONST_FORMAT(0x01A000000000)
#define FPNMA_PATTERN                   CONST_FORMAT(0x01B000000000)


#define F4_MIN_MASK                     CONST_FORMAT(0x018000000000)
#define F4_PATTERN                      CONST_FORMAT(0x008000000000)

#define F4_MASK                         CONST_FORMAT(0x01F200001000)

#define FCMP_EQ_PATTERN                 CONST_FORMAT(0x008000000000)
#define FCMP_LT_PATTERN                 CONST_FORMAT(0x009000000000)
#define FCMP_LE_PATTERN                 CONST_FORMAT(0x008200000000)
#define FCMP_UNORD_PATTERN              CONST_FORMAT(0x009200000000)
#define FCMP_EQ_UNC_PATTERN             CONST_FORMAT(0x008000001000)
#define FCMP_LT_UNC_PATTERN             CONST_FORMAT(0x009000001000)
#define FCMP_LE_UNC_PATTERN             CONST_FORMAT(0x008200001000)
#define FCMP_UNORD_UNC_PATTERN          CONST_FORMAT(0x009200001000)


#define F6_MIN_MASK                     CONST_FORMAT(0x019200000000)
#define F6_PATTERN                      CONST_FORMAT(0x000200000000)

#define F6_MASK                         CONST_FORMAT(0x01F200000000)

#define FRCPA_PATTERN                   CONST_FORMAT(0x000200000000)
#define FPRCPA_PATTERN                  CONST_FORMAT(0x002200000000)


#define F7_MIN_MASK                     CONST_FORMAT(0x019200000000)
#define F7_PATTERN                      CONST_FORMAT(0x001200000000)

#define F7_MASK                         CONST_FORMAT(0x01F200000000)

#define FRSQRTA_PATTERN                 CONST_FORMAT(0x001200000000)
#define FPRSQRTA_PATTERN                CONST_FORMAT(0x003200000000)


#define F8_MIN_MASK                     CONST_FORMAT(0x018240000000)
#define F8_PATTERN                      CONST_FORMAT(0x000000000000)

#define F8_MASK                         CONST_FORMAT(0x01E3F8000000)

#define FMIN_PATTERN                    CONST_FORMAT(0x0000A0000000)
#define FMAX_PATTERN                    CONST_FORMAT(0x0000A8000000)
#define FAMIN_PATTERN                   CONST_FORMAT(0x0000B0000000)
#define FAMAX_PATTERN                   CONST_FORMAT(0x0000B8000000)
#define FPMIN_PATTERN                   CONST_FORMAT(0x0020A0000000)
#define FPMAX_PATTERN                   CONST_FORMAT(0x0020A8000000)
#define FPAMIN_PATTERN                  CONST_FORMAT(0x0020B0000000)
#define FPAMAX_PATTERN                  CONST_FORMAT(0x0020B8000000)
#define FPCMP_EQ_PATTERN                CONST_FORMAT(0x002180000000)
#define FPCMP_LT_PATTERN                CONST_FORMAT(0x002188000000)
#define FPCMP_LE_PATTERN                CONST_FORMAT(0x002190000000)
#define FPCMP_UNORD_PATTERN             CONST_FORMAT(0x002198000000)
#define FPCMP_NEQ_PATTERN               CONST_FORMAT(0x0021A0000000)
#define FPCMP_NLT_PATTERN               CONST_FORMAT(0x0021A8000000)
#define FPCMP_NLE_PATTERN               CONST_FORMAT(0x0021B0000000)
#define FPCMP_ORD_PATTERN               CONST_FORMAT(0x0021B8000000)


#define F10_MIN_MASK                    CONST_FORMAT(0x018240000000)
#define F10_PATTERN                     CONST_FORMAT(0x000040000000)

#define F10_MASK                        CONST_FORMAT(0x01E3F8000000)

#define FCVT_FX_PATTERN                 CONST_FORMAT(0x0000C0000000)
#define FCVT_FXU_PATTERN                CONST_FORMAT(0x0000C8000000)
#define FCVT_FX_TRUNC_PATTERN           CONST_FORMAT(0x0000D0000000)
#define FCVT_FXU_TRUNC_PATTERN          CONST_FORMAT(0x0000D8000000)
#define FPCVT_FX_PATTERN                CONST_FORMAT(0x0020C0000000)
#define FPCVT_FXU_PATTERN               CONST_FORMAT(0x0020C8000000)
#define FPCVT_FX_TRUNC_PATTERN          CONST_FORMAT(0x0020D0000000)
#define FPCVT_FXU_TRUNC_PATTERN         CONST_FORMAT(0x0020D8000000)



// minimum and maximum values of the exponent

#define EMIN_08_BITS    -126
#define EMIN_11_BITS    -1022
#define EMIN_15_BITS    -16382
#define EMIN_17_BITS    -65534



int
fp_emulate (
    int trap_type,
    BUNDLE *pbundle,
    EM_int64_t *pipsr,
    EM_int64_t *pfpsr,
    EM_int64_t *pisr,
    EM_int64_t *ppreds,
    EM_int64_t *pifs,
    void *fp_state
    )

{

  EM_uint64_t BundleHigh;
  EM_uint64_t BundleLow;
  EM_uint_t ISRlow;
  EM_uint_t ei;
  EM_uint64_t OpCode;

  EM_uint_t fault_ISR_code;
  EM_uint_t trap_ISR_code;

  EM_uint64_t FPSR;
  EM_uint64_t FPSR1;
  EM_uint64_t CFM;

  // arguments to emulation functions
  EM_opcode_sf_type sf;
  EM_pred_reg_specifier qp;
  EM_fp_reg_specifier f1;
  EM_fp_reg_specifier f2;
  EM_fp_reg_specifier f3;
  EM_fp_reg_specifier f4;
  EM_pred_reg_specifier p1;
  EM_pred_reg_specifier p2;

  EM_opcode_pc_type opcode_pc;
  EM_sf_pc_type pc;
  EM_sf_rc_type rc;
  EM_uint_t wre;

  int significand_size;

  EM_uint_t fpa, fpa_lo, fpa_hi;
  EM_uint_t I_exc, I_exc_lo, I_exc_hi;
  EM_uint_t U_exc, U_exc_lo, U_exc_hi;
  EM_uint_t O_exc, O_exc_lo, O_exc_hi;
  EM_uint_t sign, sign_lo, sign_hi;
  EM_uint_t exponent, exponent_lo, exponent_hi;
  EM_uint64_t significand;
  EM_uint_t significand_lo, significand_hi;
  EM_uint64_t low_half;
  EM_uint64_t high_half;
  EM_uint_t lsb, lsb_lo, lsb_hi;
  EM_uint_t round, round_lo, round_hi;
  EM_uint_t sticky, sticky_lo, sticky_hi;
  EM_uint_t I_dis, U_dis, O_dis;
  EM_uint_t Z_dis, D_dis, V_dis;

  EM_fp_reg_type tmp_fp;

  EM_int_t true_bexp, true_bexp_lo, true_bexp_hi;
  EM_int_t shift_cnt, shift_cnt_lo, shift_cnt_hi;
  EM_int_t emin;
  EM_int_t decr_exp, decr_exp_lo, decr_exp_hi;
  int ind;

  EM_uint_t EmulationExceptionCode;

  EM_state_type proc_state, *ps;

  EM_uint_t SIMD_instruction;

  // sign, exponent, and significand for the operands of a and b 
  // in FRCPA and FRSQRTA

  EM_uint_t sign_a;
  EM_int_t exponent_a;
  EM_uint64_t significand_a;
  EM_uint_t sign_b;
  EM_int_t exponent_b;
  EM_uint64_t significand_b;
  EM_int_t sign_c;
  EM_int_t exponent_c;
  EM_uint64_t significand_c;

  FLOAT128_TYPE a_float128;
  FLOAT128_TYPE b_float128;
  FLOAT128_TYPE c_float128;
  FLOAT128_TYPE c1_float128;
  FLOAT128_TYPE d_float128;
  FLOAT128_TYPE s_float128;
  int I_flag;
  int ftz;
  int unnormal;
  int new_trap_type;

  // local index registers
  int lf1 = 5;
  int lf2 = 2;
  int lf3 = 3;
  int lf4 = 4;

  unsigned int rrbpr;
  unsigned int rrbfr;


#ifdef DEBUG_UNIX
  printf ("**** DEBUG: ENTERING fp_emulate () ****\n");
#endif

  ps = &proc_state;
  EM_initialize_state (ps); 
  // do not reg any exception handlers
#ifndef unix
  f1 = 127; // initialize f1 (for MS compiler only; not really needed)
#endif
  SIMD_instruction = 2;
  ei = (EM_uint_t)0;
  OpCode = (EM_uint64_t)0;

  BundleLow = pbundle->BundleLow;
  BundleHigh = pbundle->BundleHigh;
#ifdef DEBUG_UNIX
  printf ("fp_emulate DEBUG: Bundle High/Low  = %Lx %Lx\n", 
      BundleHigh, BundleLow);
#endif

  ISRlow = (EM_uint_t)(*pisr);

  // FP status reg
  FPSR = *pfpsr;
  CFM = *pifs & CONST_FORMAT(0x03fffffffff);
  rrbpr = (unsigned int)((CFM >> 32) & 0x3f);
  rrbfr = (unsigned int)((CFM >> 25) & 0x7f);
#ifdef DEBUG_UNIX
  printf ("fp_emulate DEBUG: FPSR = %Lx\n", FPSR);
  printf ("fp_emulate DEBUG: CFM = %Lx\n", CFM);
  printf ("fp_emulate DEBUG: rrbpr = %x rrbfr = %x\n", rrbpr, rrbfr);
  printf ("fp_emulate DEBUG: PREDS = %Lx\n", *ppreds);
  printf ("fp_emulate DEBUG: ISRlow = %x\n", ISRlow);
#endif

  // copy the FPSR into AR[0]
  ps->state_AR[0].uint_value = FPSR;

#ifdef DEBUG_UNIX
  OpCode = (BundleLow >> 5) & CONST_FORMAT(0x01ffffffffff);
  printf ("DEBUG: OpCode0 = %Lx\n", OpCode);
  OpCode = ((BundleHigh & CONST_FORMAT(0x07fffff)) << 18) |
      ((BundleLow >> 46) & CONST_FORMAT(0x03ffff));
  printf ("DEBUG: OpCode1 = %Lx\n", OpCode);
  OpCode = (BundleHigh >> 23) & CONST_FORMAT(0x01ffffffffff);
  printf ("DEBUG: OpCode2 = %Lx\n", OpCode);
#endif

  // excepting instruction in bundle: slot 0, 1, or 2
  ei = (EM_uint_t)(((*pisr) >> 41) & 0x03);
  // cut the faulting instruction opcode (41 bits)
  if (ei == 0) { // no template for this case
    // OpCode = (BundleLow >> 5) & CONST_FORMAT(0x01ffffffffff);
#ifndef unix
# if DBG
    DbgPrint ("fp_emulate () Internal Error: template FXX\n");
# endif
#else
    FP_EMULATION_ERROR0 ("fp_emulate () Internal Error: template FXX\n");
    return (FP_EMUL_ERROR);
#endif
  } else if (ei == 1) { // templates: MFI, MFB
    OpCode = ((BundleHigh & CONST_FORMAT(0x07fffff)) << 18) | 
        ((BundleLow >> 46) & CONST_FORMAT(0x03ffff));
#ifdef DEBUG_UNIX
  printf ("DEBUG: ei = 1 OpCode = %Lx\n", OpCode);
#endif
  } else if (ei == 2) { // templates: MMF
    OpCode = (BundleHigh >> 23) & CONST_FORMAT(0x01ffffffffff);
#ifdef DEBUG_UNIX
  printf ("DEBUG: ei = 2 OpCode = %Lx\n", OpCode);
#endif
  } else {
#ifndef unix
# if DBG
    DbgPrint ("fp_emulate () Internal Error: instruction slot 3 is invalid\n");
# endif
#else
    FP_EMULATION_ERROR0 ("fp_emulate () Internal Error: \
instruction slot 3 is not valid\n");
    return (FP_EMUL_ERROR);
#endif
  }

  // decode the instruction opcode; assume fp_emulate () is only called
  // for FP instructions that caused an FP fault or trap

  // sf and qp have the same offset, for all the FP instructions
  sf = (EM_opcode_sf_type)((OpCode >> 34) & CONST_FORMAT(0x000000000003));
  qp = (EM_uint_t)(OpCode & CONST_FORMAT(0x00000000003F));
  if (qp >= 16) qp = 16 + (rrbpr + qp - 16) % 48;

  // read predicate reg qp
  ps->state_PR[qp] = (EM_boolean_t)(((*ppreds) >> qp) & 0x01);

  if (ps->state_PR[qp] == 0) {
#ifdef DEBUG_UNIX
    printf ("fp_emulate DEBUG: QUALIFYING PREDICATE %d IS 0\n", qp);
#endif
#ifndef unix
    FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
qualifying predicate PR[%2.2d] = 0\n", qp);
#else
    FP_EMULATION_ERROR1 ("fp_emulate ()  Internal Error: \
qualifying predicate PR[%2.2d] = 0\n", qp);
    return (FP_EMUL_ERROR);
#endif
  }

  I_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * (EM_uint_t)sf)) & 0x01) || 
      ((FPSR >> 5) & 0x01);
  U_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * (EM_uint_t)sf)) & 0x01) || 
      ((FPSR >> 4) & 0x01);
  O_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * (EM_uint_t)sf)) & 0x01) || 
      ((FPSR >> 3) & 0x01);
  Z_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * (EM_uint_t)sf)) & 0x01) || 
      ((FPSR >> 2) & 0x01);
  D_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * (EM_uint_t)sf)) & 0x01) || 
      ((FPSR >> 1) & 0x01);
  V_dis = sf != 0 && ((FPSR >> (6 + 6 + 13 * (EM_uint_t)sf)) & 0x01) || 
      (FPSR & 0x01);


  if ((trap_type == FPFLT) &&
      (ISRlow & 0x0088)) { // if this is a SWA fault

    // this will occur only for unnormal inputs for Merced, or for
    // architecturally mandated conditions for divide and square root
    // reciprocal approximations

    // decode the rest of the instruction
    if ((OpCode & F1_MIN_MASK) == F1_PATTERN) {
      // F1 instruction

      // extract f4, f3, f2, and f1
      f4 = (EM_uint_t)((OpCode >> 27) & CONST_FORMAT(0x00000000007F));
      if (f4 >= 32) f4 = 32 + (rrbfr + f4 - 32) % 96;
      f3 = (EM_uint_t)((OpCode >> 20) & CONST_FORMAT(0x00000000007F));
      if (f3 >= 32) f3 = 32 + (rrbfr + f3 - 32) % 96;
      f2 = (EM_uint_t)((OpCode >> 13) & CONST_FORMAT(0x00000000007F));
      if (f2 >= 32) f2 = 32 + (rrbfr + f2 - 32) % 96;
      f1 = (EM_uint_t)((OpCode >>  6) & CONST_FORMAT(0x00000000007F));
      if (f1 >= 32) f1 = 32 + (rrbfr + f1 - 32) % 96;

#ifdef DEBUG_UNIX
printf ("DEBUG BEF. F1 SWA FAULT: f1 f2 f3 f4 = %x %x %x %x\n", f1, f2, f3, f4);
#endif

      // get source floating-point reg values
      ps->state_FR[lf2] = FP128ToFPReg (get_fp_register (f2, fp_state));
      ps->state_FR[lf3] = FP128ToFPReg (get_fp_register (f3, fp_state));
      ps->state_FR[lf4] = FP128ToFPReg (get_fp_register (f4, fp_state));

#ifdef DEBUG_UNIX
printf ("DEBUG BEFORE F1 SWA FAULT: ps->state_FR[lf2] = %x %x %Lx\n",
  ps->state_FR[lf2].sign, ps->state_FR[lf2].exponent, ps->state_FR[lf2].significand);
printf ("DEBUG BEFORE F1 SWA FAULT: ps->state_FR[lf3] = %x %x %Lx\n",
  ps->state_FR[lf3].sign, ps->state_FR[lf3].exponent, ps->state_FR[lf3].significand);
printf ("DEBUG BEFORE F1 SWA FAULT: ps->state_FR[lf4] = %x %x %Lx\n",
  ps->state_FR[lf4].sign, ps->state_FR[lf4].exponent, ps->state_FR[lf4].significand);
#endif

      switch (OpCode & F1_MASK) {

        case FMA_PATTERN:
          SIMD_instruction = 0;
          fma (ps, pc_sf, sf, qp, lf1, lf3, lf4, lf2);
          break;
        case FMA_S_PATTERN:
          SIMD_instruction = 0;
          fma (ps, pc_s, sf, qp, lf1, lf3, lf4, lf2);
          break;
        case FMA_D_PATTERN:
          SIMD_instruction = 0;
          fma (ps, pc_d, sf, qp, lf1, lf3, lf4, lf2);
          break;
        case FPMA_PATTERN:
          SIMD_instruction = 1;
          fpma (ps, sf, qp, lf1, lf3, lf4, lf2);
          break;

        case FMS_PATTERN:
          SIMD_instruction = 0;
          fms (ps, pc_sf, sf, qp, lf1, lf3, lf4, lf2);
          break;
        case FMS_S_PATTERN:
          SIMD_instruction = 0;
          fms (ps, pc_s, sf, qp, lf1, lf3, lf4, lf2);
          break;
        case FMS_D_PATTERN:
          SIMD_instruction = 0;
          fms (ps, pc_d, sf, qp, lf1, lf3, lf4, lf2);
          break;
        case FPMS_PATTERN:
          SIMD_instruction = 1;
          fpms (ps, sf, qp, lf1, lf3, lf4, lf2);
          break;

        case FNMA_PATTERN:
          SIMD_instruction = 0;
          fnma (ps, pc_sf, sf, qp, lf1, lf3, lf4, lf2);
          break;
        case FNMA_S_PATTERN:
          SIMD_instruction = 0;
          fnma (ps, pc_s, sf, qp, lf1, lf3, lf4, lf2);
          break;
        case FNMA_D_PATTERN:
          SIMD_instruction = 0;
          fnma (ps, pc_d, sf, qp, lf1, lf3, lf4, lf2);
          break;
        case FPNMA_PATTERN:
          SIMD_instruction = 1;
          fpnma (ps, sf, qp, lf1, lf3, lf4, lf2);
          break;
        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode);
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode);
          return (FP_EMUL_ERROR);
#endif

      }

      if ((ps->state_MERCED_RTL >> 16) & 0x0ffff) goto new_exception;

      // successful emulation
      // set the destination floating-point reg value
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F1 SWA FAULT: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif
      set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
      if (f1 < 32)
        *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
      else
        *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

      *pfpsr =  ps->state_AR[0].uint_value;
      return (TRUE);


    } else if ((OpCode & F4_MIN_MASK) == F4_PATTERN) {
      // F4 instruction

      // extract p2, f3, f2, and p1
      p2 = (EM_uint_t)((OpCode >> 27) & CONST_FORMAT(0x00000000003f));
      if (p2 >= 16) p2 = 16 + (rrbpr + p2 - 16) % 48;
      f3 = (EM_uint_t)((OpCode >> 20) & CONST_FORMAT(0x00000000007F));
      if (f3 >= 32) f3 = 32 + (rrbfr + f3 - 32) % 96;
      f2 = (EM_uint_t)((OpCode >> 13) & CONST_FORMAT(0x00000000007F));
      if (f2 >= 32) f2 = 32 + (rrbfr + f2 - 32) % 96;
      p1 = (EM_uint_t)((OpCode >>  6) & CONST_FORMAT(0x00000000003F));
      if (p1 >= 16) p1 = 16 + (rrbpr + p1 - 16) % 48;

      // get source floating-point reg values
      ps->state_FR[lf2] = FP128ToFPReg (get_fp_register (f2, fp_state));
      ps->state_FR[lf3] = FP128ToFPReg (get_fp_register (f3, fp_state));
#ifdef DEBUG_UNIX
printf ("DEBUG BEFORE F4 SWA FAULT: ps->state_FR[lf2] = %x %x %Lx\n",
  ps->state_FR[lf2].sign, ps->state_FR[lf2].exponent, ps->state_FR[lf2].significand);
printf ("DEBUG BEFORE F4 SWA FAULT: ps->state_FR[lf3] = %x %x %Lx\n",
  ps->state_FR[lf3].sign, ps->state_FR[lf3].exponent, ps->state_FR[lf3].significand);
#endif

      switch (OpCode & F4_MASK) {

        case FCMP_EQ_PATTERN:
          SIMD_instruction = 0;
          fcmp_eq (ps, ctype_none, sf, qp, p1, p2, lf2, lf3);
          break;
        case FCMP_LT_PATTERN:
          SIMD_instruction = 0;
          fcmp_lt (ps, ctype_none, sf, qp, p1, p2, lf2, lf3);
          break;
        case FCMP_LE_PATTERN:
          SIMD_instruction = 0;
          fcmp_le (ps, ctype_none, sf, qp, p1, p2, lf2, lf3);
          break;
        case FCMP_UNORD_PATTERN:
          SIMD_instruction = 0;
          fcmp_unord (ps, ctype_none, sf, qp, p1, p2, lf2, lf3);
          break;

        case FCMP_EQ_UNC_PATTERN:
          SIMD_instruction = 0;
          fcmp_eq (ps, fctypeUNC, sf, qp, p1, p2, lf2, lf3);
          break;
        case FCMP_LT_UNC_PATTERN:
          SIMD_instruction = 0;
          fcmp_lt (ps, fctypeUNC, sf, qp, p1, p2, lf2, lf3);
          break;
        case FCMP_LE_UNC_PATTERN:
          SIMD_instruction = 0;
          fcmp_le (ps, fctypeUNC, sf, qp, p1, p2, lf2, lf3);
          break;
        case FCMP_UNORD_UNC_PATTERN:
          SIMD_instruction = 0;
          fcmp_unord (ps, fctypeUNC, sf, qp, p1, p2, lf2, lf3);
          break;
        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode);
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode);
          return (FP_EMUL_ERROR);
#endif

      }

      if ((ps->state_MERCED_RTL >> 16) & 0x0ffff) goto new_exception;

      // successful emulation
      // set the destination predicate reg values
      *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p1));
      *ppreds |= (((EM_uint64_t)(ps->state_PR[p1] & 0x01)) << (EM_uint_t)p1);
      *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
      *ppreds |= (((EM_uint64_t)(ps->state_PR[p2] & 0x01)) << (EM_uint_t)p2);
      *pfpsr =  ps->state_AR[0].uint_value;
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F4 SWA FAULT: *ppreds = %Lx\n", (*ppreds));
#endif
      return (TRUE);

    } else if ((OpCode & F6_MIN_MASK) == F6_PATTERN) {
      // F6 instruction
      switch (OpCode & F6_MASK) {

        case FRCPA_PATTERN:
          SIMD_instruction = 0;
          break;
        case FPRCPA_PATTERN:
          SIMD_instruction = 1;
          break;
        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode);
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode);
          return (FP_EMUL_ERROR);
#endif
      }

      // extract the ftz bit
      ftz = (int)((FPSR >> 6) & 0x01);

      // extract the rounding mode
      rc = (EM_sf_rc_type)((FPSR >> (6 + 4 + 13 * (EM_uint_t)sf)) & 0x03);

      // extract p2, f3, f2, and f1
      p2 = (EM_uint_t)((OpCode >> 27) & CONST_FORMAT(0x00000000003f));
      if (p2 >= 16) p2 = 16 + (rrbpr + p2 - 16) % 48;
      f3 = (EM_uint_t)((OpCode >> 20) & CONST_FORMAT(0x00000000007F));
      if (f3 >= 32) f3 = 32 + (rrbfr + f3 - 32) % 96;
      f2 = (EM_uint_t)((OpCode >> 13) & CONST_FORMAT(0x00000000007F));
      if (f2 >= 32) f2 = 32 + (rrbfr + f2 - 32) % 96;
      f1 = (EM_uint_t)((OpCode >>  6) & CONST_FORMAT(0x00000000007F));
      if (f1 >= 32) f1 = 32 + (rrbfr + f1 - 32) % 96;

      // get source floating-point reg values
      ps->state_FR[lf2] = FP128ToFPReg (get_fp_register (f2, fp_state));
      ps->state_FR[lf3] = FP128ToFPReg (get_fp_register (f3, fp_state));
#ifdef DEBUG_UNIX
printf ("DEBUG BEFORE F6 SWA FAULT: ps->state_FR[lf2] = %x %x %Lx\n",
  ps->state_FR[lf2].sign, ps->state_FR[lf2].exponent, ps->state_FR[lf2].significand);
printf ("DEBUG BEFORE F6 SWA FAULT: ps->state_FR[lf3] = %x %x %Lx\n",
  ps->state_FR[lf3].sign, ps->state_FR[lf3].exponent, ps->state_FR[lf3].significand);
#endif

      switch (OpCode & F6_MASK) {

        case FRCPA_PATTERN:

          // extract sign, exponent, and significand of a
          sign_a = (EM_uint_t)ps->state_FR[lf2].sign;
          exponent_a = (EM_int_t)ps->state_FR[lf2].exponent;
          significand_a = ps->state_FR[lf2].significand;

          // extract sign, exponent, and significand of b
          // note that b cannot be 0
          sign_b = (EM_uint_t)ps->state_FR[lf3].sign;
          exponent_b = (EM_int_t)ps->state_FR[lf3].exponent;
          significand_b = ps->state_FR[lf3].significand;

          // if any of a or b is zero or pseudo-zero, return the result
          if (significand_a == 0 || significand_b == 0) {

            frcpa (ps, sf, qp, lf1, p2, lf2, lf3);

            if ((ps->state_MERCED_RTL >> 16) & 0x0ffff) goto new_exception;

            *pfpsr =  ps->state_AR[0].uint_value;

            // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F6 SWA FAULT FOR a OR b ZERO/PSEUDO-ZERO: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent,
  ps->state_FR[lf1].significand);
#endif
            set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
            if (f1 < 32)
              *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
            else
              *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit
            // ps->state_PR[p2] = 0 for a or b zero or pseudo-zero
            *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F6 SWA FAULT FOR a OR b ZERO/PSEUDO-ZERO: p2 = %x\n", p2);
#endif
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F6 SWA FAULT FOR a OR b ZERO/PSEUDO-ZERO: *ppreds = %Lx\n",
 *ppreds);
#endif
            return (TRUE);

          }

          // if any of a or b is infinity, return the result
          if (exponent_a == 0x1ffff && 
              significand_a == CONST_FORMAT(0x8000000000000000) || 
              exponent_b == 0x1ffff && 
              significand_b == CONST_FORMAT(0x8000000000000000)) {

            frcpa (ps, sf, qp, lf1, p2, lf2, lf3);

            if ((ps->state_MERCED_RTL >> 16) & 0x0ffff) goto new_exception;
                // will never happen

            *pfpsr =  ps->state_AR[0].uint_value;

            // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F6 SWA FAULT FOR a OR b INFINITY: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent,
  ps->state_FR[lf1].significand);
#endif
            set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
            if (f1 < 32)
              *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
            else
              *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

            // ps->state_PR[p2] = 0; clear the output predicate for a or b inf
            *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F6 SWA FAULT FOR a OR b INFINITY: p2 = %x\n", p2);
#endif
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F6 SWA FAULT FOR a OR b INFINITY: *ppreds = %Lx\n",
 *ppreds);
#endif
            return (TRUE);

          }

          // a and b are not [pseudo]0, and are not special (a and be are
          // normal or unnormal [denormal] floating-point numbers)

          if (exponent_a == 0) exponent_a = 0xc001;
              // this covers double-extended real [pseudo-]denormals
          // un-bias the exponent of a
          exponent_a = exponent_a - 0xffff;

          if (exponent_b == 0) exponent_b = 0xc001;
              // this covers double-extended real [pseudo-]denormals
          // un-bias the exponent of b
          exponent_b = exponent_b - 0xffff;

          unnormal = 0;
          // check whether a is unnormal; will set D in FPSR.sfx if true
          // and denormal exceptions are disabled
          if (!(significand_a & CONST_FORMAT(0x8000000000000000))) {
            unnormal = 1;
          }
          // check whether b is unnormal; will set D in FPSR.sfx if true
          // and denormal exceptions are disabled
          if (!(significand_b & CONST_FORMAT(0x8000000000000000))) {
            unnormal = 1;
          }
#ifdef DEBUG_UNIX
if (unnormal) printf ("DEBUG F6 FRCPA SWA FAULT: unnormal = 1\n");
#endif

          if (unnormal && !D_dis) {
            ISRlow = 0x0002; // denormal bit set
            *pisr = ((*pisr) & 0xffffffffffff0000) | ISRlow;
            return (FALSE); // will raise D fault
          }

          // normalize a (even if exponent_a becomes less than e_min)
          while (!(significand_a & CONST_FORMAT(0x8000000000000000))) {
            significand_a = significand_a << 1;
            exponent_a--;
          }

          // normalize b (even if exponent_b becomes less than e_min)
          while (!(significand_b & CONST_FORMAT(0x8000000000000000))) {
            significand_b = significand_b << 1;
            exponent_b--;
          }

          // Case (I) and Case (II)
          // |a/b| > MAXFP ==> might have O or I traps

          if ((exponent_b <= exponent_a - FP_REG_EMAX - 2) ||
            (exponent_b == exponent_a - FP_REG_EMAX - 1) &&
            (significand_a >= significand_b)) {

#ifdef DEBUG_UNIX
printf ("DEBUG: BEGIN F6 SWA FAULT CASE (I) - (II)\n");
#endif

            // scale a to a' and b to b', such that c' = a'/ b' will be
            // normal 

            // set the scaled (possibly normalized) value of a' (sign ok)
            ps->state_FR[lf2].exponent = (EM_uint_t)(0xffff);
            ps->state_FR[lf2].significand = significand_a;

            // set the scaled (possibly normalized) value of b' (sign ok)
            ps->state_FR[lf3].exponent = (EM_uint_t)(0xffff);
            ps->state_FR[lf3].significand = significand_b;

            // convert a' and b' to FLOAT128
            a_float128 = FPRegToFP128 (ps->state_FR[lf2]);
            b_float128 = FPRegToFP128 (ps->state_FR[lf3]);

            // invoke the divide algorithm to calculate c' = a' / b';
            // the algorithm uses sf0 with user settings, and sf1 with
            // rn, 64-bits, wre, traps disabled;
            // copy FPSR.sfx with clear flags to FPSR1.sf0; rn,64,wre in sf1
            FPSR1 = (EM_uint64_t)((FPSR >> ((EM_uint_t)sf * 13)) & 0x01fc0)
                | 0x000000000270003f; // set sf0,sf1 and disable fp exceptions
            thmF (&FPSR1, &a_float128, &b_float128, &c_float128);
            I_flag = FPSR1 & 0x40000 ? 1 : 0;

            if (O_dis && (I_dis || !I_flag)) {

              // overflow exceptions are disabled and (inexact exceptions
              // are disabled or the result is exact) => return the 
              // IEEE mandated result

              if (sign_a ^ sign_b) { // opposite signs
                if (rc == rc_rn || rc == rc_rm) {
                  // -Inf
                  ps->state_FR[lf1].sign = 1;
                  ps->state_FR[lf1].exponent = 0x1ffff;
                  ps->state_FR[lf1].significand = 
                      CONST_FORMAT(0x8000000000000000);
                } else { // if (rc == rc_rp || rc == rc_rz)
                  // -MAX_FP_REG_VAL
                  ps->state_FR[lf1].sign = 1;
                  ps->state_FR[lf1].exponent = 0x1fffe;
                  ps->state_FR[lf1].significand = 
                      CONST_FORMAT(0xffffffffffffffff);
                }
              } else { // same sign
                if (rc == rc_rn || rc == rc_rp) {
                  // Inf
                  ps->state_FR[lf1].sign = 0;
                  ps->state_FR[lf1].exponent = 0x1ffff;
                  ps->state_FR[lf1].significand = 
                      CONST_FORMAT(0x8000000000000000);
                } else { // if (rc == rc_rm || rc == rc_rz)
                  // MAX_FP_REG_VAL
                  ps->state_FR[lf1].sign = 0;
                  ps->state_FR[lf1].exponent = 0x1fffe;
                  ps->state_FR[lf1].significand = 
                      CONST_FORMAT(0xffffffffffffffff);
                }
              }

              // set D in FPSR.sfx if any of a and b was unnormal
              if (unnormal) {
                // set D = 1 in *pfpsr
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 8));
              }

              // set I = 1 and O = 1 in *pfpsr
              // set O = 1
              *pfpsr = *pfpsr | 
                  ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 10));
              // set I = 1
              *pfpsr = *pfpsr |
                  ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 12));
              // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG Case (I), (II) AFTER F6 SWA FAULT 1: ps->state_FR[lf1] = %x %x %Lx\n",
ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
ps->state_FR[lf1].significand);
#endif
              set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
              if (f1 < 32)
                *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
              else
                *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

              // ps->state_PR[p2] = 0; clear the output predicate
              *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
#ifdef DEBUG_UNIX
printf ("DEBUG Case (I), (II) AFTER F6 SWA FAULT 1 a: *ppreds = %Lx\n",
*ppreds);
#endif
              return (TRUE);

            } else if (!O_dis) {

              // overflow exceptions are enabled => compute the result, and
              // propagate an overflow exception (deliver the result with
              // the exponent mod 2^17

              // convert c' (normal fp#) from FLOAT128 to EM_fp_reg_type
              ps->state_FR[lf1] = FP128ToFPReg (c_float128);
              // scale c' to c and take the mod 2^17 exponent
              exponent_c = (EM_uint_t)ps->state_FR[lf1].exponent + 
                  exponent_a - exponent_b;
              ISRlow = 0x0801; // O = 1
              ps->state_FR[lf1].exponent = exponent_c & 0x1ffff;

              // determine fpa, and set the values of I and fpa in ISRlow
              // if (I_flag == 0) fpa = 0
              if (I_flag == 1) {

                // calculate d' = |b'| * |c'| - |a'| to determine fpa
                c_float128.hiFlt64 = c_float128.hiFlt64 & 
                    CONST_FORMAT(0x000000000001ffff); // take |c'|
                b_float128.hiFlt64 = b_float128.hiFlt64 & 
                    CONST_FORMAT(0x000000000001ffff); // take |b'|
                a_float128.hiFlt64 = a_float128.hiFlt64 & 
                    CONST_FORMAT(0x000000000001ffff); // take |a'|

                FPSR1 = CONST_FORMAT(0x00000000000003bf); // rn,64,wre=1,dis
                run_fms (&FPSR1, &d_float128, &b_float128, &c_float128, 
                    &a_float128); // d' = |b'| * |c'| - |a'|

                if (d_float128.hiFlt64 & CONST_FORMAT(0x020000)) {
                  // if d' < 0, I = 1 and fpa = 0
                  ISRlow = ISRlow | 0x2000;
                } else {
                  // if d' > 0, I = 1 and fpa = 1
                  ISRlow = ISRlow | 0x6000;
                }

              }

              // set the destination floating-point and predicate reg values
              *pisr = ((*pisr) & 0xffffffffffff0000) | ISRlow;

#ifdef DEBUG_UNIX
printf ("DEBUG Case (I), (II) AFTER F6 SWA FAULT 2: ps->state_FR[lf1] = %x %x %Lx\n",
ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
ps->state_FR[lf1].significand);
#endif
              set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
              if (f1 < 32)
                *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
              else
                *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

              // ps->state_PR[p2] = 0; clear the output predicate
              // update (*ppreds) [as if O disabled]
              *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
#ifdef DEBUG_UNIX
printf ("DEBUG Case (I), (II) AFTER F6 SWA FAULT 2 a: *ppreds = %Lx\n", *ppreds);
#endif
              // update *pfpsr
              // set D in FPSR.sfx if any of a and b was unnormal
              if (unnormal) {
                // set D = 1 in *pfpsr
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 8));
              }
              // set O = 1
              *pfpsr = *pfpsr |
                  ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 10));
              // set I = 1 if I_flag = 1
              if (I_flag) *pfpsr = *pfpsr |
                  ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 12));

              // caller will advance instruction pointer
              return (FALSE | FAULT_TO_TRAP); // will raise O trap

            } else { // if (!I_dis && I_flag)

              // overflow exceptions are disabled, but the inexact 
              // exceptions are enabled and the result is inexact => 
              // provide the IEEE mandated result, and
              // propagate an inexact exception

              if (sign_a ^ sign_b) { // opposite signs
                if (rc == rc_rn || rc == rc_rm) {
                  // -Inf
                  ps->state_FR[lf1].sign = 1;
                  ps->state_FR[lf1].exponent = 0x1ffff;
                  ps->state_FR[lf1].significand = 
                      CONST_FORMAT(0x8000000000000000);
                  fpa = 1;
                } else { // if (rc == rc_rp || rc == rc_rz)
                  // -MAX_FP_REG_VAL
                  ps->state_FR[lf1].sign = 1;
                  ps->state_FR[lf1].exponent = 0x1fffe;
                  ps->state_FR[lf1].significand = 
                      CONST_FORMAT(0xffffffffffffffff);
                  fpa = 0;
                }
              } else { // same sign
                if (rc == rc_rn || rc == rc_rp) {
                  // Inf
                  ps->state_FR[lf1].sign = 0;
                  ps->state_FR[lf1].exponent = 0x1ffff;
                  ps->state_FR[lf1].significand = 
                      CONST_FORMAT(0x8000000000000000);
                  fpa = 1;
                } else { // if (rc == rc_rm || rc == rc_rz)
                  // MAX_FP_REG_VAL
                  ps->state_FR[lf1].sign = 0;
                  ps->state_FR[lf1].exponent = 0x1fffe;
                  ps->state_FR[lf1].significand = 
                      CONST_FORMAT(0xffffffffffffffff);
                  fpa = 0;
                }
              }

              ISRlow = 0x2001 | (fpa == 1 ? 0x4000 : 0x0000); // I = 1 and fpa
              *pisr = ((*pisr) & 0xffffffffffff0000) | ISRlow;

              // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG Case (I), (II) AFTER F6 SWA FAULT 3: ps->state_FR[lf1] = %x %x %Lx\n",
ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
ps->state_FR[lf1].significand);
#endif
              set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
              if (f1 < 32)
                *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
              else
                *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

              // ps->state_PR[p2] = 0; clear the output predicate
              // update *ppreds [as if O disabled]
              *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
#ifdef DEBUG_UNIX
printf ("DEBUG Case (I), (II) AFTER F6 SWA FAULT 3 a: *ppreds = %Lx\n",
*ppreds);
#endif
              // update *pfpsr
              // set D in FPSR.sfx if any of a and b was unnormal
              if (unnormal) {
                // set D = 1 in *pfpsr
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 8));
              }
              // set I = 1 and O = 1 in *pfpsr
              // set O = 1
              *pfpsr = *pfpsr |
                  ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 10));
              // set I = 1
              *pfpsr = *pfpsr |
                  ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 12));

              // caller will advance instruction pointer
              return (FALSE | FAULT_TO_TRAP); // will raise I trap

            }

          // Case (III), Case (IV), Case (V), Case (VI), and Case (VII)
          // a/b is normal, non-zero ==> might have an I trap

          } else if ((exponent_b == exponent_a - FP_REG_EMAX - 1) &&
            (significand_a < significand_b) ||
            (exponent_b == exponent_a - FP_REG_EMAX) ||
            (exponent_a - FP_REG_EMAX + 1 <= exponent_b ) &&
            (exponent_b <= exponent_a - FP_REG_EMIN - 2) &&
            ((exponent_a <= FP_REG_EMIN + N64 - 1) ||
            (exponent_b <= FP_REG_EMIN - 1) ||
            (exponent_b >= FP_REG_EMAX - 2)) ||
            (exponent_b == exponent_a - FP_REG_EMIN - 1) ||
            (exponent_b == exponent_a - FP_REG_EMIN) &&
            (significand_a >= significand_b)) {

#ifdef DEBUG_UNIX
printf ("DEBUG: BEGIN F6 SWA FAULT CASE (III) - (VII)\n");
#endif

            // scale a to a' and b to b', such that c' = a'/ b' will be 
            // normal

            // set the scaled (possibly normalized) value of a' (sign ok)
            ps->state_FR[lf2].exponent = (EM_uint_t)(0xffff);
            ps->state_FR[lf2].significand = significand_a;

            // set the scaled (possibly normalized) value of b' (sign ok)
            ps->state_FR[lf3].exponent = (EM_uint_t)(0xffff);
            ps->state_FR[lf3].significand = significand_b;

            // convert a' and b' to FLOAT128
            a_float128 = FPRegToFP128 (ps->state_FR[lf2]);
            b_float128 = FPRegToFP128 (ps->state_FR[lf3]);

            // invoke the divide algorithm to calculate c' = a' / b';
            // the algorithm uses sf0 with user settings, and sf1 with
            // rn, 64-bits, wre, traps disabled;
            // copy FPSR.sfx with clear flags to FPSR1.sf0; rn,64,wre in sf1
            FPSR1 = (EM_uint64_t)((FPSR >> ((EM_uint_t)sf * 13)) & 0x01fc0)
                | 0x000000000270003f; // set sf0,sf1 and disable fp exceptions
            thmF (&FPSR1, &a_float128, &b_float128, &c_float128);
            I_flag = FPSR1 & 0x40000 ? 1 : 0;

            // set the result
            // convert c' (normal fp#) from FLOAT128 to EM_fp_reg_type
            ps->state_FR[lf1] = FP128ToFPReg (c_float128);
            // scale c' to c
            ps->state_FR[lf1].exponent = (EM_uint_t)ps->state_FR[lf1].exponent
                + exponent_a - exponent_b;

            if (I_dis || !I_flag) {

              // set D in FPSR.sfx if any of a and b was unnormal
              if (unnormal) {
                // set D = 1 in *pfpsr
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 8));
              }

              // set I in *pfpsr
              if (I_flag) *pfpsr = *pfpsr |
                  ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 12));
              // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG Case (III) - (VII) AFTER F6 SWA FAULT 4: ps->state_FR[lf1] = %x %x %Lx\n",
ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
ps->state_FR[lf1].significand);
#endif
              set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
              if (f1 < 32)
                *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
              else
                *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

              // ps->state_PR[p2] = 0; clear the output predicate
              *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
#ifdef DEBUG_UNIX
printf ("DEBUG Case (III) - (VII) AFTER F6 SWA FAULT 4 a: *ppreds = %Lx\n",
*ppreds);
#endif
              return (TRUE);

            } else { // if (!I_dis && I_flag)

              // calculate d' = |b'| * |c'| - |a'| to determine fpa
              c_float128.hiFlt64 = c_float128.hiFlt64 &
                  CONST_FORMAT(0x000000000001ffff); // take |c'|
              b_float128.hiFlt64 = b_float128.hiFlt64 &
                  CONST_FORMAT(0x000000000001ffff); // take |b'|
              a_float128.hiFlt64 = a_float128.hiFlt64 &
                  CONST_FORMAT(0x000000000001ffff); // take |a'|
              FPSR1 = CONST_FORMAT(0x00000000000003bf); // rn,64,wre=1,dis
              run_fms (&FPSR1, &d_float128, &b_float128, &c_float128,
                  &a_float128); // d' = |b'| * |c'| - |a'|

              if (d_float128.hiFlt64 & CONST_FORMAT(0x0000000000020000)) {
                // if d' < 0, I = 1 and fpa = 0
                ISRlow = 0x2001;
              } else {
                // if d' > 0, I = 1 and fpa = 1
                ISRlow = 0x6001;
              }

              *pisr = ((*pisr) & 0xffffffffffff0000) | ISRlow;

              // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG Case (III) - (VII) AFTER F6 SWA FAULT 5: ps->state_FR[lf1] = %x %x %Lx\n",
ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
ps->state_FR[lf1].significand);
#endif
              set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
              if (f1 < 32)
                *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
              else
                *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

              // ps->state_PR[p2] = 0; clear the output predicate
              // update *ppreds
              *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
#ifdef DEBUG_UNIX
printf ("DEBUG Case (III) -(VII) AFTER F6 SWA FAULT 5 a: *ppreds = %Lx\n",
*ppreds);
#endif
              // update *pfpsr
              // set D = 1 in FPSR.sfx if any of a and b was unnormal
              if (unnormal) {
                // set D = 1 in *pfpsr
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 8));
              }
              // set I = 1 in *pfpsr
              if (I_flag) *pfpsr = *pfpsr |
                  ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 12));

              // caller will advance instruction pointer
              return (FALSE | FAULT_TO_TRAP); // will raise I trap

            }

          // Case (VIII), Case (IX), Case (X), Case (XI), and Case (XII)
          // 0 < |a/b| < SMALLEST NORMAL ==> might have U or I traps
          // As a/b cannot contain more than N-1 consecutive 1's or N-1
          // consecutive 0's, even if fpa = 1 is added in the first IEEE
          // rounding, that does not change the quotient's exponent

          } else if ( (exponent_b == exponent_a - FP_REG_EMIN) &&
            (significand_a < significand_b) ||
            (exponent_b >= exponent_a - FP_REG_EMIN + 1)) {

#ifdef DEBUG_UNIX
printf ("DEBUG: BEGIN F6 SWA FAULT CASE (VIII) - (XII)\n");
#endif
            // scale a to a' and b to b', such that c' = a'/ b' will be 
            // normal

            // set the scaled (possibly normalized) value of a' (sign ok)
            ps->state_FR[lf2].exponent = (EM_uint_t)(0xffff);
            ps->state_FR[lf2].significand = significand_a;

            // set the scaled (possibly normalized) value of b' (sign ok)
            ps->state_FR[lf3].exponent = (EM_uint_t)(0xffff);
            ps->state_FR[lf3].significand = significand_b;

            // convert a' and b' to FLOAT128
            a_float128 = FPRegToFP128 (ps->state_FR[lf2]);
            b_float128 = FPRegToFP128 (ps->state_FR[lf3]);

            // invoke the divide algorithm to calculate c' = a' / b';
            // the algorithm uses sf0 with user settings, and sf1 with 
            // rn, 64-bits, wre, traps disabled;
            // copy FPSR.sfx with clear flags to FPSR1.sf0; rn,64,wre in sf1
            FPSR1 = (EM_uint64_t)((FPSR >> ((EM_uint_t)sf * 13)) & 0x01fc0)
                | 0x000000000270003f; // set sf0,sf1 and disable fp exceptions
            thmF (&FPSR1, &a_float128, &b_float128, &c_float128);
            I_flag = FPSR1 & 0x40000 ? 1 : 0;

            c1_float128 = c_float128;

            if (I_flag == 1) {
              // calculate d' = |b'| * |c'| - |a'| to determine fpa (used
              // only if a U or I exception will be raised)
              c1_float128.hiFlt64 = c1_float128.hiFlt64 & 
                  CONST_FORMAT(0x000000000001ffff); // take |c'|
              b_float128.hiFlt64 = b_float128.hiFlt64 & 
                  CONST_FORMAT(0x000000000001ffff); // take |b'|
              a_float128.hiFlt64 = a_float128.hiFlt64 & 
                  CONST_FORMAT(0x000000000001ffff); // take |a'|
              FPSR1 = CONST_FORMAT(0x00000000000003bf); // rn,64,wre=1,dis
              run_fms (&FPSR1, &d_float128, &b_float128, &c1_float128,
                  &a_float128); // d' = |b'| * |c'| - |a'|
              if (d_float128.hiFlt64 & CONST_FORMAT(0x020000)) {
                // if d' < 0, I = 1 and fpa = 0
                fpa = 0;
              } else {
                // if d' > 0, I = 1 and fpa = 1
                fpa = 1;
              }
            } else { // if (I_flag == 0) fpa = 0
              fpa = 0;
            }

            if (!U_dis) {

              // underflow exceptions are enabled => compute the result, and
              // propagate an underflow exception (deliver the result with
              // the exponent mod 2^17

              // convert c' (normal fp#) from FLOAT128 to EM_fp_reg_type
              ps->state_FR[lf1] = FP128ToFPReg (c_float128);
              // scale c' to c and take the mod 2^17 exponent
              exponent_c = (EM_uint_t)ps->state_FR[lf1].exponent + 
                  exponent_a - exponent_b;
              ps->state_FR[lf1].exponent = exponent_c & 0x1ffff;

              ISRlow = 0x1001; // U = 1
              // set the values of I and fpa in ISRlow
              if (I_flag == 1) {
                if (fpa == 0) {
                  ISRlow = ISRlow | 0x2000; // I = 1
                } else {
                  ISRlow = ISRlow | 0x6000; // I = 1, fpa = 1
                }
              }

              *pisr = ((*pisr) & 0xffffffffffff0000) | ISRlow;

              // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG Case (VIII) - (XII) AFTER F6 SWA FAULT 7: ps->state_FR[lf1] = %x %x %Lx\n",
ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
ps->state_FR[lf1].significand);
#endif
              set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
              if (f1 < 32)
                *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
              else
                *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

              // ps->state_PR[p2] = 0; clear the output predicate
              // update *ppreds
              *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
#ifdef DEBUG_UNIX
printf ("DEBUG Case (VIII) - (XII) AFTER F6 SWA FAULT 7 a: *ppreds = %Lx\n",
*ppreds);
#endif
              // update *pfpsr
              // set D in FPSR.sfx if any of a and b was unnormal
              if (unnormal) {
                // set D = 1 in *pfpsr
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 8));
              }
              // set U = 1
              *pfpsr = *pfpsr | 
                  ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 11));
              // set I = 1 if I_flag = 1
              if (I_flag) {
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 12));
              }

              // caller will advance instruction pointer
              return (FALSE | FAULT_TO_TRAP); // will raise U trap

            } else  { // if (U_dis)

              // underflow exceptions are disabled

              // convert c' (normal fp#) from FLOAT128 to EM_fp_reg_type
              ps->state_FR[lf1] = FP128ToFPReg (c_float128);

              if (ftz == 0) {

                // denormalize
                sign_c = ps->state_FR[lf1].sign;
                exponent_c = ps->state_FR[lf1].exponent;
                significand_c = ps->state_FR[lf1].significand;
                if (fpa) significand_c = significand_c - 1;
                    // Note: if fpa = 1, significand_c cannot be 1.0...0,
                    // because it could not have been 1.1...1 before adding
                    // 1 to it (cannot have N consecutive 1's in the result);
                    // this means that significand_c - 1 above does not 
                    // require an exponent correction (it does not lose 
                    // the J-bit)
                true_bexp = exponent_c + exponent_a - exponent_b;
                // true_bexp - 0x0ffff is the true unbiased exponent after
                // the first IEEE rounding

                // perform the second IEEE rounding
    
                significand_size = N64;
                shift_cnt = FP_REG_EMIN - true_bexp + 0x0ffff;

                if (shift_cnt <= significand_size) {
                  // do the actual shift to denormalize the result; the 
                  // result will be a denormal, or zero
                  round = I_flag;
                  sticky = 0;
                  for (ind = 0 ; ind < shift_cnt ; ind++) {
                    sticky = round | sticky;
                    round = (EM_uint_t)(significand_c & 0x01);
                    significand_c = significand_c >> 1;
                  }
                  true_bexp = true_bexp + shift_cnt; // e_min + 0xffff
                } else { // all the significand bits shift out into sticky
                  significand_c = 0;
                  round = 0;
                  sticky = 1;
                  true_bexp = true_bexp + shift_cnt; // e_min + 0xffff
                }

                // perform the rounding; the result is 0, denormal, or 
                // 1.0 x 2^emin
                switch (rc) {
                  case rc_rn:
                    lsb = (EM_uint_t)(significand_c & 0x01);
                    fpa = round & (lsb | sticky);
                    break;
                  case rc_rm:
                    fpa = (sign_c == 0 ? 0 : (round | sticky));
                    break;
                  case rc_rp:
                    fpa = (sign_c == 1 ? 0 : (round | sticky));
                    break;
                  case rc_rz:
                    fpa = 0;
                    break;
                  default:
#ifndef unix
                    FP_EMULATION_ERROR1 ("fp_emulate () Internal \
Error: invalid rc = %d\n", rc);
#else
                    FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
invalid rc = %d\n", rc);
                    return (FP_EMUL_ERROR);
#endif
                }


                // add fpa to the significand if fpa = 1
                if (fpa == 1) {
                  significand_c = significand_c + 1;
                }
    
                if (significand_c == 0) {
                  true_bexp = 0; // ow it is e_min
                }
    
                exponent_c = true_bexp;
    
                // determine the new value of I_flag (must be 1)
                I_flag = round | sticky; // not used except for check below
                // ps->state_FR[lf1].sign unchanged
                ps->state_FR[lf1].exponent = exponent_c;
                ps->state_FR[lf1].significand = significand_c;
 
              } else { // if ftz == 1

                fpa = 0;
                // ps->state_FR[lf1].sign unchanged
                ps->state_FR[lf1].exponent = 0;
                ps->state_FR[lf1].significand = 0;
                I_flag = 1;

              }

              // update *pfpsr
              // set D in FPSR.sfx if any of a and b was unnormal
              if (unnormal) {
                // set D = 1 in *pfpsr
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 8));
              }
              if (I_flag) {
                // set U = 1
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 11));
                // set I = 1
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 12));
              }

              // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG Case (VIII) - (XII) AFTER F6 SWA FAULT 8: ps->state_FR[lf1] = %x %x %Lx\n",
ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent,
ps->state_FR[lf1].significand);
#endif
              set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
              if (f1 < 32)
                *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
              else
                *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit
 
              // ps->state_PR[p2] = 0; clear the output predicate
              // update *ppreds [as if O disabled]
              *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
#ifdef DEBUG_UNIX
printf ("DEBUG Case (VIII) - (XII) AFTER F6 SWA FAULT 8 a: *ppreds = %Lx\n",
*ppreds);
#endif

              if (I_flag && !I_dis) {

                // underflow exceptions are disabled, but the inexact 
                // exceptions are enabled and the result is inexact => 
                // provide the IEEE mandated result, and
                // propagate an inexact exception

                ISRlow = 0x2001; // I = 1
                if (fpa) ISRlow = ISRlow | 0x4000; // fpa = 1
                *pisr = ((*pisr) & 0xffffffffffff0000) | ISRlow;

                // caller will advance instruction pointer
                return (FALSE | FAULT_TO_TRAP); // will raise I trap

              } // else no trap (tiny and inexact result)
    
              return (TRUE);

            }

          // Case (XIII)
          } else { 

#ifdef DEBUG_UNIX
printf ("DEBUG: BEGIN F6 SWA FAULT CASE (XIII)\n");
#endif

            // this must be a Merced specific SWA fault (e.g. for single,
            // double, or double-extended denormals)
            frcpa (ps, sf, qp, lf1, p2, lf2, lf3);

            if ((ps->state_MERCED_RTL >> 16) & 0x0ffff) goto new_exception;

            *pfpsr =  ps->state_AR[0].uint_value;
            // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG Case (XIII) AFTER F6 SWA FAULT 14: ps->state_FR[lf1] = %x %x %Lx\n",
ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
ps->state_FR[lf1].significand);
#endif
            set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
            if (f1 < 32)
              *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
            else
              *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

            // update ps->state_PR[p2] = 1;
            *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
            *ppreds |=
                (((EM_uint64_t)(ps->state_PR[p2] & 0x01)) << (EM_uint_t)p2);
#ifdef DEBUG_UNIX
printf ("DEBUG Case (XIII) AFTER F6 SWA FAULT 14 a: *ppreds = %Lx\n",
*ppreds);
#endif
            return (TRUE);

          }

          break;

        case FPRCPA_PATTERN:

          // should get here only for denormal inputs
          fprcpa (ps, sf, qp, lf1, p2, lf2, lf3);

          if ((ps->state_MERCED_RTL >> 16) & 0x0ffff) goto new_exception;

          *pfpsr =  ps->state_AR[0].uint_value;
          // set the destination floating-point and predicate 
          // reg values (redundant, as the output predicate will be cleared)
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F6 FPRCPA SWA FAULT 15: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif
          set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
          if (f1 < 32)
            *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
          else
            *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

          // update ps->state_PR[p2] = 0; clear the output predicate
          *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
          *ppreds |= 
              (((EM_uint64_t)(ps->state_PR[p2] & 0x01)) << (EM_uint_t)p2);
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F6 FPRCPA SWA FAULT 15 a: *ppreds = %Lx\n",
  *ppreds);
#endif
        return (TRUE);

      }

    } else if ((OpCode & F7_MIN_MASK) == F7_PATTERN) {
      // F7 instruction

      switch (OpCode & F7_MASK) {

        case FRSQRTA_PATTERN:
          SIMD_instruction = 0;
          break;
        case FPRSQRTA_PATTERN:
          SIMD_instruction = 1;
          break;
        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode);
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode);
          return (FP_EMUL_ERROR);
#endif
      }

      // extract the rounding mode
      rc = (EM_sf_rc_type)((FPSR >> (6 + 4 + 13 * (EM_uint_t)sf)) & 0x03);

      // extract p2, f3, and f1
      p2 = (EM_uint_t)((OpCode >> 27) & CONST_FORMAT(0x00000000003f));
      if (p2 >= 16) p2 = 16 + (rrbpr + p2 - 16) % 48;
#ifdef DEBUG_UNIX
printf ("DEBUG F7 instruction: p2 = %x\n", p2);
#endif
      f3 = (EM_uint_t)((OpCode >> 20) & CONST_FORMAT(0x00000000007F));
      if (f3 >= 32) f3 = 32 + (rrbfr + f3 - 32) % 96;
      f1 = (EM_uint_t)((OpCode >>  6) & CONST_FORMAT(0x00000000007F));
      if (f1 >= 32) f1 = 32 + (rrbfr + f1 - 32) % 96;

      // get source floating-point reg value
      ps->state_FR[lf3] = FP128ToFPReg (get_fp_register (f3, fp_state));
#ifdef DEBUG_UNIX
printf ("DEBUG BEFORE F7 SWA FAULT: ps->state_FR[lf3] = %x %x %Lx\n",
  ps->state_FR[lf3].sign, ps->state_FR[lf3].exponent, ps->state_FR[lf3].significand);
#endif

      switch (OpCode & F7_MASK) {

        case FRSQRTA_PATTERN:

          // extract sign, exponent, and significand of a
          // note that a is (a non-zero positive normal, or a positive
          // pseudo-zero or unnormal/denormal fp#), or (a negative pseudo-zero
          // or non-zero unnormal/denormal fp#)
          sign_a = (EM_uint_t)ps->state_FR[lf3].sign;
          exponent_a = (EM_int_t)ps->state_FR[lf3].exponent;
          significand_a = ps->state_FR[lf3].significand;
          if (exponent_a == 0 && significand_a != 0) exponent_a = 0xc001;

          unnormal = 0;
          if (!(significand_a & CONST_FORMAT(0x8000000000000000))) {
            unnormal = 1;
          }
#ifdef DEBUG_UNIX
if (unnormal) printf ("DEBUG F7 FRSQRTA SWA FAULT: unnormal = 1\n");
#endif

          // raise a D trap if an unnormal, but not a non-zero negative one
          if (unnormal && !D_dis && !(sign_a == 1 && significand_a != 0)) {
            ISRlow = 0x0002; // denormal
            *pisr = ((*pisr) & 0xffffffffffff0000) | ISRlow;
            return (FALSE); // will raise D trap
          }

          // if a pseudo-zero or negative non-zero, return the result
          if (exponent_a != 0 && significand_a == 0 || 
              sign_a == 1 && significand_a != 0) { 
              // a is pseudo-zero or negative non-zero

            frsqrta (ps, sf, qp, lf1, p2, lf3);

            if ((ps->state_MERCED_RTL >> 16) & 0x0ffff) goto new_exception;

            *pfpsr =  ps->state_AR[0].uint_value;

            // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F7 SWA FAULT FOR PSEUDO-ZERO OR -DEN: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent,
  ps->state_FR[lf1].significand);
#endif
            set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
            if (f1 < 32)
              *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
            else
              *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

            // ps->state_PR[p2] = 0 for zero or negative argument
            *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
            *ppreds |= 
                (((EM_uint64_t)(ps->state_PR[p2] & 0x01)) << (EM_uint_t)p2);
#ifdef DEBUG_UNIX
printf ("DEBUG  AFTER F7 SWA FAULT FOR PSEUDO-ZERO OR -DEN: p2 = %x\n", p2);
#endif
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F7 SWA FAULT FOR PSEUDO-ZERO OR -DEN: *ppreds = %Lx\n", *ppreds);
#endif
            return (TRUE);

          }

          // normalize a (even if exponent_a becomes less than e_min)
          while (!(significand_a & CONST_FORMAT(0x8000000000000000))) {
            significand_a = significand_a << 1;
            exponent_a--;
          }

          // Case (I)
          // sqrt (a) is normal ==> might have an I trap

          if (exponent_a - 0xffff <= FP_REG_EMIN + N64 - 1) {

            // scale a to a', such that s' = sqrt (a') will be normal

            // set the scaled (and possibly normalized) value of a' (sign ok)
            if (exponent_a % 2 != 0) // exponent_a is biased
              ps->state_FR[lf3].exponent = (EM_uint_t)0xffff;
            else
              ps->state_FR[lf3].exponent = (EM_uint_t)0x10000;
            ps->state_FR[lf3].significand = significand_a;

            // convert a' to FLOAT128
            a_float128 = FPRegToFP128 (ps->state_FR[lf3]);

            // invoke the square root algorithm to calculate s' = sqrt (a');
            // the algorithm uses sf0 with user settings, and sf1 with
            // rn, 64-bits, wre, traps disabled;
            // the current FPSR is not affected
            // copy FPSR.sfx with clear flags to FPSR1.sf0; rn,64,wre in sf1
            FPSR1 = (EM_uint64_t)((FPSR >> ((EM_uint_t)sf * 13)) & 0x01fc0) |
                0x000000000270003f; // set sf0,sf1 and disable fp exceptions
            thmL (&FPSR1, &a_float128, &s_float128);
            I_flag = FPSR1 & 0x40000 ? 1 : 0;

            // convert s' (normal fp#) from FLOAT128 to EM_fp_reg_type
            ps->state_FR[lf1] = FP128ToFPReg (s_float128);

            // scale s' to s
            if (exponent_a % 2 != 0) // exponent_a is biased
              ps->state_FR[lf1].exponent += ((exponent_a - 0xffff) / 2);
            else
              ps->state_FR[lf1].exponent += ((exponent_a - 0xffff - 1) / 2);

            if (I_dis || !I_flag) {

              // set D in FPSR.sfx if unnormal operand (and denormal exeptions
              // are disabled)
              if (unnormal) { // if (unnormal && D_dis)
                // set D in FPSR.sfx (set D = 1 in *pfpsr)
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 8));
              }
              if (I_flag) { // set I in FPSR
                // set I = 1 in *pfpsr
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 12));
              }

              // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG Case (I) AFTER F7 SWA FAULT 1: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif
              set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
              if (f1 < 32)
                *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
              else
                *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

              // ps->state_PR[p2] = 0; clear the output predicate
              *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
#ifdef DEBUG_UNIX
printf ("DEBUG Case (I) AFTER F7 SWA FAULT 1 a: *ppreds = %Lx\n",
  *ppreds);
#endif
              return (TRUE);

            } else { // if (!I_dis && I_flag)

              // determine fpa, and set the values of I and fpa in ISRlow
              // calculate d' = s' * s' - a' to determine fpa
              FPSR1 = CONST_FORMAT(0x00000000000003bf);
              run_fms (&FPSR1, &d_float128, &s_float128, &s_float128, 
                  &a_float128); // d' = s' * s' - a'

              if (d_float128.hiFlt64 & CONST_FORMAT(0x0000000000020000)) {
                // if d' < 0, I = 1 and fpa = 0
                ISRlow = 0x2001;
              } else {
                // if d' > 0, I = 1 and fpa = 1
                ISRlow = 0x6001;
              }

              *pisr = ((*pisr) & 0xffffffffffff0000) | ISRlow;

              // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG Case (I) AFTER F7 SWA FAULT 2: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif
              set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
              if (f1 < 32)
                *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
              else
                *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

              //update *ppreds
              // ps->state_PR[p2] = 0; clear the output predicate
              *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
#ifdef DEBUG_UNIX
printf ("DEBUG Case (I) AFTER F7 SWA FAULT 2 a: *ppreds = %Lx\n",
  *ppreds);
#endif
              // set D in FPSR.sfx if unnormal operand (and denormal exeptions
              // are disabled)
              if (unnormal) { // if (unnormal && D_dis)
                // set D in FPSR.sfx (set D = 1 in *pfpsr)
                *pfpsr = *pfpsr |
                    ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 8));
              }
              // update: *pfpsr: set I = 1
              *pfpsr = *pfpsr |
                  ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 12));

              // caller will advance instruction pointer
              return (FALSE | FAULT_TO_TRAP); // will raise I trap

            }

          // Case (II)
          } else {

            frsqrta (ps, sf, qp, lf1, p2, lf3);

            if ((ps->state_MERCED_RTL >> 16) & 0x0ffff) goto new_exception;

            *pfpsr =  ps->state_AR[0].uint_value;
            // set the destination floating-point and predicate reg values
#ifdef DEBUG_UNIX
printf ("DEBUG Case (II) AFTER F7 SWA FAULT 3: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif
            set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
            if (f1 < 32)
              *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
            else
              *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

            // update ps->state_PR[p2] = 1 for positive arguments only
            *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
            *ppreds |=
                (((EM_uint64_t)(ps->state_PR[p2] & 0x01)) << (EM_uint_t)p2);
#ifdef DEBUG_UNIX
printf ("DEBUG  Case (II) AFTER F7 SWA FAULT 3 a: p2 = %x\n", p2);
#endif
#ifdef DEBUG_UNIX
printf ("DEBUG Case (II) AFTER F7 SWA FAULT 3 a: *ppreds = %Lx\n", *ppreds);
#endif
            return (TRUE);

          }

          break;

        case FPRSQRTA_PATTERN:

          // should get here only for denormal inputs
          fprsqrta (ps, sf, qp, lf1, p2, lf3);

          if ((ps->state_MERCED_RTL >> 16) & 0x0ffff) goto new_exception;

          *pfpsr =  ps->state_AR[0].uint_value;
          // set the destination floating-point and predicate 
          // reg values (redundant, as the output predicate will be cleared)
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F7 FPRSQRTA SWA FAULT 4: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif
          set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
          if (f1 < 32)
            *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
          else
            *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

          // update ps->state_PR[p2] = 0; clear the output predicate
          *ppreds &= (~(((EM_uint64_t)1) << (EM_uint_t)p2));
          *ppreds |= 
              (((EM_uint64_t)(ps->state_PR[p2] & 0x01)) << (EM_uint_t)p2);
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F7 FPRSQRTA SWA FAULT 4 a: *ppreds = %Lx\n", *ppreds);
#endif
          return (TRUE);

      }

    } else if ((OpCode & F8_MIN_MASK) == F8_PATTERN) {
      // F8 instruction

      // extract f3, f2, and f1
      f3 = (EM_fp_reg_specifier)((OpCode >> 20) & CONST_FORMAT(0x00000000007F));
      if (f3 >= 32) f3 = 32 + (rrbfr + f3 - 32) % 96;
      f2 = (EM_fp_reg_specifier)((OpCode >> 13) & CONST_FORMAT(0x00000000007F));
      if (f2 >= 32) f2 = 32 + (rrbfr + f2 - 32) % 96;
      f1 = (EM_fp_reg_specifier)((OpCode >>  6) & CONST_FORMAT(0x00000000007F));
      if (f1 >= 32) f1 = 32 + (rrbfr + f1 - 32) % 96;

      // get source floating-point reg values
      ps->state_FR[lf2] = FP128ToFPReg (get_fp_register (f2, fp_state));
      ps->state_FR[lf3] = FP128ToFPReg (get_fp_register (f3, fp_state));
#ifdef DEBUG_UNIX
printf ("DEBUG BEFORE F8 SWA FAULT: ps->state_FR[lf2] = %x %x %Lx\n",
  ps->state_FR[lf2].sign, ps->state_FR[lf2].exponent, ps->state_FR[lf2].significand);
printf ("DEBUG BEFORE F8 SWA FAULT: ps->state_FR[lf3] = %x %x %Lx\n",
  ps->state_FR[lf3].sign, ps->state_FR[lf3].exponent, ps->state_FR[lf3].significand);
#endif

      switch (OpCode & F8_MASK) {

        case FMIN_PATTERN:
          SIMD_instruction = 0;
          fmin (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FMAX_PATTERN:
          SIMD_instruction = 0;
          fmax (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FAMIN_PATTERN:
          SIMD_instruction = 0;
          famin (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FAMAX_PATTERN:
          SIMD_instruction = 0;
          famax (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FPMIN_PATTERN:
          SIMD_instruction = 1;
          fpmin (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FPMAX_PATTERN:
          SIMD_instruction = 1;
          fpmax (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FPAMIN_PATTERN:
          SIMD_instruction = 1;
          fpamin (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FPAMAX_PATTERN:
          SIMD_instruction = 1;
          fpamax (ps, sf, qp, lf1, lf2, lf3);
          break;

        case FPCMP_EQ_PATTERN:
          SIMD_instruction = 1;
          fpcmp_eq (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FPCMP_LT_PATTERN:
          SIMD_instruction = 1;
          fpcmp_lt (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FPCMP_LE_PATTERN:
          SIMD_instruction = 1;
          fpcmp_le (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FPCMP_UNORD_PATTERN:
          SIMD_instruction = 1;
          fpcmp_unord (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FPCMP_NEQ_PATTERN:
          SIMD_instruction = 1;
          fpcmp_neq (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FPCMP_NLT_PATTERN:
          SIMD_instruction = 1;
          fpcmp_nlt (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FPCMP_NLE_PATTERN:
          SIMD_instruction = 1;
          fpcmp_nle (ps, sf, qp, lf1, lf2, lf3);
          break;
        case FPCMP_ORD_PATTERN:
          SIMD_instruction = 1;
          fpcmp_ord (ps, sf, qp, lf1, lf2, lf3);
          break;

        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode);
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode);
          return (FP_EMUL_ERROR);
#endif
      }

      if ((ps->state_MERCED_RTL >> 16) & 0x0ffff) goto new_exception;

      // successful emulation
      // set the destination floating-point reg value
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F8 SWA FAULT: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif
      set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
      if (f1 < 32)
        *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
      else
        *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

      *pfpsr =  ps->state_AR[0].uint_value;
      return (TRUE);

    } else if ((OpCode & F10_MIN_MASK) == F10_PATTERN) {
      // F10 instruction

      // extract f2 and f1
      f2 = (EM_uint_t)((OpCode >> 13) & CONST_FORMAT(0x00000000007F));
      if (f2 >= 32) f2 = 32 + (rrbfr + f2 - 32) % 96;
      f1 = (EM_uint_t)((OpCode >>  6) & CONST_FORMAT(0x00000000007F));
      if (f1 >= 32) f1 = 32 + (rrbfr + f1 - 32) % 96;

      // get source floating-point reg value
      ps->state_FR[lf2] = FP128ToFPReg (get_fp_register (f2, fp_state));
#ifdef DEBUG_UNIX
printf ("DEBUG BEFORE F10 SWA FAULT: ps->state_FR[lf2] = %x %x %Lx\n",
  ps->state_FR[lf2].sign, ps->state_FR[lf2].exponent, ps->state_FR[lf2].significand);
#endif

      switch (OpCode & F10_MASK) {

        case FCVT_FX_PATTERN:
          SIMD_instruction = 0;
          fcvt_fx (ps, sf, qp, lf1, lf2);
          break;
        case FCVT_FXU_PATTERN:
          SIMD_instruction = 0;
          fcvt_fxu (ps, sf, qp, lf1, lf2);
          break;
        case FCVT_FX_TRUNC_PATTERN:
          SIMD_instruction = 0;
          fcvt_fx_trunc (ps, sf, qp, lf1, lf2);
          break;
        case FCVT_FXU_TRUNC_PATTERN:
          SIMD_instruction = 0;
          fcvt_fxu_trunc (ps, sf, qp, lf1, lf2);
          break;
        case FPCVT_FX_PATTERN:
          SIMD_instruction = 1;
          fpcvt_fx (ps, sf, qp, lf1, lf2);
          break;
        case FPCVT_FXU_PATTERN:
          SIMD_instruction = 1;
          fpcvt_fxu (ps, sf, qp, lf1, lf2);
          break;
        case FPCVT_FX_TRUNC_PATTERN:
          SIMD_instruction = 1;
          fpcvt_fx_trunc (ps, sf, qp, lf1, lf2);
          break;
        case FPCVT_FXU_TRUNC_PATTERN:
          SIMD_instruction = 1;
          fpcvt_fxu_trunc (ps, sf, qp, lf1, lf2);
          break;
        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode);
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode);
          return (FP_EMUL_ERROR);
#endif
      }

      if ((ps->state_MERCED_RTL >> 16) & 0x0ffff) goto new_exception;

      // successful emulation
      // set the destination floating-point reg value
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F10 SWA FAULT: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif
      set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
      if (f1 < 32)
        *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
      else
        *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

      *pfpsr =  ps->state_AR[0].uint_value;
      return (TRUE);

    } else {

      // unrecognized instruction type
#ifndef unix
      FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode);
#else
      FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode);
      return (FP_EMUL_ERROR);
#endif
    }

    // advance instruction pointer in the trap frame

    return (TRUE);

    // end 'if ((trap_type == FPFLT) && (ISRlow & 0x0088))'

  } else if ((trap_type == FPTRAP) && swa_trap (sf, FPSR, ISRlow)) {

    // else if this is a SWA trap

    // this can only happen in one situation for Merced at the present time:
    // for a tiny result (which can occur only for an F1 instruction), when the
    // underflow exceptions are disabled; the IA-64 FP Emulation Library also
    // handles correctly the situations in which a huge result occurs, and the
    // overflow exceptions are disabled, or when an inexact result occurs, and
    // the inexact exceptions are disabled

    // shortcut the case when the result is inexact, and the inexact exceptions
    // are disabled
    if ((ISRlow & 0x1980) == 0) return (TRUE); // nothing to do in this case

    // Note: overflow, which can also be caused only by an F1 instruction
    // (but not on Merced) is included too; 

    // Note that for Merced, if a SWA trap occurs, fp_emulate () is
    // entered with U exceptions disabled and the U flag set in the ISR code;
    // the I flag can be set or not (unlike the U flag in the FPSR in the
    // absence of a trap, which will be set together with the I flag when
    // U traps are disabled [with U traps disabled, the result has to be
    // tiny and inexact for the U flag to be set - the I flag will be set 
    // too])

    // SIMD_instruction unchanged at 2

    // decode the rest of the instruction
    if ((OpCode & F1_MIN_MASK) == F1_PATTERN) {
      // F1 instruction

      // extract f1
      f1 = (EM_uint_t)((OpCode >>  6) & CONST_FORMAT(0x00000000007F));
      if (f1 >= 32) f1 = 32 + (rrbfr + f1 - 32) % 96;
#ifdef DEBUG_UNIX
printf ("DEBUG BEFORE F1 SWA TRAP: f1 = %x\n", f1);
#endif

      // no need to get the source floating-point reg values - they 
      // might have changed

      // read the destination reg f1
      ps->state_FR[lf1] = FP128ToFPReg (get_fp_register (f1, fp_state));
#ifdef DEBUG_UNIX
printf ("DEBUG BEFORE F1 SWA TRAP: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif

      switch (OpCode & F1_MASK) {

        case FMA_PATTERN:
        case FMS_PATTERN:
        case FNMA_PATTERN:
          opcode_pc = pc_sf;
          break;
        case FMA_S_PATTERN:
        case FMS_S_PATTERN:
        case FNMA_S_PATTERN:
          opcode_pc = pc_s;
          break;
        case FMA_D_PATTERN:
        case FMS_D_PATTERN:
        case FNMA_D_PATTERN:
          opcode_pc = pc_d;
          break;
        case FPMA_PATTERN:
        case FPMS_PATTERN:
        case FPNMA_PATTERN:
          opcode_pc = pc_simd;
          break;

        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode);
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode);
          return (FP_EMUL_ERROR);
#endif
      }

      // SWA trap - software assistance required

      // implement table 3-5 of EM EAS 2.1
      rc = (EM_sf_rc_type)((FPSR >> (6 + 4 + 13 * (EM_uint_t)sf)) & 0x03);
      pc = (EM_sf_pc_type)((FPSR >> (6 + 2 + 13 * (EM_uint_t)sf)) & 0x03);
      wre = (EM_uint_t)((FPSR >> (6 + 1 + 13 * (EM_uint_t)sf)) & 0x01);

      // determine the precision of the result, the size of the exponent,
      // and emin
      if (opcode_pc == pc_simd || opcode_pc == pc_s && wre == 0) {
        significand_size = 24;
        emin = EMIN_08_BITS;
      } else if (opcode_pc == pc_d && wre == 0) {
        significand_size = 53;
        emin = EMIN_11_BITS;
      } else if (opcode_pc == pc_s && wre == 1) {
        significand_size = 24;
        emin = EMIN_17_BITS;
      } else if (opcode_pc == pc_d && wre == 1) {
        significand_size = 53;
        emin = EMIN_17_BITS;
      } else if (opcode_pc == pc_sf) {
        if (pc == sf_single && wre == 0) {
          significand_size = 24;
          emin = EMIN_15_BITS;
        } else if (pc == sf_double && wre == 0) {
          significand_size = 53;
          emin = EMIN_15_BITS;
        } else if (pc == sf_double_extended && wre == 0) {
          significand_size = 64;
          emin = EMIN_15_BITS;
        } else if (pc == sf_single && wre == 1) {
          significand_size = 24;
          emin = EMIN_17_BITS;
        } else if (pc == sf_double && wre == 1) {
          significand_size = 53;
          emin = EMIN_17_BITS;
        } else if (pc == sf_double_extended && wre == 1) {
          significand_size = 64;
          emin = EMIN_17_BITS;
        } else {
#ifndef unix
          FP_EMULATION_ERROR0 ("fp_emulate () internal error in \
determining the computation model\n");
        }
      } else {
        FP_EMULATION_ERROR0 ("fp_emulate () internal error in \
determining the computation model\n");
#else
          FP_EMULATION_ERROR0 ("fp_emulate () internal error in \
determining the computation model\n");
          return (FP_EMUL_ERROR);
        }
      } else {
        FP_EMULATION_ERROR0 ("fp_emulate () internal error in \
determining the computation model\n");
        return (FP_EMUL_ERROR);
#endif
      }

      tmp_fp = ps->state_FR[lf1];

      // Note: if the cause of the SWA trap is O or U, cannot get here with
      // zero or a denormal in FR[f1]; if U, the result in FR[f1] will have
      // to be denormalized
      if (opcode_pc != pc_simd) { // non-SIMD instruction

        fpa = (ISRlow >> 14) & 0x01;
        I_exc = (ISRlow >> 13) & 0x01; // inexact = round OR sticky
        U_exc = (ISRlow >> 12) & 0x01; // underflow
        O_exc = (ISRlow >> 11) & 0x01; // overflow

        sign = tmp_fp.sign;
        exponent = tmp_fp.exponent;
        significand = tmp_fp.significand;

        // calculate the true biased exponent, and then the true exponent
        if (U_dis && U_exc) { // the result is not zero, and is normal with
            // unbounded exponent (tiny result)

          decr_exp = 0;
  
          // extract the number of significand bits specified by the 
          // destination precision, and determine the significand, before 
          // the rounding performed in hardware (1st IEEE rounding)
  
          if (significand_size == 64) {
            if (fpa == 1) {
              significand = significand - 1;
              if (significand == CONST_FORMAT(0x07fffffffffffffff)) {
                significand = CONST_FORMAT(0x0ffffffffffffffff);
                decr_exp = 1;
              }
            }
          } else if (significand_size == 53) {
            // the 53 bits are already there, but need to be shifted right
            significand = significand >> 11;
            if (fpa == 1) {
              significand = significand - 1;
              if (significand == CONST_FORMAT(0x0fffffffffffff)) {
                significand = CONST_FORMAT(0x01fffffffffffff);
                decr_exp = 1;
              }
            }
          } else if (significand_size == 24) {
            // the 24 bits are already there, but need to be shifted right
            significand = significand >> 40;
            if (fpa == 1) {
              significand = significand - 1;
              if (significand == CONST_FORMAT(0x07fffff)) {
                significand = CONST_FORMAT(0x0ffffff);
                decr_exp = 1;
              }
            }
          } else {
            // internal error
#ifndef unix
            FP_EMULATION_ERROR6 ("fp_emulate (): incorrect \
  significand size %d for ISRlow = %4.4x and FR[%d] = %1.1x %5.5x %8.8x %8.8x\n",
                significand_size, ISRlow, f1, tmp_fp.sign, tmp_fp.exponent,
                tmp_fp.significand)
#else
            FP_EMULATION_ERROR6 ("fp_emulate (): incorrect \
  significand size %d for ISRlow = %4.4x and FR[%d] = %1.1x %5.5x %Lx\n",
                significand_size, ISRlow, f1, tmp_fp.sign, tmp_fp.exponent,
                tmp_fp.significand)
                return (FP_EMUL_ERROR);
#endif
          }

          true_bexp = ((exponent + 0x1007b) & 0x1ffff) - 0x1007b;

          // true_bexp - 0x0ffff is the true unbiased exponent after the
          // first IEEE rounding; determine whether the result is tiny
          if (true_bexp - 0x0ffff > emin - 1) {
#ifndef unix
            FP_EMULATION_ERROR0 ("fp_emulate () Internal Error: non-tiny res\n");
#else
            FP_EMULATION_ERROR0 ("fp_emulate () Internal Error: non-tiny res\n");
            return (FP_EMUL_ERROR);
#endif
          }

          // adjust now true_bexp if necessary
          if (decr_exp) true_bexp--;

          // perform the second IEEE rounding

          shift_cnt = emin - true_bexp + 0x0ffff; // >= 1

          if (shift_cnt <= significand_size) {
            // do the actual shift to denormalize the result; the result
            // will be a denormal, or zero
            round = I_exc;
                // this is indicated even for O or U if the result is inexact
            sticky = 0;
            for (ind = 0 ; ind < shift_cnt ; ind++) {
              sticky = round | sticky;
              round = (EM_uint_t)(significand & 0x01);
              significand = significand >> 1;
            }
            true_bexp = true_bexp + shift_cnt; // e_min + 0xffff
          } else { // all the significand bits shift out into sticky
            significand = 0;
            round = 0;
            sticky = 1;
            true_bexp = true_bexp + shift_cnt; // e_min + 0xffff
          }

          // perform the rounding; the result is 0, denormal, or 
          // 1.0 x 2^emin
          switch (rc) {
            case rc_rn:
              lsb = (EM_uint_t)(significand & 0x01);
              fpa = round & (lsb | sticky);
              break;
            case rc_rm:
              fpa = (sign == 0 ? 0 : (round | sticky));
              break;
            case rc_rp:
              fpa = (sign == 1 ? 0 : (round | sticky));
              break;
            case rc_rz:
              fpa = 0;
              break;
            default:
#ifndef unix
              FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
invalid rc = %d\n", rc)
#else
              FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
invalid rc = %d\n", rc)
              return (FP_EMUL_ERROR);
#endif
          }


          // add fpa to the significand if fpa = 1, and fix for the case when 
          // there is a carry-out in this addition

          if (fpa == 1) {

            significand = significand + 1;

            if (significand_size == 64) {
              if (significand == CONST_FORMAT(0x0)) { // was 0xff....f
                significand = CONST_FORMAT(0x08000000000000000);
                true_bexp++; // e_min + 0xffff + 0x1
              }
            } else if (significand_size == 53) {
              if (significand == CONST_FORMAT(0x020000000000000)) {
                significand = CONST_FORMAT(0x010000000000000);
                true_bexp++; // e_min + 0xffff + 0x1
              }
            } else if (significand_size == 24) {
              if (significand == CONST_FORMAT(0x01000000)) {
                significand = CONST_FORMAT(0x0800000);
                true_bexp++; // e_min + 0xffff + 0x1
              }
            } else { // this case not really needed
              // internal error
#ifndef unix
              FP_EMULATION_ERROR1 (
                  "fp_emulate (): incorrect significand size %d\n",
                  significand_size)
#else
              FP_EMULATION_ERROR1 (
                  "fp_emulate (): incorrect significand size %d\n",
                  significand_size)
              return (FP_EMUL_ERROR);
#endif
            }

          }

          if (significand == 0) {
            true_bexp = 0; // ow it is e_min or e_min + 1
          }

          exponent = true_bexp;

          // determine the new value of I_exc
          I_exc = round | sticky;

          // set the new values for both the FPSR and the ISR code, but the
          // ISR code will only be used if an inexact exception will be
          // raised (for this, the final result generated by 
          // fp_emulate () has to be inexact, and the inexact traps have
          // to be enabled

          if (I_exc) {
            // if tiny and inexact
            /* set U and set I in FPSR */
            *pfpsr = *pfpsr |
                ((EM_uint64_t)3 << (6 + (EM_uint_t)sf * 13 + 11));
            if (!I_dis) { // update ISR code
              // clear U and set I in ISRlow - will raise an inexact trap
              ISRlow = (ISRlow & 0xefff) | 0x2000;
              // update the fpa bit in the ISR code (NOT DONE IN EM_FP82)
              if (fpa)
                ISRlow = ISRlow | 0x4000;
              else
                ISRlow = ISRlow & 0xbfff; 
            }
          }
 
          // shift left the significand if needed
          if (significand_size == 53)
            significand = significand << 11; // msb explicit
          else if (significand_size == 24)
            significand = significand << 40; // msb explicit

          // if the exponent is 0xc001 and the result is unnormal,
          // modify the exponent to 0x0000
          if (exponent == 0xc001 && ((significand & 0x8000000000000000) == 0))
              exponent = 0x0;

        } else if (O_dis && O_exc) { // the result is not zero, and is normal
            // with unbounded exponent

          // true_bexp not used in this case, and neither is decr_exp
          // true_bexp = ((exponent - 0x1007f) & 0x1ffff) + 0x1007f; 

          // determine the result, according to the rounding mode
          switch (rc) {
            case rc_rn: // +infinity or -infinity
              exponent = 0x01ffff;
              significand = CONST_FORMAT(0x8000000000000000);
              break;
            case rc_rm: // +max_fp or -infinity
              exponent = (sign == 0 ? 0x01fffe : 0x01ffff);
              significand = (sign == 0 ? CONST_FORMAT(0x0ffffffffffffffff) : 
                  CONST_FORMAT(0x8000000000000000));
              break;
            case rc_rp: // +infinity or -max_fp
              exponent = (sign == 0 ? 0x01ffff : 0x01fffe);
              significand = (sign == 0 ? CONST_FORMAT(0x8000000000000000) : 
                  CONST_FORMAT(0x0ffffffffffffffff));
              break;
            case rc_rz: // +max_fp or -max_fp
              exponent = 0x01fffe;
              significand = CONST_FORMAT(0x0ffffffffffffffff);
              break;
            default:
#ifndef unix
              FP_EMULATION_ERROR1 ("fp_emulate () Internal Error:\
 invalid rc = %d for non-SIMD F1 instruction\n", rc)
#else
              FP_EMULATION_ERROR1 ("fp_emulate () Internal Error:\
 invalid rc = %d for non-SIMD F1 instruction\n", rc)
              return (FP_EMUL_ERROR);
#endif
          }

          /* set O and set I in FPSR */
          *pfpsr = *pfpsr | ((EM_uint64_t)5 << (6 + (EM_uint_t)sf * 13 + 10));

          if (!I_dis) { // update ISR code - will raise an inexact exception

            if (exponent == 0x1ffff)
              fpa = 1;
            else
              fpa = 0;

            /* clear O and set I in ISRlow - the result is always inexact */
            ISRlow = (ISRlow & 0xf7ff) | 0x2000;
            // update the fpa bit in the ISR code (NOT DONE IN EM_FP82)
            if (fpa)
              ISRlow = ISRlow | 0x4000;
            else
              ISRlow = ISRlow & 0xbfff; 

            // Note that the exact result cannot be retrieved by the inexact
            // exception trap handler [but this will not occur on Merced]

          }

        } else if (I_dis && I_exc) {

          // redundant - this case was caught before
          ; // nothing to do

        } else {

          // internal error
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
SWA trap code invoked with F1 instruction, w/o O or U set in ISR.code = %x\n", 
              ISRlow & 0x0ffff)
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
SWA trap code invoked with F1 instruction, w/o O or U set in ISR.code = %x\n", 
              ISRlow & 0x0ffff)
          return (FP_EMUL_ERROR);
#endif
        }

        // return the result
        // FR[f1].sign is unchanged
        ps->state_FR[lf1].exponent = exponent;
        ps->state_FR[lf1].significand = significand;

        // successful emulation, but might still raise another trap - O, U, or I

        // set the destination floating-point reg value (this is a trap)
        // [redundant if the cause of the SWA trap was I && I_dis]
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F1 SWA TRAP 1: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F1 SWA TRAP: f1 = %x\n", f1);
#endif

        set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
        if (f1 < 32)
          *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
        else
          *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

        // return TRUE if no exception has to be raised
        if (I_dis || I_exc == 0) {

          return (TRUE);

        } else { 
          // if (inexact && I traps enabled)
          // ISRlow might have been updated
          *pisr = ((*pisr) & 0xffffffffffff0000) | ISRlow;

#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F1 SWA TRAP 1 A RET FALSE: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif
          return (FALSE); // will raise I trap

        }

      } else { // SIMD instruction

        // tmp_fp.exponent = 0x1003e

        fpa_hi = (ISRlow >> 14) & 0x01;
        I_exc_hi = (ISRlow >> 13) & 0x01; // inexact = round OR sticky
        U_exc_hi = (ISRlow >> 12) & 0x01; // underflow
        O_exc_hi = (ISRlow >> 11) & 0x01; // overflow
        fpa_lo = (ISRlow >> 10) & 0x01;
        I_exc_lo = (ISRlow >> 9) & 0x01; // inexact = round OR sticky
        U_exc_lo = (ISRlow >> 8) & 0x01; // underflow
        O_exc_lo = (ISRlow >> 7) & 0x01; // overflow

        if (!U_exc_lo && !U_exc_hi && !O_exc_lo && !O_exc_hi) {

          // internal error
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
SWA trap code invoked with SIMD F1 instruction, w/o O or U set in \
ISR.code = %x\n", ISRlow & 0x0ffff)
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
SWA trap code invoked with SIMD F1 instruction, w/o O or U set in \
ISR.code = %x\n", ISRlow & 0x0ffff)
          return (FP_EMUL_ERROR);
#endif

        }

        sign_lo = (EM_uint_t)((tmp_fp.significand >> 31) & 0x01);
        exponent_lo = (EM_uint_t)((tmp_fp.significand >> 23) & 0x0ff);
        significand_lo = (EM_uint_t)((tmp_fp.significand) & 0x07fffff);
        // never get here with a 0 or an unnormal result if a disabled
        // underflow or overflow occurred in the low half; if the low half
        // has neither an underflow nor an overflow, then the assignment
        // below might be incorrect, but significand_lo will be corrected when
        // the result is returned
        significand_lo = significand_lo | 0x800000; 

        sign_hi = (EM_uint_t)((tmp_fp.significand >> 63) & 0x01);
        exponent_hi = (EM_uint_t)((tmp_fp.significand >> 55) & 0x0ff);
        significand_hi = (EM_uint_t)((tmp_fp.significand >> 32) & 0x07fffff);
        // never get here with a 0 or an unnormal result if a disabled
        // underflow or overflow occurred in the high half; if the high half
        // has neither an underflow nor an overflow, then the assignment
        // below might be incorrect, but significand_hi will be corrected when
        // the result is returned
        significand_hi = significand_hi | 0x800000; 

        // if underflow or overflow, calculate the true biased exponent, by 
        // possibly adding or subtracting 2^8

        if (U_dis && U_exc_lo) { // the result is not zero, and is normal with
            // unbounded exponent (but tiny)

          decr_exp_lo = 0;
          // if SWA trap in the low half
          if (fpa_lo == 1) {
            significand_lo = significand_lo - 1;
            if (significand_lo == 0x07fffff) {
              significand_lo = 0x0ffffff;
              decr_exp_lo = 1;
            }
          }

          true_bexp_lo = (exponent_lo == 0 ? exponent_lo : exponent_lo - 0x100);

          // true_bexp_lo - 0x07f is the true unbiased exponent after the
          // first IEEE rounding; determine whether the result is tiny
          if (true_bexp_lo - 0x07f > emin - 1) {
#ifndef unix
            FP_EMULATION_ERROR0 ("fp_emulate () Internal Error:non-tiny resL\n");
#else
            FP_EMULATION_ERROR0 ("fp_emulate () Internal Error:non-tiny resL\n");
            return (FP_EMUL_ERROR);
#endif
          }

          // adjust now true_bexp_lo if necessary
          if (decr_exp_lo) true_bexp_lo--;

          // perform the second IEEE rounding

          shift_cnt_lo = emin - true_bexp_lo + 0x07f; // >= 1

          if (shift_cnt_lo <= significand_size) { // <= 24
            // do the actual shift to denormalize the result; the result
            // will be a denormal, or zero
            round_lo = I_exc_lo;
                // this is indicated even for O or U, if the result inexact
            sticky_lo = 0;
            for (ind = 0 ; ind < shift_cnt_lo ; ind++) {
              sticky_lo = round_lo | sticky_lo;
              round_lo = significand_lo & 0x01;
              significand_lo = significand_lo >> 1;
            }
            true_bexp_lo = true_bexp_lo + shift_cnt_lo; // e_min + 0x7f
          } else { // all the significand bits shift out into sticky
            significand_lo = 0;
            round_lo = 0;
            sticky_lo = 1;
            true_bexp_lo = true_bexp_lo + shift_cnt_lo; // e_min + 0x7f
          }

          // perform the rounding; the result is 0, denormal, or 
          // 1.0 x 2^emin
          switch (rc) {
            case rc_rn:
              lsb_lo = significand_lo & 0x01;
              fpa_lo = round_lo & (lsb_lo | sticky_lo);
              break;
            case rc_rm:
              fpa_lo = (sign_lo == 0 ? 0 : (round_lo | sticky_lo));
              break;
            case rc_rp:
              fpa_lo = (sign_lo == 1 ? 0 : (round_lo | sticky_lo));
              break;
            case rc_rz:
              fpa_lo = 0;
              break;
            default:
#ifndef unix
              FP_EMULATION_ERROR1 ("fp_emulate () Internal \
Error: invalid rc = %d for SIMD F1 instruction\n", rc)
#else
              FP_EMULATION_ERROR1 ("fp_emulate () Internal \
Error: invalid rc = %d for SIMD F1 instruction\n", rc)
              return (FP_EMUL_ERROR);
#endif
          }


          // add fpa_lo to the significand if fpa_lo = 1, and fix for the 
          // case when there is a carry-out in this addition

          if (fpa_lo == 1) {
            significand_lo = significand_lo + 1;

            if (significand_lo == 0x01000000) {
              significand_lo = 0x0800000;
              true_bexp_lo++; // e_min + 0x7f + 0x001
            }
          }

          if (significand_lo == 0) {
            true_bexp_lo = 0; // otherwise it is e_min or e_min + 1
          }

          exponent_lo = true_bexp_lo;

          if ((exponent_lo == 0x01) && ((significand_lo & 0x0800000) == 0)) {
            exponent_lo = 0x0; // result low is denormal
#ifdef DEBUG_UNIX
            printf ("DEBUG: result low is denormal\n");
#endif
          }

          // determine the new value of I_exc_lo
          I_exc_lo = round_lo | sticky_lo;

          // the low half could only raise an inexact exception; if it does
          // not, clear U_exc_lo, I_exc_lo, and fpa_lo in ISRlow

          if (!I_exc_lo) { 

            // if exact, then no new exception has to be raised for the low
            // half of the instruction; clear U_exc_lo in ISRlow (an exception
            // might be raised by the high half of the instruction) [clearing
            // I is redundant: it could not have been inexact and then become 
            // exact after denormalization]

            ISRlow = ISRlow & 0xfeff;
                // need to clean ISRlow because the other half might raise exc.

            // FPSR in the trap frame needs no update for the low half: with 
            // U traps disabled, the result would have to be tiny and inexact
            // in order to set the U flag; both U and I flags in the FPSR
            // stay unchanged
            U_exc_lo = 0;

            // no exception raised for the low half - clear fpa_lo (an 
            // exception might be raised by the high half of the instruction)
            ISRlow = ISRlow & 0xfbff; 

          } else { // if (I_exc_lo)

            // if tiny and inexact will set U and I in FPSR, as U_exc_lo = 1
            if (I_dis) { // the low half will not raise an inexact exception
              // clear I_exc_lo and U_exc_lo in ISRlow (prepare ISRlow
              // because the high half might raise an exception); no exception
              // raised for the low half - clear also fpa_lo
              ISRlow = ISRlow & 0xf8ff;
            } else { // update ISR code - will raise an inexact trap
              // clear U_lo and set I_lo in ISRlow - will raise inexact trap
              ISRlow = (ISRlow & 0xfeff) | 0x0200;
              // update the fpa_lo bit in the ISR code (NOT DONE IN EM_FP82)
              if (fpa_lo)
                ISRlow = ISRlow | 0x0400;
              else
                ISRlow = ISRlow & 0xfbff; 

            }

          }

        }

        if (O_dis && O_exc_lo) { // the result is not zero, and is normal with
            // unbounded exponent

          // true_bexp_lo not used in this case, and neither is decr_exp_lo
          // true_bexp_lo = 
          //     (exponent_lo == 0xff ? exponent_lo : exponent_lo + 0x100);

          // determine the result, according to the rounding mode
          switch (rc) {
            case rc_rn: // +infinity or -infinity
              exponent_lo = 0x0ff;
              significand_lo = 0x0800000;
              break;
            case rc_rm: // +max_fp or -infinity
              exponent_lo = (sign_lo == 0 ? 0x0fe : 0x0ff);
              significand_lo = (sign_lo == 0 ? 0x0ffffff : 0x0800000);
              break;
            case rc_rp: // +infinity or -max_fp
              exponent_lo = (sign_lo == 0 ? 0x0ff : 0x0fe);
              significand_lo = (sign_lo == 0 ? 0x0800000 : 0x0ffffff);
              break;
            case rc_rz: // +max_fp or -max_fp
              exponent_lo = 0x0fe;
              significand_lo = 0x0ffffff;
              break;
            default:
#ifndef unix
              FP_EMULATION_ERROR1 ("fp_emulate () Internal \
Error: invalid rc = %d for SIMD F1 instruction\n", rc)
#else
              FP_EMULATION_ERROR1 ("fp_emulate () Internal \
Error: invalid rc = %d for SIMD F1 instruction\n", rc)
              return (FP_EMUL_ERROR);
#endif
          }

          if (exponent_lo == 0xff)
            fpa_lo = 1;
          else
            fpa_lo = 0;

          // will update FPSR
          O_exc_lo = 1; // must have been so already
          I_exc_lo = 1;

          if (I_dis) {

            // clear fpa_lo, I_exc_lo and O_exc_lo in ISRlow - an enabled
            // exception might be raised for the high half
            ISRlow = ISRlow & 0xf97f;

          } else { // if (!I_dis)

            // update ISR code - will raise an inexact exception for low half;
            // clear O_exc_lo and set I_exc_lo in ISRlow - the result
            // is always inexact
            ISRlow = (ISRlow & 0xff7f) | 0x0200;
            // update the fpa_lo bit
            if (fpa_lo)
              ISRlow = ISRlow | 0x0400;
            else
              ISRlow = ISRlow & 0xfbff;
            // Note that the exact result cannot be retrieved by the inexact
            // exception trap handler [but this will not occur on Merced]

          }

        }

        if (I_dis && I_exc_lo) {

          ; // nothing to do (keep this as a place holder)
          // redundant - this case was caught before

        }

        if (U_dis && U_exc_hi) { // the result is not zero, and is normal with
            // unbounded exponent (but tiny)

          decr_exp_hi = 0;
          // if SWA trap in the high half
          if (fpa_hi == 1) {
            significand_hi = significand_hi - 1;
            if (significand_hi == 0x07fffff) {
              significand_hi = 0x0ffffff;
              decr_exp_hi = 1;
            }
          }

          true_bexp_hi = (exponent_hi == 0 ? exponent_hi : exponent_hi - 0x100);

          // true_bexp_hi - 0x07f is the true unbiased exponent after the
          // first IEEE rounding; determine whether the result is tiny
          if (true_bexp_hi - 0x07f > emin - 1) {
#ifndef unix
            FP_EMULATION_ERROR0 ("fp_emulate () Internal Error:non-tiny resH\n");
#else
            FP_EMULATION_ERROR0 ("fp_emulate () Internal Error:non-tiny resH\n");
            return (FP_EMUL_ERROR);
#endif
          }

          // adjust now true_bexp_hi if necessary
          if (decr_exp_hi) true_bexp_hi--;

          // perform the second IEEE rounding

          shift_cnt_hi = emin - true_bexp_hi + 0x07f; // >= 1

          if (shift_cnt_hi <= significand_size) { // <= 24
            // do the actual shift to denormalize the result; the result
            // will be a denormal, or zero
            round_hi = I_exc_hi;
                // this is indicated even for O or U, if the result inexact
            sticky_hi = 0;
            for (ind = 0 ; ind < shift_cnt_hi ; ind++) {
              sticky_hi = round_hi | sticky_hi;
              round_hi = significand_hi & 0x01;
              significand_hi = significand_hi >> 1;
            }
            true_bexp_hi = true_bexp_hi + shift_cnt_hi; // e_min + 0x7f
          } else { // all the significand bits shift out into sticky
            significand_hi = 0;
            round_hi = 0;
            sticky_hi = 1;
            true_bexp_hi = true_bexp_hi + shift_cnt_hi; // e_min + 0x7f
          }

          // perform the rounding; the result is 0, denormal, or 
          // 1.0 x 2^emin
          switch (rc) {
            case rc_rn:
              lsb_hi = significand_hi & 0x01;
              fpa_hi = round_hi & (lsb_hi | sticky_hi);
              break;
            case rc_rm:
              fpa_hi = (sign_hi == 0 ? 0 : (round_hi | sticky_hi));
              break;
            case rc_rp:
              fpa_hi = (sign_hi == 1 ? 0 : (round_hi | sticky_hi));
              break;
            case rc_rz:
              fpa_hi = 0;
              break;
            default:
#ifndef unix
              FP_EMULATION_ERROR1 ("fp_emulate () Internal \
Error: invalid rc = %d for SIMD F1 instruction\n", rc)
#else
              FP_EMULATION_ERROR1 ("fp_emulate () Internal \
Error: invalid rc = %d for SIMD F1 instruction\n", rc)
              return (FP_EMUL_ERROR);
#endif
          }


          // add fpa_hi to the significand if fpa = 1, and fix for the 
          // case when there is a carry-out in this addition

          if (fpa_hi == 1) {
            significand_hi = significand_hi + 1;

            if (significand_hi == 0x01000000) {
              significand_hi = 0x0800000;
              true_bexp_hi++; // e_min + 0x7f + 0x01
            }
          }

          if (significand_hi == 0) {
            true_bexp_hi = 0; // otherwise it is e_min or e_min + 1
          }

          exponent_hi = true_bexp_hi;

          if ((exponent_hi == 0x01) && ((significand_hi & 0x0800000) == 0)) {
            exponent_hi = 0x0; // result high is denormal
#ifdef DEBUG_UNIX
            printf ("DEBUG: result high is denormal\n");
#endif
          }

          // determine the new value of I_exc_hi
          I_exc_hi = round_hi | sticky_hi;

          // the high half could only raise an inexact exception; if it does
          // not, clear U_exc_hi, I_exc_hi, and fpa_hi in ISRlow

          if (!I_exc_hi) { 

            // if exact, then no new exception has to be raised for the high
            // half of the instruction; clear U_exc_hi in ISRlow (an exception
            // might be raised by the high half of the instruction)  [clearing
            // I is redundant: it could not have been inexact and then become
            // exact after denormalization]

            ISRlow = ISRlow & 0xefff;
              // need to clean ISRlow because the other half might raise exc.

            // FPSR in the trap frame needs no update for the high half: with 
            // U traps disabled, the result would have to be tiny and inexact
            // in order to set the U flag; both U and I flags in the FPSR
            // stay unchanged
            U_exc_hi = 0;

            // no exception raised for the high half - clear fpa_hi (an
            // exception might be raised by the low half of the instruction)
            ISRlow = ISRlow & 0xbfff;

          } else { // if (I_exc_hi)

            // if tiny and inexact will set U and I in FPSR, as U_exc_hi = 1
            if (I_dis) { // the high half will not raise an inexact exception
              // clear I_exc_hi and U_exc_hi in ISRlow (prepare ISRlow
              // because the low half might raise an exception); no exception
              // raised for the high half - clear also fpa_hi
              ISRlow = ISRlow & 0x8fff;
            } else { // update ISR code - will raise an inexact trap
              // clear U_hi and set I_hi in ISRlow - will raise inexact trap
              ISRlow = (ISRlow & 0xefff) | 0x2000;
            // update the fpa_hi bit in the ISR code (NOT DONE IN EM_FP82)
            if (fpa_hi)
              ISRlow = ISRlow | 0x4000;
            else
              ISRlow = ISRlow & 0xbfff;
            }

          }

        }

        if (O_dis && O_exc_hi) { // the result is not zero, and is normal with
            // unbounded exponent

          // true_bexp_hi not used in this case, and neither is decr_exp_hi
          // true_bexp_hi = 
          //     (exponent_hi == 0xff ? exponent_hi : exponent_hi + 0x100);

          // determine the result, according to the rounding mode
          switch (rc) {
            case rc_rn: // +infinity or -infinity
              exponent_hi = 0x0ff;
              significand_hi = 0x0800000;
              break;
            case rc_rm: // +max_fp or -infinity
              exponent_hi = (sign_hi == 0 ? 0x0fe : 0x0ff);
              significand_hi = (sign_hi == 0 ? 0x0ffffff : 0x0800000);
              break;
            case rc_rp: // +infinity or -max_fp
              exponent_hi = (sign_hi == 0 ? 0x0ff : 0x0fe);
              significand_hi = (sign_hi == 0 ? 0x0800000 : 0x0ffffff);
              break;
            case rc_rz: // +max_fp or -max_fp
              exponent_hi = 0x0fe;
              significand_hi = 0x0ffffff;
              break;
            default:
#ifndef unix
              FP_EMULATION_ERROR1 ("fp_emulate () Internal \
Error: invalid rc = %d for SIMD F1 instruction\n", rc)
#else
              FP_EMULATION_ERROR1 ("fp_emulate () Internal \
Error: invalid rc = %d for SIMD F1 instruction\n", rc)
              return (FP_EMUL_ERROR);
#endif
          }

          if (exponent_hi == 0xff)
            fpa_hi = 1;
          else
            fpa_hi = 0;

          // will update FPSR
          O_exc_hi = 1; // must have been so already
          I_exc_hi = 1;

          if (I_dis) { 

            // clear fpa_hi, I_exc_hi and O_exc_hi in ISRlow - an enabled U
            // exception might be raised for the low half
            ISRlow = ISRlow & 0x97ff;

          } else { // if (!I_dis)

            // update ISR code - will raise an inexact exception for high half;
            // clear O_exc_hi and set I_exc_hi in ISRlow - the result
            // is always inexact
            ISRlow = (ISRlow & 0xf7ff) | 0x2000;
            // update the fpa_hi bit
            if (fpa_hi)
              ISRlow = ISRlow | 0x4000;
            else
              ISRlow = ISRlow & 0xbfff;
            // Note that the exact result cannot be retrieved by the inexact
            // exception trap handler [but this will not occur on Merced]

          }

        }

        if (I_dis && I_exc_hi) {
       
          ; // nothing to do (keep this as a place holder)
          // redundant - this case was caught before

        }

        // return the result
        low_half = (sign_lo << 31) | (exponent_lo << 23) | 
            significand_lo & 0x07fffff;

        high_half = (sign_hi << 31) | (exponent_hi << 23) |
            significand_hi & 0x07fffff;

        ps->state_FR[lf1].significand = high_half << 32 | low_half; 

        // Note: the exponent is 0x1003e even for a significand of 0 (ACR 131)

        // successful emulation

        // set the destination floating-point register value (this is a trap)
#ifdef DEBUG_UNIX
printf ("DEBUG AFTER F1 SWA TRAP 2: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
#endif
        set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
        if (f1 < 32)
          *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
        else
          *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

        // update the FPSR (but will not be able to distinguish lo from hi)
        if (I_exc_lo || I_exc_hi) { // set I in FPSR
          // I traps must be disabled at this point
          *pfpsr = *pfpsr |
              ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 12));
        } // else leave unchanged the sticky bit I
        if (U_exc_lo || U_exc_hi) { // set U in FPSR
          // U traps must be disabled at this point
          *pfpsr = *pfpsr |
              ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 11));
        } // else leave unchanged the sticky bit U
        if (O_exc_lo || O_exc_hi) { // set O in FPSR
          // O traps must be disabled at this point
          *pfpsr = *pfpsr |
              ((EM_uint64_t)1 << (6 + (EM_uint_t)sf * 13 + 10));
        } // else leave unchanged the sticky bit O

        // return TRUE if no exception has to be raised
        if ((I_dis || I_exc_lo == 0 && I_exc_hi == 0) &&
            (U_dis || U_exc_lo == 0 && U_exc_hi == 0) &&
            (O_dis || O_exc_lo == 0 && O_exc_hi == 0)) {

          return (TRUE);

        } else {

          // if ((low inexact || high inexact) && I traps enabled or
          //     (low underflow || high underflow) && U traps enabled or
          //     (low overflow || high overflow) && O traps enabled)

          // ISRlow might have been updated
          *pisr = ((*pisr) & 0xffffffffffff0000) | ISRlow;

          return (FALSE | SIMD_INSTRUCTION); // will raise O, U, or I trap

        }

      }

    } else {

      // unrecognized instruction type
#ifndef unix
      FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not valid for SWA trap\n", OpCode)
#else
      FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not valid for SWA trap\n", OpCode)
      return (FP_EMUL_ERROR);
#endif

    }

    // end 'else if (trap_type == FPTRAP && swa_trap (sf, FPSR, ISRlow))'

  } else if (trap_type == FPFLT || trap_type == FPTRAP) {

#ifdef DEBUG_UNIX
    printf ("DEBUG: INSTRUCTION NOT EMULATED\n");
#endif

    // if we got here, the trapping instruction was not emulated, because it
    // did not raise a SWA fault or trap. This includes the cases when a 
    // V or Z fault or an U, O, or I trap occurred, and the exceptions 
    // raised were enabled

    // determine whether this is a non-SIMD, or a SIMD instruction

    if ((OpCode & F1_MIN_MASK) == F1_PATTERN) {
      // F1 instruction

      switch (OpCode & F1_MASK) {
        case FMA_PATTERN:
        case FMA_S_PATTERN:
        case FMA_D_PATTERN:
        case FMS_PATTERN:
        case FMS_S_PATTERN:
        case FMS_D_PATTERN:
        case FNMA_PATTERN:
        case FNMA_S_PATTERN:
        case FNMA_D_PATTERN:
          SIMD_instruction = 0;
          break;
        case FPMA_PATTERN:
        case FPMS_PATTERN:
        case FPNMA_PATTERN:
          SIMD_instruction = 1;
          break;
        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode)
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode)
          return (FP_EMUL_ERROR);
#endif
      }

    } else if ((OpCode & F4_MIN_MASK) == F4_PATTERN) {
      // F4 instruction

      switch (OpCode & F4_MASK) {
        case FCMP_EQ_PATTERN:
        case FCMP_LT_PATTERN:
        case FCMP_LE_PATTERN:
        case FCMP_UNORD_PATTERN:
        case FCMP_EQ_UNC_PATTERN:
        case FCMP_LT_UNC_PATTERN:
        case FCMP_LE_UNC_PATTERN:
        case FCMP_UNORD_UNC_PATTERN:
          SIMD_instruction = 0;
          break;
        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode)
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode)
          return (FP_EMUL_ERROR);
#endif
      }

    } else if ((OpCode & F6_MIN_MASK) == F6_PATTERN) {
      // F6 instruction

      switch (OpCode & F6_MASK) {
        case FRCPA_PATTERN:
          SIMD_instruction = 0;
          break;
        case FPRCPA_PATTERN:
          SIMD_instruction = 1;
          break;
        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode)
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode)
          return (FP_EMUL_ERROR);
#endif
      }

    } else if ((OpCode & F7_MIN_MASK) == F7_PATTERN) {
      // F7 instruction

      switch (OpCode & F7_MASK) {
        case FRSQRTA_PATTERN:
          SIMD_instruction = 0;
          break;
        case FPRSQRTA_PATTERN:
          SIMD_instruction = 1;
          break;
        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode)
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode)
          return (FP_EMUL_ERROR);
#endif
      }

    } else if ((OpCode & F8_MIN_MASK) == F8_PATTERN) {
      // F8 instruction

      switch (OpCode & F8_MASK) {
        case FMIN_PATTERN:
        case FMAX_PATTERN:
        case FAMIN_PATTERN:
        case FAMAX_PATTERN:
          SIMD_instruction = 0;
          break;
        case FPMIN_PATTERN:
        case FPMAX_PATTERN:
        case FPAMIN_PATTERN:
        case FPAMAX_PATTERN:
        case FPCMP_EQ_PATTERN:
        case FPCMP_LT_PATTERN:
        case FPCMP_LE_PATTERN:
        case FPCMP_UNORD_PATTERN:
        case FPCMP_NEQ_PATTERN:
        case FPCMP_NLT_PATTERN:
        case FPCMP_NLE_PATTERN:
        case FPCMP_ORD_PATTERN:
          SIMD_instruction = 1;
          break;
        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode)
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode)
          return (FP_EMUL_ERROR);
#endif
      }

    } else if ((OpCode & F10_MIN_MASK) == F10_PATTERN) {
      // F10 instruction

      switch (OpCode & F10_MASK) {
        case FCVT_FX_PATTERN:
        case FCVT_FXU_PATTERN:
        case FCVT_FX_TRUNC_PATTERN:
        case FCVT_FXU_TRUNC_PATTERN:
          SIMD_instruction = 0;
          break;
        case FPCVT_FX_PATTERN:
        case FPCVT_FXU_PATTERN:
        case FPCVT_FX_TRUNC_PATTERN:
        case FPCVT_FXU_TRUNC_PATTERN:
          SIMD_instruction = 1;
          break;
        default:
          // unrecognized instruction type
#ifndef unix
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode)
#else
          FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode)
          return (FP_EMUL_ERROR);
#endif
      }

    } else {

      // unrecognized instruction type
#ifndef unix
      FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %8x %8x not recognized\n", OpCode)
#else
      FP_EMULATION_ERROR1 ("fp_emulate () Internal Error: \
instruction opcode %Lx not recognized\n", OpCode)
      return (FP_EMUL_ERROR);
#endif

    }

#ifdef DEBUG_UNIX
if ((OpCode & F1_MIN_MASK) == F1_PATTERN) {
printf ("DEBUG AFTER 'NO EMULATION' RET FALSE: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
}
#endif

    // no emulation; ISRlow and AR[0].uint_value (FPSR) have not changed
    if (SIMD_instruction)
      return (FALSE | SIMD_INSTRUCTION); // will raise fp exception
    else
      return (FALSE); // will raise fp exception

  } else {

#ifdef DEBUG_UNIX
    printf ("DEBUG: STATUS FLOAT ERROR\n");
#endif

    // fp_emulate () called w/o 
    // trap_frame->type == 0x020 || trap_fram->type == 0x021 
#ifndef unix
    FP_EMULATION_ERROR2 ("fp_emulate () Internal Error: \
fp_emulate () called w/o trap_type FPFLT or FPTRAP \
 OpCode = %8x %8x, and ISR code = %x\n", OpCode, ISRlow)
#else
    FP_EMULATION_ERROR2 ("fp_emulate () Internal Error: \
fp_emulate () called w/o trap_type FPFLT or FPTRAP \
 OpCode = %Lx, and ISR code = %x\n", OpCode, ISRlow)
    return (FP_EMUL_ERROR);
#endif

  }

new_exception:

  // can get here only if 
  // trap_type == FPFLT, and
  // the emulation generated a new FP exception (e.g. if an fma with
  // denormal input[s] causes an underflow, and the underflow traps are
  // enabled; also [but not only] if an fma has a denormal input, and denormal
  // exceptions are enabled)

  new_trap_type = ps->trap_type;

  if (new_trap_type == FPFLT) { // fault

    // this is a fault - it must be a denormal exception or an invalid
    // exception (the latter only for fcvt.fxu[.trunc] with negative
    // unnormal input)
    fault_ISR_code = (ps->state_MERCED_RTL >> 16) & 0x0ffff;
    if ((fault_ISR_code & 0x077) == 0) { // SWA fault only
#ifndef unix
      FP_EMULATION_ERROR0 (
          "fp_emulate () Internal Error: SWA fault repeated\n");
#else
      FP_EMULATION_ERROR0 (
          "fp_emulate () Internal Error: SWA fault repeated\n");
      return (FP_EMUL_ERROR);
#endif
    }

#ifdef DEBUG_UNIX
    printf ("DEBUG fp_emulate () NEW_EXC fault_ISR_code = %x\n", fault_ISR_code);
#endif

#ifdef DEBUG_UNIX
if ((OpCode & F1_MIN_MASK) == F1_PATTERN)
  printf ("DEBUG NEW_EXC FAULT PART RET FALSE: ps->state_FR[lf1] = %x %x %Lx\n",
      ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
      ps->state_FR[lf1].significand);
#endif

    // update the ISR code, to pass the new value of ISR.code to the 
    // user
    *pisr = ((*pisr) & 0xffffffffffff0000) | fault_ISR_code;

    // the FPSR is unchanged
    if (SIMD_instruction)
      return (FALSE | SIMD_INSTRUCTION); // will raise fp exception
    else
      return (FALSE); // will raise fp exception

  } else if (new_trap_type == FPTRAP) { // trap

    // this is a trap
    trap_ISR_code = (ps->state_MERCED_RTL >> 16) & 0x0ffff;

#ifdef DEBUG_UNIX
    printf ("DEBUG fp_emulate () NEW_EXC trap_ISR_code = %4.4x\n", trap_ISR_code);
#endif

    // set the destination floating-point reg value, as this is a trap
#ifdef DEBUG_UNIX
printf ("DEBUG NEW_EXC TRAP PART RET FALSE: ps->state_FR[lf1] = %x %x %Lx\n",
  ps->state_FR[lf1].sign, ps->state_FR[lf1].exponent, 
  ps->state_FR[lf1].significand);
printf ("DEBUG NEW_EXC TRAP PART RET FALSE: f1 f2 f3 f4 = %x %x %x %x\n",
  f1, f2, f3, f4);
#endif
    set_fp_register (f1, FPRegToFP128(ps->state_FR[lf1]), fp_state);
    if (f1 < 32)
      *pipsr = *pipsr | (EM_uint64_t)0x10; // set mfl bit
    else
      *pipsr = *pipsr | (EM_uint64_t)0x20; // set mfh bit

    // replace trap_ISR_code in the updated ISR code (nothing else changes)
    *pisr = ((*pisr) & 0xffffffffffff0000) | trap_ISR_code;

    // copy ps->state_AR[0] back into the trap frame FPSR
    *pfpsr =  ps->state_AR[0].uint_value;

    // caller will advance instruction pointer iip
    if (SIMD_instruction)
      return (FALSE | FAULT_TO_TRAP | SIMD_INSTRUCTION); // will raise fp exc
    else
      return (FALSE | FAULT_TO_TRAP); // will raise fp exception

  } else {
#ifndef unix
    FP_EMULATION_ERROR1 (
        "fp_emulate () Internal Error: new_trap_type = %x\n", new_trap_type)
#else
    FP_EMULATION_ERROR1 (
        "fp_emulate () Internal Error: new_trap_type = %x\n", new_trap_type)
    return (FP_EMUL_ERROR);
#endif
  }

}



int
swa_trap (EM_opcode_sf_type sf, EM_uint64_t FPSR, EM_uint_t ISRlow)

{

  int IsSWA;
  int U_en, U_exc;
  int O_en, O_exc;

  // this assumes that for a non-SIMD instruction, the "low" bits in 
  // ISRlow are 0

  // SWA trap for U if 
  //     bit 0 in ISR.code is set             [fp trap] [redundant]
  //         and 
  //     (bit 8 or bit 12 in ISR.code is set) [U normal, or SIMD low or high]
  //         and
  //     (sfx not 0 and bit 6 in FPSR.sfx set [sfx != 0 and traps disabled in
  //      or bit 4 in FPSR set)                  sfx or U traps disabled]
  //

  IsSWA = (ISRlow & 0x01) && (ISRlow & 0x100 | ISRlow & 0x1000) && 
      ((EM_uint_t)sf != 0 && ((FPSR >> (6 + 6 + 13 * (EM_uint_t)sf)) & 0x01) || 
      ((FPSR >> 4) & 0x01));

  // SWA trap for O if 
  //     bit 0 in ISR.code is set             [fp trap] [redundant]
  //         and 
  //     (bit 7 or bit 11 in ISR.code is set) [O normal, or SIMD low or high]
  //         and
  //     (sfx not 0 and bit 6 in FPSR.sfx set [traps disabled in sfx]
  //      or bit 3 in FPSR set)                  [O traps disabled]
  //

  IsSWA = IsSWA || 
      ((ISRlow & 0x01) && (ISRlow & 0x080 | ISRlow & 0x0800) && 
      ((EM_uint_t)sf != 0 && ((FPSR >> (6 + 6 + 13 * (EM_uint_t)sf)) & 0x01) || 
      ((FPSR >> 3) & 0x01)));

#ifdef DEBUG_UNIX
  if (IsSWA && ((ISRlow & 0x1980) == 0))
    printf ("DEBUG swa_trap () WARNING: IsSWA and ((ISRlow & 0x1980) == 0)\n");
#endif

  // U_en = ((EM_uint_t)sf == 0 || td == 0) && FPSR_4 == 0
  U_en = ((EM_uint_t)sf == 0 || 
      !((FPSR >> (6 + 6 + 13 * (EM_uint_t)sf)) & 0x01)) &&
      !((FPSR >> 4) & 0x01);
  // U exc if
  //     bit 0 in ISR.code is set             [fp trap] [redundant]
  //         and
  //     (bit 8 or bit 12 in ISR.code is set) [U normal, or SIMD low or high]
  //         and
  //     U traps are enabled
  U_exc = (ISRlow & 0x01) && (ISRlow & 0x100 | ISRlow & 0x1000) && U_en;

  // O_en = ((EM_uint_t)sf == 0 || td == 0) && FPSR_3 == 0
  O_en = ((EM_uint_t)sf == 0 || 
      !((FPSR >> (6 + 6 + 13 * (EM_uint_t)sf)) & 0x01)) && 
      !((FPSR >> 3) & 0x01);
  // O exc if
  //     bit 0 in ISR.code is set             [fp trap] [redundant]
  //         and
  //     (bit 7 or bit 11 in ISR.code is set) [O normal, or SIMD low or high]
  //         and
  //     O traps are enabled
  O_exc = (ISRlow & 0x01) && (ISRlow & 0x080 | ISRlow & 0x0800) && O_en;

  // if no U exc and no O exc and no SWA trap due to U or O, then check I
  if (!U_exc && !O_exc && !IsSWA) {

    // SWA trap for I if
    //     bit 0 in ISR.code is set             [fp trap] [redundant]
    //         and
    //     (bit 9 or bit 13 in ISR.code is set) [I normal, or SIMD low or high]
    //         and
    //     (sfx not 0 and bit 6 in FPSR.sfx set [sfx != 0 and traps disabled in
    //      or bit 5 in FPSR set)                  sfx or U traps disabled]
    //

    IsSWA = (ISRlow & 0x01) && (ISRlow & 0x200 | ISRlow & 0x2000) &&
        ((EM_uint_t)sf != 0 && ((FPSR >> (6 + 6 + 13 * (EM_uint_t)sf)) & 0x01)
        || ((FPSR >> 5) & 0x01));

  }

  return (IsSWA);

}


EM_fp_reg_type
FP128ToFPReg (FLOAT128_TYPE f128)

{

  EM_fp_reg_type freg;
  char *p;
  EM_uint64_t *ui64;


  p = (char *)(&f128);
  ui64 = (EM_uint64_t *)(&f128);

  freg.sign = (p[10] >> 1) & 0x01;
  freg.exponent = ((unsigned int)(p[10] & 0x01) << 16) |
      ((unsigned int)(p[9] & 0xff) << 8) | ((unsigned int)(p[8] & 0xff));
  freg.significand = *ui64;

  return (freg);

}



FLOAT128_TYPE
FPRegToFP128 (EM_fp_reg_type freg)

{

  FLOAT128_TYPE f128;


  f128.loFlt64 = freg.significand;

  f128.hiFlt64 = ((long)(freg.sign) << 17) | ((long)freg.exponent);

  return (f128);

}
