/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.   All Rights Reserved.
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
#include "main/context.h"
#include "main/colormac.h"
#include "main/imports.h"
#include "main/pixeltransfer.h"
#include "program/prog_instruction.h"

#include "s_context.h"
#include "s_texcombine.h"


/**
 * Pointer to array of float[4]
 * This type makes the code below more concise and avoids a lot of casting.
 */
typedef float (*float4_array)[4];


/**
 * Return array of texels for given unit.
 */
static inline float4_array
get_texel_array(SWcontext *swrast, GLuint unit)
{
#ifdef _OPENMP
   return (float4_array) (swrast->TexelBuffer + unit * MAX_WIDTH * 4 * omp_get_num_threads() + (MAX_WIDTH * 4 * omp_get_thread_num()));
#else
   return (float4_array) (swrast->TexelBuffer + unit * MAX_WIDTH * 4);
#endif
}



/**
 * Do texture application for:
 *  GL_EXT_texture_env_combine
 *  GL_ARB_texture_env_combine
 *  GL_EXT_texture_env_dot3
 *  GL_ARB_texture_env_dot3
 *  GL_ATI_texture_env_combine3
 *  GL_NV_texture_env_combine4
 *  conventional GL texture env modes
 *
 * \param ctx          rendering context
 * \param unit         the texture combiner unit
 * \param primary_rgba incoming fragment color array
 * \param texelBuffer  pointer to texel colors for all texture units
 * 
 * \param span         two fields are used in this function:
 *                       span->end: number of fragments to process
 *                       span->array->rgba: incoming/result fragment colors
 */
static void
texture_combine( struct gl_context *ctx, GLuint unit,
                 const float4_array primary_rgba,
                 const GLfloat *texelBuffer,
                 SWspan *span )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const struct gl_texture_unit *textureUnit = &(ctx->Texture.Unit[unit]);
   const struct gl_tex_env_combine_state *combine = textureUnit->_CurrentCombine;
   float4_array argRGB[MAX_COMBINER_TERMS];
   float4_array argA[MAX_COMBINER_TERMS];
   const GLfloat scaleRGB = (GLfloat) (1 << combine->ScaleShiftRGB);
   const GLfloat scaleA = (GLfloat) (1 << combine->ScaleShiftA);
   const GLuint numArgsRGB = combine->_NumArgsRGB;
   const GLuint numArgsA = combine->_NumArgsA;
   float4_array ccolor[4], rgba;
   GLuint i, term;
   GLuint n = span->end;
   GLchan (*rgbaChan)[4] = span->array->rgba;

   /* alloc temp pixel buffers */
   rgba = (float4_array) malloc(4 * n * sizeof(GLfloat));
   if (!rgba) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "texture_combine");
      return;
   }

   for (i = 0; i < numArgsRGB || i < numArgsA; i++) {
      ccolor[i] = (float4_array) malloc(4 * n * sizeof(GLfloat));
      if (!ccolor[i]) {
         while (i) {
            free(ccolor[i]);
            i--;
         }
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "texture_combine");
         free(rgba);
         return;
      }
   }

   for (i = 0; i < n; i++) {
      rgba[i][RCOMP] = CHAN_TO_FLOAT(rgbaChan[i][RCOMP]);
      rgba[i][GCOMP] = CHAN_TO_FLOAT(rgbaChan[i][GCOMP]);
      rgba[i][BCOMP] = CHAN_TO_FLOAT(rgbaChan[i][BCOMP]);
      rgba[i][ACOMP] = CHAN_TO_FLOAT(rgbaChan[i][ACOMP]);
   }

   /*
   printf("modeRGB 0x%x  modeA 0x%x  srcRGB1 0x%x  srcA1 0x%x  srcRGB2 0x%x  srcA2 0x%x\n",
          combine->ModeRGB,
          combine->ModeA,
          combine->SourceRGB[0],
          combine->SourceA[0],
          combine->SourceRGB[1],
          combine->SourceA[1]);
   */

   /*
    * Do operand setup for up to 4 operands.  Loop over the terms.
    */
   for (term = 0; term < numArgsRGB; term++) {
      const GLenum srcRGB = combine->SourceRGB[term];
      const GLenum operandRGB = combine->OperandRGB[term];

      switch (srcRGB) {
         case GL_TEXTURE:
            argRGB[term] = get_texel_array(swrast, unit);
            break;
         case GL_PRIMARY_COLOR:
            argRGB[term] = primary_rgba;
            break;
         case GL_PREVIOUS:
            argRGB[term] = rgba;
            break;
         case GL_CONSTANT:
            {
               float4_array c = ccolor[term];
               GLfloat red   = textureUnit->EnvColor[0];
               GLfloat green = textureUnit->EnvColor[1];
               GLfloat blue  = textureUnit->EnvColor[2];
               GLfloat alpha = textureUnit->EnvColor[3];
               for (i = 0; i < n; i++) {
                  ASSIGN_4V(c[i], red, green, blue, alpha);
               }
               argRGB[term] = ccolor[term];
            }
            break;
	 /* GL_ATI_texture_env_combine3 allows GL_ZERO & GL_ONE as sources.
	  */
	 case GL_ZERO:
            {
               float4_array c = ccolor[term];
               for (i = 0; i < n; i++) {
                  ASSIGN_4V(c[i], 0.0F, 0.0F, 0.0F, 0.0F);
               }
               argRGB[term] = ccolor[term];
            }
            break;
	 case GL_ONE:
            {
               float4_array c = ccolor[term];
               for (i = 0; i < n; i++) {
                  ASSIGN_4V(c[i], 1.0F, 1.0F, 1.0F, 1.0F);
               }
               argRGB[term] = ccolor[term];
            }
            break;
         default:
            /* ARB_texture_env_crossbar source */
            {
               const GLuint srcUnit = srcRGB - GL_TEXTURE0;
               ASSERT(srcUnit < ctx->Const.MaxTextureUnits);
               if (!ctx->Texture.Unit[srcUnit]._ReallyEnabled)
                  goto end;
               argRGB[term] = get_texel_array(swrast, srcUnit);
            }
      }

      if (operandRGB != GL_SRC_COLOR) {
         float4_array src = argRGB[term];
         float4_array dst = ccolor[term];

         /* point to new arg[term] storage */
         argRGB[term] = ccolor[term];

         switch (operandRGB) {
         case GL_ONE_MINUS_SRC_COLOR:
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = 1.0F - src[i][RCOMP];
               dst[i][GCOMP] = 1.0F - src[i][GCOMP];
               dst[i][BCOMP] = 1.0F - src[i][BCOMP];
            }
            break;
         case GL_SRC_ALPHA:
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] =
               dst[i][GCOMP] =
               dst[i][BCOMP] = src[i][ACOMP];
            }
            break;
         case GL_ONE_MINUS_SRC_ALPHA:
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] =
               dst[i][GCOMP] =
               dst[i][BCOMP] = 1.0F - src[i][ACOMP];
            }
            break;
         default:
            _mesa_problem(ctx, "Bad operandRGB");
         }
      }
   }

   /*
    * Set up the argA[term] pointers
    */
   for (term = 0; term < numArgsA; term++) {
      const GLenum srcA = combine->SourceA[term];
      const GLenum operandA = combine->OperandA[term];

      switch (srcA) {
         case GL_TEXTURE:
            argA[term] = get_texel_array(swrast, unit);
            break;
         case GL_PRIMARY_COLOR:
            argA[term] = primary_rgba;
            break;
         case GL_PREVIOUS:
            argA[term] = rgba;
            break;
         case GL_CONSTANT:
            {
               float4_array c = ccolor[term];
               GLfloat alpha = textureUnit->EnvColor[3];
               for (i = 0; i < n; i++)
                  c[i][ACOMP] = alpha;
               argA[term] = ccolor[term];
            }
            break;
	 /* GL_ATI_texture_env_combine3 allows GL_ZERO & GL_ONE as sources.
	  */
	 case GL_ZERO:
            {
               float4_array c = ccolor[term];
               for (i = 0; i < n; i++)
                  c[i][ACOMP] = 0.0F;
               argA[term] = ccolor[term];
            }
            break;
	 case GL_ONE:
            {
               float4_array c = ccolor[term];
               for (i = 0; i < n; i++)
                  c[i][ACOMP] = 1.0F;
               argA[term] = ccolor[term];
            }
            break;
         default:
            /* ARB_texture_env_crossbar source */
            {
               const GLuint srcUnit = srcA - GL_TEXTURE0;
               ASSERT(srcUnit < ctx->Const.MaxTextureUnits);
               if (!ctx->Texture.Unit[srcUnit]._ReallyEnabled)
                  goto end;
               argA[term] = get_texel_array(swrast, srcUnit);
            }
      }

      if (operandA == GL_ONE_MINUS_SRC_ALPHA) {
         float4_array src = argA[term];
         float4_array dst = ccolor[term];
         argA[term] = ccolor[term];
         for (i = 0; i < n; i++) {
            dst[i][ACOMP] = 1.0F - src[i][ACOMP];
         }
      }
   }

   /* RGB channel combine */
   {
      float4_array arg0 = argRGB[0];
      float4_array arg1 = argRGB[1];
      float4_array arg2 = argRGB[2];
      float4_array arg3 = argRGB[3];

      switch (combine->ModeRGB) {
      case GL_REPLACE:
         for (i = 0; i < n; i++) {
            rgba[i][RCOMP] = arg0[i][RCOMP] * scaleRGB;
            rgba[i][GCOMP] = arg0[i][GCOMP] * scaleRGB;
            rgba[i][BCOMP] = arg0[i][BCOMP] * scaleRGB;
         }
         break;
      case GL_MODULATE:
         for (i = 0; i < n; i++) {
            rgba[i][RCOMP] = arg0[i][RCOMP] * arg1[i][RCOMP] * scaleRGB;
            rgba[i][GCOMP] = arg0[i][GCOMP] * arg1[i][GCOMP] * scaleRGB;
            rgba[i][BCOMP] = arg0[i][BCOMP] * arg1[i][BCOMP] * scaleRGB;
         }
         break;
      case GL_ADD:
         if (textureUnit->EnvMode == GL_COMBINE4_NV) {
            /* (a * b) + (c * d) */
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = (arg0[i][RCOMP] * arg1[i][RCOMP] +
                                 arg2[i][RCOMP] * arg3[i][RCOMP]) * scaleRGB;
               rgba[i][GCOMP] = (arg0[i][GCOMP] * arg1[i][GCOMP] +
                                 arg2[i][GCOMP] * arg3[i][GCOMP]) * scaleRGB;
               rgba[i][BCOMP] = (arg0[i][BCOMP] * arg1[i][BCOMP] +
                                 arg2[i][BCOMP] * arg3[i][BCOMP]) * scaleRGB;
            }
         }
         else {
            /* 2-term addition */
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = (arg0[i][RCOMP] + arg1[i][RCOMP]) * scaleRGB;
               rgba[i][GCOMP] = (arg0[i][GCOMP] + arg1[i][GCOMP]) * scaleRGB;
               rgba[i][BCOMP] = (arg0[i][BCOMP] + arg1[i][BCOMP]) * scaleRGB;
            }
         }
         break;
      case GL_ADD_SIGNED:
         if (textureUnit->EnvMode == GL_COMBINE4_NV) {
            /* (a * b) + (c * d) - 0.5 */
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = (arg0[i][RCOMP] * arg1[i][RCOMP] +
                                 arg2[i][RCOMP] * arg3[i][RCOMP] - 0.5F) * scaleRGB;
               rgba[i][GCOMP] = (arg0[i][GCOMP] * arg1[i][GCOMP] +
                                 arg2[i][GCOMP] * arg3[i][GCOMP] - 0.5F) * scaleRGB;
               rgba[i][BCOMP] = (arg0[i][BCOMP] * arg1[i][BCOMP] +
                                 arg2[i][BCOMP] * arg3[i][BCOMP] - 0.5F) * scaleRGB;
            }
         }
         else {
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = (arg0[i][RCOMP] + arg1[i][RCOMP] - 0.5F) * scaleRGB;
               rgba[i][GCOMP] = (arg0[i][GCOMP] + arg1[i][GCOMP] - 0.5F) * scaleRGB;
               rgba[i][BCOMP] = (arg0[i][BCOMP] + arg1[i][BCOMP] - 0.5F) * scaleRGB;
            }
         }
         break;
      case GL_INTERPOLATE:
         for (i = 0; i < n; i++) {
            rgba[i][RCOMP] = (arg0[i][RCOMP] * arg2[i][RCOMP] +
                          arg1[i][RCOMP] * (1.0F - arg2[i][RCOMP])) * scaleRGB;
            rgba[i][GCOMP] = (arg0[i][GCOMP] * arg2[i][GCOMP] +
                          arg1[i][GCOMP] * (1.0F - arg2[i][GCOMP])) * scaleRGB;
            rgba[i][BCOMP] = (arg0[i][BCOMP] * arg2[i][BCOMP] +
                          arg1[i][BCOMP] * (1.0F - arg2[i][BCOMP])) * scaleRGB;
         }
         break;
      case GL_SUBTRACT:
         for (i = 0; i < n; i++) {
            rgba[i][RCOMP] = (arg0[i][RCOMP] - arg1[i][RCOMP]) * scaleRGB;
            rgba[i][GCOMP] = (arg0[i][GCOMP] - arg1[i][GCOMP]) * scaleRGB;
            rgba[i][BCOMP] = (arg0[i][BCOMP] - arg1[i][BCOMP]) * scaleRGB;
         }
         break;
      case GL_DOT3_RGB_EXT:
      case GL_DOT3_RGBA_EXT:
         /* Do not scale the result by 1 2 or 4 */
         for (i = 0; i < n; i++) {
            GLfloat dot = ((arg0[i][RCOMP] - 0.5F) * (arg1[i][RCOMP] - 0.5F) +
                           (arg0[i][GCOMP] - 0.5F) * (arg1[i][GCOMP] - 0.5F) +
                           (arg0[i][BCOMP] - 0.5F) * (arg1[i][BCOMP] - 0.5F))
               * 4.0F;
            dot = CLAMP(dot, 0.0F, 1.0F);
            rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = dot;
         }
         break;
      case GL_DOT3_RGB:
      case GL_DOT3_RGBA:
         /* DO scale the result by 1 2 or 4 */
         for (i = 0; i < n; i++) {
            GLfloat dot = ((arg0[i][RCOMP] - 0.5F) * (arg1[i][RCOMP] - 0.5F) +
                           (arg0[i][GCOMP] - 0.5F) * (arg1[i][GCOMP] - 0.5F) +
                           (arg0[i][BCOMP] - 0.5F) * (arg1[i][BCOMP] - 0.5F))
               * 4.0F * scaleRGB;
            dot = CLAMP(dot, 0.0F, 1.0F);
            rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = dot;
         }
         break;
      case GL_MODULATE_ADD_ATI:
         for (i = 0; i < n; i++) {
            rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) +
                              arg1[i][RCOMP]) * scaleRGB;
            rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) +
                              arg1[i][GCOMP]) * scaleRGB;
            rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) +
                              arg1[i][BCOMP]) * scaleRGB;
	 }
         break;
      case GL_MODULATE_SIGNED_ADD_ATI:
         for (i = 0; i < n; i++) {
            rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) +
                              arg1[i][RCOMP] - 0.5F) * scaleRGB;
            rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) +
                              arg1[i][GCOMP] - 0.5F) * scaleRGB;
            rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) +
                              arg1[i][BCOMP] - 0.5F) * scaleRGB;
	 }
         break;
      case GL_MODULATE_SUBTRACT_ATI:
         for (i = 0; i < n; i++) {
            rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) -
                              arg1[i][RCOMP]) * scaleRGB;
            rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) -
                              arg1[i][GCOMP]) * scaleRGB;
            rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) -
                              arg1[i][BCOMP]) * scaleRGB;
	 }
         break;
      case GL_BUMP_ENVMAP_ATI:
         /* this produces a fixed rgba color, and the coord calc is done elsewhere */
         for (i = 0; i < n; i++) {
            /* rgba result is 0,0,0,1 */
            rgba[i][RCOMP] = 0.0;
            rgba[i][GCOMP] = 0.0;
            rgba[i][BCOMP] = 0.0;
            rgba[i][ACOMP] = 1.0;
	 }
         goto end; /* no alpha processing */
      default:
         _mesa_problem(ctx, "invalid combine mode");
      }
   }

   /* Alpha channel combine */
   {
      float4_array arg0 = argA[0];
      float4_array arg1 = argA[1];
      float4_array arg2 = argA[2];
      float4_array arg3 = argA[3];

      switch (combine->ModeA) {
      case GL_REPLACE:
         for (i = 0; i < n; i++) {
            rgba[i][ACOMP] = arg0[i][ACOMP] * scaleA;
         }
         break;
      case GL_MODULATE:
         for (i = 0; i < n; i++) {
            rgba[i][ACOMP] = arg0[i][ACOMP] * arg1[i][ACOMP] * scaleA;
         }
         break;
      case GL_ADD:
         if (textureUnit->EnvMode == GL_COMBINE4_NV) {
            /* (a * b) + (c * d) */
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = (arg0[i][ACOMP] * arg1[i][ACOMP] +
                                 arg2[i][ACOMP] * arg3[i][ACOMP]) * scaleA;
            }
         }
         else {
            /* two-term add */
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = (arg0[i][ACOMP] + arg1[i][ACOMP]) * scaleA;
            }
         }
         break;
      case GL_ADD_SIGNED:
         if (textureUnit->EnvMode == GL_COMBINE4_NV) {
            /* (a * b) + (c * d) - 0.5 */
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = (arg0[i][ACOMP] * arg1[i][ACOMP] +
                                 arg2[i][ACOMP] * arg3[i][ACOMP] -
                                 0.5F) * scaleA;
            }
         }
         else {
            /* a + b - 0.5 */
            for (i = 0; i < n; i++) {
               rgba[i][ACOMP] = (arg0[i][ACOMP] + arg1[i][ACOMP] - 0.5F) * scaleA;
            }
         }
         break;
      case GL_INTERPOLATE:
         for (i = 0; i < n; i++) {
            rgba[i][ACOMP] = (arg0[i][ACOMP] * arg2[i][ACOMP] +
                              arg1[i][ACOMP] * (1.0F - arg2[i][ACOMP]))
               * scaleA;
         }
         break;
      case GL_SUBTRACT:
         for (i = 0; i < n; i++) {
            rgba[i][ACOMP] = (arg0[i][ACOMP] - arg1[i][ACOMP]) * scaleA;
         }
         break;
      case GL_MODULATE_ADD_ATI:
         for (i = 0; i < n; i++) {
            rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP])
                              + arg1[i][ACOMP]) * scaleA;
         }
         break;
      case GL_MODULATE_SIGNED_ADD_ATI:
         for (i = 0; i < n; i++) {
            rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP]) +
                              arg1[i][ACOMP] - 0.5F) * scaleA;
         }
         break;
      case GL_MODULATE_SUBTRACT_ATI:
         for (i = 0; i < n; i++) {
            rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP])
                              - arg1[i][ACOMP]) * scaleA;
         }
         break;
      default:
         _mesa_problem(ctx, "invalid combine mode");
      }
   }

   /* Fix the alpha component for GL_DOT3_RGBA_EXT/ARB combining.
    * This is kind of a kludge.  It would have been better if the spec
    * were written such that the GL_COMBINE_ALPHA value could be set to
    * GL_DOT3.
    */
   if (combine->ModeRGB == GL_DOT3_RGBA_EXT ||
       combine->ModeRGB == GL_DOT3_RGBA) {
      for (i = 0; i < n; i++) {
	 rgba[i][ACOMP] = rgba[i][RCOMP];
      }
   }

   for (i = 0; i < n; i++) {
      UNCLAMPED_FLOAT_TO_CHAN(rgbaChan[i][RCOMP], rgba[i][RCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(rgbaChan[i][GCOMP], rgba[i][GCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(rgbaChan[i][BCOMP], rgba[i][BCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(rgbaChan[i][ACOMP], rgba[i][ACOMP]);
   }
   /* The span->array->rgba values are of CHAN type so set
    * span->array->ChanType field accordingly.
    */
   span->array->ChanType = CHAN_TYPE;

end:
   for (i = 0; i < numArgsRGB || i < numArgsA; i++) {
      free(ccolor[i]);
   }
   free(rgba);
}


/**
 * Apply X/Y/Z/W/0/1 swizzle to an array of colors/texels.
 * See GL_EXT_texture_swizzle.
 */
static void
swizzle_texels(GLuint swizzle, GLuint count, float4_array texels)
{
   const GLuint swzR = GET_SWZ(swizzle, 0);
   const GLuint swzG = GET_SWZ(swizzle, 1);
   const GLuint swzB = GET_SWZ(swizzle, 2);
   const GLuint swzA = GET_SWZ(swizzle, 3);
   GLfloat vector[6];
   GLuint i;

   vector[SWIZZLE_ZERO] = 0;
   vector[SWIZZLE_ONE] = 1.0F;

   for (i = 0; i < count; i++) {
      vector[SWIZZLE_X] = texels[i][0];
      vector[SWIZZLE_Y] = texels[i][1];
      vector[SWIZZLE_Z] = texels[i][2];
      vector[SWIZZLE_W] = texels[i][3];
      texels[i][RCOMP] = vector[swzR];
      texels[i][GCOMP] = vector[swzG];
      texels[i][BCOMP] = vector[swzB];
      texels[i][ACOMP] = vector[swzA];
   }
}


/**
 * Apply texture mapping to a span of fragments.
 */
void
_swrast_texture_span( struct gl_context *ctx, SWspan *span )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   float4_array primary_rgba;
   GLuint unit;

   if (!swrast->TexelBuffer) {
#ifdef _OPENMP
      const GLint maxThreads = omp_get_max_threads();
#else
      const GLint maxThreads = 1;
#endif

      /* TexelBuffer is also global and normally shared by all SWspan
       * instances; when running with multiple threads, create one per
       * thread.
       */
      swrast->TexelBuffer =
	 (GLfloat *) MALLOC(ctx->Const.MaxTextureImageUnits * maxThreads *
			    MAX_WIDTH * 4 * sizeof(GLfloat));
      if (!swrast->TexelBuffer) {
	 _mesa_error(ctx, GL_OUT_OF_MEMORY, "texture_combine");
	 return;
      }
   }

   primary_rgba = (float4_array) malloc(span->end * 4 * sizeof(GLfloat));

   if (!primary_rgba) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "texture_span");
      return;
   }

   ASSERT(span->end <= MAX_WIDTH);

   /*
    * Save copy of the incoming fragment colors (the GL_PRIMARY_COLOR)
    */
   if (swrast->_TextureCombinePrimary) {
      GLuint i;
      for (i = 0; i < span->end; i++) {
         primary_rgba[i][RCOMP] = CHAN_TO_FLOAT(span->array->rgba[i][RCOMP]);
         primary_rgba[i][GCOMP] = CHAN_TO_FLOAT(span->array->rgba[i][GCOMP]);
         primary_rgba[i][BCOMP] = CHAN_TO_FLOAT(span->array->rgba[i][BCOMP]);
         primary_rgba[i][ACOMP] = CHAN_TO_FLOAT(span->array->rgba[i][ACOMP]);
      }
   }

   /* First must sample all bump maps */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

      if (texUnit->_ReallyEnabled &&
         texUnit->_CurrentCombine->ModeRGB == GL_BUMP_ENVMAP_ATI) {
         const GLfloat (*texcoords)[4] = (const GLfloat (*)[4])
            span->array->attribs[FRAG_ATTRIB_TEX0 + unit];
         float4_array targetcoords =
            span->array->attribs[FRAG_ATTRIB_TEX0 +
               ctx->Texture.Unit[unit].BumpTarget - GL_TEXTURE0];

         const struct gl_texture_object *curObj = texUnit->_Current;
         GLfloat *lambda = span->array->lambda[unit];
         float4_array texels = get_texel_array(swrast, unit);
         GLuint i;
         GLfloat rotMatrix00 = ctx->Texture.Unit[unit].RotMatrix[0];
         GLfloat rotMatrix01 = ctx->Texture.Unit[unit].RotMatrix[1];
         GLfloat rotMatrix10 = ctx->Texture.Unit[unit].RotMatrix[2];
         GLfloat rotMatrix11 = ctx->Texture.Unit[unit].RotMatrix[3];

         /* adjust texture lod (lambda) */
         if (span->arrayMask & SPAN_LAMBDA) {
            if (texUnit->LodBias + curObj->Sampler.LodBias != 0.0F) {
               /* apply LOD bias, but don't clamp yet */
               const GLfloat bias = CLAMP(texUnit->LodBias + curObj->Sampler.LodBias,
                                          -ctx->Const.MaxTextureLodBias,
                                          ctx->Const.MaxTextureLodBias);
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  lambda[i] += bias;
               }
            }

            if (curObj->Sampler.MinLod != -1000.0 ||
                curObj->Sampler.MaxLod != 1000.0) {
               /* apply LOD clamping to lambda */
               const GLfloat min = curObj->Sampler.MinLod;
               const GLfloat max = curObj->Sampler.MaxLod;
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  GLfloat l = lambda[i];
                  lambda[i] = CLAMP(l, min, max);
               }
            }
         }

         /* Sample the texture (span->end = number of fragments) */
         swrast->TextureSample[unit]( ctx, texUnit->_Current, span->end,
                                      texcoords, lambda, texels );

         /* manipulate the span values of the bump target
            not sure this can work correctly even ignoring
            the problem that channel is unsigned */
         for (i = 0; i < span->end; i++) {
            targetcoords[i][0] += (texels[i][0] * rotMatrix00 + texels[i][1] *
                                  rotMatrix01) / targetcoords[i][3];
            targetcoords[i][1] += (texels[i][0] * rotMatrix10 + texels[i][1] *
                                  rotMatrix11) / targetcoords[i][3];
         }
      }
   }

   /*
    * Must do all texture sampling before combining in order to
    * accomodate GL_ARB_texture_env_crossbar.
    */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
      if (texUnit->_ReallyEnabled &&
          texUnit->_CurrentCombine->ModeRGB != GL_BUMP_ENVMAP_ATI) {
         const GLfloat (*texcoords)[4] = (const GLfloat (*)[4])
            span->array->attribs[FRAG_ATTRIB_TEX0 + unit];
         const struct gl_texture_object *curObj = texUnit->_Current;
         GLfloat *lambda = span->array->lambda[unit];
         float4_array texels = get_texel_array(swrast, unit);

         /* adjust texture lod (lambda) */
         if (span->arrayMask & SPAN_LAMBDA) {
            if (texUnit->LodBias + curObj->Sampler.LodBias != 0.0F) {
               /* apply LOD bias, but don't clamp yet */
               const GLfloat bias = CLAMP(texUnit->LodBias + curObj->Sampler.LodBias,
                                          -ctx->Const.MaxTextureLodBias,
                                          ctx->Const.MaxTextureLodBias);
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  lambda[i] += bias;
               }
            }

            if (curObj->Sampler.MinLod != -1000.0 ||
                curObj->Sampler.MaxLod != 1000.0) {
               /* apply LOD clamping to lambda */
               const GLfloat min = curObj->Sampler.MinLod;
               const GLfloat max = curObj->Sampler.MaxLod;
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  GLfloat l = lambda[i];
                  lambda[i] = CLAMP(l, min, max);
               }
            }
         }
         else if (curObj->Sampler.MaxAnisotropy > 1.0 &&
                  curObj->Sampler.MinFilter == GL_LINEAR_MIPMAP_LINEAR) {
            /* sample_lambda_2d_aniso is beeing used as texture_sample_func,
             * it requires the current SWspan *span as an additional parameter.
             * In order to keep the same function signature, the unused lambda
             * parameter will be modified to actually contain the SWspan pointer.
             * This is a Hack. To make it right, the texture_sample_func
             * signature and all implementing functions need to be modified.
             */
            /* "hide" SWspan struct; cast to (GLfloat *) to suppress warning */
            lambda = (GLfloat *)span;
         }

         /* Sample the texture (span->end = number of fragments) */
         swrast->TextureSample[unit]( ctx, texUnit->_Current, span->end,
                                      texcoords, lambda, texels );

         /* GL_EXT_texture_swizzle */
         if (curObj->_Swizzle != SWIZZLE_NOOP) {
            swizzle_texels(curObj->_Swizzle, span->end, texels);
         }
      }
   }

   /*
    * OK, now apply the texture (aka texture combine/blend).
    * We modify the span->color.rgba values.
    */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (ctx->Texture.Unit[unit]._ReallyEnabled)
         texture_combine(ctx, unit, primary_rgba, swrast->TexelBuffer, span);
   }

   free(primary_rgba);
}
