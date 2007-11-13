
#ifndef INTEL_DEPTH_STENCIL_H
#define INTEL_DEPTH_STENCIL_H


extern void
intel_unpair_depth_stencil(GLcontext * ctx, struct intel_renderbuffer *irb);

extern void
intel_validate_paired_depth_stencil(GLcontext * ctx,
                                    struct gl_framebuffer *fb);


#endif /* INTEL_DEPTH_STENCIL_H */
