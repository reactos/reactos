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

#ifndef BRW_DRAW_H
#define BRW_DRAW_H

#include "mtypes.h"		/* for GLcontext... */
#include "vbo/vbo.h"

struct brw_context;


void brw_draw_prims( GLcontext *ctx,
		     const struct gl_client_array *arrays[],
		     const struct _mesa_prim *prims,
		     GLuint nr_prims,
		     const struct _mesa_index_buffer *ib,
		     GLuint min_index,
		     GLuint max_index );

void brw_draw_init( struct brw_context *brw );
void brw_draw_destroy( struct brw_context *brw );

/* brw_draw_current.c
 */
void brw_init_current_values(GLcontext *ctx,
			     struct gl_client_array *arrays);


/* brw_draw_upload.c
 */
void brw_upload_indices( struct brw_context *brw,
			 const struct _mesa_index_buffer *index_buffer);

GLboolean brw_upload_vertices( struct brw_context *brw,
			       GLuint min_index,
			       GLuint max_index );



#endif
