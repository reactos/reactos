/**************************************************************************

Copyright 2001 VA Linux Systems Inc., Fremont, California.

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_state.h,v 1.3 2002/12/10 01:26:53 dawes Exp $ */

/*
 * Author:
 *   Jeff Hartmann <jhartmann@2d3d.com>
 *
 * Heavily based on the I810 driver, which was written by:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */
#ifndef _I830_STATE_H
#define _I830_STATE_H

#include "i830_context.h"
#include "colormac.h"
#define FloatToInt(F) ((int)(F))

/* 
 *  * This function/macro is sensitive to precision.  Test carefully
 *  * if you change it.
 *  */
#define FLOAT_COLOR_TO_UBYTE_COLOR(b, f)                        \
	    do {		                                            \
           union {GLfloat r; GLuint i; }  tmp;                  \
           tmp.r = f;                                           \
           b = ((tmp.i >= IEEE_ONE)                             \
			? ((GLint)tmp.i < 0) ? (GLubyte)0 : (GLubyte)255 	\
			:  (tmp.r = tmp.r*(255.0F/256.0F) + 32768.0F,  	    \
                  (GLubyte)tmp.i));                             \
        } while (0)
	 
	 

extern void i830DDInitState( GLcontext *ctx );
extern void i830DDInitStateFuncs( GLcontext *ctx );

extern void i830PrintDirty( const char *msg, GLuint state );
extern void i830SetDrawBuffer(GLcontext *ctx, GLenum mode );

extern void i830Fallback( i830ContextPtr imesa, GLuint bit, GLboolean mode );
#define FALLBACK( imesa, bit, mode ) i830Fallback( imesa, bit, mode )
#endif
