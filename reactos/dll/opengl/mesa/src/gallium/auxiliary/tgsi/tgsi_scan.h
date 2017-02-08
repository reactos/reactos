/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef TGSI_SCAN_H
#define TGSI_SCAN_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"

/**
 * Shader summary info
 */
struct tgsi_shader_info
{
   uint num_tokens;

   ubyte num_inputs;
   ubyte num_outputs;
   ubyte input_semantic_name[PIPE_MAX_SHADER_INPUTS]; /**< TGSI_SEMANTIC_x */
   ubyte input_semantic_index[PIPE_MAX_SHADER_INPUTS];
   ubyte input_interpolate[PIPE_MAX_SHADER_INPUTS];
   ubyte input_centroid[PIPE_MAX_SHADER_INPUTS];
   ubyte input_usage_mask[PIPE_MAX_SHADER_INPUTS];
   ubyte input_cylindrical_wrap[PIPE_MAX_SHADER_INPUTS];
   ubyte output_semantic_name[PIPE_MAX_SHADER_OUTPUTS]; /**< TGSI_SEMANTIC_x */
   ubyte output_semantic_index[PIPE_MAX_SHADER_OUTPUTS];

   ubyte num_system_values;
   ubyte system_value_semantic_name[PIPE_MAX_SHADER_INPUTS];

   uint file_mask[TGSI_FILE_COUNT];  /**< bitmask of declared registers */
   uint file_count[TGSI_FILE_COUNT];  /**< number of declared registers */
   int file_max[TGSI_FILE_COUNT];  /**< highest index of declared registers */

   uint immediate_count; /**< number of immediates declared */
   uint num_instructions;

   uint opcode_count[TGSI_OPCODE_LAST];  /**< opcode histogram */

   boolean reads_position; /**< does fragment shader read position? */
   boolean reads_z; /**< does fragment shader read depth? */
   boolean writes_z;  /**< does fragment shader write Z value? */
   boolean writes_stencil; /**< does fragment shader write stencil value? */
   boolean writes_edgeflag; /**< vertex shader outputs edgeflag */
   boolean uses_kill;  /**< KIL or KILP instruction used? */
   boolean uses_instanceid;
   boolean uses_vertexid;
   boolean origin_lower_left;
   boolean pixel_center_integer;
   boolean color0_writes_all_cbufs;

   unsigned num_written_clipdistance;
   /**
    * Bitmask indicating which register files are accessed with
    * indirect addressing.  The bits are (1 << TGSI_FILE_x), etc.
    */
   unsigned indirect_files;

   struct {
      unsigned name;
      unsigned data[8];
   } properties[TGSI_PROPERTY_COUNT];
   uint num_properties;
};

extern void
tgsi_scan_shader(const struct tgsi_token *tokens,
                 struct tgsi_shader_info *info);


extern boolean
tgsi_is_passthrough_shader(const struct tgsi_token *tokens);


#endif /* TGSI_SCAN_H */
