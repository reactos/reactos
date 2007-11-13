/**************************************************************************
 * 
 * Copyright 2005 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "swrast_setup/swrast_setup.h"
#include "swrast/swrast.h"
#include "tnl/tnl.h"
#include "context.h"
#include "brw_context.h"
#include "brw_fallback.h"

#include "glheader.h"
#include "enums.h"
#include "glapi.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"







static GLboolean do_check_fallback(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   GLuint i;
   
   /* BRW_NEW_METAOPS
    */
   if (brw->metaops.active)
      return GL_FALSE;

   if (brw->intel.no_rast)
      return GL_TRUE;
   
   /* _NEW_BUFFERS
    */
   if (ctx->DrawBuffer->_ColorDrawBufferMask[0] != BUFFER_BIT_FRONT_LEFT &&
       ctx->DrawBuffer->_ColorDrawBufferMask[0] != BUFFER_BIT_BACK_LEFT)
      return GL_TRUE;

   /* _NEW_RENDERMODE
    *
    * XXX: need to save/restore RenderMode in metaops state, or
    * somehow move to a new attribs pointer:
    */
   if (ctx->RenderMode != GL_RENDER)
      return GL_TRUE;

   /* _NEW_TEXTURE:
    */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      struct gl_texture_unit *texUnit = &brw->attribs.Texture->Unit[i];
      if (texUnit->_ReallyEnabled) {
	 struct intel_texture_object *intelObj = intel_texture_object(texUnit->_Current);
	 struct gl_texture_image *texImage = intelObj->base.Image[0][intelObj->firstLevel];
	 if (texImage->Border)
	    return GL_TRUE;
      }
   }
   
   /* _NEW_STENCIL 
    */
   if (brw->attribs.Stencil->Enabled && 
       !brw->intel.hw_stencil) {
      return GL_TRUE;
   }


   return GL_FALSE;
}

static void check_fallback(struct brw_context *brw)
{
   brw->intel.Fallback = do_check_fallback(brw);
}

const struct brw_tracked_state brw_check_fallback = {
   .dirty = {
      .mesa = _NEW_BUFFERS | _NEW_RENDERMODE | _NEW_TEXTURE | _NEW_STENCIL,
      .brw  = BRW_NEW_METAOPS,
      .cache = 0
   },
   .update = check_fallback
};




/* Not used:
 */
void intelFallback( struct intel_context *intel, GLuint bit, GLboolean mode )
{
}



