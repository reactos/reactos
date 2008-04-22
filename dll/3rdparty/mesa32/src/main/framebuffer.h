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


extern struct gl_framebuffer *
_mesa_create_framebuffer(const GLvisual *visual);

extern struct gl_framebuffer *
_mesa_new_framebuffer(GLcontext *ctx, GLuint name);

extern void
_mesa_initialize_framebuffer(struct gl_framebuffer *fb, const GLvisual *visual);

extern void
_mesa_destroy_framebuffer(struct gl_framebuffer *buffer);

extern void
_mesa_free_framebuffer_data(struct gl_framebuffer *buffer);

extern void
_mesa_reference_framebuffer(struct gl_framebuffer **ptr,
                            struct gl_framebuffer *fb);

extern void
_mesa_unreference_framebuffer(struct gl_framebuffer **fb);

extern void
_mesa_resize_framebuffer(GLcontext *ctx, struct gl_framebuffer *fb,
                         GLuint width, GLuint height);

extern void 
_mesa_update_draw_buffer_bounds(GLcontext *ctx);

extern void
_mesa_update_framebuffer_visual(struct gl_framebuffer *fb);

extern void
_mesa_update_depth_buffer(GLcontext *ctx, struct gl_framebuffer *fb,
                            GLuint attIndex);

extern void
_mesa_update_stencil_buffer(GLcontext *ctx, struct gl_framebuffer *fb,
                            GLuint attIndex);

extern void
_mesa_update_framebuffer(GLcontext *ctx);

extern GLboolean
_mesa_source_buffer_exists(GLcontext *ctx, GLenum format);

extern GLboolean
_mesa_dest_buffer_exists(GLcontext *ctx, GLenum format);


#endif /* FRAMEBUFFER_H */
