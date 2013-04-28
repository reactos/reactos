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
 * Key lookup/associative container.
 *
 * Like Jose's util_hash_table, based on CSO cache code for now.
 *
 * Author: Brian Paul
 */


#include "pipe/p_compiler.h"
#include "util/u_debug.h"

#include "cso_cache/cso_hash.h"

#include "util/u_memory.h"
#include "util/u_keymap.h"


struct keymap
{
   struct cso_hash *cso;   
   unsigned key_size;
   unsigned max_entries; /* XXX not obeyed net */
   unsigned num_entries;
   keymap_delete_func delete_func;
};


struct keymap_item
{
   void *key, *value;
};


/**
 * This the default key-delete function used when the client doesn't
 * provide one.
 */
static void
default_delete_func(const struct keymap *map,
                    const void *key, void *data, void *user)
{
   FREE((void*) data);
}


static INLINE struct keymap_item *
hash_table_item(struct cso_hash_iter iter)
{
   return (struct keymap_item *) cso_hash_iter_data(iter);
}


/**
 * Return 4-byte hash key for a block of bytes.
 */
static unsigned
hash(const void *key, unsigned keySize)
{
   unsigned i, hash;

   keySize /= 4; /* convert from bytes to uints */

   hash = 0;
   for (i = 0; i < keySize; i++) {
      hash ^= (i + 1) * ((const unsigned *) key)[i];
   }

   /*hash = hash ^ (hash >> 11) ^ (hash >> 22);*/

   return hash;
}


/**
 * Create a new map.
 * \param keySize  size of the keys in bytes
 * \param maxEntries  max number of entries to allow (~0 = infinity)
 * \param deleteFunc  optional callback to call when entries
 *                    are deleted/replaced
 */
struct keymap *
util_new_keymap(unsigned keySize, unsigned maxEntries,
                 keymap_delete_func deleteFunc)
{
   struct keymap *map = MALLOC_STRUCT(keymap);
   if (!map)
      return NULL;
   
   map->cso = cso_hash_create();
   if (!map->cso) {
      FREE(map);
      return NULL;
   }
   
   map->max_entries = maxEntries;
   map->num_entries = 0;
   map->key_size = keySize;
   map->delete_func = deleteFunc ? deleteFunc : default_delete_func;

   return map;
}


/**
 * Delete/free a keymap and all entries.  The deleteFunc that was given at
 * create time will be called for each entry.
 * \param user  user-provided pointer passed through to the delete callback
 */
void
util_delete_keymap(struct keymap *map, void *user)
{
   util_keymap_remove_all(map, user);
   cso_hash_delete(map->cso);
   FREE(map);
}


static INLINE struct cso_hash_iter
hash_table_find_iter(const struct keymap *map, const void *key,
                     unsigned key_hash)
{
   struct cso_hash_iter iter;
   struct keymap_item *item;
   
   iter = cso_hash_find(map->cso, key_hash);
   while (!cso_hash_iter_is_null(iter)) {
      item = (struct keymap_item *) cso_hash_iter_data(iter);
      if (!memcmp(item->key, key, map->key_size))
         break;
      iter = cso_hash_iter_next(iter);
   }
   
   return iter;
}


static INLINE struct keymap_item *
hash_table_find_item(const struct keymap *map, const void *key,
                     unsigned key_hash)
{
   struct cso_hash_iter iter = hash_table_find_iter(map, key, key_hash);
   if (cso_hash_iter_is_null(iter)) {
      return NULL;
   }
   else {
      return hash_table_item(iter);
   }
}


/**
 * Insert a new key + data pointer into the table.
 * Note: we create a copy of the key, but not the data!
 * If the key is already present in the table, replace the existing
 * entry (calling the delete callback on the previous entry).
 * If the maximum capacity of the map is reached an old entry
 * will be deleted (the delete callback will be called).
 */
boolean
util_keymap_insert(struct keymap *map, const void *key,
                   const void *data, void *user)
{
   unsigned key_hash;
   struct keymap_item *item;
   struct cso_hash_iter iter;

   assert(map);
   if (!map)
      return FALSE;

   key_hash = hash(key, map->key_size);

   item = hash_table_find_item(map, key, key_hash);
   if (item) {
      /* call delete callback for old entry/item */
      map->delete_func(map, item->key, item->value, user);
      item->value = (void *) data;
      return TRUE;
   }
   
   item = MALLOC_STRUCT(keymap_item);
   if (!item)
      return FALSE;

   item->key = mem_dup(key, map->key_size);
   item->value = (void *) data;
   
   iter = cso_hash_insert(map->cso, key_hash, item);
   if (cso_hash_iter_is_null(iter)) {
      FREE(item);
      return FALSE;
   }

   map->num_entries++;

   return TRUE;
}


/**
 * Look up a key in the map and return the associated data pointer.
 */
const void *
util_keymap_lookup(const struct keymap *map, const void *key)
{
   unsigned key_hash;
   struct keymap_item *item;

   assert(map);
   if (!map)
      return NULL;

   key_hash = hash(key, map->key_size);

   item = hash_table_find_item(map, key, key_hash);
   if (!item)
      return NULL;
   
   return item->value;
}


/**
 * Remove an entry from the map.
 * The delete callback will be called if the given key/entry is found.
 * \param user  passed to the delete callback as the last param.
 */
void
util_keymap_remove(struct keymap *map, const void *key, void *user)
{
   unsigned key_hash;
   struct cso_hash_iter iter;
   struct keymap_item *item;

   assert(map);
   if (!map)
      return;

   key_hash = hash(key, map->key_size);

   iter = hash_table_find_iter(map, key, key_hash);
   if (cso_hash_iter_is_null(iter))
      return;
   
   item = hash_table_item(iter);
   assert(item);
   if (!item)
      return;
   map->delete_func(map, item->key, item->value, user);
   FREE(item->key);
   FREE(item);
   
   map->num_entries--;

   cso_hash_erase(map->cso, iter);
}


/**
 * Remove all entries from the map, calling the delete callback for each.
 * \param user  passed to the delete callback as the last param.
 */
void
util_keymap_remove_all(struct keymap *map, void *user)
{
   struct cso_hash_iter iter;
   struct keymap_item *item;

   assert(map);
   if (!map)
      return;

   iter = cso_hash_first_node(map->cso);
   while (!cso_hash_iter_is_null(iter)) {
      item = (struct keymap_item *)
         cso_hash_take(map->cso, cso_hash_iter_key(iter));
      map->delete_func(map, item->key, item->value, user);
      FREE(item->key);
      FREE(item);
      iter = cso_hash_first_node(map->cso);
   }
}


extern void
util_keymap_info(const struct keymap *map)
{
   debug_printf("Keymap %p: %u of max %u entries\n",
                (void *) map, map->num_entries, map->max_entries);
}
