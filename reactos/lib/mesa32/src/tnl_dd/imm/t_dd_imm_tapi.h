
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
 *    Gareth Hughes <gareth@valinux.com>
 *    Keith Whitwell <keith_whitwell@yahoo.com>
 */

/* Template for immediate mode texture coordinate functions.
 */

#ifndef DO_PROJ_TEX
#error "Need to define DO_PROJ_TEX"
#endif


/* =============================================================
 * Notify on calls to texture4f, to allow switch to projected texture
 * vertex format:
 */

static void TAG(TexCoord4f)( GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
   GET_CURRENT;
   DO_PROJ_TEX;
   TEXCOORD4( s, t, r, q );
}

static void TAG(TexCoord4fv)( const GLfloat *v )
{
   GET_CURRENT;
   DO_PROJ_TEX;
   TEXCOORD4( v[0], v[1], v[2], v[3] );
}

static void TAG(MultiTexCoord4fARB)( GLenum target, GLfloat s,
				     GLfloat t, GLfloat r, GLfloat q )
{
   GET_CURRENT;
   DO_PROJ_TEX;
   MULTI_TEXCOORD4( unit, s, t, r, q );
}

static void TAG(MultiTexCoord4fvARB)( GLenum target, const GLfloat *v )
{
   GET_CURRENT;
   DO_PROJ_TEX;
   MULTI_TEXCOORD4( unit, v[0], v[1], v[2], v[3] );
}



#undef DO_PROJ_TEX
#undef TAG
