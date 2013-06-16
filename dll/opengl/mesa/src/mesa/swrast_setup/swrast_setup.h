
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

/* Public interface to the swrast_setup module.  This module provides
 * an implementation of the driver interface to t_vb_render.c, and uses
 * the software rasterizer (swrast) to perform actual rasterization.
 *
 * The internals of the implementation are private, but can be hooked
 * into tnl at any time (except between RenderStart/RenderEnd) by
 * calling _swsetup_Wakeup(). 
 */

#ifndef SWRAST_SETUP_H
#define SWRAST_SETUP_H

#include "swrast/swrast.h"

extern GLboolean
_swsetup_CreateContext( struct gl_context *ctx );

extern void
_swsetup_DestroyContext( struct gl_context *ctx );

extern void
_swsetup_InvalidateState( struct gl_context *ctx, GLuint new_state );

extern void
_swsetup_Wakeup( struct gl_context *ctx );

/* Helper function to translate a hardware vertex (as understood by
 * the tnl/t_vertex.c code) to a swrast vertex.
 */
extern void 
_swsetup_Translate( struct gl_context *ctx, const void *vertex, SWvertex *dest );

#endif
