/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include <strings.h>

#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "tnl/t_context.h"
#include "intel_batchbuffer.h"

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_program.h"


#define A0_DEST( reg ) (((reg)&UREG_TYPE_NR_MASK)>>UREG_A0_DEST_SHIFT_LEFT)
#define D0_DEST( reg ) (((reg)&UREG_TYPE_NR_MASK)>>UREG_A0_DEST_SHIFT_LEFT)
#define T0_DEST( reg ) (((reg)&UREG_TYPE_NR_MASK)>>UREG_A0_DEST_SHIFT_LEFT)
#define A0_SRC0( reg ) (((reg)&UREG_MASK)>>UREG_A0_SRC0_SHIFT_LEFT)
#define A1_SRC0( reg ) (((reg)&UREG_MASK)<<UREG_A1_SRC0_SHIFT_RIGHT)
#define A1_SRC1( reg ) (((reg)&UREG_MASK)>>UREG_A1_SRC1_SHIFT_LEFT)
#define A2_SRC1( reg ) (((reg)&UREG_MASK)<<UREG_A2_SRC1_SHIFT_RIGHT)
#define A2_SRC2( reg ) (((reg)&UREG_MASK)>>UREG_A2_SRC2_SHIFT_LEFT)

/* These are special, and don't have swizzle/negate bits.
 */
#define T0_SAMPLER( reg )     (GET_UREG_NR(reg)<<T0_SAMPLER_NR_SHIFT)
#define T1_ADDRESS_REG( reg ) ((GET_UREG_NR(reg)<<T1_ADDRESS_REG_NR_SHIFT) | \
			       (GET_UREG_TYPE(reg)<<T1_ADDRESS_REG_TYPE_SHIFT))


/* Macros for translating UREG's into the various register fields used
 * by the I915 programmable unit.
 */
#define UREG_A0_DEST_SHIFT_LEFT  (UREG_TYPE_SHIFT - A0_DEST_TYPE_SHIFT)
#define UREG_A0_SRC0_SHIFT_LEFT  (UREG_TYPE_SHIFT - A0_SRC0_TYPE_SHIFT)
#define UREG_A1_SRC0_SHIFT_RIGHT (A1_SRC0_CHANNEL_W_SHIFT - UREG_CHANNEL_W_SHIFT)
#define UREG_A1_SRC1_SHIFT_LEFT  (UREG_TYPE_SHIFT - A1_SRC1_TYPE_SHIFT)
#define UREG_A2_SRC1_SHIFT_RIGHT (A2_SRC1_CHANNEL_W_SHIFT - UREG_CHANNEL_W_SHIFT)
#define UREG_A2_SRC2_SHIFT_LEFT  (UREG_TYPE_SHIFT - A2_SRC2_TYPE_SHIFT)

#define UREG_MASK         0xffffff00
#define UREG_TYPE_NR_MASK ((REG_TYPE_MASK << UREG_TYPE_SHIFT) | \
  			   (REG_NR_MASK << UREG_NR_SHIFT))


#define I915_CONSTFLAG_PARAM 0x1f

GLuint
i915_get_temp(struct i915_fragment_program *p)
{
   int bit = ffs(~p->temp_flag);
   if (!bit) {
      fprintf(stderr, "%s: out of temporaries\n", __FILE__);
      exit(1);
   }

   p->temp_flag |= 1 << (bit - 1);
   return UREG(REG_TYPE_R, (bit - 1));
}


GLuint
i915_get_utemp(struct i915_fragment_program * p)
{
   int bit = ffs(~p->utemp_flag);
   if (!bit) {
      fprintf(stderr, "%s: out of temporaries\n", __FILE__);
      exit(1);
   }

   p->utemp_flag |= 1 << (bit - 1);
   return UREG(REG_TYPE_U, (bit - 1));
}

void
i915_release_utemps(struct i915_fragment_program *p)
{
   p->utemp_flag = ~0x7;
}


GLuint
i915_emit_decl(struct i915_fragment_program *p,
               GLuint type, GLuint nr, GLuint d0_flags)
{
   GLuint reg = UREG(type, nr);

   if (type == REG_TYPE_T) {
      if (p->decl_t & (1 << nr))
         return reg;

      p->decl_t |= (1 << nr);
   }
   else if (type == REG_TYPE_S) {
      if (p->decl_s & (1 << nr))
         return reg;

      p->decl_s |= (1 << nr);
   }
   else
      return reg;

   *(p->decl++) = (D0_DCL | D0_DEST(reg) | d0_flags);
   *(p->decl++) = D1_MBZ;
   *(p->decl++) = D2_MBZ;

   p->nr_decl_insn++;
   return reg;
}

GLuint
i915_emit_arith(struct i915_fragment_program * p,
                GLuint op,
                GLuint dest,
                GLuint mask,
                GLuint saturate, GLuint src0, GLuint src1, GLuint src2)
{
   GLuint c[3];
   GLuint nr_const = 0;

   assert(GET_UREG_TYPE(dest) != REG_TYPE_CONST);
   dest = UREG(GET_UREG_TYPE(dest), GET_UREG_NR(dest));
   assert(dest);

   if (GET_UREG_TYPE(src0) == REG_TYPE_CONST)
      c[nr_const++] = 0;
   if (GET_UREG_TYPE(src1) == REG_TYPE_CONST)
      c[nr_const++] = 1;
   if (GET_UREG_TYPE(src2) == REG_TYPE_CONST)
      c[nr_const++] = 2;

   /* Recursively call this function to MOV additional const values
    * into temporary registers.  Use utemp registers for this -
    * currently shouldn't be possible to run out, but keep an eye on
    * this.
    */
   if (nr_const > 1) {
      GLuint s[3], first, i, old_utemp_flag;

      s[0] = src0;
      s[1] = src1;
      s[2] = src2;
      old_utemp_flag = p->utemp_flag;

      first = GET_UREG_NR(s[c[0]]);
      for (i = 1; i < nr_const; i++) {
         if (GET_UREG_NR(s[c[i]]) != first) {
            GLuint tmp = i915_get_utemp(p);

            i915_emit_arith(p, A0_MOV, tmp, A0_DEST_CHANNEL_ALL, 0,
                            s[c[i]], 0, 0);
            s[c[i]] = tmp;
         }
      }

      src0 = s[0];
      src1 = s[1];
      src2 = s[2];
      p->utemp_flag = old_utemp_flag;   /* restore */
   }

   *(p->csr++) = (op | A0_DEST(dest) | mask | saturate | A0_SRC0(src0));
   *(p->csr++) = (A1_SRC0(src0) | A1_SRC1(src1));
   *(p->csr++) = (A2_SRC1(src1) | A2_SRC2(src2));

   p->nr_alu_insn++;
   return dest;
}

GLuint i915_emit_texld( struct i915_fragment_program *p,
			GLuint dest,
			GLuint destmask,
			GLuint sampler,
			GLuint coord,
			GLuint op )
{
   if (coord != UREG(GET_UREG_TYPE(coord), GET_UREG_NR(coord))) {
      /* No real way to work around this in the general case - need to
       * allocate and declare a new temporary register (a utemp won't
       * do).  Will fallback for now.
       */
      i915_program_error(p, "Can't (yet) swizzle TEX arguments");
      return 0;
   }

   /* Don't worry about saturate as we only support  
    */
   if (destmask != A0_DEST_CHANNEL_ALL) {
      GLuint tmp = i915_get_utemp(p);
      i915_emit_texld( p, tmp, A0_DEST_CHANNEL_ALL, sampler, coord, op );
      i915_emit_arith( p, A0_MOV, dest, destmask, 0, tmp, 0, 0 );
      return dest;
   }
   else {
      assert(GET_UREG_TYPE(dest) != REG_TYPE_CONST);
      assert(dest = UREG(GET_UREG_TYPE(dest), GET_UREG_NR(dest)));

      if (GET_UREG_TYPE(coord) != REG_TYPE_T) {
	 p->nr_tex_indirect++;
      }

      *(p->csr++) = (op | 
		     T0_DEST( dest ) |
		     T0_SAMPLER( sampler ));

      *(p->csr++) = T1_ADDRESS_REG( coord );
      *(p->csr++) = T2_MBZ;

      p->nr_tex_insn++;
      return dest;
   }
}


GLuint
i915_emit_const1f(struct i915_fragment_program * p, GLfloat c0)
{
   GLint reg, idx;

   if (c0 == 0.0)
      return swizzle(UREG(REG_TYPE_R, 0), ZERO, ZERO, ZERO, ZERO);
   if (c0 == 1.0)
      return swizzle(UREG(REG_TYPE_R, 0), ONE, ONE, ONE, ONE);

   for (reg = 0; reg < I915_MAX_CONSTANT; reg++) {
      if (p->constant_flags[reg] == I915_CONSTFLAG_PARAM)
         continue;
      for (idx = 0; idx < 4; idx++) {
         if (!(p->constant_flags[reg] & (1 << idx)) ||
             p->constant[reg][idx] == c0) {
            p->constant[reg][idx] = c0;
            p->constant_flags[reg] |= 1 << idx;
            if (reg + 1 > p->nr_constants)
               p->nr_constants = reg + 1;
            return swizzle(UREG(REG_TYPE_CONST, reg), idx, ZERO, ZERO, ONE);
         }
      }
   }

   fprintf(stderr, "%s: out of constants\n", __FUNCTION__);
   p->error = 1;
   return 0;
}

GLuint
i915_emit_const2f(struct i915_fragment_program * p, GLfloat c0, GLfloat c1)
{
   GLint reg, idx;

   if (c0 == 0.0)
      return swizzle(i915_emit_const1f(p, c1), ZERO, X, Z, W);
   if (c0 == 1.0)
      return swizzle(i915_emit_const1f(p, c1), ONE, X, Z, W);

   if (c1 == 0.0)
      return swizzle(i915_emit_const1f(p, c0), X, ZERO, Z, W);
   if (c1 == 1.0)
      return swizzle(i915_emit_const1f(p, c0), X, ONE, Z, W);

   for (reg = 0; reg < I915_MAX_CONSTANT; reg++) {
      if (p->constant_flags[reg] == 0xf ||
          p->constant_flags[reg] == I915_CONSTFLAG_PARAM)
         continue;
      for (idx = 0; idx < 3; idx++) {
         if (!(p->constant_flags[reg] & (3 << idx))) {
            p->constant[reg][idx] = c0;
            p->constant[reg][idx + 1] = c1;
            p->constant_flags[reg] |= 3 << idx;
            if (reg + 1 > p->nr_constants)
               p->nr_constants = reg + 1;
            return swizzle(UREG(REG_TYPE_CONST, reg), idx, idx + 1, ZERO,
                           ONE);
         }
      }
   }

   fprintf(stderr, "%s: out of constants\n", __FUNCTION__);
   p->error = 1;
   return 0;
}



GLuint
i915_emit_const4f(struct i915_fragment_program * p,
                  GLfloat c0, GLfloat c1, GLfloat c2, GLfloat c3)
{
   GLint reg;

   for (reg = 0; reg < I915_MAX_CONSTANT; reg++) {
      if (p->constant_flags[reg] == 0xf &&
          p->constant[reg][0] == c0 &&
          p->constant[reg][1] == c1 &&
          p->constant[reg][2] == c2 && p->constant[reg][3] == c3) {
         return UREG(REG_TYPE_CONST, reg);
      }
      else if (p->constant_flags[reg] == 0) {
         p->constant[reg][0] = c0;
         p->constant[reg][1] = c1;
         p->constant[reg][2] = c2;
         p->constant[reg][3] = c3;
         p->constant_flags[reg] = 0xf;
         if (reg + 1 > p->nr_constants)
            p->nr_constants = reg + 1;
         return UREG(REG_TYPE_CONST, reg);
      }
   }

   fprintf(stderr, "%s: out of constants\n", __FUNCTION__);
   p->error = 1;
   return 0;
}


GLuint
i915_emit_const4fv(struct i915_fragment_program * p, const GLfloat * c)
{
   return i915_emit_const4f(p, c[0], c[1], c[2], c[3]);
}


GLuint
i915_emit_param4fv(struct i915_fragment_program * p, const GLfloat * values)
{
   GLint reg, i;

   for (i = 0; i < p->nr_params; i++) {
      if (p->param[i].values == values)
         return UREG(REG_TYPE_CONST, p->param[i].reg);
   }


   for (reg = 0; reg < I915_MAX_CONSTANT; reg++) {
      if (p->constant_flags[reg] == 0) {
         p->constant_flags[reg] = I915_CONSTFLAG_PARAM;
         i = p->nr_params++;

         p->param[i].values = values;
         p->param[i].reg = reg;
         p->params_uptodate = 0;

         if (reg + 1 > p->nr_constants)
            p->nr_constants = reg + 1;
         return UREG(REG_TYPE_CONST, reg);
      }
   }

   fprintf(stderr, "%s: out of constants\n", __FUNCTION__);
   p->error = 1;
   return 0;
}



void
i915_program_error(struct i915_fragment_program *p, const char *msg)
{
   _mesa_problem(NULL, "i915_program_error: %s", msg);
   p->error = 1;
}


void
i915_init_program(struct i915_context *i915, struct i915_fragment_program *p)
{
   GLcontext *ctx = &i915->intel.ctx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   p->translated = 0;
   p->params_uptodate = 0;
   p->on_hardware = 0;
   p->error = 0;

   p->nr_tex_indirect = 1;      /* correct? */
   p->nr_tex_insn = 0;
   p->nr_alu_insn = 0;
   p->nr_decl_insn = 0;

   p->ctx = ctx;
   memset(p->constant_flags, 0, sizeof(p->constant_flags));

   p->nr_constants = 0;
   p->csr = p->program;
   p->decl = p->declarations;
   p->decl_s = 0;
   p->decl_t = 0;
   p->temp_flag = 0xffff000;
   p->utemp_flag = ~0x7;
   p->wpos_tex = -1;
   p->depth_written = 0;
   p->nr_params = 0;

   p->src_texture = UREG_BAD;
   p->src_previous = UREG(REG_TYPE_T, T_DIFFUSE);
   p->last_tex_stage = 0;
   p->VB = &tnl->vb;

   *(p->decl++) = _3DSTATE_PIXEL_SHADER_PROGRAM;
}


void
i915_fini_program(struct i915_fragment_program *p)
{
   GLuint program_size = p->csr - p->program;
   GLuint decl_size = p->decl - p->declarations;

   if (p->nr_tex_indirect > I915_MAX_TEX_INDIRECT)
      i915_program_error(p, "Exceeded max nr indirect texture lookups");

   if (p->nr_tex_insn > I915_MAX_TEX_INSN)
      i915_program_error(p, "Exceeded max TEX instructions");

   if (p->nr_alu_insn > I915_MAX_ALU_INSN)
      i915_program_error(p, "Exceeded max ALU instructions");

   if (p->nr_decl_insn > I915_MAX_DECL_INSN)
      i915_program_error(p, "Exceeded max DECL instructions");

   if (p->error) {
      p->FragProg.Base.NumNativeInstructions = 0;
      p->FragProg.Base.NumNativeAluInstructions = 0;
      p->FragProg.Base.NumNativeTexInstructions = 0;
      p->FragProg.Base.NumNativeTexIndirections = 0;
   }
   else {
      p->FragProg.Base.NumNativeInstructions = (p->nr_alu_insn +
                                                p->nr_tex_insn +
                                                p->nr_decl_insn);
      p->FragProg.Base.NumNativeAluInstructions = p->nr_alu_insn;
      p->FragProg.Base.NumNativeTexInstructions = p->nr_tex_insn;
      p->FragProg.Base.NumNativeTexIndirections = p->nr_tex_indirect;
   }

   p->declarations[0] |= program_size + decl_size - 2;
}

void
i915_upload_program(struct i915_context *i915,
                    struct i915_fragment_program *p)
{
   GLuint program_size = p->csr - p->program;
   GLuint decl_size = p->decl - p->declarations;

   FALLBACK(&i915->intel, I915_FALLBACK_PROGRAM, p->error);

   /* Could just go straight to the batchbuffer from here:
    */
   if (i915->state.ProgramSize != (program_size + decl_size) ||
       memcmp(i915->state.Program + decl_size, p->program,
              program_size * sizeof(int)) != 0) {
      I915_STATECHANGE(i915, I915_UPLOAD_PROGRAM);
      memcpy(i915->state.Program, p->declarations, decl_size * sizeof(int));
      memcpy(i915->state.Program + decl_size, p->program,
             program_size * sizeof(int));
      i915->state.ProgramSize = decl_size + program_size;
   }

   /* Always seemed to get a failure if I used memcmp() to
    * shortcircuit this state upload.  Needs further investigation?
    */
   if (p->nr_constants) {
      GLuint nr = p->nr_constants;

      I915_ACTIVESTATE(i915, I915_UPLOAD_CONSTANTS, 1);
      I915_STATECHANGE(i915, I915_UPLOAD_CONSTANTS);

      i915->state.Constant[0] = _3DSTATE_PIXEL_SHADER_CONSTANTS | ((nr) * 4);
      i915->state.Constant[1] = (1 << (nr - 1)) | ((1 << (nr - 1)) - 1);

      memcpy(&i915->state.Constant[2], p->constant, 4 * sizeof(int) * (nr));
      i915->state.ConstantSize = 2 + (nr) * 4;

      if (0) {
         GLuint i;
         for (i = 0; i < nr; i++) {
            fprintf(stderr, "const[%d]: %f %f %f %f\n", i,
                    p->constant[i][0],
                    p->constant[i][1], p->constant[i][2], p->constant[i][3]);
         }
      }
   }
   else {
      I915_ACTIVESTATE(i915, I915_UPLOAD_CONSTANTS, 0);
   }

   p->on_hardware = 1;
}
