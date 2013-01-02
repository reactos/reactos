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

/**
 * Polygon stipple stage:  implement polygon stipple with texture map and
 * fragment program.  The fragment program samples the texture using the
 * fragment window coordinate register and does a fragment kill for the
 * stipple-failing fragments.
 *
 * Authors:  Brian Paul
 */


#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_inlines.h"

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_sampler.h"

#include "tgsi/tgsi_transform.h"
#include "tgsi/tgsi_dump.h"

#include "draw_context.h"
#include "draw_pipe.h"


/** Approx number of new tokens for instructions in pstip_transform_inst() */
#define NUM_NEW_TOKENS 50


/**
 * Subclass of pipe_shader_state to carry extra fragment shader info.
 */
struct pstip_fragment_shader
{
   struct pipe_shader_state state;
   void *driver_fs;
   void *pstip_fs;
   uint sampler_unit;
};


/**
 * Subclass of draw_stage
 */
struct pstip_stage
{
   struct draw_stage stage;

   void *sampler_cso;
   struct pipe_resource *texture;
   struct pipe_sampler_view *sampler_view;
   uint num_samplers;
   uint num_sampler_views;

   /*
    * Currently bound state
    */
   struct pstip_fragment_shader *fs;
   struct {
      void *samplers[PIPE_MAX_SAMPLERS];
      struct pipe_sampler_view *sampler_views[PIPE_MAX_SAMPLERS];
      const struct pipe_poly_stipple *stipple;
   } state;

   /*
    * Driver interface/override functions
    */
   void * (*driver_create_fs_state)(struct pipe_context *,
                                    const struct pipe_shader_state *);
   void (*driver_bind_fs_state)(struct pipe_context *, void *);
   void (*driver_delete_fs_state)(struct pipe_context *, void *);

   void (*driver_bind_sampler_states)(struct pipe_context *, unsigned, void **);

   void (*driver_set_sampler_views)(struct pipe_context *,
                                    unsigned,
                                    struct pipe_sampler_view **);

   void (*driver_set_polygon_stipple)(struct pipe_context *,
                                      const struct pipe_poly_stipple *);

   struct pipe_context *pipe;
};



/**
 * Subclass of tgsi_transform_context, used for transforming the
 * user's fragment shader to add the extra texture sample and fragment kill
 * instructions.
 */
struct pstip_transform_context {
   struct tgsi_transform_context base;
   uint tempsUsed;  /**< bitmask */
   int wincoordInput;
   int maxInput;
   uint samplersUsed;  /**< bitfield of samplers used */
   int freeSampler;  /** an available sampler for the pstipple */
   int texTemp;  /**< temp registers */
   int numImmed;
   boolean firstInstruction;
};


/**
 * TGSI declaration transform callback.
 * Look for a free sampler, a free input attrib, and two free temp regs.
 */
static void
pstip_transform_decl(struct tgsi_transform_context *ctx,
                     struct tgsi_full_declaration *decl)
{
   struct pstip_transform_context *pctx = (struct pstip_transform_context *) ctx;

   if (decl->Declaration.File == TGSI_FILE_SAMPLER) {
      uint i;
      for (i = decl->Range.First;
           i <= decl->Range.Last; i++) {
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
      for (i = decl->Range.First;
           i <= decl->Range.Last; i++) {
         pctx->tempsUsed |= (1 << i);
      }
   }

   ctx->emit_declaration(ctx, decl);
}


/**
 * TGSI immediate declaration transform callback.
 * We're just counting the number of immediates here.
 */
static void
pstip_transform_immed(struct tgsi_transform_context *ctx,
                      struct tgsi_full_immediate *immed)
{
   struct pstip_transform_context *pctx = (struct pstip_transform_context *) ctx;
   ctx->emit_immediate(ctx, immed); /* emit to output shader */
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
 * Replace writes to result.color w/ a temp reg.
 * Upon END instruction, insert texture sampling code for antialiasing.
 */
static void
pstip_transform_inst(struct tgsi_transform_context *ctx,
                     struct tgsi_full_instruction *inst)
{
   struct pstip_transform_context *pctx = (struct pstip_transform_context *) ctx;

   if (pctx->firstInstruction) {
      /* emit our new declarations before the first instruction */

      struct tgsi_full_declaration decl;
      struct tgsi_full_instruction newInst;
      uint i;
      int wincoordInput;

      /* find free sampler */
      pctx->freeSampler = free_bit(pctx->samplersUsed);
      if (pctx->freeSampler >= PIPE_MAX_SAMPLERS)
         pctx->freeSampler = PIPE_MAX_SAMPLERS - 1;

      if (pctx->wincoordInput < 0)
         wincoordInput = pctx->maxInput + 1;
      else
         wincoordInput = pctx->wincoordInput;

      /* find one free temp reg */
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
         decl.Declaration.Interpolate = TGSI_INTERPOLATE_LINEAR; /* XXX? */
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
       * texture, but we can only use GL_REPEAT wrap mode with normalized
       * texcoords.  Darn.
       */

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
 * Generate the frag shader we'll use for doing polygon stipple.
 * This will be the user's shader prefixed with a TEX and KIL instruction.
 */
static boolean
generate_pstip_fs(struct pstip_stage *pstip)
{
   const struct pipe_shader_state *orig_fs = &pstip->fs->state;
   /*struct draw_context *draw = pstip->stage.draw;*/
   struct pipe_shader_state pstip_fs;
   struct pstip_transform_context transform;
   const uint newLen = tgsi_num_tokens(orig_fs->tokens) + NUM_NEW_TOKENS;

   pstip_fs = *orig_fs; /* copy to init */
   pstip_fs.tokens = tgsi_alloc_tokens(newLen);
   if (pstip_fs.tokens == NULL)
      return FALSE;

   memset(&transform, 0, sizeof(transform));
   transform.wincoordInput = -1;
   transform.maxInput = -1;
   transform.texTemp = -1;
   transform.firstInstruction = TRUE;
   transform.base.transform_instruction = pstip_transform_inst;
   transform.base.transform_declaration = pstip_transform_decl;
   transform.base.transform_immediate = pstip_transform_immed;

   tgsi_transform_shader(orig_fs->tokens,
                         (struct tgsi_token *) pstip_fs.tokens,
                         newLen, &transform.base);

#if 0 /* DEBUG */
   tgsi_dump(orig_fs->tokens, 0);
   tgsi_dump(pstip_fs.tokens, 0);
#endif

   assert(pstip->fs);

   pstip->fs->sampler_unit = transform.freeSampler;
   assert(pstip->fs->sampler_unit < PIPE_MAX_SAMPLERS);

   pstip->fs->pstip_fs = pstip->driver_create_fs_state(pstip->pipe, &pstip_fs);
   
   FREE((void *)pstip_fs.tokens);

   if (!pstip->fs->pstip_fs)
      return FALSE;

   return TRUE;
}


/**
 * Load texture image with current stipple pattern.
 */
static void
pstip_update_texture(struct pstip_stage *pstip)
{
   static const uint bit31 = 1 << 31;
   struct pipe_context *pipe = pstip->pipe;
   struct pipe_transfer *transfer;
   const uint *stipple = pstip->state.stipple->stipple;
   uint i, j;
   ubyte *data;

   transfer = pipe_get_transfer(pipe, pstip->texture, 0, 0,
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
         if (stipple[i] & (bit31 >> j)) {
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
 * Create the texture map we'll use for stippling.
 */
static boolean
pstip_create_texture(struct pstip_stage *pstip)
{
   struct pipe_context *pipe = pstip->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_resource texTemp;
   struct pipe_sampler_view viewTempl;

   memset(&texTemp, 0, sizeof(texTemp));
   texTemp.target = PIPE_TEXTURE_2D;
   texTemp.format = PIPE_FORMAT_A8_UNORM; /* XXX verify supported by driver! */
   texTemp.last_level = 0;
   texTemp.width0 = 32;
   texTemp.height0 = 32;
   texTemp.depth0 = 1;
   texTemp.array_size = 1;
   texTemp.bind = PIPE_BIND_SAMPLER_VIEW;

   pstip->texture = screen->resource_create(screen, &texTemp);
   if (pstip->texture == NULL)
      return FALSE;

   u_sampler_view_default_template(&viewTempl,
                                   pstip->texture,
                                   pstip->texture->format);
   pstip->sampler_view = pipe->create_sampler_view(pipe,
                                                   pstip->texture,
                                                   &viewTempl);
   if (!pstip->sampler_view) {
      return FALSE;
   }

   return TRUE;
}


/**
 * Create the sampler CSO that'll be used for stippling.
 */
static boolean
pstip_create_sampler(struct pstip_stage *pstip)
{
   struct pipe_sampler_state sampler;
   struct pipe_context *pipe = pstip->pipe;

   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_REPEAT;
   sampler.wrap_t = PIPE_TEX_WRAP_REPEAT;
   sampler.wrap_r = PIPE_TEX_WRAP_REPEAT;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.normalized_coords = 1;
   sampler.min_lod = 0.0f;
   sampler.max_lod = 0.0f;

   pstip->sampler_cso = pipe->create_sampler_state(pipe, &sampler);
   if (pstip->sampler_cso == NULL)
      return FALSE;
   
   return TRUE;
}


/**
 * When we're about to draw our first stipple polygon in a batch, this function
 * is called to tell the driver to bind our modified fragment shader.
 */
static boolean
bind_pstip_fragment_shader(struct pstip_stage *pstip)
{
   struct draw_context *draw = pstip->stage.draw;
   if (!pstip->fs->pstip_fs &&
       !generate_pstip_fs(pstip))
      return FALSE;

   draw->suspend_flushing = TRUE;
   pstip->driver_bind_fs_state(pstip->pipe, pstip->fs->pstip_fs);
   draw->suspend_flushing = FALSE;
   return TRUE;
}


static INLINE struct pstip_stage *
pstip_stage( struct draw_stage *stage )
{
   return (struct pstip_stage *) stage;
}


static void
pstip_first_tri(struct draw_stage *stage, struct prim_header *header)
{
   struct pstip_stage *pstip = pstip_stage(stage);
   struct pipe_context *pipe = pstip->pipe;
   struct draw_context *draw = stage->draw;
   uint num_samplers;

   assert(stage->draw->rasterizer->poly_stipple_enable);

   /* bind our fragprog */
   if (!bind_pstip_fragment_shader(pstip)) {
      stage->tri = draw_pipe_passthrough_tri;
      stage->tri(stage, header);
      return;
   }
      

   /* how many samplers? */
   /* we'll use sampler/texture[pstip->sampler_unit] for the stipple */
   num_samplers = MAX2(pstip->num_sampler_views, pstip->num_samplers);
   num_samplers = MAX2(num_samplers, pstip->fs->sampler_unit + 1);

   /* plug in our sampler, texture */
   pstip->state.samplers[pstip->fs->sampler_unit] = pstip->sampler_cso;
   pipe_sampler_view_reference(&pstip->state.sampler_views[pstip->fs->sampler_unit],
                               pstip->sampler_view);

   assert(num_samplers <= PIPE_MAX_SAMPLERS);

   draw->suspend_flushing = TRUE;
   pstip->driver_bind_sampler_states(pipe, num_samplers, pstip->state.samplers);
   pstip->driver_set_sampler_views(pipe, num_samplers, pstip->state.sampler_views);
   draw->suspend_flushing = FALSE;

   /* now really draw first triangle */
   stage->tri = draw_pipe_passthrough_tri;
   stage->tri(stage, header);
}


static void
pstip_flush(struct draw_stage *stage, unsigned flags)
{
   struct draw_context *draw = stage->draw;
   struct pstip_stage *pstip = pstip_stage(stage);
   struct pipe_context *pipe = pstip->pipe;

   stage->tri = pstip_first_tri;
   stage->next->flush( stage->next, flags );

   /* restore original frag shader, texture, sampler state */
   draw->suspend_flushing = TRUE;
   pstip->driver_bind_fs_state(pipe, pstip->fs->driver_fs);
   pstip->driver_bind_sampler_states(pipe, pstip->num_samplers,
                                     pstip->state.samplers);
   pstip->driver_set_sampler_views(pipe,
                                   pstip->num_sampler_views,
                                   pstip->state.sampler_views);
   draw->suspend_flushing = FALSE;
}


static void
pstip_reset_stipple_counter(struct draw_stage *stage)
{
   stage->next->reset_stipple_counter( stage->next );
}


static void
pstip_destroy(struct draw_stage *stage)
{
   struct pstip_stage *pstip = pstip_stage(stage);
   uint i;

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      pipe_sampler_view_reference(&pstip->state.sampler_views[i], NULL);
   }

   pstip->pipe->delete_sampler_state(pstip->pipe, pstip->sampler_cso);

   pipe_resource_reference(&pstip->texture, NULL);

   if (pstip->sampler_view) {
      pipe_sampler_view_reference(&pstip->sampler_view, NULL);
   }

   draw_free_temp_verts( stage );
   FREE( stage );
}


/** Create a new polygon stipple drawing stage object */
static struct pstip_stage *
draw_pstip_stage(struct draw_context *draw, struct pipe_context *pipe)
{
   struct pstip_stage *pstip = CALLOC_STRUCT(pstip_stage);
   if (pstip == NULL)
      goto fail;

   pstip->pipe = pipe;

   pstip->stage.draw = draw;
   pstip->stage.name = "pstip";
   pstip->stage.next = NULL;
   pstip->stage.point = draw_pipe_passthrough_point;
   pstip->stage.line = draw_pipe_passthrough_line;
   pstip->stage.tri = pstip_first_tri;
   pstip->stage.flush = pstip_flush;
   pstip->stage.reset_stipple_counter = pstip_reset_stipple_counter;
   pstip->stage.destroy = pstip_destroy;

   if (!draw_alloc_temp_verts( &pstip->stage, 8 ))
      goto fail;

   return pstip;

fail:
   if (pstip)
      pstip->stage.destroy( &pstip->stage );

   return NULL;
}


static struct pstip_stage *
pstip_stage_from_pipe(struct pipe_context *pipe)
{
   struct draw_context *draw = (struct draw_context *) pipe->draw;
   return pstip_stage(draw->pipeline.pstipple);
}


/**
 * This function overrides the driver's create_fs_state() function and
 * will typically be called by the state tracker.
 */
static void *
pstip_create_fs_state(struct pipe_context *pipe,
                       const struct pipe_shader_state *fs)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   struct pstip_fragment_shader *pstipfs = CALLOC_STRUCT(pstip_fragment_shader);

   if (pstipfs) {
      pstipfs->state = *fs;

      /* pass-through */
      pstipfs->driver_fs = pstip->driver_create_fs_state(pstip->pipe, fs);
   }

   return pstipfs;
}


static void
pstip_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   struct pstip_fragment_shader *pstipfs = (struct pstip_fragment_shader *) fs;
   /* save current */
   pstip->fs = pstipfs;
   /* pass-through */
   pstip->driver_bind_fs_state(pstip->pipe,
                               (pstipfs ? pstipfs->driver_fs : NULL));
}


static void
pstip_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   struct pstip_fragment_shader *pstipfs = (struct pstip_fragment_shader *) fs;
   /* pass-through */
   pstip->driver_delete_fs_state(pstip->pipe, pstipfs->driver_fs);

   if (pstipfs->pstip_fs)
      pstip->driver_delete_fs_state(pstip->pipe, pstipfs->pstip_fs);

   FREE(pstipfs);
}


static void
pstip_bind_sampler_states(struct pipe_context *pipe,
                          unsigned num, void **sampler)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   uint i;

   /* save current */
   memcpy(pstip->state.samplers, sampler, num * sizeof(void *));
   for (i = num; i < PIPE_MAX_SAMPLERS; i++) {
      pstip->state.samplers[i] = NULL;
   }

   pstip->num_samplers = num;
   /* pass-through */
   pstip->driver_bind_sampler_states(pstip->pipe, num, sampler);
}


static void
pstip_set_sampler_views(struct pipe_context *pipe,
                        unsigned num,
                        struct pipe_sampler_view **views)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   uint i;

   /* save current */
   for (i = 0; i < num; i++) {
      pipe_sampler_view_reference(&pstip->state.sampler_views[i], views[i]);
   }
   for (; i < PIPE_MAX_SAMPLERS; i++) {
      pipe_sampler_view_reference(&pstip->state.sampler_views[i], NULL);
   }

   pstip->num_sampler_views = num;

   /* pass-through */
   pstip->driver_set_sampler_views(pstip->pipe, num, views);
}


static void
pstip_set_polygon_stipple(struct pipe_context *pipe,
                          const struct pipe_poly_stipple *stipple)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);

   /* save current */
   pstip->state.stipple = stipple;

   /* pass-through */
   pstip->driver_set_polygon_stipple(pstip->pipe, stipple);

   pstip_update_texture(pstip);
}


/**
 * Called by drivers that want to install this polygon stipple stage
 * into the draw module's pipeline.  This will not be used if the
 * hardware has native support for polygon stipple.
 */
boolean
draw_install_pstipple_stage(struct draw_context *draw,
                            struct pipe_context *pipe)
{
   struct pstip_stage *pstip;

   pipe->draw = (void *) draw;

   /*
    * Create / install pgon stipple drawing / prim stage
    */
   pstip = draw_pstip_stage( draw, pipe );
   if (pstip == NULL)
      goto fail;

   draw->pipeline.pstipple = &pstip->stage;

   /* create special texture, sampler state */
   if (!pstip_create_texture(pstip))
      goto fail;

   if (!pstip_create_sampler(pstip))
      goto fail;

   /* save original driver functions */
   pstip->driver_create_fs_state = pipe->create_fs_state;
   pstip->driver_bind_fs_state = pipe->bind_fs_state;
   pstip->driver_delete_fs_state = pipe->delete_fs_state;

   pstip->driver_bind_sampler_states = pipe->bind_fragment_sampler_states;
   pstip->driver_set_sampler_views = pipe->set_fragment_sampler_views;
   pstip->driver_set_polygon_stipple = pipe->set_polygon_stipple;

   /* override the driver's functions */
   pipe->create_fs_state = pstip_create_fs_state;
   pipe->bind_fs_state = pstip_bind_fs_state;
   pipe->delete_fs_state = pstip_delete_fs_state;

   pipe->bind_fragment_sampler_states = pstip_bind_sampler_states;
   pipe->set_fragment_sampler_views = pstip_set_sampler_views;
   pipe->set_polygon_stipple = pstip_set_polygon_stipple;

   return TRUE;

 fail:
   if (pstip)
      pstip->stage.destroy( &pstip->stage );

   return FALSE;
}
