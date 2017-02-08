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
 * Private data structures, etc for the draw module.
 */


/**
 * Authors:
 * Keith Whitwell <keith@tungstengraphics.com>
 * Brian Paul
 */


#ifndef DRAW_PRIVATE_H
#define DRAW_PRIVATE_H


#include "pipe/p_state.h"
#include "pipe/p_defines.h"

#include "tgsi/tgsi_scan.h"

#ifdef HAVE_LLVM
#include <llvm-c/ExecutionEngine.h>
struct draw_llvm;
#endif


/** Sum of frustum planes and user-defined planes */
#define DRAW_TOTAL_CLIP_PLANES (6 + PIPE_MAX_CLIP_PLANES)


struct pipe_context;
struct draw_vertex_shader;
struct draw_context;
struct draw_stage;
struct vbuf_render;
struct tgsi_exec_machine;
struct tgsi_sampler;


/**
 * Basic vertex info.
 * Carry some useful information around with the vertices in the prim pipe.  
 */
struct vertex_header {
   unsigned clipmask:DRAW_TOTAL_CLIP_PLANES;
   unsigned edgeflag:1;
   unsigned have_clipdist:1;
   unsigned vertex_id:16;

   float clip[4];
   float pre_clip_pos[4];

   /* This will probably become float (*data)[4] soon:
    */
   float data[][4];
};

/* NOTE: It should match vertex_id size above */
#define UNDEFINED_VERTEX_ID 0xffff


/* maximum number of shader variants we can cache */
#define DRAW_MAX_SHADER_VARIANTS 128

/**
 * Private context for the drawing module.
 */
struct draw_context
{
   struct pipe_context *pipe;

   /** Drawing/primitive pipeline stages */
   struct {
      struct draw_stage *first;  /**< one of the following */

      struct draw_stage *validate; 

      /* stages (in logical order) */
      struct draw_stage *flatshade;
      struct draw_stage *clip;
      struct draw_stage *cull;
      struct draw_stage *twoside;
      struct draw_stage *offset;
      struct draw_stage *unfilled;
      struct draw_stage *stipple;
      struct draw_stage *aapoint;
      struct draw_stage *aaline;
      struct draw_stage *pstipple;
      struct draw_stage *wide_line;
      struct draw_stage *wide_point;
      struct draw_stage *rasterize;

      float wide_point_threshold; /**< convert pnts to tris if larger than this */
      float wide_line_threshold;  /**< convert lines to tris if wider than this */
      boolean wide_point_sprites; /**< convert points to tris for sprite mode */
      boolean line_stipple;       /**< do line stipple? */
      boolean point_sprite;       /**< convert points to quads for sprites? */

      /* Temporary storage while the pipeline is being run:
       */
      char *verts;
      unsigned vertex_stride;
      unsigned vertex_count;
   } pipeline;


   struct vbuf_render *render;

   /* Support prototype passthrough path:
    */
   struct {
      struct {
         struct draw_pt_middle_end *fetch_emit;
         struct draw_pt_middle_end *fetch_shade_emit;
         struct draw_pt_middle_end *general;
         struct draw_pt_middle_end *llvm;
      } middle;

      struct {
         struct draw_pt_front_end *vsplit;
      } front;

      struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
      unsigned nr_vertex_buffers;

      /*
       * This is the largest legal index value for the current set of
       * bound vertex buffers.  Regardless of any other consideration,
       * all vertex lookups need to be clamped to 0..max_index to
       * prevent out-of-bound access.
       */
      unsigned max_index;

      struct pipe_vertex_element vertex_element[PIPE_MAX_ATTRIBS];
      unsigned nr_vertex_elements;

      struct pipe_index_buffer index_buffer;

      /* user-space vertex data, buffers */
      struct {
         /** vertex element/index buffer (ex: glDrawElements) */
         const void *elts;
         /** bytes per index (0, 1, 2 or 4) */
         unsigned eltSize;
         int eltBias;
         unsigned min_index;
         unsigned max_index;
         
         /** vertex arrays */
         const void *vbuffer[PIPE_MAX_ATTRIBS];
         
         /** constant buffers (for vertex/geometry shader) */
         const void *vs_constants[PIPE_MAX_CONSTANT_BUFFERS];
         unsigned vs_constants_size[PIPE_MAX_CONSTANT_BUFFERS];
         const void *gs_constants[PIPE_MAX_CONSTANT_BUFFERS];
         unsigned gs_constants_size[PIPE_MAX_CONSTANT_BUFFERS];
         
         /* pointer to planes */
         float (*planes)[DRAW_TOTAL_CLIP_PLANES][4]; 
      } user;

      boolean test_fse;         /* enable FSE even though its not correct (eg for softpipe) */
      boolean no_fse;           /* disable FSE even when it is correct */
   } pt;

   struct {
      boolean bypass_clip_xy;
      boolean bypass_clip_z;
      boolean guard_band_xy;
   } driver;

   boolean flushing;         /**< debugging/sanity */
   boolean suspend_flushing; /**< internally set */

   /* Flags set if API requires clipping in these planes and the
    * driver doesn't indicate that it can do it for us.
    */
   boolean clip_xy;
   boolean clip_z;
   boolean clip_user;
   boolean guard_band_xy;

   boolean force_passthrough; /**< never clip or shade */

   boolean dump_vs;

   double mrd;  /**< minimum resolvable depth value, for polygon offset */

   /** Current rasterizer state given to us by the driver */
   const struct pipe_rasterizer_state *rasterizer;
   /** Driver CSO handle for the current rasterizer state */
   void *rast_handle;

   /** Rasterizer CSOs without culling/stipple/etc */
   void *rasterizer_no_cull[2][2];

   struct pipe_viewport_state viewport;
   boolean identity_viewport;

   /** Vertex shader state */
   struct {
      struct draw_vertex_shader *vertex_shader;
      uint num_vs_outputs;  /**< convenience, from vertex_shader */
      uint position_output;
      uint edgeflag_output;
      uint clipvertex_output;
      uint clipdistance_output[2];
      /** TGSI program interpreter runtime state */
      struct tgsi_exec_machine *machine;

      uint num_samplers;
      struct tgsi_sampler **samplers;


      const void *aligned_constants[PIPE_MAX_CONSTANT_BUFFERS];

      const void *aligned_constant_storage[PIPE_MAX_CONSTANT_BUFFERS];
      unsigned const_storage_size[PIPE_MAX_CONSTANT_BUFFERS];


      struct translate *fetch;
      struct translate_cache *fetch_cache;
      struct translate *emit;
      struct translate_cache *emit_cache;
   } vs;

   /** Geometry shader state */
   struct {
      struct draw_geometry_shader *geometry_shader;
      uint num_gs_outputs;  /**< convenience, from geometry_shader */
      uint position_output;

      /** TGSI program interpreter runtime state */
      struct tgsi_exec_machine *machine;

      uint num_samplers;
      struct tgsi_sampler **samplers;
   } gs;

   /** Fragment shader state */
   struct {
      struct draw_fragment_shader *fragment_shader;
   } fs;

   /** Stream output (vertex feedback) state */
   struct {
      struct pipe_stream_output_info state;
      struct draw_so_target *targets[PIPE_MAX_SO_BUFFERS];
      uint num_targets;
   } so;

   /* Clip derived state:
    */
   float plane[DRAW_TOTAL_CLIP_PLANES][4];

   /* If a prim stage introduces new vertex attributes, they'll be stored here
    */
   struct {
      uint num;
      uint semantic_name[10];
      uint semantic_index[10];
      uint slot[10];
   } extra_shader_outputs;

   unsigned reduced_prim;

   unsigned instance_id;

#ifdef HAVE_LLVM
   struct draw_llvm *llvm;
   struct gallivm_state *own_gallivm;
#endif

   struct pipe_sampler_view *sampler_views[PIPE_MAX_VERTEX_SAMPLERS];
   unsigned num_sampler_views;
   const struct pipe_sampler_state *samplers[PIPE_MAX_VERTEX_SAMPLERS];
   unsigned num_samplers;

   void *driver_private;
};


struct draw_fetch_info {
   boolean linear;
   unsigned start;
   const unsigned *elts;
   unsigned count;
};

struct draw_vertex_info {
   struct vertex_header *verts;
   unsigned vertex_size;
   unsigned stride;
   unsigned count;
};

/* these flags are set if the primitive is a segment of a larger one */
#define DRAW_SPLIT_BEFORE 0x1
#define DRAW_SPLIT_AFTER  0x2

struct draw_prim_info {
   boolean linear;
   unsigned start;

   const ushort *elts;
   unsigned count;

   unsigned prim;
   unsigned flags;
   unsigned *primitive_lengths;
   unsigned primitive_count;
};


/*******************************************************************************
 * Draw common initialization code
 */
boolean draw_init(struct draw_context *draw);

/*******************************************************************************
 * Vertex shader code:
 */
boolean draw_vs_init( struct draw_context *draw );
void draw_vs_destroy( struct draw_context *draw );

void draw_vs_set_viewport( struct draw_context *, 
                           const struct pipe_viewport_state * );

void
draw_vs_set_constants(struct draw_context *,
                      unsigned slot,
                      const void *constants,
                      unsigned size);



/*******************************************************************************
 * Geometry shading code:
 */
boolean draw_gs_init( struct draw_context *draw );

void
draw_gs_set_constants(struct draw_context *,
                      unsigned slot,
                      const void *constants,
                      unsigned size);

void draw_gs_destroy( struct draw_context *draw );

/*******************************************************************************
 * Common shading code:
 */
uint draw_current_shader_outputs(const struct draw_context *draw);
uint draw_current_shader_position_output(const struct draw_context *draw);
uint draw_current_shader_clipvertex_output(const struct draw_context *draw);
uint draw_current_shader_clipdistance_output(const struct draw_context *draw, int index);
int draw_alloc_extra_vertex_attrib(struct draw_context *draw,
                                   uint semantic_name, uint semantic_index);
void draw_remove_extra_vertex_attribs(struct draw_context *draw);


/*******************************************************************************
 * Vertex processing (was passthrough) code:
 */
boolean draw_pt_init( struct draw_context *draw );
void draw_pt_destroy( struct draw_context *draw );
void draw_pt_reset_vertex_ids( struct draw_context *draw );


/*******************************************************************************
 * Primitive processing (pipeline) code: 
 */

boolean draw_pipeline_init( struct draw_context *draw );
void draw_pipeline_destroy( struct draw_context *draw );





/*
 * These flags are used by the pipeline when unfilled and/or line stipple modes
 * are operational.
 */
#define DRAW_PIPE_EDGE_FLAG_0   0x1
#define DRAW_PIPE_EDGE_FLAG_1   0x2
#define DRAW_PIPE_EDGE_FLAG_2   0x4
#define DRAW_PIPE_EDGE_FLAG_ALL 0x7
#define DRAW_PIPE_RESET_STIPPLE 0x8

void draw_pipeline_run( struct draw_context *draw,
                        const struct draw_vertex_info *vert,
                        const struct draw_prim_info *prim);

void draw_pipeline_run_linear( struct draw_context *draw,
                               const struct draw_vertex_info *vert,
                               const struct draw_prim_info *prim);




void draw_pipeline_flush( struct draw_context *draw, 
                          unsigned flags );



/*******************************************************************************
 * Flushing 
 */

#define DRAW_FLUSH_STATE_CHANGE              0x8
#define DRAW_FLUSH_BACKEND                   0x10


void draw_do_flush( struct draw_context *draw, unsigned flags );



void *
draw_get_rasterizer_no_cull( struct draw_context *draw,
                             boolean scissor,
                             boolean flatshade );


#endif /* DRAW_PRIVATE_H */
