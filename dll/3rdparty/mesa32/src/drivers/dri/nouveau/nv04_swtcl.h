#ifndef __NV04_SWTCL_H__
#define __NV04_SWTCL_H__

#include "mtypes.h"

extern void nv04Fallback( GLcontext *ctx, GLuint bit, GLboolean mode );
extern void nv04FinishPrimitive(struct nouveau_context *nmesa);
extern void nv04TriInitFunctions(GLcontext *ctx);
#define FALLBACK( nmesa, bit, mode ) nouveauFallback( nmesa->glCtx, bit, mode )

#endif /* __NV04_SWTCL_H__ */

