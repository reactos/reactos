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

/**
 * AA line stage:  AA lines are converted to texture mapped triangles.
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
#include "draw_private.h"
#include "draw_pipe.h"


/** Approx number of new tokens for instructions in aa_transform_inst() */
#define NUM_NEW_TOKENS 50


/**
 * Size for the alpha texture used for antialiasing
 */
#define TEXTURE_SIZE_LOG2  5   /* 32 x 32 */

/**
 * Max texture level for the alpha texture used for antialiasing
 *
 * Don't use the 1x1 and 2x2 mipmap levels.
 */
#define MAX_TEXTURE_LEVEL  (TEXTURE_SIZE_LOG2 - 2)


/**
 * Subclass of pipe_shader_state to carry extra fragment shader info.
 */
struct aaline_fragment_shader
{
   struct pipe_shader_state state;
   void *driver_fs;
   void *aaline_fs;
   uint sampler_unit;
   int generic_attrib;  /**< texcoord/generic used for texture */
};


/**
 * Subclass of draw_stage
 */
struct aaline_stage
{
   struct draw_stage stage;

   float half_line_width;

   /** For AA lines, this is the vertex attrib slot for the new texcoords */
   uint tex_slot;
   /** position, not necessarily output zero */
   uint pos_slot;

   void *sampler_cso;
   struct pipe_resource *texture;
   struct pipe_sampler_view *sampler_view;
   uint num_samplers;
   uint num_sampler_views;


   /*
    * Currently bound state
    */
   struct aaline_fragment_shader *fs;
   struct {
      void *sampler[PIPE_MAX_SAMPLERS];
      struct pipe_sampler_view *sampler_views[PIPE_MAX_SAMPLERS];
   } state;

   /*
    * Driver interface/override functions
    */
   void * (*driver_create_fs_state)(struct pipe_context *,
                                    const struct pipe_shader_state *);
   void (*driver_bind_fs_state)(struct pipe_context *, void *);
   void (*driver_delete_fs_state)(struct pipe_context *, void *);

   void (*driver_bind_sampler_states)(struct pipe_context *, unsigned,
                                      void **);

   void (*driver_set_sampler_views)(struct pipe_context *,
                                    unsigned,
                                    struct pipe_sampler_view **);
};



/**
 * Subclass of tgsi_transform_context, used for transforming the
 * user's fragment shader to add the special AA instructions.
 */
struct aa_transform_context {
   struct tgsi_transform_context base;
   uint tempsUsed;  /**< bitmask */
   int colorOutput; /**< which output is the primary color */
   uint samplersUsed;  /**< bitfield of samplers used */
   int freeSampler;  /** an available sampler for the pstipple */
   int maxInput, maxGeneric;  /**< max input index found */
   int colorTemp, texTemp;  /**< temp registers */
   boolean firstInstruction;
};


/**
 * TGSI declaration transform callback.
 * Look for a free sampler, a free input attrib, and two free temp regs.
 */
static void
aa_transform_decl(struct tgsi_transform_context *ctx,
                  struct tgsi_full_declaration *decl)
{
   struct aa_transform_context *aactx = (struct aa_transform_context *) ctx;

   if (decl->Declaration.File == TGSI_FILE_OUTPUT &&
       decl->Semantic.Name == TGSI_SEMANTIC_COLOR &&
       decl->Semantic.Index == 0) {
      aactx->colorOutput = decl->Range.First;
   }
   else if (decl->Declaration.File == TGSI_FILE_SAMPLER) {
      uint i;
      for (i = decl->Range.First;
           i <= decl->Range.Last; i++) {
         aactx->samplersUsed |= 1 << i;
      }
   }
   else if (decl->Declaration.File == TGSI_FILE_INPUT) {
      if ((int) decl->Range.Last > aactx->maxInput)
         aactx->maxInput = decl->Range.Last;
      if (decl->Semantic.Name == TGSI_SEMANTIC_GENERIC &&
           (int) decl->Semantic.Index > aactx->maxGeneric) {
         aactx->maxGeneric = decl->Semantic.Index;
      }
   }
   else if (decl->Declaration.File == TGSI_FILE_TEMPORARY) {
      uint i;
      for (i = decl->Range.First;
           i <= decl->Range.Last; i++) {
         aactx->tempsUsed |= (1 << i);
      }
   }

   ctx->emit_declaration(ctx, decl);
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
aa_transform_inst(struct tgsi_transform_context *ctx,
                  struct tgsi_full_instruction *inst)
{
   struct aa_transform_context *aactx = (struct aa_transform_context *) ctx;

   if (aactx->firstInstruction) {
      /* emit our new declarations before the first instruction */

      struct tgsi_full_declaration decl;
      uint i;

      /* find free sampler */
      aactx->freeSampler = free_bit(aactx->samplersUsed);
      if (aactx->freeSampler >= PIPE_MAX_SAMPLERS)
         aactx->freeSampler = PIPE_MAX_SAMPLERS - 1;

      /* find two free temp regs */
      for (i = 0; i < 32; i++) {
         if ((aactx->tempsUsed & (1 << i)) == 0) {
            /* found a free temp */
            if (aactx->colorTemp < 0)
               aactx->colorTemp  = i;
            else if (aactx->texTemp < 0)
               aactx->texTemp  = i;
            else
               break;
         }
      }
      assert(aactx->colorTemp >= 0);
      assert(aactx->texTemp >= 0);

      /* declare new generic input/texcoord */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_INPUT;
      /* XXX this could be linear... */
      decl.Declaration.Interpolate = TGSI_INTERPOLATE_PERSPECTIVE;
      decl.Declaration.Semantic = 1;
      decl.Semantic.Name = TGSI_SEMANTIC_GENERIC;
      decl.Semantic.Index = aactx->maxGeneric + 1;
      decl.Range.First = 
      decl.Range.Last = aactx->maxInput + 1;
      ctx->emit_declaration(ctx, &decl);

      /* declare new sampler */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_SAMPLER;
      decl.Range.First = 
      decl.Range.Last = aactx->freeSampler;
      ctx->emit_declaration(ctx, &decl);

      /* declare new temp regs */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_TEMPORARY;
      decl.Range.First = 
      decl.Range.Last = aactx->texTemp;
      ctx->emit_declaration(ctx, &decl);

      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_TEMPORARY;
      decl.Range.First = 
      decl.Range.Last = aactx->colorTemp;
      ctx->emit_declaration(ctx, &decl);

      aactx->firstInstruction = FALSE;
   }

   if (inst->Instruction.Opcode == TGSI_OPCODE_END &&
       aactx->colorOutput != -1) {
      struct tgsi_full_instruction newInst;

      /* TEX */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_TEX;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = aactx->texTemp;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.Instruction.Texture = TRUE;
      newInst.Texture.Texture = TGSI_TEXTURE_2D;
      newInst.Src[0].Register.File = TGSI_FILE_INPUT;
      newInst.Src[0].Register.Index = aactx->maxInput + 1;
      newInst.Src[1].Register.File = TGSI_FILE_SAMPLER;
      newInst.Src[1].Register.Index = aactx->freeSampler;

      ctx->emit_instruction(ctx, &newInst);

      /* MOV rgb */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_MOV;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_OUTPUT;
      newInst.Dst[0].Register.Index = aactx->colorOutput;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_XYZ;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = aactx->colorTemp;
      ctx->emit_instruction(ctx, &newInst);

      /* MUL alpha */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_MUL;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_OUTPUT;
      newInst.Dst[0].Register.Index = aactx->colorOutput;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_W;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = aactx->colorTemp;
      newInst.Src[1].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[1].Register.Index = aactx->texTemp;
      ctx->emit_instruction(ctx, &newInst);

      /* END */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_END;
      newInst.Instruction.NumDstRegs = 0;
      newInst.Instruction.NumSrcRegs = 0;
      ctx->emit_instruction(ctx, &newInst);
   }
   else {
      /* Not an END instruction.
       * Look for writes to result.color and replace with colorTemp reg.
       */
      uint i;

      for (i = 0; i < inst->Instruction.NumDstRegs; i++) {
         struct tgsi_full_dst_register *dst = &inst->Dst[i];
         if (dst->Register.File == TGSI_FILE_OUTPUT &&
             dst->Register.Index == aactx->colorOutput) {
            dst->Register.File = TGSI_FILE_TEMPORARY;
            dst->Register.Index = aactx->colorTemp;
         }
      }

      ctx->emit_instruction(ctx, inst);
   }
}


/**
 * Generate the frag shader we'll use for drawing AA lines.
 * This will be the user's shader plus some texture/modulate instructions.
 */
static boolean
generate_aaline_fs(struct aaline_stage *aaline)
{
   struct pipe_context *pipe = aaline->stage.draw->pipe;
   const struct pipe_shader_state *orig_fs = &aaline->fs->state;
   struct pipe_shader_state aaline_fs;
   struct aa_transform_context transform;
   const uint newLen = tgsi_num_tokens(orig_fs->tokens) + NUM_NEW_TOKENS;

   aaline_fs = *orig_fs; /* copy to init */
   aaline_fs.tokens = tgsi_alloc_tokens(newLen);
   if (aaline_fs.tokens == NULL)
      return FALSE;

   memset(&transform, 0, sizeof(transform));
   transform.colorOutput = -1;
   transform.maxInput = -1;
   transform.maxGeneric = -1;
   transform.colorTemp = -1;
   transform.texTemp = -1;
   transform.firstInstruction = TRUE;
   transform.base.transform_instruction = aa_transform_inst;
   transform.base.transform_declaration = aa_transform_decl;

   tgsi_transform_shader(orig_fs->tokens,
                         (struct tgsi_token *) aaline_fs.tokens,
                         newLen, &transform.base);

#if 0 /* DEBUG */
   debug_printf("draw_aaline, orig shader:\n");
   tgsi_dump(orig_fs->tokens, 0);
   debug_printf("draw_aaline, new shader:\n");
   tgsi_dump(aaline_fs.tokens, 0);
#endif

   aaline->fs->sampler_unit = transform.freeSampler;

   aaline->fs->aaline_fs = aaline->driver_create_fs_state(pipe, &aaline_fs);
   if (aaline->fs->aaline_fs == NULL)
      goto fail;

   aaline->fs->generic_attrib = transform.maxGeneric + 1;
   FREE((void *)aaline_fs.tokens);
   return TRUE;

fail:
   FREE((void *)aaline_fs.tokens);
   return FALSE;
}


/**
 * Create the texture map we'll use for antialiasing the lines.
 */
static boolean
aaline_create_texture(struct aaline_stage *aaline)
{
   struct pipe_context *pipe = aaline->stage.draw->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_resource texTemp;
   struct pipe_sampler_view viewTempl;
   uint level;

   memset(&texTemp, 0, sizeof(texTemp));
   texTemp.target = PIPE_TEXTURE_2D;
   texTemp.format = PIPE_FORMAT_A8_UNORM; /* XXX verify supported by driver! */
   texTemp.last_level = MAX_TEXTURE_LEVEL;
   texTemp.width0 = 1 << TEXTURE_SIZE_LOG2;
   texTemp.height0 = 1 << TEXTURE_SIZE_LOG2;
   texTemp.depth0 = 1;
   texTemp.array_size = 1;
   texTemp.bind = PIPE_BIND_SAMPLER_VIEW;

   aaline->texture = screen->resource_create(screen, &texTemp);
   if (!aaline->texture)
      return FALSE;

   u_sampler_view_default_template(&viewTempl,
                                   aaline->texture,
                                   aaline->texture->format);
   aaline->sampler_view = pipe->create_sampler_view(pipe,
                                                    aaline->texture,
                                                    &viewTempl);
   if (!aaline->sampler_view) {
      return FALSE;
   }

   /* Fill in mipmap images.
    * Basically each level is solid opaque, except for the outermost
    * texels which are zero.  Special case the 1x1 and 2x2 levels
    * (though, those levels shouldn't be used - see the max_lod setting).
    */
   for (level = 0; level <= MAX_TEXTURE_LEVEL; level++) {
      struct pipe_transfer *transfer;
      struct pipe_box box;
      const uint size = u_minify(aaline->texture->width0, level);
      ubyte *data;
      uint i, j;

      assert(aaline->texture->width0 == aaline->texture->height0);

      u_box_origin_2d( size, size, &box );

      /* This texture is new, no need to flush. 
       */
      transfer = pipe->get_transfer(pipe,
                                    aaline->texture,
                                    level,
                                    PIPE_TRANSFER_WRITE,
                                    &box);

      data = pipe->transfer_map(pipe, transfer);
      if (data == NULL)
         return FALSE;

      for (i = 0; i < size; i++) {
         for (j = 0; j < size; j++) {
            ubyte d;
            if (size == 1) {
               d = 255;
            }
            else if (size == 2) {
               d = 200; /* tuneable */
            }
            else if (i == 0 || j == 0 || i == size - 1 || j == size - 1) {
               d = 35;  /* edge texel */
            }
            else {
               d = 255;
            }
            data[i * transfer->stride + j] = d;
         }
      }

      /* unmap */
      pipe->transfer_unmap(pipe, transfer);
      pipe->transfer_destroy(pipe, transfer);
   }
   return TRUE;
}


/**
 * Create the sampler CSO that'll be used for antialiasing.
 * By using a mipmapped texture, we don't have to generate a different
 * texture image for each line size.
 */
static boolean
aaline_create_sampler(struct aaline_stage *aaline)
{
   struct pipe_sampler_state sampler;
   struct pipe_context *pipe = aaline->stage.draw->pipe;

   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_LINEAR;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.normalized_coords = 1;
   sampler.min_lod = 0.0f;
   sampler.max_lod = MAX_TEXTURE_LEVEL;

   aaline->sampler_cso = pipe->create_sampler_state(pipe, &sampler);
   if (aaline->sampler_cso == NULL)
      return FALSE;

   return TRUE;
}


/**
 * When we're about to draw our first AA line in a batch, this function is
 * called to tell the driver to bind our modified fragment shader.
 */
static boolean
bind_aaline_fragment_shader(struct aaline_stage *aaline)
{
   struct draw_context *draw = aaline->stage.draw;
   struct pipe_context *pipe = draw->pipe;

   if (!aaline->fs->aaline_fs && 
       !generate_aaline_fs(aaline))
      return FALSE;

   draw->suspend_flushing = TRUE;
   aaline->driver_bind_fs_state(pipe, aaline->fs->aaline_fs);
   draw->suspend_flushing = FALSE;

   return TRUE;
}



static INLINE struct aaline_stage *
aaline_stage( struct draw_stage *stage )
{
   return (struct aaline_stage *) stage;
}


/**
 * Draw a wide line by drawing a quad, using geometry which will
 * fullfill GL's antialiased line requirements.
 */
static void
aaline_line(struct draw_stage *stage, struct prim_header *header)
{
   const struct aaline_stage *aaline = aaline_stage(stage);
   const float half_width = aaline->half_line_width;
   struct prim_header tri;
   struct vertex_header *v[8];
   uint texPos = aaline->tex_slot;
   uint posPos = aaline->pos_slot;
   float *pos, *tex;
   float dx = header->v[1]->data[posPos][0] - header->v[0]->data[posPos][0];
   float dy = header->v[1]->data[posPos][1] - header->v[0]->data[posPos][1];
   double a = atan2(dy, dx);
   float c_a = (float) cos(a), s_a = (float) sin(a);
   uint i;

   /* XXX the ends of lines aren't quite perfect yet, but probably passable */
   dx = 0.5F * half_width;
   dy = half_width;

   /* allocate/dup new verts */
   for (i = 0; i < 8; i++) {
      v[i] = dup_vert(stage, header->v[i/4], i);
   }

   /*
    * Quad strip for line from v0 to v1 (*=endpoints):
    *
    *  1   3                     5   7
    *  +---+---------------------+---+
    *  |                             |
    *  | *v0                     v1* |
    *  |                             |
    *  +---+---------------------+---+
    *  0   2                     4   6
    */

   /* new verts */
   pos = v[0]->data[posPos];
   pos[0] += (-dx * c_a -  dy * s_a);
   pos[1] += (-dx * s_a +  dy * c_a);

   pos = v[1]->data[posPos];
   pos[0] += (-dx * c_a - -dy * s_a);
   pos[1] += (-dx * s_a + -dy * c_a);

   pos = v[2]->data[posPos];
   pos[0] += ( dx * c_a -  dy * s_a);
   pos[1] += ( dx * s_a +  dy * c_a);

   pos = v[3]->data[posPos];
   pos[0] += ( dx * c_a - -dy * s_a);
   pos[1] += ( dx * s_a + -dy * c_a);

   pos = v[4]->data[posPos];
   pos[0] += (-dx * c_a -  dy * s_a);
   pos[1] += (-dx * s_a +  dy * c_a);

   pos = v[5]->data[posPos];
   pos[0] += (-dx * c_a - -dy * s_a);
   pos[1] += (-dx * s_a + -dy * c_a);

   pos = v[6]->data[posPos];
   pos[0] += ( dx * c_a -  dy * s_a);
   pos[1] += ( dx * s_a +  dy * c_a);

   pos = v[7]->data[posPos];
   pos[0] += ( dx * c_a - -dy * s_a);
   pos[1] += ( dx * s_a + -dy * c_a);

   /* new texcoords */
   tex = v[0]->data[texPos];
   ASSIGN_4V(tex, 0, 0, 0, 1);

   tex = v[1]->data[texPos];
   ASSIGN_4V(tex, 0, 1, 0, 1);

   tex = v[2]->data[texPos];
   ASSIGN_4V(tex, .5, 0, 0, 1);

   tex = v[3]->data[texPos];
   ASSIGN_4V(tex, .5, 1, 0, 1);

   tex = v[4]->data[texPos];
   ASSIGN_4V(tex, .5, 0, 0, 1);

   tex = v[5]->data[texPos];
   ASSIGN_4V(tex, .5, 1, 0, 1);

   tex = v[6]->data[texPos];
   ASSIGN_4V(tex, 1, 0, 0, 1);

   tex = v[7]->data[texPos];
   ASSIGN_4V(tex, 1, 1, 0, 1);

   /* emit 6 tris for the quad strip */
   tri.v[0] = v[2];  tri.v[1] = v[1];  tri.v[2] = v[0];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[3];  tri.v[1] = v[1];  tri.v[2] = v[2];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[4];  tri.v[1] = v[3];  tri.v[2] = v[2];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[5];  tri.v[1] = v[3];  tri.v[2] = v[4];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[6];  tri.v[1] = v[5];  tri.v[2] = v[4];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[7];  tri.v[1] = v[5];  tri.v[2] = v[6];
   stage->next->tri( stage->next, &tri );
}


static void
aaline_first_line(struct draw_stage *stage, struct prim_header *header)
{
   auto struct aaline_stage *aaline = aaline_stage(stage);
   struct draw_context *draw = stage->draw;
   struct pipe_context *pipe = draw->pipe;
   const struct pipe_rasterizer_state *rast = draw->rasterizer;
   uint num_samplers;
   void *r;

   assert(draw->rasterizer->line_smooth);

   if (draw->rasterizer->line_width <= 2.2)
      aaline->half_line_width = 1.1f;
   else
      aaline->half_line_width = 0.5f * draw->rasterizer->line_width;

   /*
    * Bind (generate) our fragprog, sampler and texture
    */
   if (!bind_aaline_fragment_shader(aaline)) {
      stage->line = draw_pipe_passthrough_line;
      stage->line(stage, header);
      return;
   }

   /* update vertex attrib info */
   aaline->pos_slot = draw_current_shader_position_output(draw);;

   /* allocate the extra post-transformed vertex attribute */
   aaline->tex_slot = draw_alloc_extra_vertex_attrib(draw,
                                                     TGSI_SEMANTIC_GENERIC,
                                                     aaline->fs->generic_attrib);

   /* how many samplers? */
   /* we'll use sampler/texture[pstip->sampler_unit] for the stipple */
   num_samplers = MAX2(aaline->num_sampler_views, aaline->num_samplers);
   num_samplers = MAX2(num_samplers, aaline->fs->sampler_unit + 1);

   aaline->state.sampler[aaline->fs->sampler_unit] = aaline->sampler_cso;
   pipe_sampler_view_reference(&aaline->state.sampler_views[aaline->fs->sampler_unit],
                               aaline->sampler_view);

   draw->suspend_flushing = TRUE;
   aaline->driver_bind_sampler_states(pipe, num_samplers, aaline->state.sampler);
   aaline->driver_set_sampler_views(pipe, num_samplers, aaline->state.sampler_views);

   /* Disable triangle culling, stippling, unfilled mode etc. */
   r = draw_get_rasterizer_no_cull(draw, rast->scissor, rast->flatshade);
   pipe->bind_rasterizer_state(pipe, r);

   draw->suspend_flushing = FALSE;

   /* now really draw first line */
   stage->line = aaline_line;
   stage->line(stage, header);
}


static void
aaline_flush(struct draw_stage *stage, unsigned flags)
{
   struct draw_context *draw = stage->draw;
   struct aaline_stage *aaline = aaline_stage(stage);
   struct pipe_context *pipe = draw->pipe;

   stage->line = aaline_first_line;
   stage->next->flush( stage->next, flags );

   /* restore original frag shader, texture, sampler state */
   draw->suspend_flushing = TRUE;
   aaline->driver_bind_fs_state(pipe, aaline->fs->driver_fs);
   aaline->driver_bind_sampler_states(pipe, aaline->num_samplers,
                                      aaline->state.sampler);
   aaline->driver_set_sampler_views(pipe,
                                    aaline->num_sampler_views,
                                    aaline->state.sampler_views);

   /* restore original rasterizer state */
   if (draw->rast_handle) {
      pipe->bind_rasterizer_state(pipe, draw->rast_handle);
   }

   draw->suspend_flushing = FALSE;

   draw_remove_extra_vertex_attribs(draw);
}


static void
aaline_reset_stipple_counter(struct draw_stage *stage)
{
   stage->next->reset_stipple_counter( stage->next );
}


static void
aaline_destroy(struct draw_stage *stage)
{
   struct aaline_stage *aaline = aaline_stage(stage);
   struct pipe_context *pipe = stage->draw->pipe;
   uint i;

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      pipe_sampler_view_reference(&aaline->state.sampler_views[i], NULL);
   }

   if (aaline->sampler_cso)
      pipe->delete_sampler_state(pipe, aaline->sampler_cso);

   if (aaline->texture)
      pipe_resource_reference(&aaline->texture, NULL);

   if (aaline->sampler_view) {
      pipe_sampler_view_reference(&aaline->sampler_view, NULL);
   }

   draw_free_temp_verts( stage );

   /* restore the old entry points */
   pipe->create_fs_state = aaline->driver_create_fs_state;
   pipe->bind_fs_state = aaline->driver_bind_fs_state;
   pipe->delete_fs_state = aaline->driver_delete_fs_state;

   pipe->bind_fragment_sampler_states = aaline->driver_bind_sampler_states;
   pipe->set_fragment_sampler_views = aaline->driver_set_sampler_views;

   FREE( stage );
}


static struct aaline_stage *
draw_aaline_stage(struct draw_context *draw)
{
   struct aaline_stage *aaline = CALLOC_STRUCT(aaline_stage);
   if (aaline == NULL)
      return NULL;

   aaline->stage.draw = draw;
   aaline->stage.name = "aaline";
   aaline->stage.next = NULL;
   aaline->stage.point = draw_pipe_passthrough_point;
   aaline->stage.line = aaline_first_line;
   aaline->stage.tri = draw_pipe_passthrough_tri;
   aaline->stage.flush = aaline_flush;
   aaline->stage.reset_stipple_counter = aaline_reset_stipple_counter;
   aaline->stage.destroy = aaline_destroy;

   if (!draw_alloc_temp_verts( &aaline->stage, 8 ))
      goto fail;

   return aaline;

 fail:
   if (aaline)
      aaline->stage.destroy(&aaline->stage);

   return NULL;
}


static struct aaline_stage *
aaline_stage_from_pipe(struct pipe_context *pipe)
{
   struct draw_context *draw = (struct draw_context *) pipe->draw;
   return aaline_stage(draw->pipeline.aaline);
}


/**
 * This function overrides the driver's create_fs_state() function and
 * will typically be called by the state tracker.
 */
static void *
aaline_create_fs_state(struct pipe_context *pipe,
                       const struct pipe_shader_state *fs)
{
   struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
   struct aaline_fragment_shader *aafs = CALLOC_STRUCT(aaline_fragment_shader);

   if (aafs == NULL)
      return NULL;

   aafs->state.tokens = tgsi_dup_tokens(fs->tokens);

   /* pass-through */
   aafs->driver_fs = aaline->driver_create_fs_state(pipe, fs);

   return aafs;
}


static void
aaline_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
   struct aaline_fragment_shader *aafs = (struct aaline_fragment_shader *) fs;

   /* save current */
   aaline->fs = aafs;
   /* pass-through */
   aaline->driver_bind_fs_state(pipe, (aafs ? aafs->driver_fs : NULL));
}


static void
aaline_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
   struct aaline_fragment_shader *aafs = (struct aaline_fragment_shader *) fs;

   /* pass-through */
   aaline->driver_delete_fs_state(pipe, aafs->driver_fs);

   if (aafs->aaline_fs)
      aaline->driver_delete_fs_state(pipe, aafs->aaline_fs);

   FREE((void*)aafs->state.tokens);

   FREE(aafs);
}


static void
aaline_bind_sampler_states(struct pipe_context *pipe,
                           unsigned num, void **sampler)
{
   struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);

   /* save current */
   memcpy(aaline->state.sampler, sampler, num * sizeof(void *));
   aaline->num_samplers = num;

   /* pass-through */
   aaline->driver_bind_sampler_states(pipe, num, sampler);
}


static void
aaline_set_sampler_views(struct pipe_context *pipe,
                         unsigned num,
                         struct pipe_sampler_view **views)
{
   struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
   uint i;

   /* save current */
   for (i = 0; i < num; i++) {
      pipe_sampler_view_reference(&aaline->state.sampler_views[i], views[i]);
   }
   for ( ; i < PIPE_MAX_SAMPLERS; i++) {
      pipe_sampler_view_reference(&aaline->state.sampler_views[i], NULL);
   }
   aaline->num_sampler_views = num;

   /* pass-through */
   aaline->driver_set_sampler_views(pipe, num, views);
}


/**
 * Called by drivers that want to install this AA line prim stage
 * into the draw module's pipeline.  This will not be used if the
 * hardware has native support for AA lines.
 */
boolean
draw_install_aaline_stage(struct draw_context *draw, struct pipe_context *pipe)
{
   struct aaline_stage *aaline;

   pipe->draw = (void *) draw;

   /*
    * Create / install AA line drawing / prim stage
    */
   aaline = draw_aaline_stage( draw );
   if (!aaline)
      goto fail;

   /* create special texture, sampler state */
   if (!aaline_create_texture(aaline))
      goto fail;

   if (!aaline_create_sampler(aaline))
      goto fail;

   /* save original driver functions */
   aaline->driver_create_fs_state = pipe->create_fs_state;
   aaline->driver_bind_fs_state = pipe->bind_fs_state;
   aaline->driver_delete_fs_state = pipe->delete_fs_state;

   aaline->driver_bind_sampler_states = pipe->bind_fragment_sampler_states;
   aaline->driver_set_sampler_views = pipe->set_fragment_sampler_views;

   /* override the driver's functions */
   pipe->create_fs_state = aaline_create_fs_state;
   pipe->bind_fs_state = aaline_bind_fs_state;
   pipe->delete_fs_state = aaline_delete_fs_state;

   pipe->bind_fragment_sampler_states = aaline_bind_sampler_states;
   pipe->set_fragment_sampler_views = aaline_set_sampler_views;
   
   /* Install once everything is known to be OK:
    */
   draw->pipeline.aaline = &aaline->stage;

   return TRUE;

 fail:
   if (aaline)
      aaline->stage.destroy( &aaline->stage );
   
   return FALSE;
}
