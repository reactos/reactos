/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
#include "main/api_noop.h"

#include "vbo_context.h"

/* Compute min and max elements for drawelements calls.
 */
static void get_minmax_index( GLuint count, GLuint type, 
			      const GLvoid *indices,
			      GLuint *min_index,
			      GLuint *max_index)
{
   GLuint i;

   switch(type) {
   case GL_UNSIGNED_INT: {
      const GLuint *ui_indices = (const GLuint *)indices;
      GLuint max_ui = ui_indices[count-1];
      GLuint min_ui = ui_indices[0];
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
      GLuint max_us = us_indices[count-1];
      GLuint min_us = us_indices[0];
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
      GLuint max_ub = ub_indices[count-1];
      GLuint min_ub = ub_indices[0];
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
}


/* Just translate the arrayobj into a sane layout.
 */
static void bind_array_obj( GLcontext *ctx )
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   GLuint i;

   /* TODO: Fix the ArrayObj struct to keep legacy arrays in an array
    * rather than as individual named arrays.  Then this function can
    * go away.
    */
   exec->array.legacy_array[VERT_ATTRIB_POS] = &ctx->Array.ArrayObj->Vertex;
   exec->array.legacy_array[VERT_ATTRIB_WEIGHT] = &vbo->legacy_currval[VERT_ATTRIB_WEIGHT];
   exec->array.legacy_array[VERT_ATTRIB_NORMAL] = &ctx->Array.ArrayObj->Normal;
   exec->array.legacy_array[VERT_ATTRIB_COLOR0] = &ctx->Array.ArrayObj->Color;
   exec->array.legacy_array[VERT_ATTRIB_COLOR1] = &ctx->Array.ArrayObj->SecondaryColor;
   exec->array.legacy_array[VERT_ATTRIB_FOG] = &ctx->Array.ArrayObj->FogCoord;
   exec->array.legacy_array[VERT_ATTRIB_COLOR_INDEX] = &ctx->Array.ArrayObj->Index;
   if (ctx->Array.ArrayObj->PointSize.Enabled) {
      /* this aliases COLOR_INDEX */
      exec->array.legacy_array[VERT_ATTRIB_POINT_SIZE] = &ctx->Array.ArrayObj->PointSize;
   }
   exec->array.legacy_array[VERT_ATTRIB_EDGEFLAG] = &ctx->Array.ArrayObj->EdgeFlag;

   for (i = 0; i < 8; i++)
      exec->array.legacy_array[VERT_ATTRIB_TEX0 + i] = &ctx->Array.ArrayObj->TexCoord[i];

   for (i = 0; i < VERT_ATTRIB_MAX; i++)
      exec->array.generic_array[i] = &ctx->Array.ArrayObj->VertexAttrib[i];
   
   exec->array.array_obj = ctx->Array.ArrayObj->Name;
}

static void recalculate_input_bindings( GLcontext *ctx )
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   const struct gl_client_array **inputs = &exec->array.inputs[0];
   GLuint i;

   exec->array.program_mode = get_program_mode(ctx);
   exec->array.enabled_flags = ctx->Array.ArrayObj->_Enabled;

   switch (exec->array.program_mode) {
   case VP_NONE:
      /* When no vertex program is active, we put the material values
       * into the generic slots.  This is the only situation where
       * material values are available as per-vertex attributes.
       */
      for (i = 0; i <= VERT_ATTRIB_TEX7; i++) {
	 if (exec->array.legacy_array[i]->Enabled)
	    inputs[i] = exec->array.legacy_array[i];
	 else
	    inputs[i] = &vbo->legacy_currval[i];
      }

      for (i = 0; i < MAT_ATTRIB_MAX; i++) {
	 inputs[VERT_ATTRIB_GENERIC0 + i] = &vbo->mat_currval[i];
      }

      /* Could use just about anything, just to fill in the empty
       * slots:
       */
      for (i = MAT_ATTRIB_MAX; i < VERT_ATTRIB_MAX - VERT_ATTRIB_GENERIC0; i++)
	 inputs[VERT_ATTRIB_GENERIC0 + i] = &vbo->generic_currval[i];

      break;
   case VP_NV:
      /* NV_vertex_program - attribute arrays alias and override
       * conventional, legacy arrays.  No materials, and the generic
       * slots are vacant.
       */
      for (i = 0; i <= VERT_ATTRIB_TEX7; i++) {
	 if (exec->array.generic_array[i]->Enabled)
	    inputs[i] = exec->array.generic_array[i];
	 else if (exec->array.legacy_array[i]->Enabled)
	    inputs[i] = exec->array.legacy_array[i];
	 else
	    inputs[i] = &vbo->legacy_currval[i];
      }

      /* Could use just about anything, just to fill in the empty
       * slots:
       */
      for (i = VERT_ATTRIB_GENERIC0; i < VERT_ATTRIB_MAX; i++)
	 inputs[i] = &vbo->generic_currval[i - VERT_ATTRIB_GENERIC0];

      break;
   case VP_ARB:
      /* ARB_vertex_program - Only the attribute zero (position) array
       * aliases and overrides the legacy position array.  
       *
       * Otherwise, legacy attributes available in the legacy slots,
       * generic attributes in the generic slots and materials are not
       * available as per-vertex attributes.
       */
      if (exec->array.generic_array[0]->Enabled)
	 inputs[0] = exec->array.generic_array[0];
      else if (exec->array.legacy_array[0]->Enabled)
	 inputs[0] = exec->array.legacy_array[0];
      else
	 inputs[0] = &vbo->legacy_currval[0];


      for (i = 1; i <= VERT_ATTRIB_TEX7; i++) {
	 if (exec->array.legacy_array[i]->Enabled)
	    inputs[i] = exec->array.legacy_array[i];
	 else
	    inputs[i] = &vbo->legacy_currval[i];
      }

      for (i = 0; i < 16; i++) {
	 if (exec->array.generic_array[i]->Enabled)
	    inputs[VERT_ATTRIB_GENERIC0 + i] = exec->array.generic_array[i];
	 else
	    inputs[VERT_ATTRIB_GENERIC0 + i] = &vbo->generic_currval[i];
      }
      break;
   }
}

static void bind_arrays( GLcontext *ctx )
{
#if 0
   if (ctx->Array.ArrayObj.Name != exec->array.array_obj) {
      bind_array_obj(ctx);
      recalculate_input_bindings(ctx);
   }
   else if (exec->array.program_mode != get_program_mode(ctx) ||
	    exec->array.enabled_flags != ctx->Array.ArrayObj->_Enabled) {
      
      recalculate_input_bindings(ctx);
   }
#else
   bind_array_obj(ctx);
   recalculate_input_bindings(ctx);
#endif
}



/***********************************************************************
 * API functions.
 */

static void GLAPIENTRY
vbo_exec_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_prim prim[1];

   if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
      return;

   FLUSH_CURRENT( ctx, 0 );

   if (ctx->NewState)
      _mesa_update_state( ctx );
      
   if (!vbo_validate_shaders(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawArrays(bad shader)");
      return;
   }

   bind_arrays( ctx );

   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].weak = 0;
   prim[0].pad = 0;
   prim[0].mode = mode;
   prim[0].start = start;
   prim[0].count = count;
   prim[0].indexed = 0;

   vbo->draw_prims( ctx, exec->array.inputs, prim, 1, NULL, start, start + count - 1 );
}



static void GLAPIENTRY
vbo_exec_DrawRangeElements(GLenum mode,
			   GLuint start, GLuint end,
			   GLsizei count, GLenum type, const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_index_buffer ib;
   struct _mesa_prim prim[1];

   if (!_mesa_validate_DrawRangeElements( ctx, mode, start, end, count, type, indices ))
      return;

   FLUSH_CURRENT( ctx, 0 );

   if (ctx->NewState)
      _mesa_update_state( ctx );

   if (!vbo_validate_shaders(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawRangeElements(bad shader)");
      return;
   }

   bind_arrays( ctx );

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

   vbo->draw_prims( ctx, exec->array.inputs, prim, 1, &ib, start, end );
}

static void GLAPIENTRY
vbo_exec_DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint min_index = 0;
   GLuint max_index = 0;

   if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices ))
      return;

   if (!vbo_validate_shaders(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawElements(bad shader)");
      return;
   }

   if (ctx->Array.ElementArrayBufferObj->Name) {
      const GLvoid *map = ctx->Driver.MapBuffer(ctx,
						 GL_ELEMENT_ARRAY_BUFFER_ARB,
						 GL_READ_ONLY,
						 ctx->Array.ElementArrayBufferObj);

      get_minmax_index(count, type, ADD_POINTERS(map, indices), &min_index, &max_index);

      ctx->Driver.UnmapBuffer(ctx,
			      GL_ELEMENT_ARRAY_BUFFER_ARB,
			      ctx->Array.ElementArrayBufferObj);
   }
   else {
      get_minmax_index(count, type, indices, &min_index, &max_index);
   }

   vbo_exec_DrawRangeElements(mode, min_index, max_index, count, type, indices);
}


/***********************************************************************
 * Initialization
 */




void vbo_exec_array_init( struct vbo_exec_context *exec )
{
#if 1
   exec->vtxfmt.DrawArrays = vbo_exec_DrawArrays;
   exec->vtxfmt.DrawElements = vbo_exec_DrawElements;
   exec->vtxfmt.DrawRangeElements = vbo_exec_DrawRangeElements;
#else
   exec->vtxfmt.DrawArrays = _mesa_noop_DrawArrays;
   exec->vtxfmt.DrawElements = _mesa_noop_DrawElements;
   exec->vtxfmt.DrawRangeElements = _mesa_noop_DrawRangeElements;
#endif
}


void vbo_exec_array_destroy( struct vbo_exec_context *exec )
{
   /* nothing to do */
}
