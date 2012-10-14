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
    

#ifndef ST_ATOM_H
#define ST_ATOM_H

#include "main/glheader.h"

struct st_context;
struct st_tracked_state;

void st_init_atoms( struct st_context *st );
void st_destroy_atoms( struct st_context *st );


void st_validate_state( struct st_context *st );


extern const struct st_tracked_state st_update_framebuffer;
extern const struct st_tracked_state st_update_clip;
extern const struct st_tracked_state st_update_depth_stencil_alpha;
extern const struct st_tracked_state st_update_fp;
extern const struct st_tracked_state st_update_gp;
extern const struct st_tracked_state st_update_vp;
extern const struct st_tracked_state st_update_rasterizer;
extern const struct st_tracked_state st_update_polygon_stipple;
extern const struct st_tracked_state st_update_viewport;
extern const struct st_tracked_state st_update_scissor;
extern const struct st_tracked_state st_update_blend;
extern const struct st_tracked_state st_update_msaa;
extern const struct st_tracked_state st_update_sampler;
extern const struct st_tracked_state st_update_texture;
extern const struct st_tracked_state st_update_vertex_texture;
extern const struct st_tracked_state st_finalize_textures;
extern const struct st_tracked_state st_update_fs_constants;
extern const struct st_tracked_state st_update_gs_constants;
extern const struct st_tracked_state st_update_vs_constants;
extern const struct st_tracked_state st_update_pixel_transfer;


GLuint st_compare_func_to_pipe(GLenum func);

#endif
