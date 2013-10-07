/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 */

/**
 * Meta operations.  Some GL operations can be expressed in terms of
 * other GL operations.  For example, glBlitFramebuffer() can be done
 * with texture mapping and glClear() can be done with polygon rendering.
 *
 * \author Brian Paul
 */


#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/imports.h"
#include "main/arrayobj.h"
#include "main/blend.h"
#include "main/bufferobj.h"
#include "main/buffers.h"
#include "main/colortab.h"
#include "main/context.h"
#include "main/depth.h"
#include "main/enable.h"
#include "main/feedback.h"
#include "main/formats.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/matrix.h"
#include "main/pixel.h"
#include "main/pbo.h"
#include "main/polygon.h"
#include "main/readpix.h"
#include "main/scissor.h"
#include "main/state.h"
#include "main/stencil.h"
#include "main/texobj.h"
#include "main/texenv.h"
#include "main/texgetimage.h"
#include "main/teximage.h"
#include "main/texparam.h"
#include "main/texstate.h"
#include "main/varray.h"
#include "main/viewport.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"


/** Return offset in bytes of the field within a vertex struct */
#define OFFSET(FIELD) ((void *) offsetof(struct vertex, FIELD))

/**
 * State which we may save/restore across meta ops.
 * XXX this may be incomplete...
 */
struct save_state
{
   GLbitfield SavedState;  /**< bitmask of MESA_META_* flags */

   /** MESA_META_ALPHA_TEST */
   GLboolean AlphaEnabled;
   GLenum AlphaFunc;
   GLclampf AlphaRef;

   /** MESA_META_BLEND */
   GLbitfield BlendEnabled;
   GLboolean ColorLogicOpEnabled;

   /** MESA_META_COLOR_MASK */
   GLubyte ColorMask[4];

   /** MESA_META_DEPTH_TEST */
   struct gl_depthbuffer_attrib Depth;

   /** MESA_META_FOG */
   GLboolean Fog;

   /** MESA_META_PIXEL_STORE */
   struct gl_pixelstore_attrib Pack, Unpack;

   /** MESA_META_PIXEL_TRANSFER */
   GLfloat RedBias, RedScale;
   GLfloat GreenBias, GreenScale;
   GLfloat BlueBias, BlueScale;
   GLfloat AlphaBias, AlphaScale;
   GLfloat DepthBias, DepthScale;
   GLboolean MapColorFlag;

   /** MESA_META_RASTERIZATION */
   GLenum FrontPolygonMode, BackPolygonMode;
   GLboolean PolygonOffset;
   GLboolean PolygonSmooth;
   GLboolean PolygonStipple;
   GLboolean PolygonCull;

   /** MESA_META_SCISSOR */
   struct gl_scissor_attrib Scissor;

   /** MESA_META_STENCIL_TEST */
   struct gl_stencil_attrib Stencil;

   /** MESA_META_TRANSFORM */
   GLenum MatrixMode;
   GLfloat ModelviewMatrix[16];
   GLfloat ProjectionMatrix[16];
   GLfloat TextureMatrix[16];

   /** MESA_META_CLIP */
   GLbitfield ClipPlanesEnabled;

   /** MESA_META_TEXTURE */
   struct gl_texture_object *CurrentTexture[NUM_TEXTURE_TARGETS];
   /** mask of TEXTURE_2D_BIT, etc */
   GLbitfield TexEnabled;
   GLbitfield TexGenEnabled;
   GLuint EnvMode;  /* unit[0] only */

   /** MESA_META_VERTEX */
   struct gl_array_object *ArrayObj;
   struct gl_buffer_object *ArrayBufferObj;

   /** MESA_META_VIEWPORT */
   GLint ViewportX, ViewportY, ViewportW, ViewportH;
   GLclampd DepthNear, DepthFar;

   /** MESA_META_SELECT_FEEDBACK */
   GLenum RenderMode;
   struct gl_selection Select;
   struct gl_feedback Feedback;

   /** Miscellaneous (always disabled) */
   GLboolean Lighting;
   GLboolean RasterDiscard;
};

/**
 * Temporary texture used for glBlitFramebuffer, glDrawPixels, etc.
 * This is currently shared by all the meta ops.  But we could create a
 * separate one for each of glDrawPixel, glBlitFramebuffer, glCopyPixels, etc.
 */
struct temp_texture
{
   GLuint TexObj;
   GLenum Target;         /**< GL_TEXTURE_2D */
   GLsizei MinSize;       /**< Min texture size to allocate */
   GLsizei MaxSize;       /**< Max possible texture size */
   GLboolean NPOT;        /**< Non-power of two size OK? */
   GLsizei Width, Height; /**< Current texture size */
   GLenum IntFormat;
   GLfloat Sright, Ttop;  /**< right, top texcoords */
};


/**
 * State for glBlitFramebufer()
 */
struct blit_state
{
   GLuint ArrayObj;
   GLuint VBO;
   GLuint DepthFP;
};


/**
 * State for glClear()
 */
struct clear_state
{
   GLuint ArrayObj;
   GLuint VBO;
   GLuint ShaderProg;
   GLint ColorLocation;

   GLuint IntegerShaderProg;
   GLint IntegerColorLocation;
};


/**
 * State for glCopyPixels()
 */
struct copypix_state
{
   GLuint ArrayObj;
   GLuint VBO;
};

#define MAX_META_OPS_DEPTH      8
/**
 * All per-context meta state.
 */
struct gl_meta_state
{
   /** Stack of state saved during meta-ops */
   struct save_state Save[MAX_META_OPS_DEPTH];
   /** Save stack depth */
   GLuint SaveStackDepth;

   struct temp_texture TempTex;

   struct copypix_state CopyPix;  /**< For _mesa_meta_CopyPixels() */
};

static void cleanup_temp_texture(struct gl_context *ctx, struct temp_texture *tex);

/**
 * Initialize meta-ops for a context.
 * To be called once during context creation.
 */
void
_mesa_meta_init(struct gl_context *ctx)
{
   ASSERT(!ctx->Meta);

   ctx->Meta = CALLOC_STRUCT(gl_meta_state);
}


/**
 * Free context meta-op state.
 * To be called once during context destruction.
 */
void
_mesa_meta_free(struct gl_context *ctx)
{
   GET_CURRENT_CONTEXT(old_context);
   _mesa_make_current(ctx, NULL, NULL);
   cleanup_temp_texture(ctx, &ctx->Meta->TempTex);
   if (old_context)
      _mesa_make_current(old_context, old_context->WinSysDrawBuffer, old_context->WinSysReadBuffer);
   else
      _mesa_make_current(NULL, NULL, NULL);
   free(ctx->Meta);
   ctx->Meta = NULL;
}


/**
 * Enter meta state.  This is like a light-weight version of glPushAttrib
 * but it also resets most GL state back to default values.
 *
 * \param state  bitmask of MESA_META_* flags indicating which attribute groups
 *               to save and reset to their defaults
 */
void
_mesa_meta_begin(struct gl_context *ctx, GLbitfield state)
{
   struct save_state *save;

   /* hope MAX_META_OPS_DEPTH is large enough */
   assert(ctx->Meta->SaveStackDepth < MAX_META_OPS_DEPTH);

   save = &ctx->Meta->Save[ctx->Meta->SaveStackDepth++];
   memset(save, 0, sizeof(*save));
   save->SavedState = state;

   if (state & MESA_META_ALPHA_TEST) {
      save->AlphaEnabled = ctx->Color.AlphaEnabled;
      save->AlphaFunc = ctx->Color.AlphaFunc;
      save->AlphaRef = ctx->Color.AlphaRef;
      if (ctx->Color.AlphaEnabled)
         _mesa_set_enable(ctx, GL_ALPHA_TEST, GL_FALSE);
   }

   if (state & MESA_META_BLEND) {
      save->BlendEnabled = ctx->Color.BlendEnabled;
      if (ctx->Color.BlendEnabled) {
         _mesa_set_enable(ctx, GL_BLEND, GL_FALSE);
      }
      save->ColorLogicOpEnabled = ctx->Color.ColorLogicOpEnabled;
      if (ctx->Color.ColorLogicOpEnabled)
         _mesa_set_enable(ctx, GL_COLOR_LOGIC_OP, GL_FALSE);
   }

   if (state & MESA_META_COLOR_MASK) {
      memcpy(save->ColorMask, ctx->Color.ColorMask,
             sizeof(ctx->Color.ColorMask));
      if (!ctx->Color.ColorMask[0] ||
          !ctx->Color.ColorMask[1] ||
          !ctx->Color.ColorMask[2] ||
          !ctx->Color.ColorMask[3])
         _mesa_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   }

   if (state & MESA_META_DEPTH_TEST) {
      save->Depth = ctx->Depth; /* struct copy */
      if (ctx->Depth.Test)
         _mesa_set_enable(ctx, GL_DEPTH_TEST, GL_FALSE);
   }

   if (state & MESA_META_FOG) {
      save->Fog = ctx->Fog.Enabled;
      if (ctx->Fog.Enabled)
         _mesa_set_enable(ctx, GL_FOG, GL_FALSE);
   }

   if (state & MESA_META_PIXEL_STORE) {
      save->Pack = ctx->Pack;
      save->Unpack = ctx->Unpack;
      ctx->Pack = ctx->DefaultPacking;
      ctx->Unpack = ctx->DefaultPacking;
   }

   if (state & MESA_META_PIXEL_TRANSFER) {
      save->RedScale = ctx->Pixel.RedScale;
      save->RedBias = ctx->Pixel.RedBias;
      save->GreenScale = ctx->Pixel.GreenScale;
      save->GreenBias = ctx->Pixel.GreenBias;
      save->BlueScale = ctx->Pixel.BlueScale;
      save->BlueBias = ctx->Pixel.BlueBias;
      save->AlphaScale = ctx->Pixel.AlphaScale;
      save->AlphaBias = ctx->Pixel.AlphaBias;
      save->MapColorFlag = ctx->Pixel.MapColorFlag;
      ctx->Pixel.RedScale = 1.0F;
      ctx->Pixel.RedBias = 0.0F;
      ctx->Pixel.GreenScale = 1.0F;
      ctx->Pixel.GreenBias = 0.0F;
      ctx->Pixel.BlueScale = 1.0F;
      ctx->Pixel.BlueBias = 0.0F;
      ctx->Pixel.AlphaScale = 1.0F;
      ctx->Pixel.AlphaBias = 0.0F;
      ctx->Pixel.MapColorFlag = GL_FALSE;
      /* XXX more state */
      ctx->NewState |=_NEW_PIXEL;
   }

   if (state & MESA_META_RASTERIZATION) {
      save->FrontPolygonMode = ctx->Polygon.FrontMode;
      save->BackPolygonMode = ctx->Polygon.BackMode;
      save->PolygonOffset = ctx->Polygon.OffsetFill;
      save->PolygonSmooth = ctx->Polygon.SmoothFlag;
      save->PolygonStipple = ctx->Polygon.StippleFlag;
      save->PolygonCull = ctx->Polygon.CullFlag;
      _mesa_PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      _mesa_set_enable(ctx, GL_POLYGON_OFFSET_FILL, GL_FALSE);
      _mesa_set_enable(ctx, GL_POLYGON_SMOOTH, GL_FALSE);
      _mesa_set_enable(ctx, GL_POLYGON_STIPPLE, GL_FALSE);
      _mesa_set_enable(ctx, GL_CULL_FACE, GL_FALSE);
   }

   if (state & MESA_META_SCISSOR) {
      save->Scissor = ctx->Scissor; /* struct copy */
      _mesa_set_enable(ctx, GL_SCISSOR_TEST, GL_FALSE);
   }

   if (state & MESA_META_STENCIL_TEST) {
      save->Stencil = ctx->Stencil; /* struct copy */
      if (ctx->Stencil.Enabled)
         _mesa_set_enable(ctx, GL_STENCIL_TEST, GL_FALSE);
      /* NOTE: other stencil state not reset */
   }

   if (state & MESA_META_TEXTURE) {
      GLuint tgt;

      save->EnvMode = ctx->Texture.Unit.EnvMode;

      /* Disable all texture units */
      save->TexEnabled = ctx->Texture.Unit.Enabled;
      save->TexGenEnabled = ctx->Texture.Unit.TexGenEnabled;
      if (ctx->Texture.Unit.Enabled ||
          ctx->Texture.Unit.TexGenEnabled) {
         _mesa_set_enable(ctx, GL_TEXTURE_1D, GL_FALSE);
         _mesa_set_enable(ctx, GL_TEXTURE_2D, GL_FALSE);
         _mesa_set_enable(ctx, GL_TEXTURE_3D, GL_FALSE);
         if (ctx->Extensions.ARB_texture_cube_map)
            _mesa_set_enable(ctx, GL_TEXTURE_CUBE_MAP, GL_FALSE);
         _mesa_set_enable(ctx, GL_TEXTURE_GEN_S, GL_FALSE);
         _mesa_set_enable(ctx, GL_TEXTURE_GEN_T, GL_FALSE);
         _mesa_set_enable(ctx, GL_TEXTURE_GEN_R, GL_FALSE);
         _mesa_set_enable(ctx, GL_TEXTURE_GEN_Q, GL_FALSE);
      }

      /* save current texture objects for unit[0] only */
      for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
         _mesa_reference_texobj(&save->CurrentTexture[tgt],
                                ctx->Texture.Unit.CurrentTex[tgt]);
      }
      _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   }

   if (state & MESA_META_TRANSFORM) {
      memcpy(save->ModelviewMatrix, ctx->ModelviewMatrixStack.Top->m,
             16 * sizeof(GLfloat));
      memcpy(save->ProjectionMatrix, ctx->ProjectionMatrixStack.Top->m,
             16 * sizeof(GLfloat));
      memcpy(save->TextureMatrix, ctx->TextureMatrixStack.Top->m,
             16 * sizeof(GLfloat));
      save->MatrixMode = ctx->Transform.MatrixMode;
      /* set 1:1 vertex:pixel coordinate transform */
      _mesa_MatrixMode(GL_TEXTURE);
      _mesa_LoadIdentity();
      _mesa_MatrixMode(GL_MODELVIEW);
      _mesa_LoadIdentity();
      _mesa_MatrixMode(GL_PROJECTION);
      _mesa_LoadIdentity();
      _mesa_Ortho(0.0, ctx->DrawBuffer->Width,
                  0.0, ctx->DrawBuffer->Height,
                  -1.0, 1.0);
   }

   if (state & MESA_META_CLIP) {
      save->ClipPlanesEnabled = ctx->Transform.ClipPlanesEnabled;
      if (ctx->Transform.ClipPlanesEnabled) {
         GLuint i;
         for (i = 0; i < ctx->Const.MaxClipPlanes; i++) {
            _mesa_set_enable(ctx, GL_CLIP_PLANE0 + i, GL_FALSE);
         }
      }
   }

   if (state & MESA_META_VERTEX) {
      /* save vertex array object state */
      _mesa_reference_array_object(ctx, &save->ArrayObj,
                                   ctx->Array.ArrayObj);
      _mesa_reference_buffer_object(ctx, &save->ArrayBufferObj,
                                    ctx->Array.ArrayBufferObj);
      /* set some default state? */
   }

   if (state & MESA_META_VIEWPORT) {
      /* save viewport state */
      save->ViewportX = ctx->Viewport.X;
      save->ViewportY = ctx->Viewport.Y;
      save->ViewportW = ctx->Viewport.Width;
      save->ViewportH = ctx->Viewport.Height;
      /* set viewport to match window size */
      if (ctx->Viewport.X != 0 ||
          ctx->Viewport.Y != 0 ||
          ctx->Viewport.Width != ctx->DrawBuffer->Width ||
          ctx->Viewport.Height != ctx->DrawBuffer->Height) {
         _mesa_set_viewport(ctx, 0, 0,
                            ctx->DrawBuffer->Width, ctx->DrawBuffer->Height);
      }
      /* save depth range state */
      save->DepthNear = ctx->Viewport.Near;
      save->DepthFar = ctx->Viewport.Far;
      /* set depth range to default */
      _mesa_DepthRange(0.0, 1.0);
   }

   if (state & MESA_META_SELECT_FEEDBACK) {
      save->RenderMode = ctx->RenderMode;
      if (ctx->RenderMode == GL_SELECT) {
	 save->Select = ctx->Select; /* struct copy */
	 _mesa_RenderMode(GL_RENDER);
      } else if (ctx->RenderMode == GL_FEEDBACK) {
	 save->Feedback = ctx->Feedback; /* struct copy */
	 _mesa_RenderMode(GL_RENDER);
      }
   }

   /* misc */
   {
      save->Lighting = ctx->Light.Enabled;
      if (ctx->Light.Enabled)
         _mesa_set_enable(ctx, GL_LIGHTING, GL_FALSE);
      save->RasterDiscard = ctx->RasterDiscard;
      if (ctx->RasterDiscard)
         _mesa_set_enable(ctx, GL_RASTERIZER_DISCARD, GL_FALSE);
   }
}


/**
 * Leave meta state.  This is like a light-weight version of glPopAttrib().
 */
void
_mesa_meta_end(struct gl_context *ctx)
{
   struct save_state *save = &ctx->Meta->Save[--ctx->Meta->SaveStackDepth];
   const GLbitfield state = save->SavedState;

   if (state & MESA_META_ALPHA_TEST) {
      if (ctx->Color.AlphaEnabled != save->AlphaEnabled)
         _mesa_set_enable(ctx, GL_ALPHA_TEST, save->AlphaEnabled);
      _mesa_AlphaFunc(save->AlphaFunc, save->AlphaRef);
   }

   if (state & MESA_META_BLEND) {
      if (ctx->Color.BlendEnabled != save->BlendEnabled) {
         _mesa_set_enable(ctx, GL_BLEND, (save->BlendEnabled & 1));
      }
      if (ctx->Color.ColorLogicOpEnabled != save->ColorLogicOpEnabled)
         _mesa_set_enable(ctx, GL_COLOR_LOGIC_OP, save->ColorLogicOpEnabled);
   }

   if (state & MESA_META_COLOR_MASK) {
      if (!TEST_EQ_4V(ctx->Color.ColorMask, save->ColorMask)) {
         _mesa_ColorMask(save->ColorMask[0], save->ColorMask[1],
                         save->ColorMask[2], save->ColorMask[3]);         
      }
   }

   if (state & MESA_META_DEPTH_TEST) {
      if (ctx->Depth.Test != save->Depth.Test)
         _mesa_set_enable(ctx, GL_DEPTH_TEST, save->Depth.Test);
      _mesa_DepthFunc(save->Depth.Func);
      _mesa_DepthMask(save->Depth.Mask);
   }

   if (state & MESA_META_FOG) {
      _mesa_set_enable(ctx, GL_FOG, save->Fog);
   }

   if (state & MESA_META_PIXEL_STORE) {
      ctx->Pack = save->Pack;
      ctx->Unpack = save->Unpack;
   }

   if (state & MESA_META_PIXEL_TRANSFER) {
      ctx->Pixel.RedScale = save->RedScale;
      ctx->Pixel.RedBias = save->RedBias;
      ctx->Pixel.GreenScale = save->GreenScale;
      ctx->Pixel.GreenBias = save->GreenBias;
      ctx->Pixel.BlueScale = save->BlueScale;
      ctx->Pixel.BlueBias = save->BlueBias;
      ctx->Pixel.AlphaScale = save->AlphaScale;
      ctx->Pixel.AlphaBias = save->AlphaBias;
      ctx->Pixel.MapColorFlag = save->MapColorFlag;
      /* XXX more state */
      ctx->NewState |=_NEW_PIXEL;
   }

   if (state & MESA_META_RASTERIZATION) {
      _mesa_PolygonMode(GL_FRONT, save->FrontPolygonMode);
      _mesa_PolygonMode(GL_BACK, save->BackPolygonMode);
      _mesa_set_enable(ctx, GL_POLYGON_STIPPLE, save->PolygonStipple);
      _mesa_set_enable(ctx, GL_POLYGON_OFFSET_FILL, save->PolygonOffset);
      _mesa_set_enable(ctx, GL_POLYGON_SMOOTH, save->PolygonSmooth);
      _mesa_set_enable(ctx, GL_CULL_FACE, save->PolygonCull);
   }

   if (state & MESA_META_SCISSOR) {
      _mesa_set_enable(ctx, GL_SCISSOR_TEST, save->Scissor.Enabled);
      _mesa_Scissor(save->Scissor.X, save->Scissor.Y,
                    save->Scissor.Width, save->Scissor.Height);
   }

   if (state & MESA_META_STENCIL_TEST) {
      const struct gl_stencil_attrib *stencil = &save->Stencil;

      _mesa_set_enable(ctx, GL_STENCIL_TEST, stencil->Enabled);
      _mesa_ClearStencil(stencil->Clear);
      if (ctx->Extensions.EXT_stencil_two_side) {
         _mesa_set_enable(ctx, GL_STENCIL_TEST_TWO_SIDE_EXT,
                          stencil->TestTwoSide);
         _mesa_ActiveStencilFaceEXT(stencil->ActiveFace
                                    ? GL_BACK : GL_FRONT);
      }
      /* front state */
      _mesa_StencilFuncSeparate(GL_FRONT,
                                stencil->Function[0],
                                stencil->Ref[0],
                                stencil->ValueMask[0]);
      _mesa_StencilMaskSeparate(GL_FRONT, stencil->WriteMask[0]);
      _mesa_StencilOpSeparate(GL_FRONT, stencil->FailFunc[0],
                              stencil->ZFailFunc[0],
                              stencil->ZPassFunc[0]);
      /* back state */
      _mesa_StencilFuncSeparate(GL_BACK,
                                stencil->Function[1],
                                stencil->Ref[1],
                                stencil->ValueMask[1]);
      _mesa_StencilMaskSeparate(GL_BACK, stencil->WriteMask[1]);
      _mesa_StencilOpSeparate(GL_BACK, stencil->FailFunc[1],
                              stencil->ZFailFunc[1],
                              stencil->ZPassFunc[1]);
   }

   if (state & MESA_META_TEXTURE) {
      GLuint tgt;

      /* restore texenv for unit[0] */
      _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, save->EnvMode);

      /* restore texture objects for unit[0] only */
      for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
	 if (ctx->Texture.Unit.CurrentTex[tgt] != save->CurrentTexture[tgt]) {
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    _mesa_reference_texobj(&ctx->Texture.Unit.CurrentTex[tgt],
				   save->CurrentTexture[tgt]);
	 }
         _mesa_reference_texobj(&save->CurrentTexture[tgt], NULL);
      }

      /* Restore fixed function texture enables, texgen */
	 if (ctx->Texture.Unit.Enabled != save->TexEnabled) {
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    ctx->Texture.Unit.Enabled = save->TexEnabled;
	 }

	 if (ctx->Texture.Unit.TexGenEnabled != save->TexGenEnabled) {
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    ctx->Texture.Unit.TexGenEnabled = save->TexGenEnabled;
	 }
   }

   if (state & MESA_META_TRANSFORM) {
      _mesa_MatrixMode(GL_TEXTURE);
      _mesa_LoadMatrixf(save->TextureMatrix);

      _mesa_MatrixMode(GL_MODELVIEW);
      _mesa_LoadMatrixf(save->ModelviewMatrix);

      _mesa_MatrixMode(GL_PROJECTION);
      _mesa_LoadMatrixf(save->ProjectionMatrix);

      _mesa_MatrixMode(save->MatrixMode);
   }

   if (state & MESA_META_CLIP) {
      if (save->ClipPlanesEnabled) {
         GLuint i;
         for (i = 0; i < ctx->Const.MaxClipPlanes; i++) {
            if (save->ClipPlanesEnabled & (1 << i)) {
               _mesa_set_enable(ctx, GL_CLIP_PLANE0 + i, GL_TRUE);
            }
         }
      }
   }

   if (state & MESA_META_VERTEX) {
      /* restore vertex buffer object */
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, save->ArrayBufferObj->Name);
      _mesa_reference_buffer_object(ctx, &save->ArrayBufferObj, NULL);

      /* restore vertex array object */
      _mesa_BindVertexArray(save->ArrayObj->Name);
      _mesa_reference_array_object(ctx, &save->ArrayObj, NULL);
   }

   if (state & MESA_META_VIEWPORT) {
      if (save->ViewportX != ctx->Viewport.X ||
          save->ViewportY != ctx->Viewport.Y ||
          save->ViewportW != ctx->Viewport.Width ||
          save->ViewportH != ctx->Viewport.Height) {
         _mesa_set_viewport(ctx, save->ViewportX, save->ViewportY,
                            save->ViewportW, save->ViewportH);
      }
      _mesa_DepthRange(save->DepthNear, save->DepthFar);
   }

   /* misc */
   if (save->Lighting) {
      _mesa_set_enable(ctx, GL_LIGHTING, GL_TRUE);
   }
   if (save->RasterDiscard) {
      _mesa_set_enable(ctx, GL_RASTERIZER_DISCARD, GL_TRUE);
   }
}


/**
 * Determine whether Mesa is currently in a meta state.
 */
GLboolean
_mesa_meta_in_progress(struct gl_context *ctx)
{
   return ctx->Meta->SaveStackDepth != 0;
}


/**
 * Convert Z from a normalized value in the range [0, 1] to an object-space
 * Z coordinate in [-1, +1] so that drawing at the new Z position with the
 * default/identity ortho projection results in the original Z value.
 * Used by the meta-Clear, Draw/CopyPixels and Bitmap functions where the Z
 * value comes from the clear value or raster position.
 */
static INLINE GLfloat
invert_z(GLfloat normZ)
{
   GLfloat objZ = 1.0f - 2.0f * normZ;
   return objZ;
}


/**
 * One-time init for a temp_texture object.
 * Choose tex target, compute max tex size, etc.
 */
static void
init_temp_texture(struct gl_context *ctx, struct temp_texture *tex)
{
   /* use 2D texture, NPOT if possible */
   tex->Target = GL_TEXTURE_2D;
   tex->MaxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
   tex->NPOT = ctx->Extensions.ARB_texture_non_power_of_two;
   tex->MinSize = 16;  /* 16 x 16 at least */
   assert(tex->MaxSize > 0);

   _mesa_GenTextures(1, &tex->TexObj);
}

static void
cleanup_temp_texture(struct gl_context *ctx, struct temp_texture *tex)
{
   if (!tex->TexObj)
     return;
   _mesa_DeleteTextures(1, &tex->TexObj);
   tex->TexObj = 0;
}


/**
 * Return pointer to temp_texture info for non-bitmap ops.
 * This does some one-time init if needed.
 */
static struct temp_texture *
get_temp_texture(struct gl_context *ctx)
{
   struct temp_texture *tex = &ctx->Meta->TempTex;

   if (!tex->TexObj) {
      init_temp_texture(ctx, tex);
   }

   return tex;
}


/**
 * Compute the width/height of texture needed to draw an image of the
 * given size.  Return a flag indicating whether the current texture
 * can be re-used (glTexSubImage2D) or if a new texture needs to be
 * allocated (glTexImage2D).
 * Also, compute s/t texcoords for drawing.
 *
 * \return GL_TRUE if new texture is needed, GL_FALSE otherwise
 */
static GLboolean
alloc_texture(struct temp_texture *tex,
              GLsizei width, GLsizei height, GLenum intFormat)
{
   GLboolean newTex = GL_FALSE;

   ASSERT(width <= tex->MaxSize);
   ASSERT(height <= tex->MaxSize);

   if (width > tex->Width ||
       height > tex->Height ||
       intFormat != tex->IntFormat) {
      /* alloc new texture (larger or different format) */

      if (tex->NPOT) {
         /* use non-power of two size */
         tex->Width = MAX2(tex->MinSize, width);
         tex->Height = MAX2(tex->MinSize, height);
      }
      else {
         /* find power of two size */
         GLsizei w, h;
         w = h = tex->MinSize;
         while (w < width)
            w *= 2;
         while (h < height)
            h *= 2;
         tex->Width = w;
         tex->Height = h;
      }

      tex->IntFormat = intFormat;

      newTex = GL_TRUE;
   }

   /* compute texcoords */
   tex->Sright = (GLfloat) width / tex->Width;
   tex->Ttop = (GLfloat) height / tex->Height;

   return newTex;
}


/**
 * Setup/load texture for glCopyPixels or glBlitFramebuffer.
 */
static void
setup_copypix_texture(struct temp_texture *tex,
                      GLboolean newTex,
                      GLint srcX, GLint srcY,
                      GLsizei width, GLsizei height, GLenum intFormat,
                      GLenum filter)
{
   _mesa_BindTexture(tex->Target, tex->TexObj);
   _mesa_TexParameteri(tex->Target, GL_TEXTURE_MIN_FILTER, filter);
   _mesa_TexParameteri(tex->Target, GL_TEXTURE_MAG_FILTER, filter);
   _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   /* copy framebuffer image to texture */
   if (newTex) {
      /* create new tex image */
      if (tex->Width == width && tex->Height == height) {
         /* create new tex with framebuffer data */
         _mesa_CopyTexImage2D(tex->Target, 0, tex->IntFormat,
                              srcX, srcY, width, height, 0);
      }
      else {
         /* create empty texture */
         _mesa_TexImage2D(tex->Target, 0, tex->IntFormat,
                          tex->Width, tex->Height, 0,
                          intFormat, GL_UNSIGNED_BYTE, NULL);
         /* load image */
         _mesa_CopyTexSubImage2D(tex->Target, 0,
                                 0, 0, srcX, srcY, width, height);
      }
   }
   else {
      /* replace existing tex image */
      _mesa_CopyTexSubImage2D(tex->Target, 0,
                              0, 0, srcX, srcY, width, height);
   }
}

/**
 * Meta implementation of ctx->Driver.CopyPixels() in terms
 * of texture mapping and polygon rendering and GLSL shaders.
 */
void
_mesa_meta_CopyPixels(struct gl_context *ctx, GLint srcX, GLint srcY,
                      GLsizei width, GLsizei height,
                      GLint dstX, GLint dstY, GLenum type)
{
   struct copypix_state *copypix = &ctx->Meta->CopyPix;
   struct temp_texture *tex = get_temp_texture(ctx);
   struct vertex {
      GLfloat x, y, z, s, t;
   };
   struct vertex verts[4];
   GLboolean newTex;
   GLenum intFormat = GL_RGBA;

   if (type != GL_COLOR ||
       ctx->_ImageTransferState ||
       ctx->Fog.Enabled ||
       width > tex->MaxSize ||
       height > tex->MaxSize) {
      /* XXX avoid this fallback */
      _swrast_CopyPixels(ctx, srcX, srcY, width, height, dstX, dstY, type);
      return;
   }

   /* Most GL state applies to glCopyPixels, but a there's a few things
    * we need to override:
    */
   _mesa_meta_begin(ctx, (MESA_META_RASTERIZATION |
                          MESA_META_TEXTURE |
                          MESA_META_TRANSFORM |
                          MESA_META_CLIP |
                          MESA_META_VERTEX |
                          MESA_META_VIEWPORT));

   if (copypix->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArrays(1, &copypix->ArrayObj);
      _mesa_BindVertexArray(copypix->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &copypix->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, copypix->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                          NULL, GL_DYNAMIC_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_TexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(s));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
   }
   else {
      _mesa_BindVertexArray(copypix->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, copypix->VBO);
   }

   newTex = alloc_texture(tex, width, height, intFormat);

   /* vertex positions, texcoords (after texture allocation!) */
   {
      const GLfloat dstX0 = (GLfloat) dstX;
      const GLfloat dstY0 = (GLfloat) dstY;
      const GLfloat dstX1 = dstX + width * ctx->Pixel.ZoomX;
      const GLfloat dstY1 = dstY + height * ctx->Pixel.ZoomY;
      const GLfloat z = invert_z(ctx->Current.RasterPos[2]);

      verts[0].x = dstX0;
      verts[0].y = dstY0;
      verts[0].z = z;
      verts[0].s = 0.0F;
      verts[0].t = 0.0F;
      verts[1].x = dstX1;
      verts[1].y = dstY0;
      verts[1].z = z;
      verts[1].s = tex->Sright;
      verts[1].t = 0.0F;
      verts[2].x = dstX1;
      verts[2].y = dstY1;
      verts[2].z = z;
      verts[2].s = tex->Sright;
      verts[2].t = tex->Ttop;
      verts[3].x = dstX0;
      verts[3].y = dstY1;
      verts[3].z = z;
      verts[3].s = 0.0F;
      verts[3].t = tex->Ttop;

      /* upload new vertex data */
      _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
   }

   /* Alloc/setup texture */
   setup_copypix_texture(tex, newTex, srcX, srcY, width, height,
                         GL_RGBA, GL_NEAREST);

   _mesa_set_enable(ctx, tex->Target, GL_TRUE);

   /* draw textured quad */
   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   _mesa_set_enable(ctx, tex->Target, GL_FALSE);

   _mesa_meta_end(ctx);
}


/**
 * Determine the GL data type to use for the temporary image read with
 * ReadPixels() and passed to Tex[Sub]Image().
 */
static GLenum
get_temp_image_type(struct gl_context *ctx, GLenum baseFormat)
{
   switch (baseFormat) {
   case GL_RGBA:
   case GL_RGB:
   case GL_RG:
   case GL_RED:
   case GL_ALPHA:
   case GL_LUMINANCE:
   case GL_LUMINANCE_ALPHA:
   case GL_INTENSITY:
      if (ctx->DrawBuffer->Visual.redBits <= 8)
         return GL_UNSIGNED_BYTE;
      else if (ctx->DrawBuffer->Visual.redBits <= 16)
         return GL_UNSIGNED_SHORT;
      else
         return GL_FLOAT;
   case GL_DEPTH_COMPONENT:
      return GL_UNSIGNED_INT;
   default:
      _mesa_problem(ctx, "Unexpected format %d in get_temp_image_type()",
		    baseFormat);
      return 0;
   }
}


/**
 * Helper for _mesa_meta_CopyTexSubImage1/2/3D() functions.
 * Have to be careful with locking and meta state for pixel transfer.
 */
static void
copy_tex_sub_image(struct gl_context *ctx,
                   GLuint dims,
                   struct gl_texture_image *texImage,
                   GLint xoffset, GLint yoffset, GLint zoffset,
                   struct gl_renderbuffer *rb,
                   GLint x, GLint y,
                   GLsizei width, GLsizei height)
{
   struct gl_texture_object *texObj = texImage->TexObject;
   const GLenum target = texObj->Target;
   GLenum format, type;
   GLint bpp;
   void *buf;

   /* Choose format/type for temporary image buffer */
   format = _mesa_get_format_base_format(texImage->TexFormat);
   if (format == GL_LUMINANCE ||
       format == GL_LUMINANCE_ALPHA ||
       format == GL_INTENSITY) {
      /* We don't want to use GL_LUMINANCE, GL_INTENSITY, etc. for the
       * temp image buffer because glReadPixels will do L=R+G+B which is
       * not what we want (should be L=R).
       */
      format = GL_RGBA;
   }

   if (_mesa_is_format_integer_color(texImage->TexFormat)) {
      _mesa_problem(ctx, "unsupported integer color copyteximage");
      return;
   }

   type = get_temp_image_type(ctx, format);
   bpp = _mesa_bytes_per_pixel(format, type);
   if (bpp <= 0) {
      _mesa_problem(ctx, "Bad bpp in meta copy_tex_sub_image()");
      return;
   }

   /*
    * Alloc image buffer (XXX could use a PBO)
    */
   buf = malloc(width * height * bpp);
   if (!buf) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage%uD", dims);
      return;
   }

   _mesa_unlock_texture(ctx, texObj); /* need to unlock first */

   /*
    * Read image from framebuffer (disable pixel transfer ops)
    */
   _mesa_meta_begin(ctx, MESA_META_PIXEL_STORE | MESA_META_PIXEL_TRANSFER);
   ctx->Driver.ReadPixels(ctx, x, y, width, height,
			  format, type, &ctx->Pack, buf);
   _mesa_meta_end(ctx);

   _mesa_update_state(ctx); /* to update pixel transfer state */

   /*
    * Store texture data (with pixel transfer ops)
    */
   _mesa_meta_begin(ctx, MESA_META_PIXEL_STORE);
   if (target == GL_TEXTURE_1D) {
      ctx->Driver.TexSubImage1D(ctx, texImage,
                                xoffset, width,
                                format, type, buf, &ctx->Unpack);
   }
   else if (target == GL_TEXTURE_3D) {
      ctx->Driver.TexSubImage3D(ctx, texImage,
                                xoffset, yoffset, zoffset, width, height, 1,
                                format, type, buf, &ctx->Unpack);
   }
   else {
      ctx->Driver.TexSubImage2D(ctx, texImage,
                                xoffset, yoffset, width, height,
                                format, type, buf, &ctx->Unpack);
   }
   _mesa_meta_end(ctx);

   _mesa_lock_texture(ctx, texObj); /* re-lock */

   free(buf);
}


void
_mesa_meta_CopyTexSubImage1D(struct gl_context *ctx,
                             struct gl_texture_image *texImage,
                             GLint xoffset,
                             struct gl_renderbuffer *rb,
                             GLint x, GLint y, GLsizei width)
{
   copy_tex_sub_image(ctx, 1, texImage, xoffset, 0, 0,
                      rb, x, y, width, 1);
}


void
_mesa_meta_CopyTexSubImage2D(struct gl_context *ctx,
                             struct gl_texture_image *texImage,
                             GLint xoffset, GLint yoffset,
                             struct gl_renderbuffer *rb,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height)
{
   copy_tex_sub_image(ctx, 2, texImage, xoffset, yoffset, 0,
                      rb, x, y, width, height);
}


void
_mesa_meta_CopyTexSubImage3D(struct gl_context *ctx,
                             struct gl_texture_image *texImage,
                             GLint xoffset, GLint yoffset, GLint zoffset,
                             struct gl_renderbuffer *rb,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height)
{
   copy_tex_sub_image(ctx, 3, texImage, xoffset, yoffset, zoffset,
                      rb, x, y, width, height);
}
