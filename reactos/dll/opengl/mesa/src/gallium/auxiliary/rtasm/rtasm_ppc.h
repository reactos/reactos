/**************************************************************************
 *
 * Copyright (C) 2008 Tungsten Graphics, Inc.   All Rights Reserved.
 * Copyright (C) 2009 VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * PPC code generation.
 * \author Brian Paul
 */


#ifndef RTASM_PPC_H
#define RTASM_PPC_H


#include "pipe/p_compiler.h"


#define PPC_INST_SIZE 4  /**< 4 bytes / instruction */

#define PPC_NUM_REGS 32
#define PPC_NUM_FP_REGS 32
#define PPC_NUM_VEC_REGS 32

/** Stack pointer register */
#define PPC_REG_SP 1

/** Branch conditions */
#define BRANCH_COND_ALWAYS       0x14  /* binary 1z1zz (z=ignored) */

/** Branch hints */
#define BRANCH_HINT_SUB_RETURN   0x0   /* binary 00 */


struct ppc_function
{
   uint32_t *store;  /**< instruction buffer */
   uint num_inst;
   uint max_inst;
   uint32_t reg_used;   /** used/free general-purpose registers bitmask */
   uint32_t fp_used;   /** used/free floating point registers bitmask */
   uint32_t vec_used;   /** used/free vector registers bitmask */
   int indent;
   boolean print;
};



extern void ppc_init_func(struct ppc_function *p);
extern void ppc_release_func(struct ppc_function *p);
extern uint ppc_num_instructions(const struct ppc_function *p);
extern void (*ppc_get_func( struct ppc_function *p ))( void );
extern void ppc_dump_func(const struct ppc_function *p);

extern void ppc_print_code(struct ppc_function *p, boolean enable);
extern void ppc_indent(struct ppc_function *p, int spaces);
extern void ppc_comment(struct ppc_function *p, int rel_indent, const char *s);

extern int ppc_reserve_register(struct ppc_function *p, int reg);
extern int ppc_allocate_register(struct ppc_function *p);
extern void ppc_release_register(struct ppc_function *p, int reg);
extern int ppc_allocate_fp_register(struct ppc_function *p);
extern void ppc_release_fp_register(struct ppc_function *p, int reg);
extern int ppc_allocate_vec_register(struct ppc_function *p);
extern void ppc_release_vec_register(struct ppc_function *p, int reg);



/**
 ** float vector arithmetic
 **/

/** vector float add */
extern void
ppc_vaddfp(struct ppc_function *p,uint vD, uint vA, uint vB);

/** vector float substract */
extern void
ppc_vsubfp(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float min */
extern void
ppc_vminfp(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float max */
extern void
ppc_vmaxfp(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float mult add: vD = vA * vB + vC */
extern void
ppc_vmaddfp(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC);

/** vector float negative mult subtract: vD = vA - vB * vC */
extern void
ppc_vnmsubfp(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC);

/** vector float compare greater than */
extern void
ppc_vcmpgtfpx(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float compare greater than or equal to */
extern void
ppc_vcmpgefpx(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float compare equal */
extern void
ppc_vcmpeqfpx(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float 2^x */
extern void
ppc_vexptefp(struct ppc_function *p, uint vD, uint vB);

/** vector float log2(x) */
extern void
ppc_vlogefp(struct ppc_function *p, uint vD, uint vB);

/** vector float reciprocol */
extern void
ppc_vrefp(struct ppc_function *p, uint vD, uint vB);

/** vector float reciprocol sqrt estimate */
extern void
ppc_vrsqrtefp(struct ppc_function *p, uint vD, uint vB);

/** vector float round to negative infinity */
extern void
ppc_vrfim(struct ppc_function *p, uint vD, uint vB);

/** vector float round to positive infinity */
extern void
ppc_vrfip(struct ppc_function *p, uint vD, uint vB);

/** vector float round to nearest int */
extern void
ppc_vrfin(struct ppc_function *p, uint vD, uint vB);

/** vector float round to int toward zero */
extern void
ppc_vrfiz(struct ppc_function *p, uint vD, uint vB);


/** vector store: store vR at mem[vA+vB] */
extern void
ppc_stvx(struct ppc_function *p, uint vR, uint vA, uint vB);

/** vector load: vR = mem[vA+vB] */
extern void
ppc_lvx(struct ppc_function *p, uint vR, uint vA, uint vB);

/** load vector element word: vR = mem_word[vA+vB] */
extern void
ppc_lvewx(struct ppc_function *p, uint vR, uint vA, uint vB);



/**
 ** vector bitwise operations
 **/


/** vector and */
extern void
ppc_vand(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector and complement */
extern void
ppc_vandc(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector or */
extern void
ppc_vor(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector nor */
extern void
ppc_vnor(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector xor */
extern void
ppc_vxor(struct ppc_function *p, uint vD, uint vA, uint vB);

/** Pseudo-instruction: vector move */
extern void
ppc_vmove(struct ppc_function *p, uint vD, uint vA);

/** Set vector register to {0,0,0,0} */
extern void
ppc_vzero(struct ppc_function *p, uint vr);



/**
 ** Vector shuffle / select / splat / etc
 **/

/** vector permute */
extern void
ppc_vperm(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC);

/** vector select */
extern void
ppc_vsel(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC);

/** vector splat byte */
extern void
ppc_vspltb(struct ppc_function *p, uint vD, uint vB, uint imm);

/** vector splat half word */
extern void
ppc_vsplthw(struct ppc_function *p, uint vD, uint vB, uint imm);

/** vector splat word */
extern void
ppc_vspltw(struct ppc_function *p, uint vD, uint vB, uint imm);

/** vector splat signed immediate word */
extern void
ppc_vspltisw(struct ppc_function *p, uint vD, int imm);

/** vector shift left word: vD[word] = vA[word] << (vB[word] & 0x1f) */
extern void
ppc_vslw(struct ppc_function *p, uint vD, uint vA, uint vB);



/**
 ** scalar arithmetic
 **/

extern void
ppc_add(struct ppc_function *p, uint rt, uint ra, uint rb);

extern void
ppc_addi(struct ppc_function *p, uint rt, uint ra, int imm);

extern void
ppc_addis(struct ppc_function *p, uint rt, uint ra, int imm);

extern void
ppc_and(struct ppc_function *p, uint rt, uint ra, uint rb);

extern void
ppc_andi(struct ppc_function *p, uint rt, uint ra, int imm);

extern void
ppc_or(struct ppc_function *p, uint rt, uint ra, uint rb);

extern void
ppc_ori(struct ppc_function *p, uint rt, uint ra, int imm);

extern void
ppc_xor(struct ppc_function *p, uint rt, uint ra, uint rb);

extern void
ppc_xori(struct ppc_function *p, uint rt, uint ra, int imm);

extern void
ppc_mr(struct ppc_function *p, uint rt, uint ra);

extern void
ppc_li(struct ppc_function *p, uint rt, int imm);

extern void
ppc_lis(struct ppc_function *p, uint rt, int imm);

extern void
ppc_load_int(struct ppc_function *p, uint rt, int imm);



/**
 ** scalar load/store
 **/

extern void
ppc_stwu(struct ppc_function *p, uint rs, uint ra, int d);

extern void
ppc_stw(struct ppc_function *p, uint rs, uint ra, int d);

extern void
ppc_lwz(struct ppc_function *p, uint rs, uint ra, int d);



/**
 ** Float (non-vector) arithmetic
 **/

extern void
ppc_fadd(struct ppc_function *p, uint frt, uint fra, uint frb);

extern void
ppc_fsub(struct ppc_function *p, uint frt, uint fra, uint frb);

extern void
ppc_fctiwz(struct ppc_function *p, uint rt, uint ra);

extern void
ppc_stfs(struct ppc_function *p, uint frs, uint ra, int offset);

extern void
ppc_stfiwx(struct ppc_function *p, uint frs, uint ra, uint rb);

extern void
ppc_lfs(struct ppc_function *p, uint frt, uint ra, int offset);



/**
 ** branch instructions
 **/

extern void
ppc_blr(struct ppc_function *p);

void
ppc_bclr(struct ppc_function *p, uint condOp, uint branchHint, uint condReg);

extern void
ppc_return(struct ppc_function *p);


#endif /* RTASM_PPC_H */
