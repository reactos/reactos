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
 * For reference, see http://www.power.org/resources/reading/PowerISA_V2.05.pdf
 * ABI info: http://www.cs.utsa.edu/~whaley/teach/cs6463FHPO/LEC/lec12_ho.pdf
 *
 * Other PPC refs:
 * http://www-01.ibm.com/chips/techlib/techlib.nsf/techdocs/852569B20050FF778525699600719DF2
 * http://www.ibm.com/developerworks/eserver/library/es-archguide-v2.html
 * http://www.freescale.com/files/product/doc/MPCFPE32B.pdf
 *
 * \author Brian Paul
 */


#include <stdio.h>
#include "util/u_memory.h"
#include "util/u_debug.h"
#include "rtasm_execmem.h"
#include "rtasm_ppc.h"


void
ppc_init_func(struct ppc_function *p)
{
   uint i;

   memset(p, 0, sizeof(*p));

   p->num_inst = 0;
   p->max_inst = 100; /* first guess at buffer size */
   p->store = rtasm_exec_malloc(p->max_inst * PPC_INST_SIZE);
   p->reg_used = 0x0;
   p->fp_used = 0x0;
   p->vec_used = 0x0;

   p->print = FALSE;
   p->indent = 0;

   /* only allow using gp registers 3..12 for now */
   for (i = 0; i < 3; i++)
      ppc_reserve_register(p, i);
   for (i = 12; i < PPC_NUM_REGS; i++)
      ppc_reserve_register(p, i);
}


void
ppc_release_func(struct ppc_function *p)
{
   assert(p->num_inst <= p->max_inst);
   if (p->store != NULL) {
      rtasm_exec_free(p->store);
   }
   p->store = NULL;
}


uint
ppc_num_instructions(const struct ppc_function *p)
{
   return p->num_inst;
}


void (*ppc_get_func(struct ppc_function *p))(void)
{
#if 0
   DUMP_END();
   if (DISASSEM && p->store)
      debug_printf("disassemble %p %p\n", p->store, p->csr);

   if (p->store == p->error_overflow)
      return (void (*)(void)) NULL;
   else
#endif
      return (void (*)(void)) pointer_to_func(p->store);
}


void
ppc_dump_func(const struct ppc_function *p)
{
   uint i;
   for (i = 0; i < p->num_inst; i++) {
      debug_printf("%3u: 0x%08x\n", i, p->store[i]);
   }
}


void
ppc_print_code(struct ppc_function *p, boolean enable)
{
   p->print = enable;
}


void
ppc_indent(struct ppc_function *p, int spaces)
{
   p->indent += spaces;
}


static void
indent(const struct ppc_function *p)
{
   int i;
   for (i = 0; i < p->indent; i++) {
      putchar(' ');
   }
}


void
ppc_comment(struct ppc_function *p, int rel_indent, const char *s)
{
   if (p->print) {
      p->indent += rel_indent;
      indent(p);
      p->indent -= rel_indent;
      printf("# %s\n", s);
   }
}


/**
 * Mark a register as being unavailable.
 */
int
ppc_reserve_register(struct ppc_function *p, int reg)
{
   assert(reg < PPC_NUM_REGS);
   p->reg_used |= (1 << reg);
   return reg;
}


/**
 * Allocate a general purpose register.
 * \return register index or -1 if none left.
 */
int
ppc_allocate_register(struct ppc_function *p)
{
   unsigned i;
   for (i = 0; i < PPC_NUM_REGS; i++) {
      const uint32_t mask = 1 << i;
      if ((p->reg_used & mask) == 0) {
         p->reg_used |= mask;
         return i;
      }
   }
   printf("OUT OF PPC registers!\n");
   return -1;
}


/**
 * Mark the given general purpose register as "unallocated".
 */
void
ppc_release_register(struct ppc_function *p, int reg)
{
   assert(reg < PPC_NUM_REGS);
   assert(p->reg_used & (1 << reg));
   p->reg_used &= ~(1 << reg);
}


/**
 * Allocate a floating point register.
 * \return register index or -1 if none left.
 */
int
ppc_allocate_fp_register(struct ppc_function *p)
{
   unsigned i;
   for (i = 0; i < PPC_NUM_FP_REGS; i++) {
      const uint32_t mask = 1 << i;
      if ((p->fp_used & mask) == 0) {
         p->fp_used |= mask;
         return i;
      }
   }
   printf("OUT OF PPC FP registers!\n");
   return -1;
}


/**
 * Mark the given floating point register as "unallocated".
 */
void
ppc_release_fp_register(struct ppc_function *p, int reg)
{
   assert(reg < PPC_NUM_FP_REGS);
   assert(p->fp_used & (1 << reg));
   p->fp_used &= ~(1 << reg);
}


/**
 * Allocate a vector register.
 * \return register index or -1 if none left.
 */
int
ppc_allocate_vec_register(struct ppc_function *p)
{
   unsigned i;
   for (i = 0; i < PPC_NUM_VEC_REGS; i++) {
      const uint32_t mask = 1 << i;
      if ((p->vec_used & mask) == 0) {
         p->vec_used |= mask;
         return i;
      }
   }
   printf("OUT OF PPC VEC registers!\n");
   return -1;
}


/**
 * Mark the given vector register as "unallocated".
 */
void
ppc_release_vec_register(struct ppc_function *p, int reg)
{
   assert(reg < PPC_NUM_VEC_REGS);
   assert(p->vec_used & (1 << reg));
   p->vec_used &= ~(1 << reg);
}


/**
 * Append instruction to instruction buffer.  Grow buffer if out of room.
 */
static void
emit_instruction(struct ppc_function *p, uint32_t inst_bits)
{
   if (!p->store)
      return;  /* out of memory, drop the instruction */

   if (p->num_inst == p->max_inst) {
      /* allocate larger buffer */
      uint32_t *newbuf;
      p->max_inst *= 2;  /* 2x larger */
      newbuf = rtasm_exec_malloc(p->max_inst * PPC_INST_SIZE);
      if (newbuf) {
         memcpy(newbuf, p->store, p->num_inst * PPC_INST_SIZE);
      }
      rtasm_exec_free(p->store);
      p->store = newbuf;
      if (!p->store) {
         /* out of memory */
         p->num_inst = 0;
         return;
      }
   }

   p->store[p->num_inst++] = inst_bits;
}


union vx_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned vD:5;
      unsigned vA:5;
      unsigned vB:5;
      unsigned op2:11;
   } inst;
};

static INLINE void
emit_vx(struct ppc_function *p, uint op2, uint vD, uint vA, uint vB,
        const char *format, boolean transpose)
{
   union vx_inst inst;
   inst.inst.op = 4;
   inst.inst.vD = vD;
   inst.inst.vA = vA;
   inst.inst.vB = vB;
   inst.inst.op2 = op2;
   emit_instruction(p, inst.bits);
   if (p->print) {
      indent(p);
      if (transpose)
         printf(format, vD, vB, vA);
      else
         printf(format, vD, vA, vB);
   }
}


union vxr_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned vD:5;
      unsigned vA:5;
      unsigned vB:5;
      unsigned rC:1;
      unsigned op2:10;
   } inst;
};

static INLINE void
emit_vxr(struct ppc_function *p, uint op2, uint vD, uint vA, uint vB,
         const char *format)
{
   union vxr_inst inst;
   inst.inst.op = 4;
   inst.inst.vD = vD;
   inst.inst.vA = vA;
   inst.inst.vB = vB;
   inst.inst.rC = 0;
   inst.inst.op2 = op2;
   emit_instruction(p, inst.bits);
   if (p->print) {
      indent(p);
      printf(format, vD, vA, vB);
   }
}


union va_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned vD:5;
      unsigned vA:5;
      unsigned vB:5;
      unsigned vC:5;
      unsigned op2:6;
   } inst;
};

static INLINE void
emit_va(struct ppc_function *p, uint op2, uint vD, uint vA, uint vB, uint vC,
        const char *format)
{
   union va_inst inst;
   inst.inst.op = 4;
   inst.inst.vD = vD;
   inst.inst.vA = vA;
   inst.inst.vB = vB;
   inst.inst.vC = vC;
   inst.inst.op2 = op2;
   emit_instruction(p, inst.bits);
   if (p->print) {
      indent(p);
      printf(format, vD, vA, vB, vC);
   }
}


union i_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned li:24;
      unsigned aa:1;
      unsigned lk:1;
   } inst;
};

static INLINE void
emit_i(struct ppc_function *p, uint op, uint li, uint aa, uint lk)
{
   union i_inst inst;
   inst.inst.op = op;
   inst.inst.li = li;
   inst.inst.aa = aa;
   inst.inst.lk = lk;
   emit_instruction(p, inst.bits);
}


union xl_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned bo:5;
      unsigned bi:5;
      unsigned unused:3;
      unsigned bh:2;
      unsigned op2:10;
      unsigned lk:1;
   } inst;
};

static INLINE void
emit_xl(struct ppc_function *p, uint op, uint bo, uint bi, uint bh,
        uint op2, uint lk)
{
   union xl_inst inst;
   inst.inst.op = op;
   inst.inst.bo = bo;
   inst.inst.bi = bi;
   inst.inst.unused = 0x0;
   inst.inst.bh = bh;
   inst.inst.op2 = op2;
   inst.inst.lk = lk;
   emit_instruction(p, inst.bits);
}

static INLINE void
dump_xl(const char *name, uint inst)
{
   union xl_inst i;

   i.bits = inst;
   debug_printf("%s = 0x%08x\n", name, inst);
   debug_printf(" op: %d 0x%x\n", i.inst.op, i.inst.op);
   debug_printf(" bo: %d 0x%x\n", i.inst.bo, i.inst.bo);
   debug_printf(" bi: %d 0x%x\n", i.inst.bi, i.inst.bi);
   debug_printf(" unused: %d 0x%x\n", i.inst.unused, i.inst.unused);
   debug_printf(" bh: %d 0x%x\n", i.inst.bh, i.inst.bh);
   debug_printf(" op2: %d 0x%x\n", i.inst.op2, i.inst.op2);
   debug_printf(" lk: %d 0x%x\n", i.inst.lk, i.inst.lk);
}


union x_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned vrs:5;
      unsigned ra:5;
      unsigned rb:5;
      unsigned op2:10;
      unsigned unused:1;
   } inst;
};

static INLINE void
emit_x(struct ppc_function *p, uint op, uint vrs, uint ra, uint rb, uint op2,
       const char *format)
{
   union x_inst inst;
   inst.inst.op = op;
   inst.inst.vrs = vrs;
   inst.inst.ra = ra;
   inst.inst.rb = rb;
   inst.inst.op2 = op2;
   inst.inst.unused = 0x0;
   emit_instruction(p, inst.bits);
   if (p->print) {
      indent(p);
      printf(format, vrs, ra, rb);
   }
}


union d_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned rt:5;
      unsigned ra:5;
      unsigned si:16;
   } inst;
};

static INLINE void
emit_d(struct ppc_function *p, uint op, uint rt, uint ra, int si,
       const char *format, boolean transpose)
{
   union d_inst inst;
   assert(si >= -32768);
   assert(si <= 32767);
   inst.inst.op = op;
   inst.inst.rt = rt;
   inst.inst.ra = ra;
   inst.inst.si = (unsigned) (si & 0xffff);
   emit_instruction(p, inst.bits);
   if (p->print) {
      indent(p);
      if (transpose)
         printf(format, rt, si, ra);
      else
         printf(format, rt, ra, si);
   }
}


union a_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned frt:5;
      unsigned fra:5;
      unsigned frb:5;
      unsigned unused:5;
      unsigned op2:5;
      unsigned rc:1;
   } inst;
};

static INLINE void
emit_a(struct ppc_function *p, uint op, uint frt, uint fra, uint frb, uint op2,
       uint rc, const char *format)
{
   union a_inst inst;
   inst.inst.op = op;
   inst.inst.frt = frt;
   inst.inst.fra = fra;
   inst.inst.frb = frb;
   inst.inst.unused = 0x0;
   inst.inst.op2 = op2;
   inst.inst.rc = rc;
   emit_instruction(p, inst.bits);
   if (p->print) {
      indent(p);
      printf(format, frt, fra, frb);
   }
}


union xo_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned rt:5;
      unsigned ra:5;
      unsigned rb:5;
      unsigned oe:1;
      unsigned op2:9;
      unsigned rc:1;
   } inst;
};

static INLINE void
emit_xo(struct ppc_function *p, uint op, uint rt, uint ra, uint rb, uint oe,
        uint op2, uint rc, const char *format)
{
   union xo_inst inst;
   inst.inst.op = op;
   inst.inst.rt = rt;
   inst.inst.ra = ra;
   inst.inst.rb = rb;
   inst.inst.oe = oe;
   inst.inst.op2 = op2;
   inst.inst.rc = rc;
   emit_instruction(p, inst.bits);
   if (p->print) {
      indent(p);
      printf(format, rt, ra, rb);
   }
}





/**
 ** float vector arithmetic
 **/

/** vector float add */
void
ppc_vaddfp(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 10, vD, vA, vB, "vaddfp\t%u, v%u, v%u\n", FALSE);
}

/** vector float substract */
void
ppc_vsubfp(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 74, vD, vA, vB, "vsubfp\tv%u, v%u, v%u\n", FALSE);
}

/** vector float min */
void
ppc_vminfp(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1098, vD, vA, vB, "vminfp\tv%u, v%u, v%u\n", FALSE);
}

/** vector float max */
void
ppc_vmaxfp(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1034, vD, vA, vB, "vmaxfp\tv%u, v%u, v%u\n", FALSE);
}

/** vector float mult add: vD = vA * vB + vC */
void
ppc_vmaddfp(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC)
{
   /* note arg order */
   emit_va(p, 46, vD, vA, vC, vB, "vmaddfp\tv%u, v%u, v%u, v%u\n");
}

/** vector float negative mult subtract: vD = vA - vB * vC */
void
ppc_vnmsubfp(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC)
{
   /* note arg order */
   emit_va(p, 47, vD, vB, vA, vC, "vnmsubfp\tv%u, v%u, v%u, v%u\n");
}

/** vector float compare greater than */
void
ppc_vcmpgtfpx(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vxr(p, 710, vD, vA, vB, "vcmpgtfpx\tv%u, v%u, v%u");
}

/** vector float compare greater than or equal to */
void
ppc_vcmpgefpx(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vxr(p, 454, vD, vA, vB, "vcmpgefpx\tv%u, v%u, v%u");
}

/** vector float compare equal */
void
ppc_vcmpeqfpx(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vxr(p, 198, vD, vA, vB, "vcmpeqfpx\tv%u, v%u, v%u");
}

/** vector float 2^x */
void
ppc_vexptefp(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 394, vD, 0, vB, "vexptefp\tv%u, 0%u, v%u\n", FALSE);
}

/** vector float log2(x) */
void
ppc_vlogefp(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 458, vD, 0, vB, "vlogefp\tv%u, 0%u, v%u\n", FALSE);
}

/** vector float reciprocol */
void
ppc_vrefp(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 266, vD, 0, vB, "vrefp\tv%u, 0%u, v%u\n", FALSE);
}

/** vector float reciprocol sqrt estimate */
void
ppc_vrsqrtefp(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 330, vD, 0, vB, "vrsqrtefp\tv%u, 0%u, v%u\n", FALSE);
}

/** vector float round to negative infinity */
void
ppc_vrfim(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 714, vD, 0, vB, "vrfim\tv%u, 0%u, v%u\n", FALSE);
}

/** vector float round to positive infinity */
void
ppc_vrfip(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 650, vD, 0, vB, "vrfip\tv%u, 0%u, v%u\n", FALSE);
}

/** vector float round to nearest int */
void
ppc_vrfin(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 522, vD, 0, vB, "vrfin\tv%u, 0%u, v%u\n", FALSE);
}

/** vector float round to int toward zero */
void
ppc_vrfiz(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 586, vD, 0, vB, "vrfiz\tv%u, 0%u, v%u\n", FALSE);
}

/** vector store: store vR at mem[rA+rB] */
void
ppc_stvx(struct ppc_function *p, uint vR, uint rA, uint rB)
{
   emit_x(p, 31, vR, rA, rB, 231, "stvx\tv%u, r%u, r%u\n");
}

/** vector load: vR = mem[rA+rB] */
void
ppc_lvx(struct ppc_function *p, uint vR, uint rA, uint rB)
{
   emit_x(p, 31, vR, rA, rB, 103, "lvx\tv%u, r%u, r%u\n");
}

/** load vector element word: vR = mem_word[ra+rb] */
void
ppc_lvewx(struct ppc_function *p, uint vR, uint rA, uint rB)
{
   emit_x(p, 31, vR, rA, rB, 71, "lvewx\tv%u, r%u, r%u\n");
}




/**
 ** vector bitwise operations
 **/

/** vector and */
void
ppc_vand(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1028, vD, vA, vB, "vand\tv%u, v%u, v%u\n", FALSE);
}

/** vector and complement */
void
ppc_vandc(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1092, vD, vA, vB, "vandc\tv%u, v%u, v%u\n", FALSE);
}

/** vector or */
void
ppc_vor(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1156, vD, vA, vB, "vor\tv%u, v%u, v%u\n", FALSE);
}

/** vector nor */
void
ppc_vnor(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1284, vD, vA, vB, "vnor\tv%u, v%u, v%u\n", FALSE);
}

/** vector xor */
void
ppc_vxor(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1220, vD, vA, vB, "vxor\tv%u, v%u, v%u\n", FALSE);
}

/** Pseudo-instruction: vector move */
void
ppc_vmove(struct ppc_function *p, uint vD, uint vA)
{
   boolean print = p->print;
   p->print = FALSE;
   ppc_vor(p, vD, vA, vA);
   if (print) {
      indent(p);
      printf("vor\tv%u, v%u, v%u \t# v%u = v%u\n", vD, vA, vA, vD, vA);
   }
   p->print = print;
}

/** Set vector register to {0,0,0,0} */
void
ppc_vzero(struct ppc_function *p, uint vr)
{
   boolean print = p->print;
   p->print = FALSE;
   ppc_vxor(p, vr, vr, vr);
   if (print) {
      indent(p);
      printf("vxor\tv%u, v%u, v%u \t# v%u = {0,0,0,0}\n", vr, vr, vr, vr);
   }
   p->print = print;
}




/**
 ** Vector shuffle / select / splat / etc
 **/

/** vector permute */
void
ppc_vperm(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC)
{
   emit_va(p, 43, vD, vA, vB, vC, "vperm\tr%u, r%u, r%u, r%u");
}

/** vector select */
void
ppc_vsel(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC)
{
   emit_va(p, 42, vD, vA, vB, vC, "vsel\tr%u, r%u, r%u, r%u");
}

/** vector splat byte */
void
ppc_vspltb(struct ppc_function *p, uint vD, uint vB, uint imm)
{
   emit_vx(p, 42, vD, imm, vB, "vspltb\tv%u, v%u, %u\n", TRUE);
}

/** vector splat half word */
void
ppc_vsplthw(struct ppc_function *p, uint vD, uint vB, uint imm)
{
   emit_vx(p, 588, vD, imm, vB, "vsplthw\tv%u, v%u, %u\n", TRUE);
}

/** vector splat word */
void
ppc_vspltw(struct ppc_function *p, uint vD, uint vB, uint imm)
{
   emit_vx(p, 652, vD, imm, vB, "vspltw\tv%u, v%u, %u\n", TRUE);
}

/** vector splat signed immediate word */
void
ppc_vspltisw(struct ppc_function *p, uint vD, int imm)
{
   assert(imm >= -16);
   assert(imm < 15);
   emit_vx(p, 908, vD, imm, 0, "vspltisw\tv%u, %d, %u\n", FALSE);
}

/** vector shift left word: vD[word] = vA[word] << (vB[word] & 0x1f) */
void
ppc_vslw(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 388, vD, vA, vB, "vslw\tv%u, v%u, v%u\n", FALSE);
}




/**
 ** integer arithmetic
 **/

/** rt = ra + imm */
void
ppc_addi(struct ppc_function *p, uint rt, uint ra, int imm)
{
   emit_d(p, 14, rt, ra, imm, "addi\tr%u, r%u, %d\n", FALSE);
}

/** rt = ra + (imm << 16) */
void
ppc_addis(struct ppc_function *p, uint rt, uint ra, int imm)
{
   emit_d(p, 15, rt, ra, imm, "addis\tr%u, r%u, %d\n", FALSE);
}

/** rt = ra + rb */
void
ppc_add(struct ppc_function *p, uint rt, uint ra, uint rb)
{
   emit_xo(p, 31, rt, ra, rb, 0, 266, 0, "add\tr%u, r%u, r%u\n");
}

/** rt = ra AND ra */
void
ppc_and(struct ppc_function *p, uint rt, uint ra, uint rb)
{
   emit_x(p, 31, ra, rt, rb, 28, "and\tr%u, r%u, r%u\n");  /* note argument order */
}

/** rt = ra AND imm */
void
ppc_andi(struct ppc_function *p, uint rt, uint ra, int imm)
{
   /* note argument order */
   emit_d(p, 28, ra, rt, imm, "andi\tr%u, r%u, %d\n", FALSE);
}

/** rt = ra OR ra */
void
ppc_or(struct ppc_function *p, uint rt, uint ra, uint rb)
{
   emit_x(p, 31, ra, rt, rb, 444, "or\tr%u, r%u, r%u\n");  /* note argument order */
}

/** rt = ra OR imm */
void
ppc_ori(struct ppc_function *p, uint rt, uint ra, int imm)
{
   /* note argument order */
   emit_d(p, 24, ra, rt, imm, "ori\tr%u, r%u, %d\n", FALSE);
}

/** rt = ra XOR ra */
void
ppc_xor(struct ppc_function *p, uint rt, uint ra, uint rb)
{
   emit_x(p, 31, ra, rt, rb, 316, "xor\tr%u, r%u, r%u\n");  /* note argument order */
}

/** rt = ra XOR imm */
void
ppc_xori(struct ppc_function *p, uint rt, uint ra, int imm)
{
   /* note argument order */
   emit_d(p, 26, ra, rt, imm, "xori\tr%u, r%u, %d\n", FALSE);
}

/** pseudo instruction: move: rt = ra */
void
ppc_mr(struct ppc_function *p, uint rt, uint ra)
{
   ppc_or(p, rt, ra, ra);
}

/** pseudo instruction: load immediate: rt = imm */
void
ppc_li(struct ppc_function *p, uint rt, int imm)
{
   boolean print = p->print;
   p->print = FALSE;
   ppc_addi(p, rt, 0, imm);
   if (print) {
      indent(p);
      printf("addi\tr%u, r0, %d \t# r%u = %d\n", rt, imm, rt, imm);
   }
   p->print = print;
}

/** rt = imm << 16 */
void
ppc_lis(struct ppc_function *p, uint rt, int imm)
{
   ppc_addis(p, rt, 0, imm);
}

/** rt = imm */
void
ppc_load_int(struct ppc_function *p, uint rt, int imm)
{
   ppc_lis(p, rt, (imm >> 16));          /* rt = imm >> 16 */
   ppc_ori(p, rt, rt, (imm & 0xffff));   /* rt = rt | (imm & 0xffff) */
}




/**
 ** integer load/store
 **/

/** store rs at memory[(ra)+d],
 * then update ra = (ra)+d
 */
void
ppc_stwu(struct ppc_function *p, uint rs, uint ra, int d)
{
   emit_d(p, 37, rs, ra, d, "stwu\tr%u, %d(r%u)\n", TRUE);
}

/** store rs at memory[(ra)+d] */
void
ppc_stw(struct ppc_function *p, uint rs, uint ra, int d)
{
   emit_d(p, 36, rs, ra, d, "stw\tr%u, %d(r%u)\n", TRUE);
}

/** Load rt = mem[(ra)+d];  then zero set high 32 bits to zero. */
void
ppc_lwz(struct ppc_function *p, uint rt, uint ra, int d)
{
   emit_d(p, 32, rt, ra, d, "lwz\tr%u, %d(r%u)\n", TRUE);
}



/**
 ** Float (non-vector) arithmetic
 **/

/** add: frt = fra + frb */
void
ppc_fadd(struct ppc_function *p, uint frt, uint fra, uint frb)
{
   emit_a(p, 63, frt, fra, frb, 21, 0, "fadd\tf%u, f%u, f%u\n");
}

/** sub: frt = fra - frb */
void
ppc_fsub(struct ppc_function *p, uint frt, uint fra, uint frb)
{
   emit_a(p, 63, frt, fra, frb, 20, 0, "fsub\tf%u, f%u, f%u\n");
}

/** convert to int: rt = (int) ra */
void
ppc_fctiwz(struct ppc_function *p, uint rt, uint fra)
{
   emit_x(p, 63, rt, 0, fra, 15, "fctiwz\tr%u, r%u, r%u\n");
}

/** store frs at mem[(ra)+offset] */
void
ppc_stfs(struct ppc_function *p, uint frs, uint ra, int offset)
{
   emit_d(p, 52, frs, ra, offset, "stfs\tr%u, %d(r%u)\n", TRUE);
}

/** store frs at mem[(ra)+(rb)] */
void
ppc_stfiwx(struct ppc_function *p, uint frs, uint ra, uint rb)
{
   emit_x(p, 31, frs, ra, rb, 983, "stfiwx\tr%u, r%u, r%u\n");
}

/** load frt = mem[(ra)+offset] */
void
ppc_lfs(struct ppc_function *p, uint frt, uint ra, int offset)
{
   emit_d(p, 48, frt, ra, offset, "stfs\tr%u, %d(r%u)\n", TRUE);
}





/**
 ** branch instructions
 **/

/** BLR: Branch to link register (p. 35) */
void
ppc_blr(struct ppc_function *p)
{
   emit_i(p, 18, 0, 0, 1);
   if (p->print) {
      indent(p);
      printf("blr\n");
   }
}

/** Branch Conditional to Link Register (p. 36) */
void
ppc_bclr(struct ppc_function *p, uint condOp, uint branchHint, uint condReg)
{
   emit_xl(p, 19, condOp, condReg, branchHint, 16, 0);
   if (p->print) {
      indent(p);
      printf("bclr\t%u %u %u\n", condOp, branchHint, condReg);
   }
}

/** Pseudo instruction: return from subroutine */
void
ppc_return(struct ppc_function *p)
{
   ppc_bclr(p, BRANCH_COND_ALWAYS, BRANCH_HINT_SUB_RETURN, 0);
}
