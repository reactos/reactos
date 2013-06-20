/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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

#ifndef DRAW_GS_H
#define DRAW_GS_H

#include "draw_context.h"
#include "draw_private.h"


#define MAX_TGSI_PRIMITIVES 4

struct draw_context;

/**
 * Private version of the compiled geometry shader
 */
struct draw_geometry_shader {
   struct draw_context *draw;

   struct tgsi_exec_machine *machine;

   /* This member will disappear shortly:*/
   struct pipe_shader_state state;

   struct tgsi_shader_info info;
   unsigned position_output;

   unsigned max_output_vertices;
   unsigned input_primitive;
   unsigned output_primitive;

   unsigned *primitive_lengths;
   unsigned emitted_vertices;
   unsigned emitted_primitives;

   float (*tmp_output)[4];
   unsigned vertex_size;

   unsigned in_prim_idx;
   unsigned input_vertex_stride;
   const float (*input)[4];
};

/*
 * Returns the number of vertices emitted.
 * The vertex shader can emit any number of vertices as long as it's
 * smaller than the GS_MAX_OUTPUT_VERTICES shader property.
 */
int draw_geometry_shader_run(struct draw_geometry_shader *shader,
                             const void *constants[PIPE_MAX_CONSTANT_BUFFERS], 
                             const unsigned constants_size[PIPE_MAX_CONSTANT_BUFFERS], 
                             const struct draw_vertex_info *input_verts,
                             const struct draw_prim_info *input_prim,
                             struct draw_vertex_info *output_verts,
                             struct draw_prim_info *output_prims );

void draw_geometry_shader_prepare(struct draw_geometry_shader *shader,
                                  struct draw_context *draw);

void draw_geometry_shader_delete(struct draw_geometry_shader *shader);

int draw_gs_max_output_vertices(struct draw_geometry_shader *shader,
                                unsigned pipe_prim);

#endif
