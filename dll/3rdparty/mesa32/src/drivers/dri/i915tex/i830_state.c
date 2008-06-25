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


#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "enums.h"
#include "dd.h"

#include "texmem.h"

#include "intel_screen.h"
#include "intel_batchbuffer.h"
#include "intel_fbo.h"

#include "i830_context.h"
#include "i830_reg.h"

#define FILE_DEBUG_FLAG DEBUG_STATE

static void
i830StencilFuncSeparate(GLcontext * ctx, GLenum face, GLenum func, GLint ref,
                        GLuint mask)
{
   struct i830_context *i830 = i830_context(ctx);
   int test = intel_translate_compare_func(func);

   mask = mask & 0xff;

   DBG("%s : func: %s, ref : 0x%x, mask: 0x%x\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(func), ref, mask);


   I830_STATECHANGE(i830, I830_UPLOAD_CTX);
   i830->state.Ctx[I830_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_TEST_MASK;
   i830->state.Ctx[I830_CTXREG_STATE4] |= (ENABLE_STENCIL_TEST_MASK |
                                           STENCIL_TEST_MASK(mask));
   i830->state.Ctx[I830_CTXREG_STENCILTST] &= ~(STENCIL_REF_VALUE_MASK |
                                                ENABLE_STENCIL_TEST_FUNC_MASK);
   i830->state.Ctx[I830_CTXREG_STENCILTST] |= (ENABLE_STENCIL_REF_VALUE |
                                               ENABLE_STENCIL_TEST_FUNC |
                                               STENCIL_REF_VALUE(ref) |
                                               STENCIL_TEST_FUNC(test));
}

static void
i830StencilMaskSeparate(GLcontext * ctx, GLenum face, GLuint mask)
{
   struct i830_context *i830 = i830_context(ctx);

   DBG("%s : mask 0x%x\n", __FUNCTION__, mask);
   
   mask = mask & 0xff;

   I830_STATECHANGE(i830, I830_UPLOAD_CTX);
   i830->state.Ctx[I830_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_WRITE_MASK;
   i830->state.Ctx[I830_CTXREG_STATE4] |= (ENABLE_STENCIL_WRITE_MASK |
                                           STENCIL_WRITE_MASK(mask));
}

static void
i830StencilOpSeparate(GLcontext * ctx, GLenum face, GLenum fail, GLenum zfail,
                      GLenum zpass)
{
   struct i830_context *i830 = i830_context(ctx);
   int fop, dfop, dpop;

   DBG("%s: fail : %s, zfail: %s, zpass : %s\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(fail),
       _mesa_lookup_enum_by_nr(zfail), 
       _mesa_lookup_enum_by_nr(zpass));

   fop = 0;
   dfop = 0;
   dpop = 0;

   switch (fail) {
   case GL_KEEP:
      fop = STENCILOP_KEEP;
      break;
   case GL_ZERO:
      fop = STENCILOP_ZERO;
      break;
   case GL_REPLACE:
      fop = STENCILOP_REPLACE;
      break;
   case GL_INCR:
      fop = STENCILOP_INCRSAT;
      break;
   case GL_DECR:
      fop = STENCILOP_DECRSAT;
      break;
   case GL_INCR_WRAP:
      fop = STENCILOP_INCR;
      break;
   case GL_DECR_WRAP:
      fop = STENCILOP_DECR;
      break;
   case GL_INVERT:
      fop = STENCILOP_INVERT;
      break;
   default:
      break;
   }
   switch (zfail) {
   case GL_KEEP:
      dfop = STENCILOP_KEEP;
      break;
   case GL_ZERO:
      dfop = STENCILOP_ZERO;
      break;
   case GL_REPLACE:
      dfop = STENCILOP_REPLACE;
      break;
   case GL_INCR:
      dfop = STENCILOP_INCRSAT;
      break;
   case GL_DECR:
      dfop = STENCILOP_DECRSAT;
      break;
   case GL_INCR_WRAP:
      dfop = STENCILOP_INCR;
      break;
   case GL_DECR_WRAP:
      dfop = STENCILOP_DECR;
      break;
   case GL_INVERT:
      dfop = STENCILOP_INVERT;
      break;
   default:
      break;
   }
   switch (zpass) {
   case GL_KEEP:
      dpop = STENCILOP_KEEP;
      break;
   case GL_ZERO:
      dpop = STENCILOP_ZERO;
      break;
   case GL_REPLACE:
      dpop = STENCILOP_REPLACE;
      break;
   case GL_INCR:
      dpop = STENCILOP_INCRSAT;
      break;
   case GL_DECR:
      dpop = STENCILOP_DECRSAT;
      break;
   case GL_INCR_WRAP:
      dpop = STENCILOP_INCR;
      break;
   case GL_DECR_WRAP:
      dpop = STENCILOP_DECR;
      break;
   case GL_INVERT:
      dpop = STENCILOP_INVERT;
      break;
   default:
      break;
   }


   I830_STATECHANGE(i830, I830_UPLOAD_CTX);
   i830->state.Ctx[I830_CTXREG_STENCILTST] &= ~(STENCIL_OPS_MASK);
   i830->state.Ctx[I830_CTXREG_STENCILTST] |= (ENABLE_STENCIL_PARMS |
                                               STENCIL_FAIL_OP(fop) |
                                               STENCIL_PASS_DEPTH_FAIL_OP
                                               (dfop) |
                                               STENCIL_PASS_DEPTH_PASS_OP
                                               (dpop));
}

static void
i830AlphaFunc(GLcontext * ctx, GLenum func, GLfloat ref)
{
   struct i830_context *i830 = i830_context(ctx);
   int test = intel_translate_compare_func(func);
   GLubyte refByte;
   GLuint refInt;

   UNCLAMPED_FLOAT_TO_UBYTE(refByte, ref);
   refInt = (GLuint) refByte;

   I830_STATECHANGE(i830, I830_UPLOAD_CTX);
   i830->state.Ctx[I830_CTXREG_STATE2] &= ~ALPHA_TEST_REF_MASK;
   i830->state.Ctx[I830_CTXREG_STATE2] |= (ENABLE_ALPHA_TEST_FUNC |
                                           ENABLE_ALPHA_REF_VALUE |
                                           ALPHA_TEST_FUNC(test) |
                                           ALPHA_REF_VALUE(refInt));
}

/**
 * Makes sure that the proper enables are set for LogicOp, Independant Alpha
 * Blend, and Blending.  It needs to be called from numerous places where we
 * could change the LogicOp or Independant Alpha Blend without subsequent
 * calls to glEnable.
 * 
 * \todo
 * This function is substantially different from the old i830-specific driver.
 * I'm not sure which is correct.
 */
static void
i830EvalLogicOpBlendState(GLcontext * ctx)
{
   struct i830_context *i830 = i830_context(ctx);

   I830_STATECHANGE(i830, I830_UPLOAD_CTX);

   if (RGBA_LOGICOP_ENABLED(ctx)) {
      i830->state.Ctx[I830_CTXREG_ENABLES_1] &= ~(ENABLE_COLOR_BLEND |
                                                  ENABLE_LOGIC_OP_MASK);
      i830->state.Ctx[I830_CTXREG_ENABLES_1] |= (DISABLE_COLOR_BLEND |
                                                 ENABLE_LOGIC_OP);
   }
   else if (ctx->Color.BlendEnabled) {
      i830->state.Ctx[I830_CTXREG_ENABLES_1] &= ~(ENABLE_COLOR_BLEND |
                                                  ENABLE_LOGIC_OP_MASK);
      i830->state.Ctx[I830_CTXREG_ENABLES_1] |= (ENABLE_COLOR_BLEND |
                                                 DISABLE_LOGIC_OP);
   }
   else {
      i830->state.Ctx[I830_CTXREG_ENABLES_1] &= ~(ENABLE_COLOR_BLEND |
                                                  ENABLE_LOGIC_OP_MASK);
      i830->state.Ctx[I830_CTXREG_ENABLES_1] |= (DISABLE_COLOR_BLEND |
                                                 DISABLE_LOGIC_OP);
   }
}

static void
i830BlendColor(GLcontext * ctx, const GLfloat color[4])
{
   struct i830_context *i830 = i830_context(ctx);
   GLubyte r, g, b, a;

   DBG("%s\n", __FUNCTION__);
   
   UNCLAMPED_FLOAT_TO_UBYTE(r, color[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, color[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(b, color[BCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, color[ACOMP]);

   I830_STATECHANGE(i830, I830_UPLOAD_CTX);
   i830->state.Ctx[I830_CTXREG_BLENDCOLOR1] =
      (a << 24) | (r << 16) | (g << 8) | b;
}

/**
 * Sets both the blend equation (called "function" in i830 docs) and the
 * blend function (called "factor" in i830 docs).  This is done in a single
 * function because some blend equations (i.e., \c GL_MIN and \c GL_MAX)
 * change the interpretation of the blend function.
 */
static void
i830_set_blend_state(GLcontext * ctx)
{
   struct i830_context *i830 = i830_context(ctx);
   int funcA;
   int funcRGB;
   int eqnA;
   int eqnRGB;
   int iab;
   int s1;


   funcRGB =
      SRC_BLND_FACT(intel_translate_blend_factor(ctx->Color.BlendSrcRGB))
      | DST_BLND_FACT(intel_translate_blend_factor(ctx->Color.BlendDstRGB));

   switch (ctx->Color.BlendEquationRGB) {
   case GL_FUNC_ADD:
      eqnRGB = BLENDFUNC_ADD;
      break;
   case GL_MIN:
      eqnRGB = BLENDFUNC_MIN;
      funcRGB = SRC_BLND_FACT(BLENDFACT_ONE) | DST_BLND_FACT(BLENDFACT_ONE);
      break;
   case GL_MAX:
      eqnRGB = BLENDFUNC_MAX;
      funcRGB = SRC_BLND_FACT(BLENDFACT_ONE) | DST_BLND_FACT(BLENDFACT_ONE);
      break;
   case GL_FUNC_SUBTRACT:
      eqnRGB = BLENDFUNC_SUB;
      break;
   case GL_FUNC_REVERSE_SUBTRACT:
      eqnRGB = BLENDFUNC_RVRSE_SUB;
      break;
   default:
      fprintf(stderr, "[%s:%u] Invalid RGB blend equation (0x%04x).\n",
              __FUNCTION__, __LINE__, ctx->Color.BlendEquationRGB);
      return;
   }


   funcA = SRC_ABLEND_FACT(intel_translate_blend_factor(ctx->Color.BlendSrcA))
      | DST_ABLEND_FACT(intel_translate_blend_factor(ctx->Color.BlendDstA));

   switch (ctx->Color.BlendEquationA) {
   case GL_FUNC_ADD:
      eqnA = BLENDFUNC_ADD;
      break;
   case GL_MIN:
      eqnA = BLENDFUNC_MIN;
      funcA = SRC_BLND_FACT(BLENDFACT_ONE) | DST_BLND_FACT(BLENDFACT_ONE);
      break;
   case GL_MAX:
      eqnA = BLENDFUNC_MAX;
      funcA = SRC_BLND_FACT(BLENDFACT_ONE) | DST_BLND_FACT(BLENDFACT_ONE);
      break;
   case GL_FUNC_SUBTRACT:
      eqnA = BLENDFUNC_SUB;
      break;
   case GL_FUNC_REVERSE_SUBTRACT:
      eqnA = BLENDFUNC_RVRSE_SUB;
      break;
   default:
      fprintf(stderr, "[%s:%u] Invalid alpha blend equation (0x%04x).\n",
              __FUNCTION__, __LINE__, ctx->Color.BlendEquationA);
      return;
   }

   iab = eqnA | funcA
      | _3DSTATE_INDPT_ALPHA_BLEND_CMD
      | ENABLE_SRC_ABLEND_FACTOR | ENABLE_DST_ABLEND_FACTOR
      | ENABLE_ALPHA_BLENDFUNC;
   s1 = eqnRGB | funcRGB
      | _3DSTATE_MODES_1_CMD
      | ENABLE_SRC_BLND_FACTOR | ENABLE_DST_BLND_FACTOR
      | ENABLE_COLR_BLND_FUNC;

   if ((eqnA | funcA) != (eqnRGB | funcRGB))
      iab |= ENABLE_INDPT_ALPHA_BLEND;
   else
      iab |= DISABLE_INDPT_ALPHA_BLEND;

   if (iab != i830->state.Ctx[I830_CTXREG_IALPHAB] ||
       s1 != i830->state.Ctx[I830_CTXREG_STATE1]) {
      I830_STATECHANGE(i830, I830_UPLOAD_CTX);
      i830->state.Ctx[I830_CTXREG_IALPHAB] = iab;
      i830->state.Ctx[I830_CTXREG_STATE1] = s1;
   }

   /* This will catch a logicop blend equation.  It will also ensure
    * independant alpha blend is really in the correct state (either enabled
    * or disabled) if blending is already enabled.
    */

   i830EvalLogicOpBlendState(ctx);

   if (0) {
      fprintf(stderr,
              "[%s:%u] STATE1: 0x%08x IALPHAB: 0x%08x blend is %sabled\n",
              __FUNCTION__, __LINE__, i830->state.Ctx[I830_CTXREG_STATE1],
              i830->state.Ctx[I830_CTXREG_IALPHAB],
              (ctx->Color.BlendEnabled) ? "en" : "dis");
   }
}


static void
i830BlendEquationSeparate(GLcontext * ctx, GLenum modeRGB, GLenum modeA)
{
   DBG("%s -> %s, %s\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(modeRGB),
       _mesa_lookup_enum_by_nr(modeA));

   (void) modeRGB;
   (void) modeA;
   i830_set_blend_state(ctx);
}


static void
i830BlendFuncSeparate(GLcontext * ctx, GLenum sfactorRGB,
                      GLenum dfactorRGB, GLenum sfactorA, GLenum dfactorA)
{
   DBG("%s -> RGB(%s, %s) A(%s, %s)\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(sfactorRGB),
       _mesa_lookup_enum_by_nr(dfactorRGB),
       _mesa_lookup_enum_by_nr(sfactorA),
       _mesa_lookup_enum_by_nr(dfactorA));

   (void) sfactorRGB;
   (void) dfactorRGB;
   (void) sfactorA;
   (void) dfactorA;
   i830_set_blend_state(ctx);
}



static void
i830DepthFunc(GLcontext * ctx, GLenum func)
{
   struct i830_context *i830 = i830_context(ctx);
   int test = intel_translate_compare_func(func);

   DBG("%s\n", __FUNCTION__);
   
   I830_STATECHANGE(i830, I830_UPLOAD_CTX);
   i830->state.Ctx[I830_CTXREG_STATE3] &= ~DEPTH_TEST_FUNC_MASK;
   i830->state.Ctx[I830_CTXREG_STATE3] |= (ENABLE_DEPTH_TEST_FUNC |
                                           DEPTH_TEST_FUNC(test));
}

static void
i830DepthMask(GLcontext * ctx, GLboolean flag)
{
   struct i830_context *i830 = i830_context(ctx);

   DBG("%s flag (%d)\n", __FUNCTION__, flag);
   
   I830_STATECHANGE(i830, I830_UPLOAD_CTX);

   i830->state.Ctx[I830_CTXREG_ENABLES_2] &= ~ENABLE_DIS_DEPTH_WRITE_MASK;

   if (flag && ctx->Depth.Test)
      i830->state.Ctx[I830_CTXREG_ENABLES_2] |= ENABLE_DEPTH_WRITE;
   else
      i830->state.Ctx[I830_CTXREG_ENABLES_2] |= DISABLE_DEPTH_WRITE;
}

/* =============================================================
 * Polygon stipple
 *
 * The i830 supports a 4x4 stipple natively, GL wants 32x32.
 * Fortunately stipple is usually a repeating pattern.
 */
static void
i830PolygonStipple(GLcontext * ctx, const GLubyte * mask)
{
   struct i830_context *i830 = i830_context(ctx);
   const GLubyte *m = mask;
   GLubyte p[4];
   int i, j, k;
   int active = (ctx->Polygon.StippleFlag &&
                 i830->intel.reduced_primitive == GL_TRIANGLES);
   GLuint newMask;

   if (active) {
      I830_STATECHANGE(i830, I830_UPLOAD_STIPPLE);
      i830->state.Stipple[I830_STPREG_ST1] &= ~ST1_ENABLE;
   }

   p[0] = mask[12] & 0xf;
   p[0] |= p[0] << 4;
   p[1] = mask[8] & 0xf;
   p[1] |= p[1] << 4;
   p[2] = mask[4] & 0xf;
   p[2] |= p[2] << 4;
   p[3] = mask[0] & 0xf;
   p[3] |= p[3] << 4;

   for (k = 0; k < 8; k++)
      for (j = 3; j >= 0; j--)
         for (i = 0; i < 4; i++, m++)
            if (*m != p[j]) {
               i830->intel.hw_stipple = 0;
               return;
            }

   newMask = (((p[0] & 0xf) << 0) |
              ((p[1] & 0xf) << 4) |
              ((p[2] & 0xf) << 8) | ((p[3] & 0xf) << 12));


   if (newMask == 0xffff || newMask == 0x0) {
      /* this is needed to make conform pass */
      i830->intel.hw_stipple = 0;
      return;
   }

   i830->state.Stipple[I830_STPREG_ST1] &= ~0xffff;
   i830->state.Stipple[I830_STPREG_ST1] |= newMask;
   i830->intel.hw_stipple = 1;

   if (active)
      i830->state.Stipple[I830_STPREG_ST1] |= ST1_ENABLE;
}


/* =============================================================
 * Hardware clipping
 */
static void
i830Scissor(GLcontext * ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   struct i830_context *i830 = i830_context(ctx);
   int x1, y1, x2, y2;

   if (!ctx->DrawBuffer)
      return;

   DBG("%s %d,%d %dx%d\n", __FUNCTION__, x, y, w, h);

   if (ctx->DrawBuffer->Name == 0) {
      x1 = x;
      y1 = ctx->DrawBuffer->Height - (y + h);
      x2 = x + w - 1;
      y2 = y1 + h - 1;
      DBG("%s %d..%d,%d..%d (inverted)\n", __FUNCTION__, x1, x2, y1, y2);
   }
   else {
      /* FBO - not inverted
       */
      x1 = x;
      y1 = y;
      x2 = x + w - 1;
      y2 = y + h - 1;
      DBG("%s %d..%d,%d..%d (not inverted)\n", __FUNCTION__, x1, x2, y1, y2);
   }

   x1 = CLAMP(x1, 0, ctx->DrawBuffer->Width - 1);
   y1 = CLAMP(y1, 0, ctx->DrawBuffer->Height - 1);
   x2 = CLAMP(x2, 0, ctx->DrawBuffer->Width - 1);
   y2 = CLAMP(y2, 0, ctx->DrawBuffer->Height - 1);
   
   DBG("%s %d..%d,%d..%d (clamped)\n", __FUNCTION__, x1, x2, y1, y2);

   I830_STATECHANGE(i830, I830_UPLOAD_BUFFERS);
   i830->state.Buffer[I830_DESTREG_SR1] = (y1 << 16) | (x1 & 0xffff);
   i830->state.Buffer[I830_DESTREG_SR2] = (y2 << 16) | (x2 & 0xffff);
}

static void
i830LogicOp(GLcontext * ctx, GLenum opcode)
{
   struct i830_context *i830 = i830_context(ctx);
   int tmp = intel_translate_logic_op(opcode);

   DBG("%s\n", __FUNCTION__);
   
   I830_STATECHANGE(i830, I830_UPLOAD_CTX);
   i830->state.Ctx[I830_CTXREG_STATE4] &= ~LOGICOP_MASK;
   i830->state.Ctx[I830_CTXREG_STATE4] |= LOGIC_OP_FUNC(tmp);
}



static void
i830CullFaceFrontFace(GLcontext * ctx, GLenum unused)
{
   struct i830_context *i830 = i830_context(ctx);
   GLuint mode;

   DBG("%s\n", __FUNCTION__);
   
   if (!ctx->Polygon.CullFlag) {
      mode = CULLMODE_NONE;
   }
   else if (ctx->Polygon.CullFaceMode != GL_FRONT_AND_BACK) {
      mode = CULLMODE_CW;

      if (ctx->Polygon.CullFaceMode == GL_FRONT)
         mode ^= (CULLMODE_CW ^ CULLMODE_CCW);
      if (ctx->Polygon.FrontFace != GL_CCW)
         mode ^= (CULLMODE_CW ^ CULLMODE_CCW);
   }
   else {
      mode = CULLMODE_BOTH;
   }

   I830_STATECHANGE(i830, I830_UPLOAD_CTX);
   i830->state.Ctx[I830_CTXREG_STATE3] &= ~CULLMODE_MASK;
   i830->state.Ctx[I830_CTXREG_STATE3] |= ENABLE_CULL_MODE | mode;
}

static void
i830LineWidth(GLcontext * ctx, GLfloat widthf)
{
   struct i830_context *i830 = i830_context(ctx);
   int width;
   int state5;

   DBG("%s\n", __FUNCTION__);
   
   width = (int) (widthf * 2);
   CLAMP_SELF(width, 1, 15);

   state5 = i830->state.Ctx[I830_CTXREG_STATE5] & ~FIXED_LINE_WIDTH_MASK;
   state5 |= (ENABLE_FIXED_LINE_WIDTH | FIXED_LINE_WIDTH(width));

   if (state5 != i830->state.Ctx[I830_CTXREG_STATE5]) {
      I830_STATECHANGE(i830, I830_UPLOAD_CTX);
      i830->state.Ctx[I830_CTXREG_STATE5] = state5;
   }
}

static void
i830PointSize(GLcontext * ctx, GLfloat size)
{
   struct i830_context *i830 = i830_context(ctx);
   GLint point_size = (int) size;

   DBG("%s\n", __FUNCTION__);
   
   CLAMP_SELF(point_size, 1, 256);
   I830_STATECHANGE(i830, I830_UPLOAD_CTX);
   i830->state.Ctx[I830_CTXREG_STATE5] &= ~FIXED_POINT_WIDTH_MASK;
   i830->state.Ctx[I830_CTXREG_STATE5] |= (ENABLE_FIXED_POINT_WIDTH |
                                           FIXED_POINT_WIDTH(point_size));
}


/* =============================================================
 * Color masks
 */

static void
i830ColorMask(GLcontext * ctx,
              GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
   struct i830_context *i830 = i830_context(ctx);
   GLuint tmp = 0;

   DBG("%s r(%d) g(%d) b(%d) a(%d)\n", __FUNCTION__, r, g, b, a);

   tmp = ((i830->state.Ctx[I830_CTXREG_ENABLES_2] & ~WRITEMASK_MASK) |
          ENABLE_COLOR_MASK |
          ENABLE_COLOR_WRITE |
          ((!r) << WRITEMASK_RED_SHIFT) |
          ((!g) << WRITEMASK_GREEN_SHIFT) |
          ((!b) << WRITEMASK_BLUE_SHIFT) | ((!a) << WRITEMASK_ALPHA_SHIFT));

   if (tmp != i830->state.Ctx[I830_CTXREG_ENABLES_2]) {
      I830_STATECHANGE(i830, I830_UPLOAD_CTX);
      i830->state.Ctx[I830_CTXREG_ENABLES_2] = tmp;
   }
}

static void
update_specular(GLcontext * ctx)
{
   struct i830_context *i830 = i830_context(ctx);

   I830_STATECHANGE(i830, I830_UPLOAD_CTX);
   i830->state.Ctx[I830_CTXREG_ENABLES_1] &= ~ENABLE_SPEC_ADD_MASK;

   if (NEED_SECONDARY_COLOR(ctx))
      i830->state.Ctx[I830_CTXREG_ENABLES_1] |= ENABLE_SPEC_ADD;
   else
      i830->state.Ctx[I830_CTXREG_ENABLES_1] |= DISABLE_SPEC_ADD;
}

static void
i830LightModelfv(GLcontext * ctx, GLenum pname, const GLfloat * param)
{
   DBG("%s\n", __FUNCTION__);
   
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
      update_specular(ctx);
   }
}

/* In Mesa 3.5 we can reliably do native flatshading.
 */
static void
i830ShadeModel(GLcontext * ctx, GLenum mode)
{
   struct i830_context *i830 = i830_context(ctx);
   I830_STATECHANGE(i830, I830_UPLOAD_CTX);


#define SHADE_MODE_MASK ((1<<10)|(1<<8)|(1<<6)|(1<<4))

   i830->state.Ctx[I830_CTXREG_STATE3] &= ~SHADE_MODE_MASK;

   if (mode == GL_FLAT) {
      i830->state.Ctx[I830_CTXREG_STATE3] |=
         (ALPHA_SHADE_MODE(SHADE_MODE_FLAT) | FOG_SHADE_MODE(SHADE_MODE_FLAT)
          | SPEC_SHADE_MODE(SHADE_MODE_FLAT) |
          COLOR_SHADE_MODE(SHADE_MODE_FLAT));
   }
   else {
      i830->state.Ctx[I830_CTXREG_STATE3] |=
         (ALPHA_SHADE_MODE(SHADE_MODE_LINEAR) |
          FOG_SHADE_MODE(SHADE_MODE_LINEAR) |
          SPEC_SHADE_MODE(SHADE_MODE_LINEAR) |
          COLOR_SHADE_MODE(SHADE_MODE_LINEAR));
   }
}

/* =============================================================
 * Fog
 */
static void
i830Fogfv(GLcontext * ctx, GLenum pname, const GLfloat * param)
{
   struct i830_context *i830 = i830_context(ctx);

   DBG("%s\n", __FUNCTION__);
   
   if (pname == GL_FOG_COLOR) {
      GLuint color = (((GLubyte) (ctx->Fog.Color[0] * 255.0F) << 16) |
                      ((GLubyte) (ctx->Fog.Color[1] * 255.0F) << 8) |
                      ((GLubyte) (ctx->Fog.Color[2] * 255.0F) << 0));

      I830_STATECHANGE(i830, I830_UPLOAD_CTX);
      i830->state.Ctx[I830_CTXREG_FOGCOLOR] =
         (_3DSTATE_FOG_COLOR_CMD | color);
   }
}

/* =============================================================
 */

static void
i830Enable(GLcontext * ctx, GLenum cap, GLboolean state)
{
   struct i830_context *i830 = i830_context(ctx);

   switch (cap) {
   case GL_LIGHTING:
   case GL_COLOR_SUM:
      update_specular(ctx);
      break;

   case GL_ALPHA_TEST:
      I830_STATECHANGE(i830, I830_UPLOAD_CTX);
      i830->state.Ctx[I830_CTXREG_ENABLES_1] &= ~ENABLE_DIS_ALPHA_TEST_MASK;
      if (state)
         i830->state.Ctx[I830_CTXREG_ENABLES_1] |= ENABLE_ALPHA_TEST;
      else
         i830->state.Ctx[I830_CTXREG_ENABLES_1] |= DISABLE_ALPHA_TEST;

      break;

   case GL_BLEND:
      i830EvalLogicOpBlendState(ctx);
      break;

   case GL_COLOR_LOGIC_OP:
      i830EvalLogicOpBlendState(ctx);

      /* Logicop doesn't seem to work at 16bpp:
       */
      if (i830->intel.intelScreen->cpp == 2)
         FALLBACK(&i830->intel, I830_FALLBACK_LOGICOP, state);
      break;

   case GL_DITHER:
      I830_STATECHANGE(i830, I830_UPLOAD_CTX);
      i830->state.Ctx[I830_CTXREG_ENABLES_2] &= ~ENABLE_DITHER;

      if (state)
         i830->state.Ctx[I830_CTXREG_ENABLES_2] |= ENABLE_DITHER;
      else
         i830->state.Ctx[I830_CTXREG_ENABLES_2] |= DISABLE_DITHER;
      break;

   case GL_DEPTH_TEST:
      I830_STATECHANGE(i830, I830_UPLOAD_CTX);
      i830->state.Ctx[I830_CTXREG_ENABLES_1] &= ~ENABLE_DIS_DEPTH_TEST_MASK;

      if (state)
         i830->state.Ctx[I830_CTXREG_ENABLES_1] |= ENABLE_DEPTH_TEST;
      else
         i830->state.Ctx[I830_CTXREG_ENABLES_1] |= DISABLE_DEPTH_TEST;

      /* Also turn off depth writes when GL_DEPTH_TEST is disabled:
       */
      i830DepthMask(ctx, ctx->Depth.Mask);
      break;

   case GL_SCISSOR_TEST:
      I830_STATECHANGE(i830, I830_UPLOAD_BUFFERS);

      if (state)
         i830->state.Buffer[I830_DESTREG_SENABLE] =
            (_3DSTATE_SCISSOR_ENABLE_CMD | ENABLE_SCISSOR_RECT);
      else
         i830->state.Buffer[I830_DESTREG_SENABLE] =
            (_3DSTATE_SCISSOR_ENABLE_CMD | DISABLE_SCISSOR_RECT);

      break;

   case GL_LINE_SMOOTH:
      I830_STATECHANGE(i830, I830_UPLOAD_CTX);

      i830->state.Ctx[I830_CTXREG_AA] &= ~AA_LINE_ENABLE;
      if (state)
         i830->state.Ctx[I830_CTXREG_AA] |= AA_LINE_ENABLE;
      else
         i830->state.Ctx[I830_CTXREG_AA] |= AA_LINE_DISABLE;
      break;

   case GL_FOG:
      I830_STATECHANGE(i830, I830_UPLOAD_CTX);
      i830->state.Ctx[I830_CTXREG_ENABLES_1] &= ~ENABLE_DIS_FOG_MASK;
      if (state)
         i830->state.Ctx[I830_CTXREG_ENABLES_1] |= ENABLE_FOG;
      else
         i830->state.Ctx[I830_CTXREG_ENABLES_1] |= DISABLE_FOG;
      break;

   case GL_CULL_FACE:
      i830CullFaceFrontFace(ctx, 0);
      break;

   case GL_TEXTURE_2D:
      break;

   case GL_STENCIL_TEST:
      {
         GLboolean hw_stencil = GL_FALSE;
         if (ctx->DrawBuffer) {
            struct intel_renderbuffer *irbStencil
               = intel_get_renderbuffer(ctx->DrawBuffer, BUFFER_STENCIL);
            hw_stencil = (irbStencil && irbStencil->region);
         }
         if (hw_stencil) {
            I830_STATECHANGE(i830, I830_UPLOAD_CTX);

            if (state) {
               i830->state.Ctx[I830_CTXREG_ENABLES_1] |= ENABLE_STENCIL_TEST;
               i830->state.Ctx[I830_CTXREG_ENABLES_2] |= ENABLE_STENCIL_WRITE;
            }
            else {
               i830->state.Ctx[I830_CTXREG_ENABLES_1] &= ~ENABLE_STENCIL_TEST;
               i830->state.Ctx[I830_CTXREG_ENABLES_2] &=
                  ~ENABLE_STENCIL_WRITE;
               i830->state.Ctx[I830_CTXREG_ENABLES_1] |= DISABLE_STENCIL_TEST;
               i830->state.Ctx[I830_CTXREG_ENABLES_2] |=
                  DISABLE_STENCIL_WRITE;
            }
         }
         else {
            FALLBACK(&i830->intel, I830_FALLBACK_STENCIL, state);
         }
      }
      break;

   case GL_POLYGON_STIPPLE:
      /* The stipple command worked on my 855GM box, but not my 845G.
       * I'll do more testing later to find out exactly which hardware
       * supports it.  Disabled for now.
       */
      if (i830->intel.hw_stipple &&
          i830->intel.reduced_primitive == GL_TRIANGLES) {
         I830_STATECHANGE(i830, I830_UPLOAD_STIPPLE);
         i830->state.Stipple[I830_STPREG_ST1] &= ~ST1_ENABLE;
         if (state)
            i830->state.Stipple[I830_STPREG_ST1] |= ST1_ENABLE;
      }
      break;

   default:
      ;
   }
}


static void
i830_init_packets(struct i830_context *i830)
{
   intelScreenPrivate *screen = i830->intel.intelScreen;

   /* Zero all state */
   memset(&i830->state, 0, sizeof(i830->state));

   /* Set default blend state */
   i830->state.TexBlend[0][0] = (_3DSTATE_MAP_BLEND_OP_CMD(0) |
                                 TEXPIPE_COLOR |
                                 ENABLE_TEXOUTPUT_WRT_SEL |
                                 TEXOP_OUTPUT_CURRENT |
                                 DISABLE_TEX_CNTRL_STAGE |
                                 TEXOP_SCALE_1X |
                                 TEXOP_MODIFY_PARMS |
                                 TEXOP_LAST_STAGE | TEXBLENDOP_ARG1);
   i830->state.TexBlend[0][1] = (_3DSTATE_MAP_BLEND_OP_CMD(0) |
                                 TEXPIPE_ALPHA |
                                 ENABLE_TEXOUTPUT_WRT_SEL |
                                 TEXOP_OUTPUT_CURRENT |
                                 TEXOP_SCALE_1X |
                                 TEXOP_MODIFY_PARMS | TEXBLENDOP_ARG1);
   i830->state.TexBlend[0][2] = (_3DSTATE_MAP_BLEND_ARG_CMD(0) |
                                 TEXPIPE_COLOR |
                                 TEXBLEND_ARG1 |
                                 TEXBLENDARG_MODIFY_PARMS |
                                 TEXBLENDARG_DIFFUSE);
   i830->state.TexBlend[0][3] = (_3DSTATE_MAP_BLEND_ARG_CMD(0) |
                                 TEXPIPE_ALPHA |
                                 TEXBLEND_ARG1 |
                                 TEXBLENDARG_MODIFY_PARMS |
                                 TEXBLENDARG_DIFFUSE);

   i830->state.TexBlendWordsUsed[0] = 4;


   i830->state.Ctx[I830_CTXREG_VF] = 0;
   i830->state.Ctx[I830_CTXREG_VF2] = 0;

   i830->state.Ctx[I830_CTXREG_AA] = (_3DSTATE_AA_CMD |
                                      AA_LINE_ECAAR_WIDTH_ENABLE |
                                      AA_LINE_ECAAR_WIDTH_1_0 |
                                      AA_LINE_REGION_WIDTH_ENABLE |
                                      AA_LINE_REGION_WIDTH_1_0 |
                                      AA_LINE_DISABLE);

   i830->state.Ctx[I830_CTXREG_ENABLES_1] = (_3DSTATE_ENABLES_1_CMD |
                                             DISABLE_LOGIC_OP |
                                             DISABLE_STENCIL_TEST |
                                             DISABLE_DEPTH_BIAS |
                                             DISABLE_SPEC_ADD |
                                             DISABLE_FOG |
                                             DISABLE_ALPHA_TEST |
                                             DISABLE_COLOR_BLEND |
                                             DISABLE_DEPTH_TEST);

#if 000                         /* XXX all the stencil enable state is set in i830Enable(), right? */
   if (i830->intel.hw_stencil) {
      i830->state.Ctx[I830_CTXREG_ENABLES_2] = (_3DSTATE_ENABLES_2_CMD |
                                                ENABLE_STENCIL_WRITE |
                                                ENABLE_TEX_CACHE |
                                                ENABLE_DITHER |
                                                ENABLE_COLOR_MASK |
                                                /* set no color comps disabled */
                                                ENABLE_COLOR_WRITE |
                                                ENABLE_DEPTH_WRITE);
   }
   else
#endif
   {
      i830->state.Ctx[I830_CTXREG_ENABLES_2] = (_3DSTATE_ENABLES_2_CMD |
                                                DISABLE_STENCIL_WRITE |
                                                ENABLE_TEX_CACHE |
                                                ENABLE_DITHER |
                                                ENABLE_COLOR_MASK |
                                                /* set no color comps disabled */
                                                ENABLE_COLOR_WRITE |
                                                ENABLE_DEPTH_WRITE);
   }

   i830->state.Ctx[I830_CTXREG_STATE1] = (_3DSTATE_MODES_1_CMD |
                                          ENABLE_COLR_BLND_FUNC |
                                          BLENDFUNC_ADD |
                                          ENABLE_SRC_BLND_FACTOR |
                                          SRC_BLND_FACT(BLENDFACT_ONE) |
                                          ENABLE_DST_BLND_FACTOR |
                                          DST_BLND_FACT(BLENDFACT_ZERO));

   i830->state.Ctx[I830_CTXREG_STATE2] = (_3DSTATE_MODES_2_CMD |
                                          ENABLE_GLOBAL_DEPTH_BIAS |
                                          GLOBAL_DEPTH_BIAS(0) |
                                          ENABLE_ALPHA_TEST_FUNC |
                                          ALPHA_TEST_FUNC(COMPAREFUNC_ALWAYS)
                                          | ALPHA_REF_VALUE(0));

   i830->state.Ctx[I830_CTXREG_STATE3] = (_3DSTATE_MODES_3_CMD |
                                          ENABLE_DEPTH_TEST_FUNC |
                                          DEPTH_TEST_FUNC(COMPAREFUNC_LESS) |
                                          ENABLE_ALPHA_SHADE_MODE |
                                          ALPHA_SHADE_MODE(SHADE_MODE_LINEAR)
                                          | ENABLE_FOG_SHADE_MODE |
                                          FOG_SHADE_MODE(SHADE_MODE_LINEAR) |
                                          ENABLE_SPEC_SHADE_MODE |
                                          SPEC_SHADE_MODE(SHADE_MODE_LINEAR) |
                                          ENABLE_COLOR_SHADE_MODE |
                                          COLOR_SHADE_MODE(SHADE_MODE_LINEAR)
                                          | ENABLE_CULL_MODE | CULLMODE_NONE);

   i830->state.Ctx[I830_CTXREG_STATE4] = (_3DSTATE_MODES_4_CMD |
                                          ENABLE_LOGIC_OP_FUNC |
                                          LOGIC_OP_FUNC(LOGICOP_COPY) |
                                          ENABLE_STENCIL_TEST_MASK |
                                          STENCIL_TEST_MASK(0xff) |
                                          ENABLE_STENCIL_WRITE_MASK |
                                          STENCIL_WRITE_MASK(0xff));

   i830->state.Ctx[I830_CTXREG_STENCILTST] = (_3DSTATE_STENCIL_TEST_CMD |
                                              ENABLE_STENCIL_PARMS |
                                              STENCIL_FAIL_OP(STENCILOP_KEEP)
                                              |
                                              STENCIL_PASS_DEPTH_FAIL_OP
                                              (STENCILOP_KEEP) |
                                              STENCIL_PASS_DEPTH_PASS_OP
                                              (STENCILOP_KEEP) |
                                              ENABLE_STENCIL_TEST_FUNC |
                                              STENCIL_TEST_FUNC
                                              (COMPAREFUNC_ALWAYS) |
                                              ENABLE_STENCIL_REF_VALUE |
                                              STENCIL_REF_VALUE(0));

   i830->state.Ctx[I830_CTXREG_STATE5] = (_3DSTATE_MODES_5_CMD | FLUSH_TEXTURE_CACHE | ENABLE_SPRITE_POINT_TEX | SPRITE_POINT_TEX_OFF | ENABLE_FIXED_LINE_WIDTH | FIXED_LINE_WIDTH(0x2) |       /* 1.0 */
                                          ENABLE_FIXED_POINT_WIDTH |
                                          FIXED_POINT_WIDTH(1));

   i830->state.Ctx[I830_CTXREG_IALPHAB] = (_3DSTATE_INDPT_ALPHA_BLEND_CMD |
                                           DISABLE_INDPT_ALPHA_BLEND |
                                           ENABLE_ALPHA_BLENDFUNC |
                                           ABLENDFUNC_ADD);

   i830->state.Ctx[I830_CTXREG_FOGCOLOR] = (_3DSTATE_FOG_COLOR_CMD |
                                            FOG_COLOR_RED(0) |
                                            FOG_COLOR_GREEN(0) |
                                            FOG_COLOR_BLUE(0));

   i830->state.Ctx[I830_CTXREG_BLENDCOLOR0] = _3DSTATE_CONST_BLEND_COLOR_CMD;
   i830->state.Ctx[I830_CTXREG_BLENDCOLOR1] = 0;

   i830->state.Ctx[I830_CTXREG_MCSB0] = _3DSTATE_MAP_COORD_SETBIND_CMD;
   i830->state.Ctx[I830_CTXREG_MCSB1] = (TEXBIND_SET3(TEXCOORDSRC_VTXSET_3) |
                                         TEXBIND_SET2(TEXCOORDSRC_VTXSET_2) |
                                         TEXBIND_SET1(TEXCOORDSRC_VTXSET_1) |
                                         TEXBIND_SET0(TEXCOORDSRC_VTXSET_0));


   i830->state.Stipple[I830_STPREG_ST0] = _3DSTATE_STIPPLE;

   i830->state.Buffer[I830_DESTREG_CBUFADDR0] = _3DSTATE_BUF_INFO_CMD;
   i830->state.Buffer[I830_DESTREG_CBUFADDR1] = (BUF_3D_ID_COLOR_BACK | BUF_3D_PITCH(screen->front.pitch) |     /* pitch in bytes */
                                                 BUF_3D_USE_FENCE);


   i830->state.Buffer[I830_DESTREG_DBUFADDR0] = _3DSTATE_BUF_INFO_CMD;
   i830->state.Buffer[I830_DESTREG_DBUFADDR1] = (BUF_3D_ID_DEPTH | BUF_3D_PITCH(screen->depth.pitch) |  /* pitch in bytes */
                                                 BUF_3D_USE_FENCE);

   i830->state.Buffer[I830_DESTREG_DV0] = _3DSTATE_DST_BUF_VARS_CMD;

#if 0
   switch (screen->fbFormat) {
   case DV_PF_565:
      i830->state.Buffer[I830_DESTREG_DV1] = (DSTORG_HORT_BIAS(0x8) |   /* .5 */
                                              DSTORG_VERT_BIAS(0x8) |   /* .5 */
                                              screen->fbFormat |
                                              DEPTH_IS_Z |
                                              DEPTH_FRMT_16_FIXED);
      break;
   case DV_PF_8888:
      i830->state.Buffer[I830_DESTREG_DV1] = (DSTORG_HORT_BIAS(0x8) |   /* .5 */
                                              DSTORG_VERT_BIAS(0x8) |   /* .5 */
                                              screen->fbFormat |
                                              DEPTH_IS_Z |
                                              DEPTH_FRMT_24_FIXED_8_OTHER);
      break;
   }
#endif
   i830->state.Buffer[I830_DESTREG_SENABLE] = (_3DSTATE_SCISSOR_ENABLE_CMD |
                                               DISABLE_SCISSOR_RECT);
   i830->state.Buffer[I830_DESTREG_SR0] = _3DSTATE_SCISSOR_RECT_0_CMD;
   i830->state.Buffer[I830_DESTREG_SR1] = 0;
   i830->state.Buffer[I830_DESTREG_SR2] = 0;
}


void
i830InitStateFuncs(struct dd_function_table *functions)
{
   functions->AlphaFunc = i830AlphaFunc;
   functions->BlendColor = i830BlendColor;
   functions->BlendEquationSeparate = i830BlendEquationSeparate;
   functions->BlendFuncSeparate = i830BlendFuncSeparate;
   functions->ColorMask = i830ColorMask;
   functions->CullFace = i830CullFaceFrontFace;
   functions->DepthFunc = i830DepthFunc;
   functions->DepthMask = i830DepthMask;
   functions->Enable = i830Enable;
   functions->Fogfv = i830Fogfv;
   functions->FrontFace = i830CullFaceFrontFace;
   functions->LightModelfv = i830LightModelfv;
   functions->LineWidth = i830LineWidth;
   functions->LogicOpcode = i830LogicOp;
   functions->PointSize = i830PointSize;
   functions->PolygonStipple = i830PolygonStipple;
   functions->Scissor = i830Scissor;
   functions->ShadeModel = i830ShadeModel;
   functions->StencilFuncSeparate = i830StencilFuncSeparate;
   functions->StencilMaskSeparate = i830StencilMaskSeparate;
   functions->StencilOpSeparate = i830StencilOpSeparate;
}

void
i830InitState(struct i830_context *i830)
{
   GLcontext *ctx = &i830->intel.ctx;

   i830_init_packets(i830);

   intelInitState(ctx);

   memcpy(&i830->initial, &i830->state, sizeof(i830->state));

   i830->current = &i830->state;
   i830->state.emitted = 0;
   i830->state.active = (I830_UPLOAD_INVARIENT |
                         I830_UPLOAD_TEXBLEND(0) |
                         I830_UPLOAD_STIPPLE |
                         I830_UPLOAD_CTX | I830_UPLOAD_BUFFERS);
}
