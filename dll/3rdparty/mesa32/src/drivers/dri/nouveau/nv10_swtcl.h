/**************************************************************************

Copyright 2006 Stephane Marchesin
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/



#ifndef __NV10_SWTCL_H__
#define __NV10_SWTCL_H__

#include "mtypes.h"

extern void nv10Fallback( GLcontext *ctx, GLuint bit, GLboolean mode );
extern void nv10FinishPrimitive(struct nouveau_context *nmesa);
extern void nv10TriInitFunctions(GLcontext *ctx);
#define FALLBACK( nmesa, bit, mode ) nouveauFallback( nmesa->glCtx, bit, mode )

#endif /* __NV10_SWTCL_H__ */

