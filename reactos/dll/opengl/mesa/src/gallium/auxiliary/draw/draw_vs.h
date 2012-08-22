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

#ifndef DRAW_VS_H
#define DRAW_VS_H

#include "draw_context.h"
#include "draw_private.h"
#include "draw_vertex.h"


struct draw_context;
struct pipe_shader_state;

struct draw_variant_input 
{
   enum pipe_format format;
   unsigned buffer;
   unsigned offset; 
   unsigned instance_divisor;
};

struct draw_variant_output
{
   enum attrib_emit format;     /* output format */
   unsigned vs_output:8;        /* which vertex shader output is this? */
   unsigned offset:24;          /* offset into output vertex */
};

struct draw_variant_element {
   struct draw_variant_input in;
   struct draw_variant_output out;
};

struct draw_vs_variant_key {
   unsigned output_stride;
   unsigned nr_elements:8;      /* max2(nr_inputs, nr_outputs) */
   unsigned nr_inputs:8;
   unsigned nr_outputs:8;
   unsigned viewport:1;
   unsigned clip:1;
   unsigned const_vbuffers:5;
   struct draw_variant_element element[PIPE_MAX_ATTRIBS];
};

struct draw_vs_variant;


struct draw_vs_variant {
   struct draw_vs_variant_key key;

   struct draw_vertex_shader *vs;

   void (*set_buffer)( struct draw_vs_variant *,
                      unsigned i,
                      const void *ptr,
                      unsigned stride,
                      unsigned max_stride );

   void (PIPE_CDECL *run_linear)( struct draw_vs_variant *shader,
                                  unsigned start,
                                  unsigned count,
                                  void *output_buffer );

   void (PIPE_CDECL *run_elts)( struct draw_vs_variant *shader,
                                const unsigned *elts,
                                unsigned count,
                                void *output_buffer );

   void (*destroy)( struct draw_vs_variant * );
};


/**
 * Private version of the compiled vertex_shader
 */
struct draw_vertex_shader {
   struct draw_context *draw;

   /* This member will disappear shortly:
    */
   struct pipe_shader_state   state;

   struct tgsi_shader_info info;
   unsigned position_output;
   unsigned edgeflag_output;
   unsigned clipvertex_output;
   unsigned clipdistance_output[2];
   /* Extracted from shader:
    */
   const float (*immediates)[4];

   /* 
    */
   struct draw_vs_variant *variant[16];
   unsigned nr_variants;
   unsigned last_variant;
   struct draw_vs_variant *(*create_variant)( struct draw_vertex_shader *shader,
                                              const struct draw_vs_variant_key *key );


   void (*prepare)( struct draw_vertex_shader *shader,
		    struct draw_context *draw );

   /* Run the shader - this interface will get cleaned up in the
    * future:
    */
   void (*run_linear)( struct draw_vertex_shader *shader,
		       const float (*input)[4],
		       float (*output)[4],
                       const void *constants[PIPE_MAX_CONSTANT_BUFFERS],
                       const unsigned const_size[PIPE_MAX_CONSTANT_BUFFERS],
		       unsigned count,
		       unsigned input_stride,
		       unsigned output_stride );


   void (*delete)( struct draw_vertex_shader * );
};


struct draw_vs_variant *
draw_vs_lookup_variant( struct draw_vertex_shader *base,
                        const struct draw_vs_variant_key *key );


/********************************************************************************
 * Internal functions:
 */

struct draw_vertex_shader *
draw_create_vs_exec(struct draw_context *draw,
		    const struct pipe_shader_state *templ);

struct draw_vertex_shader *
draw_create_vs_ppc(struct draw_context *draw,
		   const struct pipe_shader_state *templ);


struct draw_vs_variant_key;
struct draw_vertex_shader;

#if HAVE_LLVM
struct draw_vertex_shader *
draw_create_vs_llvm(struct draw_context *draw,
		    const struct pipe_shader_state *state);
#endif


/********************************************************************************
 * Helpers for vs implementations that don't do their own fetch/emit variants.
 * Means these can be shared between shaders.
 */
struct translate;
struct translate_key;

struct translate *draw_vs_get_fetch( struct draw_context *draw,
                                     struct translate_key *key );


struct translate *draw_vs_get_emit( struct draw_context *draw,
                                    struct translate_key *key );

struct draw_vs_variant *
draw_vs_create_variant_generic( struct draw_vertex_shader *vs,
                                const struct draw_vs_variant_key *key );



static INLINE int draw_vs_variant_keysize( const struct draw_vs_variant_key *key )
{
   return 2 * sizeof(int) + key->nr_elements * sizeof(struct draw_variant_element);
}

static INLINE int draw_vs_variant_key_compare( const struct draw_vs_variant_key *a,
                                         const struct draw_vs_variant_key *b )
{
   int keysize = draw_vs_variant_keysize(a);
   return memcmp(a, b, keysize);
}


#define MAX_TGSI_VERTICES 4
   


#endif
