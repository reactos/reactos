/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2008 VMware, Inc.  All rights Reserved.
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
 * TGSI program scan utility.
 * Used to determine which registers and instructions are used by a shader.
 *
 * Authors:  Brian Paul
 */


#include "util/u_math.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_scan.h"




/**
 * Scan the given TGSI shader to collect information such as number of
 * registers used, special instructions used, etc.
 * \return info  the result of the scan
 */
void
tgsi_scan_shader(const struct tgsi_token *tokens,
                 struct tgsi_shader_info *info)
{
   uint procType, i;
   struct tgsi_parse_context parse;

   memset(info, 0, sizeof(*info));
   for (i = 0; i < TGSI_FILE_COUNT; i++)
      info->file_max[i] = -1;

   /**
    ** Setup to begin parsing input shader
    **/
   if (tgsi_parse_init( &parse, tokens ) != TGSI_PARSE_OK) {
      debug_printf("tgsi_parse_init() failed in tgsi_scan_shader()!\n");
      return;
   }
   procType = parse.FullHeader.Processor.Processor;
   assert(procType == TGSI_PROCESSOR_FRAGMENT ||
          procType == TGSI_PROCESSOR_VERTEX ||
          procType == TGSI_PROCESSOR_GEOMETRY);


   /**
    ** Loop over incoming program tokens/instructions
    */
   while( !tgsi_parse_end_of_tokens( &parse ) ) {

      info->num_tokens++;

      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         {
            const struct tgsi_full_instruction *fullinst
               = &parse.FullToken.FullInstruction;
            uint i;

            assert(fullinst->Instruction.Opcode < TGSI_OPCODE_LAST);
            info->opcode_count[fullinst->Instruction.Opcode]++;

            for (i = 0; i < fullinst->Instruction.NumSrcRegs; i++) {
               const struct tgsi_full_src_register *src =
                  &fullinst->Src[i];
               int ind = src->Register.Index;

               /* Mark which inputs are effectively used */
               if (src->Register.File == TGSI_FILE_INPUT) {
                  unsigned usage_mask;
                  usage_mask = tgsi_util_get_inst_usage_mask(fullinst, i);
                  if (src->Register.Indirect) {
                     for (ind = 0; ind < info->num_inputs; ++ind) {
                        info->input_usage_mask[ind] |= usage_mask;
                     }
                  } else {
                     assert(ind >= 0);
                     assert(ind < PIPE_MAX_SHADER_INPUTS);
                     info->input_usage_mask[ind] |= usage_mask;
                  }

                  if (procType == TGSI_PROCESSOR_FRAGMENT &&
                      src->Register.File == TGSI_FILE_INPUT &&
                      info->reads_position &&
                      src->Register.Index == 0 &&
                      (src->Register.SwizzleX == TGSI_SWIZZLE_Z ||
                       src->Register.SwizzleY == TGSI_SWIZZLE_Z ||
                       src->Register.SwizzleZ == TGSI_SWIZZLE_Z ||
                       src->Register.SwizzleW == TGSI_SWIZZLE_Z)) {
                     info->reads_z = TRUE;
                  }
               }

               /* check for indirect register reads */
               if (src->Register.Indirect) {
                  info->indirect_files |= (1 << src->Register.File);
               }
            }

            /* check for indirect register writes */
            for (i = 0; i < fullinst->Instruction.NumDstRegs; i++) {
               const struct tgsi_full_dst_register *dst = &fullinst->Dst[i];
               if (dst->Register.Indirect) {
                  info->indirect_files |= (1 << dst->Register.File);
               }
            }

            info->num_instructions++;
         }
         break;

      case TGSI_TOKEN_TYPE_DECLARATION:
         {
            const struct tgsi_full_declaration *fulldecl
               = &parse.FullToken.FullDeclaration;
            const uint file = fulldecl->Declaration.File;
            uint reg;
            for (reg = fulldecl->Range.First;
                 reg <= fulldecl->Range.Last;
                 reg++) {

               /* only first 32 regs will appear in this bitfield */
               info->file_mask[file] |= (1 << reg);
               info->file_count[file]++;
               info->file_max[file] = MAX2(info->file_max[file], (int)reg);

               if (file == TGSI_FILE_INPUT) {
                  info->input_semantic_name[reg] = (ubyte)fulldecl->Semantic.Name;
                  info->input_semantic_index[reg] = (ubyte)fulldecl->Semantic.Index;
                  info->input_interpolate[reg] = (ubyte)fulldecl->Declaration.Interpolate;
                  info->input_centroid[reg] = (ubyte)fulldecl->Declaration.Centroid;
                  info->input_cylindrical_wrap[reg] = (ubyte)fulldecl->Declaration.CylindricalWrap;
                  info->num_inputs++;

                  if (procType == TGSI_PROCESSOR_FRAGMENT &&
                      fulldecl->Semantic.Name == TGSI_SEMANTIC_POSITION)
                        info->reads_position = TRUE;
               }
               else if (file == TGSI_FILE_SYSTEM_VALUE) {
                  unsigned index = fulldecl->Range.First;
                  unsigned semName = fulldecl->Semantic.Name;

                  info->system_value_semantic_name[index] = semName;
                  info->num_system_values = MAX2(info->num_system_values,
                                                 index + 1);

                  /*
                  info->system_value_semantic_name[info->num_system_values++] = 
                     fulldecl->Semantic.Name;
                  */

                  if (fulldecl->Semantic.Name == TGSI_SEMANTIC_INSTANCEID) {
                     info->uses_instanceid = TRUE;
                  }
                  else if (fulldecl->Semantic.Name == TGSI_SEMANTIC_VERTEXID) {
                     info->uses_vertexid = TRUE;
                  }
               }
               else if (file == TGSI_FILE_OUTPUT) {
                  info->output_semantic_name[reg] = (ubyte)fulldecl->Semantic.Name;
                  info->output_semantic_index[reg] = (ubyte)fulldecl->Semantic.Index;
                  info->num_outputs++;

                  if (procType == TGSI_PROCESSOR_VERTEX &&
                      fulldecl->Semantic.Name == TGSI_SEMANTIC_CLIPDIST) {
                     info->num_written_clipdistance += util_bitcount(fulldecl->Declaration.UsageMask);
                  }
                  /* extra info for special outputs */
                  if (procType == TGSI_PROCESSOR_FRAGMENT &&
                      fulldecl->Semantic.Name == TGSI_SEMANTIC_POSITION)
                        info->writes_z = TRUE;
                  if (procType == TGSI_PROCESSOR_FRAGMENT &&
                      fulldecl->Semantic.Name == TGSI_SEMANTIC_STENCIL)
                        info->writes_stencil = TRUE;
                  if (procType == TGSI_PROCESSOR_VERTEX &&
                      fulldecl->Semantic.Name == TGSI_SEMANTIC_EDGEFLAG) {
                     info->writes_edgeflag = TRUE;
                  }
               }

             }
         }
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         {
            uint reg = info->immediate_count++;
            uint file = TGSI_FILE_IMMEDIATE;

            info->file_mask[file] |= (1 << reg);
            info->file_count[file]++;
            info->file_max[file] = MAX2(info->file_max[file], (int)reg);
         }
         break;

      case TGSI_TOKEN_TYPE_PROPERTY:
         {
            const struct tgsi_full_property *fullprop
               = &parse.FullToken.FullProperty;

            info->properties[info->num_properties].name =
               fullprop->Property.PropertyName;
            memcpy(info->properties[info->num_properties].data,
                   fullprop->u, 8 * sizeof(unsigned));;

            ++info->num_properties;
         }
         break;

      default:
         assert( 0 );
      }
   }

   info->uses_kill = (info->opcode_count[TGSI_OPCODE_KIL] ||
                      info->opcode_count[TGSI_OPCODE_KILP]);

   /* extract simple properties */
   for (i = 0; i < info->num_properties; ++i) {
      switch (info->properties[i].name) {
      case TGSI_PROPERTY_FS_COORD_ORIGIN:
         info->origin_lower_left = info->properties[i].data[0];
         break;
      case TGSI_PROPERTY_FS_COORD_PIXEL_CENTER:
         info->pixel_center_integer = info->properties[i].data[0];
         break;
      case TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS:
         info->color0_writes_all_cbufs = info->properties[i].data[0];
         break;
      default:
         ;
      }
   }

   tgsi_parse_free (&parse);
}



/**
 * Check if the given shader is a "passthrough" shader consisting of only
 * MOV instructions of the form:  MOV OUT[n], IN[n]
 *  
 */
boolean
tgsi_is_passthrough_shader(const struct tgsi_token *tokens)
{
   struct tgsi_parse_context parse;

   /**
    ** Setup to begin parsing input shader
    **/
   if (tgsi_parse_init(&parse, tokens) != TGSI_PARSE_OK) {
      debug_printf("tgsi_parse_init() failed in tgsi_is_passthrough_shader()!\n");
      return FALSE;
   }

   /**
    ** Loop over incoming program tokens/instructions
    */
   while (!tgsi_parse_end_of_tokens(&parse)) {

      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         {
            struct tgsi_full_instruction *fullinst =
               &parse.FullToken.FullInstruction;
            const struct tgsi_full_src_register *src =
               &fullinst->Src[0];
            const struct tgsi_full_dst_register *dst =
               &fullinst->Dst[0];

            /* Do a whole bunch of checks for a simple move */
            if (fullinst->Instruction.Opcode != TGSI_OPCODE_MOV ||
                (src->Register.File != TGSI_FILE_INPUT &&
                 src->Register.File != TGSI_FILE_SYSTEM_VALUE) ||
                dst->Register.File != TGSI_FILE_OUTPUT ||
                src->Register.Index != dst->Register.Index ||

                src->Register.Negate ||
                src->Register.Absolute ||

                src->Register.SwizzleX != TGSI_SWIZZLE_X ||
                src->Register.SwizzleY != TGSI_SWIZZLE_Y ||
                src->Register.SwizzleZ != TGSI_SWIZZLE_Z ||
                src->Register.SwizzleW != TGSI_SWIZZLE_W ||

                dst->Register.WriteMask != TGSI_WRITEMASK_XYZW)
            {
               tgsi_parse_free(&parse);
               return FALSE;
            }
         }
         break;

      case TGSI_TOKEN_TYPE_DECLARATION:
         /* fall-through */
      case TGSI_TOKEN_TYPE_IMMEDIATE:
         /* fall-through */
      case TGSI_TOKEN_TYPE_PROPERTY:
         /* fall-through */
      default:
         ; /* no-op */
      }
   }

   tgsi_parse_free(&parse);

   /* if we get here, it's a pass-through shader */
   return TRUE;
}
