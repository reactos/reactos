/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
    

#ifndef ST_PROGRAM_H
#define ST_PROGRAM_H

#include "main/mtypes.h"
#include "program/program.h"
#include "pipe/p_state.h"
#include "st_context.h"
#include "st_glsl_to_tgsi.h"


/** Fragment program variant key */
struct st_fp_variant_key
{
   struct st_context *st;         /**< variants are per-context */

   /** for glBitmap */
   GLuint bitmap:1;               /**< glBitmap variant? */

   /** for glDrawPixels */
   GLuint drawpixels:1;           /**< glDrawPixels variant */
   GLuint scaleAndBias:1;         /**< glDrawPixels w/ scale and/or bias? */
   GLuint pixelMaps:1;            /**< glDrawPixels w/ pixel lookup map? */
   GLuint drawpixels_z:1;         /**< glDrawPixels(GL_DEPTH) */
   GLuint drawpixels_stencil:1;   /**< glDrawPixels(GL_STENCIL) */
};


/**
 * Variant of a fragment program.
 */
struct st_fp_variant
{
   /** Parameters which generated this version of fragment program */
   struct st_fp_variant_key key;

   /** Driver's compiled shader */
   void *driver_shader;

   /** For glBitmap variants */
   struct gl_program_parameter_list *parameters;
   uint bitmap_sampler;

   /** next in linked list */
   struct st_fp_variant *next;
};


/**
 * Derived from Mesa gl_fragment_program:
 */
struct st_fragment_program
{
   struct gl_fragment_program Base;
   struct glsl_to_tgsi_visitor* glsl_to_tgsi;

   struct pipe_shader_state tgsi;

   struct st_fp_variant *variants;
};



/** Vertex program variant key */
struct st_vp_variant_key
{
   struct st_context *st;          /**< variants are per-context */
   boolean passthrough_edgeflags;
};


/**
 * This represents a vertex program, especially translated to match
 * the inputs of a particular fragment shader.
 */
struct st_vp_variant
{
   /* Parameters which generated this translated version of a vertex
    * shader:
    */
   struct st_vp_variant_key key;

   /**
    * TGSI tokens (to later generate a 'draw' module shader for
    * selection/feedback/rasterpos)
    */
   struct pipe_shader_state tgsi;

   /** Driver's compiled shader */
   void *driver_shader;

   /** For using our private draw module (glRasterPos) */
   struct draw_vertex_shader *draw_shader;

   /** Next in linked list */
   struct st_vp_variant *next;  

   /** similar to that in st_vertex_program, but with edgeflags info too */
   GLuint num_inputs;
};


/**
 * Derived from Mesa gl_fragment_program:
 */
struct st_vertex_program
{
   struct gl_vertex_program Base;  /**< The Mesa vertex program */
   struct glsl_to_tgsi_visitor* glsl_to_tgsi;

   /** maps a Mesa VERT_ATTRIB_x to a packed TGSI input index */
   GLuint input_to_index[VERT_ATTRIB_MAX];
   /** maps a TGSI input index back to a Mesa VERT_ATTRIB_x */
   GLuint index_to_input[PIPE_MAX_SHADER_INPUTS];
   GLuint num_inputs;

   /** Maps VERT_RESULT_x to slot */
   GLuint result_to_output[VERT_RESULT_MAX];
   ubyte output_semantic_name[VERT_RESULT_MAX];
   ubyte output_semantic_index[VERT_RESULT_MAX];
   GLuint num_outputs;

   /** List of translated variants of this vertex program.
    */
   struct st_vp_variant *variants;
};



/** Geometry program variant key */
struct st_gp_variant_key
{
   struct st_context *st;          /**< variants are per-context */
   /* no other fields yet */
};


/**
 * Geometry program variant.
 */
struct st_gp_variant
{
   /* Parameters which generated this translated version of a vertex */
   struct st_gp_variant_key key;

   void *driver_shader;

   struct st_gp_variant *next;
};


/**
 * Derived from Mesa gl_geometry_program:
 */
struct st_geometry_program
{
   struct gl_geometry_program Base;  /**< The Mesa geometry program */
   struct glsl_to_tgsi_visitor* glsl_to_tgsi;

   /** map GP input back to VP output */
   GLuint input_map[PIPE_MAX_SHADER_INPUTS];

   /** maps a Mesa GEOM_ATTRIB_x to a packed TGSI input index */
   GLuint input_to_index[GEOM_ATTRIB_MAX];
   /** maps a TGSI input index back to a Mesa GEOM_ATTRIB_x */
   GLuint index_to_input[PIPE_MAX_SHADER_INPUTS];

   GLuint num_inputs;

   GLuint input_to_slot[GEOM_ATTRIB_MAX];  /**< Maps GEOM_ATTRIB_x to slot */
   GLuint num_input_slots;

   ubyte input_semantic_name[PIPE_MAX_SHADER_INPUTS];
   ubyte input_semantic_index[PIPE_MAX_SHADER_INPUTS];

   struct pipe_shader_state tgsi;

   struct st_gp_variant *variants;
};



static INLINE struct st_fragment_program *
st_fragment_program( struct gl_fragment_program *fp )
{
   return (struct st_fragment_program *)fp;
}


static INLINE struct st_vertex_program *
st_vertex_program( struct gl_vertex_program *vp )
{
   return (struct st_vertex_program *)vp;
}

static INLINE struct st_geometry_program *
st_geometry_program( struct gl_geometry_program *gp )
{
   return (struct st_geometry_program *)gp;
}

static INLINE void
st_reference_vertprog(struct st_context *st,
                      struct st_vertex_program **ptr,
                      struct st_vertex_program *prog)
{
   _mesa_reference_program(st->ctx,
                           (struct gl_program **) ptr,
                           (struct gl_program *) prog);
}

static INLINE void
st_reference_geomprog(struct st_context *st,
                      struct st_geometry_program **ptr,
                      struct st_geometry_program *prog)
{
   _mesa_reference_program(st->ctx,
                           (struct gl_program **) ptr,
                           (struct gl_program *) prog);
}

static INLINE void
st_reference_fragprog(struct st_context *st,
                      struct st_fragment_program **ptr,
                      struct st_fragment_program *prog)
{
   _mesa_reference_program(st->ctx,
                           (struct gl_program **) ptr,
                           (struct gl_program *) prog);
}


extern struct st_vp_variant *
st_get_vp_variant(struct st_context *st,
                  struct st_vertex_program *stvp,
                  const struct st_vp_variant_key *key);


extern struct st_fp_variant *
st_get_fp_variant(struct st_context *st,
                  struct st_fragment_program *stfp,
                  const struct st_fp_variant_key *key);


extern struct st_gp_variant *
st_get_gp_variant(struct st_context *st,
                  struct st_geometry_program *stgp,
                  const struct st_gp_variant_key *key);


extern void
st_prepare_vertex_program(struct gl_context *ctx,
                          struct st_vertex_program *stvp);

extern GLboolean
st_prepare_fragment_program(struct gl_context *ctx,
                            struct st_fragment_program *stfp);


extern void
st_release_vp_variants( struct st_context *st,
                        struct st_vertex_program *stvp );

extern void
st_release_fp_variants( struct st_context *st,
                        struct st_fragment_program *stfp );

extern void
st_release_gp_variants(struct st_context *st,
                       struct st_geometry_program *stgp);


extern void
st_print_shaders(struct gl_context *ctx);

extern void
st_destroy_program_variants(struct st_context *st);


#endif
