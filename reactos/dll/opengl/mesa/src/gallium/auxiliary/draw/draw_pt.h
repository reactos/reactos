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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#ifndef DRAW_PT_H
#define DRAW_PT_H

#include "pipe/p_compiler.h"

struct draw_pt_middle_end;
struct draw_context;
struct draw_prim_info;
struct draw_vertex_info;


#define PT_SHADE      0x1
#define PT_CLIPTEST   0x2
#define PT_PIPELINE   0x4
#define PT_MAX_MIDDLE 0x8


/* The "front end" - prepare sets of fetch, draw elements for the
 * middle end.
 *
 * The fetch elements are indices to the vertices.  The draw elements are
 * indices to the fetched vertices.  When both arrays of elements are both
 * linear, middle->run_linear is called;  When only the fetch elements are
 * linear, middle->run_linear_elts is called;  Otherwise, middle->run is
 * called.
 *
 * When the number of the draw elements exceeds max_vertex of the middle end,
 * the draw elements (as well as the fetch elements) are splitted and the
 * middle end is called multiple times.
 *
 * Currenly there is:
 *    - vsplit - catchall implementation, splits big prims
 */
struct draw_pt_front_end {
   void (*prepare)( struct draw_pt_front_end *,
                    unsigned prim,
                    struct draw_pt_middle_end *,
		    unsigned opt );

   void (*run)( struct draw_pt_front_end *,
                unsigned start,
                unsigned count );

   void (*finish)( struct draw_pt_front_end * );
   void (*destroy)( struct draw_pt_front_end * );
};


/* The "middle end" - prepares actual hardware vertices for the
 * hardware backend.
 *
 * prim_flags is as defined by pipe_draw_info::flags.
 *
 * Currently two versions of this:
 *     - fetch, vertex shade, cliptest, prim-pipeline
 *     - fetch, emit (ie passthrough)
 */
struct draw_pt_middle_end {
   void (*prepare)( struct draw_pt_middle_end *,
                    unsigned prim,
		    unsigned opt,
                    unsigned *max_vertices );

   void (*run)( struct draw_pt_middle_end *,
                const unsigned *fetch_elts,
                unsigned fetch_count,
                const ushort *draw_elts,
                unsigned draw_count,
                unsigned prim_flags );

   void (*run_linear)(struct draw_pt_middle_end *,
                      unsigned start,
                      unsigned count,
                      unsigned prim_flags );

   /* Transform all vertices in a linear range and then draw them with
    * the supplied element list.  May fail and return FALSE.
    */
   boolean (*run_linear_elts)( struct draw_pt_middle_end *,
                            unsigned fetch_start,
                            unsigned fetch_count,
                            const ushort *draw_elts,
                            unsigned draw_count,
                            unsigned prim_flags );

   int (*get_max_vertex_count)( struct draw_pt_middle_end * );

   void (*finish)( struct draw_pt_middle_end * );
   void (*destroy)( struct draw_pt_middle_end * );
};


/* The "back end" - supplied by the driver, defined in draw_vbuf.h.
 */
struct vbuf_render;
struct vertex_header;


/* Frontends: 
 *
 * Currently only the general-purpose vsplit implementation.
 */
struct draw_pt_front_end *draw_pt_vsplit(struct draw_context *draw);


/* Middle-ends:
 *
 * Currently one general-purpose case which can do all possibilities,
 * at the slight expense of creating a vertex_header in some cases
 * unecessarily.
 *
 * The special case fetch_emit code avoids pipeline vertices
 * altogether and builds hardware vertices directly from API
 * vertex_elements.
 */
struct draw_pt_middle_end *draw_pt_fetch_emit( struct draw_context *draw );
struct draw_pt_middle_end *draw_pt_middle_fse( struct draw_context *draw );
struct draw_pt_middle_end *draw_pt_fetch_pipeline_or_emit(struct draw_context *draw);
struct draw_pt_middle_end *draw_pt_fetch_pipeline_or_emit_llvm(struct draw_context *draw);



/*******************************************************************************
 * HW vertex emit:
 */
struct pt_emit;

void draw_pt_emit_prepare( struct pt_emit *emit,
			   unsigned prim,
                           unsigned *max_vertices );

void draw_pt_emit( struct pt_emit *emit,
                   const struct draw_vertex_info *vert_info,
                   const struct draw_prim_info *prim_info);

void draw_pt_emit_linear( struct pt_emit *emit,
                          const struct draw_vertex_info *vert_info,
                          const struct draw_prim_info *prim_info);

void draw_pt_emit_destroy( struct pt_emit *emit );

struct pt_emit *draw_pt_emit_create( struct draw_context *draw );

/*******************************************************************************
 * HW stream output emit:
 */
struct pt_so_emit;

void draw_pt_so_emit_prepare( struct pt_so_emit *emit );

void draw_pt_so_emit( struct pt_so_emit *emit,
                      const struct draw_vertex_info *vert_info,
                      const struct draw_prim_info *prim_info );

void draw_pt_so_emit_destroy( struct pt_so_emit *emit );

struct pt_so_emit *draw_pt_so_emit_create( struct draw_context *draw );

/*******************************************************************************
 * API vertex fetch:
 */

struct pt_fetch;
void draw_pt_fetch_prepare( struct pt_fetch *fetch,
                            unsigned vertex_input_count,
                            unsigned vertex_size,
                            unsigned instance_id_index );

void draw_pt_fetch_run( struct pt_fetch *fetch,
			const unsigned *elts,
			unsigned count,
			char *verts );

void draw_pt_fetch_run_linear( struct pt_fetch *fetch,
                               unsigned start,
                               unsigned count,
                               char *verts );

void draw_pt_fetch_destroy( struct pt_fetch *fetch );

struct pt_fetch *draw_pt_fetch_create( struct draw_context *draw );

/*******************************************************************************
 * Post-VS: cliptest, rhw, viewport
 */
struct pt_post_vs;

boolean draw_pt_post_vs_run( struct pt_post_vs *pvs,
			     struct draw_vertex_info *info );

void draw_pt_post_vs_prepare( struct pt_post_vs *pvs,
			      boolean clip_xy,
			      boolean clip_z,
			      boolean clip_user,
                              boolean guard_band,
			      boolean bypass_viewport,
			      boolean opengl,
			      boolean need_edgeflags );

struct pt_post_vs *draw_pt_post_vs_create( struct draw_context *draw );

void draw_pt_post_vs_destroy( struct pt_post_vs *pvs );


/*******************************************************************************
 * Utils: 
 */
void draw_pt_split_prim(unsigned prim, unsigned *first, unsigned *incr);
unsigned draw_pt_trim_count(unsigned count, unsigned first, unsigned incr);


#endif
