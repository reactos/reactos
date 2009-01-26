/**
 * \file varray.h
 * Vertex arrays.
 *
 * \if subset
 * (No-op)
 *
 * \endif
 */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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


#ifndef VARRAY_H
#define VARRAY_H


#include "mtypes.h"

#if _HAVE_FULL_GL

extern void GLAPIENTRY
_mesa_VertexPointer(GLint size, GLenum type, GLsizei stride,
                    const GLvoid *ptr);

extern void GLAPIENTRY
_mesa_UnlockArraysEXT( void );

extern void GLAPIENTRY
_mesa_LockArraysEXT(GLint first, GLsizei count);


extern void GLAPIENTRY
_mesa_NormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_IndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_TexCoordPointer(GLint size, GLenum type, GLsizei stride,
                      const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_EdgeFlagPointer(GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_VertexPointerEXT(GLint size, GLenum type, GLsizei stride,
                       GLsizei count, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_NormalPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                       const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_ColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_IndexPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_TexCoordPointerEXT(GLint size, GLenum type, GLsizei stride,
                         GLsizei count, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_EdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *ptr);


extern void GLAPIENTRY
_mesa_FogCoordPointerEXT(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_SecondaryColorPointerEXT(GLint size, GLenum type,
			       GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_PointSizePointer(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_VertexAttribPointerNV(GLuint index, GLint size, GLenum type,
                            GLsizei stride, const GLvoid *pointer);


extern void GLAPIENTRY
_mesa_VertexAttribPointerARB(GLuint index, GLint size, GLenum type,
                             GLboolean normalized, GLsizei stride,
                             const GLvoid *pointer);


extern void GLAPIENTRY
_mesa_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer);


extern void GLAPIENTRY
_mesa_MultiDrawArraysEXT( GLenum mode, GLint *first,
                          GLsizei *count, GLsizei primcount );

extern void GLAPIENTRY
_mesa_MultiDrawElementsEXT( GLenum mode, const GLsizei *count, GLenum type,
                            const GLvoid **indices, GLsizei primcount );


extern void GLAPIENTRY
_mesa_MultiModeDrawArraysIBM( const GLenum * mode, const GLint * first,
			      const GLsizei * count,
			      GLsizei primcount, GLint modestride );


extern void GLAPIENTRY
_mesa_MultiModeDrawElementsIBM( const GLenum * mode, const GLsizei * count,
				GLenum type, const GLvoid * const * indices,
				GLsizei primcount, GLint modestride );

extern void GLAPIENTRY
_mesa_LockArraysEXT(GLint first, GLsizei count);

extern void GLAPIENTRY
_mesa_UnlockArraysEXT( void );

extern void
_mesa_init_varray( GLcontext * ctx );

#else

/** No-op */
#define _mesa_init_varray( c )  ((void)0)

#endif

#endif
