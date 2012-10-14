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
 * Generate fragment programs to implement pixel transfer ops, such as
 * scale/bias, colortable, convolution...
 *
 * Authors:
 *   Brian Paul
 */

#include "main/imports.h"
#include "main/image.h"
#include "main/macros.h"
#include "program/program.h"
#include "program/prog_cache.h"
#include "program/prog_instruction.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"

#include "st_context.h"
#include "st_format.h"
#include "st_texture.h"

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_pack_color.h"


struct state_key
{
   GLuint scaleAndBias:1;
   GLuint pixelMaps:1;

#if 0
   GLfloat Maps[3][256][4];
   int NumMaps;
   GLint NumStages;
   pipeline_stage Stages[STAGE_MAX];
   GLboolean StagesUsed[STAGE_MAX];
   GLfloat Scale1[4], Bias1[4];
   GLfloat Scale2[4], Bias2[4];
#endif
};

static void
make_state_key(struct gl_context *ctx,  struct state_key *key)
{
   memset(key, 0, sizeof(*key));

   if (ctx->Pixel.RedBias != 0.0 || ctx->Pixel.RedScale != 1.0 ||
       ctx->Pixel.GreenBias != 0.0 || ctx->Pixel.GreenScale != 1.0 ||
       ctx->Pixel.BlueBias != 0.0 || ctx->Pixel.BlueScale != 1.0 ||
       ctx->Pixel.AlphaBias != 0.0 || ctx->Pixel.AlphaScale != 1.0) {
      key->scaleAndBias = 1;
   }

   key->pixelMaps = ctx->Pixel.MapColorFlag;
}


/**
 * Update the pixelmap texture with the contents of the R/G/B/A pixel maps.
 */
static void
load_color_map_texture(struct gl_context *ctx, struct pipe_resource *pt)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_transfer *transfer;
   const GLuint rSize = ctx->PixelMaps.RtoR.Size;
   const GLuint gSize = ctx->PixelMaps.GtoG.Size;
   const GLuint bSize = ctx->PixelMaps.BtoB.Size;
   const GLuint aSize = ctx->PixelMaps.AtoA.Size;
   const uint texSize = pt->width0;
   uint *dest;
   uint i, j;

   transfer = pipe_get_transfer(pipe,
                                pt, 0, 0, PIPE_TRANSFER_WRITE,
                                0, 0, texSize, texSize);
   dest = (uint *) pipe_transfer_map(pipe, transfer);

   /* Pack four 1D maps into a 2D texture:
    * R map is placed horizontally, indexed by S, in channel 0
    * G map is placed vertically, indexed by T, in channel 1
    * B map is placed horizontally, indexed by S, in channel 2
    * A map is placed vertically, indexed by T, in channel 3
    */
   for (i = 0; i < texSize; i++) {
      for (j = 0; j < texSize; j++) {
         union util_color uc;
         int k = (i * texSize + j);
         ubyte r = ctx->PixelMaps.RtoR.Map8[j * rSize / texSize];
         ubyte g = ctx->PixelMaps.GtoG.Map8[i * gSize / texSize];
         ubyte b = ctx->PixelMaps.BtoB.Map8[j * bSize / texSize];
         ubyte a = ctx->PixelMaps.AtoA.Map8[i * aSize / texSize];
         util_pack_color_ub(r, g, b, a, pt->format, &uc);
         *(dest + k) = uc.ui;
      }
   }

   pipe_transfer_unmap(pipe, transfer);
   pipe->transfer_destroy(pipe, transfer);
}



#define MAX_INST 100

/**
 * Returns a fragment program which implements the current pixel transfer ops.
 */
static struct gl_fragment_program *
get_pixel_transfer_program(struct gl_context *ctx, const struct state_key *key)
{
   struct st_context *st = st_context(ctx);
   struct prog_instruction inst[MAX_INST];
   struct gl_program_parameter_list *params;
   struct gl_fragment_program *fp;
   GLuint ic = 0;
   const GLuint colorTemp = 0;

   fp = (struct gl_fragment_program *)
      ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!fp)
      return NULL;

   params = _mesa_new_parameter_list();

   /*
    * Get initial pixel color from the texture.
    * TEX colorTemp, fragment.texcoord[0], texture[0], 2D;
    */
   _mesa_init_instructions(inst + ic, 1);
   inst[ic].Opcode = OPCODE_TEX;
   inst[ic].DstReg.File = PROGRAM_TEMPORARY;
   inst[ic].DstReg.Index = colorTemp;
   inst[ic].SrcReg[0].File = PROGRAM_INPUT;
   inst[ic].SrcReg[0].Index = FRAG_ATTRIB_TEX0;
   inst[ic].TexSrcUnit = 0;
   inst[ic].TexSrcTarget = TEXTURE_2D_INDEX;
   ic++;
   fp->Base.InputsRead = BITFIELD64_BIT(FRAG_ATTRIB_TEX0);
   fp->Base.OutputsWritten = BITFIELD64_BIT(FRAG_RESULT_COLOR);
   fp->Base.SamplersUsed = 0x1;  /* sampler 0 (bit 0) is used */

   if (key->scaleAndBias) {
      static const gl_state_index scale_state[STATE_LENGTH] =
         { STATE_INTERNAL, STATE_PT_SCALE, 0, 0, 0 };
      static const gl_state_index bias_state[STATE_LENGTH] =
         { STATE_INTERNAL, STATE_PT_BIAS, 0, 0, 0 };
      GLint scale_p, bias_p;

      scale_p = _mesa_add_state_reference(params, scale_state);
      bias_p = _mesa_add_state_reference(params, bias_state);

      /* MAD colorTemp, colorTemp, scale, bias; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_MAD;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = colorTemp;
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = colorTemp;
      inst[ic].SrcReg[1].File = PROGRAM_STATE_VAR;
      inst[ic].SrcReg[1].Index = scale_p;
      inst[ic].SrcReg[2].File = PROGRAM_STATE_VAR;
      inst[ic].SrcReg[2].Index = bias_p;
      ic++;
   }

   if (key->pixelMaps) {
      const GLuint temp = 1;

      /* create the colormap/texture now if not already done */
      if (!st->pixel_xfer.pixelmap_texture) {
         st->pixel_xfer.pixelmap_texture = st_create_color_map_texture(ctx);
         st->pixel_xfer.pixelmap_sampler_view =
            st_create_texture_sampler_view(st->pipe,
                                           st->pixel_xfer.pixelmap_texture);
      }

      /* with a little effort, we can do four pixel map look-ups with
       * two TEX instructions:
       */

      /* TEX temp.rg, colorTemp.rgba, texture[1], 2D; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_TEX;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = temp;
      inst[ic].DstReg.WriteMask = WRITEMASK_XY; /* write R,G */
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = colorTemp;
      inst[ic].TexSrcUnit = 1;
      inst[ic].TexSrcTarget = TEXTURE_2D_INDEX;
      ic++;

      /* TEX temp.ba, colorTemp.baba, texture[1], 2D; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_TEX;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = temp;
      inst[ic].DstReg.WriteMask = WRITEMASK_ZW; /* write B,A */
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = colorTemp;
      inst[ic].SrcReg[0].Swizzle = MAKE_SWIZZLE4(SWIZZLE_Z, SWIZZLE_W,
                                                 SWIZZLE_Z, SWIZZLE_W);
      inst[ic].TexSrcUnit = 1;
      inst[ic].TexSrcTarget = TEXTURE_2D_INDEX;
      ic++;

      /* MOV colorTemp, temp; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_MOV;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = colorTemp;
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = temp;
      ic++;

      fp->Base.SamplersUsed |= (1 << 1);  /* sampler 1 is used */
   }

   /* Modify last instruction's dst reg to write to result.color */
   {
      struct prog_instruction *last = &inst[ic - 1];
      last->DstReg.File = PROGRAM_OUTPUT;
      last->DstReg.Index = FRAG_RESULT_COLOR;
   }

   /* END; */
   _mesa_init_instructions(inst + ic, 1);
   inst[ic].Opcode = OPCODE_END;
   ic++;

   assert(ic <= MAX_INST);


   fp->Base.Instructions = _mesa_alloc_instructions(ic);
   if (!fp->Base.Instructions) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "generating pixel transfer program");
      _mesa_free_parameter_list(params);
      return NULL;
   }

   _mesa_copy_instructions(fp->Base.Instructions, inst, ic);
   fp->Base.NumInstructions = ic;
   fp->Base.Parameters = params;

#if 0
   printf("========= pixel transfer prog\n");
   _mesa_print_program(&fp->Base);
   _mesa_print_parameter_list(fp->Base.Parameters);
#endif

   return fp;
}



/**
 * Update st->pixel_xfer.program in response to new pixel-transfer state.
 */
static void
update_pixel_transfer(struct st_context *st)
{
   struct gl_context *ctx = st->ctx;
   struct state_key key;
   struct gl_fragment_program *fp;

   make_state_key(st->ctx, &key);

   fp = (struct gl_fragment_program *)
      _mesa_search_program_cache(st->pixel_xfer.cache, &key, sizeof(key));
   if (!fp) {
      fp = get_pixel_transfer_program(st->ctx, &key);
      _mesa_program_cache_insert(st->ctx, st->pixel_xfer.cache,
                                 &key, sizeof(key), &fp->Base);
   }

   if (ctx->Pixel.MapColorFlag) {
      load_color_map_texture(ctx, st->pixel_xfer.pixelmap_texture);
   }
   st->pixel_xfer.pixelmap_enabled = ctx->Pixel.MapColorFlag;

   st->pixel_xfer.program = (struct st_fragment_program *) fp;
}



const struct st_tracked_state st_update_pixel_transfer = {
   "st_update_pixel_transfer",				/* name */
   {							/* dirty */
      _NEW_PIXEL,					/* mesa */
      0,						/* st */
   },
   update_pixel_transfer				/* update */
};
