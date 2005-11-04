/*
 * Copyright 2001 by Alan Hourihane.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@tungstengraphics.com>
 *           Keith Whitwell, <keith@tungstengraphics.com>
 *
 * 3DLabs Gamma driver.
 */

#ifndef GAMMAVB_INC
#define GAMMAVB_INC

#include "mtypes.h"
#include "swrast/swrast.h"

#define _GAMMA_NEW_VERTEX (_NEW_TEXTURE |		\
			   _DD_NEW_TRI_UNFILLED |	\
			   _DD_NEW_TRI_LIGHT_TWOSIDE)


extern void gammaChooseVertexState( GLcontext *ctx );
extern void gammaCheckTexSizes( GLcontext *ctx );
extern void gammaBuildVertices( GLcontext *ctx, 
				GLuint start, 
				GLuint count,
				GLuint newinputs );


extern void gamma_import_float_colors( GLcontext *ctx );
extern void gamma_import_float_spec_colors( GLcontext *ctx );

extern void gamma_translate_vertex( GLcontext *ctx, 
				    const gammaVertex *src, 
				    SWvertex *dst );

extern void gammaInitVB( GLcontext *ctx );
extern void gammaFreeVB( GLcontext *ctx );

extern void gamma_print_vertex( GLcontext *ctx, const gammaVertex *v );
extern void gammaPrintSetupFlags(char *msg, GLuint flags );

#endif
