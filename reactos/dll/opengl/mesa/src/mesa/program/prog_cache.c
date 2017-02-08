/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/imports.h"
#include "main/shaderobj.h"
#include "program/prog_cache.h"
#include "program/program.h"


struct cache_item
{
   GLuint hash;
   void *key;
   struct gl_program *program;
   struct cache_item *next;
};

struct gl_program_cache
{
   struct cache_item **items;
   struct cache_item *last;
   GLuint size, n_items;
};



/**
 * Compute hash index from state key.
 */
static GLuint
hash_key(const void *key, GLuint key_size)
{
   const GLuint *ikey = (const GLuint *) key;
   GLuint hash = 0, i;

   assert(key_size >= 4);

   /* Make a slightly better attempt at a hash function:
    */
   for (i = 0; i < key_size / sizeof(*ikey); i++)
   {
      hash += ikey[i];
      hash += (hash << 10);
      hash ^= (hash >> 6);
   }

   return hash;
}


/**
 * Rebuild/expand the hash table to accomodate more entries
 */
static void
rehash(struct gl_program_cache *cache)
{
   struct cache_item **items;
   struct cache_item *c, *next;
   GLuint size, i;

   cache->last = NULL;

   size = cache->size * 3;
   items = (struct cache_item**) malloc(size * sizeof(*items));
   memset(items, 0, size * sizeof(*items));

   for (i = 0; i < cache->size; i++)
      for (c = cache->items[i]; c; c = next) {
	 next = c->next;
	 c->next = items[c->hash % size];
	 items[c->hash % size] = c;
      }

   free(cache->items);
   cache->items = items;
   cache->size = size;
}


static void
clear_cache(struct gl_context *ctx, struct gl_program_cache *cache,
	    GLboolean shader)
{
   struct cache_item *c, *next;
   GLuint i;
   
   cache->last = NULL;

   for (i = 0; i < cache->size; i++) {
      for (c = cache->items[i]; c; c = next) {
	 next = c->next;
	 free(c->key);
	 if (shader) {
	    _mesa_reference_shader_program(ctx,
					   (struct gl_shader_program **)&c->program,
					   NULL);
	 } else {
	    _mesa_reference_program(ctx, &c->program, NULL);
	 }
	 free(c);
      }
      cache->items[i] = NULL;
   }


   cache->n_items = 0;
}



struct gl_program_cache *
_mesa_new_program_cache(void)
{
   struct gl_program_cache *cache = CALLOC_STRUCT(gl_program_cache);
   if (cache) {
      cache->size = 17;
      cache->items = (struct cache_item **)
         calloc(1, cache->size * sizeof(struct cache_item));
      if (!cache->items) {
         free(cache);
         return NULL;
      }
   }
   return cache;
}


void
_mesa_delete_program_cache(struct gl_context *ctx, struct gl_program_cache *cache)
{
   clear_cache(ctx, cache, GL_FALSE);
   free(cache->items);
   free(cache);
}

void
_mesa_delete_shader_cache(struct gl_context *ctx,
			  struct gl_program_cache *cache)
{
   clear_cache(ctx, cache, GL_TRUE);
   free(cache->items);
   free(cache);
}


struct gl_program *
_mesa_search_program_cache(struct gl_program_cache *cache,
                           const void *key, GLuint keysize)
{
   if (cache->last && 
       memcmp(cache->last->key, key, keysize) == 0) {
      return cache->last->program;
   }
   else {
      const GLuint hash = hash_key(key, keysize);
      struct cache_item *c;

      for (c = cache->items[hash % cache->size]; c; c = c->next) {
         if (c->hash == hash && memcmp(c->key, key, keysize) == 0) {
            cache->last = c;
            return c->program;
         }
      }

      return NULL;
   }
}


void
_mesa_program_cache_insert(struct gl_context *ctx,
                           struct gl_program_cache *cache,
                           const void *key, GLuint keysize,
                           struct gl_program *program)
{
   const GLuint hash = hash_key(key, keysize);
   struct cache_item *c = CALLOC_STRUCT(cache_item);

   c->hash = hash;

   c->key = malloc(keysize);
   memcpy(c->key, key, keysize);

   c->program = program;  /* no refcount change */

   if (cache->n_items > cache->size * 1.5) {
      if (cache->size < 1000)
	 rehash(cache);
      else 
	 clear_cache(ctx, cache, GL_FALSE);
   }

   cache->n_items++;
   c->next = cache->items[hash % cache->size];
   cache->items[hash % cache->size] = c;
}

void
_mesa_shader_cache_insert(struct gl_context *ctx,
			  struct gl_program_cache *cache,
			  const void *key, GLuint keysize,
			  struct gl_shader_program *program)
{
   const GLuint hash = hash_key(key, keysize);
   struct cache_item *c = CALLOC_STRUCT(cache_item);

   c->hash = hash;

   c->key = malloc(keysize);
   memcpy(c->key, key, keysize);

   c->program = (struct gl_program *)program;  /* no refcount change */

   if (cache->n_items > cache->size * 1.5) {
      if (cache->size < 1000)
	 rehash(cache);
      else
	 clear_cache(ctx, cache, GL_TRUE);
   }

   cache->n_items++;
   c->next = cache->items[hash % cache->size];
   cache->items[hash % cache->size] = c;
}
