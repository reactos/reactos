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

#ifndef INTEL_REGIONS_H
#define INTEL_REGIONS_H

#include "mtypes.h"
#include "bufmgr.h"		/* for DBG! */
struct intel_context;

/* A layer on top of the bufmgr buffers that adds a few useful things:
 *
 * - Refcounting for local buffer references.
 * - Refcounting for buffer maps
 * - Buffer dimensions - pitch and height.
 * - Blitter commands for copying 2D regions between buffers.
 */
struct intel_region {
   struct buffer *buffer;
   GLuint refcount;
   GLuint cpp;
   GLuint pitch;
   GLuint height;
   GLboolean tiled;
   GLubyte *map;
   GLuint map_refcount;
};

/* Allocate a refcounted region.  Pointers to regions should only be
 * copied by calling intel_reference_region().
 *
 * No support for dynamically allocating tiled regions at this point.
 */
struct intel_region *intel_region_alloc( struct intel_context *intel,
					 GLuint cpp,
					 GLuint pitch, 
					 GLuint height );

void intel_region_reference( struct intel_region **dst, 
			     struct intel_region *src );

void intel_region_release(struct intel_context *intel,
			  struct intel_region **ib );

/* Static regions may be tiled.  The assumption is that the X server
 * has set up fence registers to define tiled zones in agp and these
 * buffers are within those zones.  Tiling regions without fence
 * registers is more work.
 */
struct intel_region *intel_region_create_static( struct intel_context *intel,
						 GLuint mem_type,
						 GLuint offset,
						 void *virtual,
						 GLuint cpp,
						 GLuint pitch,
						 GLuint height,
						 GLuint size,
						 GLboolean tiled );

/* Map/unmap regions.  This is refcounted also: 
 */
GLubyte *intel_region_map(struct intel_context *intel, 
		       struct intel_region *ib);

void intel_region_unmap(struct intel_context *intel,
			struct intel_region *ib);


/* Upload data to a rectangular sub-region
 */
GLboolean intel_region_data(struct intel_context *intel, 
			    struct intel_region *dest,
			    GLuint dest_offset,
			    GLuint destx, GLuint desty,
			    const void *src, GLuint src_stride,
			    GLuint srcx, GLuint srcy,
			    GLuint width, GLuint height);
			  
/* Copy rectangular sub-regions
 */
void intel_region_copy( struct intel_context *intel,
			struct intel_region *dest,
			GLuint dest_offset,
			GLuint destx, GLuint desty,
			struct intel_region *src,
			GLuint src_offset,
			GLuint srcx, GLuint srcy,
			GLuint width, GLuint height );

/* Fill a rectangular sub-region
 */
void intel_region_fill( struct intel_context *intel,
			struct intel_region *dest,
			GLuint dest_offset,
			GLuint destx, GLuint desty,
			GLuint width, GLuint height,
			GLuint color );


/***********************************************************************
 * Misc utilities: move to somewhere generic
 */
void _mesa_copy_rect( GLubyte *dst,
		      GLuint cpp,
		      GLuint dst_pitch,
		      GLuint dst_x, 
		      GLuint dst_y,
		      GLuint width,
		      GLuint height,
		      const GLubyte *src,
		      GLuint src_pitch,
		      GLuint src_x,
		      GLuint src_y );


#endif
