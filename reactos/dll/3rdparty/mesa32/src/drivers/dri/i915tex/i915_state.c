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
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "texmem.h"

#include "intel_fbo.h"
#include "intel_screen.h"
#include "intel_batchbuffer.h"

#include "i915_context.h"
#include "i915_reg.h"

#define FILE_DEBUG_FLAG DEBUG_STATE

static void
i915StencilFuncSeparate(GLcontext * ctx, GLenum face, GLenum func, GLint ref,
                        GLuint mask)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int test = intel_translate_compare_func(func);

   mask = mask & 0xff;

   DBG("%s : func: %s, ref : 0x%x, mask: 0x%x\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(func), ref, mask);


   I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   i915->state.Ctx[I915_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_TEST_MASK;
   i915->state.Ctx[I915_CTXREG_STATE4] |= (ENABLE_STENCIL_TEST_MASK |
                                           STENCIL_TEST_MASK(mask));

   i915->state.Ctx[I915_CTXREG_LIS5] &= ~(S5_STENCIL_REF_MASK |
                                          S5_STENCIL_TEST_FUNC_MASK);

   i915->state.Ctx[I915_CTXREG_LIS5] |= ((ref << S5_STENCIL_REF_SHIFT) |
                                         (test <<
                                          S5_STENCIL_TEST_FUNC_SHIFT));
}

static void
i915StencilMaskSeparate(GLcontext * ctx, GLenum face, GLuint mask)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);

   DBG("%s : mask 0x%x\n", __FUNCTION__, mask);
   
   mask = mask & 0xff;

   I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   i915->state.Ctx[I915_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_WRITE_MASK;
   i915->state.Ctx[I915_CTXREG_STATE4] |= (ENABLE_STENCIL_WRITE_MASK |
                                           STENCIL_WRITE_MASK(mask));
}


static void
i915StencilOpSeparate(GLcontext * ctx, GLenum face, GLenum fail, GLenum zfail,
                      GLenum zpass)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int fop = intel_translate_stencil_op(fail);
   int dfop = intel_translate_stencil_op(zfail);
   int dpop = intel_translate_stencil_op(zpass);


   DBG("%s: fail : %s, zfail: %s, zpass : %s\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(fail),
       _mesa_lookup_enum_by_nr(zfail), _mesa_lookup_enum_by_nr(zpass));

   I915_STATECHANGE(i915, I915_UPLOAD_CTX);

   i915->state.Ctx[I915_CTXREG_LIS5] &= ~(S5_STENCIL_FAIL_MASK |
                                          S5_STENCIL_PASS_Z_FAIL_MASK |
                                          S5_STENCIL_PASS_Z_PASS_MASK);

   i915->state.Ctx[I915_CTXREG_LIS5] |= ((fop << S5_STENCIL_FAIL_SHIFT) |
                                         (dfop <<
                                          S5_STENCIL_PASS_Z_FAIL_SHIFT) |
                                         (dpop <<
                                          S5_STENCIL_PASS_Z_PASS_SHIFT));
}

static void
i915AlphaFunc(GLcontext * ctx, GLenum func, GLfloat ref)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int test = intel_translate_compare_func(func);
   GLubyte refByte;

   UNCLAMPED_FLOAT_TO_UBYTE(refByte, ref);

   I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   i915->state.Ctx[I915_CTXREG_LIS6] &= ~(S6_ALPHA_TEST_FUNC_MASK |
                                          S6_ALPHA_REF_MASK);
   i915->state.Ctx[I915_CTXREG_LIS6] |= ((test << S6_ALPHA_TEST_FUNC_SHIFT) |
                                         (((GLuint) refByte) <<
                                          S6_ALPHA_REF_SHIFT));
}

/* This function makes sure that the proper enables are
 * set for LogicOp, Independant Alpha Blend, and Blending.
 * It needs to be called from numerous places where we
 * could change the LogicOp or Independant Alpha Blend without subsequent
 * calls to glEnable.
 */
static void
i915EvalLogicOpBlendState(GLcontext * ctx)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);

   I915_STATECHANGE(i915, I915_UPLOAD_CTX);

   if (RGBA_LOGICOP_ENABLED(ctx)) {
      i915->state.Ctx[I915_CTXREG_LIS5] |= S5_LOGICOP_ENABLE;
      i915->state.Ctx[I915_CTXREG_LIS6] &= ~S6_CBUF_BLEND_ENABLE;
   }
   else {
      i915->state.Ctx[I915_CTXREG_LIS5] &= ~S5_LOGICOP_ENABLE;

      if (ctx->Color.BlendEnabled) {
         i915->state.Ctx[I915_CTXREG_LIS6] |= S6_CBUF_BLEND_ENABLE;
      }
      else {
         i915->state.Ctx[I915_CTXREG_LIS6] &= ~S6_CBUF_BLEND_ENABLE;
      }
   }
}

static void
i915BlendColor(GLcontext * ctx, const GLfloat color[4])
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLubyte r, g, b, a;

   DBG("%s\n", __FUNCTION__);
   
   UNCLAMPED_FLOAT_TO_UBYTE(r, color[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, color[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(b, color[BCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, color[ACOMP]);

   I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   i915->state.Ctx[I915_CTXREG_BLENDCOLOR1] =
      (a << 24) | (r << 16) | (g << 8) | b;
}


#define DST_BLND_FACT(f) ((f)<<S6_CBUF_DST_BLEND_FACT_SHIFT)
#define SRC_BLND_FACT(f) ((f)<<S6_CBUF_SRC_BLEND_FACT_SHIFT)
#define DST_ABLND_FACT(f) ((f)<<IAB_DST_FACTOR_SHIFT)
#define SRC_ABLND_FACT(f) ((f)<<IAB_SRC_FACTOR_SHIFT)



static GLuint
translate_blend_equation(GLenum mode)
{
   switch (mode) {
   case GL_FUNC_ADD:
      return BLENDFUNC_ADD;
   case GL_MIN:
      return BLENDFUNC_MIN;
   case GL_MAX:
      return BLENDFUNC_MAX;
   case GL_FUNC_SUBTRACT:
      return BLENDFUNC_SUBTRACT;
   case GL_FUNC_REVERSE_SUBTRACT:
      return BLENDFUNC_REVERSE_SUBTRACT;
   default:
      return 0;
   }
}

static void
i915UpdateBlendState(GLcontext * ctx)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLuint iab = (i915->state.Ctx[I915_CTXREG_IAB] &
                 ~(IAB_SRC_FACTOR_MASK |
                   IAB_DST_FACTOR_MASK |
                   (BLENDFUNC_MASK << IAB_FUNC_SHIFT) | IAB_ENABLE));

   GLuint lis6 = (i915->state.Ctx[I915_CTXREG_LIS6] &
                  ~(S6_CBUF_SRC_BLEND_FACT_MASK |
                    S6_CBUF_DST_BLEND_FACT_MASK | S6_CBUF_BLEND_FUNC_MASK));

   GLuint eqRGB = ctx->Color.BlendEquationRGB;
   GLuint eqA = ctx->Color.BlendEquationA;
   GLuint srcRGB = ctx->Color.BlendSrcRGB;
   GLuint dstRGB = ctx->Color.BlendDstRGB;
   GLuint srcA = ctx->Color.BlendSrcA;
   GLuint dstA = ctx->Color.BlendDstA;

   if (eqRGB == GL_MIN || eqRGB == GL_MAX) {
      srcRGB = dstRGB = GL_ONE;
   }

   if (eqA == GL_MIN || eqA == GL_MAX) {
      srcA = dstA = GL_ONE;
   }

   lis6 |= SRC_BLND_FACT(intel_translate_blend_factor(srcRGB));
   lis6 |= DST_BLND_FACT(intel_translate_blend_factor(dstRGB));
   lis6 |= translate_blend_equation(eqRGB) << S6_CBUF_BLEND_FUNC_SHIFT;

   iab |= SRC_ABLND_FACT(intel_translate_blend_factor(srcA));
   iab |= DST_ABLND_FACT(intel_translate_blend_factor(dstA));
   iab |= translate_blend_equation(eqA) << IAB_FUNC_SHIFT;

   if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB)
      iab |= IAB_ENABLE;

   if (iab != i915->state.Ctx[I915_CTXREG_IAB] ||
       lis6 != i915->state.Ctx[I915_CTXREG_LIS6]) {
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      i915->state.Ctx[I915_CTXREG_IAB] = iab;
      i915->state.Ctx[I915_CTXREG_LIS6] = lis6;
   }

   /* This will catch a logicop blend equation */
   i915EvalLogicOpBlendState(ctx);
}


static void
i915BlendFuncSeparate(GLcontext * ctx, GLenum srcRGB,
                      GLenum dstRGB, GLenum srcA, GLenum dstA)
{
   i915UpdateBlendState(ctx);
}


static void
i915BlendEquationSeparate(GLcontext * ctx, GLenum eqRGB, GLenum eqA)
{
   i915UpdateBlendState(ctx);
}


static void
i915DepthFunc(GLcontext * ctx, GLenum func)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int test = intel_translate_compare_func(func);

   DBG("%s\n", __FUNCTION__);
   
   I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   i915->state.Ctx[I915_CTXREG_LIS6] &= ~S6_DEPTH_TEST_FUNC_MASK;
   i915->state.Ctx[I915_CTXREG_LIS6] |= test << S6_DEPTH_TEST_FUNC_SHIFT;
}

static void
i915DepthMask(GLcontext * ctx, GLboolean flag)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);

   DBG("%s flag (%d)\n", __FUNCTION__, flag);
   
   I915_STATECHANGE(i915, I915_UPLOAD_CTX);

   if (flag && ctx->Depth.Test)
      i915->state.Ctx[I915_CTXREG_LIS6] |= S6_DEPTH_WRITE_ENABLE;
   else
      i915->state.Ctx[I915_CTXREG_LIS6] &= ~S6_DEPTH_WRITE_ENABLE;
}

/* =============================================================
 * Polygon stipple
 *
 * The i915 supports a 4x4 stipple natively, GL wants 32x32.
 * Fortunately stipple is usually a repeating pattern.
 */
static void
i915PolygonStipple(GLcontext * ctx, const GLubyte * mask)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   const GLubyte *m = mask;
   GLubyte p[4];
   int i, j, k;
   int active = (ctx->Polygon.StippleFlag &&
                 i915->intel.reduced_primitive == GL_TRIANGLES);
   GLuint newMask;

   if (active) {
      I915_STATECHANGE(i915, I915_UPLOAD_STIPPLE);
      i915->state.Stipple[I915_STPREG_ST1] &= ~ST1_ENABLE;
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
               i915->intel.hw_stipple = 0;
               return;
            }

   newMask = (((p[0] & 0xf) << 0) |
              ((p[1] & 0xf) << 4) |
              ((p[2] & 0xf) << 8) | ((p[3] & 0xf) << 12));


   if (newMask == 0xffff || newMask == 0x0) {
      /* this is needed to make conform pass */
      i915->intel.hw_stipple = 0;
      return;
   }

   i915->state.Stipple[I915_STPREG_ST1] &= ~0xffff;
   i915->state.Stipple[I915_STPREG_ST1] |= newMask;
   i915->intel.hw_stipple = 1;

   if (active)
      i915->state.Stipple[I915_STPREG_ST1] |= ST1_ENABLE;
}


/* =============================================================
 * Hardware clipping
 */
static void
i915Scissor(GLcontext * ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
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

   I915_STATECHANGE(i915, I915_UPLOAD_BUFFERS);
   i915->state.Buffer[I915_DESTREG_SR1] = (y1 << 16) | (x1 & 0xffff);
   i915->state.Buffer[I915_DESTREG_SR2] = (y2 << 16) | (x2 & 0xffff);
}

static void
i915LogicOp(GLcontext * ctx, GLenum opcode)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int tmp = intel_translate_logic_op(opcode);

   DBG("%s\n", __FUNCTION__);
   
   I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   i915->state.Ctx[I915_CTXREG_STATE4] &= ~LOGICOP_MASK;
   i915->state.Ctx[I915_CTXREG_STATE4] |= LOGIC_OP_FUNC(tmp);
}



static void
i915CullFaceFrontFace(GLcontext * ctx, GLenum unused)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLuint mode;

   DBG("%s %d\n", __FUNCTION__,
       ctx->DrawBuffer ? ctx->DrawBuffer->Name : 0);

   if (!ctx->Polygon.CullFlag) {
      mode = S4_CULLMODE_NONE;
   }
   else if (ctx->Polygon.CullFaceMode != GL_FRONT_AND_BACK) {
      mode = S4_CULLMODE_CW;

      if (ctx->DrawBuffer && ctx->DrawBuffer->Name != 0)
         mode ^= (S4_CULLMODE_CW ^ S4_CULLMODE_CCW);
      if (ctx->Polygon.CullFaceMode == GL_FRONT)
         mode ^= (S4_CULLMODE_CW ^ S4_CULLMODE_CCW);
      if (ctx->Polygon.FrontFace != GL_CCW)
         mode ^= (S4_CULLMODE_CW ^ S4_CULLMODE_CCW);
   }
   else {
      mode = S4_CULLMODE_BOTH;
   }

   I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   i915->state.Ctx[I915_CTXREG_LIS4] &= ~S4_CULLMODE_MASK;
   i915->state.Ctx[I915_CTXREG_LIS4] |= mode;
}

static void
i915LineWidth(GLcontext * ctx, GLfloat widthf)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int lis4 = i915->state.Ctx[I915_CTXREG_LIS4] & ~S4_LINE_WIDTH_MASK;
   int width;

   DBG("%s\n", __FUNCTION__);
   
   width = (int) (widthf * 2);
   CLAMP_SELF(width, 1, 0xf);
   lis4 |= width << S4_LINE_WIDTH_SHIFT;

   if (lis4 != i915->state.Ctx[I915_CTXREG_LIS4]) {
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      i915->state.Ctx[I915_CTXREG_LIS4] = lis4;
   }
}

static void
i915PointSize(GLcontext * ctx, GLfloat size)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int lis4 = i915->state.Ctx[I915_CTXREG_LIS4] & ~S4_POINT_WIDTH_MASK;
   GLint point_size = (int) size;

   DBG("%s\n", __FUNCTION__);
   
   CLAMP_SELF(point_size, 1, 255);
   lis4 |= point_size << S4_POINT_WIDTH_SHIFT;

   if (lis4 != i915->state.Ctx[I915_CTXREG_LIS4]) {
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      i915->state.Ctx[I915_CTXREG_LIS4] = lis4;
   }
}


/* =============================================================
 * Color masks
 */

static void
i915ColorMask(GLcontext * ctx,
              GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLuint tmp = i915->state.Ctx[I915_CTXREG_LIS5] & ~S5_WRITEDISABLE_MASK;

   DBG("%s r(%d) g(%d) b(%d) a(%d)\n", __FUNCTION__, r, g, b,
       a);

   if (!r)
      tmp |= S5_WRITEDISABLE_RED;
   if (!g)
      tmp |= S5_WRITEDISABLE_GREEN;
   if (!b)
      tmp |= S5_WRITEDISABLE_BLUE;
   if (!a)
      tmp |= S5_WRITEDISABLE_ALPHA;

   if (tmp != i915->state.Ctx[I915_CTXREG_LIS5]) {
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      i915->state.Ctx[I915_CTXREG_LIS5] = tmp;
   }
}

static void
update_specular(GLcontext * ctx)
{
   /* A hack to trigger the rebuild of the fragment program.
    */
   intel_context(ctx)->NewGLState |= _NEW_TEXTURE;
}

static void
i915LightModelfv(GLcontext * ctx, GLenum pname, const GLfloat * param)
{
   DBG("%s\n", __FUNCTION__);
   
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
      update_specular(ctx);
   }
}

static void
i915ShadeModel(GLcontext * ctx, GLenum mode)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   I915_STATECHANGE(i915, I915_UPLOAD_CTX);

   if (mode == GL_SMOOTH) {
      i915->state.Ctx[I915_CTXREG_LIS4] &= ~(S4_FLATSHADE_ALPHA |
                                             S4_FLATSHADE_COLOR |
                                             S4_FLATSHADE_SPECULAR);
   }
   else {
      i915->state.Ctx[I915_CTXREG_LIS4] |= (S4_FLATSHADE_ALPHA |
                                            S4_FLATSHADE_COLOR |
                                            S4_FLATSHADE_SPECULAR);
   }
}

/* =============================================================
 * Fog
 */
void
i915_update_fog(GLcontext * ctx)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLenum mode;
   GLboolean enabled;
   GLboolean try_pixel_fog;

   if (ctx->FragmentProgram._Active) {
      /* Pull in static fog state from program */
      mode = ctx->FragmentProgram._Current->FogOption;
      enabled = (mode != GL_NONE);
      try_pixel_fog = 0;
   }
   else {
      enabled = ctx->Fog.Enabled;
      mode = ctx->Fog.Mode;
#if 0
      /* XXX - DISABLED -- Need ortho fallback */
      try_pixel_fog = (ctx->Fog.FogCoordinateSource == GL_FRAGMENT_DEPTH_EXT
                       && ctx->Hint.Fog == GL_NICEST);
#else
      try_pixel_fog = 0;
#endif
   }

   if (!enabled) {
      i915->vertex_fog = I915_FOG_NONE;
   }
   else if (try_pixel_fog) {
      I915_STATECHANGE(i915, I915_UPLOAD_FOG);
      i915->state.Fog[I915_FOGREG_MODE1] &= ~FMC1_FOGFUNC_MASK;
      i915->vertex_fog = I915_FOG_PIXEL;

      switch (mode) {
      case GL_LINEAR:
         if (ctx->Fog.End <= ctx->Fog.Start) {
            /* XXX - this won't work with fragment programs.  Need to
             * either fallback or append fog instructions to end of
             * program in the case of linear fog.
             */
            printf("vertex fog!\n");
            i915->state.Fog[I915_FOGREG_MODE1] |= FMC1_FOGFUNC_VERTEX;
            i915->vertex_fog = I915_FOG_VERTEX;
         }
         else {
            GLfloat c2 = 1.0 / (ctx->Fog.End - ctx->Fog.Start);
            GLfloat c1 = ctx->Fog.End * c2;

            i915->state.Fog[I915_FOGREG_MODE1] &= ~FMC1_C1_MASK;
            i915->state.Fog[I915_FOGREG_MODE1] |= FMC1_FOGFUNC_PIXEL_LINEAR;
            i915->state.Fog[I915_FOGREG_MODE1] |=
               ((GLuint) (c1 * FMC1_C1_ONE)) & FMC1_C1_MASK;

            if (i915->state.Fog[I915_FOGREG_MODE1] & FMC1_FOGINDEX_Z) {
               i915->state.Fog[I915_FOGREG_MODE2]
                  = (GLuint) (c2 * FMC2_C2_ONE);
            }
            else {
               fi_type fi;
               fi.f = c2;
               i915->state.Fog[I915_FOGREG_MODE2] = fi.i;
            }
         }
         break;
      case GL_EXP:
         i915->state.Fog[I915_FOGREG_MODE1] |= FMC1_FOGFUNC_PIXEL_EXP;
         break;
      case GL_EXP2:
         i915->state.Fog[I915_FOGREG_MODE1] |= FMC1_FOGFUNC_PIXEL_EXP2;
         break;
      default:
         break;
      }
   }
   else { /* if (i915->vertex_fog != I915_FOG_VERTEX) */
      I915_STATECHANGE(i915, I915_UPLOAD_FOG);
      i915->state.Fog[I915_FOGREG_MODE1] &= ~FMC1_FOGFUNC_MASK;
      i915->state.Fog[I915_FOGREG_MODE1] |= FMC1_FOGFUNC_VERTEX;
      i915->vertex_fog = I915_FOG_VERTEX;
   }

   I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   I915_ACTIVESTATE(i915, I915_UPLOAD_FOG, enabled);
   if (enabled)
      i915->state.Ctx[I915_CTXREG_LIS5] |= S5_FOG_ENABLE;
   else
      i915->state.Ctx[I915_CTXREG_LIS5] &= ~S5_FOG_ENABLE;

   /* Always enable pixel fog.  Vertex fog using fog coord will conflict
    * with fog code appended onto fragment program.
    */
    _tnl_allow_vertex_fog( ctx, 0 );
    _tnl_allow_pixel_fog( ctx, 1 );
}

static void
i915Fogfv(GLcontext * ctx, GLenum pname, const GLfloat * param)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);

   switch (pname) {
   case GL_FOG_COORDINATE_SOURCE_EXT:
   case GL_FOG_MODE:
   case GL_FOG_START:
   case GL_FOG_END:
      break;

   case GL_FOG_DENSITY:
      I915_STATECHANGE(i915, I915_UPLOAD_FOG);

      if (i915->state.Fog[I915_FOGREG_MODE1] & FMC1_FOGINDEX_Z) {
         i915->state.Fog[I915_FOGREG_MODE3] =
            (GLuint) (ctx->Fog.Density * FMC3_D_ONE);
      }
      else {
         fi_type fi;
         fi.f = ctx->Fog.Density;
         i915->state.Fog[I915_FOGREG_MODE3] = fi.i;
      }
      break;

   case GL_FOG_COLOR:
      I915_STATECHANGE(i915, I915_UPLOAD_FOG);
      i915->state.Fog[I915_FOGREG_COLOR] =
         (_3DSTATE_FOG_COLOR_CMD |
          ((GLubyte) (ctx->Fog.Color[0] * 255.0F) << 16) |
          ((GLubyte) (ctx->Fog.Color[1] * 255.0F) << 8) |
          ((GLubyte) (ctx->Fog.Color[2] * 255.0F) << 0));
      break;

   default:
      break;
   }
}

static void
i915Hint(GLcontext * ctx, GLenum target, GLenum state)
{
   switch (target) {
   case GL_FOG_HINT:
      break;
   default:
      break;
   }
}

/* =============================================================
 */

static void
i915Enable(GLcontext * ctx, GLenum cap, GLboolean state)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);

   switch (cap) {
   case GL_TEXTURE_2D:
      break;

   case GL_LIGHTING:
   case GL_COLOR_SUM:
      update_specular(ctx);
      break;

   case GL_ALPHA_TEST:
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      if (state)
         i915->state.Ctx[I915_CTXREG_LIS6] |= S6_ALPHA_TEST_ENABLE;
      else
         i915->state.Ctx[I915_CTXREG_LIS6] &= ~S6_ALPHA_TEST_ENABLE;
      break;

   case GL_BLEND:
      i915EvalLogicOpBlendState(ctx);
      break;

   case GL_COLOR_LOGIC_OP:
      i915EvalLogicOpBlendState(ctx);

      /* Logicop doesn't seem to work at 16bpp:
       */
      if (i915->intel.intelScreen->cpp == 2)    /* XXX FBO fix */
         FALLBACK(&i915->intel, I915_FALLBACK_LOGICOP, state);
      break;

   case GL_FRAGMENT_PROGRAM_ARB:
      break;

   case GL_DITHER:
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      if (state)
         i915->state.Ctx[I915_CTXREG_LIS5] |= S5_COLOR_DITHER_ENABLE;
      else
         i915->state.Ctx[I915_CTXREG_LIS5] &= ~S5_COLOR_DITHER_ENABLE;
      break;

   case GL_DEPTH_TEST:
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      if (state)
         i915->state.Ctx[I915_CTXREG_LIS6] |= S6_DEPTH_TEST_ENABLE;
      else
         i915->state.Ctx[I915_CTXREG_LIS6] &= ~S6_DEPTH_TEST_ENABLE;

      i915DepthMask(ctx, ctx->Depth.Mask);
      break;

   case GL_SCISSOR_TEST:
      I915_STATECHANGE(i915, I915_UPLOAD_BUFFERS);
      if (state)
         i915->state.Buffer[I915_DESTREG_SENABLE] =
            (_3DSTATE_SCISSOR_ENABLE_CMD | ENABLE_SCISSOR_RECT);
      else
         i915->state.Buffer[I915_DESTREG_SENABLE] =
            (_3DSTATE_SCISSOR_ENABLE_CMD | DISABLE_SCISSOR_RECT);
      break;

   case GL_LINE_SMOOTH:
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      if (state)
         i915->state.Ctx[I915_CTXREG_LIS4] |= S4_LINE_ANTIALIAS_ENABLE;
      else
         i915->state.Ctx[I915_CTXREG_LIS4] &= ~S4_LINE_ANTIALIAS_ENABLE;
      break;

   case GL_FOG:
      break;

   case GL_CULL_FACE:
      i915CullFaceFrontFace(ctx, 0);
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
            I915_STATECHANGE(i915, I915_UPLOAD_CTX);
            if (state)
               i915->state.Ctx[I915_CTXREG_LIS5] |= (S5_STENCIL_TEST_ENABLE |
                                                     S5_STENCIL_WRITE_ENABLE);
            else
               i915->state.Ctx[I915_CTXREG_LIS5] &= ~(S5_STENCIL_TEST_ENABLE |
                                                      S5_STENCIL_WRITE_ENABLE);
         }
         else {
            FALLBACK(&i915->intel, I915_FALLBACK_STENCIL, state);
         }
      }
      break;

   case GL_POLYGON_STIPPLE:
      /* The stipple command worked on my 855GM box, but not my 845G.
       * I'll do more testing later to find out exactly which hardware
       * supports it.  Disabled for now.
       */
      if (i915->intel.hw_stipple &&
          i915->intel.reduced_primitive == GL_TRIANGLES) {
         I915_STATECHANGE(i915, I915_UPLOAD_STIPPLE);
         if (state)
            i915->state.Stipple[I915_STPREG_ST1] |= ST1_ENABLE;
         else
            i915->state.Stipple[I915_STPREG_ST1] &= ~ST1_ENABLE;
      }
      break;

   case GL_POLYGON_SMOOTH:
      break;

   case GL_POINT_SMOOTH:
      break;

   default:
      ;
   }
}


static void
i915_init_packets(struct i915_context *i915)
{
   intelScreenPrivate *screen = i915->intel.intelScreen;

   /* Zero all state */
   memset(&i915->state, 0, sizeof(i915->state));


   {
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      /* Probably don't want to upload all this stuff every time one 
       * piece changes.
       */
      i915->state.Ctx[I915_CTXREG_LI] = (_3DSTATE_LOAD_STATE_IMMEDIATE_1 |
                                         I1_LOAD_S(2) |
                                         I1_LOAD_S(4) |
                                         I1_LOAD_S(5) | I1_LOAD_S(6) | (3));
      i915->state.Ctx[I915_CTXREG_LIS2] = 0;
      i915->state.Ctx[I915_CTXREG_LIS4] = 0;
      i915->state.Ctx[I915_CTXREG_LIS5] = 0;

      if (screen->cpp == 2)     /* XXX FBO fix */
         i915->state.Ctx[I915_CTXREG_LIS5] |= S5_COLOR_DITHER_ENABLE;


      i915->state.Ctx[I915_CTXREG_LIS6] = (S6_COLOR_WRITE_ENABLE |
                                           (2 << S6_TRISTRIP_PV_SHIFT));

      i915->state.Ctx[I915_CTXREG_STATE4] = (_3DSTATE_MODES_4_CMD |
                                             ENABLE_LOGIC_OP_FUNC |
                                             LOGIC_OP_FUNC(LOGICOP_COPY) |
                                             ENABLE_STENCIL_TEST_MASK |
                                             STENCIL_TEST_MASK(0xff) |
                                             ENABLE_STENCIL_WRITE_MASK |
                                             STENCIL_WRITE_MASK(0xff));

      i915->state.Ctx[I915_CTXREG_IAB] =
         (_3DSTATE_INDEPENDENT_ALPHA_BLEND_CMD | IAB_MODIFY_ENABLE |
          IAB_MODIFY_FUNC | IAB_MODIFY_SRC_FACTOR | IAB_MODIFY_DST_FACTOR);

      i915->state.Ctx[I915_CTXREG_BLENDCOLOR0] =
         _3DSTATE_CONST_BLEND_COLOR_CMD;
      i915->state.Ctx[I915_CTXREG_BLENDCOLOR1] = 0;

   }

   {
      I915_STATECHANGE(i915, I915_UPLOAD_STIPPLE);
      i915->state.Stipple[I915_STPREG_ST0] = _3DSTATE_STIPPLE;
   }


   {
      I915_STATECHANGE(i915, I915_UPLOAD_FOG);
      i915->state.Fog[I915_FOGREG_MODE0] = _3DSTATE_FOG_MODE_CMD;
      i915->state.Fog[I915_FOGREG_MODE1] = (FMC1_FOGFUNC_MODIFY_ENABLE |
                                            FMC1_FOGFUNC_VERTEX |
                                            FMC1_FOGINDEX_MODIFY_ENABLE |
                                            FMC1_FOGINDEX_W |
                                            FMC1_C1_C2_MODIFY_ENABLE |
                                            FMC1_DENSITY_MODIFY_ENABLE);
      i915->state.Fog[I915_FOGREG_COLOR] = _3DSTATE_FOG_COLOR_CMD;
   }


   {
      I915_STATECHANGE(i915, I915_UPLOAD_BUFFERS);
      /* color buffer offset/stride */
      i915->state.Buffer[I915_DESTREG_CBUFADDR0] = _3DSTATE_BUF_INFO_CMD;
      /* XXX FBO: remove this?  Also get set in i915_set_draw_region() */
      i915->state.Buffer[I915_DESTREG_CBUFADDR1] = (BUF_3D_ID_COLOR_BACK | BUF_3D_PITCH(screen->front.pitch) |  /* pitch in bytes */
                                                    BUF_3D_USE_FENCE);

      i915->state.Buffer[I915_DESTREG_DBUFADDR0] = _3DSTATE_BUF_INFO_CMD;
      /* XXX FBO: remove this?  Also get set in i915_set_draw_region() */
      i915->state.Buffer[I915_DESTREG_DBUFADDR1] = (BUF_3D_ID_DEPTH | BUF_3D_PITCH(screen->depth.pitch) |       /* pitch in bytes */
                                                    BUF_3D_USE_FENCE);

      i915->state.Buffer[I915_DESTREG_DV0] = _3DSTATE_DST_BUF_VARS_CMD;

      /* XXX FBO: remove this?  Also get set in i915_set_draw_region() */
#if 0                           /* seems we don't need this */
      switch (screen->fbFormat) {
      case DV_PF_565:
         i915->state.Buffer[I915_DESTREG_DV1] = (DSTORG_HORT_BIAS(0x8) |        /* .5 */
                                                 DSTORG_VERT_BIAS(0x8) |        /* .5 */
                                                 LOD_PRECLAMP_OGL |
                                                 TEX_DEFAULT_COLOR_OGL |
                                                 DITHER_FULL_ALWAYS |
                                                 screen->fbFormat |
                                                 DEPTH_FRMT_16_FIXED);
         break;
      case DV_PF_8888:
         i915->state.Buffer[I915_DESTREG_DV1] = (DSTORG_HORT_BIAS(0x8) |        /* .5 */
                                                 DSTORG_VERT_BIAS(0x8) |        /* .5 */
                                                 LOD_PRECLAMP_OGL |
                                                 TEX_DEFAULT_COLOR_OGL |
                                                 screen->fbFormat |
                                                 DEPTH_FRMT_24_FIXED_8_OTHER);
         break;
      }
#endif


      /* scissor */
      i915->state.Buffer[I915_DESTREG_SENABLE] =
         (_3DSTATE_SCISSOR_ENABLE_CMD | DISABLE_SCISSOR_RECT);
      i915->state.Buffer[I915_DESTREG_SR0] = _3DSTATE_SCISSOR_RECT_0_CMD;
      i915->state.Buffer[I915_DESTREG_SR1] = 0;
      i915->state.Buffer[I915_DESTREG_SR2] = 0;
   }


#if 0
   {
      I915_STATECHANGE(i915, I915_UPLOAD_DEFAULTS);
      i915->state.Default[I915_DEFREG_C0] = _3DSTATE_DEFAULT_DIFFUSE;
      i915->state.Default[I915_DEFREG_C1] = 0;
      i915->state.Default[I915_DEFREG_S0] = _3DSTATE_DEFAULT_SPECULAR;
      i915->state.Default[I915_DEFREG_S1] = 0;
      i915->state.Default[I915_DEFREG_Z0] = _3DSTATE_DEFAULT_Z;
      i915->state.Default[I915_DEFREG_Z1] = 0;
   }
#endif


   /* These will be emitted every at the head of every buffer, unless
    * we get hardware contexts working.
    */
   i915->state.active = (I915_UPLOAD_PROGRAM |
                         I915_UPLOAD_STIPPLE |
                         I915_UPLOAD_CTX |
                         I915_UPLOAD_BUFFERS | I915_UPLOAD_INVARIENT);
}

void
i915InitStateFunctions(struct dd_function_table *functions)
{
   functions->AlphaFunc = i915AlphaFunc;
   functions->BlendColor = i915BlendColor;
   functions->BlendEquationSeparate = i915BlendEquationSeparate;
   functions->BlendFuncSeparate = i915BlendFuncSeparate;
   functions->ColorMask = i915ColorMask;
   functions->CullFace = i915CullFaceFrontFace;
   functions->DepthFunc = i915DepthFunc;
   functions->DepthMask = i915DepthMask;
   functions->Enable = i915Enable;
   functions->Fogfv = i915Fogfv;
   functions->FrontFace = i915CullFaceFrontFace;
   functions->Hint = i915Hint;
   functions->LightModelfv = i915LightModelfv;
   functions->LineWidth = i915LineWidth;
   functions->LogicOpcode = i915LogicOp;
   functions->PointSize = i915PointSize;
   functions->PolygonStipple = i915PolygonStipple;
   functions->Scissor = i915Scissor;
   functions->ShadeModel = i915ShadeModel;
   functions->StencilFuncSeparate = i915StencilFuncSeparate;
   functions->StencilMaskSeparate = i915StencilMaskSeparate;
   functions->StencilOpSeparate = i915StencilOpSeparate;
}


void
i915InitState(struct i915_context *i915)
{
   GLcontext *ctx = &i915->intel.ctx;

   i915_init_packets(i915);

   intelInitState(ctx);

   memcpy(&i915->initial, &i915->state, sizeof(i915->state));
   i915->current = &i915->state;
}
