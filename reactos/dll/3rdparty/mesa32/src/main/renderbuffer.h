/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#ifndef RENDERBUFFER_H
#define RENDERBUFFER_H


extern void
_mesa_init_renderbuffer(struct gl_renderbuffer *rb, GLuint name);

extern struct gl_renderbuffer *
_mesa_new_renderbuffer(GLcontext *ctx, GLuint name);

extern void
_mesa_delete_renderbuffer(struct gl_renderbuffer *rb);


extern struct gl_renderbuffer *
_mesa_new_soft_renderbuffer(GLcontext *ctx, GLuint name);


extern GLboolean
_mesa_soft_renderbuffer_storage(GLcontext *ctx, struct gl_renderbuffer *rb,
                                GLenum internalFormat,
                                GLuint width, GLuint height);

extern GLboolean
_mesa_add_color_renderbuffers(GLcontext *ctx, struct gl_framebuffer *fb,
                              GLuint rgbBits, GLuint alphaBits,
                              GLboolean frontLeft, GLboolean backLeft,
                              GLboolean frontRight, GLboolean backRight);

extern GLboolean
_mesa_add_color_index_renderbuffers(GLcontext *ctx, struct gl_framebuffer *fb,
                                    GLuint indexBits,
                                    GLboolean frontLeft, GLboolean backLeft,
                                    GLboolean frontRight, GLboolean backRight);

extern GLboolean
_mesa_add_alpha_renderbuffers(GLcontext *ctx, struct gl_framebuffer *fb,
                              GLuint alphaBits,
                              GLboolean frontLeft, GLboolean backLeft,
                              GLboolean frontRight, GLboolean backRight);

extern void
_mesa_copy_soft_alpha_renderbuffers(GLcontext *ctx, struct gl_framebuffer *fb);

extern GLboolean
_mesa_add_depth_renderbuffer(GLcontext *ctx, struct gl_framebuffer *fb,
                             GLuint depthBits);

extern GLboolean
_mesa_add_stencil_renderbuffer(GLcontext *ctx, struct gl_framebuffer *fb,
                               GLuint stencilBits);


extern GLboolean
_mesa_add_accum_renderbuffer(GLcontext *ctx, struct gl_framebuffer *fb,
                             GLuint redBits, GLuint greenBits,
                             GLuint blueBits, GLuint alphaBits);

extern GLboolean
_mesa_add_aux_renderbuffers(GLcontext *ctx, struct gl_framebuffer *fb,
                            GLuint bits, GLuint numBuffers);

extern void
_mesa_add_soft_renderbuffers(struct gl_framebuffer *fb,
                             GLboolean color,
                             GLboolean depth,
                             GLboolean stencil,
                             GLboolean accum,
                             GLboolean alpha,
                             GLboolean aux);

extern void
_mesa_add_renderbuffer(struct gl_framebuffer *fb,
                       GLuint bufferName, struct gl_renderbuffer *rb);

extern void
_mesa_remove_renderbuffer(struct gl_framebuffer *fb, GLuint bufferName);

extern void
_mesa_reference_renderbuffer(struct gl_renderbuffer **ptr,
                             struct gl_renderbuffer *rb);

extern struct gl_renderbuffer *
_mesa_new_depthstencil_renderbuffer(GLcontext *ctx, GLuint name);


#endif /* RENDERBUFFER_H */
