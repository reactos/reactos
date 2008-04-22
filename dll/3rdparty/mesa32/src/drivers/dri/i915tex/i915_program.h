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


#ifndef I915_PROGRAM_H
#define I915_PROGRAM_H

#include "i915_context.h"
#include "i915_reg.h"



/* Having zero and one in here makes the definition of swizzle a lot
 * easier.
 */
#define UREG_TYPE_SHIFT               29
#define UREG_NR_SHIFT                 24
#define UREG_CHANNEL_X_NEGATE_SHIFT   23
#define UREG_CHANNEL_X_SHIFT          20
#define UREG_CHANNEL_Y_NEGATE_SHIFT   19
#define UREG_CHANNEL_Y_SHIFT          16
#define UREG_CHANNEL_Z_NEGATE_SHIFT   15
#define UREG_CHANNEL_Z_SHIFT          12
#define UREG_CHANNEL_W_NEGATE_SHIFT   11
#define UREG_CHANNEL_W_SHIFT          8
#define UREG_CHANNEL_ZERO_NEGATE_MBZ  5
#define UREG_CHANNEL_ZERO_SHIFT       4
#define UREG_CHANNEL_ONE_NEGATE_MBZ   1
#define UREG_CHANNEL_ONE_SHIFT        0

#define UREG_BAD          0xffffffff    /* not a valid ureg */

#define X    SRC_X
#define Y    SRC_Y
#define Z    SRC_Z
#define W    SRC_W
#define ZERO SRC_ZERO
#define ONE  SRC_ONE

/* Construct a ureg:
 */
#define UREG( type, nr ) (((type)<< UREG_TYPE_SHIFT) |		\
			  ((nr)  << UREG_NR_SHIFT) |		\
			  (X     << UREG_CHANNEL_X_SHIFT) |	\
			  (Y     << UREG_CHANNEL_Y_SHIFT) |	\
			  (Z     << UREG_CHANNEL_Z_SHIFT) |	\
			  (W     << UREG_CHANNEL_W_SHIFT) |	\
			  (ZERO  << UREG_CHANNEL_ZERO_SHIFT) |	\
			  (ONE   << UREG_CHANNEL_ONE_SHIFT))

#define GET_CHANNEL_SRC( reg, channel ) ((reg<<(channel*4)) & (0xf<<20))
#define CHANNEL_SRC( src, channel ) (src>>(channel*4))

#define GET_UREG_TYPE(reg) (((reg)>>UREG_TYPE_SHIFT)&REG_TYPE_MASK)
#define GET_UREG_NR(reg)   (((reg)>>UREG_NR_SHIFT)&REG_NR_MASK)



#define UREG_XYZW_CHANNEL_MASK 0x00ffff00

/* One neat thing about the UREG representation:  
 */
static INLINE int
swizzle(int reg, int x, int y, int z, int w)
{
   return ((reg & ~UREG_XYZW_CHANNEL_MASK) |
           CHANNEL_SRC(GET_CHANNEL_SRC(reg, x), 0) |
           CHANNEL_SRC(GET_CHANNEL_SRC(reg, y), 1) |
           CHANNEL_SRC(GET_CHANNEL_SRC(reg, z), 2) |
           CHANNEL_SRC(GET_CHANNEL_SRC(reg, w), 3));
}

/* Another neat thing about the UREG representation:  
 */
static INLINE int
negate(int reg, int x, int y, int z, int w)
{
   return reg ^ (((x & 1) << UREG_CHANNEL_X_NEGATE_SHIFT) |
                 ((y & 1) << UREG_CHANNEL_Y_NEGATE_SHIFT) |
                 ((z & 1) << UREG_CHANNEL_Z_NEGATE_SHIFT) |
                 ((w & 1) << UREG_CHANNEL_W_NEGATE_SHIFT));
}


extern GLuint i915_get_temp(struct i915_fragment_program *p);
extern GLuint i915_get_utemp(struct i915_fragment_program *p);
extern void i915_release_utemps(struct i915_fragment_program *p);


extern GLuint i915_emit_texld(struct i915_fragment_program *p,
                              GLuint dest,
                              GLuint destmask,
                              GLuint sampler, GLuint coord, GLuint op);

extern GLuint i915_emit_arith(struct i915_fragment_program *p,
                              GLuint op,
                              GLuint dest,
                              GLuint mask,
                              GLuint saturate,
                              GLuint src0, GLuint src1, GLuint src2);

extern GLuint i915_emit_decl(struct i915_fragment_program *p,
                             GLuint type, GLuint nr, GLuint d0_flags);


extern GLuint i915_emit_const1f(struct i915_fragment_program *p, GLfloat c0);

extern GLuint i915_emit_const2f(struct i915_fragment_program *p,
                                GLfloat c0, GLfloat c1);

extern GLuint i915_emit_const4fv(struct i915_fragment_program *p,
                                 const GLfloat * c);

extern GLuint i915_emit_const4f(struct i915_fragment_program *p,
                                GLfloat c0, GLfloat c1,
                                GLfloat c2, GLfloat c3);


extern GLuint i915_emit_param4fv(struct i915_fragment_program *p,
                                 const GLfloat * values);

extern void i915_program_error(struct i915_fragment_program *p,
                               const char *msg);

extern void i915_init_program(struct i915_context *i915,
                              struct i915_fragment_program *p);

extern void i915_upload_program(struct i915_context *i915,
                                struct i915_fragment_program *p);

extern void i915_fini_program(struct i915_fragment_program *p);




#endif
