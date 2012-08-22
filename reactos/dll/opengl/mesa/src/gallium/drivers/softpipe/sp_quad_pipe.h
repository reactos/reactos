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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef SP_QUAD_PIPE_H
#define SP_QUAD_PIPE_H


struct softpipe_context;
struct quad_header;


/**
 * Fragment processing is performed on 2x2 blocks of pixels called "quads".
 * Quad processing is performed with a pipeline of stages represented by
 * this type.
 */
struct quad_stage {
   struct softpipe_context *softpipe;

   struct quad_stage *next;

   void (*begin)(struct quad_stage *qs);

   /** the stage action */
   void (*run)(struct quad_stage *qs, struct quad_header *quad[], unsigned nr);

   void (*destroy)(struct quad_stage *qs);
};


struct quad_stage *sp_quad_polygon_stipple_stage( struct softpipe_context *softpipe );
struct quad_stage *sp_quad_earlyz_stage( struct softpipe_context *softpipe );
struct quad_stage *sp_quad_shade_stage( struct softpipe_context *softpipe );
struct quad_stage *sp_quad_alpha_test_stage( struct softpipe_context *softpipe );
struct quad_stage *sp_quad_stencil_test_stage( struct softpipe_context *softpipe );
struct quad_stage *sp_quad_depth_test_stage( struct softpipe_context *softpipe );
struct quad_stage *sp_quad_occlusion_stage( struct softpipe_context *softpipe );
struct quad_stage *sp_quad_coverage_stage( struct softpipe_context *softpipe );
struct quad_stage *sp_quad_blend_stage( struct softpipe_context *softpipe );
struct quad_stage *sp_quad_colormask_stage( struct softpipe_context *softpipe );
struct quad_stage *sp_quad_output_stage( struct softpipe_context *softpipe );

void sp_build_quad_pipeline(struct softpipe_context *sp);

#endif /* SP_QUAD_PIPE_H */
