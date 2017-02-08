/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2009 Marek Olšák <maraeo@gmail.com>
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
 * @file
 * Simple vertex/fragment shader generators.
 *  
 * @author Brian Paul
           Marek Olšák
 */


#include "pipe/p_context.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"
#include "util/u_simple_shaders.h"
#include "util/u_debug.h"
#include "tgsi/tgsi_ureg.h"



/**
 * Make simple vertex pass-through shader.
 * \param num_attribs  number of attributes to pass through
 * \param semantic_names  array of semantic names for each attribute
 * \param semantic_indexes  array of semantic indexes for each attribute
 */
void *
util_make_vertex_passthrough_shader(struct pipe_context *pipe,
                                    uint num_attribs,
                                    const uint *semantic_names,
                                    const uint *semantic_indexes)
{
   return util_make_vertex_passthrough_shader_with_so(pipe, num_attribs,
                                                      semantic_names,
                                                      semantic_indexes, NULL);
}

void *
util_make_vertex_passthrough_shader_with_so(struct pipe_context *pipe,
                                    uint num_attribs,
                                    const uint *semantic_names,
                                    const uint *semantic_indexes,
				    const struct pipe_stream_output_info *so)
{
   struct ureg_program *ureg;
   uint i;

   ureg = ureg_create( TGSI_PROCESSOR_VERTEX );
   if (ureg == NULL)
      return NULL;

   for (i = 0; i < num_attribs; i++) {
      struct ureg_src src;
      struct ureg_dst dst;

      src = ureg_DECL_vs_input( ureg, i );
      
      dst = ureg_DECL_output( ureg,
                              semantic_names[i],
                              semantic_indexes[i]);
      
      ureg_MOV( ureg, dst, src );
   }

   ureg_END( ureg );

   return ureg_create_shader_with_so_and_destroy( ureg, pipe, so );
}


/**
 * Make simple fragment texture shader:
 *  IMM {0,0,0,1}                         // (if writemask != 0xf)
 *  MOV OUT[0], IMM[0]                    // (if writemask != 0xf)
 *  TEX OUT[0].writemask, IN[0], SAMP[0], 2D;
 *  END;
 *
 * \param tex_target  one of PIPE_TEXTURE_x
 * \parma interp_mode  either TGSI_INTERPOLATE_LINEAR or PERSPECTIVE
 * \param writemask  mask of TGSI_WRITEMASK_x
 */
void *
util_make_fragment_tex_shader_writemask(struct pipe_context *pipe,
                                        unsigned tex_target,
                                        unsigned interp_mode,
                                        unsigned writemask )
{
   struct ureg_program *ureg;
   struct ureg_src sampler;
   struct ureg_src tex;
   struct ureg_dst out;

   assert(interp_mode == TGSI_INTERPOLATE_LINEAR ||
          interp_mode == TGSI_INTERPOLATE_PERSPECTIVE);

   ureg = ureg_create( TGSI_PROCESSOR_FRAGMENT );
   if (ureg == NULL)
      return NULL;
   
   sampler = ureg_DECL_sampler( ureg, 0 );

   tex = ureg_DECL_fs_input( ureg, 
                             TGSI_SEMANTIC_GENERIC, 0, 
                             interp_mode );

   out = ureg_DECL_output( ureg, 
                           TGSI_SEMANTIC_COLOR,
                           0 );

   if (writemask != TGSI_WRITEMASK_XYZW) {
      struct ureg_src imm = ureg_imm4f( ureg, 0, 0, 0, 1 );

      ureg_MOV( ureg, out, imm );
   }

   ureg_TEX( ureg, 
             ureg_writemask(out, writemask),
             tex_target, tex, sampler );
   ureg_END( ureg );

   return ureg_create_shader_and_destroy( ureg, pipe );
}


/**
 * Make a simple fragment shader that sets the output color to a color
 * taken from a texture.
 * \param tex_target  one of PIPE_TEXTURE_x
 */
void *
util_make_fragment_tex_shader(struct pipe_context *pipe, unsigned tex_target,
                              unsigned interp_mode)
{
   return util_make_fragment_tex_shader_writemask( pipe,
                                                   tex_target,
                                                   interp_mode,
                                                   TGSI_WRITEMASK_XYZW );
}


/**
 * Make a simple fragment texture shader which reads an X component from
 * a texture and writes it as depth.
 */
void *
util_make_fragment_tex_shader_writedepth(struct pipe_context *pipe,
                                         unsigned tex_target,
                                         unsigned interp_mode)
{
   struct ureg_program *ureg;
   struct ureg_src sampler;
   struct ureg_src tex;
   struct ureg_dst out, depth;
   struct ureg_src imm;

   ureg = ureg_create( TGSI_PROCESSOR_FRAGMENT );
   if (ureg == NULL)
      return NULL;

   sampler = ureg_DECL_sampler( ureg, 0 );

   tex = ureg_DECL_fs_input( ureg,
                             TGSI_SEMANTIC_GENERIC, 0,
                             interp_mode );

   out = ureg_DECL_output( ureg,
                           TGSI_SEMANTIC_COLOR,
                           0 );

   depth = ureg_DECL_output( ureg,
                             TGSI_SEMANTIC_POSITION,
                             0 );

   imm = ureg_imm4f( ureg, 0, 0, 0, 1 );

   ureg_MOV( ureg, out, imm );

   ureg_TEX( ureg,
             ureg_writemask(depth, TGSI_WRITEMASK_Z),
             tex_target, tex, sampler );
   ureg_END( ureg );

   return ureg_create_shader_and_destroy( ureg, pipe );
}


/**
 * Make simple fragment color pass-through shader.
 */
void *
util_make_fragment_passthrough_shader(struct pipe_context *pipe)
{
   return util_make_fragment_cloneinput_shader(pipe, 1, TGSI_SEMANTIC_COLOR,
                                               TGSI_INTERPOLATE_PERSPECTIVE);
}


/**
 * Make a fragment shader that copies the input color to N output colors.
 */
void *
util_make_fragment_cloneinput_shader(struct pipe_context *pipe, int num_cbufs,
                                     int input_semantic,
                                     int input_interpolate)
{
   struct ureg_program *ureg;
   struct ureg_src src;
   struct ureg_dst dst[PIPE_MAX_COLOR_BUFS];
   int i;

   assert(num_cbufs <= PIPE_MAX_COLOR_BUFS);

   ureg = ureg_create( TGSI_PROCESSOR_FRAGMENT );
   if (ureg == NULL)
      return NULL;

   src = ureg_DECL_fs_input( ureg, input_semantic, 0,
                             input_interpolate );

   for (i = 0; i < num_cbufs; i++)
      dst[i] = ureg_DECL_output( ureg, TGSI_SEMANTIC_COLOR, i );

   for (i = 0; i < num_cbufs; i++)
      ureg_MOV( ureg, dst[i], src );

   ureg_END( ureg );

   return ureg_create_shader_and_destroy( ureg, pipe );
}
