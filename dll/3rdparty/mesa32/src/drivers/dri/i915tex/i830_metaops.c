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
#include "enums.h"
#include "mtypes.h"
#include "macros.h"
#include "utils.h"

#include "intel_screen.h"
#include "intel_batchbuffer.h"
#include "intel_ioctl.h"
#include "intel_regions.h"

#include "i830_context.h"
#include "i830_reg.h"

/* A large amount of state doesn't need to be uploaded.
 */
#define ACTIVE (I830_UPLOAD_INVARIENT |         \
		I830_UPLOAD_CTX |		\
		I830_UPLOAD_BUFFERS |		\
		I830_UPLOAD_STIPPLE |		\
		I830_UPLOAD_TEXBLEND(0) |	\
		I830_UPLOAD_TEX(0))


#define SET_STATE( i830, STATE )		\
do {						\
   i830->current->emitted &= ~ACTIVE;			\
   i830->current = &i830->STATE;		\
   i830->current->emitted &= ~ACTIVE;			\
} while (0)


static void
set_no_stencil_write(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_STENCIL_TEST, GL_FALSE )
    */
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] &= ~ENABLE_STENCIL_TEST;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] &= ~ENABLE_STENCIL_WRITE;
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] |= DISABLE_STENCIL_TEST;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] |= DISABLE_STENCIL_WRITE;

   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}

static void
set_no_depth_write(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_DEPTH_TEST, GL_FALSE )
    */
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] &= ~ENABLE_DIS_DEPTH_TEST_MASK;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] &= ~ENABLE_DIS_DEPTH_WRITE_MASK;
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] |= DISABLE_DEPTH_TEST;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] |= DISABLE_DEPTH_WRITE;

   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}

/* Set depth unit to replace.
 */
static void
set_depth_replace(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_DEPTH_TEST, GL_FALSE )
    * ctx->Driver.DepthMask( ctx, GL_TRUE )
    */
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] &= ~ENABLE_DIS_DEPTH_TEST_MASK;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] &= ~ENABLE_DIS_DEPTH_WRITE_MASK;
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] |= ENABLE_DEPTH_TEST;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] |= ENABLE_DEPTH_WRITE;

   /* ctx->Driver.DepthFunc( ctx, GL_ALWAYS )
    */
   i830->meta.Ctx[I830_CTXREG_STATE3] &= ~DEPTH_TEST_FUNC_MASK;
   i830->meta.Ctx[I830_CTXREG_STATE3] |= (ENABLE_DEPTH_TEST_FUNC |
                                          DEPTH_TEST_FUNC
                                          (COMPAREFUNC_ALWAYS));

   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}


/* Set stencil unit to replace always with the reference value.
 */
static void
set_stencil_replace(struct intel_context *intel,
                    GLuint s_mask, GLuint s_clear)
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_STENCIL_TEST, GL_TRUE )
    */
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] |= ENABLE_STENCIL_TEST;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] |= ENABLE_STENCIL_WRITE;

   /* ctx->Driver.StencilMask( ctx, s_mask )
    */
   i830->meta.Ctx[I830_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_WRITE_MASK;
   i830->meta.Ctx[I830_CTXREG_STATE4] |= (ENABLE_STENCIL_WRITE_MASK |
                                          STENCIL_WRITE_MASK((s_mask &
                                                              0xff)));

   /* ctx->Driver.StencilOp( ctx, GL_REPLACE, GL_REPLACE, GL_REPLACE )
    */
   i830->meta.Ctx[I830_CTXREG_STENCILTST] &= ~(STENCIL_OPS_MASK);
   i830->meta.Ctx[I830_CTXREG_STENCILTST] |=
      (ENABLE_STENCIL_PARMS |
       STENCIL_FAIL_OP(STENCILOP_REPLACE) |
       STENCIL_PASS_DEPTH_FAIL_OP(STENCILOP_REPLACE) |
       STENCIL_PASS_DEPTH_PASS_OP(STENCILOP_REPLACE));

   /* ctx->Driver.StencilFunc( ctx, GL_ALWAYS, s_clear, ~0 )
    */
   i830->meta.Ctx[I830_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_TEST_MASK;
   i830->meta.Ctx[I830_CTXREG_STATE4] |= (ENABLE_STENCIL_TEST_MASK |
                                          STENCIL_TEST_MASK(0xff));

   i830->meta.Ctx[I830_CTXREG_STENCILTST] &= ~(STENCIL_REF_VALUE_MASK |
                                               ENABLE_STENCIL_TEST_FUNC_MASK);
   i830->meta.Ctx[I830_CTXREG_STENCILTST] |=
      (ENABLE_STENCIL_REF_VALUE |
       ENABLE_STENCIL_TEST_FUNC |
       STENCIL_REF_VALUE((s_clear & 0xff)) |
       STENCIL_TEST_FUNC(COMPAREFUNC_ALWAYS));



   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}


static void
set_color_mask(struct intel_context *intel, GLboolean state)
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   const GLuint mask = ((1 << WRITEMASK_RED_SHIFT) |
                        (1 << WRITEMASK_GREEN_SHIFT) |
                        (1 << WRITEMASK_BLUE_SHIFT) |
                        (1 << WRITEMASK_ALPHA_SHIFT));

   i830->meta.Ctx[I830_CTXREG_ENABLES_2] &= ~mask;

   if (state) {
      i830->meta.Ctx[I830_CTXREG_ENABLES_2] |=
         (i830->state.Ctx[I830_CTXREG_ENABLES_2] & mask);
   }

   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}

/* Installs a one-stage passthrough texture blend pipeline.  Is there
 * more that can be done to turn off texturing?
 */
static void
set_no_texture(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   static const struct gl_tex_env_combine_state comb = {
      GL_NONE, GL_NONE,
      {GL_TEXTURE, 0, 0,}, {GL_TEXTURE, 0, 0,},
      {GL_SRC_COLOR, 0, 0}, {GL_SRC_ALPHA, 0, 0},
      0, 0, 0, 0
   };

   i830->meta.TexBlendWordsUsed[0] =
      i830SetTexEnvCombine(i830, &comb, 0, TEXBLENDARG_TEXEL0,
                           i830->meta.TexBlend[0], NULL);

   i830->meta.TexBlend[0][0] |= TEXOP_LAST_STAGE;
   i830->meta.emitted &= ~I830_UPLOAD_TEXBLEND(0);
}

/* Set up a single element blend stage for 'replace' texturing with no
 * funny ops.
 */
static void
set_texture_blend_replace(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   static const struct gl_tex_env_combine_state comb = {
      GL_REPLACE, GL_REPLACE,
      {GL_TEXTURE, GL_TEXTURE, GL_TEXTURE,}, {GL_TEXTURE, GL_TEXTURE,
                                              GL_TEXTURE,},
      {GL_SRC_COLOR, GL_SRC_COLOR, GL_SRC_COLOR}, {GL_SRC_ALPHA, GL_SRC_ALPHA,
                                                   GL_SRC_ALPHA},
      0, 0, 1, 1
   };

   i830->meta.TexBlendWordsUsed[0] =
      i830SetTexEnvCombine(i830, &comb, 0, TEXBLENDARG_TEXEL0,
                           i830->meta.TexBlend[0], NULL);

   i830->meta.TexBlend[0][0] |= TEXOP_LAST_STAGE;
   i830->meta.emitted &= ~I830_UPLOAD_TEXBLEND(0);

/*    fprintf(stderr, "%s: TexBlendWordsUsed[0]: %d\n",  */
/* 	   __FUNCTION__, i830->meta.TexBlendWordsUsed[0]); */
}



/* Set up an arbitary piece of memory as a rectangular texture
 * (including the front or back buffer).
 */
static GLboolean
set_tex_rect_source(struct intel_context *intel,
                    struct _DriBufferObject *buffer,
                    GLuint offset,
                    GLuint pitch, GLuint height, GLenum format, GLenum type)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   GLuint *setup = i830->meta.Tex[0];
   GLint numLevels = 1;
   GLuint textureFormat;
   GLuint cpp;

   /* A full implementation of this would do the upload through
    * glTexImage2d, and get all the conversion operations at that
    * point.  We are restricted, but still at least have access to the
    * fragment program swizzle.
    */
   switch (format) {
   case GL_BGRA:
      switch (type) {
      case GL_UNSIGNED_INT_8_8_8_8_REV:
      case GL_UNSIGNED_BYTE:
         textureFormat = (MAPSURF_32BIT | MT_32BIT_ARGB8888);
         cpp = 4;
         break;
      default:
         return GL_FALSE;
      }
      break;
   case GL_RGBA:
      switch (type) {
      case GL_UNSIGNED_INT_8_8_8_8_REV:
      case GL_UNSIGNED_BYTE:
         textureFormat = (MAPSURF_32BIT | MT_32BIT_ABGR8888);
         cpp = 4;
         break;
      default:
         return GL_FALSE;
      }
      break;
   case GL_BGR:
      switch (type) {
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         textureFormat = (MAPSURF_16BIT | MT_16BIT_RGB565);
         cpp = 2;
         break;
      default:
         return GL_FALSE;
      }
      break;
   case GL_RGB:
      switch (type) {
      case GL_UNSIGNED_SHORT_5_6_5:
         textureFormat = (MAPSURF_16BIT | MT_16BIT_RGB565);
         cpp = 2;
         break;
      default:
         return GL_FALSE;
      }
      break;

   default:
      return GL_FALSE;
   }

   i830->meta.tex_buffer[0] = buffer;
   i830->meta.tex_offset[0] = offset;

   setup[I830_TEXREG_TM0LI] = (_3DSTATE_LOAD_STATE_IMMEDIATE_2 |
                               (LOAD_TEXTURE_MAP0 << 0) | 4);
   setup[I830_TEXREG_TM0S1] = (((height - 1) << TM0S1_HEIGHT_SHIFT) |
                               ((pitch - 1) << TM0S1_WIDTH_SHIFT) |
                               textureFormat);
   setup[I830_TEXREG_TM0S2] =
      (((((pitch * cpp) / 4) -
         1) << TM0S2_PITCH_SHIFT) | TM0S2_CUBE_FACE_ENA_MASK);

   setup[I830_TEXREG_TM0S3] =
      ((((numLevels -
          1) *
         4) << TM0S3_MIN_MIP_SHIFT) | (FILTER_NEAREST <<
                                       TM0S3_MIN_FILTER_SHIFT) |
       (MIPFILTER_NONE << TM0S3_MIP_FILTER_SHIFT) | (FILTER_NEAREST <<
                                                     TM0S3_MAG_FILTER_SHIFT));

   setup[I830_TEXREG_CUBE] = (_3DSTATE_MAP_CUBE | MAP_UNIT(0));

   setup[I830_TEXREG_MCS] = (_3DSTATE_MAP_COORD_SET_CMD |
                             MAP_UNIT(0) |
                             ENABLE_TEXCOORD_PARAMS |
                             TEXCOORDS_ARE_IN_TEXELUNITS |
                             TEXCOORDTYPE_CARTESIAN |
                             ENABLE_ADDR_V_CNTL |
                             TEXCOORD_ADDR_V_MODE(TEXCOORDMODE_WRAP) |
                             ENABLE_ADDR_U_CNTL |
                             TEXCOORD_ADDR_U_MODE(TEXCOORDMODE_WRAP));

   i830->meta.emitted &= ~I830_UPLOAD_TEX(0);
   return GL_TRUE;
}


static void
set_vertex_format(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   i830->meta.Ctx[I830_CTXREG_VF] = (_3DSTATE_VFT0_CMD |
                                     VFT0_TEX_COUNT(1) |
                                     VFT0_DIFFUSE | VFT0_XYZ);
   i830->meta.Ctx[I830_CTXREG_VF2] = (_3DSTATE_VFT1_CMD |
                                      VFT1_TEX0_FMT(TEXCOORDFMT_2D) |
                                      VFT1_TEX1_FMT(TEXCOORDFMT_2D) |
                                      VFT1_TEX2_FMT(TEXCOORDFMT_2D) |
                                      VFT1_TEX3_FMT(TEXCOORDFMT_2D));
   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}


static void
meta_import_pixel_state(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   i830->meta.Ctx[I830_CTXREG_STATE1] = i830->state.Ctx[I830_CTXREG_STATE1];
   i830->meta.Ctx[I830_CTXREG_STATE2] = i830->state.Ctx[I830_CTXREG_STATE2];
   i830->meta.Ctx[I830_CTXREG_STATE3] = i830->state.Ctx[I830_CTXREG_STATE3];
   i830->meta.Ctx[I830_CTXREG_STATE4] = i830->state.Ctx[I830_CTXREG_STATE4];
   i830->meta.Ctx[I830_CTXREG_STATE5] = i830->state.Ctx[I830_CTXREG_STATE5];
   i830->meta.Ctx[I830_CTXREG_IALPHAB] = i830->state.Ctx[I830_CTXREG_IALPHAB];
   i830->meta.Ctx[I830_CTXREG_STENCILTST] =
      i830->state.Ctx[I830_CTXREG_STENCILTST];
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] =
      i830->state.Ctx[I830_CTXREG_ENABLES_1];
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] =
      i830->state.Ctx[I830_CTXREG_ENABLES_2];
   i830->meta.Ctx[I830_CTXREG_AA] = i830->state.Ctx[I830_CTXREG_AA];
   i830->meta.Ctx[I830_CTXREG_FOGCOLOR] =
      i830->state.Ctx[I830_CTXREG_FOGCOLOR];
   i830->meta.Ctx[I830_CTXREG_BLENDCOLOR0] =
      i830->state.Ctx[I830_CTXREG_BLENDCOLOR0];
   i830->meta.Ctx[I830_CTXREG_BLENDCOLOR1] =
      i830->state.Ctx[I830_CTXREG_BLENDCOLOR1];
   i830->meta.Ctx[I830_CTXREG_MCSB0] = i830->state.Ctx[I830_CTXREG_MCSB0];
   i830->meta.Ctx[I830_CTXREG_MCSB1] = i830->state.Ctx[I830_CTXREG_MCSB1];


   i830->meta.Ctx[I830_CTXREG_STATE3] &= ~CULLMODE_MASK;
   i830->meta.Stipple[I830_STPREG_ST1] &= ~ST1_ENABLE;
   i830->meta.emitted &= ~I830_UPLOAD_CTX;


   i830->meta.Buffer[I830_DESTREG_SENABLE] =
      i830->state.Buffer[I830_DESTREG_SENABLE];
   i830->meta.Buffer[I830_DESTREG_SR1] = i830->state.Buffer[I830_DESTREG_SR1];
   i830->meta.Buffer[I830_DESTREG_SR2] = i830->state.Buffer[I830_DESTREG_SR2];
   i830->meta.emitted &= ~I830_UPLOAD_BUFFERS;
}



/* Select between front and back draw buffers.
 */
static void
meta_draw_region(struct intel_context *intel,
                 struct intel_region *color_region,
                 struct intel_region *depth_region)
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   i830_state_draw_region(intel, &i830->meta, color_region, depth_region);
}


/* Operations where the 3D engine is decoupled temporarily from the
 * current GL state and used for other purposes than simply rendering
 * incoming triangles.
 */
static void
install_meta_state(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   memcpy(&i830->meta, &i830->initial, sizeof(i830->meta));

   i830->meta.active = ACTIVE;
   i830->meta.emitted = 0;

   SET_STATE(i830, meta);
   set_vertex_format(intel);
   set_no_texture(intel);
}

static void
leave_meta_state(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   intel_region_release(&i830->meta.draw_region);
   intel_region_release(&i830->meta.depth_region);
/*    intel_region_release(intel, &i830->meta.tex_region[0]); */
   SET_STATE(i830, state);
}



void
i830InitMetaFuncs(struct i830_context *i830)
{
   i830->intel.vtbl.install_meta_state = install_meta_state;
   i830->intel.vtbl.leave_meta_state = leave_meta_state;
   i830->intel.vtbl.meta_no_depth_write = set_no_depth_write;
   i830->intel.vtbl.meta_no_stencil_write = set_no_stencil_write;
   i830->intel.vtbl.meta_stencil_replace = set_stencil_replace;
   i830->intel.vtbl.meta_depth_replace = set_depth_replace;
   i830->intel.vtbl.meta_color_mask = set_color_mask;
   i830->intel.vtbl.meta_no_texture = set_no_texture;
   i830->intel.vtbl.meta_texture_blend_replace = set_texture_blend_replace;
   i830->intel.vtbl.meta_tex_rect_source = set_tex_rect_source;
   i830->intel.vtbl.meta_draw_region = meta_draw_region;
   i830->intel.vtbl.meta_import_pixel_state = meta_import_pixel_state;
}
