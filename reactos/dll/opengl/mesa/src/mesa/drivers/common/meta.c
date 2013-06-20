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
#include "main/arbprogram.h"
#include "main/arrayobj.h"
#include "main/blend.h"
#include "main/bufferobj.h"
#include "main/buffers.h"
#include "main/colortab.h"
#include "main/condrender.h"
#include "main/depth.h"
#include "main/enable.h"
#include "main/fbobject.h"
#include "main/feedback.h"
#include "main/formats.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/matrix.h"
#include "main/mipmap.h"
#include "main/pixel.h"
#include "main/pbo.h"
#include "main/polygon.h"
#include "main/readpix.h"
#include "main/scissor.h"
#include "main/shaderapi.h"
#include "main/shaderobj.h"
#include "main/state.h"
#include "main/stencil.h"
#include "main/texobj.h"
#include "main/texenv.h"
#include "main/texgetimage.h"
#include "main/teximage.h"
#include "main/texparam.h"
#include "main/texstate.h"
#include "main/transformfeedback.h"
#include "main/uniforms.h"
#include "main/varray.h"
#include "main/viewport.h"
#include "program/program.h"
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
   GLubyte ColorMask[MAX_DRAW_BUFFERS][4];

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

   /** MESA_META_SHADER */
   GLboolean VertexProgramEnabled;
   struct gl_vertex_program *VertexProgram;
   GLboolean FragmentProgramEnabled;
   struct gl_fragment_program *FragmentProgram;
   struct gl_shader_program *VertexShader;
   struct gl_shader_program *GeometryShader;
   struct gl_shader_program *FragmentShader;
   struct gl_shader_program *ActiveShader;

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
   GLuint ActiveUnit;
   GLuint ClientActiveUnit;
   /** for unit[0] only */
   struct gl_texture_object *CurrentTexture[NUM_TEXTURE_TARGETS];
   /** mask of TEXTURE_2D_BIT, etc */
   GLbitfield TexEnabled[MAX_TEXTURE_UNITS];
   GLbitfield TexGenEnabled[MAX_TEXTURE_UNITS];
   GLuint EnvMode;  /* unit[0] only */

   /** MESA_META_VERTEX */
   struct gl_array_object *ArrayObj;
   struct gl_buffer_object *ArrayBufferObj;

   /** MESA_META_VIEWPORT */
   GLint ViewportX, ViewportY, ViewportW, ViewportH;
   GLclampd DepthNear, DepthFar;

   /** MESA_META_CLAMP_FRAGMENT_COLOR */
   GLenum ClampFragmentColor;

   /** MESA_META_CLAMP_VERTEX_COLOR */
   GLenum ClampVertexColor;

   /** MESA_META_CONDITIONAL_RENDER */
   struct gl_query_object *CondRenderQuery;
   GLenum CondRenderMode;

   /** MESA_META_SELECT_FEEDBACK */
   GLenum RenderMode;
   struct gl_selection Select;
   struct gl_feedback Feedback;

   /** Miscellaneous (always disabled) */
   GLboolean Lighting;
   GLboolean RasterDiscard;
   GLboolean TransformFeedbackNeedsResume;
};

/**
 * Temporary texture used for glBlitFramebuffer, glDrawPixels, etc.
 * This is currently shared by all the meta ops.  But we could create a
 * separate one for each of glDrawPixel, glBlitFramebuffer, glCopyPixels, etc.
 */
struct temp_texture
{
   GLuint TexObj;
   GLenum Target;         /**< GL_TEXTURE_2D or GL_TEXTURE_RECTANGLE */
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


/**
 * State for glDrawPixels()
 */
struct drawpix_state
{
   GLuint ArrayObj;

   GLuint StencilFP;  /**< Fragment program for drawing stencil images */
   GLuint DepthFP;  /**< Fragment program for drawing depth images */
};


/**
 * State for glBitmap()
 */
struct bitmap_state
{
   GLuint ArrayObj;
   GLuint VBO;
   struct temp_texture Tex;  /**< separate texture from other meta ops */
};


/**
 * State for _mesa_meta_generate_mipmap()
 */
struct gen_mipmap_state
{
   GLuint ArrayObj;
   GLuint VBO;
   GLuint FBO;
};


/**
 * State for texture decompression
 */
struct decompress_state
{
   GLuint ArrayObj;
   GLuint VBO, FBO, RBO;
   GLint Width, Height;
};

/**
 * State for glDrawTex()
 */
struct drawtex_state
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

   struct blit_state Blit;    /**< For _mesa_meta_BlitFramebuffer() */
   struct clear_state Clear;  /**< For _mesa_meta_Clear() */
   struct copypix_state CopyPix;  /**< For _mesa_meta_CopyPixels() */
   struct drawpix_state DrawPix;  /**< For _mesa_meta_DrawPixels() */
   struct bitmap_state Bitmap;    /**< For _mesa_meta_Bitmap() */
   struct gen_mipmap_state Mipmap;    /**< For _mesa_meta_GenerateMipmap() */
   struct decompress_state Decompress;  /**< For texture decompression */
   struct drawtex_state DrawTex;  /**< For _mesa_meta_DrawTex() */
};

static void meta_glsl_blit_cleanup(struct gl_context *ctx, struct blit_state *blit);
static void cleanup_temp_texture(struct gl_context *ctx, struct temp_texture *tex);
static void meta_glsl_clear_cleanup(struct gl_context *ctx, struct clear_state *clear);

static GLuint
compile_shader_with_debug(struct gl_context *ctx, GLenum target, const GLcharARB *source)
{
   GLuint shader;
   GLint ok, size;
   GLchar *info;

   shader = _mesa_CreateShaderObjectARB(target);
   _mesa_ShaderSourceARB(shader, 1, &source, NULL);
   _mesa_CompileShaderARB(shader);

   _mesa_GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
   if (ok)
      return shader;

   _mesa_GetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
   if (size == 0) {
      _mesa_DeleteObjectARB(shader);
      return 0;
   }

   info = malloc(size);
   if (!info) {
      _mesa_DeleteObjectARB(shader);
      return 0;
   }

   _mesa_GetProgramInfoLog(shader, size, NULL, info);
   _mesa_problem(ctx,
		 "meta program compile failed:\n%s\n"
		 "source:\n%s\n",
		 info, source);

   free(info);
   _mesa_DeleteObjectARB(shader);

   return 0;
}

static GLuint
link_program_with_debug(struct gl_context *ctx, GLuint program)
{
   GLint ok, size;
   GLchar *info;

   _mesa_LinkProgramARB(program);

   _mesa_GetProgramiv(program, GL_LINK_STATUS, &ok);
   if (ok)
      return program;

   _mesa_GetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
   if (size == 0)
      return 0;

   info = malloc(size);
   if (!info)
      return 0;

   _mesa_GetProgramInfoLog(program, size, NULL, info);
   _mesa_problem(ctx, "meta program link failed:\n%s", info);

   free(info);

   return 0;
}

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
   meta_glsl_blit_cleanup(ctx, &ctx->Meta->Blit);
   meta_glsl_clear_cleanup(ctx, &ctx->Meta->Clear);
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

   /* Pausing transform feedback needs to be done early, or else we won't be
    * able to change other state.
    */
   save->TransformFeedbackNeedsResume =
      ctx->TransformFeedback.CurrentObject->Active &&
      !ctx->TransformFeedback.CurrentObject->Paused;
   if (save->TransformFeedbackNeedsResume)
      _mesa_PauseTransformFeedback();

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
         if (ctx->Extensions.EXT_draw_buffers2) {
            GLuint i;
            for (i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
               _mesa_set_enablei(ctx, GL_BLEND, i, GL_FALSE);
            }
         }
         else {
            _mesa_set_enable(ctx, GL_BLEND, GL_FALSE);
         }
      }
      save->ColorLogicOpEnabled = ctx->Color.ColorLogicOpEnabled;
      if (ctx->Color.ColorLogicOpEnabled)
         _mesa_set_enable(ctx, GL_COLOR_LOGIC_OP, GL_FALSE);
   }

   if (state & MESA_META_COLOR_MASK) {
      memcpy(save->ColorMask, ctx->Color.ColorMask,
             sizeof(ctx->Color.ColorMask));
      if (!ctx->Color.ColorMask[0][0] ||
          !ctx->Color.ColorMask[0][1] ||
          !ctx->Color.ColorMask[0][2] ||
          !ctx->Color.ColorMask[0][3])
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

   if (state & MESA_META_SHADER) {
      if (ctx->Extensions.ARB_vertex_program) {
         save->VertexProgramEnabled = ctx->VertexProgram.Enabled;
         _mesa_reference_vertprog(ctx, &save->VertexProgram,
				  ctx->VertexProgram.Current);
         _mesa_set_enable(ctx, GL_VERTEX_PROGRAM_ARB, GL_FALSE);
      }

      if (ctx->Extensions.ARB_fragment_program) {
         save->FragmentProgramEnabled = ctx->FragmentProgram.Enabled;
         _mesa_reference_fragprog(ctx, &save->FragmentProgram,
				  ctx->FragmentProgram.Current);
         _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB, GL_FALSE);
      }

      if (ctx->Extensions.ARB_shader_objects) {
	 _mesa_reference_shader_program(ctx, &save->VertexShader,
					ctx->Shader.CurrentVertexProgram);
	 _mesa_reference_shader_program(ctx, &save->GeometryShader,
					ctx->Shader.CurrentGeometryProgram);
	 _mesa_reference_shader_program(ctx, &save->FragmentShader,
					ctx->Shader.CurrentFragmentProgram);
	 _mesa_reference_shader_program(ctx, &save->ActiveShader,
					ctx->Shader.ActiveProgram);

         _mesa_UseProgramObjectARB(0);
      }
   }

   if (state & MESA_META_STENCIL_TEST) {
      save->Stencil = ctx->Stencil; /* struct copy */
      if (ctx->Stencil.Enabled)
         _mesa_set_enable(ctx, GL_STENCIL_TEST, GL_FALSE);
      /* NOTE: other stencil state not reset */
   }

   if (state & MESA_META_TEXTURE) {
      GLuint u, tgt;

      save->ActiveUnit = ctx->Texture.CurrentUnit;
      save->ClientActiveUnit = ctx->Array.ActiveTexture;
      save->EnvMode = ctx->Texture.Unit[0].EnvMode;

      /* Disable all texture units */
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
         save->TexEnabled[u] = ctx->Texture.Unit[u].Enabled;
         save->TexGenEnabled[u] = ctx->Texture.Unit[u].TexGenEnabled;
         if (ctx->Texture.Unit[u].Enabled ||
             ctx->Texture.Unit[u].TexGenEnabled) {
            _mesa_ActiveTextureARB(GL_TEXTURE0 + u);
            _mesa_set_enable(ctx, GL_TEXTURE_1D, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_2D, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_3D, GL_FALSE);
            if (ctx->Extensions.ARB_texture_cube_map)
               _mesa_set_enable(ctx, GL_TEXTURE_CUBE_MAP, GL_FALSE);
            if (ctx->Extensions.NV_texture_rectangle)
               _mesa_set_enable(ctx, GL_TEXTURE_RECTANGLE, GL_FALSE);
            if (ctx->Extensions.OES_EGL_image_external)
               _mesa_set_enable(ctx, GL_TEXTURE_EXTERNAL_OES, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_GEN_S, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_GEN_T, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_GEN_R, GL_FALSE);
            _mesa_set_enable(ctx, GL_TEXTURE_GEN_Q, GL_FALSE);
         }
      }

      /* save current texture objects for unit[0] only */
      for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
         _mesa_reference_texobj(&save->CurrentTexture[tgt],
                                ctx->Texture.Unit[0].CurrentTex[tgt]);
      }

      /* set defaults for unit[0] */
      _mesa_ActiveTextureARB(GL_TEXTURE0);
      _mesa_ClientActiveTextureARB(GL_TEXTURE0);
      _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   }

   if (state & MESA_META_TRANSFORM) {
      GLuint activeTexture = ctx->Texture.CurrentUnit;
      memcpy(save->ModelviewMatrix, ctx->ModelviewMatrixStack.Top->m,
             16 * sizeof(GLfloat));
      memcpy(save->ProjectionMatrix, ctx->ProjectionMatrixStack.Top->m,
             16 * sizeof(GLfloat));
      memcpy(save->TextureMatrix, ctx->TextureMatrixStack[0].Top->m,
             16 * sizeof(GLfloat));
      save->MatrixMode = ctx->Transform.MatrixMode;
      /* set 1:1 vertex:pixel coordinate transform */
      _mesa_ActiveTextureARB(GL_TEXTURE0);
      _mesa_MatrixMode(GL_TEXTURE);
      _mesa_LoadIdentity();
      _mesa_ActiveTextureARB(GL_TEXTURE0 + activeTexture);
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

   if (state & MESA_META_CLAMP_FRAGMENT_COLOR) {
      save->ClampFragmentColor = ctx->Color.ClampFragmentColor;

      /* Generally in here we want to do clamping according to whether
       * it's for the pixel path (ClampFragmentColor is GL_TRUE),
       * regardless of the internal implementation of the metaops.
       */
      if (ctx->Color.ClampFragmentColor != GL_TRUE)
	 _mesa_ClampColorARB(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
   }

   if (state & MESA_META_CLAMP_VERTEX_COLOR) {
      save->ClampVertexColor = ctx->Light.ClampVertexColor;

      /* Generally in here we never want vertex color clamping --
       * result clamping is only dependent on fragment clamping.
       */
      _mesa_ClampColorARB(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
   }

   if (state & MESA_META_CONDITIONAL_RENDER) {
      save->CondRenderQuery = ctx->Query.CondRenderQuery;
      save->CondRenderMode = ctx->Query.CondRenderMode;

      if (ctx->Query.CondRenderQuery)
	 _mesa_EndConditionalRender();
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
         if (ctx->Extensions.EXT_draw_buffers2) {
            GLuint i;
            for (i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
               _mesa_set_enablei(ctx, GL_BLEND, i, (save->BlendEnabled >> i) & 1);
            }
         }
         else {
            _mesa_set_enable(ctx, GL_BLEND, (save->BlendEnabled & 1));
         }
      }
      if (ctx->Color.ColorLogicOpEnabled != save->ColorLogicOpEnabled)
         _mesa_set_enable(ctx, GL_COLOR_LOGIC_OP, save->ColorLogicOpEnabled);
   }

   if (state & MESA_META_COLOR_MASK) {
      GLuint i;
      for (i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
         if (!TEST_EQ_4V(ctx->Color.ColorMask[i], save->ColorMask[i])) {
            if (i == 0) {
               _mesa_ColorMask(save->ColorMask[i][0], save->ColorMask[i][1],
                               save->ColorMask[i][2], save->ColorMask[i][3]);
            }
            else {
               _mesa_ColorMaskIndexed(i,
                                      save->ColorMask[i][0],
                                      save->ColorMask[i][1],
                                      save->ColorMask[i][2],
                                      save->ColorMask[i][3]);
            }
         }
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

   if (state & MESA_META_SHADER) {
      if (ctx->Extensions.ARB_vertex_program) {
         _mesa_set_enable(ctx, GL_VERTEX_PROGRAM_ARB,
                          save->VertexProgramEnabled);
         _mesa_reference_vertprog(ctx, &ctx->VertexProgram.Current, 
                                  save->VertexProgram);
	 _mesa_reference_vertprog(ctx, &save->VertexProgram, NULL);
      }

      if (ctx->Extensions.ARB_fragment_program) {
         _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB,
                          save->FragmentProgramEnabled);
         _mesa_reference_fragprog(ctx, &ctx->FragmentProgram.Current,
                                  save->FragmentProgram);
	 _mesa_reference_fragprog(ctx, &save->FragmentProgram, NULL);
      }

      if (ctx->Extensions.ARB_vertex_shader)
	 _mesa_use_shader_program(ctx, GL_VERTEX_SHADER, save->VertexShader);

      if (ctx->Extensions.ARB_geometry_shader4)
	 _mesa_use_shader_program(ctx, GL_GEOMETRY_SHADER_ARB,
				  save->GeometryShader);

      if (ctx->Extensions.ARB_fragment_shader)
	 _mesa_use_shader_program(ctx, GL_FRAGMENT_SHADER,
				  save->FragmentShader);

      _mesa_reference_shader_program(ctx, &ctx->Shader.ActiveProgram,
				     save->ActiveShader);

      _mesa_reference_shader_program(ctx, &save->VertexShader, NULL);
      _mesa_reference_shader_program(ctx, &save->GeometryShader, NULL);
      _mesa_reference_shader_program(ctx, &save->FragmentShader, NULL);
      _mesa_reference_shader_program(ctx, &save->ActiveShader, NULL);
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
      GLuint u, tgt;

      ASSERT(ctx->Texture.CurrentUnit == 0);

      /* restore texenv for unit[0] */
      _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, save->EnvMode);

      /* restore texture objects for unit[0] only */
      for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
	 if (ctx->Texture.Unit[0].CurrentTex[tgt] != save->CurrentTexture[tgt]) {
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    _mesa_reference_texobj(&ctx->Texture.Unit[0].CurrentTex[tgt],
				   save->CurrentTexture[tgt]);
	 }
         _mesa_reference_texobj(&save->CurrentTexture[tgt], NULL);
      }

      /* Restore fixed function texture enables, texgen */
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
	 if (ctx->Texture.Unit[u].Enabled != save->TexEnabled[u]) {
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    ctx->Texture.Unit[u].Enabled = save->TexEnabled[u];
	 }

	 if (ctx->Texture.Unit[u].TexGenEnabled != save->TexGenEnabled[u]) {
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    ctx->Texture.Unit[u].TexGenEnabled = save->TexGenEnabled[u];
	 }
      }

      /* restore current unit state */
      _mesa_ActiveTextureARB(GL_TEXTURE0 + save->ActiveUnit);
      _mesa_ClientActiveTextureARB(GL_TEXTURE0 + save->ClientActiveUnit);
   }

   if (state & MESA_META_TRANSFORM) {
      GLuint activeTexture = ctx->Texture.CurrentUnit;
      _mesa_ActiveTextureARB(GL_TEXTURE0);
      _mesa_MatrixMode(GL_TEXTURE);
      _mesa_LoadMatrixf(save->TextureMatrix);
      _mesa_ActiveTextureARB(GL_TEXTURE0 + activeTexture);

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

   if (state & MESA_META_CLAMP_FRAGMENT_COLOR) {
      _mesa_ClampColorARB(GL_CLAMP_FRAGMENT_COLOR, save->ClampFragmentColor);
   }

   if (state & MESA_META_CLAMP_VERTEX_COLOR) {
      _mesa_ClampColorARB(GL_CLAMP_VERTEX_COLOR, save->ClampVertexColor);
   }

   if (state & MESA_META_CONDITIONAL_RENDER) {
      if (save->CondRenderQuery)
	 _mesa_BeginConditionalRender(save->CondRenderQuery->Id,
				      save->CondRenderMode);
   }

   if (state & MESA_META_SELECT_FEEDBACK) {
      if (save->RenderMode == GL_SELECT) {
	 _mesa_RenderMode(GL_SELECT);
	 ctx->Select = save->Select;
      } else if (save->RenderMode == GL_FEEDBACK) {
	 _mesa_RenderMode(GL_FEEDBACK);
	 ctx->Feedback = save->Feedback;
      }
   }

   /* misc */
   if (save->Lighting) {
      _mesa_set_enable(ctx, GL_LIGHTING, GL_TRUE);
   }
   if (save->RasterDiscard) {
      _mesa_set_enable(ctx, GL_RASTERIZER_DISCARD, GL_TRUE);
   }
   if (save->TransformFeedbackNeedsResume)
      _mesa_ResumeTransformFeedback();
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
   /* prefer texture rectangle */
   if (ctx->Extensions.NV_texture_rectangle) {
      tex->Target = GL_TEXTURE_RECTANGLE;
      tex->MaxSize = ctx->Const.MaxTextureRectSize;
      tex->NPOT = GL_TRUE;
   }
   else {
      /* use 2D texture, NPOT if possible */
      tex->Target = GL_TEXTURE_2D;
      tex->MaxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      tex->NPOT = ctx->Extensions.ARB_texture_non_power_of_two;
   }
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
 * Return pointer to temp_texture info for _mesa_meta_bitmap().
 * We use a separate texture for bitmaps to reduce texture
 * allocation/deallocation.
 */
static struct temp_texture *
get_bitmap_temp_texture(struct gl_context *ctx)
{
   struct temp_texture *tex = &ctx->Meta->Bitmap.Tex;

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
   if (tex->Target == GL_TEXTURE_RECTANGLE) {
      tex->Sright = (GLfloat) width;
      tex->Ttop = (GLfloat) height;
   }
   else {
      tex->Sright = (GLfloat) width / tex->Width;
      tex->Ttop = (GLfloat) height / tex->Height;
   }

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
 * Setup/load texture for glDrawPixels.
 */
static void
setup_drawpix_texture(struct gl_context *ctx,
		      struct temp_texture *tex,
                      GLboolean newTex,
                      GLenum texIntFormat,
                      GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const GLvoid *pixels)
{
   _mesa_BindTexture(tex->Target, tex->TexObj);
   _mesa_TexParameteri(tex->Target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   _mesa_TexParameteri(tex->Target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   /* copy pixel data to texture */
   if (newTex) {
      /* create new tex image */
      if (tex->Width == width && tex->Height == height) {
         /* create new tex and load image data */
         _mesa_TexImage2D(tex->Target, 0, tex->IntFormat,
                          tex->Width, tex->Height, 0, format, type, pixels);
      }
      else {
	 struct gl_buffer_object *save_unpack_obj = NULL;

	 _mesa_reference_buffer_object(ctx, &save_unpack_obj,
				       ctx->Unpack.BufferObj);
	 _mesa_BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
         /* create empty texture */
         _mesa_TexImage2D(tex->Target, 0, tex->IntFormat,
                          tex->Width, tex->Height, 0, format, type, NULL);
	 if (save_unpack_obj != NULL)
	    _mesa_BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
				save_unpack_obj->Name);
         /* load image */
         _mesa_TexSubImage2D(tex->Target, 0,
                             0, 0, width, height, format, type, pixels);
      }
   }
   else {
      /* replace existing tex image */
      _mesa_TexSubImage2D(tex->Target, 0,
                          0, 0, width, height, format, type, pixels);
   }
}



/**
 * One-time init for drawing depth pixels.
 */
static void
init_blit_depth_pixels(struct gl_context *ctx)
{
   static const char *program =
      "!!ARBfp1.0\n"
      "TEX result.depth, fragment.texcoord[0], texture[0], %s; \n"
      "END \n";
   char program2[200];
   struct blit_state *blit = &ctx->Meta->Blit;
   struct temp_texture *tex = get_temp_texture(ctx);
   const char *texTarget;

   assert(blit->DepthFP == 0);

   /* replace %s with "RECT" or "2D" */
   assert(strlen(program) + 4 < sizeof(program2));
   if (tex->Target == GL_TEXTURE_RECTANGLE)
      texTarget = "RECT";
   else
      texTarget = "2D";
   _mesa_snprintf(program2, sizeof(program2), program, texTarget);

   _mesa_GenPrograms(1, &blit->DepthFP);
   _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, blit->DepthFP);
   _mesa_ProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                          strlen(program2), (const GLubyte *) program2);
}


/**
 * Try to do a glBlitFramebuffer using no-copy texturing.
 * We can do this when the src renderbuffer is actually a texture.
 * But if the src buffer == dst buffer we cannot do this.
 *
 * \return new buffer mask indicating the buffers left to blit using the
 *         normal path.
 */
static GLbitfield
blitframebuffer_texture(struct gl_context *ctx,
                        GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                        GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                        GLbitfield mask, GLenum filter)
{
   if (mask & GL_COLOR_BUFFER_BIT) {
      const struct gl_framebuffer *drawFb = ctx->DrawBuffer;
      const struct gl_framebuffer *readFb = ctx->ReadBuffer;
      const struct gl_renderbuffer_attachment *drawAtt =
         &drawFb->Attachment[drawFb->_ColorDrawBufferIndexes[0]];
      const struct gl_renderbuffer_attachment *readAtt =
         &readFb->Attachment[readFb->_ColorReadBufferIndex];

      if (readAtt && readAtt->Texture) {
         const struct gl_texture_object *texObj = readAtt->Texture;
         const GLuint srcLevel = readAtt->TextureLevel;
         const GLenum minFilterSave = texObj->Sampler.MinFilter;
         const GLenum magFilterSave = texObj->Sampler.MagFilter;
         const GLint baseLevelSave = texObj->BaseLevel;
         const GLint maxLevelSave = texObj->MaxLevel;
         const GLenum wrapSSave = texObj->Sampler.WrapS;
         const GLenum wrapTSave = texObj->Sampler.WrapT;
         const GLenum srgbSave = texObj->Sampler.sRGBDecode;
         const GLenum fbo_srgb_save = ctx->Color.sRGBEnabled;
         const GLenum target = texObj->Target;

         if (drawAtt->Texture == readAtt->Texture) {
            /* Can't use same texture as both the source and dest.  We need
             * to handle overlapping blits and besides, some hw may not
             * support this.
             */
            return mask;
         }

         if (target != GL_TEXTURE_2D && target != GL_TEXTURE_RECTANGLE_ARB) {
            /* Can't handle other texture types at this time */
            return mask;
         }

         /*
         printf("Blit from texture!\n");
         printf("  srcAtt %p  dstAtt %p\n", readAtt, drawAtt);
         printf("  srcTex %p  dstText %p\n", texObj, drawAtt->Texture);
         */

         /* Prepare src texture state */
         _mesa_BindTexture(target, texObj->Name);
         _mesa_TexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
         _mesa_TexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
         if (target != GL_TEXTURE_RECTANGLE_ARB) {
            _mesa_TexParameteri(target, GL_TEXTURE_BASE_LEVEL, srcLevel);
            _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, srcLevel);
         }
         _mesa_TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
         _mesa_TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	 /* Always do our blits with no sRGB decode or encode.*/
	 if (ctx->Extensions.EXT_texture_sRGB_decode) {
	    _mesa_TexParameteri(target, GL_TEXTURE_SRGB_DECODE_EXT,
				GL_SKIP_DECODE_EXT);
	 }
         if (ctx->Extensions.EXT_framebuffer_sRGB) {
            _mesa_set_enable(ctx, GL_FRAMEBUFFER_SRGB_EXT, GL_FALSE);
         }

         _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
         _mesa_set_enable(ctx, target, GL_TRUE);

         /* Prepare vertex data (the VBO was previously created and bound) */
         {
            struct vertex {
               GLfloat x, y, s, t;
            };
            struct vertex verts[4];
            GLfloat s0, t0, s1, t1;

            if (target == GL_TEXTURE_2D) {
               const struct gl_texture_image *texImage
                   = _mesa_select_tex_image(ctx, texObj, target, srcLevel);
               s0 = srcX0 / (float) texImage->Width;
               s1 = srcX1 / (float) texImage->Width;
               t0 = srcY0 / (float) texImage->Height;
               t1 = srcY1 / (float) texImage->Height;
            }
            else {
               assert(target == GL_TEXTURE_RECTANGLE_ARB);
               s0 = srcX0;
               s1 = srcX1;
               t0 = srcY0;
               t1 = srcY1;
            }

            verts[0].x = (GLfloat) dstX0;
            verts[0].y = (GLfloat) dstY0;
            verts[1].x = (GLfloat) dstX1;
            verts[1].y = (GLfloat) dstY0;
            verts[2].x = (GLfloat) dstX1;
            verts[2].y = (GLfloat) dstY1;
            verts[3].x = (GLfloat) dstX0;
            verts[3].y = (GLfloat) dstY1;

            verts[0].s = s0;
            verts[0].t = t0;
            verts[1].s = s1;
            verts[1].t = t0;
            verts[2].s = s1;
            verts[2].t = t1;
            verts[3].s = s0;
            verts[3].t = t1;

            _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
         }

         _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

         /* Restore texture object state, the texture binding will
          * be restored by _mesa_meta_end().
          */
         _mesa_TexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilterSave);
         _mesa_TexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilterSave);
         if (target != GL_TEXTURE_RECTANGLE_ARB) {
            _mesa_TexParameteri(target, GL_TEXTURE_BASE_LEVEL, baseLevelSave);
            _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxLevelSave);
         }
         _mesa_TexParameteri(target, GL_TEXTURE_WRAP_S, wrapSSave);
         _mesa_TexParameteri(target, GL_TEXTURE_WRAP_T, wrapTSave);
	 if (ctx->Extensions.EXT_texture_sRGB_decode) {
	    _mesa_TexParameteri(target, GL_TEXTURE_SRGB_DECODE_EXT, srgbSave);
	 }
	 if (ctx->Extensions.EXT_framebuffer_sRGB && fbo_srgb_save) {
	    _mesa_set_enable(ctx, GL_FRAMEBUFFER_SRGB_EXT, GL_TRUE);
	 }

         /* Done with color buffer */
         mask &= ~GL_COLOR_BUFFER_BIT;
      }
   }

   return mask;
}


/**
 * Meta implementation of ctx->Driver.BlitFramebuffer() in terms
 * of texture mapping and polygon rendering.
 */
void
_mesa_meta_BlitFramebuffer(struct gl_context *ctx,
                           GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter)
{
   struct blit_state *blit = &ctx->Meta->Blit;
   struct temp_texture *tex = get_temp_texture(ctx);
   const GLsizei maxTexSize = tex->MaxSize;
   const GLint srcX = MIN2(srcX0, srcX1);
   const GLint srcY = MIN2(srcY0, srcY1);
   const GLint srcW = abs(srcX1 - srcX0);
   const GLint srcH = abs(srcY1 - srcY0);
   const GLboolean srcFlipX = srcX1 < srcX0;
   const GLboolean srcFlipY = srcY1 < srcY0;
   struct vertex {
      GLfloat x, y, s, t;
   };
   struct vertex verts[4];
   GLboolean newTex;

   /* In addition to falling back if the blit size is larger than the maximum
    * texture size, fallback if the source is multisampled.  This fallback can
    * be removed once Mesa gets support ARB_texture_multisample.
    */
   if (srcW > maxTexSize || srcH > maxTexSize
       || ctx->ReadBuffer->Visual.samples > 0) {
      /* XXX avoid this fallback */
      _swrast_BlitFramebuffer(ctx, srcX0, srcY0, srcX1, srcY1,
                              dstX0, dstY0, dstX1, dstY1, mask, filter);
      return;
   }

   if (srcFlipX) {
      GLint tmp = dstX0;
      dstX0 = dstX1;
      dstX1 = tmp;
   }

   if (srcFlipY) {
      GLint tmp = dstY0;
      dstY0 = dstY1;
      dstY1 = tmp;
   }

   /* only scissor effects blit so save/clear all other relevant state */
   _mesa_meta_begin(ctx, ~MESA_META_SCISSOR);

   if (blit->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArrays(1, &blit->ArrayObj);
      _mesa_BindVertexArray(blit->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &blit->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, blit->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                          NULL, GL_DYNAMIC_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_TexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(s));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
   }
   else {
      _mesa_BindVertexArray(blit->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, blit->VBO);
   }

   /* Try faster, direct texture approach first */
   mask = blitframebuffer_texture(ctx, srcX0, srcY0, srcX1, srcY1,
                                  dstX0, dstY0, dstX1, dstY1, mask, filter);
   if (mask == 0x0) {
      _mesa_meta_end(ctx);
      return;
   }

   /* Continue with "normal" approach which involves copying the src rect
    * into a temporary texture and is "blitted" by drawing a textured quad.
    */

   newTex = alloc_texture(tex, srcW, srcH, GL_RGBA);

   /* vertex positions/texcoords (after texture allocation!) */
   {
      verts[0].x = (GLfloat) dstX0;
      verts[0].y = (GLfloat) dstY0;
      verts[1].x = (GLfloat) dstX1;
      verts[1].y = (GLfloat) dstY0;
      verts[2].x = (GLfloat) dstX1;
      verts[2].y = (GLfloat) dstY1;
      verts[3].x = (GLfloat) dstX0;
      verts[3].y = (GLfloat) dstY1;

      verts[0].s = 0.0F;
      verts[0].t = 0.0F;
      verts[1].s = tex->Sright;
      verts[1].t = 0.0F;
      verts[2].s = tex->Sright;
      verts[2].t = tex->Ttop;
      verts[3].s = 0.0F;
      verts[3].t = tex->Ttop;

      /* upload new vertex data */
      _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
   }

   _mesa_set_enable(ctx, tex->Target, GL_TRUE);

   if (mask & GL_COLOR_BUFFER_BIT) {
      setup_copypix_texture(tex, newTex, srcX, srcY, srcW, srcH,
                            GL_RGBA, filter);
      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
      mask &= ~GL_COLOR_BUFFER_BIT;
   }

   if (mask & GL_DEPTH_BUFFER_BIT) {
      GLuint *tmp = (GLuint *) malloc(srcW * srcH * sizeof(GLuint));
      if (tmp) {
         if (!blit->DepthFP)
            init_blit_depth_pixels(ctx);

         /* maybe change tex format here */
         newTex = alloc_texture(tex, srcW, srcH, GL_DEPTH_COMPONENT);

         _mesa_ReadPixels(srcX, srcY, srcW, srcH,
                          GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, tmp);

         setup_drawpix_texture(ctx, tex, newTex, GL_DEPTH_COMPONENT, srcW, srcH,
                               GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, tmp);

         _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, blit->DepthFP);
         _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB, GL_TRUE);
         _mesa_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
         _mesa_set_enable(ctx, GL_DEPTH_TEST, GL_TRUE);
         _mesa_DepthFunc(GL_ALWAYS);
         _mesa_DepthMask(GL_TRUE);

         _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
         mask &= ~GL_DEPTH_BUFFER_BIT;

         free(tmp);
      }
   }

   if (mask & GL_STENCIL_BUFFER_BIT) {
      /* XXX can't easily do stencil */
   }

   _mesa_set_enable(ctx, tex->Target, GL_FALSE);

   _mesa_meta_end(ctx);

   if (mask) {
      _swrast_BlitFramebuffer(ctx, srcX0, srcY0, srcX1, srcY1,
                              dstX0, dstY0, dstX1, dstY1, mask, filter);
   }
}

static void
meta_glsl_blit_cleanup(struct gl_context *ctx, struct blit_state *blit)
{
   if (blit->ArrayObj) {
      _mesa_DeleteVertexArraysAPPLE(1, &blit->ArrayObj);
      blit->ArrayObj = 0;
      _mesa_DeleteBuffersARB(1, &blit->VBO);
      blit->VBO = 0;
   }
   if (blit->DepthFP) {
      _mesa_DeletePrograms(1, &blit->DepthFP);
      blit->DepthFP = 0;
   }
}


/**
 * Meta implementation of ctx->Driver.Clear() in terms of polygon rendering.
 */
void
_mesa_meta_Clear(struct gl_context *ctx, GLbitfield buffers)
{
   struct clear_state *clear = &ctx->Meta->Clear;
   struct vertex {
      GLfloat x, y, z, r, g, b, a;
   };
   struct vertex verts[4];
   /* save all state but scissor, pixel pack/unpack */
   GLbitfield metaSave = (MESA_META_ALL -
			  MESA_META_SCISSOR -
			  MESA_META_PIXEL_STORE -
			  MESA_META_CONDITIONAL_RENDER);
   const GLuint stencilMax = (1 << ctx->DrawBuffer->Visual.stencilBits) - 1;

   if (buffers & BUFFER_BITS_COLOR) {
      /* if clearing color buffers, don't save/restore colormask */
      metaSave -= MESA_META_COLOR_MASK;
   }

   _mesa_meta_begin(ctx, metaSave);

   if (clear->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArrays(1, &clear->ArrayObj);
      _mesa_BindVertexArray(clear->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &clear->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, clear->VBO);

      /* setup vertex arrays */
      _mesa_VertexPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_ColorPointer(4, GL_FLOAT, sizeof(struct vertex), OFFSET(r));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_COLOR_ARRAY);
   }
   else {
      _mesa_BindVertexArray(clear->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, clear->VBO);
   }

   /* GL_COLOR_BUFFER_BIT */
   if (buffers & BUFFER_BITS_COLOR) {
      /* leave colormask, glDrawBuffer state as-is */

      /* Clears never have the color clamped. */
      _mesa_ClampColorARB(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
   }
   else {
      ASSERT(metaSave & MESA_META_COLOR_MASK);
      _mesa_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
   }

   /* GL_DEPTH_BUFFER_BIT */
   if (buffers & BUFFER_BIT_DEPTH) {
      _mesa_set_enable(ctx, GL_DEPTH_TEST, GL_TRUE);
      _mesa_DepthFunc(GL_ALWAYS);
      _mesa_DepthMask(GL_TRUE);
   }
   else {
      assert(!ctx->Depth.Test);
   }

   /* GL_STENCIL_BUFFER_BIT */
   if (buffers & BUFFER_BIT_STENCIL) {
      _mesa_set_enable(ctx, GL_STENCIL_TEST, GL_TRUE);
      _mesa_StencilOpSeparate(GL_FRONT_AND_BACK,
                              GL_REPLACE, GL_REPLACE, GL_REPLACE);
      _mesa_StencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS,
                                ctx->Stencil.Clear & stencilMax,
                                ctx->Stencil.WriteMask[0]);
   }
   else {
      assert(!ctx->Stencil.Enabled);
   }

   /* vertex positions/colors */
   {
      const GLfloat x0 = (GLfloat) ctx->DrawBuffer->_Xmin;
      const GLfloat y0 = (GLfloat) ctx->DrawBuffer->_Ymin;
      const GLfloat x1 = (GLfloat) ctx->DrawBuffer->_Xmax;
      const GLfloat y1 = (GLfloat) ctx->DrawBuffer->_Ymax;
      const GLfloat z = invert_z(ctx->Depth.Clear);
      GLuint i;

      verts[0].x = x0;
      verts[0].y = y0;
      verts[0].z = z;
      verts[1].x = x1;
      verts[1].y = y0;
      verts[1].z = z;
      verts[2].x = x1;
      verts[2].y = y1;
      verts[2].z = z;
      verts[3].x = x0;
      verts[3].y = y1;
      verts[3].z = z;

      /* vertex colors */
      for (i = 0; i < 4; i++) {
         verts[i].r = ctx->Color.ClearColor.f[0];
         verts[i].g = ctx->Color.ClearColor.f[1];
         verts[i].b = ctx->Color.ClearColor.f[2];
         verts[i].a = ctx->Color.ClearColor.f[3];
      }

      /* upload new vertex data */
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts), verts,
			  GL_DYNAMIC_DRAW_ARB);
   }

   /* draw quad */
   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   _mesa_meta_end(ctx);
}

static void
meta_glsl_clear_init(struct gl_context *ctx, struct clear_state *clear)
{
   const char *vs_source =
      "attribute vec4 position;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = position;\n"
      "}\n";
   const char *fs_source =
      "uniform vec4 color;\n"
      "void main()\n"
      "{\n"
      "   gl_FragColor = color;\n"
      "}\n";
   const char *vs_int_source =
      "#version 130\n"
      "attribute vec4 position;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = position;\n"
      "}\n";
   const char *fs_int_source =
      "#version 130\n"
      "uniform ivec4 color;\n"
      "out ivec4 out_color;\n"
      "\n"
      "void main()\n"
      "{\n"
      "   out_color = color;\n"
      "}\n";
   GLuint vs, fs;

   if (clear->ArrayObj != 0)
      return;

   /* create vertex array object */
   _mesa_GenVertexArrays(1, &clear->ArrayObj);
   _mesa_BindVertexArray(clear->ArrayObj);

   /* create vertex array buffer */
   _mesa_GenBuffersARB(1, &clear->VBO);
   _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, clear->VBO);

   /* setup vertex arrays */
   _mesa_VertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
   _mesa_EnableVertexAttribArrayARB(0);

   vs = _mesa_CreateShaderObjectARB(GL_VERTEX_SHADER);
   _mesa_ShaderSourceARB(vs, 1, &vs_source, NULL);
   _mesa_CompileShaderARB(vs);

   fs = _mesa_CreateShaderObjectARB(GL_FRAGMENT_SHADER);
   _mesa_ShaderSourceARB(fs, 1, &fs_source, NULL);
   _mesa_CompileShaderARB(fs);

   clear->ShaderProg = _mesa_CreateProgramObjectARB();
   _mesa_AttachShader(clear->ShaderProg, fs);
   _mesa_DeleteObjectARB(fs);
   _mesa_AttachShader(clear->ShaderProg, vs);
   _mesa_DeleteObjectARB(vs);
   _mesa_BindAttribLocationARB(clear->ShaderProg, 0, "position");
   _mesa_LinkProgramARB(clear->ShaderProg);

   clear->ColorLocation = _mesa_GetUniformLocationARB(clear->ShaderProg,
						      "color");

   if (ctx->Const.GLSLVersion >= 130) {
      vs = compile_shader_with_debug(ctx, GL_VERTEX_SHADER, vs_int_source);
      fs = compile_shader_with_debug(ctx, GL_FRAGMENT_SHADER, fs_int_source);

      clear->IntegerShaderProg = _mesa_CreateProgramObjectARB();
      _mesa_AttachShader(clear->IntegerShaderProg, fs);
      _mesa_DeleteObjectARB(fs);
      _mesa_AttachShader(clear->IntegerShaderProg, vs);
      _mesa_DeleteObjectARB(vs);
      _mesa_BindAttribLocationARB(clear->IntegerShaderProg, 0, "position");

      /* Note that user-defined out attributes get automatically assigned
       * locations starting from 0, so we don't need to explicitly
       * BindFragDataLocation to 0.
       */

      link_program_with_debug(ctx, clear->IntegerShaderProg);

      clear->IntegerColorLocation =
	 _mesa_GetUniformLocationARB(clear->IntegerShaderProg, "color");
   }
}

static void
meta_glsl_clear_cleanup(struct gl_context *ctx, struct clear_state *clear)
{
   if (clear->ArrayObj == 0)
      return;
   _mesa_DeleteVertexArraysAPPLE(1, &clear->ArrayObj);
   clear->ArrayObj = 0;
   _mesa_DeleteBuffersARB(1, &clear->VBO);
   clear->VBO = 0;
   _mesa_DeleteObjectARB(clear->ShaderProg);
   clear->ShaderProg = 0;

   if (clear->IntegerShaderProg) {
      _mesa_DeleteObjectARB(clear->IntegerShaderProg);
      clear->IntegerShaderProg = 0;
   }
}

/**
 * Meta implementation of ctx->Driver.Clear() in terms of polygon rendering.
 */
void
_mesa_meta_glsl_Clear(struct gl_context *ctx, GLbitfield buffers)
{
   struct clear_state *clear = &ctx->Meta->Clear;
   GLbitfield metaSave;
   const GLuint stencilMax = (1 << ctx->DrawBuffer->Visual.stencilBits) - 1;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   const float x0 = ((float)fb->_Xmin / fb->Width)  * 2.0f - 1.0f;
   const float y0 = ((float)fb->_Ymin / fb->Height) * 2.0f - 1.0f;
   const float x1 = ((float)fb->_Xmax / fb->Width)  * 2.0f - 1.0f;
   const float y1 = ((float)fb->_Ymax / fb->Height) * 2.0f - 1.0f;
   const float z = -invert_z(ctx->Depth.Clear);
   struct vertex {
      GLfloat x, y, z;
   } verts[4];

   metaSave = (MESA_META_ALPHA_TEST |
	       MESA_META_BLEND |
	       MESA_META_DEPTH_TEST |
	       MESA_META_RASTERIZATION |
	       MESA_META_SHADER |
	       MESA_META_STENCIL_TEST |
	       MESA_META_VERTEX |
	       MESA_META_VIEWPORT |
	       MESA_META_CLIP |
	       MESA_META_CLAMP_FRAGMENT_COLOR);

   if (!(buffers & BUFFER_BITS_COLOR)) {
      /* We'll use colormask to disable color writes.  Otherwise,
       * respect color mask
       */
      metaSave |= MESA_META_COLOR_MASK;
   }

   _mesa_meta_begin(ctx, metaSave);

   meta_glsl_clear_init(ctx, clear);

   if (fb->_IntegerColor) {
      _mesa_UseProgramObjectARB(clear->IntegerShaderProg);
      _mesa_Uniform4ivARB(clear->IntegerColorLocation, 1,
			  ctx->Color.ClearColor.i);
   } else {
      _mesa_UseProgramObjectARB(clear->ShaderProg);
      _mesa_Uniform4fvARB(clear->ColorLocation, 1,
			  ctx->Color.ClearColor.f);
   }

   _mesa_BindVertexArray(clear->ArrayObj);
   _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, clear->VBO);

   /* GL_COLOR_BUFFER_BIT */
   if (buffers & BUFFER_BITS_COLOR) {
      /* leave colormask, glDrawBuffer state as-is */

      /* Clears never have the color clamped. */
      _mesa_ClampColorARB(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
   }
   else {
      ASSERT(metaSave & MESA_META_COLOR_MASK);
      _mesa_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
   }

   /* GL_DEPTH_BUFFER_BIT */
   if (buffers & BUFFER_BIT_DEPTH) {
      _mesa_set_enable(ctx, GL_DEPTH_TEST, GL_TRUE);
      _mesa_DepthFunc(GL_ALWAYS);
      _mesa_DepthMask(GL_TRUE);
   }
   else {
      assert(!ctx->Depth.Test);
   }

   /* GL_STENCIL_BUFFER_BIT */
   if (buffers & BUFFER_BIT_STENCIL) {
      _mesa_set_enable(ctx, GL_STENCIL_TEST, GL_TRUE);
      _mesa_StencilOpSeparate(GL_FRONT_AND_BACK,
                              GL_REPLACE, GL_REPLACE, GL_REPLACE);
      _mesa_StencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS,
                                ctx->Stencil.Clear & stencilMax,
                                ctx->Stencil.WriteMask[0]);
   }
   else {
      assert(!ctx->Stencil.Enabled);
   }

   /* vertex positions */
   verts[0].x = x0;
   verts[0].y = y0;
   verts[0].z = z;
   verts[1].x = x1;
   verts[1].y = y0;
   verts[1].z = z;
   verts[2].x = x1;
   verts[2].y = y1;
   verts[2].z = z;
   verts[3].x = x0;
   verts[3].y = y1;
   verts[3].z = z;

   /* upload new vertex data */
   _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts), verts,
		       GL_DYNAMIC_DRAW_ARB);

   /* draw quad */
   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   _mesa_meta_end(ctx);
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
                          MESA_META_SHADER |
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
 * When the glDrawPixels() image size is greater than the max rectangle
 * texture size we use this function to break the glDrawPixels() image
 * into tiles which fit into the max texture size.
 */
static void
tiled_draw_pixels(struct gl_context *ctx,
                  GLint tileSize,
                  GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type,
                  const struct gl_pixelstore_attrib *unpack,
                  const GLvoid *pixels)
{
   struct gl_pixelstore_attrib tileUnpack = *unpack;
   GLint i, j;

   if (tileUnpack.RowLength == 0)
      tileUnpack.RowLength = width;

   for (i = 0; i < width; i += tileSize) {
      const GLint tileWidth = MIN2(tileSize, width - i);
      const GLint tileX = (GLint) (x + i * ctx->Pixel.ZoomX);

      tileUnpack.SkipPixels = unpack->SkipPixels + i;

      for (j = 0; j < height; j += tileSize) {
         const GLint tileHeight = MIN2(tileSize, height - j);
         const GLint tileY = (GLint) (y + j * ctx->Pixel.ZoomY);

         tileUnpack.SkipRows = unpack->SkipRows + j;

         _mesa_meta_DrawPixels(ctx, tileX, tileY, tileWidth, tileHeight,
                               format, type, &tileUnpack, pixels);
      }
   }
}


/**
 * One-time init for drawing stencil pixels.
 */
static void
init_draw_stencil_pixels(struct gl_context *ctx)
{
   /* This program is run eight times, once for each stencil bit.
    * The stencil values to draw are found in an 8-bit alpha texture.
    * We read the texture/stencil value and test if bit 'b' is set.
    * If the bit is not set, use KIL to kill the fragment.
    * Finally, we use the stencil test to update the stencil buffer.
    *
    * The basic algorithm for checking if a bit is set is:
    *   if (is_odd(value / (1 << bit)))
    *      result is one (or non-zero).
    *   else
    *      result is zero.
    * The program parameter contains three values:
    *   parm.x = 255 / (1 << bit)
    *   parm.y = 0.5
    *   parm.z = 0.0
    */
   static const char *program =
      "!!ARBfp1.0\n"
      "PARAM parm = program.local[0]; \n"
      "TEMP t; \n"
      "TEX t, fragment.texcoord[0], texture[0], %s; \n"   /* NOTE %s here! */
      "# t = t * 255 / bit \n"
      "MUL t.x, t.a, parm.x; \n"
      "# t = (int) t \n"
      "FRC t.y, t.x; \n"
      "SUB t.x, t.x, t.y; \n"
      "# t = t * 0.5 \n"
      "MUL t.x, t.x, parm.y; \n"
      "# t = fract(t.x) \n"
      "FRC t.x, t.x; # if t.x != 0, then the bit is set \n"
      "# t.x = (t.x == 0 ? 1 : 0) \n"
      "SGE t.x, -t.x, parm.z; \n"
      "KIL -t.x; \n"
      "# for debug only \n"
      "#MOV result.color, t.x; \n"
      "END \n";
   char program2[1000];
   struct drawpix_state *drawpix = &ctx->Meta->DrawPix;
   struct temp_texture *tex = get_temp_texture(ctx);
   const char *texTarget;

   assert(drawpix->StencilFP == 0);

   /* replace %s with "RECT" or "2D" */
   assert(strlen(program) + 4 < sizeof(program2));
   if (tex->Target == GL_TEXTURE_RECTANGLE)
      texTarget = "RECT";
   else
      texTarget = "2D";
   _mesa_snprintf(program2, sizeof(program2), program, texTarget);

   _mesa_GenPrograms(1, &drawpix->StencilFP);
   _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, drawpix->StencilFP);
   _mesa_ProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                          strlen(program2), (const GLubyte *) program2);
}


/**
 * One-time init for drawing depth pixels.
 */
static void
init_draw_depth_pixels(struct gl_context *ctx)
{
   static const char *program =
      "!!ARBfp1.0\n"
      "PARAM color = program.local[0]; \n"
      "TEX result.depth, fragment.texcoord[0], texture[0], %s; \n"
      "MOV result.color, color; \n"
      "END \n";
   char program2[200];
   struct drawpix_state *drawpix = &ctx->Meta->DrawPix;
   struct temp_texture *tex = get_temp_texture(ctx);
   const char *texTarget;

   assert(drawpix->DepthFP == 0);

   /* replace %s with "RECT" or "2D" */
   assert(strlen(program) + 4 < sizeof(program2));
   if (tex->Target == GL_TEXTURE_RECTANGLE)
      texTarget = "RECT";
   else
      texTarget = "2D";
   _mesa_snprintf(program2, sizeof(program2), program, texTarget);

   _mesa_GenPrograms(1, &drawpix->DepthFP);
   _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, drawpix->DepthFP);
   _mesa_ProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                          strlen(program2), (const GLubyte *) program2);
}


/**
 * Meta implementation of ctx->Driver.DrawPixels() in terms
 * of texture mapping and polygon rendering.
 */
void
_mesa_meta_DrawPixels(struct gl_context *ctx,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const struct gl_pixelstore_attrib *unpack,
                      const GLvoid *pixels)
{
   struct drawpix_state *drawpix = &ctx->Meta->DrawPix;
   struct temp_texture *tex = get_temp_texture(ctx);
   const struct gl_pixelstore_attrib unpackSave = ctx->Unpack;
   const GLuint origStencilMask = ctx->Stencil.WriteMask[0];
   struct vertex {
      GLfloat x, y, z, s, t;
   };
   struct vertex verts[4];
   GLenum texIntFormat;
   GLboolean fallback, newTex;
   GLbitfield metaExtraSave = 0x0;
   GLuint vbo;

   /*
    * Determine if we can do the glDrawPixels with texture mapping.
    */
   fallback = GL_FALSE;
   if (ctx->_ImageTransferState ||
       ctx->Fog.Enabled) {
      fallback = GL_TRUE;
   }

   if (_mesa_is_color_format(format)) {
      /* use more compact format when possible */
      /* XXX disable special case for GL_LUMINANCE for now to work around
       * apparent i965 driver bug (see bug #23670).
       */
      if (/*format == GL_LUMINANCE ||*/ format == GL_LUMINANCE_ALPHA)
         texIntFormat = format;
      else
         texIntFormat = GL_RGBA;

      /* If we're not supposed to clamp the resulting color, then just
       * promote our texture to fully float.  We could do better by
       * just going for the matching set of channels, in floating
       * point.
       */
      if (ctx->Color.ClampFragmentColor != GL_TRUE &&
	  ctx->Extensions.ARB_texture_float)
	 texIntFormat = GL_RGBA32F;
   }
   else if (_mesa_is_stencil_format(format)) {
      if (ctx->Extensions.ARB_fragment_program &&
          ctx->Pixel.IndexShift == 0 &&
          ctx->Pixel.IndexOffset == 0 &&
          type == GL_UNSIGNED_BYTE) {
         /* We'll store stencil as alpha.  This only works for GLubyte
          * image data because of how incoming values are mapped to alpha
          * in [0,1].
          */
         texIntFormat = GL_ALPHA;
         metaExtraSave = (MESA_META_COLOR_MASK |
                          MESA_META_DEPTH_TEST |
                          MESA_META_SHADER |
                          MESA_META_STENCIL_TEST);
      }
      else {
         fallback = GL_TRUE;
      }
   }
   else if (_mesa_is_depth_format(format)) {
      if (ctx->Extensions.ARB_depth_texture &&
          ctx->Extensions.ARB_fragment_program) {
         texIntFormat = GL_DEPTH_COMPONENT;
         metaExtraSave = (MESA_META_SHADER);
      }
      else {
         fallback = GL_TRUE;
      }
   }
   else {
      fallback = GL_TRUE;
   }

   if (fallback) {
      _swrast_DrawPixels(ctx, x, y, width, height,
                         format, type, unpack, pixels);
      return;
   }

   /*
    * Check image size against max texture size, draw as tiles if needed.
    */
   if (width > tex->MaxSize || height > tex->MaxSize) {
      tiled_draw_pixels(ctx, tex->MaxSize, x, y, width, height,
                        format, type, unpack, pixels);
      return;
   }

   /* Most GL state applies to glDrawPixels (like blending, stencil, etc),
    * but a there's a few things we need to override:
    */
   _mesa_meta_begin(ctx, (MESA_META_RASTERIZATION |
                          MESA_META_SHADER |
                          MESA_META_TEXTURE |
                          MESA_META_TRANSFORM |
                          MESA_META_CLIP |
                          MESA_META_VERTEX |
                          MESA_META_VIEWPORT |
			  MESA_META_CLAMP_FRAGMENT_COLOR |
                          metaExtraSave));

   newTex = alloc_texture(tex, width, height, texIntFormat);

   /* vertex positions, texcoords (after texture allocation!) */
   {
      const GLfloat x0 = (GLfloat) x;
      const GLfloat y0 = (GLfloat) y;
      const GLfloat x1 = x + width * ctx->Pixel.ZoomX;
      const GLfloat y1 = y + height * ctx->Pixel.ZoomY;
      const GLfloat z = invert_z(ctx->Current.RasterPos[2]);

      verts[0].x = x0;
      verts[0].y = y0;
      verts[0].z = z;
      verts[0].s = 0.0F;
      verts[0].t = 0.0F;
      verts[1].x = x1;
      verts[1].y = y0;
      verts[1].z = z;
      verts[1].s = tex->Sright;
      verts[1].t = 0.0F;
      verts[2].x = x1;
      verts[2].y = y1;
      verts[2].z = z;
      verts[2].s = tex->Sright;
      verts[2].t = tex->Ttop;
      verts[3].x = x0;
      verts[3].y = y1;
      verts[3].z = z;
      verts[3].s = 0.0F;
      verts[3].t = tex->Ttop;
   }

   if (drawpix->ArrayObj == 0) {
      /* one-time setup: create vertex array object */
      _mesa_GenVertexArrays(1, &drawpix->ArrayObj);
   }
   _mesa_BindVertexArray(drawpix->ArrayObj);

   /* create vertex array buffer */
   _mesa_GenBuffersARB(1, &vbo);
   _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
   _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                       verts, GL_DYNAMIC_DRAW_ARB);

   /* setup vertex arrays */
   _mesa_VertexPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
   _mesa_TexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(s));
   _mesa_EnableClientState(GL_VERTEX_ARRAY);
   _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);

   /* set given unpack params */
   ctx->Unpack = *unpack;

   _mesa_set_enable(ctx, tex->Target, GL_TRUE);

   if (_mesa_is_stencil_format(format)) {
      /* Drawing stencil */
      GLint bit;

      if (!drawpix->StencilFP)
         init_draw_stencil_pixels(ctx);

      setup_drawpix_texture(ctx, tex, newTex, texIntFormat, width, height,
                            GL_ALPHA, type, pixels);

      _mesa_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

      _mesa_set_enable(ctx, GL_STENCIL_TEST, GL_TRUE);

      /* set all stencil bits to 0 */
      _mesa_StencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
      _mesa_StencilFunc(GL_ALWAYS, 0, 255);
      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
  
      /* set stencil bits to 1 where needed */
      _mesa_StencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

      _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, drawpix->StencilFP);
      _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB, GL_TRUE);

      for (bit = 0; bit < ctx->DrawBuffer->Visual.stencilBits; bit++) {
         const GLuint mask = 1 << bit;
         if (mask & origStencilMask) {
            _mesa_StencilFunc(GL_ALWAYS, mask, mask);
            _mesa_StencilMask(mask);

            _mesa_ProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0,
                                             255.0 / mask, 0.5, 0.0, 0.0);

            _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
         }
      }
   }
   else if (_mesa_is_depth_format(format)) {
      /* Drawing depth */
      if (!drawpix->DepthFP)
         init_draw_depth_pixels(ctx);

      _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, drawpix->DepthFP);
      _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB, GL_TRUE);

      /* polygon color = current raster color */
      _mesa_ProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0,
                                        ctx->Current.RasterColor);

      setup_drawpix_texture(ctx, tex, newTex, texIntFormat, width, height,
                            format, type, pixels);

      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
   }
   else {
      /* Drawing RGBA */
      setup_drawpix_texture(ctx, tex, newTex, texIntFormat, width, height,
                            format, type, pixels);
      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
   }

   _mesa_set_enable(ctx, tex->Target, GL_FALSE);

   _mesa_DeleteBuffersARB(1, &vbo);

   /* restore unpack params */
   ctx->Unpack = unpackSave;

   _mesa_meta_end(ctx);
}

static GLboolean
alpha_test_raster_color(struct gl_context *ctx)
{
   GLfloat alpha = ctx->Current.RasterColor[ACOMP];
   GLfloat ref = ctx->Color.AlphaRef;

   switch (ctx->Color.AlphaFunc) {
      case GL_NEVER:
	 return GL_FALSE;
      case GL_LESS:
	 return alpha < ref;
      case GL_EQUAL:
	 return alpha == ref;
      case GL_LEQUAL:
	 return alpha <= ref;
      case GL_GREATER:
	 return alpha > ref;
      case GL_NOTEQUAL:
	 return alpha != ref;
      case GL_GEQUAL:
	 return alpha >= ref;
      case GL_ALWAYS:
	 return GL_TRUE;
      default:
	 assert(0);
	 return GL_FALSE;
   }
}

/**
 * Do glBitmap with a alpha texture quad.  Use the alpha test to cull
 * the 'off' bits.  A bitmap cache as in the gallium/mesa state
 * tracker would improve performance a lot.
 */
void
_mesa_meta_Bitmap(struct gl_context *ctx,
                  GLint x, GLint y, GLsizei width, GLsizei height,
                  const struct gl_pixelstore_attrib *unpack,
                  const GLubyte *bitmap1)
{
   struct bitmap_state *bitmap = &ctx->Meta->Bitmap;
   struct temp_texture *tex = get_bitmap_temp_texture(ctx);
   const GLenum texIntFormat = GL_ALPHA;
   const struct gl_pixelstore_attrib unpackSave = *unpack;
   GLubyte fg, bg;
   struct vertex {
      GLfloat x, y, z, s, t, r, g, b, a;
   };
   struct vertex verts[4];
   GLboolean newTex;
   GLubyte *bitmap8;

   /*
    * Check if swrast fallback is needed.
    */
   if (ctx->_ImageTransferState ||
       ctx->FragmentProgram._Enabled ||
       ctx->Fog.Enabled ||
       ctx->Texture._EnabledUnits ||
       width > tex->MaxSize ||
       height > tex->MaxSize) {
      _swrast_Bitmap(ctx, x, y, width, height, unpack, bitmap1);
      return;
   }

   if (ctx->Color.AlphaEnabled && !alpha_test_raster_color(ctx))
      return;

   /* Most GL state applies to glBitmap (like blending, stencil, etc),
    * but a there's a few things we need to override:
    */
   _mesa_meta_begin(ctx, (MESA_META_ALPHA_TEST |
                          MESA_META_PIXEL_STORE |
                          MESA_META_RASTERIZATION |
                          MESA_META_SHADER |
                          MESA_META_TEXTURE |
                          MESA_META_TRANSFORM |
                          MESA_META_CLIP |
                          MESA_META_VERTEX |
                          MESA_META_VIEWPORT));

   if (bitmap->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArraysAPPLE(1, &bitmap->ArrayObj);
      _mesa_BindVertexArrayAPPLE(bitmap->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &bitmap->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, bitmap->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                          NULL, GL_DYNAMIC_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_TexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(s));
      _mesa_ColorPointer(4, GL_FLOAT, sizeof(struct vertex), OFFSET(r));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
      _mesa_EnableClientState(GL_COLOR_ARRAY);
   }
   else {
      _mesa_BindVertexArray(bitmap->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, bitmap->VBO);
   }

   newTex = alloc_texture(tex, width, height, texIntFormat);

   /* vertex positions, texcoords, colors (after texture allocation!) */
   {
      const GLfloat x0 = (GLfloat) x;
      const GLfloat y0 = (GLfloat) y;
      const GLfloat x1 = (GLfloat) (x + width);
      const GLfloat y1 = (GLfloat) (y + height);
      const GLfloat z = invert_z(ctx->Current.RasterPos[2]);
      GLuint i;

      verts[0].x = x0;
      verts[0].y = y0;
      verts[0].z = z;
      verts[0].s = 0.0F;
      verts[0].t = 0.0F;
      verts[1].x = x1;
      verts[1].y = y0;
      verts[1].z = z;
      verts[1].s = tex->Sright;
      verts[1].t = 0.0F;
      verts[2].x = x1;
      verts[2].y = y1;
      verts[2].z = z;
      verts[2].s = tex->Sright;
      verts[2].t = tex->Ttop;
      verts[3].x = x0;
      verts[3].y = y1;
      verts[3].z = z;
      verts[3].s = 0.0F;
      verts[3].t = tex->Ttop;

      for (i = 0; i < 4; i++) {
         verts[i].r = ctx->Current.RasterColor[0];
         verts[i].g = ctx->Current.RasterColor[1];
         verts[i].b = ctx->Current.RasterColor[2];
         verts[i].a = ctx->Current.RasterColor[3];
      }

      /* upload new vertex data */
      _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
   }

   /* choose different foreground/background alpha values */
   CLAMPED_FLOAT_TO_UBYTE(fg, ctx->Current.RasterColor[ACOMP]);
   bg = (fg > 127 ? 0 : 255);

   bitmap1 = _mesa_map_pbo_source(ctx, &unpackSave, bitmap1);
   if (!bitmap1) {
      _mesa_meta_end(ctx);
      return;
   }

   bitmap8 = (GLubyte *) malloc(width * height);
   if (bitmap8) {
      memset(bitmap8, bg, width * height);
      _mesa_expand_bitmap(width, height, &unpackSave, bitmap1,
                          bitmap8, width, fg);

      _mesa_set_enable(ctx, tex->Target, GL_TRUE);

      _mesa_set_enable(ctx, GL_ALPHA_TEST, GL_TRUE);
      _mesa_AlphaFunc(GL_NOTEQUAL, UBYTE_TO_FLOAT(bg));

      setup_drawpix_texture(ctx, tex, newTex, texIntFormat, width, height,
                            GL_ALPHA, GL_UNSIGNED_BYTE, bitmap8);

      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

      _mesa_set_enable(ctx, tex->Target, GL_FALSE);

      free(bitmap8);
   }

   _mesa_unmap_pbo_source(ctx, &unpackSave);

   _mesa_meta_end(ctx);
}


/**
 * Check if the call to _mesa_meta_GenerateMipmap() will require a
 * software fallback.  The fallback path will require that the texture
 * images are mapped.
 * \return GL_TRUE if a fallback is needed, GL_FALSE otherwise
 */
GLboolean
_mesa_meta_check_generate_mipmap_fallback(struct gl_context *ctx, GLenum target,
                                          struct gl_texture_object *texObj)
{
   const GLuint fboSave = ctx->DrawBuffer->Name;
   struct gen_mipmap_state *mipmap = &ctx->Meta->Mipmap;
   struct gl_texture_image *baseImage;
   GLuint srcLevel;
   GLenum status;

   /* check for fallbacks */
   if (!ctx->Extensions.EXT_framebuffer_object ||
       target == GL_TEXTURE_3D ||
       target == GL_TEXTURE_1D_ARRAY ||
       target == GL_TEXTURE_2D_ARRAY) {
      return GL_TRUE;
   }

   srcLevel = texObj->BaseLevel;
   baseImage = _mesa_select_tex_image(ctx, texObj, target, srcLevel);
   if (!baseImage || _mesa_is_format_compressed(baseImage->TexFormat)) {
      return GL_TRUE;
   }

   if (_mesa_get_format_color_encoding(baseImage->TexFormat) == GL_SRGB &&
       !ctx->Extensions.EXT_texture_sRGB_decode) {
      /* The texture format is sRGB but we can't turn off sRGB->linear
       * texture sample conversion.  So we won't be able to generate the
       * right colors when rendering.  Need to use a fallback.
       */
      return GL_TRUE;
   }

   /*
    * Test that we can actually render in the texture's format.
    */
   if (!mipmap->FBO)
      _mesa_GenFramebuffersEXT(1, &mipmap->FBO);
   _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, mipmap->FBO);

   if (target == GL_TEXTURE_1D) {
      _mesa_FramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT,
                                    GL_COLOR_ATTACHMENT0_EXT,
                                    target, texObj->Name, srcLevel);
   }
#if 0
   /* other work is needed to enable 3D mipmap generation */
   else if (target == GL_TEXTURE_3D) {
      GLint zoffset = 0;
      _mesa_FramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT,
                                    GL_COLOR_ATTACHMENT0_EXT,
                                    target, texObj->Name, srcLevel, zoffset);
   }
#endif
   else {
      /* 2D / cube */
      _mesa_FramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                    GL_COLOR_ATTACHMENT0_EXT,
                                    target, texObj->Name, srcLevel);
   }

   status = _mesa_CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

   _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboSave);

   if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      return GL_TRUE;
   }

   return GL_FALSE;
}


/**
 * Compute the texture coordinates for the four vertices of a quad for
 * drawing a 2D texture image or slice of a cube/3D texture.
 * \param faceTarget  GL_TEXTURE_1D/2D/3D or cube face name
 * \param slice  slice of a 1D/2D array texture or 3D texture
 * \param width  width of the texture image
 * \param height  height of the texture image
 * \param coords0/1/2/3  returns the computed texcoords
 */
static void
setup_texture_coords(GLenum faceTarget,
                     GLint slice,
                     GLint width,
                     GLint height,
                     GLfloat coords0[3],
                     GLfloat coords1[3],
                     GLfloat coords2[3],
                     GLfloat coords3[3])
{
   static const GLfloat st[4][2] = {
      {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
   };
   GLuint i;
   GLfloat r;

   switch (faceTarget) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_2D_ARRAY:
      if (faceTarget == GL_TEXTURE_3D)
         r = 1.0F / slice;
      else if (faceTarget == GL_TEXTURE_2D_ARRAY)
         r = slice;
      else
         r = 0.0F;
      coords0[0] = 0.0F; /* s */
      coords0[1] = 0.0F; /* t */
      coords0[2] = r; /* r */
      coords1[0] = 1.0F;
      coords1[1] = 0.0F;
      coords1[2] = r;
      coords2[0] = 1.0F;
      coords2[1] = 1.0F;
      coords2[2] = r;
      coords3[0] = 0.0F;
      coords3[1] = 1.0F;
      coords3[2] = r;
      break;
   case GL_TEXTURE_RECTANGLE_ARB:
      coords0[0] = 0.0F; /* s */
      coords0[1] = 0.0F; /* t */
      coords0[2] = 0.0F; /* r */
      coords1[0] = width;
      coords1[1] = 0.0F;
      coords1[2] = 0.0F;
      coords2[0] = width;
      coords2[1] = height;
      coords2[2] = 0.0F;
      coords3[0] = 0.0F;
      coords3[1] = height;
      coords3[2] = 0.0F;
      break;
   case GL_TEXTURE_1D_ARRAY:
      coords0[0] = 0.0F; /* s */
      coords0[1] = slice; /* t */
      coords0[2] = 0.0F; /* r */
      coords1[0] = 1.0f;
      coords1[1] = slice;
      coords1[2] = 0.0F;
      coords2[0] = 1.0F;
      coords2[1] = slice;
      coords2[2] = 0.0F;
      coords3[0] = 0.0F;
      coords3[1] = slice;
      coords3[2] = 0.0F;
      break;

   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      /* loop over quad verts */
      for (i = 0; i < 4; i++) {
         /* Compute sc = +/-scale and tc = +/-scale.
          * Not +/-1 to avoid cube face selection ambiguity near the edges,
          * though that can still sometimes happen with this scale factor...
          */
         const GLfloat scale = 0.9999f;
         const GLfloat sc = (2.0f * st[i][0] - 1.0f) * scale;
         const GLfloat tc = (2.0f * st[i][1] - 1.0f) * scale;
         GLfloat *coord;

         switch (i) {
         case 0:
            coord = coords0;
            break;
         case 1:
            coord = coords1;
            break;
         case 2:
            coord = coords2;
            break;
         case 3:
            coord = coords3;
            break;
         default:
            assert(0);
         }

         switch (faceTarget) {
         case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            coord[0] = 1.0f;
            coord[1] = -tc;
            coord[2] = -sc;
            break;
         case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            coord[0] = -1.0f;
            coord[1] = -tc;
            coord[2] = sc;
            break;
         case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            coord[0] = sc;
            coord[1] = 1.0f;
            coord[2] = tc;
            break;
         case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            coord[0] = sc;
            coord[1] = -1.0f;
            coord[2] = -tc;
            break;
         case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            coord[0] = sc;
            coord[1] = -tc;
            coord[2] = 1.0f;
            break;
         case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            coord[0] = -sc;
            coord[1] = -tc;
            coord[2] = -1.0f;
            break;
         default:
            assert(0);
         }
      }
      break;
   default:
      assert(0 && "unexpected target in meta setup_texture_coords()");
   }
}


/**
 * Called via ctx->Driver.GenerateMipmap()
 * Note: We don't yet support 3D textures, 1D/2D array textures or texture
 * borders.
 */
void
_mesa_meta_GenerateMipmap(struct gl_context *ctx, GLenum target,
                          struct gl_texture_object *texObj)
{
   struct gen_mipmap_state *mipmap = &ctx->Meta->Mipmap;
   struct vertex {
      GLfloat x, y, tex[3];
   };
   struct vertex verts[4];
   const GLuint baseLevel = texObj->BaseLevel;
   const GLuint maxLevel = texObj->MaxLevel;
   const GLenum minFilterSave = texObj->Sampler.MinFilter;
   const GLenum magFilterSave = texObj->Sampler.MagFilter;
   const GLint maxLevelSave = texObj->MaxLevel;
   const GLboolean genMipmapSave = texObj->GenerateMipmap;
   const GLenum wrapSSave = texObj->Sampler.WrapS;
   const GLenum wrapTSave = texObj->Sampler.WrapT;
   const GLenum wrapRSave = texObj->Sampler.WrapR;
   const GLenum srgbDecodeSave = texObj->Sampler.sRGBDecode;
   const GLenum srgbBufferSave = ctx->Color.sRGBEnabled;
   const GLuint fboSave = ctx->DrawBuffer->Name;
   const GLuint original_active_unit = ctx->Texture.CurrentUnit;
   GLenum faceTarget;
   GLuint dstLevel;
   const GLuint border = 0;
   const GLint slice = 0;

   if (_mesa_meta_check_generate_mipmap_fallback(ctx, target, texObj)) {
      _mesa_generate_mipmap(ctx, target, texObj);
      return;
   }

   if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
       target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
      faceTarget = target;
      target = GL_TEXTURE_CUBE_MAP;
   }
   else {
      faceTarget = target;
   }

   _mesa_meta_begin(ctx, MESA_META_ALL);

   if (original_active_unit != 0)
      _mesa_BindTexture(target, texObj->Name);

   if (mipmap->ArrayObj == 0) {
      /* one-time setup */

      /* create vertex array object */
      _mesa_GenVertexArraysAPPLE(1, &mipmap->ArrayObj);
      _mesa_BindVertexArrayAPPLE(mipmap->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &mipmap->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, mipmap->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                          NULL, GL_DYNAMIC_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_TexCoordPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(tex));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
   }
   else {
      _mesa_BindVertexArray(mipmap->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, mipmap->VBO);
   }

   if (!mipmap->FBO) {
      _mesa_GenFramebuffersEXT(1, &mipmap->FBO);
   }
   _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, mipmap->FBO);

   _mesa_TexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   _mesa_TexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   _mesa_TexParameteri(target, GL_GENERATE_MIPMAP, GL_FALSE);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

   /* We don't want to encode or decode sRGB values; treat them as linear */
   if (ctx->Extensions.EXT_texture_sRGB_decode) {
      _mesa_TexParameteri(target, GL_TEXTURE_SRGB_DECODE_EXT,
                          GL_SKIP_DECODE_EXT);
   }
   if (ctx->Extensions.EXT_framebuffer_sRGB) {
      _mesa_set_enable(ctx, GL_FRAMEBUFFER_SRGB_EXT, GL_FALSE);
   }

   _mesa_set_enable(ctx, target, GL_TRUE);

   /* setup texcoords (XXX what about border?) */
   setup_texture_coords(faceTarget,
                        slice,
                        0, 0, /* width, height never used here */
                        verts[0].tex,
                        verts[1].tex,
                        verts[2].tex,
                        verts[3].tex);

   /* setup vertex positions */
   verts[0].x = 0.0F;
   verts[0].y = 0.0F;
   verts[1].x = 1.0F;
   verts[1].y = 0.0F;
   verts[2].x = 1.0F;
   verts[2].y = 1.0F;
   verts[3].x = 0.0F;
   verts[3].y = 1.0F;
      
   /* upload new vertex data */
   _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);

   /* setup projection matrix */
   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_LoadIdentity();
   _mesa_Ortho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

   /* texture is already locked, unlock now */
   _mesa_unlock_texture(ctx, texObj);

   for (dstLevel = baseLevel + 1; dstLevel <= maxLevel; dstLevel++) {
      const struct gl_texture_image *srcImage;
      const GLuint srcLevel = dstLevel - 1;
      GLsizei srcWidth, srcHeight, srcDepth;
      GLsizei dstWidth, dstHeight, dstDepth;
      GLenum status;

      srcImage = _mesa_select_tex_image(ctx, texObj, faceTarget, srcLevel);
      assert(srcImage->Border == 0); /* XXX we can fix this */

      /* src size w/out border */
      srcWidth = srcImage->Width - 2 * border;
      srcHeight = srcImage->Height - 2 * border;
      srcDepth = srcImage->Depth - 2 * border;

      /* new dst size w/ border */
      dstWidth = MAX2(1, srcWidth / 2) + 2 * border;
      dstHeight = MAX2(1, srcHeight / 2) + 2 * border;
      dstDepth = MAX2(1, srcDepth / 2) + 2 * border;

      if (dstWidth == srcImage->Width &&
          dstHeight == srcImage->Height &&
          dstDepth == srcImage->Depth) {
         /* all done */
         break;
      }

      /* Allocate storage for the destination mipmap image(s) */

      /* Set MaxLevel large enough to hold the new level when we allocate it */
      _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, dstLevel);

      if (!_mesa_prepare_mipmap_level(ctx, texObj, dstLevel,
                                      dstWidth, dstHeight, dstDepth,
                                      srcImage->Border,
                                      srcImage->InternalFormat,
                                      srcImage->TexFormat)) {
         /* All done.  We either ran out of memory or we would go beyond the
          * last valid level of an immutable texture if we continued.
          */
         break;
      }

      /* limit minification to src level */
      _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, srcLevel);

      /* Set to draw into the current dstLevel */
      if (target == GL_TEXTURE_1D) {
         _mesa_FramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT,
                                       GL_COLOR_ATTACHMENT0_EXT,
                                       target,
                                       texObj->Name,
                                       dstLevel);
      }
      else if (target == GL_TEXTURE_3D) {
         GLint zoffset = 0; /* XXX unfinished */
         _mesa_FramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT,
                                       GL_COLOR_ATTACHMENT0_EXT,
                                       target,
                                       texObj->Name,
                                       dstLevel, zoffset);
      }
      else {
         /* 2D / cube */
         _mesa_FramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                       GL_COLOR_ATTACHMENT0_EXT,
                                       faceTarget,
                                       texObj->Name,
                                       dstLevel);
      }

      _mesa_DrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

      /* sanity check */
      status = _mesa_CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
      if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         abort();
         break;
      }

      assert(dstWidth == ctx->DrawBuffer->Width);
      assert(dstHeight == ctx->DrawBuffer->Height);

      /* setup viewport */
      _mesa_set_viewport(ctx, 0, 0, dstWidth, dstHeight);

      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
   }

   if (ctx->Extensions.EXT_texture_sRGB_decode) {
      _mesa_TexParameteri(target, GL_TEXTURE_SRGB_DECODE_EXT,
                          srgbDecodeSave);
   }
   if (ctx->Extensions.EXT_framebuffer_sRGB && srgbBufferSave) {
      _mesa_set_enable(ctx, GL_FRAMEBUFFER_SRGB_EXT, GL_TRUE);
   }

   _mesa_lock_texture(ctx, texObj); /* relock */

   _mesa_meta_end(ctx);

   _mesa_TexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilterSave);
   _mesa_TexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilterSave);
   _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxLevelSave);
   _mesa_TexParameteri(target, GL_GENERATE_MIPMAP, genMipmapSave);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_S, wrapSSave);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_T, wrapTSave);
   _mesa_TexParameteri(target, GL_TEXTURE_WRAP_R, wrapRSave);

   _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboSave);
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
   case GL_DEPTH_STENCIL:
      return GL_UNSIGNED_INT_24_8;
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


/**
 * Decompress a texture image by drawing a quad with the compressed
 * texture and reading the pixels out of the color buffer.
 * \param slice  which slice of a 3D texture or layer of a 1D/2D texture
 * \param destFormat  format, ala glReadPixels
 * \param destType  type, ala glReadPixels
 * \param dest  destination buffer
 * \param destRowLength  dest image rowLength (ala GL_PACK_ROW_LENGTH)
 */
static void
decompress_texture_image(struct gl_context *ctx,
                         struct gl_texture_image *texImage,
                         GLuint slice,
                         GLenum destFormat, GLenum destType,
                         GLvoid *dest)
{
   struct decompress_state *decompress = &ctx->Meta->Decompress;
   struct gl_texture_object *texObj = texImage->TexObject;
   const GLint width = texImage->Width;
   const GLint height = texImage->Height;
   const GLenum target = texObj->Target;
   GLenum faceTarget;
   struct vertex {
      GLfloat x, y, tex[3];
   };
   struct vertex verts[4];
   GLuint fboDrawSave, fboReadSave;
   GLuint rbSave;

   if (slice > 0) {
      assert(target == GL_TEXTURE_3D ||
             target == GL_TEXTURE_2D_ARRAY);
   }

   if (target == GL_TEXTURE_CUBE_MAP) {
      faceTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + texImage->Face;
   }
   else {
      faceTarget = target;
   }

   /* save fbo bindings (not saved by _mesa_meta_begin()) */
   fboDrawSave = ctx->DrawBuffer->Name;
   fboReadSave = ctx->ReadBuffer->Name;
   rbSave = ctx->CurrentRenderbuffer ? ctx->CurrentRenderbuffer->Name : 0;

   _mesa_meta_begin(ctx, MESA_META_ALL & ~MESA_META_PIXEL_STORE);

   /* Create/bind FBO/renderbuffer */
   if (decompress->FBO == 0) {
      _mesa_GenFramebuffersEXT(1, &decompress->FBO);
      _mesa_GenRenderbuffersEXT(1, &decompress->RBO);
      _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, decompress->FBO);
      _mesa_BindRenderbufferEXT(GL_RENDERBUFFER_EXT, decompress->RBO);
      _mesa_FramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                       GL_COLOR_ATTACHMENT0_EXT,
                                       GL_RENDERBUFFER_EXT,
                                       decompress->RBO);
   }
   else {
      _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, decompress->FBO);
   }

   /* alloc dest surface */
   if (width > decompress->Width || height > decompress->Height) {
      _mesa_BindRenderbufferEXT(GL_RENDERBUFFER_EXT, decompress->RBO);
      _mesa_RenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA,
                                   width, height);
      decompress->Width = width;
      decompress->Height = height;
   }

   /* setup VBO data */
   if (decompress->ArrayObj == 0) {
      /* create vertex array object */
      _mesa_GenVertexArrays(1, &decompress->ArrayObj);
      _mesa_BindVertexArray(decompress->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &decompress->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, decompress->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                          NULL, GL_DYNAMIC_DRAW_ARB);

      /* setup vertex arrays */
      _mesa_VertexPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_TexCoordPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(tex));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
   }
   else {
      _mesa_BindVertexArray(decompress->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, decompress->VBO);
   }

   setup_texture_coords(faceTarget, slice, width, height,
                        verts[0].tex,
                        verts[1].tex,
                        verts[2].tex,
                        verts[3].tex);

   /* setup vertex positions */
   verts[0].x = 0.0F;
   verts[0].y = 0.0F;
   verts[1].x = width;
   verts[1].y = 0.0F;
   verts[2].x = width;
   verts[2].y = height;
   verts[3].x = 0.0F;
   verts[3].y = height;

   /* upload new vertex data */
   _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);

   /* setup texture state */
   _mesa_BindTexture(target, texObj->Name);
   _mesa_set_enable(ctx, target, GL_TRUE);

   {
      /* save texture object state */
      const GLenum minFilterSave = texObj->Sampler.MinFilter;
      const GLenum magFilterSave = texObj->Sampler.MagFilter;
      const GLint baseLevelSave = texObj->BaseLevel;
      const GLint maxLevelSave = texObj->MaxLevel;
      const GLenum wrapSSave = texObj->Sampler.WrapS;
      const GLenum wrapTSave = texObj->Sampler.WrapT;
      const GLenum srgbSave = texObj->Sampler.sRGBDecode;

      /* restrict sampling to the texture level of interest */
      _mesa_TexParameteri(target, GL_TEXTURE_BASE_LEVEL, texImage->Level);
      _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, texImage->Level);
      /* nearest filtering */
      _mesa_TexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      _mesa_TexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      /* No sRGB decode or encode.*/
      if (ctx->Extensions.EXT_texture_sRGB_decode) {
         _mesa_TexParameteri(target, GL_TEXTURE_SRGB_DECODE_EXT,
                             GL_SKIP_DECODE_EXT);
      }
      if (ctx->Extensions.EXT_framebuffer_sRGB) {
         _mesa_set_enable(ctx, GL_FRAMEBUFFER_SRGB_EXT, GL_FALSE);
      }

      /* render quad w/ texture into renderbuffer */
      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
      
      /* Restore texture object state, the texture binding will
       * be restored by _mesa_meta_end().
       */
      _mesa_TexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilterSave);
      _mesa_TexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilterSave);
      if (target != GL_TEXTURE_RECTANGLE_ARB) {
         _mesa_TexParameteri(target, GL_TEXTURE_BASE_LEVEL, baseLevelSave);
         _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxLevelSave);
      }
      _mesa_TexParameteri(target, GL_TEXTURE_WRAP_S, wrapSSave);
      _mesa_TexParameteri(target, GL_TEXTURE_WRAP_T, wrapTSave);
      if (ctx->Extensions.EXT_texture_sRGB_decode) {
         _mesa_TexParameteri(target, GL_TEXTURE_SRGB_DECODE_EXT, srgbSave);
      }
   }

   /* read pixels from renderbuffer */
   {
      GLenum baseTexFormat = texImage->_BaseFormat;

      /* The pixel transfer state will be set to default values at this point
       * (see MESA_META_PIXEL_TRANSFER) so pixel transfer ops are effectively
       * turned off (as required by glGetTexImage) but we need to handle some
       * special cases.  In particular, single-channel texture values are
       * returned as red and two-channel texture values are returned as
       * red/alpha.
       */
      if (baseTexFormat == GL_LUMINANCE ||
          baseTexFormat == GL_LUMINANCE_ALPHA ||
          baseTexFormat == GL_INTENSITY) {
         /* Green and blue must be zero */
         _mesa_PixelTransferf(GL_GREEN_SCALE, 0.0f);
         _mesa_PixelTransferf(GL_BLUE_SCALE, 0.0f);
      }

      _mesa_ReadPixels(0, 0, width, height, destFormat, destType, dest);
   }

   /* disable texture unit */
   _mesa_set_enable(ctx, target, GL_FALSE);

   _mesa_meta_end(ctx);

   /* restore fbo bindings */
   if (fboDrawSave == fboReadSave) {
      _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboDrawSave);
   }
   else {
      _mesa_BindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, fboDrawSave);
      _mesa_BindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, fboReadSave);
   }
   _mesa_BindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbSave);
}


/**
 * This is just a wrapper around _mesa_get_tex_image() and
 * decompress_texture_image().  Meta functions should not be directly called
 * from core Mesa.
 */
void
_mesa_meta_GetTexImage(struct gl_context *ctx,
                       GLenum format, GLenum type, GLvoid *pixels,
                       struct gl_texture_image *texImage)
{
   /* We can only use the decompress-with-blit method here if the texels are
    * unsigned, normalized values.  We could handle signed and unnormalized 
    * with floating point renderbuffers...
    */
   if (_mesa_is_format_compressed(texImage->TexFormat) &&
       _mesa_get_format_datatype(texImage->TexFormat)
       == GL_UNSIGNED_NORMALIZED) {
      struct gl_texture_object *texObj = texImage->TexObject;
      const GLuint slice = 0; /* only 2D compressed textures for now */
      /* Need to unlock the texture here to prevent deadlock... */
      _mesa_unlock_texture(ctx, texObj);
      decompress_texture_image(ctx, texImage, slice, format, type, pixels);
      /* ... and relock it */
      _mesa_lock_texture(ctx, texObj);
   }
   else {
      _mesa_get_teximage(ctx, format, type, pixels, texImage);
   }
}


/**
 * Meta implementation of ctx->Driver.DrawTex() in terms
 * of polygon rendering.
 */
void
_mesa_meta_DrawTex(struct gl_context *ctx, GLfloat x, GLfloat y, GLfloat z,
                   GLfloat width, GLfloat height)
{
#if FEATURE_OES_draw_texture
   struct drawtex_state *drawtex = &ctx->Meta->DrawTex;
   struct vertex {
      GLfloat x, y, z, st[MAX_TEXTURE_UNITS][2];
   };
   struct vertex verts[4];
   GLuint i;

   _mesa_meta_begin(ctx, (MESA_META_RASTERIZATION |
                          MESA_META_SHADER |
                          MESA_META_TRANSFORM |
                          MESA_META_VERTEX |
                          MESA_META_VIEWPORT));

   if (drawtex->ArrayObj == 0) {
      /* one-time setup */
      GLint active_texture;

      /* create vertex array object */
      _mesa_GenVertexArrays(1, &drawtex->ArrayObj);
      _mesa_BindVertexArray(drawtex->ArrayObj);

      /* create vertex array buffer */
      _mesa_GenBuffersARB(1, &drawtex->VBO);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, drawtex->VBO);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                          NULL, GL_DYNAMIC_DRAW_ARB);

      /* client active texture is not part of the array object */
      active_texture = ctx->Array.ActiveTexture;

      /* setup vertex arrays */
      _mesa_VertexPointer(3, GL_FLOAT, sizeof(struct vertex), OFFSET(x));
      _mesa_EnableClientState(GL_VERTEX_ARRAY);
      for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
         _mesa_ClientActiveTextureARB(GL_TEXTURE0 + i);
         _mesa_TexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), OFFSET(st[i]));
         _mesa_EnableClientState(GL_TEXTURE_COORD_ARRAY);
      }

      /* restore client active texture */
      _mesa_ClientActiveTextureARB(GL_TEXTURE0 + active_texture);
   }
   else {
      _mesa_BindVertexArray(drawtex->ArrayObj);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, drawtex->VBO);
   }

   /* vertex positions, texcoords */
   {
      const GLfloat x1 = x + width;
      const GLfloat y1 = y + height;

      z = CLAMP(z, 0.0, 1.0);
      z = invert_z(z);

      verts[0].x = x;
      verts[0].y = y;
      verts[0].z = z;

      verts[1].x = x1;
      verts[1].y = y;
      verts[1].z = z;

      verts[2].x = x1;
      verts[2].y = y1;
      verts[2].z = z;

      verts[3].x = x;
      verts[3].y = y1;
      verts[3].z = z;

      for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
         const struct gl_texture_object *texObj;
         const struct gl_texture_image *texImage;
         GLfloat s, t, s1, t1;
         GLuint tw, th;

         if (!ctx->Texture.Unit[i]._ReallyEnabled) {
            GLuint j;
            for (j = 0; j < 4; j++) {
               verts[j].st[i][0] = 0.0f;
               verts[j].st[i][1] = 0.0f;
            }
            continue;
         }

         texObj = ctx->Texture.Unit[i]._Current;
         texImage = texObj->Image[0][texObj->BaseLevel];
         tw = texImage->Width2;
         th = texImage->Height2;

         s = (GLfloat) texObj->CropRect[0] / tw;
         t = (GLfloat) texObj->CropRect[1] / th;
         s1 = (GLfloat) (texObj->CropRect[0] + texObj->CropRect[2]) / tw;
         t1 = (GLfloat) (texObj->CropRect[1] + texObj->CropRect[3]) / th;

         verts[0].st[i][0] = s;
         verts[0].st[i][1] = t;

         verts[1].st[i][0] = s1;
         verts[1].st[i][1] = t;

         verts[2].st[i][0] = s1;
         verts[2].st[i][1] = t1;

         verts[3].st[i][0] = s;
         verts[3].st[i][1] = t1;
      }

      _mesa_BufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
   }

   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   _mesa_meta_end(ctx);
#endif /* FEATURE_OES_draw_texture */
}
