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

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "mtypes.h"
#include "macros.h"
#include "light.h"
#include "state.h"
#include "t_pipeline.h"
#include "t_save_api.h"
#include "t_vtx_api.h"

static INLINE GLint get_size( const GLfloat *f )
{
   if (f[3] != 1.0) return 4;
   if (f[2] != 0.0) return 3;
   return 2;
}


/* Some nasty stuff still hanging on here.  
 *
 * TODO - remove VB->ColorPtr, etc and just use the AttrPtr's.
 */
static void _tnl_bind_vertex_list( GLcontext *ctx,
                                   const struct tnl_vertex_list *node )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   struct tnl_vertex_arrays *tmp = &tnl->save_inputs;
   GLfloat *data = node->buffer;
   GLuint attr, i;

   /* Setup constant data in the VB.
    */
   VB->Count = node->count;
   VB->Primitive = node->prim;
   VB->PrimitiveCount = node->prim_count;
   VB->Elts = NULL;
   VB->NormalLengthPtr = node->normal_lengths;

   for (attr = 0; attr <= _TNL_ATTRIB_INDEX; attr++) {
      if (node->attrsz[attr]) {
	 tmp->Attribs[attr].count = node->count;
	 tmp->Attribs[attr].data = (GLfloat (*)[4]) data;
	 tmp->Attribs[attr].start = data;
	 tmp->Attribs[attr].size = node->attrsz[attr];
	 tmp->Attribs[attr].stride = node->vertex_size * sizeof(GLfloat);
	 VB->AttribPtr[attr] = &tmp->Attribs[attr];
	 data += node->attrsz[attr];
      }
      else {
	 tmp->Attribs[attr].count = 1;
	 tmp->Attribs[attr].data = (GLfloat (*)[4]) tnl->vtx.current[attr];
	 tmp->Attribs[attr].start = tnl->vtx.current[attr];
	 tmp->Attribs[attr].size = get_size( tnl->vtx.current[attr] );
	 tmp->Attribs[attr].stride = 0;
	 VB->AttribPtr[attr] = &tmp->Attribs[attr];
      }
   }

   
   /* Copy edgeflag to a contiguous array
    */
   if (ctx->Polygon.FrontMode != GL_FILL || ctx->Polygon.BackMode != GL_FILL) {
      if (node->attrsz[_TNL_ATTRIB_EDGEFLAG]) {
	 VB->EdgeFlag = _tnl_translate_edgeflag( ctx, data, 
						 node->count,
						 node->vertex_size );
	 data++;
      }
      else 
	 VB->EdgeFlag = _tnl_import_current_edgeflag( ctx, node->count );
   }

   /* Legacy pointers -- remove one day.
    */
   VB->ObjPtr = VB->AttribPtr[_TNL_ATTRIB_POS];
   VB->NormalPtr = VB->AttribPtr[_TNL_ATTRIB_NORMAL];
   VB->ColorPtr[0] = VB->AttribPtr[_TNL_ATTRIB_COLOR0];
   VB->ColorPtr[1] = NULL;
   VB->IndexPtr[0] = VB->AttribPtr[_TNL_ATTRIB_INDEX];
   VB->IndexPtr[1] = NULL;
   VB->SecondaryColorPtr[0] = VB->AttribPtr[_TNL_ATTRIB_COLOR1];
   VB->SecondaryColorPtr[1] = NULL;
   VB->FogCoordPtr = VB->AttribPtr[_TNL_ATTRIB_FOG];

   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      VB->TexCoordPtr[i] = VB->AttribPtr[_TNL_ATTRIB_TEX0 + i];
   }
}

static void _playback_copy_to_current( GLcontext *ctx,
				       const struct tnl_vertex_list *node )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   const GLfloat *data;
   GLuint i;

   if (node->count)
      data = node->buffer + (node->count-1) * node->vertex_size;
   else
      data = node->buffer;

   for (i = _TNL_ATTRIB_POS+1 ; i <= _TNL_ATTRIB_INDEX ; i++) {
      if (node->attrsz[i]) {
	 COPY_CLEAN_4V(tnl->vtx.current[i], node->attrsz[i], data);
	 data += node->attrsz[i];
      }
   }

   /* Edgeflag requires special treatment:
    */
   if (node->attrsz[_TNL_ATTRIB_EDGEFLAG]) {
      ctx->Current.EdgeFlag = (data[0] == 1.0);
   }

   /* Colormaterial -- this kindof sucks.
    */
   if (ctx->Light.ColorMaterialEnabled) {
      _mesa_update_color_material(ctx, ctx->Current.Attrib[VERT_ATTRIB_COLOR0]);
   }

   if (node->have_materials) {
      tnl->Driver.NotifyMaterialChange( ctx );
   }

   /* CurrentExecPrimitive
    */
   if (node->prim_count) {
      GLenum mode = node->prim[node->prim_count - 1].mode;
      if (mode & PRIM_END)
	 ctx->Driver.CurrentExecPrimitive = PRIM_OUTSIDE_BEGIN_END;
      else
	 ctx->Driver.CurrentExecPrimitive = (mode & PRIM_MODE_MASK);
   }
}


/**
 * Execute the buffer and save copied verts.
 */
void _tnl_playback_vertex_list( GLcontext *ctx, void *data )
{
   const struct tnl_vertex_list *node = (const struct tnl_vertex_list *) data;
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   FLUSH_CURRENT(ctx, 0);

   if (node->prim_count > 0 && node->count > 0) {

      if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END &&
	  (node->prim[0].mode & PRIM_BEGIN)) {

	 /* Degenerate case: list is called inside begin/end pair and
	  * includes operations such as glBegin or glDrawArrays.
	  */
	 _mesa_error( ctx, GL_INVALID_OPERATION, "displaylist recursive begin");
	 _tnl_loopback_vertex_list( ctx, node );
	 return;
      }
      else if (tnl->save.replay_flags) {
	 /* Various degnerate cases: translate into immediate mode
	  * calls rather than trying to execute in place.
	  */
	 _tnl_loopback_vertex_list( ctx, node );
	 return;
      }
      
      if (ctx->NewState)
	 _mesa_update_state( ctx );

      if ((ctx->VertexProgram.Enabled && !ctx->VertexProgram._Enabled) ||
          (ctx->FragmentProgram.Enabled && !ctx->FragmentProgram._Enabled)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBegin (invalid vertex/fragment program)");
         return;
      }

      _tnl_bind_vertex_list( ctx, node );

      tnl->Driver.RunPipeline( ctx );
   }

   /* Copy to current?
    */
   _playback_copy_to_current( ctx, node );
}
