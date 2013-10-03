/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * \file texparam.c
 *
 * glTexParameter-related functions
 */


#include "main/glheader.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/formats.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "main/state.h"
#include "main/texparam.h"
#include "main/teximage.h"
#include "main/texstate.h"
#include "program/prog_instruction.h"


/**
 * Check if a coordinate wrap mode is supported for the texture target.
 * \return GL_TRUE if legal, GL_FALSE otherwise
 */
static GLboolean 
validate_texture_wrap_mode(struct gl_context * ctx, GLenum target, GLenum wrap)
{
   const struct gl_extensions * const e = & ctx->Extensions;

   switch (wrap) {
   case GL_CLAMP:
   case GL_REPEAT:
   case GL_CLAMP_TO_EDGE:
   case GL_MIRRORED_REPEAT:
      return GL_TRUE;
   case GL_CLAMP_TO_BORDER:
      if (e->ARB_texture_border_clamp)
         return GL_TRUE;
      break;
   case GL_MIRROR_CLAMP_EXT:
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      if (e->ATI_texture_mirror_once || e->EXT_texture_mirror_clamp)
         return GL_TRUE;
      break;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      if (e->EXT_texture_mirror_clamp)
         return GL_TRUE;
      break;
   default:
      break;
   }

   _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(param=0x%x)", wrap );
   return GL_FALSE;
}


/**
 * Get current texture object for given target.
 * Return NULL if any error (and record the error).
 * Note that this is different from _mesa_select_tex_object() in that proxy
 * targets are not accepted.
 * Only the glGetTexLevelParameter() functions accept proxy targets.
 */
static struct gl_texture_object *
get_texobj(struct gl_context *ctx, GLenum target, GLboolean get)
{
   struct gl_texture_unit *texUnit;

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxCombinedTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "gl%sTexParameter(current unit)", get ? "Get" : "");
      return NULL;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);

   switch (target) {
   case GL_TEXTURE_1D:
      return texUnit->CurrentTex[TEXTURE_1D_INDEX];
   case GL_TEXTURE_2D:
      return texUnit->CurrentTex[TEXTURE_2D_INDEX];
   case GL_TEXTURE_3D:
      return texUnit->CurrentTex[TEXTURE_3D_INDEX];
   case GL_TEXTURE_CUBE_MAP:
      if (ctx->Extensions.ARB_texture_cube_map) {
         return texUnit->CurrentTex[TEXTURE_CUBE_INDEX];
      }
      break;
   default:
      ;
   }

   _mesa_error(ctx, GL_INVALID_ENUM,
                  "gl%sTexParameter(target)", get ? "Get" : "");
   return NULL;
}


/**
 * This is called just prior to changing any texture object state which
 * will not effect texture completeness.
 */
static inline void
flush(struct gl_context *ctx)
{
   FLUSH_VERTICES(ctx, _NEW_TEXTURE);
}


/**
 * This is called just prior to changing any texture object state which
 * can effect texture completeness (texture base level, max level,
 * minification filter).
 * Any pending rendering will be flushed out, we'll set the _NEW_TEXTURE
 * state flag and then mark the texture object as 'incomplete' so that any
 * per-texture derived state gets recomputed.
 */
static inline void
incomplete(struct gl_context *ctx, struct gl_texture_object *texObj)
{
   FLUSH_VERTICES(ctx, _NEW_TEXTURE);
   texObj->_Complete = GL_FALSE;
}


/**
 * Set an integer-valued texture parameter
 * \return GL_TRUE if legal AND the value changed, GL_FALSE otherwise
 */
static GLboolean
set_tex_parameteri(struct gl_context *ctx,
                   struct gl_texture_object *texObj,
                   GLenum pname, const GLint *params)
{
   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
      if (texObj->Sampler.MinFilter == params[0])
         return GL_FALSE;
      switch (params[0]) {
      case GL_NEAREST:
      case GL_LINEAR:
         incomplete(ctx, texObj);
         texObj->Sampler.MinFilter = params[0];
         return GL_TRUE;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
         incomplete(ctx, texObj);
         texObj->Sampler.MinFilter = params[0];
         return GL_TRUE;
         /* fall-through */
      default:
         goto invalid_param;
      }
      return GL_FALSE;

   case GL_TEXTURE_MAG_FILTER:
      if (texObj->Sampler.MagFilter == params[0])
         return GL_FALSE;
      switch (params[0]) {
      case GL_NEAREST:
      case GL_LINEAR:
         flush(ctx); /* does not effect completeness */
         texObj->Sampler.MagFilter = params[0];
         return GL_TRUE;
      default:
         goto invalid_param;
      }
      return GL_FALSE;

   case GL_TEXTURE_WRAP_S:
      if (texObj->Sampler.WrapS == params[0])
         return GL_FALSE;
      if (validate_texture_wrap_mode(ctx, texObj->Target, params[0])) {
         flush(ctx);
         texObj->Sampler.WrapS = params[0];
         return GL_TRUE;
      }
      return GL_FALSE;

   case GL_TEXTURE_WRAP_T:
      if (texObj->Sampler.WrapT == params[0])
         return GL_FALSE;
      if (validate_texture_wrap_mode(ctx, texObj->Target, params[0])) {
         flush(ctx);
         texObj->Sampler.WrapT = params[0];
         return GL_TRUE;
      }
      return GL_FALSE;

   case GL_TEXTURE_WRAP_R:
      if (texObj->Sampler.WrapR == params[0])
         return GL_FALSE;
      if (validate_texture_wrap_mode(ctx, texObj->Target, params[0])) {
         flush(ctx);
         texObj->Sampler.WrapR = params[0];
         return GL_TRUE;
      }
      return GL_FALSE;

   case GL_TEXTURE_BASE_LEVEL:
      if (texObj->BaseLevel == params[0])
         return GL_FALSE;
      if (params[0] < 0) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexParameter(param=%d)", params[0]);
         return GL_FALSE;
      }
      incomplete(ctx, texObj);
      texObj->BaseLevel = params[0];
      return GL_TRUE;

   case GL_TEXTURE_MAX_LEVEL:
      if (texObj->MaxLevel == params[0])
         return GL_FALSE;
      if (params[0] < 0) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexParameter(param=%d)", params[0]);
         return GL_FALSE;
      }
      incomplete(ctx, texObj);
      texObj->MaxLevel = params[0];
      return GL_TRUE;

   case GL_GENERATE_MIPMAP_SGIS:
      if (texObj->GenerateMipmap != params[0]) {
         /* no flush() */
	 texObj->GenerateMipmap = params[0] ? GL_TRUE : GL_FALSE;
	 return GL_TRUE;
      }
      return GL_FALSE;

   default:
      goto invalid_pname;
   }

invalid_pname:
   _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(pname=%s)",
               _mesa_lookup_enum_by_nr(pname));
   return GL_FALSE;

invalid_param:
   _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(param=%s)",
               _mesa_lookup_enum_by_nr(params[0]));
   return GL_FALSE;
}


/**
 * Set a float-valued texture parameter
 * \return GL_TRUE if legal AND the value changed, GL_FALSE otherwise
 */
static GLboolean
set_tex_parameterf(struct gl_context *ctx,
                   struct gl_texture_object *texObj,
                   GLenum pname, const GLfloat *params)
{
   switch (pname) {
   case GL_TEXTURE_MIN_LOD:
      if (texObj->Sampler.MinLod == params[0])
         return GL_FALSE;
      flush(ctx);
      texObj->Sampler.MinLod = params[0];
      return GL_TRUE;

   case GL_TEXTURE_MAX_LOD:
      if (texObj->Sampler.MaxLod == params[0])
         return GL_FALSE;
      flush(ctx);
      texObj->Sampler.MaxLod = params[0];
      return GL_TRUE;

   case GL_TEXTURE_PRIORITY:
      flush(ctx);
      texObj->Priority = CLAMP(params[0], 0.0F, 1.0F);
      return GL_TRUE;

   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      if (ctx->Extensions.EXT_texture_filter_anisotropic) {
         if (texObj->Sampler.MaxAnisotropy == params[0])
            return GL_FALSE;
         if (params[0] < 1.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return GL_FALSE;
         }
         flush(ctx);
         /* clamp to max, that's what NVIDIA does */
         texObj->Sampler.MaxAnisotropy = MIN2(params[0],
                                      ctx->Const.MaxTextureMaxAnisotropy);
         return GL_TRUE;
      }
      else {
         static GLuint count = 0;
         if (count++ < 10)
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_MAX_ANISOTROPY_EXT)");
      }
      return GL_FALSE;

   case GL_TEXTURE_LOD_BIAS:
      /* NOTE: this is really part of OpenGL 1.4, not EXT_texture_lod_bias */
      if (texObj->Sampler.LodBias != params[0]) {
	 flush(ctx);
	 texObj->Sampler.LodBias = params[0];
	 return GL_TRUE;
      }
      break;

   case GL_TEXTURE_BORDER_COLOR:
      flush(ctx);
      /* ARB_texture_float disables clamping */
      if (ctx->Extensions.ARB_texture_float) {
         texObj->Sampler.BorderColor.f[RCOMP] = params[0];
         texObj->Sampler.BorderColor.f[GCOMP] = params[1];
         texObj->Sampler.BorderColor.f[BCOMP] = params[2];
         texObj->Sampler.BorderColor.f[ACOMP] = params[3];
      } else {
         texObj->Sampler.BorderColor.f[RCOMP] = CLAMP(params[0], 0.0F, 1.0F);
         texObj->Sampler.BorderColor.f[GCOMP] = CLAMP(params[1], 0.0F, 1.0F);
         texObj->Sampler.BorderColor.f[BCOMP] = CLAMP(params[2], 0.0F, 1.0F);
         texObj->Sampler.BorderColor.f[ACOMP] = CLAMP(params[3], 0.0F, 1.0F);
      }
      return GL_TRUE;

   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(pname=0x%x)", pname);
   }
   return GL_FALSE;
}


void GLAPIENTRY
_mesa_TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
   GLboolean need_update;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
   case GL_TEXTURE_WRAP_R:
   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_GENERATE_MIPMAP_SGIS:
   case GL_TEXTURE_COMPARE_MODE_ARB:
   case GL_TEXTURE_COMPARE_FUNC_ARB:
   case GL_DEPTH_TEXTURE_MODE_ARB:
   case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      {
         /* convert float param to int */
         GLint p[4];
         p[0] = (GLint) param;
         p[1] = p[2] = p[3] = 0;
         need_update = set_tex_parameteri(ctx, texObj, pname, p);
      }
      break;
   case GL_TEXTURE_SWIZZLE_R_EXT:
   case GL_TEXTURE_SWIZZLE_G_EXT:
   case GL_TEXTURE_SWIZZLE_B_EXT:
   case GL_TEXTURE_SWIZZLE_A_EXT:
      {
         GLint p[4];
         p[0] = (GLint) param;
         p[1] = p[2] = p[3] = 0;
         need_update = set_tex_parameteri(ctx, texObj, pname, p);
      }
      break;
   default:
      {
         /* this will generate an error if pname is illegal */
         GLfloat p[4];
         p[0] = param;
         p[1] = p[2] = p[3] = 0.0F;
         need_update = set_tex_parameterf(ctx, texObj, pname, p);
      }
   }

   if (ctx->Driver.TexParameter && need_update) {
      ctx->Driver.TexParameter(ctx, target, texObj, pname, &param);
   }
}


void GLAPIENTRY
_mesa_TexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GLboolean need_update;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
   case GL_TEXTURE_WRAP_R:
   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_GENERATE_MIPMAP_SGIS:
   case GL_TEXTURE_COMPARE_MODE_ARB:
   case GL_TEXTURE_COMPARE_FUNC_ARB:
   case GL_DEPTH_TEXTURE_MODE_ARB:
   case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      {
         /* convert float param to int */
         GLint p[4];
         p[0] = (GLint) params[0];
         p[1] = p[2] = p[3] = 0;
         need_update = set_tex_parameteri(ctx, texObj, pname, p);
      }
      break;

   case GL_TEXTURE_SWIZZLE_R_EXT:
   case GL_TEXTURE_SWIZZLE_G_EXT:
   case GL_TEXTURE_SWIZZLE_B_EXT:
   case GL_TEXTURE_SWIZZLE_A_EXT:
   case GL_TEXTURE_SWIZZLE_RGBA_EXT:
      {
         GLint p[4] = {0, 0, 0, 0};
         p[0] = (GLint) params[0];
         if (pname == GL_TEXTURE_SWIZZLE_RGBA_EXT) {
            p[1] = (GLint) params[1];
            p[2] = (GLint) params[2];
            p[3] = (GLint) params[3];
         }
         need_update = set_tex_parameteri(ctx, texObj, pname, p);
      }
      break;
   default:
      /* this will generate an error if pname is illegal */
      need_update = set_tex_parameterf(ctx, texObj, pname, params);
   }

   if (ctx->Driver.TexParameter && need_update) {
      ctx->Driver.TexParameter(ctx, target, texObj, pname, params);
   }
}


void GLAPIENTRY
_mesa_TexParameteri(GLenum target, GLenum pname, GLint param)
{
   GLboolean need_update;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
   case GL_TEXTURE_PRIORITY:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
   case GL_TEXTURE_LOD_BIAS:
   case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
      {
         GLfloat fparam[4];
         fparam[0] = (GLfloat) param;
         fparam[1] = fparam[2] = fparam[3] = 0.0F;
         /* convert int param to float */
         need_update = set_tex_parameterf(ctx, texObj, pname, fparam);
      }
      break;
   default:
      /* this will generate an error if pname is illegal */
      {
         GLint iparam[4];
         iparam[0] = param;
         iparam[1] = iparam[2] = iparam[3] = 0;
         need_update = set_tex_parameteri(ctx, texObj, pname, iparam);
      }
   }

   if (ctx->Driver.TexParameter && need_update) {
      GLfloat fparam = (GLfloat) param;
      ctx->Driver.TexParameter(ctx, target, texObj, pname, &fparam);
   }
}


void GLAPIENTRY
_mesa_TexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GLboolean need_update;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_BORDER_COLOR:
      {
         /* convert int params to float */
         GLfloat fparams[4];
         fparams[0] = INT_TO_FLOAT(params[0]);
         fparams[1] = INT_TO_FLOAT(params[1]);
         fparams[2] = INT_TO_FLOAT(params[2]);
         fparams[3] = INT_TO_FLOAT(params[3]);
         need_update = set_tex_parameterf(ctx, texObj, pname, fparams);
      }
      break;
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
   case GL_TEXTURE_PRIORITY:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
   case GL_TEXTURE_LOD_BIAS:
   case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
      {
         /* convert int param to float */
         GLfloat fparams[4];
         fparams[0] = (GLfloat) params[0];
         fparams[1] = fparams[2] = fparams[3] = 0.0F;
         need_update = set_tex_parameterf(ctx, texObj, pname, fparams);
      }
      break;
   default:
      /* this will generate an error if pname is illegal */
      need_update = set_tex_parameteri(ctx, texObj, pname, params);
   }

   if (ctx->Driver.TexParameter && need_update) {
      GLfloat fparams[4];
      fparams[0] = INT_TO_FLOAT(params[0]);
      if (pname == GL_TEXTURE_BORDER_COLOR ||
          pname == GL_TEXTURE_CROP_RECT_OES) {
         fparams[1] = INT_TO_FLOAT(params[1]);
         fparams[2] = INT_TO_FLOAT(params[2]);
         fparams[3] = INT_TO_FLOAT(params[3]);
      }
      ctx->Driver.TexParameter(ctx, target, texObj, pname, fparams);
   }
}


/**
 * Set tex parameter to integer value(s).  Primarily intended to set
 * integer-valued texture border color (for integer-valued textures).
 * New in GL 3.0.
 */
void GLAPIENTRY
_mesa_TexParameterIiv(GLenum target, GLenum pname, const GLint *params)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_BORDER_COLOR:
      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
      /* set the integer-valued border color */
      COPY_4V(texObj->Sampler.BorderColor.i, params);
      break;
   default:
      _mesa_TexParameteriv(target, pname, params);
      break;
   }
   /* XXX no driver hook for TexParameterIiv() yet */
}


/**
 * Set tex parameter to unsigned integer value(s).  Primarily intended to set
 * uint-valued texture border color (for integer-valued textures).
 * New in GL 3.0
 */
void GLAPIENTRY
_mesa_TexParameterIuiv(GLenum target, GLenum pname, const GLuint *params)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_FALSE);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_BORDER_COLOR:
      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
      /* set the unsigned integer-valued border color */
      COPY_4V(texObj->Sampler.BorderColor.ui, params);
      break;
   default:
      _mesa_TexParameteriv(target, pname, (const GLint *) params);
      break;
   }
   /* XXX no driver hook for TexParameterIuiv() yet */
}


void GLAPIENTRY
_mesa_GetTexLevelParameterfv( GLenum target, GLint level,
                              GLenum pname, GLfloat *params )
{
   GLint iparam;
   _mesa_GetTexLevelParameteriv( target, level, pname, &iparam );
   *params = (GLfloat) iparam;
}


void GLAPIENTRY
_mesa_GetTexLevelParameteriv( GLenum target, GLint level,
                              GLenum pname, GLint *params )
{
   const struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   const struct gl_texture_image *img = NULL;
   GLint maxLevels;
   gl_format texFormat;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxCombinedTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetTexLevelParameteriv(current unit)");
      return;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);

   /* this will catch bad target values */
   maxLevels = _mesa_max_texture_levels(ctx, target);
   if (maxLevels == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetTexLevelParameter[if]v(target=0x%x)", target);
      return;
   }

   if (level < 0 || level >= maxLevels) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGetTexLevelParameter[if]v" );
      return;
   }

   texObj = _mesa_select_tex_object(ctx, texUnit, target);

   img = _mesa_select_tex_image(ctx, texObj, target, level);
   if (!img || img->TexFormat == MESA_FORMAT_NONE) {
      /* undefined texture image */
      if (pname == GL_TEXTURE_COMPONENTS)
         *params = 1;
      else
         *params = 0;
      return;
   }

   texFormat = img->TexFormat;

   switch (pname) {
      case GL_TEXTURE_WIDTH:
         *params = img->Width;
         break;
      case GL_TEXTURE_HEIGHT:
         *params = img->Height;
         break;
      case GL_TEXTURE_DEPTH:
         *params = img->Depth;
         break;
      case GL_TEXTURE_INTERNAL_FORMAT:
         *params = img->InternalFormat;
         break;
      case GL_TEXTURE_BORDER:
         *params = img->Border;
         break;
      case GL_TEXTURE_RED_SIZE:
      case GL_TEXTURE_GREEN_SIZE:
      case GL_TEXTURE_BLUE_SIZE:
      case GL_TEXTURE_ALPHA_SIZE:
         if (_mesa_base_format_has_channel(img->_BaseFormat, pname))
            *params = _mesa_get_format_bits(texFormat, pname);
         else
            *params = 0;
         break;
      case GL_TEXTURE_INTENSITY_SIZE:
      case GL_TEXTURE_LUMINANCE_SIZE:
         if (_mesa_base_format_has_channel(img->_BaseFormat, pname)) {
            *params = _mesa_get_format_bits(texFormat, pname);
            if (*params == 0) {
               /* intensity or luminance is probably stored as RGB[A] */
               *params = MIN2(_mesa_get_format_bits(texFormat,
                                                    GL_TEXTURE_RED_SIZE),
                              _mesa_get_format_bits(texFormat,
                                                    GL_TEXTURE_GREEN_SIZE));
            }
         }
         else {
            *params = 0;
         }
         break;

      /* GL_ARB_texture_float */
      case GL_TEXTURE_RED_TYPE_ARB:
      case GL_TEXTURE_GREEN_TYPE_ARB:
      case GL_TEXTURE_BLUE_TYPE_ARB:
      case GL_TEXTURE_ALPHA_TYPE_ARB:
      case GL_TEXTURE_LUMINANCE_TYPE_ARB:
      case GL_TEXTURE_INTENSITY_TYPE_ARB:
      case GL_TEXTURE_DEPTH_TYPE_ARB:
         if (!ctx->Extensions.ARB_texture_float)
            goto invalid_pname;
	 if (_mesa_base_format_has_channel(img->_BaseFormat, pname))
	    *params = _mesa_get_format_datatype(texFormat);
	 else
	    *params = GL_NONE;
         break;

      default:
         goto invalid_pname;
   }

   /* no error if we get here */
   return;

invalid_pname:
   _mesa_error(ctx, GL_INVALID_ENUM,
               "glGetTexLevelParameter[if]v(pname=%s)",
               _mesa_lookup_enum_by_nr(pname));
}



void GLAPIENTRY
_mesa_GetTexParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   struct gl_texture_object *obj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   obj = get_texobj(ctx, target, GL_TRUE);
   if (!obj)
      return;

   _mesa_lock_texture(ctx, obj);
   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
	 *params = ENUM_TO_FLOAT(obj->Sampler.MagFilter);
	 break;
      case GL_TEXTURE_MIN_FILTER:
         *params = ENUM_TO_FLOAT(obj->Sampler.MinFilter);
         break;
      case GL_TEXTURE_WRAP_S:
         *params = ENUM_TO_FLOAT(obj->Sampler.WrapS);
         break;
      case GL_TEXTURE_WRAP_T:
         *params = ENUM_TO_FLOAT(obj->Sampler.WrapT);
         break;
      case GL_TEXTURE_WRAP_R:
         *params = ENUM_TO_FLOAT(obj->Sampler.WrapR);
         break;
      case GL_TEXTURE_BORDER_COLOR:
         if (ctx->NewState & (_NEW_BUFFERS | _NEW_FRAG_CLAMP))
            _mesa_update_state_locked(ctx);
         if (ctx->Color._ClampFragmentColor) {
            params[0] = CLAMP(obj->Sampler.BorderColor.f[0], 0.0F, 1.0F);
            params[1] = CLAMP(obj->Sampler.BorderColor.f[1], 0.0F, 1.0F);
            params[2] = CLAMP(obj->Sampler.BorderColor.f[2], 0.0F, 1.0F);
            params[3] = CLAMP(obj->Sampler.BorderColor.f[3], 0.0F, 1.0F);
         }
         else {
            params[0] = obj->Sampler.BorderColor.f[0];
            params[1] = obj->Sampler.BorderColor.f[1];
            params[2] = obj->Sampler.BorderColor.f[2];
            params[3] = obj->Sampler.BorderColor.f[3];
         }
         break;
      case GL_TEXTURE_RESIDENT:
         *params = 1.0F;
         break;
      case GL_TEXTURE_PRIORITY:
         *params = obj->Priority;
         break;
      case GL_TEXTURE_MIN_LOD:
         *params = obj->Sampler.MinLod;
         break;
      case GL_TEXTURE_MAX_LOD:
         *params = obj->Sampler.MaxLod;
         break;
      case GL_TEXTURE_BASE_LEVEL:
         *params = (GLfloat) obj->BaseLevel;
         break;
      case GL_TEXTURE_MAX_LEVEL:
         *params = (GLfloat) obj->MaxLevel;
         break;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (!ctx->Extensions.EXT_texture_filter_anisotropic)
            goto invalid_pname;
         *params = obj->Sampler.MaxAnisotropy;
         break;
      case GL_GENERATE_MIPMAP_SGIS:
	 *params = (GLfloat) obj->GenerateMipmap;
         break;
      case GL_TEXTURE_LOD_BIAS:
         *params = obj->Sampler.LodBias;
         break;

      case GL_TEXTURE_IMMUTABLE_FORMAT:
         if (!ctx->Extensions.ARB_texture_storage)
            goto invalid_pname;
         *params = (GLfloat) obj->Immutable;
         break;

      default:
         goto invalid_pname;
   }

   /* no error if we get here */
   _mesa_unlock_texture(ctx, obj);
   return;

invalid_pname:
   _mesa_unlock_texture(ctx, obj);
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(pname=0x%x)", pname);
}


void GLAPIENTRY
_mesa_GetTexParameteriv( GLenum target, GLenum pname, GLint *params )
{
   struct gl_texture_object *obj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   obj = get_texobj(ctx, target, GL_TRUE);
   if (!obj)
      return;

   _mesa_lock_texture(ctx, obj);
   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
         *params = (GLint) obj->Sampler.MagFilter;
         break;;
      case GL_TEXTURE_MIN_FILTER:
         *params = (GLint) obj->Sampler.MinFilter;
         break;;
      case GL_TEXTURE_WRAP_S:
         *params = (GLint) obj->Sampler.WrapS;
         break;;
      case GL_TEXTURE_WRAP_T:
         *params = (GLint) obj->Sampler.WrapT;
         break;;
      case GL_TEXTURE_WRAP_R:
         *params = (GLint) obj->Sampler.WrapR;
         break;;
      case GL_TEXTURE_BORDER_COLOR:
         {
            GLfloat b[4];
            b[0] = CLAMP(obj->Sampler.BorderColor.f[0], 0.0F, 1.0F);
            b[1] = CLAMP(obj->Sampler.BorderColor.f[1], 0.0F, 1.0F);
            b[2] = CLAMP(obj->Sampler.BorderColor.f[2], 0.0F, 1.0F);
            b[3] = CLAMP(obj->Sampler.BorderColor.f[3], 0.0F, 1.0F);
            params[0] = FLOAT_TO_INT(b[0]);
            params[1] = FLOAT_TO_INT(b[1]);
            params[2] = FLOAT_TO_INT(b[2]);
            params[3] = FLOAT_TO_INT(b[3]);
         }
         break;;
      case GL_TEXTURE_RESIDENT:
         *params = 1;
         break;;
      case GL_TEXTURE_PRIORITY:
         *params = FLOAT_TO_INT(obj->Priority);
         break;;
      case GL_TEXTURE_MIN_LOD:
         *params = (GLint) obj->Sampler.MinLod;
         break;;
      case GL_TEXTURE_MAX_LOD:
         *params = (GLint) obj->Sampler.MaxLod;
         break;;
      case GL_TEXTURE_BASE_LEVEL:
         *params = obj->BaseLevel;
         break;;
      case GL_TEXTURE_MAX_LEVEL:
         *params = obj->MaxLevel;
         break;;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (!ctx->Extensions.EXT_texture_filter_anisotropic)
            goto invalid_pname;
         *params = (GLint) obj->Sampler.MaxAnisotropy;
         break;
      case GL_GENERATE_MIPMAP_SGIS:
	 *params = (GLint) obj->GenerateMipmap;
         break;
      case GL_TEXTURE_LOD_BIAS:
         *params = (GLint) obj->Sampler.LodBias;
         break;

      case GL_TEXTURE_IMMUTABLE_FORMAT:
         if (!ctx->Extensions.ARB_texture_storage)
            goto invalid_pname;
         *params = (GLint) obj->Immutable;
         break;

      default:
         goto invalid_pname;
   }

   /* no error if we get here */
   _mesa_unlock_texture(ctx, obj);
   return;

invalid_pname:
   _mesa_unlock_texture(ctx, obj);
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameteriv(pname=0x%x)", pname);
}


/** New in GL 3.0 */
void GLAPIENTRY
_mesa_GetTexParameterIiv(GLenum target, GLenum pname, GLint *params)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_TRUE);
   if (!texObj)
      return;
   
   switch (pname) {
   case GL_TEXTURE_BORDER_COLOR:
      COPY_4V(params, texObj->Sampler.BorderColor.i);
      break;
   default:
      _mesa_GetTexParameteriv(target, pname, params);
   }
}


/** New in GL 3.0 */
void GLAPIENTRY
_mesa_GetTexParameterIuiv(GLenum target, GLenum pname, GLuint *params)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target, GL_TRUE);
   if (!texObj)
      return;
   
   switch (pname) {
   case GL_TEXTURE_BORDER_COLOR:
      COPY_4V(params, texObj->Sampler.BorderColor.i);
      break;
   default:
      {
         GLint ip[4];
         _mesa_GetTexParameteriv(target, pname, ip);
         params[0] = ip[0];
         if (pname == GL_TEXTURE_SWIZZLE_RGBA_EXT || 
             pname == GL_TEXTURE_CROP_RECT_OES) {
            params[1] = ip[1];
            params[2] = ip[2];
            params[3] = ip[3];
         }
      }
   }
}
