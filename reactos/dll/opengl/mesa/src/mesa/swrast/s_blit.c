/*
 * Mesa 3-D graphics library
 * Version:  6.5
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


#include "main/glheader.h"
#include "main/condrender.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/format_unpack.h"
#include "main/format_pack.h"
#include "s_context.h"


#define ABS(X)   ((X) < 0 ? -(X) : (X))


/**
 * Generate a row resampler function for GL_NEAREST mode.
 */
#define RESAMPLE(NAME, PIXELTYPE, SIZE)			\
static void						\
NAME(GLint srcWidth, GLint dstWidth,			\
     const GLvoid *srcBuffer, GLvoid *dstBuffer,	\
     GLboolean flip)					\
{							\
   const PIXELTYPE *src = (const PIXELTYPE *) srcBuffer;\
   PIXELTYPE *dst = (PIXELTYPE *) dstBuffer;		\
   GLint dstCol;					\
							\
   if (flip) {						\
      for (dstCol = 0; dstCol < dstWidth; dstCol++) {	\
         GLint srcCol = (dstCol * srcWidth) / dstWidth;	\
         ASSERT(srcCol >= 0);				\
         ASSERT(srcCol < srcWidth);			\
         srcCol = srcWidth - 1 - srcCol; /* flip */	\
         if (SIZE == 1) {				\
            dst[dstCol] = src[srcCol];			\
         }						\
         else if (SIZE == 2) {				\
            dst[dstCol*2+0] = src[srcCol*2+0];		\
            dst[dstCol*2+1] = src[srcCol*2+1];		\
         }						\
         else if (SIZE == 4) {				\
            dst[dstCol*4+0] = src[srcCol*4+0];		\
            dst[dstCol*4+1] = src[srcCol*4+1];		\
            dst[dstCol*4+2] = src[srcCol*4+2];		\
            dst[dstCol*4+3] = src[srcCol*4+3];		\
         }						\
      }							\
   }							\
   else {						\
      for (dstCol = 0; dstCol < dstWidth; dstCol++) {	\
         GLint srcCol = (dstCol * srcWidth) / dstWidth;	\
         ASSERT(srcCol >= 0);				\
         ASSERT(srcCol < srcWidth);			\
         if (SIZE == 1) {				\
            dst[dstCol] = src[srcCol];			\
         }						\
         else if (SIZE == 2) {				\
            dst[dstCol*2+0] = src[srcCol*2+0];		\
            dst[dstCol*2+1] = src[srcCol*2+1];		\
         }						\
         else if (SIZE == 4) {				\
            dst[dstCol*4+0] = src[srcCol*4+0];		\
            dst[dstCol*4+1] = src[srcCol*4+1];		\
            dst[dstCol*4+2] = src[srcCol*4+2];		\
            dst[dstCol*4+3] = src[srcCol*4+3];		\
         }						\
      }							\
   }							\
}

/**
 * Resamplers for 1, 2, 4, 8 and 16-byte pixels.
 */
RESAMPLE(resample_row_1, GLubyte, 1)
RESAMPLE(resample_row_2, GLushort, 1)
RESAMPLE(resample_row_4, GLuint, 1)
RESAMPLE(resample_row_8, GLuint, 2)
RESAMPLE(resample_row_16, GLuint, 4)


/**
 * Blit color, depth or stencil with GL_NEAREST filtering.
 */
static void
blit_nearest(struct gl_context *ctx,
             GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
             GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
             GLbitfield buffer)
{
   struct gl_renderbuffer *readRb, *drawRb;

   const GLint srcWidth = ABS(srcX1 - srcX0);
   const GLint dstWidth = ABS(dstX1 - dstX0);
   const GLint srcHeight = ABS(srcY1 - srcY0);
   const GLint dstHeight = ABS(dstY1 - dstY0);

   const GLint srcXpos = MIN2(srcX0, srcX1);
   const GLint srcYpos = MIN2(srcY0, srcY1);
   const GLint dstXpos = MIN2(dstX0, dstX1);
   const GLint dstYpos = MIN2(dstY0, dstY1);

   const GLboolean invertX = (srcX1 < srcX0) ^ (dstX1 < dstX0);
   const GLboolean invertY = (srcY1 < srcY0) ^ (dstY1 < dstY0);
   enum mode {
      DIRECT,
      UNPACK_RGBA_FLOAT,
      UNPACK_Z_FLOAT,
      UNPACK_Z_INT,
      UNPACK_S,
   } mode;
   GLubyte *srcMap, *dstMap;
   GLint srcRowStride, dstRowStride;
   GLint dstRow;

   GLint pixelSize;
   GLvoid *srcBuffer, *dstBuffer;
   GLint prevY = -1;

   typedef void (*resample_func)(GLint srcWidth, GLint dstWidth,
                                 const GLvoid *srcBuffer, GLvoid *dstBuffer,
                                 GLboolean flip);
   resample_func resampleRow;

   switch (buffer) {
   case GL_COLOR_BUFFER_BIT:
      readRb = ctx->ReadBuffer->_ColorReadBuffer;
      drawRb = ctx->DrawBuffer->_ColorDrawBuffers[0];

      if (readRb->Format == drawRb->Format) {
	 mode = DIRECT;
	 pixelSize = _mesa_get_format_bytes(readRb->Format);
      } else {
	 mode = UNPACK_RGBA_FLOAT;
	 pixelSize = 16;
      }

      break;
   case GL_DEPTH_BUFFER_BIT:
      readRb = ctx->ReadBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
      drawRb = ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;

      /* Note that for depth/stencil, the formats of src/dst must match.  By
       * using the core helpers for pack/unpack, we avoid needing to handle
       * masking for things like DEPTH copies of Z24S8.
       */
      if (readRb->Format == MESA_FORMAT_Z32_FLOAT ||
	  readRb->Format == MESA_FORMAT_Z32_FLOAT_X24S8) {
	 mode = UNPACK_Z_FLOAT;
      } else {
	 mode = UNPACK_Z_INT;
      }
      pixelSize = 4;
      break;
   case GL_STENCIL_BUFFER_BIT:
      readRb = ctx->ReadBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
      drawRb = ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
      mode = UNPACK_S;
      pixelSize = 1;
      break;
   default:
      _mesa_problem(ctx, "unexpected buffer in blit_nearest()");
      return;
   }

   /* choose row resampler */
   switch (pixelSize) {
   case 1:
      resampleRow = resample_row_1;
      break;
   case 2:
      resampleRow = resample_row_2;
      break;
   case 4:
      resampleRow = resample_row_4;
      break;
   case 8:
      resampleRow = resample_row_8;
      break;
   case 16:
      resampleRow = resample_row_16;
      break;
   default:
      _mesa_problem(ctx, "unexpected pixel size (%d) in blit_nearest",
                    pixelSize);
      return;
   }

   if (readRb == drawRb) {
      /* map whole buffer for read/write */
      /* XXX we could be clever and just map the union region of the
       * source and dest rects.
       */
      GLubyte *map;
      GLint rowStride;
      GLint formatSize = _mesa_get_format_bytes(readRb->Format);

      ctx->Driver.MapRenderbuffer(ctx, readRb, 0, 0,
                                  readRb->Width, readRb->Height,
                                  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
                                  &map, &rowStride);
      if (!map) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBlitFramebuffer");
         return;
      }

      srcMap = map + srcYpos * rowStride + srcXpos * formatSize;
      dstMap = map + dstYpos * rowStride + dstXpos * formatSize;

      /* this handles overlapping copies */
      if (srcY0 < dstY0) {
         /* copy in reverse (top->down) order */
         srcMap += rowStride * (readRb->Height - 1);
         dstMap += rowStride * (readRb->Height - 1);
         srcRowStride = -rowStride;
         dstRowStride = -rowStride;
      }
      else {
         /* copy in normal (bottom->up) order */
         srcRowStride = rowStride;
         dstRowStride = rowStride;
      }
   }
   else {
      /* different src/dst buffers */
      ctx->Driver.MapRenderbuffer(ctx, readRb,
				  srcXpos, srcYpos,
                                  srcWidth, srcHeight,
                                  GL_MAP_READ_BIT, &srcMap, &srcRowStride);
      if (!srcMap) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBlitFramebuffer");
         return;
      }
      ctx->Driver.MapRenderbuffer(ctx, drawRb,
				  dstXpos, dstYpos,
                                  dstWidth, dstHeight,
                                  GL_MAP_WRITE_BIT, &dstMap, &dstRowStride);
      if (!dstMap) {
         ctx->Driver.UnmapRenderbuffer(ctx, readRb);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBlitFramebuffer");
         return;
      }
   }

   /* allocate the src/dst row buffers */
   srcBuffer = malloc(pixelSize * srcWidth);
   if (!srcBuffer) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBlitFrameBufferEXT");
      return;
   }
   dstBuffer = malloc(pixelSize * dstWidth);
   if (!dstBuffer) {
      free(srcBuffer);
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBlitFrameBufferEXT");
      return;
   }

   for (dstRow = 0; dstRow < dstHeight; dstRow++) {
      GLint srcRow = (dstRow * srcHeight) / dstHeight;
      GLubyte *dstRowStart = dstMap + dstRowStride * dstRow;

      ASSERT(srcRow >= 0);
      ASSERT(srcRow < srcHeight);

      if (invertY) {
         srcRow = srcHeight - 1 - srcRow;
      }

      /* get pixel row from source and resample to match dest width */
      if (prevY != srcRow) {
	 GLubyte *srcRowStart = srcMap + srcRowStride * srcRow;

	 switch (mode) {
	 case DIRECT:
	    memcpy(srcBuffer, srcRowStart, pixelSize * srcWidth);
	    break;
	 case UNPACK_RGBA_FLOAT:
	    _mesa_unpack_rgba_row(readRb->Format, srcWidth, srcRowStart,
				  srcBuffer);
	    break;
	 case UNPACK_Z_FLOAT:
	    _mesa_unpack_float_z_row(readRb->Format, srcWidth, srcRowStart,
				     srcBuffer);
	    break;
	 case UNPACK_Z_INT:
	    _mesa_unpack_uint_z_row(readRb->Format, srcWidth, srcRowStart,
				    srcBuffer);
	    break;
	 case UNPACK_S:
	    _mesa_unpack_ubyte_stencil_row(readRb->Format, srcWidth,
					   srcRowStart, srcBuffer);
	    break;
	 }

         (*resampleRow)(srcWidth, dstWidth, srcBuffer, dstBuffer, invertX);
         prevY = srcRow;
      }

      /* store pixel row in destination */
      switch (mode) {
      case DIRECT:
	 memcpy(dstRowStart, dstBuffer, pixelSize * srcWidth);
	 break;
      case UNPACK_RGBA_FLOAT:
	 _mesa_pack_float_rgba_row(drawRb->Format, dstWidth, dstBuffer,
				   dstRowStart);
	 break;
      case UNPACK_Z_FLOAT:
	 _mesa_pack_float_z_row(drawRb->Format, dstWidth, dstBuffer,
				dstRowStart);
	 break;
      case UNPACK_Z_INT:
	 _mesa_pack_uint_z_row(drawRb->Format, dstWidth, dstBuffer,
			       dstRowStart);
	 break;
      case UNPACK_S:
	 _mesa_pack_ubyte_stencil_row(drawRb->Format, dstWidth, dstBuffer,
				      dstRowStart);
	 break;
      }
   }

   free(srcBuffer);
   free(dstBuffer);

   ctx->Driver.UnmapRenderbuffer(ctx, readRb);
   if (drawRb != readRb) {
      ctx->Driver.UnmapRenderbuffer(ctx, drawRb);
   }
}



#define LERP(T, A, B)  ( (A) + (T) * ((B) - (A)) )

static inline GLfloat
lerp_2d(GLfloat a, GLfloat b,
        GLfloat v00, GLfloat v10, GLfloat v01, GLfloat v11)
{
   const GLfloat temp0 = LERP(a, v00, v10);
   const GLfloat temp1 = LERP(a, v01, v11);
   return LERP(b, temp0, temp1);
}


/**
 * Bilinear interpolation of two source rows.
 * GLubyte pixels.
 */
static void
resample_linear_row_ub(GLint srcWidth, GLint dstWidth,
                       const GLvoid *srcBuffer0, const GLvoid *srcBuffer1,
                       GLvoid *dstBuffer, GLboolean flip, GLfloat rowWeight)
{
   const GLubyte (*srcColor0)[4] = (const GLubyte (*)[4]) srcBuffer0;
   const GLubyte (*srcColor1)[4] = (const GLubyte (*)[4]) srcBuffer1;
   GLubyte (*dstColor)[4] = (GLubyte (*)[4]) dstBuffer;
   const GLfloat dstWidthF = (GLfloat) dstWidth;
   GLint dstCol;

   for (dstCol = 0; dstCol < dstWidth; dstCol++) {
      const GLfloat srcCol = (dstCol * srcWidth) / dstWidthF;
      GLint srcCol0 = IFLOOR(srcCol);
      GLint srcCol1 = srcCol0 + 1;
      GLfloat colWeight = srcCol - srcCol0; /* fractional part of srcCol */
      GLfloat red, green, blue, alpha;

      ASSERT(srcCol0 >= 0);
      ASSERT(srcCol0 < srcWidth);
      ASSERT(srcCol1 <= srcWidth);

      if (srcCol1 == srcWidth) {
         /* last column fudge */
         srcCol1--;
         colWeight = 0.0;
      }

      if (flip) {
         srcCol0 = srcWidth - 1 - srcCol0;
         srcCol1 = srcWidth - 1 - srcCol1;
      }

      red = lerp_2d(colWeight, rowWeight,
                    srcColor0[srcCol0][RCOMP], srcColor0[srcCol1][RCOMP],
                    srcColor1[srcCol0][RCOMP], srcColor1[srcCol1][RCOMP]);
      green = lerp_2d(colWeight, rowWeight,
                    srcColor0[srcCol0][GCOMP], srcColor0[srcCol1][GCOMP],
                    srcColor1[srcCol0][GCOMP], srcColor1[srcCol1][GCOMP]);
      blue = lerp_2d(colWeight, rowWeight,
                    srcColor0[srcCol0][BCOMP], srcColor0[srcCol1][BCOMP],
                    srcColor1[srcCol0][BCOMP], srcColor1[srcCol1][BCOMP]);
      alpha = lerp_2d(colWeight, rowWeight,
                    srcColor0[srcCol0][ACOMP], srcColor0[srcCol1][ACOMP],
                    srcColor1[srcCol0][ACOMP], srcColor1[srcCol1][ACOMP]);
      
      dstColor[dstCol][RCOMP] = IFLOOR(red);
      dstColor[dstCol][GCOMP] = IFLOOR(green);
      dstColor[dstCol][BCOMP] = IFLOOR(blue);
      dstColor[dstCol][ACOMP] = IFLOOR(alpha);
   }
}


/**
 * Bilinear interpolation of two source rows.  floating point pixels.
 */
static void
resample_linear_row_float(GLint srcWidth, GLint dstWidth,
                          const GLvoid *srcBuffer0, const GLvoid *srcBuffer1,
                          GLvoid *dstBuffer, GLboolean flip, GLfloat rowWeight)
{
   const GLfloat (*srcColor0)[4] = (const GLfloat (*)[4]) srcBuffer0;
   const GLfloat (*srcColor1)[4] = (const GLfloat (*)[4]) srcBuffer1;
   GLfloat (*dstColor)[4] = (GLfloat (*)[4]) dstBuffer;
   const GLfloat dstWidthF = (GLfloat) dstWidth;
   GLint dstCol;

   for (dstCol = 0; dstCol < dstWidth; dstCol++) {
      const GLfloat srcCol = (dstCol * srcWidth) / dstWidthF;
      GLint srcCol0 = IFLOOR(srcCol);
      GLint srcCol1 = srcCol0 + 1;
      GLfloat colWeight = srcCol - srcCol0; /* fractional part of srcCol */
      GLfloat red, green, blue, alpha;

      ASSERT(srcCol0 >= 0);
      ASSERT(srcCol0 < srcWidth);
      ASSERT(srcCol1 <= srcWidth);

      if (srcCol1 == srcWidth) {
         /* last column fudge */
         srcCol1--;
         colWeight = 0.0;
      }

      if (flip) {
         srcCol0 = srcWidth - 1 - srcCol0;
         srcCol1 = srcWidth - 1 - srcCol1;
      }

      red = lerp_2d(colWeight, rowWeight,
                    srcColor0[srcCol0][RCOMP], srcColor0[srcCol1][RCOMP],
                    srcColor1[srcCol0][RCOMP], srcColor1[srcCol1][RCOMP]);
      green = lerp_2d(colWeight, rowWeight,
                    srcColor0[srcCol0][GCOMP], srcColor0[srcCol1][GCOMP],
                    srcColor1[srcCol0][GCOMP], srcColor1[srcCol1][GCOMP]);
      blue = lerp_2d(colWeight, rowWeight,
                    srcColor0[srcCol0][BCOMP], srcColor0[srcCol1][BCOMP],
                    srcColor1[srcCol0][BCOMP], srcColor1[srcCol1][BCOMP]);
      alpha = lerp_2d(colWeight, rowWeight,
                    srcColor0[srcCol0][ACOMP], srcColor0[srcCol1][ACOMP],
                    srcColor1[srcCol0][ACOMP], srcColor1[srcCol1][ACOMP]);
      
      dstColor[dstCol][RCOMP] = red;
      dstColor[dstCol][GCOMP] = green;
      dstColor[dstCol][BCOMP] = blue;
      dstColor[dstCol][ACOMP] = alpha;
   }
}



/**
 * Bilinear filtered blit (color only, non-integer values).
 */
static void
blit_linear(struct gl_context *ctx,
            GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
            GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1)
{
   struct gl_renderbuffer *readRb = ctx->ReadBuffer->_ColorReadBuffer;
   struct gl_renderbuffer *drawRb = ctx->DrawBuffer->_ColorDrawBuffers[0];

   const GLint srcWidth = ABS(srcX1 - srcX0);
   const GLint dstWidth = ABS(dstX1 - dstX0);
   const GLint srcHeight = ABS(srcY1 - srcY0);
   const GLint dstHeight = ABS(dstY1 - dstY0);
   const GLfloat dstHeightF = (GLfloat) dstHeight;

   const GLint srcXpos = MIN2(srcX0, srcX1);
   const GLint srcYpos = MIN2(srcY0, srcY1);
   const GLint dstXpos = MIN2(dstX0, dstX1);
   const GLint dstYpos = MIN2(dstY0, dstY1);

   const GLboolean invertX = (srcX1 < srcX0) ^ (dstX1 < dstX0);
   const GLboolean invertY = (srcY1 < srcY0) ^ (dstY1 < dstY0);

   GLint dstRow;

   GLint pixelSize;
   GLvoid *srcBuffer0, *srcBuffer1;
   GLint srcBufferY0 = -1, srcBufferY1 = -1;
   GLvoid *dstBuffer;

   gl_format readFormat = _mesa_get_srgb_format_linear(readRb->Format);
   gl_format drawFormat = _mesa_get_srgb_format_linear(drawRb->Format);
   GLuint bpp = _mesa_get_format_bytes(readFormat);

   GLenum pixelType;

   GLubyte *srcMap, *dstMap;
   GLint srcRowStride, dstRowStride;


   /* Determine datatype for resampling */
   if (_mesa_get_format_max_bits(readFormat) == 8 &&
       _mesa_get_format_datatype(readFormat) == GL_UNSIGNED_NORMALIZED) {
      pixelType = GL_UNSIGNED_BYTE;
      pixelSize = 4 * sizeof(GLubyte);
   }
   else {
      pixelType = GL_FLOAT;
      pixelSize = 4 * sizeof(GLfloat);
   }

   /* Allocate the src/dst row buffers.
    * Keep two adjacent src rows around for bilinear sampling.
    */
   srcBuffer0 = malloc(pixelSize * srcWidth);
   if (!srcBuffer0) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBlitFrameBufferEXT");
      return;
   }
   srcBuffer1 = malloc(pixelSize * srcWidth);
   if (!srcBuffer1) {
      free(srcBuffer0);
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBlitFrameBufferEXT");
      return;
   }
   dstBuffer = malloc(pixelSize * dstWidth);
   if (!dstBuffer) {
      free(srcBuffer0);
      free(srcBuffer1);
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBlitFrameBufferEXT");
      return;
   }

   /*
    * Map src / dst renderbuffers
    */
   if (readRb == drawRb) {
      /* map whole buffer for read/write */
      ctx->Driver.MapRenderbuffer(ctx, readRb,
                                  0, 0, readRb->Width, readRb->Height,
                                  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
                                  &srcMap, &srcRowStride);
      if (!srcMap) {
         free(srcBuffer0);
         free(srcBuffer1);
         free(dstBuffer);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBlitFramebuffer");
         return;
      }

      dstMap = srcMap;
      dstRowStride = srcRowStride;
   }
   else {
      /* different src/dst buffers */
      /* XXX with a bit of work we could just map the regions to be
       * read/written instead of the whole buffers.
       */
      ctx->Driver.MapRenderbuffer(ctx, readRb,
				  0, 0, readRb->Width, readRb->Height,
                                  GL_MAP_READ_BIT, &srcMap, &srcRowStride);
      if (!srcMap) {
         free(srcBuffer0);
         free(srcBuffer1);
         free(dstBuffer);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBlitFramebuffer");
         return;
      }
      ctx->Driver.MapRenderbuffer(ctx, drawRb,
                                  0, 0, drawRb->Width, drawRb->Height,
                                  GL_MAP_WRITE_BIT, &dstMap, &dstRowStride);
      if (!dstMap) {
         ctx->Driver.UnmapRenderbuffer(ctx, readRb);
         free(srcBuffer0);
         free(srcBuffer1);
         free(dstBuffer);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBlitFramebuffer");
         return;
      }
   }

   for (dstRow = 0; dstRow < dstHeight; dstRow++) {
      const GLint dstY = dstYpos + dstRow;
      const GLfloat srcRow = (dstRow * srcHeight) / dstHeightF;
      GLint srcRow0 = IFLOOR(srcRow);
      GLint srcRow1 = srcRow0 + 1;
      GLfloat rowWeight = srcRow - srcRow0; /* fractional part of srcRow */

      ASSERT(srcRow >= 0);
      ASSERT(srcRow < srcHeight);

      if (srcRow1 == srcHeight) {
         /* last row fudge */
         srcRow1 = srcRow0;
         rowWeight = 0.0;
      }

      if (invertY) {
         srcRow0 = srcHeight - 1 - srcRow0;
         srcRow1 = srcHeight - 1 - srcRow1;
      }

      srcY0 = srcYpos + srcRow0;
      srcY1 = srcYpos + srcRow1;

      /* get the two source rows */
      if (srcY0 == srcBufferY0 && srcY1 == srcBufferY1) {
         /* use same source row buffers again */
      }
      else if (srcY0 == srcBufferY1) {
         /* move buffer1 into buffer0 by swapping pointers */
         GLvoid *tmp = srcBuffer0;
         srcBuffer0 = srcBuffer1;
         srcBuffer1 = tmp;
         /* get y1 row */
         {
            GLubyte *src = srcMap + srcY1 * srcRowStride + srcXpos * bpp;
            if (pixelType == GL_UNSIGNED_BYTE) {
               _mesa_unpack_ubyte_rgba_row(readFormat, srcWidth,
                                           src, srcBuffer1);
            }
            else {
               _mesa_unpack_rgba_row(readFormat, srcWidth,
                                     src, srcBuffer1);
            }
         }            
         srcBufferY0 = srcY0;
         srcBufferY1 = srcY1;
      }
      else {
         /* get both new rows */
         {
            GLubyte *src0 = srcMap + srcY0 * srcRowStride + srcXpos * bpp;
            GLubyte *src1 = srcMap + srcY1 * srcRowStride + srcXpos * bpp;
            if (pixelType == GL_UNSIGNED_BYTE) {
               _mesa_unpack_ubyte_rgba_row(readFormat, srcWidth,
                                           src0, srcBuffer0);
               _mesa_unpack_ubyte_rgba_row(readFormat, srcWidth,
                                           src1, srcBuffer1);
            }
            else {
               _mesa_unpack_rgba_row(readFormat, srcWidth, src0, srcBuffer0);
               _mesa_unpack_rgba_row(readFormat, srcWidth, src1, srcBuffer1);
            }
         }
         srcBufferY0 = srcY0;
         srcBufferY1 = srcY1;
      }

      if (pixelType == GL_UNSIGNED_BYTE) {
         resample_linear_row_ub(srcWidth, dstWidth, srcBuffer0, srcBuffer1,
                                dstBuffer, invertX, rowWeight);
      }
      else {
         resample_linear_row_float(srcWidth, dstWidth, srcBuffer0, srcBuffer1,
                                   dstBuffer, invertX, rowWeight);
      }

      /* store pixel row in destination */
      {
         GLubyte *dst = dstMap + dstY * dstRowStride + dstXpos * bpp;
         if (pixelType == GL_UNSIGNED_BYTE) {
            _mesa_pack_ubyte_rgba_row(drawFormat, dstWidth, dstBuffer, dst);
         }
         else {
            _mesa_pack_float_rgba_row(drawFormat, dstWidth, dstBuffer, dst);
         }
      }
   }

   free(srcBuffer0);
   free(srcBuffer1);
   free(dstBuffer);

   ctx->Driver.UnmapRenderbuffer(ctx, readRb);
   if (drawRb != readRb) {
      ctx->Driver.UnmapRenderbuffer(ctx, drawRb);
   }
}



/**
 * Software fallback for glBlitFramebufferEXT().
 */
void
_swrast_BlitFramebuffer(struct gl_context *ctx,
                        GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                        GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                        GLbitfield mask, GLenum filter)
{
   static const GLbitfield buffers[3] = {
      GL_COLOR_BUFFER_BIT,
      GL_DEPTH_BUFFER_BIT,
      GL_STENCIL_BUFFER_BIT
   };
   static const GLenum buffer_enums[3] = {
      GL_COLOR,
      GL_DEPTH,
      GL_STENCIL,
   };
   GLint i;

   if (!_mesa_clip_blit(ctx, &srcX0, &srcY0, &srcX1, &srcY1,
                        &dstX0, &dstY0, &dstX1, &dstY1)) {
      return;
   }

   if (SWRAST_CONTEXT(ctx)->NewState)
      _swrast_validate_derived(ctx);

   /* First, try covering whatever buffers possible using the fast 1:1 copy
    * path.
    */
   if (srcX1 - srcX0 == dstX1 - dstX0 &&
       srcY1 - srcY0 == dstY1 - dstY0 &&
       srcX0 < srcX1 &&
       srcY0 < srcY1 &&
       dstX0 < dstX1 &&
       dstY0 < dstY1) {
      for (i = 0; i < 3; i++) {
         if (mask & buffers[i]) {
	    if (swrast_fast_copy_pixels(ctx,
					srcX0, srcY0,
					srcX1 - srcX0, srcY1 - srcY0,
					dstX0, dstY0,
					buffer_enums[i])) {
	       mask &= ~buffers[i];
	    }
	 }
      }

      if (!mask)
	 return;
   }

   if (filter == GL_NEAREST) {
      for (i = 0; i < 3; i++) {
	 if (mask & buffers[i]) {
	    blit_nearest(ctx,  srcX0, srcY0, srcX1, srcY1,
			 dstX0, dstY0, dstX1, dstY1, buffers[i]);
	 }
      }
   }
   else {
      ASSERT(filter == GL_LINEAR);
      if (mask & GL_COLOR_BUFFER_BIT) {  /* depth/stencil not allowed */
	 blit_linear(ctx,  srcX0, srcY0, srcX1, srcY1,
		     dstX0, dstY0, dstX1, dstY1);
      }
   }

}
