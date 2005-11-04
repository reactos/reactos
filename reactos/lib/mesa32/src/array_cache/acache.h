
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
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef _ARRAYCACHE_H
#define _ARRAYCACHE_H

#include "mtypes.h"


extern GLboolean
_ac_CreateContext( GLcontext *ctx );

extern void
_ac_DestroyContext( GLcontext *ctx );

extern void
_ac_InvalidateState( GLcontext *ctx, GLuint new_state );

extern struct gl_client_array *
_ac_import_vertex( GLcontext *ctx,
		   GLenum type,
		   GLuint reqstride,
		   GLuint reqsize,
		   GLboolean reqwritable,
		   GLboolean *writable );

extern struct gl_client_array *
_ac_import_normal( GLcontext *ctx,
		   GLenum type,
		   GLuint reqstride,
		   GLboolean reqwritable,
		   GLboolean *writable );

extern struct gl_client_array *
_ac_import_color( GLcontext *ctx,
		  GLenum type,
		  GLuint reqstride,
		  GLuint reqsize,
		  GLboolean reqwritable,
		  GLboolean *writable );

extern struct gl_client_array *
_ac_import_index( GLcontext *ctx,
		  GLenum type,
		  GLuint reqstride,
		  GLboolean reqwritable,
		  GLboolean *writable );

extern struct gl_client_array *
_ac_import_secondarycolor( GLcontext *ctx,
			   GLenum type,
			   GLuint reqstride,
			   GLuint reqsize,
			   GLboolean reqwritable,
			   GLboolean *writable );

extern struct gl_client_array *
_ac_import_fogcoord( GLcontext *ctx,
		     GLenum type,
		     GLuint reqstride,
		     GLboolean reqwritable,
		     GLboolean *writable );

extern struct gl_client_array *
_ac_import_edgeflag( GLcontext *ctx,
		     GLenum type,
		     GLuint reqstride,
		     GLboolean reqwritable,
		     GLboolean *writable );

extern struct gl_client_array *
_ac_import_texcoord( GLcontext *ctx,
		     GLuint unit,
		     GLenum type,
		     GLuint reqstride,
		     GLuint reqsize,
		     GLboolean reqwritable,
		     GLboolean *writable );

extern struct gl_client_array *
_ac_import_attrib( GLcontext *ctx,
                   GLuint index,
                   GLenum type,
                   GLuint reqstride,
                   GLuint reqsize,
                   GLboolean reqwritable,
                   GLboolean *writable );


/* Clients must call this function to validate state and set bounds
 * before importing any data:
 */
extern void
_ac_import_range( GLcontext *ctx, GLuint start, GLuint count );


/* Additional convenience function:
 */
extern CONST void *
_ac_import_elements( GLcontext *ctx,
		     GLenum new_type,
		     GLuint count,
		     GLenum old_type,
		     CONST void *indices );


#endif
