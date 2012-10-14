
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
 */


#ifndef API_ARRAYELT_H
#define API_ARRAYELT_H


#include "main/mfeatures.h"
#include "main/mtypes.h"

#if FEATURE_arrayelt

#define _MESA_INIT_ARRAYELT_VTXFMT(vfmt, impl)     \
   do {                                            \
      (vfmt)->ArrayElement = impl ## ArrayElement; \
   } while (0)

extern GLboolean _ae_create_context( struct gl_context *ctx );
extern void _ae_destroy_context( struct gl_context *ctx );
extern void _ae_invalidate_state( struct gl_context *ctx, GLuint new_state );
extern void GLAPIENTRY _ae_ArrayElement( GLint elt );

/* May optionally be called before a batch of element calls:
 */
extern void _ae_map_vbos( struct gl_context *ctx );
extern void _ae_unmap_vbos( struct gl_context *ctx );

extern void
_mesa_install_arrayelt_vtxfmt(struct _glapi_table *disp,
                              const GLvertexformat *vfmt);

#else /* FEATURE_arrayelt */

#define _MESA_INIT_ARRAYELT_VTXFMT(vfmt, impl) do { } while (0)

static inline GLboolean
_ae_create_context( struct gl_context *ctx )
{
   return GL_TRUE;
}

static inline void
_ae_destroy_context( struct gl_context *ctx )
{
}

static inline void
_ae_invalidate_state( struct gl_context *ctx, GLuint new_state )
{
}

static inline void
_mesa_install_arrayelt_vtxfmt(struct _glapi_table *disp,
                              const GLvertexformat *vfmt)
{
}

#endif /* FEATURE_arrayelt */


#endif /* API_ARRAYELT_H */
