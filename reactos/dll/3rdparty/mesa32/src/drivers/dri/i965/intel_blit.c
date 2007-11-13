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
#include "intel_blit.h"
#include "intel_regions.h"
#include "intel_structs.h"

#include "bufmgr.h"




/*
 * Copy the back buffer to the front buffer. 
 */
void intelCopyBuffer( const __DRIdrawablePrivate *dPriv,
		      const drm_clip_rect_t *rect ) 
{
   struct intel_context *intel;
   GLboolean   missed_target;
   int64_t ust;

   DBG("%s\n", __FUNCTION__);

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   intel = (struct intel_context *) dPriv->driContextPriv->driverPrivate;
   intelFlush( &intel->ctx );


   bmFinishFenceLock(intel, intel->last_swap_fence);

   /* The LOCK_HARDWARE is required for the cliprects.  Buffer offsets
    * should work regardless.
    */
   LOCK_HARDWARE( intel );

   if (!rect)
   {
       UNLOCK_HARDWARE( intel );
       driWaitForVBlank( dPriv, &intel->vbl_seq, intel->vblank_flags, & missed_target );
       LOCK_HARDWARE( intel );
   }

   {
      intelScreenPrivate *intelScreen = intel->intelScreen;
      __DRIdrawablePrivate *dPriv = intel->driDrawable;
      int nbox = dPriv->numClipRects;
      drm_clip_rect_t *pbox = dPriv->pClipRects;
      int cpp = intelScreen->cpp;
      struct intel_region *src, *dst;
      int BR13, CMD;
      int i;
      int src_pitch, dst_pitch;

      if (intel->sarea->pf_current_page == 0) {
	 dst = intel->front_region;
	 src = intel->back_region;
      }
      else {
	 assert(0);
	 src = intel->front_region;
	 dst = intel->back_region;
      }

      src_pitch = src->pitch * src->cpp;
      dst_pitch = dst->pitch * dst->cpp;

      if (cpp == 2) {
	 BR13 = (0xCC << 16) | (1<<24);
	 CMD = XY_SRC_COPY_BLT_CMD;
      } 
      else {
	 BR13 = (0xCC << 16) | (1<<24) | (1<<25);
	 CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
		XY_SRC_COPY_BLT_WRITE_RGB);
      }

      if (src->tiled) {
	 CMD |= XY_SRC_TILED;
	 src_pitch /= 4;
      }
      
      if (dst->tiled) {
	 CMD |= XY_DST_TILED;
 	 dst_pitch /= 4;
      }
  
      for (i = 0 ; i < nbox; i++, pbox++) 
      {
	 drm_clip_rect_t tmp = *pbox;

	 if (rect) {
	    if (!intel_intersect_cliprects(&tmp, &tmp, rect))
	       continue;
	 }


	 if (tmp.x1 > tmp.x2 ||
	     tmp.y1 > tmp.y2 ||
	     tmp.x2 > intelScreen->width ||
	     tmp.y2 > intelScreen->height)
	    continue;
 
	 BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
	 OUT_BATCH( CMD );
	 OUT_BATCH( dst_pitch | BR13 );
	 OUT_BATCH( (tmp.y1 << 16) | tmp.x1 );
	 OUT_BATCH( (tmp.y2 << 16) | tmp.x2 );
	 OUT_BATCH( bmBufferOffset(intel, dst->buffer) );
	 OUT_BATCH( (tmp.y1 << 16) | tmp.x1 );
	 OUT_BATCH( src_pitch );
	 OUT_BATCH( bmBufferOffset(intel, src->buffer) ); 
	 ADVANCE_BATCH();
      }
   }

   intel_batchbuffer_flush( intel->batch );
   intel->second_last_swap_fence = intel->last_swap_fence;
   intel->last_swap_fence = bmSetFenceLock( intel );
   UNLOCK_HARDWARE( intel );

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




void intelEmitFillBlit( struct intel_context *intel,
			GLuint cpp,
			GLshort dst_pitch,
			struct buffer *dst_buffer,
			GLuint dst_offset,
			GLboolean dst_tiled,
			GLshort x, GLshort y, 
			GLshort w, GLshort h,
			GLuint color )
{
   GLuint BR13, CMD;
   BATCH_LOCALS;

   dst_pitch *= cpp;

   switch(cpp) {
   case 1: 
   case 2: 
   case 3: 
      BR13 = (0xF0 << 16) | (1<<24);
      CMD = XY_COLOR_BLT_CMD;
      break;
   case 4:
      BR13 = (0xF0 << 16) | (1<<24) | (1<<25);
      CMD = (XY_COLOR_BLT_CMD | XY_COLOR_BLT_WRITE_ALPHA |
	     XY_COLOR_BLT_WRITE_RGB);
      break;
   default:
      return;
   }

   if (dst_tiled) {
      CMD |= XY_DST_TILED;
      dst_pitch /= 4;
   }

   BEGIN_BATCH(6, INTEL_BATCH_NO_CLIPRECTS);
   OUT_BATCH( CMD );
   OUT_BATCH( dst_pitch | BR13 );
   OUT_BATCH( (y << 16) | x );
   OUT_BATCH( ((y+h) << 16) | (x+w) );
   OUT_BATCH( bmBufferOffset(intel, dst_buffer) + dst_offset );
   OUT_BATCH( color );
   ADVANCE_BATCH();
}

static GLuint translate_raster_op(GLenum logicop)
{
   switch(logicop) {
   case GL_CLEAR: return 0x00;
   case GL_AND: return 0x88;
   case GL_AND_REVERSE: return 0x44;
   case GL_COPY: return 0xCC;
   case GL_AND_INVERTED: return 0x22;
   case GL_NOOP: return 0xAA;
   case GL_XOR: return 0x66;
   case GL_OR: return 0xEE;
   case GL_NOR: return 0x11;
   case GL_EQUIV: return 0x99;
   case GL_INVERT: return 0x55;
   case GL_OR_REVERSE: return 0xDD;
   case GL_COPY_INVERTED: return 0x33;
   case GL_OR_INVERTED: return 0xBB;
   case GL_NAND: return 0x77;
   case GL_SET: return 0xFF;
   default: return 0;
   }
}


/* Copy BitBlt
 */
void intelEmitCopyBlit( struct intel_context *intel,
			GLuint cpp,
			GLshort src_pitch,
			struct buffer *src_buffer,
			GLuint  src_offset,
			GLboolean src_tiled,
			GLshort dst_pitch,
			struct buffer *dst_buffer,
			GLuint  dst_offset,
			GLboolean dst_tiled,
			GLshort src_x, GLshort src_y,
			GLshort dst_x, GLshort dst_y,
			GLshort w, GLshort h,
			GLenum logic_op )
{
   GLuint CMD, BR13;
   int dst_y2 = dst_y + h;
   int dst_x2 = dst_x + w;
   BATCH_LOCALS;


   DBG("%s src:buf(%d)/%d %d,%d dst:buf(%d)/%d %d,%d sz:%dx%d op:%d\n",
       __FUNCTION__,
       src_buffer, src_pitch, src_x, src_y,
       dst_buffer, dst_pitch, dst_x, dst_y,
       w,h,logic_op);

   assert( logic_op - GL_CLEAR >= 0 );
   assert( logic_op - GL_CLEAR < 0x10 );
      
   src_pitch *= cpp;
   dst_pitch *= cpp;

   switch(cpp) {
   case 1: 
   case 2: 
   case 3: 
      BR13 = (translate_raster_op(logic_op) << 16) | (1<<24);
      CMD = XY_SRC_COPY_BLT_CMD;
      break;
   case 4:
      BR13 = (translate_raster_op(logic_op) << 16) | (1<<24) |
	  (1<<25);
      CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
	     XY_SRC_COPY_BLT_WRITE_RGB);
      break;
   default:
      return;
   }

   if (src_tiled) {
      CMD |= XY_SRC_TILED;
      src_pitch /= 4;
   }
   
   if (dst_tiled) {
      CMD |= XY_DST_TILED;
      dst_pitch /= 4;
   }

   if (dst_y2 < dst_y ||
       dst_x2 < dst_x) {
      return;
   }

   dst_pitch &= 0xffff;
   src_pitch &= 0xffff;

   /* Initial y values don't seem to work with negative pitches.  If
    * we adjust the offsets manually (below), it seems to work fine.
    *
    * On the other hand, if we always adjust, the hardware doesn't
    * know which blit directions to use, so overlapping copypixels get
    * the wrong result.
    */
   if (dst_pitch > 0 && src_pitch > 0) {
      BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
      OUT_BATCH( CMD );
      OUT_BATCH( dst_pitch | BR13 );
      OUT_BATCH( (dst_y << 16) | dst_x );
      OUT_BATCH( (dst_y2 << 16) | dst_x2 );
      OUT_BATCH( bmBufferOffset(intel, dst_buffer) + dst_offset );	
      OUT_BATCH( (src_y << 16) | src_x );
      OUT_BATCH( src_pitch );
      OUT_BATCH( bmBufferOffset(intel, src_buffer) + src_offset ); 
      ADVANCE_BATCH();
   }
   else {
      BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
      OUT_BATCH( CMD );
      OUT_BATCH( (dst_pitch & 0xffff) | BR13 );
      OUT_BATCH( (0 << 16) | dst_x );
      OUT_BATCH( (h << 16) | dst_x2 );
      OUT_BATCH( bmBufferOffset(intel, dst_buffer) + dst_offset + dst_y * dst_pitch );	
      OUT_BATCH( (0 << 16) | src_x );
      OUT_BATCH( (src_pitch & 0xffff) );
      OUT_BATCH( bmBufferOffset(intel, src_buffer) + src_offset + src_y * src_pitch ); 
      ADVANCE_BATCH();
   }
}



void intelClearWithBlit(GLcontext *ctx, GLbitfield flags)
{
   struct intel_context *intel = intel_context( ctx );
   intelScreenPrivate *intelScreen = intel->intelScreen;
   GLuint clear_depth, clear_color;
   GLint cx, cy, cw, ch;
   GLint cpp = intelScreen->cpp;
   GLboolean all;
   GLint i;
   struct intel_region *front = intel->front_region;
   struct intel_region *back = intel->back_region;
   struct intel_region *depth = intel->depth_region;
   GLuint BR13, FRONT_CMD, BACK_CMD, DEPTH_CMD;
   GLuint front_pitch;
   GLuint back_pitch;
   GLuint depth_pitch;
   BATCH_LOCALS;

   
   clear_color = intel->ClearColor;
   clear_depth = 0;

   if (flags & BUFFER_BIT_DEPTH) {
      clear_depth = (GLuint)(ctx->Depth.Clear * intel->ClearDepth);
   }

   if (flags & BUFFER_BIT_STENCIL) {
      clear_depth |= (ctx->Stencil.Clear & 0xff) << 24;
   }

   switch(cpp) {
   case 2: 
      BR13 = (0xF0 << 16) | (1<<24);
      BACK_CMD  = FRONT_CMD = XY_COLOR_BLT_CMD;
      DEPTH_CMD = XY_COLOR_BLT_CMD;
      break;
   case 4:
      BR13 = (0xF0 << 16) | (1<<24) | (1<<25);
      BACK_CMD = FRONT_CMD = (XY_COLOR_BLT_CMD |
			      XY_COLOR_BLT_WRITE_ALPHA | 
			      XY_COLOR_BLT_WRITE_RGB);
      DEPTH_CMD = XY_COLOR_BLT_CMD;
      if (flags & BUFFER_BIT_DEPTH) DEPTH_CMD |= XY_COLOR_BLT_WRITE_RGB;
      if (flags & BUFFER_BIT_STENCIL) DEPTH_CMD |= XY_COLOR_BLT_WRITE_ALPHA;
      break;
   default:
      return;
   }



   intelFlush( &intel->ctx );
   LOCK_HARDWARE( intel );
   {
      /* get clear bounds after locking */
      cx = ctx->DrawBuffer->_Xmin;
      cy = ctx->DrawBuffer->_Ymin;
      ch = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
      cw = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
      all = (cw == ctx->DrawBuffer->Width && ch == ctx->DrawBuffer->Height);

      /* flip top to bottom */
      cy = intel->driDrawable->h - cy - ch;
      cx = cx + intel->drawX;
      cy += intel->drawY;

      /* adjust for page flipping */
      if ( intel->sarea->pf_current_page == 0 ) {
	 front = intel->front_region;
	 back = intel->back_region;
      } 
      else {
	 back = intel->front_region;
	 front = intel->back_region;
      }
      
      front_pitch = front->pitch * front->cpp;
      back_pitch = back->pitch * back->cpp;
      depth_pitch = depth->pitch * depth->cpp;
      
      if (front->tiled) {
	 FRONT_CMD |= XY_DST_TILED;
	 front_pitch /= 4;
      }

      if (back->tiled) {
	 BACK_CMD |= XY_DST_TILED;
	 back_pitch /= 4;
      }

      if (depth->tiled) {
	 DEPTH_CMD |= XY_DST_TILED;
	 depth_pitch /= 4;
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

	 if ( flags & BUFFER_BIT_FRONT_LEFT ) {	    
	    BEGIN_BATCH(6, INTEL_BATCH_NO_CLIPRECTS);
	    OUT_BATCH( FRONT_CMD );
	    OUT_BATCH( front_pitch | BR13 );
	    OUT_BATCH( (b.y1 << 16) | b.x1 );
	    OUT_BATCH( (b.y2 << 16) | b.x2 );
	    OUT_BATCH( bmBufferOffset(intel, front->buffer) );
	    OUT_BATCH( clear_color );
	    ADVANCE_BATCH();
	 }

	 if ( flags & BUFFER_BIT_BACK_LEFT ) {
	    BEGIN_BATCH(6, INTEL_BATCH_NO_CLIPRECTS); 
	    OUT_BATCH( BACK_CMD );
	    OUT_BATCH( back_pitch | BR13 );
	    OUT_BATCH( (b.y1 << 16) | b.x1 );
	    OUT_BATCH( (b.y2 << 16) | b.x2 );
	    OUT_BATCH( bmBufferOffset(intel, back->buffer) );
	    OUT_BATCH( clear_color );
	    ADVANCE_BATCH();
	 }

	 if ( flags & (BUFFER_BIT_STENCIL | BUFFER_BIT_DEPTH) ) {
	    BEGIN_BATCH(6, INTEL_BATCH_NO_CLIPRECTS);
	    OUT_BATCH( DEPTH_CMD );
	    OUT_BATCH( depth_pitch | BR13 );
	    OUT_BATCH( (b.y1 << 16) | b.x1 );
	    OUT_BATCH( (b.y2 << 16) | b.x2 );
	    OUT_BATCH( bmBufferOffset(intel, depth->buffer) );
	    OUT_BATCH( clear_depth );
	    ADVANCE_BATCH();
	 }      
      }
   }
   intel_batchbuffer_flush( intel->batch );
   UNLOCK_HARDWARE( intel );
}



#define BR13_565  0x1
#define BR13_8888 0x3


void
intelEmitImmediateColorExpandBlit(struct intel_context *intel,
				  GLuint cpp,
				  GLubyte *src_bits, GLuint src_size,
				  GLuint fg_color,
				  GLshort dst_pitch,
				  struct buffer *dst_buffer,
				  GLuint dst_offset,
				  GLboolean dst_tiled,
				  GLshort x, GLshort y, 
				  GLshort w, GLshort h,
				  GLenum logic_op)
{
   struct xy_setup_blit setup;
   struct xy_text_immediate_blit text;
   int dwords = ((src_size + 7) & ~7) / 4;

   assert( logic_op - GL_CLEAR >= 0 );
   assert( logic_op - GL_CLEAR < 0x10 );

   if (w < 0 || h < 0) 
      return;

   dst_pitch *= cpp;

   if (dst_tiled) 
      dst_pitch /= 4;

   DBG("%s dst:buf(%p)/%d+%d %d,%d sz:%dx%d, %d bytes %d dwords\n",
       __FUNCTION__,
       dst_buffer, dst_pitch, dst_offset, x, y, w, h, src_size, dwords);

   memset(&setup, 0, sizeof(setup));
   
   setup.br0.client = CLIENT_2D;
   setup.br0.opcode = OPCODE_XY_SETUP_BLT;
   setup.br0.write_alpha = (cpp == 4);
   setup.br0.write_rgb = (cpp == 4);
   setup.br0.dst_tiled = dst_tiled;
   setup.br0.length = (sizeof(setup) / sizeof(int)) - 2;
      
   setup.br13.dest_pitch = dst_pitch;
   setup.br13.rop = translate_raster_op(logic_op);
   setup.br13.color_depth = (cpp == 4) ? BR13_8888 : BR13_565;
   setup.br13.clipping_enable = 0;
   setup.br13.mono_source_transparency = 1;

   setup.dw2.clip_y1 = 0;
   setup.dw2.clip_x1 = 0;
   setup.dw3.clip_y2 = 100;
   setup.dw3.clip_x2 = 100;

   setup.dest_base_addr = bmBufferOffset(intel, dst_buffer) + dst_offset;
   setup.background_color = 0;
   setup.foreground_color = fg_color;
   setup.pattern_base_addr = 0;

   memset(&text, 0, sizeof(text));
   text.dw0.client = CLIENT_2D;
   text.dw0.opcode = OPCODE_XY_TEXT_IMMEDIATE_BLT;
   text.dw0.pad0 = 0;
   text.dw0.byte_packed = 1;	/* ?maybe? */
   text.dw0.pad1 = 0;
   text.dw0.dst_tiled = dst_tiled;
   text.dw0.pad2 = 0;
   text.dw0.length = (sizeof(text)/sizeof(int)) - 2 + dwords;
   text.dw1.dest_y1 = y;	/* duplicates info in setup blit */
   text.dw1.dest_x1 = x;
   text.dw2.dest_y2 = y + h;
   text.dw2.dest_x2 = x + w;

   intel_batchbuffer_require_space( intel->batch,
				    sizeof(setup) + 
				    sizeof(text) + 
				    dwords,
				    INTEL_BATCH_NO_CLIPRECTS );

   intel_batchbuffer_data( intel->batch,
			   &setup,
			   sizeof(setup),
			   INTEL_BATCH_NO_CLIPRECTS );

   intel_batchbuffer_data( intel->batch,
			   &text,
			   sizeof(text),
			   INTEL_BATCH_NO_CLIPRECTS );

   intel_batchbuffer_data( intel->batch,
			   src_bits,
			   dwords * 4,
			   INTEL_BATCH_NO_CLIPRECTS );
}

