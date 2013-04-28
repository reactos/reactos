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
 * AA point stage:  AA points are converted to quads and rendered with a
 * special fragment shader.  Another approach would be to use a texture
 * map image of a point, but experiments indicate the quality isn't nearly
 * as good as this approach.
 *
 * Note: this looks a lot like draw_aaline.c but there's actually little
 * if any code that can be shared.
 *
 * Authors:  Brian Paul
 */


#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"

#include "tgsi/tgsi_transform.h"
#include "tgsi/tgsi_dump.h"

#include "util/u_math.h"
#include "util/u_memory.h"

#include "draw_context.h"
#include "draw_vs.h"
#include "draw_pipe.h"


/** Approx number of new tokens for instructions in aa_transform_inst() */
#define NUM_NEW_TOKENS 200


/*
 * Enabling NORMALIZE might give _slightly_ better results.
 * Basically, it controls whether we compute distance as d=sqrt(x*x+y*y) or
 * d=x*x+y*y.  Since we're working with a unit circle, the later seems
 * close enough and saves some costly instructions.
 */
#define NORMALIZE 0


/**
 * Subclass of pipe_shader_state to carry extra fragment shader info.
 */
struct aapoint_fragment_shader
{
   struct pipe_shader_state state;
   void *driver_fs;   /**< the regular shader */
   void *aapoint_fs;  /**< the aa point-augmented shader */
   int generic_attrib; /**< The generic input attrib/texcoord we'll use */
};


/**
 * Subclass of draw_stage
 */
struct aapoint_stage
{
   struct draw_stage stage;

   /** half of pipe_rasterizer_state::point_size */
   float radius;

   /** vertex attrib slot containing point size */
   int psize_slot;

   /** this is the vertex attrib slot for the new texcoords */
   uint tex_slot;

   /** vertex attrib slot containing position */
   uint pos_slot;

   /** Currently bound fragment shader */
   struct aapoint_fragment_shader *fs;

   /*
    * Driver interface/override functions
    */
   void * (*driver_create_fs_state)(struct pipe_context *,
                                    const struct pipe_shader_state *);
   void (*driver_bind_fs_state)(struct pipe_context *, void *);
   void (*driver_delete_fs_state)(struct pipe_context *, void *);
};



/**
 * Subclass of tgsi_transform_context, used for transforming the
 * user's fragment shader to add the special AA instructions.
 */
struct aa_transform_context {
   struct tgsi_transform_context base;
   uint tempsUsed;  /**< bitmask */
   int colorOutput; /**< which output is the primary color */
   int maxInput, maxGeneric;  /**< max input index found */
   int tmp0, colorTemp;  /**< temp registers */
   boolean firstInstruction;
};


/**
 * TGSI declaration transform callback.
 * Look for two free temp regs and available input reg for new texcoords.
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
 * TGSI instruction transform callback.
 * Replace writes to result.color w/ a temp reg.
 * Upon END instruction, insert texture sampling code for antialiasing.
 */
static void
aa_transform_inst(struct tgsi_transform_context *ctx,
                  struct tgsi_full_instruction *inst)
{
   struct aa_transform_context *aactx = (struct aa_transform_context *) ctx;
   struct tgsi_full_instruction newInst;

   if (aactx->firstInstruction) {
      /* emit our new declarations before the first instruction */

      struct tgsi_full_declaration decl;
      const int texInput = aactx->maxInput + 1;
      int tmp0;
      uint i;

      /* find two free temp regs */
      for (i = 0; i < 32; i++) {
         if ((aactx->tempsUsed & (1 << i)) == 0) {
            /* found a free temp */
            if (aactx->tmp0 < 0)
               aactx->tmp0 = i;
            else if (aactx->colorTemp < 0)
               aactx->colorTemp = i;
            else
               break;
         }
      }

      assert(aactx->colorTemp != aactx->tmp0);

      tmp0 = aactx->tmp0;

      /* declare new generic input/texcoord */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_INPUT;
      /* XXX this could be linear... */
      decl.Declaration.Interpolate = TGSI_INTERPOLATE_PERSPECTIVE;
      decl.Declaration.Semantic = 1;
      decl.Semantic.Name = TGSI_SEMANTIC_GENERIC;
      decl.Semantic.Index = aactx->maxGeneric + 1;
      decl.Range.First = 
      decl.Range.Last = texInput;
      ctx->emit_declaration(ctx, &decl);

      /* declare new temp regs */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_TEMPORARY;
      decl.Range.First = 
      decl.Range.Last = tmp0;
      ctx->emit_declaration(ctx, &decl);

      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_TEMPORARY;
      decl.Range.First = 
      decl.Range.Last = aactx->colorTemp;
      ctx->emit_declaration(ctx, &decl);

      aactx->firstInstruction = FALSE;


      /*
       * Emit code to compute fragment coverage, kill if outside point radius
       *
       * Temp reg0 usage:
       *  t0.x = distance of fragment from center point
       *  t0.y = boolean, is t0.x > 1.0, also misc temp usage
       *  t0.z = temporary for computing 1/(1-k) value
       *  t0.w = final coverage value
       */

      /* MUL t0.xy, tex, tex;  # compute x^2, y^2 */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_MUL;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = tmp0;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_XY;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.Src[0].Register.File = TGSI_FILE_INPUT;
      newInst.Src[0].Register.Index = texInput;
      newInst.Src[1].Register.File = TGSI_FILE_INPUT;
      newInst.Src[1].Register.Index = texInput;
      ctx->emit_instruction(ctx, &newInst);

      /* ADD t0.x, t0.x, t0.y;  # x^2 + y^2 */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_ADD;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = tmp0;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_X;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = tmp0;
      newInst.Src[0].Register.SwizzleX = TGSI_SWIZZLE_X;
      newInst.Src[1].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[1].Register.Index = tmp0;
      newInst.Src[1].Register.SwizzleX = TGSI_SWIZZLE_Y;
      ctx->emit_instruction(ctx, &newInst);

#if NORMALIZE  /* OPTIONAL normalization of length */
      /* RSQ t0.x, t0.x; */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_RSQ;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = tmp0;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_X;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = tmp0;
      ctx->emit_instruction(ctx, &newInst);

      /* RCP t0.x, t0.x; */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_RCP;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = tmp0;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_X;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = tmp0;
      ctx->emit_instruction(ctx, &newInst);
#endif

      /* SGT t0.y, t0.xxxx, tex.wwww;  # bool b = d > 1 (NOTE tex.w == 1) */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_SGT;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = tmp0;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_Y;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = tmp0;
      newInst.Src[0].Register.SwizzleY = TGSI_SWIZZLE_X;
      newInst.Src[1].Register.File = TGSI_FILE_INPUT;
      newInst.Src[1].Register.Index = texInput;
      newInst.Src[1].Register.SwizzleY = TGSI_SWIZZLE_W;
      ctx->emit_instruction(ctx, &newInst);

      /* KIL -tmp0.yyyy;   # if -tmp0.y < 0, KILL */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_KIL;
      newInst.Instruction.NumDstRegs = 0;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = tmp0;
      newInst.Src[0].Register.SwizzleX = TGSI_SWIZZLE_Y;
      newInst.Src[0].Register.SwizzleY = TGSI_SWIZZLE_Y;
      newInst.Src[0].Register.SwizzleZ = TGSI_SWIZZLE_Y;
      newInst.Src[0].Register.SwizzleW = TGSI_SWIZZLE_Y;
      newInst.Src[0].Register.Negate = 1;
      ctx->emit_instruction(ctx, &newInst);


      /* compute coverage factor = (1-d)/(1-k) */

      /* SUB t0.z, tex.w, tex.z;  # m = 1 - k */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_SUB;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = tmp0;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_Z;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.Src[0].Register.File = TGSI_FILE_INPUT;
      newInst.Src[0].Register.Index = texInput;
      newInst.Src[0].Register.SwizzleZ = TGSI_SWIZZLE_W;
      newInst.Src[1].Register.File = TGSI_FILE_INPUT;
      newInst.Src[1].Register.Index = texInput;
      newInst.Src[1].Register.SwizzleZ = TGSI_SWIZZLE_Z;
      ctx->emit_instruction(ctx, &newInst);

      /* RCP t0.z, t0.z;  # t0.z = 1 / m */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_RCP;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = tmp0;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_Z;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = tmp0;
      newInst.Src[0].Register.SwizzleX = TGSI_SWIZZLE_Z;
      ctx->emit_instruction(ctx, &newInst);

      /* SUB t0.y, 1, t0.x;  # d = 1 - d */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_SUB;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = tmp0;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_Y;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.Src[0].Register.File = TGSI_FILE_INPUT;
      newInst.Src[0].Register.Index = texInput;
      newInst.Src[0].Register.SwizzleY = TGSI_SWIZZLE_W;
      newInst.Src[1].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[1].Register.Index = tmp0;
      newInst.Src[1].Register.SwizzleY = TGSI_SWIZZLE_X;
      ctx->emit_instruction(ctx, &newInst);

      /* MUL t0.w, t0.y, t0.z;   # coverage = d * m */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_MUL;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = tmp0;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_W;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = tmp0;
      newInst.Src[0].Register.SwizzleW = TGSI_SWIZZLE_Y;
      newInst.Src[1].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[1].Register.Index = tmp0;
      newInst.Src[1].Register.SwizzleW = TGSI_SWIZZLE_Z;
      ctx->emit_instruction(ctx, &newInst);

      /* SLE t0.y, t0.x, tex.z;  # bool b = distance <= k */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_SLE;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = tmp0;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_Y;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = tmp0;
      newInst.Src[0].Register.SwizzleY = TGSI_SWIZZLE_X;
      newInst.Src[1].Register.File = TGSI_FILE_INPUT;
      newInst.Src[1].Register.Index = texInput;
      newInst.Src[1].Register.SwizzleY = TGSI_SWIZZLE_Z;
      ctx->emit_instruction(ctx, &newInst);

      /* CMP t0.w, -t0.y, tex.w, t0.w;
       *  # if -t0.y < 0 then
       *       t0.w = 1
       *    else
       *       t0.w = t0.w
       */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_CMP;
      newInst.Instruction.NumDstRegs = 1;
      newInst.Dst[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Dst[0].Register.Index = tmp0;
      newInst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_W;
      newInst.Instruction.NumSrcRegs = 3;
      newInst.Src[0].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[0].Register.Index = tmp0;
      newInst.Src[0].Register.SwizzleX = TGSI_SWIZZLE_Y;
      newInst.Src[0].Register.SwizzleY = TGSI_SWIZZLE_Y;
      newInst.Src[0].Register.SwizzleZ = TGSI_SWIZZLE_Y;
      newInst.Src[0].Register.SwizzleW = TGSI_SWIZZLE_Y;
      newInst.Src[0].Register.Negate = 1;
      newInst.Src[1].Register.File = TGSI_FILE_INPUT;
      newInst.Src[1].Register.Index = texInput;
      newInst.Src[1].Register.SwizzleX = TGSI_SWIZZLE_W;
      newInst.Src[1].Register.SwizzleY = TGSI_SWIZZLE_W;
      newInst.Src[1].Register.SwizzleZ = TGSI_SWIZZLE_W;
      newInst.Src[1].Register.SwizzleW = TGSI_SWIZZLE_W;
      newInst.Src[2].Register.File = TGSI_FILE_TEMPORARY;
      newInst.Src[2].Register.Index = tmp0;
      newInst.Src[2].Register.SwizzleX = TGSI_SWIZZLE_W;
      newInst.Src[2].Register.SwizzleY = TGSI_SWIZZLE_W;
      newInst.Src[2].Register.SwizzleZ = TGSI_SWIZZLE_W;
      newInst.Src[2].Register.SwizzleW = TGSI_SWIZZLE_W;
      ctx->emit_instruction(ctx, &newInst);

   }

   if (inst->Instruction.Opcode == TGSI_OPCODE_END) {
      /* add alpha modulation code at tail of program */

      /* MOV result.color.xyz, colorTemp; */
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

      /* MUL result.color.w, colorTemp, tmp0.w; */
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
      newInst.Src[1].Register.Index = aactx->tmp0;
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
   }

   ctx->emit_instruction(ctx, inst);
}


/**
 * Generate the frag shader we'll use for drawing AA points.
 * This will be the user's shader plus some texture/modulate instructions.
 */
static boolean
generate_aapoint_fs(struct aapoint_stage *aapoint)
{
   const struct pipe_shader_state *orig_fs = &aapoint->fs->state;
   struct pipe_shader_state aapoint_fs;
   struct aa_transform_context transform;
   const uint newLen = tgsi_num_tokens(orig_fs->tokens) + NUM_NEW_TOKENS;
   struct pipe_context *pipe = aapoint->stage.draw->pipe;

   aapoint_fs = *orig_fs; /* copy to init */
   aapoint_fs.tokens = tgsi_alloc_tokens(newLen);
   if (aapoint_fs.tokens == NULL)
      return FALSE;

   memset(&transform, 0, sizeof(transform));
   transform.colorOutput = -1;
   transform.maxInput = -1;
   transform.maxGeneric = -1;
   transform.colorTemp = -1;
   transform.tmp0 = -1;
   transform.firstInstruction = TRUE;
   transform.base.transform_instruction = aa_transform_inst;
   transform.base.transform_declaration = aa_transform_decl;

   tgsi_transform_shader(orig_fs->tokens,
                         (struct tgsi_token *) aapoint_fs.tokens,
                         newLen, &transform.base);

#if 0 /* DEBUG */
   debug_printf("draw_aapoint, orig shader:\n");
   tgsi_dump(orig_fs->tokens, 0);
   debug_printf("draw_aapoint, new shader:\n");
   tgsi_dump(aapoint_fs.tokens, 0);
#endif

   aapoint->fs->aapoint_fs
      = aapoint->driver_create_fs_state(pipe, &aapoint_fs);
   if (aapoint->fs->aapoint_fs == NULL)
      goto fail;

   aapoint->fs->generic_attrib = transform.maxGeneric + 1;
   FREE((void *)aapoint_fs.tokens);
   return TRUE;

fail:
   FREE((void *)aapoint_fs.tokens);
   return FALSE;
}


/**
 * When we're about to draw our first AA point in a batch, this function is
 * called to tell the driver to bind our modified fragment shader.
 */
static boolean
bind_aapoint_fragment_shader(struct aapoint_stage *aapoint)
{
   struct draw_context *draw = aapoint->stage.draw;
   struct pipe_context *pipe = draw->pipe;

   if (!aapoint->fs->aapoint_fs &&
       !generate_aapoint_fs(aapoint))
      return FALSE;

   draw->suspend_flushing = TRUE;
   aapoint->driver_bind_fs_state(pipe, aapoint->fs->aapoint_fs);
   draw->suspend_flushing = FALSE;

   return TRUE;
}



static INLINE struct aapoint_stage *
aapoint_stage( struct draw_stage *stage )
{
   return (struct aapoint_stage *) stage;
}




/**
 * Draw an AA point by drawing a quad.
 */
static void
aapoint_point(struct draw_stage *stage, struct prim_header *header)
{
   const struct aapoint_stage *aapoint = aapoint_stage(stage);
   struct prim_header tri;
   struct vertex_header *v[4];
   const uint tex_slot = aapoint->tex_slot;
   const uint pos_slot = aapoint->pos_slot;
   float radius, *pos, *tex;
   uint i;
   float k;

   if (aapoint->psize_slot >= 0) {
      radius = 0.5f * header->v[0]->data[aapoint->psize_slot][0];
   }
   else {
      radius = aapoint->radius;
   }

   /*
    * Note: the texcoords (generic attrib, really) we use are special:
    * The S and T components simply vary from -1 to +1.
    * The R component is k, below.
    * The Q component is 1.0 and will used as a handy constant in the
    * fragment shader.
    */

   /*
    * k is the threshold distance from the point's center at which
    * we begin alpha attenuation (the coverage value).
    * Operating within a unit circle, we'll compute the fragment's
    * distance 'd' from the center point using the texcoords.
    * IF d > 1.0 THEN
    *    KILL fragment
    * ELSE IF d > k THEN
    *    compute coverage in [0,1] proportional to d in [k, 1].
    * ELSE
    *    coverage = 1.0;  // full coverage
    * ENDIF
    *
    * Note: the ELSEIF and ELSE clauses are actually implemented with CMP to
    * avoid using IF/ELSE/ENDIF TGSI opcodes.
    */

#if !NORMALIZE
   k = 1.0f / radius;
   k = 1.0f - 2.0f * k + k * k;
#else
   k = 1.0f - 1.0f / radius;
#endif

   /* allocate/dup new verts */
   for (i = 0; i < 4; i++) {
      v[i] = dup_vert(stage, header->v[0], i);
   }

   /* new verts */
   pos = v[0]->data[pos_slot];
   pos[0] -= radius;
   pos[1] -= radius;

   pos = v[1]->data[pos_slot];
   pos[0] += radius;
   pos[1] -= radius;

   pos = v[2]->data[pos_slot];
   pos[0] += radius;
   pos[1] += radius;

   pos = v[3]->data[pos_slot];
   pos[0] -= radius;
   pos[1] += radius;

   /* new texcoords */
   tex = v[0]->data[tex_slot];
   ASSIGN_4V(tex, -1, -1, k, 1);

   tex = v[1]->data[tex_slot];
   ASSIGN_4V(tex,  1, -1, k, 1);

   tex = v[2]->data[tex_slot];
   ASSIGN_4V(tex,  1,  1, k, 1);

   tex = v[3]->data[tex_slot];
   ASSIGN_4V(tex, -1,  1, k, 1);

   /* emit 2 tris for the quad strip */
   tri.v[0] = v[0];
   tri.v[1] = v[1];
   tri.v[2] = v[2];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[0];
   tri.v[1] = v[2];
   tri.v[2] = v[3];
   stage->next->tri( stage->next, &tri );
}


static void
aapoint_first_point(struct draw_stage *stage, struct prim_header *header)
{
   auto struct aapoint_stage *aapoint = aapoint_stage(stage);
   struct draw_context *draw = stage->draw;
   struct pipe_context *pipe = draw->pipe;
   const struct pipe_rasterizer_state *rast = draw->rasterizer;
   void *r;

   assert(draw->rasterizer->point_smooth);

   if (draw->rasterizer->point_size <= 2.0)
      aapoint->radius = 1.0;
   else
      aapoint->radius = 0.5f * draw->rasterizer->point_size;

   /*
    * Bind (generate) our fragprog.
    */
   bind_aapoint_fragment_shader(aapoint);

   /* update vertex attrib info */
   aapoint->pos_slot = draw_current_shader_position_output(draw);

   /* allocate the extra post-transformed vertex attribute */
   aapoint->tex_slot = draw_alloc_extra_vertex_attrib(draw,
                                                      TGSI_SEMANTIC_GENERIC,
                                                      aapoint->fs->generic_attrib);
   assert(aapoint->tex_slot > 0); /* output[0] is vertex pos */

   /* find psize slot in post-transform vertex */
   aapoint->psize_slot = -1;
   if (draw->rasterizer->point_size_per_vertex) {
      const struct tgsi_shader_info *info = draw_get_shader_info(draw);
      uint i;
      /* find PSIZ vertex output */
      for (i = 0; i < info->num_outputs; i++) {
         if (info->output_semantic_name[i] == TGSI_SEMANTIC_PSIZE) {
            aapoint->psize_slot = i;
            break;
         }
      }
   }

   draw->suspend_flushing = TRUE;

   /* Disable triangle culling, stippling, unfilled mode etc. */
   r = draw_get_rasterizer_no_cull(draw, rast->scissor, rast->flatshade);
   pipe->bind_rasterizer_state(pipe, r);

   draw->suspend_flushing = FALSE;

   /* now really draw first point */
   stage->point = aapoint_point;
   stage->point(stage, header);
}


static void
aapoint_flush(struct draw_stage *stage, unsigned flags)
{
   struct draw_context *draw = stage->draw;
   struct aapoint_stage *aapoint = aapoint_stage(stage);
   struct pipe_context *pipe = draw->pipe;

   stage->point = aapoint_first_point;
   stage->next->flush( stage->next, flags );

   /* restore original frag shader */
   draw->suspend_flushing = TRUE;
   aapoint->driver_bind_fs_state(pipe, aapoint->fs->driver_fs);

   /* restore original rasterizer state */
   if (draw->rast_handle) {
      pipe->bind_rasterizer_state(pipe, draw->rast_handle);
   }

   draw->suspend_flushing = FALSE;

   draw_remove_extra_vertex_attribs(draw);
}


static void
aapoint_reset_stipple_counter(struct draw_stage *stage)
{
   stage->next->reset_stipple_counter( stage->next );
}


static void
aapoint_destroy(struct draw_stage *stage)
{
   struct aapoint_stage* aapoint = aapoint_stage(stage);
   struct pipe_context *pipe = stage->draw->pipe;

   draw_free_temp_verts( stage );

   /* restore the old entry points */
   pipe->create_fs_state = aapoint->driver_create_fs_state;
   pipe->bind_fs_state = aapoint->driver_bind_fs_state;
   pipe->delete_fs_state = aapoint->driver_delete_fs_state;

   FREE( stage );
}


static struct aapoint_stage *
draw_aapoint_stage(struct draw_context *draw)
{
   struct aapoint_stage *aapoint = CALLOC_STRUCT(aapoint_stage);
   if (aapoint == NULL)
      goto fail;

   aapoint->stage.draw = draw;
   aapoint->stage.name = "aapoint";
   aapoint->stage.next = NULL;
   aapoint->stage.point = aapoint_first_point;
   aapoint->stage.line = draw_pipe_passthrough_line;
   aapoint->stage.tri = draw_pipe_passthrough_tri;
   aapoint->stage.flush = aapoint_flush;
   aapoint->stage.reset_stipple_counter = aapoint_reset_stipple_counter;
   aapoint->stage.destroy = aapoint_destroy;

   if (!draw_alloc_temp_verts( &aapoint->stage, 4 ))
      goto fail;

   return aapoint;

 fail:
   if (aapoint)
      aapoint->stage.destroy(&aapoint->stage);

   return NULL;

}


static struct aapoint_stage *
aapoint_stage_from_pipe(struct pipe_context *pipe)
{
   struct draw_context *draw = (struct draw_context *) pipe->draw;
   return aapoint_stage(draw->pipeline.aapoint);
}


/**
 * This function overrides the driver's create_fs_state() function and
 * will typically be called by the state tracker.
 */
static void *
aapoint_create_fs_state(struct pipe_context *pipe,
                       const struct pipe_shader_state *fs)
{
   struct aapoint_stage *aapoint = aapoint_stage_from_pipe(pipe);
   struct aapoint_fragment_shader *aafs = CALLOC_STRUCT(aapoint_fragment_shader);
   if (aafs == NULL) 
      return NULL;

   aafs->state.tokens = tgsi_dup_tokens(fs->tokens);

   /* pass-through */
   aafs->driver_fs = aapoint->driver_create_fs_state(pipe, fs);

   return aafs;
}


static void
aapoint_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct aapoint_stage *aapoint = aapoint_stage_from_pipe(pipe);
   struct aapoint_fragment_shader *aafs = (struct aapoint_fragment_shader *) fs;
   /* save current */
   aapoint->fs = aafs;
   /* pass-through */
   aapoint->driver_bind_fs_state(pipe,
                                 (aafs ? aafs->driver_fs : NULL));
}


static void
aapoint_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct aapoint_stage *aapoint = aapoint_stage_from_pipe(pipe);
   struct aapoint_fragment_shader *aafs = (struct aapoint_fragment_shader *) fs;

   /* pass-through */
   aapoint->driver_delete_fs_state(pipe, aafs->driver_fs);

   if (aafs->aapoint_fs)
      aapoint->driver_delete_fs_state(pipe, aafs->aapoint_fs);

   FREE((void*)aafs->state.tokens);

   FREE(aafs);
}


/**
 * Called by drivers that want to install this AA point prim stage
 * into the draw module's pipeline.  This will not be used if the
 * hardware has native support for AA points.
 */
boolean
draw_install_aapoint_stage(struct draw_context *draw,
                           struct pipe_context *pipe)
{
   struct aapoint_stage *aapoint;

   pipe->draw = (void *) draw;

   /*
    * Create / install AA point drawing / prim stage
    */
   aapoint = draw_aapoint_stage( draw );
   if (aapoint == NULL)
      return FALSE;

   /* save original driver functions */
   aapoint->driver_create_fs_state = pipe->create_fs_state;
   aapoint->driver_bind_fs_state = pipe->bind_fs_state;
   aapoint->driver_delete_fs_state = pipe->delete_fs_state;

   /* override the driver's functions */
   pipe->create_fs_state = aapoint_create_fs_state;
   pipe->bind_fs_state = aapoint_bind_fs_state;
   pipe->delete_fs_state = aapoint_delete_fs_state;

   draw->pipeline.aapoint = &aapoint->stage;

   return TRUE;
}
