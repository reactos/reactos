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

#include "main/glheader.h"
#include "main/context.h"
#include "main/state.h"
#include "main/api_validate.h"
#include "main/varray.h"
#include "main/bufferobj.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/transformfeedback.h"

#include "vbo_context.h"


/**
 * All vertex buffers should be in an unmapped state when we're about
 * to draw.  This debug function checks that.
 */
static void
check_buffers_are_unmapped(const struct gl_client_array **inputs)
{
#ifdef DEBUG
   GLuint i;

   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
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
   const GLboolean restart = ctx->Array.PrimitiveRestart;
   const GLuint restartIndex = ctx->Array.RestartIndex;
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
      if (restart) {
         for (i = 0; i < count; i++) {
            if (ui_indices[i] != restartIndex) {
               if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
               if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < count; i++) {
            if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
            if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
         }
      }
      *min_index = min_ui;
      *max_index = max_ui;
      break;
   }
   case GL_UNSIGNED_SHORT: {
      const GLushort *us_indices = (const GLushort *)indices;
      GLuint max_us = 0;
      GLuint min_us = ~0U;
      if (restart) {
         for (i = 0; i < count; i++) {
            if (us_indices[i] != restartIndex) {
               if (us_indices[i] > max_us) max_us = us_indices[i];
               if (us_indices[i] < min_us) min_us = us_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < count; i++) {
            if (us_indices[i] > max_us) max_us = us_indices[i];
            if (us_indices[i] < min_us) min_us = us_indices[i];
         }
      }
      *min_index = min_us;
      *max_index = max_us;
      break;
   }
   case GL_UNSIGNED_BYTE: {
      const GLubyte *ub_indices = (const GLubyte *)indices;
      GLuint max_ub = 0;
      GLuint min_ub = ~0U;
      if (restart) {
         for (i = 0; i < count; i++) {
            if (ub_indices[i] != restartIndex) {
               if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
               if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < count; i++) {
            if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
            if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
         }
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
 * Check that element 'j' of the array has reasonable data.
 * Map VBO if needed.
 * For debugging purposes; not normally used.
 */
static void
check_array_data(struct gl_context *ctx, struct gl_client_array *array,
                 GLuint attrib, GLuint j)
{
   if (array->Enabled) {
      const void *data = array->Ptr;
      if (_mesa_is_bufferobj(array->BufferObj)) {
         if (!array->BufferObj->Pointer) {
            /* need to map now */
            array->BufferObj->Pointer =
               ctx->Driver.MapBufferRange(ctx, 0, array->BufferObj->Size,
					  GL_MAP_READ_BIT, array->BufferObj);
         }
         data = ADD_POINTERS(data, array->BufferObj->Pointer);
      }
      switch (array->Type) {
      case GL_FLOAT:
         {
            GLfloat *f = (GLfloat *) ((GLubyte *) data + array->StrideB * j);
            GLint k;
            for (k = 0; k < array->Size; k++) {
               if (IS_INF_OR_NAN(f[k]) ||
                   f[k] >= 1.0e20 || f[k] <= -1.0e10) {
                  printf("Bad array data:\n");
                  printf("  Element[%u].%u = %f\n", j, k, f[k]);
                  printf("  Array %u at %p\n", attrib, (void* ) array);
                  printf("  Type 0x%x, Size %d, Stride %d\n",
			 array->Type, array->Size, array->Stride);
                  printf("  Address/offset %p in Buffer Object %u\n",
			 array->Ptr, array->BufferObj->Name);
                  f[k] = 1.0; /* XXX replace the bad value! */
               }
               /*assert(!IS_INF_OR_NAN(f[k]));*/
            }
         }
         break;
      default:
         ;
      }
   }
}


/**
 * Unmap the buffer object referenced by given array, if mapped.
 */
static void
unmap_array_buffer(struct gl_context *ctx, struct gl_client_array *array)
{
   if (array->Enabled &&
       _mesa_is_bufferobj(array->BufferObj) &&
       _mesa_bufferobj_mapped(array->BufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, array->BufferObj);
   }
}


/**
 * Examine the array's data for NaNs, etc.
 * For debug purposes; not normally used.
 */
static void
check_draw_elements_data(struct gl_context *ctx, GLsizei count, GLenum elemType,
                         const void *elements, GLint basevertex)
{
   struct gl_array_object *arrayObj = ctx->Array.ArrayObj;
   const void *elemMap;
   GLint i, k;

   if (_mesa_is_bufferobj(ctx->Array.ArrayObj->ElementArrayBufferObj)) {
      elemMap = ctx->Driver.MapBufferRange(ctx, 0,
					   ctx->Array.ArrayObj->ElementArrayBufferObj->Size,
					   GL_MAP_READ_BIT,
					   ctx->Array.ArrayObj->ElementArrayBufferObj);
      elements = ADD_POINTERS(elements, elemMap);
   }

   for (i = 0; i < count; i++) {
      GLuint j;

      /* j = element[i] */
      switch (elemType) {
      case GL_UNSIGNED_BYTE:
         j = ((const GLubyte *) elements)[i];
         break;
      case GL_UNSIGNED_SHORT:
         j = ((const GLushort *) elements)[i];
         break;
      case GL_UNSIGNED_INT:
         j = ((const GLuint *) elements)[i];
         break;
      default:
         assert(0);
      }

      /* check element j of each enabled array */
      for (k = 0; k < Elements(arrayObj->VertexAttrib); k++) {
         check_array_data(ctx, &arrayObj->VertexAttrib[k], k, j);
      }
   }

   if (_mesa_is_bufferobj(arrayObj->ElementArrayBufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, ctx->Array.ArrayObj->ElementArrayBufferObj);
   }

   for (k = 0; k < Elements(arrayObj->VertexAttrib); k++) {
      unmap_array_buffer(ctx, &arrayObj->VertexAttrib[k]);
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
   struct gl_array_object *arrayObj = ctx->Array.ArrayObj;
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
	     arrayObj->VertexAttrib[VERT_ATTRIB_FF(i)].Enabled,
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
   struct gl_client_array *vertexAttrib = ctx->Array.ArrayObj->VertexAttrib;
   const struct gl_client_array **inputs = &exec->array.inputs[0];
   GLbitfield64 const_inputs = 0x0;
   GLuint i;

   switch (get_program_mode(ctx)) {
   case VP_NONE:
      /* When no vertex program is active (or the vertex program is generated
       * from fixed-function state).  We put the material values into the
       * generic slots.  This is the only situation where material values
       * are available as per-vertex attributes.
       */
      for (i = 0; i < VERT_ATTRIB_FF_MAX; i++) {
	 if (vertexAttrib[VERT_ATTRIB_FF(i)].Enabled)
	    inputs[i] = &vertexAttrib[VERT_ATTRIB_FF(i)];
	 else {
	    inputs[i] = &vbo->legacy_currval[i];
            const_inputs |= VERT_BIT(i);
         }
      }

      for (i = 0; i < MAT_ATTRIB_MAX; i++) {
	 inputs[VERT_ATTRIB_GENERIC(i)] = &vbo->mat_currval[i];
         const_inputs |= VERT_BIT_GENERIC(i);
      }

      /* Could use just about anything, just to fill in the empty
       * slots:
       */
      for (i = MAT_ATTRIB_MAX; i < VERT_ATTRIB_GENERIC_MAX; i++) {
	 inputs[VERT_ATTRIB_GENERIC(i)] = &vbo->generic_currval[i];
         const_inputs |= VERT_BIT_GENERIC(i);
      }

      /* There is no need to make _NEW_ARRAY dirty here for the TnL program,
       * because it already takes care of invalidating the state necessary
       * to revalidate vertex arrays. Not marking the state as dirty also
       * improves performance (quite significantly in some apps).
       */
      if (!ctx->VertexProgram._MaintainTnlProgram)
         ctx->NewState |= _NEW_ARRAY;
      break;

   case VP_NV:
      /* NV_vertex_program - attribute arrays alias and override
       * conventional, legacy arrays.  No materials, and the generic
       * slots are vacant.
       */
      for (i = 0; i < VERT_ATTRIB_FF_MAX; i++) {
	 if (i < VERT_ATTRIB_GENERIC_MAX
             && vertexAttrib[VERT_ATTRIB_GENERIC(i)].Enabled)
	    inputs[i] = &vertexAttrib[VERT_ATTRIB_GENERIC(i)];
	 else if (vertexAttrib[VERT_ATTRIB_FF(i)].Enabled)
	    inputs[i] = &vertexAttrib[VERT_ATTRIB_FF(i)];
	 else {
	    inputs[i] = &vbo->legacy_currval[i];
            const_inputs |= VERT_BIT_FF(i);
         }
      }

      /* Could use just about anything, just to fill in the empty
       * slots:
       */
      for (i = 0; i < VERT_ATTRIB_GENERIC_MAX; i++) {
	 inputs[VERT_ATTRIB_GENERIC(i)] = &vbo->generic_currval[i];
         const_inputs |= VERT_BIT_GENERIC(i);
      }

      ctx->NewState |= _NEW_ARRAY;
      break;

   case VP_ARB:
      /* GL_ARB_vertex_program or GLSL vertex shader - Only the generic[0]
       * attribute array aliases and overrides the legacy position array.  
       *
       * Otherwise, legacy attributes available in the legacy slots,
       * generic attributes in the generic slots and materials are not
       * available as per-vertex attributes.
       */
      if (vertexAttrib[VERT_ATTRIB_GENERIC0].Enabled)
	 inputs[0] = &vertexAttrib[VERT_ATTRIB_GENERIC0];
      else if (vertexAttrib[VERT_ATTRIB_POS].Enabled)
	 inputs[0] = &vertexAttrib[VERT_ATTRIB_POS];
      else {
	 inputs[0] = &vbo->legacy_currval[0];
         const_inputs |= VERT_BIT_POS;
      }

      for (i = 1; i < VERT_ATTRIB_FF_MAX; i++) {
	 if (vertexAttrib[VERT_ATTRIB_FF(i)].Enabled)
	    inputs[i] = &vertexAttrib[VERT_ATTRIB_FF(i)];
	 else {
	    inputs[i] = &vbo->legacy_currval[i];
            const_inputs |= VERT_BIT_FF(i);
         }
      }

      for (i = 1; i < VERT_ATTRIB_GENERIC_MAX; i++) {
	 if (vertexAttrib[VERT_ATTRIB_GENERIC(i)].Enabled)
	    inputs[VERT_ATTRIB_GENERIC(i)] = &vertexAttrib[VERT_ATTRIB_GENERIC(i)];
	 else {
	    inputs[VERT_ATTRIB_GENERIC(i)] = &vbo->generic_currval[i];
            const_inputs |= VERT_BIT_GENERIC(i);
         }
      }

      inputs[VERT_ATTRIB_GENERIC0] = inputs[0];
      ctx->NewState |= _NEW_ARRAY;
      break;
   }

   _mesa_set_varying_vp_inputs( ctx, VERT_BIT_ALL & (~const_inputs) );
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

   /* Implement the primitive restart index */
   if (ctx->Array.PrimitiveRestart && ctx->Array.RestartIndex < count) {
      GLuint primCount = 0;

      if (ctx->Array.RestartIndex == start) {
         /* special case: RestartIndex at beginning */
         if (count > 1) {
            prim[0].start = start + 1;
            prim[0].count = count - 1;
            primCount = 1;
         }
      }
      else if (ctx->Array.RestartIndex == start + count - 1) {
         /* special case: RestartIndex at end */
         if (count > 1) {
            prim[0].start = start;
            prim[0].count = count - 1;
            primCount = 1;
         }
      }
      else {
         /* general case: RestartIndex in middle, split into two prims */
         prim[0].start = start;
         prim[0].count = ctx->Array.RestartIndex - start;

         prim[1] = prim[0];
         prim[1].start = ctx->Array.RestartIndex + 1;
         prim[1].count = count - prim[1].start;

         primCount = 2;
      }

      if (primCount > 0) {
         /* draw one or two prims */
         check_buffers_are_unmapped(exec->array.inputs);
         vbo->draw_prims(ctx, exec->array.inputs, prim, primCount, NULL,
                         GL_TRUE, start, start + count - 1, NULL);
      }
   }
   else {
      /* no prim restart */
      prim[0].start = start;
      prim[0].count = count;

      check_buffers_are_unmapped(exec->array.inputs);
      vbo->draw_prims(ctx, exec->array.inputs, prim, 1, NULL,
                      GL_TRUE, start, start + count - 1,
                      NULL);
   }
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
 * Called from glDrawArraysInstanced when in immediate mode (not
 * display list mode).
 */
static void GLAPIENTRY
vbo_exec_DrawArraysInstanced(GLenum mode, GLint start, GLsizei count,
                             GLsizei numInstances)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawArraysInstanced(%s, %d, %d, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), start, count, numInstances);

   if (!_mesa_validate_DrawArraysInstanced(ctx, mode, start, count, numInstances))
      return;

   FLUSH_CURRENT( ctx, 0 );

   if (!_mesa_valid_to_render(ctx, "glDrawArraysInstanced")) {
      return;
   }

   if (0)
      check_draw_arrays_data(ctx, start, count);

   vbo_draw_arrays(ctx, mode, start, count, numInstances);

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
				const GLvoid *indices,
				GLint basevertex, GLint numInstances)
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
   ib.obj = ctx->Array.ArrayObj->ElementArrayBufferObj;
   ib.ptr = indices;

   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].weak = 0;
   prim[0].pad = 0;
   prim[0].mode = mode;
   prim[0].start = 0;
   prim[0].count = count;
   prim[0].indexed = 1;
   prim[0].basevertex = basevertex;
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
		    index_bounds_valid, start, end, NULL );
}


/**
 * Called by glDrawRangeElementsBaseVertex() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawRangeElementsBaseVertex(GLenum mode,
				     GLuint start, GLuint end,
				     GLsizei count, GLenum type,
				     const GLvoid *indices,
				     GLint basevertex)
{
   static GLuint warnCount = 0;
   GLboolean index_bounds_valid = GL_TRUE;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx,
                "glDrawRangeElementsBaseVertex(%s, %u, %u, %d, %s, %p, %d)\n",
                _mesa_lookup_enum_by_nr(mode), start, end, count,
                _mesa_lookup_enum_by_nr(type), indices, basevertex);

   if (!_mesa_validate_DrawRangeElements( ctx, mode, start, end, count,
                                          type, indices, basevertex ))
      return;

   if ((int) end + basevertex < 0 ||
       start + basevertex >= ctx->Array.ArrayObj->_MaxElement) {
      /* The application requested we draw using a range of indices that's
       * outside the bounds of the current VBO.  This is invalid and appears
       * to give undefined results.  The safest thing to do is to simply
       * ignore the range, in case the application botched their range tracking
       * but did provide valid indices.  Also issue a warning indicating that
       * the application is broken.
       */
      if (warnCount++ < 10) {
         _mesa_warning(ctx, "glDrawRangeElements(start %u, end %u, "
                       "basevertex %d, count %d, type 0x%x, indices=%p):\n"
                       "\trange is outside VBO bounds (max=%u); ignoring.\n"
                       "\tThis should be fixed in the application.",
                       start, end, basevertex, count, type, indices,
                       ctx->Array.ArrayObj->_MaxElement - 1);
      }
      index_bounds_valid = GL_FALSE;
   }

   /* NOTE: It's important that 'end' is a reasonable value.
    * in _tnl_draw_prims(), we use end to determine how many vertices
    * to transform.  If it's too large, we can unnecessarily split prims
    * or we can read/write out of memory in several different places!
    */

   /* Catch/fix some potential user errors */
   if (type == GL_UNSIGNED_BYTE) {
      start = MIN2(start, 0xff);
      end = MIN2(end, 0xff);
   }
   else if (type == GL_UNSIGNED_SHORT) {
      start = MIN2(start, 0xffff);
      end = MIN2(end, 0xffff);
   }

   if (0) {
      printf("glDraw[Range]Elements{,BaseVertex}"
	     "(start %u, end %u, type 0x%x, count %d) ElemBuf %u, "
	     "base %d\n",
	     start, end, type, count,
	     ctx->Array.ArrayObj->ElementArrayBufferObj->Name,
	     basevertex);
   }

   if ((int) start + basevertex < 0 ||
       end + basevertex >= ctx->Array.ArrayObj->_MaxElement)
      index_bounds_valid = GL_FALSE;

#if 0
   check_draw_elements_data(ctx, count, type, indices);
#else
   (void) check_draw_elements_data;
#endif

   vbo_validated_drawrangeelements(ctx, mode, index_bounds_valid, start, end,
				   count, type, indices, basevertex, 1);
}


/**
 * Called by glDrawRangeElements() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawRangeElements(GLenum mode, GLuint start, GLuint end,
                           GLsizei count, GLenum type, const GLvoid *indices)
{
   if (MESA_VERBOSE & VERBOSE_DRAW) {
      GET_CURRENT_CONTEXT(ctx);
      _mesa_debug(ctx,
                  "glDrawRangeElements(%s, %u, %u, %d, %s, %p)\n",
                  _mesa_lookup_enum_by_nr(mode), start, end, count,
                  _mesa_lookup_enum_by_nr(type), indices);
   }

   vbo_exec_DrawRangeElementsBaseVertex(mode, start, end, count, type,
					indices, 0);
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

   if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices, 0 ))
      return;

   vbo_validated_drawrangeelements(ctx, mode, GL_FALSE, ~0, ~0,
				   count, type, indices, 0, 1);
}


/**
 * Called by glDrawElementsBaseVertex() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
				const GLvoid *indices, GLint basevertex)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawElementsBaseVertex(%s, %d, %s, %p, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), count,
                  _mesa_lookup_enum_by_nr(type), indices, basevertex);

   if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices,
				     basevertex ))
      return;

   vbo_validated_drawrangeelements(ctx, mode, GL_FALSE, ~0, ~0,
				   count, type, indices, basevertex, 1);
}


/**
 * Called by glDrawElementsInstanced() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type,
                               const GLvoid *indices, GLsizei numInstances)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawElementsInstanced(%s, %d, %s, %p, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), count,
                  _mesa_lookup_enum_by_nr(type), indices, numInstances);

   if (!_mesa_validate_DrawElementsInstanced(ctx, mode, count, type, indices,
                                             numInstances, 0))
      return;

   vbo_validated_drawrangeelements(ctx, mode, GL_FALSE, ~0, ~0,
				   count, type, indices, 0, numInstances);
}

/**
 * Called by glDrawElementsInstancedBaseVertex() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type,
                               const GLvoid *indices, GLsizei numInstances,
                               GLint basevertex)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawElementsInstancedBaseVertex(%s, %d, %s, %p, %d; %d)\n",
                  _mesa_lookup_enum_by_nr(mode), count,
                  _mesa_lookup_enum_by_nr(type), indices,
                  numInstances, basevertex);

   if (!_mesa_validate_DrawElementsInstanced(ctx, mode, count, type, indices,
                                             numInstances, basevertex))
      return;

   vbo_validated_drawrangeelements(ctx, mode, GL_FALSE, ~0, ~0,
				   count, type, indices, basevertex, numInstances);
}


/**
 * Inner support for both _mesa_MultiDrawElements() and
 * _mesa_MultiDrawRangeElements().
 * This does the actual rendering after we've checked array indexes, etc.
 */
static void
vbo_validated_multidrawelements(struct gl_context *ctx, GLenum mode,
				const GLsizei *count, GLenum type,
				const GLvoid **indices, GLsizei primcount,
				const GLint *basevertex)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_index_buffer ib;
   struct _mesa_prim *prim;
   unsigned int index_type_size = vbo_sizeof_ib_type(type);
   uintptr_t min_index_ptr, max_index_ptr;
   GLboolean fallback = GL_FALSE;
   int i;

   if (primcount == 0)
      return;

   FLUSH_CURRENT( ctx, 0 );

   if (!_mesa_valid_to_render(ctx, "glMultiDrawElements")) {
      return;
   }

   prim = calloc(1, primcount * sizeof(*prim));
   if (prim == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glMultiDrawElements");
      return;
   }

   /* Decide if we can do this all as one set of primitives sharing the
    * same index buffer, or if we have to reset the index pointer per
    * primitive.
    */
   vbo_bind_arrays( ctx );

   /* check for dirty state again */
   if (ctx->NewState)
      _mesa_update_state( ctx );

   min_index_ptr = (uintptr_t)indices[0];
   max_index_ptr = 0;
   for (i = 0; i < primcount; i++) {
      min_index_ptr = MIN2(min_index_ptr, (uintptr_t)indices[i]);
      max_index_ptr = MAX2(max_index_ptr, (uintptr_t)indices[i] +
			   index_type_size * count[i]);
   }

   /* Check if we can handle this thing as a bunch of index offsets from the
    * same index pointer.  If we can't, then we have to fall back to doing
    * a draw_prims per primitive.
    * Check that the difference between each prim's indexes is a multiple of
    * the index/element size.
    */
   if (index_type_size != 1) {
      for (i = 0; i < primcount; i++) {
	 if ((((uintptr_t)indices[i] - min_index_ptr) % index_type_size) != 0) {
	    fallback = GL_TRUE;
	    break;
	 }
      }
   }

   /* If the index buffer isn't in a VBO, then treating the application's
    * subranges of the index buffer as one large index buffer may lead to
    * us reading unmapped memory.
    */
   if (!_mesa_is_bufferobj(ctx->Array.ArrayObj->ElementArrayBufferObj))
      fallback = GL_TRUE;

   if (!fallback) {
      ib.count = (max_index_ptr - min_index_ptr) / index_type_size;
      ib.type = type;
      ib.obj = ctx->Array.ArrayObj->ElementArrayBufferObj;
      ib.ptr = (void *)min_index_ptr;

      for (i = 0; i < primcount; i++) {
	 prim[i].begin = (i == 0);
	 prim[i].end = (i == primcount - 1);
	 prim[i].weak = 0;
	 prim[i].pad = 0;
	 prim[i].mode = mode;
	 prim[i].start = ((uintptr_t)indices[i] - min_index_ptr) / index_type_size;
	 prim[i].count = count[i];
	 prim[i].indexed = 1;
         prim[i].num_instances = 1;
	 if (basevertex != NULL)
	    prim[i].basevertex = basevertex[i];
	 else
	    prim[i].basevertex = 0;
      }

      check_buffers_are_unmapped(exec->array.inputs);
      vbo->draw_prims(ctx, exec->array.inputs, prim, primcount, &ib,
		      GL_FALSE, ~0, ~0, NULL);
   } else {
      /* render one prim at a time */
      for (i = 0; i < primcount; i++) {
	 ib.count = count[i];
	 ib.type = type;
	 ib.obj = ctx->Array.ArrayObj->ElementArrayBufferObj;
	 ib.ptr = indices[i];

	 prim[0].begin = 1;
	 prim[0].end = 1;
	 prim[0].weak = 0;
	 prim[0].pad = 0;
	 prim[0].mode = mode;
	 prim[0].start = 0;
	 prim[0].count = count[i];
	 prim[0].indexed = 1;
         prim[0].num_instances = 1;
	 if (basevertex != NULL)
	    prim[0].basevertex = basevertex[i];
	 else
	    prim[0].basevertex = 0;

         check_buffers_are_unmapped(exec->array.inputs);
         vbo->draw_prims(ctx, exec->array.inputs, prim, 1, &ib,
                         GL_FALSE, ~0, ~0, NULL);
      }
   }

   free(prim);
}


static void GLAPIENTRY
vbo_exec_MultiDrawElements(GLenum mode,
			   const GLsizei *count, GLenum type,
			   const GLvoid **indices,
			   GLsizei primcount)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   for (i = 0; i < primcount; i++) {
      if (!_mesa_validate_DrawElements(ctx, mode, count[i], type, indices[i],
				       0))
	 return;
   }

   vbo_validated_multidrawelements(ctx, mode, count, type, indices, primcount,
				   NULL);
}


static void GLAPIENTRY
vbo_exec_MultiDrawElementsBaseVertex(GLenum mode,
				     const GLsizei *count, GLenum type,
				     const GLvoid **indices,
				     GLsizei primcount,
				     const GLsizei *basevertex)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   for (i = 0; i < primcount; i++) {
      if (!_mesa_validate_DrawElements(ctx, mode, count[i], type, indices[i],
				       basevertex[i]))
	 return;
   }

   vbo_validated_multidrawelements(ctx, mode, count, type, indices, primcount,
				   basevertex);
}

#if FEATURE_EXT_transform_feedback

static void
vbo_draw_transform_feedback(struct gl_context *ctx, GLenum mode,
                            struct gl_transform_feedback_object *obj,
                            GLuint numInstances)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_prim prim[2];

   vbo_bind_arrays(ctx);

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

   /* Maybe we should do some primitive splitting for primitive restart
    * (like in DrawArrays), but we have no way to know how many vertices
    * will be rendered. */

   check_buffers_are_unmapped(exec->array.inputs);
   vbo->draw_prims(ctx, exec->array.inputs, prim, 1, NULL,
                   GL_TRUE, 0, 0, obj);
}

/**
 * Like DrawArrays, but take the count from a transform feedback object.
 * \param mode  GL_POINTS, GL_LINES, GL_TRIANGLE_STRIP, etc.
 * \param name  the transform feedback object
 * User still has to setup of the vertex attribute info with
 * glVertexPointer, glColorPointer, etc.
 * Part of GL_ARB_transform_feedback2.
 */
static void GLAPIENTRY
vbo_exec_DrawTransformFeedback(GLenum mode, GLuint name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_transform_feedback_object *obj =
      _mesa_lookup_transform_feedback_object(ctx, name);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawTransformFeedback(%s, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), name);

   if (!_mesa_validate_DrawTransformFeedback(ctx, mode, obj)) {
      return;
   }

   FLUSH_CURRENT(ctx, 0);

   if (!_mesa_valid_to_render(ctx, "glDrawTransformFeedback")) {
      return;
   }

   vbo_draw_transform_feedback(ctx, mode, obj, 1);
}

#endif

/**
 * Plug in the immediate-mode vertex array drawing commands into the
 * givven vbo_exec_context object.
 */
void
vbo_exec_array_init( struct vbo_exec_context *exec )
{
   exec->vtxfmt.DrawArrays = vbo_exec_DrawArrays;
   exec->vtxfmt.DrawElements = vbo_exec_DrawElements;
   exec->vtxfmt.DrawRangeElements = vbo_exec_DrawRangeElements;
   exec->vtxfmt.MultiDrawElementsEXT = vbo_exec_MultiDrawElements;
   exec->vtxfmt.DrawElementsBaseVertex = vbo_exec_DrawElementsBaseVertex;
   exec->vtxfmt.DrawRangeElementsBaseVertex = vbo_exec_DrawRangeElementsBaseVertex;
   exec->vtxfmt.MultiDrawElementsBaseVertex = vbo_exec_MultiDrawElementsBaseVertex;
   exec->vtxfmt.DrawArraysInstanced = vbo_exec_DrawArraysInstanced;
   exec->vtxfmt.DrawElementsInstanced = vbo_exec_DrawElementsInstanced;
   exec->vtxfmt.DrawElementsInstancedBaseVertex = vbo_exec_DrawElementsInstancedBaseVertex;
#if FEATURE_EXT_transform_feedback
   exec->vtxfmt.DrawTransformFeedback = vbo_exec_DrawTransformFeedback;
#endif
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


void GLAPIENTRY
_mesa_DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
			     const GLvoid *indices, GLint basevertex)
{
   vbo_exec_DrawElementsBaseVertex(mode, count, type, indices, basevertex);
}


void GLAPIENTRY
_mesa_DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count,
                        GLenum type, const GLvoid *indices)
{
   vbo_exec_DrawRangeElements(mode, start, end, count, type, indices);
}


void GLAPIENTRY
_mesa_DrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end,
				  GLsizei count, GLenum type,
				  const GLvoid *indices, GLint basevertex)
{
   vbo_exec_DrawRangeElementsBaseVertex(mode, start, end, count, type,
					indices, basevertex);
}


void GLAPIENTRY
_mesa_MultiDrawElementsEXT(GLenum mode, const GLsizei *count, GLenum type,
			   const GLvoid **indices, GLsizei primcount)
{
   vbo_exec_MultiDrawElements(mode, count, type, indices, primcount);
}


void GLAPIENTRY
_mesa_MultiDrawElementsBaseVertex(GLenum mode,
				  const GLsizei *count, GLenum type,
				  const GLvoid **indices, GLsizei primcount,
				  const GLint *basevertex)
{
   vbo_exec_MultiDrawElementsBaseVertex(mode, count, type, indices,
					primcount, basevertex);
}

#if FEATURE_EXT_transform_feedback

void GLAPIENTRY
_mesa_DrawTransformFeedback(GLenum mode, GLuint name)
{
   vbo_exec_DrawTransformFeedback(mode, name);
}

#endif
