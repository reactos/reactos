/*
 * Copyright 2003 Tungsten Graphics, inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@tungstengraphics.com>
 */


#include "pipe/p_config.h"
#include "pipe/p_compiler.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"

#include "translate.h"


#if (defined(PIPE_ARCH_X86) || (defined(PIPE_ARCH_X86_64) && !defined(__MINGW32__))) && !defined(PIPE_SUBSYSTEM_EMBEDDED)

#include "rtasm/rtasm_cpu.h"
#include "rtasm/rtasm_x86sse.h"


#define X    0
#define Y    1
#define Z    2
#define W    3


struct translate_buffer {
   const void *base_ptr;
   uintptr_t stride;
   unsigned max_index;
};

struct translate_buffer_variant {
   unsigned buffer_index;
   unsigned instance_divisor;
   void *ptr;                    /* updated either per vertex or per instance */
};


#define ELEMENT_BUFFER_INSTANCE_ID  1001

#define NUM_CONSTS 7

enum
{
   CONST_IDENTITY,
   CONST_INV_127,
   CONST_INV_255,
   CONST_INV_32767,
   CONST_INV_65535,
   CONST_INV_2147483647,
   CONST_255
};

#define C(v) {(float)(v), (float)(v), (float)(v), (float)(v)}
static float consts[NUM_CONSTS][4] = {
      {0, 0, 0, 1},
      C(1.0 / 127.0),
      C(1.0 / 255.0),
      C(1.0 / 32767.0),
      C(1.0 / 65535.0),
      C(1.0 / 2147483647.0),
      C(255.0)
};
#undef C

struct translate_sse {
   struct translate translate;

   struct x86_function linear_func;
   struct x86_function elt_func;
   struct x86_function elt16_func;
   struct x86_function elt8_func;
   struct x86_function *func;

   PIPE_ALIGN_VAR(16) float consts[NUM_CONSTS][4];
   int8_t reg_to_const[16];
   int8_t const_to_reg[NUM_CONSTS];

   struct translate_buffer buffer[PIPE_MAX_ATTRIBS];
   unsigned nr_buffers;

   /* Multiple buffer variants can map to a single buffer. */
   struct translate_buffer_variant buffer_variant[PIPE_MAX_ATTRIBS];
   unsigned nr_buffer_variants;

   /* Multiple elements can map to a single buffer variant. */
   unsigned element_to_buffer_variant[PIPE_MAX_ATTRIBS];

   boolean use_instancing;
   unsigned instance_id;

   /* these are actually known values, but putting them in a struct
    * like this is helpful to keep them in sync across the file.
    */
   struct x86_reg tmp_EAX;
   struct x86_reg tmp2_EDX;
   struct x86_reg src_ECX;
   struct x86_reg idx_ESI;     /* either start+i or &elt[i] */
   struct x86_reg machine_EDI;
   struct x86_reg outbuf_EBX;
   struct x86_reg count_EBP;    /* decrements to zero */
};

static int get_offset( const void *a, const void *b )
{
   return (const char *)b - (const char *)a;
}

static struct x86_reg get_const( struct translate_sse *p, unsigned id)
{
   struct x86_reg reg;
   unsigned i;

   if(p->const_to_reg[id] >= 0)
      return x86_make_reg(file_XMM, p->const_to_reg[id]);

   for(i = 2; i < 8; ++i)
   {
      if(p->reg_to_const[i] < 0)
         break;
   }

   /* TODO: be smarter here */
   if(i == 8)
      --i;

   reg = x86_make_reg(file_XMM, i);

   if(p->reg_to_const[i] >= 0)
      p->const_to_reg[p->reg_to_const[i]] = -1;

   p->reg_to_const[i] = id;
   p->const_to_reg[id] = i;

   /* TODO: this should happen outside the loop, if possible */
   sse_movaps(p->func, reg,
         x86_make_disp(p->machine_EDI,
               get_offset(p, &p->consts[id][0])));

   return reg;
}

/* load the data in a SSE2 register, padding with zeros */
static boolean emit_load_sse2( struct translate_sse *p,
				       struct x86_reg data,
				       struct x86_reg src,
				       unsigned size)
{
   struct x86_reg tmpXMM = x86_make_reg(file_XMM, 1);
   struct x86_reg tmp = p->tmp_EAX;
   switch(size)
   {
   case 1:
      x86_movzx8(p->func, tmp, src);
      sse2_movd(p->func, data, tmp);
      break;
   case 2:
      x86_movzx16(p->func, tmp, src);
      sse2_movd(p->func, data, tmp);
      break;
   case 3:
      x86_movzx8(p->func, tmp, x86_make_disp(src, 2));
      x86_shl_imm(p->func, tmp, 16);
      x86_mov16(p->func, tmp, src);
      sse2_movd(p->func, data, tmp);
      break;
   case 4:
      sse2_movd(p->func, data, src);
      break;
   case 6:
      sse2_movd(p->func, data, src);
      x86_movzx16(p->func, tmp, x86_make_disp(src, 4));
      sse2_movd(p->func, tmpXMM, tmp);
      sse2_punpckldq(p->func, data, tmpXMM);
      break;
   case 8:
      sse2_movq(p->func, data, src);
      break;
   case 12:
      sse2_movq(p->func, data, src);
      sse2_movd(p->func, tmpXMM, x86_make_disp(src, 8));
      sse2_punpcklqdq(p->func, data, tmpXMM);
      break;
   case 16:
      sse2_movdqu(p->func, data, src);
      break;
   default:
      return FALSE;
   }
   return TRUE;
}

/* this value can be passed for the out_chans argument */
#define CHANNELS_0001 5

/* this function will load #chans float values, and will
 * pad the register with zeroes at least up to out_chans.
 *
 * If out_chans is set to CHANNELS_0001, then the fourth
 * value will be padded with 1. Only pass this value if
 * chans < 4 or results are undefined.
 */
static void emit_load_float32( struct translate_sse *p,
                                       struct x86_reg data,
                                       struct x86_reg arg0,
                                       unsigned out_chans,
                                       unsigned chans)
{
   switch(chans)
   {
   case 1:
      /* a 0 0 0
       * a 0 0 1
       */
      sse_movss(p->func, data, arg0);
      if(out_chans == CHANNELS_0001)
         sse_orps(p->func, data, get_const(p, CONST_IDENTITY) );
      break;
   case 2:
      /* 0 0 0 1
       * a b 0 1
       */
      if(out_chans == CHANNELS_0001)
         sse_shufps(p->func, data, get_const(p, CONST_IDENTITY), SHUF(X, Y, Z, W) );
      else if(out_chans > 2)
         sse_movlhps(p->func, data, get_const(p, CONST_IDENTITY) );
      sse_movlps(p->func, data, arg0);
      break;
   case 3:
      /* Have to jump through some hoops:
       *
       * c 0 0 0
       * c 0 0 1 if out_chans == CHANNELS_0001
       * 0 0 c 0/1
       * a b c 0/1
       */
      sse_movss(p->func, data, x86_make_disp(arg0, 8));
      if(out_chans == CHANNELS_0001)
         sse_shufps(p->func, data, get_const(p, CONST_IDENTITY), SHUF(X,Y,Z,W) );
      sse_shufps(p->func, data, data, SHUF(Y,Z,X,W) );
      sse_movlps(p->func, data, arg0);
      break;
   case 4:
      sse_movups(p->func, data, arg0);
      break;
   }
}

/* this function behaves like emit_load_float32, but loads
   64-bit floating point numbers, converting them to 32-bit
  ones */
static void emit_load_float64to32( struct translate_sse *p,
                                       struct x86_reg data,
                                       struct x86_reg arg0,
                                       unsigned out_chans,
                                       unsigned chans)
{
   struct x86_reg tmpXMM = x86_make_reg(file_XMM, 1);
   switch(chans)
   {
   case 1:
      sse2_movsd(p->func, data, arg0);
      if(out_chans > 1)
         sse2_cvtpd2ps(p->func, data, data);
      else
         sse2_cvtsd2ss(p->func, data, data);
      if(out_chans == CHANNELS_0001)
         sse_shufps(p->func, data, get_const(p, CONST_IDENTITY), SHUF(X, Y, Z, W)  );
      break;
   case 2:
      sse2_movupd(p->func, data, arg0);
      sse2_cvtpd2ps(p->func, data, data);
      if(out_chans == CHANNELS_0001)
         sse_shufps(p->func, data, get_const(p, CONST_IDENTITY), SHUF(X, Y, Z, W) );
      else if(out_chans > 2)
         sse_movlhps(p->func, data, get_const(p, CONST_IDENTITY) );
       break;
   case 3:
      sse2_movupd(p->func, data, arg0);
      sse2_cvtpd2ps(p->func, data, data);
      sse2_movsd(p->func, tmpXMM, x86_make_disp(arg0, 16));
      if(out_chans > 3)
         sse2_cvtpd2ps(p->func, tmpXMM, tmpXMM);
      else
         sse2_cvtsd2ss(p->func, tmpXMM, tmpXMM);
      sse_movlhps(p->func, data, tmpXMM);
      if(out_chans == CHANNELS_0001)
         sse_orps(p->func, data, get_const(p, CONST_IDENTITY) );
      break;
   case 4:
      sse2_movupd(p->func, data, arg0);
      sse2_cvtpd2ps(p->func, data, data);
      sse2_movupd(p->func, tmpXMM, x86_make_disp(arg0, 16));
      sse2_cvtpd2ps(p->func, tmpXMM, tmpXMM);
      sse_movlhps(p->func, data, tmpXMM);
      break;
   }
}

static void emit_mov64(struct translate_sse *p, struct x86_reg dst_gpr, struct x86_reg dst_xmm, struct x86_reg src_gpr,  struct x86_reg src_xmm)
{
   if(x86_target(p->func) != X86_32)
      x64_mov64(p->func, dst_gpr, src_gpr);
   else
   {
      /* TODO: when/on which CPUs is SSE2 actually better than SSE? */
      if(x86_target_caps(p->func) & X86_SSE2)
         sse2_movq(p->func, dst_xmm, src_xmm);
      else
         sse_movlps(p->func, dst_xmm, src_xmm);
   }
}

static void emit_load64(struct translate_sse *p, struct x86_reg dst_gpr, struct x86_reg dst_xmm, struct x86_reg src)
{
   emit_mov64(p, dst_gpr, dst_xmm, src, src);
}

static void emit_store64(struct translate_sse *p, struct x86_reg dst, struct x86_reg src_gpr, struct x86_reg src_xmm)
{
   emit_mov64(p, dst, dst, src_gpr, src_xmm);
}

static void emit_mov128(struct translate_sse *p, struct x86_reg dst, struct x86_reg src)
{
   if(x86_target_caps(p->func) & X86_SSE2)
      sse2_movdqu(p->func, dst, src);
   else
      sse_movups(p->func, dst, src);
}

/* TODO: this uses unaligned accesses liberally, which is great on Nehalem,
 * but may or may not be good on older processors
 * TODO: may perhaps want to use non-temporal stores here if possible
 */
static void emit_memcpy(struct translate_sse *p, struct x86_reg dst, struct x86_reg src, unsigned size)
{
   struct x86_reg dataXMM = x86_make_reg(file_XMM, 0);
   struct x86_reg dataXMM2 = x86_make_reg(file_XMM, 1);
   struct x86_reg dataGPR = p->tmp_EAX;
   struct x86_reg dataGPR2 = p->tmp2_EDX;

   if(size < 8)
   {
      switch (size)
      {
      case 1:
         x86_mov8(p->func, dataGPR, src);
         x86_mov8(p->func, dst, dataGPR);
         break;
      case 2:
         x86_mov16(p->func, dataGPR, src);
         x86_mov16(p->func, dst, dataGPR);
         break;
      case 3:
         x86_mov16(p->func, dataGPR, src);
         x86_mov8(p->func, dataGPR2, x86_make_disp(src, 2));
         x86_mov16(p->func, dst, dataGPR);
         x86_mov8(p->func, x86_make_disp(dst, 2), dataGPR2);
         break;
      case 4:
         x86_mov(p->func, dataGPR, src);
         x86_mov(p->func, dst, dataGPR);
         break;
      case 6:
         x86_mov(p->func, dataGPR, src);
         x86_mov16(p->func, dataGPR2, x86_make_disp(src, 4));
         x86_mov(p->func, dst, dataGPR);
         x86_mov16(p->func, x86_make_disp(dst, 4), dataGPR2);
         break;
      }
   }
   else if(!(x86_target_caps(p->func) & X86_SSE))
   {
      unsigned i = 0;
      assert((size & 3) == 0);
      for(i = 0; i < size; i += 4)
      {
         x86_mov(p->func, dataGPR, x86_make_disp(src, i));
         x86_mov(p->func, x86_make_disp(dst, i), dataGPR);
      }
   }
   else
   {
      switch(size)
      {
      case 8:
         emit_load64(p, dataGPR, dataXMM, src);
         emit_store64(p, dst, dataGPR, dataXMM);
         break;
      case 12:
         emit_load64(p, dataGPR2, dataXMM, src);
         x86_mov(p->func, dataGPR, x86_make_disp(src, 8));
         emit_store64(p, dst, dataGPR2, dataXMM);
         x86_mov(p->func, x86_make_disp(dst, 8), dataGPR);
         break;
      case 16:
         emit_mov128(p, dataXMM, src);
         emit_mov128(p, dst, dataXMM);
         break;
      case 24:
         emit_mov128(p, dataXMM, src);
         emit_load64(p, dataGPR, dataXMM2, x86_make_disp(src, 16));
         emit_mov128(p, dst, dataXMM);
         emit_store64(p, x86_make_disp(dst, 16), dataGPR, dataXMM2);
         break;
      case 32:
         emit_mov128(p, dataXMM, src);
         emit_mov128(p, dataXMM2, x86_make_disp(src, 16));
         emit_mov128(p, dst, dataXMM);
         emit_mov128(p, x86_make_disp(dst, 16), dataXMM2);
         break;
      default:
         assert(0);
      }
   }
}

static boolean translate_attr_convert( struct translate_sse *p,
                               const struct translate_element *a,
                               struct x86_reg src,
                               struct x86_reg dst)

{
   const struct util_format_description* input_desc = util_format_description(a->input_format);
   const struct util_format_description* output_desc = util_format_description(a->output_format);
   unsigned i;
   boolean id_swizzle = TRUE;
   unsigned swizzle[4] = {UTIL_FORMAT_SWIZZLE_NONE, UTIL_FORMAT_SWIZZLE_NONE, UTIL_FORMAT_SWIZZLE_NONE, UTIL_FORMAT_SWIZZLE_NONE};
   unsigned needed_chans = 0;
   unsigned imms[2] = {0, 0x3f800000};

   if(a->output_format == PIPE_FORMAT_NONE || a->input_format == PIPE_FORMAT_NONE)
      return FALSE;

   if(input_desc->channel[0].size & 7)
      return FALSE;

   if(input_desc->colorspace != output_desc->colorspace)
      return FALSE;

   for(i = 1; i < input_desc->nr_channels; ++i)
   {
      if(memcmp(&input_desc->channel[i], &input_desc->channel[0], sizeof(input_desc->channel[0])))
         return FALSE;
   }

   for(i = 1; i < output_desc->nr_channels; ++i)
   {
      if(memcmp(&output_desc->channel[i], &output_desc->channel[0], sizeof(output_desc->channel[0])))
         return FALSE;
   }

   for(i = 0; i < output_desc->nr_channels; ++i)
   {
      if(output_desc->swizzle[i] < 4)
         swizzle[output_desc->swizzle[i]] = input_desc->swizzle[i];
   }

   if((x86_target_caps(p->func) & X86_SSE) && (0
         || a->output_format == PIPE_FORMAT_R32_FLOAT
         || a->output_format == PIPE_FORMAT_R32G32_FLOAT
         || a->output_format == PIPE_FORMAT_R32G32B32_FLOAT
         || a->output_format == PIPE_FORMAT_R32G32B32A32_FLOAT))
   {
      struct x86_reg dataXMM = x86_make_reg(file_XMM, 0);

      for(i = 0; i < output_desc->nr_channels; ++i)
      {
         if(swizzle[i] == UTIL_FORMAT_SWIZZLE_0 && i >= input_desc->nr_channels)
            swizzle[i] = i;
      }

      for(i = 0; i < output_desc->nr_channels; ++i)
      {
         if(swizzle[i] < 4)
            needed_chans = MAX2(needed_chans, swizzle[i] + 1);
         if(swizzle[i] < UTIL_FORMAT_SWIZZLE_0 && swizzle[i] != i)
            id_swizzle = FALSE;
      }

      if(needed_chans > 0)
      {
         switch(input_desc->channel[0].type)
         {
         case UTIL_FORMAT_TYPE_UNSIGNED:
            if(!(x86_target_caps(p->func) & X86_SSE2))
               return FALSE;
            emit_load_sse2(p, dataXMM, src, input_desc->channel[0].size * input_desc->nr_channels >> 3);

            /* TODO: add support for SSE4.1 pmovzx */
            switch(input_desc->channel[0].size)
            {
            case 8:
               /* TODO: this may be inefficient due to get_identity() being used both as a float and integer register */
               sse2_punpcklbw(p->func, dataXMM, get_const(p, CONST_IDENTITY));
               sse2_punpcklbw(p->func, dataXMM, get_const(p, CONST_IDENTITY));
               break;
            case 16:
               sse2_punpcklwd(p->func, dataXMM, get_const(p, CONST_IDENTITY));
               break;
            case 32: /* we lose precision here */
               sse2_psrld_imm(p->func, dataXMM, 1);
               break;
            default:
               return FALSE;
            }
            sse2_cvtdq2ps(p->func, dataXMM, dataXMM);
            if(input_desc->channel[0].normalized)
            {
               struct x86_reg factor;
               switch(input_desc->channel[0].size)
               {
               case 8:
                  factor = get_const(p, CONST_INV_255);
                  break;
               case 16:
                  factor = get_const(p, CONST_INV_65535);
                  break;
               case 32:
                  factor = get_const(p, CONST_INV_2147483647);
                  break;
               default:
                  assert(0);
                  factor.disp = 0;
                  factor.file = 0;
                  factor.idx = 0;
                  factor.mod = 0;
                  break;
               }
               sse_mulps(p->func, dataXMM, factor);
            }
            else if(input_desc->channel[0].size == 32)
               sse_addps(p->func, dataXMM, dataXMM); /* compensate for the bit we threw away to fit u32 into s32 */
            break;
         case UTIL_FORMAT_TYPE_SIGNED:
            if(!(x86_target_caps(p->func) & X86_SSE2))
               return FALSE;
            emit_load_sse2(p, dataXMM, src, input_desc->channel[0].size * input_desc->nr_channels >> 3);

            /* TODO: add support for SSE4.1 pmovsx */
            switch(input_desc->channel[0].size)
            {
            case 8:
               sse2_punpcklbw(p->func, dataXMM, dataXMM);
               sse2_punpcklbw(p->func, dataXMM, dataXMM);
               sse2_psrad_imm(p->func, dataXMM, 24);
               break;
            case 16:
               sse2_punpcklwd(p->func, dataXMM, dataXMM);
               sse2_psrad_imm(p->func, dataXMM, 16);
               break;
            case 32: /* we lose precision here */
               break;
            default:
               return FALSE;
            }
            sse2_cvtdq2ps(p->func, dataXMM, dataXMM);
            if(input_desc->channel[0].normalized)
            {
               struct x86_reg factor;
               switch(input_desc->channel[0].size)
               {
               case 8:
                  factor = get_const(p, CONST_INV_127);
                  break;
               case 16:
                  factor = get_const(p, CONST_INV_32767);
                  break;
               case 32:
                  factor = get_const(p, CONST_INV_2147483647);
                  break;
               default:
                  assert(0);
                  factor.disp = 0;
                  factor.file = 0;
                  factor.idx = 0;
                  factor.mod = 0;
                  break;
               }
               sse_mulps(p->func, dataXMM, factor);
            }
            break;

            break;
         case UTIL_FORMAT_TYPE_FLOAT:
            if(input_desc->channel[0].size != 32 && input_desc->channel[0].size != 64)
               return FALSE;
            if(swizzle[3] == UTIL_FORMAT_SWIZZLE_1 && input_desc->nr_channels <= 3)
            {
               swizzle[3] = UTIL_FORMAT_SWIZZLE_W;
               needed_chans = CHANNELS_0001;
            }
            switch(input_desc->channel[0].size)
            {
            case 32:
               emit_load_float32(p, dataXMM, src, needed_chans, input_desc->nr_channels);
               break;
            case 64: /* we lose precision here */
               if(!(x86_target_caps(p->func) & X86_SSE2))
                  return FALSE;
               emit_load_float64to32(p, dataXMM, src, needed_chans, input_desc->nr_channels);
               break;
            default:
               return FALSE;
            }
            break;
         default:
            return FALSE;
         }

         if(!id_swizzle)
            sse_shufps(p->func, dataXMM, dataXMM, SHUF(swizzle[0], swizzle[1], swizzle[2], swizzle[3]) );
      }

      if(output_desc->nr_channels >= 4
            && swizzle[0] < UTIL_FORMAT_SWIZZLE_0
            && swizzle[1] < UTIL_FORMAT_SWIZZLE_0
            && swizzle[2] < UTIL_FORMAT_SWIZZLE_0
            && swizzle[3] < UTIL_FORMAT_SWIZZLE_0
            )
         sse_movups(p->func, dst, dataXMM);
      else
      {
         if(output_desc->nr_channels >= 2
               && swizzle[0] < UTIL_FORMAT_SWIZZLE_0
               && swizzle[1] < UTIL_FORMAT_SWIZZLE_0)
            sse_movlps(p->func, dst, dataXMM);
         else
         {
            if(swizzle[0] < UTIL_FORMAT_SWIZZLE_0)
               sse_movss(p->func, dst, dataXMM);
            else
               x86_mov_imm(p->func, dst, imms[swizzle[0] - UTIL_FORMAT_SWIZZLE_0]);

            if(output_desc->nr_channels >= 2)
            {
               if(swizzle[1] < UTIL_FORMAT_SWIZZLE_0)
               {
                  sse_shufps(p->func, dataXMM, dataXMM, SHUF(1, 1, 2, 3));
                  sse_movss(p->func, x86_make_disp(dst, 4), dataXMM);
               }
               else
                  x86_mov_imm(p->func, x86_make_disp(dst, 4), imms[swizzle[1] - UTIL_FORMAT_SWIZZLE_0]);
            }
         }

         if(output_desc->nr_channels >= 3)
         {
            if(output_desc->nr_channels >= 4
                  && swizzle[2] < UTIL_FORMAT_SWIZZLE_0
                  && swizzle[3] < UTIL_FORMAT_SWIZZLE_0)
               sse_movhps(p->func, x86_make_disp(dst, 8), dataXMM);
            else
            {
               if(swizzle[2] < UTIL_FORMAT_SWIZZLE_0)
               {
                  sse_shufps(p->func, dataXMM, dataXMM, SHUF(2, 2, 2, 3));
                  sse_movss(p->func, x86_make_disp(dst, 8), dataXMM);
               }
               else
                  x86_mov_imm(p->func, x86_make_disp(dst, 8), imms[swizzle[2] - UTIL_FORMAT_SWIZZLE_0]);

               if(output_desc->nr_channels >= 4)
               {
                  if(swizzle[3] < UTIL_FORMAT_SWIZZLE_0)
                  {
                     sse_shufps(p->func, dataXMM, dataXMM, SHUF(3, 3, 3, 3));
                     sse_movss(p->func, x86_make_disp(dst, 12), dataXMM);
                  }
                  else
                     x86_mov_imm(p->func, x86_make_disp(dst, 12), imms[swizzle[3] - UTIL_FORMAT_SWIZZLE_0]);
               }
            }
         }
      }
      return TRUE;
   }
   else if((x86_target_caps(p->func) & X86_SSE2) && input_desc->channel[0].size == 8 && output_desc->channel[0].size == 16
         && output_desc->channel[0].normalized == input_desc->channel[0].normalized
         && (0
               || (input_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED && output_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED)
               || (input_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED && output_desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED)
               || (input_desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED && output_desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED)
               ))
   {
      struct x86_reg dataXMM = x86_make_reg(file_XMM, 0);
      struct x86_reg tmpXMM = x86_make_reg(file_XMM, 1);
      struct x86_reg tmp = p->tmp_EAX;
      unsigned imms[2] = {0, 1};

      for(i = 0; i < output_desc->nr_channels; ++i)
      {
         if(swizzle[i] == UTIL_FORMAT_SWIZZLE_0 && i >= input_desc->nr_channels)
            swizzle[i] = i;
      }

      for(i = 0; i < output_desc->nr_channels; ++i)
      {
         if(swizzle[i] < 4)
            needed_chans = MAX2(needed_chans, swizzle[i] + 1);
         if(swizzle[i] < UTIL_FORMAT_SWIZZLE_0 && swizzle[i] != i)
            id_swizzle = FALSE;
      }

      if(needed_chans > 0)
      {
         emit_load_sse2(p, dataXMM, src, input_desc->channel[0].size * input_desc->nr_channels >> 3);

         switch(input_desc->channel[0].type)
         {
         case UTIL_FORMAT_TYPE_UNSIGNED:
            if(input_desc->channel[0].normalized)
            {
               sse2_punpcklbw(p->func, dataXMM, dataXMM);
               if(output_desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED)
        	       sse2_psrlw_imm(p->func, dataXMM, 1);
            }
            else
               sse2_punpcklbw(p->func, dataXMM, get_const(p, CONST_IDENTITY));
            break;
         case UTIL_FORMAT_TYPE_SIGNED:
            if(input_desc->channel[0].normalized)
            {
               sse2_movq(p->func, tmpXMM, get_const(p, CONST_IDENTITY));
               sse2_punpcklbw(p->func, tmpXMM, dataXMM);
               sse2_psllw_imm(p->func, dataXMM, 9);
               sse2_psrlw_imm(p->func, dataXMM, 8);
               sse2_por(p->func, tmpXMM, dataXMM);
               sse2_psrlw_imm(p->func, dataXMM, 7);
               sse2_por(p->func, tmpXMM, dataXMM);
               {
                  struct x86_reg t = dataXMM;
                  dataXMM = tmpXMM;
                  tmpXMM = t;
               }
            }
            else
            {
               sse2_punpcklbw(p->func, dataXMM, dataXMM);
               sse2_psraw_imm(p->func, dataXMM, 8);
            }
            break;
         default:
            assert(0);
         }

         if(output_desc->channel[0].normalized)
            imms[1] = (output_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED) ? 0xffff : 0x7ffff;

         if(!id_swizzle)
            sse2_pshuflw(p->func, dataXMM, dataXMM, (swizzle[0] & 3) | ((swizzle[1] & 3) << 2) | ((swizzle[2] & 3) << 4) | ((swizzle[3] & 3) << 6));
      }

      if(output_desc->nr_channels >= 4
            && swizzle[0] < UTIL_FORMAT_SWIZZLE_0
            && swizzle[1] < UTIL_FORMAT_SWIZZLE_0
            && swizzle[2] < UTIL_FORMAT_SWIZZLE_0
            && swizzle[3] < UTIL_FORMAT_SWIZZLE_0
            )
         sse2_movq(p->func, dst, dataXMM);
      else
      {
         if(swizzle[0] < UTIL_FORMAT_SWIZZLE_0)
         {
            if(output_desc->nr_channels >= 2 && swizzle[1] < UTIL_FORMAT_SWIZZLE_0)
               sse2_movd(p->func, dst, dataXMM);
            else
            {
               sse2_movd(p->func, tmp, dataXMM);
               x86_mov16(p->func, dst, tmp);
               if(output_desc->nr_channels >= 2)
                  x86_mov16_imm(p->func, x86_make_disp(dst, 2), imms[swizzle[1] - UTIL_FORMAT_SWIZZLE_0]);
            }
         }
         else
         {
            if(output_desc->nr_channels >= 2 && swizzle[1] >= UTIL_FORMAT_SWIZZLE_0)
               x86_mov_imm(p->func, dst, (imms[swizzle[1] - UTIL_FORMAT_SWIZZLE_0] << 16) | imms[swizzle[0] - UTIL_FORMAT_SWIZZLE_0]);
            else
            {
               x86_mov16_imm(p->func, dst, imms[swizzle[0] - UTIL_FORMAT_SWIZZLE_0]);
               if(output_desc->nr_channels >= 2)
               {
                  sse2_movd(p->func, tmp, dataXMM);
                  x86_shr_imm(p->func, tmp, 16);
                  x86_mov16(p->func, x86_make_disp(dst, 2), tmp);
               }
            }
         }

         if(output_desc->nr_channels >= 3)
         {
            if(swizzle[2] < UTIL_FORMAT_SWIZZLE_0)
            {
               if(output_desc->nr_channels >= 4 && swizzle[3] < UTIL_FORMAT_SWIZZLE_0)
               {
                  sse2_psrlq_imm(p->func, dataXMM, 32);
                  sse2_movd(p->func, x86_make_disp(dst, 4), dataXMM);
               }
               else
               {
                  sse2_psrlq_imm(p->func, dataXMM, 32);
                  sse2_movd(p->func, tmp, dataXMM);
                  x86_mov16(p->func, x86_make_disp(dst, 4), tmp);
                  if(output_desc->nr_channels >= 4)
                  {
                     x86_mov16_imm(p->func, x86_make_disp(dst, 6), imms[swizzle[3] - UTIL_FORMAT_SWIZZLE_0]);
                  }
               }
            }
            else
            {
               if(output_desc->nr_channels >= 4 && swizzle[3] >= UTIL_FORMAT_SWIZZLE_0)
                  x86_mov_imm(p->func, x86_make_disp(dst, 4), (imms[swizzle[3] - UTIL_FORMAT_SWIZZLE_0] << 16) | imms[swizzle[2] - UTIL_FORMAT_SWIZZLE_0]);
               else
               {
                  x86_mov16_imm(p->func, x86_make_disp(dst, 4), imms[swizzle[2] - UTIL_FORMAT_SWIZZLE_0]);

                  if(output_desc->nr_channels >= 4)
                  {
                     sse2_psrlq_imm(p->func, dataXMM, 48);
                     sse2_movd(p->func, tmp, dataXMM);
                     x86_mov16(p->func, x86_make_disp(dst, 6), tmp);
                  }
               }
            }
         }
      }
      return TRUE;
   }
   else if(!memcmp(&output_desc->channel[0], &input_desc->channel[0], sizeof(output_desc->channel[0])))
   {
      struct x86_reg tmp = p->tmp_EAX;
      unsigned i;
      if(input_desc->channel[0].size == 8 && input_desc->nr_channels == 4 && output_desc->nr_channels == 4
                     && swizzle[0] == UTIL_FORMAT_SWIZZLE_W
                     && swizzle[1] == UTIL_FORMAT_SWIZZLE_Z
                     && swizzle[2] == UTIL_FORMAT_SWIZZLE_Y
                     && swizzle[3] == UTIL_FORMAT_SWIZZLE_X)
      {
         /* TODO: support movbe */
         x86_mov(p->func, tmp, src);
         x86_bswap(p->func, tmp);
         x86_mov(p->func, dst, tmp);
         return TRUE;
      }

      for(i = 0; i < output_desc->nr_channels; ++i)
      {
         switch(output_desc->channel[0].size)
         {
         case 8:
            if(swizzle[i] >= UTIL_FORMAT_SWIZZLE_0)
            {
               unsigned v = 0;
               if(swizzle[i] == UTIL_FORMAT_SWIZZLE_1)
               {
                  switch(output_desc->channel[0].type)
                  {
                  case UTIL_FORMAT_TYPE_UNSIGNED:
                     v = output_desc->channel[0].normalized ? 0xff : 1;
                     break;
                  case UTIL_FORMAT_TYPE_SIGNED:
                     v = output_desc->channel[0].normalized ? 0x7f : 1;
                     break;
                  default:
                     return FALSE;
                  }
               }
               x86_mov8_imm(p->func, x86_make_disp(dst, i * 1), v);
            }
            else
            {
               x86_mov8(p->func, tmp, x86_make_disp(src, swizzle[i] * 1));
               x86_mov8(p->func, x86_make_disp(dst, i * 1), tmp);
            }
            break;
         case 16:
            if(swizzle[i] >= UTIL_FORMAT_SWIZZLE_0)
            {
               unsigned v = 0;
               if(swizzle[i] == UTIL_FORMAT_SWIZZLE_1)
               {
                  switch(output_desc->channel[1].type)
                  {
                  case UTIL_FORMAT_TYPE_UNSIGNED:
                     v = output_desc->channel[1].normalized ? 0xffff : 1;
                     break;
                  case UTIL_FORMAT_TYPE_SIGNED:
                     v = output_desc->channel[1].normalized ? 0x7fff : 1;
                     break;
                  case UTIL_FORMAT_TYPE_FLOAT:
                     v = 0x3c00;
                     break;
                  default:
                     return FALSE;
                  }
               }
               x86_mov16_imm(p->func, x86_make_disp(dst, i * 2), v);
            }
            else if(swizzle[i] == UTIL_FORMAT_SWIZZLE_0)
               x86_mov16_imm(p->func, x86_make_disp(dst, i * 2), 0);
            else
            {
               x86_mov16(p->func, tmp, x86_make_disp(src, swizzle[i] * 2));
               x86_mov16(p->func, x86_make_disp(dst, i * 2), tmp);
            }
            break;
         case 32:
            if(swizzle[i] >= UTIL_FORMAT_SWIZZLE_0)
            {
               unsigned v = 0;
               if(swizzle[i] == UTIL_FORMAT_SWIZZLE_1)
               {
                  switch(output_desc->channel[1].type)
                  {
                  case UTIL_FORMAT_TYPE_UNSIGNED:
                     v = output_desc->channel[1].normalized ? 0xffffffff : 1;
                     break;
                  case UTIL_FORMAT_TYPE_SIGNED:
                     v = output_desc->channel[1].normalized ? 0x7fffffff : 1;
                     break;
                  case UTIL_FORMAT_TYPE_FLOAT:
                     v = 0x3f800000;
                     break;
                  default:
                     return FALSE;
                  }
               }
               x86_mov_imm(p->func, x86_make_disp(dst, i * 4), v);
            }
            else
            {
               x86_mov(p->func, tmp, x86_make_disp(src, swizzle[i] * 4));
               x86_mov(p->func, x86_make_disp(dst, i * 4), tmp);
            }
            break;
         case 64:
            if(swizzle[i] >= UTIL_FORMAT_SWIZZLE_0)
            {
               unsigned l = 0;
               unsigned h = 0;
               if(swizzle[i] == UTIL_FORMAT_SWIZZLE_1)
               {
                  switch(output_desc->channel[1].type)
                  {
                  case UTIL_FORMAT_TYPE_UNSIGNED:
                     h = output_desc->channel[1].normalized ? 0xffffffff : 0;
                     l = output_desc->channel[1].normalized ? 0xffffffff : 1;
                     break;
                  case UTIL_FORMAT_TYPE_SIGNED:
                     h = output_desc->channel[1].normalized ? 0x7fffffff : 0;
                     l = output_desc->channel[1].normalized ? 0xffffffff : 1;
                     break;
                  case UTIL_FORMAT_TYPE_FLOAT:
                     h = 0x3ff00000;
                     l = 0;
                     break;
                  default:
                     return FALSE;
                  }
               }
               x86_mov_imm(p->func, x86_make_disp(dst, i * 8), l);
               x86_mov_imm(p->func, x86_make_disp(dst, i * 8 + 4), h);
            }
            else
            {
               if(x86_target_caps(p->func) & X86_SSE)
               {
                  struct x86_reg tmpXMM = x86_make_reg(file_XMM, 0);
                  emit_load64(p, tmp, tmpXMM, x86_make_disp(src, swizzle[i] * 8));
                  emit_store64(p, x86_make_disp(dst, i * 8), tmp, tmpXMM);
               }
               else
               {
                  x86_mov(p->func, tmp, x86_make_disp(src, swizzle[i] * 8));
                  x86_mov(p->func, x86_make_disp(dst, i * 8), tmp);
                  x86_mov(p->func, tmp, x86_make_disp(src, swizzle[i] * 8 + 4));
                  x86_mov(p->func, x86_make_disp(dst, i * 8 + 4), tmp);
               }
            }
            break;
         default:
            return FALSE;
         }
      }
      return TRUE;
   }
   /* special case for draw's EMIT_4UB (RGBA) and EMIT_4UB_BGRA */
   else if((x86_target_caps(p->func) & X86_SSE2) &&
         a->input_format == PIPE_FORMAT_R32G32B32A32_FLOAT && (0
               || a->output_format == PIPE_FORMAT_B8G8R8A8_UNORM
               || a->output_format == PIPE_FORMAT_R8G8B8A8_UNORM
         ))
   {
      struct x86_reg dataXMM = x86_make_reg(file_XMM, 0);

      /* load */
      sse_movups(p->func, dataXMM, src);

      if (a->output_format == PIPE_FORMAT_B8G8R8A8_UNORM)
         sse_shufps(p->func, dataXMM, dataXMM, SHUF(2,1,0,3));

      /* scale by 255.0 */
      sse_mulps(p->func, dataXMM, get_const(p, CONST_255));

      /* pack and emit */
      sse2_cvtps2dq(p->func, dataXMM, dataXMM);
      sse2_packssdw(p->func, dataXMM, dataXMM);
      sse2_packuswb(p->func, dataXMM, dataXMM);
      sse2_movd(p->func, dst, dataXMM);

      return TRUE;
   }

   return FALSE;
}

static boolean translate_attr( struct translate_sse *p,
			       const struct translate_element *a,
			       struct x86_reg src,
			       struct x86_reg dst)
{
   if(a->input_format == a->output_format)
   {
      emit_memcpy(p, dst, src, util_format_get_stride(a->input_format, 1));
      return TRUE;
   }

   return translate_attr_convert(p, a, src, dst);
}

static boolean init_inputs( struct translate_sse *p,
                            unsigned index_size )
{
   unsigned i;
   struct x86_reg instance_id = x86_make_disp(p->machine_EDI,
                                              get_offset(p, &p->instance_id));

   for (i = 0; i < p->nr_buffer_variants; i++) {
      struct translate_buffer_variant *variant = &p->buffer_variant[i];
      struct translate_buffer *buffer = &p->buffer[variant->buffer_index];

      if (!index_size || variant->instance_divisor) {
         struct x86_reg buf_max_index = x86_make_disp(p->machine_EDI,
                                                     get_offset(p, &buffer->max_index));
         struct x86_reg buf_stride   = x86_make_disp(p->machine_EDI,
                                                     get_offset(p, &buffer->stride));
         struct x86_reg buf_ptr      = x86_make_disp(p->machine_EDI,
                                                     get_offset(p, &variant->ptr));
         struct x86_reg buf_base_ptr = x86_make_disp(p->machine_EDI,
                                                     get_offset(p, &buffer->base_ptr));
         struct x86_reg elt = p->idx_ESI;
         struct x86_reg tmp_EAX = p->tmp_EAX;

         /* Calculate pointer to first attrib:
          *   base_ptr + stride * index, where index depends on instance divisor
          */
         if (variant->instance_divisor) {
            /* Our index is instance ID divided by instance divisor.
             */
            x86_mov(p->func, tmp_EAX, instance_id);

            if (variant->instance_divisor != 1) {
               struct x86_reg tmp_EDX = p->tmp2_EDX;
               struct x86_reg tmp_ECX = p->src_ECX;

               /* TODO: Add x86_shr() to rtasm and use it whenever
                *       instance divisor is power of two.
                */

               x86_xor(p->func, tmp_EDX, tmp_EDX);
               x86_mov_reg_imm(p->func, tmp_ECX, variant->instance_divisor);
               x86_div(p->func, tmp_ECX);    /* EAX = EDX:EAX / ECX */
            }

            /* XXX we need to clamp the index here too, but to a
             * per-array max value, not the draw->pt.max_index value
             * that's being given to us via translate->set_buffer().
             */
         } else {
            x86_mov(p->func, tmp_EAX, elt);

            /* Clamp to max_index
             */
            x86_cmp(p->func, tmp_EAX, buf_max_index);
            x86_cmovcc(p->func, tmp_EAX, buf_max_index, cc_AE);
         }

         x86_imul(p->func, tmp_EAX, buf_stride);
         x64_rexw(p->func);
         x86_add(p->func, tmp_EAX, buf_base_ptr);

         x86_cmp(p->func, p->count_EBP, p->tmp_EAX);

         /* In the linear case, keep the buffer pointer instead of the
          * index number.
          */
         if (!index_size && p->nr_buffer_variants == 1)
         {
            x64_rexw(p->func);
            x86_mov(p->func, elt, tmp_EAX);
         }
         else
         {
            x64_rexw(p->func);
            x86_mov(p->func, buf_ptr, tmp_EAX);
         }
      }
   }

   return TRUE;
}


static struct x86_reg get_buffer_ptr( struct translate_sse *p,
                                      unsigned index_size,
                                      unsigned var_idx,
                                      struct x86_reg elt )
{
   if (var_idx == ELEMENT_BUFFER_INSTANCE_ID) {
      return x86_make_disp(p->machine_EDI,
                           get_offset(p, &p->instance_id));
   }
   if (!index_size && p->nr_buffer_variants == 1) {
      return p->idx_ESI;
   }
   else if (!index_size || p->buffer_variant[var_idx].instance_divisor) {
      struct x86_reg ptr = p->src_ECX;
      struct x86_reg buf_ptr = 
         x86_make_disp(p->machine_EDI,
                       get_offset(p, &p->buffer_variant[var_idx].ptr));
      
      x64_rexw(p->func);
      x86_mov(p->func, ptr, buf_ptr);
      return ptr;
   }
   else {
      struct x86_reg ptr = p->src_ECX;
      const struct translate_buffer_variant *variant = &p->buffer_variant[var_idx];

      struct x86_reg buf_stride = 
         x86_make_disp(p->machine_EDI,
                       get_offset(p, &p->buffer[variant->buffer_index].stride));

      struct x86_reg buf_base_ptr = 
         x86_make_disp(p->machine_EDI,
                       get_offset(p, &p->buffer[variant->buffer_index].base_ptr));

      struct x86_reg buf_max_index =
         x86_make_disp(p->machine_EDI,
                       get_offset(p, &p->buffer[variant->buffer_index].max_index));



      /* Calculate pointer to current attrib:
       */
      switch(index_size)
      {
      case 1:
         x86_movzx8(p->func, ptr, elt);
         break;
      case 2:
         x86_movzx16(p->func, ptr, elt);
         break;
      case 4:
         x86_mov(p->func, ptr, elt);
         break;
      }

      /* Clamp to max_index
       */
      x86_cmp(p->func, ptr, buf_max_index);
      x86_cmovcc(p->func, ptr, buf_max_index, cc_AE);

      x86_imul(p->func, ptr, buf_stride);
      x64_rexw(p->func);
      x86_add(p->func, ptr, buf_base_ptr);
      return ptr;
   }
}



static boolean incr_inputs( struct translate_sse *p, 
                            unsigned index_size )
{
   if (!index_size && p->nr_buffer_variants == 1) {
      struct x86_reg stride = x86_make_disp(p->machine_EDI,
                                            get_offset(p, &p->buffer[0].stride));

      if (p->buffer_variant[0].instance_divisor == 0) {
         x64_rexw(p->func);
         x86_add(p->func, p->idx_ESI, stride);
         sse_prefetchnta(p->func, x86_make_disp(p->idx_ESI, 192));
      }
   }
   else if (!index_size) {
      unsigned i;

      /* Is this worthwhile??
       */
      for (i = 0; i < p->nr_buffer_variants; i++) {
         struct translate_buffer_variant *variant = &p->buffer_variant[i];
         struct x86_reg buf_ptr = x86_make_disp(p->machine_EDI,
                                                get_offset(p, &variant->ptr));
         struct x86_reg buf_stride = x86_make_disp(p->machine_EDI,
                                                   get_offset(p, &p->buffer[variant->buffer_index].stride));

         if (variant->instance_divisor == 0) {
            x86_mov(p->func, p->tmp_EAX, buf_stride);
            x64_rexw(p->func);
            x86_add(p->func, p->tmp_EAX, buf_ptr);
            if (i == 0) sse_prefetchnta(p->func, x86_make_disp(p->tmp_EAX, 192));
            x64_rexw(p->func);
            x86_mov(p->func, buf_ptr, p->tmp_EAX);
         }
      }
   } 
   else {
      x64_rexw(p->func);
      x86_lea(p->func, p->idx_ESI, x86_make_disp(p->idx_ESI, index_size));
   }
   
   return TRUE;
}


/* Build run( struct translate *machine,
 *            unsigned start,
 *            unsigned count,
 *            void *output_buffer )
 * or
 *  run_elts( struct translate *machine,
 *            unsigned *elts,
 *            unsigned count,
 *            void *output_buffer )
 *
 *  Lots of hardcoding
 *
 * EAX -- pointer to current output vertex
 * ECX -- pointer to current attribute 
 * 
 */
static boolean build_vertex_emit( struct translate_sse *p,
				  struct x86_function *func,
				  unsigned index_size )
{
   int fixup, label;
   unsigned j;

   memset(p->reg_to_const, 0xff, sizeof(p->reg_to_const));
   memset(p->const_to_reg, 0xff, sizeof(p->const_to_reg));

   p->tmp_EAX       = x86_make_reg(file_REG32, reg_AX);
   p->idx_ESI       = x86_make_reg(file_REG32, reg_SI);
   p->outbuf_EBX    = x86_make_reg(file_REG32, reg_BX);
   p->machine_EDI   = x86_make_reg(file_REG32, reg_DI);
   p->count_EBP     = x86_make_reg(file_REG32, reg_BP);
   p->tmp2_EDX     = x86_make_reg(file_REG32, reg_DX);
   p->src_ECX     = x86_make_reg(file_REG32, reg_CX);

   p->func = func;

   x86_init_func(p->func);

   if(x86_target(p->func) == X86_64_WIN64_ABI)
   {
	   /* the ABI guarantees a 16-byte aligned 32-byte "shadow space" above the return address */
	   sse2_movdqa(p->func, x86_make_disp(x86_make_reg(file_REG32, reg_SP), 8), x86_make_reg(file_XMM, 6));
	   sse2_movdqa(p->func, x86_make_disp(x86_make_reg(file_REG32, reg_SP), 24), x86_make_reg(file_XMM, 7));
   }

   x86_push(p->func, p->outbuf_EBX);
   x86_push(p->func, p->count_EBP);

/* on non-Win64 x86-64, these are already in the right registers */
   if(x86_target(p->func) != X86_64_STD_ABI)
   {
      x86_push(p->func, p->machine_EDI);
      x86_push(p->func, p->idx_ESI);

      x86_mov(p->func, p->machine_EDI, x86_fn_arg(p->func, 1));
      x86_mov(p->func, p->idx_ESI, x86_fn_arg(p->func, 2));
   }

   x86_mov(p->func, p->count_EBP, x86_fn_arg(p->func, 3));

   if(x86_target(p->func) != X86_32)
      x64_mov64(p->func, p->outbuf_EBX, x86_fn_arg(p->func, 5));
   else
      x86_mov(p->func, p->outbuf_EBX, x86_fn_arg(p->func, 5));

   /* Load instance ID.
    */
   if (p->use_instancing) {
      x86_mov(p->func,
              p->tmp_EAX,
              x86_fn_arg(p->func, 4));
      x86_mov(p->func,
              x86_make_disp(p->machine_EDI, get_offset(p, &p->instance_id)),
              p->tmp_EAX);
   }

   /* Get vertex count, compare to zero
    */
   x86_xor(p->func, p->tmp_EAX, p->tmp_EAX);
   x86_cmp(p->func, p->count_EBP, p->tmp_EAX);
   fixup = x86_jcc_forward(p->func, cc_E);

   /* always load, needed or not:
    */
   init_inputs(p, index_size);

   /* Note address for loop jump
    */
   label = x86_get_label(p->func);
   {
      struct x86_reg elt = !index_size ? p->idx_ESI : x86_deref(p->idx_ESI);
      int last_variant = -1;
      struct x86_reg vb;

      for (j = 0; j < p->translate.key.nr_elements; j++) {
         const struct translate_element *a = &p->translate.key.element[j];
         unsigned variant = p->element_to_buffer_variant[j];

         /* Figure out source pointer address:
          */
         if (variant != last_variant) {
            last_variant = variant;
            vb = get_buffer_ptr(p, index_size, variant, elt);
         }
         
         if (!translate_attr( p, a, 
                              x86_make_disp(vb, a->input_offset), 
                              x86_make_disp(p->outbuf_EBX, a->output_offset)))
            return FALSE;
      }

      /* Next output vertex:
       */
      x64_rexw(p->func);
      x86_lea(p->func, 
              p->outbuf_EBX,
              x86_make_disp(p->outbuf_EBX,
                            p->translate.key.output_stride));

      /* Incr index
       */ 
      incr_inputs( p, index_size );
   }

   /* decr count, loop if not zero
    */
   x86_dec(p->func, p->count_EBP);
   x86_jcc(p->func, cc_NZ, label);

   /* Exit mmx state?
    */
   if (p->func->need_emms)
      mmx_emms(p->func);

   /* Land forward jump here:
    */
   x86_fixup_fwd_jump(p->func, fixup);

   /* Pop regs and return
    */
   
   if(x86_target(p->func) != X86_64_STD_ABI)
   {
      x86_pop(p->func, p->idx_ESI);
      x86_pop(p->func, p->machine_EDI);
   }

   x86_pop(p->func, p->count_EBP);
   x86_pop(p->func, p->outbuf_EBX);

   if(x86_target(p->func) == X86_64_WIN64_ABI)
   {
	   sse2_movdqa(p->func, x86_make_reg(file_XMM, 6), x86_make_disp(x86_make_reg(file_REG32, reg_SP), 8));
	   sse2_movdqa(p->func, x86_make_reg(file_XMM, 7), x86_make_disp(x86_make_reg(file_REG32, reg_SP), 24));
   }
   x86_ret(p->func);

   return TRUE;
}






			       
static void translate_sse_set_buffer( struct translate *translate,
				unsigned buf,
				const void *ptr,
				unsigned stride,
				unsigned max_index )
{
   struct translate_sse *p = (struct translate_sse *)translate;

   if (buf < p->nr_buffers) {
      p->buffer[buf].base_ptr = (char *)ptr;
      p->buffer[buf].stride = stride;
      p->buffer[buf].max_index = max_index;
   }

   if (0) debug_printf("%s %d/%d: %p %d\n", 
                       __FUNCTION__, buf, 
                       p->nr_buffers, 
                       ptr, stride);
}


static void translate_sse_release( struct translate *translate )
{
   struct translate_sse *p = (struct translate_sse *)translate;

   x86_release_func( &p->linear_func );
   x86_release_func( &p->elt_func );

   os_free_aligned(p);
}


struct translate *translate_sse2_create( const struct translate_key *key )
{
   struct translate_sse *p = NULL;
   unsigned i;

   /* this is misnamed, it actually refers to whether rtasm is enabled or not */
   if (!rtasm_cpu_has_sse())
      goto fail;

   p = os_malloc_aligned(sizeof(struct translate_sse), 16);
   if (p == NULL) 
      goto fail;
   memset(p, 0, sizeof(*p));
   memcpy(p->consts, consts, sizeof(consts));

   p->translate.key = *key;
   p->translate.release = translate_sse_release;
   p->translate.set_buffer = translate_sse_set_buffer;

   for (i = 0; i < key->nr_elements; i++) {
      if (key->element[i].type == TRANSLATE_ELEMENT_NORMAL) {
         unsigned j;

         p->nr_buffers = MAX2(p->nr_buffers, key->element[i].input_buffer + 1);

         if (key->element[i].instance_divisor) {
            p->use_instancing = TRUE;
         }

         /*
          * Map vertex element to vertex buffer variant.
          */
         for (j = 0; j < p->nr_buffer_variants; j++) {
            if (p->buffer_variant[j].buffer_index == key->element[i].input_buffer &&
                p->buffer_variant[j].instance_divisor == key->element[i].instance_divisor) {
               break;
            }
         }
         if (j == p->nr_buffer_variants) {
            p->buffer_variant[j].buffer_index = key->element[i].input_buffer;
            p->buffer_variant[j].instance_divisor = key->element[i].instance_divisor;
            p->nr_buffer_variants++;
         }
         p->element_to_buffer_variant[i] = j;
      } else {
         assert(key->element[i].type == TRANSLATE_ELEMENT_INSTANCE_ID);

         p->element_to_buffer_variant[i] = ELEMENT_BUFFER_INSTANCE_ID;
      }
   }

   if (0) debug_printf("nr_buffers: %d\n", p->nr_buffers);

   if (!build_vertex_emit(p, &p->linear_func, 0))
      goto fail;

   if (!build_vertex_emit(p, &p->elt_func, 4))
      goto fail;

   if (!build_vertex_emit(p, &p->elt16_func, 2))
      goto fail;

   if (!build_vertex_emit(p, &p->elt8_func, 1))
      goto fail;

   p->translate.run = (run_func) x86_get_func(&p->linear_func);
   if (p->translate.run == NULL)
      goto fail;

   p->translate.run_elts = (run_elts_func) x86_get_func(&p->elt_func);
   if (p->translate.run_elts == NULL)
      goto fail;

   p->translate.run_elts16 = (run_elts16_func) x86_get_func(&p->elt16_func);
   if (p->translate.run_elts16 == NULL)
      goto fail;

   p->translate.run_elts8 = (run_elts8_func) x86_get_func(&p->elt8_func);
   if (p->translate.run_elts8 == NULL)
      goto fail;

   return &p->translate;

 fail:
   if (p)
      translate_sse_release( &p->translate );

   return NULL;
}



#else

struct translate *translate_sse2_create( const struct translate_key *key )
{
   return NULL;
}

#endif
