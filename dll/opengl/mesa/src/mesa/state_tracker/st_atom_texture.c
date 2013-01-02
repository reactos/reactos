/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */
 

#include "main/macros.h"
#include "main/mtypes.h"
#include "main/samplerobj.h"
#include "program/prog_instruction.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_texture.h"
#include "st_format.h"
#include "st_cb_texture.h"
#include "pipe/p_context.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "cso_cache/cso_context.h"


/**
 * Combine depth texture mode with "swizzle" so that depth mode swizzling
 * takes place before texture swizzling, and return the resulting swizzle.
 * If the format is not a depth format, return "swizzle" unchanged.
 *
 * \param format     PIPE_FORMAT_*.
 * \param swizzle    Texture swizzle, a bitmask computed using MAKE_SWIZZLE4.
 * \param depthmode  One of GL_LUMINANCE, GL_INTENSITY, GL_ALPHA, GL_RED.
 */
static GLuint
apply_depthmode(enum pipe_format format, GLuint swizzle, GLenum depthmode)
{
   const struct util_format_description *desc =
         util_format_description(format);
   unsigned char swiz[4];
   unsigned i;

   if (desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS ||
       desc->swizzle[0] == UTIL_FORMAT_SWIZZLE_NONE) {
      /* Not a depth format. */
      return swizzle;
   }

   for (i = 0; i < 4; i++)
      swiz[i] = GET_SWZ(swizzle, i);

   switch (depthmode) {
      case GL_LUMINANCE:
         /* Rewrite reads from W to ONE, and reads from XYZ to XXX. */
         for (i = 0; i < 4; i++)
            if (swiz[i] == SWIZZLE_W)
               swiz[i] = SWIZZLE_ONE;
            else if (swiz[i] < SWIZZLE_W)
               swiz[i] = SWIZZLE_X;
         break;

      case GL_INTENSITY:
         /* Rewrite reads from XYZW to XXXX. */
         for (i = 0; i < 4; i++)
            if (swiz[i] <= SWIZZLE_W)
               swiz[i] = SWIZZLE_X;
         break;

      case GL_ALPHA:
         /* Rewrite reads from W to X, and reads from XYZ to 000. */
         for (i = 0; i < 4; i++)
            if (swiz[i] == SWIZZLE_W)
               swiz[i] = SWIZZLE_X;
            else if (swiz[i] < SWIZZLE_W)
               swiz[i] = SWIZZLE_ZERO;
         break;
      case GL_RED:
	 /* Rewrite reads W to 1, XYZ to X00 */
	 for (i = 0; i < 4; i++)
	    if (swiz[i] == SWIZZLE_W)
	       swiz[i] = SWIZZLE_ONE;
	    else if (swiz[i] == SWIZZLE_Y || swiz[i] == SWIZZLE_Z)
	       swiz[i] = SWIZZLE_ZERO;
	 break;
   }

   return MAKE_SWIZZLE4(swiz[0], swiz[1], swiz[2], swiz[3]);
}


/**
 * Return TRUE if the swizzling described by "swizzle" and
 * "depthmode" (for depth textures only) is different from the swizzling
 * set in the given sampler view.
 *
 * \param sv         A sampler view.
 * \param swizzle    Texture swizzle, a bitmask computed using MAKE_SWIZZLE4.
 * \param depthmode  One of GL_LUMINANCE, GL_INTENSITY, GL_ALPHA.
 */
static boolean
check_sampler_swizzle(struct pipe_sampler_view *sv,
                      GLuint swizzle, GLenum depthmode)
{
   swizzle = apply_depthmode(sv->texture->format, swizzle, depthmode);

   if ((sv->swizzle_r != GET_SWZ(swizzle, 0)) ||
       (sv->swizzle_g != GET_SWZ(swizzle, 1)) ||
       (sv->swizzle_b != GET_SWZ(swizzle, 2)) ||
       (sv->swizzle_a != GET_SWZ(swizzle, 3)))
      return TRUE;
   return FALSE;
}


static INLINE struct pipe_sampler_view *
st_create_texture_sampler_view_from_stobj(struct pipe_context *pipe,
					  struct st_texture_object *stObj,
                                          const struct gl_sampler_object *samp,
					  enum pipe_format format)
{
   struct pipe_sampler_view templ;
   GLuint swizzle = apply_depthmode(stObj->pt->format,
                                    stObj->base._Swizzle,
                                    samp->DepthMode);

   u_sampler_view_default_template(&templ,
                                   stObj->pt,
                                   format);
   templ.u.tex.first_level = stObj->base.BaseLevel;

   if (swizzle != SWIZZLE_NOOP) {
      templ.swizzle_r = GET_SWZ(swizzle, 0);
      templ.swizzle_g = GET_SWZ(swizzle, 1);
      templ.swizzle_b = GET_SWZ(swizzle, 2);
      templ.swizzle_a = GET_SWZ(swizzle, 3);
   }

   return pipe->create_sampler_view(pipe, stObj->pt, &templ);
}


static INLINE struct pipe_sampler_view *
st_get_texture_sampler_view_from_stobj(struct st_texture_object *stObj,
				       struct pipe_context *pipe,
                                       const struct gl_sampler_object *samp,
				       enum pipe_format format)
{
   if (!stObj || !stObj->pt) {
      return NULL;
   }

   if (!stObj->sampler_view) {
      stObj->sampler_view =
         st_create_texture_sampler_view_from_stobj(pipe, stObj, samp, format);
   }

   return stObj->sampler_view;
}

static GLboolean
update_single_texture(struct st_context *st, struct pipe_sampler_view **sampler_view,
		      GLuint texUnit)
{
   struct pipe_context *pipe = st->pipe;
   struct gl_context *ctx = st->ctx;
   const struct gl_sampler_object *samp;
   struct gl_texture_object *texObj;
   struct st_texture_object *stObj;
   enum pipe_format st_view_format;
   GLboolean retval;

   samp = _mesa_get_samplerobj(ctx, texUnit);

   texObj = ctx->Texture.Unit[texUnit]._Current;

   if (!texObj) {
      texObj = st_get_default_texture(st);
      samp = &texObj->Sampler;
   }
   stObj = st_texture_object(texObj);

   retval = st_finalize_texture(ctx, st->pipe, texObj);
   if (!retval) {
      /* out of mem */
      return GL_FALSE;
   }

   /* Determine the format of the texture sampler view */
   st_view_format = stObj->pt->format;
   {
      const struct st_texture_image *firstImage =
	 st_texture_image(stObj->base.Image[0][stObj->base.BaseLevel]);
      const gl_format texFormat = firstImage->base.TexFormat;
      enum pipe_format firstImageFormat =
	 st_mesa_format_to_pipe_format(texFormat);

      if ((samp->sRGBDecode == GL_SKIP_DECODE_EXT) &&
	  (_mesa_get_format_color_encoding(texFormat) == GL_SRGB)) {
         /* Don't do sRGB->RGB conversion.  Interpret the texture data as
          * linear values.
          */
	 const gl_format linearFormat =
	    _mesa_get_srgb_format_linear(texFormat);
	 firstImageFormat = st_mesa_format_to_pipe_format(linearFormat);
      }

      if (firstImageFormat != stObj->pt->format)
	 st_view_format = firstImageFormat;
   }


   /* if sampler view has changed dereference it */
   if (stObj->sampler_view) {
      if (check_sampler_swizzle(stObj->sampler_view,
				stObj->base._Swizzle,
				samp->DepthMode) ||
	  (st_view_format != stObj->sampler_view->format) ||
	  stObj->base.BaseLevel != stObj->sampler_view->u.tex.first_level) {
	 pipe_sampler_view_reference(&stObj->sampler_view, NULL);
      }
   }

   *sampler_view = st_get_texture_sampler_view_from_stobj(stObj, pipe,
							  samp,
							  st_view_format);
   return GL_TRUE;
}

static void 
update_vertex_textures(struct st_context *st)
{
   const struct gl_context *ctx = st->ctx;
   struct gl_vertex_program *vprog = ctx->VertexProgram._Current;
   GLuint su;

   st->state.num_vertex_textures = 0;

   /* loop over sampler units (aka tex image units) */
   for (su = 0; su < ctx->Const.MaxTextureImageUnits; su++) {
      struct pipe_sampler_view *sampler_view = NULL;
      if (vprog->Base.SamplersUsed & (1 << su)) {
         GLboolean retval;
         GLuint texUnit;

         texUnit = vprog->Base.SamplerUnits[su];

         retval = update_single_texture(st, &sampler_view, texUnit);
         if (retval == GL_FALSE)
            continue;

         st->state.num_vertex_textures = su + 1;
      }
      pipe_sampler_view_reference(&st->state.sampler_vertex_views[su], sampler_view);
   }

   if (ctx->Const.MaxVertexTextureImageUnits > 0) {
      GLuint numUnits = MIN2(st->state.num_vertex_textures,
                             ctx->Const.MaxVertexTextureImageUnits);
      cso_set_vertex_sampler_views(st->cso_context,
                                   numUnits,
                                   st->state.sampler_vertex_views);
   }
}

static void 
update_fragment_textures(struct st_context *st)
{
   const struct gl_context *ctx = st->ctx;
   struct gl_fragment_program *fprog = ctx->FragmentProgram._Current;
   GLuint su;

   st->state.num_textures = 0;

   /* loop over sampler units (aka tex image units) */
   for (su = 0; su < ctx->Const.MaxTextureImageUnits; su++) {
      struct pipe_sampler_view *sampler_view = NULL;
      if (fprog->Base.SamplersUsed & (1 << su)) {
         GLboolean retval;
         GLuint texUnit;

	 texUnit = fprog->Base.SamplerUnits[su];

	 retval = update_single_texture(st, &sampler_view, texUnit);
	 if (retval == GL_FALSE)
	    continue;

         st->state.num_textures = su + 1;
      }
      pipe_sampler_view_reference(&st->state.sampler_views[su], sampler_view);
   }

   cso_set_fragment_sampler_views(st->cso_context,
                                  st->state.num_textures,
                                  st->state.sampler_views);
}

const struct st_tracked_state st_update_texture = {
   "st_update_texture",					/* name */
   {							/* dirty */
      _NEW_TEXTURE,					/* mesa */
      ST_NEW_FRAGMENT_PROGRAM,				/* st */
   },
   update_fragment_textures				/* update */
};

const struct st_tracked_state st_update_vertex_texture = {
   "st_update_vertex_texture",					/* name */
   {							/* dirty */
      _NEW_TEXTURE,					/* mesa */
      ST_NEW_VERTEX_PROGRAM,				/* st */
   },
   update_vertex_textures				/* update */
};

static void 
finalize_textures(struct st_context *st)
{
   struct gl_context *ctx = st->ctx;
   struct gl_fragment_program *fprog = ctx->FragmentProgram._Current;
   const GLboolean prev_missing_textures = st->missing_textures;
   GLuint su;

   st->missing_textures = GL_FALSE;

   for (su = 0; su < ctx->Const.MaxTextureCoordUnits; su++) {
      if (fprog->Base.SamplersUsed & (1 << su)) {
         const GLuint texUnit = fprog->Base.SamplerUnits[su];
         struct gl_texture_object *texObj
            = ctx->Texture.Unit[texUnit]._Current;

         if (texObj) {
            GLboolean retval;

            retval = st_finalize_texture(ctx, st->pipe, texObj);
            if (!retval) {
               /* out of mem */
               st->missing_textures = GL_TRUE;
               continue;
            }
         }
      }
   }

   if (prev_missing_textures != st->missing_textures)
      st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;
}



const struct st_tracked_state st_finalize_textures = {
   "st_finalize_textures",		/* name */
   {					/* dirty */
      _NEW_TEXTURE,			/* mesa */
      0,				/* st */
   },
   finalize_textures			/* update */
};
