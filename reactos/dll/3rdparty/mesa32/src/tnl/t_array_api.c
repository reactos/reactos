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

/**
 * \file t_array_api.c
 * \brief Vertex array API functions (glDrawArrays, etc)
 * \author Keith Whitwell
 */

#include "glheader.h"
#include "api_validate.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "state.h"

#include "array_cache/acache.h"

#include "t_array_api.h"
#include "t_array_import.h"
#include "t_save_api.h"
#include "t_context.h"
#include "t_pipeline.h"
#include "dispatch.h"

static void fallback_drawarrays( GLcontext *ctx, GLenum mode, GLint start,
				 GLsizei count )
{
   GLint i;

   assert(!ctx->CompileFlag);
   assert(ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1);

   CALL_Begin(GET_DISPATCH(), (mode));
   for (i = 0; i < count; i++)
       CALL_ArrayElement(GET_DISPATCH(), ( start + i ));
   CALL_End(GET_DISPATCH(), ());
}


static void fallback_drawelements( GLcontext *ctx, GLenum mode, GLsizei count,
				   const GLuint *indices)
{
   GLint i;

   assert(!ctx->CompileFlag);
   assert(ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1);

   /* Here, indices will already reflect the buffer object if active */

   CALL_Begin(GET_DISPATCH(), (mode));
   for (i = 0 ; i < count ; i++) {
      CALL_ArrayElement(GET_DISPATCH(), ( indices[i] ));
   }
   CALL_End(GET_DISPATCH(), ());
}


/* Note this function no longer takes a 'start' value, the range is
 * assumed to start at zero.  The old trick of subtracting 'start'
 * from each index won't work if the indices are not in writeable
 * memory.
 */
static void _tnl_draw_range_elements( GLcontext *ctx, GLenum mode,
				      GLuint max_index,
				      GLsizei index_count, GLuint *indices )

{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct tnl_prim prim;
   FLUSH_CURRENT( ctx, 0 );

   _tnl_vb_bind_arrays( ctx, 0, max_index );

   tnl->vb.Primitive = &prim;
   tnl->vb.Primitive[0].mode = mode | PRIM_BEGIN | PRIM_END;
   tnl->vb.Primitive[0].start = 0;
   tnl->vb.Primitive[0].count = index_count;
   tnl->vb.PrimitiveCount = 1;

   tnl->vb.Elts = (GLuint *)indices;

   tnl->Driver.RunPipeline( ctx );
}



/**
 * Called via the GL API dispatcher.
 */
void GLAPIENTRY
_tnl_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint thresh = (ctx->Driver.NeedFlush & FLUSH_STORED_VERTICES) ? 30 : 10;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(NULL, "_tnl_DrawArrays %d %d\n", start, count);

   /* Check arguments, etc.
    */
   if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
      return;

   assert(!ctx->CompileFlag);

   if (!ctx->Array.LockCount && (GLuint) count < thresh) {
      /* Small primitives: attempt to share a vb (at the expense of
       * using the immediate interface).
      */
      fallback_drawarrays( ctx, mode, start, count );
   }
   else if (start >= (GLint) ctx->Array.LockFirst &&
	    start + count <= (GLint)(ctx->Array.LockFirst + ctx->Array.LockCount)) {

      struct tnl_prim prim;

      /* Locked primitives which can fit in a single vertex buffer:
       */
      FLUSH_CURRENT( ctx, 0 );

      /* Locked drawarrays.  Reuse any previously transformed data.
       */
      _tnl_vb_bind_arrays( ctx, ctx->Array.LockFirst,
			   ctx->Array.LockFirst + ctx->Array.LockCount );

      tnl->vb.Primitive = &prim;
      tnl->vb.Primitive[0].mode = mode | PRIM_BEGIN | PRIM_END;
      tnl->vb.Primitive[0].start = start;
      tnl->vb.Primitive[0].count = count;
      tnl->vb.PrimitiveCount = 1;

      tnl->Driver.RunPipeline( ctx );
   }
   else {
      int bufsz = 256;		/* Use a small buffer for cache goodness */
      int j, nr;
      int minimum, modulo, skip;

      /* Large primitives requiring decomposition to multiple vertex
       * buffers:
       */
      switch (mode) {
      case GL_POINTS:
	 minimum = 0;
	 modulo = 1;
	 skip = 0;
	 break;
      case GL_LINES:
	 minimum = 1;
	 modulo = 2;
	 skip = 1;
	 break;
      case GL_LINE_STRIP:
	 minimum = 1;
	 modulo = 1;
	 skip = 0;
	 break;
      case GL_TRIANGLES:
	 minimum = 2;
	 modulo = 3;
	 skip = 2;
	 break;
      case GL_TRIANGLE_STRIP:
	 minimum = 2;
	 modulo = 1;
	 skip = 0;
	 break;
      case GL_QUADS:
	 minimum = 3;
	 modulo = 4;
	 skip = 3;
	 break;
      case GL_QUAD_STRIP:
	 minimum = 3;
	 modulo = 2;
	 skip = 0;
	 break;
      case GL_LINE_LOOP:
      case GL_TRIANGLE_FAN:
      case GL_POLYGON:
      default:
	 /* Primitives requiring a copied vertex (fan-like primitives)
	  * must use the slow path if they cannot fit in a single
	  * vertex buffer.
	  */
	 if (count <= (GLint) ctx->Const.MaxArrayLockSize) {
	    bufsz = ctx->Const.MaxArrayLockSize;
	    minimum = 0;
	    modulo = 1;
	    skip = 0;
	 }
	 else {
	    fallback_drawarrays( ctx, mode, start, count );
	    return;
	 }
      }

      FLUSH_CURRENT( ctx, 0 );

      bufsz -= bufsz % modulo;
      bufsz -= minimum;
      count += start;

      for (j = start + minimum ; j < count ; j += nr + skip ) {

	 struct tnl_prim prim;

	 nr = MIN2( bufsz, count - j );

         /* XXX is the last parameter a count or index into the array??? */
	 _tnl_vb_bind_arrays( ctx, j - minimum, j + nr );

	 tnl->vb.Primitive = &prim;
	 tnl->vb.Primitive[0].mode = mode;

	 if (j == start + minimum)
	    tnl->vb.Primitive[0].mode |= PRIM_BEGIN;

	 if (j + nr + skip >= count)
	    tnl->vb.Primitive[0].mode |= PRIM_END;

	 tnl->vb.Primitive[0].start = 0;
	 tnl->vb.Primitive[0].count = nr + minimum;
	 tnl->vb.PrimitiveCount = 1;

	 tnl->Driver.RunPipeline( ctx );
      }
   }
}


/**
 * Called via the GL API dispatcher.
 */
void GLAPIENTRY
_tnl_DrawRangeElements(GLenum mode,
		       GLuint start, GLuint end,
		       GLsizei count, GLenum type, const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint *ui_indices;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(NULL, "_tnl_DrawRangeElements %d %d %d\n", start, end, count);

   if (ctx->Array.ElementArrayBufferObj->Name) {
      /* use indices in the buffer object */
      if (!ctx->Array.ElementArrayBufferObj->Data) {
         _mesa_warning(ctx,
                       "DrawRangeElements with empty vertex elements buffer!");
         return;
      }
      /* actual address is the sum of pointers */
      indices = (const GLvoid *)
         ADD_POINTERS(ctx->Array.ElementArrayBufferObj->Data,
                      (const GLubyte *) indices);
   }

   /* Check arguments, etc.
    */
   if (!_mesa_validate_DrawRangeElements( ctx, mode, start, end, count,
                                          type, indices ))
      return;

   ui_indices = (GLuint *)_ac_import_elements( ctx, GL_UNSIGNED_INT,
					       count, type, indices );


   assert(!ctx->CompileFlag);

   if (ctx->Array.LockCount) {
      /* Are the arrays already locked?  If so we currently have to look
       * at the whole locked range.
       */

      if (start == 0 && ctx->Array.LockFirst == 0 &&
	  end < (ctx->Array.LockFirst + ctx->Array.LockCount))
	 _tnl_draw_range_elements( ctx, mode,
				   ctx->Array.LockCount,
				   count, ui_indices );
      else {
	 fallback_drawelements( ctx, mode, count, ui_indices );
      }
   }
   else if (start == 0 && end < ctx->Const.MaxArrayLockSize) {
      /* The arrays aren't locked but we can still fit them inside a
       * single vertexbuffer.
       */
      _tnl_draw_range_elements( ctx, mode, end + 1, count, ui_indices );
   }
   else {
      /* Range is too big to optimize:
       */
      fallback_drawelements( ctx, mode, count, ui_indices );
   }
}



/**
 * Called via the GL API dispatcher.
 */
void GLAPIENTRY
_tnl_DrawElements(GLenum mode, GLsizei count, GLenum type,
		  const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint *ui_indices;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(NULL, "_tnl_DrawElements %d\n", count);

   /* Check arguments, etc. */
   if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices ))
      return;

   if (ctx->Array.ElementArrayBufferObj->Name) {
      /* actual address is the sum of pointers */
      indices = (const GLvoid *)
         ADD_POINTERS(ctx->Array.ElementArrayBufferObj->Data,
                      (const GLubyte *) indices);
   }

   ui_indices = (GLuint *)_ac_import_elements( ctx, GL_UNSIGNED_INT,
					       count, type, indices );

   assert(!ctx->CompileFlag);

   if (ctx->Array.LockCount) {
      if (ctx->Array.LockFirst == 0)
	 _tnl_draw_range_elements( ctx, mode,
				   ctx->Array.LockCount,
				   count, ui_indices );
      else
	 fallback_drawelements( ctx, mode, count, ui_indices );
   }
   else {
      /* Scan the index list and see if we can use the locked path anyway.
       */
      GLuint max_elt = 0;
      GLint i;

      for (i = 0 ; i < count ; i++)
	 if (ui_indices[i] > max_elt)
            max_elt = ui_indices[i];

      if (max_elt < ctx->Const.MaxArrayLockSize && /* can we use it? */
	  max_elt < (GLuint) count) 	           /* do we want to use it? */
	 _tnl_draw_range_elements( ctx, mode, max_elt+1, count, ui_indices );
      else
	 fallback_drawelements( ctx, mode, count, ui_indices );
   }
}


/**
 * Initialize context's vertex array fields.  Called during T 'n L context
 * creation.
 */
void _tnl_array_init( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct tnl_vertex_arrays *tmp = &tnl->array_inputs;
   GLvertexformat *vfmt = &(TNL_CONTEXT(ctx)->exec_vtxfmt);
   GLuint i;

   vfmt->DrawArrays = _tnl_DrawArrays;
   vfmt->DrawElements = _tnl_DrawElements;
   vfmt->DrawRangeElements = _tnl_DrawRangeElements;

   /* Setup vector pointers that will be used to bind arrays to VB's.
    */
   _mesa_vector4f_init( &tmp->Obj, 0, NULL);
   _mesa_vector4f_init( &tmp->Normal, 0, NULL);
   _mesa_vector4f_init( &tmp->FogCoord, 0, NULL);
   _mesa_vector4f_init( &tmp->Index, 0, NULL);

   for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
      _mesa_vector4f_init( &tmp->TexCoord[i], 0, NULL);
}


/**
 * Destroy the context's vertex array stuff.
 * Called during T 'n L context destruction.
 */
void _tnl_array_destroy( GLcontext *ctx )
{
   (void) ctx;
}
