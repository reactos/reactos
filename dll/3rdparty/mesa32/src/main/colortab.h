/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


#ifndef COLORTAB_H
#define COLORTAB_H


#include "mtypes.h"

#if _HAVE_FULL_GL

extern void GLAPIENTRY
_mesa_ColorTable( GLenum target, GLenum internalformat,
                  GLsizei width, GLenum format, GLenum type,
                  const GLvoid *table );

extern void GLAPIENTRY
_mesa_ColorSubTable( GLenum target, GLsizei start,
                     GLsizei count, GLenum format, GLenum type,
                     const GLvoid *table );

extern void GLAPIENTRY
_mesa_CopyColorSubTable(GLenum target, GLsizei start,
                        GLint x, GLint y, GLsizei width);

extern void GLAPIENTRY
_mesa_CopyColorTable(GLenum target, GLenum internalformat,
                     GLint x, GLint y, GLsizei width);

extern void GLAPIENTRY
_mesa_GetColorTable( GLenum target, GLenum format,
                     GLenum type, GLvoid *table );

extern void GLAPIENTRY
_mesa_ColorTableParameterfv(GLenum target, GLenum pname,
                            const GLfloat *params);

extern void GLAPIENTRY
_mesa_ColorTableParameteriv(GLenum target, GLenum pname,
                            const GLint *params);

extern void GLAPIENTRY
_mesa_GetColorTableParameterfv( GLenum target, GLenum pname, GLfloat *params );

extern void GLAPIENTRY
_mesa_GetColorTableParameteriv( GLenum target, GLenum pname, GLint *params );



extern void
_mesa_init_colortable( struct gl_color_table *table );

extern void
_mesa_free_colortable_data( struct gl_color_table *table );

extern void 
_mesa_init_colortables( GLcontext *ctx );

extern void 
_mesa_free_colortables_data( GLcontext *ctx );

#else

/** No-op */
#define _mesa_init_colortable( p ) ((void) 0)

/** No-op */
#define _mesa_free_colortable_data( p ) ((void) 0)

/** No-op */
#define _mesa_init_colortables( p ) ((void)0)

/** No-op */
#define _mesa_free_colortables_data( p ) ((void)0)

#endif

#endif
