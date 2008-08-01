/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   frame buffer texture by Gary Wong <gtw@gnu.org>
  */
 


#include "glheader.h"
#include "context.h"
#include "macros.h"

#include "shader/arbprogparse.h"

#include "intel_screen.h"
#include "intel_batchbuffer.h"
#include "intel_regions.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_draw.h"
#include "brw_fallback.h"

#define INIT(brw, STRUCT, ATTRIB) 		\
do {						\
   brw->attribs.ATTRIB = &ctx->ATTRIB;		\
} while (0)

#define DUP(brw, STRUCT, ATTRIB) 		\
do {						\
   brw->metaops.attribs.ATTRIB = MALLOC_STRUCT(STRUCT);	\
   memcpy(brw->metaops.attribs.ATTRIB, 			\
	  brw->attribs.ATTRIB,			\
	  sizeof(struct STRUCT));		\
} while (0)


#define INSTALL(brw, ATTRIB, STATE)		\
do {						\
   brw->attribs.ATTRIB = brw->metaops.attribs.ATTRIB;	\
   brw->state.dirty.mesa |= STATE;		\
} while (0)

#define RESTORE(brw, ATTRIB, STATE)			\
do {							\
   brw->attribs.ATTRIB = &brw->intel.ctx.ATTRIB;	\
   brw->state.dirty.mesa |= STATE;			\
} while (0)

static void init_attribs( struct brw_context *brw )
{
   DUP(brw, gl_colorbuffer_attrib, Color);
   DUP(brw, gl_depthbuffer_attrib, Depth);
   DUP(brw, gl_fog_attrib, Fog);
   DUP(brw, gl_hint_attrib, Hint);
   DUP(brw, gl_light_attrib, Light);
   DUP(brw, gl_line_attrib, Line);
   DUP(brw, gl_point_attrib, Point);
   DUP(brw, gl_polygon_attrib, Polygon);
   DUP(brw, gl_scissor_attrib, Scissor);
   DUP(brw, gl_stencil_attrib, Stencil);
   DUP(brw, gl_texture_attrib, Texture);
   DUP(brw, gl_transform_attrib, Transform);
   DUP(brw, gl_viewport_attrib, Viewport);
   DUP(brw, gl_vertex_program_state, VertexProgram);
   DUP(brw, gl_fragment_program_state, FragmentProgram);
}

static void install_attribs( struct brw_context *brw )
{
   INSTALL(brw, Color, _NEW_COLOR);
   INSTALL(brw, Depth, _NEW_DEPTH);
   INSTALL(brw, Fog, _NEW_FOG);
   INSTALL(brw, Hint, _NEW_HINT);
   INSTALL(brw, Light, _NEW_LIGHT);
   INSTALL(brw, Line, _NEW_LINE);
   INSTALL(brw, Point, _NEW_POINT);
   INSTALL(brw, Polygon, _NEW_POLYGON);
   INSTALL(brw, Scissor, _NEW_SCISSOR);
   INSTALL(brw, Stencil, _NEW_STENCIL);
   INSTALL(brw, Texture, _NEW_TEXTURE);
   INSTALL(brw, Transform, _NEW_TRANSFORM);
   INSTALL(brw, Viewport, _NEW_VIEWPORT);
   INSTALL(brw, VertexProgram, _NEW_PROGRAM);
   INSTALL(brw, FragmentProgram, _NEW_PROGRAM);
}

static void restore_attribs( struct brw_context *brw )
{
   RESTORE(brw, Color, _NEW_COLOR);
   RESTORE(brw, Depth, _NEW_DEPTH);
   RESTORE(brw, Fog, _NEW_FOG);
   RESTORE(brw, Hint, _NEW_HINT);
   RESTORE(brw, Light, _NEW_LIGHT);
   RESTORE(brw, Line, _NEW_LINE);
   RESTORE(brw, Point, _NEW_POINT);
   RESTORE(brw, Polygon, _NEW_POLYGON);
   RESTORE(brw, Scissor, _NEW_SCISSOR);
   RESTORE(brw, Stencil, _NEW_STENCIL);
   RESTORE(brw, Texture, _NEW_TEXTURE);
   RESTORE(brw, Transform, _NEW_TRANSFORM);
   RESTORE(brw, Viewport, _NEW_VIEWPORT);
   RESTORE(brw, VertexProgram, _NEW_PROGRAM);
   RESTORE(brw, FragmentProgram, _NEW_PROGRAM);
}


static const char *vp_prog =
      "!!ARBvp1.0\n"
      "MOV  result.color, vertex.color;\n"
      "MOV  result.position, vertex.position;\n"
      "END\n";

static const char *fp_prog =
      "!!ARBfp1.0\n"
      "MOV result.color, fragment.color;\n"
      "END\n";

static const char *fp_tex_prog =
      "!!ARBfp1.0\n"
      "TEMP a;\n"
      "ADD a, fragment.position, program.local[0];\n"
      "MUL a, a, program.local[1];\n"
      "TEX result.color, a, texture[0], 2D;\n"
      "MOV result.depth.z, fragment.position;\n"
      "END\n";

/* Derived values of importance:
 *
 *   FragmentProgram->_Current
 *   VertexProgram->_Enabled
 *   brw->vertex_program
 *   DrawBuffer->_ColorDrawBufferMask[0]
 * 
 *
 * More if drawpixels-through-texture is added.  
 */
static void init_metaops_state( struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;

   brw->metaops.vbo = ctx->Driver.NewBufferObject(ctx, 1, GL_ARRAY_BUFFER_ARB);

   ctx->Driver.BufferData(ctx,
			  GL_ARRAY_BUFFER_ARB,
			  4096,
			  NULL,
			  GL_DYNAMIC_DRAW_ARB,
			  brw->metaops.vbo);

   brw->metaops.fp = (struct gl_fragment_program *)
      ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 1 );

   brw->metaops.fp_tex = (struct gl_fragment_program *)
      ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 1 );

   brw->metaops.vp = (struct gl_vertex_program *)
      ctx->Driver.NewProgram(ctx, GL_VERTEX_PROGRAM_ARB, 1 );

   _mesa_parse_arb_fragment_program(ctx, GL_FRAGMENT_PROGRAM_ARB, 
				    fp_prog, strlen(fp_prog),
				    brw->metaops.fp);

   _mesa_parse_arb_fragment_program(ctx, GL_FRAGMENT_PROGRAM_ARB, 
				    fp_tex_prog, strlen(fp_tex_prog),
				    brw->metaops.fp_tex);

   _mesa_parse_arb_vertex_program(ctx, GL_VERTEX_PROGRAM_ARB, 
				  vp_prog, strlen(vp_prog),
				  brw->metaops.vp);

   brw->metaops.attribs.VertexProgram->Current = brw->metaops.vp;
   brw->metaops.attribs.VertexProgram->_Enabled = GL_TRUE;

   brw->metaops.attribs.FragmentProgram->_Current = brw->metaops.fp;
}

static void meta_flat_shade( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   brw->metaops.attribs.Light->ShadeModel = GL_FLAT;
   brw->state.dirty.mesa |= _NEW_LIGHT;
}


static void meta_no_stencil_write( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   brw->metaops.attribs.Stencil->Enabled = GL_FALSE;
   brw->metaops.attribs.Stencil->WriteMask[0] = GL_FALSE; 
   brw->state.dirty.mesa |= _NEW_STENCIL;
}

static void meta_no_depth_write( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   brw->metaops.attribs.Depth->Test = GL_FALSE;
   brw->metaops.attribs.Depth->Mask = GL_FALSE;
   brw->state.dirty.mesa |= _NEW_DEPTH;
}


static void meta_depth_replace( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_DEPTH_TEST, GL_TRUE )
    * ctx->Driver.DepthMask( ctx, GL_TRUE )
    */
   brw->metaops.attribs.Depth->Test = GL_TRUE;
   brw->metaops.attribs.Depth->Mask = GL_TRUE;
   brw->state.dirty.mesa |= _NEW_DEPTH;

   /* ctx->Driver.DepthFunc( ctx, GL_ALWAYS )
    */
   brw->metaops.attribs.Depth->Func = GL_ALWAYS;

   brw->state.dirty.mesa |= _NEW_DEPTH;
}


static void meta_stencil_replace( struct intel_context *intel,
				 GLuint s_mask,
				 GLuint s_clear)
{
   struct brw_context *brw = brw_context(&intel->ctx);

   brw->metaops.attribs.Stencil->Enabled = GL_TRUE;
   brw->metaops.attribs.Stencil->WriteMask[0] = s_mask;
   brw->metaops.attribs.Stencil->ValueMask[0] = 0xff;
   brw->metaops.attribs.Stencil->Ref[0] = s_clear;
   brw->metaops.attribs.Stencil->Function[0] = GL_ALWAYS;
   brw->metaops.attribs.Stencil->FailFunc[0] = GL_REPLACE;
   brw->metaops.attribs.Stencil->ZPassFunc[0] = GL_REPLACE;
   brw->metaops.attribs.Stencil->ZFailFunc[0] = GL_REPLACE;
   brw->state.dirty.mesa |= _NEW_STENCIL;
}


static void meta_color_mask( struct intel_context *intel, GLboolean state )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   if (state)
      COPY_4V(brw->metaops.attribs.Color->ColorMask, 
	      brw->intel.ctx.Color.ColorMask); 
   else
      ASSIGN_4V(brw->metaops.attribs.Color->ColorMask, 0, 0, 0, 0);

   brw->state.dirty.mesa |= _NEW_COLOR;
}

static void meta_no_texture( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);
   
   brw->metaops.attribs.FragmentProgram->_Current = brw->metaops.fp;
   
   brw->metaops.attribs.Texture->CurrentUnit = 0;
   brw->metaops.attribs.Texture->_EnabledUnits = 0;
   brw->metaops.attribs.Texture->_EnabledCoordUnits = 0;
   brw->metaops.attribs.Texture->Unit[ 0 ].Enabled = 0;
   brw->metaops.attribs.Texture->Unit[ 0 ]._ReallyEnabled = 0;

   brw->state.dirty.mesa |= _NEW_TEXTURE | _NEW_PROGRAM;
}

static void meta_texture_blend_replace(struct intel_context *intel)
{
   struct brw_context *brw = brw_context(&intel->ctx);

   brw->metaops.attribs.Texture->CurrentUnit = 0;
   brw->metaops.attribs.Texture->_EnabledUnits = 1;
   brw->metaops.attribs.Texture->_EnabledCoordUnits = 1;
   brw->metaops.attribs.Texture->Unit[ 0 ].Enabled = TEXTURE_2D_BIT;
   brw->metaops.attribs.Texture->Unit[ 0 ]._ReallyEnabled = TEXTURE_2D_BIT;
   brw->metaops.attribs.Texture->Unit[ 0 ].Current2D =
      intel->frame_buffer_texobj;
   brw->metaops.attribs.Texture->Unit[ 0 ]._Current =
      intel->frame_buffer_texobj;

   brw->state.dirty.mesa |= _NEW_TEXTURE | _NEW_PROGRAM;
}

static void meta_import_pixel_state(struct intel_context *intel)
{
   struct brw_context *brw = brw_context(&intel->ctx);
   
   RESTORE(brw, Color, _NEW_COLOR);
   RESTORE(brw, Depth, _NEW_DEPTH);
   RESTORE(brw, Fog, _NEW_FOG);
   RESTORE(brw, Scissor, _NEW_SCISSOR);
   RESTORE(brw, Stencil, _NEW_STENCIL);
   RESTORE(brw, Texture, _NEW_TEXTURE);
   RESTORE(brw, FragmentProgram, _NEW_PROGRAM);
}

static void meta_frame_buffer_texture( struct intel_context *intel,
				       GLint xoff, GLint yoff )
{
   struct brw_context *brw = brw_context(&intel->ctx);
   struct intel_region *region = intel_drawbuf_region( intel );
   
   INSTALL(brw, FragmentProgram, _NEW_PROGRAM);

   brw->metaops.attribs.FragmentProgram->_Current = brw->metaops.fp_tex;
   /* This is unfortunate, but seems to be necessary, since later on we
      will end up calling _mesa_load_state_parameters to lookup the
      local params (below), and that will want to look in ctx.FragmentProgram
      instead of brw->attribs.FragmentProgram. */
   intel->ctx.FragmentProgram.Current = brw->metaops.fp_tex;

   brw->metaops.fp_tex->Base.LocalParams[ 0 ][ 0 ] = xoff;
   brw->metaops.fp_tex->Base.LocalParams[ 0 ][ 1 ] = yoff;
   brw->metaops.fp_tex->Base.LocalParams[ 0 ][ 2 ] = 0.0;
   brw->metaops.fp_tex->Base.LocalParams[ 0 ][ 3 ] = 0.0;
   brw->metaops.fp_tex->Base.LocalParams[ 1 ][ 0 ] =
      1.0 / region->pitch;
   brw->metaops.fp_tex->Base.LocalParams[ 1 ][ 1 ] =
      -1.0 / region->height;
   brw->metaops.fp_tex->Base.LocalParams[ 1 ][ 2 ] = 0.0;
   brw->metaops.fp_tex->Base.LocalParams[ 1 ][ 3 ] = 1.0;
   
   brw->state.dirty.mesa |= _NEW_PROGRAM;
}


static void meta_draw_region( struct intel_context *intel,
			     struct intel_region *draw_region,
			     struct intel_region *depth_region )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   if (!brw->metaops.saved_draw_region) {
      brw->metaops.saved_draw_region = brw->state.draw_region;
      brw->metaops.saved_depth_region = brw->state.depth_region;
   }

   brw->state.draw_region = draw_region;
   brw->state.depth_region = depth_region;

   brw->state.dirty.mesa |= _NEW_BUFFERS;
}


static void meta_draw_quad(struct intel_context *intel, 
			   GLfloat x0, GLfloat x1,
			   GLfloat y0, GLfloat y1, 
			   GLfloat z,
			   GLubyte red, GLubyte green,
			   GLubyte blue, GLubyte alpha,
			   GLfloat s0, GLfloat s1,
			   GLfloat t0, GLfloat t1)
{
   GLcontext *ctx = &intel->ctx;
   struct brw_context *brw = brw_context(&intel->ctx);
   struct gl_client_array pos_array;
   struct gl_client_array color_array;
   struct gl_client_array *attribs[VERT_ATTRIB_MAX];
   struct _mesa_prim prim[1];
   GLfloat pos[4][3];
   GLubyte color[4];

   ctx->Driver.BufferData(ctx,
			  GL_ARRAY_BUFFER_ARB,
			  sizeof(pos) + sizeof(color),
			  NULL,
			  GL_DYNAMIC_DRAW_ARB,
			  brw->metaops.vbo);

   pos[0][0] = x0;
   pos[0][1] = y0;
   pos[0][2] = z;

   pos[1][0] = x1;
   pos[1][1] = y0;
   pos[1][2] = z;

   pos[2][0] = x1;
   pos[2][1] = y1;
   pos[2][2] = z;

   pos[3][0] = x0;
   pos[3][1] = y1;
   pos[3][2] = z;


   ctx->Driver.BufferSubData(ctx,
			     GL_ARRAY_BUFFER_ARB,
			     0,
			     sizeof(pos),
			     pos,
			     brw->metaops.vbo);

   color[0] = red;
   color[1] = green;
   color[2] = blue;
   color[3] = alpha;

   ctx->Driver.BufferSubData(ctx,
			     GL_ARRAY_BUFFER_ARB,
			     sizeof(pos),
			     sizeof(color),
			     color,
			     brw->metaops.vbo);

   /* Ignoring texture coords. 
    */

   memset(attribs, 0, VERT_ATTRIB_MAX * sizeof(*attribs));

   attribs[VERT_ATTRIB_POS] = &pos_array;
   attribs[VERT_ATTRIB_POS]->Ptr = 0;
   attribs[VERT_ATTRIB_POS]->Type = GL_FLOAT;
   attribs[VERT_ATTRIB_POS]->Enabled = 1;
   attribs[VERT_ATTRIB_POS]->Size = 3;
   attribs[VERT_ATTRIB_POS]->StrideB = 3 * sizeof(GLfloat);
   attribs[VERT_ATTRIB_POS]->Stride = 3 * sizeof(GLfloat);
   attribs[VERT_ATTRIB_POS]->_MaxElement = 4;
   attribs[VERT_ATTRIB_POS]->Normalized = 0;
   attribs[VERT_ATTRIB_POS]->BufferObj = brw->metaops.vbo;

   attribs[VERT_ATTRIB_COLOR0] = &color_array;
   attribs[VERT_ATTRIB_COLOR0]->Ptr = (const GLubyte *)sizeof(pos);
   attribs[VERT_ATTRIB_COLOR0]->Type = GL_UNSIGNED_BYTE;
   attribs[VERT_ATTRIB_COLOR0]->Enabled = 1;
   attribs[VERT_ATTRIB_COLOR0]->Size = 4;
   attribs[VERT_ATTRIB_COLOR0]->StrideB = 0;
   attribs[VERT_ATTRIB_COLOR0]->Stride = 0;
   attribs[VERT_ATTRIB_COLOR0]->_MaxElement = 1;
   attribs[VERT_ATTRIB_COLOR0]->Normalized = 1;
   attribs[VERT_ATTRIB_COLOR0]->BufferObj = brw->metaops.vbo;
   
   /* Just ignoring texture coordinates for now. 
    */

   memset(prim, 0, sizeof(*prim));

   prim[0].mode = GL_TRIANGLE_FAN;
   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].weak = 0;
   prim[0].pad = 0;
   prim[0].start = 0;
   prim[0].count = 4;

   brw_draw_prims(&brw->intel.ctx, 
		  (const struct gl_client_array **)attribs,
		  prim, 1,
		  NULL,
		  0,
		  3 );
}


static void install_meta_state( struct intel_context *intel )
{
   GLcontext *ctx = &intel->ctx;
   struct brw_context *brw = brw_context(ctx);

   if (!brw->metaops.vbo) {
      init_metaops_state(brw);
   }

   install_attribs(brw);
   
   meta_no_texture(&brw->intel);
   meta_flat_shade(&brw->intel);
   brw->metaops.restore_draw_mask = ctx->DrawBuffer->_ColorDrawBufferMask[0];
   brw->metaops.restore_fp = ctx->FragmentProgram.Current;

   /* This works without adjusting refcounts.  Fix later? 
    */
   brw->metaops.saved_draw_region = brw->state.draw_region;
   brw->metaops.saved_depth_region = brw->state.depth_region;
   brw->metaops.active = 1;
   
   brw->state.dirty.brw |= BRW_NEW_METAOPS;
}

static void leave_meta_state( struct intel_context *intel )
{
   GLcontext *ctx = &intel->ctx;
   struct brw_context *brw = brw_context(ctx);

   restore_attribs(brw);

   ctx->DrawBuffer->_ColorDrawBufferMask[0] = brw->metaops.restore_draw_mask;
   ctx->FragmentProgram.Current = brw->metaops.restore_fp;

   brw->state.draw_region = brw->metaops.saved_draw_region;
   brw->state.depth_region = brw->metaops.saved_depth_region;
   brw->metaops.saved_draw_region = NULL;
   brw->metaops.saved_depth_region = NULL;
   brw->metaops.active = 0;

   brw->state.dirty.mesa |= _NEW_BUFFERS;
   brw->state.dirty.brw |= BRW_NEW_METAOPS;
}



void brw_init_metaops( struct brw_context *brw )
{
   init_attribs(brw);


   brw->intel.vtbl.install_meta_state = install_meta_state;
   brw->intel.vtbl.leave_meta_state = leave_meta_state;
   brw->intel.vtbl.meta_no_depth_write = meta_no_depth_write;
   brw->intel.vtbl.meta_no_stencil_write = meta_no_stencil_write;
   brw->intel.vtbl.meta_stencil_replace = meta_stencil_replace;
   brw->intel.vtbl.meta_depth_replace = meta_depth_replace;
   brw->intel.vtbl.meta_color_mask = meta_color_mask;
   brw->intel.vtbl.meta_no_texture = meta_no_texture;
   brw->intel.vtbl.meta_import_pixel_state = meta_import_pixel_state;
   brw->intel.vtbl.meta_frame_buffer_texture = meta_frame_buffer_texture;
   brw->intel.vtbl.meta_draw_region = meta_draw_region;
   brw->intel.vtbl.meta_draw_quad = meta_draw_quad;
   brw->intel.vtbl.meta_texture_blend_replace = meta_texture_blend_replace;
/*    brw->intel.vtbl.meta_tex_rect_source = meta_tex_rect_source; */
/*    brw->intel.vtbl.meta_draw_format = set_draw_format; */
}

void brw_destroy_metaops( struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;

   if (brw->metaops.vbo)
      ctx->Driver.DeleteBuffer( ctx, brw->metaops.vbo );

/*    ctx->Driver.DeleteProgram( ctx, brw->metaops.fp ); */
/*    ctx->Driver.DeleteProgram( ctx, brw->metaops.fp_tex ); */
/*    ctx->Driver.DeleteProgram( ctx, brw->metaops.vp ); */
}
