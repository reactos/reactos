/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_draw_quad.h"
#include "util/u_memory.h"
#include "cso_cache/cso_context.h"


/**
 * Draw a simple vertex buffer / primitive.
 * Limited to float[4] vertex attribs, tightly packed.
 */
void 
util_draw_vertex_buffer(struct pipe_context *pipe,
                        struct cso_context *cso,
                        struct pipe_resource *vbuf,
                        uint offset,
                        uint prim_type,
                        uint num_verts,
                        uint num_attribs)
{
   struct pipe_vertex_buffer vbuffer;

   assert(num_attribs <= PIPE_MAX_ATTRIBS);

   /* tell pipe about the vertex buffer */
   memset(&vbuffer, 0, sizeof(vbuffer));
   vbuffer.buffer = vbuf;
   vbuffer.stride = num_attribs * 4 * sizeof(float);  /* vertex size */
   vbuffer.buffer_offset = offset;

   if (cso) {
      cso_set_vertex_buffers(cso, 1, &vbuffer);
   } else {
      pipe->set_vertex_buffers(pipe, 1, &vbuffer);
   }

   /* note: vertex elements already set by caller */

   /* draw */
   util_draw_arrays(pipe, prim_type, 0, num_verts);
}



/**
 * Draw screen-aligned textured quad.
 * Note: this isn't especially efficient.
 */
void 
util_draw_texquad(struct pipe_context *pipe, struct cso_context *cso,
                  float x0, float y0, float x1, float y1, float z)
{
   uint numAttribs = 2, i, j;
   uint vertexBytes = 4 * (4 * numAttribs * sizeof(float));
   struct pipe_resource *vbuf = NULL;  
   float *v = NULL;

   v = MALLOC(vertexBytes);
   if (v == NULL)
      goto out;

   /*
    * Load vertex buffer
    */
   for (i = j = 0; i < 4; i++) {
      v[j + 2] = z;   /* z */
      v[j + 3] = 1.0; /* w */
      v[j + 6] = 0.0; /* r */
      v[j + 7] = 1.0; /* q */
      j += 8;
   }

   v[0] = x0;
   v[1] = y0;
   v[4] = 0.0; /*s*/
   v[5] = 0.0; /*t*/

   v[8] = x1;
   v[9] = y0;
   v[12] = 1.0;
   v[13] = 0.0;

   v[16] = x1;
   v[17] = y1;
   v[20] = 1.0;
   v[21] = 1.0;

   v[24] = x0;
   v[25] = y1;
   v[28] = 0.0;
   v[29] = 1.0;
	 
   vbuf = pipe_user_buffer_create(pipe->screen, v, vertexBytes,
				  PIPE_BIND_VERTEX_BUFFER);
   if (!vbuf)
      goto out;

   util_draw_vertex_buffer(pipe, cso, vbuf, 0, PIPE_PRIM_TRIANGLE_FAN, 4, 2);

out:
   if (vbuf)
      pipe_resource_reference(&vbuf, NULL);
   
   if (v)
      FREE(v);
}
