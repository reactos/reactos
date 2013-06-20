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
 * \brief  Primitive rasterization/rendering (points, lines, triangles)
 *
 * \author  Keith Whitwell <keith@tungstengraphics.com>
 * \author  Brian Paul
 */

#include "sp_context.h"
#include "sp_quad.h"
#include "sp_quad_pipe.h"
#include "sp_setup.h"
#include "sp_state.h"
#include "draw/draw_context.h"
#include "draw/draw_vertex.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "util/u_memory.h"


#define DEBUG_VERTS 0
#define DEBUG_FRAGS 0


/**
 * Triangle edge info
 */
struct edge {
   float dx;		/**< X(v1) - X(v0), used only during setup */
   float dy;		/**< Y(v1) - Y(v0), used only during setup */
   float dxdy;		/**< dx/dy */
   float sx, sy;	/**< first sample point coord */
   int lines;		/**< number of lines on this edge */
};


/**
 * Max number of quads (2x2 pixel blocks) to process per batch.
 * This can't be arbitrarily increased since we depend on some 32-bit
 * bitmasks (two bits per quad).
 */
#define MAX_QUADS 16


/**
 * Triangle setup info.
 * Also used for line drawing (taking some liberties).
 */
struct setup_context {
   struct softpipe_context *softpipe;

   /* Vertices are just an array of floats making up each attribute in
    * turn.  Currently fixed at 4 floats, but should change in time.
    * Codegen will help cope with this.
    */
   const float (*vmax)[4];
   const float (*vmid)[4];
   const float (*vmin)[4];
   const float (*vprovoke)[4];

   struct edge ebot;
   struct edge etop;
   struct edge emaj;

   float oneoverarea;
   int facing;

   float pixel_offset;

   struct quad_header quad[MAX_QUADS];
   struct quad_header *quad_ptrs[MAX_QUADS];
   unsigned count;

   struct tgsi_interp_coef coef[PIPE_MAX_SHADER_INPUTS];
   struct tgsi_interp_coef posCoef;  /* For Z, W */

   struct {
      int left[2];   /**< [0] = row0, [1] = row1 */
      int right[2];
      int y;
   } span;

#if DEBUG_FRAGS
   uint numFragsEmitted;  /**< per primitive */
   uint numFragsWritten;  /**< per primitive */
#endif

   unsigned cull_face;		/* which faces cull */
   unsigned nr_vertex_attrs;
};







/**
 * Clip setup->quad against the scissor/surface bounds.
 */
static INLINE void
quad_clip(struct setup_context *setup, struct quad_header *quad)
{
   const struct pipe_scissor_state *cliprect = &setup->softpipe->cliprect;
   const int minx = (int) cliprect->minx;
   const int maxx = (int) cliprect->maxx;
   const int miny = (int) cliprect->miny;
   const int maxy = (int) cliprect->maxy;

   if (quad->input.x0 >= maxx ||
       quad->input.y0 >= maxy ||
       quad->input.x0 + 1 < minx ||
       quad->input.y0 + 1 < miny) {
      /* totally clipped */
      quad->inout.mask = 0x0;
      return;
   }
   if (quad->input.x0 < minx)
      quad->inout.mask &= (MASK_BOTTOM_RIGHT | MASK_TOP_RIGHT);
   if (quad->input.y0 < miny)
      quad->inout.mask &= (MASK_BOTTOM_LEFT | MASK_BOTTOM_RIGHT);
   if (quad->input.x0 == maxx - 1)
      quad->inout.mask &= (MASK_BOTTOM_LEFT | MASK_TOP_LEFT);
   if (quad->input.y0 == maxy - 1)
      quad->inout.mask &= (MASK_TOP_LEFT | MASK_TOP_RIGHT);
}


/**
 * Emit a quad (pass to next stage) with clipping.
 */
static INLINE void
clip_emit_quad(struct setup_context *setup, struct quad_header *quad)
{
   quad_clip( setup, quad );

   if (quad->inout.mask) {
      struct softpipe_context *sp = setup->softpipe;

      sp->quad.first->run( sp->quad.first, &quad, 1 );
   }
}



/**
 * Given an X or Y coordinate, return the block/quad coordinate that it
 * belongs to.
 */
static INLINE int
block(int x)
{
   return x & ~(2-1);
}


static INLINE int
block_x(int x)
{
   return x & ~(16-1);
}


/**
 * Render a horizontal span of quads
 */
static void
flush_spans(struct setup_context *setup)
{
   const int step = MAX_QUADS;
   const int xleft0 = setup->span.left[0];
   const int xleft1 = setup->span.left[1];
   const int xright0 = setup->span.right[0];
   const int xright1 = setup->span.right[1];
   struct quad_stage *pipe = setup->softpipe->quad.first;

   const int minleft = block_x(MIN2(xleft0, xleft1));
   const int maxright = MAX2(xright0, xright1);
   int x;

   /* process quads in horizontal chunks of 16 */
   for (x = minleft; x < maxright; x += step) {
      unsigned skip_left0 = CLAMP(xleft0 - x, 0, step);
      unsigned skip_left1 = CLAMP(xleft1 - x, 0, step);
      unsigned skip_right0 = CLAMP(x + step - xright0, 0, step);
      unsigned skip_right1 = CLAMP(x + step - xright1, 0, step);
      unsigned lx = x;
      unsigned q = 0;

      unsigned skipmask_left0 = (1U << skip_left0) - 1U;
      unsigned skipmask_left1 = (1U << skip_left1) - 1U;

      /* These calculations fail when step == 32 and skip_right == 0.
       */
      unsigned skipmask_right0 = ~0U << (unsigned)(step - skip_right0);
      unsigned skipmask_right1 = ~0U << (unsigned)(step - skip_right1);

      unsigned mask0 = ~skipmask_left0 & ~skipmask_right0;
      unsigned mask1 = ~skipmask_left1 & ~skipmask_right1;

      if (mask0 | mask1) {
         do {
            unsigned quadmask = (mask0 & 3) | ((mask1 & 3) << 2);
            if (quadmask) {
               setup->quad[q].input.x0 = lx;
               setup->quad[q].input.y0 = setup->span.y;
               setup->quad[q].input.facing = setup->facing;
               setup->quad[q].inout.mask = quadmask;
               setup->quad_ptrs[q] = &setup->quad[q];
               q++;
            }
            mask0 >>= 2;
            mask1 >>= 2;
            lx += 2;
         } while (mask0 | mask1);

         pipe->run( pipe, setup->quad_ptrs, q );
      }
   }


   setup->span.y = 0;
   setup->span.right[0] = 0;
   setup->span.right[1] = 0;
   setup->span.left[0] = 1000000;     /* greater than right[0] */
   setup->span.left[1] = 1000000;     /* greater than right[1] */
}


#if DEBUG_VERTS
static void
print_vertex(const struct setup_context *setup,
             const float (*v)[4])
{
   int i;
   debug_printf("   Vertex: (%p)\n", (void *) v);
   for (i = 0; i < setup->nr_vertex_attrs; i++) {
      debug_printf("     %d: %f %f %f %f\n",  i,
              v[i][0], v[i][1], v[i][2], v[i][3]);
      if (util_is_inf_or_nan(v[i][0])) {
         debug_printf("   NaN!\n");
      }
   }
}
#endif


/**
 * Sort the vertices from top to bottom order, setting up the triangle
 * edge fields (ebot, emaj, etop).
 * \return FALSE if coords are inf/nan (cull the tri), TRUE otherwise
 */
static boolean
setup_sort_vertices(struct setup_context *setup,
                    float det,
                    const float (*v0)[4],
                    const float (*v1)[4],
                    const float (*v2)[4])
{
   if (setup->softpipe->rasterizer->flatshade_first)
      setup->vprovoke = v0;
   else
      setup->vprovoke = v2;

   /* determine bottom to top order of vertices */
   {
      float y0 = v0[0][1];
      float y1 = v1[0][1];
      float y2 = v2[0][1];
      if (y0 <= y1) {
	 if (y1 <= y2) {
	    /* y0<=y1<=y2 */
	    setup->vmin = v0;
	    setup->vmid = v1;
	    setup->vmax = v2;
	 }
	 else if (y2 <= y0) {
	    /* y2<=y0<=y1 */
	    setup->vmin = v2;
	    setup->vmid = v0;
	    setup->vmax = v1;
	 }
	 else {
	    /* y0<=y2<=y1 */
	    setup->vmin = v0;
	    setup->vmid = v2;
	    setup->vmax = v1;
	 }
      }
      else {
	 if (y0 <= y2) {
	    /* y1<=y0<=y2 */
	    setup->vmin = v1;
	    setup->vmid = v0;
	    setup->vmax = v2;
	 }
	 else if (y2 <= y1) {
	    /* y2<=y1<=y0 */
	    setup->vmin = v2;
	    setup->vmid = v1;
	    setup->vmax = v0;
	 }
	 else {
	    /* y1<=y2<=y0 */
	    setup->vmin = v1;
	    setup->vmid = v2;
	    setup->vmax = v0;
	 }
      }
   }

   setup->ebot.dx = setup->vmid[0][0] - setup->vmin[0][0];
   setup->ebot.dy = setup->vmid[0][1] - setup->vmin[0][1];
   setup->emaj.dx = setup->vmax[0][0] - setup->vmin[0][0];
   setup->emaj.dy = setup->vmax[0][1] - setup->vmin[0][1];
   setup->etop.dx = setup->vmax[0][0] - setup->vmid[0][0];
   setup->etop.dy = setup->vmax[0][1] - setup->vmid[0][1];

   /*
    * Compute triangle's area.  Use 1/area to compute partial
    * derivatives of attributes later.
    *
    * The area will be the same as prim->det, but the sign may be
    * different depending on how the vertices get sorted above.
    *
    * To determine whether the primitive is front or back facing we
    * use the prim->det value because its sign is correct.
    */
   {
      const float area = (setup->emaj.dx * setup->ebot.dy -
			    setup->ebot.dx * setup->emaj.dy);

      setup->oneoverarea = 1.0f / area;

      /*
      debug_printf("%s one-over-area %f  area %f  det %f\n",
                   __FUNCTION__, setup->oneoverarea, area, det );
      */
      if (util_is_inf_or_nan(setup->oneoverarea))
         return FALSE;
   }

   /* We need to know if this is a front or back-facing triangle for:
    *  - the GLSL gl_FrontFacing fragment attribute (bool)
    *  - two-sided stencil test
    * 0 = front-facing, 1 = back-facing
    */
   setup->facing = 
      ((det < 0.0) ^ 
       (setup->softpipe->rasterizer->front_ccw));

   {
      unsigned face = setup->facing == 0 ? PIPE_FACE_FRONT : PIPE_FACE_BACK;

      if (face & setup->cull_face)
	 return FALSE;
   }


   /* Prepare pixel offset for rasterisation:
    *  - pixel center (0.5, 0.5) for GL, or
    *  - assume (0.0, 0.0) for other APIs.
    */
   if (setup->softpipe->rasterizer->gl_rasterization_rules) {
      setup->pixel_offset = 0.5f;
   } else {
      setup->pixel_offset = 0.0f;
   }

   return TRUE;
}


/* Apply cylindrical wrapping to v0, v1, v2 coordinates, if enabled.
 * Input coordinates must be in [0, 1] range, otherwise results are undefined.
 * Some combinations of coordinates produce invalid results,
 * but this behaviour is acceptable.
 */
static void
tri_apply_cylindrical_wrap(float v0,
                           float v1,
                           float v2,
                           uint cylindrical_wrap,
                           float output[3])
{
   if (cylindrical_wrap) {
      float delta;

      delta = v1 - v0;
      if (delta > 0.5f) {
         v0 += 1.0f;
      }
      else if (delta < -0.5f) {
         v1 += 1.0f;
      }

      delta = v2 - v1;
      if (delta > 0.5f) {
         v1 += 1.0f;
      }
      else if (delta < -0.5f) {
         v2 += 1.0f;
      }

      delta = v0 - v2;
      if (delta > 0.5f) {
         v2 += 1.0f;
      }
      else if (delta < -0.5f) {
         v0 += 1.0f;
      }
   }

   output[0] = v0;
   output[1] = v1;
   output[2] = v2;
}


/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 * The value value comes from vertex[slot][i].
 * The result will be put into setup->coef[slot].a0[i].
 * \param slot  which attribute slot
 * \param i  which component of the slot (0..3)
 */
static void
const_coeff(struct setup_context *setup,
            struct tgsi_interp_coef *coef,
            uint vertSlot, uint i)
{
   assert(i <= 3);

   coef->dadx[i] = 0;
   coef->dady[i] = 0;

   /* need provoking vertex info!
    */
   coef->a0[i] = setup->vprovoke[vertSlot][i];
}


/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a triangle.
 * v[0], v[1] and v[2] are vmin, vmid and vmax, respectively.
 */
static void
tri_linear_coeff(struct setup_context *setup,
                 struct tgsi_interp_coef *coef,
                 uint i,
                 const float v[3])
{
   float botda = v[1] - v[0];
   float majda = v[2] - v[0];
   float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
   float dadx = a * setup->oneoverarea;
   float dady = b * setup->oneoverarea;

   assert(i <= 3);

   coef->dadx[i] = dadx;
   coef->dady[i] = dady;

   /* calculate a0 as the value which would be sampled for the
    * fragment at (0,0), taking into account that we want to sample at
    * pixel centers, in other words (pixel_offset, pixel_offset).
    *
    * this is neat but unfortunately not a good way to do things for
    * triangles with very large values of dadx or dady as it will
    * result in the subtraction and re-addition from a0 of a very
    * large number, which means we'll end up loosing a lot of the
    * fractional bits and precision from a0.  the way to fix this is
    * to define a0 as the sample at a pixel center somewhere near vmin
    * instead - i'll switch to this later.
    */
   coef->a0[i] = (v[0] -
                  (dadx * (setup->vmin[0][0] - setup->pixel_offset) +
                   dady * (setup->vmin[0][1] - setup->pixel_offset)));

   /*
   debug_printf("attr[%d].%c: %f dx:%f dy:%f\n",
		slot, "xyzw"[i],
		setup->coef[slot].a0[i],
		setup->coef[slot].dadx[i],
		setup->coef[slot].dady[i]);
   */
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 * v[0], v[1] and v[2] are vmin, vmid and vmax, respectively.
 */
static void
tri_persp_coeff(struct setup_context *setup,
                struct tgsi_interp_coef *coef,
                uint i,
                const float v[3])
{
   /* premultiply by 1/w  (v[0][3] is always W):
    */
   float mina = v[0] * setup->vmin[0][3];
   float mida = v[1] * setup->vmid[0][3];
   float maxa = v[2] * setup->vmax[0][3];
   float botda = mida - mina;
   float majda = maxa - mina;
   float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
   float dadx = a * setup->oneoverarea;
   float dady = b * setup->oneoverarea;

   /*
   debug_printf("tri persp %d,%d: %f %f %f\n", vertSlot, i,
          	setup->vmin[vertSlot][i],
          	setup->vmid[vertSlot][i],
       		setup->vmax[vertSlot][i]
          );
   */
   assert(i <= 3);

   coef->dadx[i] = dadx;
   coef->dady[i] = dady;
   coef->a0[i] = (mina -
                  (dadx * (setup->vmin[0][0] - setup->pixel_offset) +
                   dady * (setup->vmin[0][1] - setup->pixel_offset)));
}


/**
 * Special coefficient setup for gl_FragCoord.
 * X and Y are trivial, though Y may have to be inverted for OpenGL.
 * Z and W are copied from posCoef which should have already been computed.
 * We could do a bit less work if we'd examine gl_FragCoord's swizzle mask.
 */
static void
setup_fragcoord_coeff(struct setup_context *setup, uint slot)
{
   const struct tgsi_shader_info *fsInfo = &setup->softpipe->fs_variant->info;

   /*X*/
   setup->coef[slot].a0[0] = fsInfo->pixel_center_integer ? 0.0 : 0.5;
   setup->coef[slot].dadx[0] = 1.0;
   setup->coef[slot].dady[0] = 0.0;
   /*Y*/
   setup->coef[slot].a0[1] =
		   (fsInfo->origin_lower_left ? setup->softpipe->framebuffer.height-1 : 0)
		   + (fsInfo->pixel_center_integer ? 0.0 : 0.5);
   setup->coef[slot].dadx[1] = 0.0;
   setup->coef[slot].dady[1] = fsInfo->origin_lower_left ? -1.0 : 1.0;
   /*Z*/
   setup->coef[slot].a0[2] = setup->posCoef.a0[2];
   setup->coef[slot].dadx[2] = setup->posCoef.dadx[2];
   setup->coef[slot].dady[2] = setup->posCoef.dady[2];
   /*W*/
   setup->coef[slot].a0[3] = setup->posCoef.a0[3];
   setup->coef[slot].dadx[3] = setup->posCoef.dadx[3];
   setup->coef[slot].dady[3] = setup->posCoef.dady[3];
}



/**
 * Compute the setup->coef[] array dadx, dady, a0 values.
 * Must be called after setup->vmin,vmid,vmax,vprovoke are initialized.
 */
static void
setup_tri_coefficients(struct setup_context *setup)
{
   struct softpipe_context *softpipe = setup->softpipe;
   const struct tgsi_shader_info *fsInfo = &setup->softpipe->fs_variant->info;
   const struct vertex_info *vinfo = softpipe_get_vertex_info(softpipe);
   uint fragSlot;
   float v[3];

   /* z and w are done by linear interpolation:
    */
   v[0] = setup->vmin[0][2];
   v[1] = setup->vmid[0][2];
   v[2] = setup->vmax[0][2];
   tri_linear_coeff(setup, &setup->posCoef, 2, v);

   v[0] = setup->vmin[0][3];
   v[1] = setup->vmid[0][3];
   v[2] = setup->vmax[0][3];
   tri_linear_coeff(setup, &setup->posCoef, 3, v);

   /* setup interpolation for all the remaining attributes:
    */
   for (fragSlot = 0; fragSlot < fsInfo->num_inputs; fragSlot++) {
      const uint vertSlot = vinfo->attrib[fragSlot].src_index;
      uint j;

      switch (vinfo->attrib[fragSlot].interp_mode) {
      case INTERP_CONSTANT:
         for (j = 0; j < NUM_CHANNELS; j++)
            const_coeff(setup, &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_LINEAR:
         for (j = 0; j < NUM_CHANNELS; j++) {
            tri_apply_cylindrical_wrap(setup->vmin[vertSlot][j],
                                       setup->vmid[vertSlot][j],
                                       setup->vmax[vertSlot][j],
                                       fsInfo->input_cylindrical_wrap[fragSlot] & (1 << j),
                                       v);
            tri_linear_coeff(setup, &setup->coef[fragSlot], j, v);
         }
         break;
      case INTERP_PERSPECTIVE:
         for (j = 0; j < NUM_CHANNELS; j++) {
            tri_apply_cylindrical_wrap(setup->vmin[vertSlot][j],
                                       setup->vmid[vertSlot][j],
                                       setup->vmax[vertSlot][j],
                                       fsInfo->input_cylindrical_wrap[fragSlot] & (1 << j),
                                       v);
            tri_persp_coeff(setup, &setup->coef[fragSlot], j, v);
         }
         break;
      case INTERP_POS:
         setup_fragcoord_coeff(setup, fragSlot);
         break;
      default:
         assert(0);
      }

      if (fsInfo->input_semantic_name[fragSlot] == TGSI_SEMANTIC_FACE) {
         /* convert 0 to 1.0 and 1 to -1.0 */
         setup->coef[fragSlot].a0[0] = setup->facing * -2.0f + 1.0f;
         setup->coef[fragSlot].dadx[0] = 0.0;
         setup->coef[fragSlot].dady[0] = 0.0;
      }
   }
}


static void
setup_tri_edges(struct setup_context *setup)
{
   float vmin_x = setup->vmin[0][0] + setup->pixel_offset;
   float vmid_x = setup->vmid[0][0] + setup->pixel_offset;

   float vmin_y = setup->vmin[0][1] - setup->pixel_offset;
   float vmid_y = setup->vmid[0][1] - setup->pixel_offset;
   float vmax_y = setup->vmax[0][1] - setup->pixel_offset;

   setup->emaj.sy = ceilf(vmin_y);
   setup->emaj.lines = (int) ceilf(vmax_y - setup->emaj.sy);
   setup->emaj.dxdy = setup->emaj.dy ? setup->emaj.dx / setup->emaj.dy : .0f;
   setup->emaj.sx = vmin_x + (setup->emaj.sy - vmin_y) * setup->emaj.dxdy;

   setup->etop.sy = ceilf(vmid_y);
   setup->etop.lines = (int) ceilf(vmax_y - setup->etop.sy);
   setup->etop.dxdy = setup->etop.dy ? setup->etop.dx / setup->etop.dy : .0f;
   setup->etop.sx = vmid_x + (setup->etop.sy - vmid_y) * setup->etop.dxdy;

   setup->ebot.sy = ceilf(vmin_y);
   setup->ebot.lines = (int) ceilf(vmid_y - setup->ebot.sy);
   setup->ebot.dxdy = setup->ebot.dy ? setup->ebot.dx / setup->ebot.dy : .0f;
   setup->ebot.sx = vmin_x + (setup->ebot.sy - vmin_y) * setup->ebot.dxdy;
}


/**
 * Render the upper or lower half of a triangle.
 * Scissoring/cliprect is applied here too.
 */
static void
subtriangle(struct setup_context *setup,
            struct edge *eleft,
            struct edge *eright,
            int lines)
{
   const struct pipe_scissor_state *cliprect = &setup->softpipe->cliprect;
   const int minx = (int) cliprect->minx;
   const int maxx = (int) cliprect->maxx;
   const int miny = (int) cliprect->miny;
   const int maxy = (int) cliprect->maxy;
   int y, start_y, finish_y;
   int sy = (int)eleft->sy;

   assert((int)eleft->sy == (int) eright->sy);
   assert(lines >= 0);

   /* clip top/bottom */
   start_y = sy;
   if (start_y < miny)
      start_y = miny;

   finish_y = sy + lines;
   if (finish_y > maxy)
      finish_y = maxy;

   start_y -= sy;
   finish_y -= sy;

   /*
   debug_printf("%s %d %d\n", __FUNCTION__, start_y, finish_y);
   */

   for (y = start_y; y < finish_y; y++) {

      /* avoid accumulating adds as floats don't have the precision to
       * accurately iterate large triangle edges that way.  luckily we
       * can just multiply these days.
       *
       * this is all drowned out by the attribute interpolation anyway.
       */
      int left = (int)(eleft->sx + y * eleft->dxdy);
      int right = (int)(eright->sx + y * eright->dxdy);

      /* clip left/right */
      if (left < minx)
         left = minx;
      if (right > maxx)
         right = maxx;

      if (left < right) {
         int _y = sy + y;
         if (block(_y) != setup->span.y) {
            flush_spans(setup);
            setup->span.y = block(_y);
         }

         setup->span.left[_y&1] = left;
         setup->span.right[_y&1] = right;
      }
   }


   /* save the values so that emaj can be restarted:
    */
   eleft->sx += lines * eleft->dxdy;
   eright->sx += lines * eright->dxdy;
   eleft->sy += lines;
   eright->sy += lines;
}


/**
 * Recalculate prim's determinant.  This is needed as we don't have
 * get this information through the vbuf_render interface & we must
 * calculate it here.
 */
static float
calc_det(const float (*v0)[4],
         const float (*v1)[4],
         const float (*v2)[4])
{
   /* edge vectors e = v0 - v2, f = v1 - v2 */
   const float ex = v0[0][0] - v2[0][0];
   const float ey = v0[0][1] - v2[0][1];
   const float fx = v1[0][0] - v2[0][0];
   const float fy = v1[0][1] - v2[0][1];

   /* det = cross(e,f).z */
   return ex * fy - ey * fx;
}


/**
 * Do setup for triangle rasterization, then render the triangle.
 */
void
sp_setup_tri(struct setup_context *setup,
             const float (*v0)[4],
             const float (*v1)[4],
             const float (*v2)[4])
{
   float det;

#if DEBUG_VERTS
   debug_printf("Setup triangle:\n");
   print_vertex(setup, v0);
   print_vertex(setup, v1);
   print_vertex(setup, v2);
#endif

   if (setup->softpipe->no_rast || setup->softpipe->rasterizer->rasterizer_discard)
      return;
   
   det = calc_det(v0, v1, v2);
   /*
   debug_printf("%s\n", __FUNCTION__ );
   */

#if DEBUG_FRAGS
   setup->numFragsEmitted = 0;
   setup->numFragsWritten = 0;
#endif

   if (!setup_sort_vertices( setup, det, v0, v1, v2 ))
      return;

   setup_tri_coefficients( setup );
   setup_tri_edges( setup );

   assert(setup->softpipe->reduced_prim == PIPE_PRIM_TRIANGLES);

   setup->span.y = 0;
   setup->span.right[0] = 0;
   setup->span.right[1] = 0;
   /*   setup->span.z_mode = tri_z_mode( setup->ctx ); */

   /*   init_constant_attribs( setup ); */

   if (setup->oneoverarea < 0.0) {
      /* emaj on left:
       */
      subtriangle( setup, &setup->emaj, &setup->ebot, setup->ebot.lines );
      subtriangle( setup, &setup->emaj, &setup->etop, setup->etop.lines );
   }
   else {
      /* emaj on right:
       */
      subtriangle( setup, &setup->ebot, &setup->emaj, setup->ebot.lines );
      subtriangle( setup, &setup->etop, &setup->emaj, setup->etop.lines );
   }

   flush_spans( setup );

#if DEBUG_FRAGS
   printf("Tri: %u frags emitted, %u written\n",
          setup->numFragsEmitted,
          setup->numFragsWritten);
#endif
}


/* Apply cylindrical wrapping to v0, v1 coordinates, if enabled.
 * Input coordinates must be in [0, 1] range, otherwise results are undefined.
 */
static void
line_apply_cylindrical_wrap(float v0,
                            float v1,
                            uint cylindrical_wrap,
                            float output[2])
{
   if (cylindrical_wrap) {
      float delta;

      delta = v1 - v0;
      if (delta > 0.5f) {
         v0 += 1.0f;
      }
      else if (delta < -0.5f) {
         v1 += 1.0f;
      }
   }

   output[0] = v0;
   output[1] = v1;
}


/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a line.
 * v[0] and v[1] are vmin and vmax, respectively.
 */
static void
line_linear_coeff(const struct setup_context *setup,
                  struct tgsi_interp_coef *coef,
                  uint i,
                  const float v[2])
{
   const float da = v[1] - v[0];
   const float dadx = da * setup->emaj.dx * setup->oneoverarea;
   const float dady = da * setup->emaj.dy * setup->oneoverarea;
   coef->dadx[i] = dadx;
   coef->dady[i] = dady;
   coef->a0[i] = (v[0] -
                  (dadx * (setup->vmin[0][0] - setup->pixel_offset) +
                   dady * (setup->vmin[0][1] - setup->pixel_offset)));
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a line.
 * v[0] and v[1] are vmin and vmax, respectively.
 */
static void
line_persp_coeff(const struct setup_context *setup,
                 struct tgsi_interp_coef *coef,
                 uint i,
                 const float v[2])
{
   const float a0 = v[0] * setup->vmin[0][3];
   const float a1 = v[1] * setup->vmax[0][3];
   const float da = a1 - a0;
   const float dadx = da * setup->emaj.dx * setup->oneoverarea;
   const float dady = da * setup->emaj.dy * setup->oneoverarea;
   coef->dadx[i] = dadx;
   coef->dady[i] = dady;
   coef->a0[i] = (a0 -
                  (dadx * (setup->vmin[0][0] - setup->pixel_offset) +
                   dady * (setup->vmin[0][1] - setup->pixel_offset)));
}


/**
 * Compute the setup->coef[] array dadx, dady, a0 values.
 * Must be called after setup->vmin,vmax are initialized.
 */
static boolean
setup_line_coefficients(struct setup_context *setup,
                        const float (*v0)[4],
                        const float (*v1)[4])
{
   struct softpipe_context *softpipe = setup->softpipe;
   const struct tgsi_shader_info *fsInfo = &setup->softpipe->fs_variant->info;
   const struct vertex_info *vinfo = softpipe_get_vertex_info(softpipe);
   uint fragSlot;
   float area;
   float v[2];

   /* use setup->vmin, vmax to point to vertices */
   if (softpipe->rasterizer->flatshade_first)
      setup->vprovoke = v0;
   else
      setup->vprovoke = v1;
   setup->vmin = v0;
   setup->vmax = v1;

   setup->emaj.dx = setup->vmax[0][0] - setup->vmin[0][0];
   setup->emaj.dy = setup->vmax[0][1] - setup->vmin[0][1];

   /* NOTE: this is not really area but something proportional to it */
   area = setup->emaj.dx * setup->emaj.dx + setup->emaj.dy * setup->emaj.dy;
   if (area == 0.0f || util_is_inf_or_nan(area))
      return FALSE;
   setup->oneoverarea = 1.0f / area;

   /* z and w are done by linear interpolation:
    */
   v[0] = setup->vmin[0][2];
   v[1] = setup->vmax[0][2];
   line_linear_coeff(setup, &setup->posCoef, 2, v);

   v[0] = setup->vmin[0][3];
   v[1] = setup->vmax[0][3];
   line_linear_coeff(setup, &setup->posCoef, 3, v);

   /* setup interpolation for all the remaining attributes:
    */
   for (fragSlot = 0; fragSlot < fsInfo->num_inputs; fragSlot++) {
      const uint vertSlot = vinfo->attrib[fragSlot].src_index;
      uint j;

      switch (vinfo->attrib[fragSlot].interp_mode) {
      case INTERP_CONSTANT:
         for (j = 0; j < NUM_CHANNELS; j++)
            const_coeff(setup, &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_LINEAR:
         for (j = 0; j < NUM_CHANNELS; j++) {
            line_apply_cylindrical_wrap(setup->vmin[vertSlot][j],
                                        setup->vmax[vertSlot][j],
                                        fsInfo->input_cylindrical_wrap[fragSlot] & (1 << j),
                                        v);
            line_linear_coeff(setup, &setup->coef[fragSlot], j, v);
         }
         break;
      case INTERP_PERSPECTIVE:
         for (j = 0; j < NUM_CHANNELS; j++) {
            line_apply_cylindrical_wrap(setup->vmin[vertSlot][j],
                                        setup->vmax[vertSlot][j],
                                        fsInfo->input_cylindrical_wrap[fragSlot] & (1 << j),
                                        v);
            line_persp_coeff(setup, &setup->coef[fragSlot], j, v);
         }
         break;
      case INTERP_POS:
         setup_fragcoord_coeff(setup, fragSlot);
         break;
      default:
         assert(0);
      }

      if (fsInfo->input_semantic_name[fragSlot] == TGSI_SEMANTIC_FACE) {
         /* convert 0 to 1.0 and 1 to -1.0 */
         setup->coef[fragSlot].a0[0] = setup->facing * -2.0f + 1.0f;
         setup->coef[fragSlot].dadx[0] = 0.0;
         setup->coef[fragSlot].dady[0] = 0.0;
      }
   }
   return TRUE;
}


/**
 * Plot a pixel in a line segment.
 */
static INLINE void
plot(struct setup_context *setup, int x, int y)
{
   const int iy = y & 1;
   const int ix = x & 1;
   const int quadX = x - ix;
   const int quadY = y - iy;
   const int mask = (1 << ix) << (2 * iy);

   if (quadX != setup->quad[0].input.x0 ||
       quadY != setup->quad[0].input.y0)
   {
      /* flush prev quad, start new quad */

      if (setup->quad[0].input.x0 != -1)
         clip_emit_quad( setup, &setup->quad[0] );

      setup->quad[0].input.x0 = quadX;
      setup->quad[0].input.y0 = quadY;
      setup->quad[0].inout.mask = 0x0;
   }

   setup->quad[0].inout.mask |= mask;
}


/**
 * Do setup for line rasterization, then render the line.
 * Single-pixel width, no stipple, etc.  We rely on the 'draw' module
 * to handle stippling and wide lines.
 */
void
sp_setup_line(struct setup_context *setup,
              const float (*v0)[4],
              const float (*v1)[4])
{
   int x0 = (int) v0[0][0];
   int x1 = (int) v1[0][0];
   int y0 = (int) v0[0][1];
   int y1 = (int) v1[0][1];
   int dx = x1 - x0;
   int dy = y1 - y0;
   int xstep, ystep;

#if DEBUG_VERTS
   debug_printf("Setup line:\n");
   print_vertex(setup, v0);
   print_vertex(setup, v1);
#endif

   if (setup->softpipe->no_rast || setup->softpipe->rasterizer->rasterizer_discard)
      return;

   if (dx == 0 && dy == 0)
      return;

   if (!setup_line_coefficients(setup, v0, v1))
      return;

   assert(v0[0][0] < 1.0e9);
   assert(v0[0][1] < 1.0e9);
   assert(v1[0][0] < 1.0e9);
   assert(v1[0][1] < 1.0e9);

   if (dx < 0) {
      dx = -dx;   /* make positive */
      xstep = -1;
   }
   else {
      xstep = 1;
   }

   if (dy < 0) {
      dy = -dy;   /* make positive */
      ystep = -1;
   }
   else {
      ystep = 1;
   }

   assert(dx >= 0);
   assert(dy >= 0);
   assert(setup->softpipe->reduced_prim == PIPE_PRIM_LINES);

   setup->quad[0].input.x0 = setup->quad[0].input.y0 = -1;
   setup->quad[0].inout.mask = 0x0;

   /* XXX temporary: set coverage to 1.0 so the line appears
    * if AA mode happens to be enabled.
    */
   setup->quad[0].input.coverage[0] =
   setup->quad[0].input.coverage[1] =
   setup->quad[0].input.coverage[2] =
   setup->quad[0].input.coverage[3] = 1.0;

   if (dx > dy) {
      /*** X-major line ***/
      int i;
      const int errorInc = dy + dy;
      int error = errorInc - dx;
      const int errorDec = error - dx;

      for (i = 0; i < dx; i++) {
         plot(setup, x0, y0);

         x0 += xstep;
         if (error < 0) {
            error += errorInc;
         }
         else {
            error += errorDec;
            y0 += ystep;
         }
      }
   }
   else {
      /*** Y-major line ***/
      int i;
      const int errorInc = dx + dx;
      int error = errorInc - dy;
      const int errorDec = error - dy;

      for (i = 0; i < dy; i++) {
         plot(setup, x0, y0);

         y0 += ystep;
         if (error < 0) {
            error += errorInc;
         }
         else {
            error += errorDec;
            x0 += xstep;
         }
      }
   }

   /* draw final quad */
   if (setup->quad[0].inout.mask) {
      clip_emit_quad( setup, &setup->quad[0] );
   }
}


static void
point_persp_coeff(const struct setup_context *setup,
                  const float (*vert)[4],
                  struct tgsi_interp_coef *coef,
                  uint vertSlot, uint i)
{
   assert(i <= 3);
   coef->dadx[i] = 0.0F;
   coef->dady[i] = 0.0F;
   coef->a0[i] = vert[vertSlot][i] * vert[0][3];
}


/**
 * Do setup for point rasterization, then render the point.
 * Round or square points...
 * XXX could optimize a lot for 1-pixel points.
 */
void
sp_setup_point(struct setup_context *setup,
               const float (*v0)[4])
{
   struct softpipe_context *softpipe = setup->softpipe;
   const struct tgsi_shader_info *fsInfo = &setup->softpipe->fs_variant->info;
   const int sizeAttr = setup->softpipe->psize_slot;
   const float size
      = sizeAttr > 0 ? v0[sizeAttr][0]
      : setup->softpipe->rasterizer->point_size;
   const float halfSize = 0.5F * size;
   const boolean round = (boolean) setup->softpipe->rasterizer->point_smooth;
   const float x = v0[0][0];  /* Note: data[0] is always position */
   const float y = v0[0][1];
   const struct vertex_info *vinfo = softpipe_get_vertex_info(softpipe);
   uint fragSlot;

#if DEBUG_VERTS
   debug_printf("Setup point:\n");
   print_vertex(setup, v0);
#endif

   if (setup->softpipe->no_rast || setup->softpipe->rasterizer->rasterizer_discard)
      return;

   assert(setup->softpipe->reduced_prim == PIPE_PRIM_POINTS);

   /* For points, all interpolants are constant-valued.
    * However, for point sprites, we'll need to setup texcoords appropriately.
    * XXX: which coefficients are the texcoords???
    * We may do point sprites as textured quads...
    *
    * KW: We don't know which coefficients are texcoords - ultimately
    * the choice of what interpolation mode to use for each attribute
    * should be determined by the fragment program, using
    * per-attribute declaration statements that include interpolation
    * mode as a parameter.  So either the fragment program will have
    * to be adjusted for pointsprite vs normal point behaviour, or
    * otherwise a special interpolation mode will have to be defined
    * which matches the required behaviour for point sprites.  But -
    * the latter is not a feature of normal hardware, and as such
    * probably should be ruled out on that basis.
    */
   setup->vprovoke = v0;

   /* setup Z, W */
   const_coeff(setup, &setup->posCoef, 0, 2);
   const_coeff(setup, &setup->posCoef, 0, 3);

   for (fragSlot = 0; fragSlot < fsInfo->num_inputs; fragSlot++) {
      const uint vertSlot = vinfo->attrib[fragSlot].src_index;
      uint j;

      switch (vinfo->attrib[fragSlot].interp_mode) {
      case INTERP_CONSTANT:
         /* fall-through */
      case INTERP_LINEAR:
         for (j = 0; j < NUM_CHANNELS; j++)
            const_coeff(setup, &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_PERSPECTIVE:
         for (j = 0; j < NUM_CHANNELS; j++)
            point_persp_coeff(setup, setup->vprovoke,
                              &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_POS:
         setup_fragcoord_coeff(setup, fragSlot);
         break;
      default:
         assert(0);
      }

      if (fsInfo->input_semantic_name[fragSlot] == TGSI_SEMANTIC_FACE) {
         /* convert 0 to 1.0 and 1 to -1.0 */
         setup->coef[fragSlot].a0[0] = setup->facing * -2.0f + 1.0f;
         setup->coef[fragSlot].dadx[0] = 0.0;
         setup->coef[fragSlot].dady[0] = 0.0;
      }
   }


   if (halfSize <= 0.5 && !round) {
      /* special case for 1-pixel points */
      const int ix = ((int) x) & 1;
      const int iy = ((int) y) & 1;
      setup->quad[0].input.x0 = (int) x - ix;
      setup->quad[0].input.y0 = (int) y - iy;
      setup->quad[0].inout.mask = (1 << ix) << (2 * iy);
      clip_emit_quad( setup, &setup->quad[0] );
   }
   else {
      if (round) {
         /* rounded points */
         const int ixmin = block((int) (x - halfSize));
         const int ixmax = block((int) (x + halfSize));
         const int iymin = block((int) (y - halfSize));
         const int iymax = block((int) (y + halfSize));
         const float rmin = halfSize - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
         const float rmax = halfSize + 0.7071F;
         const float rmin2 = MAX2(0.0F, rmin * rmin);
         const float rmax2 = rmax * rmax;
         const float cscale = 1.0F / (rmax2 - rmin2);
         int ix, iy;

         for (iy = iymin; iy <= iymax; iy += 2) {
            for (ix = ixmin; ix <= ixmax; ix += 2) {
               float dx, dy, dist2, cover;

               setup->quad[0].inout.mask = 0x0;

               dx = (ix + 0.5f) - x;
               dy = (iy + 0.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad[0].input.coverage[QUAD_TOP_LEFT] = MIN2(cover, 1.0f);
                  setup->quad[0].inout.mask |= MASK_TOP_LEFT;
               }

               dx = (ix + 1.5f) - x;
               dy = (iy + 0.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad[0].input.coverage[QUAD_TOP_RIGHT] = MIN2(cover, 1.0f);
                  setup->quad[0].inout.mask |= MASK_TOP_RIGHT;
               }

               dx = (ix + 0.5f) - x;
               dy = (iy + 1.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad[0].input.coverage[QUAD_BOTTOM_LEFT] = MIN2(cover, 1.0f);
                  setup->quad[0].inout.mask |= MASK_BOTTOM_LEFT;
               }

               dx = (ix + 1.5f) - x;
               dy = (iy + 1.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad[0].input.coverage[QUAD_BOTTOM_RIGHT] = MIN2(cover, 1.0f);
                  setup->quad[0].inout.mask |= MASK_BOTTOM_RIGHT;
               }

               if (setup->quad[0].inout.mask) {
                  setup->quad[0].input.x0 = ix;
                  setup->quad[0].input.y0 = iy;
                  clip_emit_quad( setup, &setup->quad[0] );
               }
            }
         }
      }
      else {
         /* square points */
         const int xmin = (int) (x + 0.75 - halfSize);
         const int ymin = (int) (y + 0.25 - halfSize);
         const int xmax = xmin + (int) size;
         const int ymax = ymin + (int) size;
         /* XXX could apply scissor to xmin,ymin,xmax,ymax now */
         const int ixmin = block(xmin);
         const int ixmax = block(xmax - 1);
         const int iymin = block(ymin);
         const int iymax = block(ymax - 1);
         int ix, iy;

         /*
         debug_printf("(%f, %f) -> X:%d..%d Y:%d..%d\n", x, y, xmin, xmax,ymin,ymax);
         */
         for (iy = iymin; iy <= iymax; iy += 2) {
            uint rowMask = 0xf;
            if (iy < ymin) {
               /* above the top edge */
               rowMask &= (MASK_BOTTOM_LEFT | MASK_BOTTOM_RIGHT);
            }
            if (iy + 1 >= ymax) {
               /* below the bottom edge */
               rowMask &= (MASK_TOP_LEFT | MASK_TOP_RIGHT);
            }

            for (ix = ixmin; ix <= ixmax; ix += 2) {
               uint mask = rowMask;

               if (ix < xmin) {
                  /* fragment is past left edge of point, turn off left bits */
                  mask &= (MASK_BOTTOM_RIGHT | MASK_TOP_RIGHT);
               }
               if (ix + 1 >= xmax) {
                  /* past the right edge */
                  mask &= (MASK_BOTTOM_LEFT | MASK_TOP_LEFT);
               }

               setup->quad[0].inout.mask = mask;
               setup->quad[0].input.x0 = ix;
               setup->quad[0].input.y0 = iy;
               clip_emit_quad( setup, &setup->quad[0] );
            }
         }
      }
   }
}


/**
 * Called by vbuf code just before we start buffering primitives.
 */
void
sp_setup_prepare(struct setup_context *setup)
{
   struct softpipe_context *sp = setup->softpipe;

   if (sp->dirty) {
      softpipe_update_derived(sp, sp->reduced_api_prim);
   }

   /* Note: nr_attrs is only used for debugging (vertex printing) */
   setup->nr_vertex_attrs = draw_num_shader_outputs(sp->draw);

   sp->quad.first->begin( sp->quad.first );

   if (sp->reduced_api_prim == PIPE_PRIM_TRIANGLES &&
       sp->rasterizer->fill_front == PIPE_POLYGON_MODE_FILL &&
       sp->rasterizer->fill_back == PIPE_POLYGON_MODE_FILL) {
      /* we'll do culling */
      setup->cull_face = sp->rasterizer->cull_face;
   }
   else {
      /* 'draw' will do culling */
      setup->cull_face = PIPE_FACE_NONE;
   }
}


void
sp_setup_destroy_context(struct setup_context *setup)
{
   FREE( setup );
}


/**
 * Create a new primitive setup/render stage.
 */
struct setup_context *
sp_setup_create_context(struct softpipe_context *softpipe)
{
   struct setup_context *setup = CALLOC_STRUCT(setup_context);
   unsigned i;

   setup->softpipe = softpipe;

   for (i = 0; i < MAX_QUADS; i++) {
      setup->quad[i].coef = setup->coef;
      setup->quad[i].posCoef = &setup->posCoef;
   }

   setup->span.left[0] = 1000000;     /* greater than right[0] */
   setup->span.left[1] = 1000000;     /* greater than right[1] */

   return setup;
}
