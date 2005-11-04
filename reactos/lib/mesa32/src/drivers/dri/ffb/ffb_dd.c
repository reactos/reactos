/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_dd.c,v 1.4 2002/09/11 19:49:07 tsi Exp $
 *
 * GLX Hardware Device Driver for Sun Creator/Creator3D
 * Copyright (C) 2000, 2001 David S. Miller
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
 * DAVID MILLER, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *    David S. Miller <davem@redhat.com>
 */

#include "mtypes.h"
#include "mm.h"
#include "ffb_dd.h"
#include "ffb_span.h"
#include "ffb_depth.h"
#include "ffb_context.h"
#include "ffb_vb.h"
#include "ffb_tris.h"
#include "ffb_clear.h"
#include "ffb_lock.h"
#include "extensions.h"

#define FFB_DATE	"20021125"

PUBLIC const char __driConfigOptions[] = { 0 };
const GLuint __driNConfigOptions = 0;

/* Mesa's Driver Functions */

static const GLubyte *ffbDDGetString(GLcontext *ctx, GLenum name)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	static char buffer[128];

	switch (name) {
	case GL_VENDOR:
		return (GLubyte *) "David S. Miller";

	case GL_RENDERER:
		sprintf(buffer, "Mesa DRI FFB " FFB_DATE);

		if (fmesa->ffb_sarea->flags & FFB_DRI_FFB2)
			strncat(buffer, " FFB2", 5);
		if (fmesa->ffb_sarea->flags & FFB_DRI_FFB2PLUS)
			strncat(buffer, " FFB2PLUS", 9);
		if (fmesa->ffb_sarea->flags & FFB_DRI_PAC1)
			strncat(buffer, " PAC1", 5);
		if (fmesa->ffb_sarea->flags & FFB_DRI_PAC2)
			strncat(buffer, " PAC2", 5);

#ifdef USE_SPARC_ASM
		strncat(buffer, " Sparc", 6);
#endif

		return (GLubyte *) buffer;

	default:
		return NULL;
	};
}


static void ffbBufferSize(GLframebuffer *buffer, GLuint *width, GLuint *height)
{
	GET_CURRENT_CONTEXT(ctx);
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	LOCK_HARDWARE(fmesa);
	*width = fmesa->driDrawable->w;
	*height = fmesa->driDrawable->h;
	UNLOCK_HARDWARE(fmesa);
}

void ffbDDExtensionsInit(GLcontext *ctx)
{
	/* Nothing for now until we start to add
	 * real acceleration. -DaveM
	 */

	/* XXX Need to turn off GL_EXT_blend_func_separate for one.
	 * XXX Also BlendEquation should be turned off too, what
	 * XXX EXT is that assosciated with?
	 */
}

static void ffbDDFinish(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	LOCK_HARDWARE(fmesa);
	FFBWait(fmesa, fmesa->regs);
	UNLOCK_HARDWARE(fmesa);
}

void ffbDDInitDriverFuncs(GLcontext *ctx)
{
	ctx->Driver.GetBufferSize	 = ffbBufferSize;
	ctx->Driver.GetString		 = ffbDDGetString;
	ctx->Driver.Clear		 = ffbDDClear;

	ctx->Driver.Finish		 = ffbDDFinish;
}
