/**************************************************************************
 *
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "draw_llvm.h"

#include "draw_context.h"
#include "draw_vs.h"

#include "gallivm/lp_bld_arit.h"
#include "gallivm/lp_bld_logic.h"
#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_swizzle.h"
#include "gallivm/lp_bld_struct.h"
#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_flow.h"
#include "gallivm/lp_bld_debug.h"
#include "gallivm/lp_bld_tgsi.h"
#include "gallivm/lp_bld_printf.h"
#include "gallivm/lp_bld_intr.h"
#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_type.h"

#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_dump.h"

#include "util/u_math.h"
#include "util/u_pointer.h"
#include "util/u_string.h"
#include "util/u_simple_list.h"


#define DEBUG_STORE 0


/**
 * This function is called by the gallivm "garbage collector" when
 * the LLVM global data structures are freed.  We must free all LLVM-related
 * data.  Specifically, all JIT'd shader variants.
 */
static void
draw_llvm_garbage_collect_callback(void *cb_data)
{
   struct draw_llvm *llvm = (struct draw_llvm *) cb_data;
   struct draw_context *draw = llvm->draw;
   struct draw_llvm_variant_list_item *li;

   /* Ensure prepare will be run and shaders recompiled */
   assert(!draw->suspend_flushing);
   draw_do_flush(draw, DRAW_FLUSH_STATE_CHANGE);

   /* free all shader variants */
   li = first_elem(&llvm->vs_variants_list);
   while (!at_end(&llvm->vs_variants_list, li)) {
      struct draw_llvm_variant_list_item *next = next_elem(li);
      draw_llvm_destroy_variant(li->base);
      li = next;
   }

   /* Null-out these pointers so they get remade next time they're needed.
    * See the accessor functions below.
    */
   llvm->context_ptr_type = NULL;
   llvm->buffer_ptr_type = NULL;
   llvm->vb_ptr_type = NULL;
   llvm->vertex_header_ptr_type = NULL;
}


static void
draw_llvm_generate(struct draw_llvm *llvm, struct draw_llvm_variant *var,
                   boolean elts);


/**
 * Create LLVM type for struct draw_jit_texture
 */
static LLVMTypeRef
create_jit_texture_type(struct gallivm_state *gallivm, const char *struct_name)
{
   LLVMTargetDataRef target = gallivm->target;
   LLVMTypeRef texture_type;
   LLVMTypeRef elem_types[DRAW_JIT_TEXTURE_NUM_FIELDS];
   LLVMTypeRef int32_type = LLVMInt32TypeInContext(gallivm->context);

   elem_types[DRAW_JIT_TEXTURE_WIDTH]  =
   elem_types[DRAW_JIT_TEXTURE_HEIGHT] =
   elem_types[DRAW_JIT_TEXTURE_DEPTH] =
   elem_types[DRAW_JIT_TEXTURE_FIRST_LEVEL] =
   elem_types[DRAW_JIT_TEXTURE_LAST_LEVEL] = int32_type;
   elem_types[DRAW_JIT_TEXTURE_ROW_STRIDE] =
   elem_types[DRAW_JIT_TEXTURE_IMG_STRIDE] =
      LLVMArrayType(int32_type, PIPE_MAX_TEXTURE_LEVELS);
   elem_types[DRAW_JIT_TEXTURE_DATA] =
      LLVMArrayType(LLVMPointerType(LLVMInt8TypeInContext(gallivm->context), 0),
                    PIPE_MAX_TEXTURE_LEVELS);
   elem_types[DRAW_JIT_TEXTURE_MIN_LOD] =
   elem_types[DRAW_JIT_TEXTURE_MAX_LOD] =
   elem_types[DRAW_JIT_TEXTURE_LOD_BIAS] = LLVMFloatTypeInContext(gallivm->context);
   elem_types[DRAW_JIT_TEXTURE_BORDER_COLOR] = 
      LLVMArrayType(LLVMFloatTypeInContext(gallivm->context), 4);

#if HAVE_LLVM >= 0x0300
   texture_type = LLVMStructCreateNamed(gallivm->context, struct_name);
   LLVMStructSetBody(texture_type, elem_types,
                     Elements(elem_types), 0);
#else
   texture_type = LLVMStructTypeInContext(gallivm->context, elem_types,
                                          Elements(elem_types), 0);

   LLVMAddTypeName(gallivm->module, struct_name, texture_type);

   /* Make sure the target's struct layout cache doesn't return
    * stale/invalid data.
    */
   LLVMInvalidateStructLayout(gallivm->target, texture_type);
#endif

   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, width,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_WIDTH);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, height,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_HEIGHT);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, depth,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_DEPTH);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, first_level,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_FIRST_LEVEL);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, last_level,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_LAST_LEVEL);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, row_stride,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_ROW_STRIDE);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, img_stride,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_IMG_STRIDE);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, data,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_DATA);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, min_lod,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_MIN_LOD);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, max_lod,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_MAX_LOD);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, lod_bias,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_LOD_BIAS);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_texture, border_color,
                          target, texture_type,
                          DRAW_JIT_TEXTURE_BORDER_COLOR);

   LP_CHECK_STRUCT_SIZE(struct draw_jit_texture, target, texture_type);

   return texture_type;
}


/**
 * Create LLVM type for struct draw_jit_texture
 */
static LLVMTypeRef
create_jit_context_type(struct gallivm_state *gallivm,
                        LLVMTypeRef texture_type, const char *struct_name)
{
   LLVMTargetDataRef target = gallivm->target;
   LLVMTypeRef float_type = LLVMFloatTypeInContext(gallivm->context);
   LLVMTypeRef elem_types[5];
   LLVMTypeRef context_type;

   elem_types[0] = LLVMPointerType(float_type, 0); /* vs_constants */
   elem_types[1] = LLVMPointerType(float_type, 0); /* gs_constants */
   elem_types[2] = LLVMPointerType(LLVMArrayType(LLVMArrayType(float_type, 4),
                                                 DRAW_TOTAL_CLIP_PLANES), 0);
   elem_types[3] = LLVMPointerType(float_type, 0); /* viewport */
   elem_types[4] = LLVMArrayType(texture_type,
                                 PIPE_MAX_VERTEX_SAMPLERS); /* textures */
#if HAVE_LLVM >= 0x0300
   context_type = LLVMStructCreateNamed(gallivm->context, struct_name);
   LLVMStructSetBody(context_type, elem_types,
                     Elements(elem_types), 0);
#else
   context_type = LLVMStructTypeInContext(gallivm->context, elem_types,
                                          Elements(elem_types), 0);
   LLVMAddTypeName(gallivm->module, struct_name, context_type);

   LLVMInvalidateStructLayout(gallivm->target, context_type);
#endif

   LP_CHECK_MEMBER_OFFSET(struct draw_jit_context, vs_constants,
                          target, context_type, 0);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_context, gs_constants,
                          target, context_type, 1);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_context, planes,
                          target, context_type, 2);
   LP_CHECK_MEMBER_OFFSET(struct draw_jit_context, textures,
                          target, context_type,
                          DRAW_JIT_CTX_TEXTURES);
   LP_CHECK_STRUCT_SIZE(struct draw_jit_context,
                        target, context_type);

   return context_type;
}


/**
 * Create LLVM type for struct pipe_vertex_buffer
 */
static LLVMTypeRef
create_jit_vertex_buffer_type(struct gallivm_state *gallivm, const char *struct_name)
{
   LLVMTargetDataRef target = gallivm->target;
   LLVMTypeRef elem_types[3];
   LLVMTypeRef vb_type;

   elem_types[0] =
   elem_types[1] = LLVMInt32TypeInContext(gallivm->context);
   elem_types[2] = LLVMPointerType(LLVMInt8TypeInContext(gallivm->context), 0); /* vs_constants */

#if HAVE_LLVM >= 0x0300
   vb_type = LLVMStructCreateNamed(gallivm->context, struct_name);
   LLVMStructSetBody(vb_type, elem_types,
                     Elements(elem_types), 0);
#else
   vb_type = LLVMStructTypeInContext(gallivm->context, elem_types,
                                     Elements(elem_types), 0);
   LLVMAddTypeName(gallivm->module, struct_name, vb_type);

   LLVMInvalidateStructLayout(gallivm->target, vb_type);
#endif

   LP_CHECK_MEMBER_OFFSET(struct pipe_vertex_buffer, stride,
                          target, vb_type, 0);
   LP_CHECK_MEMBER_OFFSET(struct pipe_vertex_buffer, buffer_offset,
                          target, vb_type, 1);

   LP_CHECK_STRUCT_SIZE(struct pipe_vertex_buffer, target, vb_type);

   return vb_type;
}


/**
 * Create LLVM type for struct vertex_header;
 */
static LLVMTypeRef
create_jit_vertex_header(struct gallivm_state *gallivm, int data_elems)
{
   LLVMTargetDataRef target = gallivm->target;
   LLVMTypeRef elem_types[4];
   LLVMTypeRef vertex_header;
   char struct_name[24];

   util_snprintf(struct_name, 23, "vertex_header%d", data_elems);

   elem_types[DRAW_JIT_VERTEX_VERTEX_ID]  = LLVMIntTypeInContext(gallivm->context, 32);
   elem_types[DRAW_JIT_VERTEX_CLIP]  = LLVMArrayType(LLVMFloatTypeInContext(gallivm->context), 4);
   elem_types[DRAW_JIT_VERTEX_PRE_CLIP_POS]  = LLVMArrayType(LLVMFloatTypeInContext(gallivm->context), 4);
   elem_types[DRAW_JIT_VERTEX_DATA]  = LLVMArrayType(elem_types[1], data_elems);

#if HAVE_LLVM >= 0x0300
   vertex_header = LLVMStructCreateNamed(gallivm->context, struct_name);
   LLVMStructSetBody(vertex_header, elem_types,
                     Elements(elem_types), 0);
#else
   vertex_header = LLVMStructTypeInContext(gallivm->context, elem_types,
                                           Elements(elem_types), 0);
   LLVMAddTypeName(gallivm->module, struct_name, vertex_header);

   LLVMInvalidateStructLayout(gallivm->target, vertex_header);
#endif

   /* these are bit-fields and we can't take address of them
      LP_CHECK_MEMBER_OFFSET(struct vertex_header, clipmask,
      target, vertex_header,
      DRAW_JIT_VERTEX_CLIPMASK);
      LP_CHECK_MEMBER_OFFSET(struct vertex_header, edgeflag,
      target, vertex_header,
      DRAW_JIT_VERTEX_EDGEFLAG);
      LP_CHECK_MEMBER_OFFSET(struct vertex_header, pad,
      target, vertex_header,
      DRAW_JIT_VERTEX_PAD);
      LP_CHECK_MEMBER_OFFSET(struct vertex_header, vertex_id,
      target, vertex_header,
      DRAW_JIT_VERTEX_VERTEX_ID);
   */
   LP_CHECK_MEMBER_OFFSET(struct vertex_header, clip,
                          target, vertex_header,
                          DRAW_JIT_VERTEX_CLIP);
   LP_CHECK_MEMBER_OFFSET(struct vertex_header, pre_clip_pos,
                          target, vertex_header,
                          DRAW_JIT_VERTEX_PRE_CLIP_POS);
   LP_CHECK_MEMBER_OFFSET(struct vertex_header, data,
                          target, vertex_header,
                          DRAW_JIT_VERTEX_DATA);

   return vertex_header;
}


/**
 * Create LLVM types for various structures.
 */
static void
create_jit_types(struct draw_llvm *llvm)
{
   struct gallivm_state *gallivm = llvm->gallivm;
   LLVMTypeRef texture_type, context_type, buffer_type, vb_type;

   texture_type = create_jit_texture_type(gallivm, "texture");

   context_type = create_jit_context_type(gallivm, texture_type, "draw_jit_context");
   llvm->context_ptr_type = LLVMPointerType(context_type, 0);

   buffer_type = LLVMPointerType(LLVMIntTypeInContext(gallivm->context, 8), 0);
   llvm->buffer_ptr_type = LLVMPointerType(buffer_type, 0);

   vb_type = create_jit_vertex_buffer_type(gallivm, "pipe_vertex_buffer");
   llvm->vb_ptr_type = LLVMPointerType(vb_type, 0);
}


static LLVMTypeRef
get_context_ptr_type(struct draw_llvm *llvm)
{
   if (!llvm->context_ptr_type)
      create_jit_types(llvm);
   return llvm->context_ptr_type;
}


static LLVMTypeRef
get_buffer_ptr_type(struct draw_llvm *llvm)
{
   if (!llvm->buffer_ptr_type)
      create_jit_types(llvm);
   return llvm->buffer_ptr_type;
}


static LLVMTypeRef
get_vb_ptr_type(struct draw_llvm *llvm)
{
   if (!llvm->vb_ptr_type)
      create_jit_types(llvm);
   return llvm->vb_ptr_type;
}

static LLVMTypeRef
get_vertex_header_ptr_type(struct draw_llvm *llvm)
{
   if (!llvm->vertex_header_ptr_type)
      create_jit_types(llvm);
   return llvm->vertex_header_ptr_type;
}


/**
 * Create per-context LLVM info.
 */
struct draw_llvm *
draw_llvm_create(struct draw_context *draw, struct gallivm_state *gallivm)
{
   struct draw_llvm *llvm;

   llvm = CALLOC_STRUCT( draw_llvm );
   if (!llvm)
      return NULL;

   lp_build_init();

   llvm->draw = draw;
   llvm->gallivm = gallivm;

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      LLVMDumpModule(llvm->gallivm->module);
   }

   llvm->nr_variants = 0;
   make_empty_list(&llvm->vs_variants_list);

   gallivm_register_garbage_collector_callback(
                              draw_llvm_garbage_collect_callback, llvm);

   return llvm;
}


/**
 * Free per-context LLVM info.
 */
void
draw_llvm_destroy(struct draw_llvm *llvm)
{
   gallivm_remove_garbage_collector_callback(
                              draw_llvm_garbage_collect_callback, llvm);

   /* XXX free other draw_llvm data? */
   FREE(llvm);
}


/**
 * Create LLVM-generated code for a vertex shader.
 */
struct draw_llvm_variant *
draw_llvm_create_variant(struct draw_llvm *llvm,
			 unsigned num_inputs,
			 const struct draw_llvm_variant_key *key)
{
   struct draw_llvm_variant *variant;
   struct llvm_vertex_shader *shader =
      llvm_vertex_shader(llvm->draw->vs.vertex_shader);
   LLVMTypeRef vertex_header;

   variant = MALLOC(sizeof *variant +
		    shader->variant_key_size -
		    sizeof variant->key);
   if (variant == NULL)
      return NULL;

   variant->llvm = llvm;

   memcpy(&variant->key, key, shader->variant_key_size);

   vertex_header = create_jit_vertex_header(llvm->gallivm, num_inputs);

   llvm->vertex_header_ptr_type = LLVMPointerType(vertex_header, 0);

   draw_llvm_generate(llvm, variant, FALSE);  /* linear */
   draw_llvm_generate(llvm, variant, TRUE);   /* elts */

   variant->shader = shader;
   variant->list_item_global.base = variant;
   variant->list_item_local.base = variant;
   /*variant->no = */shader->variants_created++;
   variant->list_item_global.base = variant;

   return variant;
}


static void
generate_vs(struct draw_llvm *llvm,
            LLVMBuilderRef builder,
            LLVMValueRef (*outputs)[NUM_CHANNELS],
            const LLVMValueRef (*inputs)[NUM_CHANNELS],
            LLVMValueRef system_values_array,
            LLVMValueRef context_ptr,
            struct lp_build_sampler_soa *draw_sampler,
            boolean clamp_vertex_color)
{
   const struct tgsi_token *tokens = llvm->draw->vs.vertex_shader->state.tokens;
   struct lp_type vs_type;
   LLVMValueRef consts_ptr = draw_jit_context_vs_constants(llvm->gallivm, context_ptr);
   struct lp_build_sampler_soa *sampler = 0;

   memset(&vs_type, 0, sizeof vs_type);
   vs_type.floating = TRUE; /* floating point values */
   vs_type.sign = TRUE;     /* values are signed */
   vs_type.norm = FALSE;    /* values are not limited to [0,1] or [-1,1] */
   vs_type.width = 32;      /* 32-bit float */
   vs_type.length = 4;      /* 4 elements per vector */
#if 0
   num_vs = 4;              /* number of vertices per block */
#endif

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      tgsi_dump(tokens, 0);
   }

   if (llvm->draw->num_sampler_views && llvm->draw->num_samplers)
      sampler = draw_sampler;

   lp_build_tgsi_soa(llvm->gallivm,
                     tokens,
                     vs_type,
                     NULL /*struct lp_build_mask_context *mask*/,
                     consts_ptr,
                     system_values_array,
                     NULL /*pos*/,
                     inputs,
                     outputs,
                     sampler,
                     &llvm->draw->vs.vertex_shader->info);

   if (clamp_vertex_color) {
      LLVMValueRef out;
      unsigned chan, attrib;
      struct lp_build_context bld;
      struct tgsi_shader_info* info = &llvm->draw->vs.vertex_shader->info;
      lp_build_context_init(&bld, llvm->gallivm, vs_type);

      for (attrib = 0; attrib < info->num_outputs; ++attrib) {
         for (chan = 0; chan < NUM_CHANNELS; ++chan) {
            if (outputs[attrib][chan]) {
               switch (info->output_semantic_name[attrib]) {
               case TGSI_SEMANTIC_COLOR:
               case TGSI_SEMANTIC_BCOLOR:
                  out = LLVMBuildLoad(builder, outputs[attrib][chan], "");
                  out = lp_build_clamp(&bld, out, bld.zero, bld.one);
                  LLVMBuildStore(builder, out, outputs[attrib][chan]);
                  break;
               }
            }
         }
      }
   }
}


#if DEBUG_STORE
static void print_vectorf(LLVMBuilderRef builder,
                         LLVMValueRef vec)
{
   LLVMValueRef val[4];
   val[0] = LLVMBuildExtractElement(builder, vec,
                                    lp_build_const_int32(gallivm, 0), "");
   val[1] = LLVMBuildExtractElement(builder, vec,
                                    lp_build_const_int32(gallivm, 1), "");
   val[2] = LLVMBuildExtractElement(builder, vec,
                                    lp_build_const_int32(gallivm, 2), "");
   val[3] = LLVMBuildExtractElement(builder, vec,
                                    lp_build_const_int32(gallivm, 3), "");
   lp_build_printf(builder, "vector = [%f, %f, %f, %f]\n",
                   val[0], val[1], val[2], val[3]);
}
#endif


static void
generate_fetch(struct gallivm_state *gallivm,
               LLVMValueRef vbuffers_ptr,
               LLVMValueRef *res,
               struct pipe_vertex_element *velem,
               LLVMValueRef vbuf,
               LLVMValueRef index,
               LLVMValueRef instance_id)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef indices =
      LLVMConstInt(LLVMInt64TypeInContext(gallivm->context),
                   velem->vertex_buffer_index, 0);
   LLVMValueRef vbuffer_ptr = LLVMBuildGEP(builder, vbuffers_ptr,
                                           &indices, 1, "");
   LLVMValueRef vb_stride = draw_jit_vbuffer_stride(gallivm, vbuf);
   LLVMValueRef vb_buffer_offset = draw_jit_vbuffer_offset(gallivm, vbuf);
   LLVMValueRef stride;

   if (velem->instance_divisor) {
      /* array index = instance_id / instance_divisor */
      index = LLVMBuildUDiv(builder, instance_id,
                            lp_build_const_int32(gallivm, velem->instance_divisor),
                            "instance_divisor");
   }

   stride = LLVMBuildMul(builder, vb_stride, index, "");

   vbuffer_ptr = LLVMBuildLoad(builder, vbuffer_ptr, "vbuffer");

   stride = LLVMBuildAdd(builder, stride,
                         vb_buffer_offset,
                         "");
   stride = LLVMBuildAdd(builder, stride,
                         lp_build_const_int32(gallivm, velem->src_offset),
                         "");

   /*lp_build_printf(builder, "vbuf index = %d, stride is %d\n", indices, stride);*/
   vbuffer_ptr = LLVMBuildGEP(builder, vbuffer_ptr, &stride, 1, "");

   *res = draw_llvm_translate_from(gallivm, vbuffer_ptr, velem->src_format);
}


static LLVMValueRef
aos_to_soa(struct gallivm_state *gallivm,
           LLVMValueRef val0,
           LLVMValueRef val1,
           LLVMValueRef val2,
           LLVMValueRef val3,
           LLVMValueRef channel)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef ex, res;

   ex = LLVMBuildExtractElement(builder, val0,
                                channel, "");
   res = LLVMBuildInsertElement(builder,
                                LLVMConstNull(LLVMTypeOf(val0)),
                                ex,
                                lp_build_const_int32(gallivm, 0),
                                "");

   ex = LLVMBuildExtractElement(builder, val1,
                                channel, "");
   res = LLVMBuildInsertElement(builder,
                                res, ex,
                                lp_build_const_int32(gallivm, 1),
                                "");

   ex = LLVMBuildExtractElement(builder, val2,
                                channel, "");
   res = LLVMBuildInsertElement(builder,
                                res, ex,
                                lp_build_const_int32(gallivm, 2),
                                "");

   ex = LLVMBuildExtractElement(builder, val3,
                                channel, "");
   res = LLVMBuildInsertElement(builder,
                                res, ex,
                                lp_build_const_int32(gallivm, 3),
                                "");

   return res;
}


static void
soa_to_aos(struct gallivm_state *gallivm,
           LLVMValueRef soa[NUM_CHANNELS],
           LLVMValueRef aos[NUM_CHANNELS])
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef comp;
   int i = 0;

   debug_assert(NUM_CHANNELS == 4);

   aos[0] = LLVMConstNull(LLVMTypeOf(soa[0]));
   aos[1] = aos[2] = aos[3] = aos[0];

   for (i = 0; i < NUM_CHANNELS; ++i) {
      LLVMValueRef channel = lp_build_const_int32(gallivm, i);

      comp = LLVMBuildExtractElement(builder, soa[i],
                                     lp_build_const_int32(gallivm, 0), "");
      aos[0] = LLVMBuildInsertElement(builder, aos[0], comp, channel, "");

      comp = LLVMBuildExtractElement(builder, soa[i],
                                     lp_build_const_int32(gallivm, 1), "");
      aos[1] = LLVMBuildInsertElement(builder, aos[1], comp, channel, "");

      comp = LLVMBuildExtractElement(builder, soa[i],
                                     lp_build_const_int32(gallivm, 2), "");
      aos[2] = LLVMBuildInsertElement(builder, aos[2], comp, channel, "");

      comp = LLVMBuildExtractElement(builder, soa[i],
                                     lp_build_const_int32(gallivm, 3), "");
      aos[3] = LLVMBuildInsertElement(builder, aos[3], comp, channel, "");

   }
}


static void
convert_to_soa(struct gallivm_state *gallivm,
               LLVMValueRef (*aos)[NUM_CHANNELS],
               LLVMValueRef (*soa)[NUM_CHANNELS],
               int num_attribs)
{
   int i;

   debug_assert(NUM_CHANNELS == 4);

   for (i = 0; i < num_attribs; ++i) {
      LLVMValueRef val0 = aos[i][0];
      LLVMValueRef val1 = aos[i][1];
      LLVMValueRef val2 = aos[i][2];
      LLVMValueRef val3 = aos[i][3];

      soa[i][0] = aos_to_soa(gallivm, val0, val1, val2, val3,
                             lp_build_const_int32(gallivm, 0));
      soa[i][1] = aos_to_soa(gallivm, val0, val1, val2, val3,
                             lp_build_const_int32(gallivm, 1));
      soa[i][2] = aos_to_soa(gallivm, val0, val1, val2, val3,
                             lp_build_const_int32(gallivm, 2));
      soa[i][3] = aos_to_soa(gallivm, val0, val1, val2, val3,
                             lp_build_const_int32(gallivm, 3));
   }
}


static void
store_aos(struct gallivm_state *gallivm,
          LLVMValueRef io_ptr,
          LLVMValueRef index,
          LLVMValueRef value,
          LLVMValueRef clipmask)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef id_ptr = draw_jit_header_id(gallivm, io_ptr);
   LLVMValueRef data_ptr = draw_jit_header_data(gallivm, io_ptr);
   LLVMValueRef indices[3];
   LLVMValueRef val;
   int vertex_id_pad_edgeflag;

   indices[0] = lp_build_const_int32(gallivm, 0);
   indices[1] = index;
   indices[2] = lp_build_const_int32(gallivm, 0);

   /* If this assertion fails, it means we need to update the bit twidding
    * code here.  See struct vertex_header in draw_private.h.
    */
   assert(DRAW_TOTAL_CLIP_PLANES==14);
   /* initialize vertex id:16 = 0xffff, pad:1 = 0, edgeflag:1 = 1 */
   vertex_id_pad_edgeflag = (0xffff << 16) | (1 << DRAW_TOTAL_CLIP_PLANES);
   val = lp_build_const_int32(gallivm, vertex_id_pad_edgeflag);
   /* OR with the clipmask */
   val = LLVMBuildOr(builder, val, clipmask, "");               

   /* store vertex header */
   LLVMBuildStore(builder, val, id_ptr);


#if DEBUG_STORE
   lp_build_printf(builder, "    ---- %p storing attribute %d (io = %p)\n", data_ptr, index, io_ptr);
#endif
#if 0
   /*lp_build_printf(builder, " ---- %p storing at %d (%p)  ", io_ptr, index, data_ptr);
     print_vectorf(builder, value);*/
   data_ptr = LLVMBuildBitCast(builder, data_ptr,
                               LLVMPointerType(LLVMArrayType(LLVMVectorType(LLVMFloatTypeInContext(gallivm->context), 4), 0), 0),
                               "datavec");
   data_ptr = LLVMBuildGEP(builder, data_ptr, indices, 2, "");

   LLVMBuildStore(builder, value, data_ptr);
#else
   {
      LLVMValueRef x, y, z, w;
      LLVMValueRef idx0, idx1, idx2, idx3;
      LLVMValueRef gep0, gep1, gep2, gep3;
      data_ptr = LLVMBuildGEP(builder, data_ptr, indices, 3, "");

      idx0 = lp_build_const_int32(gallivm, 0);
      idx1 = lp_build_const_int32(gallivm, 1);
      idx2 = lp_build_const_int32(gallivm, 2);
      idx3 = lp_build_const_int32(gallivm, 3);

      x = LLVMBuildExtractElement(builder, value,
                                  idx0, "");
      y = LLVMBuildExtractElement(builder, value,
                                  idx1, "");
      z = LLVMBuildExtractElement(builder, value,
                                  idx2, "");
      w = LLVMBuildExtractElement(builder, value,
                                  idx3, "");

      gep0 = LLVMBuildGEP(builder, data_ptr, &idx0, 1, "");
      gep1 = LLVMBuildGEP(builder, data_ptr, &idx1, 1, "");
      gep2 = LLVMBuildGEP(builder, data_ptr, &idx2, 1, "");
      gep3 = LLVMBuildGEP(builder, data_ptr, &idx3, 1, "");

      /*lp_build_printf(builder, "##### x = %f (%p), y = %f (%p), z = %f (%p), w = %f (%p)\n",
        x, gep0, y, gep1, z, gep2, w, gep3);*/
      LLVMBuildStore(builder, x, gep0);
      LLVMBuildStore(builder, y, gep1);
      LLVMBuildStore(builder, z, gep2);
      LLVMBuildStore(builder, w, gep3);
   }
#endif
}


static void
store_aos_array(struct gallivm_state *gallivm,
                LLVMValueRef io_ptr,
                LLVMValueRef aos[NUM_CHANNELS],
                int attrib,
                int num_outputs,
                LLVMValueRef clipmask)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef attr_index = lp_build_const_int32(gallivm, attrib);
   LLVMValueRef ind0 = lp_build_const_int32(gallivm, 0);
   LLVMValueRef ind1 = lp_build_const_int32(gallivm, 1);
   LLVMValueRef ind2 = lp_build_const_int32(gallivm, 2);
   LLVMValueRef ind3 = lp_build_const_int32(gallivm, 3);
   LLVMValueRef io0_ptr, io1_ptr, io2_ptr, io3_ptr;
   LLVMValueRef clipmask0, clipmask1, clipmask2, clipmask3;
   
   debug_assert(NUM_CHANNELS == 4);

   io0_ptr = LLVMBuildGEP(builder, io_ptr,
                          &ind0, 1, "");
   io1_ptr = LLVMBuildGEP(builder, io_ptr,
                          &ind1, 1, "");
   io2_ptr = LLVMBuildGEP(builder, io_ptr,
                          &ind2, 1, "");
   io3_ptr = LLVMBuildGEP(builder, io_ptr,
                          &ind3, 1, "");

   clipmask0 = LLVMBuildExtractElement(builder, clipmask,
                                       ind0, "");
   clipmask1 = LLVMBuildExtractElement(builder, clipmask,
                                       ind1, "");
   clipmask2 = LLVMBuildExtractElement(builder, clipmask,
                                       ind2, "");
   clipmask3 = LLVMBuildExtractElement(builder, clipmask,
                                       ind3, "");

#if DEBUG_STORE
   lp_build_printf(builder, "io = %p, indexes[%d, %d, %d, %d]\n, clipmask0 = %x, clipmask1 = %x, clipmask2 = %x, clipmask3 = %x\n",
                   io_ptr, ind0, ind1, ind2, ind3, clipmask0, clipmask1, clipmask2, clipmask3);
#endif
   /* store for each of the 4 vertices */
   store_aos(gallivm, io0_ptr, attr_index, aos[0], clipmask0);
   store_aos(gallivm, io1_ptr, attr_index, aos[1], clipmask1);
   store_aos(gallivm, io2_ptr, attr_index, aos[2], clipmask2);
   store_aos(gallivm, io3_ptr, attr_index, aos[3], clipmask3);
}


static void
convert_to_aos(struct gallivm_state *gallivm,
               LLVMValueRef io,
               LLVMValueRef (*outputs)[NUM_CHANNELS],
               LLVMValueRef clipmask,
               int num_outputs,
               int max_vertices)
{
   LLVMBuilderRef builder = gallivm->builder;
   unsigned chan, attrib;

#if DEBUG_STORE
   lp_build_printf(builder, "   # storing begin\n");
#endif
   for (attrib = 0; attrib < num_outputs; ++attrib) {
      LLVMValueRef soa[4];
      LLVMValueRef aos[4];
      for (chan = 0; chan < NUM_CHANNELS; ++chan) {
         if (outputs[attrib][chan]) {
            LLVMValueRef out = LLVMBuildLoad(builder, outputs[attrib][chan], "");
            lp_build_name(out, "output%u.%c", attrib, "xyzw"[chan]);
            /*lp_build_printf(builder, "output %d : %d ",
                            LLVMConstInt(LLVMInt32Type(), attrib, 0),
                            LLVMConstInt(LLVMInt32Type(), chan, 0));
              print_vectorf(builder, out);*/
            soa[chan] = out;
         }
         else {
            soa[chan] = 0;
         }
      }
      soa_to_aos(gallivm, soa, aos);
      store_aos_array(gallivm,
                      io,
                      aos,
                      attrib,
                      num_outputs,
                      clipmask);
   }
#if DEBUG_STORE
   lp_build_printf(builder, "   # storing end\n");
#endif
}


/**
 * Stores original vertex positions in clip coordinates
 * There is probably a more efficient way to do this, 4 floats at once
 * rather than extracting each element one by one.
 */
static void
store_clip(struct gallivm_state *gallivm,
           LLVMValueRef io_ptr,           
           LLVMValueRef (*outputs)[NUM_CHANNELS],
           boolean pre_clip_pos)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef out[4];
   LLVMValueRef indices[2]; 
   LLVMValueRef io0_ptr, io1_ptr, io2_ptr, io3_ptr;
   LLVMValueRef clip_ptr0, clip_ptr1, clip_ptr2, clip_ptr3;
   LLVMValueRef clip0_ptr, clip1_ptr, clip2_ptr, clip3_ptr;    
   LLVMValueRef out0elem, out1elem, out2elem, out3elem;
   int i;

   LLVMValueRef ind0 = lp_build_const_int32(gallivm, 0);
   LLVMValueRef ind1 = lp_build_const_int32(gallivm, 1);
   LLVMValueRef ind2 = lp_build_const_int32(gallivm, 2);
   LLVMValueRef ind3 = lp_build_const_int32(gallivm, 3);
   
   indices[0] =
   indices[1] = lp_build_const_int32(gallivm, 0);
   
   out[0] = LLVMBuildLoad(builder, outputs[0][0], ""); /*x0 x1 x2 x3*/
   out[1] = LLVMBuildLoad(builder, outputs[0][1], ""); /*y0 y1 y2 y3*/
   out[2] = LLVMBuildLoad(builder, outputs[0][2], ""); /*z0 z1 z2 z3*/
   out[3] = LLVMBuildLoad(builder, outputs[0][3], ""); /*w0 w1 w2 w3*/  

   io0_ptr = LLVMBuildGEP(builder, io_ptr, &ind0, 1, "");
   io1_ptr = LLVMBuildGEP(builder, io_ptr, &ind1, 1, "");
   io2_ptr = LLVMBuildGEP(builder, io_ptr, &ind2, 1, "");
   io3_ptr = LLVMBuildGEP(builder, io_ptr, &ind3, 1, "");

   /* FIXME: this needs updating for clip vertex support */
   if (!pre_clip_pos) {
      clip_ptr0 = draw_jit_header_clip(gallivm, io0_ptr);
      clip_ptr1 = draw_jit_header_clip(gallivm, io1_ptr);
      clip_ptr2 = draw_jit_header_clip(gallivm, io2_ptr);
      clip_ptr3 = draw_jit_header_clip(gallivm, io3_ptr);
   } else {
      clip_ptr0 = draw_jit_header_pre_clip_pos(gallivm, io0_ptr);
      clip_ptr1 = draw_jit_header_pre_clip_pos(gallivm, io1_ptr);
      clip_ptr2 = draw_jit_header_pre_clip_pos(gallivm, io2_ptr);
      clip_ptr3 = draw_jit_header_pre_clip_pos(gallivm, io3_ptr);
   }

   for (i = 0; i<4; i++) {
      clip0_ptr = LLVMBuildGEP(builder, clip_ptr0, indices, 2, ""); /* x0 */
      clip1_ptr = LLVMBuildGEP(builder, clip_ptr1, indices, 2, ""); /* x1 */
      clip2_ptr = LLVMBuildGEP(builder, clip_ptr2, indices, 2, ""); /* x2 */
      clip3_ptr = LLVMBuildGEP(builder, clip_ptr3, indices, 2, ""); /* x3 */

      out0elem = LLVMBuildExtractElement(builder, out[i], ind0, ""); /* x0 */
      out1elem = LLVMBuildExtractElement(builder, out[i], ind1, ""); /* x1 */
      out2elem = LLVMBuildExtractElement(builder, out[i], ind2, ""); /* x2 */
      out3elem = LLVMBuildExtractElement(builder, out[i], ind3, ""); /* x3 */
  
      LLVMBuildStore(builder, out0elem, clip0_ptr);
      LLVMBuildStore(builder, out1elem, clip1_ptr);
      LLVMBuildStore(builder, out2elem, clip2_ptr);
      LLVMBuildStore(builder, out3elem, clip3_ptr);

      indices[1]= LLVMBuildAdd(builder, indices[1], ind1, "");
   }

}


/**
 * Equivalent of _mm_set1_ps(a)
 */
static LLVMValueRef
vec4f_from_scalar(struct gallivm_state *gallivm,
                  LLVMValueRef a,
                  const char *name)
{
   LLVMTypeRef float_type = LLVMFloatTypeInContext(gallivm->context);
   LLVMValueRef res = LLVMGetUndef(LLVMVectorType(float_type, 4));
   int i;

   for (i = 0; i < 4; ++i) {
      LLVMValueRef index = lp_build_const_int32(gallivm, i);
      res = LLVMBuildInsertElement(gallivm->builder, res, a,
                                   index, i == 3 ? name : "");
   }

   return res;
}


/**
 * Transforms the outputs for viewport mapping
 */
static void
generate_viewport(struct draw_llvm *llvm,
                  LLVMBuilderRef builder,
                  LLVMValueRef (*outputs)[NUM_CHANNELS],
                  LLVMValueRef context_ptr)
{
   int i;
   struct gallivm_state *gallivm = llvm->gallivm;
   struct lp_type f32_type = lp_type_float_vec(32);
   LLVMValueRef out3 = LLVMBuildLoad(builder, outputs[0][3], ""); /*w0 w1 w2 w3*/   
   LLVMValueRef const1 = lp_build_const_vec(gallivm, f32_type, 1.0);       /*1.0 1.0 1.0 1.0*/ 
   LLVMValueRef vp_ptr = draw_jit_context_viewport(gallivm, context_ptr);

   /* for 1/w convention*/
   out3 = LLVMBuildFDiv(builder, const1, out3, "");
   LLVMBuildStore(builder, out3, outputs[0][3]);
  
   /* Viewport Mapping */
   for (i=0; i<3; i++) {
      LLVMValueRef out = LLVMBuildLoad(builder, outputs[0][i], ""); /*x0 x1 x2 x3*/
      LLVMValueRef scale;
      LLVMValueRef trans;
      LLVMValueRef scale_i;
      LLVMValueRef trans_i;
      LLVMValueRef index;
      
      index = lp_build_const_int32(gallivm, i);
      scale_i = LLVMBuildGEP(builder, vp_ptr, &index, 1, "");

      index = lp_build_const_int32(gallivm, i+4);
      trans_i = LLVMBuildGEP(builder, vp_ptr, &index, 1, "");

      scale = vec4f_from_scalar(gallivm, LLVMBuildLoad(builder, scale_i, ""), "scale");
      trans = vec4f_from_scalar(gallivm, LLVMBuildLoad(builder, trans_i, ""), "trans");

      /* divide by w */
      out = LLVMBuildFMul(builder, out, out3, "");
      /* mult by scale */
      out = LLVMBuildFMul(builder, out, scale, "");
      /* add translation */
      out = LLVMBuildFAdd(builder, out, trans, "");

      /* store transformed outputs */
      LLVMBuildStore(builder, out, outputs[0][i]);
   }
   
}


/**
 * Returns clipmask as 4xi32 bitmask for the 4 vertices
 */
static LLVMValueRef 
generate_clipmask(struct gallivm_state *gallivm,
                  LLVMValueRef (*outputs)[NUM_CHANNELS],
                  boolean clip_xy,
                  boolean clip_z,
                  boolean clip_user,
                  boolean clip_halfz,
                  unsigned ucp_enable,
                  LLVMValueRef context_ptr)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef mask; /* stores the <4xi32> clipmasks */     
   LLVMValueRef test, temp; 
   LLVMValueRef zero, shift;
   LLVMValueRef pos_x, pos_y, pos_z, pos_w;
   LLVMValueRef plane1, planes, plane_ptr, sum;
   struct lp_type f32_type = lp_type_float_vec(32); 

   mask = lp_build_const_int_vec(gallivm, lp_type_int_vec(32), 0);
   temp = lp_build_const_int_vec(gallivm, lp_type_int_vec(32), 0);
   zero = lp_build_const_vec(gallivm, f32_type, 0);                    /* 0.0f 0.0f 0.0f 0.0f */
   shift = lp_build_const_int_vec(gallivm, lp_type_int_vec(32), 1);    /* 1 1 1 1 */

   /* Assuming position stored at output[0] */
   pos_x = LLVMBuildLoad(builder, outputs[0][0], ""); /*x0 x1 x2 x3*/
   pos_y = LLVMBuildLoad(builder, outputs[0][1], ""); /*y0 y1 y2 y3*/
   pos_z = LLVMBuildLoad(builder, outputs[0][2], ""); /*z0 z1 z2 z3*/
   pos_w = LLVMBuildLoad(builder, outputs[0][3], ""); /*w0 w1 w2 w3*/   

   /* Cliptest, for hardwired planes */
   if (clip_xy) {
      /* plane 1 */
      test = lp_build_compare(gallivm, f32_type, PIPE_FUNC_GREATER, pos_x , pos_w);
      temp = shift;
      test = LLVMBuildAnd(builder, test, temp, ""); 
      mask = test;
   
      /* plane 2 */
      test = LLVMBuildFAdd(builder, pos_x, pos_w, "");
      test = lp_build_compare(gallivm, f32_type, PIPE_FUNC_GREATER, zero, test);
      temp = LLVMBuildShl(builder, temp, shift, "");
      test = LLVMBuildAnd(builder, test, temp, ""); 
      mask = LLVMBuildOr(builder, mask, test, "");
   
      /* plane 3 */
      test = lp_build_compare(gallivm, f32_type, PIPE_FUNC_GREATER, pos_y, pos_w);
      temp = LLVMBuildShl(builder, temp, shift, "");
      test = LLVMBuildAnd(builder, test, temp, ""); 
      mask = LLVMBuildOr(builder, mask, test, "");

      /* plane 4 */
      test = LLVMBuildFAdd(builder, pos_y, pos_w, "");
      test = lp_build_compare(gallivm, f32_type, PIPE_FUNC_GREATER, zero, test);
      temp = LLVMBuildShl(builder, temp, shift, "");
      test = LLVMBuildAnd(builder, test, temp, ""); 
      mask = LLVMBuildOr(builder, mask, test, "");
   }

   if (clip_z) {
      temp = lp_build_const_int_vec(gallivm, lp_type_int_vec(32), 16);
      if (clip_halfz) {
         /* plane 5 */
         test = lp_build_compare(gallivm, f32_type, PIPE_FUNC_GREATER, zero, pos_z);
         test = LLVMBuildAnd(builder, test, temp, ""); 
         mask = LLVMBuildOr(builder, mask, test, "");
      }  
      else {
         /* plane 5 */
         test = LLVMBuildFAdd(builder, pos_z, pos_w, "");
         test = lp_build_compare(gallivm, f32_type, PIPE_FUNC_GREATER, zero, test);
         test = LLVMBuildAnd(builder, test, temp, ""); 
         mask = LLVMBuildOr(builder, mask, test, "");
      }
      /* plane 6 */
      test = lp_build_compare(gallivm, f32_type, PIPE_FUNC_GREATER, pos_z, pos_w);
      temp = LLVMBuildShl(builder, temp, shift, "");
      test = LLVMBuildAnd(builder, test, temp, ""); 
      mask = LLVMBuildOr(builder, mask, test, "");
   }   

   if (clip_user) {
      LLVMValueRef planes_ptr = draw_jit_context_planes(gallivm, context_ptr);
      LLVMValueRef indices[3];

      /* userclip planes */
      while (ucp_enable) {
         unsigned plane_idx = ffs(ucp_enable)-1;
         ucp_enable &= ~(1 << plane_idx);
         plane_idx += 6;

         indices[0] = lp_build_const_int32(gallivm, 0);
         indices[1] = lp_build_const_int32(gallivm, plane_idx);

         indices[2] = lp_build_const_int32(gallivm, 0);
         plane_ptr = LLVMBuildGEP(builder, planes_ptr, indices, 3, "");
         plane1 = LLVMBuildLoad(builder, plane_ptr, "plane_x");
         planes = vec4f_from_scalar(gallivm, plane1, "plane4_x");
         sum = LLVMBuildFMul(builder, planes, pos_x, "");

         indices[2] = lp_build_const_int32(gallivm, 1);
         plane_ptr = LLVMBuildGEP(builder, planes_ptr, indices, 3, "");
         plane1 = LLVMBuildLoad(builder, plane_ptr, "plane_y"); 
         planes = vec4f_from_scalar(gallivm, plane1, "plane4_y");
         test = LLVMBuildFMul(builder, planes, pos_y, "");
         sum = LLVMBuildFAdd(builder, sum, test, "");
         
         indices[2] = lp_build_const_int32(gallivm, 2);
         plane_ptr = LLVMBuildGEP(builder, planes_ptr, indices, 3, "");
         plane1 = LLVMBuildLoad(builder, plane_ptr, "plane_z"); 
         planes = vec4f_from_scalar(gallivm, plane1, "plane4_z");
         test = LLVMBuildFMul(builder, planes, pos_z, "");
         sum = LLVMBuildFAdd(builder, sum, test, "");

         indices[2] = lp_build_const_int32(gallivm, 3);
         plane_ptr = LLVMBuildGEP(builder, planes_ptr, indices, 3, "");
         plane1 = LLVMBuildLoad(builder, plane_ptr, "plane_w"); 
         planes = vec4f_from_scalar(gallivm, plane1, "plane4_w");
         test = LLVMBuildFMul(builder, planes, pos_w, "");
         sum = LLVMBuildFAdd(builder, sum, test, "");

         test = lp_build_compare(gallivm, f32_type, PIPE_FUNC_GREATER, zero, sum);
         temp = lp_build_const_int_vec(gallivm, lp_type_int_vec(32), 1 << plane_idx);
         test = LLVMBuildAnd(builder, test, temp, "");
         mask = LLVMBuildOr(builder, mask, test, "");
      }
   }
   return mask;
}


/**
 * Returns boolean if any clipping has occurred
 * Used zero/non-zero i32 value to represent boolean 
 */
static void
clipmask_bool(struct gallivm_state *gallivm,
              LLVMValueRef clipmask,
              LLVMValueRef ret_ptr)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef ret = LLVMBuildLoad(builder, ret_ptr, "");   
   LLVMValueRef temp;
   int i;

   for (i=0; i<4; i++) {   
      temp = LLVMBuildExtractElement(builder, clipmask,
                                     lp_build_const_int32(gallivm, i) , "");
      ret = LLVMBuildOr(builder, ret, temp, "");
   }
   
   LLVMBuildStore(builder, ret, ret_ptr);
}


static void
draw_llvm_generate(struct draw_llvm *llvm, struct draw_llvm_variant *variant,
                   boolean elts)
{
   struct gallivm_state *gallivm = llvm->gallivm;
   LLVMContextRef context = gallivm->context;
   LLVMTypeRef int32_type = LLVMInt32TypeInContext(context);
   LLVMTypeRef arg_types[8];
   LLVMTypeRef func_type;
   LLVMValueRef context_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef end, start;
   LLVMValueRef count, fetch_elts, fetch_count;
   LLVMValueRef stride, step, io_itr;
   LLVMValueRef io_ptr, vbuffers_ptr, vb_ptr;
   LLVMValueRef instance_id;
   LLVMValueRef system_values_array;
   LLVMValueRef zero = lp_build_const_int32(gallivm, 0);
   LLVMValueRef one = lp_build_const_int32(gallivm, 1);
   struct draw_context *draw = llvm->draw;
   const struct tgsi_shader_info *vs_info = &draw->vs.vertex_shader->info;
   unsigned i, j;
   struct lp_build_context bld;
   struct lp_build_loop_state lp_loop;
   const int max_vertices = 4;
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][NUM_CHANNELS];
   LLVMValueRef fetch_max;
   void *code;
   struct lp_build_sampler_soa *sampler = 0;
   LLVMValueRef ret, ret_ptr;
   const boolean bypass_viewport = variant->key.bypass_viewport;
   const boolean enable_cliptest = variant->key.clip_xy || 
                                   variant->key.clip_z  ||
                                   variant->key.clip_user;
   LLVMValueRef variant_func;

   arg_types[0] = get_context_ptr_type(llvm);       /* context */
   arg_types[1] = get_vertex_header_ptr_type(llvm); /* vertex_header */
   arg_types[2] = get_buffer_ptr_type(llvm);        /* vbuffers */
   if (elts)
      arg_types[3] = LLVMPointerType(int32_type, 0);/* fetch_elts * */
   else
      arg_types[3] = int32_type;                    /* start */
   arg_types[4] = int32_type;                       /* fetch_count / count */
   arg_types[5] = int32_type;                       /* stride */
   arg_types[6] = get_vb_ptr_type(llvm);            /* pipe_vertex_buffer's */
   arg_types[7] = int32_type;                       /* instance_id */

   func_type = LLVMFunctionType(int32_type, arg_types, Elements(arg_types), 0);

   variant_func = LLVMAddFunction(gallivm->module,
                                  elts ? "draw_llvm_shader_elts" : "draw_llvm_shader",
                                  func_type);

   if (elts)
      variant->function_elts = variant_func;
   else
      variant->function = variant_func;

   LLVMSetFunctionCallConv(variant_func, LLVMCCallConv);
   for (i = 0; i < Elements(arg_types); ++i)
      if (LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind)
         LLVMAddAttribute(LLVMGetParam(variant_func, i),
                          LLVMNoAliasAttribute);

   context_ptr  = LLVMGetParam(variant_func, 0);
   io_ptr       = LLVMGetParam(variant_func, 1);
   vbuffers_ptr = LLVMGetParam(variant_func, 2);
   stride       = LLVMGetParam(variant_func, 5);
   vb_ptr       = LLVMGetParam(variant_func, 6);
   instance_id  = LLVMGetParam(variant_func, 7);

   lp_build_name(context_ptr, "context");
   lp_build_name(io_ptr, "io");
   lp_build_name(vbuffers_ptr, "vbuffers");
   lp_build_name(stride, "stride");
   lp_build_name(vb_ptr, "vb");
   lp_build_name(instance_id, "instance_id");

   if (elts) {
      fetch_elts   = LLVMGetParam(variant_func, 3);
      fetch_count  = LLVMGetParam(variant_func, 4);
      lp_build_name(fetch_elts, "fetch_elts");
      lp_build_name(fetch_count, "fetch_count");
      start = count = NULL;
   }
   else {
      start        = LLVMGetParam(variant_func, 3);
      count        = LLVMGetParam(variant_func, 4);
      lp_build_name(start, "start");
      lp_build_name(count, "count");
      fetch_elts = fetch_count = NULL;
   }

   /*
    * Function body
    */

   block = LLVMAppendBasicBlockInContext(gallivm->context, variant_func, "entry");
   builder = gallivm->builder;
   LLVMPositionBuilderAtEnd(builder, block);

   lp_build_context_init(&bld, gallivm, lp_type_int(32));

   system_values_array = lp_build_system_values_array(gallivm, vs_info,
                                                      instance_id, NULL);

   /* function will return non-zero i32 value if any clipped vertices */
   ret_ptr = lp_build_alloca(gallivm, int32_type, "");
   LLVMBuildStore(builder, zero, ret_ptr);

   /* code generated texture sampling */
   sampler = draw_llvm_sampler_soa_create(
      draw_llvm_variant_key_samplers(&variant->key),
      context_ptr);

   if (elts) {
      start = zero;
      end = fetch_count;
   }
   else {
      end = lp_build_add(&bld, start, count);
   }

   step = lp_build_const_int32(gallivm, max_vertices);

   fetch_max = LLVMBuildSub(builder, end, one, "fetch_max");

   lp_build_loop_begin(&lp_loop, gallivm, start);
   {
      LLVMValueRef inputs[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];
      LLVMValueRef aos_attribs[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS] = { { 0 } };
      LLVMValueRef io;
      LLVMValueRef clipmask;   /* holds the clipmask value */
      const LLVMValueRef (*ptr_aos)[NUM_CHANNELS];

      if (elts)
         io_itr = lp_loop.counter;
      else
         io_itr = LLVMBuildSub(builder, lp_loop.counter, start, "");

      io = LLVMBuildGEP(builder, io_ptr, &io_itr, 1, "");
#if DEBUG_STORE
      lp_build_printf(builder, " --- io %d = %p, loop counter %d\n",
                      io_itr, io, lp_loop.counter);
#endif
      for (i = 0; i < NUM_CHANNELS; ++i) {
         LLVMValueRef true_index =
            LLVMBuildAdd(builder,
                         lp_loop.counter,
                         lp_build_const_int32(gallivm, i), "");

         /* make sure we're not out of bounds which can happen
          * if fetch_count % 4 != 0, because on the last iteration
          * a few of the 4 vertex fetches will be out of bounds */
         true_index = lp_build_min(&bld, true_index, fetch_max);

         if (elts) {
            LLVMValueRef fetch_ptr;
            fetch_ptr = LLVMBuildGEP(builder, fetch_elts,
                                     &true_index, 1, "");
            true_index = LLVMBuildLoad(builder, fetch_ptr, "fetch_elt");
         }

         for (j = 0; j < draw->pt.nr_vertex_elements; ++j) {
            struct pipe_vertex_element *velem = &draw->pt.vertex_element[j];
            LLVMValueRef vb_index =
               lp_build_const_int32(gallivm, velem->vertex_buffer_index);
            LLVMValueRef vb = LLVMBuildGEP(builder, vb_ptr, &vb_index, 1, "");
            generate_fetch(gallivm, vbuffers_ptr,
                           &aos_attribs[j][i], velem, vb, true_index,
                           instance_id);
         }
      }
      convert_to_soa(gallivm, aos_attribs, inputs,
                     draw->pt.nr_vertex_elements);

      ptr_aos = (const LLVMValueRef (*)[NUM_CHANNELS]) inputs;
      generate_vs(llvm,
                  builder,
                  outputs,
                  ptr_aos,
                  system_values_array,
                  context_ptr,
                  sampler,
                  variant->key.clamp_vertex_color);

      /* store original positions in clip before further manipulation */
      store_clip(gallivm, io, outputs, 0);
      store_clip(gallivm, io, outputs, 1);

      /* do cliptest */
      if (enable_cliptest) {
         /* allocate clipmask, assign it integer type */
         clipmask = generate_clipmask(gallivm, outputs,
                                      variant->key.clip_xy,
                                      variant->key.clip_z, 
                                      variant->key.clip_user,
                                      variant->key.clip_halfz,
                                      variant->key.ucp_enable,
                                      context_ptr);
         /* return clipping boolean value for function */
         clipmask_bool(gallivm, clipmask, ret_ptr);
      }
      else {
         clipmask = lp_build_const_int_vec(gallivm, lp_type_int_vec(32), 0);
      }
      
      /* do viewport mapping */
      if (!bypass_viewport) {
         generate_viewport(llvm, builder, outputs, context_ptr);
      }

      /* store clipmask in vertex header, 
       * original positions in clip 
       * and transformed positions in data 
       */   
      convert_to_aos(gallivm, io, outputs, clipmask,
                     vs_info->num_outputs, max_vertices);
   }

   lp_build_loop_end_cond(&lp_loop, end, step, LLVMIntUGE);

   sampler->destroy(sampler);

   ret = LLVMBuildLoad(builder, ret_ptr, "");
   LLVMBuildRet(builder, ret);

   /*
    * Translate the LLVM IR into machine code.
    */
#ifdef DEBUG
   if (LLVMVerifyFunction(variant_func, LLVMPrintMessageAction)) {
      lp_debug_dump_value(variant_func);
      assert(0);
   }
#endif

   LLVMRunFunctionPassManager(gallivm->passmgr, variant_func);

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      lp_debug_dump_value(variant_func);
      debug_printf("\n");
   }

   code = LLVMGetPointerToGlobal(gallivm->engine, variant_func);
   if (elts)
      variant->jit_func_elts = (draw_jit_vert_func_elts) pointer_to_func(code);
   else
      variant->jit_func = (draw_jit_vert_func) pointer_to_func(code);

   if (gallivm_debug & GALLIVM_DEBUG_ASM) {
      lp_disassemble(code);
   }
   lp_func_delete_body(variant_func);
}


struct draw_llvm_variant_key *
draw_llvm_make_variant_key(struct draw_llvm *llvm, char *store)
{
   unsigned i;
   struct draw_llvm_variant_key *key;
   struct lp_sampler_static_state *sampler;

   key = (struct draw_llvm_variant_key *)store;

   key->clamp_vertex_color = llvm->draw->rasterizer->clamp_vertex_color; /**/

   /* Presumably all variants of the shader should have the same
    * number of vertex elements - ie the number of shader inputs.
    */
   key->nr_vertex_elements = llvm->draw->pt.nr_vertex_elements;

   /* will have to rig this up properly later */
   key->clip_xy = llvm->draw->clip_xy;
   key->clip_z = llvm->draw->clip_z;
   key->clip_user = llvm->draw->clip_user;
   key->bypass_viewport = llvm->draw->identity_viewport;
   key->clip_halfz = !llvm->draw->rasterizer->gl_rasterization_rules;
   key->need_edgeflags = (llvm->draw->vs.edgeflag_output ? TRUE : FALSE);
   key->ucp_enable = llvm->draw->rasterizer->clip_plane_enable;
   key->pad = 0;

   /* All variants of this shader will have the same value for
    * nr_samplers.  Not yet trying to compact away holes in the
    * sampler array.
    */
   key->nr_samplers = llvm->draw->vs.vertex_shader->info.file_max[TGSI_FILE_SAMPLER] + 1;

   sampler = draw_llvm_variant_key_samplers(key);

   memcpy(key->vertex_element,
          llvm->draw->pt.vertex_element,
          sizeof(struct pipe_vertex_element) * key->nr_vertex_elements);
   
   memset(sampler, 0, key->nr_samplers * sizeof *sampler);

   for (i = 0 ; i < key->nr_samplers; i++) {
      lp_sampler_static_state(&sampler[i],
			      llvm->draw->sampler_views[i],
			      llvm->draw->samplers[i]);
   }

   return key;
}


void
draw_llvm_set_mapped_texture(struct draw_context *draw,
                             unsigned sampler_idx,
                             uint32_t width, uint32_t height, uint32_t depth,
                             uint32_t first_level, uint32_t last_level,
                             uint32_t row_stride[PIPE_MAX_TEXTURE_LEVELS],
                             uint32_t img_stride[PIPE_MAX_TEXTURE_LEVELS],
                             const void *data[PIPE_MAX_TEXTURE_LEVELS])
{
   unsigned j;
   struct draw_jit_texture *jit_tex;

   assert(sampler_idx < PIPE_MAX_VERTEX_SAMPLERS);

   jit_tex = &draw->llvm->jit_context.textures[sampler_idx];

   jit_tex->width = width;
   jit_tex->height = height;
   jit_tex->depth = depth;
   jit_tex->first_level = first_level;
   jit_tex->last_level = last_level;

   for (j = first_level; j <= last_level; j++) {
      jit_tex->data[j] = data[j];
      jit_tex->row_stride[j] = row_stride[j];
      jit_tex->img_stride[j] = img_stride[j];
   }
}


void
draw_llvm_set_sampler_state(struct draw_context *draw)
{
   unsigned i;

   for (i = 0; i < draw->num_samplers; i++) {
      struct draw_jit_texture *jit_tex = &draw->llvm->jit_context.textures[i];

      if (draw->samplers[i]) {
         jit_tex->min_lod = draw->samplers[i]->min_lod;
         jit_tex->max_lod = draw->samplers[i]->max_lod;
         jit_tex->lod_bias = draw->samplers[i]->lod_bias;
         COPY_4V(jit_tex->border_color, draw->samplers[i]->border_color.f);
      }
   }
}


void
draw_llvm_destroy_variant(struct draw_llvm_variant *variant)
{
   struct draw_llvm *llvm = variant->llvm;

   if (variant->function_elts) {
      LLVMFreeMachineCodeForFunction(llvm->gallivm->engine,
                                     variant->function_elts);
      LLVMDeleteFunction(variant->function_elts);
   }

   if (variant->function) {
      LLVMFreeMachineCodeForFunction(llvm->gallivm->engine,
                                     variant->function);
      LLVMDeleteFunction(variant->function);
   }

   remove_from_list(&variant->list_item_local);
   variant->shader->variants_cached--;
   remove_from_list(&variant->list_item_global);
   llvm->nr_variants--;
   FREE(variant);
}
