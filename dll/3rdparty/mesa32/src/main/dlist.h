/**
 * \file dlist.h
 * Display lists management.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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



#ifndef DLIST_H
#define DLIST_H


#include "mtypes.h"


#if _HAVE_FULL_GL

extern void
_mesa_delete_list(GLcontext *ctx, struct mesa_display_list *dlist);

extern void GLAPIENTRY _mesa_CallList( GLuint list );

extern void GLAPIENTRY _mesa_CallLists( GLsizei n, GLenum type, const GLvoid *lists );

extern void GLAPIENTRY _mesa_DeleteLists( GLuint list, GLsizei range );

extern void GLAPIENTRY _mesa_EndList( void );

extern GLuint GLAPIENTRY _mesa_GenLists( GLsizei range );

extern GLboolean GLAPIENTRY _mesa_IsList( GLuint list );

extern void GLAPIENTRY _mesa_ListBase( GLuint base );

extern void GLAPIENTRY _mesa_NewList( GLuint list, GLenum mode );

extern void GLAPIENTRY _mesa_save_CallLists( GLsizei n, GLenum type, const GLvoid *lists );

extern void GLAPIENTRY _mesa_save_CallList( GLuint list );



extern void _mesa_init_dlist_table( struct _glapi_table *table );

extern void _mesa_compile_error( GLcontext *ctx, GLenum error, const char *s );


extern void *_mesa_alloc_instruction(GLcontext *ctx, GLuint opcode, GLuint sz);

extern GLint _mesa_alloc_opcode( GLcontext *ctx, GLuint sz,
                                 void (*execute)( GLcontext *, void * ),
                                 void (*destroy)( GLcontext *, void * ),
                                 void (*print)( GLcontext *, void * ) );

extern void _mesa_init_display_list( GLcontext * ctx );

extern void _mesa_save_vtxfmt_init( GLvertexformat *vfmt );


#else

/** No-op */
#define _mesa_init_dlist_table(t,ts) ((void)0)

/** No-op */
#define _mesa_init_display_list(c) ((void)0)

/** No-op */
#define _mesa_save_vtxfmt_init(v) ((void)0)

#endif

#endif
