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
  *   Brian Paul
  */


#include "main/imports.h"
#include "main/hash.h"
#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/programopt.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_ureg.h"

#include "st_debug.h"
#include "st_cb_bitmap.h"
#include "st_cb_drawpixels.h"
#include "st_context.h"
#include "st_program.h"
#include "st_mesa_to_tgsi.h"
#include "cso_cache/cso_context.h"



/**
 * Delete a vertex program variant.  Note the caller must unlink
 * the variant from the linked list.
 */
static void
delete_vp_variant(struct st_context *st, struct st_vp_variant *vpv)
{
   if (vpv->driver_shader) 
      cso_delete_vertex_shader(st->cso_context, vpv->driver_shader);
      
#if FEATURE_feedback || FEATURE_rastpos
   if (vpv->draw_shader)
      draw_delete_vertex_shader( st->draw, vpv->draw_shader );
#endif
      
   if (vpv->tgsi.tokens)
      st_free_tokens(vpv->tgsi.tokens);
      
   FREE( vpv );
}



/**
 * Clean out any old compilations:
 */
void
st_release_vp_variants( struct st_context *st,
                        struct st_vertex_program *stvp )
{
   struct st_vp_variant *vpv;

   for (vpv = stvp->variants; vpv; ) {
      struct st_vp_variant *next = vpv->next;
      delete_vp_variant(st, vpv);
      vpv = next;
   }

   stvp->variants = NULL;
}



/**
 * Delete a fragment program variant.  Note the caller must unlink
 * the variant from the linked list.
 */
static void
delete_fp_variant(struct st_context *st, struct st_fp_variant *fpv)
{
   if (fpv->driver_shader) 
      cso_delete_fragment_shader(st->cso_context, fpv->driver_shader);
   if (fpv->parameters)
      _mesa_free_parameter_list(fpv->parameters);
      
   FREE(fpv);
}


/**
 * Free all variants of a fragment program.
 */
void
st_release_fp_variants(struct st_context *st, struct st_fragment_program *stfp)
{
   struct st_fp_variant *fpv;

   for (fpv = stfp->variants; fpv; ) {
      struct st_fp_variant *next = fpv->next;
      delete_fp_variant(st, fpv);
      fpv = next;
   }

   stfp->variants = NULL;
}


/**
 * Delete a geometry program variant.  Note the caller must unlink
 * the variant from the linked list.
 */
static void
delete_gp_variant(struct st_context *st, struct st_gp_variant *gpv)
{
   if (gpv->driver_shader) 
      cso_delete_geometry_shader(st->cso_context, gpv->driver_shader);
      
   FREE(gpv);
}


/**
 * Free all variants of a geometry program.
 */
void
st_release_gp_variants(struct st_context *st, struct st_geometry_program *stgp)
{
   struct st_gp_variant *gpv;

   for (gpv = stgp->variants; gpv; ) {
      struct st_gp_variant *next = gpv->next;
      delete_gp_variant(st, gpv);
      gpv = next;
   }

   stgp->variants = NULL;
}




/**
 * Translate a Mesa vertex shader into a TGSI shader.
 * \param outputMapping  to map vertex program output registers (VERT_RESULT_x)
 *       to TGSI output slots
 * \param tokensOut  destination for TGSI tokens
 * \return  pointer to cached pipe_shader object.
 */
void
st_prepare_vertex_program(struct gl_context *ctx,
                            struct st_vertex_program *stvp)
{
   GLuint attr;

   stvp->num_inputs = 0;
   stvp->num_outputs = 0;

   if (stvp->Base.IsPositionInvariant)
      _mesa_insert_mvp_code(ctx, &stvp->Base);

   if (!stvp->glsl_to_tgsi)
      assert(stvp->Base.Base.NumInstructions > 1);

   /*
    * Determine number of inputs, the mappings between VERT_ATTRIB_x
    * and TGSI generic input indexes, plus input attrib semantic info.
    */
   for (attr = 0; attr < VERT_ATTRIB_MAX; attr++) {
      if ((stvp->Base.Base.InputsRead & BITFIELD64_BIT(attr)) != 0) {
         stvp->input_to_index[attr] = stvp->num_inputs;
         stvp->index_to_input[stvp->num_inputs] = attr;
         stvp->num_inputs++;
      }
   }
   /* bit of a hack, presetup potentially unused edgeflag input */
   stvp->input_to_index[VERT_ATTRIB_EDGEFLAG] = stvp->num_inputs;
   stvp->index_to_input[stvp->num_inputs] = VERT_ATTRIB_EDGEFLAG;

   /* Compute mapping of vertex program outputs to slots.
    */
   for (attr = 0; attr < VERT_RESULT_MAX; attr++) {
      if ((stvp->Base.Base.OutputsWritten & BITFIELD64_BIT(attr)) == 0) {
         stvp->result_to_output[attr] = ~0;
      }
      else {
         unsigned slot = stvp->num_outputs++;

         stvp->result_to_output[attr] = slot;

         switch (attr) {
         case VERT_RESULT_HPOS:
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_POSITION;
            stvp->output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_COL0:
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            stvp->output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_COL1:
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            stvp->output_semantic_index[slot] = 1;
            break;
         case VERT_RESULT_BFC0:
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_BCOLOR;
            stvp->output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_BFC1:
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_BCOLOR;
            stvp->output_semantic_index[slot] = 1;
            break;
         case VERT_RESULT_FOGC:
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_FOG;
            stvp->output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_PSIZ:
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_PSIZE;
            stvp->output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_CLIP_DIST0:
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_CLIPDIST;
            stvp->output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_CLIP_DIST1:
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_CLIPDIST;
            stvp->output_semantic_index[slot] = 1;
            break;
         case VERT_RESULT_EDGE:
            assert(0);
            break;
         case VERT_RESULT_CLIP_VERTEX:
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_CLIPVERTEX;
            stvp->output_semantic_index[slot] = 0;
            break;

         case VERT_RESULT_TEX0:
         case VERT_RESULT_TEX1:
         case VERT_RESULT_TEX2:
         case VERT_RESULT_TEX3:
         case VERT_RESULT_TEX4:
         case VERT_RESULT_TEX5:
         case VERT_RESULT_TEX6:
         case VERT_RESULT_TEX7:
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            stvp->output_semantic_index[slot] = attr - VERT_RESULT_TEX0;
            break;

         case VERT_RESULT_VAR0:
         default:
            assert(attr < VERT_RESULT_MAX);
            stvp->output_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            stvp->output_semantic_index[slot] = (FRAG_ATTRIB_VAR0 - 
                                                FRAG_ATTRIB_TEX0 +
                                                attr - 
                                                VERT_RESULT_VAR0);
            break;
         }
      }
   }
   /* similar hack to above, presetup potentially unused edgeflag output */
   stvp->result_to_output[VERT_RESULT_EDGE] = stvp->num_outputs;
   stvp->output_semantic_name[stvp->num_outputs] = TGSI_SEMANTIC_EDGEFLAG;
   stvp->output_semantic_index[stvp->num_outputs] = 0;
}


/**
 * Translate a vertex program to create a new variant.
 */
static struct st_vp_variant *
st_translate_vertex_program(struct st_context *st,
                            struct st_vertex_program *stvp,
                            const struct st_vp_variant_key *key)
{
   struct st_vp_variant *vpv = CALLOC_STRUCT(st_vp_variant);
   struct pipe_context *pipe = st->pipe;
   struct ureg_program *ureg;
   enum pipe_error error;
   unsigned num_outputs;

   st_prepare_vertex_program(st->ctx, stvp);

   if (!stvp->glsl_to_tgsi)
   {
      _mesa_remove_output_reads(&stvp->Base.Base, PROGRAM_OUTPUT);
      _mesa_remove_output_reads(&stvp->Base.Base, PROGRAM_VARYING);
   }

   ureg = ureg_create( TGSI_PROCESSOR_VERTEX );
   if (ureg == NULL) {
      FREE(vpv);
      return NULL;
   }

   vpv->key = *key;

   vpv->num_inputs = stvp->num_inputs;
   num_outputs = stvp->num_outputs;
   if (key->passthrough_edgeflags) {
      vpv->num_inputs++;
      num_outputs++;
   }

   if (ST_DEBUG & DEBUG_MESA) {
      _mesa_print_program(&stvp->Base.Base);
      _mesa_print_program_parameters(st->ctx, &stvp->Base.Base);
      debug_printf("\n");
   }

   if (stvp->glsl_to_tgsi)
      error = st_translate_program(st->ctx,
                                   TGSI_PROCESSOR_VERTEX,
                                   ureg,
                                   stvp->glsl_to_tgsi,
                                   &stvp->Base.Base,
                                   /* inputs */
                                   stvp->num_inputs,
                                   stvp->input_to_index,
                                   NULL, /* input semantic name */
                                   NULL, /* input semantic index */
                                   NULL, /* interp mode */
                                   /* outputs */
                                   stvp->num_outputs,
                                   stvp->result_to_output,
                                   stvp->output_semantic_name,
                                   stvp->output_semantic_index,
                                   key->passthrough_edgeflags );
   else
      error = st_translate_mesa_program(st->ctx,
                                        TGSI_PROCESSOR_VERTEX,
                                        ureg,
                                        &stvp->Base.Base,
                                        /* inputs */
                                        vpv->num_inputs,
                                        stvp->input_to_index,
                                        NULL, /* input semantic name */
                                        NULL, /* input semantic index */
                                        NULL,
                                        /* outputs */
                                        num_outputs,
                                        stvp->result_to_output,
                                        stvp->output_semantic_name,
                                        stvp->output_semantic_index,
                                        key->passthrough_edgeflags );

   if (error)
      goto fail;

   vpv->tgsi.tokens = ureg_get_tokens( ureg, NULL );
   if (!vpv->tgsi.tokens)
      goto fail;

   ureg_destroy( ureg );

   if (stvp->glsl_to_tgsi) {
      st_translate_stream_output_info(stvp->glsl_to_tgsi,
                                      stvp->result_to_output,
                                      &vpv->tgsi.stream_output);
   }

   vpv->driver_shader = pipe->create_vs_state(pipe, &vpv->tgsi);

   if (ST_DEBUG & DEBUG_TGSI) {
      tgsi_dump( vpv->tgsi.tokens, 0 );
      debug_printf("\n");
   }

   return vpv;

fail:
   debug_printf("%s: failed to translate Mesa program:\n", __FUNCTION__);
   _mesa_print_program(&stvp->Base.Base);
   debug_assert(0);

   ureg_destroy( ureg );
   return NULL;
}


/**
 * Find/create a vertex program variant.
 */
struct st_vp_variant *
st_get_vp_variant(struct st_context *st,
                  struct st_vertex_program *stvp,
                  const struct st_vp_variant_key *key)
{
   struct st_vp_variant *vpv;

   /* Search for existing variant */
   for (vpv = stvp->variants; vpv; vpv = vpv->next) {
      if (memcmp(&vpv->key, key, sizeof(*key)) == 0) {
         break;
      }
   }

   if (!vpv) {
      /* create now */
      vpv = st_translate_vertex_program(st, stvp, key);
      if (vpv) {
         /* insert into list */
         vpv->next = stvp->variants;
         stvp->variants = vpv;
      }
   }

   return vpv;
}


static unsigned
st_translate_interp(enum glsl_interp_qualifier glsl_qual, bool is_color)
{
   switch (glsl_qual) {
   case INTERP_QUALIFIER_NONE:
      if (is_color)
         return TGSI_INTERPOLATE_COLOR;
      return TGSI_INTERPOLATE_PERSPECTIVE;
   case INTERP_QUALIFIER_SMOOTH:
      return TGSI_INTERPOLATE_PERSPECTIVE;
   case INTERP_QUALIFIER_FLAT:
      return TGSI_INTERPOLATE_CONSTANT;
   case INTERP_QUALIFIER_NOPERSPECTIVE:
      return TGSI_INTERPOLATE_LINEAR;
   default:
      assert(0 && "unexpected interp mode in st_translate_interp()");
      return TGSI_INTERPOLATE_PERSPECTIVE;
   }
}


/**
 * Translate a Mesa fragment shader into a TGSI shader using extra info in
 * the key.
 * \return  new fragment program variant
 */
static struct st_fp_variant *
st_translate_fragment_program(struct st_context *st,
                              struct st_fragment_program *stfp,
                              const struct st_fp_variant_key *key)
{
   struct pipe_context *pipe = st->pipe;
   struct st_fp_variant *variant = CALLOC_STRUCT(st_fp_variant);
   GLboolean deleteFP = GL_FALSE;

   if (!variant)
      return NULL;

   assert(!(key->bitmap && key->drawpixels));

#if FEATURE_drawpix
   if (key->bitmap) {
      /* glBitmap drawing */
      struct gl_fragment_program *fp; /* we free this temp program below */

      st_make_bitmap_fragment_program(st, &stfp->Base,
                                      &fp, &variant->bitmap_sampler);

      variant->parameters = _mesa_clone_parameter_list(fp->Base.Parameters);
      stfp = st_fragment_program(fp);
      deleteFP = GL_TRUE;
   }
   else if (key->drawpixels) {
      /* glDrawPixels drawing */
      struct gl_fragment_program *fp; /* we free this temp program below */

      if (key->drawpixels_z || key->drawpixels_stencil) {
         fp = st_make_drawpix_z_stencil_program(st, key->drawpixels_z,
                                                key->drawpixels_stencil);
      }
      else {
         /* RGBA */
         st_make_drawpix_fragment_program(st, &stfp->Base, &fp);
         variant->parameters = _mesa_clone_parameter_list(fp->Base.Parameters);
         deleteFP = GL_TRUE;
      }
      stfp = st_fragment_program(fp);
   }
#endif

   if (!stfp->tgsi.tokens) {
      /* need to translate Mesa instructions to TGSI now */
      GLuint outputMapping[FRAG_RESULT_MAX];
      GLuint inputMapping[FRAG_ATTRIB_MAX];
      GLuint interpMode[PIPE_MAX_SHADER_INPUTS];  /* XXX size? */
      GLuint attr;
      const GLbitfield64 inputsRead = stfp->Base.Base.InputsRead;
      struct ureg_program *ureg;

      GLboolean write_all = GL_FALSE;

      ubyte input_semantic_name[PIPE_MAX_SHADER_INPUTS];
      ubyte input_semantic_index[PIPE_MAX_SHADER_INPUTS];
      uint fs_num_inputs = 0;

      ubyte fs_output_semantic_name[PIPE_MAX_SHADER_OUTPUTS];
      ubyte fs_output_semantic_index[PIPE_MAX_SHADER_OUTPUTS];
      uint fs_num_outputs = 0;
      
      if (!stfp->glsl_to_tgsi)
         _mesa_remove_output_reads(&stfp->Base.Base, PROGRAM_OUTPUT);

      /*
       * Convert Mesa program inputs to TGSI input register semantics.
       */
      for (attr = 0; attr < FRAG_ATTRIB_MAX; attr++) {
         if ((inputsRead & BITFIELD64_BIT(attr)) != 0) {
            const GLuint slot = fs_num_inputs++;

            inputMapping[attr] = slot;

            switch (attr) {
            case FRAG_ATTRIB_WPOS:
               input_semantic_name[slot] = TGSI_SEMANTIC_POSITION;
               input_semantic_index[slot] = 0;
               interpMode[slot] = TGSI_INTERPOLATE_LINEAR;
               break;
            case FRAG_ATTRIB_COL0:
               input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
               input_semantic_index[slot] = 0;
               interpMode[slot] = st_translate_interp(stfp->Base.InterpQualifier[attr],
                                                      TRUE);
               break;
            case FRAG_ATTRIB_COL1:
               input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
               input_semantic_index[slot] = 1;
               interpMode[slot] = st_translate_interp(stfp->Base.InterpQualifier[attr],
                                                      TRUE);
               break;
            case FRAG_ATTRIB_FOGC:
               input_semantic_name[slot] = TGSI_SEMANTIC_FOG;
               input_semantic_index[slot] = 0;
               interpMode[slot] = TGSI_INTERPOLATE_PERSPECTIVE;
               break;
            case FRAG_ATTRIB_FACE:
               input_semantic_name[slot] = TGSI_SEMANTIC_FACE;
               input_semantic_index[slot] = 0;
               interpMode[slot] = TGSI_INTERPOLATE_CONSTANT;
               break;
            case FRAG_ATTRIB_CLIP_DIST0:
               input_semantic_name[slot] = TGSI_SEMANTIC_CLIPDIST;
               input_semantic_index[slot] = 0;
               interpMode[slot] = TGSI_INTERPOLATE_LINEAR;
               break;
            case FRAG_ATTRIB_CLIP_DIST1:
               input_semantic_name[slot] = TGSI_SEMANTIC_CLIPDIST;
               input_semantic_index[slot] = 1;
               interpMode[slot] = TGSI_INTERPOLATE_LINEAR;
               break;
               /* In most cases, there is nothing special about these
                * inputs, so adopt a convention to use the generic
                * semantic name and the mesa FRAG_ATTRIB_ number as the
                * index. 
                * 
                * All that is required is that the vertex shader labels
                * its own outputs similarly, and that the vertex shader
                * generates at least every output required by the
                * fragment shader plus fixed-function hardware (such as
                * BFC).
                * 
                * There is no requirement that semantic indexes start at
                * zero or be restricted to a particular range -- nobody
                * should be building tables based on semantic index.
                */
            case FRAG_ATTRIB_PNTC:
            case FRAG_ATTRIB_TEX0:
            case FRAG_ATTRIB_TEX1:
            case FRAG_ATTRIB_TEX2:
            case FRAG_ATTRIB_TEX3:
            case FRAG_ATTRIB_TEX4:
            case FRAG_ATTRIB_TEX5:
            case FRAG_ATTRIB_TEX6:
            case FRAG_ATTRIB_TEX7:
            case FRAG_ATTRIB_VAR0:
            default:
               /* Actually, let's try and zero-base this just for
                * readability of the generated TGSI.
                */
               assert(attr >= FRAG_ATTRIB_TEX0);
               input_semantic_index[slot] = (attr - FRAG_ATTRIB_TEX0);
               input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
               if (attr == FRAG_ATTRIB_PNTC)
                  interpMode[slot] = TGSI_INTERPOLATE_LINEAR;
               else
                  interpMode[slot] = st_translate_interp(stfp->Base.InterpQualifier[attr],
                                                         FALSE);
               break;
            }
         }
         else {
            inputMapping[attr] = -1;
         }
      }

      /*
       * Semantics and mapping for outputs
       */
      {
         uint numColors = 0;
         GLbitfield64 outputsWritten = stfp->Base.Base.OutputsWritten;

         /* if z is written, emit that first */
         if (outputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {
            fs_output_semantic_name[fs_num_outputs] = TGSI_SEMANTIC_POSITION;
            fs_output_semantic_index[fs_num_outputs] = 0;
            outputMapping[FRAG_RESULT_DEPTH] = fs_num_outputs;
            fs_num_outputs++;
            outputsWritten &= ~(1 << FRAG_RESULT_DEPTH);
         }

         if (outputsWritten & BITFIELD64_BIT(FRAG_RESULT_STENCIL)) {
            fs_output_semantic_name[fs_num_outputs] = TGSI_SEMANTIC_STENCIL;
            fs_output_semantic_index[fs_num_outputs] = 0;
            outputMapping[FRAG_RESULT_STENCIL] = fs_num_outputs;
            fs_num_outputs++;
            outputsWritten &= ~(1 << FRAG_RESULT_STENCIL);
         }

         /* handle remaning outputs (color) */
         for (attr = 0; attr < FRAG_RESULT_MAX; attr++) {
            if (outputsWritten & BITFIELD64_BIT(attr)) {
               switch (attr) {
               case FRAG_RESULT_DEPTH:
               case FRAG_RESULT_STENCIL:
                  /* handled above */
                  assert(0);
                  break;
               case FRAG_RESULT_COLOR:
                  write_all = GL_TRUE; /* fallthrough */
               default:
                  assert(attr == FRAG_RESULT_COLOR ||
                         (FRAG_RESULT_DATA0 <= attr && attr < FRAG_RESULT_MAX));
                  fs_output_semantic_name[fs_num_outputs] = TGSI_SEMANTIC_COLOR;
                  fs_output_semantic_index[fs_num_outputs] = numColors;
                  outputMapping[attr] = fs_num_outputs;
                  numColors++;
                  break;
               }

               fs_num_outputs++;
            }
         }
      }

      ureg = ureg_create( TGSI_PROCESSOR_FRAGMENT );
      if (ureg == NULL) {
         FREE(variant);
         return NULL;
      }

      if (ST_DEBUG & DEBUG_MESA) {
         _mesa_print_program(&stfp->Base.Base);
         _mesa_print_program_parameters(st->ctx, &stfp->Base.Base);
         debug_printf("\n");
      }
      if (write_all == GL_TRUE)
         ureg_property_fs_color0_writes_all_cbufs(ureg, 1);

      if (stfp->Base.FragDepthLayout != FRAG_DEPTH_LAYOUT_NONE) {
         switch (stfp->Base.FragDepthLayout) {
         case FRAG_DEPTH_LAYOUT_ANY:
            ureg_property_fs_depth_layout(ureg, TGSI_FS_DEPTH_LAYOUT_ANY);
            break;
         case FRAG_DEPTH_LAYOUT_GREATER:
            ureg_property_fs_depth_layout(ureg, TGSI_FS_DEPTH_LAYOUT_GREATER);
            break;
         case FRAG_DEPTH_LAYOUT_LESS:
            ureg_property_fs_depth_layout(ureg, TGSI_FS_DEPTH_LAYOUT_LESS);
            break;
         case FRAG_DEPTH_LAYOUT_UNCHANGED:
            ureg_property_fs_depth_layout(ureg, TGSI_FS_DEPTH_LAYOUT_UNCHANGED);
            break;
         default:
            assert(0);
         }
      }

      if (stfp->glsl_to_tgsi)
         st_translate_program(st->ctx,
                              TGSI_PROCESSOR_FRAGMENT,
                              ureg,
                              stfp->glsl_to_tgsi,
                              &stfp->Base.Base,
                              /* inputs */
                              fs_num_inputs,
                              inputMapping,
                              input_semantic_name,
                              input_semantic_index,
                              interpMode,
                              /* outputs */
                              fs_num_outputs,
                              outputMapping,
                              fs_output_semantic_name,
                              fs_output_semantic_index, FALSE );
      else
         st_translate_mesa_program(st->ctx,
                                   TGSI_PROCESSOR_FRAGMENT,
                                   ureg,
                                   &stfp->Base.Base,
                                   /* inputs */
                                   fs_num_inputs,
                                   inputMapping,
                                   input_semantic_name,
                                   input_semantic_index,
                                   interpMode,
                                   /* outputs */
                                   fs_num_outputs,
                                   outputMapping,
                                   fs_output_semantic_name,
                                   fs_output_semantic_index, FALSE );

      stfp->tgsi.tokens = ureg_get_tokens( ureg, NULL );
      ureg_destroy( ureg );
   }

   /* fill in variant */
   variant->driver_shader = pipe->create_fs_state(pipe, &stfp->tgsi);
   variant->key = *key;

   if (ST_DEBUG & DEBUG_TGSI) {
      tgsi_dump( stfp->tgsi.tokens, 0/*TGSI_DUMP_VERBOSE*/ );
      debug_printf("\n");
   }

   if (deleteFP) {
      /* Free the temporary program made above */
      struct gl_fragment_program *fp = &stfp->Base;
      _mesa_reference_fragprog(st->ctx, &fp, NULL);
   }

   return variant;
}


/**
 * Translate fragment program if needed.
 */
struct st_fp_variant *
st_get_fp_variant(struct st_context *st,
                  struct st_fragment_program *stfp,
                  const struct st_fp_variant_key *key)
{
   struct st_fp_variant *fpv;

   /* Search for existing variant */
   for (fpv = stfp->variants; fpv; fpv = fpv->next) {
      if (memcmp(&fpv->key, key, sizeof(*key)) == 0) {
         break;
      }
   }

   if (!fpv) {
      /* create new */
      fpv = st_translate_fragment_program(st, stfp, key);
      if (fpv) {
         /* insert into list */
         fpv->next = stfp->variants;
         stfp->variants = fpv;
      }
   }

   return fpv;
}


/**
 * Translate a geometry program to create a new variant.
 */
static struct st_gp_variant *
st_translate_geometry_program(struct st_context *st,
                              struct st_geometry_program *stgp,
                              const struct st_gp_variant_key *key)
{
   GLuint inputMapping[GEOM_ATTRIB_MAX];
   GLuint outputMapping[GEOM_RESULT_MAX];
   struct pipe_context *pipe = st->pipe;
   GLuint attr;
   const GLbitfield64 inputsRead = stgp->Base.Base.InputsRead;
   GLuint vslot = 0;
   GLuint num_generic = 0;

   uint gs_num_inputs = 0;
   uint gs_builtin_inputs = 0;
   uint gs_array_offset = 0;

   ubyte gs_output_semantic_name[PIPE_MAX_SHADER_OUTPUTS];
   ubyte gs_output_semantic_index[PIPE_MAX_SHADER_OUTPUTS];
   uint gs_num_outputs = 0;

   GLint i;
   GLuint maxSlot = 0;
   struct ureg_program *ureg;

   struct st_gp_variant *gpv;

   gpv = CALLOC_STRUCT(st_gp_variant);
   if (!gpv)
      return NULL;

   _mesa_remove_output_reads(&stgp->Base.Base, PROGRAM_OUTPUT);
   _mesa_remove_output_reads(&stgp->Base.Base, PROGRAM_VARYING);

   ureg = ureg_create( TGSI_PROCESSOR_GEOMETRY );
   if (ureg == NULL) {
      FREE(gpv);
      return NULL;
   }

   /* which vertex output goes to the first geometry input */
   vslot = 0;

   memset(inputMapping, 0, sizeof(inputMapping));
   memset(outputMapping, 0, sizeof(outputMapping));

   /*
    * Convert Mesa program inputs to TGSI input register semantics.
    */
   for (attr = 0; attr < GEOM_ATTRIB_MAX; attr++) {
      if ((inputsRead & BITFIELD64_BIT(attr)) != 0) {
         const GLuint slot = gs_num_inputs;

         gs_num_inputs++;

         inputMapping[attr] = slot;

         stgp->input_map[slot + gs_array_offset] = vslot - gs_builtin_inputs;
         stgp->input_to_index[attr] = vslot;
         stgp->index_to_input[vslot] = attr;
         ++vslot;

         if (attr != GEOM_ATTRIB_PRIMITIVE_ID) {
            gs_array_offset += 2;
         } else
            ++gs_builtin_inputs;

#if 0
         debug_printf("input map at %d = %d\n",
                      slot + gs_array_offset, stgp->input_map[slot + gs_array_offset]);
#endif

         switch (attr) {
         case GEOM_ATTRIB_PRIMITIVE_ID:
            stgp->input_semantic_name[slot] = TGSI_SEMANTIC_PRIMID;
            stgp->input_semantic_index[slot] = 0;
            break;
         case GEOM_ATTRIB_POSITION:
            stgp->input_semantic_name[slot] = TGSI_SEMANTIC_POSITION;
            stgp->input_semantic_index[slot] = 0;
            break;
         case GEOM_ATTRIB_COLOR0:
            stgp->input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            stgp->input_semantic_index[slot] = 0;
            break;
         case GEOM_ATTRIB_COLOR1:
            stgp->input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            stgp->input_semantic_index[slot] = 1;
            break;
         case GEOM_ATTRIB_FOG_FRAG_COORD:
            stgp->input_semantic_name[slot] = TGSI_SEMANTIC_FOG;
            stgp->input_semantic_index[slot] = 0;
            break;
         case GEOM_ATTRIB_TEX_COORD:
            stgp->input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            stgp->input_semantic_index[slot] = num_generic++;
            break;
         case GEOM_ATTRIB_VAR0:
            /* fall-through */
         default:
            stgp->input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            stgp->input_semantic_index[slot] = num_generic++;
         }
      }
   }

   /* initialize output semantics to defaults */
   for (i = 0; i < PIPE_MAX_SHADER_OUTPUTS; i++) {
      gs_output_semantic_name[i] = TGSI_SEMANTIC_GENERIC;
      gs_output_semantic_index[i] = 0;
   }

   num_generic = 0;
   /*
    * Determine number of outputs, the (default) output register
    * mapping and the semantic information for each output.
    */
   for (attr = 0; attr < GEOM_RESULT_MAX; attr++) {
      if (stgp->Base.Base.OutputsWritten & BITFIELD64_BIT(attr)) {
         GLuint slot;

         slot = gs_num_outputs;
         gs_num_outputs++;
         outputMapping[attr] = slot;

         switch (attr) {
         case GEOM_RESULT_POS:
            assert(slot == 0);
            gs_output_semantic_name[slot] = TGSI_SEMANTIC_POSITION;
            gs_output_semantic_index[slot] = 0;
            break;
         case GEOM_RESULT_COL0:
            gs_output_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            gs_output_semantic_index[slot] = 0;
            break;
         case GEOM_RESULT_COL1:
            gs_output_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            gs_output_semantic_index[slot] = 1;
            break;
         case GEOM_RESULT_SCOL0:
            gs_output_semantic_name[slot] = TGSI_SEMANTIC_BCOLOR;
            gs_output_semantic_index[slot] = 0;
            break;
         case GEOM_RESULT_SCOL1:
            gs_output_semantic_name[slot] = TGSI_SEMANTIC_BCOLOR;
            gs_output_semantic_index[slot] = 1;
            break;
         case GEOM_RESULT_FOGC:
            gs_output_semantic_name[slot] = TGSI_SEMANTIC_FOG;
            gs_output_semantic_index[slot] = 0;
            break;
         case GEOM_RESULT_PSIZ:
            gs_output_semantic_name[slot] = TGSI_SEMANTIC_PSIZE;
            gs_output_semantic_index[slot] = 0;
            break;
         case GEOM_RESULT_TEX0:
         case GEOM_RESULT_TEX1:
         case GEOM_RESULT_TEX2:
         case GEOM_RESULT_TEX3:
         case GEOM_RESULT_TEX4:
         case GEOM_RESULT_TEX5:
         case GEOM_RESULT_TEX6:
         case GEOM_RESULT_TEX7:
            /* fall-through */
         case GEOM_RESULT_VAR0:
            /* fall-through */
         default:
            assert(slot < Elements(gs_output_semantic_name));
            /* use default semantic info */
            gs_output_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            gs_output_semantic_index[slot] = num_generic++;
         }
      }
   }

   assert(gs_output_semantic_name[0] == TGSI_SEMANTIC_POSITION);

   /* find max output slot referenced to compute gs_num_outputs */
   for (attr = 0; attr < GEOM_RESULT_MAX; attr++) {
      if (outputMapping[attr] != ~0 && outputMapping[attr] > maxSlot)
         maxSlot = outputMapping[attr];
   }
   gs_num_outputs = maxSlot + 1;

#if 0 /* debug */
   {
      GLuint i;
      printf("outputMapping? %d\n", outputMapping ? 1 : 0);
      if (outputMapping) {
         printf("attr -> slot\n");
         for (i = 0; i < 16;  i++) {
            printf(" %2d       %3d\n", i, outputMapping[i]);
         }
      }
      printf("slot    sem_name  sem_index\n");
      for (i = 0; i < gs_num_outputs; i++) {
         printf(" %2d         %d         %d\n",
                i,
                gs_output_semantic_name[i],
                gs_output_semantic_index[i]);
      }
   }
#endif

   /* free old shader state, if any */
   if (stgp->tgsi.tokens) {
      st_free_tokens(stgp->tgsi.tokens);
      stgp->tgsi.tokens = NULL;
   }

   ureg_property_gs_input_prim(ureg, stgp->Base.InputType);
   ureg_property_gs_output_prim(ureg, stgp->Base.OutputType);
   ureg_property_gs_max_vertices(ureg, stgp->Base.VerticesOut);

   st_translate_mesa_program(st->ctx,
                             TGSI_PROCESSOR_GEOMETRY,
                             ureg,
                             &stgp->Base.Base,
                             /* inputs */
                             gs_num_inputs,
                             inputMapping,
                             stgp->input_semantic_name,
                             stgp->input_semantic_index,
                             NULL,
                             /* outputs */
                             gs_num_outputs,
                             outputMapping,
                             gs_output_semantic_name,
                             gs_output_semantic_index,
                             FALSE);

   stgp->num_inputs = gs_num_inputs;
   stgp->tgsi.tokens = ureg_get_tokens( ureg, NULL );
   ureg_destroy( ureg );

   if (stgp->glsl_to_tgsi) {
      st_translate_stream_output_info(stgp->glsl_to_tgsi,
                                      outputMapping,
                                      &stgp->tgsi.stream_output);
   }

   /* fill in new variant */
   gpv->driver_shader = pipe->create_gs_state(pipe, &stgp->tgsi);
   gpv->key = *key;

   if ((ST_DEBUG & DEBUG_TGSI) && (ST_DEBUG & DEBUG_MESA)) {
      _mesa_print_program(&stgp->Base.Base);
      debug_printf("\n");
   }

   if (ST_DEBUG & DEBUG_TGSI) {
      tgsi_dump(stgp->tgsi.tokens, 0);
      debug_printf("\n");
   }

   return gpv;
}


/**
 * Get/create geometry program variant.
 */
struct st_gp_variant *
st_get_gp_variant(struct st_context *st,
                  struct st_geometry_program *stgp,
                  const struct st_gp_variant_key *key)
{
   struct st_gp_variant *gpv;

   /* Search for existing variant */
   for (gpv = stgp->variants; gpv; gpv = gpv->next) {
      if (memcmp(&gpv->key, key, sizeof(*key)) == 0) {
         break;
      }
   }

   if (!gpv) {
      /* create new */
      gpv = st_translate_geometry_program(st, stgp, key);
      if (gpv) {
         /* insert into list */
         gpv->next = stgp->variants;
         stgp->variants = gpv;
      }
   }

   return gpv;
}




/**
 * Debug- print current shader text
 */
void
st_print_shaders(struct gl_context *ctx)
{
   struct gl_shader_program *shProg[3] = {
      ctx->Shader.CurrentVertexProgram,
      ctx->Shader.CurrentGeometryProgram,
      ctx->Shader.CurrentFragmentProgram,
   };
   unsigned j;

   for (j = 0; j < 3; j++) {
      unsigned i;

      if (shProg[j] == NULL)
	 continue;

      for (i = 0; i < shProg[j]->NumShaders; i++) {
	 struct gl_shader *sh;

	 switch (shProg[j]->Shaders[i]->Type) {
	 case GL_VERTEX_SHADER:
	    sh = (i != 0) ? NULL : shProg[j]->Shaders[i];
	    break;
	 case GL_GEOMETRY_SHADER_ARB:
	    sh = (i != 1) ? NULL : shProg[j]->Shaders[i];
	    break;
	 case GL_FRAGMENT_SHADER:
	    sh = (i != 2) ? NULL : shProg[j]->Shaders[i];
	    break;
	 default:
	    assert(0);
	    sh = NULL;
	    break;
	 }

	 if (sh != NULL) {
	    printf("GLSL shader %u of %u:\n", i, shProg[j]->NumShaders);
	    printf("%s\n", sh->Source);
	 }
      }
   }
}


/**
 * Vert/Geom/Frag programs have per-context variants.  Free all the
 * variants attached to the given program which match the given context.
 */
static void
destroy_program_variants(struct st_context *st, struct gl_program *program)
{
   if (!program)
      return;

   switch (program->Target) {
   case GL_VERTEX_PROGRAM_ARB:
      {
         struct st_vertex_program *stvp = (struct st_vertex_program *) program;
         struct st_vp_variant *vpv, **prevPtr = &stvp->variants;

         for (vpv = stvp->variants; vpv; ) {
            struct st_vp_variant *next = vpv->next;
            if (vpv->key.st == st) {
               /* unlink from list */
               *prevPtr = next;
               /* destroy this variant */
               delete_vp_variant(st, vpv);
            }
            else {
               prevPtr = &vpv->next;
            }
            vpv = next;
         }
      }
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      {
         struct st_fragment_program *stfp =
            (struct st_fragment_program *) program;
         struct st_fp_variant *fpv, **prevPtr = &stfp->variants;

         for (fpv = stfp->variants; fpv; ) {
            struct st_fp_variant *next = fpv->next;
            if (fpv->key.st == st) {
               /* unlink from list */
               *prevPtr = next;
               /* destroy this variant */
               delete_fp_variant(st, fpv);
            }
            else {
               prevPtr = &fpv->next;
            }
            fpv = next;
         }
      }
      break;
   case MESA_GEOMETRY_PROGRAM:
      {
         struct st_geometry_program *stgp =
            (struct st_geometry_program *) program;
         struct st_gp_variant *gpv, **prevPtr = &stgp->variants;

         for (gpv = stgp->variants; gpv; ) {
            struct st_gp_variant *next = gpv->next;
            if (gpv->key.st == st) {
               /* unlink from list */
               *prevPtr = next;
               /* destroy this variant */
               delete_gp_variant(st, gpv);
            }
            else {
               prevPtr = &gpv->next;
            }
            gpv = next;
         }
      }
      break;
   default:
      _mesa_problem(NULL, "Unexpected program target 0x%x in "
                    "destroy_program_variants_cb()", program->Target);
   }
}


/**
 * Callback for _mesa_HashWalk.  Free all the shader's program variants
 * which match the given context.
 */
static void
destroy_shader_program_variants_cb(GLuint key, void *data, void *userData)
{
   struct st_context *st = (struct st_context *) userData;
   struct gl_shader *shader = (struct gl_shader *) data;

   switch (shader->Type) {
   case GL_SHADER_PROGRAM_MESA:
      {
         struct gl_shader_program *shProg = (struct gl_shader_program *) data;
         GLuint i;

         for (i = 0; i < shProg->NumShaders; i++) {
            destroy_program_variants(st, shProg->Shaders[i]->Program);
         }

	 for (i = 0; i < Elements(shProg->_LinkedShaders); i++) {
	    if (shProg->_LinkedShaders[i])
               destroy_program_variants(st, shProg->_LinkedShaders[i]->Program);
	 }
      }
      break;
   case GL_VERTEX_SHADER:
   case GL_FRAGMENT_SHADER:
   case GL_GEOMETRY_SHADER:
      {
         destroy_program_variants(st, shader->Program);
      }
      break;
   default:
      assert(0);
   }
}


/**
 * Callback for _mesa_HashWalk.  Free all the program variants which match
 * the given context.
 */
static void
destroy_program_variants_cb(GLuint key, void *data, void *userData)
{
   struct st_context *st = (struct st_context *) userData;
   struct gl_program *program = (struct gl_program *) data;
   destroy_program_variants(st, program);
}


/**
 * Walk over all shaders and programs to delete any variants which
 * belong to the given context.
 * This is called during context tear-down.
 */
void
st_destroy_program_variants(struct st_context *st)
{
   /* ARB vert/frag program */
   _mesa_HashWalk(st->ctx->Shared->Programs,
                  destroy_program_variants_cb, st);

   /* GLSL vert/frag/geom shaders */
   _mesa_HashWalk(st->ctx->Shared->ShaderObjects,
                  destroy_shader_program_variants_cb, st);
}
