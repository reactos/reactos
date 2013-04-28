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

#include "util/u_memory.h"
#include "pipe/p_state.h"
#include "translate.h"
#include "translate_cache.h"

#include "cso_cache/cso_cache.h"
#include "cso_cache/cso_hash.h"

struct translate_cache {
   struct cso_hash *hash;
};

struct translate_cache * translate_cache_create( void )
{
   struct translate_cache *cache = MALLOC_STRUCT(translate_cache);
   if (cache == NULL) {
      return NULL;
   }

   cache->hash = cso_hash_create();
   return cache;
}


static INLINE void delete_translates(struct translate_cache *cache)
{
   struct cso_hash *hash = cache->hash;
   struct cso_hash_iter iter = cso_hash_first_node(hash);
   while (!cso_hash_iter_is_null(iter)) {
      struct translate *state = (struct translate*)cso_hash_iter_data(iter);
      iter = cso_hash_iter_next(iter);
      if (state) {
         state->release(state);
      }
   }
}

void translate_cache_destroy(struct translate_cache *cache)
{
   delete_translates(cache);
   cso_hash_delete(cache->hash);
   FREE(cache);
}


static INLINE unsigned translate_hash_key_size(struct translate_key *key)
{
   unsigned size = sizeof(struct translate_key) -
                   sizeof(struct translate_element) * (PIPE_MAX_ATTRIBS - key->nr_elements);
   return size;
}

static INLINE unsigned create_key(struct translate_key *key)
{
   unsigned hash_key;
   unsigned size = translate_hash_key_size(key);
   /*debug_printf("key size = %d, (els = %d)\n",
     size, key->nr_elements);*/
   hash_key = cso_construct_key(key, size);
   return hash_key;
}

struct translate * translate_cache_find(struct translate_cache *cache,
                                        struct translate_key *key)
{
   unsigned hash_key = create_key(key);
   struct translate *translate = (struct translate*)
      cso_hash_find_data_from_template(cache->hash,
                                       hash_key,
                                       key, sizeof(*key));

   if (!translate) {
      /* create/insert */
      translate = translate_create(key);
      cso_hash_insert(cache->hash, hash_key, translate);
   }

   return translate;
}
