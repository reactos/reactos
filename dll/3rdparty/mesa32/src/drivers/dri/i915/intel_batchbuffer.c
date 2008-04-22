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


#include <stdio.h>
#include <errno.h>

#include "mtypes.h"
#include "context.h"
#include "enums.h"
#include "vblank.h"

#include "intel_reg.h"
#include "intel_batchbuffer.h"
#include "intel_context.h"




/* ================================================================
 * Performance monitoring functions
 */

static void intel_fill_box( intelContextPtr intel,
			    GLshort x, GLshort y,
			    GLshort w, GLshort h,
			    GLubyte r, GLubyte g, GLubyte b )
{
   x += intel->drawX;
   y += intel->drawY;

   if (x >= 0 && y >= 0 &&
       x+w < intel->intelScreen->width &&
       y+h < intel->intelScreen->height)
      intelEmitFillBlitLocked( intel, 
			       intel->intelScreen->cpp,
			       intel->intelScreen->back.pitch,
			       intel->intelScreen->back.offset,
			       x, y, w, h,
			       INTEL_PACKCOLOR(intel->intelScreen->fbFormat,
					       r,g,b,0xff));
}

static void intel_draw_performance_boxes( intelContextPtr intel )
{
   /* Purple box for page flipping
    */
   if ( intel->perf_boxes & I830_BOX_FLIP ) 
      intel_fill_box( intel, 4, 4, 8, 8, 255, 0, 255 );

   /* Red box if we have to wait for idle at any point
    */
   if ( intel->perf_boxes & I830_BOX_WAIT ) 
      intel_fill_box( intel, 16, 4, 8, 8, 255, 0, 0 );

   /* Blue box: lost context?
    */
   if ( intel->perf_boxes & I830_BOX_LOST_CONTEXT ) 
      intel_fill_box( intel, 28, 4, 8, 8, 0, 0, 255 );

   /* Yellow box for texture swaps
    */
   if ( intel->perf_boxes & I830_BOX_TEXTURE_LOAD ) 
      intel_fill_box( intel, 40, 4, 8, 8, 255, 255, 0 );

   /* Green box if hardware never idles (as far as we can tell)
    */
   if ( !(intel->perf_boxes & I830_BOX_RING_EMPTY) ) 
      intel_fill_box( intel, 64, 4, 8, 8, 0, 255, 0 );


   /* Draw bars indicating number of buffers allocated 
    * (not a great measure, easily confused)
    */
#if 0
   if (intel->dma_used) {
      int bar = intel->dma_used / 10240;
      if (bar > 100) bar = 100;
      if (bar < 1) bar = 1;
      intel_fill_box( intel, 4, 16, bar, 4, 196, 128, 128 );
      intel->dma_used = 0;
   }
#endif

   intel->perf_boxes = 0;
}






static int bad_prim_vertex_nr( int primitive, int nr )
{
   switch (primitive & PRIM3D_MASK) {
   case PRIM3D_POINTLIST:
      return nr < 1;
   case PRIM3D_LINELIST:
      return (nr & 1) || nr == 0;
   case PRIM3D_LINESTRIP:
      return nr < 2;
   case PRIM3D_TRILIST:
   case PRIM3D_RECTLIST:
      return nr % 3 || nr == 0;
   case PRIM3D_POLY:
   case PRIM3D_TRIFAN:
   case PRIM3D_TRISTRIP:
   case PRIM3D_TRISTRIP_RVRSE:
      return nr < 3;
   default:
      return 1;
   }	
}

static void intel_flush_inline_primitive( GLcontext *ctx )
{
   intelContextPtr intel = INTEL_CONTEXT( ctx );
   GLuint used = intel->batch.ptr - intel->prim.start_ptr;
   GLuint vertcount;

   assert(intel->prim.primitive != ~0);

   if (1) {
      /* Check vertex size against the vertex we're specifying to
       * hardware.  If it's wrong, ditch the primitive.
       */ 
      if (!intel->vtbl.check_vertex_size( intel, intel->vertex_size )) 
	 goto do_discard;

      vertcount = (used - 4)/ (intel->vertex_size * 4);

      if (!vertcount)
	 goto do_discard;
      
      if (vertcount * intel->vertex_size * 4 != used - 4) {
	 fprintf(stderr, "vertex size confusion %d %d\n", used, 
		 intel->vertex_size * vertcount * 4);
	 goto do_discard;
      }

      if (bad_prim_vertex_nr( intel->prim.primitive, vertcount )) {
	 fprintf(stderr, "bad_prim_vertex_nr %x %d\n", intel->prim.primitive,
		 vertcount);
	 goto do_discard;
      }
   }

   if (used < 8)
      goto do_discard;

   *(int *)intel->prim.start_ptr = (_3DPRIMITIVE | 
				    intel->prim.primitive |
				    (used/4-2));

   goto finished;
   
 do_discard:
   intel->batch.ptr -= used;
   intel->batch.space += used;
   assert(intel->batch.space >= 0);

 finished:
   intel->prim.primitive = ~0;
   intel->prim.start_ptr = 0;
   intel->prim.flush = 0;
}


/* Emit a primitive referencing vertices in a vertex buffer.
 */
void intelStartInlinePrimitive( intelContextPtr intel, GLuint prim )
{
   BATCH_LOCALS;

   if (0)
      fprintf(stderr, "%s %x\n", __FUNCTION__, prim);


   /* Finish any in-progress primitive:
    */
   INTEL_FIREVERTICES( intel );
   
   /* Emit outstanding state:
    */
   intel->vtbl.emit_state( intel );
   
   /* Make sure there is some space in this buffer:
    */
   if (intel->vertex_size * 10 * sizeof(GLuint) >= intel->batch.space) {
      intelFlushBatch(intel, GL_TRUE); 
      intel->vtbl.emit_state( intel );
   }

#if 1
   if (((unsigned long)intel->batch.ptr) & 0x4) {
      BEGIN_BATCH(1);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
#endif

   /* Emit a slot which will be filled with the inline primitive
    * command later.
    */
   BEGIN_BATCH(2);
   OUT_BATCH( 0 );

   intel->prim.start_ptr = batch_ptr;
   intel->prim.primitive = prim;
   intel->prim.flush = intel_flush_inline_primitive;
   intel->batch.contains_geometry = 1;

   OUT_BATCH( 0 );
   ADVANCE_BATCH();
}


void intelRestartInlinePrimitive( intelContextPtr intel )
{
   GLuint prim = intel->prim.primitive;

   intel_flush_inline_primitive( &intel->ctx );
   if (1) intelFlushBatch(intel, GL_TRUE); /* GL_TRUE - is critical */
   intelStartInlinePrimitive( intel, prim );
}



void intelWrapInlinePrimitive( intelContextPtr intel )
{
   GLuint prim = intel->prim.primitive;

   if (0)
      fprintf(stderr, "%s\n", __FUNCTION__);
   intel_flush_inline_primitive( &intel->ctx );
   intelFlushBatch(intel, GL_TRUE);
   intelStartInlinePrimitive( intel, prim );
}


/* Emit a primitive with space for inline vertices.
 */
GLuint *intelEmitInlinePrimitiveLocked(intelContextPtr intel, 
				       int primitive,
				       int dwords,
				       int vertex_size )
{
   GLuint *tmp = 0;
   BATCH_LOCALS;

   if (0)
      fprintf(stderr, "%s 0x%x %d\n", __FUNCTION__, primitive, dwords);

   /* Emit outstanding state:
    */
   intel->vtbl.emit_state( intel );

   if ((1+dwords)*4 >= intel->batch.space) {
      intelFlushBatch(intel, GL_TRUE); 
      intel->vtbl.emit_state( intel );
   }


   if (1) {
      int used = dwords * 4;
      int vertcount;

      /* Check vertex size against the vertex we're specifying to
       * hardware.  If it's wrong, ditch the primitive.
       */ 
      if (!intel->vtbl.check_vertex_size( intel, vertex_size )) 
	 goto do_discard;

      vertcount = dwords / vertex_size;
      
      if (dwords % vertex_size) {
	 fprintf(stderr, "did not request a whole number of vertices\n");
	 goto do_discard;
      }

      if (bad_prim_vertex_nr( primitive, vertcount )) {
	 fprintf(stderr, "bad_prim_vertex_nr %x %d\n", primitive, vertcount);
	 goto do_discard;
      }

      if (used < 8)
	 goto do_discard;
   }

   /* Emit 3D_PRIMITIVE commands:
    */
   BEGIN_BATCH(1 + dwords);
   OUT_BATCH( _3DPRIMITIVE | 
	      primitive |
	      (dwords-1) );

   tmp = (GLuint *)batch_ptr;
   batch_ptr += dwords * 4;

   ADVANCE_BATCH();

   intel->batch.contains_geometry = 1;

 do_discard:
   return tmp;
}


static void intelWaitForFrameCompletion( intelContextPtr intel )
{
  drm_i915_sarea_t *sarea = (drm_i915_sarea_t *)intel->sarea;

   if (intel->do_irqs) {
      if (intelGetLastFrame(intel) < sarea->last_dispatch) {
	 if (!intel->irqsEmitted) {
	    while (intelGetLastFrame (intel) < sarea->last_dispatch)
	       ;
	 }
	 else {
	    intelWaitIrq( intel, intel->alloc.irq_emitted );	
	 }
	 intel->irqsEmitted = 10;
      }

      if (intel->irqsEmitted) {
	 LOCK_HARDWARE( intel ); 
	 intelEmitIrqLocked( intel );
	 intel->irqsEmitted--;
	 UNLOCK_HARDWARE( intel ); 
      }
   } 
   else {
      while (intelGetLastFrame (intel) < sarea->last_dispatch) {
	 if (intel->do_usleeps) 
	    DO_USLEEP( 1 );
      }
   }
}

/*
 * Copy the back buffer to the front buffer. 
 */
void intelCopyBuffer( const __DRIdrawablePrivate *dPriv,
		      const drm_clip_rect_t	 *rect)
{
   intelContextPtr intel;
   const intelScreenPrivate *intelScreen;
   GLboolean   missed_target;
   int64_t ust;

   if (0)
      fprintf(stderr, "%s\n", __FUNCTION__);

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   intel = (intelContextPtr) dPriv->driContextPriv->driverPrivate;

   intelFlush( &intel->ctx );
   
   intelScreen = intel->intelScreen;

   if (!rect && !intel->swap_scheduled && intelScreen->drmMinor >= 6 &&
       !(intel->vblank_flags & VBLANK_FLAG_NO_IRQ) &&
       intelScreen->current_rotation == 0) {
      unsigned int interval = driGetVBlankInterval(dPriv, intel->vblank_flags);
      unsigned int target;
      drm_i915_vblank_swap_t swap;

      swap.drawable = dPriv->hHWDrawable;
      swap.seqtype = DRM_VBLANK_ABSOLUTE;
      target = swap.sequence = intel->vbl_seq + interval;

      if (intel->vblank_flags & VBLANK_FLAG_SYNC) {
	 swap.seqtype |= DRM_VBLANK_NEXTONMISS;
      } else if (interval == 0) {
	 goto noschedule;
      }

      if ( intel->vblank_flags & VBLANK_FLAG_SECONDARY ) {
	 swap.seqtype |= DRM_VBLANK_SECONDARY;
      }

      if (!drmCommandWriteRead(intel->driFd, DRM_I915_VBLANK_SWAP, &swap,
                              sizeof(swap))) {
        intel->swap_scheduled = 1;
        intel->vbl_seq = swap.sequence;
        swap.sequence -= target;
        missed_target = swap.sequence > 0 && swap.sequence <= (1 << 23);
      }
   } else {
      intel->swap_scheduled = 0;
   }
noschedule:

   if (!intel->swap_scheduled) {
      intelWaitForFrameCompletion( intel );
      LOCK_HARDWARE( intel );

      if (!rect)
      {
	 UNLOCK_HARDWARE( intel );
	 driWaitForVBlank( dPriv, &intel->vbl_seq, intel->vblank_flags, & missed_target );
	 LOCK_HARDWARE( intel );
      }
      {
	 const intelScreenPrivate *intelScreen = intel->intelScreen;
	 const __DRIdrawablePrivate *dPriv = intel->driDrawable;
	 const int nbox = dPriv->numClipRects;
	 const drm_clip_rect_t *pbox = dPriv->pClipRects;
	 drm_clip_rect_t box;
	 const int cpp = intelScreen->cpp;
	 const int pitch = intelScreen->front.pitch; /* in bytes */
	 int i;
	 GLuint CMD, BR13;
	 BATCH_LOCALS;

	 switch(cpp) {
	 case 2: 
	    BR13 = (pitch) | (0xCC << 16) | (1<<24);
	    CMD = XY_SRC_COPY_BLT_CMD;
	    break;
	 case 4:
	    BR13 = (pitch) | (0xCC << 16) | (1<<24) | (1<<25);
	    CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
		   XY_SRC_COPY_BLT_WRITE_RGB);
	    break;
	 default:
	    BR13 = (pitch) | (0xCC << 16) | (1<<24);
	    CMD = XY_SRC_COPY_BLT_CMD;
	    break;
	 }
   
	 if (0) 
	    intel_draw_performance_boxes( intel );

	 for (i = 0 ; i < nbox; i++, pbox++) 
	 {
	    if (pbox->x1 > pbox->x2 ||
		pbox->y1 > pbox->y2 ||
		pbox->x2 > intelScreen->width ||
		pbox->y2 > intelScreen->height) {
	       _mesa_warning(&intel->ctx, "Bad cliprect in intelCopyBuffer()");
	       continue;
	    }

	    box = *pbox;

	    if (rect)
	    {
	       if (rect->x1 > box.x1)
		  box.x1 = rect->x1;
	       if (rect->y1 > box.y1)
		  box.y1 = rect->y1;
	       if (rect->x2 < box.x2)
		  box.x2 = rect->x2;
	       if (rect->y2 < box.y2)
		  box.y2 = rect->y2;

	       if (box.x1 > box.x2 || box.y1 > box.y2)
		  continue;
	    }

	    BEGIN_BATCH( 8);
	    OUT_BATCH( CMD );
	    OUT_BATCH( BR13 );
	    OUT_BATCH( (box.y1 << 16) | box.x1 );
	    OUT_BATCH( (box.y2 << 16) | box.x2 );

	    if (intel->sarea->pf_current_page == 0) 
	       OUT_BATCH( intelScreen->front.offset );
	    else
	       OUT_BATCH( intelScreen->back.offset );			

	    OUT_BATCH( (box.y1 << 16) | box.x1 );
	    OUT_BATCH( BR13 & 0xffff );

	    if (intel->sarea->pf_current_page == 0) 
	       OUT_BATCH( intelScreen->back.offset );			
	    else
	       OUT_BATCH( intelScreen->front.offset );

	    ADVANCE_BATCH();
	 }
      }
      intelFlushBatchLocked( intel, GL_TRUE, GL_TRUE, GL_TRUE );
      UNLOCK_HARDWARE( intel );
   }

   if (!rect)
   {
       intel->swap_count++;
       (*dri_interface->getUST)(&ust);
       if (missed_target) {
	   intel->swap_missed_count++;
	   intel->swap_missed_ust = ust -  intel->swap_ust;
       }
   
       intel->swap_ust = ust;
   }
}




void intelEmitFillBlitLocked( intelContextPtr intel,
			      GLuint cpp,
			      GLshort dst_pitch,  /* in bytes */
			      GLuint dst_offset,
			      GLshort x, GLshort y, 
			      GLshort w, GLshort h,
			      GLuint color )
{
   GLuint BR13, CMD;
   BATCH_LOCALS;

   switch(cpp) {
   case 1: 
   case 2: 
   case 3: 
      BR13 = dst_pitch | (0xF0 << 16) | (1<<24);
      CMD = XY_COLOR_BLT_CMD;
      break;
   case 4:
      BR13 = dst_pitch | (0xF0 << 16) | (1<<24) | (1<<25);
      CMD = (XY_COLOR_BLT_CMD | XY_COLOR_BLT_WRITE_ALPHA |
	     XY_COLOR_BLT_WRITE_RGB);
      break;
   default:
      return;
   }

   BEGIN_BATCH( 6);
   OUT_BATCH( CMD );
   OUT_BATCH( BR13 );
   OUT_BATCH( (y << 16) | x );
   OUT_BATCH( ((y+h) << 16) | (x+w) );
   OUT_BATCH( dst_offset );
   OUT_BATCH( color );
   ADVANCE_BATCH();
}


/* Copy BitBlt
 */
void intelEmitCopyBlitLocked( intelContextPtr intel,
			      GLuint cpp,
			      GLshort src_pitch,
			      GLuint  src_offset,
			      GLshort dst_pitch,
			      GLuint  dst_offset,
			      GLshort src_x, GLshort src_y,
			      GLshort dst_x, GLshort dst_y,
			      GLshort w, GLshort h )
{
   GLuint CMD, BR13;
   int dst_y2 = dst_y + h;
   int dst_x2 = dst_x + w;
   BATCH_LOCALS;

   src_pitch *= cpp;
   dst_pitch *= cpp;

   switch(cpp) {
   case 1: 
   case 2: 
   case 3: 
      BR13 = dst_pitch | (0xCC << 16) | (1<<24);
      CMD = XY_SRC_COPY_BLT_CMD;
      break;
   case 4:
      BR13 = dst_pitch | (0xCC << 16) | (1<<24) | (1<<25);
      CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
	     XY_SRC_COPY_BLT_WRITE_RGB);
      break;
   default:
      return;
   }

   if (dst_y2 < dst_y ||
       dst_x2 < dst_x) {
      return;
   }

   BEGIN_BATCH( 12);
   OUT_BATCH( CMD );
   OUT_BATCH( BR13 );
   OUT_BATCH( (dst_y << 16) | dst_x );
   OUT_BATCH( (dst_y2 << 16) | dst_x2 );
   OUT_BATCH( dst_offset );	
   OUT_BATCH( (src_y << 16) | src_x );
   OUT_BATCH( src_pitch );
   OUT_BATCH( src_offset ); 
   ADVANCE_BATCH();
}



void intelClearWithBlit(GLcontext *ctx, GLbitfield buffers, GLboolean allFoo,
                        GLint cx1Foo, GLint cy1Foo, GLint cwFoo, GLint chFoo)
{
   intelContextPtr intel = INTEL_CONTEXT( ctx );
   intelScreenPrivate *intelScreen = intel->intelScreen;
   GLuint clear_depth, clear_color;
   GLint cx, cy, cw, ch;
   GLboolean all;
   GLint pitch;
   GLint cpp = intelScreen->cpp;
   GLint i;
   GLuint BR13, CMD, D_CMD;
   BATCH_LOCALS;

   intelFlush( &intel->ctx );
   LOCK_HARDWARE( intel );

   /* get clear bounds after locking */
   cx = intel->ctx.DrawBuffer->_Xmin;
   cy = intel->ctx.DrawBuffer->_Ymin;
   cw = intel->ctx.DrawBuffer->_Xmax - cx;
   ch = intel->ctx.DrawBuffer->_Ymax - cy;
   all = (cw == intel->ctx.DrawBuffer->Width &&
          ch == intel->ctx.DrawBuffer->Height);

   pitch = intelScreen->front.pitch;

   clear_color = intel->ClearColor;
   clear_depth = 0;

   if (buffers & BUFFER_BIT_DEPTH) {
      clear_depth = (GLuint)(ctx->Depth.Clear * intel->ClearDepth);
   }

   if (buffers & BUFFER_BIT_STENCIL) {
      clear_depth |= (ctx->Stencil.Clear & 0xff) << 24;
   }

   switch(cpp) {
   case 2: 
      BR13 = (0xF0 << 16) | (pitch) | (1<<24);
      D_CMD = CMD = XY_COLOR_BLT_CMD;
      break;
   case 4:
      BR13 = (0xF0 << 16) | (pitch) | (1<<24) | (1<<25);
      CMD = (XY_COLOR_BLT_CMD |
	     XY_COLOR_BLT_WRITE_ALPHA | 
	     XY_COLOR_BLT_WRITE_RGB);
      D_CMD = XY_COLOR_BLT_CMD;
      if (buffers & BUFFER_BIT_DEPTH) D_CMD |= XY_COLOR_BLT_WRITE_RGB;
      if (buffers & BUFFER_BIT_STENCIL) D_CMD |= XY_COLOR_BLT_WRITE_ALPHA;
      break;
   default:
      BR13 = (0xF0 << 16) | (pitch) | (1<<24);
      D_CMD = CMD = XY_COLOR_BLT_CMD;
      break;
   }

   {
      /* flip top to bottom */
      cy = intel->driDrawable->h - cy - ch;
      cx = cx + intel->drawX;
      cy += intel->drawY;

      /* adjust for page flipping */
      if ( intel->sarea->pf_current_page == 1 ) {
	 GLuint tmp = buffers;

	 buffers &= ~(BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT);
	 if ( tmp & BUFFER_BIT_FRONT_LEFT ) buffers |= BUFFER_BIT_BACK_LEFT;
	 if ( tmp & BUFFER_BIT_BACK_LEFT )  buffers |= BUFFER_BIT_FRONT_LEFT;
      }

      for (i = 0 ; i < intel->numClipRects ; i++) 
      { 	 
	 drm_clip_rect_t *box = &intel->pClipRects[i];	 
	 drm_clip_rect_t b;

	 if (!all) {
	    GLint x = box->x1;
	    GLint y = box->y1;
	    GLint w = box->x2 - x;
	    GLint h = box->y2 - y;

	    if (x < cx) w -= cx - x, x = cx; 
	    if (y < cy) h -= cy - y, y = cy;
	    if (x + w > cx + cw) w = cx + cw - x;
	    if (y + h > cy + ch) h = cy + ch - y;
	    if (w <= 0) continue;
	    if (h <= 0) continue;

	    b.x1 = x;
	    b.y1 = y;
	    b.x2 = x + w;
	    b.y2 = y + h;      
	 } else {
	    b = *box;
	 }


	 if (b.x1 > b.x2 ||
	     b.y1 > b.y2 ||
	     b.x2 > intelScreen->width ||
	     b.y2 > intelScreen->height)
	    continue;

	 if ( buffers & BUFFER_BIT_FRONT_LEFT ) {	    
	    BEGIN_BATCH( 6);	    
	    OUT_BATCH( CMD );
	    OUT_BATCH( BR13 );
	    OUT_BATCH( (b.y1 << 16) | b.x1 );
	    OUT_BATCH( (b.y2 << 16) | b.x2 );
	    OUT_BATCH( intelScreen->front.offset );
	    OUT_BATCH( clear_color );
	    ADVANCE_BATCH();
	 }

	 if ( buffers & BUFFER_BIT_BACK_LEFT ) {
	    BEGIN_BATCH( 6); 
	    OUT_BATCH( CMD );
	    OUT_BATCH( BR13 );
	    OUT_BATCH( (b.y1 << 16) | b.x1 );
	    OUT_BATCH( (b.y2 << 16) | b.x2 );
	    OUT_BATCH( intelScreen->back.offset );
	    OUT_BATCH( clear_color );
	    ADVANCE_BATCH();
	 }

	 if ( buffers & (BUFFER_BIT_STENCIL | BUFFER_BIT_DEPTH) ) {
	    BEGIN_BATCH( 6);
	    OUT_BATCH( D_CMD );
	    OUT_BATCH( BR13 );
	    OUT_BATCH( (b.y1 << 16) | b.x1 );
	    OUT_BATCH( (b.y2 << 16) | b.x2 );
	    OUT_BATCH( intelScreen->depth.offset );
	    OUT_BATCH( clear_depth );
	    ADVANCE_BATCH();
	 }      
      }
   }
   intelFlushBatchLocked( intel, GL_TRUE, GL_FALSE, GL_TRUE );
   UNLOCK_HARDWARE( intel );
}




void intelDestroyBatchBuffer( GLcontext *ctx )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);

   if (intel->alloc.offset) {
      intelFreeAGP( intel, intel->alloc.ptr );
      intel->alloc.ptr = NULL;
      intel->alloc.offset = 0;
   }
   else if (intel->alloc.ptr) {
      free(intel->alloc.ptr);
      intel->alloc.ptr = NULL;
   }

   memset(&intel->batch, 0, sizeof(intel->batch));
}


void intelInitBatchBuffer( GLcontext *ctx )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);

   /* This path isn't really safe with rotate:
    */
   if (getenv("INTEL_BATCH") && intel->intelScreen->allow_batchbuffer) {      
      switch (intel->intelScreen->deviceID) {
      case PCI_CHIP_I865_G:
	 /* HW bug?  Seems to crash if batchbuffer crosses 4k boundary.
	  */
	 intel->alloc.size = 8 * 1024; 
	 break;
      default:
	 /* This is the smallest amount of memory the kernel deals with.
	  * We'd ideally like to make this smaller.
	  */
	 intel->alloc.size = 1 << intel->intelScreen->logTextureGranularity;
	 break;
      }

      intel->alloc.ptr = intelAllocateAGP( intel, intel->alloc.size );
      if (intel->alloc.ptr)
	 intel->alloc.offset = 
	    intelAgpOffsetFromVirtual( intel, intel->alloc.ptr );
      else
         intel->alloc.offset = 0; /* OK? */
   }

   /* The default is now to use a local buffer and pass that to the
    * kernel.  This is also a fallback if allocation fails on the
    * above path:
    */
   if (!intel->alloc.ptr) {
      intel->alloc.size = 8 * 1024;
      intel->alloc.ptr = malloc( intel->alloc.size );
      intel->alloc.offset = 0;
   }

   assert(intel->alloc.ptr);
}
