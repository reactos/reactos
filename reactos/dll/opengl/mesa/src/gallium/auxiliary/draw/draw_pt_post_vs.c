/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "util/u_memory.h"
#include "util/u_math.h"
#include "pipe/p_context.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_pt.h"
#include "draw/draw_vs.h"

#define DO_CLIP_XY           0x1
#define DO_CLIP_FULL_Z       0x2
#define DO_CLIP_HALF_Z       0x4
#define DO_CLIP_USER         0x8
#define DO_VIEWPORT          0x10
#define DO_EDGEFLAG          0x20
#define DO_CLIP_XY_GUARD_BAND 0x40


struct pt_post_vs {
   struct draw_context *draw;

   unsigned flags;

   boolean (*run)( struct pt_post_vs *pvs,
                   struct draw_vertex_info *info );
};

static INLINE void
initialize_vertex_header(struct vertex_header *header)
{
   header->clipmask = 0;
   header->edgeflag = 1;
   header->have_clipdist = 0;
   header->vertex_id = UNDEFINED_VERTEX_ID;
}

static INLINE float
dot4(const float *a, const float *b)
{
   return (a[0]*b[0] +
           a[1]*b[1] +
           a[2]*b[2] +
           a[3]*b[3]);
}

#define FLAGS (0)
#define TAG(x) x##_none
#include "draw_cliptest_tmp.h"

#define FLAGS (DO_CLIP_XY | DO_CLIP_FULL_Z | DO_VIEWPORT)
#define TAG(x) x##_xy_fullz_viewport
#include "draw_cliptest_tmp.h"

#define FLAGS (DO_CLIP_XY | DO_CLIP_HALF_Z | DO_VIEWPORT)
#define TAG(x) x##_xy_halfz_viewport
#include "draw_cliptest_tmp.h"

#define FLAGS (DO_CLIP_XY_GUARD_BAND | DO_CLIP_HALF_Z | DO_VIEWPORT)
#define TAG(x) x##_xy_gb_halfz_viewport
#include "draw_cliptest_tmp.h"

#define FLAGS (DO_CLIP_FULL_Z | DO_VIEWPORT)
#define TAG(x) x##_fullz_viewport
#include "draw_cliptest_tmp.h"

#define FLAGS (DO_CLIP_HALF_Z | DO_VIEWPORT)
#define TAG(x) x##_halfz_viewport
#include "draw_cliptest_tmp.h"

#define FLAGS (DO_CLIP_XY | DO_CLIP_FULL_Z | DO_CLIP_USER | DO_VIEWPORT)
#define TAG(x) x##_xy_fullz_user_viewport
#include "draw_cliptest_tmp.h"

#define FLAGS (DO_CLIP_XY | DO_CLIP_FULL_Z | DO_CLIP_USER | DO_VIEWPORT | DO_EDGEFLAG)
#define TAG(x) x##_xy_fullz_user_viewport_edgeflag
#include "draw_cliptest_tmp.h"



/* Don't want to create 64 versions of this function, so catch the
 * less common ones here.  This is looking like something which should
 * be code-generated, perhaps appended to the end of the vertex
 * shader.
 */
#define FLAGS (pvs->flags)
#define TAG(x) x##_generic
#include "draw_cliptest_tmp.h"



boolean draw_pt_post_vs_run( struct pt_post_vs *pvs,
			     struct draw_vertex_info *info )
{
   return pvs->run( pvs, info );
}


void draw_pt_post_vs_prepare( struct pt_post_vs *pvs,
			      boolean clip_xy,
			      boolean clip_z,
                              boolean clip_user,
                              boolean guard_band,
			      boolean bypass_viewport,
			      boolean opengl,
			      boolean need_edgeflags )
{
   pvs->flags = 0;

   /* This combination not currently tested/in use:
    */
   if (opengl)
      guard_band = FALSE;

   if (clip_xy && !guard_band) {
      pvs->flags |= DO_CLIP_XY;
      ASSIGN_4V( pvs->draw->plane[0], -1,  0,  0, 1 );
      ASSIGN_4V( pvs->draw->plane[1],  1,  0,  0, 1 );
      ASSIGN_4V( pvs->draw->plane[2],  0, -1,  0, 1 );
      ASSIGN_4V( pvs->draw->plane[3],  0,  1,  0, 1 );
   }
   else if (clip_xy && guard_band) {
      pvs->flags |= DO_CLIP_XY_GUARD_BAND;
      ASSIGN_4V( pvs->draw->plane[0], -0.5,  0,  0, 1 );
      ASSIGN_4V( pvs->draw->plane[1],  0.5,  0,  0, 1 );
      ASSIGN_4V( pvs->draw->plane[2],  0, -0.5,  0, 1 );
      ASSIGN_4V( pvs->draw->plane[3],  0,  0.5,  0, 1 );
   }

   if (clip_z && opengl) {
      pvs->flags |= DO_CLIP_FULL_Z;
      ASSIGN_4V( pvs->draw->plane[4],  0,  0,  1, 1 );
   }

   if (clip_z && !opengl) {
      pvs->flags |= DO_CLIP_HALF_Z;
      ASSIGN_4V( pvs->draw->plane[4],  0,  0,  1, 0 );
   }

   if (clip_user)
      pvs->flags |= DO_CLIP_USER;

   if (!bypass_viewport)
      pvs->flags |= DO_VIEWPORT;

   if (need_edgeflags)
      pvs->flags |= DO_EDGEFLAG;

   /* Now select the relevant function:
    */
   switch (pvs->flags) {
   case 0:
      pvs->run = do_cliptest_none;
      break;

   case DO_CLIP_XY | DO_CLIP_FULL_Z | DO_VIEWPORT:
      pvs->run = do_cliptest_xy_fullz_viewport;
      break;

   case DO_CLIP_XY | DO_CLIP_HALF_Z | DO_VIEWPORT:
      pvs->run = do_cliptest_xy_halfz_viewport;
      break;

   case DO_CLIP_XY_GUARD_BAND | DO_CLIP_HALF_Z | DO_VIEWPORT:
      pvs->run = do_cliptest_xy_gb_halfz_viewport;
      break;

   case DO_CLIP_FULL_Z | DO_VIEWPORT:
      pvs->run = do_cliptest_fullz_viewport;
      break;

   case DO_CLIP_HALF_Z | DO_VIEWPORT:
      pvs->run = do_cliptest_halfz_viewport;
      break;

   case DO_CLIP_XY | DO_CLIP_FULL_Z | DO_CLIP_USER | DO_VIEWPORT:
      pvs->run = do_cliptest_xy_fullz_user_viewport;
      break;

   case (DO_CLIP_XY | DO_CLIP_FULL_Z | DO_CLIP_USER |
         DO_VIEWPORT | DO_EDGEFLAG):
      pvs->run = do_cliptest_xy_fullz_user_viewport_edgeflag;
      break;
      
   default:
      pvs->run = do_cliptest_generic;
      break;
   }
}


struct pt_post_vs *draw_pt_post_vs_create( struct draw_context *draw )
{
   struct pt_post_vs *pvs = CALLOC_STRUCT( pt_post_vs );
   if (!pvs)
      return NULL;

   pvs->draw = draw;

   return pvs;
}

void draw_pt_post_vs_destroy( struct pt_post_vs *pvs )
{
   FREE(pvs);
}
