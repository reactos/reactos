/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "api_arrayelt.h"
#include "context.h"
#include "glapi.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"

typedef void (GLAPIENTRY *array_func)( const void * );

typedef struct {
   const struct gl_client_array *array;
   array_func func;
} AEarray;

typedef void (GLAPIENTRY *attrib_func)( GLuint indx, const void *data );

typedef struct {
   const struct gl_client_array *array;
   attrib_func func;
   GLuint index;
} AEattrib;

typedef struct {
   AEarray arrays[32];
   AEattrib attribs[VERT_ATTRIB_MAX + 1];
   GLuint NewState;
} AEcontext;

#define AE_CONTEXT(ctx) ((AEcontext *)(ctx)->aelt_context)

/*
 * Convert GL_BYTE, GL_UNSIGNED_BYTE, .. GL_DOUBLE into an integer
 * in the range [0, 7].  Luckily these type tokens are sequentially
 * numbered in gl.h
 */
#define TYPE_IDX(t) ((t) & 0xf)


static array_func colorfuncs[2][8] = {
   { (array_func)glColor3bv,
     (array_func)glColor3ubv,
     (array_func)glColor3sv,
     (array_func)glColor3usv,
     (array_func)glColor3iv,
     (array_func)glColor3uiv,
     (array_func)glColor3fv,
     (array_func)glColor3dv },

   { (array_func)glColor4bv,
     (array_func)glColor4ubv,
     (array_func)glColor4sv,
     (array_func)glColor4usv,
     (array_func)glColor4iv,
     (array_func)glColor4uiv,
     (array_func)glColor4fv,
     (array_func)glColor4dv }
};

static array_func vertexfuncs[3][8] = {
   { 0,
     0,
     (array_func)glVertex2sv,
     0,
     (array_func)glVertex2iv,
     0,
     (array_func)glVertex2fv,
     (array_func)glVertex2dv },

   { 0,
     0,
     (array_func)glVertex3sv,
     0,
     (array_func)glVertex3iv,
     0,
     (array_func)glVertex3fv,
     (array_func)glVertex3dv },

   { 0,
     0,
     (array_func)glVertex4sv,
     0,
     (array_func)glVertex4iv,
     0,
     (array_func)glVertex4fv,
     (array_func)glVertex4dv }
};

static array_func indexfuncs[8] = {
   0,
   (array_func)glIndexubv,
   (array_func)glIndexsv,
   0,
   (array_func)glIndexiv,
   0,
   (array_func)glIndexfv,
   (array_func)glIndexdv
};

static array_func normalfuncs[8] = {
   (array_func)glNormal3bv,
   0,
   (array_func)glNormal3sv,
   0,
   (array_func)glNormal3iv,
   0,
   (array_func)glNormal3fv,
   (array_func)glNormal3dv,
};


/* Wrapper functions in case glSecondaryColor*EXT doesn't exist */
static void GLAPIENTRY SecondaryColor3bvEXT(const GLbyte *c)
{
   _glapi_Dispatch->SecondaryColor3bvEXT(c);
}

static void GLAPIENTRY SecondaryColor3ubvEXT(const GLubyte *c)
{
   _glapi_Dispatch->SecondaryColor3ubvEXT(c);
}

static void GLAPIENTRY SecondaryColor3svEXT(const GLshort *c)
{
   _glapi_Dispatch->SecondaryColor3svEXT(c);
}

static void GLAPIENTRY SecondaryColor3usvEXT(const GLushort *c)
{
   _glapi_Dispatch->SecondaryColor3usvEXT(c);
}

static void GLAPIENTRY SecondaryColor3ivEXT(const GLint *c)
{
   _glapi_Dispatch->SecondaryColor3ivEXT(c);
}

static void GLAPIENTRY SecondaryColor3uivEXT(const GLuint *c)
{
   _glapi_Dispatch->SecondaryColor3uivEXT(c);
}

static void GLAPIENTRY SecondaryColor3fvEXT(const GLfloat *c)
{
   _glapi_Dispatch->SecondaryColor3fvEXT(c);
}

static void GLAPIENTRY SecondaryColor3dvEXT(const GLdouble *c)
{
   _glapi_Dispatch->SecondaryColor3dvEXT(c);
}

static array_func secondarycolorfuncs[8] = {
   (array_func) SecondaryColor3bvEXT,
   (array_func) SecondaryColor3ubvEXT,
   (array_func) SecondaryColor3svEXT,
   (array_func) SecondaryColor3usvEXT,
   (array_func) SecondaryColor3ivEXT,
   (array_func) SecondaryColor3uivEXT,
   (array_func) SecondaryColor3fvEXT,
   (array_func) SecondaryColor3dvEXT,
};


/* Again, wrapper functions in case glSecondaryColor*EXT doesn't exist */
static void GLAPIENTRY FogCoordfvEXT(const GLfloat *f)
{
   _glapi_Dispatch->FogCoordfvEXT(f);
}

static void GLAPIENTRY FogCoorddvEXT(const GLdouble *f)
{
   _glapi_Dispatch->FogCoorddvEXT(f);
}

static array_func fogcoordfuncs[8] = {
   0,
   0,
   0,
   0,
   0,
   0,
   (array_func) FogCoordfvEXT,
   (array_func) FogCoorddvEXT
};


/**********************************************************************/

/* GL_BYTE attributes */

static void GLAPIENTRY VertexAttrib1Nbv(GLuint index, const GLbyte *v)
{
   _glapi_Dispatch->VertexAttrib1fNV(index, BYTE_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1bv(GLuint index, const GLbyte *v)
{
      _glapi_Dispatch->VertexAttrib1fNV(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2Nbv(GLuint index, const GLbyte *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, BYTE_TO_FLOAT(v[0]),
                                     BYTE_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2bv(GLuint index, const GLbyte *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3Nbv(GLuint index, const GLbyte *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, BYTE_TO_FLOAT(v[0]),
                                     BYTE_TO_FLOAT(v[1]),
                                     BYTE_TO_FLOAT(v[2]));
}

static void GLAPIENTRY VertexAttrib3bv(GLuint index, const GLbyte *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4Nbv(GLuint index, const GLbyte *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, BYTE_TO_FLOAT(v[0]),
                                     BYTE_TO_FLOAT(v[1]),
                                     BYTE_TO_FLOAT(v[2]),
                                     BYTE_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4bv(GLuint index, const GLbyte *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
}

/* GL_UNSIGNED_BYTE attributes */

static void GLAPIENTRY VertexAttrib1Nubv(GLuint index, const GLubyte *v)
{
   _glapi_Dispatch->VertexAttrib1fNV(index, UBYTE_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1ubv(GLuint index, const GLubyte *v)
{
   _glapi_Dispatch->VertexAttrib1fNV(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2Nubv(GLuint index, const GLubyte *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, UBYTE_TO_FLOAT(v[0]),
                                     UBYTE_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2ubv(GLuint index, const GLubyte *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3Nubv(GLuint index, const GLubyte *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, UBYTE_TO_FLOAT(v[0]),
                                     UBYTE_TO_FLOAT(v[1]),
                                     UBYTE_TO_FLOAT(v[2]));
}
static void GLAPIENTRY VertexAttrib3ubv(GLuint index, const GLubyte *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4Nubv(GLuint index, const GLubyte *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, UBYTE_TO_FLOAT(v[0]),
                                     UBYTE_TO_FLOAT(v[1]),
                                     UBYTE_TO_FLOAT(v[2]),
                                     UBYTE_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4ubv(GLuint index, const GLubyte *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
}

/* GL_SHORT attributes */

static void GLAPIENTRY VertexAttrib1Nsv(GLuint index, const GLshort *v)
{
   _glapi_Dispatch->VertexAttrib1fNV(index, SHORT_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1sv(GLuint index, const GLshort *v)
{
   _glapi_Dispatch->VertexAttrib1fNV(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2Nsv(GLuint index, const GLshort *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, SHORT_TO_FLOAT(v[0]),
                                     SHORT_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2sv(GLuint index, const GLshort *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3Nsv(GLuint index, const GLshort *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, SHORT_TO_FLOAT(v[0]),
                                     SHORT_TO_FLOAT(v[1]),
                                     SHORT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY VertexAttrib3sv(GLuint index, const GLshort *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4Nsv(GLuint index, const GLshort *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, SHORT_TO_FLOAT(v[0]),
                                     SHORT_TO_FLOAT(v[1]),
                                     SHORT_TO_FLOAT(v[2]),
                                     SHORT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4sv(GLuint index, const GLshort *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
}

/* GL_UNSIGNED_SHORT attributes */

static void GLAPIENTRY VertexAttrib1Nusv(GLuint index, const GLushort *v)
{
   _glapi_Dispatch->VertexAttrib1fNV(index, USHORT_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1usv(GLuint index, const GLushort *v)
{
   _glapi_Dispatch->VertexAttrib1fNV(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2Nusv(GLuint index, const GLushort *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, USHORT_TO_FLOAT(v[0]),
                                     USHORT_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2usv(GLuint index, const GLushort *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3Nusv(GLuint index, const GLushort *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, USHORT_TO_FLOAT(v[0]),
                                     USHORT_TO_FLOAT(v[1]),
                                     USHORT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY VertexAttrib3usv(GLuint index, const GLushort *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4Nusv(GLuint index, const GLushort *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, USHORT_TO_FLOAT(v[0]),
                                     USHORT_TO_FLOAT(v[1]),
                                     USHORT_TO_FLOAT(v[2]),
                                     USHORT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4usv(GLuint index, const GLushort *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
}

/* GL_INT attributes */

static void GLAPIENTRY VertexAttrib1Niv(GLuint index, const GLint *v)
{
   _glapi_Dispatch->VertexAttrib1fNV(index, INT_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1iv(GLuint index, const GLint *v)
{
   _glapi_Dispatch->VertexAttrib1fNV(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2Niv(GLuint index, const GLint *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, INT_TO_FLOAT(v[0]),
                                     INT_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2iv(GLuint index, const GLint *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3Niv(GLuint index, const GLint *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, INT_TO_FLOAT(v[0]),
                                     INT_TO_FLOAT(v[1]),
                                     INT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY VertexAttrib3iv(GLuint index, const GLint *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4Niv(GLuint index, const GLint *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, INT_TO_FLOAT(v[0]),
                                     INT_TO_FLOAT(v[1]),
                                     INT_TO_FLOAT(v[2]),
                                     INT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4iv(GLuint index, const GLint *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
}

/* GL_UNSIGNED_INT attributes */

static void GLAPIENTRY VertexAttrib1Nuiv(GLuint index, const GLuint *v)
{
   _glapi_Dispatch->VertexAttrib1fNV(index, UINT_TO_FLOAT(v[0]));
}

static void GLAPIENTRY VertexAttrib1uiv(GLuint index, const GLuint *v)
{
   _glapi_Dispatch->VertexAttrib1fNV(index, v[0]);
}

static void GLAPIENTRY VertexAttrib2Nuiv(GLuint index, const GLuint *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, UINT_TO_FLOAT(v[0]),
                                     UINT_TO_FLOAT(v[1]));
}

static void GLAPIENTRY VertexAttrib2uiv(GLuint index, const GLuint *v)
{
   _glapi_Dispatch->VertexAttrib2fNV(index, v[0], v[1]);
}

static void GLAPIENTRY VertexAttrib3Nuiv(GLuint index, const GLuint *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, UINT_TO_FLOAT(v[0]),
                                     UINT_TO_FLOAT(v[1]),
                                     UINT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY VertexAttrib3uiv(GLuint index, const GLuint *v)
{
   _glapi_Dispatch->VertexAttrib3fNV(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY VertexAttrib4Nuiv(GLuint index, const GLuint *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, UINT_TO_FLOAT(v[0]),
                                     UINT_TO_FLOAT(v[1]),
                                     UINT_TO_FLOAT(v[2]),
                                     UINT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY VertexAttrib4uiv(GLuint index, const GLuint *v)
{
   _glapi_Dispatch->VertexAttrib4fNV(index, v[0], v[1], v[2], v[3]);
}

/* GL_FLOAT attributes */

static void GLAPIENTRY VertexAttrib1fv(GLuint index, const GLfloat *v)
{
   _glapi_Dispatch->VertexAttrib1fvNV(index, v);
}

static void GLAPIENTRY VertexAttrib2fv(GLuint index, const GLfloat *v)
{
   _glapi_Dispatch->VertexAttrib2fvNV(index, v);
}

static void GLAPIENTRY VertexAttrib3fv(GLuint index, const GLfloat *v)
{
   _glapi_Dispatch->VertexAttrib3fvNV(index, v);
}

static void GLAPIENTRY VertexAttrib4fv(GLuint index, const GLfloat *v)
{
   _glapi_Dispatch->VertexAttrib4fvNV(index, v);
}

/* GL_DOUBLE attributes */

static void GLAPIENTRY VertexAttrib1dv(GLuint index, const GLdouble *v)
{
   _glapi_Dispatch->VertexAttrib1dvNV(index, v);
}

static void GLAPIENTRY VertexAttrib2dv(GLuint index, const GLdouble *v)
{
   _glapi_Dispatch->VertexAttrib2dvNV(index, v);
}

static void GLAPIENTRY VertexAttrib3dv(GLuint index, const GLdouble *v)
{
   _glapi_Dispatch->VertexAttrib3dvNV(index, v);
}

static void GLAPIENTRY VertexAttrib4dv(GLuint index, const GLdouble *v)
{
   _glapi_Dispatch->VertexAttrib4dvNV(index, v);
}


/*
 * Array [size][type] of VertexAttrib functions
 */
static attrib_func attribfuncs[2][4][8] = {
   {
      /* non-normalized */
      {
         /* size 1 */
         (attrib_func) VertexAttrib1bv,
         (attrib_func) VertexAttrib1ubv,
         (attrib_func) VertexAttrib1sv,
         (attrib_func) VertexAttrib1usv,
         (attrib_func) VertexAttrib1iv,
         (attrib_func) VertexAttrib1uiv,
         (attrib_func) VertexAttrib1fv,
         (attrib_func) VertexAttrib1dv
      },
      {
         /* size 2 */
         (attrib_func) VertexAttrib2bv,
         (attrib_func) VertexAttrib2ubv,
         (attrib_func) VertexAttrib2sv,
         (attrib_func) VertexAttrib2usv,
         (attrib_func) VertexAttrib2iv,
         (attrib_func) VertexAttrib2uiv,
         (attrib_func) VertexAttrib2fv,
         (attrib_func) VertexAttrib2dv
      },
      {
         /* size 3 */
         (attrib_func) VertexAttrib3bv,
         (attrib_func) VertexAttrib3ubv,
         (attrib_func) VertexAttrib3sv,
         (attrib_func) VertexAttrib3usv,
         (attrib_func) VertexAttrib3iv,
         (attrib_func) VertexAttrib3uiv,
         (attrib_func) VertexAttrib3fv,
         (attrib_func) VertexAttrib3dv
      },
      {
         /* size 4 */
         (attrib_func) VertexAttrib4bv,
         (attrib_func) VertexAttrib4ubv,
         (attrib_func) VertexAttrib4sv,
         (attrib_func) VertexAttrib4usv,
         (attrib_func) VertexAttrib4iv,
         (attrib_func) VertexAttrib4uiv,
         (attrib_func) VertexAttrib4fv,
         (attrib_func) VertexAttrib4dv
      }
   },
   {
      /* normalized (except for float/double) */
      {
         /* size 1 */
         (attrib_func) VertexAttrib1Nbv,
         (attrib_func) VertexAttrib1Nubv,
         (attrib_func) VertexAttrib1Nsv,
         (attrib_func) VertexAttrib1Nusv,
         (attrib_func) VertexAttrib1Niv,
         (attrib_func) VertexAttrib1Nuiv,
         (attrib_func) VertexAttrib1fv,
         (attrib_func) VertexAttrib1dv
      },
      {
         /* size 2 */
         (attrib_func) VertexAttrib2Nbv,
         (attrib_func) VertexAttrib2Nubv,
         (attrib_func) VertexAttrib2Nsv,
         (attrib_func) VertexAttrib2Nusv,
         (attrib_func) VertexAttrib2Niv,
         (attrib_func) VertexAttrib2Nuiv,
         (attrib_func) VertexAttrib2fv,
         (attrib_func) VertexAttrib2dv
      },
      {
         /* size 3 */
         (attrib_func) VertexAttrib3Nbv,
         (attrib_func) VertexAttrib3Nubv,
         (attrib_func) VertexAttrib3Nsv,
         (attrib_func) VertexAttrib3Nusv,
         (attrib_func) VertexAttrib3Niv,
         (attrib_func) VertexAttrib3Nuiv,
         (attrib_func) VertexAttrib3fv,
         (attrib_func) VertexAttrib3dv
      },
      {
         /* size 4 */
         (attrib_func) VertexAttrib4Nbv,
         (attrib_func) VertexAttrib4Nubv,
         (attrib_func) VertexAttrib4Nsv,
         (attrib_func) VertexAttrib4Nusv,
         (attrib_func) VertexAttrib4Niv,
         (attrib_func) VertexAttrib4Nuiv,
         (attrib_func) VertexAttrib4fv,
         (attrib_func) VertexAttrib4dv
      }
   }
};

/**********************************************************************/


GLboolean _ae_create_context( GLcontext *ctx )
{
   if (ctx->aelt_context)
      return GL_TRUE;

   ctx->aelt_context = MALLOC( sizeof(AEcontext) );
   if (!ctx->aelt_context)
      return GL_FALSE;

   AE_CONTEXT(ctx)->NewState = ~0;
   return GL_TRUE;
}


void _ae_destroy_context( GLcontext *ctx )
{
   if ( AE_CONTEXT( ctx ) ) {
      FREE( ctx->aelt_context );
      ctx->aelt_context = 0;
   }
}


/**
 * Make a list of per-vertex functions to call for each glArrayElement call.
 * These functions access the array data (i.e. glVertex, glColor, glNormal, etc);
 */
static void _ae_update_state( GLcontext *ctx )
{
   AEcontext *actx = AE_CONTEXT(ctx);
   AEarray *aa = actx->arrays;
   AEattrib *at = actx->attribs;
   GLuint i;

   /* yuck, no generic array to correspond to color index or edge flag */
   if (ctx->Array.Index.Enabled) {
      aa->array = &ctx->Array.Index;
      aa->func = indexfuncs[TYPE_IDX(aa->array->Type)];
      aa++;
   }
   if (ctx->Array.EdgeFlag.Enabled) {
      aa->array = &ctx->Array.EdgeFlag;
      aa->func = (array_func) glEdgeFlagv;
      aa++;
   }

   /* all other arrays handled here */
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      /* Note: we count down to zero so glVertex (attrib 0) is last!!! */
      const GLuint index = VERT_ATTRIB_MAX - i - 1;
      struct gl_client_array *attribArray = NULL;

      /* Generic arrays take priority over conventional arrays if vertex program
       * mode is enabled.
       */
      if (ctx->VertexProgram.Enabled
          && ctx->Array.VertexAttrib[index].Enabled) {
         if (index == 0) {
            /* Special case: use glVertex() for vertex position so
             * that it's always executed last.
             */
            aa->array = &ctx->Array.VertexAttrib[0];
            aa->func = vertexfuncs[aa->array->Size-2][TYPE_IDX(aa->array->Type)];
            aa++;
         }
         else {
            attribArray = &ctx->Array.VertexAttrib[index];
         }
      }
      else {
         switch (index) {
         case VERT_ATTRIB_POS:
            if (ctx->Array.Vertex.Enabled) {
               aa->array = &ctx->Array.Vertex;
               aa->func = vertexfuncs[aa->array->Size-2][TYPE_IDX(aa->array->Type)];
               aa++;
            }
            break;
         case VERT_ATTRIB_NORMAL:
            if (ctx->Array.Normal.Enabled) {
               aa->array = &ctx->Array.Normal;
               aa->func = normalfuncs[TYPE_IDX(aa->array->Type)];
               aa++;
            }
            break;
         case VERT_ATTRIB_COLOR0:
            if (ctx->Array.Color.Enabled) {
               aa->array = &ctx->Array.Color;
               aa->func = colorfuncs[aa->array->Size-3][TYPE_IDX(aa->array->Type)];
               aa++;
            }
            break;
         case VERT_ATTRIB_COLOR1:
            if (ctx->Array.SecondaryColor.Enabled) {
               aa->array = &ctx->Array.SecondaryColor;
               aa->func = secondarycolorfuncs[TYPE_IDX(aa->array->Type)];
               aa++;
            }
            break;
         case VERT_ATTRIB_FOG:
            if (ctx->Array.FogCoord.Enabled) {
               aa->array = &ctx->Array.FogCoord;
               aa->func = fogcoordfuncs[TYPE_IDX(aa->array->Type)];
               aa++;
            }
            break;
         case VERT_ATTRIB_TEX0:
         case VERT_ATTRIB_TEX1:
         case VERT_ATTRIB_TEX2:
         case VERT_ATTRIB_TEX3:
         case VERT_ATTRIB_TEX4:
         case VERT_ATTRIB_TEX5:
         case VERT_ATTRIB_TEX6:
         case VERT_ATTRIB_TEX7:
            /* use generic vertex attribs for texcoords */
            if (ctx->Array.TexCoord[index - VERT_ATTRIB_TEX0].Enabled) {
               attribArray = &ctx->Array.TexCoord[index - VERT_ATTRIB_TEX0];
            }
         default:
            /* nothing */;
         }
      }

      /* Save glVertexAttrib call (may be for glMultiTexCoord) */
      if (attribArray) {
         at->array = attribArray;
         /* Note: we can't grab the _glapi_Dispatch->VertexAttrib1fvNV
          * function pointer here (for float arrays) since the pointer may
          * change from one execution of _ae_loopback_array_elt() to
          * the next.  Doing so caused UT to break.
          */
         at->func = attribfuncs[at->array->Normalized][at->array->Size-1][TYPE_IDX(at->array->Type)];
         at->index = index;
         at++;
      }
   }
   ASSERT(at - actx->attribs <= VERT_ATTRIB_MAX);
   ASSERT(aa - actx->arrays < 32);
   at->func = NULL;  /* terminate the list */
   aa->func = NULL;  /* terminate the list */

   actx->NewState = 0;
}


void GLAPIENTRY _ae_loopback_array_elt( GLint elt )
{
   GET_CURRENT_CONTEXT(ctx);
   const AEcontext *actx = AE_CONTEXT(ctx);
   const AEarray *aa;
   const AEattrib *at;

   if (actx->NewState)
      _ae_update_state( ctx );

   /* generic attributes */
   for (at = actx->attribs; at->func; at++) {
      const GLubyte *src = at->array->BufferObj->Data
                         + (unsigned long) at->array->Ptr
                         + elt * at->array->StrideB;
      at->func( at->index, src );
   }

   /* conventional arrays */
   for (aa = actx->arrays; aa->func ; aa++) {
      const GLubyte *src = aa->array->BufferObj->Data
                         + (unsigned long) aa->array->Ptr
                         + elt * aa->array->StrideB;
      aa->func( src );
   }
}



void _ae_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   AE_CONTEXT(ctx)->NewState |= new_state;
}
