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


void GLAPIENTRY
_mesa_TexParameterf( GLenum target, GLenum pname, GLfloat param )
{
   _mesa_TexParameterfv(target, pname, &param);
}


void GLAPIENTRY
_mesa_TexParameterfv( GLenum target, GLenum pname, const GLfloat *params )
{
   const GLenum eparam = (GLenum) (GLint) params[0];
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexParameter %s %s %.1f(%s)...\n",
                  _mesa_lookup_enum_by_nr(target),
                  _mesa_lookup_enum_by_nr(pname),
                  *params,
		  _mesa_lookup_enum_by_nr(eparam));

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexParameterfv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->Current1D;
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->Current2D;
         break;
      case GL_TEXTURE_3D:
         texObj = texUnit->Current3D;
         break;
      case GL_TEXTURE_CUBE_MAP:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
            return;
         }
         texObj = texUnit->CurrentCubeMap;
         break;
      case GL_TEXTURE_RECTANGLE_NV:
         if (!ctx->Extensions.NV_texture_rectangle) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
            return;
         }
         texObj = texUnit->CurrentRect;
         break;
      case GL_TEXTURE_1D_ARRAY_EXT:
         if (!ctx->Extensions.MESA_texture_array) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
            return;
         }
         texObj = texUnit->Current1DArray;
         break;
      case GL_TEXTURE_2D_ARRAY_EXT:
         if (!ctx->Extensions.MESA_texture_array) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
            return;
         }
         texObj = texUnit->Current2DArray;
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
         return;
   }

   switch (pname) {
      case GL_TEXTURE_MIN_FILTER:
         /* A small optimization */
         if (texObj->MinFilter == eparam)
            return;
         if (eparam==GL_NEAREST || eparam==GL_LINEAR) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MinFilter = eparam;
         }
         else if ((eparam==GL_NEAREST_MIPMAP_NEAREST ||
                   eparam==GL_LINEAR_MIPMAP_NEAREST ||
                   eparam==GL_NEAREST_MIPMAP_LINEAR ||
                   eparam==GL_LINEAR_MIPMAP_LINEAR) &&
                  texObj->Target != GL_TEXTURE_RECTANGLE_NV) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MinFilter = eparam;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_MAG_FILTER:
         /* A small optimization */
         if (texObj->MagFilter == eparam)
            return;

         if (eparam==GL_NEAREST || eparam==GL_LINEAR) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MagFilter = eparam;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_S:
         if (texObj->WrapS == eparam)
            return;
         if (validate_texture_wrap_mode(ctx, texObj->Target, eparam)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapS = eparam;
         }
         else {
            return;
         }
         break;
      case GL_TEXTURE_WRAP_T:
         if (texObj->WrapT == eparam)
            return;
         if (validate_texture_wrap_mode(ctx, texObj->Target, eparam)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapT = eparam;
         }
         else {
            return;
         }
         break;
      case GL_TEXTURE_WRAP_R:
         if (texObj->WrapR == eparam)
            return;
         if (validate_texture_wrap_mode(ctx, texObj->Target, eparam)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapR = eparam;
         }
         else {
	    return;
         }
         break;
      case GL_TEXTURE_BORDER_COLOR:
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->BorderColor[RCOMP] = params[0];
         texObj->BorderColor[GCOMP] = params[1];
         texObj->BorderColor[BCOMP] = params[2];
         texObj->BorderColor[ACOMP] = params[3];
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[RCOMP], params[0]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[GCOMP], params[1]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[BCOMP], params[2]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[ACOMP], params[3]);
         break;
      case GL_TEXTURE_MIN_LOD:
         if (texObj->MinLod == params[0])
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MinLod = params[0];
         break;
      case GL_TEXTURE_MAX_LOD:
         if (texObj->MaxLod == params[0])
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MaxLod = params[0];
         break;
      case GL_TEXTURE_BASE_LEVEL:
         if (params[0] < 0.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)");
            return;
         }
         if (target == GL_TEXTURE_RECTANGLE_ARB && params[0] != 0.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)");
            return;
         }
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->BaseLevel = (GLint) params[0];
         break;
      case GL_TEXTURE_MAX_LEVEL:
         if (params[0] < 0.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)");
            return;
         }
         if (target == GL_TEXTURE_RECTANGLE_ARB) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glTexParameter(param)");
            return;
         }
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MaxLevel = (GLint) params[0];
         break;
      case GL_TEXTURE_PRIORITY:
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->Priority = CLAMP( params[0], 0.0F, 1.0F );
         break;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
	    if (params[0] < 1.0) {
	       _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
	       return;
	    }
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            /* clamp to max, that's what NVIDIA does */
            texObj->MaxAnisotropy = MIN2(params[0],
                                         ctx->Const.MaxTextureMaxAnisotropy);
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_MAX_ANISOTROPY_EXT)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->CompareFlag = params[0] ? GL_TRUE : GL_FALSE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_SGIX)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            GLenum op = (GLenum) params[0];
            if (op == GL_TEXTURE_LEQUAL_R_SGIX ||
                op == GL_TEXTURE_GEQUAL_R_SGIX) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareOperator = op;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(param)");
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                    "glTexParameter(pname=GL_TEXTURE_COMPARE_OPERATOR_SGIX)");
            return;
         }
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->ShadowAmbient = CLAMP(params[0], 0.0F, 1.0F);
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_SHADOW_AMBIENT_SGIX)");
            return;
         }
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->GenerateMipmap = params[0] ? GL_TRUE : GL_FALSE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_GENERATE_MIPMAP_SGIS)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            const GLenum mode = (GLenum) params[0];
            if (mode == GL_NONE || mode == GL_COMPARE_R_TO_TEXTURE_ARB) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareMode = mode;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                           "glTexParameter(bad GL_TEXTURE_COMPARE_MODE_ARB: 0x%x)", mode);
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_MODE_ARB)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            const GLenum func = (GLenum) params[0];
            if (func == GL_LEQUAL || func == GL_GEQUAL) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareFunc = func;
            }
            else if (ctx->Extensions.EXT_shadow_funcs &&
                     (func == GL_EQUAL ||
                      func == GL_NOTEQUAL ||
                      func == GL_LESS ||
                      func == GL_GREATER ||
                      func == GL_ALWAYS ||
                      func == GL_NEVER)) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareFunc = func;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                           "glTexParameter(bad GL_TEXTURE_COMPARE_FUNC_ARB)");
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_FUNC_ARB)");
            return;
         }
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            const GLenum result = (GLenum) params[0];
            if (result == GL_LUMINANCE || result == GL_INTENSITY
                || result == GL_ALPHA) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->DepthMode = result;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                          "glTexParameter(bad GL_DEPTH_TEXTURE_MODE_ARB)");
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_DEPTH_TEXTURE_MODE_ARB)");
            return;
         }
         break;
      case GL_TEXTURE_LOD_BIAS:
         /* NOTE: this is really part of OpenGL 1.4, not EXT_texture_lod_bias*/
         if (ctx->Extensions.EXT_texture_lod_bias) {
            if (texObj->LodBias != params[0]) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->LodBias = params[0];
            }
         }
         break;
#ifdef FEATURE_OES_draw_texture
      case GL_TEXTURE_CROP_RECT_OES:
         texObj->CropRect[0] = (GLint) params[0];
         texObj->CropRect[1] = (GLint) params[1];
         texObj->CropRect[2] = (GLint) params[2];
         texObj->CropRect[3] = (GLint) params[3];
         break;
#endif

      default:
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(pname=0x%x)", pname);
         return;
   }

   texObj->_Complete = GL_FALSE;

   if (ctx->Driver.TexParameter) {
      (*ctx->Driver.TexParameter)( ctx, target, texObj, pname, params );
   }
}


void GLAPIENTRY
_mesa_TexParameteri( GLenum target, GLenum pname, GLint param )
{
   GLfloat fparam[4];
   if (pname == GL_TEXTURE_PRIORITY)
      fparam[0] = INT_TO_FLOAT(param);
   else
      fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0;
   _mesa_TexParameterfv(target, pname, fparam);
}


void GLAPIENTRY
_mesa_TexParameteriv( GLenum target, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   if (pname == GL_TEXTURE_BORDER_COLOR) {
      fparam[0] = INT_TO_FLOAT(params[0]);
      fparam[1] = INT_TO_FLOAT(params[1]);
      fparam[2] = INT_TO_FLOAT(params[2]);
      fparam[3] = INT_TO_FLOAT(params[3]);
   }
   else if (pname == GL_TEXTURE_CROP_RECT_OES) {
      fparam[0] = (GLfloat) params[0];
      fparam[1] = (GLfloat) params[1];
      fparam[2] = (GLfloat) params[2];
      fparam[3] = (GLfloat) params[3];
   }
   else {
      if (pname == GL_TEXTURE_PRIORITY)
         fparam[0] = INT_TO_FLOAT(params[0]);
      else
         fparam[0] = (GLfloat) params[0];
      fparam[1] = fparam[2] = fparam[3] = 0.0F;
   }
   _mesa_TexParameterfv(target, pname, fparam);
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
