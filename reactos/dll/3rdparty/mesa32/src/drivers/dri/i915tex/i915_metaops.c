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
#include "intel_rotate.h"

#include "i915_context.h"
#include "i915_reg.h"

/* We touch almost everything:
 */
#define ACTIVE (I915_UPLOAD_INVARIENT | 	\
		I915_UPLOAD_CTX |		\
		I915_UPLOAD_BUFFERS |		\
		I915_UPLOAD_STIPPLE |		\
                I915_UPLOAD_PROGRAM | 		\
                I915_UPLOAD_FOG | 		\
		I915_UPLOAD_TEX(0))

#define SET_STATE( i915, STATE )		\
do {						\
   i915->current->emitted &= ~ACTIVE;		\
   i915->current = &i915->STATE;		\
   i915->current->emitted &= ~ACTIVE;		\
} while (0)


static void
meta_no_stencil_write(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_STENCIL_TEST, GL_FALSE )
    */
   i915->meta.Ctx[I915_CTXREG_LIS5] &= ~(S5_STENCIL_TEST_ENABLE |
                                         S5_STENCIL_WRITE_ENABLE);

   i915->meta.emitted &= ~I915_UPLOAD_CTX;
}

static void
meta_no_depth_write(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_DEPTH_TEST, GL_FALSE )
    */
   i915->meta.Ctx[I915_CTXREG_LIS6] &= ~(S6_DEPTH_TEST_ENABLE |
                                         S6_DEPTH_WRITE_ENABLE);

   i915->meta.emitted &= ~I915_UPLOAD_CTX;
}

static void
meta_depth_replace(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_DEPTH_TEST, GL_TRUE )
    * ctx->Driver.DepthMask( ctx, GL_TRUE )
    */
   i915->meta.Ctx[I915_CTXREG_LIS6] |= (S6_DEPTH_TEST_ENABLE |
                                        S6_DEPTH_WRITE_ENABLE);

   /* ctx->Driver.DepthFunc( ctx, GL_ALWAYS )
    */
   i915->meta.Ctx[I915_CTXREG_LIS6] &= ~S6_DEPTH_TEST_FUNC_MASK;
   i915->meta.Ctx[I915_CTXREG_LIS6] |=
      COMPAREFUNC_ALWAYS << S6_DEPTH_TEST_FUNC_SHIFT;

   i915->meta.emitted &= ~I915_UPLOAD_CTX;
}


/* Set stencil unit to replace always with the reference value.
 */
static void
meta_stencil_replace(struct intel_context *intel,
                     GLuint s_mask, GLuint s_clear)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   GLuint op = STENCILOP_REPLACE;
   GLuint func = COMPAREFUNC_ALWAYS;

   /* ctx->Driver.Enable( ctx, GL_STENCIL_TEST, GL_TRUE )
    */
   i915->meta.Ctx[I915_CTXREG_LIS5] |= (S5_STENCIL_TEST_ENABLE |
                                        S5_STENCIL_WRITE_ENABLE);

   /* ctx->Driver.StencilMask( ctx, s_mask )
    */
   i915->meta.Ctx[I915_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_WRITE_MASK;

   i915->meta.Ctx[I915_CTXREG_STATE4] |= (ENABLE_STENCIL_WRITE_MASK |
                                          STENCIL_WRITE_MASK(s_mask));

   /* ctx->Driver.StencilOp( ctx, GL_REPLACE, GL_REPLACE, GL_REPLACE )
    */
   i915->meta.Ctx[I915_CTXREG_LIS5] &= ~(S5_STENCIL_FAIL_MASK |
                                         S5_STENCIL_PASS_Z_FAIL_MASK |
                                         S5_STENCIL_PASS_Z_PASS_MASK);

   i915->meta.Ctx[I915_CTXREG_LIS5] |= ((op << S5_STENCIL_FAIL_SHIFT) |
                                        (op << S5_STENCIL_PASS_Z_FAIL_SHIFT) |
                                        (op << S5_STENCIL_PASS_Z_PASS_SHIFT));


   /* ctx->Driver.StencilFunc( ctx, GL_ALWAYS, s_ref, ~0 )
    */
   i915->meta.Ctx[I915_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_TEST_MASK;
   i915->meta.Ctx[I915_CTXREG_STATE4] |= (ENABLE_STENCIL_TEST_MASK |
                                          STENCIL_TEST_MASK(0xff));

   i915->meta.Ctx[I915_CTXREG_LIS5] &= ~(S5_STENCIL_REF_MASK |
                                         S5_STENCIL_TEST_FUNC_MASK);

   i915->meta.Ctx[I915_CTXREG_LIS5] |= ((s_clear << S5_STENCIL_REF_SHIFT) |
                                        (func << S5_STENCIL_TEST_FUNC_SHIFT));


   i915->meta.emitted &= ~I915_UPLOAD_CTX;
}


static void
meta_color_mask(struct intel_context *intel, GLboolean state)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   const GLuint mask = (S5_WRITEDISABLE_RED |
                        S5_WRITEDISABLE_GREEN |
                        S5_WRITEDISABLE_BLUE | S5_WRITEDISABLE_ALPHA);

   /* Copy colormask state from "regular" hw context.
    */
   if (state) {
      i915->meta.Ctx[I915_CTXREG_LIS5] &= ~mask;
      i915->meta.Ctx[I915_CTXREG_LIS5] |=
         (i915->state.Ctx[I915_CTXREG_LIS5] & mask);
   }
   else
      i915->meta.Ctx[I915_CTXREG_LIS5] |= mask;

   i915->meta.emitted &= ~I915_UPLOAD_CTX;
}



static void
meta_import_pixel_state(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   memcpy(i915->meta.Fog, i915->state.Fog, I915_FOG_SETUP_SIZE * 4);

   i915->meta.Ctx[I915_CTXREG_LIS5] = i915->state.Ctx[I915_CTXREG_LIS5];
   i915->meta.Ctx[I915_CTXREG_LIS6] = i915->state.Ctx[I915_CTXREG_LIS6];
   i915->meta.Ctx[I915_CTXREG_STATE4] = i915->state.Ctx[I915_CTXREG_STATE4];
   i915->meta.Ctx[I915_CTXREG_BLENDCOLOR1] =
      i915->state.Ctx[I915_CTXREG_BLENDCOLOR1];
   i915->meta.Ctx[I915_CTXREG_IAB] = i915->state.Ctx[I915_CTXREG_IAB];

   i915->meta.Buffer[I915_DESTREG_SENABLE] =
      i915->state.Buffer[I915_DESTREG_SENABLE];
   i915->meta.Buffer[I915_DESTREG_SR1] = i915->state.Buffer[I915_DESTREG_SR1];
   i915->meta.Buffer[I915_DESTREG_SR2] = i915->state.Buffer[I915_DESTREG_SR2];

   i915->meta.emitted &= ~I915_UPLOAD_FOG;
   i915->meta.emitted &= ~I915_UPLOAD_BUFFERS;
   i915->meta.emitted &= ~I915_UPLOAD_CTX;
}




#define REG( type, nr ) (((type)<<5)|(nr))

#define REG_R(x)       REG(REG_TYPE_R, x)
#define REG_T(x)       REG(REG_TYPE_T, x)
#define REG_CONST(x)   REG(REG_TYPE_CONST, x)
#define REG_S(x)       REG(REG_TYPE_S, x)
#define REG_OC         REG(REG_TYPE_OC, 0)
#define REG_OD	       REG(REG_TYPE_OD, 0)
#define REG_U(x)       REG(REG_TYPE_U, x)

#define REG_T_DIFFUSE  REG(REG_TYPE_T, T_DIFFUSE)
#define REG_T_SPECULAR REG(REG_TYPE_T, T_SPECULAR)
#define REG_T_FOG_W    REG(REG_TYPE_T, T_FOG_W)
#define REG_T_TEX(x)   REG(REG_TYPE_T, x)


#define A0_DEST_REG( reg ) ( (reg) << A0_DEST_NR_SHIFT )
#define A0_SRC0_REG( reg ) ( (reg) << A0_SRC0_NR_SHIFT )
#define A1_SRC1_REG( reg ) ( (reg) << A1_SRC1_NR_SHIFT )
#define A1_SRC2_REG( reg ) ( (reg) << A1_SRC2_NR_SHIFT )
#define A2_SRC2_REG( reg ) ( (reg) << A2_SRC2_NR_SHIFT )
#define D0_DECL_REG( reg ) ( (reg) << D0_NR_SHIFT )
#define T0_DEST_REG( reg ) ( (reg) << T0_DEST_NR_SHIFT )

#define T0_SAMPLER( unit )     ((unit)<<T0_SAMPLER_NR_SHIFT)

#define T1_ADDRESS_REG( type, nr ) (((type)<<T1_ADDRESS_REG_TYPE_SHIFT)| \
				    ((nr)<<T1_ADDRESS_REG_NR_SHIFT))


#define A1_SRC0_XYZW ((SRC_X << A1_SRC0_CHANNEL_X_SHIFT) |	\
		      (SRC_Y << A1_SRC0_CHANNEL_Y_SHIFT) |	\
		      (SRC_Z << A1_SRC0_CHANNEL_Z_SHIFT) |	\
		      (SRC_W << A1_SRC0_CHANNEL_W_SHIFT))

#define A1_SRC1_XY   ((SRC_X << A1_SRC1_CHANNEL_X_SHIFT) |	\
		      (SRC_Y << A1_SRC1_CHANNEL_Y_SHIFT))

#define A2_SRC1_ZW   ((SRC_Z << A2_SRC1_CHANNEL_Z_SHIFT) |	\
		      (SRC_W << A2_SRC1_CHANNEL_W_SHIFT))

#define A2_SRC2_XYZW ((SRC_X << A2_SRC2_CHANNEL_X_SHIFT) |	\
		      (SRC_Y << A2_SRC2_CHANNEL_Y_SHIFT) |	\
		      (SRC_Z << A2_SRC2_CHANNEL_Z_SHIFT) |	\
		      (SRC_W << A2_SRC2_CHANNEL_W_SHIFT))





static void
meta_no_texture(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);

   static const GLuint prog[] = {
      _3DSTATE_PIXEL_SHADER_PROGRAM,

      /* Declare incoming diffuse color:
       */
      (D0_DCL | D0_DECL_REG(REG_T_DIFFUSE) | D0_CHANNEL_ALL),
      D1_MBZ,
      D2_MBZ,

      /* output-color = mov(t_diffuse)
       */
      (A0_MOV |
       A0_DEST_REG(REG_OC) |
       A0_DEST_CHANNEL_ALL | A0_SRC0_REG(REG_T_DIFFUSE)),
      (A1_SRC0_XYZW),
      0,
   };


   memcpy(i915->meta.Program, prog, sizeof(prog));
   i915->meta.ProgramSize = sizeof(prog) / sizeof(*prog);
   i915->meta.Program[0] |= i915->meta.ProgramSize - 2;
   i915->meta.emitted &= ~I915_UPLOAD_PROGRAM;
}

static void
meta_texture_blend_replace(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);

   static const GLuint prog[] = {
      _3DSTATE_PIXEL_SHADER_PROGRAM,

      /* Declare the sampler:
       */
      (D0_DCL | D0_DECL_REG(REG_S(0)) | D0_SAMPLE_TYPE_2D | D0_CHANNEL_NONE),
      D1_MBZ,
      D2_MBZ,

      /* Declare the interpolated texture coordinate:
       */
      (D0_DCL | D0_DECL_REG(REG_T_TEX(0)) | D0_CHANNEL_ALL),
      D1_MBZ,
      D2_MBZ,

      /* output-color = texld(sample0, texcoord0) 
       */
      (T0_TEXLD | T0_DEST_REG(REG_OC) | T0_SAMPLER(0)),
      T1_ADDRESS_REG(REG_TYPE_T, 0),
      T2_MBZ
   };

   memcpy(i915->meta.Program, prog, sizeof(prog));
   i915->meta.ProgramSize = sizeof(prog) / sizeof(*prog);
   i915->meta.Program[0] |= i915->meta.ProgramSize - 2;
   i915->meta.emitted &= ~I915_UPLOAD_PROGRAM;
}





/* Set up an arbitary piece of memory as a rectangular texture
 * (including the front or back buffer).
 */
static GLboolean
meta_tex_rect_source(struct intel_context *intel,
                     struct _DriBufferObject *buffer,
                     GLuint offset,
                     GLuint pitch, GLuint height, GLenum format, GLenum type)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   GLuint unit = 0;
   GLint numLevels = 1;
   GLuint *state = i915->meta.Tex[0];
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


   if ((pitch * cpp) & 3) {
      _mesa_printf("%s: texture is not dword pitch\n", __FUNCTION__);
      return GL_FALSE;
   }

/*    intel_region_release(&i915->meta.tex_region[0]); */
/*    intel_region_reference(&i915->meta.tex_region[0], region); */
   i915->meta.tex_buffer[0] = buffer;
   i915->meta.tex_offset[0] = offset;

   state[I915_TEXREG_MS3] = (((height - 1) << MS3_HEIGHT_SHIFT) |
                             ((pitch - 1) << MS3_WIDTH_SHIFT) |
                             textureFormat | MS3_USE_FENCE_REGS);

   state[I915_TEXREG_MS4] = (((((pitch * cpp) / 4) - 1) << MS4_PITCH_SHIFT) |
                             MS4_CUBE_FACE_ENA_MASK |
                             ((((numLevels - 1) * 4)) << MS4_MAX_LOD_SHIFT));

   state[I915_TEXREG_SS2] = ((FILTER_NEAREST << SS2_MIN_FILTER_SHIFT) |
                             (MIPFILTER_NONE << SS2_MIP_FILTER_SHIFT) |
                             (FILTER_NEAREST << SS2_MAG_FILTER_SHIFT));

   state[I915_TEXREG_SS3] = ((TEXCOORDMODE_WRAP << SS3_TCX_ADDR_MODE_SHIFT) |
                             (TEXCOORDMODE_WRAP << SS3_TCY_ADDR_MODE_SHIFT) |
                             (TEXCOORDMODE_WRAP << SS3_TCZ_ADDR_MODE_SHIFT) |
                             (unit << SS3_TEXTUREMAP_INDEX_SHIFT));

   state[I915_TEXREG_SS4] = 0;

   i915->meta.emitted &= ~I915_UPLOAD_TEX(0);
   return GL_TRUE;
}


/**
 * Set the color and depth drawing region for meta ops.
 */
static void
meta_draw_region(struct intel_context *intel,
                 struct intel_region *color_region,
                 struct intel_region *depth_region)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   i915_state_draw_region(intel, &i915->meta, color_region, depth_region);
}


static void
set_vertex_format(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);

   i915->meta.Ctx[I915_CTXREG_LIS2] =
      (S2_TEXCOORD_FMT(0, TEXCOORDFMT_2D) |
       S2_TEXCOORD_FMT(1, TEXCOORDFMT_NOT_PRESENT) |
       S2_TEXCOORD_FMT(2, TEXCOORDFMT_NOT_PRESENT) |
       S2_TEXCOORD_FMT(3, TEXCOORDFMT_NOT_PRESENT) |
       S2_TEXCOORD_FMT(4, TEXCOORDFMT_NOT_PRESENT) |
       S2_TEXCOORD_FMT(5, TEXCOORDFMT_NOT_PRESENT) |
       S2_TEXCOORD_FMT(6, TEXCOORDFMT_NOT_PRESENT) |
       S2_TEXCOORD_FMT(7, TEXCOORDFMT_NOT_PRESENT));

   i915->meta.Ctx[I915_CTXREG_LIS4] &= ~S4_VFMT_MASK;

   i915->meta.Ctx[I915_CTXREG_LIS4] |= (S4_VFMT_COLOR | S4_VFMT_XYZ);

   i915->meta.emitted &= ~I915_UPLOAD_CTX;
}



/* Operations where the 3D engine is decoupled temporarily from the
 * current GL state and used for other purposes than simply rendering
 * incoming triangles.
 */
static void
install_meta_state(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   memcpy(&i915->meta, &i915->initial, sizeof(i915->meta));
   i915->meta.active = ACTIVE;
   i915->meta.emitted = 0;

   SET_STATE(i915, meta);
   set_vertex_format(intel);
   meta_no_texture(intel);
}

static void
leave_meta_state(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   intel_region_release(&i915->meta.draw_region);
   intel_region_release(&i915->meta.depth_region);
/*    intel_region_release(&i915->meta.tex_region[0]); */
   SET_STATE(i915, state);
}



void
i915InitMetaFuncs(struct i915_context *i915)
{
   i915->intel.vtbl.install_meta_state = install_meta_state;
   i915->intel.vtbl.leave_meta_state = leave_meta_state;
   i915->intel.vtbl.meta_no_depth_write = meta_no_depth_write;
   i915->intel.vtbl.meta_no_stencil_write = meta_no_stencil_write;
   i915->intel.vtbl.meta_stencil_replace = meta_stencil_replace;
   i915->intel.vtbl.meta_depth_replace = meta_depth_replace;
   i915->intel.vtbl.meta_color_mask = meta_color_mask;
   i915->intel.vtbl.meta_no_texture = meta_no_texture;
   i915->intel.vtbl.meta_texture_blend_replace = meta_texture_blend_replace;
   i915->intel.vtbl.meta_tex_rect_source = meta_tex_rect_source;
   i915->intel.vtbl.meta_draw_region = meta_draw_region;
   i915->intel.vtbl.meta_import_pixel_state = meta_import_pixel_state;
}
