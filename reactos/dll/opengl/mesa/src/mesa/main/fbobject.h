/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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


#ifndef FBOBJECT_H
#define FBOBJECT_H

#include "compiler.h"
#include "glheader.h"

struct gl_context;
struct gl_texture_object;

extern void
_mesa_init_fbobjects(struct gl_context *ctx);

extern struct gl_framebuffer *
_mesa_get_incomplete_framebuffer(void);

extern struct gl_renderbuffer *
_mesa_lookup_renderbuffer(struct gl_context *ctx, GLuint id);

extern struct gl_framebuffer *
_mesa_lookup_framebuffer(struct gl_context *ctx, GLuint id);

extern struct gl_renderbuffer_attachment *
_mesa_get_attachment(struct gl_context *ctx, struct gl_framebuffer *fb,
                     GLenum attachment);


/** Return the texture image for a renderbuffer attachment */
static inline struct gl_texture_image *
_mesa_get_attachment_teximage(struct gl_renderbuffer_attachment *att)
{
   assert(att->Type == GL_TEXTURE);
   return att->Texture->Image[att->CubeMapFace][att->TextureLevel];
}


/** Return the (const) texture image for a renderbuffer attachment */
static inline const struct gl_texture_image *
_mesa_get_attachment_teximage_const(const struct gl_renderbuffer_attachment *att)
{
   assert(att->Type == GL_TEXTURE);
   return att->Texture->Image[att->CubeMapFace][att->TextureLevel];
}


extern void
_mesa_remove_attachment(struct gl_context *ctx,
                        struct gl_renderbuffer_attachment *att);

extern void
_mesa_set_texture_attachment(struct gl_context *ctx,
                             struct gl_framebuffer *fb,
                             struct gl_renderbuffer_attachment *att,
                             struct gl_texture_object *texObj,
                             GLenum texTarget, GLuint level, GLuint zoffset);

extern void
_mesa_set_renderbuffer_attachment(struct gl_context *ctx,
                                  struct gl_renderbuffer_attachment *att,
                                  struct gl_renderbuffer *rb);

extern void
_mesa_framebuffer_renderbuffer(struct gl_context *ctx,
                               struct gl_framebuffer *fb,
                               GLenum attachment, struct gl_renderbuffer *rb);

extern void
_mesa_validate_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb);

extern void
_mesa_test_framebuffer_completeness(struct gl_context *ctx,
                                    struct gl_framebuffer *fb);

extern GLboolean
_mesa_is_legal_color_format(const struct gl_context *ctx, GLenum baseFormat);

extern GLenum
_mesa_base_fbo_format(struct gl_context *ctx, GLenum internalFormat);

extern GLboolean GLAPIENTRY
_mesa_IsRenderbufferEXT(GLuint renderbuffer);

extern void GLAPIENTRY
_mesa_BindRenderbufferEXT(GLenum target, GLuint renderbuffer);

extern void GLAPIENTRY
_mesa_DeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers);

extern void GLAPIENTRY
_mesa_GenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers);

extern void GLAPIENTRY
_mesa_RenderbufferStorageEXT(GLenum target, GLenum internalformat,
                             GLsizei width, GLsizei height);

extern void GLAPIENTRY
_mesa_RenderbufferStorageMultisample(GLenum target, GLsizei samples,
                                     GLenum internalformat,
                                     GLsizei width, GLsizei height);

extern void GLAPIENTRY
_es_RenderbufferStorageEXT(GLenum target, GLenum internalFormat,
			   GLsizei width, GLsizei height);

extern void GLAPIENTRY
_mesa_EGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image);

extern void GLAPIENTRY
_mesa_GetRenderbufferParameterivEXT(GLenum target, GLenum pname,
                                    GLint *params);

extern GLboolean GLAPIENTRY
_mesa_IsFramebufferEXT(GLuint framebuffer);

extern void GLAPIENTRY
_mesa_BindFramebufferEXT(GLenum target, GLuint framebuffer);

extern void GLAPIENTRY
_mesa_DeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers);

extern void GLAPIENTRY
_mesa_GenFramebuffersEXT(GLsizei n, GLuint *framebuffers);

extern GLenum GLAPIENTRY
_mesa_CheckFramebufferStatusEXT(GLenum target);

extern void GLAPIENTRY
_mesa_FramebufferTexture1DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level);

extern void GLAPIENTRY
_mesa_FramebufferTexture2DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level);

extern void GLAPIENTRY
_mesa_FramebufferTexture3DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture,
                              GLint level, GLint zoffset);

extern void GLAPIENTRY
_mesa_FramebufferTextureLayerEXT(GLenum target, GLenum attachment,
                                 GLuint texture, GLint level, GLint layer);

extern void GLAPIENTRY
_mesa_FramebufferRenderbufferEXT(GLenum target, GLenum attachment,
                                 GLenum renderbuffertarget,
                                 GLuint renderbuffer);

extern void GLAPIENTRY
_mesa_GetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment,
                                             GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GenerateMipmapEXT(GLenum target);


extern void GLAPIENTRY
_mesa_BlitFramebufferEXT(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                         GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                         GLbitfield mask, GLenum filter);

extern void GLAPIENTRY
_mesa_FramebufferTextureARB(GLenum target, GLenum attachment,
                            GLuint texture, GLint level);

extern void GLAPIENTRY
_mesa_FramebufferTextureFaceARB(GLenum target, GLenum attachment,
                                GLuint texture, GLint level, GLenum face);


#endif /* FBOBJECT_H */
