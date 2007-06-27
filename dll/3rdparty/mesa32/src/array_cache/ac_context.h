
/*
 * Mesa 3-D graphics library
 * Version:  5.1
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

#ifndef _AC_CONTEXT_H
#define _AC_CONTEXT_H

#include "glheader.h"
#include "mtypes.h"

#include "array_cache/acache.h"

/* These are used to make the ctx->Current values look like
 * arrays (with zero StrideB).
 */
struct ac_arrays {
   struct gl_client_array Vertex;
   struct gl_client_array Normal;
   struct gl_client_array Color;
   struct gl_client_array SecondaryColor;
   struct gl_client_array FogCoord;
   struct gl_client_array Index;
   struct gl_client_array TexCoord[MAX_TEXTURE_COORD_UNITS];
   struct gl_client_array EdgeFlag;
   struct gl_client_array Attrib[VERT_ATTRIB_MAX];  /* GL_NV_vertex_program */
};

struct ac_array_pointers {
   struct gl_client_array *Vertex;
   struct gl_client_array *Normal;
   struct gl_client_array *Color;
   struct gl_client_array *SecondaryColor;
   struct gl_client_array *FogCoord;
   struct gl_client_array *Index;
   struct gl_client_array *TexCoord[MAX_TEXTURE_COORD_UNITS];
   struct gl_client_array *EdgeFlag;
   struct gl_client_array *Attrib[VERT_ATTRIB_MAX];  /* GL_NV_vertex_program */
};

struct ac_array_flags {
   GLboolean Vertex;
   GLboolean Normal;
   GLboolean Color;
   GLboolean SecondaryColor;
   GLboolean FogCoord;
   GLboolean Index;
   GLboolean TexCoord[MAX_TEXTURE_COORD_UNITS];
   GLboolean EdgeFlag;
   GLboolean Attrib[VERT_ATTRIB_MAX];  /* GL_NV_vertex_program */
};


typedef struct {
   GLuint NewState;		/* not needed? */
   GLuint NewArrayState;

   /* Facility for importing and caching array data:
    */
   struct ac_arrays Fallback;
   struct ac_arrays Cache;
   struct ac_arrays Raw;
   struct ac_array_flags IsCached;
   GLuint start;
   GLuint count;

   /* Facility for importing element lists:
    */
   GLuint *Elts;
   GLuint elt_size;

} ACcontext;

#define AC_CONTEXT(ctx) ((ACcontext *)ctx->acache_context)

#endif
