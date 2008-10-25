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

#include "i915_context.h"
#include "i915_reg.h"



static void
i915TexEnv(GLcontext * ctx, GLenum target,
           GLenum pname, const GLfloat * param)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);

   switch (pname) {
   case GL_TEXTURE_LOD_BIAS:{
         GLuint unit = ctx->Texture.CurrentUnit;
         GLint b = (int) ((*param) * 16.0);
         if (b > 255)
            b = 255;
         if (b < -256)
            b = -256;
         I915_STATECHANGE(i915, I915_UPLOAD_TEX(unit));
         i915->lodbias_ss2[unit] =
            ((b << SS2_LOD_BIAS_SHIFT) & SS2_LOD_BIAS_MASK);
         break;
      }

   default:
      break;
   }
}


void
i915InitTextureFuncs(struct dd_function_table *functions)
{
   functions->TexEnv = i915TexEnv;
}
