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



#ifndef __NOUVEAU_SWTCL_H__
#define __NOUVEAU_SWTCL_H__

#include "nouveau_context.h"

extern void nouveau_fallback_tri(struct nouveau_context *nmesa,
		nouveauVertex *v0,
		nouveauVertex *v1,
		nouveauVertex *v2);

extern void nouveau_fallback_line(struct nouveau_context *nmesa,
		nouveauVertex *v0,
		nouveauVertex *v1);

extern void nouveau_fallback_point(struct nouveau_context *nmesa, 
		nouveauVertex *v0);

extern void nouveauFallback(struct nouveau_context *nmesa, GLuint bit, GLboolean mode);

extern void nouveauRunPipeline( GLcontext *ctx );

extern void nouveauTriInitFunctions( GLcontext *ctx );


#endif /* __NOUVEAU_SWTCL_H__ */


