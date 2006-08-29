#ifndef TEXRENDER_H
#define TEXRENDER_H


extern void
_mesa_renderbuffer_texture(GLcontext *ctx,
                           struct gl_renderbuffer_attachment *att,
                           struct gl_texture_object *texObj,
                           GLenum texTarget, GLuint level, GLuint zoffset);


#endif /* TEXRENDER_H */
