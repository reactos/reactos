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

#ifndef INTEL_BLIT_H
#define INTEL_BLIT_H

#include "intel_context.h"
#include "intel_ioctl.h"

struct buffer;

extern void intelCopyBuffer( const __DRIdrawablePrivate *dpriv,
			     const drm_clip_rect_t *rect );
extern void intelClearWithBlit(GLcontext *ctx, GLbitfield mask);

extern void intelEmitCopyBlit( struct intel_context *intel,
			       GLuint cpp,
			       GLshort src_pitch,
			       struct buffer *src_buffer,
			       GLuint  src_offset,
			       GLboolean src_tiled,
			       GLshort dst_pitch,
			       struct buffer *dst_buffer,
			       GLuint  dst_offset,
			       GLboolean dst_tiled,
			       GLshort srcx, GLshort srcy,
			       GLshort dstx, GLshort dsty,
			       GLshort w, GLshort h,
			       GLenum logic_op );

extern void intelEmitFillBlit( struct intel_context *intel,
			       GLuint cpp,
			       GLshort dst_pitch,
			       struct buffer *dst_buffer,
			       GLuint dst_offset,
			       GLboolean dst_tiled,
			       GLshort x, GLshort y, 
			       GLshort w, GLshort h,
			       GLuint color );

void
intelEmitImmediateColorExpandBlit(struct intel_context *intel,
				  GLuint cpp,
				  GLubyte *src_bits, GLuint src_size,
				  GLuint fg_color,
				  GLshort dst_pitch,
				  struct buffer *dst_buffer,
				  GLuint dst_offset,
				  GLboolean dst_tiled,
				  GLshort dst_x, GLshort dst_y, 
				  GLshort w, GLshort h,
				  GLenum logic_op );

#endif
