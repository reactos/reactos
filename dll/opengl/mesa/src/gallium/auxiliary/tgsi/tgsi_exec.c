/**************************************************************************
 * 
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2009-2010 VMware, Inc.  All rights Reserved.
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
 * TGSI interpreter/executor.
 *
 * Flow control information:
 *
 * Since we operate on 'quads' (4 pixels or 4 vertices in parallel)
 * flow control statements (IF/ELSE/ENDIF, LOOP/ENDLOOP) require special
 * care since a condition may be true for some quad components but false
 * for other components.
 *
 * We basically execute all statements (even if they're in the part of
 * an IF/ELSE clause that's "not taken") and use a special mask to
 * control writing to destination registers.  This is the ExecMask.
 * See store_dest().
 *
 * The ExecMask is computed from three other masks (CondMask, LoopMask and
 * ContMask) which are controlled by the flow control instructions (namely:
 * (IF/ELSE/ENDIF, LOOP/ENDLOOP and CONT).
 *
 *
 * Authors:
 *   Michal Krol
 *   Brian Paul
 */

#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi_exec.h"
#include "util/u_memory.h"
#include "util/u_math.h"


#define FAST_MATH 0

#define TILE_TOP_LEFT     0
#define TILE_TOP_RIGHT    1
#define TILE_BOTTOM_LEFT  2
#define TILE_BOTTOM_RIGHT 3

static void
micro_abs(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->f[0] = fabsf(src->f[0]);
   dst->f[1] = fabsf(src->f[1]);
   dst->f[2] = fabsf(src->f[2]);
   dst->f[3] = fabsf(src->f[3]);
}

static void
micro_arl(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->i[0] = (int)floorf(src->f[0]);
   dst->i[1] = (int)floorf(src->f[1]);
   dst->i[2] = (int)floorf(src->f[2]);
   dst->i[3] = (int)floorf(src->f[3]);
}

static void
micro_arr(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->i[0] = (int)floorf(src->f[0] + 0.5f);
   dst->i[1] = (int)floorf(src->f[1] + 0.5f);
   dst->i[2] = (int)floorf(src->f[2] + 0.5f);
   dst->i[3] = (int)floorf(src->f[3] + 0.5f);
}

static void
micro_ceil(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src)
{
   dst->f[0] = ceilf(src->f[0]);
   dst->f[1] = ceilf(src->f[1]);
   dst->f[2] = ceilf(src->f[2]);
   dst->f[3] = ceilf(src->f[3]);
}

static void
micro_clamp(union tgsi_exec_channel *dst,
            const union tgsi_exec_channel *src0,
            const union tgsi_exec_channel *src1,
            const union tgsi_exec_channel *src2)
{
   dst->f[0] = src0->f[0] < src1->f[0] ? src1->f[0] : src0->f[0] > src2->f[0] ? src2->f[0] : src0->f[0];
   dst->f[1] = src0->f[1] < src1->f[1] ? src1->f[1] : src0->f[1] > src2->f[1] ? src2->f[1] : src0->f[1];
   dst->f[2] = src0->f[2] < src1->f[2] ? src1->f[2] : src0->f[2] > src2->f[2] ? src2->f[2] : src0->f[2];
   dst->f[3] = src0->f[3] < src1->f[3] ? src1->f[3] : src0->f[3] > src2->f[3] ? src2->f[3] : src0->f[3];
}

static void
micro_cmp(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1,
          const union tgsi_exec_channel *src2)
{
   dst->f[0] = src0->f[0] < 0.0f ? src1->f[0] : src2->f[0];
   dst->f[1] = src0->f[1] < 0.0f ? src1->f[1] : src2->f[1];
   dst->f[2] = src0->f[2] < 0.0f ? src1->f[2] : src2->f[2];
   dst->f[3] = src0->f[3] < 0.0f ? src1->f[3] : src2->f[3];
}

static void
micro_cnd(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1,
          const union tgsi_exec_channel *src2)
{
   dst->f[0] = src2->f[0] > 0.5f ? src0->f[0] : src1->f[0];
   dst->f[1] = src2->f[1] > 0.5f ? src0->f[1] : src1->f[1];
   dst->f[2] = src2->f[2] > 0.5f ? src0->f[2] : src1->f[2];
   dst->f[3] = src2->f[3] > 0.5f ? src0->f[3] : src1->f[3];
}

static void
micro_cos(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->f[0] = cosf(src->f[0]);
   dst->f[1] = cosf(src->f[1]);
   dst->f[2] = cosf(src->f[2]);
   dst->f[3] = cosf(src->f[3]);
}

static void
micro_ddx(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->f[0] =
   dst->f[1] =
   dst->f[2] =
   dst->f[3] = src->f[TILE_BOTTOM_RIGHT] - src->f[TILE_BOTTOM_LEFT];
}

static void
micro_ddy(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->f[0] =
   dst->f[1] =
   dst->f[2] =
   dst->f[3] = src->f[TILE_BOTTOM_LEFT] - src->f[TILE_TOP_LEFT];
}

static void
micro_exp2(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src)
{
#if FAST_MATH
   dst->f[0] = util_fast_exp2(src->f[0]);
   dst->f[1] = util_fast_exp2(src->f[1]);
   dst->f[2] = util_fast_exp2(src->f[2]);
   dst->f[3] = util_fast_exp2(src->f[3]);
#else
#if DEBUG
   /* Inf is okay for this instruction, so clamp it to silence assertions. */
   uint i;
   union tgsi_exec_channel clamped;

   for (i = 0; i < 4; i++) {
      if (src->f[i] > 127.99999f) {
         clamped.f[i] = 127.99999f;
      } else if (src->f[i] < -126.99999f) {
         clamped.f[i] = -126.99999f;
      } else {
         clamped.f[i] = src->f[i];
      }
   }
   src = &clamped;
#endif /* DEBUG */

   dst->f[0] = powf(2.0f, src->f[0]);
   dst->f[1] = powf(2.0f, src->f[1]);
   dst->f[2] = powf(2.0f, src->f[2]);
   dst->f[3] = powf(2.0f, src->f[3]);
#endif /* FAST_MATH */
}

static void
micro_flr(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->f[0] = floorf(src->f[0]);
   dst->f[1] = floorf(src->f[1]);
   dst->f[2] = floorf(src->f[2]);
   dst->f[3] = floorf(src->f[3]);
}

static void
micro_frc(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->f[0] = src->f[0] - floorf(src->f[0]);
   dst->f[1] = src->f[1] - floorf(src->f[1]);
   dst->f[2] = src->f[2] - floorf(src->f[2]);
   dst->f[3] = src->f[3] - floorf(src->f[3]);
}

static void
micro_iabs(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src)
{
   dst->i[0] = src->i[0] >= 0 ? src->i[0] : -src->i[0];
   dst->i[1] = src->i[1] >= 0 ? src->i[1] : -src->i[1];
   dst->i[2] = src->i[2] >= 0 ? src->i[2] : -src->i[2];
   dst->i[3] = src->i[3] >= 0 ? src->i[3] : -src->i[3];
}

static void
micro_ineg(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src)
{
   dst->i[0] = -src->i[0];
   dst->i[1] = -src->i[1];
   dst->i[2] = -src->i[2];
   dst->i[3] = -src->i[3];
}

static void
micro_lg2(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
#if FAST_MATH
   dst->f[0] = util_fast_log2(src->f[0]);
   dst->f[1] = util_fast_log2(src->f[1]);
   dst->f[2] = util_fast_log2(src->f[2]);
   dst->f[3] = util_fast_log2(src->f[3]);
#else
   dst->f[0] = logf(src->f[0]) * 1.442695f;
   dst->f[1] = logf(src->f[1]) * 1.442695f;
   dst->f[2] = logf(src->f[2]) * 1.442695f;
   dst->f[3] = logf(src->f[3]) * 1.442695f;
#endif
}

static void
micro_lrp(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1,
          const union tgsi_exec_channel *src2)
{
   dst->f[0] = src0->f[0] * (src1->f[0] - src2->f[0]) + src2->f[0];
   dst->f[1] = src0->f[1] * (src1->f[1] - src2->f[1]) + src2->f[1];
   dst->f[2] = src0->f[2] * (src1->f[2] - src2->f[2]) + src2->f[2];
   dst->f[3] = src0->f[3] * (src1->f[3] - src2->f[3]) + src2->f[3];
}

static void
micro_mad(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1,
          const union tgsi_exec_channel *src2)
{
   dst->f[0] = src0->f[0] * src1->f[0] + src2->f[0];
   dst->f[1] = src0->f[1] * src1->f[1] + src2->f[1];
   dst->f[2] = src0->f[2] * src1->f[2] + src2->f[2];
   dst->f[3] = src0->f[3] * src1->f[3] + src2->f[3];
}

static void
micro_mov(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->u[0] = src->u[0];
   dst->u[1] = src->u[1];
   dst->u[2] = src->u[2];
   dst->u[3] = src->u[3];
}

static void
micro_rcp(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
#if 0 /* for debugging */
   assert(src->f[0] != 0.0f);
   assert(src->f[1] != 0.0f);
   assert(src->f[2] != 0.0f);
   assert(src->f[3] != 0.0f);
#endif
   dst->f[0] = 1.0f / src->f[0];
   dst->f[1] = 1.0f / src->f[1];
   dst->f[2] = 1.0f / src->f[2];
   dst->f[3] = 1.0f / src->f[3];
}

static void
micro_rnd(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->f[0] = floorf(src->f[0] + 0.5f);
   dst->f[1] = floorf(src->f[1] + 0.5f);
   dst->f[2] = floorf(src->f[2] + 0.5f);
   dst->f[3] = floorf(src->f[3] + 0.5f);
}

static void
micro_rsq(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
#if 0 /* for debugging */
   assert(src->f[0] != 0.0f);
   assert(src->f[1] != 0.0f);
   assert(src->f[2] != 0.0f);
   assert(src->f[3] != 0.0f);
#endif
   dst->f[0] = 1.0f / sqrtf(fabsf(src->f[0]));
   dst->f[1] = 1.0f / sqrtf(fabsf(src->f[1]));
   dst->f[2] = 1.0f / sqrtf(fabsf(src->f[2]));
   dst->f[3] = 1.0f / sqrtf(fabsf(src->f[3]));
}

static void
micro_seq(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->f[0] = src0->f[0] == src1->f[0] ? 1.0f : 0.0f;
   dst->f[1] = src0->f[1] == src1->f[1] ? 1.0f : 0.0f;
   dst->f[2] = src0->f[2] == src1->f[2] ? 1.0f : 0.0f;
   dst->f[3] = src0->f[3] == src1->f[3] ? 1.0f : 0.0f;
}

static void
micro_sge(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->f[0] = src0->f[0] >= src1->f[0] ? 1.0f : 0.0f;
   dst->f[1] = src0->f[1] >= src1->f[1] ? 1.0f : 0.0f;
   dst->f[2] = src0->f[2] >= src1->f[2] ? 1.0f : 0.0f;
   dst->f[3] = src0->f[3] >= src1->f[3] ? 1.0f : 0.0f;
}

static void
micro_sgn(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->f[0] = src->f[0] < 0.0f ? -1.0f : src->f[0] > 0.0f ? 1.0f : 0.0f;
   dst->f[1] = src->f[1] < 0.0f ? -1.0f : src->f[1] > 0.0f ? 1.0f : 0.0f;
   dst->f[2] = src->f[2] < 0.0f ? -1.0f : src->f[2] > 0.0f ? 1.0f : 0.0f;
   dst->f[3] = src->f[3] < 0.0f ? -1.0f : src->f[3] > 0.0f ? 1.0f : 0.0f;
}

static void
micro_isgn(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->i[0] = src->i[0] < 0 ? -1 : src->i[0] > 0 ? 1 : 0;
   dst->i[1] = src->i[1] < 0 ? -1 : src->i[1] > 0 ? 1 : 0;
   dst->i[2] = src->i[2] < 0 ? -1 : src->i[2] > 0 ? 1 : 0;
   dst->i[3] = src->i[3] < 0 ? -1 : src->i[3] > 0 ? 1 : 0;
}

static void
micro_sgt(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->f[0] = src0->f[0] > src1->f[0] ? 1.0f : 0.0f;
   dst->f[1] = src0->f[1] > src1->f[1] ? 1.0f : 0.0f;
   dst->f[2] = src0->f[2] > src1->f[2] ? 1.0f : 0.0f;
   dst->f[3] = src0->f[3] > src1->f[3] ? 1.0f : 0.0f;
}

static void
micro_sin(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->f[0] = sinf(src->f[0]);
   dst->f[1] = sinf(src->f[1]);
   dst->f[2] = sinf(src->f[2]);
   dst->f[3] = sinf(src->f[3]);
}

static void
micro_sle(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->f[0] = src0->f[0] <= src1->f[0] ? 1.0f : 0.0f;
   dst->f[1] = src0->f[1] <= src1->f[1] ? 1.0f : 0.0f;
   dst->f[2] = src0->f[2] <= src1->f[2] ? 1.0f : 0.0f;
   dst->f[3] = src0->f[3] <= src1->f[3] ? 1.0f : 0.0f;
}

static void
micro_slt(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->f[0] = src0->f[0] < src1->f[0] ? 1.0f : 0.0f;
   dst->f[1] = src0->f[1] < src1->f[1] ? 1.0f : 0.0f;
   dst->f[2] = src0->f[2] < src1->f[2] ? 1.0f : 0.0f;
   dst->f[3] = src0->f[3] < src1->f[3] ? 1.0f : 0.0f;
}

static void
micro_sne(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->f[0] = src0->f[0] != src1->f[0] ? 1.0f : 0.0f;
   dst->f[1] = src0->f[1] != src1->f[1] ? 1.0f : 0.0f;
   dst->f[2] = src0->f[2] != src1->f[2] ? 1.0f : 0.0f;
   dst->f[3] = src0->f[3] != src1->f[3] ? 1.0f : 0.0f;
}

static void
micro_sfl(union tgsi_exec_channel *dst)
{
   dst->f[0] = 0.0f;
   dst->f[1] = 0.0f;
   dst->f[2] = 0.0f;
   dst->f[3] = 0.0f;
}

static void
micro_str(union tgsi_exec_channel *dst)
{
   dst->f[0] = 1.0f;
   dst->f[1] = 1.0f;
   dst->f[2] = 1.0f;
   dst->f[3] = 1.0f;
}

static void
micro_trunc(union tgsi_exec_channel *dst,
            const union tgsi_exec_channel *src)
{
   dst->f[0] = (float)(int)src->f[0];
   dst->f[1] = (float)(int)src->f[1];
   dst->f[2] = (float)(int)src->f[2];
   dst->f[3] = (float)(int)src->f[3];
}


#define CHAN_X  0
#define CHAN_Y  1
#define CHAN_Z  2
#define CHAN_W  3

enum tgsi_exec_datatype {
   TGSI_EXEC_DATA_FLOAT,
   TGSI_EXEC_DATA_INT,
   TGSI_EXEC_DATA_UINT
};

/*
 * Shorthand locations of various utility registers (_I = Index, _C = Channel)
 */
#define TEMP_KILMASK_I     TGSI_EXEC_TEMP_KILMASK_I
#define TEMP_KILMASK_C     TGSI_EXEC_TEMP_KILMASK_C
#define TEMP_OUTPUT_I      TGSI_EXEC_TEMP_OUTPUT_I
#define TEMP_OUTPUT_C      TGSI_EXEC_TEMP_OUTPUT_C
#define TEMP_PRIMITIVE_I   TGSI_EXEC_TEMP_PRIMITIVE_I
#define TEMP_PRIMITIVE_C   TGSI_EXEC_TEMP_PRIMITIVE_C


/** The execution mask depends on the conditional mask and the loop mask */
#define UPDATE_EXEC_MASK(MACH) \
      MACH->ExecMask = MACH->CondMask & MACH->LoopMask & MACH->ContMask & MACH->Switch.mask & MACH->FuncMask


static const union tgsi_exec_channel ZeroVec =
   { { 0.0, 0.0, 0.0, 0.0 } };

static const union tgsi_exec_channel OneVec = {
   {1.0f, 1.0f, 1.0f, 1.0f}
};

static const union tgsi_exec_channel P128Vec = {
   {128.0f, 128.0f, 128.0f, 128.0f}
};

static const union tgsi_exec_channel M128Vec = {
   {-128.0f, -128.0f, -128.0f, -128.0f}
};


/**
 * Assert that none of the float values in 'chan' are infinite or NaN.
 * NaN and Inf may occur normally during program execution and should
 * not lead to crashes, etc.  But when debugging, it's helpful to catch
 * them.
 */
static INLINE void
check_inf_or_nan(const union tgsi_exec_channel *chan)
{
   assert(!util_is_inf_or_nan((chan)->f[0]));
   assert(!util_is_inf_or_nan((chan)->f[1]));
   assert(!util_is_inf_or_nan((chan)->f[2]));
   assert(!util_is_inf_or_nan((chan)->f[3]));
}


#ifdef DEBUG
static void
print_chan(const char *msg, const union tgsi_exec_channel *chan)
{
   debug_printf("%s = {%f, %f, %f, %f}\n",
                msg, chan->f[0], chan->f[1], chan->f[2], chan->f[3]);
}
#endif


#ifdef DEBUG
static void
print_temp(const struct tgsi_exec_machine *mach, uint index)
{
   const struct tgsi_exec_vector *tmp = &mach->Temps[index];
   int i;
   debug_printf("Temp[%u] =\n", index);
   for (i = 0; i < 4; i++) {
      debug_printf("  %c: { %f, %f, %f, %f }\n",
                   "XYZW"[i],
                   tmp->xyzw[i].f[0],
                   tmp->xyzw[i].f[1],
                   tmp->xyzw[i].f[2],
                   tmp->xyzw[i].f[3]);
   }
}
#endif


void
tgsi_exec_set_constant_buffers(struct tgsi_exec_machine *mach,
                               unsigned num_bufs,
                               const void **bufs,
                               const unsigned *buf_sizes)
{
   unsigned i;

   for (i = 0; i < num_bufs; i++) {
      mach->Consts[i] = bufs[i];
      mach->ConstsSize[i] = buf_sizes[i];
   }
}


/**
 * Check if there's a potential src/dst register data dependency when
 * using SOA execution.
 * Example:
 *   MOV T, T.yxwz;
 * This would expand into:
 *   MOV t0, t1;
 *   MOV t1, t0;
 *   MOV t2, t3;
 *   MOV t3, t2;
 * The second instruction will have the wrong value for t0 if executed as-is.
 */
boolean
tgsi_check_soa_dependencies(const struct tgsi_full_instruction *inst)
{
   uint i, chan;

   uint writemask = inst->Dst[0].Register.WriteMask;
   if (writemask == TGSI_WRITEMASK_X ||
       writemask == TGSI_WRITEMASK_Y ||
       writemask == TGSI_WRITEMASK_Z ||
       writemask == TGSI_WRITEMASK_W ||
       writemask == TGSI_WRITEMASK_NONE) {
      /* no chance of data dependency */
      return FALSE;
   }

   /* loop over src regs */
   for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
      if ((inst->Src[i].Register.File ==
           inst->Dst[0].Register.File) &&
          ((inst->Src[i].Register.Index ==
            inst->Dst[0].Register.Index) ||
           inst->Src[i].Register.Indirect ||
           inst->Dst[0].Register.Indirect)) {
         /* loop over dest channels */
         uint channelsWritten = 0x0;
         for (chan = 0; chan < NUM_CHANNELS; chan++) {
            if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
               /* check if we're reading a channel that's been written */
               uint swizzle = tgsi_util_get_full_src_register_swizzle(&inst->Src[i], chan);
               if (channelsWritten & (1 << swizzle)) {
                  return TRUE;
               }

               channelsWritten |= (1 << chan);
            }
         }
      }
   }
   return FALSE;
}


/**
 * Initialize machine state by expanding tokens to full instructions,
 * allocating temporary storage, setting up constants, etc.
 * After this, we can call tgsi_exec_machine_run() many times.
 */
void 
tgsi_exec_machine_bind_shader(
   struct tgsi_exec_machine *mach,
   const struct tgsi_token *tokens,
   uint numSamplers,
   struct tgsi_sampler **samplers)
{
   uint k;
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction *instructions;
   struct tgsi_full_declaration *declarations;
   uint maxInstructions = 10, numInstructions = 0;
   uint maxDeclarations = 10, numDeclarations = 0;

#if 0
   tgsi_dump(tokens, 0);
#endif

   util_init_math();

   if (numSamplers) {
      assert(samplers);
   }

   mach->Tokens = tokens;
   mach->Samplers = samplers;

   if (!tokens) {
      /* unbind and free all */
      if (mach->Declarations) {
         FREE( mach->Declarations );
      }
      mach->Declarations = NULL;
      mach->NumDeclarations = 0;

      if (mach->Instructions) {
         FREE( mach->Instructions );
      }
      mach->Instructions = NULL;
      mach->NumInstructions = 0;

      return;
   }

   k = tgsi_parse_init (&parse, mach->Tokens);
   if (k != TGSI_PARSE_OK) {
      debug_printf( "Problem parsing!\n" );
      return;
   }

   mach->Processor = parse.FullHeader.Processor.Processor;
   mach->ImmLimit = 0;

   if (mach->Processor == TGSI_PROCESSOR_GEOMETRY &&
       !mach->UsedGeometryShader) {
      struct tgsi_exec_vector *inputs;
      struct tgsi_exec_vector *outputs;

      inputs = align_malloc(sizeof(struct tgsi_exec_vector) *
                            TGSI_MAX_PRIM_VERTICES * PIPE_MAX_ATTRIBS,
                            16);

      if (!inputs)
         return;

      outputs = align_malloc(sizeof(struct tgsi_exec_vector) *
                             TGSI_MAX_TOTAL_VERTICES, 16);

      if (!outputs) {
         align_free(inputs);
         return;
      }

      align_free(mach->Inputs);
      align_free(mach->Outputs);

      mach->Inputs = inputs;
      mach->Outputs = outputs;
      mach->UsedGeometryShader = TRUE;
   }

   declarations = (struct tgsi_full_declaration *)
      MALLOC( maxDeclarations * sizeof(struct tgsi_full_declaration) );

   if (!declarations) {
      return;
   }

   instructions = (struct tgsi_full_instruction *)
      MALLOC( maxInstructions * sizeof(struct tgsi_full_instruction) );

   if (!instructions) {
      FREE( declarations );
      return;
   }

   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      uint i;

      tgsi_parse_token( &parse );
      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         /* save expanded declaration */
         if (numDeclarations == maxDeclarations) {
            declarations = REALLOC(declarations,
                                   maxDeclarations
                                   * sizeof(struct tgsi_full_declaration),
                                   (maxDeclarations + 10)
                                   * sizeof(struct tgsi_full_declaration));
            maxDeclarations += 10;
         }
         if (parse.FullToken.FullDeclaration.Declaration.File == TGSI_FILE_OUTPUT) {
            unsigned reg;
            for (reg = parse.FullToken.FullDeclaration.Range.First;
                 reg <= parse.FullToken.FullDeclaration.Range.Last;
                 ++reg) {
               ++mach->NumOutputs;
            }
         }
         if (parse.FullToken.FullDeclaration.Declaration.File ==
             TGSI_FILE_IMMEDIATE_ARRAY) {
            unsigned reg;
            struct tgsi_full_declaration *decl =
               &parse.FullToken.FullDeclaration;
            debug_assert(decl->Range.Last < TGSI_EXEC_NUM_IMMEDIATES);
            for (reg = decl->Range.First; reg <= decl->Range.Last; ++reg) {
               for( i = 0; i < 4; i++ ) {
                  int idx = reg * 4 + i;
                  mach->ImmArray[reg][i] = decl->ImmediateData.u[idx].Float;
               }
            }
         }
         memcpy(declarations + numDeclarations,
                &parse.FullToken.FullDeclaration,
                sizeof(declarations[0]));
         numDeclarations++;
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         {
            uint size = parse.FullToken.FullImmediate.Immediate.NrTokens - 1;
            assert( size <= 4 );
            assert( mach->ImmLimit + 1 <= TGSI_EXEC_NUM_IMMEDIATES );

            for( i = 0; i < size; i++ ) {
               mach->Imms[mach->ImmLimit][i] = 
		  parse.FullToken.FullImmediate.u[i].Float;
            }
            mach->ImmLimit += 1;
         }
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:

         /* save expanded instruction */
         if (numInstructions == maxInstructions) {
            instructions = REALLOC(instructions,
                                   maxInstructions
                                   * sizeof(struct tgsi_full_instruction),
                                   (maxInstructions + 10)
                                   * sizeof(struct tgsi_full_instruction));
            maxInstructions += 10;
         }

         memcpy(instructions + numInstructions,
                &parse.FullToken.FullInstruction,
                sizeof(instructions[0]));

         numInstructions++;
         break;

      case TGSI_TOKEN_TYPE_PROPERTY:
         break;

      default:
         assert( 0 );
      }
   }
   tgsi_parse_free (&parse);

   if (mach->Declarations) {
      FREE( mach->Declarations );
   }
   mach->Declarations = declarations;
   mach->NumDeclarations = numDeclarations;

   if (mach->Instructions) {
      FREE( mach->Instructions );
   }
   mach->Instructions = instructions;
   mach->NumInstructions = numInstructions;
}


struct tgsi_exec_machine *
tgsi_exec_machine_create( void )
{
   struct tgsi_exec_machine *mach;
   uint i;

   mach = align_malloc( sizeof *mach, 16 );
   if (!mach)
      goto fail;

   memset(mach, 0, sizeof(*mach));

   mach->Addrs = &mach->Temps[TGSI_EXEC_TEMP_ADDR];
   mach->MaxGeometryShaderOutputs = TGSI_MAX_TOTAL_VERTICES;
   mach->Predicates = &mach->Temps[TGSI_EXEC_TEMP_P0];

   mach->Inputs = align_malloc(sizeof(struct tgsi_exec_vector) * PIPE_MAX_ATTRIBS, 16);
   mach->Outputs = align_malloc(sizeof(struct tgsi_exec_vector) * PIPE_MAX_ATTRIBS, 16);
   if (!mach->Inputs || !mach->Outputs)
      goto fail;

   /* Setup constants needed by the SSE2 executor. */
   for( i = 0; i < 4; i++ ) {
      mach->Temps[TGSI_EXEC_TEMP_00000000_I].xyzw[TGSI_EXEC_TEMP_00000000_C].u[i] = 0x00000000;
      mach->Temps[TGSI_EXEC_TEMP_7FFFFFFF_I].xyzw[TGSI_EXEC_TEMP_7FFFFFFF_C].u[i] = 0x7FFFFFFF;
      mach->Temps[TGSI_EXEC_TEMP_80000000_I].xyzw[TGSI_EXEC_TEMP_80000000_C].u[i] = 0x80000000;
      mach->Temps[TGSI_EXEC_TEMP_FFFFFFFF_I].xyzw[TGSI_EXEC_TEMP_FFFFFFFF_C].u[i] = 0xFFFFFFFF;    /* not used */
      mach->Temps[TGSI_EXEC_TEMP_ONE_I].xyzw[TGSI_EXEC_TEMP_ONE_C].f[i] = 1.0f;
      mach->Temps[TGSI_EXEC_TEMP_TWO_I].xyzw[TGSI_EXEC_TEMP_TWO_C].f[i] = 2.0f;    /* not used */
      mach->Temps[TGSI_EXEC_TEMP_128_I].xyzw[TGSI_EXEC_TEMP_128_C].f[i] = 128.0f;
      mach->Temps[TGSI_EXEC_TEMP_MINUS_128_I].xyzw[TGSI_EXEC_TEMP_MINUS_128_C].f[i] = -128.0f;
      mach->Temps[TGSI_EXEC_TEMP_THREE_I].xyzw[TGSI_EXEC_TEMP_THREE_C].f[i] = 3.0f;
      mach->Temps[TGSI_EXEC_TEMP_HALF_I].xyzw[TGSI_EXEC_TEMP_HALF_C].f[i] = 0.5f;
   }

#ifdef DEBUG
   /* silence warnings */
   (void) print_chan;
   (void) print_temp;
#endif

   return mach;

fail:
   if (mach) {
      align_free(mach->Inputs);
      align_free(mach->Outputs);
      align_free(mach);
   }
   return NULL;
}


void
tgsi_exec_machine_destroy(struct tgsi_exec_machine *mach)
{
   if (mach) {
      if (mach->Instructions)
         FREE(mach->Instructions);
      if (mach->Declarations)
         FREE(mach->Declarations);

      align_free(mach->Inputs);
      align_free(mach->Outputs);

      align_free(mach);
   }
}

static void
micro_add(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->f[0] = src0->f[0] + src1->f[0];
   dst->f[1] = src0->f[1] + src1->f[1];
   dst->f[2] = src0->f[2] + src1->f[2];
   dst->f[3] = src0->f[3] + src1->f[3];
}

static void
micro_div(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   if (src1->f[0] != 0) {
      dst->f[0] = src0->f[0] / src1->f[0];
   }
   if (src1->f[1] != 0) {
      dst->f[1] = src0->f[1] / src1->f[1];
   }
   if (src1->f[2] != 0) {
      dst->f[2] = src0->f[2] / src1->f[2];
   }
   if (src1->f[3] != 0) {
      dst->f[3] = src0->f[3] / src1->f[3];
   }
}

static void
micro_rcc(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   uint i;

   for (i = 0; i < 4; i++) {
      float recip = 1.0f / src->f[i];

      if (recip > 0.0f) {
         if (recip > 1.884467e+019f) {
            dst->f[i] = 1.884467e+019f;
         }
         else if (recip < 5.42101e-020f) {
            dst->f[i] = 5.42101e-020f;
         }
         else {
            dst->f[i] = recip;
         }
      }
      else {
         if (recip < -1.884467e+019f) {
            dst->f[i] = -1.884467e+019f;
         }
         else if (recip > -5.42101e-020f) {
            dst->f[i] = -5.42101e-020f;
         }
         else {
            dst->f[i] = recip;
         }
      }
   }
}

static void
micro_lt(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1,
   const union tgsi_exec_channel *src2,
   const union tgsi_exec_channel *src3 )
{
   dst->f[0] = src0->f[0] < src1->f[0] ? src2->f[0] : src3->f[0];
   dst->f[1] = src0->f[1] < src1->f[1] ? src2->f[1] : src3->f[1];
   dst->f[2] = src0->f[2] < src1->f[2] ? src2->f[2] : src3->f[2];
   dst->f[3] = src0->f[3] < src1->f[3] ? src2->f[3] : src3->f[3];
}

static void
micro_max(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->f[0] = src0->f[0] > src1->f[0] ? src0->f[0] : src1->f[0];
   dst->f[1] = src0->f[1] > src1->f[1] ? src0->f[1] : src1->f[1];
   dst->f[2] = src0->f[2] > src1->f[2] ? src0->f[2] : src1->f[2];
   dst->f[3] = src0->f[3] > src1->f[3] ? src0->f[3] : src1->f[3];
}

static void
micro_min(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->f[0] = src0->f[0] < src1->f[0] ? src0->f[0] : src1->f[0];
   dst->f[1] = src0->f[1] < src1->f[1] ? src0->f[1] : src1->f[1];
   dst->f[2] = src0->f[2] < src1->f[2] ? src0->f[2] : src1->f[2];
   dst->f[3] = src0->f[3] < src1->f[3] ? src0->f[3] : src1->f[3];
}

static void
micro_mul(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->f[0] = src0->f[0] * src1->f[0];
   dst->f[1] = src0->f[1] * src1->f[1];
   dst->f[2] = src0->f[2] * src1->f[2];
   dst->f[3] = src0->f[3] * src1->f[3];
}

static void
micro_neg(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = -src->f[0];
   dst->f[1] = -src->f[1];
   dst->f[2] = -src->f[2];
   dst->f[3] = -src->f[3];
}

static void
micro_pow(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
#if FAST_MATH
   dst->f[0] = util_fast_pow( src0->f[0], src1->f[0] );
   dst->f[1] = util_fast_pow( src0->f[1], src1->f[1] );
   dst->f[2] = util_fast_pow( src0->f[2], src1->f[2] );
   dst->f[3] = util_fast_pow( src0->f[3], src1->f[3] );
#else
   dst->f[0] = powf( src0->f[0], src1->f[0] );
   dst->f[1] = powf( src0->f[1], src1->f[1] );
   dst->f[2] = powf( src0->f[2], src1->f[2] );
   dst->f[3] = powf( src0->f[3], src1->f[3] );
#endif
}

static void
micro_sub(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->f[0] = src0->f[0] - src1->f[0];
   dst->f[1] = src0->f[1] - src1->f[1];
   dst->f[2] = src0->f[2] - src1->f[2];
   dst->f[3] = src0->f[3] - src1->f[3];
}

static void
fetch_src_file_channel(const struct tgsi_exec_machine *mach,
                       const uint chan_index,
                       const uint file,
                       const uint swizzle,
                       const union tgsi_exec_channel *index,
                       const union tgsi_exec_channel *index2D,
                       union tgsi_exec_channel *chan)
{
   uint i;

   assert(swizzle < 4);

   switch (file) {
   case TGSI_FILE_CONSTANT:
      for (i = 0; i < QUAD_SIZE; i++) {
         assert(index2D->i[i] >= 0 && index2D->i[i] < PIPE_MAX_CONSTANT_BUFFERS);
         assert(mach->Consts[index2D->i[i]]);

         if (index->i[i] < 0) {
            chan->u[i] = 0;
         } else {
            /* NOTE: copying the const value as a uint instead of float */
            const uint constbuf = index2D->i[i];
            const uint *buf = (const uint *)mach->Consts[constbuf];
            const int pos = index->i[i] * 4 + swizzle;
            /* const buffer bounds check */
            if (pos < 0 || pos >= mach->ConstsSize[constbuf]) {
               if (0) {
                  /* Debug: print warning */
                  static int count = 0;
                  if (count++ < 100)
                     debug_printf("TGSI Exec: const buffer index %d"
                                  " out of bounds\n", pos);
               }
               chan->u[i] = 0;
            }
            else
               chan->u[i] = buf[pos];
         }
      }
      break;

   case TGSI_FILE_INPUT:
      for (i = 0; i < QUAD_SIZE; i++) {
         /*
         if (TGSI_PROCESSOR_GEOMETRY == mach->Processor) {
            debug_printf("Fetching Input[%d] (2d=%d, 1d=%d)\n",
                         index2D->i[i] * TGSI_EXEC_MAX_INPUT_ATTRIBS + index->i[i],
                         index2D->i[i], index->i[i]);
                         }*/
         int pos = index2D->i[i] * TGSI_EXEC_MAX_INPUT_ATTRIBS + index->i[i];
         assert(pos >= 0);
         assert(pos < TGSI_MAX_PRIM_VERTICES * PIPE_MAX_ATTRIBS);
         chan->u[i] = mach->Inputs[pos].xyzw[swizzle].u[i];
      }
      break;

   case TGSI_FILE_SYSTEM_VALUE:
      /* XXX no swizzling at this point.  Will be needed if we put
       * gl_FragCoord, for example, in a sys value register.
       */
      for (i = 0; i < QUAD_SIZE; i++) {
         chan->u[i] = mach->SystemValue[index->i[i]].u[i];
      }
      break;

   case TGSI_FILE_TEMPORARY:
      for (i = 0; i < QUAD_SIZE; i++) {
         assert(index->i[i] < TGSI_EXEC_NUM_TEMPS);
         assert(index2D->i[i] == 0);

         chan->u[i] = mach->Temps[index->i[i]].xyzw[swizzle].u[i];
      }
      break;

   case TGSI_FILE_TEMPORARY_ARRAY:
      for (i = 0; i < QUAD_SIZE; i++) {
         assert(index->i[i] < TGSI_EXEC_NUM_TEMPS);
         assert(index2D->i[i] < TGSI_EXEC_NUM_TEMP_ARRAYS);

         chan->u[i] =
            mach->TempArray[index2D->i[i]][index->i[i]].xyzw[swizzle].u[i];
      }
      break;

   case TGSI_FILE_IMMEDIATE:
      for (i = 0; i < QUAD_SIZE; i++) {
         assert(index->i[i] >= 0 && index->i[i] < (int)mach->ImmLimit);
         assert(index2D->i[i] == 0);

         chan->f[i] = mach->Imms[index->i[i]][swizzle];
      }
      break;

   case TGSI_FILE_IMMEDIATE_ARRAY:
      for (i = 0; i < QUAD_SIZE; i++) {
         assert(index2D->i[i] == 0);

         chan->f[i] = mach->ImmArray[index->i[i]][swizzle];
      }
      break;

   case TGSI_FILE_ADDRESS:
      for (i = 0; i < QUAD_SIZE; i++) {
         assert(index->i[i] >= 0);
         assert(index2D->i[i] == 0);

         chan->u[i] = mach->Addrs[index->i[i]].xyzw[swizzle].u[i];
      }
      break;

   case TGSI_FILE_PREDICATE:
      for (i = 0; i < QUAD_SIZE; i++) {
         assert(index->i[i] >= 0 && index->i[i] < TGSI_EXEC_NUM_PREDS);
         assert(index2D->i[i] == 0);

         chan->u[i] = mach->Predicates[0].xyzw[swizzle].u[i];
      }
      break;

   case TGSI_FILE_OUTPUT:
      /* vertex/fragment output vars can be read too */
      for (i = 0; i < QUAD_SIZE; i++) {
         assert(index->i[i] >= 0);
         assert(index2D->i[i] == 0);

         chan->u[i] = mach->Outputs[index->i[i]].xyzw[swizzle].u[i];
      }
      break;

   default:
      assert(0);
      for (i = 0; i < QUAD_SIZE; i++) {
         chan->u[i] = 0;
      }
   }
}

static void
fetch_source(const struct tgsi_exec_machine *mach,
             union tgsi_exec_channel *chan,
             const struct tgsi_full_src_register *reg,
             const uint chan_index,
             enum tgsi_exec_datatype src_datatype)
{
   union tgsi_exec_channel index;
   union tgsi_exec_channel index2D;
   uint swizzle;

   /* We start with a direct index into a register file.
    *
    *    file[1],
    *    where:
    *       file = Register.File
    *       [1] = Register.Index
    */
   index.i[0] =
   index.i[1] =
   index.i[2] =
   index.i[3] = reg->Register.Index;

   /* There is an extra source register that indirectly subscripts
    * a register file. The direct index now becomes an offset
    * that is being added to the indirect register.
    *
    *    file[ind[2].x+1],
    *    where:
    *       ind = Indirect.File
    *       [2] = Indirect.Index
    *       .x = Indirect.SwizzleX
    */
   if (reg->Register.Indirect) {
      union tgsi_exec_channel index2;
      union tgsi_exec_channel indir_index;
      const uint execmask = mach->ExecMask;
      uint i;

      /* which address register (always zero now) */
      index2.i[0] =
      index2.i[1] =
      index2.i[2] =
      index2.i[3] = reg->Indirect.Index;
      assert(reg->Indirect.File == TGSI_FILE_ADDRESS);
      /* get current value of address register[swizzle] */
      swizzle = tgsi_util_get_src_register_swizzle( &reg->Indirect, CHAN_X );
      fetch_src_file_channel(mach,
                             chan_index,
                             reg->Indirect.File,
                             swizzle,
                             &index2,
                             &ZeroVec,
                             &indir_index);

      /* add value of address register to the offset */
      index.i[0] += indir_index.i[0];
      index.i[1] += indir_index.i[1];
      index.i[2] += indir_index.i[2];
      index.i[3] += indir_index.i[3];

      /* for disabled execution channels, zero-out the index to
       * avoid using a potential garbage value.
       */
      for (i = 0; i < QUAD_SIZE; i++) {
         if ((execmask & (1 << i)) == 0)
            index.i[i] = 0;
      }
   }

   /* There is an extra source register that is a second
    * subscript to a register file. Effectively it means that
    * the register file is actually a 2D array of registers.
    *
    *    file[3][1],
    *    where:
    *       [3] = Dimension.Index
    */
   if (reg->Register.Dimension) {
      index2D.i[0] =
      index2D.i[1] =
      index2D.i[2] =
      index2D.i[3] = reg->Dimension.Index;

      /* Again, the second subscript index can be addressed indirectly
       * identically to the first one.
       * Nothing stops us from indirectly addressing the indirect register,
       * but there is no need for that, so we won't exercise it.
       *
       *    file[ind[4].y+3][1],
       *    where:
       *       ind = DimIndirect.File
       *       [4] = DimIndirect.Index
       *       .y = DimIndirect.SwizzleX
       */
      if (reg->Dimension.Indirect) {
         union tgsi_exec_channel index2;
         union tgsi_exec_channel indir_index;
         const uint execmask = mach->ExecMask;
         uint i;

         index2.i[0] =
         index2.i[1] =
         index2.i[2] =
         index2.i[3] = reg->DimIndirect.Index;

         swizzle = tgsi_util_get_src_register_swizzle( &reg->DimIndirect, CHAN_X );
         fetch_src_file_channel(mach,
                                chan_index,
                                reg->DimIndirect.File,
                                swizzle,
                                &index2,
                                &ZeroVec,
                                &indir_index);

         index2D.i[0] += indir_index.i[0];
         index2D.i[1] += indir_index.i[1];
         index2D.i[2] += indir_index.i[2];
         index2D.i[3] += indir_index.i[3];

         /* for disabled execution channels, zero-out the index to
          * avoid using a potential garbage value.
          */
         for (i = 0; i < QUAD_SIZE; i++) {
            if ((execmask & (1 << i)) == 0) {
               index2D.i[i] = 0;
            }
         }
      }

      /* If by any chance there was a need for a 3D array of register
       * files, we would have to check whether Dimension is followed
       * by a dimension register and continue the saga.
       */
   } else {
      index2D.i[0] =
      index2D.i[1] =
      index2D.i[2] =
      index2D.i[3] = 0;
   }

   swizzle = tgsi_util_get_full_src_register_swizzle( reg, chan_index );
   fetch_src_file_channel(mach,
                          chan_index,
                          reg->Register.File,
                          swizzle,
                          &index,
                          &index2D,
                          chan);

   if (reg->Register.Absolute) {
      if (src_datatype == TGSI_EXEC_DATA_FLOAT) {
         micro_abs(chan, chan);
      } else {
         micro_iabs(chan, chan);
      }
   }

   if (reg->Register.Negate) {
      if (src_datatype == TGSI_EXEC_DATA_FLOAT) {
         micro_neg(chan, chan);
      } else {
         micro_ineg(chan, chan);
      }
   }
}

static void
store_dest(struct tgsi_exec_machine *mach,
           const union tgsi_exec_channel *chan,
           const struct tgsi_full_dst_register *reg,
           const struct tgsi_full_instruction *inst,
           uint chan_index,
           enum tgsi_exec_datatype dst_datatype)
{
   uint i;
   union tgsi_exec_channel null;
   union tgsi_exec_channel *dst;
   union tgsi_exec_channel index2D;
   uint execmask = mach->ExecMask;
   int offset = 0;  /* indirection offset */
   int index;

   /* for debugging */
   if (0 && dst_datatype == TGSI_EXEC_DATA_FLOAT) {
      check_inf_or_nan(chan);
   }

   /* There is an extra source register that indirectly subscripts
    * a register file. The direct index now becomes an offset
    * that is being added to the indirect register.
    *
    *    file[ind[2].x+1],
    *    where:
    *       ind = Indirect.File
    *       [2] = Indirect.Index
    *       .x = Indirect.SwizzleX
    */
   if (reg->Register.Indirect) {
      union tgsi_exec_channel index;
      union tgsi_exec_channel indir_index;
      uint swizzle;

      /* which address register (always zero for now) */
      index.i[0] =
      index.i[1] =
      index.i[2] =
      index.i[3] = reg->Indirect.Index;

      /* get current value of address register[swizzle] */
      swizzle = tgsi_util_get_src_register_swizzle( &reg->Indirect, CHAN_X );

      /* fetch values from the address/indirection register */
      fetch_src_file_channel(mach,
                             chan_index,
                             reg->Indirect.File,
                             swizzle,
                             &index,
                             &ZeroVec,
                             &indir_index);

      /* save indirection offset */
      offset = indir_index.i[0];
   }

   /* There is an extra source register that is a second
    * subscript to a register file. Effectively it means that
    * the register file is actually a 2D array of registers.
    *
    *    file[3][1],
    *    where:
    *       [3] = Dimension.Index
    */
   if (reg->Register.Dimension) {
      index2D.i[0] =
      index2D.i[1] =
      index2D.i[2] =
      index2D.i[3] = reg->Dimension.Index;

      /* Again, the second subscript index can be addressed indirectly
       * identically to the first one.
       * Nothing stops us from indirectly addressing the indirect register,
       * but there is no need for that, so we won't exercise it.
       *
       *    file[ind[4].y+3][1],
       *    where:
       *       ind = DimIndirect.File
       *       [4] = DimIndirect.Index
       *       .y = DimIndirect.SwizzleX
       */
      if (reg->Dimension.Indirect) {
         union tgsi_exec_channel index2;
         union tgsi_exec_channel indir_index;
         const uint execmask = mach->ExecMask;
         unsigned swizzle;
         uint i;

         index2.i[0] =
         index2.i[1] =
         index2.i[2] =
         index2.i[3] = reg->DimIndirect.Index;

         swizzle = tgsi_util_get_src_register_swizzle( &reg->DimIndirect, CHAN_X );
         fetch_src_file_channel(mach,
                                chan_index,
                                reg->DimIndirect.File,
                                swizzle,
                                &index2,
                                &ZeroVec,
                                &indir_index);

         index2D.i[0] += indir_index.i[0];
         index2D.i[1] += indir_index.i[1];
         index2D.i[2] += indir_index.i[2];
         index2D.i[3] += indir_index.i[3];

         /* for disabled execution channels, zero-out the index to
          * avoid using a potential garbage value.
          */
         for (i = 0; i < QUAD_SIZE; i++) {
            if ((execmask & (1 << i)) == 0) {
               index2D.i[i] = 0;
            }
         }
      }

      /* If by any chance there was a need for a 3D array of register
       * files, we would have to check whether Dimension is followed
       * by a dimension register and continue the saga.
       */
   } else {
      index2D.i[0] =
      index2D.i[1] =
      index2D.i[2] =
      index2D.i[3] = 0;
   }

   switch (reg->Register.File) {
   case TGSI_FILE_NULL:
      dst = &null;
      break;

   case TGSI_FILE_OUTPUT:
      index = mach->Temps[TEMP_OUTPUT_I].xyzw[TEMP_OUTPUT_C].u[0]
         + reg->Register.Index;
      dst = &mach->Outputs[offset + index].xyzw[chan_index];
#if 0
      if (TGSI_PROCESSOR_GEOMETRY == mach->Processor) {
         fprintf(stderr, "STORING OUT[%d] mask(%d), = (", offset + index, execmask);
         for (i = 0; i < QUAD_SIZE; i++)
            if (execmask & (1 << i))
               fprintf(stderr, "%f, ", chan->f[i]);
         fprintf(stderr, ")\n");
      }
#endif
      break;

   case TGSI_FILE_TEMPORARY:
      index = reg->Register.Index;
      assert( index < TGSI_EXEC_NUM_TEMPS );
      dst = &mach->Temps[offset + index].xyzw[chan_index];
      break;

   case TGSI_FILE_TEMPORARY_ARRAY:
      index = reg->Register.Index;
      assert( index < TGSI_EXEC_NUM_TEMPS );
      assert( index2D.i[0] < TGSI_EXEC_NUM_TEMP_ARRAYS );
      /* XXX we use index2D.i[0] here but somehow we might
       * end up with someone trying to store indirectly in
       * different buffers */
      dst = &mach->TempArray[index2D.i[0]][offset + index].xyzw[chan_index];
      break;

   case TGSI_FILE_ADDRESS:
      index = reg->Register.Index;
      dst = &mach->Addrs[index].xyzw[chan_index];
      break;

   case TGSI_FILE_PREDICATE:
      index = reg->Register.Index;
      assert(index < TGSI_EXEC_NUM_PREDS);
      dst = &mach->Predicates[index].xyzw[chan_index];
      break;

   default:
      assert( 0 );
      return;
   }

   if (inst->Instruction.Predicate) {
      uint swizzle;
      union tgsi_exec_channel *pred;

      switch (chan_index) {
      case CHAN_X:
         swizzle = inst->Predicate.SwizzleX;
         break;
      case CHAN_Y:
         swizzle = inst->Predicate.SwizzleY;
         break;
      case CHAN_Z:
         swizzle = inst->Predicate.SwizzleZ;
         break;
      case CHAN_W:
         swizzle = inst->Predicate.SwizzleW;
         break;
      default:
         assert(0);
         return;
      }

      assert(inst->Predicate.Index == 0);

      pred = &mach->Predicates[inst->Predicate.Index].xyzw[swizzle];

      if (inst->Predicate.Negate) {
         for (i = 0; i < QUAD_SIZE; i++) {
            if (pred->u[i]) {
               execmask &= ~(1 << i);
            }
         }
      } else {
         for (i = 0; i < QUAD_SIZE; i++) {
            if (!pred->u[i]) {
               execmask &= ~(1 << i);
            }
         }
      }
   }

   switch (inst->Instruction.Saturate) {
   case TGSI_SAT_NONE:
      for (i = 0; i < QUAD_SIZE; i++)
         if (execmask & (1 << i))
            dst->i[i] = chan->i[i];
      break;

   case TGSI_SAT_ZERO_ONE:
      for (i = 0; i < QUAD_SIZE; i++)
         if (execmask & (1 << i)) {
            if (chan->f[i] < 0.0f)
               dst->f[i] = 0.0f;
            else if (chan->f[i] > 1.0f)
               dst->f[i] = 1.0f;
            else
               dst->i[i] = chan->i[i];
         }
      break;

   case TGSI_SAT_MINUS_PLUS_ONE:
      for (i = 0; i < QUAD_SIZE; i++)
         if (execmask & (1 << i)) {
            if (chan->f[i] < -1.0f)
               dst->f[i] = -1.0f;
            else if (chan->f[i] > 1.0f)
               dst->f[i] = 1.0f;
            else
               dst->i[i] = chan->i[i];
         }
      break;

   default:
      assert( 0 );
   }
}

#define FETCH(VAL,INDEX,CHAN)\
    fetch_source(mach, VAL, &inst->Src[INDEX], CHAN, TGSI_EXEC_DATA_FLOAT)

#define IFETCH(VAL,INDEX,CHAN)\
    fetch_source(mach, VAL, &inst->Src[INDEX], CHAN, TGSI_EXEC_DATA_INT)


/**
 * Execute ARB-style KIL which is predicated by a src register.
 * Kill fragment if any of the four values is less than zero.
 */
static void
exec_kil(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   uint uniquemask;
   uint chan_index;
   uint kilmask = 0; /* bit 0 = pixel 0, bit 1 = pixel 1, etc */
   union tgsi_exec_channel r[1];

   /* This mask stores component bits that were already tested. */
   uniquemask = 0;

   for (chan_index = 0; chan_index < 4; chan_index++)
   {
      uint swizzle;
      uint i;

      /* unswizzle channel */
      swizzle = tgsi_util_get_full_src_register_swizzle (
                        &inst->Src[0],
                        chan_index);

      /* check if the component has not been already tested */
      if (uniquemask & (1 << swizzle))
         continue;
      uniquemask |= 1 << swizzle;

      FETCH(&r[0], 0, chan_index);
      for (i = 0; i < 4; i++)
         if (r[0].f[i] < 0.0f)
            kilmask |= 1 << i;
   }

   mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0] |= kilmask;
}

/**
 * Execute NVIDIA-style KIL which is predicated by a condition code.
 * Kill fragment if the condition code is TRUE.
 */
static void
exec_kilp(struct tgsi_exec_machine *mach,
          const struct tgsi_full_instruction *inst)
{
   uint kilmask; /* bit 0 = pixel 0, bit 1 = pixel 1, etc */

   /* "unconditional" kil */
   kilmask = mach->ExecMask;
   mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0] |= kilmask;
}

static void
emit_vertex(struct tgsi_exec_machine *mach)
{
   /* FIXME: check for exec mask correctly
   unsigned i;
   for (i = 0; i < QUAD_SIZE; ++i) {
         if ((mach->ExecMask & (1 << i)))
   */
   if (mach->ExecMask) {
      mach->Temps[TEMP_OUTPUT_I].xyzw[TEMP_OUTPUT_C].u[0] += mach->NumOutputs;
      mach->Primitives[mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0]]++;
   }
}

static void
emit_primitive(struct tgsi_exec_machine *mach)
{
   unsigned *prim_count = &mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0];
   /* FIXME: check for exec mask correctly
   unsigned i;
   for (i = 0; i < QUAD_SIZE; ++i) {
         if ((mach->ExecMask & (1 << i)))
   */
   if (mach->ExecMask) {
      ++(*prim_count);
      debug_assert((*prim_count * mach->NumOutputs) < mach->MaxGeometryShaderOutputs);
      mach->Primitives[*prim_count] = 0;
   }
}

static void
conditional_emit_primitive(struct tgsi_exec_machine *mach)
{
   if (TGSI_PROCESSOR_GEOMETRY == mach->Processor) {
      int emitted_verts =
         mach->Primitives[mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0]];
      if (emitted_verts) {
         emit_primitive(mach);
      }
   }
}


/*
 * Fetch four texture samples using STR texture coordinates.
 */
static void
fetch_texel( struct tgsi_sampler *sampler,
             const union tgsi_exec_channel *s,
             const union tgsi_exec_channel *t,
             const union tgsi_exec_channel *p,
             const union tgsi_exec_channel *c0,
             enum tgsi_sampler_control control,
             union tgsi_exec_channel *r,
             union tgsi_exec_channel *g,
             union tgsi_exec_channel *b,
             union tgsi_exec_channel *a )
{
   uint j;
   float rgba[NUM_CHANNELS][QUAD_SIZE];

   sampler->get_samples(sampler, s->f, t->f, p->f, c0->f, control, rgba);

   for (j = 0; j < 4; j++) {
      r->f[j] = rgba[0][j];
      g->f[j] = rgba[1][j];
      b->f[j] = rgba[2][j];
      a->f[j] = rgba[3][j];
   }
}


#define TEX_MODIFIER_NONE           0
#define TEX_MODIFIER_PROJECTED      1
#define TEX_MODIFIER_LOD_BIAS       2
#define TEX_MODIFIER_EXPLICIT_LOD   3


static void
exec_tex(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst,
         uint modifier)
{
   const uint unit = inst->Src[1].Register.Index;
   union tgsi_exec_channel r[4];
   const union tgsi_exec_channel *lod = &ZeroVec;
   enum tgsi_sampler_control control;
   uint chan;

   if (modifier != TEX_MODIFIER_NONE) {
      FETCH(&r[3], 0, CHAN_W);
      if (modifier != TEX_MODIFIER_PROJECTED) {
         lod = &r[3];
      }
   }

   if (modifier == TEX_MODIFIER_EXPLICIT_LOD) {
      control = tgsi_sampler_lod_explicit;
   } else {
      control = tgsi_sampler_lod_bias;
   }

   switch (inst->Texture.Texture) {
   case TGSI_TEXTURE_1D:
      FETCH(&r[0], 0, CHAN_X);

      if (modifier == TEX_MODIFIER_PROJECTED) {
         micro_div(&r[0], &r[0], &r[3]);
      }

      fetch_texel(mach->Samplers[unit],
                  &r[0], &ZeroVec, &ZeroVec, lod,  /* S, T, P, LOD */
                  control,
                  &r[0], &r[1], &r[2], &r[3]);     /* R, G, B, A */
      break;
   case TGSI_TEXTURE_SHADOW1D:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[2], 0, CHAN_Z);

      if (modifier == TEX_MODIFIER_PROJECTED) {
         micro_div(&r[0], &r[0], &r[3]);
      }

      fetch_texel(mach->Samplers[unit],
                  &r[0], &ZeroVec, &r[2], lod,  /* S, T, P, LOD */
                  control,
                  &r[0], &r[1], &r[2], &r[3]);     /* R, G, B, A */
      break;

   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      if (modifier == TEX_MODIFIER_PROJECTED) {
         micro_div(&r[0], &r[0], &r[3]);
         micro_div(&r[1], &r[1], &r[3]);
         micro_div(&r[2], &r[2], &r[3]);
      }

      fetch_texel(mach->Samplers[unit],
                  &r[0], &r[1], &r[2], lod,     /* S, T, P, LOD */
                  control,
                  &r[0], &r[1], &r[2], &r[3]);  /* outputs */
      break;

   case TGSI_TEXTURE_1D_ARRAY:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);

      if (modifier == TEX_MODIFIER_PROJECTED) {
         micro_div(&r[0], &r[0], &r[3]);
      }

      fetch_texel(mach->Samplers[unit],
                  &r[0], &r[1], &ZeroVec, lod,     /* S, T, P, LOD */
                  control,
                  &r[0], &r[1], &r[2], &r[3]);  /* outputs */
      break;
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      if (modifier == TEX_MODIFIER_PROJECTED) {
         micro_div(&r[0], &r[0], &r[3]);
      }

      fetch_texel(mach->Samplers[unit],
                  &r[0], &r[1], &r[2], lod,     /* S, T, P, LOD */
                  control,
                  &r[0], &r[1], &r[2], &r[3]);  /* outputs */
      break;

   case TGSI_TEXTURE_2D_ARRAY:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      if (modifier == TEX_MODIFIER_PROJECTED) {
         micro_div(&r[0], &r[0], &r[3]);
         micro_div(&r[1], &r[1], &r[3]);
      }

      fetch_texel(mach->Samplers[unit],
                  &r[0], &r[1], &r[2], lod,     /* S, T, P, LOD */
                  control,
                  &r[0], &r[1], &r[2], &r[3]);  /* outputs */
      break;
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);
      FETCH(&r[3], 0, CHAN_W);

      fetch_texel(mach->Samplers[unit],
                  &r[0], &r[1], &r[2], &r[3],     /* S, T, P, LOD */
                  control,
                  &r[0], &r[1], &r[2], &r[3]);  /* outputs */
      break;
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      if (modifier == TEX_MODIFIER_PROJECTED) {
         micro_div(&r[0], &r[0], &r[3]);
         micro_div(&r[1], &r[1], &r[3]);
         micro_div(&r[2], &r[2], &r[3]);
      }

      fetch_texel(mach->Samplers[unit],
                  &r[0], &r[1], &r[2], lod,
                  control,
                  &r[0], &r[1], &r[2], &r[3]);
      break;

   default:
      assert(0);
   }

#if 0
   debug_printf("fetch r: %g %g %g %g\n",
         r[0].f[0], r[0].f[1], r[0].f[2], r[0].f[3]);
   debug_printf("fetch g: %g %g %g %g\n",
         r[1].f[0], r[1].f[1], r[1].f[2], r[1].f[3]);
   debug_printf("fetch b: %g %g %g %g\n",
         r[2].f[0], r[2].f[1], r[2].f[2], r[2].f[3]);
   debug_printf("fetch a: %g %g %g %g\n",
         r[3].f[0], r[3].f[1], r[3].f[2], r[3].f[3]);
#endif

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &r[chan], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
      }
   }
}

static void
exec_txd(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   const uint unit = inst->Src[3].Register.Index;
   union tgsi_exec_channel r[4];
   uint chan;

   /*
    * XXX: This is fake TXD -- the derivatives are not taken into account, yet.
    */

   switch (inst->Texture.Texture) {
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_SHADOW1D:

      FETCH(&r[0], 0, CHAN_X);

      fetch_texel(mach->Samplers[unit],
                  &r[0], &ZeroVec, &ZeroVec, &ZeroVec,   /* S, T, P, BIAS */
                  tgsi_sampler_lod_bias,
                  &r[0], &r[1], &r[2], &r[3]);           /* R, G, B, A */
      break;

   case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:

      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      fetch_texel(mach->Samplers[unit],
                  &r[0], &r[1], &r[2], &ZeroVec,   /* inputs */
                  tgsi_sampler_lod_bias,
                  &r[0], &r[1], &r[2], &r[3]);     /* outputs */
      break;

   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:

      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      fetch_texel(mach->Samplers[unit],
                  &r[0], &r[1], &r[2], &ZeroVec,
                  tgsi_sampler_lod_bias,
                  &r[0], &r[1], &r[2], &r[3]);
      break;

   case TGSI_TEXTURE_SHADOW2D_ARRAY:

      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);
      FETCH(&r[3], 0, CHAN_W);

      fetch_texel(mach->Samplers[unit],
                  &r[0], &r[1], &r[2], &r[3],
                  tgsi_sampler_lod_bias,
                  &r[0], &r[1], &r[2], &r[3]);
      break;

   default:
      assert(0);
   }

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &r[chan], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
      }
   }
}


static void
exec_txf(struct tgsi_exec_machine *mach,
	 const struct tgsi_full_instruction *inst)
{
   struct tgsi_sampler *sampler;
   const uint unit = inst->Src[2].Register.Index;
   union tgsi_exec_channel r[4];
   union tgsi_exec_channel offset[3];
   uint chan;
   float rgba[NUM_CHANNELS][QUAD_SIZE];
   int j;
   int8_t offsets[3];

   if (inst->Texture.NumOffsets == 1) {
      union tgsi_exec_channel index;
      index.i[0] = index.i[1] = index.i[2] = index.i[3] = inst->TexOffsets[0].Index;
      fetch_src_file_channel(mach, 0, inst->TexOffsets[0].File,
                             inst->TexOffsets[0].SwizzleX, &index, &ZeroVec, &offset[0]);
      fetch_src_file_channel(mach, 0, inst->TexOffsets[0].File,
                             inst->TexOffsets[0].SwizzleY, &index, &ZeroVec, &offset[1]);
      fetch_src_file_channel(mach, 0, inst->TexOffsets[0].File,
                             inst->TexOffsets[0].SwizzleZ, &index, &ZeroVec, &offset[2]);
     offsets[0] = offset[0].i[0];
     offsets[1] = offset[1].i[0];
     offsets[2] = offset[2].i[0];
   } else
     offsets[0] = offsets[1] = offsets[2] = 0;

   IFETCH(&r[3], 0, CHAN_W);

   switch(inst->Texture.Texture) {
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
      IFETCH(&r[2], 0, CHAN_Z);
      /* fallthrough */
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_1D_ARRAY:
      IFETCH(&r[1], 0, CHAN_Y);
      /* fallthrough */
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_SHADOW1D:
      IFETCH(&r[0], 0, CHAN_X);
      break;
   default:
      assert(0);
      break;
   }      

   sampler = mach->Samplers[unit];
   sampler->get_texel(sampler, r[0].i, r[1].i, r[2].i, r[3].i,
		      offsets, rgba);

   for (j = 0; j < QUAD_SIZE; j++) {
      r[0].f[j] = rgba[0][j];
      r[1].f[j] = rgba[1][j];
      r[2].f[j] = rgba[2][j];
      r[3].f[j] = rgba[3][j];
   }

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &r[chan], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
      }
   }
}

static void
exec_txq(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   struct tgsi_sampler *sampler;
   const uint unit = inst->Src[1].Register.Index;
   int result[4];
   union tgsi_exec_channel r[4], src;
   uint chan;
   int i,j;

   fetch_source(mach, &src, &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_INT);
   sampler = mach->Samplers[unit];

   sampler->get_dims(sampler, src.i[0], result);

   for (i = 0; i < QUAD_SIZE; i++) {
      for (j = 0; j < 4; j++) {
	 r[j].i[i] = result[j];
      }
   }

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
	 store_dest(mach, &r[chan], &inst->Dst[0], inst, chan,
		    TGSI_EXEC_DATA_INT);
      }
   }
}

static void
exec_sample(struct tgsi_exec_machine *mach,
            const struct tgsi_full_instruction *inst,
            uint modifier)
{
   const uint resource_unit = inst->Src[1].Register.Index;
   const uint sampler_unit = inst->Src[2].Register.Index;
   union tgsi_exec_channel r[4];
   const union tgsi_exec_channel *lod = &ZeroVec;
   enum tgsi_sampler_control control;
   uint chan;

   if (modifier != TEX_MODIFIER_NONE) {
      if (modifier == TEX_MODIFIER_LOD_BIAS)
         FETCH(&r[3], 3, CHAN_X);
      else /*TEX_MODIFIER_LOD*/
         FETCH(&r[3], 0, CHAN_W);

      if (modifier != TEX_MODIFIER_PROJECTED) {
         lod = &r[3];
      }
   }

   if (modifier == TEX_MODIFIER_EXPLICIT_LOD) {
      control = tgsi_sampler_lod_explicit;
   } else {
      control = tgsi_sampler_lod_bias;
   }

   switch (mach->Resources[resource_unit].Resource) {
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_SHADOW1D:
      FETCH(&r[0], 0, CHAN_X);

      if (modifier == TEX_MODIFIER_PROJECTED) {
         micro_div(&r[0], &r[0], &r[3]);
      }

      fetch_texel(mach->Samplers[sampler_unit],
                  &r[0], &ZeroVec, &ZeroVec, lod,  /* S, T, P, LOD */
                  control,
                  &r[0], &r[1], &r[2], &r[3]);     /* R, G, B, A */
      break;

   case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      if (modifier == TEX_MODIFIER_PROJECTED) {
         micro_div(&r[0], &r[0], &r[3]);
         micro_div(&r[1], &r[1], &r[3]);
         micro_div(&r[2], &r[2], &r[3]);
      }

      fetch_texel(mach->Samplers[sampler_unit],
                  &r[0], &r[1], &r[2], lod,     /* S, T, P, LOD */
                  control,
                  &r[0], &r[1], &r[2], &r[3]);  /* outputs */
      break;

   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      if (modifier == TEX_MODIFIER_PROJECTED) {
         micro_div(&r[0], &r[0], &r[3]);
         micro_div(&r[1], &r[1], &r[3]);
         micro_div(&r[2], &r[2], &r[3]);
      }

      fetch_texel(mach->Samplers[sampler_unit],
                  &r[0], &r[1], &r[2], lod,
                  control,
                  &r[0], &r[1], &r[2], &r[3]);
      break;

   case TGSI_TEXTURE_SHADOW2D_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);
      FETCH(&r[3], 0, CHAN_W);

      assert(modifier != TEX_MODIFIER_PROJECTED);

      fetch_texel(mach->Samplers[sampler_unit],
                  &r[0], &r[1], &r[2], &r[3],
                  control,
                  &r[0], &r[1], &r[2], &r[3]);
      break;

   default:
      assert(0);
   }

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &r[chan], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
      }
   }
}

static void
exec_sample_d(struct tgsi_exec_machine *mach,
              const struct tgsi_full_instruction *inst)
{
   const uint resource_unit = inst->Src[1].Register.Index;
   const uint sampler_unit = inst->Src[2].Register.Index;
   union tgsi_exec_channel r[4];
   uint chan;
   /*
    * XXX: This is fake SAMPLE_D -- the derivatives are not taken into account, yet.
    */

   switch (mach->Resources[resource_unit].Resource) {
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_SHADOW1D:

      FETCH(&r[0], 0, CHAN_X);

      fetch_texel(mach->Samplers[sampler_unit],
                  &r[0], &ZeroVec, &ZeroVec, &ZeroVec,   /* S, T, P, BIAS */
                  tgsi_sampler_lod_bias,
                  &r[0], &r[1], &r[2], &r[3]);           /* R, G, B, A */
      break;

   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:

      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      fetch_texel(mach->Samplers[sampler_unit],
                  &r[0], &r[1], &r[2], &ZeroVec,   /* inputs */
                  tgsi_sampler_lod_bias,
                  &r[0], &r[1], &r[2], &r[3]);     /* outputs */
      break;

   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:

      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      fetch_texel(mach->Samplers[sampler_unit],
                  &r[0], &r[1], &r[2], &ZeroVec,
                  tgsi_sampler_lod_bias,
                  &r[0], &r[1], &r[2], &r[3]);
      break;

   default:
      assert(0);
   }

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &r[chan], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
      }
   }
}


/**
 * Evaluate a constant-valued coefficient at the position of the
 * current quad.
 */
static void
eval_constant_coef(
   struct tgsi_exec_machine *mach,
   unsigned attrib,
   unsigned chan )
{
   unsigned i;

   for( i = 0; i < QUAD_SIZE; i++ ) {
      mach->Inputs[attrib].xyzw[chan].f[i] = mach->InterpCoefs[attrib].a0[chan];
   }
}

/**
 * Evaluate a linear-valued coefficient at the position of the
 * current quad.
 */
static void
eval_linear_coef(
   struct tgsi_exec_machine *mach,
   unsigned attrib,
   unsigned chan )
{
   const float x = mach->QuadPos.xyzw[0].f[0];
   const float y = mach->QuadPos.xyzw[1].f[0];
   const float dadx = mach->InterpCoefs[attrib].dadx[chan];
   const float dady = mach->InterpCoefs[attrib].dady[chan];
   const float a0 = mach->InterpCoefs[attrib].a0[chan] + dadx * x + dady * y;
   mach->Inputs[attrib].xyzw[chan].f[0] = a0;
   mach->Inputs[attrib].xyzw[chan].f[1] = a0 + dadx;
   mach->Inputs[attrib].xyzw[chan].f[2] = a0 + dady;
   mach->Inputs[attrib].xyzw[chan].f[3] = a0 + dadx + dady;
}

/**
 * Evaluate a perspective-valued coefficient at the position of the
 * current quad.
 */
static void
eval_perspective_coef(
   struct tgsi_exec_machine *mach,
   unsigned attrib,
   unsigned chan )
{
   const float x = mach->QuadPos.xyzw[0].f[0];
   const float y = mach->QuadPos.xyzw[1].f[0];
   const float dadx = mach->InterpCoefs[attrib].dadx[chan];
   const float dady = mach->InterpCoefs[attrib].dady[chan];
   const float a0 = mach->InterpCoefs[attrib].a0[chan] + dadx * x + dady * y;
   const float *w = mach->QuadPos.xyzw[3].f;
   /* divide by W here */
   mach->Inputs[attrib].xyzw[chan].f[0] = a0 / w[0];
   mach->Inputs[attrib].xyzw[chan].f[1] = (a0 + dadx) / w[1];
   mach->Inputs[attrib].xyzw[chan].f[2] = (a0 + dady) / w[2];
   mach->Inputs[attrib].xyzw[chan].f[3] = (a0 + dadx + dady) / w[3];
}


typedef void (* eval_coef_func)(
   struct tgsi_exec_machine *mach,
   unsigned attrib,
   unsigned chan );

static void
exec_declaration(struct tgsi_exec_machine *mach,
                 const struct tgsi_full_declaration *decl)
{
   if (decl->Declaration.File == TGSI_FILE_RESOURCE) {
      mach->Resources[decl->Range.First] = decl->Resource;
      return;
   }

   if (mach->Processor == TGSI_PROCESSOR_FRAGMENT) {
      if (decl->Declaration.File == TGSI_FILE_INPUT) {
         uint first, last, mask;

         first = decl->Range.First;
         last = decl->Range.Last;
         mask = decl->Declaration.UsageMask;

         /* XXX we could remove this special-case code since
          * mach->InterpCoefs[first].a0 should already have the
          * front/back-face value.  But we should first update the
          * ureg code to emit the right UsageMask value (WRITEMASK_X).
          * Then, we could remove the tgsi_exec_machine::Face field.
          */
         /* XXX make FACE a system value */
         if (decl->Semantic.Name == TGSI_SEMANTIC_FACE) {
            uint i;

            assert(decl->Semantic.Index == 0);
            assert(first == last);

            for (i = 0; i < QUAD_SIZE; i++) {
               mach->Inputs[first].xyzw[0].f[i] = mach->Face;
            }
         } else {
            eval_coef_func eval;
            uint i, j;

            switch (decl->Declaration.Interpolate) {
            case TGSI_INTERPOLATE_CONSTANT:
               eval = eval_constant_coef;
               break;

            case TGSI_INTERPOLATE_LINEAR:
               eval = eval_linear_coef;
               break;

            case TGSI_INTERPOLATE_PERSPECTIVE:
               eval = eval_perspective_coef;
               break;

            case TGSI_INTERPOLATE_COLOR:
               eval = mach->flatshade_color ? eval_constant_coef : eval_perspective_coef;
               break;

            default:
               assert(0);
               return;
            }

            for (j = 0; j < NUM_CHANNELS; j++) {
               if (mask & (1 << j)) {
                  for (i = first; i <= last; i++) {
                     eval(mach, i, j);
                  }
               }
            }
         }
      }
   }

   if (decl->Declaration.File == TGSI_FILE_SYSTEM_VALUE) {
      mach->SysSemanticToIndex[decl->Declaration.Semantic] = decl->Range.First;
   }
}


typedef void (* micro_op)(union tgsi_exec_channel *dst);

static void
exec_vector(struct tgsi_exec_machine *mach,
            const struct tgsi_full_instruction *inst,
            micro_op op,
            enum tgsi_exec_datatype dst_datatype)
{
   unsigned int chan;

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         union tgsi_exec_channel dst;

         op(&dst);
         store_dest(mach, &dst, &inst->Dst[0], inst, chan, dst_datatype);
      }
   }
}

typedef void (* micro_unary_op)(union tgsi_exec_channel *dst,
                                const union tgsi_exec_channel *src);

static void
exec_scalar_unary(struct tgsi_exec_machine *mach,
                  const struct tgsi_full_instruction *inst,
                  micro_unary_op op,
                  enum tgsi_exec_datatype dst_datatype,
                  enum tgsi_exec_datatype src_datatype)
{
   unsigned int chan;
   union tgsi_exec_channel src;
   union tgsi_exec_channel dst;

   fetch_source(mach, &src, &inst->Src[0], CHAN_X, src_datatype);
   op(&dst, &src);
   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &dst, &inst->Dst[0], inst, chan, dst_datatype);
      }
   }
}

static void
exec_vector_unary(struct tgsi_exec_machine *mach,
                  const struct tgsi_full_instruction *inst,
                  micro_unary_op op,
                  enum tgsi_exec_datatype dst_datatype,
                  enum tgsi_exec_datatype src_datatype)
{
   unsigned int chan;
   struct tgsi_exec_vector dst;

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         union tgsi_exec_channel src;

         fetch_source(mach, &src, &inst->Src[0], chan, src_datatype);
         op(&dst.xyzw[chan], &src);
      }
   }
   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &dst.xyzw[chan], &inst->Dst[0], inst, chan, dst_datatype);
      }
   }
}

typedef void (* micro_binary_op)(union tgsi_exec_channel *dst,
                                 const union tgsi_exec_channel *src0,
                                 const union tgsi_exec_channel *src1);

static void
exec_scalar_binary(struct tgsi_exec_machine *mach,
                   const struct tgsi_full_instruction *inst,
                   micro_binary_op op,
                   enum tgsi_exec_datatype dst_datatype,
                   enum tgsi_exec_datatype src_datatype)
{
   unsigned int chan;
   union tgsi_exec_channel src[2];
   union tgsi_exec_channel dst;

   fetch_source(mach, &src[0], &inst->Src[0], CHAN_X, src_datatype);
   fetch_source(mach, &src[1], &inst->Src[1], CHAN_Y, src_datatype);
   op(&dst, &src[0], &src[1]);
   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &dst, &inst->Dst[0], inst, chan, dst_datatype);
      }
   }
}

static void
exec_vector_binary(struct tgsi_exec_machine *mach,
                   const struct tgsi_full_instruction *inst,
                   micro_binary_op op,
                   enum tgsi_exec_datatype dst_datatype,
                   enum tgsi_exec_datatype src_datatype)
{
   unsigned int chan;
   struct tgsi_exec_vector dst;

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         union tgsi_exec_channel src[2];

         fetch_source(mach, &src[0], &inst->Src[0], chan, src_datatype);
         fetch_source(mach, &src[1], &inst->Src[1], chan, src_datatype);
         op(&dst.xyzw[chan], &src[0], &src[1]);
      }
   }
   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &dst.xyzw[chan], &inst->Dst[0], inst, chan, dst_datatype);
      }
   }
}

typedef void (* micro_trinary_op)(union tgsi_exec_channel *dst,
                                  const union tgsi_exec_channel *src0,
                                  const union tgsi_exec_channel *src1,
                                  const union tgsi_exec_channel *src2);

static void
exec_vector_trinary(struct tgsi_exec_machine *mach,
                    const struct tgsi_full_instruction *inst,
                    micro_trinary_op op,
                    enum tgsi_exec_datatype dst_datatype,
                    enum tgsi_exec_datatype src_datatype)
{
   unsigned int chan;
   struct tgsi_exec_vector dst;

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         union tgsi_exec_channel src[3];

         fetch_source(mach, &src[0], &inst->Src[0], chan, src_datatype);
         fetch_source(mach, &src[1], &inst->Src[1], chan, src_datatype);
         fetch_source(mach, &src[2], &inst->Src[2], chan, src_datatype);
         op(&dst.xyzw[chan], &src[0], &src[1], &src[2]);
      }
   }
   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &dst.xyzw[chan], &inst->Dst[0], inst, chan, dst_datatype);
      }
   }
}

static void
exec_dp3(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   unsigned int chan;
   union tgsi_exec_channel arg[3];

   fetch_source(mach, &arg[0], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &arg[1], &inst->Src[1], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   micro_mul(&arg[2], &arg[0], &arg[1]);

   for (chan = CHAN_Y; chan <= CHAN_Z; chan++) {
      fetch_source(mach, &arg[0], &inst->Src[0], chan, TGSI_EXEC_DATA_FLOAT);
      fetch_source(mach, &arg[1], &inst->Src[1], chan, TGSI_EXEC_DATA_FLOAT);
      micro_mad(&arg[2], &arg[0], &arg[1], &arg[2]);
   }

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &arg[2], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
      }
   }
}

static void
exec_dp4(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   unsigned int chan;
   union tgsi_exec_channel arg[3];

   fetch_source(mach, &arg[0], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &arg[1], &inst->Src[1], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   micro_mul(&arg[2], &arg[0], &arg[1]);

   for (chan = CHAN_Y; chan <= CHAN_W; chan++) {
      fetch_source(mach, &arg[0], &inst->Src[0], chan, TGSI_EXEC_DATA_FLOAT);
      fetch_source(mach, &arg[1], &inst->Src[1], chan, TGSI_EXEC_DATA_FLOAT);
      micro_mad(&arg[2], &arg[0], &arg[1], &arg[2]);
   }

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &arg[2], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
      }
   }
}

static void
exec_dp2a(struct tgsi_exec_machine *mach,
          const struct tgsi_full_instruction *inst)
{
   unsigned int chan;
   union tgsi_exec_channel arg[3];

   fetch_source(mach, &arg[0], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &arg[1], &inst->Src[1], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   micro_mul(&arg[2], &arg[0], &arg[1]);

   fetch_source(mach, &arg[0], &inst->Src[0], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &arg[1], &inst->Src[1], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   micro_mad(&arg[0], &arg[0], &arg[1], &arg[2]);

   fetch_source(mach, &arg[1], &inst->Src[2], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   micro_add(&arg[0], &arg[0], &arg[1]);

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &arg[0], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
      }
   }
}

static void
exec_dph(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   unsigned int chan;
   union tgsi_exec_channel arg[3];

   fetch_source(mach, &arg[0], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &arg[1], &inst->Src[1], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   micro_mul(&arg[2], &arg[0], &arg[1]);

   fetch_source(mach, &arg[0], &inst->Src[0], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &arg[1], &inst->Src[1], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   micro_mad(&arg[2], &arg[0], &arg[1], &arg[2]);

   fetch_source(mach, &arg[0], &inst->Src[0], CHAN_Z, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &arg[1], &inst->Src[1], CHAN_Z, TGSI_EXEC_DATA_FLOAT);
   micro_mad(&arg[0], &arg[0], &arg[1], &arg[2]);

   fetch_source(mach, &arg[1], &inst->Src[1], CHAN_W, TGSI_EXEC_DATA_FLOAT);
   micro_add(&arg[0], &arg[0], &arg[1]);

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &arg[0], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
      }
   }
}

static void
exec_dp2(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   unsigned int chan;
   union tgsi_exec_channel arg[3];

   fetch_source(mach, &arg[0], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &arg[1], &inst->Src[1], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   micro_mul(&arg[2], &arg[0], &arg[1]);

   fetch_source(mach, &arg[0], &inst->Src[0], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &arg[1], &inst->Src[1], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   micro_mad(&arg[2], &arg[0], &arg[1], &arg[2]);

   for (chan = 0; chan < NUM_CHANNELS; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         store_dest(mach, &arg[2], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
      }
   }
}

static void
exec_nrm4(struct tgsi_exec_machine *mach,
          const struct tgsi_full_instruction *inst)
{
   unsigned int chan;
   union tgsi_exec_channel arg[4];
   union tgsi_exec_channel scale;

   fetch_source(mach, &arg[0], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   micro_mul(&scale, &arg[0], &arg[0]);

   for (chan = CHAN_Y; chan <= CHAN_W; chan++) {
      union tgsi_exec_channel product;

      fetch_source(mach, &arg[chan], &inst->Src[0], chan, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&product, &arg[chan], &arg[chan]);
      micro_add(&scale, &scale, &product);
   }

   micro_rsq(&scale, &scale);

   for (chan = CHAN_X; chan <= CHAN_W; chan++) {
      if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
         micro_mul(&arg[chan], &arg[chan], &scale);
         store_dest(mach, &arg[chan], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
      }
   }
}

static void
exec_nrm3(struct tgsi_exec_machine *mach,
          const struct tgsi_full_instruction *inst)
{
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_XYZ) {
      unsigned int chan;
      union tgsi_exec_channel arg[3];
      union tgsi_exec_channel scale;

      fetch_source(mach, &arg[0], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&scale, &arg[0], &arg[0]);

      for (chan = CHAN_Y; chan <= CHAN_Z; chan++) {
         union tgsi_exec_channel product;

         fetch_source(mach, &arg[chan], &inst->Src[0], chan, TGSI_EXEC_DATA_FLOAT);
         micro_mul(&product, &arg[chan], &arg[chan]);
         micro_add(&scale, &scale, &product);
      }

      micro_rsq(&scale, &scale);

      for (chan = CHAN_X; chan <= CHAN_Z; chan++) {
         if (inst->Dst[0].Register.WriteMask & (1 << chan)) {
            micro_mul(&arg[chan], &arg[chan], &scale);
            store_dest(mach, &arg[chan], &inst->Dst[0], inst, chan, TGSI_EXEC_DATA_FLOAT);
         }
      }
   }

   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      store_dest(mach, &OneVec, &inst->Dst[0], inst, CHAN_W, TGSI_EXEC_DATA_FLOAT);
   }
}

static void
exec_scs(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_XY) {
      union tgsi_exec_channel arg;
      union tgsi_exec_channel result;

      fetch_source(mach, &arg, &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);

      if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
         micro_cos(&result, &arg);
         store_dest(mach, &result, &inst->Dst[0], inst, CHAN_X, TGSI_EXEC_DATA_FLOAT);
      }
      if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
         micro_sin(&result, &arg);
         store_dest(mach, &result, &inst->Dst[0], inst, CHAN_Y, TGSI_EXEC_DATA_FLOAT);
      }
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
      store_dest(mach, &ZeroVec, &inst->Dst[0], inst, CHAN_Z, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      store_dest(mach, &OneVec, &inst->Dst[0], inst, CHAN_W, TGSI_EXEC_DATA_FLOAT);
   }
}

static void
exec_x2d(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   union tgsi_exec_channel r[4];
   union tgsi_exec_channel d[2];

   fetch_source(mach, &r[0], &inst->Src[1], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &r[1], &inst->Src[1], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_XZ) {
      fetch_source(mach, &r[2], &inst->Src[2], CHAN_X, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&r[2], &r[2], &r[0]);
      fetch_source(mach, &r[3], &inst->Src[2], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&r[3], &r[3], &r[1]);
      micro_add(&r[2], &r[2], &r[3]);
      fetch_source(mach, &r[3], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
      micro_add(&d[0], &r[2], &r[3]);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_YW) {
      fetch_source(mach, &r[2], &inst->Src[2], CHAN_Z, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&r[2], &r[2], &r[0]);
      fetch_source(mach, &r[3], &inst->Src[2], CHAN_W, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&r[3], &r[3], &r[1]);
      micro_add(&r[2], &r[2], &r[3]);
      fetch_source(mach, &r[3], &inst->Src[0], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
      micro_add(&d[1], &r[2], &r[3]);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
      store_dest(mach, &d[0], &inst->Dst[0], inst, CHAN_X, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
      store_dest(mach, &d[1], &inst->Dst[0], inst, CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
      store_dest(mach, &d[0], &inst->Dst[0], inst, CHAN_Z, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      store_dest(mach, &d[1], &inst->Dst[0], inst, CHAN_W, TGSI_EXEC_DATA_FLOAT);
   }
}

static void
exec_rfl(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   union tgsi_exec_channel r[9];

   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_XYZ) {
      /* r0 = dp3(src0, src0) */
      fetch_source(mach, &r[2], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&r[0], &r[2], &r[2]);
      fetch_source(mach, &r[4], &inst->Src[0], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&r[8], &r[4], &r[4]);
      micro_add(&r[0], &r[0], &r[8]);
      fetch_source(mach, &r[6], &inst->Src[0], CHAN_Z, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&r[8], &r[6], &r[6]);
      micro_add(&r[0], &r[0], &r[8]);

      /* r1 = dp3(src0, src1) */
      fetch_source(mach, &r[3], &inst->Src[1], CHAN_X, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&r[1], &r[2], &r[3]);
      fetch_source(mach, &r[5], &inst->Src[1], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&r[8], &r[4], &r[5]);
      micro_add(&r[1], &r[1], &r[8]);
      fetch_source(mach, &r[7], &inst->Src[1], CHAN_Z, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&r[8], &r[6], &r[7]);
      micro_add(&r[1], &r[1], &r[8]);

      /* r1 = 2 * r1 / r0 */
      micro_add(&r[1], &r[1], &r[1]);
      micro_div(&r[1], &r[1], &r[0]);

      if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
         micro_mul(&r[2], &r[2], &r[1]);
         micro_sub(&r[2], &r[2], &r[3]);
         store_dest(mach, &r[2], &inst->Dst[0], inst, CHAN_X, TGSI_EXEC_DATA_FLOAT);
      }
      if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
         micro_mul(&r[4], &r[4], &r[1]);
         micro_sub(&r[4], &r[4], &r[5]);
         store_dest(mach, &r[4], &inst->Dst[0], inst, CHAN_Y, TGSI_EXEC_DATA_FLOAT);
      }
      if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
         micro_mul(&r[6], &r[6], &r[1]);
         micro_sub(&r[6], &r[6], &r[7]);
         store_dest(mach, &r[6], &inst->Dst[0], inst, CHAN_Z, TGSI_EXEC_DATA_FLOAT);
      }
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      store_dest(mach, &OneVec, &inst->Dst[0], inst, CHAN_W, TGSI_EXEC_DATA_FLOAT);
   }
}

static void
exec_xpd(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   union tgsi_exec_channel r[6];
   union tgsi_exec_channel d[3];

   fetch_source(mach, &r[0], &inst->Src[0], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &r[1], &inst->Src[1], CHAN_Z, TGSI_EXEC_DATA_FLOAT);

   micro_mul(&r[2], &r[0], &r[1]);

   fetch_source(mach, &r[3], &inst->Src[0], CHAN_Z, TGSI_EXEC_DATA_FLOAT);
   fetch_source(mach, &r[4], &inst->Src[1], CHAN_Y, TGSI_EXEC_DATA_FLOAT);

   micro_mul(&r[5], &r[3], &r[4] );
   micro_sub(&d[CHAN_X], &r[2], &r[5]);

   fetch_source(mach, &r[2], &inst->Src[1], CHAN_X, TGSI_EXEC_DATA_FLOAT);

   micro_mul(&r[3], &r[3], &r[2]);

   fetch_source(mach, &r[5], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);

   micro_mul(&r[1], &r[1], &r[5]);
   micro_sub(&d[CHAN_Y], &r[3], &r[1]);

   micro_mul(&r[5], &r[5], &r[4]);
   micro_mul(&r[0], &r[0], &r[2]);
   micro_sub(&d[CHAN_Z], &r[5], &r[0]);

   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
      store_dest(mach, &d[CHAN_X], &inst->Dst[0], inst, CHAN_X, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
      store_dest(mach, &d[CHAN_Y], &inst->Dst[0], inst, CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
      store_dest(mach, &d[CHAN_Z], &inst->Dst[0], inst, CHAN_Z, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      store_dest(mach, &OneVec, &inst->Dst[0], inst, CHAN_W, TGSI_EXEC_DATA_FLOAT);
   }
}

static void
exec_dst(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   union tgsi_exec_channel r[2];
   union tgsi_exec_channel d[4];

   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
      fetch_source(mach, &r[0], &inst->Src[0], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
      fetch_source(mach, &r[1], &inst->Src[1], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
      micro_mul(&d[CHAN_Y], &r[0], &r[1]);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
      fetch_source(mach, &d[CHAN_Z], &inst->Src[0], CHAN_Z, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      fetch_source(mach, &d[CHAN_W], &inst->Src[1], CHAN_W, TGSI_EXEC_DATA_FLOAT);
   }

   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
      store_dest(mach, &OneVec, &inst->Dst[0], inst, CHAN_X, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
      store_dest(mach, &d[CHAN_Y], &inst->Dst[0], inst, CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
      store_dest(mach, &d[CHAN_Z], &inst->Dst[0], inst, CHAN_Z, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      store_dest(mach, &d[CHAN_W], &inst->Dst[0], inst, CHAN_W, TGSI_EXEC_DATA_FLOAT);
   }
}

static void
exec_log(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   union tgsi_exec_channel r[3];

   fetch_source(mach, &r[0], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   micro_abs(&r[2], &r[0]);  /* r2 = abs(r0) */
   micro_lg2(&r[1], &r[2]);  /* r1 = lg2(r2) */
   micro_flr(&r[0], &r[1]);  /* r0 = floor(r1) */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
      store_dest(mach, &r[0], &inst->Dst[0], inst, CHAN_X, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
      micro_exp2(&r[0], &r[0]);       /* r0 = 2 ^ r0 */
      micro_div(&r[0], &r[2], &r[0]); /* r0 = r2 / r0 */
      store_dest(mach, &r[0], &inst->Dst[0], inst, CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
      store_dest(mach, &r[1], &inst->Dst[0], inst, CHAN_Z, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      store_dest(mach, &OneVec, &inst->Dst[0], inst, CHAN_W, TGSI_EXEC_DATA_FLOAT);
   }
}

static void
exec_exp(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   union tgsi_exec_channel r[3];

   fetch_source(mach, &r[0], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
   micro_flr(&r[1], &r[0]);  /* r1 = floor(r0) */
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
      micro_exp2(&r[2], &r[1]);       /* r2 = 2 ^ r1 */
      store_dest(mach, &r[2], &inst->Dst[0], inst, CHAN_X, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
      micro_sub(&r[2], &r[0], &r[1]); /* r2 = r0 - r1 */
      store_dest(mach, &r[2], &inst->Dst[0], inst, CHAN_Y, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
      micro_exp2(&r[2], &r[0]);       /* r2 = 2 ^ r0 */
      store_dest(mach, &r[2], &inst->Dst[0], inst, CHAN_Z, TGSI_EXEC_DATA_FLOAT);
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      store_dest(mach, &OneVec, &inst->Dst[0], inst, CHAN_W, TGSI_EXEC_DATA_FLOAT);
   }
}

static void
exec_lit(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   union tgsi_exec_channel r[3];
   union tgsi_exec_channel d[3];

   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_YZ) {
      fetch_source(mach, &r[0], &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_FLOAT);
      if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Z) {
         fetch_source(mach, &r[1], &inst->Src[0], CHAN_Y, TGSI_EXEC_DATA_FLOAT);
         micro_max(&r[1], &r[1], &ZeroVec);

         fetch_source(mach, &r[2], &inst->Src[0], CHAN_W, TGSI_EXEC_DATA_FLOAT);
         micro_min(&r[2], &r[2], &P128Vec);
         micro_max(&r[2], &r[2], &M128Vec);
         micro_pow(&r[1], &r[1], &r[2]);
         micro_lt(&d[CHAN_Z], &ZeroVec, &r[0], &r[1], &ZeroVec);
         store_dest(mach, &d[CHAN_Z], &inst->Dst[0], inst, CHAN_Z, TGSI_EXEC_DATA_FLOAT);
      }
      if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_Y) {
         micro_max(&d[CHAN_Y], &r[0], &ZeroVec);
         store_dest(mach, &d[CHAN_Y], &inst->Dst[0], inst, CHAN_Y, TGSI_EXEC_DATA_FLOAT);
      }
   }
   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_X) {
      store_dest(mach, &OneVec, &inst->Dst[0], inst, CHAN_X, TGSI_EXEC_DATA_FLOAT);
   }

   if (inst->Dst[0].Register.WriteMask & TGSI_WRITEMASK_W) {
      store_dest(mach, &OneVec, &inst->Dst[0], inst, CHAN_W, TGSI_EXEC_DATA_FLOAT);
   }
}

static void
exec_break(struct tgsi_exec_machine *mach)
{
   if (mach->BreakType == TGSI_EXEC_BREAK_INSIDE_LOOP) {
      /* turn off loop channels for each enabled exec channel */
      mach->LoopMask &= ~mach->ExecMask;
      /* Todo: if mach->LoopMask == 0, jump to end of loop */
      UPDATE_EXEC_MASK(mach);
   } else {
      assert(mach->BreakType == TGSI_EXEC_BREAK_INSIDE_SWITCH);

      mach->Switch.mask = 0x0;

      UPDATE_EXEC_MASK(mach);
   }
}

static void
exec_switch(struct tgsi_exec_machine *mach,
            const struct tgsi_full_instruction *inst)
{
   assert(mach->SwitchStackTop < TGSI_EXEC_MAX_SWITCH_NESTING);
   assert(mach->BreakStackTop < TGSI_EXEC_MAX_BREAK_STACK);

   mach->SwitchStack[mach->SwitchStackTop++] = mach->Switch;
   fetch_source(mach, &mach->Switch.selector, &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_UINT);
   mach->Switch.mask = 0x0;
   mach->Switch.defaultMask = 0x0;

   mach->BreakStack[mach->BreakStackTop++] = mach->BreakType;
   mach->BreakType = TGSI_EXEC_BREAK_INSIDE_SWITCH;

   UPDATE_EXEC_MASK(mach);
}

static void
exec_case(struct tgsi_exec_machine *mach,
          const struct tgsi_full_instruction *inst)
{
   uint prevMask = mach->SwitchStack[mach->SwitchStackTop - 1].mask;
   union tgsi_exec_channel src;
   uint mask = 0;

   fetch_source(mach, &src, &inst->Src[0], CHAN_X, TGSI_EXEC_DATA_UINT);

   if (mach->Switch.selector.u[0] == src.u[0]) {
      mask |= 0x1;
   }
   if (mach->Switch.selector.u[1] == src.u[1]) {
      mask |= 0x2;
   }
   if (mach->Switch.selector.u[2] == src.u[2]) {
      mask |= 0x4;
   }
   if (mach->Switch.selector.u[3] == src.u[3]) {
      mask |= 0x8;
   }

   mach->Switch.defaultMask |= mask;

   mach->Switch.mask |= mask & prevMask;

   UPDATE_EXEC_MASK(mach);
}

static void
exec_default(struct tgsi_exec_machine *mach)
{
   uint prevMask = mach->SwitchStack[mach->SwitchStackTop - 1].mask;

   mach->Switch.mask |= ~mach->Switch.defaultMask & prevMask;

   UPDATE_EXEC_MASK(mach);
}

static void
exec_endswitch(struct tgsi_exec_machine *mach)
{
   mach->Switch = mach->SwitchStack[--mach->SwitchStackTop];
   mach->BreakType = mach->BreakStack[--mach->BreakStackTop];

   UPDATE_EXEC_MASK(mach);
}

static void
micro_i2f(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->f[0] = (float)src->i[0];
   dst->f[1] = (float)src->i[1];
   dst->f[2] = (float)src->i[2];
   dst->f[3] = (float)src->i[3];
}

static void
micro_not(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->u[0] = ~src->u[0];
   dst->u[1] = ~src->u[1];
   dst->u[2] = ~src->u[2];
   dst->u[3] = ~src->u[3];
}

static void
micro_shl(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] << src1->u[0];
   dst->u[1] = src0->u[1] << src1->u[1];
   dst->u[2] = src0->u[2] << src1->u[2];
   dst->u[3] = src0->u[3] << src1->u[3];
}

static void
micro_and(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] & src1->u[0];
   dst->u[1] = src0->u[1] & src1->u[1];
   dst->u[2] = src0->u[2] & src1->u[2];
   dst->u[3] = src0->u[3] & src1->u[3];
}

static void
micro_or(union tgsi_exec_channel *dst,
         const union tgsi_exec_channel *src0,
         const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] | src1->u[0];
   dst->u[1] = src0->u[1] | src1->u[1];
   dst->u[2] = src0->u[2] | src1->u[2];
   dst->u[3] = src0->u[3] | src1->u[3];
}

static void
micro_xor(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] ^ src1->u[0];
   dst->u[1] = src0->u[1] ^ src1->u[1];
   dst->u[2] = src0->u[2] ^ src1->u[2];
   dst->u[3] = src0->u[3] ^ src1->u[3];
}

static void
micro_mod(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src0,
          const union tgsi_exec_channel *src1)
{
   dst->i[0] = src0->i[0] % src1->i[0];
   dst->i[1] = src0->i[1] % src1->i[1];
   dst->i[2] = src0->i[2] % src1->i[2];
   dst->i[3] = src0->i[3] % src1->i[3];
}

static void
micro_f2i(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->i[0] = (int)src->f[0];
   dst->i[1] = (int)src->f[1];
   dst->i[2] = (int)src->f[2];
   dst->i[3] = (int)src->f[3];
}

static void
micro_idiv(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->i[0] = src0->i[0] / src1->i[0];
   dst->i[1] = src0->i[1] / src1->i[1];
   dst->i[2] = src0->i[2] / src1->i[2];
   dst->i[3] = src0->i[3] / src1->i[3];
}

static void
micro_imax(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->i[0] = src0->i[0] > src1->i[0] ? src0->i[0] : src1->i[0];
   dst->i[1] = src0->i[1] > src1->i[1] ? src0->i[1] : src1->i[1];
   dst->i[2] = src0->i[2] > src1->i[2] ? src0->i[2] : src1->i[2];
   dst->i[3] = src0->i[3] > src1->i[3] ? src0->i[3] : src1->i[3];
}

static void
micro_imin(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->i[0] = src0->i[0] < src1->i[0] ? src0->i[0] : src1->i[0];
   dst->i[1] = src0->i[1] < src1->i[1] ? src0->i[1] : src1->i[1];
   dst->i[2] = src0->i[2] < src1->i[2] ? src0->i[2] : src1->i[2];
   dst->i[3] = src0->i[3] < src1->i[3] ? src0->i[3] : src1->i[3];
}

static void
micro_isge(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->i[0] = src0->i[0] >= src1->i[0] ? -1 : 0;
   dst->i[1] = src0->i[1] >= src1->i[1] ? -1 : 0;
   dst->i[2] = src0->i[2] >= src1->i[2] ? -1 : 0;
   dst->i[3] = src0->i[3] >= src1->i[3] ? -1 : 0;
}

static void
micro_ishr(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->i[0] = src0->i[0] >> src1->i[0];
   dst->i[1] = src0->i[1] >> src1->i[1];
   dst->i[2] = src0->i[2] >> src1->i[2];
   dst->i[3] = src0->i[3] >> src1->i[3];
}

static void
micro_islt(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->i[0] = src0->i[0] < src1->i[0] ? -1 : 0;
   dst->i[1] = src0->i[1] < src1->i[1] ? -1 : 0;
   dst->i[2] = src0->i[2] < src1->i[2] ? -1 : 0;
   dst->i[3] = src0->i[3] < src1->i[3] ? -1 : 0;
}

static void
micro_f2u(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->u[0] = (uint)src->f[0];
   dst->u[1] = (uint)src->f[1];
   dst->u[2] = (uint)src->f[2];
   dst->u[3] = (uint)src->f[3];
}

static void
micro_u2f(union tgsi_exec_channel *dst,
          const union tgsi_exec_channel *src)
{
   dst->f[0] = (float)src->u[0];
   dst->f[1] = (float)src->u[1];
   dst->f[2] = (float)src->u[2];
   dst->f[3] = (float)src->u[3];
}

static void
micro_uadd(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] + src1->u[0];
   dst->u[1] = src0->u[1] + src1->u[1];
   dst->u[2] = src0->u[2] + src1->u[2];
   dst->u[3] = src0->u[3] + src1->u[3];
}

static void
micro_udiv(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] / src1->u[0];
   dst->u[1] = src0->u[1] / src1->u[1];
   dst->u[2] = src0->u[2] / src1->u[2];
   dst->u[3] = src0->u[3] / src1->u[3];
}

static void
micro_umad(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1,
           const union tgsi_exec_channel *src2)
{
   dst->u[0] = src0->u[0] * src1->u[0] + src2->u[0];
   dst->u[1] = src0->u[1] * src1->u[1] + src2->u[1];
   dst->u[2] = src0->u[2] * src1->u[2] + src2->u[2];
   dst->u[3] = src0->u[3] * src1->u[3] + src2->u[3];
}

static void
micro_umax(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] > src1->u[0] ? src0->u[0] : src1->u[0];
   dst->u[1] = src0->u[1] > src1->u[1] ? src0->u[1] : src1->u[1];
   dst->u[2] = src0->u[2] > src1->u[2] ? src0->u[2] : src1->u[2];
   dst->u[3] = src0->u[3] > src1->u[3] ? src0->u[3] : src1->u[3];
}

static void
micro_umin(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] < src1->u[0] ? src0->u[0] : src1->u[0];
   dst->u[1] = src0->u[1] < src1->u[1] ? src0->u[1] : src1->u[1];
   dst->u[2] = src0->u[2] < src1->u[2] ? src0->u[2] : src1->u[2];
   dst->u[3] = src0->u[3] < src1->u[3] ? src0->u[3] : src1->u[3];
}

static void
micro_umod(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] % src1->u[0];
   dst->u[1] = src0->u[1] % src1->u[1];
   dst->u[2] = src0->u[2] % src1->u[2];
   dst->u[3] = src0->u[3] % src1->u[3];
}

static void
micro_umul(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] * src1->u[0];
   dst->u[1] = src0->u[1] * src1->u[1];
   dst->u[2] = src0->u[2] * src1->u[2];
   dst->u[3] = src0->u[3] * src1->u[3];
}

static void
micro_useq(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] == src1->u[0] ? ~0 : 0;
   dst->u[1] = src0->u[1] == src1->u[1] ? ~0 : 0;
   dst->u[2] = src0->u[2] == src1->u[2] ? ~0 : 0;
   dst->u[3] = src0->u[3] == src1->u[3] ? ~0 : 0;
}

static void
micro_usge(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] >= src1->u[0] ? ~0 : 0;
   dst->u[1] = src0->u[1] >= src1->u[1] ? ~0 : 0;
   dst->u[2] = src0->u[2] >= src1->u[2] ? ~0 : 0;
   dst->u[3] = src0->u[3] >= src1->u[3] ? ~0 : 0;
}

static void
micro_ushr(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] >> src1->u[0];
   dst->u[1] = src0->u[1] >> src1->u[1];
   dst->u[2] = src0->u[2] >> src1->u[2];
   dst->u[3] = src0->u[3] >> src1->u[3];
}

static void
micro_uslt(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] < src1->u[0] ? ~0 : 0;
   dst->u[1] = src0->u[1] < src1->u[1] ? ~0 : 0;
   dst->u[2] = src0->u[2] < src1->u[2] ? ~0 : 0;
   dst->u[3] = src0->u[3] < src1->u[3] ? ~0 : 0;
}

static void
micro_usne(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1)
{
   dst->u[0] = src0->u[0] != src1->u[0] ? ~0 : 0;
   dst->u[1] = src0->u[1] != src1->u[1] ? ~0 : 0;
   dst->u[2] = src0->u[2] != src1->u[2] ? ~0 : 0;
   dst->u[3] = src0->u[3] != src1->u[3] ? ~0 : 0;
}

static void
micro_uarl(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src)
{
   dst->i[0] = src->u[0];
   dst->i[1] = src->u[1];
   dst->i[2] = src->u[2];
   dst->i[3] = src->u[3];
}

static void
micro_ucmp(union tgsi_exec_channel *dst,
           const union tgsi_exec_channel *src0,
           const union tgsi_exec_channel *src1,
           const union tgsi_exec_channel *src2)
{
   dst->u[0] = src0->u[0] ? src1->u[0] : src2->u[0];
   dst->u[1] = src0->u[1] ? src1->u[1] : src2->u[1];
   dst->u[2] = src0->u[2] ? src1->u[2] : src2->u[2];
   dst->u[3] = src0->u[3] ? src1->u[3] : src2->u[3];
}

static void
exec_instruction(
   struct tgsi_exec_machine *mach,
   const struct tgsi_full_instruction *inst,
   int *pc )
{
   union tgsi_exec_channel r[10];

   (*pc)++;

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL:
      exec_vector_unary(mach, inst, micro_arl, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_MOV:
      exec_vector_unary(mach, inst, micro_mov, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_LIT:
      exec_lit(mach, inst);
      break;

   case TGSI_OPCODE_RCP:
      exec_scalar_unary(mach, inst, micro_rcp, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_RSQ:
      exec_scalar_unary(mach, inst, micro_rsq, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_EXP:
      exec_exp(mach, inst);
      break;

   case TGSI_OPCODE_LOG:
      exec_log(mach, inst);
      break;

   case TGSI_OPCODE_MUL:
      exec_vector_binary(mach, inst, micro_mul, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_ADD:
      exec_vector_binary(mach, inst, micro_add, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_DP3:
      exec_dp3(mach, inst);
      break;

   case TGSI_OPCODE_DP4:
      exec_dp4(mach, inst);
      break;

   case TGSI_OPCODE_DST:
      exec_dst(mach, inst);
      break;

   case TGSI_OPCODE_MIN:
      exec_vector_binary(mach, inst, micro_min, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_MAX:
      exec_vector_binary(mach, inst, micro_max, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_SLT:
      exec_vector_binary(mach, inst, micro_slt, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_SGE:
      exec_vector_binary(mach, inst, micro_sge, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_MAD:
      exec_vector_trinary(mach, inst, micro_mad, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_SUB:
      exec_vector_binary(mach, inst, micro_sub, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_LRP:
      exec_vector_trinary(mach, inst, micro_lrp, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_CND:
      exec_vector_trinary(mach, inst, micro_cnd, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_DP2A:
      exec_dp2a(mach, inst);
      break;

   case TGSI_OPCODE_FRC:
      exec_vector_unary(mach, inst, micro_frc, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_CLAMP:
      exec_vector_trinary(mach, inst, micro_clamp, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_FLR:
      exec_vector_unary(mach, inst, micro_flr, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_ROUND:
      exec_vector_unary(mach, inst, micro_rnd, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_EX2:
      exec_scalar_unary(mach, inst, micro_exp2, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_LG2:
      exec_scalar_unary(mach, inst, micro_lg2, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_POW:
      exec_scalar_binary(mach, inst, micro_pow, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_XPD:
      exec_xpd(mach, inst);
      break;

   case TGSI_OPCODE_ABS:
      exec_vector_unary(mach, inst, micro_abs, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_RCC:
      exec_scalar_unary(mach, inst, micro_rcc, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_DPH:
      exec_dph(mach, inst);
      break;

   case TGSI_OPCODE_COS:
      exec_scalar_unary(mach, inst, micro_cos, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_DDX:
      exec_vector_unary(mach, inst, micro_ddx, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_DDY:
      exec_vector_unary(mach, inst, micro_ddy, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_KILP:
      exec_kilp (mach, inst);
      break;

   case TGSI_OPCODE_KIL:
      exec_kil (mach, inst);
      break;

   case TGSI_OPCODE_PK2H:
      assert (0);
      break;

   case TGSI_OPCODE_PK2US:
      assert (0);
      break;

   case TGSI_OPCODE_PK4B:
      assert (0);
      break;

   case TGSI_OPCODE_PK4UB:
      assert (0);
      break;

   case TGSI_OPCODE_RFL:
      exec_rfl(mach, inst);
      break;

   case TGSI_OPCODE_SEQ:
      exec_vector_binary(mach, inst, micro_seq, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_SFL:
      exec_vector(mach, inst, micro_sfl, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_SGT:
      exec_vector_binary(mach, inst, micro_sgt, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_SIN:
      exec_scalar_unary(mach, inst, micro_sin, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_SLE:
      exec_vector_binary(mach, inst, micro_sle, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_SNE:
      exec_vector_binary(mach, inst, micro_sne, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_STR:
      exec_vector(mach, inst, micro_str, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_TEX:
      /* simple texture lookup */
      /* src[0] = texcoord */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, TEX_MODIFIER_NONE);
      break;

   case TGSI_OPCODE_TXB:
      /* Texture lookup with lod bias */
      /* src[0] = texcoord (src[0].w = LOD bias) */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, TEX_MODIFIER_LOD_BIAS);
      break;

   case TGSI_OPCODE_TXD:
      /* Texture lookup with explict partial derivatives */
      /* src[0] = texcoord */
      /* src[1] = d[strq]/dx */
      /* src[2] = d[strq]/dy */
      /* src[3] = sampler unit */
      exec_txd(mach, inst);
      break;

   case TGSI_OPCODE_TXL:
      /* Texture lookup with explit LOD */
      /* src[0] = texcoord (src[0].w = LOD) */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, TEX_MODIFIER_EXPLICIT_LOD);
      break;

   case TGSI_OPCODE_TXP:
      /* Texture lookup with projection */
      /* src[0] = texcoord (src[0].w = projection) */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, TEX_MODIFIER_PROJECTED);
      break;

   case TGSI_OPCODE_UP2H:
      assert (0);
      break;

   case TGSI_OPCODE_UP2US:
      assert (0);
      break;

   case TGSI_OPCODE_UP4B:
      assert (0);
      break;

   case TGSI_OPCODE_UP4UB:
      assert (0);
      break;

   case TGSI_OPCODE_X2D:
      exec_x2d(mach, inst);
      break;

   case TGSI_OPCODE_ARA:
      assert (0);
      break;

   case TGSI_OPCODE_ARR:
      exec_vector_unary(mach, inst, micro_arr, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_BRA:
      assert (0);
      break;

   case TGSI_OPCODE_CAL:
      /* skip the call if no execution channels are enabled */
      if (mach->ExecMask) {
         /* do the call */

         /* First, record the depths of the execution stacks.
          * This is important for deeply nested/looped return statements.
          * We have to unwind the stacks by the correct amount.  For a
          * real code generator, we could determine the number of entries
          * to pop off each stack with simple static analysis and avoid
          * implementing this data structure at run time.
          */
         mach->CallStack[mach->CallStackTop].CondStackTop = mach->CondStackTop;
         mach->CallStack[mach->CallStackTop].LoopStackTop = mach->LoopStackTop;
         mach->CallStack[mach->CallStackTop].ContStackTop = mach->ContStackTop;
         mach->CallStack[mach->CallStackTop].SwitchStackTop = mach->SwitchStackTop;
         mach->CallStack[mach->CallStackTop].BreakStackTop = mach->BreakStackTop;
         /* note that PC was already incremented above */
         mach->CallStack[mach->CallStackTop].ReturnAddr = *pc;

         mach->CallStackTop++;

         /* Second, push the Cond, Loop, Cont, Func stacks */
         assert(mach->CondStackTop < TGSI_EXEC_MAX_COND_NESTING);
         assert(mach->LoopStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
         assert(mach->ContStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
         assert(mach->SwitchStackTop < TGSI_EXEC_MAX_SWITCH_NESTING);
         assert(mach->BreakStackTop < TGSI_EXEC_MAX_BREAK_STACK);
         assert(mach->FuncStackTop < TGSI_EXEC_MAX_CALL_NESTING);

         mach->CondStack[mach->CondStackTop++] = mach->CondMask;
         mach->LoopStack[mach->LoopStackTop++] = mach->LoopMask;
         mach->ContStack[mach->ContStackTop++] = mach->ContMask;
         mach->SwitchStack[mach->SwitchStackTop++] = mach->Switch;
         mach->BreakStack[mach->BreakStackTop++] = mach->BreakType;
         mach->FuncStack[mach->FuncStackTop++] = mach->FuncMask;

         /* Finally, jump to the subroutine */
         *pc = inst->Label.Label;
      }
      break;

   case TGSI_OPCODE_RET:
      mach->FuncMask &= ~mach->ExecMask;
      UPDATE_EXEC_MASK(mach);

      if (mach->FuncMask == 0x0) {
         /* really return now (otherwise, keep executing */

         if (mach->CallStackTop == 0) {
            /* returning from main() */
            mach->CondStackTop = 0;
            mach->LoopStackTop = 0;
            *pc = -1;
            return;
         }

         assert(mach->CallStackTop > 0);
         mach->CallStackTop--;

         mach->CondStackTop = mach->CallStack[mach->CallStackTop].CondStackTop;
         mach->CondMask = mach->CondStack[mach->CondStackTop];

         mach->LoopStackTop = mach->CallStack[mach->CallStackTop].LoopStackTop;
         mach->LoopMask = mach->LoopStack[mach->LoopStackTop];

         mach->ContStackTop = mach->CallStack[mach->CallStackTop].ContStackTop;
         mach->ContMask = mach->ContStack[mach->ContStackTop];

         mach->SwitchStackTop = mach->CallStack[mach->CallStackTop].SwitchStackTop;
         mach->Switch = mach->SwitchStack[mach->SwitchStackTop];

         mach->BreakStackTop = mach->CallStack[mach->CallStackTop].BreakStackTop;
         mach->BreakType = mach->BreakStack[mach->BreakStackTop];

         assert(mach->FuncStackTop > 0);
         mach->FuncMask = mach->FuncStack[--mach->FuncStackTop];

         *pc = mach->CallStack[mach->CallStackTop].ReturnAddr;

         UPDATE_EXEC_MASK(mach);
      }
      break;

   case TGSI_OPCODE_SSG:
      exec_vector_unary(mach, inst, micro_sgn, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_CMP:
      exec_vector_trinary(mach, inst, micro_cmp, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_SCS:
      exec_scs(mach, inst);
      break;

   case TGSI_OPCODE_NRM:
      exec_nrm3(mach, inst);
      break;

   case TGSI_OPCODE_NRM4:
      exec_nrm4(mach, inst);
      break;

   case TGSI_OPCODE_DIV:
      exec_vector_binary(mach, inst, micro_div, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_DP2:
      exec_dp2(mach, inst);
      break;

   case TGSI_OPCODE_IF:
      /* push CondMask */
      assert(mach->CondStackTop < TGSI_EXEC_MAX_COND_NESTING);
      mach->CondStack[mach->CondStackTop++] = mach->CondMask;
      FETCH( &r[0], 0, CHAN_X );
      /* update CondMask */
      if( ! r[0].u[0] ) {
         mach->CondMask &= ~0x1;
      }
      if( ! r[0].u[1] ) {
         mach->CondMask &= ~0x2;
      }
      if( ! r[0].u[2] ) {
         mach->CondMask &= ~0x4;
      }
      if( ! r[0].u[3] ) {
         mach->CondMask &= ~0x8;
      }
      UPDATE_EXEC_MASK(mach);
      /* Todo: If CondMask==0, jump to ELSE */
      break;

   case TGSI_OPCODE_ELSE:
      /* invert CondMask wrt previous mask */
      {
         uint prevMask;
         assert(mach->CondStackTop > 0);
         prevMask = mach->CondStack[mach->CondStackTop - 1];
         mach->CondMask = ~mach->CondMask & prevMask;
         UPDATE_EXEC_MASK(mach);
         /* Todo: If CondMask==0, jump to ENDIF */
      }
      break;

   case TGSI_OPCODE_ENDIF:
      /* pop CondMask */
      assert(mach->CondStackTop > 0);
      mach->CondMask = mach->CondStack[--mach->CondStackTop];
      UPDATE_EXEC_MASK(mach);
      break;

   case TGSI_OPCODE_END:
      /* make sure we end primitives which haven't
       * been explicitly emitted */
      conditional_emit_primitive(mach);
      /* halt execution */
      *pc = -1;
      break;

   case TGSI_OPCODE_PUSHA:
      assert (0);
      break;

   case TGSI_OPCODE_POPA:
      assert (0);
      break;

   case TGSI_OPCODE_CEIL:
      exec_vector_unary(mach, inst, micro_ceil, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_I2F:
      exec_vector_unary(mach, inst, micro_i2f, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_INT);
      break;

   case TGSI_OPCODE_NOT:
      exec_vector_unary(mach, inst, micro_not, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_TRUNC:
      exec_vector_unary(mach, inst, micro_trunc, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_SHL:
      exec_vector_binary(mach, inst, micro_shl, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_AND:
      exec_vector_binary(mach, inst, micro_and, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_OR:
      exec_vector_binary(mach, inst, micro_or, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_MOD:
      exec_vector_binary(mach, inst, micro_mod, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_INT);
      break;

   case TGSI_OPCODE_XOR:
      exec_vector_binary(mach, inst, micro_xor, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_SAD:
      assert (0);
      break;

   case TGSI_OPCODE_TXF:
      exec_txf(mach, inst);
      break;

   case TGSI_OPCODE_TXQ:
      exec_txq(mach, inst);
      break;

   case TGSI_OPCODE_EMIT:
      emit_vertex(mach);
      break;

   case TGSI_OPCODE_ENDPRIM:
      emit_primitive(mach);
      break;

   case TGSI_OPCODE_BGNLOOP:
      /* push LoopMask and ContMasks */
      assert(mach->LoopStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
      assert(mach->ContStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
      assert(mach->LoopLabelStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
      assert(mach->BreakStackTop < TGSI_EXEC_MAX_BREAK_STACK);

      mach->LoopStack[mach->LoopStackTop++] = mach->LoopMask;
      mach->ContStack[mach->ContStackTop++] = mach->ContMask;
      mach->LoopLabelStack[mach->LoopLabelStackTop++] = *pc - 1;
      mach->BreakStack[mach->BreakStackTop++] = mach->BreakType;
      mach->BreakType = TGSI_EXEC_BREAK_INSIDE_LOOP;
      break;

   case TGSI_OPCODE_ENDLOOP:
      /* Restore ContMask, but don't pop */
      assert(mach->ContStackTop > 0);
      mach->ContMask = mach->ContStack[mach->ContStackTop - 1];
      UPDATE_EXEC_MASK(mach);
      if (mach->ExecMask) {
         /* repeat loop: jump to instruction just past BGNLOOP */
         assert(mach->LoopLabelStackTop > 0);
         *pc = mach->LoopLabelStack[mach->LoopLabelStackTop - 1] + 1;
      }
      else {
         /* exit loop: pop LoopMask */
         assert(mach->LoopStackTop > 0);
         mach->LoopMask = mach->LoopStack[--mach->LoopStackTop];
         /* pop ContMask */
         assert(mach->ContStackTop > 0);
         mach->ContMask = mach->ContStack[--mach->ContStackTop];
         assert(mach->LoopLabelStackTop > 0);
         --mach->LoopLabelStackTop;

         mach->BreakType = mach->BreakStack[--mach->BreakStackTop];
      }
      UPDATE_EXEC_MASK(mach);
      break;

   case TGSI_OPCODE_BRK:
      exec_break(mach);
      break;

   case TGSI_OPCODE_CONT:
      /* turn off cont channels for each enabled exec channel */
      mach->ContMask &= ~mach->ExecMask;
      /* Todo: if mach->LoopMask == 0, jump to end of loop */
      UPDATE_EXEC_MASK(mach);
      break;

   case TGSI_OPCODE_BGNSUB:
      /* no-op */
      break;

   case TGSI_OPCODE_ENDSUB:
      /*
       * XXX: This really should be a no-op. We should never reach this opcode.
       */

      assert(mach->CallStackTop > 0);
      mach->CallStackTop--;

      mach->CondStackTop = mach->CallStack[mach->CallStackTop].CondStackTop;
      mach->CondMask = mach->CondStack[mach->CondStackTop];

      mach->LoopStackTop = mach->CallStack[mach->CallStackTop].LoopStackTop;
      mach->LoopMask = mach->LoopStack[mach->LoopStackTop];

      mach->ContStackTop = mach->CallStack[mach->CallStackTop].ContStackTop;
      mach->ContMask = mach->ContStack[mach->ContStackTop];

      mach->SwitchStackTop = mach->CallStack[mach->CallStackTop].SwitchStackTop;
      mach->Switch = mach->SwitchStack[mach->SwitchStackTop];

      mach->BreakStackTop = mach->CallStack[mach->CallStackTop].BreakStackTop;
      mach->BreakType = mach->BreakStack[mach->BreakStackTop];

      assert(mach->FuncStackTop > 0);
      mach->FuncMask = mach->FuncStack[--mach->FuncStackTop];

      *pc = mach->CallStack[mach->CallStackTop].ReturnAddr;

      UPDATE_EXEC_MASK(mach);
      break;

   case TGSI_OPCODE_NOP:
      break;

   case TGSI_OPCODE_BREAKC:
      FETCH(&r[0], 0, CHAN_X);
      /* update CondMask */
      if (r[0].u[0] && (mach->ExecMask & 0x1)) {
         mach->LoopMask &= ~0x1;
      }
      if (r[0].u[1] && (mach->ExecMask & 0x2)) {
         mach->LoopMask &= ~0x2;
      }
      if (r[0].u[2] && (mach->ExecMask & 0x4)) {
         mach->LoopMask &= ~0x4;
      }
      if (r[0].u[3] && (mach->ExecMask & 0x8)) {
         mach->LoopMask &= ~0x8;
      }
      /* Todo: if mach->LoopMask == 0, jump to end of loop */
      UPDATE_EXEC_MASK(mach);
      break;

   case TGSI_OPCODE_F2I:
      exec_vector_unary(mach, inst, micro_f2i, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_IDIV:
      exec_vector_binary(mach, inst, micro_idiv, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_INT);
      break;

   case TGSI_OPCODE_IMAX:
      exec_vector_binary(mach, inst, micro_imax, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_INT);
      break;

   case TGSI_OPCODE_IMIN:
      exec_vector_binary(mach, inst, micro_imin, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_INT);
      break;

   case TGSI_OPCODE_INEG:
      exec_vector_unary(mach, inst, micro_ineg, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_INT);
      break;

   case TGSI_OPCODE_ISGE:
      exec_vector_binary(mach, inst, micro_isge, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_INT);
      break;

   case TGSI_OPCODE_ISHR:
      exec_vector_binary(mach, inst, micro_ishr, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_INT);
      break;

   case TGSI_OPCODE_ISLT:
      exec_vector_binary(mach, inst, micro_islt, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_INT);
      break;

   case TGSI_OPCODE_F2U:
      exec_vector_unary(mach, inst, micro_f2u, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_FLOAT);
      break;

   case TGSI_OPCODE_U2F:
      exec_vector_unary(mach, inst, micro_u2f, TGSI_EXEC_DATA_FLOAT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_UADD:
      exec_vector_binary(mach, inst, micro_uadd, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_UDIV:
      exec_vector_binary(mach, inst, micro_udiv, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_UMAD:
      exec_vector_trinary(mach, inst, micro_umad, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_UMAX:
      exec_vector_binary(mach, inst, micro_umax, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_UMIN:
      exec_vector_binary(mach, inst, micro_umin, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_UMOD:
      exec_vector_binary(mach, inst, micro_umod, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_UMUL:
      exec_vector_binary(mach, inst, micro_umul, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_USEQ:
      exec_vector_binary(mach, inst, micro_useq, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_USGE:
      exec_vector_binary(mach, inst, micro_usge, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_USHR:
      exec_vector_binary(mach, inst, micro_ushr, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_USLT:
      exec_vector_binary(mach, inst, micro_uslt, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_USNE:
      exec_vector_binary(mach, inst, micro_usne, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_SWITCH:
      exec_switch(mach, inst);
      break;

   case TGSI_OPCODE_CASE:
      exec_case(mach, inst);
      break;

   case TGSI_OPCODE_DEFAULT:
      exec_default(mach);
      break;

   case TGSI_OPCODE_ENDSWITCH:
      exec_endswitch(mach);
      break;

   case TGSI_OPCODE_LOAD:
      assert(0);
      break;

   case TGSI_OPCODE_LOAD_MS:
      assert(0);
      break;

   case TGSI_OPCODE_SAMPLE:
      exec_sample(mach, inst, TEX_MODIFIER_NONE);
      break;

   case TGSI_OPCODE_SAMPLE_B:
      exec_sample(mach, inst, TEX_MODIFIER_LOD_BIAS);
      break;

   case TGSI_OPCODE_SAMPLE_C:
      exec_sample(mach, inst, TEX_MODIFIER_NONE);
      break;

   case TGSI_OPCODE_SAMPLE_C_LZ:
      exec_sample(mach, inst, TEX_MODIFIER_LOD_BIAS);
      break;

   case TGSI_OPCODE_SAMPLE_D:
      exec_sample_d(mach, inst);
      break;

   case TGSI_OPCODE_SAMPLE_L:
      exec_sample(mach, inst, TEX_MODIFIER_EXPLICIT_LOD);
      break;

   case TGSI_OPCODE_GATHER4:
      assert(0);
      break;

   case TGSI_OPCODE_RESINFO:
      assert(0);
      break;

   case TGSI_OPCODE_SAMPLE_POS:
      assert(0);
      break;

   case TGSI_OPCODE_SAMPLE_INFO:
      assert(0);
      break;

   case TGSI_OPCODE_UARL:
      exec_vector_unary(mach, inst, micro_uarl, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_UCMP:
      exec_vector_trinary(mach, inst, micro_ucmp, TGSI_EXEC_DATA_UINT, TGSI_EXEC_DATA_UINT);
      break;

   case TGSI_OPCODE_IABS:
      exec_vector_unary(mach, inst, micro_iabs, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_INT);
      break;

   case TGSI_OPCODE_ISSG:
      exec_vector_unary(mach, inst, micro_isgn, TGSI_EXEC_DATA_INT, TGSI_EXEC_DATA_INT);
      break;

   default:
      assert( 0 );
   }
}


#define DEBUG_EXECUTION 0


/**
 * Run TGSI interpreter.
 * \return bitmask of "alive" quad components
 */
uint
tgsi_exec_machine_run( struct tgsi_exec_machine *mach )
{
   uint i;
   int pc = 0;

   mach->CondMask = 0xf;
   mach->LoopMask = 0xf;
   mach->ContMask = 0xf;
   mach->FuncMask = 0xf;
   mach->ExecMask = 0xf;

   mach->Switch.mask = 0xf;

   assert(mach->CondStackTop == 0);
   assert(mach->LoopStackTop == 0);
   assert(mach->ContStackTop == 0);
   assert(mach->SwitchStackTop == 0);
   assert(mach->BreakStackTop == 0);
   assert(mach->CallStackTop == 0);

   mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0] = 0;
   mach->Temps[TEMP_OUTPUT_I].xyzw[TEMP_OUTPUT_C].u[0] = 0;

   if( mach->Processor == TGSI_PROCESSOR_GEOMETRY ) {
      mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0] = 0;
      mach->Primitives[0] = 0;
   }

   /* execute declarations (interpolants) */
   for (i = 0; i < mach->NumDeclarations; i++) {
      exec_declaration( mach, mach->Declarations+i );
   }

   {
#if DEBUG_EXECUTION
      struct tgsi_exec_vector temps[TGSI_EXEC_NUM_TEMPS + TGSI_EXEC_NUM_TEMP_EXTRAS];
      struct tgsi_exec_vector outputs[PIPE_MAX_ATTRIBS];
      uint inst = 1;

      memcpy(temps, mach->Temps, sizeof(temps));
      memcpy(outputs, mach->Outputs, sizeof(outputs));
#endif

      /* execute instructions, until pc is set to -1 */
      while (pc != -1) {

#if DEBUG_EXECUTION
         uint i;

         tgsi_dump_instruction(&mach->Instructions[pc], inst++);
#endif

         assert(pc < (int) mach->NumInstructions);
         exec_instruction(mach, mach->Instructions + pc, &pc);

#if DEBUG_EXECUTION
         for (i = 0; i < TGSI_EXEC_NUM_TEMPS + TGSI_EXEC_NUM_TEMP_EXTRAS; i++) {
            if (memcmp(&temps[i], &mach->Temps[i], sizeof(temps[i]))) {
               uint j;

               memcpy(&temps[i], &mach->Temps[i], sizeof(temps[i]));
               debug_printf("TEMP[%2u] = ", i);
               for (j = 0; j < 4; j++) {
                  if (j > 0) {
                     debug_printf("           ");
                  }
                  debug_printf("(%6f %u, %6f %u, %6f %u, %6f %u)\n",
                               temps[i].xyzw[0].f[j], temps[i].xyzw[0].u[j],
                               temps[i].xyzw[1].f[j], temps[i].xyzw[1].u[j],
                               temps[i].xyzw[2].f[j], temps[i].xyzw[2].u[j],
                               temps[i].xyzw[3].f[j], temps[i].xyzw[3].u[j]);
               }
            }
         }
         for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
            if (memcmp(&outputs[i], &mach->Outputs[i], sizeof(outputs[i]))) {
               uint j;

               memcpy(&outputs[i], &mach->Outputs[i], sizeof(outputs[i]));
               debug_printf("OUT[%2u] =  ", i);
               for (j = 0; j < 4; j++) {
                  if (j > 0) {
                     debug_printf("           ");
                  }
                  debug_printf("(%6f %u, %6f %u, %6f %u, %6f %u)\n",
                               outputs[i].xyzw[0].f[j], outputs[i].xyzw[0].u[j],
                               outputs[i].xyzw[1].f[j], outputs[i].xyzw[1].u[j],
                               outputs[i].xyzw[2].f[j], outputs[i].xyzw[2].u[j],
                               outputs[i].xyzw[3].f[j], outputs[i].xyzw[3].u[j]);
               }
            }
         }
#endif
      }
   }

#if 0
   /* we scale from floats in [0,1] to Zbuffer ints in sp_quad_depth_test.c */
   if (mach->Processor == TGSI_PROCESSOR_FRAGMENT) {
      /*
       * Scale back depth component.
       */
      for (i = 0; i < 4; i++)
         mach->Outputs[0].xyzw[2].f[i] *= ctx->DrawBuffer->_DepthMaxF;
   }
#endif

   /* Strictly speaking, these assertions aren't really needed but they
    * can potentially catch some bugs in the control flow code.
    */
   assert(mach->CondStackTop == 0);
   assert(mach->LoopStackTop == 0);
   assert(mach->ContStackTop == 0);
   assert(mach->SwitchStackTop == 0);
   assert(mach->BreakStackTop == 0);
   assert(mach->CallStackTop == 0);

   return ~mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0];
}
