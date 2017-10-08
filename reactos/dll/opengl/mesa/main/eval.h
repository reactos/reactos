/**
 * \file eval.h
 * Eval operations.
 * 
 * \if subset
 * (No-op)
 *
 * \endif
 */

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


#ifndef EVAL_H
#define EVAL_H


#include "main/mfeatures.h"
#include "main/mtypes.h"


#if FEATURE_evaluators

#define _MESA_INIT_EVAL_VTXFMT(vfmt, impl)         \
   do {                                            \
      (vfmt)->EvalCoord1f  = impl ## EvalCoord1f;  \
      (vfmt)->EvalCoord1fv = impl ## EvalCoord1fv; \
      (vfmt)->EvalCoord2f  = impl ## EvalCoord2f;  \
      (vfmt)->EvalCoord2fv = impl ## EvalCoord2fv; \
      (vfmt)->EvalPoint1   = impl ## EvalPoint1;   \
      (vfmt)->EvalPoint2   = impl ## EvalPoint2;   \
      (vfmt)->EvalMesh1    = impl ## EvalMesh1;    \
      (vfmt)->EvalMesh2    = impl ## EvalMesh2;    \
   } while (0)

extern GLuint _mesa_evaluator_components( GLenum target );


extern GLfloat *_mesa_copy_map_points1f( GLenum target,
                                      GLint ustride, GLint uorder,
                                      const GLfloat *points );

extern GLfloat *_mesa_copy_map_points1d( GLenum target,
                                      GLint ustride, GLint uorder,
                                      const GLdouble *points );

extern GLfloat *_mesa_copy_map_points2f( GLenum target,
                                      GLint ustride, GLint uorder,
                                      GLint vstride, GLint vorder,
                                      const GLfloat *points );

extern GLfloat *_mesa_copy_map_points2d(GLenum target,
                                     GLint ustride, GLint uorder,
                                     GLint vstride, GLint vorder,
                                     const GLdouble *points );

extern void
_mesa_install_eval_vtxfmt(struct _glapi_table *disp,
                          const GLvertexformat *vfmt);

extern void
_mesa_init_eval_dispatch(struct _glapi_table *disp);

#else /* FEATURE_evaluators */

#define _MESA_INIT_EVAL_VTXFMT(vfmt, impl) do { } while (0)

static inline void
_mesa_install_eval_vtxfmt(struct _glapi_table *disp,
                          const GLvertexformat *vfmt)
{
}

static inline void
_mesa_init_eval_dispatch(struct _glapi_table *disp)
{
}

#endif /* FEATURE_evaluators */

extern void _mesa_init_eval( struct gl_context *ctx );
extern void _mesa_free_eval_data( struct gl_context *ctx );


#endif /* EVAL_H */
