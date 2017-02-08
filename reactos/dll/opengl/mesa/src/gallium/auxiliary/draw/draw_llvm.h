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

#ifndef DRAW_LLVM_H
#define DRAW_LLVM_H

#include "draw/draw_private.h"

#include "draw/draw_vs.h"
#include "gallivm/lp_bld_sample.h"

#include "pipe/p_context.h"
#include "util/u_simple_list.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/ExecutionEngine.h>


struct draw_llvm;
struct llvm_vertex_shader;

struct draw_jit_texture
{
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t first_level;
   uint32_t last_level;
   uint32_t row_stride[PIPE_MAX_TEXTURE_LEVELS];
   uint32_t img_stride[PIPE_MAX_TEXTURE_LEVELS];
   const void *data[PIPE_MAX_TEXTURE_LEVELS];
   float min_lod;
   float max_lod;
   float lod_bias;
   float border_color[4];
};

enum {
   DRAW_JIT_TEXTURE_WIDTH = 0,
   DRAW_JIT_TEXTURE_HEIGHT,
   DRAW_JIT_TEXTURE_DEPTH,
   DRAW_JIT_TEXTURE_FIRST_LEVEL,
   DRAW_JIT_TEXTURE_LAST_LEVEL,
   DRAW_JIT_TEXTURE_ROW_STRIDE,
   DRAW_JIT_TEXTURE_IMG_STRIDE,
   DRAW_JIT_TEXTURE_DATA,
   DRAW_JIT_TEXTURE_MIN_LOD,
   DRAW_JIT_TEXTURE_MAX_LOD,
   DRAW_JIT_TEXTURE_LOD_BIAS,
   DRAW_JIT_TEXTURE_BORDER_COLOR,
   DRAW_JIT_TEXTURE_NUM_FIELDS  /* number of fields above */
};

enum {
   DRAW_JIT_VERTEX_VERTEX_ID = 0,
   DRAW_JIT_VERTEX_CLIP,
   DRAW_JIT_VERTEX_PRE_CLIP_POS,
   DRAW_JIT_VERTEX_DATA
};

/**
 * This structure is passed directly to the generated vertex shader.
 *
 * It contains the derived state.
 *
 * Changes here must be reflected in the draw_jit_context_* macros.
 * Changes to the ordering should be avoided.
 *
 * Only use types with a clear size and padding here, in particular prefer the
 * stdint.h types to the basic integer types.
 */
struct draw_jit_context
{
   const float *vs_constants;
   const float *gs_constants;
   float (*planes) [DRAW_TOTAL_CLIP_PLANES][4];
   float *viewport;

   struct draw_jit_texture textures[PIPE_MAX_VERTEX_SAMPLERS];
};


#define draw_jit_context_vs_constants(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, 0, "vs_constants")

#define draw_jit_context_gs_constants(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, 1, "gs_constants")

#define draw_jit_context_planes(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, 2, "planes")

#define draw_jit_context_viewport(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, 3, "viewport")

#define DRAW_JIT_CTX_TEXTURES 4

#define draw_jit_context_textures(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_CTX_TEXTURES, "textures")

#define draw_jit_header_id(_gallivm, _ptr)              \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_VERTEX_VERTEX_ID, "id")

#define draw_jit_header_clip(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_VERTEX_CLIP, "clip")

#define draw_jit_header_pre_clip_pos(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_VERTEX_PRE_CLIP_POS, "pre_clip_pos")

#define draw_jit_header_data(_gallivm, _ptr)            \
   lp_build_struct_get_ptr(_gallivm, _ptr, DRAW_JIT_VERTEX_DATA, "data")


#define draw_jit_vbuffer_stride(_gallivm, _ptr)         \
   lp_build_struct_get(_gallivm, _ptr, 0, "stride")

#define draw_jit_vbuffer_offset(_gallivm, _ptr)         \
   lp_build_struct_get(_gallivm, _ptr, 1, "buffer_offset")


typedef int
(*draw_jit_vert_func)(struct draw_jit_context *context,
                      struct vertex_header *io,
                      const char *vbuffers[PIPE_MAX_ATTRIBS],
                      unsigned start,
                      unsigned count,
                      unsigned stride,
                      struct pipe_vertex_buffer *vertex_buffers,
                      unsigned instance_id);


typedef int
(*draw_jit_vert_func_elts)(struct draw_jit_context *context,
                           struct vertex_header *io,
                           const char *vbuffers[PIPE_MAX_ATTRIBS],
                           const unsigned *fetch_elts,
                           unsigned fetch_count,
                           unsigned stride,
                           struct pipe_vertex_buffer *vertex_buffers,
                           unsigned instance_id);

struct draw_llvm_variant_key
{
   unsigned nr_vertex_elements:8;
   unsigned nr_samplers:8;
   unsigned clamp_vertex_color:1;
   unsigned clip_xy:1;
   unsigned clip_z:1;
   unsigned clip_user:1;
   unsigned clip_halfz:1;
   unsigned bypass_viewport:1;
   unsigned need_edgeflags:1;
   unsigned ucp_enable:PIPE_MAX_CLIP_PLANES;
   unsigned pad:9-PIPE_MAX_CLIP_PLANES;

   /* Variable number of vertex elements:
    */
   struct pipe_vertex_element vertex_element[1];

   /* Followed by variable number of samplers:
    */
/*   struct lp_sampler_static_state sampler; */
};

#define DRAW_LLVM_MAX_VARIANT_KEY_SIZE \
   (sizeof(struct draw_llvm_variant_key) +	\
    PIPE_MAX_VERTEX_SAMPLERS * sizeof(struct lp_sampler_static_state) +	\
    (PIPE_MAX_ATTRIBS-1) * sizeof(struct pipe_vertex_element))


static INLINE size_t
draw_llvm_variant_key_size(unsigned nr_vertex_elements,
			   unsigned nr_samplers)
{
   return (sizeof(struct draw_llvm_variant_key) +
	   nr_samplers * sizeof(struct lp_sampler_static_state) +
	   (nr_vertex_elements - 1) * sizeof(struct pipe_vertex_element));
}


static INLINE struct lp_sampler_static_state *
draw_llvm_variant_key_samplers(struct draw_llvm_variant_key *key)
{
   return (struct lp_sampler_static_state *)
      &key->vertex_element[key->nr_vertex_elements];
}



struct draw_llvm_variant_list_item
{
   struct draw_llvm_variant *base;
   struct draw_llvm_variant_list_item *next, *prev;
};

struct draw_llvm_variant
{
   LLVMValueRef function;
   LLVMValueRef function_elts;
   draw_jit_vert_func jit_func;
   draw_jit_vert_func_elts jit_func_elts;

   struct llvm_vertex_shader *shader;

   struct draw_llvm *llvm;
   struct draw_llvm_variant_list_item list_item_global;
   struct draw_llvm_variant_list_item list_item_local;

   /* key is variable-sized, must be last */
   struct draw_llvm_variant_key key;
};

struct llvm_vertex_shader {
   struct draw_vertex_shader base;

   unsigned variant_key_size;
   struct draw_llvm_variant_list_item variants;
   unsigned variants_created;
   unsigned variants_cached;
};

struct draw_llvm {
   struct draw_context *draw;

   struct draw_jit_context jit_context;

   struct gallivm_state *gallivm;

   struct draw_llvm_variant_list_item vs_variants_list;
   int nr_variants;

   /* LLVM JIT builder types */
   LLVMTypeRef context_ptr_type;
   LLVMTypeRef buffer_ptr_type;
   LLVMTypeRef vb_ptr_type;
   LLVMTypeRef vertex_header_ptr_type;
};


static INLINE struct llvm_vertex_shader *
llvm_vertex_shader(struct draw_vertex_shader *vs)
{
   return (struct llvm_vertex_shader *)vs;
}


struct draw_llvm *
draw_llvm_create(struct draw_context *draw, struct gallivm_state *gallivm);

void
draw_llvm_destroy(struct draw_llvm *llvm);

struct draw_llvm_variant *
draw_llvm_create_variant(struct draw_llvm *llvm,
			 unsigned num_vertex_header_attribs,
			 const struct draw_llvm_variant_key *key);

void
draw_llvm_destroy_variant(struct draw_llvm_variant *variant);

struct draw_llvm_variant_key *
draw_llvm_make_variant_key(struct draw_llvm *llvm, char *store);

LLVMValueRef
draw_llvm_translate_from(struct gallivm_state *gallivm,
                         LLVMValueRef vbuffer,
                         enum pipe_format from_format);

struct lp_build_sampler_soa *
draw_llvm_sampler_soa_create(const struct lp_sampler_static_state *static_state,
                             LLVMValueRef context_ptr);

void
draw_llvm_set_sampler_state(struct draw_context *draw);

void
draw_llvm_set_mapped_texture(struct draw_context *draw,
                             unsigned sampler_idx,
                             uint32_t width, uint32_t height, uint32_t depth,
                             uint32_t first_level, uint32_t last_level,
                             uint32_t row_stride[PIPE_MAX_TEXTURE_LEVELS],
                             uint32_t img_stride[PIPE_MAX_TEXTURE_LEVELS],
                             const void *data[PIPE_MAX_TEXTURE_LEVELS]);

#endif
