/**************************************************************************

Copyright 2002-2008 Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include <precomp.h>

#ifdef ERROR
#undef ERROR
#endif


/** ID/name for immediate-mode VBO */
#define IMM_BUFFER_NAME 0xaabbccdd


static void reset_attrfv( struct vbo_exec_context *exec );


/**
 * Close off the last primitive, execute the buffer, restart the
 * primitive.  
 */
static void vbo_exec_wrap_buffers( struct vbo_exec_context *exec )
{
   if (exec->vtx.prim_count == 0) {
      exec->vtx.copied.nr = 0;
      exec->vtx.vert_count = 0;
      exec->vtx.buffer_ptr = exec->vtx.buffer_map;
   }
   else {
      GLuint last_begin = exec->vtx.prim[exec->vtx.prim_count-1].begin;
      GLuint last_count;

      if (exec->ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END) {
	 GLint i = exec->vtx.prim_count - 1;
	 assert(i >= 0);
	 exec->vtx.prim[i].count = (exec->vtx.vert_count - 
				    exec->vtx.prim[i].start);
      }

      last_count = exec->vtx.prim[exec->vtx.prim_count-1].count;

      /* Execute the buffer and save copied vertices.
       */
      if (exec->vtx.vert_count)
	 vbo_exec_vtx_flush( exec, GL_FALSE );
      else {
	 exec->vtx.prim_count = 0;
	 exec->vtx.copied.nr = 0;
      }

      /* Emit a glBegin to start the new list.
       */
      assert(exec->vtx.prim_count == 0);

      if (exec->ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END) {
	 exec->vtx.prim[0].mode = exec->ctx->Driver.CurrentExecPrimitive;
	 exec->vtx.prim[0].start = 0;
	 exec->vtx.prim[0].count = 0;
	 exec->vtx.prim_count++;
      
	 if (exec->vtx.copied.nr == last_count)
	    exec->vtx.prim[0].begin = last_begin;
      }
   }
}


/**
 * Deal with buffer wrapping where provoked by the vertex buffer
 * filling up, as opposed to upgrade_vertex().
 */
void vbo_exec_vtx_wrap( struct vbo_exec_context *exec )
{
   GLfloat *data = exec->vtx.copied.buffer;
   GLuint i;

   /* Run pipeline on current vertices, copy wrapped vertices
    * to exec->vtx.copied.
    */
   vbo_exec_wrap_buffers( exec );
   
   /* Copy stored stored vertices to start of new list. 
    */
   assert(exec->vtx.max_vert - exec->vtx.vert_count > exec->vtx.copied.nr);

   for (i = 0 ; i < exec->vtx.copied.nr ; i++) {
      memcpy( exec->vtx.buffer_ptr, data, 
	      exec->vtx.vertex_size * sizeof(GLfloat));
      exec->vtx.buffer_ptr += exec->vtx.vertex_size;
      data += exec->vtx.vertex_size;
      exec->vtx.vert_count++;
   }

   exec->vtx.copied.nr = 0;
}


/**
 * Copy the active vertex's values to the ctx->Current fields.
 */
static void vbo_exec_copy_to_current( struct vbo_exec_context *exec )
{
   struct gl_context *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   GLuint i;

   for (i = VBO_ATTRIB_POS+1 ; i < VBO_ATTRIB_MAX ; i++) {
      if (exec->vtx.attrsz[i]) {
         /* Note: the exec->vtx.current[i] pointers point into the
          * ctx->Current.Attrib and ctx->Light.Material.Attrib arrays.
          */
         GLfloat *current = (GLfloat *)vbo->currval[i].Ptr;
         GLfloat tmp[4];

         COPY_CLEAN_4V(tmp, 
                       exec->vtx.attrsz[i], 
                       exec->vtx.attrptr[i]);
         
         if (memcmp(current, tmp, sizeof(tmp)) != 0) { 
            memcpy(current, tmp, sizeof(tmp));
	 
            /* Given that we explicitly state size here, there is no need
             * for the COPY_CLEAN above, could just copy 16 bytes and be
             * done.  The only problem is when Mesa accesses ctx->Current
             * directly.
             */
            vbo->currval[i].Size = exec->vtx.attrsz[i];
            assert(vbo->currval[i].Type == GL_FLOAT);
            vbo->currval[i]._ElementSize = vbo->currval[i].Size * sizeof(GLfloat);

            /* This triggers rather too much recalculation of Mesa state
             * that doesn't get used (eg light positions).
             */
            if (i >= VBO_ATTRIB_MAT_FRONT_AMBIENT && i <= VBO_ATTRIB_MAT_BACK_INDEXES)
               ctx->NewState |= _NEW_LIGHT;
            
            ctx->NewState |= _NEW_CURRENT_ATTRIB;
         }
      }
   }

   /* Colormaterial -- this kindof sucks.
    */
   if (ctx->Light.ColorMaterialEnabled &&
       exec->vtx.attrsz[VBO_ATTRIB_COLOR]) {
      _mesa_update_color_material(ctx, 
				  ctx->Current.Attrib[VBO_ATTRIB_COLOR]);
   }
}


/**
 * Copy current vertex attribute values into the current vertex.
 */
static void
vbo_exec_copy_from_current(struct vbo_exec_context *exec)
{
   struct gl_context *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   GLint i;

   for (i = VBO_ATTRIB_POS + 1; i < VBO_ATTRIB_MAX; i++) {
      const GLfloat *current = (GLfloat *) vbo->currval[i].Ptr;
      switch (exec->vtx.attrsz[i]) {
      case 4: exec->vtx.attrptr[i][3] = current[3];
      case 3: exec->vtx.attrptr[i][2] = current[2];
      case 2: exec->vtx.attrptr[i][1] = current[1];
      case 1: exec->vtx.attrptr[i][0] = current[0];
	 break;
      }
   }
}


/**
 * Flush existing data, set new attrib size, replay copied vertices.
 * This is called when we transition from a small vertex attribute size
 * to a larger one.  Ex: glTexCoord2f -> glTexCoord4f.
 * We need to go back over the previous 2-component texcoords and insert
 * zero and one values.
 */ 
static void
vbo_exec_wrap_upgrade_vertex(struct vbo_exec_context *exec,
                             GLuint attr, GLuint newSize )
{
   struct gl_context *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   const GLint lastcount = exec->vtx.vert_count;
   GLfloat *old_attrptr[VBO_ATTRIB_MAX];
   const GLuint old_vtx_size = exec->vtx.vertex_size; /* floats per vertex */
   const GLuint oldSize = exec->vtx.attrsz[attr];
   GLuint i;

   /* Run pipeline on current vertices, copy wrapped vertices
    * to exec->vtx.copied.
    */
   vbo_exec_wrap_buffers( exec );

   if (unlikely(exec->vtx.copied.nr)) {
      /* We're in the middle of a primitive, keep the old vertex
       * format around to be able to translate the copied vertices to
       * the new format.
       */
      memcpy(old_attrptr, exec->vtx.attrptr, sizeof(old_attrptr));
   }

   if (unlikely(oldSize)) {
      /* Do a COPY_TO_CURRENT to ensure back-copying works for the
       * case when the attribute already exists in the vertex and is
       * having its size increased.
       */
      vbo_exec_copy_to_current( exec );
   }

   /* Heuristic: Attempt to isolate attributes received outside
    * begin/end so that they don't bloat the vertices.
    */
   if (ctx->Driver.CurrentExecPrimitive == PRIM_OUTSIDE_BEGIN_END &&
       !oldSize && lastcount > 8 && exec->vtx.vertex_size) {
      vbo_exec_copy_to_current( exec );
      reset_attrfv( exec );
   }

   /* Fix up sizes:
    */
   exec->vtx.attrsz[attr] = newSize;
   exec->vtx.vertex_size += newSize - oldSize;
   exec->vtx.max_vert = ((VBO_VERT_BUFFER_SIZE - exec->vtx.buffer_used) / 
                         (exec->vtx.vertex_size * sizeof(GLfloat)));
   exec->vtx.vert_count = 0;
   exec->vtx.buffer_ptr = exec->vtx.buffer_map;

   if (unlikely(oldSize)) {
      /* Size changed, recalculate all the attrptr[] values
       */
      GLfloat *tmp = exec->vtx.vertex;

      for (i = 0 ; i < VBO_ATTRIB_MAX ; i++) {
	 if (exec->vtx.attrsz[i]) {
	    exec->vtx.attrptr[i] = tmp;
	    tmp += exec->vtx.attrsz[i];
	 }
	 else
	    exec->vtx.attrptr[i] = NULL; /* will not be dereferenced */
      }

      /* Copy from current to repopulate the vertex with correct
       * values.
       */
      vbo_exec_copy_from_current( exec );
   }
   else {
      /* Just have to append the new attribute at the end */
      exec->vtx.attrptr[attr] = exec->vtx.vertex +
	 exec->vtx.vertex_size - newSize;
   }

   /* Replay stored vertices to translate them
    * to new format here.
    *
    * -- No need to replay - just copy piecewise
    */
   if (unlikely(exec->vtx.copied.nr)) {
      GLfloat *data = exec->vtx.copied.buffer;
      GLfloat *dest = exec->vtx.buffer_ptr;
      GLuint j;

      assert(exec->vtx.buffer_ptr == exec->vtx.buffer_map);

      for (i = 0 ; i < exec->vtx.copied.nr ; i++) {
	 for (j = 0 ; j < VBO_ATTRIB_MAX ; j++) {
	    GLuint sz = exec->vtx.attrsz[j];

	    if (sz) {
	       GLint old_offset = old_attrptr[j] - exec->vtx.vertex;
	       GLint new_offset = exec->vtx.attrptr[j] - exec->vtx.vertex;

	       if (j == attr) {
		  if (oldSize) {
		     GLfloat tmp[4];
		     COPY_CLEAN_4V(tmp, oldSize, data + old_offset);
		     COPY_SZ_4V(dest + new_offset, newSize, tmp);
		  } else {
		     GLfloat *current = (GLfloat *)vbo->currval[j].Ptr;
		     COPY_SZ_4V(dest + new_offset, sz, current);
		  }
	       }
	       else {
		  COPY_SZ_4V(dest + new_offset, sz, data + old_offset);
	       }
	    }
	 }

	 data += old_vtx_size;
	 dest += exec->vtx.vertex_size;
      }

      exec->vtx.buffer_ptr = dest;
      exec->vtx.vert_count += exec->vtx.copied.nr;
      exec->vtx.copied.nr = 0;
   }
}


/**
 * This is when a vertex attribute transitions to a different size.
 * For example, we saw a bunch of glTexCoord2f() calls and now we got a
 * glTexCoord4f() call.  We promote the array from size=2 to size=4.
 */
static void
vbo_exec_fixup_vertex(struct gl_context *ctx, GLuint attr, GLuint newSize)
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   if (newSize > exec->vtx.attrsz[attr]) {
      /* New size is larger.  Need to flush existing vertices and get
       * an enlarged vertex format.
       */
      vbo_exec_wrap_upgrade_vertex( exec, attr, newSize );
   }
   else if (newSize < exec->vtx.active_sz[attr]) {
      static const GLfloat id[4] = { 0, 0, 0, 1 };
      GLuint i;

      /* New size is smaller - just need to fill in some
       * zeros.  Don't need to flush or wrap.
       */
      for (i = newSize; i <= exec->vtx.attrsz[attr]; i++)
	 exec->vtx.attrptr[attr][i-1] = id[i-1];
   }

   exec->vtx.active_sz[attr] = newSize;

   /* Does setting NeedFlush belong here?  Necessitates resetting
    * vtxfmt on each flush (otherwise flags won't get reset
    * afterwards).
    */
   if (attr == 0) 
      ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
}


/**
 * This macro is used to implement all the glVertex, glColor, glTexCoord,
 * glVertexAttrib, etc functions.
 */
#define ATTR( A, N, V0, V1, V2, V3 )					\
do {									\
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;		\
									\
   if (unlikely(!(ctx->Driver.NeedFlush & FLUSH_UPDATE_CURRENT)))	\
      ctx->Driver.BeginVertices( ctx );					\
   									\
   if (unlikely(exec->vtx.active_sz[A] != N))				\
      vbo_exec_fixup_vertex(ctx, A, N);					\
   									\
   {									\
      GLfloat *dest = exec->vtx.attrptr[A];				\
      if (N>0) dest[0] = V0;						\
      if (N>1) dest[1] = V1;						\
      if (N>2) dest[2] = V2;						\
      if (N>3) dest[3] = V3;						\
   }									\
									\
   if ((A) == 0) {							\
      /* This is a glVertex call */					\
      GLuint i;								\
									\
      for (i = 0; i < exec->vtx.vertex_size; i++)			\
	 exec->vtx.buffer_ptr[i] = exec->vtx.vertex[i];			\
									\
      exec->vtx.buffer_ptr += exec->vtx.vertex_size;			\
									\
      /* Set FLUSH_STORED_VERTICES to indicate that there's now */	\
      /* something to draw (not just updating a color or texcoord).*/	\
      ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;			\
									\
      if (++exec->vtx.vert_count >= exec->vtx.max_vert)			\
	 vbo_exec_vtx_wrap( exec );					\
   }									\
} while (0)


#define ERROR(err) _mesa_error( ctx, err, __FUNCTION__ )
#define TAG(x) vbo_##x

#include "vbo_attrib_tmp.h"



/**
 * Execute a glMaterial call.  Note that if GL_COLOR_MATERIAL is enabled,
 * this may be a (partial) no-op.
 */
static void GLAPIENTRY
vbo_Materialfv(GLenum face, GLenum pname, const GLfloat *params)
{
   GLbitfield updateMats;
   GET_CURRENT_CONTEXT(ctx);

   /* This function should be a no-op when it tries to update material
    * attributes which are currently tracking glColor via glColorMaterial.
    * The updateMats var will be a mask of the MAT_BIT_FRONT/BACK_x bits
    * indicating which material attributes can actually be updated below.
    */
   if (ctx->Light.ColorMaterialEnabled) {
      updateMats = ~ctx->Light.ColorMaterialBitmask;
   }
   else {
      /* GL_COLOR_MATERIAL is disabled so don't skip any material updates */
      updateMats = ALL_MATERIAL_BITS;
   }

   if (face == GL_FRONT) {
      updateMats &= FRONT_MATERIAL_BITS;
   }
   else if (face == GL_BACK) {
      updateMats &= BACK_MATERIAL_BITS;
   }
   else if (face != GL_FRONT_AND_BACK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMaterial(invalid face)");
      return;
   }

   switch (pname) {
   case GL_EMISSION:
      if (updateMats & MAT_BIT_FRONT_EMISSION)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_EMISSION, 4, params);
      if (updateMats & MAT_BIT_BACK_EMISSION)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_EMISSION, 4, params);
      break;
   case GL_AMBIENT:
      if (updateMats & MAT_BIT_FRONT_AMBIENT)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_AMBIENT, 4, params);
      if (updateMats & MAT_BIT_BACK_AMBIENT)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_AMBIENT, 4, params);
      break;
   case GL_DIFFUSE:
      if (updateMats & MAT_BIT_FRONT_DIFFUSE)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_DIFFUSE, 4, params);
      if (updateMats & MAT_BIT_BACK_DIFFUSE)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_DIFFUSE, 4, params);
      break;
   case GL_SPECULAR:
      if (updateMats & MAT_BIT_FRONT_SPECULAR)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_SPECULAR, 4, params);
      if (updateMats & MAT_BIT_BACK_SPECULAR)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_SPECULAR, 4, params);
      break;
   case GL_SHININESS:
      if (*params < 0 || *params > ctx->Const.MaxShininess) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glMaterial(invalid shininess: %f out range [0, %f])",
		     *params, ctx->Const.MaxShininess);
         return;
      }
      if (updateMats & MAT_BIT_FRONT_SHININESS)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_SHININESS, 1, params);
      if (updateMats & MAT_BIT_BACK_SHININESS)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_SHININESS, 1, params);
      break;
   case GL_COLOR_INDEXES:
      if (updateMats & MAT_BIT_FRONT_INDEXES)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_INDEXES, 3, params);
      if (updateMats & MAT_BIT_BACK_INDEXES)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_INDEXES, 3, params);
      break;
   case GL_AMBIENT_AND_DIFFUSE:
      if (updateMats & MAT_BIT_FRONT_AMBIENT)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_AMBIENT, 4, params);
      if (updateMats & MAT_BIT_FRONT_DIFFUSE)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_DIFFUSE, 4, params);
      if (updateMats & MAT_BIT_BACK_AMBIENT)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_AMBIENT, 4, params);
      if (updateMats & MAT_BIT_BACK_DIFFUSE)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_DIFFUSE, 4, params);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glMaterialfv(pname)");
      return;
   }
}


/**
 * Flush (draw) vertices.
 * \param  unmap - leave VBO unmapped after flushing?
 */
static void
vbo_exec_FlushVertices_internal(struct vbo_exec_context *exec, GLboolean unmap)
{
   if (exec->vtx.vert_count || unmap) {
      vbo_exec_vtx_flush( exec, unmap );
   }

   if (exec->vtx.vertex_size) {
      vbo_exec_copy_to_current( exec );
      reset_attrfv( exec );
   }
}


#if FEATURE_beginend


#if FEATURE_evaluators

static void GLAPIENTRY vbo_exec_EvalCoord1f( GLfloat u )
{
   GET_CURRENT_CONTEXT( ctx );
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   {
      GLint i;
      if (exec->eval.recalculate_maps) 
	 vbo_exec_eval_update( exec );

      for (i = 0; i <= VBO_ATTRIB_TEX; i++) {
	 if (exec->eval.map1[i].map) 
	    if (exec->vtx.active_sz[i] != exec->eval.map1[i].sz)
	       vbo_exec_fixup_vertex( ctx, i, exec->eval.map1[i].sz );
      }
   }


   memcpy( exec->vtx.copied.buffer, exec->vtx.vertex, 
           exec->vtx.vertex_size * sizeof(GLfloat));

   vbo_exec_do_EvalCoord1f( exec, u );

   memcpy( exec->vtx.vertex, exec->vtx.copied.buffer,
           exec->vtx.vertex_size * sizeof(GLfloat));
}

static void GLAPIENTRY vbo_exec_EvalCoord2f( GLfloat u, GLfloat v )
{
   GET_CURRENT_CONTEXT( ctx );
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   {
      GLint i;
      if (exec->eval.recalculate_maps) 
	 vbo_exec_eval_update( exec );

      for (i = 0; i <= VBO_ATTRIB_TEX; i++) {
	 if (exec->eval.map2[i].map) 
	    if (exec->vtx.active_sz[i] != exec->eval.map2[i].sz)
	       vbo_exec_fixup_vertex( ctx, i, exec->eval.map2[i].sz );
      }

      if (ctx->Eval.AutoNormal) 
	 if (exec->vtx.active_sz[VBO_ATTRIB_NORMAL] != 3)
	    vbo_exec_fixup_vertex( ctx, VBO_ATTRIB_NORMAL, 3 );
   }

   memcpy( exec->vtx.copied.buffer, exec->vtx.vertex, 
           exec->vtx.vertex_size * sizeof(GLfloat));

   vbo_exec_do_EvalCoord2f( exec, u, v );

   memcpy( exec->vtx.vertex, exec->vtx.copied.buffer, 
           exec->vtx.vertex_size * sizeof(GLfloat));
}

static void GLAPIENTRY vbo_exec_EvalCoord1fv( const GLfloat *u )
{
   vbo_exec_EvalCoord1f( u[0] );
}

static void GLAPIENTRY vbo_exec_EvalCoord2fv( const GLfloat *u )
{
   vbo_exec_EvalCoord2f( u[0], u[1] );
}

static void GLAPIENTRY vbo_exec_EvalPoint1( GLint i )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat du = ((ctx->Eval.MapGrid1u2 - ctx->Eval.MapGrid1u1) /
		 (GLfloat) ctx->Eval.MapGrid1un);
   GLfloat u = i * du + ctx->Eval.MapGrid1u1;

   vbo_exec_EvalCoord1f( u );
}


static void GLAPIENTRY vbo_exec_EvalPoint2( GLint i, GLint j )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat du = ((ctx->Eval.MapGrid2u2 - ctx->Eval.MapGrid2u1) / 
		 (GLfloat) ctx->Eval.MapGrid2un);
   GLfloat dv = ((ctx->Eval.MapGrid2v2 - ctx->Eval.MapGrid2v1) / 
		 (GLfloat) ctx->Eval.MapGrid2vn);
   GLfloat u = i * du + ctx->Eval.MapGrid2u1;
   GLfloat v = j * dv + ctx->Eval.MapGrid2v1;

   vbo_exec_EvalCoord2f( u, v );
}


static void GLAPIENTRY
vbo_exec_EvalMesh1(GLenum mode, GLint i1, GLint i2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   GLfloat u, du;
   GLenum prim;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (mode) {
   case GL_POINT:
      prim = GL_POINTS;
      break;
   case GL_LINE:
      prim = GL_LINE_STRIP;
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glEvalMesh1(mode)" );
      return;
   }

   /* No effect if vertex maps disabled.
    */
   if (!ctx->Eval.Map1Vertex4 && 
       !ctx->Eval.Map1Vertex3)
      return;

   du = ctx->Eval.MapGrid1du;
   u = ctx->Eval.MapGrid1u1 + i1 * du;

   CALL_Begin(GET_DISPATCH(), (prim));
   for (i=i1;i<=i2;i++,u+=du) {
      CALL_EvalCoord1f(GET_DISPATCH(), (u));
   }
   CALL_End(GET_DISPATCH(), ());
}


static void GLAPIENTRY
vbo_exec_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat u, du, v, dv, v1, u1;
   GLint i, j;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (mode) {
   case GL_POINT:
   case GL_LINE:
   case GL_FILL:
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glEvalMesh2(mode)" );
      return;
   }

   /* No effect if vertex maps disabled.
    */
   if (!ctx->Eval.Map2Vertex4 && 
       !ctx->Eval.Map2Vertex3)
      return;

   du = ctx->Eval.MapGrid2du;
   dv = ctx->Eval.MapGrid2dv;
   v1 = ctx->Eval.MapGrid2v1 + j1 * dv;
   u1 = ctx->Eval.MapGrid2u1 + i1 * du;

   switch (mode) {
   case GL_POINT:
      CALL_Begin(GET_DISPATCH(), (GL_POINTS));
      for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	 for (u=u1,i=i1;i<=i2;i++,u+=du) {
	    CALL_EvalCoord2f(GET_DISPATCH(), (u, v));
	 }
      }
      CALL_End(GET_DISPATCH(), ());
      break;
   case GL_LINE:
      for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	 CALL_Begin(GET_DISPATCH(), (GL_LINE_STRIP));
	 for (u=u1,i=i1;i<=i2;i++,u+=du) {
	    CALL_EvalCoord2f(GET_DISPATCH(), (u, v));
	 }
	 CALL_End(GET_DISPATCH(), ());
      }
      for (u=u1,i=i1;i<=i2;i++,u+=du) {
	 CALL_Begin(GET_DISPATCH(), (GL_LINE_STRIP));
	 for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	    CALL_EvalCoord2f(GET_DISPATCH(), (u, v));
	 }
	 CALL_End(GET_DISPATCH(), ());
      }
      break;
   case GL_FILL:
      for (v=v1,j=j1;j<j2;j++,v+=dv) {
	 CALL_Begin(GET_DISPATCH(), (GL_TRIANGLE_STRIP));
	 for (u=u1,i=i1;i<=i2;i++,u+=du) {
	    CALL_EvalCoord2f(GET_DISPATCH(), (u, v));
	    CALL_EvalCoord2f(GET_DISPATCH(), (u, v+dv));
	 }
	 CALL_End(GET_DISPATCH(), ());
      }
      break;
   }
}

#endif /* FEATURE_evaluators */


/**
 * Execute a glRectf() function.  This is not suitable for GL_COMPILE
 * modes (as the test for outside begin/end is not compiled),
 * but may be useful for drivers in circumstances which exclude
 * display list interactions.
 *
 * (None of the functions in this file are suitable for GL_COMPILE
 * modes).
 */
static void GLAPIENTRY
vbo_exec_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   CALL_Begin(GET_DISPATCH(), (GL_QUADS));
   CALL_Vertex2f(GET_DISPATCH(), (x1, y1));
   CALL_Vertex2f(GET_DISPATCH(), (x2, y1));
   CALL_Vertex2f(GET_DISPATCH(), (x2, y2));
   CALL_Vertex2f(GET_DISPATCH(), (x1, y2));
   CALL_End(GET_DISPATCH(), ());
}


/**
 * Called via glBegin.
 */
static void GLAPIENTRY vbo_exec_Begin( GLenum mode )
{
   GET_CURRENT_CONTEXT( ctx ); 

   if (ctx->Driver.CurrentExecPrimitive == PRIM_OUTSIDE_BEGIN_END) {
      struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
      int i;

      if (!_mesa_valid_prim_mode(ctx, mode)) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glBegin");
         return;
      }

      vbo_draw_method(exec, DRAW_BEGIN_END);

      if (ctx->Driver.PrepareExecBegin)
	 ctx->Driver.PrepareExecBegin(ctx);

      if (ctx->NewState) {
	 _mesa_update_state( ctx );

	 CALL_Begin(ctx->Exec, (mode));
	 return;
      }

      if (!_mesa_valid_to_render(ctx, "glBegin")) {
         return;
      }

      /* Heuristic: attempt to isolate attributes occurring outside
       * begin/end pairs.
       */
      if (exec->vtx.vertex_size && !exec->vtx.attrsz[0]) 
	 vbo_exec_FlushVertices_internal(exec, GL_FALSE);

      i = exec->vtx.prim_count++;
      exec->vtx.prim[i].mode = mode;
      exec->vtx.prim[i].begin = 1;
      exec->vtx.prim[i].end = 0;
      exec->vtx.prim[i].indexed = 0;
      exec->vtx.prim[i].weak = 0;
      exec->vtx.prim[i].pad = 0;
      exec->vtx.prim[i].start = exec->vtx.vert_count;
      exec->vtx.prim[i].count = 0;
      exec->vtx.prim[i].num_instances = 1;

      ctx->Driver.CurrentExecPrimitive = mode;
   }
   else 
      _mesa_error( ctx, GL_INVALID_OPERATION, "glBegin" );
      
}


/**
 * Called via glEnd.
 */
static void GLAPIENTRY vbo_exec_End( void )
{
   GET_CURRENT_CONTEXT( ctx ); 

   if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END) {
      struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

      if (exec->vtx.prim_count > 0) {
         /* close off current primitive */
         int idx = exec->vtx.vert_count;
         int i = exec->vtx.prim_count - 1;

         exec->vtx.prim[i].end = 1; 
         exec->vtx.prim[i].count = idx - exec->vtx.prim[i].start;
      }

      ctx->Driver.CurrentExecPrimitive = PRIM_OUTSIDE_BEGIN_END;

      if (exec->vtx.prim_count == VBO_MAX_PRIM)
	 vbo_exec_vtx_flush( exec, GL_FALSE );
   }
   else 
      _mesa_error( ctx, GL_INVALID_OPERATION, "glEnd" );
}


static void vbo_exec_vtxfmt_init( struct vbo_exec_context *exec )
{
   GLvertexformat *vfmt = &exec->vtxfmt;

   _MESA_INIT_ARRAYELT_VTXFMT(vfmt, _ae_);

   vfmt->Begin = vbo_exec_Begin;
   vfmt->End = vbo_exec_End;

   _MESA_INIT_DLIST_VTXFMT(vfmt, _mesa_);
   _MESA_INIT_EVAL_VTXFMT(vfmt, vbo_exec_);

   vfmt->Rectf = vbo_exec_Rectf;

   /* from attrib_tmp.h:
    */
   vfmt->Color3f = vbo_Color3f;
   vfmt->Color3fv = vbo_Color3fv;
   vfmt->Color4f = vbo_Color4f;
   vfmt->Color4fv = vbo_Color4fv;
   vfmt->FogCoordfEXT = vbo_FogCoordfEXT;
   vfmt->FogCoordfvEXT = vbo_FogCoordfvEXT;
   vfmt->Normal3f = vbo_Normal3f;
   vfmt->Normal3fv = vbo_Normal3fv;
   vfmt->TexCoord1f = vbo_TexCoord1f;
   vfmt->TexCoord1fv = vbo_TexCoord1fv;
   vfmt->TexCoord2f = vbo_TexCoord2f;
   vfmt->TexCoord2fv = vbo_TexCoord2fv;
   vfmt->TexCoord3f = vbo_TexCoord3f;
   vfmt->TexCoord3fv = vbo_TexCoord3fv;
   vfmt->TexCoord4f = vbo_TexCoord4f;
   vfmt->TexCoord4fv = vbo_TexCoord4fv;
   vfmt->Vertex2f = vbo_Vertex2f;
   vfmt->Vertex2fv = vbo_Vertex2fv;
   vfmt->Vertex3f = vbo_Vertex3f;
   vfmt->Vertex3fv = vbo_Vertex3fv;
   vfmt->Vertex4f = vbo_Vertex4f;
   vfmt->Vertex4fv = vbo_Vertex4fv;

   vfmt->VertexAttrib1fNV = vbo_VertexAttrib1fNV;
   vfmt->VertexAttrib1fvNV = vbo_VertexAttrib1fvNV;
   vfmt->VertexAttrib2fNV = vbo_VertexAttrib2fNV;
   vfmt->VertexAttrib2fvNV = vbo_VertexAttrib2fvNV;
   vfmt->VertexAttrib3fNV = vbo_VertexAttrib3fNV;
   vfmt->VertexAttrib3fvNV = vbo_VertexAttrib3fvNV;
   vfmt->VertexAttrib4fNV = vbo_VertexAttrib4fNV;
   vfmt->VertexAttrib4fvNV = vbo_VertexAttrib4fvNV;

   vfmt->Materialfv = vbo_Materialfv;

   vfmt->EdgeFlag = vbo_EdgeFlag;
   vfmt->Indexf = vbo_Indexf;
   vfmt->Indexfv = vbo_Indexfv;
}


#else /* FEATURE_beginend */


static void vbo_exec_vtxfmt_init( struct vbo_exec_context *exec )
{
   /* silence warnings */
   (void) vbo_Color3f;
   (void) vbo_Color3fv;
   (void) vbo_Color4f;
   (void) vbo_Color4fv;
   (void) vbo_FogCoordfEXT;
   (void) vbo_FogCoordfvEXT;
   (void) vbo_MultiTexCoord1f;
   (void) vbo_MultiTexCoord1fv;
   (void) vbo_MultiTexCoord2f;
   (void) vbo_MultiTexCoord2fv;
   (void) vbo_MultiTexCoord3f;
   (void) vbo_MultiTexCoord3fv;
   (void) vbo_MultiTexCoord4f;
   (void) vbo_MultiTexCoord4fv;
   (void) vbo_Normal3f;
   (void) vbo_Normal3fv;
   (void) vbo_TexCoord1f;
   (void) vbo_TexCoord1fv;
   (void) vbo_TexCoord2f;
   (void) vbo_TexCoord2fv;
   (void) vbo_TexCoord3f;
   (void) vbo_TexCoord3fv;
   (void) vbo_TexCoord4f;
   (void) vbo_TexCoord4fv;
   (void) vbo_Vertex2f;
   (void) vbo_Vertex2fv;
   (void) vbo_Vertex3f;
   (void) vbo_Vertex3fv;
   (void) vbo_Vertex4f;
   (void) vbo_Vertex4fv;

   (void) vbo_VertexAttrib1fNV;
   (void) vbo_VertexAttrib1fvNV;
   (void) vbo_VertexAttrib2fNV;
   (void) vbo_VertexAttrib2fvNV;
   (void) vbo_VertexAttrib3fNV;
   (void) vbo_VertexAttrib3fvNV;
   (void) vbo_VertexAttrib4fNV;
   (void) vbo_VertexAttrib4fvNV;

   (void) vbo_Materialfv;

   (void) vbo_EdgeFlag;
   (void) vbo_Indexf;
   (void) vbo_Indexfv;
}


#endif /* FEATURE_beginend */


/**
 * Tell the VBO module to use a real OpenGL vertex buffer object to
 * store accumulated immediate-mode vertex data.
 * This replaces the malloced buffer which was created in
 * vb_exec_vtx_init() below.
 */
void vbo_use_buffer_objects(struct gl_context *ctx)
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
   /* Any buffer name but 0 can be used here since this bufferobj won't
    * go into the bufferobj hashtable.
    */
   GLuint bufName = IMM_BUFFER_NAME;
   GLenum target = GL_ARRAY_BUFFER_ARB;
   GLenum usage = GL_STREAM_DRAW_ARB;
   GLsizei size = VBO_VERT_BUFFER_SIZE;

   /* Make sure this func is only used once */
   assert(exec->vtx.bufferobj == ctx->Shared->NullBufferObj);
   if (exec->vtx.buffer_map) {
      _mesa_align_free(exec->vtx.buffer_map);
      exec->vtx.buffer_map = NULL;
      exec->vtx.buffer_ptr = NULL;
   }

   /* Allocate a real buffer object now */
   _mesa_reference_buffer_object(ctx, &exec->vtx.bufferobj, NULL);
   exec->vtx.bufferobj = ctx->Driver.NewBufferObject(ctx, bufName, target);
   if (!ctx->Driver.BufferData(ctx, target, size, NULL, usage, exec->vtx.bufferobj)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "VBO allocation");
   }
}


/**
 * If this function is called, all VBO buffers will be unmapped when
 * we flush.
 * Otherwise, if a simple command like glColor3f() is called and we flush,
 * the current VBO may be left mapped.
 */
void
vbo_always_unmap_buffers(struct gl_context *ctx)
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
   exec->begin_vertices_flags |= FLUSH_STORED_VERTICES;
}


void vbo_exec_vtx_init( struct vbo_exec_context *exec )
{
   struct gl_context *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   GLuint i;

   /* Allocate a buffer object.  Will just reuse this object
    * continuously, unless vbo_use_buffer_objects() is called to enable
    * use of real VBOs.
    */
   _mesa_reference_buffer_object(ctx,
                                 &exec->vtx.bufferobj,
                                 ctx->Shared->NullBufferObj);

   ASSERT(!exec->vtx.buffer_map);
   exec->vtx.buffer_map = (GLfloat *)_mesa_align_malloc(VBO_VERT_BUFFER_SIZE, 64);
   exec->vtx.buffer_ptr = exec->vtx.buffer_map;

   vbo_exec_vtxfmt_init( exec );
   _mesa_noop_vtxfmt_init(&exec->vtxfmt_noop);

   /* Hook our functions into the dispatch table.
    */
   _mesa_install_exec_vtxfmt( ctx, &exec->vtxfmt );

   for (i = 0 ; i < VBO_ATTRIB_MAX ; i++) {
      ASSERT(i < Elements(exec->vtx.attrsz));
      exec->vtx.attrsz[i] = 0;
      ASSERT(i < Elements(exec->vtx.active_sz));
      exec->vtx.active_sz[i] = 0;
   }
   for (i = 0 ; i < VERT_ATTRIB_MAX; i++) {
      ASSERT(i < Elements(exec->vtx.inputs));
      ASSERT(i < Elements(exec->vtx.arrays));
      exec->vtx.inputs[i] = &exec->vtx.arrays[i];
   }
   
   {
      struct gl_client_array *arrays = exec->vtx.arrays;
      unsigned i;

      memcpy(arrays, vbo->legacy_currval,
             VERT_ATTRIB_MAX * sizeof(arrays[0]));
      for (i = 0; i < VERT_ATTRIB_MAX; ++i) {
         struct gl_client_array *array;
         array = &arrays[VERT_ATTRIB(i)];
         array->BufferObj = NULL;
         _mesa_reference_buffer_object(ctx, &arrays->BufferObj,
                                       vbo->legacy_currval[i].BufferObj);
      }
   }

   exec->vtx.vertex_size = 0;

   exec->begin_vertices_flags = FLUSH_UPDATE_CURRENT;
}


void vbo_exec_vtx_destroy( struct vbo_exec_context *exec )
{
   /* using a real VBO for vertex data */
   struct gl_context *ctx = exec->ctx;
   unsigned i;

   /* True VBOs should already be unmapped
    */
   if (exec->vtx.buffer_map) {
      ASSERT(exec->vtx.bufferobj->Name == 0 ||
             exec->vtx.bufferobj->Name == IMM_BUFFER_NAME);
      if (exec->vtx.bufferobj->Name == 0) {
         _mesa_align_free(exec->vtx.buffer_map);
         exec->vtx.buffer_map = NULL;
         exec->vtx.buffer_ptr = NULL;
      }
   }

   /* Drop any outstanding reference to the vertex buffer
    */
   for (i = 0; i < Elements(exec->vtx.arrays); i++) {
      _mesa_reference_buffer_object(ctx,
                                    &exec->vtx.arrays[i].BufferObj,
                                    NULL);
   }

   /* Free the vertex buffer.  Unmap first if needed.
    */
   if (_mesa_bufferobj_mapped(exec->vtx.bufferobj)) {
      ctx->Driver.UnmapBuffer(ctx, exec->vtx.bufferobj);
   }
   _mesa_reference_buffer_object(ctx, &exec->vtx.bufferobj, NULL);
}


/**
 * Called upon first glVertex, glColor, glTexCoord, etc.
 */
void vbo_exec_BeginVertices( struct gl_context *ctx )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   vbo_exec_vtx_map( exec );

   assert((ctx->Driver.NeedFlush & FLUSH_UPDATE_CURRENT) == 0);
   assert(exec->begin_vertices_flags);

   ctx->Driver.NeedFlush |= exec->begin_vertices_flags;
}


/**
 * Called via ctx->Driver.FlushVertices()
 * \param flags  bitmask of FLUSH_STORED_VERTICES, FLUSH_UPDATE_CURRENT
 */
void vbo_exec_FlushVertices( struct gl_context *ctx, GLuint flags )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

#ifdef DEBUG
   /* debug check: make sure we don't get called recursively */
   exec->flush_call_depth++;
   assert(exec->flush_call_depth == 1);
#endif

   if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END) {
      /* We've had glBegin but not glEnd! */
#ifdef DEBUG
      exec->flush_call_depth--;
      assert(exec->flush_call_depth == 0);
#endif
      return;
   }

   /* Flush (draw), and make sure VBO is left unmapped when done */
   vbo_exec_FlushVertices_internal(exec, GL_TRUE);

   /* Need to do this to ensure BeginVertices gets called again:
    */
   ctx->Driver.NeedFlush &= ~(FLUSH_UPDATE_CURRENT | flags);

#ifdef DEBUG
   exec->flush_call_depth--;
   assert(exec->flush_call_depth == 0);
#endif
}


static void reset_attrfv( struct vbo_exec_context *exec )
{   
   GLuint i;

   for (i = 0 ; i < VBO_ATTRIB_MAX ; i++) {
      exec->vtx.attrsz[i] = 0;
      exec->vtx.active_sz[i] = 0;
   }

   exec->vtx.vertex_size = 0;
}
