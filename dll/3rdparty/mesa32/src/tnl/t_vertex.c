/*
 * Copyright 2003 Tungsten Graphics, inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@tungstengraphics.com>
 */

#include "glheader.h"
#include "context.h"
#include "colormac.h"

#include "t_context.h"
#include "t_vertex.h"

#define DBG 0

/* Build and manage clipspace/ndc/window vertices.
 */

static GLboolean match_fastpath( struct tnl_clipspace *vtx,
				 const struct tnl_clipspace_fastpath *fp)
{
   GLuint j;

   if (vtx->attr_count != fp->attr_count) 
      return GL_FALSE;

   for (j = 0; j < vtx->attr_count; j++) 
      if (vtx->attr[j].format != fp->attr[j].format ||
	  vtx->attr[j].inputsize != fp->attr[j].size ||
	  vtx->attr[j].vertoffset != fp->attr[j].offset) 
	 return GL_FALSE;
      
   if (fp->match_strides) {
      if (vtx->vertex_size != fp->vertex_size)
	 return GL_FALSE;

      for (j = 0; j < vtx->attr_count; j++) 
	 if (vtx->attr[j].inputstride != fp->attr[j].stride) 
	    return GL_FALSE;
   }
   
   return GL_TRUE;
}

static GLboolean search_fastpath_emit( struct tnl_clipspace *vtx )
{
   struct tnl_clipspace_fastpath *fp = vtx->fastpath;

   for ( ; fp ; fp = fp->next) {
      if (match_fastpath(vtx, fp)) {
         vtx->emit = fp->func;
	 return GL_TRUE;
      }
   }

   return GL_FALSE;
}

void _tnl_register_fastpath( struct tnl_clipspace *vtx,
			     GLboolean match_strides )
{
   struct tnl_clipspace_fastpath *fastpath = CALLOC_STRUCT(tnl_clipspace_fastpath);
   GLuint i;

   fastpath->vertex_size = vtx->vertex_size;
   fastpath->attr_count = vtx->attr_count;
   fastpath->match_strides = match_strides;
   fastpath->func = vtx->emit;
   fastpath->attr = (struct tnl_attr_type *)
      _mesa_malloc(vtx->attr_count * sizeof(fastpath->attr[0]));

   for (i = 0; i < vtx->attr_count; i++) {
      fastpath->attr[i].format = vtx->attr[i].format;
      fastpath->attr[i].stride = vtx->attr[i].inputstride;
      fastpath->attr[i].size = vtx->attr[i].inputsize;
      fastpath->attr[i].offset = vtx->attr[i].vertoffset;
   }

   fastpath->next = vtx->fastpath;
   vtx->fastpath = fastpath;
}



/***********************************************************************
 * Build codegen functions or return generic ones:
 */
static void choose_emit_func( GLcontext *ctx, GLuint count, GLubyte *dest)
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   struct tnl_clipspace_attr *a = vtx->attr;
   const GLuint attr_count = vtx->attr_count;
   GLuint j;

   for (j = 0; j < attr_count; j++) {
      GLvector4f *vptr = VB->AttribPtr[a[j].attrib];
      a[j].inputstride = vptr->stride;
      a[j].inputsize = vptr->size;
      a[j].emit = a[j].insert[vptr->size - 1]; /* not always used */
   }

   vtx->emit = NULL;
   
   /* Does this match an existing (hardwired, codegen or known-bad)
    * fastpath?
    */
   if (search_fastpath_emit(vtx)) {
      /* Use this result.  If it is null, then it is already known
       * that the current state will fail for codegen and there is no
       * point trying again.
       */
   }
   else if (vtx->codegen_emit) {
      vtx->codegen_emit(ctx);
   }

   if (!vtx->emit) {
      _tnl_generate_hardwired_emit(ctx);
   }

   /* Otherwise use the generic version:
    */
   if (!vtx->emit)
      vtx->emit = _tnl_generic_emit;

   vtx->emit( ctx, count, dest );
}



static void choose_interp_func( GLcontext *ctx,
				GLfloat t,
				GLuint edst, GLuint eout, GLuint ein,
				GLboolean force_boundary )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);

   if (vtx->need_extras && 
       (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
      vtx->interp = _tnl_generic_interp_extras;
   } else {
      vtx->interp = _tnl_generic_interp;
   }

   vtx->interp( ctx, t, edst, eout, ein, force_boundary );
}


static void choose_copy_pv_func(  GLcontext *ctx, GLuint edst, GLuint esrc )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);

   if (vtx->need_extras && 
       (ctx->_TriangleCaps & (DD_TRI_LIGHT_TWOSIDE|DD_TRI_UNFILLED))) {
      vtx->copy_pv = _tnl_generic_copy_pv_extras;
   } else {
      vtx->copy_pv = _tnl_generic_copy_pv;
   }

   vtx->copy_pv( ctx, edst, esrc );
}


/***********************************************************************
 * Public entrypoints, mostly dispatch to the above:
 */


/* Interpolate between two vertices to produce a third:
 */
void _tnl_interp( GLcontext *ctx,
		  GLfloat t,
		  GLuint edst, GLuint eout, GLuint ein,
		  GLboolean force_boundary )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   vtx->interp( ctx, t, edst, eout, ein, force_boundary );
}

/* Copy colors from one vertex to another:
 */
void _tnl_copy_pv(  GLcontext *ctx, GLuint edst, GLuint esrc )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   vtx->copy_pv( ctx, edst, esrc );
}


/* Extract a named attribute from a hardware vertex.  Will have to
 * reverse any viewport transformation, swizzling or other conversions
 * which may have been applied:
 */
void _tnl_get_attr( GLcontext *ctx, const void *vin,
			      GLenum attr, GLfloat *dest )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   const struct tnl_clipspace_attr *a = vtx->attr;
   const GLuint attr_count = vtx->attr_count;
   GLuint j;

   for (j = 0; j < attr_count; j++) {
      if (a[j].attrib == attr) {
	 a[j].extract( &a[j], dest, (GLubyte *)vin + a[j].vertoffset );
	 return;
      }
   }

   /* Else return the value from ctx->Current.
    */
   if (attr == _TNL_ATTRIB_POINTSIZE) {
      /* If the hardware vertex doesn't have point size then use size from
       * GLcontext.  XXX this will be wrong if drawing attenuated points!
       */
      dest[0] = ctx->Point.Size;
   }
   else {
      _mesa_memcpy( dest, ctx->Current.Attrib[attr], 4*sizeof(GLfloat));
   }
}


/* Complementary operation to the above.
 */
void _tnl_set_attr( GLcontext *ctx, void *vout,
		    GLenum attr, const GLfloat *src )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   const struct tnl_clipspace_attr *a = vtx->attr;
   const GLuint attr_count = vtx->attr_count;
   GLuint j;

   for (j = 0; j < attr_count; j++) {
      if (a[j].attrib == attr) {
	 a[j].insert[4-1]( &a[j], (GLubyte *)vout + a[j].vertoffset, src );
	 return;
      }
   }
}


void *_tnl_get_vertex( GLcontext *ctx, GLuint nr )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);

   return vtx->vertex_buf + nr * vtx->vertex_size;
}

void _tnl_invalidate_vertex_state( GLcontext *ctx, GLuint new_state )
{
   if (new_state & (_DD_NEW_TRI_LIGHT_TWOSIDE|_DD_NEW_TRI_UNFILLED) ) {
      struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
      vtx->new_inputs = ~0;
      vtx->interp = choose_interp_func;
      vtx->copy_pv = choose_copy_pv_func;
   }
}

static void invalidate_funcs( struct tnl_clipspace *vtx )
{
   vtx->emit = choose_emit_func;
   vtx->interp = choose_interp_func;
   vtx->copy_pv = choose_copy_pv_func;
   vtx->new_inputs = ~0;
}

GLuint _tnl_install_attrs( GLcontext *ctx, const struct tnl_attr_map *map,
			   GLuint nr, const GLfloat *vp, 
			   GLuint unpacked_size )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   GLuint offset = 0;
   GLuint i, j;

   assert(nr < _TNL_ATTRIB_MAX);
   assert(nr == 0 || map[0].attrib == VERT_ATTRIB_POS);

   vtx->new_inputs = ~0;
   vtx->need_viewport = GL_FALSE;

   if (vp) {
      vtx->need_viewport = GL_TRUE;
   }

   for (j = 0, i = 0; i < nr; i++) {
      const GLuint format = map[i].format;
      if (format == EMIT_PAD) {
	 if (DBG)
	    _mesa_printf("%d: pad %d, offset %d\n", i,  
			 map[i].offset, offset);  

	 offset += map[i].offset;

      }
      else {
	 GLuint tmpoffset;

	 if (unpacked_size) 
	    tmpoffset = map[i].offset;
	 else
	    tmpoffset = offset;

	 if (vtx->attr_count != j ||
	     vtx->attr[j].attrib != map[i].attrib ||
	     vtx->attr[j].format != format ||
	     vtx->attr[j].vertoffset != tmpoffset) {
	    invalidate_funcs(vtx);

	    vtx->attr[j].attrib = map[i].attrib;
	    vtx->attr[j].format = format;
	    vtx->attr[j].vp = vp;
	    vtx->attr[j].insert = _tnl_format_info[format].insert;
	    vtx->attr[j].extract = _tnl_format_info[format].extract;
	    vtx->attr[j].vertattrsize = _tnl_format_info[format].attrsize;
	    vtx->attr[j].vertoffset = tmpoffset;
	 }

	 
	 if (DBG)
	    _mesa_printf("%d: %s, vp %p, offset %d\n", i,  
			 _tnl_format_info[format].name, (void *)vp,
			 vtx->attr[j].vertoffset);   

	 offset += _tnl_format_info[format].attrsize;
	 j++;
      }
   }

   vtx->attr_count = j;

   if (unpacked_size)
      vtx->vertex_size = unpacked_size;
   else
      vtx->vertex_size = offset;

   assert(vtx->vertex_size <= vtx->max_vertex_size);
   return vtx->vertex_size;
}



void _tnl_invalidate_vertices( GLcontext *ctx, GLuint newinputs )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   vtx->new_inputs |= newinputs;
}


/* This event has broader use beyond this file - will move elsewhere
 * and probably invoke a driver callback.
 */
void _tnl_notify_pipeline_output_change( GLcontext *ctx )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   invalidate_funcs(vtx);
}

static void update_input_ptrs( GLcontext *ctx, GLuint start )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   struct tnl_clipspace_attr *a = vtx->attr;
   const GLuint count = vtx->attr_count;
   GLuint j;
   
   for (j = 0; j < count; j++) {
      GLvector4f *vptr = VB->AttribPtr[a[j].attrib];

      if (vtx->emit != choose_emit_func) {
	 assert(a[j].inputstride == vptr->stride);
	 assert(a[j].inputsize == vptr->size);
      }

      a[j].inputptr = ((GLubyte *)vptr->data) + start * vptr->stride;
   }
   
   if (a->vp) {
      vtx->vp_scale[0] = a->vp[MAT_SX];
      vtx->vp_scale[1] = a->vp[MAT_SY];
      vtx->vp_scale[2] = a->vp[MAT_SZ];
      vtx->vp_scale[3] = 1.0;
      vtx->vp_xlate[0] = a->vp[MAT_TX];
      vtx->vp_xlate[1] = a->vp[MAT_TY];
      vtx->vp_xlate[2] = a->vp[MAT_TZ];
      vtx->vp_xlate[3] = 0.0;
   }
}


void _tnl_build_vertices( GLcontext *ctx,
			  GLuint start,
			  GLuint end,
			  GLuint newinputs )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);  
   update_input_ptrs( ctx, start );      
   vtx->emit( ctx, end - start, 
	      (GLubyte *)(vtx->vertex_buf + 
			  start * vtx->vertex_size));
}

/* Emit VB vertices start..end to dest.  Note that VB vertex at
 * postion start will be emitted to dest at position zero.
 */
void *_tnl_emit_vertices_to_buffer( GLcontext *ctx,
				    GLuint start,
				    GLuint end,
				    void *dest )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);

   update_input_ptrs(ctx, start);

   /* Note: dest should not be adjusted for non-zero 'start' values:
    */
   vtx->emit( ctx, end - start, (GLubyte*) dest );	
   return (void *)((GLubyte *)dest + vtx->vertex_size * (end - start));
}


void _tnl_init_vertices( GLcontext *ctx, 
			GLuint vb_size,
			GLuint max_vertex_size )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);  

   _tnl_install_attrs( ctx, NULL, 0, NULL, 0 );

   vtx->need_extras = GL_TRUE;
   if (max_vertex_size > vtx->max_vertex_size) {
      _tnl_free_vertices( ctx );
      vtx->max_vertex_size = max_vertex_size;
      vtx->vertex_buf = (GLubyte *)ALIGN_CALLOC(vb_size * max_vertex_size, 32 );
      invalidate_funcs(vtx);
   }

   switch(CHAN_TYPE) {
   case GL_UNSIGNED_BYTE:
      vtx->chan_scale[0] = 255.0;
      vtx->chan_scale[1] = 255.0;
      vtx->chan_scale[2] = 255.0;
      vtx->chan_scale[3] = 255.0;
      break;
   case GL_UNSIGNED_SHORT:
      vtx->chan_scale[0] = 65535.0;
      vtx->chan_scale[1] = 65535.0;
      vtx->chan_scale[2] = 65535.0;
      vtx->chan_scale[3] = 65535.0;
      break;
   default:
      vtx->chan_scale[0] = 1.0;
      vtx->chan_scale[1] = 1.0;
      vtx->chan_scale[2] = 1.0;
      vtx->chan_scale[3] = 1.0;
      break;
   }

   vtx->identity[0] = 0.0;
   vtx->identity[1] = 0.0;
   vtx->identity[2] = 0.0;
   vtx->identity[3] = 1.0;

   vtx->codegen_emit = NULL;

#ifdef USE_SSE_ASM
   if (!_mesa_getenv("MESA_NO_CODEGEN"))
      vtx->codegen_emit = _tnl_generate_sse_emit;
#endif
}


void _tnl_free_vertices( GLcontext *ctx )
{
   struct tnl_clipspace *vtx = GET_VERTEX_STATE(ctx);
   struct tnl_clipspace_fastpath *fp, *tmp;

   if (vtx->vertex_buf) {
      ALIGN_FREE(vtx->vertex_buf);
      vtx->vertex_buf = NULL;
   }
   
   for (fp = vtx->fastpath ; fp ; fp = tmp) {
      tmp = fp->next;
      FREE(fp->attr);

      /* KW: At the moment, fp->func is constrained to be allocated by
       * _mesa_exec_alloc(), as the hardwired fastpaths in
       * t_vertex_generic.c are handled specially.  It would be nice
       * to unify them, but this probably won't change until this
       * module gets another overhaul.
       */
      _mesa_exec_free((void *) fp->func);
      FREE(fp);
   }
   
   vtx->fastpath = NULL;
}
