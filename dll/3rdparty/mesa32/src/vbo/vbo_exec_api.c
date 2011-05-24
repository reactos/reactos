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

#include "main/glheader.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/vtxfmt.h"
#if FEATURE_dlist
#include "main/dlist.h"
#endif
#include "main/state.h"
#include "main/light.h"
#include "main/api_arrayelt.h"
#include "main/api_noop.h"
#include "glapi/dispatch.h"

#include "vbo_context.h"

#ifdef ERROR
#undef ERROR
#endif


static void reset_attrfv( struct vbo_exec_context *exec );


/* Close off the last primitive, execute the buffer, restart the
 * primitive.  
 */
static void vbo_exec_wrap_buffers( struct vbo_exec_context *exec )
{
   if (exec->vtx.prim_count == 0) {
      exec->vtx.copied.nr = 0;
      exec->vtx.vert_count = 0;
      exec->vtx.vbptr = (GLfloat *)exec->vtx.buffer_map;
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
	 vbo_exec_vtx_flush( exec );
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


/* Deal with buffer wrapping where provoked by the vertex buffer
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
      _mesa_memcpy( exec->vtx.vbptr, data, 
		    exec->vtx.vertex_size * sizeof(GLfloat));
      exec->vtx.vbptr += exec->vtx.vertex_size;
      data += exec->vtx.vertex_size;
      exec->vtx.vert_count++;
   }

   exec->vtx.copied.nr = 0;
}


/*
 * Copy the active vertex's values to the ctx->Current fields.
 */
static void vbo_exec_copy_to_current( struct vbo_exec_context *exec )
{
   GLcontext *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   GLuint i;

   for (i = VBO_ATTRIB_POS+1 ; i < VBO_ATTRIB_MAX ; i++) {
      if (exec->vtx.attrsz[i]) {
	 GLfloat *current = (GLfloat *)vbo->currval[i].Ptr;

         /* Note: the exec->vtx.current[i] pointers point into the
          * ctx->Current.Attrib and ctx->Light.Material.Attrib arrays.
          */
         if (exec->vtx.attrptr[i]) {

	 COPY_CLEAN_4V(current, 
		       exec->vtx.attrsz[i], 
		       exec->vtx.attrptr[i]);

	 }

	 /* Given that we explicitly state size here, there is no need
	  * for the COPY_CLEAN above, could just copy 16 bytes and be
	  * done.  The only problem is when Mesa accesses ctx->Current
	  * directly.
	  */
	 vbo->currval[i].Size = exec->vtx.attrsz[i];

	 /* This triggers rather too much recalculation of Mesa state
	  * that doesn't get used (eg light positions).
	  */
	 if (i >= VBO_ATTRIB_MAT_FRONT_AMBIENT &&
	     i <= VBO_ATTRIB_MAT_BACK_INDEXES)
	    ctx->NewState |= _NEW_LIGHT;
      }
   }

   /* Colormaterial -- this kindof sucks.
    */
   if (ctx->Light.ColorMaterialEnabled &&
       exec->vtx.attrsz[VBO_ATTRIB_COLOR0]) {
      _mesa_update_color_material(ctx, 
				  ctx->Current.Attrib[VBO_ATTRIB_COLOR0]);
   }

   ctx->Driver.NeedFlush &= ~FLUSH_UPDATE_CURRENT;
}


static void vbo_exec_copy_from_current( struct vbo_exec_context *exec )
{
   GLcontext *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   GLint i;

   for (i = VBO_ATTRIB_POS+1 ; i < VBO_ATTRIB_MAX ; i++) {
      const GLfloat *current = (GLfloat *)vbo->currval[i].Ptr;
      switch (exec->vtx.attrsz[i]) {
      case 4: exec->vtx.attrptr[i][3] = current[3];
      case 3: exec->vtx.attrptr[i][2] = current[2];
      case 2: exec->vtx.attrptr[i][1] = current[1];
      case 1: exec->vtx.attrptr[i][0] = current[0];
	 break;
      }
   }

   ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;
}


/* Flush existing data, set new attrib size, replay copied vertices.
 */ 
static void vbo_exec_wrap_upgrade_vertex( struct vbo_exec_context *exec,
					  GLuint attr,
					  GLuint newsz )
{
   GLcontext *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   GLint lastcount = exec->vtx.vert_count;
   GLfloat *tmp;
   GLuint oldsz;
   GLuint i;

   /* Run pipeline on current vertices, copy wrapped vertices
    * to exec->vtx.copied.
    */
   vbo_exec_wrap_buffers( exec );


   /* Do a COPY_TO_CURRENT to ensure back-copying works for the case
    * when the attribute already exists in the vertex and is having
    * its size increased.  
    */
   vbo_exec_copy_to_current( exec );


   /* Heuristic: Attempt to isolate attributes received outside
    * begin/end so that they don't bloat the vertices.
    */
   if (ctx->Driver.CurrentExecPrimitive == PRIM_OUTSIDE_BEGIN_END &&
       exec->vtx.attrsz[attr] == 0 && 
       lastcount > 8 &&
       exec->vtx.vertex_size) {
      reset_attrfv( exec );
   }

   /* Fix up sizes:
    */
   oldsz = exec->vtx.attrsz[attr];
   exec->vtx.attrsz[attr] = newsz;

   exec->vtx.vertex_size += newsz - oldsz;
   exec->vtx.max_vert = VBO_VERT_BUFFER_SIZE / exec->vtx.vertex_size;
   exec->vtx.vert_count = 0;
   exec->vtx.vbptr = (GLfloat *)exec->vtx.buffer_map;
   

   /* Recalculate all the attrptr[] values
    */
   for (i = 0, tmp = exec->vtx.vertex ; i < VBO_ATTRIB_MAX ; i++) {
      if (exec->vtx.attrsz[i]) {
	 exec->vtx.attrptr[i] = tmp;
	 tmp += exec->vtx.attrsz[i];
      }
      else 
	 exec->vtx.attrptr[i] = NULL; /* will not be dereferenced */
   }

   /* Copy from current to repopulate the vertex with correct values.
    */
   vbo_exec_copy_from_current( exec );

   /* Replay stored vertices to translate them
    * to new format here.
    *
    * -- No need to replay - just copy piecewise
    */
   if (exec->vtx.copied.nr)
   {
      GLfloat *data = exec->vtx.copied.buffer;
      GLfloat *dest = exec->vtx.vbptr;
      GLuint j;

      assert(exec->vtx.vbptr == (GLfloat *)exec->vtx.buffer_map);
      
      for (i = 0 ; i < exec->vtx.copied.nr ; i++) {
	 for (j = 0 ; j < VBO_ATTRIB_MAX ; j++) {
	    if (exec->vtx.attrsz[j]) {
	       if (j == attr) {
		  if (oldsz) {
		     COPY_CLEAN_4V( dest, oldsz, data );
		     data += oldsz;
		     dest += newsz;
		  } else {
		     const GLfloat *current = (const GLfloat *)vbo->currval[j].Ptr;
		     COPY_SZ_4V( dest, newsz, current );
		     dest += newsz;
		  }
	       }
	       else {
		  GLuint sz = exec->vtx.attrsz[j];
		  COPY_SZ_4V( dest, sz, data );
		  dest += sz;
		  data += sz;
	       }
	    }
	 }
      }

      exec->vtx.vbptr = dest;
      exec->vtx.vert_count += exec->vtx.copied.nr;
      exec->vtx.copied.nr = 0;
   }
}


static void vbo_exec_fixup_vertex( GLcontext *ctx,
				   GLuint attr, GLuint sz )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
   int i;

   if (sz > exec->vtx.attrsz[attr]) {
      /* New size is larger.  Need to flush existing vertices and get
       * an enlarged vertex format.
       */
      vbo_exec_wrap_upgrade_vertex( exec, attr, sz );
   }
   else if (sz < exec->vtx.active_sz[attr]) {
      static const GLfloat id[4] = { 0, 0, 0, 1 };

      /* New size is smaller - just need to fill in some
       * zeros.  Don't need to flush or wrap.
       */
      for (i = sz ; i <= exec->vtx.attrsz[attr] ; i++)
	 exec->vtx.attrptr[attr][i-1] = id[i-1];
   }

   exec->vtx.active_sz[attr] = sz;

   /* Does setting NeedFlush belong here?  Necessitates resetting
    * vtxfmt on each flush (otherwise flags won't get reset
    * afterwards).
    */
   if (attr == 0) 
      exec->ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
   else 
      exec->ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;
}




/* 
 */
#define ATTR( A, N, V0, V1, V2, V3 )				\
do {								\
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;	\
								\
   if (exec->vtx.active_sz[A] != N)				\
      vbo_exec_fixup_vertex(ctx, A, N);			\
								\
   {								\
      GLfloat *dest = exec->vtx.attrptr[A];			\
      if (N>0) dest[0] = V0;					\
      if (N>1) dest[1] = V1;					\
      if (N>2) dest[2] = V2;					\
      if (N>3) dest[3] = V3;					\
   }								\
								\
   if ((A) == 0) {						\
      GLuint i;							\
								\
      for (i = 0; i < exec->vtx.vertex_size; i++)		\
	 exec->vtx.vbptr[i] = exec->vtx.vertex[i];		\
								\
      exec->vtx.vbptr += exec->vtx.vertex_size;			\
      exec->ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;	\
								\
      if (++exec->vtx.vert_count >= exec->vtx.max_vert)		\
	 vbo_exec_vtx_wrap( exec );				\
   }								\
} while (0)


#define ERROR() _mesa_error( ctx, GL_INVALID_ENUM, __FUNCTION__ )
#define TAG(x) vbo_##x

#include "vbo_attrib_tmp.h"





/* Eval
 */
static void GLAPIENTRY vbo_exec_EvalCoord1f( GLfloat u )
{
   GET_CURRENT_CONTEXT( ctx );
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   {
      GLint i;
      if (exec->eval.recalculate_maps) 
	 vbo_exec_eval_update( exec );

      for (i = 0; i <= VBO_ATTRIB_TEX7; i++) {
	 if (exec->eval.map1[i].map) 
	    if (exec->vtx.active_sz[i] != exec->eval.map1[i].sz)
	       vbo_exec_fixup_vertex( ctx, i, exec->eval.map1[i].sz );
      }
   }


   _mesa_memcpy( exec->vtx.copied.buffer, exec->vtx.vertex, 
                 exec->vtx.vertex_size * sizeof(GLfloat));

   vbo_exec_do_EvalCoord1f( exec, u );

   _mesa_memcpy( exec->vtx.vertex, exec->vtx.copied.buffer,
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

      for (i = 0; i <= VBO_ATTRIB_TEX7; i++) {
	 if (exec->eval.map2[i].map) 
	    if (exec->vtx.active_sz[i] != exec->eval.map2[i].sz)
	       vbo_exec_fixup_vertex( ctx, i, exec->eval.map2[i].sz );
      }

      if (ctx->Eval.AutoNormal) 
	 if (exec->vtx.active_sz[VBO_ATTRIB_NORMAL] != 3)
	    vbo_exec_fixup_vertex( ctx, VBO_ATTRIB_NORMAL, 3 );
   }

   _mesa_memcpy( exec->vtx.copied.buffer, exec->vtx.vertex, 
                 exec->vtx.vertex_size * sizeof(GLfloat));

   vbo_exec_do_EvalCoord2f( exec, u, v );

   _mesa_memcpy( exec->vtx.vertex, exec->vtx.copied.buffer, 
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


/**
 * Check if programs/shaders are enabled and valid at glBegin time.
 */
GLboolean 
vbo_validate_shaders(GLcontext *ctx)
{
   if ((ctx->VertexProgram.Enabled && !ctx->VertexProgram._Enabled) ||
       (ctx->FragmentProgram.Enabled && !ctx->FragmentProgram._Enabled)) {
      return GL_FALSE;
   }
   if (ctx->Shader.CurrentProgram && !ctx->Shader.CurrentProgram->LinkStatus) {
      return GL_FALSE;
   }
   return GL_TRUE;
}


/* Build a list of primitives on the fly.  Keep
 * ctx->Driver.CurrentExecPrimitive uptodate as well.
 */
static void GLAPIENTRY vbo_exec_Begin( GLenum mode )
{
   GET_CURRENT_CONTEXT( ctx ); 

   if (ctx->Driver.CurrentExecPrimitive == PRIM_OUTSIDE_BEGIN_END) {
      struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
      int i;

      if (ctx->NewState) {
	 _mesa_update_state( ctx );

	 CALL_Begin(ctx->Exec, (mode));
	 return;
      }

      if (!vbo_validate_shaders(ctx)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBegin (invalid vertex/fragment program)");
         return;
      }

      /* Heuristic: attempt to isolate attributes occuring outside
       * begin/end pairs.
       */
      if (exec->vtx.vertex_size && !exec->vtx.attrsz[0]) 
	 vbo_exec_FlushVertices( ctx, ~0 );

      i = exec->vtx.prim_count++;
      exec->vtx.prim[i].mode = mode;
      exec->vtx.prim[i].begin = 1;
      exec->vtx.prim[i].end = 0;
      exec->vtx.prim[i].indexed = 0;
      exec->vtx.prim[i].weak = 0;
      exec->vtx.prim[i].pad = 0;
      exec->vtx.prim[i].start = exec->vtx.vert_count;
      exec->vtx.prim[i].count = 0;

      ctx->Driver.CurrentExecPrimitive = mode;
   }
   else 
      _mesa_error( ctx, GL_INVALID_OPERATION, "glBegin" );
      
}

static void GLAPIENTRY vbo_exec_End( void )
{
   GET_CURRENT_CONTEXT( ctx ); 

   if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END) {
      struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
      int idx = exec->vtx.vert_count;
      int i = exec->vtx.prim_count - 1;

      exec->vtx.prim[i].end = 1; 
      exec->vtx.prim[i].count = idx - exec->vtx.prim[i].start;

      ctx->Driver.CurrentExecPrimitive = PRIM_OUTSIDE_BEGIN_END;

      if (exec->vtx.prim_count == VBO_MAX_PRIM)
	 vbo_exec_vtx_flush( exec );	
   }
   else 
      _mesa_error( ctx, GL_INVALID_OPERATION, "glEnd" );
}


static void vbo_exec_vtxfmt_init( struct vbo_exec_context *exec )
{
   GLvertexformat *vfmt = &exec->vtxfmt;

   vfmt->ArrayElement = _ae_loopback_array_elt;	        /* generic helper */
   vfmt->Begin = vbo_exec_Begin;
#if FEATURE_dlist
   vfmt->CallList = _mesa_CallList;
   vfmt->CallLists = _mesa_CallLists;
#endif
   vfmt->End = vbo_exec_End;
   vfmt->EvalCoord1f = vbo_exec_EvalCoord1f;
   vfmt->EvalCoord1fv = vbo_exec_EvalCoord1fv;
   vfmt->EvalCoord2f = vbo_exec_EvalCoord2f;
   vfmt->EvalCoord2fv = vbo_exec_EvalCoord2fv;
   vfmt->EvalPoint1 = vbo_exec_EvalPoint1;
   vfmt->EvalPoint2 = vbo_exec_EvalPoint2;

   vfmt->Rectf = _mesa_noop_Rectf;
   vfmt->EvalMesh1 = _mesa_noop_EvalMesh1;
   vfmt->EvalMesh2 = _mesa_noop_EvalMesh2;


   /* from attrib_tmp.h:
    */
   vfmt->Color3f = vbo_Color3f;
   vfmt->Color3fv = vbo_Color3fv;
   vfmt->Color4f = vbo_Color4f;
   vfmt->Color4fv = vbo_Color4fv;
   vfmt->FogCoordfEXT = vbo_FogCoordfEXT;
   vfmt->FogCoordfvEXT = vbo_FogCoordfvEXT;
   vfmt->MultiTexCoord1fARB = vbo_MultiTexCoord1f;
   vfmt->MultiTexCoord1fvARB = vbo_MultiTexCoord1fv;
   vfmt->MultiTexCoord2fARB = vbo_MultiTexCoord2f;
   vfmt->MultiTexCoord2fvARB = vbo_MultiTexCoord2fv;
   vfmt->MultiTexCoord3fARB = vbo_MultiTexCoord3f;
   vfmt->MultiTexCoord3fvARB = vbo_MultiTexCoord3fv;
   vfmt->MultiTexCoord4fARB = vbo_MultiTexCoord4f;
   vfmt->MultiTexCoord4fvARB = vbo_MultiTexCoord4fv;
   vfmt->Normal3f = vbo_Normal3f;
   vfmt->Normal3fv = vbo_Normal3fv;
   vfmt->SecondaryColor3fEXT = vbo_SecondaryColor3fEXT;
   vfmt->SecondaryColor3fvEXT = vbo_SecondaryColor3fvEXT;
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
   
   vfmt->VertexAttrib1fARB = vbo_VertexAttrib1fARB;
   vfmt->VertexAttrib1fvARB = vbo_VertexAttrib1fvARB;
   vfmt->VertexAttrib2fARB = vbo_VertexAttrib2fARB;
   vfmt->VertexAttrib2fvARB = vbo_VertexAttrib2fvARB;
   vfmt->VertexAttrib3fARB = vbo_VertexAttrib3fARB;
   vfmt->VertexAttrib3fvARB = vbo_VertexAttrib3fvARB;
   vfmt->VertexAttrib4fARB = vbo_VertexAttrib4fARB;
   vfmt->VertexAttrib4fvARB = vbo_VertexAttrib4fvARB;

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


/**
 * Tell the VBO module to use a real OpenGL vertex buffer object to
 * store accumulated immediate-mode vertex data.
 * This replaces the malloced buffer which was created in
 * vb_exec_vtx_init() below.
 */
void vbo_use_buffer_objects(GLcontext *ctx)
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
   /* Any buffer name but 0 can be used here since this bufferobj won't
    * go into the bufferobj hashtable.
    */
   GLuint bufName = 0xaabbccdd;
   GLenum target = GL_ARRAY_BUFFER_ARB;
   GLenum access = GL_READ_WRITE_ARB;
   GLenum usage = GL_STREAM_DRAW_ARB;
   GLsizei size = VBO_VERT_BUFFER_SIZE * sizeof(GLfloat);

   /* Make sure this func is only used once */
   assert(exec->vtx.bufferobj == ctx->Array.NullBufferObj);
   if (exec->vtx.buffer_map) {
      _mesa_align_free(exec->vtx.buffer_map);
   }

   /* Allocate a real buffer object now */
   exec->vtx.bufferobj = ctx->Driver.NewBufferObject(ctx, bufName, target);
   ctx->Driver.BufferData(ctx, target, size, NULL, usage, exec->vtx.bufferobj);

   /* and map it */
   exec->vtx.buffer_map
      = ctx->Driver.MapBuffer(ctx, target, access, exec->vtx.bufferobj);
}



void vbo_exec_vtx_init( struct vbo_exec_context *exec )
{
   GLcontext *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   GLuint i;

   /* Allocate a buffer object.  Will just reuse this object
    * continuously.
    */
   _mesa_reference_buffer_object(ctx,
                                 &exec->vtx.bufferobj,
                                 ctx->Array.NullBufferObj);

   ASSERT(!exec->vtx.buffer_map);
   exec->vtx.buffer_map = ALIGN_MALLOC(VBO_VERT_BUFFER_SIZE * sizeof(GLfloat), 64);
   vbo_exec_vtxfmt_init( exec );

   /* Hook our functions into the dispatch table.
    */
   _mesa_install_exec_vtxfmt( exec->ctx, &exec->vtxfmt );

   for (i = 0 ; i < VBO_ATTRIB_MAX ; i++) {
      exec->vtx.attrsz[i] = 0;
      exec->vtx.active_sz[i] = 0;
      exec->vtx.inputs[i] = &exec->vtx.arrays[i];
   }
   
   {
      struct gl_client_array *arrays = exec->vtx.arrays;
      memcpy(arrays,      vbo->legacy_currval,  16 * sizeof(arrays[0]));
      memcpy(arrays + 16, vbo->generic_currval, 16 * sizeof(arrays[0]));
   }

   exec->vtx.vertex_size = 0;
}


void vbo_exec_vtx_destroy( struct vbo_exec_context *exec )
{
   if (exec->vtx.bufferobj->Name) {
      /* using a real VBO for vertex data */
      GLcontext *ctx = exec->ctx;
      _mesa_reference_buffer_object(ctx, &exec->vtx.bufferobj, NULL);
   }
   else {
      /* just using malloc'd space for vertex data */
      if (exec->vtx.buffer_map) {
         ALIGN_FREE(exec->vtx.buffer_map);
         exec->vtx.buffer_map = NULL;
      }
   }
}


void vbo_exec_FlushVertices( GLcontext *ctx, GLuint flags )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   if (exec->ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END)
      return;

   if (exec->vtx.vert_count) {
      vbo_exec_vtx_flush( exec );
   }

   if (exec->vtx.vertex_size) {
      vbo_exec_copy_to_current( exec );
      reset_attrfv( exec );
   }

   exec->ctx->Driver.NeedFlush = 0;
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
      
