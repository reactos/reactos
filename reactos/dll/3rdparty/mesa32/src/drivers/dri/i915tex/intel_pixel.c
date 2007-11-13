/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * next paragraph) shall be included in all copies or substantial portionsalloc
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

#include "enums.h"
#include "state.h"
#include "swrast/swrast.h"

#include "intel_context.h"
#include "intel_pixel.h"
#include "intel_regions.h"


/**
 * Check if any fragment operations are in effect which might effect
 * glDraw/CopyPixels.
 */
GLboolean
intel_check_blit_fragment_ops(GLcontext * ctx)
{
   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* XXX Note: Scissor could be done with the blitter:
    */
   return !(ctx->_ImageTransferState ||
            ctx->Color.AlphaEnabled ||
            ctx->Depth.Test ||
            ctx->Fog.Enabled ||
            ctx->Scissor.Enabled ||
            ctx->Stencil.Enabled ||
            !ctx->Color.ColorMask[0] ||
            !ctx->Color.ColorMask[1] ||
            !ctx->Color.ColorMask[2] ||
            !ctx->Color.ColorMask[3] ||
            ctx->Texture._EnabledUnits || 
	    ctx->FragmentProgram._Enabled ||
	    ctx->Color.BlendEnabled);
}


GLboolean
intel_check_meta_tex_fragment_ops(GLcontext * ctx)
{
   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* Some of _ImageTransferState (scale, bias) could be done with
    * fragment programs on i915.
    */
   return !(ctx->_ImageTransferState || ctx->Fog.Enabled ||     /* not done yet */
            ctx->Texture._EnabledUnits || ctx->FragmentProgram._Enabled);
}

/* The intel_region struct doesn't really do enough to capture the
 * format of the pixels in the region.  For now this code assumes that
 * the region is a display surface and hence is either ARGB8888 or
 * RGB565.
 * XXX FBO: If we'd pass in the intel_renderbuffer instead of region, we'd
 * know the buffer's pixel format.
 *
 * \param format  as given to glDraw/ReadPixels
 * \param type  as given to glDraw/ReadPixels
 */
GLboolean
intel_check_blit_format(struct intel_region * region,
                        GLenum format, GLenum type)
{
   if (region->cpp == 4 &&
       (type == GL_UNSIGNED_INT_8_8_8_8_REV ||
        type == GL_UNSIGNED_BYTE) && format == GL_BGRA) {
      return GL_TRUE;
   }

   if (region->cpp == 2 &&
       type == GL_UNSIGNED_SHORT_5_6_5_REV && format == GL_BGR) {
      return GL_TRUE;
   }

   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s: bad format for blit (cpp %d, type %s format %s)\n",
              __FUNCTION__, region->cpp,
              _mesa_lookup_enum_by_nr(type), _mesa_lookup_enum_by_nr(format));

   return GL_FALSE;
}


void
intelInitPixelFuncs(struct dd_function_table *functions)
{
   functions->Accum = _swrast_Accum;
   functions->Bitmap = _swrast_Bitmap;
   functions->CopyPixels = intelCopyPixels;
   functions->ReadPixels = intelReadPixels;
   functions->DrawPixels = intelDrawPixels;
}
