/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include <precomp.h>

/**
 * All vertex buffers should be in an unmapped state when we're about
 * to draw.  This debug function checks that.
 */
static void
check_buffers_are_unmapped(const struct gl_client_array **inputs)
{
#ifdef DEBUG
   GLuint i;

   for (i = 0; i < VBO_ATTRIB_MAX; i++) {
      if (inputs[i]) {
         struct gl_buffer_object *obj = inputs[i]->BufferObj;
         assert(!_mesa_bufferobj_mapped(obj));
         (void) obj;
      }
   }
#endif
}


/**
 * A debug function that may be called from other parts of Mesa as
 * needed during debugging.
 */
void
vbo_check_buffers_are_unmapped(struct gl_context *ctx)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   /* check the current vertex arrays */
   check_buffers_are_unmapped(exec->array.inputs);
   /* check the current glBegin/glVertex/glEnd-style VBO */
   assert(!_mesa_bufferobj_mapped(exec->vtx.bufferobj));
}

int
vbo_sizeof_ib_type(GLenum type)
{
   switch (type) {
   case GL_UNSIGNED_INT:
      return sizeof(GLuint);
   case GL_UNSIGNED_SHORT:
      return sizeof(GLushort);
   case GL_UNSIGNED_BYTE:
      return sizeof(GLubyte);
   default:
      assert(!"unsupported index data type");
      /* In case assert is turned off */
      return 0;
   }
}


/**
 * Compute min and max elements by scanning the index buffer for
 * glDraw[Range]Elements() calls.
 * If primitive restart is enabled, we need to ignore restart
 * indexes when computing min/max.
 */
void
vbo_get_minmax_index(struct gl_context *ctx,
		     const struct _mesa_prim *prim,
		     const struct _mesa_index_buffer *ib,
		     GLuint *min_index, GLuint *max_index)
{
   const GLuint count = prim->count;
   const void *indices;
   GLuint i;

   if (_mesa_is_bufferobj(ib->obj)) {
      indices = ctx->Driver.MapBufferRange(ctx, (GLsizeiptr) ib->ptr,
                                           count * vbo_sizeof_ib_type(ib->type),
					   GL_MAP_READ_BIT, ib->obj);
   } else {
      indices = ib->ptr;
   }

   switch (ib->type) {
   case GL_UNSIGNED_INT: {
      const GLuint *ui_indices = (const GLuint *)indices;
      GLuint max_ui = 0;
      GLuint min_ui = ~0U;
      for (i = 0; i < count; i++) {
         if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
         if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
      }
      *min_index = min_ui;
      *max_index = max_ui;
      break;
   }
   case GL_UNSIGNED_SHORT: {
      const GLushort *us_indices = (const GLushort *)indices;
      GLuint max_us = 0;
      GLuint min_us = ~0U;
      for (i = 0; i < count; i++) {
         if (us_indices[i] > max_us) max_us = us_indices[i];
         if (us_indices[i] < min_us) min_us = us_indices[i];
      }
      *min_index = min_us;
      *max_index = max_us;
      break;
   }
   case GL_UNSIGNED_BYTE: {
      const GLubyte *ub_indices = (const GLubyte *)indices;
      GLuint max_ub = 0;
      GLuint min_ub = ~0U;
      for (i = 0; i < count; i++) {
         if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
         if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
      }
      *min_index = min_ub;
      *max_index = max_ub;
      break;
   }
   default:
      assert(0);
      break;
   }

   if (_mesa_is_bufferobj(ib->obj)) {
      ctx->Driver.UnmapBuffer(ctx, ib->obj);
   }
}


/**
 * Check array data, looking for NaNs, etc.
 */
static void
check_draw_arrays_data(struct gl_context *ctx, GLint start, GLsizei count)
{
   /* TO DO */
}


/**
 * Print info/data for glDrawArrays(), for debugging.
 */
static void
print_draw_arrays(struct gl_context *ctx,
                  GLenum mode, GLint start, GLsizei count)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   int i;

   printf("vbo_exec_DrawArrays(mode 0x%x, start %d, count %d):\n",
	  mode, start, count);

   for (i = 0; i < 32; i++) {
      struct gl_buffer_object *bufObj = exec->array.inputs[i]->BufferObj;
      GLuint bufName = bufObj->Name;
      GLint stride = exec->array.inputs[i]->Stride;
      printf("attr %2d: size %d stride %d  enabled %d  "
	     "ptr %p  Bufobj %u\n",
	     i,
	     exec->array.inputs[i]->Size,
	     stride,
	     /*exec->array.inputs[i]->Enabled,*/
	     ctx->Array.VertexAttrib[VERT_ATTRIB(i)].Enabled,
	     exec->array.inputs[i]->Ptr,
	     bufName);

      if (bufName) {
         GLubyte *p = ctx->Driver.MapBufferRange(ctx, 0, bufObj->Size,
						 GL_MAP_READ_BIT, bufObj);
         int offset = (int) (GLintptr) exec->array.inputs[i]->Ptr;
         float *f = (float *) (p + offset);
         int *k = (int *) f;
         int i;
         int n = (count * stride) / 4;
         if (n > 32)
            n = 32;
         printf("  Data at offset %d:\n", offset);
         for (i = 0; i < n; i++) {
            printf("    float[%d] = 0x%08x %f\n", i, k[i], f[i]);
         }
         ctx->Driver.UnmapBuffer(ctx, bufObj);
      }
   }
}


/**
 * Set the vbo->exec->inputs[] pointers to point to the enabled
 * vertex arrays.  This depends on the current vertex program/shader
 * being executed because of whether or not generic vertex arrays
 * alias the conventional vertex arrays.
 * For arrays that aren't enabled, we set the input[attrib] pointer
 * to point at a zero-stride current value "array".
 */
static void
recalculate_input_bindings(struct gl_context *ctx)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct gl_client_array *vertexAttrib = ctx->Array.VertexAttrib;
   const struct gl_client_array **inputs = &exec->array.inputs[0];
   GLbitfield64 const_inputs = 0x0;
   GLuint i;

   for (i = 0; i < VBO_ATTRIB_MAX; i++) {
      if ((i < VERT_ATTRIB_MAX) && (vertexAttrib[VERT_ATTRIB(i)].Enabled))
         inputs[i] = &vertexAttrib[VERT_ATTRIB(i)];
      else {
         inputs[i] = &vbo->currval[i];
         const_inputs |= VERT_BIT(i);
      }
   }

   ctx->NewState |= _NEW_ARRAY;
}


/**
 * Examine the enabled vertex arrays to set the exec->array.inputs[] values.
 * These will point to the arrays to actually use for drawing.  Some will
 * be user-provided arrays, other will be zero-stride const-valued arrays.
 * Note that this might set the _NEW_ARRAY dirty flag so state validation
 * must be done after this call.
 */
void
vbo_bind_arrays(struct gl_context *ctx)
{
   if (!ctx->Array.RebindArrays) {
      return;
   }

   recalculate_input_bindings(ctx);
   ctx->Array.RebindArrays = GL_FALSE;
}


/**
 * Helper function called by the other DrawArrays() functions below.
 * This is where we handle primitive restart for drawing non-indexed
 * arrays.  If primitive restart is enabled, it typically means
 * splitting one DrawArrays() into two.
 */
static void
vbo_draw_arrays(struct gl_context *ctx, GLenum mode, GLint start,
                GLsizei count, GLuint numInstances)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_prim prim[2];

   vbo_bind_arrays(ctx);

   vbo_draw_method(exec, DRAW_ARRAYS);

   /* Again... because we may have changed the bitmask of per-vertex varying
    * attributes.  If we regenerate the fixed-function vertex program now
    * we may be able to prune down the number of vertex attributes which we
    * need in the shader.
    */
   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* init most fields to zero */
   memset(prim, 0, sizeof(prim));
   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].mode = mode;
   prim[0].num_instances = numInstances;

   /* no prim restart */
   prim[0].start = start;
   prim[0].count = count;

   check_buffers_are_unmapped(exec->array.inputs);
   vbo->draw_prims(ctx, exec->array.inputs, prim, 1, NULL,
                   GL_TRUE, start, start + count - 1);
}



/**
 * Called from glDrawArrays when in immediate mode (not display list mode).
 */
static void GLAPIENTRY
vbo_exec_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawArrays(%s, %d, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), start, count);

   if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
      return;

   FLUSH_CURRENT( ctx, 0 );

   if (!_mesa_valid_to_render(ctx, "glDrawArrays")) {
      return;
   }

   if (0)
      check_draw_arrays_data(ctx, start, count);

   vbo_draw_arrays(ctx, mode, start, count, 1);

   if (0)
      print_draw_arrays(ctx, mode, start, count);
}


/**
 * Map GL_ELEMENT_ARRAY_BUFFER and print contents.
 * For debugging.
 */
#if 0
static void
dump_element_buffer(struct gl_context *ctx, GLenum type)
{
   const GLvoid *map =
      ctx->Driver.MapBufferRange(ctx, 0,
				 ctx->Array.ArrayObj->ElementArrayBufferObj->Size,
				 GL_MAP_READ_BIT,
				 ctx->Array.ArrayObj->ElementArrayBufferObj);
   switch (type) {
   case GL_UNSIGNED_BYTE:
      {
         const GLubyte *us = (const GLubyte *) map;
         GLint i;
         for (i = 0; i < ctx->Array.ArrayObj->ElementArrayBufferObj->Size; i++) {
            printf("%02x ", us[i]);
            if (i % 32 == 31)
               printf("\n");
         }
         printf("\n");
      }
      break;
   case GL_UNSIGNED_SHORT:
      {
         const GLushort *us = (const GLushort *) map;
         GLint i;
         for (i = 0; i < ctx->Array.ArrayObj->ElementArrayBufferObj->Size / 2; i++) {
            printf("%04x ", us[i]);
            if (i % 16 == 15)
               printf("\n");
         }
         printf("\n");
      }
      break;
   case GL_UNSIGNED_INT:
      {
         const GLuint *us = (const GLuint *) map;
         GLint i;
         for (i = 0; i < ctx->Array.ArrayObj->ElementArrayBufferObj->Size / 4; i++) {
            printf("%08x ", us[i]);
            if (i % 8 == 7)
               printf("\n");
         }
         printf("\n");
      }
      break;
   default:
      ;
   }

   ctx->Driver.UnmapBuffer(ctx, ctx->Array.ArrayObj->ElementArrayBufferObj);
}
#endif


/**
 * Inner support for both _mesa_DrawElements and _mesa_DrawRangeElements.
 * Do the rendering for a glDrawElements or glDrawRangeElements call after
 * we've validated buffer bounds, etc.
 */
static void
vbo_validated_drawrangeelements(struct gl_context *ctx, GLenum mode,
				GLboolean index_bounds_valid,
				GLuint start, GLuint end,
				GLsizei count, GLenum type,
				const GLvoid *indices, GLint numInstances)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_index_buffer ib;
   struct _mesa_prim prim[1];

   FLUSH_CURRENT( ctx, 0 );

   if (!_mesa_valid_to_render(ctx, "glDraw[Range]Elements")) {
      return;
   }

   vbo_bind_arrays( ctx );

   vbo_draw_method(exec, DRAW_ARRAYS);

   /* check for dirty state again */
   if (ctx->NewState)
      _mesa_update_state( ctx );

   ib.count = count;
   ib.type = type;
   ib.obj = ctx->Array.ElementArrayBufferObj;
   ib.ptr = indices;

   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].weak = 0;
   prim[0].pad = 0;
   prim[0].mode = mode;
   prim[0].start = 0;
   prim[0].count = count;
   prim[0].indexed = 1;
   prim[0].num_instances = numInstances;

   /* Need to give special consideration to rendering a range of
    * indices starting somewhere above zero.  Typically the
    * application is issuing multiple DrawRangeElements() to draw
    * successive primitives layed out linearly in the vertex arrays.
    * Unless the vertex arrays are all in a VBO (or locked as with
    * CVA), the OpenGL semantics imply that we need to re-read or
    * re-upload the vertex data on each draw call.  
    *
    * In the case of hardware tnl, we want to avoid starting the
    * upload at zero, as it will mean every draw call uploads an
    * increasing amount of not-used vertex data.  Worse - in the
    * software tnl module, all those vertices might be transformed and
    * lit but never rendered.
    *
    * If we just upload or transform the vertices in start..end,
    * however, the indices will be incorrect.
    *
    * At this level, we don't know exactly what the requirements of
    * the backend are going to be, though it will likely boil down to
    * either:
    *
    * 1) Do nothing, everything is in a VBO and is processed once
    *       only.
    *
    * 2) Adjust the indices and vertex arrays so that start becomes
    *    zero.
    *
    * Rather than doing anything here, I'll provide a helper function
    * for the latter case elsewhere.
    */

   check_buffers_are_unmapped(exec->array.inputs);
   vbo->draw_prims( ctx, exec->array.inputs, prim, 1, &ib,
		    index_bounds_valid, start, end );
}


/**
 * Called by glDrawElements() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawElements(GLenum mode, GLsizei count, GLenum type,
                      const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawElements(%s, %u, %s, %p)\n",
                  _mesa_lookup_enum_by_nr(mode), count,
                  _mesa_lookup_enum_by_nr(type), indices);

   if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices))
      return;

   vbo_validated_drawrangeelements(ctx, mode, GL_FALSE, ~0, ~0,
				   count, type, indices, 1);
}


/**
 * Plug in the immediate-mode vertex array drawing commands into the
 * givven vbo_exec_context object.
 */
void
vbo_exec_array_init( struct vbo_exec_context *exec )
{
   exec->vtxfmt.DrawArrays = vbo_exec_DrawArrays;
   exec->vtxfmt.DrawElements = vbo_exec_DrawElements;
}


void
vbo_exec_array_destroy( struct vbo_exec_context *exec )
{
   /* nothing to do */
}



/**
 * The following functions are only used for OpenGL ES 1/2 support.
 * And some aren't even supported (yet) in ES 1/2.
 */


void GLAPIENTRY
_mesa_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
   vbo_exec_DrawArrays(mode, first, count);
}


void GLAPIENTRY
_mesa_DrawElements(GLenum mode, GLsizei count, GLenum type,
                   const GLvoid *indices)
{
   vbo_exec_DrawElements(mode, count, type, indices);
}

