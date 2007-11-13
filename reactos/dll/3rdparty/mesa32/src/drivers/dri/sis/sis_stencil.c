/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/sis/sis_stencil.c,v 1.3 2000/09/26 15:56:49 tsi Exp $ */

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "sis_context.h"
#include "sis_state.h"
#include "sis_stencil.h"

static void
sisDDStencilFuncSeparate( GLcontext * ctx, GLenum face,
                          GLenum func, GLint ref, GLuint mask )
{
  sisContextPtr smesa = SIS_CONTEXT(ctx);
  __GLSiSHardware *prev = &smesa->prev;
  __GLSiSHardware *current = &smesa->current;

   /* set reference */ 
   current->hwStSetting = (STENCIL_FORMAT_8 | 
			   ((ctx->Stencil.Ref[0] & 0xff) << 8) |
			   (ctx->Stencil.ValueMask[0] & 0xff));

  switch (func)
    {
    case GL_NEVER:
      current->hwStSetting |= SiS_STENCIL_NEVER;
      break;
    case GL_LESS:
      current->hwStSetting |= SiS_STENCIL_LESS;
      break;
    case GL_EQUAL:
      current->hwStSetting |= SiS_STENCIL_EQUAL;
      break;
    case GL_LEQUAL:
      current->hwStSetting |= SiS_STENCIL_LEQUAL;
      break;
    case GL_GREATER:
      current->hwStSetting |= SiS_STENCIL_GREATER;
      break;
    case GL_NOTEQUAL:
      current->hwStSetting |= SiS_STENCIL_NOTEQUAL;
      break;
    case GL_GEQUAL:
      current->hwStSetting |= SiS_STENCIL_GEQUAL;
      break;
    case GL_ALWAYS:
      current->hwStSetting |= SiS_STENCIL_ALWAYS;
      break;
    }

   if (current->hwStSetting != prev->hwStSetting)
   {
      prev->hwStSetting = current->hwStSetting;

      smesa->GlobalFlag |= GFLAG_STENCILSETTING;
   }
}

static void
sisDDStencilMaskSeparate( GLcontext * ctx, GLenum face, GLuint mask )
{
  if (!ctx->Visual.stencilBits)
    return;

  /* set Z buffer Write Enable */
  sisDDDepthMask (ctx, ctx->Depth.Mask);
}

static void
sisDDStencilOpSeparate( GLcontext * ctx, GLenum face, GLenum fail,
                        GLenum zfail, GLenum zpass )
{
  sisContextPtr smesa = SIS_CONTEXT(ctx);
  __GLSiSHardware *prev = &smesa->prev;
  __GLSiSHardware *current = &smesa->current;

   current->hwStSetting2 &= ~(MASK_StencilZPassOp | MASK_StencilZFailOp |
      MASK_StencilFailOp);

  switch (fail)
    {
    case GL_KEEP:
      current->hwStSetting2 |= SiS_SFAIL_KEEP;
      break;
    case GL_ZERO:
      current->hwStSetting2 |= SiS_SFAIL_ZERO;
      break;
    case GL_REPLACE:
      current->hwStSetting2 |= SiS_SFAIL_REPLACE;
      break;
    case GL_INVERT:
      current->hwStSetting2 |= SiS_SFAIL_INVERT;
      break;
    case GL_INCR:
      current->hwStSetting2 |= SiS_SFAIL_INCR;
      break;
    case GL_DECR:
      current->hwStSetting2 |= SiS_SFAIL_DECR;
      break;
    case GL_INCR_WRAP:
      current->hwStSetting2 |= SiS_SFAIL_INCR_WRAP;
      break;
    case GL_DECR_WRAP:
      current->hwStSetting2 |= SiS_SFAIL_DECR_WRAP;
      break;
    }

  switch (zfail)
    {
    case GL_KEEP:
      current->hwStSetting2 |= SiS_SPASS_ZFAIL_KEEP;
      break;
    case GL_ZERO:
      current->hwStSetting2 |= SiS_SPASS_ZFAIL_ZERO;
      break;
    case GL_REPLACE:
      current->hwStSetting2 |= SiS_SPASS_ZFAIL_REPLACE;
      break;
    case GL_INVERT:
      current->hwStSetting2 |= SiS_SPASS_ZFAIL_INVERT;
      break;
    case GL_INCR:
      current->hwStSetting2 |= SiS_SPASS_ZFAIL_INCR;
      break;
    case GL_DECR:
      current->hwStSetting2 |= SiS_SPASS_ZFAIL_DECR;
      break;
    case GL_INCR_WRAP:
      current->hwStSetting2 |= SiS_SPASS_ZFAIL_INCR_WRAP;
      break;
    case GL_DECR_WRAP:
      current->hwStSetting2 |= SiS_SPASS_ZFAIL_DECR_WRAP;
      break;
    }

  switch (zpass)
    {
    case GL_KEEP:
      current->hwStSetting2 |= SiS_SPASS_ZPASS_KEEP;
      break;
    case GL_ZERO:
      current->hwStSetting2 |= SiS_SPASS_ZPASS_ZERO;
      break;
    case GL_REPLACE:
      current->hwStSetting2 |= SiS_SPASS_ZPASS_REPLACE;
      break;
    case GL_INVERT:
      current->hwStSetting2 |= SiS_SPASS_ZPASS_INVERT;
      break;
    case GL_INCR:
      current->hwStSetting2 |= SiS_SPASS_ZPASS_INCR;
      break;
    case GL_DECR:
      current->hwStSetting2 |= SiS_SPASS_ZPASS_DECR;
      break;
    case GL_INCR_WRAP:
      current->hwStSetting2 |= SiS_SPASS_ZPASS_INCR_WRAP;
      break;
    case GL_DECR_WRAP:
      current->hwStSetting2 |= SiS_SPASS_ZPASS_DECR_WRAP;
      break;
    }

   if (current->hwStSetting2 != prev->hwStSetting2)
   {
      prev->hwStSetting2 = current->hwStSetting2;
      smesa->GlobalFlag |= GFLAG_STENCILSETTING;
   }
}

void
sisDDInitStencilFuncs( GLcontext *ctx )
{
  ctx->Driver.StencilFuncSeparate = sisDDStencilFuncSeparate;
  ctx->Driver.StencilMaskSeparate = sisDDStencilMaskSeparate;
  ctx->Driver.StencilOpSeparate   = sisDDStencilOpSeparate;
}
