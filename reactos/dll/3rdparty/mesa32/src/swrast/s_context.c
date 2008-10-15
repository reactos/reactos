/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 *    Brian Paul
 */

#include "imports.h"
#include "bufferobj.h"
#include "context.h"
#include "colormac.h"
#include "mtypes.h"
#include "teximage.h"
#include "swrast.h"
#include "shader/prog_parameter.h"
#include "shader/prog_statevars.h"
#include "s_blend.h"
#include "s_context.h"
#include "s_lines.h"
#include "s_points.h"
#include "s_span.h"
#include "s_triangle.h"
#include "s_texfilter.h"


/**
 * Recompute the value of swrast->_RasterMask, etc. according to
 * the current context.  The _RasterMask field can be easily tested by
 * drivers to determine certain basic GL state (does the primitive need
 * stenciling, logic-op, fog, etc?).
 */
static void
_swrast_update_rasterflags( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLbitfield rasterMask = 0;

   if (ctx->Color.AlphaEnabled)           rasterMask |= ALPHATEST_BIT;
   if (ctx->Color.BlendEnabled)           rasterMask |= BLEND_BIT;
   if (ctx->Depth.Test)                   rasterMask |= DEPTH_BIT;
   if (swrast->_FogEnabled)               rasterMask |= FOG_BIT;
   if (ctx->Scissor.Enabled)              rasterMask |= CLIP_BIT;
   if (ctx->Stencil.Enabled)              rasterMask |= STENCIL_BIT;
   if (ctx->Visual.rgbMode) {
      const GLuint colorMask = *((GLuint *) &ctx->Color.ColorMask);
      if (colorMask != 0xffffffff)        rasterMask |= MASKING_BIT;
      if (ctx->Color._LogicOpEnabled)     rasterMask |= LOGIC_OP_BIT;
      if (ctx->Texture._EnabledUnits)     rasterMask |= TEXTURE_BIT;
   }
   else {
      if (ctx->Color.IndexMask != 0xffffffff) rasterMask |= MASKING_BIT;
      if (ctx->Color.IndexLogicOpEnabled)     rasterMask |= LOGIC_OP_BIT;
   }

   if (   ctx->Viewport.X < 0
       || ctx->Viewport.X + ctx->Viewport.Width > (GLint) ctx->DrawBuffer->Width
       || ctx->Viewport.Y < 0
       || ctx->Viewport.Y + ctx->Viewport.Height > (GLint) ctx->DrawBuffer->Height) {
      rasterMask |= CLIP_BIT;
   }

   if (ctx->Query.CurrentOcclusionObject)
      rasterMask |= OCCLUSION_BIT;


   /* If we're not drawing to exactly one color buffer set the
    * MULTI_DRAW_BIT flag.  Also set it if we're drawing to no
    * buffers or the RGBA or CI mask disables all writes.
    */
   if (ctx->DrawBuffer->_NumColorDrawBuffers != 1) {
      /* more than one color buffer designated for writing (or zero buffers) */
      rasterMask |= MULTI_DRAW_BIT;
   }
   else if (ctx->Visual.rgbMode && *((GLuint *) ctx->Color.ColorMask) == 0) {
      rasterMask |= MULTI_DRAW_BIT; /* all RGBA channels disabled */
   }
   else if (!ctx->Visual.rgbMode && ctx->Color.IndexMask==0) {
      rasterMask |= MULTI_DRAW_BIT; /* all color index bits disabled */
   }

   if (ctx->FragmentProgram._Current) {
      rasterMask |= FRAGPROG_BIT;
   }

   if (ctx->ATIFragmentShader._Enabled) {
      rasterMask |= ATIFRAGSHADER_BIT;
   }

#if CHAN_TYPE == GL_FLOAT
   if (ctx->Color.ClampFragmentColor == GL_TRUE) {
      rasterMask |= CLAMPING_BIT;
   }
#endif

   SWRAST_CONTEXT(ctx)->_RasterMask = rasterMask;
}


/**
 * Examine polygon cull state to compute the _BackfaceCullSign field.
 * _BackfaceCullSign will be 0 if no culling, -1 if culling back-faces,
 * and 1 if culling front-faces.  The Polygon FrontFace state also
 * factors in.
 */
static void
_swrast_update_polygon( GLcontext *ctx )
{
   GLfloat backface_sign;

   if (ctx->Polygon.CullFlag) {
      switch (ctx->Polygon.CullFaceMode) {
      case GL_BACK:
         backface_sign = -1.0;
	 break;
      case GL_FRONT:
         backface_sign = 1.0;
	 break;
      case GL_FRONT_AND_BACK:
         /* fallthrough */
      default:
	 backface_sign = 0.0;
      }
   }
   else {
      backface_sign = 0.0;
   }

   SWRAST_CONTEXT(ctx)->_BackfaceCullSign = backface_sign;

   /* This is for front/back-face determination, but not for culling */
   SWRAST_CONTEXT(ctx)->_BackfaceSign
      = (ctx->Polygon.FrontFace == GL_CW) ? -1.0 : 1.0;
}



/**
 * Update the _PreferPixelFog field to indicate if we need to compute
 * fog blend factors (from the fog coords) per-fragment.
 */
static void
_swrast_update_fog_hint( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   swrast->_PreferPixelFog = (!swrast->AllowVertexFog ||
                              ctx->FragmentProgram._Current ||
			      (ctx->Hint.Fog == GL_NICEST &&
			       swrast->AllowPixelFog));
}



/**
 * Update the swrast->_AnyTextureCombine flag.
 */
static void
_swrast_update_texture_env( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint i;
   swrast->_AnyTextureCombine = GL_FALSE;
   for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
      if (ctx->Texture.Unit[i].EnvMode == GL_COMBINE_EXT ||
          ctx->Texture.Unit[i].EnvMode == GL_COMBINE4_NV) {
         swrast->_AnyTextureCombine = GL_TRUE;
         return;
      }
   }
}


/**
 * Determine if we can defer texturing/shading until after Z/stencil
 * testing.  This potentially allows us to skip texturing/shading for
 * lots of fragments.
 */
static void
_swrast_update_deferred_texture(GLcontext *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   if (ctx->Color.AlphaEnabled) {
      /* alpha test depends on post-texture/shader colors */
      swrast->_DeferredTexture = GL_FALSE;
   }
   else {
      const struct gl_fragment_program *fprog
         = ctx->FragmentProgram._Current;
      if (fprog && (fprog->Base.OutputsWritten & (1 << FRAG_RESULT_DEPR))) {
         /* Z comes from fragment program/shader */
         swrast->_DeferredTexture = GL_FALSE;
      }
      else if (ctx->Query.CurrentOcclusionObject) {
         /* occlusion query depends on shader discard/kill results */
         swrast->_DeferredTexture = GL_FALSE;
      }
      else {
         swrast->_DeferredTexture = GL_TRUE;
      }
   }
}


/**
 * Update swrast->_FogColor and swrast->_FogEnable values.
 */
static void
_swrast_update_fog_state( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const struct gl_fragment_program *fp = ctx->FragmentProgram._Current;

   /* determine if fog is needed, and if so, which fog mode */
   swrast->_FogEnabled = GL_FALSE;
   if (fp && fp->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
      if (fp->FogOption != GL_NONE) {
         swrast->_FogEnabled = GL_TRUE;
         swrast->_FogMode = fp->FogOption;
      }
   }
   else if (ctx->Fog.Enabled) {
      swrast->_FogEnabled = GL_TRUE;
      swrast->_FogMode = ctx->Fog.Mode;
   }
}


/**
 * Update state for running fragment programs.  Basically, load the
 * program parameters with current state values.
 */
static void
_swrast_update_fragment_program(GLcontext *ctx, GLbitfield newState)
{
   const struct gl_fragment_program *fp = ctx->FragmentProgram._Current;
   if (fp) {
#if 0
      /* XXX Need a way to trigger the initial loading of parameters
       * even when there's no recent state changes.
       */
      if (fp->Base.Parameters->StateFlags & newState)
#endif
         _mesa_load_state_parameters(ctx, fp->Base.Parameters);
   }
}



#define _SWRAST_NEW_DERIVED (_SWRAST_NEW_RASTERMASK |	\
			     _NEW_TEXTURE |		\
			     _NEW_HINT |		\
			     _NEW_POLYGON )

/* State referenced by _swrast_choose_triangle, _swrast_choose_line.
 */
#define _SWRAST_NEW_TRIANGLE (_SWRAST_NEW_DERIVED |		\
			      _NEW_RENDERMODE|			\
                              _NEW_POLYGON|			\
                              _NEW_DEPTH|			\
                              _NEW_STENCIL|			\
                              _NEW_COLOR|			\
                              _NEW_TEXTURE|			\
                              _SWRAST_NEW_RASTERMASK|		\
                              _NEW_LIGHT|			\
                              _NEW_FOG |			\
			      _DD_NEW_SEPARATE_SPECULAR)

#define _SWRAST_NEW_LINE (_SWRAST_NEW_DERIVED |		\
			  _NEW_RENDERMODE|		\
                          _NEW_LINE|			\
                          _NEW_TEXTURE|			\
                          _NEW_LIGHT|			\
                          _NEW_FOG|			\
                          _NEW_DEPTH |			\
                          _DD_NEW_SEPARATE_SPECULAR)

#define _SWRAST_NEW_POINT (_SWRAST_NEW_DERIVED |	\
			   _NEW_RENDERMODE |		\
			   _NEW_POINT |			\
			   _NEW_TEXTURE |		\
			   _NEW_LIGHT |			\
			   _NEW_FOG |			\
                           _DD_NEW_SEPARATE_SPECULAR)

#define _SWRAST_NEW_TEXTURE_SAMPLE_FUNC _NEW_TEXTURE

#define _SWRAST_NEW_TEXTURE_ENV_MODE _NEW_TEXTURE

#define _SWRAST_NEW_BLEND_FUNC _NEW_COLOR



/**
 * Stub for swrast->Triangle to select a true triangle function
 * after a state change.
 */
static void
_swrast_validate_triangle( GLcontext *ctx,
			   const SWvertex *v0,
                           const SWvertex *v1,
                           const SWvertex *v2 )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   _swrast_validate_derived( ctx );
   swrast->choose_triangle( ctx );
   ASSERT(swrast->Triangle);

   if (ctx->Texture._EnabledUnits == 0
       && NEED_SECONDARY_COLOR(ctx)
       && !ctx->FragmentProgram._Current) {
      /* separate specular color, but no texture */
      swrast->SpecTriangle = swrast->Triangle;
      swrast->Triangle = _swrast_add_spec_terms_triangle;
   }

   swrast->Triangle( ctx, v0, v1, v2 );
}

/**
 * Called via swrast->Line.  Examine current GL state and choose a software
 * line routine.  Then call it.
 */
static void
_swrast_validate_line( GLcontext *ctx, const SWvertex *v0, const SWvertex *v1 )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   _swrast_validate_derived( ctx );
   swrast->choose_line( ctx );
   ASSERT(swrast->Line);

   if (ctx->Texture._EnabledUnits == 0
       && NEED_SECONDARY_COLOR(ctx)
       && !ctx->FragmentProgram._Current) {
      swrast->SpecLine = swrast->Line;
      swrast->Line = _swrast_add_spec_terms_line;
   }

   swrast->Line( ctx, v0, v1 );
}

/**
 * Called via swrast->Point.  Examine current GL state and choose a software
 * point routine.  Then call it.
 */
static void
_swrast_validate_point( GLcontext *ctx, const SWvertex *v0 )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   _swrast_validate_derived( ctx );
   swrast->choose_point( ctx );

   if (ctx->Texture._EnabledUnits == 0
       && NEED_SECONDARY_COLOR(ctx)
       && !ctx->FragmentProgram._Current) {
      swrast->SpecPoint = swrast->Point;
      swrast->Point = _swrast_add_spec_terms_point;
   }

   swrast->Point( ctx, v0 );
}


/**
 * Called via swrast->BlendFunc.  Examine GL state to choose a blending
 * function, then call it.
 */
static void _ASMAPI
_swrast_validate_blend_func(GLcontext *ctx, GLuint n, const GLubyte mask[],
                            GLvoid *src, const GLvoid *dst,
                            GLenum chanType )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   _swrast_validate_derived( ctx ); /* why is this needed? */
   _swrast_choose_blend_func( ctx, chanType );

   swrast->BlendFunc( ctx, n, mask, src, dst, chanType );
}


/**
 * Make sure we have texture image data for all the textures we may need
 * for subsequent rendering.
 */
static void
_swrast_validate_texture_images(GLcontext *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint u;

   if (!swrast->ValidateTextureImage || !ctx->Texture._EnabledUnits) {
      /* no textures enabled, or no way to validate images! */
      return;
   }

   for (u = 0; u < ctx->Const.MaxTextureImageUnits; u++) {
      if (ctx->Texture.Unit[u]._ReallyEnabled) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[u]._Current;
         ASSERT(texObj);
         if (texObj) {
            GLuint numFaces = (texObj->Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
            GLuint face;
            for (face = 0; face < numFaces; face++) {
               GLint lvl;
               for (lvl = texObj->BaseLevel; lvl <= texObj->_MaxLevel; lvl++) {
                  struct gl_texture_image *texImg = texObj->Image[face][lvl];
                  if (texImg && !texImg->Data) {
                     swrast->ValidateTextureImage(ctx, texObj, face, lvl);
                     ASSERT(texObj->Image[face][lvl]->Data);
                  }
               }
            }
         }
      }
   }
}


/**
 * Free the texture image data attached to all currently enabled
 * textures.  Meant to be called by device drivers when transitioning
 * from software to hardware rendering.
 */
void
_swrast_eject_texture_images(GLcontext *ctx)
{
   GLuint u;

   if (!ctx->Texture._EnabledUnits) {
      /* no textures enabled */
      return;
   }

   for (u = 0; u < ctx->Const.MaxTextureImageUnits; u++) {
      if (ctx->Texture.Unit[u]._ReallyEnabled) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[u]._Current;
         ASSERT(texObj);
         if (texObj) {
            GLuint numFaces = (texObj->Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
            GLuint face;
            for (face = 0; face < numFaces; face++) {
               GLint lvl;
               for (lvl = texObj->BaseLevel; lvl <= texObj->_MaxLevel; lvl++) {
                  struct gl_texture_image *texImg = texObj->Image[face][lvl];
                  if (texImg && texImg->Data) {
                     _mesa_free_texmemory(texImg->Data);
                     texImg->Data = NULL;
                  }
               }
            }
         }
      }
   }
}



static void
_swrast_sleep( GLcontext *ctx, GLbitfield new_state )
{
   (void) ctx; (void) new_state;
}


static void
_swrast_invalidate_state( GLcontext *ctx, GLbitfield new_state )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint i;

   swrast->NewState |= new_state;

   /* After 10 statechanges without any swrast functions being called,
    * put the module to sleep.
    */
   if (++swrast->StateChanges > 10) {
      swrast->InvalidateState = _swrast_sleep;
      swrast->NewState = ~0;
      new_state = ~0;
   }

   {
      const struct gl_fragment_program *fp = ctx->FragmentProgram._Current;
      if (fp && (fp->Base.Parameters->StateFlags & new_state)) {
         _mesa_load_state_parameters(ctx, fp->Base.Parameters);
      }
   }

   if (new_state & swrast->InvalidateTriangleMask)
      swrast->Triangle = _swrast_validate_triangle;

   if (new_state & swrast->InvalidateLineMask)
      swrast->Line = _swrast_validate_line;

   if (new_state & swrast->InvalidatePointMask)
      swrast->Point = _swrast_validate_point;

   if (new_state & _SWRAST_NEW_BLEND_FUNC)
      swrast->BlendFunc = _swrast_validate_blend_func;

   if (new_state & _SWRAST_NEW_TEXTURE_SAMPLE_FUNC)
      for (i = 0 ; i < ctx->Const.MaxTextureImageUnits ; i++)
	 swrast->TextureSample[i] = NULL;
}


void
_swrast_update_texture_samplers(GLcontext *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint u;

   for (u = 0; u < ctx->Const.MaxTextureImageUnits; u++) {
      const struct gl_texture_object *tObj = ctx->Texture.Unit[u]._Current;
      /* Note: If tObj is NULL, the sample function will be a simple
       * function that just returns opaque black (0,0,0,1).
       */
      swrast->TextureSample[u] = _swrast_choose_texture_sample_func(ctx, tObj);
   }
}


/**
 * Update swrast->_ActiveAttribs, swrast->_NumActiveAttribs,
 * swrast->_ActiveAtttribMask.
 */
static void
_swrast_update_active_attribs(GLcontext *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint attribsMask;

   /*
    * Compute _ActiveAttribsMask = which fragment attributes are needed.
    */
   if (ctx->FragmentProgram._Current) {
      /* fragment program/shader */
      attribsMask = ctx->FragmentProgram._Current->Base.InputsRead;
      attribsMask &= ~FRAG_BIT_WPOS; /* WPOS is always handled specially */
   }
   else if (ctx->ATIFragmentShader._Enabled) {
      attribsMask = ~0;  /* XXX fix me */
   }
   else {
      /* fixed function */
      attribsMask = 0x0;

#if CHAN_TYPE == GL_FLOAT
      attribsMask |= FRAG_BIT_COL0;
#endif

      if (ctx->Fog.ColorSumEnabled ||
          (ctx->Light.Enabled &&
           ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)) {
         attribsMask |= FRAG_BIT_COL1;
      }

      if (swrast->_FogEnabled)
         attribsMask |= FRAG_BIT_FOGC;

      attribsMask |= (ctx->Texture._EnabledUnits << FRAG_ATTRIB_TEX0);
   }

   swrast->_ActiveAttribMask = attribsMask;

   /* Update _ActiveAttribs[] list */
   {
      GLuint i, num = 0;
      for (i = 0; i < FRAG_ATTRIB_MAX; i++) {
         if (attribsMask & (1 << i)) {
            swrast->_ActiveAttribs[num++] = i;
            /* how should this attribute be interpolated? */
            if (i == FRAG_ATTRIB_COL0 || i == FRAG_ATTRIB_COL1)
               swrast->_InterpMode[i] = ctx->Light.ShadeModel;
            else
               swrast->_InterpMode[i] = GL_SMOOTH;
         }
      }
      swrast->_NumActiveAttribs = num;
   }
}


void
_swrast_validate_derived( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (swrast->NewState) {
      if (swrast->NewState & _NEW_POLYGON)
	 _swrast_update_polygon( ctx );

      if (swrast->NewState & (_NEW_HINT | _NEW_PROGRAM))
	 _swrast_update_fog_hint( ctx );

      if (swrast->NewState & _SWRAST_NEW_TEXTURE_ENV_MODE)
	 _swrast_update_texture_env( ctx );

      if (swrast->NewState & (_NEW_FOG | _NEW_PROGRAM))
         _swrast_update_fog_state( ctx );

      if (swrast->NewState & (_NEW_MODELVIEW |
                              _NEW_PROJECTION |
                              _NEW_TEXTURE_MATRIX |
                              _NEW_FOG |
                              _NEW_LIGHT |
                              _NEW_LINE |
                              _NEW_TEXTURE |
                              _NEW_TRANSFORM |
                              _NEW_POINT |
                              _NEW_VIEWPORT |
                              _NEW_PROGRAM))
	 _swrast_update_fragment_program( ctx, swrast->NewState );

      if (swrast->NewState & (_NEW_TEXTURE | _NEW_PROGRAM)) {
         _swrast_update_texture_samplers( ctx );
         _swrast_validate_texture_images(ctx);
      }

      if (swrast->NewState & (_NEW_COLOR | _NEW_PROGRAM))
         _swrast_update_deferred_texture(ctx);

      if (swrast->NewState & _SWRAST_NEW_RASTERMASK)
 	 _swrast_update_rasterflags( ctx );

      if (swrast->NewState & (_NEW_DEPTH |
                              _NEW_FOG |
                              _NEW_LIGHT |
                              _NEW_PROGRAM |
                              _NEW_TEXTURE))
         _swrast_update_active_attribs(ctx);

      swrast->NewState = 0;
      swrast->StateChanges = 0;
      swrast->InvalidateState = _swrast_invalidate_state;
   }
}

#define SWRAST_DEBUG 0

/* Public entrypoints:  See also s_accum.c, s_bitmap.c, etc.
 */
void
_swrast_Quad( GLcontext *ctx,
	      const SWvertex *v0, const SWvertex *v1,
              const SWvertex *v2, const SWvertex *v3 )
{
   if (SWRAST_DEBUG) {
      _mesa_debug(ctx, "_swrast_Quad\n");
      _swrast_print_vertex( ctx, v0 );
      _swrast_print_vertex( ctx, v1 );
      _swrast_print_vertex( ctx, v2 );
      _swrast_print_vertex( ctx, v3 );
   }
   SWRAST_CONTEXT(ctx)->Triangle( ctx, v0, v1, v3 );
   SWRAST_CONTEXT(ctx)->Triangle( ctx, v1, v2, v3 );
}

void
_swrast_Triangle( GLcontext *ctx, const SWvertex *v0,
                  const SWvertex *v1, const SWvertex *v2 )
{
   if (SWRAST_DEBUG) {
      _mesa_debug(ctx, "_swrast_Triangle\n");
      _swrast_print_vertex( ctx, v0 );
      _swrast_print_vertex( ctx, v1 );
      _swrast_print_vertex( ctx, v2 );
   }
   SWRAST_CONTEXT(ctx)->Triangle( ctx, v0, v1, v2 );
}

void
_swrast_Line( GLcontext *ctx, const SWvertex *v0, const SWvertex *v1 )
{
   if (SWRAST_DEBUG) {
      _mesa_debug(ctx, "_swrast_Line\n");
      _swrast_print_vertex( ctx, v0 );
      _swrast_print_vertex( ctx, v1 );
   }
   SWRAST_CONTEXT(ctx)->Line( ctx, v0, v1 );
}

void
_swrast_Point( GLcontext *ctx, const SWvertex *v0 )
{
   if (SWRAST_DEBUG) {
      _mesa_debug(ctx, "_swrast_Point\n");
      _swrast_print_vertex( ctx, v0 );
   }
   SWRAST_CONTEXT(ctx)->Point( ctx, v0 );
}

void
_swrast_InvalidateState( GLcontext *ctx, GLbitfield new_state )
{
   if (SWRAST_DEBUG) {
      _mesa_debug(ctx, "_swrast_InvalidateState\n");
   }
   SWRAST_CONTEXT(ctx)->InvalidateState( ctx, new_state );
}

void
_swrast_ResetLineStipple( GLcontext *ctx )
{
   if (SWRAST_DEBUG) {
      _mesa_debug(ctx, "_swrast_ResetLineStipple\n");
   }
   SWRAST_CONTEXT(ctx)->StippleCounter = 0;
}

void
_swrast_SetFacing(GLcontext *ctx, GLuint facing)
{
   SWRAST_CONTEXT(ctx)->PointLineFacing = facing;
}

void
_swrast_allow_vertex_fog( GLcontext *ctx, GLboolean value )
{
   if (SWRAST_DEBUG) {
      _mesa_debug(ctx, "_swrast_allow_vertex_fog %d\n", value);
   }
   SWRAST_CONTEXT(ctx)->InvalidateState( ctx, _NEW_HINT );
   SWRAST_CONTEXT(ctx)->AllowVertexFog = value;
}

void
_swrast_allow_pixel_fog( GLcontext *ctx, GLboolean value )
{
   if (SWRAST_DEBUG) {
      _mesa_debug(ctx, "_swrast_allow_pixel_fog %d\n", value);
   }
   SWRAST_CONTEXT(ctx)->InvalidateState( ctx, _NEW_HINT );
   SWRAST_CONTEXT(ctx)->AllowPixelFog = value;
}


GLboolean
_swrast_CreateContext( GLcontext *ctx )
{
   GLuint i;
   SWcontext *swrast = (SWcontext *)CALLOC(sizeof(SWcontext));

   if (SWRAST_DEBUG) {
      _mesa_debug(ctx, "_swrast_CreateContext\n");
   }

   if (!swrast)
      return GL_FALSE;

   swrast->NewState = ~0;

   swrast->choose_point = _swrast_choose_point;
   swrast->choose_line = _swrast_choose_line;
   swrast->choose_triangle = _swrast_choose_triangle;

   swrast->InvalidatePointMask = _SWRAST_NEW_POINT;
   swrast->InvalidateLineMask = _SWRAST_NEW_LINE;
   swrast->InvalidateTriangleMask = _SWRAST_NEW_TRIANGLE;

   swrast->Point = _swrast_validate_point;
   swrast->Line = _swrast_validate_line;
   swrast->Triangle = _swrast_validate_triangle;
   swrast->InvalidateState = _swrast_sleep;
   swrast->BlendFunc = _swrast_validate_blend_func;

   swrast->AllowVertexFog = GL_TRUE;
   swrast->AllowPixelFog = GL_TRUE;

   /* Optimized Accum buffer */
   swrast->_IntegerAccumMode = GL_FALSE;
   swrast->_IntegerAccumScaler = 0.0;

   for (i = 0; i < MAX_TEXTURE_IMAGE_UNITS; i++)
      swrast->TextureSample[i] = NULL;

   swrast->SpanArrays = MALLOC_STRUCT(sw_span_arrays);
   if (!swrast->SpanArrays) {
      FREE(swrast);
      return GL_FALSE;
   }
   swrast->SpanArrays->ChanType = CHAN_TYPE;
#if CHAN_TYPE == GL_UNSIGNED_BYTE
   swrast->SpanArrays->rgba = swrast->SpanArrays->rgba8;
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
   swrast->SpanArrays->rgba = swrast->SpanArrays->rgba16;
#else
   swrast->SpanArrays->rgba = swrast->SpanArrays->attribs[FRAG_ATTRIB_COL0];
#endif

   /* init point span buffer */
   swrast->PointSpan.primitive = GL_POINT;
   swrast->PointSpan.end = 0;
   swrast->PointSpan.facing = 0;
   swrast->PointSpan.array = swrast->SpanArrays;

   swrast->TexelBuffer = (GLchan *) MALLOC(ctx->Const.MaxTextureImageUnits *
                                           MAX_WIDTH * 4 * sizeof(GLchan));
   if (!swrast->TexelBuffer) {
      FREE(swrast->SpanArrays);
      FREE(swrast);
      return GL_FALSE;
   }

   ctx->swrast_context = swrast;

   return GL_TRUE;
}

void
_swrast_DestroyContext( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (SWRAST_DEBUG) {
      _mesa_debug(ctx, "_swrast_DestroyContext\n");
   }

   FREE( swrast->SpanArrays );
   if (swrast->ZoomedArrays)
      FREE( swrast->ZoomedArrays );
   FREE( swrast->TexelBuffer );
   FREE( swrast );

   ctx->swrast_context = 0;
}


struct swrast_device_driver *
_swrast_GetDeviceDriverReference( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   return &swrast->Driver;
}

void
_swrast_flush( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   /* flush any pending fragments from rendering points */
   if (swrast->PointSpan.end > 0) {
      if (ctx->Visual.rgbMode) {
         _swrast_write_rgba_span(ctx, &(swrast->PointSpan));
      }
      else {
         _swrast_write_index_span(ctx, &(swrast->PointSpan));
      }
      swrast->PointSpan.end = 0;
   }
}

void
_swrast_render_primitive( GLcontext *ctx, GLenum prim )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   if (swrast->Primitive == GL_POINTS && prim != GL_POINTS) {
      _swrast_flush(ctx);
   }
   swrast->Primitive = prim;
}


void
_swrast_render_start( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   if (swrast->Driver.SpanRenderStart)
      swrast->Driver.SpanRenderStart( ctx );
   swrast->PointSpan.end = 0;
}
 
void
_swrast_render_finish( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   if (swrast->Driver.SpanRenderFinish)
      swrast->Driver.SpanRenderFinish( ctx );

   _swrast_flush(ctx);
}


#define SWRAST_DEBUG_VERTICES 0

void
_swrast_print_vertex( GLcontext *ctx, const SWvertex *v )
{
   GLuint i;

   if (SWRAST_DEBUG_VERTICES) {
      _mesa_debug(ctx, "win %f %f %f %f\n",
                  v->attrib[FRAG_ATTRIB_WPOS][0],
                  v->attrib[FRAG_ATTRIB_WPOS][1],
                  v->attrib[FRAG_ATTRIB_WPOS][2],
                  v->attrib[FRAG_ATTRIB_WPOS][3]);

      for (i = 0 ; i < ctx->Const.MaxTextureCoordUnits ; i++)
	 if (ctx->Texture.Unit[i]._ReallyEnabled)
	    _mesa_debug(ctx, "texcoord[%d] %f %f %f %f\n", i,
                        v->attrib[FRAG_ATTRIB_TEX0 + i][0],
                        v->attrib[FRAG_ATTRIB_TEX0 + i][1],
                        v->attrib[FRAG_ATTRIB_TEX0 + i][2],
                        v->attrib[FRAG_ATTRIB_TEX0 + i][3]);

#if CHAN_TYPE == GL_FLOAT
      _mesa_debug(ctx, "color %f %f %f %f\n",
                  v->color[0], v->color[1], v->color[2], v->color[3]);
#else
      _mesa_debug(ctx, "color %d %d %d %d\n",
                  v->color[0], v->color[1], v->color[2], v->color[3]);
#endif
      _mesa_debug(ctx, "spec %g %g %g %g\n",
                  v->attrib[FRAG_ATTRIB_COL1][0],
                  v->attrib[FRAG_ATTRIB_COL1][1],
                  v->attrib[FRAG_ATTRIB_COL1][2],
                  v->attrib[FRAG_ATTRIB_COL1][3]);
      _mesa_debug(ctx, "fog %f\n", v->attrib[FRAG_ATTRIB_FOGC][0]);
      _mesa_debug(ctx, "index %d\n", v->attrib[FRAG_ATTRIB_CI][0]);
      _mesa_debug(ctx, "pointsize %f\n", v->pointSize);
      _mesa_debug(ctx, "\n");
   }
}
