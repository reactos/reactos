/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * TGSI to PowerPC code generation.
 */

#include "pipe/p_config.h"

#if defined(PIPE_ARCH_PPC)

#include "util/u_debug.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_sse.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi_dump.h"
#include "tgsi_exec.h"
#include "tgsi_ppc.h"
#include "rtasm/rtasm_ppc.h"


/**
 * Since it's pretty much impossible to form PPC vector immediates, load
 * them from memory here:
 */
PIPE_ALIGN_VAR(16) const float 
ppc_builtin_constants[] = {
   1.0f, -128.0f, 128.0, 0.0
};


#define FOR_EACH_CHANNEL( CHAN )\
   for (CHAN = 0; CHAN < NUM_CHANNELS; CHAN++)

#define IS_DST0_CHANNEL_ENABLED( INST, CHAN )\
   ((INST).Dst[0].Register.WriteMask & (1 << (CHAN)))

#define IF_IS_DST0_CHANNEL_ENABLED( INST, CHAN )\
   if (IS_DST0_CHANNEL_ENABLED( INST, CHAN ))

#define FOR_EACH_DST0_ENABLED_CHANNEL( INST, CHAN )\
   FOR_EACH_CHANNEL( CHAN )\
      IF_IS_DST0_CHANNEL_ENABLED( INST, CHAN )

#define CHAN_X 0
#define CHAN_Y 1
#define CHAN_Z 2
#define CHAN_W 3


/**
 * How many TGSI temps should be implemented with real PPC vector registers
 * rather than memory.
 */
#define MAX_PPC_TEMPS 3


/**
 * Context/state used during code gen.
 */
struct gen_context
{
   struct ppc_function *f;
   int inputs_reg;    /**< GP register pointing to input params */
   int outputs_reg;   /**< GP register pointing to output params */
   int temps_reg;     /**< GP register pointing to temporary "registers" */
   int immed_reg;     /**< GP register pointing to immediates buffer */
   int const_reg;     /**< GP register pointing to constants buffer */
   int builtins_reg;  /**< GP register pointint to built-in constants */

   int offset_reg;    /**< used to reduce redundant li instructions */
   int offset_value;

   int one_vec;       /**< vector register with {1.0, 1.0, 1.0, 1.0} */
   int bit31_vec;     /**< vector register with {1<<31, 1<<31, 1<<31, 1<<31} */

   /**
    * Map TGSI temps to PPC vector temps.
    * We have 32 PPC vector regs.  Use 16 of them for storing 4 TGSI temps.
    * XXX currently only do this for TGSI temps [0..MAX_PPC_TEMPS-1].
    */
   int temps_map[MAX_PPC_TEMPS][4];

   /**
    * Cache of src registers.
    * This is used to avoid redundant load instructions.
    */
   struct {
      struct tgsi_full_src_register src;
      uint chan;
      uint vec;
   } regs[12];  /* 3 src regs, 4 channels */
   uint num_regs;
};


/**
 * Initialize code generation context.
 */
static void
init_gen_context(struct gen_context *gen, struct ppc_function *func)
{
   uint i;

   memset(gen, 0, sizeof(*gen));
   gen->f = func;
   gen->inputs_reg = ppc_reserve_register(func, 3);   /* first function param */
   gen->outputs_reg = ppc_reserve_register(func, 4);  /* second function param */
   gen->temps_reg = ppc_reserve_register(func, 5);    /* ... */
   gen->immed_reg = ppc_reserve_register(func, 6);
   gen->const_reg = ppc_reserve_register(func, 7);
   gen->builtins_reg = ppc_reserve_register(func, 8);
   gen->one_vec = -1;
   gen->bit31_vec = -1;
   gen->offset_reg = -1;
   gen->offset_value = -9999999;
   for (i = 0; i < MAX_PPC_TEMPS; i++) {
      gen->temps_map[i][0] = ppc_allocate_vec_register(gen->f);
      gen->temps_map[i][1] = ppc_allocate_vec_register(gen->f);
      gen->temps_map[i][2] = ppc_allocate_vec_register(gen->f);
      gen->temps_map[i][3] = ppc_allocate_vec_register(gen->f);
   }
}


/**
 * Is the given TGSI register stored as a real PPC vector register?
 */
static boolean
is_ppc_vec_temporary(const struct tgsi_full_src_register *reg)
{
   return (reg->Register.File == TGSI_FILE_TEMPORARY &&
           reg->Register.Index < MAX_PPC_TEMPS);
}


/**
 * Is the given TGSI register stored as a real PPC vector register?
 */
static boolean
is_ppc_vec_temporary_dst(const struct tgsi_full_dst_register *reg)
{
   return (reg->Register.File == TGSI_FILE_TEMPORARY &&
           reg->Register.Index < MAX_PPC_TEMPS);
}



/**
 * All PPC vector load/store instructions form an effective address
 * by adding the contents of two registers.  For example:
 *    lvx v2,r8,r9   # v2 = memory[r8 + r9]
 *    stvx v2,r8,r9  # memory[r8 + r9] = v2;
 * So our lvx/stvx instructions are typically preceded by an 'li' instruction
 * to load r9 (above) with an immediate (an offset).
 * This code emits that 'li' instruction, but only if the offset value is
 * different than the previous 'li'.
 * This optimization seems to save about 10% in the instruction count.
 * Note that we need to unconditionally emit an 'li' inside basic blocks
 * (such as inside loops).
 */
static int
emit_li_offset(struct gen_context *gen, int offset)
{
   if (gen->offset_reg <= 0) {
      /* allocate a GP register for storing load/store offset */
      gen->offset_reg = ppc_allocate_register(gen->f);
   }

   /* emit new 'li' if offset is changing */
   if (gen->offset_value < 0 || gen->offset_value != offset) {
      gen->offset_value = offset;
      ppc_li(gen->f, gen->offset_reg, offset);
   }

   return gen->offset_reg;
}


/**
 * Forces subsequent emit_li_offset() calls to emit an 'li'.
 * To be called at the top of basic blocks.
 */
static void
reset_li_offset(struct gen_context *gen)
{
   gen->offset_value = -9999999;
}



/**
 * Load the given vector register with {value, value, value, value}.
 * The value must be in the ppu_builtin_constants[] array.
 * We wouldn't need this if there was a simple way to load PPC vector
 * registers with immediate values!
 */
static void
load_constant_vec(struct gen_context *gen, int dst_vec, float value)
{
   uint pos;
   for (pos = 0; pos < Elements(ppc_builtin_constants); pos++) {
      if (ppc_builtin_constants[pos] == value) {
         int offset = pos * 4;
         int offset_reg = emit_li_offset(gen, offset);

         /* Load 4-byte word into vector register.
          * The vector slot depends on the effective address we load from.
          * We know that our builtins start at a 16-byte boundary so we
          * know that 'swizzle' tells us which vector slot will have the
          * loaded word.  The other vector slots will be undefined.
          */
         ppc_lvewx(gen->f, dst_vec, gen->builtins_reg, offset_reg);
         /* splat word[pos % 4] across the vector reg */
         ppc_vspltw(gen->f, dst_vec, dst_vec, pos % 4);
         return;
      }
   }
   assert(0 && "Need to add new constant to ppc_builtin_constants array");
}


/**
 * Return index of vector register containing {1.0, 1.0, 1.0, 1.0}.
 */
static int
gen_one_vec(struct gen_context *gen)
{
   if (gen->one_vec < 0) {
      gen->one_vec = ppc_allocate_vec_register(gen->f);
      load_constant_vec(gen, gen->one_vec, 1.0f);
   }
   return gen->one_vec;
}

/**
 * Return index of vector register containing {1<<31, 1<<31, 1<<31, 1<<31}.
 */
static int
gen_get_bit31_vec(struct gen_context *gen)
{
   if (gen->bit31_vec < 0) {
      gen->bit31_vec = ppc_allocate_vec_register(gen->f);
      ppc_vspltisw(gen->f, gen->bit31_vec, -1);
      ppc_vslw(gen->f, gen->bit31_vec, gen->bit31_vec, gen->bit31_vec);
   }
   return gen->bit31_vec;
}


/**
 * Register fetch.  Return PPC vector register with result.
 */
static int
emit_fetch(struct gen_context *gen,
           const struct tgsi_full_src_register *reg,
           const unsigned chan_index)
{
   uint swizzle = tgsi_util_get_full_src_register_swizzle(reg, chan_index);
   int dst_vec = -1;

   switch (swizzle) {
   case TGSI_SWIZZLE_X:
   case TGSI_SWIZZLE_Y:
   case TGSI_SWIZZLE_Z:
   case TGSI_SWIZZLE_W:
      switch (reg->Register.File) {
      case TGSI_FILE_INPUT:
         {
            int offset = (reg->Register.Index * 4 + swizzle) * 16;
            int offset_reg = emit_li_offset(gen, offset);
            dst_vec = ppc_allocate_vec_register(gen->f);
            ppc_lvx(gen->f, dst_vec, gen->inputs_reg, offset_reg);
         }
         break;
      case TGSI_FILE_SYSTEM_VALUE:
         assert(!"unhandled system value in tgsi_ppc.c");
         break;
      case TGSI_FILE_TEMPORARY:
         if (is_ppc_vec_temporary(reg)) {
            /* use PPC vec register */
            dst_vec = gen->temps_map[reg->Register.Index][swizzle];
         }
         else {
            /* use memory-based temp register "file" */
            int offset = (reg->Register.Index * 4 + swizzle) * 16;
            int offset_reg = emit_li_offset(gen, offset);
            dst_vec = ppc_allocate_vec_register(gen->f);
            ppc_lvx(gen->f, dst_vec, gen->temps_reg, offset_reg);
         }
         break;
      case TGSI_FILE_IMMEDIATE:
         {
            int offset = (reg->Register.Index * 4 + swizzle) * 4;
            int offset_reg = emit_li_offset(gen, offset);
            dst_vec = ppc_allocate_vec_register(gen->f);
            /* Load 4-byte word into vector register.
             * The vector slot depends on the effective address we load from.
             * We know that our immediates start at a 16-byte boundary so we
             * know that 'swizzle' tells us which vector slot will have the
             * loaded word.  The other vector slots will be undefined.
             */
            ppc_lvewx(gen->f, dst_vec, gen->immed_reg, offset_reg);
            /* splat word[swizzle] across the vector reg */
            ppc_vspltw(gen->f, dst_vec, dst_vec, swizzle);
         }
         break;
      case TGSI_FILE_CONSTANT:
         {
            int offset = (reg->Register.Index * 4 + swizzle) * 4;
            int offset_reg = emit_li_offset(gen, offset);
            dst_vec = ppc_allocate_vec_register(gen->f);
            /* Load 4-byte word into vector register.
             * The vector slot depends on the effective address we load from.
             * We know that our constants start at a 16-byte boundary so we
             * know that 'swizzle' tells us which vector slot will have the
             * loaded word.  The other vector slots will be undefined.
             */
            ppc_lvewx(gen->f, dst_vec, gen->const_reg, offset_reg);
            /* splat word[swizzle] across the vector reg */
            ppc_vspltw(gen->f, dst_vec, dst_vec, swizzle);
         }
         break;
      default:
         assert( 0 );
      }
      break;
   default:
      assert( 0 );
   }

   assert(dst_vec >= 0);

   {
      uint sign_op = tgsi_util_get_full_src_register_sign_mode(reg, chan_index);
      if (sign_op != TGSI_UTIL_SIGN_KEEP) {
         int bit31_vec = gen_get_bit31_vec(gen);
         int dst_vec2;

         if (is_ppc_vec_temporary(reg)) {
            /* need to use a new temp */
            dst_vec2 = ppc_allocate_vec_register(gen->f);
         }
         else {
            dst_vec2 = dst_vec;
         }

         switch (sign_op) {
         case TGSI_UTIL_SIGN_CLEAR:
            /* vec = vec & ~bit31 */
            ppc_vandc(gen->f, dst_vec2, dst_vec, bit31_vec);
            break;
         case TGSI_UTIL_SIGN_SET:
            /* vec = vec | bit31 */
            ppc_vor(gen->f, dst_vec2, dst_vec, bit31_vec);
            break;
         case TGSI_UTIL_SIGN_TOGGLE:
            /* vec = vec ^ bit31 */
            ppc_vxor(gen->f, dst_vec2, dst_vec, bit31_vec);
            break;
         default:
            assert(0);
         }
         return dst_vec2;
      }
   }

   return dst_vec;
}



/**
 * Test if two TGSI src registers refer to the same memory location.
 * We use this to avoid redundant register loads.
 */
static boolean
equal_src_locs(const struct tgsi_full_src_register *a, uint chan_a,
               const struct tgsi_full_src_register *b, uint chan_b)
{
   int swz_a, swz_b;
   int sign_a, sign_b;
   if (a->Register.File != b->Register.File)
      return FALSE;
   if (a->Register.Index != b->Register.Index)
      return FALSE;
   swz_a = tgsi_util_get_full_src_register_swizzle(a, chan_a);
   swz_b = tgsi_util_get_full_src_register_swizzle(b, chan_b);
   if (swz_a != swz_b)
      return FALSE;
   sign_a = tgsi_util_get_full_src_register_sign_mode(a, chan_a);
   sign_b = tgsi_util_get_full_src_register_sign_mode(b, chan_b);
   if (sign_a != sign_b)
      return FALSE;
   return TRUE;
}


/**
 * Given a TGSI src register and channel index, return the PPC vector
 * register containing the value.  We use a cache to prevent re-loading
 * the same register multiple times.
 * \return index of PPC vector register with the desired src operand
 */
static int
get_src_vec(struct gen_context *gen,
            struct tgsi_full_instruction *inst, int src_reg, uint chan)
{
   const const struct tgsi_full_src_register *src = 
      &inst->Src[src_reg];
   int vec;
   uint i;

   /* check the cache */
   for (i = 0; i < gen->num_regs; i++) {
      if (equal_src_locs(&gen->regs[i].src, gen->regs[i].chan, src, chan)) {
         /* cache hit */
         assert(gen->regs[i].vec >= 0);
         return gen->regs[i].vec;
      }
   }

   /* cache miss: allocate new vec reg and emit fetch/load code */
   vec = emit_fetch(gen, src, chan);
   gen->regs[gen->num_regs].src = *src;
   gen->regs[gen->num_regs].chan = chan;
   gen->regs[gen->num_regs].vec = vec;
   gen->num_regs++;

   assert(gen->num_regs <= Elements(gen->regs));

   assert(vec >= 0);

   return vec;
}


/**
 * Clear the src operand cache.  To be called at the end of each emit function.
 */
static void
release_src_vecs(struct gen_context *gen)
{
   uint i;
   for (i = 0; i < gen->num_regs; i++) {
      const const struct tgsi_full_src_register src = gen->regs[i].src;
      if (!is_ppc_vec_temporary(&src)) {
         ppc_release_vec_register(gen->f, gen->regs[i].vec);
      }
   }
   gen->num_regs = 0;
}



static int
get_dst_vec(struct gen_context *gen, 
            const struct tgsi_full_instruction *inst,
            unsigned chan_index)
{
   const struct tgsi_full_dst_register *reg = &inst->Dst[0];

   if (is_ppc_vec_temporary_dst(reg)) {
      int vec = gen->temps_map[reg->Register.Index][chan_index];
      return vec;
   }
   else {
      return ppc_allocate_vec_register(gen->f);
   }
}


/**
 * Register store.  Store 'src_vec' at location indicated by 'reg'.
 * \param free_vec  Should the src_vec be released when done?
 */
static void
emit_store(struct gen_context *gen,
           int src_vec,
           const struct tgsi_full_instruction *inst,
           unsigned chan_index,
           boolean free_vec)
{
   const struct tgsi_full_dst_register *reg = &inst->Dst[0];

   switch (reg->Register.File) {
   case TGSI_FILE_OUTPUT:
      {
         int offset = (reg->Register.Index * 4 + chan_index) * 16;
         int offset_reg = emit_li_offset(gen, offset);
         ppc_stvx(gen->f, src_vec, gen->outputs_reg, offset_reg);
      }
      break;
   case TGSI_FILE_TEMPORARY:
      if (is_ppc_vec_temporary_dst(reg)) {
         if (!free_vec) {
            int dst_vec = gen->temps_map[reg->Register.Index][chan_index];
            if (dst_vec != src_vec)
               ppc_vmove(gen->f, dst_vec, src_vec);
         }
         free_vec = FALSE;
      }
      else {
         int offset = (reg->Register.Index * 4 + chan_index) * 16;
         int offset_reg = emit_li_offset(gen, offset);
         ppc_stvx(gen->f, src_vec, gen->temps_reg, offset_reg);
      }
      break;
#if 0
   case TGSI_FILE_ADDRESS:
      emit_addrs(
         func,
         xmm,
         reg->Register.Index,
         chan_index );
      break;
#endif
   default:
      assert( 0 );
   }

#if 0
   switch( inst->Instruction.Saturate ) {
   case TGSI_SAT_NONE:
      break;

   case TGSI_SAT_ZERO_ONE:
      /* assert( 0 ); */
      break;

   case TGSI_SAT_MINUS_PLUS_ONE:
      assert( 0 );
      break;
   }
#endif

   if (free_vec)
      ppc_release_vec_register(gen->f, src_vec);
}


static void
emit_scalar_unaryop(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int v0, v1;
   uint chan_index;

   v0 = get_src_vec(gen, inst, 0, CHAN_X);
   v1 = ppc_allocate_vec_register(gen->f);

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_RSQ:
      /* v1 = 1.0 / sqrt(v0) */
      ppc_vrsqrtefp(gen->f, v1, v0);
      break;
   case TGSI_OPCODE_RCP:
      /* v1 = 1.0 / v0 */
      ppc_vrefp(gen->f, v1, v0);
      break;
   default:
      assert(0);
   }

   FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
      emit_store(gen, v1, inst, chan_index, FALSE);
   }

   release_src_vecs(gen);
   ppc_release_vec_register(gen->f, v1);
}


static void
emit_unaryop(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   uint chan_index;

   FOR_EACH_DST0_ENABLED_CHANNEL(*inst, chan_index) {
      int v0 = get_src_vec(gen, inst, 0, chan_index);   /* v0 = srcreg[0] */
      int v1 = get_dst_vec(gen, inst, chan_index);
      switch (inst->Instruction.Opcode) {
      case TGSI_OPCODE_ABS:
         /* turn off the most significant bit of each vector float word */
         {
            int bit31_vec = gen_get_bit31_vec(gen);
            ppc_vandc(gen->f, v1, v0, bit31_vec); /* v1 = v0 & ~bit31 */
         }
         break;
      case TGSI_OPCODE_FLR:
         ppc_vrfim(gen->f, v1, v0);         /* v1 = floor(v0) */
         break;
      case TGSI_OPCODE_FRC:
         ppc_vrfim(gen->f, v1, v0);      /* tmp = floor(v0) */
         ppc_vsubfp(gen->f, v1, v0, v1); /* v1 = v0 - v1 */
         break;
      case TGSI_OPCODE_EX2:
         ppc_vexptefp(gen->f, v1, v0);     /* v1 = 2^v0 */
         break;
      case TGSI_OPCODE_LG2:
         /* XXX this may be broken! */
         ppc_vlogefp(gen->f, v1, v0);      /* v1 = log2(v0) */
         break;
      case TGSI_OPCODE_MOV:
         if (v0 != v1)
            ppc_vmove(gen->f, v1, v0);
         break;
      default:
         assert(0);
      }
      emit_store(gen, v1, inst, chan_index, TRUE);  /* store v0 */
   }

   release_src_vecs(gen);
}


static void
emit_binop(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int zero_vec = -1;
   uint chan;

   if (inst->Instruction.Opcode == TGSI_OPCODE_MUL) {
      zero_vec = ppc_allocate_vec_register(gen->f);
      ppc_vzero(gen->f, zero_vec);
   }

   FOR_EACH_DST0_ENABLED_CHANNEL(*inst, chan) {
      /* fetch src operands */
      int v0 = get_src_vec(gen, inst, 0, chan);
      int v1 = get_src_vec(gen, inst, 1, chan);
      int v2 = get_dst_vec(gen, inst, chan);

      /* emit binop */
      switch (inst->Instruction.Opcode) {
      case TGSI_OPCODE_ADD:
         ppc_vaddfp(gen->f, v2, v0, v1);
         break;
      case TGSI_OPCODE_SUB:
         ppc_vsubfp(gen->f, v2, v0, v1);
         break;
      case TGSI_OPCODE_MUL:
         ppc_vmaddfp(gen->f, v2, v0, v1, zero_vec);
         break;
      case TGSI_OPCODE_MIN:
         ppc_vminfp(gen->f, v2, v0, v1);
         break;
      case TGSI_OPCODE_MAX:
         ppc_vmaxfp(gen->f, v2, v0, v1);
         break;
      default:
         assert(0);
      }

      /* store v2 */
      emit_store(gen, v2, inst, chan, TRUE);
   }

   if (inst->Instruction.Opcode == TGSI_OPCODE_MUL)
      ppc_release_vec_register(gen->f, zero_vec);

   release_src_vecs(gen);
}


static void
emit_triop(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   uint chan;

   FOR_EACH_DST0_ENABLED_CHANNEL(*inst, chan) {
      /* fetch src operands */
      int v0 = get_src_vec(gen, inst, 0, chan);
      int v1 = get_src_vec(gen, inst, 1, chan);
      int v2 = get_src_vec(gen, inst, 2, chan);
      int v3 = get_dst_vec(gen, inst, chan);

      /* emit ALU */
      switch (inst->Instruction.Opcode) {
      case TGSI_OPCODE_MAD:
         ppc_vmaddfp(gen->f, v3, v0, v1, v2);   /* v3 = v0 * v1 + v2 */
         break;
      case TGSI_OPCODE_LRP:
         ppc_vsubfp(gen->f, v3, v1, v2);        /* v3 = v1 - v2 */
         ppc_vmaddfp(gen->f, v3, v0, v3, v2);   /* v3 = v0 * v3 + v2 */
         break;
      default:
         assert(0);
      }

      /* store v3 */
      emit_store(gen, v3, inst, chan, TRUE);
   }

   release_src_vecs(gen);
}


/**
 * Vector comparisons, resulting in 1.0 or 0.0 values.
 */
static void
emit_inequality(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   uint chan;
   int one_vec = gen_one_vec(gen);

   FOR_EACH_DST0_ENABLED_CHANNEL(*inst, chan) {
      /* fetch src operands */
      int v0 = get_src_vec(gen, inst, 0, chan);
      int v1 = get_src_vec(gen, inst, 1, chan);
      int v2 = get_dst_vec(gen, inst, chan);
      boolean complement = FALSE;

      switch (inst->Instruction.Opcode) {
      case TGSI_OPCODE_SNE:
         complement = TRUE;
         /* fall-through */
      case TGSI_OPCODE_SEQ:
         ppc_vcmpeqfpx(gen->f, v2, v0, v1); /* v2 = v0 == v1 ? ~0 : 0 */
         break;

      case TGSI_OPCODE_SGE:
         complement = TRUE;
         /* fall-through */
      case TGSI_OPCODE_SLT:
         ppc_vcmpgtfpx(gen->f, v2, v1, v0); /* v2 = v1 > v0 ? ~0 : 0 */
         break;

      case TGSI_OPCODE_SLE:
         complement = TRUE;
         /* fall-through */
      case TGSI_OPCODE_SGT:
         ppc_vcmpgtfpx(gen->f, v2, v0, v1); /* v2 = v0 > v1 ? ~0 : 0 */
         break;
      default:
         assert(0);
      }

      /* v2 is now {0,0,0,0} or {~0,~0,~0,~0} */

      if (complement)
         ppc_vandc(gen->f, v2, one_vec, v2);    /* v2 = one_vec & ~v2 */
      else
         ppc_vand(gen->f, v2, one_vec, v2);     /* v2 = one_vec & v2 */

      /* store v2 */
      emit_store(gen, v2, inst, chan, TRUE);
   }

   release_src_vecs(gen);
}


static void
emit_dotprod(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int v0, v1, v2;
   uint chan_index;

   v2 = ppc_allocate_vec_register(gen->f);

   ppc_vzero(gen->f, v2);                  /* v2 = {0, 0, 0, 0} */

   v0 = get_src_vec(gen, inst, 0, CHAN_X); /* v0 = src0.XXXX */
   v1 = get_src_vec(gen, inst, 1, CHAN_X); /* v1 = src1.XXXX */
   ppc_vmaddfp(gen->f, v2, v0, v1, v2);    /* v2 = v0 * v1 + v2 */

   v0 = get_src_vec(gen, inst, 0, CHAN_Y); /* v0 = src0.YYYY */
   v1 = get_src_vec(gen, inst, 1, CHAN_Y); /* v1 = src1.YYYY */
   ppc_vmaddfp(gen->f, v2, v0, v1, v2);    /* v2 = v0 * v1 + v2 */

   v0 = get_src_vec(gen, inst, 0, CHAN_Z); /* v0 = src0.ZZZZ */
   v1 = get_src_vec(gen, inst, 1, CHAN_Z); /* v1 = src1.ZZZZ */
   ppc_vmaddfp(gen->f, v2, v0, v1, v2);    /* v2 = v0 * v1 + v2 */

   if (inst->Instruction.Opcode == TGSI_OPCODE_DP4) {
      v0 = get_src_vec(gen, inst, 0, CHAN_W); /* v0 = src0.WWWW */
      v1 = get_src_vec(gen, inst, 1, CHAN_W); /* v1 = src1.WWWW */
      ppc_vmaddfp(gen->f, v2, v0, v1, v2);    /* v2 = v0 * v1 + v2 */
   }
   else if (inst->Instruction.Opcode == TGSI_OPCODE_DPH) {
      v1 = get_src_vec(gen, inst, 1, CHAN_W); /* v1 = src1.WWWW */
      ppc_vaddfp(gen->f, v2, v2, v1);         /* v2 = v2 + v1 */
   }

   FOR_EACH_DST0_ENABLED_CHANNEL(*inst, chan_index) {
      emit_store(gen, v2, inst, chan_index, FALSE);  /* store v2, free v2 later */
   }

   release_src_vecs(gen);

   ppc_release_vec_register(gen->f, v2);
}


/** Approximation for vr = pow(va, vb) */
static void
ppc_vec_pow(struct ppc_function *f, int vr, int va, int vb)
{
   /* pow(a,b) ~= exp2(log2(a) * b) */
   int t_vec = ppc_allocate_vec_register(f);
   int zero_vec = ppc_allocate_vec_register(f);

   ppc_vzero(f, zero_vec);

   ppc_vlogefp(f, t_vec, va);                   /* t = log2(va) */
   ppc_vmaddfp(f, t_vec, t_vec, vb, zero_vec);  /* t = t * vb + zero */
   ppc_vexptefp(f, vr, t_vec);                  /* vr = 2^t */

   ppc_release_vec_register(f, t_vec);
   ppc_release_vec_register(f, zero_vec);
}


static void
emit_lit(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int one_vec = gen_one_vec(gen);

   /* Compute X */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_X)) {
      emit_store(gen, one_vec, inst, CHAN_X, FALSE);
   }

   /* Compute Y, Z */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Y) ||
       IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Z)) {
      int x_vec;
      int zero_vec = ppc_allocate_vec_register(gen->f);

      x_vec = get_src_vec(gen, inst, 0, CHAN_X);  /* x_vec = src[0].x */

      ppc_vzero(gen->f, zero_vec);                /* zero = {0,0,0,0} */
      ppc_vmaxfp(gen->f, x_vec, x_vec, zero_vec); /* x_vec = max(x_vec, 0) */

      if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Y)) {
         emit_store(gen, x_vec, inst, CHAN_Y, FALSE);
      }

      if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Z)) {
         int y_vec, w_vec;
         int z_vec = ppc_allocate_vec_register(gen->f);
         int pow_vec = ppc_allocate_vec_register(gen->f);
         int pos_vec = ppc_allocate_vec_register(gen->f);
         int p128_vec = ppc_allocate_vec_register(gen->f);
         int n128_vec = ppc_allocate_vec_register(gen->f);

         y_vec = get_src_vec(gen, inst, 0, CHAN_Y);  /* y_vec = src[0].y */
         ppc_vmaxfp(gen->f, y_vec, y_vec, zero_vec); /* y_vec = max(y_vec, 0) */

         w_vec = get_src_vec(gen, inst, 0, CHAN_W);  /* w_vec = src[0].w */

         /* clamp W to [-128, 128] */
         load_constant_vec(gen, p128_vec, 128.0f);
         load_constant_vec(gen, n128_vec, -128.0f);
         ppc_vmaxfp(gen->f, w_vec, w_vec, n128_vec); /* w = max(w, -128) */
         ppc_vminfp(gen->f, w_vec, w_vec, p128_vec); /* w = min(w, 128) */

         /* if temp.x > 0
          *    z = pow(tmp.y, tmp.w)
          * else
          *    z = 0.0
          */
         ppc_vec_pow(gen->f, pow_vec, y_vec, w_vec);      /* pow = pow(y, w) */
         ppc_vcmpgtfpx(gen->f, pos_vec, x_vec, zero_vec); /* pos = x > 0 */
         ppc_vand(gen->f, z_vec, pow_vec, pos_vec);       /* z = pow & pos */

         emit_store(gen, z_vec, inst, CHAN_Z, FALSE);

         ppc_release_vec_register(gen->f, z_vec);
         ppc_release_vec_register(gen->f, pow_vec);
         ppc_release_vec_register(gen->f, pos_vec);
         ppc_release_vec_register(gen->f, p128_vec);
         ppc_release_vec_register(gen->f, n128_vec);
      }

      ppc_release_vec_register(gen->f, zero_vec);
   }

   /* Compute W */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_W)) {
      emit_store(gen, one_vec, inst, CHAN_W, FALSE);
   }

   release_src_vecs(gen);
}


static void
emit_exp(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   const int one_vec = gen_one_vec(gen);
   int src_vec;

   /* get src arg */
   src_vec = get_src_vec(gen, inst, 0, CHAN_X);

   /* Compute X = 2^floor(src) */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_X)) {
      int dst_vec = get_dst_vec(gen, inst, CHAN_X);
      int tmp_vec = ppc_allocate_vec_register(gen->f);
      ppc_vrfim(gen->f, tmp_vec, src_vec);             /* tmp = floor(src); */
      ppc_vexptefp(gen->f, dst_vec, tmp_vec);          /* dst = 2 ^ tmp */
      emit_store(gen, dst_vec, inst, CHAN_X, TRUE);
      ppc_release_vec_register(gen->f, tmp_vec);
   }

   /* Compute Y = src - floor(src) */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Y)) {
      int dst_vec = get_dst_vec(gen, inst, CHAN_Y);
      int tmp_vec = ppc_allocate_vec_register(gen->f);
      ppc_vrfim(gen->f, tmp_vec, src_vec);             /* tmp = floor(src); */
      ppc_vsubfp(gen->f, dst_vec, src_vec, tmp_vec);   /* dst = src - tmp */
      emit_store(gen, dst_vec, inst, CHAN_Y, TRUE);
      ppc_release_vec_register(gen->f, tmp_vec);
   }

   /* Compute Z = RoughApprox2ToX(src) */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Z)) {
      int dst_vec = get_dst_vec(gen, inst, CHAN_Z);
      ppc_vexptefp(gen->f, dst_vec, src_vec);          /* dst = 2 ^ src */
      emit_store(gen, dst_vec, inst, CHAN_Z, TRUE);
   }

   /* Compute W = 1.0 */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_W)) {
      emit_store(gen, one_vec, inst, CHAN_W, FALSE);
   }

   release_src_vecs(gen);
}


static void
emit_log(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   const int bit31_vec = gen_get_bit31_vec(gen);
   const int one_vec = gen_one_vec(gen);
   int src_vec, abs_vec;

   /* get src arg */
   src_vec = get_src_vec(gen, inst, 0, CHAN_X);

   /* compute abs(src) */
   abs_vec = ppc_allocate_vec_register(gen->f);
   ppc_vandc(gen->f, abs_vec, src_vec, bit31_vec);     /* abs = src & ~bit31 */

   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_X) &&
       IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Y)) {

      /* compute tmp = floor(log2(abs)) */
      int tmp_vec = ppc_allocate_vec_register(gen->f);
      ppc_vlogefp(gen->f, tmp_vec, abs_vec);           /* tmp = log2(abs) */
      ppc_vrfim(gen->f, tmp_vec, tmp_vec);             /* tmp = floor(tmp); */

      /* Compute X = tmp */
      if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_X)) {
         emit_store(gen, tmp_vec, inst, CHAN_X, FALSE);
      }
      
      /* Compute Y = abs / 2^tmp */
      if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Y)) {
         const int zero_vec = ppc_allocate_vec_register(gen->f);
         ppc_vzero(gen->f, zero_vec);
         ppc_vexptefp(gen->f, tmp_vec, tmp_vec);       /* tmp = 2 ^ tmp */
         ppc_vrefp(gen->f, tmp_vec, tmp_vec);          /* tmp = 1 / tmp */
         /* tmp = abs * tmp + zero */
         ppc_vmaddfp(gen->f, tmp_vec, abs_vec, tmp_vec, zero_vec);
         emit_store(gen, tmp_vec, inst, CHAN_Y, FALSE);
         ppc_release_vec_register(gen->f, zero_vec);
      }

      ppc_release_vec_register(gen->f, tmp_vec);
   }

   /* Compute Z = RoughApproxLog2(abs) */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Z)) {
      int dst_vec = get_dst_vec(gen, inst, CHAN_Z);
      ppc_vlogefp(gen->f, dst_vec, abs_vec);           /* dst = log2(abs) */
      emit_store(gen, dst_vec, inst, CHAN_Z, TRUE);
   }

   /* Compute W = 1.0 */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_W)) {
      emit_store(gen, one_vec, inst, CHAN_W, FALSE);
   }

   ppc_release_vec_register(gen->f, abs_vec);
   release_src_vecs(gen);
}


static void
emit_pow(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int s0_vec = get_src_vec(gen, inst, 0, CHAN_X);
   int s1_vec = get_src_vec(gen, inst, 1, CHAN_X);
   int pow_vec = ppc_allocate_vec_register(gen->f);
   int chan;

   ppc_vec_pow(gen->f, pow_vec, s0_vec, s1_vec);

   FOR_EACH_DST0_ENABLED_CHANNEL(*inst, chan) {
      emit_store(gen, pow_vec, inst, chan, FALSE);
   }

   ppc_release_vec_register(gen->f, pow_vec);

   release_src_vecs(gen);
}


static void
emit_xpd(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int x0_vec, y0_vec, z0_vec;
   int x1_vec, y1_vec, z1_vec;
   int zero_vec, tmp_vec;
   int tmp2_vec;

   zero_vec = ppc_allocate_vec_register(gen->f);
   ppc_vzero(gen->f, zero_vec);

   tmp_vec = ppc_allocate_vec_register(gen->f);
   tmp2_vec = ppc_allocate_vec_register(gen->f);

   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Y) ||
       IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Z)) {
      x0_vec = get_src_vec(gen, inst, 0, CHAN_X);
      x1_vec = get_src_vec(gen, inst, 1, CHAN_X);
   }
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_X) ||
       IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Z)) {
      y0_vec = get_src_vec(gen, inst, 0, CHAN_Y);
      y1_vec = get_src_vec(gen, inst, 1, CHAN_Y);
   }
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_X) ||
       IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Y)) {
      z0_vec = get_src_vec(gen, inst, 0, CHAN_Z);
      z1_vec = get_src_vec(gen, inst, 1, CHAN_Z);
   }

   IF_IS_DST0_CHANNEL_ENABLED(*inst, CHAN_X) {
      /* tmp = y0 * z1 */
      ppc_vmaddfp(gen->f, tmp_vec, y0_vec, z1_vec, zero_vec);
      /* tmp = tmp - z0 * y1*/
      ppc_vnmsubfp(gen->f, tmp_vec, tmp_vec, z0_vec, y1_vec);
      emit_store(gen, tmp_vec, inst, CHAN_X, FALSE);
   }
   IF_IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Y) {
      /* tmp = z0 * x1 */
      ppc_vmaddfp(gen->f, tmp_vec, z0_vec, x1_vec, zero_vec);
      /* tmp = tmp - x0 * z1 */
      ppc_vnmsubfp(gen->f, tmp_vec, tmp_vec, x0_vec, z1_vec);
      emit_store(gen, tmp_vec, inst, CHAN_Y, FALSE);
   }
   IF_IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Z) {
      /* tmp = x0 * y1 */
      ppc_vmaddfp(gen->f, tmp_vec, x0_vec, y1_vec, zero_vec);
      /* tmp = tmp - y0 * x1 */
      ppc_vnmsubfp(gen->f, tmp_vec, tmp_vec, y0_vec, x1_vec);
      emit_store(gen, tmp_vec, inst, CHAN_Z, FALSE);
   }
   /* W is undefined */

   ppc_release_vec_register(gen->f, tmp_vec);
   ppc_release_vec_register(gen->f, zero_vec);
   release_src_vecs(gen);
}

static int
emit_instruction(struct gen_context *gen,
                 struct tgsi_full_instruction *inst)
{

   /* we don't handle saturation/clamping yet */
   if (inst->Instruction.Saturate != TGSI_SAT_NONE)
      return 0;

   /* need to use extra temps to fix SOA dependencies : */
   if (tgsi_check_soa_dependencies(inst))
      return FALSE;

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_MOV:
   case TGSI_OPCODE_ABS:
   case TGSI_OPCODE_FLR:
   case TGSI_OPCODE_FRC:
   case TGSI_OPCODE_EX2:
   case TGSI_OPCODE_LG2:
      emit_unaryop(gen, inst);
      break;
   case TGSI_OPCODE_RSQ:
   case TGSI_OPCODE_RCP:
      emit_scalar_unaryop(gen, inst);
      break;
   case TGSI_OPCODE_ADD:
   case TGSI_OPCODE_SUB:
   case TGSI_OPCODE_MUL:
   case TGSI_OPCODE_MIN:
   case TGSI_OPCODE_MAX:
      emit_binop(gen, inst);
      break;
   case TGSI_OPCODE_SEQ:
   case TGSI_OPCODE_SNE:
   case TGSI_OPCODE_SLT:
   case TGSI_OPCODE_SGT:
   case TGSI_OPCODE_SLE:
   case TGSI_OPCODE_SGE:
      emit_inequality(gen, inst);
      break;
   case TGSI_OPCODE_MAD:
   case TGSI_OPCODE_LRP:
      emit_triop(gen, inst);
      break;
   case TGSI_OPCODE_DP3:
   case TGSI_OPCODE_DP4:
   case TGSI_OPCODE_DPH:
      emit_dotprod(gen, inst);
      break;
   case TGSI_OPCODE_LIT:
      emit_lit(gen, inst);
      break;
   case TGSI_OPCODE_LOG:
      emit_log(gen, inst);
      break;
   case TGSI_OPCODE_EXP:
      emit_exp(gen, inst);
      break;
   case TGSI_OPCODE_POW:
      emit_pow(gen, inst);
      break;
   case TGSI_OPCODE_XPD:
      emit_xpd(gen, inst);
      break;
   case TGSI_OPCODE_END:
      /* normal end */
      return 1;
   default:
      return 0;
   }
   return 1;
}


static void
emit_declaration(
   struct ppc_function *func,
   struct tgsi_full_declaration *decl )
{
   if( decl->Declaration.File == TGSI_FILE_INPUT ||
       decl->Declaration.File == TGSI_FILE_SYSTEM_VALUE ) {
#if 0
      unsigned first, last, mask;
      unsigned i, j;

      first = decl->Range.First;
      last = decl->Range.Last;
      mask = decl->Declaration.UsageMask;

      for( i = first; i <= last; i++ ) {
         for( j = 0; j < NUM_CHANNELS; j++ ) {
            if( mask & (1 << j) ) {
               switch( decl->Declaration.Interpolate ) {
               case TGSI_INTERPOLATE_CONSTANT:
                  emit_coef_a0( func, 0, i, j );
                  emit_inputs( func, 0, i, j );
                  break;

               case TGSI_INTERPOLATE_LINEAR:
                  emit_tempf( func, 0, 0, TGSI_SWIZZLE_X );
                  emit_coef_dadx( func, 1, i, j );
                  emit_tempf( func, 2, 0, TGSI_SWIZZLE_Y );
                  emit_coef_dady( func, 3, i, j );
                  emit_mul( func, 0, 1 );    /* x * dadx */
                  emit_coef_a0( func, 4, i, j );
                  emit_mul( func, 2, 3 );    /* y * dady */
                  emit_add( func, 0, 4 );    /* x * dadx + a0 */
                  emit_add( func, 0, 2 );    /* x * dadx + y * dady + a0 */
                  emit_inputs( func, 0, i, j );
                  break;

               case TGSI_INTERPOLATE_PERSPECTIVE:
                  emit_tempf( func, 0, 0, TGSI_SWIZZLE_X );
                  emit_coef_dadx( func, 1, i, j );
                  emit_tempf( func, 2, 0, TGSI_SWIZZLE_Y );
                  emit_coef_dady( func, 3, i, j );
                  emit_mul( func, 0, 1 );    /* x * dadx */
                  emit_tempf( func, 4, 0, TGSI_SWIZZLE_W );
                  emit_coef_a0( func, 5, i, j );
                  emit_rcp( func, 4, 4 );    /* 1.0 / w */
                  emit_mul( func, 2, 3 );    /* y * dady */
                  emit_add( func, 0, 5 );    /* x * dadx + a0 */
                  emit_add( func, 0, 2 );    /* x * dadx + y * dady + a0 */
                  emit_mul( func, 0, 4 );    /* (x * dadx + y * dady + a0) / w */
                  emit_inputs( func, 0, i, j );
                  break;

               default:
                  assert( 0 );
		  break;
               }
            }
         }
      }
#endif
   }
}



static void
emit_prologue(struct ppc_function *func)
{
   /* XXX set up stack frame */
}


static void
emit_epilogue(struct ppc_function *func)
{
   ppc_comment(func, -4, "Epilogue:");
   ppc_return(func);
   /* XXX restore prev stack frame */
#if 0
   debug_printf("PPC: Emitted %u instructions\n", func->num_inst);
#endif
}



/**
 * Translate a TGSI vertex/fragment shader to PPC code.
 *
 * \param tokens  the TGSI input shader
 * \param func  the output PPC code/function
 * \param immediates  buffer to place immediates, later passed to PPC func
 * \return TRUE for success, FALSE if translation failed
 */
boolean
tgsi_emit_ppc(const struct tgsi_token *tokens,
              struct ppc_function *func,
              float (*immediates)[4],
              boolean do_swizzles )
{
   static int use_ppc_asm = -1;
   struct tgsi_parse_context parse;
   /*boolean instruction_phase = FALSE;*/
   unsigned ok = 1;
   uint num_immediates = 0;
   struct gen_context gen;
   uint ic = 0;

   if (use_ppc_asm < 0) {
      /* If GALLIUM_NOPPC is set, don't use PPC codegen */
      use_ppc_asm = !debug_get_bool_option("GALLIUM_NOPPC", FALSE);
   }
   if (!use_ppc_asm)
      return FALSE;

   if (0) {
      debug_printf("\n********* TGSI->PPC ********\n");
      tgsi_dump(tokens, 0);
   }

   util_init_math();

   init_gen_context(&gen, func);

   emit_prologue(func);

   tgsi_parse_init( &parse, tokens );

   while (!tgsi_parse_end_of_tokens(&parse) && ok) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         if (parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_FRAGMENT) {
            emit_declaration(func, &parse.FullToken.FullDeclaration );
         }
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (func->print) {
            _debug_printf("# ");
            ic++;
            tgsi_dump_instruction(&parse.FullToken.FullInstruction, ic);
         }

         ok = emit_instruction(&gen, &parse.FullToken.FullInstruction);

	 if (!ok) {
            uint opcode = parse.FullToken.FullInstruction.Instruction.Opcode;
	    debug_printf("failed to translate tgsi opcode %d (%s) to PPC (%s)\n", 
			 opcode,
                         tgsi_get_opcode_name(opcode),
                         parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_VERTEX ?
                         "vertex shader" : "fragment shader");
	 }
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         /* splat each immediate component into a float[4] vector for SoA */
         {
            const uint size = parse.FullToken.FullImmediate.Immediate.NrTokens - 1;
            uint i;
            assert(size <= 4);
            assert(num_immediates < TGSI_EXEC_NUM_IMMEDIATES);
            for (i = 0; i < size; i++) {
               immediates[num_immediates][i] =
		  parse.FullToken.FullImmediate.u[i].Float;
            }
            num_immediates++;
         }
         break;

      case TGSI_TOKEN_TYPE_PROPERTY:
         break;

      default:
	 ok = 0;
         assert( 0 );
      }
   }

   emit_epilogue(func);

   tgsi_parse_free( &parse );

   if (ppc_num_instructions(func) == 0) {
      /* ran out of memory for instructions */
      ok = FALSE;
   }

   if (!ok)
      debug_printf("TGSI->PPC translation failed\n");

   return ok;
}

#else

void ppc_dummy_func(void);

void ppc_dummy_func(void)
{
}

#endif /* PIPE_ARCH_PPC */
