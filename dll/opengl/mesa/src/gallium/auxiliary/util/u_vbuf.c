/**************************************************************************
 *
 * Copyright 2011 Marek Olšák <maraeo@gmail.com>
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
 * IN NO EVENT SHALL AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_vbuf.h"

#include "util/u_dump.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"
#include "translate/translate.h"
#include "translate/translate_cache.h"
#include "cso_cache/cso_cache.h"
#include "cso_cache/cso_hash.h"

struct u_vbuf_elements {
   unsigned count;
   struct pipe_vertex_element ve[PIPE_MAX_ATTRIBS];

   unsigned src_format_size[PIPE_MAX_ATTRIBS];

   /* If (velem[i].src_format != native_format[i]), the vertex buffer
    * referenced by the vertex element cannot be used for rendering and
    * its vertex data must be translated to native_format[i]. */
   enum pipe_format native_format[PIPE_MAX_ATTRIBS];
   unsigned native_format_size[PIPE_MAX_ATTRIBS];

   /* This might mean two things:
    * - src_format != native_format, as discussed above.
    * - src_offset % 4 != 0 (if the caps don't allow such an offset). */
   boolean incompatible_layout;
   /* Per-element flags. */
   boolean incompatible_layout_elem[PIPE_MAX_ATTRIBS];
};

enum {
   VB_VERTEX = 0,
   VB_INSTANCE = 1,
   VB_CONST = 2,
   VB_NUM = 3
};

struct u_vbuf_priv {
   struct u_vbuf b;
   struct pipe_context *pipe;
   struct translate_cache *translate_cache;
   struct cso_cache *cso_cache;

   /* Vertex element state bound by the state tracker. */
   void *saved_ve;
   /* and its associated helper structure for this module. */
   struct u_vbuf_elements *ve;

   /* Vertex elements used for the translate fallback. */
   struct pipe_vertex_element fallback_velems[PIPE_MAX_ATTRIBS];
   /* If non-NULL, this is a vertex element state used for the translate
    * fallback and therefore used for rendering too. */
   void *fallback_ve;
   /* The vertex buffer slot index where translated vertices have been
    * stored in. */
   unsigned fallback_vbs[VB_NUM];
   /* When binding the fallback vertex element state, we don't want to
    * change saved_ve and ve. This is set to TRUE in such cases. */
   boolean ve_binding_lock;

   /* Whether there is any user buffer. */
   boolean any_user_vbs;
   /* Whether there is a buffer with a non-native layout. */
   boolean incompatible_vb_layout;
   /* Per-buffer flags. */
   boolean incompatible_vb[PIPE_MAX_ATTRIBS];
};

static void u_vbuf_init_format_caps(struct u_vbuf_priv *mgr)
{
   struct pipe_screen *screen = mgr->pipe->screen;

   mgr->b.caps.format_fixed32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_FIXED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->b.caps.format_float16 =
      screen->is_format_supported(screen, PIPE_FORMAT_R16_FLOAT, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->b.caps.format_float64 =
      screen->is_format_supported(screen, PIPE_FORMAT_R64_FLOAT, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->b.caps.format_norm32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_UNORM, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER) &&
      screen->is_format_supported(screen, PIPE_FORMAT_R32_SNORM, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->b.caps.format_scaled32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_USCALED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER) &&
      screen->is_format_supported(screen, PIPE_FORMAT_R32_SSCALED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);
}

struct u_vbuf *
u_vbuf_create(struct pipe_context *pipe,
              unsigned upload_buffer_size,
              unsigned upload_buffer_alignment,
              unsigned upload_buffer_bind,
              enum u_fetch_alignment fetch_alignment)
{
   struct u_vbuf_priv *mgr = CALLOC_STRUCT(u_vbuf_priv);

   mgr->pipe = pipe;
   mgr->cso_cache = cso_cache_create();
   mgr->translate_cache = translate_cache_create();
   memset(mgr->fallback_vbs, ~0, sizeof(mgr->fallback_vbs));

   mgr->b.uploader = u_upload_create(pipe, upload_buffer_size,
                                     upload_buffer_alignment,
                                     upload_buffer_bind);

   mgr->b.caps.fetch_dword_unaligned =
         fetch_alignment == U_VERTEX_FETCH_BYTE_ALIGNED;

   u_vbuf_init_format_caps(mgr);

   return &mgr->b;
}

/* XXX I had to fork this off of cso_context. */
static void *
u_vbuf_pipe_set_vertex_elements(struct u_vbuf_priv *mgr,
                                unsigned count,
                                const struct pipe_vertex_element *states)
{
   unsigned key_size, hash_key;
   struct cso_hash_iter iter;
   void *handle;
   struct cso_velems_state velems_state;

   /* need to include the count into the stored state data too. */
   key_size = sizeof(struct pipe_vertex_element) * count + sizeof(unsigned);
   velems_state.count = count;
   memcpy(velems_state.velems, states,
          sizeof(struct pipe_vertex_element) * count);
   hash_key = cso_construct_key((void*)&velems_state, key_size);
   iter = cso_find_state_template(mgr->cso_cache, hash_key, CSO_VELEMENTS,
                                  (void*)&velems_state, key_size);

   if (cso_hash_iter_is_null(iter)) {
      struct cso_velements *cso = MALLOC_STRUCT(cso_velements);
      memcpy(&cso->state, &velems_state, key_size);
      cso->data =
            mgr->pipe->create_vertex_elements_state(mgr->pipe, count,
                                                    &cso->state.velems[0]);
      cso->delete_state =
            (cso_state_callback)mgr->pipe->delete_vertex_elements_state;
      cso->context = mgr->pipe;

      iter = cso_insert_state(mgr->cso_cache, hash_key, CSO_VELEMENTS, cso);
      handle = cso->data;
   } else {
      handle = ((struct cso_velements *)cso_hash_iter_data(iter))->data;
   }

   mgr->pipe->bind_vertex_elements_state(mgr->pipe, handle);
   return handle;
}

void u_vbuf_destroy(struct u_vbuf *mgrb)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;
   unsigned i;

   for (i = 0; i < mgr->b.nr_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->b.vertex_buffer[i].buffer, NULL);
   }
   for (i = 0; i < mgr->b.nr_real_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->b.real_vertex_buffer[i].buffer, NULL);
   }

   translate_cache_destroy(mgr->translate_cache);
   u_upload_destroy(mgr->b.uploader);
   cso_cache_delete(mgr->cso_cache);
   FREE(mgr);
}

static void
u_vbuf_translate_buffers(struct u_vbuf_priv *mgr, struct translate_key *key,
                         unsigned vb_mask, unsigned out_vb,
                         int start_vertex, unsigned num_vertices,
                         int start_index, unsigned num_indices, int min_index,
                         bool unroll_indices)
{
   struct translate *tr;
   struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS] = {0};
   struct pipe_resource *out_buffer = NULL;
   uint8_t *out_map;
   unsigned i, out_offset;

   /* Get a translate object. */
   tr = translate_cache_find(mgr->translate_cache, key);

   /* Map buffers we want to translate. */
   for (i = 0; i < mgr->b.nr_vertex_buffers; i++) {
      if (vb_mask & (1 << i)) {
         struct pipe_vertex_buffer *vb = &mgr->b.vertex_buffer[i];
         unsigned offset = vb->buffer_offset + vb->stride * start_vertex;
         uint8_t *map;

         if (u_vbuf_resource(vb->buffer)->user_ptr) {
            map = u_vbuf_resource(vb->buffer)->user_ptr + offset;
         } else {
            unsigned size = vb->stride ? num_vertices * vb->stride
                                       : sizeof(double)*4;

            if (offset+size > vb->buffer->width0) {
               size = vb->buffer->width0 - offset;
            }

            map = pipe_buffer_map_range(mgr->pipe, vb->buffer, offset, size,
                                        PIPE_TRANSFER_READ, &vb_transfer[i]);
         }

         /* Subtract min_index so that indexing with the index buffer works. */
         if (unroll_indices) {
            map -= vb->stride * min_index;
         }

         tr->set_buffer(tr, i, map, vb->stride, ~0);
      }
   }

   /* Translate. */
   if (unroll_indices) {
      struct pipe_index_buffer *ib = &mgr->b.index_buffer;
      struct pipe_transfer *transfer = NULL;
      unsigned offset = ib->offset + start_index * ib->index_size;
      uint8_t *map;

      assert(ib->buffer && ib->index_size);

      if (u_vbuf_resource(ib->buffer)->user_ptr) {
         map = u_vbuf_resource(ib->buffer)->user_ptr + offset;
      } else {
         map = pipe_buffer_map_range(mgr->pipe, ib->buffer, offset,
                                     num_indices * ib->index_size,
                                     PIPE_TRANSFER_READ, &transfer);
      }

      /* Create and map the output buffer. */
      u_upload_alloc(mgr->b.uploader, 0,
                     key->output_stride * num_indices,
                     &out_offset, &out_buffer,
                     (void**)&out_map);

      switch (ib->index_size) {
      case 4:
         tr->run_elts(tr, (unsigned*)map, num_indices, 0, out_map);
         break;
      case 2:
         tr->run_elts16(tr, (uint16_t*)map, num_indices, 0, out_map);
         break;
      case 1:
         tr->run_elts8(tr, map, num_indices, 0, out_map);
         break;
      }

      if (transfer) {
         pipe_buffer_unmap(mgr->pipe, transfer);
      }
   } else {
      /* Create and map the output buffer. */
      u_upload_alloc(mgr->b.uploader,
                     key->output_stride * start_vertex,
                     key->output_stride * num_vertices,
                     &out_offset, &out_buffer,
                     (void**)&out_map);

      out_offset -= key->output_stride * start_vertex;

      tr->run(tr, 0, num_vertices, 0, out_map);
   }

   /* Unmap all buffers. */
   for (i = 0; i < mgr->b.nr_vertex_buffers; i++) {
      if (vb_transfer[i]) {
         pipe_buffer_unmap(mgr->pipe, vb_transfer[i]);
      }
   }

   /* Setup the new vertex buffer. */
   mgr->b.real_vertex_buffer[out_vb].buffer_offset = out_offset;
   mgr->b.real_vertex_buffer[out_vb].stride = key->output_stride;

   /* Move the buffer reference. */
   pipe_resource_reference(
      &mgr->b.real_vertex_buffer[out_vb].buffer, NULL);
   mgr->b.real_vertex_buffer[out_vb].buffer = out_buffer;
}

static boolean
u_vbuf_translate_find_free_vb_slots(struct u_vbuf_priv *mgr,
                                    unsigned mask[VB_NUM])
{
   unsigned i, type;
   unsigned nr = mgr->ve->count;
   boolean used_vb[PIPE_MAX_ATTRIBS] = {0};
   unsigned fallback_vbs[VB_NUM];

   memset(fallback_vbs, ~0, sizeof(fallback_vbs));

   /* Mark used vertex buffers as... used. */
   for (i = 0; i < nr; i++) {
      if (!mgr->ve->incompatible_layout_elem[i]) {
         unsigned index = mgr->ve->ve[i].vertex_buffer_index;

         if (!mgr->incompatible_vb[index]) {
            used_vb[index] = TRUE;
         }
      }
   }

   /* Find free slots for each type if needed. */
   i = 0;
   for (type = 0; type < VB_NUM; type++) {
      if (mask[type]) {
         for (; i < PIPE_MAX_ATTRIBS; i++) {
            if (!used_vb[i]) {
               /*printf("found slot=%i for type=%i\n", i, type);*/
               fallback_vbs[type] = i;
               i++;
               if (i > mgr->b.nr_real_vertex_buffers) {
                  mgr->b.nr_real_vertex_buffers = i;
               }
               break;
            }
         }
         if (i == PIPE_MAX_ATTRIBS) {
            /* fail, reset the number to its original value */
            mgr->b.nr_real_vertex_buffers = mgr->b.nr_vertex_buffers;
            return FALSE;
         }
      }
   }

   memcpy(mgr->fallback_vbs, fallback_vbs, sizeof(fallback_vbs));
   return TRUE;
}

static boolean
u_vbuf_translate_begin(struct u_vbuf_priv *mgr,
                       int start_vertex, unsigned num_vertices,
                       int start_instance, unsigned num_instances,
                       int start_index, unsigned num_indices, int min_index,
                       bool unroll_indices)
{
   unsigned mask[VB_NUM] = {0};
   struct translate_key key[VB_NUM];
   unsigned elem_index[VB_NUM][PIPE_MAX_ATTRIBS]; /* ... into key.elements */
   unsigned i, type;

   int start[VB_NUM] = {
      start_vertex,     /* VERTEX */
      start_instance,   /* INSTANCE */
      0                 /* CONST */
   };

   unsigned num[VB_NUM] = {
      num_vertices,     /* VERTEX */
      num_instances,    /* INSTANCE */
      1                 /* CONST */
   };

   memset(key, 0, sizeof(key));
   memset(elem_index, ~0, sizeof(elem_index));

   /* See if there are vertex attribs of each type to translate and
    * which ones. */
   for (i = 0; i < mgr->ve->count; i++) {
      unsigned vb_index = mgr->ve->ve[i].vertex_buffer_index;

      if (!mgr->b.vertex_buffer[vb_index].stride) {
         if (!mgr->ve->incompatible_layout_elem[i] &&
             !mgr->incompatible_vb[vb_index]) {
            continue;
         }
         mask[VB_CONST] |= 1 << vb_index;
      } else if (mgr->ve->ve[i].instance_divisor) {
         if (!mgr->ve->incompatible_layout_elem[i] &&
             !mgr->incompatible_vb[vb_index]) {
            continue;
         }
         mask[VB_INSTANCE] |= 1 << vb_index;
      } else {
         if (!unroll_indices &&
             !mgr->ve->incompatible_layout_elem[i] &&
             !mgr->incompatible_vb[vb_index]) {
            continue;
         }
         mask[VB_VERTEX] |= 1 << vb_index;
      }
   }

   assert(mask[VB_VERTEX] || mask[VB_INSTANCE] || mask[VB_CONST]);

   /* Find free vertex buffer slots. */
   if (!u_vbuf_translate_find_free_vb_slots(mgr, mask)) {
      return FALSE;
   }

   /* Initialize the translate keys. */
   for (i = 0; i < mgr->ve->count; i++) {
      struct translate_key *k;
      struct translate_element *te;
      unsigned bit, vb_index = mgr->ve->ve[i].vertex_buffer_index;
      bit = 1 << vb_index;

      if (!mgr->ve->incompatible_layout_elem[i] &&
          !mgr->incompatible_vb[vb_index] &&
          (!unroll_indices || !(mask[VB_VERTEX] & bit))) {
         continue;
      }

      /* Set type to what we will translate.
       * Whether vertex, instance, or constant attribs. */
      for (type = 0; type < VB_NUM; type++) {
         if (mask[type] & bit) {
            break;
         }
      }
      assert(type < VB_NUM);
      assert(translate_is_output_format_supported(mgr->ve->native_format[i]));
      /*printf("velem=%i type=%i\n", i, type);*/

      /* Add the vertex element. */
      k = &key[type];
      elem_index[type][i] = k->nr_elements;

      te = &k->element[k->nr_elements];
      te->type = TRANSLATE_ELEMENT_NORMAL;
      te->instance_divisor = 0;
      te->input_buffer = vb_index;
      te->input_format = mgr->ve->ve[i].src_format;
      te->input_offset = mgr->ve->ve[i].src_offset;
      te->output_format = mgr->ve->native_format[i];
      te->output_offset = k->output_stride;

      k->output_stride += mgr->ve->native_format_size[i];
      k->nr_elements++;
   }

   /* Translate buffers. */
   for (type = 0; type < VB_NUM; type++) {
      if (key[type].nr_elements) {
         u_vbuf_translate_buffers(mgr, &key[type], mask[type],
                                  mgr->fallback_vbs[type],
                                  start[type], num[type],
                                  start_index, num_indices, min_index,
                                  unroll_indices && type == VB_VERTEX);

         /* Fixup the stride for constant attribs. */
         if (type == VB_CONST) {
            mgr->b.real_vertex_buffer[mgr->fallback_vbs[VB_CONST]].stride = 0;
         }
      }
   }

   /* Setup new vertex elements. */
   for (i = 0; i < mgr->ve->count; i++) {
      for (type = 0; type < VB_NUM; type++) {
         if (elem_index[type][i] < key[type].nr_elements) {
            struct translate_element *te = &key[type].element[elem_index[type][i]];
            mgr->fallback_velems[i].instance_divisor = mgr->ve->ve[i].instance_divisor;
            mgr->fallback_velems[i].src_format = te->output_format;
            mgr->fallback_velems[i].src_offset = te->output_offset;
            mgr->fallback_velems[i].vertex_buffer_index = mgr->fallback_vbs[type];

            /* elem_index[type][i] can only be set for one type. */
            assert(type > VB_INSTANCE || elem_index[type+1][i] == ~0);
            assert(type > VB_VERTEX   || elem_index[type+2][i] == ~0);
            break;
         }
      }
      /* No translating, just copy the original vertex element over. */
      if (type == VB_NUM) {
         memcpy(&mgr->fallback_velems[i], &mgr->ve->ve[i],
                sizeof(struct pipe_vertex_element));
      }
   }

   /* Preserve saved_ve. */
   mgr->ve_binding_lock = TRUE;
   mgr->fallback_ve = u_vbuf_pipe_set_vertex_elements(mgr, mgr->ve->count,
                                                      mgr->fallback_velems);
   mgr->ve_binding_lock = FALSE;
   return TRUE;
}

static void u_vbuf_translate_end(struct u_vbuf_priv *mgr)
{
   unsigned i;

   /* Restore vertex elements. */
   /* Note that saved_ve will be overwritten in bind_vertex_elements_state. */
   mgr->pipe->bind_vertex_elements_state(mgr->pipe, mgr->saved_ve);
   mgr->fallback_ve = NULL;

   /* Unreference the now-unused VBOs. */
   for (i = 0; i < VB_NUM; i++) {
      unsigned vb = mgr->fallback_vbs[i];
      if (vb != ~0) {
         pipe_resource_reference(&mgr->b.real_vertex_buffer[vb].buffer, NULL);
         mgr->fallback_vbs[i] = ~0;
      }
   }
   mgr->b.nr_real_vertex_buffers = mgr->b.nr_vertex_buffers;
}

#define FORMAT_REPLACE(what, withwhat) \
    case PIPE_FORMAT_##what: format = PIPE_FORMAT_##withwhat; break

struct u_vbuf_elements *
u_vbuf_create_vertex_elements(struct u_vbuf *mgrb,
                              unsigned count,
                              const struct pipe_vertex_element *attribs,
                              struct pipe_vertex_element *native_attribs)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;
   unsigned i;
   struct u_vbuf_elements *ve = CALLOC_STRUCT(u_vbuf_elements);

   ve->count = count;

   if (!count) {
      return ve;
   }

   memcpy(ve->ve, attribs, sizeof(struct pipe_vertex_element) * count);
   memcpy(native_attribs, attribs, sizeof(struct pipe_vertex_element) * count);

   /* Set the best native format in case the original format is not
    * supported. */
   for (i = 0; i < count; i++) {
      enum pipe_format format = ve->ve[i].src_format;

      ve->src_format_size[i] = util_format_get_blocksize(format);

      /* Choose a native format.
       * For now we don't care about the alignment, that's going to
       * be sorted out later. */
      if (!mgr->b.caps.format_fixed32) {
         switch (format) {
            FORMAT_REPLACE(R32_FIXED,           R32_FLOAT);
            FORMAT_REPLACE(R32G32_FIXED,        R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_FIXED,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_FIXED,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->b.caps.format_float16) {
         switch (format) {
            FORMAT_REPLACE(R16_FLOAT,           R32_FLOAT);
            FORMAT_REPLACE(R16G16_FLOAT,        R32G32_FLOAT);
            FORMAT_REPLACE(R16G16B16_FLOAT,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R16G16B16A16_FLOAT,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->b.caps.format_float64) {
         switch (format) {
            FORMAT_REPLACE(R64_FLOAT,           R32_FLOAT);
            FORMAT_REPLACE(R64G64_FLOAT,        R32G32_FLOAT);
            FORMAT_REPLACE(R64G64B64_FLOAT,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R64G64B64A64_FLOAT,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->b.caps.format_norm32) {
         switch (format) {
            FORMAT_REPLACE(R32_UNORM,           R32_FLOAT);
            FORMAT_REPLACE(R32G32_UNORM,        R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_UNORM,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_UNORM,  R32G32B32A32_FLOAT);
            FORMAT_REPLACE(R32_SNORM,           R32_FLOAT);
            FORMAT_REPLACE(R32G32_SNORM,        R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_SNORM,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_SNORM,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->b.caps.format_scaled32) {
         switch (format) {
            FORMAT_REPLACE(R32_USCALED,         R32_FLOAT);
            FORMAT_REPLACE(R32G32_USCALED,      R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_USCALED,   R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_USCALED,R32G32B32A32_FLOAT);
            FORMAT_REPLACE(R32_SSCALED,         R32_FLOAT);
            FORMAT_REPLACE(R32G32_SSCALED,      R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_SSCALED,   R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_SSCALED,R32G32B32A32_FLOAT);
            default:;
         }
      }

      native_attribs[i].src_format = format;
      ve->native_format[i] = format;
      ve->native_format_size[i] =
            util_format_get_blocksize(ve->native_format[i]);

      ve->incompatible_layout_elem[i] =
            ve->ve[i].src_format != ve->native_format[i] ||
            (!mgr->b.caps.fetch_dword_unaligned && ve->ve[i].src_offset % 4 != 0);
      ve->incompatible_layout =
            ve->incompatible_layout ||
            ve->incompatible_layout_elem[i];
   }

   /* Align the formats to the size of DWORD if needed. */
   if (!mgr->b.caps.fetch_dword_unaligned) {
      for (i = 0; i < count; i++) {
         ve->native_format_size[i] = align(ve->native_format_size[i], 4);
      }
   }

   return ve;
}

void u_vbuf_bind_vertex_elements(struct u_vbuf *mgrb,
                                 void *cso,
                                 struct u_vbuf_elements *ve)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;

   if (!cso) {
      return;
   }

   if (!mgr->ve_binding_lock) {
      mgr->saved_ve = cso;
      mgr->ve = ve;
   }
}

void u_vbuf_destroy_vertex_elements(struct u_vbuf *mgr,
                                    struct u_vbuf_elements *ve)
{
   FREE(ve);
}

void u_vbuf_set_vertex_buffers(struct u_vbuf *mgrb,
                               unsigned count,
                               const struct pipe_vertex_buffer *bufs)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;
   unsigned i;

   mgr->any_user_vbs = FALSE;
   mgr->incompatible_vb_layout = FALSE;
   memset(mgr->incompatible_vb, 0, sizeof(mgr->incompatible_vb));

   if (!mgr->b.caps.fetch_dword_unaligned) {
      /* Check if the strides and offsets are aligned to the size of DWORD. */
      for (i = 0; i < count; i++) {
         if (bufs[i].buffer) {
            if (bufs[i].stride % 4 != 0 ||
                bufs[i].buffer_offset % 4 != 0) {
               mgr->incompatible_vb_layout = TRUE;
               mgr->incompatible_vb[i] = TRUE;
            }
         }
      }
   }

   for (i = 0; i < count; i++) {
      const struct pipe_vertex_buffer *vb = &bufs[i];

      pipe_resource_reference(&mgr->b.vertex_buffer[i].buffer, vb->buffer);

      mgr->b.real_vertex_buffer[i].buffer_offset =
      mgr->b.vertex_buffer[i].buffer_offset = vb->buffer_offset;

      mgr->b.real_vertex_buffer[i].stride =
      mgr->b.vertex_buffer[i].stride = vb->stride;

      if (!vb->buffer ||
          mgr->incompatible_vb[i]) {
         pipe_resource_reference(&mgr->b.real_vertex_buffer[i].buffer, NULL);
         continue;
      }

      if (u_vbuf_resource(vb->buffer)->user_ptr) {
         pipe_resource_reference(&mgr->b.real_vertex_buffer[i].buffer, NULL);
         mgr->any_user_vbs = TRUE;
         continue;
      }

      pipe_resource_reference(&mgr->b.real_vertex_buffer[i].buffer, vb->buffer);
   }

   for (i = count; i < mgr->b.nr_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->b.vertex_buffer[i].buffer, NULL);
   }
   for (i = count; i < mgr->b.nr_real_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->b.real_vertex_buffer[i].buffer, NULL);
   }

   mgr->b.nr_vertex_buffers = count;
   mgr->b.nr_real_vertex_buffers = count;
}

void u_vbuf_set_index_buffer(struct u_vbuf *mgr,
                             const struct pipe_index_buffer *ib)
{
   if (ib && ib->buffer) {
      assert(ib->offset % ib->index_size == 0);
      pipe_resource_reference(&mgr->index_buffer.buffer, ib->buffer);
      mgr->index_buffer.offset = ib->offset;
      mgr->index_buffer.index_size = ib->index_size;
   } else {
      pipe_resource_reference(&mgr->index_buffer.buffer, NULL);
   }
}

static void
u_vbuf_upload_buffers(struct u_vbuf_priv *mgr,
                      int start_vertex, unsigned num_vertices,
                      int start_instance, unsigned num_instances)
{
   unsigned i;
   unsigned nr_velems = mgr->ve->count;
   unsigned nr_vbufs = mgr->b.nr_vertex_buffers;
   struct pipe_vertex_element *velems =
         mgr->fallback_ve ? mgr->fallback_velems : mgr->ve->ve;
   unsigned start_offset[PIPE_MAX_ATTRIBS];
   unsigned end_offset[PIPE_MAX_ATTRIBS] = {0};

   /* Determine how much data needs to be uploaded. */
   for (i = 0; i < nr_velems; i++) {
      struct pipe_vertex_element *velem = &velems[i];
      unsigned index = velem->vertex_buffer_index;
      struct pipe_vertex_buffer *vb = &mgr->b.vertex_buffer[index];
      unsigned instance_div, first, size;

      /* Skip the buffers generated by translate. */
      if (index == mgr->fallback_vbs[VB_VERTEX] ||
          index == mgr->fallback_vbs[VB_INSTANCE] ||
          index == mgr->fallback_vbs[VB_CONST]) {
         continue;
      }

      assert(vb->buffer);

      if (!u_vbuf_resource(vb->buffer)->user_ptr) {
         continue;
      }

      instance_div = velem->instance_divisor;
      first = vb->buffer_offset + velem->src_offset;

      if (!vb->stride) {
         /* Constant attrib. */
         size = mgr->ve->src_format_size[i];
      } else if (instance_div) {
         /* Per-instance attrib. */
         unsigned count = (num_instances + instance_div - 1) / instance_div;
         first += vb->stride * start_instance;
         size = vb->stride * (count - 1) + mgr->ve->src_format_size[i];
      } else {
         /* Per-vertex attrib. */
         first += vb->stride * start_vertex;
         size = vb->stride * (num_vertices - 1) + mgr->ve->src_format_size[i];
      }

      /* Update offsets. */
      if (!end_offset[index]) {
         start_offset[index] = first;
         end_offset[index] = first + size;
      } else {
         if (first < start_offset[index])
            start_offset[index] = first;
         if (first + size > end_offset[index])
            end_offset[index] = first + size;
      }
   }

   /* Upload buffers. */
   for (i = 0; i < nr_vbufs; i++) {
      unsigned start, end = end_offset[i];
      struct pipe_vertex_buffer *real_vb;
      uint8_t *ptr;

      if (!end) {
         continue;
      }

      start = start_offset[i];
      assert(start < end);

      real_vb = &mgr->b.real_vertex_buffer[i];
      ptr = u_vbuf_resource(mgr->b.vertex_buffer[i].buffer)->user_ptr;

      u_upload_data(mgr->b.uploader, start, end - start, ptr + start,
                    &real_vb->buffer_offset, &real_vb->buffer);

      real_vb->buffer_offset -= start;
   }
}

unsigned u_vbuf_draw_max_vertex_count(struct u_vbuf *mgrb)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;
   unsigned i, nr = mgr->ve->count;
   struct pipe_vertex_element *velems =
         mgr->fallback_ve ? mgr->fallback_velems : mgr->ve->ve;
   unsigned result = ~0;

   for (i = 0; i < nr; i++) {
      struct pipe_vertex_buffer *vb =
            &mgr->b.real_vertex_buffer[velems[i].vertex_buffer_index];
      unsigned size, max_count, value;

      /* We're not interested in constant and per-instance attribs. */
      if (!vb->buffer ||
          !vb->stride ||
          velems[i].instance_divisor) {
         continue;
      }

      size = vb->buffer->width0;

      /* Subtract buffer_offset. */
      value = vb->buffer_offset;
      if (value >= size) {
         return 0;
      }
      size -= value;

      /* Subtract src_offset. */
      value = velems[i].src_offset;
      if (value >= size) {
         return 0;
      }
      size -= value;

      /* Subtract format_size. */
      value = mgr->ve->native_format_size[i];
      if (value >= size) {
         return 0;
      }
      size -= value;

      /* Compute the max count. */
      max_count = 1 + size / vb->stride;
      result = MIN2(result, max_count);
   }
   return result;
}

static boolean u_vbuf_need_minmax_index(struct u_vbuf_priv *mgr)
{
   unsigned i, nr = mgr->ve->count;

   for (i = 0; i < nr; i++) {
      struct pipe_vertex_buffer *vb;
      unsigned index;

      /* Per-instance attribs don't need min/max_index. */
      if (mgr->ve->ve[i].instance_divisor) {
         continue;
      }

      index = mgr->ve->ve[i].vertex_buffer_index;
      vb = &mgr->b.vertex_buffer[index];

      /* Constant attribs don't need min/max_index. */
      if (!vb->stride) {
         continue;
      }

      /* Per-vertex attribs need min/max_index. */
      if (u_vbuf_resource(vb->buffer)->user_ptr ||
          mgr->ve->incompatible_layout_elem[i] ||
          mgr->incompatible_vb[index]) {
         return TRUE;
      }
   }

   return FALSE;
}

static boolean u_vbuf_mapping_vertex_buffer_blocks(struct u_vbuf_priv *mgr)
{
   unsigned i, nr = mgr->ve->count;

   for (i = 0; i < nr; i++) {
      struct pipe_vertex_buffer *vb;
      unsigned index;

      /* Per-instance attribs are not per-vertex data. */
      if (mgr->ve->ve[i].instance_divisor) {
         continue;
      }

      index = mgr->ve->ve[i].vertex_buffer_index;
      vb = &mgr->b.vertex_buffer[index];

      /* Constant attribs are not per-vertex data. */
      if (!vb->stride) {
         continue;
      }

      /* Return true for the hw buffers which don't need to be translated. */
      /* XXX we could use some kind of a is-busy query. */
      if (!u_vbuf_resource(vb->buffer)->user_ptr &&
          !mgr->ve->incompatible_layout_elem[i] &&
          !mgr->incompatible_vb[index]) {
         return TRUE;
      }
   }

   return FALSE;
}

static void u_vbuf_get_minmax_index(struct pipe_context *pipe,
                                    struct pipe_index_buffer *ib,
                                    const struct pipe_draw_info *info,
                                    int *out_min_index,
                                    int *out_max_index)
{
   struct pipe_transfer *transfer = NULL;
   const void *indices;
   unsigned i;
   unsigned restart_index = info->restart_index;

   if (u_vbuf_resource(ib->buffer)->user_ptr) {
      indices = u_vbuf_resource(ib->buffer)->user_ptr +
                ib->offset + info->start * ib->index_size;
   } else {
      indices = pipe_buffer_map_range(pipe, ib->buffer,
                                      ib->offset + info->start * ib->index_size,
                                      info->count * ib->index_size,
                                      PIPE_TRANSFER_READ, &transfer);
   }

   switch (ib->index_size) {
   case 4: {
      const unsigned *ui_indices = (const unsigned*)indices;
      unsigned max_ui = 0;
      unsigned min_ui = ~0U;
      if (info->primitive_restart) {
         for (i = 0; i < info->count; i++) {
            if (ui_indices[i] != restart_index) {
               if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
               if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < info->count; i++) {
            if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
            if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
         }
      }
      *out_min_index = min_ui;
      *out_max_index = max_ui;
      break;
   }
   case 2: {
      const unsigned short *us_indices = (const unsigned short*)indices;
      unsigned max_us = 0;
      unsigned min_us = ~0U;
      if (info->primitive_restart) {
         for (i = 0; i < info->count; i++) {
            if (us_indices[i] != restart_index) {
               if (us_indices[i] > max_us) max_us = us_indices[i];
               if (us_indices[i] < min_us) min_us = us_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < info->count; i++) {
            if (us_indices[i] > max_us) max_us = us_indices[i];
            if (us_indices[i] < min_us) min_us = us_indices[i];
         }
      }
      *out_min_index = min_us;
      *out_max_index = max_us;
      break;
   }
   case 1: {
      const unsigned char *ub_indices = (const unsigned char*)indices;
      unsigned max_ub = 0;
      unsigned min_ub = ~0U;
      if (info->primitive_restart) {
         for (i = 0; i < info->count; i++) {
            if (ub_indices[i] != restart_index) {
               if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
               if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < info->count; i++) {
            if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
            if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
         }
      }
      *out_min_index = min_ub;
      *out_max_index = max_ub;
      break;
   }
   default:
      assert(0);
      *out_min_index = 0;
      *out_max_index = 0;
   }

   if (transfer) {
      pipe_buffer_unmap(pipe, transfer);
   }
}

enum u_vbuf_return_flags
u_vbuf_draw_begin(struct u_vbuf *mgrb,
                  struct pipe_draw_info *info)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;
   int start_vertex, min_index;
   unsigned num_vertices;
   bool unroll_indices = false;

   if (!mgr->incompatible_vb_layout &&
       !mgr->ve->incompatible_layout &&
       !mgr->any_user_vbs) {
      return 0;
   }

   if (info->indexed) {
      int max_index;
      bool index_bounds_valid = false;

      if (info->max_index != ~0) {
         min_index = info->min_index;
         max_index = info->max_index;
         index_bounds_valid = true;
      } else if (u_vbuf_need_minmax_index(mgr)) {
         u_vbuf_get_minmax_index(mgr->pipe, &mgr->b.index_buffer, info,
                                 &min_index, &max_index);
         index_bounds_valid = true;
      }

      /* If the index bounds are valid, it means some upload or translation
       * of per-vertex attribs will be performed. */
      if (index_bounds_valid) {
         assert(min_index <= max_index);

         start_vertex = min_index + info->index_bias;
         num_vertices = max_index + 1 - min_index;

         /* Primitive restart doesn't work when unrolling indices.
          * We would have to break this drawing operation into several ones. */
         /* Use some heuristic to see if unrolling indices improves
          * performance. */
         if (!info->primitive_restart &&
             num_vertices > info->count*2 &&
             num_vertices-info->count > 32 &&
             !u_vbuf_mapping_vertex_buffer_blocks(mgr)) {
            /*printf("num_vertices=%i count=%i\n", num_vertices, info->count);*/
            unroll_indices = true;
         }
      } else {
         /* Nothing to do for per-vertex attribs. */
         start_vertex = 0;
         num_vertices = 0;
         min_index = 0;
      }
   } else {
      start_vertex = info->start;
      num_vertices = info->count;
      min_index = 0;
   }

   /* Translate vertices with non-native layouts or formats. */
   if (unroll_indices ||
       mgr->incompatible_vb_layout ||
       mgr->ve->incompatible_layout) {
      /* XXX check the return value */
      u_vbuf_translate_begin(mgr, start_vertex, num_vertices,
                             info->start_instance, info->instance_count,
                             info->start, info->count, min_index,
                             unroll_indices);
   }

   /* Upload user buffers. */
   if (mgr->any_user_vbs) {
      u_vbuf_upload_buffers(mgr, start_vertex, num_vertices,
                            info->start_instance, info->instance_count);
   }

   /*
   if (unroll_indices) {
      printf("unrolling indices: start_vertex = %i, num_vertices = %i\n",
             start_vertex, num_vertices);
      util_dump_draw_info(stdout, info);
      printf("\n");
   }

   unsigned i;
   for (i = 0; i < mgr->b.nr_vertex_buffers; i++) {
      printf("input %i: ", i);
      util_dump_vertex_buffer(stdout, mgr->b.vertex_buffer+i);
      printf("\n");
   }
   for (i = 0; i < mgr->b.nr_real_vertex_buffers; i++) {
      printf("real %i: ", i);
      util_dump_vertex_buffer(stdout, mgr->b.real_vertex_buffer+i);
      printf("\n");
   }
   */

   if (unroll_indices) {
      info->indexed = FALSE;
      info->index_bias = 0;
      info->min_index = 0;
      info->max_index = info->count - 1;
      info->start = 0;
   }

   return U_VBUF_BUFFERS_UPDATED;
}

void u_vbuf_draw_end(struct u_vbuf *mgrb)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;

   if (mgr->fallback_ve) {
      u_vbuf_translate_end(mgr);
   }
}
