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

#include "util/u_debug.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi_parse.h"
#include "util/u_memory.h"

unsigned
tgsi_parse_init(
   struct tgsi_parse_context *ctx,
   const struct tgsi_token *tokens )
{
   ctx->FullHeader.Header = *(struct tgsi_header *) &tokens[0];
   if( ctx->FullHeader.Header.HeaderSize >= 2 ) {
      ctx->FullHeader.Processor = *(struct tgsi_processor *) &tokens[1];
   }
   else {
      return TGSI_PARSE_ERROR;
   }

   ctx->Tokens = tokens;
   ctx->Position = ctx->FullHeader.Header.HeaderSize;

   return TGSI_PARSE_OK;
}

void
tgsi_parse_free(
   struct tgsi_parse_context *ctx )
{
}

boolean
tgsi_parse_end_of_tokens(
   struct tgsi_parse_context *ctx )
{
   return ctx->Position >=
      ctx->FullHeader.Header.HeaderSize + ctx->FullHeader.Header.BodySize;
}


/**
 * This function is used to avoid and work-around type punning/aliasing
 * warnings.  The warnings seem harmless on x86 but on PPC they cause
 * real failures.
 */
static INLINE void
copy_token(void *dst, const void *src)
{
   memcpy(dst, src, 4);
}


/**
 * Get next 4-byte token, return it at address specified by 'token'
 */
static void
next_token(
   struct tgsi_parse_context *ctx,
   void *token )
{
   assert( !tgsi_parse_end_of_tokens( ctx ) );
   copy_token(token, &ctx->Tokens[ctx->Position]);
   ctx->Position++;
}


void
tgsi_parse_token(
   struct tgsi_parse_context *ctx )
{
   struct tgsi_token token;
   unsigned i;

   next_token( ctx, &token );

   switch( token.Type ) {
   case TGSI_TOKEN_TYPE_DECLARATION:
   {
      struct tgsi_full_declaration *decl = &ctx->FullToken.FullDeclaration;

      memset(decl, 0, sizeof *decl);
      copy_token(&decl->Declaration, &token);

      next_token( ctx, &decl->Range );

      if (decl->Declaration.Dimension) {
         next_token(ctx, &decl->Dim);
      }

      if( decl->Declaration.Semantic ) {
         next_token( ctx, &decl->Semantic );
      }

      if (decl->Declaration.File == TGSI_FILE_IMMEDIATE_ARRAY) {
         unsigned i, j;
         decl->ImmediateData.u = (union tgsi_immediate_data*)
                                 &ctx->Tokens[ctx->Position];
         for (i = 0; i <= decl->Range.Last; ++i) {
            for (j = 0; j < 4; ++j) {
               ctx->Position++;
            }
         }
      }

      if (decl->Declaration.File == TGSI_FILE_RESOURCE) {
         next_token(ctx, &decl->Resource);
      }

      break;
   }

   case TGSI_TOKEN_TYPE_IMMEDIATE:
   {
      struct tgsi_full_immediate *imm = &ctx->FullToken.FullImmediate;
      uint imm_count;

      memset(imm, 0, sizeof *imm);
      copy_token(&imm->Immediate, &token);

      imm_count = imm->Immediate.NrTokens - 1;

      switch (imm->Immediate.DataType) {
      case TGSI_IMM_FLOAT32:
         for (i = 0; i < imm_count; i++) {
            next_token(ctx, &imm->u[i].Float);
         }
         break;

      case TGSI_IMM_UINT32:
         for (i = 0; i < imm_count; i++) {
            next_token(ctx, &imm->u[i].Uint);
         }
         break;

      case TGSI_IMM_INT32:
         for (i = 0; i < imm_count; i++) {
            next_token(ctx, &imm->u[i].Int);
         }
         break;

      default:
         assert( 0 );
      }

      break;
   }

   case TGSI_TOKEN_TYPE_INSTRUCTION:
   {
      struct tgsi_full_instruction *inst = &ctx->FullToken.FullInstruction;

      memset(inst, 0, sizeof *inst);
      copy_token(&inst->Instruction, &token);

      if (inst->Instruction.Predicate) {
         next_token(ctx, &inst->Predicate);
      }

      if (inst->Instruction.Label) {
         next_token( ctx, &inst->Label);
      }

      if (inst->Instruction.Texture) {
         next_token( ctx, &inst->Texture);
         for( i = 0; i < inst->Texture.NumOffsets; i++ ) {
            next_token( ctx, &inst->TexOffsets[i] );
         }
      }

      assert( inst->Instruction.NumDstRegs <= TGSI_FULL_MAX_DST_REGISTERS );

      for(  i = 0; i < inst->Instruction.NumDstRegs; i++ ) {

         next_token( ctx, &inst->Dst[i].Register );

         if( inst->Dst[i].Register.Indirect ) {
            next_token( ctx, &inst->Dst[i].Indirect );

            /*
             * No support for indirect or multi-dimensional addressing.
             */
            assert( !inst->Dst[i].Indirect.Dimension );
            assert( !inst->Dst[i].Indirect.Indirect );
         }
         if( inst->Dst[i].Register.Dimension ) {
            next_token( ctx, &inst->Dst[i].Dimension );

            /*
             * No support for multi-dimensional addressing.
             */
            assert( !inst->Dst[i].Dimension.Dimension );

            if( inst->Dst[i].Dimension.Indirect ) {
               next_token( ctx, &inst->Dst[i].DimIndirect );

               /*
                * No support for indirect or multi-dimensional addressing.
                */
               assert( !inst->Dst[i].Indirect.Indirect );
               assert( !inst->Dst[i].Indirect.Dimension );
            }
         }
      }

      assert( inst->Instruction.NumSrcRegs <= TGSI_FULL_MAX_SRC_REGISTERS );

      for( i = 0; i < inst->Instruction.NumSrcRegs; i++ ) {

         next_token( ctx, &inst->Src[i].Register );

         if( inst->Src[i].Register.Indirect ) {
            next_token( ctx, &inst->Src[i].Indirect );

            /*
             * No support for indirect or multi-dimensional addressing.
             */
            assert( !inst->Src[i].Indirect.Indirect );
            assert( !inst->Src[i].Indirect.Dimension );
         }

         if( inst->Src[i].Register.Dimension ) {
            next_token( ctx, &inst->Src[i].Dimension );

            /*
             * No support for multi-dimensional addressing.
             */
            assert( !inst->Src[i].Dimension.Dimension );

            if( inst->Src[i].Dimension.Indirect ) {
               next_token( ctx, &inst->Src[i].DimIndirect );

               /*
               * No support for indirect or multi-dimensional addressing.
               */
               assert( !inst->Src[i].Indirect.Indirect );
               assert( !inst->Src[i].Indirect.Dimension );
            }
         }
      }

      break;
   }

   case TGSI_TOKEN_TYPE_PROPERTY:
   {
      struct tgsi_full_property *prop = &ctx->FullToken.FullProperty;
      uint prop_count;

      memset(prop, 0, sizeof *prop);
      copy_token(&prop->Property, &token);

      prop_count = prop->Property.NrTokens - 1;
      for (i = 0; i < prop_count; i++) {
         next_token(ctx, &prop->u[i]);
      }

      break;
   }

   default:
      assert( 0 );
   }
}




/**
 * Make a new copy of a token array.
 */
struct tgsi_token *
tgsi_dup_tokens(const struct tgsi_token *tokens)
{
   unsigned n = tgsi_num_tokens(tokens);
   unsigned bytes = n * sizeof(struct tgsi_token);
   struct tgsi_token *new_tokens = (struct tgsi_token *) MALLOC(bytes);
   if (new_tokens)
      memcpy(new_tokens, tokens, bytes);
   return new_tokens;
}


/**
 * Allocate memory for num_tokens tokens.
 */
struct tgsi_token *
tgsi_alloc_tokens(unsigned num_tokens)
{
   unsigned bytes = num_tokens * sizeof(struct tgsi_token);
   return (struct tgsi_token *) MALLOC(bytes);
}


void
tgsi_dump_tokens(const struct tgsi_token *tokens)
{
   const unsigned *dwords = (const unsigned *)tokens;
   int nr = tgsi_num_tokens(tokens);
   int i;
   
   assert(sizeof(*tokens) == sizeof(unsigned));

   debug_printf("const unsigned tokens[%d] = {\n", nr);
   for (i = 0; i < nr; i++)
      debug_printf("0x%08x,\n", dwords[i]);
   debug_printf("};\n");
}
