
/*
 * Mesa 3-D graphics library
 * Version:  4.0.3
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

typedef struct {
   GLfloat x, y, z, w;
} TAG(_coord_t);

#ifdef COLOR_IS_RGBA
typedef struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && BYTE_ORDER == BIG_ENDIAN
   GLubyte alpha, blue, green, red;
#else
   GLubyte red, green, blue, alpha;
#endif
} TAG(_color_t);
#else
typedef struct {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && BYTE_ORDER == BIG_ENDIAN
   GLubyte alpha, red, green, blue;
#else
   GLubyte blue, green, red, alpha;
#endif
} TAG(_color_t);
#endif

typedef union {
   struct {
      GLfloat x, y, z, w;
      TAG(_color_t) color;
      TAG(_color_t) specular;
      GLfloat u0, v0;
      GLfloat u1, v1;
      GLfloat u2, v2;
      GLfloat u3, v3;
   } v;
   struct {
      GLfloat x, y, z, w;
      TAG(_color_t) color;
      TAG(_color_t) specular;
      GLfloat u0, v0, q0;
      GLfloat u1, v1, q1;
      GLfloat u2, v2, q2;
      GLfloat u3, v3, q3;
   } pv;
   struct {
      GLfloat x, y, z;
      TAG(_color_t) color;
   } tv;
   GLfloat f[24];
   GLuint  ui[24];
   GLubyte ub4[24][4];
} TAG(Vertex), *TAG(VertexPtr);

