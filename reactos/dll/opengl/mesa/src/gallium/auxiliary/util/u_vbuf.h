/**************************************************************************
 *
 * Copyright 2011 Marek Olšák <maraeo@gmail.com>
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
 * IN NO EVENT SHALL AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef U_VBUF_MGR_H
#define U_VBUF_MGR_H

/* This module builds upon u_upload_mgr and translate_cache and takes care of
 * user buffer uploads and vertex format fallbacks. It's designed
 * for the drivers which don't always use the Draw module. (e.g. for HWTCL)
 */

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_transfer.h"

/* Hardware vertex fetcher limitations can be described by this structure. */
struct u_vbuf_caps {
   /* Vertex format CAPs. */
   /* TRUE if hardware supports it. */
   unsigned format_fixed32:1;    /* PIPE_FORMAT_*32*_FIXED */
   unsigned format_float16:1;    /* PIPE_FORMAT_*16*_FLOAT */
   unsigned format_float64:1;    /* PIPE_FORMAT_*64*_FLOAT */
   unsigned format_norm32:1;     /* PIPE_FORMAT_*32*NORM */
   unsigned format_scaled32:1;   /* PIPE_FORMAT_*32*SCALED */

   /* Whether vertex fetches don't have to be dword-aligned. */
   /* TRUE if hardware supports it. */
   unsigned fetch_dword_unaligned:1;
};

/* The manager.
 * This structure should also be used to access vertex buffers
 * from a driver. */
struct u_vbuf {
   /* This is what was set in set_vertex_buffers.
    * May contain user buffers. */
   struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
   unsigned nr_vertex_buffers;

   /* Contains only real vertex buffers.
    * Hardware drivers should use real_vertex_buffers[i]
    * instead of vertex_buffers[i].buffer. */
   struct pipe_vertex_buffer real_vertex_buffer[PIPE_MAX_ATTRIBS];
   int nr_real_vertex_buffers;

   /* The index buffer. */
   struct pipe_index_buffer index_buffer;

   /* This uploader can optionally be used by the driver.
    *
    * Allowed functions:
    * - u_upload_alloc
    * - u_upload_data
    * - u_upload_buffer
    * - u_upload_flush */
   struct u_upload_mgr *uploader;

   struct u_vbuf_caps caps;
};

struct u_vbuf_resource {
   struct u_resource b;
   uint8_t *user_ptr;
};

/* Opaque type containing information about vertex elements for the manager. */
struct u_vbuf_elements;

enum u_fetch_alignment {
   U_VERTEX_FETCH_BYTE_ALIGNED,
   U_VERTEX_FETCH_DWORD_ALIGNED
};

enum u_vbuf_return_flags {
   U_VBUF_BUFFERS_UPDATED = 1
};


struct u_vbuf *
u_vbuf_create(struct pipe_context *pipe,
              unsigned upload_buffer_size,
              unsigned upload_buffer_alignment,
              unsigned upload_buffer_bind,
              enum u_fetch_alignment fetch_alignment);

void u_vbuf_destroy(struct u_vbuf *mgr);

struct u_vbuf_elements *
u_vbuf_create_vertex_elements(struct u_vbuf *mgr,
                              unsigned count,
                              const struct pipe_vertex_element *attrs,
                              struct pipe_vertex_element *native_attrs);

void u_vbuf_bind_vertex_elements(struct u_vbuf *mgr,
                                 void *cso,
                                 struct u_vbuf_elements *ve);

void u_vbuf_destroy_vertex_elements(struct u_vbuf *mgr,
                                    struct u_vbuf_elements *ve);

void u_vbuf_set_vertex_buffers(struct u_vbuf *mgr,
                               unsigned count,
                               const struct pipe_vertex_buffer *bufs);

void u_vbuf_set_index_buffer(struct u_vbuf *mgr,
                             const struct pipe_index_buffer *ib);

enum u_vbuf_return_flags u_vbuf_draw_begin(struct u_vbuf *mgr,
                                           struct pipe_draw_info *info);

unsigned u_vbuf_draw_max_vertex_count(struct u_vbuf *mgr);

void u_vbuf_draw_end(struct u_vbuf *mgr);


static INLINE struct u_vbuf_resource *u_vbuf_resource(struct pipe_resource *r)
{
   return (struct u_vbuf_resource*)r;
}

#endif
