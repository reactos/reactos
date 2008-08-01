/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "simple_list.h"
#include "enums.h"
#include "image.h"
#include "texstore.h"
#include "texformat.h"
#include "texmem.h"
#include "swrast/swrast.h"

#include "mm.h"

#include "intel_ioctl.h"

#include "i830_context.h"
#include "i830_reg.h"



static void
i830TexEnv(GLcontext * ctx, GLenum target,
           GLenum pname, const GLfloat * param)
{

   switch (pname) {
   case GL_TEXTURE_ENV_COLOR:
   case GL_TEXTURE_ENV_MODE:
   case GL_COMBINE_RGB:
   case GL_COMBINE_ALPHA:
   case GL_SOURCE0_RGB:
   case GL_SOURCE1_RGB:
   case GL_SOURCE2_RGB:
   case GL_SOURCE0_ALPHA:
   case GL_SOURCE1_ALPHA:
   case GL_SOURCE2_ALPHA:
   case GL_OPERAND0_RGB:
   case GL_OPERAND1_RGB:
   case GL_OPERAND2_RGB:
   case GL_OPERAND0_ALPHA:
   case GL_OPERAND1_ALPHA:
   case GL_OPERAND2_ALPHA:
   case GL_RGB_SCALE:
   case GL_ALPHA_SCALE:
      break;

   case GL_TEXTURE_LOD_BIAS:{
         struct i830_context *i830 = i830_context(ctx);
         GLuint unit = ctx->Texture.CurrentUnit;
         int b = (int) ((*param) * 16.0);
         if (b > 63)
            b = 63;
         if (b < -64)
            b = -64;
         I830_STATECHANGE(i830, I830_UPLOAD_TEX(unit));
         i830->lodbias_tm0s3[unit] =
            ((b << TM0S3_LOD_BIAS_SHIFT) & TM0S3_LOD_BIAS_MASK);
         break;
      }

   default:
      break;
   }
}




void
i830InitTextureFuncs(struct dd_function_table *functions)
{
   functions->TexEnv = i830TexEnv;
}
