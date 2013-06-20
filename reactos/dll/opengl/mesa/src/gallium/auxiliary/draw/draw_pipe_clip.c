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
 * \brief  Clipping stage
 *
 * \author  Keith Whitwell <keith@tungstengraphics.com>
 */


#include "util/u_memory.h"
#include "util/u_math.h"

#include "pipe/p_shader_tokens.h"

#include "draw_vs.h"
#include "draw_pipe.h"


#ifndef IS_NEGATIVE
#define IS_NEGATIVE(X) ((X) < 0.0)
#endif

#ifndef DIFFERENT_SIGNS
#define DIFFERENT_SIGNS(x, y) ((x) * (y) <= 0.0F && (x) - (y) != 0.0F)
#endif

#define MAX_CLIPPED_VERTICES ((2 * (6 + PIPE_MAX_CLIP_PLANES))+1)



struct clip_stage {
   struct draw_stage stage;      /**< base class */

   /* Basically duplicate some of the flatshading logic here:
    */
   boolean flat;
   uint num_color_attribs;
   uint color_attribs[4];  /* front/back primary/secondary colors */

   float (*plane)[4];
};


/** Cast wrapper */
static INLINE struct clip_stage *clip_stage( struct draw_stage *stage )
{
   return (struct clip_stage *)stage;
}


#define LINTERP(T, OUT, IN) ((OUT) + (T) * ((IN) - (OUT)))


/* All attributes are float[4], so this is easy:
 */
static void interp_attr( float dst[4],
			 float t,
			 const float in[4],
			 const float out[4] )
{  
   dst[0] = LINTERP( t, out[0], in[0] );
   dst[1] = LINTERP( t, out[1], in[1] );
   dst[2] = LINTERP( t, out[2], in[2] );
   dst[3] = LINTERP( t, out[3], in[3] );
}


/**
 * Copy front/back, primary/secondary colors from src vertex to dst vertex.
 * Used when flat shading.
 */
static void copy_colors( struct draw_stage *stage,
			 struct vertex_header *dst,
			 const struct vertex_header *src )
{
   const struct clip_stage *clipper = clip_stage(stage);
   uint i;
   for (i = 0; i < clipper->num_color_attribs; i++) {
      const uint attr = clipper->color_attribs[i];
      COPY_4FV(dst->data[attr], src->data[attr]);
   }
}



/* Interpolate between two vertices to produce a third.  
 */
static void interp( const struct clip_stage *clip,
		    struct vertex_header *dst,
		    float t,
		    const struct vertex_header *out, 
		    const struct vertex_header *in )
{
   const unsigned nr_attrs = draw_current_shader_outputs(clip->stage.draw);
   const unsigned pos_attr = draw_current_shader_position_output(clip->stage.draw);
   const unsigned clip_attr = draw_current_shader_clipvertex_output(clip->stage.draw);
   unsigned clip_dist[2];
   unsigned j;

   clip_dist[0] = draw_current_shader_clipdistance_output(clip->stage.draw, 0);
   clip_dist[1] = draw_current_shader_clipdistance_output(clip->stage.draw, 1);

   /* Vertex header.
    */
   dst->clipmask = 0;
   dst->edgeflag = 0;        /* will get overwritten later */
   dst->have_clipdist = in->have_clipdist;
   dst->vertex_id = UNDEFINED_VERTEX_ID;

   /* Interpolate the clip-space coords.
    */
   interp_attr(dst->clip, t, in->clip, out->clip);
   /* interpolate the clip-space position */
   interp_attr(dst->pre_clip_pos, t, in->pre_clip_pos, out->pre_clip_pos);

   /* Do the projective divide and viewport transformation to get
    * new window coordinates:
    */
   {
      const float *pos = dst->pre_clip_pos;
      const float *scale = clip->stage.draw->viewport.scale;
      const float *trans = clip->stage.draw->viewport.translate;
      const float oow = 1.0f / pos[3];

      dst->data[pos_attr][0] = pos[0] * oow * scale[0] + trans[0];
      dst->data[pos_attr][1] = pos[1] * oow * scale[1] + trans[1];
      dst->data[pos_attr][2] = pos[2] * oow * scale[2] + trans[2];
      dst->data[pos_attr][3] = oow;
   }

   /* Other attributes
    */
   for (j = 0; j < nr_attrs; j++) {
      if (j != pos_attr && j != clip_attr)
        interp_attr(dst->data[j], t, in->data[j], out->data[j]);
   }
}


/**
 * Emit a post-clip polygon to the next pipeline stage.  The polygon
 * will be convex and the provoking vertex will always be vertex[0].
 */
static void emit_poly( struct draw_stage *stage,
		       struct vertex_header **inlist,
                       const boolean *edgeflags,
		       unsigned n,
		       const struct prim_header *origPrim)
{
   struct prim_header header;
   unsigned i;
   ushort edge_first, edge_middle, edge_last;

   if (stage->draw->rasterizer->flatshade_first) {
      edge_first  = DRAW_PIPE_EDGE_FLAG_0;
      edge_middle = DRAW_PIPE_EDGE_FLAG_1;
      edge_last   = DRAW_PIPE_EDGE_FLAG_2;
   }
   else {
      edge_first  = DRAW_PIPE_EDGE_FLAG_2;
      edge_middle = DRAW_PIPE_EDGE_FLAG_0;
      edge_last   = DRAW_PIPE_EDGE_FLAG_1;
   }

   if (!edgeflags[0])
      edge_first = 0;

   /* later stages may need the determinant, but only the sign matters */
   header.det = origPrim->det;
   header.flags = DRAW_PIPE_RESET_STIPPLE | edge_first | edge_middle;
   header.pad = 0;

   for (i = 2; i < n; i++, header.flags = edge_middle) {
      /* order the triangle verts to respect the provoking vertex mode */
      if (stage->draw->rasterizer->flatshade_first) {
         header.v[0] = inlist[0];  /* the provoking vertex */
         header.v[1] = inlist[i-1];
         header.v[2] = inlist[i];
      }
      else {
         header.v[0] = inlist[i-1];
         header.v[1] = inlist[i];
         header.v[2] = inlist[0];  /* the provoking vertex */
      }

      if (!edgeflags[i-1]) {
         header.flags &= ~edge_middle;
      }

      if (i == n - 1 && edgeflags[i])
         header.flags |= edge_last;

      if (0) {
         const struct draw_vertex_shader *vs = stage->draw->vs.vertex_shader;
         uint j, k;
         debug_printf("Clipped tri: (flat-shade-first = %d)\n",
                      stage->draw->rasterizer->flatshade_first);
         for (j = 0; j < 3; j++) {
            for (k = 0; k < vs->info.num_outputs; k++) {
               debug_printf("  Vert %d: Attr %d:  %f %f %f %f\n", j, k,
                            header.v[j]->data[k][0],
                            header.v[j]->data[k][1],
                            header.v[j]->data[k][2],
                            header.v[j]->data[k][3]);
            }
         }
      }

      stage->next->tri( stage->next, &header );
   }
}


static INLINE float
dot4(const float *a, const float *b)
{
   return (a[0] * b[0] +
           a[1] * b[1] +
           a[2] * b[2] +
           a[3] * b[3]);
}

/*
 * this function extracts the clip distance for the current plane,
 * it first checks if the shader provided a clip distance, otherwise
 * it works out the value using the clipvertex
 */
static INLINE float getclipdist(const struct clip_stage *clipper,
                                struct vertex_header *vert,
                                int plane_idx)
{
   const float *plane;
   float dp;
   if (vert->have_clipdist && plane_idx >= 6) {
      /* pick the correct clipdistance element from the output vectors */
      int _idx = plane_idx - 6;
      int cdi = _idx >= 4;
      int vidx = cdi ? _idx - 4 : _idx;
      dp = vert->data[draw_current_shader_clipdistance_output(clipper->stage.draw, cdi)][vidx];
   } else {
      plane = clipper->plane[plane_idx];
      dp = dot4(vert->clip, plane);
   }
   return dp;
}

/* Clip a triangle against the viewport and user clip planes.
 */
static void
do_clip_tri( struct draw_stage *stage, 
	     struct prim_header *header,
	     unsigned clipmask )
{
   struct clip_stage *clipper = clip_stage( stage );
   struct vertex_header *a[MAX_CLIPPED_VERTICES];
   struct vertex_header *b[MAX_CLIPPED_VERTICES];
   struct vertex_header **inlist = a;
   struct vertex_header **outlist = b;
   unsigned tmpnr = 0;
   unsigned n = 3;
   unsigned i;
   boolean aEdges[MAX_CLIPPED_VERTICES];
   boolean bEdges[MAX_CLIPPED_VERTICES];
   boolean *inEdges = aEdges;
   boolean *outEdges = bEdges;

   inlist[0] = header->v[0];
   inlist[1] = header->v[1];
   inlist[2] = header->v[2];

   /*
    * Note: at this point we can't just use the per-vertex edge flags.
    * We have to observe the edge flag bits set in header->flags which
    * were set during primitive decomposition.  Put those flags into
    * an edge flags array which parallels the vertex array.
    * Later, in the 'unfilled' pipeline stage we'll draw the edge if both
    * the header.flags bit is set AND the per-vertex edgeflag field is set.
    */
   inEdges[0] = !!(header->flags & DRAW_PIPE_EDGE_FLAG_0);
   inEdges[1] = !!(header->flags & DRAW_PIPE_EDGE_FLAG_1);
   inEdges[2] = !!(header->flags & DRAW_PIPE_EDGE_FLAG_2);

   while (clipmask && n >= 3) {
      const unsigned plane_idx = ffs(clipmask)-1;
      const boolean is_user_clip_plane = plane_idx >= 6;
      struct vertex_header *vert_prev = inlist[0];
      boolean *edge_prev = &inEdges[0];
      float dp_prev;
      unsigned outcount = 0;

      dp_prev = getclipdist(clipper, vert_prev, plane_idx);
      clipmask &= ~(1<<plane_idx);

      assert(n < MAX_CLIPPED_VERTICES);
      if (n >= MAX_CLIPPED_VERTICES)
         return;
      inlist[n] = inlist[0]; /* prevent rotation of vertices */
      inEdges[n] = inEdges[0];

      for (i = 1; i <= n; i++) {
	 struct vertex_header *vert = inlist[i];
         boolean *edge = &inEdges[i];

         float dp = getclipdist(clipper, vert, plane_idx);

	 if (!IS_NEGATIVE(dp_prev)) {
            assert(outcount < MAX_CLIPPED_VERTICES);
            if (outcount >= MAX_CLIPPED_VERTICES)
               return;
            outEdges[outcount] = *edge_prev;
	    outlist[outcount++] = vert_prev;
	 }

	 if (DIFFERENT_SIGNS(dp, dp_prev)) {
	    struct vertex_header *new_vert;
            boolean *new_edge;

            assert(tmpnr < MAX_CLIPPED_VERTICES + 1);
            if (tmpnr >= MAX_CLIPPED_VERTICES + 1)
               return;
            new_vert = clipper->stage.tmp[tmpnr++];

            assert(outcount < MAX_CLIPPED_VERTICES);
            if (outcount >= MAX_CLIPPED_VERTICES)
               return;

            new_edge = &outEdges[outcount];
	    outlist[outcount++] = new_vert;

	    if (IS_NEGATIVE(dp)) {
	       /* Going out of bounds.  Avoid division by zero as we
		* know dp != dp_prev from DIFFERENT_SIGNS, above.
		*/
	       float t = dp / (dp - dp_prev);
	       interp( clipper, new_vert, t, vert, vert_prev );
	       
	       /* Whether or not to set edge flag for the new vert depends
                * on whether it's a user-defined clipping plane.  We're
                * copying NVIDIA's behaviour here.
		*/
               if (is_user_clip_plane) {
                  /* we want to see an edge along the clip plane */
                  *new_edge = TRUE;
                  new_vert->edgeflag = TRUE;
               }
               else {
                  /* we don't want to see an edge along the frustum clip plane */
                  *new_edge = *edge_prev;
                  new_vert->edgeflag = FALSE;
               }
	    }
            else {
	       /* Coming back in.
		*/
	       float t = dp_prev / (dp_prev - dp);
	       interp( clipper, new_vert, t, vert_prev, vert );

	       /* Copy starting vert's edgeflag:
		*/
	       new_vert->edgeflag = vert_prev->edgeflag;
               *new_edge = *edge_prev;
	    }
	 }

	 vert_prev = vert;
         edge_prev = edge;
	 dp_prev = dp;
      }

      /* swap in/out lists */
      {
	 struct vertex_header **tmp = inlist;
	 inlist = outlist;
	 outlist = tmp;
	 n = outcount;
      }
      {
         boolean *tmp = inEdges;
         inEdges = outEdges;
         outEdges = tmp;
      }

   }

   /* If flat-shading, copy provoking vertex color to polygon vertex[0]
    */
   if (n >= 3) {
      if (clipper->flat) {
         if (stage->draw->rasterizer->flatshade_first) {
            if (inlist[0] != header->v[0]) {
               assert(tmpnr < MAX_CLIPPED_VERTICES + 1);
               if (tmpnr >= MAX_CLIPPED_VERTICES + 1)
                  return;
               inlist[0] = dup_vert(stage, inlist[0], tmpnr++);
               copy_colors(stage, inlist[0], header->v[0]);
            }
         }
         else {
            if (inlist[0] != header->v[2]) {
               assert(tmpnr < MAX_CLIPPED_VERTICES + 1);
               if (tmpnr >= MAX_CLIPPED_VERTICES + 1)
                  return;
               inlist[0] = dup_vert(stage, inlist[0], tmpnr++);
               copy_colors(stage, inlist[0], header->v[2]);
            }
         }
      }
      
      /* Emit the polygon as triangles to the setup stage:
       */
      emit_poly( stage, inlist, inEdges, n, header );
   }
}


/* Clip a line against the viewport and user clip planes.
 */
static void
do_clip_line( struct draw_stage *stage,
	      struct prim_header *header,
	      unsigned clipmask )
{
   const struct clip_stage *clipper = clip_stage( stage );
   struct vertex_header *v0 = header->v[0];
   struct vertex_header *v1 = header->v[1];
   float t0 = 0.0F;
   float t1 = 0.0F;
   struct prim_header newprim;

   while (clipmask) {
      const unsigned plane_idx = ffs(clipmask)-1;
      const float dp0 = getclipdist(clipper, v0, plane_idx);
      const float dp1 = getclipdist(clipper, v1, plane_idx);

      if (dp1 < 0.0F) {
	 float t = dp1 / (dp1 - dp0);
         t1 = MAX2(t1, t);
      } 

      if (dp0 < 0.0F) {
	 float t = dp0 / (dp0 - dp1);
         t0 = MAX2(t0, t);
      }

      if (t0 + t1 >= 1.0F)
	 return; /* discard */

      clipmask &= ~(1 << plane_idx);  /* turn off this plane's bit */
   }

   if (v0->clipmask) {
      interp( clipper, stage->tmp[0], t0, v0, v1 );

      if (clipper->flat)
	 copy_colors(stage, stage->tmp[0], v0);

      newprim.v[0] = stage->tmp[0];
   }
   else {
      newprim.v[0] = v0;
   }

   if (v1->clipmask) {
      interp( clipper, stage->tmp[1], t1, v1, v0 );
      newprim.v[1] = stage->tmp[1];
   }
   else {
      newprim.v[1] = v1;
   }

   stage->next->line( stage->next, &newprim );
}


static void
clip_point( struct draw_stage *stage, 
	    struct prim_header *header )
{
   if (header->v[0]->clipmask == 0) 
      stage->next->point( stage->next, header );
}


static void
clip_line( struct draw_stage *stage,
	   struct prim_header *header )
{
   unsigned clipmask = (header->v[0]->clipmask | 
                        header->v[1]->clipmask);
   
   if (clipmask == 0) {
      /* no clipping needed */
      stage->next->line( stage->next, header );
   }
   else if ((header->v[0]->clipmask &
             header->v[1]->clipmask) == 0) {
      do_clip_line(stage, header, clipmask);
   }
   /* else, totally clipped */
}


static void
clip_tri( struct draw_stage *stage,
	  struct prim_header *header )
{
   unsigned clipmask = (header->v[0]->clipmask | 
                        header->v[1]->clipmask | 
                        header->v[2]->clipmask);
   
   if (clipmask == 0) {
      /* no clipping needed */
      stage->next->tri( stage->next, header );
   }
   else if ((header->v[0]->clipmask & 
             header->v[1]->clipmask & 
             header->v[2]->clipmask) == 0) {
      do_clip_tri(stage, header, clipmask);
   }
}


/* Update state.  Could further delay this until we hit the first
 * primitive that really requires clipping.
 */
static void 
clip_init_state( struct draw_stage *stage )
{
   struct clip_stage *clipper = clip_stage( stage );

   clipper->flat = stage->draw->rasterizer->flatshade ? TRUE : FALSE;

   if (clipper->flat) {
      const struct draw_vertex_shader *vs = stage->draw->vs.vertex_shader;
      uint i;

      clipper->num_color_attribs = 0;
      for (i = 0; i < vs->info.num_outputs; i++) {
	 if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_COLOR ||
	     vs->info.output_semantic_name[i] == TGSI_SEMANTIC_BCOLOR) {
	    clipper->color_attribs[clipper->num_color_attribs++] = i;
	 }
      }
   }
   
   stage->tri = clip_tri;
   stage->line = clip_line;
}



static void clip_first_tri( struct draw_stage *stage,
			    struct prim_header *header )
{
   clip_init_state( stage );
   stage->tri( stage, header );
}

static void clip_first_line( struct draw_stage *stage,
			     struct prim_header *header )
{
   clip_init_state( stage );
   stage->line( stage, header );
}


static void clip_flush( struct draw_stage *stage, 
			     unsigned flags )
{
   stage->tri = clip_first_tri;
   stage->line = clip_first_line;
   stage->next->flush( stage->next, flags );
}


static void clip_reset_stipple_counter( struct draw_stage *stage )
{
   stage->next->reset_stipple_counter( stage->next );
}


static void clip_destroy( struct draw_stage *stage )
{
   draw_free_temp_verts( stage );
   FREE( stage );
}


/**
 * Allocate a new clipper stage.
 * \return pointer to new stage object
 */
struct draw_stage *draw_clip_stage( struct draw_context *draw )
{
   struct clip_stage *clipper = CALLOC_STRUCT(clip_stage);
   if (clipper == NULL)
      goto fail;

   clipper->stage.draw = draw;
   clipper->stage.name = "clipper";
   clipper->stage.point = clip_point;
   clipper->stage.line = clip_first_line;
   clipper->stage.tri = clip_first_tri;
   clipper->stage.flush = clip_flush;
   clipper->stage.reset_stipple_counter = clip_reset_stipple_counter;
   clipper->stage.destroy = clip_destroy;

   clipper->plane = draw->plane;

   if (!draw_alloc_temp_verts( &clipper->stage, MAX_CLIPPED_VERTICES+1 ))
      goto fail;

   return &clipper->stage;

 fail:
   if (clipper)
      clipper->stage.destroy( &clipper->stage );

   return NULL;
}
