/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "glheader.h"

struct gl_config;
struct gl_context;

extern struct gl_framebuffer *
_mesa_create_framebuffer(const struct gl_config *visual);

extern void
_mesa_initialize_window_framebuffer(struct gl_framebuffer *fb,
				     const struct gl_config *visual);

extern void
_mesa_destroy_framebuffer(struct gl_framebuffer *buffer);

extern void
_mesa_free_framebuffer_data(struct gl_framebuffer *buffer);

extern void
_mesa_reference_framebuffer_(struct gl_framebuffer **ptr,
                             struct gl_framebuffer *fb);

static inline void
_mesa_reference_framebuffer(struct gl_framebuffer **ptr,
                            struct gl_framebuffer *fb)
{
   if (*ptr != fb)
      _mesa_reference_framebuffer_(ptr, fb);
}

extern void
_mesa_resize_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb,
                         GLuint width, GLuint height);


extern void
_mesa_resizebuffers( struct gl_context *ctx );


extern void 
_mesa_update_draw_buffer_bounds(struct gl_context *ctx);

extern void
_mesa_update_framebuffer(struct gl_context *ctx);

extern GLboolean
_mesa_source_buffer_exists(struct gl_context *ctx, GLenum format);

extern GLboolean
_mesa_dest_buffer_exists(struct gl_context *ctx, GLenum format);

extern GLenum
_mesa_get_color_read_type(struct gl_context *ctx);

extern GLenum
_mesa_get_color_read_format(struct gl_context *ctx);

#endif /* FRAMEBUFFER_H */
