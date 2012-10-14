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

#ifndef SP_TEX_TILE_CACHE_H
#define SP_TEX_TILE_CACHE_H


#include "pipe/p_compiler.h"
#include "sp_limits.h"


struct softpipe_context;
struct softpipe_tex_tile_cache;


/**
 * Cache tile size (width and height). This needs to be a power of two.
 */
#define TILE_SIZE_LOG2 6
#define TILE_SIZE (1 << TILE_SIZE_LOG2)


#define TEX_ADDR_BITS (SP_MAX_TEXTURE_2D_LEVELS - 1 - TILE_SIZE_LOG2)
#define TEX_Z_BITS (SP_MAX_TEXTURE_2D_LEVELS - 1)

/**
 * Texture tile address as a union for fast compares.
 */
union tex_tile_address {
   struct {
      unsigned x:TEX_ADDR_BITS;  /* 16K / TILE_SIZE */
      unsigned y:TEX_ADDR_BITS;  /* 16K / TILE_SIZE */
      unsigned z:TEX_Z_BITS;     /* 16K -- z not tiled */
      unsigned face:3;
      unsigned level:4;
      unsigned invalid:1;
   } bits;
   uint64_t value;
};


struct softpipe_tex_cached_tile
{
   union tex_tile_address addr;
   union {
      float color[TILE_SIZE][TILE_SIZE][4];
      unsigned int colorui[TILE_SIZE][TILE_SIZE][4];
      int colori[TILE_SIZE][TILE_SIZE][4];
   } data;
};

#define NUM_ENTRIES 50

struct softpipe_tex_tile_cache
{
   struct pipe_context *pipe;
   struct pipe_transfer *transfer;
   void *transfer_map;

   struct pipe_resource *texture;  /**< if caching a texture */
   unsigned timestamp;

   struct softpipe_tex_cached_tile entries[NUM_ENTRIES];

   struct pipe_transfer *tex_trans;
   void *tex_trans_map;
   int tex_face, tex_level, tex_z;

   unsigned swizzle_r;
   unsigned swizzle_g;
   unsigned swizzle_b;
   unsigned swizzle_a;
   enum pipe_format format;

   struct softpipe_tex_cached_tile *last_tile;  /**< most recently retrieved tile */
};


extern struct softpipe_tex_tile_cache *
sp_create_tex_tile_cache( struct pipe_context *pipe );

extern void
sp_destroy_tex_tile_cache(struct softpipe_tex_tile_cache *tc);


extern void
sp_tex_tile_cache_map_transfers(struct softpipe_tex_tile_cache *tc);

extern void
sp_tex_tile_cache_unmap_transfers(struct softpipe_tex_tile_cache *tc);

extern void
sp_tex_tile_cache_set_sampler_view(struct softpipe_tex_tile_cache *tc,
                                   struct pipe_sampler_view *view);

void
sp_tex_tile_cache_validate_texture(struct softpipe_tex_tile_cache *tc);

extern void
sp_flush_tex_tile_cache(struct softpipe_tex_tile_cache *tc);



extern const struct softpipe_tex_cached_tile *
sp_find_cached_tile_tex(struct softpipe_tex_tile_cache *tc, 
                         union tex_tile_address addr );

static INLINE union tex_tile_address
tex_tile_address( unsigned x,
                  unsigned y,
                  unsigned z,
                  unsigned face,
                  unsigned level )
{
   union tex_tile_address addr;

   addr.value = 0;
   addr.bits.x = x / TILE_SIZE;
   addr.bits.y = y / TILE_SIZE;
   addr.bits.z = z;
   addr.bits.face = face;
   addr.bits.level = level;

   return addr;
}

/* Quickly retrieve tile if it matches last lookup.
 */
static INLINE const struct softpipe_tex_cached_tile *
sp_get_cached_tile_tex(struct softpipe_tex_tile_cache *tc, 
                         union tex_tile_address addr )
{
   if (tc->last_tile->addr.value == addr.value)
      return tc->last_tile;

   return sp_find_cached_tile_tex( tc, addr );
}


#endif /* SP_TEX_TILE_CACHE_H */

