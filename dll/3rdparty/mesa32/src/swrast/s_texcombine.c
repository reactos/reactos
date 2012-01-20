/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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
#include "main/context.h"
#include "main/colormac.h"
#include "main/image.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/pixel.h"

#include "s_context.h"
#include "s_texcombine.h"


#define PROD(A,B)   ( (GLuint)(A) * ((GLuint)(B)+1) )
#define S_PROD(A,B) ( (GLint)(A) * ((GLint)(B)+1) )
#if CHAN_BITS == 32
typedef GLfloat ChanTemp;
#else
typedef GLuint ChanTemp;
#endif


/**
 * Do texture application for GL_ARB/EXT_texture_env_combine.
 * This function also supports GL_{EXT,ARB}_texture_env_dot3 and
 * GL_ATI_texture_env_combine3.  Since "classic" texture environments are
 * implemented using GL_ARB_texture_env_combine-like state, this same function
 * is used for classic texture environment application as well.
 *
 * \param ctx          rendering context
 * \param textureUnit  the texture unit to apply
 * \param n            number of fragments to process (span width)
 * \param primary_rgba incoming fragment color array
 * \param texelBuffer  pointer to texel colors for all texture units
 * 
 * \param rgba         incoming colors, which get modified here
 */
static void
texture_combine( const GLcontext *ctx, GLuint unit, GLuint n,
                 CONST GLchan (*primary_rgba)[4],
                 CONST GLchan *texelBuffer,
                 GLchan (*rgba)[4] )
{
   const struct gl_texture_unit *textureUnit = &(ctx->Texture.Unit[unit]);
   const GLchan (*argRGB [3])[4];
   const GLchan (*argA [3])[4];
   const GLuint RGBshift = textureUnit->_CurrentCombine->ScaleShiftRGB;
   const GLuint Ashift   = textureUnit->_CurrentCombine->ScaleShiftA;
#if CHAN_TYPE == GL_FLOAT
   const GLchan RGBmult = (GLfloat) (1 << RGBshift);
   const GLchan Amult = (GLfloat) (1 << Ashift);
#else
   const GLint half = (CHAN_MAX + 1) / 2;
#endif
   static const GLchan one[4] = { CHAN_MAX, CHAN_MAX, CHAN_MAX, CHAN_MAX };
   static const GLchan zero[4] = { 0, 0, 0, 0 };
   const GLuint numColorArgs = textureUnit->_CurrentCombine->_NumArgsRGB;
   const GLuint numAlphaArgs = textureUnit->_CurrentCombine->_NumArgsA;
   GLchan ccolor[3][MAX_WIDTH][4];
   GLuint i, j;

   ASSERT(ctx->Extensions.EXT_texture_env_combine ||
          ctx->Extensions.ARB_texture_env_combine);
   ASSERT(SWRAST_CONTEXT(ctx)->_AnyTextureCombine);

   /*
   printf("modeRGB 0x%x  modeA 0x%x  srcRGB1 0x%x  srcA1 0x%x  srcRGB2 0x%x  srcA2 0x%x\n",
          textureUnit->_CurrentCombine->ModeRGB,
          textureUnit->_CurrentCombine->ModeA,
          textureUnit->_CurrentCombine->SourceRGB[0],
          textureUnit->_CurrentCombine->SourceA[0],
          textureUnit->_CurrentCombine->SourceRGB[1],
          textureUnit->_CurrentCombine->SourceA[1]);
   */

   /*
    * Do operand setup for up to 3 operands.  Loop over the terms.
    */
   for (j = 0; j < numColorArgs; j++) {
      const GLenum srcRGB = textureUnit->_CurrentCombine->SourceRGB[j];

      switch (srcRGB) {
         case GL_TEXTURE:
            argRGB[j] = (const GLchan (*)[4])
               (texelBuffer + unit * (n * 4 * sizeof(GLchan)));
            break;
         case GL_PRIMARY_COLOR:
            argRGB[j] = primary_rgba;
            break;
         case GL_PREVIOUS:
            argRGB[j] = (const GLchan (*)[4]) rgba;
            break;
         case GL_CONSTANT:
            {
               GLchan (*c)[4] = ccolor[j];
               GLchan red, green, blue, alpha;
               UNCLAMPED_FLOAT_TO_CHAN(red,   textureUnit->EnvColor[0]);
               UNCLAMPED_FLOAT_TO_CHAN(green, textureUnit->EnvColor[1]);
               UNCLAMPED_FLOAT_TO_CHAN(blue,  textureUnit->EnvColor[2]);
               UNCLAMPED_FLOAT_TO_CHAN(alpha, textureUnit->EnvColor[3]);
               for (i = 0; i < n; i++) {
                  c[i][RCOMP] = red;
                  c[i][GCOMP] = green;
                  c[i][BCOMP] = blue;
                  c[i][ACOMP] = alpha;
               }
               argRGB[j] = (const GLchan (*)[4]) ccolor[j];
            }
            break;
	 /* GL_ATI_texture_env_combine3 allows GL_ZERO & GL_ONE as sources.
	  */
	 case GL_ZERO:
            argRGB[j] = & zero;
            break;
	 case GL_ONE:
            argRGB[j] = & one;
            break;
         default:
            /* ARB_texture_env_crossbar source */
            {
               const GLuint srcUnit = srcRGB - GL_TEXTURE0;
               ASSERT(srcUnit < ctx->Const.MaxTextureUnits);
               if (!ctx->Texture.Unit[srcUnit]._ReallyEnabled)
                  return;
               argRGB[j] = (const GLchan (*)[4])
                  (texelBuffer + srcUnit * (n * 4 * sizeof(GLchan)));
            }
      }

      if (textureUnit->_CurrentCombine->OperandRGB[j] != GL_SRC_COLOR) {
         const GLchan (*src)[4] = argRGB[j];
         GLchan (*dst)[4] = ccolor[j];

         /* point to new arg[j] storage */
         argRGB[j] = (const GLchan (*)[4]) ccolor[j];

         if (textureUnit->_CurrentCombine->OperandRGB[j] == GL_ONE_MINUS_SRC_COLOR) {
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = CHAN_MAX - src[i][RCOMP];
               dst[i][GCOMP] = CHAN_MAX - src[i][GCOMP];
               dst[i][BCOMP] = CHAN_MAX - src[i][BCOMP];
            }
         }
         else if (textureUnit->_CurrentCombine->OperandRGB[j] == GL_SRC_ALPHA) {
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = src[i][ACOMP];
               dst[i][GCOMP] = src[i][ACOMP];
               dst[i][BCOMP] = src[i][ACOMP];
            }
         }
         else {
            ASSERT(textureUnit->_CurrentCombine->OperandRGB[j] ==GL_ONE_MINUS_SRC_ALPHA);
            for (i = 0; i < n; i++) {
               dst[i][RCOMP] = CHAN_MAX - src[i][ACOMP];
               dst[i][GCOMP] = CHAN_MAX - src[i][ACOMP];
               dst[i][BCOMP] = CHAN_MAX - src[i][ACOMP];
            }
         }
      }
   }

   /*
    * Set up the argA[i] pointers
    */
   for (j = 0; j < numAlphaArgs; j++) {
      const GLenum srcA = textureUnit->_CurrentCombine->SourceA[j];

      switch (srcA) {
         case GL_TEXTURE:
            argA[j] = (const GLchan (*)[4])
               (texelBuffer + unit * (n * 4 * sizeof(GLchan)));
            break;
         case GL_PRIMARY_COLOR:
            argA[j] = primary_rgba;
            break;
         case GL_PREVIOUS:
            argA[j] = (const GLchan (*)[4]) rgba;
            break;
         case GL_CONSTANT:
            {
               GLchan alpha, (*c)[4] = ccolor[j];
               UNCLAMPED_FLOAT_TO_CHAN(alpha, textureUnit->EnvColor[3]);
               for (i = 0; i < n; i++)
                  c[i][ACOMP] = alpha;
               argA[j] = (const GLchan (*)[4]) ccolor[j];
            }
            break;
	 /* GL_ATI_texture_env_combine3 allows GL_ZERO & GL_ONE as sources.
	  */
	 case GL_ZERO:
            argA[j] = & zero;
            break;
	 case GL_ONE:
            argA[j] = & one;
            break;
         default:
            /* ARB_texture_env_crossbar source */
            {
               const GLuint srcUnit = srcA - GL_TEXTURE0;
               ASSERT(srcUnit < ctx->Const.MaxTextureUnits);
               if (!ctx->Texture.Unit[srcUnit]._ReallyEnabled)
                  return;
               argA[j] = (const GLchan (*)[4])
                  (texelBuffer + srcUnit * (n * 4 * sizeof(GLchan)));
            }
      }

      if (textureUnit->_CurrentCombine->OperandA[j] == GL_ONE_MINUS_SRC_ALPHA) {
         const GLchan (*src)[4] = argA[j];
         GLchan (*dst)[4] = ccolor[j];
         argA[j] = (const GLchan (*)[4]) ccolor[j];
         for (i = 0; i < n; i++) {
            dst[i][ACOMP] = CHAN_MAX - src[i][ACOMP];
         }
      }
   }

   /*
    * Do the texture combine.
    */
   switch (textureUnit->_CurrentCombine->ModeRGB) {
      case GL_REPLACE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            if (RGBshift) {
               for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
                  rgba[i][RCOMP] = arg0[i][RCOMP] * RGBmult;
                  rgba[i][GCOMP] = arg0[i][GCOMP] * RGBmult;
                  rgba[i][BCOMP] = arg0[i][BCOMP] * RGBmult;
#else
                  GLuint r = (GLuint) arg0[i][RCOMP] << RGBshift;
                  GLuint g = (GLuint) arg0[i][GCOMP] << RGBshift;
                  GLuint b = (GLuint) arg0[i][BCOMP] << RGBshift;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
#endif
               }
            }
            else {
               for (i = 0; i < n; i++) {
                  rgba[i][RCOMP] = arg0[i][RCOMP];
                  rgba[i][GCOMP] = arg0[i][GCOMP];
                  rgba[i][BCOMP] = arg0[i][BCOMP];
               }
            }
         }
         break;
      case GL_MODULATE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - RGBshift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = arg0[i][RCOMP] * arg1[i][RCOMP] * RGBmult;
               rgba[i][GCOMP] = arg0[i][GCOMP] * arg1[i][GCOMP] * RGBmult;
               rgba[i][BCOMP] = arg0[i][BCOMP] * arg1[i][BCOMP] * RGBmult;
#else
               GLuint r = PROD(arg0[i][RCOMP], arg1[i][RCOMP]) >> shift;
               GLuint g = PROD(arg0[i][GCOMP], arg1[i][GCOMP]) >> shift;
               GLuint b = PROD(arg0[i][BCOMP], arg1[i][BCOMP]) >> shift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_ADD:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = (arg0[i][RCOMP] + arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] + arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] + arg1[i][BCOMP]) * RGBmult;
#else
               GLint r = ((GLint) arg0[i][RCOMP] + (GLint) arg1[i][RCOMP]) << RGBshift;
               GLint g = ((GLint) arg0[i][GCOMP] + (GLint) arg1[i][GCOMP]) << RGBshift;
               GLint b = ((GLint) arg0[i][BCOMP] + (GLint) arg1[i][BCOMP]) << RGBshift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_ADD_SIGNED:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = (arg0[i][RCOMP] + arg1[i][RCOMP] - 0.5) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] + arg1[i][GCOMP] - 0.5) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] + arg1[i][BCOMP] - 0.5) * RGBmult;
#else
               GLint r = (GLint) arg0[i][RCOMP] + (GLint) arg1[i][RCOMP] -half;
               GLint g = (GLint) arg0[i][GCOMP] + (GLint) arg1[i][GCOMP] -half;
               GLint b = (GLint) arg0[i][BCOMP] + (GLint) arg1[i][BCOMP] -half;
               r = (r < 0) ? 0 : r << RGBshift;
               g = (g < 0) ? 0 : g << RGBshift;
               b = (b < 0) ? 0 : b << RGBshift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_INTERPOLATE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argRGB[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - RGBshift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = (arg0[i][RCOMP] * arg2[i][RCOMP] +
                      arg1[i][RCOMP] * (CHAN_MAXF - arg2[i][RCOMP])) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] * arg2[i][GCOMP] +
                      arg1[i][GCOMP] * (CHAN_MAXF - arg2[i][GCOMP])) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] * arg2[i][BCOMP] +
                      arg1[i][BCOMP] * (CHAN_MAXF - arg2[i][BCOMP])) * RGBmult;
#else
               GLuint r = (PROD(arg0[i][RCOMP], arg2[i][RCOMP])
                           + PROD(arg1[i][RCOMP], CHAN_MAX - arg2[i][RCOMP]))
                              >> shift;
               GLuint g = (PROD(arg0[i][GCOMP], arg2[i][GCOMP])
                           + PROD(arg1[i][GCOMP], CHAN_MAX - arg2[i][GCOMP]))
                              >> shift;
               GLuint b = (PROD(arg0[i][BCOMP], arg2[i][BCOMP])
                           + PROD(arg1[i][BCOMP], CHAN_MAX - arg2[i][BCOMP]))
                              >> shift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_SUBTRACT:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = (arg0[i][RCOMP] - arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = (arg0[i][GCOMP] - arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = (arg0[i][BCOMP] - arg1[i][BCOMP]) * RGBmult;
#else
               GLint r = ((GLint) arg0[i][RCOMP] - (GLint) arg1[i][RCOMP]) << RGBshift;
               GLint g = ((GLint) arg0[i][GCOMP] - (GLint) arg1[i][GCOMP]) << RGBshift;
               GLint b = ((GLint) arg0[i][BCOMP] - (GLint) arg1[i][BCOMP]) << RGBshift;
               rgba[i][RCOMP] = (GLchan) CLAMP(r, 0, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) CLAMP(g, 0, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) CLAMP(b, 0, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_DOT3_RGB_EXT:
      case GL_DOT3_RGBA_EXT:
         {
            /* Do not scale the result by 1 2 or 4 */
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               GLchan dot = ((arg0[i][RCOMP]-0.5F) * (arg1[i][RCOMP]-0.5F) +
                             (arg0[i][GCOMP]-0.5F) * (arg1[i][GCOMP]-0.5F) +
                             (arg0[i][BCOMP]-0.5F) * (arg1[i][BCOMP]-0.5F))
                            * 4.0F;
               dot = CLAMP(dot, 0.0F, CHAN_MAXF);
#else
               GLint dot = (S_PROD((GLint)arg0[i][RCOMP] - half,
				   (GLint)arg1[i][RCOMP] - half) +
			    S_PROD((GLint)arg0[i][GCOMP] - half,
				   (GLint)arg1[i][GCOMP] - half) +
			    S_PROD((GLint)arg0[i][BCOMP] - half,
				   (GLint)arg1[i][BCOMP] - half)) >> 6;
               dot = CLAMP(dot, 0, CHAN_MAX);
#endif
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = (GLchan) dot;
            }
         }
         break;
      case GL_DOT3_RGB:
      case GL_DOT3_RGBA:
         {
            /* DO scale the result by 1 2 or 4 */
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               GLchan dot = ((arg0[i][RCOMP]-0.5F) * (arg1[i][RCOMP]-0.5F) +
                             (arg0[i][GCOMP]-0.5F) * (arg1[i][GCOMP]-0.5F) +
                             (arg0[i][BCOMP]-0.5F) * (arg1[i][BCOMP]-0.5F))
                            * 4.0F * RGBmult;
               dot = CLAMP(dot, 0.0, CHAN_MAXF);
#else
               GLint dot = (S_PROD((GLint)arg0[i][RCOMP] - half,
				   (GLint)arg1[i][RCOMP] - half) +
			    S_PROD((GLint)arg0[i][GCOMP] - half,
				   (GLint)arg1[i][GCOMP] - half) +
			    S_PROD((GLint)arg0[i][BCOMP] - half,
				   (GLint)arg1[i][BCOMP] - half)) >> 6;
               dot <<= RGBshift;
               dot = CLAMP(dot, 0, CHAN_MAX);
#endif
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = (GLchan) dot;
            }
         }
         break;
      case GL_MODULATE_ADD_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argRGB[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - RGBshift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) + arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) + arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) + arg1[i][BCOMP]) * RGBmult;
#else
               GLuint r = (PROD(arg0[i][RCOMP], arg2[i][RCOMP])
                           + ((GLuint) arg1[i][RCOMP] << CHAN_BITS)) >> shift;
               GLuint g = (PROD(arg0[i][GCOMP], arg2[i][GCOMP])
                           + ((GLuint) arg1[i][GCOMP] << CHAN_BITS)) >> shift;
               GLuint b = (PROD(arg0[i][BCOMP], arg2[i][BCOMP])
                           + ((GLuint) arg1[i][BCOMP] << CHAN_BITS)) >> shift;
               rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
            }
	 }
         break;
      case GL_MODULATE_SIGNED_ADD_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argRGB[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - RGBshift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) + arg1[i][RCOMP] - 0.5) * RGBmult;
               rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) + arg1[i][GCOMP] - 0.5) * RGBmult;
               rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) + arg1[i][BCOMP] - 0.5) * RGBmult;
#else
               GLint r = (S_PROD(arg0[i][RCOMP], arg2[i][RCOMP])
			  + (((GLint) arg1[i][RCOMP] - half) << CHAN_BITS))
		    >> shift;
               GLint g = (S_PROD(arg0[i][GCOMP], arg2[i][GCOMP])
			  + (((GLint) arg1[i][GCOMP] - half) << CHAN_BITS))
		    >> shift;
               GLint b = (S_PROD(arg0[i][BCOMP], arg2[i][BCOMP])
			  + (((GLint) arg1[i][BCOMP] - half) << CHAN_BITS))
		    >> shift;
               rgba[i][RCOMP] = (GLchan) CLAMP(r, 0, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) CLAMP(g, 0, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) CLAMP(b, 0, CHAN_MAX);
#endif
            }
	 }
         break;
      case GL_MODULATE_SUBTRACT_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argRGB[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argRGB[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argRGB[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - RGBshift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][RCOMP] = ((arg0[i][RCOMP] * arg2[i][RCOMP]) - arg1[i][RCOMP]) * RGBmult;
               rgba[i][GCOMP] = ((arg0[i][GCOMP] * arg2[i][GCOMP]) - arg1[i][GCOMP]) * RGBmult;
               rgba[i][BCOMP] = ((arg0[i][BCOMP] * arg2[i][BCOMP]) - arg1[i][BCOMP]) * RGBmult;
#else
               GLint r = (S_PROD(arg0[i][RCOMP], arg2[i][RCOMP])
			  - ((GLint) arg1[i][RCOMP] << CHAN_BITS))
		    >> shift;
               GLint g = (S_PROD(arg0[i][GCOMP], arg2[i][GCOMP])
			  - ((GLint) arg1[i][GCOMP] << CHAN_BITS))
		    >> shift;
               GLint b = (S_PROD(arg0[i][BCOMP], arg2[i][BCOMP])
			  - ((GLint) arg1[i][BCOMP] << CHAN_BITS))
		    >> shift;
               rgba[i][RCOMP] = (GLchan) CLAMP(r, 0, CHAN_MAX);
               rgba[i][GCOMP] = (GLchan) CLAMP(g, 0, CHAN_MAX);
               rgba[i][BCOMP] = (GLchan) CLAMP(b, 0, CHAN_MAX);
#endif
            }
	 }
         break;
      default:
         _mesa_problem(ctx, "invalid combine mode");
   }

   switch (textureUnit->_CurrentCombine->ModeA) {
      case GL_REPLACE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            if (Ashift) {
               for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
                  GLchan a = arg0[i][ACOMP] * Amult;
#else
                  GLuint a = (GLuint) arg0[i][ACOMP] << Ashift;
#endif
                  rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
               }
            }
            else {
               for (i = 0; i < n; i++) {
                  rgba[i][ACOMP] = arg0[i][ACOMP];
               }
            }
         }
         break;
      case GL_MODULATE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - Ashift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = arg0[i][ACOMP] * arg1[i][ACOMP] * Amult;
#else
               GLuint a = (PROD(arg0[i][ACOMP], arg1[i][ACOMP]) >> shift);
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_ADD:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan  (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = (arg0[i][ACOMP] + arg1[i][ACOMP]) * Amult;
#else
               GLint a = ((GLint) arg0[i][ACOMP] + arg1[i][ACOMP]) << Ashift;
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_ADD_SIGNED:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = (arg0[i][ACOMP] + arg1[i][ACOMP] - 0.5F) * Amult;
#else
               GLint a = (GLint) arg0[i][ACOMP] + (GLint) arg1[i][ACOMP] -half;
               a = (a < 0) ? 0 : a << Ashift;
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_INTERPOLATE:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argA[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - Ashift;
#endif
            for (i=0; i<n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = (arg0[i][ACOMP] * arg2[i][ACOMP] +
                                 arg1[i][ACOMP] * (CHAN_MAXF - arg2[i][ACOMP]))
                                * Amult;
#else
               GLuint a = (PROD(arg0[i][ACOMP], arg2[i][ACOMP])
                           + PROD(arg1[i][ACOMP], CHAN_MAX - arg2[i][ACOMP]))
                              >> shift;
               rgba[i][ACOMP] = (GLchan) MIN2(a, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_SUBTRACT:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = (arg0[i][ACOMP] - arg1[i][ACOMP]) * Amult;
#else
               GLint a = ((GLint) arg0[i][ACOMP] - (GLint) arg1[i][ACOMP]) << Ashift;
               rgba[i][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_MODULATE_ADD_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argA[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - Ashift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP]) + arg1[i][ACOMP]) * Amult;
#else
               GLint a = (PROD(arg0[i][ACOMP], arg2[i][ACOMP])
			   + ((GLuint) arg1[i][ACOMP] << CHAN_BITS))
		    >> shift;
               rgba[i][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_MODULATE_SIGNED_ADD_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argA[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - Ashift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP]) + arg1[i][ACOMP] - 0.5F) * Amult;
#else
               GLint a = (S_PROD(arg0[i][ACOMP], arg2[i][ACOMP])
			  + (((GLint) arg1[i][ACOMP] - half) << CHAN_BITS))
		    >> shift;
               rgba[i][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
#endif
            }
         }
         break;
      case GL_MODULATE_SUBTRACT_ATI:
         {
            const GLchan (*arg0)[4] = (const GLchan (*)[4]) argA[0];
            const GLchan (*arg1)[4] = (const GLchan (*)[4]) argA[1];
            const GLchan (*arg2)[4] = (const GLchan (*)[4]) argA[2];
#if CHAN_TYPE != GL_FLOAT
            const GLint shift = CHAN_BITS - Ashift;
#endif
            for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
               rgba[i][ACOMP] = ((arg0[i][ACOMP] * arg2[i][ACOMP]) - arg1[i][ACOMP]) * Amult;
#else
               GLint a = (S_PROD(arg0[i][ACOMP], arg2[i][ACOMP]) 
			  - ((GLint) arg1[i][ACOMP] << CHAN_BITS))
		    >> shift;
               rgba[i][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
#endif
            }
         }
         break;
      default:
         _mesa_problem(ctx, "invalid combine mode");
   }

   /* Fix the alpha component for GL_DOT3_RGBA_EXT/ARB combining.
    * This is kind of a kludge.  It would have been better if the spec
    * were written such that the GL_COMBINE_ALPHA value could be set to
    * GL_DOT3.
    */
   if (textureUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA_EXT ||
       textureUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA) {
      for (i = 0; i < n; i++) {
	 rgba[i][ACOMP] = rgba[i][RCOMP];
      }
   }
}
#undef PROD


/**
 * Apply a conventional OpenGL texture env mode (REPLACE, ADD, BLEND,
 * MODULATE, or DECAL) to an array of fragments.
 * Input:  textureUnit - pointer to texture unit to apply
 *         format - base internal texture format
 *         n - number of fragments
 *         primary_rgba - primary colors (may alias rgba for single texture)
 *         texels - array of texel colors
 * InOut:  rgba - incoming fragment colors modified by texel colors
 *                according to the texture environment mode.
 */
static void
texture_apply( const GLcontext *ctx,
               const struct gl_texture_unit *texUnit,
               GLuint n,
               CONST GLchan primary_rgba[][4], CONST GLchan texel[][4],
               GLchan rgba[][4] )
{
   GLint baseLevel;
   GLuint i;
   GLchan Rc, Gc, Bc, Ac;
   GLenum format;
   (void) primary_rgba;

   ASSERT(texUnit);
   ASSERT(texUnit->_Current);

   baseLevel = texUnit->_Current->BaseLevel;
   ASSERT(texUnit->_Current->Image[0][baseLevel]);

   format = texUnit->_Current->Image[0][baseLevel]->_BaseFormat;

   if (format == GL_COLOR_INDEX || format == GL_YCBCR_MESA) {
      format = GL_RGBA;  /* a bit of a hack */
   }
   else if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL_EXT) {
      format = texUnit->_Current->DepthMode;
   }

   switch (texUnit->EnvMode) {
      case GL_REPLACE:
	 switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
                  /* Av = At */
                  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
	    case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = Lt */
                  GLchan Lt = texel[i][RCOMP];
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = Lt;
                  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
                  GLchan Lt = texel[i][RCOMP];
		  /* Cv = Lt */
		  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = Lt;
		  /* Av = At */
		  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
	    case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = It */
                  GLchan It = texel[i][RCOMP];
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = It;
                  /* Av = It */
                  rgba[i][ACOMP] = It;
	       }
	       break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = At */
		  rgba[i][ACOMP] = texel[i][ACOMP];
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_REPLACE) in texture_apply");
               return;
	 }
	 break;

      case GL_MODULATE:
         switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], texel[i][ACOMP] );
	       }
	       break;
	    case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = LtCf */
                  GLchan Lt = texel[i][RCOMP];
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], Lt );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], Lt );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], Lt );
		  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = CfLt */
                  GLchan Lt = texel[i][RCOMP];
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], Lt );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], Lt );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], Lt );
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], texel[i][ACOMP] );
	       }
	       break;
	    case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = CfIt */
                  GLchan It = texel[i][RCOMP];
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], It );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], It );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], It );
		  /* Av = AfIt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], It );
	       }
	       break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = CfCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], texel[i][RCOMP] );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], texel[i][GCOMP] );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], texel[i][BCOMP] );
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = CfCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT( rgba[i][RCOMP], texel[i][RCOMP] );
		  rgba[i][GCOMP] = CHAN_PRODUCT( rgba[i][GCOMP], texel[i][GCOMP] );
		  rgba[i][BCOMP] = CHAN_PRODUCT( rgba[i][BCOMP], texel[i][BCOMP] );
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT( rgba[i][ACOMP], texel[i][ACOMP] );
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_MODULATE) in texture_apply");
               return;
	 }
	 break;

      case GL_DECAL:
         switch (format) {
            case GL_ALPHA:
            case GL_LUMINANCE:
            case GL_LUMINANCE_ALPHA:
            case GL_INTENSITY:
               /* undefined */
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  rgba[i][RCOMP] = texel[i][RCOMP];
		  rgba[i][GCOMP] = texel[i][GCOMP];
		  rgba[i][BCOMP] = texel[i][BCOMP];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-At) + CtAt */
		  GLchan t = texel[i][ACOMP], s = CHAN_MAX - t;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(texel[i][RCOMP],t);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(texel[i][GCOMP],t);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(texel[i][BCOMP],t);
		  /* Av = Af */
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_DECAL) in texture_apply");
               return;
	 }
	 break;

      case GL_BLEND:
         UNCLAMPED_FLOAT_TO_CHAN(Rc, texUnit->EnvColor[0]);
         UNCLAMPED_FLOAT_TO_CHAN(Gc, texUnit->EnvColor[1]);
         UNCLAMPED_FLOAT_TO_CHAN(Bc, texUnit->EnvColor[2]);
         UNCLAMPED_FLOAT_TO_CHAN(Ac, texUnit->EnvColor[3]);
	 switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
		  /* Av = AfAt */
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
	       }
	       break;
            case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Lt) + CcLt */
		  GLchan Lt = texel[i][RCOMP], s = CHAN_MAX - Lt;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(Rc, Lt);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(Gc, Lt);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(Bc, Lt);
		  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Lt) + CcLt */
		  GLchan Lt = texel[i][RCOMP], s = CHAN_MAX - Lt;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(Rc, Lt);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(Gc, Lt);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(Bc, Lt);
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP],texel[i][ACOMP]);
	       }
	       break;
            case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-It) + CcIt */
		  GLchan It = texel[i][RCOMP], s = CHAN_MAX - It;
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], s) + CHAN_PRODUCT(Rc, It);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], s) + CHAN_PRODUCT(Gc, It);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], s) + CHAN_PRODUCT(Bc, It);
                  /* Av = Af(1-It) + Ac*It */
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], s) + CHAN_PRODUCT(Ac, It);
               }
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Ct) + CcCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], (CHAN_MAX-texel[i][RCOMP])) + CHAN_PRODUCT(Rc,texel[i][RCOMP]);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], (CHAN_MAX-texel[i][GCOMP])) + CHAN_PRODUCT(Gc,texel[i][GCOMP]);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], (CHAN_MAX-texel[i][BCOMP])) + CHAN_PRODUCT(Bc,texel[i][BCOMP]);
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Ct) + CcCt */
		  rgba[i][RCOMP] = CHAN_PRODUCT(rgba[i][RCOMP], (CHAN_MAX-texel[i][RCOMP])) + CHAN_PRODUCT(Rc,texel[i][RCOMP]);
		  rgba[i][GCOMP] = CHAN_PRODUCT(rgba[i][GCOMP], (CHAN_MAX-texel[i][GCOMP])) + CHAN_PRODUCT(Gc,texel[i][GCOMP]);
		  rgba[i][BCOMP] = CHAN_PRODUCT(rgba[i][BCOMP], (CHAN_MAX-texel[i][BCOMP])) + CHAN_PRODUCT(Bc,texel[i][BCOMP]);
		  /* Av = AfAt */
		  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP],texel[i][ACOMP]);
	       }
	       break;
            default:
               _mesa_problem(ctx, "Bad format (GL_BLEND) in texture_apply");
               return;
	 }
	 break;

     /* XXX don't clamp results if GLchan is float??? */

      case GL_ADD:  /* GL_EXT_texture_add_env */
         switch (format) {
            case GL_ALPHA:
               for (i=0;i<n;i++) {
                  /* Rv = Rf */
                  /* Gv = Gf */
                  /* Bv = Bf */
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
               }
               break;
            case GL_LUMINANCE:
               for (i=0;i<n;i++) {
                  ChanTemp Lt = texel[i][RCOMP];
                  ChanTemp r = rgba[i][RCOMP] + Lt;
                  ChanTemp g = rgba[i][GCOMP] + Lt;
                  ChanTemp b = rgba[i][BCOMP] + Lt;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  /* Av = Af */
               }
               break;
            case GL_LUMINANCE_ALPHA:
               for (i=0;i<n;i++) {
                  ChanTemp Lt = texel[i][RCOMP];
                  ChanTemp r = rgba[i][RCOMP] + Lt;
                  ChanTemp g = rgba[i][GCOMP] + Lt;
                  ChanTemp b = rgba[i][BCOMP] + Lt;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
               }
               break;
            case GL_INTENSITY:
               for (i=0;i<n;i++) {
                  GLchan It = texel[i][RCOMP];
                  ChanTemp r = rgba[i][RCOMP] + It;
                  ChanTemp g = rgba[i][GCOMP] + It;
                  ChanTemp b = rgba[i][BCOMP] + It;
                  ChanTemp a = rgba[i][ACOMP] + It;
                  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
                  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
                  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  rgba[i][ACOMP] = MIN2(a, CHAN_MAX);
               }
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
                  ChanTemp r = rgba[i][RCOMP] + texel[i][RCOMP];
                  ChanTemp g = rgba[i][GCOMP] + texel[i][GCOMP];
                  ChanTemp b = rgba[i][BCOMP] + texel[i][BCOMP];
		  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
		  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
		  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
                  ChanTemp r = rgba[i][RCOMP] + texel[i][RCOMP];
                  ChanTemp g = rgba[i][GCOMP] + texel[i][GCOMP];
                  ChanTemp b = rgba[i][BCOMP] + texel[i][BCOMP];
		  rgba[i][RCOMP] = MIN2(r, CHAN_MAX);
		  rgba[i][GCOMP] = MIN2(g, CHAN_MAX);
		  rgba[i][BCOMP] = MIN2(b, CHAN_MAX);
                  rgba[i][ACOMP] = CHAN_PRODUCT(rgba[i][ACOMP], texel[i][ACOMP]);
               }
               break;
            default:
               _mesa_problem(ctx, "Bad format (GL_ADD) in texture_apply");
               return;
	 }
	 break;

      default:
         _mesa_problem(ctx, "Bad env mode in texture_apply");
         return;
   }
}



/**
 * Apply texture mapping to a span of fragments.
 */
void
_swrast_texture_span( GLcontext *ctx, SWspan *span )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLchan primary_rgba[MAX_WIDTH][4];
   GLuint unit;

   ASSERT(span->end < MAX_WIDTH);

   /*
    * Save copy of the incoming fragment colors (the GL_PRIMARY_COLOR)
    */
   if (swrast->_AnyTextureCombine)
      MEMCPY(primary_rgba, span->array->rgba, 4 * span->end * sizeof(GLchan));

   /*
    * Must do all texture sampling before combining in order to
    * accomodate GL_ARB_texture_env_crossbar.
    */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (ctx->Texture.Unit[unit]._ReallyEnabled) {
         const GLfloat (*texcoords)[4]
            = (const GLfloat (*)[4])
            span->array->attribs[FRAG_ATTRIB_TEX0 + unit];
         const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
         const struct gl_texture_object *curObj = texUnit->_Current;
         GLfloat *lambda = span->array->lambda[unit];
         GLchan (*texels)[4] = (GLchan (*)[4])
            (swrast->TexelBuffer + unit * (span->end * 4 * sizeof(GLchan)));

         /* adjust texture lod (lambda) */
         if (span->arrayMask & SPAN_LAMBDA) {
            if (texUnit->LodBias + curObj->LodBias != 0.0F) {
               /* apply LOD bias, but don't clamp yet */
               const GLfloat bias = CLAMP(texUnit->LodBias + curObj->LodBias,
                                          -ctx->Const.MaxTextureLodBias,
                                          ctx->Const.MaxTextureLodBias);
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  lambda[i] += bias;
               }
            }

            if (curObj->MinLod != -1000.0 || curObj->MaxLod != 1000.0) {
               /* apply LOD clamping to lambda */
               const GLfloat min = curObj->MinLod;
               const GLfloat max = curObj->MaxLod;
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

         /* GL_SGI_texture_color_table */
         if (texUnit->ColorTableEnabled) {
#if CHAN_TYPE == GL_UNSIGNED_BYTE
            _mesa_lookup_rgba_ubyte(&texUnit->ColorTable, span->end, texels);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
            _mesa_lookup_rgba_ubyte(&texUnit->ColorTable, span->end, texels);
#else
            _mesa_lookup_rgba_float(&texUnit->ColorTable, span->end, texels);
#endif
         }
      }
   }

   /*
    * OK, now apply the texture (aka texture combine/blend).
    * We modify the span->color.rgba values.
    */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (ctx->Texture.Unit[unit]._ReallyEnabled) {
         const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
         if (texUnit->_CurrentCombine != &texUnit->_EnvMode ) {
            texture_combine( ctx, unit, span->end,
                             (CONST GLchan (*)[4]) primary_rgba,
                             swrast->TexelBuffer,
                             span->array->rgba );
         }
         else {
            /* conventional texture blend */
            const GLchan (*texels)[4] = (const GLchan (*)[4])
               (swrast->TexelBuffer + unit *
                (span->end * 4 * sizeof(GLchan)));
            texture_apply( ctx, texUnit, span->end,
                           (CONST GLchan (*)[4]) primary_rgba, texels,
                           span->array->rgba );
         }
      }
   }
}
