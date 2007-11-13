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
#include "imports.h"
#include "context.h"
#include "depthstencil.h"
#include "fbobject.h"
#include "framebuffer.h"
#include "hash.h"
#include "mtypes.h"
#include "renderbuffer.h"

#include "intel_context.h"
#include "intel_fbo.h"
#include "intel_depthstencil.h"
#include "intel_regions.h"


/**
 * The GL_EXT_framebuffer_object allows the user to create their own
 * framebuffer objects consisting of color renderbuffers (0 or more),
 * depth renderbuffers (0 or 1) and stencil renderbuffers (0 or 1).
 *
 * The spec considers depth and stencil renderbuffers to be totally independent
 * buffers.  In reality, most graphics hardware today uses a combined
 * depth+stencil buffer (one 32-bit pixel = 24 bits of Z + 8 bits of stencil).
 *
 * This causes difficulty because the user may create some number of depth
 * renderbuffers and some number of stencil renderbuffers and bind them
 * together in framebuffers in any combination.
 *
 * This code manages all that.
 *
 * 1. Depth renderbuffers are always allocated in hardware as 32bpp
 *    GL_DEPTH24_STENCIL8 buffers.
 *
 * 2. Stencil renderbuffers are initially allocated in software as 8bpp
 *    GL_STENCIL_INDEX8 buffers.
 *
 * 3. Depth and Stencil renderbuffers use the PairedStencil and PairedDepth
 *    fields (respectively) to indicate if the buffer's currently paired
 *    with another stencil or depth buffer (respectively).
 *
 * 4. When a depth and stencil buffer are initially both attached to the
 *    current framebuffer, we merge the stencil buffer values into the
 *    depth buffer (really a depth+stencil buffer).  The then hardware uses
 *    the combined buffer.
 *
 * 5. Whenever a depth or stencil buffer is reallocated (with
 *    glRenderbufferStorage) we undo the pairing and copy the stencil values
 *    from the combined depth/stencil buffer back to the stencil-only buffer.
 *
 * 6. We also undo the pairing when we find a change in buffer bindings.
 *
 * 7. If a framebuffer is only using a depth renderbuffer (no stencil), we
 *    just use the combined depth/stencil buffer and ignore the stencil values.
 *
 * 8. If a framebuffer is only using a stencil renderbuffer (no depth) we have
 *    to promote the 8bpp software stencil buffer to a 32bpp hardware
 *    depth+stencil buffer.
 *
 */



static void
map_regions(GLcontext * ctx,
            struct intel_renderbuffer *depthRb,
            struct intel_renderbuffer *stencilRb)
{
   struct intel_context *intel = intel_context(ctx);
   if (depthRb && depthRb->region) {
      intel_region_map(intel->intelScreen, depthRb->region);
      depthRb->pfMap = depthRb->region->map;
      depthRb->pfPitch = depthRb->region->pitch;
   }
   if (stencilRb && stencilRb->region) {
      intel_region_map(intel->intelScreen, stencilRb->region);
      stencilRb->pfMap = stencilRb->region->map;
      stencilRb->pfPitch = stencilRb->region->pitch;
   }
}

static void
unmap_regions(GLcontext * ctx,
              struct intel_renderbuffer *depthRb,
              struct intel_renderbuffer *stencilRb)
{
   struct intel_context *intel = intel_context(ctx);
   if (depthRb && depthRb->region) {
      intel_region_unmap(intel->intelScreen, depthRb->region);
      depthRb->pfMap = NULL;
      depthRb->pfPitch = 0;
   }
   if (stencilRb && stencilRb->region) {
      intel_region_unmap(intel->intelScreen, stencilRb->region);
      stencilRb->pfMap = NULL;
      stencilRb->pfPitch = 0;
   }
}



/**
 * Undo the pairing/interleaving between depth and stencil buffers.
 * irb should be a depth/stencil or stencil renderbuffer.
 */
void
intel_unpair_depth_stencil(GLcontext * ctx, struct intel_renderbuffer *irb)
{
   if (irb->PairedStencil) {
      /* irb is a depth/stencil buffer */
      struct gl_renderbuffer *stencilRb;
      struct intel_renderbuffer *stencilIrb;

      ASSERT(irb->Base._ActualFormat == GL_DEPTH24_STENCIL8_EXT);

      stencilRb = _mesa_lookup_renderbuffer(ctx, irb->PairedStencil);
      stencilIrb = intel_renderbuffer(stencilRb);
      if (stencilIrb) {
         /* need to extract stencil values from the depth buffer */
         ASSERT(stencilIrb->PairedDepth == irb->Base.Name);
         map_regions(ctx, irb, stencilIrb);
         _mesa_extract_stencil(ctx, &irb->Base, &stencilIrb->Base);
         unmap_regions(ctx, irb, stencilIrb);
         stencilIrb->PairedDepth = 0;
      }
      irb->PairedStencil = 0;
   }
   else if (irb->PairedDepth) {
      /* irb is a stencil buffer */
      struct gl_renderbuffer *depthRb;
      struct intel_renderbuffer *depthIrb;

      ASSERT(irb->Base._ActualFormat == GL_STENCIL_INDEX8_EXT ||
             irb->Base._ActualFormat == GL_DEPTH24_STENCIL8_EXT);

      depthRb = _mesa_lookup_renderbuffer(ctx, irb->PairedDepth);
      depthIrb = intel_renderbuffer(depthRb);
      if (depthIrb) {
         /* need to extract stencil values from the depth buffer */
         ASSERT(depthIrb->PairedStencil == irb->Base.Name);
         map_regions(ctx, depthIrb, irb);
         _mesa_extract_stencil(ctx, &depthIrb->Base, &irb->Base);
         unmap_regions(ctx, depthIrb, irb);
         depthIrb->PairedStencil = 0;
      }
      irb->PairedDepth = 0;
   }
   else {
      _mesa_problem(ctx, "Problem in undo_depth_stencil_pairing");
   }

   ASSERT(irb->PairedStencil == 0);
   ASSERT(irb->PairedDepth == 0);
}


/**
 * Examine the depth and stencil renderbuffers which are attached to the
 * framebuffer.  If both depth and stencil are attached, make sure that the
 * renderbuffers are 'paired' (combined).  If only depth or only stencil is
 * attached, undo any previous pairing.
 *
 * Must be called if NewState & _NEW_BUFFER (when renderbuffer attachments
 * change, for example).
 */
void
intel_validate_paired_depth_stencil(GLcontext * ctx,
                                    struct gl_framebuffer *fb)
{
   struct intel_renderbuffer *depthRb, *stencilRb;

   depthRb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   stencilRb = intel_get_renderbuffer(fb, BUFFER_STENCIL);

   if (depthRb && stencilRb) {
      if (depthRb == stencilRb) {
         /* Using a user-created combined depth/stencil buffer.
          * Nothing to do.
          */
         ASSERT(depthRb->Base._BaseFormat == GL_DEPTH_STENCIL_EXT);
         ASSERT(depthRb->Base._ActualFormat == GL_DEPTH24_STENCIL8_EXT);
      }
      else {
         /* Separate depth/stencil buffers, need to interleave now */
         ASSERT(depthRb->Base._BaseFormat == GL_DEPTH_COMPONENT);
         ASSERT(stencilRb->Base._BaseFormat == GL_STENCIL_INDEX);
         /* may need to interleave depth/stencil now */
         if (depthRb->PairedStencil == stencilRb->Base.Name) {
            /* OK, the depth and stencil buffers are already interleaved */
            ASSERT(stencilRb->PairedDepth == depthRb->Base.Name);
         }
         else {
            /* need to setup new pairing/interleaving */
            if (depthRb->PairedStencil) {
               intel_unpair_depth_stencil(ctx, depthRb);
            }
            if (stencilRb->PairedDepth) {
               intel_unpair_depth_stencil(ctx, stencilRb);
            }

            ASSERT(depthRb->Base._ActualFormat == GL_DEPTH24_STENCIL8_EXT);
            ASSERT(stencilRb->Base._ActualFormat == GL_STENCIL_INDEX8_EXT ||
                   stencilRb->Base._ActualFormat == GL_DEPTH24_STENCIL8_EXT);

            /* establish new pairing: interleave stencil into depth buffer */
            map_regions(ctx, depthRb, stencilRb);
            _mesa_insert_stencil(ctx, &depthRb->Base, &stencilRb->Base);
            unmap_regions(ctx, depthRb, stencilRb);
            depthRb->PairedStencil = stencilRb->Base.Name;
            stencilRb->PairedDepth = depthRb->Base.Name;
         }

      }
   }
   else if (depthRb) {
      /* Depth buffer but no stencil buffer.
       * We'll use a GL_DEPTH24_STENCIL8 buffer and ignore the stencil bits.
       */
      /* can't assert this until storage is allocated:
         ASSERT(depthRb->Base._ActualFormat == GL_DEPTH24_STENCIL8_EXT);
       */
      /* intel_undo any previous pairing */
      if (depthRb->PairedStencil) {
         intel_unpair_depth_stencil(ctx, depthRb);
      }
   }
   else if (stencilRb) {
      /* Stencil buffer but no depth buffer.
       * Since h/w doesn't typically support just 8bpp stencil w/out Z,
       * we'll use a GL_DEPTH24_STENCIL8 buffer and ignore the depth bits.
       */
      /* undo any previous pairing */
      if (stencilRb->PairedDepth) {
         intel_unpair_depth_stencil(ctx, stencilRb);
      }
      if (stencilRb->Base._ActualFormat == GL_STENCIL_INDEX8_EXT) {
         /* promote buffer to GL_DEPTH24_STENCIL8 for hw rendering */
         _mesa_promote_stencil(ctx, &stencilRb->Base);
         ASSERT(stencilRb->Base._ActualFormat == GL_DEPTH24_STENCIL8_EXT);
      }
   }

   /* Finally, update the fb->_DepthBuffer and fb->_StencilBuffer fields */
   _mesa_update_depth_buffer(ctx, fb, BUFFER_DEPTH);
   if (depthRb && depthRb->PairedStencil)
      _mesa_update_stencil_buffer(ctx, fb, BUFFER_DEPTH);
   else
      _mesa_update_stencil_buffer(ctx, fb, BUFFER_STENCIL);


   /* The hardware should use fb->Attachment[BUFFER_DEPTH].Renderbuffer
    * first, if present, then fb->Attachment[BUFFER_STENCIL].Renderbuffer
    * if present.
    */
}
