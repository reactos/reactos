/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2010 VMware, Inc.
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
 * Polygon stipple helper module.  Drivers/GPUs which don't support polygon
 * stipple natively can use this module to simulate it.
 *
 * Basically, modify fragment shader to sample the 32x32 stipple pattern
 * texture and do a fragment kill for the 'off' bits.
 *
 * This was originally a 'draw' module stage, but since we don't need
 * vertex window coords or anything, it can be a stand-alone utility module.
 *
 * Authors:  Brian Paul
 */


#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_inlines.h"

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_pstipple.h"
#include "util/u_sampler.h"

#include "tgsi/tgsi_transform.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_scan.h"

/** Approx number of new tokens for instructions in pstip_transform_inst() */
#define NUM_NEW_TOKENS 50


static void
util_pstipple_update_stipple_texture(struct pipe_context *pipe,
                                     struct pipe_resource *tex,
                                     const uint32_t pattern[32])
{
   static const uint bit31 = 1 << 31;
   struct pipe_transfer *transfer;
   ubyte *data;
   int i, j;

   /* map texture memory */
   transfer = pipe_get_transfer(pipe, tex, 0, 0,
                                PIPE_TRANSFER_WRITE, 0, 0, 32, 32);
   data = pipe->transfer_map(pipe, transfer);

   /*
    * Load alpha texture.
    * Note: 0 means keep the fragment, 255 means kill it.
    * We'll negate the texel value and use KILP which kills if value
    * is negative.
    */
   for (i = 0; i < 32; i++) {
      for (j = 0; j < 32; j++) {
         if (pattern[i] & (bit31 >> j)) {
            /* fragment "on" */
            data[i * transfer->stride + j] = 0;
         }
         else {
            /* fragment "off" */
            data[i * transfer->stride + j] = 255;
         }
      }
   }

   /* unmap */
   pipe->transfer_unmap(pipe, transfer);
   pipe->transfer_destroy(pipe, transfer);
}


/**
 * Create a 32x32 alpha8 texture that encodes the given stipple pattern.
 */
struct pipe_resource *
util_pstipple_create_stipple_texture(struct pipe_context *pipe,
                                     const uint32_t pattern[32])
{
   struct pipe_screen *screen = pipe->screen;
   struct pipe_resource templat, *tex;

   memset(&templat, 0, sizeof(templat));
   templat.target = PIPE_TEXTURE_2D;
   templat.format = PIPE_FORMAT_A8_UNORM;
   templat.last_level = 0;
   templat.width0 = 32;
   templat.height0 = 32;
   templat.depth0 = 1;
   templat.array_size = 1;
   templat.bind = PIPE_BIND_SAMPLER_VIEW;

   tex = screen->resource_create(screen, &templat);

   if (tex)
      util_pstipple_update_stipple_texture(pipe, tex, pattern);

   return tex;
}


/**
 * Create sampler view to sample the stipple texture.
 */
struct pipe_sampler_view *
util_pstipple_create_sampler_view(struct pipe_context *pipe,
                                  struct pipe_resource *tex)
{
   struct pipe_sampler_view templat, *sv;

   u_sampler_view_default_template(&templat, tex, tex->format);
   sv = pipe->create_sampler_view(pipe, tex, &templat);

   return sv;
}


/**
 * Create the sampler CSO that'll be used for stippling.
 */
void *
util_pstipple_create_sampler(struct pipe_context *pipe)
{
   struct pipe_sampler_state templat;
   void *s;

   memset(&templat, 0, sizeof(templat));
   templat.wrap_s = PIPE_TEX_WRAP_REPEAT;
   templat.wrap_t = PIPE_TEX_WRAP_REPEAT;
   templat.wrap_r = PIPE_TEX_WRAP_REPEAT;
   templat.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   templat.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   templat.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   templat.normalized_coords = 1;
   templat.min_lod = 0.0f;
   templat.max_lod = 0.0f;

   s = pipe->create_sampler_state(pipe, &templat);
   return s;
}



/**
 * Subclass of tgsi_transform_context, used for transforming the
 * user's fragment shader to add the extra texture sample and fragment kill
 * instructions.
 */
struct pstip_transform_context {
   struct tgsi_transform_context base;
   struct tgsi_shader_info info;
   uint tempsUsed;  /**< bitmask */
   int wincoordInput;
   int maxInput;
   uint samplersUsed;  /**< bitfield of samplers used */
   int freeSampler;  /** an available sampler for the pstipple */
   int texTemp;  /**< temp registers */
   int numImmed;
   boolean firstInstruction;
   uint coordOrigin;
};


/**
 * TGSI declaration transform callback.
 * Track samplers used, temps used, inputs used.
 */
static void
pstip_transform_decl(struct tgsi_transform_context *ctx,
                     struct tgsi_full_declaration *decl)
{
   struct pstip_transform_context *pctx =
      (struct pstip_transform_context *) ctx;

   /* XXX we can use tgsi_shader_info instead of some of this */

   if (decl->Declaration.File == TGSI_FILE_SAMPLER) {
      uint i;
      for (i = decl->Range.First; i <= decl->Range.Last; i++) {
         pctx->samplersUsed |= 1 << i;
      }
   }
   else if (decl->Declaration.File == TGSI_FILE_INPUT) {
      pctx->maxInput = MAX2(pctx->maxInput, (int) decl->Range.Last);
      if (decl->Semantic.Name == TGSI_SEMANTIC_POSITION)
         pctx->wincoordInput = (int) decl->Range.First;
   }
   else if (decl->Declaration.File == TGSI_FILE_TEMPORARY) {
      uint i;
      for (i = decl->Range.First; i <= decl->Range.Last; i++) {
         pctx->tempsUsed |= (1 << i);
      }
   }

   ctx->emit_declaration(ctx, decl);
}


static void
pstip_transform_immed(struct tgsi_transform_context *ctx,
                      struct tgsi_full_immediate *immed)
{
   struct pstip_transform_context *pctx =
      (struct pstip_transform_context *) ctx;
   pctx->numImmed++;
}


/**
 * Find the lowest zero bit in the given word, or -1 if bitfield is all ones.
 */
static int
free_bit(uint bitfield)
{
   return ffs(~bitfield) - 1;
}


/**
 * TGSI instruction transform callback.
 * Before the first instruction, insert our new code to sample the
 * stipple texture (using the fragment coord register) then kill the
 * fragment if the stipple texture bit is off.
 *
 * Insert:
 *   declare new registers
 *   MUL texTemp, INPUT[wincoord], 1/32;
 *   TEX texTemp, texTemp, sampler;
 *   KIL -texTemp;   # if -texTemp < 0, KILL fragment
 *   [...original code...]
 */
static void
pstip_transform_inst(struct tgsi_transform_context *ctx,
                     struct tgsi_full_instruction *inst)
{
   struct pstip_transform_context *pctx =
      (struct pstip_transform_context *) ctx;

   if (pctx->firstInstruction) {
      /* emit our new declarations before the first instruction */

      struct tgsi_full_declaration decl;
      struct tgsi_full_instruction newInst;
      uint i;
      int wincoordInput;

      /* find free texture sampler */
      pctx->freeSampler = free_bit(pctx->samplersUsed);
      if (pctx->freeSampler >= PIPE_MAX_SAMPLERS)
         pctx->freeSampler = PIPE_MAX_SAMPLERS - 1;

      if (pctx->wincoordInput < 0)
         wincoordInput = pctx->maxInput + 1;
      else
         wincoordInput = pctx->wincoordInput;

      /* find one free temp register */
      for (i = 0; i < 32; i++) {
         if ((pctx->tempsUsed & (1 << i)) == 0) {
            /* found a free temp */
            if (pctx->texTemp < 0)
               pctx->texTemp  = i;
            else
               break;
         }
      }
      assert(pctx->texTemp >= 0);

      if (pctx->wincoordInput < 0) {
         /* declare new position input reg */
         decl = tgsi_default_full_declaration();
         decl.Declaration.File = TGSI_FILE_INPUT;
         decl.Declaration.Interpolate = TGSI_INTERPOLATE_LINEAR;
         decl.Declaration.Semantic = 1;
         decl.Semantic.Name = TGSI_SEMANTIC_POSITION;
         decl.Semantic.Index = 0;
         decl.Range.First = 
            decl.Range.Last = wincoordInput;
         ctx->emit_declaration(ctx, &decl);
      }

      /* declare new sampler */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_SAMPLER;
      decl.Range.First = 
      decl.Range.Last = pctx->freeSampler;
      ctx->emit_declaration(ctx, &decl);

      /* declare new temp regs */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_TEMPORARY;
      decl.Range.First = 
      decl.Range.Last = pctx->texTemp;
      ctx->emit_declaration(ctx, &decl);

      /* emit immediate = {1/32, 1/32, 1, 1}
       * The index/position of this immediate will be pctx->numImmed
       */
      {
         static const float value[4] = { 1.0/32, 1.0/32, 1.0, 1.0 };
         struct tgsi_full_immediate immed;
         uint size = 4;
         immed = tgsi_default_full_immediate();
         immed.Immediate.NrTokens = 1 + size; /* one for the token itself */
         immed.u[0].Float = value[0];
         immed.u[1].Float = value[1];
         immed.u[2].Float = value[2];
         immed.u[3].Float = value[3];
         ctx->emit_immediate(ctx, &immed);
      }

      pctx->firstInstruction = FALSE;


      /* 
       * Insert new MUL/TEX/KILP instructions at start of program
       * Take gl_FragCoord, divide by 32 (stipple size), sample the
       * texture and kill fragment if needed.
       *
       * We'd like to use non-normalized texcoords to index into a RECT
       * texture, but we can only use REPEAT wrap mode with normalized
       * texcoords.  Darn.
       */

      /* XXX invert wincoord if origin isn't lower-left... */

      /* MUL texTemp, INPUT[wincoord], 1/32; */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_MUL;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = pctx->texTemp;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.Src[0].Register.File = TGSI_FILE_INPUT;
      newInst.Src[0].Register.Index = wincoordInput;
      newInst.Src[1].Register.File = TGSI_FILE_IMMEDIATE;
      newInst.Src[1].Register.Index = pctx->numImmed;
      ctx->emit_instruction(ctx, &newInst);

      /* TEX texTemp, texTemp, sampler; */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_TEX;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = pctx->texTemp;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.Instruction.Texture = TRUE;
      newInst.Texture.Texture = TGSI_TEXTURE_2D;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = pctx->texTemp;
      newInst.Src[1].Register.File = TGSI_FILE_SAMPLER;
      newInst.Src[1].Register.Index = pctx->freeSampler;
      ctx->emit_instruction(ctx, &newInst);

      /* KIL -texTemp;   # if -texTemp < 0, KILL fragment */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_KIL;
      newInst.Instruction.NumDstRegs = 0;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = pctx->texTemp;
      newInst.Src[0].Register.Negate = 1;
      ctx->emit_instruction(ctx, &newInst);
   }

   /* emit this instruction */
   ctx->emit_instruction(ctx, inst);
}


/**
 * Given a fragment shader, return a new fragment shader which
 * samples a stipple texture and executes KILL.
 */
struct pipe_shader_state *
util_pstipple_create_fragment_shader(struct pipe_context *pipe,
                                     struct pipe_shader_state *fs,
                                     unsigned *samplerUnitOut)
{
   struct pipe_shader_state *new_fs;
   struct pstip_transform_context transform;
   const uint newLen = tgsi_num_tokens(fs->tokens) + NUM_NEW_TOKENS;
   unsigned i;

   new_fs = MALLOC(sizeof(*new_fs));
   if (!new_fs)
      return NULL;

   new_fs->tokens = tgsi_alloc_tokens(newLen);
   if (!new_fs->tokens) {
      FREE(new_fs);
      return NULL;
   }

   /* Setup shader transformation info/context.
    */
   memset(&transform, 0, sizeof(transform));
   transform.wincoordInput = -1;
   transform.maxInput = -1;
   transform.texTemp = -1;
   transform.firstInstruction = TRUE;
   transform.coordOrigin = TGSI_FS_COORD_ORIGIN_UPPER_LEFT;
   transform.base.transform_instruction = pstip_transform_inst;
   transform.base.transform_declaration = pstip_transform_decl;
   transform.base.transform_immediate = pstip_transform_immed;

   tgsi_scan_shader(fs->tokens, &transform.info);

   /* find fragment coordinate origin property */
   for (i = 0; i < transform.info.num_properties; i++) {
      if (transform.info.properties[i].name == TGSI_PROPERTY_FS_COORD_ORIGIN)
         transform.coordOrigin = transform.info.properties[i].data[0];
   }

   tgsi_transform_shader(fs->tokens,
                         (struct tgsi_token *) new_fs->tokens,
                         newLen, &transform.base);

#if 0 /* DEBUG */
   tgsi_dump(fs->tokens, 0);
   tgsi_dump(new_fs->tokens, 0);
#endif

   assert(transform.freeSampler < PIPE_MAX_SAMPLERS);
   *samplerUnitOut = transform.freeSampler;

   return new_fs;
}

