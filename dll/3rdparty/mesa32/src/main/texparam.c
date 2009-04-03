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
#include "main/context.h"
#include "main/enums.h"
#include "main/colormac.h"
#include "main/macros.h"
#include "main/texcompress.h"
#include "main/texparam.h"
#include "main/teximage.h"


/**
 * Check if a coordinate wrap mode is supported for the texture target.
 * \return GL_TRUE if legal, GL_FALSE otherwise
 */
static GLboolean 
validate_texture_wrap_mode(GLcontext * ctx, GLenum target, GLenum wrap)
{
   const struct gl_extensions * const e = & ctx->Extensions;

   if (wrap == GL_CLAMP || wrap == GL_CLAMP_TO_EDGE ||
       (wrap == GL_CLAMP_TO_BORDER && e->ARB_texture_border_clamp)) {
      /* any texture target */
      return GL_TRUE;
   }
   else if (target != GL_TEXTURE_RECTANGLE_NV &&
	    (wrap == GL_REPEAT ||
	     (wrap == GL_MIRRORED_REPEAT &&
	      e->ARB_texture_mirrored_repeat) ||
	     (wrap == GL_MIRROR_CLAMP_EXT &&
	      (e->ATI_texture_mirror_once || e->EXT_texture_mirror_clamp)) ||
	     (wrap == GL_MIRROR_CLAMP_TO_EDGE_EXT &&
	      (e->ATI_texture_mirror_once || e->EXT_texture_mirror_clamp)) ||
	     (wrap == GL_MIRROR_CLAMP_TO_BORDER_EXT &&
	      (e->EXT_texture_mirror_clamp)))) {
      /* non-rectangle texture */
      return GL_TRUE;
   }

   _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
   return GL_FALSE;
}


/**
 * Get current texture object for given target.
 * Return NULL if any error.
 */
static struct gl_texture_object *
get_texobj(GLcontext *ctx, GLenum target)
{
   struct gl_texture_unit *texUnit;

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexParameter(current unit)");
      return NULL;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

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
   case GL_TEXTURE_RECTANGLE_NV:
      if (ctx->Extensions.NV_texture_rectangle) {
         return texUnit->CurrentTex[TEXTURE_RECT_INDEX];
      }
      break;
   case GL_TEXTURE_1D_ARRAY_EXT:
      if (ctx->Extensions.MESA_texture_array) {
         return texUnit->CurrentTex[TEXTURE_1D_ARRAY_INDEX];
      }
      break;
   case GL_TEXTURE_2D_ARRAY_EXT:
      if (ctx->Extensions.MESA_texture_array) {
         return texUnit->CurrentTex[TEXTURE_2D_ARRAY_INDEX];
      }
      break;
   default:
      ;
   }

   _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(target)");
   return NULL;
}


/**
 * This is called just prior to changing any texture object state.
 * Any pending rendering will be flushed out, we'll set the _NEW_TEXTURE
 * state flag and then mark the texture object as 'incomplete' so that any
 * per-texture derived state gets recomputed.
 */
static INLINE void
flush(GLcontext *ctx, struct gl_texture_object *texObj)
{
   FLUSH_VERTICES(ctx, _NEW_TEXTURE);
   texObj->_Complete = GL_FALSE;
}


/** Set an integer-valued texture parameter */
static void
set_tex_parameteri(GLcontext *ctx,
                   struct gl_texture_object *texObj,
                   GLenum pname, const GLint *params)
{
   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
      if (texObj->MinFilter == params[0])
         return;
      switch (params[0]) {
      case GL_NEAREST:
      case GL_LINEAR:
         flush(ctx, texObj);
         texObj->MinFilter = params[0];
         return;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
         if (texObj->Target != GL_TEXTURE_RECTANGLE_NV) {
            flush(ctx, texObj);
            texObj->MinFilter = params[0];
            return;
         }
         /* fall-through */
      default:
         _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
      }
      return;

   case GL_TEXTURE_MAG_FILTER:
      if (texObj->MagFilter == params[0])
         return;
      switch (params[0]) {
      case GL_NEAREST:
      case GL_LINEAR:
         flush(ctx, texObj);
         texObj->MagFilter = params[0];
         return;
      default:
         _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
      }
      return;

   case GL_TEXTURE_WRAP_S:
      if (texObj->WrapS == params[0])
         return;
      if (validate_texture_wrap_mode(ctx, texObj->Target, params[0])) {
         flush(ctx, texObj);
         texObj->WrapS = params[0];
      }
      return;

   case GL_TEXTURE_WRAP_T:
      if (texObj->WrapT == params[0])
         return;
      if (validate_texture_wrap_mode(ctx, texObj->Target, params[0])) {
         flush(ctx, texObj);
         texObj->WrapT = params[0];
      }
      return;

   case GL_TEXTURE_WRAP_R:
      if (texObj->WrapR == params[0])
         return;
      if (validate_texture_wrap_mode(ctx, texObj->Target, params[0])) {
         flush(ctx, texObj);
         texObj->WrapR = params[0];
      }
      return;

   case GL_TEXTURE_BASE_LEVEL:
      if (texObj->BaseLevel == params[0])
         return;
      if (params[0] < 0 ||
          (texObj->Target == GL_TEXTURE_RECTANGLE_ARB && params[0] != 0)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)");
         return;
      }
      flush(ctx, texObj);
      texObj->BaseLevel = params[0];
      return;

   case GL_TEXTURE_MAX_LEVEL:
      if (texObj->MaxLevel == params[0])
         return;
      if (params[0] < 0 || texObj->Target == GL_TEXTURE_RECTANGLE_ARB) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "glTexParameter(param)");
         return;
      }
      flush(ctx, texObj);
      texObj->MaxLevel = params[0];
      return;

   case GL_TEXTURE_COMPARE_SGIX:
      if (ctx->Extensions.SGIX_shadow) {
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->CompareFlag = params[0] ? GL_TRUE : GL_FALSE;
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(pname=GL_TEXTURE_COMPARE_SGIX)");
      }
      return;

   case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
      if (ctx->Extensions.SGIX_shadow &&
          (params[0] == GL_TEXTURE_LEQUAL_R_SGIX || 
           params[0] == GL_TEXTURE_GEQUAL_R_SGIX)) {
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->CompareOperator = params[0];
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(GL_TEXTURE_COMPARE_OPERATOR_SGIX)");
      }
      return;

   case GL_GENERATE_MIPMAP_SGIS:
      if (ctx->Extensions.SGIS_generate_mipmap) {
         if (texObj->GenerateMipmap != params[0]) {
            flush(ctx, texObj);
            texObj->GenerateMipmap = params[0] ? GL_TRUE : GL_FALSE;
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(pname=GL_GENERATE_MIPMAP_SGIS)");
      }
      return;

   case GL_TEXTURE_COMPARE_MODE_ARB:
      if (ctx->Extensions.ARB_shadow &&
          (params[0] == GL_NONE ||
           params[0] == GL_COMPARE_R_TO_TEXTURE_ARB)) {
         if (texObj->CompareMode != params[0]) {
            flush(ctx, texObj);
            texObj->CompareMode = params[0];
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(GL_TEXTURE_COMPARE_MODE_ARB)");
      }
      return;

   case GL_TEXTURE_COMPARE_FUNC_ARB:
      if (ctx->Extensions.ARB_shadow) {
         if (texObj->CompareFunc == params[0])
            return;
         switch (params[0]) {
         case GL_LEQUAL:
         case GL_GEQUAL:
            flush(ctx, texObj);
            texObj->CompareFunc = params[0];
            return;
         case GL_EQUAL:
         case GL_NOTEQUAL:
         case GL_LESS:
         case GL_GREATER:
         case GL_ALWAYS:
         case GL_NEVER:
            if (ctx->Extensions.EXT_shadow_funcs) {
               flush(ctx, texObj);
               texObj->CompareFunc = params[0];
               return;
            }
            /* fall-through */
         default:
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(GL_TEXTURE_COMPARE_FUNC_ARB)");
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(param)");
      }
      return;

   case GL_DEPTH_TEXTURE_MODE_ARB:
      if (ctx->Extensions.ARB_depth_texture &&
          (params[0] == GL_LUMINANCE ||
           params[0] == GL_INTENSITY ||
           params[0] == GL_ALPHA)) {
         if (texObj->DepthMode != params[0]) {
            flush(ctx, texObj);
            texObj->DepthMode = params[0];
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(GL_DEPTH_TEXTURE_MODE_ARB)");
      }
      return;

#ifdef FEATURE_OES_draw_texture
   case GL_TEXTURE_CROP_RECT_OES:
      texObj->CropRect[0] = params[0];
      texObj->CropRect[1] = params[1];
      texObj->CropRect[2] = params[2];
      texObj->CropRect[3] = params[3];
      break;
#endif

   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(pname=0x%x)", pname);
   }
}


/** Set a float-valued texture parameter */
static void
set_tex_parameterf(GLcontext *ctx,
                   struct gl_texture_object *texObj,
                   GLenum pname, const GLfloat *params)
{
   switch (pname) {
   case GL_TEXTURE_MIN_LOD:
      if (texObj->MinLod == params[0])
         return;
      flush(ctx, texObj);
      texObj->MinLod = params[0];
      return;

   case GL_TEXTURE_MAX_LOD:
      if (texObj->MaxLod == params[0])
         return;
      flush(ctx, texObj);
      texObj->MaxLod = params[0];
      return;

   case GL_TEXTURE_PRIORITY:
      flush(ctx, texObj);
      texObj->Priority = CLAMP(params[0], 0.0F, 1.0F);
      return;

   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      if (ctx->Extensions.EXT_texture_filter_anisotropic) {
         if (texObj->MaxAnisotropy == params[0])
            return;
         if (params[0] < 1.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         flush(ctx, texObj);
         /* clamp to max, that's what NVIDIA does */
         texObj->MaxAnisotropy = MIN2(params[0],
                                      ctx->Const.MaxTextureMaxAnisotropy);
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(pname=GL_TEXTURE_MAX_ANISOTROPY_EXT)");
      }
      return;

   case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
      if (ctx->Extensions.SGIX_shadow_ambient) {
         if (texObj->ShadowAmbient != params[0]) {
            flush(ctx, texObj);
            texObj->ShadowAmbient = CLAMP(params[0], 0.0F, 1.0F);
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(pname=GL_SHADOW_AMBIENT_SGIX)");
      }
      return;

   case GL_TEXTURE_LOD_BIAS:
      /* NOTE: this is really part of OpenGL 1.4, not EXT_texture_lod_bias */
      if (ctx->Extensions.EXT_texture_lod_bias) {
         if (texObj->LodBias != params[0]) {
            flush(ctx, texObj);
            texObj->LodBias = params[0];
         }
      }
      break;

   case GL_TEXTURE_BORDER_COLOR:
      flush(ctx, texObj);
      texObj->BorderColor[RCOMP] = params[0];
      texObj->BorderColor[GCOMP] = params[1];
      texObj->BorderColor[BCOMP] = params[2];
      texObj->BorderColor[ACOMP] = params[3];
      UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[RCOMP], params[0]);
      UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[GCOMP], params[1]);
      UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[BCOMP], params[2]);
      UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[ACOMP], params[3]);
      return;

   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(pname=0x%x)", pname);
   }
}


void GLAPIENTRY
_mesa_TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target);
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
   case GL_TEXTURE_COMPARE_SGIX:
   case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
   case GL_GENERATE_MIPMAP_SGIS:
   case GL_TEXTURE_COMPARE_MODE_ARB:
   case GL_TEXTURE_COMPARE_FUNC_ARB:
   case GL_DEPTH_TEXTURE_MODE_ARB:
      {
         /* convert float param to int */
         GLint p = (GLint) param;
         set_tex_parameteri(ctx, texObj, pname, &p);
      }
      return;
   default:
      /* this will generate an error if pname is illegal */
      set_tex_parameterf(ctx, texObj, pname, &param);
   }

   if (ctx->Driver.TexParameter && ctx->ErrorValue == GL_NO_ERROR) {
      ctx->Driver.TexParameter(ctx, target, texObj, pname, &param);
   }
}


void GLAPIENTRY
_mesa_TexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target);
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
   case GL_TEXTURE_COMPARE_SGIX:
   case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
   case GL_GENERATE_MIPMAP_SGIS:
   case GL_TEXTURE_COMPARE_MODE_ARB:
   case GL_TEXTURE_COMPARE_FUNC_ARB:
   case GL_DEPTH_TEXTURE_MODE_ARB:
      {
         /* convert float param to int */
         GLint p = (GLint) params[0];
         set_tex_parameteri(ctx, texObj, pname, &p);
      }
      break;

#ifdef FEATURE_OES_draw_texture
   case GL_TEXTURE_CROP_RECT_OES:
      {
         /* convert float params to int */
         GLint iparams[4];
         iparams[0] = (GLint) params[0];
         iparams[1] = (GLint) params[1];
         iparams[2] = (GLint) params[2];
         iparams[3] = (GLint) params[3];
         set_tex_parameteri(ctx, target, iparams);
      }
      break;
#endif

   default:
      /* this will generate an error if pname is illegal */
      set_tex_parameterf(ctx, texObj, pname, params);
   }

   if (ctx->Driver.TexParameter && ctx->ErrorValue == GL_NO_ERROR) {
      ctx->Driver.TexParameter(ctx, target, texObj, pname, params);
   }
}


void GLAPIENTRY
_mesa_TexParameteri(GLenum target, GLenum pname, GLint param)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target);
   if (!texObj)
      return;

   switch (pname) {
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
   case GL_TEXTURE_PRIORITY:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
   case GL_TEXTURE_LOD_BIAS:
   case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
      {
         GLfloat fparam = (GLfloat) param;
         /* convert int param to float */
         set_tex_parameterf(ctx, texObj, pname, &fparam);
      }
      break;
   default:
      /* this will generate an error if pname is illegal */
      set_tex_parameteri(ctx, texObj, pname, &param);
   }

   if (ctx->Driver.TexParameter && ctx->ErrorValue == GL_NO_ERROR) {
      GLfloat fparam = (GLfloat) param;
      ctx->Driver.TexParameter(ctx, target, texObj, pname, &fparam);
   }
}


void GLAPIENTRY
_mesa_TexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   texObj = get_texobj(ctx, target);
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
         set_tex_parameterf(ctx, texObj, pname, fparams);
      }
      break;
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
   case GL_TEXTURE_PRIORITY:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
   case GL_TEXTURE_LOD_BIAS:
   case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
      {
         /* convert int param to float */
         GLfloat fparam = (GLfloat) params[0];
         set_tex_parameterf(ctx, texObj, pname, &fparam);
      }
      break;
   default:
      /* this will generate an error if pname is illegal */
      set_tex_parameteri(ctx, texObj, pname, params);
   }

   if (ctx->Driver.TexParameter && ctx->ErrorValue == GL_NO_ERROR) {
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


void GLAPIENTRY
_mesa_GetTexLevelParameterfv( GLenum target, GLint level,
                              GLenum pname, GLfloat *params )
{
   GLint iparam;
   _mesa_GetTexLevelParameteriv( target, level, pname, &iparam );
   *params = (GLfloat) iparam;
}


static GLuint
tex_image_dimensions(GLcontext *ctx, GLenum target)
{
   switch (target) {
      case GL_TEXTURE_1D:
      case GL_PROXY_TEXTURE_1D:
         return 1;
      case GL_TEXTURE_2D:
      case GL_PROXY_TEXTURE_2D:
         return 2;
      case GL_TEXTURE_3D:
      case GL_PROXY_TEXTURE_3D:
         return 3;
      case GL_TEXTURE_CUBE_MAP:
      case GL_PROXY_TEXTURE_CUBE_MAP:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         return ctx->Extensions.ARB_texture_cube_map ? 2 : 0;
      case GL_TEXTURE_RECTANGLE_NV:
      case GL_PROXY_TEXTURE_RECTANGLE_NV:
         return ctx->Extensions.NV_texture_rectangle ? 2 : 0;
      case GL_TEXTURE_1D_ARRAY_EXT:
      case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
         return ctx->Extensions.MESA_texture_array ? 2 : 0;
      case GL_TEXTURE_2D_ARRAY_EXT:
      case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
         return ctx->Extensions.MESA_texture_array ? 3 : 0;
      default:
         _mesa_problem(ctx, "bad target in _mesa_tex_target_dimensions()");
         return 0;
   }
}


void GLAPIENTRY
_mesa_GetTexLevelParameteriv( GLenum target, GLint level,
                              GLenum pname, GLint *params )
{
   const struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   const struct gl_texture_image *img = NULL;
   GLuint dimensions;
   GLboolean isProxy;
   GLint maxLevels;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetTexLevelParameteriv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   /* this will catch bad target values */
   dimensions = tex_image_dimensions(ctx, target);  /* 1, 2 or 3 */
   if (dimensions == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(target)");
      return;
   }

   maxLevels = _mesa_max_texture_levels(ctx, target);
   if (maxLevels == 0) {
      /* should not happen since <target> was just checked above */
      _mesa_problem(ctx, "maxLevels=0 in _mesa_GetTexLevelParameter");
      return;
   }

   if (level < 0 || level >= maxLevels) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGetTexLevelParameter[if]v" );
      return;
   }

   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   _mesa_lock_texture(ctx, texObj);

   img = _mesa_select_tex_image(ctx, texObj, target, level);
   if (!img || !img->TexFormat) {
      /* undefined texture image */
      if (pname == GL_TEXTURE_COMPONENTS)
         *params = 1;
      else
         *params = 0;
      goto out;
   }

   isProxy = _mesa_is_proxy_texture(target);

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
         if (img->_BaseFormat == GL_RGB || img->_BaseFormat == GL_RGBA)
            *params = img->TexFormat->RedBits;
         else
            *params = 0;
         break;
      case GL_TEXTURE_GREEN_SIZE:
         if (img->_BaseFormat == GL_RGB || img->_BaseFormat == GL_RGBA)
            *params = img->TexFormat->GreenBits;
         else
            *params = 0;
         break;
      case GL_TEXTURE_BLUE_SIZE:
         if (img->_BaseFormat == GL_RGB || img->_BaseFormat == GL_RGBA)
            *params = img->TexFormat->BlueBits;
         else
            *params = 0;
         break;
      case GL_TEXTURE_ALPHA_SIZE:
         if (img->_BaseFormat == GL_ALPHA ||
             img->_BaseFormat == GL_LUMINANCE_ALPHA ||
             img->_BaseFormat == GL_RGBA)
            *params = img->TexFormat->AlphaBits;
         else
            *params = 0;
         break;
      case GL_TEXTURE_INTENSITY_SIZE:
         if (img->_BaseFormat != GL_INTENSITY)
            *params = 0;
         else if (img->TexFormat->IntensityBits > 0)
            *params = img->TexFormat->IntensityBits;
         else /* intensity probably stored as rgb texture */
            *params = MIN2(img->TexFormat->RedBits, img->TexFormat->GreenBits);
         break;
      case GL_TEXTURE_LUMINANCE_SIZE:
         if (img->_BaseFormat != GL_LUMINANCE &&
             img->_BaseFormat != GL_LUMINANCE_ALPHA)
            *params = 0;
         else if (img->TexFormat->LuminanceBits > 0)
            *params = img->TexFormat->LuminanceBits;
         else /* luminance probably stored as rgb texture */
            *params = MIN2(img->TexFormat->RedBits, img->TexFormat->GreenBits);
         break;
      case GL_TEXTURE_INDEX_SIZE_EXT:
         if (img->_BaseFormat == GL_COLOR_INDEX)
            *params = img->TexFormat->IndexBits;
         else
            *params = 0;
         break;
      case GL_TEXTURE_DEPTH_SIZE_ARB:
         if (ctx->Extensions.ARB_depth_texture)
            *params = img->TexFormat->DepthBits;
         else
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         break;
      case GL_TEXTURE_STENCIL_SIZE_EXT:
         if (ctx->Extensions.EXT_packed_depth_stencil) {
            *params = img->TexFormat->StencilBits;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSED_IMAGE_SIZE:
         if (ctx->Extensions.ARB_texture_compression) {
            if (img->IsCompressed && !isProxy) {
               /* Don't use ctx->Driver.CompressedTextureSize() since that
                * may returned a padded hardware size.
                */
               *params = _mesa_compressed_texture_size(ctx, img->Width,
                                                   img->Height, img->Depth,
                                                   img->TexFormat->MesaFormat);
            }
            else {
               _mesa_error(ctx, GL_INVALID_OPERATION,
                           "glGetTexLevelParameter[if]v(pname)");
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_COMPRESSED:
         if (ctx->Extensions.ARB_texture_compression) {
            *params = (GLint) img->IsCompressed;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;

      /* GL_ARB_texture_float */
      case GL_TEXTURE_RED_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->RedBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_GREEN_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->GreenBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_BLUE_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->BlueBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_ALPHA_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->AlphaBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_LUMINANCE_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->LuminanceBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_INTENSITY_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->IntensityBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_DEPTH_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->DepthBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;

      default:
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glGetTexLevelParameter[if]v(pname)");
   }

 out:
   _mesa_unlock_texture(ctx, texObj);
}



void GLAPIENTRY
_mesa_GetTexParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *obj;
   GLboolean error = GL_FALSE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetTexParameterfv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   obj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!obj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(target)");
      return;
   }

   _mesa_lock_texture(ctx, obj);
   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
	 *params = ENUM_TO_FLOAT(obj->MagFilter);
	 break;
      case GL_TEXTURE_MIN_FILTER:
         *params = ENUM_TO_FLOAT(obj->MinFilter);
         break;
      case GL_TEXTURE_WRAP_S:
         *params = ENUM_TO_FLOAT(obj->WrapS);
         break;
      case GL_TEXTURE_WRAP_T:
         *params = ENUM_TO_FLOAT(obj->WrapT);
         break;
      case GL_TEXTURE_WRAP_R:
         *params = ENUM_TO_FLOAT(obj->WrapR);
         break;
      case GL_TEXTURE_BORDER_COLOR:
         params[0] = CLAMP(obj->BorderColor[0], 0.0F, 1.0F);
         params[1] = CLAMP(obj->BorderColor[1], 0.0F, 1.0F);
         params[2] = CLAMP(obj->BorderColor[2], 0.0F, 1.0F);
         params[3] = CLAMP(obj->BorderColor[3], 0.0F, 1.0F);
         break;
      case GL_TEXTURE_RESIDENT:
         {
            GLboolean resident;
            if (ctx->Driver.IsTextureResident)
               resident = ctx->Driver.IsTextureResident(ctx, obj);
            else
               resident = GL_TRUE;
            *params = ENUM_TO_FLOAT(resident);
         }
         break;
      case GL_TEXTURE_PRIORITY:
         *params = obj->Priority;
         break;
      case GL_TEXTURE_MIN_LOD:
         *params = obj->MinLod;
         break;
      case GL_TEXTURE_MAX_LOD:
         *params = obj->MaxLod;
         break;
      case GL_TEXTURE_BASE_LEVEL:
         *params = (GLfloat) obj->BaseLevel;
         break;
      case GL_TEXTURE_MAX_LEVEL:
         *params = (GLfloat) obj->MaxLevel;
         break;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
            *params = obj->MaxAnisotropy;
         }
	 else
	    error = 1;
         break;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLfloat) obj->CompareFlag;
         }
	 else 
	    error = 1;
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLfloat) obj->CompareOperator;
         }
	 else 
	    error = 1;
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            *params = obj->ShadowAmbient;
         }
	 else 
	    error = 1;
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            *params = (GLfloat) obj->GenerateMipmap;
         }
	 else 
	    error = 1;
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLfloat) obj->CompareMode;
         }
	 else 
	    error = 1;
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLfloat) obj->CompareFunc;
         }
	 else 
	    error = 1;
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            *params = (GLfloat) obj->DepthMode;
         }
	 else 
	    error = 1;
         break;
      case GL_TEXTURE_LOD_BIAS:
         if (ctx->Extensions.EXT_texture_lod_bias) {
            *params = obj->LodBias;
         }
	 else 
	    error = 1;
         break;
#ifdef FEATURE_OES_draw_texture
      case GL_TEXTURE_CROP_RECT_OES:
         params[0] = obj->CropRect[0];
         params[1] = obj->CropRect[1];
         params[2] = obj->CropRect[2];
         params[3] = obj->CropRect[3];
         break;
#endif
      default:
	 error = 1;
	 break;
   }
   if (error)
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(pname=0x%x)",
		  pname);

   _mesa_unlock_texture(ctx, obj);
}


void GLAPIENTRY
_mesa_GetTexParameteriv( GLenum target, GLenum pname, GLint *params )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *obj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetTexParameteriv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   obj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!obj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameteriv(target)");
      return;
   }

   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
         *params = (GLint) obj->MagFilter;
         return;
      case GL_TEXTURE_MIN_FILTER:
         *params = (GLint) obj->MinFilter;
         return;
      case GL_TEXTURE_WRAP_S:
         *params = (GLint) obj->WrapS;
         return;
      case GL_TEXTURE_WRAP_T:
         *params = (GLint) obj->WrapT;
         return;
      case GL_TEXTURE_WRAP_R:
         *params = (GLint) obj->WrapR;
         return;
      case GL_TEXTURE_BORDER_COLOR:
         {
            GLfloat b[4];
            b[0] = CLAMP(obj->BorderColor[0], 0.0F, 1.0F);
            b[1] = CLAMP(obj->BorderColor[1], 0.0F, 1.0F);
            b[2] = CLAMP(obj->BorderColor[2], 0.0F, 1.0F);
            b[3] = CLAMP(obj->BorderColor[3], 0.0F, 1.0F);
            params[0] = FLOAT_TO_INT(b[0]);
            params[1] = FLOAT_TO_INT(b[1]);
            params[2] = FLOAT_TO_INT(b[2]);
            params[3] = FLOAT_TO_INT(b[3]);
         }
         return;
      case GL_TEXTURE_RESIDENT:
         {
            GLboolean resident;
            if (ctx->Driver.IsTextureResident)
               resident = ctx->Driver.IsTextureResident(ctx, obj);
            else
               resident = GL_TRUE;
            *params = (GLint) resident;
         }
         return;
      case GL_TEXTURE_PRIORITY:
         *params = FLOAT_TO_INT(obj->Priority);
         return;
      case GL_TEXTURE_MIN_LOD:
         *params = (GLint) obj->MinLod;
         return;
      case GL_TEXTURE_MAX_LOD:
         *params = (GLint) obj->MaxLod;
         return;
      case GL_TEXTURE_BASE_LEVEL:
         *params = obj->BaseLevel;
         return;
      case GL_TEXTURE_MAX_LEVEL:
         *params = obj->MaxLevel;
         return;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
            *params = (GLint) obj->MaxAnisotropy;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLint) obj->CompareFlag;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLint) obj->CompareOperator;
            return;
         }
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            *params = (GLint) FLOAT_TO_INT(obj->ShadowAmbient);
            return;
         }
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            *params = (GLint) obj->GenerateMipmap;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLint) obj->CompareMode;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLint) obj->CompareFunc;
            return;
         }
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            *params = (GLint) obj->DepthMode;
            return;
         }
         break;
      case GL_TEXTURE_LOD_BIAS:
         if (ctx->Extensions.EXT_texture_lod_bias) {
            *params = (GLint) obj->LodBias;
            return;
         }
         break;
#ifdef FEATURE_OES_draw_texture
      case GL_TEXTURE_CROP_RECT_OES:
         params[0] = obj->CropRect[0];
         params[1] = obj->CropRect[1];
         params[2] = obj->CropRect[2];
         params[3] = obj->CropRect[3];
         break;
#endif
      default:
         ; /* silence warnings */
   }
   /* If we get here, pname was an unrecognized enum */
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameteriv(pname=0x%x)", pname);
}
