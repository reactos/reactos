
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

static void copy_pv_rgba4_spec5( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLubyte *i810verts = (GLubyte *)imesa->verts;
   GLuint shift = imesa->vertex_stride_shift;
   i810Vertex *dst = (i810Vertex *)(i810verts + (edst << shift));
   i810Vertex *src = (i810Vertex *)(i810verts + (esrc << shift));
   dst->ui[4] = src->ui[4];
   dst->ui[5] = src->ui[5];
}

static void copy_pv_rgba4( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLubyte *i810verts = (GLubyte *)imesa->verts;
   GLuint shift = imesa->vertex_stride_shift;
   i810Vertex *dst = (i810Vertex *)(i810verts + (edst << shift));
   i810Vertex *src = (i810Vertex *)(i810verts + (esrc << shift));
   dst->ui[4] = src->ui[4];
}

static void copy_pv_rgba3( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLubyte *i810verts = (GLubyte *)imesa->verts;
   GLuint shift = imesa->vertex_stride_shift;
   i810Vertex *dst = (i810Vertex *)(i810verts + (edst << shift));
   i810Vertex *src = (i810Vertex *)(i810verts + (esrc << shift));
   dst->ui[3] = src->ui[3];
}
