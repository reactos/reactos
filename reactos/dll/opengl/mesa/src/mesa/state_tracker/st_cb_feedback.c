/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * GL_SELECT and GL_FEEDBACK render modes.
 * Basically, we use a private instance of the 'draw' module for doing
 * selection/feedback.  It would be nice to use the transform_feedback
 * hardware feature, but it's defined as happening pre-clip and we want
 * post-clipped primitives.  Also, there's concerns about the efficiency
 * of using the hardware for this anyway.
 *
 * Authors:
 *   Brian Paul
 */

#include "main/imports.h"
#include "main/context.h"
#include "main/feedback.h"
#include "main/mfeatures.h"

#include "vbo/vbo.h"

#include "st_context.h"
#include "st_draw.h"
#include "st_cb_feedback.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"

#include "draw/draw_context.h"
#include "draw/draw_pipe.h"


#if FEATURE_feedback

/**
 * This is actually used for both feedback and selection.
 */
struct feedback_stage
{
   struct draw_stage stage;   /**< Base class */
   struct gl_context *ctx;            /**< Rendering context */
   GLboolean reset_stipple_counter;
};


/**********************************************************************
 * GL Feedback functions
 **********************************************************************/

static INLINE struct feedback_stage *
feedback_stage( struct draw_stage *stage )
{
   return (struct feedback_stage *)stage;
}


static void
feedback_vertex(struct gl_context *ctx, const struct draw_context *draw,
                const struct vertex_header *v)
{
   const struct st_context *st = st_context(ctx);
   GLfloat win[4];
   const GLfloat *color, *texcoord;
   GLuint slot;

   /* Recall that Y=0=Top of window for Gallium wincoords */
   win[0] = v->data[0][0];
   win[1] = ctx->DrawBuffer->Height - v->data[0][1];
   win[2] = v->data[0][2];
   win[3] = 1.0F / v->data[0][3];

   /* XXX
    * When we compute vertex layout, save info about position of the
    * color and texcoord attribs to use here.
    */

   slot = st->vertex_result_to_slot[VERT_RESULT_COL0];
   if (slot != ~0U)
      color = v->data[slot];
   else
      color = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];

   slot = st->vertex_result_to_slot[VERT_RESULT_TEX0];
   if (slot != ~0U)
      texcoord = v->data[slot];
   else
      texcoord = ctx->Current.Attrib[VERT_ATTRIB_TEX0];

   _mesa_feedback_vertex(ctx, win, color, texcoord);
}


static void
feedback_tri( struct draw_stage *stage, struct prim_header *prim )
{
   struct feedback_stage *fs = feedback_stage(stage);
   struct draw_context *draw = stage->draw;
   _mesa_feedback_token(fs->ctx, (GLfloat) GL_POLYGON_TOKEN);
   _mesa_feedback_token(fs->ctx, (GLfloat) 3); /* three vertices */
   feedback_vertex(fs->ctx, draw, prim->v[0]);
   feedback_vertex(fs->ctx, draw, prim->v[1]);
   feedback_vertex(fs->ctx, draw, prim->v[2]);
}


static void
feedback_line( struct draw_stage *stage, struct prim_header *prim )
{
   struct feedback_stage *fs = feedback_stage(stage);
   struct draw_context *draw = stage->draw;
   if (fs->reset_stipple_counter) {
      _mesa_feedback_token(fs->ctx, (GLfloat) GL_LINE_RESET_TOKEN);
      fs->reset_stipple_counter = GL_FALSE;
   }
   else {
      _mesa_feedback_token(fs->ctx, (GLfloat) GL_LINE_TOKEN);
   }
   feedback_vertex(fs->ctx, draw, prim->v[0]);
   feedback_vertex(fs->ctx, draw, prim->v[1]);
}


static void
feedback_point( struct draw_stage *stage, struct prim_header *prim )
{
   struct feedback_stage *fs = feedback_stage(stage);
   struct draw_context *draw = stage->draw;
   _mesa_feedback_token(fs->ctx, (GLfloat) GL_POINT_TOKEN);
   feedback_vertex(fs->ctx, draw, prim->v[0]);
}


static void
feedback_flush( struct draw_stage *stage, unsigned flags )
{
   /* no-op */
}


static void
feedback_reset_stipple_counter( struct draw_stage *stage )
{
   struct feedback_stage *fs = feedback_stage(stage);
   fs->reset_stipple_counter = GL_TRUE;
}


static void
feedback_destroy( struct draw_stage *stage )
{
   /* no-op */
}

/**
 * Create GL feedback drawing stage.
 */
static struct draw_stage *
draw_glfeedback_stage(struct gl_context *ctx, struct draw_context *draw)
{
   struct feedback_stage *fs = ST_CALLOC_STRUCT(feedback_stage);

   fs->stage.draw = draw;
   fs->stage.next = NULL;
   fs->stage.point = feedback_point;
   fs->stage.line = feedback_line;
   fs->stage.tri = feedback_tri;
   fs->stage.flush = feedback_flush;
   fs->stage.reset_stipple_counter = feedback_reset_stipple_counter;
   fs->stage.destroy = feedback_destroy;
   fs->ctx = ctx;

   return &fs->stage;
}



/**********************************************************************
 * GL Selection functions
 **********************************************************************/

static void
select_tri( struct draw_stage *stage, struct prim_header *prim )
{
   struct feedback_stage *fs = feedback_stage(stage);
   _mesa_update_hitflag( fs->ctx, prim->v[0]->data[0][2] );
   _mesa_update_hitflag( fs->ctx, prim->v[1]->data[0][2] );
   _mesa_update_hitflag( fs->ctx, prim->v[2]->data[0][2] );
}

static void
select_line( struct draw_stage *stage, struct prim_header *prim )
{
   struct feedback_stage *fs = feedback_stage(stage);
   _mesa_update_hitflag( fs->ctx, prim->v[0]->data[0][2] );
   _mesa_update_hitflag( fs->ctx, prim->v[1]->data[0][2] );
}


static void
select_point( struct draw_stage *stage, struct prim_header *prim )
{
   struct feedback_stage *fs = feedback_stage(stage);
   _mesa_update_hitflag( fs->ctx, prim->v[0]->data[0][2] );
}


static void
select_flush( struct draw_stage *stage, unsigned flags )
{
   /* no-op */
}


static void
select_reset_stipple_counter( struct draw_stage *stage )
{
   /* no-op */
}

static void
select_destroy( struct draw_stage *stage )
{
   /* no-op */
}


/**
 * Create GL selection mode drawing stage.
 */
static struct draw_stage *
draw_glselect_stage(struct gl_context *ctx, struct draw_context *draw)
{
   struct feedback_stage *fs = ST_CALLOC_STRUCT(feedback_stage);

   fs->stage.draw = draw;
   fs->stage.next = NULL;
   fs->stage.point = select_point;
   fs->stage.line = select_line;
   fs->stage.tri = select_tri;
   fs->stage.flush = select_flush;
   fs->stage.reset_stipple_counter = select_reset_stipple_counter;
   fs->stage.destroy = select_destroy;
   fs->ctx = ctx;

   return &fs->stage;
}


static void
st_RenderMode(struct gl_context *ctx, GLenum newMode )
{
   struct st_context *st = st_context(ctx);
   struct draw_context *draw = st->draw;

   if (newMode == GL_RENDER) {
      /* restore normal VBO draw function */
      vbo_set_draw_func(ctx, st_draw_vbo);
   }
   else if (newMode == GL_SELECT) {
      if (!st->selection_stage)
         st->selection_stage = draw_glselect_stage(ctx, draw);
      draw_set_rasterize_stage(draw, st->selection_stage);
      /* Plug in new vbo draw function */
      vbo_set_draw_func(ctx, st_feedback_draw_vbo);
   }
   else {
      if (!st->feedback_stage)
         st->feedback_stage = draw_glfeedback_stage(ctx, draw);
      draw_set_rasterize_stage(draw, st->feedback_stage);
      /* Plug in new vbo draw function */
      vbo_set_draw_func(ctx, st_feedback_draw_vbo);
      /* need to generate/use a vertex program that emits pos/color/tex */
      st->dirty.st |= ST_NEW_VERTEX_PROGRAM;
   }
}



void st_init_feedback_functions(struct dd_function_table *functions)
{
   functions->RenderMode = st_RenderMode;
}

#endif /* FEATURE_feedback */
