/**************************************************************************
 *
 * Copyright 2010 Christian König
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

#include <assert.h>
#include "util/u_format.h"
#include "vl_vertex_buffers.h"
#include "vl_types.h"

/* vertices for a quad covering a block */
static const struct vertex2f block_quad[4] = {
   {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
};

struct pipe_vertex_buffer
vl_vb_upload_quads(struct pipe_context *pipe)
{
   struct pipe_vertex_buffer quad;
   struct pipe_transfer *buf_transfer;
   struct vertex2f *v;

   unsigned i;

   assert(pipe);

   /* create buffer */
   quad.stride = sizeof(struct vertex2f);
   quad.buffer_offset = 0;
   quad.buffer = pipe_buffer_create
   (
      pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      PIPE_USAGE_STATIC,
      sizeof(struct vertex2f) * 4
   );

   if(!quad.buffer)
      return quad;

   /* and fill it */
   v = pipe_buffer_map
   (
      pipe,
      quad.buffer,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD_RANGE,
      &buf_transfer
   );

   for (i = 0; i < 4; ++i, ++v) {
      v->x = block_quad[i].x;
      v->y = block_quad[i].y;
   }

   pipe_buffer_unmap(pipe, buf_transfer);

   return quad;
}

struct pipe_vertex_buffer
vl_vb_upload_pos(struct pipe_context *pipe, unsigned width, unsigned height)
{
   struct pipe_vertex_buffer pos;
   struct pipe_transfer *buf_transfer;
   struct vertex2s *v;

   unsigned x, y;

   assert(pipe);

   /* create buffer */
   pos.stride = sizeof(struct vertex2s);
   pos.buffer_offset = 0;
   pos.buffer = pipe_buffer_create
   (
      pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      PIPE_USAGE_STATIC,
      sizeof(struct vertex2s) * width * height
   );

   if(!pos.buffer)
      return pos;

   /* and fill it */
   v = pipe_buffer_map
   (
      pipe,
      pos.buffer,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD_RANGE,
      &buf_transfer
   );

   for ( y = 0; y < height; ++y) {
      for ( x = 0; x < width; ++x, ++v) {
         v->x = x;
         v->y = y;
      }
   }

   pipe_buffer_unmap(pipe, buf_transfer);

   return pos;
}

static struct pipe_vertex_element
vl_vb_get_quad_vertex_element(void)
{
   struct pipe_vertex_element element;

   /* setup rectangle element */
   element.src_offset = 0;
   element.instance_divisor = 0;
   element.vertex_buffer_index = 0;
   element.src_format = PIPE_FORMAT_R32G32_FLOAT;

   return element;
}

static void
vl_vb_element_helper(struct pipe_vertex_element* elements, unsigned num_elements,
                     unsigned vertex_buffer_index)
{
   unsigned i, offset = 0;

   assert(elements && num_elements);

   for ( i = 0; i < num_elements; ++i ) {
      elements[i].src_offset = offset;
      elements[i].instance_divisor = 1;
      elements[i].vertex_buffer_index = vertex_buffer_index;
      offset += util_format_get_blocksize(elements[i].src_format);
   }
}

void *
vl_vb_get_ves_ycbcr(struct pipe_context *pipe)
{
   struct pipe_vertex_element vertex_elems[NUM_VS_INPUTS];

   assert(pipe);

   memset(&vertex_elems, 0, sizeof(vertex_elems));
   vertex_elems[VS_I_RECT] = vl_vb_get_quad_vertex_element();

   /* Position element */
   vertex_elems[VS_I_VPOS].src_format = PIPE_FORMAT_R8G8B8A8_USCALED;

   /* block num element */
   vertex_elems[VS_I_BLOCK_NUM].src_format = PIPE_FORMAT_R32_FLOAT;

   vl_vb_element_helper(&vertex_elems[VS_I_VPOS], 2, 1);

   return pipe->create_vertex_elements_state(pipe, 3, vertex_elems);
}

void *
vl_vb_get_ves_mv(struct pipe_context *pipe)
{
   struct pipe_vertex_element vertex_elems[NUM_VS_INPUTS];

   assert(pipe);

   memset(&vertex_elems, 0, sizeof(vertex_elems));
   vertex_elems[VS_I_RECT] = vl_vb_get_quad_vertex_element();

   /* Position element */
   vertex_elems[VS_I_VPOS].src_format = PIPE_FORMAT_R16G16_SSCALED;

   vl_vb_element_helper(&vertex_elems[VS_I_VPOS], 1, 1);

   /* motion vector TOP element */
   vertex_elems[VS_I_MV_TOP].src_format = PIPE_FORMAT_R16G16B16A16_SSCALED;

   /* motion vector BOTTOM element */
   vertex_elems[VS_I_MV_BOTTOM].src_format = PIPE_FORMAT_R16G16B16A16_SSCALED;

   vl_vb_element_helper(&vertex_elems[VS_I_MV_TOP], 2, 2);

   return pipe->create_vertex_elements_state(pipe, NUM_VS_INPUTS, vertex_elems);
}

bool
vl_vb_init(struct vl_vertex_buffer *buffer, struct pipe_context *pipe,
           unsigned width, unsigned height)
{
   unsigned i, size;

   assert(buffer);

   buffer->width = width;
   buffer->height = height;

   size = width * height;

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      buffer->ycbcr[i].resource = pipe_buffer_create
      (
         pipe->screen,
         PIPE_BIND_VERTEX_BUFFER,
         PIPE_USAGE_STREAM,
         sizeof(struct vl_ycbcr_block) * size * 4
      );
      if (!buffer->ycbcr[i].resource)
         goto error_ycbcr;
   }

   for (i = 0; i < VL_MAX_REF_FRAMES; ++i) {
      buffer->mv[i].resource = pipe_buffer_create
      (
         pipe->screen,
         PIPE_BIND_VERTEX_BUFFER,
         PIPE_USAGE_STREAM,
         sizeof(struct vl_motionvector) * size
      );
      if (!buffer->mv[i].resource)
         goto error_mv;
   }

   vl_vb_map(buffer, pipe);
   return true;

error_mv:
   for (i = 0; i < VL_MAX_PLANES; ++i)
      pipe_resource_reference(&buffer->mv[i].resource, NULL);

error_ycbcr:
   for (i = 0; i < VL_MAX_PLANES; ++i)
      pipe_resource_reference(&buffer->ycbcr[i].resource, NULL);
   return false;
}

unsigned
vl_vb_attributes_per_plock(struct vl_vertex_buffer *buffer)
{
   return 1;
}

struct pipe_vertex_buffer
vl_vb_get_ycbcr(struct vl_vertex_buffer *buffer, int component)
{
   struct pipe_vertex_buffer buf;

   assert(buffer);

   buf.stride = sizeof(struct vl_ycbcr_block);
   buf.buffer_offset = 0;
   buf.buffer = buffer->ycbcr[component].resource;

   return buf;
}

struct pipe_vertex_buffer
vl_vb_get_mv(struct vl_vertex_buffer *buffer, int motionvector)
{
   struct pipe_vertex_buffer buf;

   assert(buffer);

   buf.stride = sizeof(struct vl_motionvector);
   buf.buffer_offset = 0;
   buf.buffer = buffer->mv[motionvector].resource;

   return buf;
}

void
vl_vb_map(struct vl_vertex_buffer *buffer, struct pipe_context *pipe)
{
   unsigned i;

   assert(buffer && pipe);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      buffer->ycbcr[i].vertex_stream = pipe_buffer_map
      (
         pipe,
         buffer->ycbcr[i].resource,
         PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD_RANGE,
         &buffer->ycbcr[i].transfer
      );
   }

   for (i = 0; i < VL_MAX_REF_FRAMES; ++i) {
      buffer->mv[i].vertex_stream = pipe_buffer_map
      (
         pipe,
         buffer->mv[i].resource,
         PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD_RANGE,
         &buffer->mv[i].transfer
      );
   }

}

struct vl_ycbcr_block *
vl_vb_get_ycbcr_stream(struct vl_vertex_buffer *buffer, int component)
{
   assert(buffer);
   assert(component < VL_MAX_PLANES);

   return buffer->ycbcr[component].vertex_stream;
}

unsigned
vl_vb_get_mv_stream_stride(struct vl_vertex_buffer *buffer)
{
   assert(buffer);

   return buffer->width;
}

struct vl_motionvector *
vl_vb_get_mv_stream(struct vl_vertex_buffer *buffer, int ref_frame)
{
   assert(buffer);
   assert(ref_frame < VL_MAX_REF_FRAMES);

   return buffer->mv[ref_frame].vertex_stream;
}

void
vl_vb_unmap(struct vl_vertex_buffer *buffer, struct pipe_context *pipe)
{
   unsigned i;

   assert(buffer && pipe);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      pipe_buffer_unmap(pipe, buffer->ycbcr[i].transfer);
   }

   for (i = 0; i < VL_MAX_REF_FRAMES; ++i) {
      pipe_buffer_unmap(pipe, buffer->mv[i].transfer);
   }
}

void
vl_vb_cleanup(struct vl_vertex_buffer *buffer)
{
   unsigned i;

   assert(buffer);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      pipe_resource_reference(&buffer->ycbcr[i].resource, NULL);
   }

   for (i = 0; i < VL_MAX_REF_FRAMES; ++i) {
      pipe_resource_reference(&buffer->mv[i].resource, NULL);
   }
}
