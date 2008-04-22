/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


/*
 * Image convolution functions.
 *
 * Notes: filter kernel elements are indexed by <n> and <m> as in
 * the GL spec.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "convolve.h"
#include "context.h"
#include "image.h"
#include "mtypes.h"
#include "pixel.h"
#include "state.h"


/*
 * Given an internalFormat token passed to glConvolutionFilter
 * or glSeparableFilter, return the corresponding base format.
 * Return -1 if invalid token.
 */
static GLint
base_filter_format( GLenum format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return GL_LUMINANCE_ALPHA;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return GL_INTENSITY;
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
      case 4:
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return GL_RGBA;
      default:
         return -1;  /* error */
   }
}


void GLAPIENTRY
_mesa_ConvolutionFilter1D(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
   GLint baseFormat;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (target != GL_CONVOLUTION_1D) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionFilter1D(target)");
      return;
   }

   baseFormat = base_filter_format(internalFormat);
   if (baseFormat < 0 || baseFormat == GL_COLOR_INDEX) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionFilter1D(internalFormat)");
      return;
   }

   if (width < 0 || width > MAX_CONVOLUTION_WIDTH) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glConvolutionFilter1D(width)");
      return;
   }

   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glConvolutionFilter1D(format or type)");
      return;
   }

   if (format == GL_COLOR_INDEX ||
       format == GL_STENCIL_INDEX ||
       format == GL_DEPTH_COMPONENT ||
       format == GL_INTENSITY ||
       type == GL_BITMAP) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionFilter1D(format or type)");
      return;
   }

   ctx->Convolution1D.Format = format;
   ctx->Convolution1D.InternalFormat = internalFormat;
   ctx->Convolution1D.Width = width;
   ctx->Convolution1D.Height = 1;

   if (ctx->Unpack.BufferObj->Name) {
      /* unpack filter from PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(1, &ctx->Unpack, width, 1, 1,
                                     format, type, image)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glConvolutionFilter1D(invalid PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                              GL_READ_ONLY_ARB,
                                              ctx->Unpack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glConvolutionFilter1D(PBO is mapped)");
         return;
      }
      image = ADD_POINTERS(buf, image);
   }
   else if (!image) {
      return;
   }

   _mesa_unpack_color_span_float(ctx, width, GL_RGBA,
                                 ctx->Convolution1D.Filter,
                                 format, type, image, &ctx->Unpack,
                                 0); /* transferOps */

   if (ctx->Unpack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              ctx->Unpack.BufferObj);
   }

   _mesa_scale_and_bias_rgba(width,
                             (GLfloat (*)[4]) ctx->Convolution1D.Filter,
                             ctx->Pixel.ConvolutionFilterScale[0][0],
                             ctx->Pixel.ConvolutionFilterScale[0][1],
                             ctx->Pixel.ConvolutionFilterScale[0][2],
                             ctx->Pixel.ConvolutionFilterScale[0][3],
                             ctx->Pixel.ConvolutionFilterBias[0][0],
                             ctx->Pixel.ConvolutionFilterBias[0][1],
                             ctx->Pixel.ConvolutionFilterBias[0][2],
                             ctx->Pixel.ConvolutionFilterBias[0][3]);

   ctx->NewState |= _NEW_PIXEL;
}


void GLAPIENTRY
_mesa_ConvolutionFilter2D(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
   GLint baseFormat;
   GLint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (target != GL_CONVOLUTION_2D) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionFilter2D(target)");
      return;
   }

   baseFormat = base_filter_format(internalFormat);
   if (baseFormat < 0 || baseFormat == GL_COLOR_INDEX) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionFilter2D(internalFormat)");
      return;
   }

   if (width < 0 || width > MAX_CONVOLUTION_WIDTH) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glConvolutionFilter2D(width)");
      return;
   }
   if (height < 0 || height > MAX_CONVOLUTION_HEIGHT) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glConvolutionFilter2D(height)");
      return;
   }

   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glConvolutionFilter2D(format or type)");
      return;
   }
   if (format == GL_COLOR_INDEX ||
       format == GL_STENCIL_INDEX ||
       format == GL_DEPTH_COMPONENT ||
       format == GL_INTENSITY ||
       type == GL_BITMAP) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionFilter2D(format or type)");
      return;
   }

   /* this should have been caught earlier */
   assert(_mesa_components_in_format(format));

   ctx->Convolution2D.Format = format;
   ctx->Convolution2D.InternalFormat = internalFormat;
   ctx->Convolution2D.Width = width;
   ctx->Convolution2D.Height = height;

   if (ctx->Unpack.BufferObj->Name) {
      /* unpack filter from PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(2, &ctx->Unpack, width, height, 1,
                                     format, type, image)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glConvolutionFilter2D(invalid PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                              GL_READ_ONLY_ARB,
                                              ctx->Unpack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glConvolutionFilter2D(PBO is mapped)");
         return;
      }
      image = ADD_POINTERS(buf, image);
   }
   else if (!image) {
      return;
   }

   /* Unpack filter image.  We always store filters in RGBA format. */
   for (i = 0; i < height; i++) {
      const GLvoid *src = _mesa_image_address2d(&ctx->Unpack, image, width,
                                                height, format, type, i, 0);
      GLfloat *dst = ctx->Convolution2D.Filter + i * width * 4;
      _mesa_unpack_color_span_float(ctx, width, GL_RGBA, dst,
                                    format, type, src, &ctx->Unpack,
                                    0); /* transferOps */
   }

   if (ctx->Unpack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              ctx->Unpack.BufferObj);
   }

   _mesa_scale_and_bias_rgba(width * height,
                             (GLfloat (*)[4]) ctx->Convolution2D.Filter,
                             ctx->Pixel.ConvolutionFilterScale[1][0],
                             ctx->Pixel.ConvolutionFilterScale[1][1],
                             ctx->Pixel.ConvolutionFilterScale[1][2],
                             ctx->Pixel.ConvolutionFilterScale[1][3],
                             ctx->Pixel.ConvolutionFilterBias[1][0],
                             ctx->Pixel.ConvolutionFilterBias[1][1],
                             ctx->Pixel.ConvolutionFilterBias[1][2],
                             ctx->Pixel.ConvolutionFilterBias[1][3]);

   ctx->NewState |= _NEW_PIXEL;
}


void GLAPIENTRY
_mesa_ConvolutionParameterf(GLenum target, GLenum pname, GLfloat param)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint c;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   switch (target) {
      case GL_CONVOLUTION_1D:
         c = 0;
         break;
      case GL_CONVOLUTION_2D:
         c = 1;
         break;
      case GL_SEPARABLE_2D:
         c = 2;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameterf(target)");
         return;
   }

   switch (pname) {
      case GL_CONVOLUTION_BORDER_MODE:
         if (param == (GLfloat) GL_REDUCE ||
             param == (GLfloat) GL_CONSTANT_BORDER ||
             param == (GLfloat) GL_REPLICATE_BORDER) {
            ctx->Pixel.ConvolutionBorderMode[c] = (GLenum) param;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameterf(params)");
            return;
         }
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameterf(pname)");
         return;
   }

   ctx->NewState |= _NEW_PIXEL;
}


void GLAPIENTRY
_mesa_ConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint c;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   switch (target) {
      case GL_CONVOLUTION_1D:
         c = 0;
         break;
      case GL_CONVOLUTION_2D:
         c = 1;
         break;
      case GL_SEPARABLE_2D:
         c = 2;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameterfv(target)");
         return;
   }

   switch (pname) {
      case GL_CONVOLUTION_BORDER_COLOR:
         COPY_4V(ctx->Pixel.ConvolutionBorderColor[c], params);
         break;
      case GL_CONVOLUTION_BORDER_MODE:
         if (params[0] == (GLfloat) GL_REDUCE ||
             params[0] == (GLfloat) GL_CONSTANT_BORDER ||
             params[0] == (GLfloat) GL_REPLICATE_BORDER) {
            ctx->Pixel.ConvolutionBorderMode[c] = (GLenum) params[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameterfv(params)");
            return;
         }
         break;
      case GL_CONVOLUTION_FILTER_SCALE:
         COPY_4V(ctx->Pixel.ConvolutionFilterScale[c], params);
         break;
      case GL_CONVOLUTION_FILTER_BIAS:
         COPY_4V(ctx->Pixel.ConvolutionFilterBias[c], params);
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameterfv(pname)");
         return;
   }

   ctx->NewState |= _NEW_PIXEL;
}


void GLAPIENTRY
_mesa_ConvolutionParameteri(GLenum target, GLenum pname, GLint param)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint c;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   switch (target) {
      case GL_CONVOLUTION_1D:
         c = 0;
         break;
      case GL_CONVOLUTION_2D:
         c = 1;
         break;
      case GL_SEPARABLE_2D:
         c = 2;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameteri(target)");
         return;
   }

   switch (pname) {
      case GL_CONVOLUTION_BORDER_MODE:
         if (param == (GLint) GL_REDUCE ||
             param == (GLint) GL_CONSTANT_BORDER ||
             param == (GLint) GL_REPLICATE_BORDER) {
            ctx->Pixel.ConvolutionBorderMode[c] = (GLenum) param;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameteri(params)");
            return;
         }
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameteri(pname)");
         return;
   }

   ctx->NewState |= _NEW_PIXEL;
}


void GLAPIENTRY
_mesa_ConvolutionParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint c;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   switch (target) {
      case GL_CONVOLUTION_1D:
         c = 0;
         break;
      case GL_CONVOLUTION_2D:
         c = 1;
         break;
      case GL_SEPARABLE_2D:
         c = 2;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameteriv(target)");
         return;
   }

   switch (pname) {
      case GL_CONVOLUTION_BORDER_COLOR:
	 ctx->Pixel.ConvolutionBorderColor[c][0] = INT_TO_FLOAT(params[0]);
	 ctx->Pixel.ConvolutionBorderColor[c][1] = INT_TO_FLOAT(params[1]);
	 ctx->Pixel.ConvolutionBorderColor[c][2] = INT_TO_FLOAT(params[2]);
	 ctx->Pixel.ConvolutionBorderColor[c][3] = INT_TO_FLOAT(params[3]);
         break;
      case GL_CONVOLUTION_BORDER_MODE:
         if (params[0] == (GLint) GL_REDUCE ||
             params[0] == (GLint) GL_CONSTANT_BORDER ||
             params[0] == (GLint) GL_REPLICATE_BORDER) {
            ctx->Pixel.ConvolutionBorderMode[c] = (GLenum) params[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameteriv(params)");
            return;
         }
         break;
      case GL_CONVOLUTION_FILTER_SCALE:
	 /* COPY_4V(ctx->Pixel.ConvolutionFilterScale[c], params); */
	 /* need cast to prevent compiler warnings */  
	 ctx->Pixel.ConvolutionFilterScale[c][0] = (GLfloat) params[0]; 
	 ctx->Pixel.ConvolutionFilterScale[c][1] = (GLfloat) params[1]; 
	 ctx->Pixel.ConvolutionFilterScale[c][2] = (GLfloat) params[2]; 
	 ctx->Pixel.ConvolutionFilterScale[c][3] = (GLfloat) params[3]; 
         break;
      case GL_CONVOLUTION_FILTER_BIAS:
	 /* COPY_4V(ctx->Pixel.ConvolutionFilterBias[c], params); */
	 /* need cast to prevent compiler warnings */  
	 ctx->Pixel.ConvolutionFilterBias[c][0] = (GLfloat) params[0]; 
	 ctx->Pixel.ConvolutionFilterBias[c][1] = (GLfloat) params[1]; 
	 ctx->Pixel.ConvolutionFilterBias[c][2] = (GLfloat) params[2]; 
	 ctx->Pixel.ConvolutionFilterBias[c][3] = (GLfloat) params[3]; 
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameteriv(pname)");
         return;
   }

   ctx->NewState |= _NEW_PIXEL;
}


void GLAPIENTRY
_mesa_CopyConvolutionFilter1D(GLenum target, GLenum internalFormat, GLint x, GLint y, GLsizei width)
{
   GLint baseFormat;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (target != GL_CONVOLUTION_1D) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCopyConvolutionFilter1D(target)");
      return;
   }

   baseFormat = base_filter_format(internalFormat);
   if (baseFormat < 0 || baseFormat == GL_COLOR_INDEX) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCopyConvolutionFilter1D(internalFormat)");
      return;
   }

   if (width < 0 || width > MAX_CONVOLUTION_WIDTH) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCopyConvolutionFilter1D(width)");
      return;
   }

   ctx->Driver.CopyConvolutionFilter1D( ctx, target, 
					internalFormat, x, y, width);
}


void GLAPIENTRY
_mesa_CopyConvolutionFilter2D(GLenum target, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height)
{
   GLint baseFormat;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (target != GL_CONVOLUTION_2D) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCopyConvolutionFilter2D(target)");
      return;
   }

   baseFormat = base_filter_format(internalFormat);
   if (baseFormat < 0 || baseFormat == GL_COLOR_INDEX) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCopyConvolutionFilter2D(internalFormat)");
      return;
   }

   if (width < 0 || width > MAX_CONVOLUTION_WIDTH) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCopyConvolutionFilter2D(width)");
      return;
   }
   if (height < 0 || height > MAX_CONVOLUTION_HEIGHT) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCopyConvolutionFilter2D(height)");
      return;
   }

   ctx->Driver.CopyConvolutionFilter2D( ctx, target, internalFormat, x, y, 
					width, height );
}


void GLAPIENTRY
_mesa_GetConvolutionFilter(GLenum target, GLenum format, GLenum type,
                           GLvoid *image)
{
   struct gl_convolution_attrib *filter;
   GLuint row;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->NewState) {
      _mesa_update_state(ctx);
   }

   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetConvolutionFilter(format or type)");
      return;
   }

   if (format == GL_COLOR_INDEX ||
       format == GL_STENCIL_INDEX ||
       format == GL_DEPTH_COMPONENT ||
       format == GL_INTENSITY ||
       type == GL_BITMAP) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetConvolutionFilter(format or type)");
      return;
   }

   switch (target) {
      case GL_CONVOLUTION_1D:
         filter = &(ctx->Convolution1D);
         break;
      case GL_CONVOLUTION_2D:
         filter = &(ctx->Convolution2D);
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetConvolutionFilter(target)");
         return;
   }

   if (ctx->Pack.BufferObj->Name) {
      /* Pack the filter into a PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(2, &ctx->Pack,
                                     filter->Width, filter->Height,
                                     1, format, type, image)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetConvolutionFilter(invalid PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              ctx->Pack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetConvolutionFilter(PBO is mapped)");
         return;
      }
      image = ADD_POINTERS(image, buf);
   }

   for (row = 0; row < filter->Height; row++) {
      GLvoid *dst = _mesa_image_address2d(&ctx->Pack, image, filter->Width,
                                          filter->Height, format, type,
                                          row, 0);
      GLfloat (*src)[4] = (GLfloat (*)[4]) (filter->Filter + row * filter->Width * 4);
      _mesa_pack_rgba_span_float(ctx, filter->Width, src,
                                 format, type, dst, &ctx->Pack, 0x0);
   }

   if (ctx->Pack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }
}


void GLAPIENTRY
_mesa_GetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_convolution_attrib *conv;
   GLuint c;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
      case GL_CONVOLUTION_1D:
         c = 0;
         conv = &ctx->Convolution1D;
         break;
      case GL_CONVOLUTION_2D:
         c = 1;
         conv = &ctx->Convolution2D;
         break;
      case GL_SEPARABLE_2D:
         c = 2;
         conv = &ctx->Separable2D;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetConvolutionParameterfv(target)");
         return;
   }

   switch (pname) {
      case GL_CONVOLUTION_BORDER_COLOR:
         COPY_4V(params, ctx->Pixel.ConvolutionBorderColor[c]);
         break;
      case GL_CONVOLUTION_BORDER_MODE:
         *params = (GLfloat) ctx->Pixel.ConvolutionBorderMode[c];
         break;
      case GL_CONVOLUTION_FILTER_SCALE:
         COPY_4V(params, ctx->Pixel.ConvolutionFilterScale[c]);
         break;
      case GL_CONVOLUTION_FILTER_BIAS:
         COPY_4V(params, ctx->Pixel.ConvolutionFilterBias[c]);
         break;
      case GL_CONVOLUTION_FORMAT:
         *params = (GLfloat) conv->Format;
         break;
      case GL_CONVOLUTION_WIDTH:
         *params = (GLfloat) conv->Width;
         break;
      case GL_CONVOLUTION_HEIGHT:
         *params = (GLfloat) conv->Height;
         break;
      case GL_MAX_CONVOLUTION_WIDTH:
         *params = (GLfloat) ctx->Const.MaxConvolutionWidth;
         break;
      case GL_MAX_CONVOLUTION_HEIGHT:
         *params = (GLfloat) ctx->Const.MaxConvolutionHeight;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetConvolutionParameterfv(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetConvolutionParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_convolution_attrib *conv;
   GLuint c;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
      case GL_CONVOLUTION_1D:
         c = 0;
         conv = &ctx->Convolution1D;
         break;
      case GL_CONVOLUTION_2D:
         c = 1;
         conv = &ctx->Convolution2D;
         break;
      case GL_SEPARABLE_2D:
         c = 2;
         conv = &ctx->Separable2D;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetConvolutionParameteriv(target)");
         return;
   }

   switch (pname) {
      case GL_CONVOLUTION_BORDER_COLOR:
         params[0] = FLOAT_TO_INT(ctx->Pixel.ConvolutionBorderColor[c][0]);
         params[1] = FLOAT_TO_INT(ctx->Pixel.ConvolutionBorderColor[c][1]);
         params[2] = FLOAT_TO_INT(ctx->Pixel.ConvolutionBorderColor[c][2]);
         params[3] = FLOAT_TO_INT(ctx->Pixel.ConvolutionBorderColor[c][3]);
         break;
      case GL_CONVOLUTION_BORDER_MODE:
         *params = (GLint) ctx->Pixel.ConvolutionBorderMode[c];
         break;
      case GL_CONVOLUTION_FILTER_SCALE:
         params[0] = (GLint) ctx->Pixel.ConvolutionFilterScale[c][0];
         params[1] = (GLint) ctx->Pixel.ConvolutionFilterScale[c][1];
         params[2] = (GLint) ctx->Pixel.ConvolutionFilterScale[c][2];
         params[3] = (GLint) ctx->Pixel.ConvolutionFilterScale[c][3];
         break;
      case GL_CONVOLUTION_FILTER_BIAS:
         params[0] = (GLint) ctx->Pixel.ConvolutionFilterBias[c][0];
         params[1] = (GLint) ctx->Pixel.ConvolutionFilterBias[c][1];
         params[2] = (GLint) ctx->Pixel.ConvolutionFilterBias[c][2];
         params[3] = (GLint) ctx->Pixel.ConvolutionFilterBias[c][3];
         break;
      case GL_CONVOLUTION_FORMAT:
         *params = (GLint) conv->Format;
         break;
      case GL_CONVOLUTION_WIDTH:
         *params = (GLint) conv->Width;
         break;
      case GL_CONVOLUTION_HEIGHT:
         *params = (GLint) conv->Height;
         break;
      case GL_MAX_CONVOLUTION_WIDTH:
         *params = (GLint) ctx->Const.MaxConvolutionWidth;
         break;
      case GL_MAX_CONVOLUTION_HEIGHT:
         *params = (GLint) ctx->Const.MaxConvolutionHeight;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetConvolutionParameteriv(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetSeparableFilter(GLenum target, GLenum format, GLenum type,
                         GLvoid *row, GLvoid *column, GLvoid *span)
{
   const GLint colStart = MAX_CONVOLUTION_WIDTH * 4;
   struct gl_convolution_attrib *filter;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->NewState) {
      _mesa_update_state(ctx);
   }

   if (target != GL_SEPARABLE_2D) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetSeparableFilter(target)");
      return;
   }

   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetConvolutionFilter(format or type)");
      return;
   }

   if (format == GL_COLOR_INDEX ||
       format == GL_STENCIL_INDEX ||
       format == GL_DEPTH_COMPONENT ||
       format == GL_INTENSITY ||
       type == GL_BITMAP) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetConvolutionFilter(format or type)");
      return;
   }

   filter = &ctx->Separable2D;

   if (ctx->Pack.BufferObj->Name) {
      /* Pack filter into PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(1, &ctx->Pack, filter->Width, 1, 1,
                                     format, type, row)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetSeparableFilter(invalid PBO access, width)");
         return;
      }
      if (!_mesa_validate_pbo_access(1, &ctx->Pack, filter->Height, 1, 1,
                                     format, type, column)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetSeparableFilter(invalid PBO access, height)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              ctx->Pack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetSeparableFilter(PBO is mapped)");
         return;
      }
      row = ADD_POINTERS(buf, row);
      column = ADD_POINTERS(buf, column);
   }

   /* Row filter */
   if (row) {
      GLvoid *dst = _mesa_image_address1d(&ctx->Pack, row, filter->Width,
                                          format, type, 0);
      _mesa_pack_rgba_span_float(ctx, filter->Width,
                                 (GLfloat (*)[4]) filter->Filter,
                                 format, type, dst, &ctx->Pack, 0x0);
   }

   /* Column filter */
   if (column) {
      GLvoid *dst = _mesa_image_address1d(&ctx->Pack, column, filter->Height,
                                          format, type, 0);
      GLfloat (*src)[4] = (GLfloat (*)[4]) (filter->Filter + colStart);
      _mesa_pack_rgba_span_float(ctx, filter->Height, src,
                                 format, type, dst, &ctx->Pack, 0x0);
   }

   (void) span;  /* unused at this time */

   if (ctx->Pack.BufferObj->Name) {
      /* Pack filter into PBO */
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              ctx->Unpack.BufferObj);
   }
}


void GLAPIENTRY
_mesa_SeparableFilter2D(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
   const GLint colStart = MAX_CONVOLUTION_WIDTH * 4;
   GLint baseFormat;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (target != GL_SEPARABLE_2D) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glSeparableFilter2D(target)");
      return;
   }

   baseFormat = base_filter_format(internalFormat);
   if (baseFormat < 0 || baseFormat == GL_COLOR_INDEX) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glSeparableFilter2D(internalFormat)");
      return;
   }

   if (width < 0 || width > MAX_CONVOLUTION_WIDTH) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glSeparableFilter2D(width)");
      return;
   }
   if (height < 0 || height > MAX_CONVOLUTION_HEIGHT) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glSeparableFilter2D(height)");
      return;
   }

   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glSeparableFilter2D(format or type)");
      return;
   }

   if (format == GL_COLOR_INDEX ||
       format == GL_STENCIL_INDEX ||
       format == GL_DEPTH_COMPONENT ||
       format == GL_INTENSITY ||
       type == GL_BITMAP) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glSeparableFilter2D(format or type)");
      return;
   }

   ctx->Separable2D.Format = format;
   ctx->Separable2D.InternalFormat = internalFormat;
   ctx->Separable2D.Width = width;
   ctx->Separable2D.Height = height;

   if (ctx->Unpack.BufferObj->Name) {
      /* unpack filter from PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(1, &ctx->Unpack, width, 1, 1,
                                     format, type, row)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glSeparableFilter2D(invalid PBO access, width)");
         return;
      }
      if (!_mesa_validate_pbo_access(1, &ctx->Unpack, height, 1, 1,
                                     format, type, column)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glSeparableFilter2D(invalid PBO access, height)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                              GL_READ_ONLY_ARB,
                                              ctx->Unpack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glSeparableFilter2D(PBO is mapped)");
         return;
      }
      row = ADD_POINTERS(buf, row);
      column = ADD_POINTERS(buf, column);
   }

   /* unpack row filter */
   if (row) {
      _mesa_unpack_color_span_float(ctx, width, GL_RGBA,
                                    ctx->Separable2D.Filter,
                                    format, type, row, &ctx->Unpack,
                                    0);  /* transferOps */

      _mesa_scale_and_bias_rgba(width,
                             (GLfloat (*)[4]) ctx->Separable2D.Filter,
                             ctx->Pixel.ConvolutionFilterScale[2][0],
                             ctx->Pixel.ConvolutionFilterScale[2][1],
                             ctx->Pixel.ConvolutionFilterScale[2][2],
                             ctx->Pixel.ConvolutionFilterScale[2][3],
                             ctx->Pixel.ConvolutionFilterBias[2][0],
                             ctx->Pixel.ConvolutionFilterBias[2][1],
                             ctx->Pixel.ConvolutionFilterBias[2][2],
                             ctx->Pixel.ConvolutionFilterBias[2][3]);
   }

   /* unpack column filter */
   if (column) {
      _mesa_unpack_color_span_float(ctx, height, GL_RGBA,
                                    &ctx->Separable2D.Filter[colStart],
                                    format, type, column, &ctx->Unpack,
                                    0); /* transferOps */

      _mesa_scale_and_bias_rgba(height,
                       (GLfloat (*)[4]) (ctx->Separable2D.Filter + colStart),
                       ctx->Pixel.ConvolutionFilterScale[2][0],
                       ctx->Pixel.ConvolutionFilterScale[2][1],
                       ctx->Pixel.ConvolutionFilterScale[2][2],
                       ctx->Pixel.ConvolutionFilterScale[2][3],
                       ctx->Pixel.ConvolutionFilterBias[2][0],
                       ctx->Pixel.ConvolutionFilterBias[2][1],
                       ctx->Pixel.ConvolutionFilterBias[2][2],
                       ctx->Pixel.ConvolutionFilterBias[2][3]);
   }

   if (ctx->Unpack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              ctx->Unpack.BufferObj);
   }

   ctx->NewState |= _NEW_PIXEL;
}


/**********************************************************************/
/***                   image convolution functions                  ***/
/**********************************************************************/

static void
convolve_1d_reduce(GLint srcWidth, const GLfloat src[][4],
                   GLint filterWidth, const GLfloat filter[][4],
                   GLfloat dest[][4])
{
   GLint dstWidth;
   GLint i, n;

   if (filterWidth >= 1)
      dstWidth = srcWidth - (filterWidth - 1);
   else
      dstWidth = srcWidth;

   if (dstWidth <= 0)
      return;  /* null result */

   for (i = 0; i < dstWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (n = 0; n < filterWidth; n++) {
         sumR += src[i + n][RCOMP] * filter[n][RCOMP];
         sumG += src[i + n][GCOMP] * filter[n][GCOMP];
         sumB += src[i + n][BCOMP] * filter[n][BCOMP];
         sumA += src[i + n][ACOMP] * filter[n][ACOMP];
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
}


static void
convolve_1d_constant(GLint srcWidth, const GLfloat src[][4],
                     GLint filterWidth, const GLfloat filter[][4],
                     GLfloat dest[][4],
                     const GLfloat borderColor[4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   GLint i, n;

   for (i = 0; i < srcWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (n = 0; n < filterWidth; n++) {
         if (i + n < halfFilterWidth || i + n - halfFilterWidth >= srcWidth) {
            sumR += borderColor[RCOMP] * filter[n][RCOMP];
            sumG += borderColor[GCOMP] * filter[n][GCOMP];
            sumB += borderColor[BCOMP] * filter[n][BCOMP];
            sumA += borderColor[ACOMP] * filter[n][ACOMP];
         }
         else {
            sumR += src[i + n - halfFilterWidth][RCOMP] * filter[n][RCOMP];
            sumG += src[i + n - halfFilterWidth][GCOMP] * filter[n][GCOMP];
            sumB += src[i + n - halfFilterWidth][BCOMP] * filter[n][BCOMP];
            sumA += src[i + n - halfFilterWidth][ACOMP] * filter[n][ACOMP];
         }
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
}


static void
convolve_1d_replicate(GLint srcWidth, const GLfloat src[][4],
                      GLint filterWidth, const GLfloat filter[][4],
                      GLfloat dest[][4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   GLint i, n;

   for (i = 0; i < srcWidth; i++) {
      GLfloat sumR = 0.0;
      GLfloat sumG = 0.0;
      GLfloat sumB = 0.0;
      GLfloat sumA = 0.0;
      for (n = 0; n < filterWidth; n++) {
         if (i + n < halfFilterWidth) {
            sumR += src[0][RCOMP] * filter[n][RCOMP];
            sumG += src[0][GCOMP] * filter[n][GCOMP];
            sumB += src[0][BCOMP] * filter[n][BCOMP];
            sumA += src[0][ACOMP] * filter[n][ACOMP];
         }
         else if (i + n - halfFilterWidth >= srcWidth) {
            sumR += src[srcWidth - 1][RCOMP] * filter[n][RCOMP];
            sumG += src[srcWidth - 1][GCOMP] * filter[n][GCOMP];
            sumB += src[srcWidth - 1][BCOMP] * filter[n][BCOMP];
            sumA += src[srcWidth - 1][ACOMP] * filter[n][ACOMP];
         }
         else {
            sumR += src[i + n - halfFilterWidth][RCOMP] * filter[n][RCOMP];
            sumG += src[i + n - halfFilterWidth][GCOMP] * filter[n][GCOMP];
            sumB += src[i + n - halfFilterWidth][BCOMP] * filter[n][BCOMP];
            sumA += src[i + n - halfFilterWidth][ACOMP] * filter[n][ACOMP];
         }
      }
      dest[i][RCOMP] = sumR;
      dest[i][GCOMP] = sumG;
      dest[i][BCOMP] = sumB;
      dest[i][ACOMP] = sumA;
   }
}


static void
convolve_2d_reduce(GLint srcWidth, GLint srcHeight,
                   const GLfloat src[][4],
                   GLint filterWidth, GLint filterHeight,
                   const GLfloat filter[][4],
                   GLfloat dest[][4])
{
   GLint dstWidth, dstHeight;
   GLint i, j, n, m;

   if (filterWidth >= 1)
      dstWidth = srcWidth - (filterWidth - 1);
   else
      dstWidth = srcWidth;

   if (filterHeight >= 1)
      dstHeight = srcHeight - (filterHeight - 1);
   else
      dstHeight = srcHeight;

   if (dstWidth <= 0 || dstHeight <= 0)
      return;

   for (j = 0; j < dstHeight; j++) {
      for (i = 0; i < dstWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               const GLint k = (j + m) * srcWidth + i + n;
               const GLint f = m * filterWidth + n;
               sumR += src[k][RCOMP] * filter[f][RCOMP];
               sumG += src[k][GCOMP] * filter[f][GCOMP];
               sumB += src[k][BCOMP] * filter[f][BCOMP];
               sumA += src[k][ACOMP] * filter[f][ACOMP];
            }
         }
         dest[j * dstWidth + i][RCOMP] = sumR;
         dest[j * dstWidth + i][GCOMP] = sumG;
         dest[j * dstWidth + i][BCOMP] = sumB;
         dest[j * dstWidth + i][ACOMP] = sumA;
      }
   }
}


static void
convolve_2d_constant(GLint srcWidth, GLint srcHeight,
                     const GLfloat src[][4],
                     GLint filterWidth, GLint filterHeight,
                     const GLfloat filter[][4],
                     GLfloat dest[][4],
                     const GLfloat borderColor[4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   const GLint halfFilterHeight = filterHeight / 2;
   GLint i, j, n, m;

   for (j = 0; j < srcHeight; j++) {
      for (i = 0; i < srcWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               const GLint f = m * filterWidth + n;
               const GLint is = i + n - halfFilterWidth;
               const GLint js = j + m - halfFilterHeight;
               if (is < 0 || is >= srcWidth ||
                   js < 0 || js >= srcHeight) {
                  sumR += borderColor[RCOMP] * filter[f][RCOMP];
                  sumG += borderColor[GCOMP] * filter[f][GCOMP];
                  sumB += borderColor[BCOMP] * filter[f][BCOMP];
                  sumA += borderColor[ACOMP] * filter[f][ACOMP];
               }
               else {
                  const GLint k = js * srcWidth + is;
                  sumR += src[k][RCOMP] * filter[f][RCOMP];
                  sumG += src[k][GCOMP] * filter[f][GCOMP];
                  sumB += src[k][BCOMP] * filter[f][BCOMP];
                  sumA += src[k][ACOMP] * filter[f][ACOMP];
               }
            }
         }
         dest[j * srcWidth + i][RCOMP] = sumR;
         dest[j * srcWidth + i][GCOMP] = sumG;
         dest[j * srcWidth + i][BCOMP] = sumB;
         dest[j * srcWidth + i][ACOMP] = sumA;
      }
   }
}


static void
convolve_2d_replicate(GLint srcWidth, GLint srcHeight,
                      const GLfloat src[][4],
                      GLint filterWidth, GLint filterHeight,
                      const GLfloat filter[][4],
                      GLfloat dest[][4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   const GLint halfFilterHeight = filterHeight / 2;
   GLint i, j, n, m;

   for (j = 0; j < srcHeight; j++) {
      for (i = 0; i < srcWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               const GLint f = m * filterWidth + n;
               GLint is = i + n - halfFilterWidth;
               GLint js = j + m - halfFilterHeight;
               GLint k;
               if (is < 0)
                  is = 0;
               else if (is >= srcWidth)
                  is = srcWidth - 1;
               if (js < 0)
                  js = 0;
               else if (js >= srcHeight)
                  js = srcHeight - 1;
               k = js * srcWidth + is;
               sumR += src[k][RCOMP] * filter[f][RCOMP];
               sumG += src[k][GCOMP] * filter[f][GCOMP];
               sumB += src[k][BCOMP] * filter[f][BCOMP];
               sumA += src[k][ACOMP] * filter[f][ACOMP];
            }
         }
         dest[j * srcWidth + i][RCOMP] = sumR;
         dest[j * srcWidth + i][GCOMP] = sumG;
         dest[j * srcWidth + i][BCOMP] = sumB;
         dest[j * srcWidth + i][ACOMP] = sumA;
      }
   }
}


static void
convolve_sep_reduce(GLint srcWidth, GLint srcHeight,
                    const GLfloat src[][4],
                    GLint filterWidth, GLint filterHeight,
                    const GLfloat rowFilt[][4],
                    const GLfloat colFilt[][4],
                    GLfloat dest[][4])
{
   GLint dstWidth, dstHeight;
   GLint i, j, n, m;

   if (filterWidth >= 1)
      dstWidth = srcWidth - (filterWidth - 1);
   else
      dstWidth = srcWidth;

   if (filterHeight >= 1)
      dstHeight = srcHeight - (filterHeight - 1);
   else
      dstHeight = srcHeight;

   if (dstWidth <= 0 || dstHeight <= 0)
      return;

   for (j = 0; j < dstHeight; j++) {
      for (i = 0; i < dstWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               GLint k = (j + m) * srcWidth + i + n;
               sumR += src[k][RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
               sumG += src[k][GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
               sumB += src[k][BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
               sumA += src[k][ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
            }
         }
         dest[j * dstWidth + i][RCOMP] = sumR;
         dest[j * dstWidth + i][GCOMP] = sumG;
         dest[j * dstWidth + i][BCOMP] = sumB;
         dest[j * dstWidth + i][ACOMP] = sumA;
      }
   }
}


static void
convolve_sep_constant(GLint srcWidth, GLint srcHeight,
                      const GLfloat src[][4],
                      GLint filterWidth, GLint filterHeight,
                      const GLfloat rowFilt[][4],
                      const GLfloat colFilt[][4],
                      GLfloat dest[][4],
                      const GLfloat borderColor[4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   const GLint halfFilterHeight = filterHeight / 2;
   GLint i, j, n, m;

   for (j = 0; j < srcHeight; j++) {
      for (i = 0; i < srcWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               const GLint is = i + n - halfFilterWidth;
               const GLint js = j + m - halfFilterHeight;
               if (is < 0 || is >= srcWidth ||
                   js < 0 || js >= srcHeight) {
                  sumR += borderColor[RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
                  sumG += borderColor[GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
                  sumB += borderColor[BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
                  sumA += borderColor[ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
               }
               else {
                  GLint k = js * srcWidth + is;
                  sumR += src[k][RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
                  sumG += src[k][GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
                  sumB += src[k][BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
                  sumA += src[k][ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
               }

            }
         }
         dest[j * srcWidth + i][RCOMP] = sumR;
         dest[j * srcWidth + i][GCOMP] = sumG;
         dest[j * srcWidth + i][BCOMP] = sumB;
         dest[j * srcWidth + i][ACOMP] = sumA;
      }
   }
}


static void
convolve_sep_replicate(GLint srcWidth, GLint srcHeight,
                       const GLfloat src[][4],
                       GLint filterWidth, GLint filterHeight,
                       const GLfloat rowFilt[][4],
                       const GLfloat colFilt[][4],
                       GLfloat dest[][4])
{
   const GLint halfFilterWidth = filterWidth / 2;
   const GLint halfFilterHeight = filterHeight / 2;
   GLint i, j, n, m;

   for (j = 0; j < srcHeight; j++) {
      for (i = 0; i < srcWidth; i++) {
         GLfloat sumR = 0.0;
         GLfloat sumG = 0.0;
         GLfloat sumB = 0.0;
         GLfloat sumA = 0.0;
         for (m = 0; m < filterHeight; m++) {
            for (n = 0; n < filterWidth; n++) {
               GLint is = i + n - halfFilterWidth;
               GLint js = j + m - halfFilterHeight;
               GLint k;
               if (is < 0)
                  is = 0;
               else if (is >= srcWidth)
                  is = srcWidth - 1;
               if (js < 0)
                  js = 0;
               else if (js >= srcHeight)
                  js = srcHeight - 1;
               k = js * srcWidth + is;
               sumR += src[k][RCOMP] * rowFilt[n][RCOMP] * colFilt[m][RCOMP];
               sumG += src[k][GCOMP] * rowFilt[n][GCOMP] * colFilt[m][GCOMP];
               sumB += src[k][BCOMP] * rowFilt[n][BCOMP] * colFilt[m][BCOMP];
               sumA += src[k][ACOMP] * rowFilt[n][ACOMP] * colFilt[m][ACOMP];
            }
         }
         dest[j * srcWidth + i][RCOMP] = sumR;
         dest[j * srcWidth + i][GCOMP] = sumG;
         dest[j * srcWidth + i][BCOMP] = sumB;
         dest[j * srcWidth + i][ACOMP] = sumA;
      }
   }
}



void
_mesa_convolve_1d_image(const GLcontext *ctx, GLsizei *width,
                        const GLfloat *srcImage, GLfloat *dstImage)
{
   switch (ctx->Pixel.ConvolutionBorderMode[0]) {
      case GL_REDUCE:
         convolve_1d_reduce(*width, (const GLfloat (*)[4]) srcImage,
                            ctx->Convolution1D.Width,
                            (const GLfloat (*)[4]) ctx->Convolution1D.Filter,
                            (GLfloat (*)[4]) dstImage);
         *width = *width - (MAX2(ctx->Convolution1D.Width, 1) - 1);
         break;
      case GL_CONSTANT_BORDER:
         convolve_1d_constant(*width, (const GLfloat (*)[4]) srcImage,
                              ctx->Convolution1D.Width,
                              (const GLfloat (*)[4]) ctx->Convolution1D.Filter,
                              (GLfloat (*)[4]) dstImage,
                              ctx->Pixel.ConvolutionBorderColor[0]);
         break;
      case GL_REPLICATE_BORDER:
         convolve_1d_replicate(*width, (const GLfloat (*)[4]) srcImage,
                              ctx->Convolution1D.Width,
                              (const GLfloat (*)[4]) ctx->Convolution1D.Filter,
                              (GLfloat (*)[4]) dstImage);
         break;
      default:
         ;
   }
}


void
_mesa_convolve_2d_image(const GLcontext *ctx, GLsizei *width, GLsizei *height,
                        const GLfloat *srcImage, GLfloat *dstImage)
{
   switch (ctx->Pixel.ConvolutionBorderMode[1]) {
      case GL_REDUCE:
         convolve_2d_reduce(*width, *height,
                            (const GLfloat (*)[4]) srcImage,
                            ctx->Convolution2D.Width,
                            ctx->Convolution2D.Height,
                            (const GLfloat (*)[4]) ctx->Convolution2D.Filter,
                            (GLfloat (*)[4]) dstImage);
         *width = *width - (MAX2(ctx->Convolution2D.Width, 1) - 1);
         *height = *height - (MAX2(ctx->Convolution2D.Height, 1) - 1);
         break;
      case GL_CONSTANT_BORDER:
         convolve_2d_constant(*width, *height,
                              (const GLfloat (*)[4]) srcImage,
                              ctx->Convolution2D.Width,
                              ctx->Convolution2D.Height,
                              (const GLfloat (*)[4]) ctx->Convolution2D.Filter,
                              (GLfloat (*)[4]) dstImage,
                              ctx->Pixel.ConvolutionBorderColor[1]);
         break;
      case GL_REPLICATE_BORDER:
         convolve_2d_replicate(*width, *height,
                               (const GLfloat (*)[4]) srcImage,
                               ctx->Convolution2D.Width,
                               ctx->Convolution2D.Height,
                               (const GLfloat (*)[4])ctx->Convolution2D.Filter,
                               (GLfloat (*)[4]) dstImage);
         break;
      default:
         ;
      }
}


void
_mesa_convolve_sep_image(const GLcontext *ctx,
                         GLsizei *width, GLsizei *height,
                         const GLfloat *srcImage, GLfloat *dstImage)
{
   const GLfloat *rowFilter = ctx->Separable2D.Filter;
   const GLfloat *colFilter = rowFilter + 4 * MAX_CONVOLUTION_WIDTH;

   switch (ctx->Pixel.ConvolutionBorderMode[2]) {
      case GL_REDUCE:
         convolve_sep_reduce(*width, *height,
                             (const GLfloat (*)[4]) srcImage,
                             ctx->Separable2D.Width,
                             ctx->Separable2D.Height,
                             (const GLfloat (*)[4]) rowFilter,
                             (const GLfloat (*)[4]) colFilter,
                             (GLfloat (*)[4]) dstImage);
         *width = *width - (MAX2(ctx->Separable2D.Width, 1) - 1);
         *height = *height - (MAX2(ctx->Separable2D.Height, 1) - 1);
         break;
      case GL_CONSTANT_BORDER:
         convolve_sep_constant(*width, *height,
                               (const GLfloat (*)[4]) srcImage,
                               ctx->Separable2D.Width,
                               ctx->Separable2D.Height,
                               (const GLfloat (*)[4]) rowFilter,
                               (const GLfloat (*)[4]) colFilter,
                               (GLfloat (*)[4]) dstImage,
                               ctx->Pixel.ConvolutionBorderColor[2]);
         break;
      case GL_REPLICATE_BORDER:
         convolve_sep_replicate(*width, *height,
                                (const GLfloat (*)[4]) srcImage,
                                ctx->Separable2D.Width,
                                ctx->Separable2D.Height,
                                (const GLfloat (*)[4]) rowFilter,
                                (const GLfloat (*)[4]) colFilter,
                                (GLfloat (*)[4]) dstImage);
         break;
      default:
         ;
   }
}



/*
 * This function computes an image's size after convolution.
 * If the convolution border mode is GL_REDUCE, the post-convolution
 * image will be smaller than the original.
 */
void
_mesa_adjust_image_for_convolution(const GLcontext *ctx, GLuint dimensions,
                                   GLsizei *width, GLsizei *height)
{
   if (ctx->Pixel.Convolution1DEnabled
       && dimensions == 1
       && ctx->Pixel.ConvolutionBorderMode[0] == GL_REDUCE) {
      *width = *width - (MAX2(ctx->Convolution1D.Width, 1) - 1);
   }
   else if (ctx->Pixel.Convolution2DEnabled
            && dimensions > 1
            && ctx->Pixel.ConvolutionBorderMode[1] == GL_REDUCE) {
      *width = *width - (MAX2(ctx->Convolution2D.Width, 1) - 1);
      *height = *height - (MAX2(ctx->Convolution2D.Height, 1) - 1);
   }
   else if (ctx->Pixel.Separable2DEnabled
            && dimensions > 1
            && ctx->Pixel.ConvolutionBorderMode[2] == GL_REDUCE) {
      *width = *width - (MAX2(ctx->Separable2D.Width, 1) - 1);
      *height = *height - (MAX2(ctx->Separable2D.Height, 1) - 1);
   }
}
