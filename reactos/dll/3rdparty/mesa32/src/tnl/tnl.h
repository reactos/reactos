
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

#ifndef _TNL_H
#define _TNL_H

#include "mtypes.h"



/* These are the public-access functions exported from tnl.  (A few
 * more are currently hooked into dispatch directly by the module
 * itself.)
 */
extern GLboolean
_tnl_CreateContext( GLcontext *ctx );

extern void
_tnl_DestroyContext( GLcontext *ctx );

extern void
_tnl_InvalidateState( GLcontext *ctx, GLuint new_state );

/* Functions to revive the tnl module after being unhooked from
 * dispatch and/or driver callbacks.
 */

/* Restore just the ctx->Exec table:
 */
extern void
_tnl_wakeup_exec( GLcontext *ctx );

/* Restore both ctx->Exec and ctx->Save:
 */
extern void
_tnl_wakeup_save_exec( GLcontext *ctx );

/* Driver configuration options:
 */
extern void
_tnl_need_projected_coords( GLcontext *ctx, GLboolean flag );

extern void
_tnl_need_dlist_loopback( GLcontext *ctx, GLboolean flag );

extern void
_tnl_need_dlist_norm_lengths( GLcontext *ctx, GLboolean flag );

extern void
_tnl_isolate_materials( GLcontext *ctx, GLboolean flag );


/* Control whether T&L does per-vertex fog
 */
extern void
_tnl_allow_vertex_fog( GLcontext *ctx, GLboolean value );

extern void
_tnl_allow_pixel_fog( GLcontext *ctx, GLboolean value );


#endif
