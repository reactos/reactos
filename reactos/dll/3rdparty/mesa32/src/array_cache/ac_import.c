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
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"

#include "math/m_translate.h"
#include "array_cache/ac_context.h"
#include "math/m_translate.h"

#define STRIDE_ARRAY( array, offset ) 					\
do {									\
   GLubyte *tmp = ADD_POINTERS( (array).BufferObj->Data, (array).Ptr )	\
                + (offset) * (array).StrideB;				\
   (array).Ptr = tmp;							\
} while (0)


/* Set the array pointer back to its source when the cached data is
 * invalidated:
 */
static void
reset_texcoord( GLcontext *ctx, GLuint unit )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array.TexCoord[unit].Enabled) {
      ac->Raw.TexCoord[unit] = ctx->Array.TexCoord[unit];
      STRIDE_ARRAY(ac->Raw.TexCoord[unit], ac->start);
   }
   else {
      ac->Raw.TexCoord[unit] = ac->Fallback.TexCoord[unit];

      if (ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][3] != 1.0)
	 ac->Raw.TexCoord[unit].Size = 4;
      else if (ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][2] != 0.0)
	 ac->Raw.TexCoord[unit].Size = 3;
      else
	 ac->Raw.TexCoord[unit].Size = 2;
   }

   ac->IsCached.TexCoord[unit] = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_TEXCOORD(unit);
}

static void
reset_vertex( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   ASSERT(ctx->Array.Vertex.Enabled
          || (ctx->VertexProgram._Enabled && ctx->Array.VertexAttrib[0].Enabled));
   ac->Raw.Vertex = ctx->Array.Vertex;
   STRIDE_ARRAY(ac->Raw.Vertex, ac->start);
   ac->IsCached.Vertex = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_VERTEX;
}


static void
reset_normal( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array.Normal.Enabled) {
      ac->Raw.Normal = ctx->Array.Normal;
      STRIDE_ARRAY(ac->Raw.Normal, ac->start);
   }
   else {
      ac->Raw.Normal = ac->Fallback.Normal;
   }

   ac->IsCached.Normal = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_NORMAL;
}


static void
reset_color( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array.Color.Enabled) {
      ac->Raw.Color = ctx->Array.Color;
      STRIDE_ARRAY(ac->Raw.Color, ac->start);
   }
   else
      ac->Raw.Color = ac->Fallback.Color;

   ac->IsCached.Color = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_COLOR0;
}


static void
reset_secondarycolor( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array.SecondaryColor.Enabled) {
      ac->Raw.SecondaryColor = ctx->Array.SecondaryColor;
      STRIDE_ARRAY(ac->Raw.SecondaryColor, ac->start);
   }
   else
      ac->Raw.SecondaryColor = ac->Fallback.SecondaryColor;

   ac->IsCached.SecondaryColor = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_COLOR1;
}


static void
reset_index( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array.Index.Enabled) {
      ac->Raw.Index = ctx->Array.Index;
      STRIDE_ARRAY(ac->Raw.Index, ac->start);
   }
   else
      ac->Raw.Index = ac->Fallback.Index;

   ac->IsCached.Index = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_INDEX;
}


static void
reset_fogcoord( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array.FogCoord.Enabled) {
      ac->Raw.FogCoord = ctx->Array.FogCoord;
      STRIDE_ARRAY(ac->Raw.FogCoord, ac->start);
   }
   else
      ac->Raw.FogCoord = ac->Fallback.FogCoord;

   ac->IsCached.FogCoord = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_FOGCOORD;
}


static void
reset_edgeflag( GLcontext *ctx )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array.EdgeFlag.Enabled) {
      ac->Raw.EdgeFlag = ctx->Array.EdgeFlag;
      STRIDE_ARRAY(ac->Raw.EdgeFlag, ac->start);
   }
   else
      ac->Raw.EdgeFlag = ac->Fallback.EdgeFlag;

   ac->IsCached.EdgeFlag = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_EDGEFLAG;
}


static void
reset_attrib( GLcontext *ctx, GLuint index )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (ctx->Array.VertexAttrib[index].Enabled) {
      ac->Raw.Attrib[index] = ctx->Array.VertexAttrib[index];
      STRIDE_ARRAY(ac->Raw.Attrib[index], ac->start);
   }
   else
      ac->Raw.Attrib[index] = ac->Fallback.Attrib[index];

   ac->IsCached.Attrib[index] = GL_FALSE;
   ac->NewArrayState &= ~_NEW_ARRAY_ATTRIB(index);
}


/**
 * Generic import function for color data
 */
static void
import( const GLcontext *ctx,
        GLenum destType,
        struct gl_client_array *to,
        const struct gl_client_array *from )
{
   const ACcontext *ac = AC_CONTEXT(ctx);

   if (destType == 0)
      destType = from->Type;

   switch (destType) {
   case GL_FLOAT:
      _math_trans_4fc( (GLfloat (*)[4]) to->Ptr,
                       from->Ptr,
		       from->StrideB,
		       from->Type,
		       from->Size,
		       0,
		       ac->count - ac->start);

      to->StrideB = 4 * sizeof(GLfloat);
      to->Type = GL_FLOAT;
      break;

   case GL_UNSIGNED_BYTE:
      _math_trans_4ub( (GLubyte (*)[4]) to->Ptr,
                       from->Ptr,
		       from->StrideB,
		       from->Type,
		       from->Size,
		       0,
		       ac->count - ac->start);

      to->StrideB = 4 * sizeof(GLubyte);
      to->Type = GL_UNSIGNED_BYTE;
      break;

   case GL_UNSIGNED_SHORT:
      _math_trans_4us( (GLushort (*)[4]) to->Ptr,
                       from->Ptr,
		       from->StrideB,
		       from->Type,
		       from->Size,
		       0,
		       ac->count - ac->start);

      to->StrideB = 4 * sizeof(GLushort);
      to->Type = GL_UNSIGNED_SHORT;
      break;

   default:
      _mesa_problem(ctx, "Unexpected dest format in import()");
      break;
   }
}



/*
 * Functions to import array ranges with specified types and strides.
 * For example, if the vertex data is GLshort[2] and we want GLfloat[3]
 * we'll use an import function to do the data conversion.
 */

static void
import_texcoord( GLcontext *ctx, GLuint unit, GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   const struct gl_client_array *from = &ac->Raw.TexCoord[unit];
   struct gl_client_array *to = &ac->Cache.TexCoord[unit];
   (void) type; (void) stride;

   ASSERT(unit < ctx->Const.MaxTextureCoordUnits);

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_FLOAT);
   ASSERT(stride == 4*sizeof(GLfloat) || stride == 0);
   ASSERT(ac->count - ac->start < ctx->Const.MaxArrayLockSize);

   _math_trans_4f( (GLfloat (*)[4]) to->Ptr,
		   from->Ptr,
		   from->StrideB,
		   from->Type,
		   from->Size,
		   0,
		   ac->count - ac->start);

   to->Size = from->Size;
   to->StrideB = 4 * sizeof(GLfloat);
   to->Type = GL_FLOAT;
   ac->IsCached.TexCoord[unit] = GL_TRUE;
}

static void
import_vertex( GLcontext *ctx, GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   const struct gl_client_array *from = &ac->Raw.Vertex;
   struct gl_client_array *to = &ac->Cache.Vertex;
   (void) type; (void) stride;

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_FLOAT);
   ASSERT(stride == 4*sizeof(GLfloat) || stride == 0);

   _math_trans_4f( (GLfloat (*)[4]) to->Ptr,
		   from->Ptr,
		   from->StrideB,
		   from->Type,
		   from->Size,
		   0,
		   ac->count - ac->start);

   to->Size = from->Size;
   to->StrideB = 4 * sizeof(GLfloat);
   to->Type = GL_FLOAT;
   ac->IsCached.Vertex = GL_TRUE;
}

static void
import_normal( GLcontext *ctx, GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   const struct gl_client_array *from = &ac->Raw.Normal;
   struct gl_client_array *to = &ac->Cache.Normal;
   (void) type; (void) stride;

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_FLOAT);
   ASSERT(stride == 3*sizeof(GLfloat) || stride == 0);

   _math_trans_3f( (GLfloat (*)[3]) to->Ptr,
		   from->Ptr,
		   from->StrideB,
		   from->Type,
		   0,
		   ac->count - ac->start);

   to->StrideB = 3 * sizeof(GLfloat);
   to->Type = GL_FLOAT;
   ac->IsCached.Normal = GL_TRUE;
}

static void
import_color( GLcontext *ctx, GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   const struct gl_client_array *from = &ac->Raw.Color;
   struct gl_client_array *to = &ac->Cache.Color;
   (void) stride;

   import( ctx, type, to, from );

   ac->IsCached.Color = GL_TRUE;
}

static void
import_index( GLcontext *ctx, GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   const struct gl_client_array *from = &ac->Raw.Index;
   struct gl_client_array *to = &ac->Cache.Index;
   (void) type; (void) stride;

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_UNSIGNED_INT);
   ASSERT(stride == sizeof(GLuint) || stride == 0);

   _math_trans_1ui( (GLuint *) to->Ptr,
		    from->Ptr,
		    from->StrideB,
		    from->Type,
		    0,
		    ac->count - ac->start);

   to->StrideB = sizeof(GLuint);
   to->Type = GL_UNSIGNED_INT;
   ac->IsCached.Index = GL_TRUE;
}

static void
import_secondarycolor( GLcontext *ctx, GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   const struct gl_client_array *from = &ac->Raw.SecondaryColor;
   struct gl_client_array *to = &ac->Cache.SecondaryColor;
   (void) stride;

   import( ctx, type, to, from );

   ac->IsCached.SecondaryColor = GL_TRUE;
}

static void
import_fogcoord( GLcontext *ctx, GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   const struct gl_client_array *from = &ac->Raw.FogCoord;
   struct gl_client_array *to = &ac->Cache.FogCoord;
   (void) type; (void) stride;

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_FLOAT);
   ASSERT(stride == sizeof(GLfloat) || stride == 0);

   _math_trans_1f( (GLfloat *) to->Ptr,
		   from->Ptr,
		   from->StrideB,
		   from->Type,
		   0,
		   ac->count - ac->start);

   to->StrideB = sizeof(GLfloat);
   to->Type = GL_FLOAT;
   ac->IsCached.FogCoord = GL_TRUE;
}

static void
import_edgeflag( GLcontext *ctx, GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   const struct gl_client_array *from = &ac->Raw.EdgeFlag;
   struct gl_client_array *to = &ac->Cache.EdgeFlag;
   (void) type; (void) stride;

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_UNSIGNED_BYTE);
   ASSERT(stride == sizeof(GLubyte) || stride == 0);

   _math_trans_1ub( (GLubyte *) to->Ptr,
		    from->Ptr,
		    from->StrideB,
		    from->Type,
		    0,
		    ac->count - ac->start);

   to->StrideB = sizeof(GLubyte);
   to->Type = GL_UNSIGNED_BYTE;
   ac->IsCached.EdgeFlag = GL_TRUE;
}

static void
import_attrib( GLcontext *ctx, GLuint index, GLenum type, GLuint stride )
{
   ACcontext *ac = AC_CONTEXT(ctx);
   const struct gl_client_array *from = &ac->Raw.Attrib[index];
   struct gl_client_array *to = &ac->Cache.Attrib[index];
   (void) type; (void) stride;

   ASSERT(index < MAX_VERTEX_PROGRAM_ATTRIBS);

   /* Limited choices at this stage:
    */
   ASSERT(type == GL_FLOAT);
   ASSERT(stride == 4*sizeof(GLfloat) || stride == 0);
   ASSERT(ac->count - ac->start < ctx->Const.MaxArrayLockSize);

   _math_trans_4f( (GLfloat (*)[4]) to->Ptr,
		   from->Ptr,
		   from->StrideB,
		   from->Type,
		   from->Size,
		   0,
		   ac->count - ac->start);

   to->Size = from->Size;
   to->StrideB = 4 * sizeof(GLfloat);
   to->Type = GL_FLOAT;
   ac->IsCached.Attrib[index] = GL_TRUE;
}



/*
 * Externals to request arrays with specific properties:
 */


struct gl_client_array *
_ac_import_texcoord( GLcontext *ctx,
                     GLuint unit,
                     GLenum type,
                     GLuint reqstride,
                     GLuint reqsize,
                     GLboolean reqwriteable,
                     GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   ASSERT(unit < ctx->Const.MaxTextureCoordUnits);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_TEXCOORD(unit))
      reset_texcoord( ctx, unit );

   /* Is the request impossible?
    */
   if (reqsize != 0 && ac->Raw.TexCoord[unit].Size > (GLint) reqsize)
      return NULL;

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Raw.TexCoord[unit].Type != type ||
       (reqstride != 0 && ac->Raw.TexCoord[unit].StrideB != (GLint)reqstride) ||
       reqwriteable)
   {
      if (!ac->IsCached.TexCoord[unit])
	 import_texcoord(ctx, unit, type, reqstride );
      *writeable = GL_TRUE;
      return &ac->Cache.TexCoord[unit];
   }
   else {
      *writeable = GL_FALSE;
      return &ac->Raw.TexCoord[unit];
   }
}

struct gl_client_array *
_ac_import_vertex( GLcontext *ctx,
                   GLenum type,
                   GLuint reqstride,
                   GLuint reqsize,
                   GLboolean reqwriteable,
                   GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_VERTEX)
      reset_vertex( ctx );

   /* Is the request impossible?
    */
   if (reqsize != 0 && ac->Raw.Vertex.Size > (GLint) reqsize)
      return NULL;

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Raw.Vertex.Type != type ||
       (reqstride != 0 && ac->Raw.Vertex.StrideB != (GLint) reqstride) ||
       reqwriteable)
   {
      if (!ac->IsCached.Vertex)
	 import_vertex(ctx, type, reqstride );
      *writeable = GL_TRUE;
      return &ac->Cache.Vertex;
   }
   else {
      *writeable = GL_FALSE;
      return &ac->Raw.Vertex;
   }
}

struct gl_client_array *
_ac_import_normal( GLcontext *ctx,
                   GLenum type,
                   GLuint reqstride,
                   GLboolean reqwriteable,
                   GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_NORMAL)
      reset_normal( ctx );

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Raw.Normal.Type != type ||
       (reqstride != 0 && ac->Raw.Normal.StrideB != (GLint) reqstride) ||
       reqwriteable)
   {
      if (!ac->IsCached.Normal)
	 import_normal(ctx, type, reqstride );
      *writeable = GL_TRUE;
      return &ac->Cache.Normal;
   }
   else {
      *writeable = GL_FALSE;
      return &ac->Raw.Normal;
   }
}

struct gl_client_array *
_ac_import_color( GLcontext *ctx,
                  GLenum type,
                  GLuint reqstride,
                  GLuint reqsize,
                  GLboolean reqwriteable,
                  GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_COLOR0)
      reset_color( ctx );

   /* Is the request impossible?
    */
   if (reqsize != 0 && ac->Raw.Color.Size > (GLint) reqsize) {
      return NULL;
   }

   /* Do we need to pull in a copy of the client data:
    */
   if ((type != 0 && ac->Raw.Color.Type != type) ||
       (reqstride != 0 && ac->Raw.Color.StrideB != (GLint) reqstride) ||
       reqwriteable)
   {
      if (!ac->IsCached.Color) {
      	 import_color(ctx, type, reqstride );
      }
      *writeable = GL_TRUE;
      return &ac->Cache.Color;
   }
   else {
      *writeable = GL_FALSE;
      return &ac->Raw.Color;
   }
}

struct gl_client_array *
_ac_import_index( GLcontext *ctx,
                  GLenum type,
                  GLuint reqstride,
                  GLboolean reqwriteable,
                  GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_INDEX)
      reset_index( ctx );


   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Raw.Index.Type != type ||
       (reqstride != 0 && ac->Raw.Index.StrideB != (GLint) reqstride) ||
       reqwriteable)
   {
      if (!ac->IsCached.Index)
	 import_index(ctx, type, reqstride );
      *writeable = GL_TRUE;
      return &ac->Cache.Index;
   }
   else {
      *writeable = GL_FALSE;
      return &ac->Raw.Index;
   }
}

struct gl_client_array *
_ac_import_secondarycolor( GLcontext *ctx,
                           GLenum type,
                           GLuint reqstride,
                           GLuint reqsize,
                           GLboolean reqwriteable,
                           GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_COLOR1)
      reset_secondarycolor( ctx );

   /* Is the request impossible?
    */
   if (reqsize != 0 && ac->Raw.SecondaryColor.Size > (GLint) reqsize)
      return NULL;

   /* Do we need to pull in a copy of the client data:
    */
   if ((type != 0 && ac->Raw.SecondaryColor.Type != type) ||
       (reqstride != 0 && ac->Raw.SecondaryColor.StrideB != (GLint)reqstride) ||
       reqwriteable)
   {
      if (!ac->IsCached.SecondaryColor)
	 import_secondarycolor(ctx, type, reqstride );
      *writeable = GL_TRUE;
      return &ac->Cache.SecondaryColor;
   }
   else {
      *writeable = GL_FALSE;
      return &ac->Raw.SecondaryColor;
   }
}

struct gl_client_array *
_ac_import_fogcoord( GLcontext *ctx,
                     GLenum type,
                     GLuint reqstride,
                     GLboolean reqwriteable,
                     GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_FOGCOORD)
      reset_fogcoord( ctx );

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Raw.FogCoord.Type != type ||
       (reqstride != 0 && ac->Raw.FogCoord.StrideB != (GLint) reqstride) ||
       reqwriteable)
   {
      if (!ac->IsCached.FogCoord)
	 import_fogcoord(ctx, type, reqstride );
      *writeable = GL_TRUE;
      return &ac->Cache.FogCoord;
   }
   else {
      *writeable = GL_FALSE;
      return &ac->Raw.FogCoord;
   }
}

struct gl_client_array *
_ac_import_edgeflag( GLcontext *ctx,
                     GLenum type,
                     GLuint reqstride,
                     GLboolean reqwriteable,
                     GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_EDGEFLAG)
      reset_edgeflag( ctx );

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Raw.EdgeFlag.Type != type ||
       (reqstride != 0 && ac->Raw.EdgeFlag.StrideB != (GLint) reqstride) ||
       reqwriteable)
   {
      if (!ac->IsCached.EdgeFlag)
	 import_edgeflag(ctx, type, reqstride );
      *writeable = GL_TRUE;
      return &ac->Cache.EdgeFlag;
   }
   else {
      *writeable = GL_FALSE;
      return &ac->Raw.EdgeFlag;
   }
}

/* GL_NV_vertex_program */
struct gl_client_array *
_ac_import_attrib( GLcontext *ctx,
                   GLuint index,
                   GLenum type,
                   GLuint reqstride,
                   GLuint reqsize,
                   GLboolean reqwriteable,
                   GLboolean *writeable )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   ASSERT(index < VERT_ATTRIB_MAX);

   /* Can we keep the existing version?
    */
   if (ac->NewArrayState & _NEW_ARRAY_ATTRIB(index))
      reset_attrib( ctx, index );

   /* Is the request impossible?
    */
   if (reqsize != 0 && ac->Raw.Attrib[index].Size > (GLint) reqsize)
      return NULL;

   /* Do we need to pull in a copy of the client data:
    */
   if (ac->Raw.Attrib[index].Type != type ||
       (reqstride != 0 && ac->Raw.Attrib[index].StrideB != (GLint)reqstride) ||
       reqwriteable)
   {
      if (!ac->IsCached.Attrib[index])
	 import_attrib(ctx, index, type, reqstride );
      *writeable = GL_TRUE;
      return &ac->Cache.Attrib[index];
   }
   else {
      *writeable = GL_FALSE;
      return &ac->Raw.Attrib[index];
   }
}


/* Clients must call this function to validate state and set bounds
 * before importing any data:
 */
void
_ac_import_range( GLcontext *ctx, GLuint start, GLuint count )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (!ctx->Array.LockCount) {
      /* Not locked, discard cached data.  Changes to lock
       * status are caught via. _ac_invalidate_state().
       */
      ac->NewArrayState = _NEW_ARRAY_ALL;
      ac->start = start;
      ac->count = count;
   }
   else {
      /* Locked, discard data for any disabled arrays.  Require that
       * the whole locked range always be dealt with, otherwise hard to
       * maintain cached data in the face of clipping.
       */
      ac->NewArrayState |= ~ctx->Array._Enabled;
      ac->start = ctx->Array.LockFirst;
      ac->count = ctx->Array.LockCount;
      ASSERT(ac->start == start); /* hmm? */
      ASSERT(ac->count == count);
   }
}



/* Additional convienence function for importing the element list
 * for glDrawElements() and glDrawRangeElements().
 */
CONST void *
_ac_import_elements( GLcontext *ctx,
		     GLenum new_type,
		     GLuint count,
		     GLenum old_type,
		     CONST void *indices )
{
   ACcontext *ac = AC_CONTEXT(ctx);

   if (old_type == new_type)
      return indices;

   if (ac->elt_size < count * sizeof(GLuint)) {
      if (ac->Elts) FREE(ac->Elts);
      while (ac->elt_size < count * sizeof(GLuint))
	 ac->elt_size *= 2;
      ac->Elts = (GLuint *) MALLOC(ac->elt_size);
   }

   switch (new_type) {
   case GL_UNSIGNED_BYTE:
      ASSERT(0);
      return NULL;
   case GL_UNSIGNED_SHORT:
      ASSERT(0);
      return NULL;
   case GL_UNSIGNED_INT: {
      GLuint *out = (GLuint *)ac->Elts;
      GLuint i;

      switch (old_type) {
      case GL_UNSIGNED_BYTE: {
	 CONST GLubyte *in = (CONST GLubyte *)indices;
	 for (i = 0 ; i < count ; i++)
	    out[i] = in[i];
	 break;
      }
      case GL_UNSIGNED_SHORT: {
	 CONST GLushort *in = (CONST GLushort *)indices;
	 for (i = 0 ; i < count ; i++)
	       out[i] = in[i];
	 break;
      }
      default:
	 ASSERT(0);
      }

      return (CONST void *)out;
   }
   default:
      ASSERT(0);
      break;
   }

   return NULL;
}
