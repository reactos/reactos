/*
 * Mesa 3-D graphics library
 * Version:  6.3
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
#include "context.h"
#include "enums.h"
#include "glapi.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "t_context.h"
#include "t_save_api.h"
#include "dispatch.h"

/* If someone compiles a display list like:
 *      glBegin(Triangles)
 *      glVertex() 
 *      ... lots of vertices ...
 *      glEnd()
 *
 * or: 
 *      glDrawArrays(...)
 * 
 * and then tries to execute it like this:
 *
 *      glBegin(Lines)
 *      glCallList()
 *      glEnd()
 *
 * it will wind up in here, as the vertex copying used when wrapping
 * buffers in list compilation (Triangles) won't be right for how the
 * list is being executed (as Lines). 
 *
 * This could be avoided by not compiling as vertex_lists until after
 * the first glEnd() has been seen.  However, that would miss an
 * important category of display lists, for the sake of a degenerate
 * usage. 
 *
 * Further, replaying degenerately-called lists in this fashion is
 * probably still faster than the replay using opcodes.
 */

typedef void (*attr_func)( GLcontext *ctx, GLint target, const GLfloat * );


/* Wrapper functions in case glVertexAttrib*fvNV doesn't exist */
static void VertexAttrib1fvNV(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib1fvNV(ctx->Exec, (target, v));
}

static void VertexAttrib2fvNV(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib2fvNV(ctx->Exec, (target, v));
}

static void VertexAttrib3fvNV(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib3fvNV(ctx->Exec, (target, v));
}

static void VertexAttrib4fvNV(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib4fvNV(ctx->Exec, (target, v));
}

static attr_func vert_attrfunc[4] = {
   VertexAttrib1fvNV,
   VertexAttrib2fvNV,
   VertexAttrib3fvNV,
   VertexAttrib4fvNV
};


static void VertexAttrib1fvARB(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib1fvARB(ctx->Exec, (target, v));
}

static void VertexAttrib2fvARB(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib2fvARB(ctx->Exec, (target, v));
}

static void VertexAttrib3fvARB(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib3fvARB(ctx->Exec, (target, v));
}

static void VertexAttrib4fvARB(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib4fvARB(ctx->Exec, (target, v));
}

static attr_func vert_attrfunc_arb[4] = {
   VertexAttrib1fvARB,
   VertexAttrib2fvARB,
   VertexAttrib3fvARB,
   VertexAttrib4fvARB
};







static void mat_attr1fv( GLcontext *ctx, GLint target, const GLfloat *v )
{
   switch (target) {
   case _TNL_ATTRIB_MAT_FRONT_SHININESS:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_SHININESS, v ));
      break;
   case _TNL_ATTRIB_MAT_BACK_SHININESS:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_SHININESS, v ));
      break;
   }
}


static void mat_attr3fv( GLcontext *ctx, GLint target, const GLfloat *v )
{
   switch (target) {
   case _TNL_ATTRIB_MAT_FRONT_INDEXES:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_COLOR_INDEXES, v ));
      break;
   case _TNL_ATTRIB_MAT_BACK_INDEXES:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_COLOR_INDEXES, v ));
      break;
   }
}


static void mat_attr4fv( GLcontext *ctx, GLint target, const GLfloat *v )
{
   switch (target) {
   case _TNL_ATTRIB_MAT_FRONT_EMISSION:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_EMISSION, v ));
      break;
   case _TNL_ATTRIB_MAT_BACK_EMISSION:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_EMISSION, v ));
      break;
   case _TNL_ATTRIB_MAT_FRONT_AMBIENT:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_AMBIENT, v ));
      break;
   case _TNL_ATTRIB_MAT_BACK_AMBIENT:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_AMBIENT, v ));
      break;
   case _TNL_ATTRIB_MAT_FRONT_DIFFUSE:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_DIFFUSE, v ));
      break;
   case _TNL_ATTRIB_MAT_BACK_DIFFUSE:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_DIFFUSE, v ));
      break;
   case _TNL_ATTRIB_MAT_FRONT_SPECULAR:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_SPECULAR, v ));
      break;
   case _TNL_ATTRIB_MAT_BACK_SPECULAR:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_SPECULAR, v ));
      break;
   }
}


static attr_func mat_attrfunc[4] = {
   mat_attr1fv,
   NULL,
   mat_attr3fv,
   mat_attr4fv
};


static void index_attr1fv(GLcontext *ctx, GLint target, const GLfloat *v)
{
   (void) target;
   CALL_Indexf(ctx->Exec, (v[0]));
}

static void edgeflag_attr1fv(GLcontext *ctx, GLint target, const GLfloat *v)
{
   (void) target;
   CALL_EdgeFlag(ctx->Exec, ((GLboolean)(v[0] == 1.0)));
}

struct loopback_attr {
   GLint target;
   GLint sz;
   attr_func func;
};

/* Don't emit ends and begins on wrapped primitives.  Don't replay
 * wrapped vertices.  If we get here, it's probably because the the
 * precalculated wrapping is wrong.
 */
static void loopback_prim( GLcontext *ctx,
			   const struct tnl_vertex_list *list, GLuint i,
			   const struct loopback_attr *la, GLuint nr )
{
   struct tnl_prim *prim = &list->prim[i];
   GLint begin = prim->start;
   GLint end = begin + prim->count;
   GLfloat *data;
   GLint j;
   GLuint k;

   if (prim->mode & PRIM_BEGIN) {
      CALL_Begin(GET_DISPATCH(), ( prim->mode & PRIM_MODE_MASK ));
   }
   else {
      assert(i == 0);
      assert(begin == 0);
      begin += list->wrap_count;
   }

   data = list->buffer + begin * list->vertex_size;

   for (j = begin ; j < end ; j++) {
      GLfloat *tmp = data + la[0].sz;

      for (k = 1 ; k < nr ; k++) {
	 la[k].func( ctx, la[k].target, tmp );
	 tmp += la[k].sz;
      }
	 
      /* Fire the vertex
       */
      la[0].func( ctx, VERT_ATTRIB_POS, data );
      data = tmp;
   }

   if (prim->mode & PRIM_END) {
      CALL_End(GET_DISPATCH(), ());
   }
   else {
      assert (i == list->prim_count-1);
   }
}

/* Primitives generated by DrawArrays/DrawElements/Rectf may be
 * caught here.  If there is no primitive in progress, execute them
 * normally, otherwise need to track and discard the generated
 * primitives.
 */
static void loopback_weak_prim( GLcontext *ctx,
				const struct tnl_vertex_list *list, GLuint i,
				const struct loopback_attr *la, GLuint nr )
{
   if (ctx->Driver.CurrentExecPrimitive == PRIM_OUTSIDE_BEGIN_END) 
      loopback_prim( ctx, list, i, la, nr );
   else {
      struct tnl_prim *prim = &list->prim[i];

      /* Use the prim_weak flag to ensure that if this primitive
       * wraps, we don't mistake future vertex_lists for part of the
       * surrounding primitive.
       *
       * While this flag is set, we are simply disposing of data
       * generated by an operation now known to be a noop.
       */
      if (prim->mode & PRIM_BEGIN)
	 ctx->Driver.CurrentExecPrimitive |= PRIM_WEAK;
      if (prim->mode & PRIM_END)
	 ctx->Driver.CurrentExecPrimitive &= ~PRIM_WEAK;
   }
}



void _tnl_loopback_vertex_list( GLcontext *ctx,
                                const struct tnl_vertex_list *list )
{
   struct loopback_attr la[_TNL_ATTRIB_MAX];
   GLuint i, nr = 0;

   for (i = 0 ; i <= _TNL_ATTRIB_TEX7 ; i++) {
      if (list->attrsz[i]) {
	 la[nr].target = i;
	 la[nr].sz = list->attrsz[i];
	 la[nr].func = vert_attrfunc[list->attrsz[i]-1];
	 nr++;
      }
   }

   for (i = _TNL_ATTRIB_MAT_FRONT_AMBIENT ; 
	i <= _TNL_ATTRIB_MAT_BACK_INDEXES ; 
	i++) {
      if (list->attrsz[i]) {
	 la[nr].target = i;
	 la[nr].sz = list->attrsz[i];
	 la[nr].func = mat_attrfunc[list->attrsz[i]-1];
	 nr++;
      }
   }

   if (list->attrsz[_TNL_ATTRIB_EDGEFLAG]) {
      la[nr].target = _TNL_ATTRIB_EDGEFLAG;
      la[nr].sz = list->attrsz[_TNL_ATTRIB_EDGEFLAG];
      la[nr].func = edgeflag_attr1fv;
      nr++;
   }

   if (list->attrsz[_TNL_ATTRIB_INDEX]) {
      la[nr].target = _TNL_ATTRIB_INDEX;
      la[nr].sz = list->attrsz[_TNL_ATTRIB_INDEX];
      la[nr].func = index_attr1fv;
      nr++;
   }

   /* XXX ARB vertex attribs */

   for (i = 0 ; i < list->prim_count ; i++) {
      if (list->prim[i].mode & PRIM_WEAK)
	 loopback_weak_prim( ctx, list, i, la, nr );
      else
	 loopback_prim( ctx, list, i, la, nr );
   }
}
