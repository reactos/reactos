#ifndef TEXRENDER_H
#define TEXRENDER_H


extern void
_mesa_render_texture(GLcontext *ctx,
                     struct gl_framebuffer *fb,
                     struct gl_renderbuffer_attachment *att);

extern void
_mesa_finish_render_texture(GLcontext *ctx,
                            struct gl_renderbuffer_attachment *att);


#endif /* TEXRENDER_H */
