/*
 * Mesa 3-D graphics library
 * Version:  7.2
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

#include <precomp.h>

#if FEATURE_beginend


static void
vbo_exec_debug_verts( struct vbo_exec_context *exec )
{
   GLuint count = exec->vtx.vert_count;
   GLuint i;

   printf("%s: %u vertices %d primitives, %d vertsize\n",
	  __FUNCTION__,
	  count,
	  exec->vtx.prim_count,
	  exec->vtx.vertex_size);

   for (i = 0 ; i < exec->vtx.prim_count ; i++) {
      struct _mesa_prim *prim = &exec->vtx.prim[i];
      printf("   prim %d: %s%s %d..%d %s %s\n",
	     i, 
	     _mesa_lookup_prim_by_nr(prim->mode),
	     prim->weak ? " (weak)" : "",
	     prim->start, 
	     prim->start + prim->count,
	     prim->begin ? "BEGIN" : "(wrap)",
	     prim->end ? "END" : "(wrap)");
   }
}


/*
 * NOTE: Need to have calculated primitives by this point -- do it on the fly.
 * NOTE: Old 'parity' issue is gone.
 */
static GLuint
vbo_copy_vertices( struct vbo_exec_context *exec )
{
   GLuint nr = exec->vtx.prim[exec->vtx.prim_count-1].count;
   GLuint ovf, i;
   GLuint sz = exec->vtx.vertex_size;
   GLfloat *dst = exec->vtx.copied.buffer;
   const GLfloat *src = (exec->vtx.buffer_map + 
                         exec->vtx.prim[exec->vtx.prim_count-1].start * 
                         exec->vtx.vertex_size);


   switch (exec->ctx->Driver.CurrentExecPrimitive) {
   case GL_POINTS:
      return 0;
   case GL_LINES:
      ovf = nr&1;
      for (i = 0 ; i < ovf ; i++)
	 memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case GL_TRIANGLES:
      ovf = nr%3;
      for (i = 0 ; i < ovf ; i++)
	 memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case GL_QUADS:
      ovf = nr&3;
      for (i = 0 ; i < ovf ; i++)
	 memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case GL_LINE_STRIP:
      if (nr == 0) {
	 return 0;
      }
      else {
	 memcpy( dst, src+(nr-1)*sz, sz * sizeof(GLfloat) );
	 return 1;
      }
   case GL_LINE_LOOP:
   case GL_TRIANGLE_FAN:
   case GL_POLYGON:
      if (nr == 0) {
	 return 0;
      }
      else if (nr == 1) {
	 memcpy( dst, src+0, sz * sizeof(GLfloat) );
	 return 1;
      }
      else {
	 memcpy( dst, src+0, sz * sizeof(GLfloat) );
	 memcpy( dst+sz, src+(nr-1)*sz, sz * sizeof(GLfloat) );
	 return 2;
      }
   case GL_TRIANGLE_STRIP:
      /* no parity issue, but need to make sure the tri is not drawn twice */
      if (nr & 1) {
	 exec->vtx.prim[exec->vtx.prim_count-1].count--;
      }
      /* fallthrough */
   case GL_QUAD_STRIP:
      switch (nr) {
      case 0:
         ovf = 0;
         break;
      case 1:
         ovf = 1;
         break;
      default:
         ovf = 2 + (nr & 1);
         break;
      }
      for (i = 0 ; i < ovf ; i++)
	 memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case PRIM_OUTSIDE_BEGIN_END:
      return 0;
   default:
      assert(0);
      return 0;
   }
}



/* TODO: populate these as the vertex is defined:
 */
static void
vbo_exec_bind_arrays( struct gl_context *ctx )
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct gl_client_array *arrays = exec->vtx.arrays;
   const GLuint count = exec->vtx.vert_count;
   GLuint attr;
   GLbitfield64 varying_inputs = 0x0;

   /* Install the default (ie Current) attributes first, then overlay
    * all active ones.
    */
   for (attr = 0; attr < VBO_ATTRIB_MAX; attr++) {
      exec->vtx.inputs[attr] = &vbo->legacy_currval[attr];
   }

   /* Make all active attributes (including edgeflag) available as
    * arrays of floats.
    */
   for (attr = 0; attr < VBO_ATTRIB_MAX ; attr++) {

      if (exec->vtx.attrsz[attr]) {
	 GLsizeiptr offset = (GLbyte *)exec->vtx.attrptr[attr] -
	    (GLbyte *)exec->vtx.vertex;

         /* override the default array set above */
         ASSERT(attr < Elements(exec->vtx.inputs));
         ASSERT(attr < Elements(exec->vtx.arrays)); /* arrays[] */
         exec->vtx.inputs[attr] = &arrays[attr];

         if (_mesa_is_bufferobj(exec->vtx.bufferobj)) {
            /* a real buffer obj: Ptr is an offset, not a pointer*/
            assert(exec->vtx.bufferobj->Pointer);  /* buf should be mapped */
            assert(offset >= 0);
            arrays[attr].Ptr = (GLubyte *)exec->vtx.bufferobj->Offset + offset;
         }
         else {
            /* Ptr into ordinary app memory */
            arrays[attr].Ptr = (GLubyte *)exec->vtx.buffer_map + offset;
         }
	 arrays[attr].Size = exec->vtx.attrsz[attr];
	 arrays[attr].StrideB = exec->vtx.vertex_size * sizeof(GLfloat);
	 arrays[attr].Stride = exec->vtx.vertex_size * sizeof(GLfloat);
	 arrays[attr].Type = GL_FLOAT;
	 arrays[attr].Enabled = 1;
         arrays[attr]._ElementSize = arrays[attr].Size * sizeof(GLfloat);
         _mesa_reference_buffer_object(ctx,
                                       &arrays[attr].BufferObj,
                                       exec->vtx.bufferobj);
	 arrays[attr]._MaxElement = count; /* ??? */

         varying_inputs |= VERT_BIT(attr);
         ctx->NewState |= _NEW_ARRAY;
      }
   }
}


/**
 * Unmap the VBO.  This is called before drawing.
 */
static void
vbo_exec_vtx_unmap( struct vbo_exec_context *exec )
{
   if (_mesa_is_bufferobj(exec->vtx.bufferobj)) {
      struct gl_context *ctx = exec->ctx;
      
      if (ctx->Driver.FlushMappedBufferRange) {
         GLintptr offset = exec->vtx.buffer_used - exec->vtx.bufferobj->Offset;
         GLsizeiptr length = (exec->vtx.buffer_ptr - exec->vtx.buffer_map) * sizeof(float);

         if (length)
            ctx->Driver.FlushMappedBufferRange(ctx, offset, length,
                                               exec->vtx.bufferobj);
      }

      exec->vtx.buffer_used += (exec->vtx.buffer_ptr -
                                exec->vtx.buffer_map) * sizeof(float);

      assert(exec->vtx.buffer_used <= VBO_VERT_BUFFER_SIZE);
      assert(exec->vtx.buffer_ptr != NULL);
      
      ctx->Driver.UnmapBuffer(ctx, exec->vtx.bufferobj);
      exec->vtx.buffer_map = NULL;
      exec->vtx.buffer_ptr = NULL;
      exec->vtx.max_vert = 0;
   }
}


/**
 * Map the vertex buffer to begin storing glVertex, glColor, etc data.
 */
void
vbo_exec_vtx_map( struct vbo_exec_context *exec )
{
   struct gl_context *ctx = exec->ctx;
   const GLenum accessRange = GL_MAP_WRITE_BIT |  /* for MapBufferRange */
                              GL_MAP_INVALIDATE_RANGE_BIT |
                              GL_MAP_UNSYNCHRONIZED_BIT |
                              GL_MAP_FLUSH_EXPLICIT_BIT |
                              MESA_MAP_NOWAIT_BIT;
   const GLenum usage = GL_STREAM_DRAW_ARB;
   
   if (!_mesa_is_bufferobj(exec->vtx.bufferobj))
      return;

   assert(!exec->vtx.buffer_map);
   assert(!exec->vtx.buffer_ptr);

   if (VBO_VERT_BUFFER_SIZE > exec->vtx.buffer_used + 1024) {
      /* The VBO exists and there's room for more */
      if (exec->vtx.bufferobj->Size > 0) {
         exec->vtx.buffer_map =
            (GLfloat *)ctx->Driver.MapBufferRange(ctx, 
                                                  exec->vtx.buffer_used,
                                                  (VBO_VERT_BUFFER_SIZE - 
                                                   exec->vtx.buffer_used),
                                                  accessRange,
                                                  exec->vtx.bufferobj);
         exec->vtx.buffer_ptr = exec->vtx.buffer_map;
      }
      else {
         exec->vtx.buffer_ptr = exec->vtx.buffer_map = NULL;
      }
   }
   
   if (!exec->vtx.buffer_map) {
      /* Need to allocate a new VBO */
      exec->vtx.buffer_used = 0;

      if (ctx->Driver.BufferData(ctx, GL_ARRAY_BUFFER_ARB,
                                  VBO_VERT_BUFFER_SIZE, 
                                  NULL, usage, exec->vtx.bufferobj)) {
         /* buffer allocation worked, now map the buffer */
         exec->vtx.buffer_map =
            (GLfloat *)ctx->Driver.MapBufferRange(ctx,
                                                  0, VBO_VERT_BUFFER_SIZE,
                                                  accessRange,
                                                  exec->vtx.bufferobj);
      }
      else {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "VBO allocation");
         exec->vtx.buffer_map = NULL;
      }
   }

   exec->vtx.buffer_ptr = exec->vtx.buffer_map;

   if (!exec->vtx.buffer_map) {
      /* out of memory */
      _mesa_install_exec_vtxfmt( ctx, &exec->vtxfmt_noop );
   }
   else {
      if (_mesa_using_noop_vtxfmt(ctx->Exec)) {
         /* The no-op functions are installed so switch back to regular
          * functions.  We do this test just to avoid frequent and needless
          * calls to _mesa_install_exec_vtxfmt().
          */
         _mesa_install_exec_vtxfmt(ctx, &exec->vtxfmt);
      }
   }

   if (0)
      printf("map %d..\n", exec->vtx.buffer_used);
}



/**
 * Execute the buffer and save copied verts.
 * \param keep_unmapped  if true, leave the VBO unmapped when we're done.
 */
void
vbo_exec_vtx_flush(struct vbo_exec_context *exec, GLboolean keepUnmapped)
{
   if (0)
      vbo_exec_debug_verts( exec );

   if (exec->vtx.prim_count && 
       exec->vtx.vert_count) {

      exec->vtx.copied.nr = vbo_copy_vertices( exec ); 

      if (exec->vtx.copied.nr != exec->vtx.vert_count) {
	 struct gl_context *ctx = exec->ctx;

	 vbo_exec_bind_arrays( ctx );

         if (ctx->NewState)
            _mesa_update_state( ctx );

         if (_mesa_is_bufferobj(exec->vtx.bufferobj)) {
            vbo_exec_vtx_unmap( exec );
         }

         if (0)
            printf("%s %d %d\n", __FUNCTION__, exec->vtx.prim_count,
		   exec->vtx.vert_count);

	 vbo_context(ctx)->draw_prims( ctx, 
				       exec->vtx.inputs, 
				       exec->vtx.prim, 
				       exec->vtx.prim_count,
				       NULL,
				       GL_TRUE,
				       0,
				       exec->vtx.vert_count - 1);

	 /* If using a real VBO, get new storage -- unless asked not to.
          */
         if (_mesa_is_bufferobj(exec->vtx.bufferobj) && !keepUnmapped) {
            vbo_exec_vtx_map( exec );
         }
      }
   }

   /* May have to unmap explicitly if we didn't draw:
    */
   if (keepUnmapped &&
       _mesa_is_bufferobj(exec->vtx.bufferobj) &&
       exec->vtx.buffer_map) {
      vbo_exec_vtx_unmap( exec );
   }

   if (keepUnmapped || exec->vtx.vertex_size == 0)
      exec->vtx.max_vert = 0;
   else
      exec->vtx.max_vert = ((VBO_VERT_BUFFER_SIZE - exec->vtx.buffer_used) / 
                            (exec->vtx.vertex_size * sizeof(GLfloat)));

   exec->vtx.buffer_ptr = exec->vtx.buffer_map;
   exec->vtx.prim_count = 0;
   exec->vtx.vert_count = 0;
}


#endif /* FEATURE_beginend */
